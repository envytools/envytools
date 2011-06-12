#include "nva.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
	int cnum =0;
	while ((c = getopt (argc, argv, "c:")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
		}
	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}
	int32_t a, b = 4, i;
	if (optind >= argc) {
		fprintf (stderr, "No address specified.\n");
		return 1;
	}
	sscanf (argv[optind], "%x", &a);
	if (optind + 1 < argc)
		sscanf (argv[optind + 1], "%x", &b);
	int ls = 1;
	for (i = 0; i < b; i+=4) {
		uint32_t x, y, z;
		x = nva_rd32(cnum, a+i);
		nva_wr32(cnum, a+i, -1);
		y = nva_rd32(cnum, a+i);
		nva_wr32(cnum, a+i, 0);
		z = nva_rd32(cnum, a+i);
		nva_wr32(cnum, a+i, x);
		if (x || y || z) {
			printf ("%06x: %08x %08x %08x\n", a+i, x, y, z);
			ls = 1;
		} else {
			if (ls)
				printf("...\n");
			ls = 0;
		}
	}
	return 0;
}
