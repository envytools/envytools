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

#include "ed2v.h"
#include "util.h"
#include <assert.h>
#include <stdlib.h>

struct ed2v_variant *ed2v_new_variant_i(struct ed2i_isa *isa, int variant) {
	assert(variant >= -1 && variant < isa->variantsnum);
	struct ed2v_variant *res = calloc(sizeof *res, 1);
	int i;
	res->mode = calloc(sizeof *res->mode, isa->modesetsnum);
	res->isa = isa;
	if (variant != -1) {
		res->fmask = mask_dup(isa->variants[variant].fmask, isa->featuresnum);
	} else {
		res->fmask = mask_new(isa->featuresnum);
	}
	for (i = 0; i < isa->modesetsnum; i++) {
		res->mode[i] = isa->modesets[i].defmode;
	}
	return res;
}

struct ed2v_variant *ed2v_new_variant(struct ed2i_isa *isa, const char *variant) {
	if (!variant)
		return ed2v_new_variant_i(isa, -1);
	int stype, sdata;
	if (symtab_get(isa->symtab, variant, &stype, &sdata) == -1 || stype != ED2I_ST_VARIANT) {
		fprintf (stderr, "%s is not a variant name\n", variant);
		return 0;
	}
	return ed2v_new_variant_i(isa, sdata);
}

void ed2v_del_variant(struct ed2v_variant *var) {
	free(var->fmask);
	free(var->mode);
	free(var);
}

int ed2v_add_feature_i(struct ed2v_variant *var, int feature) {
	int i;
	struct ed2i_feature *f = &var->isa->features[feature];
	assert(feature >= 0 && feature < var->isa->featuresnum);
	if (ed2v_has_feature(var, feature))
		return 0;
	for (i = 0; i < var->isa->featuresnum; i++) {
		if (ed2v_has_feature(var, i) && mask_get(f->cfmask, i)) {
			fprintf (stderr, "Conflicting ISA features used: %s and %s\n", f->names[0], var->isa->features[i].names[0]);
			return -1;
		}
	}
	mask_or(var->fmask, f->ifmask, var->isa->featuresnum);
	return 0;
}

int ed2v_add_feature(struct ed2v_variant *var, const char *feature) {
	int stype, sdata;
	if (symtab_get(var->isa->symtab, feature, &stype, &sdata) == -1 || stype != ED2I_ST_FEATURE) {
		fprintf (stderr, "%s is not a feature name\n", feature);
		return -1;
	}
	return ed2v_add_feature_i(var, sdata);
}

int ed2v_set_mode_i(struct ed2v_variant *var, int mode) {
	struct ed2i_mode *m = &var->isa->modes[mode];
	assert(mode >= 0 && mode < var->isa->modesnum);
	if (!mask_contains(var->fmask, m->fmask, var->isa->featuresnum)) {
		fprintf (stderr, "%i feature requirements not met\n", mode);
		return -1;
	}
	var->mode[m->modeset] = mode;
	return 0;
}

int ed2v_set_mode(struct ed2v_variant *var, const char *mode) {
	int stype, sdata;
	if (symtab_get(var->isa->symtab, mode, &stype, &sdata) == -1 || stype != ED2I_ST_MODE) {
		fprintf (stderr, "%s is not a mode name\n", mode);
		return -1;
	}
	return ed2v_set_mode_i(var, sdata);
}
