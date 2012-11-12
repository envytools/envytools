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
#include "pgraph.h"
#include "nva.h"
#include "nvhw.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const uint32_t nv01_pgraph_state_regs[] = {
	/* INTR, INVALID */
	0x400100, 0x400104,
	/* INTR_EN, INVALID_EN */
	0x400140, 0x400144,
	/* CTX_* */
	0x400180, 0x400190,
	/* ICLIP */
	0x400450, 0x400454,
	/* UCLIP */
	0x400460, 0x400468, 0x400464, 0x40046c,
	/* VTX_X */
	0x400400, 0x400404, 0x400408, 0x40040c,
	0x400410, 0x400414, 0x400418, 0x40041c,
	0x400420, 0x400424, 0x400428, 0x40042c,
	0x400430, 0x400434, 0x400438, 0x40043c,
	0x400440, 0x400444,
	/* VTX_Y */
	0x400480, 0x400484, 0x400488, 0x40048c,
	0x400490, 0x400494, 0x400498, 0x40049c,
	0x4004a0, 0x4004a4, 0x4004a8, 0x4004ac,
	0x4004b0, 0x4004b4, 0x4004b8, 0x4004bc,
	0x4004c0, 0x4004c4,
	/* VTX_BETA */
	0x400700, 0x400704, 0x400708, 0x40070c,
	0x400710, 0x400714, 0x400718, 0x40071c,
	0x400720, 0x400724, 0x400728, 0x40072c,
	0x400730, 0x400734,
	/* PATTERN_RGB, _A */
	0x400600, 0x400608, 0x400604, 0x40060c,
	/* PATTERN_MONO, _SHAPE */
	0x400610, 0x400614, 0x400618,
	/* BITMAP_COLOR */
	0x40061c, 0x400620,
	/* ROP, PLANE, CHROMA, BETA */
	0x400624, 0x400628, 0x40062c, 0x400630,
	/* CANVAS_CONFIG */
	0x400634,
	/* CANVAS */
	0x400688, 0x40068c,
	/* CLIPRECT */
	0x400690, 0x400698, 0x400694, 0x40069c,
	/* CLIPRECT_CTRL */
	0x4006a0,
	/* VALID, SOURCE_COLOR, SUBDIVIDE, EDGEFILL */
	0x400650, 0x400654, 0x400658, 0x40065c,
	/* XY_MISC */
	0x400648, 0x40064c, 0x400640, 0x400644,
	/* DMA, NOTIFY */
	0x400680, 0x400684,
	/* ACCESS */
	0x4006a4,
	/* DEBUG */
	0x400080, 0x400084, 0x400088,
	/* PFB_CONFIG */
	0x600200,

};

static void nv01_pgraph_gen_state(struct hwtest_ctx *ctx, struct nv01_pgraph_state *state) {
	if (sizeof *state != sizeof nv01_pgraph_state_regs)
		abort();
	int i;
	uint32_t *rawstate = (uint32_t *)state;
	for (i = 0; i < ARRAY_SIZE(nv01_pgraph_state_regs); i++)
		rawstate[i] = jrand48(ctx->rand48);
	state->debug[0] &= 0x11111110;
	state->debug[1] &= 0x31111101;
	state->debug[2] &= 0x11111111;
	state->intr = 0;
	state->invalid = 0;
	state->intr_en &= 0x11111011;
	state->invalid_en &= 0x00011111;
	state->ctx_switch &= 0x807fffff;
	state->ctx_control &= 0x11010103;
	/* no change for VTX ram */
	for (i = 0; i < 14; i++)
		state->vtx_beta[i] &= 0x01ffffff;
	for (i = 0; i < 2; i++) {
		state->iclip[i] &= 0x3ffff;
		state->uclip_min[i] &= 0x3ffff;
		state->uclip_max[i] &= 0x3ffff;
		state->pattern_rgb[i] &= 0x3fffffff;
		state->pattern_a[i] &= 0xff;
		state->pattern_mono[i] &= 0xffffffff;
		state->bitmap_color[i] &= 0x7fffffff;
	}
	state->pattern_shape &= 3;
	state->rop &= 0xff;
	state->plane &= 0x7fffffff;
	state->chroma &= 0x7fffffff;
	state->beta &= 0x7f800000;
	state->canvas_config &= 0x01111011;
	state->xy_misc_0 &= 0xf1ff11ff;
	state->xy_misc_1 &= 0x03177331;
	state->x_misc &= 0x30ffffff;
	state->y_misc &= 0x30ffffff;
	state->valid &= 0x111ff1ff;
	/* source color: no change */
	state->subdivide &= 0xffff00ff;
	state->edgefill &= 0xffff0113;
	state->dma &= 0xffff;
	state->notify &= 0x11ffff;
	state->canvas_min &= 0xffffffff;
	state->canvas_max &= 0x0fff0fff;
	state->cliprect_min[0] &= 0x0fff0fff;
	state->cliprect_min[1] &= 0x0fff0fff;
	state->cliprect_max[0] &= 0x0fff0fff;
	state->cliprect_max[1] &= 0x0fff0fff;
	state->cliprect_ctrl &= 0x113;
	state->access &= 0x0001f000;
	state->access |= 0x0f000111;
	state->pfb_config &= 0x70;
	state->pfb_config |= nva_rd32(ctx->cnum, 0x600200) & ~0x71;
}

static void nv01_pgraph_load_state(struct hwtest_ctx *ctx, struct nv01_pgraph_state *state) {
	if (sizeof *state != sizeof nv01_pgraph_state_regs)
		abort();
	int i;
	uint32_t *rawstate = (uint32_t*)state;
	nva_wr32(ctx->cnum, 0x4006a4, 0x04000100);
	nva_wr32(ctx->cnum, 0x400100, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400104, 0xffffffff);
	for (i = 0; i < ARRAY_SIZE(nv01_pgraph_state_regs); i++)
		nva_wr32(ctx->cnum, nv01_pgraph_state_regs[i], rawstate[i]);
}

static void nv01_pgraph_dump_state(struct hwtest_ctx *ctx, struct nv01_pgraph_state *state) {
	if (sizeof *state != sizeof nv01_pgraph_state_regs)
		abort();
	int i;
	uint32_t *rawstate = (uint32_t*)state;
	uint32_t access = nva_rd32(ctx->cnum, 0x4006a4);
	uint32_t xy_misc_1 = nva_rd32(ctx->cnum, 0x400644); /* this one can be disturbed by *reading* VTX mem */
	nva_wr32(ctx->cnum, 0x4006a4, 0x04000100);
	for (i = 0; i < ARRAY_SIZE(nv01_pgraph_state_regs); i++)
		rawstate[i] = nva_rd32(ctx->cnum, nv01_pgraph_state_regs[i]);
	state->intr &= ~0x100;
	state->ctx_control &= ~0x00100000;
	state->pfb_config &= ~1;
	state->access = access;
	state->xy_misc_1 = xy_misc_1;
}

static int nv01_pgraph_cmp_state(struct nv01_pgraph_state *exp, struct nv01_pgraph_state *real) {
	if (sizeof *exp != sizeof nv01_pgraph_state_regs)
		abort();
	int i;
	uint32_t *rawexp = (uint32_t*)exp;
	uint32_t *rawreal = (uint32_t*)real;
	int res = 0;
	for (i = 0; i < ARRAY_SIZE(nv01_pgraph_state_regs); i++) {
		if (rawexp[i] != rawreal[i]) {
			printf("Difference in reg %06x: expected %08x real %08x\n", nv01_pgraph_state_regs[i], rawexp[i], rawreal[i]);
			res = 1;
		}
	}
	return res;
}

static int test_scan_access(struct hwtest_ctx *ctx) {
	uint32_t val = nva_rd32(ctx->cnum, 0x4006a4);
	int i;
	for (i = 0; i < 1000; i++) {
		uint32_t nv = jrand48(ctx->rand48);
		uint32_t next = val;
		nva_wr32(ctx->cnum, 0x4006a4, nv);
		if (nv & 1 << 24)
			next &= ~0x1, next |= nv & 0x1;
		if (nv & 1 << 25)
			next &= ~0x10, next |= nv & 0x10;
		if (nv & 1 << 26)
			next &= ~0x100, next |= nv & 0x100;
		if (nv & 1 << 27)
			next &= ~0x1f000, next |= nv & 0x1f000;
		uint32_t real = nva_rd32(ctx->cnum, 0x4006a4);
		if (real != next) {
			printf("ACCESS mismatch: prev %08x write %08x expected %08x real %08x\n", val, nv, next, real);
			return HWTEST_RES_FAIL;
		}
		val = next;
	}
	return HWTEST_RES_PASS;
}

static int test_scan_debug(struct hwtest_ctx *ctx) {
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	TEST_BITSCAN(0x400080, 0x11111110, 0);
	TEST_BITSCAN(0x400084, 0x31111101, 0);
	TEST_BITSCAN(0x400088, 0x11111111, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_control(struct hwtest_ctx *ctx) {
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	TEST_BITSCAN(0x400140, 0x11111111, 0);
	TEST_BITSCAN(0x400144, 0x00011111, 0);
	TEST_BITSCAN(0x400180, 0x807fffff, 0);
	TEST_BITSCAN(0x400190, 0x11010103, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_canvas(struct hwtest_ctx *ctx) {
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	TEST_BITSCAN(0x400634, 0x01111011, 0);
	nva_wr32(ctx->cnum, 0x400688, 0x7fff7fff);
	if(nva_rd32(ctx->cnum, 0x400688) != 0x7fff7fff) {
		return HWTEST_RES_FAIL;
	}
	TEST_BITSCAN(0x400688, 0xffffffff, 0);
	TEST_BITSCAN(0x40068c, 0x0fff0fff, 0);
	TEST_BITSCAN(0x400690, 0x0fff0fff, 0);
	TEST_BITSCAN(0x400694, 0x0fff0fff, 0);
	TEST_BITSCAN(0x400698, 0x0fff0fff, 0);
	TEST_BITSCAN(0x40069c, 0x0fff0fff, 0);
	TEST_BITSCAN(0x4006a0, 0x113, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_vtx(struct hwtest_ctx *ctx) {
	int i;
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	for (i = 0 ; i < 18; i++) {
		TEST_BITSCAN(0x400400 + i * 4, 0xffffffff, 0);
		TEST_BITSCAN(0x400480 + i * 4, 0xffffffff, 0);
		if (i < 14)
			TEST_BITSCAN(0x400700 + i * 4, 0x01ffffff, 0);
	}
	/* relative */
	for (i = 0; i < 1000; i++) {
		int idx = nrand48(ctx->rand48) % 18;
		int xy = jrand48(ctx->rand48) & 1;
		uint32_t canv_min = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, 0x400688, canv_min);
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cm = canv_min >> xy * 16 & 0xffff;
		uint32_t exp = (int16_t)cm + val;
		nva_wr32(ctx->cnum, 0x400500 + idx * 4 + xy * 0x80, val);
		TEST_READ(0x400400 + idx * 4 + xy * 0x80, exp, "cm %08x val %08x", cm, val);
		TEST_READ(0x400c00 + idx * 4 + xy * 0x80, exp, "cm %08x val %08x", cm, val);
	}
	return HWTEST_RES_PASS;
}

static int test_scan_clip(struct hwtest_ctx *ctx) {
	int i;
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	TEST_BITSCAN(0x400450, 0x0003ffff, 0);
	TEST_BITSCAN(0x400454, 0x0003ffff, 0);
	for (i = 0; i < 1000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t v0 = jrand48(ctx->rand48);
		uint32_t v1 = jrand48(ctx->rand48);
		uint32_t v2 = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, 0x400460 + idx * 8, v0);
		nva_wr32(ctx->cnum, 0x400464 + idx * 8, v1);
		v0 &= 0x3ffff;
		v1 &= 0x3ffff;
		TEST_READ(0x400460 + idx * 8, v0, "v0 %08x v1 %08x", v0, v1);
		TEST_READ(0x400464 + idx * 8, v1, "v0 %08x v1 %08x", v0, v1);
		if (jrand48(ctx->rand48) & 1) {
			nva_wr32(ctx->cnum, 0x400460 + idx * 8, v2);
		} else {
			nva_wr32(ctx->cnum, 0x400464 + idx * 8, v2);
		}
		v2 &= 0x3ffff;
		TEST_READ(0x400460 + idx * 8, v1, "v0 %08x v1 %08x v2 %08x", v0, v1, v2);
		TEST_READ(0x400464 + idx * 8, v2, "v0 %08x v1 %08x v2 %08x", v0, v1, v2);
	}
	/* relative */
	for (i = 0; i < 1000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t canv_min = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, 0x400688, canv_min);
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cm = canv_min >> idx * 16 & 0xffff;
		uint32_t r = jrand48(ctx->rand48);
		uint32_t exp = ((int16_t)cm + val) & 0x3ffff;
		if (r&1) {
			nva_wr32(ctx->cnum, 0x400550 + idx * 4, val);
			TEST_READ(0x400450 + idx * 4, exp, "cm %08x val %08x", cm, val);
		} else {
			nva_wr32(ctx->cnum, 0x400560 + idx * 8 + (r & 2) * 2, val);
			TEST_READ(0x400464 + idx * 8, exp, "cm %08x val %08x", cm, val);
		}
	}
	return HWTEST_RES_PASS;
}

static int test_scan_context(struct hwtest_ctx *ctx) {
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	TEST_BITSCAN(0x400600, 0x3fffffff, 0);
	TEST_BITSCAN(0x400604, 0xff, 0);
	TEST_BITSCAN(0x400608, 0x3fffffff, 0);
	TEST_BITSCAN(0x40060c, 0xff, 0);
	TEST_BITSCAN(0x400610, 0xffffffff, 0);
	TEST_BITSCAN(0x400614, 0xffffffff, 0);
	TEST_BITSCAN(0x400618, 3, 0);
	TEST_BITSCAN(0x40061c, 0x7fffffff, 0);
	TEST_BITSCAN(0x400620, 0x7fffffff, 0);
	TEST_BITSCAN(0x400624, 0xff, 0);
	TEST_BITSCAN(0x400628, 0x7fffffff, 0);
	TEST_BITSCAN(0x40062c, 0x7fffffff, 0);
	TEST_BITSCAN(0x400680, 0x0000ffff, 0);
	TEST_BITSCAN(0x400684, 0x0011ffff, 0);
	int i;
	for (i = 0; i < 1000; i++) {
		uint32_t orig = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, 0x400630, orig);
		uint32_t exp = orig & 0x7f800000;
		if (orig & 0x80000000)
			exp = 0;
		uint32_t real = nva_rd32(ctx->cnum, 0x400630);
		if (real != exp) {
			printf("BETA scan mismatch: orig %08x expected %08x real %08x\n", orig, exp, real);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_scan_vstate(struct hwtest_ctx *ctx) {
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	TEST_BITSCAN(0x400640, 0xf1ff11ff, 0);
	TEST_BITSCAN(0x400644, 0x03177331, 0);
	TEST_BITSCAN(0x400648, 0x30ffffff, 0);
	TEST_BITSCAN(0x40064c, 0x30ffffff, 0);
	TEST_BITSCAN(0x400650, 0x111ff1ff, 0);
	TEST_BITSCAN(0x400654, 0xffffffff, 0);
	TEST_BITSCAN(0x400658, 0xffff00ff, 0);
	TEST_BITSCAN(0x40065c, 0xffff0113, 0);
	return HWTEST_RES_PASS;
}

static int test_state(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 1000; i++) {
		struct nv01_pgraph_state exp, real;
		nv01_pgraph_gen_state(ctx, &exp);
		nv01_pgraph_load_state(ctx, &exp);
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&exp, &real))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int nv01_pgraph_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset != 0x01)
		return HWTEST_RES_NA;
	if (!(nva_rd32(ctx->cnum, 0x200) & 1 << 24)) {
		printf("Mem controller not up.\n");
		return HWTEST_RES_UNPREP;
	}
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(nv01_pgraph,
	HWTEST_TEST(test_scan_access, 0),
	HWTEST_TEST(test_scan_debug, 0),
	HWTEST_TEST(test_scan_control, 0),
	HWTEST_TEST(test_scan_canvas, 0),
	HWTEST_TEST(test_scan_vtx, 0),
	HWTEST_TEST(test_scan_clip, 0),
	HWTEST_TEST(test_scan_context, 0),
	HWTEST_TEST(test_scan_vstate, 0),
	HWTEST_TEST(test_state, 0),
)
