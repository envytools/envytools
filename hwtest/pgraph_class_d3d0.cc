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

class MthdD3D0TexOffset : public SingleMthdTest {
	void emulate_mthd() override {
		exp.dma_offset[0] = val;
		insrt(exp.valid[0], 23, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0TexFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff31ffff;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		if (val & ~0xff31ffff)
			return false;
		if (extr(val, 24, 4) > 0xb)
			return false;
		if (extr(val, 28, 4) > 0xb)
			return false;
		return true;
	}
	void emulate_mthd() override {
		exp.misc32[0] = val;
		insrt(exp.valid[0], 24, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0TexFilter : public SingleMthdTest {
	void emulate_mthd() override {
		exp.misc32[1] = val;
		insrt(exp.valid[0], 25, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0FogColor : public SingleMthdTest {
	void emulate_mthd() override {
		exp.misc24[0] = val & 0xffffff;
		insrt(exp.valid[0], 27, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0Config : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (cls == 0x18) {
				val &= 0xf77f0000;
			} else {
				val &= 0xf77fbdf3;
			}
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		if (val & ~0xf77fbdf3)
			return false;
		if (cls == 0x18) {
			if (val & 0xffff)
				return false;
		} else {
			if (extr(val, 0, 2) > 2)
				return false;
			if (extr(val, 12, 2) == 0)
				return false;
		}
		if (extr(val, 16, 4) == 0)
			return false;
		if (extr(val, 16, 4) > 8)
			return false;
		if (extr(val, 20, 3) > 4)
			return false;
		if (extr(val, 24, 3) > 4)
			return false;
		return true;
	}
	void emulate_mthd() override {
		exp.d3d0_config = val & 0xf77fbdf3;
		insrt(exp.valid[0], 26, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0Alpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xfff;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		if (val & ~0xfff)
			return false;
		if (extr(val, 8, 4) == 0)
			return false;
		if (extr(val, 8, 4) > 8)
			return false;
		return true;
	}
	void emulate_mthd() override {
		exp.d3d0_alpha = val & 0xfff;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0TlvFogTri : public SingleMthdTest {
	void emulate_mthd() override {
		exp.d3d0_tlv_fog_tri_col1 = val;
		insrt(exp.valid[0], 16, 7, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0TlvColor : public SingleMthdTest {
	void emulate_mthd() override {
		insrt(exp.d3d0_tlv_color, 0, 24, extr(val, 0, 24));
		insrt(exp.d3d0_tlv_color, 24, 7, extr(val, 25, 7));
		insrt(exp.valid[0], 17, 1, 1);
		insrt(exp.valid[0], extr(exp.d3d0_tlv_fog_tri_col1, 0, 4), 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0TlvX : public SingleMthdTest {
	void emulate_mthd() override {
		uint16_t tv = nv03_pgraph_convert_xy(val);
		insrt(exp.d3d0_tlv_xy, 0, 16, tv);
		insrt(exp.valid[0], 21, 1, 1);
		insrt(exp.valid[0], extr(exp.d3d0_tlv_fog_tri_col1, 0, 4), 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0TlvY : public SingleMthdTest {
	void emulate_mthd() override {
		uint16_t tv = nv03_pgraph_convert_xy(val);
		insrt(exp.d3d0_tlv_xy, 16, 16, tv);
		insrt(exp.valid[0], 20, 1, 1);
		insrt(exp.valid[0], extr(exp.d3d0_tlv_fog_tri_col1, 0, 4), 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0TlvZ : public SingleMthdTest {
	void emulate_mthd() override {
		uint32_t tv = nv03_pgraph_convert_z(val);
		exp.d3d0_tlv_z = extr(tv, 0, 16);
		insrt(exp.d3d0_tlv_rhw, 25, 7, extr(tv, 16, 7));
		insrt(exp.d3d0_tlv_color, 31, 1, extr(tv, 23, 1));
		insrt(exp.valid[0], 19, 1, 1);
		insrt(exp.valid[0], extr(exp.d3d0_tlv_fog_tri_col1, 0, 4), 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0TlvRhw : public SingleMthdTest {
	void emulate_mthd() override {
		insrt(exp.d3d0_tlv_rhw, 0, 25, extr(val, 6, 25));
		insrt(exp.valid[0], 18, 1, 1);
		insrt(exp.valid[0], extr(exp.d3d0_tlv_fog_tri_col1, 0, 4), 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0TlvU : public SingleMthdTest {
	void emulate_mthd() override {
		int sz = extr(exp.misc32[0], 28, 4);
		uint16_t tv = nv03_pgraph_convert_uv(val, sz);
		insrt(exp.d3d0_tlv_uv, 0, 16, tv);
		insrt(exp.valid[0], 22, 1, 1);
		insrt(exp.valid[0], extr(exp.d3d0_tlv_fog_tri_col1, 0, 4), 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D0TlvV : public SingleMthdTest {
	void adjust_orig_mthd() override {
		// XXX: disable this some day and test the actual DMA
		if (rnd() & 1) {
			insrt(orig.cliprect_ctrl, 8, 1, 1);
		} else {
			insrt(orig.valid[0], 27, 1, 0);
		}
	}
	void emulate_mthd() override {
		int sz = extr(exp.misc32[0], 28, 4);
		uint16_t tv = nv03_pgraph_convert_uv(val, sz);
		insrt(exp.d3d0_tlv_uv, 16, 16, tv);
		int vtxid = extr(exp.d3d0_tlv_fog_tri_col1, 0, 4);
		exp.misc24[1] = extr(exp.d3d0_tlv_fog_tri_col1, 0, 24);
		exp.vtx_z[vtxid] = extr(exp.d3d0_tlv_fog_tri_col1, 24, 8) << 16 | exp.d3d0_tlv_z;
		for (int j = 0; j < 2; j++) {
			uint32_t coord = extr(exp.d3d0_tlv_xy, j * 16, 16);
			exp.vtx_xy[vtxid][j] = coord | extr(exp.d3d0_tlv_uv, j * 16, 16) << 16;
			int cstat = nv03_pgraph_clip_status(&exp, (int16_t)coord, j, false);
			pgraph_set_xy_d(&exp, j, vtxid, vtxid, false, false, false, cstat);
		}
		exp.vtx_xy[vtxid+16][0] = exp.d3d0_tlv_rhw;
		exp.vtx_xy[vtxid+16][1] = exp.d3d0_tlv_color;
		if (extr(exp.valid[0], 16, 7) == 0x7f) {
			insrt(exp.valid[0], 16, 7, 0);
			insrt(exp.valid[0], vtxid, 1, 1);
		}
		pgraph_prep_draw(&exp, false, false);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdZPointZeta : public SingleMthdTest {
	int repeats() override { return 10000; };
	void adjust_orig_mthd() override {
		if (rnd() & 3)
			insrt(orig.cliprect_ctrl, 8, 1, 0);
		if (rnd() & 3)
			insrt(orig.debug[2], 28, 1, 0);
		if (rnd() & 3) {
			insrt(orig.xy_misc_4[0], 4, 4, 0);
			insrt(orig.xy_misc_4[1], 4, 4, 0);
		}
		if (rnd() & 3) {
			orig.valid[0] = 0x0fffffff;
			if (rnd() & 1) {
				orig.valid[0] &= ~0xf0f;
			}
			orig.valid[0] ^= 1 << (rnd() & 0x1f);
			orig.valid[0] ^= 1 << (rnd() & 0x1f);
		}
		if (rnd() & 1) {
			insrt(orig.ctx_switch[0], 24, 5, 0x17);
			insrt(orig.ctx_switch[0], 13, 2, 0);
			int j;
			for (j = 0; j < 8; j++) {
				insrt(orig.ctx_cache[j][0], 24, 5, 0x17);
				insrt(orig.ctx_cache[j][0], 13, 2, 0);
			}
		}
		orig.debug[2] &= 0xffdfffff;
		orig.debug[3] &= 0xfffeffff;
		orig.debug[3] &= 0xfffdffff;
	}
	void emulate_mthd() override {
		insrt(exp.misc32[1], 16, 16, extr(val, 16, 16));
		pgraph_prep_draw(&exp, false, false);
		nv03_pgraph_vtx_add(&exp, 0, 0, exp.vtx_xy[0][0], 1, 0, false, false);
	}
	using SingleMthdTest::SingleMthdTest;
};

}

std::vector<SingleMthdTest *> ZPoint::mthds() {
	return {
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdD3D0Config(opt, rnd(), "config", -1, cls, 0x304),
		new MthdD3D0Alpha(opt, rnd(), "alpha", -1, cls, 0x308),
		new MthdVtxXy(opt, rnd(), "xy", -1, cls, 0x7fc, 1, 4, VTX_FIRST),
		new MthdSolidColor(opt, rnd(), "color", -1, cls, 0x800, 0x100, 8),
		new MthdZPointZeta(opt, rnd(), "zeta", -1, cls, 0x804, 0x100, 8),
	};
}

std::vector<SingleMthdTest *> D3D0::mthds() {
	return {
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdD3D0TexOffset(opt, rnd(), "tex_offset", -1, cls, 0x304),
		new MthdD3D0TexFormat(opt, rnd(), "tex_format", -1, cls, 0x308),
		new MthdD3D0TexFilter(opt, rnd(), "tex_filter", -1, cls, 0x30c),
		new MthdD3D0FogColor(opt, rnd(), "fog_color", -1, cls, 0x310),
		new MthdD3D0Config(opt, rnd(), "config", -1, cls, 0x314),
		new MthdD3D0Alpha(opt, rnd(), "alpha", -1, cls, 0x318),
		new MthdD3D0TlvFogTri(opt, rnd(), "tlv_fog_tri", -1, cls, 0x1000, 0x80, 0x20),
		new MthdD3D0TlvColor(opt, rnd(), "tlv_color", -1, cls, 0x1004, 0x80, 0x20),
		new MthdD3D0TlvX(opt, rnd(), "tlv_x", -1, cls, 0x1008, 0x80, 0x20),
		new MthdD3D0TlvY(opt, rnd(), "tlv_y", -1, cls, 0x100c, 0x80, 0x20),
		new MthdD3D0TlvZ(opt, rnd(), "tlv_z", -1, cls, 0x1010, 0x80, 0x20),
		new MthdD3D0TlvRhw(opt, rnd(), "tlv_rhw", -1, cls, 0x1014, 0x80, 0x20),
		new MthdD3D0TlvU(opt, rnd(), "tlv_u", -1, cls, 0x1018, 0x80, 0x20),
		new MthdD3D0TlvV(opt, rnd(), "tlv_v", -1, cls, 0x101c, 0x80, 0x20),
	};
}

}
}
