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
#include "ed2_misc.h"
#include <stdio.h>

int main(int argc, char **argv) {
	struct ed2i_isa *isa = ed2i_read_isa(argv[1]);
	if (isa) {
		int i, j;
		for (i = 0; i < isa->featuresnum; i++) {
			printf ("Feature %s \"%s\"\n", isa->features[i].names[0], isa->features[i].description);
			for (j = 0; j < isa->featuresnum; j++) {
				if (ed2_mask_get(isa->features[i].ifmask, j) && j != i)
					printf ("\tImplies %s\n", isa->features[j].names[0]);
			}
			for (j = 0; j < isa->featuresnum; j++) {
				if (ed2_mask_get(isa->features[i].cfmask, j))
					printf ("\tConflicts with %s\n", isa->features[j].names[0]);
			}
		}
		for (i = 0; i < isa->variantsnum; i++) {
			printf ("Variant %s \"%s\"\n", isa->variants[i].names[0], isa->variants[i].description);
			for (j = 0; j < isa->featuresnum; j++) {
				if (ed2_mask_get(isa->variants[i].fmask, j))
					printf ("\tFeature %s\n", isa->features[j].names[0]);
			}
		}
		for (i = 0; i < isa->modesetsnum; i++) {
			struct ed2i_modeset *ms = &isa->modesets[i];
			int k;
			printf ("%s set %s \"%s\"\n", (ms->isoptional?"Optional mode":"Mode"), ms->names[0], ms->description);
			for (k = ms->firstmode; k < ms->firstmode + ms->modesnum; k++) {
				printf ("\t%s %s \"%s\"\n", (k == ms->defmode?"Default mode":"Mode"), isa->modes[k].names[0], isa->modes[k].description);
				for (j = 0; j < isa->featuresnum; j++) {
					if (ed2_mask_get(isa->modes[k].fmask, j))
						printf ("\t\tRequired feature %s\n", isa->features[j].names[0]);
				}
			}
		}
		ed2i_del_isa(isa);
	}
	return 0;
}
