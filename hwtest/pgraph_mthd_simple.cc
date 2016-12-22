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
		if (chipset.card_type < 4)
			cls = 0x01;
		else
			cls = 0x12;
		mthd = 0x300;
		trapbit = 2;
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

class MthdBeta4Test : public MthdTest {
	bool supported() override { return chipset.card_type >= 4; }
	void choose_mthd() override {
		cls = 0x72;
		mthd = 0x300;
		trapbit = 2;
	}
	void emulate_mthd() override {
		exp.beta4 = val;
	}
public:
	MthdBeta4Test(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdRopTest : public MthdTest {
	void choose_mthd() override {
		if (chipset.card_type < 4)
			cls = 0x02;
		else
			cls = 0x43;
		mthd = 0x300;
		trapbit = 2;
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
		if (chipset.card_type < 4) {
			if (rnd() & 1)
				cls = 0x03;
			else
				cls = 0x04;
		} else {
			if (rnd() & 1)
				cls = 0x17;
			else
				cls = 0x57;
		}
		mthd = 0x304;
		trapbit = 3;
	}
	void emulate_mthd() override {
		if (chipset.card_type < 4) {
			uint32_t color = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
			if (cls == 0x04)
				exp.plane = color;
			else
				exp.chroma = color;
		} else {
			if (cls == 0x17)
				nv04_pgraph_set_chroma_nv01(&exp, val);
			else
				exp.chroma = val;
		}
	}
public:
	MthdChromaPlaneTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternShapeTest : public MthdTest {
	void choose_mthd() override {
		if (chipset.card_type < 4)
			cls = 0x06;
		else
			cls = (rnd() & 1) ? 0x18 : 0x44;
		mthd = 0x308;
		trapbit = 4;
	}
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
public:
	MthdPatternShapeTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternSelectTest : public MthdTest {
	bool supported() override { return chipset.card_type >= 4; }
	void choose_mthd() override {
		cls = 0x44;
		mthd = 0x30c;
		trapbit = 5;
	}
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
public:
	MthdPatternSelectTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternMonoColorTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		if (chipset.card_type < 4)
			cls = 0x06;
		else
			cls = (rnd() & 1) ? 0x18 : 0x44;
		idx = rnd() & 1;
		mthd = 0x310 + idx * 4;
		trapbit = (cls == 0x18 ? 5 : 6) + idx;
	}
	void emulate_mthd() override {
		if (chipset.card_type < 4) {
			struct pgraph_color c = pgraph_expand_color(&exp, val);
			exp.pattern_mono_rgb[idx] = c.r << 20 | c.g << 10 | c.b;
			exp.pattern_mono_a[idx] = c.a;
		} else if (cls == 0x18) {
			nv04_pgraph_set_pattern_mono_color_nv01(&exp, idx, val);
		} else {
			exp.pattern_mono_color[idx] = val;
		}
	}
public:
	MthdPatternMonoColorTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternMonoBitmapTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		if (chipset.card_type < 4)
			cls = 0x06;
		else
			cls = (rnd() & 1) ? 0x18 : 0x44;
		idx = rnd() & 1;
		mthd = 0x318 + idx * 4;
		trapbit = (cls == 0x18 ? 7 : 8) + idx;
	}
	void emulate_mthd() override {
		uint32_t rval = nv04_pgraph_bswap(&exp, val);
		if (chipset.card_type == 3) {
			// yup, orig. hw bug.
			exp.pattern_mono_bitmap[idx] = pgraph_expand_mono(&orig, rval);
		} else {
			exp.pattern_mono_bitmap[idx] = pgraph_expand_mono(&exp, rval);
		}
		if (chipset.card_type >= 4 && cls == 0x18) {
			uint32_t fmt = extr(exp.ctx_switch[1], 0, 2);
			if (fmt != 1 && fmt != 2) {
				pgraph_state_error(&exp);
			}
			insrt(exp.ctx_valid, 26+idx, 1, !extr(exp.nsource, 1, 1));
		}
	}
public:
	MthdPatternMonoBitmapTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternColorY8Test : public MthdTest {
	int idx;
	bool supported() override { return chipset.card_type >= 4; }
	void choose_mthd() override {
		cls = 0x44;
		idx = rnd() & 0xf;
		mthd = 0x400 + idx * 4;
		trapbit = 10;
	}
	void emulate_mthd() override {
		uint32_t rval = nv04_pgraph_bswap(&exp, val);
		for (int i = 0; i < 4; i++)
			exp.pattern_color[idx*4+i] = extr(rval, 8*i, 8) * 0x010101;
	}
public:
	MthdPatternColorY8Test(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternColorR5G6B5Test : public MthdTest {
	int idx;
	bool supported() override { return chipset.card_type >= 4; }
	void choose_mthd() override {
		cls = 0x44;
		idx = rnd() & 0x1f;
		mthd = 0x500 + idx * 4;
		trapbit = 12;
	}
	void emulate_mthd() override {
		uint32_t rval = nv04_pgraph_hswap(&exp, val);
		for (int i = 0; i < 2; i++) {
			uint8_t b = extr(rval, i * 16 + 0, 5) * 0x21 >> 2;
			uint8_t g = extr(rval, i * 16 + 5, 6) * 0x41 >> 4;
			uint8_t r = extr(rval, i * 16 + 11, 5) * 0x21 >> 2;
			exp.pattern_color[idx*2+i] = b | g << 8 | r << 16;
		}
	}
public:
	MthdPatternColorR5G6B5Test(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternColorR5G5B5Test : public MthdTest {
	int idx;
	bool supported() override { return chipset.card_type >= 4; }
	void choose_mthd() override {
		cls = 0x44;
		idx = rnd() & 0x1f;
		mthd = 0x600 + idx * 4;
		trapbit = 11;
	}
	void emulate_mthd() override {
		uint32_t rval = nv04_pgraph_hswap(&exp, val);
		for (int i = 0; i < 2; i++) {
			uint8_t b = extr(rval, i * 16 + 0, 5) * 0x21 >> 2;
			uint8_t g = extr(rval, i * 16 + 5, 5) * 0x21 >> 2;
			uint8_t r = extr(rval, i * 16 + 10, 5) * 0x21 >> 2;
			exp.pattern_color[idx*2+i] = b | g << 8 | r << 16;
		}
	}
public:
	MthdPatternColorR5G5B5Test(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternColorR8G8B8Test : public MthdTest {
	int idx;
	bool supported() override { return chipset.card_type >= 4; }
	void choose_mthd() override {
		cls = 0x44;
		idx = rnd() & 0x3f;
		mthd = 0x700 + idx * 4;
		trapbit = 13;
	}
	void emulate_mthd() override {
		exp.pattern_color[idx] = extr(val, 0, 24);
	}
public:
	MthdPatternColorR8G8B8Test(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
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
		} else if (chipset.card_type < 4) {
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
		} else {
			switch (rnd() % 9) {
				default:
					cls = (rnd() & 1 ? 0x5c : 0x1c);
					mthd = 0x304;
					trapbit = 10;
					break;
				case 1:
					cls = (rnd() & 1 ? 0x5c : 0x1c);
					mthd = 0x600 | (rnd() & 0x78);
					trapbit = 20;
					break;
				case 2:
					cls = (rnd() & 1 ? 0x5d : 0x1d);
					mthd = 0x304;
					trapbit = 10;
					break;
				case 3:
					cls = (rnd() & 1 ? 0x5d : 0x1d);
					mthd = 0x500 | (rnd() & 0x70);
					trapbit = 23;
					break;
				case 4:
					cls = (rnd() & 1 ? 0x5d : 0x1d);
					mthd = 0x580 | (rnd() & 0x78);
					trapbit = 27;
					break;
				case 5:
					cls = (rnd() & 1 ? 0x5e : 0x1e);
					mthd = 0x304;
					trapbit = 10;
					break;
				case 6:
					cls = (rnd() & 1 ? 0x4a : 0x4b);
					mthd = 0x3fc;
					trapbit = 11;
					break;
				case 7:
					cls = 0x4b;
					mthd = 0x7fc;
					trapbit = 11;
					break;
				case 8:
					cls = 0x4a;
					mthd = 0x5fc;
					trapbit = 11;
					break;
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
		} else if (chipset.card_type < 4) {
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
		} else {
			switch (rnd() % 18) {
				default:
					cls = 0x4b;
					mthd = 0xbf4;
					idx = 1;
					trapbit = 12;
					break;
				case 1:
					cls = 0x4b;
					mthd = 0xff0;
					idx = 1;
					trapbit = 12;
					break;
				case 2:
					cls = 0x4b;
					mthd = 0x13f0;
					idx = 1;
					trapbit = 12;
					break;
				case 3:
					cls = 0x4a;
					mthd = 0x7f4;
					idx = 1;
					trapbit = 12;
					break;
				case 4:
					cls = 0x4a;
					mthd = 0xbf0;
					idx = 1;
					trapbit = 12;
					break;
				case 5:
					cls = 0x4a;
					mthd = 0xffc;
					idx = 1;
					trapbit = 12;
					break;
				case 6:
					cls = 0x4a;
					mthd = 0x17fc;
					idx = 1;
					trapbit = 12;
					break;
				case 7:
					cls = 0x4b;
					mthd = 0x13ec;
					idx = 0;
					trapbit = 13;
					break;
				case 8:
					cls = 0x4a;
					mthd = 0xbec;
					idx = 0;
					trapbit = 13;
					break;
			}
		}
	}
	void emulate_mthd() override {
		if (chipset.card_type < 3 || idx == 0) {
			if (chipset.card_type < 4) {
				exp.bitmap_color[idx] = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
			} else {
				if (cls == 0x4b) {
					nv04_pgraph_set_bitmap_color_0_nv01(&exp, val);
				} else {
					exp.bitmap_color[0] = val;
					bool likes_format = false;
					if (nv04_pgraph_is_nv17p(&chipset) || (chipset.chipset >= 5 && !nv04_pgraph_is_nv15p(&chipset)))
						likes_format = true;
					if (!likes_format)
						insrt(exp.ctx_format, 0, 8, extr(exp.ctx_switch[1], 8, 8));
				}
			}
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
	bool supported() override { return chipset.card_type < 4; }
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
	return chipset.card_type < 0x20;
}

Test::Subtests PGraphMthdSimpleTests::subtests() {
	return {
		{"beta", new MthdBetaTest(opt, rnd())},
		{"beta4", new MthdBeta4Test(opt, rnd())},
		{"rop", new MthdRopTest(opt, rnd())},
		{"chroma_plane", new MthdChromaPlaneTest(opt, rnd())},
		{"pattern_shape", new MthdPatternShapeTest(opt, rnd())},
		{"pattern_select", new MthdPatternSelectTest(opt, rnd())},
		{"pattern_mono_color", new MthdPatternMonoColorTest(opt, rnd())},
		{"pattern_mono_bitmap", new MthdPatternMonoBitmapTest(opt, rnd())},
		{"pattern_color_y8", new MthdPatternColorY8Test(opt, rnd())},
		{"pattern_color_r5g6b5", new MthdPatternColorR5G6B5Test(opt, rnd())},
		{"pattern_color_r5g5b5", new MthdPatternColorR5G5B5Test(opt, rnd())},
		{"pattern_color_r8g8b8", new MthdPatternColorR8G8B8Test(opt, rnd())},
		{"solid_color", new MthdSolidColorTest(opt, rnd())},
		{"bitmap_color", new MthdBitmapColorTest(opt, rnd())},
		{"subdivide", new MthdSubdivideTest(opt, rnd())},
		{"vtx_beta", new MthdVtxBetaTest(opt, rnd())},
		{"ixm_pitch", new MthdIxmPitchTest(opt, rnd())},
	};
}

}
}
