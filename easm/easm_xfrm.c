/*
 * Copyright (C) 2012 Marcelina Kościelnicka <mwk@0x04.net>
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

void easm_cfold_sinsn(struct easm_sinsn *sinsn) {
	int i, j;
	for (i = 0; i < sinsn->operandsnum; i++)
		for (j = 0; j < sinsn->operands[i]->exprsnum; j++)
			easm_cfold_expr(sinsn->operands[i]->exprs[j]);
}

int easm_cfold_expr(struct easm_expr *expr) {
	if (expr->type == EASM_EXPR_NUM)
		return 1;
	int e1f = 1, e2f = 1;
	if (expr->e1)
		e1f = easm_cfold_expr(expr->e1);
	if (expr->e2)
		e2f = easm_cfold_expr(expr->e2);
	if (expr->sinsn)
		easm_cfold_sinsn(expr->sinsn);
	if (!e1f || !e2f || expr->type < EASM_EXPR_LOR || expr->type > EASM_EXPR_LNOT)
		return 0;
	uint64_t val;
	uint64_t x;
	switch (expr->type) {
		case EASM_EXPR_NEG:
			val = -expr->e1->num;
			break;
		case EASM_EXPR_NOT:
			val = ~expr->e1->num;
			break;
		case EASM_EXPR_LNOT:
			val = !expr->e1->num;
			break;
		case EASM_EXPR_MUL:
			val = expr->e1->num * expr->e2->num;
			break;
		case EASM_EXPR_DIV:
			x = expr->e2->num;
			if (x)
				val = expr->e1->num / x;
			else {
				fprintf (stderr, LOC_FORMAT(expr->loc, "Division by 0\n"));
				return 0;
			}
			break;
		case EASM_EXPR_MOD:
			x = expr->e2->num;
			if (x)
				val = expr->e1->num % x;
			else {
				fprintf (stderr, LOC_FORMAT(expr->loc, "Division by 0\n"));
				return 0;
			}
			break;
		case EASM_EXPR_ADD:
			val = expr->e1->num + expr->e2->num;
			break;
		case EASM_EXPR_SUB:
			val = expr->e1->num - expr->e2->num;
			break;
		case EASM_EXPR_SHL:
			val = expr->e1->num << expr->e2->num;
			break;
		case EASM_EXPR_SHR:
			val = expr->e1->num >> expr->e2->num;
			break;
		case EASM_EXPR_AND:
			val = expr->e1->num & expr->e2->num;
			break;
		case EASM_EXPR_XOR:
			val = expr->e1->num ^ expr->e2->num;
			break;
		case EASM_EXPR_OR:
			val = expr->e1->num | expr->e2->num;
			break;
		case EASM_EXPR_LAND:
			val = expr->e1->num && expr->e2->num;
			break;
		case EASM_EXPR_LOR:
			val = expr->e1->num || expr->e2->num;
			break;
		default:
			abort();
	}
	easm_del_expr(expr->e1);
	easm_del_expr(expr->e2);
	expr->e1 = 0;
	expr->e2 = 0;
	expr->num = val;
	expr->type = EASM_EXPR_NUM;
	return 1;
}

void easm_cfold_insn(struct easm_insn *insn) {
	int i, j;
	for (i = 0; i < insn->subinsnsnum; i++) {
		for (j = 0; j < insn->subinsns[i]->prefsnum; j++)
			easm_cfold_expr(insn->subinsns[i]->prefs[j]);
		easm_cfold_sinsn(insn->subinsns[i]->sinsn);
	}
}

void easm_substpos_sinsn(struct easm_sinsn *sinsn, uint64_t val) {
	int i, j;
	for (i = 0; i < sinsn->operandsnum; i++)
		for (j = 0; j < sinsn->operands[i]->exprsnum; j++)
			easm_substpos_expr(sinsn->operands[i]->exprs[j], val);
}

void easm_substpos_expr(struct easm_expr *expr, uint64_t val) {
	if (expr->e1)
		easm_substpos_expr(expr->e1, val);
	if (expr->e2)
		easm_substpos_expr(expr->e2, val);
	if (expr->sinsn)
		easm_substpos_sinsn(expr->sinsn, val);
	if (expr->type == EASM_EXPR_POS) {
		expr->type = EASM_EXPR_NUM;
		expr->num = val;
	}
}

void easm_substpos_insn(struct easm_insn *insn, uint64_t val) {
	int i, j;
	for (i = 0; i < insn->subinsnsnum; i++) {
		for (j = 0; j < insn->subinsns[i]->prefsnum; j++)
			easm_substpos_expr(insn->subinsns[i]->prefs[j], val);
		easm_substpos_sinsn(insn->subinsns[i]->sinsn, val);
	}
}
