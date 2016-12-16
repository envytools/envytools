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

void pgraph_gen_state_debug(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	bool is_nv5 = state->chipset.chipset >= 5;
	bool is_nv11p = nv04_pgraph_is_nv11p(&state->chipset);
	bool is_nv15p = nv04_pgraph_is_nv15p(&state->chipset);
	bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
	if (state->chipset.card_type < 3) {
		state->debug[0] = rnd() & 0x11111110;
		state->debug[1] = rnd() & 0x31111101;
		state->debug[2] = rnd() & 0x11111111;
	} else if (state->chipset.card_type < 4) {
		state->debug[0] = rnd() & 0x13311110;
		state->debug[1] = rnd() & 0x10113301;
		state->debug[2] = rnd() & 0x1133f111;
		state->debug[3] = rnd() & 0x1173ff31;
	} else if (state->chipset.card_type < 0x10) {
		state->debug[0] = rnd() & 0x1337f000;
		state->debug[1] = rnd() & (is_nv5 ? 0xf2ffb701 : 0x72113101);
		state->debug[2] = rnd() & 0x11d7fff1;
		state->debug[3] = rnd() & (is_nv5 ? 0xfbffff73 : 0x11ffff33);
	} else if (state->chipset.card_type < 0x20) {
		// debug[0] holds only resets?
		state->debug[0] = 0;
		state->debug[1] = rnd() & (is_nv11p ? 0xfe71f701 : 0xfe11f701);
		state->debug[2] = rnd() & 0xffffffff;
		state->debug[3] = rnd() & (is_nv15p ? 0xffffff78 : 0xfffffc70);
		state->debug[4] = rnd() & (is_nv17p ? 0x1fffffff : 0x00ffffff);
		if (is_nv17p)
			state->debug[3] |= 0x400;
	}
}

void pgraph_gen_state_control(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	bool is_nv5 = state->chipset.chipset >= 5;
	bool is_nv11p = nv04_pgraph_is_nv11p(&state->chipset);
	bool is_nv15p = nv04_pgraph_is_nv15p(&state->chipset);
	bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
	state->intr = 0;
	state->invalid = 0;
	state->dma_intr = 0;
	if (state->chipset.card_type < 4) {
		state->intr_en = rnd() & 0x11111011;
		state->invalid_en = rnd() & 0x00011111;
		state->dma_intr_en = rnd() & 0x00011111;
	} else if (state->chipset.card_type < 0x10) {
		state->intr_en = rnd() & 0x11311;
		state->nstatus = rnd() & 0x7800;
	} else {
		state->intr_en = rnd() & 0x1113711;
		state->nstatus = rnd() & 0x7800000;
	}
	state->nsource = 0;
	if (state->chipset.card_type < 3) {
		state->ctx_switch[0] = rnd() & 0x807fffff;
		state->ctx_switch[1] = rnd() & 0xffff;
		state->notify = rnd() & 0x11ffff;
		state->access = rnd() & 0x0001f000;
		state->access |= 0x0f000111;
		state->pfb_config = rnd() & 0x1370;
		state->pfb_config |= nva_rd32(cnum, 0x600200) & ~0x1371;
		state->pfb_boot = nva_rd32(cnum, 0x600000);
	} else if (state->chipset.card_type < 4) {
		state->ctx_switch[0] = rnd() & 0x3ff3f71f;
		state->ctx_switch[1] = rnd() & 0xffff;
		state->notify = rnd() & 0xf1ffff;
		state->ctx_switch[3] = rnd() & 0xffff;
		state->ctx_switch[2] = rnd() & 0x1ffff;
		state->ctx_user = rnd() & 0x7f1fe000;
		for (int i = 0; i < 8; i++)
			state->ctx_cache[i][0] = rnd() & 0x3ff3f71f;
		state->fifo_enable = rnd() & 1;
	} else {
		uint32_t ctxs_mask, ctxc_mask;
		if (state->chipset.card_type < 0x10) {
			ctxs_mask = ctxc_mask = is_nv5 ? 0x7f73f0ff : 0x0303f0ff;
			state->ctx_user = rnd() & 0x0f00e000;
			state->unk610 = rnd() & (is_nv5 ? 0xe1fffff0 : 0xe0fffff0);
			state->unk614 = rnd() & (is_nv5 ? 0xc1fffff0 : 0xc0fffff0);
		} else {
			ctxs_mask = is_nv11p ? 0x7ffff0ff : 0x7fb3f0ff;
			ctxc_mask = is_nv11p ? 0x7ffff0ff : is_nv15p ? 0x7fb3f0ff : 0x7f33f0ff;
			state->ctx_user = rnd() & (is_nv15p ? 0x9f00e000 : 0x1f00e000);
			state->unk610 = rnd() & 0xfffffff0;
			state->unk614 = rnd() & 0xc7fffff0;
			if (!is_nv15p) {
				state->unk77c = rnd() & 0x1100ffff;
				if (state->unk77c & 0x10000000)
					state->unk77c |= 0x70000000;
			} else {
				state->unk77c = rnd() & 0x631fffff;
			}
			state->unk6b0 = rnd();
			state->unk838 = rnd();
			state->unk83c = rnd();
			state->unka00 = rnd() & 0x1fff1fff;
			state->unka04 = rnd() & 0x1fff1fff;
			state->unka10 = rnd() & 0xdfff3fff;
		}
		state->ctx_switch[0] = rnd() & ctxs_mask;
		state->ctx_switch[1] = rnd() & 0xffff3f03;
		state->ctx_switch[2] = rnd() & 0xffffffff;
		state->ctx_switch[3] = rnd() & 0x0000ffff;
		state->ctx_switch[4] = rnd();
		for (int i = 0; i < 8; i++) {
			state->ctx_cache[i][0] = rnd() & ctxc_mask;
			state->ctx_cache[i][1] = rnd() & 0xffff3f03;
			state->ctx_cache[i][2] = rnd() & 0xffffffff;
			state->ctx_cache[i][3] = rnd() & 0x0000ffff;
			state->ctx_cache[i][4] = rnd();
		}
		state->fifo_enable = rnd() & 1;
		for (int i = 0; i < 8; i++) {
			if (state->chipset.card_type < 0x10) {
				state->fifo_mthd[i] = rnd() & 0x00007fff;
			} else if (!is_nv17p) {
				state->fifo_mthd[i] = rnd() & 0x01171ffc;
			} else {
				state->fifo_mthd[i] = rnd() & 0x00371ffc;
			}
			state->fifo_data[i][0] = rnd() & 0xffffffff;
			state->fifo_data[i][1] = rnd() & 0xffffffff;
		}
		if (state->fifo_enable) {
			state->fifo_ptr = (rnd() & (is_nv17p ? 0xf : 7)) * 0x11;
			if (state->chipset.card_type < 0x10) {
				state->fifo_mthd_st2 = rnd() & 0x000ffffe;
			} else {
				state->fifo_mthd_st2 = rnd() & (is_nv17p ? 0x7bf71ffc : 0x3bf71ffc);
			}
		} else {
			state->fifo_ptr = rnd() & (is_nv17p ? 0xff : 0x77);
			if (state->chipset.card_type < 0x10) {
				state->fifo_mthd_st2 = rnd() & 0x000fffff;
			} else {
				state->fifo_mthd_st2 = rnd() & (is_nv17p ? 0x7ff71ffc : 0x3ff71ffc);
			}
		}
		state->fifo_data_st2[0] = rnd() & 0xffffffff;
		state->fifo_data_st2[1] = rnd() & 0xffffffff;
		if (state->chipset.card_type < 0x10) {
			state->notify = rnd() & 0x00110101;
		} else {
			state->notify = rnd() & 0x73110101;
		}
	}
	state->ctx_control = rnd() & 0x11010103;
	state->status = 0;
	if (state->chipset.card_type < 4) {
		state->trap_addr = nva_rd32(cnum, 0x4006b4);
		state->trap_data[0] = nva_rd32(cnum, 0x4006b8);
		if (state->chipset.card_type == 3)
			state->trap_grctx = nva_rd32(cnum, 0x4006bc);
	} else {
		state->trap_addr = nva_rd32(cnum, 0x400704);
		state->trap_data[0] = nva_rd32(cnum, 0x400708);
		state->trap_data[1] = nva_rd32(cnum, 0x40070c);
	}
}

void pgraph_gen_state_vtx(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (int i = 0; i < pgraph_vtx_count(&state->chipset); i++) {
		state->vtx_xy[i][0] = gen_rnd(rnd);
		state->vtx_xy[i][1] = gen_rnd(rnd);
	}
	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 14; i++)
			state->vtx_beta[i] = rnd() & 0x01ffffff;
	} else if (state->chipset.card_type < 4) {
		for (int i = 0; i < 16; i++)
			state->vtx_z[i] = rnd() & 0xffffff;
	} else if (state->chipset.card_type < 0x10) {
		for (int i = 0; i < 16; i++) {
			state->vtx_u[i] = rnd() & 0xffffffc0;
			state->vtx_v[i] = rnd() & 0xffffffc0;
			state->vtx_m[i] = rnd() & 0xffffffc0;
		}
	}
	for (int i = 0; i < 2; i++) {
		state->iclip[i] = rnd() & 0x3ffff;
		for (int j = 0; j < 3; j++) {
			state->uclip_min[j][i] = rnd() & (state->chipset.card_type < 4 ? 0x3ffff : 0xffff);
			state->uclip_max[j][i] = rnd() & 0x3ffff;
		}
	}
}

void pgraph_gen_state_canvas(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	uint32_t canvas_mask;
	uint32_t offset_mask = pgraph_offset_mask(&state->chipset);
	if (state->chipset.card_type < 4) {
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
	} else {
		uint32_t pitch_mask = pgraph_pitch_mask(&state->chipset);
		bool is_nv11p = nv04_pgraph_is_nv11p(&state->chipset);
		bool is_nv15p = nv04_pgraph_is_nv15p(&state->chipset);
		bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
		for (int i = 0; i < 6; i++) {
			state->surf_base[i] = rnd() & offset_mask;
			state->surf_offset[i] = rnd() & offset_mask;
			state->surf_limit[i] = (rnd() & (offset_mask | 1 << 31)) | 0xf;
		}
		for (int i = 0; i < 5; i++)
			state->surf_pitch[i] = rnd() & pitch_mask;
		for (int i = 0; i < 2; i++)
			state->surf_swizzle[i] = rnd() & 0x0f0f0000;
		if (!is_nv15p) {
			state->surf_type = rnd() & 3;
		} else if (!is_nv11p) {
			state->surf_type = rnd() & 0x77777703;
		} else if (!is_nv17p) {
			state->surf_type = rnd() & 0x77777713;
		} else {
			state->surf_type = rnd() & 0xf77777ff;
		}
		state->surf_format = rnd() & 0xffffff;
	}
}

void pgraph_gen_state_rop(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (int i = 0; i < 2; i++) {
		if (state->chipset.card_type < 4) {
			state->pattern_mono_rgb[i] = rnd() & 0x3fffffff;
			state->pattern_mono_a[i] = rnd() & 0xff;
			state->bitmap_color[i] = rnd() & 0x7fffffff;
		} else {
			state->bitmap_color[i] = rnd();
			state->pattern_mono_color[i] = rnd();
		}
		state->pattern_mono_bitmap[i] = rnd();
	}
	state->rop = rnd() & 0xff;
	state->beta = rnd() & 0x7f800000;
	if (state->chipset.card_type < 4) {
		state->pattern_config = rnd() & 3;
		state->plane = rnd() & 0x7fffffff;
		state->chroma = rnd() & 0x7fffffff;
	} else {
		bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
		state->pattern_config = rnd() & 0x13;
		state->beta4 = rnd();
		state->chroma = rnd();
		for (int i = 0; i < 64; i++)
			state->pattern_color[i] = rnd() & 0xffffff;
		state->ctx_valid = rnd() & (is_nv17p ? 0x3f731f3f : 0x0f731f3f);
		state->ctx_format = rnd() & 0x3f3f3f3f;
	}
}

void pgraph_gen_state_vstate(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	if (state->chipset.card_type < 3) {
		state->xy_a = rnd() & 0xf1ff11ff;
		state->xy_misc_1[0] = rnd() & 0x03177331;
		state->xy_misc_4[0] = rnd() & 0x30ffffff;
		state->xy_misc_4[1] = rnd() & 0x30ffffff;
		state->valid[0] = rnd() & 0x111ff1ff;
		state->misc32[0] = rnd();
		state->subdivide = rnd() & 0xffff00ff;
		state->edgefill = rnd() & 0xffff0113;
	} else {
		state->misc24[0] = rnd() & 0xffffff;
		state->misc24[1] = rnd() & 0xffffff;
		state->misc24[2] = rnd() & 0xffffff;
		state->misc32[0] = rnd();
		state->misc32[1] = rnd();
		state->misc32[2] = rnd();
		state->misc32[3] = rnd();
		if (state->chipset.card_type < 0x10) {
			state->xy_a = rnd() & 0xf013ffff;
		} else {
			state->xy_a = rnd() & 0xf113ffff;
		}
		if (state->chipset.card_type < 4) {
			state->xy_misc_1[0] = rnd() & 0x0f177331;
			state->xy_misc_1[1] = rnd() & 0x0f177331;
			state->valid[0] = rnd();
		} else {
			state->xy_misc_1[0] = rnd() & 0x00111031;
			state->xy_misc_1[1] = rnd() & 0x00111031;
			state->valid[0] = rnd() & 0xf07fffff;
			if (state->chipset.card_type < 0x10) {
				state->valid[1] = rnd() & 0x1fffffff;
			} else {
				state->valid[1] = rnd() & 0xdfffffff;
			}
			state->dma_pitch = rnd();
			state->dvd_format = rnd() & 0x0000033f;
			state->sifm_mode = rnd() & 0x01030000;
			state->unk588 = rnd() & 0x0000ffff;
			state->unk58c = rnd() & 0x0001ffff;
		}
		state->xy_misc_3 = rnd() & 0x7f7f1111;
		state->xy_misc_4[0] = rnd() & 0x300000ff;
		state->xy_misc_4[1] = rnd() & 0x300000ff;
		state->xy_clip[0][0] = rnd();
		state->xy_clip[0][1] = rnd();
		state->xy_clip[1][0] = rnd();
		state->xy_clip[1][1] = rnd();
	}
}

void pgraph_gen_state_dma_nv3(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	uint32_t offset_mask = pgraph_offset_mask(&state->chipset);
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
}

void pgraph_gen_state_dma_nv4(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	state->dma_offset[0] = rnd();
	state->dma_offset[1] = rnd();
	state->dma_length = rnd() & 0x003fffff;
	state->dma_misc = rnd() & 0x0077ffff;
	state->dma_unk20[0] = rnd();
	state->dma_unk20[1] = rnd();
	state->dma_unk3c = rnd() & 0x1f;
	for (int i = 0; i < 2; i++) {
		state->dma_eng_inst[i] = rnd() & 0x0000ffff;
		state->dma_eng_flags[i] = rnd() & 0xfff33000;
		state->dma_eng_limit[i] = rnd();
		state->dma_eng_pte[i] = rnd() & 0xfffff002;
		state->dma_eng_pte_tag[i] = rnd() & 0xfffff000;
		state->dma_eng_addr_virt_adj[i] = rnd();
		state->dma_eng_addr_phys[i] = rnd();
		state->dma_eng_bytes[i] = rnd() & 0x01ffffff;
		state->dma_eng_lines[i] = rnd() & 0x000007ff;
	}
}

void pgraph_gen_state_d3d0(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	state->d3d_tlv_xy = rnd();
	state->d3d_tlv_uv[0][0] = rnd();
	state->d3d_tlv_z = rnd() & 0xffff;
	state->d3d_tlv_color = rnd();
	state->d3d_tlv_fog_tri_col1 = rnd();
	state->d3d_tlv_rhw = rnd();
	state->d3d_config = rnd() & 0xf77fbdf3;
	state->d3d_alpha = rnd() & 0xfff;
}

void pgraph_gen_state_d3d56(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (int i = 0; i < 2; i++) {
		state->d3d_rc_alpha[i] = rnd() & 0xfd1d1d1d;
		state->d3d_rc_color[i] = rnd() & 0xff1f1f1f;
		state->d3d_tex_format[i] = rnd() & 0xfffff7a6;
		state->d3d_tex_filter[i] = rnd() & 0xffff9e1e;
	}
	state->d3d_tlv_xy = rnd() & 0xffffffff;
	state->d3d_tlv_uv[0][0] = rnd() & 0xffffffc0;
	state->d3d_tlv_uv[0][1] = rnd() & 0xffffffc0;
	state->d3d_tlv_uv[1][0] = rnd() & 0xffffffc0;
	state->d3d_tlv_uv[1][1] = rnd() & 0xffffffc0;
	state->d3d_tlv_z = rnd() & 0xffffffff;
	state->d3d_tlv_color = rnd() & 0xffffffff;
	state->d3d_tlv_fog_tri_col1 = rnd() & 0xffffffff;
	state->d3d_tlv_rhw = rnd() & 0xffffffc0;
	state->d3d_config = rnd() & 0xffff5fff;
	state->d3d_stencil_func = rnd() & 0xfffffff1;
	state->d3d_stencil_op = rnd() & 0x00000fff;
	state->d3d_blend = rnd() & 0xff1111ff;
}

void pgraph_gen_state_celsius(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	bool is_nv15p = nv04_pgraph_is_nv15p(&state->chipset);
	bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
	for (int i = 0; i < 2; i++) {
		state->celsius_tex_offset[i] = rnd();
		state->celsius_tex_palette[i] = rnd() & 0xffffffc1;
		state->celsius_tex_format[i] = rnd() & (is_nv17p ? 0xffffffde : 0xffffffd6);
		state->celsius_tex_control[i] = rnd() & 0x7fffffff;
		state->celsius_tex_pitch[i] = rnd() & 0xffff0000;
		state->celsius_tex_unk238[i] = rnd();
		state->celsius_tex_rect[i] = rnd() & 0x07ff07ff;
		state->celsius_tex_filter[i] = rnd() & 0x77001fff;
		state->celsius_tex_color_key[i] = rnd();
		state->celsius_rc_in[0][i] = rnd();
		state->celsius_rc_in[1][i] = rnd();
		state->celsius_rc_factor[i] = rnd();
		state->celsius_rc_out[0][i] = rnd() & 0x0003cfff;
	}
	state->celsius_rc_out[1][0] = rnd() & 0x0003ffff;
	state->celsius_rc_out[1][1] = rnd() & 0x3803ffff;
	state->celsius_rc_final[0] = rnd() & 0x3f3f3f3f;
	state->celsius_rc_final[1] = rnd() & 0x3f3f3fe0;
	state->celsius_config_a = rnd() & (is_nv17p ? 0xbfcf5fff : 0x3fcf5fff);
	state->celsius_stencil_func = rnd() & 0xfffffff1;
	state->celsius_stencil_op = rnd() & 0x00000fff;
	state->celsius_config_b = rnd() & (is_nv17p ? 0x0000fff5 : 0x00003ff5);
	state->celsius_blend = rnd() & (is_nv15p ? 0x0001ffff : 0x00000fff);
	state->celsius_unke84 = rnd();
	state->celsius_unke88 = rnd() & 0xfcffffcf;
	state->celsius_fog_color = rnd();
	state->celsius_unke90 = rnd();
	state->celsius_unke94 = rnd();
	state->celsius_unke98 = rnd();
	state->celsius_unke9c = rnd();
	state->celsius_unkea8 = rnd() & 0x000001ff;
	state->celsius_unkeac[0] = rnd() & 0x0fff0fff;
	state->celsius_unkeac[1] = rnd() & 0x0fff0fff;
	state->celsius_unkeb4 = rnd() & 0x3fffffff;
	state->celsius_unkeb8 = rnd() & 0xbfffffff;
	state->celsius_unkebc = rnd() & 0x3fffffff;
	state->celsius_unkec0 = rnd() & 0x0000ffff;
	state->celsius_unkec4 = rnd() & 0x07ffffff;
	state->celsius_unkec8 = rnd() & 0x87ffffff;
	state->celsius_unkecc = rnd() & 0x07ffffff;
	state->celsius_unked0 = rnd() & 0x0000ffff;
	state->celsius_unked4 = rnd() & 0x0000000f;
	state->celsius_unked8 = rnd() & 0x80000046;
	state->celsius_unkedc[0] = rnd();
	state->celsius_unkedc[1] = rnd();
	for (int i = 0; i < 16; i++)
		state->celsius_unkf00[i] = rnd() & 0x0fff0fff;
	state->celsius_unkf40 = rnd() & 0x3bffffff;
	state->celsius_unkf44 = rnd();
	state->celsius_unkf48 = rnd() & 0x17ff0117;
	state->celsius_unkf4c = rnd();
}

void pgraph_gen_state(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	state->chipset = nva_cards[cnum]->chipset;
	pgraph_gen_state_debug(cnum, rnd, state);
	pgraph_gen_state_control(cnum, rnd, state);
	pgraph_gen_state_vtx(cnum, rnd, state);
	pgraph_gen_state_canvas(cnum, rnd, state);
	pgraph_gen_state_rop(cnum, rnd, state);
	pgraph_gen_state_vstate(cnum, rnd, state);
	if (state->chipset.card_type == 3)
		pgraph_gen_state_dma_nv3(cnum, rnd, state);
	if (state->chipset.card_type >= 4)
		pgraph_gen_state_dma_nv4(cnum, rnd, state);
	if (state->chipset.card_type == 3)
		pgraph_gen_state_d3d0(cnum, rnd, state);
	else if (state->chipset.card_type == 4)
		pgraph_gen_state_d3d56(cnum, rnd, state);
	else if (state->chipset.card_type == 0x10)
		pgraph_gen_state_celsius(cnum, rnd, state);
	if (extr(state->debug[4], 2, 1) && extr(state->surf_type, 2, 2))
		state->unka10 |= 0x20000000;
}

void pgraph_load_vtx(int cnum, struct pgraph_state *state) {
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
			if (state->chipset.card_type == 0x10) {
				nva_wr32(cnum, 0x400550 + i * 4, state->uclip_min[2][i]);
				nva_wr32(cnum, 0x400558 + i * 4, state->uclip_max[2][i]);
			}
		}
	}
	for (int i = 0; i < pgraph_vtx_count(&state->chipset); i++) {
		nva_wr32(cnum, 0x400400 + i * 4, state->vtx_xy[i][0]);
		nva_wr32(cnum, 0x400480 + i * 4, state->vtx_xy[i][1]);
	}
	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 14; i++)
			nva_wr32(cnum, 0x400700 + i * 4, state->vtx_beta[i]);
	} else if (state->chipset.card_type < 4) {
		for (int i = 0; i < 16; i++)
			nva_wr32(cnum, 0x400580 + i * 4, state->vtx_z[i]);
	} else if (state->chipset.card_type < 0x10) {
		for (int i = 0; i < 16; i++) {
			nva_wr32(cnum, 0x400d00 + i * 4, state->vtx_u[i]);
			nva_wr32(cnum, 0x400d40 + i * 4, state->vtx_v[i]);
			nva_wr32(cnum, 0x400d80 + i * 4, state->vtx_m[i]);
		}
	}
}

void pgraph_load_rop(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type < 4) {
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
		} else {
			nva_wr32(cnum, 0x400640, state->beta);
		}
	} else {
		nva_wr32(cnum, 0x40008c, 0x01000000);
		nva_wr32(cnum, 0x400600, state->bitmap_color[0]);
		nva_wr32(cnum, 0x400604, state->rop);
		nva_wr32(cnum, 0x400608, state->beta);
		nva_wr32(cnum, 0x40060c, state->beta4);
		for (int i = 0; i < 2; i++) {
			nva_wr32(cnum, 0x400800 + i * 4, state->pattern_mono_color[i]);
			nva_wr32(cnum, 0x400808 + i * 4, state->pattern_mono_bitmap[i]);
		}
		nva_wr32(cnum, 0x400810, state->pattern_config);
		nva_wr32(cnum, 0x400814, state->chroma);
		for (int i = 0; i < 64; i++)
			nva_wr32(cnum, 0x400900 + i * 4, state->pattern_color[i]);
		nva_wr32(cnum, 0x400830, state->ctx_format);
	}
}

void pgraph_load_canvas(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type < 4) {
		if (state->chipset.card_type < 3) {
			nva_wr32(cnum, 0x400688, state->dst_canvas_min);
			nva_wr32(cnum, 0x40068c, state->dst_canvas_max);
			nva_wr32(cnum, 0x400634, state->canvas_config);
		} else {
			nva_wr32(cnum, 0x400550, state->src_canvas_min);
			nva_wr32(cnum, 0x400554, state->src_canvas_max);
			nva_wr32(cnum, 0x400558, state->dst_canvas_min);
			nva_wr32(cnum, 0x40055c, state->dst_canvas_max);
			for (int i = 0; i < 4; i++) {
				nva_wr32(cnum, 0x400630 + i * 4, state->surf_offset[i]);
				nva_wr32(cnum, 0x400650 + i * 4, state->surf_pitch[i]);
			}
			nva_wr32(cnum, 0x4006a8, state->surf_format);
		}
		for (int i = 0; i < 2; i++) {
			nva_wr32(cnum, 0x400690 + i * 8, state->cliprect_min[i]);
			nva_wr32(cnum, 0x400694 + i * 8, state->cliprect_max[i]);
		}
		nva_wr32(cnum, 0x4006a0, state->cliprect_ctrl);
	} else {
		for (int i = 0; i < 6; i++) {
			nva_wr32(cnum, 0x400640 + i * 4, state->surf_offset[i]);
			nva_wr32(cnum, 0x400658 + i * 4, state->surf_base[i]);
			nva_wr32(cnum, 0x400684 + i * 4, state->surf_limit[i]);
		}
		for (int i = 0; i < 5; i++)
			nva_wr32(cnum, 0x400670 + i * 4, state->surf_pitch[i]);
		for (int i = 0; i < 2; i++)
			nva_wr32(cnum, 0x40069c + i * 4, state->surf_swizzle[i]);
		if (state->chipset.card_type < 0x10) {
			nva_wr32(cnum, 0x40070c, state->surf_type);
			nva_wr32(cnum, 0x400710, state->ctx_valid);
		} else {
			nva_wr32(cnum, 0x400710, state->surf_type);
			nva_wr32(cnum, 0x400714, state->ctx_valid);
		}
		nva_wr32(cnum, 0x400724, state->surf_format);
	}
}

void pgraph_load_control(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type < 4) {
		nva_wr32(cnum, 0x400100, 0xffffffff);
		nva_wr32(cnum, 0x400104, 0xffffffff);
		nva_wr32(cnum, 0x400140, state->intr_en);
		nva_wr32(cnum, 0x400144, state->invalid_en);
		nva_wr32(cnum, 0x400180, state->ctx_switch[0]);
		nva_wr32(cnum, 0x400190, state->ctx_control);
		nva_wr32(cnum, 0x400680, state->ctx_switch[1]);
		nva_wr32(cnum, 0x400684, state->notify);
		if (state->chipset.card_type >= 3) {
			nva_wr32(cnum, 0x400688, state->ctx_switch[3]);
			nva_wr32(cnum, 0x40068c, state->ctx_switch[2]);
			nva_wr32(cnum, 0x400194, state->ctx_user);
			for (int i = 0; i < 8; i++)
				nva_wr32(cnum, 0x4001a0 + i * 4, state->ctx_cache[i][0]);
		}
	} else {
		nva_wr32(cnum, 0x400100, 0xffffffff);
		nva_wr32(cnum, 0x400140, state->intr_en);
		nva_wr32(cnum, 0x400104, state->nstatus);
		nva_wr32(cnum, 0x400610, state->unk610);
		nva_wr32(cnum, 0x400614, state->unk614);
		if (state->chipset.card_type < 0x10) {
			for (int j = 0; j < 4; j++) {
				nva_wr32(cnum, 0x400160 + j * 4, state->ctx_switch[j]);
				for (int i = 0; i < 8; i++) {
					nva_wr32(cnum, 0x400180 + i * 4 + j * 0x20, state->ctx_cache[i][j]);
				}
			}
			nva_wr32(cnum, 0x400170, state->ctx_control);
			nva_wr32(cnum, 0x400174, state->ctx_user);
			nva_wr32(cnum, 0x400714, state->notify);
		} else {
			for (int j = 0; j < 5; j++) {
				nva_wr32(cnum, 0x40014c + j * 4, state->ctx_switch[j]);
				for (int i = 0; i < 8; i++) {
					nva_wr32(cnum, 0x400160 + i * 4 + j * 0x20, state->ctx_cache[i][j]);
				}
			}
			nva_wr32(cnum, 0x400144, state->ctx_control);
			nva_wr32(cnum, 0x400148, state->ctx_user);
			nva_wr32(cnum, 0x400718, state->notify);
			nva_wr32(cnum, 0x40077c, state->unk77c);
			bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
			if (is_nv17p) {
				nva_wr32(cnum, 0x4006b0, state->unk6b0);
				nva_wr32(cnum, 0x400838, state->unk838);
				nva_wr32(cnum, 0x40083c, state->unk83c);
				nva_wr32(cnum, 0x400a00, state->unka00);
				nva_wr32(cnum, 0x400a04, state->unka04);
				nva_wr32(cnum, 0x400a10, state->unka10);
			}
		}
	}
}

void pgraph_load_debug(int cnum, struct pgraph_state *state) {
	nva_wr32(cnum, 0x400080, state->debug[0]);
	if (state->chipset.card_type < 0x10) {
		nva_wr32(cnum, 0x400084, state->debug[1]);
	} else {
		// fuck you
		uint32_t mangled = state->debug[1] & 0x3fffffff;
		if (state->debug[1] & 1 << 30)
			mangled |= 1 << 31;
		if (state->debug[1] & 1 << 31)
			mangled |= 1 << 30;
		nva_wr32(cnum, 0x400084, mangled);
	}
	nva_wr32(cnum, 0x400088, state->debug[2]);
	if (state->chipset.card_type >= 3)
		nva_wr32(cnum, 0x40008c, state->debug[3]);
	if (state->chipset.card_type >= 0x10)
		nva_wr32(cnum, 0x400090, state->debug[4]);
}

void pgraph_load_vstate(int cnum, struct pgraph_state *state) {
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
		if (state->chipset.card_type < 4) {
			nva_wr32(cnum, 0x40054c, state->misc32[1]);
		} else {
			nva_wr32(cnum, 0x400574, state->misc24[2]);
			nva_wr32(cnum, 0x40057c, state->misc32[1]);
			nva_wr32(cnum, 0x400580, state->misc32[2]);
			nva_wr32(cnum, 0x400584, state->misc32[3]);
			if (state->chipset.card_type < 0x10) {
				nva_wr32(cnum, 0x400760, state->dma_pitch);
				nva_wr32(cnum, 0x400764, state->dvd_format);
				nva_wr32(cnum, 0x400768, state->sifm_mode);
			} else {
				nva_wr32(cnum, 0x400770, state->dma_pitch);
				nva_wr32(cnum, 0x400774, state->dvd_format);
				nva_wr32(cnum, 0x400778, state->sifm_mode);
				nva_wr32(cnum, 0x400588, state->unk588);
				nva_wr32(cnum, 0x40058c, state->unk58c);
			}
			nva_wr32(cnum, 0x400578, state->valid[1]);
		}
		nva_wr32(cnum, 0x400508, state->valid[0]);
	}
}

void pgraph_load_dma_nv3(int cnum, struct pgraph_state *state) {
	nva_wr32(cnum, 0x401100, 0xffffffff);
	nva_wr32(cnum, 0x401140, state->dma_intr_en);
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

void pgraph_load_dma_nv4(int cnum, struct pgraph_state *state) {
	bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
	nva_wr32(cnum, 0x401000, state->dma_offset[0]);
	nva_wr32(cnum, 0x401004, state->dma_offset[1]);
	nva_wr32(cnum, 0x401008, state->dma_length);
	nva_wr32(cnum, 0x40100c, state->dma_misc);
	nva_wr32(cnum, 0x401020, state->dma_unk20[0]);
	nva_wr32(cnum, 0x401024, state->dma_unk20[1]);
	if (is_nv17p)
		nva_wr32(cnum, 0x40103c, state->dma_unk3c);
	for (int i = 0; i < 2; i++) {
		nva_wr32(cnum, 0x401040 + i * 0x40, state->dma_eng_inst[i]);
		nva_wr32(cnum, 0x401044 + i * 0x40, state->dma_eng_flags[i]);
		nva_wr32(cnum, 0x401048 + i * 0x40, state->dma_eng_limit[i]);
		nva_wr32(cnum, 0x40104c + i * 0x40, state->dma_eng_pte[i]);
		nva_wr32(cnum, 0x401050 + i * 0x40, state->dma_eng_pte_tag[i]);
		nva_wr32(cnum, 0x401054 + i * 0x40, state->dma_eng_addr_virt_adj[i]);
		nva_wr32(cnum, 0x401058 + i * 0x40, state->dma_eng_addr_phys[i]);
		nva_wr32(cnum, 0x40105c + i * 0x40, state->dma_eng_bytes[i]);
		nva_wr32(cnum, 0x401060 + i * 0x40, state->dma_eng_lines[i]);
	}
}

void pgraph_load_d3d0(int cnum, struct pgraph_state *state) {
	nva_wr32(cnum, 0x4005c0, state->d3d_tlv_xy);
	nva_wr32(cnum, 0x4005c4, state->d3d_tlv_uv[0][0]);
	nva_wr32(cnum, 0x4005c8, state->d3d_tlv_z);
	nva_wr32(cnum, 0x4005cc, state->d3d_tlv_color);
	nva_wr32(cnum, 0x4005d0, state->d3d_tlv_fog_tri_col1);
	nva_wr32(cnum, 0x4005d4, state->d3d_tlv_rhw);
	nva_wr32(cnum, 0x400644, state->d3d_config);
	nva_wr32(cnum, 0x4006c8, state->d3d_alpha);
}

void pgraph_load_d3d56(int cnum, struct pgraph_state *state) {
	for (int i = 0; i < 2; i++) {
		nva_wr32(cnum, 0x400590 + i * 8, state->d3d_rc_alpha[i]);
		nva_wr32(cnum, 0x400594 + i * 8, state->d3d_rc_color[i]);
		nva_wr32(cnum, 0x4005a8 + i * 4, state->d3d_tex_format[i]);
		nva_wr32(cnum, 0x4005b0 + i * 4, state->d3d_tex_filter[i]);
	}
	nva_wr32(cnum, 0x4005c0, state->d3d_tlv_xy);
	nva_wr32(cnum, 0x4005c4, state->d3d_tlv_uv[0][0]);
	nva_wr32(cnum, 0x4005c8, state->d3d_tlv_uv[0][1]);
	nva_wr32(cnum, 0x4005cc, state->d3d_tlv_uv[1][0]);
	nva_wr32(cnum, 0x4005d0, state->d3d_tlv_uv[1][1]);
	nva_wr32(cnum, 0x4005d4, state->d3d_tlv_z);
	nva_wr32(cnum, 0x4005d8, state->d3d_tlv_color);
	nva_wr32(cnum, 0x4005dc, state->d3d_tlv_fog_tri_col1);
	nva_wr32(cnum, 0x4005e0, state->d3d_tlv_rhw);
	nva_wr32(cnum, 0x400818, state->d3d_config);
	nva_wr32(cnum, 0x40081c, state->d3d_stencil_func);
	nva_wr32(cnum, 0x400820, state->d3d_stencil_op);
	nva_wr32(cnum, 0x400824, state->d3d_blend);
}

void pgraph_load_celsius(int cnum, struct pgraph_state *state) {
	bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
	for (int i = 0; i < 2; i++) {
		nva_wr32(cnum, 0x400e00 + i * 4, state->celsius_tex_offset[i]);
		nva_wr32(cnum, 0x400e08 + i * 4, state->celsius_tex_palette[i]);
		nva_wr32(cnum, 0x400e10 + i * 4, state->celsius_tex_format[i]);
		nva_wr32(cnum, 0x400e18 + i * 4, state->celsius_tex_control[i]);
		nva_wr32(cnum, 0x400e20 + i * 4, state->celsius_tex_pitch[i]);
		nva_wr32(cnum, 0x400e28 + i * 4, state->celsius_tex_unk238[i]);
		nva_wr32(cnum, 0x400e30 + i * 4, state->celsius_tex_rect[i]);
		nva_wr32(cnum, 0x400e38 + i * 4, state->celsius_tex_filter[i]);
		nva_wr32(cnum, 0x400e40 + i * 4, state->celsius_rc_in[0][i]);
		nva_wr32(cnum, 0x400e48 + i * 4, state->celsius_rc_in[1][i]);
		nva_wr32(cnum, 0x400e50 + i * 4, state->celsius_rc_factor[i]);
		nva_wr32(cnum, 0x400e58 + i * 4, state->celsius_rc_out[0][i]);
		nva_wr32(cnum, 0x400e60 + i * 4, state->celsius_rc_out[1][i]);
	}
	nva_wr32(cnum, 0x400e68, state->celsius_rc_final[0]);
	nva_wr32(cnum, 0x400e6c, state->celsius_rc_final[1]);
	nva_wr32(cnum, 0x400e70, state->celsius_config_a);
	nva_wr32(cnum, 0x400e74, state->celsius_stencil_func);
	nva_wr32(cnum, 0x400e78, state->celsius_stencil_op);
	nva_wr32(cnum, 0x400e7c, state->celsius_config_b);
	nva_wr32(cnum, 0x400e80, state->celsius_blend);
	nva_wr32(cnum, 0x400e84, state->celsius_unke84);
	nva_wr32(cnum, 0x400e88, state->celsius_unke88);
	nva_wr32(cnum, 0x400e8c, state->celsius_fog_color);
	nva_wr32(cnum, 0x400e90, state->celsius_unke90);
	nva_wr32(cnum, 0x400e94, state->celsius_unke94);
	nva_wr32(cnum, 0x400e98, state->celsius_unke98);
	nva_wr32(cnum, 0x400e9c, state->celsius_unke9c);
	nva_wr32(cnum, 0x400ea0, state->celsius_tex_color_key[0]);
	nva_wr32(cnum, 0x400ea4, state->celsius_tex_color_key[1]);
	nva_wr32(cnum, 0x400ea8, state->celsius_unkea8);
	if (is_nv17p) {
		nva_wr32(cnum, 0x400eac, state->celsius_unkeac[0]);
		nva_wr32(cnum, 0x400eb0, state->celsius_unkeac[1]);
		nva_wr32(cnum, 0x400eb4, state->celsius_unkeb4);
		nva_wr32(cnum, 0x400eb8, state->celsius_unkeb8);
		nva_wr32(cnum, 0x400ebc, state->celsius_unkebc);
		nva_wr32(cnum, 0x400ec0, state->celsius_unkec0);
		nva_wr32(cnum, 0x400ec4, state->celsius_unkec4);
		nva_wr32(cnum, 0x400ec8, state->celsius_unkec8);
		nva_wr32(cnum, 0x400ecc, state->celsius_unkecc);
		nva_wr32(cnum, 0x400ed0, state->celsius_unked0);
		nva_wr32(cnum, 0x400ed4, state->celsius_unked4);
		nva_wr32(cnum, 0x400ed8, state->celsius_unked8);
		nva_wr32(cnum, 0x400edc, state->celsius_unkedc[0]);
		nva_wr32(cnum, 0x400ee0, state->celsius_unkedc[1]);
	}
	for (int i = 0; i < 16; i++)
		nva_wr32(cnum, 0x400f00 + i * 4, state->celsius_unkf00[i]);
	nva_wr32(cnum, 0x400f40, state->celsius_unkf40);
	nva_wr32(cnum, 0x400f44, state->celsius_unkf44);
	nva_wr32(cnum, 0x400f48, state->celsius_unkf48);
	nva_wr32(cnum, 0x400f4c, state->celsius_unkf4c);
}

void pgraph_load_fifo(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type < 3) {
		nva_wr32(cnum, 0x4006a4, state->access);
	} else if (state->chipset.card_type < 4) {
		nva_wr32(cnum, 0x4006a4, state->fifo_enable);
	} else {
		if (state->chipset.card_type < 0x10) {
			for (int i = 0; i < 4; i++) {
				nva_wr32(cnum, 0x400730 + i * 4, state->fifo_mthd[i]);
				nva_wr32(cnum, 0x400740 + i * 4, state->fifo_data[i][0]);
			}
			nva_wr32(cnum, 0x400750, state->fifo_ptr);
			nva_wr32(cnum, 0x400754, state->fifo_mthd_st2);
			nva_wr32(cnum, 0x400758, state->fifo_data_st2[0]);
		} else {
			bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
			if (!is_nv17p) {
				for (int i = 0; i < 4; i++) {
					nva_wr32(cnum, 0x400730 + i * 4, state->fifo_mthd[i]);
					nva_wr32(cnum, 0x400740 + i * 4, state->fifo_data[i][0]);
					nva_wr32(cnum, 0x400750 + i * 4, state->fifo_data[i][1]);
				}
			} else {
				for (int i = 0; i < 8; i++) {
					nva_wr32(cnum, 0x4007a0 + i * 4, state->fifo_mthd[i]);
					nva_wr32(cnum, 0x4007c0 + i * 4, state->fifo_data[i][0]);
					nva_wr32(cnum, 0x4007e0 + i * 4, state->fifo_data[i][1]);
				}
			}
			nva_wr32(cnum, 0x400760, state->fifo_ptr);
			nva_wr32(cnum, 0x400764, state->fifo_mthd_st2);
			nva_wr32(cnum, 0x400768, state->fifo_data_st2[0]);
			nva_wr32(cnum, 0x40076c, state->fifo_data_st2[1]);
		}
		nva_wr32(cnum, 0x400720, state->fifo_enable);
	}
}

void pgraph_load_state(int cnum, struct pgraph_state *state) {
	nva_wr32(cnum, 0x000200, 0xffffeeff);
	nva_wr32(cnum, 0x000200, 0xffffffff);
	if (state->chipset.card_type < 3) {
		nva_wr32(cnum, 0x4006a4, 0x04000100);
		nva_wr32(cnum, 0x600200, state->pfb_config);
	} else if (state->chipset.card_type < 4) {
		nva_wr32(cnum, 0x4006a4, 0);
		for (int i = 0; i < 8; i++) {
			nva_wr32(cnum, 0x400648, 0x400 | i);
			nva_wr32(cnum, 0x40064c, 0xffffffff);
		}
	} else {
		nva_wr32(cnum, 0x400720, 0);
		nva_wr32(cnum, 0x400160, 0);
	}

	pgraph_load_vtx(cnum, state);
	pgraph_load_rop(cnum, state);
	pgraph_load_canvas(cnum, state);

	if (state->chipset.card_type == 3)
		pgraph_load_d3d0(cnum, state);
	else if (state->chipset.card_type == 4)
		pgraph_load_d3d56(cnum, state);
	else if (state->chipset.card_type == 0x10)
		pgraph_load_celsius(cnum, state);

	pgraph_load_vstate(cnum, state);

	if (state->chipset.card_type == 3)
		pgraph_load_dma_nv3(cnum, state);
	else if (state->chipset.card_type >= 4)
		pgraph_load_dma_nv4(cnum, state);

	pgraph_load_control(cnum, state);
	pgraph_load_debug(cnum, state);
	pgraph_load_fifo(cnum, state);
}

void pgraph_dump_fifo(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type < 4) {
		if (state->chipset.card_type < 3) {
			state->access = nva_rd32(cnum, 0x4006a4);
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
	} else {
		state->trap_addr = nva_rd32(cnum, 0x400704);
		state->trap_data[0] = nva_rd32(cnum, 0x400708);
		state->trap_data[1] = nva_rd32(cnum, 0x40070c);
		state->fifo_enable = nva_rd32(cnum, 0x400720);
		nva_wr32(cnum, 0x400720, 0);
		if (state->chipset.card_type < 0x10) {
			for (int i = 0; i < 4; i++) {
				state->fifo_mthd[i] = nva_rd32(cnum, 0x400730 + i * 4);
				state->fifo_data[i][0] = nva_rd32(cnum, 0x400740 + i * 4);
			}
			state->fifo_ptr = nva_rd32(cnum, 0x400750);
			state->fifo_mthd_st2 = nva_rd32(cnum, 0x400754);
			state->fifo_data_st2[0] = nva_rd32(cnum, 0x400758);
		} else {
			bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
			if (!is_nv17p) {
				for (int i = 0; i < 4; i++) {
					state->fifo_mthd[i] = nva_rd32(cnum, 0x400730 + i * 4);
					state->fifo_data[i][0] = nva_rd32(cnum, 0x400740 + i * 4);
					state->fifo_data[i][1] = nva_rd32(cnum, 0x400750 + i * 4);
				}
			} else {
				for (int i = 0; i < 8; i++) {
					state->fifo_mthd[i] = nva_rd32(cnum, 0x4007a0 + i * 4);
					state->fifo_data[i][0] = nva_rd32(cnum, 0x4007c0 + i * 4);
					state->fifo_data[i][1] = nva_rd32(cnum, 0x4007e0 + i * 4);
				}
			}
			state->fifo_ptr = nva_rd32(cnum, 0x400760);
			state->fifo_mthd_st2 = nva_rd32(cnum, 0x400764);
			state->fifo_data_st2[0] = nva_rd32(cnum, 0x400768);
			state->fifo_data_st2[1] = nva_rd32(cnum, 0x40076c);
		}
	}
}

void pgraph_dump_control(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type < 4) {
		state->intr = nva_rd32(cnum, 0x400100) & ~0x100;
		state->invalid = nva_rd32(cnum, 0x400104);
		state->intr_en = nva_rd32(cnum, 0x400140);
		state->invalid_en = nva_rd32(cnum, 0x400144);
		state->ctx_switch[0] = nva_rd32(cnum, 0x400180);
		state->ctx_control = nva_rd32(cnum, 0x400190) & ~0x00100000;
		if (state->chipset.card_type == 3) {
			state->ctx_user = nva_rd32(cnum, 0x400194);
			for (int i = 0; i < 8; i++)
				state->ctx_cache[i][0] = nva_rd32(cnum, 0x4001a0 + i * 4);
			state->ctx_switch[3] = nva_rd32(cnum, 0x400688);
			state->ctx_switch[2] = nva_rd32(cnum, 0x40068c);
		}
		state->ctx_switch[1] = nva_rd32(cnum, 0x400680);
		state->notify = nva_rd32(cnum, 0x400684);
	} else {
		state->intr = nva_rd32(cnum, 0x400100);
		state->intr_en = nva_rd32(cnum, 0x400140);
		state->nstatus = nva_rd32(cnum, 0x400104);
		state->nsource = nva_rd32(cnum, 0x400108);
		if (state->chipset.card_type < 0x10) {
			for (int j = 0; j < 4; j++) {
				state->ctx_switch[j] = nva_rd32(cnum, 0x400160 + j * 4);
				for (int i = 0; i < 8; i++) {
					state->ctx_cache[i][j] = nva_rd32(cnum, 0x400180 + i * 4 + j * 0x20);
				}
			}
			// eh
			state->ctx_control = nva_rd32(cnum, 0x400170) & ~0x100000;
			state->ctx_user = nva_rd32(cnum, 0x400174);
		} else {
			for (int j = 0; j < 5; j++) {
				state->ctx_switch[j] = nva_rd32(cnum, 0x40014c + j * 4);
				for (int i = 0; i < 8; i++) {
					state->ctx_cache[i][j] = nva_rd32(cnum, 0x400160 + i * 4 + j * 0x20);
				}
			}
			// eh
			state->ctx_control = nva_rd32(cnum, 0x400144) & ~0x100000;
			state->ctx_user = nva_rd32(cnum, 0x400148);
		}
		if (state->chipset.card_type < 0x10) {
			state->notify = nva_rd32(cnum, 0x400714);
		} else {
			state->notify = nva_rd32(cnum, 0x400718);
		}
		state->unk610 = nva_rd32(cnum, 0x400610);
		state->unk614 = nva_rd32(cnum, 0x400614);
		if (state->chipset.card_type >= 0x10)
			state->unk77c = nva_rd32(cnum, 0x40077c);
		bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
		if (is_nv17p) {
			state->unk6b0 = nva_rd32(cnum, 0x4006b0);
			state->unk838 = nva_rd32(cnum, 0x400838);
			state->unk83c = nva_rd32(cnum, 0x40083c);
			state->unka00 = nva_rd32(cnum, 0x400a00);
			state->unka04 = nva_rd32(cnum, 0x400a04);
			state->unka10 = nva_rd32(cnum, 0x400a10);
		}
	}
}

void pgraph_dump_vstate(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type < 3) {
		state->valid[0] = nva_rd32(cnum, 0x400650);
		state->misc32[0] = nva_rd32(cnum, 0x400654);
		state->subdivide = nva_rd32(cnum, 0x400658);
		state->edgefill = nva_rd32(cnum, 0x40065c);
		state->xy_a = nva_rd32(cnum, 0x400640);
		state->xy_misc_1[0] = nva_rd32(cnum, 0x400644);
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
		if (state->chipset.card_type < 4) {
			state->misc32[1] = nva_rd32(cnum, 0x40054c);
		} else {
			state->valid[1] = nva_rd32(cnum, 0x400578);
			state->misc24[2] = nva_rd32(cnum, 0x400574);
			state->misc32[1] = nva_rd32(cnum, 0x40057c);
			state->misc32[2] = nva_rd32(cnum, 0x400580);
			state->misc32[3] = nva_rd32(cnum, 0x400584);
			if (state->chipset.card_type < 0x10) {
				state->dma_pitch = nva_rd32(cnum, 0x400760);
				state->dvd_format = nva_rd32(cnum, 0x400764);
				state->sifm_mode = nva_rd32(cnum, 0x400768);
			} else {
				state->dma_pitch = nva_rd32(cnum, 0x400770);
				state->dvd_format = nva_rd32(cnum, 0x400774);
				state->sifm_mode = nva_rd32(cnum, 0x400778);
				state->unk588 = nva_rd32(cnum, 0x400588);
				state->unk58c = nva_rd32(cnum, 0x40058c);
			}
		}
	}
}

void pgraph_dump_vtx(int cnum, struct pgraph_state *state) {
	for (int i = 0; i < pgraph_vtx_count(&state->chipset); i++) {
		state->vtx_xy[i][0] = nva_rd32(cnum, 0x400400 + i * 4);
		state->vtx_xy[i][1] = nva_rd32(cnum, 0x400480 + i * 4);
	}
	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 14; i++) {
			state->vtx_beta[i] = nva_rd32(cnum, 0x400700 + i * 4);
		}
	} else if (state->chipset.card_type < 4) {
		for (int i = 0; i < 16; i++) {
			state->vtx_z[i] = nva_rd32(cnum, 0x400580 + i * 4);
		}
	} else if (state->chipset.card_type < 0x10) {
		for (int i = 0; i < 16; i++) {
			state->vtx_u[i] = nva_rd32(cnum, 0x400d00 + i * 4);
			state->vtx_v[i] = nva_rd32(cnum, 0x400d40 + i * 4);
			state->vtx_m[i] = nva_rd32(cnum, 0x400d80 + i * 4);
		}
	}

	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 2; i++) {
			state->iclip[i] = nva_rd32(cnum, 0x400450 + i * 4);
			state->uclip_min[0][i] = nva_rd32(cnum, 0x400460 + i * 8);
			state->uclip_max[0][i] = nva_rd32(cnum, 0x400464 + i * 8);
		}
	} else {
		for (int i = 0; i < 2; i++) {
			state->iclip[i] = nva_rd32(cnum, 0x400534 + i * 4);
			state->uclip_min[0][i] = nva_rd32(cnum, 0x40053c + i * 4);
			state->uclip_max[0][i] = nva_rd32(cnum, 0x400544 + i * 4);
			state->uclip_min[1][i] = nva_rd32(cnum, 0x400560 + i * 4);
			state->uclip_max[1][i] = nva_rd32(cnum, 0x400568 + i * 4);
			if (state->chipset.card_type >= 0x10) {
				state->uclip_min[2][i] = nva_rd32(cnum, 0x400550 + i * 4);
				state->uclip_max[2][i] = nva_rd32(cnum, 0x400558 + i * 4);
			}
		}
	}
}

void pgraph_dump_rop(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type < 4) {
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
		} else {
			state->beta = nva_rd32(cnum, 0x400640);
		}
	} else {
		state->bitmap_color[0] = nva_rd32(cnum, 0x400600);
		state->rop = nva_rd32(cnum, 0x400604);
		state->beta = nva_rd32(cnum, 0x400608);
		state->beta4 = nva_rd32(cnum, 0x40060c);
		for (int i = 0; i < 2; i++) {
			state->pattern_mono_color[i] = nva_rd32(cnum, 0x400800 + i * 4);
			state->pattern_mono_bitmap[i] = nva_rd32(cnum, 0x400808 + i * 4);
		}
		state->pattern_config = nva_rd32(cnum, 0x400810);
		state->chroma = nva_rd32(cnum, 0x400814);
		if (state->chipset.card_type == 4)
			nva_wr32(cnum, 0x40008c, 0x01000000);
		for (int i = 0; i < 64; i++)
			state->pattern_color[i] = nva_rd32(cnum, 0x400900 + i * 4);
		state->ctx_format = nva_rd32(cnum, 0x400830);
	}
}

void pgraph_dump_canvas(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type < 4) {
		if (state->chipset.card_type < 3) {
			state->canvas_config = nva_rd32(cnum, 0x400634);
			state->dst_canvas_min = nva_rd32(cnum, 0x400688);
			state->dst_canvas_max = nva_rd32(cnum, 0x40068c);
		} else {
			state->src_canvas_min = nva_rd32(cnum, 0x400550);
			state->src_canvas_max = nva_rd32(cnum, 0x400554);
			state->dst_canvas_min = nva_rd32(cnum, 0x400558);
			state->dst_canvas_max = nva_rd32(cnum, 0x40055c);
			for (int i = 0; i < 4; i++) {
				state->surf_offset[i] = nva_rd32(cnum, 0x400630 + i * 4);
				state->surf_pitch[i] = nva_rd32(cnum, 0x400650 + i * 4);
			}
			state->surf_format = nva_rd32(cnum, 0x4006a8);
		}
		for (int i = 0; i < 2; i++) {
			state->cliprect_min[i] = nva_rd32(cnum, 0x400690 + i * 8);
			state->cliprect_max[i] = nva_rd32(cnum, 0x400694 + i * 8);
		}
		state->cliprect_ctrl = nva_rd32(cnum, 0x4006a0);
	} else {
		for (int i = 0; i < 6; i++) {
			state->surf_offset[i] = nva_rd32(cnum, 0x400640 + i * 4);
			state->surf_base[i] = nva_rd32(cnum, 0x400658 + i * 4);
			state->surf_limit[i] = nva_rd32(cnum, 0x400684 + i * 4);
		}
		for (int i = 0; i < 5; i++)
			state->surf_pitch[i] = nva_rd32(cnum, 0x400670 + i * 4);
		for (int i = 0; i < 2; i++)
			state->surf_swizzle[i] = nva_rd32(cnum, 0x40069c + i * 4);
		if (state->chipset.card_type < 0x10) {
			state->surf_type = nva_rd32(cnum, 0x40070c);
			state->ctx_valid = nva_rd32(cnum, 0x400710);
		} else {
			state->surf_type = nva_rd32(cnum, 0x400710);
			state->ctx_valid = nva_rd32(cnum, 0x400714);
		}
		state->surf_format = nva_rd32(cnum, 0x400724);
	}
}

void pgraph_dump_d3d0(int cnum, struct pgraph_state *state) {
	state->d3d_tlv_xy = nva_rd32(cnum, 0x4005c0);
	state->d3d_tlv_uv[0][0] = nva_rd32(cnum, 0x4005c4);
	state->d3d_tlv_z = nva_rd32(cnum, 0x4005c8);
	state->d3d_tlv_color = nva_rd32(cnum, 0x4005cc);
	state->d3d_tlv_fog_tri_col1 = nva_rd32(cnum, 0x4005d0);
	state->d3d_tlv_rhw = nva_rd32(cnum, 0x4005d4);
	state->d3d_config = nva_rd32(cnum, 0x400644);
	state->d3d_alpha = nva_rd32(cnum, 0x4006c8);
}

void pgraph_dump_d3d56(int cnum, struct pgraph_state *state) {
	for (int i = 0; i < 2; i++) {
		state->d3d_rc_alpha[i] = nva_rd32(cnum, 0x400590 + i * 8);
		state->d3d_rc_color[i] = nva_rd32(cnum, 0x400594 + i * 8);
		state->d3d_tex_format[i] = nva_rd32(cnum, 0x4005a8 + i * 4);
		state->d3d_tex_filter[i] = nva_rd32(cnum, 0x4005b0 + i * 4);
	}
	state->d3d_tlv_xy = nva_rd32(cnum, 0x4005c0);
	state->d3d_tlv_uv[0][0] = nva_rd32(cnum, 0x4005c4);
	state->d3d_tlv_uv[0][1] = nva_rd32(cnum, 0x4005c8);
	state->d3d_tlv_uv[1][0] = nva_rd32(cnum, 0x4005cc);
	state->d3d_tlv_uv[1][1] = nva_rd32(cnum, 0x4005d0);
	state->d3d_tlv_z = nva_rd32(cnum, 0x4005d4);
	state->d3d_tlv_color = nva_rd32(cnum, 0x4005d8);
	state->d3d_tlv_fog_tri_col1 = nva_rd32(cnum, 0x4005dc);
	state->d3d_tlv_rhw = nva_rd32(cnum, 0x4005e0);
	state->d3d_config = nva_rd32(cnum, 0x400818);
	state->d3d_stencil_func = nva_rd32(cnum, 0x40081c);
	state->d3d_stencil_op = nva_rd32(cnum, 0x400820);
	state->d3d_blend = nva_rd32(cnum, 0x400824);
}

void pgraph_dump_celsius(int cnum, struct pgraph_state *state) {
	for (int i = 0; i < 2; i++) {
		state->celsius_tex_offset[i] = nva_rd32(cnum, 0x400e00 + i * 4);
		state->celsius_tex_palette[i] = nva_rd32(cnum, 0x400e08 + i * 4);
		state->celsius_tex_format[i] = nva_rd32(cnum, 0x400e10 + i * 4);
		state->celsius_tex_control[i] = nva_rd32(cnum, 0x400e18 + i * 4);
		state->celsius_tex_pitch[i] = nva_rd32(cnum, 0x400e20 + i * 4);
		state->celsius_tex_unk238[i] = nva_rd32(cnum, 0x400e28 + i * 4);
		state->celsius_tex_rect[i] = nva_rd32(cnum, 0x400e30 + i * 4);
		state->celsius_tex_filter[i] = nva_rd32(cnum, 0x400e38 + i * 4);
		state->celsius_rc_in[0][i] = nva_rd32(cnum, 0x400e40 + i * 4);
		state->celsius_rc_in[1][i] = nva_rd32(cnum, 0x400e48 + i * 4);
		state->celsius_rc_factor[i] = nva_rd32(cnum, 0x400e50 + i * 4);
		state->celsius_rc_out[0][i] = nva_rd32(cnum, 0x400e58 + i * 4);
		state->celsius_rc_out[1][i] = nva_rd32(cnum, 0x400e60 + i * 4);
	}
	state->celsius_rc_final[0] = nva_rd32(cnum, 0x400e68);
	state->celsius_rc_final[1] = nva_rd32(cnum, 0x400e6c);
	state->celsius_config_a = nva_rd32(cnum, 0x400e70);
	state->celsius_stencil_func = nva_rd32(cnum, 0x400e74);
	state->celsius_stencil_op = nva_rd32(cnum, 0x400e78);
	state->celsius_config_b = nva_rd32(cnum, 0x400e7c);
	state->celsius_blend = nva_rd32(cnum, 0x400e80);
	state->celsius_unke84 = nva_rd32(cnum, 0x400e84);
	state->celsius_unke88 = nva_rd32(cnum, 0x400e88);
	state->celsius_fog_color = nva_rd32(cnum, 0x400e8c);
	state->celsius_unke90 = nva_rd32(cnum, 0x400e90);
	state->celsius_unke94 = nva_rd32(cnum, 0x400e94);
	state->celsius_unke98 = nva_rd32(cnum, 0x400e98);
	state->celsius_unke9c = nva_rd32(cnum, 0x400e9c);
	state->celsius_tex_color_key[0] = nva_rd32(cnum, 0x400ea0);
	state->celsius_tex_color_key[1] = nva_rd32(cnum, 0x400ea4);
	state->celsius_unkea8 = nva_rd32(cnum, 0x400ea8);
	bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
	if (is_nv17p) {
		state->celsius_unkeac[0] = nva_rd32(cnum, 0x400eac);
		state->celsius_unkeac[1] = nva_rd32(cnum, 0x400eb0);
		state->celsius_unkeb4 = nva_rd32(cnum, 0x400eb4);
		state->celsius_unkeb8 = nva_rd32(cnum, 0x400eb8);
		state->celsius_unkebc = nva_rd32(cnum, 0x400ebc);
		state->celsius_unkec0 = nva_rd32(cnum, 0x400ec0);
		state->celsius_unkec4 = nva_rd32(cnum, 0x400ec4);
		state->celsius_unkec8 = nva_rd32(cnum, 0x400ec8);
		state->celsius_unkecc = nva_rd32(cnum, 0x400ecc);
		state->celsius_unked0 = nva_rd32(cnum, 0x400ed0);
		state->celsius_unked4 = nva_rd32(cnum, 0x400ed4);
		state->celsius_unked8 = nva_rd32(cnum, 0x400ed8);
		state->celsius_unkedc[0] = nva_rd32(cnum, 0x400edc);
		state->celsius_unkedc[1] = nva_rd32(cnum, 0x400ee0);
	}
	for (int i = 0; i < 16; i++)
		state->celsius_unkf00[i] = nva_rd32(cnum, 0x400f00 + i * 4);
	state->celsius_unkf40 = nva_rd32(cnum, 0x400f40);
	state->celsius_unkf44 = nva_rd32(cnum, 0x400f44);
	state->celsius_unkf48 = nva_rd32(cnum, 0x400f48);
	state->celsius_unkf4c = nva_rd32(cnum, 0x400f4c);
}

void pgraph_dump_debug(int cnum, struct pgraph_state *state) {
	state->debug[0] = nva_rd32(cnum, 0x400080);
	state->debug[1] = nva_rd32(cnum, 0x400084);
	state->debug[2] = nva_rd32(cnum, 0x400088);
	if (state->chipset.card_type >= 3)
		state->debug[3] = nva_rd32(cnum, 0x40008c);
	if (state->chipset.card_type >= 0x10)
		state->debug[4] = nva_rd32(cnum, 0x400090);
	if (state->chipset.card_type >= 0x10)
		nva_wr32(cnum, 0x400080, 0);
}

void pgraph_dump_dma_nv3(int cnum, struct pgraph_state *state) {
	state->dma_intr = nva_rd32(cnum, 0x401100);
	state->dma_intr_en = nva_rd32(cnum, 0x401140);
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
}

void pgraph_dump_dma_nv4(int cnum, struct pgraph_state *state) {
	state->dma_offset[0] = nva_rd32(cnum, 0x401000);
	state->dma_offset[1] = nva_rd32(cnum, 0x401004);
	state->dma_length = nva_rd32(cnum, 0x401008);
	state->dma_misc = nva_rd32(cnum, 0x40100c);
	state->dma_unk20[0] = nva_rd32(cnum, 0x401020);
	state->dma_unk20[1] = nva_rd32(cnum, 0x401024);
	bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
	if (is_nv17p)
		state->dma_unk3c = nva_rd32(cnum, 0x40103c);
	for (int i = 0; i < 2; i++) {
		state->dma_eng_inst[i] = nva_rd32(cnum, 0x401040 + i * 0x40);
		state->dma_eng_flags[i] = nva_rd32(cnum, 0x401044 + i * 0x40);
		state->dma_eng_limit[i] = nva_rd32(cnum, 0x401048 + i * 0x40);
		state->dma_eng_pte[i] = nva_rd32(cnum, 0x40104c + i * 0x40);
		state->dma_eng_pte_tag[i] = nva_rd32(cnum, 0x401050 + i * 0x40);
		state->dma_eng_addr_virt_adj[i] = nva_rd32(cnum, 0x401054 + i * 0x40);
		state->dma_eng_addr_phys[i] = nva_rd32(cnum, 0x401058 + i * 0x40);
		state->dma_eng_bytes[i] = nva_rd32(cnum, 0x40105c + i * 0x40);
		state->dma_eng_lines[i] = nva_rd32(cnum, 0x401060 + i * 0x40);
	}
}

void pgraph_dump_state(int cnum, struct pgraph_state *state) {
	state->chipset = nva_cards[cnum]->chipset;
	int ctr = 0;
	while((state->status = nva_rd32(cnum, state->chipset.card_type < 4 ? 0x4006b0: 0x400700))) {
		ctr++;
		if (ctr > 100000) {
			fprintf(stderr, "PGRAPH locked up [%08x]!\n", state->status);
			nva_wr32(cnum, 0x000200, 0xffffefff);
			nva_wr32(cnum, 0x000200, 0xffffffff);
			break;
		}
	}
	pgraph_dump_debug(cnum, state);
	pgraph_dump_fifo(cnum, state);
	pgraph_dump_control(cnum, state);
	pgraph_dump_vstate(cnum, state);
	pgraph_dump_vtx(cnum, state);
	pgraph_dump_rop(cnum, state);
	pgraph_dump_canvas(cnum, state);

	if (state->chipset.card_type == 3)
		pgraph_dump_d3d0(cnum, state);
	else if (state->chipset.card_type == 4)
		pgraph_dump_d3d56(cnum, state);
	else if (state->chipset.card_type == 0x10)
		pgraph_dump_celsius(cnum, state);

	if (state->chipset.card_type == 3)
		pgraph_dump_dma_nv3(cnum, state);
	else if (state->chipset.card_type >= 4)
		pgraph_dump_dma_nv4(cnum, state);
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
