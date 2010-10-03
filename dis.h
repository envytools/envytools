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

#ifndef COREDIS_H
#define COREDIS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

extern char *cnorm;	// instruction code and misc stuff
extern char *cgray;	// instruction address
extern char *cgr;	// instruction name and mods
extern char *cbl;	// $r registers
extern char *ccy;	// memory accesses
extern char *cyel;	// numbers
extern char *cred;	// unknown stuff
extern char *cbr;	// code labels
extern char *cmag;	// funny registers

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

#define BF(s, l) (*m |= (((1ull<<l)-1)<<s), *a>>s&((1ull<<l)-1))
#define RCL(s, l) (*a &= ~(((1ull<<l)-1)<<s))

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

#define F(n, f, a, b) static struct insn tab ## n[] = {\
	{ ~0, 0,		1ull<<(f), a },\
	{ ~0, 1ull<<(f),	1ull<<(f), b },\
};
#define F1(n, f, b) static struct insn tab ## n[] = {\
	{ ~0, 0,		1ull<<(f) },\
	{ ~0, 1ull<<(f),	1ull<<(f), b },\
};

#define T(x) atomtab, tab ## x
void atomtab APROTO;

#define N(x) atomname, x
void atomname APROTO;

#define NL atomnl, 0
void atomnl APROTO;

#define U(x) atomunk, "unk" x
void atomunk APROTO;

#define OOPS atomunk, "???"
void atomunk APROTO;

void atomnum APROTO;

void atomign APROTO;

void atomreg APROTO;
void atomdreg APROTO;
void atomqreg APROTO;
void atomoreg APROTO;
void atomhreg APROTO;

uint32_t readle32 (uint8_t *);
uint16_t readle16 (uint8_t *);

#define VP 1
#define GP 2
#define FP 4
#define CP 8
#define AP (VP|GP|FP|CP)

void nv50dis (FILE *out, uint32_t *code, uint32_t start, int num, int ptype);
void nvc0dis (FILE *out, uint32_t *code, uint32_t start, int num, int ptype);

#define NV4x 1
#define NV5x 2 /* and 8x and 9x and Ax */
#define NVxx 3

void ctxdis (FILE *out, uint32_t *code, uint32_t start, int num, int ptype);

void fucdis (FILE *out, uint8_t *code, uint32_t start, int num, int ptype);

void pmdis (FILE *out, uint8_t *code, uint32_t start, int num, int ptype);

void vp2dis (FILE *out, uint8_t *code, uint32_t start, int num, int ptype);

#endif
