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
	uint32_t debug_a;
	uint32_t debug_b;
	uint32_t debug_c;
	uint32_t debug_d;
	uint32_t debug_e;
	uint32_t debug_f;
	uint32_t debug_g;
	uint32_t debug_h;
	uint32_t debug_fd_check_skip;
	uint32_t debug_i;
	uint32_t debug_j;
	uint32_t debug_k;
	uint32_t debug_l;
	uint32_t intr;
	uint32_t intr_en;
	uint32_t invalid;
	uint32_t invalid_en;
	uint32_t dma_intr;
	uint32_t dma_intr_en;
	uint32_t nstatus;
	uint32_t nsource;
	bool was_switched;
	uint32_t ctx_switch_a;
	uint32_t ctx_switch_b;
	uint32_t ctx_switch_c;
	uint32_t ctx_switch_d;
	uint32_t ctx_switch_i;
	uint32_t ctx_switch_t;
	uint32_t ctx_cache_a[8];
	uint32_t ctx_cache_b[8];
	uint32_t ctx_cache_c[8];
	uint32_t ctx_cache_d[8];
	uint32_t ctx_cache_i[8];
	uint32_t ctx_cache_t[8];
	uint32_t ctx_control;
	uint32_t ctx_user;
	uint32_t state3d;
	uint32_t src2d_dma;
	uint32_t src2d_pitch;
	uint32_t src2d_offset;
	uint32_t unk6b0;
	uint32_t unk838;
	uint32_t unk83c;
	uint32_t surf_unk800;
	uint32_t surf_unk80c;
	uint32_t surf_unk810;
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
	uint32_t surf_unk610;
	uint32_t surf_unk614;
	uint32_t surf_base[7];
	uint32_t surf_offset[7];
	uint32_t surf_limit[7];
	uint32_t surf_pitch[6];
	uint32_t surf_swizzle[2];
	uint32_t surf_type;
	uint32_t surf_format;
	uint32_t surf_unk880;
	uint32_t surf_unk888;
	uint32_t surf_unk89c;
	uint32_t surf_unk8a4;
	uint32_t surf_unk8a8;
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
	uint32_t dma_unk38;
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
	uint32_t bundle_multisample;
	uint32_t bundle_blend;
	uint32_t bundle_blend_color;
	uint32_t bundle_tex_border_color[0x10];
	uint32_t bundle_tex_unk10[3];
	uint32_t bundle_tex_unk11[3];
	uint32_t bundle_tex_unk13[3];
	uint32_t bundle_tex_unk12[3];
	uint32_t bundle_tex_unk15[3];
	uint32_t bundle_tex_unk14[3];
	uint32_t bundle_clear_hv[2];
	uint32_t bundle_clear_color;
	uint32_t bundle_tex_color_key[0x10];
	uint32_t bundle_rc_factor_0[8];
	uint32_t bundle_rc_factor_1[8];
	uint32_t bundle_rc_in_alpha[8];
	uint32_t bundle_rc_out_alpha[8];
	uint32_t bundle_rc_in_color[8];
	uint32_t bundle_rc_out_color[8];
	uint32_t bundle_rc_config;
	uint32_t bundle_rc_final_a;
	uint32_t bundle_rc_final_b;
	uint32_t bundle_config_a;
	uint32_t bundle_stencil_a;
	uint32_t bundle_stencil_b;
	uint32_t bundle_stencil_c;
	uint32_t bundle_stencil_d;
	uint32_t bundle_config_b;
	uint32_t shadow_config_b;
	uint32_t bundle_viewport_offset;
	uint32_t bundle_ps_offset;
	uint32_t bundle_clipid_id;
	uint32_t bundle_surf_base_clipid;
	uint32_t bundle_surf_limit_clipid;
	uint32_t bundle_surf_offset_clipid;
	uint32_t bundle_surf_pitch_clipid;
	uint32_t bundle_line_stipple;
	uint32_t bundle_rt_enable;
	uint32_t bundle_fog_color;
	uint32_t bundle_fog_coeff[2];
	uint32_t bundle_point_size;
	uint32_t bundle_raster;
	uint32_t bundle_tex_shader_cull_mode;
	uint32_t bundle_tex_shader_misc;
	uint32_t bundle_tex_shader_op;
	uint32_t bundle_fence_offset;
	uint32_t bundle_tex_zcomp;
	uint32_t bundle_unk069;
	uint32_t bundle_unk06a;
	uint32_t bundle_rc_final_factor[2];
	uint32_t bundle_clip_hv[2];
	uint32_t bundle_tex_wrap[0x10];
	uint32_t bundle_tex_control_a[0x10];
	uint32_t bundle_tex_control_b[0x10];
	uint32_t bundle_tex_control_c[2];
	uint32_t bundle_tex_filter[0x10];
	uint32_t bundle_tex_format[0x10];
	uint32_t bundle_tex_rect[0x10];
	uint32_t bundle_tex_offset[0x10];
	uint32_t bundle_tex_palette[0x10];
	uint32_t bundle_tex_control_d[0x10];
	uint32_t bundle_clip_plane_enable;
	uint32_t bundle_viewport_hv[2];
	uint32_t bundle_scissor_hv[2];
	uint32_t bundle_clip_rect_horiz[8];
	uint32_t bundle_clip_rect_vert[8];
	uint32_t bundle_z_config;
	uint32_t bundle_clear_zeta;
	uint32_t bundle_depth_range_far;
	uint32_t bundle_depth_range_near;
	uint32_t bundle_dma_tex[2];
	uint32_t bundle_dma_vtx[2];
	uint32_t bundle_polygon_offset_units;
	uint32_t bundle_polygon_offset_factor;
	uint32_t bundle_tex_shader_const_eye[3];
	uint32_t bundle_unk0ae;
	uint32_t bundle_unk0af;
	uint32_t bundle_unk0b0;
	uint32_t bundle_surf_base_zcull;
	uint32_t bundle_surf_limit_zcull;
	uint32_t bundle_surf_offset_zcull;
	uint32_t bundle_surf_pitch_zcull;
	uint32_t bundle_unk0b4[4];
	uint32_t bundle_unk0b8;
	uint32_t bundle_primitive_restart_enable;
	uint32_t bundle_primitive_restart_index;
	uint32_t bundle_txc_cylwrap;
	uint32_t bundle_ps_control;
	uint32_t bundle_txc_enable;
	uint32_t bundle_unk0c6;
	uint32_t bundle_window_config;
	uint32_t bundle_depth_bounds_min;
	uint32_t bundle_depth_bounds_max;
	uint32_t bundle_xf_a;
	uint32_t bundle_xf_light;
	uint32_t bundle_xf_c;
	uint32_t bundle_unk0cd;
	uint32_t bundle_unk0ce;
	uint32_t bundle_unk0cf;
	uint32_t bundle_xf_txc[8];
	uint32_t bundle_unk0d8[2];
	uint32_t bundle_unk0da[3];
	uint32_t bundle_unk0dd;
	uint32_t bundle_unk0de[2];
	uint32_t bundle_unk0df;
	uint32_t bundle_unk0e0;
	uint32_t bundle_unk0e1[2];
	uint32_t bundle_xf_d;
	uint32_t bundle_alpha_func_ref;
	uint32_t bundle_unk0e5;
	uint32_t bundle_unk0e6;
	uint32_t bundle_unk0e7;
	uint32_t bundle_xf_load_pos;
	uint32_t bundle_unk0e9[5];
	uint32_t bundle_unk0f0[8];
	uint32_t bundle_color_mask;
	uint32_t celsius_mthd_unk3fc;
	uint32_t celsius_clip_rect_horiz[8];
	uint32_t celsius_clip_rect_vert[8];
	uint32_t celsius_xf_misc_a;
	uint32_t celsius_xf_misc_b;
	uint32_t celsius_config_c;
	uint32_t celsius_dma;
	uint32_t celsius_pipe_vtx[0x1c];
	uint32_t celsius_pipe_xfrm[0x3c][4];
	uint32_t celsius_pipe_light_v[0x30][3];
	uint32_t celsius_pipe_light_sa[3];
	uint32_t celsius_pipe_light_sb[19];
	uint32_t celsius_pipe_light_sc[12];
	uint32_t celsius_pipe_light_sd[12];
	uint32_t celsius_pipe_junk[4];
	uint32_t celsius_pipe_vtxbuf_offset[8];
	uint32_t celsius_pipe_vtxbuf_format[8];
	uint32_t celsius_pipe_begin_end;
	uint32_t celsius_pipe_edge_flag;
	uint32_t celsius_pipe_unk48;
	uint32_t celsius_pipe_vtx_state;
	uint32_t celsius_pipe_xvtx[3][0x10];
	uint32_t celsius_pipe_ovtx[0x10][0x10];
	uint32_t celsius_pipe_ovtx_pos;
	uint32_t celsius_pipe_prev_ovtx_pos;
	bool celsius_pipe_broke_ovtx;
	uint32_t fe3d_misc;
	uint32_t fe3d_state_curve;
	uint32_t fe3d_state_swatch;
	uint32_t fe3d_state_transition;
	uint32_t fe3d_emu_material_factor_rgb[3];
	uint32_t fe3d_emu_light_model_ambient[3];
	uint32_t fe3d_shadow_clip_rect_horiz;
	uint32_t fe3d_shadow_clip_rect_vert;
	uint32_t fe3d_dma_state;
	uint32_t fe3d_shadow_begin_patch_a;
	uint32_t fe3d_shadow_begin_patch_b;
	uint32_t fe3d_shadow_begin_patch_c;
	uint32_t fe3d_shadow_begin_patch_d;
	uint32_t fe3d_shadow_curve;
	uint32_t fe3d_shadow_begin_transition_a;
	uint32_t fe3d_shadow_begin_transition_b;
	uint32_t xf_mode_a;
	uint32_t xf_mode_b;
	uint32_t xf_mode_c;
	uint32_t xf_mode_t[4];
	uint32_t fe3d_xf_load_pos;
	// RDI starts here.
	uint32_t idx_cache[4][0x200];
	uint32_t idx_fifo[0x40][4];
	uint32_t idx_fifo_ptr;
	uint32_t idx_unk25[0x80];
	uint32_t idx_state_vtxbuf_offset[0x10];
	uint32_t idx_state_vtxbuf_format[0x10];
	uint32_t idx_state_a;
	uint32_t idx_state_b;
	uint32_t idx_state_c;
	uint32_t idx_state_d;
	uint8_t idx_unk27[0x100];
	uint32_t idx_unk27_ptr;
	uint32_t idx_prefifo[0x40][4];
	uint32_t idx_prefifo_ptr;
	uint32_t fd_state_begin_pt_a;
	uint32_t fd_state_begin_pt_b;
	uint32_t fd_state_begin_patch_c;
	uint32_t fd_state_swatch;
	uint32_t fd_state_unk10;
	uint32_t fd_state_unk14;
	uint32_t fd_state_unk18;
	uint32_t fd_state_unk1c;
	uint32_t fd_state_unk20;
	uint32_t fd_state_unk24;
	uint32_t fd_state_unk28;
	uint32_t fd_state_unk2c;
	uint32_t fd_state_unk30;
	uint32_t fd_state_unk34;
	uint32_t fd_state_begin_patch_d;
	uint32_t vab[0x11][4];
	uint32_t xfpr[0x118][4];
	uint32_t xfprunk1[2][4];
	uint32_t xfprunk2;
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
void pgraph_set_surf_format(struct pgraph_state *state, int which, uint32_t fmt);
uint32_t pgraph_surf_format(struct pgraph_state *state, int which);
bool pgraph_state3d_ok(struct pgraph_state *state);

/* pgraph_class.c */
enum {
	PGRAPH_3D_NONE,
	PGRAPH_3D_D3D0,
	PGRAPH_3D_D3D56,
	PGRAPH_3D_CELSIUS,
	PGRAPH_3D_KELVIN,
	PGRAPH_3D_RANKINE,
	PGRAPH_3D_CURIE,
};
int pgraph_3d_class(struct pgraph_state *state);
bool nv04_pgraph_is_3d_class(struct pgraph_state *state);
bool nv04_pgraph_is_celsius_class(struct pgraph_state *state);
bool nv04_pgraph_is_kelvin_class(struct pgraph_state *state);
bool nv04_pgraph_is_rankine_class(struct pgraph_state *state);
bool nv04_pgraph_is_sync_class(struct pgraph_state *state);
bool nv04_pgraph_is_syncable_class(struct pgraph_state *state);
bool nv04_pgraph_is_new_render_class(struct pgraph_state *state);
bool nv04_pgraph_is_sync(struct pgraph_state *state);

/* pgraph_grobj.c */
uint32_t pgraph_grobj_get_dma(struct pgraph_state *state, int which);
void pgraph_grobj_set_dma_pre(struct pgraph_state *state, uint32_t *grobj, int which, uint32_t val, bool clr);
void pgraph_grobj_set_dma_post(struct pgraph_state *state, int which, uint32_t val, bool clr);
uint32_t pgraph_grobj_get_operation(struct pgraph_state *state);
void pgraph_grobj_set_operation(struct pgraph_state *state, uint32_t *grobj, uint32_t val);
void pgraph_grobj_set_dither(struct pgraph_state *state, uint32_t *grobj, uint32_t val);
uint32_t pgraph_grobj_get_color_format(struct pgraph_state *state);
void pgraph_grobj_set_color_format(struct pgraph_state *state, uint32_t *grobj, uint32_t val);
uint32_t pgraph_grobj_get_bitmap_format(struct pgraph_state *state);
void pgraph_grobj_set_bitmap_format(struct pgraph_state *state, uint32_t *grobj, uint32_t val);
uint32_t pgraph_grobj_get_endian(struct pgraph_state *state);
uint32_t pgraph_grobj_get_sync(struct pgraph_state *state);
uint32_t pgraph_grobj_get_new(struct pgraph_state *state);
uint32_t pgraph_grobj_get_chroma(struct pgraph_state *state);
uint32_t pgraph_grobj_get_clip(struct pgraph_state *state);
uint32_t pgraph_grobj_get_swz(struct pgraph_state *state);

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
void nv04_pgraph_missing_hw(struct pgraph_state *state);
void pgraph_state_error(struct pgraph_state *state);
void nv04_pgraph_set_chroma_nv01(struct pgraph_state *state, uint32_t val);
void nv04_pgraph_set_pattern_mono_color_nv01(struct pgraph_state *state, int idx, uint32_t val);
void nv04_pgraph_set_bitmap_color_0_nv01(struct pgraph_state *state, uint32_t val);
void nv04_pgraph_set_clip(struct pgraph_state *state, int which, int idx, uint32_t val);
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

/* pgraph_celsius.c */
uint32_t pgraph_celsius_convert_light_v(uint32_t val);
uint32_t pgraph_celsius_convert_light_sx(uint32_t val);
void pgraph_celsius_pre_icmd(struct pgraph_state *state);
void pgraph_celsius_icmd(struct pgraph_state *state, int cmd, uint32_t val, bool last);
void pgraph_celsius_raw_icmd(struct pgraph_state *state, int cmd, uint32_t val, bool last);
uint32_t pgraph_celsius_fixup_vtxbuf_format(struct pgraph_state *state, int idx, uint32_t val);
void pgraph_celsius_xfrm(struct pgraph_state *state, int idx);
void pgraph_celsius_post_xfrm(struct pgraph_state *state, int idx);

/* pgraph_idx.c */
uint32_t pgraph_idx_ubyte_to_float(uint8_t val);
uint32_t pgraph_idx_short_to_float(struct pgraph_state *state, int16_t val);
uint32_t pgraph_idx_nshort_to_float(int16_t val);

enum {
	BUNDLE_MULTISAMPLE,
	BUNDLE_BLEND,
	BUNDLE_BLEND_COLOR,
	BUNDLE_TEX_BORDER_COLOR,
	BUNDLE_TEX_UNK10,
	BUNDLE_TEX_UNK11,
	BUNDLE_TEX_UNK12,
	BUNDLE_TEX_UNK13,
	BUNDLE_TEX_UNK14,
	BUNDLE_TEX_UNK15,
	BUNDLE_CLEAR_HV,
	BUNDLE_CLEAR_COLOR,
	BUNDLE_TEX_COLOR_KEY,
	BUNDLE_STENCIL_A,
	BUNDLE_STENCIL_B,
	BUNDLE_STENCIL_C,
	BUNDLE_STENCIL_D,
	BUNDLE_CONFIG_A,
	BUNDLE_CONFIG_B,
	BUNDLE_RASTER,
	BUNDLE_CLIP_HV,
	BUNDLE_VIEWPORT_HV,
	BUNDLE_SCISSOR_HV,
	BUNDLE_POINT_SIZE,
	BUNDLE_LINE_STIPPLE,
	BUNDLE_RT_ENABLE,
	BUNDLE_CLEAR_ZETA,
	BUNDLE_DMA_TEX,
	BUNDLE_DMA_VTX,
	BUNDLE_POLYGON_STIPPLE,
	BUNDLE_TEX_SHADER_CULL_MODE,
	BUNDLE_TEX_SHADER_MISC,
	BUNDLE_TEX_SHADER_OP,
	BUNDLE_TEX_SHADER_CONST_EYE,
	BUNDLE_TEX_OFFSET,
	BUNDLE_TEX_FORMAT,
	BUNDLE_TEX_WRAP,
	BUNDLE_TEX_CONTROL_A,
	BUNDLE_TEX_CONTROL_B,
	BUNDLE_TEX_FILTER,
	BUNDLE_TEX_RECT,
	BUNDLE_TEX_PALETTE,
	BUNDLE_TEX_CONTROL_C,
	BUNDLE_TEX_CONTROL_D,
	BUNDLE_TEX_ZCOMP,
	BUNDLE_TXC_CYLWRAP,
	BUNDLE_TXC_ENABLE,
	BUNDLE_UNK1E68,
	BUNDLE_RC_FINAL_FACTOR,
	BUNDLE_RC_IN_ALPHA,
	BUNDLE_RC_IN_COLOR,
	BUNDLE_RC_OUT_ALPHA,
	BUNDLE_RC_OUT_COLOR,
	BUNDLE_RC_FACTOR_A,
	BUNDLE_RC_FACTOR_B,
	BUNDLE_RC_CONFIG,
	BUNDLE_RC_FINAL_A,
	BUNDLE_RC_FINAL_B,
	BUNDLE_FOG_COLOR,
	BUNDLE_FOG_COEFF,
	BUNDLE_DEPTH_RANGE_FAR,
	BUNDLE_DEPTH_RANGE_NEAR,
	BUNDLE_POLYGON_OFFSET_UNITS,
	BUNDLE_POLYGON_OFFSET_FACTOR,
	BUNDLE_CLIP_RECT_HORIZ,
	BUNDLE_CLIP_RECT_VERT,
	BUNDLE_Z_CONFIG,
	BUNDLE_FENCE_OFFSET,
	BUNDLE_CLIPID_ID,
	BUNDLE_SURF_BASE_CLIPID,
	BUNDLE_SURF_LIMIT_CLIPID,
	BUNDLE_SURF_OFFSET_CLIPID,
	BUNDLE_SURF_PITCH_CLIPID,
	BUNDLE_SURF_BASE_ZCULL,
	BUNDLE_SURF_LIMIT_ZCULL,
	BUNDLE_SURF_OFFSET_ZCULL,
	BUNDLE_SURF_PITCH_ZCULL,
	BUNDLE_UNK0AF,
	BUNDLE_UNK0B4,
	BUNDLE_UNK0B8,
	BUNDLE_PS_CONTROL,
	BUNDLE_PS_OFFSET,
	BUNDLE_PS_PREFETCH,
	BUNDLE_VIEWPORT_OFFSET,
	BUNDLE_CLIP_PLANE_ENABLE,
	BUNDLE_PRIMITIVE_RESTART_ENABLE,
	BUNDLE_PRIMITIVE_RESTART_INDEX,
	BUNDLE_WINDOW_CONFIG,
	BUNDLE_DEPTH_BOUNDS_MIN,
	BUNDLE_DEPTH_BOUNDS_MAX,
	BUNDLE_XF_A,
	BUNDLE_XF_LIGHT,
	BUNDLE_XF_C,
	BUNDLE_XF_TXC,
	BUNDLE_XF_D,
	BUNDLE_ALPHA_FUNC_REF,
	BUNDLE_XF_LOAD_POS,
	BUNDLE_COLOR_MASK,
	BUNDLE_ZPASS_COUNTER_RESET,
	BUNDLE_UNK1F7,
};

bool pgraph_in_begin_end(struct pgraph_state *state);
void pgraph_bundle(struct pgraph_state *state, int bundle, int idx, uint32_t val, bool last);
void pgraph_kelvin_bundle(struct pgraph_state *state, int cmd, uint32_t val, bool last);
void pgraph_flush_xf_mode(struct pgraph_state *state);
void pgraph_ld_xfctx2(struct pgraph_state *state, uint32_t which, int comp, uint32_t a, uint32_t b);
void pgraph_ld_xfctx(struct pgraph_state *state, uint32_t which, int comp, uint32_t a);
void pgraph_ld_ltctx2(struct pgraph_state *state, uint32_t which, int comp, uint32_t a, uint32_t b);
void pgraph_ld_ltctx(struct pgraph_state *state, uint32_t which, int comp, uint32_t a);
void pgraph_ld_ltc(struct pgraph_state *state, int space, uint32_t which, uint32_t a);
void pgraph_ld_xfpr(struct pgraph_state *state, uint32_t which, int comp, uint32_t a);
void pgraph_ld_xfunk4(struct pgraph_state *state, uint32_t addr, uint32_t a);
void pgraph_ld_xfunk8(struct pgraph_state *state, uint32_t addr, uint32_t a);
void pgraph_xf_nop(struct pgraph_state *state, uint32_t val);
void pgraph_xf_sync(struct pgraph_state *state, uint32_t val);
void pgraph_ld_vtx(struct pgraph_state *state, int fmt, int which, int num, int comp, uint32_t a);
void pgraph_set_edge_flag(struct pgraph_state *state, bool val);
void pgraph_idx_unkf8(struct pgraph_state *state, uint32_t val);
void pgraph_idx_unkfc(struct pgraph_state *state, uint32_t val);
int pgraph_vtx_attr_xlat_celsius(struct pgraph_state *state, int idx);
int pgraph_vtx_attr_xlat_kelvin(struct pgraph_state *state, int idx);
void pgraph_set_vtxbuf_offset(struct pgraph_state *state, int which, uint32_t val);
void pgraph_set_vtxbuf_format(struct pgraph_state *state, int which, uint32_t fmt);
void pgraph_set_idxbuf_offset(struct pgraph_state *state, uint32_t val);
void pgraph_set_idxbuf_format(struct pgraph_state *state, uint32_t fmt);
void pgraph_fd_cmd(struct pgraph_state *state, uint32_t cmd, uint32_t arg);

static inline bool nv04_pgraph_is_nv11p(const struct chipset_info *chipset) {
	return chipset->chipset > 0x10 && chipset->chipset != 0x15;
}

static inline bool nv04_pgraph_is_nv15p(const struct chipset_info *chipset) {
	return chipset->chipset > 0x10;
}

static inline bool nv04_pgraph_is_nv17p(const struct chipset_info *chipset) {
	return chipset->chipset >= 0x17 && chipset->chipset != 0x1a && chipset->card_type < 0x20;
}

static inline bool nv04_pgraph_is_nv25p(const struct chipset_info *chipset) {
	return chipset->chipset >= 0x25 && chipset->chipset != 0x2a;
}

static inline bool nv04_pgraph_is_nv44p(const struct chipset_info *chipset) {
	if (chipset->chipset < 0x44)
		return false;
	if (chipset->chipset == 0x45)
		return false;
	if (chipset->chipset == 0x47)
		return false;
	if (chipset->chipset == 0x49)
		return false;
	if (chipset->chipset == 0x4b)
		return false;
	return true;
}

static inline uint32_t pgraph_class(struct pgraph_state *state) {
	if (state->chipset.card_type < 3) {
		return extr(state->access, 12, 5);
	} else if (state->chipset.card_type < 4) {
		return extr(state->ctx_user, 16, 5);
	} else if (!nv04_pgraph_is_nv25p(&state->chipset)) {
		return extr(state->ctx_switch_a, 0, 8);
	} else if (state->chipset.card_type < 0x40) {
		return extr(state->ctx_switch_a, 0, 12);
	} else {
		return extr(state->ctx_switch_a, 0, 16);
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

static inline uint32_t pgraph_offset_mask(const struct chipset_info *chipset) {
	if (chipset->chipset < 4)
		return chipset->is_nv03t ? 0x007ffff0 : 0x003ffff0;
	if (chipset->chipset < 5)
		return 0x00fffff0;
	else if (chipset->card_type < 0x10)
		return 0x01fffff0;
	else if (chipset->card_type < 0x20)
		return 0x07fffff0;
	else if (chipset->chipset == 0x34)
		return 0x3ffffff0;
	else
		return 0x3fffffc0;
}

static inline uint32_t pgraph_pitch_mask(const struct chipset_info *chipset) {
	if (chipset->card_type < 0x10)
		return 0x1ff0;
	else if (chipset->card_type < 0x20 || chipset->chipset == 0x34)
		return 0xfff0;
	else if (chipset->card_type < 0x40)
		return 0xffc0;
	else
		return 0x1ffc0;
}

static inline bool pgraph_is_class_line(struct pgraph_state *state) {
	uint32_t cls = pgraph_class(state);
	if (state->chipset.card_type < 4) {
		return cls == 0x9 || cls == 0xa;
	} else {
		return cls == 0x1c || cls == 0x5c || (cls == 0x35c && state->chipset.card_type == 0x30);
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
		if (state->chipset.card_type == 0x30 && cls == 0x366)
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
