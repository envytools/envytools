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

std::vector<SingleMthdTest *> EmuCelsius::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", 2, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", 1, cls, 0x110),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 3, cls, 0x180),
		new UntestedMthd(opt, rnd(), "dma_tex_a", 4, cls, 0x184), // XXX
		new UntestedMthd(opt, rnd(), "dma_tex_b", 5, cls, 0x188), // XXX
		new UntestedMthd(opt, rnd(), "dma_vtx", 6, cls, 0x18c), // XXX
		new UntestedMthd(opt, rnd(), "dma_state", 7, cls, 0x190), // XXX
		new MthdDmaSurf(opt, rnd(), "dma_surf_color", 8, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", 9, cls, 0x198, 3, SURF_NV10),
		new UntestedMthd(opt, rnd(), "clip_h", 10, cls, 0x200), // XXX
		new UntestedMthd(opt, rnd(), "clip_v", 10, cls, 0x204), // XXX
		new MthdSurf3DFormat(opt, rnd(), "surf_format", 11, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", 12, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_offset", 13, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", 14, cls, 0x214, 3, SURF_NV10),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x218, 2), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x220, 8), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x240, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x280, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x300, 0x40), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x400, 0x100), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x800, 0x200), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1000, 0x400), // XXX
	};
	if (cls == 0x56) {
	} else {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "unk114", -1, cls, 0x114), // XXX
			new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
			new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
			new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
			new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
			new UntestedMthd(opt, rnd(), "flip_unk130", -1, cls, 0x130),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Kelvin::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", -1, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", -1, cls, 0x110),
		new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
		new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
		new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
		new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
		new UntestedMthd(opt, rnd(), "flip_unk130", -1, cls, 0x130),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", -1, cls, 0x180),
		new UntestedMthd(opt, rnd(), "dma_tex_a", -1, cls, 0x184), // XXX
		new UntestedMthd(opt, rnd(), "dma_tex_b", -1, cls, 0x188), // XXX
		new UntestedMthd(opt, rnd(), "dma_state", -1, cls, 0x190), // XXX
		new MthdDmaSurf(opt, rnd(), "dma_surf_color", -1, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", -1, cls, 0x198, 3, SURF_NV10),
		new UntestedMthd(opt, rnd(), "dma_vtx_a", -1, cls, 0x19c), // XXX
		new UntestedMthd(opt, rnd(), "dma_vtx_b", -1, cls, 0x1a0), // XXX
		new UntestedMthd(opt, rnd(), "dma_fence", -1, cls, 0x1a4), // XXX
		new UntestedMthd(opt, rnd(), "dma_query", -1, cls, 0x1a8), // XXX
		new UntestedMthd(opt, rnd(), "clip_h", -1, cls, 0x200), // XXX
		new UntestedMthd(opt, rnd(), "clip_v", -1, cls, 0x204), // XXX
		new MthdSurf3DFormat(opt, rnd(), "surf_format", -1, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", -1, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_offset", -1, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", -1, cls, 0x214, 3, SURF_NV10),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x218, 2), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x220, 8), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x240, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x280, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x300, 0x40), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x400, 0x100), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x800, 0x200), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1000, 0x400), // XXX
	};
	if (cls == 0x597) {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "dma_clipid", -1, cls, 0x1ac),
			new UntestedMthd(opt, rnd(), "dma_zcull", -1, cls, 0x1b0),
		});
	}
	return res;
}

}
}
