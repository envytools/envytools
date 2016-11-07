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

struct nv01_pgraph_state {
	uint32_t intr;
	uint32_t invalid;
	uint32_t intr_en;
	uint32_t invalid_en;
	uint32_t ctx_switch;
	uint32_t ctx_control;
	uint32_t iclip[2];
	uint32_t uclip_min[2];
	uint32_t uclip_max[2];
	uint32_t vtx_x[18];
	uint32_t vtx_y[18];
	uint32_t vtx_beta[14];
	uint32_t pattern_rgb[2];
	uint32_t pattern_a[2];
	uint32_t pattern_bitmap[2];
	uint32_t pattern_shape;
	uint32_t bitmap_color[2];
	uint32_t rop;
	uint32_t plane;
	uint32_t chroma;
	uint32_t beta;
	uint32_t canvas_config;
	uint32_t canvas_min;
	uint32_t canvas_max;
	uint32_t cliprect_min[2];
	uint32_t cliprect_max[2];
	uint32_t cliprect_ctrl;
	uint32_t valid;
	uint32_t source_color;
	uint32_t subdivide;
	uint32_t edgefill;
	uint32_t xy_misc_0;
	uint32_t xy_misc_1;
	uint32_t xy_misc_2[2];
	uint32_t dma;
	uint32_t notify;
	uint32_t access;
	uint32_t debug[3];
	uint32_t status;
	/* not exactly PGRAPH reg, but important */
	uint32_t pfb_config;
	uint32_t pfb_boot;
};

struct nv03_pgraph_state {
	uint32_t intr;
	uint32_t invalid;
	uint32_t dma_intr;
	uint32_t intr_en;
	uint32_t invalid_en;
	uint32_t dma_intr_en;
	uint32_t ctx_switch;
	uint32_t ctx_control;
	uint32_t ctx_user;
	uint32_t ctx_cache[8];
	uint32_t iclip[2];
	uint32_t uclip_min[2];
	uint32_t uclip_max[2];
	uint32_t oclip_min[2];
	uint32_t oclip_max[2];
	uint32_t vtx_x[32];
	uint32_t vtx_y[32];
	uint32_t vtx_z[16];
	uint32_t pattern_rgb[2];
	uint32_t pattern_a[2];
	uint32_t pattern_bitmap[2];
	uint32_t pattern_shape;
	uint32_t bitmap_color_0;
	uint32_t rop;
	uint32_t chroma;
	uint32_t beta;
	uint32_t src_canvas_min;
	uint32_t src_canvas_max;
	uint32_t dst_canvas_min;
	uint32_t dst_canvas_max;
	uint32_t cliprect_min[2];
	uint32_t cliprect_max[2];
	uint32_t cliprect_ctrl;
	uint32_t xy_misc_0;
	uint32_t xy_misc_1[2];
	uint32_t xy_misc_3;
	uint32_t xy_misc_4[2];
	uint32_t xy_clip[2][2];
	uint32_t valid;
	uint32_t misc32_0;
	uint32_t misc32_1;
	uint32_t misc24_0;
	uint32_t misc24_1;
	uint32_t d3d_xy;
	uint32_t d3d_uv;
	uint32_t d3d_z;
	uint32_t d3d_rgb;
	uint32_t d3d_fog_tri;
	uint32_t d3d_m;
	uint32_t d3d_config;
	uint32_t d3d_alpha;
	uint32_t dma;
	uint32_t notify;
	uint32_t grobj;
	uint32_t m2mf;
	uint32_t surf_offset[4];
	uint32_t surf_pitch[4];
	uint32_t surf_format;
	uint32_t dma_flags;
	uint32_t dma_limit;
	uint32_t dma_pte;
	uint32_t dma_pte_tag;
	uint32_t dma_addr_virt_adj;
	uint32_t dma_addr_phys;
	uint32_t dma_bytes;
	uint32_t dma_inst;
	uint32_t dma_lines;
	uint32_t dma_lin_limit;
	uint32_t dma_offset[3];
	uint32_t dma_pitch;
	uint32_t dma_format;
	uint32_t access;
	uint32_t debug[4];
	uint32_t status;
};

struct nv04_pgraph_state {
	struct chipset_info chipset;
	uint32_t debug[5];
	uint32_t intr;
	uint32_t intr_en;
	uint32_t nstatus;
	uint32_t nsource;
	uint32_t ctx_switch[5];
	uint32_t ctx_cache[8][5];
	uint32_t ctx_control;
	uint32_t ctx_user;
	uint32_t unk610;
	uint32_t unk614;
	uint32_t unk77c;
	uint32_t fifo_enable;
	uint32_t fifo_mthd[4];
	uint32_t fifo_data[4][2];
	uint32_t fifo_ptr;
	uint32_t fifo_mthd_st2;
	uint32_t fifo_data_st2[2];
	uint32_t status;
	uint32_t vtx_x[32];
	uint32_t vtx_y[32];
	uint32_t vtx_u[16];
	uint32_t vtx_v[16];
	uint32_t vtx_m[16];
	uint32_t iclip[2];
	uint32_t uclip_min[2];
	uint32_t uclip_max[2];
	uint32_t oclip_min[2];
	uint32_t oclip_max[2];
	uint32_t clip3d_min[2];
	uint32_t clip3d_max[2];
	uint32_t xy_misc_0;
	uint32_t xy_misc_1[2];
	uint32_t xy_misc_3;
	uint32_t xy_misc_4[2];
	uint32_t xy_clip[2][2];
	uint32_t valid[2];
	uint32_t misc24[3];
	uint32_t misc32[4];
	uint32_t dma_pitch;
	uint32_t unk764;
	uint32_t unk768;
	uint32_t unk588;
	uint32_t unk58c;
	uint32_t bitmap_color_0;
	uint32_t rop;
	uint32_t beta;
	uint32_t beta4;
	uint32_t chroma;
	uint32_t pattern_mono_color[2];
	uint32_t pattern_mono_bitmap[2];
	uint32_t pattern_config;
	uint32_t pattern_color[64];
	uint32_t surf_base[6];
	uint32_t surf_offset[6];
	uint32_t surf_limit[6];
	uint32_t surf_pitch[5];
	uint32_t surf_swizzle[5];
	uint32_t surf_type;
	uint32_t surf_format;
	uint32_t ctx_valid;
	uint32_t ctx_format;
	uint32_t notify;
	uint32_t d3d_unk590[2];
	uint32_t d3d_unk594[2];
	uint32_t d3d_unk5a8[2];
	uint32_t d3d_unk5b0[2];
	uint32_t d3d_unk5c0;
	uint32_t d3d_unk5c4;
	uint32_t d3d_unk5c8;
	uint32_t d3d_unk5cc;
	uint32_t d3d_unk5d0;
	uint32_t d3d_unk5d4;
	uint32_t d3d_unk5d8;
	uint32_t d3d_unk5dc;
	uint32_t d3d_unk5e0;
	uint32_t d3d_unk818;
	uint32_t d3d_unk81c;
	uint32_t d3d_unk820;
	uint32_t d3d_unk824;
	uint32_t dma_offset[2];
	uint32_t dma_length;
	uint32_t dma_misc;
	uint32_t dma_unk20[2];
	uint32_t dma_eng_inst[2];
	uint32_t dma_eng_flags[2];
	uint32_t dma_eng_limit[2];
	uint32_t dma_eng_pte[2];
	uint32_t dma_eng_pte_tag[2];
	uint32_t dma_eng_addr_virt_adj[2];
	uint32_t dma_eng_addr_phys[2];
	uint32_t dma_eng_bytes[2];
	uint32_t dma_eng_lines[2];
};

enum {
	NV01_VTX_ZERO = 18,
	NV01_VTX_CANVAS = 19,
};

struct nv01_color {
	uint16_t r, g, b;
	uint8_t a, i;
	uint16_t i16;
	enum {
		NV01_MODE_RGB5,
		NV01_MODE_RGB8,
		NV01_MODE_RGB10,
		NV01_MODE_Y8,
		NV01_MODE_Y16,
	} mode;
};

struct nv01_color nv01_pgraph_expand_color(uint32_t ctx, uint32_t config, uint32_t color);
struct nv01_color nv03_pgraph_expand_color(uint32_t ctx, uint32_t color);
struct nv01_color nv01_pgraph_expand_surf(struct nv01_pgraph_state *state, uint32_t pixel);
struct nv01_color nv03_pgraph_expand_surf(int fmt, uint32_t pixel);
uint32_t nv01_pgraph_expand_a1r10g10b10(uint32_t ctx, uint32_t config, uint32_t color);
uint32_t nv03_pgraph_expand_a1r10g10b10(uint32_t ctx, uint32_t color);
uint32_t nv01_pgraph_to_a1r10g10b10(struct nv01_color color);
uint32_t nv01_pgraph_expand_mono(uint32_t ctx, uint32_t mono);
uint32_t nv03_pgraph_expand_mono(uint32_t ctx, uint32_t mono);
int nv01_pgraph_cpp(uint32_t pfb_config);
int nv01_pgraph_cpp_in(uint32_t ctx_switch);
uint32_t nv01_pgraph_pixel_addr(struct nv01_pgraph_state *state, int x, int y, int buf);
uint32_t nv01_pgraph_rop(struct nv01_pgraph_state *state, int x, int y, uint32_t pixel, struct nv01_color src);
uint32_t nv01_pgraph_solid_rop(struct nv01_pgraph_state *state, int x, int y, uint32_t pixel);
int nv03_pgraph_surf_format(struct nv03_pgraph_state *state);
int nv03_pgraph_cpp(struct nv03_pgraph_state *state);
int nv01_pgraph_dither_10to5(int val, int x, int y, bool isg);
uint32_t nv03_pgraph_rop(struct nv03_pgraph_state *state, int x, int y, uint32_t pixel, struct nv01_color src);
uint32_t nv03_pgraph_solid_rop(struct nv03_pgraph_state *state, int x, int y, uint32_t pixel);
bool nv01_pgraph_cliprect_pass(struct nv01_pgraph_state *state, int32_t x, int32_t y);
bool nv03_pgraph_cliprect_pass(struct nv03_pgraph_state *state, int32_t x, int32_t y);

/* pgraph_xy.c */
int nv01_pgraph_use_v16(struct nv01_pgraph_state *state);
void nv01_pgraph_set_xym2(struct nv01_pgraph_state *state, int xy, int idx, int sid, bool carry, bool oob, int cstat);
void nv01_pgraph_clip_bounds(struct nv01_pgraph_state *state, int32_t min[2], int32_t max[2]);
void nv01_pgraph_vtx_fixup(struct nv01_pgraph_state *state, int xy, int idx, int32_t coord, int rel, int ridx, int sid);
void nv01_pgraph_vtx_add(struct nv01_pgraph_state *state, int xy, int idx, int sid, uint32_t a, uint32_t b, uint32_t c);
void nv01_pgraph_iclip_fixup(struct nv01_pgraph_state *state, int xy, int32_t coord, int rel);
void nv01_pgraph_uclip_fixup(struct nv01_pgraph_state *state, int xy, int idx, int32_t coord, int rel);
void nv01_pgraph_set_clip(struct nv01_pgraph_state *state, int is_size, uint32_t val);
void nv01_pgraph_set_vtx(struct nv01_pgraph_state *state, int xy, int idx, int32_t coord, bool is32);
void nv01_pgraph_bump_vtxid(struct nv01_pgraph_state *state);
void nv01_pgraph_prep_draw(struct nv01_pgraph_state *state, bool poly);

/* pgraph_xy3.c */
int nv03_pgraph_clip_status(struct chipset_info *cinfo, struct nv03_pgraph_state *state, int32_t coord, int xy, bool canvas_only);
void nv03_pgraph_vtx_fixup(struct chipset_info *cinfo, struct nv03_pgraph_state *state, int xy, int idx, int32_t coord);
void nv03_pgraph_iclip_fixup(struct chipset_info *cinfo, struct nv03_pgraph_state *state, int xy, int32_t coord);
void nv03_pgraph_uclip_fixup(struct chipset_info *cinfo, struct nv03_pgraph_state *state, int uo, int xy, int idx, int32_t coord);
void nv03_pgraph_set_clip(struct chipset_info *cinfo, struct nv03_pgraph_state *state, int which, int idx, uint32_t val, bool prev_inited);
void nv03_pgraph_vtx_add(struct chipset_info *cinfo, struct nv03_pgraph_state *state, int xy, int idx, uint32_t a, uint32_t b, uint32_t c, bool noclip);
void nv03_pgraph_prep_draw(struct nv03_pgraph_state *state, bool poly, bool noclip);

/* pgraph_xy4.c */
int nv04_pgraph_clip_status(struct nv04_pgraph_state *state, int32_t coord, int xy, bool canvas_only);
void nv04_pgraph_vtx_fixup(struct nv04_pgraph_state *state, int xy, int idx, int32_t coord);
void nv04_pgraph_iclip_fixup(struct nv04_pgraph_state *state, int xy, int32_t coord);
void nv04_pgraph_uclip_write(struct nv04_pgraph_state *state, int uo, int xy, int idx, int32_t coord);
uint32_t nv04_pgraph_formats(struct nv04_pgraph_state *state);
void nv04_pgraph_volatile_reset(struct nv04_pgraph_state *state);

/* pgraph_d3d_nv3.c */
bool nv03_pgraph_d3d_cmp(int func, uint32_t a, uint32_t b);
bool nv03_pgraph_d3d_wren(int func, bool zeta_test, bool alpha_test);
uint16_t nv03_pgraph_zpoint_rop(struct nv03_pgraph_state *state, int32_t x, int32_t y, uint16_t pixel);

static inline void nv01_pgraph_vtx_cmp(struct nv01_pgraph_state *state, int xy, int idx) {
	int32_t val = (xy ? state->vtx_y : state->vtx_x)[idx];
	int stat = 0;
	if (val < 0)
		stat = 1;
	else if (val > 0)
		stat = 2;
	insrt(state->xy_misc_2[xy], 28, 2, stat);
}

static inline void nv03_pgraph_vtx_cmp(struct nv03_pgraph_state *state, int xy, int idx, bool weird) {
	int32_t val = (xy ? state->vtx_y : state->vtx_x)[idx];
	if (weird)
		val <<= 1;
	int stat = 0;
	if (val < 0)
		stat = 1;
	else if (val > 0)
		stat = 2;
	insrt(state->xy_misc_4[xy], 28, 2, stat);
}

static inline bool nv01_pgraph_is_tex_class(int cls) {
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

#ifdef __cplusplus
}
#endif

#endif
