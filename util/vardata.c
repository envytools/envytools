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

struct vardata *vardata_new(const char *name) {
	struct vardata *res = calloc(sizeof *res, 1);
	res->name = strdup(name);
	res->symtab = symtab_new();
	return res;
}

void vardata_del(struct vardata *data) {
	int i;
	free(data->name);
	for (i = 0; i < data->featuresnum; i++) {
		free(data->features[i].name);
		free(data->features[i].description);
		free(data->features[i].ifmask);
		free(data->features[i].cfmask);
		free(data->features[i].implies);
		free(data->features[i].conflicts);
	}
	free(data->features);
	for (i = 0; i < data->variantsnum; i++) {
		free(data->variants[i].name);
		free(data->variants[i].description);
		free(data->variants[i].fmask);
		free(data->variants[i].features);
	}
	free(data->variants);
	for (i = 0; i < data->varsetsnum; i++) {
		free(data->varsets[i].name);
		free(data->varsets[i].description);
	}
	free(data->varsets);
	for (i = 0; i < data->modesnum; i++) {
		free(data->modes[i].name);
		free(data->modes[i].description);
		free(data->modes[i].rfmask);
		free(data->modes[i].rfeatures);
	}
	free(data->modes);
	for (i = 0; i < data->modesetsnum; i++) {
		free(data->modesets[i].name);
		free(data->modesets[i].description);
	}
	free(data->modesets);
	symtab_del(data->symtab);
	free(data);
}

int vardata_add_feature(struct vardata *data, const char *name, const char *description) {
	if (data->validated)
		abort();
	if (symtab_put(data->symtab, name, VARDATA_ST_FEATURE, data->featuresnum) == -1)
		return -1;
	int res = data->featuresnum;
	struct vardata_feature new = { strdup(name) };
	if (description)
		new.description = strdup(description);
	ADDARRAY(data->features, new);
	return res;
}

int vardata_add_varset(struct vardata *data, const char *name, const char *description) {
	if (data->validated)
		abort();
	if (symtab_put(data->symtab, name, VARDATA_ST_VARSET, data->varsetsnum) == -1)
		return -1;
	int res = data->varsetsnum;
	struct vardata_varset new = { strdup(name) };
	if (description)
		new.description = strdup(description);
	ADDARRAY(data->varsets, new);
	return res;
}

int vardata_add_variant(struct vardata *data, const char *name, const char *description, int varset) {
	if (data->validated)
		abort();
	if (symtab_put(data->symtab, name, VARDATA_ST_VARIANT, data->variantsnum) == -1)
		return -1;
	int res = data->variantsnum;
	struct vardata_variant new = { strdup(name), .varset = varset };
	if (description)
		new.description = strdup(description);
	ADDARRAY(data->variants, new);
	return res;
}

int vardata_add_modeset(struct vardata *data, const char *name, const char *description) {
	if (data->validated)
		abort();
	if (symtab_put(data->symtab, name, VARDATA_ST_MODESET, data->modesetsnum) == -1)
		return -1;
	int res = data->modesetsnum;
	struct vardata_modeset new = { strdup(name), .defmode = -1 };
	if (description)
		new.description = strdup(description);
	ADDARRAY(data->modesets, new);
	return res;
}

int vardata_add_mode(struct vardata *data, const char *name, const char *description, int modeset) {
	if (data->validated)
		abort();
	if (symtab_put(data->symtab, name, VARDATA_ST_MODE, data->modesnum) == -1)
		return -1;
	int res = data->modesnum;
	struct vardata_mode new = { strdup(name), .modeset = modeset };
	if (description)
		new.description = strdup(description);
	ADDARRAY(data->modes, new);
	return res;
}

void vardata_feature_imply(struct vardata *data, int f1, int f2) {
	ADDARRAY(data->features[f1].implies, f2);
}

void vardata_feature_conflict(struct vardata *data, int f1, int f2) {
	ADDARRAY(data->features[f1].conflicts, f2);
}

void vardata_variant_feature(struct vardata *data, int v, int f) {
	ADDARRAY(data->variants[v].features, f);
}

void vardata_mode_require(struct vardata *data, int m, int f) {
	ADDARRAY(data->modes[m].rfeatures, f);
}

int vardata_modeset_def(struct vardata *data, int ms, int m) {
	if (data->modesets[ms].defmode != -1 || data->modes[m].modeset != ms)
		return -1;
	data->modesets[ms].defmode = m;
	return 0;
}

int vardata_validate(struct vardata *data) {
	int res = 0;
	if (data->validated)
		abort();
	data->validated = 1;
	int i, j;
	for (i = 0; i < data->featuresnum; i++) {
		data->features[i].ifmask = mask_new(data->featuresnum);
		data->features[i].cfmask = mask_new(data->featuresnum);
		mask_set(data->features[i].ifmask, i);
	}
	for (i = 0; i < data->variantsnum; i++) {
		data->variants[i].fmask = mask_new(data->featuresnum);
	}
	for (i = 0; i < data->modesnum; i++) {
		data->modes[i].rfmask = mask_new(data->featuresnum);
	}
	/* implies */
	for (i = 0; i < data->featuresnum; i++) {
		for (j = 0; j < data->features[i].impliesnum; j++) {
			int k = data->features[i].implies[j];
			int l;
			for (l = 0; l < data->featuresnum; l++) {
				if (mask_get(data->features[l].ifmask, i)) {
					mask_or(data->features[l].ifmask, data->features[k].ifmask, data->featuresnum);
				}
			}
		}
	}
	/* conflicts */
	for (i = 0; i < data->featuresnum; i++) {
		for (j = 0; j < data->features[i].conflictsnum; j++) {
			int k = data->features[i].conflicts[j];
			mask_set(data->features[i].cfmask, k);
			mask_set(data->features[k].cfmask, i);
		}
	}
	int p = 1;
	while (p) {
		p = 0;
		for (i = 0; i < data->featuresnum; i++) {
			for (j = 0; j < data->featuresnum; j++) {
				if (mask_get(data->features[i].ifmask, j)) {
					p |= mask_or_r(data->features[i].cfmask, data->features[j].cfmask, data->featuresnum);
				}
			}
		}
	}
	for (i = 0; i < data->featuresnum; i++) {
		if (mask_get(data->features[i].cfmask, i)) {
			fprintf(stderr, "vardata %s: feature %s conflicts with itself!\n", data->name, data->features[i].name);
			res = 1;
		}
	}
	/* variants */
	for (i = 0; i < data->variantsnum; i++) {
		for (j = 0; j < data->variants[i].featuresnum; j++) {
			int k = data->variants[i].features[j];
			mask_or(data->variants[i].fmask, data->features[k].ifmask, data->featuresnum);
		}
	}
	/* variants */
	for (i = 0; i < data->modesnum; i++) {
		for (j = 0; j < data->modes[i].rfeaturesnum; j++) {
			int k = data->modes[i].rfeatures[j];
			mask_set(data->modes[i].rfmask, k);
		}
	}
	return res;
}
