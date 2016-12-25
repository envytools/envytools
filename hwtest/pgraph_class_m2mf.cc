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

namespace {

class MthdM2mfOffset : public SingleMthdTest {
	int which;
	void emulate_mthd() override {
		exp.dma_offset[which] = val;
		insrt(exp.valid[0], which, 1, 1);
	}
public:
	MthdM2mfOffset(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdM2mfPitch : public SingleMthdTest {
	int which;
	bool is_valid_val() override {
		return !(val & ~0x7fff);
	}
	void emulate_mthd_pre() override {
		insrt(exp.dma_pitch, 16 * which, 16, val);
	}
	void emulate_mthd() override {
		insrt(exp.valid[0], which + 2, 1, 1);
	}
public:
	MthdM2mfPitch(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdM2mfLineLength : public SingleMthdTest {
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
	using SingleMthdTest::SingleMthdTest;
};

class MthdM2mfLineCount : public SingleMthdTest {
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
	using SingleMthdTest::SingleMthdTest;
};

class MthdM2mfFormat : public SingleMthdTest {
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
	using SingleMthdTest::SingleMthdTest;
};

class MthdM2mfTrigger : public SingleMthdTest {
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
				if (!sync)
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
	using SingleMthdTest::SingleMthdTest;
};

}

std::vector<SingleMthdTest *> M2mf::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdDmaGrobj(opt, rnd(), "dma_in", 2, cls, 0x184, 0, DMA_R | DMA_CLR),
		new MthdDmaGrobj(opt, rnd(), "dma_out", 3, cls, 0x188, 1, DMA_W),
		new MthdM2mfOffset(opt, rnd(), "offset_in", 4, cls, 0x30c, 0),
		new MthdM2mfOffset(opt, rnd(), "offset_out", 5, cls, 0x310, 1),
		new MthdM2mfPitch(opt, rnd(), "pitch_in", 6, cls, 0x314, 0),
		new MthdM2mfPitch(opt, rnd(), "pitch_out", 7, cls, 0x318, 1),
		new MthdM2mfLineLength(opt, rnd(), "line_length", 8, cls, 0x31c),
		new MthdM2mfLineCount(opt, rnd(), "line_count", 9, cls, 0x320),
		new MthdM2mfFormat(opt, rnd(), "format", 10, cls, 0x324),
		new MthdM2mfTrigger(opt, rnd(), "trigger", 11, cls, 0x328),
	};
}

}
}
