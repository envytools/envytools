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

class MthdClipXy : public SingleMthdTest {
	int repeats() override { return 10000; };
	void adjust_orig_mthd() override {
		if (!(rnd() & 3)) {
			val &= ~0xffff;
		}
		if (!(rnd() & 3)) {
			val &= ~0xffff0000;
		}
		if (rnd() & 1) {
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	void emulate_mthd() override {
		if (chipset.card_type < 3) {
			nv01_pgraph_set_clip(&exp, 0, val);
		} else if (chipset.card_type < 4) {
			nv03_pgraph_set_clip(&exp, 0, 0, val, extr(orig.xy_misc_1[0], 0, 1));
		} else {
			nv04_pgraph_set_clip(&exp, 0, 0, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdClipRect : public SingleMthdTest {
	int repeats() override { return 10000; };
	void adjust_orig_mthd() override {
		if (!(rnd() & 3)) {
			val &= ~0xffff;
		}
		if (!(rnd() & 3)) {
			val &= ~0xffff0000;
		}
		if (rnd() & 1) {
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	void emulate_mthd() override {
		if (chipset.card_type < 3) {
			if (pgraph_class(&exp) == 0x10) {
				int n = extr(exp.valid[0], 24, 1);
				insrt(exp.valid[0], 24, 1, 0);
				insrt(exp.valid[0], 28, 1, !n);
				exp.xy_misc_1[0] &= 0x03000331;
				nv01_pgraph_vtx_fixup(&exp, 0, 2, extr(val, 0, 16), 1, 0, 2);
				nv01_pgraph_vtx_fixup(&exp, 1, 2, extr(val, 16, 16), 1, 0, 2);
				nv01_pgraph_vtx_fixup(&exp, 0, 3, extr(val, 0, 16), 1, 1, 3);
				nv01_pgraph_vtx_fixup(&exp, 1, 3, extr(val, 16, 16), 1, 1, 3);
				pgraph_prep_draw(&exp, false, false);
			} else {
				nv01_pgraph_set_clip(&exp, 1, val);
			}
		} else if (chipset.card_type < 4) {
			nv03_pgraph_set_clip(&exp, 0, 1, val, extr(orig.xy_misc_1[0], 0, 1));
		} else {
			nv04_pgraph_set_clip(&exp, 0, 1, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

std::vector<SingleMthdTest *> Clip::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdClipXy(opt, rnd(), "clip_xy", 2, cls, 0x300),
		new MthdClipRect(opt, rnd(), "clip_rect", 3, cls, 0x304),
	};
}

}
}
