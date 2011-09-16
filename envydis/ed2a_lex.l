%top{
	struct ed2a_lex_intern {
		int line;
		int pos;
		int ws;
		const char *file;
	};
}

%{
#include "ed2_misc.h"
#include "ed2a.h"
#define YYLTYPE struct ed2_loc
#include "ed2a_parse.h"

static void common(struct ed2a_lex_intern *x, YYLTYPE *loc, const char *str) {
	int i;
	loc->lstart = x->line;
	loc->cstart = x->pos;
	for (i = 0; str[i]; i++) {
		if (str[i] == '\n') {
			x->pos = 1;
			x->line++;
		} else {
			x->pos++;
		}
	}
	loc->lend = x->line;
	loc->cend = x->pos;
	loc->file = x->file;
}

#define YY_USER_ACTION common(&yyextra, yylloc, yytext);
#define YY_USER_INIT yyextra.line = 1; yyextra.pos = 1; yyextra.ws = 0; yylloc->lend = 1; yylloc->cend = 1; yylloc->lstart = 1; yylloc->cstart = 1; yylloc->file = yyextra.file;

%}

%option bison-bridge
%option bison-locations
%option reentrant
%option noyywrap
%option prefix="ed2a_"
%option header-file="ed2a_lex.h"
%option extra-type="struct ed2a_lex_intern"

%%

"/*"([^*]|"*"+[^/])*"*"+"/"	{ }
[ \t]				{ yyextra.ws = 1; }
\n				{ yyextra.ws = 1; return '\n'; }
"//".*\n			{ yyextra.ws = 1; return '\n'; }
[a-zA-Z_][a-zA-Z_0-9]*		{ yyextra.ws = 0; yylval->str = strdup(yytext); return T_WORD; }
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
