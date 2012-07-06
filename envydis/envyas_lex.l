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

%{
#include "envyas.h"
#include "yy.h"
#include "envyas_parse.h"

%}

%option bison-bridge
%option bison-locations
%option reentrant
%option noyywrap
%option prefix="envyas_"
%option header-file="envyas_lex.h"
%option extra-type="struct yy_lex_intern"
%option noinput
%option nounput

%%

"/*"([^*]|"*"+[^/])*"*"+"/"	{ }
[ \t]				{ yyextra.ws = 1; }
\n				{ yyextra.ws = 1; return ';'; }
"//".*\n			{ yyextra.ws = 1; return ';'; }
[a-zA-Z_][a-zA-Z_0-9]*		{ yyextra.ws = 0; yylval->str = strdup(yytext); return T_ID; }
#[a-zA-Z_][a-zA-Z_0-9]*		{ yyextra.ws = 0; yylval->str = strdup(yytext+1); return T_LABEL; }
"."[a-zA-Z_][a-zA-Z_0-9]*	{ yyextra.ws = 0; yylval->str = strdup(yytext); return T_DIR; }
\$[a-zA-Z_0-9]*			{ yyextra.ws = 0; yylval->str = strdup(yytext+1); return T_REG; }
0[0-7]*|[1-9][0-9]*|0[xX][0-9a-fA-F]+		{ yyextra.ws = 0; yylval->num = strtoull(yytext, 0, 0); return T_NUM; }
"~"				{ yyextra.ws = 0; return '~'; }
"*"				{ yyextra.ws = 0; return '*'; }
"/"				{ yyextra.ws = 0; return '/'; }
"+"				{ yyextra.ws = 0; return '+'; }
"++"				{ yyextra.ws = 0; return T_PLUSPLUS; }
"-"/[ \t]			{ if (yyextra.ws) { yyextra.ws = 0; return '-'; } else { yyextra.ws = 0; return T_ERR; } }
"-"/[^- \t]			{ if (yyextra.ws) { yyextra.ws = 0; return T_UMINUS; } else { yyextra.ws = 0; return '-'; } }
"--"				{ yyextra.ws = 0; return T_MINUSMINUS; }
">>"				{ yyextra.ws = 0; return T_SHR; }
"<<"				{ yyextra.ws = 0; return T_SHL; }
"&"				{ yyextra.ws = 0; return '&'; }
"|"				{ yyextra.ws = 0; return '|'; }
"^"				{ yyextra.ws = 0; return '^'; }
"("				{ yyextra.ws = 1; return '('; }
")"				{ yyextra.ws = 0; return ')'; }
"{"				{ yyextra.ws = 1; return '{'; }
"}"				{ yyextra.ws = 0; return '}'; }
[a-zA-Z_0-9]*"["		{ yyextra.ws = 1; yytext[strlen(yytext)-1] = 0; yylval->str = strdup(yytext); return T_MEM; }
"]"				{ yyextra.ws = 0; return ']'; }
":"				{ yyextra.ws = 0; return ':'; }
";"				{ yyextra.ws = 0; return ';'; }
"#"				{ yyextra.ws = 0; return '#'; }
.				{ return T_ERR; }
