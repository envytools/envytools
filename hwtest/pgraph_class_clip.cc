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

void MthdClipXy::emulate_mthd() {
	if (chipset.card_type < 3) {
		nv01_pgraph_set_clip(&exp, 0, val);
	} else if (chipset.card_type < 4) {
		nv03_pgraph_set_clip(&exp, which_clip, is_max, val, extr(orig.xy_misc_1[0], 0, 1));
	} else {
		nv04_pgraph_set_clip(&exp, which_clip, is_max, val);
	}
}

void MthdClipRect::emulate_mthd() {
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
		nv03_pgraph_set_clip(&exp, which_clip, 2, val, extr(orig.xy_misc_1[0], 0, 1));
	} else {
		nv04_pgraph_set_clip(&exp, which_clip, 2, val);
	}
}

bool MthdClipHv::is_valid_val() {
	return !extr(val, 15, 1);
}

void MthdClipHv::emulate_mthd() {
	int vidx = chipset.card_type < 0x10 ? 13 : 9;
	if (which_clip == 0) {
		insrt(exp.valid[0], 28, 1, 0);
		insrt(exp.valid[0], 30, 1, 0);
	} else if (which_clip == 1) {
		insrt(exp.valid[0], 29, 1, 0);
		insrt(exp.valid[0], 31, 1, 0);
		insrt(exp.valid[0], 20, 1, 1);
	} else {
		insrt(exp.valid[1], 30, 1, 0);
		insrt(exp.valid[1], 31, 1, 0);
	}
	insrt(exp.valid[0], 19, 1, 0);
	if (which_clip < 2) {
		insrt(exp.xy_misc_1[which_clip], 4+which_hv, 1, 0);
		insrt(exp.xy_misc_1[which_clip], 12, 1, 0);
		insrt(exp.xy_misc_1[which_clip], 16, 1, 0);
		insrt(exp.xy_misc_1[which_clip], 20, 1, 0);
	}
	insrt(exp.xy_misc_1[1], 0, 1, 1);
	insrt(exp.xy_misc_1[0], 0, 1, 0);
	insrt(exp.xy_misc_3, 8, 1, 0);
	uint32_t min = extr(val, 0, 16);
	uint32_t max;
	if (chipset.card_type < 0x10)
		max = min + extrs(val, 16, 16);
	else
		max = min + extr(val, 16, 16);
	exp.vtx_xy[vidx][which_hv] = min;
	int cstat = nv04_pgraph_clip_status(&exp, min, which_hv);
	pgraph_set_xy_d(&exp, which_hv, 0, 0, false, min != extrs(min, 0, 16), false, cstat);
	if (!which_hv) {
		exp.uclip_min[which_clip][which_hv] = exp.uclip_max[which_clip][which_hv] & 0xffff;
		exp.uclip_max[which_clip][which_hv] = min & 0x3ffff;
		exp.vtx_xy[vidx][0] = exp.vtx_xy[vidx][1] = max;
		if (chipset.card_type < 0x10) {
			int xcstat = nv04_pgraph_clip_status(&exp, max, 0);
			int ycstat = nv04_pgraph_clip_status(&exp, max, 1);
			pgraph_set_xy_d(&exp, 0, 1, 1, false, false, false, xcstat);
			pgraph_set_xy_d(&exp, 1, 1, 1, false, false, false, ycstat);
		}
	}
	exp.uclip_min[which_clip][which_hv] = min;
	exp.uclip_max[which_clip][which_hv] = max & 0x3ffff;
}

std::vector<SingleMthdTest *> Clip::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdClipXy(opt, rnd(), "clip_xy", 2, cls, 0x300, 0, 0),
		new MthdClipRect(opt, rnd(), "clip_rect", 3, cls, 0x304, 0),
	};
}

}
}
