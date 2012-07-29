/*
 * Copyright (C) 2011 Martin Peres <martin.peres@ensi-bourges.fr>
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
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

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
	int cnum =0;
	uint32_t base = 0;
	int eng = 0;
	while ((c = getopt (argc, argv, "c:bvpso")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
			case 'b':
				base = 0x84000;
				eng = 6;
				break;
			case 'v':
				base = 0x85000;
				eng = 4;
				break;
			case 'p':
				base = 0x86000;
				eng = 2;
				break;
			case 's':
				base = 0x87000;
				eng = 5;
				break;
			case 'o':
				base = 0x104000;
				eng = 3;
				break;
		}
	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}

	if (!base) {
		fprintf (stderr, "No engine specified. Specify -b for PBSP, -v for PVP, -p for PPPP, -s for PCRYPT, -o for PCOPY\n");
		return 1;
	}

	if (optind >= argc) {
		fprintf(stderr, "At least specify payload file.\n");
		return 1;
	}

	FILE *payload = fopen(argv[optind], "rb");

	if (payload == NULL) {
		fprintf(stderr, "Binary file %s could not be opened.\n", argv[optind]);
		return 1;
	} else {
		fprintf(stderr, "Uploading file %s.\n", argv[optind]);
	}

	int32_t i;
	nva_wr32(cnum, 0x200, 0x40110011);
	nva_wr32(cnum, 0x200, 0xffffffff);
	usleep(1000);
	nva_wr32(cnum, 0x1700, 0x10);
	for (i = 0; i < 0x10; i++) {
	nva_wr32(cnum, 0x701000 + i * 0x20, 0x00190000);
	nva_wr32(cnum, 0x701004 + i * 0x20, 0x003fffff);
	nva_wr32(cnum, 0x701008 + i * 0x20, 0x00300000);
	nva_wr32(cnum, 0x70100c + i * 0x20, 0x00000000);
	nva_wr32(cnum, 0x701010 + i * 0x20, 0x00000000);
	nva_wr32(cnum, 0x701014 + i * 0x20, 0x00000000);
	}
	nva_wr32(cnum, 0x1700, 0x30);
	nva_wr32(cnum, 0x700000, 0x00000000);
	nva_wr32(cnum, 0x1700, 0x20);
	for (i = 0; i < 0x100000; i+= 4)
		nva_wr32(cnum, 0x700000 + i, 0xffff06);
	int version = nva_rd32(cnum, base + 0x12c) & 0xf;
	if (version)
		nva_wr32(cnum, base + 0x180, 0x03000000);
	else
		nva_wr32(cnum, base + 0xff8, 0x00100000);

	for (i = 0; ; i += 4) {
		uint32_t word = 0;
		int y;
		int character = 0;
		for (y = 0; y < 4; y++) {
			character = fgetc(payload);
			if (character == EOF) {
				break;
			}
			word |= ((unsigned char)character) << (y * 8);
		}
		if (version == 0) {
			nva_wr32(cnum, base + 0xff4, word);
		}	else {
			if (!(i & 0xff))
				nva_wr32(cnum, base + 0x188, i >> 8);
			nva_wr32(cnum, base + 0x184, word);
		}
		if (character == EOF) {
			break;
		}
	}
	while (i & 0xff) {
		if (version == 0) {
			nva_wr32(cnum, base + 0xff4, 0);
		}	else {
			nva_wr32(cnum, base + 0x184, 0);
		}
		i += 4;
	}

	nva_wr32(cnum, 0x70000, 1);
	while (nva_rd32(cnum, 0x70000));
	usleep(1000);
	nva_wr32(cnum, base+0x004, 0x10);
	nva_wr32(cnum, base+0x048, 3);
	nva_wr32(cnum, base+0x10c, 1);
	nva_wr32(cnum, base+0x104, 0);
	nva_wr32(cnum, base+0x100, 2);
	nva_wr32(cnum, 0x1588, nva_rd32(cnum, 0x1588) & ~0xffff);
	nva_wr32(cnum, 0x2500, 1);
	nva_wr32(cnum, 0x3200, 1);
	nva_wr32(cnum, 0x3250, 1);
	nva_wr32(cnum, 0x1700, 0x10);
	nva_wr32(cnum, 0x700000+0xf00, 0);
	nva_wr32(cnum, 0x700000+0xf04, 0);
	nva_wr32(cnum, 0x700000+0xf08, 0);
	nva_wr32(cnum, 0x700000+0xf0c, 0);
	nva_wr32(cnum, 0x700000+0xf10, 0);
	nva_wr32(cnum, 0x700000+0xf14, 0);
	nva_wr32(cnum, 0x700000+0xf18, 0);
	nva_wr32(cnum, 0x700000+0xf1c, 0);
	nva_wr32(cnum, 0x700000+0xf20, 0);
	nva_wr32(cnum, 0x700000+0xf24, 0);
	nva_wr32(cnum, 0x700000+0xf28, 0);
	nva_wr32(cnum, 0x700000+0xf2c, 0);
	nva_wr32(cnum, 0x700000+0xf30, 0);
	nva_wr32(cnum, 0x700000+0xf34, 0);
	nva_wr32(cnum, 0x700000+0xf38, 0);
	nva_wr32(cnum, 0x700000+0xf3c, 0x403f6078);
	nva_wr32(cnum, 0x700000+0xf40, 0);
	nva_wr32(cnum, 0x700000+0xf44, 0x2101ffff);
	nva_wr32(cnum, 0x700000+0xf48, 0x10);
	nva_wr32(cnum, 0x700000+0xf4c, 0);
	nva_wr32(cnum, 0x700000+0xf50, 0x110000);
	nva_wr32(cnum, 0x700000+0xf54, 0x80000);
	nva_wr32(cnum, 0x700000+0xf58, 0);
	nva_wr32(cnum, 0x700000+0xf5c, 0);
	nva_wr32(cnum, 0x700000+0xf60, 0);
	nva_wr32(cnum, 0x700000+0xf64, 0);
	nva_wr32(cnum, 0x700000+0xf68, 0);
	nva_wr32(cnum, 0x700000+0xf6c, 0);
	nva_wr32(cnum, 0x700000+0xf70, 0);
	nva_wr32(cnum, 0x700000+0xf74, 0);
	nva_wr32(cnum, 0x700000+0xf78, 0);
	nva_wr32(cnum, 0x700000+0xf7c, 0x30000fff);
	nva_wr32(cnum, 0x700000+0xf80, 0x4000600);
	nva_wr32(cnum, 0x700000+0xf84, 0);
	nva_wr32(cnum, 0x700000+0xf88, 0x410);
	nva_wr32(cnum, 0x700000+0xf8c, 0);
	nva_wr32(cnum, 0x700000+0xf90, 0);
	nva_wr32(cnum, 0x700000+0xf94, 0);
	nva_wr32(cnum, 0x700000+0xf98, 0x101);

	nva_wr32(cnum, 0x700000+0xe00, 0);
	nva_wr32(cnum, 0x700000+0xe04, 0);
	nva_wr32(cnum, 0x700000+0xe08, 0);
	nva_wr32(cnum, 0x700000+0xe0c, 0);
	nva_wr32(cnum, 0x700000+0xe10, 0);
	nva_wr32(cnum, 0x700000+0xe14, 0);
	nva_wr32(cnum, 0x700000+0xe18, 0);
	nva_wr32(cnum, 0x700000+0xe1c, 0);
	nva_wr32(cnum, 0x700000+0xe20, 0);
	nva_wr32(cnum, 0x700000+0xe24, 0);
	nva_wr32(cnum, 0x700000+0xe28, 0);
	nva_wr32(cnum, 0x700000+0xe2c, 0);
	nva_wr32(cnum, 0x700000+0xe30, 0);
	nva_wr32(cnum, 0x700000+0xe34, 0);
	nva_wr32(cnum, 0x700000+0xe38, 0);
	nva_wr32(cnum, 0x700000+0xe3c, 0x403f6078);
	nva_wr32(cnum, 0x700000+0xe40, 0);
	nva_wr32(cnum, 0x700000+0xe44, 0x2101ffff);
	nva_wr32(cnum, 0x700000+0xe48, 0x10);
	nva_wr32(cnum, 0x700000+0xe4c, 0);
	nva_wr32(cnum, 0x700000+0xe50, 0x110000);
	nva_wr32(cnum, 0x700000+0xe54, 0x80000);
	nva_wr32(cnum, 0x700000+0xe58, 0);
	nva_wr32(cnum, 0x700000+0xe5c, 0);
	nva_wr32(cnum, 0x700000+0xe60, 0);
	nva_wr32(cnum, 0x700000+0xe64, 0);
	nva_wr32(cnum, 0x700000+0xe68, 0);
	nva_wr32(cnum, 0x700000+0xe6c, 0);
	nva_wr32(cnum, 0x700000+0xe70, 0);
	nva_wr32(cnum, 0x700000+0xe74, 0);
	nva_wr32(cnum, 0x700000+0xe78, 0);
	nva_wr32(cnum, 0x700000+0xe7c, 0x30000fff);
	nva_wr32(cnum, 0x700000+0xe80, 0x4000600);
	nva_wr32(cnum, 0x700000+0xe84, 0);
	nva_wr32(cnum, 0x700000+0xe88, 0x410);
	nva_wr32(cnum, 0x700000+0xe8c, 0);
	nva_wr32(cnum, 0x700000+0xe90, 0);
	nva_wr32(cnum, 0x700000+0xe94, 0);
	nva_wr32(cnum, 0x700000+0xe98, 0x101);
	for (i = 0; i <= 0x1000; i+= 4)
		nva_wr32(cnum, 0x700000+0x6000 + i, 0);
	nva_wr32(cnum, 0x700000+0x7008, 0x1);
	nva_wr32(cnum, 0x700000+0x700c, 0x00000004 | eng << 20);
	nva_wr32(cnum, 0x700000+0x1100, 0x0019003d);
	nva_wr32(cnum, 0x700000+0x1104, 0xffffffff);
	nva_wr32(cnum, 0x700000+0x1108, 0);
	nva_wr32(cnum, 0x700000+0x110c, 0);
	nva_wr32(cnum, 0x700000+0x1110, 0);
	nva_wr32(cnum, 0x700000+0x1114, 0);
	nva_wr32(cnum, 0x700000+0x9000, 1);
	nva_wr32(cnum, 0x70000, 1);
	while (nva_rd32(cnum, 0x70000));
	nva_wr32(cnum, 0x2600, 0x8000100d);
	nva_wr32(cnum, 0x2604, 0xc000100f);
	nva_wr32(cnum, 0x27fc, 0x8000100e);
	nva_wr32(cnum, 0x32f4, 0x109);
	nva_wr32(cnum, 0x32ec, 0x1);
	nva_wr32(cnum, 0x2500, 0x101);
	nva_wr32(cnum, 0x700000+0x20000, 0x00040000);
	nva_wr32(cnum, 0x700000+0x20004, 0x00000001);
	nva_wr32(cnum, 0x700000+0x10000, 0x00120000);
	nva_wr32(cnum, 0x700000+0x10004, 0x00008000);
	nva_wr32(cnum, 0x70000, 1);
	while (nva_rd32(cnum, 0x70000));
	nva_wr32(cnum, 0xc0208c, 1);
	return 0;
}
