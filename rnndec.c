#include "rnndec.h"
#include <stdio.h>

struct rnnvarcontext *rnndec_newvc() {
	struct rnnvarcontext *res = calloc (sizeof *res, 1);
	return res;
}

int rnndec_vcadd(struct rnndb *db, struct rnnvarcontext *cv, char *varset, char *variant) {
	struct rnnenum *en = rnn_findenum(db, varset);
	if (!en) {
		fprintf (stderr, "Enum %s doesn't exist in database!\n", varset);
		return 0;
	}
	int i;
	for (i = 0; i < en->valsnum; i++)
		if (!strcmp(en->vals[i]->name, variant)) {
			struct rnnvarcitem *ci = calloc (sizeof *ci, 1);
			ci->en = en;
			ci->variant = i;
			RNN_ADDARRAY(cv->citems, ci);
			return 1;
		}
	fprintf (stderr, "Variant %s doesn't exist in enum %s!\n", variant, varset);
	return 0;
}

int rnndec_vcmatch(struct rnnvarinfo *vi, struct rnnvarcontext *cv) {
	if (vi->dead)
		return 0;
	int i;
	for (i = 0; i < vi->varsetsnum; i++) {
		int j;
		for (j = 0; j < cv->citemsnum; j++)
			if (vi->varsets[i]->venum == cv->citems[j]->en)
				break;
		if (j == cv->citemsnum) {
			fprintf (stderr, "I don't know which %s variant to use!\n", vi->varsets[i]->venum->name);
		} else {
			if (!vi->varsets[i]->variants[cv->citems[j]->variant])
				return 0;
		}
	}
	return 1;
}
