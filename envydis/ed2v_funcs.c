#include "ed2v.h"
#include "util.h"
#include <assert.h>
#include <stdlib.h>

struct ed2v_variant *ed2v_new_variant_i(struct ed2i_isa *isa, int variant) {
	assert(variant >= -1 && variant < isa->variantsnum);
	struct ed2v_variant *res = calloc(sizeof *res + sizeof *res->fmask * CEILDIV(isa->featuresnum, ED2V_CHUNK_SIZE), 1);
	res->isa = isa;
	if (variant != -1) {
		int i;
		for (i = 0; i < isa->variants[variant].featuresnum; i++) {
			if (ed2v_add_feature_i(res, isa->variants[variant].features[i])) {
				fprintf (stderr, "Broken ISA description: variant %s has conflicting features!\n", isa->variants[variant].names[0]);
				free(res);
				return 0;
			}
		}
	}
	res->mode = isa->defmode;
	return res;
}

struct ed2v_variant *ed2v_new_variant(struct ed2i_isa *isa, const char *variant) {
	if (!variant)
		return ed2v_new_variant_i(isa, -1);
	int idx = ed2s_symtab_get_int(isa->symtab, variant, ED2I_ST_VARIANT);
	if (idx == -1) {
		fprintf (stderr, "%s is not a variant name\n", variant);
		return 0;
	}
	return ed2v_new_variant_i(isa, idx);
}

void ed2v_del_variant(struct ed2v_variant *var) {
	free(var);
}

int ed2v_add_feature_i(struct ed2v_variant *var, int feature) {
	int i;
	struct ed2i_feature *f = &var->isa->features[feature];
	assert(feature >= 0 && feature < var->isa->featuresnum);
	if (ed2v_has_feature(var, feature))
		return 0;
	for (i = 0; i < f->conflictsnum; i++) {
		if (ed2v_has_feature(var, f->conflicts[i])) {
			fprintf (stderr, "Conflicting ISA features used: %s and %s\n", f->names[0], var->isa->features[f->conflicts[i]].names[0]);
			return -1;
		}
	}
	var->fmask[feature/32] |= 1 << (feature % 32);
	for (i = 0; i < f->impliesnum; i++) {
		if (ed2v_add_feature_i(var, f->implies[i]))
			return -1;
	}
	return 0;
}

int ed2v_add_feature(struct ed2v_variant *var, const char *feature) {
	int idx = ed2s_symtab_get_int(var->isa->symtab, feature, ED2I_ST_FEATURE);
	if (idx == -1) {
		fprintf (stderr, "%s is not a feature name\n", feature);
		return -1;
	}
	return ed2v_add_feature_i(var, idx);
}

int ed2v_set_mode_i(struct ed2v_variant *var, int mode) {
	int i;
	struct ed2i_mode *m = &var->isa->modes[mode];
	assert(mode >= 0 && mode < var->isa->modesnum);
	var->mode = mode;
	for (i = 0; i < m->featuresnum; i++) {
		if (ed2v_add_feature_i(var, m->features[i]))
			return -1;
	}
	return 0;
}

int ed2v_set_mode(struct ed2v_variant *var, const char *mode) {
	int idx = ed2s_symtab_get_int(var->isa->symtab, mode, ED2I_ST_MODE);
	if (idx == -1) {
		fprintf (stderr, "%s is not a mode name\n", mode);
		return -1;
	}
	return ed2v_set_mode_i(var, idx);
}
