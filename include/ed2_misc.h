#ifndef ED2_MISC_H
#define ED2_MISC_H

#include "util.h"
#include <stdlib.h>
#include <inttypes.h>

struct ed2_loc {
	int lstart;
	int cstart;
	int lend;
	int cend;
	const char *file;
};

#define ED2_LOC_FORMAT "%s:%d.%d-%d.%d: "
#define ED2_LOC_PARAMS(loc) (loc).file, (loc).lstart, (loc).cstart, (loc).lend, (loc).cend

char *ed2_str_deescape(char *str, uint64_t *len);

struct ed2_astr {
	char *str;
	uint64_t len;
};

#endif
