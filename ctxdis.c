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
 * Each op takes four arguments: output file to print to, pointer to the
 * opcode being decoded, pointer to a mask value of already-used fields,
 * and a free-form void * specified in table entry that called this op.
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
 

#define BF(s, l) (m[0] |= ((1<<l)-1<<s), a[0]>>s&(1<<l)-1)
#define RCL(s, l) (a[0] &= ~((1<<l)-1<<s))

#define APROTO (FILE *out, uint32_t *a, uint32_t *m, const void *v)

typedef void (*afun) APROTO;

struct atom {
	afun fun;
	const void *arg;
};

struct insn {
	uint32_t val;
	uint32_t mask;
	struct atom atoms[10];
};

#define T(x) atomtab, tab ## x
void atomtab APROTO {
	const struct insn *tab = v;
	int i;
	while ((a[0]&tab->mask) != tab->val)
		tab++;
	m[0] |= tab->mask;
	for (i = 0; i < 10; i++)
		if (tab->atoms[i].fun)
			tab->atoms[i].fun (out, a, m, tab->atoms[i].arg);
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
 *
 * This field represents a code address and is used for branching and the
 * likes. Target is counted in 32-bit words from the start of microcode.
 * It needs +4 offset for some reason.
 */

#define CTARG atomctarg, 0
void atomctarg APROTO {
	fprintf (out, " %s%#x", cbr, BF(8, 10)+1<<2);
}

/*
 * Misc number fields
 *
 * Used for plain numerical arguments.
 */

// BF, shift

int unitoff[] = { 0, 5, 0 };
int gsizeoff[] = { 16, 4, 0 };
int immoff[] = { 0, 20, 0 };
int goffoff[] = { 0, 16, 11 };
#define UNIT atomnum, unitoff
#define GSIZE atomnum, gsizeoff
#define IMM atomnum, immoff
#define GOFF atomnum, goffoff
void atomnum APROTO {
	const int *n = v;
	uint32_t num = BF(n[0], n[1])<<n[2];
	fprintf (out, " %s%#x", cyel, num);
}

/*
 * Registers
 */

#define R(x) atomreg, x
void atomreg APROTO {
	fprintf (out, " %s$%s", cbl, (char *)v);
}
#define RA R("a")	// Accumulator. Whatever.
#define RG R("g")	// PGRAPH offset register.
#define RR R("r")	// RAMIN offset register.
#define RM R("m")	// 0x40033c

/*
 * Memory fields
 */

// BF, offset shift, 'l'

int pgmem[] = { 0, 16, 2, 'G' };

#define PGRAPH atommem, pgmem
void atommem APROTO {
	const int *n = v;
	fprintf (out, " %s%c[", ccy, n[3]);
	if (n[3] == 'G') printf("%s$g%s+", cbl, ccy);
	int mo = BF(n[0], n[1])<<n[2];
	fprintf (out, "%s%#x%s]", cyel, mo, ccy);
}

struct insn tabrpred[] = {
	{ 0x00, 0x60, N("flag"), UNIT }, // if given flag set in PGRAPH+0x824
	{ 0x4d, 0x7f },	// always
	{ 0x60, 0x60, N("unit"), UNIT }, // if given unit present
	{ 0, 0, OOPS },
};

struct insn tabpred[] = {
	{ 0x80, 0x80, N("not"), T(rpred) },
	{ 0x00, 0x80, T(rpred) },
};

struct insn tabm[] = {
	{ 0x100000, 0xff0000, N("pgraph"), PGRAPH, RA },
	{ 0x100000, 0xf00000, N("pgraph"), PGRAPH, GSIZE },
	{ 0x200000, 0xf00000, N("mov"), RA, IMM },
	{ 0x300000, 0xff0000, N("mov"), RG, GOFF },
	{ 0x400000, 0xfc0000, N("jmp"), T(pred), CTARG },
	{ 0x440000, 0xfc0000, N("call"), T(pred), CTARG },
	{ 0x480000, 0xfc0000, N("ret"), T(pred) },
	{ 0x500000, 0xf00000, N("assert"), T(pred) },
	{ 0x600006, 0xffffff, N("mov"), RR, RA },
	{ 0x600007, 0xffffff, N("mov"), RM, RA },
	{ 0x60000c, 0xffffff, N("exit") },
	{ 0x700000, 0xf00080, N("clear"), T(rpred) },
	{ 0x700080, 0xf00080, N("set"), T(rpred) },
	{ 0, 0, OOPS },
};

/*
 * Disassembler driver
 *
 * You pass a block of memory to this function, disassembly goes out to given
 * FILE*.
 */

void nv50dis (FILE *out, uint32_t *code, int num) {
	int cur = 0;
	while (cur < num) {
		uint32_t a[1] = {code[cur++]}, m[1] = { 0 };
		fprintf (out, "%s%08x: %s", cgray, cur*4, cnorm);
		fprintf (out, "%08x         ", a[0]);
		atomtab (out, a, m, tabm);
		a[0] &= ~m[0];
		if (a[0]) {
			fprintf (out, " %s[unknown: %08x", cred, a[0]);
			fprintf (out, "]%s", cnorm);
		}
		printf ("%s\n", cnorm);
	}
}

int main(int argc, char **argv) {
	int num = 0;
	int maxnum = 16;
	uint32_t *code = malloc (maxnum * 4);
	uint32_t t;
	while (scanf ("%x", &t) == 1) {
		if (num == maxnum) maxnum *= 2, code = realloc (code, maxnum*4);
		code[num++] = t;
	}
	nv50dis (stdout, code, num);
	return 0;
}
