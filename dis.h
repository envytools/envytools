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
extern char *cmag;	// funny registers, jump labels
extern char *cbr;	// call labels
extern char *cbrmag;	// call and jump labels

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
 * given bitfield and marks it as used in the mask. Args are
 * given as start bit, size in bits.
 *
 * Also, two simple ops are provided: N("string") prints a literal to output
 * and is supposed to be used for instruction names and various modifiers.
 * OOPS is for unknown encodings,
 */

typedef unsigned long long ull;

#define BF(s, l) (*m |= (((1ull<<l)-1)<<s), *a>>s&((1ull<<l)-1))

struct disctx;
struct disisa;

#define APROTO (struct disctx *ctx, ull *a, ull *m, const void *v)

typedef void (*afun) APROTO;

struct sbf {
	int pos;
	int len;
};

struct bitfield {
	struct sbf sbf[2];
	enum {
		BF_UNSIGNED,
		BF_SIGNED,
		BF_SLIGHTLY_SIGNED,
		BF_ULTRASIGNED,
		BF_LUT,
	} mode;
	int shr;
	int pcrel;
	ull addend;
	ull pospreadd; // <3 xtensa...
	ull *lut;
};

struct sreg {
	int num;
	const char *name;
	enum {
		SR_NAMED,
		SR_ZERO,
		SR_ONE,
		SR_DISCARD,
	} mode;
};

struct reg {
	const struct bitfield *bf;
	const char *name;
	const char *suffix;
	const struct sreg *specials;
	int always_special;
	int hilo;
};

struct mem {
	const char *name;
	const struct bitfield *idx;
	const struct reg *reg;
	const struct bitfield *imm;
	const struct reg *reg2;
	int reg2shr;
};

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

struct disctx {
	struct disisa *isa;
	FILE *out;
	uint8_t *code8;
	uint32_t *code32;
	uint64_t *code64;
	int *labels;
	uint32_t codebase;
	uint32_t codesz;
	int ptype;
	int oplen;
	uint32_t pos;
};

struct disisa {
	struct insn *troot;
	int maxoplen;
	int opunit;
	int posunit;
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

#define OP8 atomopl, op8len
#define OP16 atomopl, op16len
#define OP24 atomopl, op24len
#define OP32 atomopl, op32len
#define OP40 atomopl, op40len
#define OP64 atomopl, op64len
extern int op8len[];
extern int op16len[];
extern int op24len[];
extern int op32len[];
extern int op40len[];
extern int op64len[];
void atomopl APROTO;

#define N(x) atomname, x
void atomname APROTO;

#define U(x) atomunk, "unk" x
void atomunk APROTO;

#define OOPS atomunk, "???"
void atomunk APROTO;

void atomimm APROTO;
void atomctarg APROTO;
void atombtarg APROTO;

void atomign APROTO;

void atomreg APROTO;

void atommem APROTO;

ull getbf(const struct bitfield *bf, ull *a, ull *m, struct disctx *ctx);
#define GETBF(bf) getbf(bf, a, m, ctx)

uint32_t readle32 (uint8_t *);
uint16_t readle16 (uint8_t *);

void markct8(struct disctx *ctx, uint32_t ptr);
void markbt8(struct disctx *ctx, uint32_t ptr);

#define VP 1
#define GP 2
#define FP 4
#define CP 8
#define AP (VP|GP|FP|CP)

#define NV4x 1
#define NV5x 2 /* and 8x and 9x and Ax */
#define NVxx 3

extern struct disisa *nv50_isa;
extern struct disisa *nvc0_isa;
extern struct disisa *ctx_isa;
extern struct disisa *fuc_isa;
extern struct disisa *pms_isa;
extern struct disisa *vp2_isa;
extern struct disisa *vp3m_isa;
extern struct disisa *vp3t_isa;
extern struct disisa *macro_isa;

void envydis (struct disisa *isa, FILE *out, uint8_t *code, uint32_t start, int num, int ptype);

#endif
