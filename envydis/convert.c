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

#include "dis.h"
#include "easm.h"

void convert_expr_top(struct line *line, struct easm_expr *expr);

void convert_mods(struct line *line, struct easm_mods *mods) {
	int i;
	for (i = 0; i < mods->modsnum; i++) {
		struct expr *e = makeex(EXPR_ID);
		e->str = mods->mods[i];
		ADDARRAY(line->atoms, e);
	}
}

void convert_operand(struct line *line, struct easm_operand *operand) {
	int i;
	convert_mods(line, operand->mods);
	for (i = 0; i < operand->exprsnum; i++) {
		/* XXX */
		convert_expr_top(line, operand->exprs[i]);
	}
}

void convert_sinsn(struct line *line, struct easm_sinsn *sinsn) {
	int i;
	struct expr *e = makeex(EXPR_ID);
	e->str = sinsn->str;
	ADDARRAY(line->atoms, e);
	for (i = 0; i < sinsn->operandsnum; i++) {
		convert_operand(line, sinsn->operands[i]);
	}
	convert_mods(line, sinsn->mods);
}

void convert_subinsn(struct line *line, struct easm_subinsn *subinsn) {
	int i;
	for (i = 0; i < subinsn->prefsnum; i++) {
		convert_expr_top(line, subinsn->prefs[i]);
	}
	convert_sinsn(line, subinsn->sinsn);
}

void convert_vec(struct expr *vec, struct easm_expr *expr) {
	if (expr->type == EASM_EXPR_VEC) {
		convert_vec(vec, expr->e1);
		convert_vec(vec, expr->e2);
	} else {
		struct expr *se = convert_expr(expr);
		ADDARRAY(vec->vexprs, se);
	}
}

struct expr *convert_expr(struct easm_expr *expr) {
	struct expr *res;
	switch (expr->type) {
		case EASM_EXPR_ADD:
			res = makebinex(EXPR_ADD, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case EASM_EXPR_SUB:
			res = makebinex(EXPR_SUB, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case EASM_EXPR_MUL:
			res = makebinex(EXPR_MUL, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case EASM_EXPR_DIV:
			res = makebinex(EXPR_DIV, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case EASM_EXPR_SHL:
			res = makebinex(EXPR_SHL, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case EASM_EXPR_SHR:
			res = makebinex(EXPR_SHR, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case EASM_EXPR_OR:
			res = makebinex(EXPR_OR, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case EASM_EXPR_XOR:
			res = makebinex(EXPR_XOR, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case EASM_EXPR_AND:
			res = makebinex(EXPR_AND, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case EASM_EXPR_NEG:
			res = makeunex(EXPR_NEG, convert_expr(expr->e1));
			break;
		case EASM_EXPR_NOT:
			res = makeunex(EXPR_NOT, convert_expr(expr->e1));
			break;
		case EASM_EXPR_DISCARD:
			res = makeex(EXPR_DISCARD);
			break;
		case EASM_EXPR_NUM:
			res = makeex(EXPR_NUM);
			res->num1 = expr->num;
			res->isimm = 1;
			break;
		case EASM_EXPR_LABEL:
			res = makeex(EXPR_LABEL);
			res->str = expr->str;
			res->isimm = 1;
			break;
		case EASM_EXPR_REG:
			res = makeex(EXPR_REG);
			res->str = expr->str;
			break;
		case EASM_EXPR_MEM:
			res = makeex(EXPR_MEM);
			res->str = expr->str;
			res->expr1 = convert_expr(expr->e1);
			break;
		case EASM_EXPR_MEMPP:
			res = makeex(EXPR_MEM);
			res->str = expr->str;
			res->expr1 = makebinex(EXPR_PIADD, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case EASM_EXPR_MEMMM:
			res = makeex(EXPR_MEM);
			res->str = expr->str;
			res->expr1 = makebinex(EXPR_PISUB, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case EASM_EXPR_VEC:
			if (expr->e1->type == EASM_EXPR_NUM && expr->e2->type == EASM_EXPR_NUM) {
				res = makeex(EXPR_BITFIELD);
				res->num1 = expr->e1->num;
				res->num2 = expr->e2->num;

			} else {
				res = makeex(EXPR_VEC);
				convert_vec(res, expr);
			}
			break;
		default:
			abort();
	}
	return res;
}

void convert_expr_top(struct line *line, struct easm_expr *expr) {
	if (expr->type == EASM_EXPR_SINSN) {
		ADDARRAY(line->atoms, makeex(EXPR_SESTART));
		convert_sinsn(line, expr->sinsn);
		ADDARRAY(line->atoms, makeex(EXPR_SEEND));
	} else {
		ADDARRAY(line->atoms, convert_expr(expr));
	}
}

void convert_insn(struct line *line, struct easm_insn *insn) {
	int i;
	for (i = 0; i < insn->subinsnsnum; i++) {
		/* XXX */
		convert_subinsn(line, insn->subinsns[i]);
	}
}
