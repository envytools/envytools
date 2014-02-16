/*
 * Copyright 2011 Red Hat Inc.
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
 *
 * Authors: Ben Skeggs, Marcin Kościelnicki, Maxim Levitsky
 *
 * based on nvaevo and nvatiming
 */

#include "nva.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

typedef unsigned long long u64;
typedef u64 ptime_t;
#include <time.h>


ptime_t time_diff_us(struct timeval start, struct timeval end)
{
	return ((end.tv_sec) - (start.tv_sec))*1000000 + ((end.tv_usec) - (start.tv_usec));
}

int evosend (int cnum, int c, int m, int d)
{
	uint32_t ctrl;

	if (nva_cards[cnum]->chipset.chipset >= 0xd0) {
		ctrl = nva_rd32(cnum, 0x610700 + (c * 8));
		nva_wr32(cnum, 0x610700 + (c * 8), ctrl | 1);
		nva_wr32(cnum, 0x610704 + (c * 8), d);
		nva_wr32(cnum, 0x610700 + (c * 8), 0x80000001 | m);
		while (nva_rd32(cnum, 0x610700 + (c * 8)) & 0x80000000);
		nva_wr32(cnum, 0x610700 + (c * 8), ctrl);
	} else
	if (nva_cards[cnum]->chipset.chipset == 0x50 ||
	    nva_cards[cnum]->chipset.chipset >= 0x84) {
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

int main(int argc, char **argv) {
	int c, i;
	int cnum =0;
	struct timeval start, end;

	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}

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

	gettimeofday(&start, NULL);
	gettimeofday(&end, NULL);

	for (i=0 ; time_diff_us(start, end) < 1000000 ; i++) {
		evosend(cnum, 0, 0x80, 0);
		gettimeofday(&end, NULL);
	};

	printf("%d evo 0x80 commands in 1 sec\n", i);
	return 0;
}
