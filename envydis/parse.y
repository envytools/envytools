%{
#include "envyas.h"
int yylex (void);
void yyerror (char const *err) {
	fprintf (stderr, "%s\n", err);
}
%}

%union {
	ull num;
	char *str;
	struct expr *expr;
	struct line *line;
	struct file *file;
}

%token <str> T_ID
%token <str> T_LABEL
%token <str> T_DIR
%token <str> T_MEM
%token <str> T_REG
%token <num> T_NUM
%token T_PLUSPLUS
%token T_UMINUS
%token T_MINUSMINUS
%token T_SHR
%token T_SHL
%token T_ERR
%type <file> file
%type <line> line
%type <line> insn
%type <expr> expr
%type <expr> expr0
%type <expr> expr1
%type <expr> expr2
%type <expr> expr3
%type <expr> expr4
%type <expr> expr5
%type <expr> aexpr
%type <expr> vbody

%%

start:	file { return envyas_process($1); }
;

file:	file line { $$ = $1; if ($2) ADDARRAY($$->lines, $2); }
|	/**/ { $$ = calloc(sizeof *$$, 1); }
;

line:	T_ID ':' { $$ = calloc(sizeof *$$, 1); $$->type = LINE_LABEL; $$->str = $1; }
|	insn ';'
|	T_DIR insn ';' { $$ = $2; $$->str = $1; $$->type = LINE_DIR; }
|	';' { $$ = 0; }
;

insn:	insn expr { $$ = $1; ADDARRAY($$->atoms, $2); }
|	insn T_ID { $$ = $1; struct expr *e = makeex(EXPR_ID); e->str = $2; ADDARRAY($$->atoms, e); }
|	expr { $$ = calloc(sizeof *$$, 1); $$->type = LINE_INSN; ADDARRAY($$->atoms, $1); }
|	T_ID { $$ = calloc(sizeof *$$, 1); $$->type = LINE_INSN; struct expr *e = makeex(EXPR_ID); e->str = $1; ADDARRAY($$->atoms, e); }
;

expr:	expr T_PLUSPLUS expr0 { $$ = makebinex(EXPR_PIADD, $1, $3); }
|	expr T_MINUSMINUS expr0 { $$ = makebinex(EXPR_PISUB, $1, $3); }
|	expr0
;

expr0:	expr0 '|' expr1 { $$ = makebinex(EXPR_OR, $1, $3); }
|	expr1
;

expr1:	expr1 '^' expr2 { $$ = makebinex(EXPR_XOR, $1, $3); }
|	expr2
;

expr2:	expr2 '&' expr3 { $$ = makebinex(EXPR_AND, $1, $3); }
|	expr3
;

expr3:	expr3 T_SHL expr4 { $$ = makebinex(EXPR_SHL, $1, $3); }
|	expr3 T_SHR expr4 { $$ = makebinex(EXPR_SHR, $1, $3); }
|	expr4
;

expr4:	expr4 '+' expr5 { $$ = makebinex(EXPR_ADD, $1, $3); }
|	expr4 '-' expr5 { $$ = makebinex(EXPR_SUB, $1, $3); }
|	expr5
;

expr5:	expr5 '*' aexpr { $$ = makebinex(EXPR_MUL, $1, $3); }
|	expr5 '/' aexpr { $$ = makebinex(EXPR_DIV, $1, $3); }
|	aexpr
;

aexpr:	T_MEM expr ']' { $$ = makeex(EXPR_MEM); $$->str = $1; $$->expr1 = $2; }
|	'{' vbody '}' { $$ = $2; }
|	'(' expr ')' { $$ = $2; }
|	T_REG { $$ = makeex(EXPR_REG); $$->str = $1; }
|	T_NUM { $$ = makeex(EXPR_NUM); $$->num1 = $1; $$->isimm = 1; }
|	T_LABEL { $$ = makeex(EXPR_LABEL); $$->str = $1; $$->isimm = 1; }
|	T_UMINUS aexpr { $$ = makeunex(EXPR_NEG, $2); }
|	'~' aexpr { $$ = makeunex(EXPR_NOT, $2); }
|	'#' { $$ = makeex(EXPR_DISCARD); }
|	T_NUM ':' T_NUM { $$ = makeex(EXPR_BITFIELD); $$->num1 = $1; $$->num2 = $3; }
;

vbody:	vbody expr { $$ = $1; ADDARRAY($$->vexprs, $2); }
|	/**/ { $$ = makeex(EXPR_VEC); }
;

%%
