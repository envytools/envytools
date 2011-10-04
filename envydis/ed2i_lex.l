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

%%

"/*"([^*]|"*"+[^/])*"*"+"/"	{ }
[ \t]				{ yyextra.ws = 1; }
\n				{ yyextra.ws = 1; }
"//".*\n			{ yyextra.ws = 1; }
"FEATURE"			{ yyextra.ws = 0; return T_FEATURE; }
"CONFLICT"			{ yyextra.ws = 0; return T_CONFLICT; }
"IMPLIES"			{ yyextra.ws = 0; return T_IMPLIES; }
"VARIANT"			{ yyextra.ws = 0; return T_VARIANT; }
[a-z_][a-zA-Z_0-9]*		{ yyextra.ws = 0; yylval->str = strdup(yytext); return T_WORD; }
\$[a-zA-Z_0-9]*			{ yyextra.ws = 0; yylval->str = strdup(yytext+1); return T_REG; }
\#[a-zA-Z_0-9]+			{ yyextra.ws = 0; yylval->str = strdup(yytext+1); return T_HASHWORD; }
0[0-7]*|[1-9][0-9]*|0[xX][0-9a-fA-F]+		{ yyextra.ws = 0; yylval->num = strtoull(yytext, 0, 0); return T_NUM; }
"++"				{ yyextra.ws = 0; return T_PLUSPLUS; }
"+="				{ yyextra.ws = 0; return T_PLUSEQ; }
"-"/[ \t]			{ if (yyextra.ws) { yyextra.ws = 0; return '-'; } else { yyextra.ws = 0; return T_ERR; } }
"-"/[^-= \t]			{ if (yyextra.ws) { yyextra.ws = 0; return T_UMINUS; } else { yyextra.ws = 0; return '-'; } }
"--"				{ yyextra.ws = 0; return T_MINUSMINUS; }
"-="				{ yyextra.ws = 0; return T_MINUSEQ; }
[({]				{ yyextra.ws = 1; return yytext[0]; }
[a-zA-Z_0-9]*:			{ yyextra.ws = 1; yytext[strlen(yytext)-1] = 0; yylval->str = strdup(yytext); return T_WORDC; }
[a-zA-Z_0-9]*"["		{ yyextra.ws = 1; yytext[strlen(yytext)-1] = 0; yylval->str = strdup(yytext); return T_WORDLB; }
[*+&|)}\]:;#.]			{ yyextra.ws = 0; return yytext[0]; }
\"([^\\"]|\\[\\"'nrtafv]|\\x[0-9a-fA-F][0-9a-fA-F])*\"	{ yyextra.ws = 0; yylval->astr.str = ed2_str_deescape(yytext, &yylval->astr.len); return T_STR; }
.				{ return T_ERR; }
