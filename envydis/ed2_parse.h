#ifndef ED2_PARSE_H
#define ED2_PARSE_H

#include "ed2_misc.h"

struct ed2_lex_intern {
	int line;
	int pos;
	const char *file;
	int ws;
	int nest;
};

#define YYLTYPE struct ed2_loc

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

#define YY_USER_ACTION ed2_lex_common(&yyextra, yylloc, yytext);
#define YY_USER_INIT yyextra.line = 1; yyextra.pos = 1; yyextra.ws = 0; yylloc->lend = 1; yylloc->cend = 1; yylloc->lstart = 1; yylloc->cstart = 1; yylloc->file = yyextra.file; yyextra.nest = 0;

void ed2_lex_common(struct ed2_lex_intern *x, YYLTYPE *loc, const char *str);

#endif
