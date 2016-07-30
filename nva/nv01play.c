/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

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
	nva_wr32(cnum, 0x200, 0xffffff00);
	nva_wr32(cnum, 0x200, 0xffffffff);
	nva_wr32(cnum, 0x100610, 1);
	nva_wr32(cnum, 0x3000c0, 0);
	nva_wr32(cnum, 0x3000c0, 1);
	while (nva_rd32(cnum, 0x300500) & 0x80);
	nva_wr32(cnum, 0x300500, 0x40);
	nva_wr32(cnum, 0x300510, 0xc0);
	nva_wr32(cnum, 0x300500, 0x46);
	nva_wr32(cnum, 0x300510, 0);
	nva_wr32(cnum, 0x300500, 0x47);
	nva_wr32(cnum, 0x300510, 0);
	nva_wr32(cnum, 0x300500, 0x48);
	nva_wr32(cnum, 0x300510, 0x5a);
	usleep(10000);
	nva_wr32(cnum, 0x300500, 0x49);
	nva_wr32(cnum, 0x300510, 0xc3);
	nva_wr32(cnum, 0x300500, 0);
	nva_wr32(cnum, 0x300200, 0x20);
	nva_wr32(cnum, 0x300400, 0x10);
	nva_wr32(cnum, 0x300980, 0);
	nva_wr32(cnum, 0x300800, 0);
	nva_wr32(cnum, 0x300804, 0);
	nva_wr32(cnum, 0x300808, 0);
	//
	FILE *f = fopen(argv[1], "rb");
	nva_wr32(cnum, 0x710000, 0x00000000);
	nva_wr32(cnum, 0x710004, 0x00000000);
	nva_wr32(cnum, 0x710008, 0x00000000);
	nva_wr32(cnum, 0x71000c, 0x00000000);
	// VOLUME_A
	nva_wr32(cnum, 0x710010, 0x10001000);
	// TEMPO
	nva_wr32(cnum, 0x710014, 0x00018000);
	// VOLUME_B
	nva_wr32(cnum, 0x710018, 0x10001000);
	// CONFIG
	nva_wr32(cnum, 0x71001c, 0x8c000000);
	nva_wr32(cnum, 0x710020, 0x00000000);
	// BUF[0]
	nva_wr32(cnum, 0x710024, 0x00000000);
	nva_wr32(cnum, 0x710028, 0x00060000);
	nva_wr32(cnum, 0x71002c, 0x00070000);
	// BUF[1]
	nva_wr32(cnum, 0x710030, 0x00000000);
	nva_wr32(cnum, 0x710034, 0x00070000);
	nva_wr32(cnum, 0x710038, 0x00080000);
	nva_wr32(cnum, 0x300804, 0x1000);
	nva_wr32(cnum, 0x3000c0, 0x111);
	int nbuf = 1;
	while (1) {
		while ((nva_rd32(cnum, 0x710000) >> (nbuf + 24) & 1) != (nva_rd32(cnum, 0x71001c) >> (nbuf + 24) & 1));
		uint8_t buf[0x20000] = {0};
		int num = fread(buf, 1, 0x20000, f);
		int i;
		for (i = 0; i < 0x20000; i++)
			nva_wr8(cnum, 0x10c0000 + i + nbuf * 0x20000, buf[i]);
		nva_wr32(cnum, 0x71001c, nva_rd32(cnum, 0x71001c) ^ 1 << (nbuf + 24));
		printf("a %d %08x %08x\n", num, nva_rd32(cnum, 0x710000), nva_rd32(cnum, 0x71001c));
		nbuf = !nbuf;
		if (num < 0x20000)
			break;
	}
	return 0;
}
