/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
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

#ifndef ED2A_ASM_H
#define ED2A_ASM_H

#include "util.h"
#include "colors.h"
#include <stdio.h>

struct ed2a_file {
	struct ed2a_insn **insns;
	int insnsnum;
	int insnsmax;
	int broken;
	struct envy_loc loc;
};

struct ed2a_insn {
	struct ed2a_ipiece **pieces;
	int piecesnum;
	int piecesmax;
	struct envy_loc loc;
};

struct ed2a_ipiece {
	struct ed2a_expr **prefs;
	int prefsnum;
	int prefsmax;
	char *name;
	struct ed2a_iop **iops;
	int iopsnum;
	int iopsmax;
	char **mods;
	int modsnum;
	int modsmax;
	struct envy_loc loc;
};

struct ed2a_iop {
	char **mods;
	int modsnum;
	int modsmax;
	struct ed2a_expr **exprs;
	int exprsnum;
	int exprsmax;
	struct envy_loc loc;
};

struct ed2a_expr {
	enum ed2a_etype {
		ED2A_ET_PLUS,
		ED2A_ET_MINUS,
		ED2A_ET_MUL,
		ED2A_ET_UMINUS,
		ED2A_ET_SWZ,
		ED2A_ET_DISCARD,
		ED2A_ET_IPIECE,
		ED2A_ET_MEM,
		ED2A_ET_MEMPOSTI,
		ED2A_ET_MEMPOSTD,
		ED2A_ET_MEMPREI,
		ED2A_ET_MEMPRED,
		ED2A_ET_LABEL,
		ED2A_ET_NUM,
		ED2A_ET_NUM2,
		ED2A_ET_REG,
		ED2A_ET_REG2,
		ED2A_ET_RVEC,
		ED2A_ET_STR,
		ED2A_ET_ERR,
	} type;
	struct ed2a_expr *e1;
	struct ed2a_expr *e2;
	uint64_t num, num2;
	char *str;
	char *str2;
	struct ed2a_ipiece *ipiece;
	struct ed2a_rvec *rvec;
	struct ed2a_swz *swz;
	struct envy_loc loc;
};

struct ed2a_rvec {
	char **elems;
	int elemsnum;
	int elemsmax;
};

struct ed2a_swz {
	enum ed2a_swz_style {
		ED2A_SWZ_UNK,
		ED2A_SWZ_xyzw,
		ED2A_SWZ_XYZW,
		ED2A_SWZ_rgba,
		ED2A_SWZ_RGBA,
		ED2A_SWZ_stpq,
		ED2A_SWZ_STPQ,
		ED2A_SWZ_lh,
		ED2A_SWZ_LH,
		ED2A_SWZ_NUM,
	} style;
	uint64_t *elems;
	int elemsnum;
	int elemsmax;
};

struct ed2a_expr *ed2a_make_expr(enum ed2a_etype type);
struct ed2a_expr *ed2a_make_expr_bi(enum ed2a_etype type, struct ed2a_expr *, struct ed2a_expr *);
struct ed2a_expr *ed2a_make_expr_mem(enum ed2a_etype type, char *str, struct ed2a_expr *, struct ed2a_expr *);
struct ed2a_expr *ed2a_make_expr_swz(struct ed2a_expr *, struct ed2a_swz *);
struct ed2a_expr *ed2a_make_expr_str(enum ed2a_etype type, char *str);
struct ed2a_expr *ed2a_make_expr_ipiece(struct ed2a_ipiece *);
struct ed2a_expr *ed2a_make_expr_reg2(char *str, char *str2);
struct ed2a_expr *ed2a_make_expr_num(uint64_t);
struct ed2a_expr *ed2a_make_expr_num2(uint64_t, uint64_t);
struct ed2a_expr *ed2a_make_expr_rvec(struct ed2a_rvec *rv);
struct ed2a_insn *ed2a_make_label_insn(char *str);
struct ed2a_swz *ed2a_make_swz_empty();
struct ed2a_swz *ed2a_make_swz_num(uint64_t num);
struct ed2a_swz *ed2a_make_swz_str(char *str);
struct ed2a_swz *ed2a_make_swz_cat(struct ed2a_swz *a, struct ed2a_swz *b);

void ed2a_del_file(struct ed2a_file *file);
void ed2a_del_insn(struct ed2a_insn *insn);
void ed2a_del_ipiece(struct ed2a_ipiece *ipiece);
void ed2a_del_iop(struct ed2a_iop *iop);
void ed2a_del_expr(struct ed2a_expr *expr);
void ed2a_del_rvec(struct ed2a_rvec *rvec);
void ed2a_del_swz(struct ed2a_swz *swz);

void ed2a_print_file(struct ed2a_file *file, FILE *ofile, const struct envy_colors *col);
void ed2a_print_insn(struct ed2a_insn *insn, FILE *ofile, const struct envy_colors *col);
void ed2a_print_ipiece(struct ed2a_ipiece *ipiece, FILE *ofile, const struct envy_colors *col);
void ed2a_print_iop(struct ed2a_iop *iop, FILE *ofile, const struct envy_colors *col);
void ed2a_print_expr(struct ed2a_expr *expr, FILE *ofile, const struct envy_colors *col, int prio);

struct ed2a_file *ed2a_read_file (FILE *file, const char *filename, void (*fun) (struct ed2a_insn *insn, void *parm), void *parm);

#endif
