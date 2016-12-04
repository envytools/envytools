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

class MthdClipTest : public MthdTest {
	int idx;
	int which;
	int repeats() override { return 10000; };
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			insrt(orig.vtx_x[15], 16, 15, (rnd() & 1) ? 0 : 0x7fff);
			insrt(orig.vtx_y[15], 16, 15, (rnd() & 1) ? 0 : 0x7fff);
		}
		if (chipset.card_type < 3) {
			/* XXX: submitting on BLIT causes an actual blit */
			if (idx && extr(orig.access, 12, 5) == 0x10)
				insrt(orig.access, 12, 5, 0);
		}
	}
	void choose_mthd() override {
		idx = rnd() & 1;
		if (chipset.card_type < 3) {
			cls = 0x05;
			mthd = 0x300 + idx * 4;;
			which = 0;
		} else {
			switch (rnd() % 7) {
				case 0:
					cls = 0x05;
					mthd = 0x300 + idx * 4;
					which = 0;
					break;
				case 1:
					cls = 0x0c;
					mthd = 0x7f4 + idx * 4;
					which = 3;
					break;
				case 2:
					cls = 0x0c;
					mthd = 0xbec + idx * 4;
					which = 3;
					break;
				case 3:
					cls = 0x0c;
					mthd = 0xfe8 + idx * 4;
					which = 3;
					break;
				case 4:
					cls = 0x0c;
					mthd = 0x13e4 + idx * 4;
					which = 3;
					break;
				case 5:
					cls = 0x0e;
					mthd = 0x0308 + idx * 4;
					which = 1;
					break;
				case 6:
					cls = 0x15;
					mthd = 0x0310 + idx * 4;
					which = 2;
					break;
				default:
					abort();
			}
		}
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
			nv01_pgraph_set_clip(&exp, idx, val);
		} else {
			nv03_pgraph_set_clip(&exp, which, idx, val, extr(orig.xy_misc_1[0], 0, 1));
		}
	}
public:
	MthdClipTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdVtxTest : public MthdTest {
	bool first, poly, draw, fract, ifc, noclip;
	int repeats() override { return 10000; };
	void adjust_orig_mthd() override {
		if (chipset.card_type < 3) {
			if (rnd() & 1)
				insrt(orig.access, 12, 5, 8 + rnd() % 4);
			if (rnd() & 1)
				orig.valid[0] |= 0x1ff1ff;
			if (rnd() & 1)
				orig.valid[0] |= 0x033033;
			if (rnd() & 1)
				orig.valid[0] &= ~0x11000000;
			if (rnd() & 1)
				orig.valid[0] &= ~0x00f00f;
		} else {
			if (rnd() & 1) {
				orig.valid[0] &= ~0xf0f;
				orig.valid[0] ^= 1 << (rnd() & 0x1f);
				orig.valid[0] ^= 1 << (rnd() & 0x1f);
			}
			if (draw) {
				if (rnd() & 3)
					insrt(orig.cliprect_ctrl, 8, 1, 0);
				if (rnd() & 3)
					insrt(orig.debug[2], 28, 1, 0);
				if (rnd() & 3) {
					insrt(orig.xy_misc_4[0], 4, 4, 0);
					insrt(orig.xy_misc_4[1], 4, 4, 0);
				}
				if (rnd() & 3) {
					orig.valid[0] = 0x0fffffff;
					if (rnd() & 1) {
						orig.valid[0] &= ~0xf0f;
					}
					orig.valid[0] ^= 1 << (rnd() & 0x1f);
					orig.valid[0] ^= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					insrt(orig.ctx_switch[0], 24, 5, 0x17);
					insrt(orig.ctx_switch[0], 13, 2, 0);
					for (int j = 0; j < 8; j++) {
						insrt(orig.ctx_cache[j][0], 24, 5, 0x17);
						insrt(orig.ctx_cache[j][0], 13, 2, 0);
					}
				}
			}
		}
		if (rnd() & 1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (rnd() & 1)
			orig.xy_misc_1[0] &= ~0x330;
	}
	void choose_mthd() override {
		first = poly = draw = fract = noclip = ifc = false;;
		if (chipset.card_type < 3) {
			switch (rnd() % 27) {
				case 0:
					cls = 0x10;
					mthd = 0x300;
					first = true;
					break;
				case 1:
					cls = 0x11;
					mthd = 0x304;
					first = true;
					break;
				case 2:
					cls = 0x12;
					mthd = 0x310;
					first = true;
					break;
				case 3:
					cls = 0x13;
					mthd = 0x308;
					first = true;
					break;
				case 4:
					cls = 0x14;
					mthd = 0x308;
					first = true;
					break;
				case 5:
					cls = 0x10;
					mthd = 0x304;
					first = false;
					break;
				case 6:
					cls = 0x0c;
					mthd = 0x400 | (rnd() & 0x78);
					first = true;
					break;
				case 7:
					cls = 0x0b;
					mthd = 0x310;
					first = true;
					break;
				case 8:
					cls = 0x0b;
					mthd = 0x504 | (rnd() & 0x70);
					first = true;
					break;
				case 9:
					cls = 0x09;
					mthd = 0x400 | (rnd() & 0x78);
					first = true;
					break;
				case 10:
					cls = 0x0a;
					mthd = 0x400 | (rnd() & 0x78);
					first = true;
					break;
				case 11: {
					int beta = rnd() & 1;
					int idx = rnd() & 3;
					fract = rnd() & 1;
					cls = 0x0d | beta << 4;
					mthd = (0x310 + idx * 4) | fract << 6;
					first = idx == 0;
					break;
				}
				case 12: {
					int beta = rnd() & 1;
					int idx = rnd() % 9;
					fract = rnd() & 1;
					cls = 0x0e | beta << 4;
					mthd = (0x310 + idx * 4) | fract << 6;
					first = idx == 0;
					break;
				}
				case 13:
					cls = 0x08;
					mthd = 0x400 | (rnd() & 0x7c);
					first = true;
					draw = true;
					break;
				case 14:
					cls = 0x08;
					mthd = 0x504 | (rnd() & 0x78);
					first = true;
					draw = true;
					break;
				case 15:
					cls = 0x09;
					mthd = 0x404 | (rnd() & 0x78);
					first = false;
					draw = true;
					break;
				case 16:
					cls = 0x0a;
					mthd = 0x404 | (rnd() & 0x78);
					first = false;
					draw = true;
					break;
				case 17:
					cls = 0x0b;
					mthd = 0x314;
					first = false;
					break;
				case 18:
					cls = 0x0b;
					mthd = 0x318;
					first = false;
					draw = true;
					break;
				case 19:
					cls = 0x0b;
					mthd = 0x508 | (rnd() & 0x70);
					first = false;
					break;
				case 20:
					cls = 0x0b;
					mthd = 0x50c | (rnd() & 0x70);
					first = false;
					draw = true;
					break;
				case 21:
					cls = 0x09;
					mthd = 0x500 | (rnd() & 0x7c);
					first = false;
					draw = true;
					poly = true;
					break;
				case 22:
					cls = 0x0a;
					mthd = 0x500 | (rnd() & 0x7c);
					first = false;
					draw = true;
					poly = true;
					break;
				case 23:
					cls = 0x09;
					mthd = 0x604 | (rnd() & 0x78);
					first = false;
					draw = true;
					poly = true;
					break;
				case 24:
					cls = 0x0a;
					mthd = 0x604 | (rnd() & 0x78);
					first = false;
					draw = true;
					poly = true;
					break;
				case 25:
					cls = 0x0b;
					mthd = 0x400 | (rnd() & 0x7c);
					first = false;
					draw = true;
					poly = true;
					break;
				case 26:
					cls = 0x0b;
					mthd = 0x584 | (rnd() & 0x78);
					first = false;
					draw = true;
					poly = true;
					break;
				default:
					abort();
			}
		} else {
			switch (rnd() % 27) {
				case 0:
					cls = 0x07;
					mthd = 0x400 | (rnd() & 0x78);
					first = true;
					break;
				case 1:
					cls = 0x08;
					mthd = 0x400 | (rnd() & 0x7c);
					first = true;
					draw = true;
					break;
				case 2:
					cls = 0x08;
					mthd = 0x504 | (rnd() & 0x78);
					first = true;
					draw = true;
					break;
				case 3:
					cls = 0x09 + (rnd() & 1);
					mthd = 0x400 | (rnd() & 0x78);
					first = true;
					break;
				case 4:
					cls = 0x09 + (rnd() & 1);
					mthd = 0x404 | (rnd() & 0x78);
					draw = true;
					break;
				case 5:
					cls = 0x09 + (rnd() & 1);
					mthd = 0x500 | (rnd() & 0x7c);
					poly = true;
					draw = true;
					break;
				case 6:
					cls = 0x09 + (rnd() & 1);
					mthd = 0x604 | (rnd() & 0x78);
					poly = true;
					draw = true;
					break;
				case 7:
					cls = 0x0b;
					mthd = 0x310;
					first = true;
					break;
				case 8:
					cls = 0x0b;
					mthd = 0x314;
					break;
				case 9:
					cls = 0x0b;
					mthd = 0x318;
					draw = true;
					break;
				case 10:
					cls = 0x0b;
					mthd = 0x400 | (rnd() & 0x7c);
					poly = true;
					draw = true;
					break;
				case 11:
					cls = 0x0b;
					mthd = 0x504 | (rnd() & 0x70);
					first = true;
					break;
				case 12:
					cls = 0x0b;
					mthd = 0x508 | (rnd() & 0x70);
					break;
				case 13:
					cls = 0x0b;
					mthd = 0x50c | (rnd() & 0x70);
					draw = true;
					break;
				case 14:
					cls = 0x0b;
					mthd = 0x584 | (rnd() & 0x78);
					poly = true;
					draw = true;
					break;
				case 15:
					cls = 0x0c;
					mthd = 0x400 | (rnd() & 0x1f8);
					first = true;
					noclip = true;
					break;
				case 16:
					cls = 0x0c;
					mthd = 0x800 | (rnd() & 0x1f8);
					first = true;
					break;
				case 17:
					cls = 0x0c;
					mthd = 0x804 | (rnd() & 0x1f8);
					draw = true;
					break;
				case 18:
					cls = 0xc;
					mthd = 0xbfc;
					first = true;
					ifc = true;
					break;
				case 19:
					cls = 0xc;
					mthd = 0xffc;
					first = true;
					ifc = true;
					break;
				case 20:
					cls = 0xc;
					mthd = 0x13fc;
					first = true;
					ifc = true;
					break;
				case 21:
					cls = 0x10;
					mthd = 0x300;
					first = true;
					break;
				case 22:
					cls = 0x10;
					mthd = 0x304;
					break;
				case 23:
					cls = 0x11;
					mthd = 0x304;
					first = true;
					ifc = true;
					break;
				case 24:
					cls = 0x12;
					mthd = 0x310;
					first = true;
					ifc = true;
					break;
				case 25:
					cls = 0x14;
					mthd = 0x308;
					first = true;
					break;
				case 26:
					cls = 0x18;
					mthd = 0x7fc;
					first = true;
					break;
				default:
					abort();
			}
		}
	}
	void emulate_mthd() override {
		int rcls;
		if (chipset.card_type < 3) {
			rcls = extr(exp.access, 12, 5);
		} else {
			rcls = cls;
		}
		if (chipset.card_type < 3) {
			if (first)
				insrt(exp.xy_misc_0, 28, 4, 0);
			int idx = extr(exp.xy_misc_0, 28, 4);
			if (rcls == 0x11 || rcls == 0x12 || rcls == 0x13)
				idx = 4;
			if (nv01_pgraph_is_tex_class(rcls)) {
				idx = (mthd - 0x10) >> 2 & 0xf;
				if (idx >= 12)
					idx -= 8;
				if (extr(exp.xy_misc_1[0], 24, 1) && extr(exp.xy_misc_1[0], 25, 1) != (uint32_t)fract) {
					exp.valid[0] &= ~0xffffff;
				}
			} else {
				fract = 0;
			}
			insrt(exp.xy_misc_1[0], 0, 1, 0);
			insrt(exp.xy_misc_1[0], 24, 1, 1);
			insrt(exp.xy_misc_1[0], 25, 1, fract);
			nv01_pgraph_bump_vtxid(&exp);
			nv01_pgraph_set_vtx(&exp, 0, idx, extrs(val, 0, 16), false);
			nv01_pgraph_set_vtx(&exp, 1, idx, extrs(val, 16, 16), false);
			if (!poly) {
				if (idx <= 8)
					exp.valid[0] |= 0x1001 << idx;
				if (rcls >= 0x09 && rcls <= 0x0b) {
					if (first) {
						exp.valid[0] &= ~0xffffff;
						exp.valid[0] |= 0x011111;
					} else {
						exp.valid[0] |= 0x10010 << (idx & 3);
					}
				}
				if ((rcls == 0x10 || rcls == 0x0c) && first)
					exp.valid[0] |= 0x100;
			} else {
				if (rcls >= 9 && rcls <= 0xb) {
					exp.valid[0] |= 0x10010 << (idx & 3);
				}
			}
			if (draw)
				nv01_pgraph_prep_draw(&exp, poly);
			// XXX
			if (draw && (rcls == 0x11 || rcls == 0x12 || rcls == 0x13))
				skip = true;
		} else {
			int vidx;
			if (first) {
				vidx = 0;
				if (cls >= 9 && cls <= 0xb) {
					exp.valid[0] &= ~0xffff;
					insrt(exp.valid[0], 21, 1, 1);
				}
				if (cls == 0x18)
					insrt(exp.valid[0], 21, 1, 1);
			} else {
				vidx = extr(exp.xy_misc_0, 28, 4);
			}
			int rvidx = ifc ? 4 : vidx;
			int svidx = vidx & 3;
			int nvidx = (vidx + 1) & 0xf;
			if (cls == 0x8 || cls == 0x9 || cls == 0xa || cls == 0xc)
				nvidx &= 1;
			if (vidx == 2 && cls == 0xb)
				nvidx = 0;
			if (vidx == 3 && cls == 0x10)
				nvidx = 0;
			if (cls == 0xc && !first)
				vidx = rvidx = svidx = 1;
			insrt(exp.xy_misc_0, 28, 4, nvidx);
			insrt(exp.xy_misc_1[0], 0, 1, 0);
			insrt(exp.xy_misc_1[1], 0, 1, 1);
			if (cls != 0xc || first)
				insrt(exp.xy_misc_3, 8, 1, 0);
			if (poly && (exp.valid[0] & 0xf0f))
				insrt(exp.valid[0], 21, 1, 0);
			if (!poly) {
				insrt(exp.valid[0], rvidx, 1, 1);
				insrt(exp.valid[0], 8|rvidx, 1, 1);
			}
			if ((cls >= 9 && cls <= 0xb)) {
				insrt(exp.valid[0], 4|svidx, 1, 1);
				insrt(exp.valid[0], 0xc|svidx, 1, 1);
			}
			if (cls == 0x10) {
				insrt(exp.valid[0], vidx, 1, 1);
				insrt(exp.valid[0], vidx|8, 1, 1);
			}
			if (cls != 0xc || first)
				insrt(exp.valid[0], 19, 1, noclip);
			if (cls >= 8 && cls <= 0x14) {
				insrt(exp.xy_misc_4[0], 0+svidx, 1, 0);
				insrt(exp.xy_misc_4[0], 4+svidx, 1, 0);
				insrt(exp.xy_misc_4[1], 0+svidx, 1, 0);
				insrt(exp.xy_misc_4[1], 4+svidx, 1, 0);
			}
			if (noclip) {
				exp.vtx_x[rvidx] = extrs(val, 16, 16);
				exp.vtx_y[rvidx] = extrs(val, 0, 16);
			} else {
				exp.vtx_x[rvidx] = extrs(val, 0, 16);
				exp.vtx_y[rvidx] = extrs(val, 16, 16);
			}
			int xcstat = nv03_pgraph_clip_status(&exp, exp.vtx_x[rvidx], 0, noclip);
			int ycstat = nv03_pgraph_clip_status(&exp, exp.vtx_y[rvidx], 1, noclip);
			insrt(exp.xy_clip[0][vidx >> 3], 4*(vidx & 7), 4, xcstat);
			insrt(exp.xy_clip[1][vidx >> 3], 4*(vidx & 7), 4, ycstat);
			if (cls == 0x08 || cls == 0x18) {
				insrt(exp.xy_clip[0][vidx >> 3], 4*((vidx|1) & 7), 4, xcstat);
				insrt(exp.xy_clip[1][vidx >> 3], 4*((vidx|1) & 7), 4, ycstat);
			}
			if (draw) {
				nv03_pgraph_prep_draw(&exp, poly, false);
			}
		}
	}
	bool other_fail() override {
		int rcls;
		if (chipset.card_type < 3) {
			rcls = extr(exp.access, 12, 5);
		} else {
			rcls = cls;
		}
		if (real.status && (rcls == 9 || rcls == 0xa || rcls == 0x0b || rcls == 0x0c || rcls == 0x10)) {
			/* Hung PGRAPH... */
			skip = true;
		}
		return false;
	}
public:
	MthdVtxTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdX32Test : public MthdTest {
	bool first, poly;
	int repeats() override { return 10000; };
	void adjust_orig_mthd() override {
		if (chipset.card_type < 3) {
			if (rnd() & 1) {
				insrt(orig.access, 12, 5, 8 + rnd() % 4);
			}
			if (rnd() & 1)
				orig.valid[0] |= 0x1ff1ff;
			if (rnd() & 1)
				orig.valid[0] |= 0x033033;
			if (rnd() & 1) {
				orig.valid[0] &= ~0x11000000;
			}
			if (rnd() & 1) {
				orig.valid[0] &= ~0x00f00f;
			}
		} else {
			if (rnd() & 1) {
				orig.valid[0] &= ~(rnd() & 0x00000f0f);
				orig.valid[0] &= ~(rnd() & 0x00000f0f);
			}
		}
		if (rnd() & 1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (rnd() & 1) {
			orig.xy_misc_1[0] &= ~0x330;
		}
	}
	void choose_mthd() override {
		first = poly = false;
		switch (rnd() % 7) {
			case 0:
				cls = 0x08;
				mthd = 0x480 | (rnd() & 0x78);
				first = true;
				break;
			case 1:
				cls = 0x09;
				mthd = 0x480 | (rnd() & 0x78);
				first = !(mthd & 8);
				break;
			case 2:
				cls = 0x0a;
				mthd = 0x480 | (rnd() & 0x78);
				first = !(mthd & 8);
				break;
			case 3:
				cls = 0x0b;
				mthd = 0x320 + 8 * (rnd() % 3);
				first = (mthd == 0x320);
				break;
			case 4:
				cls = 0x09;
				mthd = 0x580 | (rnd() & 0x78);
				first = false;
				poly = true;
				break;
			case 5:
				cls = 0x0a;
				mthd = 0x580 | (rnd() & 0x78);
				first = false;
				poly = true;
				break;
			case 6:
				cls = 0x0b;
				mthd = 0x480 | (rnd() & 0x78);
				first = false;
				poly = true;
				break;
			default:
				abort();
		}
		if (rnd() & 1) {
			val = extrs(val, 0, 17);
		}
	}
	void emulate_mthd() override {
		if (chipset.card_type < 3) {
			int rcls = extr(exp.access, 12, 5);
			if (first)
				insrt(exp.xy_misc_0, 28, 4, 0);
			int idx = extr(exp.xy_misc_0, 28, 4);
			if (nv01_pgraph_is_tex_class(rcls)) {
				if (extr(exp.xy_misc_1[0], 24, 1) && extr(exp.xy_misc_1[0], 25, 1)) {
					exp.valid[0] &= ~0xffffff;
				}
			}
			insrt(exp.xy_misc_0, 28, 4, idx);
			insrt(exp.xy_misc_1[0], 0, 1, 0);
			insrt(exp.xy_misc_1[0], 24, 1, 1);
			insrt(exp.xy_misc_1[0], 25, 1, 0);
			nv01_pgraph_set_vtx(&exp, 0, idx, val, true);
			if (!poly) {
				if (idx <= 8)
					exp.valid[0] |= 1 << idx;
				if (rcls >= 0x09 && rcls <= 0x0b) {
					if (first) {
						exp.valid[0] &= ~0xffffff;
						exp.valid[0] |= 0x000111;
					} else {
						exp.valid[0] |= 0x10 << (idx & 3);
					}
				}
				if ((rcls == 0x10 || rcls == 0x0c) && first)
					exp.valid[0] |= 0x100;
			} else {
				if (rcls >= 9 && rcls <= 0xb) {
					if (exp.valid[0] & 0xf00f)
						exp.valid[0] &= ~0x100;
					exp.valid[0] |= 0x10 << (idx & 3);
				}
			}
		} else {
			int vidx;
			if (first) {
				vidx = 0;
				if (cls >= 9 && cls <= 0xb) {
					exp.valid[0] &= ~0xffff;
					insrt(exp.valid[0], 21, 1, 1);
				}
				if (cls == 0x18)
					insrt(exp.valid[0], 21, 1, 1);
			} else {
				vidx = extr(exp.xy_misc_0, 28, 4);
			}
			int svidx = vidx & 3;
			insrt(exp.xy_misc_0, 28, 4, vidx);
			insrt(exp.xy_misc_1[0], 0, 1, 0);
			insrt(exp.xy_misc_1[1], 0, 1, 1);
			insrt(exp.xy_misc_3, 8, 1, 0);
			if (poly && (exp.valid[0] & 0xf0f))
				insrt(exp.valid[0], 21, 1, 0);
			if (!poly)
				insrt(exp.valid[0], vidx, 1, 1);
			if ((cls >= 9 && cls <= 0xb))
				insrt(exp.valid[0], 4|svidx, 1, 1);
			insrt(exp.valid[0], 19, 1, false);
			if (cls >= 8 && cls <= 0x14) {
				insrt(exp.xy_misc_4[0], 0+svidx, 1, 0);
				insrt(exp.xy_misc_4[0], 4+svidx, 1, (int32_t)val != sext(val, 15));
			}
			exp.vtx_x[vidx] = val;
			int xcstat = nv03_pgraph_clip_status(&exp, exp.vtx_x[vidx], 0, false);
			insrt(exp.xy_clip[0][vidx >> 3], 4*(vidx & 7), 4, xcstat);
			if (cls == 0x08 || cls == 0x18) {
				insrt(exp.xy_clip[0][vidx >> 3], 4*((vidx|1) & 7), 4, xcstat);
			}
		}
	}
public:
	MthdX32Test(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdY32Test : public MthdTest {
	bool poly, draw;
	int repeats() override { return 10000; };
	void adjust_orig_mthd() override {
		if (chipset.card_type < 3) {
			if (rnd() & 1) {
				insrt(orig.access, 12, 5, 8 + rnd() % 4);
			}
			if (rnd() & 1)
				orig.valid[0] |= 0x1ff1ff;
			if (rnd() & 1)
				orig.valid[0] |= 0x033033;
			if (rnd() & 1)
				orig.valid[0] &= ~0x11000000;
			if (rnd() & 1)
				orig.valid[0] &= ~0x00f00f;
		} else {
			if (draw) {
				if (rnd() & 3)
					insrt(orig.cliprect_ctrl, 8, 1, 0);
				if (rnd() & 3)
					insrt(orig.debug[2], 28, 1, 0);
				if (rnd() & 3) {
					insrt(orig.xy_misc_4[0], 4, 4, 0);
					insrt(orig.xy_misc_4[1], 4, 4, 0);
				}
				if (rnd() & 3) {
					orig.valid[0] = 0x0fffffff;
					if (rnd() & 1) {
						orig.valid[0] &= ~0xf0f;
					}
					orig.valid[0] ^= 1 << (rnd() & 0x1f);
					orig.valid[0] ^= 1 << (rnd() & 0x1f);
				}
				if (rnd() & 1) {
					insrt(orig.ctx_switch[0], 24, 5, 0x17);
					insrt(orig.ctx_switch[0], 13, 2, 0);
					for (int j = 0; j < 8; j++) {
						insrt(orig.ctx_cache[j][0], 24, 5, 0x17);
						insrt(orig.ctx_cache[j][0], 13, 2, 0);
					}
				}
			}
		}
		if (rnd() & 1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (rnd() & 1) {
			orig.xy_misc_1[0] &= ~0x330;
		}
	}
	void choose_mthd() override {
		poly = draw = false;
		switch (rnd() % 7) {
			case 0:
				cls = 0x08;
				mthd = 0x484 | (rnd() & 0x78);
				draw = true;
				break;
			case 1:
				cls = 0x09;
				mthd = 0x484 | (rnd() & 0x78);
				draw = !!(mthd & 0x8);
				break;
			case 2:
				cls = 0x0a;
				mthd = 0x484 | (rnd() & 0x78);
				draw = !!(mthd & 0x8);
				break;
			case 3:
				cls = 0x0b;
				mthd = 0x324 + 8 * (rnd() % 3);
				draw = mthd == 0x334;
				break;
			case 4:
				cls = 0x09;
				mthd = 0x584 | (rnd() & 0x78);
				poly = true;
				draw = true;
				break;
			case 5:
				cls = 0x0a;
				mthd = 0x584 | (rnd() & 0x78);
				poly = true;
				draw = true;
				break;
			case 6:
				cls = 0x0b;
				mthd = 0x484 | (rnd() & 0x78);
				poly = true;
				draw = true;
				break;
			default:
				abort();
		}
		if (rnd() & 1)
			val = extrs(val, 0, 17);
	}
	void emulate_mthd() override {
		if (chipset.card_type < 3) {
			int rcls = extr(exp.access, 12, 5);
			int idx = extr(exp.xy_misc_0, 28, 4);
			if (nv01_pgraph_is_tex_class(rcls)) {
				if (extr(exp.xy_misc_1[0], 24, 1) && extr(exp.xy_misc_1[0], 25, 1)) {
					exp.valid[0] &= ~0xffffff;
				}
			}
			nv01_pgraph_bump_vtxid(&exp);
			insrt(exp.xy_misc_1[0], 0, 1, 0);
			insrt(exp.xy_misc_1[0], 24, 1, 1);
			insrt(exp.xy_misc_1[0], 25, 1, 0);
			nv01_pgraph_set_vtx(&exp, 1, idx, val, true);
			if (!poly) {
				if (idx <= 8)
					exp.valid[0] |= 0x1000 << idx;
				if (rcls >= 0x09 && rcls <= 0x0b) {
					exp.valid[0] |= 0x10000 << (idx & 3);
				}
			} else {
				if (rcls >= 9 && rcls <= 0xb) {
					exp.valid[0] |= 0x10000 << (idx & 3);
				}
			}
			if (draw)
				nv01_pgraph_prep_draw(&exp, poly);
			// XXX
			if (draw && (rcls == 0x11 || rcls == 0x12 || rcls == 0x13))
				skip = true;
		} else {
			int vidx;
			vidx = extr(exp.xy_misc_0, 28, 4);
			int svidx = vidx & 3;
			int nvidx = (vidx + 1) & 0xf;
			if (cls == 0x8 || cls == 0x9 || cls == 0xa)
				nvidx &= 1;
			if (vidx == 2 && cls == 0xb)
				nvidx = 0;
			insrt(exp.xy_misc_0, 28, 4, nvidx);
			insrt(exp.xy_misc_1[0], 0, 1, 0);
			insrt(exp.xy_misc_1[1], 0, 1, 1);
			insrt(exp.xy_misc_3, 8, 1, 0);
			if (poly && (exp.valid[0] & 0xf0f))
				insrt(exp.valid[0], 21, 1, 0);
			if (!poly)
				insrt(exp.valid[0], vidx|8, 1, 1);
			if ((cls >= 9 && cls <= 0xb))
				insrt(exp.valid[0], 0xc|svidx, 1, 1);
			insrt(exp.valid[0], 19, 1, false);
			if (cls >= 8 && cls <= 0x14) {
				insrt(exp.xy_misc_4[1], 0+svidx, 1, 0);
				insrt(exp.xy_misc_4[1], 4+svidx, 1, (int32_t)val != sext(val, 15));
			}
			exp.vtx_y[vidx] = val;
			int ycstat = nv03_pgraph_clip_status(&exp, exp.vtx_y[vidx], 1, false);
			if (cls == 0x08 || cls == 0x18) {
				insrt(exp.xy_clip[1][0], 0, 4, ycstat);
				insrt(exp.xy_clip[1][0], 4, 4, ycstat);
			} else {
				insrt(exp.xy_clip[1][vidx >> 3], 4*(vidx & 7), 4, ycstat);
			}
			if (draw) {
				nv03_pgraph_prep_draw(&exp, poly, false);
			}
		}
	}
	bool other_fail() override {
		int rcls;
		if (chipset.card_type < 3) {
			rcls = extr(exp.access, 12, 5);
		} else {
			rcls = cls;
		}
		if (real.status && (rcls == 9 || rcls == 0xa || rcls == 0x0b || rcls == 0x0c || rcls == 0x10)) {
			/* Hung PGRAPH... */
			skip = true;
		}
		return false;
	}
public:
	MthdY32Test(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdIfcSizeTest : public MthdTest {
	bool is_ifm, is_in, is_out;
	int repeats() override { return 10000; };
	void adjust_orig_mthd() override {
		if (chipset.card_type < 3) {
			if (!(rnd() & 3))
				insrt(orig.access, 12, 5, rnd() & 1 ? 0x12 : 0x14);
		}
	}
	void choose_mthd() override {
		if (!(rnd() & 3))
			val &= 0xff00ff;
		if (!(rnd() & 3))
			val &= 0xffff000f;
		is_in = is_out = is_ifm = false;
		if (chipset.card_type < 3) {
			switch (rnd() % 5) {
				case 0:
					cls = 0x11;
					mthd = 0x30c;
					is_in = true;
					break;
				case 1:
					cls = 0x12;
					mthd = 0x318;
					is_in = true;
					break;
				case 2:
					cls = 0x13;
					mthd = 0x30c;
					is_ifm = true;
					is_in = true;
					break;
				case 3:
					cls = 0x11;
					mthd = 0x308;
					is_out = true;
					break;
				case 4:
					cls = 0x12;
					mthd = 0x314;
					is_out = true;
					break;
				default:
					abort();
			}
		} else {
			switch (rnd() % 10) {
				case 0:
					cls = 0xc;
					mthd = 0xbf8;
					is_in = is_out = true;
					break;
				case 1:
					cls = 0xc;
					mthd = 0xff4;
					is_in = true;
					break;
				case 2:
					cls = 0xc;
					mthd = 0xff8;
					is_out = true;
					break;
				case 3:
					cls = 0xc;
					mthd = 0x13f4;
					is_in = true;
					break;
				case 4:
					cls = 0xc;
					mthd = 0x13f8;
					is_out = true;
					break;
				case 5:
					cls = 0x11;
					mthd = 0x308;
					is_out = true;
					break;
				case 6:
					cls = 0x11;
					mthd = 0x30c;
					is_in = true;
					break;
				case 7:
					cls = 0x12;
					mthd = 0x314;
					is_out = true;
					break;
				case 8:
					cls = 0x12;
					mthd = 0x318;
					is_in = true;
					break;
				case 9:
					cls = 0x15;
					mthd = 0x304;
					is_in = true;
					break;
				default:
					abort();
			}
		}
	}
	void emulate_mthd() override {
		if (chipset.card_type < 3) {
			if (is_out) {
				int rcls = extr(exp.access, 12, 5);
				exp.vtx_x[5] = extr(val, 0, 16);
				exp.vtx_y[5] = extr(val, 16, 16);
				if (rcls <= 0xb && rcls >= 9)
					exp.valid[0] &= ~0xffffff;
				exp.valid[0] |= 0x020020;
				insrt(exp.xy_misc_0, 28, 4, 0);
				if (rcls >= 0x11 && rcls <= 0x13)
					insrt(exp.xy_misc_1[0], 0, 1, 0);
				if (rcls == 0x10 || (rcls >= 9 && rcls <= 0xc))
					exp.valid[0] |= 0x100;
			}
			if (is_in) {
				int rcls = extr(exp.access, 12, 5);
				exp.vtx_y[1] = 0;
				exp.vtx_x[3] = extr(val, 0, 16);
				exp.vtx_y[3] = -extr(val, 16, 16);
				if (rcls >= 0x11 && rcls <= 0x13)
					insrt(exp.xy_misc_1[0], 0, 1, 0);
				if (!is_ifm) {
					if (rcls <= 0xb && rcls >= 9)
						exp.valid[0] &= ~0xffffff;
					if (rcls == 0x10 || (rcls >= 9 && rcls <= 0xc))
						exp.valid[0] |= 0x100;
				}
				exp.valid[0] |= 0x008008;
				if (rcls <= 0xb && rcls >= 9)
					exp.valid[0] |= 0x080080;
				exp.edgefill &= ~0x110;
				if (exp.vtx_x[3] < 0x20 && rcls == 0x12)
					exp.edgefill |= 0x100;
				if (rcls != 0x0d && rcls != 0x1d) {
					insrt(exp.xy_misc_4[0], 28, 2, 0);
					insrt(exp.xy_misc_4[1], 28, 2, 0);
				}
				if (exp.vtx_x[3])
					exp.xy_misc_4[0] |= 2 << 28;
				if (exp.vtx_y[3])
					exp.xy_misc_4[1] |= 2 << 28;
				bool zero;
				if (rcls == 0x14) {
					uint32_t pixels = 4 / nv01_pgraph_cpp_in(exp.ctx_switch[0]);
					zero = (exp.vtx_x[3] == pixels || !exp.vtx_y[3]);
				} else {
					zero = extr(exp.xy_misc_4[0], 28, 2) == 0 ||
						 extr(exp.xy_misc_4[1], 28, 2) == 0;
				}
				insrt(exp.xy_misc_0, 12, 1, zero);
				insrt(exp.xy_misc_0, 28, 4, 0);
			}
		} else {
			if (is_out) {
				exp.valid[0] |= 0x2020;
				exp.vtx_x[5] = extr(val, 0, 16);
				exp.vtx_y[5] = extr(val, 16, 16);
			}
			if (is_in) {
				exp.valid[0] |= 0x8008;
				exp.vtx_x[3] = extr(val, 0, 16);
				exp.vtx_y[3] = -extr(val, 16, 16);
				exp.vtx_y[7] = extr(val, 16, 16);
				if (!is_out)
					exp.misc24[0] = extr(val, 0, 16);
				nv03_pgraph_vtx_cmp(&exp, 0, 3, false);
				nv03_pgraph_vtx_cmp(&exp, 1, cls == 0x15 ? 3 : 7, false);
				bool zero = false;
				if (!extr(exp.xy_misc_4[0], 28, 4))
					zero = true;
				if (!extr(exp.xy_misc_4[1], 28, 4))
					zero = true;
				insrt(exp.xy_misc_0, 20, 1, zero);
				if (cls != 0x15) {
					insrt(exp.xy_misc_3, 12, 1, extr(val, 0, 16) < 0x20 && cls != 0x11);
				}
			}
			insrt(exp.xy_misc_1[0], 0, 1, 0);
			insrt(exp.xy_misc_1[1], 0, 1, cls != 0x15);
			insrt(exp.xy_misc_0, 28, 4, 0);
			insrt(exp.xy_misc_3, 8, 1, 0);
		}
	}
public:
	MthdIfcSizeTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdRectTest : public MthdTest {
	bool draw, noclip;
	int repeats() override { return 10000; };
	void adjust_orig_mthd() override {
		if (chipset.card_type < 3) {
			if (rnd() & 1)
				orig.valid[0] |= 0x1ff1ff;
			if (rnd() & 1)
				orig.valid[0] |= 0x033033;
			if (rnd() & 1) {
				orig.xy_misc_4[0] &= ~0xf0;
				orig.xy_misc_4[1] &= ~0xf0;
			}
			if (rnd() & 1) {
				orig.valid[0] &= ~0x11000000;
				orig.xy_misc_1[0] &= ~0x330;
			}
			if (rnd() & 1) {
				orig.valid[0] &= ~0x00f00f;
			}
		} else {
			if (rnd() & 3)
				insrt(orig.cliprect_ctrl, 8, 1, 0);
			if (rnd() & 3)
				insrt(orig.debug[2], 28, 1, 0);
			if (rnd() & 3) {
				insrt(orig.xy_misc_4[0], 4, 4, 0);
				insrt(orig.xy_misc_4[1], 4, 4, 0);
			}
			if (rnd() & 3) {
				orig.valid[0] = 0x0fffffff;
				if (rnd() & 1) {
					orig.valid[0] &= ~0xf0f;
				}
				orig.valid[0] ^= 1 << (rnd() & 0x1f);
				orig.valid[0] ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				insrt(orig.ctx_switch[0], 24, 5, 0x17);
				insrt(orig.ctx_switch[0], 13, 2, 0);
				int j;
				for (j = 0; j < 8; j++) {
					insrt(orig.ctx_cache[j][0], 24, 5, 0x17);
					insrt(orig.ctx_cache[j][0], 13, 2, 0);
				}
			}
			if (rnd() & 1) {
				insrt(orig.vtx_x[0], 16, 15, (rnd() & 1) ? 0 : 0x7fff);
				insrt(orig.vtx_y[0], 16, 15, (rnd() & 1) ? 0 : 0x7fff);
			}
			if (rnd() & 1) {
				insrt(orig.vtx_x[1], 16, 15, (rnd() & 1) ? 0 : 0x7fff);
				insrt(orig.vtx_y[1], 16, 15, (rnd() & 1) ? 0 : 0x7fff);
			}
			if (rnd() & 1) {
				if (rnd() & 1) {
					orig.vtx_x[0] &= 0x8000ffff;
				} else {
					orig.vtx_x[0] |= 0x7fff0000;
				}
			}
			if (rnd() & 1) {
				if (rnd() & 1) {
					orig.vtx_y[0] &= 0x8000ffff;
				} else {
					orig.vtx_y[0] |= 0x7fff0000;
				}
			}
		}
	}
	void choose_mthd() override {
		noclip = false;
		if (chipset.card_type < 3) {
			switch (rnd() % 3) {
				case 0:
					cls = 0x10;
					mthd = 0x308;
					draw = true;
					break;
				case 1:
					cls = 0x14;
					mthd = 0x30c;
					draw = false;
					break;
				case 2:
					cls = 0x0c;
					mthd = 0x404 | (rnd() & 0x78);
					draw = true;
					break;
				default:
					abort();
			}
		} else {
			switch (rnd() % 3) {
				case 0:
					cls = 0x10;
					mthd = 0x308;
					draw = true;
					break;
				case 1:
					cls = 0x14;
					mthd = 0x30c;
					draw = false;
					break;
				case 2:
					cls = 0x07;
					mthd = 0x404 | (rnd() & 0x78);
					draw = true;
					break;
				case 3:
					cls = 0xc;
					mthd = 0x404 | (rnd() & 0x1f8);
					noclip = true;
					draw = true;
					break;
				default:
					abort();
			}
		}
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
		int rcls;
		if (chipset.card_type < 3) {
			rcls = extr(exp.access, 12, 5);
		} else {
			rcls = cls;
		}
		if (rcls == 0x14) {
			if (chipset.card_type < 3) {
				exp.vtx_x[3] = extr(val, 0, 16);
				exp.vtx_y[3] = extr(val, 16, 16);
				nv01_pgraph_vtx_fixup(&exp, 0, 2, exp.vtx_x[3], 1, 0, 2);
				nv01_pgraph_vtx_fixup(&exp, 1, 2, exp.vtx_y[3], 1, 0, 2);
				exp.valid[0] |= 0x4004;
				insrt(exp.xy_misc_0, 12, 1, !exp.vtx_x[3] || !exp.vtx_y[3]);
				nv01_pgraph_bump_vtxid(&exp);
			} else {
				exp.vtx_x[3] = extr(val, 0, 16);
				exp.vtx_y[3] = extr(val, 16, 16);
				int vidx = extr(exp.xy_misc_0, 28, 4);
				int nvidx = vidx + 1;
				if (nvidx == 4)
					nvidx = 0;
				insrt(exp.xy_misc_0, 28, 4, nvidx);
				nv03_pgraph_vtx_cmp(&exp, 0, 3, false);
				nv03_pgraph_vtx_cmp(&exp, 1, 3, false);
				bool zero = false;
				if (extr(exp.xy_misc_4[0], 28, 4) < 2)
					zero = true;
				if (extr(exp.xy_misc_4[1], 28, 4) < 2)
					zero = true;
				insrt(exp.xy_misc_0, 20, 1, zero);
				nv03_pgraph_vtx_add(&exp, 0, 2, exp.vtx_x[0], exp.vtx_x[3], 0, false);
				nv03_pgraph_vtx_add(&exp, 1, 2, exp.vtx_y[0], exp.vtx_y[3], 0, false);
				exp.misc24[0] = extr(val, 0, 16);
				exp.valid[0] |= 0x404;
			}
		} else if (rcls == 0x10) {
			if (chipset.card_type < 3) {
				nv01_pgraph_vtx_fixup(&exp, 0, 2, extr(val, 0, 16), 1, 0, 2);
				nv01_pgraph_vtx_fixup(&exp, 1, 2, extr(val, 16, 16), 1, 0, 2);
				nv01_pgraph_vtx_fixup(&exp, 0, 3, extr(val, 0, 16), 1, 1, 3);
				nv01_pgraph_vtx_fixup(&exp, 1, 3, extr(val, 16, 16), 1, 1, 3);
				nv01_pgraph_bump_vtxid(&exp);
				nv01_pgraph_bump_vtxid(&exp);
				exp.valid[0] |= 0x00c00c;
			} else {
				int vtxid = extr(exp.xy_misc_0, 28, 4);
				vtxid++;
				if (vtxid == 4)
					vtxid = 0;
				vtxid++;
				if (vtxid == 4)
					vtxid = 0;
				insrt(exp.xy_misc_0, 28, 4, vtxid);
				insrt(exp.xy_misc_1[1], 0, 1, 1);
				nv03_pgraph_vtx_add(&exp, 0, 2, exp.vtx_x[0], extr(val, 0, 16), 0, false);
				nv03_pgraph_vtx_add(&exp, 1, 2, exp.vtx_y[0], extr(val, 16, 16), 0, false);
				nv03_pgraph_vtx_add(&exp, 0, 3, exp.vtx_x[1], extr(val, 0, 16), 0, false);
				nv03_pgraph_vtx_add(&exp, 1, 3, exp.vtx_y[1], extr(val, 16, 16), 0, false);
				nv03_pgraph_vtx_cmp(&exp, 0, 8, true);
				nv03_pgraph_vtx_cmp(&exp, 1, 8, true);
				exp.valid[0] |= 0xc0c;
			}
		} else if (rcls == 0x0c || rcls == 0x11 || rcls == 0x12 || rcls == 0x13 || (chipset.card_type >= 3 && rcls == 7)) {
			if (chipset.card_type < 3) {
				int idx = extr(exp.xy_misc_0, 28, 4);
				nv01_pgraph_vtx_fixup(&exp, 0, idx, extr(val, 0, 16), 1, 0, idx & 3);
				nv01_pgraph_vtx_fixup(&exp, 1, idx, extr(val, 16, 16), 1, 0, idx & 3);
				nv01_pgraph_bump_vtxid(&exp);
				if (idx <= 8)
					exp.valid[0] |= 0x1001 << idx;
			} else {
				int vidx = extr(exp.xy_misc_0, 28, 4);
				int nvidx = (vidx + 1) & 1;
				insrt(exp.xy_misc_0, 28, 4, nvidx);
				if (noclip) {
					nv03_pgraph_vtx_add(&exp, 0, 1, exp.vtx_x[0], extr(val, 16, 16), 0, true);
					nv03_pgraph_vtx_add(&exp, 1, 1, exp.vtx_y[0], extr(val, 0, 16), 0, true);
				} else {
					nv03_pgraph_vtx_add(&exp, 0, 1, exp.vtx_x[0], extr(val, 0, 16), 0, false);
					nv03_pgraph_vtx_add(&exp, 1, 1, exp.vtx_y[0], extr(val, 16, 16), 0, false);
				}
				if (noclip) {
					nv03_pgraph_vtx_cmp(&exp, 0, 2, false);
					nv03_pgraph_vtx_cmp(&exp, 1, 2, false);
				} else {
					nv03_pgraph_vtx_cmp(&exp, 0, 8, true);
					nv03_pgraph_vtx_cmp(&exp, 1, 8, true);
				}
				exp.valid[0] |= 0x202;
				insrt(exp.xy_misc_1[1], 0, 1, 1);
			}
		} else {
			nv01_pgraph_vtx_fixup(&exp, 0, 15, extr(val, 0, 16), 1, 15, 1);
			nv01_pgraph_vtx_fixup(&exp, 1, 15, extr(val, 16, 16), 1, 15, 1);
			nv01_pgraph_bump_vtxid(&exp);
			if (rcls >= 0x09 && rcls <= 0x0b) {
				exp.valid[0] |= 0x080080;
			}
		}
		if (draw) {
			if (chipset.card_type < 3) {
				nv01_pgraph_prep_draw(&exp, false);
			} else {
				nv03_pgraph_prep_draw(&exp, false, noclip);
			}
		}
		// XXX
		if (draw && (rcls == 0x11 || rcls == 0x12 || rcls == 0x13))
			skip = true;
	}
	bool other_fail() override {
		int rcls = extr(exp.access, 12, 5);
		if (draw && real.status && (rcls == 0x0b || rcls == 0x0c)) {
			/* Hung PGRAPH... */
			skip = true;
		}
		return false;
	}
public:
	MthdRectTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdIfcDataTest : public MthdTest {
	bool is_bitmap;
	int repeats() override { return 100000; };
	bool supported() override { return chipset.card_type < 3; } // XXX
	void adjust_orig_mthd() override {
		if (rnd() & 3)
			orig.valid[0] = 0x1ff1ff;
		if (rnd() & 3) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (rnd() & 3) {
			orig.valid[0] &= ~0x11000000;
			orig.xy_misc_1[0] &= ~0x330;
		}
		for (int j = 0; j < 6; j++) {
			if (rnd() & 1) {
				orig.vtx_x[j] &= 0xff;
				orig.vtx_x[j] -= 0x80;
			}
			if (rnd() & 1) {
				orig.vtx_y[j] &= 0xff;
				orig.vtx_y[j] -= 0x80;
			}
		}
		if (rnd() & 3)
			insrt(orig.access, 12, 5, 0x11 + (rnd() & 1));
	}
	void choose_mthd() override {
		switch (rnd() % 3) {
			case 0:
				cls = 0x11;
				mthd = 0x400 | (rnd() & 0x7c);
				is_bitmap = false;
				break;
			case 1:
				cls = 0x12;
				mthd = 0x400 | (rnd() & 0x7c);
				is_bitmap = true;
				break;
			case 2:
				cls = 0x13;
				mthd = 0x040 | (rnd() & 0x3c);
				is_bitmap = false;
				break;
			default:
				abort();
		}
	}
	void emulate_mthd() override {
		exp.misc32[0] = is_bitmap ? pgraph_expand_mono(&exp, val) : val;
		insrt(exp.xy_misc_1[0], 24, 1, 0);
		int rcls = exp.access >> 12 & 0x1f;
		int steps = 4 / nv01_pgraph_cpp_in(exp.ctx_switch[0]);
		if (rcls == 0x12)
			steps = 0x20;
		if (rcls != 0x11 && rcls != 0x12)
			goto done;
		if (exp.valid[0] & 0x11000000 && exp.ctx_switch[0] & 0x80)
			exp.intr |= 1 << 16;
		if (extr(exp.canvas_config, 24, 1))
			exp.intr |= 1 << 20;
		if (extr(exp.cliprect_ctrl, 8, 1))
			exp.intr |= 1 << 24;
		int iter;
		iter = 0;
		if (extr(exp.xy_misc_0, 12, 1)) {
			if ((exp.valid[0] & 0x38038) != 0x38038)
				exp.intr |= 1 << 16;
			if ((exp.xy_misc_4[0] & 0xf0) || (exp.xy_misc_4[1] & 0xf0))
				exp.intr |= 1 << 12;
			goto done;
		}
		int vidx;
		if (!(exp.xy_misc_1[0] & 1)) {
			exp.vtx_x[6] = exp.vtx_x[4] + exp.vtx_x[5];
			exp.vtx_y[6] = exp.vtx_y[4] + exp.vtx_y[5];
			insrt(exp.xy_misc_1[0], 14, 1, 0);
			insrt(exp.xy_misc_1[0], 18, 1, 0);
			insrt(exp.xy_misc_1[0], 20, 1, 0);
			if ((exp.valid[0] & 0x38038) != 0x38038) {
				exp.intr |= 1 << 16;
				if ((exp.xy_misc_4[0] & 0xf0) || (exp.xy_misc_4[1] & 0xf0))
					exp.intr |= 1 << 12;
				goto done;
			}
			nv01_pgraph_iclip_fixup(&exp, 0, exp.vtx_x[6], 0);
			nv01_pgraph_iclip_fixup(&exp, 1, exp.vtx_y[6], 0);
			insrt(exp.xy_misc_1[0], 0, 1, 1);
			if (extr(exp.edgefill, 8, 1)) {
				/* XXX */
				skip = true;
				return;
			}
			insrt(exp.xy_misc_0, 28, 4, 0);
			vidx = 1;
			exp.vtx_y[2] = exp.vtx_y[3] + 1;
			nv01_pgraph_vtx_cmp(&exp, 1, 2);
			nv01_pgraph_vtx_fixup(&exp, 1, 0, 0, 1, 4, 0);
			nv01_pgraph_vtx_fixup(&exp, 0, 0, 0, 1, 4, 0);
			exp.vtx_x[2] = exp.vtx_x[3];
			exp.vtx_x[2] -= steps;
			nv01_pgraph_vtx_cmp(&exp, 0, 2);
			nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[vidx ^ 1], steps, 0);
			if (extr(exp.xy_misc_4[0], 28, 1)) {
				nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[2], exp.vtx_x[vidx], 0);
			}
			if ((exp.xy_misc_4[0] & 0xc0) || (exp.xy_misc_4[1] & 0xf0))
				exp.intr |= 1 << 12;
			if ((exp.xy_misc_4[0] & 0x30) == 0x30)
				exp.intr |= 1 << 12;
		} else {
			if ((exp.valid[0] & 0x38038) != 0x38038)
				exp.intr |= 1 << 16;
			if ((exp.xy_misc_4[0] & 0xf0) || (exp.xy_misc_4[1] & 0xf0))
				exp.intr |= 1 << 12;
		}
restart:;
		vidx = extr(exp.xy_misc_0, 28, 1);
		if (extr(exp.edgefill, 8, 1)) {
			/* XXX */
			skip = true;
			return;
		}
		if (!exp.intr) {
			if (extr(exp.xy_misc_4[0], 29, 1)) {
				nv01_pgraph_bump_vtxid(&exp);
			} else {
				insrt(exp.xy_misc_0, 28, 4, 0);
				vidx = 1;
				bool check_y = false;
				if (extr(exp.xy_misc_4[1], 28, 1)) {
					exp.vtx_y[2]++;
					nv01_pgraph_vtx_add(&exp, 1, 0, 0, exp.vtx_y[0], exp.vtx_y[1], 1);
					check_y = true;
				} else {
					exp.vtx_x[4] += exp.vtx_x[3];
					exp.vtx_y[2] = exp.vtx_y[3] + 1;
					nv01_pgraph_vtx_fixup(&exp, 1, 0, 0, 1, 4, 0);
				}
				nv01_pgraph_vtx_cmp(&exp, 1, 2);
				nv01_pgraph_vtx_fixup(&exp, 0, 0, 0, 1, 4, 0);
				if (extr(exp.xy_misc_4[0], 28, 1)) {
					nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[vidx ^ 1], ~exp.vtx_x[2], 1);
					exp.vtx_x[2] += exp.vtx_x[3];
					nv01_pgraph_vtx_cmp(&exp, 0, 2);
					if (extr(exp.xy_misc_4[0], 28, 1)) {
						nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[2], exp.vtx_x[vidx], 0);
						if (exp.xy_misc_4[0] & 0x10)
							exp.intr |= 1 << 12;
						check_y = true;
					} else {
						if ((exp.xy_misc_4[0] & 0x20))
							exp.intr |= 1 << 12;
					}
					if (exp.xy_misc_4[1] & 0x10 && check_y)
						exp.intr |= 1 << 12;
					iter++;
					if (iter > 10000) {
						/* This is a hang - skip this test run.  */
						skip = true;
						return;
					}
					goto restart;
				}
				exp.vtx_x[2] = exp.vtx_x[3];
			}
			exp.vtx_x[2] -= steps;
			nv01_pgraph_vtx_cmp(&exp, 0, 2);
			nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[vidx ^ 1], steps, 0);
			if (extr(exp.xy_misc_4[0], 28, 1)) {
				nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[2], exp.vtx_x[vidx], 0);
			}
		} else {
			nv01_pgraph_bump_vtxid(&exp);
			if (extr(exp.xy_misc_4[0], 29, 1)) {
				exp.vtx_x[2] -= steps;
				nv01_pgraph_vtx_cmp(&exp, 0, 2);
			} else if (extr(exp.xy_misc_4[1], 28, 1)) {
				exp.vtx_y[2]++;
			}
		}
done:
		if (exp.intr)
			exp.access &= ~0x101;
	}
public:
	MthdIfcDataTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdZPointZetaTest : public MthdTest {
	int repeats() override { return 10000; };
	bool supported() override { return chipset.card_type == 3; }
	void adjust_orig_mthd() override {
		if (rnd() & 3)
			insrt(orig.cliprect_ctrl, 8, 1, 0);
		if (rnd() & 3)
			insrt(orig.debug[2], 28, 1, 0);
		if (rnd() & 3) {
			insrt(orig.xy_misc_4[0], 4, 4, 0);
			insrt(orig.xy_misc_4[1], 4, 4, 0);
		}
		if (rnd() & 3) {
			orig.valid[0] = 0x0fffffff;
			if (rnd() & 1) {
				orig.valid[0] &= ~0xf0f;
			}
			orig.valid[0] ^= 1 << (rnd() & 0x1f);
			orig.valid[0] ^= 1 << (rnd() & 0x1f);
		}
		if (rnd() & 1) {
			insrt(orig.ctx_switch[0], 24, 5, 0x17);
			insrt(orig.ctx_switch[0], 13, 2, 0);
			int j;
			for (j = 0; j < 8; j++) {
				insrt(orig.ctx_cache[j][0], 24, 5, 0x17);
				insrt(orig.ctx_cache[j][0], 13, 2, 0);
			}
		}
		if (!(rnd() & 7)) {
			insrt(orig.vtx_x[0], 2, 29, (rnd() & 1) ? 0 : -1);
			insrt(orig.vtx_y[0], 2, 29, (rnd() & 1) ? 0 : -1);
		}
		orig.debug[2] &= 0xffdfffff;
		orig.debug[3] &= 0xfffeffff;
		orig.debug[3] &= 0xfffdffff;
	}
	void choose_mthd() override {
		cls = 0x18;
		int idx = rnd() & 0xff;
		mthd = 0x804 + idx * 8;
	}
	void emulate_mthd() override {
		insrt(exp.misc32[1], 16, 16, extr(val, 16, 16));
		nv03_pgraph_prep_draw(&exp, false, false);
		nv03_pgraph_vtx_add(&exp, 0, 0, exp.vtx_x[0], 1, 0, false);
	}
public:
	MthdZPointZetaTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSifcDiffTest : public MthdTest {
	bool xy;
	int repeats() override { return 10000; };
	bool supported() override { return chipset.card_type >= 3; }
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			val &= 0xffff;
		if (!(rnd() & 3))
			val &= 0x000f000f;
		if (!(rnd() & 3))
			val = 0x00100000;
	}
	void choose_mthd() override {
		xy = rnd() & 1;
		cls = 0x15;
		mthd = 0x308 + xy * 4;
	}
	void emulate_mthd() override {
		insrt(exp.valid[0], 5 + xy * 8, 1, 1);
		if (xy)
			exp.vtx_y[5] = val;
		else
			exp.vtx_x[5] = val;
		insrt(exp.xy_misc_0, 28, 4, 0);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 0);
	}
public:
	MthdSifcDiffTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSifcVtxTest : public MthdTest {
	int repeats() override { return 10000; };
	bool supported() override { return chipset.card_type >= 3; }
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			val &= 0xffff;
		if (!(rnd() & 3))
			val &= 0x000f000f;
		if (!(rnd() & 3))
			val = 0x00100000;
	}
	void choose_mthd() override {
		cls = 0x15;
		mthd = 0x318;
	}
	void emulate_mthd() override {
		exp.valid[0] |= 0x9018;
		exp.vtx_x[4] = extr(val, 0, 16) << 16;
		exp.vtx_y[3] = -exp.vtx_y[7];
		exp.vtx_y[4] = extr(val, 16, 16) << 16;
		insrt(exp.valid[0], 19, 1, 0);
		insrt(exp.xy_misc_0, 28, 4, 0);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 0);
		int xcstat = nv03_pgraph_clip_status(&exp, extrs(val, 4, 12), 0, false);
		int ycstat = nv03_pgraph_clip_status(&exp, extrs(val, 20, 12), 1, false);
		insrt(exp.xy_clip[0][0], 0, 4, xcstat);
		insrt(exp.xy_clip[1][0], 0, 4, ycstat);
		insrt(exp.xy_misc_3, 8, 1, 0);
		nv03_pgraph_vtx_cmp(&exp, 0, 3, false);
		nv03_pgraph_vtx_cmp(&exp, 1, 3, false);
		insrt(exp.xy_misc_0, 20, 1, 0);
		insrt(exp.xy_misc_4[0], 0, 1, 0);
		insrt(exp.xy_misc_4[0], 4, 1, 0);
		insrt(exp.xy_misc_4[1], 0, 1, 0);
		insrt(exp.xy_misc_4[1], 4, 1, 0);
	}
public:
	MthdSifcVtxTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

}

bool PGraphMthdXyTests::supported() {
	return chipset.card_type < 4;
}

Test::Subtests PGraphMthdXyTests::subtests() {
	return {
		{"clip", new MthdClipTest(opt, rnd())},
		{"vtx", new MthdVtxTest(opt, rnd())},
		{"vtx_x32", new MthdX32Test(opt, rnd())},
		{"vtx_y32", new MthdY32Test(opt, rnd())},
		{"ifc_size", new MthdIfcSizeTest(opt, rnd())},
		{"rect", new MthdRectTest(opt, rnd())},
		{"ifc_data", new MthdIfcDataTest(opt, rnd())},
		{"zpoint_zeta", new MthdZPointZetaTest(opt, rnd())},
		{"sifc_diff", new MthdSifcDiffTest(opt, rnd())},
		{"sifc_vtx", new MthdSifcVtxTest(opt, rnd())},
	};
}

}
}
