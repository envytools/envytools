/*
 *
 * Copyright (C) 2009 Marcin Kościelnicki <koriakin@0x04.net>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "dis.h"

/*
 * Table of Contents
 *  0. Table of Contents
 *  1. Instructions
 *  2. Differences from PTX
 *  3. General instruction format
 *  4. Misc. hack alerts
 *  5. Color scheme
 *  6. Table format
 *  7. Code target field
 *  8. Immediate field
 *  9. Misc number fields
 * 10. Register fields
 * 11. Memory fields
 * 12. Short instructions
 * 13. Immediate instructions
 * 14. Long instructions
 * 15. Non-predicated long instructions
 */

/*
 * Instructions, from PTX manual:
 *   
 *   1. Integer Arithmetic
 *    - sad.64		TODO	emulated
 *    - div		TODO	emulated
 *    - rem		TODO	emulated
 *    - abs		desc...
 *    - neg		desc...
 *   2. Floating-Point
 *    - add		desc...
 *    - sub		desc...
 *    - mul		desc...
 *    - fma		XXX
 *    - mad		XXX
 *    - div.approxf32	desc...
 *    - div.full.f32	TODO	emulated
 *    - div.f64		TODO	emulated
 *    - abs		desc...
 *    - neg		desc...
 *    - min		desc...
 *    - max		desc...
 *    - rcp.f32		desc...
 *    - rcp.f64		TODO	emulated
 *    - sqrt.f32	desc...
 *    - sqrt.f64	TODO	emulated
 *    - rsqrt.f32	desc...
 *    - rsqrt.f64	TODO	emulated
 *    - sin		XXX
 *    - cos		XXX
 *    - lg2		desc
 *    - ex2		XXX
 *   3. Comparison and selection
 *    - selp		desc...
 *    - slct		desc...
 *   5. Data Movement and Conversion
 *    - mov		desc...
 *    - ld		desc...
 *    - st		desc...
 *    - cvt		XXX cleanup, rounding modes
 *   6. Texture
 *    - tex		desc...
 *   7. Control Flow
 *    - { }		done
 *    - @		done
 *    - bra		desc...
 *    - call		XXX WTF?
 *    - ret		desc...
 *    - exit		desc...
 *   8. Parallel Synchronization and Communication
 *    - bar		desc...
 *    - membar.cta	desc...
 *    - membar.gl	sweet mother of God, what?
 *    - atom		desc...
 *    - red		desc...
 *    - vote		desc...
 *   9. Miscellaneous
 *    - trap		desc...
 *    - brkpt		desc...
 *    - pmevent		desc... XXX
 *  10. Undocumented/implicit stuff
 *    - join		XXX what does it do?
 *    - joinat		desc...
 *    - interp		XXX Needs figuring out.
 *    - discard		XXX what happens after you use it?
 *    - emit/reset	XXX can do combo?
 *
 */

/*
 * ISA differences from what's published about PTX:
 *
 * Instructions that don't really exist and are emulated:
 *  - sqrt.f32 -> rsqrt, rcp
 *  - neg and abs on f32 -> cvt
 *  - cvt.sat to f64 is implemented in software.
 *  - cvt with floating point can only convert up/down one size level, you
 *    need a chain of floats to go f32->u8 and others.
 *  - no 64-bit loads/stores except in l[] and g[].
 *  - vote.uni is emulated by vote.all
 *  - membar.cta is actually a nop that just forces instruction ordering
 *    before and after it.
 *  - div.approx.f32 doesn't exist and is replaced by mul by rcp
 *  - selp and slct. Emulated with set and predicated mov's.
 * Additional instructions and other stuff:
 *  - a lot of instructions can accept c?[], s[] directly
 *  - cvt can optionally abs and neg stuff before use. the sequence of
 *    operations is: abs, neg, convert+round, sat
 * Instructions that behave otherwise than you'd expect:
 *  - sin, cos, ex2 are split into two parts: function and prefunction. you
 *    need to run prefunction on input, and the real function on output from
 *    prefunction. no idea yet what the prefunctions are. presin is same as
 *    precos. presin could be mod-pi, what about preex2?
 *  - cvt to integer types always saturates. if you need chopping, just ignore
 *    the high part [or upconvert from a low part of register to full]
 *  - cvt.sat to f32 doesn't convert NaNs to 0. max.f32 with 0 to finish it off
 *  - cvt.rpi.f64 may or may not work on 1.25, add a specific check for it
 *  - same for rmi and -1.25.
 * Misc:
 *  - explicit join points
 *  - $a registers exist
 *  - there are actually 16 const spaces and global spaces.
 * Incl. graphics stuff:
 *  - v[] space exists
 *  - interp instruction
 *  - discard instruction
 *  - $o registers exist
 *
 * XXX this section is probably useless, remove it once proper ISA
 * documentation exists.
 */

/*
 * General instruction format
 * 
 * Machine code is read in 64-bit naturaly-aligned chunks, consisting of two
 * 32-bit words. If bit 0 of first word is 0, this chunk consists of two
 * single-word short instructions. Otherwise, it's a double-word instruction.
 * For double-word instructions, bits 0x20-0x21 further determine its type:
 *  00: long
 *  01: long with exit after it
 *  10: long with join before it
 *  11: immediate
 *
 * Short, long, and immediate instructions each have separate encoding tables.
 *
 * All [?] long instructions can be predicated. $c register number to be used
 * is in 0x2c-0x2d, 0x27-0x2b specifies condition to be checked in that register:
 *  code   $c formula		meaning for sub/cmp	meaning for set-to-res
 *  00000: never
 *  00001: (S & ~Z) ^ O		s/f <			<0
 *  00010: Z & ~S		=			0
 *  00011: S ^ (Z | O)		s/f <=			<=0
 *  00100: ~Z & ~(S ^ O)	s/f >			>0
 *  00101: ~Z			<>			<>0
 *  00110: ~(S ^ O)		s/f >=			>=0
 *  00111: ~Z | ~S		s/f ordered		not nan
 *  01000: Z & S		unordered		nan
 *  01001: S ^ O		s/f < or unordered	<0 or nan
 *  01010: Z			= or unordered		0 or nan
 *  01011: Z | (S ^ O)		s/f <= or unordered	<=0 or nan
 *  01100: ~S ^ (Z | O)		s/f > or undordered	>0 or nan
 *  01101: S | ~Z		s/f <> or unordered	not 0
 *  01110: (Z | ~S) ^ O		s/f >= or unordered	>=0
 *  01111: always
 *  10000: O			signed overflow
 *  10001: C			u >=
 *  10010: C & ~Z		u >
 *  10011: S			result <0		<0
 *  11100: ~S			result >=0		>=0
 *  11101: Z | ~C		u <=
 *  11110: ~C			u <
 *  11111: ~O			no signed overflow
 *
 * Bit 1 is yet another instruction subtype: 0 is normal instruction, 1 is
 * a control instruction [branches and stuff like that]. For all types and
 * subtypes, the major opcode field is 0x1c-0x1f. For long instructions,
 * 0x3d-0x3f is subopcode. From there on, there is little regularity.
 */

/*
 * Misc. hack alerts:
 *  - 0x2c-0x2d is read twice in addc instructions: it selects $c to use for
 *    a carry flag, and selects $c to use for conditional execution. Printing
 *    it depends on zeroing happening after decoding the instruction.
 *    Same thing also happens in mov-from-$c instruction.
 *  - If instruction has both s[] and c?[] memory operands and sets the AREG
 *    field to non-0, specified $a only applies to the first one. This depends
 *    on bits in *a and *b getting zeroed out by function printing out the first
 *    argument. For added fun, a[] behaves differently.
 *  - long instruction always need to be aligned to even word bounduary.
 *    However, I disabled checking for this case, since renouveau dumps contain
 *    shader code starting at random positions, and the check is mostly useless
 *    anyway.
 */

/*
 * Code target field
 *
 * This field represents a code address and is used for branching and the
 * likes. Target is counted in 32-bit words from the start point of code
 * segment. Low part of target is in 0/11-26, high part is in 1/14-19. Code
 * addresses max out at 0xfffffc.
 */

#define CTARG atomctarg, 0
void atomctarg APROTO {
	fprintf (out, " %s%#llx", cbr, BF(0xb, 16)<<2 | BF(0x2e, 6) << 18);
}

/*
 * Immediate field
 *
 * Used by all immediate insns [tabi table] for last argument. Full 32-bit.
 */

#define IMM atomimm, 0
void atomimm APROTO {
	fprintf (out, " %s%#llx", cyel, (BF(0x10, 6)) | (BF(0x22, 26)<<6));
}

/*
 * Misc number fields
 *
 * Used for plain numerical arguments. These include:
 *  - PM: PM event id for pmevent
 *  - BAR: barrier id for bar.sync
 *  - OFFS: an offset to add in add $a, num, $a, which is basically a LEA
 *  - SHCNT: used in shift-by-immediate insns for shift amount
 *  - HSHCNT: used in $a shift-by-immediate insn for shift amount
 */

static int pmoff[] = { 0xa, 4, 0, 0 };
static int baroff[] = { 0x15, 4, 0, 0 };
static int offoff[] = { 9, 16, 0, 0 };
static int shcntoff[] = { 0x10, 7, 0, 0 };
static int hshcntoff[] = { 0x10, 4, 0, 0 };
static int toffxoff[] = { 0x38, 4, 0, 1 };
static int toffyoff[] = { 0x34, 4, 0, 1 };
static int toffzoff[] = { 0x30, 4, 0, 1 };
#define PM atomnum, pmoff
#define BAR atomnum, baroff
#define OFFS atomnum, offoff
#define SHCNT atomnum, shcntoff
#define HSHCNT atomnum, hshcntoff
#define TOFFX atomnum, toffxoff
#define TOFFY atomnum, toffyoff
#define TOFFZ atomnum, toffzoff

/*
 * Ignored fields
 *
 * Used in cases when some bitfield doesn't do anything, but nvidia's blob
 * sets it due to stupi^H^H^H^H^Hdesign decisions.
 *  - IGNCE: $c write enable
 *  - IGNPRED: predicates, for instructions that don't use them.
 *
 * The whole point of this op is to kill spurious red when disassembling
 * ptxas output and blob shaders. Don't include all random unused fields here.
 */

static int ignce[] = { 0x26, 1 };
static int ignpred[] = { 0x27, 7 };
#define IGNCE atomign, ignce
#define IGNPRED atomign, ignpred

/*
 * Register fields
 *
 * There are four locations of register fields for $r:
 *  - DST: 2 and up, used for destination reg of most insns, but for source
 *         in store instructions. Insn seems to get ignored if this field is
 *         > number of allocated regs, so don't use that for storing 0s.
 *  - SRC: 9 and up, used as a source register in all types of insns.
 *  - SRC2: 0x10 and up, used as a source register in short and long insns.
 *  - SRC3: 0x2e and up, used as s source register in long insns.
 *
 * These fields can be unused, or used as part of opcode or modifiers if insn
 * doesn't need all available fields.
 *
 * All fields are either 6-bit [S] or 7-bit [L]. Short insns and non-mov
 * immediate insns have 6-bit fields and use the high bit as opcode/mods,
 * while mov immediate and long insns use full 7-bit fields.
 *
 * If encoded registers are 32-bit [no modifier letter] or 64-bit [D], the
 * field contains register number directly. For 16-bit halves of registers [H],
 * lowest bit selects low [0] or high [1] half of given register, and higher
 * bits contain the number of register. For 64-bit registers, the number needs
 * to be even.
 *
 * DST field seems to sometimes store $o registers instead of $r. If it can,
 * using $o instead of $r is indicated by bit 0x23 being set. LLDST means
 * either $o or $r in DST. It's unknown yet in what instructions it can be
 * used [probably all non-tex non-access-g[] longs?]
 *
 * ADST is a smaller version of DST field used for storing $a destination in
 * insns that allow one.
 *
 * COND is $c register used as input, both for predicating and other purposes,
 * like addition with carry or mov from $c.
 *
 * CDST is $c register used for output. It's set according to insn result for
 * most insns that allow it. Writing to $c in this field needs to be enabled
 * by setting bit 0x26. MCDST is an op that reflects that and is used for insns
 * that can function both with or without $c output.
 * 
 * AREG is a field storing $a register used in insn for memory addressing,
 * or as a normal source register sometimes. It's especially ugly, because
 * it's spread out across 0x1a-0x1b [low part] and 0x22 [high part], with the
 * high part assumed 0 if insn is short or immediate. Also, if two memory
 * accesses are used in the same insn, this field applies only to the first.
 *
 */

static int sdstoff[] = { 2, 6, 'r' };
static int ldstoff[] = { 2, 7, 'r' };
static int ssrcoff[] = { 9, 6, 'r' };
static int lsrcoff[] = { 9, 7, 'r' };
static int ssrc2off[] = { 0x10, 6, 'r' };
static int lsrc2off[] = { 0x10, 7, 'r' };
static int lsrc3off[] = { 0x2e, 7, 'r' };
static int odstoff[] = { 2, 7, 'o' };
static int adstoff[] = { 2, 3, 'a' };
static int condoff[] = { 0x2c, 2, 'c' };
static int c0off[] = { 0, 0, 'c' };
static int cdstoff[] = { 0x24, 2, 'c' };
static int texoff[] = { 9, 7, 't' };
static int sampoff[] = { 0x11, 4, 's' };
#define SDST atomreg, sdstoff
#define LDST atomreg, ldstoff
#define SHDST atomhreg, sdstoff
#define LHDST atomhreg, ldstoff
#define LDDST atomdreg, ldstoff
#define LQDST atomqreg, ldstoff
#define SSRC atomreg, ssrcoff
#define LSRC atomreg, lsrcoff
#define SHSRC atomhreg, ssrcoff
#define LHSRC atomhreg, lsrcoff
#define LDSRC atomdreg, lsrcoff
#define SSRC2 atomreg, ssrc2off
#define LSRC2 atomreg, lsrc2off
#define SHSRC2 atomhreg, ssrc2off
#define LHSRC2 atomhreg, lsrc2off
#define LDSRC2 atomdreg, lsrc2off
#define LSRC3 atomreg, lsrc3off
#define LHSRC3 atomhreg, lsrc3off
#define LDSRC3 atomdreg, lsrc3off
#define ODST atomreg, odstoff
#define OHDST atomhreg, odstoff
#define ADST atomreg, adstoff
#define COND atomreg, condoff
#define C0 atomreg, c0off
#define CDST atomreg, cdstoff
#define TEX atomreg, texoff
#define SAMP atomreg, sampoff

static int getareg (ull *a, ull *m, int l) {
	int r = BF(0x1a, 2);
	RCL(0x1a, 2);
	if (l) {
		r |= BF(0x22, 1)<<2;
		RCL(0x22, 1);
	}
	return r;
}
#define AREG atomareg, 0
void atomareg APROTO {
	fprintf (out, " %s$a%d", cmag, getareg(a, m, 1));
}

#define LTDST atomltdst, 0
void atomltdst APROTO {
	int base = BF(2, 7);
	int mask = BF(0x2e, 2)<<2 | BF(0x19, 2);
	int k = 0, i;
	fprintf (out, " %s{", cnorm);
	for (i = 0; i < 4; i++)
		if (mask & 1<<i)
			fprintf (out, " %s$r%d", cbl, base+k++);
		else
			fprintf (out, " %s#", cbl);
	fprintf (out, " %s}", cnorm);
}
#define STDST atomstdst, 0
void atomstdst APROTO {
	int base = BF(2, 6);
	int i;
	fprintf (out, " %s{", cnorm);
	for (i = 0; i < 4; i++)
		fprintf (out, " %s$r%d", cbl, base+i);
	fprintf (out, " %s}", cnorm);
}
#define LTSRC atomltsrc, 0
void atomltsrc APROTO {
	int base = BF(2, 7);
	int cnt = BF(0x16, 2);
	int i;
	fprintf (out, " %s{", cnorm);
	for (i = 0; i <= cnt; i++)
		fprintf (out, " %s$r%d", cbl, base+i);
	fprintf (out, " %s}", cnorm);
}
#define STSRC atomstsrc, 0
void atomstsrc APROTO {
	int base = BF(2, 6);
	int cnt = BF(0x16, 2);
	int i;
	fprintf (out, " %s{", cnorm);
	for (i = 0; i <= cnt; i++)
		fprintf (out, " %s$r%d", cbl, base+i);
	fprintf (out, " %s}", cnorm);
}


#define LLDST T(lldst)
F(lldst, 0x23, LDST, ODST)
#define LLHDST T(llhdst)
F(llhdst, 0x23, LHDST, OHDST)

#define MCDST T(mcdst)
F1(mcdst, 0x26, CDST)

/*
 * Memory fields
 *
 * Memory accesses on nv50 are generally of the form x[$a+offset]. $a register
 * is taken from AREG and applies only to first memory operand actually used
 * in a given insn. Offset is taken from a field in insn and maybe multiplied
 * by operand size before use.
 *
 * Fields for reading shared/attribute spaces:
 *  S*SHARED: s/a space, multiplied, shares field with S*SRC.
 *    Used in many short/immediate insns instead of S*SRC when 0/24 set.
 *  L*SHARED, s/a space, multiplied, shares field with L*SRC.
 *    Used in many long insns instead of L*SRC if 1/21 set.
 * High 2 bits of both of these fields are some sort of subspace specifier.
 * Known ones include:
 *  For 8-bit operands: 00 - s[] space.
 *  For 16-bit operands: 01 - s[] space for unsigned operands, 10 - s[] space
 *    for signed operands. I have no idea what's the difference.
 *  For 32-bit operands: 00 - a[] space, 11 - s[] space.
 * Low 4 or 5 bits are offset that gets multiplied by access size before use.
 *
 * Fields for reading const spaces:
 *  S*CONST: c* space, multiplied, shares field with S*SRC2.
 *    Used in many short insns instead of S*SRC2 if 0/23 set.
 *  L*CONST2: c* space, multiplied, shares field with L*SRC2.
 *    Used in many long insns instead of L*SRC2 if 1/23 set.
 *  L*CONST3: c* space, multiplied, shares field with L*SRC3.
 *    Used in many long insns instead of L*SRC3 if 1/24 set.
 *  F*CONST: c* space, multiplied, uses 15-bit offset. Seen only on special
 *    movs from c* spaces.
 * L*CONST* and F*CONST have constant space number in 1/22-25. S*CONST uses
 * only 5 bits of its 6-bit field, presumably 0/21 selects c1 or c0, but needs
 * to be checked.
 *
 * For some reason ptxas hasn't been seen emitting both S*CONST and S*SHARED
 * nor L*CONST2 and L*CONST3 in the same insn. But L*SHARED with L*CONST[23]
 * works ok.
 *
 * Fields for storing to shared space:
 *  F*SHARED: s space, multiplied. Used only in mov to s[].
 *
 * Fields for local space:
 *  LOCAL: l space, not multiplied. Used only in movs to/from l[].
 *
 * g[] space is totally different: its only addressing mode is g[$r], where
 * $r is taken from LSRC operand.
 */

// BF, offset shift, 'l', flags, const space num BF. flags: 1 supports $a, 2 supports full 3-bit $a, 4 supports autoincrement

// shared
static int ssmem[] = { 9, 4, 2, 's', 5, 0, 0 };		// done
static int shsmem[] = { 9, 4, 1, 's', 5, 0, 0 };		// done
static int sbsmem[] = { 9, 4, 0, 's', 5, 0, 0 };		// done
static int lsmem[] = { 9, 5, 2, 's', 7, 0, 0 };		// done
static int lhsmem[] = { 9, 5, 1, 's', 7, 0, 0 };		// done
static int lbsmem[] = { 9, 5, 0, 's', 7, 0, 0 };		// done
static int fsmem[] = { 9, 14, 2, 's', 7, 0, 0 };		// done
static int fhsmem[] = { 9, 15, 1, 's', 7, 0, 0 };		// done
static int fbsmem[] = { 9, 16, 0, 's', 7, 0, 0 };		// done
// attr
static int samem[] = { 9, 6, 2, 'a', 0, 0, 0 };		// TODO
static int lamem[] = { 9, 7, 2, 'a', 0, 0, 0 };		// TODO
static int famem[] = { 9, 7, 2, 'a', 3, 0, 0 };		// TODO
// prim
static int spmem[] = { 9, 6, 2, 'p', 1, 0, 0 };		// TODO
static int lpmem[] = { 9, 7, 2, 'p', 3, 0, 0 };		// TODO
// const
static int scmem[] = { 0x10, 5, 2, 'c', 1, 0x15, 1 };		// TODO
static int shcmem[] = { 0x10, 5, 1, 'c', 1, 0x15, 1 };		// TODO
static int lcmem2[] = { 0x10, 7, 2, 'c', 3, 0x36, 4 };		// TODO
static int lhcmem2[] = { 0x10, 7, 1, 'c', 3, 0x36, 4 };	// TODO
static int lcmem3[] = { 0x2e, 7, 2, 'c', 3, 0x36, 4 };		// TODO
static int lhcmem3[] = { 0x2e, 7, 1, 'c', 3, 0x36, 4 };	// TODO
static int fcmem[] = { 9, 14, 2, 'c', 7, 0x36, 4 };		// done
static int fhcmem[] = { 9, 15, 1, 'c', 7, 0x36, 4 };		// done
static int fbcmem[] = { 9, 16, 0, 'c', 7, 0x36, 4 };		// done
// local
static int lmem[] = { 9, 16, 0, 'l', 7, 0, 0 };		// done
// varying
static int svmem[] = { 0x10, 8, 2, 'v', 1, 0, 0 };		// TODO
static int lvmem[] = { 0x10, 8, 2, 'v', 3, 0, 0 };		// TODO

#define SSHARED atommem, ssmem
#define SHSHARED atommem, shsmem
#define SBSHARED atommem, sbsmem
#define LSHARED atommem, lsmem
#define LHSHARED atommem, lhsmem
#define LBSHARED atommem, lbsmem
#define SATTR atommem, samem
#define LATTR atommem, lamem
#define FATTR atommem, famem
#define SPRIM atommem, spmem
#define LPRIM atommem, lpmem
#define SCONST atommem, scmem
#define SHCONST atommem, shcmem
#define LCONST2 atommem, lcmem2
#define LHCONST2 atommem, lhcmem2
#define LCONST3 atommem, lcmem3
#define LHCONST3 atommem, lhcmem3
#define LOCAL atommem, lmem
#define FSHARED atommem, fsmem
#define FHSHARED atommem, fhsmem
#define FBSHARED atommem, fbsmem
#define FCONST atommem, fcmem
#define FHCONST atommem, fhcmem
#define FBCONST atommem, fbcmem
#define SVAR atommem, svmem
#define LVAR atommem, lvmem
void atommem APROTO {
	const int *n = v;
	fprintf (out, " %s%c", ccy, n[3]);
	if (n[6]) fprintf (out, "%lld", BF(n[5], n[6]));
	fprintf (out, "[", ccy);
	int mo = BF(n[0], n[1])<<n[2];
	int r = (n[4]&1?getareg(a, m, n[4]&2):0);
	if (n[4]&4 && BF(0x19, 1))  {
		fprintf (out, "%s$a%d%s++", cmag, r, ccy);
		if (mo&(1<<(n[1]+n[2]-1)))
			mo += (1<<16)-(1<<(n[1]+n[2]));
		fprintf (out, "%s%#x%s]", cyel, mo, ccy);
	} else {
		if (r) fprintf (out, "%s$a%d%s+", cmag, r, ccy);
		fprintf (out, "%s%#x%s]", cyel, mo, ccy);
	}
}

static int g1mem[] = { 0x10, 4 };
static int g2mem[] = { 0x17, 4 };
#define GLOBAL atomglobal, g1mem
#define GLOBAL2 atomglobal, g2mem
void atomglobal APROTO {
	const int *n = v;
	fprintf (out, " %sg%lld[%s$r%lld%s]", ccy, BF(n[0], n[1]), cbl, BF(9, 7), ccy);
}

static struct insn tabss[] = {
	{ GP, 0x01800000, 0x01800000, SPRIM },	// XXX check
	{ CP, 0x00000000, 0x00006000, N("u8"), SBSHARED },
	{ CP, 0x00002000, 0x00006000, N("u16"), SHSHARED },
	{ CP, 0x00004000, 0x00006000, N("s16"), SHSHARED },
	{ CP, 0x00006000, 0x00006000, N("b32"), SSHARED },
	{ VP|GP, 0, 0, SATTR },
	{ AP, 0, 0, OOPS },
};

static struct insn tabls[] = {
	{ GP, 0x01800000, 0x01800000, LPRIM },	// XXX check
	{ CP, 0x00000000, 0x0000c000, N("u8"), LBSHARED },
	{ CP, 0x00004000, 0x0000c000, N("u16"), LHSHARED },
	{ CP, 0x00008000, 0x0000c000, N("s16"), LHSHARED },
	{ CP, 0x0000c000, 0x0000c000, N("b32"), LSHARED },
	{ VP|GP, 0, 0, LATTR },
	{ AP, 0, 0, OOPS },
};

static struct insn tabaddop[] = {
	{ AP, 0x00000000, 0x10400000, N("add") },
	{ AP, 0x00400000, 0x10400000, N("sub") },
	{ AP, 0x10000000, 0x10400000, N("subr") },
	{ AP, 0x10400000, 0x10400000, N("addc") },
};

/*
 * Short instructions
 */

// various stuff available for filling the misc bits.

F1(sm1sat, 8, N("sat"))
F(sm1us16, 8, N("u16"), N("s16"))
F(sm1us32, 8, N("u32"), N("s32"))
F1(sm1high, 8, N("high"))
F(sm1tex, 8, N("all"), N("live"))

F1(sm2neg, 0xf, N("neg"))
F1(sm2abs, 0xf, N("abs"))
F(sm2us16, 0xf, N("u16"), N("s16"))
F(sm2us24, 0xf, N("u24"), N("s24"))

F1(sm3neg, 0x16, N("neg"))
F1(sm3not, 0x16, N("not"))

F1(splease, 0x11, N("please"))

static struct insn tabssh[] = {
	{ AP, 0x00000000, 0x01000000, SHSRC },
	{ AP, 0x01000000, 0x01000000, T(ss) },
	{ AP, 0, 0, OOPS }
};

static struct insn tabssw[] = {
	{ AP, 0x00000000, 0x01000000, SSRC },
	{ AP, 0x01000000, 0x01000000, T(ss) },
	{ AP, 0, 0, OOPS }
};

static struct insn tabsch[] = {
	{ AP, 0x00800000, 0x01800000, SHCONST },
	{ AP, 0x00000000, 0x00000000, SHSRC2 },
	{ AP, 0, 0, OOPS }
};

static struct insn tabscw[] = {
	{ AP, 0x00800000, 0x01800000, SCONST },
	{ AP, 0x00000000, 0x00000000, SSRC2 },
	{ AP, 0, 0, OOPS }
};

static struct insn tabaddc0[] = {
	{ AP, 0x10400000, 0x10400000, C0 },
	{ AP, 0, 0 }
};

static struct insn tabas[] = {
	{ AP, 0x00000000, 0x00008000, T(sm1sat), N("b16"), SHDST, T(ssh), T(sch) },
	{ AP, 0x00008000, 0x00008000, T(sm1sat), N("b32"), SDST, T(ssw), T(scw) },
	{ AP, 0, 0, OOPS }
};

static struct insn tabs[] = {
	{ AP, 0x10000000, 0xf0008002, T(splease), N("mov"), N("b16"), SHDST, T(ssh) },
	{ AP, 0x10008000, 0xf0008002, T(splease), N("mov"), N("b32"), SDST, T(ssw) },

	{ AP, 0x20000000, 0xe0008002, T(addop), T(sm1sat), N("b16"), SHDST, T(ssh), T(sch), T(addc0) },
	{ AP, 0x20008000, 0xe0008002, T(addop), T(sm1sat), N("b32"), SDST, T(ssw), T(scw), T(addc0) },

	{ AP, 0x40000000, 0xf0400002, N("mul"), SDST, T(sm2us16), T(ssh), T(sm1us16), T(sch) },
	{ AP, 0x40400000, 0xf0400002, N("mul"), SDST, T(sm1high), T(sm2us24), T(ssw), T(scw) },

	{ AP, 0x50000000, 0xf0008002, N("sad"), SDST, T(sm1us16), SHSRC, SHSRC2, SDST },
	{ AP, 0x50008000, 0xf0008002, N("sad"), SDST, T(sm1us32), SSRC, SSRC2, SDST },

	{ AP, 0x60000000, 0xe0008102, T(addop), SDST, N("mul"), N("u16"), T(ssh), T(sch), SDST, T(addc0) },
	{ AP, 0x60000100, 0xe0008102, T(addop), SDST, N("mul"), N("s16"), T(ssh), T(sch), SDST, T(addc0) },
	{ AP, 0x60008000, 0xe0008102, T(addop), N("sat"), SDST, N("mul"), N("s16"), T(ssh), T(sch), SDST, T(addc0) },
	{ AP, 0x60008100, 0xe0008102, T(addop), SDST, N("mul"), N("u24"), T(ssw), T(scw), SDST, T(addc0) },

	// desc VVV
	{ FP, 0x80000000, 0xf3000102, N("interp"), SDST, SVAR },		// most likely something is wrong with this.
	{ FP, 0x81000000, 0xf3000102, N("interp"), SDST, N("cent"), SVAR },
	{ FP, 0x82000000, 0xf3000102, N("interp"), SDST, SVAR, SSRC },
	{ FP, 0x83000000, 0xf3000102, N("interp"), SDST, N("cent"), SVAR, SSRC },
	{ FP, 0x80000100, 0xf3000102, N("interp"), SDST, N("flat"), SVAR },

	{ AP, 0x90000000, 0xf0000002, N("rcp f32"), SDST, T(sm3neg), T(sm2abs), SSRC },

	{ AP, 0xb0000000, 0xf0000002, N("add"), T(sm1sat), N("f32"), SDST, T(sm2neg), T(ssw), T(sm3neg), T(scw) },

	{ AP, 0xc0000000, 0xf0000002, N("mul f32"), SDST, T(sm2neg), T(ssw), T(sm3neg), T(scw) },

	{ AP, 0xe0000000, 0xf0000002, N("add"), T(sm1sat), N("f32"), SDST, T(sm2neg), N("mul"), T(ssw), T(scw), T(sm3neg), SDST },

	{ AP, 0xf0000000, 0xf1000002, N("texauto"), T(sm1tex), STDST, TEX, SAMP, STSRC },
	{ AP, 0xf1000000, 0xf1000002, N("texfetch"), T(sm1tex), STDST, TEX, SAMP, STSRC},
	// desc ^^^

	{ AP, 0, 2, OOPS, SDST, T(ssw), T(scw) },
	{ AP, 0, 0, OOPS }
};

/*
 * Immediate instructions
 */

static struct insn tabi[] = {
	{ AP, 0x10000000, 0xf0008000, N("mov"), N("b16"), LHDST, IMM },
	{ AP, 0x10008000, 0xf0008000, N("mov"), N("b32"), LDST, IMM },	// yes. LDST. special case.

	{ AP, 0x20000000, 0xe0008000, T(addop), T(sm1sat), N("b16"), SHDST, T(ssh), IMM, T(addc0) },
	{ AP, 0x20008000, 0xe0008000, T(addop), T(sm1sat), N("b32"), SDST, T(ssw), IMM, T(addc0) },

	{ AP, 0x40000000, 0xf0400000, N("mul"), SDST, T(sm2us16), T(ssh), T(sm1us16), IMM },
	{ AP, 0x40400000, 0xf0400000, N("mul"), SDST, T(sm1high), T(sm2us24), T(ssw), IMM },

	{ AP, 0x60000000, 0xe0008100, T(addop), SDST, N("mul"), N("u16"), T(ssh), IMM, SDST, T(addc0) },
	{ AP, 0x60000100, 0xe0008100, T(addop), SDST, N("mul"), N("s16"), T(ssh), IMM, SDST, T(addc0) },
	{ AP, 0x60008000, 0xe0008100, T(addop), N("sat"), SDST, N("mul"), N("s16"), T(ssh), IMM, SDST, T(addc0) },
	{ AP, 0x60008100, 0xe0008100, T(addop), SDST, N("mul"), N("u24"), T(ssw), IMM, SDST, T(addc0) },

	// desc VVV
	{ AP, 0xb0000000, 0xf0000000, N("add"), T(sm1sat), N("f32"), SDST, T(sm2neg), T(ssw), T(sm3neg), IMM },

	{ AP, 0xc0000000, 0xf0000000, N("mul f32"), SDST, T(sm2neg), T(ssw), T(sm3neg), IMM },
	// desc ^^^

	{ AP, 0xd0000000, 0xf0008100, N("and"), N("b32"), SDST, T(sm3not), T(ssw), IMM },
	{ AP, 0xd0000100, 0xf0008100, N("or"), N("b32"), SDST, T(sm3not), T(ssw), IMM },
	{ AP, 0xd0008000, 0xf0008100, N("xor"), N("b32"), SDST, T(sm3not), T(ssw), IMM },
	{ AP, 0xd0008100, 0xf0008100, N("mov2"), N("b32"), SDST, T(sm3not), T(ssw), IMM },

	// desc VVV
	{ AP, 0xe0000000, 0xf0000000, N("add"), T(sm1sat), N("f32"), SDST, T(sm2neg), N("mul"), T(ssw), IMM, T(sm3neg), SDST },
	// desc ^^^

	{ AP, 0, 0, OOPS, SDST, T(ssw), IMM },
};

/*
 * Long instructions
 */

// A few helper tables for usual operand types.

F(lsh, 0x35, LHSRC, T(ls))
F(lsw, 0x35, LSRC, T(ls))

static struct insn tablc2h[] = {
	{ AP, 0x00800000, 0x01800000, LHCONST2 },
	{ AP, 0x00000000, 0x00000000, LHSRC2 },
	{ AP, 0, 0, OOPS }
};

static struct insn tablc2w[] = {
	{ AP, 0x00800000, 0x01800000, LCONST2 },
	{ AP, 0x00000000, 0x00000000, LSRC2 },
};

static struct insn tablc3h[] = {
	{ AP, 0x01000000, 0x01800000, LHCONST3 },
	{ AP, 0x00000000, 0x00000000, LHSRC3 },
};

static struct insn tablc3w[] = {
	{ AP, 0x01000000, 0x01800000, LCONST3 },
	{ AP, 0x00000000, 0x00000000, LSRC3 },
};

F(shcnt, 0x34, T(lc2w), SHCNT)
F(hshcnt, 0x34, T(lc2h), SHCNT)

// various stuff available for filling the misc bits.

// stolen from SRC3
F(s30us16, 0x2e, N("u16"), N("s16"))
F1(s30high, 0x2e, N("high"))
F(s31us16, 0x2f, N("u16"), N("s16"))
F(s31us24, 0x2f, N("u24"), N("s24"))
F1(s32not, 0x30, N("not"))
F1(s33not, 0x31, N("not"))
F1(s35sat, 0x33, N("sat"))
F1(s35abs, 0x33, N("abs"))
F1(s36abs, 0x34, N("abs"))
// the actual misc field
F1(m1neg, 0x3a, N("neg"))
F1(m2neg, 0x3b, N("neg"))
F1(m2sat, 0x3b, N("sat"))
F(lm2us16, 0x3b, N("u16"), N("s16"))
F(lm2us32, 0x3b, N("u32"), N("s32"))
// stolen from opcode field.
F1(o0neg, 0x3d, N("neg"))
F1(o0sat, 0x3d, N("sat"))
F1(lplease, 0x39, N("please"))

static struct insn tablfm1[] = {
	{ AP, 0, 0, T(m1neg), T(s36abs) }
};

static struct insn tablfm2[] = {
	{ AP, 0, 0, T(m2neg), T(s35abs) }
};

static struct insn tabcvtmod[] = {
	{ AP, 0, 0, T(o0neg), T(s36abs) },
};

// for g[] and l[] insns.
static struct insn tabldstm[] = { // we seriously need to unify these, if possible. I wonder if atomics will work with smaller sizes.
	{ AP, 0x0000000000000000ull, 0x00e0000000000000ull, N("u8") },
	{ AP, 0x0020000000000000ull, 0x00e0000000000000ull, N("s8") },
	{ AP, 0x0040000000000000ull, 0x00e0000000000000ull, N("u16") },
	{ AP, 0x0060000000000000ull, 0x00e0000000000000ull, N("s16") },
	{ AP, 0x0080000000000000ull, 0x00e0000000000000ull, N("b64") },
	{ AP, 0x00a0000000000000ull, 0x00e0000000000000ull, N("b128") },
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, N("b32") },
	{ AP, 0, 0, OOPS }
};

static struct insn tabraadd[] = {
	{ AP, 0x0080000000000000ull, 0x00e0000000000000ull, N("u64") },
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, N("u32") },
	{ AP, 0x00e0000000000000ull, 0x00e0000000000000ull, N("s32") },
	{ AP, 0, 0, OOPS }
};

static struct insn tabrab[] = {
	{ AP, 0x0080000000000000ull, 0x00e0000000000000ull, N("b64") },
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, N("b32") },
	{ AP, 0, 0, OOPS }
};

static struct insn tabramm[] = {
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, N("u32") },
	{ AP, 0x00e0000000000000ull, 0x00e0000000000000ull, N("s32") },
	{ AP, 0, 0, OOPS }
};

static struct insn tabrab32[] = {
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, N("b32") },
	{ AP, 0, 0, OOPS }
};

static struct insn tabrau32[] = {
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, N("u32") },
	{ AP, 0, 0, OOPS }
};

static struct insn tabldsto[] = {	// hack alert: reads the bitfield second time.
	{ AP, 0x0000000000000000ull, 0x00e0000000000000ull, LDST },
	{ AP, 0x0020000000000000ull, 0x00e0000000000000ull, LDST },
	{ AP, 0x0040000000000000ull, 0x00e0000000000000ull, LDST },
	{ AP, 0x0060000000000000ull, 0x00e0000000000000ull, LDST },
	{ AP, 0x0080000000000000ull, 0x00e0000000000000ull, LDDST },
	{ AP, 0x00a0000000000000ull, 0x00e0000000000000ull, LQDST },
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, LDST },
	{ AP, 0, 0, LDST },
};

static struct insn tabldsts2[] = {
	{ AP, 0x0080000000000000ull, 0x00e0000000000000ull, LDSRC2 },
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, LSRC2 },
	{ AP, 0x00e0000000000000ull, 0x00e0000000000000ull, LSRC2 },
	{ AP, 0, 0, OOPS },
};

static struct insn tabldsts3[] = {
	{ AP, 0x0080000000000000ull, 0x00e0000000000000ull, LDSRC3 },
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, LSRC3 },
	{ AP, 0x00e0000000000000ull, 0x00e0000000000000ull, LSRC3 },
	{ AP, 0, 0, OOPS },
};

static struct insn tabmldsts3[] = {
	{ AP, 0x0000000800000000ull, 0x0000000800000000ull, T(ldsts3) },
	{ AP, 0, 0 },
};

static struct insn tabredm[] = {
	{ AP, 0x0000000000000000ull, 0x0000003c00000000ull, N("add"), T(raadd), },
	{ AP, 0x0000001000000000ull, 0x0000003c00000000ull, N("inc"), T(rau32), },
	{ AP, 0x0000001400000000ull, 0x0000003c00000000ull, N("dec"), T(rau32), },
	{ AP, 0x0000001800000000ull, 0x0000003c00000000ull, N("max"), T(ramm), },
	{ AP, 0x0000001c00000000ull, 0x0000003c00000000ull, N("min"), T(ramm), },
	{ AP, 0x0000002800000000ull, 0x0000003c00000000ull, N("and"), T(rab32), },
	{ AP, 0x0000002c00000000ull, 0x0000003c00000000ull, N("or"), T(rab32), },
	{ AP, 0x0000003000000000ull, 0x0000003c00000000ull, N("xor"), T(rab32), },
	{ AP, 0, 0, OOPS },
};

static struct insn tabatomm[] = {
	{ AP, 0x0000000400000000ull, 0x0000003c00000000ull, N("exch"), T(rab), },
	{ AP, 0x0000000800000000ull, 0x0000003c00000000ull, N("cas"), T(rab), },
	{ AP, 0, 0, N("ld"), T(redm) },
};

// rounding modes
static struct insn tabcvtrnd[] = {
	{ AP, 0x0000000000000000ull, 0x0006000000000000ull, N("rn") },
	{ AP, 0x0002000000000000ull, 0x0006000000000000ull, N("rm") },
	{ AP, 0x0004000000000000ull, 0x0006000000000000ull, N("rp") },
	{ AP, 0x0006000000000000ull, 0x0006000000000000ull, N("rz") },
	{ AP, 0, 0, OOPS }
};

static struct insn tabcvtrint[] = {
	{ AP, 0x0000000000000000ull, 0x0006000000000000ull, N("rni") },
	{ AP, 0x0002000000000000ull, 0x0006000000000000ull, N("rmi") },
	{ AP, 0x0004000000000000ull, 0x0006000000000000ull, N("rpi") },
	{ AP, 0x0006000000000000ull, 0x0006000000000000ull, N("rzi") },
	{ AP, 0, 0, OOPS }
};

static struct insn tabaf64r[] = {
	{ AP, 0x00000000, 0x00030000, N("rn") },
	{ AP, 0x00010000, 0x00030000, N("rm") },
	{ AP, 0x00020000, 0x00030000, N("rp") },
	{ AP, 0x00030000, 0x00030000, N("rz") },
};

static struct insn tabaf32r[] = {
	{ AP, 0x00000000, 0x00030000, N("rn") },
	{ AP, 0x00030000, 0x00030000, N("rz") },
	{ AP, 0, 0, OOPS }
};

static struct insn tabmf32r[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, N("rn") },
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, N("rz") },
	{ AP, 0, 0, OOPS }
};

static struct insn tabmad64r[] = {
	{ AP, 0x0000000000000000ull, 0x00c0000000000000ull, N("rn") },
	{ AP, 0x0040000000000000ull, 0x00c0000000000000ull, N("rm") },
	{ AP, 0x0080000000000000ull, 0x00c0000000000000ull, N("rp") },
	{ AP, 0x00c0000000000000ull, 0x00c0000000000000ull, N("rz") },
	{ AP, 0, 0, OOPS }
};

// ops for set
static struct insn tabseti[] = {
	{ AP, 0x0000000000000000ull, 0x0001c00000000000ull, N("never") },
	{ AP, 0x0000400000000000ull, 0x0001c00000000000ull, N("l") },
	{ AP, 0x0000800000000000ull, 0x0001c00000000000ull, N("e") },
	{ AP, 0x0000c00000000000ull, 0x0001c00000000000ull, N("le") },
	{ AP, 0x0001000000000000ull, 0x0001c00000000000ull, N("g") },
	{ AP, 0x0001400000000000ull, 0x0001c00000000000ull, N("lg") },
	{ AP, 0x0001800000000000ull, 0x0001c00000000000ull, N("ge") },
	{ AP, 0x0001c00000000000ull, 0x0001c00000000000ull, N("always") },
	{ AP, 0, 0, OOPS }
};

static struct insn tabsetf[] = {
	{ AP, 0x0000000000000000ull, 0x0003c00000000000ull, N("never") },
	{ AP, 0x0000400000000000ull, 0x0003c00000000000ull, N("l") },
	{ AP, 0x0000800000000000ull, 0x0003c00000000000ull, N("e") },
	{ AP, 0x0000c00000000000ull, 0x0003c00000000000ull, N("le") },
	{ AP, 0x0001000000000000ull, 0x0003c00000000000ull, N("g") },
	{ AP, 0x0001400000000000ull, 0x0003c00000000000ull, N("lg") },
	{ AP, 0x0001800000000000ull, 0x0003c00000000000ull, N("ge") },
	{ AP, 0x0001c00000000000ull, 0x0003c00000000000ull, N("lge") },
	{ AP, 0x0002000000000000ull, 0x0003c00000000000ull, N("u") },
	{ AP, 0x0002400000000000ull, 0x0003c00000000000ull, N("lu") },
	{ AP, 0x0002800000000000ull, 0x0003c00000000000ull, N("eu") },
	{ AP, 0x0002c00000000000ull, 0x0003c00000000000ull, N("leu") },
	{ AP, 0x0003000000000000ull, 0x0003c00000000000ull, N("gu") },
	{ AP, 0x0003400000000000ull, 0x0003c00000000000ull, N("lgu") },
	{ AP, 0x0003800000000000ull, 0x0003c00000000000ull, N("geu") },
	{ AP, 0x0003c00000000000ull, 0x0003c00000000000ull, N("always") },
	{ AP, 0, 0, OOPS }
};

// for cvt
static struct insn tabcvtiisrc[] ={
	{ AP, 0x0000000000000000ull, 0x0001c00000000000ull, N("u16"), T(lsh) },
	{ AP, 0x0000400000000000ull, 0x0001c00000000000ull, N("u32"), T(lsw) },
	{ AP, 0x0000800000000000ull, 0x0001c00000000000ull, N("u8"), T(lsh) },
	{ AP, 0x0000c00000000000ull, 0x0001c00000000000ull, N("u8"), LSRC },	// what about mem?
	{ AP, 0x0001000000000000ull, 0x0001c00000000000ull, N("s16"), T(lsh) },
	{ AP, 0x0001400000000000ull, 0x0001c00000000000ull, N("s32"), T(lsw) },
	{ AP, 0x0001800000000000ull, 0x0001c00000000000ull, N("s8"), T(lsh) },
	{ AP, 0x0001c00000000000ull, 0x0001c00000000000ull, N("s8"), LSRC },	// what about mem?
	{ AP, 0, 0, OOPS }
};


// for tex
F1(dtex, 0x23, N("deriv")) // suspected to enable implicit derivatives on non-FPs.
F(ltex, 0x22, N("all"), N("live"))

static struct insn tabtexf[] = {
	{ AP, 0, 0, T(ltex), T(dtex) },
};

// for mov
static struct insn tablane[] = {
	{ AP, 0x0000000000000000ull, 0x0003c00000000000ull, N("lnone") },
	{ AP, 0x0000400000000000ull, 0x0003c00000000000ull, N("l0") },
	{ AP, 0x0000800000000000ull, 0x0003c00000000000ull, N("l1") },
	{ AP, 0x0000c00000000000ull, 0x0003c00000000000ull, N("l01") },
	{ AP, 0x0001000000000000ull, 0x0003c00000000000ull, N("l2") },
	{ AP, 0x0001400000000000ull, 0x0003c00000000000ull, N("l02") },
	{ AP, 0x0001800000000000ull, 0x0003c00000000000ull, N("l12") },
	{ AP, 0x0001c00000000000ull, 0x0003c00000000000ull, N("l012") },
	{ AP, 0x0002000000000000ull, 0x0003c00000000000ull, N("l3") },
	{ AP, 0x0002400000000000ull, 0x0003c00000000000ull, N("l03") },
	{ AP, 0x0002800000000000ull, 0x0003c00000000000ull, N("l13") },
	{ AP, 0x0002c00000000000ull, 0x0003c00000000000ull, N("l013") },
	{ AP, 0x0003000000000000ull, 0x0003c00000000000ull, N("l23") },
	{ AP, 0x0003400000000000ull, 0x0003c00000000000ull, N("l023") },
	{ AP, 0x0003800000000000ull, 0x0003c00000000000ull, N("l123") },
	{ AP, 0x0003c00000000000ull, 0x0003c00000000000ull },
};

// for mov from c[]
static struct insn tabfcon[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, N("u8"), FBCONST },
	{ AP, 0x0000400000000000ull, 0x0000c00000000000ull, N("u16"), FHCONST },
	{ AP, 0x0000800000000000ull, 0x0000c00000000000ull, N("s16"), FHCONST },
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, N("b32"), FCONST },
	{ AP, 0, 0, OOPS }
};

// for quadop
static struct insn tabqs1[] = {
	{ AP, 0x00000000, 0x00070000, N("l0") },
	{ AP, 0x00010000, 0x00070000, N("l1") },
	{ AP, 0x00020000, 0x00070000, N("l2") },
	{ AP, 0x00030000, 0x00070000, N("l3") },
	{ AP, 0x00040000, 0x00050000, N("dx") },
	{ AP, 0x00050000, 0x00050000, N("dy") },
	{ AP, 0, 0 },
};

static struct insn tabqop0[] = {
	{ AP, 0x0000000000000000ull, 0x0c00000000000000ull, N("add") },
	{ AP, 0x0400000000000000ull, 0x0c00000000000000ull, N("subr") },
	{ AP, 0x0800000000000000ull, 0x0c00000000000000ull, N("sub") },
	{ AP, 0x0c00000000000000ull, 0x0c00000000000000ull, N("mov2") },
};

static struct insn tabqop1[] = {
	{ AP, 0x0000000000000000ull, 0x0300000000000000ull, N("add") },
	{ AP, 0x0100000000000000ull, 0x0300000000000000ull, N("subr") },
	{ AP, 0x0200000000000000ull, 0x0300000000000000ull, N("sub") },
	{ AP, 0x0300000000000000ull, 0x0300000000000000ull, N("mov2") },
};

static struct insn tabqop2[] = {
	{ AP, 0x0000000000000000ull, 0x00c0000000000000ull, N("add") },
	{ AP, 0x0040000000000000ull, 0x00c0000000000000ull, N("subr") },
	{ AP, 0x0080000000000000ull, 0x00c0000000000000ull, N("sub") },
	{ AP, 0x00c0000000000000ull, 0x00c0000000000000ull, N("mov2") },
};

static struct insn tabqop3[] = {
	{ AP, 0x00000000, 0x00300000, N("add") },
	{ AP, 0x00100000, 0x00300000, N("subr") },
	{ AP, 0x00200000, 0x00300000, N("sub") },
	{ AP, 0x00300000, 0x00300000, N("mov2") },
};

// for mov from sreg
static struct insn tabsreg[] = {
	{ AP, 0x0000000000000000ull, 0x0003c00000000000ull, N("physid") },
	{ AP, 0x0000400000000000ull, 0x0003c00000000000ull, N("clock") },
	{ AP, 0x0000800000000000ull, 0x0003c00000000000ull, U("2") },
	{ AP, 0x0000c00000000000ull, 0x0003c00000000000ull, U("3") },
	{ AP, 0x0001000000000000ull, 0x0003c00000000000ull, N("pm0") },
	{ AP, 0x0001400000000000ull, 0x0003c00000000000ull, N("pm1") },
	{ AP, 0x0001800000000000ull, 0x0003c00000000000ull, N("pm2") },
	{ AP, 0x0001c00000000000ull, 0x0003c00000000000ull, N("pm3") },
	{ AP, 0x0002000000000000ull, 0x0003c00000000000ull, N("sampleid") }, // NVA3+
};

static struct insn tablogop[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, N("and") },
	{ AP, 0x0000400000000000ull, 0x0000c00000000000ull, N("or") },
	{ AP, 0x0000800000000000ull, 0x0000c00000000000ull, N("xor") },
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, N("mov2") },
};

static struct insn tabaddcond[] = {
	{ AP, 0x10400000, 0x10400000, COND },
	{ AP, 0, 0 }
};

static struct insn tabaddop2[] = {
	{ AP, 0x0000000000000000ull, 0x0c00000000000000ull, N("add") },
	{ AP, 0x0400000000000000ull, 0x0c00000000000000ull, N("sub") },
	{ AP, 0x0800000000000000ull, 0x0c00000000000000ull, N("subr") },
	{ AP, 0x0c00000000000000ull, 0x0c00000000000000ull, N("addc") },
};

static struct insn tabaddcond2[] = {
	{ AP, 0x0c00000000000000ull, 0x0c00000000000000ull, COND },
	{ AP, 0, 0 }
};

F(sstreg, 0x35, LHSRC3, LSRC3)
F1(unlock, 0x37, N("unlock"))
F(csldreg, 0x3a, LLHDST, LLDST)

static struct insn tabl[] = {
	// 0
	// desc VVV
	{ VP|GP, 0x0420000000000000ull, 0xe4200000f0000000ull,
		T(lane), N("ld"), N("b32"), LLDST, FATTR },
	// desc ^^^
	{ AP, 0x2000000000000000ull, 0xe0000000f0000000ull,
		N("mov"), LDST, COND },
	{ AP, 0x4000000000000000ull, 0xe0000000f0000000ull,
		N("mov"), LDST, AREG },
	{ AP, 0x6000000000000000ull, 0xe0000000f0000000ull,
		N("mov"), LDST, T(sreg) },

	{ AP, 0xa000000000000000ull, 0xe0000000f0000000ull,
		N("mov"), CDST, LSRC, IGNCE },
	{ AP, 0xc000000000000000ull, 0xe0000000f0000000ull,
		N("shl"), ADST, T(lsw), HSHCNT },
	
	// desc VVV
	{ CP, 0xe040000000000000ull, 0xe0400000f0000000ull,
		N("st"), N("b8"), FBSHARED, T(sstreg) },
	{ CP, 0xe000000000000000ull, 0xe4400000f0000000ull,
		N("st"), N("b16"), FHSHARED, T(sstreg) },
	{ CP, 0xe400000000000000ull, 0xe4400000f0000000ull,
		N("st"), T(unlock), N("b32"), FSHARED, T(sstreg) },
	// desc ^^^

	// 1
	{ AP, 0x0000000010000000ull, 0xe4000000f0000000ull,
		T(lane), T(lplease), N("mov"), N("b16"), LLHDST, T(lsh) },
	{ AP, 0x0400000010000000ull, 0xe4000000f0000000ull,
		T(lane), T(lplease), N("mov"), N("b32"), LLDST, T(lsw) },

	// desc VVV
	{ AP, 0x2000000010000000ull, 0xe0000000f0000000ull,
		N("ld"), T(csldreg), T(fcon) },

	{ CP, 0x4000000010000000ull, 0xe000c000f0000000ull,	// sm_11. ptxas inexplicably starts using
		N("ld"), T(csldreg), N("u8"), FBSHARED },	// these forms instead of the other one
	{ CP, 0x4000400010000000ull, 0xe000c000f0000000ull,	// on >=sm_11.
		N("ld"), T(csldreg), N("u16"), FHSHARED },
	{ CP, 0x4000800010000000ull, 0xe000c000f0000000ull,
		N("ld"), T(csldreg), N("s16"), FHSHARED },
	{ CP, 0x4000c00010000000ull, 0xe080c000f0000000ull,
		N("ld"), T(csldreg), N("b32"), FSHARED },
	{ CP, 0x4080c04010000000ull, 0xe080c040f0000000ull,
		N("ld"), N("lock"), CDST, T(csldreg), N("b32"), FSHARED },

	{ AP, 0x6000000010000200ull, 0xe0000000f0000600ull,
		N("vote any"), CDST, IGNCE },	// sm_12
	{ AP, 0x6000000010000400ull, 0xe0000000f0000600ull,
		N("vote all"), CDST, IGNCE },	// sm_12
	// desc ^^^

	// 2 and 3
	{ AP, 0x0000000020000000ull, 0xe4000000e0000000ull, T(addop), N("b16"), T(m2sat), MCDST, LLHDST, T(lsh), T(lc3h), T(addcond) },
	{ AP, 0x0400000020000000ull, 0xe4000000e0000000ull, T(addop), N("b32"), T(m2sat), MCDST, LLDST, T(lsw), T(lc3w), T(addcond) },

	{ AP, 0x6000000030000000ull, 0xe4000000f0000000ull,
		N("set"), MCDST, LLHDST, T(seti), T(lm2us16), T(lsh), T(lc2h) },
	{ AP, 0x6400000030000000ull, 0xe4000000f0000000ull,
		N("set"), MCDST, LLDST, T(seti), T(lm2us32), T(lsw), T(lc2w) },

	{ AP, 0x8000000030000000ull, 0xe4000000f0000000ull,
		N("max"), T(lm2us16), MCDST, LLHDST, T(lsh), T(lc2h) },
	{ AP, 0x8400000030000000ull, 0xe4000000f0000000ull,
		N("max"), T(lm2us32), MCDST, LLDST, T(lsw), T(lc2w) },

	{ AP, 0xa000000030000000ull, 0xe4000000f0000000ull,
		N("min"), T(lm2us16), MCDST, LLHDST, T(lsh), T(lc2h) },
	{ AP, 0xa400000030000000ull, 0xe4000000f0000000ull,
		N("min"), T(lm2us32), MCDST, LLDST, T(lsw), T(lc2w) },

	{ AP, 0xc000000030000000ull, 0xe4000000f0000000ull,
		N("shl"), N("b16"), MCDST, LLHDST, T(lsh), T(hshcnt) },
	{ AP, 0xc400000030000000ull, 0xe4000000f0000000ull,
		N("shl"), N("b32"), MCDST, LLDST, T(lsw), T(shcnt) },

	{ AP, 0xe000000030000000ull, 0xe4000000f0000000ull,
		N("shr"), T(lm2us16), MCDST, LLHDST, T(lsh), T(hshcnt) },
	{ AP, 0xe400000030000000ull, 0xe4000000f0000000ull,
		N("shr"), T(lm2us32), MCDST, LLDST, T(lsw), T(shcnt) },

	// 4
	{ AP, 0x0000000040000000ull, 0xe0010000f0000000ull,
		N("mul"), MCDST, LLDST, T(s31us16), T(lsh), T(s30us16), T(lc2h) },
	{ AP, 0x0001000040000000ull, 0xe0010000f0000000ull,
		N("mul"), MCDST, LLDST, T(s30high), T(s31us24), T(lsw), T(lc2w) },

	// 5
	{ AP, 0x0000000050000000ull, 0xe4000000f0000000ull,
		N("sad"), MCDST, LLDST, T(lm2us16), T(lsh), T(lc2h), T(lc3w) },
	{ AP, 0x0400000050000000ull, 0xe4000000f0000000ull,
		N("sad"), MCDST, LLDST, T(lm2us32), T(lsw), T(lc2w), T(lc3w) },

	// 6 and 7
	{ AP, 0x0000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), MCDST, LLDST, N("mul"), N("u16"), T(lsh), T(lc2h), T(lc3w), T(addcond2) },
	{ AP, 0x2000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), MCDST, LLDST, N("mul"), N("s16"), T(lsh), T(lc2h), T(lc3w), T(addcond2) },
	{ AP, 0x4000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), N("sat"), MCDST, LLDST, N("mul"), N("s16"), T(lsh), T(lc2h), T(lc3w), T(addcond2) },
	{ AP, 0x6000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), MCDST, LLDST, N("mul"), N("u24"), T(lsw), T(lc2w), T(lc3w), T(addcond2) },
	{ AP, 0x8000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), MCDST, LLDST, N("mul"), N("s24"), T(lsw), T(lc2w), T(lc3w), T(addcond2) },
	{ AP, 0xa000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), N("sat"), MCDST, LLDST, N("mul"), N("s24"), T(lsw), T(lc2w), T(lc3w), T(addcond2) },
	{ AP, 0xc000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), MCDST, LLDST, N("mul"), N("high"), N("u24"), T(lsw), T(lc2w), T(lc3w), T(addcond2) },
	{ AP, 0xe000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), MCDST, LLDST, N("mul"), N("high"), N("s24"), T(lsw), T(lc2w), T(lc3w), T(addcond2) },
	{ AP, 0x0000000070000000ull, 0xe0000000f0000000ull,
		T(addop2), N("sat"), MCDST, LLDST, N("mul"), N("high"), N("s24"), T(lsw), T(lc2w), T(lc3w), T(addcond2) },

	// desc VVV
	// 8
	{ FP, 0x0000000080000000ull, 0x00070000f0000000ull,
		N("interp"), LDST, LVAR },
	{ FP, 0x0001000080000000ull, 0x00070000f0000000ull,
		N("interp"), LDST, N("cent"), LVAR },
	{ FP, 0x0002000080000000ull, 0x00070000f0000000ull,
		N("interp"), LDST, LVAR, LSRC },
	{ FP, 0x0003000080000000ull, 0x00070000f0000000ull,
		N("interp"), LDST, N("cent"), LVAR, LSRC },
	{ FP, 0x0004000080000000ull, 0x00070000f0000000ull,
		N("interp"), LDST, N("flat"), LVAR },

	// 9
	{ AP, 0x0000000090000000ull, 0xe0000000f0000000ull,
		N("rcp f32"), LLDST, T(lfm1), LSRC },
	{ AP, 0x4000000090000000ull, 0xe0000000f0000000ull,
		N("rsqrt f32"), LLDST, T(lfm1), LSRC },
	{ AP, 0x6000000090000000ull, 0xe0000000f0000000ull,
		N("lg2 f32"), LLDST, T(lfm1), LSRC },
	{ AP, 0x8000000090000000ull, 0xe0000000f0000000ull,
		N("sin f32"), LLDST, LSRC },
	{ AP, 0xa000000090000000ull, 0xe0000000f0000000ull,
		N("cos f32"), LLDST, LSRC },
	{ AP, 0xc000000090000000ull, 0xe0000000f0000000ull,
		N("ex2 f32"), LLDST, LSRC },

	// a
	{ AP, 0xc0000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), N("f16"), MCDST, LLHDST, N("f16"), T(lsh) },
	{ AP, 0xc8000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrint), N("f16"), MCDST, LLHDST, N("f16"), T(lsh) },
	{ AP, 0xc0004000a0000000ull, 0xc4404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f16"), MCDST, LLHDST, N("f32"), T(lsw) },
	{ AP, 0xc4004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), N("f32"), MCDST, LLDST, N("f32"), T(lsw) },
	{ AP, 0xcc004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrint), N("f32"), MCDST, LLDST, N("f32"), T(lsw) },
	{ AP, 0xc4000000a0000000ull, 0xc4404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), N("f32"), MCDST, LLDST, N("f16"), T(lsh) },

	{ AP, 0xc0404000a0000000ull, 0xc4404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrnd), N("f32"), MCDST, LLDST, N("f64"), LDSRC },
	{ AP, 0xc4404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), N("f64"), MCDST, LDDST, N("f64"), LDSRC },
	{ AP, 0xcc404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("f64"), MCDST, LDDST, N("f64"), LDSRC },
	{ AP, 0xc4400000a0000000ull, 0xc4404000f0000000ull,
		N("cvt"), T(cvtmod), N("f64"), MCDST, LDDST, N("f32"), T(lsw) },

	{ AP, 0x80000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u16"), MCDST, LLHDST, N("f16"), T(lsh) },
	{ AP, 0x80004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u16"), MCDST, LLHDST, N("f32"), T(lsw) },
	{ AP, 0x88000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s16"), MCDST, LLHDST, N("f16"), T(lsh) },
	{ AP, 0x88004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s16"), MCDST, LLHDST, N("f32"), T(lsw) },
	{ AP, 0x84000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u32"), MCDST, LLDST, N("f16"), T(lsh) },
	{ AP, 0x84004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u32"), MCDST, LLDST, N("f32"), T(lsw) },
	{ AP, 0x8c000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s32"), MCDST, LLDST, N("f16"), T(lsh) },
	{ AP, 0x8c004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s32"), MCDST, LLDST, N("f32"), T(lsw) },

	{ AP, 0x80404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u32"), MCDST, LLDST, N("f64"), LDSRC },
	{ AP, 0x88404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s32"), MCDST, LLDST, N("f64"), LDSRC },
	{ AP, 0x84400000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u64"), MCDST, LDDST, N("f32"), T(lsw) },
	{ AP, 0x84404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u64"), MCDST, LDDST, N("f64"), LDSRC },
	{ AP, 0x8c400000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s64"), MCDST, LDDST, N("f32"), T(lsw) },
	{ AP, 0x8c404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s64"), MCDST, LDDST, N("f64"), LDSRC },

	{ AP, 0x40000000a0000000ull, 0xc4400000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f16"), MCDST, LLHDST, T(cvtiisrc) },
	{ AP, 0x44000000a0000000ull, 0xc4400000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f32"), MCDST, LLDST, T(cvtiisrc) },

	{ AP, 0x44400000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f64"), MCDST, LDDST, N("u32"), T(lsw) },
	{ AP, 0x44410000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f64"), MCDST, LDDST, N("s32"), T(lsw) },
	{ AP, 0x40404000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f32"), MCDST, LLDST, N("u64"), LDSRC },
	{ AP, 0x40414000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f32"), MCDST, LLDST, N("s64"), LDSRC },
	{ AP, 0x44404000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f64"), MCDST, LDDST, N("u64"), LDSRC },
	{ AP, 0x44414000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f64"), MCDST, LDDST, N("s64"), LDSRC },

	{ AP, 0x00000000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("u16"), MCDST, LLHDST, T(cvtiisrc) },
	{ AP, 0x00080000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("u8"), MCDST, LLHDST, T(cvtiisrc) },
	{ AP, 0x04000000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("u32"), MCDST, LLDST, T(cvtiisrc) },
	{ AP, 0x04080000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("u8"), MCDST, LLDST, T(cvtiisrc) },
	{ AP, 0x08000000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("s16"), MCDST, LLHDST, T(cvtiisrc) },
	{ AP, 0x08080000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("s8"), MCDST, LLHDST, T(cvtiisrc) },
	{ AP, 0x0c000000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("s32"), MCDST, LLDST, T(cvtiisrc) },
	{ AP, 0x0c080000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("s8"), MCDST, LLDST, T(cvtiisrc) },


	// b
	{ AP, 0x00000000b0000000ull, 0xc0000000f0000000ull,
		N("add"), T(o0sat), T(af32r), N("f32"), MCDST, LLDST, T(m1neg), T(lsw), T(m2neg), T(lc3w) },

	{ AP, 0x60000000b0000000ull, 0xe0000000f0000000ull,
		N("set"), MCDST, LLDST, T(setf), N("f32"), T(lfm1), T(lsw), T(lfm2), T(lc2w) },

	{ AP, 0x80000000b0000000ull, 0xe0000000f0000000ull,
		N("max"), N("f32"), MCDST, LLDST, T(lfm1), T(lsw), T(lfm2), T(lc2w) },

	{ AP, 0xa0000000b0000000ull, 0xe0000000f0000000ull,
		N("min"), N("f32"), MCDST, LLDST, T(lfm1), T(lsw), T(lfm2), T(lc2w) },

	{ AP, 0xc0000000b0000000ull, 0xe0004000f0000000ull,
		N("presin f32"), LLDST, T(lfm1), T(lsw) },
	{ AP, 0xc0004000b0000000ull, 0xe0004000f0000000ull,
		N("preex2 f32"), LLDST, T(lfm1), T(lsw) },
	/* preex2 converts float to fixed point, results:
	 * 0-0x3fffffff: 7.23 fixed-point number
	 * 0x40000000: +nan
	 * 0x40800000: +infinity
	 * flip bit 0x80000000 in any of the above for negative numbers.
	 * presin divides by pi/2, mods with 4 [or with 2*pi, pre-div], then does preex2
	 */

	// c
	{ AP, 0x00000000c0000000ull, 0xe0000000f0000000ull,
		N("mul"), T(mf32r), N("f32"), MCDST, LLDST, T(m1neg), T(lsw), T(m2neg), T(lc2w) },

	{ AP, 0x40000000c0000000ull, 0xc0000000f0000000ull,
		N("slct"), N("b32"), MCDST, LLDST, T(lsw), T(lc2w), N("f32"), T(o0neg), T(lc3w) },

	{ AP, 0x80000000c0000000ull, 0xf0000000f0000000ull,
		N("quadop f32"), T(qop0), T(qop1), T(qop2), T(qop3), MCDST, LLDST, T(qs1), LSRC, LSRC3 },
	// desc ^^^

	// d
	{ AP, 0x00000000d0000000ull, 0xe4000000f0000000ull,
		T(logop), N("b16"), MCDST, LLHDST, T(s32not), T(lsh), T(s33not), T(lc2h) },
	{ AP, 0x04000000d0000000ull, 0xe4000000f0000000ull,
		T(logop), N("b32"), MCDST, LLDST, T(s32not), T(lsw), T(s33not), T(lc2w) },

	{ AP, 0x20000000d0000000ull, 0xe0000000f0000000ull,
		N("add"), ADST, AREG, OFFS },

	// desc VVV
	{ AP, 0x40000000d0000000ull, 0xe0000000f0000000ull,
		N("ld"), T(ldstm), T(ldsto), LOCAL },
	{ AP, 0x60000000d0000000ull, 0xe0000000f0000000ull,
		N("st"), T(ldstm), LOCAL, T(ldsto) },

	{ CP, 0x80000000d0000000ull, 0xe0000000f0000000ull,
		N("ld"), T(ldstm), T(ldsto), GLOBAL },
	{ CP, 0xa0000000d0000000ull, 0xe0000000f0000000ull,
		N("st"), T(ldstm), GLOBAL, T(ldsto) },
	{ CP, 0xc0000000d0000000ull, 0xe0000000f0000000ull,
		T(redm), GLOBAL, T(ldsto) },
	{ CP, 0xe0000000d0000000ull, 0xe0000000f0000000ull,
		T(atomm), T(ldsto), GLOBAL2, T(ldsts2), T(mldsts3) },

	// e
	{ AP, 0x00000000e0000000ull, 0xc0000000f0000000ull,
		N("add"), T(o0sat), N("f32"), MCDST, LLDST, T(m1neg), N("mul"), T(lsw), T(lc2w), T(m2neg), T(lc3w) },	// multiply, round, add, round. meh.

	{ AP, 0x40000000e0000000ull, 0xe0000000f0000000ull,
		N("fma"), T(mad64r), N("f64"), MCDST, LDDST, T(m1neg), LDSRC, LDSRC2, T(m2neg), LDSRC3 },	// *fused* multiply-add, no intermediate rounding :)
	{ AP, 0x60000000e0000000ull, 0xe0000000f0000000ull,
		N("add"), T(af64r), N("f64"), MCDST, LDDST, T(m1neg), LDSRC, T(m2neg), LDSRC3 },
	{ AP, 0x80000000e0000000ull, 0xe0000000f0000000ull,
		N("mul"), T(cvtrnd), N("f64"), MCDST, LDDST, T(m1neg), LDSRC, LDSRC2 },
	{ AP, 0xa0000000e0000000ull, 0xe0000000f0000000ull,
		N("min"), N("f64"), MCDST, LDDST, T(lfm1), LDSRC, T(lfm2), LDSRC2 },
	{ AP, 0xc0000000e0000000ull, 0xe0000000f0000000ull,
		N("max"), N("f64"), MCDST, LDDST, T(lfm1), LDSRC, T(lfm2), LDSRC2 },
	{ AP, 0xe0000000e0000000ull, 0xe0000000f0000000ull,
		N("set"), MCDST, LLDST, T(setf), N("f64"), T(lfm1), LDSRC, T(lfm2), LDSRC2 },

	// f
	{ AP, 0x00000000f0000000ull, 0xf0000000f9000000ull, // order of inputs: x, y, z, index, dref, bias/lod. index is integer, others float.
		N("texauto"), T(texf), LTDST, TEX, SAMP, LTSRC, TOFFX, TOFFY, TOFFZ },
	{ AP, 0x00000000f8000000ull, 0xf0000000f9000000ull,
		N("texauto cube"), T(texf), LTDST, TEX, SAMP, LTSRC },

	{ AP, 0x00000000f1000000ull, 0xf0000000f1000000ull, // takes integer inputs.
		N("texfetch"), T(texf), LTDST, TEX, SAMP, LTSRC, TOFFX, TOFFY, TOFFZ },

	{ AP, 0x20000000f0000000ull, 0xf0000000f8000000ull, // bias needs to be same for everything, or else.
		N("texbias"), T(texf), LTDST, TEX, SAMP, LTSRC, TOFFX, TOFFY, TOFFZ },
	{ AP, 0x20000000f8000000ull, 0xf0000000f8000000ull,
		N("texbias cube"), T(texf), LTDST, TEX, SAMP, LTSRC },

	{ AP, 0x40000000f0000000ull, 0xf0000000f8000000ull, // lod needs to be same for everything, or else.
		N("texlod"), T(texf), LTDST, TEX, SAMP, LTSRC, TOFFX, TOFFY, TOFFZ },
	{ AP, 0x40000000f8000000ull, 0xf0000000f8000000ull,
		N("texlod cube"), T(texf), LTDST, TEX, SAMP, LTSRC },

	{ AP, 0x60000000f0000000ull, 0xf00f0000f0000000ull, // integer input and output.
		N("texsize"), T(texf), LTDST, TEX, SAMP, LDST }, // in: LOD, out: size.x, size.y, size.z
	{ AP, 0x60010000f8000000ull, 0xf00f0000f8000000ull, // NVA3+? input: 3 normalized cube coords [float], layer [int]; output: equivalent x, y, combined layer coords to pass to non-cube tex variants.
		N("texprep cube"), T(texf), LTDST, TEX, SAMP, LTSRC },
	{ AP, 0x60020000f0000000ull, 0xf00f0000f8000000ull, // NVA3+, returned values are FIXED-point with 6 fractional bits
		N("texquerylod"), T(texf), LTDST, TEX, SAMP, LTSRC },
	{ AP, 0x60020000f8000000ull, 0xf00f0000f8000000ull,
		N("texquerylod cube"), T(texf), LTDST, TEX, SAMP, LTSRC },

	{ AP, 0x80000000f0000000ull, 0xf0000000f1000000ull, // no idea what this is. but it *is* texturing.
		U("f/8/0"), T(texf), LTDST, TEX, SAMP, LTSRC, TOFFX, TOFFY, TOFFZ },

	{ AP, 0x80000000f1000000ull, 0xf0000000f9000000ull, // NVA3+
		N("texgather"), T(texf), LTDST, TEX, SAMP, LTSRC, TOFFX, TOFFY, TOFFZ },
	{ AP, 0x80000000f9000000ull, 0xf0000000f9000000ull, // NVA3+
		N("texgather cube"), T(texf), LTDST, TEX, SAMP, LTSRC },

	{ GP, 0xc0000000f0000200ull, 0xe0000000f0000600ull, N("emit") },
	{ GP, 0xc0000000f0000400ull, 0xe0000000f0000600ull, N("restart") },

	{ AP, 0xe0000000f0000000ull, 0xe0000004f0000000ull, N("nop") },
	{ AP, 0xe0000004f0000000ull, 0xe0000004f0000000ull, N("pmevent"), PM },

	// desc ^^^
	
	// try to print out *some* info.
	{ AP, 0, 0, OOPS, MCDST, LLDST, T(lsw), T(lc2w), T(lc3w) },

};

/*
 * Predicates
 */
static struct insn tabp[] = {
	{ AP, 0x0000000000000000ull, 0x00000f8000000000ull, N("never") },
	{ AP, 0x0000008000000000ull, 0x00000f8000000000ull, N("l"), COND },
	{ AP, 0x0000010000000000ull, 0x00000f8000000000ull, N("e"), COND },
	{ AP, 0x0000018000000000ull, 0x00000f8000000000ull, N("le"), COND },
	{ AP, 0x0000020000000000ull, 0x00000f8000000000ull, N("g"), COND },
	{ AP, 0x0000028000000000ull, 0x00000f8000000000ull, N("lg"), COND },
	{ AP, 0x0000030000000000ull, 0x00000f8000000000ull, N("ge"), COND },
	{ AP, 0x0000038000000000ull, 0x00000f8000000000ull, N("lge"), COND },
	{ AP, 0x0000040000000000ull, 0x00000f8000000000ull, N("u"), COND },
	{ AP, 0x0000048000000000ull, 0x00000f8000000000ull, N("lu"), COND },
	{ AP, 0x0000050000000000ull, 0x00000f8000000000ull, N("eu"), COND },
	{ AP, 0x0000058000000000ull, 0x00000f8000000000ull, N("leu"), COND },
	{ AP, 0x0000060000000000ull, 0x00000f8000000000ull, N("gu"), COND },
	{ AP, 0x0000068000000000ull, 0x00000f8000000000ull, N("lgu"), COND },
	{ AP, 0x0000070000000000ull, 0x00000f8000000000ull, N("geu"), COND },
	{ AP, 0x0000078000000000ull, 0x00000f8000000000ull },
	{ AP, 0x0000080000000000ull, 0x00000f8000000000ull, N("o"), COND },
	{ AP, 0x0000088000000000ull, 0x00000f8000000000ull, N("c"), COND },
	{ AP, 0x0000090000000000ull, 0x00000f8000000000ull, N("a"), COND },
	{ AP, 0x0000098000000000ull, 0x00000f8000000000ull, N("s"), COND },
	{ AP, 0x00000e0000000000ull, 0x00000f8000000000ull, N("ns"), COND },
	{ AP, 0x00000e8000000000ull, 0x00000f8000000000ull, N("na"), COND },
	{ AP, 0x00000f0000000000ull, 0x00000f8000000000ull, N("nc"), COND },
	{ AP, 0x00000f8000000000ull, 0x00000f8000000000ull, N("no"), COND },
	{ AP, 0, 0, OOPS },
};

static struct insn tabc[] = {
	{ FP, 0x00000000, 0xf0000000, T(p), N("discard") },
	{ AP, 0x10000000, 0xf0000000, T(p), N("bra"), CTARG },
	{ AP, 0x20000000, 0xf0000000, IGNPRED, N("call"), CTARG },
	{ AP, 0x30000000, 0xf0000000, T(p), N("ret") },
	{ AP, 0x40000000, 0xf0000000, IGNPRED, N("breakaddr"), CTARG },
	{ AP, 0x50000000, 0xf0000000, T(p), N("break") },
	{ AP, 0x60000000, 0xf0000000, IGNPRED, N("quadon") },
	{ AP, 0x70000000, 0xf0000000, IGNPRED, N("quadpop") },
	{ AP, 0x861ffe00, 0xf61ffe00, IGNPRED, N("bar sync"), BAR },
	{ AP, 0x90000000, 0xf0000000, IGNPRED, N("trap") },
	{ AP, 0xa0000000, 0xf0000000, IGNPRED, N("joinat"), CTARG },
	{ AP, 0xb0000000, 0xf0000000, T(p), N("brkpt") }, // sm_11. check predicates.
	{ AP, 0, 0, T(p), OOPS, CTARG },
};

static struct insn tab2w[] = {
	{ AP, 0x0000000000000002ull, 0x0000000000000002ull, T(c) },
	{ AP, 0x0000000000000000ull, 0x0000000300000000ull, T(p), T(l) },
	{ AP, 0x0000000100000000ull, 0x0000000300000000ull, T(p), T(l), NL, N("exit") },
	{ AP, 0x0000000200000000ull, 0x0000000300000000ull, N("join"), NL, T(p), T(l) },
	{ AP, 0x0000000300000000ull, 0x0000000300000000ull, T(i) },
};

/*
 * Disassembler driver
 *
 * You pass a block of memory to this function, disassembly goes out to given
 * FILE*.
 */

void nv50dis (FILE *out, uint32_t *code, uint32_t start, int num, int ptype) {
	int cur = 0;
	while (cur < num) {
		ull a = code[cur], m = 0;
		fprintf (out, "%s%08x: %s", cgray, cur*4 + start, cnorm);
		if (code[cur++]&1) {
			if (cur >= num) {
				fprintf (out, "        %08llx ", a);
				fprintf (out, "%sincomplete%s\n", cred, cnorm);
				return;
			}
/*			if (!(cur&1)) {
				fprintf (out, "        %08x ", a);
				fprintf (out, "%smisaligned%s\n", cred, cnorm);
				continue;
			}*/
			a |= (ull)code[cur++] << 32;
			fprintf (out, "%016llx", a);
		} else {
			fprintf (out, "        %08llx", a);
		}
		struct insn *tab = ((a&1)?tab2w:tabs);
		atomtab (out, &a, &m, tab, ptype, cur*4 + start); // fix it one day.
		a &= ~m;
		if (a & ~1ull) {
			fprintf (out, (a&1?" %s[unknown: %016llx]%s":" %s[unknown: %08llx]%s"), cred, a&~1ull, cnorm);
		}
		printf ("%s\n", cnorm);
	}
}
