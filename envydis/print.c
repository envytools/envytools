/*
 * Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
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

void printexpr(FILE *out, const struct expr *expr, int lvl, const struct envy_colors *cols) {
	int i;
	switch (expr->type) {
		case EXPR_NUM:
			switch (expr->special) {
				case 1:
					fprintf(out, "%s", cols->btarg);
					break;
				case 2:
					fprintf(out, "%s", cols->ctarg);
					break;
				case 3:
					fprintf(out, "%s", cols->mem);
					break;
				default:
					fprintf(out, "%s", cols->num);
					break;
			}
			if (expr->num1 & 1ull << 63)
				fprintf(out, "-%#llx", -expr->num1);
			else
				fprintf(out, "%#llx", expr->num1);
			return;
		case EXPR_ID:
			switch (expr->special) {
				case 1:
					fprintf(out, "%s%s", cols->num, expr->str);
					break;
				case 2:
					fprintf(out, "%s%s", cols->err, expr->str);
					break;
				default:
					fprintf(out, "%s%s", cols->iname, expr->str);
					break;
			}
			return;
		case EXPR_LABEL:
			switch (expr->special) {
				case 2:
					fprintf(out, "%s#%s", cols->err, expr->str);
					break;
				default:
					fprintf(out, "%s#%s", cols->num, expr->str);
					break;
			}
			return;
		case EXPR_REG:
			switch (expr->special) {
				case 1:
					fprintf(out, "%s$%s", cols->regsp, expr->str);
					break;
				case 2:
					fprintf(out, "%s$%s", cols->err, expr->str);
					break;
				default:
					fprintf(out, "%s$%s", cols->reg, expr->str);
					break;
			}
			return;
		case EXPR_MEM:
			if (expr->str)
				fprintf(out, "%s%s[", cols->mem, expr->str);
			printexpr(out, expr->expr1, 0, cols);
			if (expr->str)
				fprintf(out, "%s]", cols->mem);
			return;
		case EXPR_VEC:
			fprintf(out, "%s{", cols->sym);
			for (i = 0; i < expr->vexprsnum; i++) {
				fprintf(out, " ");
				printexpr(out, expr->vexprs[i], 0, cols);
			}
			fprintf(out, " %s}", cols->sym);
			return;
		case EXPR_DISCARD:
			fprintf(out, "%s#", cols->sym);
			return;
		case EXPR_BITFIELD:
			fprintf(out, "%s%lld:%lld", cols->num, expr->num1, expr->num2);
			return;
		case EXPR_NEG:
			fprintf(out, "%s-", cols->sym);
			printexpr(out, expr->expr1, 7, cols);
			return;
		case EXPR_NOT:
			fprintf(out, "%s~", cols->sym);
			printexpr(out, expr->expr1, 7, cols);
			return;
		case EXPR_MUL:
			if (lvl >= 7)
				fprintf(out, "%s(", cols->sym);
			printexpr(out, expr->expr1, 6, cols);
			fprintf(out, "%s*", cols->sym);
			printexpr(out, expr->expr2, 7, cols);
			if (lvl >= 7)
				fprintf(out, "%s)", cols->sym);
			return;
		case EXPR_DIV:
			if (lvl >= 7)
				fprintf(out, "%s(", cols->sym);
			printexpr(out, expr->expr1, 6, cols);
			fprintf(out, "%s/", cols->sym);
			printexpr(out, expr->expr2, 7, cols);
			if (lvl >= 7)
				fprintf(out, "%s)", cols->sym);
			return;
		case EXPR_ADD:
			if (lvl >= 6)
				fprintf(out, "%s(", cols->sym);
			printexpr(out, expr->expr1, 5, cols);
			fprintf(out, "%s+", cols->sym);
			printexpr(out, expr->expr2, 6, cols);
			if (lvl >= 6)
				fprintf(out, "%s)", cols->sym);
			return;
		case EXPR_SUB:
			if (lvl >= 6)
				fprintf(out, "%s(", cols->sym);
			printexpr(out, expr->expr1, 5, cols);
			fprintf(out, "%s-", cols->sym);
			printexpr(out, expr->expr2, 6, cols);
			if (lvl >= 6)
				fprintf(out, "%s)", cols->sym);
			return;
		case EXPR_SHL:
			if (lvl >= 5)
				fprintf(out, "%s(", cols->sym);
			printexpr(out, expr->expr1, 4, cols);
			fprintf(out, "%s<<", cols->sym);
			printexpr(out, expr->expr2, 5, cols);
			if (lvl >= 5)
				fprintf(out, "%s)", cols->sym);
			return;
		case EXPR_SHR:
			if (lvl >= 5)
				fprintf(out, "%s(", cols->sym);
			printexpr(out, expr->expr1, 4, cols);
			fprintf(out, "%s>>", cols->sym);
			printexpr(out, expr->expr2, 5, cols);
			if (lvl >= 5)
				fprintf(out, "%s)", cols->sym);
			return;
		case EXPR_AND:
			if (lvl >= 4)
				fprintf(out, "%s(", cols->sym);
			printexpr(out, expr->expr1, 3, cols);
			fprintf(out, "%s&", cols->sym);
			printexpr(out, expr->expr2, 4, cols);
			if (lvl >= 4)
				fprintf(out, "%s)", cols->sym);
			return;
		case EXPR_XOR:
			if (lvl >= 3)
				fprintf(out, "%s(", cols->sym);
			printexpr(out, expr->expr1, 2, cols);
			fprintf(out, "%s^", cols->sym);
			printexpr(out, expr->expr2, 3, cols);
			if (lvl >= 3)
				fprintf(out, "%s)", cols->sym);
			return;
		case EXPR_OR:
			if (lvl >= 2)
				fprintf(out, "%s(", cols->sym);
			printexpr(out, expr->expr1, 1, cols);
			fprintf(out, "%s|", cols->sym);
			printexpr(out, expr->expr2, 2, cols);
			if (lvl >= 2)
				fprintf(out, "%s)", cols->sym);
			return;
		case EXPR_PIADD:
			if (lvl >= 1)
				fprintf(out, "%s(", cols->sym);
			printexpr(out, expr->expr1, 0, cols);
			fprintf(out, "%s++", cols->sym);
			printexpr(out, expr->expr2, 1, cols);
			if (lvl >= 1)
				fprintf(out, "%s)", cols->sym);
			return;
		case EXPR_PISUB:
			if (lvl >= 1)
				fprintf(out, "%s(", cols->sym);
			printexpr(out, expr->expr1, 0, cols);
			fprintf(out, "%s--", cols->sym);
			printexpr(out, expr->expr2, 1, cols);
			if (lvl >= 1)
				fprintf(out, "%s)", cols->sym);
			return;
		case EXPR_SESTART:
			fprintf(out, "%s(", cols->sym);
			return;
		case EXPR_SEEND:
			fprintf(out, "%s)", cols->sym);
			return;
		case EXPR_ED2A:
			if (lvl >= 1)
				fprintf(out, "%s(", cols->sym);
			ed2a_print_expr(expr->ed2a, out, cols, 0);
			if (lvl >= 1)
				fprintf(out, "%s)", cols->sym);
			return;
		default:
			assert(0);
	}
}
