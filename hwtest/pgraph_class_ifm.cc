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

class MthdIxmPitch : public SingleMthdTest {
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
	using SingleMthdTest::SingleMthdTest;
};

class MthdIxmOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		// XXX: disable this some day and test the actual DMA
		if (chipset.card_type < 3) {
			switch (rnd() % 3) {
				case 0:
					insrt(orig.xy_misc_4[0], 4, 4, 0xf);
					break;
				case 1:
					insrt(orig.canvas_config, 24, 1, 1);
					break;
				case 2:
					insrt(orig.cliprect_ctrl, 8, 1, 1);
					break;
			}
		} else {
			switch (rnd() % 2) {
				case 0:
					insrt(orig.xy_misc_4[0], 4, 4, 0xf);
					insrt(orig.ctx_user, 13, 3, subc);
					break;
				case 1:
					insrt(orig.cliprect_ctrl, 8, 1, 1);
					break;
			}
		}
	}
	void emulate_mthd() override {
		int rcls = pgraph_class(&exp);
		if (chipset.card_type < 3) {
			exp.vtx_xy[5][0] = val;
			pgraph_clear_vtxid(&exp);
			exp.valid[0] |= 0x020020;
			if (rcls == 0x13) {
				if ((exp.valid[0] & 0x70070) != 0x70070)
					exp.intr |= 1 << 16;
			} else {
				if ((exp.valid[0] & 0x65065) != 0x65065)
					exp.intr |= 1 << 16;
			}
			if (exp.xy_misc_4[0] & 0xf0)
				exp.intr |= 1 << 12;
			if (exp.xy_misc_4[1] & 0xf0)
				exp.intr |= 1 << 12;
			if (exp.valid[0] & 0x11000000 && exp.ctx_switch[0] & 0x80)
				exp.intr |= 1 << 16;
			if (extr(exp.canvas_config, 24, 1))
				exp.intr |= 1 << 20;
			if (extr(exp.cliprect_ctrl, 8, 1))
				exp.intr |= 1 << 24;
			if (exp.intr)
				exp.access &= ~0x101;
			if (rcls == 0x0d || rcls == 0x0e || rcls == 0x1d || rcls == 0x1e) {
				insrt(exp.xy_a, 0, 12, 0);
				insrt(exp.xy_a, 16, 12, 0);
				insrt(exp.valid[0], 0, 24, 0);
			}
			if (rcls == 8 || rcls == 0x0c || rcls == 0x10 || rcls == 0x13 || rcls == 0x14) {
				insrt(exp.valid[0], 0, 24, 0);
			}
			if (rcls == 9 || rcls == 0xa || rcls == 0xb) {
				insrt(exp.valid[0], 0, 4, 0);
				insrt(exp.valid[0], 12, 4, 0);
			}
		} else {
			exp.vtx_xy[5][0] = 0;
			exp.dma_offset[0] = val;
			insrt(exp.valid[0], 5, 1, 1);
			insrt(exp.valid[0], 11, 1, 1);
			insrt(exp.valid[0], 13, 1, 1);
			insrt(exp.valid[0], 15, 1, 0);
			insrt(exp.valid[0], 16, 1, 0);
			insrt(exp.valid[0], 17, 1, 0);
			pgraph_clear_vtxid(&exp);
			pgraph_vtx_cmp(&exp, 1, 2, false);
			pgraph_prep_draw(&exp, false, false);
			if (!(exp.intr & 0x01110000) && !extr(exp.xy_a, 20, 1)) {
				nv03_pgraph_vtx_add(&exp, 0, 1, 0, 0, 0, false, false);
				nv03_pgraph_vtx_add(&exp, 1, 1, 0, 0, 0, false, false);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

std::vector<SingleMthdTest *> Ifm::mthds() {
	return {
		new UntestedMthd(opt, rnd(), "ifc_data", -1, cls, 0x40, 0x10), // XXX
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdVtxXy(opt, rnd(), "xy", -1, cls, 0x308, 1, 4, VTX_FIRST),
		new MthdIfcSize(opt, rnd(), "size_in", -1, cls, 0x30c, IFC_IN | IFC_IFM),
		new MthdIxmPitch(opt, rnd(), "pitch", -1, cls, 0x310),
		new MthdIxmOffset(opt, rnd(), "offset", -1, cls, 0x314),
	};
}

std::vector<SingleMthdTest *> Itm::mthds() {
	return {
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdVtxXy(opt, rnd(), "xy", -1, cls, 0x308, 1, 4, VTX_FIRST),
		new UntestedMthd(opt, rnd(), "rect", -1, cls, 0x30c), // XXX
		new MthdIxmPitch(opt, rnd(), "pitch", -1, cls, 0x310),
		new MthdIxmOffset(opt, rnd(), "offset", -1, cls, 0x314),
	};
}

}
}
