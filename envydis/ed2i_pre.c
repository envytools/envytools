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
#include "mask.h"
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

void ed2ip_del_insn(struct ed2ip_insn *in) {
	free(in->iname);
	free(in);
}

void ed2ip_del_seq(struct ed2ip_seq *s) {
	int i;
	free(s->name);
	for (i = 0; i < s->seqsnum; i++)
		ed2ip_del_seq(s->seqs[i]);
	free(s->seqs);
	for (i = 0; i < s->casesnum; i++)
		ed2ip_del_case(s->cases[i]);
	free(s->cases);
	if (s->insn_inline)
		ed2ip_del_insn(s->insn_inline);
	free(s->call_seq);
	if (s->bf_dst)
		ed2ip_del_bitfield(s->bf_dst);
	if (s->bf_src)
		ed2ip_del_bitfield(s->bf_src);
	free(s->const_str);
	free(s);
}

void ed2ip_del_case(struct ed2ip_case *c) {
	int i;
	for (i = 0; i < c->partsnum; i++)
		ed2ip_del_casepart(c->parts[i]);
	free(c->parts);
	free(c);
}

void ed2ip_del_casepart(struct ed2ip_casepart *cp) {
	int i;
	free(cp->bf);
	for (i = 0; i < cp->valsnum; i++)
		ed2ip_del_caseval(cp->vals[i]);
	free(cp->vals);
	ed2_free_strings(cp->names, cp->namesnum);
	free(cp);
}

void ed2ip_del_caseval(struct ed2ip_caseval *cv) {
	free(cv->str);
	free(cv);
}

void ed2ip_del_bitfield(struct ed2ip_bitfield *bf) {
	int i;
	for (i = 0; i < bf->chunksnum; i++)
		free(bf->chunks[i]);
	free(bf->chunks);
	free(bf);
}

void ed2ip_del_bitchunk(struct ed2ip_bitchunk *bc) {
	free(bc->name);
	free(bc);
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
};

int ed2ip_transform_sym(struct ed2i_isa *isa, char *name, int type, struct ed2_loc *loc, int *pbroken) {
	int stype, sdata;
	int idx = symtab_get(isa->symtab, name, &stype, &sdata);
	if (idx == -1) {
		fprintf (stderr, ED2_LOC_FORMAT(*loc, "Undefined symbol %s\n"), name);
		*pbroken |= 1;
		free(name);
		return -1;
	} else if (stype != type) {
		fprintf (stderr, ED2_LOC_FORMAT(*loc, "Symbol %s is not a %s\n"), name, typenames[type]);
		*pbroken |= 1;
		free(name);
		return -1;
	} else {
		free(name);
		return sdata;
	}
}

int ed2ip_put_sym(struct ed2i_isa *isa, char *name, int type, struct ed2_loc *loc, int data) {
	int idx = symtab_put(isa->symtab, name, type, data);
	if (idx == -1) {
		fprintf (stderr, ED2_LOC_FORMAT(*loc, "Redefined symbol %s\n"), name);
		return 1;
	}
	return 0;
}

int ed2ip_transform_features(struct ed2ip_isa *preisa, struct ed2i_isa *isa) {
	int i, j;
	int broken = 0;
	for (i = 0; i < preisa->featuresnum; i++) {
		int f1;
		if ((f1 = vardata_add_feature(isa->vardata, preisa->features[i]->names[0], preisa->features[i]->description)) == -1) {
			fprintf (stderr, ED2_LOC_FORMAT(preisa->features[i]->loc, "Redefined symbol %s\n"), preisa->features[i]->names[0]);
			broken = 1;
		} else {
			/* XXX names */
			for (j = 0; j < preisa->features[i]->impliesnum; j++) {
				int f2 = symtab_get_td(isa->vardata->symtab, preisa->features[i]->implies[j], VARDATA_ST_FEATURE);
				if (f2 == -1) {
					fprintf (stderr, ED2_LOC_FORMAT(preisa->features[i]->loc, "%s is not a feature name\n"), preisa->features[i]->implies[j]);
					broken = 1;
				} else {
					vardata_feature_imply(isa->vardata, f1, f2);
				}
			}
			for (j = 0; j < preisa->features[i]->conflictsnum; j++) {
				int f2 = symtab_get_td(isa->vardata->symtab, preisa->features[i]->conflicts[j], VARDATA_ST_FEATURE);
				if (f2 == -1) {
					fprintf (stderr, ED2_LOC_FORMAT(preisa->features[i]->loc, "%s is not a feature name\n"), preisa->features[i]->conflicts[j]);
					broken = 1;
				} else {
					vardata_feature_conflict(isa->vardata, f1, f2);
				}
			}
		}
		ed2ip_del_feature(preisa->features[i]);
	}
	free(preisa->features);
	return broken;
}

int ed2ip_transform_variants(struct ed2ip_isa *preisa, struct ed2i_isa *isa) {
	int i, j;
	int broken = 0;
	int vs;
	if (preisa->variantsnum) {
		vs = vardata_add_varset(isa->vardata, "defvarset", "Default variant set");
		if (vs == -1)
			abort();
	}
	for (i = 0; i < preisa->variantsnum; i++) {
		int v;
		if ((v = vardata_add_variant(isa->vardata, preisa->variants[i]->names[0], preisa->variants[i]->description, vs)) == -1) {
			fprintf (stderr, ED2_LOC_FORMAT(preisa->variants[i]->loc, "Redefined symbol %s\n"), preisa->variants[i]->names[0]);
			broken = 1;
		} else {
			/* XXX names */
			for (j = 0; j < preisa->variants[i]->featuresnum; j++) {
				int f = symtab_get_td(isa->vardata->symtab, preisa->variants[i]->features[j], VARDATA_ST_FEATURE);
				if (f == -1) {
					fprintf (stderr, ED2_LOC_FORMAT(preisa->variants[i]->loc, "%s is not a feature name\n"), preisa->variants[i]->features[j]);
					broken = 1;
				} else {
					vardata_variant_feature(isa->vardata, v, f);
				}
			}
		}
		ed2ip_del_variant(preisa->variants[i]);
	}
	free(preisa->variants);
	return broken;
}

int ed2ip_transform_modesets(struct ed2ip_isa *preisa, struct ed2i_isa *isa) {
	int i, j, k;
	int broken = 0;
	for (i = 0; i < preisa->modesetsnum; i++) {
		struct ed2ip_modeset *pms = preisa->modesets[i];
		int ms;
		if ((ms = vardata_add_modeset(isa->vardata, pms->names[0], pms->description)) == -1) {
			fprintf (stderr, ED2_LOC_FORMAT(pms->loc, "Redefined symbol %s\n"), pms->names[0]);
			broken = 1;
		} else {
			/* XXX names */
			/* XXX isoptional */
			for (j = 0; j < pms->modesnum; j++) {
				struct ed2ip_mode *pm = pms->modes[j];
				int m;
				if ((m = vardata_add_mode(isa->vardata, pm->names[0], pm->description, ms)) == -1) {
					fprintf (stderr, ED2_LOC_FORMAT(pm->loc, "Redefined symbol %s\n"), pm->names[0]);
					broken = 1;
				} else {
					/* XXX names */
					for (k = 0; k < pm->featuresnum; k++) {
						int f = symtab_get_td(isa->vardata->symtab, pm->features[k], VARDATA_ST_FEATURE);
						if (f == -1) {
							fprintf (stderr, ED2_LOC_FORMAT(pm->loc, "%s is not a feature name\n"), pm->features[k]);
							broken = 1;
						} else {
							vardata_mode_require(isa->vardata, m, f);
						}
					}
					if (pm->isdefault) {
						if (vardata_modeset_def(isa->vardata, ms, m)) {
							fprintf (stderr, "More than one default mode: %s and %s\n", isa->vardata->modes[isa->vardata->modesets[ms].defmode].name, pm->names[0]);
							broken = 1;
						}
					}
				}
			}
		}
		ed2ip_del_modeset(pms);
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
	isa->opdefault = mask_new(isa->opbits);
	isa->opdefmask = mask_new(isa->opbits);
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
	isa->symtab = symtab_new();
	isa->vardata = vardata_new("isavar");
	broken |= ed2ip_transform_features(preisa, isa);
	broken |= ed2ip_transform_variants(preisa, isa);
	broken |= ed2ip_transform_modesets(preisa, isa);
	broken |= vardata_validate(isa->vardata);
	broken |= ed2ip_transform_opfields(preisa, isa);
	int i;
	for (i = 0; i < preisa->seqsnum; i++)
		ed2ip_del_seq(preisa->seqs[i]);
	free(preisa->seqs);
	free(preisa);
	if (broken) {
		ed2i_del_isa(isa);
		return 0;
	} else {
		return isa;
	}
}
