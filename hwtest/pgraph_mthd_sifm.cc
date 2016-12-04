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
	MthdSifmOffsetTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
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
	MthdSifmPitchTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
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
	MthdSifmInvalidTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

}

bool PGraphMthdSifmTests::supported() {
	return chipset.card_type >= 3 && chipset.card_type < 4;
}

Test::Subtests PGraphMthdSifmTests::subtests() {
	return {
		{"offset", new MthdSifmOffsetTest(opt, rnd())},
		{"pitch", new MthdSifmPitchTest(opt, rnd())},
		{"invalid", new MthdSifmInvalidTest(opt, rnd())},
	};
}

}
}
