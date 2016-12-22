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
		cls = chipset.card_type < 4 ? 0x0d : 0x39;;
		mthd = 0x30c + idx * 4;
		trapbit = 4 + idx;
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
		cls = chipset.card_type < 4 ? 0x0d : 0x39;;
		mthd = 0x314 + idx * 4;
		trapbit = 6 + idx;
	}
	bool is_valid_val() override {
		return !(val & ~0x7fff);
	}
	void emulate_mthd_pre() override {
		insrt(exp.dma_pitch, 16 * idx, 16, val);
	}
	void emulate_mthd() override {
		insrt(exp.valid[0], idx + 2, 1, 1);
	}
public:
	MthdM2mfPitchTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdM2mfLineLengthTest : public MthdTest {
	void choose_mthd() override {
		cls = chipset.card_type < 4 ? 0x0d : 0x39;;
		mthd = 0x31c;
		trapbit = 8;
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
		if (chipset.card_type < 4)
			exp.misc24[0] = val & 0xffffff;
		else
			exp.dma_length = val & 0x3fffff;
		insrt(exp.valid[0], 4, 1, 1);
	}
public:
	MthdM2mfLineLengthTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdM2mfLineCountTest : public MthdTest {
	void choose_mthd() override {
		cls = chipset.card_type < 4 ? 0x0d : 0x39;;
		mthd = 0x320;
		trapbit = 9;
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
		if (chipset.card_type < 4)
			exp.dma_eng_lines[0] = val & 0x7ff;
		else
			insrt(exp.dma_misc, 0, 16, val & 0x7ff);
		insrt(exp.valid[0], 5, 1, 1);
	}
public:
	MthdM2mfLineCountTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdM2mfFormatTest : public MthdTest {
	void choose_mthd() override {
		cls = chipset.card_type < 4 ? 0x0d : 0x39;;
		mthd = 0x324;
		trapbit = 10;
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
		if (chipset.chipset >= 5 && val & ~0x7ff)
			return false;
		return !(val & 0xf8);
	}
	void emulate_mthd() override {
		if (chipset.card_type < 4)
			exp.dma_misc = val & 0x707;
		else {
			insrt(exp.dma_misc, 16, 3, extr(val, 0, 3));
			insrt(exp.dma_misc, 20, 3, extr(val, 8, 3));
		}
		insrt(exp.valid[0], 6, 1, 1);
	}
public:
	MthdM2mfFormatTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdM2mfTriggerTest : public MthdTest {
	void choose_mthd() override {
		cls = chipset.card_type < 4 ? 0x0d : 0x39;;
		mthd = 0x328;
		trapbit = 11;
	}
	void adjust_orig_mthd() override {
		// XXX: disable this some day and test the actual DMA
		insrt(orig.valid[0], rnd() % 7, 1, 0);
		if (rnd() & 1) {
			subc = extr(orig.ctx_user, 13, 3);
			if (rnd() & 1)
				insrt(orig.ctx_switch[1], 16, 16, 0);
			else
				insrt(orig.ctx_switch[2], rnd() & 0x10, 16, 0);
			insrt(orig.debug[3], 24, 1, 1);
		}
	}
	void emulate_mthd() override {
		insrt(exp.valid[0], 7, 1, 1);
		if (chipset.card_type < 4) {
			exp.dma_offset[2] = val;
			pgraph_prep_draw(&exp, false, false);
		} else {
			if (!extr(exp.nsource, 1, 1) && !extr(exp.notify, 0, 1)) {
				insrt(exp.notify, 0, 1, 1);
				insrt(exp.notify, 8, 1, val);
			}
			// XXX do it right
			if (extr(exp.xy_misc_4[0], 4, 4) || extr(exp.xy_misc_4[1], 4, 4)) {
				nv04_pgraph_blowup(&exp, 0x0008);
			}
			nv04_pgraph_blowup(&exp, 0x4000);
			if (!extr(exp.ctx_switch[2], 0, 16) || !extr(exp.ctx_switch[2], 16, 16) || !extr(exp.ctx_switch[1], 16, 16))
				pgraph_state_error(&exp);
		}
	}
public:
	MthdM2mfTriggerTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

}

bool PGraphMthdM2mfTests::supported() {
	return chipset.card_type >= 3 && chipset.card_type < 0x20;
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
