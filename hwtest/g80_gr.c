/*
 * Copyright (C) 2016 Marcin Kościelnicki <koriakin@0x04.net>
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
#include "g80_gr.h"
#include "nva.h"


int g80_gr_intr(struct hwtest_ctx *ctx) {
	uint32_t intr = nva_rd32(ctx->cnum, 0x400100);
	if (!intr)
		return 0;
	if (intr & 0x100000)
		printf("DATA_ERROR %#x\n", nva_rd32(ctx->cnum, 0x400110));
	if (intr & 0x200000) {
		uint32_t trap = nva_rd32(ctx->cnum, 0x400108);
		printf("TRAP %#x\n", trap);
	}
	if (intr & 0x1)
		printf("NOTIFY\n");
	if (intr & 0x10)
		printf("INVALID METHOD\n");
	if (intr & 0x20)
		printf("INVALID CLASS\n");
	if (intr & 0x40)
		printf("INVALID NOTIFY\n");
	if (intr & 0x10000)
		printf("BUFFER\n");
	if (intr & 0x1000000)
		printf("SINGLE STEP\n");
	if (intr & 0x1100071)
		printf("CLASS %04x ADDR %08x DATA %08x",
			nva_rd32(ctx->cnum, 0x400814) & 0xffff,
			nva_rd32(ctx->cnum, 0x400704),
			nva_rd32(ctx->cnum, 0x400708));
	uint32_t rintr = intr & ~0x1310071;
	if (rintr)
		printf("INTERRUPT %08x\n", rintr);
	if (intr & ~0x01010001)
		return 1;
	nva_wr32(ctx->cnum, 0x400100, intr);
	nva_wr32(ctx->cnum, 0x400500, 0x10001);
	return 0;
}

int g80_gr_mthd(struct hwtest_ctx *ctx, uint32_t subc, uint32_t mthd, uint32_t data) {
	while (nva_rd32(ctx->cnum, 0x400808) & 0x80000000) {
		if (g80_gr_intr(ctx))
			return 1;
	}
	if (g80_gr_intr(ctx))
		return 1;
	nva_wr32(ctx->cnum, 0x40080c, data);
	nva_wr32(ctx->cnum, 0x400808, mthd | subc << 16 | 0x80000000);
	return 0;
}

int g80_gr_idle(struct hwtest_ctx *ctx) {
	while (nva_rd32(ctx->cnum, 0x400700)) {
		if (g80_gr_intr(ctx))
			return 1;
	}
	if (g80_gr_intr(ctx))
		return 1;
	return 0;
}

int g80_gr_prep(struct hwtest_ctx *ctx) {
	uint32_t units = nva_rd32(ctx->cnum, 0x1540);
	/* Reset graph & fifo */
	nva_wr32(ctx->cnum, 0x200, 0xffdfeeff);
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	/* Aim window at 0x1000000, where our channel will be. */
	nva_wr32(ctx->cnum, 0x1700, 0x100);
	/* Make a DMA object 0x10 convering 4GB of VRAM. */
	if (ctx->chipset == 0xaa || ctx->chipset == 0xac || ctx->chipset == 0xaf) {
		uint64_t base = (uint64_t)nva_rd32(ctx->cnum, 0x880f4) << 12;
		uint64_t limit = base + 0xffffffffull;
		nva_wr32(ctx->cnum, 0x700100, 0x001a003d);
		nva_wr32(ctx->cnum, 0x700104, limit);
		nva_wr32(ctx->cnum, 0x700108, base);
		nva_wr32(ctx->cnum, 0x70010c, (base >> 32 & 0xff) | (limit >> 32 & 0xff) << 24);
		nva_wr32(ctx->cnum, 0x700110, 0);
		nva_wr32(ctx->cnum, 0x700114, 0x10000);
	} else {
		nva_wr32(ctx->cnum, 0x700100, 0x0019003d);
		nva_wr32(ctx->cnum, 0x700104, 0xffffffff);
		nva_wr32(ctx->cnum, 0x700108, 0);
		nva_wr32(ctx->cnum, 0x70010c, 0);
		nva_wr32(ctx->cnum, 0x700110, 0);
		nva_wr32(ctx->cnum, 0x700114, 0x10000);
	}
	/* Make m2mf object 0x12. */
	nva_wr32(ctx->cnum, 0x700120, 0x5039);
	nva_wr32(ctx->cnum, 0x700124, 0);
	nva_wr32(ctx->cnum, 0x700128, 0);
	nva_wr32(ctx->cnum, 0x70012c, 0);
	/* Make 2d object 0x13. */
	nva_wr32(ctx->cnum, 0x700130, 0x502d);
	nva_wr32(ctx->cnum, 0x700134, 0);
	nva_wr32(ctx->cnum, 0x700138, 0);
	nva_wr32(ctx->cnum, 0x70013c, 0);
	/* Make compat 3d object 0x14. */
	nva_wr32(ctx->cnum, 0x700140, 0x5097);
	nva_wr32(ctx->cnum, 0x700144, 0);
	nva_wr32(ctx->cnum, 0x700148, 0);
	nva_wr32(ctx->cnum, 0x70014c, 0);
	/* Make current 3d object 0x15. */
	uint16_t gr3d;
	if (ctx->chipset == 0x50) {
		/* G80 */
		gr3d = 0x5097;
	} else if (ctx->chipset < 0xa0) {
		/* G84 */
		gr3d = 0x8297;
	} else if (ctx->chipset == 0xa0 || ctx->chipset == 0xaa || ctx->chipset == 0xac) {
		/* G200 */
		gr3d = 0x8397;
	} else if (ctx->chipset != 0xaf) {
		/* GT215 */
		gr3d = 0x8597;
	} else {
		/* MCP89 */
		gr3d = 0x8697;
	}
	nva_wr32(ctx->cnum, 0x700150, gr3d);
	nva_wr32(ctx->cnum, 0x700154, 0);
	nva_wr32(ctx->cnum, 0x700158, 0);
	nva_wr32(ctx->cnum, 0x70015c, 0);
	/* Make old compute object 0x16. */
	nva_wr32(ctx->cnum, 0x700160, 0x50c0);
	nva_wr32(ctx->cnum, 0x700164, 0);
	nva_wr32(ctx->cnum, 0x700168, 0);
	nva_wr32(ctx->cnum, 0x70016c, 0);
	/* Make new compute object 0x17. */
	nva_wr32(ctx->cnum, 0x700170, 0x85c0);
	nva_wr32(ctx->cnum, 0x700174, 0);
	nva_wr32(ctx->cnum, 0x700178, 0);
	nva_wr32(ctx->cnum, 0x70017c, 0);
	/* Flush it. */
	nva_wr32(ctx->cnum, 0x70000, 1);
	while (nva_rd32(ctx->cnum, 0x70000));
	/* Clear exception status. */
	nva_wr32(ctx->cnum, 0x400804, 0xc0000000);
	nva_wr32(ctx->cnum, 0x406800, 0xc0000000);
	nva_wr32(ctx->cnum, 0x400c04, 0xc0000000);
	nva_wr32(ctx->cnum, 0x401800, 0xc0000000);
	nva_wr32(ctx->cnum, 0x405018, 0xc0000000);
	nva_wr32(ctx->cnum, 0x402000, 0xc0000000);
	int i;
	/* Clear TPC exception status. */
	for (i = 0; i < 16; i++)
		if (units & 1 << i) {
			if (ctx->chipset < 0xa0) {
				nva_wr32(ctx->cnum, 0x408314 | i << 12, 0xc0000000);
				nva_wr32(ctx->cnum, 0x408318 | i << 12, 0xffffffff);
				nva_wr32(ctx->cnum, 0x408900 | i << 12, 0xc0000000);
				nva_wr32(ctx->cnum, 0x408e08 | i << 12, 0xc0000000);
			} else {
				nva_wr32(ctx->cnum, 0x40831c | i << 11, 0xc0000000);
				nva_wr32(ctx->cnum, 0x408320 | i << 11, 0xffffffff);
				nva_wr32(ctx->cnum, 0x408600 | i << 11, 0xc0000000);
				nva_wr32(ctx->cnum, 0x408708 | i << 11, 0xc0000000);
			}
		}
	/* Reset all units, for a good measure. */
	nva_wr32(ctx->cnum, 0x400040, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400040, 0);
	/* Cargo. */
	nva_wr32(ctx->cnum, 0x405020, 1);
	nva_wr32(ctx->cnum, 0x400080, 0x401889f2);
	/* Reset interrupt & trap status, enable them all. */
	nva_wr32(ctx->cnum, 0x400100, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400108, 0xffffffff);
	nva_wr32(ctx->cnum, 0x40013c, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400138, 0xffffffff);
	/* Poke CTXCTL and bind the channel. */
	nva_wr32(ctx->cnum, 0x400824, 0x00004000);
	nva_wr32(ctx->cnum, 0x400784, 0x00001000);
	nva_wr32(ctx->cnum, 0x400320, 4);
	nva_wr32(ctx->cnum, 0x40032c, 0x80001000);
	/* Enable FIFO access. */
	nva_wr32(ctx->cnum, 0x400500, 0x00010001);
	/* Bind subchannels. */
	if (
		g80_gr_mthd(ctx, 0, 0, 0x12) ||
		g80_gr_mthd(ctx, 1, 0, 0x13) ||
		g80_gr_mthd(ctx, 2, 0, 0x15) ||
		g80_gr_mthd(ctx, 3, 0, 0x16))
		return 1;
	return 0;
}

int g80_gr_compute_prep(struct hwtest_ctx *ctx) {
	/* Notify, semaphore, cond. */
	if (
		g80_gr_mthd(ctx, 3, 0x180, 0x10) ||
		g80_gr_mthd(ctx, 3, 0x1a4, 0x10) ||
		g80_gr_mthd(ctx, 3, 0x320, 0x0) ||
		g80_gr_mthd(ctx, 3, 0x324, 0x0) ||
		g80_gr_mthd(ctx, 3, 0x328, 0x1))
		return 1;
	/* Cargo and misc. */
	if (
		g80_gr_mthd(ctx, 3, 0x364, 0x0) ||
		g80_gr_mthd(ctx, 3, 0x228, 0xffff0) ||
		g80_gr_mthd(ctx, 3, 0x224, 1) ||
		g80_gr_mthd(ctx, 3, 0x288, 0) ||
		g80_gr_mthd(ctx, 3, 0x290, 1) ||
		g80_gr_mthd(ctx, 3, 0x2a0, 0) ||
		g80_gr_mthd(ctx, 3, 0x2b0, 7) ||
		g80_gr_mthd(ctx, 3, 0x2bc, 0) ||
		g80_gr_mthd(ctx, 3, 0x2f0, 0) ||
		g80_gr_mthd(ctx, 3, 0x370, 0) ||
		g80_gr_mthd(ctx, 3, 0x36c, 0))
		return 1;
	/* Global. */
	if (g80_gr_mthd(ctx, 3, 0x1a0, 0x10))
		return 1;
	int i;
	for (i = 0; i < 16; i++) {
		/* Bind each one at 32MB mark, size 16MB. */
		if (
			g80_gr_mthd(ctx, 3, 0x400 | i << 5, 0) ||
			g80_gr_mthd(ctx, 3, 0x404 | i << 5, 0x02000000) ||
			g80_gr_mthd(ctx, 3, 0x408 | i << 5, 0) ||
			g80_gr_mthd(ctx, 3, 0x40c | i << 5, 0xffffff) ||
			g80_gr_mthd(ctx, 3, 0x410 | i << 5, 1))
			return 1;
	}
	/* Local - aimed at 0x1010000 and disabled. */
	if (
		g80_gr_mthd(ctx, 3, 0x1b8, 0x10) ||
		g80_gr_mthd(ctx, 3, 0x294, 0x0) ||
		g80_gr_mthd(ctx, 3, 0x298, 0x1010000) ||
		g80_gr_mthd(ctx, 3, 0x29c, 0x0) ||
		g80_gr_mthd(ctx, 3, 0x2fc, 0x7) ||
		g80_gr_mthd(ctx, 3, 0x300, 0x0))
		return 1;
	/* Stack - aimed at 0x1020000 and disabled. */
	if (
		g80_gr_mthd(ctx, 3, 0x1bc, 0x10) ||
		g80_gr_mthd(ctx, 3, 0x218, 0x0) ||
		g80_gr_mthd(ctx, 3, 0x21c, 0x1020000) ||
		g80_gr_mthd(ctx, 3, 0x220, 0x0) ||
		g80_gr_mthd(ctx, 3, 0x304, 0x7) ||
		g80_gr_mthd(ctx, 3, 0x308, 0x0))
		return 1;
	/* Code+CB - at 0x1030000 and 0x1040000. */
	if (
		g80_gr_mthd(ctx, 3, 0x1c0, 0x10) ||
		g80_gr_mthd(ctx, 3, 0x380, 0) || /* Flush for a good measure. */
		g80_gr_mthd(ctx, 3, 0x210, 0) ||
		g80_gr_mthd(ctx, 3, 0x214, 0x1030000) ||
		g80_gr_mthd(ctx, 3, 0x3b4, 0) ||
		g80_gr_mthd(ctx, 3, 0x2a4, 0) ||
		g80_gr_mthd(ctx, 3, 0x2a8, 0x1040000))
		return 1;
	for (i = 0; i < 0x80; i++)
		if (g80_gr_mthd(ctx, 3, 0x2ac, i << 16))
			return 1;
	for (i = 0; i < 0x10; i++)
		if (g80_gr_mthd(ctx, 3, 0x3c8, i << 8 | 1))
			return 1;
	/* Texture. */
	if (
		g80_gr_mthd(ctx, 3, 0x1cc, 0x10) ||
		g80_gr_mthd(ctx, 3, 0x378, 0) ||
		g80_gr_mthd(ctx, 3, 0x3bc, 0x22) ||
		g80_gr_mthd(ctx, 3, 0x32c, 0) ||
		g80_gr_mthd(ctx, 3, 0x358, 1) ||
		g80_gr_mthd(ctx, 3, 0x3d0, 0x20) ||
		g80_gr_mthd(ctx, 3, 0x37c, 0))
		return 1;
	/* TSC - at 0x1050000. */
	if (
		g80_gr_mthd(ctx, 3, 0x1c4, 0x10) ||
		g80_gr_mthd(ctx, 3, 0x22c, 0) ||
		g80_gr_mthd(ctx, 3, 0x230, 0x1050000) ||
		g80_gr_mthd(ctx, 3, 0x234, 0x7f) ||
		g80_gr_mthd(ctx, 3, 0x27c, 0))
		return 1;
	/* TIC - at 0x1060000. */
	if (
		g80_gr_mthd(ctx, 3, 0x1c8, 0x10) ||
		g80_gr_mthd(ctx, 3, 0x2c4, 0) ||
		g80_gr_mthd(ctx, 3, 0x2c8, 0x1060000) ||
		g80_gr_mthd(ctx, 3, 0x2cc, 0x7f) ||
		g80_gr_mthd(ctx, 3, 0x280, 0))
		return 1;
	return 0;
}
