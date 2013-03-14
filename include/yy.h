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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef YY_H
#define YY_H

#include "util.h"

struct yy_lex_intern {
	int line;
	int pos;
	const char *file;
	int ws;
	int nest;
};

#define YYLTYPE struct envy_loc

#define YYLLOC_DEFAULT(Current, Rhs, N)					\
	do {								\
		if (N) {						\
			(Current).lstart = YYRHSLOC(Rhs, 1).lstart;	\
			(Current).cstart = YYRHSLOC(Rhs, 1).cstart;	\
			(Current).lend = YYRHSLOC(Rhs, N).lend;		\
			(Current).cend = YYRHSLOC(Rhs, N).cend;		\
			(Current).file = YYRHSLOC(Rhs, 1).file;		\
		} else {						\
			(Current).lstart = yylloc.lstart;	\
			(Current).cstart = yylloc.cstart;	\
			(Current).lend = yylloc.lstart;		\
			(Current).cend = yylloc.cstart;		\
			(Current).file = yylloc.file;		\
		}							\
	} while(0)

#define YY_USER_ACTION yy_lex_common(&yyextra, yylloc, yytext);
#define YY_USER_INIT yyextra.line = 1; yyextra.pos = 1; yyextra.ws = 0; yylloc->lend = 1; yylloc->cend = 1; yylloc->lstart = 1; yylloc->cstart = 1; yylloc->file = yyextra.file; yyextra.nest = 0;

void yy_lex_common(struct yy_lex_intern *x, YYLTYPE *loc, const char *str);

void yy_str_deescape(const char *str, struct astr *astr);

#endif
