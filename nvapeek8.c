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
	int32_t a, b = 1, i;
	if (optind >= argc) {
		fprintf (stderr, "No address specified.\n");
		return 1;
	}
	sscanf (argv[optind], "%x", &a);
	if (optind + 1 < argc)
		sscanf (argv[optind + 1], "%x", &b);
	int ls = 1;
	while (b > 0) {
		uint8_t z[16];
		int s = 0;
		for (i = 0; i < 16 && i < b; i++)
			if (z[i] = nva_rd8(cnum, a+i)) s = 1;
		if (s) {
			ls = 1;
			printf ("%08x:", a);
			for (i = 0; i < 16 && i < b; i++) {
				printf (" %02x", z[i]);
			}
			printf ("\n");
		} else  {
			if (ls) printf ("...\n"), ls = 0;
		}
		a+=16;
		b-=16;
	}
	return 0;
}
