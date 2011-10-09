#ifndef ED2V_H
#define ED2V_H

#include "ed2i.h"

struct ed2v_variant {
	struct ed2i_isa *isa;
	int mode;
	uint32_t fmask[0];
};

#define ED2V_CHUNK_SIZE 32

static inline int ed2v_has_feature(struct ed2v_variant *var, int feature) {
	return (var->fmask[feature/32] >> (feature % 32)) & 1;
};

struct ed2v_variant *ed2v_new_variant_i(struct ed2i_isa *isa, int variant);
struct ed2v_variant *ed2v_new_variant(struct ed2i_isa *isa, const char *variant);
void ed2v_del_variant(struct ed2v_variant *var);
int ed2v_add_feature_i(struct ed2v_variant *var, int feature);
int ed2v_add_feature(struct ed2v_variant *var, const char *feature);
int ed2v_set_mode_i(struct ed2v_variant *var, int mode);
int ed2v_set_mode(struct ed2v_variant *var, const char *mode);

#endif
