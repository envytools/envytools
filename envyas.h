/*
 *
 * Copyright (C) 2009 Marcin Kościelnicki <koriakin@0x04.net>
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

#ifndef ENVYAS_H
#define ENVYAS_H
#include "dis.h"
#include "rnn.h"

struct file {
	struct line **lines;
	int linesnum;
	int linesmax;
};

struct line {
	enum ltype {
		LINE_INSN,
		LINE_LABEL,
	} type;
	char *str;
	struct expr **atoms;
	int atomsnum;
	int atomsmax;
};

struct expr {
	enum etype {
		EXPR_NUM,
		EXPR_ID,
		EXPR_REG,
		EXPR_MEM,
		EXPR_VEC,
		EXPR_DISCARD,
		EXPR_BITFIELD,
		EXPR_NEG,
		EXPR_NOT,
		EXPR_MUL,
		EXPR_DIV,
		EXPR_ADD,
		EXPR_SUB,
		EXPR_SHL,
		EXPR_SHR,
		EXPR_AND,
		EXPR_XOR,
		EXPR_OR,
	} type;
	char *str;
	ull num1, num2;
	struct expr *expr1;
	struct expr *expr2;
	struct expr **vexprs;
	int vexprsnum;
	int vexprsmax;
	int isimm;
};

static inline struct expr *makeex(enum etype type) {
	struct expr *res = calloc(sizeof(*res), 1);
	res->type = type;
	return res;
}

static inline struct expr *makeunex(enum etype type, struct expr *expr) {
	struct expr *res = makeex(type);
	res->expr1 = expr;
	res->isimm = expr->isimm;
	return res;
}

static inline struct expr *makebinex(enum etype type, struct expr *expr1, struct expr *expr2) {
	struct expr *res = makeex(type);
	res->expr1 = expr1;
	res->expr2 = expr2;
	res->isimm = expr1->isimm && expr2->isimm;
	return res;
}

int setsbf (struct match *res, int pos, int len, ull num);

int yyparse();
int envyas_process(struct file *file);

#endif
