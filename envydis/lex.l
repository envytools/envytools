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

%option nounput
%option noinput

%{
#include "envyas.h"
#include "parse.h"
int ws;

static int yywrap(void) {
	return 1;
}

%}

%%

"/*"([^*]|"*"+[^/])*"*"+"/"	{ }
[ \t]				{ ws = 1; }
\n				{ ws = 1; return ';'; }
"//".*\n			{ ws = 1; return ';'; }
[a-zA-Z_][a-zA-Z_0-9]*		{ ws = 0; yylval.str = strdup(yytext); return T_ID; }
#[a-zA-Z_][a-zA-Z_0-9]*		{ ws = 0; yylval.str = strdup(yytext+1); return T_LABEL; }
"."[a-zA-Z_][a-zA-Z_0-9]*	{ ws = 0; yylval.str = strdup(yytext); return T_DIR; }
\$[a-zA-Z_0-9]*			{ ws = 0; yylval.str = strdup(yytext+1); return T_REG; }
0[0-7]*|[1-9][0-9]*|0[xX][0-9a-fA-F]+		{ ws = 0; yylval.num = strtoull(yytext, 0, 0); return T_NUM; }
"~"				{ ws = 0; return '~'; }
"*"				{ ws = 0; return '*'; }
"/"				{ ws = 0; return '/'; }
"+"				{ ws = 0; return '+'; }
"++"				{ ws = 0; return T_PLUSPLUS; }
"-"/[ \t]			{ if (ws) { ws = 0; return '-'; } else { ws = 0; return T_ERR; } }
"-"/[^- \t]			{ if (ws) { ws = 0; return T_UMINUS; } else { ws = 0; return '-'; } }
"--"				{ ws = 0; return T_MINUSMINUS; }
">>"				{ ws = 0; return T_SHR; }
"<<"				{ ws = 0; return T_SHL; }
"&"				{ ws = 0; return '&'; }
"|"				{ ws = 0; return '|'; }
"^"				{ ws = 0; return '^'; }
"("				{ ws = 1; return '('; }
")"				{ ws = 0; return ')'; }
"{"				{ ws = 1; return '{'; }
"}"				{ ws = 0; return '}'; }
[a-zA-Z_0-9]*"["		{ ws = 1; yytext[strlen(yytext)-1] = 0; yylval.str = strdup(yytext); return T_MEM; }
"]"				{ ws = 0; return ']'; }
":"				{ ws = 0; return ':'; }
";"				{ ws = 0; return ';'; }
"#"				{ ws = 0; return '#'; }
.				{ return T_ERR; }
