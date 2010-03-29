#ifndef RNNDEC_H
#define RNNDEC_H

#include "rnn.h"

struct rnndecvariant {
	struct rnnenum *en;
	int variant;
};

struct rnndeccontext {
	struct rnndb *db;
	struct rnndecvariant **vars;
	int varsnum;
	int varsmax;
};

struct rnndeccontext *rnndec_newcontext(struct rnndb *db);
int rnndec_varadd(struct rnndeccontext *ctx, char *varset, char *variant);
int rnndec_varmatch(struct rnndeccontext *ctx, struct rnnvarinfo *vi);

#endif
