/*************************************************************************
 *                                                                       *
 *             DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE               *
 *                     Version 2, December 2004                          *
 *                                                                       *
 *  Copyright (C) 2004 Sam Hocevar                                       *
 *   14 rue de Plaisance, 75014 Paris, France                            *
 *  Everyone is permitted to copy and distribute verbatim or modified    *
 *  copies of this license document, and changing it is allowed as long  *
 *  as the name is changed.                                              *
 *                                                                       *
 *             DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE               *
 *    TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION    *
 *                                                                       *
 *   0. You just DO WHAT THE FUCK YOU WANT TO.                           *
 *                                                                       *
 *************************************************************************/

/*
 * Copyright (C) 2009 Marcin Kościelnicki <koriakin@0x04.net>
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

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
 * 16. Disassembler driver
 */

#define VP 1
#define GP 2
#define FP 4
#define CP 8
#define AP (VP|GP|FP|CP)

/*
 * Instructions, from PTX manual:
 *   
 *   1. Integer Arithmetic
 *    - mad		desc...
 *    - mad24		XXX
 *    - sad		XXX
 *    - sad.64		TODO	emulated
 *    - div		TODO	emulated
 *    - rem		TODO	emulated
 *    - abs		desc...
 *    - neg		desc...
 *    - min		desc...
 *    - max		desc...
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
 *    - set		desc...
 *    - setp		desc...
 *    - selp		desc...
 *    - slct		XXX slct WTF?
 *   4. Logic and Shift
 *    - shl		desc... XXX overfl
 *    - shr		desc...
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
 *  - subc -> not+addc
 *  - add/sub on b64 -> long addition/substraction with addc
 *  - neg on integers -> subr with 0
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
 *  - there is subr instruction, which is sub with reversed 2nd and 3rd arg
 *  - instructions and, xor, or can optionally not either one or both arguments
 *    before use
 *  - sad.b16 actually has first and fourth parameters 32-bit.
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
 *  - there are multiple const spaces. how many?
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
 * Color scheme
 */

char cnorm[] = "\x1b[0m";	// lighgray: instruction code and misc stuff
char cgray[] = "\x1b[37m";	// darkgray: instruction address
char cgr[] = "\x1b[32m";	// green: instruction name and mods
char cbl[] = "\x1b[34m";	// blue: $r registers
char ccy[] = "\x1b[36m";	// cyan: memory accesses
char cyel[] = "\x1b[33m";	// yellow: numbers
char cred[] = "\x1b[31m";	// red: unknown stuff
char cbr[] = "\x1b[37m";	// white: code labels
char cmag[] = "\x1b[35m";	// pink: funny registers


/*
 * Table format
 *
 * Decoding various fields is done by multi-level tables of decode operations.
 * Fields of a single table entry:
 *  - bitmask of program types for which this entry applies
 *  - value that needs to match
 *  - mask of bits in opcode that need to match the val
 *  - a sequence of 0 to 8 operations to perform if this entry is matched.
 *
 * Each table is scanned linearly until a matching entry is found, then all
 * ops in this entry are executed. Length of a table is not checked, so they
 * need either a terminator showing '???' for unknown stuff, or match all
 * possible values.
 *
 * A single op is supposed to decode some field of the instruction and print
 * it. In the table, an op is just a function pointer plus a void* that gets
 * passed to it as an argument. atomtab is an op that descends recursively
 * into another table to decude a subfield. T(too) is an op shorthand for use
 * in tables.
 *
 * Each op takes five arguments: output file to print to, pointer to the
 * opcode being decoded, pointer to a mask value of already-used fields,
 * a free-form void * specified in table entry that called this op, and
 * allowed program type bitmask.
 *
 * The mask argument is supposed to detect unknown fields in opcodes: if some
 * bit in decoded opcode is set, but no op executed along the way claims to
 * use it, it's probably some unknown field that could totally change the
 * meaning of instruction and will be printed in bold fucking red to alert
 * the user. Each op ors in the bitmask that it uses, and after all ops are
 * executed, main code prints unclaimed bits. This works for fields that can
 * be used multiple times, like the COND field: it'll be read fine by all ops
 * that happen to use it, but will be marked red if no op used it, like
 * an unconditional non-addc non-mov-from-$c insn.
 *
 * This doesn't work for $a, as there is a special case here: only the first
 * field using it actually gets its value, next ones just use 0. This is
 * hacked around by zeroing out the field directly in passed opcode parameter
 * in the op reading it. This gives proper behavior on stuff like add b32 $r0,
 * s[$a1+0x4], c0[0x10].
 *
 * Macros provided for quickly getting the bitfields: BF(...) gets value of
 * given bitfield and marks it as used in the mask, RCL(...) wipes it
 * from the opcode field directly, allowing for the $a special case. Args are
 * given as start bit, size in bits.
 *
 * Also, three simple ops are provided: N("string") prints a literal to output
 * and is supposed to be used for instruction names and various modifiers.
 * OOPS is for unknown encodings, NL is for separating instructions in case
 * a single opcode represents two [only possible with join and exit].
 */

typedef unsigned long long ull;

#define BF(s, l) (*m |= ((1ull<<l)-1<<s), *a>>s&(1ull<<l)-1)
#define RCL(s, l) (*a &= ~((1ull<<l)-1<<s))

#define APROTO (FILE *out, ull *a, ull *m, const void *v, int ptype)

typedef void (*afun) APROTO;

struct atom {
	afun fun;
	const void *arg;
};

struct insn {
	int ptype;
	ull val;
	ull mask;
	struct atom atoms[16];
};

/*
 * Makes a simple table for checking a single flag.
 *
 * Arguments: table name, flag position, ops for 0, ops for 1.
 */

#define F(n, f, a, b) struct insn tab ## n[] = {\
	{ AP, 0,		1ull<<(f), a },\
	{ AP, 1ull<<(f),	1ull<<(f), b },\
};
#define F1(n, f, b) struct insn tab ## n[] = {\
	{ AP, 0,		1ull<<(f) },\
	{ AP, 1ull<<(f),	1ull<<(f), b },\
};


#define T(x) atomtab, tab ## x
void atomtab APROTO {
	const struct insn *tab = v;
	int i;
	while ((a[0]&tab->mask) != tab->val || !(tab->ptype&ptype))
		tab++;
	m[0] |= tab->mask;
	for (i = 0; i < 16; i++)
		if (tab->atoms[i].fun)
			tab->atoms[i].fun (out, a, m, tab->atoms[i].arg, ptype);
}

#define N(x) atomname, x
void atomname APROTO {
	fprintf (out, " %s%s", cgr, (char *)v);
}

#define NL atomnl, 0
void atomnl APROTO {
	fprintf (out, "\n                          ");
}

#define OOPS atomoops, 0
void atomoops APROTO {
	fprintf (out, " %s???", cred);
}

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
 *  - SHCNT: used in 32-bit shift-by-immediate insns for shift amount
 *  - HSHCNT: used in 16-bit shift-by-immediate insns for shift amount
 */

int pmoff[] = { 0xa, 4, 0 };
int baroff[] = { 0x15, 4, 0 };
int offoff[] = { 9, 16, 0 };
int shcntoff[] = { 0x10, 5, 0 };
int hshcntoff[] = { 0x10, 4, 0 };
int toffxoff[] = { 0x38, 4, 1 };
int toffyoff[] = { 0x34, 4, 1 };
int toffzoff[] = { 0x30, 4, 1 };
#define PM atomnum, pmoff
#define BAR atomnum, baroff
#define OFFS atomnum, offoff
#define SHCNT atomnum, shcntoff
#define HSHCNT atomnum, hshcntoff
#define TOFFX atomnum, toffxoff
#define TOFFY atomnum, toffyoff
#define TOFFZ atomnum, toffzoff
void atomnum APROTO {
	const int *n = v;
	uint32_t num = BF(n[0], n[1]);
	if (n[2] && num&1<<(n[1]-1))
		fprintf (out, " %s-%#x", cyel, (1<<n[1]) - num);
	else
		fprintf (out, " %s%#x", cyel, num);
}

/*
 * Ignored fields
 *
 * Used in cases when some bitfield doesn't do anything, but nvidia's blob
 * sets it due to stupi^H^H^H^H^Hdesign decisions.
 *  - IGNCE: $c write enable
 *  - IGNDTEX: texture id duplicated in higher bits for some reason
 *  - IGNPRED: predicates, for instructions that don't use them.
 *
 * The whole point of this op is to kill spurious red when disassembling
 * ptxas output and blob shaders. Don't include all random unused fields here.
 */

int ignce[] = { 0x26, 1 };
int igndtex[] = { 0x10, 6 };
int ignpred[] = { 0x27, 7 };
#define IGNCE atomign, ignce
#define IGNDTEX atomign, igndtex
#define IGNPRED atomign, ignpred
void atomign APROTO {
	const int *n = v;
	BF(n[0], n[1]);
}

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

int sdstoff[] = { 2, 6, 'r' };
int ldstoff[] = { 2, 7, 'r' };
int ssrcoff[] = { 9, 6, 'r' };
int lsrcoff[] = { 9, 7, 'r' };
int ssrc2off[] = { 0x10, 6, 'r' };
int lsrc2off[] = { 0x10, 7, 'r' };
int lsrc3off[] = { 0x2e, 7, 'r' };
int odstoff[] = { 2, 7, 'o' };
int adstoff[] = { 2, 3, 'a' };
int condoff[] = { 0x2c, 2, 'c' };
int c0off[] = { 0, 0, 'c' };
int cdstoff[] = { 0x24, 2, 'c' };
int texoff[] = { 9, 7, 't' };
#define SDST atomreg, sdstoff
#define LDST atomreg, ldstoff
#define SHDST atomhreg, sdstoff
#define LHDST atomhreg, ldstoff
#define LDDST atomdreg, ldstoff
#define SQDST atomqreg, sdstoff
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
void atomreg APROTO {
	const int *n = v;
	int r = BF(n[0], n[1]);
	if (r == 127 && n[2] == 'o') fprintf (out, " %s#", cbl);
	else fprintf (out, " %s$%c%d", (n[2]=='r')?cbl:cmag, n[2], r);
}
void atomdreg APROTO {
	const int *n = v;
	fprintf (out, " %s$%c%lldd", (n[2]=='r')?cbl:cmag, n[2], BF(n[0], n[1]));
}
void atomqreg APROTO {
	const int *n = v;
	fprintf (out, " %s$%c%lldq", (n[2]=='r')?cbl:cmag, n[2], BF(n[0], n[1]));
}
void atomhreg APROTO {
	const int *n = v;
	int r = BF(n[0], n[1]);
	if (r == 127 && n[2] == 'o') fprintf (out, " %s#", cbl);
	else fprintf (out, " %s$%c%d%c", (n[2]=='r')?cbl:cmag, n[2], r>>1, "lh"[r&1]);
}

int getareg (ull *a, ull *m, int l) {
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
int ssmem[] = { 9, 4, 2, 's', 5, 0, 0 };		// done
int shsmem[] = { 9, 4, 1, 's', 5, 0, 0 };		// done
int sbsmem[] = { 9, 4, 0, 's', 5, 0, 0 };		// done
int lsmem[] = { 9, 5, 2, 's', 7, 0, 0 };		// done
int lhsmem[] = { 9, 5, 1, 's', 7, 0, 0 };		// done
int lbsmem[] = { 9, 5, 0, 's', 7, 0, 0 };		// done
int fsmem[] = { 9, 14, 2, 's', 7, 0, 0 };		// done
int fhsmem[] = { 9, 15, 1, 's', 7, 0, 0 };		// done
int fbsmem[] = { 9, 16, 0, 's', 7, 0, 0 };		// done
// attr
int samem[] = { 9, 6, 2, 'a', 0, 0, 0 };		// TODO
int lamem[] = { 9, 7, 2, 'a', 0, 0, 0 };		// TODO
int famem[] = { 9, 7, 2, 'a', 3, 0, 0 };		// TODO
// prim
int spmem[] = { 9, 6, 2, 'p', 1, 0, 0 };		// TODO
int lpmem[] = { 9, 7, 2, 'p', 3, 0, 0 };		// TODO
// const
int scmem[] = { 0x10, 5, 2, 'c', 1, 0x15, 1 };		// TODO
int shcmem[] = { 0x10, 5, 1, 'c', 1, 0x15, 1 };		// TODO
int lcmem2[] = { 0x10, 7, 2, 'c', 3, 0x36, 4 };		// TODO
int lhcmem2[] = { 0x10, 7, 1, 'c', 3, 0x36, 4 };	// TODO
int lcmem3[] = { 0x2e, 7, 2, 'c', 3, 0x36, 4 };		// TODO
int lhcmem3[] = { 0x2e, 7, 1, 'c', 3, 0x36, 4 };	// TODO
int fcmem[] = { 9, 14, 2, 'c', 7, 0x36, 4 };		// done
int fhcmem[] = { 9, 15, 1, 'c', 7, 0x36, 4 };		// done
int fbcmem[] = { 9, 16, 0, 'c', 7, 0x36, 4 };		// done
// local
int lmem[] = { 9, 16, 0, 'l', 7, 0, 0 };		// done
// varying
int svmem[] = { 0x10, 8, 2, 'v', 1, 0, 0 };		// TODO
int lvmem[] = { 0x10, 8, 2, 'v', 3, 0, 0 };		// TODO

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

int g1mem[] = { 0x10, 4 };
int g2mem[] = { 0x17, 4 };
#define GLOBAL atomglobal, g1mem
#define GLOBAL2 atomglobal, g2mem
void atomglobal APROTO {
	const int *n = v;
	fprintf (out, " %sg%lld[%s$r%lld%s]", ccy, BF(n[0], n[1]), cbl, BF(9, 7), ccy);
}

struct insn tabss[] = {
	{ GP, 0x01800000, 0x01800000, SPRIM },	// XXX check
	{ CP, 0x00000000, 0x00006000, N("u8"), SBSHARED },
	{ CP, 0x00002000, 0x00006000, N("u16"), SHSHARED },
	{ CP, 0x00004000, 0x00006000, N("s16"), SHSHARED },
	{ CP, 0x00006000, 0x00006000, N("b32"), SSHARED },
	{ VP|GP, 0, 0, SATTR },
	{ AP, 0, 0, OOPS },
};

struct insn tabls[] = {
	{ GP, 0x01800000, 0x01800000, LPRIM },	// XXX check
	{ CP, 0x00000000, 0x0000c000, N("u8"), LBSHARED },
	{ CP, 0x00004000, 0x0000c000, N("u16"), LHSHARED },
	{ CP, 0x00008000, 0x0000c000, N("s16"), LHSHARED },
	{ CP, 0x0000c000, 0x0000c000, N("b32"), LSHARED },
	{ VP|GP, 0, 0, LATTR },
	{ AP, 0, 0, OOPS },
};

struct insn tabaddop[] = {
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
F1(sm1high, 8, N("high"))
F(sm1tex, 8, N("all"), N("live"))

F1(sm2neg, 0xf, N("neg"))
F(sm2us16, 0xf, N("u16"), N("s16"))
F(sm2us24, 0xf, N("u24"), N("s24"))
F(sslus2, 0xf, N("u32"), N("s32"))

F1(sm3neg, 0x16, N("neg"))
F1(sm3not, 0x16, N("not"))

struct insn tabssh[] = {
	{ AP, 0x00000000, 0x01000000, SHSRC },
	{ AP, 0x01000000, 0x01000000, T(ss) },
	{ AP, 0, 0, OOPS }
};

struct insn tabssw[] = {
	{ AP, 0x00000000, 0x01000000, SSRC },
	{ AP, 0x01000000, 0x01000000, T(ss) },
	{ AP, 0, 0, OOPS }
};

struct insn tabsch[] = {
	{ AP, 0x00800000, 0x01800000, SHCONST },
	{ AP, 0x00000000, 0x00000000, SHSRC2 },
	{ AP, 0, 0, OOPS }
};

struct insn tabscw[] = {
	{ AP, 0x00800000, 0x01800000, SCONST },
	{ AP, 0x00000000, 0x00000000, SSRC2 },
	{ AP, 0, 0, OOPS }
};

struct insn tabaddc0[] = {
	{ AP, 0x10400000, 0x10400000, C0 },
	{ AP, 0, 0 }
};

struct insn tabas[] = {
	{ AP, 0x00000000, 0x00008000, T(sm1sat), N("b16"), SHDST, T(ssh), T(sch) },
	{ AP, 0x00008000, 0x00008000, T(sm1sat), N("b32"), SDST, T(ssw), T(scw) },
	{ AP, 0, 0, OOPS }
};

struct insn tabs[] = {
	// SCAN 0-1
	{ AP, 0x10000000, 0xf0008002, N("mov"), N("b16"), SHDST, T(ssh) },
	{ AP, 0x10008000, 0xf0008002, N("mov"), N("b32"), SDST, T(ssw) },

	{ AP, 0x20000000, 0xe0008002, T(addop), T(sm1sat), N("b16"), SHDST, T(ssh), T(sch), T(addc0) },
	{ AP, 0x20008000, 0xe0008002, T(addop), T(sm1sat), N("b32"), SDST, T(ssw), T(scw), T(addc0) },

	{ AP, 0x40000000, 0xf0400002, N("mul"), SDST, T(sm2us16), T(ssh), T(sm1us16), T(sch) },
	{ AP, 0x40400000, 0xf0400002, N("mul"), SDST, T(sm1high), T(sm2us24), T(ssw), T(scw) },

	{ AP, 0x50000000, 0xf0008002, N("sad"), SDST, T(sm1us16), SHSRC, SHSRC2, SDST },
	{ AP, 0x50008000, 0xf0008002, N("sad"), SDST, T(sslus2), SSRC, SSRC2, SDST },

	{ AP, 0x60000000, 0xe0008102, T(addop), SDST, N("mul"), N("u16"), T(ssh), T(sch), SDST, T(addc0) },
	{ AP, 0x60000100, 0xe0008102, T(addop), SDST, N("mul"), N("s16"), T(ssh), T(sch), SDST, T(addc0) },
	{ AP, 0x60008000, 0xe0008102, T(addop), N("sat"), SDST, N("mul"), N("s16"), T(ssh), T(sch), SDST, T(addc0) },
	{ AP, 0x60008100, 0xe0008102, T(addop), SDST, N("mul"), N("u24"), T(ssw), T(scw), SDST, T(addc0) },

	// SCAN 9-F

	{ FP, 0x80000000, 0xf3000102, N("interp"), SDST, SVAR },		// most likely something is wrong with this.
	{ FP, 0x81000000, 0xf3000102, N("interp"), SDST, N("cent"), SVAR },
	{ FP, 0x82000000, 0xf3000102, N("interp"), SDST, SVAR, SSRC },
	{ FP, 0x83000000, 0xf3000102, N("interp"), SDST, N("cent"), SVAR, SSRC },
	{ FP, 0x80000100, 0xf3000102, N("interp"), SDST, N("flat"), SVAR },

	{ AP, 0x90000000, 0xf0000002, N("rcp f32"), SDST, SSRC },

	// cvt ? probably not.

	{ AP, 0xb0000000, 0xf0000002, N("add"), T(sm1sat), N("f32"), SDST, T(sm2neg), T(ssw), T(sm3neg), T(scw) },

	{ AP, 0xc0000000, 0xf0000002, N("mul f32"), SDST, T(sm2neg), T(ssw), T(sm3neg), T(scw) },
	// and

	{ AP, 0xe0000000, 0xf0000002, N("mad f32"), SDST, SSRC, SSRC2, SDST },	// XXX: flags like tabi?

	{ AP, 0xf0000000, 0xf1000002, N("texauto"), T(sm1tex), STDST, TEX, STSRC, IGNDTEX },
	{ AP, 0xf1000000, 0xf1000002, N("texfetch"), T(sm1tex), STDST, TEX, STSRC, IGNDTEX },

	{ AP, 0, 2, OOPS, SDST, T(ssw), T(scw) },
	{ AP, 0, 0, OOPS }
};

/*
 * Immediate instructions
 */

struct insn tabi[] = {
	// SCAN 0-1
	{ AP, 0x10000000, 0xf0008000, N("mov"), N("b16"), LHDST, IMM },
	{ AP, 0x10008000, 0xf0008000, N("mov"), N("b32"), LDST, IMM },	// yes. LDST. special case.

	{ AP, 0x20000000, 0xe0008000, T(addop), T(sm1sat), N("b16"), SHDST, T(ssh), IMM, T(addc0) },
	{ AP, 0x20008000, 0xe0008000, T(addop), T(sm1sat), N("b32"), SDST, T(ssw), IMM, T(addc0) },

	{ AP, 0x40000000, 0xf0400000, N("mul"), SDST, T(sm2us16), T(ssh), T(sm1us16), IMM },
	{ AP, 0x40400000, 0xf0400000, N("mul"), SDST, T(sm1high), T(sm2us24), T(ssw), IMM },

	// SCAN 6-F
	{ AP, 0x60000000, 0xe0008100, T(addop), SDST, N("mul"), N("u16"), T(ssh), IMM, SDST, T(addc0) }, // XXX: warning: guess here.
	{ AP, 0x60000100, 0xe0008100, T(addop), SDST, N("mul"), N("s16"), T(ssh), IMM, SDST, T(addc0) },
	{ AP, 0x60008000, 0xe0008100, T(addop), N("sat"), SDST, N("mul"), N("s16"), T(ssh), IMM, SDST, T(addc0) },
	{ AP, 0x60008100, 0xe0008100, T(addop), SDST, N("mul"), N("u24"), T(ssw), IMM, SDST, T(addc0) },

	{ AP, 0xb0000000, 0xf0000000, N("add"), T(sm1sat), N("f32"), SDST, T(sm2neg), T(ssw), T(sm3neg), IMM },

	{ AP, 0xc0000000, 0xf0000000, N("mul f32"), SDST, T(sm2neg), T(ssw), T(sm3neg), IMM },

	{ AP, 0xd0000000, 0xf0008100, N("and"), N("b32"), SDST, T(sm3not), T(ssw), IMM },
	{ AP, 0xd0000100, 0xf0008100, N("or"), N("b32"), SDST, T(sm3not), T(ssw), IMM },
	{ AP, 0xd0008000, 0xf0008100, N("xor"), N("b32"), SDST, T(sm3not), T(ssw), IMM },
	{ AP, 0xd0008100, 0xf0008100, N("mov2"), N("b32"), SDST, T(sm3not), T(ssw), IMM },

	{ AP, 0xe0000000, 0xf0000000, N("mad"), T(sm1sat), N("f32"), SDST, T(sm2neg), T(ssw), IMM, T(sm3neg), SDST },

	{ AP, 0, 2, OOPS, SDST, T(ssw), IMM },
	{ AP, 0, 0, OOPS }
};

/*
 * Long instructions
 */

// A few helper tables for usual operand types.

F(lsh, 0x35, LHSRC, T(ls))
F(lsw, 0x35, LSRC, T(ls))

struct insn tablc2h[] = {
	{ AP, 0x00800000, 0x01800000, LHCONST2 },
	{ AP, 0x00000000, 0x00000000, LHSRC2 },
	{ AP, 0, 0, OOPS }
};

struct insn tablc2w[] = {
	{ AP, 0x00800000, 0x01800000, LCONST2 },
	{ AP, 0x00000000, 0x00000000, LSRC2 },
};

struct insn tablc3h[] = {
	{ AP, 0x01000000, 0x01800000, LHCONST3 },
	{ AP, 0x00000000, 0x00000000, LHSRC3 },
};

struct insn tablc3w[] = {
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
F1(m1neg, 0x3a, N("neg"))
// the actual misc field
F1(m2neg, 0x3b, N("neg"))
F1(m2sat, 0x3b, N("sat"))
F(lm2us16, 0x3b, N("u16"), N("s16"))
F(lm2us32, 0x3b, N("u32"), N("s32"))
// stolen from opcode field.
F1(o0neg, 0x3d, N("neg"))
F1(o0sat, 0x3d, N("sat"))

struct insn tablfm1[] = {
	{ AP, 0, 0, T(s36abs), T(m1neg) }
};

struct insn tablfm2[] = {
	{ AP, 0, 0, T(s35abs), T(m2neg) }
};

struct insn tabcvtmod[] = {
	{ AP, 0, 0, T(s36abs), T(o0neg) },
};

// for g[] and l[] insns.
struct insn tabldstm[] = { // we seriously need to unify these, if possible. I wonder if atomics will work with smaller sizes.
	{ AP, 0x0000000000000000ull, 0x00e0000000000000ull, N("u8") },
	{ AP, 0x0020000000000000ull, 0x00e0000000000000ull, N("s8") },
	{ AP, 0x0040000000000000ull, 0x00e0000000000000ull, N("u16") },
	{ AP, 0x0060000000000000ull, 0x00e0000000000000ull, N("s16") },
	{ AP, 0x0080000000000000ull, 0x00e0000000000000ull, N("b64") },
	{ AP, 0x00a0000000000000ull, 0x00e0000000000000ull, N("b128") },
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, N("b32") },
	{ AP, 0, 0, OOPS }
};

struct insn tabraadd[] = {
	{ AP, 0x0080000000000000ull, 0x00e0000000000000ull, N("u64") },
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, N("u32") },
	{ AP, 0x00e0000000000000ull, 0x00e0000000000000ull, N("s32") },
	{ AP, 0, 0, OOPS }
};

struct insn tabrab[] = {
	{ AP, 0x0080000000000000ull, 0x00e0000000000000ull, N("b64") },
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, N("b32") },
	{ AP, 0, 0, OOPS }
};

struct insn tabramm[] = {
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, N("u32") },
	{ AP, 0x00e0000000000000ull, 0x00e0000000000000ull, N("s32") },
	{ AP, 0, 0, OOPS }
};

struct insn tabrab32[] = {
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, N("b32") },
	{ AP, 0, 0, OOPS }
};

struct insn tabrau32[] = {
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, N("u32") },
	{ AP, 0, 0, OOPS }
};

struct insn tabldsto[] = {	// hack alert: reads the bitfield second time.
	{ AP, 0x0000000000000000ull, 0x00e0000000000000ull, LDST },
	{ AP, 0x0020000000000000ull, 0x00e0000000000000ull, LDST },
	{ AP, 0x0040000000000000ull, 0x00e0000000000000ull, LDST },
	{ AP, 0x0060000000000000ull, 0x00e0000000000000ull, LDST },
	{ AP, 0x0080000000000000ull, 0x00e0000000000000ull, LDDST },
	{ AP, 0x00a0000000000000ull, 0x00e0000000000000ull, LQDST },
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, LDST },
	{ AP, 0, 0, LDST },
};

struct insn tabldsts2[] = {
	{ AP, 0x0080000000000000ull, 0x00e0000000000000ull, LDSRC2 },
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, LSRC2 },
	{ AP, 0x00e0000000000000ull, 0x00e0000000000000ull, LSRC2 },
	{ AP, 0, 0, OOPS },
};

struct insn tabldsts3[] = {
	{ AP, 0x0080000000000000ull, 0x00e0000000000000ull, LDSRC3 },
	{ AP, 0x00c0000000000000ull, 0x00e0000000000000ull, LSRC3 },
	{ AP, 0x00e0000000000000ull, 0x00e0000000000000ull, LSRC3 },
	{ AP, 0, 0, OOPS },
};

struct insn tabmldsts3[] = {
	{ AP, 0x0000000800000000ull, 0x0000000800000000ull, T(ldsts3) },
	{ AP, 0, 0 },
};

struct insn tabredm[] = {
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

struct insn tabatomm[] = {
	{ AP, 0x0000000400000000ull, 0x0000003c00000000ull, N("exch"), T(rab), },
	{ AP, 0x0000000800000000ull, 0x0000003c00000000ull, N("cas"), T(rab), },
	{ AP, 0, 0, N("ld"), T(redm) },
};

// rounding modes
struct insn tabcvtrnd[] = {
	{ AP, 0x0000000000000000ull, 0x0006000000000000ull, N("rn") },
	{ AP, 0x0002000000000000ull, 0x0006000000000000ull, N("rm") },
	{ AP, 0x0004000000000000ull, 0x0006000000000000ull, N("rp") },
	{ AP, 0x0006000000000000ull, 0x0006000000000000ull, N("rz") },
	{ AP, 0, 0, OOPS }
};

struct insn tabcvtrint[] = {
	{ AP, 0x0000000000000000ull, 0x0006000000000000ull, N("rni") },
	{ AP, 0x0002000000000000ull, 0x0006000000000000ull, N("rmi") },
	{ AP, 0x0004000000000000ull, 0x0006000000000000ull, N("rpi") },
	{ AP, 0x0006000000000000ull, 0x0006000000000000ull, N("rzi") },
	{ AP, 0, 0, OOPS }
};

struct insn tabaf64r[] = {
	{ AP, 0x00000000, 0x00030000, N("rn") },
	{ AP, 0x00010000, 0x00030000, N("rm") },
	{ AP, 0x00020000, 0x00030000, N("rp") },
	{ AP, 0x00030000, 0x00030000, N("rz") },
};

struct insn tabaf32r[] = {
	{ AP, 0x00000000, 0x00030000, N("rn") },
	{ AP, 0x00030000, 0x00030000, N("rz") },
	{ AP, 0, 0, OOPS }
};

struct insn tabmf32r[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, N("rn") },
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, N("rz") },
	{ AP, 0, 0, OOPS }
};

struct insn tabmad64r[] = {
	{ AP, 0x0000000000000000ull, 0x00c0000000000000ull, N("rn") },
	{ AP, 0x0040000000000000ull, 0x00c0000000000000ull, N("rm") },
	{ AP, 0x0080000000000000ull, 0x00c0000000000000ull, N("rp") },
	{ AP, 0x00c0000000000000ull, 0x00c0000000000000ull, N("rz") },
	{ AP, 0, 0, OOPS }
};

// ops for set
struct insn tabseti[] = {
	{ AP, 0x0000400000000000ull, 0x0001c00000000000ull, N("lt") },
	{ AP, 0x0000800000000000ull, 0x0001c00000000000ull, N("eq") },
	{ AP, 0x0000c00000000000ull, 0x0001c00000000000ull, N("le") },
	{ AP, 0x0001000000000000ull, 0x0001c00000000000ull, N("gt") },
	{ AP, 0x0001400000000000ull, 0x0001c00000000000ull, N("ne") },
	{ AP, 0x0001800000000000ull, 0x0001c00000000000ull, N("ge") },
	{ AP, 0, 0, OOPS }
};

struct insn tabsetf[] = {
	{ AP, 0x0000400000000000ull, 0x0003c00000000000ull, N("lt") },
	{ AP, 0x0000800000000000ull, 0x0003c00000000000ull, N("eq") },
	{ AP, 0x0000c00000000000ull, 0x0003c00000000000ull, N("le") },
	{ AP, 0x0001000000000000ull, 0x0003c00000000000ull, N("gt") },
	{ AP, 0x0001400000000000ull, 0x0003c00000000000ull, N("ne") },
	{ AP, 0x0001800000000000ull, 0x0003c00000000000ull, N("ge") },
	{ AP, 0x0001c00000000000ull, 0x0003c00000000000ull, N("num") },
	{ AP, 0x0002000000000000ull, 0x0003c00000000000ull, N("nan") },
	{ AP, 0x0002400000000000ull, 0x0003c00000000000ull, N("ltu") },
	{ AP, 0x0002800000000000ull, 0x0003c00000000000ull, N("equ") },
	{ AP, 0x0002c00000000000ull, 0x0003c00000000000ull, N("leu") },
	{ AP, 0x0003000000000000ull, 0x0003c00000000000ull, N("gtu") },
	{ AP, 0x0003400000000000ull, 0x0003c00000000000ull, N("neu") },
	{ AP, 0x0003800000000000ull, 0x0003c00000000000ull, N("geu") },
	{ AP, 0, 0, OOPS }
};

// for cvt
struct insn tabcvtiisrc[] ={
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

struct insn tabtexf[] = {
	{ AP, 0, 0, T(ltex), T(dtex), IGNDTEX },
};

// for mov
struct insn tablane[] = {
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
struct insn tabfcon[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, N("u8"), FBCONST },
	{ AP, 0x0000400000000000ull, 0x0000c00000000000ull, N("u16"), FHCONST },
	{ AP, 0x0000800000000000ull, 0x0000c00000000000ull, N("s16"), FHCONST },
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, N("b32"), FCONST },
	{ AP, 0, 0, OOPS }
};

// for quadop
struct insn tabqs1[] = {
	{ AP, 0x00000000, 0x00030000, N("l0") },
	{ AP, 0x00010000, 0x00030000, N("l1") },
	{ AP, 0x00020000, 0x00030000, N("l2") },
	{ AP, 0x00030000, 0x00030000, N("l3") },
	{ AP, 0, 0 },
};

struct insn tabqop0[] = {
	{ AP, 0x0000000000000000ull, 0x0c00000000000000ull, N("add") },
	{ AP, 0x0400000000000000ull, 0x0c00000000000000ull, N("subr") },
	{ AP, 0x0800000000000000ull, 0x0c00000000000000ull, N("sub") },
	{ AP, 0x0c00000000000000ull, 0x0c00000000000000ull, N("mov2") },
};

struct insn tabqop1[] = {
	{ AP, 0x0000000000000000ull, 0x0300000000000000ull, N("add") },
	{ AP, 0x0100000000000000ull, 0x0300000000000000ull, N("subr") },
	{ AP, 0x0200000000000000ull, 0x0300000000000000ull, N("sub") },
	{ AP, 0x0300000000000000ull, 0x0300000000000000ull, N("mov2") },
};

struct insn tabqop2[] = {
	{ AP, 0x0000000000000000ull, 0x00c0000000000000ull, N("add") },
	{ AP, 0x0040000000000000ull, 0x00c0000000000000ull, N("subr") },
	{ AP, 0x0080000000000000ull, 0x00c0000000000000ull, N("sub") },
	{ AP, 0x00c0000000000000ull, 0x00c0000000000000ull, N("mov2") },
};

struct insn tabqop3[] = {
	{ AP, 0x00000000, 0x00300000, N("add") },
	{ AP, 0x00100000, 0x00300000, N("subr") },
	{ AP, 0x00200000, 0x00300000, N("sub") },
	{ AP, 0x00300000, 0x00300000, N("mov2") },
};

// for mov from sreg
struct insn tabsreg[] = {
	{ AP, 0x0000000000000000ull, 0x0001c00000000000ull, N("physid") },
	{ AP, 0x0000400000000000ull, 0x0001c00000000000ull, N("clock") },
	{ AP, 0x0000800000000000ull, 0x0001c00000000000ull, N("sreg2") },
	{ AP, 0x0000c00000000000ull, 0x0001c00000000000ull, N("sreg3") },
	{ AP, 0x0001000000000000ull, 0x0001c00000000000ull, N("pm0") },
	{ AP, 0x0001400000000000ull, 0x0001c00000000000ull, N("pm1") },
	{ AP, 0x0001800000000000ull, 0x0001c00000000000ull, N("pm2") },
	{ AP, 0x0001c00000000000ull, 0x0001c00000000000ull, N("pm3") },
};

struct insn tablogop[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, N("and") },
	{ AP, 0x0000400000000000ull, 0x0000c00000000000ull, N("or") },
	{ AP, 0x0000800000000000ull, 0x0000c00000000000ull, N("xor") },
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, N("mov2") },
};

struct insn tabaddcond[] = {
	{ AP, 0x10400000, 0x10400000, COND },
	{ AP, 0, 0 }
};

struct insn tabl[] = {
	// 0
	{ VP|GP, 0x0420000000000000ull, 0xe4200000f0000000ull,
		T(lane), N("mov"), N("b32"), LLDST, FATTR },
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
	
	{ CP, 0xe000000000000000ull, 0xe4400000f0000000ull,	// XXX ok, seriously, what's up with all thse flags?
		N("mov b16"), FHSHARED, LHSRC3 },
	{ CP, 0xe040000000000000ull, 0xe4400000f0000000ull,
		N("mov b8"), FBSHARED, LHSRC3 },
	{ CP, 0xe420000000000000ull, 0xe4a00000f0000000ull,
		N("mov b32"), FSHARED, LSRC3 },
	{ CP, 0xe4a0000000000000ull, 0xe4a00000f0000000ull,
		N("mov unlock b32"), FSHARED, LSRC3 },

	// 1
	{ AP, 0x0000000010000000ull, 0xe4000000f0000000ull,
		T(lane), N("mov"), N("b16"), LLHDST, T(lsh) },
	{ AP, 0x0400000010000000ull, 0xe4000000f0000000ull,
		T(lane), N("mov"), N("b32"), LLDST, T(lsw) },

	{ AP, 0x2000000010000000ull, 0xe4000000f0000000ull,
		N("mov b16"), LLHDST, T(fcon) },
	{ AP, 0x2400000010000000ull, 0xe4000000f0000000ull,
		N("mov b32"), LLDST, T(fcon) },

	{ CP, 0x4000400010000000ull, 0xe400c000f0000000ull,	// sm_11. ptxas inexplicably starts using
		N("mov u16"), LHDST, LHSHARED },		// these forms instead of the other one
	{ CP, 0x4000800010000000ull, 0xe400c000f0000000ull,	// on >=sm_11. XXX check length XXX mode
		N("mov s16"), LHDST, LHSHARED },
	{ CP, 0x4400c00010000000ull, 0xe480c000f0000000ull,	// getting ridiculous.
		N("mov b32"), LDST, LSHARED },
	{ CP, 0x4480c04010000000ull, 0xe480c040f0000000ull,
		N("mov lock b32"), CDST, LDST, LSHARED },

	{ AP, 0x6000000010000200ull, 0xe0000000f0000600ull,
		N("vote any"), CDST, IGNCE },	// sm_12
	{ AP, 0x6000000010000400ull, 0xe0000000f0000600ull,
		N("vote all"), CDST, IGNCE },	// sm_12

	// 2 and 3
	{ AP, 0x0000000020000000ull, 0xe4000000e0000000ull, T(addop), N("b16"), T(m2sat), MCDST, LLHDST, T(lsh), T(lc3h), T(addcond) },
	{ AP, 0x0400000020000000ull, 0xe4000000e0000000ull, T(addop), N("b32"), T(m2sat), MCDST, LLDST, T(lsw), T(lc3w), T(addcond) },

	// YARLY.
	{ AP, 0x6000000030000000ull, 0xec000000f0000000ull,
		N("set"), T(seti), N("u16"), MCDST, LLHDST, T(lsh), T(lc2h) },
	{ AP, 0x6800000030000000ull, 0xec000000f0000000ull,
		N("set"), T(seti), N("s16"), MCDST, LLHDST, T(lsh), T(lc2h) },
	{ AP, 0x6400000030000000ull, 0xec000000f0000000ull,
		N("set"), T(seti), N("u32"), MCDST, LLDST, T(lsw), T(lc2w) },
	{ AP, 0x6c00000030000000ull, 0xec000000f0000000ull,
		N("set"), T(seti), N("s32"), MCDST, LLDST, T(lsw), T(lc2w) },

	{ AP, 0x8000000030000000ull, 0xec000000f0000000ull,
		N("max u16"), LHDST, T(lsh), T(lc2h) },
	{ AP, 0x8800000030000000ull, 0xec000000f0000000ull,
		N("max s16"), LHDST, T(lsh), T(lc2h) },
	{ AP, 0x8400000030000000ull, 0xec000000f0000000ull,
		N("max u32"), LDST, T(lsw), T(lc2w) },
	{ AP, 0x8c00000030000000ull, 0xec000000f0000000ull,
		N("max s32"), LDST, T(lsw), T(lc2w) },

	{ AP, 0xa000000030000000ull, 0xec000000f0000000ull,
		N("min u16"), LHDST, T(lsh), T(lc2h) },
	{ AP, 0xa800000030000000ull, 0xec000000f0000000ull,
		N("min s16"), LHDST, T(lsh), T(lc2h) },
	{ AP, 0xa400000030000000ull, 0xec000000f0000000ull,
		N("min u32"), LDST, T(lsw), T(lc2w) },
	{ AP, 0xac00000030000000ull, 0xec000000f0000000ull,
		N("min s32"), LDST, T(lsw), T(lc2w) },

	{ AP, 0xc000000030000000ull, 0xe4000000f0000000ull,	// XXX FUCK ALERT: shift count *CAN* be 16-bit.
		N("shl b16"), LHDST, T(lsh), T(hshcnt) },
	{ AP, 0xc400000030000000ull, 0xe4000000f0000000ull,
		N("shl b32"), LDST, T(lsw), T(shcnt) },
	{ AP, 0xe000000030000000ull, 0xec000000f0000000ull,
		N("shr u16"), LHDST, T(lsh), T(hshcnt) },
	{ AP, 0xe400000030000000ull, 0xec000000f0000000ull,
		N("shr u32"), LDST, T(lsw), T(shcnt) },
	{ AP, 0xe800000030000000ull, 0xec000000f0000000ull,
		N("shr s16"), LHDST, T(lsh), T(hshcnt) },
	{ AP, 0xec00000030000000ull, 0xec000000f0000000ull,
		N("shr s32"), LDST, T(lsw), T(shcnt) },


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

	// 6
	{ AP, 0x0000000060000000ull, 0xec000000f0000000ull,
		N("mad u16"), MCDST, LLDST, T(lsh), T(lc2h), T(lc3w) },
	{ AP, 0x0c00000060000000ull, 0xec000000f0000000ull,
		N("mad u16"), MCDST, LLDST, T(lsh), T(lc2h), T(lc3w), COND },
	{ AP, 0x2000000060000000ull, 0xe0000000f0000000ull,
		N("mad s16"), MCDST, LLDST, T(lsh), T(lc2h), T(lc3w) },
	{ AP, 0x6000000060000000ull, 0xe0000000f0000000ull,
		N("mad u24"), LDST, T(lsw), T(lc2w), T(lc3w) },
	{ AP, 0x8000000060000000ull, 0xe0000000f0000000ull,
		N("mad s24"), LDST, T(lsw), T(lc2w), T(lc3w) },
	{ AP, 0xc000000060000000ull, 0xe0000000f0000000ull,
		N("mad high u24"), LDST, T(lsw), T(lc2w), T(lc3w) },
	{ AP, 0xe000000060000000ull, 0xe0000000f0000000ull,
		N("mad high s24"), LDST, T(lsw), T(lc2w), T(lc3w) },

	// 7
	{ AP, 0x0000000070000000ull, 0xe0000000f0000000ull,
		N("mad high s24 sat"), LDST, T(lsw), T(lc2w), T(lc3w) },

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
		N("rcp f32"), LLDST, LSRC },
	{ AP, 0x4000000090000000ull, 0xe0000000f0000000ull,
		N("rsqrt f32"),  LLDST, LSRC },
	{ AP, 0x6000000090000000ull, 0xe0000000f0000000ull,
		N("lg2 f32"), LLDST, LSRC },
	{ AP, 0x8000000090000000ull, 0xe0000000f0000000ull,
		N("sin f32"), LLDST, LSRC },
	{ AP, 0xa000000090000000ull, 0xe0000000f0000000ull,
		N("cos f32"), LLDST, LSRC },
	{ AP, 0xc000000090000000ull, 0xe0000000f0000000ull,
		N("ex2 f32"), LLDST, LSRC },

	// a
	{ AP, 0xc0000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), N("f16"), LHDST, N("f16"), T(lsh) },
	{ AP, 0xc8000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrint), N("f16"), LHDST, N("f16"), T(lsh) },
	{ AP, 0xc0004000a0000000ull, 0xc4404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f16"), LHDST, N("f32"), T(lsw) },
	{ AP, 0xc4004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), N("f32"), MCDST, LLDST, N("f32"), T(lsw) },
	{ AP, 0xcc004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrint), N("f32"), LDST, N("f32"), T(lsw) },
	{ AP, 0xc4000000a0000000ull, 0xc4404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), N("f32"), LDST, N("f16"), T(lsh) },

	{ AP, 0xc0404000a0000000ull, 0xc4404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrnd), N("f32"), LDST, N("f64"), LDSRC },
	{ AP, 0xc4404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), N("f64"), LDDST, N("f64"), LDSRC },
	{ AP, 0xcc404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("f64"), LDDST, N("f64"), LDSRC },
	{ AP, 0xc4400000a0000000ull, 0xc4404000f0000000ull,
		N("cvt"), T(cvtmod), N("f64"), LDDST, N("f32"), T(lsw) },

	{ AP, 0x80000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u16"), LHDST, N("f16"), T(lsh) },
	{ AP, 0x80004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u16"), LHDST, N("f32"), T(lsw) },
	{ AP, 0x88000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s16"), LHDST, N("f16"), T(lsh) },
	{ AP, 0x88004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s16"), LHDST, N("f32"), T(lsw) },
	{ AP, 0x84000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u32"), LDST, N("f16"), T(lsh) },
	{ AP, 0x84004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u32"), LDST, N("f32"), T(lsw) },
	{ AP, 0x8c000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s32"), LDST, N("f16"), T(lsh) },
	{ AP, 0x8c004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s32"), LDST, N("f32"), T(lsw) },

	{ AP, 0x80404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u32"), LDST, N("f64"), LDSRC },
	{ AP, 0x88404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s32"), LDST, N("f64"), LDSRC },
	{ AP, 0x84400000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u64"), LDDST, N("f32"), T(lsw) },
	{ AP, 0x84404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u64"), LDDST, N("f64"), LDSRC },
	{ AP, 0x8c400000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s64"), LDDST, N("f32"), T(lsw) },
	{ AP, 0x8c404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s64"), LDDST, N("f64"), LDSRC },

	{ AP, 0x40000000a0000000ull, 0xc4400000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f16"), LHDST, T(cvtiisrc) },
	{ AP, 0x44000000a0000000ull, 0xc4400000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f32"), LDST, T(cvtiisrc) },

	{ AP, 0x44400000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f64"), LDDST, N("u32"), T(lsw) },
	{ AP, 0x44410000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f64"), LDDST, N("s32"), T(lsw) },
	{ AP, 0x40404000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f32"), LDST, N("u64"), LDSRC },
	{ AP, 0x40414000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f32"), LDST, N("s64"), LDSRC },
	{ AP, 0x44404000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f64"), LDDST, N("u64"), LDSRC },
	{ AP, 0x44414000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f64"), LDDST, N("s64"), LDSRC },

	{ AP, 0x00000000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("u16"), MCDST, LLHDST, T(cvtiisrc) },
	{ AP, 0x00080000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("u8"), LHDST, T(cvtiisrc) },
	{ AP, 0x04000000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("u32"), MCDST, LLDST, T(cvtiisrc) },
	{ AP, 0x04080000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("u8"), LDST, T(cvtiisrc) },
	{ AP, 0x08000000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("s16"), MCDST, LLHDST, T(cvtiisrc) },
	{ AP, 0x08080000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("s8"), LHDST, T(cvtiisrc) },
	{ AP, 0x0c000000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("s32"), MCDST, LLDST, T(cvtiisrc) },
	{ AP, 0x0c080000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("s8"), LDST, T(cvtiisrc) },


	// b
	{ AP, 0x00000000b0000000ull, 0xc0000000f0000000ull,
		N("add"), T(o0sat), T(af32r),  N("f32"), LLDST, T(m1neg), T(lsw), T(m2neg), T(lc3w) },
	{ AP, 0x80000000b0000000ull, 0xe0000000f0000000ull,
		N("max f32"), LLDST, T(lfm1), T(lsw), T(lfm2), T(lc2w) },
	{ AP, 0xa0000000b0000000ull, 0xe0000000f0000000ull,
		N("min f32"), LLDST, T(lfm1), T(lsw), T(lfm2), T(lc2w) },

	{ AP, 0x60000000b0000000ull, 0xe0000000f0000000ull,
		N("set"), T(setf), N("f32"), MCDST, LLDST, T(lfm1), T(lsw), T(lfm2), T(lc2w) },

	{ AP, 0xc0000000b0000000ull, 0xe0004000f0000000ull,
		N("presin f32"), LLDST, T(lsw) },
	{ AP, 0xc0004000b0000000ull, 0xe0004000f0000000ull,
		N("preex2 f32"), LLDST, T(lsw) },
	/* preex2 converts float to fixed point, results:
	 * 0-0x3fffffff: 7.23 fixed-point number
	 * 0x40000000: +nan
	 * 0x40800000: +infinity
	 * flip bit 0x80000000 in any of the above for negative numbers.
	 * presin divides by pi/2, mods with 4 [or with 2*pi, pre-div], then does preex2
	 */

	// c
	{ AP, 0x00000000c0000000ull, 0xe0000000f0000000ull,
		N("mul"), T(mf32r), N("f32"), LLDST, T(m1neg), T(lsw), T(m2neg), T(lc2w) },

	{ AP, 0x80000000c0000000ull, 0xf0000000f0040000ull,
		N("quadop f32"), T(qop0), T(qop1), T(qop2), T(qop3), MCDST, LLDST, T(qs1), LSRC, LSRC3 },

	{ AP, 0x89800000c0140000ull, 0x8bc00000f0150000ull,	// XXX fuck me harder.
		N("dfdx f32"), LDST, LSRC, LSRC3 },
	{ AP, 0x8a400000c0150000ull, 0x8bc00000f0150000ull,
		N("dfdy f32"), LDST, LSRC, LSRC3 },

	// d
	{ AP, 0x00000000d0000000ull, 0xe4000000f0000000ull,
		T(logop), N("b16"), MCDST, LLHDST, T(s32not), T(lsh), T(s33not), T(lc2h) },
	{ AP, 0x04000000d0000000ull, 0xe4000000f0000000ull,
		T(logop), N("b32"), MCDST, LLDST, T(s32not), T(lsw), T(s33not), T(lc2w) },

	{ AP, 0x20000000d0000000ull, 0xe0000000f0000000ull,
		N("add"), ADST, AREG, OFFS },

	{ AP, 0x40000000d0000000ull, 0xe0000000f0000000ull,
		N("mov"), T(ldstm), T(ldsto), LOCAL },
	{ AP, 0x60000000d0000000ull, 0xe0000000f0000000ull,
		N("mov"), T(ldstm), LOCAL, T(ldsto) },

	{ CP, 0x80000000d0000000ull, 0xe0000000f0000000ull,
		N("mov"), T(ldstm), T(ldsto), GLOBAL },
	{ CP, 0xa0000000d0000000ull, 0xe0000000f0000000ull,
		N("mov"), T(ldstm), GLOBAL, T(ldsto) },
	{ CP, 0xc0000000d0000000ull, 0xe0000000f0000000ull,
		T(redm), GLOBAL, T(ldsto) },
	{ CP, 0xe0000000d0000000ull, 0xe0000000f0000000ull,
		T(atomm), T(ldsto), GLOBAL2, T(ldsts2), T(mldsts3) },

	// e
	{ AP, 0x00000000e0000000ull, 0xc0000000f0000000ull,
		N("mad"), T(o0sat), N("f32"), LLDST, T(m1neg), T(lsw), T(lc2w), T(m2neg), T(lc3w) },	// XXX what happens if you try both?

	{ AP, 0x40000000e0000000ull, 0xe0000000f0000000ull,
		N("mad"), T(mad64r), N("f64"), LDDST, T(m1neg), LDSRC, LDSRC2, T(m2neg), LDSRC3 },
	{ AP, 0x60000000e0000000ull, 0xe0000000f0000000ull,
		N("add"), T(af64r), N("f64"), LDDST, T(m1neg), LDSRC, T(m2neg), LDSRC3 },
	{ AP, 0x80000000e0000000ull, 0xe0000000f0000000ull,
		N("mul"), T(cvtrnd), N("f64"), LDDST, T(m1neg), LDSRC, LDSRC2 },
	{ AP, 0xa0000000e0000000ull, 0xe0000000f0000000ull,
		N("min"), N("f64"), LDDST, T(lfm1), LDSRC, T(lfm2), LDSRC2 },
	{ AP, 0xc0000000e0000000ull, 0xe0000000f0000000ull,
		N("max"), N("f64"), LDDST, T(lfm1), LDSRC, T(lfm2), LDSRC2 },
	{ AP, 0xe0000000e0000000ull, 0xe0000000f0000000ull,
		N("set"), T(setf), N("f64"), MCDST, LLDST, T(lfm1), LDSRC, T(lfm2), LDSRC2 },

	// f
	{ AP, 0x00000000f0000000ull, 0xf0000000f9000000ull, // order of inputs: x, y, z, index, dref, bias/lod. index is integer, others float.
		N("texauto"), T(texf), LTDST, TEX, LTSRC, TOFFX, TOFFY, TOFFZ },
	{ AP, 0x00000000f8000000ull, 0xf0000000f9000000ull,
		N("texauto cube"), T(texf), LTDST, TEX, LTSRC },

	{ AP, 0x00000000f1000000ull, 0xf0000000f1000000ull, // takes integer inputs.
		N("texfetch"), T(texf), LTDST, TEX, LTSRC, TOFFX, TOFFY, TOFFZ },

	{ AP, 0x20000000f0000000ull, 0xf0000000f8000000ull, // bias needs to be same for everything, or else.
		N("texbias"), T(texf), LTDST, TEX, LTSRC, TOFFX, TOFFY, TOFFZ },
	{ AP, 0x20000000f8000000ull, 0xf0000000f8000000ull,
		N("texbias cube"), T(texf), LTDST, TEX, LTSRC },

	{ AP, 0x40000000f0000000ull, 0xf0000000f8000000ull, // lod needs to be same for everything, or else.
		N("texlod"), T(texf), LTDST, TEX, LTSRC, TOFFX, TOFFY, TOFFZ },
	{ AP, 0x40000000f8000000ull, 0xf0000000f8000000ull,
		N("texlod cube"), T(texf), LTDST, TEX, LTSRC },

	{ AP, 0x60000000f0000000ull, 0xf0000000f0000000ull, // integer input and output.
		N("texsize"), T(texf), LTDST, TEX, LDST }, // in: LOD, out: size.x, size.y, size.z

	{ GP, 0xc0000000f0000200ull, 0xe0000000f0000600ull, N("emit") },
	{ GP, 0xc0000000f0000400ull, 0xe0000000f0000600ull, N("restart") },

	{ AP, 0xe0000000f0000000ull, 0xe0000004f0000000ull, N("nop") },
	{ AP, 0xe0000004f0000000ull, 0xe0000004f0000000ull, N("pmevent"), PM },

	
	// try to print out *some* info.
	{ AP, 0, 0, OOPS, MCDST, LLDST, T(lsw), T(lc2w), T(lc3w) },

};

/*
 * Predicates
 */
struct insn tabp[] = {
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

struct insn tabc[] = {
	{ FP, 0x00000000, 0xf0000000, T(p), N("discard") },
	{ AP, 0x10000000, 0xf0000000, T(p), N("bra"), CTARG },
	{ AP, 0x20000000, 0xf0000000, IGNPRED, N("call"), CTARG },
	{ AP, 0x30000000, 0xf0000000, T(p), N("ret") },
	{ AP, 0x60000000, 0xf0000000, IGNPRED, N("quadon") },
	{ AP, 0x70000000, 0xf0000000, IGNPRED, N("quadpop") },
	{ AP, 0x861ffe00, 0xf61ffe00, IGNPRED, N("bar sync"), BAR },
	{ AP, 0x90000000, 0xf0000000, IGNPRED, N("trap") },
	{ AP, 0xa0000000, 0xf0000000, IGNPRED, N("joinat"), CTARG },
	{ AP, 0xb0000000, 0xf0000000, T(p), N("brkpt") }, // sm_11. check predicates.
	{ AP, 0, 0, T(p), OOPS, CTARG },
};

struct insn tab2w[] = {
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

void nv50dis (FILE *out, uint32_t *code, int num, int ptype) {
	int cur = 0;
	while (cur < num) {
		ull a = code[cur], m = 0;
		fprintf (out, "%s%08x: %s", cgray, cur*4, cnorm);
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
		atomtab (out, &a, &m, tab, ptype);
		a &= ~m;
		if (a & ~1ull) {
			fprintf (out, (a&1?" %s[unknown: %016llx]%s":" %s[unknown: %08llx]%s"), cred, a&~1ull, cnorm);
		}
		printf ("%s\n", cnorm);
	}
}

/*
 * Options:
 *
 *  -v		Disassembles VP code
 *  -g		Disassembles GP code
 *  -f, -p	Disassembles FP code
 *  -c		Disassembles CP code
 *  -s		Disassembles VP, GP, and FP code [currently conflictless]
 *  -a		Disassembles any code, assumes CP in case of conflict [default]
 */

int main(int argc, char **argv) {
	int ptype = AP;
	int c;
	while ((c = getopt (argc, argv, "vgfpcas")) != -1)
		switch (c) {
			case 'v':
				ptype = VP;
				break;
			case 'g':
				ptype = GP;
				break;
			case 'f':
			case 'p':
				ptype = FP;
				break;
			case 'c':
				ptype = CP;
				break;
			case 's':
				ptype = VP|GP|FP;
				break;
			case 'a':
				ptype = AP;
				break;
		}
	int num = 0;
	int maxnum = 16;
	uint32_t *code = malloc (maxnum * 4);
	uint32_t t;
	while (!feof(stdin) && scanf ("%x", &t) == 1) {
		if (num == maxnum) maxnum *= 2, code = realloc (code, maxnum*4);
		code[num++] = t;
		scanf (" ,");
	}
	nv50dis (stdout, code, num, ptype);
	return 0;
}
