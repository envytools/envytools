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

class MthdSurfFormatTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x1c;
		mthd = 0x300;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x01010003;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val == 1 || val == 0x01000000 || val == 0x01010000 || val == 0x01010001;
	}
	void emulate_mthd() override {
		int s = extr(exp.ctx_switch[0], 16, 2);
		int f = 1;
		if (extr(val, 0, 1))
			f = 0;
		if (!extr(val, 16, 1))
			f = 2;
		if (!extr(val, 24, 1))
			f = 3;
		insrt(exp.surf_format, 4*s, 3, f | 4);
	}
public:
	MthdSurfFormatTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSurfPitchTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x1c;
		mthd = 0x308;
	}
	void emulate_mthd() override {
		int s = extr(exp.ctx_switch[0], 16, 2);
		exp.surf_pitch[s] = val & 0x1ff0;
	}
public:
	MthdSurfPitchTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSurfOffsetTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x1c;
		mthd = 0x30c;
	}
	void emulate_mthd() override {
		int s = extr(exp.ctx_switch[0], 16, 2);
		exp.surf_offset[s] = val & pgraph_offset_mask(&chipset);
	}
public:
	MthdSurfOffsetTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

}

bool PGraphMthdSurfTests::supported() {
	return chipset.card_type >= 3 && chipset.card_type < 4;
}

Test::Subtests PGraphMthdSurfTests::subtests() {
	return {
		{"format", new MthdSurfFormatTest(opt, rnd())},
		{"pitch", new MthdSurfPitchTest(opt, rnd())},
		{"offset", new MthdSurfOffsetTest(opt, rnd())},
	};
}

}
}
