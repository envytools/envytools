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
	int i;
	for (i = 0; i < 8; i++) {
		nva_wr32(cnum, 0xa7c0 + i * 4, 1);
		nva_wr32(cnum, 0xa420 + i * 4, 0xffff);
	}
		sleep(1);
	for (i = 0; i < 8; i++)
		nva_wr32(cnum, 0xa420 + i * 4, 0xffff);
	for (i = 0; i < 8; i++)
		printf ("Set %d: %dHz\n", i, nva_rd32(cnum, 0xa600 + i * 4));
	return 0;
}
