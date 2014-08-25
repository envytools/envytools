/*
 * Copyright (C) 2014 Martin Peres <martin.peres@free.fr>
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
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

static void usage(int error_code)
{
	fprintf(stderr, "\nUsage: nvaforcetemp [ -c card_number ] temp (0 to disable)\n");
	exit(error_code);
}

int main(int argc, char **argv) {
	int c, cnum = 0;
	int temp = 0;

	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	
	while ((c = getopt (argc, argv, "hc:")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
			case 'h':
				usage(0);
			default:
				usage(1);
		}
	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}
	
	if (nva_cards[cnum]->chipset.chipset < 0x94) {
		fprintf (stderr, "Your chipset is unsupported (0x%x < 0x94)\n",
			nva_cards[cnum]->chipset.chipset);
		usage(1);
	}
		
	
	if (optind >= argc) {
		fprintf (stderr, "No temperature specified.\n");
		usage(1);
	}
	sscanf (argv[optind], "%u", &temp);
	
	if (temp > 0 || temp > 0x7f)
		nva_mask(cnum, 0x20008, 0x3fc08000, ((temp & 0x7f) << 22) | 0x8000);
	else
		nva_mask(cnum, 0x20008, 0x3fc08000, 0x0);
	
	return 0;
}
