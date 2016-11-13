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
#include <initializer_list>

static int test_scan_debug(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type == 4) {
		bool is_nv5 = ctx->chipset.chipset >= 5;
		TEST_BITSCAN(0x400080, 0x1337f000, 0);
		TEST_BITSCAN(0x400084, is_nv5 ? 0xf2ffb701 : 0x72113101, 0);
		TEST_BITSCAN(0x400088, 0x11d7fff1, 0);
		TEST_BITSCAN(0x40008c, is_nv5 ? 0xfbffff73 : 0x11ffff33, 0);
	} else if (ctx->chipset.card_type == 0x10) {
		bool is_nv11p = ctx->chipset.chipset > 0x10;
		TEST_BITSCAN(0x400080, 0x0003ffff, 0);
		TEST_BITSCAN(0x400084, is_nv11p ? 0xfe71f701 : 0xfe11f701, 0);
		TEST_BITSCAN(0x400088, 0xffffffff, 0);
		TEST_BITSCAN(0x40008c, is_nv11p ? 0xffffff78 : 0xfffffc70, 0);
		TEST_BITSCAN(0x400090, 0x00ffffff, 0);
	}
	return HWTEST_RES_PASS;
}

static int test_scan_control(struct hwtest_ctx *ctx) {
	bool is_nv5 = ctx->chipset.chipset >= 5;
	bool is_nv11p = ctx->chipset.chipset > 0x10;
	uint32_t ctxs_mask, ctxc_mask;
	uint32_t offset_mask;
	nva_wr32(ctx->cnum, 0x400720, 0);
	if (ctx->chipset.card_type < 0x10) {
		ctxs_mask = ctxc_mask = is_nv5 ? 0x7f73f0ff : 0x0303f0ff;
		offset_mask = is_nv5 ? 0x01fffff0 : 0x00fffff0;
		TEST_BITSCAN(0x400104, 0x00007800, 0);
		TEST_BITSCAN(0x400140, 0x00011311, 0);
		TEST_BITSCAN(0x400170, 0x11010103, 0);
		TEST_BITSCAN(0x400174, 0x0f00e000, 0);
		TEST_BITSCAN(0x400160, ctxs_mask, 0);
		TEST_BITSCAN(0x400164, 0xffff3f03, 0);
		TEST_BITSCAN(0x400168, 0xffffffff, 0);
		TEST_BITSCAN(0x40016c, 0x0000ffff, 0);
		TEST_BITSCAN(0x400610, 0xe0000000 | offset_mask, 0);
		TEST_BITSCAN(0x400614, 0xc0000000 | offset_mask, 0);
		nva_wr32(ctx->cnum, 0x400750, 0);
		nva_wr32(ctx->cnum, 0x400754, 0);
		TEST_BITSCAN(0x400750, 0x00000077, 0);
		TEST_BITSCAN(0x400754, is_nv5 ? 0x003fffff : 0x000fffff, 0);
		TEST_BITSCAN(0x400758, 0xffffffff, 0);
		TEST_BITSCAN(0x400720, 0x1, 0);
		for (int i = 0 ; i < 8; i++) {
			TEST_BITSCAN(0x400180 + i * 4, ctxc_mask, 0);
			TEST_BITSCAN(0x4001a0 + i * 4, 0xffff3f03, 0);
			TEST_BITSCAN(0x4001c0 + i * 4, 0xffffffff, 0);
			TEST_BITSCAN(0x4001e0 + i * 4, 0x0000ffff, 0);
		}
		for (int i = 0 ; i < 4; i++) {
			TEST_BITSCAN(0x400730 + i * 4, 0x00007fff, 0);
			TEST_BITSCAN(0x400740 + i * 4, 0xffffffff, 0);
		}
	} else {
		ctxs_mask = is_nv11p ? 0x7ffff0ff : 0x7fb3f0ff;
		ctxc_mask = is_nv11p ? 0x7ffff0ff : 0x7f33f0ff;
		offset_mask = 0x07fffff0;
		TEST_BITSCAN(0x400104, 0x07800000, 0);
		TEST_BITSCAN(0x400140, 0x01113711, 0);
		TEST_BITSCAN(0x400144, 0x11010103, 0);
		TEST_BITSCAN(0x400148, is_nv11p ? 0x9f00e000 : 0x1f00e000, 0);
		TEST_BITSCAN(0x40014c, ctxs_mask, 0);
		TEST_BITSCAN(0x400150, 0xffff3f03, 0);
		TEST_BITSCAN(0x400154, 0xffffffff, 0);
		TEST_BITSCAN(0x400158, 0x0000ffff, 0);
		TEST_BITSCAN(0x40015c, 0xffffffff, 0);
		TEST_BITSCAN(0x400610, 0xf8000000 | offset_mask, 0);
		TEST_BITSCAN(0x400614, 0xc0000000 | offset_mask, 0);
		TEST_BITSCAN(0x40077c, is_nv11p ? 0x631fffff : 0x7100ffff, 0);
		nva_wr32(ctx->cnum, 0x400760, 0);
		nva_wr32(ctx->cnum, 0x400764, 0);
		TEST_BITSCAN(0x400760, 0x00000077, 0);
		TEST_BITSCAN(0x400764, 0x3ff71ffc, 0);
		TEST_BITSCAN(0x400768, 0xffffffff, 0);
		TEST_BITSCAN(0x40076c, 0xffffffff, 0);
		TEST_BITSCAN(0x400720, 0x1, 0);
		for (int i = 0 ; i < 8; i++) {
			TEST_BITSCAN(0x400160 + i * 4, ctxc_mask, 0);
			TEST_BITSCAN(0x400180 + i * 4, 0xffff3f03, 0);
			TEST_BITSCAN(0x4001a0 + i * 4, 0xffffffff, 0);
			TEST_BITSCAN(0x4001c0 + i * 4, 0x0000ffff, 0);
			TEST_BITSCAN(0x4001e0 + i * 4, 0xffffffff, 0);
		}
		for (int i = 0 ; i < 4; i++) {
			TEST_BITSCAN(0x400730 + i * 4, 0x01171ffc, 0);
			TEST_BITSCAN(0x400740 + i * 4, 0xffffffff, 0);
			TEST_BITSCAN(0x400750 + i * 4, 0xffffffff, 0);
		}
	}
	return HWTEST_RES_PASS;
}

static int test_scan_vtx(struct hwtest_ctx *ctx) {
	int i;
	if (ctx->chipset.card_type <= 4) {
		for (i = 0 ; i < 32; i++) {
			TEST_BITSCAN(0x400400 + i * 4, 0xffffffff, 0);
			TEST_BITSCAN(0x400480 + i * 4, 0xffffffff, 0);
		}
		for (i = 0 ; i < 16; i++) {
			TEST_BITSCAN(0x400d00 + i * 4, 0xffffffc0, 0);
			TEST_BITSCAN(0x400d40 + i * 4, 0xffffffc0, 0);
			TEST_BITSCAN(0x400d80 + i * 4, 0xffffffc0, 0);
		}
	} else {
		for (i = 0 ; i < 10; i++) {
			TEST_BITSCAN(0x400400 + i * 4, 0xffffffff, 0);
			TEST_BITSCAN(0x400480 + i * 4, 0xffffffff, 0);
		}
	}
	return HWTEST_RES_PASS;
}

static int test_scan_d3d(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type >= 0x10)
		return HWTEST_RES_NA;
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
	if (ctx->chipset.card_type >= 0x10) {
		for (i = 0; i < 1000; i++) {
			int idx = jrand48(ctx->rand48) & 1;
			uint32_t v0 = jrand48(ctx->rand48);
			uint32_t v1 = jrand48(ctx->rand48);
			uint32_t v2 = jrand48(ctx->rand48);
			nva_wr32(ctx->cnum, 0x400550 + idx * 4, v0);
			nva_wr32(ctx->cnum, 0x400558 + idx * 4, v1);
			v0 &= 0xffff;
			v1 &= 0x3ffff;
			TEST_READ(0x400550 + idx * 4, v0, "v0 %08x v1 %08x", v0, v1);
			TEST_READ(0x400558 + idx * 4, v1, "v0 %08x v1 %08x", v0, v1);
			if (jrand48(ctx->rand48) & 1) {
				nva_wr32(ctx->cnum, 0x400550 + idx * 4, v2);
			} else {
				nva_wr32(ctx->cnum, 0x400558 + idx * 4, v2);
			}
			v1 &= 0xffff;
			v2 &= 0x3ffff;
			TEST_READ(0x400550 + idx * 4, v1, "v0 %08x v1 %08x v2 %08x", v0, v1, v2);
			TEST_READ(0x400558 + idx * 4, v2, "v0 %08x v1 %08x v2 %08x", v0, v1, v2);
		}
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
	uint32_t offset_mask;
	uint32_t pitch_mask;
	if (ctx->chipset.card_type < 0x10) {
		offset_mask = ctx->chipset.chipset >= 5 ? 0x01fffff0 : 0x00fffff0;
		pitch_mask = 0x1ff0;
	} else {
		offset_mask = 0x07fffff0;
		pitch_mask = 0xfff0;
	}
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
	TEST_BITSCAN(0x400670, pitch_mask, 0);
	TEST_BITSCAN(0x400674, pitch_mask, 0);
	TEST_BITSCAN(0x400678, pitch_mask, 0);
	TEST_BITSCAN(0x40067c, pitch_mask, 0);
	TEST_BITSCAN(0x400680, pitch_mask, 0);
	TEST_BITSCAN(0x400684, 0x8000000f | offset_mask, 0xf);
	TEST_BITSCAN(0x400688, 0x8000000f | offset_mask, 0xf);
	TEST_BITSCAN(0x40068c, 0x8000000f | offset_mask, 0xf);
	TEST_BITSCAN(0x400690, 0x8000000f | offset_mask, 0xf);
	TEST_BITSCAN(0x400694, 0x8000000f | offset_mask, 0xf);
	TEST_BITSCAN(0x400698, 0x8000000f | offset_mask, 0xf);
	TEST_BITSCAN(0x40069c, 0x0f0f0000, 0);
	TEST_BITSCAN(0x4006a0, 0x0f0f0000, 0);
	if (ctx->chipset.card_type < 0x10) {
		TEST_BITSCAN(0x40070c, 0x00000003, 0);
		TEST_BITSCAN(0x400710, 0x0f731f3f, 0);
		TEST_BITSCAN(0x400714, 0x00110101, 0);
	} else {
		bool is_nv11p = ctx->chipset.chipset > 0x10;
		TEST_BITSCAN(0x400710, is_nv11p ? 0x77777713 : 3, 0);
		TEST_BITSCAN(0x400714, 0x0f731f3f, 0);
		TEST_BITSCAN(0x400718, 0x73110101, 0);
	}
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
	if (ctx->chipset.card_type < 0x10) {
		TEST_BITSCAN(0x400578, 0x1fffffff, 0);
	} else {
		TEST_BITSCAN(0x400578, 0xdfffffff, 0);
	}
	TEST_BITSCAN(0x40050c, 0xffffffff, 0);
	TEST_BITSCAN(0x400510, 0x00ffffff, 0);
	if (ctx->chipset.card_type < 0x10) {
		TEST_BITSCAN(0x400514, 0xf013ffff, 0);
	} else {
		TEST_BITSCAN(0x400514, 0xf113ffff, 0);
	}
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
	if (ctx->chipset.card_type < 0x10) {
		TEST_BITSCAN(0x400760, 0xffffffff, 0);
		TEST_BITSCAN(0x400764, 0x0000033f, 0);
		TEST_BITSCAN(0x400768, 0x01030000, 0);
	} else {
		TEST_BITSCAN(0x400770, 0xffffffff, 0);
		TEST_BITSCAN(0x400774, 0x0000033f, 0);
		TEST_BITSCAN(0x400778, 0x01030000, 0);
	}
	if (ctx->chipset.card_type >= 0x10) {
		TEST_BITSCAN(0x400588, 0x0000ffff, 0);
		TEST_BITSCAN(0x40058c, 0x0001ffff, 0);
	}
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

static void nv04_pgraph_gen_state(struct hwtest_ctx *ctx, struct nv04_pgraph_state *state) {
	bool is_nv5 = ctx->chipset.chipset >= 5;
	bool is_nv11p = ctx->chipset.chipset > 0x10;
	state->chipset = ctx->chipset;
	if (ctx->chipset.card_type == 4) {
		state->debug[0] = jrand48(ctx->rand48) & 0x1337f000;
		state->debug[1] = jrand48(ctx->rand48) & (is_nv5 ? 0xf2ffb701 : 0x72113101);
		state->debug[2] = jrand48(ctx->rand48) & 0x11d7fff1;
		state->debug[3] = jrand48(ctx->rand48) & (is_nv5 ? 0xfbffff73 : 0x11ffff33);
	} else if (ctx->chipset.card_type == 0x10) {
		// debug[0] holds only resets?
		state->debug[0] = 0;
		state->debug[1] = jrand48(ctx->rand48) & (is_nv11p ? 0xfe71f701 : 0xfe11f701);
		state->debug[2] = jrand48(ctx->rand48) & 0xffffffff;
		state->debug[3] = jrand48(ctx->rand48) & (is_nv11p ? 0xffffff78 : 0xfffffc70);
		state->debug[4] = jrand48(ctx->rand48) & 0x00ffffff;
	}
	state->status = 0;
	state->intr = 0;
	if (ctx->chipset.card_type == 4) {
		state->intr_en = jrand48(ctx->rand48) & 0x11311;
		state->nstatus = jrand48(ctx->rand48) & 0x7800;
	} else {
		state->intr_en = jrand48(ctx->rand48) & 0x1113711;
		state->nstatus = jrand48(ctx->rand48) & 0x7800000;
	}
	state->nsource = 0;
	uint32_t ctxs_mask, ctxc_mask;
	if (ctx->chipset.chipset < 0x10) {
		ctxs_mask = ctxc_mask = is_nv5 ? 0x7f73f0ff : 0x0303f0ff;
	} else {
		ctxs_mask = is_nv11p ? 0x7ffff0ff : 0x7fb3f0ff;
		ctxc_mask = is_nv11p ? 0x7ffff0ff : 0x7f33f0ff;
	}
	state->ctx_switch[0] = jrand48(ctx->rand48) & ctxs_mask;
	state->ctx_switch[1] = jrand48(ctx->rand48) & 0xffff3f03;
	state->ctx_switch[2] = jrand48(ctx->rand48) & 0xffffffff;
	state->ctx_switch[3] = jrand48(ctx->rand48) & 0x0000ffff;
	state->ctx_switch[4] = jrand48(ctx->rand48);
	for (int i = 0; i < 8; i++) {
		state->ctx_cache[i][0] = jrand48(ctx->rand48) & ctxc_mask;
		state->ctx_cache[i][1] = jrand48(ctx->rand48) & 0xffff3f03;
		state->ctx_cache[i][2] = jrand48(ctx->rand48) & 0xffffffff;
		state->ctx_cache[i][3] = jrand48(ctx->rand48) & 0x0000ffff;
		state->ctx_cache[i][4] = jrand48(ctx->rand48);
	}
	state->ctx_control = jrand48(ctx->rand48) & 0x11010103;
	if (ctx->chipset.card_type == 4) {
		state->ctx_user = jrand48(ctx->rand48) & 0x0f00e000;
		state->unk610 = jrand48(ctx->rand48) & (is_nv5 ? 0xe1fffff0 : 0xe0fffff0);
		state->unk614 = jrand48(ctx->rand48) & (is_nv5 ? 0xc1fffff0 : 0xc0fffff0);
	} else {
		state->ctx_user = jrand48(ctx->rand48) & (is_nv11p ? 0x9f00e000 : 0x1f00e000);
		state->unk610 = jrand48(ctx->rand48) & 0xfffffff0;
		state->unk614 = jrand48(ctx->rand48) & 0xc7fffff0;
		if (!is_nv11p) {
			state->unk77c = jrand48(ctx->rand48) & 0x1100ffff;
			if (state->unk77c & 0x10000000)
				state->unk77c |= 0x70000000;
		} else {
			state->unk77c = jrand48(ctx->rand48) & 0x631fffff;
		}
	}
	state->fifo_enable = jrand48(ctx->rand48) & 1;
	for (int i = 0; i < 4; i++) {
		if (ctx->chipset.chipset < 0x10) {
			state->fifo_mthd[i] = jrand48(ctx->rand48) & 0x00007fff;
		} else {
			state->fifo_mthd[i] = jrand48(ctx->rand48) & 0x01171ffc;
		}
		state->fifo_data[i][0] = jrand48(ctx->rand48) & 0xffffffff;
		state->fifo_data[i][1] = jrand48(ctx->rand48) & 0xffffffff;
	}
	if (state->fifo_enable) {
		state->fifo_ptr = (jrand48(ctx->rand48) & 7) * 0x11;
		if (ctx->chipset.chipset < 0x10) {
			state->fifo_mthd_st2 = jrand48(ctx->rand48) & 0x000ffffe;
		} else {
			state->fifo_mthd_st2 = jrand48(ctx->rand48) & 0x3bf71ffc;
		}
	} else {
		state->fifo_ptr = jrand48(ctx->rand48) & 0x00000077;
		if (ctx->chipset.chipset < 0x10) {
			state->fifo_mthd_st2 = jrand48(ctx->rand48) & 0x000fffff;
		} else {
			state->fifo_mthd_st2 = jrand48(ctx->rand48) & 0x3ff71ffc;
		}
	}
	state->fifo_data_st2[0] = jrand48(ctx->rand48) & 0xffffffff;
	state->fifo_data_st2[1] = jrand48(ctx->rand48) & 0xffffffff;
	for (int i = 0; i < 32; i++) {
		state->vtx_x[i] = jrand48(ctx->rand48);
		state->vtx_y[i] = jrand48(ctx->rand48);
	}
	for (int i = 0; i < 16; i++) {
		state->vtx_u[i] = jrand48(ctx->rand48) & 0xffffffc0;
		state->vtx_v[i] = jrand48(ctx->rand48) & 0xffffffc0;
		state->vtx_m[i] = jrand48(ctx->rand48) & 0xffffffc0;
	}
	for (int i = 0; i < 2; i++) {
		state->iclip[i] = jrand48(ctx->rand48) & 0x3ffff;
		state->uclip_min[i] = jrand48(ctx->rand48) & 0xffff;
		state->oclip_min[i] = jrand48(ctx->rand48) & 0xffff;
		state->clip3d_min[i] = jrand48(ctx->rand48) & 0xffff;
		state->uclip_max[i] = jrand48(ctx->rand48) & 0x3ffff;
		state->oclip_max[i] = jrand48(ctx->rand48) & 0x3ffff;
		state->clip3d_max[i] = jrand48(ctx->rand48) & 0x3ffff;
	}
	if (ctx->chipset.card_type < 0x10) {
		state->xy_misc_0 = jrand48(ctx->rand48) & 0xf013ffff;
	} else {
		state->xy_misc_0 = jrand48(ctx->rand48) & 0xf113ffff;
	}
	state->xy_misc_1[0] = jrand48(ctx->rand48) & 0x00111031;
	state->xy_misc_1[1] = jrand48(ctx->rand48) & 0x00111031;
	state->xy_misc_3 = jrand48(ctx->rand48) & 0x7f7f1111;
	state->xy_misc_4[0] = jrand48(ctx->rand48) & 0x300000ff;
	state->xy_misc_4[1] = jrand48(ctx->rand48) & 0x300000ff;
	state->xy_clip[0][0] = jrand48(ctx->rand48);
	state->xy_clip[0][1] = jrand48(ctx->rand48);
	state->xy_clip[1][0] = jrand48(ctx->rand48);
	state->xy_clip[1][1] = jrand48(ctx->rand48);
	state->valid[0] = jrand48(ctx->rand48) & 0xf07fffff;
	if (ctx->chipset.card_type < 0x10) {
		state->valid[1] = jrand48(ctx->rand48) & 0x1fffffff;
	} else {
		state->valid[1] = jrand48(ctx->rand48) & 0xdfffffff;
	}
	state->misc24[0] = jrand48(ctx->rand48) & 0x00ffffff;
	state->misc24[1] = jrand48(ctx->rand48) & 0x00ffffff;
	state->misc24[2] = jrand48(ctx->rand48) & 0x00ffffff;
	state->misc32[0] = jrand48(ctx->rand48);
	state->misc32[1] = jrand48(ctx->rand48);
	state->misc32[2] = jrand48(ctx->rand48);
	state->misc32[3] = jrand48(ctx->rand48);
	state->dma_pitch = jrand48(ctx->rand48);
	state->dvd_format = jrand48(ctx->rand48) & 0x0000033f;
	state->sifm_mode = jrand48(ctx->rand48) & 0x01030000;
	state->unk588 = jrand48(ctx->rand48) & 0x0000ffff;
	state->unk58c = jrand48(ctx->rand48) & 0x0001ffff;
	state->bitmap_color_0 = jrand48(ctx->rand48);
	state->rop = jrand48(ctx->rand48) & 0xff;
	state->beta = jrand48(ctx->rand48) & 0x7f800000;
	state->beta4 = jrand48(ctx->rand48);
	state->chroma = jrand48(ctx->rand48);
	state->pattern_mono_color[0] = jrand48(ctx->rand48);
	state->pattern_mono_color[1] = jrand48(ctx->rand48);
	state->pattern_mono_bitmap[0] = jrand48(ctx->rand48);
	state->pattern_mono_bitmap[1] = jrand48(ctx->rand48);
	state->pattern_config = jrand48(ctx->rand48) & 0x13;
	for (int i = 0; i< 64; i++)
		state->pattern_color[i] = jrand48(ctx->rand48) & 0xffffff;
	uint32_t offset_mask, pitch_mask;
	if (ctx->chipset.card_type < 0x10) {
		offset_mask = is_nv5 ? 0x01fffff0 : 0x00fffff0;
		pitch_mask = 0x1ff0;
	} else {
		offset_mask = 0x07fffff0;
		pitch_mask = 0xfff0;
	}
	for (int i = 0; i < 6; i++) {
		state->surf_base[i] = jrand48(ctx->rand48) & offset_mask;
		state->surf_offset[i] = jrand48(ctx->rand48) & offset_mask;
		state->surf_limit[i] = (jrand48(ctx->rand48) & (offset_mask | 1 << 31)) | 0xf;
	}
	for (int i = 0; i < 5; i++)
		state->surf_pitch[i] = jrand48(ctx->rand48) & pitch_mask;
	for (int i = 0; i < 2; i++)
		state->surf_swizzle[i] = jrand48(ctx->rand48) & 0x0f0f0000;
	if (!is_nv11p) {
		state->surf_type = jrand48(ctx->rand48) & 3;
	} else {
		state->surf_type = jrand48(ctx->rand48) & 0x77777713;
	}
	state->surf_format = jrand48(ctx->rand48) & 0xffffff;
	state->ctx_valid = jrand48(ctx->rand48) & 0x0f731f3f;
	state->ctx_format = jrand48(ctx->rand48) & 0x3f3f3f3f;
	if (ctx->chipset.card_type < 0x10) {
		state->notify = jrand48(ctx->rand48) & 0x00110101;
	} else {
		state->notify = jrand48(ctx->rand48) & 0x73110101;
	}
	if (ctx->chipset.card_type < 0x10) {
		for (int i = 0; i < 2; i++) {
			state->d3d_rc_alpha[i] = jrand48(ctx->rand48) & 0xfd1d1d1d;
			state->d3d_rc_color[i] = jrand48(ctx->rand48) & 0xff1f1f1f;
			state->d3d_tex_format[i] = jrand48(ctx->rand48) & 0xfffff7a6;
			state->d3d_tex_filter[i] = jrand48(ctx->rand48) & 0xffff9e1e;
		}
		state->d3d_unk5c0 = jrand48(ctx->rand48) & 0xffffffff;
		state->d3d_unk5c4 = jrand48(ctx->rand48) & 0xffffffc0;
		state->d3d_unk5c8 = jrand48(ctx->rand48) & 0xffffffc0;
		state->d3d_unk5cc = jrand48(ctx->rand48) & 0xffffffc0;
		state->d3d_unk5d0 = jrand48(ctx->rand48) & 0xffffffc0;
		state->d3d_unk5d4 = jrand48(ctx->rand48) & 0xffffffff;
		state->d3d_unk5d8 = jrand48(ctx->rand48) & 0xffffffff;
		state->d3d_unk5dc = jrand48(ctx->rand48) & 0xffffffff;
		state->d3d_unk5e0 = jrand48(ctx->rand48) & 0xffffffc0;
		state->d3d_config = jrand48(ctx->rand48) & 0xffff5fff;
		state->d3d_stencil_func = jrand48(ctx->rand48) & 0xfffffff1;
		state->d3d_stencil_op = jrand48(ctx->rand48) & 0x00000fff;
		state->d3d_blend = jrand48(ctx->rand48) & 0xff1111ff;
	}
	state->dma_offset[0] = jrand48(ctx->rand48);
	state->dma_offset[1] = jrand48(ctx->rand48);
	state->dma_length = jrand48(ctx->rand48) & 0x003fffff;
	state->dma_misc = jrand48(ctx->rand48) & 0x0077ffff;
	state->dma_unk20[0] = jrand48(ctx->rand48);
	state->dma_unk20[1] = jrand48(ctx->rand48);
	for (int i = 0; i < 2; i++) {
		state->dma_eng_inst[i] = jrand48(ctx->rand48) & 0x0000ffff;
		state->dma_eng_flags[i] = jrand48(ctx->rand48) & 0xfff33000;
		state->dma_eng_limit[i] = jrand48(ctx->rand48);
		state->dma_eng_pte[i] = jrand48(ctx->rand48) & 0xfffff002;
		state->dma_eng_pte_tag[i] = jrand48(ctx->rand48) & 0xfffff000;
		state->dma_eng_addr_virt_adj[i] = jrand48(ctx->rand48);
		state->dma_eng_addr_phys[i] = jrand48(ctx->rand48);
		state->dma_eng_bytes[i] = jrand48(ctx->rand48) & 0x01ffffff;
		state->dma_eng_lines[i] = jrand48(ctx->rand48) & 0x000007ff;
	}
}

static void nv04_pgraph_load_state(struct hwtest_ctx *ctx, struct nv04_pgraph_state *state) {
	nva_wr32(ctx->cnum, 0x000200, 0xffffeeff);
	nva_wr32(ctx->cnum, 0x000200, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400720, 0);
	nva_wr32(ctx->cnum, 0x400160, 0);
	for (int i = 0; i < 2; i++) {
		nva_wr32(ctx->cnum, 0x400534 + i * 4, state->iclip[i]);
		nva_wr32(ctx->cnum, 0x40053c + i * 4, state->uclip_min[i]);
		nva_wr32(ctx->cnum, 0x400544 + i * 4, state->uclip_max[i]);
		nva_wr32(ctx->cnum, 0x400560 + i * 4, state->oclip_min[i]);
		nva_wr32(ctx->cnum, 0x400568 + i * 4, state->oclip_max[i]);
		if (ctx->chipset.card_type >= 0x10) {
			nva_wr32(ctx->cnum, 0x400550 + i * 4, state->clip3d_min[i]);
			nva_wr32(ctx->cnum, 0x400558 + i * 4, state->clip3d_max[i]);
		}
	}
	if (ctx->chipset.card_type < 0x10) {
		for (int i = 0; i < 32; i++) {
			nva_wr32(ctx->cnum, 0x400400 + i * 4, state->vtx_x[i]);
			nva_wr32(ctx->cnum, 0x400480 + i * 4, state->vtx_y[i]);
		}
		for (int i = 0; i < 16; i++) {
			nva_wr32(ctx->cnum, 0x400d00 + i * 4, state->vtx_u[i]);
			nva_wr32(ctx->cnum, 0x400d40 + i * 4, state->vtx_v[i]);
			nva_wr32(ctx->cnum, 0x400d80 + i * 4, state->vtx_m[i]);
		}
	} else {
		for (int i = 0; i < 10; i++) {
			nva_wr32(ctx->cnum, 0x400400 + i * 4, state->vtx_x[i]);
			nva_wr32(ctx->cnum, 0x400480 + i * 4, state->vtx_y[i]);
		}
	}
	nva_wr32(ctx->cnum, 0x400514, state->xy_misc_0);
	nva_wr32(ctx->cnum, 0x400518, state->xy_misc_1[0]);
	nva_wr32(ctx->cnum, 0x40051c, state->xy_misc_1[1]);
	nva_wr32(ctx->cnum, 0x400520, state->xy_misc_3);
	nva_wr32(ctx->cnum, 0x400500, state->xy_misc_4[0]);
	nva_wr32(ctx->cnum, 0x400504, state->xy_misc_4[1]);
	nva_wr32(ctx->cnum, 0x400524, state->xy_clip[0][0]);
	nva_wr32(ctx->cnum, 0x400528, state->xy_clip[0][1]);
	nva_wr32(ctx->cnum, 0x40052c, state->xy_clip[1][0]);
	nva_wr32(ctx->cnum, 0x400530, state->xy_clip[1][1]);
	nva_wr32(ctx->cnum, 0x400510, state->misc24[0]);
	nva_wr32(ctx->cnum, 0x400570, state->misc24[1]);
	nva_wr32(ctx->cnum, 0x400574, state->misc24[2]);
	nva_wr32(ctx->cnum, 0x40050c, state->misc32[0]);
	nva_wr32(ctx->cnum, 0x40057c, state->misc32[1]);
	nva_wr32(ctx->cnum, 0x400580, state->misc32[2]);
	nva_wr32(ctx->cnum, 0x400584, state->misc32[3]);
	if (ctx->chipset.card_type < 0x10) {
		nva_wr32(ctx->cnum, 0x400760, state->dma_pitch);
		nva_wr32(ctx->cnum, 0x400764, state->dvd_format);
		nva_wr32(ctx->cnum, 0x400768, state->sifm_mode);
	} else {
		nva_wr32(ctx->cnum, 0x400770, state->dma_pitch);
		nva_wr32(ctx->cnum, 0x400774, state->dvd_format);
		nva_wr32(ctx->cnum, 0x400778, state->sifm_mode);
		nva_wr32(ctx->cnum, 0x400588, state->unk588);
		nva_wr32(ctx->cnum, 0x40058c, state->unk58c);
	}
	nva_wr32(ctx->cnum, 0x40008c, 0x01000000);
	nva_wr32(ctx->cnum, 0x400600, state->bitmap_color_0);
	nva_wr32(ctx->cnum, 0x400604, state->rop);
	nva_wr32(ctx->cnum, 0x400608, state->beta);
	nva_wr32(ctx->cnum, 0x40060c, state->beta4);
	for (int i = 0; i < 2; i++) {
		nva_wr32(ctx->cnum, 0x400800 + i * 4, state->pattern_mono_color[i]);
		nva_wr32(ctx->cnum, 0x400808 + i * 4, state->pattern_mono_bitmap[i]);
	}
	nva_wr32(ctx->cnum, 0x400810, state->pattern_config);
	nva_wr32(ctx->cnum, 0x400814, state->chroma);
	for (int i = 0; i < 64; i++)
		nva_wr32(ctx->cnum, 0x400900 + i * 4, state->pattern_color[i]);
	for (int i = 0; i < 6; i++) {
		nva_wr32(ctx->cnum, 0x400640 + i * 4, state->surf_offset[i]);
		nva_wr32(ctx->cnum, 0x400658 + i * 4, state->surf_base[i]);
		nva_wr32(ctx->cnum, 0x400684 + i * 4, state->surf_limit[i]);
	}
	for (int i = 0; i < 5; i++)
		nva_wr32(ctx->cnum, 0x400670 + i * 4, state->surf_pitch[i]);
	for (int i = 0; i < 2; i++)
		nva_wr32(ctx->cnum, 0x40069c + i * 4, state->surf_swizzle[i]);
	if (ctx->chipset.card_type < 0x10) {
		nva_wr32(ctx->cnum, 0x40070c, state->surf_type);
		nva_wr32(ctx->cnum, 0x400710, state->ctx_valid);
		nva_wr32(ctx->cnum, 0x400714, state->notify);
	} else {
		nva_wr32(ctx->cnum, 0x400710, state->surf_type);
		nva_wr32(ctx->cnum, 0x400714, state->ctx_valid);
		nva_wr32(ctx->cnum, 0x400718, state->notify);
	}
	nva_wr32(ctx->cnum, 0x400724, state->surf_format);
	nva_wr32(ctx->cnum, 0x400830, state->ctx_format);
	if (ctx->chipset.card_type < 0x10) {
		for (int i = 0; i < 2; i++) {
			nva_wr32(ctx->cnum, 0x400590 + i * 8, state->d3d_rc_alpha[i]);
			nva_wr32(ctx->cnum, 0x400594 + i * 8, state->d3d_rc_color[i]);
			nva_wr32(ctx->cnum, 0x4005a8 + i * 4, state->d3d_tex_format[i]);
			nva_wr32(ctx->cnum, 0x4005b0 + i * 4, state->d3d_tex_filter[i]);
		}
		nva_wr32(ctx->cnum, 0x4005c0, state->d3d_unk5c0);
		nva_wr32(ctx->cnum, 0x4005c4, state->d3d_unk5c4);
		nva_wr32(ctx->cnum, 0x4005c8, state->d3d_unk5c8);
		nva_wr32(ctx->cnum, 0x4005cc, state->d3d_unk5cc);
		nva_wr32(ctx->cnum, 0x4005d0, state->d3d_unk5d0);
		nva_wr32(ctx->cnum, 0x4005d4, state->d3d_unk5d4);
		nva_wr32(ctx->cnum, 0x4005d8, state->d3d_unk5d8);
		nva_wr32(ctx->cnum, 0x4005dc, state->d3d_unk5dc);
		nva_wr32(ctx->cnum, 0x4005e0, state->d3d_unk5e0);
		nva_wr32(ctx->cnum, 0x400818, state->d3d_config);
		nva_wr32(ctx->cnum, 0x40081c, state->d3d_stencil_func);
		nva_wr32(ctx->cnum, 0x400820, state->d3d_stencil_op);
		nva_wr32(ctx->cnum, 0x400824, state->d3d_blend);
	}
	nva_wr32(ctx->cnum, 0x401000, state->dma_offset[0]);
	nva_wr32(ctx->cnum, 0x401004, state->dma_offset[1]);
	nva_wr32(ctx->cnum, 0x401008, state->dma_length);
	nva_wr32(ctx->cnum, 0x40100c, state->dma_misc);
	nva_wr32(ctx->cnum, 0x401020, state->dma_unk20[0]);
	nva_wr32(ctx->cnum, 0x401024, state->dma_unk20[1]);
	for (int i = 0; i < 2; i++) {
		nva_wr32(ctx->cnum, 0x401040 + i * 0x40, state->dma_eng_inst[i]);
		nva_wr32(ctx->cnum, 0x401044 + i * 0x40, state->dma_eng_flags[i]);
		nva_wr32(ctx->cnum, 0x401048 + i * 0x40, state->dma_eng_limit[i]);
		nva_wr32(ctx->cnum, 0x40104c + i * 0x40, state->dma_eng_pte[i]);
		nva_wr32(ctx->cnum, 0x401050 + i * 0x40, state->dma_eng_pte_tag[i]);
		nva_wr32(ctx->cnum, 0x401054 + i * 0x40, state->dma_eng_addr_virt_adj[i]);
		nva_wr32(ctx->cnum, 0x401058 + i * 0x40, state->dma_eng_addr_phys[i]);
		nva_wr32(ctx->cnum, 0x40105c + i * 0x40, state->dma_eng_bytes[i]);
		nva_wr32(ctx->cnum, 0x401060 + i * 0x40, state->dma_eng_lines[i]);
	}
	nva_wr32(ctx->cnum, 0x400100, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400140, state->intr_en);
	nva_wr32(ctx->cnum, 0x400104, state->nstatus);
	if (ctx->chipset.card_type < 0x10) {
		for (int j = 0; j < 4; j++) {
			nva_wr32(ctx->cnum, 0x400160 + j * 4, state->ctx_switch[j]);
			for (int i = 0; i < 8; i++) {
				nva_wr32(ctx->cnum, 0x400180 + i * 4 + j * 0x20, state->ctx_cache[i][j]);
			}
		}
		nva_wr32(ctx->cnum, 0x400170, state->ctx_control);
		nva_wr32(ctx->cnum, 0x400174, state->ctx_user);
	} else {
		for (int j = 0; j < 5; j++) {
			nva_wr32(ctx->cnum, 0x40014c + j * 4, state->ctx_switch[j]);
			for (int i = 0; i < 8; i++) {
				nva_wr32(ctx->cnum, 0x400160 + i * 4 + j * 0x20, state->ctx_cache[i][j]);
			}
		}
		nva_wr32(ctx->cnum, 0x400144, state->ctx_control);
		nva_wr32(ctx->cnum, 0x400148, state->ctx_user);
	}
	nva_wr32(ctx->cnum, 0x400610, state->unk610);
	nva_wr32(ctx->cnum, 0x400614, state->unk614);
	nva_wr32(ctx->cnum, 0x400508, state->valid[0]);
	nva_wr32(ctx->cnum, 0x400578, state->valid[1]);
	nva_wr32(ctx->cnum, 0x400080, state->debug[0]);
	if (ctx->chipset.card_type < 0x10) {
		nva_wr32(ctx->cnum, 0x400084, state->debug[1]);
	} else {
		// fuck you
		uint32_t mangled = state->debug[1] & 0x3fffffff;
		if (state->debug[1] & 1 << 30)
			mangled |= 1 << 31;
		if (state->debug[1] & 1 << 31)
			mangled |= 1 << 30;
		nva_wr32(ctx->cnum, 0x400084, mangled);
	}
	nva_wr32(ctx->cnum, 0x400088, state->debug[2]);
	nva_wr32(ctx->cnum, 0x40008c, state->debug[3]);
	if (ctx->chipset.card_type >= 0x10)
		nva_wr32(ctx->cnum, 0x400090, state->debug[4]);
	if (ctx->chipset.card_type >= 0x10)
		nva_wr32(ctx->cnum, 0x40077c, state->unk77c);
	if (ctx->chipset.card_type < 0x10) {
		for (int i = 0; i < 4; i++) {
			nva_wr32(ctx->cnum, 0x400730 + i * 4, state->fifo_mthd[i]);
			nva_wr32(ctx->cnum, 0x400740 + i * 4, state->fifo_data[i][0]);
		}
		nva_wr32(ctx->cnum, 0x400750, state->fifo_ptr);
		nva_wr32(ctx->cnum, 0x400754, state->fifo_mthd_st2);
		nva_wr32(ctx->cnum, 0x400758, state->fifo_data_st2[0]);
	} else {
		for (int i = 0; i < 4; i++) {
			nva_wr32(ctx->cnum, 0x400730 + i * 4, state->fifo_mthd[i]);
			nva_wr32(ctx->cnum, 0x400740 + i * 4, state->fifo_data[i][0]);
			nva_wr32(ctx->cnum, 0x400750 + i * 4, state->fifo_data[i][1]);
		}
		nva_wr32(ctx->cnum, 0x400760, state->fifo_ptr);
		nva_wr32(ctx->cnum, 0x400764, state->fifo_mthd_st2);
		nva_wr32(ctx->cnum, 0x400768, state->fifo_data_st2[0]);
		nva_wr32(ctx->cnum, 0x40076c, state->fifo_data_st2[1]);
	}
	nva_wr32(ctx->cnum, 0x400720, state->fifo_enable);
}

static void nv04_pgraph_dump_state(struct hwtest_ctx *ctx, struct nv04_pgraph_state *state) {
	int ctr = 0;
	state->chipset = ctx->chipset;
	while ((state->status = nva_rd32(ctx->cnum, 0x400700))) {
		ctr++;
		if (ctr > 100000) {
			fprintf(stderr, "PGRAPH locked up [%08x]!\n", state->status);
			break;
		}
	}
	state->debug[0] = nva_rd32(ctx->cnum, 0x400080);
	state->debug[1] = nva_rd32(ctx->cnum, 0x400084);
	state->debug[2] = nva_rd32(ctx->cnum, 0x400088);
	state->debug[3] = nva_rd32(ctx->cnum, 0x40008c);
	if (ctx->chipset.card_type >= 0x10) {
		nva_wr32(ctx->cnum, 0x400080, 0);
		state->debug[4] = nva_rd32(ctx->cnum, 0x400090);
	}
	state->unk610 = nva_rd32(ctx->cnum, 0x400610);
	state->unk614 = nva_rd32(ctx->cnum, 0x400614);
	if (ctx->chipset.card_type >= 0x10)
		state->unk77c = nva_rd32(ctx->cnum, 0x40077c);
	state->fifo_enable = nva_rd32(ctx->cnum, 0x400720);
	nva_wr32(ctx->cnum, 0x400720, 0);
	if (ctx->chipset.card_type < 0x10) {
		for (int i = 0; i < 4; i++) {
			state->fifo_mthd[i] = nva_rd32(ctx->cnum, 0x400730 + i * 4);
			state->fifo_data[i][0] = nva_rd32(ctx->cnum, 0x400740 + i * 4);
		}
		state->fifo_ptr = nva_rd32(ctx->cnum, 0x400750);
		state->fifo_mthd_st2 = nva_rd32(ctx->cnum, 0x400754);
		state->fifo_data_st2[0] = nva_rd32(ctx->cnum, 0x400758);
	} else {
		for (int i = 0; i < 4; i++) {
			state->fifo_mthd[i] = nva_rd32(ctx->cnum, 0x400730 + i * 4);
			state->fifo_data[i][0] = nva_rd32(ctx->cnum, 0x400740 + i * 4);
			state->fifo_data[i][1] = nva_rd32(ctx->cnum, 0x400750 + i * 4);
		}
		state->fifo_ptr = nva_rd32(ctx->cnum, 0x400760);
		state->fifo_mthd_st2 = nva_rd32(ctx->cnum, 0x400764);
		state->fifo_data_st2[0] = nva_rd32(ctx->cnum, 0x400768);
		state->fifo_data_st2[1] = nva_rd32(ctx->cnum, 0x40076c);
	}
	state->intr = nva_rd32(ctx->cnum, 0x400100);
	state->intr_en = nva_rd32(ctx->cnum, 0x400140);
	state->nstatus = nva_rd32(ctx->cnum, 0x400104);
	state->nsource = nva_rd32(ctx->cnum, 0x400108);
	if (ctx->chipset.card_type < 0x10) {
		for (int j = 0; j < 4; j++) {
			state->ctx_switch[j] = nva_rd32(ctx->cnum, 0x400160 + j * 4);
			for (int i = 0; i < 8; i++) {
				state->ctx_cache[i][j] = nva_rd32(ctx->cnum, 0x400180 + i * 4 + j * 0x20);
			}
		}
		// eh
		state->ctx_control = nva_rd32(ctx->cnum, 0x400170) & ~0x100000;
		state->ctx_user = nva_rd32(ctx->cnum, 0x400174);
	} else {
		for (int j = 0; j < 5; j++) {
			state->ctx_switch[j] = nva_rd32(ctx->cnum, 0x40014c + j * 4);
			for (int i = 0; i < 8; i++) {
				state->ctx_cache[i][j] = nva_rd32(ctx->cnum, 0x400160 + i * 4 + j * 0x20);
			}
		}
		// eh
		state->ctx_control = nva_rd32(ctx->cnum, 0x400144) & ~0x100000;
		state->ctx_user = nva_rd32(ctx->cnum, 0x400148);
	}
	state->valid[0] = nva_rd32(ctx->cnum, 0x400508);
	state->valid[1] = nva_rd32(ctx->cnum, 0x400578);
	state->xy_misc_0 = nva_rd32(ctx->cnum, 0x400514);
	state->xy_misc_1[0] = nva_rd32(ctx->cnum, 0x400518);
	state->xy_misc_1[1] = nva_rd32(ctx->cnum, 0x40051c);
	state->xy_misc_3 = nva_rd32(ctx->cnum, 0x400520);
	state->xy_misc_4[0] = nva_rd32(ctx->cnum, 0x400500);
	state->xy_misc_4[1] = nva_rd32(ctx->cnum, 0x400504);
	state->xy_clip[0][0] = nva_rd32(ctx->cnum, 0x400524);
	state->xy_clip[0][1] = nva_rd32(ctx->cnum, 0x400528);
	state->xy_clip[1][0] = nva_rd32(ctx->cnum, 0x40052c);
	state->xy_clip[1][1] = nva_rd32(ctx->cnum, 0x400530);
	state->misc24[0] = nva_rd32(ctx->cnum, 0x400510);
	state->misc24[1] = nva_rd32(ctx->cnum, 0x400570);
	state->misc24[2] = nva_rd32(ctx->cnum, 0x400574);
	state->misc32[0] = nva_rd32(ctx->cnum, 0x40050c);
	state->misc32[1] = nva_rd32(ctx->cnum, 0x40057c);
	state->misc32[2] = nva_rd32(ctx->cnum, 0x400580);
	state->misc32[3] = nva_rd32(ctx->cnum, 0x400584);
	if (ctx->chipset.card_type < 0x10) {
		state->dma_pitch = nva_rd32(ctx->cnum, 0x400760);
		state->dvd_format = nva_rd32(ctx->cnum, 0x400764);
		state->sifm_mode = nva_rd32(ctx->cnum, 0x400768);
	} else {
		state->dma_pitch = nva_rd32(ctx->cnum, 0x400770);
		state->dvd_format = nva_rd32(ctx->cnum, 0x400774);
		state->sifm_mode = nva_rd32(ctx->cnum, 0x400778);
		state->unk588 = nva_rd32(ctx->cnum, 0x400588);
		state->unk58c = nva_rd32(ctx->cnum, 0x40058c);
	}
	nva_wr32(ctx->cnum, 0x40008c, 0x01000000);
	state->bitmap_color_0 = nva_rd32(ctx->cnum, 0x400600);
	state->rop = nva_rd32(ctx->cnum, 0x400604);
	state->beta = nva_rd32(ctx->cnum, 0x400608);
	state->beta4 = nva_rd32(ctx->cnum, 0x40060c);
	for (int i = 0; i < 2; i++) {
		state->pattern_mono_color[i] = nva_rd32(ctx->cnum, 0x400800 + i * 4);
		state->pattern_mono_bitmap[i] = nva_rd32(ctx->cnum, 0x400808 + i * 4);
	}
	state->pattern_config = nva_rd32(ctx->cnum, 0x400810);
	state->chroma = nva_rd32(ctx->cnum, 0x400814);
	for (int i = 0; i < 64; i++)
		state->pattern_color[i] = nva_rd32(ctx->cnum, 0x400900 + i * 4);
	for (int i = 0; i < 6; i++) {
		state->surf_offset[i] = nva_rd32(ctx->cnum, 0x400640 + i * 4);
		state->surf_base[i] = nva_rd32(ctx->cnum, 0x400658 + i * 4);
		state->surf_limit[i] = nva_rd32(ctx->cnum, 0x400684 + i * 4);
	}
	for (int i = 0; i < 5; i++)
		state->surf_pitch[i] = nva_rd32(ctx->cnum, 0x400670 + i * 4);
	for (int i = 0; i < 2; i++)
		state->surf_swizzle[i] = nva_rd32(ctx->cnum, 0x40069c + i * 4);
	if (ctx->chipset.card_type < 0x10) {
		state->surf_type = nva_rd32(ctx->cnum, 0x40070c);
		state->ctx_valid = nva_rd32(ctx->cnum, 0x400710);
		state->notify = nva_rd32(ctx->cnum, 0x400714);
	} else {
		state->surf_type = nva_rd32(ctx->cnum, 0x400710);
		state->ctx_valid = nva_rd32(ctx->cnum, 0x400714);
		state->notify = nva_rd32(ctx->cnum, 0x400718);
	}
	state->surf_format = nva_rd32(ctx->cnum, 0x400724);
	state->ctx_format = nva_rd32(ctx->cnum, 0x400830);
	if (state->chipset.card_type < 0x10) {
		for (int i = 0; i < 32; i++) {
			state->vtx_x[i] = nva_rd32(ctx->cnum, 0x400400 + i * 4);
			state->vtx_y[i] = nva_rd32(ctx->cnum, 0x400480 + i * 4);
		}
		for (int i = 0; i < 16; i++) {
			state->vtx_u[i] = nva_rd32(ctx->cnum, 0x400d00 + i * 4);
			state->vtx_v[i] = nva_rd32(ctx->cnum, 0x400d40 + i * 4);
			state->vtx_m[i] = nva_rd32(ctx->cnum, 0x400d80 + i * 4);
		}
	} else {
		for (int i = 0; i < 10; i++) {
			state->vtx_x[i] = nva_rd32(ctx->cnum, 0x400400 + i * 4);
			state->vtx_y[i] = nva_rd32(ctx->cnum, 0x400480 + i * 4);
		}
	}
	for (int i = 0; i < 2; i++) {
		state->iclip[i] = nva_rd32(ctx->cnum, 0x400534 + i * 4);
		state->uclip_min[i] = nva_rd32(ctx->cnum, 0x40053c + i * 4);
		state->uclip_max[i] = nva_rd32(ctx->cnum, 0x400544 + i * 4);
		state->oclip_min[i] = nva_rd32(ctx->cnum, 0x400560 + i * 4);
		state->oclip_max[i] = nva_rd32(ctx->cnum, 0x400568 + i * 4);
		if (ctx->chipset.card_type >= 0x10) {
			state->clip3d_min[i] = nva_rd32(ctx->cnum, 0x400550 + i * 4);
			state->clip3d_max[i] = nva_rd32(ctx->cnum, 0x400558 + i * 4);
		}
	}
	if (ctx->chipset.card_type < 0x10) {
		for (int i = 0; i < 2; i++) {
			state->d3d_rc_alpha[i] = nva_rd32(ctx->cnum, 0x400590 + i * 8);
			state->d3d_rc_color[i] = nva_rd32(ctx->cnum, 0x400594 + i * 8);
			state->d3d_tex_format[i] = nva_rd32(ctx->cnum, 0x4005a8 + i * 4);
			state->d3d_tex_filter[i] = nva_rd32(ctx->cnum, 0x4005b0 + i * 4);
		}
		state->d3d_unk5c0 = nva_rd32(ctx->cnum, 0x4005c0);
		state->d3d_unk5c4 = nva_rd32(ctx->cnum, 0x4005c4);
		state->d3d_unk5c8 = nva_rd32(ctx->cnum, 0x4005c8);
		state->d3d_unk5cc = nva_rd32(ctx->cnum, 0x4005cc);
		state->d3d_unk5d0 = nva_rd32(ctx->cnum, 0x4005d0);
		state->d3d_unk5d4 = nva_rd32(ctx->cnum, 0x4005d4);
		state->d3d_unk5d8 = nva_rd32(ctx->cnum, 0x4005d8);
		state->d3d_unk5dc = nva_rd32(ctx->cnum, 0x4005dc);
		state->d3d_unk5e0 = nva_rd32(ctx->cnum, 0x4005e0);
		state->d3d_config = nva_rd32(ctx->cnum, 0x400818);
		state->d3d_stencil_func = nva_rd32(ctx->cnum, 0x40081c);
		state->d3d_stencil_op = nva_rd32(ctx->cnum, 0x400820);
		state->d3d_blend = nva_rd32(ctx->cnum, 0x400824);
	}
	state->dma_offset[0] = nva_rd32(ctx->cnum, 0x401000);
	state->dma_offset[1] = nva_rd32(ctx->cnum, 0x401004);
	state->dma_length = nva_rd32(ctx->cnum, 0x401008);
	state->dma_misc = nva_rd32(ctx->cnum, 0x40100c);
	state->dma_unk20[0] = nva_rd32(ctx->cnum, 0x401020);
	state->dma_unk20[1] = nva_rd32(ctx->cnum, 0x401024);
	for (int i = 0; i < 2; i++) {
		state->dma_eng_inst[i] = nva_rd32(ctx->cnum, 0x401040 + i * 0x40);
		state->dma_eng_flags[i] = nva_rd32(ctx->cnum, 0x401044 + i * 0x40);
		state->dma_eng_limit[i] = nva_rd32(ctx->cnum, 0x401048 + i * 0x40);
		state->dma_eng_pte[i] = nva_rd32(ctx->cnum, 0x40104c + i * 0x40);
		state->dma_eng_pte_tag[i] = nva_rd32(ctx->cnum, 0x401050 + i * 0x40);
		state->dma_eng_addr_virt_adj[i] = nva_rd32(ctx->cnum, 0x401054 + i * 0x40);
		state->dma_eng_addr_phys[i] = nva_rd32(ctx->cnum, 0x401058 + i * 0x40);
		state->dma_eng_bytes[i] = nva_rd32(ctx->cnum, 0x40105c + i * 0x40);
		state->dma_eng_lines[i] = nva_rd32(ctx->cnum, 0x401060 + i * 0x40);
	}
}

static int nv04_pgraph_cmp_state(struct nv04_pgraph_state *orig, struct nv04_pgraph_state *exp, struct nv04_pgraph_state *real, bool broke = false) {
	bool print = false;
#define CMP(reg, name, ...) \
	if (print) \
		printf("%08x %08x %08x " name " %s\n", \
			orig->reg, exp->reg, real->reg , \
			## __VA_ARGS__, (exp->reg == real->reg ? "" : "*")); \
	else if (exp->reg != real->reg) { \
		printf("Difference in reg " name ": expected %08x real %08x\n" , \
		## __VA_ARGS__, exp->reg, real->reg); \
		broke = true; \
	}
restart:
	CMP(status, "STATUS")
	CMP(debug[0], "DEBUG[0]")
	CMP(debug[1], "DEBUG[1]")
	CMP(debug[2], "DEBUG[2]")
	CMP(debug[3], "DEBUG[3]")
	if (orig->chipset.card_type >= 0x10) {
		CMP(debug[4], "DEBUG[4]")
	}
	CMP(intr, "INTR")
	CMP(intr_en, "INTR_EN")
	CMP(nstatus, "NSTATUS")
	CMP(nsource, "NSOURCE")
	int ctx_num;
	if (orig->chipset.card_type < 0x10) {
		ctx_num = 4;
	} else {
		ctx_num = 5;
	}
	for (int i = 0; i < ctx_num; i++) {
		CMP(ctx_switch[i], "CTX_SWITCH[%d]", i)
	}
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < ctx_num; j++)
		CMP(ctx_cache[i][j], "CTX_CACHE[%d][%d]", i, j)
	}
	if (print || (exp->ctx_control & ~0x100) != (real->ctx_control & ~0x100)) {
		CMP(ctx_control, "CTX_CONTROL")
	}
	CMP(ctx_user, "CTX_USER")
	CMP(unk610, "UNK610")
	CMP(unk614, "UNK614")
	if (orig->chipset.card_type >= 0x10) {
		CMP(unk77c, "UNK77C")
	}
	CMP(fifo_enable, "FIFO_ENABLE")
	for (int i = 0; i < 4; i++) {
		CMP(fifo_mthd[i], "FIFO_MTHD[%d]", i)
		CMP(fifo_data[i][0], "FIFO_DATA[%d][0]", i)
		if (orig->chipset.card_type >= 0x10) {
			CMP(fifo_data[i][1], "FIFO_DATA[%d][1]", i)
		}
	}
	CMP(fifo_ptr, "FIFO_PTR")
	CMP(fifo_mthd_st2, "FIFO_MTHD_ST2")
	CMP(fifo_data_st2[0], "FIFO_DATA_ST2[0]")
	if (orig->chipset.card_type >= 0x10) {
		CMP(fifo_data_st2[1], "FIFO_DATA_ST2[1]")
	}
	CMP(xy_misc_0, "XY_MISC_0")
	CMP(xy_misc_1[0], "XY_MISC_1[0]")
	CMP(xy_misc_1[1], "XY_MISC_1[1]")
	CMP(xy_misc_3, "XY_MISC_3")
	CMP(xy_misc_4[0], "XY_MISC_4[0]")
	CMP(xy_misc_4[1], "XY_MISC_4[1]")
	CMP(xy_clip[0][0], "XY_CLIP[0][0]")
	CMP(xy_clip[0][1], "XY_CLIP[0][1]")
	CMP(xy_clip[1][0], "XY_CLIP[1][0]")
	CMP(xy_clip[1][1], "XY_CLIP[1][1]")
	if (orig->chipset.card_type < 0x10) {
		for (int i = 0; i < 32; i++) {
			CMP(vtx_x[i], "VTX_X[%d]", i)
			CMP(vtx_y[i], "VTX_Y[%d]", i)
		}
		for (int i = 0; i < 16; i++) {
			CMP(vtx_u[i], "VTX_U[%d]", i)
			CMP(vtx_v[i], "VTX_V[%d]", i)
			CMP(vtx_m[i], "VTX_M[%d]", i)
		}
	} else {
		for (int i = 0; i < 10; i++) {
			CMP(vtx_x[i], "VTX_X[%d]", i)
			CMP(vtx_y[i], "VTX_Y[%d]", i)
		}
	}
	for (int i = 0; i < 2; i++) {
		CMP(iclip[i], "ICLIP[%d]", i)
	}
	for (int i = 0; i < 2; i++) {
		CMP(uclip_min[i], "UCLIP_MIN[%d]", i)
		CMP(uclip_max[i], "UCLIP_MAX[%d]", i)
	}
	for (int i = 0; i < 2; i++) {
		CMP(oclip_min[i], "OCLIP_MIN[%d]", i)
		CMP(oclip_max[i], "OCLIP_MAX[%d]", i)
	}
	if (orig->chipset.card_type >= 0x10) {
		for (int i = 0; i < 2; i++) {
			CMP(clip3d_min[i], "CLIP3D_MIN[%d]", i)
			CMP(clip3d_max[i], "CLIP3D_MAX[%d]", i)
		}
	}
	CMP(valid[0], "VALID[0]")
	CMP(valid[1], "VALID[1]")
	CMP(misc24[0], "MISC24[0]")
	CMP(misc24[1], "MISC24[1]")
	CMP(misc24[2], "MISC24[2]")
	CMP(misc32[0], "MISC32[0]")
	CMP(misc32[1], "MISC32[1]")
	CMP(misc32[2], "MISC32[2]")
	CMP(misc32[3], "MISC32[3]")
	CMP(dma_pitch, "DMA_PITCH")
	CMP(dvd_format, "DVD_FORMAT")
	CMP(sifm_mode, "SIFM_MODE")
	if (orig->chipset.card_type >= 0x10) {
		CMP(unk588, "UNK588")
		CMP(unk58c, "UNK58C")
	}
	CMP(bitmap_color_0, "BITMAP_COLOR_0")
	CMP(rop, "ROP")
	CMP(beta, "BETA")
	CMP(beta4, "BETA4")
	CMP(chroma, "CHROMA")
	CMP(pattern_mono_color[0], "PATTERN_MONO_COLOR[0]")
	CMP(pattern_mono_color[1], "PATTERN_MONO_COLOR[1]")
	CMP(pattern_mono_bitmap[0], "PATTERN_MONO_BITMAP[0]")
	CMP(pattern_mono_bitmap[1], "PATTERN_MONO_BITMAP[1]")
	CMP(pattern_config, "PATTERN_CONFIG")
	for (int i = 0; i < 64; i++)
		CMP(pattern_color[i], "PATTERN_COLOR[%d]", i)
	for (int i = 0; i < 6; i++) {
		CMP(surf_base[i], "SURF_BASE[%d]", i)
		CMP(surf_offset[i], "SURF_OFFSET[%d]", i)
		CMP(surf_limit[i], "SURF_LIMIT[%d]", i)
	}
	for (int i = 0; i < 5; i++)
		CMP(surf_pitch[i], "SURF_PITCH[%d]", i)
	for (int i = 0; i < 2; i++)
		CMP(surf_swizzle[i], "SURF_SWIZZLE[%d]", i)
	CMP(surf_type, "SURF_TYPE")
	CMP(surf_format, "SURF_FORMAT")
	CMP(ctx_valid, "CTX_VALID")
	CMP(ctx_format, "CTX_FORMAT")
	CMP(notify, "NOTIFY")
	if (orig->chipset.card_type < 0x10) {
		for (int i = 0; i < 2; i++) {
			CMP(d3d_rc_alpha[i], "D3D_RC_ALPHA[%d]", i)
			CMP(d3d_rc_color[i], "D3D_RC_COLOR[%d]", i)
			CMP(d3d_tex_format[i], "D3D_TEX_FORMAT[%d]", i)
			CMP(d3d_tex_filter[i], "D3D_TEX_FILTER[%d]", i)
		}
		CMP(d3d_unk5c0, "D3D_UNK5C0")
		CMP(d3d_unk5c4, "D3D_UNK5C4")
		CMP(d3d_unk5c8, "D3D_UNK5C8")
		CMP(d3d_unk5cc, "D3D_UNK5CC")
		CMP(d3d_unk5d0, "D3D_UNK5D0")
		CMP(d3d_unk5d4, "D3D_UNK5D4")
		CMP(d3d_unk5d8, "D3D_UNK5D8")
		CMP(d3d_unk5dc, "D3D_UNK5DC")
		CMP(d3d_unk5e0, "D3D_UNK5E0")
		CMP(d3d_config, "D3D_CONFIG")
		CMP(d3d_stencil_func, "D3D_STENCIL_FUNC")
		CMP(d3d_stencil_op, "D3D_STENCIL_OP")
		CMP(d3d_blend, "D3D_BLEND")
	}
	CMP(dma_offset[0], "DMA_OFFSET[0]")
	CMP(dma_offset[1], "DMA_OFFSET[1]")
	CMP(dma_length, "DMA_LENGTH")
	CMP(dma_misc, "DMA_MISC")
	CMP(dma_unk20[0], "DMA_UNK20[0]")
	CMP(dma_unk20[1], "DMA_UNK20[1]")
	for (int i = 0; i < 2; i++) {
		CMP(dma_eng_inst[i], "DMA_ENG_INST[%d]", i)
		CMP(dma_eng_flags[i], "DMA_ENG_FLAGS[%d]", i)
		CMP(dma_eng_limit[i], "DMA_ENG_LIMIT[%d]", i)
		CMP(dma_eng_pte[i], "DMA_ENG_PTE[%d]", i)
		CMP(dma_eng_pte_tag[i], "DMA_ENG_PTE_TAG[%d]", i)
		CMP(dma_eng_addr_virt_adj[i], "DMA_ENG_ADDR_VIRT_ADJ[%d]", i)
		CMP(dma_eng_addr_phys[i], "DMA_ENG_ADDR_PHYS[%d]", i)
		CMP(dma_eng_bytes[i], "DMA_ENG_BYTES[%d]", i)
		CMP(dma_eng_lines[i], "DMA_ENG_LINES[%d]", i)
	}
	if (broke && !print) {
		print = true;
		goto restart;
	}
	return broke;
}

static int test_state(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		struct nv04_pgraph_state orig, real;
		nv04_pgraph_gen_state(ctx, &orig);
		nv04_pgraph_load_state(ctx, &orig);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &orig, &real)) {
			printf("Iter %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_write(struct hwtest_ctx *ctx) {
	int i;
	bool is_nv5 = ctx->chipset.chipset >= 5;
	bool is_nv11p = ctx->chipset.chipset > 0x10;
	uint32_t ctxs_mask, ctxc_mask;
	uint32_t offset_mask;
	if (ctx->chipset.card_type < 0x10) {
		offset_mask = is_nv5 ? 0x01fffff0 : 0x00fffff0;
		ctxs_mask = ctxc_mask = is_nv5 ? 0x7f73f0ff : 0x0303f0ff;
	} else {
		offset_mask = 0x07fffff0;
		ctxs_mask = is_nv11p ? 0x7ffff0ff : 0x7fb3f0ff;
		ctxc_mask = is_nv11p ? 0x7ffff0ff : 0x7f33f0ff;
	}
	for (i = 0; i < 100000; i++) {
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_load_state(ctx, &orig);
		uint32_t reg;
		uint32_t val = jrand48(ctx->rand48);
		int idx;
		switch (nrand48(ctx->rand48) % 74) {
			default:
			case 0:
				reg = 0x400140;
				if (ctx->chipset.card_type < 0x10) {
					exp.intr_en = val & 0x11311;
				} else {
					exp.intr_en = val & 0x1113711;
				}
				break;
			case 1:
				reg = 0x400104;
				if (ctx->chipset.card_type < 0x10) {
					exp.nstatus = val & 0x7800;
				} else {
					exp.nstatus = val & 0x7800000;
				}
				break;
			case 2:
				reg = 0x400080;
				if (ctx->chipset.card_type >= 0x10) {
					// XXX
					continue;
				}
				exp.debug[0] = val & 0x1337f000;
				if (val & 3) {
					exp.xy_misc_0 &= 1 << 20;
					exp.xy_misc_1[0] = 0;
					exp.xy_misc_1[1] = 0;
					exp.xy_misc_3 &= ~0x1100;
					exp.xy_misc_4[0] &= ~0xff;
					exp.xy_misc_4[1] &= ~0xff;
					exp.xy_clip[0][0] = 0x55555555;
					exp.xy_clip[0][1] = 0x55555555;
					exp.xy_clip[1][0] = 0x55555555;
					exp.xy_clip[1][1] = 0x55555555;
					exp.valid[0] = 0;
					exp.valid[1] = 0;
					exp.misc24[0] = 0;
					exp.misc24[1] = 0;
					exp.misc24[2] = 0;
				}
				if (val & 0x11) {
					for (int i = 0; i < 6; i++) {
						exp.surf_offset[i] = 0;
						exp.surf_base[i] = 0;
						exp.surf_limit[i] = offset_mask | 0xf;
					}
					for (int i = 0; i < 5; i++)
						exp.surf_pitch[i] = 0;
					for (int i = 0; i < 2; i++)
						exp.surf_swizzle[i] = 0;
					exp.unk610 = 0;
					exp.unk614 = 0;
				}
				if (val & 0x101) {
					exp.dma_eng_flags[0] &= ~0x1000;
					exp.dma_eng_flags[1] &= ~0x1000;
				}
				break;
			case 3:
				reg = 0x400084;
				if (ctx->chipset.card_type < 0x10) {
					exp.debug[1] = val & (is_nv5 ? 0xf2ffb701 : 0x72113101);
					if (val & 0x10)
						exp.xy_misc_1[0] &= ~1;
				} else {
					uint32_t mangled = val & 0x3fffffff;
					if (val & 1 << 30)
						mangled |= 1 << 31;
					if (val & 1 << 31)
						mangled |= 1 << 30;
					exp.debug[1] = mangled & (is_nv11p ? 0xfe71f701 : 0xfe11f701);
					if (val & 0x10)
						exp.xy_misc_1[0] &= ~1;
				}
				break;
			case 4:
				reg = 0x400088;
				if (ctx->chipset.card_type <0x10)
					exp.debug[2] = val & 0x11d7fff1;
				else
					exp.debug[2] = val;
				break;
			case 5:
				reg = 0x40008c;
				if (ctx->chipset.card_type < 0x10) {
					exp.debug[3] = val & (is_nv5 ? 0xfbffff73 : 0x11ffff33);
				} else {
					bool is_nv11p = ctx->chipset.chipset > 0x10;
					exp.debug[3] = val & (is_nv11p ? 0xffffff78 : 0xfffffc70);
				}
				break;
			case 6:
				reg = 0x400090;
				if (ctx->chipset.card_type < 0x10)
					continue;
				exp.debug[4] = val & 0x00ffffff;
				break;
			case 7:
				{
				bool vre = ctx->chipset.card_type >= 0x10 ? extr(orig.debug[3], 19, 1) : extr(orig.debug[2], 28, 1);
				reg = ctx->chipset.card_type >= 0x10 ? 0x40014c : 0x400160;
				exp.ctx_switch[0] = val & ctxs_mask;
				insrt(exp.debug[1], 0, 1, extr(val, 31, 1) && vre);
				if (extr(exp.debug[1], 0, 1)) {
					nv04_pgraph_volatile_reset(&exp);
				}
				break;
				}
			case 8:
				reg = ctx->chipset.card_type >= 0x10 ? 0x400150 : 0x400164;
				exp.ctx_switch[1] = val & 0xffff3f03;
				break;
			case 9:
				reg = ctx->chipset.card_type >= 0x10 ? 0x400154 : 0x400168;
				exp.ctx_switch[2] = val;
				break;
			case 10:
				reg = ctx->chipset.card_type >= 0x10 ? 0x400158 : 0x40016c;
				exp.ctx_switch[3] = val & 0xffff;
				break;
			case 11:
				if (ctx->chipset.card_type < 0x10)
					continue;
				reg = 0x40015c;
				exp.ctx_switch[4] = val;
				break;
			case 12:
				idx = jrand48(ctx->rand48) & 7;
				reg = (ctx->chipset.card_type >= 0x10 ? 0x400160 : 0x400180) + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][0] = val & ctxc_mask;
				break;
			case 13:
				idx = jrand48(ctx->rand48) & 7;
				reg = (ctx->chipset.card_type >= 0x10 ? 0x400180 : 0x4001a0) + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][1] = val & 0xffff3f03;
				break;
			case 14:
				idx = jrand48(ctx->rand48) & 7;
				reg = (ctx->chipset.card_type >= 0x10 ? 0x4001a0 : 0x4001c0) + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][2] = val;
				break;
			case 15:
				idx = jrand48(ctx->rand48) & 7;
				reg = (ctx->chipset.card_type >= 0x10 ? 0x4001c0 : 0x4001e0) + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][3] = val & 0x0000ffff;
				break;
			case 16:
				if (ctx->chipset.card_type < 0x10)
					continue;
				idx = jrand48(ctx->rand48) & 7;
				reg = 0x4001e0 + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][4] = val;
				break;
			case 17:
				reg = ctx->chipset.card_type >= 0x10 ? 0x400144 : 0x400170;
				exp.ctx_control = val & 0x11010103;
				break;
			case 18:
				reg = ctx->chipset.card_type >= 0x10 ? 0x400148 : 0x400174;
				if (ctx->chipset.card_type < 0x10) {
					exp.ctx_user = val & 0x0f00e000;
				} else {
					bool is_nv11p = ctx->chipset.chipset > 0x10;
					exp.ctx_user = val & (is_nv11p ? 0x9f00e000 : 0x1f00e000);
				}
				break;
			case 19:
				reg = 0x400610;
				if (ctx->chipset.card_type < 0x10) {
					exp.unk610 = val & (0xe0000000 | offset_mask);
				} else {
					exp.unk610 = val & 0xfffffff0;
				}
				break;
			case 20:
				reg = 0x400614;
				exp.unk614 = val & (0xc0000000 | offset_mask);
				break;
			case 21:
				if (ctx->chipset.card_type < 0x10)
					continue;
				reg = 0x40077c;
				if (!is_nv11p) {
					exp.unk77c = val & 0x631fffff;
				} else {
					exp.unk77c = val & 0x0100ffff;
					if (val & 1 << 28)
						exp.unk77c |= 7 << 28;
				}
				break;
			case 22:
				if (ctx->chipset.card_type < 0x10)
					idx = jrand48(ctx->rand48) & 0x1f;
				else
					idx = nrand48(ctx->rand48) % 10;
				reg = 0x400400 + idx * 4;
				exp.vtx_x[idx] = val;
				nv04_pgraph_vtx_fixup(&exp, 0, 8, val);
				break;
			case 23:
				if (ctx->chipset.card_type < 0x10)
					idx = jrand48(ctx->rand48) & 0x1f;
				else
					idx = nrand48(ctx->rand48) % 10;
				reg = 0x400480 + idx * 4;
				exp.vtx_y[idx] = val;
				nv04_pgraph_vtx_fixup(&exp, 1, 8, val);
				break;
			case 24:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400534 + idx * 4;
				exp.iclip[idx] = val & 0x3ffff;
				insrt(exp.xy_misc_1[0], 12, 1, 0);
				insrt(exp.xy_misc_1[0], 16, 1, 0);
				insrt(exp.xy_misc_1[0], 20, 1, 0);
				insrt(exp.xy_misc_1[1], 12, 1, 0);
				insrt(exp.xy_misc_1[1], 16, 1, 0);
				insrt(exp.xy_misc_1[1], 20, 1, 0);
				nv04_pgraph_iclip_fixup(&exp, idx, val);
				break;
			case 25:
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x40053c + idx * 4;
				nv04_pgraph_uclip_write(&exp, 0, idx & 1, idx >> 1, val);
				break;
			case 26:
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x400560 + idx * 4;
				nv04_pgraph_uclip_write(&exp, 1, idx & 1, idx >> 1, val);
				break;
			case 27:
				if (ctx->chipset.card_type < 0x10)
					continue;
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x400550 + idx * 4;
				nv04_pgraph_uclip_write(&exp, 2, idx & 1, idx >> 1, val);
				break;
			case 28:
				reg = 0x400514;
				if (ctx->chipset.card_type < 0x10)
					exp.xy_misc_0 = val & 0xf013ffff;
				else
					exp.xy_misc_0 = val & 0xf113ffff;
				break;
			case 29:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400518 + idx * 4;
				exp.xy_misc_1[idx] = val & 0x00111031;
				break;
			case 30:
				reg = 0x400520;
				exp.xy_misc_3 = val & 0x7f7f1111;
				break;
			case 31:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400500 + idx * 4;
				exp.xy_misc_4[idx] = val & 0x300000ff;
				break;
			case 32:
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x400524 + idx * 4;
				exp.xy_clip[idx >> 1][idx & 1] = val;
				break;
			case 33:
				reg = 0x400508;
				exp.valid[0] = val & 0xf07fffff;
				break;
			case 34:
				reg = 0x400578;
				if (ctx->chipset.card_type < 0x10)
					exp.valid[1] = val & 0x1fffffff;
				else
					exp.valid[1] = val & 0xdfffffff;
				break;
			case 35:
				idx = nrand48(ctx->rand48) % 3;
				reg = ((uint32_t[3]){0x400510, 0x400570, 0x400574})[idx];
				exp.misc24[idx] = val & 0xffffff;
				break;
			case 36:
				idx = jrand48(ctx->rand48) & 3;
				reg = ((uint32_t[4]){0x40050c, 0x40057c, 0x400580, 0x400584})[idx];
				exp.misc32[idx] = val;
				if (idx == 0)
					exp.valid[0] |= 1 << 16;
				break;
			case 37:
				reg = ctx->chipset.card_type >= 0x10 ? 0x400770 : 0x400760;
				exp.dma_pitch = val;
				break;
			case 38:
				reg = ctx->chipset.card_type >= 0x10 ? 0x400774 : 0x400764;
				exp.dvd_format = val & 0x33f;
				break;
			case 39:
				reg = ctx->chipset.card_type >= 0x10 ? 0x400778 : 0x400768;
				exp.sifm_mode = val & 0x01030000;
				break;
			case 40:
				if (ctx->chipset.card_type < 0x10)
					continue;
				reg = 0x400588;
				exp.unk588 = val & 0xffff;
				break;
			case 41:
				if (ctx->chipset.card_type < 0x10)
					continue;
				reg = 0x40058c;
				exp.unk58c = val & 0x1ffff;
				break;
			case 42:
				reg = 0x400600;
				exp.bitmap_color_0 = val;
				exp.valid[0] |= 1 << 17;
				break;
			case 43:
				reg = 0x400604;
				exp.rop = val & 0xff;
				break;
			case 44:
				reg = 0x400608;
				exp.beta = val & 0x7f800000;
				break;
			case 45:
				reg = 0x40060c;
				exp.beta4 = val;
				break;
			case 46:
				reg = 0x400814;
				exp.chroma = val;
				break;
			case 47:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400800 + idx * 4;
				exp.pattern_mono_color[idx] = val;
				break;
			case 48:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400808 + idx * 4;
				exp.pattern_mono_bitmap[idx] = val;
				break;
			case 49:
				reg = 0x400810;
				exp.pattern_config = val & 0x13;
				break;
			case 50:
				idx = jrand48(ctx->rand48) & 0x3f;
				reg = 0x400900 + idx * 4;
				exp.pattern_color[idx] = val & 0xffffff;
				break;
			case 51:
				idx = nrand48(ctx->rand48) % 6;
				reg = 0x400640 + idx * 4;
				exp.surf_offset[idx] = val & offset_mask;
				exp.valid[0] |= 8;
				break;
			case 52:
				idx = nrand48(ctx->rand48) % 6;
				reg = 0x400658 + idx * 4;
				exp.surf_base[idx] = val & offset_mask;
				break;
			case 53:
				idx = nrand48(ctx->rand48) % 6;
				reg = 0x400684 + idx * 4;
				exp.surf_limit[idx] = (val & (1 << 31 | offset_mask)) | 0xf;
				break;
			case 54:
				idx = nrand48(ctx->rand48) % 5;
				reg = 0x400670 + idx * 4;
				if (ctx->chipset.card_type < 0x10)
					exp.surf_pitch[idx] = val & 0x1ff0;
				else
					exp.surf_pitch[idx] = val & 0xfff0;
				exp.valid[0] |= 4;
				break;
			case 55:
				idx = nrand48(ctx->rand48) % 2;
				reg = 0x40069c + idx * 4;
				exp.surf_swizzle[idx] = val & 0x0f0f0000;
				break;
			case 56:
				reg = ctx->chipset.card_type >= 0x10 ? 0x400710 : 0x40070c;
				if (!is_nv11p) {
					exp.surf_type = val & 3;
				} else {
					exp.surf_type = val & 0x77777713;
				}
				break;
			case 57:
				reg = 0x400724;
				exp.surf_format = val & 0xffffff;
				break;
			case 58:
				reg = ctx->chipset.card_type >= 0x10 ? 0x400714 : 0x400710;
				exp.ctx_valid = val & 0x0f731f3f;
				break;
			case 59:
				reg = 0x400830;
				exp.ctx_format = val & 0x3f3f3f3f;
				break;
			case 60:
				reg = ctx->chipset.card_type >= 0x10 ? 0x400718 : 0x400714;
				exp.notify = val & (ctx->chipset.card_type >= 0x10 ? 0x73110101 : 0x110101);
				break;
			case 61:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401000 + idx * 4;
				exp.dma_offset[idx] = val;
				break;
			case 62:
				reg = 0x401008;
				exp.dma_length = val & 0x3fffff;
				break;
			case 63:
				reg = 0x40100c;
				exp.dma_misc = val & 0x77ffff;
				break;
			case 64:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401020 + idx * 4;
				exp.dma_unk20[idx] = val;
				break;
			case 65:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401040 + idx * 0x40;
				exp.dma_eng_inst[idx] = val & 0xffff;
				break;
			case 66:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401044 + idx * 0x40;
				exp.dma_eng_flags[idx] = val & 0xfff33000;
				break;
			case 67:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401048 + idx * 0x40;
				exp.dma_eng_limit[idx] = val;
				break;
			case 68:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x40104c + idx * 0x40;
				exp.dma_eng_pte[idx] = val & 0xfffff002;
				break;
			case 69:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401050 + idx * 0x40;
				exp.dma_eng_pte_tag[idx] = val & 0xfffff000;
				break;
			case 70:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401054 + idx * 0x40;
				exp.dma_eng_addr_virt_adj[idx] = val;
				break;
			case 71:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401058 + idx * 0x40;
				exp.dma_eng_addr_phys[idx] = val;
				break;
			case 72:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x40105c + idx * 0x40;
				exp.dma_eng_bytes[idx] = val & 0x1ffffff;
				break;
			case 73:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401060 + idx * 0x40;
				exp.dma_eng_lines[idx] = val & 0x7ff;
				break;
		}
		nva_wr32(ctx->cnum, reg, val);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d wrote %08x to %06x\n", i, val, reg);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_d3d_write(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type != 4)
		return HWTEST_RES_NA;
	for (int i = 0; i < 100000; i++) {
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_load_state(ctx, &orig);
		uint32_t reg;
		uint32_t val = jrand48(ctx->rand48);
		int idx;
		switch (nrand48(ctx->rand48) % 20) {
			default:
			case 0:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400590 + idx * 8;
				exp.d3d_rc_alpha[idx] = val & 0xfd1d1d1d;
				if (idx == 0)
					exp.valid[1] |= 1 << 28;
				else
					exp.valid[1] |= 1 << 26;
				break;
			case 1:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400594 + idx * 8;
				exp.d3d_rc_color[idx] = val & 0xff1f1f1f;
				if (idx == 0)
					exp.valid[1] |= 1 << 27;
				else
					exp.valid[1] |= 1 << 25;
				break;
			case 2:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x4005a8 + idx * 4;
				exp.d3d_tex_format[idx] = val & 0xfffff7a6;
				break;
			case 3:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x4005b0 + idx * 4;
				exp.d3d_tex_filter[idx] = val & 0xffff9e1e;
				break;
			case 4:
				reg = 0x4005c0;
				exp.d3d_unk5c0 = val;
				break;
			case 5:
				reg = 0x4005c4;
				exp.d3d_unk5c4 = val & 0xffffffc0;
				break;
			case 6:
				reg = 0x4005c8;
				exp.d3d_unk5c8 = val & 0xffffffc0;
				break;
			case 7:
				reg = 0x4005cc;
				exp.d3d_unk5cc = val & 0xffffffc0;
				break;
			case 8:
				reg = 0x4005d0;
				exp.d3d_unk5d0 = val & 0xffffffc0;
				break;
			case 9:
				reg = 0x4005d4;
				exp.d3d_unk5d4 = val;
				break;
			case 10:
				reg = 0x4005d8;
				exp.d3d_unk5d8 = val;
				break;
			case 11:
				reg = 0x4005dc;
				exp.d3d_unk5dc = val;
				break;
			case 12:
				reg = 0x4005e0;
				exp.d3d_unk5e0 = val & 0xffffffc0;
				break;
			case 13:
				reg = 0x400818;
				exp.d3d_config = val & 0xffff5fff;
				exp.valid[1] |= 1 << 17;
				break;
			case 14:
				reg = 0x40081c;
				exp.d3d_stencil_func = val & 0xfffffff1;
				exp.valid[1] |= 1 << 18;
				break;
			case 15:
				reg = 0x400820;
				exp.d3d_stencil_op = val & 0x00000fff;
				exp.valid[1] |= 1 << 19;
				break;
			case 16:
				reg = 0x400824;
				exp.d3d_blend = val & 0xff1111ff;
				exp.valid[1] |= 1 << 20;
				break;
			case 17:
				if (ctx->chipset.card_type >= 0x10)
					continue;
				idx = jrand48(ctx->rand48) & 0xf;
				reg = 0x400d00 + idx * 4;
				exp.vtx_u[idx] = val & 0xffffffc0;
				break;
			case 18:
				if (ctx->chipset.card_type >= 0x10)
					continue;
				idx = jrand48(ctx->rand48) & 0xf;
				reg = 0x400d40 + idx * 4;
				exp.vtx_v[idx] = val & 0xffffffc0;
				break;
			case 19:
				if (ctx->chipset.card_type >= 0x10)
					continue;
				idx = jrand48(ctx->rand48) & 0xf;
				reg = 0x400d80 + idx * 4;
				exp.vtx_m[idx] = val & 0xffffffc0;
				break;
		}
		nva_wr32(ctx->cnum, reg, val);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d wrote %08x to %06x\n", i, val, reg);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_vtx_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int vtxnum = ctx->chipset.chipset >= 0x10 ? 10 : 32;
		int idx = nrand48(ctx->rand48) % vtxnum;
		int xy = jrand48(ctx->rand48) & 1;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = 0x400400 + idx * 4 + xy * 0x80;
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			switch (jrand48(ctx->rand48) & 3) {
				case 0:
					val = orig.uclip_min[xy];
					break;
				case 1:
					val = orig.uclip_max[xy];
					break;
				case 2:
					val = orig.oclip_min[xy];
					break;
				case 3:
					val = orig.oclip_max[xy];
					break;
			}
		}
		if (jrand48(ctx->rand48) & 1)
			val = extrs(val, 0, 18);
		nva_wr32(ctx->cnum, reg, val);
		(xy ? exp.vtx_y : exp.vtx_x)[idx] = val;
		nv04_pgraph_vtx_fixup(&exp, xy, 8, val);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d wrote %08x to %06x\n", i, val, reg);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_iclip_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int xy = jrand48(ctx->rand48) & 1;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = 0x400534 + xy * 4;
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			switch (jrand48(ctx->rand48) & 3) {
				case 0:
					val = orig.uclip_min[xy];
					break;
				case 1:
					val = orig.uclip_max[xy];
					break;
				case 2:
					val = orig.oclip_min[xy];
					break;
				case 3:
					val = orig.oclip_max[xy];
					break;
			}
		}
		if (jrand48(ctx->rand48) & 1)
			val = extrs(val, 0, 18);
		nva_wr32(ctx->cnum, reg, val);
		exp.iclip[xy] = val & 0x3ffff;
		insrt(exp.xy_misc_1[0], 12, 1, 0);
		insrt(exp.xy_misc_1[0], 16, 1, 0);
		insrt(exp.xy_misc_1[0], 20, 1, 0);
		insrt(exp.xy_misc_1[1], 12, 1, 0);
		insrt(exp.xy_misc_1[1], 16, 1, 0);
		insrt(exp.xy_misc_1[1], 20, 1, 0);
		nv04_pgraph_iclip_fixup(&exp, xy, val);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d wrote %08x to %06x\n", i, val, reg);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_uclip_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int nuo = ctx->chipset.card_type == 0x10 ? 3 : 2;
		int which = nrand48(ctx->rand48) % nuo;
		int xy = jrand48(ctx->rand48) & 1;
		int idx = jrand48(ctx->rand48) & 1;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t regs[] = {0x40053c, 0x400560, 0x400550};
		uint32_t reg = regs[which] + xy * 4 + idx * 8;
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1)
			val = extrs(val, 0, 18);
		nva_wr32(ctx->cnum, reg, val);
		nv04_pgraph_uclip_write(&exp, which, xy, idx, val);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d wrote %08x to %06x\n", i, val, reg);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_read(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		struct nv04_pgraph_state orig, real;
		uint32_t reg = 0x400000 | (jrand48(ctx->rand48) & 0x1ffc);
		nv04_pgraph_gen_state(ctx, &orig);
		nv04_pgraph_load_state(ctx, &orig);
		nva_rd32(ctx->cnum, reg);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &orig, &real)) {
			printf("Iter %d read %06x\n", i, reg);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_formats(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		struct nv04_pgraph_state orig, real;
		nv04_pgraph_gen_state(ctx, &orig);
		if (!(jrand48(ctx->rand48) & 3)) {
			uint32_t classes[] = {
				0x1f, 0x5f,
				0x48, 0x54, 0x55,
				0x37, 0x77,
				0x60, 0x38, 0x39, 0x64,
			};
			insrt(orig.ctx_switch[0], 0, 8, classes[nrand48(ctx->rand48)%ARRAY_SIZE(classes)]);
		}
		if (!(jrand48(ctx->rand48) & 3)) {
			orig.chroma = 0;
			if (jrand48(ctx->rand48) & 1) {
				orig.chroma |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				orig.chroma |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				orig.chroma |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				orig.chroma |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				orig.surf_format = 3;
				orig.ctx_switch[1] = 0x0d00;
				insrt(orig.ctx_switch[0], 15, 3, 0);
			}
			if (jrand48(ctx->rand48) & 1) {
				uint32_t formats[] = {
					2, 6, 8, 0xb, 0x10,
				};
				insrt(orig.ctx_format, 24, 6, formats[nrand48(ctx->rand48)%ARRAY_SIZE(formats)]);
			}
		}
		nv04_pgraph_load_state(ctx, &orig);
		uint32_t val = nva_rd32(ctx->cnum, 0x400618);
		nv04_pgraph_dump_state(ctx, &real);
		uint32_t ev = nv04_pgraph_formats(&orig);
		if (nv04_pgraph_cmp_state(&orig, &orig, &real, ev != val)) {
			printf("Iter %d got %08x expected %08x\n", i, val, ev);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int state_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static uint32_t nv04_pgraph_gen_dma(struct hwtest_ctx *ctx, struct nv04_pgraph_state *state, uint32_t dma[3]) {
	int old_subc = extr(state->ctx_user, 13, 3);
	int new_subc = extr(state->fifo_mthd_st2, 12, 3);
	uint32_t inst;
	if (old_subc != new_subc && extr(state->debug[1], 20, 1)) {
		inst = state->ctx_cache[new_subc][3];
	} else {
		inst = state->ctx_switch[3];
	}
	inst ^= 1 << (jrand48(ctx->rand48) & 0xf);
	for (int i = 0; i < 3; i++) {
		dma[i] = jrand48(ctx->rand48);
	}
	if (jrand48(ctx->rand48) & 1) {
		uint32_t classes[4] = {0x2, 0x3, 0x3d, 0x30};
		insrt(dma[0], 0, 12, classes[jrand48(ctx->rand48) & 3]);
	}
	if (jrand48(ctx->rand48) & 1) {
		insrt(dma[0], 20, 4, 0);
	}
	if (jrand48(ctx->rand48) & 1) {
		insrt(dma[0], 24, 4, 0);
	}
	if (jrand48(ctx->rand48) & 1) {
		dma[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
	}
	if (jrand48(ctx->rand48) & 1) {
		dma[1] |= 0x0000007f;
		dma[1] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
	}
	for (int i = 0; i < 3; i++) {
		nva_wr32(ctx->cnum, 0x700000 | inst << 4 | i << 2, dma[i]);
	}
	inst |= jrand48(ctx->rand48) << 16;
	return inst;
}

static uint32_t nv04_pgraph_gen_ctxobj(struct hwtest_ctx *ctx, struct nv04_pgraph_state *state, uint32_t ctxobj[4]) {
	int old_subc = extr(state->ctx_user, 13, 3);
	int new_subc = extr(state->fifo_mthd_st2, 12, 3);
	uint32_t inst;
	if (old_subc != new_subc && extr(state->debug[1], 20, 1)) {
		inst = state->ctx_cache[new_subc][3];
	} else {
		inst = state->ctx_switch[3];
	}
	inst ^= 1 << (jrand48(ctx->rand48) & 0xf);
	for (int i = 0; i < 4; i++) {
		ctxobj[i] = jrand48(ctx->rand48);
	}
	if (jrand48(ctx->rand48) & 1) {
		uint32_t classes[] = {0x30, 0x12, 0x72, 0x19, 0x43, 0x17, 0x57, 0x18, 0x44, 0x42, 0x52, 0x53, 0x58, 0x59, 0x5a, 0x5b};
		insrt(ctxobj[0], 0, 12, classes[nrand48(ctx->rand48) % ARRAY_SIZE(classes)]);
	}
	if (jrand48(ctx->rand48) & 1) {
		ctxobj[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
	}
	for (int i = 0; i < 4; i++) {
		nva_wr32(ctx->cnum, 0x700000 | inst << 4 | i << 2, ctxobj[i]);
	}
	inst |= jrand48(ctx->rand48) << 16;
	return inst;
}

static void nv04_pgraph_prep_mthd(struct hwtest_ctx *ctx, uint32_t grobj[4], struct nv04_pgraph_state *state, uint32_t cls, uint32_t addr, uint32_t val, bool same_subc = false) {
	int chid = extr(state->ctx_user, 24, 7);
	state->fifo_ptr = 0;
	state->fifo_mthd_st2 = chid << 15 | addr >> 1 | 1;
	state->fifo_data_st2[0] = val;
	state->fifo_enable = 1;
	state->ctx_control |= 1 << 16;
	if (same_subc)
		insrt(state->ctx_user, 13, 3, extr(addr, 13, 3));
	int old_subc = extr(state->ctx_user, 13, 3);
	int new_subc = extr(state->fifo_mthd_st2, 12, 3);
	for (int i = 0; i < 4; i++)
		grobj[i] = jrand48(ctx->rand48);
	uint32_t inst;
	if (extr(addr, 2, 11) == 0) {
		insrt(grobj[0], 0, 8, cls);
		inst = val & 0xffff;
	} else if (old_subc != new_subc && extr(state->debug[1], 20, 1)) {
		if (extr(state->debug[1], 15, 1)) {
			insrt(grobj[0], 0, 8, cls);
		} else {
			insrt(state->ctx_cache[new_subc][0], 0, 8, cls);
		}
		inst = state->ctx_cache[new_subc][3];
	} else {
		insrt(state->ctx_switch[0], 0, 8, cls);
		inst = state->ctx_switch[3];
	}
	for (int i = 0; i < 4; i++)
		nva_wr32(ctx->cnum, 0x700000 | inst << 4 | i << 2, grobj[i]);
}

static void nv04_pgraph_mthd(struct nv04_pgraph_state *state, uint32_t grobj[4]) {
	state->fifo_mthd_st2 &= ~1;
	int old_subc = extr(state->ctx_user, 13, 3);
	int new_subc = extr(state->fifo_mthd_st2, 12, 3);
	bool ctxsw = extr(state->fifo_mthd_st2, 1, 11) == 0;
	if (extr(state->debug[3], 20, 2) == 3 && !ctxsw)
		nv04_pgraph_blowup(state, 2);
	if (old_subc != new_subc || ctxsw) {
		if (ctxsw) {
			state->ctx_cache[new_subc][3] = state->fifo_data_st2[0] & 0xffff;
		}
		uint32_t ctx_mask = state->chipset.chipset >= 5 ? 0x7f73f0ff : 0x0303f0ff;
		if (extr(state->debug[1], 15, 1) || ctxsw) {
			state->ctx_cache[new_subc][0] = grobj[0] & ctx_mask;
			state->ctx_cache[new_subc][1] = grobj[1] & 0xffff3f03;
			state->ctx_cache[new_subc][2] = grobj[2];
			state->ctx_cache[new_subc][4] = grobj[3];
		}
		bool reset = extr(state->debug[2], 28, 1);
		insrt(state->debug[1], 0, 1, reset);
		if (reset)
			nv04_pgraph_volatile_reset(state);
		insrt(state->ctx_user, 13, 3, new_subc);
		if (extr(state->debug[1], 20, 1)) {
			for (int i = 0; i < 5; i++)
				state->ctx_switch[i] = state->ctx_cache[new_subc][i];
		}
	}
}

static uint32_t get_random_class(struct hwtest_ctx *ctx) {
	uint32_t classes_nv4[] = {
		0x10, 0x11, 0x13, 0x15, 0x64, 0x65, 0x66, 0x67,
		0x12, 0x72, 0x43, 0x19, 0x17, 0x57, 0x18, 0x44,
		0x42, 0x52, 0x53, 0x58, 0x59, 0x5a, 0x5b,
		0x38, 0x39,
		0x1c, 0x1d, 0x1e, 0x1f, 0x5c, 0x5d, 0x5e, 0x5f,
		0x21, 0x61, 0x60, 0x37, 0x77, 0x36, 0x76, 0x4a,
		0x4b,
		0x54, 0x55, 0x48,
	};
	uint32_t classes_nv5[] = {
		0x12, 0x72, 0x43, 0x19, 0x17, 0x57, 0x18, 0x44,
		0x42, 0x52, 0x53, 0x58, 0x59, 0x5a, 0x5b,
		0x38, 0x39,
		0x1c, 0x1d, 0x1e, 0x1f, 0x5c, 0x5d, 0x5e, 0x5f,
		0x21, 0x61, 0x60, 0x37, 0x77, 0x36, 0x76, 0x4a,
		0x4b, 0x64, 0x65, 0x66,
		0x54, 0x55, 0x48,
	};
	if (ctx->chipset.chipset == 4)
		return classes_nv4[nrand48(ctx->rand48) % ARRAY_SIZE(classes_nv4)];
	else
		return classes_nv5[nrand48(ctx->rand48) % ARRAY_SIZE(classes_nv5)];
}

static int test_invalid_class(struct hwtest_ctx *ctx) {
	int i;
	for (int cls = 0; cls < 0x100; cls++) {
		switch (cls) {
			case 0x10:
			case 0x11:
			case 0x13:
			case 0x15:
			case 0x67:
				if (ctx->chipset.chipset == 4)
					continue;
				break;
			case 0x64:
			case 0x65:
			case 0x66:
			case 0x12:
			case 0x72:
			case 0x43:
			case 0x19:
			case 0x17:
			case 0x57:
			case 0x18:
			case 0x44:
			case 0x42:
			case 0x52:
			case 0x53:
			case 0x58:
			case 0x59:
			case 0x5a:
			case 0x5b:
			case 0x38:
			case 0x39:
			case 0x1c:
			case 0x1d:
			case 0x1e:
			case 0x1f:
			case 0x21:
			case 0x36:
			case 0x37:
			case 0x5c:
			case 0x5d:
			case 0x5e:
			case 0x5f:
			case 0x60:
			case 0x61:
			case 0x76:
			case 0x77:
			case 0x4a:
			case 0x4b:
			case 0x54:
			case 0x55:
			case 0x48:
				continue;
		}
		for (i = 0; i < 10; i++) {
			uint32_t val = jrand48(ctx->rand48);
			uint32_t mthd = jrand48(ctx->rand48) & 0x1ffc;
			if (mthd == 0 || mthd == 0x100 || i < 3)
				mthd = 0x104;
			uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
			struct nv04_pgraph_state orig, exp, real;
			nv04_pgraph_gen_state(ctx, &orig);
			orig.notify &= ~0x10000;
			uint32_t grobj[4];
			nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
			nv04_pgraph_load_state(ctx, &orig);
			exp = orig;
			nv04_pgraph_mthd(&exp, grobj);
			nv04_pgraph_blowup(&exp, 0x0040);
			nv04_pgraph_dump_state(ctx, &real);
			if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
				printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
				return HWTEST_RES_FAIL;
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_op(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset != 4)
		return HWTEST_RES_NA;
	for (int cls : {0x10, 0x11, 0x13, 0x15, 0x64, 0x65, 0x66, 0x67}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200 || mthd == 0x204)
				continue;
			if (mthd == 0x208 && (cls == 0x11 || cls == 0x13 || cls == 0x66 || cls == 0x67))
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_ctx(struct hwtest_ctx *ctx) {
	for (int cls : {0x12, 0x72, 0x43, 0x19, 0x17, 0x57, 0x18, 0x44}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if ((cls == 0x12 || cls == 0x72 || cls == 0x43) && mthd == 0x300)
				continue;
			if (cls == 0x19 && (mthd == 0x300 || mthd == 0x304))
				continue;
			if ((cls == 0x17 || cls == 0x57) && (mthd == 0x300 || mthd == 0x304))
				continue;
			if ((cls == 0x18 || cls == 0x44) && (mthd == 0x300 || mthd == 0x304 || mthd == 0x308 || (mthd & 0x1ff0) == 0x310))
				continue;
			if (cls == 0x44 && (mthd == 0x30c ||
				(mthd & 0x1fc0) == 0x400 ||
				(mthd & 0x1f80) == 0x500 ||
				(mthd & 0x1f80) == 0x600 ||
				(mthd & 0x1f00) == 0x700))
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_surf(struct hwtest_ctx *ctx) {
	for (int cls : {0x42, 0x52, 0x53, 0x58, 0x59, 0x5a, 0x5b}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x184)
				continue;
			if ((cls == 0x42 || cls == 0x53) && mthd == 0x188)
				continue;
			if ((cls == 0x42 || cls == 0x52) && (mthd & 0x1f00) == 0x200)
				continue;
			if (cls == 0x42 && (mthd >= 0x300 && mthd <= 0x30c))
				continue;
			if (cls == 0x52 && (mthd >= 0x300 && mthd <= 0x304))
				continue;
			if (cls == 0x53 && (mthd >= 0x300 && mthd <= 0x310))
				continue;
			if (cls == 0x53 && (mthd == 0x2f8 || mthd == 0x2fc) && ctx->chipset.chipset >= 5)
				continue;
			if ((cls & 0xfc) == 0x58 && ((mthd & 0x1ff8) == 0x200 || mthd == 0x300 || mthd == 0x308 || mthd == 0x30c))
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_dvd(struct hwtest_ctx *ctx) {
	for (int cls : {0x38}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd >= 0x180 && mthd <= 0x18c)
				continue;
			if (mthd >= 0x300 && mthd <= 0x33c)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_m2mf(struct hwtest_ctx *ctx) {
	for (int cls : {0x39}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd >= 0x180 && mthd <= 0x188)
				continue;
			if (mthd >= 0x30c && mthd <= 0x328)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_lin(struct hwtest_ctx *ctx) {
	for (int cls : {0x1c, 0x5c}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x10c)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x300 || mthd == 0x304)
				continue;
			if (mthd >= 0x400 && mthd <= 0x67c)
				continue;
			if (cls == 0x1c && mthd >= 0x184 && mthd <= 0x194 && ctx->chipset.chipset >= 5)
				continue;
			if (cls == 0x5c && mthd >= 0x184 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_tri(struct hwtest_ctx *ctx) {
	for (int cls : {0x1d, 0x5d}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x10c)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x300 || mthd == 0x304)
				continue;
			if (mthd >= 0x310 && mthd <= 0x318)
				continue;
			if (mthd >= 0x320 && mthd <= 0x334)
				continue;
			if (mthd >= 0x400 && mthd <= 0x5fc)
				continue;
			if (cls == 0x1d && mthd >= 0x184 && mthd <= 0x194 && ctx->chipset.chipset >= 5)
				continue;
			if (cls == 0x5d && mthd >= 0x184 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_rect(struct hwtest_ctx *ctx) {
	for (int cls : {0x1e, 0x5e}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x10c)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x300 || mthd == 0x304)
				continue;
			if (mthd >= 0x400 && mthd <= 0x47c)
				continue;
			if (cls == 0x1e && mthd >= 0x184 && mthd <= 0x194 && ctx->chipset.chipset >= 5)
				continue;
			if (cls == 0x5e && mthd >= 0x184 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_blit(struct hwtest_ctx *ctx) {
	for (int cls : {0x1f, 0x5f}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x10c)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200 || mthd == 0x204)
				continue;
			if (mthd == 0x300 || mthd == 0x304 || mthd == 0x308)
				continue;
			if (mthd >= 0x184 && mthd <= 0x19c && ctx->chipset.chipset >= 5)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_ifc(struct hwtest_ctx *ctx) {
	for (int cls : {0x21, 0x61, 0x65}) {
		if (cls == 0x65 && ctx->chipset.chipset < 5)
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x10c)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x300 || mthd == 0x304 || mthd == 0x308 || mthd == 0x30c)
				continue;
			if (cls == 0x21 && mthd >= 0x400 && mthd < 0x480)
				continue;
			if (cls != 0x21 && mthd >= 0x400)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			if (cls == 0x21 && mthd >= 0x184 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			if (cls != 0x21 && mthd >= 0x184 && mthd <= 0x19c && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_sifc(struct hwtest_ctx *ctx) {
	for (int cls : {0x36, 0x76, 0x66}) {
		if (cls == 0x66 && ctx->chipset.chipset < 5)
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x10c)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd >= 0x300 && mthd <= 0x318)
				continue;
			if (mthd >= 0x400)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			if (cls == 0x36 && mthd >= 0x184 && mthd <= 0x194 && ctx->chipset.chipset >= 5)
				continue;
			if (cls != 0x36 && mthd >= 0x184 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_iifc(struct hwtest_ctx *ctx) {
	for (int cls : {0x60, 0x64}) {
		if (cls == 0x64 && ctx->chipset.chipset < 5)
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x10c)
				continue;
			if (mthd == 0x180 || mthd == 0x184)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd >= 0x3e8)
				continue;
			if (mthd == 0x3e4 && ctx->chipset.chipset >= 5)
				continue;
			if (mthd >= 0x188 && mthd <= 0x1a0 && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_sifm(struct hwtest_ctx *ctx) {
	for (int cls : {0x37, 0x77}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x10c)
				continue;
			if (mthd == 0x180 || mthd == 0x184)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x300)
				continue;
			if (mthd == 0x304 && ctx->chipset.chipset >= 5)
				continue;
			if (mthd >= 0x308 && mthd <= 0x31c)
				continue;
			if (mthd >= 0x400 && mthd <= 0x40c)
				continue;
			if (cls == 0x37 && mthd >= 0x188 && mthd <= 0x194 && ctx->chipset.chipset >= 5)
				continue;
			if (cls != 0x37 && mthd >= 0x188 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_gdi_nv3(struct hwtest_ctx *ctx) {
	for (int cls : {0x4b}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x10c)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			if (mthd == 0x300 || mthd == 0x304)
				continue;
			if (mthd >= 0x3fc && mthd <= 0x5fc)
				continue;
			if (mthd >= 0x7f4 && mthd <= 0x9fc)
				continue;
			if (mthd >= 0xbec && mthd <= 0xdfc)
				continue;
			if (mthd >= 0xfe8 && mthd <= 0x11fc)
				continue;
			if (mthd >= 0x13e4 && mthd <= 0x15fc)
				continue;
			if (mthd >= 0x184 && mthd <= 0x190 && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_gdi_nv4(struct hwtest_ctx *ctx) {
	for (int cls : {0x4a}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x10c)
				continue;
			if (mthd == 0x180 || mthd == 0x184)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			if (mthd == 0x300 || mthd == 0x304)
				continue;
			if (mthd >= 0x3fc && mthd <= 0x4fc)
				continue;
			if (mthd >= 0x5f4 && mthd <= 0x6fc)
				continue;
			if (mthd >= 0x7ec && mthd <= 0xbfc)
				continue;
			if (mthd >= 0xbe4 && mthd <= 0xffc)
				continue;
			if (mthd >= 0xff0 && mthd <= 0x13fc)
				continue;
			if (mthd >= 0x17f0 && mthd <= 0x1ffc)
				continue;
			if (mthd >= 0x188 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_d3d0(struct hwtest_ctx *ctx) {
	for (int cls : {0x48}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x10c)
				continue;
			if (mthd == 0x180 || mthd == 0x184)
				continue;
			if ((mthd == 0x188 || mthd == 0x18c || mthd == 0x190) && ctx->chipset.chipset >= 5)
				continue;
			if ((mthd & 0x1ff0) == 0x200)
				continue;
			if (mthd >= 0x304 && mthd <= 0x318)
				continue;
			if (mthd >= 0x1000)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_d3d5(struct hwtest_ctx *ctx) {
	for (int cls : {0x54}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x180 || mthd == 0x184 || mthd == 0x188 || mthd == 0x18c)
				continue;
			if ((mthd & 0x1ff0) == 0x200)
				continue;
			if (mthd >= 0x300 && mthd <= 0x318)
				continue;
			if (mthd >= 0x400 && mthd < 0x700)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_d3d6(struct hwtest_ctx *ctx) {
	for (int cls : {0x55}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x180 || mthd == 0x184 || mthd == 0x188 || mthd == 0x18c)
				continue;
			if ((mthd & 0x1ff0) == 0x200)
				continue;
			if (mthd >= 0x308 && mthd <= 0x324)
				continue;
			if (mthd >= 0x32c && mthd <= 0x348)
				continue;
			if (mthd >= 0x400 && mthd < 0x600)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct nv04_pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctxsw(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = jrand48(ctx->rand48) & 0xff;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000);
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_nop(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = jrand48(ctx->rand48) & 0xff;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x100;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_notify(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = get_random_class(ctx);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x104;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[1] &= 0x000fffff;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		if (extr(exp.notify, 16, 1))
			nv04_pgraph_blowup(&exp, 0x1000);
		if (extr(exp.debug[3], 20, 1) && val > 1)
			nv04_pgraph_blowup(&exp, 2);
		if (extr(exp.debug[3], 28, 1) && !extr(exp.ctx_switch[1], 16, 16))
			nv04_pgraph_blowup(&exp, 0x0800);
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.notify, 16, 1, 1);
			insrt(exp.notify, 20, 1, val & 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_missing(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls;
		uint32_t mthd;
		switch (nrand48(ctx->rand48) % 50) {
			case 0:
			default:
				cls = 0x10;
				mthd = 0x200 + (nrand48(ctx->rand48) % 2) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 1:
				cls = 0x11;
				mthd = 0x200 + (nrand48(ctx->rand48) % 3) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 2:
				cls = 0x13;
				mthd = 0x200 + (nrand48(ctx->rand48) % 3) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 3:
				cls = 0x15;
				mthd = 0x200 + (nrand48(ctx->rand48) % 2) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 4:
				cls = 0x64;
				mthd = 0x200 + (nrand48(ctx->rand48) % 2) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 5:
				cls = 0x65;
				mthd = 0x200 + (nrand48(ctx->rand48) % 2) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 6:
				cls = 0x66;
				mthd = 0x200 + (nrand48(ctx->rand48) % 3) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 7:
				cls = 0x67;
				mthd = 0x200 + (nrand48(ctx->rand48) % 3) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 8:
				cls = 0x12;
				mthd = 0x200;
				break;
			case 9:
				cls = 0x72;
				mthd = 0x200;
				break;
			case 10:
				cls = 0x43;
				mthd = 0x200;
				break;
			case 11:
				cls = 0x17;
				mthd = 0x200;
				break;
			case 12:
				cls = 0x57;
				mthd = 0x200;
				break;
			case 13:
				cls = 0x18;
				mthd = 0x200;
				break;
			case 14:
				cls = 0x44;
				mthd = 0x200;
				break;
			case 15:
				cls = 0x19;
				mthd = 0x200;
				break;
			case 16:
				cls = 0x42;
				mthd = 0x200 | (jrand48(ctx->rand48) & 0xfc);
				break;
			case 17:
				cls = 0x52;
				mthd = 0x200 | (jrand48(ctx->rand48) & 0xfc);
				break;
			case 18:
				cls = 0x58;
				mthd = 0x200 | (jrand48(ctx->rand48) & 4);
				break;
			case 19:
				cls = 0x59;
				mthd = 0x200 | (jrand48(ctx->rand48) & 4);
				break;
			case 20:
				cls = 0x5a;
				mthd = 0x200 | (jrand48(ctx->rand48) & 4);
				break;
			case 21:
				cls = 0x5b;
				mthd = 0x200 | (jrand48(ctx->rand48) & 4);
				break;
			case 22:
				cls = 0x1c;
				mthd = 0x200;
				break;
			case 23:
				cls = 0x5c;
				mthd = 0x200;
				break;
			case 24:
				cls = 0x1d;
				mthd = 0x200;
				break;
			case 25:
				cls = 0x5d;
				mthd = 0x200;
				break;
			case 26:
				cls = 0x1e;
				mthd = 0x200;
				break;
			case 27:
				cls = 0x5e;
				mthd = 0x200;
				break;
			case 28:
				cls = 0x1f;
				mthd = 0x200 | (jrand48(ctx->rand48) & 4);
				break;
			case 29:
				cls = 0x5f;
				mthd = 0x200 | (jrand48(ctx->rand48) & 4);
				break;
			case 30:
				cls = 0x21;
				mthd = 0x200;
				break;
			case 31:
				cls = 0x61;
				mthd = 0x200;
				break;
			case 32:
				cls = 0x36;
				mthd = 0x200;
				break;
			case 33:
				cls = 0x76;
				mthd = 0x200;
				break;
			case 34:
				cls = 0x37;
				mthd = 0x200;
				break;
			case 35:
				cls = 0x77;
				mthd = 0x200;
				break;
			case 36:
				cls = 0x60;
				mthd = 0x200;
				break;
			case 37:
				cls = 0x4a;
				mthd = 0x200;
				break;
			case 38:
				cls = 0x4b;
				mthd = 0x200;
				break;
			case 39:
				cls = 0x48;
				mthd = 0x200 | (jrand48(ctx->rand48) & 0xc);
				break;
			case 40:
				cls = 0x64;
				mthd = 0x200;
				if (ctx->chipset.chipset < 5)
					continue;
				break;
			case 41:
				cls = 0x65;
				mthd = 0x200;
				if (ctx->chipset.chipset < 5)
					continue;
				break;
			case 42:
				cls = 0x66;
				mthd = 0x200;
				if (ctx->chipset.chipset < 5)
					continue;
				break;
		}
		if (jrand48(ctx->rand48) & 1)
			val &= 0xf;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.intr |= 0x10;
		exp.fifo_enable = 0;
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_beta(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x12;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x300;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.beta = val;
		if (exp.beta & 0x80000000)
			exp.beta = 0;
		exp.beta &= 0x7f800000;
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_beta4(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x72;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x300;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.beta4 = val;
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_rop(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x43;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x300;
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.rop = val & 0xff;
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_chroma_nv1(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x17;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x304;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		nv04_pgraph_set_chroma_nv01(&exp, val);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_chroma_nv4(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x57;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x304;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.chroma = val;
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_shape(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = (jrand48(ctx->rand48) & 1 ? 0x18 : 0x44);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x308;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1)
			val &= 0xf;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		insrt(exp.pattern_config, 0, 2, val);
		if (extr(exp.debug[3], 20, 1) && val > 2)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_select(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x44;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x30c;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1)
			val &= 0xf;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		uint32_t rv = val & 3;
		insrt(exp.pattern_config, 4, 1, rv > 1);
		if (extr(exp.debug[3], 20, 1) && (val > 2 || val == 0)) {
			nv04_pgraph_blowup(&exp, 2);
		}
		insrt(exp.ctx_valid, 22, 1, !extr(exp.nsource, 1, 1));
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_mono_color_nv1(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x18;
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x310 | idx << 2;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		nv04_pgraph_set_pattern_mono_color_nv01(&exp, idx, val);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_mono_color_nv4(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x44;
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x310 | idx << 2;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.pattern_mono_color[idx] = val;
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_mono_bitmap(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = (jrand48(ctx->rand48) & 1 ? 0x18 : 0x44);
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x318 | idx << 2;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1)
			val &= 0xf;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.pattern_mono_bitmap[idx] = nv04_pgraph_expand_mono(&exp, val);
		if (cls == 0x18) {
			uint32_t fmt = extr(exp.ctx_switch[1], 0, 2);
			if (extr(exp.debug[3], 28, 1) && fmt != 1 && fmt != 2) {
				nv04_pgraph_blowup(&exp, 0x800);
			}
			insrt(exp.ctx_valid, 26+idx, 1, !extr(exp.nsource, 1, 1));
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_color_y8(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x44;
		int idx = jrand48(ctx->rand48) & 0xf;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x400 | idx << 2;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		for (int i = 0; i < 4; i++)
			exp.pattern_color[idx*4+i] = extr(val, 8*i, 8) * 0x010101;
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_color_r5g6b5(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x44;
		int idx = jrand48(ctx->rand48) & 0x1f;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x500 | idx << 2;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		for (int i = 0; i < 2; i++) {
			uint8_t b = extr(val, i * 16 + 0, 5) * 0x21 >> 2;
			uint8_t g = extr(val, i * 16 + 5, 6) * 0x41 >> 4;
			uint8_t r = extr(val, i * 16 + 11, 5) * 0x21 >> 2;
			exp.pattern_color[idx*2+i] = b | g << 8 | r << 16;
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_color_r5g5b5(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x44;
		int idx = jrand48(ctx->rand48) & 0x1f;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x600 | idx << 2;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		for (int i = 0; i < 2; i++) {
			uint8_t b = extr(val, i * 16 + 0, 5) * 0x21 >> 2;
			uint8_t g = extr(val, i * 16 + 5, 5) * 0x21 >> 2;
			uint8_t r = extr(val, i * 16 + 10, 5) * 0x21 >> 2;
			exp.pattern_color[idx*2+i] = b | g << 8 | r << 16;
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_color_r8g8b8(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x44;
		int idx = jrand48(ctx->rand48) & 0x3f;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x700 | idx << 2;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		for (int i = 0; i < 4; i++)
			exp.pattern_color[idx] = extr(val, 0, 24);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_offset(struct hwtest_ctx *ctx) {
	int i;
	uint32_t offset_mask = ctx->chipset.chipset >= 5 ? 0x01fffff0 : 0x00fffff0;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1)
			val &= ~0xf;
		if (jrand48(ctx->rand48) & 1)
			val &= ~0xfc000000;
		uint32_t cls;
		uint32_t mthd;
		int idx;
		bool isnew = false;
		switch (nrand48(ctx->rand48) % 10) {
			case 0:
			default:
				cls = 0x58;
				mthd = 0x30c;
				idx = 0;
				break;
			case 1:
				cls = 0x59;
				mthd = 0x30c;
				idx = 1;
				break;
			case 2:
				cls = 0x5a;
				mthd = 0x30c;
				idx = 2;
				break;
			case 3:
				cls = 0x5b;
				mthd = 0x30c;
				idx = 3;
				break;
			case 4:
				cls = 0x42;
				mthd = 0x308;
				idx = 1;
				isnew = true;
				break;
			case 5:
				cls = 0x42;
				mthd = 0x30c;
				idx = 0;
				isnew = true;
				break;
			case 6:
				cls = 0x52;
				mthd = 0x304;
				idx = 5;
				isnew = true;
				break;
			case 7:
				cls = 0x38;
				mthd = 0x30c;
				idx = 4;
				isnew = true;
				break;
			case 8:
				cls = 0x53;
				mthd = 0x30c;
				idx = 2;
				isnew = true;
				break;
			case 9:
				cls = 0x53;
				mthd = 0x310;
				idx = 3;
				isnew = true;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.surf_offset[idx] = val & offset_mask;
		exp.valid[0] |= 8;
		bool bad = !!(val & 0xf);
		if (isnew)
			bad |= !!(val & 0x1f);
		if (extr(exp.debug[3], 20, 1) && bad) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_pitch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff000f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls;
		uint32_t mthd;
		int idx;
		switch (nrand48(ctx->rand48) % 4) {
			case 0:
			default:
				cls = 0x58;
				mthd = 0x308;
				idx = 0;
				break;
			case 1:
				cls = 0x59;
				mthd = 0x308;
				idx = 1;
				break;
			case 2:
				cls = 0x5a;
				mthd = 0x308;
				idx = 2;
				break;
			case 3:
				cls = 0x5b;
				mthd = 0x308;
				idx = 3;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.surf_pitch[idx] = val & 0x00001ff0;
		exp.valid[0] |= 4;
		bool bad = !!(val & ~0x1ff0) || !(val & 0x1ff0);
		if (extr(exp.debug[3], 20, 1) && bad) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		insrt(exp.ctx_valid, 8+idx, 1, !extr(exp.nsource, 1, 1));
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_pitch_2(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xe00fe00f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff0000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls;
		uint32_t mthd;
		int idx0, idx1;
		switch (nrand48(ctx->rand48) % 2) {
			case 0:
			default:
				cls = 0x42;
				mthd = 0x304;
				idx0 = 1;
				idx1 = 0;
				break;
			case 1:
				cls = 0x53;
				mthd = 0x308;
				idx0 = 2;
				idx1 = 3;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.surf_pitch[idx0] = val & 0x00001ff0;
		exp.surf_pitch[idx1] = val >> 16 & 0x00001ff0;
		exp.valid[0] |= 4;
		bool bad = !!(val & ~0x1fe01fe0) || !(val & 0x1ff0) || !(val & 0x1ff00000);
		if (extr(exp.debug[3], 20, 1) && bad) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		insrt(exp.ctx_valid, 8+idx0, 1, !extr(exp.nsource, 1, 1));
		insrt(exp.ctx_valid, 8+idx1, 1, !extr(exp.nsource, 1, 1));
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_nv3_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x01010003;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls;
		uint32_t mthd;
		int idx;
		switch (nrand48(ctx->rand48) % 4) {
			case 0:
			default:
				cls = 0x58;
				mthd = 0x300;
				idx = 0;
				break;
			case 1:
				cls = 0x59;
				mthd = 0x300;
				idx = 1;
				break;
			case 2:
				cls = 0x5a;
				mthd = 0x300;
				idx = 2;
				break;
			case 3:
				cls = 0x5b;
				mthd = 0x300;
				idx = 3;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		int fmt = 0;
		if (val == 1) {
			fmt = 7;
		} else if (val == 0x01010000) {
			fmt = 1;
		} else if (val == 0x01000000) {
			fmt = 2;
		} else if (val == 0x01010001) {
			fmt = 6;
		} else {
			if (extr(exp.debug[3], 20, 1)) {
				nv04_pgraph_blowup(&exp, 0x2);
			}
		}
		insrt(exp.surf_format, idx*4, 4, fmt);
		nv04_pgraph_mthd(&exp, grobj);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_2d_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x1f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x42;
		uint32_t mthd = 0x300;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		int fmt = 0;
		switch (val & 0xf) {
			case 0x01:
				fmt = 1;
				break;
			case 0x02:
				fmt = 2;
				break;
			case 0x03:
				fmt = 3;
				break;
			case 0x04:
				fmt = 5;
				break;
			case 0x05:
				fmt = 6;
				break;
			case 0x06:
				fmt = 7;
				break;
			case 0x07:
				fmt = 0xb;
				break;
			case 0x08:
				fmt = 0x9;
				break;
			case 0x09:
				fmt = 0xa;
				break;
			case 0x0a:
				fmt = 0xc;
				break;
			case 0x0b:
				fmt = 0xd;
				break;
		}
		if (extr(exp.debug[3], 20, 1) && (val == 0 || val > 0xd)) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		insrt(exp.surf_format, 0, 4, fmt);
		insrt(exp.surf_format, 4, 4, fmt);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_swz_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x0f0f001f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x52;
		uint32_t mthd = 0x300;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		int fmt = 0;
		switch (val & 0xf) {
			case 0x01:
				fmt = 1;
				break;
			case 0x02:
				fmt = 2;
				break;
			case 0x03:
				fmt = 3;
				break;
			case 0x04:
				fmt = 5;
				break;
			case 0x05:
				fmt = 6;
				break;
			case 0x06:
				fmt = 7;
				break;
			case 0x07:
				fmt = 0xb;
				break;
			case 0x08:
				fmt = 0x9;
				break;
			case 0x09:
				fmt = 0xa;
				break;
			case 0x0a:
				fmt = 0xc;
				break;
			case 0x0b:
				fmt = 0xd;
				break;
		}
		int sfmt = extr(val, 0, 16);
		int swzx = extr(val, 16, 4);
		int swzy = extr(val, 24, 4);
		if (extr(exp.debug[3], 20, 1) && (sfmt == 0 || sfmt > 0xd || val & 0xf0f00000 || swzx >= 0xc || swzy >= 0xc || swzx == 0 || swzy == 0)) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		insrt(exp.surf_format, 20, 4, fmt);
		insrt(exp.surf_swizzle[1], 16, 4, swzx);
		insrt(exp.surf_swizzle[1], 24, 4, swzy);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_dvd_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x000fe01f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x38;
		uint32_t mthd = 0x308;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		int fmt = 0;
		switch (val >> 16 & 0xf) {
			case 0x01:
				fmt = 0xe;
				break;
			case 0x02:
				fmt = 0xf;
				break;
		}
		int sfmt = extr(val, 16, 16);
		if (extr(exp.debug[3], 20, 1) && (sfmt == 0 || sfmt > 2 || val & 0xe01f || !(val & 0x1ff0))) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		exp.valid[0] |= 4;
		exp.surf_pitch[4] = val & 0x1ff0;
		insrt(exp.ctx_valid, 12, 1, !extr(exp.nsource, 1, 1));
		insrt(exp.surf_format, 16, 4, fmt);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_3d_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x0f0f031f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x53;
		uint32_t mthd = 0x300;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		int fmt = 0;
		switch (val & 0xf) {
			case 1:
				fmt = 0x2;
				break;
			case 2:
				fmt = 0x3;
				break;
			case 3:
				fmt = 0x5;
				break;
			case 4:
				fmt = 0x7;
				break;
			case 5:
				fmt = 0xb;
				break;
			case 6:
				fmt = 0x9;
				break;
			case 7:
				fmt = 0xa;
				break;
			case 8:
				fmt = 0xc;
				break;
		}
		int sfmt = extr(val, 0, 8);
		int mode = extr(val, 8, 3);
		int swzx = extr(val, 16, 4);
		int swzy = extr(val, 24, 4);
		if (extr(exp.debug[3], 20, 1) && (sfmt == 0 || sfmt > 8 || val & 0xf0f0fc00 || swzx >= 0xc || swzy >= 0xc || mode == 0 || mode == 3)) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		insrt(exp.surf_format, 8, 4, fmt);
		insrt(exp.surf_type, 0, 2, mode);
		insrt(exp.surf_swizzle[0], 16, 4, swzx);
		insrt(exp.surf_swizzle[0], 24, 4, swzy);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_dma_surf(struct hwtest_ctx *ctx) {
	int i;
	uint32_t offset_mask = ctx->chipset.chipset >= 5 ? 0x01fffff0 : 0x00fffff0;
	for (i = 0; i < 10000; i++) {
		uint32_t cls;
		uint32_t mthd;
		int idx;
		bool isnew = false;
		switch (nrand48(ctx->rand48) % 10) {
			case 0:
			default:
				cls = 0x58;
				mthd = 0x184;
				idx = 0;
				break;
			case 1:
				cls = 0x59;
				mthd = 0x184;
				idx = 1;
				break;
			case 2:
				cls = 0x5a;
				mthd = 0x184;
				idx = 2;
				break;
			case 3:
				cls = 0x5b;
				mthd = 0x184;
				idx = 3;
				break;
			case 4:
				cls = 0x42;
				mthd = 0x184;
				idx = 1;
				isnew = true;
				break;
			case 5:
				cls = 0x42;
				mthd = 0x188;
				idx = 0;
				isnew = true;
				break;
			case 6:
				cls = 0x52;
				mthd = 0x184;
				idx = 5;
				isnew = true;
				break;
			case 7:
				cls = 0x38;
				mthd = 0x18c;
				idx = 4;
				isnew = true;
				break;
			case 8:
				cls = 0x53;
				mthd = 0x184;
				idx = 2;
				isnew = true;
				break;
			case 9:
				cls = 0x53;
				mthd = 0x188;
				idx = 3;
				isnew = true;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t dma[3];
		uint32_t val = nv04_pgraph_gen_dma(ctx, &orig, dma);
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		uint32_t base = (dma[2] & ~0xfff) | extr(dma[0], 20, 12);
		uint32_t limit = dma[1];
		uint32_t dcls = extr(dma[0], 0, 12);
		exp.surf_limit[idx] = (limit & offset_mask) | 0xf | (dcls == 0x30) << 31;
		exp.surf_base[idx] = base & offset_mask;
		bool bad = true;
		if (dcls == 0x30 || dcls == 0x3d)
			bad = false;
		if (dcls == 3 && cls == 0x38)
			bad = false;
		if (dcls == 2 && cls == 0x42 && idx == 1)
			bad = false;
		if (extr(exp.debug[3], 23, 1) && bad) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		bool prot_err = false;
		if (extr(base, 0, isnew ? 5 : 4))
			prot_err = true;
		if (extr(dma[0], 16, 2))
			prot_err = true;
		if (ctx->chipset.chipset >= 5 && (bad || dcls == 0x30))
			prot_err = false;
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		insrt(exp.ctx_valid, idx, 1, dcls != 0x30 && !(bad && extr(exp.debug[3], 23, 1)));
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("DMA %08x %08x %08x\n", dma[0], dma[1], dma[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_clip(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t cls, mthd;
		int which;
		switch (nrand48(ctx->rand48) % 13) {
			default:
				cls = 0x19;
				mthd = 0x300 + idx * 4;
				which = 0;
				break;
			case 1:
				cls = 0x36;
				mthd = 0x310 + idx * 4;
				which = 2;
				break;
			case 2:
				cls = 0x76;
				mthd = 0x310 + idx * 4;
				which = 2;
				break;
			case 3:
				if (ctx->chipset.chipset < 5)
					continue;
				cls = 0x66;
				mthd = 0x310 + idx * 4;
				which = 2;
				break;
			case 4:
				cls = 0x37;
				mthd = 0x308 + idx * 4;
				which = 1;
				break;
			case 5:
				cls = 0x77;
				mthd = 0x308 + idx * 4;
				which = 1;
				break;
			case 6:
				cls = 0x4b;
				mthd = 0x7f4 + idx * 4;
				which = 3;
				break;
			case 7:
				cls = 0x4b;
				mthd = 0xbec + idx * 4;
				which = 3;
				break;
			case 8:
				cls = 0x4b;
				mthd = 0xfe8 + idx * 4;
				which = 3;
				break;
			case 9:
				cls = 0x4b;
				mthd = 0x13e4 + idx * 4;
				which = 3;
				break;
			case 10:
				cls = 0x4a;
				mthd = 0x5f4 + idx * 4;
				which = 3;
				break;
			case 11:
				cls = 0x4a;
				mthd = 0x7ec + idx * 4;
				which = 3;
				break;
			case 12:
				cls = 0x4a;
				mthd = 0xbe4 + idx * 4;
				which = 3;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_x[13], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_y[13], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_x[9], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_y[9], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
		}
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		nv04_pgraph_set_clip(&exp, which, idx, val);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_clip_zero_size(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		cls = 0x53;
		mthd = 0x304;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_x[13], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_y[13], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_x[9], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_y[9], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
		}
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.vtx_x[13] = extr(val, 0, 16);
		exp.vtx_y[13] = extr(val, 16, 16);
		int xcstat = nv04_pgraph_clip_status(&exp, exp.vtx_x[13], 0);
		insrt(exp.xy_clip[0][0], 4, 4, xcstat);
		int ycstat = nv04_pgraph_clip_status(&exp, exp.vtx_y[13], 1);
		insrt(exp.xy_clip[1][0], 4, 4, ycstat);
		exp.uclip_min[0] = 0;
		exp.uclip_min[1] = 0;
		exp.uclip_max[0] = exp.vtx_x[13];
		exp.uclip_max[1] = exp.vtx_y[13];
		insrt(exp.valid[0], 28, 1, 0);
		insrt(exp.valid[0], 30, 1, 0);
		insrt(exp.xy_misc_1[0], 4, 2, 0);
		insrt(exp.xy_misc_1[0], 12, 1, 0);
		insrt(exp.xy_misc_1[0], 16, 1, 0);
		insrt(exp.xy_misc_1[0], 20, 1, 0);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_clip_hv(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset < 5)
		return HWTEST_RES_NA;
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int xy = jrand48(ctx->rand48) & 1;
		cls = 0x53;
		mthd = 0x2f8 + xy * 4;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_x[13], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_y[13], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_x[9], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_y[9], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
		}
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		if (extr(exp.debug[3], 25, 1)) {
			insrt(exp.valid[0], 28, 1, 0);
			insrt(exp.valid[0], 30, 1, 0);
			insrt(exp.valid[0], 19, 1, 0);
			insrt(exp.xy_misc_1[0], 4+xy, 1, 0);
			insrt(exp.xy_misc_1[0], 12, 1, 0);
			insrt(exp.xy_misc_1[0], 16, 1, 0);
			insrt(exp.xy_misc_1[0], 20, 1, 0);
			insrt(exp.xy_misc_1[1], 0, 1, 1);
			insrt(exp.xy_misc_1[0], 0, 1, 0);
			insrt(exp.xy_misc_3, 8, 1, 0);
			uint32_t min = extr(val, 0, 16);
			uint32_t max = min + extrs(val, 16, 16);
			(xy ? exp.vtx_y : exp.vtx_x)[13] = min;
			int cstat = nv04_pgraph_clip_status(&exp, min, xy);
			insrt(exp.xy_clip[xy][0], 0, 4, cstat);
			if (!xy) {
				exp.uclip_min[xy] = exp.uclip_max[xy] & 0xffff;
				exp.uclip_max[xy] = min & 0x3ffff;
				exp.vtx_x[13] = exp.vtx_y[13] = max;
				int xcstat = nv04_pgraph_clip_status(&exp, max, 0);
				insrt(exp.xy_clip[0][0], 4, 4, xcstat);
				int ycstat = nv04_pgraph_clip_status(&exp, max, 1);
				insrt(exp.xy_clip[1][0], 4, 4, ycstat);
			}
			exp.uclip_min[xy] = min;
			exp.uclip_max[xy] = max & 0x3ffff;
			if (extr(exp.debug[3], 20, 1) && extr(val, 15, 1))
				nv04_pgraph_blowup(&exp, 2);
		} else {
			nv04_pgraph_blowup(&exp, 0x40);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_solid_color(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int which = 0;
		switch (nrand48(ctx->rand48) % 18) {
			default:
				cls = (jrand48(ctx->rand48) & 1 ? 0x5c : 0x1c);
				mthd = 0x304;
				break;
			case 1:
				cls = (jrand48(ctx->rand48) & 1 ? 0x5c : 0x1c);
				mthd = 0x600 | (jrand48(ctx->rand48) & 0x78);
				break;
			case 2:
				cls = (jrand48(ctx->rand48) & 1 ? 0x5d : 0x1d);
				mthd = 0x304;
				break;
			case 3:
				cls = (jrand48(ctx->rand48) & 1 ? 0x5d : 0x1d);
				mthd = 0x500 | (jrand48(ctx->rand48) & 0x70);
				break;
			case 4:
				cls = (jrand48(ctx->rand48) & 1 ? 0x5d : 0x1d);
				mthd = 0x580 | (jrand48(ctx->rand48) & 0x78);
				break;
			case 5:
				cls = (jrand48(ctx->rand48) & 1 ? 0x5e : 0x1e);
				mthd = 0x304;
				break;
			case 6:
				cls = (jrand48(ctx->rand48) & 1 ? 0x4a : 0x4b);
				mthd = 0x3fc;
				break;
			case 7:
				cls = 0x4b;
				mthd = 0x7fc;
				break;
			case 8:
				cls = 0x4a;
				mthd = 0x5fc;
				break;
			case 9:
				cls = 0x4b;
				mthd = 0xbf4;
				which = 1;
				break;
			case 10:
				cls = 0x4b;
				mthd = 0xff0;
				which = 1;
				break;
			case 11:
				cls = 0x4b;
				mthd = 0x13f0;
				which = 1;
				break;
			case 12:
				cls = 0x4a;
				mthd = 0x7f4;
				which = 1;
				break;
			case 13:
				cls = 0x4a;
				mthd = 0xbf0;
				which = 1;
				break;
			case 14:
				cls = 0x4a;
				mthd = 0xffc;
				which = 1;
				break;
			case 15:
				cls = 0x4a;
				mthd = 0x17fc;
				which = 1;
				break;
			case 16:
				cls = 0x4b;
				mthd = 0x13ec;
				which = 2;
				break;
			case 17:
				cls = 0x4a;
				mthd = 0xbec;
				which = 3;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		if (which == 0) {
			exp.misc32[0] = val;
			insrt(exp.valid[0], 16, 1, 1);
		} else if (which == 1) {
			exp.misc32[1] = val;
			insrt(exp.valid[0], 18, 1, 1);
		} else if (which == 2) {
			nv04_pgraph_set_bitmap_color_0_nv01(&exp, val);
			insrt(exp.valid[0], 17, 1, 1);
		} else if (which == 3) {
			exp.bitmap_color_0 = val;
			insrt(exp.valid[0], 17, 1, 1);
			if (ctx->chipset.chipset < 5)
				insrt(exp.ctx_format, 0, 8, extr(exp.ctx_switch[1], 8, 8));
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifm_pitch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				cls = 0x37;
				mthd = 0x404;
				break;
			case 1:
				cls = 0x77;
				mthd = 0x404;
				break;
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xfefc000f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		uint32_t pitch = extr(val, 0, 16);
		exp.dma_pitch = pitch * 0x10001;
		insrt(exp.valid[0], 10, 1, 1);
		bool bad = false;
		if (cls != 0x37) {
			insrt(exp.sifm_mode, 16, 2, extr(val, 16, 2));
			insrt(exp.sifm_mode, 24, 1, extr(val, 24, 1));
			if (!extr(val, 16, 8))
				bad = true;
			if (extr(val, 16, 8) > 2)
				bad = true;
			if (extr(val, 24, 8) > 1)
				bad = true;
		} else {
			if (extr(val, 16, 16))
				bad = true;
		}
		if (pitch & ~0x1fff)
			bad = true;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_dvd_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x38;
		int which = jrand48(ctx->rand48) & 1;
		uint32_t mthd = which ? 0x334 : 0x31c;
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xfffc000f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		uint32_t pitch = extr(val, 0, 16);
		insrt(exp.dma_pitch, 16 * which, 16, pitch);
		insrt(exp.valid[0], which ? 16 : 10, 1, 1);
		bool bad = false;
		int sfmt = extr(val, 16, 2);
		int fmt = 0;
		if (which == 0) {
			if (sfmt == 1)
				fmt = 0x12;
			if (sfmt == 2)
				fmt = 0x13;
			insrt(exp.dvd_format, 0, 6, fmt);
		} else {
			fmt = sfmt;
			insrt(exp.dvd_format, 8, 2, fmt);
		}
		if (!fmt || extr(val, 18, 14))
			bad = true;
		if (pitch & ~0x1fff)
			bad = true;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_index_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x00000001;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls;
		uint32_t mthd = 0x3ec;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				cls = 0x60;
				break;
			case 1:
				cls = 0x64;
				if (ctx->chipset.chipset < 5)
					cls = 0x60;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		insrt(exp.dvd_format, 0, 6, val & 1);
		if (extr(exp.debug[3], 20, 1) && val > 1)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_dma_offset(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int which;
		int vbit;
		switch (nrand48(ctx->rand48) % 8) {
			default:
				cls = 0x37;
				mthd = 0x408;
				which = 0;
				vbit = 11;
				break;
			case 1:
				cls = 0x77;
				mthd = 0x408;
				which = 0;
				vbit = 11;
				break;
			case 2:
				cls = 0x39;
				mthd = 0x30c;
				which = 0;
				vbit = 0;
				break;
			case 3:
				cls = 0x39;
				mthd = 0x310;
				which = 1;
				vbit = 1;
				break;
			case 4:
				cls = 0x38;
				mthd = 0x320;
				which = 0;
				vbit = 11;
				break;
			case 5:
				cls = 0x38;
				mthd = 0x338;
				which = 1;
				vbit = 17;
				break;
			case 6:
				cls = 0x60;
				mthd = 0x3f0;
				which = 0;
				vbit = 22;
				break;
			case 7:
				cls = 0x64;
				mthd = 0x3f0;
				which = 0;
				vbit = 22;
				if (ctx->chipset.chipset < 5)
					continue;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.dma_offset[which] = val;
		insrt(exp.valid[0], vbit, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_pitch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff0000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x39, mthd;
		int which;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				mthd = 0x314;
				which = 0;
				break;
			case 1:
				mthd = 0x318;
				which = 1;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		insrt(exp.dma_pitch, which * 16, 16, val);
		insrt(exp.valid[0], 2+which, 1, 1);
		bool bad = false;
		if (val & ~0x7fff)
			bad = true;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_line_length(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xff000000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x39, mthd = 0x31c;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		insrt(exp.valid[0], 4, 1, 1);
		bool bad = false;
		if (val & ~0x3fffff)
			bad = true;
		exp.dma_length = val & 0x3fffff;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_line_count(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xfffff000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x39, mthd = 0x320;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		insrt(exp.valid[0], 5, 1, 1);
		bool bad = false;
		if (val & ~0x7ff)
			bad = true;
		insrt(exp.dma_misc, 0, 16, val & 0x7ff);
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xfffff0f0;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x39, mthd = 0x324;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		insrt(exp.valid[0], 6, 1, 1);
		bool bad = false;
		int fmtx = extr(val, 0, 3);
		int fmty = extr(val, 8, 3);
		if (fmtx != 1 && fmtx != 2 && fmtx != 4)
			bad = true;
		if (fmty != 1 && fmty != 2 && fmty != 4)
			bad = true;
		if (val & 0xf8)
			bad = true;
		if (val & 0xfffff800 && ctx->chipset.chipset >= 5)
			bad = true;
		insrt(exp.dma_misc, 16, 3, extr(val, 0, 3));
		insrt(exp.dma_misc, 20, 3, extr(val, 8, 3));
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_font(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		int which = jrand48(ctx->rand48) & 1;
		uint32_t cls = 0x4a, mthd = which ? 0x17f0 : 0xff0;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.dma_offset[0] = val;
		int pitch = extr(val, 28, 4);
		switch (pitch) {
			default:
				exp.dma_length = 1 << pitch;
				break;
			case 0:
				exp.dma_length = 0x18;
				break;
			case 1:
				exp.dma_length = 0x28;
				break;
			case 2:
				exp.dma_length = 0x48;
				break;
			case 0xb:
			case 0xd:
				exp.dma_length = 0x200;
				break;
			case 0xf:
				exp.dma_length = 0x280;
				break;
			case 0xc:
				exp.dma_length = 0x100;
				break;
			case 0xa:
			case 0xe:
				exp.dma_length = 0x140;
				break;
		}
		insrt(exp.valid[0], 22, 1, 1);
		if (extr(exp.debug[3], 20, 1) && (pitch < 3 || pitch > 9))
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_color_key(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x54, mthd = 0x300;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.misc32[2] = val;
		insrt(exp.valid[1], 12, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_offset(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0x3ff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int which;
		switch (nrand48(ctx->rand48) % 4) {
			default:
				cls = 0x48;
				mthd = 0x304;
				which = 3;
				break;
			case 1:
				cls = 0x54;
				mthd = 0x304;
				which = 3;
				break;
			case 2:
				cls = 0x55;
				mthd = 0x308;
				which = 1;
				break;
			case 3:
				cls = 0x55;
				mthd = 0x30c;
				which = 2;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		if (which & 1) {
			exp.misc32[0] = val;
			insrt(exp.valid[1], 14, 1, 1);
		}
		if (which & 2) {
			exp.misc32[1] = val;
			insrt(exp.valid[1], 22, 1, 1);
		}
		if (extr(exp.debug[3], 20, 1) && (val & 0xff) && cls != 0x48)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_format_d3d0(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 22, 2, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 17, 3, 0);
		}
		uint32_t cls = 0x48, mthd = 0x308;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		exp.misc32[2] = val;
		int fmt = 5;
		int sfmt = extr(val, 20, 4);
		if (sfmt == 0)
			fmt = 2;
		else if (sfmt == 1)
			fmt = 3;
		else if (sfmt == 2)
			fmt = 4;
		else if (sfmt == 3)
			fmt = 5;
		else
			bad = true;
		int max_l = extr(val, 28, 4);
		int min_l = extr(val, 24, 4);
		if (min_l < 2)
			bad = true;
		if (min_l > max_l)
			bad = true;
		if (max_l > 0xb)
			bad = true;
		if (extr(val, 17, 3))
			bad = true;
		for (int i = 0; i < 2; i++) {
			insrt(exp.d3d_tex_format[i], 1, 1, 0);
			insrt(exp.d3d_tex_format[i], 2, 1, extr(val, 16, 1));
			insrt(exp.d3d_tex_format[i], 7, 1, 0);
			insrt(exp.d3d_tex_format[i], 8, 3, fmt);
			insrt(exp.d3d_tex_format[i], 12, 4, max_l - min_l + 1);
			insrt(exp.d3d_tex_format[i], 16, 4, max_l);
			insrt(exp.d3d_tex_format[i], 20, 4, max_l);
		}
		insrt(exp.valid[1], 21, 1, 1);
		insrt(exp.valid[1], 15, 1, 1);
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 0, 2, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 2, 2, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 4, 2, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 6, 2, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 11, 1, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 24, 3, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 28, 3, 1);
		}
		uint32_t cls, mthd;
		int which;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				cls = 0x54;
				mthd = 0x308;
				which = 3;
				break;
			case 1:
				cls = 0x55;
				mthd = 0x310;
				which = 1;
				break;
			case 2:
				cls = 0x55;
				mthd = 0x314;
				which = 2;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (!extr(val, 0, 4) || extr(val, 0, 4) == 3)
			bad = true;
		if (!extr(val, 4, 2) || extr(val, 4, 2) == 3)
			bad = true;
		if (!extr(val, 6, 2) || extr(val, 6, 2) == 3)
			bad = true;
		if (!extr(val, 8, 3))
			bad = true;
		if (extr(val, 11, 1))
			bad = true;
		if (!extr(val, 12, 4))
			bad = true;
		if (extr(val, 16, 4) > 0xb)
			bad = true;
		if (extr(val, 20, 4) > 0xb)
			bad = true;
		if (extr(val, 24, 3) < 1 || extr(val, 24, 3) > 4)
			bad = true;
		if (extr(val, 28, 3) < 1 || extr(val, 28, 3) > 4)
			bad = true;
		uint32_t rval = val & 0xfffff7a6;
		if (cls == 0x54) {
			if (extr(val, 8, 3) == 1)
				insrt(rval, 8, 3, 0);
			if (extr(val, 2, 2) > 1)
				bad = true;
		} else {
			if (extr(val, 2, 2))
				bad = true;
		}
		if (which & 1) {
			insrt(exp.valid[1], 15, 1, 1);
			exp.d3d_tex_format[0] = rval;
		}
		if (which & 2) {
			insrt(exp.valid[1], 21, 1, 1);
			exp.d3d_tex_format[1] = rval;
		}
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_filter_d3d0(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x48, mthd = 0x30c;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		for (int i = 0; i < 2; i++) {
			insrt(exp.d3d_tex_filter[i], 1, 4, extr(val, 1, 4));
			insrt(exp.d3d_tex_filter[i], 9, 4, extr(val, 9, 4));
			insrt(exp.d3d_tex_filter[i], 15, 1, 0);
			insrt(exp.d3d_tex_filter[i], 16, 8, extrs(val, 17, 7));
			insrt(exp.d3d_tex_filter[i], 27, 1, 1);
			insrt(exp.d3d_tex_filter[i], 31, 1, 1);
		}
		insrt(exp.valid[1], 23, 1, 1);
		insrt(exp.valid[1], 16, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_filter(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 5, 3, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 13, 2, 0);
		}
		uint32_t cls, mthd;
		int which;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				cls = 0x54;
				mthd = 0x30c;
				which = 3;
				break;
			case 1:
				cls = 0x55;
				mthd = 0x318;
				which = 1;
				break;
			case 2:
				cls = 0x55;
				mthd = 0x31c;
				which = 2;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (extr(val, 13, 2))
			bad = true;
		if (extr(val, 5, 3))
			bad = true;
		if (extr(val, 24, 3) == 0 || extr(val, 24, 3) > 6)
			bad = true;
		if (extr(val, 28, 3) == 0 || extr(val, 28, 3) > 6)
			bad = true;
		bool is_mip = extr(val, 24, 3) == 5 || extr(val, 24, 3) == 6;
		uint32_t rval = val & 0xffff9e1e;
		if (which & 1) {
			insrt(exp.valid[1], 16, 1, 1);
			exp.d3d_tex_filter[0] = rval;
		}
		if (cls == 0x54 && is_mip && !extr(val, 15, 1)) {
			int bias = extr(val, 16, 8);
			if (bias < 0x78 || bias >= 0x80)
				bias += 8;
			insrt(rval, 16, 8, bias);
		}
		if (which & 2) {
			insrt(exp.valid[1], 23, 1, 1);
			exp.d3d_tex_filter[1] = rval;
		}
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_blend(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 9, 3, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 13, 3, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 17, 3, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 21, 3, 0);
			if (jrand48(ctx->rand48) & 1)
				insrt(val, 0, 4, 0);
		}
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				cls = 0x54;
				mthd = 0x310;
				break;
			case 1:
				cls = 0x55;
				mthd = 0x338;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (cls == 0x54) {
			if (extr(val, 0, 4) < 1 || extr(val, 0, 4) > 8)
				bad = true;
		} else {
			if (extr(val, 0, 4))
				bad = true;
		}
		if (extr(val, 4, 2) < 1 || extr(val, 4, 2) > 2)
			bad = true;
		if (!extr(val, 6, 2))
			bad = true;
		if (extr(val, 9, 3))
			bad = true;
		if (extr(val, 13, 3))
			bad = true;
		if (extr(val, 17, 3))
			bad = true;
		if (extr(val, 21, 3))
			bad = true;
		if (extr(val, 24, 4) < 1 || extr(val, 24, 4) > 0xb)
			bad = true;
		if (extr(val, 28, 4) < 1 || extr(val, 28, 4) > 0xb)
			bad = true;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		uint32_t rval = val & 0xff1111ff;
		if (cls == 0x55)
			insrt(rval, 0, 4, 0);
		exp.d3d_blend = rval;
		insrt(exp.valid[1], 20, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_config(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 8, 4, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 15, 1, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 16, 4, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 25, 5, 0);
		}
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				cls = 0x54;
				mthd = 0x314;
				break;
			case 1:
				cls = 0x55;
				mthd = 0x33c;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (extr(val, 8, 4) < 1 || extr(val, 8, 4) > 8)
			bad = true;
		if (extr(val, 15, 1))
			bad = true;
		if (extr(val, 16, 4) < 1 || extr(val, 16, 4) > 8)
			bad = true;
		if (extr(val, 20, 2) < 1)
			bad = true;
		if (extr(val, 25, 5) && cls == 0x54)
			bad = true;
		if (extr(val, 30, 2) < 1 || extr(val, 30, 2) > 2)
			bad = true;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		uint32_t rval = val & 0xffff5fff;
		if (cls == 0x54)
			insrt(rval, 25, 5, 0x1e);
		exp.d3d_config = rval;
		insrt(exp.dvd_format, 0, 8, extr(val, 13, 1));
		insrt(exp.valid[1], 17, 1, 1);
		if (cls == 0x54) {
			exp.d3d_stencil_func = 0x80;
			exp.d3d_stencil_op = 0x222;
			insrt(exp.valid[1], 18, 1, 1);
			insrt(exp.valid[1], 19, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_config_d3d0(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 0, 4, 1);
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x48, mthd = 0x314;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (extr(val, 9, 1))
			bad = true;
		if (!extr(val, 12, 2))
			bad = true;
		if (extr(val, 14, 1))
			bad = true;
		if (extr(val, 16, 4) < 1 || extr(val, 16, 4) > 8)
			bad = true;
		if (extr(val, 20, 4) > 4)
			bad = true;
		if (extr(val, 24, 4) > 4)
			bad = true;
		exp.misc32[3] = val;
		int filt = 1;
		int origin = 0;
		int sinterp = extr(val, 0, 4);
		if (sinterp == 0)
			origin = 2;
		if (sinterp == 2)
			filt = 2;
		if (sinterp > 2)
			bad = true;
		int wrapu = extr(val, 4, 2);
		if (wrapu == 0)
			wrapu = 9;
		int wrapv = extr(val, 6, 2);
		if (wrapv == 0)
			wrapv = 9;
		int scull = extr(val, 12, 2);
		int cull = 0;
		if (scull == 0) {
			cull = 1;
		} else if (scull == 1) {
			cull = 1;
		} else if (scull == 2) {
			cull = 3;
		} else if (scull == 3) {
			cull = 2;
		}
		int dblend = 6;
		int sblend = 5;
		if (extr(val, 29, 1)) {
			dblend = 0xa;
			sblend = 0x9;
		}
		if (extr(val, 28, 1))
			dblend = sblend = 2;
		if (extr(val, 30, 1))
			dblend = 1;
		if (extr(val, 31, 1))
			sblend = 1;
		for (int i = 0; i < 2; i++) {
			insrt(exp.d3d_tex_format[i], 4, 2, origin);
			insrt(exp.d3d_tex_format[i], 24, 4, wrapu);
			insrt(exp.d3d_tex_format[i], 28, 4, wrapv);
			insrt(exp.d3d_tex_filter[i], 24, 3, filt);
			insrt(exp.d3d_tex_filter[i], 28, 3, filt);
		}
		insrt(exp.d3d_blend, 4, 2, 1);
		insrt(exp.d3d_blend, 6, 2, 2);
		insrt(exp.d3d_blend, 8, 1, 1);
		insrt(exp.d3d_blend, 12, 1, 0);
		insrt(exp.d3d_blend, 16, 1, 1);
		insrt(exp.d3d_blend, 20, 1, 1);
		insrt(exp.d3d_blend, 24, 4, sblend);
		insrt(exp.d3d_blend, 28, 4, dblend);
		insrt(exp.d3d_config, 14, 1, !!extr(val, 20, 3));
		insrt(exp.d3d_config, 16, 4, extr(val, 16, 4));
		insrt(exp.d3d_config, 20, 2, cull);
		insrt(exp.d3d_config, 22, 1, 1);
		insrt(exp.d3d_config, 23, 1, extr(val, 15, 1));
		insrt(exp.d3d_config, 24, 1, !!extr(val, 20, 3));
		insrt(exp.d3d_config, 25, 1, 0);
		insrt(exp.d3d_config, 26, 1, 0);
		insrt(exp.d3d_config, 27, 3, extr(val, 24, 3) ? 0x7 : 0);
		insrt(exp.d3d_config, 30, 2, 1);
		exp.d3d_stencil_func = 0x80;
		exp.d3d_stencil_op = 0x222;
		insrt(exp.valid[1], 17, 1, 1);
		insrt(exp.valid[1], 24, 1, 1);
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_alpha_d3d0(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 8, 4, 1);
			val &= ~0xfffff000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x48, mthd = 0x318;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (extr(val, 8, 4) < 1 || extr(val, 8, 4) > 8)
			bad = true;
		if (extr(val, 12, 20))
			bad = true;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		insrt(exp.d3d_config, 0, 12, val);
		insrt(exp.d3d_config, 12, 1, 1);
		insrt(exp.valid[1], 18, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_stencil_func(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x55, mthd = 0x340;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (extr(val, 1, 3))
			bad = true;
		if (extr(val, 4, 4) < 1 || extr(val, 4, 4) > 8)
			bad = true;
		exp.d3d_stencil_func = val & 0xfffffff1;
		insrt(exp.valid[1], 18, 1, 1);
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_stencil_op(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xfffff000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x55, mthd = 0x344;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (extr(val, 0, 4) < 1 || extr(val, 0, 4) > 8)
			bad = true;
		if (extr(val, 4, 4) < 1 || extr(val, 4, 4) > 8)
			bad = true;
		if (extr(val, 8, 4) < 1 || extr(val, 8, 4) > 8)
			bad = true;
		if (extr(val, 12, 20))
			bad = true;
		exp.d3d_stencil_op = val & 0xfff;
		insrt(exp.valid[1], 19, 1, 1);
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_fog_color(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				cls = 0x48;
				mthd = 0x310;
				break;
			case 1:
				cls = 0x54;
				mthd = 0x318;
				break;
			case 2:
				cls = 0x55;
				mthd = 0x348;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.misc24[0] = val & 0xffffff;
		insrt(exp.valid[1], 13, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_rc_cw(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0x00e0e0e0;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		int idx = jrand48(ctx->rand48) & 1;
		int ac = jrand48(ctx->rand48) & 1;
		uint32_t cls = 0x55, mthd = 0x320 + idx * 0xc + ac * 4;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (val & 0x00e0e0e0)
			bad = true;
		if (!extr(val, 2, 3))
			bad = true;
		if (!extr(val, 10, 3))
			bad = true;
		if (!extr(val, 18, 3))
			bad = true;
		if (!extr(val, 26, 3))
			bad = true;
		if (!extr(val, 29, 3))
			bad = true;
		if (idx) {
			if (extr(val, 2, 3) == 7)
				bad = true;
			if (extr(val, 10, 3) == 7)
				bad = true;
			if (extr(val, 18, 3) == 7)
				bad = true;
			if (extr(val, 26, 3) == 7)
				bad = true;
		}
		if (ac)
			exp.d3d_rc_color[idx] = val & 0xff1f1f1f;
		else
			exp.d3d_rc_alpha[idx] = val & 0xfd1d1d1d;
		insrt(exp.valid[1], 28 - idx * 2 - ac, 1, 1);
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_rc_factor(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x55, mthd = 0x334;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.misc32[3] = val;
		insrt(exp.valid[1], 24, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 4) {
			default:
				cls = 0x17;
				mthd = 0x300;
				break;
			case 1:
				cls = 0x18;
				mthd = 0x300;
				break;
			case 2:
				cls = 0x57;
				mthd = 0x300;
				break;
			case 3:
				cls = 0x44;
				mthd = 0x300;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		if (extr(exp.debug[3], 20, 1) && (val > 3 || val == 0))
			nv04_pgraph_blowup(&exp, 2);
		int sfmt = val & 3;
		int fmt = 0;
		if (sfmt == 1) {
			if (cls == 0x44 || cls == 0x57)
				fmt = 0xb;
			else
				fmt = 0x2;
		}
		if (sfmt == 2)
			fmt = 0x8;
		if (sfmt == 3)
			fmt = 0xd;
		if (!extr(exp.nsource, 1, 1)) {
			insrt(egrobj[1], 8, 8, fmt);
			int subc = extr(exp.ctx_user, 13, 3);
			exp.ctx_cache[subc][1] = exp.ctx_switch[1];
			insrt(exp.ctx_cache[subc][1], 8, 8, fmt);
			if (extr(exp.debug[1], 20, 1))
				exp.ctx_switch[1] = exp.ctx_cache[subc][1];
		}
		if (cls == 0x57) {
			insrt(exp.ctx_format, 24, 8, extr(exp.ctx_switch[1], 8, 8));
			insrt(exp.ctx_valid, 17, 1, !extr(exp.nsource, 1, 1));
		}
		if (cls == 0x44) {
			insrt(exp.ctx_format, 8, 8, extr(exp.ctx_switch[1], 8, 8));
			insrt(exp.ctx_format, 16, 8, extr(exp.ctx_switch[1], 8, 8));
			insrt(exp.ctx_valid, 20, 1, !extr(exp.nsource, 1, 1));
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_solid_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		bool isnew;
		switch (nrand48(ctx->rand48) % 8) {
			default:
				cls = 0x1c;
				mthd = 0x300;
				isnew = false;
				break;
			case 1:
				cls = 0x1d;
				mthd = 0x300;
				isnew = false;
				break;
			case 2:
				cls = 0x1e;
				mthd = 0x300;
				isnew = false;
				break;
			case 3:
				cls = 0x4b;
				mthd = 0x300;
				isnew = false;
				break;
			case 4:
				cls = 0x5c;
				mthd = 0x300;
				isnew = true;
				break;
			case 5:
				cls = 0x5d;
				mthd = 0x300;
				isnew = true;
				break;
			case 6:
				cls = 0x5e;
				mthd = 0x300;
				isnew = true;
				break;
			case 7:
				cls = 0x4a;
				mthd = 0x300;
				isnew = true;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = (val > (isnew || cls == 0x4b ? 3 : 4) || val == 0);
		int sfmt = val & 0xf;
		int fmt = 0;
		if (sfmt == 1)
			fmt = isnew ? 0xc : 0x3;
		if (sfmt == 2)
			fmt = 0x9;
		if (sfmt == 3)
			fmt = 0xe;
		if (sfmt == 4 && !isnew)
			fmt = 0x11;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		if (!extr(exp.nsource, 1, 1)) {
			insrt(egrobj[1], 8, 8, fmt);
			int subc = extr(exp.ctx_user, 13, 3);
			exp.ctx_cache[subc][1] = exp.ctx_switch[1];
			insrt(exp.ctx_cache[subc][1], 8, 8, fmt);
			if (extr(exp.debug[1], 20, 1))
				exp.ctx_switch[1] = exp.ctx_cache[subc][1];
		}
		if (cls == 0x4a && ctx->chipset.chipset >= 5) {
			insrt(exp.ctx_format, 0, 8, extr(exp.ctx_switch[1], 8, 8));
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ifc_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		bool isnew;
		switch (nrand48(ctx->rand48) % 8) {
			default:
				cls = 0x21;
				mthd = 0x300;
				isnew = false;
				break;
			case 1:
				cls = 0x61;
				mthd = 0x300;
				isnew = true;
				break;
			case 2:
				cls = 0x65;
				mthd = 0x300;
				isnew = true;
				if (ctx->chipset.chipset < 5)
					cls = 0x61;
				break;
			case 3:
				cls = 0x36;
				mthd = 0x300;
				isnew = false;
				break;
			case 4:
				cls = 0x76;
				mthd = 0x300;
				isnew = true;
				break;
			case 5:
				cls = 0x66;
				mthd = 0x300;
				isnew = true;
				if (ctx->chipset.chipset < 5)
					cls = 0x61;
				break;
			case 6:
				cls = 0x60;
				mthd = 0x3e8;
				isnew = true;
				break;
			case 7:
				cls = 0x64;
				mthd = 0x3e8;
				isnew = true;
				if (ctx->chipset.chipset < 5)
					cls = 0x61;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = (val > 5 || val == 0);
		int sfmt = val & 7;
		int fmt = 0;
		if (sfmt == 1)
			fmt = isnew ? 0xa : 0x1;
		if (sfmt == 2)
			fmt = 0x6;
		if (sfmt == 3)
			fmt = 0x7;
		if (sfmt == 4)
			fmt = 0xd;
		if (sfmt == 5)
			fmt = 0xe;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		if (!extr(exp.nsource, 1, 1)) {
			insrt(egrobj[1], 8, 8, fmt);
			int subc = extr(exp.ctx_user, 13, 3);
			exp.ctx_cache[subc][1] = exp.ctx_switch[1];
			insrt(exp.ctx_cache[subc][1], 8, 8, fmt);
			if (extr(exp.debug[1], 20, 1))
				exp.ctx_switch[1] = exp.ctx_cache[subc][1];
		}
		if (cls == 0x4a) {
			insrt(exp.ctx_format, 0, 8, extr(exp.ctx_switch[1], 8, 8));
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifm_src_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		bool isnew;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				cls = 0x37;
				mthd = 0x300;
				isnew = false;
				break;
			case 1:
				cls = 0x77;
				mthd = 0x300;
				isnew = true;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = (val > (isnew ? 7 : 6) || val == 0);
		int sfmt = val & 7;
		int fmt = 0;
		if (sfmt == 1)
			fmt = 0x6;
		if (sfmt == 2)
			fmt = 0x7;
		if (sfmt == 3)
			fmt = 0xd;
		if (sfmt == 4)
			fmt = 0xe;
		if (sfmt == 5)
			fmt = 0x12;
		if (sfmt == 6)
			fmt = 0x13;
		if (sfmt == 7)
			fmt = 0xa;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		if (!extr(exp.nsource, 1, 1)) {
			insrt(egrobj[1], 8, 8, fmt);
			int subc = extr(exp.ctx_user, 13, 3);
			exp.ctx_cache[subc][1] = exp.ctx_switch[1];
			insrt(exp.ctx_cache[subc][1], 8, 8, fmt);
			if (extr(exp.debug[1], 20, 1))
				exp.ctx_switch[1] = exp.ctx_cache[subc][1];
		}
		if (cls == 0x4a) {
			insrt(exp.ctx_format, 0, 8, extr(exp.ctx_switch[1], 8, 8));
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_mono_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		bool is_ctx = false;
		switch (nrand48(ctx->rand48) % 4) {
			default:
				cls = 0x18;
				mthd = 0x304;
				break;
			case 1:
				is_ctx = true;
				cls = 0x44;
				mthd = 0x304;
				break;
			case 2:
				cls = 0x4b;
				mthd = 0x304;
				break;
			case 3:
				cls = 0x4a;
				mthd = 0x304;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		if (extr(exp.debug[3], 20, 1) && (val > 2 || val == 0))
			nv04_pgraph_blowup(&exp, 2);
		if (!extr(exp.nsource, 1, 1)) {
			int fmt = val & 3;
			insrt(egrobj[1], 0, 8, fmt);
			int subc = extr(exp.ctx_user, 13, 3);
			exp.ctx_cache[subc][1] = exp.ctx_switch[1];
			insrt(exp.ctx_cache[subc][1], 0, 2, fmt);
			if (is_ctx)
				insrt(exp.ctx_valid, 21, 1, 1);
			if (extr(exp.debug[1], 20, 1))
				exp.ctx_switch[1] = exp.ctx_cache[subc][1];
		} else {
			if (is_ctx)
				insrt(exp.ctx_valid, 21, 1, 0);
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_dma_grobj(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t cls, mthd;
		int which, mode;
		bool is3d = false;
		bool clr = false;
		switch (nrand48(ctx->rand48) % 11) {
			default:
				cls = get_random_class(ctx);
				mthd = 0x180;
				which = 2;
				mode = 3;
				break;
			case 1:
				cls = 0x37;
				mthd = 0x184;
				which = 0;
				mode = 2;
				break;
			case 2:
				cls = 0x77;
				mthd = 0x184;
				which = 0;
				mode = 2;
				break;
			case 3:
				cls = 0x4a;
				mthd = 0x184;
				which = 0;
				mode = 2;
				break;
			case 4:
				cls = 0x60;
				mthd = 0x184;
				which = 0;
				mode = 2;
				break;
			case 5:
				cls = 0x64;
				if (ctx->chipset.chipset < 5)
					cls = 0x60;
				mthd = 0x184;
				which = 0;
				mode = 2;
				break;
			case 6:
				which = jrand48(ctx->rand48) & 1;
				cls = 0x38;
				mthd = 0x188 - which * 4;
				mode = 2;
				break;
			case 7:
				which = jrand48(ctx->rand48) & 1;
				cls = 0x39;
				mthd = 0x184 + which * 4;
				mode = which ? 3 : 2;
				clr = !which;
				break;
			case 8:
				cls = 0x48;
				mthd = 0x184;
				which = 0;
				mode = 2;
				is3d = true;
				break;
			case 9:
				cls = 0x54;
				mthd = 0x184;
				which = 0;
				mode = 2;
				is3d = true;
				break;
			case 10:
				which = jrand48(ctx->rand48) & 1;
				cls = 0x55;
				mthd = 0x184 + which * 4;
				mode = 2;
				is3d = true;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t dma[3];
		uint32_t val = nv04_pgraph_gen_dma(ctx, &orig, dma);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		uint32_t rval = val & 0xffff;
		int dcls = extr(dma[0], 0, 12);
		if (dcls == 0x30)
			rval = 0;
		bool bad = false;
		if (dcls != 0x30 && dcls != 0x3d && dcls != mode)
			bad = true;
		if (!extr(exp.nsource, 1, 1)) {
			if (which == 2)
				insrt(egrobj[1], 16, 16, rval);
			else {
				insrt(egrobj[2], 16 * which, 16, rval);
				if (clr)
					insrt(egrobj[2], 16, 16, 0);
			}
		}
		if (bad && extr(exp.debug[3], 23, 1))
			nv04_pgraph_blowup(&exp, 2);
		if (!extr(exp.nsource, 1, 1)) {
			int subc = extr(exp.ctx_user, 13, 3);
			if (which == 2) {
				exp.ctx_cache[subc][1] = exp.ctx_switch[1];
				insrt(exp.ctx_cache[subc][1], 16, 16, rval);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[1] = exp.ctx_cache[subc][1];
			} else {
				exp.ctx_cache[subc][2] = exp.ctx_switch[2];
				insrt(exp.ctx_cache[subc][2], 16 * which, 16, rval);
				if (clr)
					insrt(exp.ctx_cache[subc][2], 16, 16, 0);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[2] = exp.ctx_cache[subc][2];
			}
		}
		bool prot_err = false;
		if (is3d && (dcls == 2 || dcls == 0x3d)) {
			if (extr(dma[1], 0, 8) != 0xff)
				prot_err = true;
			if (cls != 0x48 && extr(dma[0], 20, 8))
				prot_err = true;
		}
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("DMA %08x %08x %08x\n", dma[0], dma[1], dma[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_clip(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 14) {
			default:
				cls = 0x1c;
				mthd = 0x184;
				break;
			case 1:
				cls = 0x5c;
				mthd = 0x184;
				break;
			case 2:
				cls = 0x1d;
				mthd = 0x184;
				break;
			case 3:
				cls = 0x5d;
				mthd = 0x184;
				break;
			case 4:
				cls = 0x1e;
				mthd = 0x184;
				break;
			case 5:
				cls = 0x5e;
				mthd = 0x184;
				break;
			case 6:
				cls = 0x1f;
				mthd = 0x188;
				break;
			case 7:
				cls = 0x5f;
				mthd = 0x188;
				break;
			case 8:
				cls = 0x21;
				mthd = 0x188;
				break;
			case 9:
				cls = 0x61;
				mthd = 0x188;
				break;
			case 10:
				cls = 0x65;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				mthd = 0x188;
				break;
			case 11:
				cls = 0x60;
				mthd = 0x18c;
				break;
			case 12:
				cls = 0x64;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				mthd = 0x18c;
				break;
			case 13:
				cls = 0x48;
				mthd = 0x188;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t ctxobj[4];
		uint32_t val = nv04_pgraph_gen_ctxobj(ctx, &orig, ctxobj);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		if (extr(exp.debug[3], 29, 1)) {
			int ccls = extr(ctxobj[0], 0, 12);
			bool bad = false;
			bad = ccls != 0x19 && ccls != 0x30;
			if (!extr(exp.nsource, 1, 1)) {
				insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
				insrt(egrobj[0], 13, 1, ccls != 0x30);
			}
			if (bad && extr(exp.debug[3], 23, 1))
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1)) {
				int subc = extr(exp.ctx_user, 13, 3);
				exp.ctx_cache[subc][0] = exp.ctx_switch[0];
				insrt(exp.ctx_cache[subc][0], 13, 1, ccls != 0x30);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[0] = exp.ctx_cache[subc][0];
			}
		} else {
			nv04_pgraph_blowup(&exp, 0x40);
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("CTX %08x %08x %08x\n", ctxobj[0], ctxobj[1], ctxobj[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_chroma(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 10) {
			default:
				cls = 0x1f;
				mthd = 0x184;
				break;
			case 1:
				cls = 0x5f;
				mthd = 0x184;
				break;
			case 2:
				cls = 0x21;
				mthd = 0x184;
				break;
			case 3:
				cls = 0x61;
				mthd = 0x184;
				break;
			case 4:
				cls = 0x65;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				mthd = 0x184;
				break;
			case 5:
				cls = 0x36;
				mthd = 0x184;
				break;
			case 6:
				cls = 0x76;
				mthd = 0x184;
				break;
			case 7:
				cls = 0x66;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				mthd = 0x184;
				break;
			case 8:
				cls = 0x60;
				mthd = 0x188;
				break;
			case 9:
				cls = 0x64;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				mthd = 0x188;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t ctxobj[4];
		uint32_t val = nv04_pgraph_gen_ctxobj(ctx, &orig, ctxobj);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		if (extr(exp.debug[3], 29, 1)) {
			int ccls = extr(ctxobj[0], 0, 12);
			bool bad = false;
			bad = ccls != 0x57 && ccls != 0x30;
			if (!extr(exp.nsource, 1, 1)) {
				insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
				insrt(egrobj[0], 12, 1, ccls != 0x30);
			}
			if (bad && extr(exp.debug[3], 23, 1))
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1)) {
				int subc = extr(exp.ctx_user, 13, 3);
				exp.ctx_cache[subc][0] = exp.ctx_switch[0];
				insrt(exp.ctx_cache[subc][0], 12, 1, ccls != 0x30);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[0] = exp.ctx_cache[subc][0];
			}
		} else {
			nv04_pgraph_blowup(&exp, 0x40);
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("CTX %08x %08x %08x\n", ctxobj[0], ctxobj[1], ctxobj[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_rop(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t cls, mthd;
		bool isnew = true;
		int which;
		switch (nrand48(ctx->rand48) % 20) {
			default:
				which = nrand48(ctx->rand48) % 3;
				cls = 0x1c;
				mthd = 0x188 + which * 4;
				isnew = false;
				break;
			case 1:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x5c;
				mthd = 0x188 + which * 4;
				break;
			case 2:
				which = nrand48(ctx->rand48) % 3;
				cls = 0x1d;
				mthd = 0x188 + which * 4;
				isnew = false;
				break;
			case 3:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x5d;
				mthd = 0x188 + which * 4;
				break;
			case 4:
				which = nrand48(ctx->rand48) % 3;
				cls = 0x1e;
				mthd = 0x188 + which * 4;
				isnew = false;
				break;
			case 5:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x5e;
				mthd = 0x188 + which * 4;
				break;
			case 6:
				which = nrand48(ctx->rand48) % 3;
				cls = 0x1f;
				mthd = 0x18c + which * 4;
				isnew = false;
				break;
			case 7:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x5f;
				mthd = 0x18c + which * 4;
				break;
			case 8:
				which = nrand48(ctx->rand48) % 3;
				cls = 0x21;
				mthd = 0x18c + which * 4;
				isnew = false;
				break;
			case 9:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x61;
				mthd = 0x18c + which * 4;
				break;
			case 10:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x65;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				mthd = 0x18c + which * 4;
				break;
			case 11:
				which = nrand48(ctx->rand48) % 3;
				cls = 0x36;
				mthd = 0x188 + which * 4;
				isnew = false;
				break;
			case 12:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x76;
				mthd = 0x188 + which * 4;
				break;
			case 13:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x66;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				mthd = 0x188 + which * 4;
				break;
			case 14:
				which = nrand48(ctx->rand48) % 3;
				cls = 0x37;
				mthd = 0x188 + which * 4;
				isnew = false;
				break;
			case 15:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x77;
				mthd = 0x188 + which * 4;
				break;
			case 16:
				which = nrand48(ctx->rand48) % 3;
				cls = 0x4b;
				mthd = 0x184 + which * 4;
				isnew = false;
				break;
			case 17:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x4a;
				mthd = 0x188 + which * 4;
				break;
			case 18:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x60;
				mthd = 0x190 + which * 4;
				break;
			case 19:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x64;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				mthd = 0x190 + which * 4;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t ctxobj[4];
		uint32_t val = nv04_pgraph_gen_ctxobj(ctx, &orig, ctxobj);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		if (extr(exp.debug[3], 29, 1)) {
			int ccls = extr(ctxobj[0], 0, 12);
			bool bad = false;
			int ecls = 0;
			if (which == 0)
				ecls = isnew ? 0x44 : 0x18;
			if (which == 1)
				ecls = 0x43;
			if (which == 2)
				ecls = 0x12;
			if (which == 3)
				ecls = 0x72;
			bad = ccls != ecls && ccls != 0x30;
			if (!extr(exp.nsource, 1, 1)) {
				insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
				insrt(egrobj[0], 27 + which, 1, ccls != 0x30);
			}
			if (bad && extr(exp.debug[3], 23, 1))
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1)) {
				int subc = extr(exp.ctx_user, 13, 3);
				exp.ctx_cache[subc][0] = exp.ctx_switch[0];
				insrt(exp.ctx_cache[subc][0], 27 + which, 1, ccls != 0x30);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[0] = exp.ctx_cache[subc][0];
			}
		} else {
			nv04_pgraph_blowup(&exp, 0x40);
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("CTX %08x %08x %08x\n", ctxobj[0], ctxobj[1], ctxobj[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_surf_nv3(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t cls, mthd;
		int which;
		switch (nrand48(ctx->rand48) % 11) {
			default:
				which = 0;
				cls = 0x1c;
				mthd = 0x194;
				break;
			case 1:
				which = 0;
				cls = 0x1d;
				mthd = 0x194;
				break;
			case 2:
				which = 0;
				cls = 0x1e;
				mthd = 0x194;
				break;
			case 3:
				which = 0;
				cls = 0x1f;
				mthd = 0x19c;
				break;
			case 4:
				which = 1;
				cls = 0x1f;
				mthd = 0x198;
				break;
			case 5:
				which = 0;
				cls = 0x21;
				mthd = 0x198;
				break;
			case 6:
				which = 0;
				cls = 0x36;
				mthd = 0x194;
				break;
			case 7:
				which = 0;
				cls = 0x37;
				mthd = 0x194;
				break;
			case 8:
				which = 0;
				cls = 0x4b;
				mthd = 0x190;
				break;
			case 9:
				which = 2;
				cls = 0x48;
				mthd = 0x18c;
				break;
			case 10:
				which = 3;
				cls = 0x48;
				mthd = 0x190;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t ctxobj[4];
		uint32_t val = nv04_pgraph_gen_ctxobj(ctx, &orig, ctxobj);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		if (extr(exp.debug[3], 29, 1)) {
			int ccls = extr(ctxobj[0], 0, 12);
			bool bad = false;
			int ecls = 0x58 + which;
			bad = ccls != ecls && ccls != 0x30;
			bool isswz = ccls == 0x52;
			if (!extr(exp.nsource, 1, 1)) {
				insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
				insrt(egrobj[0], 25 + (which & 1), 1, ccls != 0x30);
				if (which == 0 || which == 2)
					insrt(egrobj[0], 14, 1, isswz);
			}
			if (bad && extr(exp.debug[3], 23, 1))
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1)) {
				int subc = extr(exp.ctx_user, 13, 3);
				exp.ctx_cache[subc][0] = exp.ctx_switch[0];
				insrt(exp.ctx_cache[subc][0], 25 + (which & 1), 1, ccls != 0x30);
				if (which == 0 || which == 2)
					insrt(exp.ctx_cache[subc][0], 14, 1, isswz);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[0] = exp.ctx_cache[subc][0];
			}
		} else {
			nv04_pgraph_blowup(&exp, 0x40);
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("CTX %08x %08x %08x\n", ctxobj[0], ctxobj[1], ctxobj[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_surf_nv4(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t cls, mthd;
		bool swzok = false;
		bool is3d = false;
		switch (nrand48(ctx->rand48) % 14) {
			default:
				cls = 0x5c;
				mthd = 0x198;
				break;
			case 1:
				cls = 0x5d;
				mthd = 0x198;
				break;
			case 2:
				cls = 0x5e;
				mthd = 0x198;
				break;
			case 3:
				cls = 0x5f;
				mthd = 0x19c;
				break;
			case 4:
				cls = 0x61;
				mthd = 0x19c;
				break;
			case 5:
				cls = 0x65;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				mthd = 0x19c;
				break;
			case 6:
				cls = 0x76;
				mthd = 0x198;
				break;
			case 7:
				cls = 0x66;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				mthd = 0x198;
				break;
			case 8:
				cls = 0x77;
				mthd = 0x198;
				swzok = true;
				break;
			case 9:
				cls = 0x4a;
				mthd = 0x198;
				break;
			case 10:
				cls = 0x60;
				mthd = 0x1a0;
				swzok = true;
				break;
			case 11:
				cls = 0x64;
				if (ctx->chipset.chipset < 5)
					cls = 0x60;
				mthd = 0x1a0;
				swzok = true;
				break;
			case 12:
				cls = 0x54;
				mthd = 0x18c;
				is3d = true;
				break;
			case 13:
				cls = 0x55;
				mthd = 0x18c;
				is3d = true;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t ctxobj[4];
		uint32_t val = nv04_pgraph_gen_ctxobj(ctx, &orig, ctxobj);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		if (extr(exp.debug[3], 29, 1) || is3d) {
			int ccls = extr(ctxobj[0], 0, 12);
			bool bad = false;
			if (is3d)
				bad = ccls != 0x53 && ccls != 0x30;
			else
				bad = ccls != 0x42 && ccls != 0x52 && ccls != 0x30;
			bool isswz = ccls == 0x52;
			if (isswz && !swzok)
				bad = true;
			if (!extr(exp.nsource, 1, 1)) {
				insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
				insrt(egrobj[0], 25, 1, ccls != 0x30);
				insrt(egrobj[0], 14, 1, isswz);
			}
			if (bad && extr(exp.debug[3], 23, 1))
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1)) {
				int subc = extr(exp.ctx_user, 13, 3);
				exp.ctx_cache[subc][0] = exp.ctx_switch[0];
				insrt(exp.ctx_cache[subc][0], 25, 1, ccls != 0x30);
				insrt(exp.ctx_cache[subc][0], 14, 1, isswz);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[0] = exp.ctx_cache[subc][0];
			}
		} else {
			nv04_pgraph_blowup(&exp, 0x40);
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("CTX %08x %08x %08x\n", ctxobj[0], ctxobj[1], ctxobj[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_operation(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		bool isnew = true;
		switch (nrand48(ctx->rand48) % 20) {
			default:
				cls = 0x1c;
				mthd = 0x2fc;
				isnew = false;
				break;
			case 1:
				cls = 0x5c;
				mthd = 0x2fc;
				break;
			case 2:
				cls = 0x1d;
				mthd = 0x2fc;
				isnew = false;
				break;
			case 3:
				cls = 0x5d;
				mthd = 0x2fc;
				break;
			case 4:
				cls = 0x1e;
				mthd = 0x2fc;
				isnew = false;
				break;
			case 5:
				cls = 0x5e;
				mthd = 0x2fc;
				break;
			case 6:
				cls = 0x1f;
				mthd = 0x2fc;
				isnew = false;
				break;
			case 7:
				cls = 0x5f;
				mthd = 0x2fc;
				break;
			case 8:
				cls = 0x21;
				mthd = 0x2fc;
				isnew = false;
				break;
			case 9:
				cls = 0x61;
				mthd = 0x2fc;
				break;
			case 10:
				cls = 0x65;
				mthd = 0x2fc;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				break;
			case 11:
				cls = 0x36;
				mthd = 0x2fc;
				isnew = false;
				break;
			case 12:
				cls = 0x76;
				mthd = 0x2fc;
				break;
			case 13:
				cls = 0x66;
				mthd = 0x2fc;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				break;
			case 14:
				cls = 0x37;
				mthd = 0x304;
				isnew = false;
				break;
			case 15:
				cls = 0x77;
				mthd = 0x304;
				break;
			case 16:
				cls = 0x4b;
				mthd = 0x2fc;
				isnew = false;
				break;
			case 17:
				cls = 0x4a;
				mthd = 0x2fc;
				break;
			case 18:
				cls = 0x60;
				mthd = 0x3e5;
				break;
			case 19:
				cls = 0x64;
				mthd = 0x3e5;
				if (ctx->chipset.chipset < 5)
					cls = 0x64;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		if (extr(exp.debug[3], 30, 1)) {
			bool bad = false;
			if (val > (isnew ? 5 : 2))
				bad = true;
			if (bad && extr(exp.debug[3], 20, 1))
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1)) {
				insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
				insrt(egrobj[0], 15, 3, val);
				int subc = extr(exp.ctx_user, 13, 3);
				exp.ctx_cache[subc][0] = exp.ctx_switch[0];
				insrt(exp.ctx_cache[subc][0], 15, 3, val);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[0] = exp.ctx_cache[subc][0];
			}
		} else {
			nv04_pgraph_blowup(&exp, 0x40);
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_patch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd = 0x10c;
		switch (nrand48(ctx->rand48) % 20) {
			default:
				cls = 0x1c;
				break;
			case 1:
				cls = 0x5c;
				break;
			case 2:
				cls = 0x1d;
				break;
			case 3:
				cls = 0x5d;
				break;
			case 4:
				cls = 0x1e;
				break;
			case 5:
				cls = 0x5e;
				break;
			case 6:
				cls = 0x1f;
				break;
			case 7:
				cls = 0x5f;
				break;
			case 8:
				cls = 0x21;
				break;
			case 9:
				cls = 0x61;
				break;
			case 10:
				cls = 0x65;
				if (ctx->chipset.chipset < 5)
					cls = 0x61;
				break;
			case 11:
				cls = 0x36;
				break;
			case 12:
				cls = 0x76;
				break;
			case 13:
				cls = 0x66;
				if (ctx->chipset.chipset < 5)
					cls = 0x76;
				break;
			case 14:
				cls = 0x37;
				break;
			case 15:
				cls = 0x77;
				break;
			case 16:
				cls = 0x4b;
				break;
			case 17:
				cls = 0x4a;
				break;
			case 18:
				cls = 0x60;
				break;
			case 19:
				cls = 0x64;
				if (ctx->chipset.chipset < 5)
					cls = 0x60;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		if (ctx->chipset.chipset < 5) {
			bool bad = false;
			if (val < 2) {
				if (extr(exp.debug[1], 8, 1) && val == 0) {
					int subc = extr(exp.ctx_user, 13, 3);
					exp.ctx_cache[subc][0] = exp.ctx_switch[0];
					insrt(exp.ctx_cache[subc][0], 24, 1, 0);
					if (extr(exp.debug[1], 20, 1))
						exp.ctx_switch[0] = exp.ctx_cache[subc][0];
				} else {
					exp.intr |= 0x10;
					exp.fifo_enable = 0;
				}
			} else {
				bad = true;
			}
			if (bad && extr(exp.debug[3], 20, 1))
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1) && !extr(exp.intr, 4, 1)) {
				insrt(egrobj[0], 24, 8, extr(exp.ctx_switch[0], 24, 8));
				insrt(egrobj[0], 24, 1, 0);
			}
		} else {
			exp.intr |= 0x10;
			exp.fifo_enable = 0;
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int invalid_mthd_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int simple_mthd_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int nv04_pgraph_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type < 0x04 || ctx->chipset.card_type > 0x10)
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

HWTEST_DEF_GROUP(state,
	HWTEST_TEST(test_state, 0),
	HWTEST_TEST(test_mmio_write, 0),
	HWTEST_TEST(test_mmio_d3d_write, 0),
	HWTEST_TEST(test_mmio_vtx_write, 0),
	HWTEST_TEST(test_mmio_iclip_write, 0),
	HWTEST_TEST(test_mmio_uclip_write, 0),
	HWTEST_TEST(test_mmio_read, 0),
	HWTEST_TEST(test_formats, 0),
)

HWTEST_DEF_GROUP(invalid_mthd,
	HWTEST_TEST(test_invalid_class, 0),
	HWTEST_TEST(test_invalid_mthd_op, 0),
	HWTEST_TEST(test_invalid_mthd_ctx, 0),
	HWTEST_TEST(test_invalid_mthd_surf, 0),
	HWTEST_TEST(test_invalid_mthd_dvd, 0),
	HWTEST_TEST(test_invalid_mthd_m2mf, 0),
	HWTEST_TEST(test_invalid_mthd_lin, 0),
	HWTEST_TEST(test_invalid_mthd_tri, 0),
	HWTEST_TEST(test_invalid_mthd_rect, 0),
	HWTEST_TEST(test_invalid_mthd_blit, 0),
	HWTEST_TEST(test_invalid_mthd_ifc, 0),
	HWTEST_TEST(test_invalid_mthd_sifc, 0),
	HWTEST_TEST(test_invalid_mthd_iifc, 0),
	HWTEST_TEST(test_invalid_mthd_sifm, 0),
	HWTEST_TEST(test_invalid_mthd_gdi_nv3, 0),
	HWTEST_TEST(test_invalid_mthd_gdi_nv4, 0),
	HWTEST_TEST(test_invalid_mthd_d3d0, 0),
	HWTEST_TEST(test_invalid_mthd_d3d5, 0),
	HWTEST_TEST(test_invalid_mthd_d3d6, 0),
)

HWTEST_DEF_GROUP(simple_mthd,
	HWTEST_TEST(test_mthd_ctxsw, 0),
	HWTEST_TEST(test_mthd_nop, 0),
	HWTEST_TEST(test_mthd_notify, 0),
	HWTEST_TEST(test_mthd_missing, 0),
	HWTEST_TEST(test_mthd_beta, 0),
	HWTEST_TEST(test_mthd_beta4, 0),
	HWTEST_TEST(test_mthd_rop, 0),
	HWTEST_TEST(test_mthd_chroma_nv1, 0),
	HWTEST_TEST(test_mthd_chroma_nv4, 0),
	HWTEST_TEST(test_mthd_pattern_shape, 0),
	HWTEST_TEST(test_mthd_pattern_select, 0),
	HWTEST_TEST(test_mthd_pattern_mono_color_nv1, 0),
	HWTEST_TEST(test_mthd_pattern_mono_color_nv4, 0),
	HWTEST_TEST(test_mthd_pattern_mono_bitmap, 0),
	HWTEST_TEST(test_mthd_pattern_color_y8, 0),
	HWTEST_TEST(test_mthd_pattern_color_r5g6b5, 0),
	HWTEST_TEST(test_mthd_pattern_color_r5g5b5, 0),
	HWTEST_TEST(test_mthd_pattern_color_r8g8b8, 0),
	HWTEST_TEST(test_mthd_surf_offset, 0),
	HWTEST_TEST(test_mthd_surf_pitch, 0),
	HWTEST_TEST(test_mthd_surf_pitch_2, 0),
	HWTEST_TEST(test_mthd_surf_nv3_format, 0),
	HWTEST_TEST(test_mthd_surf_2d_format, 0),
	HWTEST_TEST(test_mthd_surf_swz_format, 0),
	HWTEST_TEST(test_mthd_surf_dvd_format, 0),
	HWTEST_TEST(test_mthd_surf_3d_format, 0),
	HWTEST_TEST(test_mthd_dma_surf, 0),
	HWTEST_TEST(test_mthd_clip, 0),
	HWTEST_TEST(test_mthd_clip_zero_size, 0),
	HWTEST_TEST(test_mthd_clip_hv, 0),
	HWTEST_TEST(test_mthd_solid_color, 0),
	HWTEST_TEST(test_mthd_sifm_pitch, 0),
	HWTEST_TEST(test_mthd_dvd_format, 0),
	HWTEST_TEST(test_mthd_index_format, 0),
	HWTEST_TEST(test_mthd_dma_offset, 0),
	HWTEST_TEST(test_mthd_m2mf_pitch, 0),
	HWTEST_TEST(test_mthd_m2mf_line_length, 0),
	HWTEST_TEST(test_mthd_m2mf_line_count, 0),
	HWTEST_TEST(test_mthd_m2mf_format, 0),
	HWTEST_TEST(test_mthd_font, 0),
	HWTEST_TEST(test_mthd_d3d_tex_color_key, 0),
	HWTEST_TEST(test_mthd_d3d_tex_offset, 0),
	HWTEST_TEST(test_mthd_d3d_tex_format_d3d0, 0),
	HWTEST_TEST(test_mthd_d3d_tex_format, 0),
	HWTEST_TEST(test_mthd_d3d_tex_filter_d3d0, 0),
	HWTEST_TEST(test_mthd_d3d_tex_filter, 0),
	HWTEST_TEST(test_mthd_d3d_blend, 0),
	HWTEST_TEST(test_mthd_d3d_config, 0),
	HWTEST_TEST(test_mthd_d3d_config_d3d0, 0),
	HWTEST_TEST(test_mthd_d3d_alpha_d3d0, 0),
	HWTEST_TEST(test_mthd_d3d_stencil_func, 0),
	HWTEST_TEST(test_mthd_d3d_stencil_op, 0),
	HWTEST_TEST(test_mthd_d3d_fog_color, 0),
	HWTEST_TEST(test_mthd_d3d_rc_cw, 0),
	HWTEST_TEST(test_mthd_d3d_rc_factor, 0),
	HWTEST_TEST(test_mthd_ctx_format, 0),
	HWTEST_TEST(test_mthd_solid_format, 0),
	HWTEST_TEST(test_mthd_ifc_format, 0),
	HWTEST_TEST(test_mthd_sifm_src_format, 0),
	HWTEST_TEST(test_mthd_mono_format, 0),
	HWTEST_TEST(test_mthd_dma_grobj, 0),
	HWTEST_TEST(test_mthd_ctx_clip, 0),
	HWTEST_TEST(test_mthd_ctx_chroma, 0),
	HWTEST_TEST(test_mthd_ctx_rop, 0),
	HWTEST_TEST(test_mthd_ctx_surf_nv3, 0),
	HWTEST_TEST(test_mthd_ctx_surf_nv4, 0),
	HWTEST_TEST(test_mthd_operation, 0),
	HWTEST_TEST(test_mthd_patch, 0),
)

}

HWTEST_DEF_GROUP(nv04_pgraph,
	HWTEST_GROUP(scan),
	HWTEST_GROUP(state),
	HWTEST_GROUP(invalid_mthd),
	HWTEST_GROUP(simple_mthd),
)
