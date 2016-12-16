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

int StateTest::run_once() {
	skip = false;
	pgraph_gen_state(cnum, rnd, &orig);
	adjust_orig();
	pgraph_load_state(cnum, &orig);
	exp = orig;
	mutate();
	pgraph_dump_state(cnum, &real);
	bool fail = other_fail();
	if (skip)
		return HWTEST_RES_NA;
	if (pgraph_cmp_state(&orig, &exp, &real, fail)) {
		print_fail();
		return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

namespace {

class SoftResetTest : public StateTest {
protected:
	bool supported() override { return chipset.card_type < 4; } // XXX
	void mutate() override {
		nva_wr32(cnum, 0x400080, exp.debug[0] | 1);
		pgraph_reset(&exp);
	}
public:
	SoftResetTest(TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

const uint32_t nv01_pgraph_state_regs[] = {
	/* INTR, INVALID */
	0x400100, 0x400104,
	/* INTR_EN, INVALID_EN */
	0x400140, 0x400144,
	/* CTX_* */
	0x400180, 0x400190,
	/* ICLIP */
	0x400450, 0x400454,
	/* UCLIP */
	0x400460, 0x400468, 0x400464, 0x40046c,
	/* VTX_X */
	0x400400, 0x400404, 0x400408, 0x40040c,
	0x400410, 0x400414, 0x400418, 0x40041c,
	0x400420, 0x400424, 0x400428, 0x40042c,
	0x400430, 0x400434, 0x400438, 0x40043c,
	0x400440, 0x400444,
	/* VTX_Y */
	0x400480, 0x400484, 0x400488, 0x40048c,
	0x400490, 0x400494, 0x400498, 0x40049c,
	0x4004a0, 0x4004a4, 0x4004a8, 0x4004ac,
	0x4004b0, 0x4004b4, 0x4004b8, 0x4004bc,
	0x4004c0, 0x4004c4,
	/* VTX_BETA */
	0x400700, 0x400704, 0x400708, 0x40070c,
	0x400710, 0x400714, 0x400718, 0x40071c,
	0x400720, 0x400724, 0x400728, 0x40072c,
	0x400730, 0x400734,
	/* PATTERN_RGB, _A */
	0x400600, 0x400608, 0x400604, 0x40060c,
	/* PATTERN_MONO, _SHAPE */
	0x400610, 0x400614, 0x400618,
	/* BITMAP_COLOR */
	0x40061c, 0x400620,
	/* ROP, PLANE, CHROMA, BETA */
	0x400624, 0x400628, 0x40062c, 0x400630,
	/* CANVAS_CONFIG */
	0x400634,
	/* CANVAS */
	0x400688, 0x40068c,
	/* CLIPRECT */
	0x400690, 0x400698, 0x400694, 0x40069c,
	/* CLIPRECT_CTRL */
	0x4006a0,
	/* VALID, SOURCE_COLOR, SUBDIVIDE, EDGEFILL */
	0x400650, 0x400654, 0x400658, 0x40065c,
	/* XY_MISC */
	0x400640, 0x400644, 0x400648, 0x40064c,
	/* DMA, NOTIFY */
	0x400680, 0x400684,
	/* ACCESS */
	0x4006a4,
	/* DEBUG */
	0x400080, 0x400084, 0x400088,
	/* STATUS */
	0x4006b0,
	/* PFB_CONFIG */
	0x600200,
	0x600000,
};

class MMIOReadTest : public StateTest {
private:
	uint32_t reg;
protected:
	void mutate() override {
		if (chipset.card_type < 3) {
			int idx = rnd() % ARRAY_SIZE(nv01_pgraph_state_regs);
			reg = nv01_pgraph_state_regs[idx];
			if ((reg & ~0xf) == 0x400460) {
				exp.xy_misc_1[0] &= ~0xfff000;
			}
		} else {
			reg = 0x400000 | (rnd() & 0x1ffc);
			// Reading PIPE is dangerous...
			if (reg == 0x400f54 && chipset.card_type == 0x10)
				return;
			// No idea.
			if ((reg & 0xffffff00) == 0x400800 && chipset.chipset == 0x10)
				return;
		}
		nva_rd32(cnum, reg);
	}
	void print_fail() {
		printf("After reading %08x\n", reg);
	}
public:
	MMIOReadTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteControlNv1Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type == 1; }
	void mutate() override {
		val = rnd();
		switch (rnd() % 10) {
			default:
				reg = 0x400140;
				exp.intr_en = val & 0x11111111;
				break;
			case 1:
				reg = 0x400144;
				exp.invalid_en = val & 0x11111;
				break;
			case 2:
				reg = 0x400180;
				exp.ctx_switch[0] = val & 0x807fffff;
				insrt(exp.debug[1], 0, 1, 0);
				insrt(exp.ctx_control, 24, 1, 0);
				break;
			case 3:
				reg = 0x400190;
				exp.ctx_control = val & 0x11010103;
				break;
			case 4:
				reg = 0x400680;
				exp.ctx_switch[1] = val & 0xffff;
				break;
			case 5:
				reg = 0x400684;
				exp.notify = val & 0x11ffff;
				break;
			case 6:
				reg = 0x4006a4;
				if (extr(val, 24, 1))
					insrt(exp.access, 0, 1, extr(val, 0, 1));
				if (extr(val, 25, 1))
					insrt(exp.access, 4, 1, extr(val, 4, 1));
				if (extr(val, 26, 1))
					insrt(exp.access, 8, 1, extr(val, 8, 1));
				if (extr(val, 27, 1))
					insrt(exp.access, 12, 5, extr(val, 12, 5));
				break;
			case 7:
				reg = 0x400080;
				exp.debug[0] = val & 0x11111110;
				if (extr(val, 0, 1))
					pgraph_reset(&exp);
				break;
			case 8:
				reg = 0x400084;
				exp.debug[1] = val & 0x31111101;
				break;
			case 9:
				reg = 0x400088;
				exp.debug[2] = val & 0x11111111;
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteControlNv1Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteControlNv3Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type == 3; }
	void mutate() override {
		val = rnd();
		int idx;
		switch (rnd() % 16) {
			default:
				reg = 0x400140;
				exp.intr_en = val & 0x11111111;
				break;
			case 1:
				reg = 0x400144;
				exp.invalid_en = val & 0x11111;
				break;
			case 3:
				reg = 0x400180;
				exp.ctx_switch[0] = val & 0x3ff3f71f;
				insrt(exp.debug[1], 0, 1, extr(val, 31, 1) && extr(exp.debug[2], 28, 1));
				if (extr(exp.debug[1], 0, 1))
					pgraph_volatile_reset(&exp);
				break;
			case 4:
				reg = 0x400190;
				exp.ctx_control = val & 0x11010103;
				break;
			case 5:
				reg = 0x400194;
				exp.ctx_user = val & 0x7f1fe000;
				break;
			case 6:
				idx = rnd() & 7;
				reg = 0x4001a0 + idx * 4;
				if (!exp.fifo_enable)
					exp.ctx_cache[idx][0] = val & 0x3ff3f71f;
				break;
			case 7:
				reg = 0x400680;
				exp.ctx_switch[1] = val & 0xffff;
				break;
			case 8:
				reg = 0x400684;
				exp.notify = val & 0xf1ffff;
				break;
			case 9:
				reg = 0x400688;
				exp.ctx_switch[3] = val & 0xffff;
				break;
			case 10:
				reg = 0x40068c;
				exp.ctx_switch[2] = val & 0x1ffff;
				break;
			case 11:
				reg = 0x4006a4;
				exp.fifo_enable = val & 1;
				break;
			case 12:
				reg = 0x400080;
				exp.debug[0] = val & 0x13311110;
				if (extr(val, 0, 1))
					pgraph_reset(&exp);
				break;
			case 13:
				reg = 0x400084;
				exp.debug[1] = val & 0x10113301;
				if (extr(val, 4, 1))
					insrt(exp.xy_misc_1[0], 0, 1, 0);
				break;
			case 14:
				reg = 0x400088;
				exp.debug[2] = val & 0x1133f111;
				break;
			case 15:
				reg = 0x40008c;
				exp.debug[3] = val & 0x1173ff31;
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteControlNv3Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteControlNv4Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type >= 4; }
	void mutate() override {
		val = rnd();
		int idx;
		bool is_nv5 = chipset.chipset >= 5;
		bool is_nv11p = nv04_pgraph_is_nv11p(&chipset);
		bool is_nv15p = nv04_pgraph_is_nv15p(&chipset);
		bool is_nv17p = nv04_pgraph_is_nv17p(&chipset);
		uint32_t ctxs_mask, ctxc_mask;
		uint32_t offset_mask = pgraph_offset_mask(&chipset);
		if (chipset.card_type < 0x10) {
			ctxs_mask = ctxc_mask = is_nv5 ? 0x7f73f0ff : 0x0303f0ff;
		} else {
			ctxs_mask = is_nv11p ? 0x7ffff0ff : 0x7fb3f0ff;
			ctxc_mask = is_nv11p ? 0x7ffff0ff : is_nv15p ? 0x7fb3f0ff : 0x7f33f0ff;
		}
		switch (rnd() % 29) {
			default:
				reg = 0x400140;
				if (chipset.card_type < 0x10) {
					exp.intr_en = val & 0x11311;
				} else {
					exp.intr_en = val & 0x1113711;
				}
				break;
			case 1:
				reg = 0x400104;
				if (chipset.card_type < 0x10) {
					exp.nstatus = val & 0x7800;
				} else {
					exp.nstatus = val & 0x7800000;
				}
				break;
			case 2:
				reg = 0x400080;
				if (chipset.card_type >= 0x10) {
					// XXX
					return;
				}
				exp.debug[0] = val & 0x1337f000;
				if (val & 3) {
					exp.xy_a &= 1 << 20;
					exp.xy_misc_1[0] = 0;
					exp.xy_misc_1[1] = 0;
					exp.xy_misc_3 &= ~0x1100;
					exp.xy_misc_4[0] &= ~0xff;
					exp.xy_misc_4[1] &= ~0xff;
					exp.xy_clip[0][0] = 0x55555555;
					exp.xy_clip[0][1] = 0x55555555;
					exp.xy_clip[1][0] = 0x55555555;
					exp.xy_clip[1][1] = 0x55555555;
					exp.valid[0] = 0;
					exp.valid[1] = 0;
					exp.misc24[0] = 0;
					exp.misc24[1] = 0;
					exp.misc24[2] = 0;
				}
				if (val & 0x11) {
					for (int i = 0; i < 6; i++) {
						exp.surf_offset[i] = 0;
						exp.surf_base[i] = 0;
						exp.surf_limit[i] = offset_mask | 0xf;
					}
					for (int i = 0; i < 5; i++)
						exp.surf_pitch[i] = 0;
					for (int i = 0; i < 2; i++)
						exp.surf_swizzle[i] = 0;
					exp.unk610 = 0;
					exp.unk614 = 0;
				}
				if (val & 0x101) {
					exp.dma_eng_flags[0] &= ~0x1000;
					exp.dma_eng_flags[1] &= ~0x1000;
				}
				break;
			case 3:
				reg = 0x400084;
				if (chipset.card_type < 0x10) {
					exp.debug[1] = val & (is_nv5 ? 0xf2ffb701 : 0x72113101);
					if (val & 0x10)
						exp.xy_misc_1[0] &= ~1;
				} else {
					uint32_t mangled = val & 0x3fffffff;
					if (val & 1 << 30)
						mangled |= 1 << 31;
					if (val & 1 << 31)
						mangled |= 1 << 30;
					exp.debug[1] = mangled & (is_nv11p ? 0xfe71f701 : 0xfe11f701);
					if (val & 0x10)
						exp.xy_misc_1[0] &= ~1;
				}
				break;
			case 4:
				reg = 0x400088;
				if (chipset.card_type <0x10)
					exp.debug[2] = val & 0x11d7fff1;
				else
					exp.debug[2] = val;
				break;
			case 5:
				reg = 0x40008c;
				if (chipset.card_type < 0x10) {
					exp.debug[3] = val & (is_nv5 ? 0xfbffff73 : 0x11ffff33);
				} else {
					exp.debug[3] = val & (is_nv15p ? 0xffffff78 : 0xfffffc70);
					if (is_nv17p)
						exp.debug[3] |= 0x400;
				}
				break;
			case 6:
				reg = 0x400090;
				if (chipset.card_type < 0x10)
					return;
				exp.debug[4] = val & (is_nv17p ? 0x1fffffff : 0x00ffffff);
				insrt(exp.unka10, 29, 1, extr(exp.debug[4], 2, 1) && !!extr(exp.surf_type, 2, 2));
				break;
			case 7:
				{
				bool vre = chipset.card_type >= 0x10 ? extr(orig.debug[3], 19, 1) : extr(orig.debug[2], 28, 1);
				reg = chipset.card_type >= 0x10 ? 0x40014c : 0x400160;
				exp.ctx_switch[0] = val & ctxs_mask;
				insrt(exp.debug[1], 0, 1, extr(val, 31, 1) && vre);
				if (extr(exp.debug[1], 0, 1)) {
					pgraph_volatile_reset(&exp);
				}
				break;
				}
			case 8:
				reg = chipset.card_type >= 0x10 ? 0x400150 : 0x400164;
				exp.ctx_switch[1] = val & 0xffff3f03;
				break;
			case 9:
				reg = chipset.card_type >= 0x10 ? 0x400154 : 0x400168;
				exp.ctx_switch[2] = val;
				break;
			case 10:
				reg = chipset.card_type >= 0x10 ? 0x400158 : 0x40016c;
				exp.ctx_switch[3] = val & 0xffff;
				break;
			case 11:
				if (chipset.card_type < 0x10)
					return;
				reg = 0x40015c;
				exp.ctx_switch[4] = val;
				break;
			case 12:
				idx = rnd() & 7;
				reg = (chipset.card_type >= 0x10 ? 0x400160 : 0x400180) + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][0] = val & ctxc_mask;
				break;
			case 13:
				idx = rnd() & 7;
				reg = (chipset.card_type >= 0x10 ? 0x400180 : 0x4001a0) + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][1] = val & 0xffff3f03;
				break;
			case 14:
				idx = rnd() & 7;
				reg = (chipset.card_type >= 0x10 ? 0x4001a0 : 0x4001c0) + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][2] = val;
				break;
			case 15:
				idx = rnd() & 7;
				reg = (chipset.card_type >= 0x10 ? 0x4001c0 : 0x4001e0) + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][3] = val & 0x0000ffff;
				break;
			case 16:
				if (chipset.card_type < 0x10)
					return;
				idx = rnd() & 7;
				reg = 0x4001e0 + idx * 4;
				if (!orig.fifo_enable)
					exp.ctx_cache[idx][4] = val;
				break;
			case 17:
				reg = chipset.card_type >= 0x10 ? 0x400144 : 0x400170;
				exp.ctx_control = val & 0x11010103;
				break;
			case 18:
				reg = chipset.card_type >= 0x10 ? 0x400148 : 0x400174;
				if (chipset.card_type < 0x10) {
					exp.ctx_user = val & 0x0f00e000;
				} else {
					exp.ctx_user = val & (is_nv15p ? 0x9f00e000 : 0x1f00e000);
				}
				break;
			case 19:
				reg = 0x400610;
				if (chipset.card_type < 0x10) {
					exp.unk610 = val & (0xe0000000 | offset_mask);
				} else {
					exp.unk610 = val & 0xfffffff0;
				}
				break;
			case 20:
				reg = 0x400614;
				exp.unk614 = val & (0xc0000000 | offset_mask);
				break;
			case 21:
				if (chipset.card_type < 0x10)
					return;
				reg = 0x40077c;
				if (!is_nv15p) {
					exp.unk77c = val & 0x0100ffff;
					if (val & 1 << 28)
						exp.unk77c |= 7 << 28;
				} else {
					exp.unk77c = val & 0x631fffff;
				}
				break;
			case 22:
				reg = chipset.card_type >= 0x10 ? 0x400718 : 0x400714;
				exp.notify = val & (chipset.card_type >= 0x10 ? 0x73110101 : 0x110101);
				break;
			case 23:
				if (!is_nv17p)
					return;
				reg = 0x4006b0;
				exp.unk6b0 = val;
				break;
			case 24:
				if (!is_nv17p)
					return;
				reg = 0x400838;
				exp.unk838 = val;
				break;
			case 25:
				if (!is_nv17p)
					return;
				reg = 0x40083c;
				exp.unk83c = val;
				break;
			case 26:
				if (!is_nv17p)
					return;
				reg = 0x400a00;
				exp.unka00 = val & 0x1fff1fff;
				break;
			case 27:
				if (!is_nv17p)
					return;
				reg = 0x400a04;
				exp.unka04 = val & 0x1fff1fff;
				break;
			case 28:
				if (!is_nv17p)
					return;
				reg = 0x400a10;
				exp.unka10 = val & 0xdfff3fff;
				insrt(exp.unka10, 29, 1, extr(exp.debug[4], 2, 1) && !!extr(exp.surf_type, 2, 2));
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteControlNv4Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteCanvasNv1Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type < 4; }
	void mutate() override {
		val = rnd();
		int idx;
		uint32_t canvas_mask = chipset.is_nv03t ? 0x7fff07ff : 0x3fff07ff;
		uint32_t offset_mask = chipset.is_nv03t ? 0x007fffff : 0x003fffff;
		if (chipset.card_type == 1)
			canvas_mask = 0x0fff0fff;
		switch (rnd() % 13) {
			default:
				if (chipset.card_type != 1)
					return;
				reg = 0x400634;
				exp.canvas_config = val & 0x01111011;
				break;
			case 1:
				if (chipset.card_type != 1)
					return;
				reg = 0x400688;
				exp.dst_canvas_min = val & 0xffffffff;
				break;
			case 2:
				if (chipset.card_type != 1)
					return;
				reg = 0x40068c;
				exp.dst_canvas_max = val & 0x0fff0fff;
				break;
			case 3:
				if (chipset.card_type != 3)
					return;
				reg = 0x400550;
				exp.src_canvas_min = val & canvas_mask;
				break;
			case 4:
				if (chipset.card_type != 3)
					return;
				reg = 0x400554;
				exp.src_canvas_max = val & canvas_mask;
				break;
			case 5:
				if (chipset.card_type != 3)
					return;
				reg = 0x400558;
				exp.dst_canvas_min = val & canvas_mask;
				break;
			case 6:
				if (chipset.card_type != 3)
					return;
				reg = 0x40055c;
				exp.dst_canvas_max = val & canvas_mask;
				break;
			case 7:
				idx = rnd() & 1;
				reg = 0x400690 + idx * 8;
				exp.cliprect_min[idx] = val & canvas_mask;
				break;
			case 8:
				idx = rnd() & 1;
				reg = 0x400694 + idx * 8;
				exp.cliprect_max[idx] = val & canvas_mask;
				break;
			case 9:
				reg = 0x4006a0;
				exp.cliprect_ctrl = val & 0x113;
				break;
			case 10:
				if (chipset.card_type != 3)
					return;
				idx = rnd() & 3;
				reg = 0x400630 + idx * 4;
				exp.surf_offset[idx] = val & offset_mask & ~0xf;
				break;
			case 11:
				if (chipset.card_type != 3)
					return;
				idx = rnd() & 3;
				reg = 0x400650 + idx * 4;
				exp.surf_pitch[idx] = val & 0x1ff0;
				break;
			case 12:
				if (chipset.card_type != 3)
					return;
				reg = 0x4006a8;
				exp.surf_format = val & 0x7777;
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteCanvasNv1Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteCanvasNv4Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type >= 4; }
	void mutate() override {
		val = rnd();
		int idx;
		uint32_t offset_mask = pgraph_offset_mask(&chipset);
		uint32_t pitch_mask = pgraph_pitch_mask(&chipset);
		bool is_nv11p = nv04_pgraph_is_nv11p(&chipset);
		bool is_nv15p = nv04_pgraph_is_nv15p(&chipset);
		bool is_nv17p = nv04_pgraph_is_nv17p(&chipset);
		switch (rnd() % 8) {
			default:
				idx = rnd() % 6;
				reg = 0x400640 + idx * 4;
				exp.surf_offset[idx] = val & offset_mask;
				exp.valid[0] |= 8;
				break;
			case 1:
				idx = rnd() % 6;
				reg = 0x400658 + idx * 4;
				exp.surf_base[idx] = val & offset_mask;
				break;
			case 2:
				idx = rnd() % 6;
				reg = 0x400684 + idx * 4;
				exp.surf_limit[idx] = (val & (1 << 31 | offset_mask)) | 0xf;
				break;
			case 3:
				idx = rnd() % 5;
				reg = 0x400670 + idx * 4;
				exp.surf_pitch[idx] = val & pitch_mask;
				exp.valid[0] |= 4;
				break;
			case 4:
				idx = rnd() % 2;
				reg = 0x40069c + idx * 4;
				exp.surf_swizzle[idx] = val & 0x0f0f0000;
				break;
			case 5:
				reg = chipset.card_type >= 0x10 ? 0x400710 : 0x40070c;
				if (!is_nv15p) {
					exp.surf_type = val & 3;
				} else if (!is_nv11p) {
					exp.surf_type = val & 0x77777703;
				} else if (!is_nv17p) {
					exp.surf_type = val & 0x77777713;
				} else {
					exp.surf_type = val & 0xf77777ff;
				}
				insrt(exp.unka10, 29, 1, extr(exp.debug[4], 2, 1) && !!extr(exp.surf_type, 2, 2));
				break;
			case 6:
				reg = 0x400724;
				exp.surf_format = val & 0xffffff;
				break;
			case 7:
				reg = chipset.card_type >= 0x10 ? 0x400714 : 0x400710;
				exp.ctx_valid = val & (is_nv17p ? 0x3f731f3f : 0x0f731f3f);
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteCanvasNv4Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteVStateNv1Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type == 1; }
	void mutate() override {
		val = rnd();
		int idx;
		switch (rnd() % 7) {
			default:
				reg = 0x400640;
				exp.xy_a = val & 0xf1ff11ff;
				break;
			case 1:
				reg = 0x400644;
				exp.xy_misc_1[0] = val & 0x03177331;
				break;
			case 2:
				idx = rnd() & 1;
				reg = 0x400648 + idx * 4;
				exp.xy_misc_4[idx] = val & 0x30ffffff;
				break;
			case 3:
				reg = 0x400650;
				exp.valid[0] = val & 0x111ff1ff;
				break;
			case 4:
				reg = 0x400654;
				exp.misc32[0] = val;
				break;
			case 5:
				reg = 0x400658;
				exp.subdivide = val & 0xffff00ff;
				break;
			case 6:
				reg = 0x40065c;
				exp.edgefill = val & 0xffff0113;
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteVStateNv1Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteVStateNv3Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type >= 3; }
	void mutate() override {
		val = rnd();
		int idx;
		switch (rnd() % 14) {
			default:
				reg = 0x400514;
				if (chipset.card_type < 0x10)
					exp.xy_a = val & 0xf013ffff;
				else
					exp.xy_a = val & 0xf113ffff;
				break;
			case 1:
				idx = rnd() & 1;
				reg = 0x400518 + idx * 4;
				if (chipset.card_type < 4)
					exp.xy_misc_1[idx] = val & 0x0f177331;
				else
					exp.xy_misc_1[idx] = val & 0x00111031;
				break;
			case 2:
				reg = 0x400520;
				exp.xy_misc_3 = val & 0x7f7f1111;
				break;
			case 3:
				idx = rnd() & 1;
				reg = 0x400500 + idx * 4;
				exp.xy_misc_4[idx] = val & 0x300000ff;
				break;
			case 4:
				idx = rnd() & 3;
				reg = 0x400524 + idx * 4;
				exp.xy_clip[idx >> 1][idx & 1] = val;
				break;
			case 5:
				reg = 0x400508;
				if (chipset.card_type < 4)
					exp.valid[0] = val;
				else
					exp.valid[0] = val & 0xf07fffff;
				break;
			case 6:
				if (chipset.card_type < 4)
					return;
				reg = 0x400578;
				if (chipset.card_type < 0x10)
					exp.valid[1] = val & 0x1fffffff;
				else
					exp.valid[1] = val & 0xdfffffff;
				break;
			case 7:
				idx = rnd() % 3;
				if (chipset.card_type < 4)
					idx = rnd() & 1;
				reg = ((uint32_t[3]){0x400510, 0x400570, 0x400574})[idx];
				exp.misc24[idx] = val & 0xffffff;
				break;
			case 8:
				idx = rnd() & 3;
				if (chipset.card_type < 4)
					idx = rnd() & 1;
				reg = ((uint32_t[4]){0x40050c, 0x40057c, 0x400580, 0x400584})[idx];
				if (chipset.card_type < 4 && idx == 1)
					reg = 0x40054c;
				exp.misc32[idx] = val;
				if (idx == 0)
					insrt(exp.valid[0], 16, 1, 1);
				break;
			case 9:
				if (chipset.card_type < 4)
					return;
				reg = chipset.card_type >= 0x10 ? 0x400770 : 0x400760;
				exp.dma_pitch = val;
				break;
			case 10:
				if (chipset.card_type < 4)
					return;
				reg = chipset.card_type >= 0x10 ? 0x400774 : 0x400764;
				exp.dvd_format = val & 0x33f;
				break;
			case 11:
				if (chipset.card_type < 4)
					return;
				reg = chipset.card_type >= 0x10 ? 0x400778 : 0x400768;
				exp.sifm_mode = val & 0x01030000;
				break;
			case 12:
				if (chipset.card_type < 0x10)
					return;
				reg = 0x400588;
				exp.unk588 = val & 0xffff;
				break;
			case 13:
				if (chipset.card_type < 0x10)
					return;
				reg = 0x40058c;
				exp.unk58c = val & 0x1ffff;
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteVStateNv3Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteRopNv1Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type < 4; }
	void mutate() override {
		val = rnd();
		int idx;
		switch (rnd() % 9) {
			default:
				idx = rnd() & 1;
				reg = 0x400600 + idx * 8;
				exp.pattern_mono_rgb[idx] = val & 0x3fffffff;
				break;
			case 1:
				idx = rnd() & 1;
				reg = 0x400604 + idx * 8;
				exp.pattern_mono_a[idx] = val & 0xff;
				break;
			case 2:
				idx = rnd() & 1;
				reg = 0x400610 + idx * 4;
				exp.pattern_mono_bitmap[idx] = val;
				break;
			case 3:
				reg = 0x400618;
				exp.pattern_config = val & 3;
				break;
			case 4:
				idx = rnd() & 1;
				if (chipset.card_type == 3)
					idx = 0;
				reg = 0x40061c + idx * 4;
				exp.bitmap_color[idx] = val & 0x7fffffff;
				break;
			case 5:
				reg = 0x400624;
				exp.rop = val & 0xff;
				break;
			case 6:
				reg = 0x400628;
				if (chipset.card_type == 3)
					return;
				exp.plane = val & 0x7fffffff;
				break;
			case 7:
				reg = 0x40062c;
				exp.chroma = val & 0x7fffffff;
				break;
			case 8:
				reg = chipset.card_type == 3 ? 0x400640 : 0x400630;
				exp.beta = val & 0x7f800000;
				if (val & 0x80000000)
					exp.beta = 0;
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteRopNv1Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteRopNv4Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type >= 4; }
	void mutate() override {
		val = rnd();
		int idx;
		switch (rnd() % 10) {
			default:
				reg = 0x400604;
				exp.rop = val & 0xff;
				break;
			case 1:
				reg = 0x400608;
				exp.beta = val & 0x7f800000;
				break;
			case 2:
				reg = 0x40060c;
				exp.beta4 = val;
				break;
			case 3:
				reg = 0x400814;
				exp.chroma = val;
				break;
			case 4:
				idx = rnd() & 1;
				reg = 0x400800 + idx * 4;
				exp.pattern_mono_color[idx] = val;
				break;
			case 5:
				idx = rnd() & 1;
				reg = 0x400808 + idx * 4;
				exp.pattern_mono_bitmap[idx] = val;
				break;
			case 6:
				reg = 0x400810;
				exp.pattern_config = val & 0x13;
				break;
			case 7:
				idx = rnd() & 0x3f;
				reg = 0x400900 + idx * 4;
				exp.pattern_color[idx] = val & 0xffffff;
				break;
			case 8:
				reg = 0x400600;
				exp.bitmap_color[0] = val;
				exp.valid[0] |= 1 << 17;
				break;
			case 9:
				reg = 0x400830;
				exp.ctx_format = val & 0x3f3f3f3f;
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteRopNv4Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteD3D0Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type == 3; }
	void mutate() override {
		val = rnd();
		int idx;
		switch (rnd() % 9) {
			default:
				reg = 0x4005c0;
				exp.d3d_tlv_xy = val;
				break;
			case 1:
				reg = 0x4005c4;
				exp.d3d_tlv_uv[0][0] = val;
				break;
			case 2:
				reg = 0x4005c8;
				exp.d3d_tlv_z = val & 0xffff;
				break;
			case 3:
				reg = 0x4005cc;
				exp.d3d_tlv_color = val;
				break;
			case 4:
				reg = 0x4005d0;
				exp.d3d_tlv_fog_tri_col1 = val;
				break;
			case 5:
				reg = 0x4005d4;
				exp.d3d_tlv_rhw = val;
				break;
			case 6:
				reg = 0x400644;
				exp.d3d_config = val & 0xf77fbdf3;
				insrt(exp.valid[0], 26, 1, 1);
				break;
			case 7:
				reg = 0x4006c8;
				exp.d3d_alpha = val & 0xfff;
				break;
			case 8:
				idx = rnd() & 0xf;
				reg = 0x400580 + idx * 4;
				exp.vtx_z[idx] = val & 0xffffff;
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteD3D0Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteD3D56Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type == 4; }
	void mutate() override {
		val = rnd();
		int idx;
		switch (rnd() % 20) {
			default:
				idx = rnd() & 1;
				reg = 0x400590 + idx * 8;
				exp.d3d_rc_alpha[idx] = val & 0xfd1d1d1d;
				if (idx == 0)
					exp.valid[1] |= 1 << 28;
				else
					exp.valid[1] |= 1 << 26;
				break;
			case 1:
				idx = rnd() & 1;
				reg = 0x400594 + idx * 8;
				exp.d3d_rc_color[idx] = val & 0xff1f1f1f;
				if (idx == 0)
					exp.valid[1] |= 1 << 27;
				else
					exp.valid[1] |= 1 << 25;
				break;
			case 2:
				idx = rnd() & 1;
				reg = 0x4005a8 + idx * 4;
				exp.d3d_tex_format[idx] = val & 0xfffff7a6;
				break;
			case 3:
				idx = rnd() & 1;
				reg = 0x4005b0 + idx * 4;
				exp.d3d_tex_filter[idx] = val & 0xffff9e1e;
				break;
			case 4:
				reg = 0x4005c0;
				exp.d3d_tlv_xy = val;
				break;
			case 5:
				reg = 0x4005c4;
				exp.d3d_tlv_uv[0][0] = val & 0xffffffc0;
				break;
			case 6:
				reg = 0x4005c8;
				exp.d3d_tlv_uv[0][1] = val & 0xffffffc0;
				break;
			case 7:
				reg = 0x4005cc;
				exp.d3d_tlv_uv[1][0] = val & 0xffffffc0;
				break;
			case 8:
				reg = 0x4005d0;
				exp.d3d_tlv_uv[1][1] = val & 0xffffffc0;
				break;
			case 9:
				reg = 0x4005d4;
				exp.d3d_tlv_z = val;
				break;
			case 10:
				reg = 0x4005d8;
				exp.d3d_tlv_color = val;
				break;
			case 11:
				reg = 0x4005dc;
				exp.d3d_tlv_fog_tri_col1 = val;
				break;
			case 12:
				reg = 0x4005e0;
				exp.d3d_tlv_rhw = val & 0xffffffc0;
				break;
			case 13:
				reg = 0x400818;
				exp.d3d_config = val & 0xffff5fff;
				exp.valid[1] |= 1 << 17;
				break;
			case 14:
				reg = 0x40081c;
				exp.d3d_stencil_func = val & 0xfffffff1;
				exp.valid[1] |= 1 << 18;
				break;
			case 15:
				reg = 0x400820;
				exp.d3d_stencil_op = val & 0x00000fff;
				exp.valid[1] |= 1 << 19;
				break;
			case 16:
				reg = 0x400824;
				exp.d3d_blend = val & 0xff1111ff;
				exp.valid[1] |= 1 << 20;
				break;
			case 17:
				idx = rnd() & 0xf;
				reg = 0x400d00 + idx * 4;
				exp.vtx_u[idx] = val & 0xffffffc0;
				break;
			case 18:
				idx = rnd() & 0xf;
				reg = 0x400d40 + idx * 4;
				exp.vtx_v[idx] = val & 0xffffffc0;
				break;
			case 19:
				idx = rnd() & 0xf;
				reg = 0x400d80 + idx * 4;
				exp.vtx_m[idx] = val & 0xffffffc0;
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteD3D56Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteDmaNv3Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type == 3; }
	void mutate() override {
		val = rnd();
		uint32_t offset_mask = chipset.is_nv03t ? 0x007fffff : 0x003fffff;
		int idx;
		switch (rnd() % 14) {
			default:
				reg = 0x401140;
				exp.dma_intr_en = val & 0x11111;
				break;
			case 1:
				reg = 0x401210;
				exp.dma_eng_flags[0] = val & 0x03010fff;
				break;
			case 2:
				reg = 0x401220;
				exp.dma_eng_limit[0] = val;
				break;
			case 3:
				reg = 0x401230;
				exp.dma_eng_pte[0] = val & 0xfffff003;
				break;
			case 4:
				reg = 0x401240;
				exp.dma_eng_pte_tag[0] = val & 0xfffff000;
				break;
			case 5:
				reg = 0x401250;
				exp.dma_eng_addr_virt_adj[0] = val;
				break;
			case 6:
				reg = 0x401260;
				exp.dma_eng_addr_phys[0] = val;
				break;
			case 7:
				reg = 0x401270;
				exp.dma_eng_bytes[0] = val & offset_mask;
				break;
			case 8:
				reg = 0x401280;
				exp.dma_eng_inst[0] = val & 0xffff;
				break;
			case 9:
				reg = 0x401290;
				exp.dma_eng_lines[0] = val & 0x7ff;
				break;
			case 10:
				reg = 0x401400;
				exp.dma_lin_limit = val & offset_mask;
				break;
			case 11:
				idx = rnd() % 3;
				reg = 0x401800 + idx * 0x10;
				exp.dma_offset[idx] = val;
				break;
			case 12:
				reg = 0x401830;
				exp.dma_pitch = val;
				break;
			case 13:
				reg = 0x401840;
				exp.dma_misc = val & 0x707;
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteDmaNv3Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteDmaNv4Test : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type >= 4; }
	void mutate() override {
		val = rnd();
		int idx;
		switch (rnd() % 14) {
			default:
				idx = rnd() & 1;
				reg = 0x401000 + idx * 4;
				exp.dma_offset[idx] = val;
				break;
			case 1:
				reg = 0x401008;
				exp.dma_length = val & 0x3fffff;
				break;
			case 2:
				reg = 0x40100c;
				exp.dma_misc = val & 0x77ffff;
				break;
			case 3:
				idx = rnd() & 1;
				reg = 0x401020 + idx * 4;
				exp.dma_unk20[idx] = val;
				break;
			case 4:
				idx = rnd() & 1;
				reg = 0x401040 + idx * 0x40;
				exp.dma_eng_inst[idx] = val & 0xffff;
				break;
			case 5:
				idx = rnd() & 1;
				reg = 0x401044 + idx * 0x40;
				exp.dma_eng_flags[idx] = val & 0xfff33000;
				break;
			case 6:
				idx = rnd() & 1;
				reg = 0x401048 + idx * 0x40;
				exp.dma_eng_limit[idx] = val;
				break;
			case 7:
				idx = rnd() & 1;
				reg = 0x40104c + idx * 0x40;
				exp.dma_eng_pte[idx] = val & 0xfffff002;
				break;
			case 8:
				idx = rnd() & 1;
				reg = 0x401050 + idx * 0x40;
				exp.dma_eng_pte_tag[idx] = val & 0xfffff000;
				break;
			case 9:
				idx = rnd() & 1;
				reg = 0x401054 + idx * 0x40;
				exp.dma_eng_addr_virt_adj[idx] = val;
				break;
			case 10:
				idx = rnd() & 1;
				reg = 0x401058 + idx * 0x40;
				exp.dma_eng_addr_phys[idx] = val;
				break;
			case 11:
				idx = rnd() & 1;
				reg = 0x40105c + idx * 0x40;
				exp.dma_eng_bytes[idx] = val & 0x1ffffff;
				break;
			case 12:
				idx = rnd() & 1;
				reg = 0x401060 + idx * 0x40;
				exp.dma_eng_lines[idx] = val & 0x7ff;
				break;
			case 13:
				if (!nv04_pgraph_is_nv17p(&chipset))
					return;
				reg = 0x40103c;
				exp.dma_unk3c = val & 0x1f;
				break;
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteDmaNv4Test(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class ClipStatusTest : public hwtest::RepeatTest {
protected:
	bool supported() override {
		// XXX
		return chipset.card_type < 3;
	}
	int run_once() override {
		int xy = rnd() & 1;
		struct pgraph_state exp;
		pgraph_gen_state(cnum, rnd, &exp);
		uint32_t cls = extr(exp.access, 12, 5);
		pgraph_load_state(cnum, &exp);
		int32_t min, max;
		int32_t min_exp[2], max_exp[2];
		nv01_pgraph_clip_bounds(&exp, min_exp, max_exp);
		if (nv01_pgraph_is_tex_class(&exp)) {
			min = max = 0x40000000;
			int bit;
			for (bit = 30; bit >= 15; bit--) {
				nva_wr32(cnum, 0x400400 + xy * 0x80, min ^ 1 << bit);
				if (nva_rd32(cnum, 0x400648 + xy * 4) & 0x300) {
					min ^= 1 << bit;
				}
				nva_wr32(cnum, 0x400400 + xy * 0x80, max ^ 1 << bit);
				if (!(nva_rd32(cnum, 0x400648 + xy * 4) & 0x400)) {
					max ^= 1 << bit;
				}
			}
			min >>= 15;
			max >>= 15;
			if (exp.xy_misc_1[0] & 0x02000000) {
				min >>= 4, max >>= 4;
				if (min_exp[xy] & 0x800)
					min_exp[xy] = 0x7ff;
				if (max_exp[xy] & 0x800)
					max_exp[xy] = 0x7ff;
			}
		} else {
			min = max = 0x20000;
			int bit;
			for (bit = 17; bit >= 0; bit--) {
				nva_wr32(cnum, 0x400400 + xy * 0x80, min ^ 1 << bit);
				if (nva_rd32(cnum, 0x400648 + xy * 4) & 0x300) {
					min ^= 1 << bit;
				}
				nva_wr32(cnum, 0x400400 + xy * 0x80, max ^ 1 << bit);
				if (!(nva_rd32(cnum, 0x400648 + xy * 4) & 0x400)) {
					max ^= 1 << bit;
				}
			}
		}
		if (min_exp[xy] != min || max_exp[xy] != max) {
			printf("%08x %08x %08x %08x  %08x %08x  %08x %08x  %08x  %03x %03x\n", cls, exp.xy_misc_1[0], min, max, exp.dst_canvas_min, exp.dst_canvas_max, exp.uclip_min[0][xy], exp.uclip_max[0][xy], exp.iclip[xy], min_exp[xy], max_exp[xy]);
			return HWTEST_RES_FAIL;
		}
		return HWTEST_RES_PASS;
	}
public:
	ClipStatusTest(hwtest::TestOptions &opt, uint32_t seed) : RepeatTest(opt, seed) {}
};

class VtxWriteTest : public StateTest {
private:
	uint32_t reg, val;
protected:
	void adjust_orig() override {
		if (chipset.card_type < 3 && rnd() & 1) {
			/* rare and complicated enough to warrant better testing */
			orig.access = 0x0f00d111 + (rnd() & 0x11000);
		}
	}
	void mutate() override {
		int idx = rnd() % pgraph_vtx_count(&chipset);
		int xy = rnd() & 1;
		int rel = rnd() & 1 && chipset.card_type < 3;
		if (rnd() & 1 && chipset.card_type < 3) {
			/* rare and complicated enough to warrant better testing */
			idx &= 1;
			idx |= 0x10;
		}
		reg = 0x400400 + idx * 4 + xy * 0x80 + rel * 0x100;
		val = rnd();
		if (rnd() & 1) {
			switch (rnd() & 3) {
				case 0:
					val = orig.uclip_min[0][xy];
					break;
				case 1:
					val = orig.uclip_max[0][xy];
					break;
				case 2:
					val = orig.uclip_min[1][xy];
					break;
				case 3:
					val = orig.uclip_max[1][xy];
					break;
			}
		}
		if (rnd() & 1)
			val = extrs(val, 0, 18);
		nva_wr32(cnum, reg, val);
		if (chipset.card_type < 3) {
			nv01_pgraph_vtx_fixup(&exp, xy, idx, val, rel, -1, rel ? idx & 3 : 0);
		} else if (chipset.card_type < 4) {
			exp.vtx_xy[idx][xy] = val;
			nv03_pgraph_vtx_fixup(&exp, xy, 8, val);
		} else {
			exp.vtx_xy[idx][xy] = val;
			nv04_pgraph_vtx_fixup(&exp, xy, 8, val);
		}
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	VtxWriteTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class VtxBetaWriteTest : public StateTest {
private:
	uint32_t reg, val;
protected:
	bool supported() override { return chipset.card_type == 1; }
	void mutate() override {
		int idx = rnd() % 14;
		reg = 0x400700 + idx * 4;
		val = rnd();
		nva_wr32(cnum, reg, val);
		exp.vtx_beta[idx] = val & 0x01ffffff;
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	VtxBetaWriteTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class IClipWriteTest : public StateTest {
private:
	uint32_t reg, val;
protected:
	void mutate() override {
		int xy = rnd() & 1;
		int rel = rnd() & 1;
		if (chipset.card_type < 3) {
			reg = 0x400450 + xy * 4 + rel * 0x100;
		} else {
			rel = false;
			reg = 0x400534 + xy * 4;
		}
		val = rnd();
		if (rnd() & 1) {
			switch (rnd() & 3) {
				case 0:
					val = orig.uclip_min[0][xy];
					break;
				case 1:
					val = orig.uclip_max[0][xy];
					break;
				case 2:
					val = orig.uclip_min[1][xy];
					break;
				case 3:
					val = orig.uclip_max[1][xy];
					break;
			}
		}
		if (rnd() & 1)
			val = extrs(val, 0, 18);
		nva_wr32(cnum, reg, val);
		if (chipset.card_type < 4) {
			insrt(exp.xy_misc_1[0], 14, 1, 0);
			insrt(exp.xy_misc_1[0], 18, 1, 0);
			insrt(exp.xy_misc_1[0], 20, 1, 0);
			insrt(exp.xy_misc_1[1], 14, 1, 0);
			insrt(exp.xy_misc_1[1], 18, 1, 0);
			insrt(exp.xy_misc_1[1], 20, 1, 0);
		} else {
			insrt(exp.xy_misc_1[0], 12, 1, 0);
			insrt(exp.xy_misc_1[0], 16, 1, 0);
			insrt(exp.xy_misc_1[0], 20, 1, 0);
			insrt(exp.xy_misc_1[1], 12, 1, 0);
			insrt(exp.xy_misc_1[1], 16, 1, 0);
			insrt(exp.xy_misc_1[1], 20, 1, 0);
		}
		if (chipset.card_type < 3) {
			nv01_pgraph_iclip_fixup(&exp, xy, val, rel);
		} else if (chipset.card_type < 4) {
			nv03_pgraph_iclip_fixup(&exp, xy, val);
		} else {
			exp.iclip[xy] = val & 0x3ffff;
			nv04_pgraph_iclip_fixup(&exp, xy, val);
		}
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	IClipWriteTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class UClipWriteTest : public StateTest {
private:
	uint32_t reg, val;
protected:
	void mutate() override {
		int xy = rnd() & 1;
		int idx = rnd() & 1;
		int rel = rnd() & 1;
		int which;
		if (chipset.card_type < 3) {
			which = 0;
			reg = 0x400460 + xy * 8 + idx * 4 + rel * 0x100;
		} else {
			rel = false;
			uint32_t ubase[3] = { 0x40053c, 0x400560, 0x400550 };
			which = rnd() & 1;
			if (chipset.card_type == 0x10)
				which = rnd() % 3;
			reg = ubase[which] + xy * 4 + idx * 8;
		}
		val = rnd();
		if (rnd() & 1)
			val = extrs(val, 0, 18);
		nva_wr32(cnum, reg, val);
		if (chipset.card_type < 3) {
			nv01_pgraph_uclip_fixup(&exp, xy, idx, val, rel);
		} else if (chipset.card_type < 4) {
			nv03_pgraph_uclip_fixup(&exp, which, xy, idx, val);
		} else {
			nv04_pgraph_uclip_write(&exp, which, xy, idx, val);
		}
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	UClipWriteTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class FormatsTest : public StateTest {
	uint32_t val;
protected:
	bool supported() override { return chipset.card_type >= 4; }
	void adjust_orig() override {
		if (!(rnd() & 3)) {
			uint32_t classes[] = {
				0x1f, 0x5f,
				0x48, 0x54, 0x55,
				0x37, 0x77,
				0x60, 0x38, 0x39, 0x64,
			};
			insrt(orig.ctx_switch[0], 0, 8, classes[rnd()%ARRAY_SIZE(classes)]);
		}
		if (rnd() & 1) {
			orig.chroma = 0;
			if (rnd() & 1) {
				orig.chroma |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				orig.chroma |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				orig.chroma |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				orig.chroma |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 3) {
				insrt(orig.ctx_format, 24, 8, 2);
			}
			if (rnd() & 3) {
				orig.surf_format = 3;
				orig.ctx_switch[1] = 0x0d00;
				insrt(orig.ctx_switch[0], 15, 3, 0);
			}
			if (rnd() & 3) {
				uint32_t formats[] = {
					2, 6, 8, 0xb, 0x10,
				};
				insrt(orig.ctx_format, 24, 6, formats[rnd()%ARRAY_SIZE(formats)]);
			}
		}
	}
	void mutate() override {
		val = nva_rd32(cnum, 0x400618);
	}
	bool other_fail() override {
		uint32_t ev = nv04_pgraph_formats(&orig);
		if (ev != val) {
			printf("Got formats %08x expected %08x\n", val, ev);
			return true;
		}
		return false;
	}
public:
	FormatsTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

}

bool PGraphStateTests::supported() {
	return chipset.card_type < 0x20;
}

Test::Subtests PGraphStateTests::subtests() {
	return {
		{"state", new StateTest(opt, rnd())},
		{"soft_reset", new SoftResetTest(opt, rnd())},
		{"mmio_read", new MMIOReadTest(opt, rnd())},
		{"mmio_write_control_nv1", new MMIOWriteControlNv1Test(opt, rnd())},
		{"mmio_write_control_nv3", new MMIOWriteControlNv3Test(opt, rnd())},
		{"mmio_write_control_nv4", new MMIOWriteControlNv4Test(opt, rnd())},
		{"mmio_write_canvas_nv1", new MMIOWriteCanvasNv1Test(opt, rnd())},
		{"mmio_write_canvas_nv4", new MMIOWriteCanvasNv4Test(opt, rnd())},
		{"mmio_write_vstate_nv1", new MMIOWriteVStateNv1Test(opt, rnd())},
		{"mmio_write_vstate_nv3", new MMIOWriteVStateNv3Test(opt, rnd())},
		{"mmio_write_rop_nv1", new MMIOWriteRopNv1Test(opt, rnd())},
		{"mmio_write_rop_nv4", new MMIOWriteRopNv4Test(opt, rnd())},
		{"mmio_write_d3d0", new MMIOWriteD3D0Test(opt, rnd())},
		{"mmio_write_d3d56", new MMIOWriteD3D56Test(opt, rnd())},
		{"mmio_write_dma_nv3", new MMIOWriteDmaNv3Test(opt, rnd())},
		{"mmio_write_dma_nv4", new MMIOWriteDmaNv4Test(opt, rnd())},
		{"mmio_vtx_write", new VtxWriteTest(opt, rnd())},
		{"mmio_vtx_beta_write", new VtxBetaWriteTest(opt, rnd())},
		{"mmio_iclip_write", new IClipWriteTest(opt, rnd())},
		{"mmio_uclip_write", new UClipWriteTest(opt, rnd())},
		{"clip_status", new ClipStatusTest(opt, rnd())},
		{"formats", new FormatsTest(opt, rnd())},
	};
}

}
}
