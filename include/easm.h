/*
 * Copyright (C) 2011-2012 Marcin Kościelnicki <koriakin@0x04.net>
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

#ifndef EASM_H
#define EASM_H

#include "util.h"
#include "colors.h"
#include <stdio.h>

struct easm_swizzle {
	char *str;
	uint64_t num;
};

struct easm_mod {
	char *str;
	int isunk;
	struct envy_loc loc;
};

struct easm_mods {
	struct easm_mod **mods;
	int modsnum;
	int modsmax;
	struct envy_loc loc;
};

struct easm_expr {
	enum easm_expr_type {
		/* binary */
		EASM_EXPR_VEC,
		EASM_EXPR_LOR,
		EASM_EXPR_LAND,
		EASM_EXPR_OR,
		EASM_EXPR_XOR,
		EASM_EXPR_AND,
		EASM_EXPR_SHL,
		EASM_EXPR_SHR,
		EASM_EXPR_ADD,
		EASM_EXPR_SUB,
		EASM_EXPR_MUL,
		EASM_EXPR_DIV,
		EASM_EXPR_MOD,
		/* unary */
		EASM_EXPR_NEG,
		EASM_EXPR_NOT,
		EASM_EXPR_LNOT,
		/* other */
		EASM_EXPR_NUM,
		EASM_EXPR_REG,
		EASM_EXPR_LABEL,
		EASM_EXPR_STR,
		EASM_EXPR_DISCARD,
		EASM_EXPR_SINSN,
		EASM_EXPR_SWIZZLE,
		EASM_EXPR_MEM,
		EASM_EXPR_MEMPP,
		EASM_EXPR_MEMMM,
		EASM_EXPR_MEMPE,
		EASM_EXPR_MEMME,
		EASM_EXPR_POS,
	} type;
	enum easm_expr_special {
		EASM_SPEC_NONE,
		EASM_SPEC_ERR,
		EASM_SPEC_BTARG,
		EASM_SPEC_CTARG,
		EASM_SPEC_LITERAL,
		EASM_SPEC_REGSP,
	} special;
	struct easm_expr *e1;
	struct easm_expr *e2;
	struct easm_sinsn *sinsn;
	struct easm_swizzle *swizzles;
	int swizzlesnum;
	int swizzlesmax;
	uint64_t num;
	int bitsize;
	char *str;
	struct astr astr;
	struct easm_mods *mods;
	struct envy_loc loc;
	char *alabel;
	uint64_t alit;
};

struct easm_directive {
	char *str;
	struct easm_expr **params;
	int paramsnum;
	int paramsmax;
	struct envy_loc loc;
};

struct easm_operand {
	struct easm_mods *mods;
	struct easm_expr **exprs;
	int exprsnum;
	int exprsmax;
	struct envy_loc loc;
};

struct easm_sinsn {
	char *str;
	int isunk;
	struct easm_mods *mods;
	struct easm_operand **operands;
	int operandsnum;
	int operandsmax;
	struct envy_loc loc;
};

struct easm_subinsn {
	struct easm_expr **prefs;
	int prefsnum;
	int prefsmax;
	struct easm_sinsn *sinsn;
	struct envy_loc loc;
};

struct easm_insn {
	struct easm_subinsn **subinsns;
	int subinsnsnum;
	int subinsnsmax;
	struct envy_loc loc;
};

struct easm_line {
	enum {
		EASM_LINE_INSN,
		EASM_LINE_DIRECTIVE,
		EASM_LINE_LABEL,
	} type;
	struct easm_insn *insn;
	struct easm_directive *directive;
	char *lname;
	struct envy_loc loc;
};

struct easm_file {
	struct easm_line **lines;
	int linesnum;
	int linesmax;
};

struct easm_expr *easm_expr_bin(enum easm_expr_type type, struct easm_expr *e1, struct easm_expr *e2);
struct easm_expr *easm_expr_un(enum easm_expr_type type, struct easm_expr *e1);
struct easm_expr *easm_expr_num(enum easm_expr_type type, uint64_t num);
struct easm_expr *easm_expr_str(enum easm_expr_type type, char *str);
struct easm_expr *easm_expr_astr(struct astr astr);
struct easm_expr *easm_expr_sinsn(struct easm_sinsn *sinsn);
struct easm_expr *easm_expr_simple(enum easm_expr_type type);

void easm_del_mod(struct easm_mod *mod);
void easm_del_mods(struct easm_mods *mods);
void easm_del_expr(struct easm_expr *expr);
void easm_del_operand(struct easm_operand *operand);
void easm_del_subinsn(struct easm_subinsn *subinsn);
void easm_del_sinsn(struct easm_sinsn *sinsn);
void easm_del_insn(struct easm_insn *insn);
void easm_del_directive(struct easm_directive *directive);
void easm_del_line(struct easm_line *line);
void easm_del_file(struct easm_file *file);

int easm_read_file(FILE *file, const char *filename, struct easm_file **res);

void easm_print_expr(FILE *out, const struct envy_colors *cols, struct easm_expr *expr, int lvl);
void easm_print_sexpr(FILE *out, const struct envy_colors *cols, struct easm_expr *expr, int lvl);
void easm_print_mod(FILE *out, const struct envy_colors *cols, struct easm_mod *mod);
void easm_print_mods(FILE *out, const struct envy_colors *cols, struct easm_mods *mods, int spcafter);
void easm_print_operand(FILE *out, const struct envy_colors *cols, struct easm_operand *operand);
void easm_print_sinsn(FILE *out, const struct envy_colors *cols, struct easm_sinsn *sinsn);
void easm_print_subinsn(FILE *out, const struct envy_colors *cols, struct easm_subinsn *subinsn);
void easm_print_insn(FILE *out, const struct envy_colors *cols, struct easm_insn *insn);

int easm_isimm(struct easm_expr *expr);

/* does const-folding of expression, returns 1 if folded to a simple EASM_EXPR_NUM, 0 otherwise */
int easm_cfold_expr(struct easm_expr *expr);
void easm_substpos_expr(struct easm_expr *expr, uint64_t val);

void easm_cfold_insn(struct easm_insn *insn);
void easm_substpos_insn(struct easm_insn *insn, uint64_t val);

#endif
