#include "rnn.h"
#include "rnndec.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

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
		struct rnndecaddrinfo *info = rnndec_decodeaddr(vc, dom, a, 0);
		if (info)
			printf ("%s\n", info->name);
		if (info && info->typeinfo)
			printf ("%s\n", rnndec_decodeval(vc, info->typeinfo, b, info->width));
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
