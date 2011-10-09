#include "ed2i.h"
#include <stdio.h>

int main(int argc, char **argv) {
	struct ed2i_isa *isa = ed2i_read_isa(argv[1]);
	if (isa)
		ed2i_del_isa(isa);
	return 0;
}
