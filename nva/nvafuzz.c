#include "nva.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

uint32_t proper_rand(int x) {
	uint32_t res = 0;
	while (x--) {
		res <<= 1;
		res |= rand() & 1;
	}
	return res;
}

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
	int cnum =0;
	int alias = 0;
	while ((c = getopt (argc, argv, "ac:")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
			case 'a':
				alias = 1;
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
	srand(time(0) + nva_rd32(cnum, 0x9400));
	while (1) {
		uint32_t reg = (a + proper_rand(24) % b) & 0xfffffc;
		uint32_t val = proper_rand(32);
		nva_wr32(cnum, reg, val);
	}
	return 0;
}
