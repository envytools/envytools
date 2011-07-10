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
