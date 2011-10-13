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

%{
#include "ed2_misc.h"
#include "ed2_parse.h"
#include "ed2i.h"
#include "ed2i_pre.h"
#include "ed2i_parse.h"

%}

%option bison-bridge
%option bison-locations
%option reentrant
%option noyywrap
%option prefix="ed2i_"
%option header-file="ed2i_lex.h"
%option extra-type="struct ed2_lex_intern"

%s normal
%x ncomment

%%

"/*"([^*]|"*"+[^/])*"*"+"/"	{ }
"/+"				{ yyextra.nest++; BEGIN ncomment; }
[ \t]				{ yyextra.ws = 1; }
\n				{ yyextra.ws = 1; }
"//".*\n			{ yyextra.ws = 1; }
"FEATURE"			{ yyextra.ws = 0; return T_FEATURE; }
"CONFLICT"			{ yyextra.ws = 0; return T_CONFLICT; }
"IMPLIES"			{ yyextra.ws = 0; return T_IMPLIES; }
"VARIANT"			{ yyextra.ws = 0; return T_VARIANT; }
"MODE"				{ yyextra.ws = 0; return T_MODE; }
"DEFAULT"			{ yyextra.ws = 0; return T_DEFAULT; }
[a-z_][a-zA-Z_0-9]*		{ yyextra.ws = 0; yylval->str = strdup(yytext); return T_WORD; }
\$[a-zA-Z_0-9]*			{ yyextra.ws = 0; yylval->str = strdup(yytext+1); return T_REG; }
0[0-7]*|[1-9][0-9]*|0[xX][0-9a-fA-F]+		{ yyextra.ws = 0; yylval->num = strtoull(yytext, 0, 0); return T_NUM; }
"++"				{ yyextra.ws = 0; return T_PLUSPLUS; }
"+="				{ yyextra.ws = 0; return T_PLUSEQ; }
"-"/[ \t]			{ if (yyextra.ws) { yyextra.ws = 0; return '-'; } else { yyextra.ws = 0; return T_ERR; } }
"-"/[^-= \t]			{ if (yyextra.ws) { yyextra.ws = 0; return T_UMINUS; } else { yyextra.ws = 0; return '-'; } }
"--"				{ yyextra.ws = 0; return T_MINUSMINUS; }
"-="				{ yyextra.ws = 0; return T_MINUSEQ; }
[({]				{ yyextra.ws = 1; return yytext[0]; }
[*+&|)}\]:;#.]			{ yyextra.ws = 0; return yytext[0]; }
\"([^\\"]|\\[\\"'nrtafv]|\\x[0-9a-fA-F][0-9a-fA-F])*\"	{ yyextra.ws = 0; yylval->astr.str = ed2_str_deescape(yytext, &yylval->astr.len); return T_STR; }
.				{ return T_ERR; }
<ncomment>"+/"			{ yyextra.nest--; if (!yyextra.nest) BEGIN normal; }
<ncomment>"/+"			{ yyextra.nest++; }
<ncomment>.			{ }
<ncomment>\n			{ }
