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

static_assert(sizeof (nv03_pgraph_state) == sizeof nv03_pgraph_state_regs, "");
static_assert(ARRAY_SIZE(nv03_pgraph_state_regs) == ARRAY_SIZE(nv03_pgraph_state_reg_names), "");

static void nv03_pgraph_gen_state(struct hwtest_ctx *ctx, struct nv03_pgraph_state *state) {
	unsigned i;
	uint32_t *rawstate = (uint32_t *)state;
	uint32_t offset_mask = ctx->chipset.is_nv03t ? 0x007fffff : 0x003fffff;
	uint32_t canvas_mask = ctx->chipset.is_nv03t ? 0x7fff07ff : 0x3fff07ff;
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
		state->surf_offset[i] &= offset_mask & ~0xf;
		state->surf_pitch[i] &= 0x00001ff0;
	}
	state->surf_format &= 0x7777;
	state->src_canvas_min &= canvas_mask;
	state->src_canvas_max &= canvas_mask;
	state->dst_canvas_min &= canvas_mask;
	state->dst_canvas_max &= canvas_mask;
	state->dma &= 0xffff;
	state->notify &= 0xf1ffff;
	state->grobj &= 0xffff;
	state->m2mf &= 0x1ffff;
	state->cliprect_min[0] &= canvas_mask;
	state->cliprect_min[1] &= canvas_mask;
	state->cliprect_max[0] &= canvas_mask;
	state->cliprect_max[1] &= canvas_mask;
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
	state->dma_bytes &= offset_mask;
	state->dma_inst &= 0x0000ffff;
	state->dma_lines &= 0x000007ff;
	state->dma_lin_limit &= offset_mask;
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
	nva_wr32(ctx->cnum, 0x000200, 0xffffeeff);
	nva_wr32(ctx->cnum, 0x000200, 0xffffffff);
	nva_wr32(ctx->cnum, 0x4006a4, 0);
	nva_wr32(ctx->cnum, 0x400100, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400104, 0xffffffff);
	nva_wr32(ctx->cnum, 0x401100, 0xffffffff);
	for (i = 0; i < 8; i++) {
		nva_wr32(ctx->cnum, 0x400648, 0x400 | i);
		nva_wr32(ctx->cnum, 0x40064c, 0xffffffff);
	}
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

static void nv03_pgraph_prep_mthd(struct nv03_pgraph_state *state, uint32_t *pgctx, int cls, uint32_t addr) {
	state->access = 1;
	state->ctx_control = 0x10010003;
	state->ctx_user = (state->ctx_user & 0xffffff) | (*pgctx & 0x7f000000);
	int old_subc = extr(state->ctx_user, 13, 3);
	int new_subc = extr(addr, 13, 3);
	if (old_subc != new_subc) {
		*pgctx &= ~0x1f0000;
		*pgctx |= cls << 16;
	} else {
		state->ctx_user &= ~0x1f0000;
		state->ctx_user |= cls << 16;
	}
	if (extr(state->debug[1], 16, 1) || extr(addr, 0, 13) == 0) {
		*pgctx &= ~0xf000;
		*pgctx |= 0x1000;
	}
}

static void nv03_pgraph_reset(struct nv03_pgraph_state *state) {
	state->bitmap_color_0 &= 0x3fffffff;
	state->dma_intr_en = 0;
	state->xy_misc_0 &= 0x100000;
	state->xy_misc_1[0] &= 0x0f000000;
	state->xy_misc_1[1] &= 0x0f000000;
	state->xy_misc_3 &= ~0x00001100;
	state->xy_misc_4[0] &= 0x30000000;
	state->xy_misc_4[1] &= 0x30000000;
	state->surf_offset[0] = 0;
	state->surf_offset[1] = 0;
	state->surf_offset[2] = 0;
	state->surf_offset[3] = 0;
	state->d3d_config = 0;
	state->misc24_0 = 0;
	state->misc24_1 = 0;
	state->valid = 0;
	state->xy_clip[0][0] = 0x55555555;
	state->xy_clip[0][1] = 0x55555555;
	state->xy_clip[1][0] = 0x55555555;
	state->xy_clip[1][1] = 0x55555555;
}

static void nv03_pgraph_volatile_reset(struct nv03_pgraph_state *state) {
	state->bitmap_color_0 &= 0x3fffffff;
	state->xy_misc_0 = 0;
	state->xy_misc_1[0] &= ~0x001440ff;
	state->xy_misc_1[1] &= ~0x001440fe;
	state->xy_misc_3 &= ~0x00000011;
	state->xy_misc_4[0] = 0;
	state->xy_misc_4[1] = 0;
	state->valid &= 0xf0000000;
	state->misc32_0 &= 0x00ff00ff;
	state->d3d_alpha = 0x800;
	state->xy_clip[0][0] = 0x55555555;
	state->xy_clip[0][1] = 0x55555555;
	state->xy_clip[1][0] = 0x55555555;
	state->xy_clip[1][1] = 0x55555555;
}

static void nv03_pgraph_mthd(struct hwtest_ctx *ctx, struct nv03_pgraph_state *state, uint32_t *grobj, uint32_t gctx, uint32_t addr, uint32_t val) {
	if (extr(state->debug[1], 16, 1) || extr(addr, 0, 13) == 0) {
		uint32_t inst = gctx & 0xffff;
		nva_gwr32(nva_cards[ctx->cnum]->bar1, 0xc00000 + inst * 0x10, grobj[0]);
		nva_gwr32(nva_cards[ctx->cnum]->bar1, 0xc00004 + inst * 0x10, grobj[1]);
		nva_gwr32(nva_cards[ctx->cnum]->bar1, 0xc00008 + inst * 0x10, grobj[2]);
		nva_gwr32(nva_cards[ctx->cnum]->bar1, 0xc0000c + inst * 0x10, grobj[3]);
	}
	if ((addr & 0x1ffc) == 0) {
		int i;
		for (i = 0; i < 0x200; i++) {
			nva_gwr32(nva_cards[ctx->cnum]->bar1, 0xc00000 + i * 8, val);
			nva_gwr32(nva_cards[ctx->cnum]->bar1, 0xc00004 + i * 8, gctx | 1 << 23);
		}
	}
	nva_wr32(ctx->cnum, 0x2100, 0xffffffff);
	nva_wr32(ctx->cnum, 0x2140, 0xffffffff);
	nva_wr32(ctx->cnum, 0x2210, 0);
	nva_wr32(ctx->cnum, 0x2214, 0x1000);
	nva_wr32(ctx->cnum, 0x2218, 0x12000);
	nva_wr32(ctx->cnum, 0x2410, 0);
	nva_wr32(ctx->cnum, 0x2420, 0);
	nva_wr32(ctx->cnum, 0x3000, 0);
	nva_wr32(ctx->cnum, 0x3040, 0);
	nva_wr32(ctx->cnum, 0x3004, gctx >> 24);
	nva_wr32(ctx->cnum, 0x3010, 4);
	nva_wr32(ctx->cnum, 0x3070, 0);
	nva_wr32(ctx->cnum, 0x3080, (addr & 0x1ffc) == 0 ? 0xdeadbeef : gctx | 1 << 23);
	nva_wr32(ctx->cnum, 0x3100, addr);
	nva_wr32(ctx->cnum, 0x3104, val);
	nva_wr32(ctx->cnum, 0x3000, 1);
	nva_wr32(ctx->cnum, 0x3040, 1);
	int old_subc = extr(state->ctx_user, 13, 3);
	int new_subc = extr(addr, 13, 3);
	if (old_subc != new_subc || extr(addr, 0, 13) == 0) {
		if (extr(addr, 0, 13) == 0)
			state->ctx_cache[addr >> 13 & 7] = grobj[0] & 0x3ff3f71f;
		state->ctx_user &= ~0x1fe000;
		state->ctx_user |= gctx & 0x1f0000;
		state->ctx_user |= addr & 0xe000;
		if (extr(state->debug[1], 20, 1))
			state->ctx_switch = state->ctx_cache[addr >> 13 & 7];
		if (extr(state->debug[2], 28, 1)) {
			nv03_pgraph_volatile_reset(state);
			insrt(state->debug[1], 0, 1, 1);
		} else {
			insrt(state->debug[1], 0, 1, 0);
		}
		if (extr(state->notify, 16, 1)) {
			state->intr |= 1;
			state->invalid |= 0x10000;
			state->access = 0;
		}
	}
	if (!state->invalid && extr(state->debug[3], 20, 2) == 3 && extr(addr, 0, 13)) {
		state->invalid |= 0x10;
		state->intr |= 0x1;
		state->access = 0;
	}
	int ncls = extr(gctx, 16, 5);
	if (extr(state->debug[1], 16, 1) && (gctx & 0xffff) != state->grobj &&
		(ncls == 0x0d || ncls == 0x0e || ncls == 0x14 || ncls == 0x17 ||
		 extr(addr, 0, 13) == 0x104)) {
		state->grobj = gctx & 0xffff;
		state->dma = grobj[1] & 0xffff;
		state->notify &= 0xf10000;
		state->notify |= grobj[1] >> 16 & 0xffff;
		state->m2mf = grobj[2] & 0x1ffff;
	}
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
	uint32_t mask = ctx->chipset.is_nv03t ? 0x7fff07ff : 0x3fff07ff;
	TEST_BITSCAN(0x400550, mask, 0);
	TEST_BITSCAN(0x400554, mask, 0);
	TEST_BITSCAN(0x400558, mask, 0);
	TEST_BITSCAN(0x40055c, mask, 0);
	TEST_BITSCAN(0x400690, mask, 0);
	TEST_BITSCAN(0x400694, mask, 0);
	TEST_BITSCAN(0x400698, mask, 0);
	TEST_BITSCAN(0x40069c, mask, 0);
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
	TEST_BITSCAN(0x40062c, 0x7fffffff, 0);
	TEST_BITSCAN(0x400680, 0x0000ffff, 0);
	TEST_BITSCAN(0x400684, 0x00f1ffff, 0);
	TEST_BITSCAN(0x400688, 0x0000ffff, 0);
	TEST_BITSCAN(0x40068c, 0x0001ffff, 0);
	uint32_t offset_mask = ctx->chipset.is_nv03t ? 0x007ffff0 : 0x003ffff0;
	TEST_BITSCAN(0x400630, offset_mask, 0);
	TEST_BITSCAN(0x400634, offset_mask, 0);
	TEST_BITSCAN(0x400638, offset_mask, 0);
	TEST_BITSCAN(0x40063c, offset_mask, 0);
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
	uint32_t offset_mask = ctx->chipset.is_nv03t ? 0x007fffff : 0x003fffff;
	TEST_BITSCAN(0x401270, offset_mask, 0);
	TEST_BITSCAN(0x401280, 0x0000ffff, 0);
	TEST_BITSCAN(0x401290, 0x000007ff, 0);
	TEST_BITSCAN(0x401400, offset_mask, 0);
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
		nv03_pgraph_reset(&exp);
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
			nv03_pgraph_vtx_fixup(&ctx->chipset, &exp, xy, 8, val);
		}
		if (reg >= 0x400534 && reg <= 0x400538) {
			int xy = (reg - 4) >> 2 & 1;
			insrt(exp.xy_misc_1[0], 14, 1, 0);
			insrt(exp.xy_misc_1[0], 18, 1, 0);
			insrt(exp.xy_misc_1[0], 20, 1, 0);
			insrt(exp.xy_misc_1[1], 14, 1, 0);
			insrt(exp.xy_misc_1[1], 18, 1, 0);
			insrt(exp.xy_misc_1[1], 20, 1, 0);
			nv03_pgraph_iclip_fixup(&ctx->chipset, &exp, xy, val);
		}
		if (reg >= 0x40053c && reg <= 0x400548) {
			int xy = (reg - 0xc) >> 2 & 1;
			int idx = (reg - 0xc) >> 3 & 1;
			nv03_pgraph_uclip_fixup(&ctx->chipset, &exp, 0, xy, idx, val);
		}
		if (reg >= 0x400560 && reg <= 0x40056c) {
			int xy = reg >> 2 & 1;
			int idx = reg >> 3 & 1;
			nv03_pgraph_uclip_fixup(&ctx->chipset, &exp, 1, xy, idx, val);
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
		nv03_pgraph_vtx_fixup(&ctx->chipset, &exp, xy, 8, val);
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
		nv03_pgraph_iclip_fixup(&ctx->chipset, &exp, xy, val);
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
		nv03_pgraph_uclip_fixup(&ctx->chipset, &exp, 0, xy, idx, val);
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
		nv03_pgraph_uclip_fixup(&ctx->chipset, &exp, 1, xy, idx, val);
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

static int test_mthd_ctxsw(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int cls = jrand48(ctx->rand48) & 0x1f;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000);
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_notify(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int classes[22] = {
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
			0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
			0x0d, 0x0e, 0x10, 0x11, 0x12, 0x14,
			0x15, 0x17, 0x18, 0x1c,
		};
		int cls = classes[nrand48(ctx->rand48) % 22];
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1)
			val &= 0x3f;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x104;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		if (!extr(exp.invalid, 16, 1)) {
			if (val & ~0xf && extr(exp.debug[3], 20, 1)) {
				exp.intr |= 1;
				exp.invalid |= 0x10;
				exp.access = 0;
			}
			if (extr(exp.notify, 16, 1)) {
				exp.intr |= 1;
				exp.invalid |= 0x1000;
				exp.access = 0;
			}
		}
		if (!extr(exp.invalid, 16, 1) && !extr(exp.invalid, 4, 1)) {
			insrt(exp.notify, 16, 1, 1);
			insrt(exp.notify, 20, 4, val);
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int check_mthd_invalid(struct hwtest_ctx *ctx, int cls, int mthd) {
	int i;
	for (i = 0; i < 4; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		if (!extr(exp.invalid, 16, 1)) {
			exp.intr |= 1;
			exp.invalid |= 1;
			exp.access = 0;
			if (!extr(exp.invalid, 4, 1)) {
				if (extr(exp.notify, 16, 1) && extr(exp.notify, 20, 4)) {
					exp.intr |= 1 << 28;
				}
			}
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return 0;
}

static int test_mthd_invalid(struct hwtest_ctx *ctx) {
	int cls;
	int res = 0;
	for (cls = 0; cls < 0x20; cls++) {
		int mthd;
		for (mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0) /* CTX_SWITCH */
				continue;
			if (((cls >= 1 && cls <= 0xe) || (cls >= 0x10 && cls <= 0x12) ||
			     cls >= 0x14 || cls == 0x15 || cls == 0x17 || cls == 0x18 ||
			     cls == 0x1c) && mthd == 0x104) /* NOTIFY */
				continue;
			if ((cls == 1 || cls == 2) && mthd == 0x300) /* ROP, BETA */
				continue;
			if ((cls == 3 || cls == 4) && mthd == 0x304) /* CHROMA, PLANE */
				continue;
			if (cls == 5 && (mthd == 0x300 || mthd == 0x304)) /* CLIP */
				continue;
			if (cls == 6 && (mthd == 0x308 || (mthd >= 0x310 && mthd <= 0x31c))) /* PATTERN */
				continue;
			if (cls >= 7 && cls <= 0xb && mthd == 0x304) /* COLOR */
				continue;
			if (cls == 7 && mthd >= 0x400 && mthd < 0x480) /* RECT */
				continue;
			if (cls == 8 && mthd >= 0x400 && mthd < 0x580) /* POINT */
				continue;
			if (cls >= 9 && cls <= 0xa && mthd >= 0x400 && mthd < 0x680) /* LINE/LIN */
				continue;
			if (cls == 0xb && mthd >= 0x310 && mthd < 0x31c) /* TRI.TRIANGLE */
				continue;
			if (cls == 0xb && mthd >= 0x320 && mthd < 0x338) /* TRI.TRIANGLE32 */
				continue;
			if (cls == 0xb && mthd >= 0x400 && mthd < 0x600) /* TRI.TRIMESH, TRI.TRIMESH32, TRI.CTRIANGLE, TRI.CTRIMESH */
				continue;
			if (cls == 0xc && mthd >= 0x3fc && mthd < 0x600) /* GDIRECT.RECT_NCLIP */
				continue;
			if (cls == 0xc && mthd >= 0x7f4 && mthd < 0xa00) /* GDIRECT.RECT_CLIP */
				continue;
			if (cls == 0xc && mthd >= 0xbec && mthd < 0xe00) /* GDIRECT.BITMAP_1COLOR */
				continue;
			if (cls == 0xc && mthd >= 0xfe8 && mthd < 0x1200) /* GDIRECT.BITMAP_1COLOR_ICLIP */
				continue;
			if (cls == 0xc && mthd >= 0x13e4 && mthd < 0x1600) /* GDIRECT.BITMAP_2COLOR_ICLIP */
				continue;
			if (cls == 0xd && mthd >= 0x30c && mthd < 0x32c) /* M2MF */
				continue;
			if (cls == 0xe && mthd >= 0x308 && mthd < 0x320) /* SIFM */
				continue;
			if (cls == 0xe && mthd >= 0x400 && mthd < 0x418) /* SIFM */
				continue;
			if (cls == 0x10 && mthd >= 0x300 && mthd < 0x30c) /* BLIT */
				continue;
			if (cls == 0x11 && mthd >= 0x304 && mthd < 0x310) /* IFC setup */
				continue;
			if (cls == 0x12 && mthd >= 0x308 && mthd < 0x31c) /* BITMAP setup */
				continue;
			if (cls >= 0x11 && cls <= 0x12 && mthd >= 0x400 && mthd < 0x480) /* IFC/BITMAP data */
				continue;
			if (cls == 0x14 && mthd >= 0x308 && mthd < 0x318) /* ITM */
				continue;
			if (cls == 0x15 && mthd >= 0x304 && mthd < 0x31c) /* SIFC */
				continue;
			if (cls == 0x15 && mthd >= 0x400 && mthd < 0x2000) /* SIFC data */
				continue;
			if (cls == 0x17 && mthd >= 0x304 && mthd < 0x31c) /* D3D */
				continue;
			if (cls == 0x17 && mthd >= 0x1000 && mthd < 0x2000) /* D3D */
				continue;
			if (cls == 0x18 && mthd >= 0x304 && mthd < 0x30c) /* ZPOINT */
				continue;
			if (cls == 0x18 && mthd >= 0x7fc && mthd < 0x1000) /* ZPOINT */
				continue;
			if (cls == 0x1c && (mthd == 0x300 || mthd == 0x308 || mthd == 0x30c)) /* SURF */
				continue;
			res |= check_mthd_invalid(ctx, cls, mthd);
		}
	}
	return res ? HWTEST_RES_FAIL : HWTEST_RES_PASS;
}

static int test_mthd_beta(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x300;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x01, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.beta = val;
		if (exp.beta & 0x80000000)
			exp.beta = 0;
		exp.beta &= 0x7f800000;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_rop(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x300;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x02, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.rop = val & 0xff;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_chroma(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x304;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x03, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.chroma = nv01_pgraph_to_a1r10g10b10(nv03_pgraph_expand_color(exp.ctx_switch, val));
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_plane(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x304;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x04, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_shape(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x308;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1)
			val &= 7;
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x06, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.pattern_shape = val & 3;
		if (val > 2 && extr(exp.debug[3], 20, 1)) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_mono_color(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x310 | idx << 2;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x06, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		struct nv01_color c = nv03_pgraph_expand_color(exp.ctx_switch, val);
		exp.pattern_rgb[idx] = c.r << 20 | c.g << 10 | c.b;
		exp.pattern_a[idx] = c.a;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_mono_bitmap(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x318 | idx << 2;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x06, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		// yup, hw bug.
		exp.pattern_bitmap[idx] = nv03_pgraph_expand_mono(orig.ctx_switch, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x300;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x01010003;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x1c, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int s = extr(exp.ctx_switch, 16, 2);
		int f = 1;
		if (extr(val, 0, 1))
			f = 0;
		if (!extr(val, 16, 1))
			f = 2;
		if (!extr(val, 24, 1))
			f = 3;
		insrt(exp.surf_format, 4*s, 3, f | 4);
		if (extr(exp.debug[3], 20, 1) &&
		    (val != 1 && val != 0x01000000 && val != 0x01010000 && val != 0x01010001)) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_pitch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x308;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x1c, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int s = extr(exp.ctx_switch, 16, 2);
		exp.surf_pitch[s] = val & 0x1ff0;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_offset(struct hwtest_ctx *ctx) {
	int i;
	uint32_t offset_mask = ctx->chipset.is_nv03t ? 0x007ffff0 : 0x003ffff0;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x30c;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x1c, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int s = extr(exp.ctx_switch, 16, 2);
		exp.surf_offset[s] = val & offset_mask;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_solid_color(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t mthd;
		int cls;
		int which = 0;
		switch (nrand48(ctx->rand48) % 14) {
			case 0:
				mthd = 0x304;
				cls = 7 + nrand48(ctx->rand48)%5;
				break;
			case 1:
				mthd = 0x500 | (jrand48(ctx->rand48)&0x78);
				cls = 8;
				break;
			case 2:
				mthd = 0x600 | (jrand48(ctx->rand48)&0x78);
				cls = 9 + (jrand48(ctx->rand48)&1);
				break;
			case 3:
				mthd = 0x500 | (jrand48(ctx->rand48)&0x70);
				cls = 0xb;
				break;
			case 4:
				mthd = 0x580 | (jrand48(ctx->rand48)&0x78);
				cls = 0xb;
				break;
			case 5:
				mthd = 0x3fc;
				cls = 0xc;
				break;
			case 6:
				mthd = 0x7fc;
				cls = 0xc;
				break;
			case 7:
				mthd = 0xbf4;
				cls = 0xc;
				which = 2;
				break;
			case 8:
				mthd = 0xff0;
				cls = 0xc;
				which = 2;
				break;
			case 9:
				mthd = 0x13ec;
				cls = 0xc;
				which = 1;
				break;
			case 10:
				mthd = 0x13f0;
				cls = 0xc;
				which = 2;
				break;
			case 11:
				mthd = 0x308;
				cls = 0x12;
				which = 1;
				break;
			case 12:
				mthd = 0x30c;
				cls = 0x12;
				which = 2;
				break;
			case 13:
				mthd = 0x800 | (jrand48(ctx->rand48)&0x7f8);
				cls = 0x18;
				break;
			default:
				abort();
		}
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		switch (which) {
			case 0:
				exp.misc32_0 = val;
				exp.valid |= 0x10000;
				break;
			case 1:
				exp.bitmap_color_0 = nv01_pgraph_to_a1r10g10b10(nv03_pgraph_expand_color(exp.ctx_switch, val));
				exp.valid |= 0x20000;
				break;
			case 2:
				exp.misc32_1 = val;
				exp.valid |= 0x40000;
				break;
			default:
				abort();
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_offset(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		int cls = 0x0d;
		uint32_t mthd = 0x30c + idx * 4;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.dma_offset[idx] = val;
		exp.valid |= 1 << idx;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_pitch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		int cls = 0x0d;
		uint32_t mthd = 0x314 + idx * 4;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		insrt(exp.dma_pitch, 16 * idx, 16, val);
		exp.valid |= 1 << (idx + 2);
		if (extr(exp.debug[3], 20, 1) && val & ~0x7fff) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_line_length(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int cls = 0x0d;
		uint32_t mthd = 0x31c;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffffff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.misc24_0 = val & 0xffffff;
		exp.valid |= 1 << 4;
		if (extr(exp.debug[3], 20, 1) && val > 0x3fffff) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_line_count(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int cls = 0x0d;
		uint32_t mthd = 0x320;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xfff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.dma_lines = val & 0x7ff;
		exp.valid |= 1 << 5;
		if (extr(exp.debug[3], 20, 1) && val > 0x7ff) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int cls = 0x0d;
		uint32_t mthd = 0x324;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= ~0xf8;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.dma_format = val & 0x707;
		exp.valid |= 1 << 6;
		int a = extr(val, 0, 3);
		int b = extr(val, 8, 3);
		if (extr(exp.debug[3], 20, 1) && (val & 0xf8 ||
			!(a == 1 || a == 2 || a == 4) ||
			!(b == 1 || b == 2 || b == 4))) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifm_offset(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int idx = nrand48(ctx->rand48) % 3;
		int cls = 0x0e;
		uint32_t mthd = 0x408 + idx * 4;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x4006a4, 0);
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.dma_offset[idx] = val;
		int fmt;
		if (orig.grobj == exp.grobj) {
			fmt = extr(orig.ctx_switch, 0, 3);
		} else {
			fmt = extr(exp.ctx_switch, 0, 3);
		}
		if (fmt == 7) {
			exp.valid |= 1 << (15 + idx);
			exp.valid &= ~0x800;
		} else {
			if (idx)
				continue;
			exp.valid |= 1 << 11;
			exp.valid &= ~0x00038000;
		}
		nva_wr32(ctx->cnum, 0x4006a4, 1);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifm_pitch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0x0e;
		uint32_t mthd = 0x404;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int fmt;
		if (orig.grobj == exp.grobj) {
			fmt = extr(orig.ctx_switch, 0, 3);
		} else {
			fmt = extr(exp.ctx_switch, 0, 3);
		}
		if (fmt == 7) {
			exp.dma_pitch = val;
			exp.valid |= 1 << 14;
			exp.valid &= ~0x400;
		} else {
			insrt(exp.dma_pitch, 0, 16, val);
			exp.valid |= 1 << 10;
			exp.valid &= ~0x00004000;
			if (extr(exp.debug[3], 20, 1) && val & ~0x1fff) {
				exp.intr |= 1;
				exp.invalid |= 0x10;
				exp.access = 0;
			}
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifm_invalid(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		int cls = 0x0e;
		uint32_t mthd = 0x410 + idx * 4;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x4006a4, 0);
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int fmt;
		if (orig.grobj == exp.grobj) {
			fmt = extr(orig.ctx_switch, 0, 3);
		} else {
			fmt = extr(exp.ctx_switch, 0, 3);
		}
		if (fmt == 7)
			continue;
		nva_wr32(ctx->cnum, 0x4006a4, 1);
		exp.intr |= 1;
		exp.invalid |= 1;
		exp.access = 0;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_itm_pitch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0x14;
		uint32_t mthd = 0x310;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		insrt(exp.dma_pitch, 0, 16, val);
		exp.valid |= 1 << 10;
		exp.valid &= ~0x4000;
		if (extr(exp.debug[3], 20, 1) && val & ~0x1fff) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_clip(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		int which;
		int cls;
		uint32_t mthd;
		switch (nrand48(ctx->rand48) % 7) {
			case 0:
				cls = 0x05;
				mthd = 0x300 + idx * 4;
				which = 0;
				break;
			case 1:
				cls = 0x0c;
				mthd = 0x7f4 + idx * 4;
				which = 3;
				break;
			case 2:
				cls = 0x0c;
				mthd = 0xbec + idx * 4;
				which = 3;
				break;
			case 3:
				cls = 0x0c;
				mthd = 0xfe8 + idx * 4;
				which = 3;
				break;
			case 4:
				cls = 0x0c;
				mthd = 0x13e4 + idx * 4;
				which = 3;
				break;
			case 5:
				cls = 0x0e;
				mthd = 0x0308 + idx * 4;
				which = 1;
				break;
			case 6:
				cls = 0x15;
				mthd = 0x0310 + idx * 4;
				which = 2;
				break;
			default:
				abort();
		}
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1) {
			orig.vtx_x[8] &= 0x8000000f;
			orig.vtx_y[8] &= 0x8000000f;
		}
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_x[13], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_y[13], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
		}
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		nv03_pgraph_set_clip(&ctx->chipset, &exp, which, idx, val, extr(orig.xy_misc_1[0], 0, 1));
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_vtx(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls;
		uint32_t mthd;
		bool first = false;
		bool noclip = false;
		bool ifc = false;
		bool draw = false;
		bool poly = false;
		switch (nrand48(ctx->rand48) % 27) {
			case 0:
				cls = 0x07;
				mthd = 0x400 | (jrand48(ctx->rand48) & 0x78);
				first = true;
				break;
			case 1:
				cls = 0x08;
				mthd = 0x400 | (jrand48(ctx->rand48) & 0x7c);
				first = true;
				draw = true;
				break;
			case 2:
				cls = 0x08;
				mthd = 0x504 | (jrand48(ctx->rand48) & 0x78);
				first = true;
				draw = true;
				break;
			case 3:
				cls = 0x09 + (jrand48(ctx->rand48) & 1);
				mthd = 0x400 | (jrand48(ctx->rand48) & 0x78);
				first = true;
				break;
			case 4:
				cls = 0x09 + (jrand48(ctx->rand48) & 1);
				mthd = 0x404 | (jrand48(ctx->rand48) & 0x78);
				draw = true;
				break;
			case 5:
				cls = 0x09 + (jrand48(ctx->rand48) & 1);
				mthd = 0x500 | (jrand48(ctx->rand48) & 0x7c);
				poly = true;
				draw = true;
				break;
			case 6:
				cls = 0x09 + (jrand48(ctx->rand48) & 1);
				mthd = 0x604 | (jrand48(ctx->rand48) & 0x78);
				poly = true;
				draw = true;
				break;
			case 7:
				cls = 0x0b;
				mthd = 0x310;
				first = true;
				break;
			case 8:
				cls = 0x0b;
				mthd = 0x314;
				break;
			case 9:
				cls = 0x0b;
				mthd = 0x318;
				draw = true;
				break;
			case 10:
				cls = 0x0b;
				mthd = 0x400 | (jrand48(ctx->rand48) & 0x7c);
				poly = true;
				draw = true;
				break;
			case 11:
				cls = 0x0b;
				mthd = 0x504 | (jrand48(ctx->rand48) & 0x70);
				first = true;
				break;
			case 12:
				cls = 0x0b;
				mthd = 0x508 | (jrand48(ctx->rand48) & 0x70);
				break;
			case 13:
				cls = 0x0b;
				mthd = 0x50c | (jrand48(ctx->rand48) & 0x70);
				draw = true;
				break;
			case 14:
				cls = 0x0b;
				mthd = 0x584 | (jrand48(ctx->rand48) & 0x78);
				poly = true;
				draw = true;
				break;
			case 15:
				cls = 0x0c;
				mthd = 0x400 | (jrand48(ctx->rand48) & 0x1f8);
				first = true;
				noclip = true;
				break;
			case 16:
				cls = 0x0c;
				mthd = 0x800 | (jrand48(ctx->rand48) & 0x1f8);
				first = true;
				break;
			case 17:
				cls = 0x0c;
				mthd = 0x804 | (jrand48(ctx->rand48) & 0x1f8);
				draw = true;
				break;
			case 18:
				cls = 0xc;
				mthd = 0xbfc;
				first = true;
				ifc = true;
				break;
			case 19:
				cls = 0xc;
				mthd = 0xffc;
				first = true;
				ifc = true;
				break;
			case 20:
				cls = 0xc;
				mthd = 0x13fc;
				first = true;
				ifc = true;
				break;
			case 21:
				cls = 0x10;
				mthd = 0x300;
				first = true;
				break;
			case 22:
				cls = 0x10;
				mthd = 0x304;
				break;
			case 23:
				cls = 0x11;
				mthd = 0x304;
				first = true;
				ifc = true;
				break;
			case 24:
				cls = 0x12;
				mthd = 0x310;
				first = true;
				ifc = true;
				break;
			case 25:
				cls = 0x14;
				mthd = 0x308;
				first = true;
				break;
			case 26:
				cls = 0x18;
				mthd = 0x7fc;
				first = true;
				break;
			default:
				abort();
		}
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		if (draw) {
			if (jrand48(ctx->rand48) & 3)
				insrt(orig.cliprect_ctrl, 8, 1, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(orig.debug[2], 28, 1, 0);
			if (jrand48(ctx->rand48) & 3) {
				insrt(orig.xy_misc_4[0], 4, 4, 0);
				insrt(orig.xy_misc_4[1], 4, 4, 0);
			}
			if (jrand48(ctx->rand48) & 3) {
				orig.valid |= jrand48(ctx->rand48);
				orig.valid |= jrand48(ctx->rand48);
			}
			if (jrand48(ctx->rand48) & 1) {
				insrt(orig.ctx_switch, 24, 5, 0x17);
				insrt(orig.ctx_switch, 13, 2, 0);
				int j;
				for (j = 0; j < 8; j++) {
					insrt(orig.ctx_cache[j], 24, 5, 0x17);
					insrt(orig.ctx_cache[j], 13, 2, 0);
				}
			}
		}
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int vidx;
		if (first) {
			vidx = 0;
			if (cls >= 9 && cls <= 0xb) {
				exp.valid &= ~0xffff;
				insrt(exp.valid, 21, 1, 1);
			}
			if (cls == 0x18)
				insrt(exp.valid, 21, 1, 1);
		} else {
			vidx = extr(exp.xy_misc_0, 28, 4);
		}
		int rvidx = ifc ? 4 : vidx;
		int svidx = vidx & 3;
		int nvidx = (vidx + 1) & 0xf;
		if (cls == 0x8 || cls == 0x9 || cls == 0xa || cls == 0xc)
			nvidx &= 1;
		if (vidx == 2 && cls == 0xb)
			nvidx = 0;
		if (vidx == 3 && cls == 0x10)
			nvidx = 0;
		if (cls == 0xc && !first)
			vidx = rvidx = svidx = 1;
		insrt(exp.xy_misc_0, 28, 4, nvidx);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		if (cls != 0xc || first)
			insrt(exp.xy_misc_3, 8, 1, 0);
		if (poly && (exp.valid & 0xf0f))
			insrt(exp.valid, 21, 1, 0);
		if (!poly) {
			insrt(exp.valid, rvidx, 1, 1);
			insrt(exp.valid, 8|rvidx, 1, 1);
		}
		if ((cls >= 9 && cls <= 0xb)) {
			insrt(exp.valid, 4|svidx, 1, 1);
			insrt(exp.valid, 0xc|svidx, 1, 1);
		}
		if (cls == 0x10) {
			insrt(exp.valid, vidx, 1, 1);
			insrt(exp.valid, vidx|8, 1, 1);
		}
		if (cls != 0xc || first)
			insrt(exp.valid, 19, 1, noclip);
		if (cls >= 8 && cls <= 0x14) {
			insrt(exp.xy_misc_4[0], 0+svidx, 1, 0);
			insrt(exp.xy_misc_4[0], 4+svidx, 1, 0);
			insrt(exp.xy_misc_4[1], 0+svidx, 1, 0);
			insrt(exp.xy_misc_4[1], 4+svidx, 1, 0);
		}
		if (noclip) {
			exp.vtx_x[rvidx] = extrs(val, 16, 16);
			exp.vtx_y[rvidx] = extrs(val, 0, 16);
		} else {
			exp.vtx_x[rvidx] = extrs(val, 0, 16);
			exp.vtx_y[rvidx] = extrs(val, 16, 16);
		}
		int xcstat = nv03_pgraph_clip_status(&ctx->chipset, &exp, exp.vtx_x[rvidx], 0, noclip);
		int ycstat = nv03_pgraph_clip_status(&ctx->chipset, &exp, exp.vtx_y[rvidx], 1, noclip);
		insrt(exp.xy_clip[0][vidx >> 3], 4*(vidx & 7), 4, xcstat);
		insrt(exp.xy_clip[1][vidx >> 3], 4*(vidx & 7), 4, ycstat);
		if (cls == 0x08 || cls == 0x18) {
			insrt(exp.xy_clip[0][vidx >> 3], 4*((vidx|1) & 7), 4, xcstat);
			insrt(exp.xy_clip[1][vidx >> 3], 4*((vidx|1) & 7), 4, ycstat);
		}
		if (draw) {
			nv03_pgraph_prep_draw(&exp, poly, false);
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (real.status && cls == 0xb) {
			/* Hung PGRAPH... */
			continue;
		}
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_x32(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls;
		uint32_t mthd;
		bool first = false;
		bool poly = false;
		switch (nrand48(ctx->rand48) % 8) {
			case 0:
				cls = 0x08;
				mthd = 0x480 | (jrand48(ctx->rand48) & 0x78);
				first = true;
				break;
			case 1:
				cls = 0x09 + (jrand48(ctx->rand48) & 1);
				mthd = 0x480 | (jrand48(ctx->rand48) & 0x70);
				first = true;
				break;
			case 2:
				cls = 0x09 + (jrand48(ctx->rand48) & 1);
				mthd = 0x488 | (jrand48(ctx->rand48) & 0x70);
				break;
			case 3:
				cls = 0x09 + (jrand48(ctx->rand48) & 1);
				mthd = 0x580 | (jrand48(ctx->rand48) & 0x78);
				poly = true;
				break;
			case 4:
				cls = 0x0b;
				mthd = 0x320;
				first = true;
				break;
			case 5:
				cls = 0x0b;
				mthd = 0x328;
				break;
			case 6:
				cls = 0x0b;
				mthd = 0x330;
				break;
			case 7:
				cls = 0x0b;
				mthd = 0x480 | (jrand48(ctx->rand48) & 0x78);
				poly = true;
				break;
			default:
				abort();
		}
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1) {
			orig.valid &= ~(jrand48(ctx->rand48) & 0x00000f0f);
			orig.valid &= ~(jrand48(ctx->rand48) & 0x00000f0f);
		}
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int vidx;
		if (first) {
			vidx = 0;
			if (cls >= 9 && cls <= 0xb) {
				exp.valid &= ~0xffff;
				insrt(exp.valid, 21, 1, 1);
			}
			if (cls == 0x18)
				insrt(exp.valid, 21, 1, 1);
		} else {
			vidx = extr(exp.xy_misc_0, 28, 4);
		}
		int svidx = vidx & 3;
		insrt(exp.xy_misc_0, 28, 4, vidx);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		insrt(exp.xy_misc_3, 8, 1, 0);
		if (poly && (exp.valid & 0xf0f))
			insrt(exp.valid, 21, 1, 0);
		if (!poly)
			insrt(exp.valid, vidx, 1, 1);
		if ((cls >= 9 && cls <= 0xb))
			insrt(exp.valid, 4|svidx, 1, 1);
		insrt(exp.valid, 19, 1, false);
		if (cls >= 8 && cls <= 0x14) {
			insrt(exp.xy_misc_4[0], 0+svidx, 1, 0);
			insrt(exp.xy_misc_4[0], 4+svidx, 1, (int32_t)val != sext(val, 15));
		}
		exp.vtx_x[vidx] = val;
		int xcstat = nv03_pgraph_clip_status(&ctx->chipset, &exp, exp.vtx_x[vidx], 0, false);
		insrt(exp.xy_clip[0][vidx >> 3], 4*(vidx & 7), 4, xcstat);
		if (cls == 0x08 || cls == 0x18) {
			insrt(exp.xy_clip[0][vidx >> 3], 4*((vidx|1) & 7), 4, xcstat);
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_y32(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls;
		uint32_t mthd;
		bool draw = false;
		bool poly = false;
		switch (nrand48(ctx->rand48) % 8) {
			case 0:
				cls = 0x08;
				mthd = 0x484 | (jrand48(ctx->rand48) & 0x78);
				draw = true;
				break;
			case 1:
				cls = 0x09 + (jrand48(ctx->rand48) & 1);
				mthd = 0x484 | (jrand48(ctx->rand48) & 0x70);
				break;
			case 2:
				cls = 0x09 + (jrand48(ctx->rand48) & 1);
				mthd = 0x48c | (jrand48(ctx->rand48) & 0x70);
				draw = true;
				break;
			case 3:
				cls = 0x09 + (jrand48(ctx->rand48) & 1);
				mthd = 0x584 | (jrand48(ctx->rand48) & 0x78);
				draw = true;
				poly = true;
				break;
			case 4:
				cls = 0x0b;
				mthd = 0x324;
				break;
			case 5:
				cls = 0x0b;
				mthd = 0x32c;
				break;
			case 6:
				cls = 0x0b;
				mthd = 0x334;
				draw = true;
				break;
			case 7:
				cls = 0x0b;
				mthd = 0x484 | (jrand48(ctx->rand48) & 0x78);
				draw = true;
				poly = true;
				break;
			default:
				abort();
		}
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 1)
				val &= 0xffff;
			else
				val |= 0xffff0000;
		}
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1) {
			orig.valid &= ~(jrand48(ctx->rand48) & 0x00000f0f);
			orig.valid &= ~(jrand48(ctx->rand48) & 0x00000f0f);
		}
		if (draw) {
			if (jrand48(ctx->rand48) & 3)
				insrt(orig.cliprect_ctrl, 8, 1, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(orig.debug[2], 28, 1, 0);
			if (jrand48(ctx->rand48) & 3) {
				insrt(orig.xy_misc_4[0], 4, 4, 0);
				insrt(orig.xy_misc_4[1], 4, 4, 0);
			}
			if (jrand48(ctx->rand48) & 3) {
				orig.valid |= jrand48(ctx->rand48);
				orig.valid |= jrand48(ctx->rand48);
			}
			if (jrand48(ctx->rand48) & 1) {
				insrt(orig.ctx_switch, 24, 5, 0x17);
				insrt(orig.ctx_switch, 13, 2, 0);
				int j;
				for (j = 0; j < 8; j++) {
					insrt(orig.ctx_cache[j], 24, 5, 0x17);
					insrt(orig.ctx_cache[j], 13, 2, 0);
				}
			}
		}
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int vidx;
		vidx = extr(exp.xy_misc_0, 28, 4);
		int svidx = vidx & 3;
		int nvidx = (vidx + 1) & 0xf;
		if (cls == 0x8 || cls == 0x9 || cls == 0xa)
			nvidx &= 1;
		if (vidx == 2 && cls == 0xb)
			nvidx = 0;
		insrt(exp.xy_misc_0, 28, 4, nvidx);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		insrt(exp.xy_misc_3, 8, 1, 0);
		if (poly && (exp.valid & 0xf0f))
			insrt(exp.valid, 21, 1, 0);
		if (!poly)
			insrt(exp.valid, vidx|8, 1, 1);
		if ((cls >= 9 && cls <= 0xb))
			insrt(exp.valid, 0xc|svidx, 1, 1);
		insrt(exp.valid, 19, 1, false);
		if (cls >= 8 && cls <= 0x14) {
			insrt(exp.xy_misc_4[1], 0+svidx, 1, 0);
			insrt(exp.xy_misc_4[1], 4+svidx, 1, (int32_t)val != sext(val, 15));
		}
		exp.vtx_y[vidx] = val;
		int ycstat = nv03_pgraph_clip_status(&ctx->chipset, &exp, exp.vtx_y[vidx], 1, false);
		if (cls == 0x08 || cls == 0x18) {
			insrt(exp.xy_clip[1][0], 0, 4, ycstat);
			insrt(exp.xy_clip[1][0], 4, 4, ycstat);
		} else {
			insrt(exp.xy_clip[1][vidx >> 3], 4*(vidx & 7), 4, ycstat);
		}
		if (draw) {
			nv03_pgraph_prep_draw(&exp, poly, false);
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (real.status && draw && (cls == 0xb || cls == 9 || cls == 0xa)) {
			/* Hung PGRAPH... */
			continue;
		}
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_rect(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls;
		uint32_t mthd;
		bool noclip = false;
		switch (nrand48(ctx->rand48) % 2) {
			case 0:
				cls = 0x7;
				mthd = 0x404 | (jrand48(ctx->rand48) & 0x78);
				break;
			case 1:
				cls = 0xc;
				mthd = 0x404 | (jrand48(ctx->rand48) & 0x1f8);
				noclip = true;
				break;
			default:
				abort();
		}
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0x000f000f;
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 3)
			insrt(orig.cliprect_ctrl, 8, 1, 0);
		if (jrand48(ctx->rand48) & 3)
			insrt(orig.debug[2], 28, 1, 0);
		if (jrand48(ctx->rand48) & 3) {
			insrt(orig.xy_misc_4[0], 4, 4, 0);
			insrt(orig.xy_misc_4[1], 4, 4, 0);
		}
		if (jrand48(ctx->rand48) & 3) {
			orig.valid |= jrand48(ctx->rand48);
			orig.valid |= jrand48(ctx->rand48);
		}
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.ctx_switch, 24, 5, 0x17);
			insrt(orig.ctx_switch, 13, 2, 0);
			int j;
			for (j = 0; j < 8; j++) {
				insrt(orig.ctx_cache[j], 24, 5, 0x17);
				insrt(orig.ctx_cache[j], 13, 2, 0);
			}
		}
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 1) {
				orig.vtx_x[0] &= 0x8000ffff;
			} else {
				orig.vtx_x[0] |= 0x7fff0000;
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 1) {
				orig.vtx_y[0] &= 0x8000ffff;
			} else {
				orig.vtx_y[0] |= 0x7fff0000;
			}
		}
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int vidx = extr(exp.xy_misc_0, 28, 4);
		int nvidx = (vidx + 1) & 1;
		insrt(exp.xy_misc_0, 28, 4, nvidx);
		if (noclip) {
			nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 1, exp.vtx_x[0], extr(val, 16, 16), 0, true);
			nv03_pgraph_vtx_add(&ctx->chipset, &exp, 1, 1, exp.vtx_y[0], extr(val, 0, 16), 0, true);
		} else {
			nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 1, exp.vtx_x[0], extr(val, 0, 16), 0, false);
			nv03_pgraph_vtx_add(&ctx->chipset, &exp, 1, 1, exp.vtx_y[0], extr(val, 16, 16), 0, false);
		}
		if (noclip) {
			nv03_pgraph_vtx_cmp(&exp, 0, 2, false);
			nv03_pgraph_vtx_cmp(&exp, 1, 2, false);
		} else {
			nv03_pgraph_vtx_cmp(&exp, 0, 8, true);
			nv03_pgraph_vtx_cmp(&exp, 1, 8, true);
		}
		exp.valid |= 0x202;
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		nv03_pgraph_prep_draw(&exp, false, noclip);
		nv03_pgraph_dump_state(ctx, &real);
		if (real.status) {
			/* Hung PGRAPH... */
			continue;
		}
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_zpoint_zeta(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0x18;
		uint32_t mthd = 0x804 | (jrand48(ctx->rand48) & 0x7f8);
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 3)
			insrt(orig.cliprect_ctrl, 8, 1, 0);
		if (jrand48(ctx->rand48) & 3)
			insrt(orig.debug[2], 28, 1, 0);
		if (jrand48(ctx->rand48) & 3) {
			insrt(orig.xy_misc_4[0], 4, 4, 0);
			insrt(orig.xy_misc_4[1], 4, 4, 0);
		}
		if (jrand48(ctx->rand48) & 3) {
			orig.valid |= jrand48(ctx->rand48);
			orig.valid |= jrand48(ctx->rand48);
		}
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.ctx_switch, 24, 5, 0x17);
			insrt(orig.ctx_switch, 13, 2, 0);
			int j;
			for (j = 0; j < 8; j++) {
				insrt(orig.ctx_cache[j], 24, 5, 0x17);
				insrt(orig.ctx_cache[j], 13, 2, 0);
			}
		}
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		insrt(exp.misc32_1, 16, 16, extr(val, 16, 16));
		nv03_pgraph_prep_draw(&exp, false, false);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 0, exp.vtx_x[0], 1, 0, false);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_blit_rect(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0x10;
		uint32_t mthd = 0x308;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 3)
			insrt(orig.cliprect_ctrl, 8, 1, 0);
		if (jrand48(ctx->rand48) & 3)
			insrt(orig.debug[2], 28, 1, 0);
		if (jrand48(ctx->rand48) & 3) {
			insrt(orig.xy_misc_4[0], 4, 4, 0);
			insrt(orig.xy_misc_4[1], 4, 4, 0);
		}
		if (jrand48(ctx->rand48) & 3) {
			orig.valid |= jrand48(ctx->rand48);
			orig.valid |= jrand48(ctx->rand48);
		}
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.ctx_switch, 24, 5, 0x17);
			insrt(orig.ctx_switch, 13, 2, 0);
			int j;
			for (j = 0; j < 8; j++) {
				insrt(orig.ctx_cache[j], 24, 5, 0x17);
				insrt(orig.ctx_cache[j], 13, 2, 0);
			}
		}
		if (jrand48(ctx->rand48) & 3) {
			if (jrand48(ctx->rand48) & 1) {
				orig.vtx_x[0] &= 0x8000ffff;
			} else {
				orig.vtx_x[0] |= 0x7fff0000;
			}
		}
		if (jrand48(ctx->rand48) & 3) {
			if (jrand48(ctx->rand48) & 1) {
				orig.vtx_y[0] &= 0x8000ffff;
			} else {
				orig.vtx_y[0] |= 0x7fff0000;
			}
		}
		if (jrand48(ctx->rand48) & 3) {
			if (jrand48(ctx->rand48) & 1) {
				orig.vtx_x[1] &= 0x8000ffff;
			} else {
				orig.vtx_x[1] |= 0x7fff0000;
			}
		}
		if (jrand48(ctx->rand48) & 3) {
			if (jrand48(ctx->rand48) & 1) {
				orig.vtx_y[1] &= 0x8000ffff;
			} else {
				orig.vtx_y[1] |= 0x7fff0000;
			}
		}
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int vtxid = extr(exp.xy_misc_0, 28, 4);
		vtxid++;
		if (vtxid == 4)
			vtxid = 0;
		vtxid++;
		if (vtxid == 4)
			vtxid = 0;
		insrt(exp.xy_misc_0, 28, 4, vtxid);
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 2, exp.vtx_x[0], extr(val, 0, 16), 0, false);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 1, 2, exp.vtx_y[0], extr(val, 16, 16), 0, false);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 3, exp.vtx_x[1], extr(val, 0, 16), 0, false);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 1, 3, exp.vtx_y[1], extr(val, 16, 16), 0, false);
		nv03_pgraph_vtx_cmp(&exp, 0, 8, true);
		nv03_pgraph_vtx_cmp(&exp, 1, 8, true);
		exp.valid |= 0xc0c;
		nv03_pgraph_prep_draw(&exp, false, false);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ifc_size(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls;
		uint32_t mthd;
		bool is_in = false;
		bool is_out = false;
		switch (nrand48(ctx->rand48) % 10) {
			case 0:
				cls = 0xc;
				mthd = 0xbf8;
				is_in = is_out = true;
				break;
			case 1:
				cls = 0xc;
				mthd = 0xff4;
				is_in = true;
				break;
			case 2:
				cls = 0xc;
				mthd = 0xff8;
				is_out = true;
				break;
			case 3:
				cls = 0xc;
				mthd = 0x13f4;
				is_in = true;
				break;
			case 4:
				cls = 0xc;
				mthd = 0x13f8;
				is_out = true;
				break;
			case 5:
				cls = 0x11;
				mthd = 0x308;
				is_out = true;
				break;
			case 6:
				cls = 0x11;
				mthd = 0x30c;
				is_in = true;
				break;
			case 7:
				cls = 0x12;
				mthd = 0x314;
				is_out = true;
				break;
			case 8:
				cls = 0x12;
				mthd = 0x318;
				is_in = true;
				break;
			case 9:
				cls = 0x15;
				mthd = 0x304;
				is_in = true;
				break;
			default:
				abort();
		}
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0x000f003f;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 1) {
				orig.vtx_x[0] &= 0x8000ffff;
			} else {
				orig.vtx_x[0] |= 0x7fff0000;
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 1) {
				orig.vtx_y[0] &= 0x8000ffff;
			} else {
				orig.vtx_y[0] |= 0x7fff0000;
			}
		}
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		if (is_out) {
			exp.valid |= 0x2020;
			exp.vtx_x[5] = extr(val, 0, 16);
			exp.vtx_y[5] = extr(val, 16, 16);
		}
		if (is_in) {
			exp.valid |= 0x8008;
			exp.vtx_x[3] = extr(val, 0, 16);
			exp.vtx_y[3] = -extr(val, 16, 16);
			exp.vtx_y[7] = extr(val, 16, 16);
			if (!is_out)
				exp.misc24_0 = extr(val, 0, 16);
			nv03_pgraph_vtx_cmp(&exp, 0, 3, false);
			nv03_pgraph_vtx_cmp(&exp, 1, cls == 0x15 ? 3 : 7, false);
			bool zero = false;
			if (!extr(exp.xy_misc_4[0], 28, 4))
				zero = true;
			if (!extr(exp.xy_misc_4[1], 28, 4))
				zero = true;
			insrt(exp.xy_misc_0, 20, 1, zero);
			if (cls != 0x15) {
				insrt(exp.xy_misc_3, 12, 1, extr(val, 0, 16) < 0x20 && cls != 0x11);
			}
		}
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, cls != 0x15);
		insrt(exp.xy_misc_0, 28, 4, 0);
		insrt(exp.xy_misc_3, 8, 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_itm_rect(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0x14;
		uint32_t mthd = 0x30c;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0x000f000f;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 1) {
				orig.vtx_x[0] &= 0x8000ffff;
			} else {
				orig.vtx_x[0] |= 0x7fff0000;
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 1) {
				orig.vtx_y[0] &= 0x8000ffff;
			} else {
				orig.vtx_y[0] |= 0x7fff0000;
			}
		}
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.vtx_x[3] = extr(val, 0, 16);
		exp.vtx_y[3] = extr(val, 16, 16);
		int vidx = extr(exp.xy_misc_0, 28, 4);
		int nvidx = vidx + 1;
		if (nvidx == 4)
			nvidx = 0;
		insrt(exp.xy_misc_0, 28, 4, nvidx);
		nv03_pgraph_vtx_cmp(&exp, 0, 3, false);
		nv03_pgraph_vtx_cmp(&exp, 1, 3, false);
		bool zero = false;
		if (extr(exp.xy_misc_4[0], 28, 4) < 2)
			zero = true;
		if (extr(exp.xy_misc_4[1], 28, 4) < 2)
			zero = true;
		insrt(exp.xy_misc_0, 20, 1, zero);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 2, exp.vtx_x[0], exp.vtx_x[3], 0, false);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 1, 2, exp.vtx_y[0], exp.vtx_y[3], 0, false);
		exp.misc24_0 = extr(val, 0, 16);
		exp.valid |= 0x404;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifc_diff(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0x15;
		int xy = jrand48(ctx->rand48) & 1;
		uint32_t mthd = 0x308 + xy * 4;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0x000f000f;
		if (!(jrand48(ctx->rand48) & 3))
			val = 0x00100000;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.valid |= 0x20 << xy * 8;
		if (xy)
			exp.vtx_y[5] = val;
		else
			exp.vtx_x[5] = val;
		insrt(exp.xy_misc_0, 28, 4, 0);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifc_vtx(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0x15;
		uint32_t mthd = 0x318;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0x000f000f;
		if (!(jrand48(ctx->rand48) & 3))
			val = 0x00100000;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.valid |= 0x9018;
		exp.vtx_x[4] = extr(val, 0, 16) << 16;
		exp.vtx_y[3] = -exp.vtx_y[7];
		exp.vtx_y[4] = extr(val, 16, 16) << 16;
		insrt(exp.valid, 19, 1, 0);
		insrt(exp.xy_misc_0, 28, 4, 0);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 0);
		int xcstat = nv03_pgraph_clip_status(&ctx->chipset, &exp, extrs(val, 4, 12), 0, false);
		int ycstat = nv03_pgraph_clip_status(&ctx->chipset, &exp, extrs(val, 20, 12), 1, false);
		insrt(exp.xy_clip[0][0], 0, 4, xcstat);
		insrt(exp.xy_clip[1][0], 0, 4, ycstat);
		insrt(exp.xy_misc_3, 8, 1, 0);
		nv03_pgraph_vtx_cmp(&exp, 0, 3, false);
		nv03_pgraph_vtx_cmp(&exp, 1, 3, false);
		insrt(exp.xy_misc_0, 20, 1, 0);
		insrt(exp.xy_misc_4[0], 0, 1, 0);
		insrt(exp.xy_misc_4[0], 4, 1, 0);
		insrt(exp.xy_misc_4[1], 0, 1, 0);
		insrt(exp.xy_misc_4[1], 4, 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifm_vtx(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0xe;
		uint32_t mthd = 0x310;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0x000f000f;
		if (!(jrand48(ctx->rand48) & 3))
			val = 0x00100000;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.valid |= 0x1001;
		exp.vtx_x[0] = extrs(val, 0, 16);
		exp.vtx_y[4] = extrs(val, 16, 16);
		insrt(exp.valid, 19, 1, 0);
		insrt(exp.xy_misc_0, 28, 4, 1);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		int xcstat = nv03_pgraph_clip_status(&ctx->chipset, &exp, exp.vtx_x[0], 0, false);
		int ycstat = nv03_pgraph_clip_status(&ctx->chipset, &exp, exp.vtx_y[4], 1, false);
		insrt(exp.xy_clip[0][0], 0, 4, xcstat);
		insrt(exp.xy_clip[1][0], 0, 4, ycstat);
		insrt(exp.xy_misc_3, 8, 1, 0);
		insrt(exp.xy_misc_4[0], 0, 1, 0);
		insrt(exp.xy_misc_4[0], 4, 1, 0);
		insrt(exp.xy_misc_4[1], 0, 1, 0);
		insrt(exp.xy_misc_4[1], 4, 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifm_rect(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0xe;
		uint32_t mthd = 0x314;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0x000f000f;
		if (!(jrand48(ctx->rand48) & 3))
			val = 0x00100000;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.valid |= 0x202;
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 1, exp.vtx_x[0], extr(val, 0, 16), 0, false);
		exp.vtx_y[1] = extr(val, 16, 16);
		int vtxid = extr(exp.xy_misc_0, 28, 4);
		int nvtxid = (vtxid + 1) & 1;
		insrt(exp.xy_misc_0, 28, 4, nvtxid);
		int xcstat = nv03_pgraph_clip_status(&ctx->chipset, &exp, exp.vtx_x[1], 0, false);
		int ycstat = nv03_pgraph_clip_status(&ctx->chipset, &exp, exp.vtx_y[1], 1, false);
		insrt(exp.xy_clip[0][0], 4, 4, xcstat);
		insrt(exp.xy_clip[1][0], 4, 4, ycstat);
		insrt(exp.xy_misc_4[0], 1, 1, 0);
		insrt(exp.xy_misc_4[0], 5, 1, exp.vtx_x[1] > 0x7fff && exp.vtx_x[1] < 0xffff8000);
		insrt(exp.xy_misc_4[1], 1, 1, 0);
		insrt(exp.xy_misc_4[1], 5, 1, exp.vtx_y[1] > 0x7fff);
		nv03_pgraph_vtx_cmp(&exp, 0, 8, true);
		nv03_pgraph_vtx_cmp(&exp, 1, 8, true);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifm_du_dx(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0xe;
		uint32_t mthd = 0x318;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0x003fffff;
		if (!(jrand48(ctx->rand48) & 3))
			val |= 0xffc00000;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int sv = val >> 17;
		if (sv >= 0x4000) {
			if (sv >= 0x7ff8)
				sv &= 0xf;
			else
				sv = 9;
		} else {
			if (sv > 7)
				sv = 7;
		}
		exp.valid |= 0x10;
		exp.misc32_0 = val;
		insrt(exp.xy_misc_1[0], 24, 4, sv);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifm_dv_dy(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0xe;
		uint32_t mthd = 0x31c;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0x003fffff;
		if (!(jrand48(ctx->rand48) & 3))
			val |= 0xffc00000;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int sv = val >> 17;
		if (sv >= 0x4000) {
			if (sv >= 0x7ff8)
				sv &= 0xf;
			else
				sv = 9;
		} else {
			if (sv > 7)
				sv = 7;
		}
		exp.valid |= 0x20;
		exp.vtx_y[3] = val;
		insrt(exp.xy_misc_1[1], 24, 4, sv);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifm_src_size(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0x0e;
		uint32_t mthd = 0x400;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x0fff0fff;
			val |= 1 << (jrand48(ctx->rand48) & 0x1f);
		}
		if (!(jrand48(ctx->rand48) & 3)) {
			val &= ~0x7f0;
			val |= 0x400;
		}
		if (!(jrand48(ctx->rand48) & 3)) {
			val &= ~0x7f00000;
			val |= 0x4000000;
		}
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int fmt;
		if (orig.grobj == exp.grobj) {
			fmt = extr(orig.ctx_switch, 0, 3);
		} else {
			fmt = extr(exp.ctx_switch, 0, 3);
		}
		exp.misc24_0 = extr(val, 0, 16);
		insrt(exp.misc24_1, 0, 11, extr(val, 0, 11));
		insrt(exp.misc24_1, 11, 11, extr(val, 16, 11));
		if (fmt == 7) {
			insrt(exp.valid, 9, 1, 0);
			insrt(exp.valid, 13, 1, 1);
		} else {
			insrt(exp.valid, 9, 1, 1);
			insrt(exp.valid, 13, 1, 0);
		}
		if (extr(exp.debug[3], 20, 1) && (extr(val, 0, 16) > 0x400 || extr(val, 16, 16) > 0x400)) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_offset(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int cls = 0x17;
		uint32_t mthd = 0x304;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.dma_offset[0] = val;
		exp.valid |= 1 << 23;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int cls = 0x17;
		uint32_t mthd = 0x308;
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xff31ffff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.misc32_0 = val;
		exp.valid |= 1 << 24;
		if (extr(exp.debug[3], 20, 1) && (val & ~0xff31ffff ||
			extr(val, 28, 4) > 0xb || extr(val, 24, 4) > 0xb)) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_filter(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int cls = 0x17;
		uint32_t mthd = 0x30c;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.misc32_1 = val;
		exp.valid |= 1 << 25;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_fog_color(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int cls = 0x17;
		uint32_t mthd = 0x310;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.misc24_0 = val & 0xffffff;
		exp.valid |= 1 << 27;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_config(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		bool is_point = jrand48(ctx->rand48) & 1;
		int cls = is_point ? 0x18 : 0x17;
		uint32_t mthd = is_point ? 0x304 : 0x314;
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf77fbdf3;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.d3d_config = val & 0xf77fbdf3;
		exp.valid |= 1 << 26;
		bool bad;
		if (is_point)
			bad = (val & 0xffff ||
				extr(val, 16, 4) > 8 ||
				extr(val, 20, 3) > 4 ||
				extr(val, 24, 3) > 4);
		else
			bad = (extr(val, 0, 2) > 2 ||
				extr(val, 12, 2) == 0 ||
				extr(val, 16, 4) == 0 ||
				extr(val, 16, 4) > 8 ||
				extr(val, 20, 3) > 4 ||
				extr(val, 24, 3) > 4);
		if (extr(exp.debug[3], 20, 1) && (val & ~0xf77fbdf3 || bad)) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_alpha(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		bool is_point = jrand48(ctx->rand48) & 1;
		int cls = is_point ? 0x18 : 0x17;
		uint32_t mthd = is_point ? 0x308 : 0x318;
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xfff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.d3d_alpha = val & 0xfff;
		bool bad = extr(val, 8, 4) == 0 || extr(val, 8, 4) > 8;
		if (extr(exp.debug[3], 20, 1) && (val & ~0xfff || bad)) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_fog_tri(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int idx = jrand48(ctx->rand48) & 0x7f;
		int cls = 0x17;
		uint32_t mthd = 0x1000 | idx << 5;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.d3d_fog_tri = val;
		exp.valid |= 1 << 16;
		exp.valid &= ~0x007e0000;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_color(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int idx = jrand48(ctx->rand48) & 0x7f;
		int cls = 0x17;
		uint32_t mthd = 0x1004 | idx << 5;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		insrt(exp.d3d_rgb, 0, 24, extr(val, 0, 24));
		insrt(exp.d3d_rgb, 24, 7, extr(val, 25, 7));
		exp.valid |= 1 << 17;
		insrt(exp.valid, extr(exp.d3d_fog_tri, 0, 4), 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_xy(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int idx = jrand48(ctx->rand48) & 0x7f;
		int xy = jrand48(ctx->rand48) & 1;
		int cls = 0x17;
		uint32_t mthd = 0x1008 | xy << 2 | idx << 5;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int e = extr(val, 23, 8);
		bool s = extr(val, 31, 1);
		uint32_t tv = extr(val, 0, 23) | 1 << 23;
		if (e > 0x7f+10) {
			tv = 0x7fff + s;
		} else if (e < 0x7f-4) {
			tv = 0;
		} else {
			tv >>= 0x7f + 10 - e + 24 - 15;
			if (s)
				tv = -tv;
		}
		insrt(exp.d3d_xy, 16*xy, 16, tv);
		exp.valid |= 1 << (21 - xy);
		insrt(exp.valid, extr(exp.d3d_fog_tri, 0, 4), 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_z(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int idx = jrand48(ctx->rand48) & 0x7f;
		int cls = 0x17;
		uint32_t mthd = 0x1010 | idx << 5;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int e = extr(val, 23, 8);
		bool s = extr(val, 31, 1);
		uint32_t tv = extr(val, 0, 23) | 1 << 23;
		if (s) {
			tv = 0;
		} else if (e > 0x7f-1) {
			tv = 0xffffff;
		} else if (e < 0x7f-24) {
			tv = 0;
		} else {
			tv >>= 0x7f-1 - e;
		}
		tv = ~tv;
		exp.d3d_z = extr(tv, 0, 16);
		insrt(exp.d3d_m, 25, 7, extr(tv, 16, 7));
		insrt(exp.d3d_rgb, 31, 1, extr(tv, 23, 1));
		exp.valid |= 1 << 19;
		insrt(exp.valid, extr(exp.d3d_fog_tri, 0, 4), 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_m(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int idx = jrand48(ctx->rand48) & 0x7f;
		int cls = 0x17;
		uint32_t mthd = 0x1014 | idx << 5;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		insrt(exp.d3d_m, 0, 25, extr(val, 6, 25));
		exp.valid |= 1 << 18;
		insrt(exp.valid, extr(exp.d3d_fog_tri, 0, 4), 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_u(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int idx = jrand48(ctx->rand48) & 0x7f;
		int cls = 0x17;
		uint32_t mthd = 0x1018 | idx << 5;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int e = extr(val, 23, 8);
		bool s = extr(val, 31, 1);
		uint32_t tv = extr(val, 0, 23) | 1 << 23;
		int sz = extr(exp.misc32_0, 28, 4);
		e -= (11 - sz);
		if (e > 0x7f-1) {
			tv = 0x7fff + s;
		} else if (e < 0x7f-15) {
			tv = 0;
		} else {
			tv >>= 0x7f - 1 - e + 24 - 15;
			if (s)
				tv = -tv;
		}
		insrt(exp.d3d_uv, 0, 16, tv);
		exp.valid |= 1 << 22;
		insrt(exp.valid, extr(exp.d3d_fog_tri, 0, 4), 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &real)) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_rop_simple(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int idx = jrand48(ctx->rand48) & 0x1f;
		int cls = 0x8;
		uint32_t mthd = 0x0400 | idx << 2;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		uint32_t paddr[4], pixel[4], epixel[4], rpixel[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000100;
		orig.ctx_user &= ~0xe000;
		orig.ctx_switch &= ~0x8000;
		int op = extr(orig.ctx_switch, 24, 5);
		if (!((op >= 0x00 && op <= 0x15) || op == 0x17 || (op >= 0x19 && op <= 0x1a) || op == 0x1d))
			op = 0x17;
		insrt(orig.ctx_switch, 24, 5, op);
		orig.pattern_shape = nrand48(ctx->rand48)%3; /* shape 3 is a rather ugly hole in Karnough map */
		insrt(orig.cliprect_ctrl, 8, 1, 0);
		orig.xy_misc_0 = 0;
		orig.xy_misc_1[0] = 0;
		orig.xy_misc_1[1] = 0;
		orig.xy_misc_3 = 0;
		orig.xy_misc_4[0] = 0;
		orig.xy_misc_4[1] = 0;
		orig.valid = 0x10000;
		orig.surf_offset[0] = 0x000000;
		orig.surf_offset[1] = 0x040000;
		orig.surf_offset[2] = 0x080000;
		orig.surf_offset[3] = 0x0c0000;
		orig.surf_pitch[0] = 0x0400;
		orig.surf_pitch[1] = 0x0400;
		orig.surf_pitch[2] = 0x0400;
		orig.surf_pitch[3] = 0x0400;
		if (op > 0x17)
			orig.surf_format |= 0x2222;
		orig.debug[3] &= ~(1 << 22);
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_min[jrand48(ctx->rand48)&1], 0, 16, extr(val, 0, 8));
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_min[jrand48(ctx->rand48)&1], 16, 16, extr(val, 16, 8));
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_max[jrand48(ctx->rand48)&1], 0, 16, extr(val, 0, 8));
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_max[jrand48(ctx->rand48)&1], 16, 16, extr(val, 16, 8));
		if (jrand48(ctx->rand48)&1) {
			/* it's vanishingly rare for the chroma key to match perfectly by random, so boost the odds */
			uint32_t ckey;
			if ((nv03_pgraph_surf_format(&orig) & 3) == 0 && extr(orig.ctx_switch, 0, 3) == 4) {
				ckey = nv01_pgraph_to_a1r10g10b10(nv03_pgraph_expand_color(orig.ctx_switch & ~0x7, orig.misc32_0));
			} else {
				ckey = nv01_pgraph_to_a1r10g10b10(nv03_pgraph_expand_color(orig.ctx_switch, orig.misc32_0));
			}
			ckey ^= (jrand48(ctx->rand48) & 1) << 30; /* perturb alpha */
			if (jrand48(ctx->rand48)&1) {
				/* perturb it a bit to check which bits have to match */
				ckey ^= 1 << (nrand48(ctx->rand48) % 30);
			}
			orig.chroma = ckey;
		}
		val &= 0x00ff00ff;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		int cpp = nv03_pgraph_cpp(&orig);
		int x = extr(val, 0, 16);
		int y = extr(val, 16, 16);
		for (int j = 0; j < 4; j++) {
			paddr[j] = (x * cpp + y * 0x400 + j * 0x40000);
			pixel[j] = epixel[j] = jrand48(ctx->rand48);
			nva_gwr32(nva_cards[ctx->cnum]->bar1, paddr[j] & ~3, pixel[j]);
		}
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.vtx_x[0] = x;
		exp.vtx_y[0] = y;
		insrt(exp.xy_misc_0, 28, 4, 1);
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		int xcstat = nv03_pgraph_clip_status(&ctx->chipset, &exp, exp.vtx_x[0], 0, false);
		int ycstat = nv03_pgraph_clip_status(&ctx->chipset, &exp, exp.vtx_y[0], 1, false);
		insrt(exp.xy_clip[0][0], 0, 4, xcstat);
		insrt(exp.xy_clip[0][0], 4, 4, xcstat);
		insrt(exp.xy_clip[1][0], 0, 4, ycstat);
		insrt(exp.xy_clip[1][0], 4, 4, ycstat);
		if (nv03_pgraph_cliprect_pass(&exp, x, y)) {
			for (int j = 0; j < 4; j++) {
				if (extr(exp.ctx_switch, 20 + j, 1)) {
					uint32_t src = extr(pixel[j], (paddr[j] & 3) * 8, cpp * 8);
					uint32_t res = nv03_pgraph_solid_rop(&exp, x, y, src);
					insrt(epixel[j], (paddr[j] & 3) * 8, cpp * 8, res);
				}
			}
		}
		nv03_pgraph_dump_state(ctx, &real);
		bool mismatch = false;
		for (int j = 0; j < 4; j++) {
			rpixel[j] = nva_grd32(nva_cards[ctx->cnum]->bar1, paddr[j] & ~3);
			if (rpixel[j] != epixel[j])
			mismatch = true;
		}
		if (nv03_pgraph_cmp_state(&exp, &real) || mismatch) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			for (int j = 0; j < 4; j++) {
				printf("pixel%d: %08x %08x %08x%s\n", j, pixel[j], epixel[j], rpixel[j], epixel[j] == rpixel[j] ? "" : " *");
			}
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_rop_zpoint(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int idx = jrand48(ctx->rand48) & 0xff;
		int cls = 0x18;
		uint32_t mthd = 0x0804 | idx << 3;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		uint32_t paddr[4], pixel[4], epixel[4], rpixel[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000100;
		orig.ctx_user &= ~0xe000;
		orig.ctx_switch &= ~0x8000;
		insrt(orig.cliprect_ctrl, 8, 1, 0);
		orig.xy_misc_0 = 0;
		orig.xy_misc_1[0] = 0;
		orig.xy_misc_1[1] = 0;
		orig.xy_misc_3 = 0;
		orig.xy_misc_4[0] = 0;
		orig.xy_misc_4[1] = 0;
		orig.xy_clip[0][0] = 0;
		orig.xy_clip[1][0] = 0;
		orig.valid = 0x4210101;
		orig.surf_offset[0] = 0x000000;
		orig.surf_offset[1] = 0x040000;
		orig.surf_offset[2] = 0x080000;
		orig.surf_offset[3] = 0x0c0000;
		orig.surf_pitch[0] = 0x0400;
		orig.surf_pitch[1] = 0x0400;
		orig.surf_pitch[2] = 0x0400;
		orig.surf_pitch[3] = 0x0400;
		orig.surf_format = 0x6666;
		if (extr(orig.d3d_config, 20, 3) > 4)
			insrt(orig.d3d_config, 22, 1, 0);
		orig.debug[2] &= 0xffdfffff;
		orig.debug[3] &= 0xfffeffff;
		orig.debug[3] &= 0xfffdffff;
		int x = jrand48(ctx->rand48) & 0xff;
		int y = jrand48(ctx->rand48) & 0xff;
		orig.vtx_x[0] = x;
		orig.vtx_y[0] = y;
		orig.debug[3] &= ~(1 << 22);
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_min[jrand48(ctx->rand48)&1], 0, 16, extr(val, 0, 8));
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_min[jrand48(ctx->rand48)&1], 16, 16, extr(val, 16, 8));
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_max[jrand48(ctx->rand48)&1], 0, 16, extr(val, 0, 8));
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_max[jrand48(ctx->rand48)&1], 16, 16, extr(val, 16, 8));
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		int cpp = nv03_pgraph_cpp(&orig);
		for (int j = 0; j < 4; j++) {
			paddr[j] = (x * cpp + y * 0x400 + j * 0x40000);
			pixel[j] = epixel[j] = jrand48(ctx->rand48);
			nva_gwr32(nva_cards[ctx->cnum]->bar1, paddr[j] & ~3, pixel[j]);
		}
		uint32_t zcur = extr(pixel[3], (paddr[3] & 3) * 8, cpp * 8);
		if (!(jrand48(ctx->rand48) & 3)) {
			insrt(val, 16, 16, zcur);
		}
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		insrt(exp.misc32_1, 16, 16, extr(val, 16, 16));
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 0, exp.vtx_x[0], 1, 0, false);
		uint32_t zeta = extr(val, 16, 16);
		struct nv01_color s = nv03_pgraph_expand_color(exp.ctx_switch, exp.misc32_0);
		uint8_t ra = nv01_pgraph_dither_10to5(s.a << 2, x, y, false) >> 1;
		if (nv03_pgraph_cliprect_pass(&exp, x, y)) {
			bool zeta_test = nv03_pgraph_d3d_cmp(extr(exp.d3d_config, 16, 4), zeta, zcur);
			if (!extr(exp.ctx_switch, 12, 1))
				zeta_test = true;
			bool alpha_test = nv03_pgraph_d3d_cmp(extr(exp.d3d_alpha, 8, 4), extr(exp.misc32_0, 24, 8), extr(exp.d3d_alpha, 0, 8));
			bool alpha_nz = !!ra;
			bool zeta_wr = nv03_pgraph_d3d_wren(extr(exp.d3d_config, 20, 3), zeta_test, alpha_nz);
			bool color_wr = nv03_pgraph_d3d_wren(extr(exp.d3d_config, 24, 3), zeta_test, alpha_nz);
			if (!alpha_test) {
				color_wr = false;
				zeta_wr = false;
			}
			if (extr(exp.ctx_switch, 12, 1) && extr(exp.ctx_switch, 20, 4) && zeta_wr) {
				insrt(epixel[3], (paddr[3] & 3) * 8, cpp * 8, zeta);
			}
			for (int j = 0; j < 4; j++) {
				if (extr(exp.ctx_switch, 20 + j, 1) && color_wr) {
					uint32_t src = extr(pixel[j], (paddr[j] & 3) * 8, cpp * 8);
					uint32_t res = nv03_pgraph_zpoint_rop(&exp, x, y, src);
					insrt(epixel[j], (paddr[j] & 3) * 8, cpp * 8, res);
				}
			}
		}
		nv03_pgraph_dump_state(ctx, &real);
		bool mismatch = false;
		for (int j = 0; j < 4; j++) {
			rpixel[j] = nva_grd32(nva_cards[ctx->cnum]->bar1, paddr[j] & ~3);
			if (rpixel[j] != epixel[j])
			mismatch = true;
		}
		if (nv03_pgraph_cmp_state(&exp, &real) || mismatch) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			for (int j = 0; j < 4; j++) {
				printf("pixel%d: %08x %08x %08x%s\n", j, pixel[j], epixel[j], rpixel[j], epixel[j] == rpixel[j] ? "" : " *");
			}
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_rop_blit(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0x10;
		uint32_t val = 0x00010001;
		uint32_t mthd = 0x0308;
		uint32_t addr = mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		uint32_t spaddr[4], spixel[4];
		uint32_t paddr[4], pixel[4], epixel[4], rpixel[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct nv03_pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000080;
		orig.ctx_user &= ~0xe000;
		orig.ctx_switch &= ~0x8000;
		int op = extr(orig.ctx_switch, 24, 5);
		if (!((op >= 0x00 && op <= 0x15) || op == 0x17 || (op >= 0x19 && op <= 0x1a) || op == 0x1d))
			op = 0x17;
		insrt(orig.ctx_switch, 24, 5, op);
		orig.pattern_shape = nrand48(ctx->rand48)%3; /* shape 3 is a rather ugly hole in Karnough map */
		// XXX: if source pixel hits cliprect, bad things happen
		orig.cliprect_ctrl = 0;
		orig.xy_misc_0 = 0x20000000;
		orig.xy_misc_1[0] = 0;
		orig.xy_misc_1[1] = 0;
		orig.xy_misc_3 = 0;
		orig.xy_misc_4[0] = 0;
		orig.xy_misc_4[1] = 0;
		orig.xy_clip[0][0] = 0;
		orig.xy_clip[1][0] = 0;
		orig.valid = 0x0303;
		orig.surf_offset[0] = 0x000000;
		orig.surf_offset[1] = 0x040000;
		orig.surf_offset[2] = 0x080000;
		orig.surf_offset[3] = 0x0c0000;
		orig.surf_pitch[0] = 0x0400;
		orig.surf_pitch[1] = 0x0400;
		orig.surf_pitch[2] = 0x0400;
		orig.surf_pitch[3] = 0x0400;
		if (op > 0x17)
			orig.surf_format |= 0x2222;
		orig.src_canvas_min = 0x00000080;
		orig.src_canvas_max = 0x01000100;
		int x = jrand48(ctx->rand48) & 0x7f;
		int y = jrand48(ctx->rand48) & 0xff;
		int sx = jrand48(ctx->rand48) & 0xff;
		int sy = jrand48(ctx->rand48) & 0xff;
		orig.vtx_x[0] = sx;
		orig.vtx_y[0] = sy;
		orig.vtx_x[1] = x;
		orig.vtx_y[1] = y;
		orig.debug[3] &= ~(1 << 22);
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.grobj = gctx & 0xffff;
		int cpp = nv03_pgraph_cpp(&orig);
		for (int j = 0; j < 4; j++) {
			spaddr[j] = (sx * cpp + sy * 0x400 + j * 0x40000);
			spixel[j] = jrand48(ctx->rand48);
			if (sx >= 0x80)
				nva_gwr32(nva_cards[ctx->cnum]->bar1, spaddr[j] & ~3, spixel[j]);
		}
		for (int j = 0; j < 4; j++) {
			paddr[j] = (x * cpp + y * 0x400 + j * 0x40000);
			pixel[j] = epixel[j] = jrand48(ctx->rand48);
			nva_gwr32(nva_cards[ctx->cnum]->bar1, paddr[j] & ~3, pixel[j]);
		}
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.valid = 0;
		insrt(exp.xy_misc_0, 28, 4, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 2, exp.vtx_x[0], extr(val, 0, 16), 0, false);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 1, 2, exp.vtx_y[0], extr(val, 16, 16), 0, false);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 3, exp.vtx_x[1], extr(val, 0, 16), 0, false);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 1, 3, exp.vtx_y[1], extr(val, 16, 16), 0, false);
		nv03_pgraph_vtx_cmp(&exp, 0, 8, true);
		nv03_pgraph_vtx_cmp(&exp, 1, 8, true);
		if (nv03_pgraph_cliprect_pass(&exp, x, y)) {
			int fmt = nv03_pgraph_surf_format(&exp) & 3;
			int ss = extr(exp.ctx_switch, 16, 2);
			uint32_t sp = extr(spixel[ss], (spaddr[ss] & 3) * 8, cpp * 8);
			if (sx < 0x80)
				sp = 0;
			if (!nv03_pgraph_cliprect_pass(&exp, sx, sy))
				sp = 0xffffffff;
			struct nv01_color s = nv03_pgraph_expand_surf(fmt, sp);
			for (int j = 0; j < 4; j++) {
				if (extr(exp.ctx_switch, 20 + j, 1)) {
					uint32_t src = extr(pixel[j], (paddr[j] & 3) * 8, cpp * 8);
					uint32_t res = nv03_pgraph_rop(&exp, x, y, src, s);
					insrt(epixel[j], (paddr[j] & 3) * 8, cpp * 8, res);
				}
			}
		}
		nv03_pgraph_dump_state(ctx, &real);
		bool mismatch = false;
		for (int j = 0; j < 4; j++) {
			rpixel[j] = nva_grd32(nva_cards[ctx->cnum]->bar1, paddr[j] & ~3);
			if (rpixel[j] != epixel[j])
			mismatch = true;
		}
		if (nv03_pgraph_cmp_state(&exp, &real) || mismatch) {
			nv03_pgraph_print_states(&orig, &exp, &real);
			for (int j = 0; j < 4; j++) {
				printf("spixel%d: %08x\n", j, spixel[j]);
			}
			for (int j = 0; j < 4; j++) {
				printf("pixel%d: %08x %08x %08x%s\n", j, pixel[j], epixel[j], rpixel[j], epixel[j] == rpixel[j] ? "" : " *");
			}
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int nv03_pgraph_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset != 0x03)
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

static int simple_mthd_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int xy_mthd_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int d3d_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int rop_prep(struct hwtest_ctx *ctx) {
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


HWTEST_DEF_GROUP(simple_mthd,
	HWTEST_TEST(test_mthd_invalid, 0),
	HWTEST_TEST(test_mthd_ctxsw, 0),
	HWTEST_TEST(test_mthd_notify, 0),
	HWTEST_TEST(test_mthd_beta, 0),
	HWTEST_TEST(test_mthd_rop, 0),
	HWTEST_TEST(test_mthd_chroma, 0),
	HWTEST_TEST(test_mthd_plane, 0),
	HWTEST_TEST(test_mthd_pattern_shape, 0),
	HWTEST_TEST(test_mthd_pattern_mono_color, 0),
	HWTEST_TEST(test_mthd_pattern_mono_bitmap, 0),
	HWTEST_TEST(test_mthd_surf_format, 0),
	HWTEST_TEST(test_mthd_surf_pitch, 0),
	HWTEST_TEST(test_mthd_surf_offset, 0),
	HWTEST_TEST(test_mthd_solid_color, 0),
	HWTEST_TEST(test_mthd_m2mf_offset, 0),
	HWTEST_TEST(test_mthd_m2mf_pitch, 0),
	HWTEST_TEST(test_mthd_m2mf_line_length, 0),
	HWTEST_TEST(test_mthd_m2mf_line_count, 0),
	HWTEST_TEST(test_mthd_m2mf_format, 0),
	HWTEST_TEST(test_mthd_sifm_offset, 0),
	HWTEST_TEST(test_mthd_sifm_pitch, 0),
	HWTEST_TEST(test_mthd_sifm_invalid, 0),
	HWTEST_TEST(test_mthd_itm_pitch, 0),
)

HWTEST_DEF_GROUP(xy_mthd,
	HWTEST_TEST(test_mthd_clip, 0),
	HWTEST_TEST(test_mthd_vtx, 0),
	HWTEST_TEST(test_mthd_x32, 0),
	HWTEST_TEST(test_mthd_y32, 0),
	HWTEST_TEST(test_mthd_rect, 0),
	HWTEST_TEST(test_mthd_zpoint_zeta, 0),
	HWTEST_TEST(test_mthd_blit_rect, 0),
	HWTEST_TEST(test_mthd_ifc_size, 0),
	HWTEST_TEST(test_mthd_itm_rect, 0),
	HWTEST_TEST(test_mthd_sifc_diff, 0),
	HWTEST_TEST(test_mthd_sifc_vtx, 0),
	HWTEST_TEST(test_mthd_sifm_vtx, 0),
	HWTEST_TEST(test_mthd_sifm_rect, 0),
	HWTEST_TEST(test_mthd_sifm_du_dx, 0),
	HWTEST_TEST(test_mthd_sifm_dv_dy, 0),
	HWTEST_TEST(test_mthd_sifm_src_size, 0),
)

HWTEST_DEF_GROUP(d3d,
	HWTEST_TEST(test_mthd_d3d_tex_offset, 0),
	HWTEST_TEST(test_mthd_d3d_tex_format, 0),
	HWTEST_TEST(test_mthd_d3d_tex_filter, 0),
	HWTEST_TEST(test_mthd_d3d_fog_color, 0),
	HWTEST_TEST(test_mthd_d3d_config, 0),
	HWTEST_TEST(test_mthd_d3d_alpha, 0),
	HWTEST_TEST(test_mthd_d3d_fog_tri, 0),
	HWTEST_TEST(test_mthd_d3d_color, 0),
	HWTEST_TEST(test_mthd_d3d_xy, 0),
	HWTEST_TEST(test_mthd_d3d_z, 0),
	HWTEST_TEST(test_mthd_d3d_m, 0),
	HWTEST_TEST(test_mthd_d3d_u, 0),
)

HWTEST_DEF_GROUP(rop,
	HWTEST_TEST(test_rop_simple, 0),
	HWTEST_TEST(test_rop_zpoint, 0),
	HWTEST_TEST(test_rop_blit, 0),
)

}

HWTEST_DEF_GROUP(nv03_pgraph,
	HWTEST_GROUP(scan),
	HWTEST_GROUP(state),
	HWTEST_GROUP(simple_mthd),
	HWTEST_GROUP(xy_mthd),
	HWTEST_GROUP(d3d),
	HWTEST_GROUP(rop),
)
