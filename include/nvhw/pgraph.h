/*
 * Copyright (C) 2012-2016 Marcin Kościelnicki <koriakin@0x04.net>
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

#ifndef PGRAPH_H
#define PGRAPH_H

#include <stdint.h>
#include <stdbool.h>
#include "util.h"
#include "nvhw/chipset.h"

#ifdef __cplusplus
extern "C" {
#endif

enum pgraph_type {
	PGRAPH_NV01,
	PGRAPH_NV03,
	PGRAPH_NV04,
	PGRAPH_NV10,
	PGRAPH_NV20,
	PGRAPH_NV40,
	PGRAPH_NV50,
	PGRAPH_NVC0,
};

int pgraph_type(int chipset);

struct pgraph_state {
	struct chipset_info chipset;
	uint32_t debug[5];
	uint32_t intr;
	uint32_t intr_en;
	uint32_t invalid;
	uint32_t invalid_en;
	uint32_t dma_intr;
	uint32_t dma_intr_en;
	uint32_t nstatus;
	uint32_t nsource;
	uint32_t ctx_switch[5];
	uint32_t ctx_cache[8][5];
	uint32_t ctx_control;
	uint32_t ctx_user;
	uint32_t unk610;
	uint32_t unk614;
	uint32_t unk77c;
	uint32_t unk6b0;
	uint32_t unk838;
	uint32_t unk83c;
	uint32_t zcull_unka00[2];
	uint32_t unka10;
	uint32_t access;
	uint32_t fifo_enable;
	uint32_t fifo_mthd[8];
	uint32_t fifo_data[8][2];
	uint32_t fifo_ptr;
	uint32_t fifo_mthd_st2;
	uint32_t fifo_data_st2[2];
	uint32_t status;
	uint32_t trap_addr;
	uint32_t trap_data[2];
	uint32_t trap_grctx;
	uint32_t vtx_xy[32][2];
	uint32_t vtx_beta[14];
	uint32_t vtx_z[16];
	uint32_t vtx_u[16];
	uint32_t vtx_v[16];
	uint32_t vtx_m[16];
	uint32_t iclip[2];
	uint32_t uclip_min[3][2];
	uint32_t uclip_max[3][2];
	uint32_t xy_a;
	uint32_t xy_misc_1[2];
	uint32_t xy_misc_3;
	uint32_t xy_misc_4[2];
	uint32_t xy_clip[2][2];
	uint32_t subdivide;
	uint32_t edgefill;
	uint32_t valid[2];
	uint32_t misc24[3];
	uint32_t misc32[4];
	uint32_t dma_pitch;
	uint32_t dvd_format;
	uint32_t sifm_mode;
	uint32_t unk588;
	uint32_t unk58c;
	uint32_t bitmap_color[2];
	uint32_t rop;
	uint32_t beta;
	uint32_t beta4;
	uint32_t plane;
	uint32_t chroma;
	uint32_t pattern_mono_rgb[2];
	uint32_t pattern_mono_a[2];
	uint32_t pattern_mono_color[2];
	uint32_t pattern_mono_bitmap[2];
	uint32_t pattern_config;
	uint32_t pattern_color[64];
	uint32_t surf_base[6];
	uint32_t surf_offset[6];
	uint32_t surf_limit[6];
	uint32_t surf_pitch[5];
	uint32_t surf_swizzle[2];
	uint32_t surf_type;
	uint32_t surf_format;
	uint32_t ctx_valid;
	uint32_t ctx_format;
	uint32_t notify;
	uint32_t canvas_config;
	uint32_t src_canvas_min;
	uint32_t src_canvas_max;
	uint32_t dst_canvas_min;
	uint32_t dst_canvas_max;
	uint32_t cliprect_min[2];
	uint32_t cliprect_max[2];
	uint32_t cliprect_ctrl;
	uint32_t pfb_config;
	uint32_t pfb_boot;
	uint32_t d3d0_tlv_xy;
	uint32_t d3d0_tlv_uv;
	uint32_t d3d0_tlv_z;
	uint32_t d3d0_tlv_color;
	uint32_t d3d0_tlv_fog_tri_col1;
	uint32_t d3d0_tlv_rhw;
	uint32_t d3d0_config;
	uint32_t d3d0_alpha;
	uint32_t d3d56_rc_alpha[2];
	uint32_t d3d56_rc_color[2];
	uint32_t d3d56_tex_format[2];
	uint32_t d3d56_tex_filter[2];
	uint32_t d3d56_tlv_xy;
	uint32_t d3d56_tlv_uv[2][2];
	uint32_t d3d56_tlv_z;
	uint32_t d3d56_tlv_color;
	uint32_t d3d56_tlv_fog_tri_col1;
	uint32_t d3d56_tlv_rhw;
	uint32_t d3d56_config;
	uint32_t d3d56_stencil_func;
	uint32_t d3d56_stencil_op;
	uint32_t d3d56_blend;
	uint32_t dma_offset[3];
	uint32_t dma_length;
	uint32_t dma_misc;
	uint32_t dma_unk20[2];
	uint32_t dma_unk3c;
	uint32_t dma_eng_inst[2];
	uint32_t dma_eng_flags[2];
	uint32_t dma_eng_limit[2];
	uint32_t dma_eng_pte[2];
	uint32_t dma_eng_pte_tag[2];
	uint32_t dma_eng_addr_virt_adj[2];
	uint32_t dma_eng_addr_phys[2];
	uint32_t dma_eng_bytes[2];
	uint32_t dma_eng_lines[2];
	uint32_t dma_lin_limit;
	uint32_t celsius_tex_offset[2];
	uint32_t celsius_tex_palette[2];
	uint32_t celsius_tex_format[2];
	uint32_t celsius_tex_control[2];
	uint32_t celsius_tex_pitch[2];
	uint32_t celsius_tex_unk238[2];
	uint32_t celsius_tex_rect[2];
	uint32_t celsius_tex_filter[2];
	uint32_t celsius_rc_in[2][2];
	uint32_t celsius_rc_factor[2];
	uint32_t celsius_rc_out[2][2];
	uint32_t celsius_rc_final[2];
	uint32_t celsius_config_a;
	uint32_t celsius_stencil_func;
	uint32_t celsius_stencil_op;
	uint32_t celsius_config_b;
	uint32_t celsius_blend;
	uint32_t celsius_unke84;
	uint32_t celsius_unke88;
	uint32_t celsius_fog_color;
	uint32_t celsius_unke90;
	uint32_t celsius_unke94;
	uint32_t celsius_unke98;
	uint32_t celsius_unke9c;
	uint32_t celsius_tex_color_key[2];
	uint32_t celsius_unkea8;
	uint32_t celsius_clear_hv[2];
	uint32_t celsius_surf_base_zcull;
	uint32_t celsius_surf_limit_zcull;
	uint32_t celsius_surf_offset_zcull;
	uint32_t celsius_surf_pitch_zcull;
	uint32_t celsius_surf_base_clipid;
	uint32_t celsius_surf_limit_clipid;
	uint32_t celsius_surf_offset_clipid;
	uint32_t celsius_surf_pitch_clipid;
	uint32_t celsius_clipid_id;
	uint32_t celsius_config_d;
	uint32_t celsius_clear_zeta;
	uint32_t celsius_mthd_unk3fc;
	uint32_t celsius_unkf00[16];
	uint32_t celsius_unkf40;
	uint32_t celsius_unkf44;
	uint32_t celsius_unkf48;
	uint32_t celsius_dma;
};

enum {
	NV01_VTX_ZERO = 18,
	NV01_VTX_CANVAS = 19,
};

struct pgraph_color {
	uint16_t r, g, b;
	uint8_t a, i;
	uint16_t i16;
	enum {
		COLOR_MODE_RGB5,
		COLOR_MODE_RGB8,
		COLOR_MODE_RGB10,
		COLOR_MODE_Y8,
		COLOR_MODE_Y16,
	} mode;
};

void pgraph_reset(struct pgraph_state *state);
void pgraph_volatile_reset(struct pgraph_state *state);
struct pgraph_color pgraph_expand_color(struct pgraph_state *state, uint32_t color);
struct pgraph_color nv01_pgraph_expand_surf(struct pgraph_state *state, uint32_t pixel);
struct pgraph_color nv03_pgraph_expand_surf(int fmt, uint32_t pixel);
uint32_t pgraph_to_a1r10g10b10(struct pgraph_color color);
uint32_t pgraph_expand_mono(struct pgraph_state *state, uint32_t mono);
int nv01_pgraph_cpp(uint32_t pfb_config);
int pgraph_cpp_in(struct pgraph_state *state);
uint32_t nv01_pgraph_pixel_addr(struct pgraph_state *state, int x, int y, int buf);
uint32_t nv01_pgraph_rop(struct pgraph_state *state, int x, int y, uint32_t pixel, struct pgraph_color src);
uint32_t nv01_pgraph_solid_rop(struct pgraph_state *state, int x, int y, uint32_t pixel);
int nv03_pgraph_surf_format(struct pgraph_state *state);
int nv03_pgraph_cpp(struct pgraph_state *state);
int nv01_pgraph_dither_10to5(int val, int x, int y, bool isg);
uint32_t nv03_pgraph_rop(struct pgraph_state *state, int x, int y, uint32_t pixel, struct pgraph_color src);
uint32_t nv03_pgraph_solid_rop(struct pgraph_state *state, int x, int y, uint32_t pixel);
bool pgraph_cliprect_pass(struct pgraph_state *state, int32_t x, int32_t y);
void pgraph_prep_draw(struct pgraph_state *state, bool poly, bool noclip);

/* pgraph_xy.c */
void pgraph_set_xy_d(struct pgraph_state *state, int xy, int idx, int sid, bool carry, bool oob, bool ovf, int cstat);
int nv01_pgraph_use_v16(struct pgraph_state *state);
void nv01_pgraph_clip_bounds(struct pgraph_state *state, int32_t min[2], int32_t max[2]);
void nv01_pgraph_vtx_fixup(struct pgraph_state *state, int xy, int idx, int32_t coord, int rel, int ridx, int sid);
void nv01_pgraph_vtx_add(struct pgraph_state *state, int xy, int idx, int sid, uint32_t a, uint32_t b, uint32_t c);
void nv01_pgraph_iclip_fixup(struct pgraph_state *state, int xy, int32_t coord, int rel);
void nv01_pgraph_uclip_fixup(struct pgraph_state *state, int xy, int idx, int32_t coord, int rel);
void nv01_pgraph_set_clip(struct pgraph_state *state, int is_size, uint32_t val);
void nv01_pgraph_set_vtx(struct pgraph_state *state, int xy, int idx, int32_t coord, bool is32);
uint32_t pgraph_vtxid(struct pgraph_state *state);
void pgraph_clear_vtxid(struct pgraph_state *state);
void pgraph_bump_vtxid(struct pgraph_state *state);
bool pgraph_image_zero(struct pgraph_state *state);
void pgraph_set_image_zero(struct pgraph_state *state, bool val);

/* pgraph_xy3.c */
int nv03_pgraph_clip_status(struct pgraph_state *state, int32_t coord, int xy, bool canvas_only);
void nv03_pgraph_vtx_fixup(struct pgraph_state *state, int xy, int idx, int32_t coord);
void nv03_pgraph_iclip_fixup(struct pgraph_state *state, int xy, int32_t coord);
void nv03_pgraph_uclip_fixup(struct pgraph_state *state, int which, int xy, int idx, int32_t coord);
void nv03_pgraph_set_clip(struct pgraph_state *state, int which, int idx, uint32_t val, bool prev_inited);
void nv03_pgraph_vtx_add(struct pgraph_state *state, int xy, int idx, uint32_t a, uint32_t b, uint32_t c, bool noclip, bool nostat);

/* pgraph_xy4.c */
int nv04_pgraph_clip_status(struct pgraph_state *state, int32_t coord, int xy);
void nv04_pgraph_vtx_fixup(struct pgraph_state *state, int xy, int idx, int32_t coord);
void nv04_pgraph_iclip_fixup(struct pgraph_state *state, int xy, int32_t coord);
void nv04_pgraph_uclip_write(struct pgraph_state *state, int which, int xy, int idx, int32_t coord);
uint32_t nv04_pgraph_formats(struct pgraph_state *state);
void nv04_pgraph_blowup(struct pgraph_state *state, uint32_t nsource);
void pgraph_state_error(struct pgraph_state *state);
void nv04_pgraph_set_chroma_nv01(struct pgraph_state *state, uint32_t val);
void nv04_pgraph_set_pattern_mono_color_nv01(struct pgraph_state *state, int idx, uint32_t val);
void nv04_pgraph_set_bitmap_color_0_nv01(struct pgraph_state *state, uint32_t val);
void nv04_pgraph_set_clip(struct pgraph_state *state, int which, int idx, uint32_t val);
bool nv04_pgraph_is_3d_class(struct pgraph_state *state);
bool nv04_pgraph_is_sync_class(struct pgraph_state *state);
bool nv04_pgraph_is_syncable_class(struct pgraph_state *state);
bool nv04_pgraph_is_new_render_class(struct pgraph_state *state);
bool nv04_pgraph_is_sync(struct pgraph_state *state);
uint32_t nv04_pgraph_bswap(struct pgraph_state *state, uint32_t val);
uint32_t nv04_pgraph_hswap(struct pgraph_state *state, uint32_t val);
void nv04_pgraph_vtx_add(struct pgraph_state *state, int xy, int idx, uint32_t a, uint32_t b, uint32_t c, bool nostat);

/* pgraph_d3d_nv3.c */
uint16_t nv03_pgraph_convert_xy(uint32_t val);
uint32_t nv03_pgraph_convert_z(uint32_t val);
uint16_t nv03_pgraph_convert_uv(uint32_t val, int sz);
bool nv03_pgraph_d3d_cmp(int func, uint32_t a, uint32_t b);
bool nv03_pgraph_d3d_wren(int func, bool zeta_test, bool alpha_test);
uint16_t nv03_pgraph_zpoint_rop(struct pgraph_state *state, int32_t x, int32_t y, uint16_t pixel);

static inline uint32_t pgraph_class(struct pgraph_state *state) {
	if (state->chipset.card_type < 3) {
		return extr(state->access, 12, 5);
	} else if (state->chipset.card_type < 4) {
		return extr(state->ctx_user, 16, 5);
	} else {
		return extr(state->ctx_switch[0], 0, 8);
	}
}

static inline int pgraph_vtx_count(struct chipset_info *chipset) {
	if (chipset->card_type < 3) {
		return 18;
	} else if (chipset->card_type < 0x10) {
		return 32;
	} else {
		return 10;
	}
}

static inline void pgraph_vtx_cmp(struct pgraph_state *state, int xy, int idx, bool weird) {
	int32_t val = state->vtx_xy[idx][xy];
	if (weird) {
		val <<= 1;
	}
	int stat = 0;
	if (val < 0)
		stat = 1;
	else if (val > 0)
		stat = 2;
	insrt(state->xy_misc_4[xy], 28, 2, stat);
}

static inline bool nv01_pgraph_is_tex_class(struct pgraph_state *state) {
	if (state->chipset.card_type >= 3)
		return false;
	uint32_t cls = pgraph_class(state);
	bool is_texlin_class = (cls & 0xf) == 0xd;
	bool is_texquad_class = (cls & 0xf) == 0xe;
	return is_texlin_class || is_texquad_class;
}

static inline bool nv01_pgraph_is_solid_class(int cls) {
	return cls >= 8 && cls <= 0xc;
}

static inline bool nv01_pgraph_is_drawable_class(int cls) {
	return nv01_pgraph_is_solid_class(cls) || (cls >= 0x10 && cls <= 0x13);
}

static inline bool nv04_pgraph_is_nv11p(const struct chipset_info *chipset) {
	return chipset->chipset > 0x10 && chipset->chipset != 0x15;
}

static inline bool nv04_pgraph_is_nv15p(const struct chipset_info *chipset) {
	return chipset->chipset > 0x10;
}

static inline bool nv04_pgraph_is_nv17p(const struct chipset_info *chipset) {
	return chipset->chipset >= 0x17 && chipset->chipset != 0x1a;
}

static inline uint32_t pgraph_offset_mask(const struct chipset_info *chipset) {
	if (chipset->chipset < 4)
		return chipset->is_nv03t ? 0x007ffff0 : 0x003ffff0;
	if (chipset->chipset < 5)
		return 0x00fffff0;
	else if (chipset->card_type < 0x10)
		return 0x01fffff0;
	else
		return 0x07fffff0;
}

static inline uint32_t pgraph_pitch_mask(const struct chipset_info *chipset) {
	if (chipset->card_type < 0x10)
		return 0x1ff0;
	else
		return 0xfff0;
}

static inline bool pgraph_is_class_line(struct pgraph_state *state) {
	uint32_t cls = pgraph_class(state);
	if (state->chipset.card_type < 4) {
		return cls == 0x9 || cls == 0xa;
	} else {
		return cls == 0x1c || cls == 0x5c;
	}
}

static inline bool pgraph_is_class_tri(struct pgraph_state *state) {
	uint32_t cls = pgraph_class(state);
	if (state->chipset.card_type < 4) {
		return cls == 0xb;
	} else {
		return cls == 0x1d || cls == 0x5d;
	}
}

static inline bool pgraph_is_class_blit(struct pgraph_state *state) {
	uint32_t cls = pgraph_class(state);
	if (state->chipset.card_type < 4) {
		return cls == 0x10;
	} else {
		return cls == 0x1f || cls == 0x5f || (nv04_pgraph_is_nv15p(&state->chipset) && cls == 0x9f);
	}
}

static inline bool pgraph_is_class_dvd(struct pgraph_state *state) {
	uint32_t cls = pgraph_class(state);
	if (state->chipset.card_type < 4)
		return false;
	if (cls == 0x38)
		return true;
	if (cls == 0x88 && state->chipset.card_type >= 0x10)
		return true;
	return false;
}

static inline bool pgraph_is_class_sifc(struct pgraph_state *state) {
	uint32_t cls = pgraph_class(state);
	if (state->chipset.card_type < 3) {
		return false;
	} else if (state->chipset.card_type < 4) {
		return cls == 0x15;
	} else {
		if (cls == 0x36 || cls == 0x76)
			return true;
		if (state->chipset.chipset >= 5 && cls == 0x66)
			return true;
		return false;
	}
}

static inline bool pgraph_is_class_rect(struct pgraph_state *state) {
	uint32_t cls = pgraph_class(state);
	if (state->chipset.card_type < 3) {
		return cls == 0xc;
	} else if (state->chipset.card_type < 4) {
		return cls == 7 || cls == 0xc;
	} else {
		if (cls == 0x1e || cls == 0x5e)
			return true;
		if (cls == 0x4b || cls == 0x4a)
			return true;
		return false;
	}
}

static inline bool pgraph_is_class_itm(struct pgraph_state *state) {
	uint32_t cls = pgraph_class(state);
	if (state->chipset.card_type < 4) {
		return cls == 0x14;
	} else {
		return false;
	}
}

static inline bool pgraph_is_class_ifc_nv1(struct pgraph_state *state) {
	uint32_t cls = pgraph_class(state);
	if (state->chipset.card_type < 3) {
		return cls == 0x11 || cls == 0x12 || cls == 0x13;
	}
	return false;
}

#ifdef __cplusplus
}
#endif

#endif
