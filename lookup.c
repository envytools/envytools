#include "rnn.h"
#include "rnndec.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

void trymatch (uint64_t a, uint64_t b, struct rnndelem *elem, int width, struct rnndeccontext *vc) {
	if (!rnndec_varmatch(vc, &elem->varinfo))
		return;
	int i;
	switch (elem->type) {
		case RNN_ETYPE_REG:
			if (a >= elem->offset && (a < elem->offset + elem->stride * elem->length || elem->length == 1)) {
				uint32_t res;
				if (elem->stride) {
					i = (a-elem->offset)/elem->stride;
					res = (a-elem->offset)%elem->stride;
				} else {
					i = 0;
					res = a-elem->offset;
				}
				if (res < elem->width/width) {
					printf ("%s[%d]+%x %s\n", elem->fullname, i, res, rnndec_decodeval(vc, &elem->typeinfo, b, elem->width));
				}
			}
			break;
		case RNN_ETYPE_STRIPE:
			for (i = 0; (i < elem->length || !elem->length) && a >= elem->offset + elem->stride * i; i++) {
				int j;
				for (j = 0; j < elem->subelemsnum; j++)
					trymatch(a-(elem->offset + elem->stride * i), b, elem->subelems[j], width, vc);
			}
			break;
		case RNN_ETYPE_ARRAY:
			if (a >= elem->offset && a < elem->offset + elem->stride * elem->length) {
				i = (a-elem->offset)/elem->stride;
				uint32_t res = (a-elem->offset)%elem->stride;
				printf ("%s[%d]+%x\n", elem->fullname, i, res);
				int j;
				for (j = 0; j < elem->subelemsnum; j++)
					trymatch(res, b, elem->subelems[j], width, vc);
			}
			break;
		default:
			break;
	}
}

int main(int argc, char **argv) {
	rnn_init();
	if (argc < 4) {
		printf ("Usage:\n"
				"\tlookup file.xml enum-name value\n"
				"\tlookup file.xml bitset-name value\n"
				"\tlookup file.xml domain-name address\n"
				"\tlookup file.xml domain-name address value\n"
			);
		return 0;
	}
	struct rnndb *db = rnn_newdb();
	rnn_parsefile (db, argv[1]);
	rnn_prepdb (db);
	struct rnndeccontext *vc = rnndec_newcontext(db);
	vc->colors = &rnndec_colorsterm;
	while (!strcmp (argv[2], "-v")) {
		rnndec_varadd(vc, argv[3], argv[4]);
		argv += 3;
		argc -= 3;
	}
	struct rnndomain *dom = rnn_finddomain (db, argv[2]);
	struct rnnbitset *bs = rnn_findbitset (db, argv[2]);
	struct rnnenum *en = rnn_findenum (db, argv[2]);
	uint64_t a, b;
	a = strtoull(argv[3], 0, 0);
	if (argc > 4)
		b = strtoull(argv[4], 0, 0);
	if (dom) {
		int i;
		for (i = 0; i < dom->subelemsnum; i++)
			trymatch (a, b, dom->subelems[i], dom->width, vc);
	} else if (bs) {
	} else if (en) {
		int i;
		int dec = 0;
		for (i = 0; i < en->valsnum; i++)
			if (en->vals[i]->valvalid && en->vals[i]->value == a) {
				printf ("%s\n", en->vals[i]->name);
				dec = 1;
				break;
			}
		if (!dec)
			printf ("%#"PRIx64"\n", a);
	} else {
		printf ("Not an enum, bitset, or domain: %s\n", argv[2]);
		return 0;
	}
	return 0;
}
