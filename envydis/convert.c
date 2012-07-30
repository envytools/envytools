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
#include "envyas.h"
#include "easm.h"

void convert_expr_top(struct line *line, struct easm_expr *expr);

void convert_mods(struct line *line, struct easm_mods *mods) {
	int i;
	for (i = 0; i < mods->modsnum; i++) {
		struct litem *li = calloc(sizeof *li, 1);
		li->type = LITEM_NAME;
		li->str = mods->mods[i]->str;
		ADDARRAY(line->atoms, li);
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
	struct litem *li = calloc(sizeof *li, 1);
	li->type = LITEM_NAME;
	li->str = sinsn->str;
	ADDARRAY(line->atoms, li);
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

void convert_expr_top(struct line *line, struct easm_expr *expr) {
	if (expr->type == EASM_EXPR_SINSN) {
		struct litem *ses = calloc(sizeof *ses, 1);
		struct litem *see = calloc(sizeof *see, 1);
		ses->type = LITEM_SESTART;
		see->type = LITEM_SEEND;
		ADDARRAY(line->atoms, ses);
		convert_sinsn(line, expr->sinsn);
		ADDARRAY(line->atoms, see);
	} else {
		struct litem *li = calloc(sizeof *li, 1);
		li->type = LITEM_EXPR;
		li->expr = expr;
		ADDARRAY(line->atoms, li);
	}
}

void convert_insn(struct line *line, struct easm_insn *insn) {
	int i;
	for (i = 0; i < insn->subinsnsnum; i++) {
		/* XXX */
		convert_subinsn(line, insn->subinsns[i]);
	}
}
