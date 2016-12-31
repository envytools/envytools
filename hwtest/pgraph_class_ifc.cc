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

class MthdSifcXy : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			val &= 0xffff;
		if (!(rnd() & 3))
			val &= 0x000f000f;
		if (!(rnd() & 3))
			val = 0x00100000;
	}
	void emulate_mthd() override {
		exp.valid[0] |= 0x9018;
		if (chipset.card_type < 4) {
			exp.vtx_xy[3][1] = -exp.vtx_xy[7][1];
			pgraph_vtx_cmp(&exp, 0, 3, false);
			pgraph_vtx_cmp(&exp, 1, 3, false);
			nv03_pgraph_vtx_add(&exp, 1, 3, ~exp.vtx_xy[7][1], 1, 0, false, true);
		} else {
			nv04_pgraph_vtx_add(&exp, 1, 3, ~exp.vtx_xy[7][1], 1, 0, true);
			pgraph_vtx_cmp(&exp, 0, 3, false);
			pgraph_vtx_cmp(&exp, 1, 3, false);
		}
		exp.vtx_xy[4][0] = extr(val, 0, 16) << 16;
		exp.vtx_xy[4][1] = extr(val, 16, 16) << 16;
		insrt(exp.valid[0], 19, 1, 0);
		pgraph_clear_vtxid(&exp);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 0);
		if (chipset.card_type < 4) {
			int xcstat = nv03_pgraph_clip_status(&exp, extrs(val, 4, 12), 0, false);
			int ycstat = nv03_pgraph_clip_status(&exp, extrs(val, 20, 12), 1, false);
			pgraph_set_xy_d(&exp, 0, 0, 0, false, false, false, xcstat);
			pgraph_set_xy_d(&exp, 1, 0, 0, false, false, false, ycstat);
		} else {
			int xcstat = nv04_pgraph_clip_status(&exp, extrs(val, 4, 12), 0);
			int ycstat = nv04_pgraph_clip_status(&exp, extrs(val, 20, 12), 1);
			pgraph_set_xy_d(&exp, 0, 0, 0, false, false, false, xcstat);
			pgraph_set_xy_d(&exp, 1, 0, 0, false, false, false, ycstat);
		}
		insrt(exp.xy_misc_3, 8, 1, 0);
		pgraph_set_image_zero(&exp, !exp.vtx_xy[3][0] || !exp.vtx_xy[3][1]);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSifcDiff : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			val &= 0xffff;
		if (!(rnd() & 3))
			val &= 0x000f000f;
		if (!(rnd() & 3))
			val = 0x00100000;
	}
	void emulate_mthd() override {
		insrt(exp.valid[0], 5 + which * 8, 1, 1);
		exp.vtx_xy[5][which] = val;
		pgraph_clear_vtxid(&exp);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 0);
	}
public:
	MthdSifcDiff(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdIfcFormat : public SingleMthdTest {
	bool is_new;
	bool supported() override {
		return chipset.card_type >= 4;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		if (cls & 0xff00)
			return val < 8 && val != 0;
		return val < 6 && val != 0;
	}
	void emulate_mthd() override {
		int sfmt = val & 7;
		int fmt = 0;
		if (sfmt == 1)
			fmt = is_new ? 0xa : 0x1;
		if (sfmt == 2)
			fmt = 0x6;
		if (sfmt == 3)
			fmt = 0x7;
		if (sfmt == 4)
			fmt = 0xd;
		if (sfmt == 5)
			fmt = 0xe;
		if (sfmt == 6 && is_new && chipset.card_type >= 0x30)
			fmt = 0x18;
		if (sfmt == 7 && is_new && chipset.card_type >= 0x30)
			fmt = 0x19;
		if (!extr(exp.nsource, 1, 1)) {
			insrt(egrobj[1], 8, 8, fmt);
			exp.ctx_cache[subc][1] = exp.ctx_switch[1];
			insrt(exp.ctx_cache[subc][1], 8, 8, fmt);
			if (extr(exp.debug[1], 20, 1))
				exp.ctx_switch[1] = exp.ctx_cache[subc][1];
		}
	}
public:
	MthdIfcFormat(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, bool is_new)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), is_new(is_new) {}
};

class MthdIndexFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		insrt(exp.dvd_format, 0, 6, val & 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdPaletteOffset : public SingleMthdTest {
	void emulate_mthd() override {
		exp.dma_offset[0] = val;
		insrt(exp.valid[0], 22, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

void MthdBitmapData::adjust_orig_mthd() {
	if (chipset.card_type >= 4) {
		insrt(orig.notify, 0, 1, 0);
	}
}

void MthdBitmapData::emulate_mthd_pre() {
	if (!extr(exp.xy_misc_3, 12, 1))
		insrt(exp.xy_misc_3, 4, 1, 0);
}

void MthdBitmapData::emulate_mthd() {
	uint32_t rval = nv04_pgraph_bswap(&exp, val);
	if (chipset.card_type == 3) {
		// yup, orig. hw bug.
		exp.misc32[0] = pgraph_expand_mono(&orig, rval);
	} else {
		exp.misc32[0] = pgraph_expand_mono(&exp, rval);
	}
	// XXX: test me
	skip = true;
}

class MthdIifcData : public SingleMthdTest {
	void adjust_orig_mthd() override {
		insrt(orig.notify, 0, 1, 0);
		// XXX: unlock it some day
		orig.valid[0] = 0;
	}
	void emulate_mthd_pre() override {
		insrt(exp.xy_misc_3, 4, 1, 0);
	}
	void emulate_mthd() override {
		exp.misc32[0] = nv04_pgraph_bswap(&exp, val);
		// XXX: test me
		skip = true;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdIfcData : public SingleMthdTest {
	int which;
	bool draw;
	void adjust_orig_mthd() override {
		if (draw && chipset.card_type >= 4) {
			insrt(orig.notify, 0, 1, 0);
		}
	}
	void emulate_mthd_pre() override {
		if (draw && !pgraph_is_class_sifc(&exp))
			insrt(exp.xy_misc_3, 4, 1, 0);
	}
	void emulate_mthd() override {
		uint32_t rv = val;
		switch (extr(nv04_pgraph_formats(&exp), 12, 5)) {
			case 1:
				rv = nv04_pgraph_bswap(&exp, rv);
				break;
			case 6:
				rv = nv04_pgraph_hswap(&exp, rv);
				break;
			default:
				switch (extr(exp.ctx_switch[1], 8, 6)) {
					case 6:
					case 7:
					case 0xa:
						rv = nv04_pgraph_hswap(&exp, rv);
						break;
				}
				break;
		}
		exp.misc32[which] = rv;
		if (draw) {
			// XXX: test me
			skip = true;
		}
	}
public:
	MthdIfcData(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, uint32_t stride, int which, bool draw)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num, stride), which(which), draw(draw) {}
};

std::vector<SingleMthdTest *> Ifc::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdCtxChroma(opt, rnd(), "ctx_chroma", 2, cls, 0x184, cls != 0x21),
		new MthdCtxClip(opt, rnd(), "ctx_clip", 3, cls, 0x188),
		new MthdCtxPattern(opt, rnd(), "ctx_pattern", 4, cls, 0x18c, cls != 0x21),
		new MthdCtxRop(opt, rnd(), "ctx_rop", 5, cls, 0x190),
		new MthdCtxBeta(opt, rnd(), "ctx_beta", 6, cls, 0x194),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdOperation(opt, rnd(), "operation", 9, cls, 0x2fc, cls != 0x21),
		new MthdIfcFormat(opt, rnd(), "format", 10, cls, 0x300, cls != 0x21),
		new MthdVtxXy(opt, rnd(), "xy", 11, cls, 0x304, 1, 4, VTX_FIRST | VTX_IFC),
		new MthdIfcSize(opt, rnd(), "size_out", 12, cls, 0x308, IFC_OUT),
		new MthdIfcSize(opt, rnd(), "size_in", 13, cls, 0x30c, IFC_IN),
	};
	if (cls == 0x21) {
		res.insert(res.end(), {
			new MthdCtxSurf(opt, rnd(), "ctx_dst", 8, cls, 0x198, 0),
		});
	} else {
		res.insert(res.end(), {
			new MthdCtxBeta4(opt, rnd(), "ctx_beta4", 7, cls, 0x198),
			new MthdCtxSurf2D(opt, rnd(), "ctx_surf2d", 8, cls, 0x19c, SURF2D_NV10),
		});
	}
	if (cls == 0x65 || (cls & 0xff) == 0x8a) {
		res.insert(res.end(), {
			new MthdDither(opt, rnd(), "dither", 15, cls, 0x2f8),
		});
	}
	if ((cls & 0xff) == 0x8a) {
		res.insert(res.begin(), {
			new MthdSync(opt, rnd(), "sync", -1, cls, 0x108),
		});
		res.insert(res.end(), {
			new MthdIfcData(opt, rnd(), "ifc_data_lo", 14, cls, 0x400, 0x380, 8, 0, false),
			new MthdIfcData(opt, rnd(), "ifc_data_hi", 14, cls, 0x404, 0x380, 8, 1, true),
		});
	} else {
		res.insert(res.end(), {
			new MthdIfcData(opt, rnd(), "ifc_data", 14, cls, 0x400, cls == 0x21 ? 0x20 : 0x700, 4, 0, true),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Bitmap::mthds() {
	return {
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdBitmapColor0(opt, rnd(), "color0", -1, cls, 0x308),
		new MthdBitmapColor1(opt, rnd(), "color1", -1, cls, 0x30c),
		new MthdVtxXy(opt, rnd(), "xy", -1, cls, 0x310, 1, 4, VTX_FIRST | VTX_IFC),
		new MthdIfcSize(opt, rnd(), "size_out", -1, cls, 0x314, IFC_OUT | IFC_BITMAP),
		new MthdIfcSize(opt, rnd(), "size_in", -1, cls, 0x318, IFC_IN | IFC_BITMAP),
		new MthdBitmapData(opt, rnd(), "bitmap_data", -1, cls, 0x400, 0x20, 4, true),
	};
}

std::vector<SingleMthdTest *> Sifc::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdCtxChroma(opt, rnd(), "ctx_chroma", 2, cls, 0x184, cls != 0x36),
		new MthdCtxPattern(opt, rnd(), "ctx_pattern", 3, cls, 0x188, cls != 0x36),
		new MthdCtxRop(opt, rnd(), "ctx_rop", 4, cls, 0x18c),
		new MthdCtxBeta(opt, rnd(), "ctx_beta", 5, cls, 0x190),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdOperation(opt, rnd(), "operation", 8, cls, 0x2fc, cls != 0x36),
		new MthdIfcFormat(opt, rnd(), "format", 9, cls, 0x300, cls != 0x36),
		new MthdIfcSize(opt, rnd(), "size_in", 10, cls, 0x304, IFC_IN),
		new MthdSifcDiff(opt, rnd(), "dxdu", 11, cls, 0x308, 0),
		new MthdSifcDiff(opt, rnd(), "dydv", 12, cls, 0x30c, 1),
		new MthdClipXy(opt, rnd(), "clip_xy", 13, cls, 0x310, 1, 0),
		new MthdClipRect(opt, rnd(), "clip_rect", 14, cls, 0x314, 1),
		new MthdSifcXy(opt, rnd(), "xy", 15, cls, 0x318),
		new MthdIfcData(opt, rnd(), "sifc_data", 16, cls, 0x400, 0x700, 4, 0, true),
	};
	if (cls == 0x36) {
		res.insert(res.end(), {
			new MthdCtxSurf(opt, rnd(), "ctx_dst", 7, cls, 0x194, 0),
		});
	} else {
		res.insert(res.end(), {
			new MthdCtxBeta4(opt, rnd(), "ctx_beta4", 6, cls, 0x194),
			new MthdCtxSurf2D(opt, rnd(), "ctx_surf2d", 7, cls, 0x198, SURF2D_NV10),
		});
	}
	if ((cls & 0xff) == 0x66) {
		res.insert(res.end(), {
			new MthdDither(opt, rnd(), "dither", 17, cls, 0x2f8),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Iifc::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdDmaGrobj(opt, rnd(), "dma_palette", 2, cls, 0x184, 0, DMA_R),
		new MthdCtxChroma(opt, rnd(), "ctx_chroma", 3, cls, 0x188, true),
		new MthdCtxClip(opt, rnd(), "ctx_clip", 4, cls, 0x18c),
		new MthdCtxPattern(opt, rnd(), "ctx_pattern", 5, cls, 0x190, true),
		new MthdCtxRop(opt, rnd(), "ctx_rop", 6, cls, 0x194),
		new MthdCtxBeta(opt, rnd(), "ctx_beta", 7, cls, 0x198),
		new MthdCtxBeta4(opt, rnd(), "ctx_beta4", 8, cls, 0x19c),
		new MthdCtxSurf2D(opt, rnd(), "ctx_surf2d", 9, cls, 0x1a0, (cls == 0x60 ? SURF2D_NV4 : SURF2D_NV10) | SURF2D_SWZOK),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdOperation(opt, rnd(), "operation", 10, cls, 0x3e4, true),
		new MthdIfcFormat(opt, rnd(), "format", 11, cls, 0x3e8, true),
		new MthdIndexFormat(opt, rnd(), "index_format", 12, cls, 0x3ec),
		new MthdPaletteOffset(opt, rnd(), "palette_offset", 13, cls, 0x3f0),
		new MthdVtxXy(opt, rnd(), "xy", 14, cls, 0x3f4, 1, 4, VTX_FIRST | VTX_IFC),
		new MthdIfcSize(opt, rnd(), "size_out", 15, cls, 0x3f8, IFC_OUT),
		new MthdIfcSize(opt, rnd(), "size_in", 16, cls, 0x3fc, IFC_IN),
		new MthdIifcData(opt, rnd(), "iifc_data", 17, cls, 0x400, 0x700),
	};
	if ((cls & 0xff) == 0x64) {
		res.insert(res.end(), {
			new MthdDither(opt, rnd(), "dither", 18, cls, 0x3e0),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Tfc::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdSync(opt, rnd(), "sync", 1, cls, 0x108),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 2, cls, 0x180),
		new MthdCtxSurf2D(opt, rnd(), "ctx_surf2d", 3, cls, 0x184, SURF2D_NV10 | SURF2D_SWZOK),
		new MthdDither(opt, rnd(), "dither", 4, cls, 0x2fc),
		new MthdIfcFormat(opt, rnd(), "format", 5, cls, 0x300, true),
		new MthdVtxXy(opt, rnd(), "xy", 6, cls, 0x304, 1, 4, VTX_FIRST | VTX_IFC | VTX_TFC),
		new MthdIfcSize(opt, rnd(), "size", 7, cls, 0x308, IFC_IN | IFC_OUT | IFC_TFC),
		new MthdClipHv(opt, rnd(), "clip_h", 8, cls, 0x30c, 1, 0),
		new MthdClipHv(opt, rnd(), "clip_v", 9, cls, 0x310, 1, 1),
		new MthdIfcData(opt, rnd(), "tfc_data_lo", 10, cls, 0x400, 0x380, 8, 0, false),
		new MthdIfcData(opt, rnd(), "tfc_data_hi", 11, cls, 0x404, 0x380, 8, 1, true),
	};
}

}
}
