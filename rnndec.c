#include "rnndec.h"
#include <stdio.h>
#include <string.h>

struct rnndeccontext *rnndec_newcontext(struct rnndb *db) {
	struct rnndeccontext *res = calloc (sizeof *res, 1);
	res->db = db;
	return res;
}

int rnndec_varadd(struct rnndeccontext *ctx, char *varset, char *variant) {
	struct rnnenum *en = rnn_findenum(ctx->db, varset);
	if (!en) {
		fprintf (stderr, "Enum %s doesn't exist in database!\n", varset);
		return 0;
	}
	int i;
	for (i = 0; i < en->valsnum; i++)
		if (!strcmp(en->vals[i]->name, variant)) {
			struct rnndecvariant *ci = calloc (sizeof *ci, 1);
			ci->en = en;
			ci->variant = i;
			RNN_ADDARRAY(ctx->vars, ci);
			return 1;
		}
	fprintf (stderr, "Variant %s doesn't exist in enum %s!\n", variant, varset);
	return 0;
}

int rnndec_varmatch(struct rnndeccontext *ctx, struct rnnvarinfo *vi) {
	if (vi->dead)
		return 0;
	int i;
	for (i = 0; i < vi->varsetsnum; i++) {
		int j;
		for (j = 0; j < ctx->varsnum; j++)
			if (vi->varsets[i]->venum == ctx->vars[j]->en)
				break;
		if (j == ctx->varsnum) {
			fprintf (stderr, "I don't know which %s variant to use!\n", vi->varsets[i]->venum->name);
		} else {
			if (!vi->varsets[i]->variants[ctx->vars[j]->variant])
				return 0;
		}
	}
	return 1;
}
