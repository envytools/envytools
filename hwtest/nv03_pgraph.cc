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

static const uint32_t nv03_pgraph_state_regs[] = {
	/* INTR, INVALID */
	0x400100, 0x400104, 0x401100,
	/* INTR_EN, INVALID_EN */
	0x400140, 0x400144, 0x401140,
	/* CTX_* */
	0x400180, 0x400190, 0x400194,
	0x4001a0, 0x4001a4, 0x4001a8, 0x4001ac,
	0x4001b0, 0x4001b4, 0x4001b8, 0x4001bc,
	/* ICLIP */
	0x400534, 0x400538,
	/* UCLIP */
	0x40053c, 0x400540, 0x400544, 0x400548,
	/* OCLIP */
	0x400560, 0x400564, 0x400568, 0x40056c,
	/* VTX_X */
	0x400400, 0x400404, 0x400408, 0x40040c,
	0x400410, 0x400414, 0x400418, 0x40041c,
	0x400420, 0x400424, 0x400428, 0x40042c,
	0x400430, 0x400434, 0x400438, 0x40043c,
	0x400440, 0x400444, 0x400448, 0x40044c,
	0x400450, 0x400454, 0x400458, 0x40045c,
	0x400460, 0x400464, 0x400468, 0x40046c,
	0x400470, 0x400474, 0x400478, 0x40047c,
	/* VTX_Y */
	0x400480, 0x400484, 0x400488, 0x40048c,
	0x400490, 0x400494, 0x400498, 0x40049c,
	0x4004a0, 0x4004a4, 0x4004a8, 0x4004ac,
	0x4004b0, 0x4004b4, 0x4004b8, 0x4004bc,
	0x4004c0, 0x4004c4, 0x4004c8, 0x4004cc,
	0x4004d0, 0x4004d4, 0x4004d8, 0x4004dc,
	0x4004e0, 0x4004e4, 0x4004e8, 0x4004ec,
	0x4004f0, 0x4004f4, 0x4004f8, 0x4004fc,
	/* VTX_Z */
	0x400580, 0x400584, 0x400588, 0x40058c,
	0x400590, 0x400594, 0x400598, 0x40059c,
	0x4005a0, 0x4005a4, 0x4005a8, 0x4005ac,
	0x4005b0, 0x4005b4, 0x4005b8, 0x4005bc,
	/* PATTERN_RGB, _A */
	0x400600, 0x400608, 0x400604, 0x40060c,
	/* PATTERN_MONO, _SHAPE */
	0x400610, 0x400614, 0x400618,
	/* BITMAP_COLOR */
	0x40061c,
	/* ROP, PLANE, CHROMA, BETA */
	0x400624, 0x40062c, 0x400640,
	/* CANVAS */
	0x400550, 0x400554, 0x400558, 0x40055c,
	/* CLIPRECT */
	0x400690, 0x400698, 0x400694, 0x40069c,
	/* CLIPRECT_CTRL */
	0x4006a0,
	/* XY_MISC */
	0x400514, 0x400518, 0x40051c, 0x400520,
	0x400500, 0x400504,
	/* XY_CLIP */
	0x400524, 0x400528, 0x40052c, 0x400530,
	/* VALID, MISC32_*, MISC24_* */
	0x400508, 0x40050c, 0x40054c,
	0x400510, 0x400570,
	/* D3D_* */
	0x4005c0, 0x4005c4, 0x4005c8, 0x4005cc,
	0x4005d0, 0x4005d4, 0x400644, 0x4006c8,
	/* DMA, NOTIFY, GROBJ, M2MF */
	0x400680, 0x400684, 0x400688, 0x40068c,
	/* SURF_OFFSET */
	0x400630, 0x400634, 0x400638, 0x40063c,
	/* SURF_PITCH */
	0x400650, 0x400654, 0x400658, 0x40065c,
	/* SURF_FORMAT */
	0x4006a8,
	/* DMA */
	0x401210, 0x401220, 0x401230, 0x401240,
	0x401250, 0x401260, 0x401270, 0x401280,
	0x401290, 0x401400,
	0x401800, 0x401810, 0x401820, 0x401830,
	0x401840,
	/* ACCESS */
	0x4006a4,
	/* DEBUG */
	0x400080, 0x400084, 0x400088, 0x40008c,
	/* STATUS */
	0x4006b0,
};

static const char *const nv03_pgraph_state_reg_names[] = {
	"INTR", "INVALID", "DMA_INTR",
	"INTR_EN", "INVALID_EN", "DMA_INTR_EN",
	"CTX_SWITCH", "CTX_CONTROL", "CTX_USER",
	"CTX_CACHE[0]", "CTX_CACHE[1]", "CTX_CACHE[2]", "CTX_CACHE[3]",
	"CTX_CACHE[4]", "CTX_CACHE[5]", "CTX_CACHE[6]", "CTX_CACHE[7]",
	"ICLIP.X", "ICLIP.Y",
	"UCLIP_MIN.X", "UCLIP_MIN.Y",
	"UCLIP_MAX.X", "UCLIP_MAX.Y",
	"OCLIP_MIN.X", "OCLIP_MIN.Y",
	"OCLIP_MAX.X", "OCLIP_MAX.Y",
	"VTX[0].X", "VTX[1].X", "VTX[2].X", "VTX[3].X",
	"VTX[4].X", "VTX[5].X", "VTX[6].X", "VTX[7].X",
	"VTX[8].X", "VTX[9].X", "VTX[10].X", "VTX[11].X",
	"VTX[12].X", "VTX[13].X", "VTX[14].X", "VTX[15].X",
	"VTX[16].X", "VTX[17].X", "VTX[18].X", "VTX[19].X",
	"VTX[20].X", "VTX[21].X", "VTX[22].X", "VTX[23].X",
	"VTX[24].X", "VTX[25].X", "VTX[26].X", "VTX[27].X",
	"VTX[28].X", "VTX[29].X", "VTX[30].X", "VTX[31].X",
	"VTX[0].Y", "VTX[1].Y", "VTX[2].Y", "VTX[3].Y",
	"VTX[4].Y", "VTX[5].Y", "VTX[6].Y", "VTX[7].Y",
	"VTX[8].Y", "VTX[9].Y", "VTX[10].Y", "VTX[11].Y",
	"VTX[12].Y", "VTX[13].Y", "VTX[14].Y", "VTX[15].Y",
	"VTX[16].Y", "VTX[17].Y", "VTX[18].Y", "VTX[19].Y",
	"VTX[20].Y", "VTX[21].Y", "VTX[22].Y", "VTX[23].Y",
	"VTX[24].Y", "VTX[25].Y", "VTX[26].Y", "VTX[27].Y",
	"VTX[28].Y", "VTX[29].Y", "VTX[30].Y", "VTX[31].Y",
	"VTX[0].Z", "VTX[1].Z", "VTX[2].Z", "VTX[3].Z",
	"VTX[4].Z", "VTX[5].Z", "VTX[6].Z", "VTX[7].Z",
	"VTX[8].Z", "VTX[9].Z", "VTX[10].Z", "VTX[11].Z",
	"VTX[12].Z", "VTX[13].Z", "VTX[14].Z", "VTX[15].Z",
	"PATTERN_COLOR[0].RGB", "PATTERN_COLOR[1].RGB",
	"PATTERN_COLOR[0].A", "PATTERN_COLOR[1].A",
	"PATTERN_BITMAP[0]", "PATTERN_BITMAP[1]", "PATTERN_SHAPE",
	"BITMAP_COLOR[0]", "ROP", "CHROMA", "BETA",
	"SRC_CANVAS_MIN", "SRC_CANVAS_MAX",
	"DST_CANVAS_MIN", "DST_CANVAS_MAX",
	"CLIPRECT[0].MIN", "CLIPRECT[1].MIN", "CLIPRECT[0].MAX", "CLIPRECT[1].MAX",
	"CLIPRECT_CTRL",
	"XY_MISC_0", "XY_MISC_1", "XY_MISC_2", "XY_MISC_3",
	"X_MISC", "Y_MISC",
	"X_CLIP[0]", "X_CLIP[1]", "Y_CLIP[0]", "Y_CLIP[1]",
	"VALID", "MISC32_0", "MISC32_1", "MISC24_0", "MISC24_1",
	"D3D_XY", "D3D_UV", "D3D_Z", "D3D_RGB",
	"D3D_FOG_TRI", "D3D_M", "D3D_CONFIG", "D3D_ALPHA",
	"DMA", "NOTIFY", "GROBJ", "M2MF",
	"SURF_OFFSET[0]", "SURF_OFFSET[1]", "SURF_OFFSET[2]", "SURF_OFFSET[3]",
	"SURF_PITCH[0]", "SURF_PITCH[1]", "SURF_PITCH[2]", "SURF_PITCH[3]",
	"SURF_FORMAT",
	"DMA_FLAGS", "DMA_LIMIT", "DMA_PTE", "DMA_PTE_TAG",
	"DMA_ADDR_VIRT_ADJ", "DMA_ADDR_PHYS", "DMA_BYTES",
	"DMA_INST", "DMA_LINES", "DMA_LIN_LIMIT",
	"DMA_OFFSET[0]", "DMA_OFFSET[1]", "DMA_OFFSET[2]",
	"DMA_PITCH", "DMA_FORMAT",
	"ACCESS",
	"DEBUG[0]", "DEBUG[1]", "DEBUG[2]", "DEBUG[3]",
	"STATUS",
};

static_assert(sizeof (nv03_pgraph_state) == sizeof nv03_pgraph_state_regs);
static_assert(ARRAY_SIZE(nv03_pgraph_state_regs) == ARRAY_SIZE(nv03_pgraph_state_reg_names));

static void nv03_pgraph_gen_state(struct hwtest_ctx *ctx, struct nv03_pgraph_state *state) {
	unsigned i;
	uint32_t *rawstate = (uint32_t *)state;
	for (i = 0; i < ARRAY_SIZE(nv03_pgraph_state_regs); i++)
		rawstate[i] = jrand48(ctx->rand48);
	state->intr = 0;
	state->invalid = 0;
	state->dma_intr = 0;
	state->intr_en &= 0x11111011;
	state->invalid_en &= 0x00011111;
	state->dma_intr_en &= 0x00011111;
	state->ctx_switch &= 0x3ff3f71f;
	state->ctx_control &= 0x11010103;
	state->ctx_user &= 0x7f1fe000;
	for (i = 0; i < 8; i++)
		state->ctx_cache[i] &= 0x3ff3f71f;
	state->iclip[0] &= 0x3ffff;
	state->iclip[1] &= 0x3ffff;
	state->uclip_min[0] &= 0x3ffff;
	state->uclip_max[0] &= 0x3ffff;
	state->uclip_min[1] &= 0x3ffff;
	state->uclip_max[1] &= 0x3ffff;
	state->oclip_min[0] &= 0x3ffff;
	state->oclip_max[0] &= 0x3ffff;
	state->oclip_min[1] &= 0x3ffff;
	state->oclip_max[1] &= 0x3ffff;
	for (i = 0; i < 16; i++)
		state->vtx_z[i] &= 0xffffff;
	for (i = 0; i < 2; i++) {
		state->pattern_rgb[i] &= 0x3fffffff;
		state->pattern_a[i] &= 0xff;
		state->pattern_bitmap[i] &= 0xffffffff;
	}
	for (i = 0; i < 4; i++) {
		state->surf_offset[i] &= 0x003ffff0;
		state->surf_pitch[i] &= 0x00001ff0;
	}
	state->surf_format &= 0x7777;
	state->src_canvas_min &= 0x3fff07ff;
	state->src_canvas_max &= 0x3fff07ff;
	state->dst_canvas_min &= 0x3fff07ff;
	state->dst_canvas_max &= 0x3fff07ff;
	state->dma &= 0xffff;
	state->notify &= 0xf1ffff;
	state->grobj &= 0xffff;
	state->m2mf &= 0x1ffff;
	state->cliprect_min[0] &= 0x3fff07ff;
	state->cliprect_min[1] &= 0x3fff07ff;
	state->cliprect_max[0] &= 0x3fff07ff;
	state->cliprect_max[1] &= 0x3fff07ff;
	state->cliprect_ctrl &= 0x113;
	state->pattern_shape &= 3;
	state->rop &= 0xff;
	state->chroma &= 0x7fffffff;
	state->beta &= 0x7f800000;
	state->bitmap_color_0 &= 0x7fffffff;
	state->d3d_z &= 0xffff;
	state->d3d_config &= 0xf77fbdf3;
	state->d3d_alpha &= 0xfff;
	state->misc24_0 &= 0xffffff;
	state->misc24_1 &= 0xffffff;
	state->xy_misc_0 &= 0xf013ffff;
	state->xy_misc_1[0] &= 0x0f177331;
	state->xy_misc_1[1] &= 0x0f177331;
	state->xy_misc_3 &= 0x7f7f1111;
	state->xy_misc_4[0] &= 0x300000ff;
	state->xy_misc_4[1] &= 0x300000ff;
	state->dma_flags &= 0x03010fff;
	state->dma_pte &= 0xfffff003;
	state->dma_pte_tag &= 0xfffff000;
	state->dma_bytes &= 0x003fffff;
	state->dma_inst &= 0x0000ffff;
	state->dma_lines &= 0x000007ff;
	state->dma_lin_limit &= 0x003fffff;
	state->dma_format &= 0x707;
	state->debug[0] &= 0x13311110;
	state->debug[1] &= 0x10113301;
	state->debug[2] &= 0x1133f111;
	state->debug[3] &= 0x1173ff31;
	state->access = 1;
	state->status = 0;
}

static void nv03_pgraph_load_state(struct hwtest_ctx *ctx, struct nv03_pgraph_state *state) {
	unsigned i;
	uint32_t *rawstate = (uint32_t*)state;
	nva_wr32(ctx->cnum, 0x000200, 0xffffefff);
	nva_wr32(ctx->cnum, 0x000200, 0xffffffff);
	nva_wr32(ctx->cnum, 0x4006a4, 0);
	nva_wr32(ctx->cnum, 0x400100, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400104, 0xffffffff);
	nva_wr32(ctx->cnum, 0x401100, 0xffffffff);
	for (i = 0; i < ARRAY_SIZE(nv03_pgraph_state_regs); i++)
		nva_wr32(ctx->cnum, nv03_pgraph_state_regs[i], rawstate[i]);
	nva_wr32(ctx->cnum, 0x400508, state->valid);
}

static void nv03_pgraph_dump_state(struct hwtest_ctx *ctx, struct nv03_pgraph_state *state) {
	unsigned i;
	uint32_t *rawstate = (uint32_t*)state;
	int ctr = 0;
	uint32_t status;
	while((status = nva_rd32(ctx->cnum, 0x4006b0))) {
		ctr++;
		if (ctr > 100000) {
			fprintf(stderr, "PGRAPH locked up [%08x]!\n", status);
			uint32_t save_intr_en = nva_rd32(ctx->cnum, 0x400140);
			uint32_t save_invalid_en = nva_rd32(ctx->cnum, 0x400144);
			uint32_t save_ctx_ctrl = nva_rd32(ctx->cnum, 0x400190);
			uint32_t save_access = nva_rd32(ctx->cnum, 0x4006a4);
			nva_wr32(ctx->cnum, 0x000200, 0xffffefff);
			nva_wr32(ctx->cnum, 0x000200, 0xffffffff);
			nva_wr32(ctx->cnum, 0x400140, save_intr_en);
			nva_wr32(ctx->cnum, 0x400144, save_invalid_en);
			nva_wr32(ctx->cnum, 0x400190, save_ctx_ctrl);
			nva_wr32(ctx->cnum, 0x4006a4, save_access);
			break;
		}
	}
	uint32_t access = nva_rd32(ctx->cnum, 0x4006a4);
	nva_wr32(ctx->cnum, 0x4006a4, 0);
	for (i = 0; i < ARRAY_SIZE(nv03_pgraph_state_regs); i++)
		rawstate[i] = nva_rd32(ctx->cnum, nv03_pgraph_state_regs[i]);
	state->intr &= ~0x100;
	state->ctx_control &= ~0x00100000;
	state->access = access;
	state->status = status;
}

static int nv03_pgraph_cmp_state(struct nv03_pgraph_state *exp, struct nv03_pgraph_state *real) {
	unsigned i;
	uint32_t *rawexp = (uint32_t*)exp;
	uint32_t *rawreal = (uint32_t*)real;
	int res = 0;
	for (i = 0; i < ARRAY_SIZE(nv03_pgraph_state_regs); i++) {
		if (rawexp[i] != rawreal[i]) {
			printf("Difference in reg %06x: expected %08x real %08x\n", nv03_pgraph_state_regs[i], rawexp[i], rawreal[i]);
			res = 1;
		}
	}
	return res;
}

static void nv03_pgraph_print_states(struct nv03_pgraph_state *orig, struct nv03_pgraph_state *exp, struct nv03_pgraph_state *real) {
	unsigned i;
	uint32_t *raworig = (uint32_t*)orig;
	uint32_t *rawexp = (uint32_t*)exp;
	uint32_t *rawreal = (uint32_t*)real;
	for (i = 0; i < ARRAY_SIZE(nv03_pgraph_state_regs); i++)
		printf("%06x: %08x %08x %08x %c [%s]\n", nv03_pgraph_state_regs[i],
			raworig[i],
			rawexp[i],
			rawreal[i],
			rawexp[i] == rawreal[i] ? ' ' : '*',
			nv03_pgraph_state_reg_names[i]
		);
}

static int test_scan_debug(struct hwtest_ctx *ctx) {
	TEST_BITSCAN(0x400080, 0x13311110, 0);
	TEST_BITSCAN(0x400084, 0x10113301, 0);
	TEST_BITSCAN(0x400088, 0x1133f111, 0);
	TEST_BITSCAN(0x40008c, 0x1173ff31, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_control(struct hwtest_ctx *ctx) {
	TEST_BITSCAN(0x400140, 0x11111111, 0);
	TEST_BITSCAN(0x400144, 0x00011111, 0);
	TEST_BITSCAN(0x401140, 0x00011111, 0);
	TEST_BITSCAN(0x400180, 0x3ff3f71f, 0);
	TEST_BITSCAN(0x400190, 0x11010103, 0);
	TEST_BITSCAN(0x400194, 0x7f1fe000, 0);
	TEST_BITSCAN(0x4006a4, 0x1, 0);
	nva_wr32(ctx->cnum, 0x4006a4, 0);
	int i;
	for (i = 0 ; i < 8; i++)
		TEST_BITSCAN(0x4001a0 + i * 4, 0x3ff3f71f, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_canvas(struct hwtest_ctx *ctx) {
	TEST_BITSCAN(0x400550, 0x3fff07ff, 0);
	TEST_BITSCAN(0x400554, 0x3fff07ff, 0);
	TEST_BITSCAN(0x400558, 0x3fff07ff, 0);
	TEST_BITSCAN(0x40055c, 0x3fff07ff, 0);
	TEST_BITSCAN(0x400690, 0x3fff07ff, 0);
	TEST_BITSCAN(0x400694, 0x3fff07ff, 0);
	TEST_BITSCAN(0x400698, 0x3fff07ff, 0);
	TEST_BITSCAN(0x40069c, 0x3fff07ff, 0);
	TEST_BITSCAN(0x4006a0, 0x113, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_vtx(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0 ; i < 32; i++) {
		TEST_BITSCAN(0x400400 + i * 4, 0xffffffff, 0);
		TEST_BITSCAN(0x400480 + i * 4, 0xffffffff, 0);
	}
	for (i = 0 ; i < 16; i++) {
		TEST_BITSCAN(0x400580 + i * 4, 0x00ffffff, 0);
	}
	return HWTEST_RES_PASS;
}

static int test_scan_d3d(struct hwtest_ctx *ctx) {
	TEST_BITSCAN(0x4005c0, 0xffffffff, 0);
	TEST_BITSCAN(0x4005c4, 0xffffffff, 0);
	TEST_BITSCAN(0x4005c8, 0x0000ffff, 0);
	TEST_BITSCAN(0x4005cc, 0xffffffff, 0);
	TEST_BITSCAN(0x4005d0, 0xffffffff, 0);
	TEST_BITSCAN(0x4005d4, 0xffffffff, 0);
	TEST_BITSCAN(0x400644, 0xf77fbdf3, 0);
	TEST_BITSCAN(0x4006c8, 0x00000fff, 0);
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
		v0 &= 0x3ffff;
		v1 &= 0x3ffff;
		TEST_READ(0x40053c + idx * 4, v0, "v0 %08x v1 %08x", v0, v1);
		TEST_READ(0x400544 + idx * 4, v1, "v0 %08x v1 %08x", v0, v1);
		if (jrand48(ctx->rand48) & 1) {
			nva_wr32(ctx->cnum, 0x40053c + idx * 4, v2);
		} else {
			nva_wr32(ctx->cnum, 0x400544 + idx * 4, v2);
		}
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
		v0 &= 0x3ffff;
		v1 &= 0x3ffff;
		TEST_READ(0x400560 + idx * 4, v0, "v0 %08x v1 %08x", v0, v1);
		TEST_READ(0x400568 + idx * 4, v1, "v0 %08x v1 %08x", v0, v1);
		if (jrand48(ctx->rand48) & 1) {
			nva_wr32(ctx->cnum, 0x400560 + idx * 4, v2);
		} else {
			nva_wr32(ctx->cnum, 0x400568 + idx * 4, v2);
		}
		v2 &= 0x3ffff;
		TEST_READ(0x400560 + idx * 4, v1, "v0 %08x v1 %08x v2 %08x", v0, v1, v2);
		TEST_READ(0x400568 + idx * 4, v2, "v0 %08x v1 %08x v2 %08x", v0, v1, v2);
	}
	return HWTEST_RES_PASS;
}

static int test_scan_context(struct hwtest_ctx *ctx) {
	TEST_BITSCAN(0x400600, 0x3fffffff, 0);
	TEST_BITSCAN(0x400604, 0xff, 0);
	TEST_BITSCAN(0x400608, 0x3fffffff, 0);
	TEST_BITSCAN(0x40060c, 0xff, 0);
	TEST_BITSCAN(0x400610, 0xffffffff, 0);
	TEST_BITSCAN(0x400614, 0xffffffff, 0);
	TEST_BITSCAN(0x400618, 3, 0);
	TEST_BITSCAN(0x40061c, 0x7fffffff, 0);
	TEST_BITSCAN(0x400624, 0xff, 0);
	// XXX 628?
	TEST_BITSCAN(0x40062c, 0x7fffffff, 0);
	TEST_BITSCAN(0x400680, 0x0000ffff, 0);
	TEST_BITSCAN(0x400684, 0x00f1ffff, 0);
	TEST_BITSCAN(0x400688, 0x0000ffff, 0);
	TEST_BITSCAN(0x40068c, 0x0001ffff, 0);
	TEST_BITSCAN(0x400630, 0x003ffff0, 0);
	TEST_BITSCAN(0x400634, 0x003ffff0, 0);
	TEST_BITSCAN(0x400638, 0x003ffff0, 0);
	TEST_BITSCAN(0x40063c, 0x003ffff0, 0);
	TEST_BITSCAN(0x400650, 0x00001ff0, 0);
	TEST_BITSCAN(0x400654, 0x00001ff0, 0);
	TEST_BITSCAN(0x400658, 0x00001ff0, 0);
	TEST_BITSCAN(0x40065c, 0x00001ff0, 0);
	TEST_BITSCAN(0x4006a8, 0x00007777, 0);
	int i;
	for (i = 0; i < 1000; i++) {
		uint32_t orig = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, 0x400640, orig);
		uint32_t exp = orig & 0x7f800000;
		if (orig & 0x80000000)
			exp = 0;
		uint32_t real = nva_rd32(ctx->cnum, 0x400640);
		if (real != exp) {
			printf("BETA scan mismatch: orig %08x expected %08x real %08x\n", orig, exp, real);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_scan_vstate(struct hwtest_ctx *ctx) {
	TEST_BITSCAN(0x400500, 0x300000ff, 0);
	TEST_BITSCAN(0x400504, 0x300000ff, 0);
	TEST_BITSCAN(0x400508, 0xffffffff, 0);
	TEST_BITSCAN(0x40050c, 0xffffffff, 0);
	TEST_BITSCAN(0x400510, 0x00ffffff, 0);
	TEST_BITSCAN(0x400514, 0xf013ffff, 0);
	TEST_BITSCAN(0x400518, 0x0f177331, 0);
	TEST_BITSCAN(0x40051c, 0x0f177331, 0);
	TEST_BITSCAN(0x400520, 0x7f7f1111, 0);
	TEST_BITSCAN(0x400524, 0xffffffff, 0);
	TEST_BITSCAN(0x400528, 0xffffffff, 0);
	TEST_BITSCAN(0x40052c, 0xffffffff, 0);
	TEST_BITSCAN(0x400530, 0xffffffff, 0);
	TEST_BITSCAN(0x40054c, 0xffffffff, 0);
	TEST_BITSCAN(0x400570, 0x00ffffff, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_dma(struct hwtest_ctx *ctx) {
	TEST_BITSCAN(0x401200, 0x00000000, 0);
	TEST_BITSCAN(0x401210, 0x03010fff, 0);
	TEST_BITSCAN(0x401220, 0xffffffff, 0);
	TEST_BITSCAN(0x401230, 0xfffff003, 0);
	TEST_BITSCAN(0x401240, 0xfffff000, 0);
	TEST_BITSCAN(0x401250, 0xffffffff, 0);
	TEST_BITSCAN(0x401260, 0xffffffff, 0);
	TEST_BITSCAN(0x401270, 0x003fffff, 0);
	TEST_BITSCAN(0x401280, 0x0000ffff, 0);
	TEST_BITSCAN(0x401290, 0x000007ff, 0);
	TEST_BITSCAN(0x401400, 0x003fffff, 0);
	TEST_BITSCAN(0x401800, 0xffffffff, 0);
	TEST_BITSCAN(0x401810, 0xffffffff, 0);
	TEST_BITSCAN(0x401820, 0xffffffff, 0);
	TEST_BITSCAN(0x401830, 0xffffffff, 0);
	TEST_BITSCAN(0x401840, 0x00000707, 0);
	return HWTEST_RES_PASS;
}

static int test_state(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 1000; i++) {
		struct nv03_pgraph_state orig, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_load_state(ctx, &orig);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &real)) {
			nv03_pgraph_print_states(&orig, &orig, &real);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_soft_reset(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x400080, exp.debug[0] | 1);
		exp.bitmap_color_0 &= 0x3fffffff;
		exp.dma_intr_en = 0;
		exp.xy_misc_0 &= 0x100000;
		exp.xy_misc_1[0] &= 0x0f000000;
		exp.xy_misc_1[1] &= 0x0f000000;
		exp.xy_misc_3 &= ~0x00001100;
		exp.xy_misc_4[0] &= 0x30000000;
		exp.xy_misc_4[1] &= 0x30000000;
		exp.surf_offset[0] = 0;
		exp.surf_offset[1] = 0;
		exp.surf_offset[2] = 0;
		exp.surf_offset[3] = 0;
		exp.d3d_config = 0;
		exp.misc24_0 = 0;
		exp.misc24_1 = 0;
		exp.valid = 0;
		exp.xy_clip[0][0] = 0x55555555;
		exp.xy_clip[0][1] = 0x55555555;
		exp.xy_clip[1][0] = 0x55555555;
		exp.xy_clip[1][1] = 0x55555555;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			printf("Iteration %d\n", i);
			nv03_pgraph_print_states(&orig, &exp, &real);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_read(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		struct nv03_pgraph_state exp, real;
		nv03_pgraph_gen_state(ctx, &exp);
		int idx = nrand48(ctx->rand48) % ARRAY_SIZE(nv03_pgraph_state_regs);
		uint32_t reg = nv03_pgraph_state_regs[idx];
		nv03_pgraph_load_state(ctx, &exp);
		nva_rd32(ctx->cnum, reg);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			printf("After reading %08x\n", reg);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		exp = orig;
		int idx = nrand48(ctx->rand48) % ARRAY_SIZE(nv03_pgraph_state_regs);
		uint32_t reg = nv03_pgraph_state_regs[idx];
		uint32_t val = ((uint32_t*)&exp)[idx];
		nv03_pgraph_load_state(ctx, &exp);
		nva_wr32(ctx->cnum, reg, val);
		if (reg == 0x400180) {
			exp.debug[1] &= ~1;
		}
		if ((reg & ~0xff) == 0x400400) {
			int xy = reg >> 7 & 1;
			nv03_pgraph_vtx_fixup(&exp, xy, 8, val);
		}
		if (reg >= 0x400534 && reg <= 0x400538) {
			int xy = (reg - 4) >> 2 & 1;
			insrt(exp.xy_misc_1[0], 14, 1, 0);
			insrt(exp.xy_misc_1[0], 18, 1, 0);
			insrt(exp.xy_misc_1[0], 20, 1, 0);
			insrt(exp.xy_misc_1[1], 14, 1, 0);
			insrt(exp.xy_misc_1[1], 18, 1, 0);
			insrt(exp.xy_misc_1[1], 20, 1, 0);
			nv03_pgraph_iclip_fixup(&exp, xy, val);
		}
		if (reg >= 0x40053c && reg <= 0x400548) {
			int xy = (reg - 0xc) >> 2 & 1;
			int idx = (reg - 0xc) >> 3 & 1;
			nv03_pgraph_uclip_fixup(&exp, 0, xy, idx, val);
		}
		if (reg >= 0x400560 && reg <= 0x40056c) {
			int xy = reg >> 2 & 1;
			int idx = reg >> 3 & 1;
			nv03_pgraph_uclip_fixup(&exp, 1, xy, idx, val);
		}
		if (reg == 0x400644) {
			insrt(exp.valid, 26, 1, 1);
		}
		if (reg == 0x40050c) {
			insrt(exp.valid, 16, 1, 1);
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("After writing %08x <- %08x\n", reg, val);
			printf("Iteration %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_vtx_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int idx = jrand48(ctx->rand48) & 0x1f;
		int xy = jrand48(ctx->rand48) & 1;
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = 0x400400 + idx * 4 + xy * 0x80;
		uint32_t val = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, reg, val);
		(xy ? exp.vtx_y : exp.vtx_x)[idx] = val;
		nv03_pgraph_vtx_fixup(&exp, xy, 8, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("After writing %08x <- %08x\n", reg, val);
			printf("Iteration %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_iclip_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int xy = jrand48(ctx->rand48) & 1;
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = 0x400534 + xy * 4;
		uint32_t val = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, reg, val);
		insrt(exp.xy_misc_1[0], 14, 1, 0);
		insrt(exp.xy_misc_1[0], 18, 1, 0);
		insrt(exp.xy_misc_1[0], 20, 1, 0);
		insrt(exp.xy_misc_1[1], 14, 1, 0);
		insrt(exp.xy_misc_1[1], 18, 1, 0);
		insrt(exp.xy_misc_1[1], 20, 1, 0);
		nv03_pgraph_iclip_fixup(&exp, xy, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("After writing %08x <- %08x\n", reg, val);
			printf("Iteration %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_uclip_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int xy = jrand48(ctx->rand48) & 1;
		int idx = jrand48(ctx->rand48) & 1;
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = 0x40053c + xy * 4 + idx * 8;
		uint32_t val = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, reg, val);
		nv03_pgraph_uclip_fixup(&exp, 0, xy, idx, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("After writing %08x <- %08x\n", reg, val);
			printf("Iteration %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_oclip_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int xy = jrand48(ctx->rand48) & 1;
		int idx = jrand48(ctx->rand48) & 1;
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = 0x400560 + xy * 4 + idx * 8;
		uint32_t val = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, reg, val);
		nv03_pgraph_uclip_fixup(&exp, 1, xy, idx, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("After writing %08x <- %08x\n", reg, val);
			printf("Iteration %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int nv03_pgraph_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset != 0x03)
		return HWTEST_RES_NA;
	if (!(nva_rd32(ctx->cnum, 0x200) & 1 << 24)) {
		printf("Mem controller not up.\n");
		return HWTEST_RES_UNPREP;
	}
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	return HWTEST_RES_PASS;
}

static int scan_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int state_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

namespace {

HWTEST_DEF_GROUP(scan,
	HWTEST_TEST(test_scan_debug, 0),
	HWTEST_TEST(test_scan_control, 0),
	HWTEST_TEST(test_scan_canvas, 0),
	HWTEST_TEST(test_scan_vtx, 0),
	HWTEST_TEST(test_scan_d3d, 0),
	HWTEST_TEST(test_scan_clip, 0),
	HWTEST_TEST(test_scan_context, 0),
	HWTEST_TEST(test_scan_vstate, 0),
	HWTEST_TEST(test_scan_dma, 0),
)

HWTEST_DEF_GROUP(state,
	HWTEST_TEST(test_state, 0),
	HWTEST_TEST(test_soft_reset, 0),
	HWTEST_TEST(test_mmio_read, 0),
	HWTEST_TEST(test_mmio_write, 0),
	HWTEST_TEST(test_mmio_vtx_write, 0),
	HWTEST_TEST(test_mmio_iclip_write, 0),
	HWTEST_TEST(test_mmio_uclip_write, 0),
	HWTEST_TEST(test_mmio_oclip_write, 0),
)

}

HWTEST_DEF_GROUP(nv03_pgraph,
	HWTEST_GROUP(scan),
	HWTEST_GROUP(state),
)
