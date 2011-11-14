#include "nva.h"
#include <stdio.h>
#include <unistd.h>

uint32_t xt_insn(int cnum, uint32_t insn, uint32_t parm, uint32_t *status) {
	nva_wr32(cnum, 0x700004, insn);
	nva_wr32(cnum, 0x700008, parm);
	nva_wr32(cnum, 0x70000, 1);
	while (nva_rd32(cnum, 0x70000));
	nva_wr32(cnum, 0x700000, 5);
	while (nva_rd32(cnum, 0x700000));
	*status = nva_rd32(cnum, 0x70000c);
	return nva_rd32(cnum, 0x700008);
}

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
	int32_t a, b = 0, i;
	if (optind >= argc) {
		fprintf (stderr, "No insn specified.\n");
		return 1;
	}
	sscanf (argv[optind], "%x", &a);
	if (optind + 1 < argc)
		sscanf (argv[optind + 1], "%x", &b);
	nva_wr32(cnum, 0x1700, 0x30);
	uint32_t status;
	uint32_t val = xt_insn(cnum, a, b, &status);
	if (status) {
		printf("EXC%02x\n", status & 0x3f);
	} else {
		printf("%08x\n", val);
	}
	return 0;
}
