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

#include "ed2_misc.h"
#include "mask.h"
#include "ed2i.h"
#include <stdlib.h>

void ed2i_del_isa(struct ed2i_isa *isa) {
	ed2s_symtab_del(isa->symtab);
	int i;
	for (i = 0; i < isa->featuresnum; i++) {
		ed2_free_strings(isa->features[i].names, isa->features[i].namesnum);
		free(isa->features[i].description);
		free(isa->features[i].ifmask);
		free(isa->features[i].cfmask);
	}
	free(isa->features);
	for (i = 0; i < isa->variantsnum; i++) {
		ed2_free_strings(isa->variants[i].names, isa->variants[i].namesnum);
		free(isa->variants[i].description);
		free(isa->variants[i].fmask);
	}
	free(isa->variants);
	for (i = 0; i < isa->modesnum; i++) {
		ed2_free_strings(isa->modes[i].names, isa->modes[i].namesnum);
		free(isa->modes[i].description);
		free(isa->modes[i].fmask);
	}
	free(isa->modes);
	for (i = 0; i < isa->modesetsnum; i++) {
		ed2_free_strings(isa->modesets[i].names, isa->modesets[i].namesnum);
		free(isa->modesets[i].description);
	}
	free(isa->modesets);
	for (i = 0; i < isa->opfieldsnum; i++) {
		free(isa->opfields[i].name);
		ed2_free_strings(isa->opfields[i].enumvals, isa->opfields[i].enumvalsnum);
	}
	free(isa->opfields);
	free(isa->opdefault);
	free(isa->opdefmask);
	free(isa);
}

void ed2i_set_opfield_1(uint32_t *mask, struct ed2i_opfield *opf) {
	/* XXX: unsuck? */
	int i;
	for (i = 0; i < opf->len; i++) {
		mask_set(mask, opf->start + i);
	}
}

void ed2i_set_opfield_val(uint32_t *mask, struct ed2i_opfield *opf, uint64_t val) {
	/* XXX: unsuck? */
	int i;
	for (i = 0; i < opf->len; i++) {
		if (i < 64 && ((val >> i) & 1))
			mask_set(mask, opf->start + i);
	}
}
