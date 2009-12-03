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
 *    - add		TODO
 *    - sub		TODO
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
 *    - min		TODO
 *    - max		TODO
 *   2. Floating-Point
 *    - add		TODO
 *    - sub		TODO
 *    - mul		TODO
 *    - fma		TODO
 *    - mad		TODO
 *    - div.approxf32	TODO
 *    - div.full.f32	TODO
 *    - div.f64		TODO
 *    - abs		TODO
 *    - neg		TODO
 *    - min		TODO
 *    - max		TODO
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
 *    - and		TODO
 *    - or		TODO
 *    - xor		TODO
 *    - not		TODO
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
 *    - { }		TODO
 *    - @		TODO
 *    - bra		TODO
 *    - call		TODO
 *    - ret		TODO
 *    - exit		TODO
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
	struct atom atoms[10];
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
	fprintf (out, "\n                          ");
}

#define OOPS atomoops, 0
void atomoops APROTO {
	fprintf (out, " %s???", cred);
}

/*
 * Misc number fields
 */

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
int src2off[] = { 0x1a, 6, 'r' };
#define DST atomreg, dstoff
#define SRC1 atomreg, src1off
#define SRC2 atomreg, src2off
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


struct insn tabm[] = {
	{ AP, 0x4800000000001c03ull, 0xff00000000003fffull, N("add"), DST, SRC1, SRC2 },
	{ AP, 0x8000000000001de7ull, 0xff00000000003fffull, N("exit") },
	{ AP, 0x0, 0x0, OOPS },
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
		struct insn *tab = tabm;
		atomtab (out, &a, &m, tab, ptype);
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
