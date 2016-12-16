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

namespace hwtest {
namespace pgraph {

namespace {

class MthdD3D0TexOffsetTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x17;
		mthd = 0x304;
	}
	void emulate_mthd() override {
		exp.dma_offset[0] = val;
		insrt(exp.valid[0], 23, 1, 1);
	}
public:
	MthdD3D0TexOffsetTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdD3D0TexFormatTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x17;
		mthd = 0x308;
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
public:
	MthdD3D0TexFormatTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdD3D0TexFilterTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x17;
		mthd = 0x30c;
	}
	void emulate_mthd() override {
		exp.misc32[1] = val;
		insrt(exp.valid[0], 25, 1, 1);
	}
public:
	MthdD3D0TexFilterTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdD3D0FogColorTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x17;
		mthd = 0x310;
	}
	void emulate_mthd() override {
		exp.misc24[0] = val & 0xffffff;
		insrt(exp.valid[0], 27, 1, 1);
	}
public:
	MthdD3D0FogColorTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdD3D0ConfigTest : public MthdTest {
	bool is_zpoint;
	void choose_mthd() override {
		is_zpoint = rnd() & 1;
		if (is_zpoint) {
			cls = 0x18;
			mthd = 0x304;
			if (rnd() & 1) {
				val &= 0xf77f0000;
				val ^= 1 << (rnd() & 0x1f);
			}
		} else {
			cls = 0x17;
			mthd = 0x314;
			if (rnd() & 1) {
				val &= 0xf77fbdf3;
				val ^= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (val & ~0xf77fbdf3)
			return false;
		if (is_zpoint) {
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
public:
	MthdD3D0ConfigTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdD3D0AlphaTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x17;
		mthd = 0x318;
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
public:
	MthdD3D0AlphaTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdD3D0TlvFogTriTest : public MthdTest {
	void choose_mthd() override {
		int idx = rnd() & 0x7f;
		cls = 0x17;
		mthd = 0x1000 + idx * 0x20;
	}
	void emulate_mthd() override {
		exp.d3d0_tlv_fog_tri_col1 = val;
		insrt(exp.valid[0], 16, 7, 1);
	}
public:
	MthdD3D0TlvFogTriTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdD3D0TlvColorTest : public MthdTest {
	void choose_mthd() override {
		int idx = rnd() & 0x7f;
		cls = 0x17;
		mthd = 0x1004 + idx * 0x20;
	}
	void emulate_mthd() override {
		insrt(exp.d3d0_tlv_color, 0, 24, extr(val, 0, 24));
		insrt(exp.d3d0_tlv_color, 24, 7, extr(val, 25, 7));
		insrt(exp.valid[0], 17, 1, 1);
		insrt(exp.valid[0], extr(exp.d3d0_tlv_fog_tri_col1, 0, 4), 1, 0);
	}
public:
	MthdD3D0TlvColorTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdD3D0TlvXyTest : public MthdTest {
	bool xy;
	void choose_mthd() override {
		int idx = rnd() & 0x7f;
		xy = rnd() & 1;
		cls = 0x17;
		mthd = 0x1008 + xy * 4 + idx * 0x20;
	}
	void emulate_mthd() override {
		int e = extr(val, 23, 8);
		bool s = extr(val, 31, 1);
		uint32_t tv = extr(val, 0, 23) | 1 << 23;
		if (e > 0x7f+10) {
			tv = 0x7fff + s;
		} else if (e < 0x7f-4) {
			tv = 0;
		} else {
			tv >>= 0x7f + 10 - e + 24 - 15;
			if (s)
				tv = -tv;
		}
		insrt(exp.d3d0_tlv_xy, 16*xy, 16, tv);
		insrt(exp.valid[0], 21 - xy, 1, 1);
		insrt(exp.valid[0], extr(exp.d3d0_tlv_fog_tri_col1, 0, 4), 1, 0);
	}
public:
	MthdD3D0TlvXyTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdD3D0TlvZTest : public MthdTest {
	void choose_mthd() override {
		int idx = rnd() & 0x7f;
		cls = 0x17;
		mthd = 0x1010 + idx * 0x20;
	}
	void emulate_mthd() override {
		int e = extr(val, 23, 8);
		bool s = extr(val, 31, 1);
		uint32_t tv = extr(val, 0, 23) | 1 << 23;
		if (s) {
			tv = 0;
		} else if (e > 0x7f-1) {
			tv = 0xffffff;
		} else if (e < 0x7f-24) {
			tv = 0;
		} else {
			tv >>= 0x7f-1 - e;
		}
		tv = ~tv;
		exp.d3d0_tlv_z = extr(tv, 0, 16);
		insrt(exp.d3d0_tlv_rhw, 25, 7, extr(tv, 16, 7));
		insrt(exp.d3d0_tlv_color, 31, 1, extr(tv, 23, 1));
		insrt(exp.valid[0], 19, 1, 1);
		insrt(exp.valid[0], extr(exp.d3d0_tlv_fog_tri_col1, 0, 4), 1, 0);
	}
public:
	MthdD3D0TlvZTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdD3D0TlvRhwTest : public MthdTest {
	void choose_mthd() override {
		int idx = rnd() & 0x7f;
		cls = 0x17;
		mthd = 0x1014 + idx * 0x20;
	}
	void emulate_mthd() override {
		insrt(exp.d3d0_tlv_rhw, 0, 25, extr(val, 6, 25));
		insrt(exp.valid[0], 18, 1, 1);
		insrt(exp.valid[0], extr(exp.d3d0_tlv_fog_tri_col1, 0, 4), 1, 0);
	}
public:
	MthdD3D0TlvRhwTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdD3D0TlvUTest : public MthdTest {
	void choose_mthd() override {
		int idx = rnd() & 0x7f;
		cls = 0x17;
		mthd = 0x1018 + idx * 0x20;
	}
	void emulate_mthd() override {
		int e = extr(val, 23, 8);
		bool s = extr(val, 31, 1);
		uint32_t tv = extr(val, 0, 23) | 1 << 23;
		int sz = extr(exp.misc32[0], 28, 4);
		e -= (11 - sz);
		if (e > 0x7f-1) {
			tv = 0x7fff + s;
		} else if (e < 0x7f-15) {
			tv = 0;
		} else {
			tv >>= 0x7f - 1 - e + 24 - 15;
			if (s)
				tv = -tv;
		}
		insrt(exp.d3d0_tlv_uv, 0, 16, tv);
		insrt(exp.valid[0], 22, 1, 1);
		insrt(exp.valid[0], extr(exp.d3d0_tlv_fog_tri_col1, 0, 4), 1, 0);
	}
public:
	MthdD3D0TlvUTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

}

bool PGraphMthdD3D0Tests::supported() {
	return chipset.card_type >= 3 && chipset.card_type < 4;
}

Test::Subtests PGraphMthdD3D0Tests::subtests() {
	return {
		{"tex_offset", new MthdD3D0TexOffsetTest(opt, rnd())},
		{"tex_format", new MthdD3D0TexFormatTest(opt, rnd())},
		{"tex_filter", new MthdD3D0TexFilterTest(opt, rnd())},
		{"fog_color", new MthdD3D0FogColorTest(opt, rnd())},
		{"config", new MthdD3D0ConfigTest(opt, rnd())},
		{"alpha", new MthdD3D0AlphaTest(opt, rnd())},
		{"tlv_fog_tri", new MthdD3D0TlvFogTriTest(opt, rnd())},
		{"tlv_color", new MthdD3D0TlvColorTest(opt, rnd())},
		{"tlv_xy", new MthdD3D0TlvXyTest(opt, rnd())},
		{"tlv_z", new MthdD3D0TlvZTest(opt, rnd())},
		{"tlv_rhw", new MthdD3D0TlvRhwTest(opt, rnd())},
		{"tlv_u", new MthdD3D0TlvUTest(opt, rnd())},
	};
}

}
}
