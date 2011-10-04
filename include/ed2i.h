#ifndef ED2I_ISA_H
#define ED2I_ISA_H

#include "ed2s.h"
#include <stdio.h>

enum {
	ED2I_ST_FEATURE,
	ED2I_ST_VARIANT,
};

struct ed2i_feature {
	char **names;
	int namesnum;
	char *description;
	int *implies;
	int impliesnum;
	int *conflicts;
	int conflictsnum;
};

struct ed2i_variant {
	char **names;
	int namesnum;
	char *description;
	int *features;
	int featuresnum;
};

struct ed2i_isa {
	struct ed2i_feature *features;
	int featuresnum;
	struct ed2i_variant *variants;
	int variantsnum;
	struct ed2s_symtab *symtab;
};

void ed2i_del_isa(struct ed2i_isa *isa);
struct ed2i_isa *ed2i_read_file (FILE *file, const char *filename);

#endif
