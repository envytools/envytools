/*
 * Copyright (C) 2010 Luca Barbieri <luca@luca-barbieri.com>
 * Copyright (C) 2010 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "rnn.h"
#include "rnndec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

void expand (const char* prefix, struct rnndelem *elem, uint64_t base, int width, struct rnndeccontext *vc) {
	int i, j;
	if(elem->length > 1024)
		return;
	if(!rnndec_varmatch(vc, &elem->varinfo))
		return;
	switch (elem->type) {
		case RNN_ETYPE_REG:
			for(i = 0; i < elem->length; ++i) {
					if(elem->length >= 2)
						printf("d %s %08"PRIx64" %s(%i)", prefix, base + elem->offset + i * elem->stride, elem->fullname, i);
					else
						printf("d %s %08"PRIx64" %s", prefix, base + elem->offset, elem->fullname);

				printf(" %s\n", elem->typeinfo.name);
			}
			break;
		case RNN_ETYPE_STRIPE:
		case RNN_ETYPE_ARRAY:
			for (i = 0; i < elem->length; i++)
				for (j = 0; j < elem->subelemsnum; j++)
					expand(prefix, elem->subelems[j], base + elem->offset + i * elem->stride, width, vc);
			break;
		default:
			break;
	}
}

int main(int argc, char **argv) {
	char** argp;
	rnn_init();
	struct rnndb *db = rnn_newdb();
	rnn_parsefile (db, argv[1]);
	rnn_prepdb (db);
	struct rnndeccontext *vc = rnndec_newcontext(db);
	vc->colors = &rnndec_colorsterm;
	argp = argv + 2;
	while (*argp && !strcmp (*argp, "-v")) {
		++argp;
		if(!argp[0] || !argp[1])
			fprintf(stderr, "error: -v option lacking VARSET and VARIANT arguments following it\n");
		else {
			rnndec_varadd(vc, argp[0], argp[1]);
			argp += 2;
		}
	}
	int i, j;
	for(i = 0; i < db->domainsnum; ++i)
		for (j = 0; j < db->domains[i]->subelemsnum; ++j)
			expand (db->domains[i]->name, db->domains[i]->subelems[j], 0, db->domains[i]->width, vc);
	for(i = 0; i < db->enumsnum; ++i)
		for (j = 0; j < db->enums[i]->valsnum; ++j)
			if (db->enums[i]->vals[j]->valvalid)
				printf("e %s %"PRIx64" %s\n", db->enums[i]->name, db->enums[i]->vals[j]->value, db->enums[i]->vals[j]->name);
	return 0;
}

