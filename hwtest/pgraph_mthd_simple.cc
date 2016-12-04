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

class MthdBetaTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x01;
		mthd = 0x300;
	}
	void emulate_mthd() override {
		exp.beta = val;
		if (exp.beta & 0x80000000)
			exp.beta = 0;
		exp.beta &= 0x7f800000;
	}
public:
	MthdBetaTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdRopTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x02;
		mthd = 0x300;
	}
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
public:
	MthdRopTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdChromaPlaneTest : public MthdTest {
	void choose_mthd() override {
		if (rnd() & 1)
			cls = 0x03;
		else
			cls = 0x04;
		mthd = 0x304;
	}
	void emulate_mthd() override {
		uint32_t color = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
		if (cls == 0x04)
			exp.plane = color;
		else
			exp.chroma = color;
	}
public:
	MthdChromaPlaneTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternShapeTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x06;
		mthd = 0x308;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			val &= 0xf;
	}
	bool is_valid_val() override {
		return val <= 2;
	}
	void emulate_mthd() override {
		exp.pattern_config = val & 3;
	}
public:
	MthdPatternShapeTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternMonoColorTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		cls = 0x06;
		idx = rnd() & 1;
		mthd = 0x310 + idx * 4;
	}
	void emulate_mthd() override {
		struct pgraph_color c = pgraph_expand_color(&exp, val);
		exp.pattern_mono_rgb[idx] = c.r << 20 | c.g << 10 | c.b;
		exp.pattern_mono_a[idx] = c.a;
	}
public:
	MthdPatternMonoColorTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternMonoBitmapTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		cls = 0x06;
		idx = rnd() & 1;
		mthd = 0x318 + idx * 4;
	}
	void emulate_mthd() override {
		if (chipset.card_type == 3) {
			// yup, orig. hw bug.
			exp.pattern_mono_bitmap[idx] = pgraph_expand_mono(&orig, val);
		} else {
			exp.pattern_mono_bitmap[idx] = pgraph_expand_mono(&exp, val);
		}
	}
public:
	MthdPatternMonoBitmapTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSolidColorTest : public MthdTest {
	void choose_mthd() override {
		if (chipset.card_type < 3) {
			switch (rnd() % 5) {
				case 0:
					mthd = 0x304;
					cls = 8 + rnd() % 5;
					break;
				case 1:
					mthd = 0x500 | (rnd()&0x78);
					cls = 8;
					break;
				case 2:
					mthd = 0x600 | (rnd()&0x78);
					cls = 9 + (rnd() & 1);
					break;
				case 3:
					mthd = 0x500 | (rnd()&0x70);
					cls = 0xb;
					break;
				case 4:
					mthd = 0x580 | (rnd()&0x78);
					cls = 0xb;
					break;
				default:
					abort();
			}
		} else {
			switch (rnd() % 8) {
				case 0:
					mthd = 0x304;
					cls = 7 + rnd()%5;
					break;
				case 1:
					mthd = 0x500 | (rnd()&0x78);
					cls = 8;
					break;
				case 2:
					mthd = 0x600 | (rnd()&0x78);
					cls = 9 + (rnd()&1);
					break;
				case 3:
					mthd = 0x500 | (rnd()&0x70);
					cls = 0xb;
					break;
				case 4:
					mthd = 0x580 | (rnd()&0x78);
					cls = 0xb;
					break;
				case 5:
					mthd = 0x3fc;
					cls = 0xc;
					break;
				case 6:
					mthd = 0x7fc;
					cls = 0xc;
					break;
				case 7:
					mthd = 0x800 | (rnd()&0x7f8);
					cls = 0x18;
					break;
				default:
					abort();
			}
		}
	}
	void emulate_mthd() override {
		exp.misc32[0] = val;
		if (chipset.card_type >= 3)
			insrt(exp.valid[0], 16, 1, 1);
	}
public:
	MthdSolidColorTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdBitmapColorTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		if (chipset.card_type < 3) {
			idx = rnd() & 1;
			cls = 0x12;
			mthd = 0x308 + idx * 4;
		} else {
			switch (rnd() % 4) {
				case 0:
					mthd = 0xbf4;
					cls = 0xc;
					idx = 1;
					break;
				case 1:
					mthd = 0xff0;
					cls = 0xc;
					idx = 1;
					break;
				case 2:
					idx = rnd() & 1;
					mthd = 0x13ec + idx * 4;
					cls = 0xc;
					break;
				case 3:
					idx = rnd() & 1;
					mthd = 0x308 + idx * 4;
					cls = 0x12;
					break;
				default:
					abort();
			}
		}
	}
	void emulate_mthd() override {
		if (chipset.card_type < 3 || idx == 0) {
			exp.bitmap_color[idx] = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
			if (chipset.card_type >= 3)
				insrt(exp.valid[0], 17, 1, 1);
		} else {
			exp.misc32[1] = val;
			insrt(exp.valid[0], 18, 1, 1);
		}
	}
public:
	MthdBitmapColorTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSubdivideTest : public MthdTest {
	bool supported() override {
		return chipset.card_type < 3;
	}
	void choose_mthd() override {
		bool beta = rnd() & 1;
		bool quad = rnd() & 1;
		cls = 0xd + beta * 0x10 + quad;
		mthd = 0x304;
		if (rnd() & 1)
			val &= ~0xff00;
	}
	void emulate_mthd() override {
		exp.subdivide = val & 0xffff00ff;
		bool err = false;
		if (val & 0xff00)
			err = true;
		for (int j = 0; j < 8; j++) {
			if (extr(val, 4*j, 4) > 8)
				err = true;
			if (j < 2 && extr(val, 4*j, 4) < 2)
				err = true;
		}
		if (err) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access &= ~0x101;
		}
	}
public:
	MthdSubdivideTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdVtxBetaTest : public MthdTest {
	int idx;
	bool supported() override {
		return chipset.card_type < 3;
	}
	void choose_mthd() override {
		bool quad = rnd() & 1;
		cls = 0x1d + quad;
		idx = rnd() % (quad ? 5 : 2);
		mthd = 0x380 + idx * 4;
	}
	void emulate_mthd() override {
		uint32_t rclass = extr(exp.access, 12, 5);
		for (int j = 0; j < 2; j++) {
			int vid = idx * 2 + j;
			if (vid == 9 && (rclass & 0xf) == 0xe)
				break;
			uint32_t beta = extr(val, j*16, 16);
			beta &= 0xff80;
			beta <<= 8;
			beta |= 0x4000;
			if (beta & 1 << 23)
				beta |= 1 << 24;
			exp.vtx_beta[vid] = beta;
		}
		if (rclass == 0x1d || rclass == 0x1e)
			exp.valid[0] |= 1 << (12 + idx);
	}
public:
	MthdVtxBetaTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdIxmPitchTest : public MthdTest {
	void choose_mthd() override {
		if (chipset.card_type < 3)
			cls = 0x13 + (rnd() & 1);
		else
			cls = 0x14;
		mthd = 0x310;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return chipset.card_type < 3 || !(val & ~0x1fff);
	}
	void emulate_mthd() override {
		if (chipset.card_type < 3) {
			exp.vtx_xy[6][0] = val;
			exp.valid[0] |= 0x040040;
		} else {
			insrt(exp.dma_pitch, 0, 16, val);
			insrt(exp.valid[0], 10, 1, 1);
			insrt(exp.valid[0], 14, 1, 0);
		}
	}
public:
	MthdIxmPitchTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

}

bool PGraphMthdSimpleTests::supported() {
	return chipset.card_type < 4;
}

Test::Subtests PGraphMthdSimpleTests::subtests() {
	return {
		{"beta", new MthdBetaTest(opt, rnd())},
		{"rop", new MthdRopTest(opt, rnd())},
		{"chroma_plane", new MthdChromaPlaneTest(opt, rnd())},
		{"pattern_shape", new MthdPatternShapeTest(opt, rnd())},
		{"pattern_mono_color", new MthdPatternMonoColorTest(opt, rnd())},
		{"pattern_mono_bitmap", new MthdPatternMonoBitmapTest(opt, rnd())},
		{"solid_color", new MthdSolidColorTest(opt, rnd())},
		{"bitmap_color", new MthdBitmapColorTest(opt, rnd())},
		{"subdivide", new MthdSubdivideTest(opt, rnd())},
		{"vtx_beta", new MthdVtxBetaTest(opt, rnd())},
		{"ixm_pitch", new MthdIxmPitchTest(opt, rnd())},
	};
}

}
}
