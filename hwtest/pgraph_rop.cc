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
#include "nva.h"

namespace {

using namespace hwtest::pgraph;

class RopSolidTest : public MthdTest {
	int x, y;
	uint32_t paddr[4], pixel[4], epixel[4], rpixel[4];
	int repeats() override { return 100000; };
	void adjust_orig_mthd() override {
		x = rnd() & 0xff;
		y = rnd() & 0xff;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000100;
		if (chipset.card_type < 3) {
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
				paddr[i] = nv01_pgraph_pixel_addr(&orig, x, y, i);
				nva_wr32(cnum, 0x1000000+(paddr[i]&~3), rnd());
				pixel[i] = nva_rd32(cnum, 0x1000000+(paddr[i]&~3)) >> (paddr[i] & 3) * 8;
				pixel[i] &= bflmask(nv01_pgraph_cpp(orig.pfb_config)*8);
			}
		} else {
			orig.ctx_user &= ~0xe000;
			orig.ctx_switch[0] &= ~0x8000;
			int op = extr(orig.ctx_switch[0], 24, 5);
			if (!((op >= 0x00 && op <= 0x15) || op == 0x17 || (op >= 0x19 && op <= 0x1a) || op == 0x1d))
				op = 0x17;
			insrt(orig.ctx_switch[0], 24, 5, op);
			orig.pattern_config = rnd() % 3; /* shape 3 is a rather ugly hole in Karnough map */
			insrt(orig.cliprect_ctrl, 8, 1, 0);
			orig.xy_misc_1[0] = 0;
			orig.xy_misc_1[1] = 0;
			orig.xy_misc_3 = 0;
			orig.xy_misc_4[0] = 0;
			orig.xy_misc_4[1] = 0;
			orig.valid[0] = 0x10000;
			orig.surf_offset[0] = 0x000000;
			orig.surf_offset[1] = 0x040000;
			orig.surf_offset[2] = 0x080000;
			orig.surf_offset[3] = 0x0c0000;
			orig.surf_pitch[0] = 0x0400;
			orig.surf_pitch[1] = 0x0400;
			orig.surf_pitch[2] = 0x0400;
			orig.surf_pitch[3] = 0x0400;
			if (op > 0x17)
				orig.surf_format |= 0x2222;
			orig.debug[3] &= ~(1 << 22);
			if (rnd()&1)
				insrt(orig.cliprect_min[rnd()&1], 0, 16, extr(val, 0, 8));
			if (rnd()&1)
				insrt(orig.cliprect_min[rnd()&1], 16, 16, extr(val, 16, 8));
			if (rnd()&1)
				insrt(orig.cliprect_max[rnd()&1], 0, 16, extr(val, 0, 8));
			if (rnd()&1)
				insrt(orig.cliprect_max[rnd()&1], 16, 16, extr(val, 16, 8));
			if (rnd()&1) {
				/* it's vanishingly rare for the chroma key to match perfectly by random, so boost the odds */
				uint32_t ckey;
				if ((nv03_pgraph_surf_format(&orig) & 3) == 0 && extr(orig.ctx_switch[0], 0, 3) == 4) {
					uint32_t save_ctxsw = orig.ctx_switch[0];
					orig.ctx_switch[0] &= ~7;
					ckey = pgraph_to_a1r10g10b10(pgraph_expand_color(&orig, orig.misc32[0]));
					orig.ctx_switch[0] = save_ctxsw;
				} else {
					ckey = pgraph_to_a1r10g10b10(pgraph_expand_color(&orig, orig.misc32[0]));
				}
				ckey ^= (rnd() & 1) << 30; /* perturb alpha */
				if (rnd()&1) {
					/* perturb it a bit to check which bits have to match */
					ckey ^= 1 << (rnd() % 30);
				}
				orig.chroma = ckey;
			}
			int cpp = nv03_pgraph_cpp(&orig);
			for (int j = 0; j < 4; j++) {
				paddr[j] = (x * cpp + y * 0x400 + j * 0x40000);
				pixel[j] = epixel[j] = rnd();
				nva_gwr32(nva_cards[cnum]->bar1, paddr[j] & ~3, pixel[j]);
			}
		}
		val = y << 16 | x;
	}
	void choose_mthd() override {
		cls = 0x08;
		mthd = 0x400 | (rnd() & 0x7c);
		subc = 0;
	}
	void emulate_mthd() override {
		if (chipset.card_type < 3) {
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
		} else {
			exp.vtx_xy[0][0] = x;
			exp.vtx_xy[0][1] = y;
			pgraph_clear_vtxid(&exp);
			pgraph_bump_vtxid(&exp);
			insrt(exp.xy_misc_1[1], 0, 1, 1);
			int xcstat = nv03_pgraph_clip_status(&exp, exp.vtx_xy[0][0], 0, false);
			int ycstat = nv03_pgraph_clip_status(&exp, exp.vtx_xy[0][1], 1, false);
			pgraph_set_xy_d(&exp, 0, 0, 0, false, false, false, xcstat);
			pgraph_set_xy_d(&exp, 1, 0, 0, false, false, false, ycstat);
			int cpp = nv03_pgraph_cpp(&orig);
			if (pgraph_cliprect_pass(&exp, x, y)) {
				for (int j = 0; j < 4; j++) {
					if (extr(exp.ctx_switch[0], 20 + j, 1)) {
						uint32_t src = extr(pixel[j], (paddr[j] & 3) * 8, cpp * 8);
						uint32_t res = nv03_pgraph_solid_rop(&exp, x, y, src);
						insrt(epixel[j], (paddr[j] & 3) * 8, cpp * 8, res);
					}
				}
			}
		}
	}
	bool other_fail() override {
		bool res = false;
		if (chipset.card_type < 3) {
			for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
				rpixel[i] = nva_rd32(cnum, 0x1000000+(paddr[i]&~3)) >> (paddr[i] & 3) * 8;
				rpixel[i] &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
				if (rpixel[i] != epixel[i]) {
				
					printf("Difference in PIXEL[%d]: expected %08x real %08x\n", i, epixel[i], rpixel[i]);
					res = true;
				}
			}
		} else {
			for (int j = 0; j < 4; j++) {
				rpixel[j] = nva_grd32(nva_cards[cnum]->bar1, paddr[j] & ~3);
				if (rpixel[j] != epixel[j]) {
					printf("Difference in PIXEL[%d]: expected %08x real %08x\n", j, epixel[j], rpixel[j]);
					res = true;
				}
			}
		}
		return res;
	}
	void print_fail() override {
		if (chipset.card_type < 3) {
			for (unsigned j = 0; j < 1 + extr(orig.pfb_config, 12, 1); j++) {
				printf("%08x %08x %08x PIXEL[%d]%s\n", pixel[j], epixel[j], rpixel[j], j, epixel[j] == rpixel[j] ? "" : " *");
			}
		} else {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x PIXEL[%d]%s\n", pixel[j], epixel[j], rpixel[j], j, epixel[j] == rpixel[j] ? "" : " *");
			}
		}
		MthdTest::print_fail();
	}
public:
	RopSolidTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class RopBlitTest : public MthdTest {
	int x, y, sx, sy;
	uint32_t paddr[4], spaddr[4], pixel[4], epixel[4], rpixel[4], spixel[4];
	int repeats() override { return 100000; };
	void adjust_orig_mthd() override {
		if (chipset.card_type < 3) {
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
				paddr[i] = nv01_pgraph_pixel_addr(&orig, x, y, i);
				spaddr[i] = nv01_pgraph_pixel_addr(&orig, sx, sy, i);
				nva_wr32(cnum, 0x1000000+(paddr[i]&~3), rnd());
				nva_wr32(cnum, 0x1000000+(spaddr[i]&~3), rnd());
				pixel[i] = nva_rd32(cnum, 0x1000000+(paddr[i]&~3)) >> (paddr[i] & 3) * 8;
				spixel[i] = nva_rd32(cnum, 0x1000000+(spaddr[i]&~3)) >> (spaddr[i] & 3) * 8;
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
		} else {
			orig.dst_canvas_min = 0;
			orig.dst_canvas_max = 0x01000080;
			orig.ctx_user &= ~0xe000;
			orig.ctx_switch[0] &= ~0x8000;
			int op = extr(orig.ctx_switch[0], 24, 5);
			if (!((op >= 0x00 && op <= 0x15) || op == 0x17 || (op >= 0x19 && op <= 0x1a) || op == 0x1d))
				op = 0x17;
			insrt(orig.ctx_switch[0], 24, 5, op);
			orig.pattern_config = rnd()%3; /* shape 3 is a rather ugly hole in Karnough map */
			// XXX: if source pixel hits cliprect, bad things happen
			orig.cliprect_ctrl = 0;
			orig.xy_misc_1[0] = 0;
			orig.xy_misc_1[1] = 0;
			orig.xy_misc_3 = 0;
			orig.xy_misc_4[0] = 0;
			orig.xy_misc_4[1] = 0;
			orig.xy_clip[0][0] = 0;
			orig.xy_clip[1][0] = 0;
			orig.valid[0] = 0x0303;
			orig.surf_offset[0] = 0x000000;
			orig.surf_offset[1] = 0x040000;
			orig.surf_offset[2] = 0x080000;
			orig.surf_offset[3] = 0x0c0000;
			orig.surf_pitch[0] = 0x0400;
			orig.surf_pitch[1] = 0x0400;
			orig.surf_pitch[2] = 0x0400;
			orig.surf_pitch[3] = 0x0400;
			if (op > 0x17)
				orig.surf_format |= 0x2222;
			orig.src_canvas_min = 0x00000080;
			orig.src_canvas_max = 0x01000100;
			x = rnd() & 0x7f;
			y = rnd() & 0xff;
			sx = rnd() & 0xff;
			sy = rnd() & 0xff;
			orig.vtx_xy[0][0] = sx;
			orig.vtx_xy[0][1] = sy;
			orig.vtx_xy[1][0] = x;
			orig.vtx_xy[1][1] = y;
			orig.debug[3] &= ~(1 << 22);
			int cpp = nv03_pgraph_cpp(&orig);
			for (int j = 0; j < 4; j++) {
				spaddr[j] = (sx * cpp + sy * 0x400 + j * 0x40000);
				spixel[j] = rnd();
				if (sx >= 0x80)
					nva_gwr32(nva_cards[cnum]->bar1, spaddr[j] & ~3, spixel[j]);
			}
			for (int j = 0; j < 4; j++) {
				paddr[j] = (x * cpp + y * 0x400 + j * 0x40000);
				pixel[j] = epixel[j] = rnd();
				nva_gwr32(nva_cards[cnum]->bar1, paddr[j] & ~3, pixel[j]);
			}
		}
		val = 1 << 16 | 1;
	}
	void choose_mthd() override {
		cls = 0x10;
		mthd = 0x308;
		subc = 0;
	}
	void emulate_mthd() override {
		if (chipset.card_type < 3) {
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
		} else {
			int cpp = nv03_pgraph_cpp(&orig);
			exp.valid[0] = 0;
			pgraph_bump_vtxid(&exp);
			pgraph_bump_vtxid(&exp);
			insrt(exp.xy_misc_1[1], 0, 1, 1);
			nv03_pgraph_vtx_add(&exp, 0, 2, exp.vtx_xy[0][0], extr(val, 0, 16), 0, false, false);
			nv03_pgraph_vtx_add(&exp, 1, 2, exp.vtx_xy[0][1], extr(val, 16, 16), 0, false, false);
			nv03_pgraph_vtx_add(&exp, 0, 3, exp.vtx_xy[1][0], extr(val, 0, 16), 0, false, false);
			nv03_pgraph_vtx_add(&exp, 1, 3, exp.vtx_xy[1][1], extr(val, 16, 16), 0, false, false);
			pgraph_vtx_cmp(&exp, 0, 8, true);
			pgraph_vtx_cmp(&exp, 1, 8, true);
			if (pgraph_cliprect_pass(&exp, x, y)) {
				int fmt = nv03_pgraph_surf_format(&exp) & 3;
				int ss = extr(exp.ctx_switch[0], 16, 2);
				uint32_t sp = extr(spixel[ss], (spaddr[ss] & 3) * 8, cpp * 8);
				if (sx < 0x80)
					sp = 0;
				if (!pgraph_cliprect_pass(&exp, sx, sy))
					sp = 0xffffffff;
				struct pgraph_color s = nv03_pgraph_expand_surf(fmt, sp);
				for (int j = 0; j < 4; j++) {
					if (extr(exp.ctx_switch[0], 20 + j, 1)) {
						uint32_t src = extr(pixel[j], (paddr[j] & 3) * 8, cpp * 8);
						uint32_t res = nv03_pgraph_rop(&exp, x, y, src, s);
						insrt(epixel[j], (paddr[j] & 3) * 8, cpp * 8, res);
					}
				}
			}
		}
	}
	bool other_fail() override {
		bool res = false;
		if (chipset.card_type < 3) {
			for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
				rpixel[i] = nva_rd32(cnum, 0x1000000+(paddr[i]&~3)) >> (paddr[i] & 3) * 8;
				rpixel[i] &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
				if (rpixel[i] != epixel[i]) {
				
					printf("Difference in PIXEL[%d]: expected %08x real %08x\n", i, epixel[i], rpixel[i]);
					res = true;
				}
			}
		} else {
			for (int j = 0; j < 4; j++) {
				rpixel[j] = nva_grd32(nva_cards[cnum]->bar1, paddr[j] & ~3);
				if (rpixel[j] != epixel[j]) {
					printf("Difference in PIXEL[%d]: expected %08x real %08x\n", j, epixel[j], rpixel[j]);
					res = true;
				}
			}
		}
		return res;
	}
	void print_fail() override {
		if (chipset.card_type < 3) {
			for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
				printf("%08x %08x %08x PIXEL[%d]\n", pixel[i], epixel[i], rpixel[i], i);
			}
			for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
				printf("%08x SPIXEL[%d]\n", spixel[i], i);
			}
		} else {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x PIXEL[%d]%s\n", pixel[j], epixel[j], rpixel[j], j, epixel[j] == rpixel[j] ? "" : " *");
			}
			for (int j = 0; j < 4; j++) {
				printf("%08x SPIXEL[%d]\n", spixel[j], j);
			}
		}
		MthdTest::print_fail();
	}
public:
	RopBlitTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class RopZPointTest : public MthdTest {
	int x, y;
	uint32_t paddr[4], pixel[4], epixel[4], rpixel[4];
	int repeats() override { return 100000; };
	bool supported() override { return chipset.card_type == 3; }
	void adjust_orig_mthd() override {
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000100;
		orig.ctx_user &= ~0xe000;
		orig.ctx_switch[0] &= ~0x8000;
		insrt(orig.cliprect_ctrl, 8, 1, 0);
		orig.xy_misc_1[0] = 0;
		orig.xy_misc_1[1] = 0;
		orig.xy_misc_3 = 0;
		orig.xy_misc_4[0] = 0;
		orig.xy_misc_4[1] = 0;
		orig.xy_clip[0][0] = 0;
		orig.xy_clip[1][0] = 0;
		orig.valid[0] = 0x4210101;
		orig.surf_offset[0] = 0x000000;
		orig.surf_offset[1] = 0x040000;
		orig.surf_offset[2] = 0x080000;
		orig.surf_offset[3] = 0x0c0000;
		orig.surf_pitch[0] = 0x0400;
		orig.surf_pitch[1] = 0x0400;
		orig.surf_pitch[2] = 0x0400;
		orig.surf_pitch[3] = 0x0400;
		orig.surf_format = 0x6666;
		if (extr(orig.d3d0_config, 20, 3) > 4)
			insrt(orig.d3d0_config, 22, 1, 0);
		orig.debug[2] &= 0xffdfffff;
		orig.debug[3] &= 0xfffeffff;
		orig.debug[3] &= 0xfffdffff;
		x = rnd() & 0xff;
		y = rnd() & 0xff;
		orig.vtx_xy[0][0] = x;
		orig.vtx_xy[0][1] = y;
		orig.debug[3] &= ~(1 << 22);
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 0, 16, extr(val, 0, 8));
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 16, 16, extr(val, 16, 8));
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 0, 16, extr(val, 0, 8));
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 16, 16, extr(val, 16, 8));
		int cpp = nv03_pgraph_cpp(&orig);
		for (int j = 0; j < 4; j++) {
			paddr[j] = (x * cpp + y * 0x400 + j * 0x40000);
			pixel[j] = epixel[j] = rnd();
			nva_gwr32(nva_cards[cnum]->bar1, paddr[j] & ~3, pixel[j]);
		}
		uint32_t zcur = extr(pixel[3], (paddr[3] & 3) * 8, cpp * 8);
		if (!(rnd() & 3)) {
			insrt(val, 16, 16, zcur);
		}
	}
	void choose_mthd() override {
		cls = 0x18;
		mthd = 0x804 | (rnd() & 0x7f8);
		subc = 0;
	}
	void emulate_mthd() override {
		int cpp = nv03_pgraph_cpp(&orig);
		uint32_t zcur = extr(pixel[3], (paddr[3] & 3) * 8, cpp * 8);
		insrt(exp.misc32[1], 16, 16, extr(val, 16, 16));
		nv03_pgraph_vtx_add(&exp, 0, 0, exp.vtx_xy[0][0], 1, 0, false, false);
		uint32_t zeta = extr(val, 16, 16);
		struct pgraph_color s = pgraph_expand_color(&exp, exp.misc32[0]);
		uint8_t ra = nv01_pgraph_dither_10to5(s.a << 2, x, y, false) >> 1;
		if (pgraph_cliprect_pass(&exp, x, y)) {
			bool zeta_test = nv03_pgraph_d3d_cmp(extr(exp.d3d0_config, 16, 4), zeta, zcur);
			if (!extr(exp.ctx_switch[0], 12, 1))
				zeta_test = true;
			bool alpha_test = nv03_pgraph_d3d_cmp(extr(exp.d3d0_alpha, 8, 4), extr(exp.misc32[0], 24, 8), extr(exp.d3d0_alpha, 0, 8));
			bool alpha_nz = !!ra;
			bool zeta_wr = nv03_pgraph_d3d_wren(extr(exp.d3d0_config, 20, 3), zeta_test, alpha_nz);
			bool color_wr = nv03_pgraph_d3d_wren(extr(exp.d3d0_config, 24, 3), zeta_test, alpha_nz);
			if (!alpha_test) {
				color_wr = false;
				zeta_wr = false;
			}
			if (extr(exp.ctx_switch[0], 12, 1) && extr(exp.ctx_switch[0], 20, 4) && zeta_wr) {
				insrt(epixel[3], (paddr[3] & 3) * 8, cpp * 8, zeta);
			}
			for (int j = 0; j < 4; j++) {
				if (extr(exp.ctx_switch[0], 20 + j, 1) && color_wr) {
					uint32_t src = extr(pixel[j], (paddr[j] & 3) * 8, cpp * 8);
					uint32_t res = nv03_pgraph_zpoint_rop(&exp, x, y, src);
					insrt(epixel[j], (paddr[j] & 3) * 8, cpp * 8, res);
				}
			}
		}
	}
	bool other_fail() override {
		bool res = false;
		for (int j = 0; j < 4; j++) {
			rpixel[j] = nva_grd32(nva_cards[cnum]->bar1, paddr[j] & ~3);
			if (rpixel[j] != epixel[j]) {
				printf("Difference in PIXEL[%d]: expected %08x real %08x\n", j, epixel[j], rpixel[j]);
				res = true;
			}
		}
		return res;
	}
	void print_fail() override {
		for (int j = 0; j < 4; j++) {
			printf("%08x %08x %08x PIXEL[%d]%s\n", pixel[j], epixel[j], rpixel[j], j, epixel[j] == rpixel[j] ? "" : " *");
		}
		MthdTest::print_fail();
	}
public:
	RopZPointTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

}

namespace hwtest {
namespace pgraph {

bool PGraphRopTests::supported() {
	return chipset.card_type < 4;
}

Test::Subtests PGraphRopTests::subtests() {
	return {
		{"solid", new RopSolidTest(opt, rnd())},
		{"blit", new RopBlitTest(opt, rnd())},
		{"zpoint", new RopZPointTest(opt, rnd())},
	};
}

}
}
