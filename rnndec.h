#ifndef RNNDEC_H
#define RNNDEC_H

#include "rnn.h"

struct rnnvarcitem {
	struct rnnenum *en;
	int variant;
};

struct rnnvarcontext {
	struct rnnvarcitem **citems;
	int citemsnum;
	int citemsmax;
};

struct rnnvarcontext *rnndec_newvc();
int rnndec_vcadd(struct rnndb *db, struct rnnvarcontext *vc, char *varset, char *variant);
int rnndec_vcmatch(struct rnnvarinfo *vi, struct rnnvarcontext *cv);

#endif
