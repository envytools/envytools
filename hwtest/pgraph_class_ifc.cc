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
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
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

class MthdIfcDataLo : public SingleMthdTest {
	void emulate_mthd() override {
		exp.misc32[0] = val;
	}
	using SingleMthdTest::SingleMthdTest;
};

std::vector<SingleMthdTest *> Ifc::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new UntestedMthd(opt, rnd(), "ctx", -1, cls, 0x184, 7), // XXX
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdOperation(opt, rnd(), "operation", 9, cls, 0x2fc, cls != 0x21),
		new MthdIfcFormat(opt, rnd(), "format", 10, cls, 0x300, cls != 0x21),
		new MthdVtxXy(opt, rnd(), "xy", 11, cls, 0x304, 1, 4, VTX_FIRST | VTX_IFC),
		new MthdIfcSize(opt, rnd(), "size_out", 12, cls, 0x308, IFC_OUT),
		new MthdIfcSize(opt, rnd(), "size_in", 13, cls, 0x30c, IFC_IN),
	};
	if (cls == 0x65 || (cls & 0xff) == 0x8a) {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "unk2f8", -1, cls, 0x2f8), // XXX
		});
	}
	if ((cls & 0xff) == 0x8a) {
		res.insert(res.begin(), {
			new UntestedMthd(opt, rnd(), "nop", -1, cls, 0x100), // XXX
			new UntestedMthd(opt, rnd(), "notify", -1, cls, 0x104), // XXX
			new UntestedMthd(opt, rnd(), "sync", -1, cls, 0x108), // XXX
		});
		res.insert(res.end(), {
			new MthdIfcDataLo(opt, rnd(), "ifc_data_lo", 14, cls, 0x400, 0x380, 8),
			new UntestedMthd(opt, rnd(), "ifc_data_hi", -1, cls, 0x404, 0x380, 8), // XXX
		});
	} else {
		res.insert(res.begin(), {
			new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
			new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		});
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "ifc_data", -1, cls, 0x400, cls == 0x21 ? 0x20 : 0x700), // XXX
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
		new UntestedMthd(opt, rnd(), "bitmap_data", -1, cls, 0x400, 0x20), // XXX
	};
}

std::vector<SingleMthdTest *> Sifc::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new UntestedMthd(opt, rnd(), "ctx", -1, cls, 0x184, 7), // XXX
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdOperation(opt, rnd(), "operation", 8, cls, 0x2fc, cls != 0x36),
		new MthdIfcFormat(opt, rnd(), "format", 9, cls, 0x300, cls != 0x36),
		new MthdIfcSize(opt, rnd(), "size_in", 10, cls, 0x304, IFC_IN),
		new MthdSifcDiff(opt, rnd(), "dxdu", 11, cls, 0x308, 0),
		new MthdSifcDiff(opt, rnd(), "dydv", 12, cls, 0x30c, 1),
		new UntestedMthd(opt, rnd(), "unk", 13, cls, 0x310), // XXX
		new UntestedMthd(opt, rnd(), "unk", 14, cls, 0x314), // XXX
		new MthdSifcXy(opt, rnd(), "xy", 15, cls, 0x318),
		new UntestedMthd(opt, rnd(), "sifc_data", -1, cls, 0x400, 0x700), // XXX
	};
	if (cls == 0x66) {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "unk2f8", -1, cls, 0x2f8), // XXX
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
		new UntestedMthd(opt, rnd(), "ctx", -1, cls, 0x184, 8), // XXX
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdOperation(opt, rnd(), "operation", 10, cls, 0x3e4, true),
		new MthdIfcFormat(opt, rnd(), "format", 11, cls, 0x3e8, true),
		new MthdIndexFormat(opt, rnd(), "index_format", 12, cls, 0x3ec),
		new MthdPaletteOffset(opt, rnd(), "palette_offset", 13, cls, 0x3f0),
		new MthdVtxXy(opt, rnd(), "xy", 14, cls, 0x3f4, 1, 4, VTX_FIRST | VTX_IFC),
		new MthdIfcSize(opt, rnd(), "size_out", 15, cls, 0x3f8, IFC_OUT),
		new MthdIfcSize(opt, rnd(), "size_in", 16, cls, 0x3fc, IFC_IN),
		new UntestedMthd(opt, rnd(), "iifc_data", -1, cls, 0x400, 0x700), // XXX
	};
	if (cls == 0x64) {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "unk2f8", -1, cls, 0x3e0), // XXX
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Tfc::mthds() {
	return {
		new UntestedMthd(opt, rnd(), "nop", -1, cls, 0x100), // XXX
		new UntestedMthd(opt, rnd(), "notify", 0, cls, 0x104), // XXX
		new UntestedMthd(opt, rnd(), "sync", -1, cls, 0x108), // XXX
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 2, cls, 0x180),
		new UntestedMthd(opt, rnd(), "ctx", 3, cls, 0x184), // XXX
		new UntestedMthd(opt, rnd(), "dither", 10, cls, 0x2fc), // XXX
		new MthdIfcFormat(opt, rnd(), "format", 5, cls, 0x300, true),
		new MthdVtxXy(opt, rnd(), "xy", 6, cls, 0x304, 1, 4, VTX_FIRST | VTX_IFC | VTX_TFC),
		new MthdIfcSize(opt, rnd(), "size", 7, cls, 0x308, IFC_IN | IFC_OUT | IFC_TFC),
		new UntestedMthd(opt, rnd(), "unk30c", 8, cls, 0x30c), // XXX
		new UntestedMthd(opt, rnd(), "unk310", 9, cls, 0x310), // XXX
		new MthdIfcDataLo(opt, rnd(), "tfc_data_lo", 10, cls, 0x400, 0x380, 8),
		new UntestedMthd(opt, rnd(), "tfc_data_hi", -1, cls, 0x404, 0x380, 8), // XXX
	};
}

}
}
