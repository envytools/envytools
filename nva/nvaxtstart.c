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

uint32_t code[] = {
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x400fa012,
	0xa0220111,
	0x13e6200f,
	0x26002122,
	0x22260e12,
	0x32322620,
	0x264a4226,
	0xfa866252,
	0x012122ff,
	0x22002222,
	0x20c00161,
	0x00a02200,
	0x46006122,
	0x2122fff5,
	0x02213201,
	0xc0006232,
	0xa0220020,
	0x00612200,
	0x22fff006,
	0x05050121,
	0x03002000,
	0x22016122,
	0x612200a0,
	0x0020c003,
	0x46006122,
	0x2122ffe9,
	0x02213201,
	0x30000305,
	0xa0221300,
	0x03612200,
	0x220020c0,
	0xe2860061,
	0x012122ff,
	0x05022132,
	0x20f00003,
	0x02613200,
	0x2200a022,
	0x20c00361,
	0x00612200,
	0x00ffdb06,
	0x42014022,
	0x70420070,
	0x00200001,
	0xe20070e2,
	0x20000170,
	0x00008000,
	0x20004022,
	0x40224128,
	0x41282001,
	0x42024022,
	0x70420070,
	0x00200001,
	0xe20070e2,
	0x20000170,
	0x00008000,
	[0x600/4] = 0x2203e820,
	[0x604/4] = 0x612240c2,
	[0x608/4] = 0x0020c003,
	[0x60c/4] = 0x2200a022,
	[0x610/4] = 0x82860061,
	[0x614/4] = 0x000000fe,
};

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
	int32_t a, i;
	nva_wr32(cnum, 0x200, 0x40110011);
	nva_wr32(cnum, 0x200, 0xffffffff);
	usleep(1000);
	nva_wr32(cnum, 0x1700, 0x10);
	nva_wr32(cnum, 0x701080, 0x00190000);
	nva_wr32(cnum, 0x701084, 0x003fffff);
	nva_wr32(cnum, 0x701088, 0x00300000);
	nva_wr32(cnum, 0x70108c, 0x00000000);
	nva_wr32(cnum, 0x701090, 0x00000000);
	nva_wr32(cnum, 0x701094, 0x00000000);
	nva_wr32(cnum, 0x1700, 0x30);
	nva_wr32(cnum, 0x700000, 0x00000000);
	nva_wr32(cnum, 0x1700, 0x20);
	for (i = 0; i < 0x100000; i+= 4)
		nva_wr32(cnum, 0x700000 + i, 0xffff06);
	for (i = 0; i < sizeof(code) / 4; i++) {
		nva_wr32(cnum, 0x700000 + i * 4, code[i]);
	}
	nva_wr32(cnum, 0x70000, 1);
	while (nva_rd32(cnum, 0x70000));
	usleep(1000);
	nva_wr32(cnum, 0x103d10, 0x1fffffff);
	nva_wr32(cnum, 0x103d08, 0xfffffff);
	nva_wr32(cnum, 0x103d28, 0x90044);
	nva_wr32(cnum, 0x103cc0, 0x2000);
	nva_wr32(cnum, 0x103cc4, 0x1c);
	nva_wr32(cnum, 0x103cc8, 0x400);
	nva_wr32(cnum, 0x103ce4, 0x0000);
	nva_wr32(cnum, 0x103ce8, 0x0f);
	nva_wr32(cnum, 0x103cec, 0x404);
	nva_wr32(cnum, 0x103c20, 0x3f);
	nva_wr32(cnum, 0x103d84, 0x3f);
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
	nva_wr32(cnum, 0x700000+0x700c, 0x00600004);
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
