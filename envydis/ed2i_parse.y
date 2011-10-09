%{
#include "ed2_misc.h"
#include "ed2_parse.h"
#include "ed2i.h"
#include "ed2i_pre.h"
#include "ed2i_parse.h"
#include "ed2i_lex.h"
#include <stdio.h>
void ed2i_error (YYLTYPE *loc, yyscan_t lex_state, struct ed2i_isa **isa, char const *err) {
	fprintf (stderr, ED2_LOC_FORMAT(*loc, "%s\n"), err);
}

%}

%locations
%define api.pure
%name-prefix "ed2i_"
%lex-param { yyscan_t lex_state }
%parse-param { yyscan_t lex_state }
%parse-param { struct ed2i_isa **isa }

%union {
	uint64_t num;
	char *str;
	struct ed2_astr astr;
	struct {
		char **strs;
		int strsnum;
		int strsmax;
	} strs;
	struct ed2ip_feature *feature;
	struct ed2ip_variant *variant;
	struct ed2ip_isa *isa;
}

/* XXX: destructors */

%token <str> T_WORD
%token <str> T_WORDC
%token <str> T_HASHWORD
%token <str> T_WORDLB
%token <str> T_REG
%token <astr> T_STR
%token <num> T_NUM
%token T_ERR
%token T_UMINUS
%token T_PLUSPLUS
%token T_MINUSMINUS
%token T_PLUSEQ
%token T_MINUSEQ
%token T_FEATURE "FEATURE"
%token T_CONFLICT "CONFLICT"
%token T_IMPLIES "IMPLIES"
%token T_VARIANT "VARIANT"

%type <str> strnn
%type <str> description
%type <strs> wordsnz
%type <feature> feature
%type <feature> fmods
%type <variant> variant
%type <variant> vmods
%type <isa> file

%destructor { free($$); } <str>
%destructor { free($$.str); } <astr>
%destructor { ed2_free_strings($$.strs, $$.strsnum); } <strs>
%destructor { ed2ip_del_feature($$); } <feature>
%destructor { ed2ip_del_variant($$); } <variant>
%destructor { ed2ip_del_isa($$); } <isa>

%%

start:	file		{ *isa = ed2ip_transform($1); }

file:	/**/		{ $$ = calloc(sizeof *$$, 1); }
|	file feature	{ $$ = $1; ADDARRAY($$->features, $2); }
|	file variant	{ $$ = $1; ADDARRAY($$->variants, $2); }
|	file error ';'	{ $$ = $1; $$->broken = 1; }
;

feature:	"FEATURE" wordsnz description fmods ';'	{ $$ = $4; $$->loc = @$; $$->description = $3; $$->names = $2.strs; $$->namesnum = $2.strsnum; $$->namesmax = $2.strsmax; }
;

fmods:		/**/	{ $$ = calloc(sizeof *$$, 1); }
|		fmods "CONFLICT" wordsnz { int i; $$ = $1; for (i = 0; i < $3.strsnum; i++) ADDARRAY($$->conflicts, $3.strs[i]); free($3.strs); }
|		fmods "IMPLIES" wordsnz { int i; $$ = $1; for (i = 0; i < $3.strsnum; i++) ADDARRAY($$->implies, $3.strs[i]); free($3.strs); }
;

wordsnz:	T_WORD { memset(&$$, 0, sizeof($$)); ADDARRAY($$.strs, $1); }
|		wordsnz T_WORD { $$ = $1; ADDARRAY($$.strs, $2); }
;

description:	/**/	{ $$ = 0; }
|		strnn
;

variant:	"VARIANT" wordsnz description vmods ';'	{ $$ = $4; $$->loc = @$; $$->description = $3; $$->names = $2.strs; $$->namesnum = $2.strsnum; $$->namesmax = $2.strsmax; }
;

vmods:		/**/	{ $$ = calloc(sizeof *$$, 1); }
|		vmods "FEATURE" wordsnz { int i; $$ = $1; for (i = 0; i < $3.strsnum; i++) ADDARRAY($$->features, $3.strs[i]); free($3.strs); }
;

strnn:		T_STR	{ $$ = $1.str; if (strlen($$) != $1.len) YYERROR; }
;

%%

struct ed2i_isa *ed2i_read_isa (const char *isaname) {
	FILE *file = ed2_find_file(isaname, getenv("ED2_PATH"), 0); 
	if (!file)
		file = ed2_find_file(isaname, ".:isadb:../isadb:envytools/isadb", 0);
	if (!file) {
		fprintf (stderr, "Cannot find ISA definition file for ISA %s.\n", isaname);
		fprintf (stderr, "Please set ED2_PATH to point to the isadb directory from envytools.\n");
		return 0;
	}
	yyscan_t lex_state;
	struct ed2_lex_intern lex_extra;
	struct ed2i_isa *isa;
	lex_extra.line = 1;
	lex_extra.pos = 1;
	lex_extra.ws = 0;
	lex_extra.file = isaname;
	lex_extra.nest = 0;
	ed2i_lex_init_extra(lex_extra, &lex_state);
	ed2i_set_in(file, lex_state);
	int res = ed2i_parse(lex_state, &isa);
	ed2i_lex_destroy(lex_state);
	fclose(file);
	if (res)
		return 0;
	return isa;
}
