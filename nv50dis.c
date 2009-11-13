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
 * Instructions
 * 
 * 0. Conventions
 * 1. Integer ADD family
 * 2. Integer short MUL
 *
 * 0. Conventions
 *
 *   S(x): 31th bit of x for 32-bit x, 15th for 16-bit x.
 *   SEX(x): sign-extension of x
 *   ZEX(x): zero-extension of x
 *
 * 1. Integer ADD family
 *
 *   add [sat] b32/b16 [CDST] DST SRC1 SRC3		O2=0, O1=0
 *   sub [sat] b32/b16 [CDST] DST SRC1 SRC3		O2=0, O1=1
 *   subr [sat] b32/b16 [CDST] DST SRC1 SRC3		O2=1, O1=0
 *   addc [sat] b32/b16 [CDST] DST SRC1 SRC3 COND	O2=1, O1=1
 *
 *   All operands are 32-bit or 16-bit according to size specifier.
 *
 *	b16/b32 s1, s2;
 *	bool c;
 *	switch (OP) {
 *		case add: s1 = SRC1, s2 = SRC2, c = 0; break;
 *		case sub: s1 = SRC1, s2 = ~SRC2, c = 1; break;
 *		case subr: s1 = ~SRC1, s2 = SRC2, c = 1; break;
 *		case addc: s1 = SRC1, s2 = SRC2, c = COND.C; break;
 *	}
 *	res = s1+s2+c;	// infinite precision
 *	CDST.C = res >> (b32 ? 32 : 16);
 *	res = res & (b32 ? 0xffffffff : 0xffff);
 *	CDST.O = (S(s1) == S(s2)) && (S(s1) != S(res));
 *	if (sat && CDST.O)
 *		if (S(res)) res = (b32 ? 0x7fffffff : 0x7fff);
 *		else res = (b32 ? 0x80000000 : 0x8000);
 *	CDST.S = S(res);
 *	CDST.Z = res == 0;
 *	DST = res;
 *
 *   Short/imm:	0x20000000 base opcode
 *		0x10000000 O2 bit
 *		0x00400000 O1 bit
 *		0x00008000 0: b16, 1: b32
 *		0x00000100 sat flag
 *		operands: S*DST, S*SRC1/S*SHARED, S*SRC2/S*CONST/IMM, $c0
 *
 *   Long:	0x20000000 0x00000000 base opcode
 *		0x10000000 0x00000000 O2 bit
 *		0x00400000 0x00000000 O1 bit
 *		0x00000000 0x04000000 0: b16, 1: b32
 *		0x00000000 0x08000000 sat flag
 *		operands: MCDST, L*DST, L*SRC1/L*SHARED, L*SRC3/L*CONST3, COND
 *
 * 2. Integer short MUL
 *
 *   mul [CDST] DST u16/s16 SRC1 u16/s16 SRC2
 *
 *   DST is 32-bit, SRC1 and SRC2 are 16-bit.
 *
 *	b32 s1, s2;
 *	if (src1_signed)
 *		s1 = SEX(SRC1);
 *	else
 *		s1 = ZEX(SRC1);
 *	if (src2_signed)
 *		s2 = SEX(SRC2);
 *	else
 *		s2 = ZEX(SRC2);
 *	b32 res = s1*s2;	// modulo 2^32
 *	CDST.O = 0;
 *	CDST.C = 0;
 *	CDST.S = S(res);
 *	CDST.Z = res == 0;
 *	DST = res;
 *
 *   Short/imm:	0x40000000 base opcode
 *   		0x00008000 src1 is signed
 *   		0x00000100 src2 is signed
 *   		operands: SDST, SHSRC/SHSHARED, SHSRC2/SHCONST/IMM
 *
 *   Long:	0x40000000 0x00000000 base opcode
 *   		0x00000000 0x00008000 src1 is signed
 *   		0x00000000 0x00004000 src2 is signed
 *   		operands: MCDST, LDST, LHSRC1/LHSHARED, LHSRC2/LHCONST2
 *
 * 3. Integer MAD
 *
 *   mad [CDST] DST u16 SRC1 SRC2 SRC3
 *
 */

/*
 * Instructions, from PTX manual:
 *   
 *   1. Integer Arithmetic
 *    - mul		desc...
 *    - mad		desc...
 *    - mul24		XXX
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
 *    - and		desc... XXX imm
 *    - or 		desc...
 *    - xor		desc...
 *    - not		desc...
 *    - cnot		desc...
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
 * Notation for bitfields: 0/4-9 is bits 4-9 of first word, 1/8-9 is bits 8-9
 * of second word.
 *
 * Machine code is read in 64-bit naturaly-aligned chunks, consisting of two
 * 32-bit words. If bit 0 of first word is 0, this chunk consists of two
 * single-word ishort instructions. Otherwise, it's a dobule-word instruction.
 * For double-word instructions, 1/0-1 further determines its type:
 *  00: long
 *  01: long with exit after it
 *  10: long with join before it
 *  11: immediate
 *
 * Short, long, and immediate instructions each have separate encoding tables.
 *
 * All [?] long instructions can be predicated. $c register number to be used
 * is in 1/12-13, 1/7-11 specifies condition to be checked in that register:
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
 * 0/1 is yet another instruction subtype: 0 is normal instruction, 1 is
 * a control instruction [branches and stuff like that]. For all types and
 * subtypes, the major opcode field is 0/28-31. For long instructions, 1/29-31
 * is subopcode. From there on, there seems to be little regularity.
 */

/*
 * Misc. hack alerts:
 *  - 1/12-13 is read twice in addc instructions: it selects $c to use for
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
 *  - same fields, but for the second word, in case of two-word insns
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
 * given as word number [0-based], start bit, size in bits.
 *
 * Also, three simple ops are provided: N("string") prints a literal to output
 * and is supposed to be used for instruction names and various modifiers.
 * OOPS is for unknown encodings, NL is for separating instructions in case
 * a single opcode represents two [only possible with join and exit].
 */
 

#define BF(w, s, l) (m[w] |= ((1<<l)-1<<s), a[w]>>s&(1<<l)-1)
#define RCL(w, s, l) (a[w] &= ~((1<<l)-1<<s))

#define APROTO (FILE *out, uint32_t *a, uint32_t *m, const void *v, int ptype)

typedef void (*afun) APROTO;

struct atom {
	afun fun;
	const void *arg;
};

struct insn {
	int ptype;
	uint32_t val;
	uint32_t mask;
	uint32_t val2;
	uint32_t mask2;
	struct atom atoms[10];
};

#define T(x) atomtab, tab ## x
void atomtab APROTO {
	const struct insn *tab = v;
	int i;
	while ((a[0]&tab->mask) != tab->val || (a[1]&tab->mask2) != tab->val2 || !(tab->ptype&ptype))
		tab++;
	m[0] |= tab->mask;
	m[1] |= tab->mask2;
	for (i = 0; i < 10; i++)
		if (tab->atoms[i].fun)
			tab->atoms[i].fun (out, a, m, tab->atoms[i].arg, ptype);
}

#define N(x) atomname, x
void atomname APROTO {
	fprintf (out, " %s%s", cgr, (char *)v);
}

#define NL atomnl, 0
void atomnl APROTO {
	fprintf (out, "\n                           ");
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
 * segment [which does NOT have to be entry point of code. it is for CUDA, but // XXX ORLY? and CP_START_ID is what?
 * not for shaders]. Low part of target is in 0/11-26, high part is in 1/14-19.
 * It's unknown yet how big this field realy is. Some CUDA manual claims code
 * can be up to 8M PTX instructions long, which would make it 23 bits in total.
 */

#define CTARG atomctarg, 0
void atomctarg APROTO {
	fprintf (out, " %s%#x", cbr, BF(0, 11, 16)<<2 | BF(1, 14, 6) << 18);
}

/*
 * Immediate field
 *
 * Used by all immediate insns [tabi table] for last argument. Full 32-bit.
 */

#define IMM atomimm, 0
void atomimm APROTO {
	fprintf (out, " %s%#x", cyel, (BF(0, 16, 6)) | (BF(1, 2, 26)<<6));
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

int pmoff[] = { 0, 10, 4, 0 };
int baroff[] = { 0, 21, 4, 0 };
int offoff[] = { 0, 9, 16, 0 };
int shcntoff[] = { 0, 16, 5, 0 };
int hshcntoff[] = { 0, 16, 4, 0 };
int toffxoff[] = { 1, 24, 4, 1 };
int toffyoff[] = { 1, 20, 4, 1 };
int toffzoff[] = { 1, 16, 4, 1 };
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
	uint32_t num = BF(n[0], n[1], n[2]);
	if (n[3] && num&1<<(n[2]-1))
		fprintf (out, " %s-%#x", cyel, (1<<n[2]) - num);
	else
		fprintf (out, " %s%#x", cyel, num);
}

/*
 * Register fields
 *
 * There are four locations of register fields for $r:
 *  - DST: 0/2 and up, used for destination reg of most insns, but for source
 *         in store instructions. Insn seems to get ignored if this field is
 *         > number of allocated regs, so don't use that for storing 0s.
 *  - SRC: 0/9 and up, used as a source register in all types of insns.
 *  - SRC2: 0/16 and up, used as a source register in short and long insns.
 *  - SRC3: 1/14 and up, used as s source register in long insns.
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
 * using $o instead of $r is indicated by 1/3 being set. LLDST means either
 * $o or $r in DST. It's unknown yet in what instructions it can be used
 * [probably all longs?] and whether it works for 16-bit halves [$o63h
 * encoding is used as a bit bucket for sure, however].
 *
 * ADST is a smaller version of DST field used for storing $a destination in
 * insns that allow one.
 *
 * COND is $c register used as input, both for predicating and other purposes,
 * like addition with carry or mov from $c.
 *
 * CDST is $c register used for output. It's set according to insn result for
 * most insns that allow it. Writing to $c in this field needs to be enabled
 * by setting 1/6. MCDST is an op that reflects that and is used for insns
 * that can function both with or without $c output.
 * 
 * AREG is a field storing $a register used in insn for memory addressing,
 * or as a normal source register sometimes. It's especially ugly, because
 * it's spread out across 0/26-27 [low part] and 1/2 [high part], with the
 * high part assumed 0 if insn is short or immediate. Also, if two memory
 * accesses are used in the same insn, this field applies only to the first.
 *
 */

int sdstoff[] = { 0, 2, 6, 'r' };
int ldstoff[] = { 0, 2, 7, 'r' };
int ssrcoff[] = { 0, 9, 6, 'r' };
int lsrcoff[] = { 0, 9, 7, 'r' };
int ssrc2off[] = { 0, 16, 6, 'r' };
int lsrc2off[] = { 0, 16, 7, 'r' };
int lsrc3off[] = { 1, 14, 7, 'r' };
int odstoff[] = { 0, 2, 7, 'o' };
int adstoff[] = { 0, 2, 3, 'a' };
int condoff[] = { 1, 12, 2, 'c' };
int c0off[] = { 0, 0, 0, 'c' };
int cdstoff[] = { 1, 4, 2, 'c' };
int texoff[] = { 0, 9, 7, 't' };
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
	int r = BF(n[0], n[1], n[2]);
	if (r == 127 && n[3] == 'o') fprintf (out, " %s_", cbl);
	else fprintf (out, " %s$%c%d", (n[3]=='r')?cbl:cmag, n[3], r);
}
void atomdreg APROTO {
	const int *n = v;
	fprintf (out, " %s$%c%dd", (n[3]=='r')?cbl:cmag, n[3], BF(n[0], n[1], n[2]));
}
void atomqreg APROTO {
	const int *n = v;
	fprintf (out, " %s$%c%dq", (n[3]=='r')?cbl:cmag, n[3], BF(n[0], n[1], n[2]));
}
void atomhreg APROTO {
	const int *n = v;
	int r = BF(n[0], n[1], n[2]);
	if (r == 127 && n[3] == 'o') fprintf (out, " %s_", cbl);
	else fprintf (out, " %s$%c%d%c", (n[3]=='r')?cbl:cmag, n[3], r>>1, "lh"[r&1]);
}

int getareg (uint32_t *a, uint32_t *m, int l) {
	int r = BF(0, 26, 2);
	RCL(0, 26, 2);
	if (l) {
		r |= BF(1, 2, 1)<<2;
		RCL(1, 2, 1);
	}
	return r;
}
#define AREG atomareg, 0
void atomareg APROTO {
	fprintf (out, " %s$a%d", cmag, getareg(a, m, 1));
}

#define LTDST atomltdst, 0
void atomltdst APROTO {
	int base = BF(0, 2, 7);
	int mask = BF(1, 14, 2)<<2 | BF(0, 25, 2);
	int k = 0, i;
	fprintf (out, " %s{", cnorm);
	for (i = 0; i < 4; i++)
		if (mask & 1<<i)
			fprintf (out, " %s$r%d", cbl, base+k++);
		else
			fprintf (out, " %s_", cbl);
	fprintf (out, " %s}", cnorm);
}
#define STDST atomstdst, 0
void atomstdst APROTO {
	int base = BF(0, 2, 7);
	int i;
	fprintf (out, " %s{", cnorm);
	for (i = 0; i < 4; i++)
		fprintf (out, " %s$r%d", cbl, base+i);
	fprintf (out, " %s}", cnorm);
}
#define TSRC atomtsrc, 0
void atomtsrc APROTO {
	int base = BF(0, 2, 7);
	int cnt = BF(0, 22, 2);
	int i;
	fprintf (out, " %s{", cnorm);
	for (i = 0; i <= cnt; i++)
		fprintf (out, " %s$r%d", cbl, base+i);
	fprintf (out, " %s}", cnorm);
}


#define LLDST T(lldst)
struct insn tablldst[] = {
	{ AP, 0, 0, 0, 8, LDST },
	{ AP, 0, 0, 8, 8, ODST },
};
#define LLHDST T(llhdst)
struct insn tabllhdst[] = {
	{ AP, 0, 0, 0, 8, LHDST },
	{ AP, 0, 0, 8, 8, OHDST },
};

#define MCDST T(mcdst)
struct insn tabmcdst[] = {
	{ AP, 0, 0, 0x40, 0x40, CDST },
	{ AP, 0, 0, 0, 0 }
};

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
int ssmem[] = { 0, 9, 4, 2, 's', 5, 0, 0, 0 };		// done
int shsmem[] = { 0, 9, 4, 1, 's', 5, 0, 0, 0 };		// done
int sbsmem[] = { 0, 9, 4, 0, 's', 5, 0, 0, 0 };		// done
int lsmem[] = { 0, 9, 5, 2, 's', 7, 0, 0, 0 };		// done
int lhsmem[] = { 0, 9, 5, 1, 's', 7, 0, 0, 0 };		// done
int lbsmem[] = { 0, 9, 5, 0, 's', 7, 0, 0, 0 };		// done
int fsmem[] = { 0, 9, 14, 2, 's', 7, 0, 0, 0 };		// done
int fhsmem[] = { 0, 9, 15, 1, 's', 7, 0, 0, 0 };	// done
int fbsmem[] = { 0, 9, 16, 0, 's', 7, 0, 0, 0 };	// done
// attr
int samem[] = { 0, 9, 6, 2, 'a', 0, 0, 0, 0 };		// TODO
int lamem[] = { 0, 9, 7, 2, 'a', 0, 0, 0, 0 };		// TODO
int famem[] = { 0, 9, 7, 2, 'a', 3, 0, 0, 0 };		// TODO
// prim
int spmem[] = { 0, 9, 6, 2, 'p', 1, 0, 0, 0 };		// TODO
int lpmem[] = { 0, 9, 7, 2, 'p', 3, 0, 0, 0 };		// TODO
// const
int scmem[] = { 0, 16, 5, 2, 'c', 1, 0, 21, 1 };	// TODO
int shcmem[] = { 0, 16, 5, 1, 'c', 1, 0, 21, 1 };	// TODO
int lcmem2[] = { 0, 16, 7, 2, 'c', 3, 1, 22, 4 };	// TODO
int lhcmem2[] = { 0, 16, 7, 1, 'c', 3, 1, 22, 4 };	// TODO
int lcmem3[] = { 1, 14, 7, 2, 'c', 3, 1, 22, 4 };	// TODO
int lhcmem3[] = { 1, 14, 7, 1, 'c', 3, 1, 22, 4 };	// TODO
int fcmem[] = { 0, 9, 14, 2, 'c', 7, 1, 22, 4 };	// done
int fhcmem[] = { 0, 9, 15, 1, 'c', 7, 1, 22, 4 };	// done
int fbcmem[] = { 0, 9, 16, 0, 'c', 7, 1, 22, 4 };	// done
// local
int lmem[] = { 0, 9, 16, 0, 'l', 7, 0, 0, 0 };		// done
// varying
int svmem[] = { 0, 16, 8, 2, 'v', 1, 0, 0, 0 };		// TODO
int lvmem[] = { 0, 16, 8, 2, 'v', 3, 0, 0, 0 };		// TODO

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
	fprintf (out, " %s%c", ccy, n[4]);
	if (n[8]) fprintf (out, "%d", BF(n[6], n[7], n[8]));
	fprintf (out, "[", ccy);
	int mo = BF(n[0], n[1], n[2])<<n[3];
	int r = (n[5]&1?getareg(a, m, n[5]&2):0);
	if (n[5]&4 && BF(0, 25, 1))  {
		fprintf (out, "%s$a%d%s++", cmag, r, ccy);
		if (mo&(1<<(n[2]+n[3]-1)))
			mo += (1<<16)-(1<<(n[2]+n[3]));
		fprintf (out, "%s%#x%s]", cyel, mo, ccy);
	} else {
		if (r) fprintf (out, "%s$a%d%s+", cmag, r, ccy);
		fprintf (out, "%s%#x%s]", cyel, mo, ccy);
	}
}

int g1mem[] = { 0, 16, 4 };
int g2mem[] = { 0, 23, 4 };
#define GLOBAL atomglobal, g1mem
#define GLOBAL2 atomglobal, g2mem
void atomglobal APROTO {
	const int *n = v;
	fprintf (out, " %sg%d[%s$r%d%s]", ccy, BF(n[0], n[1], n[2]), cbl, BF(0, 9, 7), ccy);
}

struct insn tabss[] = {
	{ GP, 0x01800000, 0x01800000, 0, 0, SPRIM },	// XXX check
	{ CP, 0x00000000, 0x00006000, 0, 0, N("u8"), SBSHARED },
	{ CP, 0x00002000, 0x00006000, 0, 0, N("u16"), SHSHARED },
	{ CP, 0x00004000, 0x00006000, 0, 0, N("s16"), SHSHARED },
	{ CP, 0x00006000, 0x00006000, 0, 0, N("b32"), SSHARED },
	{ VP|GP, 0, 0, 0, 0, SATTR },
	{ AP, 0, 0, 0, 0, OOPS },
};

struct insn tabls[] = {
	{ GP, 0x01800000, 0x01800000, 0, 0, LPRIM },	// XXX check
	{ CP, 0x00000000, 0x0000c000, 0, 0, N("u8"), LBSHARED },
	{ CP, 0x00004000, 0x0000c000, 0, 0, N("u16"), LHSHARED },
	{ CP, 0x00008000, 0x0000c000, 0, 0, N("s16"), LHSHARED },
	{ CP, 0x0000c000, 0x0000c000, 0, 0, N("b32"), LSHARED },
	{ VP|GP, 0, 0, 0, 0, LATTR },
	{ AP, 0, 0, 0, 0, OOPS },
};

/*
 * Short instructions
 *
 * Generally, destination in S*DST, source 1 in S*SRC or S*SHARED,
 * source 2 in S*SRC2 or S*CONST.
 *
 * Usual format:
 *
 * 0x00000003: set to 0
 * 0x000000fc: S*DST
 * 0x00000100: flag1
 * 0x00007e00: S*SRC or S*SHARED
 * 0x00008000: flag2
 * 0x003f0000: S*SRC2 or S*CONST
 * 0x00400000: flag3
 * 0x00800000: use S*CONST
 * 0x01000000: use S*SHARED
 * 0x0e000000: addressing
 * 0xf0000000: opcode
 *
 * Op	flag3	flag2	flag1	op
 *
 * 0	-	-	-	-			crashes in FP
 * 1	?	32/16	?	mov d s1
 * 2	0	32/16	sat	add d s1 s2
 * 2	1	32/16	sat	sub d s1 s2
 * 3	0	32/16	sat	subr d s1 s2
 * 3	1	32/16	sat	addc d s1 s2 $c0
 * 4	0	s/u1	s/u2	mul d s1 s2		d is 32-bit, s? are 16-bit
 * 4	1	s/u	?	mul b24 b32 d s1 s2
 * 5	?	?	?	?
 * 6	?	?	s/u	mad d s1 s2 d		d is 32-bit, s? are 16-bit
 * 7	?	?	?	?
 * 8	uses weird format
 * 9	?	?	?	rcp f32 d s1		check S*SHARED.
 * a	?	?	?	?
 * b	neg2	neg1	sat	add f32 d s1 s2
 * c	neg2	neg1	?	mul f32 d s1 s2
 * d	-	-	-	-			crashes in FP
 * e	?	?	?	mad f32 d, s1, s2, d	check S*SHARED
 * f	uses weird format
 *
 */

struct insn tabssh[] = {
	{ AP, 0x00000000, 0x01000000, 0, 0, SHSRC },
	{ AP, 0x01000000, 0x01000000, 0, 0, T(ss) },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabssw[] = {
	{ AP, 0x00000000, 0x01000000, 0, 0, SSRC },
	{ AP, 0x01000000, 0x01000000, 0, 0, T(ss) },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabsch[] = {
	{ AP, 0x00800000, 0x01800000, 0, 0, SHCONST },
	{ AP, 0x00000000, 0x00000000, 0, 0, SHSRC2 },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabscw[] = {
	{ AP, 0x00800000, 0x01800000, 0, 0, SCONST },
	{ AP, 0x00000000, 0x00000000, 0, 0, SSRC2 },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabssat[] = {
	{ AP, 0x00000000, 0x00000100, 0, 0 },
	{ AP, 0x00000100, 0x00000100, 0, 0, N("sat") },
};

struct insn tabsneg1[] = {
	{ AP, 0x00000000, 0x00008000, 0, 0 },
	{ AP, 0x00008000, 0x00008000, 0, 0, N("neg") },
};

struct insn tabsneg2[] = {
	{ AP, 0x00000000, 0x00400000, 0, 0 },
	{ AP, 0x00400000, 0x00400000, 0, 0, N("neg") },
};

struct insn tabsnot1[] = {
	{ AP, 0x00000000, 0x00400000, 0, 0 },
	{ AP, 0x00400000, 0x00400000, 0, 0, N("not") },
};

struct insn tabas[] = {
	{ AP, 0x00000000, 0x00008000, 0, 0, T(ssat), N("b16"), SHDST, T(ssh), T(sch) },
	{ AP, 0x00008000, 0x00008000, 0, 0, T(ssat), N("b32"), SDST, T(ssw), T(scw) },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabsmus1[] = {
	{ AP, 0x00000000, 0x00000100, 0, 0, N("u16") },
	{ AP, 0x00000100, 0x00000100, 0, 0, N("s16") },
};

struct insn tabms[] = {
	{ AP, 0x00000000, 0x00008100, 0, 0, SDST, N("u16"), T(ssh), T(sch), SDST },
	{ AP, 0x00000100, 0x00008100, 0, 0, SDST, N("s16"), T(ssh), T(sch), SDST },
	{ AP, 0x00008000, 0x00008100, 0, 0, N("sat"), SDST, N("s16"), T(ssh), T(sch), SDST },
	{ AP, 0x00008100, 0x00008100, 0, 0, SDST, N("u24"), T(ssw), T(scw), SDST },
};

struct insn tabsmus2[] = {
	{ AP, 0x00000000, 0x00008000, 0, 0, N("u16") },
	{ AP, 0x00008000, 0x00008000, 0, 0, N("s16") },
};

struct insn tabsm24us[] = {
	{ AP, 0x00000000, 0x00008000, 0, 0, N("u24") },
	{ AP, 0x00008000, 0x00008000, 0, 0, N("s24") },
};

struct insn tabstex[] = {
	{ AP, 0x00000000, 0x00000100, 0, 0, N("all") },
	{ AP, 0x00000100, 0x00000100, 0, 0, N("live") },
};

struct insn tabs[] = {
	// SCAN 0-1
	{ AP, 0x10000000, 0xf0008002, 0, 0, N("mov b16"), SHDST, T(ssh) },
	{ AP, 0x10008000, 0xf0008002, 0, 0, N("mov b32"), SDST, T(ssw) },

	{ AP, 0x20000000, 0xf0400002, 0, 0, N("add"), T(as) },
	{ AP, 0x20400000, 0xf0400002, 0, 0, N("sub"), T(as) },
	{ AP, 0x30000000, 0xf0400002, 0, 0, N("subr"), T(as) },
	{ AP, 0x30400000, 0xf0400002, 0, 0, N("addc"), T(as), C0 },

	{ AP, 0x40000000, 0xf0400002, 0, 0, N("mul"), SDST, T(smus2), T(ssh), T(smus1), T(sch) },
	{ AP, 0x40400000, 0xf0400102, 0, 0, N("mul"), SDST, T(sm24us), T(ssw), T(scw) },
	{ AP, 0x40400100, 0xf0400102, 0, 0, N("mul"), SDST, N("high"), T(sm24us), T(ssw), T(scw) },

	/// SCAN 5
	// XXX sad

	{ AP, 0x60000000, 0xf0400002, 0, 0, N("madd"), T(ms) },
	{ AP, 0x60400000, 0xf0400002, 0, 0, N("msub"), T(ms) },
	{ AP, 0x70000000, 0xf0400002, 0, 0, N("msubr"), T(ms) },
	{ AP, 0x70400000, 0xf0400002, 0, 0, N("maddc"), T(ms), C0 },


	// SCAN 9-F

	{ FP, 0x80000000, 0xf3000102, 0, 0, N("interp"), SDST, SVAR },		// most likely something is wrong with this.
	{ FP, 0x81000000, 0xf3000102, 0, 0, N("interp"), SDST, N("cent"), SVAR },
	{ FP, 0x82000000, 0xf3000102, 0, 0, N("interp"), SDST, SVAR, SSRC },
	{ FP, 0x83000000, 0xf3000102, 0, 0, N("interp"), SDST, N("cent"), SVAR, SSRC },
	{ FP, 0x80000100, 0xf3000102, 0, 0, N("interp"), SDST, N("flat"), SVAR },

	{ AP, 0x90000000, 0xf0000002, 0, 0, N("rcp f32"), SDST, SSRC },

	// cvt ? probably not.

	{ AP, 0xb0000000, 0xf0000002, 0, 0, N("add"), T(ssat), N("f32"), SDST, T(sneg1), T(ssw), T(sneg2), T(scw) },

	{ AP, 0xc0000000, 0xf0000002, 0, 0, N("mul f32"), SDST, T(sneg1), T(ssw), T(sneg2), T(scw) },
	// and

	{ AP, 0xe0000000, 0xf0000002, 0, 0, N("mad f32"), SDST, SSRC, SSRC2, SDST },	// XXX: flags like tabi?

	{ AP, 0xf0000000, 0xf1000002, 0, 0, N("texauto"), T(stex), STDST, TEX, TSRC },
	{ AP, 0xf1000000, 0xf1000002, 0, 0, N("texfetch"), T(stex), STDST, TEX, TSRC },

	{ AP, 0, 2, 0, 0, OOPS, SDST, T(ssw), T(scw) },
	{ AP, 0, 0, 0, 0, OOPS }
};

/*
 * Immediate instructions
 *
 * Destination in S*DST, source in S*SRC or S*SHARED. Except for mov.
 *
 * Usual format:
 *
 * 0x0000000000000003: set to 1
 * 0x00000000000000fc: S*DST
 * 0x0000000000000100: flag1
 * 0x0000000000007e00: S*SRC or S*SHARED
 * 0x0000000000008000: flag2
 * 0x00000000003f0000: IMMD, low part
 * 0x0000000000400000: flag3
 * 0x0000000000800000: ???
 * 0x0000000001000000: use S*SHARED
 * 0x000000000e000000: addressing
 * 0x00000000f0000000: opcode
 * 0x0000000300000000: set to 3
 * 0x0ffffffc00000000: IMMD, high part
 * 0xf000000000000000: ???
 *
 * Op	flag3	flag2	flag1	op
 *
 * 0	?	?	?	?
 * 1	?	32/16	dst-hi	mov d i			uses L*DST instead of S*DST.
 * 2	0	32/16	sat	add d s1 i
 * 2	1	32/16	sat	sub d s1 i
 * 3	0	32/16	sat	subr d s1 i
 * 3	1	32/16	sat	addc d s1 i $c0
 * 4	0	s/u1	s/u2	mul d s1 i		d is 32-bit, s/i are 16-bit
 * 4	1	s/u	hi/lo	mul b24 d s1 i
 * 5	?	?	?	?
 * 6	?	?	s/u	mad d s1 i d		d is 32-bit, s? are 16-bit
 * 7	?	?	?	?
 * 8	?	?	?	?
 * 9	?	?	?	?
 * a	?	?	?	?
 * b	neg2	neg1	sat	add f32 d s1 i
 * c	neg2	neg1	?	mul f32 d s1 i
 * d	not1	0	0	and b32 d s1 i
 * d	not1	0	1	or b32 d s1 i
 * d	not1	1	0	xor b32 d s1 i
 * d	not1	1	1	mov b32 d i		analogous to long... useless
 * e	neg3	neg1	sat	mad f32 d s1 i d
 * f	?	?	?	?
 *
 */

struct insn tabgi[] = {
	{ AP, 0x00000000, 0x00008000, 0, 0, T(ssat), N("b16"), SHDST, T(ssh), IMM },
	{ AP, 0x00008000, 0x00008000, 0, 0, T(ssat), N("b32"), SDST, T(ssw), IMM },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabi[] = {
	// SCAN 0-1
	{ AP, 0x10000000, 0xf0008002, 0, 0, N("mov b16"), LHDST, IMM },
	{ AP, 0x10008000, 0xf0008002, 0, 0, N("mov b32"), LDST, IMM },	// yes. LDST. special case.

	{ AP, 0x20000000, 0xf0400002, 0, 0, N("add"), T(gi) },
	{ AP, 0x20400000, 0xf0400002, 0, 0, N("sub"), T(gi) },
	{ AP, 0x30000000, 0xf0400002, 0, 0, N("subr"), T(gi) },
	{ AP, 0x30400000, 0xf0400002, 0, 0, N("add"), T(gi), C0 },

	// SCAN 4-F
	{ AP, 0x40000000, 0xf0400002, 0, 0, N("mul"), SDST, T(smus2), T(ssh), T(smus1), IMM },

	{ AP, 0x40400000, 0xf0408002, 0, 0, N("mul u24"), SDST, T(ssw), IMM },
	{ AP, 0x40408000, 0xf0408002, 0, 0, N("mul s24"), SDST, T(ssw), IMM },

	{ AP, 0x60000000, 0xf0000002, 0, 0, N("mad"), SDST, T(smus1), T(ssh), IMM, SDST },

	{ AP, 0xb0000000, 0xf0000002, 0, 0, N("add"), T(ssat), N("f32"), SDST, T(sneg1), T(ssw), T(sneg2), IMM },

	{ AP, 0xc0000000, 0xf0000002, 0, 0, N("mul f32"), SDST, T(sneg1), T(ssw), T(sneg2), IMM },

	{ AP, 0xd0000000, 0xf0008102, 0, 0, N("and b32"), SDST, T(snot1), T(ssw), IMM },
	{ AP, 0xd0000100, 0xf0008102, 0, 0, N("or b32"), SDST, T(snot1), T(ssw), IMM },
	{ AP, 0xd0008000, 0xf0008102, 0, 0, N("xor b32"), SDST, T(snot1), T(ssw), IMM },

	{ AP, 0xe0000000, 0xf0000002, 0, 0, N("mad"), T(ssat), N("f32"), SDST, T(sneg1), T(ssw), IMM, T(sneg2), SDST },

	{ AP, 0, 2, 0, 0, OOPS, SDST, T(ssw), IMM },
	{ AP, 0, 0, 0, 0, OOPS }
};

/*
 * Long instructions
 *
 * Mostly, destination is in L*DST or LLDST, sources are in:
 *  - 1: L*SRC or L*SHARED
 *  - 2: L*SRC2 or L*CONST2, or SHCNT for shift insns
 *  - 3: L*SRC3 or L*CONST3
 *
 * Some insns can also set $c registers with MCDST, or read them eith COND.
 *
 * Usual format:
 *
 * 0x0000000000000003: set to 1
 * 0x00000000000001fc: L*DST
 * 0x000000000000fe00: L*SRC or L*SHARED
 * 0x00000000007f0000: L*SRC2 or L*CONST2
 * 0x0000000000800000: use L*CONST2
 * 0x0000000001000000: use L*CONST3
 * 0x000000000e000000: addressing
 * 0x00000000f0000000: opcode
 * 0x0000000300000000: 0 normal, 1 with exit, 2 with join
 * 0x0000000400000000: addressing
 * 0x0000000800000000: $o DST instead of $r
 * 0x0000003000000000: $c reg to set
 * 0x0000004000000000: enable setting that $c.
 * 0x00000f8000000000: predicate condition
 * 0x0000300000000000: $c to use for predicate and/or carry input
 * 0x001fc00000000000: L*SRC3 or L*CONST3
 * 0x0020000000000000: use L*SHARED
 * 0x03c0000000000000: c[] space to use
 * 0x0c00000000000000: misc flags
 * 0x1000000000000000: ??? never seen set
 * 0xe000000000000000: subopcode
 *
 * L*SRC3 and less often L*SRC2 can be replaced by misc flags when not used.
 *
 * Teh opcodes: [subop shifted left to be in alignment with the hexit you see]
 *
 * op	subop	tested	insn
 * 0	0	!FP
 * 0	2		mov d $c
 * 0	4		mov d $a	$a taken from usual addressing field
 * 0	6		mov d sreg	sreg in LSRC3
 * 0	8	!FP
 * 0	a		mov $c s1
 * 0	c		shl $a s1 shcnt	immediate shift count in LSRC2, no special flag
 * 0	e		mov s[] s3	XXX	WTF? does something on FP
 *
 * for mov from sreg: registers are physid, clock, ?, ?, pm0, pm1, pm2, pm3
 *
 * 1	0		mov d s1	lanemask in LSRC3
 * 1	2		mov d c[]	XXX
 * 1	4		mov d s[]	XXX can do lock, new on sm_11?	!FP
 * 1	6		vote $c		LSRC abused for type: 0 uni 1 any 2 all, needs sm_12
 * 1	8		!FP
 * 1	a		!FP
 * 1	c		!FP
 * 1	e		!FP
 *
 * 2	0		add d s1 s3	highest bit of LSRC2 clear, misc for sat, 32/16
 * 			sub d s1 s3	highest bit of LSRC2 set, misc for sat, 32/16
 * 2	2		!FP
 * 2	4		!FP
 * 2	6		!FP
 * 2	8		!FP
 * 2	a		!FP
 * 2	c		!FP
 * 2	e		!FP
 *
 * 3	0		subr d s1 s3	highest bit of LSRC2 clear, misc for sat, 32/16
 * 			addc d s1 s3 $c	highest bit of LSRC2 set, misc for sat, 32/16
 * 3	2		!FP
 * 3	4		!FP
 * 3	6		set d s1 s2	LSRC3 abused for code, misc for s/u and 32/16
 * 3	8		max d s1 s2	misc for s/u and 32/16
 * 3	a		min d s1 s2	misc for s/u and 32/16
 * 3	c		shl d s1 s2	highest bit of LSRC3 for imm s2, misc for 32/16
 * 3	e		shr d s1 s2	highest bit of LSRC3 for imm s2, misc for 32/16, s/u
 *
 * 4	0		mul d s1 s2	LSRC3 abused for subsubop and s/u
 * 			mul24 d s1 s2	same
 * 4	2		
 * 4	4		?
 * 4	6		?
 * 4	8		?
 * 4	a		?
 * 4	c		?
 * 4	e		?
 *
 * 5	0		sad d s1 s2 s3	misc for s/u and 32/16
 * 5	2		!FP
 * 5	4		!FP
 * 5	6		!FP
 * 5	8		!FP
 * 5	a		!FP
 * 5	c		!FP
 * 5	e		!FP
 *
 * 6	0		mad u16 d s1 s2 s3	misc for carry??
 * 6	2		mad s16 d s1 s2 s3
 * 6	4		mad sat s16 s1 s2 s3	[?]
 * 6	6		mad24l u32 d s1 s2 s3
 * 6	8		mad24l s32 d s1 s2 s3
 * 6	a		XXX ??? probably mad24l sat s32.
 * 6	c		mad24h u32 d s1 s2 s3
 * 6	e		mad24h s32 d s1 s2 s3
 *
 * 7	0		mas24h s32 sat d s1 s2 s3
 * 7	2		XXX USED! \
 * 7	4		XXX USED! |
 * 7	6		XXX USED! |
 * 7	8		XXX USED! | seem to give the same result
 * 7	a		XXX USED! |
 * 7	c		XXX USED! |
 * 7	e		XXX USED! /
 *
 * 8	0		interp	weird format
 * 8	2		!FP
 * 8	4		!FP
 * 8	6		!FP
 * 8	8		!FP
 * 8	a		!FP
 * 8	c		!FP
 * 8	e		!FP
 *
 * 9	0		rcp f32 d s1	doesn't work with SHARED
 * 9	2		!FP
 * 9	4		rsqrt f32 d s1	same
 * 9	6		lg2 f32 d s1	same
 * 9	8		sin f32 d s1	same
 * 9	a		cos f32 d s1	same
 * 9	c		ex2 f32 d s1	same
 * 9	e		!FP
 *
 * a	*		cvt
 *
 * b	0		add f32 d s1 s3		misc: neg s1/s2, LSRC2: rounding
 * b	2		XXX USED!
 * b	4		!FP
 * b	6		set f32 d s1 s2		misc: neg, high LSRC3: abs, low LSRC3: cond
 * b	8		max f32 d s1 s2		misc: neg, high LSRC3: abs
 * b	a		min f32 d s1 s2		misc: neg, high LSRC3: abs
 * b	c		pre f32 d s1 s2		low LSRC3: subsubop, 0-sin/cos, 1-ex2.
 * b	e		!FP
 *
 * c	0		mul f32 d s1 s2		misc: neg, LSRC3: rounding
 * c	2		!FP
 * c	4		XXX USED!
 * c	6		XXX USED!
 * c	8		inter-lane stuff	fucked up format
 * c	a		!FP
 * c	c		!FP
 * c	e		!FP
 *
 * d	0		lop d s1 s2	misc: 32/16, LSRC3: op and not's for both args
 * d	2		add $a offs $a	a LEA.
 * d	4		mov d l[]		weird format
 * d	6		mov l[] d		weird format
 * d	8		mov d g[]		weird format
 * d	a		mov g[] d		weird format
 * d	c		atom g[] d		weird format
 * d	e		ld atom d g[] s2 s3	weird format
 *
 * e	0		mad f32 d s1 s2 s3	misc: neg
 * e	2		mad f32 sat d s1 s2 s3	misc: neg
 * e	4		mad f64 d s1 s2 s3	misc: neg, const space abused as rounding
 * e	6		add f64 d s1 s3		misc: neg, LSRC3/2-3: rounding
 * e	8		mul f64 d s1 s2		misc: neg, LSRC3/3-4: rounding
 * e	a		min f64 d s1 s2		misc: neg, high LSRC3: abs
 * e	c		max f64 d s1 s2		misc: neg, high LSRC3: abs
 * e	e		set f64 d s1 s2		misc: neg, high LSRC3: abs, low LSRC3: cond
 *
 * f	0		tex		weird format
 * f	2		tex bias	weird format
 * f	4		tex lod		weird format
 * f	6		tex size	weird format
 * f	8		XXX USED!
 * f	a		XXX USED!
 * f	c		emit,restart
 * f	e		XXX USED!
 *
 */

struct insn tablsh[] = {
	{ AP, 0, 0, 0x00000000, 0x00200000, LHSRC },
	{ AP, 0, 0, 0x00200000, 0x00200000, T(ls) },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tablsw[] = {
	{ AP, 0, 0, 0x00000000, 0x00200000, LSRC },
	{ AP, 0, 0, 0x00200000, 0x00200000, T(ls) },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tablc2h[] = {
	{ AP, 0x00800000, 0x01800000, 0, 0, LHCONST2 },
	{ AP, 0x00000000, 0x00000000, 0, 0, LHSRC2 },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tablc2w[] = {
	{ AP, 0x00800000, 0x01800000, 0, 0, LCONST2 },
	{ AP, 0x00000000, 0x00000000, 0, 0, LSRC2 },
};

struct insn tablc3h[] = {
	{ AP, 0x01000000, 0x01800000, 0, 0, LHCONST3 },
	{ AP, 0x00000000, 0x00000000, 0, 0, LHSRC3 },
};

struct insn tablc3w[] = {
	{ AP, 0x01000000, 0x01800000, 0, 0, LCONST3 },
	{ AP, 0x00000000, 0x00000000, 0, 0, LSRC3 },
};

struct insn tabshcnt[] = {
	{ AP, 0, 0, 0x00000000, 0x00100000, T(lc2w) },
	{ AP, 0, 0, 0x00100000, 0x00100000, SHCNT },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabhshcnt[] = {
	{ AP, 0, 0, 0x00000000, 0x00100000, T(lc2h) },
	{ AP, 0, 0, 0x00100000, 0x00100000, SHCNT },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tablasat[] = {
	{ AP, 0, 0, 0x08000000, 0x08000000, N("sat") },
	{ AP, 0, 0, 0, 0 }
};

struct insn tabla[] = {
	{ AP, 0, 0, 0x00000000, 0x04000000, N("b16"), T(lasat), LHDST, T(lsh), T(lc3h) },
	{ AP, 0, 0, 0x04000000, 0x04000000, N("b32"), T(lasat), MCDST, LLDST, T(lsw), T(lc3w) },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabldstm[] = {
	{ AP, 0, 0, 0x00000000, 0x00e00000, N("u8") },
	{ AP, 0, 0, 0x00200000, 0x00e00000, N("s8") },
	{ AP, 0, 0, 0x00400000, 0x00e00000, N("u16") },
	{ AP, 0, 0, 0x00600000, 0x00e00000, N("s16") },
	{ AP, 0, 0, 0x00800000, 0x00e00000, N("b64") },
	{ AP, 0, 0, 0x00a00000, 0x00e00000, N("b128") },
	{ AP, 0, 0, 0x00c00000, 0x00e00000, N("b32") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabldsto[] = {	// hack alert: reads the bitfield second time.
	{ AP, 0, 0, 0x00000000, 0x00e00000, LDST },
	{ AP, 0, 0, 0x00200000, 0x00e00000, LDST },
	{ AP, 0, 0, 0x00400000, 0x00e00000, LDST },
	{ AP, 0, 0, 0x00600000, 0x00e00000, LDST },
	{ AP, 0, 0, 0x00800000, 0x00e00000, LDDST },
	{ AP, 0, 0, 0x00a00000, 0x00e00000, LQDST },
	{ AP, 0, 0, 0x00c00000, 0x00e00000, LDST },
	{ AP, 0, 0, 0, 0, LDST },
};

struct insn tabldsts2[] = {
	{ AP, 0, 0, 0x00800000, 0x00e00000, LDSRC2 },
	{ AP, 0, 0, 0x00c00000, 0x00e00000, LSRC2 },
	{ AP, 0, 0, 0x00e00000, 0x00e00000, LSRC2 },
	{ AP, 0, 0, 0, 0, OOPS },
};

struct insn tabldsts3[] = {
	{ AP, 0, 0, 0x00800000, 0x00e00000, LDSRC3 },
	{ AP, 0, 0, 0x00c00000, 0x00e00000, LSRC3 },
	{ AP, 0, 0, 0x00e00000, 0x00e00000, LSRC3 },
	{ AP, 0, 0, 0, 0, OOPS },
};

struct insn tabmldsts3[] = {
	{ AP, 0, 0, 0x08, 0x3c, T(ldsts3) },
	{ AP, 0, 0, 0, 0 },
};

struct insn tabraadd[] = {
	{ AP, 0, 0, 0x00800000, 0x00e00000, N("u64") },
	{ AP, 0, 0, 0x00c00000, 0x00e00000, N("u32") },
	{ AP, 0, 0, 0x00e00000, 0x00e00000, N("s32") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabrab[] = {
	{ AP, 0, 0, 0x00800000, 0x00e00000, N("b64") },
	{ AP, 0, 0, 0x00c00000, 0x00e00000, N("b32") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabramm[] = {
	{ AP, 0, 0, 0x00c00000, 0x00e00000, N("u32") },
	{ AP, 0, 0, 0x00e00000, 0x00e00000, N("s32") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabrab32[] = {
	{ AP, 0, 0, 0x00c00000, 0x00e00000, N("b32") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabrau32[] = {
	{ AP, 0, 0, 0x00c00000, 0x00e00000, N("u32") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabredm[] = {
	{ AP, 0, 0, 0x00, 0x3c, N("add"), T(raadd), },
	{ AP, 0, 0, 0x10, 0x3c, N("inc"), T(rau32), },
	{ AP, 0, 0, 0x14, 0x3c, N("dec"), T(rau32), },
	{ AP, 0, 0, 0x18, 0x3c, N("max"), T(ramm), },
	{ AP, 0, 0, 0x1c, 0x3c, N("min"), T(ramm), },
	{ AP, 0, 0, 0x28, 0x3c, N("and"), T(rab32), },
	{ AP, 0, 0, 0x2c, 0x3c, N("or"), T(rab32), },
	{ AP, 0, 0, 0x30, 0x3c, N("xor"), T(rab32), },
	{ AP, 0, 0, 0, 0, OOPS },
};

struct insn tabatomm[] = {
	{ AP, 0, 0, 0x04, 0x3c, N("exch"), T(rab), },
	{ AP, 0, 0, 0x08, 0x3c, N("cas"), T(rab), },
	{ AP, 0, 0, 0, 0, N("ld"), T(redm) },
};

struct insn tabcvtneg[] = {
	{ AP, 0, 0, 0x20000000, 0x20000000, N("neg")  },
	{ AP, 0, 0, 0, 0 },
};

struct insn tabcvtmod[] = {
	{ AP, 0, 0, 0x00100000, 0x00100000, N("abs"), T(cvtneg) },
	{ AP, 0, 0, 0, 0, T(cvtneg) }
};

struct insn tabcvtffsat[] = {
	{ AP, 0, 0, 0x00080000, 0x00080000, N("sat") },
	{ AP, 0, 0, 0, 0 }
};

struct insn tabcvtrnd[] = {
	{ AP, 0, 0, 0x00000000, 0x00060000, N("rn") },
	{ AP, 0, 0, 0x00020000, 0x00060000, N("rm") },
	{ AP, 0, 0, 0x00040000, 0x00060000, N("rp") },
	{ AP, 0, 0, 0x00060000, 0x00060000, N("rz") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabcvtrint[] = {
	{ AP, 0, 0, 0x00000000, 0x00060000, N("rni") },
	{ AP, 0, 0, 0x00020000, 0x00060000, N("rmi") },
	{ AP, 0, 0, 0x00040000, 0x00060000, N("rpi") },
	{ AP, 0, 0, 0x00060000, 0x00060000, N("rzi") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabaf64r[] = {
	{ AP, 0x00000000, 0x00030000, 0, 0, N("rn") },
	{ AP, 0x00010000, 0x00030000, 0, 0, N("rm") },
	{ AP, 0x00020000, 0x00030000, 0, 0, N("rp") },
	{ AP, 0x00030000, 0x00030000, 0, 0, N("rz") },
};

struct insn tabaf32r[] = {
	{ AP, 0x00000000, 0x00030000, 0, 0, N("rn") },
	{ AP, 0x00030000, 0x00030000, 0, 0, N("rz") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabmf32r[] = {
	{ AP, 0, 0, 0x00000000, 0x0000c000, N("rn") },
	{ AP, 0, 0, 0x0000c000, 0x0000c000, N("rz") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabmad64r[] = {
	{ AP, 0, 0, 0x00000000, 0x00c00000, N("rn") },
	{ AP, 0, 0, 0x00400000, 0x00c00000, N("rm") },
	{ AP, 0, 0, 0x00800000, 0x00c00000, N("rp") },
	{ AP, 0, 0, 0x00c00000, 0x00c00000, N("rz") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabnot1[] = {
	{ AP, 0, 0, 0x00010000, 0x00010000, N("not") },
	{ AP, 0, 0, 0, 0 }
};

struct insn tabnot2[] = {
	{ AP, 0, 0, 0x00020000, 0x00020000, N("not") },
	{ AP, 0, 0, 0, 0 }
};

struct insn tabseti[] = {
	{ AP, 0, 0, 0x00004000, 0x0003c000, N("lt") },
	{ AP, 0, 0, 0x00008000, 0x0003c000, N("eq") },
	{ AP, 0, 0, 0x0000c000, 0x0003c000, N("le") },
	{ AP, 0, 0, 0x00010000, 0x0003c000, N("gt") },
	{ AP, 0, 0, 0x00014000, 0x0003c000, N("ne") },
	{ AP, 0, 0, 0x00018000, 0x0003c000, N("ge") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabsetf[] = {
	{ AP, 0, 0, 0x00004000, 0x0003c000, N("lt") },
	{ AP, 0, 0, 0x00008000, 0x0003c000, N("eq") },
	{ AP, 0, 0, 0x0000c000, 0x0003c000, N("le") },
	{ AP, 0, 0, 0x00010000, 0x0003c000, N("gt") },
	{ AP, 0, 0, 0x00014000, 0x0003c000, N("ne") },
	{ AP, 0, 0, 0x00018000, 0x0003c000, N("ge") },
	{ AP, 0, 0, 0x0001c000, 0x0003c000, N("num") },
	{ AP, 0, 0, 0x00020000, 0x0003c000, N("nan") },
	{ AP, 0, 0, 0x00024000, 0x0003c000, N("ltu") },
	{ AP, 0, 0, 0x00028000, 0x0003c000, N("equ") },
	{ AP, 0, 0, 0x0002c000, 0x0003c000, N("leu") },
	{ AP, 0, 0, 0x00030000, 0x0003c000, N("gtu") },
	{ AP, 0, 0, 0x00034000, 0x0003c000, N("neu") },
	{ AP, 0, 0, 0x00038000, 0x0003c000, N("geu") },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tablneg1[] = {
	{ AP, 0, 0, 0x04000000, 0x04000000, N("neg") },
	{ AP, 0, 0, 0, 0 }
};

struct insn tablneg2[] = {
	{ AP, 0, 0, 0x08000000, 0x08000000, N("neg") },
	{ AP, 0, 0, 0, 0 }
};

struct insn tablfm1[] = {
	{ AP, 0, 0, 0x00100000, 0x00100000, N("abs"), T(lneg1) },
	{ AP, 0, 0, 0, 0, T(lneg1) }
};

struct insn tablfm2[] = {
	{ AP, 0, 0, 0x00080000, 0x00080000, N("abs"), T(lneg2) },
	{ AP, 0, 0, 0, 0, T(lneg2) }
};

struct insn tabcvtiisrc[] ={
	{ AP, 0, 0, 0x00000000, 0x0001c000, N("u16"), T(lsh) },
	{ AP, 0, 0, 0x00004000, 0x0001c000, N("u32"), T(lsw) },
	{ AP, 0, 0, 0x00008000, 0x0001c000, N("u8"), T(lsh) },
	{ AP, 0, 0, 0x0000c000, 0x0001c000, N("u8"), LSRC },	// what about mem?
	{ AP, 0, 0, 0x00010000, 0x0001c000, N("s16"), T(lsh) },
	{ AP, 0, 0, 0x00014000, 0x0001c000, N("s32"), T(lsw) },
	{ AP, 0, 0, 0x00018000, 0x0001c000, N("s8"), T(lsh) },
	{ AP, 0, 0, 0x0001c000, 0x0001c000, N("s8"), LSRC },	// what about mem?
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tabfcon[] = {
	{ AP, 0, 0, 0x00000000, 0x0000c000, N("u8"), FBCONST },
	{ AP, 0, 0, 0x00004000, 0x0000c000, N("u16"), FHCONST },
	{ AP, 0, 0, 0x00008000, 0x0000c000, N("s16"), FHCONST },
	{ AP, 0, 0, 0x0000c000, 0x0000c000, N("b32"), FCONST },
	{ AP, 0, 0, 0, 0, OOPS }
};

struct insn tablmus1[] = {
	{ AP, 0, 0, 0x00000000, 0x00008000, N("u16") },
	{ AP, 0, 0, 0x00008000, 0x00008000, N("s16") },
};

struct insn tabdtex[] = { // suspected to enable implicit derivatives on non-FPs.
	{ AP, 0, 0, 0x00000000, 0x00000008 },
	{ AP, 0, 0, 0x00000008, 0x00000008, N("deriv") },
};

struct insn tabltex[] = {
	{ AP, 0, 0, 0x00000000, 0x00000004, N("all"), T(dtex) },
	{ AP, 0, 0, 0x00000004, 0x00000004, N("live"), T(dtex) },
};


struct insn tablmus2[] = {
	{ AP, 0, 0, 0x00000000, 0x00004000, N("u16") },
	{ AP, 0, 0, 0x00004000, 0x00004000, N("s16") },
};

struct insn tablane[] = {
	{ AP, 0, 0, 0x00000000, 0x0003c000, N("lnone") },
	{ AP, 0, 0, 0x00004000, 0x0003c000, N("l0") },
	{ AP, 0, 0, 0x00008000, 0x0003c000, N("l1") },
	{ AP, 0, 0, 0x0000c000, 0x0003c000, N("l01") },
	{ AP, 0, 0, 0x00010000, 0x0003c000, N("l2") },
	{ AP, 0, 0, 0x00014000, 0x0003c000, N("l02") },
	{ AP, 0, 0, 0x00018000, 0x0003c000, N("l12") },
	{ AP, 0, 0, 0x0001c000, 0x0003c000, N("l012") },
	{ AP, 0, 0, 0x00020000, 0x0003c000, N("l3") },
	{ AP, 0, 0, 0x00024000, 0x0003c000, N("l03") },
	{ AP, 0, 0, 0x00028000, 0x0003c000, N("l13") },
	{ AP, 0, 0, 0x0002c000, 0x0003c000, N("l013") },
	{ AP, 0, 0, 0x00030000, 0x0003c000, N("l23") },
	{ AP, 0, 0, 0x00034000, 0x0003c000, N("l023") },
	{ AP, 0, 0, 0x00038000, 0x0003c000, N("l123") },
	{ AP, 0, 0, 0x0003c000, 0x0003c000 },
};

struct insn tabqs1[] = {
	{ AP, 0x00000000, 0x00030000, 0, 0, N("l0") },
	{ AP, 0x00010000, 0x00030000, 0, 0, N("l1") },
	{ AP, 0x00020000, 0x00030000, 0, 0, N("l2") },
	{ AP, 0x00030000, 0x00030000, 0, 0, N("l3") },
	{ AP, 0, 0, 0, 0 },
};

struct insn tabqop0[] = {
	{ AP, 0, 0, 0x00000000, 0x0c000000, N("add") },
	{ AP, 0, 0, 0x04000000, 0x0c000000, N("subr") },
	{ AP, 0, 0, 0x08000000, 0x0c000000, N("sub") },
	{ AP, 0, 0, 0x0c000000, 0x0c000000, N("mov2") },
};

struct insn tabqop1[] = {
	{ AP, 0, 0, 0x00000000, 0x03000000, N("add") },
	{ AP, 0, 0, 0x01000000, 0x03000000, N("subr") },
	{ AP, 0, 0, 0x02000000, 0x03000000, N("sub") },
	{ AP, 0, 0, 0x03000000, 0x03000000, N("mov2") },
};

struct insn tabqop2[] = {
	{ AP, 0, 0, 0x00000000, 0x00c00000, N("add") },
	{ AP, 0, 0, 0x00400000, 0x00c00000, N("subr") },
	{ AP, 0, 0, 0x00800000, 0x00c00000, N("sub") },
	{ AP, 0, 0, 0x00c00000, 0x00c00000, N("mov2") },
};

struct insn tabqop3[] = {
	{ AP, 0x00000000, 0x00300000, 0, 0, N("add") },
	{ AP, 0x00100000, 0x00300000, 0, 0, N("subr") },
	{ AP, 0x00200000, 0x00300000, 0, 0, N("sub") },
	{ AP, 0x00300000, 0x00300000, 0, 0, N("mov2") },
};

struct insn tabl[] = {
	// 0
	{ VP|GP, 0x00000000, 0xf0000002, 0x04200000, 0xe4200000,
		T(lane), N("mov b32"), LLDST, FATTR },
	{ AP, 0x00000000, 0xf0000002, 0x20000000, 0xe0000000,
		N("mov"), LDST, COND },
	{ AP, 0x00000000, 0xf0000002, 0x40000000, 0xe0000000,	// no, OFFS doesn't work.
		N("mov b32"), LDST, AREG },

	{ AP, 0x00000000, 0xf0000002, 0x60000000, 0xe001c000,
		N("mov b32"), LDST, N("physid") },
	{ AP, 0x00000000, 0xf0000002, 0x60004000, 0xe001c000,
		N("mov b32"), LDST, N("clock") },
	{ AP, 0x00000000, 0xf0000002, 0x60010000, 0xe001c000,
		N("mov b32"), LDST, N("pm0") },
	{ AP, 0x00000000, 0xf0000002, 0x60014000, 0xe001c000,
		N("mov b32"), LDST, N("pm1") },
	{ AP, 0x00000000, 0xf0000002, 0x60018000, 0xe001c000,
		N("mov b32"), LDST, N("pm2") },
	{ AP, 0x00000000, 0xf0000002, 0x6001c000, 0xe001c000,
		N("mov b32"), LDST, N("pm3") },

	{ AP, 0x00000000, 0xf0000002, 0xa0000040, 0xe0000040,
		N("mov"), CDST, LSRC },
	{ AP, 0x00000000, 0xf0000002, 0xc0000000, 0xe0000000,
		N("shl b32"),	ADST, T(lsw), HSHCNT },
	
	{ CP, 0x00000000, 0xf0000002, 0xe0000000, 0xe4400000,	// XXX ok, seriously, what's up with all thse flags?
		N("mov b16"), FHSHARED, LHSRC3 },
	{ CP, 0x00000000, 0xf0000002, 0xe0400000, 0xe4400000,
		N("mov b8"), FBSHARED, LHSRC3 },
	{ CP, 0x00000000, 0xf0000002, 0xe4200000, 0xe4a00000,
		N("mov b32"), FSHARED, LSRC3 },
	{ CP, 0x00000000, 0xf0000002, 0xe4a00000, 0xe4a00000,
		N("mov unlock b32"), FSHARED, LSRC3 },

	// 1
	{ AP, 0x10000000, 0xf0000002, 0x00000000, 0xe4000000,
		T(lane), N("mov b16"), LLHDST, T(lsh) },
	{ AP, 0x10000000, 0xf0000002, 0x04000000, 0xe4000000,
		T(lane), N("mov b32"), LLDST, T(lsw) },

	{ AP, 0x10000000, 0xf0000002, 0x20000000, 0xe4000000,
		N("mov b16"), LLHDST, T(fcon) },
	{ AP, 0x10000000, 0xf0000002, 0x24000000, 0xe4000000,
		N("mov b32"), LLDST, T(fcon) },

	{ CP, 0x10000000, 0xf0000002, 0x40004000, 0xe400c000,	// sm_11. ptxas inexplicably starts using
		N("mov u16"), LHDST, LHSHARED },		// these forms instead of the other one
	{ CP, 0x10000000, 0xf0000002, 0x40008000, 0xe400c000,	// on >=sm_11. XXX check length XXX mode
		N("mov s16"), LHDST, LHSHARED },
	{ CP, 0x10000000, 0xf0000002, 0x4400c000, 0xe480c000,	// getting ridiculous.
		N("mov b32"), LDST, LSHARED },
	{ CP, 0x10000000, 0xf0000002, 0x4480c040, 0xe480c040,
		N("mov lock b32"), CDST, LDST, LSHARED },

	{ AP, 0x10000000, 0xf0000602, 0x60000040, 0xe0000040,
		N("vote uni"), CDST },	// sm_12
	{ AP, 0x10000200, 0xf0000602, 0x60000040, 0xe0000040,
		N("vote any"), CDST },	// sm_12
	{ AP, 0x10000400, 0xf0000602, 0x60000040, 0xe0000040,
		N("vote all"), CDST },	// sm_12

	// 2
	{ AP, 0x20000000, 0xf0400002, 0x00000000, 0x00000000,
		N("add"), T(la) },
	{ AP, 0x20400000, 0xf0400002, 0x00000000, 0x00000000,
		N("sub"), T(la) },

	// 3
	{ AP, 0x30000000, 0xf0400002, 0x00000000, 0xe0000000,
		N("subr"), T(la) },
	{ AP, 0x30400000, 0xf0400002, 0x00000000, 0xe0000000,
		N("addc"), T(la), COND },

	// YARLY.
	{ AP, 0x30000000, 0xf0000002, 0x60000000, 0xec000000,
		N("set"), T(seti), N("u16"), MCDST, LLHDST, T(lsh), T(lc2h) },
	{ AP, 0x30000000, 0xf0000002, 0x68000000, 0xec000000,
		N("set"), T(seti), N("s16"), MCDST, LLHDST, T(lsh), T(lc2h) },
	{ AP, 0x30000000, 0xf0000002, 0x64000000, 0xec000000,
		N("set"), T(seti), N("u32"), MCDST, LLDST, T(lsw), T(lc2w) },
	{ AP, 0x30000000, 0xf0000002, 0x6c000000, 0xec000000,
		N("set"), T(seti), N("s32"), MCDST, LLDST, T(lsw), T(lc2w) },

	{ AP, 0x30000000, 0xf0000002, 0x80000000, 0xec000000,
		N("max u16"), LHDST, T(lsh), T(lc2h) },
	{ AP, 0x30000000, 0xf0000002, 0x88000000, 0xec000000,
		N("max s16"), LHDST, T(lsh), T(lc2h) },
	{ AP, 0x30000000, 0xf0000002, 0x84000000, 0xec000000,
		N("max u32"), LDST, T(lsw), T(lc2w) },
	{ AP, 0x30000000, 0xf0000002, 0x8c000000, 0xec000000,
		N("max s32"), LDST, T(lsw), T(lc2w) },

	{ AP, 0x30000000, 0xf0000002, 0xa0000000, 0xec000000,
		N("min u16"), LHDST, T(lsh), T(lc2h) },
	{ AP, 0x30000000, 0xf0000002, 0xa8000000, 0xec000000,
		N("min s16"), LHDST, T(lsh), T(lc2h) },
	{ AP, 0x30000000, 0xf0000002, 0xa4000000, 0xec000000,
		N("min u32"), LDST, T(lsw), T(lc2w) },
	{ AP, 0x30000000, 0xf0000002, 0xac000000, 0xec000000,
		N("min s32"), LDST, T(lsw), T(lc2w) },

	{ AP, 0x30000000, 0xf0000002, 0xc0000000, 0xe4000000,	// XXX FUCK ALERT: shift count *CAN* be 16-bit.
		N("shl b16"), LHDST, T(lsh), T(hshcnt) },
	{ AP, 0x30000000, 0xf0000002, 0xc4000000, 0xe4000000,
		N("shl b32"), LDST, T(lsw), T(shcnt) },
	{ AP, 0x30000000, 0xf0000002, 0xe0000000, 0xec000000,
		N("shr u16"), LHDST, T(lsh), T(hshcnt) },
	{ AP, 0x30000000, 0xf0000002, 0xe4000000, 0xec000000,
		N("shr u32"), LDST, T(lsw), T(shcnt) },
	{ AP, 0x30000000, 0xf0000002, 0xe8000000, 0xec000000,
		N("shr s16"), LHDST, T(lsh), T(hshcnt) },
	{ AP, 0x30000000, 0xf0000002, 0xec000000, 0xec000000,
		N("shr s32"), LDST, T(lsw), T(shcnt) },


	// 4
	{ AP, 0x40000000, 0xf0000002, 0x00000000, 0xe0010000,
		N("mul"), MCDST, LDST, T(lmus1), T(lsh), T(lmus2), T(lc2h) },
	{ AP, 0x40000000, 0xf0000002, 0x00010000, 0xe003c000,
		N("mul u24"), LDST, T(lsw), T(lc2w) },
	{ AP, 0x40000000, 0xf0000002, 0x00014000, 0xe003c000,
		N("mul high u24"), LDST, T(lsw), T(lc2w) },
	{ AP, 0x40000000, 0xf0000002, 0x00018000, 0xe003c000,
		N("mul s24"), LDST, T(lsw), T(lc2w) },
	{ AP, 0x40000000, 0xf0000002, 0x0001c000, 0xe003c000,
		N("mul high s24"), LDST, T(lsw), T(lc2w) },

	// 5
	{ AP, 0x50000000, 0xf0000002, 0x00000000, 0xec000000,
		N("sad u16"), LDST, T(lsh), T(lc2h), T(lc3w) },
	{ AP, 0x50000000, 0xf0000002, 0x04000000, 0xec000000,
		N("sad u32"), LDST, T(lsw), T(lc2w), T(lc3w) },
	{ AP, 0x50000000, 0xf0000002, 0x08000000, 0xec000000,
		N("sad s16"), LDST, T(lsh), T(lc2h), T(lc3w) },
	{ AP, 0x50000000, 0xf0000002, 0x0c000000, 0xec000000,
		N("sad s32"), LDST, T(lsw), T(lc2w), T(lc3w) },

	// 6
	{ AP, 0x60000000, 0xf0000002, 0x00000000, 0xec000000,
		N("mad u16"), MCDST, LLDST, T(lsh), T(lc2h), T(lc3w) },
	{ AP, 0x60000000, 0xf0000002, 0x0c000000, 0xec000000,
		N("mad u16"), MCDST, LLDST, T(lsh), T(lc2h), T(lc3w), COND },
	{ AP, 0x60000000, 0xf0000002, 0x20000000, 0xe0000000,
		N("mad s16"), MCDST, LLDST, T(lsh), T(lc2h), T(lc3w) },
	{ AP, 0x60000000, 0xf0000002, 0x60000000, 0xe0000000,
		N("mad u24"), LDST, T(lsw), T(lc2w), T(lc3w) },
	{ AP, 0x60000000, 0xf0000002, 0x80000000, 0xe0000000,
		N("mad s24"), LDST, T(lsw), T(lc2w), T(lc3w) },
	{ AP, 0x60000000, 0xf0000002, 0xc0000000, 0xe0000000,
		N("mad high u24"), LDST, T(lsw), T(lc2w), T(lc3w) },
	{ AP, 0x60000000, 0xf0000002, 0xe0000000, 0xe0000000,
		N("mad high s24"), LDST, T(lsw), T(lc2w), T(lc3w) },

	// 7
	{ AP, 0x70000000, 0xf0000002, 0x00000000, 0xe0000000,
		N("mad high s24 sat"), LDST, T(lsw), T(lc2w), T(lc3w) },

	// 8
	{ FP, 0x80000000, 0xf0000002, 0x00000000, 0x00070000,
		N("interp"), LDST, LVAR },
	{ FP, 0x80000000, 0xf0000002, 0x00010000, 0x00070000,
		N("interp"), LDST, N("cent"), LVAR },
	{ FP, 0x80000000, 0xf0000002, 0x00020000, 0x00070000,
		N("interp"), LDST, LVAR, LSRC },
	{ FP, 0x80000000, 0xf0000002, 0x00030000, 0x00070000,
		N("interp"), LDST, N("cent"), LVAR, LSRC },
	{ FP, 0x80000000, 0xf0000002, 0x00040000, 0x00070000,
		N("interp"), LDST, N("flat"), LVAR },

	// 9
	{ AP, 0x90000000, 0xf0000002, 0x00000000, 0xe0000000,
		N("rcp f32"), LLDST, LSRC },
	{ AP, 0x90000000, 0xf0000002, 0x40000000, 0xe0000000,
		N("rsqrt f32"),  LLDST, LSRC },
	{ AP, 0x90000000, 0xf0000002, 0x60000000, 0xe0000000,
		N("lg2 f32"), LLDST, LSRC },
	{ AP, 0x90000000, 0xf0000002, 0x80000000, 0xe0000000,
		N("sin f32"), LLDST, LSRC },
	{ AP, 0x90000000, 0xf0000002, 0xa0000000, 0xe0000000,
		N("cos f32"), LLDST, LSRC },
	{ AP, 0x90000000, 0xf0000002, 0xc0000000, 0xe0000000,
		N("ex2 f32"), LLDST, LSRC },

	// a
	{ AP, 0xa0000000, 0xf0000002, 0xc0000000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtffsat), N("f16"), LHDST, N("f16"), T(lsh) },
	{ AP, 0xa0000000, 0xf0000002, 0xc8000000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtffsat), T(cvtrint), N("f16"), LHDST, N("f16"), T(lsh) },
	{ AP, 0xa0000000, 0xf0000002, 0xc0004000, 0xc4404000,
		N("cvt"), T(cvtmod), T(cvtffsat), T(cvtrnd), N("f16"), LHDST, N("f32"), T(lsw) },
	{ AP, 0xa0000000, 0xf0000002, 0xc4004000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtffsat), N("f32"), MCDST, LLDST, N("f32"), T(lsw) },
	{ AP, 0xa0000000, 0xf0000002, 0xcc004000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtffsat), T(cvtrint), N("f32"), LDST, N("f32"), T(lsw) },
	{ AP, 0xa0000000, 0xf0000002, 0xc4000000, 0xc4404000,
		N("cvt"), T(cvtmod), T(cvtffsat), N("f32"), LDST, N("f16"), T(lsh) },

	{ AP, 0xa0000000, 0xf0000002, 0xc0404000, 0xc4404000,
		N("cvt"), T(cvtmod), T(cvtrnd), N("f32"), LDST, N("f64"), LDSRC },
	{ AP, 0xa0000000, 0xf0000002, 0xc4404000, 0xcc404000,
		N("cvt"), T(cvtmod), N("f64"), LDDST, N("f64"), LDSRC },
	{ AP, 0xa0000000, 0xf0000002, 0xcc404000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("f64"), LDDST, N("f64"), LDSRC },
	{ AP, 0xa0000000, 0xf0000002, 0xc4400000, 0xc4404000,
		N("cvt"), T(cvtmod), N("f64"), LDDST, N("f32"), T(lsw) },

	{ AP, 0xa0000000, 0xf0000002, 0x80000000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("u16"), LHDST, N("f16"), T(lsh) },
	{ AP, 0xa0000000, 0xf0000002, 0x80004000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("u16"), LHDST, N("f32"), T(lsw) },
	{ AP, 0xa0000000, 0xf0000002, 0x88000000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("s16"), LHDST, N("f16"), T(lsh) },
	{ AP, 0xa0000000, 0xf0000002, 0x88004000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("s16"), LHDST, N("f32"), T(lsw) },
	{ AP, 0xa0000000, 0xf0000002, 0x84000000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("u32"), LDST, N("f16"), T(lsh) },
	{ AP, 0xa0000000, 0xf0000002, 0x84004000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("u32"), LDST, N("f32"), T(lsw) },
	{ AP, 0xa0000000, 0xf0000002, 0x8c000000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("s32"), LDST, N("f16"), T(lsh) },
	{ AP, 0xa0000000, 0xf0000002, 0x8c004000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("s32"), LDST, N("f32"), T(lsw) },

	{ AP, 0xa0000000, 0xf0000002, 0x80404000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("u32"), LDST, N("f64"), LDSRC },
	{ AP, 0xa0000000, 0xf0000002, 0x88404000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("s32"), LDST, N("f64"), LDSRC },
	{ AP, 0xa0000000, 0xf0000002, 0x84400000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("u64"), LDDST, N("f32"), T(lsw) },
	{ AP, 0xa0000000, 0xf0000002, 0x84404000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("u64"), LDDST, N("f64"), LDSRC },
	{ AP, 0xa0000000, 0xf0000002, 0x8c400000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("s64"), LDDST, N("f32"), T(lsw) },
	{ AP, 0xa0000000, 0xf0000002, 0x8c404000, 0xcc404000,
		N("cvt"), T(cvtmod), T(cvtrint), N("s64"), LDDST, N("f64"), LDSRC },

	{ AP, 0xa0000000, 0xf0000002, 0x40000000, 0xc4400000,
		N("cvt"), T(cvtmod), T(cvtffsat), T(cvtrnd), N("f16"), LHDST, T(cvtiisrc) },
	{ AP, 0xa0000000, 0xf0000002, 0x44000000, 0xc4400000,
		N("cvt"), T(cvtmod), T(cvtffsat), T(cvtrnd), N("f32"), LDST, T(cvtiisrc) },

	{ AP, 0xa0000000, 0xf0000002, 0x44400000, 0xc4414000,
		N("cvt"), T(cvtmod), T(cvtffsat), T(cvtrnd), N("f64"), LDDST, N("u32"), T(lsw) },
	{ AP, 0xa0000000, 0xf0000002, 0x44410000, 0xc4414000,
		N("cvt"), T(cvtmod), T(cvtffsat), T(cvtrnd), N("f64"), LDDST, N("s32"), T(lsw) },
	{ AP, 0xa0000000, 0xf0000002, 0x40404000, 0xc4414000,
		N("cvt"), T(cvtmod), T(cvtffsat), T(cvtrnd), N("f32"), LDST, N("u64"), LDSRC },
	{ AP, 0xa0000000, 0xf0000002, 0x40414000, 0xc4414000,
		N("cvt"), T(cvtmod), T(cvtffsat), T(cvtrnd), N("f32"), LDST, N("s64"), LDSRC },
	{ AP, 0xa0000000, 0xf0000002, 0x44404000, 0xc4414000,
		N("cvt"), T(cvtmod), T(cvtffsat), T(cvtrnd), N("f64"), LDDST, N("u64"), LDSRC },
	{ AP, 0xa0000000, 0xf0000002, 0x44414000, 0xc4414000,
		N("cvt"), T(cvtmod), T(cvtffsat), T(cvtrnd), N("f64"), LDDST, N("s64"), LDSRC },

	{ AP, 0xa0000000, 0xf0000002, 0x00000000, 0xcc080000,
		N("cvt"), T(cvtmod), N("u16"), MCDST, LLHDST, T(cvtiisrc) },
	{ AP, 0xa0000000, 0xf0000002, 0x00080000, 0xcc080000,
		N("cvt"), T(cvtmod), N("u8"), LHDST, T(cvtiisrc) },
	{ AP, 0xa0000000, 0xf0000002, 0x04000000, 0xcc080000,
		N("cvt"), T(cvtmod), N("u32"), MCDST, LLDST, T(cvtiisrc) },
	{ AP, 0xa0000000, 0xf0000002, 0x04080000, 0xcc080000,
		N("cvt"), T(cvtmod), N("u8"), LDST, T(cvtiisrc) },
	{ AP, 0xa0000000, 0xf0000002, 0x08000000, 0xcc080000,
		N("cvt"), T(cvtmod), N("s16"), MCDST, LLHDST, T(cvtiisrc) },
	{ AP, 0xa0000000, 0xf0000002, 0x08080000, 0xcc080000,
		N("cvt"), T(cvtmod), N("s8"), LHDST, T(cvtiisrc) },
	{ AP, 0xa0000000, 0xf0000002, 0x0c000000, 0xcc080000,
		N("cvt"), T(cvtmod), N("s32"), MCDST, LLDST, T(cvtiisrc) },
	{ AP, 0xa0000000, 0xf0000002, 0x0c080000, 0xcc080000,
		N("cvt"), T(cvtmod), N("s8"), LDST, T(cvtiisrc) },


	// b
	{ AP, 0xb0000000, 0xf0000002, 0x00000000, 0xe0000000,
		N("add"), T(af32r),  N("f32"), LLDST, T(lneg1), T(lsw), T(lneg2), T(lc3w) },
	{ AP, 0xb0000000, 0xf0000002, 0x80000000, 0xe0000000,
		N("max f32"), LLDST, T(lfm1), T(lsw), T(lfm2), T(lc2w) },
	{ AP, 0xb0000000, 0xf0000002, 0xa0000000, 0xe0000000,
		N("min f32"), LLDST, T(lfm1), T(lsw), T(lfm2), T(lc2w) },

	{ AP, 0xb0000000, 0xf0000002, 0x60000000, 0xe0000000,
		N("set"), T(setf), N("f32"), MCDST, LLDST, T(lfm1), T(lsw), T(lfm2), T(lc2w) },

	{ AP, 0xb0000000, 0xf0000002, 0xc0000000, 0xe0004000,
		N("presin f32"), LDST, T(lsw) },
	{ AP, 0xb0000000, 0xf0000002, 0xc0004000, 0xe0004000,
		N("preex2 f32"), LDST, T(lsw) },

	// c
	{ AP, 0xc0000000, 0xf0000002, 0x00000000, 0xe0000000,
		N("mul"), T(mf32r), N("f32"), LLDST, T(lneg1), T(lsw), T(lneg2), T(lc2w) },

	{ AP, 0xc0000000, 0xf0040002, 0x80000000, 0xf0000000,
		N("quadop f32"), T(qop0), T(qop1), T(qop2), T(qop3), MCDST, LLDST, T(qs1), LSRC, LSRC3 },

	{ AP, 0xc0140000, 0xf0150002, 0x89800000, 0x8bc00000,	// XXX fuck me harder.
		N("dfdx f32"), LDST, LSRC, LSRC3 },
	{ AP, 0xc0150000, 0xf0150002, 0x8a400000, 0x8bc00000,
		N("dfdy f32"), LDST, LSRC, LSRC3 },

	// d
	{ AP, 0xd0000000, 0xf0000002, 0x00000000, 0xe400c000,
		N("and b16"), MCDST, LLHDST, T(not1), T(lsh), T(not2), T(lc2h) },
	{ AP, 0xd0000000, 0xf0000002, 0x04000000, 0xe400c000,
		N("and b32"), MCDST, LLDST, T(not1), T(lsw), T(not2), T(lc2w) },
	{ AP, 0xd0000000, 0xf0000002, 0x00004000, 0xe400c000,
		N("or b16"), MCDST, LLHDST, T(not1), T(lsh), T(not2), T(lc2h) },
	{ AP, 0xd0000000, 0xf0000002, 0x04004000, 0xe400c000,
		N("or b32"), MCDST, LLDST, T(not1), T(lsw), T(not2), T(lc2w) },
	{ AP, 0xd0000000, 0xf0000002, 0x00008000, 0xe400c000,
		N("xor b16"), MCDST, LLHDST, T(not1), T(lsh), T(not2), T(lc2h) },
	{ AP, 0xd0000000, 0xf0000002, 0x04008000, 0xe400c000,
		N("xor b32"), MCDST, LLDST, T(not1), T(lsw), T(not2), T(lc2w) },

	{ AP, 0xd0000000, 0xf0000002, 0x0002c000, 0xe403c000,
		N("not b16"), MCDST, LLHDST, LHSRC2 },
	{ AP, 0xd0000000, 0xf0000002, 0x0402c000, 0xe403c000,
		N("not b32"), MCDST, LLDST, LSRC2 },

	{ AP, 0xd0000000, 0xf0000002, 0x20000000, 0xe0000000,	// really a LEA.
		N("add"), ADST, OFFS, AREG },

	{ AP, 0xd0000000, 0xf0000002, 0x40000000, 0xe0000000,
		N("mov"), T(ldstm), T(ldsto), LOCAL },
	{ AP, 0xd0000000, 0xf0000002, 0x60000000, 0xe0000000,
		N("mov"), T(ldstm), LOCAL, T(ldsto) },

	{ CP, 0xd0000000, 0xf0000002, 0x80000000, 0xe0000000,
		N("mov"), T(ldstm), T(ldsto), GLOBAL },
	{ CP, 0xd0000000, 0xf0000002, 0xa0000000, 0xe0000000,
		N("mov"), T(ldstm), GLOBAL, T(ldsto) },
	{ CP, 0xd0000000, 0xf0000002, 0xc0000000, 0xe0000000,
		T(redm), GLOBAL, T(ldsto) },
	{ CP, 0xd0000000, 0xf0000002, 0xe0000000, 0xe0000000,
		T(atomm), T(ldsto), GLOBAL2, T(ldsts2), T(mldsts3) },

	// e
	{ AP, 0xe0000000, 0xf0000002, 0x00000000, 0xe0000000,
		N("mad f32"), LLDST, T(lneg1), T(lsw), T(lc2w), T(lneg2), T(lc3w) },	// XXX what happens if you try both?
	{ AP, 0xe0000000, 0xf0000002, 0x20000000, 0xe0000000,
		N("mad sat f32"), LLDST, T(lneg1), T(lsw), T(lc2w), T(lneg2), T(lc3w) },	// XXX what happens if you try both?

	{ AP, 0xe0000000, 0xf0000002, 0x40000000, 0xe0000000,
		N("mad"), T(mad64r), N("f64"), LDDST, T(lneg1), LDSRC, LDSRC2, T(lneg2), LDSRC3 },
	{ AP, 0xe0000000, 0xf0000002, 0x60000000, 0xe0000000,
		N("add"), T(af64r), N("f64"), LDDST, T(lneg1), LDSRC, T(lneg2), LDSRC3 },
	{ AP, 0xe0000000, 0xf0000002, 0x80000000, 0xe0000000,
		N("mul"), T(cvtrnd), N("f64"), LDDST, T(lneg1), LDSRC, LDSRC2 },
	{ AP, 0xe0000000, 0xf0000002, 0xa0000000, 0xe0000000,
		N("min"), N("f64"), LDDST, T(lfm1), LDSRC, T(lfm2), LDSRC2 },
	{ AP, 0xe0000000, 0xf0000002, 0xc0000000, 0xe0000000,
		N("max"), N("f64"), LDDST, T(lfm1), LDSRC, T(lfm2), LDSRC2 },
	{ AP, 0xe0000000, 0xf0000002, 0xe0000000, 0xe0000000,
		N("set"), T(setf), N("f64"), MCDST, LLDST, T(lfm1), LDSRC, T(lfm2), LDSRC2 },

	// f
	{ AP, 0xf0000000, 0xf9000002, 0x00000000, 0xf0000000, // order of inputs: x, y, z, index, dref, bias/lod. index is integer, others float.
		N("texauto"), T(ltex), LTDST, TEX, TSRC, TOFFX, TOFFY, TOFFZ },
	{ AP, 0xf8000000, 0xf9000002, 0x00000000, 0xf0000000,
		N("texauto cube"), T(ltex), LTDST, TEX, TSRC },

	{ AP, 0xf1000000, 0xf1000002, 0x00000000, 0xf0000000, // takes integer inputs.
		N("texfetch"), T(ltex), LTDST, TEX, TSRC, TOFFX, TOFFY, TOFFZ },

	{ AP, 0xf0000000, 0xf8000002, 0x20000000, 0xf0000000, // bias needs to be same for everything, or else.
		N("texbias"), T(ltex), LTDST, TEX, TSRC, TOFFX, TOFFY, TOFFZ },
	{ AP, 0xf8000000, 0xf8000002, 0x20000000, 0xf0000000,
		N("texbias cube"), T(ltex), LTDST, TEX, TSRC },

	{ AP, 0xf0000000, 0xf8000002, 0x40000000, 0xf0000000, // lod needs to be same for everything, or else.
		N("texlod"), T(ltex), LTDST, TEX, TSRC, TOFFX, TOFFY, TOFFZ },
	{ AP, 0xf8000000, 0xf8000002, 0x40000000, 0xf0000000,
		N("texlod cube"), T(ltex), LTDST, TEX, TSRC },

	{ AP, 0xf0000000, 0xf0000002, 0x60000000, 0xf0000000, // integer input and output.
		N("texsize"), T(ltex), LTDST, TEX, LDST }, // in: LOD, out: size.x, size.y, size.z

	{ GP, 0xf0000200, 0xf0000602, 0xc0000000, 0xe0000000, N("emit") },
	{ GP, 0xf0000400, 0xf0000602, 0xc0000000, 0xe0000000, N("restart") },

	{ AP, 0xf0000000, 0xf0000002, 0xe0000000, 0xe0000004, N("nop") },
	{ AP, 0xf0000000, 0xf0000002, 0xe0000004, 0xe0000004, N("pmevent"), PM },

	{ FP, 0x00000002, 0xf0000002, 0, 0, N("discard") },
	{ AP, 0x10000002, 0xf0000002, 0, 0, N("bra"), CTARG },
	{ AP, 0x20000002, 0xf0000002, 0, 0, N("call"), CTARG },
	{ AP, 0x30000002, 0xf0000002, 0, 0, N("ret") },
	{ AP, 0x60000002, 0xf0000002, 0, 0, N("quadon") },
	{ AP, 0x70000002, 0xf0000002, 0, 0, N("quadpop") },
	{ AP, 0x861ffe02, 0xf61ffe02, 0, 0, N("bar sync"), BAR },
	{ AP, 0x90000002, 0xf0000002, 0, 0, N("trap") },
	{ AP, 0xa0000002, 0xf0000002, 0, 0, N("joinat"), CTARG },
	{ AP, 0xb0000002, 0xf0000002, 0, 0, N("brkpt") }, // sm_11
	
	// try to print out *some* info.
//	{ 0, 2, 0x00000000, 0x04000000, { OOPS, LHDST, T(lsh), T(lc3h) },
//	{ 0, 2, 0x04000000, 0x04000000, { OOPS, LLDST, T(lsw), T(lc3w) },
	// remaining stuff is floating point, no use for b16.
	{ AP, 0, 2, 0, 0, OOPS, MCDST, LLDST, T(lsw), T(lc2w), T(lc3w) },
	{ AP, 0, 0, 0, 0, OOPS },

};

/*
 * Predicates
 */
struct insn tabp[] = {
	{ AP, 0, 0, 0x00000000, 0x00000f80, N("never") },
	{ AP, 0, 0, 0x00000080, 0x00000f80, N("l"), COND },
	{ AP, 0, 0, 0x00000100, 0x00000f80, N("e"), COND },
	{ AP, 0, 0, 0x00000180, 0x00000f80, N("le"), COND },
	{ AP, 0, 0, 0x00000200, 0x00000f80, N("g"), COND },
	{ AP, 0, 0, 0x00000280, 0x00000f80, N("lg"), COND },
	{ AP, 0, 0, 0x00000300, 0x00000f80, N("ge"), COND },
	{ AP, 0, 0, 0x00000380, 0x00000f80, N("lge"), COND },
	{ AP, 0, 0, 0x00000400, 0x00000f80, N("u"), COND },
	{ AP, 0, 0, 0x00000480, 0x00000f80, N("lu"), COND },
	{ AP, 0, 0, 0x00000500, 0x00000f80, N("eu"), COND },
	{ AP, 0, 0, 0x00000580, 0x00000f80, N("leu"), COND },
	{ AP, 0, 0, 0x00000600, 0x00000f80, N("gu"), COND },
	{ AP, 0, 0, 0x00000680, 0x00000f80, N("lgu"), COND },
	{ AP, 0, 0, 0x00000700, 0x00000f80, N("geu"), COND },
	{ AP, 0, 0, 0x00000780, 0x00000f80 },
	{ AP, 0, 0, 0x00000800, 0x00000f80, N("o"), COND },
	{ AP, 0, 0, 0x00000880, 0x00000f80, N("c"), COND },
	{ AP, 0, 0, 0x00000900, 0x00000f80, N("a"), COND },
	{ AP, 0, 0, 0x00000980, 0x00000f80, N("s"), COND },
	{ AP, 0, 0, 0x00000e00, 0x00000f80, N("ns"), COND },
	{ AP, 0, 0, 0x00000e80, 0x00000f80, N("na"), COND },
	{ AP, 0, 0, 0x00000f00, 0x00000f80, N("nc"), COND },
	{ AP, 0, 0, 0x00000f80, 0x00000f80, N("no"), COND },
	{ AP, 0, 0, 0, 0, OOPS },
};

struct insn tab2w[] = {
	{ AP, 0, 0, 0, 3, T(p), T(l) },
	{ AP, 0, 0, 1, 3, T(p), T(l), NL, N("exit") },
	{ AP, 0, 0, 2, 3, N("join"), NL, T(p), T(l) },
	{ AP, 0, 0, 3, 3, T(i) },
};

/*
 * Disassembler driver
 *
 * You pass a block of memory to this function, disassembly goes out to given
 * FILE*. Optionally specify a start address of the code, to be used for
 * proper display of address on the left [so that addresses match the ones
 * used in branch insns].
 */

void nv50dis (FILE *out, uint32_t *code, int num, int ptype) {
	int cur = 0;
	while (cur < num) {
		uint32_t a[2] = {code[cur], 0}, m[2] = { 0, 0 };
		int end = 0;
		fprintf (out, "%s%08x: %s", cgray, cur*4, cnorm);
		if (code[cur++]&1) {
			if (cur >= num) {
				fprintf (out, "%08x          ", a[0]);
				fprintf (out, "%sincomplete%s\n", cred, cnorm);
				return;
			}
/*			if (!(cur&1)) {
				fprintf (out, "%08x          ", a);
				fprintf (out, "%smisaligned%s\n", cred, cnorm);
				continue;
			}*/
			a[1] = code[cur++];
			fprintf (out, "%08x %08x", a[0], a[1]);
		} else {
			fprintf (out, "%08x         ", a[0]);
		}
		struct insn *tab = ((a[0]&1)?tab2w:tabs);
		atomtab (out, a, m, tab, ptype);
		a[0] &= ~m[0];
		a[1] &= ~m[1];
		if (a[0] & ~1 || a[1]) {
			fprintf (out, " %s[unknown: %08x", cred, a[0]&~1);
			if (a[0]&1) fprintf (out, " %08x", a[1]);
			fprintf (out, "]%s", cnorm);
		}
		printf ("%s\n", cnorm);
		if (end) fprintf(out, "                                  %sexit%s\n", cgr, cnorm);
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
	while (scanf ("%x", &t) == 1) {
		if (num == maxnum) maxnum *= 2, code = realloc (code, maxnum*4);
		code[num++] = t;
	}
	nv50dis (stdout, code, num, ptype);
	return 0;
}
