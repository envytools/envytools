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

class MthdBeta : public SingleMthdTest {
	void emulate_mthd() override {
		exp.beta = val;
		if (exp.beta & 0x80000000)
			exp.beta = 0;
		exp.beta &= 0x7f800000;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdBeta4 : public SingleMthdTest {
	void emulate_mthd() override {
		exp.beta4 = val;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdRop : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return chipset.card_type >= 3 || !(val & ~0xff);
	}
	void emulate_mthd() override {
		exp.rop = val & 0xff;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdChroma : public SingleMthdTest {
	void emulate_mthd() override {
		if (chipset.card_type < 4) {
			exp.chroma = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
		} else {
			if (cls == 0x17)
				nv04_pgraph_set_chroma_nv01(&exp, val);
			else
				exp.chroma = val;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdPlane : public SingleMthdTest {
	void emulate_mthd() override {
		if (chipset.card_type < 3) {
			exp.plane = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdPatternShape : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			val &= 0xf;
	}
	bool is_valid_val() override {
		return val <= 2;
	}
	void emulate_mthd() override {
		insrt(exp.pattern_config, 0, 2, val);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdPatternSelect : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			val &= 0xf;
	}
	bool is_valid_val() override {
		return val == 1 || val == 2;
	}
	void emulate_mthd() override {
		insrt(exp.pattern_config, 4, 1, extr(val, 1, 1));
		insrt(exp.ctx_valid, 22, 1, !extr(exp.nsource, 1, 1));
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdPatternBitmapColor : public SingleMthdTest {
	int which;
	void emulate_mthd() override {
		if (chipset.card_type < 4) {
			struct pgraph_color c = pgraph_expand_color(&exp, val);
			exp.pattern_mono_rgb[which] = c.r << 20 | c.g << 10 | c.b;
			exp.pattern_mono_a[which] = c.a;
		} else if (cls == 0x18) {
			nv04_pgraph_set_pattern_mono_color_nv01(&exp, which, val);
		} else {
			exp.pattern_mono_color[which] = val;
		}
	}
public:
	MthdPatternBitmapColor(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdPatternBitmap : public SingleMthdTest {
	int which;
	void emulate_mthd() override {
		uint32_t rval = nv04_pgraph_bswap(&exp, val);
		if (chipset.card_type == 3) {
			// yup, orig. hw bug.
			exp.pattern_mono_bitmap[which] = pgraph_expand_mono(&orig, rval);
		} else {
			exp.pattern_mono_bitmap[which] = pgraph_expand_mono(&exp, rval);
		}
		if (chipset.card_type >= 4 && cls == 0x18) {
			uint32_t fmt = extr(exp.ctx_switch[1], 0, 2);
			if (fmt != 1 && fmt != 2) {
				pgraph_state_error(&exp);
			}
			insrt(exp.ctx_valid, 26+which, 1, !extr(exp.nsource, 1, 1));
		}
	}
public:
	MthdPatternBitmap(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdPatternColorY8 : public SingleMthdTest {
	void emulate_mthd() override {
		uint32_t rval = nv04_pgraph_bswap(&exp, val);
		for (int i = 0; i < 4; i++)
			exp.pattern_color[idx*4+i] = extr(rval, 8*i, 8) * 0x010101;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdPatternColorR5G6B5 : public SingleMthdTest {
	void emulate_mthd() override {
		uint32_t rval = nv04_pgraph_hswap(&exp, val);
		for (int i = 0; i < 2; i++) {
			uint8_t b = extr(rval, i * 16 + 0, 5) * 0x21 >> 2;
			uint8_t g = extr(rval, i * 16 + 5, 6) * 0x41 >> 4;
			uint8_t r = extr(rval, i * 16 + 11, 5) * 0x21 >> 2;
			exp.pattern_color[idx*2+i] = b | g << 8 | r << 16;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdPatternColorR5G5B5 : public SingleMthdTest {
	void emulate_mthd() override {
		uint32_t rval = nv04_pgraph_hswap(&exp, val);
		for (int i = 0; i < 2; i++) {
			uint8_t b = extr(rval, i * 16 + 0, 5) * 0x21 >> 2;
			uint8_t g = extr(rval, i * 16 + 5, 5) * 0x21 >> 2;
			uint8_t r = extr(rval, i * 16 + 10, 5) * 0x21 >> 2;
			exp.pattern_color[idx*2+i] = b | g << 8 | r << 16;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdPatternColorR8G8B8 : public SingleMthdTest {
	void emulate_mthd() override {
		exp.pattern_color[idx] = extr(val, 0, 24);
	}
	using SingleMthdTest::SingleMthdTest;
};

std::vector<SingleMthdTest *> Beta::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdBeta(opt, rnd(), "beta", 2, cls, 0x300),
	};
}

std::vector<SingleMthdTest *> Beta4::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdBeta4(opt, rnd(), "beta4", 2, cls, 0x300),
	};
}

std::vector<SingleMthdTest *> Rop::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdRop(opt, rnd(), "rop", 2, cls, 0x300),
	};
}

std::vector<SingleMthdTest *> Chroma::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new UntestedMthd(opt, rnd(), "format", 2, cls, 0x300), // XXX
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdChroma(opt, rnd(), "chroma", 3, cls, 0x304),
	};
}

std::vector<SingleMthdTest *> Plane::mthds() {
	return {
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPlane(opt, rnd(), "plane", 3, cls, 0x304),
	};
}

std::vector<SingleMthdTest *> Pattern::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new UntestedMthd(opt, rnd(), "format", 2, cls, 0x300), // XXX
		new UntestedMthd(opt, rnd(), "bitmap_format", 3, cls, 0x304), // XXX
		new MthdPatternShape(opt, rnd(), "pattern_shape", 4, cls, 0x308),
		new MthdPatternBitmapColor(opt, rnd(), "pattern_bitmap_color_0", 5, cls, 0x310, 0),
		new MthdPatternBitmapColor(opt, rnd(), "pattern_bitmap_color_1", 6, cls, 0x314, 1),
		new MthdPatternBitmap(opt, rnd(), "pattern_bitmap_0", 7, cls, 0x318, 0),
		new MthdPatternBitmap(opt, rnd(), "pattern_bitmap_1", 8, cls, 0x31c, 1),
	};
}

std::vector<SingleMthdTest *> CPattern::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new UntestedMthd(opt, rnd(), "format", 2, cls, 0x300), // XXX
		new UntestedMthd(opt, rnd(), "bitmap_format", 3, cls, 0x304), // XXX
		new MthdPatternShape(opt, rnd(), "pattern_shape", 4, cls, 0x308),
		new MthdPatternSelect(opt, rnd(), "pattern_select", 5, cls, 0x30c),
		new MthdPatternBitmapColor(opt, rnd(), "pattern_bitmap_color_0", 6, cls, 0x310, 0),
		new MthdPatternBitmapColor(opt, rnd(), "pattern_bitmap_color_1", 7, cls, 0x314, 1),
		new MthdPatternBitmap(opt, rnd(), "pattern_bitmap_0", 8, cls, 0x318, 0),
		new MthdPatternBitmap(opt, rnd(), "pattern_bitmap_1", 9, cls, 0x31c, 1),
		new MthdPatternColorY8(opt, rnd(), "pattern_color_y8", 10, cls, 0x400, 0x10),
		new MthdPatternColorR5G6B5(opt, rnd(), "pattern_color_r5g6b5", 12, cls, 0x500, 0x20),
		new MthdPatternColorR5G5B5(opt, rnd(), "pattern_color_r5g5b5", 11, cls, 0x600, 0x20),
		new MthdPatternColorR8G8B8(opt, rnd(), "pattern_color_r8g8b8", 13, cls, 0x700, 0x40),
	};
}

}
}
