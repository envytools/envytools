#include "nva.h"
#include <stdio.h>
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
	uint32_t a, b;
	if (optind >= argc) {
		fprintf (stderr, "No address specified.\n");
		return 1;
	}
	if (optind + 1 >= argc) {
		fprintf (stderr, "No value specified.\n");
		return 1;
	}
	sscanf (argv[optind], "%x", &a);
	sscanf (argv[optind + 1], "%x", &b);
	nva_wr8(cnum, a, b);
	return 0;
}
