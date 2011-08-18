#include "nva.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#define FLAGS 0x4000

int cnum =0;

int slen[16];
int seekstart[16][0x400];
int seeksize[16][0x400];
int seekcnt[16];

void strwait() {
	int ctr = 0;
	while (nva_rd32(cnum, 0x400404) & 0x20000 || nva_rd32(cnum, 0x400400) || nva_rd32(cnum, 0x400700) & 0x100 || nva_rd32(cnum, 0x40082c & 0x800)) {
		ctr++;
		if (ctr > 0x1000000) {
			printf("Aiiii, xfer is hung. bailing.\n");
			exit(1);
		}
	}
	while (nva_rd32(cnum, 0x400404) & 0x20000 || nva_rd32(cnum, 0x400400) || nva_rd32(cnum, 0x400700) & 0x100 || nva_rd32(cnum, 0x40082c & 0x800));
}

void strread(int s, int f) {
	nva_wr32(cnum, 0x400824, FLAGS | f | 1);
	nva_wr32(cnum, 0x400304, 1);
	strwait();
}

void strwrite(int s, int f) {
	nva_wr32(cnum, 0x400824, FLAGS | f);
	nva_wr32(cnum, 0x400304, 1);
	strwait();
}

void nv50_graph_reset() {
	nva_wr32(cnum, 0x200, 0xffffefff);
	nva_wr32(cnum, 0x200, 0xffffffff);
	nva_wr32(cnum, 0x400040, -1);
	nva_wr32(cnum, 0x400040, 0);
	nva_wr32(cnum, 0x400080, 0x3083c2);
	nva_wr32(cnum, 0x400088, 0x6fe7);
}

void kill(int j) {
	nv50_graph_reset();
	if (nva_rd32(cnum, 0x40082c) & 0x800) {
		printf("Aiiii, xfer is hung. bailing.\n");
		exit(1);
	}
	nva_wr32(cnum, 0x400824, FLAGS|1);
	nva_wr32(cnum, 0x400828, 1);
	nva_wr32(cnum, 0x40032c, 0x4000);
	nva_wr32(cnum, 0x400784, 0x4000);
	nva_wr32(cnum, 0x400320, 4);

	nva_wr32(cnum, 0x400324, 0);
	nva_wr32(cnum, 0x400328, 0x200000);
	nva_wr32(cnum, 0x400328, 0x600007);
	nva_wr32(cnum, 0x400328, 0xc00000 | (j&8)<<16 | 1<<(j&7));
	nva_wr32(cnum, 0x400328, 0x203000);
	nva_wr32(cnum, 0x400328, 0x800000 | (j&8)<<16 | 1<<(j&7));
	nva_wr32(cnum, 0x400328, 0x60000c);
}

int main(int argc, char **argv){
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
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
	nv50_graph_reset();
	nva_wr32(cnum, 0x1700, 0x400);
	nva_wr32(cnum, 0x700020, 0x190000);
	nva_wr32(cnum, 0x700024, 0x4000000 + 0x80000);
	nva_wr32(cnum, 0x700028, 0x4000000 + 0x10000);
	nva_wr32(cnum, 0x70002c, 0);
	nva_wr32(cnum, 0x700030, 0);
	nva_wr32(cnum, 0x700034, 0x10000);
	nva_wr32(cnum, 0x700200, 0x190000);
	nva_wr32(cnum, 0x700204, 0x4000000 + 0x80000);
	nva_wr32(cnum, 0x700208, 0x4000000 + 0x10000);
	nva_wr32(cnum, 0x70020c, 0);
	nva_wr32(cnum, 0x700210, 0);
	nva_wr32(cnum, 0x700214, 0x10000);

	int j;
	int k;

	for (j = 0; j < 16; j++) {
		printf ("Strand %d!\n", j);
		uint32_t tab[0x3000][6];
		kill(j);
		for (k = 0; k < 0x70000; k += 4)
			nva_wr32(cnum, 0x710000 + k, 0xdeafbeef);
		strread(j, 0);
		for (k = 4*(j&7); k < 0x3000*32; k += 4*8) {
			tab[k/32][0] = nva_rd32(cnum, 0x710000 + k);
		}

		for (k = 0; k < 0x70000; k += 4)
			nva_wr32(cnum, 0x710000 + k, 0);
		strwrite(j, 0);
		for (k = 0; k < 0x70000; k += 4)
			nva_wr32(cnum, 0x710000 + k, 0);
		strread(j, 0);
		for (k = 4*(j&7); k < 0x3000*32; k += 4*8) {
			tab[k/32][1] = nva_rd32(cnum, 0x710000 + k);
		}

		for (k = 0; k < 0x70000; k += 4)
			nva_wr32(cnum, 0x710000 + k, -1);
		strwrite(j, 0);
		for (k = 0; k < 0x70000; k += 4)
			nva_wr32(cnum, 0x710000 + k, 0);
		strread(j, 0);
		for (k = 4*(j&7); k < 0x3000*32; k += 4*8) {
			tab[k/32][2] = nva_rd32(cnum, 0x710000 + k);
		}

		for (k = 0; k < 0x70000; k += 4)
			nva_wr32(cnum, 0x710000 + k, 0);
		strwrite(j, 8);
		for (k = 0; k < 0x70000; k += 4)
			nva_wr32(cnum, 0x710000 + k, 0);
		strread(j, 8);
		for (k = 4*(j&7); k < 0x3000*32; k += 4*8) {
			tab[k/32][3] = nva_rd32(cnum, 0x710000 + k);
		}

		for (k = 0; k < 0x70000; k += 4)
			nva_wr32(cnum, 0x710000 + k, -1);
		strwrite(j, 8);
		for (k = 0; k < 0x70000; k += 4)
			nva_wr32(cnum, 0x710000 + k, 0);
		strread(j, 8);
		for (k = 4*(j&7); k < 0x3000*32; k += 4*8) {
			tab[k/32][4] = nva_rd32(cnum, 0x710000 + k);
			tab[k/32][5] = 0;
		}

		int z;
		for (z = 0; z < 32; z++) {
			for (k = 0; k < 0x70000; k += 4)
				nva_wr32(cnum, 0x710000 + k, -1);
			strwrite(j, 8);
			nva_wr32(cnum, 0x400040, 1 << z);
			nva_wr32(cnum, 0x400040, 0);
			strwait();
			for (k = 0; k < 0x70000; k += 4)
				nva_wr32(cnum, 0x710000 + k, 0);
			strread(j, 8);
			for (k = 4*(j&7); k < 0x3000*32; k += 4*8) {
				if (tab[k/32][4] != nva_rd32(cnum, 0x710000 + k))
					tab[k/32][5] |= 1 << z;
			}
		}

		for (k = 0; k < 0x3000; k++)
			if (tab[k][0] == 0xdeafbeef && !tab[k][4]) {
				slen[j] = k;
				break;
			}

		if (slen[j]) {
			printf ("Initial length: %04x\n", slen[j]);
			while (slen[j]) {
				for (k = 0; k < 0x70000; k += 4)
					nva_wr32(cnum, 0x710000 + k, 0);
				nva_wr32(cnum, 0x710000 + (j&7)*4 + (slen[j] - 1) * 32, 0xffffffff);
				strwrite(j, 8);
				for (k = 0; k < 0x70000; k += 4)
					nva_wr32(cnum, 0x710000 + k, 0);
				strread(j, 8);
				if (nva_rd32(cnum, 0x710000 + (j&7)*4 + (slen[j] - 1) * 32)) {
					break;
				} else {
					slen[j]--;
				}
			}
		}
		printf ("Length: %04x\n", slen[j]);

		int i;
		for (i = 0; i < 0x400; i++) {
			int pos = -1;
			int l;
			int ctr = 0;
			int try;
			for (l = 1; l < 0x14; l++) {
//				try = 0;
//respin:
				for (k = 0; k < 0x70000; k += 4)
					nva_wr32(cnum, 0x710000 + k, 0);
				strwrite(j, 8);
				for (k = 0; k < l; k++)
					nva_wr32(cnum, 0x400420 + k * 4, 0xffffffff);
				nva_wr32(cnum, 0x400408, i << 16);
				nva_wr32(cnum, 0x400404, 0x30000 | (j & 7) << 8 | (j&8) << (12-3) | l);
				strwait();
				strread(j, 8);
				ctr = 0;
				pos = -1;
				for (k = 0; k < 0x70000; k += 4)
					if (nva_rd32(cnum, 0x710000 + k) && (k & 0x1c) == (j & 7) << 2) {
						if (pos == -1)
							pos = k / 32;
						ctr++;
//						printf ("%04x: %08x\n", k/32, nva_rd32(cnum, 0x710000 + k));
					}
				if (ctr <= l && ctr)
					break;
//				else
//					if (try++ < 4)
//						goto respin;
			}
			if (pos == -1) {
				seekcnt[j] = i;
				break;
			}
			assert(ctr == l);
			printf ("SEEK: %04x [%d/%d]...\n", pos, ctr, l);
			seekstart[j][i] = pos;
			seeksize[j][i] = l;
		}

		i = 0;
		for (k = 0; k < slen[j]; k++) {
			if (i < seekcnt[j] && k == seekstart[j][i]) {
				printf ("\nSEEK %03x [unit %x]\n", i, seeksize[j][i]);
				i++;
			} else if (i && seeksize[j][i-1] != 1 && !((k - seekstart[j][i-1])%seeksize[j][i-1]))
				printf ("\n");
			printf ("%04x: %08x %08x %08x %08x %08x %08x\n", k, tab[k][0], tab[k][1], tab[k][2], tab[k][3], tab[k][4], tab[k][5]);
		}
	}


	return 0;
}
