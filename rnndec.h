#ifndef RNNDEC_H
#define RNNDEC_H

#include "rnn.h"

struct rnndecvariant {
	struct rnnenum *en;
	int variant;
};

struct rnndeccolors {
	char *ceval;
	char *cimm;
	char *cname;
	char *cbool;
	char *cerr;
	char *cend;
};

struct rnndeccontext {
	struct rnndb *db;
	struct rnndecvariant **vars;
	int varsnum;
	int varsmax;
	struct rnndeccolors *colors;
};

struct rnndecaddrinfo {
	struct rnntypeinfo *typeinfo;
	int width;
	char *name;
};

extern struct rnndeccolors rnndec_colorsterm, rnndec_colorsnull;

struct rnndeccontext *rnndec_newcontext(struct rnndb *db);
int rnndec_varadd(struct rnndeccontext *ctx, char *varset, char *variant);
int rnndec_varmatch(struct rnndeccontext *ctx, struct rnnvarinfo *vi);
char *rnndec_decodeval(struct rnndeccontext *ctx, struct rnntypeinfo *ti, uint64_t value, int width);
struct rnndecaddrinfo *rnndec_decodeaddr(struct rnndeccontext *ctx, struct rnndomain *domain, uint64_t addr, int write);

#endif
