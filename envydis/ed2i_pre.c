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

#include "ed2i_pre.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void ed2ip_del_feature(struct ed2ip_feature *f) {
	ed2_free_strings(f->names, f->namesnum);
       	ed2_free_strings(f->conflicts, f->conflictsnum);
       	ed2_free_strings(f->implies, f->impliesnum);
	free(f->description);
	free(f);
}

void ed2ip_del_variant(struct ed2ip_variant *v) {
	ed2_free_strings(v->names, v->namesnum);
       	ed2_free_strings(v->features, v->featuresnum);
	free(v->description);
	free(v);
}

void ed2ip_del_modeset(struct ed2ip_modeset *ms) {
	int i;
	ed2_free_strings(ms->names, ms->namesnum);
	free(ms->description);
	for (i = 0; i < ms->modesnum; i++)
		ed2ip_del_mode(ms->modes[i]);
	free(ms);
}

void ed2ip_del_mode(struct ed2ip_mode *m) {
	ed2_free_strings(m->names, m->namesnum);
	ed2_free_strings(m->features, m->featuresnum);
	free(m->description);
	free(m);
}

void ed2ip_del_isa(struct ed2ip_isa *isa) {
	int i;
	for (i = 0; i < isa->featuresnum; i++)
		ed2ip_del_feature(isa->features[i]);
	for (i = 0; i < isa->variantsnum; i++)
		ed2ip_del_variant(isa->variants[i]);
	for (i = 0; i < isa->modesetsnum; i++)
		ed2ip_del_modeset(isa->modesets[i]);
	free(isa);
}

static const char *const typenames[] = {
	[ED2I_ST_FEATURE] = "feature",
	[ED2I_ST_VARIANT] = "variant",
	[ED2I_ST_MODESET] = "modeset",
	[ED2I_ST_MODE] = "mode",
};

int ed2ip_find_sym(struct ed2i_isa *isa, const char *name, int type, struct ed2_loc *loc, int *pbroken) {
	int idx = ed2s_symtab_get(isa->symtab, name);
	if (idx == -1) {
		fprintf (stderr, ED2_LOC_FORMAT(*loc, "Undefined symbol %s\n"), name);
		*pbroken |= 1;
		return -1;
	} else if (isa->symtab->syms[idx].type != type) {
		fprintf (stderr, ED2_LOC_FORMAT(*loc, "Symbol %s is not a %s\n"), name, typenames[type]);
		*pbroken |= 1;
		return -1;
	} else {
		return isa->symtab->syms[idx].idata;
	}
}

int ed2ip_put_sym(struct ed2i_isa *isa, char *name, int type, struct ed2_loc *loc, int idata) {
	int idx = ed2s_symtab_put(isa->symtab, name);
	if (idx == -1) {
		fprintf (stderr, ED2_LOC_FORMAT(*loc, "Redefined symbol %s\n"), name);
		return 1;
	} else {
		isa->symtab->syms[idx].type = type;
		isa->symtab->syms[idx].idata = idata;
		return 0;
	}
}

int ed2ip_transform_features(struct ed2ip_isa *preisa, struct ed2i_isa *isa) {
	int i, j;
	int broken = 0;
	isa->featuresnum = preisa->featuresnum;
	isa->features = calloc(sizeof *isa->features, isa->featuresnum);
	for (i = 0; i < isa->featuresnum; i++) {
		for (j = 0; j < preisa->features[i]->namesnum; j++) {
			broken |= ed2ip_put_sym(isa, preisa->features[i]->names[j], ED2I_ST_FEATURE, &preisa->features[i]->loc, i);
		}
		isa->features[i].names = preisa->features[i]->names;
		isa->features[i].namesnum = preisa->features[i]->namesnum;
		isa->features[i].description = preisa->features[i]->description;
	}
	for (i = 0; i < isa->featuresnum; i++) {
		isa->features[i].impliesnum = preisa->features[i]->impliesnum;
		isa->features[i].implies = calloc(sizeof *isa->features[i].implies, isa->features[i].impliesnum);
		for (j = 0; j < isa->features[i].impliesnum; j++) {
			isa->features[i].implies[j] = ed2ip_find_sym(isa, preisa->features[i]->implies[j], ED2I_ST_FEATURE, &preisa->features[i]->loc, &broken);
			free(preisa->features[i]->implies[j]);
		}
		free(preisa->features[i]->implies);
		isa->features[i].conflictsnum = preisa->features[i]->conflictsnum;
		isa->features[i].conflicts = calloc(sizeof *isa->features[i].conflicts, isa->features[i].conflictsnum);
		for (j = 0; j < isa->features[i].conflictsnum; j++) {
			isa->features[i].conflicts[j] = ed2ip_find_sym(isa, preisa->features[i]->conflicts[j], ED2I_ST_FEATURE, &preisa->features[i]->loc, &broken);
			free(preisa->features[i]->conflicts[j]);
		}
		free(preisa->features[i]->conflicts);
		free(preisa->features[i]);
	}
	free(preisa->features);
	return broken;
}

int ed2ip_transform_variants(struct ed2ip_isa *preisa, struct ed2i_isa *isa) {
	int i, j;
	int broken = 0;
	isa->variantsnum = preisa->variantsnum;
	isa->variants = calloc(sizeof *isa->variants, isa->variantsnum);
	for (i = 0; i < isa->variantsnum; i++) {
		for (j = 0; j < preisa->variants[i]->namesnum; j++) {
			broken |= ed2ip_put_sym(isa, preisa->variants[i]->names[j], ED2I_ST_VARIANT, &preisa->variants[i]->loc, i);
		}
		isa->variants[i].names = preisa->variants[i]->names;
		isa->variants[i].namesnum = preisa->variants[i]->namesnum;
		isa->variants[i].description = preisa->variants[i]->description;

		isa->variants[i].featuresnum = preisa->variants[i]->featuresnum;
		isa->variants[i].features = calloc(sizeof *isa->variants[i].features, isa->variants[i].featuresnum);
		for (j = 0; j < isa->variants[i].featuresnum; j++) {
			isa->variants[i].features[j] = ed2ip_find_sym(isa, preisa->variants[i]->features[j], ED2I_ST_FEATURE, &preisa->variants[i]->loc, &broken);
			free(preisa->variants[i]->features[j]);
		}
		free(preisa->variants[i]->features);
		free(preisa->variants[i]);
	}
	free(preisa->variants);
	return broken;
}

int ed2ip_transform_modesets(struct ed2ip_isa *preisa, struct ed2i_isa *isa) {
	int i, j, k;
	int broken = 0;
	isa->modesetsnum = preisa->modesetsnum;
	isa->modesets = calloc(sizeof *isa->modesets, isa->modesetsnum);
	int modesnum = 0;
	for (i = 0; i < isa->modesetsnum; i++) {
		for (j = 0; j < preisa->modesets[i]->namesnum; j++) {
			broken |= ed2ip_put_sym(isa, preisa->modesets[i]->names[j], ED2I_ST_MODESET, &preisa->modesets[i]->loc, i);
		}
		isa->modesets[i].names = preisa->modesets[i]->names;
		isa->modesets[i].namesnum = preisa->modesets[i]->namesnum;
		isa->modesets[i].description = preisa->modesets[i]->description;
		isa->modesets[i].isoptional = preisa->modesets[i]->isoptional;
		isa->modesets[i].firstmode = modesnum;
		isa->modesets[i].modesnum = preisa->modesets[i]->modesnum;
		isa->modesets[i].defmode = -1;
		modesnum += isa->modesets[i].modesnum;
	}
	isa->modesnum = modesnum;
	isa->modes = calloc(sizeof *isa->modesets, isa->modesnum);
	for (i = 0; i < isa->modesetsnum; i++) {
		struct ed2ip_modeset *ms = preisa->modesets[i];
		for (k = 0; k < ms->modesnum; k++) {
			int kk = k + isa->modesets[i].firstmode;
			for (j = 0; j < ms->modes[k]->namesnum; j++) {
				broken |= ed2ip_put_sym(isa, ms->modes[k]->names[j], ED2I_ST_MODE, &ms->modes[k]->loc, kk);
			}
			isa->modes[kk].names = ms->modes[k]->names;
			isa->modes[kk].namesnum = ms->modes[k]->namesnum;
			isa->modes[kk].description = ms->modes[k]->description;

			isa->modes[kk].featuresnum = ms->modes[k]->featuresnum;
			isa->modes[kk].features = calloc(sizeof *isa->modes[kk].features, isa->modes[kk].featuresnum);
			for (j = 0; j < isa->modes[kk].featuresnum; j++) {
				isa->modes[kk].features[j] = ed2ip_find_sym(isa, ms->modes[k]->features[j], ED2I_ST_FEATURE, &ms->modes[k]->loc, &broken);
				free(ms->modes[k]->features[j]);
			}
			if (ms->modes[k]->isdefault) {
				if (ms->modes[k]->featuresnum) {
					fprintf (stderr, "Default mode %s has required features\n", isa->modes[kk].names[0]);
					broken = 1;
				}
				if (isa->modesets[i].defmode == -1) {
					isa->modesets[i].defmode = kk;
				} else {
					fprintf (stderr, "More than one default mode: %s and %s\n", isa->modes[isa->modesets[i].defmode].names[0], isa->modes[kk].names[0]);
					broken = 1;
				}
			}
			free(ms->modes[k]->features);
			free(ms->modes[k]);
		}
		free(ms->modes);
		free(ms);
	}
	free(preisa->modesets);
	return broken;
}

struct ed2i_isa *ed2ip_transform(struct ed2ip_isa *preisa) {
	int broken = preisa->broken;
	struct ed2i_isa *isa = calloc(sizeof *isa, 1);
	isa->symtab = ed2s_symtab_new();
	broken |= ed2ip_transform_features(preisa, isa);
	broken |= ed2ip_transform_variants(preisa, isa);
	broken |= ed2ip_transform_modesets(preisa, isa);
	free(preisa);
	if (broken) {
		ed2i_del_isa(isa);
		return 0;
	} else {
		return isa;
	}
}
