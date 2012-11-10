/*
 * Copyright (C) 2010-2012 Marcin Kościelnicki <koriakin@0x04.net>
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

%{
#include "easm.h"
#include "yy.h"
#include "easm_parse.h"
#include "easm_lex.h"
void easm_error(YYLTYPE *loc, void *lex_state, struct easm_file **res, const char *err) {
	fprintf(stderr, LOC_FORMAT(*loc, "%s\n"), err);
}
%}

%locations
%define api.pure
%name-prefix "easm_"
%lex-param { void *lex_state }
%parse-param { void *lex_state }
%parse-param { struct easm_file **res }
/* XXX */

%union {
	uint64_t num;
	char *str;
	struct astr astr;
	struct easm_file *file;
	struct easm_line *line;
	struct easm_insn *insn;
	struct easm_sinsn *sinsn;
	struct easm_subinsn *subinsn;
	struct easm_directive *direct;
	struct easm_expr *expr;
	struct easm_mod *mod;
	struct easm_mods *mods;
	struct easm_operand *operand;
}

/* XXX: numeric labels */
%token T_SHL
%token T_SHR
%token T_LOR
%token T_LAND
%token T_UMINUS
%token T_PLUSPLUS
%token T_MINUSMINUS
%token T_PLUSEQ
%token T_MINUSEQ
%token T_DOTLP
%token T_ERR
%token <num> T_NUM
%token <num> T_DOTNUM
%token <str> T_WORD
%token <str> T_WORDC
%token <str> T_WORDLB
%token <str> T_DOTWORD
%token <str> T_HASHWORD
%token <str> T_DOLWORD
%token <astr> T_STR

/* XXX: %type */
%type <file> file
%type <line> line
%type <insn> insn
%type <sinsn> sinsn operands
%type <subinsn> subinsn prefs
%type <direct> direct
%type <expr> expr expr0 expr1 expr2 expr3 expr4 expr5 sexpr sexpr0 sexpr1 pexpr aexpr lswizzle membody
%type <mod> mod
%type <mods> mods
%type <operand> operand

/* XXX: destructors */

%destructor { } <num>
%destructor { free($$); } <str>
%destructor { free($$.str); } <astr>
%destructor { easm_del_mod($$); } <mod>
%destructor { easm_del_mods($$); } <mods>
%destructor { easm_del_operand($$); } <operand>
%destructor { easm_del_expr($$); } <expr>
%destructor { easm_del_insn($$); } <insn>
%destructor { easm_del_subinsn($$); } <subinsn>
%destructor { easm_del_sinsn($$); } <sinsn>
%destructor { easm_del_directive($$); } <direct>
%destructor { easm_del_line($$); } <line>
%destructor { easm_del_file($$); } <file>

%%

start:	file		{ *res = $1; }

file:	file line	{ $$ = $1; if ($2) ADDARRAY($$->lines, $2); }
file:	/**/		{ $$ = calloc(sizeof *$$, 1); }

line:	direct eol	{ $$ = calloc(sizeof *$$, 1); $$->loc = @$; $$->type = EASM_LINE_DIRECTIVE; $$->directive = $1; }
line:	insn eol	{ $$ = calloc(sizeof *$$, 1); $$->loc = @$; $$->type = EASM_LINE_INSN; $$->insn = $1; }
line:	T_WORDC		{ $$ = calloc(sizeof *$$, 1); $$->loc = @$; $$->type = EASM_LINE_LABEL; $$->lname = $1; }
line:	eol		{ $$ = 0; }

eol:	'\n'
eol:	';'

direct:	T_DOTWORD	{ $$ = calloc(sizeof *$$, 1); $$->loc = @$; $$->str = $1; }
direct:	direct expr	{ $$ = $1; $$->loc = @$; ADDARRAY($$->params, $2); }

insn:	subinsn			{ $$ = calloc(sizeof *$$, 1); $$->loc = @$; ADDARRAY($$->subinsns, $1); }
insn:	insn '&' subinsn	{ $$ = $1; $$->loc = @$; ADDARRAY($$->subinsns, $3); }
insn:	insn '\n' '&' subinsn	{ $$ = $1; $$->loc = @$; ADDARRAY($$->subinsns, $4); }
insn:	insn '&' '\n' subinsn	{ $$ = $1; $$->loc = @$; ADDARRAY($$->subinsns, $4); }

subinsn:	prefs sinsn	{ $$ = $1; $$->loc = @$; $$->sinsn = $2; }

prefs:	prefs pexpr		{ $$ = $1; ADDARRAY($$->prefs, $2); }
prefs:	/**/			{ $$ = calloc(sizeof *$$, 1); }

sinsn:	T_WORD operands mods	{ $$ = $2; $$->loc = @$; $$->str = $1; $$->mods = $3; }

operands:	operands operand	{ $$ = $1; ADDARRAY($$->operands, $2); }
operands:	/**/		{ $$ = calloc(sizeof *$$, 1); }

operand:	mods sexpr	{ $$ = calloc(sizeof *$$, 1); $$->loc = @$; $$->mods = $1; ADDARRAY($$->exprs, $2); }
operand:	operand '|' sexpr	{ $$ = $1; $$->loc = @$; ADDARRAY($$->exprs, $3); }

mod:	T_WORD			{ $$ = calloc(sizeof *$$, 1); $$->str = $1; $$->loc = @$; }

mods:	mods mod		{ $$ = $1; ADDARRAY($$->mods, $2); $$->loc = @$; }
mods:	/**/			{ $$ = calloc(sizeof *$$, 1); $$->loc = @$; }

expr:	expr ':' expr0		{ $$ = easm_expr_bin(EASM_EXPR_VEC, $1, $3); $$->loc = @$; }
expr:	expr0

expr0:	expr0 T_LOR expr1	{ $$ = easm_expr_bin(EASM_EXPR_LOR, $1, $3); $$->loc = @$; }
expr0:	expr1

expr1:	expr1 T_LAND expr2	{ $$ = easm_expr_bin(EASM_EXPR_LAND, $1, $3); $$->loc = @$; }
expr1:	expr2

expr2:	expr2 '|' expr3		{ $$ = easm_expr_bin(EASM_EXPR_OR, $1, $3); $$->loc = @$; }
expr2:	expr3

expr3:	expr3 '^' expr4		{ $$ = easm_expr_bin(EASM_EXPR_XOR, $1, $3); $$->loc = @$; }
expr3:	expr4

expr4:	expr4 '&' expr5		{ $$ = easm_expr_bin(EASM_EXPR_AND, $1, $3); $$->loc = @$; }
expr4:	expr5

expr5:	expr5 T_SHL sexpr0	{ $$ = easm_expr_bin(EASM_EXPR_SHL, $1, $3); $$->loc = @$; }
expr5:	expr5 T_SHR sexpr0	{ $$ = easm_expr_bin(EASM_EXPR_SHR, $1, $3); $$->loc = @$; }
expr5:	sexpr0

sexpr:	sexpr ':' sexpr0	{ $$ = easm_expr_bin(EASM_EXPR_VEC, $1, $3); $$->loc = @$; }
sexpr:	sexpr0

sexpr0:	sexpr0 '+' sexpr1	{ $$ = easm_expr_bin(EASM_EXPR_ADD, $1, $3); $$->loc = @$; }
sexpr0:	sexpr0 '-' sexpr1	{ $$ = easm_expr_bin(EASM_EXPR_SUB, $1, $3); $$->loc = @$; }
sexpr0:	sexpr1

sexpr1:	sexpr1 '*' pexpr	{ $$ = easm_expr_bin(EASM_EXPR_MUL, $1, $3); $$->loc = @$; }
sexpr1:	sexpr1 '/' pexpr	{ $$ = easm_expr_bin(EASM_EXPR_DIV, $1, $3); $$->loc = @$; }
sexpr1:	sexpr1 '%' pexpr	{ $$ = easm_expr_bin(EASM_EXPR_MOD, $1, $3); $$->loc = @$; }
sexpr1:	pexpr

pexpr:	T_UMINUS pexpr		{ $$ = easm_expr_un(EASM_EXPR_NEG, $2); $$->loc = @$; }
pexpr:	'~' pexpr		{ $$ = easm_expr_un(EASM_EXPR_NOT, $2); $$->loc = @$; }
pexpr:	'!' pexpr		{ $$ = easm_expr_un(EASM_EXPR_LNOT, $2); $$->loc = @$; }
pexpr:	aexpr

aexpr:	'(' expr ')'		{ $$ = $2; }
aexpr:	T_NUM			{ $$ = easm_expr_num(EASM_EXPR_NUM, $1); $$->loc = @$; }
aexpr:	T_HASHWORD		{ $$ = easm_expr_str(EASM_EXPR_LABEL, $1); $$->loc = @$; }
aexpr:	T_DOLWORD		{ $$ = easm_expr_str(EASM_EXPR_REG, $1); $$->loc = @$; }
aexpr:	'[' membody ']'		{ $$ = $2; $$->loc = @$; }
aexpr:	T_WORDLB membody ']'	{ $$ = $2; $$->str = $1; $$->loc = @$; }
aexpr:	aexpr T_DOTWORD		{ $$ = easm_expr_un(EASM_EXPR_SWIZZLE, $1); ADDARRAY($$->swizzles, ((struct easm_swizzle){$2, 0})); $$->loc = @$; }
aexpr:	aexpr T_DOTNUM		{ $$ = easm_expr_un(EASM_EXPR_SWIZZLE, $1); ADDARRAY($$->swizzles, ((struct easm_swizzle){0, $2})); $$->loc = @$; }
aexpr:	lswizzle ')'		{ $$ = $1; $$->loc = @$; }
aexpr:	'(' sinsn ')'		{ $$ = easm_expr_sinsn($2); $$->loc = @$; }
aexpr:	'#'			{ $$ = easm_expr_simple(EASM_EXPR_DISCARD); $$->loc = @$; }
aexpr:	'(' ')'			{ $$ = easm_expr_simple(EASM_EXPR_ZVEC); $$->loc = @$; }
aexpr:	T_STR			{ $$ = easm_expr_astr($1); $$->loc = @$; }

lswizzle:	aexpr T_DOTLP	{ $$ = easm_expr_un(EASM_EXPR_SWIZZLE, $1); }
lswizzle:	lswizzle T_WORD	{ $$ = $1; ADDARRAY($$->swizzles, ((struct easm_swizzle){$2, 0})); }
lswizzle:	lswizzle T_NUM	{ $$ = $1; ADDARRAY($$->swizzles, ((struct easm_swizzle){0, $2})); }

membody:	mods expr			{ $$ = easm_expr_un(EASM_EXPR_MEM, $2); $$->mods = $1; }
membody:	mods expr T_PLUSPLUS expr	{ $$ = easm_expr_bin(EASM_EXPR_MEMPP, $2, $4); $$->mods = $1; }
membody:	mods expr T_MINUSMINUS expr	{ $$ = easm_expr_bin(EASM_EXPR_MEMMM, $2, $4); $$->mods = $1; }
membody:	mods expr T_PLUSEQ expr		{ $$ = easm_expr_bin(EASM_EXPR_MEMPE, $2, $4); $$->mods = $1; }
membody:	mods expr T_MINUSEQ expr	{ $$ = easm_expr_bin(EASM_EXPR_MEMME, $2, $4); $$->mods = $1; }

%%

int easm_read_file(FILE *file, const char *filename, struct easm_file **res) {
	yyscan_t lex_state;
	struct yy_lex_intern lex_extra;
	lex_extra.line = 1;
	lex_extra.pos = 1;
	lex_extra.ws = 0;
	lex_extra.file = filename;
	lex_extra.nest = 0;
	easm_lex_init_extra(lex_extra, &lex_state);
	easm_set_in(file, lex_state);
	int ret = easm_parse(lex_state, res);
	easm_lex_destroy(lex_state);
	return ret;
}
