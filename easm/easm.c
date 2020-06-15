/*
 * Copyright (C) 2011-2012 Marcelina Kościelnicka <mwk@0x04.net>
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

#include "easm.h"
#include <stdlib.h>

struct easm_expr *easm_expr_bin(enum easm_expr_type type, struct easm_expr *e1, struct easm_expr *e2) {
	struct easm_expr *res = calloc(sizeof *res, 1);
	res->type = type;
	res->e1 = e1;
	res->e2 = e2;
	return res;
}

struct easm_expr *easm_expr_un(enum easm_expr_type type, struct easm_expr *e1) {
	struct easm_expr *res = calloc(sizeof *res, 1);
	res->type = type;
	res->e1 = e1;
	return res;
}

struct easm_expr *easm_expr_num(enum easm_expr_type type, uint64_t num) {
	struct easm_expr *res = calloc(sizeof *res, 1);
	res->type = type;
	res->num = num;
	return res;
}

struct easm_expr *easm_expr_str(enum easm_expr_type type, char *str) {
	struct easm_expr *res = calloc(sizeof *res, 1);
	res->type = type;
	res->str = str;
	return res;
}

struct easm_expr *easm_expr_astr(struct astr astr) {
	struct easm_expr *res = calloc(sizeof *res, 1);
	res->type = EASM_EXPR_STR;
	res->astr = astr;
	return res;
}

struct easm_expr *easm_expr_sinsn(struct easm_sinsn *sinsn) {
	struct easm_expr *res = calloc(sizeof *res, 1);
	res->type = EASM_EXPR_SINSN;
	res->sinsn = sinsn;
	return res;
}

struct easm_expr *easm_expr_simple(enum easm_expr_type type) {
	struct easm_expr *res = calloc(sizeof *res, 1);
	res->type = type;
	return res;
}

void easm_del_mod(struct easm_mod *mod) {
	if (!mod)
		return;
	free(mod->str);
	free(mod);
}

void easm_del_mods(struct easm_mods *mods) {
	if (!mods) return;
	int i;
	for (i = 0; i < mods->modsnum; i++)
		easm_del_mod(mods->mods[i]);
	free(mods->mods);
	free(mods);
}

void easm_del_operand(struct easm_operand *operand) {
	if (!operand) return;
	easm_del_mods(operand->mods);
	int i;
	for (i = 0; i < operand->exprsnum; i++)
		easm_del_expr(operand->exprs[i]);
	free(operand->exprs);
	free(operand);
}

void easm_del_expr(struct easm_expr *expr) {
	if (!expr) return;
	int i;
	for (i = 0; i < expr->swizzlesnum; i++)
		free(expr->swizzles[i].str);
	free(expr->swizzles);
	easm_del_expr(expr->e1);
	easm_del_expr(expr->e2);
	free(expr->astr.str);
	free(expr->str);
	easm_del_sinsn(expr->sinsn);
	easm_del_mods(expr->mods);
	free(expr);
}

void easm_del_sinsn(struct easm_sinsn *sinsn) {
	if (!sinsn) return;
	int i;
	for (i = 0; i < sinsn->operandsnum; i++)
		easm_del_operand(sinsn->operands[i]);
	free(sinsn->operands);
	free(sinsn->str);
	easm_del_mods(sinsn->mods);
	free(sinsn);
}

void easm_del_directive(struct easm_directive *directive) {
	if (!directive) return;
	int i;
	for (i = 0; i < directive->paramsnum; i++)
		easm_del_expr(directive->params[i]);
	free(directive->params);
	free(directive->str);
	free(directive);
}

void easm_del_subinsn(struct easm_subinsn *subinsn) {
	if (!subinsn) return;
	int i;
	for (i = 0; i < subinsn->prefsnum; i++)
		easm_del_expr(subinsn->prefs[i]);
	free(subinsn->prefs);
	easm_del_sinsn(subinsn->sinsn);
	free(subinsn);
}

void easm_del_insn(struct easm_insn *insn) {
	if (!insn) return;
	int i;
	for (i = 0; i < insn->subinsnsnum; i++)
		easm_del_subinsn(insn->subinsns[i]);
	free(insn->subinsns);
	free(insn);
}

void easm_del_line(struct easm_line *line) {
	if (!line) return;
	free(line->lname);
	easm_del_insn(line->insn);
	easm_del_directive(line->directive);
	free(line);
}

void easm_del_file(struct easm_file *file) {
	if (!file) return;
	int i;
	for (i = 0; i < file->linesnum; i++)
		easm_del_line(file->lines[i]);
	free(file->lines);
	free(file);
}

int easm_isimm(struct easm_expr *expr) {
	switch (expr->type) {
		case EASM_EXPR_LOR:
		case EASM_EXPR_LAND:
		case EASM_EXPR_OR:
		case EASM_EXPR_XOR:
		case EASM_EXPR_AND:
		case EASM_EXPR_SHL:
		case EASM_EXPR_SHR:
		case EASM_EXPR_ADD:
		case EASM_EXPR_SUB:
		case EASM_EXPR_MUL:
		case EASM_EXPR_DIV:
		case EASM_EXPR_MOD:
			return easm_isimm(expr->e1) && easm_isimm(expr->e2);
		case EASM_EXPR_NEG:
		case EASM_EXPR_NOT:
		case EASM_EXPR_LNOT:
			return easm_isimm(expr->e1);
		case EASM_EXPR_NUM:
		case EASM_EXPR_LABEL:
			return 1;
		default:
			return 0;
	}
}
