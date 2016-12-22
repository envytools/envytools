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

class MthdSurfFormat : public SingleMthdTest {
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
		if (chipset.card_type < 4) {
			int which = extr(exp.ctx_switch[0], 16, 2);
			int f = 1;
			if (extr(val, 0, 1))
				f = 0;
			if (!extr(val, 16, 1))
				f = 2;
			if (!extr(val, 24, 1))
				f = 3;
			insrt(exp.surf_format, 4*which, 3, f | 4);
		} else {
			int which = cls & 3;
			int fmt = 0;
			if (val == 1) {
				fmt = 7;
			} else if (val == 0x01010000) {
				fmt = 1;
			} else if (val == 0x01000000) {
				fmt = 2;
			} else if (val == 0x01010001) {
				fmt = 6;
			}
			insrt(exp.surf_format, which*4, 4, fmt);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurfPitch : public SingleMthdTest {
	bool is_valid_val() override {
		if (chipset.card_type < 4)
			return true;
		return val && !(val & ~0x1ff0);
	}
	void emulate_mthd() override {
		int which;
		if (chipset.card_type < 4) {
			which = extr(exp.ctx_switch[0], 16, 2);
		} else {
			which = cls & 3;
			insrt(exp.valid[0], 2, 1, 1);
			insrt(exp.ctx_valid, 8+which, 1, !extr(exp.nsource, 1, 1));
		}
		exp.surf_pitch[which] = val & pgraph_pitch_mask(&chipset);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurfOffset : public SingleMthdTest {
	bool is_valid_val() override {
		if (chipset.card_type < 4)
			return true;
		return !(val & 0xf);
	}
	void emulate_mthd() override {
		int which;
		if (chipset.card_type < 4) {
			which = extr(exp.ctx_switch[0], 16, 2);
		} else {
			which = cls & 3;
			insrt(exp.valid[0], 3, 1, 1);
		}
		exp.surf_offset[which] = val & pgraph_offset_mask(&chipset);
	}
	using SingleMthdTest::SingleMthdTest;
};

}

std::vector<SingleMthdTest *> Surf::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new UntestedMthd(opt, rnd(), "dma_surf", 2, cls, 0x184), // XXX
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 2),
		new MthdSurfFormat(opt, rnd(), "format", 3, cls, 0x300),
		new MthdSurfPitch(opt, rnd(), "pitch", 4, cls, 0x308),
		new MthdSurfOffset(opt, rnd(), "offset", 5, cls, 0x30c),
	};
}

}
}
