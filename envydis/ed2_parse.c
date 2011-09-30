#include "ed2_parse.h"

void ed2_lex_common(struct ed2_lex_intern *x, YYLTYPE *loc, const char *str) {
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

