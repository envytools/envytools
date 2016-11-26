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
#include "nvhw/pgraph.h"
#include "nvhw/chipset.h"
#include "nva.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * TODO:
 *  - cliprects
 *  - point VTX methods
 *  - lin/line VTX methods
 *  - tri VTX methods
 *  - rect VTX methods
 *  - tex* COLOR methods
 *  - ifc/bitmap VTX methods
 *  - ifc COLOR methods
 *  - bitmap MONO methods
 *  - blit VTX methods
 *  - blit pixel ops
 *  - tex* pixel ops
 *  - point rasterization
 *  - line rasterization
 *  - lin rasterization
 *  - tri rasterization
 *  - rect rasterization
 *  - texlin calculations
 *  - texquad calculations
 *  - quad rasterization
 *  - blit/ifc/bitmap rasterization
 *  - itm methods
 *  - ifm methods
 *  - notify
 *  - range interrupt
 *  - interrupt handling
 *  - ifm rasterization
 *  - ifm dma
 *  - itm rasterization
 *  - itm dma
 *  - itm pixel ops
 *  - cleanup & refactor everything
 */

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
	0x400640, 0x400644, 0x400648, 0x40064c,
	/* DMA, NOTIFY */
	0x400680, 0x400684,
	/* ACCESS */
	0x4006a4,
	/* DEBUG */
	0x400080, 0x400084, 0x400088,
	/* STATUS */
	0x4006b0,
	/* PFB_CONFIG */
	0x600200,
	0x600000,
};

static void nv01_pgraph_gen_state(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	state->chipset = nva_cards[cnum]->chipset;
	state->debug[0] = rnd() & 0x11111110;
	state->debug[1] = rnd() & 0x31111101;
	state->debug[2] = rnd() & 0x11111111;
	state->intr = 0;
	state->invalid = 0;
	state->intr_en = rnd() & 0x11111011;
	state->invalid_en = rnd() & 0x00011111;
	state->ctx_switch[0] = rnd() & 0x807fffff;
	state->ctx_control = rnd() & 0x11010103;
	for (int i = 0; i < 18; i++) {
		state->vtx_x[i] = rnd();
		state->vtx_y[i] = rnd();
	}
	for (int i = 0; i < 14; i++)
		state->vtx_beta[i] = rnd() & 0x01ffffff;
	for (int i = 0; i < 2; i++) {
		state->iclip[i] = rnd() & 0x3ffff;
		state->uclip_min[i] = rnd() & 0x3ffff;
		state->uclip_max[i] = rnd() & 0x3ffff;
		state->pattern_mono_rgb[i] = rnd() & 0x3fffffff;
		state->pattern_mono_a[i] = rnd() & 0xff;
		state->pattern_mono_bitmap[i] = rnd() & 0xffffffff;
		state->bitmap_color[i] = rnd() & 0x7fffffff;
	}
	state->pattern_config = rnd() & 3;
	state->rop = rnd() & 0xff;
	state->plane = rnd() & 0x7fffffff;
	state->chroma = rnd() & 0x7fffffff;
	state->beta = rnd() & 0x7f800000;
	state->canvas_config = rnd() & 0x01111011;
	state->xy_misc_0 = rnd() & 0xf1ff11ff;
	state->xy_misc_1[0] = rnd() & 0x03177331;
	state->xy_misc_4[0] = rnd() & 0x30ffffff;
	state->xy_misc_4[1] = rnd() & 0x30ffffff;
	state->valid[0] = rnd() & 0x111ff1ff;
	state->misc32[0] = rnd();
	state->subdivide = rnd() & 0xffff00ff;
	state->edgefill = rnd() & 0xffff0113;
	state->ctx_switch[1] = rnd() & 0xffff;
	state->notify = rnd() & 0x11ffff;
	state->dst_canvas_min = rnd() & 0xffffffff;
	state->dst_canvas_max = rnd() & 0x0fff0fff;
	state->cliprect_min[0] = rnd() & 0x0fff0fff;
	state->cliprect_min[1] = rnd() & 0x0fff0fff;
	state->cliprect_max[0] = rnd() & 0x0fff0fff;
	state->cliprect_max[1] = rnd() & 0x0fff0fff;
	state->cliprect_ctrl = rnd() & 0x113;
	state->access = rnd() & 0x0001f000;
	state->access |= 0x0f000111;
	state->status = 0;
	state->pfb_config = rnd() & 0x1370;
	state->pfb_config |= nva_rd32(cnum, 0x600200) & ~0x1371;
	state->pfb_boot = nva_rd32(cnum, 0x600000);
}

static void nv01_pgraph_load_state(int cnum, struct pgraph_state *state) {
	nva_wr32(cnum, 0x000200, 0xffffefff);
	nva_wr32(cnum, 0x000200, 0xffffffff);
	nva_wr32(cnum, 0x4006a4, 0x04000100);
	nva_wr32(cnum, 0x400100, 0xffffffff);
	nva_wr32(cnum, 0x400104, 0xffffffff);
	nva_wr32(cnum, 0x400140, state->intr_en);
	nva_wr32(cnum, 0x400144, state->invalid_en);
	nva_wr32(cnum, 0x400180, state->ctx_switch[0]);
	nva_wr32(cnum, 0x400190, state->ctx_control);
	for (int i = 0; i < 2; i++) {
		nva_wr32(cnum, 0x400450 + i * 4, state->iclip[i]);
		nva_wr32(cnum, 0x400460 + i * 8, state->uclip_min[i]);
		nva_wr32(cnum, 0x400464 + i * 8, state->uclip_max[i]);
	}
	for (int i = 0; i < 18; i++) {
		nva_wr32(cnum, 0x400400 + i * 4, state->vtx_x[i]);
		nva_wr32(cnum, 0x400480 + i * 4, state->vtx_y[i]);
	}
	for (int i = 0; i < 14; i++)
		nva_wr32(cnum, 0x400700 + i * 4, state->vtx_beta[i]);
	nva_wr32(cnum, 0x40061c, state->bitmap_color[0]);
	nva_wr32(cnum, 0x400620, state->bitmap_color[1]);
	nva_wr32(cnum, 0x400624, state->rop);
	nva_wr32(cnum, 0x400630, state->beta);
	for (int i = 0; i < 2; i++) {
		nva_wr32(cnum, 0x400600 + i * 8, state->pattern_mono_rgb[i]);
		nva_wr32(cnum, 0x400604 + i * 8, state->pattern_mono_a[i]);
		nva_wr32(cnum, 0x400610 + i * 4, state->pattern_mono_bitmap[i]);
	}
	nva_wr32(cnum, 0x400618, state->pattern_config);
	nva_wr32(cnum, 0x400628, state->plane);
	nva_wr32(cnum, 0x40062c, state->chroma);
	nva_wr32(cnum, 0x400688, state->dst_canvas_min);
	nva_wr32(cnum, 0x40068c, state->dst_canvas_max);
	nva_wr32(cnum, 0x400634, state->canvas_config);
	for (int i = 0; i < 2; i++) {
		nva_wr32(cnum, 0x400690 + i * 8, state->cliprect_min[i]);
		nva_wr32(cnum, 0x400694 + i * 8, state->cliprect_max[i]);
	}
	nva_wr32(cnum, 0x4006a0, state->cliprect_ctrl);
	nva_wr32(cnum, 0x400640, state->xy_misc_0);
	nva_wr32(cnum, 0x400644, state->xy_misc_1[0]);
	nva_wr32(cnum, 0x400648, state->xy_misc_4[0]);
	nva_wr32(cnum, 0x40064c, state->xy_misc_4[1]);
	nva_wr32(cnum, 0x400650, state->valid[0]);
	nva_wr32(cnum, 0x400654, state->misc32[0]);
	nva_wr32(cnum, 0x400658, state->subdivide);
	nva_wr32(cnum, 0x40065c, state->edgefill);
	nva_wr32(cnum, 0x400680, state->ctx_switch[1]);
	nva_wr32(cnum, 0x400684, state->notify);
	nva_wr32(cnum, 0x400080, state->debug[0]);
	nva_wr32(cnum, 0x400084, state->debug[1]);
	nva_wr32(cnum, 0x400088, state->debug[2]);
	nva_wr32(cnum, 0x4006a4, state->access);
	nva_wr32(cnum, 0x600200, state->pfb_config);
}

static void nv01_pgraph_dump_state(int cnum, struct pgraph_state *state) {
	int ctr = 0;
	while((state->status = nva_rd32(cnum, 0x4006b0))) {
		ctr++;
		if (ctr > 100000) {
			fprintf(stderr, "PGRAPH locked up [%08x]!\n", state->status);
			uint32_t save_intr_en = nva_rd32(cnum, 0x400140);
			uint32_t save_invalid_en = nva_rd32(cnum, 0x400144);
			uint32_t save_ctx_ctrl = nva_rd32(cnum, 0x400190);
			uint32_t save_access = nva_rd32(cnum, 0x4006a4);
			nva_wr32(cnum, 0x000200, 0xffffefff);
			nva_wr32(cnum, 0x000200, 0xffffffff);
			nva_wr32(cnum, 0x400140, save_intr_en);
			nva_wr32(cnum, 0x400144, save_invalid_en);
			nva_wr32(cnum, 0x400190, save_ctx_ctrl);
			nva_wr32(cnum, 0x4006a4, save_access);
			break;
		}
	}
	state->chipset = nva_cards[cnum]->chipset;
	state->access = nva_rd32(cnum, 0x4006a4);
	state->xy_misc_1[0] = nva_rd32(cnum, 0x400644); /* this one can be disturbed by *reading* VTX mem */
	nva_wr32(cnum, 0x4006a4, 0x04000100);
	state->trap_addr = nva_rd32(cnum, 0x4006b4);
	state->trap_data[0] = nva_rd32(cnum, 0x4006b8);
	state->intr = nva_rd32(cnum, 0x400100) & ~0x100;
	state->invalid = nva_rd32(cnum, 0x400104);
	state->intr_en = nva_rd32(cnum, 0x400140);
	state->invalid_en = nva_rd32(cnum, 0x400144);
	state->ctx_switch[0] = nva_rd32(cnum, 0x400180);
	state->ctx_control = nva_rd32(cnum, 0x400190) & ~0x00100000;
	for (int i = 0; i < 2; i++) {
		state->iclip[i] = nva_rd32(cnum, 0x400450 + i * 4);
		state->uclip_min[i] = nva_rd32(cnum, 0x400460 + i * 8);
		state->uclip_max[i] = nva_rd32(cnum, 0x400464 + i * 8);
	}
	for (int i = 0; i < 18; i++) {
		state->vtx_x[i] = nva_rd32(cnum, 0x400400 + i * 4);
		state->vtx_y[i] = nva_rd32(cnum, 0x400480 + i * 4);
	}
	for (int i = 0; i < 14; i++) {
		state->vtx_beta[i] = nva_rd32(cnum, 0x400700 + i * 4);
	}
	state->bitmap_color[0] = nva_rd32(cnum, 0x40061c);
	state->bitmap_color[1] = nva_rd32(cnum, 0x400620);
	state->rop = nva_rd32(cnum, 0x400624);
	state->plane = nva_rd32(cnum, 0x400628);
	state->beta = nva_rd32(cnum, 0x400630);
	for (int i = 0; i < 2; i++) {
		state->pattern_mono_rgb[i] = nva_rd32(cnum, 0x400600 + i * 8);
		state->pattern_mono_a[i] = nva_rd32(cnum, 0x400604 + i * 8);
		state->pattern_mono_bitmap[i] = nva_rd32(cnum, 0x400610 + i * 4);
	}
	state->pattern_config = nva_rd32(cnum, 0x400618);
	state->chroma = nva_rd32(cnum, 0x40062c);
	state->canvas_config = nva_rd32(cnum, 0x400634);
	state->dst_canvas_min = nva_rd32(cnum, 0x400688);
	state->dst_canvas_max = nva_rd32(cnum, 0x40068c);
	for (int i = 0; i < 2; i++) {
		state->cliprect_min[i] = nva_rd32(cnum, 0x400690 + i * 8);
		state->cliprect_max[i] = nva_rd32(cnum, 0x400694 + i * 8);
	}
	state->cliprect_ctrl = nva_rd32(cnum, 0x4006a0);
	state->valid[0] = nva_rd32(cnum, 0x400650);
	state->misc32[0] = nva_rd32(cnum, 0x400654);
	state->subdivide = nva_rd32(cnum, 0x400658);
	state->edgefill = nva_rd32(cnum, 0x40065c);
	state->xy_misc_0 = nva_rd32(cnum, 0x400640);
	state->xy_misc_4[0] = nva_rd32(cnum, 0x400648);
	state->xy_misc_4[1] = nva_rd32(cnum, 0x40064c);
	state->ctx_switch[1] = nva_rd32(cnum, 0x400680);
	state->notify = nva_rd32(cnum, 0x400684);
	state->debug[0] = nva_rd32(cnum, 0x400080);
	state->debug[1] = nva_rd32(cnum, 0x400084);
	state->debug[2] = nva_rd32(cnum, 0x400088);
}

static int nv01_pgraph_cmp_state(struct pgraph_state *orig, struct pgraph_state *exp, struct pgraph_state *real, bool broke = false) {
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
	// XXX: figure these out someday
#if 0
	CMP(trap_addr, "TRAP_ADDR")
	CMP(trap_data[0], "TRAP_DATA[0]")
#endif
	CMP(debug[0], "DEBUG[0]")
	CMP(debug[1], "DEBUG[1]")
	CMP(debug[2], "DEBUG[2]")
	CMP(intr, "INTR")
	CMP(intr_en, "INTR_EN")
	CMP(invalid, "INVALID")
	CMP(invalid_en, "INVALID_EN")
	CMP(ctx_switch[0], "CTX_SWITCH[0]")
	CMP(ctx_switch[1], "CTX_SWITCH[1]")
	CMP(notify, "NOTIFY")
	CMP(ctx_control, "CTX_CONTROL")
	for (int i = 0; i < 2; i++) {
		CMP(iclip[i], "ICLIP[%d]", i)
	}
	for (int i = 0; i < 2; i++) {
		CMP(uclip_min[i], "UCLIP_MIN[%d]", i)
		CMP(uclip_max[i], "UCLIP_MAX[%d]", i)
	}
	for (int i = 0; i < 18; i++) {
		CMP(vtx_x[i], "VTX_X[%d]", i)
		CMP(vtx_y[i], "VTX_Y[%d]", i)
	}
	for (int i = 0; i < 14; i++) {
		CMP(vtx_beta[i], "VTX_BETA[%d]", i)
	}
	CMP(xy_misc_0, "XY_MISC_0")
	CMP(xy_misc_1[0], "XY_MISC_1[0]")
	CMP(xy_misc_4[0], "XY_MISC_4[0]")
	CMP(xy_misc_4[1], "XY_MISC_4[1]")
	CMP(valid[0], "VALID[0]")
	CMP(misc32[0], "MISC32[0]")
	CMP(subdivide, "SUBDIVIDE")
	CMP(edgefill, "EDGEFILL")
	CMP(pattern_mono_rgb[0], "PATTERN_MONO_RGB[0]")
	CMP(pattern_mono_a[0], "PATTERN_MONO_A[0]")
	CMP(pattern_mono_rgb[1], "PATTERN_MONO_RGB[1]")
	CMP(pattern_mono_a[1], "PATTERN_MONO_A[1]")
	CMP(pattern_mono_bitmap[0], "PATTERN_MONO_BITMAP[0]")
	CMP(pattern_mono_bitmap[1], "PATTERN_MONO_BITMAP[1]")
	CMP(pattern_config, "PATTERN_CONFIG")
	CMP(bitmap_color[0], "BITMAP_COLOR[0]")
	CMP(bitmap_color[1], "BITMAP_COLOR[1]")
	CMP(rop, "ROP")
	CMP(beta, "BETA")
	CMP(plane, "PLANE")
	CMP(chroma, "CHROMA")
	CMP(dst_canvas_min, "DST_CANVAS_MIN")
	CMP(dst_canvas_max, "DST_CANVAS_MAX")
	CMP(canvas_config, "CANVAS_CONFIG")
	for (int i = 0; i < 2; i++) {
		CMP(cliprect_min[i], "CLIPRECT_MIN[%d]", i)
		CMP(cliprect_max[i], "CLIPRECT_MAX[%d]", i)
	}
	CMP(cliprect_ctrl, "CLIPRECT_CTRL")
	CMP(access, "ACCESS")
	if (broke && !print) {
		print = true;
		goto restart;
	}
	return broke;
}

static void nv01_pgraph_reset(struct pgraph_state *state) {
	state->valid[0] = 0;
	state->edgefill &= 0xffff0000;
	state->xy_misc_0 &= 0x1000;
	state->xy_misc_1[0] &= 0x03000000;
	state->xy_misc_4[0] &= 0xff000000;
	state->xy_misc_4[0] |= 0x00555500;
	state->xy_misc_4[1] &= 0xff000000;
	state->xy_misc_4[1] |= 0x00555500;
}

namespace {

class ScanTest : public hwtest::Test {
protected:
	int res;
	void bitscan(uint32_t reg, uint32_t all1, uint32_t all0) {
		uint32_t tmp = nva_rd32(cnum, reg);
		nva_wr32(cnum, reg, 0xffffffff);
		uint32_t rall1 = nva_rd32(cnum, reg);
		nva_wr32(cnum, reg, 0);
		uint32_t rall0 = nva_rd32(cnum, reg);
		nva_wr32(cnum, reg, tmp);
		if (rall1 != all1 || rall0 != all0) {
			printf("Bitscan mismatch for %06x: is %08x/%08x, expected %08x/%08x\n", reg, rall1, rall0, all1, all0);
			res = HWTEST_RES_FAIL;
		}
	}
	bool test_read(uint32_t reg, uint32_t exp) {
		uint32_t real = nva_rd32(cnum, reg);
		if (exp != real) {
			printf("Read mismatch for %06x: is %08x, expected %08x\n", reg, real, exp);
			res = HWTEST_RES_FAIL;
			return true;
		}
		return false;
	}
	ScanTest(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed), res(HWTEST_RES_PASS) {}
};

class ScanDebugTest : public ScanTest {
	int run() override {
		nva_wr32(cnum, 0x4006a4, 0x0f000111);
		bitscan(0x400080, 0x11111110, 0);
		bitscan(0x400084, 0x31111101, 0);
		bitscan(0x400088, 0x11111111, 0);
		return res;
	}
public:
	ScanDebugTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanControlTest : public ScanTest {
	int run() override {
		nva_wr32(cnum, 0x4006a4, 0x0f000111);
		bitscan(0x400140, 0x11111111, 0);
		bitscan(0x400144, 0x00011111, 0);
		bitscan(0x400180, 0x807fffff, 0);
		bitscan(0x400190, 0x11010103, 0);
		return res;
	}
public:
	ScanControlTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanCanvasTest : public ScanTest {
	int run() override {
		nva_wr32(cnum, 0x4006a4, 0x0f000111);
		bitscan(0x400634, 0x01111011, 0);
		nva_wr32(cnum, 0x400688, 0x7fff7fff);
		if(nva_rd32(cnum, 0x400688) != 0x7fff7fff) {
			res = HWTEST_RES_FAIL;
		}
		bitscan(0x400688, 0xffffffff, 0);
		bitscan(0x40068c, 0x0fff0fff, 0);
		bitscan(0x400690, 0x0fff0fff, 0);
		bitscan(0x400694, 0x0fff0fff, 0);
		bitscan(0x400698, 0x0fff0fff, 0);
		bitscan(0x40069c, 0x0fff0fff, 0);
		bitscan(0x4006a0, 0x113, 0);
		return res;
	}
public:
	ScanCanvasTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanVtxTest : public ScanTest {
	int run() override {
		nva_wr32(cnum, 0x4006a4, 0x0f000111);
		for (int i = 0 ; i < 18; i++) {
			bitscan(0x400400 + i * 4, 0xffffffff, 0);
			bitscan(0x400480 + i * 4, 0xffffffff, 0);
		}
		for (int i = 0 ; i < 14; i++) {
			bitscan(0x400700 + i * 4, 0x01ffffff, 0);
		}
		return res;
	}
public:
	ScanVtxTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanClipTest : public ScanTest {
	int run() override {
		nva_wr32(cnum, 0x4006a4, 0x0f000111);
		bitscan(0x400450, 0x0003ffff, 0);
		bitscan(0x400454, 0x0003ffff, 0);
		for (int i = 0; i < 1000; i++) {
			int idx = rnd() & 1;
			uint32_t v0 = rnd();
			uint32_t v1 = rnd();
			uint32_t v2 = rnd();
			nva_wr32(cnum, 0x400460 + idx * 8, v0);
			nva_wr32(cnum, 0x400464 + idx * 8, v1);
			v0 &= 0x3ffff;
			v1 &= 0x3ffff;
			if (test_read(0x400460 + idx * 8, v0) ||
			    test_read(0x400464 + idx * 8, v1)) {
				printf("v0 %08x v1 %08x\n", v0, v1);
			}
			nva_wr32(cnum, 0x400460 + idx * 8 + (rnd() & 4), v2);
			v2 &= 0x3ffff;
			if (test_read(0x400460 + idx * 8, v1) ||
			    test_read(0x400464 + idx * 8, v2)) {
				printf("v0 %08x v1 %08x v2 %08x\n", v0, v1, v2);
			}
		}
		return res;
	}
public:
	ScanClipTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanContextTest : public ScanTest {
	int run() override {
		nva_wr32(cnum, 0x4006a4, 0x0f000111);
		bitscan(0x400600, 0x3fffffff, 0);
		bitscan(0x400604, 0xff, 0);
		bitscan(0x400608, 0x3fffffff, 0);
		bitscan(0x40060c, 0xff, 0);
		bitscan(0x400610, 0xffffffff, 0);
		bitscan(0x400614, 0xffffffff, 0);
		bitscan(0x400618, 3, 0);
		bitscan(0x40061c, 0x7fffffff, 0);
		bitscan(0x400620, 0x7fffffff, 0);
		bitscan(0x400624, 0xff, 0);
		bitscan(0x400628, 0x7fffffff, 0);
		bitscan(0x40062c, 0x7fffffff, 0);
		bitscan(0x400680, 0x0000ffff, 0);
		bitscan(0x400684, 0x0011ffff, 0);
		for (int i = 0; i < 1000; i++) {
			uint32_t orig = rnd();
			nva_wr32(cnum, 0x400630, orig);
			uint32_t exp = orig & 0x7f800000;
			if (orig & 0x80000000)
				exp = 0;
			uint32_t real = nva_rd32(cnum, 0x400630);
			if (real != exp) {
				printf("BETA scan mismatch: orig %08x expected %08x real %08x\n", orig, exp, real);
				res = HWTEST_RES_FAIL;
				break;
			}
		}
		return res;
	}
public:
	ScanContextTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanVStateTest : public ScanTest {
	int run() override {
		nva_wr32(cnum, 0x4006a4, 0x0f000111);
		bitscan(0x400640, 0xf1ff11ff, 0);
		bitscan(0x400644, 0x03177331, 0);
		bitscan(0x400648, 0x30ffffff, 0);
		bitscan(0x40064c, 0x30ffffff, 0);
		bitscan(0x400650, 0x111ff1ff, 0);
		bitscan(0x400654, 0xffffffff, 0);
		bitscan(0x400658, 0xffff00ff, 0);
		bitscan(0x40065c, 0xffff0113, 0);
		return res;
	}
public:
	ScanVStateTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanAccessTest : public hwtest::Test {
	int run() override {
		uint32_t val = nva_rd32(cnum, 0x4006a4);
		int i;
		for (i = 0; i < 1000; i++) {
			uint32_t nv = rnd();
			uint32_t next = val;
			nva_wr32(cnum, 0x4006a4, nv);
			if (nv & 1 << 24)
				insrt(next, 0, 1, extr(nv, 0, 1));
			if (nv & 1 << 25)
				insrt(next, 4, 1, extr(nv, 4, 1));
			if (nv & 1 << 26)
				insrt(next, 8, 1, extr(nv, 8, 1));
			if (nv & 1 << 27)
				insrt(next, 12, 5, extr(nv, 12, 5));
			uint32_t real = nva_rd32(cnum, 0x4006a4);
			if (real != next) {
				printf("ACCESS mismatch: prev %08x write %08x expected %08x real %08x\n", val, nv, next, real);
				return HWTEST_RES_FAIL;
			}
			val = next;
		}
		return HWTEST_RES_PASS;
	}
public:
	ScanAccessTest(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class StateTest : public hwtest::RepeatTest {
protected:
	bool skip;
	struct pgraph_state orig, exp, real;
	virtual void adjust_orig() {}
	virtual void mutate() {}
	virtual bool other_fail() { return false; }
	virtual void print_fail() {}
	int run_once() override {
		skip = false;
		nv01_pgraph_gen_state(cnum, rnd, &orig);
		adjust_orig();
		nv01_pgraph_load_state(cnum, &orig);
		exp = orig;
		mutate();
		nv01_pgraph_dump_state(cnum, &real);
		bool fail = other_fail();
		if (skip)
			return HWTEST_RES_NA;
		if (nv01_pgraph_cmp_state(&orig, &exp, &real, fail)) {
			print_fail();
			return HWTEST_RES_FAIL;
		}
		return HWTEST_RES_PASS;
	}
public:
	StateTest(hwtest::TestOptions &opt, uint32_t seed) : RepeatTest(opt, seed) {}
};

class SoftResetTest : public StateTest {
protected:
	void mutate() override {
		nva_wr32(cnum, 0x400080, exp.debug[0] | 1);
		nv01_pgraph_reset(&exp);
	}
public:
	SoftResetTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOReadTest : public StateTest {
private:
	uint32_t reg;
protected:
	void mutate() override {
		int idx = rnd() % ARRAY_SIZE(nv01_pgraph_state_regs);
		reg = nv01_pgraph_state_regs[idx];
		nva_rd32(cnum, reg);
		if ((reg & ~0xf) == 0x400460) {
			exp.xy_misc_1[0] &= ~0xfff000;
		}
	}
	void print_fail() {
		printf("After reading %08x\n", reg);
	}
public:
	MMIOReadTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteTest : public StateTest {
private:
	uint32_t reg, val;
protected:
	void mutate() override {
		val = rnd();
		int idx;
		switch (rnd() % 69) {
			default:
				reg = 0x400140;
				exp.intr_en = val & 0x11111111;
				break;
			case 1:
				reg = 0x400144;
				exp.invalid_en = val & 0x11111;
				break;
			case 2:
				reg = 0x400180;
				exp.ctx_switch[0] = val & 0x807fffff;
				insrt(exp.debug[1], 0, 1, 0);
				insrt(exp.ctx_control, 24, 1, 0);
				break;
			case 3:
				reg = 0x400190;
				exp.ctx_control = val & 0x11010103;
				break;
			case 4:
				idx = rnd() % 18;
				reg = 0x400400 + idx * 4;
				exp.vtx_x[idx] = val;
				nv01_pgraph_vtx_fixup(&exp, 0, idx, val, 0, -1, 0);
				break;
			case 5:
				idx = rnd() % 18;
				reg = 0x400480 + idx * 4;
				exp.vtx_y[idx] = val;
				nv01_pgraph_vtx_fixup(&exp, 1, idx, val, 0, -1, 0);
				break;
			case 6:
				idx = rnd() % 18;
				reg = 0x400500 + idx * 4;
				exp.vtx_x[idx] = val;
				nv01_pgraph_vtx_fixup(&exp, 0, idx, val, 1, -1, idx & 3);
				break;
			case 7:
				idx = rnd() % 18;
				reg = 0x400580 + idx * 4;
				exp.vtx_y[idx] = val;
				nv01_pgraph_vtx_fixup(&exp, 1, idx, val, 1, -1, idx & 3);
				break;
			case 8:
				idx = rnd() % 14;
				reg = 0x400700 + idx * 4;
				exp.vtx_beta[idx] = val & 0x01ffffff;
				break;
			case 9:
				idx = rnd() & 1;
				reg = 0x400450 + idx * 4;
				insrt(exp.xy_misc_1[0], 14, 1, 0);
				insrt(exp.xy_misc_1[0], 18, 1, 0);
				insrt(exp.xy_misc_1[0], 20, 1, 0);
				nv01_pgraph_iclip_fixup(&exp, idx, val, 0);
				break;
			case 10:
				idx = rnd() & 1;
				reg = 0x400550 + idx * 4;
				insrt(exp.xy_misc_1[0], 14, 1, 0);
				insrt(exp.xy_misc_1[0], 18, 1, 0);
				insrt(exp.xy_misc_1[0], 20, 1, 0);
				nv01_pgraph_iclip_fixup(&exp, idx, val, 1);
				break;
			case 11:
				idx = rnd() & 3;
				reg = 0x400460 + idx * 4;
				nv01_pgraph_uclip_fixup(&exp, idx >> 1, idx & 1, val, 0);
				break;
			case 12:
				idx = rnd() & 3;
				reg = 0x400560 + idx * 4;
				nv01_pgraph_uclip_fixup(&exp, idx >> 1, idx & 1, val, 1);
				break;
			case 13:
				idx = rnd() & 1;
				reg = 0x400600 + idx * 8;
				exp.pattern_mono_rgb[idx] = val & 0x3fffffff;
				break;
			case 14:
				idx = rnd() & 1;
				reg = 0x400604 + idx * 8;
				exp.pattern_mono_a[idx] = val & 0xff;
				break;
			case 15:
				idx = rnd() & 1;
				reg = 0x400610 + idx * 4;
				exp.pattern_mono_bitmap[idx] = val;
				break;
			case 16:
				reg = 0x400618;
				exp.pattern_config = val & 3;
				break;
			case 17:
				idx = rnd() & 1;
				reg = 0x40061c + idx * 4;
				exp.bitmap_color[idx] = val & 0x7fffffff;
				break;
			case 18:
				reg = 0x400624;
				exp.rop = val & 0xff;
				break;
			case 19:
				reg = 0x400628;
				exp.plane = val & 0x7fffffff;
				break;
			case 20:
				reg = 0x40062c;
				exp.chroma = val & 0x7fffffff;
				break;
			case 21:
				reg = 0x400630;
				exp.beta = val & 0x7f800000;
				if (val & 0x80000000)
					exp.beta = 0;
				break;
			case 22:
				reg = 0x400634;
				exp.canvas_config = val & 0x01111011;
				break;
			case 23:
				reg = 0x400688;
				exp.dst_canvas_min = val & 0xffffffff;
				break;
			case 24:
				reg = 0x40068c;
				exp.dst_canvas_max = val & 0x0fff0fff;
				break;
			case 25:
				idx = rnd() & 1;
				reg = 0x400690 + idx * 8;
				exp.cliprect_min[idx] = val & 0x0fff0fff;
				break;
			case 26:
				idx = rnd() & 1;
				reg = 0x400694 + idx * 8;
				exp.cliprect_max[idx] = val & 0x0fff0fff;
				break;
			case 27:
				reg = 0x4006a0;
				exp.cliprect_ctrl = val & 0x113;
				break;
			case 28:
				reg = 0x400640;
				exp.xy_misc_0 = val & 0xf1ff11ff;
				break;
			case 29:
				idx = rnd() & 1;
				reg = 0x400644;
				exp.xy_misc_1[0] = val & 0x03177331;
				break;
			case 31:
				idx = rnd() & 1;
				reg = 0x400648 + idx * 4;
				exp.xy_misc_4[idx] = val & 0x30ffffff;
				break;
			case 32:
				reg = 0x400650;
				exp.valid[0] = val & 0x111ff1ff;
				break;
			case 33:
				reg = 0x400654;
				exp.misc32[0] = val;
				break;
			case 34:
				reg = 0x400658;
				exp.subdivide = val & 0xffff00ff;
				break;
			case 35:
				reg = 0x40065c;
				exp.edgefill = val & 0xffff0113;
				break;
			case 44:
				reg = 0x400680;
				exp.ctx_switch[1] = val & 0xffff;
				break;
			case 45:
				reg = 0x400684;
				exp.notify = val & 0x11ffff;
				break;
			case 46:
				reg = 0x4006a4;
				if (extr(val, 24, 1))
					insrt(exp.access, 0, 1, extr(val, 0, 1));
				if (extr(val, 25, 1))
					insrt(exp.access, 4, 1, extr(val, 4, 1));
				if (extr(val, 26, 1))
					insrt(exp.access, 8, 1, extr(val, 8, 1));
				if (extr(val, 27, 1))
					insrt(exp.access, 12, 5, extr(val, 12, 5));
				break;
			case 47:
				reg = 0x400080;
				exp.debug[0] = val & 0x11111110;
				if (extr(val, 0, 1))
					nv01_pgraph_reset(&exp);
				break;
			case 48:
				reg = 0x400084;
				exp.debug[1] = val & 0x31111101;
				break;
			case 49:
				reg = 0x400088;
				exp.debug[2] = val & 0x11111111;
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class ClipStatusTest : public hwtest::RepeatTest {
protected:
	int run_once() override {
		int xy = rnd() & 1;
		struct pgraph_state exp;
		nv01_pgraph_gen_state(cnum, rnd, &exp);
		uint32_t cls = extr(exp.access, 12, 5);
		nv01_pgraph_load_state(cnum, &exp);
		int32_t min, max;
		int32_t min_exp[2], max_exp[2];
		nv01_pgraph_clip_bounds(&exp, min_exp, max_exp);
		if (nv01_pgraph_is_tex_class(cls)) {
			min = max = 0x40000000;
			int bit;
			for (bit = 30; bit >= 15; bit--) {
				nva_wr32(cnum, 0x400400 + xy * 0x80, min ^ 1 << bit);
				if (nva_rd32(cnum, 0x400648 + xy * 4) & 0x300) {
					min ^= 1 << bit;
				}
				nva_wr32(cnum, 0x400400 + xy * 0x80, max ^ 1 << bit);
				if (!(nva_rd32(cnum, 0x400648 + xy * 4) & 0x400)) {
					max ^= 1 << bit;
				}
			}
			min >>= 15;
			max >>= 15;
			if (exp.xy_misc_1[0] & 0x02000000) {
				min >>= 4, max >>= 4;
				if (min_exp[xy] & 0x800)
					min_exp[xy] = 0x7ff;
				if (max_exp[xy] & 0x800)
					max_exp[xy] = 0x7ff;
			}
		} else {
			min = max = 0x20000;
			int bit;
			for (bit = 17; bit >= 0; bit--) {
				nva_wr32(cnum, 0x400400 + xy * 0x80, min ^ 1 << bit);
				if (nva_rd32(cnum, 0x400648 + xy * 4) & 0x300) {
					min ^= 1 << bit;
				}
				nva_wr32(cnum, 0x400400 + xy * 0x80, max ^ 1 << bit);
				if (!(nva_rd32(cnum, 0x400648 + xy * 4) & 0x400)) {
					max ^= 1 << bit;
				}
			}
		}
		if (min_exp[xy] != min || max_exp[xy] != max) {
			printf("%08x %08x %08x %08x  %08x %08x  %08x %08x  %08x  %03x %03x\n", cls, exp.xy_misc_1[0], min, max, exp.dst_canvas_min, exp.dst_canvas_max, exp.uclip_min[xy], exp.uclip_max[xy], exp.iclip[xy], min_exp[xy], max_exp[xy]);
			return HWTEST_RES_FAIL;
		}
		return HWTEST_RES_PASS;
	}
public:
	ClipStatusTest(hwtest::TestOptions &opt, uint32_t seed) : RepeatTest(opt, seed) {}
};

class VtxWriteTest : public StateTest {
private:
	uint32_t reg, val;
protected:
	void adjust_orig() override {
		if (rnd() & 1) {
			/* rare and complicated enough to warrant better testing */
			orig.access = 0x0f00d111 + (rnd() & 0x11000);
		}
	}
	void mutate() override {
		int idx = rnd() % 18;
		int xy = rnd() & 1;
		int rel = rnd() & 1;
		if (rnd() & 1) {
			/* rare and complicated enough to warrant better testing */
			idx &= 1;
			idx |= 0x10;
		}
		reg = 0x400400 + idx * 4 + xy * 0x80 + rel * 0x100;
		val = rnd();
		nva_wr32(cnum, reg, val);
		nv01_pgraph_vtx_fixup(&exp, xy, idx, val, rel, -1, rel ? idx & 3 : 0);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	VtxWriteTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class IClipWriteTest : public StateTest {
private:
	uint32_t reg, val;
protected:
	void mutate() override {
		int xy = rnd() & 1;
		int rel = rnd() & 1;
		reg = 0x400450 + xy * 4 + rel * 0x100;
		val = rnd();
		nva_wr32(cnum, reg, val);
		insrt(exp.xy_misc_1[0], 14, 1, 0);
		insrt(exp.xy_misc_1[0], 18, 1, 0);
		insrt(exp.xy_misc_1[0], 20, 1, 0);
		nv01_pgraph_iclip_fixup(&exp, xy, val, rel);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	IClipWriteTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class UClipWriteTest : public StateTest {
private:
	uint32_t reg, val;
protected:
	void mutate() override {
		int xy = rnd() & 1;
		int idx = rnd() & 1;
		int rel = rnd() & 1;
		reg = 0x400460 + xy * 8 + idx * 4 + rel * 0x100;
		val = rnd();
		nva_wr32(cnum, reg, val);
		nv01_pgraph_uclip_fixup(&exp, xy, idx, val, rel);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	UClipWriteTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MthdTest : public StateTest {
protected:
	uint32_t cls, mthd, val;
	virtual void choose_mthd() = 0;
	virtual void emulate_mthd() = 0;
	void adjust_orig() override {
		insrt(orig.notify, 16, 1, 0);
		insrt(orig.notify, 20, 1, 0);
	}
	void mutate() override {
		val = rnd();
		choose_mthd();
		nva_wr32(cnum, 0x400000 | cls << 16 | mthd, val);
		emulate_mthd();
	}
	void print_fail() {
		printf("Mthd %02x.%04x <- %08x\n", cls, mthd, val);
	}
	MthdTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MthdCtxSwitchTest : public MthdTest {
	void adjust_orig() override {
		// no super call by design
		insrt(orig.notify, 16, 1, 0);
	}
	void choose_mthd() override {
		int classes[20] = {
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
			0x08, 0x09, 0x0a, 0x0b, 0x0c,
			0x0d, 0x0e, 0x1d, 0x1e,
			0x10, 0x11, 0x12, 0x13, 0x14,
		};
		cls = classes[rnd() % 20];
		mthd = 0;
	}
	void emulate_mthd() override {
		bool chsw = false;
		int och = extr(exp.ctx_switch[0], 16, 7);
		int nch = extr(val, 16, 7);
		if ((val & 0x007f8000) != (exp.ctx_switch[0] & 0x007f8000))
			chsw = true;
		if (!extr(exp.ctx_control, 16, 1))
			chsw = true;
		bool volatile_reset = extr(val, 31, 1) && extr(exp.debug[2], 28, 1) && (!extr(exp.ctx_control, 16, 1) || och == nch);
		if (chsw) {
			exp.ctx_control |= 0x01010000;
			exp.intr |= 0x10;
			exp.access &= ~0x101;
		} else {
			exp.ctx_control &= ~0x01000000;
		}
		insrt(exp.access, 12, 5, cls);
		insrt(exp.debug[1], 0, 1, volatile_reset);
		if (volatile_reset) {
			exp.bitmap_color[0] &= 0x3fffffff;
			exp.bitmap_color[1] &= 0x3fffffff;
			exp.valid[0] &= 0x11000000;
			exp.xy_misc_0 = 0;
			exp.xy_misc_1[0] &= 0x33300;
			exp.xy_misc_4[0] = 0x555500;
			exp.xy_misc_4[1] = 0x555500;
			exp.misc32[0] &= 0x00ff00ff;
			exp.subdivide &= 0xffff0000;
		}
		exp.ctx_switch[0] = val & 0x807fffff;
		if (exp.notify & 0x100000) {
			exp.intr |= 0x10000001;
			exp.invalid |= 0x10000;
			exp.access &= ~0x101;
			exp.notify &= ~0x100000;
		}
	}
public:
	MthdCtxSwitchTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdNotifyTest : public MthdTest {
	void adjust_orig() override {
		// no super call by design
	}
	void choose_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		int classes[20] = {
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
			0x08, 0x09, 0x0a, 0x0b, 0x0c,
			0x0d, 0x0e, 0x1d, 0x1e,
			0x10, 0x11, 0x12, 0x13, 0x14,
		};
		cls = classes[rnd() % 20];
		mthd = 0x104;
	}
	void emulate_mthd() override {
		if (val && (cls & 0xf) != 0xd && (cls & 0xf) != 0xe)
			exp.invalid |= 0x10;
		if (exp.notify & 0x100000 && !exp.invalid)
			exp.intr |= 0x10000000;
		if (!(exp.ctx_switch[0] & 0x100))
			exp.invalid |= 0x100;
		if (exp.notify & 0x110000)
			exp.invalid |= 0x1000;
		if (exp.invalid) {
			exp.intr |= 1;
			exp.access &= ~0x101;
		} else {
			exp.notify |= 0x10000;
		}
	}
public:
	MthdNotifyTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdInvalidTest : public MthdTest {
	int repeats() override {
		return 1;
	}
	void choose_mthd() override {
		// set by constructor
	}
	void emulate_mthd() override {
		exp.intr |= 1;
		exp.invalid |= 1;
		exp.access &= ~0x101;
	}
public:
	MthdInvalidTest(hwtest::TestOptions &opt, uint32_t seed, uint32_t cls, uint32_t mthd) : MthdTest(opt, seed) {
		this->cls = cls;
		this->mthd = mthd;
	}
};

class ClsMthdInvalidTests : public hwtest::Test {
protected:
	virtual int cls() = 0;
	virtual bool is_valid(uint32_t mthd) = 0;
	bool subtests_boring() override {
		return true;
	}
	Subtests subtests() override {
		Subtests res;
		for (uint32_t mthd = 0; mthd < 0x10000; mthd += 4) {
			if (mthd != 0 && !is_valid(mthd)) {
				char buf[5];
				snprintf(buf, sizeof buf, "%04x", mthd);
				res.push_back({strdup(buf), new MthdInvalidTest(opt, rnd(), cls(), mthd)});
			}
		}
		return res;
	}
	ClsMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class BetaMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x01; }
	bool is_valid(uint32_t mthd) override {
		return mthd == 0x104 || mthd == 0x300;
	}
public:
	BetaMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class RopMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x02; }
	bool is_valid(uint32_t mthd) override {
		return mthd == 0x104 || mthd == 0x300;
	}
public:
	RopMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class ChromaMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x03; }
	bool is_valid(uint32_t mthd) override {
		return mthd == 0x104 || mthd == 0x304;
	}
public:
	ChromaMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class PlaneMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x04; }
	bool is_valid(uint32_t mthd) override {
		return mthd == 0x104 || mthd == 0x304;
	}
public:
	PlaneMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class ClipMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x05; }
	bool is_valid(uint32_t mthd) override {
		return mthd == 0x104 || mthd == 0x300 || mthd == 0x304;
	}
public:
	ClipMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class PatternMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x06; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x308)
			return true;
		if (mthd >= 0x310 && mthd <= 0x31c)
			return true;
		return false;
	}
public:
	PatternMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class PointMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x08; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x400 && mthd < 0x580)
			return true;
		return false;
	}
public:
	PointMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class LineMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x09; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x400 && mthd < 0x680)
			return true;
		return false;
	}
public:
	LineMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class LinMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x0a; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x400 && mthd < 0x680)
			return true;
		return false;
	}
public:
	LinMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class TriMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x0b; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x310 && mthd < 0x31c)
			return true;
		if (mthd >= 0x320 && mthd < 0x338)
			return true;
		if (mthd >= 0x400 && mthd < 0x600)
			return true;
		return false;
	}
public:
	TriMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class RectMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x0c; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	RectMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class TexLinMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x0d; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x310 && mthd < 0x320)
			return true;
		if (mthd >= 0x350 && mthd < 0x360)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	TexLinMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class TexQuadMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x0e; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x310 && mthd < 0x334)
			return true;
		if (mthd >= 0x350 && mthd < 0x374)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	TexQuadMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class TexLinBetaMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x1d; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x310 && mthd < 0x320)
			return true;
		if (mthd >= 0x350 && mthd < 0x360)
			return true;
		if (mthd >= 0x380 && mthd < 0x388)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	TexLinBetaMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class TexQuadBetaMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x1e; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x310 && mthd < 0x334)
			return true;
		if (mthd >= 0x350 && mthd < 0x374)
			return true;
		if (mthd >= 0x380 && mthd < 0x394)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	TexQuadBetaMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class BlitMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x10; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x300 || mthd == 0x304 || mthd == 0x308)
			return true;
		return false;
	}
public:
	BlitMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class IfcMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x11; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304 || mthd == 0x308 || mthd == 0x30c)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	IfcMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class BitmapMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x12; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd >= 0x308 && mthd < 0x31c)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	BitmapMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class IfmMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x13; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd >= 0x308 && mthd < 0x318)
			return true;
		if (mthd >= 0x40 && mthd < 0x80)
			return true;
		return false;
	}
public:
	IfmMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class ItmMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x14; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd >= 0x308 && mthd < 0x318)
			return true;
		return false;
	}
public:
	ItmMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class MthdInvalidTests : public hwtest::Test {
	Subtests subtests() override {
		return {
			{"beta", new BetaMthdInvalidTests(opt, rnd())},
			{"rop", new RopMthdInvalidTests(opt, rnd())},
			{"chroma", new ChromaMthdInvalidTests(opt, rnd())},
			{"plane", new PlaneMthdInvalidTests(opt, rnd())},
			{"clip", new ClipMthdInvalidTests(opt, rnd())},
			{"pattern", new PatternMthdInvalidTests(opt, rnd())},
			{"point", new PointMthdInvalidTests(opt, rnd())},
			{"line", new LineMthdInvalidTests(opt, rnd())},
			{"lin", new LinMthdInvalidTests(opt, rnd())},
			{"tri", new TriMthdInvalidTests(opt, rnd())},
			{"rect", new RectMthdInvalidTests(opt, rnd())},
			{"texlin", new TexLinMthdInvalidTests(opt, rnd())},
			{"texquad", new TexQuadMthdInvalidTests(opt, rnd())},
			{"texlinbeta", new TexLinBetaMthdInvalidTests(opt, rnd())},
			{"texquadbeta", new TexQuadBetaMthdInvalidTests(opt, rnd())},
			{"blit", new BlitMthdInvalidTests(opt, rnd())},
			{"ifc", new IfcMthdInvalidTests(opt, rnd())},
			{"bitmap", new BitmapMthdInvalidTests(opt, rnd())},
			{"ifm", new IfmMthdInvalidTests(opt, rnd())},
			{"itm", new ItmMthdInvalidTests(opt, rnd())},
		};
	}
public:
	MthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class MthdBetaTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x01;
		mthd = 0x300;
	}
	void emulate_mthd() override {
		exp.beta = val;
		if (exp.beta & 0x80000000)
			exp.beta = 0;
		exp.beta &= 0x7f800000;
	}
public:
	MthdBetaTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdRopTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x02;
		mthd = 0x300;
	}
	void emulate_mthd() override {
		exp.rop = val & 0xff;
		if (val & ~0xff) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access &= ~0x101;
		}
	}
public:
	MthdRopTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdChromaPlaneTest : public MthdTest {
	void choose_mthd() override {
		if (rnd() & 1)
			cls = 0x03;
		else
			cls = 0x04;
		mthd = 0x304;
	}
	void emulate_mthd() override {
		uint32_t color = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
		if (cls == 0x04)
			exp.plane = color;
		else
			exp.chroma = color;
	}
public:
	MthdChromaPlaneTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternShapeTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x06;
		mthd = 0x308;
		if (rnd() & 1)
			val &= 0xf;
	}
	void emulate_mthd() override {
		exp.pattern_config = val & 3;
		if (val > 2) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access &= ~0x101;
		}
	}
public:
	MthdPatternShapeTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternMonoColorTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		cls = 0x06;
		idx = rnd() & 1;
		mthd = 0x310 + idx * 4;
	}
	void emulate_mthd() override {
		struct pgraph_color c = pgraph_expand_color(&exp, val);
		exp.pattern_mono_rgb[idx] = c.r << 20 | c.g << 10 | c.b;
		exp.pattern_mono_a[idx] = c.a;
	}
public:
	MthdPatternMonoColorTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternMonoBitmapTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		cls = 0x06;
		idx = rnd() & 1;
		mthd = 0x318 + idx * 4;
	}
	void emulate_mthd() override {
		exp.pattern_mono_bitmap[idx] = pgraph_expand_mono(&exp, val);
	}
public:
	MthdPatternMonoBitmapTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSolidColorTest : public MthdTest {
	void choose_mthd() override {
		switch (rnd() % 5) {
			case 0:
				mthd = 0x304;
				cls = 8 + rnd() % 5;
				break;
			case 1:
				mthd = 0x500 | (rnd()&0x78);
				cls = 8;
				break;
			case 2:
				mthd = 0x600 | (rnd()&0x78);
				cls = 9 + (rnd() & 1);
				break;
			case 3:
				mthd = 0x500 | (rnd()&0x70);
				cls = 0xb;
				break;
			case 4:
				mthd = 0x580 | (rnd()&0x78);
				cls = 0xb;
				break;
			default:
				abort();
		}
	}
	void emulate_mthd() override {
		exp.misc32[0] = val;
	}
public:
	MthdSolidColorTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdBitmapColorTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		idx = rnd() & 1;
		cls = 0x12;
		mthd = 0x308 + idx * 4;
	}
	void emulate_mthd() override {
		exp.bitmap_color[idx] = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
	}
public:
	MthdBitmapColorTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSubdivideTest : public MthdTest {
	void choose_mthd() override {
		bool beta = rnd() & 1;
		bool quad = rnd() & 1;
		cls = 0xd + beta * 0x10 + quad;
		mthd = 0x304;
		if (rnd() & 1)
			val &= ~0xff00;
	}
	void emulate_mthd() override {
		exp.subdivide = val & 0xffff00ff;
		bool err = false;
		if (val & 0xff00)
			err = true;
		for (int j = 0; j < 8; j++) {
			if (extr(val, 4*j, 4) > 8)
				err = true;
			if (j < 2 && extr(val, 4*j, 4) < 2)
				err = true;
		}
		if (err) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access &= ~0x101;
		}
	}
public:
	MthdSubdivideTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdVtxBetaTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		bool quad = rnd() & 1;
		cls = 0x1d + quad;
		idx = rnd() % (quad ? 5 : 2);
		mthd = 0x380 + idx * 4;
	}
	void emulate_mthd() override {
		uint32_t rclass = extr(exp.access, 12, 5);
		for (int j = 0; j < 2; j++) {
			int vid = idx * 2 + j;
			if (vid == 9 && (rclass & 0xf) == 0xe)
				break;
			uint32_t beta = extr(val, j*16, 16);
			beta &= 0xff80;
			beta <<= 8;
			beta |= 0x4000;
			if (beta & 1 << 23)
				beta |= 1 << 24;
			exp.vtx_beta[vid] = beta;
		}
		if (rclass == 0x1d || rclass == 0x1e)
			exp.valid[0] |= 1 << (12 + idx);
	}
public:
	MthdVtxBetaTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdClipTest : public MthdTest {
	bool is_size;
	int repeats() override { return 10000; };
	void adjust_orig() override {
		is_size = rnd() & 1;
		if (rnd() & 1) {
			insrt(orig.vtx_x[15], 16, 15, (rnd() & 1) ? 0 : 0x7fff);
			insrt(orig.vtx_y[15], 16, 15, (rnd() & 1) ? 0 : 0x7fff);
		}
		/* XXX: submitting on BLIT causes an actual blit */
		if (is_size && extr(orig.access, 12, 5) == 0x10)
			insrt(orig.access, 12, 5, 0);
		MthdTest::adjust_orig();
	}
	void choose_mthd() override {
		cls = 0x05;
		mthd = 0x300 + is_size * 4;;
	}
	void emulate_mthd() override {
		nv01_pgraph_set_clip(&exp, is_size, val);
	}
public:
	MthdClipTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdVtxTest : public MthdTest {
	bool first, poly, draw, fract;
	int repeats() override { return 10000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		if (rnd() & 1) {
			insrt(orig.access, 12, 5, 8 + rnd() % 4);
		}
		if (rnd() & 1)
			orig.valid[0] |= 0x1ff1ff;
		if (rnd() & 1)
			orig.valid[0] |= 0x033033;
		if (rnd() & 1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (rnd() & 1) {
			orig.valid[0] &= ~0x11000000;
			orig.xy_misc_1[0] &= ~0x330;
		}
		if (rnd() & 1) {
			orig.valid[0] &= ~0x00f00f;
		}
	}
	void choose_mthd() override {
		first = poly = draw = fract = false;;
		switch (rnd() % 27) {
			case 0:
				cls = 0x10;
				mthd = 0x300;
				first = true;
				break;
			case 1:
				cls = 0x11;
				mthd = 0x304;
				first = true;
				break;
			case 2:
				cls = 0x12;
				mthd = 0x310;
				first = true;
				break;
			case 3:
				cls = 0x13;
				mthd = 0x308;
				first = true;
				break;
			case 4:
				cls = 0x14;
				mthd = 0x308;
				first = true;
				break;
			case 5:
				cls = 0x10;
				mthd = 0x304;
				first = false;
				break;
			case 6:
				cls = 0x0c;
				mthd = 0x400 | (rnd() & 0x78);
				first = true;
				break;
			case 7:
				cls = 0x0b;
				mthd = 0x310;
				first = true;
				break;
			case 8:
				cls = 0x0b;
				mthd = 0x504 | (rnd() & 0x70);
				first = true;
				break;
			case 9:
				cls = 0x09;
				mthd = 0x400 | (rnd() & 0x78);
				first = true;
				break;
			case 10:
				cls = 0x0a;
				mthd = 0x400 | (rnd() & 0x78);
				first = true;
				break;
			case 11: {
				int beta = rnd() & 1;
				int idx = rnd() & 3;
				fract = rnd() & 1;
				cls = 0x0d | beta << 4;
				mthd = (0x310 + idx * 4) | fract << 6;
				first = idx == 0;
				break;
			}
			case 12: {
				int beta = rnd() & 1;
				int idx = rnd() % 9;
				fract = rnd() & 1;
				cls = 0x0e | beta << 4;
				mthd = (0x310 + idx * 4) | fract << 6;
				first = idx == 0;
				break;
			}
			case 13:
				cls = 0x08;
				mthd = 0x400 | (rnd() & 0x7c);
				first = true;
				draw = true;
				break;
			case 14:
				cls = 0x08;
				mthd = 0x504 | (rnd() & 0x78);
				first = true;
				draw = true;
				break;
			case 15:
				cls = 0x09;
				mthd = 0x404 | (rnd() & 0x78);
				first = false;
				draw = true;
				break;
			case 16:
				cls = 0x0a;
				mthd = 0x404 | (rnd() & 0x78);
				first = false;
				draw = true;
				break;
			case 17:
				cls = 0x0b;
				mthd = 0x314;
				first = false;
				break;
			case 18:
				cls = 0x0b;
				mthd = 0x318;
				first = false;
				draw = true;
				break;
			case 19:
				cls = 0x0b;
				mthd = 0x508 | (rnd() & 0x70);
				first = false;
				break;
			case 20:
				cls = 0x0b;
				mthd = 0x50c | (rnd() & 0x70);
				first = false;
				draw = true;
				break;
			case 21:
				cls = 0x09;
				mthd = 0x500 | (rnd() & 0x7c);
				first = false;
				draw = true;
				poly = true;
				break;
			case 22:
				cls = 0x0a;
				mthd = 0x500 | (rnd() & 0x7c);
				first = false;
				draw = true;
				poly = true;
				break;
			case 23:
				cls = 0x09;
				mthd = 0x604 | (rnd() & 0x78);
				first = false;
				draw = true;
				poly = true;
				break;
			case 24:
				cls = 0x0a;
				mthd = 0x604 | (rnd() & 0x78);
				first = false;
				draw = true;
				poly = true;
				break;
			case 25:
				cls = 0x0b;
				mthd = 0x400 | (rnd() & 0x7c);
				first = false;
				draw = true;
				poly = true;
				break;
			case 26:
				cls = 0x0b;
				mthd = 0x584 | (rnd() & 0x78);
				first = false;
				draw = true;
				poly = true;
				break;
			default:
				abort();
		}
	}
	void emulate_mthd() override {
		int rcls = extr(exp.access, 12, 5);
		if (first)
			insrt(exp.xy_misc_0, 28, 4, 0);
		int idx = extr(exp.xy_misc_0, 28, 4);
		if (rcls == 0x11 || rcls == 0x12 || rcls == 0x13)
			idx = 4;
		if (nv01_pgraph_is_tex_class(rcls)) {
			idx = (mthd - 0x10) >> 2 & 0xf;
			if (idx >= 12)
				idx -= 8;
			if (extr(exp.xy_misc_1[0], 24, 1) && extr(exp.xy_misc_1[0], 25, 1) != (uint32_t)fract) {
				exp.valid[0] &= ~0xffffff;
			}
		} else {
			fract = 0;
		}
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[0], 24, 1, 1);
		insrt(exp.xy_misc_1[0], 25, 1, fract);
		nv01_pgraph_bump_vtxid(&exp);
		nv01_pgraph_set_vtx(&exp, 0, idx, extrs(val, 0, 16), false);
		nv01_pgraph_set_vtx(&exp, 1, idx, extrs(val, 16, 16), false);
		if (!poly) {
			if (idx <= 8)
				exp.valid[0] |= 0x1001 << idx;
			if (rcls >= 0x09 && rcls <= 0x0b) {
				if (first) {
					exp.valid[0] &= ~0xffffff;
					exp.valid[0] |= 0x011111;
				} else {
					exp.valid[0] |= 0x10010 << (idx & 3);
				}
			}
			if ((rcls == 0x10 || rcls == 0x0c) && first)
				exp.valid[0] |= 0x100;
		} else {
			if (rcls >= 9 && rcls <= 0xb) {
				exp.valid[0] |= 0x10010 << (idx & 3);
			}
		}
		if (draw)
			nv01_pgraph_prep_draw(&exp, poly);
		// XXX
		if (draw && (rcls == 0x11 || rcls == 0x12 || rcls == 0x13))
			skip = true;
	}
	bool other_fail() override {
		int rcls = extr(exp.access, 12, 5);
		if (real.status && (rcls == 0x0b || rcls == 0x0c)) {
			/* Hung PGRAPH... */
			skip = true;
		}
		return false;
	}
public:
	MthdVtxTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdX32Test : public MthdTest {
	bool first, poly;
	int repeats() override { return 10000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		if (rnd() & 1) {
			insrt(orig.access, 12, 5, 8 + rnd() % 4);
		}
		if (rnd() & 1)
			orig.valid[0] |= 0x1ff1ff;
		if (rnd() & 1)
			orig.valid[0] |= 0x033033;
		if (rnd() & 1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (rnd() & 1) {
			orig.valid[0] &= ~0x11000000;
			orig.xy_misc_1[0] &= ~0x330;
		}
		if (rnd() & 1) {
			orig.valid[0] &= ~0x00f00f;
		}
	}
	void choose_mthd() override {
		first = poly = false;
		switch (rnd() % 7) {
			case 0:
				cls = 0x08;
				mthd = 0x480 | (rnd() & 0x78);
				first = true;
				break;
			case 1:
				cls = 0x09;
				mthd = 0x480 | (rnd() & 0x78);
				first = !(mthd & 8);
				break;
			case 2:
				cls = 0x0a;
				mthd = 0x480 | (rnd() & 0x78);
				first = !(mthd & 8);
				break;
			case 3:
				cls = 0x0b;
				mthd = 0x320 + 8 * (rnd() % 3);
				first = (mthd == 0x320);
				break;
			case 4:
				cls = 0x09;
				mthd = 0x580 | (rnd() & 0x78);
				first = false;
				poly = true;
				break;
			case 5:
				cls = 0x0a;
				mthd = 0x580 | (rnd() & 0x78);
				first = false;
				poly = true;
				break;
			case 6:
				cls = 0x0b;
				mthd = 0x480 | (rnd() & 0x78);
				first = false;
				poly = true;
				break;
			default:
				abort();
		}
		if (rnd() & 1) {
			val = extrs(val, 0, 17);
		}
	}
	void emulate_mthd() override {
		int rcls = extr(exp.access, 12, 5);
		if (first)
			insrt(exp.xy_misc_0, 28, 4, 0);
		int idx = extr(exp.xy_misc_0, 28, 4);
		if (nv01_pgraph_is_tex_class(rcls)) {
			if (extr(exp.xy_misc_1[0], 24, 1) && extr(exp.xy_misc_1[0], 25, 1)) {
				exp.valid[0] &= ~0xffffff;
			}
		}
		insrt(exp.xy_misc_0, 28, 4, idx);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[0], 24, 1, 1);
		insrt(exp.xy_misc_1[0], 25, 1, 0);
		nv01_pgraph_set_vtx(&exp, 0, idx, val, true);
		if (!poly) {
			if (idx <= 8)
				exp.valid[0] |= 1 << idx;
			if (rcls >= 0x09 && rcls <= 0x0b) {
				if (first) {
					exp.valid[0] &= ~0xffffff;
					exp.valid[0] |= 0x000111;
				} else {
					exp.valid[0] |= 0x10 << (idx & 3);
				}
			}
			if ((rcls == 0x10 || rcls == 0x0c) && first)
				exp.valid[0] |= 0x100;
		} else {
			if (rcls >= 9 && rcls <= 0xb) {
				if (exp.valid[0] & 0xf00f)
					exp.valid[0] &= ~0x100;
				exp.valid[0] |= 0x10 << (idx & 3);
			}
		}
	}
public:
	MthdX32Test(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdY32Test : public MthdTest {
	bool poly, draw;
	int repeats() override { return 10000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		if (rnd() & 1) {
			insrt(orig.access, 12, 5, 8 + rnd() % 4);
		}
		if (rnd() & 1)
			orig.valid[0] |= 0x1ff1ff;
		if (rnd() & 1)
			orig.valid[0] |= 0x033033;
		if (rnd() & 1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (rnd() & 1) {
			orig.valid[0] &= ~0x11000000;
			orig.xy_misc_1[0] &= ~0x330;
		}
		if (rnd() & 1) {
			orig.valid[0] &= ~0x00f00f;
		}
	}
	void choose_mthd() override {
		poly = draw = false;
		switch (rnd() % 7) {
			case 0:
				cls = 0x08;
				mthd = 0x484 | (rnd() & 0x78);
				draw = true;
				break;
			case 1:
				cls = 0x09;
				mthd = 0x484 | (rnd() & 0x78);
				draw = !!(mthd & 0x8);
				break;
			case 2:
				cls = 0x0a;
				mthd = 0x484 | (rnd() & 0x78);
				draw = !!(mthd & 0x8);
				break;
			case 3:
				cls = 0x0b;
				mthd = 0x324 + 8 * (rnd() % 3);
				draw = mthd == 0x334;
				break;
			case 4:
				cls = 0x09;
				mthd = 0x584 | (rnd() & 0x78);
				poly = true;
				draw = true;
				break;
			case 5:
				cls = 0x0a;
				mthd = 0x584 | (rnd() & 0x78);
				poly = true;
				draw = true;
				break;
			case 6:
				cls = 0x0b;
				mthd = 0x484 | (rnd() & 0x78);
				poly = true;
				draw = true;
				break;
			default:
				abort();
		}
		if (rnd() & 1)
			val = extrs(val, 0, 17);
	}
	void emulate_mthd() override {
		int rcls = extr(exp.access, 12, 5);
		int idx = extr(exp.xy_misc_0, 28, 4);
		if (nv01_pgraph_is_tex_class(rcls)) {
			if (extr(exp.xy_misc_1[0], 24, 1) && extr(exp.xy_misc_1[0], 25, 1)) {
				exp.valid[0] &= ~0xffffff;
			}
		}
		nv01_pgraph_bump_vtxid(&exp);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[0], 24, 1, 1);
		insrt(exp.xy_misc_1[0], 25, 1, 0);
		nv01_pgraph_set_vtx(&exp, 1, idx, val, true);
		if (!poly) {
			if (idx <= 8)
				exp.valid[0] |= 0x1000 << idx;
			if (rcls >= 0x09 && rcls <= 0x0b) {
				exp.valid[0] |= 0x10000 << (idx & 3);
			}
		} else {
			if (rcls >= 9 && rcls <= 0xb) {
				exp.valid[0] |= 0x10000 << (idx & 3);
			}
		}
		if (draw)
			nv01_pgraph_prep_draw(&exp, poly);
		// XXX
		if (draw && (rcls == 0x11 || rcls == 0x12 || rcls == 0x13))
			skip = true;
	}
	bool other_fail() override {
		int rcls = extr(exp.access, 12, 5);
		if (real.status && (rcls == 0x0b || rcls == 0x0c)) {
			/* Hung PGRAPH... */
			skip = true;
		}
		return false;
	}
public:
	MthdY32Test(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdIfcSizeInTest : public MthdTest {
	bool is_ifm;
	int repeats() override { return 10000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		if (!(rnd() & 3))
			insrt(orig.access, 12, 5, rnd() & 1 ? 0x12 : 0x14);
	}
	void choose_mthd() override {
		if (!(rnd() & 3))
			val &= 0xff00ff;
		if (!(rnd() & 3))
			val &= 0xffff000f;
		switch (rnd() % 3) {
			case 0:
				cls = 0x11;
				mthd = 0x30c;
				is_ifm = false;
				break;
			case 1:
				cls = 0x12;
				mthd = 0x318;
				is_ifm = false;
				break;
			case 2:
				cls = 0x13;
				mthd = 0x30c;
				is_ifm = true;
				break;
			default:
				abort();
		}
	}
	void emulate_mthd() override {
		int rcls = extr(exp.access, 12, 5);
		exp.vtx_y[1] = 0;
		exp.vtx_x[3] = extr(val, 0, 16);
		exp.vtx_y[3] = -extr(val, 16, 16);
		if (rcls >= 0x11 && rcls <= 0x13)
			insrt(exp.xy_misc_1[0], 0, 1, 0);
		if (!is_ifm) {
			if (rcls <= 0xb && rcls >= 9)
				exp.valid[0] &= ~0xffffff;
			if (rcls == 0x10 || (rcls >= 9 && rcls <= 0xc))
				exp.valid[0] |= 0x100;
		}
		exp.valid[0] |= 0x008008;
		if (rcls <= 0xb && rcls >= 9)
			exp.valid[0] |= 0x080080;
		exp.edgefill &= ~0x110;
		if (exp.vtx_x[3] < 0x20 && rcls == 0x12)
			exp.edgefill |= 0x100;
		if (rcls != 0x0d && rcls != 0x1d) {
			insrt(exp.xy_misc_4[0], 28, 2, 0);
			insrt(exp.xy_misc_4[1], 28, 2, 0);
		}
		if (exp.vtx_x[3])
			exp.xy_misc_4[0] |= 2 << 28;
		if (exp.vtx_y[3])
			exp.xy_misc_4[1] |= 2 << 28;
		bool zero;
		if (rcls == 0x14) {
			uint32_t pixels = 4 / nv01_pgraph_cpp_in(exp.ctx_switch[0]);
			zero = (exp.vtx_x[3] == pixels || !exp.vtx_y[3]);
		} else {
			zero = extr(exp.xy_misc_4[0], 28, 2) == 0 ||
				 extr(exp.xy_misc_4[1], 28, 2) == 0;
		}
		insrt(exp.xy_misc_0, 12, 1, zero);
		insrt(exp.xy_misc_0, 28, 4, 0);
	}
public:
	MthdIfcSizeInTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdIfcSizeOutTest : public MthdTest {
	int repeats() override { return 10000; };
	void choose_mthd() override {
		bool is_bitmap = rnd() & 1;
		cls = 0x11 + is_bitmap;
		mthd = is_bitmap ? 0x314 : 0x308;
	}
	void emulate_mthd() override {
		int rcls = extr(exp.access, 12, 5);
		exp.vtx_x[5] = extr(val, 0, 16);
		exp.vtx_y[5] = extr(val, 16, 16);
		if (rcls <= 0xb && rcls >= 9)
			exp.valid[0] &= ~0xffffff;
		exp.valid[0] |= 0x020020;
		insrt(exp.xy_misc_0, 28, 4, 0);
		if (rcls >= 0x11 && rcls <= 0x13)
			insrt(exp.xy_misc_1[0], 0, 1, 0);
		if (rcls == 0x10 || (rcls >= 9 && rcls <= 0xc))
			exp.valid[0] |= 0x100;
	}
public:
	MthdIfcSizeOutTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPitchTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x13 + (rnd() & 1);
		mthd = 0x310;
	}
	void emulate_mthd() override {
		exp.vtx_x[6] = val;
		exp.valid[0] |= 0x040040;
	}
public:
	MthdPitchTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdRectTest : public MthdTest {
	bool draw;
	int repeats() override { return 10000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		if (rnd() & 1)
			orig.valid[0] |= 0x1ff1ff;
		if (rnd() & 1)
			orig.valid[0] |= 0x033033;
		if (rnd() & 1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (rnd() & 1) {
			orig.valid[0] &= ~0x11000000;
			orig.xy_misc_1[0] &= ~0x330;
		}
		if (rnd() & 1) {
			orig.valid[0] &= ~0x00f00f;
		}
	}
	void choose_mthd() override {
		switch (rnd() % 3) {
			case 0:
				cls = 0x10;
				mthd = 0x308;
				draw = true;
				break;
			case 1:
				cls = 0x14;
				mthd = 0x30c;
				draw = false;
				break;
			case 2:
				cls = 0x0c;
				mthd = 0x404 | (rnd() & 0x78);
				draw = true;
				break;
			default:
				abort();
		}
	}
	void emulate_mthd() override {
		int rcls = exp.access >> 12 & 0x1f;
		if (rcls == 0x14) {
			exp.vtx_x[3] = extr(val, 0, 16);
			exp.vtx_y[3] = extr(val, 16, 16);
			nv01_pgraph_vtx_fixup(&exp, 0, 2, exp.vtx_x[3], 1, 0, 2);
			nv01_pgraph_vtx_fixup(&exp, 1, 2, exp.vtx_y[3], 1, 0, 2);
			exp.valid[0] |= 0x4004;
			insrt(exp.xy_misc_0, 12, 1, 0);
			nv01_pgraph_bump_vtxid(&exp);
		} else if (rcls == 0x10) {
			nv01_pgraph_vtx_fixup(&exp, 0, 2, extr(val, 0, 16), 1, 0, 2);
			nv01_pgraph_vtx_fixup(&exp, 1, 2, extr(val, 16, 16), 1, 0, 2);
			nv01_pgraph_vtx_fixup(&exp, 0, 3, extr(val, 0, 16), 1, 1, 3);
			nv01_pgraph_vtx_fixup(&exp, 1, 3, extr(val, 16, 16), 1, 1, 3);
			nv01_pgraph_bump_vtxid(&exp);
			nv01_pgraph_bump_vtxid(&exp);
			exp.valid[0] |= 0x00c00c;
		} else if (rcls == 0x0c || rcls == 0x11 || rcls == 0x12 || rcls == 0x13) {
			int idx = extr(exp.xy_misc_0, 28, 4);
			nv01_pgraph_vtx_fixup(&exp, 0, idx, extr(val, 0, 16), 1, 0, idx & 3);
			nv01_pgraph_vtx_fixup(&exp, 1, idx, extr(val, 16, 16), 1, 0, idx & 3);
			nv01_pgraph_bump_vtxid(&exp);
			if (idx <= 8)
				exp.valid[0] |= 0x1001 << idx;
		} else {
			nv01_pgraph_vtx_fixup(&exp, 0, 15, extr(val, 0, 16), 1, 15, 1);
			nv01_pgraph_vtx_fixup(&exp, 1, 15, extr(val, 16, 16), 1, 15, 1);
			nv01_pgraph_bump_vtxid(&exp);
			if (rcls >= 0x09 && rcls <= 0x0b) {
				exp.valid[0] |= 0x080080;
			}
		}
		if (draw)
			nv01_pgraph_prep_draw(&exp, false);
		// XXX
		if (draw && (rcls == 0x11 || rcls == 0x12 || rcls == 0x13))
			skip = true;
	}
	bool other_fail() override {
		int rcls = extr(exp.access, 12, 5);
		if (real.status && (rcls == 0x0b || rcls == 0x0c)) {
			/* Hung PGRAPH... */
			skip = true;
		}
		return false;
	}
public:
	MthdRectTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdIfcDataTest : public MthdTest {
	bool is_bitmap;
	int repeats() override { return 100000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		if (rnd() & 3)
			orig.valid[0] = 0x1ff1ff;
		if (rnd() & 3) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (rnd() & 3) {
			orig.valid[0] &= ~0x11000000;
			orig.xy_misc_1[0] &= ~0x330;
		}
		for (int j = 0; j < 6; j++) {
			if (rnd() & 1) {
				orig.vtx_x[j] &= 0xff;
				orig.vtx_x[j] -= 0x80;
			}
			if (rnd() & 1) {
				orig.vtx_y[j] &= 0xff;
				orig.vtx_y[j] -= 0x80;
			}
		}
		if (rnd() & 3)
			insrt(orig.access, 12, 5, 0x11 + (rnd() & 1));
	}
	void choose_mthd() override {
		switch (rnd() % 3) {
			case 0:
				cls = 0x11;
				mthd = 0x400 | (rnd() & 0x7c);
				is_bitmap = false;
				break;
			case 1:
				cls = 0x12;
				mthd = 0x400 | (rnd() & 0x7c);
				is_bitmap = true;
				break;
			case 2:
				cls = 0x13;
				mthd = 0x040 | (rnd() & 0x3c);
				is_bitmap = false;
				break;
			default:
				abort();
		}
	}
	void emulate_mthd() override {
		exp.misc32[0] = is_bitmap ? pgraph_expand_mono(&exp, val) : val;
		insrt(exp.xy_misc_1[0], 24, 1, 0);
		int rcls = exp.access >> 12 & 0x1f;
		int steps = 4 / nv01_pgraph_cpp_in(exp.ctx_switch[0]);
		if (rcls == 0x12)
			steps = 0x20;
		if (rcls != 0x11 && rcls != 0x12)
			goto done;
		if (exp.valid[0] & 0x11000000 && exp.ctx_switch[0] & 0x80)
			exp.intr |= 1 << 16;
		if (extr(exp.canvas_config, 24, 1))
			exp.intr |= 1 << 20;
		if (extr(exp.cliprect_ctrl, 8, 1))
			exp.intr |= 1 << 24;
		int iter;
		iter = 0;
		if (extr(exp.xy_misc_0, 12, 1)) {
			if ((exp.valid[0] & 0x38038) != 0x38038)
				exp.intr |= 1 << 16;
			if ((exp.xy_misc_4[0] & 0xf0) || (exp.xy_misc_4[1] & 0xf0))
				exp.intr |= 1 << 12;
			goto done;
		}
		int vidx;
		if (!(exp.xy_misc_1[0] & 1)) {
			exp.vtx_x[6] = exp.vtx_x[4] + exp.vtx_x[5];
			exp.vtx_y[6] = exp.vtx_y[4] + exp.vtx_y[5];
			insrt(exp.xy_misc_1[0], 14, 1, 0);
			insrt(exp.xy_misc_1[0], 18, 1, 0);
			insrt(exp.xy_misc_1[0], 20, 1, 0);
			if ((exp.valid[0] & 0x38038) != 0x38038) {
				exp.intr |= 1 << 16;
				if ((exp.xy_misc_4[0] & 0xf0) || (exp.xy_misc_4[1] & 0xf0))
					exp.intr |= 1 << 12;
				goto done;
			}
			nv01_pgraph_iclip_fixup(&exp, 0, exp.vtx_x[6], 0);
			nv01_pgraph_iclip_fixup(&exp, 1, exp.vtx_y[6], 0);
			insrt(exp.xy_misc_1[0], 0, 1, 1);
			if (extr(exp.edgefill, 8, 1)) {
				/* XXX */
				skip = true;
				return;
			}
			insrt(exp.xy_misc_0, 28, 4, 0);
			vidx = 1;
			exp.vtx_y[2] = exp.vtx_y[3] + 1;
			nv01_pgraph_vtx_cmp(&exp, 1, 2);
			nv01_pgraph_vtx_fixup(&exp, 1, 0, 0, 1, 4, 0);
			nv01_pgraph_vtx_fixup(&exp, 0, 0, 0, 1, 4, 0);
			exp.vtx_x[2] = exp.vtx_x[3];
			exp.vtx_x[2] -= steps;
			nv01_pgraph_vtx_cmp(&exp, 0, 2);
			nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[vidx ^ 1], steps, 0);
			if (extr(exp.xy_misc_4[0], 28, 1)) {
				nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[2], exp.vtx_x[vidx], 0);
			}
			if ((exp.xy_misc_4[0] & 0xc0) || (exp.xy_misc_4[1] & 0xf0))
				exp.intr |= 1 << 12;
			if ((exp.xy_misc_4[0] & 0x30) == 0x30)
				exp.intr |= 1 << 12;
		} else {
			if ((exp.valid[0] & 0x38038) != 0x38038)
				exp.intr |= 1 << 16;
			if ((exp.xy_misc_4[0] & 0xf0) || (exp.xy_misc_4[1] & 0xf0))
				exp.intr |= 1 << 12;
		}
restart:;
		vidx = extr(exp.xy_misc_0, 28, 1);
		if (extr(exp.edgefill, 8, 1)) {
			/* XXX */
			skip = true;
			return;
		}
		if (!exp.intr) {
			if (extr(exp.xy_misc_4[0], 29, 1)) {
				nv01_pgraph_bump_vtxid(&exp);
			} else {
				insrt(exp.xy_misc_0, 28, 4, 0);
				vidx = 1;
				bool check_y = false;
				if (extr(exp.xy_misc_4[1], 28, 1)) {
					exp.vtx_y[2]++;
					nv01_pgraph_vtx_add(&exp, 1, 0, 0, exp.vtx_y[0], exp.vtx_y[1], 1);
					check_y = true;
				} else {
					exp.vtx_x[4] += exp.vtx_x[3];
					exp.vtx_y[2] = exp.vtx_y[3] + 1;
					nv01_pgraph_vtx_fixup(&exp, 1, 0, 0, 1, 4, 0);
				}
				nv01_pgraph_vtx_cmp(&exp, 1, 2);
				nv01_pgraph_vtx_fixup(&exp, 0, 0, 0, 1, 4, 0);
				if (extr(exp.xy_misc_4[0], 28, 1)) {
					nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[vidx ^ 1], ~exp.vtx_x[2], 1);
					exp.vtx_x[2] += exp.vtx_x[3];
					nv01_pgraph_vtx_cmp(&exp, 0, 2);
					if (extr(exp.xy_misc_4[0], 28, 1)) {
						nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[2], exp.vtx_x[vidx], 0);
						if ((exp.xy_misc_4[0] & 0x30) == 0x30)
							exp.intr |= 1 << 12;
						check_y = true;
					} else {
						if ((exp.xy_misc_4[0] & 0x20))
							exp.intr |= 1 << 12;
					}
					if (exp.xy_misc_4[1] & 0x10 && check_y)
						exp.intr |= 1 << 12;
					iter++;
					if (iter > 10000) {
						/* This is a hang - skip this test run.  */
						skip = true;
						return;
					}
					goto restart;
				}
				exp.vtx_x[2] = exp.vtx_x[3];
			}
			exp.vtx_x[2] -= steps;
			nv01_pgraph_vtx_cmp(&exp, 0, 2);
			nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[vidx ^ 1], steps, 0);
			if (extr(exp.xy_misc_4[0], 28, 1)) {
				nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[2], exp.vtx_x[vidx], 0);
			}
		} else {
			nv01_pgraph_bump_vtxid(&exp);
			if (extr(exp.xy_misc_4[0], 29, 1)) {
				exp.vtx_x[2] -= steps;
				nv01_pgraph_vtx_cmp(&exp, 0, 2);
			} else if (extr(exp.xy_misc_4[1], 28, 1)) {
				exp.vtx_y[2]++;
			}
		}
done:
		if (exp.intr)
			exp.access &= ~0x101;
	}
public:
	MthdIfcDataTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class RopSolidTest : public MthdTest {
	int x, y;
	uint32_t addr[2], pixel[2], epixel[2], rpixel[2];
	int repeats() override { return 100000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000400;
		/* XXX bits 8-9 affect rendering */
		/* XXX bits 12-19 affect xy_misc_4 clip status */
		orig.xy_misc_1[0] &= 0xfff00cff;
		/* avoid invalid ops */
		orig.ctx_switch[0] &= ~0x001f;
		if (rnd()&1) {
			int ops[] = {
				0x00, 0x0f,
				0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
				0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x17,
			};
			orig.ctx_switch[0] |= ops[rnd() % ARRAY_SIZE(ops)];
		} else {
			/* BLEND needs more testing */
			int ops[] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c };
			orig.ctx_switch[0] |= ops[rnd() % ARRAY_SIZE(ops)];
			/* XXX Y8 blend? */
			orig.pfb_config |= 0x200;
		}
		orig.pattern_config = rnd()%3; /* shape 3 is a rather ugly hole in Karnough map */
		if (rnd()&1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		/* XXX causes interrupts */
		orig.valid[0] &= ~0x11000000;
		insrt(orig.access, 12, 5, 8);
		insrt(orig.pfb_config, 4, 3, 3);
		x = rnd() & 0x3ff;
		y = rnd() & 0xff;
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 0, 16, x);
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 16, 16, y);
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 0, 16, x);
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 16, 16, y);
		if (rnd()&1) {
			/* it's vanishingly rare for the chroma key to match perfectly by random, so boost the odds */
			uint32_t ckey = pgraph_to_a1r10g10b10(pgraph_expand_color(&orig, orig.misc32[0]));
			ckey ^= (rnd() & 1) << 30; /* perturb alpha */
			if (rnd()&1) {
				/* perturb it a bit to check which bits have to match */
				ckey ^= 1 << (rnd() % 30);
			}
			orig.chroma = ckey;
		}
	}
	void choose_mthd() override {
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			addr[i] = nv01_pgraph_pixel_addr(&orig, x, y, i);
			nva_wr32(cnum, 0x1000000+(addr[i]&~3), rnd());
			pixel[i] = nva_rd32(cnum, 0x1000000+(addr[i]&~3)) >> (addr[i] & 3) * 8;
			pixel[i] &= bflmask(nv01_pgraph_cpp(orig.pfb_config)*8);
		}
		cls = 0x08;
		mthd = 0x400;
		val = y << 16 | x;
	}
	void emulate_mthd() override {
		int bfmt = extr(orig.ctx_switch[0], 9, 4);
		int bufmask = (bfmt / 5 + 1) & 3;
		if (!extr(orig.pfb_config, 12, 1))
			bufmask = 1;
		bool cliprect_pass = pgraph_cliprect_pass(&exp, x, y);
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			if (bufmask & (1 << i) && (cliprect_pass || (i == 1 && extr(exp.canvas_config, 4, 1))))
				epixel[i] = nv01_pgraph_solid_rop(&exp, x, y, pixel[i]);
			else
				epixel[i] = pixel[i];
		}
		exp.vtx_x[0] = x;
		exp.vtx_y[0] = y;
		exp.xy_misc_0 &= ~0xf0000000;
		exp.xy_misc_0 |= 0x10000000;
		exp.xy_misc_1[0] &= ~0x03000001;
		exp.xy_misc_1[0] |= 0x01000000;
		nv01_pgraph_set_xym2(&exp, 0, 0, 0, 0, 0, x == 0x400 ? 8 : x ? 0 : 2);
		nv01_pgraph_set_xym2(&exp, 1, 0, 0, 0, 0, y == 0x400 ? 8 : y ? 0 : 2);
		exp.valid[0] &= ~0xffffff;
		if (extr(exp.cliprect_ctrl, 8, 1)) {
			exp.intr |= 1 << 24;
			exp.access &= ~0x101;
		}
		if (extr(exp.canvas_config, 24, 1)) {
			exp.intr |= 1 << 20;
			exp.access &= ~0x101;
		}
		if (extr(exp.xy_misc_4[0], 4, 4) || extr(exp.xy_misc_4[1], 4, 4)) {
			exp.intr |= 1 << 12;
			exp.access &= ~0x101;
		}
		if (exp.intr & 0x01101000) {
			for (int i = 0; i < 2; i++) {
				epixel[i] = pixel[i];
			}
		}
	}
	bool other_fail() override {
		bool res = false;
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			rpixel[i] = nva_rd32(cnum, 0x1000000+(addr[i]&~3)) >> (addr[i] & 3) * 8;
			rpixel[i] &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
			if (rpixel[i] != epixel[i]) {
			
				printf("Difference in PIXEL[%d]: expected %08x real %08x\n", i, epixel[i], rpixel[i]);
				res = true;
			}
		}
		return res;
	}
	void print_fail() override {
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			printf("%08x %08x %08x PIXEL[%d]\n", pixel[i], epixel[i], rpixel[i], i);
		}
		MthdTest::print_fail();
	}
public:
	RopSolidTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class RopBlitTest : public MthdTest {
	int x, y, sx, sy;
	uint32_t addr[2], saddr[2], pixel[2], epixel[2], rpixel[2], spixel[2];
	int repeats() override { return 100000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000400;
		/* XXX bits 8-9 affect rendering */
		/* XXX bits 12-19 affect xy_misc_4 clip status */
		orig.xy_misc_1[0] &= 0xfff00cff;
		/* avoid invalid ops */
		orig.ctx_switch[0] &= ~0x001f;
		if (rnd()&1) {
			int ops[] = {
				0x00, 0x0f,
				0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
				0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x17,
			};
			orig.ctx_switch[0] |= ops[rnd() % ARRAY_SIZE(ops)];
		} else {
			/* BLEND needs more testing */
			int ops[] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c };
			orig.ctx_switch[0] |= ops[rnd() % ARRAY_SIZE(ops)];
			/* XXX Y8 blend? */
			orig.pfb_config |= 0x200;
		}
		orig.pattern_config = rnd()%3; /* shape 3 is a rather ugly hole in Karnough map */
		if (rnd()&1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		orig.xy_misc_4[0] &= ~0xf000;
		orig.xy_misc_4[1] &= ~0xf000;
		orig.valid[0] &= ~0x11000000;
		orig.valid[0] |= 0xf10f;
		insrt(orig.access, 12, 5, 0x10);
		insrt(orig.pfb_config, 4, 3, 3);
		x = rnd() & 0x1ff;
		y = rnd() & 0xff;
		sx = (rnd() & 0x3ff) + 0x200;
		sy = rnd() & 0xff;
		orig.vtx_x[0] = sx;
		orig.vtx_y[0] = sy;
		orig.vtx_x[1] = x;
		orig.vtx_y[1] = y;
		orig.xy_misc_0 &= ~0xf0000000;
		orig.xy_misc_0 |= 0x20000000;
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 0, 16, x);
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 16, 16, y);
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 0, 16, x);
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 16, 16, y);
		if (rnd()&1) {
			/* it's vanishingly rare for the chroma key to match perfectly by random, so boost the odds */
			uint32_t ckey = pgraph_to_a1r10g10b10(pgraph_expand_color(&orig, orig.misc32[0]));
			ckey ^= (rnd() & 1) << 30; /* perturb alpha */
			if (rnd()&1) {
				/* perturb it a bit to check which bits have to match */
				ckey ^= 1 << (rnd() % 30);
			}
			orig.chroma = ckey;
		}
	}
	void choose_mthd() override {
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			addr[i] = nv01_pgraph_pixel_addr(&orig, x, y, i);
			saddr[i] = nv01_pgraph_pixel_addr(&orig, sx, sy, i);
			nva_wr32(cnum, 0x1000000+(addr[i]&~3), rnd());
			nva_wr32(cnum, 0x1000000+(saddr[i]&~3), rnd());
			pixel[i] = nva_rd32(cnum, 0x1000000+(addr[i]&~3)) >> (addr[i] & 3) * 8;
			spixel[i] = nva_rd32(cnum, 0x1000000+(saddr[i]&~3)) >> (saddr[i] & 3) * 8;
			pixel[i] &= bflmask(nv01_pgraph_cpp(orig.pfb_config)*8);
			spixel[i] &= bflmask(nv01_pgraph_cpp(orig.pfb_config)*8);
			if (sx >= 0x400)
				spixel[i] = 0;
			if (!pgraph_cliprect_pass(&exp, sx, sy) && (i == 0 || !extr(exp.canvas_config, 4, 1))) {
				spixel[i] = 0;
			}
		}
		if (!extr(exp.pfb_config, 12, 1))
			spixel[1] = spixel[0];
		cls = 0x10;
		mthd = 0x308;
		val = 1 << 16 | 1;
	}
	void emulate_mthd() override {
		int bfmt = extr(exp.ctx_switch[0], 9, 4);
		int bufmask = (bfmt / 5 + 1) & 3;
		if (!extr(exp.pfb_config, 12, 1))
			bufmask = 1;
		bool cliprect_pass = pgraph_cliprect_pass(&exp, x, y);
		struct pgraph_color s = nv01_pgraph_expand_surf(&exp, spixel[extr(exp.ctx_switch[0], 13, 1)]);
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			if (bufmask & (1 << i) && (cliprect_pass || (i == 1 && extr(exp.canvas_config, 4, 1))))
				epixel[i] = nv01_pgraph_rop(&exp, x, y, pixel[i], s);
			else
				epixel[i] = pixel[i];
		}
		nv01_pgraph_vtx_fixup(&exp, 0, 2, 1, 1, 0, 2);
		nv01_pgraph_vtx_fixup(&exp, 1, 2, 1, 1, 0, 2);
		nv01_pgraph_vtx_fixup(&exp, 0, 3, 1, 1, 1, 3);
		nv01_pgraph_vtx_fixup(&exp, 1, 3, 1, 1, 1, 3);
		exp.xy_misc_0 &= ~0xf0000000;
		exp.xy_misc_0 |= 0x00000000;
		exp.valid[0] &= ~0xffffff;
		if (extr(exp.cliprect_ctrl, 8, 1)) {
			exp.intr |= 1 << 24;
			exp.access &= ~0x101;
		}
		if (extr(exp.canvas_config, 24, 1)) {
			exp.intr |= 1 << 20;
			exp.access &= ~0x101;
		}
		if (extr(exp.xy_misc_4[0], 4, 4) || extr(exp.xy_misc_4[1], 4, 4)) {
			exp.intr |= 1 << 12;
			exp.access &= ~0x101;
		}
		if (exp.intr & 0x01101000) {
			for (int i = 0; i < 2; i++) {
				epixel[i] = pixel[i];
			}
		}
	}
	bool other_fail() override {
		bool res = false;
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			rpixel[i] = nva_rd32(cnum, 0x1000000+(addr[i]&~3)) >> (addr[i] & 3) * 8;
			rpixel[i] &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
			if (rpixel[i] != epixel[i]) {
			
				printf("Difference in PIXEL[%d]: expected %08x real %08x\n", i, epixel[i], rpixel[i]);
				res = true;
			}
		}
		return res;
	}
	void print_fail() override {
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			printf("%08x %08x %08x PIXEL[%d]\n", pixel[i], epixel[i], rpixel[i], i);
		}
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			printf("%08x SPIXEL[%d]\n", spixel[i], i);
		}
		MthdTest::print_fail();
	}
public:
	RopBlitTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class ScanTests : public hwtest::Test {
public:
	std::vector<std::pair<const char *, Test *>> subtests() override {
		return {
			{"access", new ScanAccessTest(opt, rnd())},
			{"debug", new ScanDebugTest(opt, rnd())},
			{"control", new ScanControlTest(opt, rnd())},
			{"canvas", new ScanCanvasTest(opt, rnd())},
			{"vtx", new ScanVtxTest(opt, rnd())},
			{"clip", new ScanClipTest(opt, rnd())},
			{"context", new ScanContextTest(opt, rnd())},
			{"vstate", new ScanVStateTest(opt, rnd())},
		};
	}
	ScanTests(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class StateTests : public hwtest::Test {
public:
	std::vector<std::pair<const char *, Test *>> subtests() override {
		return {
			{"state", new StateTest(opt, rnd())},
			{"soft_reset", new SoftResetTest(opt, rnd())},
			{"mmio_read", new MMIOReadTest(opt, rnd())},
			{"mmio_write", new MMIOWriteTest(opt, rnd())},
			{"mmio_vtx_write", new VtxWriteTest(opt, rnd())},
			{"mmio_iclip_write", new IClipWriteTest(opt, rnd())},
			{"mmio_uclip_write", new UClipWriteTest(opt, rnd())},
			{"clip_status", new ClipStatusTest(opt, rnd())},
		};
	}
	StateTests(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class MthdMiscTests : public hwtest::Test {
public:
	std::vector<std::pair<const char *, Test *>> subtests() override {
		return {
			{"ctx_switch", new MthdCtxSwitchTest(opt, rnd())},
			{"notify", new MthdNotifyTest(opt, rnd())},
			{"invalid", new MthdInvalidTests(opt, rnd())},
		};
	}
	MthdMiscTests(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class MthdSimpleTests : public hwtest::Test {
public:
	std::vector<std::pair<const char *, Test *>> subtests() override {
		return {
			{"beta", new MthdBetaTest(opt, rnd())},
			{"rop", new MthdRopTest(opt, rnd())},
			{"chroma_plane", new MthdChromaPlaneTest(opt, rnd())},
			{"pattern_shape", new MthdPatternShapeTest(opt, rnd())},
			{"pattern_mono_color", new MthdPatternMonoColorTest(opt, rnd())},
			{"pattern_mono_bitmap", new MthdPatternMonoBitmapTest(opt, rnd())},
			{"solid_color", new MthdSolidColorTest(opt, rnd())},
			{"bitmap_color", new MthdBitmapColorTest(opt, rnd())},
			{"subdivide", new MthdSubdivideTest(opt, rnd())},
			{"vtx_beta", new MthdVtxBetaTest(opt, rnd())},
		};
	}
	MthdSimpleTests(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class MthdXyTests : public hwtest::Test {
public:
	std::vector<std::pair<const char *, Test *>> subtests() override {
		return {
			{"clip", new MthdClipTest(opt, rnd())},
			{"vtx", new MthdVtxTest(opt, rnd())},
			{"vtx_x32", new MthdX32Test(opt, rnd())},
			{"vtx_y32", new MthdY32Test(opt, rnd())},
			{"ifc_size_in", new MthdIfcSizeInTest(opt, rnd())},
			{"ifc_size_out", new MthdIfcSizeOutTest(opt, rnd())},
			{"pitch", new MthdPitchTest(opt, rnd())},
			{"rect", new MthdRectTest(opt, rnd())},
			{"ifc_data", new MthdIfcDataTest(opt, rnd())},
		};
	}
	MthdXyTests(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class RopTests : public hwtest::Test {
public:
	std::vector<std::pair<const char *, Test *>> subtests() override {
		return {
			{"solid", new RopSolidTest(opt, rnd())},
			{"blit", new RopBlitTest(opt, rnd())},
		};
	}
	RopTests(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class NV1PGraphTests : public hwtest::Test {
public:
	int run() override {
		if (chipset.card_type != 1)
			return HWTEST_RES_NA;
		if (!(nva_rd32(cnum, 0x200) & 1 << 24)) {
			printf("Mem controller not up.\n");
			return HWTEST_RES_UNPREP;
		}
		nva_wr32(cnum, 0x200, 0xffffffff);
		return HWTEST_RES_PASS;
	}
	std::vector<std::pair<const char *, Test *>> subtests() override {
		return {
			{"scan", new ScanTests(opt, rnd())},
			{"state", new StateTests(opt, rnd())},
			{"mthd_misc", new MthdMiscTests(opt, rnd())},
			{"mthd_simple", new MthdSimpleTests(opt, rnd())},
			{"mthd_xy", new MthdXyTests(opt, rnd())},
			{"rop", new RopTests(opt, rnd())},
		};
	}
	NV1PGraphTests(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

}

hwtest::Test *nv01_pgraph_test(hwtest::TestOptions &opt, uint32_t seed) {
	return new NV1PGraphTests(opt, seed);
}
