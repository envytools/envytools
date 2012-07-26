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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "nva.h"
#include <stdio.h>
#include <unistd.h>

void dac_write(int cnum, uint16_t addr, uint8_t val) {
	nva_wr32(cnum, 0x609010, addr & 0xff);
	nva_wr32(cnum, 0x609014, addr >> 8);
	nva_wr32(cnum, 0x609018, val);
}

uint8_t dac_read(int cnum, uint16_t addr) {
	nva_wr32(cnum, 0x609010, addr & 0xff);
	nva_wr32(cnum, 0x609014, addr >> 8);
	return nva_rd32(cnum, 0x609018);
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
	/* ??? */
	dac_write(cnum, 5, 0xff);
	/* PMC.ENABLE */
	nva_wr32(cnum, 0x200, 0);
	nva_wr32(cnum, 0x200, 0x01011111);
	/* PFB scanout config */
	nva_wr32(cnum, 0x600200, 0x130);
	nva_wr32(cnum, 0x600400, 0x0);
	nva_wr32(cnum, 0x600500, 0x18);
	nva_wr32(cnum, 0x600510, 0x88);
	nva_wr32(cnum, 0x600520, 0xa0);
	nva_wr32(cnum, 0x600530, 0x400);
	nva_wr32(cnum, 0x600540, 0x3);
	nva_wr32(cnum, 0x600550, 0x6);
	nva_wr32(cnum, 0x600560, 0x1d);
	nva_wr32(cnum, 0x600570, 0x300);
	/* DAC scanout config */
	dac_write(cnum, 4, 0x39);
	dac_write(cnum, 5, 0x08);
	/* VPLL */
	dac_write(cnum, 0x18, 0x0a);
	dac_write(cnum, 0x19, 0x35);
	dac_write(cnum, 0x1a, 0x01);
	dac_write(cnum, 0x1b, 0x01);
	/* ??? */
	dac_write(cnum, 0x1c, 0x00);
	/* start it up */
	nva_wr32(cnum, 0x6000c0, 0x00330000);
	/* ??? */
	nva_wr32(cnum, 0x001400, 0x1000);
	nva_wr32(cnum, 0x001200, 0x1010);
	nva_wr32(cnum, 0x400084, 0x01110100);

	/* memory detection */
	nva_wr32(cnum, 0x600000, 0x2);
	nva_wr32(cnum, 0x1000000, 0xabcd0000);
	nva_wr32(cnum, 0x1000004, 0xabcd0001);
	nva_wr32(cnum, 0x100000c, 0xabcd0002);
	nva_wr32(cnum, 0x1000010, 0xabcd0010);
	nva_wr32(cnum, 0x1000014, 0xabcd0011);
	nva_wr32(cnum, 0x100001c, 0xabcd0012);
	if (nva_rd32(cnum, 0x100000c) == 0xabcd0002) {
		printf("POSTing 4MB card\n");
		nva_wr32(cnum, 0x600000, 0x10000202);
		nva_wr32(cnum, 0x600040, 0x00900011);
		nva_wr32(cnum, 0x600044, 0x00000003);
		nva_wr32(cnum, 0x600080, 0x00010000);
		dac_write(cnum, 4, 0x39);
	} else if (nva_rd32(cnum, 0x1000004) == 0xabcd0001) {
		printf("POSTing 2MB card\n");
		nva_wr32(cnum, 0x600000, 0x10001201);
		nva_wr32(cnum, 0x600040, 0x00400011);
		nva_wr32(cnum, 0x600044, 0x00000002);
		nva_wr32(cnum, 0x600080, 0x00010000);
		dac_write(cnum, 4, 0x39);
	} else if (nva_rd32(cnum, 0x1000000) == 0xabcd0000) {
		printf("POSTing 1MB card\n");
		nva_wr32(cnum, 0x600000, 0x10001100);
		nva_wr32(cnum, 0x600040, 0x00400011);
		nva_wr32(cnum, 0x600044, 0x00000002);
		nva_wr32(cnum, 0x600080, 0x00010000);
		dac_write(cnum, 4, 0x35);
	} else {
		printf("POST failure - memory didn't come up!\n");
		return 1;
	}
	/* MPLL */
	dac_write(cnum, 0x10, 0x0c);
	dac_write(cnum, 0x11, 0x60);
	dac_write(cnum, 0x12, 0x01);
	dac_write(cnum, 0x13, 0x01);
	/* AUDIO */
	nva_wr32(cnum, 0x3000c0, 0x111);
	/* ??? */
	nva_wr32(cnum, 0x6c1f20, 0x332);
	nva_wr32(cnum, 0x6c1f24, 0x3330333);
	nva_wr32(cnum, 0x6c1f00, 1);

	/* palette */
	nva_wr32(cnum, 0x609000, 0);
	int i;
	for (i = 0; i < 256; i++) {
		nva_wr32(cnum, 0x609004, ((i >> 5) & 7) * 255/7);
		nva_wr32(cnum, 0x609004, ((i >> 2) & 7) * 255/7);
		nva_wr32(cnum, 0x609004, ((i >> 0) & 3) * 255/3);
	}

	/* framebuffer*/
	for (i = 0; i < 0x300 * 0x400; i++) {
		int x = i & 0x3ff;
		int y = i >> 10;
		int col = 0;
		if (x+y <= 32)
			col = 3;
		if (x-y >= 0x400-32)
			col = 0x1c;
		if (x-y <= -0x300+32)
			col = 0xe0;
		if (x+y >= 0x700-32)
			col = 0xff;
		nva_wr32(cnum, 0x1000000 + i, col);
	}
	return 0;
}
