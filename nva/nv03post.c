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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

uint8_t bios[0x10000];

static uint16_t bios16(int off) {
	return bios[off] | bios[off+1] << 8;
}

static uint32_t bios32(int off) {
	return bios[off] |
		bios[off + 1] << 8 |
		bios[off + 2] << 16 |
		bios[off + 3] << 24;
}

void nv3_compute_mem(int cnum) {
	uint32_t val = nva_rd32(cnum, 0x100000);
	val &= ~3;
	val |= 2;
	nva_wr32(cnum, 0x100000, val);
	if (!(val & 4)) {
		nva_gwr32(nva_cards[cnum]->bar1, 0x100000, 0x11111111);
		nva_gwr32(nva_cards[cnum]->bar1, 0x300000, 0x22222222);
		if (nva_grd32(nva_cards[cnum]->bar1, 0x300000) != 0x22222222) {
			val--;
			if (nva_grd32(nva_cards[cnum]->bar1, 0x100000) != 0x11111111)
				val--;
		}
	} else {
		nva_gwr32(nva_cards[cnum]->bar1, 0x300008, 0x33333333);
		if (nva_grd32(nva_cards[cnum]->bar1, 0x300008) != 0x33333333)
			val--;
	}
	nva_wr32(cnum, 0x100000, val);
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

	if (nva_cards[cnum]->chipset.chipset != 3) {
		fprintf(stderr, "Not an NV3!\n");
		return 1;
	}

	nva_wr32(cnum, 0x200, 0x01100100);

	int i;
	for (i = 0; i < 0x10000; i++) {
		bios[i] = nva_rd8(cnum, 0x110000 + i);
	}

	if (memcmp(bios, "\x55\xaa", 2)) {
		fprintf(stderr, "No BIOS!\n");
		return 1;
	}
	int bmp_off = 0;
	for (i = 0; i < 0x10000; i++) {
		if (!memcmp(bios + i, "\xff\x7fNV\x00", sizeof 5)) {
			bmp_off = i;
			break;
		}
	}
	if (!bmp_off) {
		fprintf(stderr, "No BMP!\n");
		return 1;
	}
	uint16_t pc = bios16(bmp_off + 0xc);
	uint32_t straps = nva_rd32(cnum, 0x101000);
	printf("%08x\n", straps);
	bool run = true;
	while (1) {
		uint8_t op = bios[pc];
		uint32_t reg, idx;
		uint32_t val, nval;
		uint32_t v1, v2;
		printf("%04x%c %c", pc, run ? '*' : ' ', op);
		pc++;
		switch (op) {
			// XXX: this is *not* cross-checked with BIOS, it just happens to work
			case 0x6d:
				val = bios[pc++];
				if (run)
					run = (nva_rd32(cnum, 0x100000) & 3) == val;
				printf(" %02x%c\n", val, run ? '+' : '-');
				break;
			case 0x71:
				printf("\n");
				return 0;
			case 0x72:
				run = true;
				printf("\n");
				break;
			case 0x73:
				v1 = bios32(pc);
				v2 = bios32(pc+4);
				pc += 8;
				if (run)
					run = (straps & v1) == v2;
				printf(" %08x %08x %c\n", v1, v2, run ? '+' : '-');
				break;
			case 0x63:
				nv3_compute_mem(cnum);
				printf(" -> %dMB\n", 1 << (nva_rd32(cnum, 0x100000) & 3));
				break;
			case 0x77:
				reg = bios32(pc);
				val = bios16(pc+4);
				printf(" %08x %08x\n", reg, val);
				if (run)
					nva_wr32(cnum, reg, val);
				pc += 6;
				break;
			case 0x78:
				reg = bios16(pc);
				pc += 2;
				idx = bios[pc++];
				v1 = bios[pc++];
				v2 = bios[pc++];
				nva_wr8(cnum, 0x601000 + reg, idx);
				val = nva_rd8(cnum, 0x601000 + reg + 1);
				nval = (val & v1) | v2;
				if (run)
					nva_wr8(cnum, 0x601000 + reg + 1, nval);
				printf(" %04x %02x %02x %02x [%02x -> %02x]\n", reg, idx, v1, v2, val, nval);
				break;
			case 0x79:
				reg = bios32(pc);
				v1 = bios32(pc+4);
				v2 = bios32(pc+8);
				printf(" %08x %08x %08x\n", reg, v1, v2);
				if (run)
					nva_wr32(cnum, reg, straps & 0x40 ? v2 : v1);
				pc += 12;
				break;
			case 0x7a:
				reg = bios32(pc);
				val = bios32(pc+4);
				printf(" %08x %08x\n", reg, val);
				if (run)
					nva_wr32(cnum, reg, val);
				pc += 8;
				break;
			default:
				printf(" unknown op\n");
				return 1;
		}
	}
	return 0;
}
