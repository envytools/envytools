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
	nv01_pgraph_gen_state(cnum, rnd, &orig);
	adjust_orig();
	nv01_pgraph_load_state(cnum, &orig);
	exp = orig;
	mutate();
	nv01_pgraph_dump_state(cnum, &real);
	bool fail = other_fail();
	if (skip)
		return HWTEST_RES_NA;
	if (nv01_pgraph_cmp_state(&orig, &exp, &real, fail)) {
		print_fail();
		return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

namespace {

class SoftResetTest : public StateTest {
protected:
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
		}
		nva_rd32(cnum, reg);
	}
	void print_fail() {
		printf("After reading %08x\n", reg);
	}
public:
	MMIOReadTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MMIOWriteTest : public StateTest {
private:
	uint32_t reg, val;
protected:
	void mutate() override {
		val = rnd();
		int idx;
		bool xy;
		if (chipset.card_type < 3) {
			switch (rnd() % 50) {
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
					idx = rnd() % 18;
					xy = rnd() & 1;
					reg = 0x400400 + idx * 4 + xy * 0x80;
					exp.vtx_xy[idx][xy] = val;
					nv01_pgraph_vtx_fixup(&exp, xy, idx, val, 0, -1, 0);
					break;
				case 5:
					idx = rnd() % 18;
					xy = rnd() & 1;
					reg = 0x400500 + idx * 4 + xy * 0x80;
					exp.vtx_xy[idx][xy] = val;
					nv01_pgraph_vtx_fixup(&exp, xy, idx, val, 1, -1, idx & 3);
					break;
				case 8:
					idx = rnd() % 14;
					reg = 0x400700 + idx * 4;
					exp.vtx_beta[idx] = val & 0x01ffffff;
					break;
				case 9:
					idx = rnd() & 1;
					reg = 0x400450 + idx * 4;
					insrt(exp.xy_misc_1[0], 14, 1, 0);
					insrt(exp.xy_misc_1[0], 18, 1, 0);
					insrt(exp.xy_misc_1[0], 20, 1, 0);
					nv01_pgraph_iclip_fixup(&exp, idx, val, 0);
					break;
				case 10:
					idx = rnd() & 1;
					reg = 0x400550 + idx * 4;
					insrt(exp.xy_misc_1[0], 14, 1, 0);
					insrt(exp.xy_misc_1[0], 18, 1, 0);
					insrt(exp.xy_misc_1[0], 20, 1, 0);
					nv01_pgraph_iclip_fixup(&exp, idx, val, 1);
					break;
				case 11:
					idx = rnd() & 3;
					reg = 0x400460 + idx * 4;
					nv01_pgraph_uclip_fixup(&exp, idx >> 1, idx & 1, val, 0);
					break;
				case 12:
					idx = rnd() & 3;
					reg = 0x400560 + idx * 4;
					nv01_pgraph_uclip_fixup(&exp, idx >> 1, idx & 1, val, 1);
					break;
				case 13:
					idx = rnd() & 1;
					reg = 0x400600 + idx * 8;
					exp.pattern_mono_rgb[idx] = val & 0x3fffffff;
					break;
				case 14:
					idx = rnd() & 1;
					reg = 0x400604 + idx * 8;
					exp.pattern_mono_a[idx] = val & 0xff;
					break;
				case 15:
					idx = rnd() & 1;
					reg = 0x400610 + idx * 4;
					exp.pattern_mono_bitmap[idx] = val;
					break;
				case 16:
					reg = 0x400618;
					exp.pattern_config = val & 3;
					break;
				case 17:
					idx = rnd() & 1;
					reg = 0x40061c + idx * 4;
					exp.bitmap_color[idx] = val & 0x7fffffff;
					break;
				case 18:
					reg = 0x400624;
					exp.rop = val & 0xff;
					break;
				case 19:
					reg = 0x400628;
					exp.plane = val & 0x7fffffff;
					break;
				case 20:
					reg = 0x40062c;
					exp.chroma = val & 0x7fffffff;
					break;
				case 21:
					reg = 0x400630;
					exp.beta = val & 0x7f800000;
					if (val & 0x80000000)
						exp.beta = 0;
					break;
				case 22:
					reg = 0x400634;
					exp.canvas_config = val & 0x01111011;
					break;
				case 23:
					reg = 0x400688;
					exp.dst_canvas_min = val & 0xffffffff;
					break;
				case 24:
					reg = 0x40068c;
					exp.dst_canvas_max = val & 0x0fff0fff;
					break;
				case 25:
					idx = rnd() & 1;
					reg = 0x400690 + idx * 8;
					exp.cliprect_min[idx] = val & 0x0fff0fff;
					break;
				case 26:
					idx = rnd() & 1;
					reg = 0x400694 + idx * 8;
					exp.cliprect_max[idx] = val & 0x0fff0fff;
					break;
				case 27:
					reg = 0x4006a0;
					exp.cliprect_ctrl = val & 0x113;
					break;
				case 28:
					reg = 0x400640;
					exp.xy_a = val & 0xf1ff11ff;
					break;
				case 29:
					idx = rnd() & 1;
					reg = 0x400644;
					exp.xy_misc_1[0] = val & 0x03177331;
					break;
				case 31:
					idx = rnd() & 1;
					reg = 0x400648 + idx * 4;
					exp.xy_misc_4[idx] = val & 0x30ffffff;
					break;
				case 32:
					reg = 0x400650;
					exp.valid[0] = val & 0x111ff1ff;
					break;
				case 33:
					reg = 0x400654;
					exp.misc32[0] = val;
					break;
				case 34:
					reg = 0x400658;
					exp.subdivide = val & 0xffff00ff;
					break;
				case 35:
					reg = 0x40065c;
					exp.edgefill = val & 0xffff0113;
					break;
				case 44:
					reg = 0x400680;
					exp.ctx_switch[1] = val & 0xffff;
					break;
				case 45:
					reg = 0x400684;
					exp.notify = val & 0x11ffff;
					break;
				case 46:
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
				case 47:
					reg = 0x400080;
					exp.debug[0] = val & 0x11111110;
					if (extr(val, 0, 1))
						pgraph_reset(&exp);
					break;
				case 48:
					reg = 0x400084;
					exp.debug[1] = val & 0x31111101;
					break;
				case 49:
					reg = 0x400088;
					exp.debug[2] = val & 0x11111111;
					break;
			}
		} else {
			uint32_t offset_mask = chipset.is_nv03t ? 0x007fffff : 0x003fffff;
			uint32_t canvas_mask = chipset.is_nv03t ? 0x7fff07ff : 0x3fff07ff;
			switch (rnd() % 69) {
				default:
					reg = 0x400140;
					exp.intr_en = val & 0x11111111;
					break;
				case 1:
					reg = 0x400144;
					exp.invalid_en = val & 0x11111;
					break;
				case 2:
					reg = 0x401140;
					exp.dma_intr_en = val & 0x11111;
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
					idx = rnd() & 0x1f;
					xy = rnd() & 1;
					reg = 0x400400 + idx * 4 + xy * 0x80;
					exp.vtx_xy[idx][xy] = val;
					nv03_pgraph_vtx_fixup(&exp, xy, 8, val);
					break;
				case 9:
					idx = rnd() & 0xf;
					reg = 0x400580 + idx * 4;
					exp.vtx_z[idx] = val & 0xffffff;
					break;
				case 10:
					idx = rnd() & 1;
					reg = 0x400534 + idx * 4;
					insrt(exp.xy_misc_1[0], 14, 1, 0);
					insrt(exp.xy_misc_1[0], 18, 1, 0);
					insrt(exp.xy_misc_1[0], 20, 1, 0);
					insrt(exp.xy_misc_1[1], 14, 1, 0);
					insrt(exp.xy_misc_1[1], 18, 1, 0);
					insrt(exp.xy_misc_1[1], 20, 1, 0);
					nv03_pgraph_iclip_fixup(&exp, idx, val);
					break;
				case 11:
					idx = rnd() & 3;
					reg = 0x40053c + idx * 4;
					nv03_pgraph_uclip_fixup(&exp, 0, idx & 1, idx >> 1, val);
					break;
				case 12:
					idx = rnd() & 3;
					reg = 0x400560 + idx * 4;
					nv03_pgraph_uclip_fixup(&exp, 1, idx & 1, idx >> 1, val);
					break;
				case 13:
					idx = rnd() & 1;
					reg = 0x400600 + idx * 8;
					exp.pattern_mono_rgb[idx] = val & 0x3fffffff;
					break;
				case 14:
					idx = rnd() & 1;
					reg = 0x400604 + idx * 8;
					exp.pattern_mono_a[idx] = val & 0xff;
					break;
				case 15:
					idx = rnd() & 1;
					reg = 0x400610 + idx * 4;
					exp.pattern_mono_bitmap[idx] = val;
					break;
				case 16:
					reg = 0x400618;
					exp.pattern_config = val & 3;
					break;
				case 17:
					reg = 0x40061c;
					exp.bitmap_color[0] = val & 0x7fffffff;
					break;
				case 18:
					reg = 0x400624;
					exp.rop = val & 0xff;
					break;
				case 19:
					reg = 0x40062c;
					exp.chroma = val & 0x7fffffff;
					break;
				case 20:
					reg = 0x400640;
					exp.beta = val & 0x7f800000;
					if (val & 0x80000000)
						exp.beta = 0;
					break;
				case 21:
					reg = 0x400550;
					exp.src_canvas_min = val & canvas_mask;
					break;
				case 22:
					reg = 0x400554;
					exp.src_canvas_max = val & canvas_mask;
					break;
				case 23:
					reg = 0x400558;
					exp.dst_canvas_min = val & canvas_mask;
					break;
				case 24:
					reg = 0x40055c;
					exp.dst_canvas_max = val & canvas_mask;
					break;
				case 25:
					idx = rnd() & 1;
					reg = 0x400690 + idx * 8;
					exp.cliprect_min[idx] = val & canvas_mask;
					break;
				case 26:
					idx = rnd() & 1;
					reg = 0x400694 + idx * 8;
					exp.cliprect_max[idx] = val & canvas_mask;
					break;
				case 27:
					reg = 0x4006a0;
					exp.cliprect_ctrl = val & 0x113;
					break;
				case 28:
					reg = 0x400514;
					exp.xy_a = val & 0xf013ffff;
					break;
				case 29:
					idx = rnd() & 1;
					reg = 0x400518 + idx * 4;
					exp.xy_misc_1[idx] = val & 0x0f177331;
					break;
				case 30:
					reg = 0x400520;
					exp.xy_misc_3 = val & 0x7f7f1111;
					break;
				case 31:
					idx = rnd() & 1;
					reg = 0x400500 + idx * 4;
					exp.xy_misc_4[idx] = val & 0x300000ff;
					break;
				case 32:
					idx = rnd() & 3;
					reg = 0x400524 + idx * 4;
					exp.xy_clip[idx >> 1][idx & 1] = val;
					break;
				case 33:
					reg = 0x400508;
					exp.valid[0] = val;
					break;
				case 34:
					idx = rnd() & 1;
					reg = ((uint32_t[2]){0x400510, 0x400570})[idx];
					exp.misc24[idx] = val & 0xffffff;
					break;
				case 35:
					idx = rnd() & 1;
					reg = ((uint32_t[2]){0x40050c, 0x40054c})[idx];
					exp.misc32[idx] = val;
					if (idx == 0)
						exp.valid[0] |= 1 << 16;
					break;
				case 36:
					reg = 0x4005c0;
					exp.d3d_tlv_xy = val;
					break;
				case 37:
					reg = 0x4005c4;
					exp.d3d_tlv_uv[0][0] = val;
					break;
				case 38:
					reg = 0x4005c8;
					exp.d3d_tlv_z = val & 0xffff;
					break;
				case 39:
					reg = 0x4005cc;
					exp.d3d_tlv_color = val;
					break;
				case 40:
					reg = 0x4005d0;
					exp.d3d_tlv_fog_tri_col1 = val;
					break;
				case 41:
					reg = 0x4005d4;
					exp.d3d_tlv_rhw = val;
					break;
				case 42:
					reg = 0x400644;
					exp.d3d_config = val & 0xf77fbdf3;
					insrt(exp.valid[0], 26, 1, 1);
					break;
				case 43:
					reg = 0x4006c8;
					exp.d3d_alpha = val & 0xfff;
					break;
				case 44:
					reg = 0x400680;
					exp.ctx_switch[1] = val & 0xffff;
					break;
				case 45:
					reg = 0x400684;
					exp.notify = val & 0xf1ffff;
					break;
				case 46:
					reg = 0x400688;
					exp.ctx_switch[3] = val & 0xffff;
					break;
				case 47:
					reg = 0x40068c;
					exp.ctx_switch[2] = val & 0x1ffff;
					break;
				case 48:
					idx = rnd() & 3;
					reg = 0x400630 + idx * 4;
					exp.surf_offset[idx] = val & offset_mask & ~0xf;
					break;
				case 49:
					idx = rnd() & 3;
					reg = 0x400650 + idx * 4;
					exp.surf_pitch[idx] = val & 0x1ff0;
					break;
				case 50:
					reg = 0x4006a8;
					exp.surf_format = val & 0x7777;
					break;
				case 51:
					reg = 0x401210;
					exp.dma_eng_flags[0] = val & 0x03010fff;
					break;
				case 52:
					reg = 0x401220;
					exp.dma_eng_limit[0] = val;
					break;
				case 53:
					reg = 0x401230;
					exp.dma_eng_pte[0] = val & 0xfffff003;
					break;
				case 54:
					reg = 0x401240;
					exp.dma_eng_pte_tag[0] = val & 0xfffff000;
					break;
				case 55:
					reg = 0x401250;
					exp.dma_eng_addr_virt_adj[0] = val;
					break;
				case 56:
					reg = 0x401260;
					exp.dma_eng_addr_phys[0] = val;
					break;
				case 57:
					reg = 0x401270;
					exp.dma_eng_bytes[0] = val & offset_mask;
					break;
				case 58:
					reg = 0x401280;
					exp.dma_eng_inst[0] = val & 0xffff;
					break;
				case 59:
					reg = 0x401290;
					exp.dma_eng_lines[0] = val & 0x7ff;
					break;
				case 60:
					reg = 0x401400;
					exp.dma_lin_limit = val & offset_mask;
					break;
				case 61:
					idx = rnd() % 3;
					reg = 0x401800 + idx * 0x10;
					exp.dma_offset[idx] = val;
					break;
				case 62:
					reg = 0x401830;
					exp.dma_pitch = val;
					break;
				case 63:
					reg = 0x401840;
					exp.dma_misc = val & 0x707;
					break;
				case 64:
					reg = 0x4006a4;
					exp.fifo_enable = val & 1;
					break;
				case 65:
					reg = 0x400080;
					exp.debug[0] = val & 0x13311110;
					if (extr(val, 0, 1))
						pgraph_reset(&exp);
					break;
				case 66:
					reg = 0x400084;
					exp.debug[1] = val & 0x10113301;
					if (extr(val, 4, 1))
						insrt(exp.xy_misc_1[0], 0, 1, 0);
					break;
				case 67:
					reg = 0x400088;
					exp.debug[2] = val & 0x1133f111;
					break;
				case 68:
					reg = 0x40008c;
					exp.debug[3] = val & 0x1173ff31;
					break;
			}
		}
		nva_wr32(cnum, reg, val);
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	MMIOWriteTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
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
		nv01_pgraph_gen_state(cnum, rnd, &exp);
		uint32_t cls = extr(exp.access, 12, 5);
		nv01_pgraph_load_state(cnum, &exp);
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
		nva_wr32(cnum, reg, val);
		if (chipset.card_type < 3) {
			nv01_pgraph_vtx_fixup(&exp, xy, idx, val, rel, -1, rel ? idx & 3 : 0);
		} else {
			exp.vtx_xy[idx][xy] = val;
			nv03_pgraph_vtx_fixup(&exp, xy, 8, val);
		}
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	VtxWriteTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
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
		nva_wr32(cnum, reg, val);
		insrt(exp.xy_misc_1[0], 14, 1, 0);
		insrt(exp.xy_misc_1[0], 18, 1, 0);
		insrt(exp.xy_misc_1[0], 20, 1, 0);
		insrt(exp.xy_misc_1[1], 14, 1, 0);
		insrt(exp.xy_misc_1[1], 18, 1, 0);
		insrt(exp.xy_misc_1[1], 20, 1, 0);
		if (chipset.card_type < 3) {
			nv01_pgraph_iclip_fixup(&exp, xy, val, rel);
		} else {
			nv03_pgraph_iclip_fixup(&exp, xy, val);
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
			which = rnd() & 1;
			reg = (which ? 0x400560 : 0x40053c) + xy * 4 + idx * 8;
		}
		val = rnd();
		nva_wr32(cnum, reg, val);
		if (chipset.card_type < 3) {
			nv01_pgraph_uclip_fixup(&exp, xy, idx, val, rel);
		} else {
			nv03_pgraph_uclip_fixup(&exp, which, xy, idx, val);
		}
	}
	void print_fail() {
		printf("After writing %08x <- %08x\n", reg, val);
	}
public:
	UClipWriteTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

}

bool PGraphStateTests::supported() {
	return chipset.card_type < 4;
}

Test::Subtests PGraphStateTests::subtests() {
	return {
		{"state", new StateTest(opt, rnd())},
		{"soft_reset", new SoftResetTest(opt, rnd())},
		{"mmio_read", new MMIOReadTest(opt, rnd())},
		{"mmio_write", new MMIOWriteTest(opt, rnd())},
		{"mmio_vtx_write", new VtxWriteTest(opt, rnd())},
		{"mmio_iclip_write", new IClipWriteTest(opt, rnd())},
		{"mmio_uclip_write", new UClipWriteTest(opt, rnd())},
		{"clip_status", new ClipStatusTest(opt, rnd())},
	};
}

}
}
