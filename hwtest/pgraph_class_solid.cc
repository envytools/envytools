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

void MthdSolidColor::emulate_mthd() {
	exp.misc32[0] = val;
	if (chipset.card_type >= 3)
		insrt(exp.valid[0], 16, 1, 1);
}

void MthdBitmapColor0::emulate_mthd() {
	if (chipset.card_type < 4) {
		exp.bitmap_color[0] = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
	} else {
		if (cls == 0x4b) {
			nv04_pgraph_set_bitmap_color_0_nv01(&exp, val);
		} else {
			exp.bitmap_color[0] = val;
			bool likes_format = false;
			if (nv04_pgraph_is_nv17p(&chipset) || chipset.chipset == 5)
				likes_format = true;
			if (!likes_format)
				insrt(exp.ctx_format, 0, 8, extr(exp.ctx_switch[1], 8, 8));
		}
	}
	if (chipset.card_type >= 3)
		insrt(exp.valid[0], 17, 1, 1);
}

void MthdBitmapColor1::emulate_mthd() {
	if (chipset.card_type < 3) {
		exp.bitmap_color[1] = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
	} else {
		exp.misc32[1] = val;
		insrt(exp.valid[0], 18, 1, 1);
	}
}

class MthdSolidFormat : public SingleMthdTest {
	bool is_new;
	bool supported() override { return chipset.card_type >= 4; }
	bool is_valid_val() override {
		return val < (is_new || cls == 0x4b ? 4 : 5) && val != 0;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	void emulate_mthd() override {
		int sfmt = val & 0xf;
		int fmt = 0;
		if (sfmt == 1)
			fmt = is_new ? 0xc : 0x3;
		if (sfmt == 2)
			fmt = 0x9;
		if (sfmt == 3)
			fmt = 0xe;
		if (sfmt == 4 && !is_new)
			fmt = 0x11;
		if (!extr(exp.nsource, 1, 1)) {
			insrt(egrobj[1], 8, 8, fmt);
			exp.ctx_cache[subc][1] = exp.ctx_switch[1];
			insrt(exp.ctx_cache[subc][1], 8, 8, fmt);
			if (extr(exp.debug[1], 20, 1))
				exp.ctx_switch[1] = exp.ctx_cache[subc][1];
		}
		bool likes_format = false;
		if (nv04_pgraph_is_nv17p(&chipset) || chipset.chipset == 5)
			likes_format = true;
		if (cls == 0x4a && likes_format) {
			insrt(exp.ctx_format, 0, 8, extr(exp.ctx_switch[1], 8, 8));
		}
	}
public:
	MthdSolidFormat(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, bool is_new)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), is_new(is_new) {}
};

class MthdFont : public SingleMthdTest {
	bool is_valid_val() override {
		int pitch = extr(val, 28, 4);
		return pitch >= 3 && pitch <= 9;
	}
	void emulate_mthd() override {
		exp.dma_offset[0] = val;
		int pitch = extr(val, 28, 4);
		switch (pitch) {
			default:
				exp.dma_length = 1 << pitch;
				break;
			case 0:
				exp.dma_length = 0x18;
				break;
			case 1:
				exp.dma_length = 0x28;
				break;
			case 2:
				exp.dma_length = 0x48;
				break;
			case 0xb:
			case 0xd:
				exp.dma_length = 0x200;
				break;
			case 0xf:
				exp.dma_length = 0x280;
				break;
			case 0xc:
				exp.dma_length = 0x100;
				break;
			case 0xa:
			case 0xe:
				exp.dma_length = 0x140;
				break;
		}
		insrt(exp.valid[0], 22, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

std::vector<SingleMthdTest *> Point::mthds() {
	return {
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdSolidColor(opt, rnd(), "color", -1, cls, 0x304),
		new MthdVtxXy(opt, rnd(), "point.xy", -1, cls, 0x400, 0x20, 4, VTX_FIRST | VTX_DRAW),
		new MthdVtxX32(opt, rnd(), "point32.x", -1, cls, 0x480, 0x10, 8, VTX_FIRST | VTX_DRAW),
		new MthdVtxY32(opt, rnd(), "point32.y", -1, cls, 0x484, 0x10, 8, VTX_FIRST | VTX_DRAW),
		new MthdSolidColor(opt, rnd(), "cpoint.color", -1, cls, 0x500, 0x10, 8),
		new MthdVtxXy(opt, rnd(), "cpoint.xy", -1, cls, 0x504, 0x10, 8, VTX_FIRST | VTX_DRAW),
	};
}

std::vector<SingleMthdTest *> Line::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdCtxClip(opt, rnd(), "ctx_clip", 2, cls, 0x184),
		new MthdCtxPattern(opt, rnd(), "ctx_pattern", 3, cls, 0x188, cls != 0x1c),
		new MthdCtxRop(opt, rnd(), "ctx_rop", 4, cls, 0x18c),
		new MthdCtxBeta(opt, rnd(), "ctx_beta", 5, cls, 0x190),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdOperation(opt, rnd(), "operation", 8, cls, 0x2fc, cls != 0x1c),
		new MthdSolidFormat(opt, rnd(), "format", 9, cls, 0x300, cls != 0x1c),
		new MthdSolidColor(opt, rnd(), "color", 10, cls, 0x304),
		new MthdVtxXy(opt, rnd(), "line.0.xy", 11, cls, 0x400, 0x10, 8, VTX_FIRST),
		new MthdVtxXy(opt, rnd(), "line.1.xy", 12, cls, 0x404, 0x10, 8, VTX_DRAW),
		new MthdVtxX32(opt, rnd(), "line32.0.x", 13, cls, 0x480, 8, 0x10, VTX_FIRST),
		new MthdVtxY32(opt, rnd(), "line32.0.y", 14, cls, 0x484, 8, 0x10, VTX_FIRST),
		new MthdVtxX32(opt, rnd(), "line32.1.x", 15, cls, 0x488, 8, 0x10, VTX_DRAW),
		new MthdVtxY32(opt, rnd(), "line32.1.y", 16, cls, 0x48c, 8, 0x10, VTX_DRAW),
		new MthdVtxXy(opt, rnd(), "polyline.xy", 17, cls, 0x500, 0x20, 4, VTX_POLY | VTX_DRAW),
		new MthdVtxX32(opt, rnd(), "polyline32.x", 18, cls, 0x580, 0x10, 8, VTX_POLY | VTX_DRAW),
		new MthdVtxY32(opt, rnd(), "polyline32.y", 19, cls, 0x584, 0x10, 8, VTX_POLY | VTX_DRAW),
		new MthdSolidColor(opt, rnd(), "cpolyline.color", 20, cls, 0x600, 0x10, 8),
		new MthdVtxXy(opt, rnd(), "cpolyline.xy", 21, cls, 0x604, 0x10, 8, VTX_POLY | VTX_DRAW),
	};
	if (cls == 0x1c) {
		res.insert(res.end(), {
			new MthdCtxSurf(opt, rnd(), "ctx_dst", 7, cls, 0x194, 0),
		});
	} else {
		res.insert(res.end(), {
			new MthdCtxBeta4(opt, rnd(), "ctx_beta4", 6, cls, 0x194),
			new MthdCtxSurf2D(opt, rnd(), "ctx_surf2d", 7, cls, 0x198, SURF2D_NV10),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Tri::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdCtxClip(opt, rnd(), "ctx_clip", 2, cls, 0x184),
		new MthdCtxPattern(opt, rnd(), "ctx_pattern", 3, cls, 0x188, cls != 0x1d),
		new MthdCtxRop(opt, rnd(), "ctx_rop", 4, cls, 0x18c),
		new MthdCtxBeta(opt, rnd(), "ctx_beta", 5, cls, 0x190),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdOperation(opt, rnd(), "operation", 8, cls, 0x2fc, cls != 0x1d),
		new MthdSolidFormat(opt, rnd(), "format", 9, cls, 0x300, cls != 0x1d),
		new MthdSolidColor(opt, rnd(), "color", 10, cls, 0x304),
		new MthdVtxXy(opt, rnd(), "triangle.0.xy", 11, cls, 0x310, 1, 4, VTX_FIRST),
		new MthdVtxXy(opt, rnd(), "triangle.1.xy", 12, cls, 0x314, 1, 4, 0),
		new MthdVtxXy(opt, rnd(), "triangle.2.xy", 13, cls, 0x318, 1, 4, VTX_DRAW),
		new MthdVtxX32(opt, rnd(), "triangle32.0.x", 14, cls, 0x320, 1, 4, VTX_FIRST),
		new MthdVtxY32(opt, rnd(), "triangle32.0.y", 15, cls, 0x324, 1, 4, VTX_FIRST),
		new MthdVtxX32(opt, rnd(), "triangle32.1.x", 16, cls, 0x328, 1, 4, 0),
		new MthdVtxY32(opt, rnd(), "triangle32.1.y", 17, cls, 0x32c, 1, 4, 0),
		new MthdVtxX32(opt, rnd(), "triangle32.2.x", 18, cls, 0x330, 1, 4, VTX_DRAW),
		new MthdVtxY32(opt, rnd(), "triangle32.2.y", 19, cls, 0x334, 1, 4, VTX_DRAW),
		new MthdVtxXy(opt, rnd(), "polytri.xy", 20, cls, 0x400, 0x20, 4, VTX_POLY | VTX_DRAW),
		new MthdVtxX32(opt, rnd(), "polytri32.x", 21, cls, 0x480, 0x10, 8, VTX_POLY | VTX_DRAW),
		new MthdVtxY32(opt, rnd(), "polytri32.y", 22, cls, 0x484, 0x10, 8, VTX_POLY | VTX_DRAW),
		new MthdSolidColor(opt, rnd(), "ctriangle.color", 23, cls, 0x500, 8, 0x10),
		new MthdVtxXy(opt, rnd(), "ctriangle.0.xy", 24, cls, 0x504, 8, 0x10, VTX_FIRST),
		new MthdVtxXy(opt, rnd(), "ctriangle.1.xy", 25, cls, 0x508, 8, 0x10, 0),
		new MthdVtxXy(opt, rnd(), "ctriangle.2.xy", 26, cls, 0x50c, 8, 0x10, VTX_DRAW),
		new MthdSolidColor(opt, rnd(), "cpolytri.color", 27, cls, 0x580, 0x10, 8),
		new MthdVtxXy(opt, rnd(), "cpolytri.xy", 28, cls, 0x584, 0x10, 8, VTX_POLY | VTX_DRAW),
	};
	if (cls == 0x1d) {
		res.insert(res.end(), {
			new MthdCtxSurf(opt, rnd(), "ctx_dst", 7, cls, 0x194, 0),
		});
	} else {
		res.insert(res.end(), {
			new MthdCtxBeta4(opt, rnd(), "ctx_beta4", 6, cls, 0x194),
			new MthdCtxSurf2D(opt, rnd(), "ctx_surf2d", 7, cls, 0x198, SURF2D_NV10),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Rect::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdCtxClip(opt, rnd(), "ctx_clip", 2, cls, 0x184),
		new MthdCtxPattern(opt, rnd(), "ctx_pattern", 3, cls, 0x188, cls != 0x1e),
		new MthdCtxRop(opt, rnd(), "ctx_rop", 4, cls, 0x18c),
		new MthdCtxBeta(opt, rnd(), "ctx_beta", 5, cls, 0x190),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdOperation(opt, rnd(), "operation", 8, cls, 0x2fc, cls != 0x1e),
		new MthdSolidFormat(opt, rnd(), "format", 9, cls, 0x300, cls != 0x1e),
		new MthdSolidColor(opt, rnd(), "color", 10, cls, 0x304),
		new MthdVtxXy(opt, rnd(), "rect.xy", 11, cls, 0x400, 0x10, 8, VTX_FIRST),
		new MthdRect(opt, rnd(), "rect.rect", 12, cls, 0x404, 0x10, 8, VTX_DRAW),
	};
	if (cls == 0x1e) {
		res.insert(res.end(), {
			new MthdCtxSurf(opt, rnd(), "ctx_dst", 7, cls, 0x194, 0),
		});
	} else {
		res.insert(res.end(), {
			new MthdCtxBeta4(opt, rnd(), "ctx_beta4", 6, cls, 0x194),
			new MthdCtxSurf2D(opt, rnd(), "ctx_surf2d", 7, cls, 0x198, SURF2D_NV10),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> GdiNv3::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdCtxPattern(opt, rnd(), "ctx_pattern", 3, cls, 0x184, false),
		new MthdCtxRop(opt, rnd(), "ctx_rop", 4, cls, 0x188),
		new MthdCtxBeta(opt, rnd(), "ctx_beta", 5, cls, 0x18c),
		new MthdCtxSurf(opt, rnd(), "ctx_dst", 7, cls, 0x190, 0),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdOperation(opt, rnd(), "operation", 8, cls, 0x2fc, false),
		new MthdSolidFormat(opt, rnd(), "format", 9, cls, 0x300, false),
		new MthdBitmapFormat(opt, rnd(), "bitmap_format", 10, cls, 0x304),
		new MthdSolidColor(opt, rnd(), "a.color", 11, cls, 0x3fc),
		new MthdVtxXy(opt, rnd(), "a.xy", 14, cls, 0x400, 0x40, 8, VTX_FIRST | VTX_NOCLIP),
		new MthdRect(opt, rnd(), "a.rect", 15, cls, 0x404, 0x40, 8, VTX_DRAW | VTX_NOCLIP),
		new MthdClipXy(opt, rnd(), "b.clip_xy_0", 16, cls, 0x7f4, 1, 0),
		new MthdClipXy(opt, rnd(), "b.clip_xy_1", 17, cls, 0x7f8, 1, 1),
		new MthdSolidColor(opt, rnd(), "b.color", 11, cls, 0x7fc),
		new MthdVtxXy(opt, rnd(), "b.0.xy", 18, cls, 0x800, 0x40, 8, VTX_FIRST),
		new MthdVtxXy(opt, rnd(), "b.1.xy", -1, cls, 0x804, 0x40, 8, VTX_DRAW),
		new MthdClipXy(opt, rnd(), "c.clip_xy_0", 16, cls, 0xbec, 1, 0),
		new MthdClipXy(opt, rnd(), "c.clip_xy_1", 17, cls, 0xbf0, 1, 1),
		new MthdBitmapColor1(opt, rnd(), "c.color", 12, cls, 0xbf4),
		new MthdIfcSize(opt, rnd(), "c.size", 21, cls, 0xbf8, IFC_IN | IFC_OUT | IFC_BITMAP),
		new MthdVtxXy(opt, rnd(), "c.xy", 20, cls, 0xbfc, 1, 4, VTX_FIRST | VTX_IFC),
		new MthdBitmapData(opt, rnd(), "c.bitmap_data", 24, cls, 0xc00, 0x80, 4, false),
		new MthdClipXy(opt, rnd(), "d.clip_xy_0", 16, cls, 0xfe8, 1, 0),
		new MthdClipXy(opt, rnd(), "d.clip_xy_1", 17, cls, 0xfec, 1, 1),
		new MthdBitmapColor1(opt, rnd(), "d.color", 12, cls, 0xff0),
		new MthdIfcSize(opt, rnd(), "d.size_in", 22, cls, 0xff4, IFC_IN | IFC_BITMAP),
		new MthdIfcSize(opt, rnd(), "d.size_out", 23, cls, 0xff8, IFC_OUT | IFC_BITMAP),
		new MthdVtxXy(opt, rnd(), "d.xy", 20, cls, 0xffc, 1, 4, VTX_FIRST | VTX_IFC),
		new MthdBitmapData(opt, rnd(), "d.bitmap_data", 24, cls, 0x1000, 0x80, 4, false),
		new MthdClipXy(opt, rnd(), "e.clip_xy_0", 16, cls, 0x13e4, 1, 0),
		new MthdClipXy(opt, rnd(), "e.clip_xy_1", 17, cls, 0x13e8, 1, 1),
		new MthdBitmapColor0(opt, rnd(), "e.color0", 13, cls, 0x13ec),
		new MthdBitmapColor1(opt, rnd(), "e.color1", 12, cls, 0x13f0),
		new MthdIfcSize(opt, rnd(), "e.size_in", 22, cls, 0x13f4, IFC_IN | IFC_BITMAP),
		new MthdIfcSize(opt, rnd(), "e.size_out", 23, cls, 0x13f8, IFC_OUT | IFC_BITMAP),
		new MthdVtxXy(opt, rnd(), "e.xy", 20, cls, 0x13fc, 1, 4, VTX_FIRST | VTX_IFC),
		new MthdBitmapData(opt, rnd(), "e.bitmap_data", 25, cls, 0x1400, 0x80, 4, true),
	};
}

std::vector<SingleMthdTest *> GdiNv4::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdDmaGrobj(opt, rnd(), "dma_font", 2, cls, 0x184, 0, DMA_R),
		new MthdCtxPattern(opt, rnd(), "ctx_pattern", 3, cls, 0x188, true),
		new MthdCtxRop(opt, rnd(), "ctx_rop", 4, cls, 0x18c),
		new MthdCtxBeta(opt, rnd(), "ctx_beta", 5, cls, 0x190),
		new MthdCtxBeta4(opt, rnd(), "ctx_beta4", 6, cls, 0x194),
		new MthdCtxSurf2D(opt, rnd(), "ctx_surf2d", 7, cls, 0x198, SURF2D_NV10),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdOperation(opt, rnd(), "operation", 8, cls, 0x2fc, true),
		new MthdSolidFormat(opt, rnd(), "format", 9, cls, 0x300, true),
		new MthdBitmapFormat(opt, rnd(), "bitmap_format", 10, cls, 0x304),
		new MthdSolidColor(opt, rnd(), "a.color", 11, cls, 0x3fc),
		new MthdVtxXy(opt, rnd(), "a.xy", 14, cls, 0x400, 0x20, 8, VTX_FIRST | VTX_NOCLIP),
		new MthdRect(opt, rnd(), "a.rect", 15, cls, 0x404, 0x20, 8, VTX_DRAW | VTX_NOCLIP),
		new MthdClipXy(opt, rnd(), "b.clip_xy_0", 16, cls, 0x5f4, 1, 0),
		new MthdClipXy(opt, rnd(), "b.clip_xy_1", 17, cls, 0x5f8, 1, 1),
		new MthdSolidColor(opt, rnd(), "b.color", 11, cls, 0x5fc),
		new MthdVtxXy(opt, rnd(), "b.0.xy", 18, cls, 0x600, 0x20, 8, VTX_FIRST),
		new MthdVtxXy(opt, rnd(), "b.1.xy", -1, cls, 0x604, 0x20, 8, VTX_DRAW),
		new MthdClipXy(opt, rnd(), "c.clip_xy_0", 16, cls, 0x7ec, 1, 0),
		new MthdClipXy(opt, rnd(), "c.clip_xy_1", 17, cls, 0x7f0, 1, 1),
		new MthdBitmapColor1(opt, rnd(), "c.color", 12, cls, 0x7f4),
		new MthdIfcSize(opt, rnd(), "c.size", 21, cls, 0x7f8, IFC_IN | IFC_OUT | IFC_BITMAP),
		new MthdVtxXy(opt, rnd(), "c.xy", 20, cls, 0x7fc, 1, 4, VTX_FIRST | VTX_IFC),
		new MthdBitmapData(opt, rnd(), "c.bitmap_data", 24, cls, 0x800, 0x80, 4, false),
		new MthdClipXy(opt, rnd(), "e.clip_xy_0", 16, cls, 0xbe4, 1, 0),
		new MthdClipXy(opt, rnd(), "e.clip_xy_1", 17, cls, 0xbe8, 1, 1),
		new MthdBitmapColor0(opt, rnd(), "e.color0", 13, cls, 0xbec),
		new MthdBitmapColor1(opt, rnd(), "e.color1", 12, cls, 0xbf0),
		new MthdIfcSize(opt, rnd(), "e.size_in", 22, cls, 0xbf4, IFC_IN | IFC_BITMAP),
		new MthdIfcSize(opt, rnd(), "e.size_out", 23, cls, 0xbf8, IFC_OUT | IFC_BITMAP),
		new MthdVtxXy(opt, rnd(), "e.xy", 20, cls, 0xbfc, 1, 4, VTX_FIRST | VTX_IFC),
		new MthdBitmapData(opt, rnd(), "e.bitmap_data", 25, cls, 0xc00, 0x80, 4, true),
		new MthdFont(opt, rnd(), "f.font", 26, cls, 0xff0),
		new MthdClipXy(opt, rnd(), "f.clip_xy_0", 16, cls, 0xff4, 1, 0),
		new MthdClipXy(opt, rnd(), "f.clip_xy_1", 17, cls, 0xff8, 1, 1),
		new MthdBitmapColor1(opt, rnd(), "f.color", 12, cls, 0xffc),
		new UntestedMthd(opt, rnd(), "f.xyc", -1, cls, 0x1000, 0x100), // XXX
		new MthdFont(opt, rnd(), "g.font", 26, cls, 0x17f0),
		new MthdClipXy(opt, rnd(), "g.clip_xy_0", 16, cls, 0x17f4, 1, 0),
		new MthdClipXy(opt, rnd(), "g.clip_xy_1", 17, cls, 0x17f8, 1, 1),
		new MthdBitmapColor1(opt, rnd(), "g.color", 12, cls, 0x17fc),
		new MthdVtxXy(opt, rnd(), "g.xy", 20, cls, 0x1800, 0x100, 8, VTX_FIRST | VTX_IFC),
		new UntestedMthd(opt, rnd(), "g.char", -1, cls, 0x1804, 0x100, 8), // XXX
	};
}

std::vector<SingleMthdTest *> Blit::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdCtxChroma(opt, rnd(), "ctx_chroma", 2, cls, 0x184, cls != 0x1f),
		new MthdCtxClip(opt, rnd(), "ctx_clip", 3, cls, 0x188),
		new MthdCtxPattern(opt, rnd(), "ctx_pattern", 4, cls, 0x18c, cls != 0x1f),
		new MthdCtxRop(opt, rnd(), "ctx_rop", 5, cls, 0x190),
		new MthdCtxBeta(opt, rnd(), "ctx_beta", 6, cls, 0x194),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 2),
		new MthdOperation(opt, rnd(), "operation", 10, cls, 0x2fc, cls != 0x1f),
		new MthdVtxXy(opt, rnd(), "src.xy", 11, cls, 0x300, 1, 4, VTX_FIRST | VTX_CHECK),
		new MthdVtxXy(opt, rnd(), "dst.xy", 12, cls, 0x304, 1, 4, 0),
		new MthdRect(opt, rnd(), "rect", 13, cls, 0x308, 1, 4, VTX_DRAW),
	};
	if (cls == 0x9f) {
		res.insert(res.begin(), {
			new MthdSync(opt, rnd(), "sync", -1, cls, 0x108),
			new UntestedMthd(opt, rnd(), "unk", -1, cls, 0x10c), // XXX
			new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 0, 1),
			new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 0, 0),
			new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 0, 2),
			new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 0),
			new UntestedMthd(opt, rnd(), "unk", -1, cls, 0x130), // XXX
			new UntestedMthd(opt, rnd(), "unk", -1, cls, 0x134), // XXX
		});
	}
	if (cls == 0x1f) {
		res.insert(res.end(), {
			new MthdCtxSurf(opt, rnd(), "ctx_src", 8, cls, 0x198, 1),
			new MthdCtxSurf(opt, rnd(), "ctx_dst", 9, cls, 0x19c, 0),
		});
	} else {
		res.insert(res.end(), {
			new MthdCtxBeta4(opt, rnd(), "ctx_beta4", 7, cls, 0x198),
			new MthdCtxSurf2D(opt, rnd(), "ctx_surf2d", 8, cls, 0x19c, SURF2D_NV10),
		});
	}
	return res;
}

}
}
