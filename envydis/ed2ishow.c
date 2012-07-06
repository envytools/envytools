/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ed2i.h"
#include "mask.h"
#include <stdio.h>

void vardata_show(struct vardata *vardata) {
	int i, j;
	printf ("Features:\n");
	for (i = 0; i < vardata->featuresnum; i++) {
		printf ("Feature %s \"%s\"\n", vardata->features[i].name, vardata->features[i].description);
		for (j = 0; j < vardata->featuresnum; j++) {
			if (mask_get(vardata->features[i].ifmask, j) && j != i)
				printf ("\tImplies %s\n", vardata->features[j].name);
		}
		for (j = 0; j < vardata->featuresnum; j++) {
			if (mask_get(vardata->features[i].cfmask, j))
				printf ("\tConflicts with %s\n", vardata->features[j].name);
		}
	}
	printf ("\n");
	printf ("Variants:\n");
	for (i = 0; i < vardata->variantsnum; i++) {
		printf ("Variant %s \"%s\"\n", vardata->variants[i].name, vardata->variants[i].description);
		for (j = 0; j < vardata->featuresnum; j++) {
			if (mask_get(vardata->variants[i].fmask, j))
				printf ("\tFeature %s\n", vardata->features[j].name);
		}
	}
	printf ("\n");
	for (i = 0; i < vardata->modesetsnum; i++) {
		struct vardata_modeset *ms = &vardata->modesets[i];
		int k;
		printf ("%s set %s \"%s\"\n", (1/*ms->isoptional*/?"Optional mode":"Mode"), ms->name, ms->description);
		for (k = 0; k < vardata->modesnum; k++) {
			if (vardata->modes[k].modeset == i) {
				printf ("\t%s %s \"%s\"\n", (k == ms->defmode?"Default mode":"Mode"), vardata->modes[k].name, vardata->modes[k].description);
				for (j = 0; j < vardata->featuresnum; j++) {
					if (mask_get(vardata->modes[k].rfmask, j))
						printf ("\t\tRequired feature %s\n", vardata->features[j].name);
				}
			}
		}
		printf ("\n");
	}
}

int main(int argc, char **argv) {
	struct ed2i_isa *isa = ed2i_read_isa(argv[1]);
	if (isa) {
		vardata_show(isa->vardata);
		int i, j;
		printf("Opcode length %d bits, fields:\n", isa->opbits);
		for (i = 0; i < isa->opfieldsnum; i++) {
			struct ed2i_opfield *of = &isa->opfields[i];
			printf ("Opcode field %s: bits %d-%d\n", of->name, of->start, of->start + of->len - 1);
			for (j = 0; j < of->enumvalsnum; j++) {
				printf("\tValue %d: %s\n", j, of->enumvals[j]);
			}
		}
		printf ("Defbits: "); mask_print(stdout, isa->opdefault, isa->opbits); printf("\n");
		printf ("Defmask: "); mask_print(stdout, isa->opdefmask, isa->opbits); printf("\n");
		ed2i_del_isa(isa);
	}
	return 0;
}
