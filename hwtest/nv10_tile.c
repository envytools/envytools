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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "hwtest.h"
#include "nva.h"
#include "util.h"
#include <stdio.h>
#include <unistd.h>

static int test_scan(struct hwtest_ctx *ctx) {
	int i;
	int is_nv34 = ctx->chipset == 0x34;
	for (i = 0; i < 8; i++) {
		if (is_nv34) {
			TEST_BITSCAN(0x100240 + i * 0x10, 0xffffc001, 0);
			TEST_BITSCAN(0x100244 + i * 0x10, 0xffffc000, 0);
			TEST_BITSCAN(0x100248 + i * 0x10, 0x0000ff00, 0);
		} else {
			TEST_BITSCAN(0x100240 + i * 0x10, 0x87ffc000, 0);
			TEST_BITSCAN(0x100244 + i * 0x10, 0x07ffc000, 0);
			TEST_BITSCAN(0x100248 + i * 0x10, 0x0000ff00, 0);
		}
	}
	return HWTEST_RES_PASS;
}

static uint32_t compute_status(uint32_t pitch) {
	if (pitch & ~0xff00)
		abort();
	if (!pitch)
		return 0xffffffff;
	int shift = 0;
	while (!(pitch & 0x100))
		pitch >>= 1, shift++;
	if (!shift)
		return 0xffffffff;
	int factor;
	switch (pitch) {
		case 0x100:
			factor = 0;
			break;
		case 0x300:
			factor = 1;
			break;
		case 0x500:
			factor = 2;
			break;
		case 0x700:
			factor = 3;
			break;
		default:
			return 0xffffffff;
	}
	return factor | shift << 4;
}

static int test_status(struct hwtest_ctx *ctx) {
	int i;
	int is_nv34 = ctx->chipset == 0x34;
	for (i = 0; i < 0x100; i++) {
		uint32_t pitch = i << 8;
		nva_wr32(ctx->cnum, 0x100240, 0);
		nva_wr32(ctx->cnum, 0x100248, pitch);
		uint32_t exp = compute_status(pitch);
		if (exp == 0xffffffff)
			exp = 0;
		uint32_t real = nva_rd32(ctx->cnum, 0x10024c);
		if (exp != real) {
			printf("Tile region status mismatch for pitch %05x: is %08x, expected %08x\n", pitch, real, exp);
			return HWTEST_RES_FAIL;
		}
		nva_wr32(ctx->cnum, 0x100240, is_nv34?1:0x80000000);
		exp = compute_status(pitch);
		if (exp == 0xffffffff)
			exp = 0;
		else
			exp |= 0x80000000;
		real = nva_rd32(ctx->cnum, 0x10024c);
		nva_wr32(ctx->cnum, 0x100240, 0);
		if (exp != real) {
			printf("Tile region status mismatch for pitch %05x [enabled]: is %08x, expected %08x\n", pitch, real, exp);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static uint32_t translate_addr(uint32_t pitch, uint32_t laddr, int bankshift, int partflip) {
	uint32_t status = compute_status(pitch);
	uint32_t shift = status >> 4;
	uint32_t x = laddr % pitch;
	uint32_t y = laddr / pitch;
	uint32_t ix = x & 0xff;
	uint32_t iy = laddr >> (shift + 8) & ((1 << (bankshift-8)) - 1);
	uint32_t tx = x >> 8;
	uint32_t ty = y >> (bankshift - 8);
	uint32_t addr = ix + (iy << 8) + (tx << bankshift) + ty * (pitch << (bankshift-8));
	if (ty & 1)
		addr ^= 1 << bankshift;
	if (partflip && addr & 0x100)
		addr ^= 0x10;
	return addr;
}

static int get_bankshift(int cnum) {
	uint32_t cfg = nva_rd32(cnum, 0x100200);
	int colbits = cfg >> 24 & 0xf;
	int partcfg = cfg >> 4 & 3;
	int cellbits[4] = { 3, 4, 2 };
	int bankshift = colbits + cellbits[partcfg];
	return bankshift;
}

static int test_format(struct hwtest_ctx *ctx) {
	int i, j;
	int bankshift = get_bankshift(ctx->cnum);
	int res = HWTEST_RES_PASS;
	int is_nv15 = ctx->chipset == 0x15;
	int partflip = is_nv15 && (nva_rd32(ctx->cnum, 0x100200) & 0x30) == 0x10;
	int is_nv34 = ctx->chipset == 0x34;
	for (i = 0; i < 8; i++)
		nva_wr32(ctx->cnum, 0x100240 + i * 0x10, 0);
	for (i = 0; i < 0x200000; i += 4)
		vram_wr32(ctx->cnum, i, 0xc0000000 | i);
	for (i = 0; i < 0x100; i++) {
		uint32_t pitch = i << 8;
		if (compute_status(pitch) == 0xffffffff)
			continue;
		nva_wr32(ctx->cnum, 0x100248, pitch);
		nva_wr32(ctx->cnum, 0x100244, 0x000fc000);
		nva_wr32(ctx->cnum, 0x100240, is_nv34?1:0x80000000);
		for (j = 0; j < 0x100000; j += 4) {
			uint32_t real = vram_rd32(ctx->cnum, j);
			uint32_t exp = 0xc0000000 | translate_addr(pitch, j, bankshift, partflip);
			if (real != exp) {
				printf("Mismatch at %08x for pitch %05x: is %08x, expected %08x\n", j, pitch, real, exp);
				res = HWTEST_RES_FAIL;
				break;
			}
		}
		for (j = 0x100000; j < 0x180000; j += 4) {
			uint32_t real = vram_rd32(ctx->cnum, j);
			uint32_t exp = 0xc0000000 | j;
			if (real != exp) {
				printf("Mismatch at %08x [after limit] for pitch %05x: is %08x, expected %08x\n", j, pitch, real, exp);
				res = HWTEST_RES_FAIL;
				break;
			}
		}
		nva_wr32(ctx->cnum, 0x100240, 0x0);
	}
	return res;
}

int test_format_bs(struct hwtest_ctx *ctx, int bs) {
	uint32_t orig = nva_rd32(ctx->cnum, 0x100200);
	uint32_t cfg = orig;
	int bankshift = get_bankshift(ctx->cnum);
	if (bs > bankshift)
		return HWTEST_RES_NA;
	while (bs < bankshift) {
		/* leaving unused column bits == bad idea with NV34 for some reason */
		if ((cfg >> 24 & 0xf) > 8 && ctx->chipset != 0x34) {
			cfg -= 1 << 24;
		} else {
			int partcfg = cfg >> 4 & 3;
			if (partcfg == 1) {
				cfg &= 0xffffffcf;
			} else if (partcfg == 0 && ctx->chipset != 0x10) {
				cfg |= 0x20;
			} else {
				nva_wr32(ctx->cnum, 0x100200, orig);
				return HWTEST_RES_NA;
			}
		}
		nva_wr32(ctx->cnum, 0x100200, cfg);
		bankshift = get_bankshift(ctx->cnum);
	}
	int res = test_format(ctx);
	nva_wr32(ctx->cnum, 0x100200, orig);
	return res;
}

static int test_format_bs10(struct hwtest_ctx *ctx) {
	return test_format_bs(ctx, 10);
}

static int test_format_bs11(struct hwtest_ctx *ctx) {
	return test_format_bs(ctx, 11);
}

static int test_format_bs12(struct hwtest_ctx *ctx) {
	return test_format_bs(ctx, 12);
}

static int test_format_bs13(struct hwtest_ctx *ctx) {
	return test_format_bs(ctx, 13);
}

static int nv10_tile_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset < 0x10 || (ctx->chipset >= 0x1a && ctx->chipset != 0x34)) {
		return HWTEST_RES_NA;
	}
	if (!(nva_rd32(ctx->cnum, 0x200) & 1 << 20)) {
		printf("Mem controller not up.\n");
		return HWTEST_RES_UNPREP;
	}
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(nv10_tile, 
	HWTEST_TEST(test_scan, 0),
	HWTEST_TEST(test_status, 0),
	HWTEST_TEST(test_format_bs10, 1),
	HWTEST_TEST(test_format_bs11, 1),
	HWTEST_TEST(test_format_bs12, 1),
	HWTEST_TEST(test_format_bs13, 1),
)
