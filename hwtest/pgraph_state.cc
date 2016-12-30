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
#include <vector>
#include <memory>

namespace hwtest {
namespace pgraph {

class BetaRegister : public SimpleMmioRegister {
public:
	BetaRegister(uint32_t addr) :
		SimpleMmioRegister(addr, 0x7f800000, "BETA", &pgraph_state::beta) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		if (val & 0x80000000)
			ref(state) = 0;
		else
			ref(state) = val & mask;
	}
	bool scan_test(int cnum, std::mt19937 &rnd) override {
		for (int i = 0; i < 1000; i++) {
			uint32_t orig = rnd();
			write(cnum, orig);
			uint32_t exp = orig & 0x7f800000;
			if (orig & 0x80000000)
				exp = 0;
			uint32_t real = read(cnum);
			if (real != exp) {
				printf("BETA scan mismatch: orig %08x expected %08x real %08x\n", orig, exp, real);
				return true;
			}
		}
		return false;
	}
};

class BitmapColor0Register : public IndexedMmioRegister<2> {
public:
	BitmapColor0Register(uint32_t addr) :
		IndexedMmioRegister<2>(addr, 0xffffffff, "BITMAP_COLOR", &pgraph_state::bitmap_color, 0) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val;
		insrt(state->valid[0], 17, 1, 1);
	}
};

#define REG(a, m, n, f) res.push_back(std::unique_ptr<Register>(new SimpleMmioRegister(a, m, n, &pgraph_state::f)))
#define IREGF(a, m, n, f, i, x, fx) res.push_back(std::unique_ptr<Register>(new IndexedMmioRegister<x>(a, m, n, &pgraph_state::f, i, fx)))
#define IREG(a, m, n, f, i, x) IREGF(a, m, n, f, i, x, 0)

std::vector<std::unique_ptr<Register>> pgraph_rop_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	if (chipset.card_type < 4) {
		for (int i = 0; i < 2; i++) {
			IREG(0x400600 + i * 8, 0x3fffffff, "PATTERN_MONO_RGB", pattern_mono_rgb, i, 2);
			IREG(0x400604 + i * 8, 0x000000ff, "PATTERN_MONO_A", pattern_mono_a, i, 2);
			IREG(0x400610 + i * 4, 0xffffffff, "PATTERN_MONO_BITMAP", pattern_mono_bitmap, i, 2);
		}
		REG(0x400618, 3, "PATTERN_CONFIG", pattern_config);
		IREG(0x40061c, 0x7fffffff, "BITMAP_COLOR", bitmap_color, 0, 2);
		if (chipset.card_type < 3)
			IREG(0x400620, 0x7fffffff, "BITMAP_COLOR", bitmap_color, 1, 2);
		REG(0x400624, 0xff, "ROP", rop);
		if (chipset.card_type < 3)
			REG(0x400628, 0x7fffffff, "PLANE", plane);
		REG(0x40062c, 0x7fffffff, "CHROMA", chroma);
		if (chipset.card_type < 3)
			res.push_back(std::unique_ptr<Register>(new BetaRegister(0x400630)));
		else
			res.push_back(std::unique_ptr<Register>(new BetaRegister(0x400640)));
	} else if (chipset.card_type < 0x20) {
		for (int i = 0; i < 2; i++) {
			IREG(0x400800 + i * 4, 0xffffffff, "PATTERN_MONO_COLOR", pattern_mono_color, i, 2);
			IREG(0x400808 + i * 4, 0xffffffff, "PATTERN_MONO_BITMAP", pattern_mono_bitmap, i, 2);
		}
		REG(0x400810, 0x13, "PATTERN_CONFIG", pattern_config);
		for (int i = 0; i < 64; i++) {
			IREG(0x400900 + i * 4, 0x00ffffff, "PATTERN_COLOR", pattern_color, i, 64);
		}
		res.push_back(std::unique_ptr<Register>(new BitmapColor0Register(0x400600)));
		REG(0x400604, 0xff, "ROP", rop);
		REG(0x400608, 0x7f800000, "BETA", beta);
		REG(0x40060c, 0xffffffff, "BETA4", beta4);
		REG(0x400814, 0xffffffff, "CHROMA", chroma);
		REG(0x400830, 0x3f3f3f3f, "CTX_FORMAT", ctx_format);
	} else {
		for (int i = 0; i < 2; i++) {
			IREG(0x400b10 + i * 4, 0xffffffff, "PATTERN_MONO_COLOR", pattern_mono_color, i, 2);
			IREG(0x400b18 + i * 4, 0xffffffff, "PATTERN_MONO_BITMAP", pattern_mono_bitmap, i, 2);
		}
		REG(0x400b20, 0x13, "PATTERN_CONFIG", pattern_config);
		for (int i = 0; i < 64; i++) {
			IREG(0x400a00 + i * 4, 0x00ffffff, "PATTERN_COLOR", pattern_color, i, 64);
		}
		res.push_back(std::unique_ptr<Register>(new BitmapColor0Register(0x400814)));
		REG(0x400b00, 0xff, "ROP", rop);
		REG(0x400b04, 0x7f800000, "BETA", beta);
		REG(0x400b08, 0xffffffff, "BETA4", beta4);
		REG(0x40087c, 0xffffffff, "CHROMA", chroma);
		REG(0x400b0c, 0x3f3f3f3f, "CTX_FORMAT", ctx_format);
	}
	return res;
}

class SurfOffsetRegister : public IndexedMmioRegister<6> {
public:
	SurfOffsetRegister(uint32_t addr, int idx, uint32_t mask) :
		IndexedMmioRegister<6>(addr, mask, "SURF_OFFSET", &pgraph_state::surf_offset, idx) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[0], 3, 1, 1);
	}
};

class SurfPitchRegister : public IndexedMmioRegister<5> {
public:
	SurfPitchRegister(uint32_t addr, int idx, uint32_t mask) :
		IndexedMmioRegister<5>(addr, mask, "SURF_PITCH", &pgraph_state::surf_pitch, idx) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[0], 2, 1, 1);
	}
};

class SurfTypeRegister : public SimpleMmioRegister {
public:
	SurfTypeRegister(uint32_t addr, uint32_t mask) :
		SimpleMmioRegister(addr, mask, "SURF_TYPE", &pgraph_state::surf_type) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->unka10, 29, 1, extr(state->debug[4], 2, 1) && !!extr(state->surf_type, 2, 2));
	}
};

std::vector<std::unique_ptr<Register>> pgraph_canvas_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	if (chipset.card_type < 4) {
		uint32_t canvas_mask;
		if (chipset.card_type < 3) {
			canvas_mask = 0x0fff0fff;
			REG(0x400634, 0x01111011, "CANVAS_CONFIG", canvas_config);
			REG(0x400688, 0xffffffff, "CANVAS_MIN", dst_canvas_min);
			REG(0x40068c, canvas_mask, "CANVAS_MAX", dst_canvas_max);
		} else {
			canvas_mask = chipset.is_nv03t ? 0x7fff07ff : 0x3fff07ff;
			REG(0x400550, canvas_mask, "SRC_CANVAS_MIN", src_canvas_min);
			REG(0x400554, canvas_mask, "SRC_CANVAS_MAX", src_canvas_max);
			REG(0x400558, canvas_mask, "DST_CANVAS_MIN", dst_canvas_min);
			REG(0x40055c, canvas_mask, "DST_CANVAS_MAX", dst_canvas_max);
			uint32_t offset_mask = chipset.is_nv03t ? 0x007fffff : 0x003fffff;
			for (int i = 0; i < 4; i++) {
				IREG(0x400630 + i * 4, offset_mask & ~0xf, "SURF_OFFSET", surf_offset, i, 6);
				IREG(0x400650 + i * 4, 0x1ff0, "SURF_PITCH", surf_pitch, i, 5);
			}
			REG(0x4006a8, 0x7777, "SURF_FORMAT", surf_format);
		}
		for (int i = 0; i < 2; i++) {
			IREG(0x400690 + i * 8, canvas_mask, "CLIPRECT_MIN", cliprect_min, i, 2);
			IREG(0x400694 + i * 8, canvas_mask, "CLIPRECT_MAX", cliprect_max, i, 2);
		}
		REG(0x4006a0, 0x113, "CLIPRECT_CTRL", cliprect_ctrl);
	} else if (chipset.card_type < 0x20) {
		uint32_t offset_mask = pgraph_offset_mask(&chipset);
		uint32_t pitch_mask = pgraph_pitch_mask(&chipset);
		for (int i = 0; i < 6; i++) {
			res.push_back(std::unique_ptr<Register>(new SurfOffsetRegister(0x400640 + i * 4, i, offset_mask)));
			IREG(0x400658 + i * 4, offset_mask, "SURF_BASE", surf_base, i, 6);
			IREGF(0x400684 + i * 4, 1 << 31 | offset_mask, "SURF_LIMIT", surf_limit, i, 6, 0xf);
		}
		for (int i = 0; i < 5; i++) {
			res.push_back(std::unique_ptr<Register>(new SurfPitchRegister(0x400670 + i * 4, i, pitch_mask)));
		}
		for (int i = 0; i < 2; i++) {
			IREG(0x40069c + i * 4, 0x0f0f0000, "SURF_SWIZZLE", surf_swizzle, i, 2);
		}
		uint32_t st_mask;
		if (!nv04_pgraph_is_nv15p(&chipset))
			st_mask = 3;
		else if (!nv04_pgraph_is_nv11p(&chipset))
			st_mask = 0x77777703;
		else if (!nv04_pgraph_is_nv17p(&chipset))
			st_mask = 0x77777713;
		else
			st_mask = 0xf77777ff;
		res.push_back(std::unique_ptr<Register>(new SurfTypeRegister(
			chipset.card_type >= 0x10 ? 0x400710 : 0x40070c,
			st_mask)));
		REG(0x400724, 0xffffff, "SURF_FORMAT", surf_format);
		REG(chipset.card_type >= 0x10 ? 0x400714 : 0x400710,
			nv04_pgraph_is_nv17p(&chipset) ? 0x3f731f3f : 0x0f731f3f,
			"CTX_VALID", ctx_valid);
		REG(0x400610, (chipset.card_type < 4 ? 0xe0000000 : 0xf8000000) | offset_mask, "SURF_UNK610", surf_unk610);
		REG(0x400614, 0xc0000000 | offset_mask, "SURF_UNK614", surf_unk614);
	} else {
		uint32_t offset_mask = pgraph_offset_mask(&chipset);
		uint32_t pitch_mask = pgraph_pitch_mask(&chipset);
		for (int i = 0; i < 6; i++) {
			res.push_back(std::unique_ptr<Register>(new SurfOffsetRegister(0x400820 + i * 4, i, offset_mask)));
			IREG(0x400838 + i * 4, offset_mask, "SURF_BASE", surf_base, i, 6);
			IREGF(0x400864 + i * 4, 1 << 31 | offset_mask, "SURF_LIMIT", surf_limit, i, 6, 0x3f);
		}
		for (int i = 0; i < 5; i++) {
			res.push_back(std::unique_ptr<Register>(new SurfPitchRegister(0x400850 + i * 4, i, pitch_mask)));
		}
		for (int i = 0; i < 2; i++) {
			IREG(0x400818 + i * 4, 0x0f0f0000, "SURF_SWIZZLE", surf_swizzle, i, 2);
		}
		uint32_t st_mask;
		if (chipset.card_type < 0x20) {
			if (!nv04_pgraph_is_nv15p(&chipset))
				st_mask = 3;
			else if (!nv04_pgraph_is_nv11p(&chipset))
				st_mask = 0x77777703;
			else if (!nv04_pgraph_is_nv17p(&chipset))
				st_mask = 0x77777713;
			else
				st_mask = 0xf77777ff;
		} else if (!nv04_pgraph_is_nv25p(&chipset))
			st_mask = 0x77777733;
		else
			st_mask = 0x77777773;
		res.push_back(std::unique_ptr<Register>(new SurfTypeRegister(
			chipset.card_type >= 0x10 ? 0x400710 : 0x40070c,
			st_mask)));
		REG(0x400724, 0xffffff, "SURF_FORMAT", surf_format);
		REG(chipset.card_type >= 0x10 ? 0x400714 : 0x400710,
			nv04_pgraph_is_nv25p(&chipset) ? 0x0f733f7f : nv04_pgraph_is_nv17p(&chipset) ? 0x3f731f3f : 0x0f731f3f,
			"CTX_VALID", ctx_valid);
		REG(0x400800, 0xfff31f1f, "SURF_UNK800", surf_unk800);
		REG(0x40080c, 0xfffffffc, "SURF_UNK80C", surf_unk80c);
		REG(0x400810, 0xfffffffc, "SURF_UNK810", surf_unk810);
	}
	return res;
}

class XyClipRegister : public MmioRegister {
public:
	int xy;
	int idx;
	XyClipRegister(int xy, int idx) :
		MmioRegister(0x400524 + xy * 8 + idx * 4, 0xffffffff), xy(xy), idx(idx) {}
	std::string name() override {
		return "XY_CLIP[" + std::to_string(xy) + "][" + std::to_string(idx) + "]";
	}
	uint32_t &ref(struct pgraph_state *state) override { return state->xy_clip[xy][idx]; }
};

class Misc32_0Register : public IndexedMmioRegister<4> {
public:
	Misc32_0Register() :
		IndexedMmioRegister<4>(0x40050c, 0xffffffff, "MISC32", &pgraph_state::misc32, 0) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[0], 16, 1, 1);
	}
};

std::vector<std::unique_ptr<Register>> pgraph_vstate_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	if (chipset.card_type < 3) {
		REG(0x400640, 0xf1ff11ff, "XY_A", xy_a);
		IREG(0x400644, 0x03177331, "XY_B", xy_misc_1, 0, 2);
		IREG(0x400648, 0x30ffffff, "XY_D", xy_misc_4, 0, 2);
		IREG(0x40064c, 0x30ffffff, "XY_D", xy_misc_4, 1, 2);
		IREG(0x400650, 0x111ff1ff, "VALID", valid, 0, 2);
		IREG(0x400654, 0xffffffff, "MISC32", misc32, 0, 4);
		REG(0x400658, 0xffff00ff, "SUBDIVIDE", subdivide);
		REG(0x40065c, 0xffff0113, "XY_E", edgefill);
	} else {
		uint32_t xy_a_mask = 0xf013ffff;
		if (chipset.card_type >= 0x10)
			xy_a_mask |= 0x01000000;
		uint32_t xy_b_mask = chipset.card_type >= 4 ? 0x00111031 : 0x0f177331;
		REG(0x400514, xy_a_mask, "XY_A", xy_a);
		IREG(0x400518, xy_b_mask, "XY_B", xy_misc_1, 0, 2);
		IREG(0x40051c, xy_b_mask, "XY_B", xy_misc_1, 1, 2);
		REG(0x400520, 0x7f7f1111, "XY_C", xy_misc_3);
		IREG(0x400500, 0x300000ff, "XY_D", xy_misc_4, 0, 2);
		IREG(0x400504, 0x300000ff, "XY_D", xy_misc_4, 1, 2);
		res.push_back(std::unique_ptr<Register>(new XyClipRegister(0, 0)));
		res.push_back(std::unique_ptr<Register>(new XyClipRegister(0, 1)));
		res.push_back(std::unique_ptr<Register>(new XyClipRegister(1, 0)));
		res.push_back(std::unique_ptr<Register>(new XyClipRegister(1, 1)));
		IREG(0x400510, 0x00ffffff, "MISC24", misc24, 0, 3);
		IREG(0x400570, 0x00ffffff, "MISC24", misc24, 1, 3);
		if (chipset.card_type >= 4)
			IREG(0x400574, 0x00ffffff, "MISC24", misc24, 2, 3);
		res.push_back(std::unique_ptr<Register>(new Misc32_0Register()));
		if (chipset.card_type < 4) {
			IREG(0x40054c, 0xffffffff, "MISC32", misc32, 1, 4);
		} else {
			IREG(0x40057c, 0xffffffff, "MISC32", misc32, 1, 4);
			IREG(0x400580, 0xffffffff, "MISC32", misc32, 2, 4);
			IREG(0x400584, 0xffffffff, "MISC32", misc32, 3, 4);
			if (chipset.card_type < 0x10) {
				REG(0x400760, 0xffffffff, "DMA_PITCH", dma_pitch);
				REG(0x400764, 0x0000033f, "DVD_FORMAT", dvd_format);
				REG(0x400768, 0x01030000, "SIFM_MODE", sifm_mode);
			} else {
				REG(0x400770, 0xffffffff, "DMA_PITCH", dma_pitch);
				REG(0x400774, 0x0000033f, "DVD_FORMAT", dvd_format);
				REG(0x400778, 0x01030000, "SIFM_MODE", sifm_mode);
				REG(0x400588, 0x0000ffff, "TFC_UNK588", unk588);
				REG(0x40058c, 0x0001ffff, "TFC_UNK58C", unk58c);
			}
		}
		if (chipset.card_type < 4) {
			IREG(0x400508, 0xffffffff, "VALID", valid, 0, 2);
		} else {
			uint32_t v1_mask = chipset.card_type < 0x10 ? 0x1fffffff : 0xdfffffff;
			IREG(0x400508, 0xf07fffff, "VALID", valid, 0, 2);
			if (chipset.card_type < 0x20)
				IREG(0x400578, v1_mask, "VALID", valid, 1, 2);
		}
	}
	return res;
}

class D3D0ConfigRegister : public SimpleMmioRegister {
public:
	D3D0ConfigRegister() :
		SimpleMmioRegister(0x400644, 0xf77fbdf3, "D3D0_CONFIG", &pgraph_state::d3d0_config) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[0], 26, 1, 1);
	}
};

std::vector<std::unique_ptr<Register>> pgraph_d3d0_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	REG(0x4005c0, 0xffffffff, "D3D0_TLV_XY", d3d0_tlv_xy);
	REG(0x4005c4, 0xffffffff, "D3D0_TLV_UV", d3d0_tlv_uv);
	REG(0x4005c8, 0x0000ffff, "D3D0_TLV_Z", d3d0_tlv_z);
	REG(0x4005cc, 0xffffffff, "D3D0_TLV_COLOR", d3d0_tlv_color);
	REG(0x4005d0, 0xffffffff, "D3D0_TLV_FOG_TRI", d3d0_tlv_fog_tri_col1);
	REG(0x4005d4, 0xffffffff, "D3D0_TLV_RHW", d3d0_tlv_rhw);
	res.push_back(std::unique_ptr<Register>(new D3D0ConfigRegister()));
	REG(0x4006c8, 0x00000fff, "D3D0_ALPHA", d3d0_alpha);
	for (int i = 0; i < 16; i++) {
		IREG(0x400580 + i * 4, 0xffffff, "VTX_Z", vtx_z, i, 16);
	}
	return res;
}

class D3D56RcAlphaRegister : public IndexedMmioRegister<2> {
public:
	D3D56RcAlphaRegister(int idx) :
		IndexedMmioRegister<2>(0x400590 + idx * 8, 0xfd1d1d1d, "D3D56_RC_ALPHA", &pgraph_state::d3d56_rc_alpha, idx) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[1], 28 - idx * 2, 1, 1);
	}
};

class D3D56RcColorRegister : public IndexedMmioRegister<2> {
public:
	D3D56RcColorRegister(int idx) :
		IndexedMmioRegister<2>(0x400594 + idx * 8, 0xff1f1f1f, "D3D56_RC_COLOR", &pgraph_state::d3d56_rc_color, idx) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[1], 27 - idx * 2, 1, 1);
	}
};

class D3D56TlvUvRegister : public MmioRegister {
public:
	int idx;
	int uv;
	D3D56TlvUvRegister(int idx, int uv) :
		MmioRegister(0x4005c4 + idx * 8 + uv * 4, 0xffffffc0), idx(idx), uv(uv) {}
	std::string name() override {
		return "D3D56_TLV_UV[" + std::to_string(idx) + "][" + std::to_string(uv) + "]";
	}
	uint32_t &ref(struct pgraph_state *state) override { return state->d3d56_tlv_uv[idx][uv]; }
};

class D3D56ConfigRegister : public SimpleMmioRegister {
public:
	D3D56ConfigRegister() :
		SimpleMmioRegister(0x400818, 0xffff5fff, "D3D56_CONFIG", &pgraph_state::d3d56_config) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[1], 17, 1, 1);
	}
};

class D3D56StencilFuncRegister : public SimpleMmioRegister {
public:
	D3D56StencilFuncRegister() :
		SimpleMmioRegister(0x40081c, 0xfffffff1, "D3D56_STENCIL_FUNC", &pgraph_state::d3d56_stencil_func) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[1], 18, 1, 1);
	}
};

class D3D56StencilOpRegister : public SimpleMmioRegister {
public:
	D3D56StencilOpRegister() :
		SimpleMmioRegister(0x400820, 0x00000fff, "D3D56_STENCIL_OP", &pgraph_state::d3d56_stencil_op) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[1], 19, 1, 1);
	}
};

class D3D56BlendRegister : public SimpleMmioRegister {
public:
	D3D56BlendRegister() :
		SimpleMmioRegister(0x400824, 0xff1111ff, "D3D56_BLEND", &pgraph_state::d3d56_blend) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[1], 20, 1, 1);
	}
};

std::vector<std::unique_ptr<Register>> pgraph_d3d56_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	for (int i = 0; i < 2; i++) {
		res.push_back(std::unique_ptr<Register>(new D3D56RcAlphaRegister(i)));
		res.push_back(std::unique_ptr<Register>(new D3D56RcColorRegister(i)));
	}
	for (int i = 0; i < 2; i++) {
		IREG(0x4005a8 + i * 4, 0xfffff7a6, "D3D56_TEX_FORMAT", d3d56_tex_format, i, 2);
		IREG(0x4005b0 + i * 4, 0xffff9e1e, "D3D56_TEX_FILTER", d3d56_tex_filter, i, 2);
	}
	REG(0x4005c0, 0xffffffff, "D3D56_TLV_XY", d3d56_tlv_xy);
	res.push_back(std::unique_ptr<Register>(new D3D56TlvUvRegister(0, 0)));
	res.push_back(std::unique_ptr<Register>(new D3D56TlvUvRegister(0, 1)));
	res.push_back(std::unique_ptr<Register>(new D3D56TlvUvRegister(1, 0)));
	res.push_back(std::unique_ptr<Register>(new D3D56TlvUvRegister(1, 1)));
	REG(0x4005d4, 0xffffffff, "D3D56_TLV_Z", d3d56_tlv_z);
	REG(0x4005d8, 0xffffffff, "D3D56_TLV_COLOR", d3d56_tlv_color);
	REG(0x4005dc, 0xffffffff, "D3D56_TLV_FOG_TRI_COL1", d3d56_tlv_fog_tri_col1);
	REG(0x4005e0, 0xffffffc0, "D3D56_TLV_RHW", d3d56_tlv_rhw);
	res.push_back(std::unique_ptr<Register>(new D3D56ConfigRegister()));
	res.push_back(std::unique_ptr<Register>(new D3D56StencilFuncRegister()));
	res.push_back(std::unique_ptr<Register>(new D3D56StencilOpRegister()));
	res.push_back(std::unique_ptr<Register>(new D3D56BlendRegister()));
	for (int i = 0; i < 16; i++) {
		IREG(0x400d00 + i * 4, 0xffffffc0, "VTX_U", vtx_u, i, 16);
		IREG(0x400d40 + i * 4, 0xffffffc0, "VTX_V", vtx_v, i, 16);
		IREG(0x400d80 + i * 4, 0xffffffc0, "VTX_RHW", vtx_m, i, 16);
	}
	return res;
}

class CelsiusRegister : public MmioRegister {
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		MmioRegister::sim_write(state, val);
		pgraph_celsius_icmd(state, extr(addr, 2, 6), ref(state));
	}
	using MmioRegister::MmioRegister;
};

class SimpleCelsiusRegister : public SimpleMmioRegister {
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		MmioRegister::sim_write(state, val);
		pgraph_celsius_icmd(state, extr(addr, 2, 6), ref(state));
	}
	using SimpleMmioRegister::SimpleMmioRegister;
};

template<int n>
class IndexedCelsiusRegister : public IndexedMmioRegister<n> {
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		MmioRegister::sim_write(state, val);
		uint32_t rval = IndexedMmioRegister<n>::ref(state);
		uint32_t ad = IndexedMmioRegister<n>::addr;
		pgraph_celsius_icmd(state, extr(ad, 2, 6), rval);
	}
	using IndexedMmioRegister<n>::IndexedMmioRegister;
};

class CelsiusRcInRegister : public CelsiusRegister {
public:
	int idx;
	int which;
	CelsiusRcInRegister(int idx, int which) :
		CelsiusRegister(0x400e40 + which * 8 + idx * 4, 0xffffffff), idx(idx), which(which) {}
	std::string name() override {
		return (which ? "CELSIUS_RC_IN_COLOR[" : "CELSIUS_RC_IN_ALPHA[") + std::to_string(idx) + "]";
	}
	uint32_t &ref(struct pgraph_state *state) override { return state->celsius_rc_in[which][idx]; }
};

class CelsiusRcOutRegister : public CelsiusRegister {
public:
	int idx;
	int which;
	CelsiusRcOutRegister(int idx, int which) :
		CelsiusRegister(0x400e58 + which * 8 + idx * 4, which ? idx ? 0x3803ffff : 0x0003ffff : 0x0003cfff), idx(idx), which(which) {}
	std::string name() override {
		return (which ? "CELSIUS_RC_OUT_COLOR[" : "CELSIUS_RC_OUT_ALPHA[") + std::to_string(idx) + "]";
	}
	uint32_t &ref(struct pgraph_state *state) override { return state->celsius_rc_out[which][idx]; }
};

class CelsiusXfMiscBRegister : public SimpleMmioRegister {
public:
	CelsiusXfMiscBRegister() :
		SimpleMmioRegister(0x400f44, 0xffffffff, "CELSIUS_XF_MISC_B", &pgraph_state::celsius_xf_misc_b) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		MmioRegister::sim_write(state, val);
		state->celsius_pipe_junk[2] = state->celsius_xf_misc_a;
		state->celsius_pipe_junk[3] = state->celsius_xf_misc_b;
	}
};

#define CREG(a, m, n, f) res.push_back(std::unique_ptr<Register>(new SimpleCelsiusRegister(a, m, n, &pgraph_state::f)))
#define ICREGF(a, m, n, f, i, x, fx) res.push_back(std::unique_ptr<Register>(new IndexedCelsiusRegister<x>(a, m, n, &pgraph_state::f, i, fx)))
#define ICREG(a, m, n, f, i, x) ICREGF(a, m, n, f, i, x, 0)

std::vector<std::unique_ptr<Register>> pgraph_celsius_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	bool is_nv15p = nv04_pgraph_is_nv15p(&chipset);
	bool is_nv17p = nv04_pgraph_is_nv17p(&chipset);
	for (int i = 0; i < 2; i++) {
		ICREG(0x400e00 + i * 4, 0xffffffff, "CELSIUS_TEX_OFFSET", celsius_tex_offset, i, 2);
		ICREG(0x400e08 + i * 4, 0xffffffc1, "CELSIUS_TEX_PALETTE", celsius_tex_palette, i, 2);
		ICREG(0x400e10 + i * 4, is_nv17p ? 0xffffffde : 0xffffffd6, "CELSIUS_TEX_FORMAT", celsius_tex_format, i, 2);
		ICREG(0x400e18 + i * 4, 0x7fffffff, "CELSIUS_TEX_CONTROL", celsius_tex_control, i, 2);
		ICREG(0x400e20 + i * 4, 0xffff0000, "CELSIUS_TEX_PITCH", celsius_tex_pitch, i, 2);
		ICREG(0x400e28 + i * 4, 0xffffffff, "CELSIUS_TEX_UNK238", celsius_tex_unk238, i, 2);
		ICREG(0x400e30 + i * 4, 0x07ff07ff, "CELSIUS_TEX_RECT", celsius_tex_rect, i, 2);
		ICREG(0x400e38 + i * 4, 0x77001fff, "CELSIUS_TEX_FILTER", celsius_tex_filter, i, 2);
	}
	for (int i = 0; i < 2; i++) {
		res.push_back(std::unique_ptr<Register>(new CelsiusRcInRegister(0, i)));
		res.push_back(std::unique_ptr<Register>(new CelsiusRcInRegister(1, i)));
		ICREG(0x400e50 + i * 4, 0xffffffff, "CELSIUS_RC_FACTOR", celsius_rc_factor, i, 2);
		res.push_back(std::unique_ptr<Register>(new CelsiusRcOutRegister(0, i)));
		res.push_back(std::unique_ptr<Register>(new CelsiusRcOutRegister(1, i)));
	}
	ICREG(0x400e68, 0x3f3f3f3f, "CELSIUS_RC_FINAL", celsius_rc_final, 0, 2);
	ICREG(0x400e6c, 0x3f3f3fe0, "CELSIUS_RC_FINAL", celsius_rc_final, 1, 2);
	CREG(0x400e70, is_nv17p ? 0xbfcf5fff : 0x3fcf5fff, "CELSIUS_CONFIG_A", celsius_config_a);
	CREG(0x400e74, 0xfffffff1, "CELSIUS_STENCIL_FUNC", celsius_stencil_func);
	CREG(0x400e78, 0x00000fff, "CELSIUS_STENCIL_OP", celsius_stencil_op);
	CREG(0x400e7c, is_nv17p ? 0x0000fff5 : 0x00003ff5, "CELSIUS_CONFIG_B", celsius_config_b);
	CREG(0x400e80, is_nv15p ? 0x0001ffff : 0x00000fff, "CELSIUS_BLEND", celsius_blend);
	CREG(0x400e84, 0xffffffff, "CELSIUS_BLEND_COLOR", celsius_blend_color);
	CREG(0x400e88, 0xfcffffcf, "CELSIUS_RASTER", celsius_raster);
	CREG(0x400e8c, 0xffffffff, "CELSIUS_FOG_COLOR", celsius_fog_color);
	CREG(0x400e90, 0xffffffff, "CELSIUS_POLYGON_OFFSET_FACTOR", celsius_polygon_offset_factor);
	CREG(0x400e94, 0xffffffff, "CELSIUS_POLYGON_OFFSET_UNITS", celsius_polygon_offset_units);
	CREG(0x400e98, 0xffffffff, "CELSIUS_DEPTH_RANGE_NEAR", celsius_depth_range_near);
	CREG(0x400e9c, 0xffffffff, "CELSIUS_DEPTH_RANGE_FAR", celsius_depth_range_far);
	ICREG(0x400ea0, 0xffffffff, "CELSIUS_TEX_COLOR_KEY", celsius_tex_color_key, 0, 2);
	ICREG(0x400ea4, 0xffffffff, "CELSIUS_TEX_COLOR_KEY", celsius_tex_color_key, 1, 2);
	CREG(0x400ea8, 0x000001ff, "CELSIUS_POINT_SIZE", celsius_point_size);
	if (is_nv17p) {
		ICREG(0x400eac, 0x0fff0fff, "CELSIUS_CLEAR_HV[0]", celsius_clear_hv, 0, 2);
		ICREG(0x400eb0, 0x0fff0fff, "CELSIUS_CLEAR_HV[1]", celsius_clear_hv, 1, 2);
		CREG(0x400eb4, 0x3fffffff, "CELSIUS_SURF_BASE_ZCULL", celsius_surf_base_zcull);
		CREG(0x400eb8, 0xbfffffff, "CELSIUS_SURF_LIMIT_ZCULL", celsius_surf_limit_zcull);
		CREG(0x400ebc, 0x3fffffff, "CELSIUS_SURF_OFFSET_ZCULL", celsius_surf_offset_zcull);
		CREG(0x400ec0, 0x0000ffff, "CELSIUS_SURF_PITCH_ZCULL", celsius_surf_pitch_zcull);
		CREG(0x400ec4, 0x07ffffff, "CELSIUS_SURF_BASE_CLIPID", celsius_surf_base_clipid);
		CREG(0x400ec8, 0x87ffffff, "CELSIUS_SURF_LIMIT_CLIPID", celsius_surf_limit_clipid);
		CREG(0x400ecc, 0x07ffffff, "CELSIUS_SURF_OFFSET_CLIPID", celsius_surf_offset_clipid);
		CREG(0x400ed0, 0x0000ffff, "CELSIUS_SURF_PITCH_CLIPID", celsius_surf_pitch_clipid);
		CREG(0x400ed4, 0x0000000f, "CELSIUS_CLIPID_ID", celsius_clipid_id);
		CREG(0x400ed8, 0x80000046, "CELSIUS_CONFIG_D", celsius_config_d);
		CREG(0x400edc, 0xffffffff, "CELSIUS_CLEAR_ZETA", celsius_clear_zeta);
		CREG(0x400ee0, 0xffffffff, "CELSIUS_MTHD_UNK3FC", celsius_mthd_unk3fc);
	}
	for (int i = 0; i < 8; i++) {
		IREG(0x400f00 + i * 4, 0x0fff0fff, "CELSIUS_CLIP_RECT_HORIZ", celsius_clip_rect_horiz, i, 8);
		IREG(0x400f20 + i * 4, 0x0fff0fff, "CELSIUS_CLIP_RECT_VERT", celsius_clip_rect_vert, i, 8);
	}
	REG(0x400f40, 0x3bffffff, "CELSIUS_XF_MISC_A", celsius_xf_misc_a);
	res.push_back(std::unique_ptr<Register>(new CelsiusXfMiscBRegister()));
	REG(0x400f48, 0x17ff0117, "CELSIUS_CONFIG_C", celsius_config_c);
	REG(0x400f4c, 0xffffffff, "CELSIUS_DMA", celsius_dma);
	return res;
}

std::vector<std::unique_ptr<Register>> pgraph_dma_nv3_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	uint32_t offset_mask = pgraph_offset_mask(&chipset) | 0xf;
	REG(0x401140, 0x11111, "DMA_INTR_EN", dma_intr_en);
	IREG(0x401210, 0x03010fff, "DMA_ENG_FLAGS", dma_eng_flags, 0, 2);
	IREG(0x401220, 0xffffffff, "DMA_ENG_LIMIT", dma_eng_limit, 0, 2);
	IREG(0x401230, 0xfffff003, "DMA_ENG_PTE", dma_eng_pte, 0, 2);
	IREG(0x401240, 0xfffff000, "DMA_ENG_PTE_TAG", dma_eng_pte_tag, 0, 2);
	IREG(0x401250, 0xffffffff, "DMA_ENG_ADDR_VIRT_ADJ", dma_eng_addr_virt_adj, 0, 2);
	IREG(0x401260, 0xffffffff, "DMA_ENG_ADDR_PHYS", dma_eng_addr_phys, 0, 2);
	IREG(0x401270, offset_mask, "DMA_ENG_BYTES", dma_eng_bytes, 0, 2);
	IREG(0x401280, 0x0000ffff, "DMA_ENG_INST", dma_eng_inst, 0, 2);
	IREG(0x401290, 0x000007ff, "DMA_ENG_LINES", dma_eng_lines, 0, 2);
	REG(0x401400, offset_mask, "DMA_LIN_LIMIT", dma_lin_limit);
	IREG(0x401800, 0xffffffff, "DMA_OFFSET", dma_offset, 0, 3);
	IREG(0x401810, 0xffffffff, "DMA_OFFSET", dma_offset, 1, 3);
	IREG(0x401820, 0xffffffff, "DMA_OFFSET", dma_offset, 2, 3);
	REG(0x401830, 0xffffffff, "DMA_PITCH", dma_pitch);
	REG(0x401840, 0x00000707, "DMA_MISC", dma_misc);
	return res;
}

std::vector<std::unique_ptr<Register>> pgraph_dma_nv4_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	bool is_nv17p = nv04_pgraph_is_nv17p(&chipset) && chipset.card_type < 0x20;
	IREG(0x401000, 0xffffffff, "DMA_OFFSET", dma_offset, 0, 3);
	IREG(0x401004, 0xffffffff, "DMA_OFFSET", dma_offset, 1, 3);
	REG(0x401008, 0x3fffff, "DMA_LENGTH", dma_length);
	REG(0x40100c, 0x77ffff, "DMA_MISC", dma_misc);
	IREG(0x401020, 0xffffffff, "DMA_UNK20", dma_unk20, 0, 2);
	IREG(0x401024, 0xffffffff, "DMA_UNK20", dma_unk20, 1, 2);
	if (is_nv17p)
		REG(0x40103c, 0x1f, "DMA_UNK3C", dma_unk3c);
	if (chipset.card_type >= 0x20)
		REG(0x401038, 0xffffffff, "DMA_UNK38", dma_unk38);
	for (int i = 0; i < 2; i++) {
		IREG(0x401040 + i * 0x40, 0xffff, "DMA_ENG_INST", dma_eng_inst, i, 2);
		IREG(0x401044 + i * 0x40, 0xfff33000, "DMA_ENG_FLAGS", dma_eng_flags, i, 2);
		IREG(0x401048 + i * 0x40, 0xffffffff, "DMA_ENG_LIMIT", dma_eng_limit, i, 2);
		IREG(0x40104c + i * 0x40, 0xfffff002, "DMA_ENG_PTE", dma_eng_pte, i, 2);
		IREG(0x401050 + i * 0x40, 0xfffff000, "DMA_ENG_PTE_TAG", dma_eng_pte_tag, i, 2);
		IREG(0x401054 + i * 0x40, 0xffffffff, "DMA_ENG_ADDR_VIRT_ADJ", dma_eng_addr_virt_adj, i, 2);
		IREG(0x401058 + i * 0x40, 0xffffffff, "DMA_ENG_ADDR_PHYS", dma_eng_addr_phys, i, 2);
		IREG(0x40105c + i * 0x40, 0x1ffffff, "DMA_ENG_BYTES", dma_eng_bytes, i, 2);
		IREG(0x401060 + i * 0x40, 0x7ff, "DMA_ENG_LINES", dma_eng_lines, i, 2);
	}
	return res;
}

}
}

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
	bool is_nv25p = nv04_pgraph_is_nv25p(&state->chipset);
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
	} else {
		// debug[0] holds only resets?
		state->debug[0] = 0;
		state->debug[1] = rnd() & 0x0011f7c1;
		state->debug[3] = rnd() & (is_nv25p ? 0xffffdf7d : 0xffffd77d);
		state->debug[4] = rnd() & (is_nv25p ? 0xffffffff : 0xfffff3ff);
		state->debug[5] = rnd() & 0xff;
		state->debug[6] = rnd();
		state->debug[7] = rnd() & 0xfff;
		state->debug[16] = rnd() & 3;
	}
}

void pgraph_gen_state_control(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	bool is_nv5 = state->chipset.chipset >= 5;
	bool is_nv11p = nv04_pgraph_is_nv11p(&state->chipset);
	bool is_nv15p = nv04_pgraph_is_nv15p(&state->chipset);
	bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
	bool is_nv1720p = is_nv17p || state->chipset.card_type >= 0x20;
	state->intr = 0;
	state->invalid = 0;
	if (state->chipset.card_type < 4) {
		state->intr_en = rnd() & 0x11111011;
		state->invalid_en = rnd() & 0x00011111;
	} else if (state->chipset.card_type < 0x10) {
		state->intr_en = rnd() & 0x11311;
		state->nstatus = rnd() & 0x7800;
	} else {
		if (state->chipset.card_type < 0x20)
			state->intr_en = rnd() & 0x1113711;
		else
			state->intr_en = rnd() & 0x11137d1;
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
		} else {
			ctxs_mask = is_nv11p ? 0x7ffff0ff : 0x7fb3f0ff;
			ctxc_mask = is_nv11p ? 0x7ffff0ff : is_nv15p ? 0x7fb3f0ff : 0x7f33f0ff;
			if (state->chipset.card_type < 0x20)
				state->ctx_user = rnd() & (is_nv15p ? 0x9f00e000 : 0x1f00e000);
			else
				state->ctx_user = rnd() & 0x9f00ff11;
			if (!is_nv15p) {
				state->unk77c = rnd() & 0x1100ffff;
				if (state->unk77c & 0x10000000)
					state->unk77c |= 0x70000000;
			} else if (state->chipset.card_type < 0x20) {
				state->unk77c = rnd() & 0x631fffff;
			} else {
				state->unk77c = rnd() & 0x0100ffff;
			}
			state->unk6b0 = rnd();
			state->unk838 = rnd();
			state->unk83c = rnd();
			state->zcull_unka00[0] = rnd() & 0x1fff1fff;
			state->zcull_unka00[1] = rnd() & 0x1fff1fff;
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
			} else if (!is_nv1720p) {
				state->fifo_mthd[i] = rnd() & 0x01171ffc;
			} else {
				state->fifo_mthd[i] = rnd() & 0x00371ffc;
			}
			state->fifo_data[i][0] = rnd() & 0xffffffff;
			state->fifo_data[i][1] = rnd() & 0xffffffff;
		}
		if (state->fifo_enable) {
			state->fifo_ptr = (rnd() & (is_nv1720p ? 0xf : 7)) * 0x11;
			if (state->chipset.card_type < 0x10) {
				state->fifo_mthd_st2 = rnd() & 0x000ffffe;
			} else {
				state->fifo_mthd_st2 = rnd() & (is_nv1720p ? 0x7bf71ffc : 0x3bf71ffc);
			}
		} else {
			state->fifo_ptr = rnd() & (is_nv1720p ? 0xff : 0x77);
			if (state->chipset.card_type < 0x10) {
				state->fifo_mthd_st2 = rnd() & 0x000fffff;
			} else {
				state->fifo_mthd_st2 = rnd() & (is_nv1720p ? 0x7ff71ffc : 0x3ff71ffc);
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
	}
	for (int i = 0; i < 2; i++) {
		state->iclip[i] = rnd() & 0x3ffff;
		for (int j = 0; j < 3; j++) {
			state->uclip_min[j][i] = rnd() & (state->chipset.card_type < 4 ? 0x3ffff : 0xffff);
			state->uclip_max[j][i] = rnd() & 0x3ffff;
		}
	}
}

using namespace hwtest::pgraph;

void pgraph_gen_state_canvas(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_canvas_regs(state->chipset)) {
		reg->ref(state) = (rnd() & reg->mask) | reg->fixed;
	}
}

void pgraph_gen_state_rop(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_rop_regs(state->chipset)) {
		reg->ref(state) = (rnd() & reg->mask) | reg->fixed;
	}
}

void pgraph_gen_state_vstate(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_vstate_regs(state->chipset)) {
		reg->ref(state) = (rnd() & reg->mask) | reg->fixed;
	}
}

void pgraph_gen_state_dma_nv3(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	state->dma_intr = 0;
	for (auto &reg : pgraph_dma_nv3_regs(state->chipset)) {
		reg->ref(state) = (rnd() & reg->mask) | reg->fixed;
	}
}

void pgraph_gen_state_dma_nv4(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_dma_nv4_regs(state->chipset)) {
		reg->ref(state) = (rnd() & reg->mask) | reg->fixed;
	}
}

void pgraph_gen_state_d3d0(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_d3d0_regs(state->chipset)) {
		reg->ref(state) = (rnd() & reg->mask) | reg->fixed;
	}
}

void pgraph_gen_state_d3d56(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_d3d56_regs(state->chipset)) {
		reg->ref(state) = (rnd() & reg->mask) | reg->fixed;
	}
}

static uint32_t canonical_light_sx_float(uint32_t v) {
	// 0
	if (!extr(v, 23, 8))
		return 0;
	// NaN
	if (extr(v, 23, 8) == 0xff)
		return v & 1 ? 0x7f800000 : 0x7ffffc00;
	return v & 0xfffffc00;
}

static uint32_t canonical_ovtx_fog(uint32_t v) {
	uint32_t res = v & 0xfffff800;
	if (res & 0x800)
		res |= 0x400;
	return res;
}

void pgraph_gen_state_celsius(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_celsius_regs(state->chipset)) {
		reg->ref(state) = (rnd() & reg->mask) | reg->fixed;
		state->celsius_pipe_begin_end = rnd() & 0xf;
		state->celsius_pipe_edge_flag = rnd() & 0x1;
		state->celsius_pipe_unk48 = 0;
		state->celsius_pipe_vtx_state = rnd() & 0x70001f3f;
		for (int i = 0; i < 0x1c; i++)
			state->celsius_pipe_vtx[i] = rnd();
		for (int i = 0; i < 4; i++)
			state->celsius_pipe_junk[i] = rnd();
		for (int i = 0; i < 0x3c; i++)
			for (int j = 0; j < 4; j++) {
				state->celsius_pipe_xfrm[i][j] = rnd();
			}
		for (int i = 0; i < 0x30; i++)
			for (int j = 0; j < 3; j++) {
				state->celsius_pipe_light_v[i][j] = rnd() & 0xfffffc00;
			}
		for (int i = 0; i < 3; i++)
			state->celsius_pipe_light_sa[i] = canonical_light_sx_float(rnd());
		for (int i = 0; i < 19; i++)
			state->celsius_pipe_light_sb[i] = canonical_light_sx_float(rnd());
		for (int i = 0; i < 12; i++)
			state->celsius_pipe_light_sc[i] = canonical_light_sx_float(rnd());
		for (int i = 0; i < 12; i++)
			state->celsius_pipe_light_sd[i] = canonical_light_sx_float(rnd());
		state->celsius_pipe_light_sa[0] = 0x3f800000;
		state->celsius_pipe_light_sb[0] = 0;
		state->celsius_pipe_light_sc[0] = 0x3f800000;
		state->celsius_pipe_light_sd[0] = 0;
		if (state->chipset.chipset == 0x10) {
			state->celsius_pipe_xfrm[59][0] = 0;
			state->celsius_pipe_xfrm[59][1] = 0;
			state->celsius_pipe_xfrm[59][2] = 0;
			state->celsius_pipe_xfrm[59][3] = 0;
			state->celsius_pipe_light_v[47][0] = 0;
			state->celsius_pipe_light_v[47][1] = 0;
			state->celsius_pipe_light_v[47][2] = 0;
			state->celsius_pipe_broke_ovtx = rnd() & 1;
		} else {
			state->celsius_pipe_broke_ovtx = false;
		}
		// XXX unfix this
		if (nv04_pgraph_is_nv17p(&state->chipset))
			state->celsius_pipe_ovtx_pos = 9;
		else
			state->celsius_pipe_ovtx_pos = 11;
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 0x10; j++) {
				state->celsius_pipe_xvtx[i][j] = rnd();
			}
		for (int i = 0; i < 0x10; i++) {
			// XXX clean this.
			state->celsius_pipe_ovtx[i][0] = rnd() & 0xbfffffff;
			state->celsius_pipe_ovtx[i][1] = rnd() & 0xbfffffff;
			state->celsius_pipe_ovtx[i][2] = 0;
			state->celsius_pipe_ovtx[i][3] = rnd() & 0xbfffffff;
			state->celsius_pipe_ovtx[i][4] = rnd() & 0xbfffffff;
			state->celsius_pipe_ovtx[i][5] = rnd() & 0xbfffffff;
			state->celsius_pipe_ovtx[i][6] = rnd() & 0xbfffffff;
			state->celsius_pipe_ovtx[i][7] = rnd() & 0xbfffffff;
			state->celsius_pipe_ovtx[i][8] = rnd();
			state->celsius_pipe_ovtx[i][9] = canonical_light_sx_float(canonical_ovtx_fog(rnd()));
			state->celsius_pipe_ovtx[i][10] = rnd();
			state->celsius_pipe_ovtx[i][11] = rnd() & 0xffffff01;
			state->celsius_pipe_ovtx[i][12] = 0;
			state->celsius_pipe_ovtx[i][13] = 0;
			state->celsius_pipe_ovtx[i][14] = 0;
			state->celsius_pipe_ovtx[i][15] = 0x3f800000;
			insrt(state->celsius_pipe_ovtx[i][2], 31, 1, extr(state->celsius_pipe_ovtx[i][11], 0, 1));
		}
	}
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
	}
}

void pgraph_load_rop(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_rop_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_canvas(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_canvas_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
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
				nva_wr32(cnum, 0x400a00, state->zcull_unka00[0]);
				nva_wr32(cnum, 0x400a04, state->zcull_unka00[1]);
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
	if (state->chipset.card_type < 0x20)
		nva_wr32(cnum, 0x400088, state->debug[2]);
	if (state->chipset.card_type >= 3)
		nva_wr32(cnum, 0x40008c, state->debug[3]);
	if (state->chipset.card_type >= 0x10)
		nva_wr32(cnum, 0x400090, state->debug[4]);
	if (state->chipset.card_type >= 0x20) {
		nva_wr32(cnum, 0x400098, state->debug[6]);
		nva_wr32(cnum, 0x40009c, state->debug[7]);
		if (!nv04_pgraph_is_nv25p(&state->chipset)) {
			nva_wr32(cnum, 0x400094, state->debug[5]);
		} else {
			nva_wr32(cnum, 0x4000c0, state->debug[16]);
		}
	}
}

void pgraph_load_vstate(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_vstate_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_dma_nv3(int cnum, struct pgraph_state *state) {
	nva_wr32(cnum, 0x401100, 0xffffffff);
	for (auto &reg : pgraph_dma_nv3_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_dma_nv4(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_dma_nv4_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_d3d0(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_d3d0_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_d3d56(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_d3d56_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_pipe(int cnum, uint32_t addr, uint32_t *data, int num) {
	nva_wr32(cnum, 0x400f50, addr);
	for (int i = 0; i < num; i++)
		nva_wr32(cnum, 0x400f54, data[i]);
	int ctr = 0;
	while (nva_rd32(cnum, 0x400700)) {
		ctr++;
		if (ctr == 10000) {
			printf("PIPE write hang on %04x: %08x\n", addr, nva_rd32(cnum, 0x400700));
		}
	}
}

void pgraph_load_celsius(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_celsius_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
	nva_wr32(cnum, 0x400084, 0xffffffff);
	nva_wr32(cnum, 0x400088, 0xffffffff);
	nva_wr32(cnum, 0x40008c, 0xffffffff);
	nva_wr32(cnum, 0x400090, 0xffffffff);
	uint32_t cargo_a[8] = {
		0x3f800000,
		0x3f800000,
		0x3f800000,
		0x3f800000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
	};
	uint32_t cargo_b[3] = {
		0x3f800000,
		0x3f800000,
		0x3f800000,
	};
	uint32_t cargo_c[3] = { 0, 0, 0 };
	nva_wr32(cnum, 0x40014c, 0x00000056);
	pgraph_load_pipe(cnum, 0x64c0, cargo_a, 8);
	pgraph_load_pipe(cnum, 0x6ab0, cargo_b, 3);
	pgraph_load_pipe(cnum, 0x6a80, cargo_c, 3);
	nva_wr32(cnum, 0x400f40, 0x10000000);
	nva_wr32(cnum, 0x400f44, 0x00000000);
	uint32_t mode = 0xf;
	pgraph_load_pipe(cnum, 0x0040, &mode, 1);
	for (int i = 0; i < 3; i++)
		pgraph_load_pipe(cnum, 0x0200 + i * 0x40, state->celsius_pipe_xvtx[i], 0x10);
	for (int i = 0; i < 0x10; i++)
		pgraph_load_pipe(cnum, 0x0800 + i * 0x40, state->celsius_pipe_ovtx[i], 0x10);
	uint32_t vtx[4];
	vtx[0] = state->celsius_pipe_begin_end;
	vtx[1] = state->celsius_pipe_edge_flag;
	vtx[2] = state->celsius_pipe_unk48;
	vtx[3] = state->celsius_pipe_vtx_state;
	pgraph_load_pipe(cnum, 0x0040, vtx, 4);
	pgraph_load_pipe(cnum, 0x4400, state->celsius_pipe_vtx, 0x1c);
	for (int i = 0; i < 0x3c; i++) {
		pgraph_load_pipe(cnum, 0x6400 + i * 0x10, state->celsius_pipe_xfrm[i], 4);
	}
	for (int i = 0; i < 0x30; i++) {
		pgraph_load_pipe(cnum, 0x6800 + i * 0x10, state->celsius_pipe_light_v[i], 3);
	}
	for (int i = 0; i < 3; i++) {
		pgraph_load_pipe(cnum, 0x6c00 + i * 0x10, state->celsius_pipe_light_sa + i, 1);
	}
	for (int i = 0; i < 19; i++) {
		pgraph_load_pipe(cnum, 0x7000 + i * 0x10, state->celsius_pipe_light_sb + i, 1);
	}
	for (int i = 0; i < 12; i++) {
		pgraph_load_pipe(cnum, 0x7400 + i * 0x10, state->celsius_pipe_light_sc + i, 1);
	}
	for (int i = 0; i < 12; i++) {
		pgraph_load_pipe(cnum, 0x7800 + i * 0x10, state->celsius_pipe_light_sd + i, 1);
	}
	nva_wr32(cnum, 0x400f40, state->celsius_xf_misc_a);
	nva_wr32(cnum, 0x400f44, state->celsius_xf_misc_b);
	pgraph_load_pipe(cnum, 0x4470, state->celsius_pipe_junk, 0x4);
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
			bool is_nv1720p = is_nv17p || state->chipset.card_type >= 0x20;
			if (!is_nv1720p) {
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
		if (state->chipset.card_type < 0x10)
			nva_wr32(cnum, 0x400160, 0);
		else
			nva_wr32(cnum, 0x40014c, 0);
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
			bool is_nv1720p = is_nv17p || state->chipset.card_type >= 0x20;
			if (!is_nv1720p) {
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
		if (state->chipset.card_type >= 0x10)
			state->unk77c = nva_rd32(cnum, 0x40077c);
		bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
		if (is_nv17p) {
			state->unk6b0 = nva_rd32(cnum, 0x4006b0);
			state->unk838 = nva_rd32(cnum, 0x400838);
			state->unk83c = nva_rd32(cnum, 0x40083c);
			state->zcull_unka00[0] = nva_rd32(cnum, 0x400a00);
			state->zcull_unka00[1] = nva_rd32(cnum, 0x400a04);
			state->unka10 = nva_rd32(cnum, 0x400a10);
		}
	}
}

void pgraph_dump_vstate(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_vstate_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
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
			if (state->chipset.card_type == 0x10) {
				state->uclip_min[2][i] = nva_rd32(cnum, 0x400550 + i * 4);
				state->uclip_max[2][i] = nva_rd32(cnum, 0x400558 + i * 4);
			}
		}
	}
}

void pgraph_dump_rop(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type == 4)
		nva_wr32(cnum, 0x40008c, 0x01000000);
	for (auto &reg : pgraph_rop_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_canvas(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_canvas_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_d3d0(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_d3d0_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_d3d56(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_d3d56_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_pipe(int cnum, uint32_t addr, uint32_t *data, int num) {
	nva_wr32(cnum, 0x400f50, addr);
	for (int i = 0; i < num; i++)
		data[i] = nva_rd32(cnum, 0x400f54);
}

void pgraph_dump_celsius(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_celsius_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_celsius_pipe(int cnum, struct pgraph_state *state) {
	if (state->celsius_pipe_broke_ovtx)
		pgraph_dump_pipe(cnum, 0x4470, state->celsius_pipe_junk, 4);
	for (int i = 0; i < 0x10; i++)
		pgraph_dump_pipe(cnum, 0x0800 + i * 0x40, state->celsius_pipe_ovtx[i], 0x10);
	pgraph_dump_pipe(cnum, 0x4400, state->celsius_pipe_vtx, 0x1c);
	if (!state->celsius_pipe_broke_ovtx)
		pgraph_dump_pipe(cnum, 0x4470, state->celsius_pipe_junk, 4);
	for (int i = 0; i < 3; i++)
		pgraph_dump_pipe(cnum, 0x0200 + i * 0x40, state->celsius_pipe_xvtx[i], 0x10);
	for (int i = 0; i < 0x3c; i++) {
		pgraph_dump_pipe(cnum, 0x6400 + i * 0x10, state->celsius_pipe_xfrm[i], 4);
	}
	for (int i = 0; i < 0x30; i++) {
		pgraph_dump_pipe(cnum, 0x6800 + i * 0x10, state->celsius_pipe_light_v[i], 3);
	}
	for (int i = 0; i < 3; i++) {
		pgraph_dump_pipe(cnum, 0x6c00 + i * 0x10, state->celsius_pipe_light_sa + i, 1);
	}
	for (int i = 0; i < 19; i++) {
		pgraph_dump_pipe(cnum, 0x7000 + i * 0x10, state->celsius_pipe_light_sb + i, 1);
	}
	for (int i = 0; i < 12; i++) {
		pgraph_dump_pipe(cnum, 0x7400 + i * 0x10, state->celsius_pipe_light_sc + i, 1);
	}
	for (int i = 0; i < 12; i++) {
		pgraph_dump_pipe(cnum, 0x7800 + i * 0x10, state->celsius_pipe_light_sd + i, 1);
	}
	uint32_t vtx[4];
	pgraph_dump_pipe(cnum, 0x0040, vtx, 4);
	state->celsius_pipe_begin_end = vtx[0];
	state->celsius_pipe_edge_flag = vtx[1];
	state->celsius_pipe_unk48 = vtx[2];
	state->celsius_pipe_vtx_state = vtx[3];
}

void pgraph_dump_debug(int cnum, struct pgraph_state *state) {
	state->debug[0] = nva_rd32(cnum, 0x400080);
	state->debug[1] = nva_rd32(cnum, 0x400084);
	if (state->chipset.card_type < 0x20)
		state->debug[2] = nva_rd32(cnum, 0x400088);
	if (state->chipset.card_type >= 3)
		state->debug[3] = nva_rd32(cnum, 0x40008c);
	if (state->chipset.card_type >= 0x10)
		state->debug[4] = nva_rd32(cnum, 0x400090);
	if (state->chipset.card_type >= 0x10)
		nva_wr32(cnum, 0x400080, 0);
	if (state->chipset.card_type >= 0x20) {
		state->debug[6] = nva_rd32(cnum, 0x400098);
		state->debug[7] = nva_rd32(cnum, 0x40009c);
		if (!nv04_pgraph_is_nv25p(&state->chipset)) {
			state->debug[5] = nva_rd32(cnum, 0x400094);
		} else {
			state->debug[16] = nva_rd32(cnum, 0x4000c0);
		}
	}
}

void pgraph_dump_dma_nv3(int cnum, struct pgraph_state *state) {
	state->dma_intr = nva_rd32(cnum, 0x401100);
	for (auto &reg : pgraph_dma_nv3_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_dma_nv4(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_dma_nv4_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_state(int cnum, struct pgraph_state *state) {
	state->chipset = nva_cards[cnum]->chipset;
	int ctr = 0;
	bool reset = false;
	while((state->status = nva_rd32(cnum, state->chipset.card_type < 4 ? 0x4006b0: 0x400700))) {
		ctr++;
		if (ctr > 100000) {
			fprintf(stderr, "PGRAPH locked up [%08x]!\n", state->status);
			reset = true;
			break;
		}
	}
	pgraph_dump_debug(cnum, state);
	pgraph_dump_fifo(cnum, state);
	pgraph_dump_control(cnum, state);
	pgraph_dump_vstate(cnum, state);
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

	if (reset) {
		nva_wr32(cnum, 0x000200, 0xffffefff);
		nva_wr32(cnum, 0x000200, 0xffffffff);
	}

	if (state->chipset.card_type == 0x10)
		pgraph_dump_celsius_pipe(cnum, state);

	pgraph_dump_vtx(cnum, state);
}

int pgraph_cmp_state(struct pgraph_state *orig, struct pgraph_state *exp, struct pgraph_state *real, bool broke) {
	bool is_nv17p = nv04_pgraph_is_nv17p(&orig->chipset);
	bool is_nv1720p = is_nv17p || orig->chipset.card_type >= 0x20;
	bool is_nv25p = nv04_pgraph_is_nv25p(&orig->chipset);
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
	if (orig->chipset.card_type < 4) {
		// XXX: figure these out someday
#if 0
		CMP(trap_addr, "TRAP_ADDR")
		CMP(trap_data[0], "TRAP_DATA[0]")
		if (orig->chipset.card_type >= 3) {
			CMP(trap_grctx, "TRAP_GRCTX")
		}
#endif
	} else {
		CMP(trap_addr, "TRAP_ADDR")
		CMP(trap_data[0], "TRAP_DATA[0]")
		if (orig->chipset.card_type >= 0x10) {
			CMP(trap_data[1], "TRAP_DATA[1]")
		}
	}
	CMP(debug[0], "DEBUG[0]")
	CMP(debug[1], "DEBUG[1]")
	if (orig->chipset.card_type < 0x20) {
		CMP(debug[2], "DEBUG[2]")
	}
	if (orig->chipset.card_type >= 3) {
		CMP(debug[3], "DEBUG[3]")
	}
	if (orig->chipset.card_type >= 0x10) {
		CMP(debug[4], "DEBUG[4]")
	}
	if (orig->chipset.card_type >= 0x20) {
		if (!is_nv25p) {
			CMP(debug[5], "DEBUG[5]")
		}
		CMP(debug[6], "DEBUG[6]")
		CMP(debug[7], "DEBUG[7]")
		if (is_nv25p) {
			CMP(debug[16], "DEBUG[16]")
		}
	}

	CMP(intr, "INTR")
	CMP(intr_en, "INTR_EN")
	if (orig->chipset.card_type < 4) {
		CMP(invalid, "INVALID")
		CMP(invalid_en, "INVALID_EN")
	} else {
		CMP(nstatus, "NSTATUS")
		CMP(nsource, "NSOURCE")
	}
	if (orig->chipset.card_type < 3) {
		CMP(access, "ACCESS")
	} else {
		CMP(fifo_enable, "FIFO_ENABLE")
	}

	if (orig->chipset.card_type < 4) {
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
	} else {
		int ctx_num;
		if (orig->chipset.card_type < 0x10) {
			ctx_num = 4;
		} else {
			ctx_num = 5;
		}
		for (int i = 0; i < ctx_num; i++) {
			CMP(ctx_switch[i], "CTX_SWITCH[%d]", i)
		}
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < ctx_num; j++)
				CMP(ctx_cache[i][j], "CTX_CACHE[%d][%d]", i, j)
		}
		if (print || (exp->ctx_control & ~0x100) != (real->ctx_control & ~0x100)) {
			CMP(ctx_control, "CTX_CONTROL")
		}
		CMP(ctx_user, "CTX_USER")
		if (orig->chipset.card_type >= 0x10) {
			CMP(unk77c, "UNK77C")
		}
		if (is_nv17p) {
			CMP(unk6b0, "UNK6B0")
			CMP(unk838, "UNK838")
			CMP(unk83c, "UNK83C")
			CMP(zcull_unka00[0], "ZCULL_UNKA00[0]")
			CMP(zcull_unka00[1], "ZCULL_UNKA00[1]")
			CMP(unka10, "UNKA10")
		}
		for (int i = 0; i < (is_nv1720p ? 8 : 4); i++) {
			CMP(fifo_mthd[i], "FIFO_MTHD[%d]", i)
			CMP(fifo_data[i][0], "FIFO_DATA[%d][0]", i)
			if (orig->chipset.card_type >= 0x10) {
				CMP(fifo_data[i][1], "FIFO_DATA[%d][1]", i)
			}
		}
		CMP(fifo_ptr, "FIFO_PTR")
		CMP(fifo_mthd_st2, "FIFO_MTHD_ST2")
		CMP(fifo_data_st2[0], "FIFO_DATA_ST2[0]")
		if (orig->chipset.card_type >= 0x10) {
			CMP(fifo_data_st2[1], "FIFO_DATA_ST2[1]")
		}
		CMP(notify, "NOTIFY")
	}

	// VTX
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
	if (orig->chipset.card_type == 0x10) {
		for (int i = 0; i < 2; i++) {
			CMP(uclip_min[2][i], "CLIP3D_MIN[%d]", i)
			CMP(uclip_max[2][i], "CLIP3D_MAX[%d]", i)
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
	}

	// VSTATE
	for (auto &reg : pgraph_vstate_regs(orig->chipset)) {
		std::string name = reg->name();
		if (print)
			printf("%08x %08x %08x %s %s\n",
				reg->ref(orig), reg->ref(exp), reg->ref(real), name.c_str(),
				(reg->ref(exp) == reg->ref(real) ? "" : "*"));
		else if (reg->ref(exp) != reg->ref(real)) {
			printf("Difference in reg %s: expected %08x real %08x\n",
				name.c_str(), reg->ref(exp), reg->ref(real));
			broke = true;
		}
	}

	// ROP
	for (auto &reg : pgraph_rop_regs(orig->chipset)) {
		std::string name = reg->name();
		if (print)
			printf("%08x %08x %08x %s %s\n",
				reg->ref(orig), reg->ref(exp), reg->ref(real), name.c_str(),
				(reg->ref(exp) == reg->ref(real) ? "" : "*"));
		else if (reg->ref(exp) != reg->ref(real)) {
			printf("Difference in reg %s: expected %08x real %08x\n",
				name.c_str(), reg->ref(exp), reg->ref(real));
			broke = true;
		}
	}

	// CANVAS
	for (auto &reg : pgraph_canvas_regs(orig->chipset)) {
		std::string name = reg->name();
		if (print)
			printf("%08x %08x %08x %s %s\n",
				reg->ref(orig), reg->ref(exp), reg->ref(real), name.c_str(),
				(reg->ref(exp) == reg->ref(real) ? "" : "*"));
		else if (reg->ref(exp) != reg->ref(real)) {
			printf("Difference in reg %s: expected %08x real %08x\n",
				name.c_str(), reg->ref(exp), reg->ref(real));
			broke = true;
		}
	}

	if (orig->chipset.card_type == 3) {
		// D3D0
		for (auto &reg : pgraph_d3d0_regs(orig->chipset)) {
			std::string name = reg->name();
			if (print)
				printf("%08x %08x %08x %s %s\n",
					reg->ref(orig), reg->ref(exp), reg->ref(real), name.c_str(),
					(reg->ref(exp) == reg->ref(real) ? "" : "*"));
			else if (reg->ref(exp) != reg->ref(real)) {
				printf("Difference in reg %s: expected %08x real %08x\n",
					name.c_str(), reg->ref(exp), reg->ref(real));
				broke = true;
			}
		}
	} else if (orig->chipset.card_type == 4) {
		// D3D56
		for (auto &reg : pgraph_d3d56_regs(orig->chipset)) {
			std::string name = reg->name();
			if (print)
				printf("%08x %08x %08x %s %s\n",
					reg->ref(orig), reg->ref(exp), reg->ref(real), name.c_str(),
					(reg->ref(exp) == reg->ref(real) ? "" : "*"));
			else if (reg->ref(exp) != reg->ref(real)) {
				printf("Difference in reg %s: expected %08x real %08x\n",
					name.c_str(), reg->ref(exp), reg->ref(real));
				broke = true;
			}
		}
	} else if (orig->chipset.card_type == 0x10) {
		// CELSIUS
		for (auto &reg : pgraph_celsius_regs(orig->chipset)) {
			std::string name = reg->name();
			if (print)
				printf("%08x %08x %08x %s %s\n",
					reg->ref(orig), reg->ref(exp), reg->ref(real), name.c_str(),
					(reg->ref(exp) == reg->ref(real) ? "" : "*"));
			else if (reg->ref(exp) != reg->ref(real)) {
				printf("Difference in reg %s: expected %08x real %08x\n",
					name.c_str(), reg->ref(exp), reg->ref(real));
				broke = true;
			}
		}
		CMP(celsius_pipe_begin_end, "CELSIUS_PIPE_BEGIN_END")
		CMP(celsius_pipe_edge_flag, "CELSIUS_PIPE_EDGE_FLAG")
		CMP(celsius_pipe_unk48, "CELSIUS_PIPE_UNK48")
		CMP(celsius_pipe_vtx_state, "CELSIUS_PIPE_VTX_STATE")
		for (int i = 0; i < 0x1c; i++) {
			CMP(celsius_pipe_vtx[i], "CELSIUS_PIPE_VTX[%d]", i)
		}
		if (real->celsius_pipe_broke_ovtx || real->chipset.chipset != 0x10) {
			for (int i = 0; i < 4; i++)
				CMP(celsius_pipe_junk[i], "CELSIUS_PIPE_JUNK[%d]", i)
		}
		for (int i = 0; i < 0x3c; i++) {
			for (int j = 0; j < 4; j++)
				CMP(celsius_pipe_xfrm[i][j], "CELSIUS_PIPE_XFRM[%d][%d]", i, j)
		}
		for (int i = 0; i < 0x30; i++) {
			for (int j = 0; j < 3; j++)
				CMP(celsius_pipe_light_v[i][j], "CELSIUS_PIPE_LIGHT_V[%d][%d]", i, j)
		}
		for (int i = 0; i < 3; i++) {
			CMP(celsius_pipe_light_sa[i], "CELSIUS_PIPE_LIGHT_SA[%d]", i)
		}
		for (int i = 0; i < 19; i++) {
			CMP(celsius_pipe_light_sb[i], "CELSIUS_PIPE_LIGHT_SB[%d]", i)
		}
		for (int i = 0; i < 12; i++) {
			CMP(celsius_pipe_light_sc[i], "CELSIUS_PIPE_LIGHT_SC[%d]", i)
		}
		for (int i = 0; i < 12; i++) {
			CMP(celsius_pipe_light_sd[i], "CELSIUS_PIPE_LIGHT_SD[%d]", i)
		}
#if 0
		for (int i = 0; i < 0x3; i++) {
			for (int j = 0; j < 0x10; j++)
				CMP(celsius_pipe_xvtx[i][j], "CELSIUS_PIPE_XVTX[%d][%d]", i, j)
		}
#endif
		for (int i = 0; i < 0x10; i++) {
			CMP(celsius_pipe_ovtx[i][0], "CELSIUS_PIPE_OVTX[%d].TXC1.S", i)
			CMP(celsius_pipe_ovtx[i][1], "CELSIUS_PIPE_OVTX[%d].TXC1.T", i)
			CMP(celsius_pipe_ovtx[i][2], "CELSIUS_PIPE_OVTX[%d].PTSZ", i)
			CMP(celsius_pipe_ovtx[i][3], "CELSIUS_PIPE_OVTX[%d].TXC1.Q", i)
			if (!real->celsius_pipe_broke_ovtx || i != 0) {
				CMP(celsius_pipe_ovtx[i][4], "CELSIUS_PIPE_OVTX[%d].TXC0.S", i)
				CMP(celsius_pipe_ovtx[i][5], "CELSIUS_PIPE_OVTX[%d].TXC0.T", i)
				CMP(celsius_pipe_ovtx[i][7], "CELSIUS_PIPE_OVTX[%d].TXC0.Q", i)
			}
			CMP(celsius_pipe_ovtx[i][9], "CELSIUS_PIPE_OVTX[%d].FOG", i)
			CMP(celsius_pipe_ovtx[i][10], "CELSIUS_PIPE_OVTX[%d].COL0", i)
			CMP(celsius_pipe_ovtx[i][11], "CELSIUS_PIPE_OVTX[%d].COL1E", i)
			CMP(celsius_pipe_ovtx[i][12], "CELSIUS_PIPE_OVTX[%d].POS.X", i)
			CMP(celsius_pipe_ovtx[i][13], "CELSIUS_PIPE_OVTX[%d].POS.Y", i)
			CMP(celsius_pipe_ovtx[i][14], "CELSIUS_PIPE_OVTX[%d].POS.Z", i)
			CMP(celsius_pipe_ovtx[i][15], "CELSIUS_PIPE_OVTX[%d].POS.W", i)
		}
	}

	// DMA
	if (orig->chipset.card_type == 3) {
		CMP(dma_intr, "DMA_INTR")
		for (auto &reg : pgraph_dma_nv3_regs(orig->chipset)) {
			std::string name = reg->name();
			if (print)
				printf("%08x %08x %08x %s %s\n",
					reg->ref(orig), reg->ref(exp), reg->ref(real), name.c_str(),
					(reg->ref(exp) == reg->ref(real) ? "" : "*"));
			else if (reg->ref(exp) != reg->ref(real)) {
				printf("Difference in reg %s: expected %08x real %08x\n",
					name.c_str(), reg->ref(exp), reg->ref(real));
				broke = true;
			}
		}
	} else if (orig->chipset.card_type >= 4) {
		for (auto &reg : pgraph_dma_nv4_regs(orig->chipset)) {
			std::string name = reg->name();
			if (print)
				printf("%08x %08x %08x %s %s\n",
					reg->ref(orig), reg->ref(exp), reg->ref(real), name.c_str(),
					(reg->ref(exp) == reg->ref(real) ? "" : "*"));
			else if (reg->ref(exp) != reg->ref(real)) {
				printf("Difference in reg %s: expected %08x real %08x\n",
					name.c_str(), reg->ref(exp), reg->ref(real));
				broke = true;
			}
		}
	}
	if (broke && !print) {
		print = true;
		goto restart;
	}
	return broke;
}
