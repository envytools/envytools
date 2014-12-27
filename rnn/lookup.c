/*
 * Copyright (C) 2010 Marcin Kościelnicki <koriakin@0x04.net>
 * Copyright (C) 2011 Martin Peres <martin.peres@ensi-bourges.fr>
 * Copyright (C) 2011 Witold Waligóra <witold.waligora@gmail.com>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#define _GNU_SOURCE // for asprintf
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
	char *file = "root.xml";
	char *name = "NV_MMIO";
	char *variant = NULL;
	int c, mode = 'd', chip = 0;
	uint64_t reg, colors=1, val = 0;
	struct rnndeccontext *vc;
	int ret;

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
			case 'a':
				chip = strtoull(optarg, NULL, 16);
				if (!chip)
					variant = strdup(optarg);
				else {
					free(variant);
					variant = NULL;
				}
				break;
			case 'c':
				colors = 0;
				break;
			default: usage();
		}
	}

	rnn_parsefile (db, file);
	rnn_prepdb (db);
	vc = rnndec_newcontext(db);
	if(colors)
		vc->colors = &envy_def_colors;

	if (variant)
		rnndec_varadd(vc, "chipset", variant);
	else if (chip)
		rnndec_varaddvalue(vc, "chipset",  chip);

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

			ret = 0;
		} else {
			fprintf(stderr, "Not an enum: '%s'\n", name);
			ret = 1;
		}
	} else if (mode == 'b') {
		struct rnnbitset *bs = rnn_findbitset (db, name);

		if (bs) {
			printf("TODO\n");
			ret = 0;
		} else {
			fprintf(stderr, "Not a bitset: '%s'\n", name);
			ret = 1;
		}

	} else if (mode == 'd') {
		struct rnndomain *dom = rnn_finddomain (db, name);

		if (dom) {
			struct rnndecaddrinfo *info = rnndec_decodeaddr(vc, dom, reg, 0);
			if (info && info->typeinfo) {
				char *res = rnndec_decodeval(vc, info->typeinfo, val, info->width);
				printf ("%s => %s\n", info->name, res);
				free(res);
				rnndec_free_decaddrinfo(info);
				ret = 0;
			} else if (info) {
				printf ("%s\n", info->name);
				rnndec_free_decaddrinfo(info);
				ret = 0;
			} else
				ret = 1;
		} else {
			fprintf(stderr, "Not a domain: '%s'\n", name);
			ret = 1;
		}
	} else {
		ret = 1;
	}

	rnndec_freecontext(vc);
	rnn_freedb(db);
	rnn_fini();

	return ret;
}
