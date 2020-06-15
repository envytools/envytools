/*
 * Copyright (C) 2009-2012 Marcelina Kościelnicka <mwk@0x04.net>
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef DIS_INTERN_H
#define DIS_INTERN_H

#include "dis.h"

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

#define MAXOPLEN (128/64)

struct iasctx;
struct disctx;
struct disisa;

#define APROTO (struct iasctx *ctx, const void *v, int spos)
#define DPROTO (struct disctx *ctx, ull *a, ull *m, const void *v)

struct matches;

typedef struct matches *(*afun) APROTO;
typedef void (*dfun) DPROTO;

struct sbf {
	int pos;
	int len;
};

struct rbitfield {
	struct sbf sbf[2];
	enum {
		RBF_UNSIGNED,
		RBF_SIGNED,
		RBF_SLIGHTLY_SIGNED,
		RBF_ULTRASIGNED,
	} mode;
	int shr;
	int pcrel;
	ull addend;
	ull pospreadd; // <3 xtensa...
	int wrapok;
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
	ull addend;
	ull *lut;
	ull xorend;
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
	int fmask;
};

struct reg {
	const struct bitfield *bf;
	const char *name;
	const char *suffix;
	const struct sreg *specials;
	int always_special;
	int hilo;
	int cool;
};

struct mem {
	const char *name;
	const struct bitfield *idx;
	const struct reg *reg;
	const struct rbitfield *imm;
	const struct reg *reg2;
	int reg2shr;
	int postincr; // nv50dis hack.
	int literal;
};

struct vec {
	char *name;
	struct bitfield *bf;
	struct bitfield *cnt;
	struct bitfield *mask;
	int cool;
};

struct atom {
	afun fun_as;
	dfun fun_dis;
	const void *arg;
};

struct insn {
	ull val;
	ull mask;
	struct atom atoms[32];
	int fmask;
	int ptype;
};

struct litem {
	enum etype {
		LITEM_NAME,
		LITEM_EXPR,
		LITEM_SESTART,
		LITEM_SEEND,
	} type;
	char *str;
	int isunk;
	struct easm_expr *expr;
};

/*
 * Makes a simple table for checking a single flag.
 *
 * Arguments: table name, flag position, ops for 0, ops for 1.
 */

#define F(n, f, a, b) static struct insn tab ## n[] = {\
	{ 0,		1ull<<(f), a },\
	{ 1ull<<(f),	1ull<<(f), b },\
	{ 0, 0, OOPS },\
};
#define F1(n, f, b) static struct insn tab ## n[] = {\
	{ 0,		1ull<<(f) },\
	{ 1ull<<(f),	1ull<<(f), b },\
	{ 0, 0, OOPS },\
};
#define F1V(n, v, f, b) static struct insn tab ## n[] = {\
	{ 0,		1ull<<(f), .fmask = v },\
	{ 1ull<<(f),	1ull<<(f), b, .fmask = v },\
	{ 0, 0 },\
};

#define T(x) atomtab_a, atomtab_d, tab ## x
struct matches *atomtab_a APROTO;
void atomtab_d DPROTO;

#define OP1B atomopl_a, atomopl_d, op1blen
#define OP2B atomopl_a, atomopl_d, op2blen
#define OP3B atomopl_a, atomopl_d, op3blen
#define OP4B atomopl_a, atomopl_d, op4blen
#define OP5B atomopl_a, atomopl_d, op5blen
#define OP6B atomopl_a, atomopl_d, op6blen
#define OP8B atomopl_a, atomopl_d, op8blen
extern int op1blen[];
extern int op2blen[];
extern int op3blen[];
extern int op4blen[];
extern int op5blen[];
extern int op6blen[];
extern int op8blen[];
struct matches *atomopl_a APROTO;
void atomopl_d DPROTO;

struct matches *atomnop_a APROTO;
void atomendmark_d DPROTO;
#define ENDMARK atomnop_a, atomendmark_d, 0

struct matches *atomsestart_a APROTO;
void atomsestart_d DPROTO;
#define SESTART atomsestart_a, atomsestart_d, 0

struct matches *atomseend_a APROTO;
void atomseend_d DPROTO;
#define SEEND atomseend_a, atomseend_d, 0

#define N(x) atomname_a, atomname_d, x
struct matches *atomname_a APROTO;
void atomname_d DPROTO;
#define C(x) atomcmd_a, atomcmd_d, x
struct matches *atomcmd_a APROTO;
void atomcmd_d DPROTO;

#define U(x) atomunk_a, atomunk_d, "unk" x
#define OOPS atomunk_a, atomunk_d, "???"
struct matches *atomunk_a APROTO;
void atomunk_d DPROTO;

#define DISCARD atomdiscard_a, atomdiscard_d, 0
struct matches *atomdiscard_a APROTO;
void atomdiscard_d DPROTO;

struct matches *atomimm_a APROTO;
void atomimm_d DPROTO;
#define atomimm atomimm_a, atomimm_d

struct matches *atomrimm_a APROTO;
void atomrimm_d DPROTO;
void atomctarg_d DPROTO;
void atombtarg_d DPROTO;
#define atomrimm atomrimm_a, atomrimm_d
#define atombtarg atomrimm_a, atombtarg_d
#define atomctarg atomrimm_a, atomctarg_d

void atomign_d DPROTO;
#define atomign atomnop_a, atomign_d

struct matches *atomreg_a APROTO;
void atomreg_d DPROTO;
#define atomreg atomreg_a, atomreg_d

struct matches *atommem_a APROTO;
void atommem_d DPROTO;
#define atommem atommem_a, atommem_d

struct matches *atomvec_a APROTO;
void atomvec_d DPROTO;
#define atomvec atomvec_a, atomvec_d

struct matches *atombf_a APROTO;
void atombf_d DPROTO;
#define atombf atombf_a, atombf_d

static inline int var_ok(int fmask, int ptype, struct varinfo *varinfo) {
	return (!fmask || (varinfo->fmask[0] & fmask) == fmask) && (!ptype || (varinfo->modes[0] != -1 && ptype & 1 << varinfo->modes[0]));
}

extern struct disisa g80_isa_s;
extern struct disisa gf100_isa_s;
extern struct disisa gk110_isa_s;
extern struct disisa gm107_isa_s;
extern struct disisa ctx_isa_s;
extern struct disisa falcon_isa_s;
extern struct disisa hwsq_isa_s;
extern struct disisa xtensa_isa_s;
extern struct disisa vuc_isa_s;
extern struct disisa macro_isa_s;
extern struct disisa vp1_isa_s;
extern struct disisa vcomp_isa_s;

#endif
