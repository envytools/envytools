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

	uint32_t m, d, ctrl;
	if (optind + 3 > argc) {
		fprintf (stderr, "%s: <channel> <method> <data>\n", argv[0]);
		return 1;
	}
	sscanf (argv[optind + 0], "%d", &c);
	sscanf (argv[optind + 1], "%x", &m);
	sscanf (argv[optind + 2], "%x", &d);

	if (nva_cards[cnum].chipset >= 0xd0) {
		ctrl = nva_rd32(cnum, 0x610700 + (c * 8));
		nva_wr32(cnum, 0x610700 + (c * 8), ctrl | 1);
		nva_wr32(cnum, 0x610704 + (c * 8), d);
		nva_wr32(cnum, 0x610700 + (c * 8), 0x80000001 | m);
		while (nva_rd32(cnum, 0x610700 + (c * 8)) & 0x80000000);
		nva_wr32(cnum, 0x610700 + (c * 8), ctrl);
	} else
	if (nva_cards[cnum].chipset == 0x50 ||
	    nva_cards[cnum].chipset >= 0x84) {
		ctrl = nva_rd32(cnum, 0x610300 + (c * 8));
		nva_wr32(cnum, 0x610300 + (c * 8), ctrl | 1);
		nva_wr32(cnum, 0x610304 + (c * 8), d);
		nva_wr32(cnum, 0x610300 + (c * 8), 0x80000001 | m);
		while (nva_rd32(cnum, 0x610300 + (c * 8)) & 0x80000000);
		nva_wr32(cnum, 0x610300 + (c * 8), ctrl);
	} else {
		fprintf (stderr, "unsupported chipset\n");
		return 1;
	}

	return 0;
}
