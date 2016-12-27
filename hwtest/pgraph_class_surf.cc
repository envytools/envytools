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

namespace {

class MthdSurfFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x01010003;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val == 1 || val == 0x01000000 || val == 0x01010000 || val == 0x01010001;
	}
	void emulate_mthd() override {
		if (chipset.card_type < 4) {
			int which = extr(exp.ctx_switch[0], 16, 2);
			int f = 1;
			if (extr(val, 0, 1))
				f = 0;
			if (!extr(val, 16, 1))
				f = 2;
			if (!extr(val, 24, 1))
				f = 3;
			insrt(exp.surf_format, 4*which, 3, f | 4);
		} else {
			int which = cls & 3;
			int fmt = 0;
			if (val == 1) {
				fmt = 7;
			} else if (val == 0x01010000) {
				fmt = 1;
			} else if (val == 0x01000000) {
				fmt = 2;
			} else if (val == 0x01010001) {
				fmt = 6;
			}
			insrt(exp.surf_format, which*4, 4, fmt);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurfPitch : public SingleMthdTest {
	bool is_valid_val() override {
		if (chipset.card_type < 4)
			return true;
		return val && !(val & ~0x1ff0);
	}
	void emulate_mthd() override {
		int which;
		if (chipset.card_type < 4) {
			which = extr(exp.ctx_switch[0], 16, 2);
		} else {
			which = cls & 3;
			insrt(exp.valid[0], 2, 1, 1);
			insrt(exp.ctx_valid, 8+which, 1, !extr(exp.nsource, 1, 1));
		}
		exp.surf_pitch[which] = val & pgraph_pitch_mask(&chipset);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurf2DFormat : public SingleMthdTest {
	bool is_valid_val() override {
		return val != 0 && val < 0xe;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	void emulate_mthd() override {
		int fmt = 0;
		switch (val & 0xf) {
			case 0x01:
				fmt = 1;
				break;
			case 0x02:
				fmt = 2;
				break;
			case 0x03:
				fmt = 3;
				break;
			case 0x04:
				fmt = 5;
				break;
			case 0x05:
				fmt = 6;
				break;
			case 0x06:
				fmt = 7;
				break;
			case 0x07:
				fmt = 0xb;
				break;
			case 0x08:
				fmt = 0x9;
				break;
			case 0x09:
				fmt = 0xa;
				break;
			case 0x0a:
				fmt = 0xc;
				break;
			case 0x0b:
				fmt = 0xd;
				break;
		}
		insrt(exp.surf_format, 0, 4, fmt);
		insrt(exp.surf_format, 4, 4, fmt);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurfSwzFormat : public SingleMthdTest {
	bool is_valid_val() override {
		int fmt = extr(val, 0, 16);
		if (fmt == 0 || fmt > 0xd)
			return false;
		int swzx = extr(val, 16, 8);
		int swzy = extr(val, 24, 8);
		if (swzx > 0xb)
			return false;
		if (swzy > 0xb)
			return false;
		if (swzx == 0)
			return false;
		if (swzy == 0 && cls == 0x52)
			return false;
		return true;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x0f0f000f;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	void emulate_mthd() override {
		int fmt = 0;
		switch (val & 0xf) {
			case 0x01:
				fmt = 1;
				break;
			case 0x02:
				fmt = 2;
				break;
			case 0x03:
				fmt = 3;
				break;
			case 0x04:
				fmt = 5;
				break;
			case 0x05:
				fmt = 6;
				break;
			case 0x06:
				fmt = 7;
				break;
			case 0x07:
				fmt = 0xb;
				break;
			case 0x08:
				fmt = 0x9;
				break;
			case 0x09:
				fmt = 0xa;
				break;
			case 0x0a:
				fmt = 0xc;
				break;
			case 0x0b:
				fmt = 0xd;
				break;
		}
		int swzx = extr(val, 16, 4);
		int swzy = extr(val, 24, 4);
		if (cls == 0x9e) {
			fmt = 0;
		}
		insrt(exp.surf_format, 20, 4, fmt);
		insrt(exp.surf_swizzle[1], 16, 4, swzx);
		insrt(exp.surf_swizzle[1], 24, 4, swzy);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdClipSize : public SingleMthdTest {
	void emulate_mthd() override {
		int vidx = chipset.card_type < 0x10 ? 13 : 9;
		exp.vtx_xy[vidx][0] = extr(val, 0, 16);
		exp.vtx_xy[vidx][1] = extr(val, 16, 16);
		int xcstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[vidx][0], 0);
		int ycstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[vidx][1], 1);
		pgraph_set_xy_d(&exp, 0, 1, 1, false, false, false, xcstat);
		pgraph_set_xy_d(&exp, 1, 1, 1, false, false, false, ycstat);
		exp.uclip_min[0][0] = 0;
		exp.uclip_min[0][1] = 0;
		exp.uclip_max[0][0] = exp.vtx_xy[vidx][0];
		exp.uclip_max[0][1] = exp.vtx_xy[vidx][1];
		insrt(exp.valid[0], 28, 1, 0);
		insrt(exp.valid[0], 30, 1, 0);
		insrt(exp.xy_misc_1[0], 4, 2, 0);
		insrt(exp.xy_misc_1[0], 12, 1, 0);
		insrt(exp.xy_misc_1[0], 16, 1, 0);
		insrt(exp.xy_misc_1[0], 20, 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

}

void MthdDmaSurf::emulate_mthd() {
	uint32_t offset_mask = pgraph_offset_mask(&chipset);
	uint32_t base = (pobj[2] & ~0xfff) | extr(pobj[0], 20, 12);
	uint32_t limit = pobj[1];
	uint32_t dcls = extr(pobj[0], 0, 12);
	exp.surf_limit[which] = (limit & offset_mask) | 0xf | (dcls == 0x30) << 31;
	exp.surf_base[which] = base & offset_mask;
	bool bad = true;
	if (dcls == 0x30 || dcls == 0x3d)
		bad = false;
	if (dcls == 3 && (cls == 0x38 || cls == 0x88))
		bad = false;
	if (dcls == 2 && cls != 0x59 && which == 1)
		bad = false;
	if (extr(exp.debug[3], 23, 1) && bad) {
		nv04_pgraph_blowup(&exp, 0x2);
	}
	bool prot_err = false;
	if (extr(base, 0, 4 + kind))
		prot_err = true;
	if (extr(pobj[0], 16, 2))
		prot_err = true;
	if (chipset.chipset >= 5 && ((bad && chipset.card_type < 0x10) || dcls == 0x30))
		prot_err = false;
	if (prot_err)
		nv04_pgraph_blowup(&exp, 4);
	insrt(exp.ctx_valid, which, 1, dcls != 0x30 && !(bad && extr(exp.debug[3], 23, 1)));
}

bool MthdSurfOffset::is_valid_val() {
	if (chipset.card_type < 4)
		return true;
	if (kind == SURF_NV3)
		return !(val & 0xf);
	else if (kind == SURF_NV4)
		return !(val & 0x1f);
	else
		return !(val & 0x3f);
}

void MthdSurfOffset::emulate_mthd() {
	int surf;
	if (chipset.card_type < 4) {
		surf = extr(exp.ctx_switch[0], 16, 2);
	} else {
		surf = which;
		insrt(exp.valid[0], 3, 1, 1);
	}
	exp.surf_offset[surf] = val & pgraph_offset_mask(&chipset);
}

bool MthdSurfPitch2::is_valid_val() {
	if (!extr(val, 0, 16))
		return false;
	if (!extr(val, 16, 16))
		return false;
	if (kind == SURF_NV4)
		return !(val & 0xe01fe01f);
	else
		return !(val & 0x3f003f);
}

void MthdSurfPitch2::emulate_mthd() {
	uint32_t pitch_mask = pgraph_pitch_mask(&chipset);
	exp.surf_pitch[which_a] = val & pitch_mask;
	exp.surf_pitch[which_b] = val >> 16 & pitch_mask;
	exp.valid[0] |= 4;
	insrt(exp.ctx_valid, 8+which_a, 1, !extr(exp.nsource, 1, 1));
	insrt(exp.ctx_valid, 8+which_b, 1, !extr(exp.nsource, 1, 1));
}

bool MthdSurf3DFormat::is_valid_val() {
	int fmt = extr(val, 0, 4);
	if (fmt == 0 || fmt > (is_celsius ? 0xa : 8))
		return false;
	int zfmt = extr(val, 4, 4);
	if (((cls == 0x53 || cls == 0x93 || cls == 0x96) && extr(exp.ctx_switch[0], 22, 1)) || cls == 0x98 || cls == 0x99) {
		if (zfmt > 1)
			return false;
	} else {
		if (zfmt)
			return false;
	}
	int mode = extr(val, 8, 4);
	if (mode == 0 || mode > 2)
		return false;
	if (zfmt == 1 && mode == 2 && cls != 0x53 && cls != 0x93)
		return false;
	if (zfmt == 1 && (fmt == 9 || fmt == 0xa))
		return false;
	if (cls == 0x99) {
		int v = extr(val, 12, 4);
		if (v != 0 && v != 3)
			return false;
	} else {
		if (extr(val, 12, 4))
			return false;
	}
	if (extr(val, 16, 8) > 0xb)
		return false;
	if (extr(val, 24, 8) > 0xb)
		return false;
	return true;
}

void MthdSurf3DFormat::emulate_mthd() {
	int fmt = 0;
	switch (val & 0xf) {
		case 1:
			fmt = 0x2;
			break;
		case 2:
			fmt = 0x3;
			break;
		case 3:
			fmt = 0x5;
			break;
		case 4:
			fmt = 0x7;
			break;
		case 5:
			fmt = 0xb;
			break;
		case 6:
			fmt = 0x9;
			break;
		case 7:
			fmt = 0xa;
			break;
		case 8:
			fmt = 0xc;
			break;
		case 9:
			if (is_celsius)
				fmt = 0x1;
			break;
		case 0xa:
			if (is_celsius)
				fmt = 0x6;
			break;
	}
	insrt(exp.surf_format, 8, 4, fmt);
	insrt(exp.surf_type, 0, 2, extr(val, 8, 2));
	if (nv04_pgraph_is_nv11p(&chipset))
		insrt(exp.surf_type, 4, 1, extr(val, 4, 1));
	if (nv04_pgraph_is_nv17p(&chipset))
		insrt(exp.surf_type, 5, 1, extr(val, 12, 1));
	insrt(exp.surf_swizzle[0], 16, 4, extr(val, 16, 4));
	insrt(exp.surf_swizzle[0], 24, 4, extr(val, 24, 4));
}

std::vector<SingleMthdTest *> Surf::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdDmaSurf(opt, rnd(), "dma_surf", 2, cls, 0x184, cls & 3, SURF_NV3),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 2),
		new MthdSurfFormat(opt, rnd(), "format", 3, cls, 0x300),
		new MthdSurfPitch(opt, rnd(), "pitch", 4, cls, 0x308),
		new MthdSurfOffset(opt, rnd(), "offset", 5, cls, 0x30c, cls & 3, SURF_NV3),
	};
}

std::vector<SingleMthdTest *> Surf2D::mthds() {
	int kind = cls == 0x42 ? SURF_NV4 : SURF_NV10;
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdDmaSurf(opt, rnd(), "dma_surf_src", 2, cls, 0x184, 1, kind),
		new MthdDmaSurf(opt, rnd(), "dma_surf_dst", 3, cls, 0x188, 0, kind),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 0x40),
		new MthdSurf2DFormat(opt, rnd(), "format", 4, cls, 0x300),
		new MthdSurfPitch2(opt, rnd(), "pitch2", 5, cls, 0x304, 1, 0, kind),
		new MthdSurfOffset(opt, rnd(), "src_offset", 6, cls, 0x308, 1, kind),
		new MthdSurfOffset(opt, rnd(), "dst_offset", 7, cls, 0x30c, 0, kind),
	};
	if (cls != 0x42) {
		res.insert(res.begin(), {
			new MthdSync(opt, rnd(), "sync", -1, cls, 0x108),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> SurfSwz::mthds() {
	int kind = cls == 0x52 ? SURF_NV4 : SURF_NV10;
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdDmaSurf(opt, rnd(), "dma_surf_swz", 2, cls, 0x184, 5, kind),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 0x40),
		new MthdSurfSwzFormat(opt, rnd(), "format", 3, cls, 0x300),
		new MthdSurfOffset(opt, rnd(), "offset", 4, cls, 0x304, 5, kind),
	};
}

std::vector<SingleMthdTest *> Surf3D::mthds() {
	int kind = cls == 0x53 ? SURF_NV4 : SURF_NV10;
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdDmaSurf(opt, rnd(), "dma_surf_color", 2, cls, 0x184, 2, kind),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", 3, cls, 0x188, 3, kind),
		new MthdClipHv(opt, rnd(), "clip_h", 4, cls, 0x2f8, 0, 0),
		new MthdClipHv(opt, rnd(), "clip_v", 5, cls, 0x2fc, 0, 1),
		new MthdSurf3DFormat(opt, rnd(), "format", 6, cls, 0x300, false),
		new MthdClipSize(opt, rnd(), "clip_size", 7, cls, 0x304),
		new MthdSurfPitch2(opt, rnd(), "pitch2", 8, cls, 0x308, 2, 3, kind),
		new MthdSurfOffset(opt, rnd(), "color_offset", 9, cls, 0x30c, 2, kind),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", 10, cls, 0x310, 3, kind),
	};
}

}
}
