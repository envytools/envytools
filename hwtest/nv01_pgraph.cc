/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "hwtest.h"
#include "pgraph.h"
#include "nvhw/pgraph.h"
#include "nvhw/chipset.h"
#include "nva.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * TODO:
 *  - cliprects
 *  - point VTX methods
 *  - lin/line VTX methods
 *  - tri VTX methods
 *  - rect VTX methods
 *  - tex* COLOR methods
 *  - ifc/bitmap VTX methods
 *  - ifc COLOR methods
 *  - bitmap MONO methods
 *  - blit VTX methods
 *  - blit pixel ops
 *  - tex* pixel ops
 *  - point rasterization
 *  - line rasterization
 *  - lin rasterization
 *  - tri rasterization
 *  - rect rasterization
 *  - texlin calculations
 *  - texquad calculations
 *  - quad rasterization
 *  - blit/ifc/bitmap rasterization
 *  - itm methods
 *  - ifm methods
 *  - notify
 *  - range interrupt
 *  - interrupt handling
 *  - ifm rasterization
 *  - ifm dma
 *  - itm rasterization
 *  - itm dma
 *  - itm pixel ops
 *  - cleanup & refactor everything
 */

namespace {

using namespace hwtest::pgraph;

class RopSolidTest : public MthdTest {
	int x, y;
	uint32_t addr[2], pixel[2], epixel[2], rpixel[2];
	int repeats() override { return 100000; };
	void adjust_orig_mthd() override {
		x = rnd() & 0x3ff;
		y = rnd() & 0xff;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000400;
		/* XXX bits 8-9 affect rendering */
		/* XXX bits 12-19 affect xy_misc_4 clip status */
		orig.xy_misc_1[0] &= 0xfff00cff;
		/* avoid invalid ops */
		orig.ctx_switch[0] &= ~0x001f;
		if (rnd()&1) {
			int ops[] = {
				0x00, 0x0f,
				0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
				0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x17,
			};
			orig.ctx_switch[0] |= ops[rnd() % ARRAY_SIZE(ops)];
		} else {
			/* BLEND needs more testing */
			int ops[] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c };
			orig.ctx_switch[0] |= ops[rnd() % ARRAY_SIZE(ops)];
			/* XXX Y8 blend? */
			orig.pfb_config |= 0x200;
		}
		orig.pattern_config = rnd()%3; /* shape 3 is a rather ugly hole in Karnough map */
		if (rnd()&1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		/* XXX causes interrupts */
		orig.valid[0] &= ~0x11000000;
		insrt(orig.access, 12, 5, 8);
		insrt(orig.pfb_config, 4, 3, 3);
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 0, 16, x);
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 16, 16, y);
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 0, 16, x);
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 16, 16, y);
		if (rnd()&1) {
			/* it's vanishingly rare for the chroma key to match perfectly by random, so boost the odds */
			uint32_t ckey = pgraph_to_a1r10g10b10(pgraph_expand_color(&orig, orig.misc32[0]));
			ckey ^= (rnd() & 1) << 30; /* perturb alpha */
			if (rnd()&1) {
				/* perturb it a bit to check which bits have to match */
				ckey ^= 1 << (rnd() % 30);
			}
			orig.chroma = ckey;
		}
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			addr[i] = nv01_pgraph_pixel_addr(&orig, x, y, i);
			nva_wr32(cnum, 0x1000000+(addr[i]&~3), rnd());
			pixel[i] = nva_rd32(cnum, 0x1000000+(addr[i]&~3)) >> (addr[i] & 3) * 8;
			pixel[i] &= bflmask(nv01_pgraph_cpp(orig.pfb_config)*8);
		}
		val = y << 16 | x;
	}
	void choose_mthd() override {
		cls = 0x08;
		mthd = 0x400;
	}
	void emulate_mthd() override {
		int bfmt = extr(orig.ctx_switch[0], 9, 4);
		int bufmask = (bfmt / 5 + 1) & 3;
		if (!extr(orig.pfb_config, 12, 1))
			bufmask = 1;
		bool cliprect_pass = pgraph_cliprect_pass(&exp, x, y);
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			if (bufmask & (1 << i) && (cliprect_pass || (i == 1 && extr(exp.canvas_config, 4, 1))))
				epixel[i] = nv01_pgraph_solid_rop(&exp, x, y, pixel[i]);
			else
				epixel[i] = pixel[i];
		}
		exp.vtx_xy[0][0] = x;
		exp.vtx_xy[0][1] = y;
		pgraph_clear_vtxid(&exp);
		pgraph_bump_vtxid(&exp);
		exp.xy_misc_1[0] &= ~0x03000001;
		exp.xy_misc_1[0] |= 0x01000000;
		nv01_pgraph_set_vtx(&exp, 0, 0, extrs(val, 0, 16), false);
		nv01_pgraph_set_vtx(&exp, 1, 0, extrs(val, 16, 16), false);
		exp.valid[0] &= ~0xffffff;
		if (extr(exp.cliprect_ctrl, 8, 1)) {
			exp.intr |= 1 << 24;
			exp.access &= ~0x101;
		}
		if (extr(exp.canvas_config, 24, 1)) {
			exp.intr |= 1 << 20;
			exp.access &= ~0x101;
		}
		if (extr(exp.xy_misc_4[0], 4, 4) || extr(exp.xy_misc_4[1], 4, 4)) {
			exp.intr |= 1 << 12;
			exp.access &= ~0x101;
		}
		if (exp.intr & 0x01101000) {
			for (int i = 0; i < 2; i++) {
				epixel[i] = pixel[i];
			}
		}
	}
	bool other_fail() override {
		bool res = false;
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			rpixel[i] = nva_rd32(cnum, 0x1000000+(addr[i]&~3)) >> (addr[i] & 3) * 8;
			rpixel[i] &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
			if (rpixel[i] != epixel[i]) {
			
				printf("Difference in PIXEL[%d]: expected %08x real %08x\n", i, epixel[i], rpixel[i]);
				res = true;
			}
		}
		return res;
	}
	void print_fail() override {
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			printf("%08x %08x %08x PIXEL[%d]\n", pixel[i], epixel[i], rpixel[i], i);
		}
		MthdTest::print_fail();
	}
public:
	RopSolidTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class RopBlitTest : public MthdTest {
	int x, y, sx, sy;
	uint32_t addr[2], saddr[2], pixel[2], epixel[2], rpixel[2], spixel[2];
	int repeats() override { return 100000; };
	void adjust_orig_mthd() override {
		x = rnd() & 0x1ff;
		y = rnd() & 0xff;
		sx = (rnd() & 0x3ff) + 0x200;
		sy = rnd() & 0xff;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000400;
		/* XXX bits 8-9 affect rendering */
		/* XXX bits 12-19 affect xy_misc_4 clip status */
		orig.xy_misc_1[0] &= 0xfff00cff;
		/* avoid invalid ops */
		orig.ctx_switch[0] &= ~0x001f;
		if (rnd()&1) {
			int ops[] = {
				0x00, 0x0f,
				0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
				0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x17,
			};
			orig.ctx_switch[0] |= ops[rnd() % ARRAY_SIZE(ops)];
		} else {
			/* BLEND needs more testing */
			int ops[] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c };
			orig.ctx_switch[0] |= ops[rnd() % ARRAY_SIZE(ops)];
			/* XXX Y8 blend? */
			orig.pfb_config |= 0x200;
		}
		orig.pattern_config = rnd()%3; /* shape 3 is a rather ugly hole in Karnough map */
		if (rnd()&1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		orig.xy_misc_4[0] &= ~0xf000;
		orig.xy_misc_4[1] &= ~0xf000;
		orig.valid[0] &= ~0x11000000;
		orig.valid[0] |= 0xf10f;
		insrt(orig.access, 12, 5, 0x10);
		insrt(orig.pfb_config, 4, 3, 3);
		orig.vtx_xy[0][0] = sx;
		orig.vtx_xy[0][1] = sy;
		orig.vtx_xy[1][0] = x;
		orig.vtx_xy[1][1] = y;
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 0, 16, x);
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 16, 16, y);
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 0, 16, x);
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 16, 16, y);
		if (rnd()&1) {
			/* it's vanishingly rare for the chroma key to match perfectly by random, so boost the odds */
			uint32_t ckey = pgraph_to_a1r10g10b10(pgraph_expand_color(&orig, orig.misc32[0]));
			ckey ^= (rnd() & 1) << 30; /* perturb alpha */
			if (rnd()&1) {
				/* perturb it a bit to check which bits have to match */
				ckey ^= 1 << (rnd() % 30);
			}
			orig.chroma = ckey;
		}
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			addr[i] = nv01_pgraph_pixel_addr(&orig, x, y, i);
			saddr[i] = nv01_pgraph_pixel_addr(&orig, sx, sy, i);
			nva_wr32(cnum, 0x1000000+(addr[i]&~3), rnd());
			nva_wr32(cnum, 0x1000000+(saddr[i]&~3), rnd());
			pixel[i] = nva_rd32(cnum, 0x1000000+(addr[i]&~3)) >> (addr[i] & 3) * 8;
			spixel[i] = nva_rd32(cnum, 0x1000000+(saddr[i]&~3)) >> (saddr[i] & 3) * 8;
			pixel[i] &= bflmask(nv01_pgraph_cpp(orig.pfb_config)*8);
			spixel[i] &= bflmask(nv01_pgraph_cpp(orig.pfb_config)*8);
			if (sx >= 0x400)
				spixel[i] = 0;
			if (!pgraph_cliprect_pass(&orig, sx, sy) && (i == 0 || !extr(orig.canvas_config, 4, 1))) {
				spixel[i] = 0;
			}
		}
		if (!extr(orig.pfb_config, 12, 1))
			spixel[1] = spixel[0];
		val = 1 << 16 | 1;
	}
	void choose_mthd() override {
		cls = 0x10;
		mthd = 0x308;
	}
	void emulate_mthd() override {
		int bfmt = extr(exp.ctx_switch[0], 9, 4);
		int bufmask = (bfmt / 5 + 1) & 3;
		if (!extr(exp.pfb_config, 12, 1))
			bufmask = 1;
		bool cliprect_pass = pgraph_cliprect_pass(&exp, x, y);
		struct pgraph_color s = nv01_pgraph_expand_surf(&exp, spixel[extr(exp.ctx_switch[0], 13, 1)]);
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			if (bufmask & (1 << i) && (cliprect_pass || (i == 1 && extr(exp.canvas_config, 4, 1))))
				epixel[i] = nv01_pgraph_rop(&exp, x, y, pixel[i], s);
			else
				epixel[i] = pixel[i];
		}
		nv01_pgraph_vtx_fixup(&exp, 0, 2, 1, 1, 0, 2);
		nv01_pgraph_vtx_fixup(&exp, 1, 2, 1, 1, 0, 2);
		nv01_pgraph_vtx_fixup(&exp, 0, 3, 1, 1, 1, 3);
		nv01_pgraph_vtx_fixup(&exp, 1, 3, 1, 1, 1, 3);
		pgraph_bump_vtxid(&exp);
		pgraph_bump_vtxid(&exp);
		exp.valid[0] &= ~0xffffff;
		if (extr(exp.cliprect_ctrl, 8, 1)) {
			exp.intr |= 1 << 24;
			exp.access &= ~0x101;
		}
		if (extr(exp.canvas_config, 24, 1)) {
			exp.intr |= 1 << 20;
			exp.access &= ~0x101;
		}
		if (extr(exp.xy_misc_4[0], 4, 4) || extr(exp.xy_misc_4[1], 4, 4)) {
			exp.intr |= 1 << 12;
			exp.access &= ~0x101;
		}
		if (exp.intr & 0x01101000) {
			for (int i = 0; i < 2; i++) {
				epixel[i] = pixel[i];
			}
		}
	}
	bool other_fail() override {
		bool res = false;
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			rpixel[i] = nva_rd32(cnum, 0x1000000+(addr[i]&~3)) >> (addr[i] & 3) * 8;
			rpixel[i] &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
			if (rpixel[i] != epixel[i]) {
			
				printf("Difference in PIXEL[%d]: expected %08x real %08x\n", i, epixel[i], rpixel[i]);
				res = true;
			}
		}
		return res;
	}
	void print_fail() override {
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			printf("%08x %08x %08x PIXEL[%d]\n", pixel[i], epixel[i], rpixel[i], i);
		}
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			printf("%08x SPIXEL[%d]\n", spixel[i], i);
		}
		MthdTest::print_fail();
	}
public:
	RopBlitTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

}

namespace hwtest {
namespace pgraph {


bool PGraphRopTests::supported() {
	return chipset.card_type < 3;
}

Test::Subtests PGraphRopTests::subtests() {
	return {
		{"solid", new RopSolidTest(opt, rnd())},
		{"blit", new RopBlitTest(opt, rnd())},
	};
}

}
}
