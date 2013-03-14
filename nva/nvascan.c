/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
 * Copyright (C) 2011 Emil Velikov <emil.l.velikov@gmail.com>
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
#include <assert.h>
#include <unistd.h>

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int alias = 0;
	int slow = 0;
	int c;
	struct nva_regspace rs = { 0 };
	while ((c = getopt (argc, argv, "asc:i:b:t:")) != -1)
		switch (c) {
			case 'a':
				alias = 1;
				break;
			case 's':
				slow = 1;
				break;
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
	int32_t a, b = rs.regsz, i;
	if (optind >= argc) {
		fprintf (stderr, "No address specified.\n");
		return 1;
	}
	sscanf (argv[optind], "%x", &a);
	if (optind + 1 < argc)
		sscanf (argv[optind + 1], "%x", &b);
	int ls = 1;
	for (i = 0; i < b; i+=rs.regsz) {
		uint64_t x, y, z;
		int ex = nva_rd(&rs, a+i, &x);
		int ew = 0, ey = 0, ev = 0, ez = 0, eb = 0;
		if (!ex) {
			ew = nva_wr(&rs, a+i, -1ll);
			ey = nva_rd(&rs, a+i, &y);
			ev = nva_wr(&rs, a+i, 0);
			ez = nva_rd(&rs, a+i, &z);
			eb = nva_wr(&rs, a+i, x);
		}
		if (ex || ey || ez || ey || ev || eb || x || y || z) {
			if (ex) {
				printf ("%06x: %c\n", a+i, nva_rserrc(ex));
			} else {
				int cool = (x != y) || (y != z);
				int isalias = 0, areg;
				if (cool && alias) {
					int j;
					nva_wr(&rs, a+i, -1ll);
					for (j = 0; j < i; j+=4) {
						uint64_t sv;
						uint64_t ch;
						int es = nva_rd(&rs, a+j, &sv);
						if (!es) {
							es |= nva_wr(&rs, a+j, 0);
							es |= nva_rd(&rs, a+i, &ch);
							es |= nva_wr(&rs, a+j, sv);
							if (ch == z && !es) {
								areg = a + j;
								isalias = 1;
								break;
							}
						}
					}
					nva_wr(&rs, a+i, x);
				}
				printf ("%06x:", a+i);
				nva_rsprint(&rs, ex, x);
				nva_rsprint(&rs, ey, y);
				nva_rsprint(&rs, ez, z);
				if (ew || ev || eb)
					printf(" WERR");
				if (cool)
					printf(" *");
				if (isalias) {
					printf(" ALIASES %06x", areg);
				}
				printf("\n");
			}
			ls = 1;
		} else {
			if (ls)
				printf("...\n");
			ls = 0;
		}
		if (slow) {
			int j;
			for (j = 0; j < 100; j++)
				nva_rd32(rs.cnum, 0);
			usleep(10000);
			for (j = 0; j < 100; j++)
				nva_rd32(rs.cnum, 0);
		}
	}
	return 0;
}
