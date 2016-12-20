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

class MthdM2mfOffsetTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		idx = rnd() & 1;
		cls = 0x0d;
		mthd = 0x30c + idx * 4;
	}
	void emulate_mthd() override {
		exp.dma_offset[idx] = val;
		insrt(exp.valid[0], idx, 1, 1);
	}
public:
	MthdM2mfOffsetTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdM2mfPitchTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		idx = rnd() & 1;
		cls = 0x0d;
		mthd = 0x314 + idx * 4;
	}
	bool is_valid_val() override {
		return !(val & ~0x7fff);
	}
	void emulate_mthd() override {
		insrt(exp.dma_pitch, 16 * idx, 16, val);
		insrt(exp.valid[0], idx + 2, 1, 1);
	}
public:
	MthdM2mfPitchTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdM2mfLineLengthTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x0d;
		mthd = 0x31c;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffffff;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 0x400000;
	}
	void emulate_mthd() override {
		exp.misc24[0] = val & 0xffffff;
		insrt(exp.valid[0], 4, 1, 1);
	}
public:
	MthdM2mfLineLengthTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdM2mfLineCountTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x0d;
		mthd = 0x320;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xfff;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 0x800;
	}
	void emulate_mthd() override {
		exp.dma_eng_lines[0] = val & 0x7ff;
		insrt(exp.valid[0], 5, 1, 1);
	}
public:
	MthdM2mfLineCountTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdM2mfFormatTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x0d;
		mthd = 0x324;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0xf8;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		int a = extr(val, 0, 3);
		int b = extr(val, 8, 3);
		if (a != 1 && a != 2 && a != 4)
			return false;
		if (b != 1 && b != 2 && b != 4)
			return false;
		return !(val & 0xf8);
	}
	void emulate_mthd() override {
		exp.dma_misc = val & 0x707;
		insrt(exp.valid[0], 6, 1, 1);
	}
public:
	MthdM2mfFormatTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdM2mfTriggerTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x0d;
		mthd = 0x328;
	}
	void adjust_orig_mthd() override {
		// XXX: disable this some day and test the actual DMA
		insrt(orig.valid[0], rnd() % 7, 1, 0);
	}
	void emulate_mthd() override {
		exp.dma_offset[2] = val;
		insrt(exp.valid[0], 7, 1, 1);
		pgraph_prep_draw(&exp, false, false);
	}
public:
	MthdM2mfTriggerTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

}

bool PGraphMthdM2mfTests::supported() {
	return chipset.card_type >= 3 && chipset.card_type < 4;
}

Test::Subtests PGraphMthdM2mfTests::subtests() {
	return {
		{"offset", new MthdM2mfOffsetTest(opt, rnd())},
		{"pitch", new MthdM2mfPitchTest(opt, rnd())},
		{"line_length", new MthdM2mfLineLengthTest(opt, rnd())},
		{"line_count", new MthdM2mfLineCountTest(opt, rnd())},
		{"format", new MthdM2mfFormatTest(opt, rnd())},
		{"trigger", new MthdM2mfTriggerTest(opt, rnd())},
	};
}

}
}
