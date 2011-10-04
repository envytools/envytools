#include "ed2i.h"
#include <stdio.h>

int main(int argc, char **argv) {
	FILE *f = fopen(argv[1], "r");
	if (!f) {
		perror("fopen");
		return 1;
	}
	struct ed2i_isa *isa = ed2i_read_file(f, argv[1]);
	return 0;
}
