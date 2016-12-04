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

class MthdSifmVtxTest : public MthdTest {
	int repeats() override { return 10000; };
	void choose_mthd() override {
		cls = 0x0e;
		mthd = 0x310;
		if (!(rnd() & 3))
			val &= 0x001f000f;
	}
	void emulate_mthd() override {
		insrt(exp.valid[0], 0, 1, 1);
		insrt(exp.valid[0], 12, 1, 1);
		exp.vtx_xy[0][0] = extrs(val, 0, 16);
		exp.vtx_xy[4][1] = extrs(val, 16, 16);
		insrt(exp.valid[0], 19, 1, 0);
		pgraph_clear_vtxid(&exp);
		pgraph_bump_vtxid(&exp);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		int xcstat = nv03_pgraph_clip_status(&exp, exp.vtx_xy[0][0], 0, false);
		int ycstat = nv03_pgraph_clip_status(&exp, exp.vtx_xy[4][1], 1, false);
		insrt(exp.xy_clip[0][0], 0, 4, xcstat);
		insrt(exp.xy_clip[1][0], 0, 4, ycstat);
		insrt(exp.xy_misc_3, 8, 1, 0);
		insrt(exp.xy_misc_4[0], 0, 1, 0);
		insrt(exp.xy_misc_4[0], 4, 1, 0);
		insrt(exp.xy_misc_4[1], 0, 1, 0);
		insrt(exp.xy_misc_4[1], 4, 1, 0);
	}
public:
	MthdSifmVtxTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSifmRectTest : public MthdTest {
	int repeats() override { return 10000; };
	void choose_mthd() override {
		cls = 0x0e;
		mthd = 0x314;
		if (!(rnd() & 3))
			val &= 0x001f000f;
	}
	void emulate_mthd() override {
		insrt(exp.valid[0], 1, 1, 1);
		insrt(exp.valid[0], 9, 1, 1);
		nv03_pgraph_vtx_add(&exp, 0, 1, exp.vtx_xy[0][0], extr(val, 0, 16), 0, false);
		nv03_pgraph_vtx_add(&exp, 1, 1, 0, extr(val, 16, 16), 0, false);
		pgraph_bump_vtxid(&exp);
		nv03_pgraph_vtx_cmp(&exp, 0, 8, true);
		nv03_pgraph_vtx_cmp(&exp, 1, 8, true);
	}
public:
	MthdSifmRectTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSifmDuDxTest : public MthdTest {
	int repeats() override { return 10000; };
	void choose_mthd() override {
		cls = 0x0e;
		mthd = 0x318;
		if (!(rnd() & 3))
			val = sext(val, 21);
	}
	void emulate_mthd() override {
		int sv = val >> 17;
		if (sv >= 0x4000) {
			if (sv >= 0x7ff8)
				sv &= 0xf;
			else
				sv = 9;
		} else {
			if (sv > 7)
				sv = 7;
		}
		insrt(exp.valid[0], 4, 1, 1);
		exp.misc32[0] = val;
		insrt(exp.xy_misc_1[0], 24, 4, sv);
	}
public:
	MthdSifmDuDxTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSifmDvDyTest : public MthdTest {
	int repeats() override { return 10000; };
	void choose_mthd() override {
		cls = 0x0e;
		mthd = 0x31c;
		if (!(rnd() & 3))
			val = sext(val, 21);
	}
	void emulate_mthd() override {
		int sv = val >> 17;
		if (sv >= 0x4000) {
			if (sv >= 0x7ff8)
				sv &= 0xf;
			else
				sv = 9;
		} else {
			if (sv > 7)
				sv = 7;
		}
		insrt(exp.valid[0], 5, 1, 1);
		exp.vtx_xy[3][1] = val;
		insrt(exp.xy_misc_1[1], 24, 4, sv);
	}
public:
	MthdSifmDvDyTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSifmSrcSizeTest : public MthdTest {
	bool yuv420;
	int repeats() override { return 10000; };
	void choose_mthd() override {
		yuv420 = rnd() & 1;
		cls = 0x0e;
		mthd = 0x400;
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val &= ~0x7f0;
			val |= 0x400;
		}
		if (!(rnd() & 3)) {
			val &= ~0x7f00000;
			val |= 0x4000000;
		}
	}
	int gen_nv3_fmt() override {
		return yuv420 ? 7 : rnd() % 7;
	}
	bool is_valid_val() override {
		return (extr(val, 0, 16) <= 0x400 && extr(val, 16, 16) <= 0x400);
	}
	void emulate_mthd() override {
		exp.misc24[0] = extr(val, 0, 16);
		insrt(exp.misc24[1], 0, 11, extr(val, 0, 11));
		insrt(exp.misc24[1], 11, 11, extr(val, 16, 11));
		if (yuv420) {
			insrt(exp.valid[0], 9, 1, 0);
			insrt(exp.valid[0], 13, 1, 1);
		} else {
			insrt(exp.valid[0], 9, 1, 1);
			insrt(exp.valid[0], 13, 1, 0);
		}
	}
public:
	MthdSifmSrcSizeTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSifmOffsetTest : public MthdTest {
	int idx;
	bool yuv420;
	void choose_mthd() override {
		yuv420 = rnd() & 1;
		if (yuv420)
			idx = rnd() % 3;
		else
			idx = 0;
		cls = 0x0e;
		mthd = 0x408 + idx * 4;
	}
	int gen_nv3_fmt() override {
		return yuv420 ? 7 : rnd() % 7;
	}
	void emulate_mthd() override {
		exp.dma_offset[idx] = val;
		if (yuv420) {
			insrt(exp.valid[0], 11, 1, 0);
			insrt(exp.valid[0], 15 + idx, 1, 1);
		} else {
			insrt(exp.valid[0], 11, 1, 1);
			insrt(exp.valid[0], 15, 1, 0);
			insrt(exp.valid[0], 16, 1, 0);
			insrt(exp.valid[0], 17, 1, 0);
		}
	}
public:
	MthdSifmOffsetTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSifmPitchTest : public MthdTest {
	bool yuv420;
	void choose_mthd() override {
		yuv420 = rnd() & 1;
		cls = 0x0e;
		mthd = 0x404;
		if (rnd() & 1) {
			val &= 0x1fff;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	int gen_nv3_fmt() override {
		return yuv420 ? 7 : rnd() % 7;
	}
	bool is_valid_val() override {
		return yuv420 || !(val & ~0x1fff);
	}
	void emulate_mthd() override {
		if (yuv420) {
			exp.dma_pitch = val;
			insrt(exp.valid[0], 10, 1, 0);
			insrt(exp.valid[0], 14, 1, 1);
		} else {
			insrt(exp.dma_pitch, 0, 16, val);
			insrt(exp.valid[0], 10, 1, 1);
			insrt(exp.valid[0], 14, 1, 0);
		}
	}
public:
	MthdSifmPitchTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSifmInvalidTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x0e;
		mthd = 0x410 + (rnd() & 1) * 4;
	}
	int gen_nv3_fmt() override {
		return rnd() % 7;
	}
	void emulate_mthd() override {
		exp.intr |= 1;
		exp.invalid |= 1;
		exp.fifo_enable = 0;
	}
public:
	MthdSifmInvalidTest(TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

}

bool PGraphMthdSifmTests::supported() {
	return chipset.card_type >= 3 && chipset.card_type < 4;
}

Test::Subtests PGraphMthdSifmTests::subtests() {
	return {
		{"vtx", new MthdSifmVtxTest(opt, rnd())},
		{"rect", new MthdSifmRectTest(opt, rnd())},
		{"du_dx", new MthdSifmDuDxTest(opt, rnd())},
		{"dv_dy", new MthdSifmDvDyTest(opt, rnd())},
		{"src_size", new MthdSifmSrcSizeTest(opt, rnd())},
		{"offset", new MthdSifmOffsetTest(opt, rnd())},
		{"pitch", new MthdSifmPitchTest(opt, rnd())},
		{"invalid", new MthdSifmInvalidTest(opt, rnd())},
	};
}

}
}
