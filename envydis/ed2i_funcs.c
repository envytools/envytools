#include "ed2i.h"
#include <stdlib.h>

void ed2i_del_isa(struct ed2i_isa *isa) {
	ed2s_symtab_del(isa->symtab);
	int i;
	for (i = 0; i < isa->featuresnum; i++) {
		ed2_free_strings(isa->features[i].names, isa->features[i].namesnum);
		free(isa->features[i].description);
		free(isa->features[i].implies);
		free(isa->features[i].conflicts);
	}
	free(isa->features);
	for (i = 0; i < isa->variantsnum; i++) {
		ed2_free_strings(isa->variants[i].names, isa->variants[i].namesnum);
		free(isa->variants[i].description);
		free(isa->variants[i].features);
	}
	free(isa->variants);
	for (i = 0; i < isa->modesnum; i++) {
		ed2_free_strings(isa->modes[i].names, isa->modes[i].namesnum);
		free(isa->modes[i].description);
		free(isa->modes[i].features);
	}
	free(isa->modes);
	free(isa);
}
