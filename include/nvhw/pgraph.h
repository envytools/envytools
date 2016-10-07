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

enum {
	NV01_VTX_ZERO = 18,
	NV01_VTX_CANVAS = 19,
};

void nv01_pgraph_expand_color(uint32_t ctx, uint32_t config, uint32_t color, uint32_t *rgb, uint32_t *alpha);
void nv03_pgraph_expand_color(uint32_t ctx, uint32_t color, uint32_t *rgb, uint32_t *alpha);
uint32_t nv01_pgraph_expand_a1r10g10b10(uint32_t ctx, uint32_t config, uint32_t color);
uint32_t nv03_pgraph_expand_a1r10g10b10(uint32_t ctx, uint32_t color);
uint32_t nv01_pgraph_expand_mono(uint32_t ctx, uint32_t mono);
uint32_t nv03_pgraph_expand_mono(uint32_t ctx, uint32_t mono);
int nv01_pgraph_cpp(uint32_t pfb_config);
int nv01_pgraph_cpp_in(uint32_t ctx_switch);
uint32_t nv01_pgraph_pixel_addr(struct nv01_pgraph_state *state, int x, int y, int buf);
uint32_t nv01_pgraph_rop(struct nv01_pgraph_state *state, int x, int y, uint32_t pixel);

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
int nv03_pgraph_clip_status(struct nv03_pgraph_state *state, int32_t coord, int xy, bool canvas_only);
void nv03_pgraph_vtx_fixup(struct nv03_pgraph_state *state, int xy, int idx, int32_t coord);
void nv03_pgraph_iclip_fixup(struct nv03_pgraph_state *state, int xy, int32_t coord);
void nv03_pgraph_uclip_fixup(struct nv03_pgraph_state *state, int uo, int xy, int idx, int32_t coord);
void nv03_pgraph_set_clip(struct nv03_pgraph_state *state, int which, int idx, uint32_t val, bool prev_inited);
void nv03_pgraph_vtx_add(struct nv03_pgraph_state *state, int xy, int idx, uint32_t a, uint32_t b, uint32_t c, bool noclip);
void nv03_pgraph_prep_draw(struct nv03_pgraph_state *state, bool poly, bool noclip);

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
