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

#include "pgraph.h"
#include "nva.h"

uint32_t gen_rnd(std::mt19937 &rnd) {
	uint32_t res = rnd();
	res = sext(res, rnd() & 0x1f);
	if (!(rnd() & 3)) {
		res ^= 1 << (rnd() & 0x1f);
	}
	return res;
}

void nv01_pgraph_gen_state(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	state->chipset = nva_cards[cnum]->chipset;
	if (state->chipset.card_type < 3) {
		state->debug[0] = rnd() & 0x11111110;
		state->debug[1] = rnd() & 0x31111101;
		state->debug[2] = rnd() & 0x11111111;
	} else {
		state->debug[0] = rnd() & 0x13311110;
		state->debug[1] = rnd() & 0x10113301;
		state->debug[2] = rnd() & 0x1133f111;
		state->debug[3] = rnd() & 0x1173ff31;
	}
	state->intr = 0;
	state->invalid = 0;
	state->dma_intr = 0;
	state->intr_en = rnd() & 0x11111011;
	state->invalid_en = rnd() & 0x00011111;
	state->dma_intr_en = rnd() & 0x00011111;
	if (state->chipset.card_type < 3) {
		state->ctx_switch[0] = rnd() & 0x807fffff;
	} else {
		state->ctx_switch[0] = rnd() & 0x3ff3f71f;
		state->ctx_user = rnd() & 0x7f1fe000;
		for (int i = 0; i < 8; i++)
			state->ctx_cache[i][0] = rnd() & 0x3ff3f71f;
	}
	state->ctx_control = rnd() & 0x11010103;

	for (int i = 0; i < pgraph_vtx_count(&state->chipset); i++) {
		state->vtx_xy[i][0] = gen_rnd(rnd);
		state->vtx_xy[i][1] = gen_rnd(rnd);
	}
	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 14; i++)
			state->vtx_beta[i] = rnd() & 0x01ffffff;
	} else {
		for (int i = 0; i < 16; i++)
			state->vtx_z[i] = rnd() & 0xffffff;
	}
	for (int i = 0; i < 2; i++) {
		state->iclip[i] = rnd() & 0x3ffff;
		state->uclip_min[0][i] = rnd() & 0x3ffff;
		state->uclip_max[0][i] = rnd() & 0x3ffff;
		state->uclip_min[1][i] = rnd() & 0x3ffff;
		state->uclip_max[1][i] = rnd() & 0x3ffff;
	}
	for (int i = 0; i < 2; i++) {
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
	uint32_t canvas_mask, offset_mask = 0;
	if (state->chipset.card_type < 3) {
		canvas_mask = 0x0fff0fff;
		state->canvas_config = rnd() & 0x01111011;
		state->dst_canvas_min = rnd();
		state->dst_canvas_max = rnd() & canvas_mask;
	} else {
		canvas_mask = state->chipset.is_nv03t ? 0x7fff07ff : 0x3fff07ff;
		offset_mask = state->chipset.is_nv03t ? 0x007fffff : 0x003fffff;
		for (int i = 0; i < 4; i++) {
			state->surf_offset[i] = rnd() & offset_mask & ~0xf;
			state->surf_pitch[i] = rnd() & 0x00001ff0;
		}
		state->surf_format = rnd() & 0x7777;
		state->src_canvas_min = rnd() & canvas_mask;
		state->src_canvas_max = rnd() & canvas_mask;
		state->dst_canvas_min = rnd() & canvas_mask;
		state->dst_canvas_max = rnd() & canvas_mask;
	}
	state->cliprect_min[0] = rnd() & canvas_mask;
	state->cliprect_min[1] = rnd() & canvas_mask;
	state->cliprect_max[0] = rnd() & canvas_mask;
	state->cliprect_max[1] = rnd() & canvas_mask;
	state->cliprect_ctrl = rnd() & 0x113;

	state->ctx_switch[1] = rnd() & 0xffff;
	if (state->chipset.card_type < 3) {
		state->xy_a = rnd() & 0xf1ff11ff;
		state->xy_misc_1[0] = rnd() & 0x03177331;
		state->xy_misc_4[0] = rnd() & 0x30ffffff;
		state->xy_misc_4[1] = rnd() & 0x30ffffff;
		state->valid[0] = rnd() & 0x111ff1ff;
		state->misc32[0] = rnd();
		state->subdivide = rnd() & 0xffff00ff;
		state->edgefill = rnd() & 0xffff0113;
		state->notify = rnd() & 0x11ffff;
		state->access = rnd() & 0x0001f000;
		state->access |= 0x0f000111;
		state->pfb_config = rnd() & 0x1370;
		state->pfb_config |= nva_rd32(cnum, 0x600200) & ~0x1371;
		state->pfb_boot = nva_rd32(cnum, 0x600000);
	} else {
		state->notify = rnd() & 0xf1ffff;
		state->ctx_switch[3] = rnd() & 0xffff;
		state->ctx_switch[2] = rnd() & 0x1ffff;
		state->d3d_tlv_xy = rnd();
		state->d3d_tlv_uv[0][0] = rnd();
		state->d3d_tlv_z = rnd() & 0xffff;
		state->d3d_tlv_color = rnd();
		state->d3d_tlv_fog_tri_col1 = rnd();
		state->d3d_tlv_rhw = rnd();
		state->d3d_config = rnd() & 0xf77fbdf3;
		state->d3d_alpha = rnd() & 0xfff;
		state->misc24[0] = rnd() & 0xffffff;
		state->misc24[1] = rnd() & 0xffffff;
		state->misc32[0] = rnd();
		state->misc32[1] = rnd();
		state->valid[0] = rnd();
		state->xy_a = rnd() & 0xf013ffff;
		state->xy_misc_1[0] = rnd() & 0x0f177331;
		state->xy_misc_1[1] = rnd() & 0x0f177331;
		state->xy_misc_3 = rnd() & 0x7f7f1111;
		state->xy_misc_4[0] = rnd() & 0x300000ff;
		state->xy_misc_4[1] = rnd() & 0x300000ff;
		state->xy_clip[0][0] = rnd();
		state->xy_clip[0][1] = rnd();
		state->xy_clip[1][0] = rnd();
		state->xy_clip[1][1] = rnd();
		state->dma_eng_flags[0] = rnd() & 0x03010fff;
		state->dma_eng_limit[0] = rnd();
		state->dma_eng_pte[0] = rnd() & 0xfffff003;
		state->dma_eng_pte_tag[0] = rnd() & 0xfffff000;
		state->dma_eng_addr_virt_adj[0] = rnd();
		state->dma_eng_addr_phys[0] = rnd();
		state->dma_eng_bytes[0] = rnd() & offset_mask;
		state->dma_eng_inst[0] = rnd() & 0x0000ffff;
		state->dma_eng_lines[0] = rnd() & 0x000007ff;
		state->dma_lin_limit = rnd() & offset_mask;
		state->dma_misc = rnd() & 0x707;
		state->dma_offset[0] = rnd();
		state->dma_offset[1] = rnd();
		state->dma_offset[2] = rnd();
		state->dma_pitch = rnd();
		state->fifo_enable = rnd() & 1;
		state->trap_grctx = nva_rd32(cnum, 0x4006bc);
	}
	state->status = 0;
	state->trap_addr = nva_rd32(cnum, 0x4006b4);
	state->trap_data[0] = nva_rd32(cnum, 0x4006b8);
}

void nv01_pgraph_load_state(int cnum, struct pgraph_state *state) {
	nva_wr32(cnum, 0x000200, 0xffffeeff);
	nva_wr32(cnum, 0x000200, 0xffffffff);
	if (state->chipset.card_type < 3) {
		nva_wr32(cnum, 0x4006a4, 0x04000100);
	} else {
		nva_wr32(cnum, 0x4006a4, 0);
	}
	nva_wr32(cnum, 0x400100, 0xffffffff);
	nva_wr32(cnum, 0x400104, 0xffffffff);
	if (state->chipset.card_type >= 3) {
		nva_wr32(cnum, 0x401100, 0xffffffff);
		nva_wr32(cnum, 0x401140, state->dma_intr_en);
		for (int i = 0; i < 8; i++) {
			nva_wr32(cnum, 0x400648, 0x400 | i);
			nva_wr32(cnum, 0x40064c, 0xffffffff);
		}
	}
	nva_wr32(cnum, 0x400140, state->intr_en);
	nva_wr32(cnum, 0x400144, state->invalid_en);
	nva_wr32(cnum, 0x400180, state->ctx_switch[0]);
	nva_wr32(cnum, 0x400190, state->ctx_control);
	if (state->chipset.card_type >= 3) {
		nva_wr32(cnum, 0x400194, state->ctx_user);
		for (int i = 0; i < 8; i++)
			nva_wr32(cnum, 0x4001a0 + i * 4, state->ctx_cache[i][0]);
	}

	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 2; i++) {
			nva_wr32(cnum, 0x400450 + i * 4, state->iclip[i]);
			nva_wr32(cnum, 0x400460 + i * 8, state->uclip_min[0][i]);
			nva_wr32(cnum, 0x400464 + i * 8, state->uclip_max[0][i]);
		}
	} else {
		for (int i = 0; i < 2; i++) {
			nva_wr32(cnum, 0x400534 + i * 4, state->iclip[i]);
			nva_wr32(cnum, 0x40053c + i * 4, state->uclip_min[0][i]);
			nva_wr32(cnum, 0x400544 + i * 4, state->uclip_max[0][i]);
			nva_wr32(cnum, 0x400560 + i * 4, state->uclip_min[1][i]);
			nva_wr32(cnum, 0x400568 + i * 4, state->uclip_max[1][i]);
		}
	}
	for (int i = 0; i < pgraph_vtx_count(&state->chipset); i++) {
		nva_wr32(cnum, 0x400400 + i * 4, state->vtx_xy[i][0]);
		nva_wr32(cnum, 0x400480 + i * 4, state->vtx_xy[i][1]);
	}
	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 14; i++)
			nva_wr32(cnum, 0x400700 + i * 4, state->vtx_beta[i]);
	} else {
		for (int i = 0; i < 16; i++)
			nva_wr32(cnum, 0x400580 + i * 4, state->vtx_z[i]);
	}
	for (int i = 0; i < 2; i++) {
		nva_wr32(cnum, 0x400600 + i * 8, state->pattern_mono_rgb[i]);
		nva_wr32(cnum, 0x400604 + i * 8, state->pattern_mono_a[i]);
		nva_wr32(cnum, 0x400610 + i * 4, state->pattern_mono_bitmap[i]);
	}
	nva_wr32(cnum, 0x400618, state->pattern_config);
	nva_wr32(cnum, 0x40061c, state->bitmap_color[0]);
	nva_wr32(cnum, 0x400624, state->rop);
	nva_wr32(cnum, 0x40062c, state->chroma);
	if (state->chipset.card_type < 3) {
		nva_wr32(cnum, 0x400620, state->bitmap_color[1]);
		nva_wr32(cnum, 0x400628, state->plane);
		nva_wr32(cnum, 0x400630, state->beta);
		nva_wr32(cnum, 0x400688, state->dst_canvas_min);
		nva_wr32(cnum, 0x40068c, state->dst_canvas_max);
		nva_wr32(cnum, 0x400634, state->canvas_config);
	} else {
		nva_wr32(cnum, 0x400640, state->beta);
		nva_wr32(cnum, 0x400550, state->src_canvas_min);
		nva_wr32(cnum, 0x400554, state->src_canvas_max);
		nva_wr32(cnum, 0x400558, state->dst_canvas_min);
		nva_wr32(cnum, 0x40055c, state->dst_canvas_max);
	}
	for (int i = 0; i < 2; i++) {
		nva_wr32(cnum, 0x400690 + i * 8, state->cliprect_min[i]);
		nva_wr32(cnum, 0x400694 + i * 8, state->cliprect_max[i]);
	}
	nva_wr32(cnum, 0x4006a0, state->cliprect_ctrl);

	if (state->chipset.card_type < 3) {
		nva_wr32(cnum, 0x400640, state->xy_a);
		nva_wr32(cnum, 0x400644, state->xy_misc_1[0]);
		nva_wr32(cnum, 0x400648, state->xy_misc_4[0]);
		nva_wr32(cnum, 0x40064c, state->xy_misc_4[1]);
		nva_wr32(cnum, 0x400650, state->valid[0]);
		nva_wr32(cnum, 0x400654, state->misc32[0]);
		nva_wr32(cnum, 0x400658, state->subdivide);
		nva_wr32(cnum, 0x40065c, state->edgefill);
	} else {
		nva_wr32(cnum, 0x400514, state->xy_a);
		nva_wr32(cnum, 0x400518, state->xy_misc_1[0]);
		nva_wr32(cnum, 0x40051c, state->xy_misc_1[1]);
		nva_wr32(cnum, 0x400520, state->xy_misc_3);
		nva_wr32(cnum, 0x400500, state->xy_misc_4[0]);
		nva_wr32(cnum, 0x400504, state->xy_misc_4[1]);
		nva_wr32(cnum, 0x400524, state->xy_clip[0][0]);
		nva_wr32(cnum, 0x400528, state->xy_clip[0][1]);
		nva_wr32(cnum, 0x40052c, state->xy_clip[1][0]);
		nva_wr32(cnum, 0x400530, state->xy_clip[1][1]);
		nva_wr32(cnum, 0x400510, state->misc24[0]);
		nva_wr32(cnum, 0x400570, state->misc24[1]);
		nva_wr32(cnum, 0x40050c, state->misc32[0]);
		nva_wr32(cnum, 0x40054c, state->misc32[1]);
		nva_wr32(cnum, 0x4005c0, state->d3d_tlv_xy);
		nva_wr32(cnum, 0x4005c4, state->d3d_tlv_uv[0][0]);
		nva_wr32(cnum, 0x4005c8, state->d3d_tlv_z);
		nva_wr32(cnum, 0x4005cc, state->d3d_tlv_color);
		nva_wr32(cnum, 0x4005d0, state->d3d_tlv_fog_tri_col1);
		nva_wr32(cnum, 0x4005d4, state->d3d_tlv_rhw);
		nva_wr32(cnum, 0x400644, state->d3d_config);
		nva_wr32(cnum, 0x4006c8, state->d3d_alpha);
		nva_wr32(cnum, 0x400508, state->valid[0]);
		for (int i = 0; i < 4; i++) {
			nva_wr32(cnum, 0x400630 + i * 4, state->surf_offset[i]);
			nva_wr32(cnum, 0x400650 + i * 4, state->surf_pitch[i]);
		}
		nva_wr32(cnum, 0x4006a8, state->surf_format);
		nva_wr32(cnum, 0x401210, state->dma_eng_flags[0]);
		nva_wr32(cnum, 0x401220, state->dma_eng_limit[0]);
		nva_wr32(cnum, 0x401230, state->dma_eng_pte[0]);
		nva_wr32(cnum, 0x401240, state->dma_eng_pte_tag[0]);
		nva_wr32(cnum, 0x401250, state->dma_eng_addr_virt_adj[0]);
		nva_wr32(cnum, 0x401260, state->dma_eng_addr_phys[0]);
		nva_wr32(cnum, 0x401270, state->dma_eng_bytes[0]);
		nva_wr32(cnum, 0x401280, state->dma_eng_inst[0]);
		nva_wr32(cnum, 0x401290, state->dma_eng_lines[0]);
		nva_wr32(cnum, 0x401400, state->dma_lin_limit);
		nva_wr32(cnum, 0x401800, state->dma_offset[0]);
		nva_wr32(cnum, 0x401810, state->dma_offset[1]);
		nva_wr32(cnum, 0x401820, state->dma_offset[2]);
		nva_wr32(cnum, 0x401830, state->dma_pitch);
		nva_wr32(cnum, 0x401840, state->dma_misc);
	}

	nva_wr32(cnum, 0x400680, state->ctx_switch[1]);
	nva_wr32(cnum, 0x400684, state->notify);
	if (state->chipset.card_type >= 3) {
		nva_wr32(cnum, 0x400688, state->ctx_switch[3]);
		nva_wr32(cnum, 0x40068c, state->ctx_switch[2]);
	}
	nva_wr32(cnum, 0x400080, state->debug[0]);
	nva_wr32(cnum, 0x400084, state->debug[1]);
	nva_wr32(cnum, 0x400088, state->debug[2]);
	if (state->chipset.card_type < 3) {
		nva_wr32(cnum, 0x4006a4, state->access);
		nva_wr32(cnum, 0x600200, state->pfb_config);
	} else {
		nva_wr32(cnum, 0x40008c, state->debug[3]);
		nva_wr32(cnum, 0x4006a4, state->fifo_enable);
	}
}

void nv01_pgraph_dump_state(int cnum, struct pgraph_state *state) {
	int ctr = 0;
	state->chipset = nva_cards[cnum]->chipset;
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
	if (state->chipset.card_type < 3) {
		state->access = nva_rd32(cnum, 0x4006a4);
		state->xy_misc_1[0] = nva_rd32(cnum, 0x400644); /* this one can be disturbed by *reading* VTX mem */
		nva_wr32(cnum, 0x4006a4, 0x04000100);
	} else {
		state->fifo_enable = nva_rd32(cnum, 0x4006a4);
		nva_wr32(cnum, 0x4006a4, 0);
	}
	state->trap_addr = nva_rd32(cnum, 0x4006b4);
	state->trap_data[0] = nva_rd32(cnum, 0x4006b8);
	if (state->chipset.card_type >= 3) {
		state->trap_grctx = nva_rd32(cnum, 0x4006bc);
	}
	state->intr = nva_rd32(cnum, 0x400100) & ~0x100;
	state->invalid = nva_rd32(cnum, 0x400104);
	state->intr_en = nva_rd32(cnum, 0x400140);
	state->invalid_en = nva_rd32(cnum, 0x400144);
	if (state->chipset.card_type >= 3) {
		state->dma_intr = nva_rd32(cnum, 0x401100);
		state->dma_intr_en = nva_rd32(cnum, 0x401140);
	}
	state->ctx_switch[0] = nva_rd32(cnum, 0x400180);
	state->ctx_control = nva_rd32(cnum, 0x400190) & ~0x00100000;
	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 2; i++) {
			state->iclip[i] = nva_rd32(cnum, 0x400450 + i * 4);
			state->uclip_min[0][i] = nva_rd32(cnum, 0x400460 + i * 8);
			state->uclip_max[0][i] = nva_rd32(cnum, 0x400464 + i * 8);
		}
	} else {
		state->ctx_user = nva_rd32(cnum, 0x400194);
		for (int i = 0; i < 8; i++)
			state->ctx_cache[i][0] = nva_rd32(cnum, 0x4001a0 + i * 4);
		for (int i = 0; i < 2; i++) {
			state->iclip[i] = nva_rd32(cnum, 0x400534 + i * 4);
			state->uclip_min[0][i] = nva_rd32(cnum, 0x40053c + i * 4);
			state->uclip_max[0][i] = nva_rd32(cnum, 0x400544 + i * 4);
			state->uclip_min[1][i] = nva_rd32(cnum, 0x400560 + i * 4);
			state->uclip_max[1][i] = nva_rd32(cnum, 0x400568 + i * 4);
		}
	}
	for (int i = 0; i < pgraph_vtx_count(&state->chipset); i++) {
		state->vtx_xy[i][0] = nva_rd32(cnum, 0x400400 + i * 4);
		state->vtx_xy[i][1] = nva_rd32(cnum, 0x400480 + i * 4);
	}
	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 14; i++) {
			state->vtx_beta[i] = nva_rd32(cnum, 0x400700 + i * 4);
		}
	} else {
		for (int i = 0; i < 16; i++) {
			state->vtx_z[i] = nva_rd32(cnum, 0x400580 + i * 4);
		}
	}
	for (int i = 0; i < 2; i++) {
		state->pattern_mono_rgb[i] = nva_rd32(cnum, 0x400600 + i * 8);
		state->pattern_mono_a[i] = nva_rd32(cnum, 0x400604 + i * 8);
		state->pattern_mono_bitmap[i] = nva_rd32(cnum, 0x400610 + i * 4);
	}
	state->pattern_config = nva_rd32(cnum, 0x400618);
	state->bitmap_color[0] = nva_rd32(cnum, 0x40061c);
	state->rop = nva_rd32(cnum, 0x400624);
	state->chroma = nva_rd32(cnum, 0x40062c);
	if (state->chipset.card_type < 3) {
		state->bitmap_color[1] = nva_rd32(cnum, 0x400620);
		state->plane = nva_rd32(cnum, 0x400628);
		state->beta = nva_rd32(cnum, 0x400630);
		state->canvas_config = nva_rd32(cnum, 0x400634);
		state->dst_canvas_min = nva_rd32(cnum, 0x400688);
		state->dst_canvas_max = nva_rd32(cnum, 0x40068c);
	} else {
		state->beta = nva_rd32(cnum, 0x400640);
		state->src_canvas_min = nva_rd32(cnum, 0x400550);
		state->src_canvas_max = nva_rd32(cnum, 0x400554);
		state->dst_canvas_min = nva_rd32(cnum, 0x400558);
		state->dst_canvas_max = nva_rd32(cnum, 0x40055c);
	}
	for (int i = 0; i < 2; i++) {
		state->cliprect_min[i] = nva_rd32(cnum, 0x400690 + i * 8);
		state->cliprect_max[i] = nva_rd32(cnum, 0x400694 + i * 8);
	}
	state->cliprect_ctrl = nva_rd32(cnum, 0x4006a0);
	if (state->chipset.card_type < 3) {
		state->valid[0] = nva_rd32(cnum, 0x400650);
		state->misc32[0] = nva_rd32(cnum, 0x400654);
		state->subdivide = nva_rd32(cnum, 0x400658);
		state->edgefill = nva_rd32(cnum, 0x40065c);
		state->xy_a = nva_rd32(cnum, 0x400640);
		state->xy_misc_4[0] = nva_rd32(cnum, 0x400648);
		state->xy_misc_4[1] = nva_rd32(cnum, 0x40064c);
	} else {
		state->valid[0] = nva_rd32(cnum, 0x400508);
		state->xy_a = nva_rd32(cnum, 0x400514);
		state->xy_misc_1[0] = nva_rd32(cnum, 0x400518);
		state->xy_misc_1[1] = nva_rd32(cnum, 0x40051c);
		state->xy_misc_3 = nva_rd32(cnum, 0x400520);
		state->xy_misc_4[0] = nva_rd32(cnum, 0x400500);
		state->xy_misc_4[1] = nva_rd32(cnum, 0x400504);
		state->xy_clip[0][0] = nva_rd32(cnum, 0x400524);
		state->xy_clip[0][1] = nva_rd32(cnum, 0x400528);
		state->xy_clip[1][0] = nva_rd32(cnum, 0x40052c);
		state->xy_clip[1][1] = nva_rd32(cnum, 0x400530);
		state->misc24[0] = nva_rd32(cnum, 0x400510);
		state->misc24[1] = nva_rd32(cnum, 0x400570);
		state->misc32[0] = nva_rd32(cnum, 0x40050c);
		state->misc32[1] = nva_rd32(cnum, 0x40054c);
		state->d3d_tlv_xy = nva_rd32(cnum, 0x4005c0);
		state->d3d_tlv_uv[0][0] = nva_rd32(cnum, 0x4005c4);
		state->d3d_tlv_z = nva_rd32(cnum, 0x4005c8);
		state->d3d_tlv_color = nva_rd32(cnum, 0x4005cc);
		state->d3d_tlv_fog_tri_col1 = nva_rd32(cnum, 0x4005d0);
		state->d3d_tlv_rhw = nva_rd32(cnum, 0x4005d4);
		state->d3d_config = nva_rd32(cnum, 0x400644);
		state->d3d_alpha = nva_rd32(cnum, 0x4006c8);
	}
	state->ctx_switch[1] = nva_rd32(cnum, 0x400680);
	state->notify = nva_rd32(cnum, 0x400684);
	if (state->chipset.card_type >= 3) {
		state->ctx_switch[3] = nva_rd32(cnum, 0x400688);
		state->ctx_switch[2] = nva_rd32(cnum, 0x40068c);
		for (int i = 0; i < 4; i++) {
			state->surf_offset[i] = nva_rd32(cnum, 0x400630 + i * 4);
			state->surf_pitch[i] = nva_rd32(cnum, 0x400650 + i * 4);
		}
		state->surf_format = nva_rd32(cnum, 0x4006a8);
		state->dma_eng_flags[0] = nva_rd32(cnum, 0x401210);
		state->dma_eng_limit[0] = nva_rd32(cnum, 0x401220);
		state->dma_eng_pte[0] = nva_rd32(cnum, 0x401230);
		state->dma_eng_pte_tag[0] = nva_rd32(cnum, 0x401240);
		state->dma_eng_addr_virt_adj[0] = nva_rd32(cnum, 0x401250);
		state->dma_eng_addr_phys[0] = nva_rd32(cnum, 0x401260);
		state->dma_eng_bytes[0] = nva_rd32(cnum, 0x401270);
		state->dma_eng_inst[0] = nva_rd32(cnum, 0x401280);
		state->dma_eng_lines[0] = nva_rd32(cnum, 0x401290);
		state->dma_lin_limit = nva_rd32(cnum, 0x401400);
		state->dma_offset[0] = nva_rd32(cnum, 0x401800);
		state->dma_offset[1] = nva_rd32(cnum, 0x401810);
		state->dma_offset[2] = nva_rd32(cnum, 0x401820);
		state->dma_pitch = nva_rd32(cnum, 0x401830);
		state->dma_misc = nva_rd32(cnum, 0x401840);
		state->debug[3] = nva_rd32(cnum, 0x40008c);
	}
	state->debug[0] = nva_rd32(cnum, 0x400080);
	state->debug[1] = nva_rd32(cnum, 0x400084);
	state->debug[2] = nva_rd32(cnum, 0x400088);
}

int nv01_pgraph_cmp_state(struct pgraph_state *orig, struct pgraph_state *exp, struct pgraph_state *real, bool broke) {
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
	if (orig->chipset.card_type >= 3) {
		CMP(trap_grctx, "TRAP_GRCTX")
	}
#endif
	CMP(debug[0], "DEBUG[0]")
	CMP(debug[1], "DEBUG[1]")
	CMP(debug[2], "DEBUG[2]")
	if (orig->chipset.card_type >= 3) {
		CMP(debug[3], "DEBUG[3]")
	}
	CMP(intr, "INTR")
	CMP(intr_en, "INTR_EN")
	CMP(invalid, "INVALID")
	CMP(invalid_en, "INVALID_EN")
	if (orig->chipset.card_type < 3) {
		CMP(access, "ACCESS")
	} else {
		CMP(dma_intr, "DMA_INTR")
		CMP(dma_intr_en, "DMA_INTR_EN")
		CMP(fifo_enable, "FIFO_ENABLE")
	}
	CMP(ctx_switch[0], "CTX_SWITCH[0]")
	CMP(ctx_switch[1], "CTX_SWITCH[1]")
	if (orig->chipset.card_type >= 3) {
		CMP(ctx_switch[2], "CTX_SWITCH[2]")
		CMP(ctx_switch[3], "CTX_SWITCH[3]")
	}
	CMP(notify, "NOTIFY")
	CMP(ctx_control, "CTX_CONTROL")
	if (orig->chipset.card_type >= 3) {
		CMP(ctx_user, "CTX_USER")
		for (int i = 0; i < 8; i++) {
			CMP(ctx_cache[i][0], "CTX_CACHE[%d][0]", i)
		}
	}

	for (int i = 0; i < 2; i++) {
		CMP(iclip[i], "ICLIP[%d]", i)
	}
	for (int i = 0; i < 2; i++) {
		CMP(uclip_min[0][i], "UCLIP_MIN[%d]", i)
		CMP(uclip_max[0][i], "UCLIP_MAX[%d]", i)
	}
	if (orig->chipset.card_type >= 3) {
		for (int i = 0; i < 2; i++) {
			CMP(uclip_min[1][i], "OCLIP_MIN[%d]", i)
			CMP(uclip_max[1][i], "OCLIP_MAX[%d]", i)
		}
	}
	for (int i = 0; i < pgraph_vtx_count(&orig->chipset); i++) {
		CMP(vtx_xy[i][0], "VTX_X[%d]", i)
		CMP(vtx_xy[i][1], "VTX_Y[%d]", i)
	}
	if (orig->chipset.card_type < 3) {
		for (int i = 0; i < 14; i++) {
			CMP(vtx_beta[i], "VTX_BETA[%d]", i)
		}
	} else {
		for (int i = 0; i < 16; i++) {
			CMP(vtx_z[i], "VTX_Z[%d]", i)
		}
	}

	CMP(xy_a, "XY_A")
	CMP(xy_misc_1[0], "XY_MISC_1[0]")
	if (orig->chipset.card_type >= 3) {
		CMP(xy_misc_1[1], "XY_MISC_1[1]")
		CMP(xy_misc_3, "XY_MISC_3")
	}
	CMP(xy_misc_4[0], "XY_MISC_4[0]")
	CMP(xy_misc_4[1], "XY_MISC_4[1]")
	if (orig->chipset.card_type >= 3) {
		CMP(xy_clip[0][0], "XY_CLIP[0][0]")
		CMP(xy_clip[0][1], "XY_CLIP[0][1]")
		CMP(xy_clip[1][0], "XY_CLIP[1][0]")
		CMP(xy_clip[1][1], "XY_CLIP[1][1]")
	}
	CMP(valid[0], "VALID[0]")
	CMP(misc32[0], "MISC32[0]")
	if (orig->chipset.card_type < 3) {
		CMP(subdivide, "SUBDIVIDE")
		CMP(edgefill, "EDGEFILL")
	} else {
		CMP(misc32[1], "MISC32[1]")
		CMP(misc24[0], "MISC24[0]")
		CMP(misc24[1], "MISC24[1]")
	}

	CMP(pattern_mono_rgb[0], "PATTERN_MONO_RGB[0]")
	CMP(pattern_mono_a[0], "PATTERN_MONO_A[0]")
	CMP(pattern_mono_rgb[1], "PATTERN_MONO_RGB[1]")
	CMP(pattern_mono_a[1], "PATTERN_MONO_A[1]")
	CMP(pattern_mono_bitmap[0], "PATTERN_MONO_BITMAP[0]")
	CMP(pattern_mono_bitmap[1], "PATTERN_MONO_BITMAP[1]")
	CMP(pattern_config, "PATTERN_CONFIG")
	CMP(rop, "ROP")
	CMP(beta, "BETA")
	CMP(chroma, "CHROMA")
	CMP(bitmap_color[0], "BITMAP_COLOR[0]")
	if (orig->chipset.card_type < 3) {
		CMP(bitmap_color[1], "BITMAP_COLOR[1]")
		CMP(plane, "PLANE")
		CMP(dst_canvas_min, "DST_CANVAS_MIN")
		CMP(dst_canvas_max, "DST_CANVAS_MAX")
		CMP(canvas_config, "CANVAS_CONFIG")
	} else {
		CMP(src_canvas_min, "SRC_CANVAS_MIN")
		CMP(src_canvas_max, "SRC_CANVAS_MAX")
		CMP(dst_canvas_min, "DST_CANVAS_MIN")
		CMP(dst_canvas_max, "DST_CANVAS_MAX")
	}
	for (int i = 0; i < 2; i++) {
		CMP(cliprect_min[i], "CLIPRECT_MIN[%d]", i)
		CMP(cliprect_max[i], "CLIPRECT_MAX[%d]", i)
	}
	CMP(cliprect_ctrl, "CLIPRECT_CTRL")
	if (orig->chipset.card_type >= 3) {
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
	}
	if (broke && !print) {
		print = true;
		goto restart;
	}
	return broke;
}
