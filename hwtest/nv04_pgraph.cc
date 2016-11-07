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
	nva_wr32(ctx->cnum, 0x400720, 0);
	nva_wr32(ctx->cnum, 0x400750, 0);
	nva_wr32(ctx->cnum, 0x400754, 0);
	TEST_BITSCAN(0x400720, 0x1, 0);
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

static void nv04_pgraph_gen_state(struct hwtest_ctx *ctx, struct nv04_pgraph_state *state) {
	bool is_nv5 = ctx->chipset.chipset >= 5;
	state->chipset = ctx->chipset;
	state->debug[0] = jrand48(ctx->rand48) & 0x1337f000;
	state->debug[1] = jrand48(ctx->rand48) & (is_nv5 ? 0xf2ffb701 : 0x72113101);
	state->debug[2] = jrand48(ctx->rand48) & 0x11d7fff1;
	state->debug[3] = jrand48(ctx->rand48) & (is_nv5 ? 0xfbffff73 : 0x11ffff33);
	state->status = 0;
	state->intr = 0;
	state->intr_en = jrand48(ctx->rand48) & 0x11311;
	state->nstatus = jrand48(ctx->rand48) & 0x7800;
	state->nsource = 0;
	uint32_t ctx_mask = is_nv5 ? 0x7f73f0ff : 0x0303f0ff;
	state->ctx_switch[0] = jrand48(ctx->rand48) & ctx_mask;
	state->ctx_switch[1] = jrand48(ctx->rand48) & 0xffff3f03;
	state->ctx_switch[2] = jrand48(ctx->rand48) & 0xffffffff;
	state->ctx_switch[3] = jrand48(ctx->rand48) & 0x0000ffff;
	for (int i = 0; i < 8; i++) {
		state->ctx_cache[i][0] = jrand48(ctx->rand48) & ctx_mask;
		state->ctx_cache[i][1] = jrand48(ctx->rand48) & 0xffff3f03;
		state->ctx_cache[i][2] = jrand48(ctx->rand48) & 0xffffffff;
		state->ctx_cache[i][3] = jrand48(ctx->rand48) & 0x0000ffff;
	}
	state->ctx_control = jrand48(ctx->rand48) & 0x11010103;
	state->ctx_user = jrand48(ctx->rand48) & 0x0f00e000;
	state->unk610 = jrand48(ctx->rand48) & (is_nv5 ? 0xe1fffff0 : 0xe0fffff0);
	state->unk614 = jrand48(ctx->rand48) & (is_nv5 ? 0xc1fffff0 : 0xc0fffff0);
	state->fifo_enable = jrand48(ctx->rand48) & 1;
	for (int i = 0; i < 4; i++) {
		state->fifo_mthd[i] = jrand48(ctx->rand48) & 0x00007fff;
		state->fifo_data[i] = jrand48(ctx->rand48) & 0xffffffff;
	}
	if (state->fifo_enable) {
		state->fifo_ptr = (jrand48(ctx->rand48) & 7) * 0x11;
		state->fifo_mthd_st2 = jrand48(ctx->rand48) & 0x000ffffe;
	} else {
		state->fifo_ptr = jrand48(ctx->rand48) & 0x00000077;
		state->fifo_mthd_st2 = jrand48(ctx->rand48) & 0x000fffff;
	}
	state->fifo_data_st2 = jrand48(ctx->rand48) & 0xffffffff;
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
		state->uclip_max[i] = jrand48(ctx->rand48) & 0x3ffff;
		state->oclip_max[i] = jrand48(ctx->rand48) & 0x3ffff;
	}
	state->xy_misc_0 = jrand48(ctx->rand48) & 0xf013ffff;
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
	state->valid[1] = jrand48(ctx->rand48) & 0x1fffffff;
	state->misc24[0] = jrand48(ctx->rand48) & 0x00ffffff;
	state->misc24[1] = jrand48(ctx->rand48) & 0x00ffffff;
	state->misc24[2] = jrand48(ctx->rand48) & 0x00ffffff;
	state->misc32[0] = jrand48(ctx->rand48);
	state->misc32[1] = jrand48(ctx->rand48);
	state->misc32[2] = jrand48(ctx->rand48);
	state->misc32[3] = jrand48(ctx->rand48);
	state->dma_pitch = jrand48(ctx->rand48);
	state->unk764 = jrand48(ctx->rand48) & 0x0000033f;
	state->unk768 = jrand48(ctx->rand48) & 0x01030000;
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
	uint32_t offset_mask = is_nv5 ? 0x01fffff0 : 0x00fffff0;
	for (int i = 0; i < 6; i++) {
		state->surf_base[i] = jrand48(ctx->rand48) & offset_mask;
		state->surf_offset[i] = jrand48(ctx->rand48) & offset_mask;
		state->surf_limit[i] = (jrand48(ctx->rand48) & (offset_mask | 1 << 31)) | 0xf;
	}
	for (int i = 0; i < 5; i++)
		state->surf_pitch[i] = jrand48(ctx->rand48) & 0x1ff0;
	for (int i = 0; i < 2; i++)
		state->surf_swizzle[i] = jrand48(ctx->rand48) & 0x0f0f0000;
	state->surf_type = jrand48(ctx->rand48) & 3;
	state->surf_format = jrand48(ctx->rand48) & 0xffffff;
	state->ctx_valid = jrand48(ctx->rand48) & 0x0f731f3f;
	state->ctx_format = jrand48(ctx->rand48) & 0x3f3f3f3f;
	state->notify = jrand48(ctx->rand48) & 0x00110101;
	for (int i = 0; i < 2; i++) {
		state->d3d_unk590[i] = jrand48(ctx->rand48) & 0xfd1d1d1d;
		state->d3d_unk594[i] = jrand48(ctx->rand48) & 0xff1f1f1f;
		state->d3d_unk5a8[i] = jrand48(ctx->rand48) & 0xfffff7a6;
		state->d3d_unk5b0[i] = jrand48(ctx->rand48) & 0xffff9e1e;
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
	state->d3d_unk818 = jrand48(ctx->rand48) & 0xffff5fff;
	state->d3d_unk81c = jrand48(ctx->rand48) & 0xfffffff1;
	state->d3d_unk820 = jrand48(ctx->rand48) & 0x00000fff;
	state->d3d_unk824 = jrand48(ctx->rand48) & 0xff1111ff;
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
	}
	for (int i = 0; i < 32; i++) {
		nva_wr32(ctx->cnum, 0x400400 + i * 4, state->vtx_x[i]);
		nva_wr32(ctx->cnum, 0x400480 + i * 4, state->vtx_y[i]);
	}
	for (int i = 0; i < 16; i++) {
		nva_wr32(ctx->cnum, 0x400d00 + i * 4, state->vtx_u[i]);
		nva_wr32(ctx->cnum, 0x400d40 + i * 4, state->vtx_v[i]);
		nva_wr32(ctx->cnum, 0x400d80 + i * 4, state->vtx_m[i]);
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
	nva_wr32(ctx->cnum, 0x400760, state->dma_pitch);
	nva_wr32(ctx->cnum, 0x400764, state->unk764);
	nva_wr32(ctx->cnum, 0x400768, state->unk768);
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
	nva_wr32(ctx->cnum, 0x40070c, state->surf_type);
	nva_wr32(ctx->cnum, 0x400724, state->surf_format);
	nva_wr32(ctx->cnum, 0x400710, state->ctx_valid);
	nva_wr32(ctx->cnum, 0x400830, state->ctx_format);
	nva_wr32(ctx->cnum, 0x400714, state->notify);
	for (int i = 0; i < 2; i++) {
		nva_wr32(ctx->cnum, 0x400590 + i * 8, state->d3d_unk590[i]);
		nva_wr32(ctx->cnum, 0x400594 + i * 8, state->d3d_unk594[i]);
		nva_wr32(ctx->cnum, 0x4005a8 + i * 4, state->d3d_unk5a8[i]);
		nva_wr32(ctx->cnum, 0x4005b0 + i * 4, state->d3d_unk5b0[i]);
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
	nva_wr32(ctx->cnum, 0x400818, state->d3d_unk818);
	nva_wr32(ctx->cnum, 0x40081c, state->d3d_unk81c);
	nva_wr32(ctx->cnum, 0x400820, state->d3d_unk820);
	nva_wr32(ctx->cnum, 0x400824, state->d3d_unk824);
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
	for (int j = 0; j < 4; j++) {
		nva_wr32(ctx->cnum, 0x400160 + j * 4, state->ctx_switch[j]);
		for (int i = 0; i < 8; i++) {
			nva_wr32(ctx->cnum, 0x400180 + i * 4 + j * 0x20, state->ctx_cache[i][j]);
		}
	}
	nva_wr32(ctx->cnum, 0x400170, state->ctx_control);
	nva_wr32(ctx->cnum, 0x400174, state->ctx_user);
	nva_wr32(ctx->cnum, 0x400080, state->debug[0]);
	nva_wr32(ctx->cnum, 0x400084, state->debug[1]);
	nva_wr32(ctx->cnum, 0x400088, state->debug[2]);
	nva_wr32(ctx->cnum, 0x40008c, state->debug[3]);
	nva_wr32(ctx->cnum, 0x400610, state->unk610);
	nva_wr32(ctx->cnum, 0x400614, state->unk614);
	nva_wr32(ctx->cnum, 0x400508, state->valid[0]);
	nva_wr32(ctx->cnum, 0x400578, state->valid[1]);
	for (int i = 0; i < 4; i++) {
		nva_wr32(ctx->cnum, 0x400730 + i * 4, state->fifo_mthd[i]);
		nva_wr32(ctx->cnum, 0x400740 + i * 4, state->fifo_data[i]);
	}
	nva_wr32(ctx->cnum, 0x400750, state->fifo_ptr);
	nva_wr32(ctx->cnum, 0x400754, state->fifo_mthd_st2);
	nva_wr32(ctx->cnum, 0x400758, state->fifo_data_st2);
	nva_wr32(ctx->cnum, 0x400720, state->fifo_enable);
}

static void nv04_pgraph_dump_state(struct hwtest_ctx *ctx, struct nv04_pgraph_state *state) {
	int ctr = 0;
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
	state->unk610 = nva_rd32(ctx->cnum, 0x400610);
	state->unk614 = nva_rd32(ctx->cnum, 0x400614);
	state->fifo_enable = nva_rd32(ctx->cnum, 0x400720);
	nva_wr32(ctx->cnum, 0x400720, 0);
	for (int i = 0; i < 4; i++) {
		state->fifo_mthd[i] = nva_rd32(ctx->cnum, 0x400730 + i * 4);
		state->fifo_data[i] = nva_rd32(ctx->cnum, 0x400740 + i * 4);
	}
	state->fifo_ptr = nva_rd32(ctx->cnum, 0x400750);
	state->fifo_mthd_st2 = nva_rd32(ctx->cnum, 0x400754);
	state->fifo_data_st2 = nva_rd32(ctx->cnum, 0x400758);
	state->intr = nva_rd32(ctx->cnum, 0x400100);
	state->intr_en = nva_rd32(ctx->cnum, 0x400140);
	state->nstatus = nva_rd32(ctx->cnum, 0x400104);
	state->nsource = nva_rd32(ctx->cnum, 0x400108);
	for (int j = 0; j < 4; j++) {
		state->ctx_switch[j] = nva_rd32(ctx->cnum, 0x400160 + j * 4);
		for (int i = 0; i < 8; i++) {
			state->ctx_cache[i][j] = nva_rd32(ctx->cnum, 0x400180 + i * 4 + j * 0x20);
		}
	}
	// eh
	state->ctx_control = nva_rd32(ctx->cnum, 0x400170) & ~0x100000;
	state->ctx_user = nva_rd32(ctx->cnum, 0x400174);
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
	state->dma_pitch = nva_rd32(ctx->cnum, 0x400760);
	state->unk764 = nva_rd32(ctx->cnum, 0x400764);
	state->unk768 = nva_rd32(ctx->cnum, 0x400768);
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
	state->surf_type = nva_rd32(ctx->cnum, 0x40070c);
	state->surf_format = nva_rd32(ctx->cnum, 0x400724);
	state->ctx_valid = nva_rd32(ctx->cnum, 0x400710);
	state->ctx_format = nva_rd32(ctx->cnum, 0x400830);
	state->notify = nva_rd32(ctx->cnum, 0x400714);
	for (int i = 0; i < 32; i++) {
		state->vtx_x[i] = nva_rd32(ctx->cnum, 0x400400 + i * 4);
		state->vtx_y[i] = nva_rd32(ctx->cnum, 0x400480 + i * 4);
	}
	for (int i = 0; i < 16; i++) {
		state->vtx_u[i] = nva_rd32(ctx->cnum, 0x400d00 + i * 4);
		state->vtx_v[i] = nva_rd32(ctx->cnum, 0x400d40 + i * 4);
		state->vtx_m[i] = nva_rd32(ctx->cnum, 0x400d80 + i * 4);
	}
	for (int i = 0; i < 2; i++) {
		state->iclip[i] = nva_rd32(ctx->cnum, 0x400534 + i * 4);
		state->uclip_min[i] = nva_rd32(ctx->cnum, 0x40053c + i * 4);
		state->uclip_max[i] = nva_rd32(ctx->cnum, 0x400544 + i * 4);
		state->oclip_min[i] = nva_rd32(ctx->cnum, 0x400560 + i * 4);
		state->oclip_max[i] = nva_rd32(ctx->cnum, 0x400568 + i * 4);
	}
	for (int i = 0; i < 2; i++) {
		state->d3d_unk590[i] = nva_rd32(ctx->cnum, 0x400590 + i * 8);
		state->d3d_unk594[i] = nva_rd32(ctx->cnum, 0x400594 + i * 8);
		state->d3d_unk5a8[i] = nva_rd32(ctx->cnum, 0x4005a8 + i * 4);
		state->d3d_unk5b0[i] = nva_rd32(ctx->cnum, 0x4005b0 + i * 4);
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
	state->d3d_unk818 = nva_rd32(ctx->cnum, 0x400818);
	state->d3d_unk81c = nva_rd32(ctx->cnum, 0x40081c);
	state->d3d_unk820 = nva_rd32(ctx->cnum, 0x400820);
	state->d3d_unk824 = nva_rd32(ctx->cnum, 0x400824);
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
	CMP(intr, "INTR")
	CMP(intr_en, "INTR_EN")
	CMP(nstatus, "NSTATUS")
	CMP(nsource, "NSOURCE")
	for (int i = 0; i < 4; i++) {
		CMP(ctx_switch[i], "CTX_SWITCH[%d]", i)
	}
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 4; j++)
		CMP(ctx_cache[i][j], "CTX_CACHE[%d][%d]", i, j)
	}
	CMP(ctx_control, "CTX_CONTROL")
	CMP(ctx_user, "CTX_USER")
	CMP(unk610, "UNK610")
	CMP(unk614, "UNK614")
	CMP(fifo_enable, "FIFO_ENABLE")
	for (int i = 0; i < 4; i++) {
		CMP(fifo_mthd[i], "FIFO_MTHD[%d]", i)
		CMP(fifo_data[i], "FIFO_DATA[%d]", i)
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
	for (int i = 0; i < 32; i++) {
		CMP(vtx_x[i], "VTX_X[%d]", i)
		CMP(vtx_y[i], "VTX_Y[%d]", i)
	}
	for (int i = 0; i < 16; i++) {
		CMP(vtx_u[i], "VTX_U[%d]", i)
		CMP(vtx_v[i], "VTX_V[%d]", i)
		CMP(vtx_m[i], "VTX_M[%d]", i)
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
	CMP(fifo_ptr, "FIFO_PTR")
	CMP(fifo_mthd_st2, "FIFO_MTHD_ST2")
	CMP(fifo_data_st2, "FIFO_DATA_ST2")
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
	CMP(unk764, "UNK764")
	CMP(unk768, "UNK768")
	CMP(bitmap_color_0, "BITMAP_COLOR_0")
	CMP(rop, "ROP")
	CMP(beta, "BETA")
	CMP(beta4, "BETA4")
	CMP(chroma, "CHROMA")
	CMP(pattern_mono_color[0], "PATTERN_MONO_COLOR[0]")
	CMP(pattern_mono_color[1], "PATTERN_MONO_COLOR[1]")
	CMP(pattern_mono_bitmap[0], "PATTERN_MONO_BITMAP[0]")
	CMP(pattern_mono_bitmap[1], "PATTERN_MONO_BITMAP[1]")
	CMP(pattern_config, "PATTERN_MONO_CONFIG")
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
	for (int i = 0; i < 2; i++) {
		CMP(d3d_unk590[i], "D3D_UNK590[%d]", i)
		CMP(d3d_unk594[i], "D3D_UNK594[%d]", i)
		CMP(d3d_unk5a8[i], "D3D_UNK5A8[%d]", i)
		CMP(d3d_unk5b0[i], "D3D_UNK5B0[%d]", i)
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
	CMP(d3d_unk818, "D3D_UNK818")
	CMP(d3d_unk81c, "D3D_UNK81C")
	CMP(d3d_unk820, "D3D_UNK820")
	CMP(d3d_unk824, "D3D_UNK824")
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
	uint32_t offset_mask = is_nv5 ? 0x01fffff0 : 0x00fffff0;
	for (i = 0; i < 100000; i++) {
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_load_state(ctx, &orig);
		uint32_t reg;
		uint32_t val = jrand48(ctx->rand48);
		int idx;
		uint32_t ctx_mask = is_nv5 ? 0x7f73f0ff : 0x0303f0ff;
		switch (nrand48(ctx->rand48) % 87) {
			default:
			case 0:
				reg = 0x400140;
				exp.intr_en = val & 0x11311;
				break;
			case 1:
				reg = 0x400104;
				exp.nstatus = val & 0x7800;
				break;
			case 2:
				reg = 0x400080;
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
				exp.debug[1] = val & (is_nv5 ? 0xf2ffb701 : 0x72113101);
				if (val & 0x10)
					exp.xy_misc_1[0] &= ~1;
				break;
			case 4:
				reg = 0x400088;
				exp.debug[2] = val & 0x11d7fff1;
				break;
			case 5:
				reg = 0x40008c;
				exp.debug[3] = val & (is_nv5 ? 0xfbffff73 : 0x11ffff33);
				break;
			case 6:
				reg = 0x400160;
				exp.ctx_switch[0] = val & ctx_mask;
				insrt(exp.debug[1], 0, 1, extr(val, 31, 1) && extr(orig.debug[2], 28, 1));
				if (extr(exp.debug[1], 0, 1)) {
					exp.xy_misc_0 = 0;
					exp.xy_misc_1[0] = 0;
					exp.xy_misc_1[1] &= 1;
					exp.xy_misc_3 &= ~0x11;
					exp.xy_misc_4[0] = 0;
					exp.xy_misc_4[1] = 0;
					exp.xy_clip[0][0] = 0x55555555;
					exp.xy_clip[0][1] = 0x55555555;
					exp.xy_clip[1][0] = 0x55555555;
					exp.xy_clip[1][1] = 0x55555555;
					exp.valid[0] &= 0xf0000000;
					exp.valid[1] = 0;
					exp.misc32[0] = 0;
					exp.unk764 = 0;
					exp.notify &= ~0x10000;
				}
				break;
			case 7:
				reg = 0x400164;
				exp.ctx_switch[1] = val & 0xffff3f03;
				break;
			case 8:
				reg = 0x400168;
				exp.ctx_switch[2] = val;
				break;
			case 9:
				reg = 0x40016c;
				exp.ctx_switch[3] = val & 0xffff;
				break;
			case 10:
				idx = jrand48(ctx->rand48) & 7;
				reg = 0x400180 + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][0] = val & ctx_mask;
				break;
			case 11:
				idx = jrand48(ctx->rand48) & 7;
				reg = 0x4001a0 + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][1] = val & 0xffff3f03;
				break;
			case 12:
				idx = jrand48(ctx->rand48) & 7;
				reg = 0x4001c0 + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][2] = val;
				break;
			case 13:
				idx = jrand48(ctx->rand48) & 7;
				reg = 0x4001e0 + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][3] = val & 0x0000ffff;
				break;
			case 14:
				reg = 0x400170;
				exp.ctx_control = val & 0x11010103;
				break;
			case 15:
				reg = 0x400174;
				exp.ctx_user = val & 0x0f00e000;
				break;
			case 16:
				reg = 0x400610;
				exp.unk610 = val & (is_nv5 ? 0xe1fffff0 : 0xe0fffff0);
				break;
			case 17:
				reg = 0x400614;
				exp.unk614 = val & (is_nv5 ? 0xc1fffff0 : 0xc0fffff0);
				break;
			case 18:
				idx = jrand48(ctx->rand48) & 0x1f;
				reg = 0x400400 + idx * 4;
				exp.vtx_x[idx] = val;
				nv04_pgraph_vtx_fixup(&exp, 0, 8, val);
				break;
			case 19:
				idx = jrand48(ctx->rand48) & 0x1f;
				reg = 0x400480 + idx * 4;
				exp.vtx_y[idx] = val;
				nv04_pgraph_vtx_fixup(&exp, 1, 8, val);
				break;
			case 20:
				idx = jrand48(ctx->rand48) & 0xf;
				reg = 0x400d00 + idx * 4;
				exp.vtx_u[idx] = val & 0xffffffc0;
				break;
			case 21:
				idx = jrand48(ctx->rand48) & 0xf;
				reg = 0x400d40 + idx * 4;
				exp.vtx_v[idx] = val & 0xffffffc0;
				break;
			case 22:
				idx = jrand48(ctx->rand48) & 0xf;
				reg = 0x400d80 + idx * 4;
				exp.vtx_m[idx] = val & 0xffffffc0;
				break;
			case 23:
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
			case 24:
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x40053c + idx * 4;
				nv04_pgraph_uclip_write(&exp, 0, idx & 1, idx >> 1, val);
				break;
			case 25:
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x400560 + idx * 4;
				nv04_pgraph_uclip_write(&exp, 1, idx & 1, idx >> 1, val);
				break;
			case 26:
				reg = 0x400514;
				exp.xy_misc_0 = val & 0xf013ffff;
				break;
			case 27:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400518 + idx * 4;
				exp.xy_misc_1[idx] = val & 0x00111031;
				break;
			case 28:
				reg = 0x400520;
				exp.xy_misc_3 = val & 0x7f7f1111;
				break;
			case 29:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400500 + idx * 4;
				exp.xy_misc_4[idx] = val & 0x300000ff;
				break;
			case 30:
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x400524 + idx * 4;
				exp.xy_clip[idx >> 1][idx & 1] = val;
				break;
			case 31:
				reg = 0x400508;
				exp.valid[0] = val & 0xf07fffff;
				break;
			case 32:
				reg = 0x400578;
				exp.valid[1] = val & 0x1fffffff;
				break;
			case 33:
				idx = nrand48(ctx->rand48) % 3;
				reg = ((uint32_t[3]){0x400510, 0x400570, 0x400574})[idx];
				exp.misc24[idx] = val & 0xffffff;
				break;
			case 34:
				idx = jrand48(ctx->rand48) & 3;
				reg = ((uint32_t[4]){0x40050c, 0x40057c, 0x400580, 0x400584})[idx];
				exp.misc32[idx] = val;
				if (idx == 0)
					exp.valid[0] |= 1 << 16;
				break;
			case 35:
				reg = 0x400760;
				exp.dma_pitch = val;
				break;
			case 36:
				reg = 0x400764;
				exp.unk764 = val & 0x33f;
				break;
			case 37:
				reg = 0x400768;
				exp.unk768 = val & 0x01030000;
				break;
			case 38:
				reg = 0x400600;
				exp.bitmap_color_0 = val;
				exp.valid[0] |= 1 << 17;
				break;
			case 39:
				reg = 0x400604;
				exp.rop = val & 0xff;
				break;
			case 40:
				reg = 0x400608;
				exp.beta = val & 0x7f800000;
				break;
			case 41:
				reg = 0x40060c;
				exp.beta4 = val;
				break;
			case 42:
				reg = 0x400814;
				exp.chroma = val;
				break;
			case 43:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400800 + idx * 4;
				exp.pattern_mono_color[idx] = val;
				break;
			case 44:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400808 + idx * 4;
				exp.pattern_mono_bitmap[idx] = val;
				break;
			case 45:
				reg = 0x400810;
				exp.pattern_config = val & 0x13;
				break;
			case 46:
				idx = jrand48(ctx->rand48) & 0x3f;
				reg = 0x400900 + idx * 4;
				exp.pattern_color[idx] = val & 0xffffff;
				break;
			case 47:
				idx = nrand48(ctx->rand48) % 6;
				reg = 0x400640 + idx * 4;
				exp.surf_offset[idx] = val & offset_mask;
				exp.valid[0] |= 8;
				break;
			case 48:
				idx = nrand48(ctx->rand48) % 6;
				reg = 0x400658 + idx * 4;
				exp.surf_base[idx] = val & offset_mask;
				break;
			case 49:
				idx = nrand48(ctx->rand48) % 6;
				reg = 0x400684 + idx * 4;
				exp.surf_limit[idx] = (val & (1 << 31 | offset_mask)) | 0xf;
				break;
			case 50:
				idx = nrand48(ctx->rand48) % 5;
				reg = 0x400670 + idx * 4;
				exp.surf_pitch[idx] = val & 0x1ff0;
				exp.valid[0] |= 4;
				break;
			case 51:
				idx = nrand48(ctx->rand48) % 2;
				reg = 0x40069c + idx * 4;
				exp.surf_swizzle[idx] = val & 0x0f0f0000;
				break;
			case 52:
				reg = 0x40070c;
				exp.surf_type = val & 3;
				break;
			case 53:
				reg = 0x400724;
				exp.surf_format = val & 0xffffff;
				break;
			case 54:
				reg = 0x400710;
				exp.ctx_valid = val & 0x0f731f3f;
				break;
			case 55:
				reg = 0x400830;
				exp.ctx_format = val & 0x3f3f3f3f;
				break;
			case 56:
				reg = 0x400714;
				exp.notify = val & 0x110101;
				break;
			case 57:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400590 + idx * 8;
				exp.d3d_unk590[idx] = val & 0xfd1d1d1d;
				if (idx == 0)
					exp.valid[1] |= 1 << 28;
				else
					exp.valid[1] |= 1 << 26;
				break;
			case 58:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400594 + idx * 8;
				exp.d3d_unk594[idx] = val & 0xff1f1f1f;
				if (idx == 0)
					exp.valid[1] |= 1 << 27;
				else
					exp.valid[1] |= 1 << 25;
				break;
			case 59:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x4005a8 + idx * 4;
				exp.d3d_unk5a8[idx] = val & 0xfffff7a6;
				break;
			case 60:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x4005b0 + idx * 4;
				exp.d3d_unk5b0[idx] = val & 0xffff9e1e;
				break;
			case 61:
				reg = 0x4005c0;
				exp.d3d_unk5c0 = val;
				break;
			case 62:
				reg = 0x4005c4;
				exp.d3d_unk5c4 = val & 0xffffffc0;
				break;
			case 63:
				reg = 0x4005c8;
				exp.d3d_unk5c8 = val & 0xffffffc0;
				break;
			case 64:
				reg = 0x4005cc;
				exp.d3d_unk5cc = val & 0xffffffc0;
				break;
			case 65:
				reg = 0x4005d0;
				exp.d3d_unk5d0 = val & 0xffffffc0;
				break;
			case 66:
				reg = 0x4005d4;
				exp.d3d_unk5d4 = val;
				break;
			case 67:
				reg = 0x4005d8;
				exp.d3d_unk5d8 = val;
				break;
			case 68:
				reg = 0x4005dc;
				exp.d3d_unk5dc = val;
				break;
			case 69:
				reg = 0x4005e0;
				exp.d3d_unk5e0 = val & 0xffffffc0;
				break;
			case 70:
				reg = 0x400818;
				exp.d3d_unk818 = val & 0xffff5fff;
				exp.valid[1] |= 1 << 17;
				break;
			case 71:
				reg = 0x40081c;
				exp.d3d_unk81c = val & 0xfffffff1;
				exp.valid[1] |= 1 << 18;
				break;
			case 72:
				reg = 0x400820;
				exp.d3d_unk820 = val & 0x00000fff;
				exp.valid[1] |= 1 << 19;
				break;
			case 73:
				reg = 0x400824;
				exp.d3d_unk824 = val & 0xff1111ff;
				exp.valid[1] |= 1 << 20;
				break;
			case 74:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401000 + idx * 4;
				exp.dma_offset[idx] = val;
				break;
			case 75:
				reg = 0x401008;
				exp.dma_length = val & 0x3fffff;
				break;
			case 76:
				reg = 0x40100c;
				exp.dma_misc = val & 0x77ffff;
				break;
			case 77:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401020 + idx * 4;
				exp.dma_unk20[idx] = val;
				break;
			case 78:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401040 + idx * 0x40;
				exp.dma_eng_inst[idx] = val & 0xffff;
				break;
			case 79:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401044 + idx * 0x40;
				exp.dma_eng_flags[idx] = val & 0xfff33000;
				break;
			case 80:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401048 + idx * 0x40;
				exp.dma_eng_limit[idx] = val;
				break;
			case 81:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x40104c + idx * 0x40;
				exp.dma_eng_pte[idx] = val & 0xfffff002;
				break;
			case 82:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401050 + idx * 0x40;
				exp.dma_eng_pte_tag[idx] = val & 0xfffff000;
				break;
			case 83:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401054 + idx * 0x40;
				exp.dma_eng_addr_virt_adj[idx] = val;
				break;
			case 84:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x401058 + idx * 0x40;
				exp.dma_eng_addr_phys[idx] = val;
				break;
			case 85:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x40105c + idx * 0x40;
				exp.dma_eng_bytes[idx] = val & 0x1ffffff;
				break;
			case 86:
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

static int test_mmio_vtx_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int idx = jrand48(ctx->rand48) & 0x1f;
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
		int uo = jrand48(ctx->rand48) & 1;
		int xy = jrand48(ctx->rand48) & 1;
		int idx = jrand48(ctx->rand48) & 1;
		struct nv04_pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = (uo ? 0x400560 : 0x40053c) + xy * 4 + idx * 8;
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1)
			val = extrs(val, 0, 18);
		nva_wr32(ctx->cnum, reg, val);
		nv04_pgraph_uclip_write(&exp, uo, xy, idx, val);
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

HWTEST_DEF_GROUP(state,
	HWTEST_TEST(test_state, 0),
	HWTEST_TEST(test_mmio_write, 0),
	HWTEST_TEST(test_mmio_vtx_write, 0),
	HWTEST_TEST(test_mmio_iclip_write, 0),
	HWTEST_TEST(test_mmio_uclip_write, 0),
	HWTEST_TEST(test_mmio_read, 0),
	HWTEST_TEST(test_formats, 0),
)

}

HWTEST_DEF_GROUP(nv04_pgraph,
	HWTEST_GROUP(scan),
	HWTEST_GROUP(state),
)
