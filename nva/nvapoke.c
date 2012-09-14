/*
 * Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
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
#include <inttypes.h>

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
	struct nva_regspace rs = { 0 };
	while ((c = getopt (argc, argv, "c:i:b:t:")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &rs.cnum);
				break;
			case 'i':
				sscanf(optarg, "%d", &rs.idx);
				break;
			case 'b':
				sscanf(optarg, "%d", &rs.regsz);
				if (rs.regsz != 1 && rs.regsz != 2 && rs.regsz != 4 && rs.regsz != 8) {
					fprintf (stderr, "Invalid size.\n");
					return 1;
				}
				break;
			case 't':
				rs.type = nva_rstype(optarg);
				if (rs.type == -1) {
					fprintf (stderr, "Unknown register space.\n");
					return 1;
				}
				break;
		}
	if (rs.cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}
	rs.card = &nva_cards[rs.cnum];
	if (rs.regsz == 0)
		rs.regsz = nva_rsdefsz(&rs);
	uint32_t a, b = rs.regsz;
	uint64_t val, step = 0;
	if (optind >= argc) {
		fprintf (stderr, "No address specified.\n");
		return 1;
	}
	sscanf (argv[optind], "%x", &a);
	if (optind + 1 >= argc) {
		fprintf (stderr, "No value specified.\n");
		return 1;
	}
	if (optind + 2 >= argc) {
		sscanf (argv[optind + 1], "%"SCNx64, &val);
	} else {
		sscanf (argv[optind + 1], "%x", &b);
		sscanf (argv[optind + 2], "%"SCNx64, &val);
	}
	if (optind + 3 < argc)
		sscanf (argv[optind + 3], "%"SCNx64, &step);
	while (b > 0) {
		int e = nva_wr(&rs, a, val);
		if (e)
			printf("%08x: ERR %c\n", a, nva_rserrc(e));
		a += rs.regsz;
		b -= rs.regsz;
		val += step;
	}
	return 0;
}
