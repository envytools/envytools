#include "rnn.h"
#include "rnndec.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <getopt.h>

void usage()
{
	printf ("Usage:\n"
			"\tlookup [-f file.xml] [-a NVXX] -e enum-name [-- -v attribute value] value\n"
			"\tlookup [-f file.xml] [-a NVXX] -b bitset-name [-- -v attribute value] value\n"
			"\tlookup [-f file.xml] [-a NVXX] [-d domain-name] [-- -v attribute value] address\n"
			"\tlookup [-f file.xml] [-a NVXX] [-d domain-name] [-- -v attribute value] address value\n"
		);
	exit(2);
}

int main(int argc, char **argv) {
	char *file = "nv_mmio.xml";
	char *name = "NV_MMIO";
	char *variant = NULL;
	char c, mode = 'd';
	uint64_t reg, colors=1, val = 0;
	struct rnndeccontext *vc;

	rnn_init();
	if (argc < 2) {
		usage();
	}
	struct rnndb *db = rnn_newdb();

	/* Arguments parsing */
	while ((c = getopt (argc, argv, "f:a:d:e:b:c")) != -1) {
		switch (c) {
			case 'f':
				file = strdup(optarg);
				break;
			case 'e':
				mode = 'e';
				name = strdup(optarg);
				break;
			case 'b':
				mode = 'b';
				name = strdup(optarg);
				break;
			case 'd':
				mode = 'd';
				name = strdup(optarg);
				break;
			case  'a':
				variant = strdup(optarg);
				break;
			case 'c':
				colors = 0;
				break;
		}
	}

	rnn_parsefile (db, file);
	rnn_prepdb (db);
	vc = rnndec_newcontext(db);
	if(colors)
		vc->colors = &rnndec_colorsterm;

	if (variant)
		rnndec_varadd(vc, "chipset", variant);

	/* Parse extra arguments */
	while (!strcmp (argv[optind], "-v")) {
		rnndec_varadd(vc, argv[optind+1], argv[optind+2]);
		optind+=3;
	}

	if (optind >= argc) {
		fprintf (stderr, "No address specified.\n");
		return 1;
	}

	reg = strtoull(argv[optind], 0, 16);
	if (optind + 1 < argc)
		val = strtoull(argv[optind + 1], 0, 16);

	if (mode == 'e') {
		struct rnnenum *en = rnn_findenum (db, name);
		if (en) {
			int i;
			int dec = 0;
			for (i = 0; i < en->valsnum; i++)
				if (en->vals[i]->valvalid && en->vals[i]->value == reg) {
					printf ("%s\n", en->vals[i]->name);
					dec = 1;
					break;
				}
			if (!dec)
				printf ("%#"PRIx64"\n", reg);

			return 0;
		} else {
			fprintf(stderr, "Not an enum: '%s'\n", name);
			return 1;
		}
	} else if (mode == 'b') {
		struct rnnbitset *bs = rnn_findbitset (db, name);

		if (bs) {
			printf("TODO\n");
			return 0;
		} else {
			fprintf(stderr, "Not a bitset: '%s'\n", name);
			return 1;
		}

	} else if (mode == 'd') {
		struct rnndomain *dom = rnn_finddomain (db, name);

		if (dom) {
			struct rnndecaddrinfo *info = rnndec_decodeaddr(vc, dom, reg, 0);
			if (info && info->typeinfo)
				printf ("%s => %s\n", info->name, rnndec_decodeval(vc, info->typeinfo, val, info->width));
			else if (info)
				printf ("%s\n", info->name);
			else
				return 1;
			return 0;
		} else {
			fprintf(stderr, "Not a domain: '%s'\n", name);
			return 1;
		}
	} else {
		return 1;
	}
}
