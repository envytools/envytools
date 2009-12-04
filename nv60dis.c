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

#define VP 1
#define GP 2
#define FP 4
#define CP 8
#define AP (VP|GP|FP|CP)

/*
 * Registers:
 *
 *  - $r0-$r62: Normal, usable 32-bit regs. Allocated just like on tesla.
 *    Grouped into $r0d-$r60d for 64-bit quantities like doubles, into
 *    $r0q-$r56q for 128-bit quantities. There are no half-regs.
 *  - $r63: Bit bucket on write, 0 on read. Maybe just normal unallocated reg,
 *    like on tesla.
 *  - $p0-$p6: 1-bit predicate registers, usable.
 *  - $p7: Always-true predicate.
 */

/*
 * Instructions, from PTX manual:
 *   
 *   1. Integer Arithmetic
 *    - add		done
 *    - sub		done
 *    - addc		done
 *    - subc		done
 *    - mul		done
 *    - mul24		done
 *    - mad		done
 *    - mad24		done
 *    - sad		done
 *    - div		TODO
 *    - rem		TODO
 *    - abs		done
 *    - neg		done
 *    - min		done, check predicate selecting min/max
 *    - max		done
 *   2. Floating-Point
 *    - add		done
 *    - sub		done
 *    - mul		done
 *    - fma		done
 *    - mad		done
 *    - div.approxf32	TODO
 *    - div.full.f32	TODO
 *    - div.f64		TODO
 *    - abs		done
 *    - neg		done
 *    - min		done
 *    - max		done
 *    - rcp		TODO started
 *    - sqrt		TODO started
 *    - rsqrt		TODO started
 *    - sin		done
 *    - cos		done
 *    - lg2		TODO started
 *    - ex2		TODO started
 *   3. Comparison and selection
 *    - set		done, check these 3 bits
 *    - setp		done
 *    - selp		done
 *    - slct		done
 *   4. Logic and Shift
 *    - and		done, check not bitfields for all 4
 *    - or		done
 *    - xor		done
 *    - not		done
 *    - cnot		TODO
 *    - shl		TODO started
 *    - shr		TODO started
 *   5. Data Movement and Conversion
 *    - mov		done
 *    - ld		done
 *    - st		done
 *    - cvt		TODO started
 *   6. Texture
 *    - tex		done, needs OpenGL stuff
 *   7. Control Flow
 *    - { }		done
 *    - @		done
 *    - bra		TODO started
 *    - call		TODO started
 *    - ret		done
 *    - exit		done
 *   8. Parallel Synchronization and Communication
 *    - bar		TODO
 *    - membar.cta	TODO
 *    - membar.gl	TODO
 *    - atom		TODO
 *    - red		TODO
 *    - vote		TODO
 *   9. Miscellaneous
 *    - trap		done
 *    - brkpt		done
 *    - pmevent		TODO
 *
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

#define APROTO (FILE *out, ull *a, ull *m, const void *v, int ptype, uint32_t pos)

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
			tab->atoms[i].fun (out, a, m, tab->atoms[i].arg, ptype, pos);
}

#define N(x) atomname, x
void atomname APROTO {
	fprintf (out, " %s%s", cgr, (char *)v);
}

#define OOPS atomoops, 0
void atomoops APROTO {
	fprintf (out, " %s???", cred);
}

/*
 * Code target field
 */

#define CTARG atomctarg, 0
void atomctarg APROTO {
	uint32_t delta = BF(26, 24);
	if (delta & 0x800000) delta += 0xff000000;
	fprintf (out, " %s%#x", cbr, pos + delta);
}

/*
 * Misc number fields
 */

int baroff[] = { 0x14, 4, 0, 0 };
int tcntoff[] = { 0x1a, 12, 0, 0 };
int immoff[] = { 0x1a, 20, 0, 1 };
int fimmoff[] = { 0x1a, 20, 12, 0 };
int dimmoff[] = { 0x1a, 20, 44, 0 };
int limmoff[] = { 0x1a, 32, 0, 0 };
int bnumoff[] = { 0x37, 2, 0, 0 };
int hnumoff[] = { 0x38, 1, 0, 0 };
#define BAR atomnum, baroff
#define TCNT atomnum, tcntoff
#define IMM atomnum, immoff
#define FIMM atomnum, fimmoff
#define DIMM atomnum, dimmoff
#define LIMM atomnum, limmoff
#define BNUM atomnum, bnumoff
#define HNUM atomnum, hnumoff
void atomnum APROTO {
	const int *n = v;
	ull num = BF(n[0], n[1])<<n[2];
	if (n[3] && num&1ull<<(n[1]-1))
		fprintf (out, " %s-%#llx", cyel, (1ull<<n[1]) - num);
	else
		fprintf (out, " %s%#llx", cyel, num);
}

/*
 * Ignored fields
 */

int igndtex[] = { 0x28, 5 }; // how can you make the same bug twice? hm.
#define IGNDTEX atomign, igndtex
void atomign APROTO {
	const int *n = v;
	BF(n[0], n[1]);
}

/*
 * Register fields
 */

int dstoff[] = { 0xe, 6, 'r' };
int src1off[] = { 0x14, 6, 'r' };
int psrc1off[] = { 0x14, 3, 'p' };
int src2off[] = { 0x1a, 6, 'r' };
int psrc2off[] = { 0x1a, 3, 'p' };
int src3off[] = { 0x31, 6, 'r' };
int psrc3off[] = { 0x31, 3, 'p' };
int predoff[] = { 0xa, 3, 'p' };
int pdstoff[] = { 0x11, 3, 'p' };
int texoff[] = { 0x20, 7, 't' };
int cfoff[] = { 0, 0, 'c' };
#define DST atomreg, dstoff
#define DSTD atomdreg, dstoff
#define DSTQ atomqreg, dstoff
#define SRC1 atomreg, src1off
#define SRC1D atomdreg, src1off
#define PSRC1 atomreg, psrc1off
#define SRC2 atomreg, src2off
#define SRC2D atomdreg, src2off
#define PSRC2 atomreg, psrc2off
#define SRC3 atomreg, src3off
#define SRC3D atomdreg, src3off
#define PSRC3 atomreg, psrc3off
#define PRED atomreg, predoff
#define PDST atomreg, pdstoff
#define TEX atomreg, texoff
#define CF atomreg, cfoff
void atomreg APROTO {
	const int *n = v;
	int r = BF(n[0], n[1]);
	fprintf (out, " %s$%c%d", (n[2]=='r')?cbl:cmag, n[2], r);
}
void atomdreg APROTO {
	const int *n = v;
	fprintf (out, " %s$%c%lldd", (n[2]=='r')?cbl:cmag, n[2], BF(n[0], n[1]));
}
void atomqreg APROTO {
	const int *n = v;
	fprintf (out, " %s$%c%lldq", (n[2]=='r')?cbl:cmag, n[2], BF(n[0], n[1]));
}
#define TDST atomtdst, 0
void atomtdst APROTO {
	int base = BF(0xe, 6);
	int mask = BF(0x2e, 4);
	int k = 0, i;
	fprintf (out, " %s{", cnorm);
	for (i = 0; i < 4; i++)
		if (mask & 1<<i)
			fprintf (out, " %s$r%d", cbl, base+k++);
		else
			fprintf (out, " %s_", cbl);
	fprintf (out, " %s}", cnorm);
}
#define TSRC atomtsrc, 0
void atomtsrc APROTO {
	int base = BF(0x14, 6);
	int cnt = BF(0x34, 2);
	int i;
	fprintf (out, " %s{", cnorm);
	for (i = 0; i <= cnt; i++)
		fprintf (out, " %s$r%d", cbl, base+i);
	fprintf (out, " %s}", cnorm);
}

/*
 * Memory fields
 */

int gmem[] = { 'g', 0, 32 };
int gdmem[] = { 'g', 1, 32 };
int smem[] = { 's', 0, 24 };
int lmem[] = { 'l', 0, 24 };
int fcmem[] = { 'c', 0, 16 };
#define GLOBAL atommem, gmem
#define GLOBALD atommem, gdmem
#define SHARED atommem, smem
#define LOCAL atommem, lmem
#define FCONST atommem, fcmem
void atommem APROTO {
	const int *n = v;
	uint32_t delta = BF(26, n[2]);
	fprintf (out, " %s%c", ccy, n[0]);
	if (n[0] == 'c')
		fprintf (out, "%lld", BF(0x2a, 4));
	fprintf (out, "[%s$r%lld%s", cbl, BF(20, 6), n[1]?"d":"");
	if (delta & 0x80000000U)
		fprintf (out, "%s-%s%#x%s]", ccy, cyel, -delta, ccy);
	else
		fprintf (out, "%s+%s%#x%s]", ccy, cyel, delta, ccy);
}

#define CONST atomconst, 0
void atomconst APROTO {
	int delta = BF(0x1a, 16);
	int space = BF(0x2a, 4);
	fprintf (out, " %sc%d[%s%#x%s]", ccy, space, cyel, delta, ccy);
}

/*
 * The instructions
 */

F(gmem, 0x3a, GLOBAL, GLOBALD)
F(slmem, 0x38, LOCAL, SHARED)

struct insn tabldstt[] = {
	{ AP, 0x00, 0xe0, N("u8") },
	{ AP, 0x20, 0xe0, N("s8") },
	{ AP, 0x40, 0xe0, N("u16") },
	{ AP, 0x60, 0xe0, N("s16") },
	{ AP, 0x80, 0xe0, N("b32") },
	{ AP, 0xa0, 0xe0, N("b64") },
	{ AP, 0xc0, 0xe0, N("b128") },
	{ AP, 0, 0, OOPS },
};

struct insn tabldstd[] = {
	{ AP, 0x00, 0xe0, DST },
	{ AP, 0x20, 0xe0, DST },
	{ AP, 0x40, 0xe0, DST },
	{ AP, 0x60, 0xe0, DST },
	{ AP, 0x80, 0xe0, DST },
	{ AP, 0xa0, 0xe0, DSTD },
	{ AP, 0xc0, 0xe0, DSTQ },
	{ AP, 0, 0, OOPS, DST },
};

struct insn tabfarm[] = {
	{ AP, 0x0000000000000000ull, 0x0180000000000000ull, N("rn") },
	{ AP, 0x0080000000000000ull, 0x0180000000000000ull, N("rm") },
	{ AP, 0x0100000000000000ull, 0x0180000000000000ull, N("rp") },
	{ AP, 0x0180000000000000ull, 0x0180000000000000ull, N("rz") },
};

struct insn tabfcrm[] = {
	{ AP, 0x0000000000000000ull, 0x0006000000000000ull, N("rn") },
	{ AP, 0x0002000000000000ull, 0x0006000000000000ull, N("rm") },
	{ AP, 0x0004000000000000ull, 0x0006000000000000ull, N("rp") },
	{ AP, 0x0006000000000000ull, 0x0006000000000000ull, N("rz") },
};

struct insn tabsetit[] = {
	{ AP, 0x0080000000000000ull, 0x0780000000000000ull, N("lt") },
	{ AP, 0x0100000000000000ull, 0x0780000000000000ull, N("eq") },
	{ AP, 0x0180000000000000ull, 0x0780000000000000ull, N("le") },
	{ AP, 0x0200000000000000ull, 0x0780000000000000ull, N("gt") },
	{ AP, 0x0280000000000000ull, 0x0780000000000000ull, N("ne") },
	{ AP, 0x0300000000000000ull, 0x0780000000000000ull, N("ge") },
	{ AP, 0x0380000000000000ull, 0x0780000000000000ull, N("num") },
	{ AP, 0x0400000000000000ull, 0x0780000000000000ull, N("nan") },
	{ AP, 0x0480000000000000ull, 0x0780000000000000ull, N("ltu") },
	{ AP, 0x0500000000000000ull, 0x0780000000000000ull, N("equ") },
	{ AP, 0x0580000000000000ull, 0x0780000000000000ull, N("leu") },
	{ AP, 0x0600000000000000ull, 0x0780000000000000ull, N("gtu") },
	{ AP, 0x0680000000000000ull, 0x0780000000000000ull, N("neu") },
	{ AP, 0x0700000000000000ull, 0x0780000000000000ull, N("geu") },
	{ AP, 0, 0, OOPS },
};

struct insn tabis2[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, SRC2 },
	{ AP, 0x0000400000000000ull, 0x0000c00000000000ull, CONST },
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, IMM },
	{ AP, 0, 0, OOPS },
};

struct insn tabfs2[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, SRC2 },
	{ AP, 0x0000400000000000ull, 0x0000c00000000000ull, CONST },
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, FIMM },
	{ AP, 0, 0, OOPS },
};

struct insn tabds2[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, SRC2D },
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, DIMM },
	{ AP, 0, 0, OOPS },
};

F1(ias, 5, N("sat"))
F1(fas, 0x31, N("sat"))
F1(faf, 5, N("ftz"))
F1(fmf, 6, N("ftz"))
F1(fmneg, 0x39, N("neg"))
F1(neg1, 9, N("neg"))
F1(neg2, 8, N("neg"))
F1(abs1, 7, N("abs"))
F1(abs2, 6, N("abs"))
F1(rint, 7, N("rint"))

F1(acout, 0x30, CF)
F1(acout2, 0x3a, CF)
F1(acin, 6, CF)
F1(acin2, 0x37, CF)

F(us32, 5, N("u32"), N("s32"))
F1(high, 6, N("high"))

F1(pnot1, 0x17, N("not"))
F1(pnot2, 0x1d, N("not"))
F1(pnot3, 0x34, N("not"))

F(ltex, 9, N("all"), N("live"))

struct insn tabsetlop[] = {
	{ AP, 0x000e000000000000ull, 0x006e000000000000ull },	// noop, really "and $p7"
	{ AP, 0x0000000000000000ull, 0x0060000000000000ull, N("and"), T(pnot3), PSRC3 },
	{ AP, 0x0020000000000000ull, 0x0060000000000000ull, N("or"), T(pnot3), PSRC3 },
	{ AP, 0x0040000000000000ull, 0x0060000000000000ull, N("xor"), T(pnot3), PSRC3 },
	{ AP, 0, 0, OOPS, T(pnot3), PSRC3 },
};

// TODO: this definitely needs a second pass to see which combinations really work.
struct insn tabcvtfdst[] = {
	{ AP, 0x0000000000100000ull, 0x0000000000700000ull, T(ias), N("f16"), DST },
	{ AP, 0x0000000000200000ull, 0x0000000000700000ull, T(ias), N("f32"), DST },
	{ AP, 0x0000000000300000ull, 0x0000000000700000ull, N("f64"), DSTD },
	{ AP, 0, 0, OOPS, DST },
};

struct insn tabcvtidst[] = {
	{ AP, 0x0000000000000000ull, 0x0000000000700080ull, N("u8"), DST },
	{ AP, 0x0000000000000080ull, 0x0000000000700080ull, N("s8"), DST },
	{ AP, 0x0000000000100000ull, 0x0000000000700080ull, N("u16"), DST },
	{ AP, 0x0000000000100080ull, 0x0000000000700080ull, N("s16"), DST },
	{ AP, 0x0000000000200000ull, 0x0000000000700080ull, N("u32"), DST },
	{ AP, 0x0000000000200080ull, 0x0000000000700080ull, N("s32"), DST },
	{ AP, 0x0000000000300000ull, 0x0000000000700080ull, N("u64"), DSTD },
	{ AP, 0x0000000000300080ull, 0x0000000000700080ull, N("s64"), DSTD },
	{ AP, 0, 0, OOPS, DST },
};

struct insn tabcvtfsrc[] = {
	{ AP, 0x0000000000800000ull, 0x0000000003800000ull, T(neg2), T(abs2), N("f16"), SRC2 },
	{ AP, 0x0000000001000000ull, 0x0000000003800000ull, T(neg2), T(abs2), N("f32"), SRC2 },
	{ AP, 0x0000000001800000ull, 0x0000000003800000ull, T(neg2), T(abs2), N("f64"), SRC2D },
	{ AP, 0, 0, OOPS, T(neg2), T(abs2), SRC2 },
};

struct insn tabcvtisrc[] = {
	{ AP, 0x0000000000000000ull, 0x0000000003800200ull, T(neg2), T(abs2), N("u8"), BNUM, SRC2 },
	{ AP, 0x0000000000000200ull, 0x0000000003800200ull, T(neg2), T(abs2), N("s8"), BNUM, SRC2 },
	{ AP, 0x0000000000800000ull, 0x0000000003800200ull, T(neg2), T(abs2), N("u16"), HNUM, SRC2 },
	{ AP, 0x0000000000800200ull, 0x0000000003800200ull, T(neg2), T(abs2), N("s16"), HNUM, SRC2 },
	{ AP, 0x0000000001000000ull, 0x0000000003800200ull, T(neg2), T(abs2), N("u32"), SRC2 },
	{ AP, 0x0000000001000200ull, 0x0000000003800200ull, T(neg2), T(abs2), N("s32"), SRC2 },
	{ AP, 0x0000000001800000ull, 0x0000000003800200ull, T(neg2), T(abs2), N("u64"), SRC2D },
	{ AP, 0x0000000001800200ull, 0x0000000003800200ull, T(neg2), T(abs2), N("s64"), SRC2D },
	{ AP, 0, 0, OOPS, T(neg2), T(abs2), SRC2 },
};

struct insn tabsreg[] = {
	{ AP, 0x0000000000000000ull, 0x00000000fc000000ull, N("laneid") },
	{ AP, 0x0000000008000000ull, 0x00000000fc000000ull, N("nphysid") }, // bits 8-14: nwarpid, bits 20-28: nsmid
	{ AP, 0x000000000c000000ull, 0x00000000fc000000ull, N("physid") }, // bits 8-12: warpid, bits 20-28: smid
	{ AP, 0x0000000010000000ull, 0x00000000fc000000ull, N("pm0") },
	{ AP, 0x0000000014000000ull, 0x00000000fc000000ull, N("pm1") },
	{ AP, 0x0000000018000000ull, 0x00000000fc000000ull, N("pm2") },
	{ AP, 0x000000001c000000ull, 0x00000000fc000000ull, N("pm3") },
	{ AP, 0x0000000140000000ull, 0x00000001fc000000ull, N("clock") }, // XXX some weird shift happening here.
	{ AP, 0x0000000144000000ull, 0x00000001fc000000ull, N("clockhi") },
	{ AP, 0x0000000084000000ull, 0x00000000fc000000ull, N("tidx") },
	{ AP, 0x0000000088000000ull, 0x00000000fc000000ull, N("tidy") },
	{ AP, 0x000000008c000000ull, 0x00000000fc000000ull, N("tidz") },
	{ AP, 0x0000000094000000ull, 0x00000000fc000000ull, N("ctaidx") },
	{ AP, 0x0000000098000000ull, 0x00000000fc000000ull, N("ctaidy") },
	{ AP, 0x000000009c000000ull, 0x00000000fc000000ull, N("ctaidz") },
	{ AP, 0x00000000a4000000ull, 0x00000000fc000000ull, N("ntidx") },
	{ AP, 0x00000000a8000000ull, 0x00000000fc000000ull, N("ntidy") },
	{ AP, 0x00000000ac000000ull, 0x00000000fc000000ull, N("ntidz") },
	{ AP, 0x00000000b0000000ull, 0x00000000fc000000ull, N("gridid") },
	{ AP, 0x00000000b4000000ull, 0x00000000fc000000ull, N("nctaidx") },
	{ AP, 0x00000000b8000000ull, 0x00000000fc000000ull, N("nctaidy") },
	{ AP, 0x00000000bc000000ull, 0x00000000fc000000ull, N("nctaidz") },
	{ AP, 0x00000000e0000000ull, 0x00000000fc000000ull, N("lanemask_eq") }, // I have no idea what these do, but ptxas eats them just fine.
	{ AP, 0x00000000e4000000ull, 0x00000000fc000000ull, N("lanemask_lt") },
	{ AP, 0x00000000e8000000ull, 0x00000000fc000000ull, N("lanemask_le") },
	{ AP, 0x00000000ec000000ull, 0x00000000fc000000ull, N("lanemask_gt") },
	{ AP, 0x00000000f0000000ull, 0x00000000fc000000ull, N("lanemask_ge") },
	{ AP, 0, 0, OOPS },
};

struct insn tabaddop[] = {
	{ AP, 0x0000000000000000ull, 0x0000000000000300ull, N("add") },
	{ AP, 0x0000000000000100ull, 0x0000000000000300ull, N("sub") },
	{ AP, 0x0000000000000200ull, 0x0000000000000300ull, N("subr") },
	{ AP, 0, 0, OOPS },
};

/*
 * Opcode format
 *
 * 0000000000000007 insn type, roughly: 0: float 1: double 2: long immediate 3: integer 4: moving and converting 5: g/s/l[] memory access 6: c[] and texture access 7: control
 * 0000000000000018 ??? never seen used
 * 00000000000003e0 misc flags
 * 0000000000001c00 used predicate [7 is always true]
 * 0000000000002000 negate predicate
 * 00000000000fc000 DST
 * 0000000003f00000 SRC1
 * 00000000fc000000 SRC2
 * 000003fffc000000 CONST offset
 * 00003c0000000000 CONST space
 * 00003ffffc000000 IMM/FIMM/DIMM
 * 0000c00000000000 0 = use SRC2, 1 = use CONST, 2 = ???, 3 = IMM/FIMM/DIMM
 * 0001000000000000 misc flag
 * 007e000000000000 SRC3
 * 0780000000000000 misc field. rounding mode or comparison subop or...
 * f800000000000000 opcode
 */

struct insn tabm[] = {
	{ AP, 0x080e000000000000ull, 0xf81e000000000007ull, N("min"), N("f32"), DST, T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2) }, // looks like min/max is selected by a normal predicate. fun. needs to be checked
	{ AP, 0x081e000000000000ull, 0xf81e000000000007ull, N("max"), N("f32"), DST, T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2) },
	// 10?
	{ AP, 0x1800000000000000ull, 0xf800000000000007ull, N("set"), DST, T(setit), N("f32"), T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2), T(setlop) },
	{ AP, 0x200000000001c000ull, 0xf80000000001c007ull, N("set"), PDST, T(setit), N("f32"), T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2), T(setlop) }, // and these unknown bits are what? another predicate?
	// 28?
	{ AP, 0x3000000000000000ull, 0xf800000000000007ull, N("add"), T(fmf), T(ias), T(farm), N("f32"), DST, T(neg1), N("mul"), SRC1, T(fs2), T(neg2), SRC3 },
	{ AP, 0x3800000000000000ull, 0xf800000000000007ull, N("slct"), N("b32"), DST, SRC1, T(fs2), T(setit), N("f32"), SRC3 },
	// 40?
	// 48?
	{ AP, 0x5000000000000000ull, 0xf800000000000007ull, N("add"), T(faf), T(fas), T(farm), N("f32"), DST, T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2) },
	{ AP, 0x5800000000000000ull, 0xf800000000000007ull, N("mul"), T(fmf), T(ias), T(farm), T(fmneg), N("f32"), DST, SRC1, T(fs2) },
	{ AP, 0x6000000000000000ull, 0xf800000000000027ull, N("presin"), N("f32"), DST, T(fs2) },
	{ AP, 0x6000000000000020ull, 0xf800000000000027ull, N("preex2"), N("f32"), DST, T(fs2) },
	// 68-c0?
	{ AP, 0xc800000000000000ull, 0xf80000001c000007ull, N("cos"), N("f32"), DST, SRC1 },
	{ AP, 0xc800000004000000ull, 0xf80000001c000007ull, N("sin"), N("f32"), DST, SRC1 },
	{ AP, 0xc800000008000000ull, 0xf80000001c000007ull, N("ex2"), N("f32"), DST, SRC1 },
	{ AP, 0xc80000000c000000ull, 0xf80000001c000007ull, N("lg2"), N("f32"), DST, SRC1 },
	{ AP, 0xc800000010000000ull, 0xf80000001c000007ull, N("rcp"), N("f32"), DST, SRC1 },
	{ AP, 0xc800000014000000ull, 0xf80000001c000007ull, N("rsqrt"), N("f32"), DST, SRC1 },
	{ AP, 0x0000000000000000ull, 0x0000000000000007ull, OOPS, T(farm), N("f32"), DST, SRC1, T(fs2), SRC3 },


	{ AP, 0x080e000000000001ull, 0xf81e000000000007ull, N("min"), N("f64"), DSTD, T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2) },
	{ AP, 0x081e000000000001ull, 0xf81e000000000007ull, N("max"), N("f64"), DSTD, T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2) },
	{ AP, 0x1000000000000001ull, 0xf800000000000007ull, N("set"), DST, T(setit), N("f64"), T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2), T(setlop) },
	{ AP, 0x180000000001c001ull, 0xf80000000001c007ull, N("set"), PDST, T(setit), N("f64"), T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2), T(setlop) },
	{ AP, 0x2000000000000001ull, 0xf800000000000007ull, N("add"), T(farm), N("f64"), DSTD, T(neg1), N("mul"), SRC1D, T(ds2), T(neg2), SRC3D },
	// 28?
	// 30?
	// 38?
	// 40?
	{ AP, 0x4800000000000001ull, 0xf800000000000007ull, N("add"), T(farm), N("f64"), DSTD, T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2) },
	{ AP, 0x5000000000000001ull, 0xf800000000000007ull, N("mul"), T(farm), T(neg1), N("f64"), DSTD, SRC1D, T(ds2) },
	{ AP, 0x0000000000000001ull, 0x0000000000000007ull, OOPS, T(farm), N("f64"), DSTD, SRC1D, T(ds2), SRC3D },


	{ AP, 0x0800000000000002ull, 0xf800000000000007ull, T(addop), T(ias), N("b32"), T(acout2), DST, SRC1, LIMM, T(acin) },
	// 10?
	{ AP, 0x18000000000001e2ull, 0xf8000000000001e7ull, N("mov"), N("b32"), DST, LIMM }, // wanna bet these unknown bits are tesla-like lanemask?
	// 20?
	// 28?
	// 30?
	{ AP, 0x3800000000000002ull, 0xf8000000000000c7ull, N("and"), N("b32"), DST, SRC1, LIMM },
	{ AP, 0x3800000000000042ull, 0xf8000000000000c7ull, N("or"), N("b32"), DST, SRC1, LIMM },
	{ AP, 0x3800000000000082ull, 0xf8000000000000c7ull, N("xor"), N("b32"), DST, SRC1, LIMM },
	{ AP, 0x0000000000000002ull, 0x0000000000000007ull, OOPS, N("b32"), DST, SRC1, LIMM },


	{ AP, 0x080e000000000003ull, 0xf81e000000000007ull, N("min"), T(us32), DST, SRC1, T(is2) },
	{ AP, 0x081e000000000003ull, 0xf81e000000000007ull, N("max"), T(us32), DST, SRC1, T(is2) },
	{ AP, 0x1000000000000003ull, 0xf800000000000007ull, N("set"), DST, T(setit), T(us32), SRC1, T(is2), T(acin), T(setlop) },
	{ AP, 0x180000000001c003ull, 0xf80000000001c007ull, N("set"), PDST, T(setit), T(us32), SRC1, T(is2), T(acin), T(setlop) },
	{ AP, 0x2000000000000003ull, 0xf8000000000000a7ull, T(addop), T(acout), DST, N("mul"), T(high), N("u32"), SRC1, T(is2), SRC3, T(acin2) }, // bet you these bits are independent s/u for each source, like on tesla?
	{ AP, 0x20000000000000a3ull, 0xf8000000000000a7ull, T(addop), T(acout), DST, N("mul"), T(high), N("s32"), SRC1, T(is2), SRC3, T(acin2) },
	{ AP, 0x2800000000000003ull, 0xf800000000000007ull, N("ins"), N("b32"), DST, SRC1, T(is2), SRC3 },
	{ AP, 0x3000000000000003ull, 0xf800000000000007ull, N("slct"), N("b32"), DST, SRC1, T(is2), T(setit), T(us32), SRC3 },
	{ AP, 0x3800000000000003ull, 0xf800000000000007ull, N("sad"), T(us32), DST, SRC1, T(is2), SRC3 },
	// 40?
	{ AP, 0x4800000000000003ull, 0xf800000000000007ull, T(addop), T(ias), N("b32"), T(acout), DST, SRC1, T(is2), T(acin) },
	{ AP, 0x5000000000000003ull, 0xf8000000000000a7ull, N("mul"), T(high), N("u32"), T(acout), DST, SRC1, T(is2) },	// looks like acout, but... wouldn't it always be 0? hm.
	{ AP, 0x50000000000000a3ull, 0xf8000000000000a7ull, N("mul"), T(high), N("s32"), T(acout), DST, SRC1, T(is2) },
	{ AP, 0x5800000000000003ull, 0xf800000000000007ull, N("shr"), T(us32), DST, SRC1, T(is2) },
	{ AP, 0x6000000000000003ull, 0xf800000000000007ull, N("shl"), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x6800000000000003ull, 0xf8000000000000c7ull, N("and"), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x6800000000000043ull, 0xf8000000000000c7ull, N("or"), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x6800000000000083ull, 0xf8000000000000c7ull, N("xor"), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x68000000000001c3ull, 0xf8000000000001c7ull, N("not2"), N("b32"), DST, SRC1, T(is2) }, // yes, this is probably just a mov2 with a not bit set.
	{ AP, 0x7000000000000003ull, 0xf800000000000007ull, N("ext"), T(us32), DST, SRC1, T(is2) },
	{ AP, 0x0000000000000003ull, 0x0000000000000007ull, OOPS, N("b32"), DST, SRC1, T(is2), SRC3 },


	// 08?
	{ AP, 0x0c0e00000001c004ull, 0xfc0e0000c001c007ull, N("and"), PDST, T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ AP, 0x0c0e00004001c004ull, 0xfc0e0000c001c007ull, N("or"), PDST, T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ AP, 0x0c0e00008001c004ull, 0xfc0e0000c001c007ull, N("xor"), PDST, T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ AP, 0x1000000000000004ull, 0xfc00000000000007ull, N("cvt"), T(rint), T(fcrm), T(cvtfdst), T(cvtfsrc) },
	{ AP, 0x1400000000000004ull, 0xfc00000000000007ull, N("cvt"), T(fcrm), T(cvtidst), T(cvtfsrc) },
	{ AP, 0x1800000000000004ull, 0xfc00000000000007ull, N("cvt"), T(fcrm), T(cvtfdst), T(cvtisrc) },
	{ AP, 0x1c00000000000004ull, 0xfc00000000000007ull, N("cvt"), T(ias), T(cvtidst), T(cvtisrc) },
	{ AP, 0x2000000000000004ull, 0xf800000000000007ull, N("selp"), N("b32"), DST, SRC1, T(is2), T(pnot3), PSRC3 },
	{ AP, 0x28000000000001e4ull, 0xfc000000000001e7ull, N("mov"), N("b32"), DST, T(is2) },
	{ AP, 0x2c00000000000004ull, 0xfc00000000000007ull, N("mov"), N("b32"), DST, T(sreg) },
	// 30?
	// 38?
	// 40?
	// 48?
	{ AP, 0x50ee8000fc0fc004ull, 0xf8ff8000fc0fc087ull, N("bar sync"), BAR }, // ... what a fucking mess
	{ AP, 0x50ee0000fc0fc004ull, 0xf8ff8000fc0fc087ull, N("bar sync"), SRC1 },
	{ AP, 0x50ee0000000fc084ull, 0xf8ffc000000fc087ull, N("bar arrive"), SRC1, SRC2 },
	{ AP, 0x50ee4000000fc084ull, 0xf8ffc000000fc087ull, N("bar arrive"), SRC1, TCNT },
	{ AP, 0x50ee8000000fc084ull, 0xf8ffc000000fc087ull, N("bar arrive"), BAR, SRC2 },
	{ AP, 0x50eec000000fc084ull, 0xf8ffc000000fc087ull, N("bar arrive"), BAR, TCNT },


	{ AP, 0x8000000000000105ull, 0xf800000000000307ull, N("mov"), T(ldstt), T(ldstd), T(gmem) }, // XXX wtf is this flag?
	{ AP, 0x8000000000000305ull, 0xf800000000000307ull, N("mov"), T(ldstt), T(ldstd), N("volatile"), T(gmem) },
	{ AP, 0x9000000000000005ull, 0xf800000000000307ull, N("mov"), T(ldstt), T(gmem), T(ldstd) },
	{ AP, 0x9000000000000305ull, 0xf800000000000307ull, N("mov"), T(ldstt), N("volatile"), T(gmem), T(ldstd) },
	{ AP, 0xc000000000000005ull, 0xf800000000000007ull, N("mov"), T(ldstt), T(ldstd), T(slmem) },
	{ AP, 0xc800000000000005ull, 0xf800000000000007ull, N("mov"), T(ldstt), T(slmem), T(ldstd) },
	{ AP, 0x0000000000000005ull, 0x0000000000000007ull, OOPS, T(ldstt), T(ldstd), T(gmem), SRC3 },


	{ AP, 0x1400000000000006ull, 0xfc00000000000007ull, N("mov"), T(ldstt), T(ldstd), FCONST },
	{ AP, 0x80000000fc000086ull, 0xfc000000fc000087ull, N("texauto"), T(ltex), TDST, TEX, TSRC, IGNDTEX }, // mad as a hatter.
	{ AP, 0x90000000fc000086ull, 0xfc000000fc000087ull, N("texfetch"), T(ltex), TDST, TEX, TSRC, IGNDTEX },
	{ AP, 0x0000000000000006ull, 0x0000000000000007ull, OOPS, T(ltex), TDST, TEX, TSRC, IGNDTEX }, // is assuming a tex instruction a good idea here? probably. there are loads of unknown tex insns after all.


	{ AP, 0x40000000000001e7ull, 0xf0000000000001e7ull, N("bra"), CTARG },
	{ AP, 0x5000000000010007ull, 0xf000000000010007ull, N("call"), CTARG }, // XXX: this has no predicate field. implement it someday.
	{ AP, 0x80000000000001e7ull, 0xf0000000000001e7ull, N("exit") },
	{ AP, 0x90000000000001e7ull, 0xf0000000000001e7ull, N("ret") },
	{ AP, 0xd00000000000c007ull, 0xf00000000000c007ull, N("trap") },
	{ AP, 0x0000000000000007ull, 0x0000000000000007ull, OOPS, CTARG },


	{ AP, 0x0, 0x0, OOPS, DST, SRC1, T(is2), SRC3 },
};

struct insn tabs[] = {
	{ AP, 0x1c00, 0x3c00, T(m) },
	{ AP, 0x3c00, 0x3c00, N("never"), T(m) },	// probably.
	{ AP, 0x0000, 0x2000, PRED, T(m) },
	{ AP, 0x2000, 0x2000, N("not"), PRED, T(m) },
	{ AP, 0x0, 0x0, OOPS, T(m) },
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
		cur++;
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
		struct insn *tab = tabs;
		atomtab (out, &a, &m, tab, ptype, cur*4);
		a &= ~m;
		if (a) {
			fprintf (out, (" %s[unknown: %016llx]%s"), cred, a, cnorm);
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
