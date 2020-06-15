/*
 * Copyright (C) 2012 Marcelina Kościelnicka <mwk@0x04.net>
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

struct varinfo *varinfo_new(struct vardata *data) {
	struct varinfo *res = calloc(sizeof *res, 1);
	int i;
	res->data = data;
	res->fmask = mask_new(data->featuresnum);
	res->variants = calloc(sizeof *res->variants, data->varsetsnum);
	for (i = 0; i < data->varsetsnum; i++)
		res->variants[i] = -1;
	res->modes = calloc(sizeof *res->modes, data->modesetsnum);
	for (i = 0; i < data->modesetsnum; i++)
		res->modes[i] = data->modesets[i].defmode;
	return res;
}

void varinfo_del(struct varinfo *info) {
	free(info->fmask);
	free(info->variants);
	free(info->modes);
	free(info);
}

int varinfo_set_variant(struct varinfo *info, const char *variant) {
	int data;
	if (symtab_get_t(info->data->symtab, variant, VARDATA_ST_VARIANT, &data) == -1) {
		fprintf(stderr, "no variant %s\n", variant);
		return -1;
	}
	int varset = info->data->variants[data].varset;
	if (info->variants[varset] != -1 && info->variants[varset] != data) {
		fprintf(stderr, "a variant from varset %s has already been selected\n", info->data->varsets[varset].name);
		return -1;
	}
	int i;
	for (i = 0; i < info->data->featuresnum; i++) {
		if (mask_get(info->fmask, i) && mask_intersect(info->data->features[i].cfmask, info->data->variants[data].fmask, info->data->featuresnum) != -1) {
			fprintf(stderr, "variant %s conflicts with feature %s\n", info->data->variants[data].name, info->data->features[i].name);
			return -1;
		}
	}
	info->variants[varset] = data;
	mask_or(info->fmask, info->data->variants[data].fmask, info->data->featuresnum);
	return 0;
}

int varinfo_set_feature(struct varinfo *info, const char *feature) {
	int data;
	if (symtab_get_t(info->data->symtab, feature, VARDATA_ST_FEATURE, &data) == -1) {
		fprintf(stderr, "no feature %s\n", feature);
		return -1;
	}
	int cf;
	if ((cf = mask_intersect(info->data->features[data].cfmask, info->fmask, info->data->featuresnum)) != -1) {
		fprintf(stderr, "feature %s conflicts with already set feature %s\n", info->data->features[data].name, info->data->features[cf].name);
		return -1;
	}
	mask_or(info->fmask, info->data->features[data].ifmask, info->data->featuresnum);
	return 0;
}

int varinfo_set_mode(struct varinfo *info, const char *mode) {
	int data;
	if (symtab_get_t(info->data->symtab, mode, VARDATA_ST_MODE, &data) == -1) {
		fprintf(stderr, "no mode %s\n", mode);
		return -1;
	}
	int modeset = info->data->modes[data].modeset;
	if (info->modes[modeset] != -1 && info->modes[modeset] != data) {
		fprintf(stderr, "a mode from modeset %s has already been selected\n", info->data->modesets[modeset].name);
		return -1;
	}
	info->modes[modeset] = data;
	/* XXX: validate mode */
	return 0;
}
