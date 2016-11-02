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
#include "nvhw/pgraph.h"
#include "nva.h"

static int test_scan_debug(struct hwtest_ctx *ctx) {
	bool is_nv5 = ctx->chipset.chipset >= 5;
	TEST_BITSCAN(0x400080, 0x1337f000, 0);
	TEST_BITSCAN(0x400084, is_nv5 ? 0xf2ffb701 : 0x72113101, 0);
	TEST_BITSCAN(0x400088, 0x11d7fff1, 0);
	TEST_BITSCAN(0x40008c, is_nv5 ? 0xfbffff73 : 0x11ffff33, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_control(struct hwtest_ctx *ctx) {
	bool is_nv5 = ctx->chipset.chipset >= 5;
	uint32_t ctx_mask = is_nv5 ? 0x7f73f0ff : 0x0303f0ff;
	uint32_t offset_mask = is_nv5 ? 0x01fffff0 : 0x00fffff0;
	TEST_BITSCAN(0x400140, 0x00011311, 0);
	TEST_BITSCAN(0x400104, 0x00007800, 0);
	TEST_BITSCAN(0x400160, ctx_mask, 0);
	TEST_BITSCAN(0x400164, 0xffff3f03, 0);
	TEST_BITSCAN(0x400168, 0xffffffff, 0);
	TEST_BITSCAN(0x40016c, 0x0000ffff, 0);
	TEST_BITSCAN(0x400170, 0x11010103, 0);
	TEST_BITSCAN(0x400174, 0x0f00e000, 0);
	TEST_BITSCAN(0x400610, 0xe0000000 | offset_mask, 0);
	TEST_BITSCAN(0x400614, 0xc0000000 | offset_mask, 0);
	TEST_BITSCAN(0x400720, 0x1, 0);
	nva_wr32(ctx->cnum, 0x400720, 0);
	int i;
	for (i = 0 ; i < 8; i++) {
		TEST_BITSCAN(0x400180 + i * 4, ctx_mask, 0);
		TEST_BITSCAN(0x4001a0 + i * 4, 0xffff3f03, 0);
		TEST_BITSCAN(0x4001c0 + i * 4, 0xffffffff, 0);
		TEST_BITSCAN(0x4001e0 + i * 4, 0x0000ffff, 0);
	}
	for (i = 0 ; i < 4; i++) {
		TEST_BITSCAN(0x400730 + i * 4, 0x00007fff, 0);
		TEST_BITSCAN(0x400740 + i * 4, 0xffffffff, 0);
	}
	TEST_BITSCAN(0x400750, 0x00000077, 0);
	TEST_BITSCAN(0x400754, is_nv5 ? 0x003fffff : 0x000fffff, 0);
	TEST_BITSCAN(0x400758, 0xffffffff, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_vtx(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0 ; i < 32; i++) {
		TEST_BITSCAN(0x400400 + i * 4, 0xffffffff, 0);
		TEST_BITSCAN(0x400480 + i * 4, 0xffffffff, 0);
	}
	for (i = 0 ; i < 16; i++) {
		TEST_BITSCAN(0x400d00 + i * 4, 0xffffffc0, 0);
		TEST_BITSCAN(0x400d40 + i * 4, 0xffffffc0, 0);
		TEST_BITSCAN(0x400d80 + i * 4, 0xffffffc0, 0);
	}
	return HWTEST_RES_PASS;
}

static int test_scan_d3d(struct hwtest_ctx *ctx) {
	TEST_BITSCAN(0x400590, 0xfd1d1d1d, 0);
	TEST_BITSCAN(0x400594, 0xff1f1f1f, 0);
	TEST_BITSCAN(0x400598, 0xfd1d1d1d, 0);
	TEST_BITSCAN(0x40059c, 0xff1f1f1f, 0);
	TEST_BITSCAN(0x4005a8, 0xfffff7a6, 0);
	TEST_BITSCAN(0x4005ac, 0xfffff7a6, 0);
	TEST_BITSCAN(0x4005b0, 0xffff9e1e, 0);
	TEST_BITSCAN(0x4005b4, 0xffff9e1e, 0);
	TEST_BITSCAN(0x4005c0, 0xffffffff, 0);
	TEST_BITSCAN(0x4005c4, 0xffffffc0, 0);
	TEST_BITSCAN(0x4005c8, 0xffffffc0, 0);
	TEST_BITSCAN(0x4005cc, 0xffffffc0, 0);
	TEST_BITSCAN(0x4005d0, 0xffffffc0, 0);
	TEST_BITSCAN(0x4005d4, 0xffffffff, 0);
	TEST_BITSCAN(0x4005d8, 0xffffffff, 0);
	TEST_BITSCAN(0x4005dc, 0xffffffff, 0);
	TEST_BITSCAN(0x4005e0, 0xffffffc0, 0);
	TEST_BITSCAN(0x400818, 0xffff5fff, 0);
	TEST_BITSCAN(0x40081c, 0xfffffff1, 0);
	TEST_BITSCAN(0x400820, 0x00000fff, 0);
	TEST_BITSCAN(0x400824, 0xff1111ff, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_clip(struct hwtest_ctx *ctx) {
	int i;
	TEST_BITSCAN(0x400534, 0x0003ffff, 0);
	TEST_BITSCAN(0x400538, 0x0003ffff, 0);
	for (i = 0; i < 1000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t v0 = jrand48(ctx->rand48);
		uint32_t v1 = jrand48(ctx->rand48);
		uint32_t v2 = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, 0x40053c + idx * 4, v0);
		nva_wr32(ctx->cnum, 0x400544 + idx * 4, v1);
		v0 &= 0xffff;
		v1 &= 0x3ffff;
		TEST_READ(0x40053c + idx * 4, v0, "v0 %08x v1 %08x", v0, v1);
		TEST_READ(0x400544 + idx * 4, v1, "v0 %08x v1 %08x", v0, v1);
		if (jrand48(ctx->rand48) & 1) {
			nva_wr32(ctx->cnum, 0x40053c + idx * 4, v2);
		} else {
			nva_wr32(ctx->cnum, 0x400544 + idx * 4, v2);
		}
		v1 &= 0xffff;
		v2 &= 0x3ffff;
		TEST_READ(0x40053c + idx * 4, v1, "v0 %08x v1 %08x v2 %08x", v0, v1, v2);
		TEST_READ(0x400544 + idx * 4, v2, "v0 %08x v1 %08x v2 %08x", v0, v1, v2);
	}
	for (i = 0; i < 1000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t v0 = jrand48(ctx->rand48);
		uint32_t v1 = jrand48(ctx->rand48);
		uint32_t v2 = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, 0x400560 + idx * 4, v0);
		nva_wr32(ctx->cnum, 0x400568 + idx * 4, v1);
		v0 &= 0xffff;
		v1 &= 0x3ffff;
		TEST_READ(0x400560 + idx * 4, v0, "v0 %08x v1 %08x", v0, v1);
		TEST_READ(0x400568 + idx * 4, v1, "v0 %08x v1 %08x", v0, v1);
		if (jrand48(ctx->rand48) & 1) {
			nva_wr32(ctx->cnum, 0x400560 + idx * 4, v2);
		} else {
			nva_wr32(ctx->cnum, 0x400568 + idx * 4, v2);
		}
		v1 &= 0xffff;
		v2 &= 0x3ffff;
		TEST_READ(0x400560 + idx * 4, v1, "v0 %08x v1 %08x v2 %08x", v0, v1, v2);
		TEST_READ(0x400568 + idx * 4, v2, "v0 %08x v1 %08x v2 %08x", v0, v1, v2);
	}
	return HWTEST_RES_PASS;
}

static int test_scan_context(struct hwtest_ctx *ctx) {
	TEST_BITSCAN(0x400600, 0xffffffff, 0);
	TEST_BITSCAN(0x400604, 0xff, 0);
	TEST_BITSCAN(0x400608, 0x7f800000, 0);
	TEST_BITSCAN(0x40060c, 0xffffffff, 0);
	TEST_BITSCAN(0x400800, 0xffffffff, 0);
	TEST_BITSCAN(0x400804, 0xffffffff, 0);
	TEST_BITSCAN(0x400808, 0xffffffff, 0);
	TEST_BITSCAN(0x40080c, 0xffffffff, 0);
	TEST_BITSCAN(0x400810, 0x00000013, 0);
	TEST_BITSCAN(0x400814, 0xffffffff, 0);
	uint32_t offset_mask = ctx->chipset.chipset == 5 ? 0x01fffff0 : 0x00fffff0;
	TEST_BITSCAN(0x400640, offset_mask, 0);
	TEST_BITSCAN(0x400644, offset_mask, 0);
	TEST_BITSCAN(0x400648, offset_mask, 0);
	TEST_BITSCAN(0x40064c, offset_mask, 0);
	TEST_BITSCAN(0x400650, offset_mask, 0);
	TEST_BITSCAN(0x400654, offset_mask, 0);
	TEST_BITSCAN(0x400658, offset_mask, 0);
	TEST_BITSCAN(0x40065c, offset_mask, 0);
	TEST_BITSCAN(0x400660, offset_mask, 0);
	TEST_BITSCAN(0x400664, offset_mask, 0);
	TEST_BITSCAN(0x400668, offset_mask, 0);
	TEST_BITSCAN(0x40066c, offset_mask, 0);
	TEST_BITSCAN(0x400670, 0x00001ff0, 0);
	TEST_BITSCAN(0x400674, 0x00001ff0, 0);
	TEST_BITSCAN(0x400678, 0x00001ff0, 0);
	TEST_BITSCAN(0x40067c, 0x00001ff0, 0);
	TEST_BITSCAN(0x400680, 0x00001ff0, 0);
	TEST_BITSCAN(0x400684, 0x8000000f | offset_mask, 0xf);
	TEST_BITSCAN(0x400688, 0x8000000f | offset_mask, 0xf);
	TEST_BITSCAN(0x40068c, 0x8000000f | offset_mask, 0xf);
	TEST_BITSCAN(0x400690, 0x8000000f | offset_mask, 0xf);
	TEST_BITSCAN(0x400694, 0x8000000f | offset_mask, 0xf);
	TEST_BITSCAN(0x400698, 0x8000000f | offset_mask, 0xf);
	TEST_BITSCAN(0x40069c, 0x0f0f0000, 0);
	TEST_BITSCAN(0x4006a0, 0x0f0f0000, 0);
	TEST_BITSCAN(0x40070c, 0x00000003, 0);
	TEST_BITSCAN(0x400710, 0x0f731f3f, 0);
	TEST_BITSCAN(0x400714, 0x00110101, 0);
	TEST_BITSCAN(0x400724, 0x00ffffff, 0);
	TEST_BITSCAN(0x400830, 0x3f3f3f3f, 0);
	nva_wr32(ctx->cnum, 0x40008c, 0x01000000);
	for (int i = 0; i < 0x40; i++)
		TEST_BITSCAN(0x400900 + i * 4, 0x00ffffff, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_vstate(struct hwtest_ctx *ctx) {
	TEST_BITSCAN(0x400500, 0x300000ff, 0);
	TEST_BITSCAN(0x400504, 0x300000ff, 0);
	TEST_BITSCAN(0x400508, 0xf07fffff, 0);
	TEST_BITSCAN(0x400578, 0x1fffffff, 0);
	TEST_BITSCAN(0x40050c, 0xffffffff, 0);
	TEST_BITSCAN(0x400510, 0x00ffffff, 0);
	TEST_BITSCAN(0x400514, 0xf013ffff, 0);
	TEST_BITSCAN(0x400518, 0x00111031, 0);
	TEST_BITSCAN(0x40051c, 0x00111031, 0);
	TEST_BITSCAN(0x400520, 0x7f7f1111, 0);
	TEST_BITSCAN(0x400524, 0xffffffff, 0);
	TEST_BITSCAN(0x400528, 0xffffffff, 0);
	TEST_BITSCAN(0x40052c, 0xffffffff, 0);
	TEST_BITSCAN(0x400530, 0xffffffff, 0);
	TEST_BITSCAN(0x40057c, 0xffffffff, 0);
	TEST_BITSCAN(0x400580, 0xffffffff, 0);
	TEST_BITSCAN(0x400584, 0xffffffff, 0);
	TEST_BITSCAN(0x400570, 0x00ffffff, 0);
	TEST_BITSCAN(0x400574, 0x00ffffff, 0);
	TEST_BITSCAN(0x400760, 0xffffffff, 0);
	TEST_BITSCAN(0x400764, 0x0000033f, 0);
	TEST_BITSCAN(0x400768, 0x01030000, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_dma(struct hwtest_ctx *ctx) {
	TEST_BITSCAN(0x401000, 0xffffffff, 0);
	TEST_BITSCAN(0x401004, 0xffffffff, 0);
	TEST_BITSCAN(0x401008, 0x003fffff, 0);
	TEST_BITSCAN(0x40100c, 0x0077ffff, 0);
	TEST_BITSCAN(0x401020, 0xffffffff, 0);
	TEST_BITSCAN(0x401024, 0xffffffff, 0);
	for (int i = 0; i < 2; i++) {
		TEST_BITSCAN(0x401040 + i * 0x40, 0x0000ffff, 0);
		TEST_BITSCAN(0x401044 + i * 0x40, 0xfff33000, 0);
		TEST_BITSCAN(0x401048 + i * 0x40, 0xffffffff, 0);
		TEST_BITSCAN(0x40104c + i * 0x40, 0xfffff002, 0);
		TEST_BITSCAN(0x401050 + i * 0x40, 0xfffff000, 0);
		TEST_BITSCAN(0x401054 + i * 0x40, 0xffffffff, 0);
		TEST_BITSCAN(0x401058 + i * 0x40, 0xffffffff, 0);
		TEST_BITSCAN(0x40105c + i * 0x40, 0x01ffffff, 0);
		TEST_BITSCAN(0x401060 + i * 0x40, 0x000007ff, 0);
	}
	return HWTEST_RES_PASS;
}

static int scan_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int nv04_pgraph_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset < 0x04 || ctx->chipset.chipset >= 0x10)
		return HWTEST_RES_NA;
	if (!(nva_rd32(ctx->cnum, 0x200) & 1 << 24)) {
		printf("Mem controller not up.\n");
		return HWTEST_RES_UNPREP;
	}
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	return HWTEST_RES_PASS;
}

namespace {

HWTEST_DEF_GROUP(scan,
	HWTEST_TEST(test_scan_debug, 0),
	HWTEST_TEST(test_scan_control, 0),
	HWTEST_TEST(test_scan_vtx, 0),
	HWTEST_TEST(test_scan_d3d, 0),
	HWTEST_TEST(test_scan_clip, 0),
	HWTEST_TEST(test_scan_context, 0),
	HWTEST_TEST(test_scan_vstate, 0),
	HWTEST_TEST(test_scan_dma, 0),
)

}

HWTEST_DEF_GROUP(nv04_pgraph,
	HWTEST_GROUP(scan),
)
