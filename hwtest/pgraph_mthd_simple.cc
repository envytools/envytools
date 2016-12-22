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
#include "nva.h"

namespace hwtest {
namespace pgraph {

void MthdSolidColor::emulate_mthd() {
	exp.misc32[0] = val;
	if (chipset.card_type >= 3)
		insrt(exp.valid[0], 16, 1, 1);
}

namespace {

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
			switch (rnd() % 7) {
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
		{"solid_color", new MthdSolidColorTest(opt, rnd())},
		{"bitmap_color", new MthdBitmapColorTest(opt, rnd())},
		{"subdivide", new MthdSubdivideTest(opt, rnd())},
		{"vtx_beta", new MthdVtxBetaTest(opt, rnd())},
		{"ixm_pitch", new MthdIxmPitchTest(opt, rnd())},
	};
}

}
}
