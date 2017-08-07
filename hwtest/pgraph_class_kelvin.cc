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
#include "pgraph_mthd.h"
#include "pgraph_class.h"
#include "nva.h"

namespace hwtest {
namespace pgraph {

enum {
	KELVIN_TEX_FMT_KIND = 7,
	KELVIN_TEX_FMT_INVALID = 0,
	KELVIN_TEX_FMT_SWIZZLE = 1,
	KELVIN_TEX_FMT_RECT = 2,
	KELVIN_TEX_FMT_DXT = 3,
	KELVIN_TEX_FMT_UNK24 = 4,
	KELVIN_TEX_FMT_ZCOMP = 8,
};

static uint32_t kelvin_tex_formats[0x80] = {
	KELVIN_TEX_FMT_SWIZZLE,				// 00
	KELVIN_TEX_FMT_SWIZZLE,				// 01
	KELVIN_TEX_FMT_SWIZZLE,				// 02
	KELVIN_TEX_FMT_SWIZZLE,				// 03
	KELVIN_TEX_FMT_SWIZZLE,				// 04
	KELVIN_TEX_FMT_SWIZZLE,				// 05
	KELVIN_TEX_FMT_SWIZZLE,				// 06
	KELVIN_TEX_FMT_SWIZZLE,				// 07
	KELVIN_TEX_FMT_SWIZZLE,				// 08
	KELVIN_TEX_FMT_SWIZZLE,				// 09
	KELVIN_TEX_FMT_SWIZZLE,				// 0a
	KELVIN_TEX_FMT_SWIZZLE,				// 0b
	KELVIN_TEX_FMT_DXT,				// 0c
	KELVIN_TEX_FMT_INVALID,				// 0d
	KELVIN_TEX_FMT_DXT,				// 0e
	KELVIN_TEX_FMT_DXT,				// 0f
	KELVIN_TEX_FMT_RECT,				// 10
	KELVIN_TEX_FMT_RECT,				// 11
	KELVIN_TEX_FMT_RECT,				// 12
	KELVIN_TEX_FMT_RECT,				// 13
	KELVIN_TEX_FMT_RECT,				// 14
	KELVIN_TEX_FMT_RECT,				// 15
	KELVIN_TEX_FMT_RECT,				// 16
	KELVIN_TEX_FMT_RECT,				// 17
	KELVIN_TEX_FMT_RECT,				// 18
	// NV20+
	KELVIN_TEX_FMT_SWIZZLE,				// 19
	KELVIN_TEX_FMT_SWIZZLE,				// 1a
	KELVIN_TEX_FMT_RECT,				// 1b
	KELVIN_TEX_FMT_RECT,				// 1c
	KELVIN_TEX_FMT_RECT,				// 1d
	KELVIN_TEX_FMT_RECT,				// 1e
	KELVIN_TEX_FMT_RECT,				// 1f
	KELVIN_TEX_FMT_RECT,				// 20
	KELVIN_TEX_FMT_INVALID,				// 21
	KELVIN_TEX_FMT_INVALID,				// 22
	KELVIN_TEX_FMT_INVALID,				// 23
	KELVIN_TEX_FMT_UNK24,				// 24
	KELVIN_TEX_FMT_UNK24,				// 25
	KELVIN_TEX_FMT_RECT,				// 26
	KELVIN_TEX_FMT_SWIZZLE,				// 27
	KELVIN_TEX_FMT_SWIZZLE,				// 28
	KELVIN_TEX_FMT_SWIZZLE,				// 29
	KELVIN_TEX_FMT_SWIZZLE | KELVIN_TEX_FMT_ZCOMP,	// 2a
	KELVIN_TEX_FMT_SWIZZLE | KELVIN_TEX_FMT_ZCOMP,	// 2b
	KELVIN_TEX_FMT_SWIZZLE | KELVIN_TEX_FMT_ZCOMP,	// 2c
	KELVIN_TEX_FMT_SWIZZLE | KELVIN_TEX_FMT_ZCOMP,	// 2d
	KELVIN_TEX_FMT_RECT | KELVIN_TEX_FMT_ZCOMP,	// 2e
	KELVIN_TEX_FMT_RECT | KELVIN_TEX_FMT_ZCOMP,	// 2f
	KELVIN_TEX_FMT_RECT | KELVIN_TEX_FMT_ZCOMP,	// 30
	KELVIN_TEX_FMT_RECT | KELVIN_TEX_FMT_ZCOMP,	// 31
	KELVIN_TEX_FMT_SWIZZLE,				// 32
	KELVIN_TEX_FMT_SWIZZLE,				// 33
	KELVIN_TEX_FMT_UNK24,				// 34
	KELVIN_TEX_FMT_RECT,				// 35
	KELVIN_TEX_FMT_RECT,				// 36
	KELVIN_TEX_FMT_RECT,				// 37
	KELVIN_TEX_FMT_SWIZZLE,				// 38
	KELVIN_TEX_FMT_SWIZZLE,				// 39
	KELVIN_TEX_FMT_SWIZZLE,				// 3a
	KELVIN_TEX_FMT_SWIZZLE,				// 3b
	KELVIN_TEX_FMT_SWIZZLE,				// 3c
	KELVIN_TEX_FMT_RECT,				// 3d
	KELVIN_TEX_FMT_RECT,				// 3e
	KELVIN_TEX_FMT_RECT,				// 3f
	KELVIN_TEX_FMT_RECT,				// 40
	KELVIN_TEX_FMT_RECT,				// 41
	// NV25+
	KELVIN_TEX_FMT_SWIZZLE,				// 42
	KELVIN_TEX_FMT_RECT,				// 43
	KELVIN_TEX_FMT_SWIZZLE,				// 44
	KELVIN_TEX_FMT_SWIZZLE,				// 45
	KELVIN_TEX_FMT_RECT,				// 46
	KELVIN_TEX_FMT_RECT,				// 47
	KELVIN_TEX_FMT_RECT,				// 48
	KELVIN_TEX_FMT_SWIZZLE,				// 49
	// NV30+
	KELVIN_TEX_FMT_RECT,				// 4a
	KELVIN_TEX_FMT_RECT,				// 4b
	KELVIN_TEX_FMT_RECT,				// 4c
	KELVIN_TEX_FMT_RECT,				// 4d
	KELVIN_TEX_FMT_SWIZZLE,				// 4e
};

static uint32_t kelvin_tex_format(struct pgraph_state *state, int fmt) {
	if (!nv04_pgraph_is_nv25p(&state->chipset) && fmt >= 0x42)
		return KELVIN_TEX_FMT_INVALID;
	if (state->chipset.card_type < 0x30 && fmt >= 0x4a)
		return KELVIN_TEX_FMT_INVALID;
	return kelvin_tex_formats[fmt];
}

static void adjust_orig_idx(struct pgraph_state *state) {
	if (extr(state->idx_state_a, 20, 4) == 0xf)
		insrt(state->idx_state_a, 20, 4, 0);
	insrt(state->idx_state_b, 16, 5, 0);
	insrt(state->idx_state_b, 24, 5, 0);
	// XXX
	state->debug_d &= 0xefffffff;
}

static void adjust_orig_xf(struct pgraph_state *state) {
	insrt(state->idx_state_a, 20, 4, 0);
	// XXX
	state->fd_state_unk18 = 0;
	state->fd_state_unk20 = 0;
	state->fd_state_unk30 = 0;
	adjust_orig_idx(state);
}

static void adjust_orig_bundle(struct pgraph_state *state) {
	state->surf_unk800 = 0;
	state->debug_g &= 0xffcfffff;
	adjust_orig_xf(state);
}

static void pgraph_kelvin_check_err19(struct pgraph_state *state) {
	if (state->chipset.card_type == 0x20 && extr(state->fe3d_misc, 4, 1) && extr(state->debug_d, 3, 1) && nv04_pgraph_is_kelvin_class(state))
		nv04_pgraph_blowup(state, 0x80000);
}

static void pgraph_kelvin_check_err18(struct pgraph_state *state) {
	if (pgraph_in_begin_end(state) && (pgraph_class(state) & 0xff) == 0x97)
		nv04_pgraph_blowup(state, 0x40000);
}

static void adjust_orig_launch(struct pgraph_state *state, std::mt19937 &rnd) {
	if (rnd() & 3) {
		insrt(state->ctx_valid, 2, 2, 3);
		insrt(state->ctx_valid, 10, 2, 3);
	}
	if (rnd() & 3) {
		insrt(state->surf_type, 0, 2, 1);
	}
	if (!(rnd() & 0xf)) {
		insrt(state->bundle_dma_tex[0], 0, 16, 0);
	}
	if (!(rnd() & 0xf)) {
		insrt(state->bundle_dma_tex[1], 0, 16, 0);
	}
	for (int i = 0; i < 4; i++) {
		if (!(rnd() & 0xf)) {
			int fmt = "\x0c\x0e\x0f"[rnd() % 3];
			insrt(state->bundle_tex_format[i], 8, 7, fmt);
		}
		if (rnd() & 7) {
			insrt(state->bundle_tex_control[i], 3, 1, 0);
		}
		if (rnd() & 7) {
			insrt(state->bundle_tex_control[i], 4, 2, 0);
		}
		if (rnd() & 7)
			insrt(state->bundle_tex_control[i], 30, 1, 1);
	}
	for (int i = 0; i < 8; i++) {
		if (rnd() & 7)
			insrt(state->bundle_rc_out_color[i], 18, 2, 0);
	}
	if (rnd() & 1) {
		if (rnd() & 7)
			insrt(state->bundle_stencil_a, 0, 1, 0);
		if (rnd() & 7)
			insrt(state->bundle_blend, 3, 1, 0);
		if (rnd() & 7)
			insrt(state->bundle_config_a, 14, 1, 0);
	}
}

static void state_check_launch(struct pgraph_state *state) {
	bool state_fail = false;
	// Surfaces.
	int cfmt = extr(state->surf_format, 8, 4);
	if (cfmt == 0) {
		state_fail = true;
	} else if (cfmt == 0xd) {
		if (extr(state->bundle_blend, 3, 1)) {
			int blend_eq = extr(state->bundle_blend, 0, 3);
			if (blend_eq != 5 && blend_eq != 6)
				state_fail = true;
		}
	} else if (cfmt == 1 || cfmt == 6) {
		if (pgraph_grobj_get_swz(state))
			state_fail = true;
		if (extr(state->surf_type, 0, 2) == 2)
			state_fail = true;
		if (extr(state->bundle_stencil_a, 0, 1))
			state_fail = true;
		if (extr(state->bundle_config_a, 14, 1))
			state_fail = true;
		if (cfmt == 1 && extr(state->bundle_blend, 3, 1)) {
			int blend_eq = extr(state->bundle_blend, 0, 3);
			if (blend_eq != 5 && blend_eq != 6)
				state_fail = true;
		}
	} else if (cfmt == 2 || cfmt == 3 || cfmt == 5 || cfmt == 9 || cfmt == 0xa) {
		int blend_eq = extr(state->bundle_blend, 0, 3);
		if (extr(state->bundle_blend, 3, 1) && (blend_eq == 5 || blend_eq == 6)) {
			state_fail = true;
		}
	} else {
		// OK
	}
	bool color_valid = extr(state->ctx_valid, 2, 1) && extr(state->ctx_valid, 10, 1);
	bool zeta_valid = extr(state->ctx_valid, 3, 1) && extr(state->ctx_valid, 11, 1);
	if (!color_valid && !zeta_valid)
		state_fail = true;
	if (!extr(state->surf_type, 0, 2))
		state_fail = true;
	// Textures.
	for (int i = 0; i < 4; i++) {
		if (extr(state->bundle_tex_control[i], 30, 1)) {
			int dma = extr(state->bundle_tex_format[i], 1, 1);
			if (extr(state->bundle_dma_tex[dma], 0, 16) == 0)
				state_fail = true;
			if (extr(state->bundle_tex_control[i], 4, 2)) {
				if (extr(state->bundle_tex_format[i], 6, 2) == 3)
					state_fail = true;
				int mag = extr(state->bundle_tex_filter[i], 24, 4);
				if (mag == 4)
					state_fail = true;
				int min = extr(state->bundle_tex_filter[i], 16, 6);
				if (min == 7)
					state_fail = true;
			}
			int fmt = extr(state->bundle_tex_format[i], 8, 7);
			int wrap_s = extr(state->bundle_tex_wrap[i], 0, 3);
			if (extr(state->bundle_tex_control[i], 3, 1)) {
				if (!extr(state->bundle_tex_format[i], 3, 1))
					state_fail = true;
				if (extr(state->bundle_tex_rect[i], 0, 1))
					state_fail = true;
			}
			int xlat = kelvin_tex_format(state, fmt);
			int kind = xlat & KELVIN_TEX_FMT_KIND;
			if (kind == KELVIN_TEX_FMT_RECT || kind == KELVIN_TEX_FMT_UNK24) {
				if (wrap_s == 1 || wrap_s == 2)
					state_fail = true;
			} else if (kind == KELVIN_TEX_FMT_DXT) {
				if (extr(state->bundle_tex_control[i], 3, 1))
					state_fail = true;
				if (extr(state->bundle_tex_control[i], 0, 2))
					state_fail = true;
			} else {
				if (extr(state->bundle_tex_control[i], 3, 1))
					state_fail = true;
			}
		}
	}
	// Shaders.
	for (int i = 0; i < 4; i++) {
		int op = extr(state->bundle_tex_shader_op, i * 5, 5);
		if (i == 0)
			op &= 7;
		if (!extr(state->bundle_tex_control[i], 30, 1)) {
			if (op != 0 && op != 4 && op != 5 && (op != 0x11 || i == 3))
				state_fail = true;
		}
		if (i == 2) {
			if (op == 6 || op == 7 || op == 9 || op == 0xa || op == 0xb || op == 0xf || op == 0x10 || op == 0x11) {
				int pidx = extr(state->bundle_tex_shader_misc, 16, 1);
				int prev = extr(state->bundle_tex_shader_op, pidx * 5, 5);
				if (pidx == 0)
					prev &= 7;
				if (prev == 0 || prev == 5 || prev == 0x11)
					state_fail = true;
			}
		}
		if (i == 3) {
			int pidx = extr(state->bundle_tex_shader_misc, 20, 2);
			int prev = extr(state->bundle_tex_shader_op, pidx * 5, 5);
			if (pidx == 0)
				prev &= 7;
			if (op == 6 || op == 7 || op == 9 || op == 0xa || op == 0xc || op == 0xd || op == 0xe || op == 0xf || op == 0x10 || op == 0x12) {
				if (pidx == 2 && (prev == 0xa || prev == 0xb))
					state_fail = true;
				if (prev == 0 || prev == 5 || prev == 0x11)
					state_fail = true;
			}
		}
	}
	// Combiners.
	for (int i = 0; i < 8; i++) {
		if (extr(state->bundle_rc_out_color[i], 18, 1) &&
			extr(state->bundle_rc_out_color[i], 0, 4) != 0 &&
			extr(state->bundle_rc_out_color[i], 0, 4) ==
				extr(state->bundle_rc_out_alpha[i], 0, 4))
			state_fail = true;
		if (extr(state->bundle_rc_out_color[i], 19, 1) &&
			extr(state->bundle_rc_out_color[i], 4, 4) != 0 &&
			extr(state->bundle_rc_out_color[i], 4, 4) ==
				extr(state->bundle_rc_out_alpha[i], 4, 4))
			state_fail = true;
	}
	// And we're done.
	if (state_fail)
		pgraph_state_error(state);
}

static void pgraph_emu_celsius_calc_material(struct pgraph_state *state, uint32_t light_model_ambient[3], uint32_t material_factor_rgb[3]) {
	if (extr(state->fe3d_misc, 21, 1)) {
		material_factor_rgb[0] = 0x3f800000;
		material_factor_rgb[1] = 0x3f800000;
		material_factor_rgb[2] = 0x3f800000;
		light_model_ambient[0] = state->fe3d_emu_light_model_ambient[0];
		light_model_ambient[1] = state->fe3d_emu_light_model_ambient[1];
		light_model_ambient[2] = state->fe3d_emu_light_model_ambient[2];
	} else if (extr(state->fe3d_misc, 22, 1)) {
		material_factor_rgb[0] = state->fe3d_emu_light_model_ambient[0];
		material_factor_rgb[1] = state->fe3d_emu_light_model_ambient[1];
		material_factor_rgb[2] = state->fe3d_emu_light_model_ambient[2];
		light_model_ambient[0] = state->fe3d_emu_material_factor_rgb[0];
		light_model_ambient[1] = state->fe3d_emu_material_factor_rgb[1];
		light_model_ambient[2] = state->fe3d_emu_material_factor_rgb[2];
	} else {
		material_factor_rgb[0] = state->fe3d_emu_material_factor_rgb[0];
		material_factor_rgb[1] = state->fe3d_emu_material_factor_rgb[1];
		material_factor_rgb[2] = state->fe3d_emu_material_factor_rgb[2];
		light_model_ambient[0] = state->fe3d_emu_light_model_ambient[0];
		light_model_ambient[1] = state->fe3d_emu_light_model_ambient[1];
		light_model_ambient[2] = state->fe3d_emu_light_model_ambient[2];
	}
}

class MthdKelvinDmaTex : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t rval;
		if (chipset.card_type < 0x40)
			rval = val & 0xffff;
		else
			rval = val & 0xffffff;
		int dcls = extr(pobj[0], 0, 12);
		if (dcls == 0x30)
			rval = 0;
		bool bad = false;
		if (dcls != 0x30 && dcls != 0x3d && dcls != 2)
			bad = true;
		if (bad && extr(exp.debug_d, 23, 1))
			nv04_pgraph_blowup(&exp, 2);
		bool prot_err = false;
		if (dcls != 0x30) {
			if (extr(pobj[1], 0, 8) != 0xff)
				prot_err = true;
			if (extr(pobj[0], 20, 8))
				prot_err = true;
		}
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		if (!exp.nsource) {
			if (chipset.card_type >= 0x30) {
				insrt(exp.fe3d_misc, 28 + which, 1, 1);
				insrt(exp.fe3d_misc, 30 + which, 1, 0);
			}
			exp.bundle_dma_tex[which] = rval | extr(pobj[0], 16, 2) << 24;
			pgraph_bundle(&exp, BUNDLE_DMA_TEX, which, exp.bundle_dma_tex[which], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
public:
	MthdKelvinDmaTex(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinDmaVtx : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t rval;
		if (chipset.card_type < 0x40)
			rval = val & 0xffff;
		else
			rval = val & 0xffffff;
		int dcls = extr(pobj[0], 0, 12);
		if (dcls == 0x30)
			rval = 0;
		bool bad = false;
		if (dcls != 0x30 && dcls != 0x3d && dcls != 2)
			bad = true;
		if (bad && extr(exp.debug_d, 23, 1))
			nv04_pgraph_blowup(&exp, 2);
		bool prot_err = false;
		if (dcls != 0x30) {
			if (extr(pobj[1], 0, 8) != 0xff)
				prot_err = true;
			if (extr(pobj[0], 20, 8))
				prot_err = true;
		}
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		if (!extr(pobj[0], 12, 2) && dcls != 0x30) {
			exp.intr |= 0x400;
			exp.fifo_enable = 0;
		}
		if (!exp.intr) {
			exp.bundle_dma_vtx[which] = rval;
			pgraph_bundle(&exp, BUNDLE_DMA_VTX, which, rval, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
public:
	MthdKelvinDmaVtx(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinDmaState : public SingleMthdTest {
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t rval = val & 0xffff;
		int dcls = extr(pobj[0], 0, 12);
		if (dcls == 0x30)
			rval = 0;
		bool bad = false;
		if (dcls != 0x30 && dcls != 0x3d && dcls != 3)
			bad = true;
		if (bad && extr(exp.debug_d, 23, 1))
			nv04_pgraph_blowup(&exp, 2);
		if (chipset.card_type < 0x40)
			exp.fe3d_dma_state = rval;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClip : public SingleMthdTest {
	int which, hv;
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_celsius_class(&exp)) {
			return !(val & 0x8000);
		} else {
			return !(val & 0xf000f000);
		}
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			uint32_t rval = val;
			if (chipset.card_type >= 0x30)
				rval &= 0x1fff0fff;
			if (which == 0 || which == 3) {
				exp.bundle_clip_hv[hv] = rval;
				pgraph_bundle(&exp, BUNDLE_CLIP_HV, hv, rval, true);
			}
			if (chipset.card_type >= 0x30) {
				if (which == 1 || which == 3) {
					exp.bundle_viewport_hv[hv] = rval;
					pgraph_bundle(&exp, BUNDLE_VIEWPORT_HV, hv, rval, true);
				}
				if (which == 2 || which == 3) {
					exp.bundle_scissor_hv[hv] = rval;
					pgraph_bundle(&exp, BUNDLE_SCISSOR_HV, hv, rval, true);
				}
			}
		}
	}
public:
	MthdKelvinClip(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which, int hv)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which), hv(hv) {}
};

class MthdEmuCelsiusTexFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 0, 2, 1);
			if (rnd() & 3)
				insrt(val, 3, 2, 1);
			if (rnd() & 3)
				insrt(val, 5, 2, 1);
			if (!(rnd() & 3))
				insrt(val, 7, 5, 0x19 + rnd() % 4);
			if (rnd() & 1)
				insrt(val, 20, 4, extr(val, 16, 4));
			if (rnd() & 3)
				insrt(val, 24, 3, 3);
			if (rnd() & 3)
				insrt(val, 28, 3, 3);
			if (rnd() & 1) {
				if (rnd() & 3)
					insrt(val, 2, 1, 0);
				if (rnd() & 3)
					insrt(val, 12, 4, 1);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int mips = extr(val, 12, 4);
		int su = extr(val, 16, 4);
		int sv = extr(val, 20, 4);
		int fmt = extr(val, 7, 5);
		if (!extr(val, 0, 2) || extr(val, 0, 2) == 3)
			return false;
		if (extr(val, 2, 1)) {
			if (su != sv)
				return false;
			if (su >= 0xa && (fmt == 6 || fmt == 7 || fmt == 0xb || fmt == 0xe || fmt == 0xf))
				return false;
			if (su >= 0xb)
				return false;
		}
		if (!extr(val, 3, 2) || extr(val, 3, 2) == 3)
			return false;
		if (!extr(val, 5, 2) || extr(val, 5, 2) == 3)
			return false;
		if (fmt == 0xd)
			return false;
		if (fmt >= 0x1d)
			return false;
		if (mips > 0xc || mips == 0)
			return false;
		if (fmt >= 0x19) {
			if (cls != 0x99)
				return false;
		}
		if (fmt >= 0x10) {
			if (extr(val, 2, 1))
				return false;
			if (extr(val, 24, 3) != 3)
				return false;
			if (extr(val, 28, 3) != 3)
				return false;
			if (mips != 1)
				return false;
		}
		if (su > 0xb || sv > 0xb)
			return false;
		if (extr(val, 24, 3) < 1 || extr(val, 24, 3) > 3)
			return false;
		if (extr(val, 28, 3) < 1 || extr(val, 28, 3) > 3)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			int op = extr(val, 2, 1) ? 3 : 1;
			if (idx == 0) {
				if (!nv04_pgraph_is_nv25p(&chipset))
					insrt(exp.bundle_tex_shader_op, 0, 3, op);
				else
					insrt(exp.bundle_tex_shader_op, 0, 5, op);
			} else {
				insrt(exp.bundle_tex_shader_op, 5, 5, op);
			}
			pgraph_bundle(&exp, BUNDLE_TEX_SHADER_OP, 0, exp.bundle_tex_shader_op, true);
			insrt(exp.bundle_tex_wrap[idx], 0, 3, extr(val, 24, 3));
			insrt(exp.bundle_tex_wrap[idx], 4, 1, extr(val, 27, 1));
			insrt(exp.bundle_tex_wrap[idx], 8, 3, extr(val, 28, 3));
			insrt(exp.bundle_tex_wrap[idx], 12, 1, extr(val, 31, 1));
			pgraph_bundle(&exp, BUNDLE_TEX_WRAP, idx, exp.bundle_tex_wrap[idx], true);
			int mips = extr(val, 12, 4);
			int su = extr(val, 16, 4);
			int sv = extr(val, 20, 4);
			if (mips > su && mips > sv)
				mips = std::max(su, sv) + 1;
			insrt(exp.bundle_tex_format[idx], 1, 1, extr(val, 1, 1));
			insrt(exp.bundle_tex_format[idx], 2, 1, extr(val, 2, 1));
			insrt(exp.bundle_tex_format[idx], 4, 1, extr(val, 4, 1));
			insrt(exp.bundle_tex_format[idx], 5, 1, extr(val, 6, 1));
			insrt(exp.bundle_tex_format[idx], 8, 5, extr(val, 7, 5));
			insrt(exp.bundle_tex_format[idx], 13, 2, 0);
			insrt(exp.bundle_tex_format[idx], 16, 4, mips);
			insrt(exp.bundle_tex_format[idx], 20, 4, su);
			insrt(exp.bundle_tex_format[idx], 24, 4, sv);
			pgraph_bundle(&exp, BUNDLE_TEX_FORMAT, idx, exp.bundle_tex_format[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexUnk238 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			exp.bundle_tex_unk238[idx] = val;
			pgraph_bundle(&exp, BUNDLE_TEX_UNK238, idx, exp.bundle_tex_unk238[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		val &= ~0x7f;
		if (rnd() & 1) {
			val ^= 1 << (rnd() & 0x1f);
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_rankine_class(&exp)) {
			return true;
		} else {
			return !(val & 0x7f);
		}
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_offset[idx] = val;
			pgraph_bundle(&exp, BUNDLE_TEX_OFFSET, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 0, 2, 1);
			if (rnd() & 3)
				insrt(val, 6, 2, 0);
			if (rnd() & 3)
				insrt(val, 15, 1, 0);
			if (rnd() & 3)
				insrt(val, 20, 4, 3);
			if (rnd() & 3)
				insrt(val, 24, 4, 3);
			if (rnd() & 3)
				insrt(val, 28, 4, 0);
			if (rnd() & 1)
				insrt(val, 24, 4, extr(val, 20, 4));
			if (rnd() & 3) {
				if (extr(val, 4, 2) == 2)
					insrt(val, 28, 4, 0);
				if (extr(val, 4, 2) == 1)
					insrt(val, 24, 8, 0);
			}
			if (rnd() & 1) {
				if (rnd() & 3)
					insrt(val, 2, 1, 0);
				if (rnd() & 3)
					insrt(val, 16, 4, 1);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		bool cube = extr(val, 2, 1);
		bool border = extr(val, 3, 1);
		int mode = extr(val, 4, 2);
		int fmt = extr(val, 8, 7);
		int mips = extr(val, 16, 4);
		int su = extr(val, 20, 4);
		int sv = extr(val, 24, 4);
		int sw = extr(val, 28, 4);
		int xlat = kelvin_tex_format(&exp, fmt);
		int kind = xlat & KELVIN_TEX_FMT_KIND;
		bool rect = kind == KELVIN_TEX_FMT_RECT || kind == KELVIN_TEX_FMT_UNK24;
		bool is_dxt = kind == KELVIN_TEX_FMT_DXT;
		bool is_unk24 = kind == KELVIN_TEX_FMT_UNK24;
		bool zcomp = xlat & KELVIN_TEX_FMT_ZCOMP;
		if (!extr(val, 0, 2) || extr(val, 0, 2) == 3)
			return false;
		if (kind == KELVIN_TEX_FMT_INVALID)
			return false;
		// Deprected on Kelvin?
		if (fmt == 8 || fmt == 9 || fmt == 0xa)
			return false;
		if (fmt >= 0x42 && cls == 0x97)
			return false;
		if (fmt >= 0x4a && nv04_pgraph_is_kelvin_class(&exp))
			return false;
		if (fmt >= 0x4f)
			return false;
		if (nv04_pgraph_is_rankine_class(&exp)) {
			if (!border && (is_dxt || is_unk24)) {
				return false;
			}
		}
		if (rect) {
			if (cube)
				return false;
			if (mips != 1)
				return false;
			if (mode == 3)
				return false;
		}
		if (cube) {
			if (mode != 2)
				return false;
			if (zcomp)
				return false;
			if (su != sv)
				return false;
		}
		if (mode == 0) {
			return false;
		} else if (mode == 1) {
			int max = border ? 0xc : 0xb;
			if (su > max)
				return false;
			if (sv || sw)
				return false;
		} else if (mode == 2) {
			int max = border ? 0xc : 0xb;
			if (su > max || sv > max)
				return false;
			if (sw)
				return false;
		} else if (mode == 3) {
			int max = border ? 0x9 : 0x8;
			if (su > max || sv > max || sw > max)
				return false;
			if (zcomp)
				return false;
		}
		if (extr(val, 6, 2))
			return false;
		if (extr(val, 15, 1))
			return false;
		if (mips > 0xd || mips == 0)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			int mode = extr(val, 4, 2);
			int fmt = extr(val, 8, 7);
			bool zcomp = (fmt >= 0x2a && fmt <= 0x31);
			int mips = extr(val, 16, 4);
			int su = extr(val, 20, 4);
			int sv = extr(val, 24, 4);
			int sw = extr(val, 28, 4);
			if (mips > su && mips > sv && mips > sw)
				mips = std::max(su, std::max(sv, sw)) + 1;
			insrt(exp.bundle_tex_format[idx], 1, 3, extr(val, 1, 3));
			insrt(exp.bundle_tex_format[idx], 6, 2, mode);
			insrt(exp.bundle_tex_format[idx], 8, 7, fmt);
			insrt(exp.bundle_tex_format[idx], 16, 4, mips);
			insrt(exp.bundle_tex_format[idx], 20, 4, su);
			insrt(exp.bundle_tex_format[idx], 24, 4, sv);
			insrt(exp.bundle_tex_format[idx], 28, 4, sw);
			pgraph_bundle(&exp, BUNDLE_TEX_FORMAT, idx, exp.bundle_tex_format[idx], true);
			if (chipset.card_type == 0x20) {
				insrt(exp.xf_mode_t[idx >> 1], (idx & 1) * 16 + 2, 1, mode == 3 || zcomp);
			}
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				pgraph_flush_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexWrap : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x01171717;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int mode_s = extr(val, 0, 3);
		int mode_t = extr(val, 8, 3);
		int mode_r = extr(val, 16, 3);
		if (val & ~0x01171717)
			return false;
		if (mode_s < 1 || mode_s > 5)
			return false;
		if (mode_t < 1 || mode_t > 5)
			return false;
		if (mode_r < 1 || mode_r > 5)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20) {
				exp.bundle_tex_wrap[idx] = val & 0x01171717;
				pgraph_bundle(&exp, BUNDLE_TEX_WRAP, idx, exp.bundle_tex_wrap[idx], true);
			} else {
				int cyl = 0;
				insrt(cyl, 0, 1, extr(val, 4, 1));
				insrt(cyl, 1, 1, extr(val, 12, 1));
				insrt(cyl, 2, 1, extr(val, 20, 1));
				insrt(cyl, 3, 1, extr(val, 24, 1));
				insrt(exp.bundle_txc_cylwrap, idx * 4, 4, cyl);
				pgraph_bundle(&exp, BUNDLE_TXC_CYLWRAP, 0, exp.bundle_txc_cylwrap, true);
				insrt(exp.bundle_tex_wrap[idx], 0, 3, extr(val, 0, 3));
				insrt(exp.bundle_tex_wrap[idx], 8, 3, extr(val, 8, 3));
				insrt(exp.bundle_tex_wrap[idx], 16, 3, extr(val, 16, 3));
				pgraph_bundle(&exp, BUNDLE_TEX_WRAP, idx, exp.bundle_tex_wrap[idx], true);
				if (chipset.card_type < 0x40) {
					insrt(exp.xf_mode_t[idx >> 1], (idx & 1) * 16 + 2, 1, extr(exp.bundle_config_b, 6, 1) && !cyl);
					pgraph_flush_xf_mode(&exp);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineTexWrap : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf3ff17f7;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int mode_s = extr(val, 0, 3);
		int mode_t = extr(val, 8, 3);
		int mode_r = extr(val, 16, 3);
		int unk = extr(val, 24, 2);
		int unk2 = extr(val, 28, 4);
		if (val & ~0xf3f717f7)
			return false;
		if (mode_s < 1 || mode_s > 5)
			return false;
		if (mode_t < 1 || mode_t > 5)
			return false;
		if (mode_r < 1 || mode_r > 5)
			return false;
		if (unk == 3)
			return false;
		if (unk2 >= 0x8)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_tex_wrap[idx], 0, 3, extr(val, 0, 3));
			insrt(exp.bundle_tex_wrap[idx], 4, 7, extr(val, 4, 7));
			insrt(exp.bundle_tex_wrap[idx], 12, 1, extr(val, 12, 1));
			insrt(exp.bundle_tex_wrap[idx], 13, 2, extr(val, 24, 2));
			insrt(exp.bundle_tex_wrap[idx], 16, 3, extr(val, 16, 3));
			insrt(exp.bundle_tex_wrap[idx], 24, 4, extr(val, 20, 4));
			insrt(exp.bundle_tex_wrap[idx], 28, 4, extr(val, 28, 4));
			pgraph_bundle(&exp, BUNDLE_TEX_WRAP, idx, exp.bundle_tex_wrap[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineTexFilterOpt : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x0000001f;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & ~0x0000001f)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_tex_wrap[idx], 19, 5, val);
			pgraph_bundle(&exp, BUNDLE_TEX_WRAP, idx, exp.bundle_tex_wrap[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexControl : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (extr(val, 31, 1))
			return false;
		if (nv04_pgraph_is_celsius_class(&exp) && extr(val, 5, 1))
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_control[idx] = val & 0x7fffffff;

			pgraph_bundle(&exp, BUNDLE_TEX_CONTROL, idx, exp.bundle_tex_control[idx], true);
			if (pgraph_3d_class(&exp) < PGRAPH_3D_RANKINE) {
				insrt(exp.xf_mode_t[idx >> 1], (idx & 1) * 16, 1, extr(val, 30, 1));
				pgraph_flush_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexPitch : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0xffff;
			if (!(rnd() & 3)) {
				val &= 0xe00f0000;
			}
			if (!(rnd() & 3))
				val = 0;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_celsius_class(&exp) || nv04_pgraph_is_kelvin_class(&exp)) {
			return !(val & 0xffff) && !!(val & 0xfff80000);
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (nv04_pgraph_is_celsius_class(&exp) || nv04_pgraph_is_kelvin_class(&exp)) {
				insrt(exp.bundle_tex_pitch[idx], 16, 16, extr(val, 16, 16));
			} else {
				exp.bundle_tex_pitch[idx] = val;
			}
			pgraph_bundle(&exp, BUNDLE_TEX_PITCH, idx, exp.bundle_tex_pitch[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexFilter : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 13, 11, 0);
			if (rnd() & 3)
				insrt(val, 24, 4, 1);
			if (rnd() & 3)
				insrt(val, 28, 4, 1);
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (extr(val, 13, 11))
			return false;
		if (extr(val, 24, 4) < 1 || extr(val, 24, 4) > 6)
			return false;
		if (extr(val, 28, 4) < 1 || extr(val, 28, 4) > 2)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			insrt(exp.bundle_tex_filter[idx], 0, 13, extr(val, 0, 13));
			insrt(exp.bundle_tex_filter[idx], 16, 4, extr(val, 24, 4));
			insrt(exp.bundle_tex_filter[idx], 20, 2, 0);
			insrt(exp.bundle_tex_filter[idx], 24, 2, extr(val, 28, 2));
			insrt(exp.bundle_tex_filter[idx], 26, 2, 0);
			pgraph_bundle(&exp, BUNDLE_TEX_FILTER, idx, exp.bundle_tex_filter[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexFilter : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 13, 3, 1);
			if (rnd() & 3)
				insrt(val, 16, 6, 1);
			if (rnd() & 3)
				insrt(val, 22, 2, 0);
			if (rnd() & 3)
				insrt(val, 24, 4, 1);
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int unk = extr(val, 13, 3);
		int min = extr(val, 16, 6);
		int mag = extr(val, 24, 4);
		if (extr(val, 22, 2))
			return false;
		if (unk == 0)
			return false;
		if (unk == 3 && cls == 0x97)
			return false;
		if (unk > 3)
			return false;
		if (min < 1 || min > 7)
			return false;
		if (mag != 1 && mag != 2 && mag != 4)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_filter[idx] = val & 0xff3fffff;
			pgraph_bundle(&exp, BUNDLE_TEX_FILTER, idx, exp.bundle_tex_filter[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexRect : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				val &= ~0xf000f000;
			}
			if (rnd() & 1) {
				if (rnd() & 1) {
					val &= ~0xffff;
				} else {
					val &= ~0xffff0000;
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_celsius_class(&exp)) {
			if (extr(val, 16, 1))
				return false;
			if (!extr(val, 0, 16) || extr(val, 0, 16) >= 0x800)
				return false;
			if (!extr(val, 17, 15) || extr(val, 17, 15) >= 0x800)
				return false;
		} else {
			if (!extr(val, 0, 16) || extr(val, 0, 16) > 0x1000)
				return false;
			if (!extr(val, 16, 16) || extr(val, 16, 16) > 0x1000)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_rect[idx] = val & 0x1fff1fff;
			pgraph_bundle(&exp, BUNDLE_TEX_RECT, idx, exp.bundle_tex_rect[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexPalette : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0x32;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_celsius_class(&exp)) {
			return !(val & 0x3e);
		} else {
			return !(val & 0x32);
		}
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_palette[idx] = val & ~0x32;
			pgraph_bundle(&exp, BUNDLE_TEX_PALETTE, idx, exp.bundle_tex_palette[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexBorderColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_border_color[idx] = val;
			pgraph_bundle(&exp, BUNDLE_TEX_BORDER_COLOR, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk10 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 0xf))
			insrt(val, 0, 31, 0x40ffffe0 + (rnd() & 0x3f));
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return extr(val, 0, 31) <= 0x41000000;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_unk10[idx] = val;
			pgraph_bundle(&exp, BUNDLE_TEX_UNK10, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk11 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 0xf))
			insrt(val, 0, 31, 0x40ffffe0 + (rnd() & 0x3f));
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return extr(val, 0, 31) <= 0x41000000;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_unk11[idx] = val;
			pgraph_bundle(&exp, BUNDLE_TEX_UNK11, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk12 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 0xf))
			insrt(val, 0, 31, 0x40ffffe0 + (rnd() & 0x3f));
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return extr(val, 0, 31) <= 0x41000000;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_unk12[idx] = val;
			pgraph_bundle(&exp, BUNDLE_TEX_UNK12, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk13 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 0xf))
			insrt(val, 0, 31, 0x40ffffe0 + (rnd() & 0x3f));
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return extr(val, 0, 31) <= 0x41000000;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_unk13[idx] = val;
			pgraph_bundle(&exp, BUNDLE_TEX_UNK13, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk14 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_unk14[idx] = val;
			pgraph_bundle(&exp, BUNDLE_TEX_UNK14, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk15 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_unk15[idx] = val;
			pgraph_bundle(&exp, BUNDLE_TEX_UNK15, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexColorKey : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return nv04_pgraph_is_celsius_class(&exp);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (pgraph_in_begin_end(&exp) && nv04_pgraph_is_celsius_class(&exp)) {
			warn(4);
		} else {
			if (!exp.nsource) {
				exp.bundle_tex_color_key[idx] = val;
				pgraph_bundle(&exp, BUNDLE_TEX_COLOR_KEY, idx, val, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcFactor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			if (idx == 0) {
				exp.bundle_rc_factor_0[0] = val;
				pgraph_bundle(&exp, BUNDLE_RC_FACTOR_A, 0, val, true);
			} else {
				exp.bundle_rc_factor_1[0] = val;
				pgraph_bundle(&exp, BUNDLE_RC_FACTOR_B, 0, val, true);
			}
			exp.bundle_rc_final_factor[idx] = val;
			pgraph_bundle(&exp, BUNDLE_RC_FINAL_FACTOR, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcOutAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 18, 14, 0);
			}
			if (rnd() & 1) {
				insrt(val, 28, 2, 1);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 3);
		if (op == 5 || op == 7)
			return false;
		if (extr(val, 12, 2))
			return false;
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 18, 14))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			exp.bundle_rc_out_alpha[idx] = val & 0x3cfff;
			pgraph_bundle(&exp, BUNDLE_RC_OUT_ALPHA, idx, exp.bundle_rc_out_alpha[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcOutColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 18, 14, 0);
			}
			if (rnd() & 1) {
				insrt(val, 28, 2, 1);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 3);
		if (op == 5 || op == 7)
			return false;
		if (!idx) {
			if (extr(val, 27, 3))
				return false;
		} else {
			int cnt = extr(val, 28, 2);
			if (cnt == 0 || cnt == 3)
				return false;
		}
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 18, 9))
			return false;
		if (extr(val, 30, 2))
			return false;
		return true;
	}
	void emulate_mthd() override {
		uint32_t rval;
		rval = val & 0x3ffff;
		if (!exp.nsource) {
			exp.bundle_rc_out_color[idx] = rval;
			pgraph_bundle(&exp, BUNDLE_RC_OUT_COLOR, idx, exp.bundle_rc_out_color[idx], true);
			if (idx) {
				insrt(exp.bundle_rc_config, 0, 4, extr(val, 28, 4));
				insrt(exp.bundle_rc_config, 8, 1, extr(val, 27, 1));
				insrt(exp.bundle_rc_config, 12, 1, 0);
				insrt(exp.bundle_rc_config, 16, 1, 0);
				pgraph_bundle(&exp, BUNDLE_RC_CONFIG, 0, exp.bundle_rc_config, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcInAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (nv04_pgraph_is_celsius_class(&exp) && (reg == 0xa || reg == 0xb))
				return false;
			if (reg == 6 || reg == 7 || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_rc_in_alpha[idx] = val;
			pgraph_bundle(&exp, BUNDLE_RC_IN_ALPHA, idx, exp.bundle_rc_in_alpha[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcInColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (nv04_pgraph_is_celsius_class(&exp) && (reg == 0xa || reg == 0xb))
				return false;
			if (reg == 6 || reg == 7 || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_rc_in_color[idx] = val;
			pgraph_bundle(&exp, BUNDLE_RC_IN_COLOR, idx, exp.bundle_rc_in_color[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFactorA : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_rc_factor_0[idx] = val;
			pgraph_bundle(&exp, BUNDLE_RC_FACTOR_A, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFactorB : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_rc_factor_1[idx] = val;
			pgraph_bundle(&exp, BUNDLE_RC_FACTOR_B, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFinalFactor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_rc_final_factor[idx] = val;
			pgraph_bundle(&exp, BUNDLE_RC_FINAL_FACTOR, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcOutAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 18, 1, 0);
			}
			if (rnd() & 1) {
				insrt(val, 19, 3, 0);
				insrt(val, 26, 6, 0);
			}
			if (rnd() & 1) {
				insrt(val, 22, 4, 0);
			}
			if (rnd() & 1) {
				insrt(val, 28, 2, 1);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 4);
		if (op == 5 || op == 7)
			return false;
		if (op >= 8 && nv04_pgraph_is_kelvin_class(&exp))
			return false;
		if (op == 8 || op == 9 || op == 0xb || op == 0xd || op == 0xf)
			return false;
		if (extr(val, 12, 2))
			return false;
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xa && reg != 0xb && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 19, 3))
			return false;
		if (extr(val, 22, 4) && cls == 0x97)
			return false;
		if (extr(val, 26, 6))
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			uint32_t rval;
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				if (!nv04_pgraph_is_nv25p(&chipset))
					rval = val & 0x3cfff;
				else
					rval = val & 0x3c3cfff;
				if (chipset.card_type == 0x30) {
					int op = extr(val, 15, 3);
					if (op == 6)
						insrt(rval, 15, 4, 0xe);
				}
			} else {
				rval = val & 0x3c7cfff;
			}
			exp.bundle_rc_out_alpha[idx] = rval;
			pgraph_bundle(&exp, BUNDLE_RC_OUT_ALPHA, idx, rval, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcOutColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 16, 1, 0);
			}
			if (rnd() & 1) {
				insrt(val, 27, 5, 0);
			}
			if (rnd() & 1) {
				insrt(val, 20, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 22, 4, 0);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_rankine_class(&exp)) {
			int op = extr(val, 15, 4);
			if (op == 5 || op == 7 || op == 8 || op == 9 || op == 0xb || op == 0xd || op == 0xf)
				return false;
		} else {
			int op = extr(val, 15, 3);
			if (op == 5 || op == 7)
				return false;
			if (extr(val, 20, 6) && cls == 0x97)
				return false;
			if (extr(val, 26, 1))
				return false;
		}
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xa && reg != 0xb && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 27, 5))
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			uint32_t rval;
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				if (!nv04_pgraph_is_nv25p(&chipset))
					rval = val & 0xfffff;
				else
					rval = val & 0x3ffffff;
				if (chipset.card_type == 0x30) {
					rval = val & 0x3ffff;
					insrt(rval, 19, 8, extr(val, 18, 8));
					int op = extr(val, 15, 3);
					if (op == 6)
						insrt(rval, 15, 4, 0xe);
				}
			} else {

				rval = val & 0x7ffffff;
			}
			exp.bundle_rc_out_color[idx] = rval;
			pgraph_bundle(&exp, BUNDLE_RC_OUT_COLOR, idx, exp.bundle_rc_out_color[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcConfig : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x01f1110f;
			if (rnd() & 1) {
				insrt(val, 20, 4, 0);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & ~0x1f1110f)
			return false;
		if (cls == 0x97 && extr(val, 20, 4))
			return false;
		if (nv04_pgraph_is_kelvin_class(&exp) && extr(val, 24, 1))
			return false;
		if (extr(val, 0, 4) > 8)
			return false;
		if (nv04_pgraph_is_kelvin_class(&exp) && extr(val, 0, 4) == 0)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20) {
				if (!nv04_pgraph_is_nv25p(&chipset))
					exp.bundle_rc_config = val & 0x1110f;
				else
					exp.bundle_rc_config = val & 0xf1110f;
			} else {
				exp.bundle_rc_config = val & 0x1f1110f;
			}
			pgraph_bundle(&exp, BUNDLE_RC_CONFIG, 0, exp.bundle_rc_config, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFinalA : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f3f3f3f;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xc0c0c0c0)
			return false;
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7)
				return false;
			if (nv04_pgraph_is_celsius_class(&exp) && (reg == 0xa || reg == 0xb))
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_rc_final_a = val & 0x3f3f3f3f;
			pgraph_bundle(&exp, BUNDLE_RC_FINAL_A, 0, exp.bundle_rc_final_a, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFinalB : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f3f3fe0;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xc0c0c01f)
			return false;
		for (int j = 1; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg >= 0xe)
				return false;
			if (nv04_pgraph_is_celsius_class(&exp) && (reg == 0xa || reg == 0xb))
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_rc_final_b = val & 0x3f3f3fe0;
			pgraph_bundle(&exp, BUNDLE_RC_FINAL_B, 0, exp.bundle_rc_final_b, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusConfig : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x31111101;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xceeeeefe)
			return false;
		if (extr(val, 28, 2) && cls != 0x99)
			return false;
		if (extr(val, 8, 1) && !extr(val, 16, 1))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			insrt(exp.bundle_config_a, 25, 1, extr(val, 0, 1));
			insrt(exp.bundle_config_a, 23, 1, extr(val, 16, 1));
			insrt(exp.bundle_config_a, 30, 2, 0);
			insrt(exp.bundle_config_b, 2, 1, extr(val, 24, 1));
			insrt(exp.bundle_config_b, 6, 1, extr(val, 20, 1));
			insrt(exp.bundle_config_b, 10, 4, extr(val, 8, 4));
			insrt(exp.bundle_raster, 29, 1, extr(val, 12, 1));
			pgraph_bundle(&exp, BUNDLE_CONFIG_A, 0, exp.bundle_config_a, true);
			pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
			pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusLightModel : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0x00010007;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xfffefff8);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			insrt(exp.fe3d_misc, 2, 1, extr(val, 0, 1));
			insrt(exp.xf_mode_a, 30, 1, extr(val, 16, 1));
			insrt(exp.xf_mode_a, 28, 1, extr(val, 2, 1));
			insrt(exp.xf_mode_a, 18, 1, extr(val, 1, 1));
			int spec = 0;
			if (extr(exp.fe3d_misc, 3, 1))
				spec = extr(exp.fe3d_misc, 2, 1) ? 2 : 1;
			insrt(exp.xf_mode_a, 19, 2, spec);
			pgraph_flush_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusLightMaterial : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (val & ~0xf) {
			warn(1);
		} else {
			if (!exp.nsource) {
				insrt(exp.fe3d_misc, 3, 1, extr(val, 3, 1));
				insrt(exp.fe3d_misc, 21, 1, extr(val, 0, 1) && !extr(val, 1, 1));
				insrt(exp.fe3d_misc, 22, 1, extr(val, 1, 1));
				int spec = 0;
				if (extr(exp.fe3d_misc, 3, 1))
					spec = extr(exp.fe3d_misc, 2, 1) ? 2 : 1;
				insrt(exp.xf_mode_a, 19, 2, spec);
				insrt(exp.xf_mode_a, 21, 2, extr(val, 2, 1));
				insrt(exp.xf_mode_a, 23, 2, extr(val, 1, 1));
				insrt(exp.xf_mode_a, 25, 2, extr(val, 0, 1) && !extr(val, 1, 1));
				uint32_t material_factor_rgb[3];
				uint32_t light_model_ambient[3];
				pgraph_emu_celsius_calc_material(&exp, light_model_ambient, material_factor_rgb);
				pgraph_ld_ltctx2(&exp, 0x43, 0, material_factor_rgb[0], material_factor_rgb[1]);
				pgraph_flush_xf_mode(&exp);
				pgraph_ld_ltctx(&exp, 0x43, 2, material_factor_rgb[2]);
				pgraph_ld_ltctx2(&exp, 0x41, 0, light_model_ambient[0], light_model_ambient[1]);
				pgraph_ld_ltctx(&exp, 0x41, 2, light_model_ambient[2]);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusFogMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
				if (rnd() & 1) {
					val |= (rnd() & 1 ? 0x800 : 0x2600);
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		uint32_t rv = 0;
		if (val == 0x800) {
			rv = 1;
		} else if (val == 0x801) {
			rv = 3;
		} else if (val == 0x802) {
			rv = 5;
		} else if (val == 0x803) {
			rv = 7;
		} else if (val == 0x2601) {
			rv = 0;
		} else {
			err |= 1;
		}
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 16, 3, rv);
				pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
				insrt(exp.xf_mode_b, 21, 1, rv & 1);
				pgraph_flush_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusFogCoord : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 3)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				int rval = val;
				if (val == 0)
					rval = 4;
				insrt(exp.xf_mode_b, 22, 3, rval);
				pgraph_flush_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinConfig : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x71111101;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & ~0x71111101)
			return false;
		if (cls == 0x97) {
			if (extr(val, 28, 3) > 2)
				return false;
		} else if (cls == 0x597) {
			if (extr(val, 28, 3) > 3)
				return false;
		} else {
			if (extr(val, 28, 3) > 3)
				return false;
		}
		if (pgraph_3d_class(&exp) >= PGRAPH_3D_RANKINE) {
			if (extr(val, 0, 1))
				return false;
		}
		if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE) {
			if (extr(val, 8, 1))
				return false;
		} else {
			if (extr(val, 24, 1))
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				insrt(exp.bundle_config_a, 25, 1, extr(val, 0, 1));
			} else if (chipset.card_type < 0x40) {
				insrt(exp.bundle_config_a, 25, 1, 1);
			}
			if (chipset.card_type < 0x40)
				insrt(exp.bundle_config_a, 23, 1, extr(val, 16, 1));
			if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE) {
				insrt(exp.bundle_config_b, 2, 1, extr(val, 24, 1));
			} else {
				insrt(exp.bundle_config_b, 19, 1, extr(val, 8, 1));
				insrt(exp.bundle_config_b, 21, 1, extr(val, 16, 1));
			}
			insrt(exp.bundle_config_b, 6, 1, extr(val, 20, 1));
			if (!nv04_pgraph_is_nv25p(&chipset)) {
				insrt(exp.bundle_config_a, 30, 2, extr(val, 28, 2));
			} else {
				insrt(exp.bundle_config_b, 28, 3, extr(val, 28, 3));
			}
			insrt(exp.bundle_raster, 29, 1, extr(val, 12, 1));
			pgraph_bundle(&exp, BUNDLE_CONFIG_A, 0, exp.bundle_config_a, true);
			pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
			pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			if (chipset.card_type == 0x30) {
				for (int i = 0; i < 8; i++) {
					insrt(exp.xf_mode_t[i >> 1], (i & 1) * 16 + 2, 1, extr(exp.bundle_config_b, 6, 1) && !extr(exp.bundle_txc_cylwrap, i * 4, 4));
				}
				pgraph_flush_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLightModel : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0x00030001;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xfffcfffe);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type < 0x40) {
				insrt(exp.xf_mode_a, 30, 1, extr(val, 16, 1));
				insrt(exp.xf_mode_a, 17, 1, extr(val, 17, 1));
				insrt(exp.xf_mode_a, 18, 1, extr(val, 0, 1));
				if (chipset.card_type == 0x30)
					insrt(exp.xf_mode_a, 28, 1, extr(val, 2, 1));
				pgraph_flush_xf_mode(&exp);
			} else {
				insrt(exp.bundle_xf_light, 16, 1, extr(val, 16, 1));
				insrt(exp.bundle_xf_light, 17, 1, extr(val, 17, 1));
				insrt(exp.bundle_xf_light, 18, 1, extr(val, 0, 1));
				pgraph_bundle(&exp, BUNDLE_XF_LIGHT, 0, exp.bundle_xf_light, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLightMaterial : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0x1ffff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_kelvin_class(&exp)) {
			if (val & ~0x1ffff)
				return false;
		} else {
			if (val & ~0xffff)
				return false;
		}
		for (int i = 0; i < 8; i++)
			if (extr(val, i*2, 2) == 3)
				return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (chipset.card_type < 0x40) {
				insrt(exp.xf_mode_a, 0, 2, extr(val, 14, 2));
				insrt(exp.xf_mode_a, 2, 2, extr(val, 12, 2));
				insrt(exp.xf_mode_a, 4, 2, extr(val, 10, 2));
				insrt(exp.xf_mode_a, 6, 2, extr(val, 8, 2));
				insrt(exp.xf_mode_a, 19, 2, extr(val, 6, 2));
				insrt(exp.xf_mode_a, 21, 2, extr(val, 4, 2));
				insrt(exp.xf_mode_a, 23, 2, extr(val, 2, 2));
				insrt(exp.xf_mode_a, 25, 2, extr(val, 0, 2));
				pgraph_flush_xf_mode(&exp);
			} else {
				insrt(exp.bundle_xf_a, 3, 16, val);
				pgraph_bundle(&exp, BUNDLE_XF_A, 0, exp.bundle_xf_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
				if (rnd() & 1) {
					val |= (rnd() & 1 ? 0x800 : 0x2600);
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		uint32_t rv = 0;
		if (val == 0x800) {
			rv = 1;
		} else if (val == 0x801) {
			rv = 3;
		} else if (val == 0x802) {
			rv = 5;
		} else if (val == 0x803) {
			rv = 7;
		} else if (val == 0x804) {
			rv = 4;
		} else if (val == 0x2601) {
			rv = 0;
		} else {
			err |= 1;
		}
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 16, 3, rv);
				pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
				if (chipset.card_type == 0x20) {
					insrt(exp.xf_mode_b, 21, 1, rv & 1);
					pgraph_flush_xf_mode(&exp);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogCoord : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() {
		if (nv04_pgraph_is_kelvin_class(&exp))
			return val < 4 || val == 6;
		else
			return val < 4;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			int rval = val & 7;
			if (chipset.card_type < 0x40) {
				if (chipset.card_type == 0x20) {
					if (rval == 6)
						rval = 4;
					insrt(exp.xf_mode_b, 22, 3, rval);
				} else {
					if (nv04_pgraph_is_kelvin_class(&exp)) {
						if (rval >= 4)
							rval = 3;
						else if (rval == 3)
							rval = 2;
					}
					insrt(exp.xf_mode_b, 22, 2, rval);
				}
				pgraph_flush_xf_mode(&exp);
			} else {
				insrt(exp.bundle_xf_a, 19, 3, rval);
				pgraph_bundle(&exp, BUNDLE_XF_A, 0, exp.bundle_xf_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 8, 1, val);
				pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
				if (chipset.card_type < 0x40) {
					insrt(exp.xf_mode_b, 19, 1, val);
					pgraph_flush_xf_mode(&exp);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (pgraph_in_begin_end(&exp) && nv04_pgraph_is_celsius_class(&exp)) {
			warn(4);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_fog_color, 0, 8, extr(val, 16, 8));
				insrt(exp.bundle_fog_color, 8, 8, extr(val, 8, 8));
				insrt(exp.bundle_fog_color, 16, 8, extr(val, 0, 8));
				insrt(exp.bundle_fog_color, 24, 8, extr(val, 24, 8));
				pgraph_bundle(&exp, BUNDLE_FOG_COLOR, 0, exp.bundle_fog_color, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClipRectMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_raster, 31, 1, val);
			pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClipRect : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x00010001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_celsius_class(&exp)) {
			return !(val & 0xf000f000) && extrs(val, 0, 12) <= extrs(val, 16, 12);
		} else {
			return !(val & 0xf000f000) && extr(val, 0, 12) <= extr(val, 16, 12);
		}
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			uint32_t rval = val & 0x0fff0fff;
			if (nv04_pgraph_is_celsius_class(&exp))
				rval ^= 0x08000800;
			if (chipset.card_type >= 0x30) {
				if (which == 0)
					exp.fe3d_shadow_clip_rect_horiz = val;
				else
					exp.fe3d_shadow_clip_rect_vert = val;
				if (nv04_pgraph_is_kelvin_class(&exp)) {
					insrt(exp.fe3d_misc, 4 + which * 8, 8, 1);
				} else {
					insrt(exp.fe3d_misc, 4 + which * 8 + idx, 1, 1);
					for (int i = idx + 1; i < 8; i++) {
						insrt(exp.fe3d_misc, 4 + which * 8 + i, 1, 0);
					}
				}
			}
			if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE) {
				for (int i = idx; i < 8; i++) {
					if (which == 0)
						exp.bundle_clip_rect_horiz[i] = rval;
					else
						exp.bundle_clip_rect_vert[i] = rval;
				}
			} else {
				if (which == 0)
					exp.bundle_clip_rect_horiz[idx] = rval;
				else
					exp.bundle_clip_rect_vert[idx] = rval;
			}
			if (which == 0)
				pgraph_bundle(&exp, BUNDLE_CLIP_RECT_HORIZ, idx, rval, true);
			else
				pgraph_bundle(&exp, BUNDLE_CLIP_RECT_VERT, idx, rval, true);
		}
	}
public:
	MthdKelvinClipRect(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, uint32_t stride, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num, stride), which(which) {}
};

class MthdKelvinAlphaFuncEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 12, 1, val);
				pgraph_bundle(&exp, BUNDLE_CONFIG_A, 0, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinBlendFuncEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type < 0x40) {
					insrt(exp.bundle_blend, 3, 1, val);
				} else {
					insrt(exp.bundle_blend, 28, 1, val);
				}
				pgraph_bundle(&exp, BUNDLE_BLEND, 0, exp.bundle_blend, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinCullFaceEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 28, 1, val);
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDepthTestEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 14, 1, val);
				pgraph_bundle(&exp, BUNDLE_CONFIG_A, 0, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDitherEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 22, 1, val);
				pgraph_bundle(&exp, BUNDLE_CONFIG_A, 0, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLightingEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type < 0x40) {
					insrt(exp.xf_mode_a, 31, 1, val);
					pgraph_flush_xf_mode(&exp);
				} else {
					insrt(exp.bundle_xf_a, 22, 1, val);
					pgraph_bundle(&exp, BUNDLE_XF_A, 0, exp.bundle_xf_a, true);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPointParamsEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 9, 1, val);
				pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
				if (chipset.card_type < 0x40) {
					insrt(exp.xf_mode_b, 25, 1, val);
					pgraph_flush_xf_mode(&exp);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPointSmoothEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20) {
					insrt(exp.bundle_raster, 9, 1, val);
					pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
				} else {
					insrt(exp.bundle_config_b, 1, 1, val);
					pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLineSmoothEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 10, 1, val);
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonSmoothEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 11, 1, val);
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusWeightEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.xf_mode_b, 26, 3, val);
				pgraph_flush_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinWeightMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return val < 7;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type < 0x40) {
				insrt(exp.xf_mode_b, 26, 3, val);
				pgraph_flush_xf_mode(&exp);
			} else {
				insrt(exp.bundle_xf_a, 23, 3, val);
				pgraph_bundle(&exp, BUNDLE_XF_A, 0, exp.bundle_xf_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinStencilEnable : public SingleMthdTest {
	int side;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_stencil_a, side, 1, val);
				pgraph_bundle(&exp, BUNDLE_STENCIL_A, 0, exp.bundle_stencil_a, true);
			}
		}
	}
public:
	MthdKelvinStencilEnable(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), side(side) {}
};

class MthdKelvinPolygonOffsetPointEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 6, 1, val);
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonOffsetLineEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 7, 1, val);
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonOffsetFillEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 8, 1, val);
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinAlphaFuncFunc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 0x200;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t rval = 0;
		uint32_t err = 0;
		if (val >= 0x200 && val < 0x208) {
			rval = val & 7;
		} else if (val > 0 && val <= 8 && pgraph_3d_class(&exp) == PGRAPH_3D_CURIE) {
			rval = val - 1;
		} else {
			err |= 1;
		}
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 8, 4, rval);
				pgraph_bundle(&exp, BUNDLE_CONFIG_A, 0, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinAlphaFuncRef : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xff;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	bool is_valid_val() override {
		if (pgraph_3d_class(&exp) == PGRAPH_3D_CURIE) {
			return val < 0x10000;
		} else if (pgraph_3d_class(&exp) == PGRAPH_3D_RANKINE) {
			return val < 0x100;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val & ~0xff && pgraph_3d_class(&exp) < PGRAPH_3D_RANKINE)
			err |= 2;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type < 0x40) {
					insrt(exp.bundle_config_a, 0, 8, val);
					pgraph_bundle(&exp, BUNDLE_CONFIG_A, 0, exp.bundle_config_a, true);
				} else {
					exp.bundle_alpha_func_ref = val & 0xffff;
					pgraph_bundle(&exp, BUNDLE_ALPHA_FUNC_REF, 0, exp.bundle_alpha_func_ref, true);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinBlendFunc : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3) {
				val &= 0xffff000f;
				if (rnd() & 1) {
					val |= (rnd() & 1 ? 0x8000 : rnd() & 1 ? 0x300 : 0x1000);
				}
			}
			if (rnd() & 3) {
				val &= 0x000fffff;
				if (rnd() & 1) {
					val |= (rnd() & 1 ? 0x8000 : rnd() & 1 ? 0x300 : 0x1000) << 16;
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		uint32_t rv[2] = {0};
		for (int i = 0; i < 2; i++) {
			switch (extr(val, i * 16, 16)) {
			case 0: rv[i] = 0; break;
			case 1: rv[i] = 1; break;
			case 0x300: rv[i] = 2; break;
			case 0x301: rv[i] = 3; break;
			case 0x302: rv[i] = 4; break;
			case 0x303: rv[i] = 5; break;
			case 0x304: rv[i] = 6; break;
			case 0x305: rv[i] = 7; break;
			case 0x306: rv[i] = 8; break;
			case 0x307: rv[i] = 9; break;
			case 0x308: rv[i] = 0xa; break;
			case 0x8001: rv[i] = 0xc; break;
			case 0x8002: rv[i] = 0xd; break;
			case 0x8003: rv[i] = 0xe; break;
			case 0x8004: rv[i] = 0xf; break;
			// D3D...
			case 0x1001: rv[i] = 0x0; break;
			case 0x1002: rv[i] = 0x1; break;
			case 0x1003: rv[i] = 0x2; break;
			case 0x1004: rv[i] = 0x3; break;
			case 0x1005: rv[i] = 0x4; break;
			case 0x1006: rv[i] = 0x5; break;
			case 0x1007: rv[i] = 0x6; break;
			case 0x1008: rv[i] = 0x7; break;
			case 0x1009: rv[i] = 0x8; break;
			case 0x100a: rv[i] = 0x9; break;
			case 0x100b: rv[i] = 0xa; break;
			case 0x100c: rv[i] = 0x4; break;
			case 0x100d: rv[i] = 0x5; break;
			case 0x100e: rv[i] = 0xc; break;
			case 0x100f: rv[i] = 0xd; break;
			default:
				err |= 1;
				break;
			}
			if (extr(val, i * 16 + 12, 1) && pgraph_3d_class(&exp) < PGRAPH_3D_CURIE)
				err |= 1;
		}
		if (pgraph_3d_class(&exp) < PGRAPH_3D_RANKINE) {
			if (rv[1])
				err |= 1;
			rv[1] = rv[0];
		}
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_blend, 4 + which * 4, 4, rv[0]);
				if (chipset.card_type >= 0x30) {
					insrt(exp.bundle_blend, 20 + which * 4, 4, rv[1]);
				}
				pgraph_bundle(&exp, BUNDLE_BLEND, 0, exp.bundle_blend, true);
			}
		}
	}
public:
	MthdKelvinBlendFunc(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinBlendColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (pgraph_in_begin_end(&exp)) {
			warn(4);
		} else {
			if (!exp.nsource) {
				exp.bundle_blend_color = val;
				pgraph_bundle(&exp, BUNDLE_BLEND_COLOR, 0, exp.bundle_blend_color, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinBlendEquation : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				val &= 0xffff;
			} else {
				if (rnd() & 1) {
					val &= 0xfffff;
				}
				if (rnd() & 1) {
					val |= (rnd() & 1 ? 0x8000 : 0xf000) << 16;
				}
			}
			if (rnd() & 1) {
				val &= 0xffff000f;
			}
			if (rnd() & 1) {
				val |= (rnd() & 1 ? 0x8000 : 0xf000);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		uint32_t rv[2] = {0};
		for (int i = 0; i < 2; i++) {
			uint32_t sub = extr(val, i * 16, 16);
			if (i == 1 && pgraph_3d_class(&exp) < PGRAPH_3D_CURIE) {
				rv[1] = rv[0];
				if (sub != 0)
					err |= 1;
			} else {
				switch (sub) {
				case 0x8006: rv[i] = 0x2; break;
				case 0x8007: rv[i] = 0x3; break;
				case 0x8008: rv[i] = 0x4; break;
				case 0x800a: rv[i] = 0x0; break;
				case 0x800b: rv[i] = 0x1; break;
				case 0xf005: rv[i] = 0x5; break;
				case 0xf006: rv[i] = 0x6; break;
				case 0xf007: rv[i] = 0x7; break;
				case 1: rv[i] = 0x2; break;
				case 2: rv[i] = 0x0; break;
				case 3: rv[i] = 0x1; break;
				case 4: rv[i] = 0x3; break;
				case 5: rv[i] = 0x4; break;
				default:
					err |= 1;
					break;
				}
				if (sub >= 0xf000 && pgraph_3d_class(&exp) < PGRAPH_3D_KELVIN)
					err |= 1;
				if (sub == 0xf007 && cls == 0x97)
					err |= 1;
				if (sub < 0x100 && pgraph_3d_class(&exp) < PGRAPH_3D_CURIE)
					err |= 1;
			}
		}
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_blend, 0, 3, rv[0]);
				if (chipset.card_type >= 0x40)
					insrt(exp.bundle_blend, 17, 3, rv[1]);
				pgraph_bundle(&exp, BUNDLE_BLEND, 0, exp.bundle_blend, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDepthFunc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 0x200;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t rval = 0;
		uint32_t err = 0;
		if (val >= 0x200 && val < 0x208) {
			rval = val & 7;
		} else if (val > 0 && val <= 8 && pgraph_3d_class(&exp) == PGRAPH_3D_CURIE) {
			rval = val - 1;
		} else {
			err |= 1;
		}
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 16, 4, rval);
				pgraph_bundle(&exp, BUNDLE_CONFIG_A, 0, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinColorMask : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x1010101;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val & ~0x1010101)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type < 0x40) {
					insrt(exp.bundle_config_a, 29, 1, extr(val, 0, 1));
					insrt(exp.bundle_config_a, 28, 1, extr(val, 8, 1));
					insrt(exp.bundle_config_a, 27, 1, extr(val, 16, 1));
					insrt(exp.bundle_config_a, 26, 1, extr(val, 24, 1));
					pgraph_bundle(&exp, BUNDLE_CONFIG_A, 0, exp.bundle_config_a, true);
				} else {
					insrt(exp.bundle_color_mask, 11, 1, extr(val, 0, 1));
					insrt(exp.bundle_color_mask, 10, 1, extr(val, 8, 1));
					insrt(exp.bundle_color_mask, 9, 1, extr(val, 16, 1));
					insrt(exp.bundle_color_mask, 8, 1, extr(val, 24, 1));
					pgraph_bundle(&exp, BUNDLE_COLOR_MASK, 0, exp.bundle_color_mask, true);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDepthWriteEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_a, 24, 1, val);
				pgraph_bundle(&exp, BUNDLE_CONFIG_A, 0, exp.bundle_config_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinStencilVal : public SingleMthdTest {
	int side, which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xff;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (which == 0 && val & ~0xff && pgraph_3d_class(&exp) >= PGRAPH_3D_RANKINE)
			return false;
		return true;
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (which == 0 && val & ~0xff && pgraph_3d_class(&exp) < PGRAPH_3D_RANKINE)
			err |= 2;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (side == 0) {
					insrt(exp.bundle_stencil_a, 8 + 8 * which, 8, val);
					pgraph_bundle(&exp, BUNDLE_STENCIL_A, 0, exp.bundle_stencil_a, true);
				} else {
					if (which < 2) {
						insrt(exp.bundle_stencil_d, which ? 0 : 8, 8, val);
						pgraph_bundle(&exp, BUNDLE_STENCIL_D, 0, exp.bundle_stencil_d, true);
					} else if (which == 2) {
						insrt(exp.bundle_stencil_c, 0, 8, val);
						pgraph_bundle(&exp, BUNDLE_STENCIL_C, 0, exp.bundle_stencil_c, true);
					}
				}
			}
		}
	}
public:
	MthdKelvinStencilVal(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), side(side), which(which) {}
};

class MthdKelvinStencilFunc : public SingleMthdTest {
	int side;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 0x200;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t rval = 0;
		uint32_t err = 0;
		if (val >= 0x200 && val < 0x208) {
			rval = val & 7;
		} else if (val > 0 && val <= 8 && pgraph_3d_class(&exp) == PGRAPH_3D_CURIE) {
			rval = val - 1;
		} else {
			err |= 1;
		}
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (side == 0) {
					insrt(exp.bundle_stencil_a, 4, 4, rval);
					pgraph_bundle(&exp, BUNDLE_STENCIL_A, 0, exp.bundle_stencil_a, true);
				} else {
					insrt(exp.bundle_stencil_d, 16, 4, rval);
					pgraph_bundle(&exp, BUNDLE_STENCIL_D, 0, exp.bundle_stencil_d, true);
				}
			}
		}
	}
public:
	MthdKelvinStencilFunc(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), side(side) {}
};

class MthdKelvinStencilOp : public SingleMthdTest {
	int side, which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= (rnd() & 1 ? 0x1500 : rnd() & 1 ? 0x8500 : 0x1e00);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		uint32_t rv = 0;
		switch (val) {
			case 0: rv = 2; break;
			case 0x150a: rv = 6; break;
			case 0x1e00: rv = 1; break;
			case 0x1e01: rv = 3; break;
			case 0x1e02: rv = 4; break;
			case 0x1e03: rv = 5; break;
			case 0x8507: rv = 7; break;
			case 0x8508: rv = 8; break;
			case 1: rv = 1; break;
			case 2: rv = 2; break;
			case 3: rv = 3; break;
			case 4: rv = 4; break;
			case 5: rv = 5; break;
			case 6: rv = 6; break;
			case 7: rv = 7; break;
			case 8: rv = 8; break;
			default:
				err |= 1;
		}
		if (val > 0 && val < 0x100 && pgraph_3d_class(&exp) < PGRAPH_3D_CURIE)
			err |= 1;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (side == 0) {
					insrt(exp.bundle_stencil_b, 4 * which, 4, rv);
					pgraph_bundle(&exp, BUNDLE_STENCIL_B, 0, exp.bundle_stencil_b, true);
				} else {
					insrt(exp.bundle_stencil_c, 16 - which * 4, 4, rv);
					pgraph_bundle(&exp, BUNDLE_STENCIL_C, 0, exp.bundle_stencil_c, true);
				}
			}
		}
	}
public:
	MthdKelvinStencilOp(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), side(side), which(which) {}
};

class MthdKelvinShadeMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xf0ff;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 0x1d00;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		switch (val) {
			case 0x1d00:
				break;
			case 0x1d01:
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 7, 1, val);
				pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLineWidth : public SingleMthdTest {
	void adjust_orig_mthd() override {
		val = sext(val, rnd() & 0x1f);
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (pgraph_3d_class(&exp) >= PGRAPH_3D_RANKINE)
			return val < 0x200;
		return true;
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (val >= 0x200 && pgraph_3d_class(&exp) < PGRAPH_3D_RANKINE)
			err |= 2;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				uint32_t rv = val & 0x1ff;
				if (chipset.chipset == 0x10)
					rv &= 0xff;
				insrt(exp.bundle_raster, 12, 9, rv);
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonOffsetFactor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_polygon_offset_factor = val;
				pgraph_bundle(&exp, BUNDLE_POLYGON_OFFSET_FACTOR, 0, exp.bundle_polygon_offset_factor, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonOffsetUnits : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_polygon_offset_units = val;
				pgraph_bundle(&exp, BUNDLE_POLYGON_OFFSET_UNITS, 0, exp.bundle_polygon_offset_units, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonMode : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 0x1b00;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		uint32_t rv = 0;
		switch (val) {
			case 0x1b00:
				rv = 1;
				break;
			case 0x1b01:
				rv = 2;
				break;
			case 0x1b02:
				rv = 0;
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, which * 2, 2, rv);
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			}
		}
	}
public:
	MthdKelvinPolygonMode(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinDepthRangeNear : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return (cls & 0xff) != 0x97;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp) && (cls & 0xff) != 0x97)
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_depth_range_near = val;
				pgraph_bundle(&exp, BUNDLE_DEPTH_RANGE_NEAR, 0, exp.bundle_depth_range_near, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDepthRangeFar : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return (cls & 0xff) != 0x97;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp) && (cls & 0xff) != 0x97)
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_depth_range_far = val;
				pgraph_bundle(&exp, BUNDLE_DEPTH_RANGE_FAR, 0, exp.bundle_depth_range_far, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinCullFace : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 0x400;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		uint32_t rv = 0;
		switch (val) {
			case 0x404:
				rv = 1;
				break;
			case 0x405:
				rv = 2;
				break;
			case 0x408:
				rv = 3;
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 21, 2, rv);
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFrontFace : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		} else if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 0x900;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		switch (val) {
			case 0x900:
				break;
			case 0x901:
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 23, 1, val);
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinNormalizeEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type < 0x40) {
					insrt(exp.xf_mode_a, 27, 1, val);
					pgraph_flush_xf_mode(&exp);
				} else {
					insrt(exp.bundle_xf_a, 26, 1, val);
					pgraph_bundle(&exp, BUNDLE_XF_A, 0, exp.bundle_xf_a, true);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinSpecularEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 5, 1, val);
				pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
				if (!nv04_pgraph_is_celsius_class(&exp) && chipset.card_type < 0x40) {
					insrt(exp.xf_mode_a, 16, 1, val);
					pgraph_flush_xf_mode(&exp);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLightEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		if (val & ~0xffff)
			return false;
		if ((cls & 0xff) != 0x97) {
			bool off = false;
			for (int i = 0; i < 8; i++) {
				int mode = extr(val, i * 2, 2);
				if (off && mode != 0)
					return false;
				if (mode == 0)
					off = true;
			}
		}
		return true;
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp) && (cls & 0xff) != 0x97)
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type < 0x40) {
					insrt(exp.xf_mode_b, 0, 16, val);
					pgraph_flush_xf_mode(&exp);
				} else {
					insrt(exp.bundle_xf_light, 0, 16, val);
					pgraph_bundle(&exp, BUNDLE_XF_LIGHT, 0, exp.bundle_xf_light, true);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexGenMode : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= (rnd() & 1 ? 0x8550 : rnd() & 1 ? 0x8510 : 0x2400);
			}
			if (!(rnd() & 3)) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		} else if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1)
				val &= 0xff0f;
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		uint32_t rv = 0;
		switch (val) {
			case 0:
				rv = 0;
				break;
			case 0x2400:
				rv = 1;
				break;
			case 0x2401:
				rv = 2;
				break;
			case 0x8511:
				if (which < 3) {
					rv = 4;
				} else {
					err |= 1;
				}
				break;
			case 0x8512:
				if (which < 3) {
					rv = 5;
				} else {
					err |= 1;
				}
				break;
			case 0x855f:
				if (which < 3 && (cls & 0xff) != 0x97) {
					rv = (idx & 1) ? 6 : 0;
				} else {
					err |= 1;
				}
				break;
			case 0x2402:
				if (which < 2) {
					rv = 3;
				} else {
					err |= 1;
				}
				break;
			default:
				err |= 1;
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type < 0x40) {
					insrt(exp.xf_mode_t[idx >> 1], 4 + (idx & 1) * 16 + which * 3, 3, rv);
					pgraph_flush_xf_mode(&exp);
				} else {
					insrt(exp.bundle_xf_txc[idx], which * 4, 3, rv);
					pgraph_bundle(&exp, BUNDLE_XF_TXC, idx, exp.bundle_xf_txc[idx], true);
				}
			}
		}
	}
public:
	MthdKelvinTexGenMode(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, uint32_t stride, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num, stride), which(which) {}
};

class MthdKelvinTexMatrixEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type < 0x40) {
					insrt(exp.xf_mode_t[idx >> 1], 1 + (idx & 1) * 16, 1, val);
					pgraph_flush_xf_mode(&exp);
				} else {
					insrt(exp.bundle_xf_txc[idx], 16, 1, val);
					pgraph_bundle(&exp, BUNDLE_XF_TXC, idx, exp.bundle_xf_txc[idx], true);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTlMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (val & ~7)
			err |= 1;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (!nv04_pgraph_is_nv25p(&chipset))
					insrt(exp.bundle_tex_shader_op, 3, 1, extr(val, 0, 1));
				else
					insrt(exp.bundle_tex_shader_op, 29, 1, extr(val, 0, 1));
				insrt(exp.bundle_tex_shader_op, 30, 1, extr(val, 1, 1));
				insrt(exp.bundle_tex_shader_op, 31, 1, extr(val, 2, 1));
				pgraph_bundle(&exp, BUNDLE_TEX_SHADER_OP, 0, exp.bundle_tex_shader_op, true);
				insrt(exp.xf_mode_b, 30, 2, extr(val, 0, 1));
				pgraph_flush_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0x117;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		int mode = extr(val, 0, 2);
		if (nv04_pgraph_is_kelvin_class(&exp)) {
			if (val & ~7)
				return false;
			if (mode != 0 && mode != 2)
				return false;
		} else if (nv04_pgraph_is_rankine_class(&exp)) {
			if (val & ~0x117)
				return false;
			if (mode != 0 && mode != 2 && mode != 3)
				return false;
		} else {
			if (val & ~0x113)
				return false;
			if (mode != 1 && mode != 2 && mode != 3)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type < 0x40) {
				insrt(exp.xf_mode_b, 18, 1, extr(val, 2, 1));
				insrt(exp.xf_mode_b, 30, 2, extr(val, 0, 2));
				if (chipset.card_type == 0x30) {
					insrt(exp.xf_mode_b, 16, 1, extr(val, 8, 1));
					insrt(exp.xf_mode_b, 17, 1, extr(val, 4, 1));
				}
				pgraph_flush_xf_mode(&exp);
			} else {
				if (pgraph_3d_class(&exp)< PGRAPH_3D_CURIE)
					insrt(exp.bundle_xf_a, 2, 1, extr(val, 2, 1));
				insrt(exp.bundle_xf_a, 28, 1, extr(val, 8, 1));
				insrt(exp.bundle_xf_c, 30, 2, extr(val, 0, 2));
				insrt(exp.bundle_xf_c, 27, 1, extr(val, 4, 1));
				pgraph_bundle(&exp, BUNDLE_XF_A, 0, exp.bundle_xf_a, true);
				pgraph_bundle(&exp, BUNDLE_XF_C, 0, exp.bundle_xf_c, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPointSize : public SingleMthdTest {
	void adjust_orig_mthd() override {
		val = sext(val, rnd() & 0x1f);
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (val >= 0x200 && (nv04_pgraph_is_celsius_class(&exp) || cls == 0x97))
			err |= 2;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (!nv04_pgraph_is_nv25p(&chipset)) {
					exp.bundle_point_size = val & 0x1ff;
				} else {
					uint32_t rval = val;
					if (cls == 0x97 && val) {
						int exp = 0x84;
						int fract = val;
						while (!extr(fract, 8, 1)) {
							fract <<= 1;
							exp--;
						}
						rval = exp << 23 | extr(fract, 0, 8) << 15;
					}
					exp.bundle_point_size = rval;
				}
				pgraph_bundle(&exp, BUNDLE_POINT_SIZE, 0, exp.bundle_point_size, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk3f0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x801fff0f;
			if (rnd() & 1) {
				val &= 0x8000000f;
			}
		}
		if (rnd() & 1) {
			val |= 1 << (rnd() & 0x1f);
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & ~0x801fff0f)
			return false;
		int unk0 = extr(val, 0, 4);
		int unk8 = extr(val, 8, 8);
		int unk16 = extr(val, 16, 4);
		bool unk20 = extr(val, 20, 1);
		bool unk31 = extr(val, 31, 1);
		if (nv04_pgraph_is_celsius_class(&exp) && unk0 > 3)
			return false;
		if (unk0 > 4 && unk0 != 0xf)
			return false;
		if (unk31 && cls != 0x597)
			return false;
		if (pgraph_3d_class(&exp) < PGRAPH_3D_RANKINE) {
			if (unk8 || unk16 || unk20)
				return false;
		} else if (pgraph_3d_class(&exp) == PGRAPH_3D_RANKINE) {
			if (unk16 == 0)
				return false;
			if (unk16 > 8)
				return false;
		} else {
			if (unk20)
				return false;
			if (unk8)
				return false;
		}
		return true;
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp) && nv04_pgraph_is_celsius_class(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				int rval = extr(val, 0, 4);
				if (chipset.card_type == 0x30) {
					if (rval == 0xd)
						rval = 1;
					else if (rval == 0xe)
						rval = 2;
					else if (rval > 4 && rval != 0xf)
						rval = 0;
				} else if (chipset.card_type == 0x40) {
					if (rval == 5 || rval == 0xd)
						rval = 7;
				}
				insrt(exp.bundle_raster, 25, 3, rval);
				if (nv04_pgraph_is_nv25p(&chipset) && chipset.card_type < 0x40)
					insrt(exp.bundle_raster, 4, 1, extr(val, 31, 1));
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
				if (nv04_pgraph_is_rankine_class(&exp) && chipset.card_type < 0x40) {
					int unk16 = extr(val, 16, 4);
					if (unk16 >= 1 && unk16 <= 8)
						unk16--;
					else
						unk16 = 0;
					insrt(exp.bundle_ps_control, 8, 8, extr(val, 8, 8));
					insrt(exp.bundle_ps_control, 17, 1, extr(val, 20, 1));
					insrt(exp.bundle_ps_control, 28, 3, unk16);
					pgraph_bundle(&exp, BUNDLE_PS_CONTROL, idx, exp.bundle_ps_control, true);
				}
				if (pgraph_3d_class(&exp) >= PGRAPH_3D_CURIE) {
					insrt(exp.bundle_ps_control, 27, 4, extr(val, 16, 4));
					pgraph_bundle(&exp, BUNDLE_PS_CONTROL, idx, exp.bundle_ps_control, true);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFlatshadeFirst : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		if (rnd() & 1) {
			val |= 1 << (rnd() & 0x1f);
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp) && (cls & 0xff) != 0x97)
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_config_b, 0, 1, val);
				pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusOldUnk3f8 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		if (rnd() & 1) {
			val |= 1 << (rnd() & 0x1f);
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 3)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				pgraph_bundle(&exp, BUNDLE_TEX_SHADER_CONST_EYE, 2, exp.bundle_tex_shader_const_eye[2], true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogCoeff : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (idx < 2) {
					exp.bundle_fog_coeff[idx] = val;
					pgraph_bundle(&exp, BUNDLE_FOG_COEFF, idx, val, true);
				} else {
					if (chipset.card_type == 0x20) {
						pgraph_ld_ltctx2(&exp, 0x45, 0, exp.bundle_fog_coeff[0], exp.bundle_fog_coeff[1]);
						pgraph_ld_ltctx(&exp, 0x45, 2, val);
					}
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinColorLogicOpEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		if (rnd() & 1) {
			val |= 1 << (rnd() & 0x1f);
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_blend, 16, 1, val);
				pgraph_bundle(&exp, BUNDLE_BLEND, 0, exp.bundle_blend, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinColorLogicOpOp : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 0x1500;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (val >= 0x1500 && val < 0x1510) {
		} else {
			err |= 1;
		}
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_blend, 12, 4, val);
				pgraph_bundle(&exp, BUNDLE_BLEND, 0, exp.bundle_blend, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLightTwoSideEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				insrt(exp.bundle_raster, 24, 1, val);
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
				if (chipset.card_type < 0x40) {
					insrt(exp.xf_mode_a, 29, 1, val);
					pgraph_flush_xf_mode(&exp);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlUnk9cc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type < 0x40) {
				insrt(exp.xf_mode_b, 20, 1, val);
				pgraph_flush_xf_mode(&exp);
			} else {
				insrt(exp.bundle_xf_a, 0, 1, val);
				pgraph_bundle(&exp, BUNDLE_XF_A, 0, exp.bundle_xf_a, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonStippleEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (cls == 0x597)
			return val < 4;
		else
			return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (!nv04_pgraph_is_nv25p(&chipset)) {
				insrt(exp.bundle_raster, 4, 1, val);
				pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			} else {
				if (nv04_pgraph_is_kelvin_class(&exp)) {
					insrt(exp.bundle_line_stipple, 0, 2, val);
				} else {
					insrt(exp.bundle_line_stipple, 0, 1, val);
				}
				pgraph_bundle(&exp, BUNDLE_LINE_STIPPLE, 0, exp.bundle_line_stipple, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPolygonStipple : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			pgraph_bundle(&exp, BUNDLE_POLYGON_STIPPLE, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinZPassCounterReset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE) {
			return val == 1;
		} else {
			return val == 1 || val == 2;
		}
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			pgraph_bundle(&exp, BUNDLE_ZPASS_COUNTER_RESET, 0, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinZPassCounterEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_config_b, 20, 1, val);
			pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexShaderCullMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0xffff);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_shader_cull_mode = val & 0xffff;
			pgraph_bundle(&exp, BUNDLE_TEX_SHADER_CULL_MODE, 0, exp.bundle_tex_shader_cull_mode, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexShaderConstEye : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_tex_shader_const_eye[idx] = val;
			pgraph_bundle(&exp, BUNDLE_TEX_SHADER_CONST_EYE, idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexShaderOp : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xfffe7;
			if ((rnd() & 3) == 0) {
				insrt(val, 5, 5, 0x11);
			}
			if ((rnd() & 3) == 0) {
				insrt(val, 10, 5, 0x11);
			} else if ((rnd() & 3) == 0) {
				insrt(val, 10, 5, 0x0b);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & ~0xfffff)
			return false;
		int prev = 0;
		int pprev = 0;
		bool had_depr = false;
		for (int i = 0; i < 4; i++) {
			int op = extr(val, i * 5, 5);
			if (cls == 0x97) {
				if (op > 0x13)
					return false;
				if (op == 0x13 && i != 3)
					return false;
			} else {
				if (i == 0)
					op &= ~8;
			}
			if (!nv04_pgraph_is_nv25p(&chipset)) {
				if (op > 0x12)
					return false;
			} else {
				if (op >= 0x13 && op <= 0x17) {
					if (!extr(exp.debug_l, 1, 1))
						return false;
				}
			}
			if (op >= 0x18 && op < 0x1c)
				return false;
			if (op > 0x1d)
				return false;
			if (op == 0xa || op == 0x17) {
				if (had_depr)
					return false;
				had_depr = true;
			}
			if (op >= 6 && i == 0)
				return false;
			if (op >= 6 && i == 1 && (prev == 0 || prev == 5))
				return false;
			if (op == 0x11 && i == 3)
				return false;
			if (op == 8 && i < 2)
				return false;
			if (op == 8) {
				if (prev == 0 || prev == 5 || prev == 0x11 || prev == 0x17)
					return false;
				if (pprev == 0 || pprev == 5 || pprev == 0x11 || pprev == 0x17)
					return false;
			}
			if (op == 9 || op == 0xa) {
				if (prev != 0x11)
					return false;
			}
			if (op == 0xb) {
				if (prev != 0x11)
					return false;
				if (i == 3)
					return false;
				int next = extr(val, (i + 1) * 5, 5);
				if (next != 0xc && next != 0x12)
					return false;
			}
			if (op == 0xc || op == 0xd || op == 0xe || op == 0x12) {
				if (prev != 0x11 && prev != 0x0b)
					return false;
				if (pprev != 0x11)
					return false;
			}

			pprev = prev;
			prev = extr(val, i * 5, 5);
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (!nv04_pgraph_is_nv25p(&chipset)) {
				insrt(exp.bundle_tex_shader_op, 0, 3, extr(val, 0, 3));
				insrt(exp.bundle_tex_shader_op, 5, 15, extr(val, 5, 15));
			} else {
				insrt(exp.bundle_tex_shader_op, 0, 20, val);
			}
			pgraph_bundle(&exp, BUNDLE_TEX_SHADER_OP, 0, exp.bundle_tex_shader_op, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexShaderDotmapping : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1)
				val &= 0x777;
			else
				val &= 0x3f000fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (cls == 0x97) {
			if (val & ~0x777)
				return false;
		} else {
			if (val & ~0x3f000fff)
				return false;
		}
		for (int i = 0; i < 3; i++) {
			int map = extr(val, i * 4, 4);
			if (map == 5 && !extr(exp.debug_b, 6, 1))
				return false;
			if (map == 6 && !extr(exp.debug_b, 7, 1))
				return false;
			if (map >= 11)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (!nv04_pgraph_is_nv25p(&chipset)) {
				insrt(exp.bundle_tex_shader_misc, 0, 3, extr(val, 0, 3));
				insrt(exp.bundle_tex_shader_misc, 3, 3, extr(val, 4, 3));
				insrt(exp.bundle_tex_shader_misc, 6, 3, extr(val, 8, 3));
			} else {
				insrt(exp.bundle_tex_shader_misc, 0, 12, extr(val, 0, 12));
				insrt(exp.bundle_tex_shader_misc, 24, 6,  extr(val, 24, 6));
			}
			pgraph_bundle(&exp, BUNDLE_TEX_SHADER_MISC, 0, exp.bundle_tex_shader_misc, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexShaderPrevious : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x310001;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & ~0x310000)
			return false;
		if (extr(val, 20, 2) == 3)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_tex_shader_misc, 15, 1, extr(val, 0, 1));
			insrt(exp.bundle_tex_shader_misc, 16, 1, extr(val, 16, 1));
			insrt(exp.bundle_tex_shader_misc, 20, 2, extr(val, 20, 2));
			pgraph_bundle(&exp, BUNDLE_TEX_SHADER_MISC, 0, exp.bundle_tex_shader_misc, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinStateSave : public SingleMthdTest {
	bool supported() override {
		return chipset.card_type < 0x40;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val == 0;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			insrt(exp.fe3d_misc, 20, 1, 1);
			// XXX: model the extra grobj fields...
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFenceOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x0f000ffc;
			if (rnd() & 1) {
				val &= 0xffc;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE) {
			if (chipset.card_type == 0x20)
				return !(val & ~0xffc);
			else
				return !(val & ~0xff0);
		} else {
			int unk = extr(val, 24, 4);
			if (unk > 4)
				return false;
			if (val & ~0x0f000ff0)
				return false;
			return true;
		}
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (chipset.card_type < 0x40)
				exp.bundle_fence_offset = val;
			else {
				if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE)
					insrt(exp.bundle_fence_offset, 0, 24, val);
				else
					exp.bundle_fence_offset = val;
			}
			pgraph_bundle(&exp, BUNDLE_FENCE_OFFSET, 0, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDepthClamp : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0x3333;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_kelvin_class(&exp)) {
			return !(val & ~0x111);
		} else if (nv04_pgraph_is_rankine_class(&exp)) {
			return !(val & ~0x1111);
		} else {
			return !(val & ~0x111);
		}
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_raster, 5, 1, extr(val, 8, 1));
			insrt(exp.bundle_raster, 30, 1, extr(val, 0, 1));
			pgraph_bundle(&exp, BUNDLE_RASTER, 0, exp.bundle_raster, true);
			insrt(exp.bundle_z_config, 4, 1, extr(val, 4, 1));
			pgraph_bundle(&exp, BUNDLE_Z_CONFIG, 0, exp.bundle_z_config, true);
			if (nv04_pgraph_is_rankine_class(&exp) && chipset.card_type < 0x40) {
				insrt(exp.bundle_ps_control, 31, 1, extr(val, 12, 1));
				pgraph_bundle(&exp, BUNDLE_PS_CONTROL, 0, exp.bundle_ps_control, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinMultisample : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff0111;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0xffff0111);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_multisample = val & 0xffff0111;
			pgraph_bundle(&exp, BUNDLE_MULTISAMPLE, 0, exp.bundle_multisample, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk1d80 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_kelvin_class(&exp))
			return val < 2;
		else
			return val < 4;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_z_config, 0, 1, val);
			if (chipset.card_type >= 0x30 && chipset.chipset != 0x34) {
				insrt(exp.bundle_z_config, 7, 1, extr(val, 1, 1));
			}
			pgraph_bundle(&exp, BUNDLE_Z_CONFIG, 0, exp.bundle_z_config, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk1d84 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xc000000f;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (cls == 0x97)
			return !(val & ~3);
		else if (cls == 0x597)
			return !(val & ~0x80000003);
		else if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE)
			return !(val & ~0xc0000003);
		else
			return !(val & ~3);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (nv04_pgraph_is_kelvin_class(&exp))
			pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_z_config, 1, 2, val);
			if (chipset.card_type == 0x30) {
				insrt(exp.bundle_z_config, 30, 1, extr(val, 30, 1));
				insrt(exp.bundle_z_config, 31, 1, extr(val, 31, 1));
			}
			pgraph_bundle(&exp, BUNDLE_Z_CONFIG, 0, exp.bundle_z_config, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClearZeta : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_clear_zeta = val;
			pgraph_bundle(&exp, BUNDLE_CLEAR_ZETA, 0, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClearColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_clear_color = val;
			pgraph_bundle(&exp, BUNDLE_CLEAR_COLOR, 0, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClearHv : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x10001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0x0fff0fff) && extr(val, 0, 16) <= extr(val, 16, 16);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (nv04_pgraph_is_kelvin_class(&exp))
			pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_clear_hv[which] = val & 0x0fff0fff;
			pgraph_bundle(&exp, BUNDLE_CLEAR_HV, which, val & 0x0fff0fff, true);
		}
	}
public:
	MthdKelvinClearHv(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinUnk1e68 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_unk06a = val;
				pgraph_bundle(&exp, BUNDLE_UNK1E68, 0, val, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexZcomp : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (cls == 0x97)
			return !(val & ~7);
		else
			return !(val & ~0xfff);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				uint32_t rval;
				if (!nv04_pgraph_is_nv25p(&chipset)) {
					rval = val & 7;
				} else if (cls == 0x97) {
					rval = (val & 7) * 0x249;
				} else {
					rval = val & 0xfff;
				}
				if (chipset.card_type == 0x20) {
					exp.bundle_tex_zcomp = rval;
					pgraph_bundle(&exp, BUNDLE_TEX_ZCOMP, 0, exp.bundle_tex_zcomp, true);
				} else {
					for (int i = 0; i < 4; i++) {
						int mode = extr(rval, i * 3, 3);
						insrt(exp.bundle_tex_wrap[i], 28, 4, mode);
						pgraph_bundle(&exp, BUNDLE_TEX_WRAP, i, exp.bundle_tex_wrap[i], true);
					}
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk1e98 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type < 0x40) {
				insrt(exp.xf_mode_b, 29, 1, val);
				pgraph_flush_xf_mode(&exp);
			} else {
				// Nothing?
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlProgramLoadPos : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x030003ff;
			if (rnd() & 1)
				val &= 0x1ff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_kelvin_class(&exp)) {
			return val < 0x88;
		} else if (nv04_pgraph_is_rankine_class(&exp)) {
			return val < 0x118;
		} else {
			return val < 0x220;
		}
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE)
			pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (nv04_pgraph_is_rankine_class(&exp))
				pgraph_flush_xf_mode(&exp);
			if (chipset.card_type == 0x20)
				insrt(exp.fe3d_xf_load_pos, 0, 8, val);
			else if (chipset.card_type == 0x30)
				insrt(exp.fe3d_xf_load_pos, 0, 9, val);
			else {
				insrt(exp.bundle_xf_load_pos, 0, 2, extr(val, 24, 2));
				insrt(exp.bundle_xf_load_pos, 2, 10, val);
				pgraph_bundle(&exp, BUNDLE_XF_LOAD_POS, 0, exp.bundle_xf_load_pos, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlProgramStartPos : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x1ff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_kelvin_class(&exp)) {
			return val < 0x88;
		} else if (nv04_pgraph_is_rankine_class(&exp)) {
			if (chipset.card_type < 0x40) {
				return val < 0x118;
			} else {
				return val < 0x100;
			}
		} else {
			return val < 0x220;
		}
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE)
			pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type < 0x40) {
				// Ummm.... won't fit on Rankine.
				insrt(exp.xf_mode_a, 8, 8, val);
				pgraph_flush_xf_mode(&exp);
			} else {
				insrt(exp.bundle_xf_c, 0, 10, val);
				pgraph_bundle(&exp, BUNDLE_XF_C, 0, exp.bundle_xf_c, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlParamLoadPos : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x030003ff;
			if (rnd() & 1)
				val &= 0x1ff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		if (nv04_pgraph_is_kelvin_class(&exp)) {
			return val < 0xc0;
		} else if (nv04_pgraph_is_rankine_class(&exp)) {
			return val < 0x19c;
		} else {
			return val < 0x220;
		}
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE)
			pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				insrt(exp.fe3d_xf_load_pos, 8, 8, val);
			else if (chipset.card_type == 0x30)
				insrt(exp.fe3d_xf_load_pos, 16, 9, val);
			else {
				insrt(exp.bundle_xf_load_pos, 16, 2, extr(val, 24, 2));
				insrt(exp.bundle_xf_load_pos, 18, 10, val);
				pgraph_bundle(&exp, BUNDLE_XF_LOAD_POS, 0, exp.bundle_xf_load_pos, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexGen : public SingleMthdTest {
	int which;
	int coord;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_xfctx(&exp, 0x40 + which * 8 + coord, idx, val);
				else {
					if (which < 4)
						pgraph_ld_xfctx(&exp, 0x7c + which * 8 + coord, idx, val);
					else
						pgraph_ld_xfctx(&exp, 0x00 + (which - 4) * 8 + coord, idx, val);
				}
			}
		}
	}
public:
	MthdKelvinTexGen(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which, int coord)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4), which(which), coord(coord) {}
};

class MthdKelvinFogPlane : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_xfctx(&exp, 0x39, idx, val);
			else
				pgraph_ld_xfctx(&exp, 0x75, idx, val);
		}
	}
public:
	MthdKelvinFogPlane(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4) {}
};

class MthdKelvinUnk16d0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_xfctx(&exp, 0x3f, idx, val);
			else
				pgraph_ld_xfctx(&exp, 0x7b, idx, val);
		}
	}
public:
	MthdKelvinUnk16d0(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4) {}
};

class MthdKelvinUnk16e0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_xfctx(&exp, 0x3c, idx, val);
			else
				pgraph_ld_xfctx(&exp, 0x78, idx, val);
		}
	}
public:
	MthdKelvinUnk16e0(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4) {}
};

class MthdKelvinUnk16f0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_xfctx(&exp, 0x3d, idx, val);
			else
				pgraph_ld_xfctx(&exp, 0x79, idx, val);
		}
	}
public:
	MthdKelvinUnk16f0(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4) {}
};

class MthdKelvinUnk1700 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_xfctx(&exp, 0x3e, idx, val);
			else
				pgraph_ld_xfctx(&exp, 0x7a, idx, val);
		}
	}
public:
	MthdKelvinUnk1700(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4) {}
};

class MthdKelvinViewportTranslate : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_3d_class(&exp) < PGRAPH_3D_RANKINE) {
			if (pgraph_in_begin_end(&exp))
				err |= 4;
		} else {
			pgraph_kelvin_check_err18(&exp);
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_xfctx(&exp, 0x3b, idx, val);
				else {
					pgraph_ld_ltctx(&exp, 0x36, idx, val);
					pgraph_ld_xfctx(&exp, 0x77, idx, val);
				}
			}
		}
	}
public:
	MthdKelvinViewportTranslate(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4) {}
};

class MthdKelvinViewportScale : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_xfctx(&exp, 0x3a, idx, val);
				else {
					pgraph_ld_ltctx(&exp, 0x37, idx, val);
					pgraph_ld_xfctx(&exp, 0x76, idx, val);
				}
			}
		}
	}
public:
	MthdKelvinViewportScale(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4) {}
};

class MthdKelvinLightEyePosition : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_xfctx(&exp, 0x38, idx, val);
				else
					pgraph_ld_xfctx(&exp, 0x74, idx, val);
			}
		}
	}
public:
	MthdKelvinLightEyePosition(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4) {}
};

class MthdKelvinLightSpotDirection : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_xfctx(&exp, 0x30 + which, idx, val);
				else
					pgraph_ld_xfctx(&exp, 0x6c + which, idx, val);
			}
		}
	}
public:
	MthdKelvinLightSpotDirection(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4), which(which) {}
};

class MthdRankineUserClipPlane : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			pgraph_ld_xfctx(&exp, 0x20 + which, idx, val);
		}
	}
public:
	MthdRankineUserClipPlane(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 4, 4), which(which) {}
};

class MthdKelvinLightPosition : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				int addr;
				if (chipset.card_type == 0x20)
					addr = 0x28 + which;
				else
					addr = 0x64 + which;
				if (idx < 2)
					pgraph_ld_xfctx(&exp, addr, idx, val);
				else
					pgraph_ld_xfctx2(&exp, addr, idx, val, val);
			}
		}
	}
public:
	MthdKelvinLightPosition(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4), which(which) {}
};

class MthdKelvinPointParamsA : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20) {
					pgraph_ld_ltctx(&exp, 0x47, idx, val);
				} else {
					if (idx < 2)
						pgraph_ld_xfctx(&exp, 0x26, idx, val);
					else
						pgraph_ld_xfctx2(&exp, 0x26, idx, val, 0x3f800000);
				}
			}
		}
	}
public:
	MthdKelvinPointParamsA(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4) {}
};

class MthdKelvinPointParamsB : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (nv04_pgraph_is_rankine_class(&exp) && idx != 0 && chipset.card_type < 0x40) {
			// nothing...
		} else {
			if (pgraph_in_begin_end(&exp))
				err |= 4;
		}
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20) {
					pgraph_ld_ltctx(&exp, 0x48, idx, val);
				} else {
					if (idx == 0)
						pgraph_ld_xfctx(&exp, 0x27, idx, val);
				}
			}
		}
	}
public:
	MthdKelvinPointParamsB(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4) {}
};

class MthdKelvinTextureMatrix : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				int addr;
				if (chipset.card_type == 0x20) {
					addr = 0x44 + which * 8;
				} else {
					if (which < 4)
						addr = 0x80 + which * 8;
					else
						addr = 0x04 + (which - 4) * 8;
				}
				pgraph_ld_xfctx(&exp, addr + (idx >> 2), idx & 3, val);
			}
		}
	}
public:
	MthdKelvinTextureMatrix(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 0x10, 4), which(which) {}
};

class MthdKelvinMvMatrix : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_xfctx(&exp, 0x08 + which * 8 + (idx >> 2), idx & 3, val);
				else
					pgraph_ld_xfctx(&exp, 0x44 + which * 8 + (idx >> 2), idx & 3, val);
			}
		}
	}
public:
	MthdKelvinMvMatrix(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 0x10, 4), which(which) {}
};

class MthdKelvinImvMatrix : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_xfctx(&exp, 0x0c + which * 8 + (idx >> 2), idx & 3, val);
				else
					pgraph_ld_xfctx(&exp, 0x48 + which * 8 + (idx >> 2), idx & 3, val);
			}
		}
	}
public:
	MthdKelvinImvMatrix(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 0x10, 4), which(which) {}
};

class MthdKelvinUnk4Matrix : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_xfctx(&exp, 0x04 + (idx >> 2), idx & 3, val);
				else
					pgraph_ld_xfctx(&exp, 0x40 + (idx >> 2), idx & 3, val);
			}
		}
	}
public:
	MthdKelvinUnk4Matrix(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 0x10, 4) {}
};

class MthdKelvinProjMatrix : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_xfctx(&exp, 0x00 + (idx >> 2), idx & 3, val);
				else
					pgraph_ld_xfctx(&exp, 0x3c + (idx >> 2), idx & 3, val);
			}
		}
	}
public:
	MthdKelvinProjMatrix(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 0x10, 4) {}
};

class MthdKelvinLightColor : public SingleMthdTest {
	int which;
	int side;
	int col;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltctx(&exp, 0x00 + which * 8 + side * 5 + col, idx, val);
			else
				pgraph_ld_ltctx(&exp, 0x00 + which * 6 + side * 3 + col, idx, val);
		}
	}
public:
	MthdKelvinLightColor(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which, int side, int col)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4), which(which), side(side), col(col) {}
};

class MthdKelvinLightHalfVectorAtt : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_ltctx(&exp, 0x3 + which * 8, idx, val);
				else {
					if (idx < 2)
						pgraph_ld_xfctx(&exp, 0x30 + which, idx, val);
					else
						pgraph_ld_xfctx2(&exp, 0x30 + which, idx, val, 0x3f800000);
				}
			}
		}
	}
public:
	MthdKelvinLightHalfVectorAtt(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4), which(which) {}
};

class MthdKelvinLightDirection : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_ltctx(&exp, 0x4 + which * 8, idx, val);
				else {
					if (idx < 2)
						pgraph_ld_xfctx(&exp, 0x28 + which, idx, val);
					else
						pgraph_ld_xfctx2(&exp, 0x28 + which, idx, val, 0x3f800000);
				}
			}
		}
	}
public:
	MthdKelvinLightDirection(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4), which(which) {}
};

class MthdKelvinLightModelAmbientColor : public SingleMthdTest {
	int side;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltctx(&exp, 0x41 + side, idx, val);
			else
				pgraph_ld_ltctx(&exp, 0x30 + side, idx, val);
		}
	}
public:
	MthdKelvinLightModelAmbientColor(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4), side(side) {}
};

class MthdKelvinMaterialFactorRgb : public SingleMthdTest {
	int side;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltctx(&exp, 0x43 + side, idx, val);
			else
				pgraph_ld_ltctx(&exp, 0x32 + side, idx, val);
		}
	}
public:
	MthdKelvinMaterialFactorRgb(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4), side(side) {}
};

class MthdKelvinLtUnk17d4 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltctx(&exp, 0x46, idx, val);
			else
				pgraph_ld_ltctx(&exp, 0x34, idx, val);
		}
	}
public:
	MthdKelvinLtUnk17d4(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4) {}
};

class MthdKelvinLtUnk17e0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltctx(&exp, 0x40, idx, val);
			else {
				if (idx < 2)
					pgraph_ld_xfctx(&exp, 0x38, idx, val);
				else
					pgraph_ld_xfctx2(&exp, 0x38, idx, val, 0x3f800000);
			}
		}
	}
public:
	MthdKelvinLtUnk17e0(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4) {}
};

class MthdKelvinLtUnk17ec : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltctx(&exp, 0x49, idx, val);
		}
	}
public:
	MthdKelvinLtUnk17ec(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 3, 4) {}
};

class MthdKelvinMaterialFactorA : public SingleMthdTest {
	int side;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltc(&exp, 3, 0x0c + side, val);
			else
				pgraph_ld_ltc(&exp, 3, 0x0d + side, val);
		}
	}
public:
	MthdKelvinMaterialFactorA(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 1, 4), side(side) {}
};

class MthdKelvinMaterialShininessA : public SingleMthdTest {
	int side;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltc(&exp, 1, 0x01 + side, val);
			else
				pgraph_ld_ltc(&exp, 1, 0x01 + side, val);
		}
	}
public:
	MthdKelvinMaterialShininessA(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 1, 4), side(side) {}
};

class MthdKelvinMaterialShininessB : public SingleMthdTest {
	int side;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltc(&exp, 2, 0x01 + side, val);
			else
				pgraph_ld_ltc(&exp, 2, 0x01 + side, val);
		}
	}
public:
	MthdKelvinMaterialShininessB(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 1, 4), side(side) {}
};

class MthdKelvinMaterialShininessC : public SingleMthdTest {
	int side;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltc(&exp, 3, 0x02 + side, val);
			else
				pgraph_ld_ltc(&exp, 3, 0x01 + side, val);
		}
	}
public:
	MthdKelvinMaterialShininessC(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 1, 4), side(side) {}
};

class MthdKelvinMaterialShininessD : public SingleMthdTest {
	int side;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltc(&exp, 0, 0x02 + side, val);
			else
				pgraph_ld_ltc(&exp, 1, 0x03 + side, val);
		}
	}
public:
	MthdKelvinMaterialShininessD(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 1, 4), side(side) {}
};

class MthdKelvinMaterialShininessE : public SingleMthdTest {
	int side;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltc(&exp, 2, 0x03 + side, val);
			else
				pgraph_ld_ltc(&exp, 2, 0x03 + side, val);
		}
	}
public:
	MthdKelvinMaterialShininessE(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 1, 4), side(side) {}
};

class MthdKelvinMaterialShininessF : public SingleMthdTest {
	int side;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (chipset.card_type == 0x20)
				pgraph_ld_ltc(&exp, 2, 0x05 + side, val);
			else
				pgraph_ld_ltc(&exp, 3, 0x03 + side, val);
		}
	}
public:
	MthdKelvinMaterialShininessF(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int side)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 1, 4), side(side) {}
};

class MthdKelvinLightLocalRange : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_ltc(&exp, 1, 4 + which, val);
				else
					pgraph_ld_ltc(&exp, 1, 5 + which, val);
			}
		}
	}
public:
	MthdKelvinLightLocalRange(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinLightSpotCutoffA : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_ltc(&exp, 1, 0xc + which, val);
				else
					pgraph_ld_ltc(&exp, 1, 0xd + which, val);
			}
		}
	}
public:
	MthdKelvinLightSpotCutoffA(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinLightSpotCutoffB : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_ltc(&exp, 2, 0x7 + which, val);
				else
					pgraph_ld_ltc(&exp, 2, 0x5 + which, val);
			}
		}
	}
public:
	MthdKelvinLightSpotCutoffB(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinLightSpotCutoffC : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_ltc(&exp, 3, 0x4 + which, val);
				else
					pgraph_ld_ltc(&exp, 3, 0x5 + which, val);
			}
		}
	}
public:
	MthdKelvinLightSpotCutoffC(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinPointParamsC : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_ltc(&exp, 1, 0x03, val);
				else
					pgraph_ld_xfctx(&exp, 0x27, 1, val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPointParamsD : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (chipset.card_type == 0x20)
					pgraph_ld_ltc(&exp, 3, 0x01, val);
				else
					pgraph_ld_xfctx2(&exp, 0x27, 2, val, 0x3f800000);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusMaterialFactorRgb : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		exp.fe3d_emu_material_factor_rgb[idx] = val;
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				if (idx == 2) {
					pgraph_ld_ltctx2(&exp, 0x43, 0, exp.fe3d_emu_material_factor_rgb[0], exp.fe3d_emu_material_factor_rgb[1]);
					pgraph_ld_ltctx(&exp, 0x43, 2, exp.fe3d_emu_material_factor_rgb[2]);
					uint32_t material_factor_rgb[3];
					uint32_t light_model_ambient[3];
					pgraph_emu_celsius_calc_material(&exp, light_model_ambient, material_factor_rgb);
					pgraph_ld_ltctx2(&exp, 0x43, 0, material_factor_rgb[0], material_factor_rgb[1]);
					pgraph_ld_ltctx(&exp, 0x43, 2, material_factor_rgb[2]);
					pgraph_ld_ltctx2(&exp, 0x41, 0, light_model_ambient[0], light_model_ambient[1]);
					pgraph_ld_ltctx(&exp, 0x41, 2, light_model_ambient[2]);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusMaterialFactorRgbFree : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		exp.fe3d_emu_material_factor_rgb[idx] = val;
		if (!exp.nsource) {
			if (idx == 2) {
				pgraph_ld_ltctx2(&exp, 0x43, 0, exp.fe3d_emu_material_factor_rgb[0], exp.fe3d_emu_material_factor_rgb[1]);
				pgraph_ld_ltctx(&exp, 0x43, 2, exp.fe3d_emu_material_factor_rgb[2]);
				uint32_t material_factor_rgb[3];
				uint32_t light_model_ambient[3];
				pgraph_emu_celsius_calc_material(&exp, light_model_ambient, material_factor_rgb);
				pgraph_ld_ltctx2(&exp, 0x43, 0, material_factor_rgb[0], material_factor_rgb[1]);
				pgraph_ld_ltctx(&exp, 0x43, 2, material_factor_rgb[2]);
				pgraph_ld_ltctx2(&exp, 0x41, 0, light_model_ambient[0], light_model_ambient[1]);
				pgraph_ld_ltctx(&exp, 0x41, 2, light_model_ambient[2]);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusLightModelAmbient : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		exp.fe3d_emu_light_model_ambient[idx] = val;
		if (!exp.nsource) {
			if (idx == 2) {
				uint32_t material_factor_rgb[3];
				uint32_t light_model_ambient[3];
				pgraph_emu_celsius_calc_material(&exp, light_model_ambient, material_factor_rgb);
				pgraph_ld_ltctx2(&exp, 0x43, 0, material_factor_rgb[0], material_factor_rgb[1]);
				pgraph_ld_ltctx(&exp, 0x43, 2, material_factor_rgb[2]);
				pgraph_ld_ltctx2(&exp, 0x41, 0, light_model_ambient[0], light_model_ambient[1]);
				pgraph_ld_ltctx(&exp, 0x41, 2, light_model_ambient[2]);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlProgramLoad : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		int max;
		if (nv04_pgraph_is_kelvin_class(&exp))
			max = 0x88;
		else
			max = 0x118;
		int pos;
		if (chipset.card_type == 0x20)
			pos = extr(exp.fe3d_xf_load_pos, 0, 8);
		else
			pos = extr(exp.fe3d_xf_load_pos, 0, 9);
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (pos >= max)
			pgraph_state_error(&exp);
		if (!exp.nsource) {
			pgraph_ld_xfpr(&exp, pos, idx & 3, val);
		}
		if ((idx & 3) == 3 && pos < max) {
			if (chipset.card_type == 0x20) {
				insrt(exp.fe3d_xf_load_pos, 0, 8, pos + 1);
			} else {
				insrt(exp.fe3d_xf_load_pos, 0, 9, pos + 1);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTlParamLoad : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		int max;
		if (nv04_pgraph_is_kelvin_class(&exp))
			max = 0xc0;
		else
			max = 0x19c;
		int pos;
		if (chipset.card_type == 0x20)
			pos = extr(exp.fe3d_xf_load_pos, 8, 8);
		else
			pos = extr(exp.fe3d_xf_load_pos, 16, 9);
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (pos >= max)
			pgraph_state_error(&exp);
		if (!exp.nsource) {
			uint32_t rpos = pos;
			if (nv04_pgraph_is_kelvin_class(&exp) && chipset.card_type == 0x30)
				rpos += 0x3c;
			pgraph_ld_xfctx(&exp, rpos, (idx & 3), val);
		}
		if ((idx & 3) == 3 && pos < max) {
			if (chipset.card_type == 0x20)
				insrt(exp.fe3d_xf_load_pos, 8, 8, pos + 1);
			else
				insrt(exp.fe3d_xf_load_pos, 16, 9, pos + 1);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinXfUnk4 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			pgraph_ld_xfunk4(&exp, idx << 2, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinVtxAttrUByte : public SingleMthdTest {
	int which, num;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			int rwhich = which;
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				rwhich = pgraph_vtx_attr_xlat_kelvin(&exp, which);
			}
			pgraph_ld_vtx(&exp, 4, rwhich, num, idx, val);
		}
	}
public:
	MthdKelvinVtxAttrUByte(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 1), which(which), num(num) {}
};

class MthdKelvinVtxAttrShort : public SingleMthdTest {
	int which, num;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			int rwhich = which;
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				rwhich = pgraph_vtx_attr_xlat_kelvin(&exp, which);
			}
			pgraph_ld_vtx(&exp, 5, rwhich, num, idx, val);
		}
	}
public:
	MthdKelvinVtxAttrShort(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, (num+1)/2), which(which), num(num) {}
};

class MthdKelvinVtxAttrNShort : public SingleMthdTest {
	int which, num;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			int rwhich = which;
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				rwhich = pgraph_vtx_attr_xlat_kelvin(&exp, which);
			}
			pgraph_ld_vtx(&exp, 6, rwhich, num, idx, val);
		}
	}
public:
	MthdKelvinVtxAttrNShort(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, (num+1)/2), which(which), num(num) {}
};

class MthdKelvinVtxAttrFloat : public SingleMthdTest {
	int which, num;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			int rwhich = which;
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				rwhich = pgraph_vtx_attr_xlat_kelvin(&exp, which);
			}
			pgraph_ld_vtx(&exp, 7, rwhich, num, idx, val);
		}
	}
public:
	MthdKelvinVtxAttrFloat(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num), which(which), num(num) {}
};

class MthdKelvinVtxAttrUByteFree : public SingleMthdTest {
	int which, num;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			int rwhich = which;
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				rwhich = pgraph_vtx_attr_xlat_kelvin(&exp, which);
			}
			pgraph_ld_vtx(&exp, 4, rwhich, num, idx, val);
		}
	}
public:
	MthdKelvinVtxAttrUByteFree(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, 1), which(which), num(num) {}
};

class MthdKelvinVtxAttrShortFree : public SingleMthdTest {
	int which, num;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			int rwhich = which;
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				rwhich = pgraph_vtx_attr_xlat_kelvin(&exp, which);
			}
			pgraph_ld_vtx(&exp, 5, rwhich, num, idx, val);
		}
	}
public:
	MthdKelvinVtxAttrShortFree(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, (num+1)/2), which(which), num(num) {}
};

class MthdKelvinVtxAttrNShortFree : public SingleMthdTest {
	int which, num;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			int rwhich = which;
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				rwhich = pgraph_vtx_attr_xlat_kelvin(&exp, which);
			}
			pgraph_ld_vtx(&exp, 6, rwhich, num, idx, val);
		}
	}
public:
	MthdKelvinVtxAttrNShortFree(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, (num+1)/2), which(which), num(num) {}
};

class MthdKelvinVtxAttrFloatFree : public SingleMthdTest {
	int which, num;
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			int rwhich = which;
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				rwhich = pgraph_vtx_attr_xlat_kelvin(&exp, which);
			}
			pgraph_ld_vtx(&exp, 7, rwhich, num, idx, val);
		}
	}
public:
	MthdKelvinVtxAttrFloatFree(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num), which(which), num(num) {}
};

class MthdKelvinEdgeFlag : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_idx(&orig);
	}
	bool can_warn() override {
		return !nv04_pgraph_is_celsius_class(&exp);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (val > 1 && (cls & 0xff) == 0x97)
			warn(1);
		else {
			if (!exp.nsource) {
				pgraph_set_edge_flag(&exp, extr(val, 0, 1));
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnkcf0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_idx(&orig);
	}
	bool can_warn() override {
		return true;
	}
	bool is_valid_val() override {
		return nv04_pgraph_is_celsius_class(&exp) || val == 0;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp) && nv04_pgraph_is_celsius_class(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				pgraph_idx_unkf8(&exp,val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnkcf4 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_idx(&orig);
	}
	bool can_warn() override {
		return true;
	}
	bool is_valid_val() override {
		return nv04_pgraph_is_celsius_class(&exp) || val == 0;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp) && nv04_pgraph_is_celsius_class(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				pgraph_idx_unkfc(&exp,val);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinXfNop : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool is_valid_val() override {
		return (cls & 0xff) != 0x97 || val == 0;
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			pgraph_xf_nop(&exp, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinXfSync : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	bool is_valid_val() override {
		return (cls & 0xff) != 0x97 || val == 0;
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			pgraph_xf_sync(&exp, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusVtxbufOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0x0ffffffc;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0x0ffffffc);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			int ridx = pgraph_vtx_attr_xlat_celsius(&exp, idx);
			pgraph_set_vtxbuf_offset(&exp, ridx, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusVtxbufFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0x0100fc77;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
			if (rnd() & 3)
				insrt(val, 0, 3, 6);
			if (rnd() & 3)
				insrt(val, 4, 3, idx < 6 ? 3 : 1);
		}
	}
	bool can_warn() override {
		return true;
	}
	bool is_valid_val() override {
		if (idx == 0) {
			int comp = extr(val, 4, 3);
			if (extr(val, 25, 7))
				return false;
			if (!extr(val, 24, 1)) {
				if (comp == 4)
					return false;
			} else {
				if (comp == 2)
					return false;
			}
		}
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		int comp = extr(val, 4, 4);
		if (idx == 0) {
			if (comp != 2 && comp != 3 && comp != 4)
				err |= 1;
		} else if (idx == 1 || idx == 2) {
			if (comp != 0 && comp != 3 && comp != 4)
				err |= 1;
		} else if (idx == 3 || idx == 4) {
			if (comp > 4)
				err |= 1;
		} else if (idx == 5) {
			if (comp != 0 && comp != 3)
				err |= 1;
		} else if (idx == 6 || idx == 7) {
			if (comp != 0 && comp != 1)
				err |= 1;
		}
		int fmt = extr(val, 0, 4);
		if (idx == 1 || idx == 2) {
			if (fmt != 0 && fmt != 2 && fmt != 4)
				err |= 1;
		} else {
			if (fmt != 1 && fmt != 2)
				err |= 1;
		}
		if (val & 0xff000000 && idx != 0)
			err |= 2;
		if (val & 0x00ff0300)
			err |= 2;
		if (pgraph_in_begin_end(&exp) && (cls == 0x56 || cls == 0x85))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				uint32_t rval = val;
				if (fmt == 1 && idx != 5 && idx != 6)
					insrt(rval, 0, 3, 5);
				insrt(rval, 24, 1, 0);
				if (extr(val, 24, 1) && comp == 3) {
					insrt(rval, 4, 3, 7);
				}
				int ridx = pgraph_vtx_attr_xlat_celsius(&exp, idx);
				pgraph_set_vtxbuf_format(&exp, ridx, rval);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinVtxbufOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0x8fffffff;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE)
			return !(val & ~0x8fffffff);
		else
			return !(val & ~0x9fffffff);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			int which = idx;
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				which = pgraph_vtx_attr_xlat_kelvin(&exp, which);
			}
			pgraph_set_vtxbuf_offset(&exp, which, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinVtxbufFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
		if (rnd() & 1) {
			val &= 0x0000ff77;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
			if (rnd() & 3)
				insrt(val, 0, 3, 6);
			if (rnd() & 3)
				insrt(val, 4, 3, idx < 6 ? 3 : 1);
		}
	}
	bool is_valid_val() override {
		if (extr(val, 16, 16))
			return false;
		int fmt = extr(val, 0, 4);
		int comp = extr(val, 4, 4);
		if (fmt == 1 || fmt == 2 || fmt == 4 || fmt == 5) {
			// OK
		} else if (fmt == 0) {
			if (comp != 3 && comp != 4)
				return false;
		} else if (fmt == 6) {
			if (comp != 1)
				return false;
		} else {
			return false;
		}
		if (comp > 4 && comp != 7)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			int which = idx;
			if (nv04_pgraph_is_kelvin_class(&exp)) {
				which = pgraph_vtx_attr_xlat_kelvin(&exp, which);
			}
			pgraph_set_vtxbuf_format(&exp, which, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFdBeginPatchA : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (chipset.card_type != 0x20)
			return;
		adjust_orig_idx(&orig);
	}
	bool is_valid_val() override {
		if (chipset.card_type != 0x20)
			return true;
		if (extr(val, 0, 4) == 0)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (chipset.card_type != 0x20) {
			nv04_pgraph_missing_hw(&exp);
			return;
		}
		if (extr(exp.debug_d, 3, 1) && !extr(exp.debug_fd_check_skip, 0, 1)) {
			if (extr(exp.fe3d_misc, 4, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 5, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 24, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
		}
		if (!exp.nsource) {
			insrt(exp.fe3d_misc, 24, 1, 0);
			insrt(exp.fe3d_misc, 5, 1, 1);
			exp.fe3d_shadow_begin_patch_a = val;
			pgraph_fd_cmd(&exp, 0x2800, val);
		} else {
			insrt(exp.fe3d_misc, 24, 1, 1);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFdBeginPatchB : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (chipset.card_type != 0x20)
			return;
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		if (chipset.card_type != 0x20) {
			nv04_pgraph_missing_hw(&exp);
			return;
		}
		if (extr(exp.debug_d, 3, 1) && !extr(exp.debug_fd_check_skip, 1, 1)) {
			if (extr(exp.fe3d_misc, 4, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 6, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 24, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
		}
		if (!exp.nsource) {
			insrt(exp.fe3d_misc, 24, 1, 0);
			insrt(exp.fe3d_misc, 6, 1, 1);
			exp.fe3d_shadow_begin_patch_b = val;
			pgraph_fd_cmd(&exp, 0x2804, val);
		} else {
			insrt(exp.fe3d_misc, 24, 1, 1);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFdBeginPatchC : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (chipset.card_type != 0x20)
			return;
		adjust_orig_idx(&orig);
	}
	bool is_valid_val() override {
		if (chipset.card_type != 0x20)
			return true;
		if (extr(val, 16, 5) < 4)
			return false;
		if (extr(val, 16, 5) > 0x11)
			return false;
		if (extr(val, 21, 5) >= extr(val, 16, 5))
			return false;
		if (extr(val, 26, 5) >= extr(val, 16, 5))
			return false;
		if (extr(val, 31, 1))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (chipset.card_type != 0x20) {
			nv04_pgraph_missing_hw(&exp);
			return;
		}
		if (extr(exp.debug_d, 3, 1) && !extr(exp.debug_fd_check_skip, 2, 1)) {
			if (extr(exp.fe3d_misc, 4, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 7, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 24, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
		}
		if (!exp.nsource) {
			insrt(exp.fe3d_misc, 24, 1, 0);
			insrt(exp.fe3d_misc, 7, 1, 1);
			exp.fe3d_shadow_begin_patch_c = val & 0x7fffffff;
			pgraph_fd_cmd(&exp, 0x2810, val);
		} else {
			insrt(exp.fe3d_misc, 24, 1, 1);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFdBeginPatchD : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (chipset.card_type != 0x20)
			return;
		adjust_orig_idx(&orig);
		// Prevents things from blowing up...
		insrt(orig.fd_state_unk30, 10, 1, 0);
		if (rnd() & 1) {
			insrt(val, 17, 7, 0);
		}
		if (rnd() & 1) {
			if (rnd() & 7)
				insrt(orig.debug_d, 3, 1, 1);
			if (rnd() & 7)
				orig.debug_fd_check_skip = 0;
			if (rnd() & 7) {
				orig.fe3d_misc &= 0xfeffff0f;
				orig.fe3d_misc |= 0x000000e0;
			}
			if (rnd() & 3) {
				insrt(val, 24, 8, 0x10);
			}
		}
		if (rnd() & 1) {
			orig.fd_state_begin_patch_c &= 0xffff0000;
			orig.fd_state_begin_patch_c ^= 1 << (rnd() & 0x1f);
			orig.fd_state_begin_patch_c ^= 1 << (rnd() & 0x1f);
		}
		if (rnd() & 1) {
			if (rnd() & 1)
				orig.fe3d_shadow_begin_patch_c &= 0xffff0000;
			if (rnd() & 1)
				insrt(orig.fe3d_shadow_begin_patch_c, 21, 5, rnd() & 1);
			if (rnd() & 1)
				insrt(orig.fe3d_shadow_begin_patch_c, 26, 5, rnd() & 1);
			if (rnd() & 1) {
				orig.fe3d_shadow_begin_patch_c ^= 1 << (rnd() % 31);
				if (rnd() & 1)
					orig.fe3d_shadow_begin_patch_c ^= 1 << (rnd() % 31);
			}
		}
		if (rnd() & 1) {
			if (rnd() & 7)
				insrt(orig.fd_state_unk1c, 0, 5, rnd() & 1);
			if (rnd() & 7)
				insrt(orig.fd_state_unk20, 27, 5, rnd() & 1);
		}
		adjust_orig_launch(&orig, rnd);
	}
	bool is_valid_val() override {
		if (chipset.card_type != 0x20)
			return true;
		int unk0 = extr(val, 0, 3);
		int unk3 = extr(val, 3, 3);
		int unk6 = extr(val, 6, 4);
		if (unk0 == 4 || unk0 == 7)
			return false;
		if (unk3 == 4 || unk3 == 7)
			return false;
		if (!unk6)
			return false;
		if (extr(val, 17, 7))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (chipset.card_type != 0x20) {
			nv04_pgraph_missing_hw(&exp);
			return;
		}
		state_check_launch(&exp);
		int unk0 = extr(val, 0, 3);
		int unk3 = extr(val, 3, 3);
		int unk6 = extr(val, 6, 4);
		int unk10 = extr(val, 10, 4);
		int fe3d_unk_a = 2 + unk6;
		bool empty = true;
		if (extr(exp.fe3d_shadow_begin_patch_c, 0, 8))
			empty = false;
		if (extr(exp.fe3d_shadow_begin_patch_c, 8, 8))
			empty = false;
		bool empty_a;
		if ((unk0 == 1 || unk0 == 5 || unk0 == 3)) {
			empty_a = extr(exp.fe3d_shadow_begin_patch_c, 26, 5) == 1;
		} else {
			empty_a = extr(exp.fe3d_shadow_begin_patch_c, 26, 5) == 0;
		}
		bool empty_b;
		if ((unk3 == 1 || unk3 == 5 || unk3 == 3)) {
			empty_b = extr(exp.fe3d_shadow_begin_patch_c, 21, 5) == 1;
		} else {
			empty_b = extr(exp.fe3d_shadow_begin_patch_c, 21, 5) == 0;
		}
		if (!empty_a && !empty_b)
			empty = false;
		if (unk10)
			fe3d_unk_a += 2 + unk10;
		if (extr(exp.debug_d, 3, 1) && !extr(exp.debug_fd_check_skip, 3, 1)) {
			if (extr(exp.fe3d_misc, 4, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (!extr(exp.fe3d_misc, 5, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (!extr(exp.fe3d_misc, 6, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (!extr(exp.fe3d_misc, 7, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 24, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(val, 24, 8) < 2)
				nv04_pgraph_blowup(&exp, 0x80000);
			unsigned max = 0x3c - (fe3d_unk_a & 0x1f);
			if (extr(val, 24, 8) > max)
				nv04_pgraph_blowup(&exp, 0x80000);
			if (!extr(val, 16, 1) && extr(exp.fe3d_shadow_begin_patch_c, 0, 16) == 0) {
				unsigned need_a = 0;
				unsigned need_b = 0;
				if (unk0 == 0) {
					if (unk3 == 2 || unk3 == 6) {
						// OK
					} else if (unk3 == 1 || unk3 == 5) {
						need_b = 1;
					} else if (unk3 == 3) {
						need_a = 1;
						need_b = 2;
					} else {
						need_a = 1;
						need_b = 1;
					}
				} else if (unk3 == 0) {
					if (unk0 == 2 || unk0 == 6) {
						// OK
					} else if (unk0 == 1 || unk0 == 5) {
						need_a = 1;
					} else if (unk0 == 3) {
						need_a = 2;
						need_b = 1;
					} else {
						need_a = 1;
						need_b = 1;
					}
				} else {
					if (unk0 == 1 || unk0 == 3 || unk0 == 5) {
						need_a = 2;
					} else {
						need_a = 1;
					}
					if (unk3 == 1 || unk3 == 3 || unk3 == 5) {
						need_b = 2;
					} else {
						need_b = 1;
					}
				}
				if (extr(exp.fe3d_shadow_begin_patch_c, 26, 5) < need_a)
					nv04_pgraph_blowup(&exp, 0x80000);
				if (extr(exp.fe3d_shadow_begin_patch_c, 21, 5) < need_b)
					nv04_pgraph_blowup(&exp, 0x80000);
			}
		}
		if (!exp.nsource) {
			insrt(exp.fe3d_misc, 4, 1, 1);
			insrt(exp.fe3d_misc, 8, 4, 0);
			if (empty)
				insrt(exp.fe3d_misc, 16, 1, 1);
			insrt(exp.fe3d_misc, 17, 3, 0);
			insrt(exp.fe3d_misc, 23, 1, 0);
			insrt(exp.fe3d_misc, 24, 1, 0);
			insrt(exp.fe3d_state_curve, 12, 6, fe3d_unk_a);
			insrt(exp.fe3d_state_curve, 18, 8, extr(val, 24, 8));
			insrt(exp.fe3d_state_swatch, 0, 24, 0);
			insrt(exp.fe3d_state_transition, 0, 3, 0);
			int things = 0;
			if (unk0 == 3 || unk0 == 7)
				things += 2;
			else if (unk0 != 0 && unk0 != 4)
				things += 1;
			if (unk3 == 3 || unk3 == 7)
				things += 2;
			else if (unk3 != 0 && unk3 != 4)
				things += 1;
			insrt(exp.fe3d_state_transition, 4, 3, things);
			insrt(exp.fe3d_shadow_begin_patch_d, 0, 3, unk0);
			insrt(exp.fe3d_shadow_begin_patch_d, 3, 3, unk3);
			insrt(exp.fe3d_shadow_begin_patch_d, 14, 3, extr(val, 14, 3));
			pgraph_fd_cmd(&exp, 0x2814, val);
		} else {
			insrt(exp.fe3d_misc, 24, 1, 1);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFdEndPatch : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (chipset.card_type != 0x20)
			return;
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_idx(&orig);
	}
	bool is_valid_val() override {
		if (chipset.card_type != 0x20)
			return true;
		return val == 0;
	}
	void emulate_mthd() override {
		if (chipset.card_type != 0x20) {
			nv04_pgraph_missing_hw(&exp);
			return;
		}
		bool bad = false;
		if (!extr(exp.fe3d_misc, 4, 1))
			bad = true;
		if (!extr(exp.fe3d_misc, 16, 1))
			bad = true;
		if (extr(exp.fe3d_state_transition, 0, 3) != extr(exp.fe3d_state_transition, 4, 3))
			bad = true;
		if (!exp.nsource) {
			insrt(exp.fe3d_misc, 4, 16, 0);
			insrt(exp.fe3d_misc, 23, 1, 0);
			insrt(exp.fe3d_misc, 24, 1, 0);
			pgraph_fd_cmd(&exp, 0x1800, val);
		} else {
			insrt(exp.fe3d_misc, 24, 1, 0);
		}
		if (extr(exp.debug_d, 3, 1) && !extr(exp.debug_fd_check_skip, 4, 1)) {
			if (bad)
				nv04_pgraph_blowup(&exp, 0x80000);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFdCurveData : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (chipset.card_type != 0x20)
			return;
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		if (chipset.card_type != 0x20) {
			nv04_pgraph_missing_hw(&exp);
			return;
		}
		if (extr(exp.debug_d, 3, 1) && !extr(exp.debug_fd_check_skip, 7, 1)) {
			if (extr(exp.fe3d_misc, 8, 1) && extr(exp.fe3d_misc, 9, 2) == 0)
				nv04_pgraph_blowup(&exp, 0x80000);
			if (!extr(exp.fe3d_misc, 15, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 24, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
		}
		if (!exp.nsource) {
			insrt(exp.fe3d_misc, 24, 1, 0);
			pgraph_fd_cmd(&exp, 0x2000 + idx * 4, val);
			if (idx == 3) {
				int ctr = extr(exp.fe3d_state_curve, 0, 10);
				ctr += 4;
				insrt(exp.fe3d_state_curve, 0, 10, ctr);
			}
		} else {
			insrt(exp.fe3d_misc, 24, 1, 1);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFdBeginTransitionA : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (chipset.card_type != 0x20)
			return;
		adjust_orig_idx(&orig);
	}
	bool is_valid_val() override {
		if (chipset.card_type != 0x20)
			return true;
		if (extr(val, 0, 4) == 0)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (chipset.card_type != 0x20) {
			nv04_pgraph_missing_hw(&exp);
			return;
		}
		if (extr(exp.debug_d, 3, 1) && !extr(exp.debug_fd_check_skip, 8, 1)) {
			if (!extr(exp.fe3d_misc, 4, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 12, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 15, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (!extr(exp.fe3d_misc, 16, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 24, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
		}
		if (!exp.nsource) {
			insrt(exp.fe3d_misc, 24, 1, 0);
			insrt(exp.fe3d_misc, 12, 1, 1);
			exp.fe3d_shadow_begin_transition_a = val;
			int ctr = 0;
			for (int i = 1; i < 8; i++)
				if (extr(val, i * 4, 4))
					ctr++;
			insrt(exp.fe3d_state_transition, 24, 4, ctr);
			pgraph_fd_cmd(&exp, 0x2800, val);
		} else {
			insrt(exp.fe3d_misc, 24, 1, 1);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFdBeginTransitionB : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (chipset.card_type != 0x20)
			return;
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		if (chipset.card_type != 0x20) {
			nv04_pgraph_missing_hw(&exp);
			return;
		}
		if (extr(exp.debug_d, 3, 1) && !extr(exp.debug_fd_check_skip, 9, 1)) {
			if (!extr(exp.fe3d_misc, 4, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 13, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 15, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (!extr(exp.fe3d_misc, 16, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
			if (extr(exp.fe3d_misc, 24, 1))
				nv04_pgraph_blowup(&exp, 0x80000);
		}
		if (!exp.nsource) {
			insrt(exp.fe3d_misc, 24, 1, 0);
			insrt(exp.fe3d_misc, 13, 1, 1);
			exp.fe3d_shadow_begin_transition_b = val;
			int ctr = 0;
			for (int i = 0; i < 8; i++)
				if (extr(val, i * 4, 4))
					ctr++;
			insrt(exp.fe3d_state_transition, 28, 4, ctr);
			pgraph_fd_cmd(&exp, 0x2804, val);
		} else {
			insrt(exp.fe3d_misc, 24, 1, 1);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLineStippleEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_line_stipple, 1, 1, val);
			pgraph_bundle(&exp, BUNDLE_LINE_STIPPLE, 0, exp.bundle_line_stipple, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLineStipplePattern : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff00ff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0xffff00ff);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_line_stipple, 8, 8, extr(val, 0, 8));
			insrt(exp.bundle_line_stipple, 16, 16, extr(val, 16, 16));
			pgraph_bundle(&exp, BUNDLE_LINE_STIPPLE, 0, exp.bundle_line_stipple, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineTxcCylwrap : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_txc_cylwrap = val;
			pgraph_bundle(&exp, BUNDLE_TXC_CYLWRAP, 0, exp.bundle_txc_cylwrap, true);
			if (chipset.card_type < 0x40) {
				for (int i = 0; i < 8; i++) {
					insrt(exp.xf_mode_t[i >> 1], (i & 1) * 16 + 2, 1, extr(exp.bundle_config_b, 6, 1) && !extr(val, i * 4, 4));
				}
				pgraph_flush_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineTxcEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
		if (rnd() & 1) {
			val &= 0xff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return !(val & ~0xff);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_txc_enable = val & 0xff;
			pgraph_bundle(&exp, BUNDLE_TXC_ENABLE, 0, exp.bundle_txc_enable, true);
			if (chipset.card_type < 0x40) {
				for (int i = 0; i < 8; i++) {
					insrt(exp.xf_mode_t[i >> 1], (i & 1) * 16, 1, extr(val, i, 1));
				}
				pgraph_flush_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineRtEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0x1f;
			if (rnd() & 1) {
				val &= 3;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE) {
			return !(val & ~0x3);
		} else {
			if (val & ~0x1f)
				return false;
			if (val == 2)
				return true;
			for (int i = 1; i < 4; i++)
				if (extr(val, i, 1) && !extr(val, i - 1, 1))
					return false;
			return true;
		}
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE)
				insrt(exp.bundle_rt_enable, 0, 4, val & 3);
			else
				insrt(exp.bundle_rt_enable, 0, 8, val);
			pgraph_bundle(&exp, BUNDLE_RT_ENABLE, 0, exp.bundle_rt_enable, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDmaClipid : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t offset_mask = pgraph_offset_mask(&chipset) | 0x3f;
		uint32_t base = (pobj[2] & ~0xfff) | extr(pobj[0], 20, 12);
		uint32_t limit = pobj[1];
		uint32_t dcls = extr(pobj[0], 0, 12);
		int mem = extr(pobj[0], 16, 2);
		bool bad = true;
		if (dcls == 0x30 || dcls == 0x3d)
			bad = false;
		if (dcls == 3 && (cls == 0x38 || cls == 0x88))
			bad = false;
		if (extr(exp.debug_d, 23, 1) && bad) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		bool prot_err = false;
		if (extr(base, 0, 8))
			prot_err = true;
		if (mem >= 2)
			prot_err = true;
		if (dcls == 0x30) {
			prot_err = false;
		}
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		if (!exp.nsource) {
			exp.bundle_surf_limit_clipid = (limit & offset_mask) | (mem == 1) << 30 | (dcls != 0x30) << 31;
			exp.bundle_surf_base_clipid = base & offset_mask;
			pgraph_bundle(&exp, BUNDLE_SURF_BASE_CLIPID, 0, exp.bundle_surf_base_clipid, false);
			pgraph_bundle(&exp, BUNDLE_SURF_LIMIT_CLIPID, 0, exp.bundle_surf_limit_clipid, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinDmaZcull : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t offset_mask = pgraph_offset_mask(&chipset) | 0x3f;
		uint32_t base = (pobj[2] & ~0xfff) | extr(pobj[0], 20, 12);
		uint32_t limit = pobj[1];
		uint32_t dcls = extr(pobj[0], 0, 12);
		int mem = extr(pobj[0], 16, 2);
		bool bad = true;
		if (dcls == 0x30 || dcls == 0x3d)
			bad = false;
		if (dcls == 3 && (cls == 0x38 || cls == 0x88))
			bad = false;
		if (extr(exp.debug_d, 23, 1) && bad) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		bool prot_err = false;
		if (extr(base, 0, 8))
			prot_err = true;
		if (mem >= 2)
			prot_err = true;
		if (dcls == 0x30) {
			prot_err = false;
		}
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		if (!exp.nsource) {
			exp.bundle_surf_limit_zcull = (limit & offset_mask) | (mem == 1) << 30 | (dcls != 0x30) << 31;
			exp.bundle_surf_base_zcull = base & offset_mask;
			pgraph_bundle(&exp, BUNDLE_SURF_BASE_ZCULL, 0, exp.bundle_surf_base_zcull, false);
			pgraph_bundle(&exp, BUNDLE_SURF_LIMIT_ZCULL, 0, exp.bundle_surf_limit_zcull, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinSurfPitchClipid : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff80;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val = (rnd() & 1) << (rnd() & 0x1f);
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0xff80) && !!val;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.bundle_surf_pitch_clipid = val & 0xffff;
			pgraph_bundle(&exp, BUNDLE_SURF_PITCH_CLIPID, 0, exp.bundle_surf_pitch_clipid, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinSurfPitchZcull : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff80;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val = (rnd() & 1) << (rnd() & 0x1f);
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0xff80) && !!val;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			if (chipset.card_type < 0x40) {
				exp.bundle_surf_pitch_zcull = val & 0xffff;
				pgraph_bundle(&exp, BUNDLE_SURF_PITCH_ZCULL, 0, exp.bundle_surf_pitch_zcull, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinSurfOffsetClipid : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffffff00;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val = (rnd() & 1) << (rnd() & 0x1f);
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0xffffff00);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.bundle_surf_offset_clipid = val & 0x3fffffff;
			pgraph_bundle(&exp, BUNDLE_SURF_OFFSET_CLIPID, 0, exp.bundle_surf_offset_clipid, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinSurfOffsetZcull : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffffff00;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val = (rnd() & 1) << (rnd() & 0x1f);
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0xffffff00);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			if (chipset.card_type < 0x40) {
				exp.bundle_surf_offset_zcull = val & 0x3fffffff;
				pgraph_bundle(&exp, BUNDLE_SURF_OFFSET_ZCULL, 0, exp.bundle_surf_offset_zcull, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClipidEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (nv04_pgraph_is_kelvin_class(&exp))
			pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_z_config, 6, 1, val);
			pgraph_bundle(&exp, BUNDLE_Z_CONFIG, 0, exp.bundle_z_config, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClipidId : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0xff);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (nv04_pgraph_is_kelvin_class(&exp))
			pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_clipid_id = val & 0xff;
			pgraph_bundle(&exp, BUNDLE_CLIPID_ID, 0, exp.bundle_clipid_id, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnka0c : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_config_b, 31, 1, val);
			pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinPointSprite : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3ff0f;
			if (rnd() & 1)
				val &= 0xf07;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int mode = extr(val, 1, 2);
		if (mode == 3)
			return false;
		if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE) {
			return !(val & ~0xf07);
		} else {
			return !(val & ~0x3ff07);
		}
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_config_b, 1, 1, extr(val, 0, 1));
			insrt(exp.bundle_config_b, 3, 2, extr(val, 1, 2));
			insrt(exp.bundle_config_b, 24, 4, extr(val, 8, 4));
			if (pgraph_3d_class(&exp) >= PGRAPH_3D_CURIE) {
				insrt(exp.bundle_config_b, 10, 6, extr(val, 12, 6));
			}
			pgraph_bundle(&exp, BUNDLE_CONFIG_B, 0, exp.bundle_config_b, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk1d88 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0x3);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			// Huh.
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexShaderUnk1e7c : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~1);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_tex_shader_misc, 31, 1, val);
			pgraph_bundle(&exp, BUNDLE_TEX_SHADER_MISC, 0, exp.bundle_tex_shader_misc, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk1dbc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 3;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_unk0b8 = val & 3;
			pgraph_bundle(&exp, BUNDLE_UNK0B8, 0, exp.bundle_unk0b8, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinUnk1dc0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf8f8f8f8;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0xf8f8f8f8);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_unk0b4[idx] = val & 0xf8f8f8f8;
			pgraph_bundle(&exp, BUNDLE_UNK0B4, idx, exp.bundle_unk0b4[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankinePsOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffffffc1;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
		// XXX: Model the prefetch thing.
		skip = true;
	}
	bool is_valid_val() override {
		if (val & ~0xffffffc3)
			return false;
		int target = extr(val, 0, 2);
		if (target == 0 || target == 3)
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.fe3d_misc, 30, 2, extr(val, 1, 1) ? 2 : 1);
			uint32_t rval = val & 0xffffffc0;
			insrt(rval, 0, 1, extr(val, 1, 1));
			exp.bundle_ps_offset = rval;
			pgraph_bundle(&exp, BUNDLE_PS_OFFSET, idx, exp.bundle_ps_offset, true);
			// XXX Prefetch bundles...
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankinePsControl : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f0095ff;
			if (rnd() & 1)
				val &= 0x1ff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int unk4 = extr(val, 4, 2);
		if (unk4 == 3)
			return false;
		if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE) {
			if (val & ~0x000001ff)
				return false;
		} else {
			if (val & ~0x3f0095fe)
				return false;
			int regs = extr(val, 24, 6);
			if (regs < 2)
				return false;
			if (regs > 0x30)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type < 0x40) {
				insrt(exp.bundle_ps_control, 0, 3, extr(val, 0, 3));
			} else {
				if (pgraph_3d_class(&exp) < PGRAPH_3D_CURIE) {
					int unk0 = extr(val, 0, 3);
					int rv = 0;
					if (unk0 == 0)
						rv = 2;
					else if (unk0 == 1)
						rv = 4;
					else if (unk0 == 2)
						rv = 6;
					else if (unk0 == 3)
						rv = 8;
					else if (unk0 == 4)
						rv = 0xc;
					else if (unk0 == 5)
						rv = 0x10;
					else if (unk0 == 6)
						rv = 0x18;
					else if (unk0 == 7)
						rv = 0x20;
					insrt(exp.bundle_ps_control, 19, 8, rv);
				} else {
					insrt(exp.bundle_ps_control, 1, 2, extr(val, 1, 2));
					insrt(exp.bundle_ps_control, 9, 1, extr(val, 10, 1));
					insrt(exp.bundle_ps_control, 11, 1, extr(val, 12, 1));
					insrt(exp.bundle_ps_control, 13, 1, extr(val, 15, 1));
					insrt(exp.bundle_ps_control, 19, 8, extr(val, 24, 8));
				}
			}
			insrt(exp.bundle_ps_control, 3, 5, extr(val, 3, 5));
			insrt(exp.bundle_ps_control, 16, 1, extr(val, 8, 1));
			pgraph_bundle(&exp, BUNDLE_PS_CONTROL, idx, exp.bundle_ps_control, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineViewportOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (extrs(val, 0, 16) != extrs(val, 0, 13))
			return false;
		if (extrs(val, 16, 16) != extrs(val, 16, 13))
			return false;
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			exp.bundle_viewport_offset = val & 0x1fff1fff;
			pgraph_bundle(&exp, BUNDLE_VIEWPORT_OFFSET, idx, exp.bundle_viewport_offset, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineUnka08 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		nv04_pgraph_missing_hw(&exp);
		if (chipset.card_type < 0x40) {
			pgraph_kelvin_check_err19(&exp);
			pgraph_kelvin_check_err18(&exp);
			if (!exp.nsource) {
				pgraph_bundle(&exp, BUNDLE_UNK1F7, idx, val, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineUnka40 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x00030117;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0x00030117);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_unk0af, 0, 3, extr(val, 0, 3));
			insrt(exp.bundle_unk0af, 3, 1, extr(val, 4, 1));
			insrt(exp.bundle_unk0af, 4, 1, extr(val, 8, 1));
			insrt(exp.bundle_unk0af, 11, 2, extr(val, 16, 2));
			pgraph_bundle(&exp, BUNDLE_UNK0AF, 0, exp.bundle_unk0af, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineClipPlaneEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x00333333;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		for (int i = 0; i < 6; i++)
			if (extr(val, i*4, 2) == 3)
				return false;
		return !(val & ~0x00333333);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		uint32_t err = 0;
		if (pgraph_in_begin_end(&exp))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!exp.nsource) {
				exp.bundle_clip_plane_enable = val & 0x333333;
				pgraph_bundle(&exp, BUNDLE_CLIP_PLANE_ENABLE, 0, exp.bundle_clip_plane_enable, true);
				for (int i = 0; i < 6; i++)
					insrt(exp.xf_mode_c, i, 1, extr(val, i*4, 2) != 0);
				pgraph_flush_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankinePrimitiveRestartEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~1);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_primitive_restart_enable = val & 1;
			pgraph_bundle(&exp, BUNDLE_PRIMITIVE_RESTART_ENABLE, 0, val & 1, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankinePrimitiveRestartIndex : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_primitive_restart_index = val;
			pgraph_bundle(&exp, BUNDLE_PRIMITIVE_RESTART_INDEX, 0, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineWindowConfig : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x11fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0x11fff);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_window_config, 0, 13, extr(val, 0, 13));
			insrt(exp.bundle_window_config, 13, 1, extr(val, 16, 1));
			pgraph_bundle(&exp, BUNDLE_WINDOW_CONFIG, 0, exp.bundle_window_config, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineXfUnk1f80 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_xf(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err19(&exp);
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			if (chipset.card_type < 0x40) {
				pgraph_ld_xfunk8(&exp, idx << 2, val);
			} else {
				if (idx == 8) {
					insrt(exp.bundle_xf_d, 0, 16, val);
					pgraph_bundle(&exp, BUNDLE_XF_D, 0, exp.bundle_xf_d, true);
				}
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineIdxbufOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_idx(&orig);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			pgraph_set_idxbuf_offset(&exp, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineIdxbufFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x11;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~0x11);
	}
	void emulate_mthd() override {
		if (!exp.nsource) {
			pgraph_set_idxbuf_format(&exp, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineDepthBoundsEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & ~1);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			insrt(exp.bundle_z_config, 5, 1, val);
			pgraph_bundle(&exp, BUNDLE_Z_CONFIG, 0, exp.bundle_z_config, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineDepthBoundsMin : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_depth_bounds_min = val;
			pgraph_bundle(&exp, BUNDLE_DEPTH_BOUNDS_MIN, 0, exp.bundle_depth_bounds_min, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRankineDepthBoundsMax : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		pgraph_kelvin_check_err18(&exp);
		if (!exp.nsource) {
			exp.bundle_depth_bounds_max = val;
			pgraph_bundle(&exp, BUNDLE_DEPTH_BOUNDS_MAX, 0, exp.bundle_depth_bounds_max, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

std::vector<SingleMthdTest *> EmuCelsius::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", 2, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", 1, cls, 0x110),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 3, cls, 0x180),
		new MthdKelvinDmaTex(opt, rnd(), "dma_tex_a", 4, cls, 0x184, 0),
		new MthdKelvinDmaTex(opt, rnd(), "dma_tex_b", 5, cls, 0x188, 1),
		new MthdKelvinDmaVtx(opt, rnd(), "dma_vtx", 6, cls, 0x18c, 0),
		new MthdKelvinDmaState(opt, rnd(), "dma_state", 7, cls, 0x190),
		new MthdDmaSurf(opt, rnd(), "dma_surf_color", 8, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", 9, cls, 0x198, 3, SURF_NV10),
		new MthdKelvinClip(opt, rnd(), "clip_h", 10, cls, 0x200, 3, 0),
		new MthdKelvinClip(opt, rnd(), "clip_v", 10, cls, 0x204, 3, 1),
		new MthdSurf3DFormat(opt, rnd(), "surf_format", 11, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", 12, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_offset", 13, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", 14, cls, 0x214, 3, SURF_NV10),
		new MthdKelvinTexOffset(opt, rnd(), "tex_offset", 15, cls, 0x218, 2),
		new MthdEmuCelsiusTexFormat(opt, rnd(), "tex_format", 16, cls, 0x220, 2),
		new MthdKelvinTexControl(opt, rnd(), "tex_control", 17, cls, 0x228, 2),
		new MthdKelvinTexPitch(opt, rnd(), "tex_pitch", 18, cls, 0x230, 2),
		new MthdEmuCelsiusTexUnk238(opt, rnd(), "tex_unk238", 19, cls, 0x238, 2),
		new MthdKelvinTexRect(opt, rnd(), "tex_rect", 20, cls, 0x240, 2),
		new MthdEmuCelsiusTexFilter(opt, rnd(), "tex_filter", 21, cls, 0x248, 2),
		new MthdKelvinTexPalette(opt, rnd(), "tex_palette", 22, cls, 0x250, 2),
		new MthdKelvinRcInAlpha(opt, rnd(), "rc_in_alpha", 23, cls, 0x260, 2),
		new MthdKelvinRcInColor(opt, rnd(), "rc_in_color", 24, cls, 0x268, 2),
		new MthdEmuCelsiusRcFactor(opt, rnd(), "rc_factor", 25, cls, 0x270, 2),
		new MthdEmuCelsiusRcOutAlpha(opt, rnd(), "rc_out_alpha", 26, cls, 0x278, 2),
		new MthdEmuCelsiusRcOutColor(opt, rnd(), "rc_out_color", 27, cls, 0x280, 2),
		new MthdKelvinRcFinalA(opt, rnd(), "rc_final_a", 28, cls, 0x288),
		new MthdKelvinRcFinalB(opt, rnd(), "rc_final_b", 29, cls, 0x28c),
		new MthdEmuCelsiusConfig(opt, rnd(), "config", 30, cls, 0x290),
		new MthdEmuCelsiusLightModel(opt, rnd(), "light_model", 31, cls, 0x294),
		new MthdEmuCelsiusLightMaterial(opt, rnd(), "light_material", -1, cls, 0x298),
		new MthdEmuCelsiusFogMode(opt, rnd(), "fog_mode", -1, cls, 0x29c),
		new MthdEmuCelsiusFogCoord(opt, rnd(), "fog_coord", -1, cls, 0x2a0),
		new MthdKelvinFogEnable(opt, rnd(), "fog_enable", -1, cls, 0x2a4),
		new MthdKelvinFogColor(opt, rnd(), "fog_color", -1, cls, 0x2a8),
		new MthdKelvinTexColorKey(opt, rnd(), "tex_color_key", -1, cls, 0x2ac, 2),
		new MthdKelvinClipRectMode(opt, rnd(), "clip_rect_mode", -1, cls, 0x2b4),
		new MthdKelvinClipRect(opt, rnd(), "clip_rect_horiz", -1, cls, 0x2c0, 8, 4, 0),
		new MthdKelvinClipRect(opt, rnd(), "clip_rect_vert", -1, cls, 0x2e0, 8, 4, 1),
		new MthdKelvinAlphaFuncEnable(opt, rnd(), "alpha_func_enable", -1, cls, 0x300),
		new MthdKelvinBlendFuncEnable(opt, rnd(), "blend_func_enable", -1, cls, 0x304),
		new MthdKelvinCullFaceEnable(opt, rnd(), "cull_face_enable", -1, cls, 0x308),
		new MthdKelvinDepthTestEnable(opt, rnd(), "depth_test_enable", -1, cls, 0x30c),
		new MthdKelvinDitherEnable(opt, rnd(), "dither_enable", -1, cls, 0x310),
		new MthdKelvinLightingEnable(opt, rnd(), "lighting_enable", -1, cls, 0x314),
		new MthdKelvinPointParamsEnable(opt, rnd(), "point_params_enable", -1, cls, 0x318),
		new MthdKelvinPointSmoothEnable(opt, rnd(), "point_smooth_enable", -1, cls, 0x31c),
		new MthdKelvinLineSmoothEnable(opt, rnd(), "line_smooth_enable", -1, cls, 0x320),
		new MthdKelvinPolygonSmoothEnable(opt, rnd(), "polygon_smooth_enable", -1, cls, 0x324),
		new MthdEmuCelsiusWeightEnable(opt, rnd(), "weight_enable", -1, cls, 0x328),
		new MthdKelvinStencilEnable(opt, rnd(), "stencil_enable", -1, cls, 0x32c, 0),
		new MthdKelvinPolygonOffsetPointEnable(opt, rnd(), "polygon_offset_point_enable", -1, cls, 0x330),
		new MthdKelvinPolygonOffsetLineEnable(opt, rnd(), "polygon_offset_line_enable", -1, cls, 0x334),
		new MthdKelvinPolygonOffsetFillEnable(opt, rnd(), "polygon_offset_fill_enable", -1, cls, 0x338),
		new MthdKelvinAlphaFuncFunc(opt, rnd(), "alpha_func_func", -1, cls, 0x33c),
		new MthdKelvinAlphaFuncRef(opt, rnd(), "alpha_func_ref", -1, cls, 0x340),
		new MthdKelvinBlendFunc(opt, rnd(), "blend_func_src", -1, cls, 0x344, 0),
		new MthdKelvinBlendFunc(opt, rnd(), "blend_func_dst", -1, cls, 0x348, 1),
		new MthdKelvinBlendColor(opt, rnd(), "blend_color", -1, cls, 0x34c),
		new MthdKelvinBlendEquation(opt, rnd(), "blend_equation", -1, cls, 0x350),
		new MthdKelvinDepthFunc(opt, rnd(), "depth_func", -1, cls, 0x354),
		new MthdKelvinColorMask(opt, rnd(), "color_mask", -1, cls, 0x358),
		new MthdKelvinDepthWriteEnable(opt, rnd(), "depth_write_enable", -1, cls, 0x35c),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_mask", -1, cls, 0x360, 0, 2),
		new MthdKelvinStencilFunc(opt, rnd(), "stencil_func", -1, cls, 0x364, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_func_ref", -1, cls, 0x368, 0, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_func_mask", -1, cls, 0x36c, 0, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_fail", -1, cls, 0x370, 0, 0),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_zfail", -1, cls, 0x374, 0, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_zpass", -1, cls, 0x378, 0, 2),
		new MthdKelvinShadeMode(opt, rnd(), "shade_mode", -1, cls, 0x37c),
		new MthdKelvinLineWidth(opt, rnd(), "line_width", -1, cls, 0x380),
		new MthdKelvinPolygonOffsetFactor(opt, rnd(), "polygon_offset_factor", -1, cls, 0x384),
		new MthdKelvinPolygonOffsetUnits(opt, rnd(), "polygon_offset_units", -1, cls, 0x388),
		new MthdKelvinPolygonMode(opt, rnd(), "polygon_mode_front", -1, cls, 0x38c, 0),
		new MthdKelvinPolygonMode(opt, rnd(), "polygon_mode_back", -1, cls, 0x390, 1),
		new MthdKelvinDepthRangeNear(opt, rnd(), "depth_range_near", -1, cls, 0x394),
		new MthdKelvinDepthRangeFar(opt, rnd(), "depth_range_far", -1, cls, 0x398),
		new MthdKelvinCullFace(opt, rnd(), "cull_face", -1, cls, 0x39c),
		new MthdKelvinFrontFace(opt, rnd(), "front_face", -1, cls, 0x3a0),
		new MthdKelvinNormalizeEnable(opt, rnd(), "normalize_enable", -1, cls, 0x3a4),
		new MthdKelvinMaterialFactorA(opt, rnd(), "material_factor_a", -1, cls, 0x3b4, 0),
		new MthdKelvinSpecularEnable(opt, rnd(), "specular_enable", -1, cls, 0x3b8),
		new MthdKelvinLightEnable(opt, rnd(), "light_enable", -1, cls, 0x3bc),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_s", -1, cls, 0x3c0, 2, 0x10, 0),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_t", -1, cls, 0x3c4, 2, 0x10, 1),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_r", -1, cls, 0x3c8, 2, 0x10, 2),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_q", -1, cls, 0x3cc, 2, 0x10, 3),
		new MthdKelvinTexMatrixEnable(opt, rnd(), "tex_matrix_enable", -1, cls, 0x3e0, 2),
		new MthdEmuCelsiusTlMode(opt, rnd(), "tl_mode", -1, cls, 0x3e8),
		new MthdKelvinPointSize(opt, rnd(), "point_size", -1, cls, 0x3ec),
		new MthdKelvinUnk3f0(opt, rnd(), "unk3f0", -1, cls, 0x3f0),
		new MthdKelvinFlatshadeFirst(opt, rnd(), "flatshade_first", -1, cls, 0x3f4),
		new MthdEmuCelsiusOldUnk3f8(opt, rnd(), "old_unk3f8", -1, cls, 0x3f8),
		new MthdKelvinMvMatrix(opt, rnd(), "matrix_mv0", -1, cls, 0x400, 0),
		new MthdKelvinMvMatrix(opt, rnd(), "matrix_mv1", -1, cls, 0x440, 1),
		new MthdKelvinImvMatrix(opt, rnd(), "matrix_imv0", -1, cls, 0x480, 0),
		new MthdKelvinImvMatrix(opt, rnd(), "matrix_imv1", -1, cls, 0x4c0, 1),
		new MthdKelvinProjMatrix(opt, rnd(), "matrix_proj", -1, cls, 0x500),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx0", -1, cls, 0x540, 0),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx1", -1, cls, 0x580, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_0_s_plane", -1, cls, 0x600, 0, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_0_t_plane", -1, cls, 0x610, 0, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_0_r_plane", -1, cls, 0x620, 0, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_0_q_plane", -1, cls, 0x630, 0, 3),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_1_s_plane", -1, cls, 0x640, 1, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_1_t_plane", -1, cls, 0x650, 1, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_1_r_plane", -1, cls, 0x660, 1, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_1_q_plane", -1, cls, 0x670, 1, 3),
		new MthdKelvinFogCoeff(opt, rnd(), "fog_coeff", -1, cls, 0x680, 3),
		new MthdKelvinFogPlane(opt, rnd(), "fog_plane", -1, cls, 0x68c),
		new MthdKelvinMaterialShininessA(opt, rnd(), "material_shininess_a", -1, cls, 0x6a0, 0),
		new MthdKelvinMaterialShininessB(opt, rnd(), "material_shininess_b", -1, cls, 0x6a4, 0),
		new MthdKelvinMaterialShininessC(opt, rnd(), "material_shininess_c", -1, cls, 0x6a8, 0),
		new MthdKelvinMaterialShininessD(opt, rnd(), "material_shininess_d", -1, cls, 0x6ac, 0),
		new MthdKelvinMaterialShininessE(opt, rnd(), "material_shininess_e", -1, cls, 0x6b0, 0),
		new MthdKelvinMaterialShininessF(opt, rnd(), "material_shininess_f", -1, cls, 0x6b4, 0),
		new MthdEmuCelsiusLightModelAmbient(opt, rnd(), "light_model_ambient_color", -1, cls, 0x6c4, 3),
		new MthdKelvinViewportTranslate(opt, rnd(), "viewport_translate", -1, cls, 0x6e8),
		new MthdKelvinPointParamsA(opt, rnd(), "point_params_a", -1, cls, 0x6f8),
		new MthdKelvinPointParamsB(opt, rnd(), "point_params_b", -1, cls, 0x704),
		new MthdKelvinPointParamsC(opt, rnd(), "point_params_c", -1, cls, 0x710),
		new MthdKelvinPointParamsD(opt, rnd(), "point_params_d", -1, cls, 0x714),
		new MthdKelvinLightEyePosition(opt, rnd(), "light_eye_position", -1, cls, 0x718),
		new MthdKelvinLightColor(opt, rnd(), "light_0_ambient_color", -1, cls, 0x800, 0, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_0_diffuse_color", -1, cls, 0x80c, 0, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_0_specular_color", -1, cls, 0x818, 0, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_0_local_range", -1, cls, 0x824, 0),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_0_half_vector", -1, cls, 0x828, 0),
		new MthdKelvinLightDirection(opt, rnd(), "light_0_direction", -1, cls, 0x834, 0),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_0_spot_cutoff_a", -1, cls, 0x840, 0),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_0_spot_cutoff_b", -1, cls, 0x844, 0),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_0_spot_cutoff_c", -1, cls, 0x848, 0),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_0_spot_direction", -1, cls, 0x84c, 0),
		new MthdKelvinLightPosition(opt, rnd(), "light_0_position", -1, cls, 0x85c, 0),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_0_attenuation", -1, cls, 0x868, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_1_ambient_color", -1, cls, 0x880, 1, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_1_diffuse_color", -1, cls, 0x88c, 1, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_1_specular_color", -1, cls, 0x898, 1, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_1_local_range", -1, cls, 0x8a4, 1),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_1_half_vector", -1, cls, 0x8a8, 1),
		new MthdKelvinLightDirection(opt, rnd(), "light_1_direction", -1, cls, 0x8b4, 1),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_1_spot_cutoff_a", -1, cls, 0x8c0, 1),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_1_spot_cutoff_b", -1, cls, 0x8c4, 1),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_1_spot_cutoff_c", -1, cls, 0x8c8, 1),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_1_spot_direction", -1, cls, 0x8cc, 1),
		new MthdKelvinLightPosition(opt, rnd(), "light_1_position", -1, cls, 0x8dc, 1),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_1_attenuation", -1, cls, 0x8e8, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_2_ambient_color", -1, cls, 0x900, 2, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_2_diffuse_color", -1, cls, 0x90c, 2, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_2_specular_color", -1, cls, 0x918, 2, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_2_local_range", -1, cls, 0x924, 2),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_2_half_vector", -1, cls, 0x928, 2),
		new MthdKelvinLightDirection(opt, rnd(), "light_2_direction", -1, cls, 0x934, 2),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_2_spot_cutoff_a", -1, cls, 0x940, 2),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_2_spot_cutoff_b", -1, cls, 0x944, 2),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_2_spot_cutoff_c", -1, cls, 0x948, 2),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_2_spot_direction", -1, cls, 0x94c, 2),
		new MthdKelvinLightPosition(opt, rnd(), "light_2_position", -1, cls, 0x95c, 2),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_2_attenuation", -1, cls, 0x968, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_3_ambient_color", -1, cls, 0x980, 3, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_3_diffuse_color", -1, cls, 0x98c, 3, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_3_specular_color", -1, cls, 0x998, 3, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_3_local_range", -1, cls, 0x9a4, 3),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_3_half_vector", -1, cls, 0x9a8, 3),
		new MthdKelvinLightDirection(opt, rnd(), "light_3_direction", -1, cls, 0x9b4, 3),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_3_spot_cutoff_a", -1, cls, 0x9c0, 3),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_3_spot_cutoff_b", -1, cls, 0x9c4, 3),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_3_spot_cutoff_c", -1, cls, 0x9c8, 3),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_3_spot_direction", -1, cls, 0x9cc, 3),
		new MthdKelvinLightPosition(opt, rnd(), "light_3_position", -1, cls, 0x9dc, 3),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_3_attenuation", -1, cls, 0x9e8, 3),
		new MthdKelvinLightColor(opt, rnd(), "light_4_ambient_color", -1, cls, 0xa00, 4, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_4_diffuse_color", -1, cls, 0xa0c, 4, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_4_specular_color", -1, cls, 0xa18, 4, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_4_local_range", -1, cls, 0xa24, 4),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_4_half_vector", -1, cls, 0xa28, 4),
		new MthdKelvinLightDirection(opt, rnd(), "light_4_direction", -1, cls, 0xa34, 4),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_4_spot_cutoff_a", -1, cls, 0xa40, 4),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_4_spot_cutoff_b", -1, cls, 0xa44, 4),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_4_spot_cutoff_c", -1, cls, 0xa48, 4),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_4_spot_direction", -1, cls, 0xa4c, 4),
		new MthdKelvinLightPosition(opt, rnd(), "light_4_position", -1, cls, 0xa5c, 4),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_4_attenuation", -1, cls, 0xa68, 4),
		new MthdKelvinLightColor(opt, rnd(), "light_5_ambient_color", -1, cls, 0xa80, 5, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_5_diffuse_color", -1, cls, 0xa8c, 5, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_5_specular_color", -1, cls, 0xa98, 5, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_5_local_range", -1, cls, 0xaa4, 5),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_5_half_vector", -1, cls, 0xaa8, 5),
		new MthdKelvinLightDirection(opt, rnd(), "light_5_direction", -1, cls, 0xab4, 5),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_5_spot_cutoff_a", -1, cls, 0xac0, 5),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_5_spot_cutoff_b", -1, cls, 0xac4, 5),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_5_spot_cutoff_c", -1, cls, 0xac8, 5),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_5_spot_direction", -1, cls, 0xacc, 5),
		new MthdKelvinLightPosition(opt, rnd(), "light_5_position", -1, cls, 0xadc, 5),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_5_attenuation", -1, cls, 0xae8, 5),
		new MthdKelvinLightColor(opt, rnd(), "light_6_ambient_color", -1, cls, 0xb00, 6, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_6_diffuse_color", -1, cls, 0xb0c, 6, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_6_specular_color", -1, cls, 0xb18, 6, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_6_local_range", -1, cls, 0xb24, 6),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_6_half_vector", -1, cls, 0xb28, 6),
		new MthdKelvinLightDirection(opt, rnd(), "light_6_direction", -1, cls, 0xb34, 6),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_6_spot_cutoff_a", -1, cls, 0xb40, 6),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_6_spot_cutoff_b", -1, cls, 0xb44, 6),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_6_spot_cutoff_c", -1, cls, 0xb48, 6),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_6_spot_direction", -1, cls, 0xb4c, 6),
		new MthdKelvinLightPosition(opt, rnd(), "light_6_position", -1, cls, 0xb5c, 6),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_6_attenuation", -1, cls, 0xb68, 6),
		new MthdKelvinLightColor(opt, rnd(), "light_7_ambient_color", -1, cls, 0xb80, 7, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_7_diffuse_color", -1, cls, 0xb8c, 7, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_7_specular_color", -1, cls, 0xb98, 7, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_7_local_range", -1, cls, 0xba4, 7),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_7_half_vector", -1, cls, 0xba8, 7),
		new MthdKelvinLightDirection(opt, rnd(), "light_7_direction", -1, cls, 0xbb4, 7),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_7_spot_cutoff_a", -1, cls, 0xbc0, 7),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_7_spot_cutoff_b", -1, cls, 0xbc4, 7),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_7_spot_cutoff_c", -1, cls, 0xbc8, 7),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_7_spot_direction", -1, cls, 0xbcc, 7),
		new MthdKelvinLightPosition(opt, rnd(), "light_7_position", -1, cls, 0xbdc, 7),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_7_attenuation", -1, cls, 0xbe8, 7),
		new UntestedMthd(opt, rnd(), "vtx_pos_3f", -1, cls, 0xc00, 3), // XXX
		new UntestedMthd(opt, rnd(), "vtx_pos_3s", -1, cls, 0xc10, 2), // XXX
		new UntestedMthd(opt, rnd(), "vtx_pos_4f", -1, cls, 0xc18, 4), // XXX
		new UntestedMthd(opt, rnd(), "vtx_pos_4s", -1, cls, 0xc28, 2), // XXX
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_nrm_3f", -1, cls, 0xc30, 3, 0x2),
		new MthdKelvinVtxAttrNShort(opt, rnd(), "vtx_nrm_3s", -1, cls, 0xc40, 3, 0x2),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_col0_4f", -1, cls, 0xc50, 4, 0x3),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_col0_3f", -1, cls, 0xc60, 3, 0x3),
		new MthdKelvinVtxAttrUByte(opt, rnd(), "vtx_col0_4ub", -1, cls, 0xc6c, 4, 0x3),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_col1_4f", -1, cls, 0xc70, 4, 0x4),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_col1_3f", -1, cls, 0xc80, 3, 0x4),
		new MthdKelvinVtxAttrUByte(opt, rnd(), "vtx_col1_4ub", -1, cls, 0xc8c, 4, 0x4),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc0_2f", -1, cls, 0xc90, 2, 0x9),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc0_2s", -1, cls, 0xc98, 2, 0x9),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc0_4f", -1, cls, 0xca0, 4, 0x9),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc0_4s", -1, cls, 0xcb0, 4, 0x9),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc1_2f", -1, cls, 0xcb8, 2, 0xa),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc1_2s", -1, cls, 0xcc0, 2, 0xa),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc1_4f", -1, cls, 0xcc8, 4, 0xa),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc1_4s", -1, cls, 0xcd8, 4, 0xa),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_fog_1f", -1, cls, 0xce0, 1, 0x5),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_wei_1f", -1, cls, 0xce4, 1, 0x1),
		new MthdKelvinEdgeFlag(opt, rnd(), "vtx_edge_flag", -1, cls, 0xcec),
		new MthdKelvinUnkcf0(opt, rnd(), "unkcf0", -1, cls, 0xcf0), // XXX
		new MthdKelvinUnkcf4(opt, rnd(), "unkcf4", -1, cls, 0xcf4), // XXX
		new MthdKelvinXfNop(opt, rnd(), "xf_nop", -1, cls, 0xcf8),
		new MthdKelvinXfSync(opt, rnd(), "xf_sync", -1, cls, 0xcfc),
		new MthdEmuCelsiusVtxbufOffset(opt, rnd(), "vtxbuf_offset", -1, cls, 0xd00, 8, 8),
		new MthdEmuCelsiusVtxbufFormat(opt, rnd(), "vtxbuf_format", -1, cls, 0xd04, 8, 8),
		new UntestedMthd(opt, rnd(), "dtaw_idx16.begin", -1, cls, 0xdfc), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx16.data", -1, cls, 0xe00, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx32.begin", -1, cls, 0x10fc), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx32.data", -1, cls, 0x1100, 0x40), // XXX
		new UntestedMthd(opt, rnd(), "draw_arrays.begin", -1, cls, 0x13fc), // XXX
		new UntestedMthd(opt, rnd(), "draw_arrays.data", -1, cls, 0x1400, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "draw_inline.begin", -1, cls, 0x17fc), // XXX
		new UntestedMthd(opt, rnd(), "draw_inline.data", -1, cls, 0x1800, 0x200), // XXX
	};
	if (cls == 0x56) {
		res.insert(res.end(), {
			new MthdEmuCelsiusMaterialFactorRgb(opt, rnd(), "material_factor_rgb", -1, cls, 0x3a8, 3),
			new MthdEmuCelsiusMaterialFactorRgbFree(opt, rnd(), "material_factor_rgb_free", -1, cls, 0x1628, 3),
		});
	} else {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "unk114", -1, cls, 0x114), // XXX
			new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
			new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
			new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
			new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
			new UntestedMthd(opt, rnd(), "flip_unk130", -1, cls, 0x130),
			new MthdEmuCelsiusMaterialFactorRgbFree(opt, rnd(), "material_factor_rgb", -1, cls, 0x3a8, 3),
			new MthdKelvinColorLogicOpEnable(opt, rnd(), "color_logic_op_enable", -1, cls, 0xd40),
			new MthdKelvinColorLogicOpOp(opt, rnd(), "color_logic_op_op", -1, cls, 0xd44),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Kelvin::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", -1, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", -1, cls, 0x110),
		new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
		new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
		new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
		new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
		new UntestedMthd(opt, rnd(), "flip_unk130", -1, cls, 0x130),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", -1, cls, 0x180),
		new MthdKelvinDmaTex(opt, rnd(), "dma_tex_a", -1, cls, 0x184, 0),
		new MthdKelvinDmaTex(opt, rnd(), "dma_tex_b", -1, cls, 0x188, 1),
		new MthdKelvinDmaState(opt, rnd(), "dma_state", -1, cls, 0x190),
		new MthdDmaSurf(opt, rnd(), "dma_surf_color", -1, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", -1, cls, 0x198, 3, SURF_NV10),
		new MthdKelvinDmaVtx(opt, rnd(), "dma_vtx_a", -1, cls, 0x19c, 0),
		new MthdKelvinDmaVtx(opt, rnd(), "dma_vtx_b", -1, cls, 0x1a0, 1),
		new MthdDmaGrobj(opt, rnd(), "dma_fence", -1, cls, 0x1a4, 0, DMA_W | DMA_FENCE),
		new MthdDmaGrobj(opt, rnd(), "dma_query", -1, cls, 0x1a8, 1, DMA_W),
		new MthdKelvinClip(opt, rnd(), "clip_h", -1, cls, 0x200, 3, 0),
		new MthdKelvinClip(opt, rnd(), "clip_v", -1, cls, 0x204, 3, 1),
		new MthdSurf3DFormat(opt, rnd(), "surf_format", -1, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", -1, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_offset", -1, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", -1, cls, 0x214, 3, SURF_NV10),
		new MthdKelvinRcInAlpha(opt, rnd(), "rc_in_alpha", -1, cls, 0x260, 8),
		new MthdKelvinRcFinalA(opt, rnd(), "rc_final_a", -1, cls, 0x288),
		new MthdKelvinRcFinalB(opt, rnd(), "rc_final_b", -1, cls, 0x28c),
		new MthdKelvinConfig(opt, rnd(), "config", -1, cls, 0x290),
		new MthdKelvinLightModel(opt, rnd(), "light_model", -1, cls, 0x294),
		new MthdKelvinLightMaterial(opt, rnd(), "light_material", -1, cls, 0x298),
		new MthdKelvinFogMode(opt, rnd(), "fog_mode", -1, cls, 0x29c),
		new MthdKelvinFogCoord(opt, rnd(), "fog_coord", -1, cls, 0x2a0),
		new MthdKelvinFogEnable(opt, rnd(), "fog_enable", -1, cls, 0x2a4),
		new MthdKelvinFogColor(opt, rnd(), "fog_color", -1, cls, 0x2a8),
		new MthdKelvinClipRectMode(opt, rnd(), "clip_rect_mode", -1, cls, 0x2b4),
		new MthdKelvinClipRect(opt, rnd(), "clip_rect_horiz", -1, cls, 0x2c0, 8, 4, 0),
		new MthdKelvinClipRect(opt, rnd(), "clip_rect_vert", -1, cls, 0x2e0, 8, 4, 1),
		new MthdKelvinAlphaFuncEnable(opt, rnd(), "alpha_func_enable", -1, cls, 0x300),
		new MthdKelvinBlendFuncEnable(opt, rnd(), "blend_func_enable", -1, cls, 0x304),
		new MthdKelvinCullFaceEnable(opt, rnd(), "cull_face_enable", -1, cls, 0x308),
		new MthdKelvinDepthTestEnable(opt, rnd(), "depth_test_enable", -1, cls, 0x30c),
		new MthdKelvinDitherEnable(opt, rnd(), "dither_enable", -1, cls, 0x310),
		new MthdKelvinLightingEnable(opt, rnd(), "lighting_enable", -1, cls, 0x314),
		new MthdKelvinPointParamsEnable(opt, rnd(), "point_params_enable", -1, cls, 0x318),
		new MthdKelvinLineSmoothEnable(opt, rnd(), "line_smooth_enable", -1, cls, 0x320),
		new MthdKelvinPolygonSmoothEnable(opt, rnd(), "polygon_smooth_enable", -1, cls, 0x324),
		new MthdKelvinWeightMode(opt, rnd(), "weight_mode", -1, cls, 0x328),
		new MthdKelvinStencilEnable(opt, rnd(), "stencil_enable", -1, cls, 0x32c, 0),
		new MthdKelvinPolygonOffsetPointEnable(opt, rnd(), "polygon_offset_point_enable", -1, cls, 0x330),
		new MthdKelvinPolygonOffsetLineEnable(opt, rnd(), "polygon_offset_line_enable", -1, cls, 0x334),
		new MthdKelvinPolygonOffsetFillEnable(opt, rnd(), "polygon_offset_fill_enable", -1, cls, 0x338),
		new MthdKelvinAlphaFuncFunc(opt, rnd(), "alpha_func_func", -1, cls, 0x33c),
		new MthdKelvinAlphaFuncRef(opt, rnd(), "alpha_func_ref", -1, cls, 0x340),
		new MthdKelvinBlendFunc(opt, rnd(), "blend_func_src", -1, cls, 0x344, 0),
		new MthdKelvinBlendFunc(opt, rnd(), "blend_func_dst", -1, cls, 0x348, 1),
		new MthdKelvinBlendColor(opt, rnd(), "blend_color", -1, cls, 0x34c),
		new MthdKelvinBlendEquation(opt, rnd(), "blend_equation", -1, cls, 0x350),
		new MthdKelvinDepthFunc(opt, rnd(), "depth_func", -1, cls, 0x354),
		new MthdKelvinColorMask(opt, rnd(), "color_mask", -1, cls, 0x358),
		new MthdKelvinDepthWriteEnable(opt, rnd(), "depth_write_enable", -1, cls, 0x35c),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_mask", -1, cls, 0x360, 0, 2),
		new MthdKelvinStencilFunc(opt, rnd(), "stencil_func", -1, cls, 0x364, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_func_ref", -1, cls, 0x368, 0, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_func_mask", -1, cls, 0x36c, 0, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_fail", -1, cls, 0x370, 0, 0),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_zfail", -1, cls, 0x374, 0, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_zpass", -1, cls, 0x378, 0, 2),
		new MthdKelvinShadeMode(opt, rnd(), "shade_mode", -1, cls, 0x37c),
		new MthdKelvinLineWidth(opt, rnd(), "line_width", -1, cls, 0x380),
		new MthdKelvinPolygonOffsetFactor(opt, rnd(), "polygon_offset_factor", -1, cls, 0x384),
		new MthdKelvinPolygonOffsetUnits(opt, rnd(), "polygon_offset_units", -1, cls, 0x388),
		new MthdKelvinPolygonMode(opt, rnd(), "polygon_mode_front", -1, cls, 0x38c, 0),
		new MthdKelvinPolygonMode(opt, rnd(), "polygon_mode_back", -1, cls, 0x390, 1),
		new MthdKelvinDepthRangeNear(opt, rnd(), "depth_range_near", -1, cls, 0x394),
		new MthdKelvinDepthRangeFar(opt, rnd(), "depth_range_far", -1, cls, 0x398),
		new MthdKelvinCullFace(opt, rnd(), "cull_face", -1, cls, 0x39c),
		new MthdKelvinFrontFace(opt, rnd(), "front_face", -1, cls, 0x3a0),
		new MthdKelvinNormalizeEnable(opt, rnd(), "normalize_enable", -1, cls, 0x3a4),
		new MthdKelvinMaterialFactorRgb(opt, rnd(), "material_factor_rgb", -1, cls, 0x3a8, 0),
		new MthdKelvinMaterialFactorA(opt, rnd(), "material_factor_a", -1, cls, 0x3b4, 0),
		new MthdKelvinSpecularEnable(opt, rnd(), "specular_enable", -1, cls, 0x3b8),
		new MthdKelvinLightEnable(opt, rnd(), "light_enable", -1, cls, 0x3bc),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_s", -1, cls, 0x3c0, 4, 0x10, 0),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_t", -1, cls, 0x3c4, 4, 0x10, 1),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_r", -1, cls, 0x3c8, 4, 0x10, 2),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_q", -1, cls, 0x3cc, 4, 0x10, 3),
		new MthdKelvinTexMatrixEnable(opt, rnd(), "tex_matrix_enable", -1, cls, 0x420, 4),
		new MthdKelvinPointSize(opt, rnd(), "point_size", -1, cls, 0x43c),
		new MthdKelvinUnk4Matrix(opt, rnd(), "matrix_unk4", -1, cls, 0x440),
		new MthdKelvinMvMatrix(opt, rnd(), "matrix_mv0", -1, cls, 0x480, 0),
		new MthdKelvinMvMatrix(opt, rnd(), "matrix_mv1", -1, cls, 0x4c0, 1),
		new MthdKelvinMvMatrix(opt, rnd(), "matrix_mv2", -1, cls, 0x500, 2),
		new MthdKelvinMvMatrix(opt, rnd(), "matrix_mv3", -1, cls, 0x540, 3),
		new MthdKelvinImvMatrix(opt, rnd(), "matrix_imv0", -1, cls, 0x580, 0),
		new MthdKelvinImvMatrix(opt, rnd(), "matrix_imv1", -1, cls, 0x5c0, 1),
		new MthdKelvinImvMatrix(opt, rnd(), "matrix_imv2", -1, cls, 0x600, 2),
		new MthdKelvinImvMatrix(opt, rnd(), "matrix_imv3", -1, cls, 0x640, 3),
		new MthdKelvinProjMatrix(opt, rnd(), "matrix_proj", -1, cls, 0x680),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx0", -1, cls, 0x6c0, 0),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx1", -1, cls, 0x700, 1),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx2", -1, cls, 0x740, 2),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx3", -1, cls, 0x780, 3),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_0_s_plane", -1, cls, 0x840, 0, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_0_t_plane", -1, cls, 0x850, 0, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_0_r_plane", -1, cls, 0x860, 0, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_0_q_plane", -1, cls, 0x870, 0, 3),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_1_s_plane", -1, cls, 0x880, 1, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_1_t_plane", -1, cls, 0x890, 1, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_1_r_plane", -1, cls, 0x8a0, 1, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_1_q_plane", -1, cls, 0x8b0, 1, 3),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_2_s_plane", -1, cls, 0x8c0, 2, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_2_t_plane", -1, cls, 0x8d0, 2, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_2_r_plane", -1, cls, 0x8e0, 2, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_2_q_plane", -1, cls, 0x8f0, 2, 3),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_3_s_plane", -1, cls, 0x900, 3, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_3_t_plane", -1, cls, 0x910, 3, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_3_r_plane", -1, cls, 0x920, 3, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_3_q_plane", -1, cls, 0x930, 3, 3),
		new MthdKelvinFogCoeff(opt, rnd(), "fog_coeff", -1, cls, 0x9c0, 3),
		new MthdKelvinTlUnk9cc(opt, rnd(), "tl_unk9cc", -1, cls, 0x9cc),
		new MthdKelvinFogPlane(opt, rnd(), "fog_plane", -1, cls, 0x9d0),
		new MthdKelvinMaterialShininessA(opt, rnd(), "material_shininess_a", -1, cls, 0x9e0, 0),
		new MthdKelvinMaterialShininessB(opt, rnd(), "material_shininess_b", -1, cls, 0x9e4, 0),
		new MthdKelvinMaterialShininessC(opt, rnd(), "material_shininess_c", -1, cls, 0x9e8, 0),
		new MthdKelvinMaterialShininessD(opt, rnd(), "material_shininess_d", -1, cls, 0x9ec, 0),
		new MthdKelvinMaterialShininessE(opt, rnd(), "material_shininess_e", -1, cls, 0x9f0, 0),
		new MthdKelvinMaterialShininessF(opt, rnd(), "material_shininess_f", -1, cls, 0x9f4, 0),
		new MthdKelvinUnk3f0(opt, rnd(), "unk3f0", -1, cls, 0x9f8),
		new MthdKelvinFlatshadeFirst(opt, rnd(), "flatshade_first", -1, cls, 0x9fc),
		new MthdKelvinLightModelAmbientColor(opt, rnd(), "light_model_ambient_color", -1, cls, 0xa10, 0),
		new MthdKelvinViewportTranslate(opt, rnd(), "viewport_translate", -1, cls, 0xa20),
		new MthdKelvinPointParamsA(opt, rnd(), "point_params_a", -1, cls, 0xa30),
		new MthdKelvinPointParamsB(opt, rnd(), "point_params_b", -1, cls, 0xa3c),
		new MthdKelvinPointParamsC(opt, rnd(), "point_params_c", -1, cls, 0xa48),
		new MthdKelvinPointParamsD(opt, rnd(), "point_params_d", -1, cls, 0xa4c),
		new MthdKelvinLightEyePosition(opt, rnd(), "light_eye_position", -1, cls, 0xa50),
		new MthdKelvinRcFactorA(opt, rnd(), "rc_factor_a", -1, cls, 0xa60, 8),
		new MthdKelvinRcFactorB(opt, rnd(), "rc_factor_b", -1, cls, 0xa80, 8),
		new MthdKelvinRcOutAlpha(opt, rnd(), "rc_out_alpha", -1, cls, 0xaa0, 8),
		new MthdKelvinRcInColor(opt, rnd(), "rc_in_color", -1, cls, 0xac0, 8),
		new MthdKelvinTexColorKey(opt, rnd(), "tex_color_key", -1, cls, 0xae0, 4),
		new MthdKelvinViewportScale(opt, rnd(), "viewport_scale", -1, cls, 0xaf0),
		new MthdKelvinTlProgramLoad(opt, rnd(), "tl_program_load", -1, cls, 0xb00, 0x20),
		new MthdKelvinTlParamLoad(opt, rnd(), "tl_param_load", -1, cls, 0xb80, 0x20),
		new MthdKelvinLightColor(opt, rnd(), "light_0_back_ambient_color", -1, cls, 0xc00, 0, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_0_back_diffuse_color", -1, cls, 0xc0c, 0, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_0_back_specular_color", -1, cls, 0xc18, 0, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_1_back_ambient_color", -1, cls, 0xc40, 1, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_1_back_diffuse_color", -1, cls, 0xc4c, 1, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_1_back_specular_color", -1, cls, 0xc58, 1, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_2_back_ambient_color", -1, cls, 0xc80, 2, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_2_back_diffuse_color", -1, cls, 0xc8c, 2, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_2_back_specular_color", -1, cls, 0xc98, 2, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_3_back_ambient_color", -1, cls, 0xcc0, 3, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_3_back_diffuse_color", -1, cls, 0xccc, 3, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_3_back_specular_color", -1, cls, 0xcd8, 3, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_4_back_ambient_color", -1, cls, 0xd00, 4, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_4_back_diffuse_color", -1, cls, 0xd0c, 4, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_4_back_specular_color", -1, cls, 0xd18, 4, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_5_back_ambient_color", -1, cls, 0xd40, 5, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_5_back_diffuse_color", -1, cls, 0xd4c, 5, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_5_back_specular_color", -1, cls, 0xd58, 5, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_6_back_ambient_color", -1, cls, 0xd80, 6, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_6_back_diffuse_color", -1, cls, 0xd8c, 6, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_6_back_specular_color", -1, cls, 0xd98, 6, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_7_back_ambient_color", -1, cls, 0xdc0, 7, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_7_back_diffuse_color", -1, cls, 0xdcc, 7, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_7_back_specular_color", -1, cls, 0xdd8, 7, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_0_ambient_color", -1, cls, 0x1000, 0, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_0_diffuse_color", -1, cls, 0x100c, 0, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_0_specular_color", -1, cls, 0x1018, 0, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_0_local_range", -1, cls, 0x1024, 0),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_0_half_vector", -1, cls, 0x1028, 0),
		new MthdKelvinLightDirection(opt, rnd(), "light_0_direction", -1, cls, 0x1034, 0),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_0_spot_cutoff_a", -1, cls, 0x1040, 0),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_0_spot_cutoff_b", -1, cls, 0x1044, 0),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_0_spot_cutoff_c", -1, cls, 0x1048, 0),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_0_spot_direction", -1, cls, 0x104c, 0),
		new MthdKelvinLightPosition(opt, rnd(), "light_0_position", -1, cls, 0x105c, 0),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_0_attenuation", -1, cls, 0x1068, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_1_ambient_color", -1, cls, 0x1080, 1, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_1_diffuse_color", -1, cls, 0x108c, 1, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_1_specular_color", -1, cls, 0x1098, 1, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_1_local_range", -1, cls, 0x10a4, 1),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_1_half_vector", -1, cls, 0x10a8, 1),
		new MthdKelvinLightDirection(opt, rnd(), "light_1_direction", -1, cls, 0x10b4, 1),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_1_spot_cutoff_a", -1, cls, 0x10c0, 1),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_1_spot_cutoff_b", -1, cls, 0x10c4, 1),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_1_spot_cutoff_c", -1, cls, 0x10c8, 1),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_1_spot_direction", -1, cls, 0x10cc, 1),
		new MthdKelvinLightPosition(opt, rnd(), "light_1_position", -1, cls, 0x10dc, 1),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_1_attenuation", -1, cls, 0x10e8, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_2_ambient_color", -1, cls, 0x1100, 2, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_2_diffuse_color", -1, cls, 0x110c, 2, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_2_specular_color", -1, cls, 0x1118, 2, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_2_local_range", -1, cls, 0x1124, 2),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_2_half_vector", -1, cls, 0x1128, 2),
		new MthdKelvinLightDirection(opt, rnd(), "light_2_direction", -1, cls, 0x1134, 2),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_2_spot_cutoff_a", -1, cls, 0x1140, 2),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_2_spot_cutoff_b", -1, cls, 0x1144, 2),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_2_spot_cutoff_c", -1, cls, 0x1148, 2),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_2_spot_direction", -1, cls, 0x114c, 2),
		new MthdKelvinLightPosition(opt, rnd(), "light_2_position", -1, cls, 0x115c, 2),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_2_attenuation", -1, cls, 0x1168, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_3_ambient_color", -1, cls, 0x1180, 3, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_3_diffuse_color", -1, cls, 0x118c, 3, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_3_specular_color", -1, cls, 0x1198, 3, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_3_local_range", -1, cls, 0x11a4, 3),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_3_half_vector", -1, cls, 0x11a8, 3),
		new MthdKelvinLightDirection(opt, rnd(), "light_3_direction", -1, cls, 0x11b4, 3),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_3_spot_cutoff_a", -1, cls, 0x11c0, 3),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_3_spot_cutoff_b", -1, cls, 0x11c4, 3),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_3_spot_cutoff_c", -1, cls, 0x11c8, 3),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_3_spot_direction", -1, cls, 0x11cc, 3),
		new MthdKelvinLightPosition(opt, rnd(), "light_3_position", -1, cls, 0x11dc, 3),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_3_attenuation", -1, cls, 0x11e8, 3),
		new MthdKelvinLightColor(opt, rnd(), "light_4_ambient_color", -1, cls, 0x1200, 4, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_4_diffuse_color", -1, cls, 0x120c, 4, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_4_specular_color", -1, cls, 0x1218, 4, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_4_local_range", -1, cls, 0x1224, 4),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_4_half_vector", -1, cls, 0x1228, 4),
		new MthdKelvinLightDirection(opt, rnd(), "light_4_direction", -1, cls, 0x1234, 4),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_4_spot_cutoff_a", -1, cls, 0x1240, 4),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_4_spot_cutoff_b", -1, cls, 0x1244, 4),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_4_spot_cutoff_c", -1, cls, 0x1248, 4),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_4_spot_direction", -1, cls, 0x124c, 4),
		new MthdKelvinLightPosition(opt, rnd(), "light_4_position", -1, cls, 0x125c, 4),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_4_attenuation", -1, cls, 0x1268, 4),
		new MthdKelvinLightColor(opt, rnd(), "light_5_ambient_color", -1, cls, 0x1280, 5, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_5_diffuse_color", -1, cls, 0x128c, 5, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_5_specular_color", -1, cls, 0x1298, 5, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_5_local_range", -1, cls, 0x12a4, 5),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_5_half_vector", -1, cls, 0x12a8, 5),
		new MthdKelvinLightDirection(opt, rnd(), "light_5_direction", -1, cls, 0x12b4, 5),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_5_spot_cutoff_a", -1, cls, 0x12c0, 5),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_5_spot_cutoff_b", -1, cls, 0x12c4, 5),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_5_spot_cutoff_c", -1, cls, 0x12c8, 5),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_5_spot_direction", -1, cls, 0x12cc, 5),
		new MthdKelvinLightPosition(opt, rnd(), "light_5_position", -1, cls, 0x12dc, 5),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_5_attenuation", -1, cls, 0x12e8, 5),
		new MthdKelvinLightColor(opt, rnd(), "light_6_ambient_color", -1, cls, 0x1300, 6, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_6_diffuse_color", -1, cls, 0x130c, 6, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_6_specular_color", -1, cls, 0x1318, 6, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_6_local_range", -1, cls, 0x1324, 6),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_6_half_vector", -1, cls, 0x1328, 6),
		new MthdKelvinLightDirection(opt, rnd(), "light_6_direction", -1, cls, 0x1334, 6),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_6_spot_cutoff_a", -1, cls, 0x1340, 6),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_6_spot_cutoff_b", -1, cls, 0x1344, 6),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_6_spot_cutoff_c", -1, cls, 0x1348, 6),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_6_spot_direction", -1, cls, 0x134c, 6),
		new MthdKelvinLightPosition(opt, rnd(), "light_6_position", -1, cls, 0x135c, 6),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_6_attenuation", -1, cls, 0x1368, 6),
		new MthdKelvinLightColor(opt, rnd(), "light_7_ambient_color", -1, cls, 0x1380, 7, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_7_diffuse_color", -1, cls, 0x138c, 7, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_7_specular_color", -1, cls, 0x1398, 7, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_7_local_range", -1, cls, 0x13a4, 7),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_7_half_vector", -1, cls, 0x13a8, 7),
		new MthdKelvinLightDirection(opt, rnd(), "light_7_direction", -1, cls, 0x13b4, 7),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_7_spot_cutoff_a", -1, cls, 0x13c0, 7),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_7_spot_cutoff_b", -1, cls, 0x13c4, 7),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_7_spot_cutoff_c", -1, cls, 0x13c8, 7),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_7_spot_direction", -1, cls, 0x13cc, 7),
		new MthdKelvinLightPosition(opt, rnd(), "light_7_position", -1, cls, 0x13dc, 7),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_7_attenuation", -1, cls, 0x13e8, 7),
		new MthdKelvinPolygonStippleEnable(opt, rnd(), "polygon_stipple_enable", -1, cls, 0x147c),
		new MthdKelvinPolygonStipple(opt, rnd(), "polygon_stipple", -1, cls, 0x1480, 0x20),
		new UntestedMthd(opt, rnd(), "vtx_pos_3f", -1, cls, 0x1500, 3), // XXX
		new UntestedMthd(opt, rnd(), "vtx_pos_3s", -1, cls, 0x1510, 2), // XXX
		new UntestedMthd(opt, rnd(), "vtx_pos_4f", -1, cls, 0x1518, 4), // XXX
		new UntestedMthd(opt, rnd(), "vtx_pos_4s", -1, cls, 0x1528, 2), // XXX
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_nrm_3f", -1, cls, 0x1530, 3, 0x2),
		new MthdKelvinVtxAttrNShort(opt, rnd(), "vtx_nrm_3s", -1, cls, 0x1540, 3, 0x2),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_col0_4f", -1, cls, 0x1550, 4, 0x3),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_col0_3f", -1, cls, 0x1560, 3, 0x3),
		new MthdKelvinVtxAttrUByte(opt, rnd(), "vtx_col0_4ub", -1, cls, 0x156c, 4, 0x3),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_col1_4f", -1, cls, 0x1570, 4, 0x4),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_col1_3f", -1, cls, 0x1580, 3, 0x4),
		new MthdKelvinVtxAttrUByte(opt, rnd(), "vtx_col1_4ub", -1, cls, 0x158c, 4, 0x4),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc0_2f", -1, cls, 0x1590, 2, 0x9),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc0_2s", -1, cls, 0x1598, 2, 0x9),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc0_4f", -1, cls, 0x15a0, 4, 0x9),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc0_4s", -1, cls, 0x15b0, 4, 0x9),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc1_2f", -1, cls, 0x15b8, 2, 0xa),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc1_2s", -1, cls, 0x15c0, 2, 0xa),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc1_4f", -1, cls, 0x15c8, 4, 0xa),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc1_4s", -1, cls, 0x15d8, 4, 0xa),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc2_2f", -1, cls, 0x15e0, 2, 0xb),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc2_2s", -1, cls, 0x15e8, 2, 0xb),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc2_4f", -1, cls, 0x15f0, 4, 0xb),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc2_4s", -1, cls, 0x1600, 4, 0xb),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc3_2f", -1, cls, 0x1608, 2, 0xc),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc3_2s", -1, cls, 0x1610, 2, 0xc),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_txc3_4f", -1, cls, 0x1620, 4, 0xc),
		new MthdKelvinVtxAttrShort(opt, rnd(), "vtx_txc3_4s", -1, cls, 0x1630, 4, 0xc),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_fog_1f", -1, cls, 0x1698, 1, 0x5),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_wei_1f", -1, cls, 0x169c, 1, 0x1),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_wei_2f", -1, cls, 0x16a0, 2, 0x1),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_wei_3f", -1, cls, 0x16b0, 3, 0x1),
		new MthdKelvinEdgeFlag(opt, rnd(), "vtx_edge_flag", -1, cls, 0x16bc),
		new MthdKelvinVtxAttrFloat(opt, rnd(), "vtx_wei_4f", -1, cls, 0x16c0, 4, 0x1),
		new MthdKelvinUnk16d0(opt, rnd(), "xf_unk16d0", -1, cls, 0x16d0),
		new MthdKelvinUnk16e0(opt, rnd(), "xf_unk16e0", -1, cls, 0x16e0),
		new MthdKelvinUnk16f0(opt, rnd(), "xf_unk16f0", -1, cls, 0x16f0),
		new MthdKelvinUnk1700(opt, rnd(), "xf_unk1700", -1, cls, 0x1700),
		new MthdKelvinUnkcf0(opt, rnd(), "unkcf0", -1, cls, 0x1710), // XXX
		new MthdKelvinUnkcf4(opt, rnd(), "unkcf4", -1, cls, 0x1714), // XXX
		new MthdKelvinXfNop(opt, rnd(), "xf_nop", -1, cls, 0x1718),
		new MthdKelvinXfSync(opt, rnd(), "xf_sync", -1, cls, 0x171c),
		new MthdKelvinVtxbufOffset(opt, rnd(), "vtxbuf_offset", -1, cls, 0x1720, 0x10),
		new MthdKelvinVtxbufFormat(opt, rnd(), "vtxbuf_format", -1, cls, 0x1760, 0x10),
		new MthdKelvinLightModelAmbientColor(opt, rnd(), "light_model_back_ambient_color", -1, cls, 0x17a0, 1),
		new MthdKelvinMaterialFactorA(opt, rnd(), "material_factor_back_a", -1, cls, 0x17ac, 1),
		new MthdKelvinMaterialFactorRgb(opt, rnd(), "material_factor_back_rgb", -1, cls, 0x17b0, 1),
		new MthdKelvinColorLogicOpEnable(opt, rnd(), "color_logic_op_enable", -1, cls, 0x17bc),
		new MthdKelvinColorLogicOpOp(opt, rnd(), "color_logic_op_op", -1, cls, 0x17c0),
		new MthdKelvinLightTwoSideEnable(opt, rnd(), "light_two_side_enable", -1, cls, 0x17c4),
		new MthdKelvinZPassCounterReset(opt, rnd(), "zpass_counter_reset", -1, cls, 0x17c8),
		new MthdKelvinZPassCounterEnable(opt, rnd(), "zpass_counter_enable", -1, cls, 0x17cc),
		new UntestedMthd(opt, rnd(), "zpass_counter_read", -1, cls, 0x17d0), // XXX
		new MthdKelvinLtUnk17d4(opt, rnd(), "lt_unk17d4", -1, cls, 0x17d4),
		new MthdKelvinLtUnk17e0(opt, rnd(), "lt_unk17e0", -1, cls, 0x17e0),
		new MthdKelvinLtUnk17ec(opt, rnd(), "lt_unk17ec", -1, cls, 0x17ec),
		new MthdKelvinTexShaderCullMode(opt, rnd(), "tex_shader_cull_mode", -1, cls, 0x17f8),
		new UntestedMthd(opt, rnd(), "begin", -1, cls, 0x17fc), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx16.data", -1, cls, 0x1800), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx32.data", -1, cls, 0x1808), // XXX
		new UntestedMthd(opt, rnd(), "draw_arrays.data", -1, cls, 0x1810), // XXX
		new UntestedMthd(opt, rnd(), "draw_inline.data", -1, cls, 0x1818), // XXX
		new MthdKelvinTexShaderConstEye(opt, rnd(), "tex_shader_const_eye", -1, cls, 0x181c, 3),
		new UntestedMthd(opt, rnd(), "idx_unk1828", -1, cls, 0x1828), // XXX
		new UntestedMthd(opt, rnd(), "vtx_attr0_2f", -1, cls, 0x1880, 2), // XXX
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr1_2f", -1, cls, 0x1888, 2, 0x1),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr2_2f", -1, cls, 0x1890, 2, 0x2),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr3_2f", -1, cls, 0x1898, 2, 0x3),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr4_2f", -1, cls, 0x18a0, 2, 0x4),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr5_2f", -1, cls, 0x18a8, 2, 0x5),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr6_2f", -1, cls, 0x18b0, 2, 0x6),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr7_2f", -1, cls, 0x18b8, 2, 0x7),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr8_2f", -1, cls, 0x18c0, 2, 0x8),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr9_2f", -1, cls, 0x18c8, 2, 0x9),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr10_2f", -1, cls, 0x18d0, 2, 0xa),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr11_2f", -1, cls, 0x18d8, 2, 0xb),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr12_2f", -1, cls, 0x18e0, 2, 0xc),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr13_2f", -1, cls, 0x18e8, 2, 0xd),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr14_2f", -1, cls, 0x18f0, 2, 0xe),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr15_2f", -1, cls, 0x18f8, 2, 0xf),
		new UntestedMthd(opt, rnd(), "vtx_attr0_2s", -1, cls, 0x1900), // XXX
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr1_2s", -1, cls, 0x1904, 2, 0x1),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr2_2s", -1, cls, 0x1908, 2, 0x2),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr3_2s", -1, cls, 0x190c, 2, 0x3),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr4_2s", -1, cls, 0x1910, 2, 0x4),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr5_2s", -1, cls, 0x1914, 2, 0x5),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr6_2s", -1, cls, 0x1918, 2, 0x6),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr7_2s", -1, cls, 0x191c, 2, 0x7),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr8_2s", -1, cls, 0x1920, 2, 0x8),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr9_2s", -1, cls, 0x1924, 2, 0x9),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr10_2s", -1, cls, 0x1928, 2, 0xa),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr11_2s", -1, cls, 0x192c, 2, 0xb),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr12_2s", -1, cls, 0x1930, 2, 0xc),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr13_2s", -1, cls, 0x1934, 2, 0xd),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr14_2s", -1, cls, 0x1938, 2, 0xe),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr15_2s", -1, cls, 0x193c, 2, 0xf),
		new UntestedMthd(opt, rnd(), "vtx_attr0_4ub", -1, cls, 0x1940), // XXX
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr1_4ub", -1, cls, 0x1944, 4, 0x1),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr2_4ub", -1, cls, 0x1948, 4, 0x2),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr3_4ub", -1, cls, 0x194c, 4, 0x3),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr4_4ub", -1, cls, 0x1950, 4, 0x4),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr5_4ub", -1, cls, 0x1954, 4, 0x5),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr6_4ub", -1, cls, 0x1958, 4, 0x6),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr7_4ub", -1, cls, 0x195c, 4, 0x7),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr8_4ub", -1, cls, 0x1960, 4, 0x8),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr9_4ub", -1, cls, 0x1964, 4, 0x9),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr10_4ub", -1, cls, 0x1968, 4, 0xa),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr11_4ub", -1, cls, 0x196c, 4, 0xb),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr12_4ub", -1, cls, 0x1970, 4, 0xc),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr13_4ub", -1, cls, 0x1974, 4, 0xd),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr14_4ub", -1, cls, 0x1978, 4, 0xe),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr15_4ub", -1, cls, 0x197c, 4, 0xf),
		new UntestedMthd(opt, rnd(), "vtx_attr0_4s", -1, cls, 0x1980, 2), // XXX
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr1_4s", -1, cls, 0x1988, 4, 0x1),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr2_4s", -1, cls, 0x1990, 4, 0x2),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr3_4s", -1, cls, 0x1998, 4, 0x3),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr4_4s", -1, cls, 0x19a0, 4, 0x4),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr5_4s", -1, cls, 0x19a8, 4, 0x5),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr6_4s", -1, cls, 0x19b0, 4, 0x6),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr7_4s", -1, cls, 0x19b8, 4, 0x7),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr8_4s", -1, cls, 0x19c0, 4, 0x8),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr9_4s", -1, cls, 0x19c8, 4, 0x9),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr10_4s", -1, cls, 0x19d0, 4, 0xa),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr11_4s", -1, cls, 0x19d8, 4, 0xb),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr12_4s", -1, cls, 0x19e0, 4, 0xc),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr13_4s", -1, cls, 0x19e8, 4, 0xd),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr14_4s", -1, cls, 0x19f0, 4, 0xe),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr15_4s", -1, cls, 0x19f8, 4, 0xf),
		new UntestedMthd(opt, rnd(), "vtx_attr0_4f", -1, cls, 0x1a00, 4), // XXX
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr1_4f", -1, cls, 0x1a10, 4, 0x1),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr2_4f", -1, cls, 0x1a20, 4, 0x2),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr3_4f", -1, cls, 0x1a30, 4, 0x3),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr4_4f", -1, cls, 0x1a40, 4, 0x4),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr5_4f", -1, cls, 0x1a50, 4, 0x5),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr6_4f", -1, cls, 0x1a60, 4, 0x6),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr7_4f", -1, cls, 0x1a70, 4, 0x7),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr8_4f", -1, cls, 0x1a80, 4, 0x8),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr9_4f", -1, cls, 0x1a90, 4, 0x9),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr10_4f", -1, cls, 0x1aa0, 4, 0xa),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr11_4f", -1, cls, 0x1ab0, 4, 0xb),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr12_4f", -1, cls, 0x1ac0, 4, 0xc),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr13_4f", -1, cls, 0x1ad0, 4, 0xd),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr14_4f", -1, cls, 0x1ae0, 4, 0xe),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr15_4f", -1, cls, 0x1af0, 4, 0xf),
		new MthdKelvinTexOffset(opt, rnd(), "tex_offset", -1, cls, 0x1b00, 4, 0x40),
		new MthdKelvinTexFormat(opt, rnd(), "tex_format", -1, cls, 0x1b04, 4, 0x40),
		new MthdKelvinTexWrap(opt, rnd(), "tex_wrap", -1, cls, 0x1b08, 4, 0x40),
		new MthdKelvinTexControl(opt, rnd(), "tex_control", -1, cls, 0x1b0c, 4, 0x40),
		new MthdKelvinTexPitch(opt, rnd(), "tex_pitch", -1, cls, 0x1b10, 4, 0x40),
		new MthdKelvinTexFilter(opt, rnd(), "tex_filter", -1, cls, 0x1b14, 4, 0x40),
		new MthdKelvinTexRect(opt, rnd(), "tex_rect", -1, cls, 0x1b1c, 4, 0x40),
		new MthdKelvinTexPalette(opt, rnd(), "tex_palette", -1, cls, 0x1b20, 4, 0x40),
		new MthdKelvinTexBorderColor(opt, rnd(), "tex_border_color", -1, cls, 0x1b24, 4, 0x40),
		// XXX broken
		new MthdKelvinTexUnk10(opt, rnd(), "tex_unk10", -1, cls, 0x1b68, 3, 0x40),
		// XXX broken
		new MthdKelvinTexUnk11(opt, rnd(), "tex_unk11", -1, cls, 0x1b6c, 3, 0x40),
		// XXX broken
		new MthdKelvinTexUnk12(opt, rnd(), "tex_unk12", -1, cls, 0x1b70, 3, 0x40),
		// XXX broken
		new MthdKelvinTexUnk13(opt, rnd(), "tex_unk13", -1, cls, 0x1b74, 3, 0x40),
		new MthdKelvinTexUnk14(opt, rnd(), "tex_unk14", -1, cls, 0x1b78, 3, 0x40),
		new MthdKelvinTexUnk15(opt, rnd(), "tex_unk15", -1, cls, 0x1b7c, 3, 0x40),
		new MthdKelvinStateSave(opt, rnd(), "state_save", -1, cls, 0x1d64),
		new UntestedMthd(opt, rnd(), "state_restore", -1, cls, 0x1d68), // XXX
		new MthdKelvinFenceOffset(opt, rnd(), "fence_offset", -1, cls, 0x1d6c),
		new UntestedMthd(opt, rnd(), "fence_write_a", -1, cls, 0x1d70), // XXX
		new UntestedMthd(opt, rnd(), "fence_write_b", -1, cls, 0x1d74), // XXX
		new MthdKelvinDepthClamp(opt, rnd(), "depth_clamp", -1, cls, 0x1d78),
		new MthdKelvinMultisample(opt, rnd(), "multisample", -1, cls, 0x1d7c),
		new MthdKelvinUnk1d80(opt, rnd(), "unk1d80", -1, cls, 0x1d80),
		new MthdKelvinUnk1d84(opt, rnd(), "unk1d84", -1, cls, 0x1d84),
		new MthdKelvinClearZeta(opt, rnd(), "clear_zeta", -1, cls, 0x1d8c),
		new MthdKelvinClearColor(opt, rnd(), "clear_color", -1, cls, 0x1d90),
		new UntestedMthd(opt, rnd(), "clear_trigger", -1, cls, 0x1d94), // XXX
		new MthdKelvinClearHv(opt, rnd(), "clear_h", -1, cls, 0x1d98, 0),
		new MthdKelvinClearHv(opt, rnd(), "clear_v", -1, cls, 0x1d9c, 1),
		new MthdKelvinFdBeginPatchA(opt, rnd(), "fd_begin_patch_a", -1, cls, 0x1de0),
		new MthdKelvinFdBeginPatchB(opt, rnd(), "fd_begin_patch_b", -1, cls, 0x1de4),
		new MthdKelvinFdBeginPatchC(opt, rnd(), "fd_begin_patch_c", -1, cls, 0x1de8),
		new MthdKelvinFdBeginPatchD(opt, rnd(), "fd_begin_patch_d", -1, cls, 0x1dec),
		new MthdKelvinFdEndPatch(opt, rnd(), "fd_end_patch", -1, cls, 0x1df0),
		new UntestedMthd(opt, rnd(), "fd_begin_swatch", -1, cls, 0x1df4), // XXX
		new UntestedMthd(opt, rnd(), "fd_begin_curve", -1, cls, 0x1df8), // XXX
		new MthdKelvinFdCurveData(opt, rnd(), "fd_curve_data", -1, cls, 0x1e00, 4),
		new MthdKelvinFdBeginTransitionA(opt, rnd(), "fd_begin_transition_a", -1, cls, 0x1e10),
		new MthdKelvinFdBeginTransitionB(opt, rnd(), "fd_begin_transition_b", -1, cls, 0x1e14),
		new UntestedMthd(opt, rnd(), "fd_begin_transition_c", -1, cls, 0x1e18), // XXX
		new UntestedMthd(opt, rnd(), "fd_end_transition", -1, cls, 0x1e1c), // XXX
		new MthdKelvinRcFinalFactor(opt, rnd(), "rc_final_factor", -1, cls, 0x1e20, 2),
		new MthdKelvinMaterialShininessA(opt, rnd(), "material_back_shininess_a", -1, cls, 0x1e28, 1),
		new MthdKelvinMaterialShininessB(opt, rnd(), "material_back_shininess_b", -1, cls, 0x1e2c, 1),
		new MthdKelvinMaterialShininessC(opt, rnd(), "material_back_shininess_c", -1, cls, 0x1e30, 1),
		new MthdKelvinMaterialShininessD(opt, rnd(), "material_back_shininess_d", -1, cls, 0x1e34, 1),
		new MthdKelvinMaterialShininessE(opt, rnd(), "material_back_shininess_e", -1, cls, 0x1e38, 1),
		new MthdKelvinMaterialShininessF(opt, rnd(), "material_back_shininess_f", -1, cls, 0x1e3c, 1),
		new MthdKelvinRcOutColor(opt, rnd(), "rc_out_color", -1, cls, 0x1e40, 8),
		new MthdKelvinRcConfig(opt, rnd(), "rc_config", -1, cls, 0x1e60),
		new MthdKelvinTexZcomp(opt, rnd(), "tex_zcomp", -1, cls, 0x1e6c),
		new MthdKelvinTexShaderOp(opt, rnd(), "tex_shader_op", -1, cls, 0x1e70),
		new MthdKelvinTexShaderDotmapping(opt, rnd(), "tex_shader_dotmapping", -1, cls, 0x1e74),
		new MthdKelvinTexShaderPrevious(opt, rnd(), "tex_shader_previous", -1, cls, 0x1e78),
		new MthdKelvinXfUnk4(opt, rnd(), "xf_unk4", -1, cls, 0x1e80, 4),
		new UntestedMthd(opt, rnd(), "xf_run_program", -1, cls, 0x1e90), // XXX
		new MthdKelvinTlMode(opt, rnd(), "tl_mode", -1, cls, 0x1e94),
		new MthdKelvinUnk1e98(opt, rnd(), "unk1e98", -1, cls, 0x1e98),
		new MthdKelvinTlProgramLoadPos(opt, rnd(), "tl_program_load_pos", -1, cls, 0x1e9c),
		new MthdKelvinTlProgramStartPos(opt, rnd(), "tl_program_start_pos", -1, cls, 0x1ea0),
		new MthdKelvinTlParamLoadPos(opt, rnd(), "tl_param_load_pos", -1, cls, 0x1ea4),
	};
	if (cls == 0x097) {
		res.insert(res.end(), {
			new MthdKelvinPointSmoothEnable(opt, rnd(), "point_smooth_enable", -1, cls, 0x31c),
			new MthdKelvinUnk1e68(opt, rnd(), "unk1e68", -1, cls, 0x1e68),
		});
	} else {
		res.insert(res.end(), {
			new MthdKelvinDmaClipid(opt, rnd(), "dma_clipid", -1, cls, 0x1ac),
			new MthdKelvinDmaZcull(opt, rnd(), "dma_zcull", -1, cls, 0x1b0),
			new MthdKelvinSurfPitchClipid(opt, rnd(), "surf_pitch_clipid", -1, cls, 0x224),
			new MthdKelvinSurfOffsetClipid(opt, rnd(), "surf_offset_clipid", -1, cls, 0x228),
			new MthdKelvinSurfPitchZcull(opt, rnd(), "surf_pitch_zcull", -1, cls, 0x22c),
			new MthdKelvinSurfOffsetZcull(opt, rnd(), "surf_offset_zcull", -1, cls, 0x230),
			new UntestedMthd(opt, rnd(), "unk234", -1, cls, 0x234), // XXX
			new MthdKelvinUnka0c(opt, rnd(), "unka0c", -1, cls, 0xa0c),
			new MthdKelvinPointSprite(opt, rnd(), "point_sprite", -1, cls, 0xa1c),
			new MthdKelvinLineStipplePattern(opt, rnd(), "line_stipple_pattern", -1, cls, 0x1478),
			new MthdKelvinUnk1d88(opt, rnd(), "unk1d88", -1, cls, 0x1d88),
			new UntestedMthd(opt, rnd(), "clear_clipid_trigger", -1, cls, 0x1da0), // XXX
			new MthdKelvinClipidEnable(opt, rnd(), "clipid_enable", -1, cls, 0x1da4),
			new MthdKelvinClipidId(opt, rnd(), "clipid_id", -1, cls, 0x1da8),
			new MthdKelvinUnk1dbc(opt, rnd(), "unk1dbc", -1, cls, 0x1dbc),
			new MthdKelvinUnk1dc0(opt, rnd(), "unk1dc0", -1, cls, 0x1dc0, 4),
			new MthdKelvinTexShaderUnk1e7c(opt, rnd(), "tex_shader_unk1e7c", -1, cls, 0x1e7c),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Rankine::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", -1, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", -1, cls, 0x110),
		new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
		new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
		new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
		new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
		new UntestedMthd(opt, rnd(), "flip_unk130", -1, cls, 0x130),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", -1, cls, 0x180),
		new MthdKelvinDmaTex(opt, rnd(), "dma_tex_a", -1, cls, 0x184, 0),
		new MthdKelvinDmaTex(opt, rnd(), "dma_tex_b", -1, cls, 0x188, 1),
		new MthdDmaSurf(opt, rnd(), "dma_surf_color_b", -1, cls, 0x18c, 6, SURF_NV10),
		new MthdKelvinDmaState(opt, rnd(), "dma_state", -1, cls, 0x190),
		new MthdDmaSurf(opt, rnd(), "dma_surf_color_a", -1, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", -1, cls, 0x198, 3, SURF_NV10),
		new MthdKelvinDmaVtx(opt, rnd(), "dma_vtx_a", -1, cls, 0x19c, 0),
		new MthdKelvinDmaVtx(opt, rnd(), "dma_vtx_b", -1, cls, 0x1a0, 1),
		new MthdDmaGrobj(opt, rnd(), "dma_fence", -1, cls, 0x1a4, 0, DMA_W | DMA_FENCE),
		new MthdDmaGrobj(opt, rnd(), "dma_query", -1, cls, 0x1a8, 1, DMA_W),
		new MthdKelvinDmaClipid(opt, rnd(), "dma_clipid", -1, cls, 0x1ac),
		new MthdKelvinDmaZcull(opt, rnd(), "dma_zcull", -1, cls, 0x1b0),
		new MthdKelvinClip(opt, rnd(), "clip_h", -1, cls, 0x200, 0, 0),
		new MthdKelvinClip(opt, rnd(), "clip_v", -1, cls, 0x204, 0, 1),
		new MthdSurf3DFormat(opt, rnd(), "surf_format", -1, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", -1, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_a_offset", -1, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", -1, cls, 0x214, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_b_offset", -1, cls, 0x218, 6, SURF_NV10),
		new UntestedMthd(opt, rnd(), "color_b_pitch", -1, cls, 0x21c), // XXX
		new MthdRankineRtEnable(opt, rnd(), "rt_enable", -1, cls, 0x220),
		new MthdKelvinSurfPitchClipid(opt, rnd(), "surf_pitch_clipid", -1, cls, 0x224),
		new MthdKelvinSurfOffsetClipid(opt, rnd(), "surf_offset_clipid", -1, cls, 0x228),
		new MthdKelvinSurfPitchZcull(opt, rnd(), "surf_pitch_zcull", -1, cls, 0x22c),
		new MthdKelvinSurfOffsetZcull(opt, rnd(), "surf_offset_zcull", -1, cls, 0x230),
		new UntestedMthd(opt, rnd(), "unk234", -1, cls, 0x234), // XXX
		new MthdRankineTxcCylwrap(opt, rnd(), "txc_cylwrap", -1, cls, 0x238),
		new MthdRankineTxcEnable(opt, rnd(), "txc_enable", -1, cls, 0x23c),
		new MthdKelvinTexMatrixEnable(opt, rnd(), "tex_matrix_enable", -1, cls, 0x240, 8),
		new MthdRankineViewportOffset(opt, rnd(), "viewport_offset", -1, cls, 0x2b8),
		new MthdKelvinClipRectMode(opt, rnd(), "clip_rect_mode", -1, cls, 0x2bc),
		new MthdKelvinClipRect(opt, rnd(), "clip_rect_horiz", -1, cls, 0x2c0, 8, 8, 0),
		new MthdKelvinClipRect(opt, rnd(), "clip_rect_vert", -1, cls, 0x2c4, 8, 8, 1),
		new MthdKelvinDitherEnable(opt, rnd(), "dither_enable", -1, cls, 0x300),
		new MthdKelvinAlphaFuncEnable(opt, rnd(), "alpha_func_enable", -1, cls, 0x304),
		new MthdKelvinAlphaFuncFunc(opt, rnd(), "alpha_func_func", -1, cls, 0x308),
		new MthdKelvinAlphaFuncRef(opt, rnd(), "alpha_func_ref", -1, cls, 0x30c),
		new MthdKelvinBlendFuncEnable(opt, rnd(), "blend_func_enable", -1, cls, 0x310),
		new MthdKelvinBlendFunc(opt, rnd(), "blend_func_src", -1, cls, 0x314, 0),
		new MthdKelvinBlendFunc(opt, rnd(), "blend_func_dst", -1, cls, 0x318, 1),
		new MthdKelvinBlendColor(opt, rnd(), "blend_color", -1, cls, 0x31c),
		new MthdKelvinBlendEquation(opt, rnd(), "blend_equation", -1, cls, 0x320),
		new MthdKelvinColorMask(opt, rnd(), "color_mask", -1, cls, 0x324),
		new MthdKelvinStencilEnable(opt, rnd(), "stencil_enable", -1, cls, 0x328, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_mask", -1, cls, 0x32c, 0, 2),
		new MthdKelvinStencilFunc(opt, rnd(), "stencil_func", -1, cls, 0x330, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_func_ref", -1, cls, 0x334, 0, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_func_mask", -1, cls, 0x338, 0, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_fail", -1, cls, 0x33c, 0, 0),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_zfail", -1, cls, 0x340, 0, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_zpass", -1, cls, 0x344, 0, 2),
		new MthdKelvinStencilEnable(opt, rnd(), "stencil_back_enable", -1, cls, 0x348, 1),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_back_mask", -1, cls, 0x34c, 1, 2),
		new MthdKelvinStencilFunc(opt, rnd(), "stencil_back_func", -1, cls, 0x350, 1),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_back_func_ref", -1, cls, 0x354, 1, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_back_func_mask", -1, cls, 0x358, 1, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_back_op_fail", -1, cls, 0x35c, 1, 0),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_back_op_zfail", -1, cls, 0x360, 1, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_back_op_zpass", -1, cls, 0x364, 1, 2),
		new MthdKelvinShadeMode(opt, rnd(), "shade_mode", -1, cls, 0x368),
		new MthdKelvinFogEnable(opt, rnd(), "fog_enable", -1, cls, 0x36c),
		new MthdKelvinFogColor(opt, rnd(), "fog_color", -1, cls, 0x370),
		new MthdKelvinColorLogicOpEnable(opt, rnd(), "color_logic_op_enable", -1, cls, 0x374),
		new MthdKelvinColorLogicOpOp(opt, rnd(), "color_logic_op_op", -1, cls, 0x378),
		new MthdKelvinNormalizeEnable(opt, rnd(), "normalize_enable", -1, cls, 0x37c),
		new MthdKelvinLightMaterial(opt, rnd(), "light_material", -1, cls, 0x390),
		new MthdKelvinDepthRangeNear(opt, rnd(), "depth_range_near", -1, cls, 0x394),
		new MthdKelvinDepthRangeFar(opt, rnd(), "depth_range_far", -1, cls, 0x398),
		new UntestedMthd(opt, rnd(), "idx_unk1828", -1, cls, 0x39c), // XXX
		new MthdKelvinMaterialFactorRgb(opt, rnd(), "material_factor_rgb", -1, cls, 0x3a0, 0),
		new MthdKelvinConfig(opt, rnd(), "config", -1, cls, 0x3b0),
		new MthdKelvinMaterialFactorA(opt, rnd(), "material_factor_a", -1, cls, 0x3b4, 0),
		new MthdKelvinLineWidth(opt, rnd(), "line_width", -1, cls, 0x3b8),
		new MthdKelvinLineSmoothEnable(opt, rnd(), "line_smooth_enable", -1, cls, 0x3bc),
		new MthdKelvinUnk4Matrix(opt, rnd(), "matrix_unk4", -1, cls, 0x3c0),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_s", -1, cls, 0x400, 8, 0x10, 0),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_t", -1, cls, 0x404, 8, 0x10, 1),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_r", -1, cls, 0x408, 8, 0x10, 2),
		new MthdKelvinTexGenMode(opt, rnd(), "tex_gen_mode_q", -1, cls, 0x40c, 8, 0x10, 3),
		new MthdKelvinMvMatrix(opt, rnd(), "matrix_mv0", -1, cls, 0x480, 0),
		new MthdKelvinMvMatrix(opt, rnd(), "matrix_mv1", -1, cls, 0x4c0, 1),
		new MthdKelvinMvMatrix(opt, rnd(), "matrix_mv2", -1, cls, 0x500, 2),
		new MthdKelvinMvMatrix(opt, rnd(), "matrix_mv3", -1, cls, 0x540, 3),
		new MthdKelvinImvMatrix(opt, rnd(), "matrix_imv0", -1, cls, 0x580, 0),
		new MthdKelvinImvMatrix(opt, rnd(), "matrix_imv1", -1, cls, 0x5c0, 1),
		new MthdKelvinImvMatrix(opt, rnd(), "matrix_imv2", -1, cls, 0x600, 2),
		new MthdKelvinImvMatrix(opt, rnd(), "matrix_imv3", -1, cls, 0x640, 3),
		new MthdKelvinProjMatrix(opt, rnd(), "matrix_proj", -1, cls, 0x680),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx0", -1, cls, 0x6c0, 0),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx1", -1, cls, 0x700, 1),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx2", -1, cls, 0x740, 2),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx3", -1, cls, 0x780, 3),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx4", -1, cls, 0x7c0, 4),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx5", -1, cls, 0x800, 5),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx6", -1, cls, 0x840, 6),
		new MthdKelvinTextureMatrix(opt, rnd(), "matrix_tx7", -1, cls, 0x880, 7),
		new MthdKelvinClip(opt, rnd(), "scissor_h", -1, cls, 0x8c0, 2, 0),
		new MthdKelvinClip(opt, rnd(), "scissor_v", -1, cls, 0x8c4, 2, 1),
		new MthdKelvinFogCoord(opt, rnd(), "fog_coord", -1, cls, 0x8c8),
		new MthdKelvinFogMode(opt, rnd(), "fog_mode", -1, cls, 0x8cc),
		new MthdKelvinFogCoeff(opt, rnd(), "fog_coeff", -1, cls, 0x8d0, 3),
		new UntestedMthd(opt, rnd(), "ps_offset", -1, cls, 0x8e4),
		new MthdKelvinWeightMode(opt, rnd(), "weight_mode", -1, cls, 0x8e8),
		new MthdKelvinRcFinalFactor(opt, rnd(), "rc_final_factor", -1, cls, 0x8ec, 2),
		new MthdKelvinRcFinalA(opt, rnd(), "rc_final_a", -1, cls, 0x8f4),
		new MthdKelvinRcFinalB(opt, rnd(), "rc_final_b", -1, cls, 0x8f8),
		new MthdKelvinRcConfig(opt, rnd(), "rc_config", -1, cls, 0x8fc),
		new MthdKelvinRcInAlpha(opt, rnd(), "rc_in_alpha", -1, cls, 0x900, 8, 0x20),
		new MthdKelvinRcInColor(opt, rnd(), "rc_in_color", -1, cls, 0x904, 8, 0x20),
		new MthdKelvinRcFactorA(opt, rnd(), "rc_factor_a", -1, cls, 0x908, 8, 0x20),
		new MthdKelvinRcFactorB(opt, rnd(), "rc_factor_b", -1, cls, 0x90c, 8, 0x20),
		new MthdKelvinRcOutAlpha(opt, rnd(), "rc_out_alpha", -1, cls, 0x910, 8, 0x20),
		new MthdKelvinRcOutColor(opt, rnd(), "rc_out_color", -1, cls, 0x914, 8, 0x20),
		new MthdKelvinClip(opt, rnd(), "viewport_h", -1, cls, 0xa00, 1, 0),
		new MthdKelvinClip(opt, rnd(), "viewport_v", -1, cls, 0xa04, 1, 1),
		new MthdRankineUnka08(opt, rnd(), "unka08", -1, cls, 0xa08),
		new MthdKelvinUnka0c(opt, rnd(), "unka0c", -1, cls, 0xa0c),
		new MthdKelvinLightModelAmbientColor(opt, rnd(), "light_model_ambient_color", -1, cls, 0xa10, 0),
		new MthdKelvinViewportTranslate(opt, rnd(), "viewport_translate", -1, cls, 0xa20),
		new MthdKelvinViewportScale(opt, rnd(), "viewport_scale", -1, cls, 0xa30),
		new MthdRankineUnka40(opt, rnd(), "unka40", -1, cls, 0xa40),
		new MthdState(opt, rnd(), "unka44", -1, cls, 0xa44),
		new MthdState(opt, rnd(), "unka4c", -1, cls, 0xa4c),
		new MthdKelvinLightEyePosition(opt, rnd(), "light_eye_position", -1, cls, 0xa50),
		new MthdKelvinPolygonOffsetPointEnable(opt, rnd(), "polygon_offset_point_enable", -1, cls, 0xa60),
		new MthdKelvinPolygonOffsetLineEnable(opt, rnd(), "polygon_offset_line_enable", -1, cls, 0xa64),
		new MthdKelvinPolygonOffsetFillEnable(opt, rnd(), "polygon_offset_fill_enable", -1, cls, 0xa68),
		new MthdKelvinDepthFunc(opt, rnd(), "depth_func", -1, cls, 0xa6c),
		new MthdKelvinDepthWriteEnable(opt, rnd(), "depth_write_enable", -1, cls, 0xa70),
		new MthdKelvinDepthTestEnable(opt, rnd(), "depth_test_enable", -1, cls, 0xa74),
		new MthdKelvinPolygonOffsetFactor(opt, rnd(), "polygon_offset_factor", -1, cls, 0xa78),
		new MthdKelvinPolygonOffsetUnits(opt, rnd(), "polygon_offset_units", -1, cls, 0xa7c),
		new UntestedMthd(opt, rnd(), "vtx_attr0_4ns", -1, cls, 0xa80, 2), // XXX
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr1_4ns", -1, cls, 0xa88, 4, 0x1),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr2_4ns", -1, cls, 0xa90, 4, 0x2),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr3_4ns", -1, cls, 0xa98, 4, 0x3),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr4_4ns", -1, cls, 0xaa0, 4, 0x4),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr5_4ns", -1, cls, 0xaa8, 4, 0x5),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr6_4ns", -1, cls, 0xab0, 4, 0x6),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr7_4ns", -1, cls, 0xab8, 4, 0x7),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr8_4ns", -1, cls, 0xac0, 4, 0x8),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr9_4ns", -1, cls, 0xac8, 4, 0x9),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr10_4ns", -1, cls, 0xad0, 4, 0xa),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr11_4ns", -1, cls, 0xad8, 4, 0xb),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr12_4ns", -1, cls, 0xae0, 4, 0xc),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr13_4ns", -1, cls, 0xae8, 4, 0xd),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr14_4ns", -1, cls, 0xaf0, 4, 0xe),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr15_4ns", -1, cls, 0xaf8, 4, 0xf),
		new MthdRankineTexFilterOpt(opt, rnd(), "tex_filter_opt", -1, cls, 0xb00, 0x10),
		new MthdKelvinTlProgramLoad(opt, rnd(), "tl_program_load", -1, cls, 0xb80, 0x20),
		new MthdKelvinLightColor(opt, rnd(), "light_0_back_ambient_color", -1, cls, 0xc00, 0, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_0_back_diffuse_color", -1, cls, 0xc0c, 0, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_0_back_specular_color", -1, cls, 0xc18, 0, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_1_back_ambient_color", -1, cls, 0xc40, 1, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_1_back_diffuse_color", -1, cls, 0xc4c, 1, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_1_back_specular_color", -1, cls, 0xc58, 1, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_2_back_ambient_color", -1, cls, 0xc80, 2, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_2_back_diffuse_color", -1, cls, 0xc8c, 2, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_2_back_specular_color", -1, cls, 0xc98, 2, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_3_back_ambient_color", -1, cls, 0xcc0, 3, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_3_back_diffuse_color", -1, cls, 0xccc, 3, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_3_back_specular_color", -1, cls, 0xcd8, 3, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_4_back_ambient_color", -1, cls, 0xd00, 4, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_4_back_diffuse_color", -1, cls, 0xd0c, 4, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_4_back_specular_color", -1, cls, 0xd18, 4, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_5_back_ambient_color", -1, cls, 0xd40, 5, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_5_back_diffuse_color", -1, cls, 0xd4c, 5, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_5_back_specular_color", -1, cls, 0xd58, 5, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_6_back_ambient_color", -1, cls, 0xd80, 6, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_6_back_diffuse_color", -1, cls, 0xd8c, 6, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_6_back_specular_color", -1, cls, 0xd98, 6, 1, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_7_back_ambient_color", -1, cls, 0xdc0, 7, 1, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_7_back_diffuse_color", -1, cls, 0xdcc, 7, 1, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_7_back_specular_color", -1, cls, 0xdd8, 7, 1, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_0_s_plane", -1, cls, 0xe00, 0, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_0_t_plane", -1, cls, 0xe10, 0, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_0_r_plane", -1, cls, 0xe20, 0, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_0_q_plane", -1, cls, 0xe30, 0, 3),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_1_s_plane", -1, cls, 0xe40, 1, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_1_t_plane", -1, cls, 0xe50, 1, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_1_r_plane", -1, cls, 0xe60, 1, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_1_q_plane", -1, cls, 0xe70, 1, 3),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_2_s_plane", -1, cls, 0xe80, 2, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_2_t_plane", -1, cls, 0xe90, 2, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_2_r_plane", -1, cls, 0xea0, 2, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_2_q_plane", -1, cls, 0xeb0, 2, 3),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_3_s_plane", -1, cls, 0xec0, 3, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_3_t_plane", -1, cls, 0xed0, 3, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_3_r_plane", -1, cls, 0xee0, 3, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_3_q_plane", -1, cls, 0xef0, 3, 3),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_4_s_plane", -1, cls, 0xf00, 4, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_4_t_plane", -1, cls, 0xf10, 4, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_4_r_plane", -1, cls, 0xf20, 4, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_4_q_plane", -1, cls, 0xf30, 4, 3),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_5_s_plane", -1, cls, 0xf40, 5, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_5_t_plane", -1, cls, 0xf50, 5, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_5_r_plane", -1, cls, 0xf60, 5, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_5_q_plane", -1, cls, 0xf70, 5, 3),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_6_s_plane", -1, cls, 0xf80, 6, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_6_t_plane", -1, cls, 0xf90, 6, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_6_r_plane", -1, cls, 0xfa0, 6, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_6_q_plane", -1, cls, 0xfb0, 6, 3),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_7_s_plane", -1, cls, 0xfc0, 7, 0),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_7_t_plane", -1, cls, 0xfd0, 7, 1),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_7_r_plane", -1, cls, 0xfe0, 7, 2),
		new MthdKelvinTexGen(opt, rnd(), "tex_gen_7_q_plane", -1, cls, 0xff0, 7, 3),
		new MthdKelvinLightColor(opt, rnd(), "light_0_ambient_color", -1, cls, 0x1000, 0, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_0_diffuse_color", -1, cls, 0x100c, 0, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_0_specular_color", -1, cls, 0x1018, 0, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_0_local_range", -1, cls, 0x1024, 0),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_0_half_vector", -1, cls, 0x1028, 0),
		new MthdKelvinLightDirection(opt, rnd(), "light_0_direction", -1, cls, 0x1034, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_1_ambient_color", -1, cls, 0x1040, 1, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_1_diffuse_color", -1, cls, 0x104c, 1, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_1_specular_color", -1, cls, 0x1058, 1, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_1_local_range", -1, cls, 0x1064, 1),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_1_half_vector", -1, cls, 0x1068, 1),
		new MthdKelvinLightDirection(opt, rnd(), "light_1_direction", -1, cls, 0x1074, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_2_ambient_color", -1, cls, 0x1080, 2, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_2_diffuse_color", -1, cls, 0x108c, 2, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_2_specular_color", -1, cls, 0x1098, 2, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_2_local_range", -1, cls, 0x10a4, 2),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_2_half_vector", -1, cls, 0x10a8, 2),
		new MthdKelvinLightDirection(opt, rnd(), "light_2_direction", -1, cls, 0x10b4, 2),
		new MthdKelvinLightColor(opt, rnd(), "light_3_ambient_color", -1, cls, 0x10c0, 3, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_3_diffuse_color", -1, cls, 0x10cc, 3, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_3_specular_color", -1, cls, 0x10d8, 3, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_3_local_range", -1, cls, 0x10e4, 3),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_3_half_vector", -1, cls, 0x10e8, 3),
		new MthdKelvinLightDirection(opt, rnd(), "light_3_direction", -1, cls, 0x10f4, 3),
		new MthdKelvinLightColor(opt, rnd(), "light_4_ambient_color", -1, cls, 0x1100, 4, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_4_diffuse_color", -1, cls, 0x110c, 4, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_4_specular_color", -1, cls, 0x1118, 4, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_4_local_range", -1, cls, 0x1124, 4),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_4_half_vector", -1, cls, 0x1128, 4),
		new MthdKelvinLightDirection(opt, rnd(), "light_4_direction", -1, cls, 0x1134, 4),
		new MthdKelvinLightColor(opt, rnd(), "light_5_ambient_color", -1, cls, 0x1140, 5, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_5_diffuse_color", -1, cls, 0x114c, 5, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_5_specular_color", -1, cls, 0x1158, 5, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_5_local_range", -1, cls, 0x1164, 5),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_5_half_vector", -1, cls, 0x1168, 5),
		new MthdKelvinLightDirection(opt, rnd(), "light_5_direction", -1, cls, 0x1174, 5),
		new MthdKelvinLightColor(opt, rnd(), "light_6_ambient_color", -1, cls, 0x1180, 6, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_6_diffuse_color", -1, cls, 0x118c, 6, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_6_specular_color", -1, cls, 0x1198, 6, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_6_local_range", -1, cls, 0x11a4, 6),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_6_half_vector", -1, cls, 0x11a8, 6),
		new MthdKelvinLightDirection(opt, rnd(), "light_6_direction", -1, cls, 0x11b4, 6),
		new MthdKelvinLightColor(opt, rnd(), "light_7_ambient_color", -1, cls, 0x11c0, 7, 0, 0),
		new MthdKelvinLightColor(opt, rnd(), "light_7_diffuse_color", -1, cls, 0x11cc, 7, 0, 1),
		new MthdKelvinLightColor(opt, rnd(), "light_7_specular_color", -1, cls, 0x11d8, 7, 0, 2),
		new MthdKelvinLightLocalRange(opt, rnd(), "light_7_local_range", -1, cls, 0x11e4, 7),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_7_half_vector", -1, cls, 0x11e8, 7),
		new MthdKelvinLightDirection(opt, rnd(), "light_7_direction", -1, cls, 0x11f4, 7),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_0_spot_cutoff_a", -1, cls, 0x1200, 0),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_0_spot_cutoff_b", -1, cls, 0x1204, 0),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_0_spot_cutoff_c", -1, cls, 0x1208, 0),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_0_spot_direction", -1, cls, 0x120c, 0),
		new MthdKelvinLightPosition(opt, rnd(), "light_0_position", -1, cls, 0x121c, 0),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_0_attenuation", -1, cls, 0x1228, 0),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_1_spot_cutoff_a", -1, cls, 0x1240, 1),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_1_spot_cutoff_b", -1, cls, 0x1244, 1),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_1_spot_cutoff_c", -1, cls, 0x1248, 1),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_1_spot_direction", -1, cls, 0x124c, 1),
		new MthdKelvinLightPosition(opt, rnd(), "light_1_position", -1, cls, 0x125c, 1),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_1_attenuation", -1, cls, 0x1268, 1),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_2_spot_cutoff_a", -1, cls, 0x1280, 2),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_2_spot_cutoff_b", -1, cls, 0x1284, 2),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_2_spot_cutoff_c", -1, cls, 0x1288, 2),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_2_spot_direction", -1, cls, 0x128c, 2),
		new MthdKelvinLightPosition(opt, rnd(), "light_2_position", -1, cls, 0x129c, 2),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_2_attenuation", -1, cls, 0x12a8, 2),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_3_spot_cutoff_a", -1, cls, 0x12c0, 3),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_3_spot_cutoff_b", -1, cls, 0x12c4, 3),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_3_spot_cutoff_c", -1, cls, 0x12c8, 3),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_3_spot_direction", -1, cls, 0x12cc, 3),
		new MthdKelvinLightPosition(opt, rnd(), "light_3_position", -1, cls, 0x12dc, 3),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_3_attenuation", -1, cls, 0x12e8, 3),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_4_spot_cutoff_a", -1, cls, 0x1300, 4),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_4_spot_cutoff_b", -1, cls, 0x1304, 4),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_4_spot_cutoff_c", -1, cls, 0x1308, 4),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_4_spot_direction", -1, cls, 0x130c, 4),
		new MthdKelvinLightPosition(opt, rnd(), "light_4_position", -1, cls, 0x131c, 4),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_4_attenuation", -1, cls, 0x1328, 4),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_5_spot_cutoff_a", -1, cls, 0x1340, 5),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_5_spot_cutoff_b", -1, cls, 0x1344, 5),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_5_spot_cutoff_c", -1, cls, 0x1348, 5),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_5_spot_direction", -1, cls, 0x134c, 5),
		new MthdKelvinLightPosition(opt, rnd(), "light_5_position", -1, cls, 0x135c, 5),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_5_attenuation", -1, cls, 0x1368, 5),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_6_spot_cutoff_a", -1, cls, 0x1380, 6),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_6_spot_cutoff_b", -1, cls, 0x1384, 6),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_6_spot_cutoff_c", -1, cls, 0x1388, 6),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_6_spot_direction", -1, cls, 0x138c, 6),
		new MthdKelvinLightPosition(opt, rnd(), "light_6_position", -1, cls, 0x139c, 6),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_6_attenuation", -1, cls, 0x13a8, 6),
		new MthdKelvinLightSpotCutoffA(opt, rnd(), "light_7_spot_cutoff_a", -1, cls, 0x13c0, 7),
		new MthdKelvinLightSpotCutoffB(opt, rnd(), "light_7_spot_cutoff_b", -1, cls, 0x13c4, 7),
		new MthdKelvinLightSpotCutoffC(opt, rnd(), "light_7_spot_cutoff_c", -1, cls, 0x13c8, 7),
		new MthdKelvinLightSpotDirection(opt, rnd(), "light_7_spot_direction", -1, cls, 0x13cc, 7),
		new MthdKelvinLightPosition(opt, rnd(), "light_7_position", -1, cls, 0x13dc, 7),
		new MthdKelvinLightHalfVectorAtt(opt, rnd(), "light_7_attenuation", -1, cls, 0x13e8, 7),
		new MthdKelvinMaterialShininessA(opt, rnd(), "material_shininess_a", -1, cls, 0x1400, 0),
		new MthdKelvinMaterialShininessB(opt, rnd(), "material_shininess_b", -1, cls, 0x1404, 0),
		new MthdKelvinMaterialShininessC(opt, rnd(), "material_shininess_c", -1, cls, 0x1408, 0),
		new MthdKelvinMaterialShininessD(opt, rnd(), "material_shininess_d", -1, cls, 0x140c, 0),
		new MthdKelvinMaterialShininessE(opt, rnd(), "material_shininess_e", -1, cls, 0x1410, 0),
		new MthdKelvinMaterialShininessF(opt, rnd(), "material_shininess_f", -1, cls, 0x1414, 0),
		new MthdKelvinLightEnable(opt, rnd(), "light_enable", -1, cls, 0x1420),
		new MthdKelvinLightModel(opt, rnd(), "light_model", -1, cls, 0x1424),
		new MthdKelvinSpecularEnable(opt, rnd(), "specular_enable", -1, cls, 0x1428),
		new MthdKelvinLightTwoSideEnable(opt, rnd(), "light_two_side_enable", -1, cls, 0x142c),
		new MthdKelvinTlUnk9cc(opt, rnd(), "tl_unk9cc", -1, cls, 0x143c),
		new MthdKelvinFogPlane(opt, rnd(), "fog_plane", -1, cls, 0x1440),
		new MthdKelvinUnk3f0(opt, rnd(), "unk3f0", -1, cls, 0x1450),
		new MthdKelvinFlatshadeFirst(opt, rnd(), "flatshade_first", -1, cls, 0x1454),
		new MthdKelvinLightingEnable(opt, rnd(), "lighting_enable", -1, cls, 0x1458),
		new MthdKelvinEdgeFlag(opt, rnd(), "vtx_edge_flag", -1, cls, 0x145c),
		new MthdRankineClipPlaneEnable(opt, rnd(), "clip_plane_enable", -1, cls, 0x1478),
		new MthdKelvinPolygonStippleEnable(opt, rnd(), "polygon_stipple_enable", -1, cls, 0x147c),
		new MthdKelvinPolygonStipple(opt, rnd(), "polygon_stipple", -1, cls, 0x1480, 0x20),
		new UntestedMthd(opt, rnd(), "vtx_attr0_3f", -1, cls, 0x1500, 3), // XXX
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr1_3f", -1, cls, 0x1510, 3, 0x1),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr2_3f", -1, cls, 0x1520, 3, 0x2),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr3_3f", -1, cls, 0x1530, 3, 0x3),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr4_3f", -1, cls, 0x1540, 3, 0x4),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr5_3f", -1, cls, 0x1550, 3, 0x5),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr6_3f", -1, cls, 0x1560, 3, 0x6),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr7_3f", -1, cls, 0x1570, 3, 0x7),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr8_3f", -1, cls, 0x1580, 3, 0x8),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr9_3f", -1, cls, 0x1590, 3, 0x9),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr10_3f", -1, cls, 0x15a0, 3, 0xa),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr11_3f", -1, cls, 0x15b0, 3, 0xb),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr12_3f", -1, cls, 0x15c0, 3, 0xc),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr13_3f", -1, cls, 0x15d0, 3, 0xd),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr14_3f", -1, cls, 0x15e0, 3, 0xe),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr15_3f", -1, cls, 0x15f0, 3, 0xf),
		new MthdRankineUserClipPlane(opt, rnd(), "user_clip_plane_0", -1, cls, 0x1600, 0),
		new MthdRankineUserClipPlane(opt, rnd(), "user_clip_plane_1", -1, cls, 0x1610, 1),
		new MthdRankineUserClipPlane(opt, rnd(), "user_clip_plane_2", -1, cls, 0x1620, 2),
		new MthdRankineUserClipPlane(opt, rnd(), "user_clip_plane_3", -1, cls, 0x1630, 3),
		new MthdRankineUserClipPlane(opt, rnd(), "user_clip_plane_4", -1, cls, 0x1640, 4),
		new MthdRankineUserClipPlane(opt, rnd(), "user_clip_plane_5", -1, cls, 0x1650, 5),
		new MthdKelvinVtxbufOffset(opt, rnd(), "vtxbuf_offset", -1, cls, 0x1680, 0x10),
		new MthdKelvinUnk16e0(opt, rnd(), "xf_unk16e0", -1, cls, 0x16d0),
		new MthdKelvinUnk16f0(opt, rnd(), "xf_unk16f0", -1, cls, 0x16e0),
		new MthdKelvinUnk1700(opt, rnd(), "xf_unk1700", -1, cls, 0x16f0),
		new MthdKelvinUnk16d0(opt, rnd(), "xf_unk16d0", -1, cls, 0x1700),
		new MthdKelvinUnkcf0(opt, rnd(), "unkcf0", -1, cls, 0x1710), // XXX
		new MthdKelvinUnkcf4(opt, rnd(), "unkcf4", -1, cls, 0x1714), // XXX
		// XXX broken
		new MthdKelvinXfNop(opt, rnd(), "xf_nop", -1, cls, 0x1718),
		new MthdKelvinXfSync(opt, rnd(), "xf_sync", -1, cls, 0x171c),
		// XXX broken
		new MthdKelvinVtxbufFormat(opt, rnd(), "vtxbuf_format", -1, cls, 0x1740, 0x10),
		new MthdKelvinLightModelAmbientColor(opt, rnd(), "light_model_back_ambient_color", -1, cls, 0x17a0, 1),
		new MthdKelvinMaterialFactorRgb(opt, rnd(), "material_factor_back_rgb", -1, cls, 0x17b0, 1),
		new MthdKelvinMaterialFactorA(opt, rnd(), "material_factor_back_a", -1, cls, 0x17c0, 1),
		new MthdKelvinZPassCounterReset(opt, rnd(), "zpass_counter_reset", -1, cls, 0x17c8),
		new MthdKelvinZPassCounterEnable(opt, rnd(), "zpass_counter_enable", -1, cls, 0x17cc),
		new MthdKelvinLtUnk17d4(opt, rnd(), "lt_unk17d4", -1, cls, 0x17d0),
		new MthdKelvinLtUnk17e0(opt, rnd(), "lt_unk17e0", -1, cls, 0x17e0),
		new UntestedMthd(opt, rnd(), "zpass_counter_read", -1, cls, 0x1800), // XXX
		new UntestedMthd(opt, rnd(), "begin", -1, cls, 0x1808), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx16.data", -1, cls, 0x180c), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx32.data", -1, cls, 0x1810), // XXX
		new UntestedMthd(opt, rnd(), "draw_arrays.data", -1, cls, 0x1814), // XXX
		new UntestedMthd(opt, rnd(), "draw_inline.data", -1, cls, 0x1818), // XXX
		new MthdRankineIdxbufOffset(opt, rnd(), "idxbuf_offset", -1, cls, 0x181c),
		new MthdRankineIdxbufFormat(opt, rnd(), "idxbuf_format", -1, cls, 0x1820),
		new UntestedMthd(opt, rnd(), "draw_index.data", -1, cls, 0x1824), // XXX
		new MthdKelvinPolygonMode(opt, rnd(), "polygon_mode_front", -1, cls, 0x1828, 0),
		new MthdKelvinPolygonMode(opt, rnd(), "polygon_mode_back", -1, cls, 0x182c, 1),
		new MthdKelvinCullFace(opt, rnd(), "cull_face", -1, cls, 0x1830),
		new MthdKelvinFrontFace(opt, rnd(), "front_face", -1, cls, 0x1834),
		new MthdKelvinPolygonSmoothEnable(opt, rnd(), "polygon_smooth_enable", -1, cls, 0x1838),
		new MthdKelvinCullFaceEnable(opt, rnd(), "cull_face_enable", -1, cls, 0x183c),
		new MthdKelvinTexPalette(opt, rnd(), "tex_palette", -1, cls, 0x1840, 0x10),
		new UntestedMthd(opt, rnd(), "vtx_attr0_2f", -1, cls, 0x1880, 2), // XXX
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr1_2f", -1, cls, 0x1888, 2, 0x1),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr2_2f", -1, cls, 0x1890, 2, 0x2),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr3_2f", -1, cls, 0x1898, 2, 0x3),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr4_2f", -1, cls, 0x18a0, 2, 0x4),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr5_2f", -1, cls, 0x18a8, 2, 0x5),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr6_2f", -1, cls, 0x18b0, 2, 0x6),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr7_2f", -1, cls, 0x18b8, 2, 0x7),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr8_2f", -1, cls, 0x18c0, 2, 0x8),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr9_2f", -1, cls, 0x18c8, 2, 0x9),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr10_2f", -1, cls, 0x18d0, 2, 0xa),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr11_2f", -1, cls, 0x18d8, 2, 0xb),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr12_2f", -1, cls, 0x18e0, 2, 0xc),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr13_2f", -1, cls, 0x18e8, 2, 0xd),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr14_2f", -1, cls, 0x18f0, 2, 0xe),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr15_2f", -1, cls, 0x18f8, 2, 0xf),
		new UntestedMthd(opt, rnd(), "vtx_attr0_2s", -1, cls, 0x1900), // XXX
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr1_2s", -1, cls, 0x1904, 2, 0x1),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr2_2s", -1, cls, 0x1908, 2, 0x2),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr3_2s", -1, cls, 0x190c, 2, 0x3),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr4_2s", -1, cls, 0x1910, 2, 0x4),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr5_2s", -1, cls, 0x1914, 2, 0x5),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr6_2s", -1, cls, 0x1918, 2, 0x6),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr7_2s", -1, cls, 0x191c, 2, 0x7),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr8_2s", -1, cls, 0x1920, 2, 0x8),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr9_2s", -1, cls, 0x1924, 2, 0x9),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr10_2s", -1, cls, 0x1928, 2, 0xa),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr11_2s", -1, cls, 0x192c, 2, 0xb),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr12_2s", -1, cls, 0x1930, 2, 0xc),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr13_2s", -1, cls, 0x1934, 2, 0xd),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr14_2s", -1, cls, 0x1938, 2, 0xe),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr15_2s", -1, cls, 0x193c, 2, 0xf),
		new UntestedMthd(opt, rnd(), "vtx_attr0_4ub", -1, cls, 0x1940), // XXX
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr1_4ub", -1, cls, 0x1944, 4, 0x1),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr2_4ub", -1, cls, 0x1948, 4, 0x2),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr3_4ub", -1, cls, 0x194c, 4, 0x3),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr4_4ub", -1, cls, 0x1950, 4, 0x4),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr5_4ub", -1, cls, 0x1954, 4, 0x5),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr6_4ub", -1, cls, 0x1958, 4, 0x6),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr7_4ub", -1, cls, 0x195c, 4, 0x7),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr8_4ub", -1, cls, 0x1960, 4, 0x8),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr9_4ub", -1, cls, 0x1964, 4, 0x9),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr10_4ub", -1, cls, 0x1968, 4, 0xa),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr11_4ub", -1, cls, 0x196c, 4, 0xb),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr12_4ub", -1, cls, 0x1970, 4, 0xc),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr13_4ub", -1, cls, 0x1974, 4, 0xd),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr14_4ub", -1, cls, 0x1978, 4, 0xe),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr15_4ub", -1, cls, 0x197c, 4, 0xf),
		new UntestedMthd(opt, rnd(), "vtx_attr0_4s", -1, cls, 0x1980, 2), // XXX
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr1_4s", -1, cls, 0x1988, 4, 0x1),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr2_4s", -1, cls, 0x1990, 4, 0x2),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr3_4s", -1, cls, 0x1998, 4, 0x3),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr4_4s", -1, cls, 0x19a0, 4, 0x4),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr5_4s", -1, cls, 0x19a8, 4, 0x5),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr6_4s", -1, cls, 0x19b0, 4, 0x6),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr7_4s", -1, cls, 0x19b8, 4, 0x7),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr8_4s", -1, cls, 0x19c0, 4, 0x8),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr9_4s", -1, cls, 0x19c8, 4, 0x9),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr10_4s", -1, cls, 0x19d0, 4, 0xa),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr11_4s", -1, cls, 0x19d8, 4, 0xb),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr12_4s", -1, cls, 0x19e0, 4, 0xc),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr13_4s", -1, cls, 0x19e8, 4, 0xd),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr14_4s", -1, cls, 0x19f0, 4, 0xe),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr15_4s", -1, cls, 0x19f8, 4, 0xf),
		new MthdKelvinTexOffset(opt, rnd(), "tex_offset", -1, cls, 0x1a00, 0x10, 0x20),
		new MthdKelvinTexFormat(opt, rnd(), "tex_format", -1, cls, 0x1a04, 0x10, 0x20),
		new MthdRankineTexWrap(opt, rnd(), "tex_wrap", -1, cls, 0x1a08, 0x10, 0x20),
		new MthdKelvinTexControl(opt, rnd(), "tex_control", -1, cls, 0x1a0c, 0x10, 0x20),
		new MthdKelvinTexPitch(opt, rnd(), "tex_pitch", -1, cls, 0x1a10, 0x10, 0x20),
		new MthdKelvinTexFilter(opt, rnd(), "tex_filter", -1, cls, 0x1a14, 0x10, 0x20),
		new MthdKelvinTexRect(opt, rnd(), "tex_rect", -1, cls, 0x1a18, 0x10, 0x20),
		new MthdKelvinTexBorderColor(opt, rnd(), "tex_border_color", -1, cls, 0x1a1c, 0x10, 0x20),
		new UntestedMthd(opt, rnd(), "vtx_attr0_4f", -1, cls, 0x1c00, 4), // XXX
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr1_4f", -1, cls, 0x1c10, 4, 0x1),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr2_4f", -1, cls, 0x1c20, 4, 0x2),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr3_4f", -1, cls, 0x1c30, 4, 0x3),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr4_4f", -1, cls, 0x1c40, 4, 0x4),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr5_4f", -1, cls, 0x1c50, 4, 0x5),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr6_4f", -1, cls, 0x1c60, 4, 0x6),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr7_4f", -1, cls, 0x1c70, 4, 0x7),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr8_4f", -1, cls, 0x1c80, 4, 0x8),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr9_4f", -1, cls, 0x1c90, 4, 0x9),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr10_4f", -1, cls, 0x1ca0, 4, 0xa),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr11_4f", -1, cls, 0x1cb0, 4, 0xb),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr12_4f", -1, cls, 0x1cc0, 4, 0xc),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr13_4f", -1, cls, 0x1cd0, 4, 0xd),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr14_4f", -1, cls, 0x1ce0, 4, 0xe),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr15_4f", -1, cls, 0x1cf0, 4, 0xf),
		new MthdKelvinTexColorKey(opt, rnd(), "tex_color_key", -1, cls, 0x1d00, 0x10),
		new MthdRankinePsControl(opt, rnd(), "ps_control", -1, cls, 0x1d60),
		new MthdKelvinStateSave(opt, rnd(), "state_save", -1, cls, 0x1d64),
		new UntestedMthd(opt, rnd(), "state_restore", -1, cls, 0x1d68), // XXX
		new MthdKelvinFenceOffset(opt, rnd(), "fence_offset", -1, cls, 0x1d6c),
		new UntestedMthd(opt, rnd(), "fence_write_a", -1, cls, 0x1d70), // XXX
		new UntestedMthd(opt, rnd(), "fence_write_b", -1, cls, 0x1d74), // XXX
		new MthdKelvinDepthClamp(opt, rnd(), "depth_clamp", -1, cls, 0x1d78),
		new MthdKelvinMultisample(opt, rnd(), "multisample", -1, cls, 0x1d7c),
		new MthdKelvinUnk1d80(opt, rnd(), "unk1d80", -1, cls, 0x1d80),
		new MthdKelvinUnk1d84(opt, rnd(), "unk1d84", -1, cls, 0x1d84),
		new MthdRankineWindowConfig(opt, rnd(), "window_config", -1, cls, 0x1d88),
		new MthdKelvinClearZeta(opt, rnd(), "clear_zeta", -1, cls, 0x1d8c),
		new MthdKelvinClearColor(opt, rnd(), "clear_color", -1, cls, 0x1d90),
		new UntestedMthd(opt, rnd(), "clear_trigger", -1, cls, 0x1d94), // XXX
		new MthdKelvinClearHv(opt, rnd(), "clear_h", -1, cls, 0x1d98, 0),
		new MthdKelvinClearHv(opt, rnd(), "clear_v", -1, cls, 0x1d9c, 1),
		new UntestedMthd(opt, rnd(), "clear_clipid_trigger", -1, cls, 0x1da0), // XXX
		new MthdKelvinClipidEnable(opt, rnd(), "clipid_enable", -1, cls, 0x1da4),
		new MthdKelvinClipidId(opt, rnd(), "clipid_id", -1, cls, 0x1da8),
		new MthdRankinePrimitiveRestartEnable(opt, rnd(), "primitive_restart_enable", -1, cls, 0x1dac),
		new MthdRankinePrimitiveRestartIndex(opt, rnd(), "primitive_restart_index", -1, cls, 0x1db0),
		new MthdKelvinLineStippleEnable(opt, rnd(), "line_stipple_enable", -1, cls, 0x1db4),
		new MthdKelvinLineStipplePattern(opt, rnd(), "line_stipple_pattern", -1, cls, 0x1db8),
		new MthdKelvinUnk1dbc(opt, rnd(), "unk1dbc", -1, cls, 0x1dbc),
		new MthdKelvinUnk1dc0(opt, rnd(), "unk1dc0", -1, cls, 0x1dc0, 4),
		new MthdKelvinMaterialShininessA(opt, rnd(), "material_back_shininess_a", -1, cls, 0x1e20, 1),
		new MthdKelvinMaterialShininessB(opt, rnd(), "material_back_shininess_b", -1, cls, 0x1e24, 1),
		new MthdKelvinMaterialShininessC(opt, rnd(), "material_back_shininess_c", -1, cls, 0x1e28, 1),
		new MthdKelvinMaterialShininessD(opt, rnd(), "material_back_shininess_d", -1, cls, 0x1e2c, 1),
		new MthdKelvinMaterialShininessE(opt, rnd(), "material_back_shininess_e", -1, cls, 0x1e30, 1),
		new MthdKelvinMaterialShininessF(opt, rnd(), "material_back_shininess_f", -1, cls, 0x1e34, 1),
		new UntestedMthd(opt, rnd(), "vtx_attr0_1f", -1, cls, 0x1e40, 1), // XXX
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr1_1f", -1, cls, 0x1e44, 1, 0x1),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr2_1f", -1, cls, 0x1e48, 1, 0x2),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr3_1f", -1, cls, 0x1e4c, 1, 0x3),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr4_1f", -1, cls, 0x1e50, 1, 0x4),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr5_1f", -1, cls, 0x1e54, 1, 0x5),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr6_1f", -1, cls, 0x1e58, 1, 0x6),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr7_1f", -1, cls, 0x1e5c, 1, 0x7),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr8_1f", -1, cls, 0x1e60, 1, 0x8),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr9_1f", -1, cls, 0x1e64, 1, 0x9),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr10_1f", -1, cls, 0x1e68, 1, 0xa),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr11_1f", -1, cls, 0x1e6c, 1, 0xb),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr12_1f", -1, cls, 0x1e70, 1, 0xc),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr13_1f", -1, cls, 0x1e74, 1, 0xd),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr14_1f", -1, cls, 0x1e78, 1, 0xe),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr15_1f", -1, cls, 0x1e7c, 1, 0xf),
		new MthdKelvinXfUnk4(opt, rnd(), "xf_unk4", -1, cls, 0x1e80, 4),
		new UntestedMthd(opt, rnd(), "xf_run_program", -1, cls, 0x1e90), // XXX
		new MthdKelvinTlMode(opt, rnd(), "tl_mode", -1, cls, 0x1e94),
		new MthdKelvinUnk1e98(opt, rnd(), "unk1e98", -1, cls, 0x1e98),
		new MthdKelvinTlProgramLoadPos(opt, rnd(), "tl_program_load_pos", -1, cls, 0x1e9c),
		new MthdKelvinTlProgramStartPos(opt, rnd(), "tl_program_start_pos", -1, cls, 0x1ea0),
		new MthdKelvinPointParamsA(opt, rnd(), "point_params_a", -1, cls, 0x1ec0),
		new MthdKelvinPointParamsB(opt, rnd(), "point_params_b", -1, cls, 0x1ecc),
		new MthdKelvinPointParamsC(opt, rnd(), "point_params_c", -1, cls, 0x1ed8),
		new MthdKelvinPointParamsD(opt, rnd(), "point_params_d", -1, cls, 0x1edc),
		new MthdKelvinPointSize(opt, rnd(), "point_size", -1, cls, 0x1ee0),
		new MthdKelvinPointParamsEnable(opt, rnd(), "point_params_enable", -1, cls, 0x1ee4),
		new MthdKelvinPointSprite(opt, rnd(), "point_sprite", -1, cls, 0x1ee8),
		new MthdKelvinTlParamLoadPos(opt, rnd(), "tl_param_load_pos", -1, cls, 0x1efc),
		new MthdKelvinTlParamLoad(opt, rnd(), "tl_param_load", -1, cls, 0x1f00, 0x20),
		new MthdRankineXfUnk1f80(opt, rnd(), "xf_unk1f80", -1, cls, 0x1f80, 0x10),
	};
	if (cls == 0x497 || cls == 0x3597) {
		res.insert(res.end(), {
			new MthdRankineDepthBoundsEnable(opt, rnd(), "depth_bounds_enable", -1, cls, 0x380),
			new MthdRankineDepthBoundsMin(opt, rnd(), "depth_bounds_min", -1, cls, 0x384),
			new MthdRankineDepthBoundsMax(opt, rnd(), "depth_bounds_max", -1, cls, 0x388),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Curie::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", -1, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", -1, cls, 0x110),
		new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
		new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
		new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
		new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
		new UntestedMthd(opt, rnd(), "flip_unk130", -1, cls, 0x130),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", -1, cls, 0x180),
		new MthdKelvinDmaTex(opt, rnd(), "dma_tex_a", -1, cls, 0x184, 0),
		new MthdKelvinDmaTex(opt, rnd(), "dma_tex_b", -1, cls, 0x188, 1),
		new MthdDmaSurf(opt, rnd(), "dma_surf_color_b", -1, cls, 0x18c, 6, SURF_NV10),
		new MthdKelvinDmaState(opt, rnd(), "dma_state", -1, cls, 0x190),
		new MthdDmaSurf(opt, rnd(), "dma_surf_color_a", -1, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", -1, cls, 0x198, 3, SURF_NV10),
		new MthdKelvinDmaVtx(opt, rnd(), "dma_vtx_a", -1, cls, 0x19c, 0),
		new MthdKelvinDmaVtx(opt, rnd(), "dma_vtx_b", -1, cls, 0x1a0, 1),
		new MthdDmaGrobj(opt, rnd(), "dma_fence", -1, cls, 0x1a4, 0, DMA_W | DMA_FENCE),
		new MthdDmaGrobj(opt, rnd(), "dma_query", -1, cls, 0x1a8, 1, DMA_W),
		new MthdKelvinDmaClipid(opt, rnd(), "dma_clipid", -1, cls, 0x1ac),
		new MthdKelvinDmaZcull(opt, rnd(), "dma_zcull", -1, cls, 0x1b0),
		new UntestedMthd(opt, rnd(), "dma_surf_color_c", -1, cls, 0x1b4), // XXX
		new UntestedMthd(opt, rnd(), "dma_surf_color_d", -1, cls, 0x1b8), // XXX
		new MthdKelvinClip(opt, rnd(), "clip_h", -1, cls, 0x200, 0, 0),
		new MthdKelvinClip(opt, rnd(), "clip_v", -1, cls, 0x204, 0, 1),
		new MthdSurf3DFormat(opt, rnd(), "surf_format", -1, cls, 0x208, true),
		new UntestedMthd(opt, rnd(), "color_a_pitch", -1, cls, 0x20c), // XXX
		new MthdSurfOffset(opt, rnd(), "color_a_offset", -1, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", -1, cls, 0x214, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_b_offset", -1, cls, 0x218, 6, SURF_NV10),
		new UntestedMthd(opt, rnd(), "color_b_pitch", -1, cls, 0x21c), // XXX
		new MthdRankineRtEnable(opt, rnd(), "rt_enable", -1, cls, 0x220),
		new MthdKelvinSurfPitchClipid(opt, rnd(), "surf_pitch_clipid", -1, cls, 0x224),
		new MthdKelvinSurfOffsetClipid(opt, rnd(), "surf_offset_clipid", -1, cls, 0x228),
		new UntestedMthd(opt, rnd(), "zeta_pitch", -1, cls, 0x22c), // XXX
		new UntestedMthd(opt, rnd(), "unk234", -1, cls, 0x234), // XXX
		new MthdRankineTxcCylwrap(opt, rnd(), "txc_cylwrap", -1, cls, 0x238),
		new UntestedMthd(opt, rnd(), "unk23c", -1, cls, 0x23c), // XXX
		new UntestedMthd(opt, rnd(), "unk260", -1, cls, 0x260, 8), // XXX
		new UntestedMthd(opt, rnd(), "color_c_pitch", -1, cls, 0x280), // XXX
		new UntestedMthd(opt, rnd(), "color_d_pitch", -1, cls, 0x284), // XXX
		new UntestedMthd(opt, rnd(), "color_c_offset", -1, cls, 0x288), // XXX
		new UntestedMthd(opt, rnd(), "color_d_offset", -1, cls, 0x28c), // XXX
		new MthdRankineViewportOffset(opt, rnd(), "viewport_offset", -1, cls, 0x2b8),
		new MthdKelvinClipRectMode(opt, rnd(), "clip_rect_mode", -1, cls, 0x2bc),
		new MthdKelvinClipRect(opt, rnd(), "clip_rect_horiz", -1, cls, 0x2c0, 8, 8, 0),
		new MthdKelvinClipRect(opt, rnd(), "clip_rect_vert", -1, cls, 0x2c4, 8, 8, 1),
		new MthdKelvinDitherEnable(opt, rnd(), "dither_enable", -1, cls, 0x300),
		new MthdKelvinAlphaFuncEnable(opt, rnd(), "alpha_func_enable", -1, cls, 0x304),
		new MthdKelvinAlphaFuncFunc(opt, rnd(), "alpha_func_func", -1, cls, 0x308),
		new MthdKelvinAlphaFuncRef(opt, rnd(), "alpha_func_ref", -1, cls, 0x30c),
		new MthdKelvinBlendFuncEnable(opt, rnd(), "blend_func_enable", -1, cls, 0x310),
		new MthdKelvinBlendFunc(opt, rnd(), "blend_func_src", -1, cls, 0x314, 0),
		new MthdKelvinBlendFunc(opt, rnd(), "blend_func_dst", -1, cls, 0x318, 1),
		new MthdKelvinBlendColor(opt, rnd(), "blend_color", -1, cls, 0x31c),
		new MthdKelvinBlendEquation(opt, rnd(), "blend_equation", -1, cls, 0x320),
		new MthdKelvinColorMask(opt, rnd(), "color_mask", -1, cls, 0x324),
		new MthdKelvinStencilEnable(opt, rnd(), "stencil_enable", -1, cls, 0x328, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_mask", -1, cls, 0x32c, 0, 2),
		new MthdKelvinStencilFunc(opt, rnd(), "stencil_func", -1, cls, 0x330, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_func_ref", -1, cls, 0x334, 0, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_func_mask", -1, cls, 0x338, 0, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_fail", -1, cls, 0x33c, 0, 0),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_zfail", -1, cls, 0x340, 0, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_op_zpass", -1, cls, 0x344, 0, 2),
		new MthdKelvinStencilEnable(opt, rnd(), "stencil_back_enable", -1, cls, 0x348, 1),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_back_mask", -1, cls, 0x34c, 1, 2),
		new MthdKelvinStencilFunc(opt, rnd(), "stencil_back_func", -1, cls, 0x350, 1),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_back_func_ref", -1, cls, 0x354, 1, 0),
		new MthdKelvinStencilVal(opt, rnd(), "stencil_back_func_mask", -1, cls, 0x358, 1, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_back_op_fail", -1, cls, 0x35c, 1, 0),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_back_op_zfail", -1, cls, 0x360, 1, 1),
		new MthdKelvinStencilOp(opt, rnd(), "stencil_back_op_zpass", -1, cls, 0x364, 1, 2),
		new MthdKelvinShadeMode(opt, rnd(), "shade_mode", -1, cls, 0x368),
		new UntestedMthd(opt, rnd(), "unk36c", -1, cls, 0x36c), // XXX
		new UntestedMthd(opt, rnd(), "color_mask_mrt", -1, cls, 0x370), // XXX
		new MthdKelvinColorLogicOpEnable(opt, rnd(), "color_logic_op_enable", -1, cls, 0x374),
		new MthdKelvinColorLogicOpOp(opt, rnd(), "color_logic_op_op", -1, cls, 0x378),
		new UntestedMthd(opt, rnd(), "unk37c", -1, cls, 0x37c), // XXX
		new MthdRankineDepthBoundsEnable(opt, rnd(), "depth_bounds_enable", -1, cls, 0x380),
		new MthdRankineDepthBoundsMin(opt, rnd(), "depth_bounds_min", -1, cls, 0x384),
		new MthdRankineDepthBoundsMax(opt, rnd(), "depth_bounds_max", -1, cls, 0x388),
		new UntestedMthd(opt, rnd(), "unk38c", -1, cls, 0x38c), // XXX
		new MthdKelvinDepthRangeNear(opt, rnd(), "depth_range_near", -1, cls, 0x394),
		new MthdKelvinDepthRangeFar(opt, rnd(), "depth_range_far", -1, cls, 0x398),
		new MthdKelvinConfig(opt, rnd(), "config", -1, cls, 0x3b0),
		new MthdKelvinLineWidth(opt, rnd(), "line_width", -1, cls, 0x3b8),
		new MthdKelvinLineSmoothEnable(opt, rnd(), "line_smooth_enable", -1, cls, 0x3bc),
		new MthdKelvinClip(opt, rnd(), "scissor_h", -1, cls, 0x8c0, 2, 0),
		new MthdKelvinClip(opt, rnd(), "scissor_v", -1, cls, 0x8c4, 2, 1),
		new MthdKelvinFogMode(opt, rnd(), "fog_mode", -1, cls, 0x8cc),
		new MthdKelvinFogCoeff(opt, rnd(), "fog_coeff", -1, cls, 0x8d0, 3),
		new UntestedMthd(opt, rnd(), "unk8e0", -1, cls, 0x8e0), // XXX
		new UntestedMthd(opt, rnd(), "ps_offset", -1, cls, 0x8e4), // XXX
		new UntestedMthd(opt, rnd(), "xf_tex_offset", -1, cls, 0x900, 4, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "xf_tex_format", -1, cls, 0x904, 4, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "xf_tex_wrap", -1, cls, 0x908, 4, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "xf_tex_control", -1, cls, 0x90c, 4, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "xf_tex_swizzle", -1, cls, 0x910, 4, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "xf_tex_filter", -1, cls, 0x914, 4, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "xf_tex_rect", -1, cls, 0x918, 4, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "xf_tex_border_color", -1, cls, 0x91c, 4, 0x20), // XXX
		new MthdKelvinClip(opt, rnd(), "viewport_h", -1, cls, 0xa00, 1, 0),
		new MthdKelvinClip(opt, rnd(), "viewport_v", -1, cls, 0xa04, 1, 1),
		new MthdKelvinUnka0c(opt, rnd(), "unka0c", -1, cls, 0xa0c),
		new UntestedMthd(opt, rnd(), "unka1c", -1, cls, 0xa1c), // XXX
		new MthdKelvinViewportTranslate(opt, rnd(), "viewport_translate", -1, cls, 0xa20),
		new MthdKelvinViewportScale(opt, rnd(), "viewport_scale", -1, cls, 0xa30),
		new MthdKelvinPolygonOffsetPointEnable(opt, rnd(), "polygon_offset_point_enable", -1, cls, 0xa60),
		new MthdKelvinPolygonOffsetLineEnable(opt, rnd(), "polygon_offset_line_enable", -1, cls, 0xa64),
		new MthdKelvinPolygonOffsetFillEnable(opt, rnd(), "polygon_offset_fill_enable", -1, cls, 0xa68),
		new MthdKelvinDepthFunc(opt, rnd(), "depth_func", -1, cls, 0xa6c),
		new MthdKelvinDepthWriteEnable(opt, rnd(), "depth_write_enable", -1, cls, 0xa70),
		new MthdKelvinDepthTestEnable(opt, rnd(), "depth_test_enable", -1, cls, 0xa74),
		new MthdKelvinPolygonOffsetFactor(opt, rnd(), "polygon_offset_factor", -1, cls, 0xa78),
		new MthdKelvinPolygonOffsetUnits(opt, rnd(), "polygon_offset_units", -1, cls, 0xa7c),
		new UntestedMthd(opt, rnd(), "vtx_attr0_4ns", -1, cls, 0xa80, 2), // XXX
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr1_4ns", -1, cls, 0xa88, 4, 0x1),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr2_4ns", -1, cls, 0xa90, 4, 0x2),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr3_4ns", -1, cls, 0xa98, 4, 0x3),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr4_4ns", -1, cls, 0xaa0, 4, 0x4),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr5_4ns", -1, cls, 0xaa8, 4, 0x5),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr6_4ns", -1, cls, 0xab0, 4, 0x6),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr7_4ns", -1, cls, 0xab8, 4, 0x7),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr8_4ns", -1, cls, 0xac0, 4, 0x8),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr9_4ns", -1, cls, 0xac8, 4, 0x9),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr10_4ns", -1, cls, 0xad0, 4, 0xa),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr11_4ns", -1, cls, 0xad8, 4, 0xb),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr12_4ns", -1, cls, 0xae0, 4, 0xc),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr13_4ns", -1, cls, 0xae8, 4, 0xd),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr14_4ns", -1, cls, 0xaf0, 4, 0xe),
		new MthdKelvinVtxAttrNShortFree(opt, rnd(), "vtx_attr15_4ns", -1, cls, 0xaf8, 4, 0xf),
		new MthdRankineTexFilterOpt(opt, rnd(), "tex_filter_opt", -1, cls, 0xb00, 0x10),
		new UntestedMthd(opt, rnd(), "unkb40", -1, cls, 0xb40, 0xa), // XXX
		new MthdKelvinTlProgramLoad(opt, rnd(), "tl_program_load", -1, cls, 0xb80, 0x20),
		new MthdKelvinSpecularEnable(opt, rnd(), "specular_enable", -1, cls, 0x1428),
		new MthdKelvinLightTwoSideEnable(opt, rnd(), "light_two_side_enable", -1, cls, 0x142c),
		new UntestedMthd(opt, rnd(), "unk1430", -1, cls, 0x1430, 2), // XXX
		new UntestedMthd(opt, rnd(), "unk1438", -1, cls, 0x1438), // XXX
		new MthdKelvinUnk3f0(opt, rnd(), "unk3f0", -1, cls, 0x1450),
		new MthdKelvinFlatshadeFirst(opt, rnd(), "flatshade_first", -1, cls, 0x1454),
		new MthdKelvinEdgeFlag(opt, rnd(), "vtx_edge_flag", -1, cls, 0x145c),
		new MthdRankineClipPlaneEnable(opt, rnd(), "clip_plane_enable", -1, cls, 0x1478),
		new MthdKelvinPolygonStippleEnable(opt, rnd(), "polygon_stipple_enable", -1, cls, 0x147c),
		new MthdKelvinPolygonStipple(opt, rnd(), "polygon_stipple", -1, cls, 0x1480, 0x20),
		new UntestedMthd(opt, rnd(), "vtx_attr0_3f", -1, cls, 0x1500, 3), // XXX
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr1_3f", -1, cls, 0x1510, 3, 0x1),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr2_3f", -1, cls, 0x1520, 3, 0x2),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr3_3f", -1, cls, 0x1530, 3, 0x3),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr4_3f", -1, cls, 0x1540, 3, 0x4),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr5_3f", -1, cls, 0x1550, 3, 0x5),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr6_3f", -1, cls, 0x1560, 3, 0x6),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr7_3f", -1, cls, 0x1570, 3, 0x7),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr8_3f", -1, cls, 0x1580, 3, 0x8),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr9_3f", -1, cls, 0x1590, 3, 0x9),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr10_3f", -1, cls, 0x15a0, 3, 0xa),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr11_3f", -1, cls, 0x15b0, 3, 0xb),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr12_3f", -1, cls, 0x15c0, 3, 0xc),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr13_3f", -1, cls, 0x15d0, 3, 0xd),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr14_3f", -1, cls, 0x15e0, 3, 0xe),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr15_3f", -1, cls, 0x15f0, 3, 0xf),
		new MthdKelvinVtxbufOffset(opt, rnd(), "vtxbuf_offset", -1, cls, 0x1680, 0x10),
		new MthdKelvinUnkcf0(opt, rnd(), "unkcf0", -1, cls, 0x1710), // XXX
		new MthdKelvinUnkcf4(opt, rnd(), "unkcf4", -1, cls, 0x1714), // XXX
		new MthdKelvinXfNop(opt, rnd(), "xf_nop", -1, cls, 0x1718),
		new MthdKelvinXfSync(opt, rnd(), "xf_sync", -1, cls, 0x171c),
		new UntestedMthd(opt, rnd(), "unk1738", -1, cls, 0x1738), // XXX
		new UntestedMthd(opt, rnd(), "vb_element_base", -1, cls, 0x173c), // XXX
		new MthdKelvinVtxbufFormat(opt, rnd(), "vtxbuf_format", -1, cls, 0x1740, 0x10),
		new MthdKelvinZPassCounterReset(opt, rnd(), "zpass_counter_reset", -1, cls, 0x17c8),
		new MthdKelvinZPassCounterEnable(opt, rnd(), "zpass_counter_enable", -1, cls, 0x17cc),
		new UntestedMthd(opt, rnd(), "zpass_counter_read", -1, cls, 0x1800), // XXX
		new UntestedMthd(opt, rnd(), "unk1804", -1, cls, 0x1804), // XXX
		new UntestedMthd(opt, rnd(), "begin", -1, cls, 0x1808), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx16.data", -1, cls, 0x180c), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx32.data", -1, cls, 0x1810), // XXX
		new UntestedMthd(opt, rnd(), "draw_arrays.data", -1, cls, 0x1814), // XXX
		new UntestedMthd(opt, rnd(), "draw_inline.data", -1, cls, 0x1818), // XXX
		new MthdRankineIdxbufOffset(opt, rnd(), "idxbuf_offset", -1, cls, 0x181c),
		new MthdRankineIdxbufFormat(opt, rnd(), "idxbuf_format", -1, cls, 0x1820),
		new UntestedMthd(opt, rnd(), "draw_index.data", -1, cls, 0x1824), // XXX
		new MthdKelvinPolygonMode(opt, rnd(), "polygon_mode_front", -1, cls, 0x1828, 0),
		new MthdKelvinPolygonMode(opt, rnd(), "polygon_mode_back", -1, cls, 0x182c, 1),
		new MthdKelvinCullFace(opt, rnd(), "cull_face", -1, cls, 0x1830),
		new MthdKelvinFrontFace(opt, rnd(), "front_face", -1, cls, 0x1834),
		new MthdKelvinPolygonSmoothEnable(opt, rnd(), "polygon_smooth_enable", -1, cls, 0x1838),
		new MthdKelvinCullFaceEnable(opt, rnd(), "cull_face_enable", -1, cls, 0x183c),
		new MthdKelvinTexPalette(opt, rnd(), "tex_palette", -1, cls, 0x1840, 0x10),
		new UntestedMthd(opt, rnd(), "vtx_attr0_2f", -1, cls, 0x1880, 2), // XXX
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr1_2f", -1, cls, 0x1888, 2, 0x1),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr2_2f", -1, cls, 0x1890, 2, 0x2),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr3_2f", -1, cls, 0x1898, 2, 0x3),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr4_2f", -1, cls, 0x18a0, 2, 0x4),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr5_2f", -1, cls, 0x18a8, 2, 0x5),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr6_2f", -1, cls, 0x18b0, 2, 0x6),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr7_2f", -1, cls, 0x18b8, 2, 0x7),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr8_2f", -1, cls, 0x18c0, 2, 0x8),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr9_2f", -1, cls, 0x18c8, 2, 0x9),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr10_2f", -1, cls, 0x18d0, 2, 0xa),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr11_2f", -1, cls, 0x18d8, 2, 0xb),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr12_2f", -1, cls, 0x18e0, 2, 0xc),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr13_2f", -1, cls, 0x18e8, 2, 0xd),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr14_2f", -1, cls, 0x18f0, 2, 0xe),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr15_2f", -1, cls, 0x18f8, 2, 0xf),
		new UntestedMthd(opt, rnd(), "vtx_attr0_2s", -1, cls, 0x1900), // XXX
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr1_2s", -1, cls, 0x1904, 2, 0x1),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr2_2s", -1, cls, 0x1908, 2, 0x2),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr3_2s", -1, cls, 0x190c, 2, 0x3),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr4_2s", -1, cls, 0x1910, 2, 0x4),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr5_2s", -1, cls, 0x1914, 2, 0x5),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr6_2s", -1, cls, 0x1918, 2, 0x6),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr7_2s", -1, cls, 0x191c, 2, 0x7),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr8_2s", -1, cls, 0x1920, 2, 0x8),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr9_2s", -1, cls, 0x1924, 2, 0x9),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr10_2s", -1, cls, 0x1928, 2, 0xa),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr11_2s", -1, cls, 0x192c, 2, 0xb),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr12_2s", -1, cls, 0x1930, 2, 0xc),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr13_2s", -1, cls, 0x1934, 2, 0xd),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr14_2s", -1, cls, 0x1938, 2, 0xe),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr15_2s", -1, cls, 0x193c, 2, 0xf),
		new UntestedMthd(opt, rnd(), "vtx_attr0_4ub", -1, cls, 0x1940), // XXX
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr1_4ub", -1, cls, 0x1944, 4, 0x1),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr2_4ub", -1, cls, 0x1948, 4, 0x2),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr3_4ub", -1, cls, 0x194c, 4, 0x3),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr4_4ub", -1, cls, 0x1950, 4, 0x4),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr5_4ub", -1, cls, 0x1954, 4, 0x5),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr6_4ub", -1, cls, 0x1958, 4, 0x6),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr7_4ub", -1, cls, 0x195c, 4, 0x7),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr8_4ub", -1, cls, 0x1960, 4, 0x8),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr9_4ub", -1, cls, 0x1964, 4, 0x9),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr10_4ub", -1, cls, 0x1968, 4, 0xa),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr11_4ub", -1, cls, 0x196c, 4, 0xb),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr12_4ub", -1, cls, 0x1970, 4, 0xc),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr13_4ub", -1, cls, 0x1974, 4, 0xd),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr14_4ub", -1, cls, 0x1978, 4, 0xe),
		new MthdKelvinVtxAttrUByteFree(opt, rnd(), "vtx_attr15_4ub", -1, cls, 0x197c, 4, 0xf),
		new UntestedMthd(opt, rnd(), "vtx_attr0_4s", -1, cls, 0x1980, 2), // XXX
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr1_4s", -1, cls, 0x1988, 4, 0x1),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr2_4s", -1, cls, 0x1990, 4, 0x2),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr3_4s", -1, cls, 0x1998, 4, 0x3),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr4_4s", -1, cls, 0x19a0, 4, 0x4),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr5_4s", -1, cls, 0x19a8, 4, 0x5),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr6_4s", -1, cls, 0x19b0, 4, 0x6),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr7_4s", -1, cls, 0x19b8, 4, 0x7),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr8_4s", -1, cls, 0x19c0, 4, 0x8),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr9_4s", -1, cls, 0x19c8, 4, 0x9),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr10_4s", -1, cls, 0x19d0, 4, 0xa),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr11_4s", -1, cls, 0x19d8, 4, 0xb),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr12_4s", -1, cls, 0x19e0, 4, 0xc),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr13_4s", -1, cls, 0x19e8, 4, 0xd),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr14_4s", -1, cls, 0x19f0, 4, 0xe),
		new MthdKelvinVtxAttrShortFree(opt, rnd(), "vtx_attr15_4s", -1, cls, 0x19f8, 4, 0xf),
		new MthdKelvinTexOffset(opt, rnd(), "tex_offset", -1, cls, 0x1a00, 0x10, 0x20),
		new MthdKelvinTexFormat(opt, rnd(), "tex_format", -1, cls, 0x1a04, 0x10, 0x20),
		new MthdRankineTexWrap(opt, rnd(), "tex_wrap", -1, cls, 0x1a08, 0x10, 0x20),
		new MthdKelvinTexControl(opt, rnd(), "tex_control", -1, cls, 0x1a0c, 0x10, 0x20),
		new MthdKelvinTexPitch(opt, rnd(), "tex_pitch", -1, cls, 0x1a10, 0x10, 0x20),
		new MthdKelvinTexFilter(opt, rnd(), "tex_filter", -1, cls, 0x1a14, 0x10, 0x20),
		new MthdKelvinTexRect(opt, rnd(), "tex_rect", -1, cls, 0x1a18, 0x10, 0x20),
		new MthdKelvinTexBorderColor(opt, rnd(), "tex_border_color", -1, cls, 0x1a1c, 0x10, 0x20),
		new UntestedMthd(opt, rnd(), "vtx_attr0_4f", -1, cls, 0x1c00, 4), // XXX
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr1_4f", -1, cls, 0x1c10, 4, 0x1),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr2_4f", -1, cls, 0x1c20, 4, 0x2),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr3_4f", -1, cls, 0x1c30, 4, 0x3),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr4_4f", -1, cls, 0x1c40, 4, 0x4),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr5_4f", -1, cls, 0x1c50, 4, 0x5),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr6_4f", -1, cls, 0x1c60, 4, 0x6),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr7_4f", -1, cls, 0x1c70, 4, 0x7),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr8_4f", -1, cls, 0x1c80, 4, 0x8),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr9_4f", -1, cls, 0x1c90, 4, 0x9),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr10_4f", -1, cls, 0x1ca0, 4, 0xa),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr11_4f", -1, cls, 0x1cb0, 4, 0xb),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr12_4f", -1, cls, 0x1cc0, 4, 0xc),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr13_4f", -1, cls, 0x1cd0, 4, 0xd),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr14_4f", -1, cls, 0x1ce0, 4, 0xe),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr15_4f", -1, cls, 0x1cf0, 4, 0xf),
		new MthdKelvinTexColorKey(opt, rnd(), "tex_color_key", -1, cls, 0x1d00, 0x10),
		new MthdRankinePsControl(opt, rnd(), "ps_control", -1, cls, 0x1d60),
		new UntestedMthd(opt, rnd(), "unk1d64", -1, cls, 0x1d64), // XXX
		new MthdKelvinFenceOffset(opt, rnd(), "fence_offset", -1, cls, 0x1d6c),
		new UntestedMthd(opt, rnd(), "fence_write_a", -1, cls, 0x1d70), // XXX
		new UntestedMthd(opt, rnd(), "fence_write_b", -1, cls, 0x1d74), // XXX
		new MthdKelvinDepthClamp(opt, rnd(), "depth_clamp", -1, cls, 0x1d78),
		new MthdKelvinMultisample(opt, rnd(), "multisample", -1, cls, 0x1d7c),
		new MthdKelvinUnk1d80(opt, rnd(), "unk1d80", -1, cls, 0x1d80),
		new MthdKelvinUnk1d84(opt, rnd(), "unk1d84", -1, cls, 0x1d84),
		new MthdRankineWindowConfig(opt, rnd(), "window_config", -1, cls, 0x1d88),
		new MthdKelvinClearZeta(opt, rnd(), "clear_zeta", -1, cls, 0x1d8c),
		new MthdKelvinClearColor(opt, rnd(), "clear_color", -1, cls, 0x1d90),
		new UntestedMthd(opt, rnd(), "clear_trigger", -1, cls, 0x1d94), // XXX
		new MthdKelvinClearHv(opt, rnd(), "clear_h", -1, cls, 0x1d98, 0),
		new MthdKelvinClearHv(opt, rnd(), "clear_v", -1, cls, 0x1d9c, 1),
		new UntestedMthd(opt, rnd(), "clear_clipid_trigger", -1, cls, 0x1da0), // XXX
		new MthdKelvinClipidEnable(opt, rnd(), "clipid_enable", -1, cls, 0x1da4),
		new MthdKelvinClipidId(opt, rnd(), "clipid_id", -1, cls, 0x1da8),
		new MthdRankinePrimitiveRestartEnable(opt, rnd(), "primitive_restart_enable", -1, cls, 0x1dac),
		new MthdRankinePrimitiveRestartIndex(opt, rnd(), "primitive_restart_index", -1, cls, 0x1db0),
		new MthdKelvinLineStippleEnable(opt, rnd(), "line_stipple_enable", -1, cls, 0x1db4),
		new MthdKelvinLineStipplePattern(opt, rnd(), "line_stipple_pattern", -1, cls, 0x1db8),
		new MthdKelvinUnk1dbc(opt, rnd(), "unk1dbc", -1, cls, 0x1dbc),
		new MthdKelvinUnk1dc0(opt, rnd(), "unk1dc0", -1, cls, 0x1dc0, 4),
		new UntestedMthd(opt, rnd(), "vtx_attr0_1f", -1, cls, 0x1e40, 1), // XXX
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr1_1f", -1, cls, 0x1e44, 1, 0x1),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr2_1f", -1, cls, 0x1e48, 1, 0x2),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr3_1f", -1, cls, 0x1e4c, 1, 0x3),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr4_1f", -1, cls, 0x1e50, 1, 0x4),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr5_1f", -1, cls, 0x1e54, 1, 0x5),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr6_1f", -1, cls, 0x1e58, 1, 0x6),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr7_1f", -1, cls, 0x1e5c, 1, 0x7),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr8_1f", -1, cls, 0x1e60, 1, 0x8),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr9_1f", -1, cls, 0x1e64, 1, 0x9),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr10_1f", -1, cls, 0x1e68, 1, 0xa),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr11_1f", -1, cls, 0x1e6c, 1, 0xb),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr12_1f", -1, cls, 0x1e70, 1, 0xc),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr13_1f", -1, cls, 0x1e74, 1, 0xd),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr14_1f", -1, cls, 0x1e78, 1, 0xe),
		new MthdKelvinVtxAttrFloatFree(opt, rnd(), "vtx_attr15_1f", -1, cls, 0x1e7c, 1, 0xf),
		new MthdKelvinTlMode(opt, rnd(), "tl_mode", -1, cls, 0x1e94),
		new UntestedMthd(opt, rnd(), "unk1e98", -1, cls, 0x1e98), // XXX
		new MthdKelvinTlProgramLoadPos(opt, rnd(), "tl_program_load_pos", -1, cls, 0x1e9c),
		new MthdKelvinTlProgramStartPos(opt, rnd(), "tl_program_start_pos", -1, cls, 0x1ea0),
		new UntestedMthd(opt, rnd(), "unk1ea4", -1, cls, 0x1ea4, 3), // XXX
		new MthdKelvinPointSize(opt, rnd(), "point_size", -1, cls, 0x1ee0),
		new MthdKelvinPointParamsEnable(opt, rnd(), "point_params_enable", -1, cls, 0x1ee4),
		new MthdKelvinPointSprite(opt, rnd(), "point_sprite", -1, cls, 0x1ee8),
		new UntestedMthd(opt, rnd(), "unk1eec", -1, cls, 0x1eec, 4), // XXX
		new MthdKelvinTlParamLoadPos(opt, rnd(), "tl_param_load_pos", -1, cls, 0x1efc),
		new MthdKelvinTlParamLoad(opt, rnd(), "tl_param_load", -1, cls, 0x1f00, 0x20),
		new UntestedMthd(opt, rnd(), "unk1fc0", -1, cls, 0x1fc0, 6), // XXX
		new UntestedMthd(opt, rnd(), "tex_cache_ctl", -1, cls, 0x1fd8), // XXX
		new UntestedMthd(opt, rnd(), "unk1fdc", -1, cls, 0x1fdc, 5), // XXX
		new UntestedMthd(opt, rnd(), "xf_attr_in_mask", -1, cls, 0x1ff0), // XXX
		new UntestedMthd(opt, rnd(), "xf_attr_out_mask", -1, cls, 0x1ff4), // XXX
		new UntestedMthd(opt, rnd(), "unk1ff8", -1, cls, 0x1ff8), // XXX
	};
	return res;
}

}
}
