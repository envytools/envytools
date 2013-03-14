/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "var.h"

struct varselect *varselect_new(struct vardata *data) {
	struct varselect *res = calloc(sizeof *res, 1);
	res->data = data;
	res->fmask = mask_new(data->featuresnum);
	res->msmask = mask_new(data->modesetsnum);
	res->mmask = mask_new(data->modesnum);
	res->vsmask = mask_new(data->varsetsnum);
	res->vmask = mask_new(data->variantsnum);
	return res;
}

void varselect_del(struct varselect *select) {
	free(select->fmask);
	free(select->msmask);
	free(select->mmask);
	free(select->vsmask);
	free(select->vmask);
	free(select);
}

void varselect_need_feature(struct varselect *select, int f) {
	mask_set(select->fmask, f);
}

void varselect_need_mode(struct varselect *select, int m) {
	mask_set(select->msmask, select->data->modes[m].modeset);
	mask_set(select->mmask, m);
}

void varselect_need_variant(struct varselect *select, int v) {
	mask_set(select->vsmask, select->data->variants[v].varset);
	mask_set(select->vmask, v);
}

int varselect_match(struct varselect *select, struct varinfo *info) {
	if (!select)
		return 1;
	if (select->data != info->data)
		abort();
	if (!mask_contains(info->fmask, select->fmask, info->data->featuresnum))
		return 0;
	int i;
	for (i = 0; i < info->data->varsetsnum; i++) {
		if (mask_get(select->vsmask, i)) {
			if (info->variants[i] == -1) {
				fprintf(stderr, "Warning: don't know variant from varset %s, assuming a match\n", info->data->varsets[i].name);
			} else if (!mask_get(select->vmask, info->variants[i])) {
				return 0;
			}
		}
	}
	for (i = 0; i < info->data->modesetsnum; i++) {
		if (mask_get(select->msmask, i)) {
			if (info->modes[i] == -1) {
				if (info->data->modesets[i].defmode == -1) {
					fprintf(stderr, "Warning: don't know mode from modeset %s, assuming a match\n", info->data->modesets[i].name);
				} else if (!mask_get(select->mmask, info->data->modesets[i].defmode)) {
					return 0;
				}
			} else if (!mask_get(select->mmask, info->modes[i])) {
				return 0;
			}
		}
	}
	return 1;
}
