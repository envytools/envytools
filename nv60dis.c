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
 * Instructions, from PTX manual:
 *   
 *   1. Integer Arithmetic
 *    - add		TODO started
 *    - sub		TODO started
 *    - addc		TODO
 *    - subc		TODO
 *    - mul		TODO
 *    - mul24		TODO
 *    - mad		TODO
 *    - mad24		TODO
 *    - sad		TODO
 *    - sad.64		TODO
 *    - div		TODO
 *    - rem		TODO
 *    - abs		TODO
 *    - neg		TODO
 *    - min		TODO started
 *    - max		TODO started
 *   2. Floating-Point
 *    - add		done
 *    - sub		done
 *    - mul		done
 *    - fma		done
 *    - mad		done
 *    - div.approxf32	TODO
 *    - div.full.f32	TODO
 *    - div.f64		TODO
 *    - abs		TODO
 *    - neg		TODO
 *    - min		done
 *    - max		done
 *    - rcp.f32		TODO
 *    - rcp.f64		TODO
 *    - sqrt.f32	TODO
 *    - sqrt.f64	TODO
 *    - rsqrt.f32	TODO
 *    - rsqrt.f64	TODO
 *    - sin		TODO
 *    - cos		TODO
 *    - lg2		TODO
 *    - ex2		TODO
 *   3. Comparison and selection
 *    - set		TODO
 *    - setp		TODO
 *    - selp		TODO
 *    - slct		TODO
 *   4. Logic and Shift
 *    - and		done, check not bitfields for all 4
 *    - or		done
 *    - xor		done
 *    - not		done
 *    - cnot		TODO
 *    - shl		TODO
 *    - shr		TODO
 *   5. Data Movement and Conversion
 *    - mov		TODO
 *    - ld		TODO
 *    - st		TODO
 *    - cvt		TODO
 *   6. Texture
 *    - tex		TODO
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
 *    - membar.gl	sweet mother of God, what?
 *    - atom		TODO
 *    - red		TODO
 *    - vote		TODO
 *   9. Miscellaneous
 *    - trap		TODO
 *    - brkpt		TODO
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

int immoff[] = { 0x1a, 20, 0, 1 };
int fimmoff[] = { 0x1a, 20, 12, 0 };
int dimmoff[] = { 0x1a, 20, 44, 0 };
int limmoff[] = { 0x1a, 32, 0, 0 };
#define IMM atomnum, immoff
#define FIMM atomnum, fimmoff
#define DIMM atomnum, dimmoff
#define LIMM atomnum, limmoff
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

/*
 * Memory fields
 */

int gmem[] = { 'g', 0 };
int gdmem[] = { 'g', 1 };
#define GLOBAL atommem, gmem
#define GLOBALD atommem, gdmem
void atommem APROTO {
	const int *n = v;
	uint32_t delta = BF(26, 32);
	fprintf (out, " %s%c", ccy, n[0]);
	if (n[0] == 'c')
		fprintf (out, "%lld", BF(0x2a, 4));
	fprintf (out, "[%s$r%lld%s", cbl, BF(20, 6), n[1]?"d":"");
	if (delta & 0x80000000U)
		fprintf (out, "%s-%s%#x%s]", ccy, cyel, -delta, ccy);
	else
		fprintf (out, "%s+%s%#x%s]", ccy, cyel, delta, ccy);
}

/*
 * The instructions
 */

F(gmem, 0x3a, GLOBAL, GLOBALD)
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
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, IMM },
	{ AP, 0, 0, OOPS },
};

struct insn tabfs2[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, SRC2 },
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
F(us32, 5, N("u32"), N("s32"))

F1(pnot1, 0x17, N("not"))
F1(pnot2, 0x1d, N("not"))
F1(pnot3, 0x34, N("not"))

struct insn tabsetlop[] = {
	{ AP, 0x000e000000000000ull, 0x006e000000000000ull },	// noop, really "and $p7"
	{ AP, 0x0000000000000000ull, 0x0060000000000000ull, N("and"), T(pnot3), PSRC3 },
	{ AP, 0x0020000000000000ull, 0x0060000000000000ull, N("or"), T(pnot3), PSRC3 },
	{ AP, 0x0040000000000000ull, 0x0060000000000000ull, N("xor"), T(pnot3), PSRC3 },
	{ AP, 0, 0, OOPS, T(pnot3), PSRC3 },
};

struct insn tabm[] = {
	{ AP, 0x080e000000000000ull, 0xf81e000000000007ull, N("min"), N("f32"), DST, T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2) }, // looks like min/max is selected by a normal predicate. fun. needs to be checked
	{ AP, 0x081e000000000000ull, 0xf81e000000000007ull, N("max"), N("f32"), DST, T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2) },
	{ AP, 0x1000000000000000ull, 0xf800000000000007ull, N("set"), DST, T(setit), N("f32"), T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2), T(setlop) },
	{ AP, 0x200000000001c000ull, 0xf80000000001c007ull, N("set"), PDST, T(setit), N("f32"), T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2), T(setlop) }, // and these unknown bits are what? another predicate?
	{ AP, 0x3000000000000000ull, 0xf800000000000007ull, N("add"), T(fmf), T(ias), T(farm), N("f32"), DST, T(neg1), N("mul"), SRC1, T(fs2), T(neg2), SRC3 },
	{ AP, 0x5000000000000000ull, 0xf800000000000007ull, N("add"), T(faf), T(fas), T(farm), N("f32"), DST, T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2) },
	{ AP, 0x5800000000000000ull, 0xf800000000000007ull, N("mul"), T(fmf), T(ias), T(farm), T(fmneg), N("f32"), DST, SRC1, T(fs2) },
	{ AP, 0x0000000000000000ull, 0x0000000000000007ull, OOPS, T(farm), N("f32"), DST, SRC1, T(fs2), SRC3 },


	{ AP, 0x080e000000000001ull, 0xf81e000000000007ull, N("min"), N("f64"), DSTD, T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2) },
	{ AP, 0x081e000000000001ull, 0xf81e000000000007ull, N("max"), N("f64"), DSTD, T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2) },
	{ AP, 0x1000000000000001ull, 0xf800000000000007ull, N("set"), DST, T(setit), N("f64"), T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2), T(setlop) },
	{ AP, 0x180000000001c001ull, 0xf80000000001c007ull, N("set"), PDST, T(setit), N("f64"), T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2), T(setlop) },
	{ AP, 0x2000000000000001ull, 0xf800000000000007ull, N("add"), T(farm), N("f64"), DSTD, T(neg1), N("mul"), SRC1D, T(ds2), T(neg2), SRC3D },
	{ AP, 0x4800000000000001ull, 0xf800000000000007ull, N("add"), T(farm), N("f64"), DSTD, T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2) },
	{ AP, 0x5000000000000001ull, 0xf800000000000007ull, N("mul"), T(farm), T(neg1), N("f64"), DSTD, SRC1D, T(ds2) },
	{ AP, 0x0000000000000001ull, 0x0000000000000007ull, OOPS, T(farm), N("f64"), DSTD, SRC1D, T(ds2), SRC3D },


	{ AP, 0x0800000000000002ull, 0xf800000000000007ull, N("add"), T(ias), N("b32"), DST, SRC1, LIMM },
	{ AP, 0x0800000000000002ull, 0xf800000000000007ull, N("subr"), T(ias), N("b32"), DST, SRC1, LIMM },
	{ AP, 0x18000000000001e2ull, 0xf8000000000001e7ull, N("mov"), N("b32"), DST, LIMM }, // wanna bet these unknown bits are tesla-like lanemask?
	{ AP, 0x3800000000000002ull, 0xf8000000000000c7ull, N("and"), N("b32"), DST, SRC1, LIMM },
	{ AP, 0x3800000000000042ull, 0xf8000000000000c7ull, N("or"), N("b32"), DST, SRC1, LIMM },
	{ AP, 0x3800000000000082ull, 0xf8000000000000c7ull, N("xor"), N("b32"), DST, SRC1, LIMM },
	{ AP, 0x0000000000000002ull, 0x0000000000000007ull, OOPS, N("b32"), DST, SRC1, LIMM },


	{ AP, 0x080e000000000003ull, 0xf81e000000000007ull, N("min"), T(us32), DST, SRC1, T(is2) },
	{ AP, 0x081e000000000003ull, 0xf81e000000000007ull, N("max"), T(us32), DST, SRC1, T(is2) },
	{ AP, 0x1000000000000003ull, 0xf800000000000007ull, N("set"), DST, T(setit), T(us32), SRC1, T(is2), T(setlop) },
	{ AP, 0x180000000001c003ull, 0xf80000000001c007ull, N("set"), PDST, T(setit), T(us32), SRC1, T(is2), T(setlop) },
	{ AP, 0x4800000000000003ull, 0xf800000000000307ull, N("add"), T(ias), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x4800000000000103ull, 0xf800000000000307ull, N("sub"), T(ias), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x4800000000000203ull, 0xf800000000000307ull, N("subr"), T(ias), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x6800000000000003ull, 0xf8000000000000c7ull, N("and"), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x6800000000000043ull, 0xf8000000000000c7ull, N("or"), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x6800000000000083ull, 0xf8000000000000c7ull, N("xor"), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x68000000000001c3ull, 0xf8000000000001c7ull, N("not2"), N("b32"), DST, SRC1, T(is2) }, // yes, this is probably just a mov2 with a not bit set.
	{ AP, 0x0000000000000003ull, 0x0000000000000007ull, OOPS, N("b32"), DST, SRC1, T(is2), SRC3 },


	{ AP, 0x0c0e00000001c004ull, 0xfc0e0000c001c007ull, N("and"), PDST, T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ AP, 0x0c0e00004001c004ull, 0xfc0e0000c001c007ull, N("or"), PDST, T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ AP, 0x0c0e00008001c004ull, 0xfc0e0000c001c007ull, N("xor"), PDST, T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ AP, 0x2000000000000004ull, 0xf800000000000007ull, N("selp"), N("b32"), DST, SRC1, T(is2), T(pnot3), PSRC3 },
	{ AP, 0x28000000000001e4ull, 0xf8000000000001e7ull, N("mov"), N("b32"), DST, SRC2 },


	{ AP, 0x8000000000000105ull, 0xf800000000000107ull, N("mov"), T(ldstt), T(ldstd), T(gmem) }, // XXX wtf is this flag?
	{ AP, 0x9000000000000005ull, 0xf800000000000107ull, N("mov"), T(ldstt), T(gmem), T(ldstd) },
	{ AP, 0x0000000000000005ull, 0x0000000000000007ull, OOPS, T(ldstt), T(ldstd), T(gmem), SRC3 },


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
