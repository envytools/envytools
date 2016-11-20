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

static void nv03_pgraph_gen_state(struct hwtest_ctx *ctx, struct pgraph_state *state) {
	uint32_t offset_mask = ctx->chipset.is_nv03t ? 0x007fffff : 0x003fffff;
	uint32_t canvas_mask = ctx->chipset.is_nv03t ? 0x7fff07ff : 0x3fff07ff;
	state->chipset = ctx->chipset;
	state->intr = 0;
	state->invalid = 0;
	state->dma_intr = 0;
	state->intr_en = jrand48(ctx->rand48) & 0x11111011;
	state->invalid_en = jrand48(ctx->rand48) & 0x00011111;
	state->dma_intr_en = jrand48(ctx->rand48) & 0x00011111;
	state->ctx_switch[0] = jrand48(ctx->rand48) & 0x3ff3f71f;
	state->ctx_control = jrand48(ctx->rand48) & 0x11010103;
	state->ctx_user = jrand48(ctx->rand48) & 0x7f1fe000;
	for (int i = 0; i < 8; i++)
		state->ctx_cache[i][0] = jrand48(ctx->rand48) & 0x3ff3f71f;
	state->iclip[0] = jrand48(ctx->rand48) & 0x3ffff;
	state->iclip[1] = jrand48(ctx->rand48) & 0x3ffff;
	state->uclip_min[0] = jrand48(ctx->rand48) & 0x3ffff;
	state->uclip_max[0] = jrand48(ctx->rand48) & 0x3ffff;
	state->uclip_min[1] = jrand48(ctx->rand48) & 0x3ffff;
	state->uclip_max[1] = jrand48(ctx->rand48) & 0x3ffff;
	state->oclip_min[0] = jrand48(ctx->rand48) & 0x3ffff;
	state->oclip_max[0] = jrand48(ctx->rand48) & 0x3ffff;
	state->oclip_min[1] = jrand48(ctx->rand48) & 0x3ffff;
	state->oclip_max[1] = jrand48(ctx->rand48) & 0x3ffff;
	for (int i = 0; i < 32; i++) {
		state->vtx_x[i] = jrand48(ctx->rand48);
		state->vtx_y[i] = jrand48(ctx->rand48);
	}
	for (int i = 0; i < 16; i++)
		state->vtx_z[i] = jrand48(ctx->rand48) & 0xffffff;
	for (int i = 0; i < 2; i++) {
		state->pattern_mono_rgb[i] = jrand48(ctx->rand48) & 0x3fffffff;
		state->pattern_mono_a[i] = jrand48(ctx->rand48) & 0xff;
		state->pattern_mono_bitmap[i] = jrand48(ctx->rand48) & 0xffffffff;
	}
	state->pattern_config = jrand48(ctx->rand48) & 3;
	for (int i = 0; i < 4; i++) {
		state->surf_offset[i] = jrand48(ctx->rand48) & offset_mask & ~0xf;
		state->surf_pitch[i] = jrand48(ctx->rand48) & 0x00001ff0;
	}
	state->surf_format = jrand48(ctx->rand48) & 0x7777;
	state->src_canvas_min = jrand48(ctx->rand48) & canvas_mask;
	state->src_canvas_max = jrand48(ctx->rand48) & canvas_mask;
	state->dst_canvas_min = jrand48(ctx->rand48) & canvas_mask;
	state->dst_canvas_max = jrand48(ctx->rand48) & canvas_mask;
	state->ctx_switch[1] = jrand48(ctx->rand48) & 0xffff;
	state->notify = jrand48(ctx->rand48) & 0xf1ffff;
	state->ctx_switch[3] = jrand48(ctx->rand48) & 0xffff;
	state->ctx_switch[2] = jrand48(ctx->rand48) & 0x1ffff;
	state->cliprect_min[0] = jrand48(ctx->rand48) & canvas_mask;
	state->cliprect_min[1] = jrand48(ctx->rand48) & canvas_mask;
	state->cliprect_max[0] = jrand48(ctx->rand48) & canvas_mask;
	state->cliprect_max[1] = jrand48(ctx->rand48) & canvas_mask;
	state->cliprect_ctrl = jrand48(ctx->rand48) & 0x113;
	state->rop = jrand48(ctx->rand48) & 0xff;
	state->chroma = jrand48(ctx->rand48) & 0x7fffffff;
	state->beta = jrand48(ctx->rand48) & 0x7f800000;
	state->bitmap_color_0 = jrand48(ctx->rand48) & 0x7fffffff;
	state->d3d_tlv_xy = jrand48(ctx->rand48);
	state->d3d_tlv_uv[0][0] = jrand48(ctx->rand48);
	state->d3d_tlv_z = jrand48(ctx->rand48) & 0xffff;
	state->d3d_tlv_color = jrand48(ctx->rand48);
	state->d3d_tlv_fog_tri_col1 = jrand48(ctx->rand48);
	state->d3d_tlv_rhw = jrand48(ctx->rand48);
	state->d3d_config = jrand48(ctx->rand48) & 0xf77fbdf3;
	state->d3d_alpha = jrand48(ctx->rand48) & 0xfff;
	state->misc24[0] = jrand48(ctx->rand48) & 0xffffff;
	state->misc24[1] = jrand48(ctx->rand48) & 0xffffff;
	state->misc32[0] = jrand48(ctx->rand48);
	state->misc32[1] = jrand48(ctx->rand48);
	state->valid[0] = jrand48(ctx->rand48);
	state->xy_misc_0 = jrand48(ctx->rand48) & 0xf013ffff;
	state->xy_misc_1[0] = jrand48(ctx->rand48) & 0x0f177331;
	state->xy_misc_1[1] = jrand48(ctx->rand48) & 0x0f177331;
	state->xy_misc_3 = jrand48(ctx->rand48) & 0x7f7f1111;
	state->xy_misc_4[0] = jrand48(ctx->rand48) & 0x300000ff;
	state->xy_misc_4[1] = jrand48(ctx->rand48) & 0x300000ff;
	state->xy_clip[0][0] = jrand48(ctx->rand48);
	state->xy_clip[0][1] = jrand48(ctx->rand48);
	state->xy_clip[1][0] = jrand48(ctx->rand48);
	state->xy_clip[1][1] = jrand48(ctx->rand48);
	state->dma_eng_flags[0] = jrand48(ctx->rand48) & 0x03010fff;
	state->dma_eng_limit[0] = jrand48(ctx->rand48);
	state->dma_eng_pte[0] = jrand48(ctx->rand48) & 0xfffff003;
	state->dma_eng_pte_tag[0] = jrand48(ctx->rand48) & 0xfffff000;
	state->dma_eng_addr_virt_adj[0] = jrand48(ctx->rand48);
	state->dma_eng_addr_phys[0] = jrand48(ctx->rand48);
	state->dma_eng_bytes[0] = jrand48(ctx->rand48) & offset_mask;
	state->dma_eng_inst[0] = jrand48(ctx->rand48) & 0x0000ffff;
	state->dma_eng_lines[0] = jrand48(ctx->rand48) & 0x000007ff;
	state->dma_lin_limit = jrand48(ctx->rand48) & offset_mask;
	state->dma_misc = jrand48(ctx->rand48) & 0x707;
	state->dma_offset[0] = jrand48(ctx->rand48);
	state->dma_offset[1] = jrand48(ctx->rand48);
	state->dma_offset[2] = jrand48(ctx->rand48);
	state->dma_pitch = jrand48(ctx->rand48);
	state->debug[0] = jrand48(ctx->rand48) & 0x13311110;
	state->debug[1] = jrand48(ctx->rand48) & 0x10113301;
	state->debug[2] = jrand48(ctx->rand48) & 0x1133f111;
	state->debug[3] = jrand48(ctx->rand48) & 0x1173ff31;
	state->fifo_enable = jrand48(ctx->rand48) & 1;
	state->status = 0;
	state->trap_addr = nva_rd32(ctx->cnum, 0x4006b4);
	state->trap_data[0] = nva_rd32(ctx->cnum, 0x4006b8);
	state->trap_grctx = nva_rd32(ctx->cnum, 0x4006bc);
}

static void nv03_pgraph_load_state(struct hwtest_ctx *ctx, struct pgraph_state *state) {
	unsigned i;
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
	nva_wr32(ctx->cnum, 0x400140, state->intr_en);
	nva_wr32(ctx->cnum, 0x400144, state->invalid_en);
	nva_wr32(ctx->cnum, 0x401140, state->dma_intr_en);
	nva_wr32(ctx->cnum, 0x400180, state->ctx_switch[0]);
	nva_wr32(ctx->cnum, 0x400190, state->ctx_control);
	nva_wr32(ctx->cnum, 0x400194, state->ctx_user);
	for (int i = 0; i < 8; i++)
		nva_wr32(ctx->cnum, 0x4001a0 + i * 4, state->ctx_cache[i][0]);
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
	for (int i = 0; i < 16; i++)
		nva_wr32(ctx->cnum, 0x400580 + i * 4, state->vtx_z[i]);
	nva_wr32(ctx->cnum, 0x40061c, state->bitmap_color_0);
	nva_wr32(ctx->cnum, 0x400624, state->rop);
	nva_wr32(ctx->cnum, 0x400640, state->beta);
	for (int i = 0; i < 2; i++) {
		nva_wr32(ctx->cnum, 0x400600 + i * 8, state->pattern_mono_rgb[i]);
		nva_wr32(ctx->cnum, 0x400604 + i * 8, state->pattern_mono_a[i]);
		nva_wr32(ctx->cnum, 0x400610 + i * 4, state->pattern_mono_bitmap[i]);
	}
	nva_wr32(ctx->cnum, 0x400618, state->pattern_config);
	nva_wr32(ctx->cnum, 0x40062c, state->chroma);
	nva_wr32(ctx->cnum, 0x400550, state->src_canvas_min);
	nva_wr32(ctx->cnum, 0x400554, state->src_canvas_max);
	nva_wr32(ctx->cnum, 0x400558, state->dst_canvas_min);
	nva_wr32(ctx->cnum, 0x40055c, state->dst_canvas_max);
	for (int i = 0; i < 2; i++) {
		nva_wr32(ctx->cnum, 0x400690 + i * 8, state->cliprect_min[i]);
		nva_wr32(ctx->cnum, 0x400694 + i * 8, state->cliprect_max[i]);
	}
	nva_wr32(ctx->cnum, 0x4006a0, state->cliprect_ctrl);
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
	nva_wr32(ctx->cnum, 0x40050c, state->misc32[0]);
	nva_wr32(ctx->cnum, 0x40054c, state->misc32[1]);
	nva_wr32(ctx->cnum, 0x4005c0, state->d3d_tlv_xy);
	nva_wr32(ctx->cnum, 0x4005c4, state->d3d_tlv_uv[0][0]);
	nva_wr32(ctx->cnum, 0x4005c8, state->d3d_tlv_z);
	nva_wr32(ctx->cnum, 0x4005cc, state->d3d_tlv_color);
	nva_wr32(ctx->cnum, 0x4005d0, state->d3d_tlv_fog_tri_col1);
	nva_wr32(ctx->cnum, 0x4005d4, state->d3d_tlv_rhw);
	nva_wr32(ctx->cnum, 0x400644, state->d3d_config);
	nva_wr32(ctx->cnum, 0x4006c8, state->d3d_alpha);
	nva_wr32(ctx->cnum, 0x400680, state->ctx_switch[1]);
	nva_wr32(ctx->cnum, 0x400684, state->notify);
	nva_wr32(ctx->cnum, 0x400688, state->ctx_switch[3]);
	nva_wr32(ctx->cnum, 0x40068c, state->ctx_switch[2]);
	for (int i = 0; i < 4; i++) {
		nva_wr32(ctx->cnum, 0x400630 + i * 4, state->surf_offset[i]);
		nva_wr32(ctx->cnum, 0x400650 + i * 4, state->surf_pitch[i]);
	}
	nva_wr32(ctx->cnum, 0x4006a8, state->surf_format);
	nva_wr32(ctx->cnum, 0x401210, state->dma_eng_flags[0]);
	nva_wr32(ctx->cnum, 0x401220, state->dma_eng_limit[0]);
	nva_wr32(ctx->cnum, 0x401230, state->dma_eng_pte[0]);
	nva_wr32(ctx->cnum, 0x401240, state->dma_eng_pte_tag[0]);
	nva_wr32(ctx->cnum, 0x401250, state->dma_eng_addr_virt_adj[0]);
	nva_wr32(ctx->cnum, 0x401260, state->dma_eng_addr_phys[0]);
	nva_wr32(ctx->cnum, 0x401270, state->dma_eng_bytes[0]);
	nva_wr32(ctx->cnum, 0x401280, state->dma_eng_inst[0]);
	nva_wr32(ctx->cnum, 0x401290, state->dma_eng_lines[0]);
	nva_wr32(ctx->cnum, 0x401400, state->dma_lin_limit);
	nva_wr32(ctx->cnum, 0x401800, state->dma_offset[0]);
	nva_wr32(ctx->cnum, 0x401810, state->dma_offset[1]);
	nva_wr32(ctx->cnum, 0x401820, state->dma_offset[2]);
	nva_wr32(ctx->cnum, 0x401830, state->dma_pitch);
	nva_wr32(ctx->cnum, 0x401840, state->dma_misc);
	nva_wr32(ctx->cnum, 0x400508, state->valid[0]);
	nva_wr32(ctx->cnum, 0x400080, state->debug[0]);
	nva_wr32(ctx->cnum, 0x400084, state->debug[1]);
	nva_wr32(ctx->cnum, 0x400088, state->debug[2]);
	nva_wr32(ctx->cnum, 0x40008c, state->debug[3]);
	nva_wr32(ctx->cnum, 0x4006a4, state->fifo_enable);
}

static void nv03_pgraph_dump_state(struct hwtest_ctx *ctx, struct pgraph_state *state) {
	int ctr = 0;
	state->chipset = ctx->chipset;
	while ((state->status = nva_rd32(ctx->cnum, 0x4006b0))) {
		ctr++;
		if (ctr > 100000) {
			fprintf(stderr, "PGRAPH locked up [%08x]!\n", state->status);
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
	state->fifo_enable = nva_rd32(ctx->cnum, 0x4006a4);
	nva_wr32(ctx->cnum, 0x4006a4, 0);
	state->trap_addr = nva_rd32(ctx->cnum, 0x4006b4);
	state->trap_data[0] = nva_rd32(ctx->cnum, 0x4006b8);
	state->trap_grctx = nva_rd32(ctx->cnum, 0x4006bc);
	state->intr = nva_rd32(ctx->cnum, 0x400100) & ~0x100;
	state->invalid = nva_rd32(ctx->cnum, 0x400104);
	state->dma_intr = nva_rd32(ctx->cnum, 0x401100);
	state->intr_en = nva_rd32(ctx->cnum, 0x400140);
	state->invalid_en = nva_rd32(ctx->cnum, 0x400144);
	state->dma_intr_en = nva_rd32(ctx->cnum, 0x401140);
	state->ctx_switch[0] = nva_rd32(ctx->cnum, 0x400180);
	state->ctx_control = nva_rd32(ctx->cnum, 0x400190) & ~0x00100000;
	state->ctx_user = nva_rd32(ctx->cnum, 0x400194);
	for (int i = 0; i < 8; i++)
		state->ctx_cache[i][0] = nva_rd32(ctx->cnum, 0x4001a0 + i * 4);
	for (int i = 0; i < 2; i++) {
		state->iclip[i] = nva_rd32(ctx->cnum, 0x400534 + i * 4);
		state->uclip_min[i] = nva_rd32(ctx->cnum, 0x40053c + i * 4);
		state->uclip_max[i] = nva_rd32(ctx->cnum, 0x400544 + i * 4);
		state->oclip_min[i] = nva_rd32(ctx->cnum, 0x400560 + i * 4);
		state->oclip_max[i] = nva_rd32(ctx->cnum, 0x400568 + i * 4);
	}
	for (int i = 0; i < 32; i++) {
		state->vtx_x[i] = nva_rd32(ctx->cnum, 0x400400 + i * 4);
		state->vtx_y[i] = nva_rd32(ctx->cnum, 0x400480 + i * 4);
	}
	for (int i = 0; i < 16; i++) {
		state->vtx_z[i] = nva_rd32(ctx->cnum, 0x400580 + i * 4);
	}
	state->bitmap_color_0 = nva_rd32(ctx->cnum, 0x40061c);
	state->rop = nva_rd32(ctx->cnum, 0x400624);
	state->beta = nva_rd32(ctx->cnum, 0x400640);
	for (int i = 0; i < 2; i++) {
		state->pattern_mono_rgb[i] = nva_rd32(ctx->cnum, 0x400600 + i * 8);
		state->pattern_mono_a[i] = nva_rd32(ctx->cnum, 0x400604 + i * 8);
		state->pattern_mono_bitmap[i] = nva_rd32(ctx->cnum, 0x400610 + i * 4);
	}
	state->pattern_config = nva_rd32(ctx->cnum, 0x400618);
	state->chroma = nva_rd32(ctx->cnum, 0x40062c);
	state->src_canvas_min = nva_rd32(ctx->cnum, 0x400550);
	state->src_canvas_max = nva_rd32(ctx->cnum, 0x400554);
	state->dst_canvas_min = nva_rd32(ctx->cnum, 0x400558);
	state->dst_canvas_max = nva_rd32(ctx->cnum, 0x40055c);
	for (int i = 0; i < 2; i++) {
		state->cliprect_min[i] = nva_rd32(ctx->cnum, 0x400690 + i * 8);
		state->cliprect_max[i] = nva_rd32(ctx->cnum, 0x400694 + i * 8);
	}
	state->cliprect_ctrl = nva_rd32(ctx->cnum, 0x4006a0);
	state->valid[0] = nva_rd32(ctx->cnum, 0x400508);
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
	state->misc32[0] = nva_rd32(ctx->cnum, 0x40050c);
	state->misc32[1] = nva_rd32(ctx->cnum, 0x40054c);
	state->d3d_tlv_xy = nva_rd32(ctx->cnum, 0x4005c0);
	state->d3d_tlv_uv[0][0] = nva_rd32(ctx->cnum, 0x4005c4);
	state->d3d_tlv_z = nva_rd32(ctx->cnum, 0x4005c8);
	state->d3d_tlv_color = nva_rd32(ctx->cnum, 0x4005cc);
	state->d3d_tlv_fog_tri_col1 = nva_rd32(ctx->cnum, 0x4005d0);
	state->d3d_tlv_rhw = nva_rd32(ctx->cnum, 0x4005d4);
	state->d3d_config = nva_rd32(ctx->cnum, 0x400644);
	state->d3d_alpha = nva_rd32(ctx->cnum, 0x4006c8);
	state->ctx_switch[1] = nva_rd32(ctx->cnum, 0x400680);
	state->notify = nva_rd32(ctx->cnum, 0x400684);
	state->ctx_switch[3] = nva_rd32(ctx->cnum, 0x400688);
	state->ctx_switch[2] = nva_rd32(ctx->cnum, 0x40068c);
	for (int i = 0; i < 4; i++) {
		state->surf_offset[i] = nva_rd32(ctx->cnum, 0x400630 + i * 4);
		state->surf_pitch[i] = nva_rd32(ctx->cnum, 0x400650 + i * 4);
	}
	state->surf_format = nva_rd32(ctx->cnum, 0x4006a8);
	state->dma_eng_flags[0] = nva_rd32(ctx->cnum, 0x401210);
	state->dma_eng_limit[0] = nva_rd32(ctx->cnum, 0x401220);
	state->dma_eng_pte[0] = nva_rd32(ctx->cnum, 0x401230);
	state->dma_eng_pte_tag[0] = nva_rd32(ctx->cnum, 0x401240);
	state->dma_eng_addr_virt_adj[0] = nva_rd32(ctx->cnum, 0x401250);
	state->dma_eng_addr_phys[0] = nva_rd32(ctx->cnum, 0x401260);
	state->dma_eng_bytes[0] = nva_rd32(ctx->cnum, 0x401270);
	state->dma_eng_inst[0] = nva_rd32(ctx->cnum, 0x401280);
	state->dma_eng_lines[0] = nva_rd32(ctx->cnum, 0x401290);
	state->dma_lin_limit = nva_rd32(ctx->cnum, 0x401400);
	state->dma_offset[0] = nva_rd32(ctx->cnum, 0x401800);
	state->dma_offset[1] = nva_rd32(ctx->cnum, 0x401810);
	state->dma_offset[2] = nva_rd32(ctx->cnum, 0x401820);
	state->dma_pitch = nva_rd32(ctx->cnum, 0x401830);
	state->dma_misc = nva_rd32(ctx->cnum, 0x401840);
	state->debug[0] = nva_rd32(ctx->cnum, 0x400080);
	state->debug[1] = nva_rd32(ctx->cnum, 0x400084);
	state->debug[2] = nva_rd32(ctx->cnum, 0x400088);
	state->debug[3] = nva_rd32(ctx->cnum, 0x40008c);
}

static int nv03_pgraph_cmp_state(struct pgraph_state *orig, struct pgraph_state *exp, struct pgraph_state *real, bool broke = false) {
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
	CMP(trap_grctx, "TRAP_GRCTX")
#endif
	CMP(debug[0], "DEBUG[0]")
	CMP(debug[1], "DEBUG[1]")
	CMP(debug[2], "DEBUG[2]")
	CMP(debug[3], "DEBUG[3]")
	CMP(intr, "INTR")
	CMP(intr_en, "INTR_EN")
	CMP(invalid, "INVALID")
	CMP(invalid_en, "INVALID_EN")
	CMP(dma_intr, "DMA_INTR")
	CMP(dma_intr_en, "DMA_INTR_EN")
	CMP(fifo_enable, "FIFO_ENABLE")
	CMP(ctx_switch[0], "CTX_SWITCH[0]")
	CMP(ctx_switch[1], "CTX_SWITCH[1]")
	CMP(ctx_switch[2], "CTX_SWITCH[2]")
	CMP(ctx_switch[3], "CTX_SWITCH[3]")
	CMP(notify, "NOTIFY")
	CMP(ctx_control, "CTX_CONTROL")
	CMP(ctx_user, "CTX_USER")
	for (int i = 0; i < 8; i++) {
		CMP(ctx_cache[i][0], "CTX_CACHE[%d][0]", i)
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
	for (int i = 0; i < 32; i++) {
		CMP(vtx_x[i], "VTX_X[%d]", i)
		CMP(vtx_y[i], "VTX_Y[%d]", i)
	}
	for (int i = 0; i < 16; i++) {
		CMP(vtx_z[i], "VTX_Z[%d]", i)
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
	CMP(valid[0], "VALID[0]")
	CMP(misc24[0], "MISC24[0]")
	CMP(misc24[1], "MISC24[1]")
	CMP(misc32[0], "MISC32[0]")
	CMP(misc32[1], "MISC32[1]")
	CMP(pattern_mono_rgb[0], "PATTERN_MONO_RGB[0]")
	CMP(pattern_mono_a[0], "PATTERN_MONO_A[0]")
	CMP(pattern_mono_rgb[1], "PATTERN_MONO_RGB[1]")
	CMP(pattern_mono_a[1], "PATTERN_MONO_A[1]")
	CMP(pattern_mono_bitmap[0], "PATTERN_MONO_BITMAP[0]")
	CMP(pattern_mono_bitmap[1], "PATTERN_MONO_BITMAP[1]")
	CMP(pattern_config, "PATTERN_CONFIG")
	CMP(bitmap_color_0, "BITMAP_COLOR_0")
	CMP(rop, "ROP")
	CMP(beta, "BETA")
	CMP(chroma, "CHROMA")
	CMP(src_canvas_min, "SRC_CANVAS_MIN")
	CMP(src_canvas_max, "SRC_CANVAS_MAX")
	CMP(dst_canvas_min, "DST_CANVAS_MIN")
	CMP(dst_canvas_max, "DST_CANVAS_MAX")
	for (int i = 0; i < 2; i++) {
		CMP(cliprect_min[i], "CLIPRECT_MIN[%d]", i)
		CMP(cliprect_max[i], "CLIPRECT_MAX[%d]", i)
	}
	CMP(cliprect_ctrl, "CLIPRECT_CTRL")
	CMP(d3d_tlv_xy, "D3D_TLV_XY")
	CMP(d3d_tlv_uv[0][0], "D3D_TLV_UV[0][0]")
	CMP(d3d_tlv_z, "D3D_TLV_Z")
	CMP(d3d_tlv_color, "D3D_TLV_COLOR")
	CMP(d3d_tlv_fog_tri_col1, "D3D_TLV_FOG_TRI_COL1")
	CMP(d3d_tlv_rhw, "D3D_TLV_RHW")
	CMP(d3d_config, "D3D_CONFIG")
	CMP(d3d_alpha, "D3D_ALPHA")
	for (int i = 0; i < 4; i++) {
		CMP(surf_pitch[i], "SURF_PITCH[%d]", i)
		CMP(surf_offset[i], "SURF_OFFSET[%d]", i)
	}
	CMP(surf_format, "SURF_FORMAT")
	CMP(dma_eng_inst[0], "DMA_ENG_INST[0]")
	CMP(dma_eng_flags[0], "DMA_ENG_FLAGS[0]")
	CMP(dma_eng_limit[0], "DMA_ENG_LIMIT[0]")
	CMP(dma_eng_pte[0], "DMA_ENG_PTE[0]")
	CMP(dma_eng_pte_tag[0], "DMA_ENG_PTE_TAG[0]")
	CMP(dma_eng_addr_virt_adj[0], "DMA_ENG_ADDR_VIRT_ADJ[0]")
	CMP(dma_eng_addr_phys[0], "DMA_ENG_ADDR_PHYS[0]")
	CMP(dma_eng_bytes[0], "DMA_ENG_BYTES[0]")
	CMP(dma_eng_lines[0], "DMA_ENG_LINES[0]")
	CMP(dma_lin_limit, "DMA_LIN_LIMIT")
	CMP(dma_pitch, "DMA_PITCH")
	CMP(dma_offset[0], "DMA_OFFSET[0]")
	CMP(dma_offset[1], "DMA_OFFSET[1]")
	CMP(dma_offset[2], "DMA_OFFSET[2]")
	CMP(dma_misc, "DMA_MISC")
	if (broke && !print) {
		print = true;
		goto restart;
	}
	return broke;
}

static void nv03_pgraph_prep_mthd(struct pgraph_state *state, uint32_t *pgctx, int cls, uint32_t addr) {
	state->fifo_enable = 1;
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

static void nv03_pgraph_reset(struct pgraph_state *state) {
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
	state->misc24[0] = 0;
	state->misc24[1] = 0;
	state->valid[0] = 0;
	state->xy_clip[0][0] = 0x55555555;
	state->xy_clip[0][1] = 0x55555555;
	state->xy_clip[1][0] = 0x55555555;
	state->xy_clip[1][1] = 0x55555555;
}

static void nv03_pgraph_volatile_reset(struct pgraph_state *state) {
	state->bitmap_color_0 &= 0x3fffffff;
	state->xy_misc_0 = 0;
	state->xy_misc_1[0] &= ~0x001440ff;
	state->xy_misc_1[1] &= ~0x001440fe;
	state->xy_misc_3 &= ~0x00000011;
	state->xy_misc_4[0] = 0;
	state->xy_misc_4[1] = 0;
	state->valid[0] &= 0xf0000000;
	state->misc32[0] &= 0x00ff00ff;
	state->d3d_alpha = 0x800;
	state->xy_clip[0][0] = 0x55555555;
	state->xy_clip[0][1] = 0x55555555;
	state->xy_clip[1][0] = 0x55555555;
	state->xy_clip[1][1] = 0x55555555;
}

static void nv03_pgraph_mthd(struct hwtest_ctx *ctx, struct pgraph_state *state, uint32_t *grobj, uint32_t gctx, uint32_t addr, uint32_t val) {
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
			state->ctx_cache[addr >> 13 & 7][0] = grobj[0] & 0x3ff3f71f;
		state->ctx_user &= ~0x1fe000;
		state->ctx_user |= gctx & 0x1f0000;
		state->ctx_user |= addr & 0xe000;
		if (extr(state->debug[1], 20, 1))
			state->ctx_switch[0] = state->ctx_cache[addr >> 13 & 7][0];
		if (extr(state->debug[2], 28, 1)) {
			nv03_pgraph_volatile_reset(state);
			insrt(state->debug[1], 0, 1, 1);
		} else {
			insrt(state->debug[1], 0, 1, 0);
		}
		if (extr(state->notify, 16, 1)) {
			state->intr |= 1;
			state->invalid |= 0x10000;
			state->fifo_enable = 0;
		}
	}
	if (!state->invalid && extr(state->debug[3], 20, 2) == 3 && extr(addr, 0, 13)) {
		state->invalid |= 0x10;
		state->intr |= 0x1;
		state->fifo_enable = 0;
	}
	int ncls = extr(gctx, 16, 5);
	if (extr(state->debug[1], 16, 1) && (gctx & 0xffff) != state->ctx_switch[3] &&
		(ncls == 0x0d || ncls == 0x0e || ncls == 0x14 || ncls == 0x17 ||
		 extr(addr, 0, 13) == 0x104)) {
		state->ctx_switch[3] = gctx & 0xffff;
		state->ctx_switch[1] = grobj[1] & 0xffff;
		state->notify &= 0xf10000;
		state->notify |= grobj[1] >> 16 & 0xffff;
		state->ctx_switch[2] = grobj[2] & 0x1ffff;
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
		struct pgraph_state orig, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_load_state(ctx, &orig);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &orig, &real)) {
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_soft_reset(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x400080, exp.debug[0] | 1);
		nv03_pgraph_reset(&exp);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iteration %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_read(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		struct pgraph_state exp, real;
		uint32_t reg = 0x400000 | (jrand48(ctx->rand48) & 0x1ffc);
		nv03_pgraph_gen_state(ctx, &exp);
		nv03_pgraph_load_state(ctx, &exp);
		nva_rd32(ctx->cnum, reg);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&exp, &exp, &real)) {
			printf("After reading %08x\n", reg);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_write(struct hwtest_ctx *ctx) {
	uint32_t offset_mask = ctx->chipset.is_nv03t ? 0x007fffff : 0x003fffff;
	uint32_t canvas_mask = ctx->chipset.is_nv03t ? 0x7fff07ff : 0x3fff07ff;
	for (int i = 0; i < 100000; i++) {
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_load_state(ctx, &orig);
		uint32_t reg;
		uint32_t val = jrand48(ctx->rand48);
		int idx;
		switch (nrand48(ctx->rand48) % 69) {
			default:
				reg = 0x400140;
				exp.intr_en = val & 0x11111111;
				break;
			case 1:
				reg = 0x400144;
				exp.invalid_en = val & 0x11111;
				break;
			case 2:
				reg = 0x401140;
				exp.dma_intr_en = val & 0x11111;
				break;
			case 3:
				reg = 0x400180;
				exp.ctx_switch[0] = val & 0x3ff3f71f;
				insrt(exp.debug[1], 0, 1, extr(val, 31, 1) && extr(exp.debug[2], 28, 1));
				if (extr(exp.debug[1], 0, 1))
					nv03_pgraph_volatile_reset(&exp);
				break;
			case 4:
				reg = 0x400190;
				exp.ctx_control = val & 0x11010103;
				break;
			case 5:
				reg = 0x400194;
				exp.ctx_user = val & 0x7f1fe000;
				break;
			case 6:
				idx = jrand48(ctx->rand48) & 7;
				reg = 0x4001a0 + idx * 4;
				if (!exp.fifo_enable)
					exp.ctx_cache[idx][0] = val & 0x3ff3f71f;
				break;
			case 7:
				idx = jrand48(ctx->rand48) & 0x1f;
				reg = 0x400400 + idx * 4;
				exp.vtx_x[idx] = val;
				nv03_pgraph_vtx_fixup(&ctx->chipset, &exp, 0, 8, val);
				break;
			case 8:
				idx = jrand48(ctx->rand48) & 0x1f;
				reg = 0x400480 + idx * 4;
				exp.vtx_y[idx] = val;
				nv03_pgraph_vtx_fixup(&ctx->chipset, &exp, 1, 8, val);
				break;
			case 9:
				idx = jrand48(ctx->rand48) & 0xf;
				reg = 0x400580 + idx * 4;
				exp.vtx_z[idx] = val & 0xffffff;
				break;
			case 10:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400534 + idx * 4;
				insrt(exp.xy_misc_1[0], 14, 1, 0);
				insrt(exp.xy_misc_1[0], 18, 1, 0);
				insrt(exp.xy_misc_1[0], 20, 1, 0);
				insrt(exp.xy_misc_1[1], 14, 1, 0);
				insrt(exp.xy_misc_1[1], 18, 1, 0);
				insrt(exp.xy_misc_1[1], 20, 1, 0);
				nv03_pgraph_iclip_fixup(&ctx->chipset, &exp, idx, val);
				break;
			case 11:
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x40053c + idx * 4;
				nv03_pgraph_uclip_fixup(&ctx->chipset, &exp, 0, idx & 1, idx >> 1, val);
				break;
			case 12:
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x400560 + idx * 4;
				nv03_pgraph_uclip_fixup(&ctx->chipset, &exp, 1, idx & 1, idx >> 1, val);
				break;
			case 13:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400600 + idx * 8;
				exp.pattern_mono_rgb[idx] = val & 0x3fffffff;
				break;
			case 14:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400604 + idx * 8;
				exp.pattern_mono_a[idx] = val & 0xff;
				break;
			case 15:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400610 + idx * 4;
				exp.pattern_mono_bitmap[idx] = val;
				break;
			case 16:
				reg = 0x400618;
				exp.pattern_config = val & 3;
				break;
			case 17:
				reg = 0x40061c;
				exp.bitmap_color_0 = val & 0x7fffffff;
				break;
			case 18:
				reg = 0x400624;
				exp.rop = val & 0xff;
				break;
			case 19:
				reg = 0x40062c;
				exp.chroma = val & 0x7fffffff;
				break;
			case 20:
				reg = 0x400640;
				exp.beta = val & 0x7f800000;
				if (val & 0x80000000)
					exp.beta = 0;
				break;
			case 21:
				reg = 0x400550;
				exp.src_canvas_min = val & canvas_mask;
				break;
			case 22:
				reg = 0x400554;
				exp.src_canvas_max = val & canvas_mask;
				break;
			case 23:
				reg = 0x400558;
				exp.dst_canvas_min = val & canvas_mask;
				break;
			case 24:
				reg = 0x40055c;
				exp.dst_canvas_max = val & canvas_mask;
				break;
			case 25:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400690 + idx * 8;
				exp.cliprect_min[idx] = val & canvas_mask;
				break;
			case 26:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400694 + idx * 8;
				exp.cliprect_max[idx] = val & canvas_mask;
				break;
			case 27:
				reg = 0x4006a0;
				exp.cliprect_ctrl = val & 0x113;
				break;
			case 28:
				reg = 0x400514;
				exp.xy_misc_0 = val & 0xf013ffff;
				break;
			case 29:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400518 + idx * 4;
				exp.xy_misc_1[idx] = val & 0x0f177331;
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
				exp.valid[0] = val;
				break;
			case 34:
				idx = jrand48(ctx->rand48) & 1;
				reg = ((uint32_t[2]){0x400510, 0x400570})[idx];
				exp.misc24[idx] = val & 0xffffff;
				break;
			case 35:
				idx = jrand48(ctx->rand48) & 1;
				reg = ((uint32_t[2]){0x40050c, 0x40054c})[idx];
				exp.misc32[idx] = val;
				if (idx == 0)
					exp.valid[0] |= 1 << 16;
				break;
			case 36:
				reg = 0x4005c0;
				exp.d3d_tlv_xy = val;
				break;
			case 37:
				reg = 0x4005c4;
				exp.d3d_tlv_uv[0][0] = val;
				break;
			case 38:
				reg = 0x4005c8;
				exp.d3d_tlv_z = val & 0xffff;
				break;
			case 39:
				reg = 0x4005cc;
				exp.d3d_tlv_color = val;
				break;
			case 40:
				reg = 0x4005d0;
				exp.d3d_tlv_fog_tri_col1 = val;
				break;
			case 41:
				reg = 0x4005d4;
				exp.d3d_tlv_rhw = val;
				break;
			case 42:
				reg = 0x400644;
				exp.d3d_config = val & 0xf77fbdf3;
				insrt(exp.valid[0], 26, 1, 1);
				break;
			case 43:
				reg = 0x4006c8;
				exp.d3d_alpha = val & 0xfff;
				break;
			case 44:
				reg = 0x400680;
				exp.ctx_switch[1] = val & 0xffff;
				break;
			case 45:
				reg = 0x400684;
				exp.notify = val & 0xf1ffff;
				break;
			case 46:
				reg = 0x400688;
				exp.ctx_switch[3] = val & 0xffff;
				break;
			case 47:
				reg = 0x40068c;
				exp.ctx_switch[2] = val & 0x1ffff;
				break;
			case 48:
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x400630 + idx * 4;
				exp.surf_offset[idx] = val & offset_mask & ~0xf;
				break;
			case 49:
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x400650 + idx * 4;
				exp.surf_pitch[idx] = val & 0x1ff0;
				break;
			case 50:
				reg = 0x4006a8;
				exp.surf_format = val & 0x7777;
				break;
			case 51:
				reg = 0x401210;
				exp.dma_eng_flags[0] = val & 0x03010fff;
				break;
			case 52:
				reg = 0x401220;
				exp.dma_eng_limit[0] = val;
				break;
			case 53:
				reg = 0x401230;
				exp.dma_eng_pte[0] = val & 0xfffff003;
				break;
			case 54:
				reg = 0x401240;
				exp.dma_eng_pte_tag[0] = val & 0xfffff000;
				break;
			case 55:
				reg = 0x401250;
				exp.dma_eng_addr_virt_adj[0] = val;
				break;
			case 56:
				reg = 0x401260;
				exp.dma_eng_addr_phys[0] = val;
				break;
			case 57:
				reg = 0x401270;
				exp.dma_eng_bytes[0] = val & offset_mask;
				break;
			case 58:
				reg = 0x401280;
				exp.dma_eng_inst[0] = val & 0xffff;
				break;
			case 59:
				reg = 0x401290;
				exp.dma_eng_lines[0] = val & 0x7ff;
				break;
			case 60:
				reg = 0x401400;
				exp.dma_lin_limit = val & offset_mask;
				break;
			case 61:
				idx = nrand48(ctx->rand48) % 3;
				reg = 0x401800 + idx * 0x10;
				exp.dma_offset[idx] = val;
				break;
			case 62:
				reg = 0x401830;
				exp.dma_pitch = val;
				break;
			case 63:
				reg = 0x401840;
				exp.dma_misc = val & 0x707;
				break;
			case 64:
				reg = 0x4006a4;
				exp.fifo_enable = val & 1;
				break;
			case 65:
				reg = 0x400080;
				exp.debug[0] = val & 0x13311110;
				if (extr(val, 0, 1))
					nv03_pgraph_reset(&exp);
				break;
			case 66:
				reg = 0x400084;
				exp.debug[1] = val & 0x10113301;
				if (extr(val, 4, 1))
					insrt(exp.xy_misc_1[0], 0, 1, 0);
				break;
			case 67:
				reg = 0x400088;
				exp.debug[2] = val & 0x1133f111;
				break;
			case 68:
				reg = 0x40008c;
				exp.debug[3] = val & 0x1173ff31;
				break;
		}
		nva_wr32(ctx->cnum, reg, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d wrote %08x to %06x\n", i, val, reg);
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = 0x400400 + idx * 4 + xy * 0x80;
		uint32_t val = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, reg, val);
		(xy ? exp.vtx_y : exp.vtx_x)[idx] = val;
		nv03_pgraph_vtx_fixup(&ctx->chipset, &exp, xy, 8, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
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
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = 0x40053c + xy * 4 + idx * 8;
		uint32_t val = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, reg, val);
		nv03_pgraph_uclip_fixup(&ctx->chipset, &exp, 0, xy, idx, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = 0x400560 + xy * 4 + idx * 8;
		uint32_t val = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, reg, val);
		nv03_pgraph_uclip_fixup(&ctx->chipset, &exp, 1, xy, idx, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		if (!extr(exp.invalid, 16, 1)) {
			if (val & ~0xf && extr(exp.debug[3], 20, 1)) {
				exp.intr |= 1;
				exp.invalid |= 0x10;
				exp.fifo_enable = 0;
			}
			if (extr(exp.notify, 16, 1)) {
				exp.intr |= 1;
				exp.invalid |= 0x1000;
				exp.fifo_enable = 0;
			}
		}
		if (!extr(exp.invalid, 16, 1) && !extr(exp.invalid, 4, 1)) {
			insrt(exp.notify, 16, 1, 1);
			insrt(exp.notify, 20, 4, val);
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		if (!extr(exp.invalid, 16, 1)) {
			exp.intr |= 1;
			exp.invalid |= 1;
			exp.fifo_enable = 0;
			if (!extr(exp.invalid, 4, 1)) {
				if (extr(exp.notify, 16, 1) && extr(exp.notify, 20, 4)) {
					exp.intr |= 1 << 28;
				}
			}
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x01, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.beta = val;
		if (exp.beta & 0x80000000)
			exp.beta = 0;
		exp.beta &= 0x7f800000;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x02, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.rop = val & 0xff;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x03, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.chroma = nv01_pgraph_to_a1r10g10b10(nv03_pgraph_expand_color(exp.ctx_switch[0], val));
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x04, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x06, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.pattern_config = val & 3;
		if (val > 2 && extr(exp.debug[3], 20, 1)) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.fifo_enable = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x06, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		struct nv01_color c = nv03_pgraph_expand_color(exp.ctx_switch[0], val);
		exp.pattern_mono_rgb[idx] = c.r << 20 | c.g << 10 | c.b;
		exp.pattern_mono_a[idx] = c.a;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x06, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		// yup, hw bug.
		exp.pattern_mono_bitmap[idx] = nv03_pgraph_expand_mono(orig.ctx_switch[0], val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
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
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int s = extr(exp.ctx_switch[0], 16, 2);
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
			exp.fifo_enable = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x1c, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int s = extr(exp.ctx_switch[0], 16, 2);
		exp.surf_pitch[s] = val & 0x1ff0;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, 0x1c, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int s = extr(exp.ctx_switch[0], 16, 2);
		exp.surf_offset[s] = val & offset_mask;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		switch (which) {
			case 0:
				exp.misc32[0] = val;
				exp.valid[0] |= 0x10000;
				break;
			case 1:
				exp.bitmap_color_0 = nv01_pgraph_to_a1r10g10b10(nv03_pgraph_expand_color(exp.ctx_switch[0], val));
				exp.valid[0] |= 0x20000;
				break;
			case 2:
				exp.misc32[1] = val;
				exp.valid[0] |= 0x40000;
				break;
			default:
				abort();
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.dma_offset[idx] = val;
		exp.valid[0] |= 1 << idx;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		insrt(exp.dma_pitch, 16 * idx, 16, val);
		exp.valid[0] |= 1 << (idx + 2);
		if (extr(exp.debug[3], 20, 1) && val & ~0x7fff) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.fifo_enable = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffffff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.misc24[0] = val & 0xffffff;
		exp.valid[0] |= 1 << 4;
		if (extr(exp.debug[3], 20, 1) && val > 0x3fffff) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.fifo_enable = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xfff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.dma_eng_lines[0] = val & 0x7ff;
		exp.valid[0] |= 1 << 5;
		if (extr(exp.debug[3], 20, 1) && val > 0x7ff) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.fifo_enable = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= ~0xf8;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.dma_misc = val & 0x707;
		exp.valid[0] |= 1 << 6;
		int a = extr(val, 0, 3);
		int b = extr(val, 8, 3);
		if (extr(exp.debug[3], 20, 1) && (val & 0xf8 ||
			!(a == 1 || a == 2 || a == 4) ||
			!(b == 1 || b == 2 || b == 4))) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.fifo_enable = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x4006a4, 0);
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.dma_offset[idx] = val;
		int fmt;
		if (orig.ctx_switch[3] == exp.ctx_switch[3]) {
			fmt = extr(orig.ctx_switch[0], 0, 3);
		} else {
			fmt = extr(exp.ctx_switch[0], 0, 3);
		}
		if (fmt == 7) {
			exp.valid[0] |= 1 << (15 + idx);
			exp.valid[0] &= ~0x800;
		} else {
			if (idx)
				continue;
			exp.valid[0] |= 1 << 11;
			exp.valid[0] &= ~0x00038000;
		}
		nva_wr32(ctx->cnum, 0x4006a4, 1);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int fmt;
		if (orig.ctx_switch[3] == exp.ctx_switch[3]) {
			fmt = extr(orig.ctx_switch[0], 0, 3);
		} else {
			fmt = extr(exp.ctx_switch[0], 0, 3);
		}
		if (fmt == 7) {
			exp.dma_pitch = val;
			exp.valid[0] |= 1 << 14;
			exp.valid[0] &= ~0x400;
		} else {
			insrt(exp.dma_pitch, 0, 16, val);
			exp.valid[0] |= 1 << 10;
			exp.valid[0] &= ~0x00004000;
			if (extr(exp.debug[3], 20, 1) && val & ~0x1fff) {
				exp.intr |= 1;
				exp.invalid |= 0x10;
				exp.fifo_enable = 0;
			}
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x4006a4, 0);
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int fmt;
		if (orig.ctx_switch[3] == exp.ctx_switch[3]) {
			fmt = extr(orig.ctx_switch[0], 0, 3);
		} else {
			fmt = extr(exp.ctx_switch[0], 0, 3);
		}
		if (fmt == 7)
			continue;
		nva_wr32(ctx->cnum, 0x4006a4, 1);
		exp.intr |= 1;
		exp.invalid |= 1;
		exp.fifo_enable = 0;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		insrt(exp.dma_pitch, 0, 16, val);
		exp.valid[0] |= 1 << 10;
		exp.valid[0] &= ~0x4000;
		if (extr(exp.debug[3], 20, 1) && val & ~0x1fff) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.fifo_enable = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1) {
			orig.vtx_x[8] &= 0x8000000f;
			orig.vtx_y[8] &= 0x8000000f;
		}
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_x[13], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
			insrt(orig.vtx_y[13], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
		}
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		nv03_pgraph_set_clip(&ctx->chipset, &exp, which, idx, val, extr(orig.xy_misc_1[0], 0, 1));
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1) {
			orig.valid[0] &= ~0xf0f;
			orig.valid[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
			orig.valid[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
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
				orig.valid[0] = 0x0fffffff;
				if (jrand48(ctx->rand48) & 1) {
					orig.valid[0] &= ~0xf0f;
				}
				orig.valid[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
				orig.valid[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				insrt(orig.ctx_switch[0], 24, 5, 0x17);
				insrt(orig.ctx_switch[0], 13, 2, 0);
				int j;
				for (j = 0; j < 8; j++) {
					insrt(orig.ctx_cache[j][0], 24, 5, 0x17);
					insrt(orig.ctx_cache[j][0], 13, 2, 0);
				}
			}
		}
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int vidx;
		if (first) {
			vidx = 0;
			if (cls >= 9 && cls <= 0xb) {
				exp.valid[0] &= ~0xffff;
				insrt(exp.valid[0], 21, 1, 1);
			}
			if (cls == 0x18)
				insrt(exp.valid[0], 21, 1, 1);
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
		if (poly && (exp.valid[0] & 0xf0f))
			insrt(exp.valid[0], 21, 1, 0);
		if (!poly) {
			insrt(exp.valid[0], rvidx, 1, 1);
			insrt(exp.valid[0], 8|rvidx, 1, 1);
		}
		if ((cls >= 9 && cls <= 0xb)) {
			insrt(exp.valid[0], 4|svidx, 1, 1);
			insrt(exp.valid[0], 0xc|svidx, 1, 1);
		}
		if (cls == 0x10) {
			insrt(exp.valid[0], vidx, 1, 1);
			insrt(exp.valid[0], vidx|8, 1, 1);
		}
		if (cls != 0xc || first)
			insrt(exp.valid[0], 19, 1, noclip);
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
		if (real.status && draw && (cls == 0xc || cls == 0xb || cls == 9 || cls == 0xa)) {
			/* Hung PGRAPH... */
			continue;
		}
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1) {
			orig.valid[0] &= ~(jrand48(ctx->rand48) & 0x00000f0f);
			orig.valid[0] &= ~(jrand48(ctx->rand48) & 0x00000f0f);
		}
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int vidx;
		if (first) {
			vidx = 0;
			if (cls >= 9 && cls <= 0xb) {
				exp.valid[0] &= ~0xffff;
				insrt(exp.valid[0], 21, 1, 1);
			}
			if (cls == 0x18)
				insrt(exp.valid[0], 21, 1, 1);
		} else {
			vidx = extr(exp.xy_misc_0, 28, 4);
		}
		int svidx = vidx & 3;
		insrt(exp.xy_misc_0, 28, 4, vidx);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		insrt(exp.xy_misc_3, 8, 1, 0);
		if (poly && (exp.valid[0] & 0xf0f))
			insrt(exp.valid[0], 21, 1, 0);
		if (!poly)
			insrt(exp.valid[0], vidx, 1, 1);
		if ((cls >= 9 && cls <= 0xb))
			insrt(exp.valid[0], 4|svidx, 1, 1);
		insrt(exp.valid[0], 19, 1, false);
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
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 1)
				val &= 0xffff;
			else
				val |= 0xffff0000;
		}
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
				orig.valid[0] = 0x0fffffff;
				if (jrand48(ctx->rand48) & 1) {
					orig.valid[0] &= ~0xf0f;
				}
				orig.valid[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
				orig.valid[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				insrt(orig.ctx_switch[0], 24, 5, 0x17);
				insrt(orig.ctx_switch[0], 13, 2, 0);
				int j;
				for (j = 0; j < 8; j++) {
					insrt(orig.ctx_cache[j][0], 24, 5, 0x17);
					insrt(orig.ctx_cache[j][0], 13, 2, 0);
				}
			}
		}
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
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
		if (poly && (exp.valid[0] & 0xf0f))
			insrt(exp.valid[0], 21, 1, 0);
		if (!poly)
			insrt(exp.valid[0], vidx|8, 1, 1);
		if ((cls >= 9 && cls <= 0xb))
			insrt(exp.valid[0], 0xc|svidx, 1, 1);
		insrt(exp.valid[0], 19, 1, false);
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
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
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
			orig.valid[0] = 0x0fffffff;
			if (jrand48(ctx->rand48) & 1) {
				orig.valid[0] &= ~0xf0f;
			}
			orig.valid[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
			orig.valid[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
		}
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.ctx_switch[0], 24, 5, 0x17);
			insrt(orig.ctx_switch[0], 13, 2, 0);
			int j;
			for (j = 0; j < 8; j++) {
				insrt(orig.ctx_cache[j][0], 24, 5, 0x17);
				insrt(orig.ctx_cache[j][0], 13, 2, 0);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_x[0], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
			insrt(orig.vtx_y[0], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
		}
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
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
		exp.valid[0] |= 0x202;
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		nv03_pgraph_prep_draw(&exp, false, noclip);
		nv03_pgraph_dump_state(ctx, &real);
		if (real.status) {
			/* Hung PGRAPH... */
			continue;
		}
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
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
			orig.valid[0] = 0x0fffffff;
			if (jrand48(ctx->rand48) & 1) {
				orig.valid[0] &= ~0xf0f;
			}
			orig.valid[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
			orig.valid[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
		}
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.ctx_switch[0], 24, 5, 0x17);
			insrt(orig.ctx_switch[0], 13, 2, 0);
			int j;
			for (j = 0; j < 8; j++) {
				insrt(orig.ctx_cache[j][0], 24, 5, 0x17);
				insrt(orig.ctx_cache[j][0], 13, 2, 0);
			}
		}
		if (!(jrand48(ctx->rand48) & 7)) {
			insrt(orig.vtx_x[0], 2, 29, (jrand48(ctx->rand48) & 1) ? 0 : -1);
			insrt(orig.vtx_y[0], 2, 29, (jrand48(ctx->rand48) & 1) ? 0 : -1);
		}
		orig.debug[2] &= 0xffdfffff;
		orig.debug[3] &= 0xfffeffff;
		orig.debug[3] &= 0xfffdffff;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		insrt(exp.misc32[1], 16, 16, extr(val, 16, 16));
		nv03_pgraph_prep_draw(&exp, false, false);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 0, exp.vtx_x[0], 1, 0, false);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
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
			orig.valid[0] = 0x0fffffff;
			if (jrand48(ctx->rand48) & 1) {
				orig.valid[0] &= ~0xf0f;
			}
			orig.valid[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
			orig.valid[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
		}
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.ctx_switch[0], 24, 5, 0x17);
			insrt(orig.ctx_switch[0], 13, 2, 0);
			int j;
			for (j = 0; j < 8; j++) {
				insrt(orig.ctx_cache[j][0], 24, 5, 0x17);
				insrt(orig.ctx_cache[j][0], 13, 2, 0);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_x[0], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
			insrt(orig.vtx_y[0], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
		}
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_x[1], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
			insrt(orig.vtx_y[1], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
		}
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
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
		exp.valid[0] |= 0xc0c;
		nv03_pgraph_prep_draw(&exp, false, false);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0x000f003f;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
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
			exp.valid[0] |= 0x2020;
			exp.vtx_x[5] = extr(val, 0, 16);
			exp.vtx_y[5] = extr(val, 16, 16);
		}
		if (is_in) {
			exp.valid[0] |= 0x8008;
			exp.vtx_x[3] = extr(val, 0, 16);
			exp.vtx_y[3] = -extr(val, 16, 16);
			exp.vtx_y[7] = extr(val, 16, 16);
			if (!is_out)
				exp.misc24[0] = extr(val, 0, 16);
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
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0x000f000f;
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_x[0], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
			insrt(orig.vtx_y[0], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
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
		exp.misc24[0] = extr(val, 0, 16);
		exp.valid[0] |= 0x404;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
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
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.valid[0] |= 0x20 << xy * 8;
		if (xy)
			exp.vtx_y[5] = val;
		else
			exp.vtx_x[5] = val;
		insrt(exp.xy_misc_0, 28, 4, 0);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
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
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.valid[0] |= 0x9018;
		exp.vtx_x[4] = extr(val, 0, 16) << 16;
		exp.vtx_y[3] = -exp.vtx_y[7];
		exp.vtx_y[4] = extr(val, 16, 16) << 16;
		insrt(exp.valid[0], 19, 1, 0);
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
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
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
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.valid[0] |= 0x1001;
		exp.vtx_x[0] = extrs(val, 0, 16);
		exp.vtx_y[4] = extrs(val, 16, 16);
		insrt(exp.valid[0], 19, 1, 0);
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
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xffff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0x000f000f;
		if (!(jrand48(ctx->rand48) & 3))
			val = 0x00100000;
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_x[0], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
			insrt(orig.vtx_y[0], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
		}
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.valid[0] |= 0x202;
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 1, exp.vtx_x[0], extr(val, 0, 16), 0, false);
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 1, 1, 0, extr(val, 16, 16), 0, false);
		int vtxid = extr(exp.xy_misc_0, 28, 4);
		int nvtxid = (vtxid + 1) & 1;
		insrt(exp.xy_misc_0, 28, 4, nvtxid);
		nv03_pgraph_vtx_cmp(&exp, 0, 8, true);
		nv03_pgraph_vtx_cmp(&exp, 1, 8, true);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
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
			orig.ctx_switch[3] = gctx & 0xffff;
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
		exp.valid[0] |= 0x10;
		exp.misc32[0] = val;
		insrt(exp.xy_misc_1[0], 24, 4, sv);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
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
			orig.ctx_switch[3] = gctx & 0xffff;
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
		exp.valid[0] |= 0x20;
		exp.vtx_y[3] = val;
		insrt(exp.xy_misc_1[1], 24, 4, sv);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
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
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int fmt;
		if (orig.ctx_switch[3] == exp.ctx_switch[3]) {
			fmt = extr(orig.ctx_switch[0], 0, 3);
		} else {
			fmt = extr(exp.ctx_switch[0], 0, 3);
		}
		exp.misc24[0] = extr(val, 0, 16);
		insrt(exp.misc24[1], 0, 11, extr(val, 0, 11));
		insrt(exp.misc24[1], 11, 11, extr(val, 16, 11));
		if (fmt == 7) {
			insrt(exp.valid[0], 9, 1, 0);
			insrt(exp.valid[0], 13, 1, 1);
		} else {
			insrt(exp.valid[0], 9, 1, 1);
			insrt(exp.valid[0], 13, 1, 0);
		}
		if (extr(exp.debug[3], 20, 1) && (extr(val, 0, 16) > 0x400 || extr(val, 16, 16) > 0x400)) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.fifo_enable = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.dma_offset[0] = val;
		exp.valid[0] |= 1 << 23;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.misc32[0] = val;
		exp.valid[0] |= 1 << 24;
		if (extr(exp.debug[3], 20, 1) && (val & ~0xff31ffff ||
			extr(val, 28, 4) > 0xb || extr(val, 24, 4) > 0xb)) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.fifo_enable = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.misc32[1] = val;
		exp.valid[0] |= 1 << 25;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.misc24[0] = val & 0xffffff;
		exp.valid[0] |= 1 << 27;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.d3d_config = val & 0xf77fbdf3;
		exp.valid[0] |= 1 << 26;
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
			exp.fifo_enable = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.d3d_alpha = val & 0xfff;
		bool bad = extr(val, 8, 4) == 0 || extr(val, 8, 4) > 8;
		if (extr(exp.debug[3], 20, 1) && (val & ~0xfff || bad)) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.fifo_enable = 0;
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.d3d_tlv_fog_tri_col1 = val;
		exp.valid[0] |= 1 << 16;
		exp.valid[0] &= ~0x007e0000;
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		insrt(exp.d3d_tlv_color, 0, 24, extr(val, 0, 24));
		insrt(exp.d3d_tlv_color, 24, 7, extr(val, 25, 7));
		exp.valid[0] |= 1 << 17;
		insrt(exp.valid[0], extr(exp.d3d_tlv_fog_tri_col1, 0, 4), 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
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
		insrt(exp.d3d_tlv_xy, 16*xy, 16, tv);
		exp.valid[0] |= 1 << (21 - xy);
		insrt(exp.valid[0], extr(exp.d3d_tlv_fog_tri_col1, 0, 4), 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
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
		exp.d3d_tlv_z = extr(tv, 0, 16);
		insrt(exp.d3d_tlv_rhw, 25, 7, extr(tv, 16, 7));
		insrt(exp.d3d_tlv_color, 31, 1, extr(tv, 23, 1));
		exp.valid[0] |= 1 << 19;
		insrt(exp.valid[0], extr(exp.d3d_tlv_fog_tri_col1, 0, 4), 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		insrt(exp.d3d_tlv_rhw, 0, 25, extr(val, 6, 25));
		exp.valid[0] |= 1 << 18;
		insrt(exp.valid[0], extr(exp.d3d_tlv_fog_tri_col1, 0, 4), 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		int e = extr(val, 23, 8);
		bool s = extr(val, 31, 1);
		uint32_t tv = extr(val, 0, 23) | 1 << 23;
		int sz = extr(exp.misc32[0], 28, 4);
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
		insrt(exp.d3d_tlv_uv[0][0], 0, 16, tv);
		exp.valid[0] |= 1 << 22;
		insrt(exp.valid[0], extr(exp.d3d_tlv_fog_tri_col1, 0, 4), 1, 0);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv03_pgraph_cmp_state(&orig, &exp, &real)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000100;
		orig.ctx_user &= ~0xe000;
		orig.ctx_switch[0] &= ~0x8000;
		int op = extr(orig.ctx_switch[0], 24, 5);
		if (!((op >= 0x00 && op <= 0x15) || op == 0x17 || (op >= 0x19 && op <= 0x1a) || op == 0x1d))
			op = 0x17;
		insrt(orig.ctx_switch[0], 24, 5, op);
		orig.pattern_config = nrand48(ctx->rand48)%3; /* shape 3 is a rather ugly hole in Karnough map */
		insrt(orig.cliprect_ctrl, 8, 1, 0);
		orig.xy_misc_0 = 0;
		orig.xy_misc_1[0] = 0;
		orig.xy_misc_1[1] = 0;
		orig.xy_misc_3 = 0;
		orig.xy_misc_4[0] = 0;
		orig.xy_misc_4[1] = 0;
		orig.valid[0] = 0x10000;
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
			if ((nv03_pgraph_surf_format(&orig) & 3) == 0 && extr(orig.ctx_switch[0], 0, 3) == 4) {
				ckey = nv01_pgraph_to_a1r10g10b10(nv03_pgraph_expand_color(orig.ctx_switch[0] & ~0x7, orig.misc32[0]));
			} else {
				ckey = nv01_pgraph_to_a1r10g10b10(nv03_pgraph_expand_color(orig.ctx_switch[0], orig.misc32[0]));
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
			orig.ctx_switch[3] = gctx & 0xffff;
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
				if (extr(exp.ctx_switch[0], 20 + j, 1)) {
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
		if (nv03_pgraph_cmp_state(&orig, &exp, &real, mismatch)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000100;
		orig.ctx_user &= ~0xe000;
		orig.ctx_switch[0] &= ~0x8000;
		insrt(orig.cliprect_ctrl, 8, 1, 0);
		orig.xy_misc_0 = 0;
		orig.xy_misc_1[0] = 0;
		orig.xy_misc_1[1] = 0;
		orig.xy_misc_3 = 0;
		orig.xy_misc_4[0] = 0;
		orig.xy_misc_4[1] = 0;
		orig.xy_clip[0][0] = 0;
		orig.xy_clip[1][0] = 0;
		orig.valid[0] = 0x4210101;
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
			orig.ctx_switch[3] = gctx & 0xffff;
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
		insrt(exp.misc32[1], 16, 16, extr(val, 16, 16));
		nv03_pgraph_vtx_add(&ctx->chipset, &exp, 0, 0, exp.vtx_x[0], 1, 0, false);
		uint32_t zeta = extr(val, 16, 16);
		struct nv01_color s = nv03_pgraph_expand_color(exp.ctx_switch[0], exp.misc32[0]);
		uint8_t ra = nv01_pgraph_dither_10to5(s.a << 2, x, y, false) >> 1;
		if (nv03_pgraph_cliprect_pass(&exp, x, y)) {
			bool zeta_test = nv03_pgraph_d3d_cmp(extr(exp.d3d_config, 16, 4), zeta, zcur);
			if (!extr(exp.ctx_switch[0], 12, 1))
				zeta_test = true;
			bool alpha_test = nv03_pgraph_d3d_cmp(extr(exp.d3d_alpha, 8, 4), extr(exp.misc32[0], 24, 8), extr(exp.d3d_alpha, 0, 8));
			bool alpha_nz = !!ra;
			bool zeta_wr = nv03_pgraph_d3d_wren(extr(exp.d3d_config, 20, 3), zeta_test, alpha_nz);
			bool color_wr = nv03_pgraph_d3d_wren(extr(exp.d3d_config, 24, 3), zeta_test, alpha_nz);
			if (!alpha_test) {
				color_wr = false;
				zeta_wr = false;
			}
			if (extr(exp.ctx_switch[0], 12, 1) && extr(exp.ctx_switch[0], 20, 4) && zeta_wr) {
				insrt(epixel[3], (paddr[3] & 3) * 8, cpp * 8, zeta);
			}
			for (int j = 0; j < 4; j++) {
				if (extr(exp.ctx_switch[0], 20 + j, 1) && color_wr) {
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
		if (nv03_pgraph_cmp_state(&orig, &exp, &real, mismatch)) {
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
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000080;
		orig.ctx_user &= ~0xe000;
		orig.ctx_switch[0] &= ~0x8000;
		int op = extr(orig.ctx_switch[0], 24, 5);
		if (!((op >= 0x00 && op <= 0x15) || op == 0x17 || (op >= 0x19 && op <= 0x1a) || op == 0x1d))
			op = 0x17;
		insrt(orig.ctx_switch[0], 24, 5, op);
		orig.pattern_config = nrand48(ctx->rand48)%3; /* shape 3 is a rather ugly hole in Karnough map */
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
		orig.valid[0] = 0x0303;
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
			orig.ctx_switch[3] = gctx & 0xffff;
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
		exp.valid[0] = 0;
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
			int ss = extr(exp.ctx_switch[0], 16, 2);
			uint32_t sp = extr(spixel[ss], (spaddr[ss] & 3) * 8, cpp * 8);
			if (sx < 0x80)
				sp = 0;
			if (!nv03_pgraph_cliprect_pass(&exp, sx, sy))
				sp = 0xffffffff;
			struct nv01_color s = nv03_pgraph_expand_surf(fmt, sp);
			for (int j = 0; j < 4; j++) {
				if (extr(exp.ctx_switch[0], 20 + j, 1)) {
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
		if (nv03_pgraph_cmp_state(&orig, &exp, &real, mismatch)) {
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
	nva_wr32(ctx->cnum, 0x200, 0xffffeeff);
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
