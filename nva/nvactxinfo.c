/*
 * Copyright (C) 2011 Marcelina Kościelnicka <mwk@0x04.net>
 * Copyright (C) 2016 Roy Spliet <rs855@cam.ac.uk>
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
#include <pthread.h>
#include <sched.h>
#include <inttypes.h>

int cnum = 0;
int32_t a;

uint32_t dev;

#define NVE0_HUB_SCRATCH_7                                 0x0040981c

struct nva_interval {
	uint64_t start;
	uint64_t diff;
};

uint32_t ctxsize_strands(uint32_t reg_base)
{
	uint32_t strand_count, strand_size = 0, io_idx, i;
	uint32_t host_io_port_off = dev > 0x100 ? 0x20ac : 0x2ffc;

	strand_count = nva_rd32(cnum, reg_base + 0x2880);
	io_idx = nva_rd32(cnum, reg_base + host_io_port_off);

	for (i = 0; i < strand_count; i++) {
		nva_wr32(cnum, reg_base + host_io_port_off, i);
		strand_size += (nva_rd32(cnum, reg_base + 0x2910) << 2);
	}
	nva_wr32(cnum, reg_base + host_io_port_off, io_idx);

	return strand_size;
}

uint32_t ctxsize_tpc_strands(uint32_t reg_base)
{
	uint32_t strand_count, strand_size = 0, io_idx, i;

	strand_count = nva_rd32(cnum, reg_base + 0x570);
	io_idx = nva_rd32(cnum, reg_base + 0x59c);

	for (i = 0; i < strand_count; i++) {
		nva_wr32(cnum, reg_base + 0x59c, i);
		strand_size += (nva_rd32(cnum, reg_base + 0x590) << 2);
	}
	nva_wr32(cnum, reg_base + 0x59c, io_idx);

	return strand_size;
}

void print_hwinfo()
{
	uint32_t gpc_cnt, gpc_size, gpc_area, gpc_base, smx_cnt, tpc_cnt, tpc_base,
				tpc_size, ctx_size, size, i, j;

	printf("Card: NV%X\n", dev);

	smx_cnt = (nva_rd32(cnum, 0x418bb8) >> 8) & 0xff;
	printf("SM  count             : %d\n", smx_cnt);

	gpc_cnt = nva_rd32(cnum, 0x22430);
	printf("GPC count             : %d\n", gpc_cnt);

	gpc_size = 0;
	gpc_area = 0;
	for (i = 0; i < gpc_cnt; i++) {
		gpc_base = 0x500000 + (i * 0x8000);
		tpc_cnt = nva_rd32(cnum, gpc_base + 0x2608) & 0x1f;
		printf("  GPC[%2u] TPCs        : %d\n", i, tpc_cnt);
		ctx_size = nva_rd32(cnum, gpc_base + 0x274c) << 2;
		printf("  GPC[%2u] base size   : %d bytes\n", i, ctx_size);
		size = ctxsize_strands(gpc_base);
		printf("  GPC[%2u] strand size : %d bytes\n", i, size);
		ctx_size += size;
		if (dev >= 0x107) {
			tpc_size = 0;
			for (j = 0; j < tpc_cnt; j++) {
				size = 0;
				tpc_base = gpc_base + 0x4000 + (j * 0x800);
				size = ctxsize_tpc_strands(tpc_base);
				printf("    TPC[%2u] strands   : %d bytes\n", j, size);
				tpc_size += size;
			}
			printf("  GPC[%2u] TPC size    : %d bytes\n", i, tpc_size);
			ctx_size += tpc_size;
		}
		printf("  GPC[%2u] ctx size    : %d bytes\n", i, ctx_size);
		gpc_size += ctx_size;

		ctx_size = nva_rd32(cnum, 0x502804 + (i * 0x8000));
		printf("  GPC[%2u] ctx area    : %d bytes\n", i, ctx_size);
		gpc_area += ctx_size;
	}

	printf("GPC context size      : %d bytes\n", gpc_size);

	ctx_size = nva_rd32(cnum, 0x41a804);
	printf("GPC context area      : %d bytes\n", gpc_area);
	printf("\n");

	ctx_size = nva_rd32(cnum,0x40974c) << 2;
	ctx_size += ctxsize_strands(0x407000);
	printf("HUB context size      : %d bytes\n", ctx_size);

	printf("Total context size    : %d bytes\n", ctx_size + gpc_size);

	ctx_size = nva_rd32(cnum, 0x409804);
	printf("Total context area    : %d bytes\n", ctx_size);

	printf("\n");
}

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
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

	a = NVE0_HUB_SCRATCH_7;
	dev = nva_rd32(cnum, 0x000000);
	dev &= 0x1ff00000;
	dev >>= 20;

	print_hwinfo();

	return 0;
}
