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
#include "ed2a.h"

void convert_expr_top(struct line *line, struct ed2a_expr *expr);

void convert_iop(struct line *line, struct ed2a_iop *iop) {
	int i;
	for (i = 0; i < iop->modsnum; i++) {
		struct expr *e = makeex(EXPR_ID);
		e->str = iop->mods[i];
		ADDARRAY(line->atoms, e);
	}
	for (i = 0; i < iop->exprsnum; i++) {
		convert_expr_top(line, iop->exprs[i]);
	}
}

void convert_ipiece(struct line *line, struct ed2a_ipiece *ipiece) {
	int i;
	for (i = 0; i < ipiece->prefsnum; i++) {
		convert_expr_top(line, ipiece->prefs[i]);
	}
	struct expr *e = makeex(EXPR_ID);
	e->str = ipiece->name;
	ADDARRAY(line->atoms, e);
	for (i = 0; i < ipiece->iopsnum; i++) {
		convert_iop(line, ipiece->iops[i]);
	}
	for (i = 0; i < ipiece->modsnum; i++) {
		struct expr *e = makeex(EXPR_ID);
		e->str = ipiece->mods[i];
		ADDARRAY(line->atoms, e);
	}
}

struct expr *convert_expr(struct ed2a_expr *expr) {
	struct expr *res;
	int i;
	switch (expr->type) {
		case ED2A_ET_PLUS:
			res = makebinex(EXPR_ADD, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case ED2A_ET_MINUS:
			res = makebinex(EXPR_SUB, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case ED2A_ET_MUL:
			res = makebinex(EXPR_MUL, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case ED2A_ET_UMINUS:
			res = makebinex(EXPR_SUB, makeex(EXPR_NUM), convert_expr(expr->e1));
			break;
		case ED2A_ET_DISCARD:
			res = makeex(EXPR_DISCARD);
			break;
		case ED2A_ET_NUM:
			res = makeex(EXPR_NUM);
			res->num1 = expr->num;
			res->isimm = 1;
			break;
		case ED2A_ET_NUM2:
			res = makeex(EXPR_BITFIELD);
			res->num1 = expr->num;
			res->num2 = expr->num2;
			break;
		case ED2A_ET_LABEL:
			res = makeex(EXPR_LABEL);
			res->str = expr->str;
			res->isimm = 1;
			break;
		case ED2A_ET_REG:
			res = makeex(EXPR_REG);
			res->str = expr->str;
			break;
		case ED2A_ET_MEM:
			res = makeex(EXPR_MEM);
			res->str = expr->str;
			res->expr1 = convert_expr(expr->e1);
			break;
		case ED2A_ET_MEMPOSTI:
			res = makeex(EXPR_MEM);
			res->str = expr->str;
			res->expr1 = makebinex(EXPR_PIADD, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case ED2A_ET_MEMPOSTD:
			res = makeex(EXPR_MEM);
			res->str = expr->str;
			res->expr1 = makebinex(EXPR_PISUB, convert_expr(expr->e1), convert_expr(expr->e2));
			break;
		case ED2A_ET_RVEC:
			res = makeex(EXPR_VEC);
			for (i = 0; i < expr->rvec->elemsnum; i++) {
				struct expr *se = makeex(EXPR_REG);
				se->str = expr->rvec->elems[i];
				ADDARRAY(res->vexprs, se);
			}
			break;
		default:
			res = makeex(EXPR_ED2A);
	}
	res->ed2a = expr;
	return res;
}

void convert_expr_top(struct line *line, struct ed2a_expr *expr) {
	if (expr->type == ED2A_ET_IPIECE) {
		ADDARRAY(line->atoms, makeex(EXPR_SESTART));
		convert_ipiece(line, expr->ipiece);
		ADDARRAY(line->atoms, makeex(EXPR_SEEND));
	} else {
		ADDARRAY(line->atoms, convert_expr(expr));
	}
}
