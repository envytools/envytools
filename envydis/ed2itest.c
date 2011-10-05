#include "ed2i.h"
#include <stdio.h>

int main(int argc, char **argv) {
	struct ed2i_isa *isa = ed2i_read_isa(argv[1]);
	return 0;
}
