#ifndef ED2I_PRE_H
#define ED2I_PRE_H

#include "ed2i.h"
#include "ed2_misc.h"

struct ed2ip_feature {
	char **names;
	int namesnum;
	int namesmax;
	char *description;
	char **implies;
	int impliesnum;
	int impliesmax;
	char **conflicts;
	int conflictsnum;
	int conflictsmax;
	struct ed2_loc loc;
};

struct ed2ip_variant {
	char **names;
	int namesnum;
	int namesmax;
	char *description;
	char **features;
	int featuresnum;
	int featuresmax;
	struct ed2_loc loc;
};

struct ed2ip_mode {
	char **names;
	int namesnum;
	int namesmax;
	char *description;
	char **features;
	int featuresnum;
	int featuresmax;
	int isdefault;
	struct ed2_loc loc;
};

struct ed2ip_isa {
	struct ed2ip_feature **features;
	int featuresnum;
	int featuresmax;
	struct ed2ip_variant **variants;
	int variantsnum;
	int variantsmax;
	struct ed2ip_mode **modes;
	int modesnum;
	int modesmax;
	int broken;
};

void ed2ip_del_feature(struct ed2ip_feature *f);
void ed2ip_del_variant(struct ed2ip_variant *v);
void ed2ip_del_mode(struct ed2ip_mode *m);
void ed2ip_del_isa(struct ed2ip_isa *isa);

struct ed2i_isa *ed2ip_transform(struct ed2ip_isa *isa);

#endif
