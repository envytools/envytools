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

void ed2ip_del_enumval(struct ed2ip_enumval *ev) {
	free(ev->name);
	free(ev);
}

void ed2ip_del_opfield(struct ed2ip_opfield *opf) {
	int i;
	for (i = 0; i < opf->enumvalsnum; i++) {
		ed2ip_del_enumval(opf->enumvals[i]);
	}
	free(opf->enumvals);
	free(opf->name);
	free(opf);
}

void ed2ip_del_isa(struct ed2ip_isa *isa) {
	int i;
	for (i = 0; i < isa->featuresnum; i++)
		ed2ip_del_feature(isa->features[i]);
	for (i = 0; i < isa->variantsnum; i++)
		ed2ip_del_variant(isa->variants[i]);
	for (i = 0; i < isa->modesetsnum; i++)
		ed2ip_del_modeset(isa->modesets[i]);
	for (i = 0; i < isa->opfieldsnum; i++)
		ed2ip_del_opfield(isa->opfields[i]);
	free(isa);
}

static const char *const typenames[] = {
	[ED2I_ST_FEATURE] = "feature",
	[ED2I_ST_VARIANT] = "variant",
	[ED2I_ST_MODESET] = "modeset",
	[ED2I_ST_MODE] = "mode",
};

int ed2ip_transform_sym(struct ed2i_isa *isa, char *name, int type, struct ed2_loc *loc, int *pbroken) {
	int idx = ed2s_symtab_get(isa->symtab, name);
	if (idx == -1) {
		fprintf (stderr, ED2_LOC_FORMAT(*loc, "Undefined symbol %s\n"), name);
		*pbroken |= 1;
		free(name);
		return -1;
	} else if (isa->symtab->syms[idx].type != type) {
		fprintf (stderr, ED2_LOC_FORMAT(*loc, "Symbol %s is not a %s\n"), name, typenames[type]);
		*pbroken |= 1;
		free(name);
		return -1;
	} else {
		free(name);
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

uint32_t *ed2ip_list_to_fmask(struct ed2i_isa *isa, char **names, int namesnum, struct ed2_loc *loc, int *pbroken) {
	uint32_t *res = ed2_mask_new(isa->featuresnum);
	int i;
	for (i = 0; i < namesnum; i++) {
		int idx = ed2ip_transform_sym(isa, names[i], ED2I_ST_FEATURE, loc, pbroken);
		if (idx != -1)
			ed2_mask_set(res, idx);
	}
	free(names);
	return res;
}

void ed2ip_ifmask_closure(struct ed2i_isa *isa, uint32_t *fmask) {
	int i;
	for (i = 0; i < isa->featuresnum; i++) {
		if (ed2_mask_get(fmask, i)) {
			if (ed2_mask_or_r(fmask, isa->features[i].ifmask, isa->featuresnum)) {
				i = 0;
			}
		}
	}
}

void ed2ip_fmask_closure(struct ed2i_isa *isa, uint32_t *fmask) {
	int i;
	for (i = 0; i < isa->featuresnum; i++) {
		if (ed2_mask_get(fmask, i)) {
			ed2_mask_or(fmask, isa->features[i].ifmask, isa->featuresnum);
		}
	}
}

int ed2ip_fmask_conflicts(struct ed2i_isa *isa, uint32_t *fmask, struct ed2_loc *loc) {
	int i;
	for (i = 0; i < isa->featuresnum; i++) {
		if (ed2_mask_get(fmask, i)) {
			int idx = ed2_mask_intersect(fmask, isa->features[i].cfmask, isa->featuresnum);
			if (idx != -1) {
				fprintf(stderr, ED2_LOC_FORMAT(*loc, "Conflicting features %s and %s\n"), isa->features[i].names[0], isa->features[idx].names[0]);
				return 1;
			}
		}
	}
	return 0;
}

int ed2ip_transform_features(struct ed2ip_isa *preisa, struct ed2i_isa *isa) {
	int i, j, k;
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
	/* initialise both masks as written */
	for (i = 0; i < isa->featuresnum; i++) {
		isa->features[i].ifmask = ed2ip_list_to_fmask(isa, preisa->features[i]->implies, preisa->features[i]->impliesnum, &preisa->features[i]->loc, &broken);
		isa->features[i].cfmask = ed2ip_list_to_fmask(isa, preisa->features[i]->conflicts, preisa->features[i]->conflictsnum, &preisa->features[i]->loc, &broken);
	}
	for (i = 0; i < isa->featuresnum; i++) {
		/* every feature implies itself */
		ed2_mask_set(isa->features[i].ifmask, i);
		/* calculate full closure of implied features */
		ed2ip_ifmask_closure(isa, isa->features[i].ifmask);
		/* if i conflicts with j, j conflicts with i */
		for (j = 0; j < isa->featuresnum; j++) {
			if (ed2_mask_get(isa->features[i].cfmask, j))
				ed2_mask_set(isa->features[j].cfmask, i);
		}
	}
	for (i = 0; i < isa->featuresnum; i++) {
		/* if i implies k, j implies l, and k conflicts with l, i conflicts with j */
		for (j = 0; j < isa->featuresnum; j++) {
			for (k = 0; k < isa->featuresnum; k++) {
				if (ed2_mask_get(isa->features[i].ifmask, k) && ed2_mask_intersect(isa->features[k].cfmask, isa->features[j].ifmask, isa->featuresnum) != -1) {
					ed2_mask_set(isa->features[i].cfmask, j);
					ed2_mask_set(isa->features[j].cfmask, i);
				}
			}
		}
	}
	for (i = 0; i < isa->featuresnum; i++) {
		broken |= ed2ip_fmask_conflicts(isa, isa->features[i].ifmask, &preisa->features[i]->loc);
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

		isa->variants[i].fmask = ed2ip_list_to_fmask(isa, preisa->variants[i]->features, preisa->variants[i]->featuresnum, &preisa->variants[i]->loc, &broken);
		ed2ip_fmask_closure(isa, isa->variants[i].fmask);

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

			isa->modes[kk].fmask = ed2ip_list_to_fmask(isa, ms->modes[k]->features, ms->modes[k]->featuresnum, &ms->modes[k]->loc, &broken);
			ed2ip_fmask_closure(isa, isa->modes[kk].fmask);

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

			free(ms->modes[k]);
		}
		free(ms->modes);
		free(ms);
	}
	free(preisa->modesets);
	return broken;
}

int ed2ip_transform_opfields(struct ed2ip_isa *preisa, struct ed2i_isa *isa) {
	int i, j;
	int broken = 0;
	isa->opfieldsnum = preisa->opfieldsnum;
	isa->opfields = calloc(sizeof *isa->opfields, isa->opfieldsnum);
	isa->opbits = 0;
	for (i = 0; i < isa->opfieldsnum; i++) {
		struct ed2ip_opfield *popf = preisa->opfields[i];
		struct ed2i_opfield *opf = &isa->opfields[i];
		int def = -1;
		opf->name = popf->name;
		if (popf->enumvalsnum) {
			opf->len = ed2_lg2ceil(popf->enumvalsnum);
			opf->enumvalsnum = popf->enumvalsnum;
			opf->enumvals = calloc(sizeof *opf->enumvals, opf->enumvalsnum);
			for (j = 0; j < opf->enumvalsnum; j++) {
				opf->enumvals[j] = popf->enumvals[j]->name;
				if (popf->enumvals[j]->isdefault) {
					if (def == -1) {
						def = j;
					} else {
						fprintf (stderr, "More than one default value in OPFIELD %s: %s and %s\n", opf->name, opf->enumvals[def], opf->enumvals[j]);
						broken = 1;
					}
				}
			}
			if (def != -1) {
				if (popf->hasdef) {
					fprintf (stderr, "More than one default value in OPFIELD %s: %s and %"PRIx64"\n", opf->name, opf->enumvals[def], popf->defval);
					broken = 1;
				} else {
					popf->hasdef = 1;
					popf->defval = def;
				}
			}
		} else {
			isa->opfields[i].len = preisa->opfields[i]->len;
		}
		isa->opfields[i].start = isa->opbits;
		isa->opbits += isa->opfields[i].len;
	}
	isa->opdefault = ed2_mask_new(isa->opbits);
	isa->opdefmask = ed2_mask_new(isa->opbits);
	for (i = 0; i < isa->opfieldsnum; i++) {
		if (preisa->opfields[i]->hasdef) {
			ed2i_set_opfield_1(isa->opdefmask, &isa->opfields[i]);
			ed2i_set_opfield_val(isa->opdefault, &isa->opfields[i], preisa->opfields[i]->defval);
		}
		free(preisa->opfields[i]);
	}
	free(preisa->opfields);
	return broken;
}

struct ed2i_isa *ed2ip_transform(struct ed2ip_isa *preisa) {
	int broken = preisa->broken;
	struct ed2i_isa *isa = calloc(sizeof *isa, 1);
	isa->symtab = ed2s_symtab_new();
	broken |= ed2ip_transform_features(preisa, isa);
	broken |= ed2ip_transform_variants(preisa, isa);
	broken |= ed2ip_transform_modesets(preisa, isa);
	broken |= ed2ip_transform_opfields(preisa, isa);
	free(preisa);
	if (broken) {
		ed2i_del_isa(isa);
		return 0;
	} else {
		return isa;
	}
}
