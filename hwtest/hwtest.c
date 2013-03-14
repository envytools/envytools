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

#include "hwtest.h"
#include "nva.h"
#include <unistd.h>
#include <stdio.h>
#include <pciaccess.h>

int hwtest_root_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(hwtest_root,
	HWTEST_GROUP(nv01_pgraph),
	HWTEST_GROUP(nv10_tile),
	HWTEST_GROUP(nv50_ptherm),
	HWTEST_GROUP(nv84_ptherm),
	HWTEST_GROUP(punk1c1_isa),
)

int main(int argc, char **argv) {
	struct hwtest_ctx sctx;
	struct hwtest_ctx *ctx = &sctx;
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c, force = 0;
	ctx->cnum = 0;
	ctx->colors = 1;
	ctx->noslow = 0;
	ctx->indent = 0;
	ctx->rand48[0] = 0xdead;
	ctx->rand48[1] = 0xbeef;
	ctx->rand48[2] = 0xcafe;
	while ((c = getopt (argc, argv, "c:nsf")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &ctx->cnum);
				break;
			case 'n':
				ctx->colors = 0;
				break;
			case 's':
				ctx->noslow = 1;
				break;
			case 'f':
				force = 1;
				break;
		}
	if (ctx->cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}
	ctx->chipset = nva_cards[ctx->cnum].chipset;
	ctx->card_type = nva_cards[ctx->cnum].card_type;
	if (pci_device_has_kernel_driver(nva_cards[ctx->cnum].pci)) {
		if (force) {
			fprintf(stderr, "WARNING: Kernel driver in use.\n");
		} else {
			fprintf(stderr, "ERROR: Kernel driver in use. If you know what you are doing, use -f option.\n");
			return 1;
		}
	}
	int worst = 0;
	if (optind == argc) {
		printf("Running all tests...\n");
		worst = hwtest_run_group(ctx, &hwtest_root_group, 0);
	} else while (optind < argc) {
		int res = hwtest_run_group(ctx, &hwtest_root_group, argv[optind++]);
		if (res > worst)
			worst = res;
	}
	if (worst == HWTEST_RES_PASS)
		return 0;
	else
		return worst + 1;
}
