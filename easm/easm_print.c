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

void easm_print_expr(FILE *out, const struct envy_colors *cols, struct easm_expr *expr, int lvl) {
	if (lvl <= -1 && expr->type == EASM_EXPR_VEC) {
		easm_print_expr(out, cols, expr->e1, -1);
		fprintf(out, "%s:%s", cols->sym, cols->reset);
		easm_print_expr(out, cols, expr->e2, 0);
	} else if (lvl <= 0 && expr->type == EASM_EXPR_LOR) {
		easm_print_expr(out, cols, expr->e1, 0);
		fprintf(out, "%s||%s", cols->sym, cols->reset);
		easm_print_expr(out, cols, expr->e2, 1);
	} else if (lvl <= 1 && expr->type == EASM_EXPR_LAND) {
		easm_print_expr(out, cols, expr->e1, 1);
		fprintf(out, "%s&&%s", cols->sym, cols->reset);
		easm_print_expr(out, cols, expr->e2, 2);
	} else if (lvl <= 2 && expr->type == EASM_EXPR_OR) {
		easm_print_expr(out, cols, expr->e1, 2);
		fprintf(out, "%s|%s", cols->sym, cols->reset);
		easm_print_expr(out, cols, expr->e2, 3);
	} else if (lvl <= 3 && expr->type == EASM_EXPR_XOR) {
		easm_print_expr(out, cols, expr->e1, 3);
		fprintf(out, "%s^%s", cols->sym, cols->reset);
		easm_print_expr(out, cols, expr->e2, 4);
	} else if (lvl <= 4 && expr->type == EASM_EXPR_AND) {
		easm_print_expr(out, cols, expr->e1, 4);
		fprintf(out, "%s&%s", cols->sym, cols->reset);
		easm_print_expr(out, cols, expr->e2, 5);
	} else if (lvl <= 5 && expr->type == EASM_EXPR_SHL) {
		easm_print_expr(out, cols, expr->e1, 5);
		fprintf(out, "%s<<%s", cols->sym, cols->reset);
		easm_print_expr(out, cols, expr->e2, 6);
	} else if (lvl <= 5 && expr->type == EASM_EXPR_SHR) {
		easm_print_expr(out, cols, expr->e1, 5);
		fprintf(out, "%s>>%s", cols->sym, cols->reset);
		easm_print_expr(out, cols, expr->e2, 6);
	} else {
		easm_print_sexpr(out, cols, expr, 0);
	}
}

void easm_print_sexpr(FILE *out, const struct envy_colors *cols, struct easm_expr *expr, int lvl) {
	const char *scol = 0;
	switch (expr->special) {
		case EASM_SPEC_NONE:
		case EASM_SPEC_LITERAL:
			break;
		case EASM_SPEC_ERR:
			scol = cols->err;
			break;
		case EASM_SPEC_BTARG:
			scol = cols->btarg;
			break;
		case EASM_SPEC_CTARG:
			scol = cols->ctarg;
			break;
		case EASM_SPEC_REGSP:
			scol = cols->regsp;
			break;
		default:
			abort();
	}
	if (lvl <= -1 && expr->type == EASM_EXPR_VEC) {
		easm_print_sexpr(out, cols, expr->e1, -1);
		fprintf(out, "%s:%s", cols->sym, cols->reset);
		easm_print_sexpr(out, cols, expr->e2, 0);
	} else if (lvl <= 0 && expr->type == EASM_EXPR_ADD) {
		easm_print_sexpr(out, cols, expr->e1, 0);
		fprintf(out, "%s+%s", cols->sym, cols->reset);
		easm_print_sexpr(out, cols, expr->e2, 1);
	} else if (lvl <= 0 && expr->type == EASM_EXPR_SUB) {
		easm_print_sexpr(out, cols, expr->e1, 0);
		fprintf(out, "%s-%s", cols->sym, cols->reset);
		easm_print_sexpr(out, cols, expr->e2, 1);
	} else if (lvl <= 1 && expr->type == EASM_EXPR_MUL) {
		easm_print_sexpr(out, cols, expr->e1, 1);
		fprintf(out, "%s*%s", cols->sym, cols->reset);
		easm_print_sexpr(out, cols, expr->e2, 2);
	} else if (lvl <= 1 && expr->type == EASM_EXPR_DIV) {
		easm_print_sexpr(out, cols, expr->e1, 1);
		fprintf(out, "%s/%s", cols->sym, cols->reset);
		easm_print_sexpr(out, cols, expr->e2, 2);
	} else if (lvl <= 1 && expr->type == EASM_EXPR_MOD) {
		easm_print_sexpr(out, cols, expr->e1, 1);
		fprintf(out, "%s%%%s", cols->sym, cols->reset);
		easm_print_sexpr(out, cols, expr->e2, 2);
	} else if (lvl <= 2 && expr->type == EASM_EXPR_NEG) {
		fprintf(out, "%s-%s", cols->sym, cols->reset);
		easm_print_sexpr(out, cols, expr->e1, 2);
	} else if (lvl <= 2 && expr->type == EASM_EXPR_NOT) {
		fprintf(out, "%s~%s", cols->sym, cols->reset);
		easm_print_sexpr(out, cols, expr->e1, 2);
	} else if (lvl <= 2 && expr->type == EASM_EXPR_LNOT) {
		fprintf(out, "%s!%s", cols->sym, cols->reset);
		easm_print_sexpr(out, cols, expr->e1, 2);
	} else if (expr->type == EASM_EXPR_NUM) {
		fprintf(out, "%s0x%"PRIx64"%s", scol?scol:cols->num, expr->num, cols->reset);
		if (expr->alabel) {
			fprintf(out, " %s/*%s %s#%s%s %s*/%s", cols->comm, cols->reset, scol?scol:cols->num, expr->alabel, cols->reset, cols->comm, cols->reset);
		}
	} else if (expr->type == EASM_EXPR_REG) {
		fprintf(out, "%s$%s%s", scol?scol:cols->reg, expr->str, cols->reset);
	} else if (expr->type == EASM_EXPR_LABEL) {
		fprintf(out, "%s#%s%s", scol?scol:cols->num, expr->str, cols->reset);
	} else if (expr->type == EASM_EXPR_STR) {
		fprintf(out, "%s", scol?scol:cols->num);
		print_escaped_astr(out, &expr->astr);
		fprintf(out, "%s", cols->reset);
	} else if (expr->type == EASM_EXPR_DISCARD) {
		fprintf(out, "%s#%s", cols->sym, cols->reset);
	} else if (expr->type == EASM_EXPR_ZVEC) {
		fprintf(out, "%s()%s", cols->sym, cols->reset);
	} else if (expr->type == EASM_EXPR_SINSN) {
		fprintf(out, "%s(%s", cols->sym, cols->reset);
		easm_print_sinsn(out, cols, expr->sinsn);
		fprintf(out, "%s)%s", cols->sym, cols->reset);
	} else if (expr->type == EASM_EXPR_SWIZZLE) {
		int i;
		easm_print_sexpr(out, cols, expr->e1, 3);
		fprintf(out, "%s.", cols->rname);
		if (expr->swizzlesnum != 1) {
			fprintf(out, "(");
		}
		for (i = 0; i < expr->swizzlesnum; i++) {
			if (i)
				fprintf(out, " ");
			if (expr->swizzles[i].str) {
				fprintf(out, "%s", expr->swizzles[i].str);
			} else {
				fprintf(out, "%"PRIu64, expr->swizzles[i].num);
			}
		}
		if (expr->swizzlesnum != 1) {
			fprintf(out, ")");
		}
		fprintf(out, "%s", cols->reset);
	} else if (expr->type >= EASM_EXPR_MEM && expr->type <= EASM_EXPR_MEMME) {
		fprintf(out, "%s%s[%s", cols->mem, expr->str?expr->str:"", cols->reset);
		easm_print_mods(out, cols, expr->mods, 1);
		easm_print_expr(out, cols, expr->e1, -1);
		if (expr->type != EASM_EXPR_MEM) {
			switch(expr->type) {
				case EASM_EXPR_MEMPP:
					fprintf(out, "%s++%s", cols->mem, cols->reset);
					break;
				case EASM_EXPR_MEMMM:
					fprintf(out, "%s--%s", cols->mem, cols->reset);
					break;
				case EASM_EXPR_MEMPE:
					fprintf(out, "%s+=%s", cols->mem, cols->reset);
					break;
				case EASM_EXPR_MEMME:
					fprintf(out, "%s-=%s", cols->mem, cols->reset);
					break;
				default:
					abort();
			}
			easm_print_expr(out, cols, expr->e2, -1);
		}
		fprintf(out, "%s]%s", cols->mem, cols->reset);
		if (expr->special == EASM_SPEC_LITERAL)
			fprintf(out, " %s/*%s %s0x%"PRIx64"%s %s*/%s", cols->comm, cols->reset, scol?scol:cols->num, expr->alit, cols->reset, cols->comm, cols->reset);
	} else if (expr->type == EASM_EXPR_POS) {
		fprintf(out, "%s$%s", cols->sym, cols->reset);
	} else {
		fprintf(out, "%s(%s", cols->sym, cols->reset);
		easm_print_expr(out, cols, expr, -1);
		fprintf(out, "%s)%s", cols->sym, cols->reset);
	}
}

void easm_print_mod(FILE *out, const struct envy_colors *cols, struct easm_mod *mod) {
	if (mod->isunk)
		fprintf(out, "%s%s%s", cols->err, mod->str, cols->reset);
	else
		fprintf(out, "%s%s%s", cols->mod, mod->str, cols->reset);
}

void easm_print_mods(FILE *out, const struct envy_colors *cols, struct easm_mods *mods, int spcafter) {
	int i;
	for (i = 0; i < mods->modsnum; i++) {
		if (!spcafter)
			fprintf(out, " ");
		easm_print_mod(out, cols, mods->mods[i]);
		if (spcafter)
			fprintf(out, " ");
	}
}

void easm_print_operand(FILE *out, const struct envy_colors *cols, struct easm_operand *operand) {
	int i;
	easm_print_mods(out, cols, operand->mods, 0);
	for (i = 0; i < operand->exprsnum; i++) {
		if (i)
			fprintf(out, " %s|%s", cols->sym, cols->reset);
		fprintf(out, " ");
		easm_print_sexpr(out, cols, operand->exprs[i], -1);
	}
}

void easm_print_sinsn(FILE *out, const struct envy_colors *cols, struct easm_sinsn *sinsn) {
	int i;
	if (sinsn->isunk)
		fprintf(out, "%s%s%s", cols->err, sinsn->str, cols->reset);
	else
		fprintf(out, "%s%s%s", cols->iname, sinsn->str, cols->reset);
	for (i = 0; i < sinsn->operandsnum; i++) {
		easm_print_operand(out, cols, sinsn->operands[i]);
	}
	easm_print_mods(out, cols, sinsn->mods, 0);
}

void easm_print_subinsn(FILE *out, const struct envy_colors *cols, struct easm_subinsn *subinsn) {
	int i;
	for (i = 0; i < subinsn->prefsnum; i++) {
		easm_print_sexpr(out, cols, subinsn->prefs[i], 2);
		fprintf(out, " ");
	}
	easm_print_sinsn(out, cols, subinsn->sinsn);
}

void easm_print_insn(FILE *out, const struct envy_colors *cols, struct easm_insn *insn) {
	int i;
	for (i = 0; i < insn->subinsnsnum; i++) {
		if (i)
			fprintf(out, " %s&%s ", cols->sym, cols->reset);
		easm_print_subinsn(out, cols, insn->subinsns[i]);
	}
}
