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

class MthdSifmFormat : public SingleMthdTest {
	bool is_new;
	bool supported() override { return chipset.card_type >= 4; }
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		uint32_t max = is_new ? 7 : 6;
		if (cls != 0x37 && cls != 0x77 && cls != 0x63 && cls != 0x67) {
			max = 9;
			if (nv04_pgraph_is_nv11p(&chipset) && extr(exp.ctx_switch[0], 22, 1)) {
				max = 0xb;
			}
		}
		return val <= max && val != 0;
	}
	void emulate_mthd() override {
		int sfmt = val & 7;
		if (chipset.card_type >= 0x10)
			sfmt = val & 0xf;
		int fmt = 0;
		if (sfmt == 1)
			fmt = 0x6;
		if (sfmt == 2)
			fmt = 0x7;
		if (sfmt == 3)
			fmt = 0xd;
		if (sfmt == 4)
			fmt = 0xe;
		if (sfmt == 5)
			fmt = 0x12;
		if (sfmt == 6)
			fmt = 0x13;
		if (sfmt == 7)
			fmt = 0xa;
		if (sfmt == 8)
			fmt = 0x1;
		if (sfmt == 9)
			fmt = 0x15;
		if (nv04_pgraph_is_nv11p(&chipset)) {
			if (sfmt == 0xa)
				fmt = 0x16;
			if (sfmt == 0xb)
				fmt = 0x17;
		}
		if (!extr(exp.nsource, 1, 1)) {
			insrt(egrobj[1], 8, 8, fmt);
			exp.ctx_cache[subc][1] = exp.ctx_switch[1];
			insrt(exp.ctx_cache[subc][1], 8, 8, fmt);
			if (extr(exp.debug[1], 20, 1))
				exp.ctx_switch[1] = exp.ctx_cache[subc][1];
		}
	}
public:
	MthdSifmFormat(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, bool is_new)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), is_new(is_new) {}
};

class MthdSurfDvdFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x31ff0;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		int sfmt = extr(val, 16, 16);
		if (sfmt == 0 || sfmt > 2)
			return false;
		if (val & (cls == 0x88 ? 0x3f : 0xe01f) || !(val & 0xffff))
			return false;
		return true;
	}
	void emulate_mthd() override {
		int fmt = 0;
		switch (val >> 16 & 0xf) {
			case 0x01:
				fmt = 0xe;
				break;
			case 0x02:
				fmt = 0xf;
				break;
		}
		exp.valid[0] |= 4;
		exp.surf_pitch[4] = val & pgraph_pitch_mask(&chipset);
		insrt(exp.ctx_valid, 12, 1, !extr(exp.nsource, 1, 1));
		insrt(exp.surf_format, 16, 4, fmt);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSifmXy : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 3))
			val &= 0x001f000f;
	}
	bool is_valid_val() override {
		return !pgraph_is_class_dvd(&exp) || !(val & 1);
	}
	void emulate_mthd() override {
		insrt(exp.valid[0], 0, 1, 1);
		if (chipset.card_type < 4)
			insrt(exp.valid[0], 12, 1, 1);
		else
			insrt(exp.valid[0], 1, 1, 0);
		exp.vtx_xy[0][0] = extrs(val, 0, 16);
		exp.vtx_xy[4][1] = extrs(val, 16, 16);
		insrt(exp.valid[0], 19, 1, pgraph_is_class_dvd(&exp));
		pgraph_clear_vtxid(&exp);
		pgraph_bump_vtxid(&exp);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		if (chipset.card_type < 4) {
			int xcstat = nv03_pgraph_clip_status(&exp, exp.vtx_xy[0][0], 0, false);
			int ycstat = nv03_pgraph_clip_status(&exp, exp.vtx_xy[4][1], 1, false);
			pgraph_set_xy_d(&exp, 0, 0, 0, false, false, false, xcstat);
			pgraph_set_xy_d(&exp, 1, 0, 0, false, false, false, ycstat);
		} else {
			int xcstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[0][0], 0);
			int ycstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[4][1], 1);
			pgraph_set_xy_d(&exp, 0, 0, 0, false, false, false, xcstat);
			pgraph_set_xy_d(&exp, 1, 0, 0, false, false, false, ycstat);
		}
		insrt(exp.xy_misc_3, 8, 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSifmRect : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 3))
			val &= 0x001f000f;
	}
	bool is_valid_val() override {
		return !pgraph_is_class_dvd(&exp) || !(val & 1);
	}
	void emulate_mthd() override {
		insrt(exp.valid[0], 1, 1, 1);
		if (chipset.card_type < 4)
			insrt(exp.valid[0], 9, 1, 1);
		if (chipset.card_type < 4) {
			nv03_pgraph_vtx_add(&exp, 0, 1, exp.vtx_xy[0][0], extr(val, 0, 16), 0, false, false);
			nv03_pgraph_vtx_add(&exp, 1, 1, 0, extr(val, 16, 16), 0, false, false);
		} else {
			nv04_pgraph_vtx_add(&exp, 0, 1, 0, extr(val, 0, 16), 0, false);
			nv04_pgraph_vtx_add(&exp, 1, 1, 0, extr(val, 16, 16), 0, false);
			pgraph_vtx_cmp(&exp, 0, 1, true);
			pgraph_vtx_cmp(&exp, 1, 1, true);
			nv04_pgraph_vtx_add(&exp, 0, 1, exp.vtx_xy[0][0], extr(val, 0, 16), 0, false);
		}
		pgraph_bump_vtxid(&exp);
		if (chipset.card_type < 4) {
			pgraph_vtx_cmp(&exp, 0, 8, true);
			pgraph_vtx_cmp(&exp, 1, 8, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSifmDuDx : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (!(rnd() & 3))
			val = sext(val, 21);
	}
	void emulate_mthd() override {
		if (chipset.card_type < 4) {
			int sv = val >> 17;
			if (sv >= 0x4000) {
				if (sv >= 0x7ff8)
					sv &= 0xf;
				else
					sv = 9;
			} else {
				if (sv > 7)
					sv = 7;
			}
			insrt(exp.xy_misc_1[0], 24, 4, sv);
		}
		insrt(exp.valid[0], which ? 12 : 4, 1, 1);
		exp.misc32[which ? 3 : 0] = val;
	}
public:
	MthdSifmDuDx(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdSifmDvDy : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (!(rnd() & 3))
			val = sext(val, 21);
	}
	void emulate_mthd() override {
		if (chipset.card_type < 4) {
			int sv = val >> 17;
			if (sv >= 0x4000) {
				if (sv >= 0x7ff8)
					sv &= 0xf;
				else
					sv = 9;
			} else {
				if (sv > 7)
					sv = 7;
			}
			insrt(exp.xy_misc_1[1], 24, 4, sv);
		}
		insrt(exp.valid[0], which ? 13 : 5, 1, 1);
		exp.vtx_xy[3][1 - which] = val;
	}
	using SingleMthdTest::SingleMthdTest;
public:
	MthdSifmDvDy(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdSifmSrcSize : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val &= ~0x7f0;
			val |= 0x400;
		}
		if (!(rnd() & 3)) {
			val &= ~0x7f00000;
			val |= 0x4000000;
		}
	}
	int gen_nv3_fmt() override {
		return rnd() % 7;
	}
	bool is_valid_val() override {
		if (chipset.card_type < 4 || cls == 0x37) {
			return (extr(val, 0, 16) <= 0x400 && extr(val, 16, 16) <= 0x400);
		} else {
			if (val & 0xf800f800)
				return false;
			if (val & 1)
				return false;
			if (!extr(val, 0, 16))
				return false;
			if (!extr(val, 16, 16))
				return false;
			return true;
		}
	}
	void emulate_mthd() override {
		if (which == 0)
			exp.misc24[0] = extr(val, 0, 16);
		insrt(exp.misc24[which + 1], 0, 11, extr(val, 0, 11));
		insrt(exp.misc24[which + 1], 11, 11, extr(val, 16, 11));
		insrt(exp.valid[0], which ? 15 : 9, 1, 1);
		if (chipset.card_type < 4)
			insrt(exp.valid[0], 13, 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
public:
	MthdSifmSrcSize(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdSyfmSrcSize : public SingleMthdTest {
	bool supported() override { return chipset.card_type == 3; }
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val &= ~0x7f0;
			val |= 0x400;
		}
		if (!(rnd() & 3)) {
			val &= ~0x7f00000;
			val |= 0x4000000;
		}
	}
	int gen_nv3_fmt() override {
		return 7;
	}
	bool is_valid_val() override {
		return (extr(val, 0, 16) <= 0x400 && extr(val, 16, 16) <= 0x400);
	}
	void emulate_mthd() override {
		exp.misc24[0] = extr(val, 0, 16);
		insrt(exp.misc24[1], 0, 11, extr(val, 0, 11));
		insrt(exp.misc24[1], 11, 11, extr(val, 16, 11));
		insrt(exp.valid[0], 9, 1, 0);
		insrt(exp.valid[0], 13, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSifmOffset : public SingleMthdTest {
	int which;
	int gen_nv3_fmt() override {
		return rnd() % 7;
	}
	void emulate_mthd() override {
		exp.dma_offset[which] = val;
		insrt(exp.valid[0], which ? 17 : 11, 1, 1);
		if (chipset.card_type < 4) {
			insrt(exp.valid[0], 15, 1, 0);
			insrt(exp.valid[0], 16, 1, 0);
			insrt(exp.valid[0], 17, 1, 0);
		}
	}
	using SingleMthdTest::SingleMthdTest;
public:
	MthdSifmOffset(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdSyfmOffset : public SingleMthdTest {
	bool supported() override { return chipset.card_type == 3; }
	int gen_nv3_fmt() override {
		return 7;
	}
	void emulate_mthd() override {
		exp.dma_offset[idx] = val;
		insrt(exp.valid[0], 11, 1, 0);
		insrt(exp.valid[0], 15 + idx, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSifmPitch : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x1031ff0;
			val ^= 1 << (rnd() & 0x1f);
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	int gen_nv3_fmt() override {
		return rnd() % 7;
	}
	bool is_valid_val() override {
		if (chipset.card_type < 4 || cls == 0x37) {
			return !(val & ~0x1fff);
		} else {
			if (!extr(val, 16, 8))
				return false;
			if (extr(val, 16, 8) > 2)
				return false;
			if (extr(val, 24, 8) > 1)
				return false;
			if (cls == 0x37 || cls == 0x77 || cls == 0x63 || cls == 0x67) {
				if (val & 0xe000)
					return false;
			}
			return true;
		}
	}
	void emulate_mthd_pre() override {
		if (chipset.card_type >= 4)
			exp.dma_pitch = extr(val, 0, 16) * 0x10001;
	}
	void emulate_mthd() override {
		insrt(exp.valid[0], 10, 1, 1);
		if (chipset.card_type < 4) {
			insrt(exp.dma_pitch, 0, 16, val);
			insrt(exp.valid[0], 14, 1, 0);
		} else {
			if (cls != 0x37) {
				insrt(exp.sifm_mode, 16, 2, extr(val, 16, 2));
				insrt(exp.sifm_mode, 24, 1, extr(val, 24, 1));
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdDvdSrcFormat : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x31fff;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		int sfmt = extr(val, 16, 16);
		if (sfmt == 0 || sfmt > (which ? 3 : 2))
			return false;
		if (val & 0xe000 && (cls == 0x38 || which == 1))
			return false;
		return true;
	}
	void emulate_mthd_pre() override {
		insrt(exp.dma_pitch, 16 * which, 16, val);
	}
	void emulate_mthd() override {
		insrt(exp.valid[0], which ? 16 : 10, 1, 1);
		int sfmt = extr(val, 16, 2);
		if (which == 0) {
			int fmt = 0;
			if (sfmt == 1)
				fmt = 0x12;
			if (sfmt == 2)
				fmt = 0x13;
			insrt(exp.dvd_format, 0, 6, fmt);
		} else {
			insrt(exp.dvd_format, 8, 2, sfmt);
		}
	}
public:
	MthdDvdSrcFormat(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdSyfmPitch : public SingleMthdTest {
	bool supported() override { return chipset.card_type == 3; }
	int gen_nv3_fmt() override {
		return 7;
	}
	void emulate_mthd() override {
		exp.dma_pitch = val;
		insrt(exp.valid[0], 10, 1, 0);
		insrt(exp.valid[0], 14, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSifmSrcPos : public SingleMthdTest {
	int which;
	bool draw;
	int gen_nv3_fmt() override {
		return rnd() % 7;
	}
	void adjust_orig_mthd() override {
		// XXX: disable this some day and test the actual DMA
		if (draw) {
			if (chipset.card_type < 4) {
				switch (rnd() % 2) {
					case 0:
						insrt(orig.xy_misc_4[0], 4, 4, 0xf);
						insrt(orig.ctx_user, 13, 3, subc);
						break;
					case 1:
						insrt(orig.cliprect_ctrl, 8, 1, 1);
						break;
				}
			} else {
				insrt(orig.xy_misc_4[0], 4, 4, 0xf);
				insrt(orig.ctx_user, 13, 3, subc);
				insrt(orig.notify, 0, 1, 0);
			}
		}
		if (rnd() & 1) {
			orig.valid[0] |= 0x100033;
			if (rnd() & 1)
				orig.valid[0] |= 0x000f00;
			if (rnd() & 1)
				orig.valid[0] |= 0x03f000;
			orig.valid[0] ^= 1 << (rnd() % 20);
		}
	}
	void emulate_mthd() override {
		exp.misc32[which ? 2 : 1] = val;
		insrt(exp.valid[0], which ? 14 : 8, 1, 1);
		if (which == 0)
			exp.vtx_xy[2][1] = val & 0xffff0000;
		if (draw) {
			int ycstat;
			exp.vtx_xy[0][1] = exp.vtx_xy[4][1];
			if (chipset.card_type < 4) {
				ycstat = nv03_pgraph_clip_status(&exp, exp.vtx_xy[0][1], 1, false);
			} else {
				ycstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[0][1], 1);
			}
			pgraph_set_xy_d(&exp, 1, 1, 1, false, false, false, ycstat);
			if (chipset.card_type < 4) {
				pgraph_prep_draw(&exp, false, false);
				insrt(exp.xy_misc_3, 0, 1, 0);
				insrt(exp.xy_a, 0, 18, exp.vtx_xy[1][1]);
			} else {
				// XXX
				skip = true;
			}
			pgraph_set_xy_d(&exp, 1, 1, 1, false, (int32_t)exp.vtx_xy[0][1] != (int16_t)exp.vtx_xy[0][1], false, ycstat);
		}
	}
public:
	MthdSifmSrcPos(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which, bool draw)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which), draw(draw) {}
};

class MthdSyfmSrcPos : public SingleMthdTest {
	bool supported() override { return chipset.card_type == 3; }
	int gen_nv3_fmt() override {
		return 7;
	}
	void adjust_orig_mthd() override {
		// XXX: disable this some day and test the actual DMA
		switch (rnd() % 2) {
			case 0:
				insrt(orig.xy_misc_4[0], 4, 4, 0xf);
				insrt(orig.ctx_user, 13, 3, subc);
				break;
			case 1:
				insrt(orig.cliprect_ctrl, 8, 1, 1);
				break;
		}
		if (rnd() & 1) {
			orig.valid[0] |= 0x100033;
			if (rnd() & 1)
				orig.valid[0] |= 0x000f00;
			if (rnd() & 1)
				orig.valid[0] |= 0x03f000;
			orig.valid[0] ^= 1 << (rnd() & 0x1f);
		}
	}
	void emulate_mthd() override {
		exp.misc32[1] = val;
		exp.vtx_xy[2][1] = val & 0xffff0000;
		insrt(exp.valid[0], 12, 1, 1);
		insrt(exp.valid[0], 8, 1, 0);
		exp.vtx_xy[0][1] = exp.vtx_xy[4][1];
		int ycstat = nv03_pgraph_clip_status(&exp, exp.vtx_xy[0][1], 1, false);
		pgraph_set_xy_d(&exp, 1, 1, 1, false, false, false, ycstat);
		pgraph_prep_draw(&exp, false, false);
		insrt(exp.xy_misc_3, 0, 1, 0);
		pgraph_set_xy_d(&exp, 1, 1, 1, false, (int32_t)exp.vtx_xy[0][1] != (int16_t)exp.vtx_xy[0][1], false, ycstat);
		insrt(exp.xy_a, 0, 18, exp.vtx_xy[1][1]);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSifmInvalid : public SingleMthdTest {
	bool supported() override { return chipset.card_type == 3; }
	int gen_nv3_fmt() override {
		return rnd() % 7;
	}
	void emulate_mthd() override {
		exp.intr |= 1;
		exp.invalid |= 1;
		exp.fifo_enable = 0;
	}
	using SingleMthdTest::SingleMthdTest;
};

}
std::vector<SingleMthdTest *> Sifm::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdDmaGrobj(opt, rnd(), "dma_src", 2, cls, 0x184, 0, DMA_R),
		new MthdCtxPattern(opt, rnd(), "ctx_pattern", 3, cls, 0x188, cls != 0x37),
		new MthdCtxRop(opt, rnd(), "ctx_rop", 4, cls, 0x18c),
		new MthdCtxBeta(opt, rnd(), "ctx_beta", 5, cls, 0x190),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200),
		new MthdSifmFormat(opt, rnd(), "format", 8, cls, 0x300, cls != 0x37),
		new MthdOperation(opt, rnd(), "operation", 9, cls, 0x304, cls != 0x37),
		new MthdClipXy(opt, rnd(), "clip_xy", 10, cls, 0x308, 1, 0),
		new MthdClipRect(opt, rnd(), "clip_rect", 11, cls, 0x30c, 1),
		new MthdSifmXy(opt, rnd(), "xy", 12, cls, 0x310),
		new MthdSifmRect(opt, rnd(), "rect", cls == 0x37 ? 11 : 13, cls, 0x314),
		new MthdSifmDuDx(opt, rnd(), "dudx", 14, cls, 0x318, 0),
		new MthdSifmDvDy(opt, rnd(), "dvdy", 15, cls, 0x31c, 0),
		new MthdSifmSrcSize(opt, rnd(), "src_size", 16, cls, 0x400, 0),
		new MthdSifmPitch(opt, rnd(), "pitch", 17, cls, 0x404),
		new MthdSifmOffset(opt, rnd(), "offset", 18, cls, 0x408, 0),
		new MthdSifmSrcPos(opt, rnd(), "src_pos", 19, cls, 0x40c, 0, true),
	};
	if (cls == 0xe) {
		res.insert(res.end(), {
			new MthdSyfmSrcSize(opt, rnd(), "src_size_yuv420", -1, cls, 0x400),
			new MthdSyfmPitch(opt, rnd(), "pitch_yuv420", -1, cls, 0x404),
			new MthdSyfmOffset(opt, rnd(), "offset_yuv420", -1, cls, 0x408, 3),
			new MthdSyfmSrcPos(opt, rnd(), "src_pos_yuv420", -1, cls, 0x414),
			new MthdSifmInvalid(opt, rnd(), "invalid_yuv420", -1, cls, 0x410, 2),
		});
	}
	if (cls == 0x37) {
		res.insert(res.end(), {
			new MthdCtxSurf(opt, rnd(), "ctx_dst", 7, cls, 0x194, 0),
		});
	} else {
		res.insert(res.end(), {
			new MthdCtxBeta4(opt, rnd(), "ctx_beta4", 6, cls, 0x194),
			new MthdCtxSurf2D(opt, rnd(), "ctx_surf2d", 7, cls, 0x198, (cls == 0x77 ? SURF2D_NV4 : SURF2D_NV10) | SURF2D_SWZOK),
		});
	}
	if (cls == 0x63 || (cls & 0xff) == 0x89) {
		res.insert(res.begin(), {
			new MthdDither(opt, rnd(), "dither", 20, cls, 0x2fc),
		});
	}
	if ((cls & 0xff) == 0x89) {
		res.insert(res.begin(), {
			new MthdSync(opt, rnd(), "sync", -1, cls, 0x108),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Dvd::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdDmaGrobj(opt, rnd(), "dma_ov", 2, cls, 0x184, 1, DMA_R),
		new MthdDmaGrobj(opt, rnd(), "dma_src", 3, cls, 0x188, 0, DMA_R),
		new MthdDmaSurf(opt, rnd(), "dma_surf", 4, cls, 0x18c, 4, cls == 0x88 ? SURF_NV10 : SURF_NV4),
		new MthdSifmXy(opt, rnd(), "xy", 5, cls, 0x300),
		new MthdSifmRect(opt, rnd(), "rect", 6, cls, 0x304),
		new MthdSurfDvdFormat(opt, rnd(), "surf_format", 7, cls, 0x308),
		new MthdSurfOffset(opt, rnd(), "surf_offset", 8, cls, 0x30c, 4, SURF_NV4),
		new MthdSifmDuDx(opt, rnd(), "src.dudx", 9, cls, 0x310, 0),
		new MthdSifmDvDy(opt, rnd(), "src.dvdy", 10, cls, 0x314, 0),
		new MthdSifmSrcSize(opt, rnd(), "src.size", 11, cls, 0x318, 0),
		new MthdDvdSrcFormat(opt, rnd(), "src.format", 12, cls, 0x31c, 0),
		new MthdSifmOffset(opt, rnd(), "src.offset", 13, cls, 0x320, 0),
		new MthdSifmSrcPos(opt, rnd(), "src.pos", 14, cls, 0x324, 0, false),
		new MthdSifmDuDx(opt, rnd(), "ov.dudx", 15, cls, 0x328, 1),
		new MthdSifmDvDy(opt, rnd(), "ov.dvdy", 16, cls, 0x32c, 1),
		new MthdSifmSrcSize(opt, rnd(), "ov.size", 17, cls, 0x330, 1),
		new MthdDvdSrcFormat(opt, rnd(), "ov.format", 18, cls, 0x334, 1),
		new MthdSifmOffset(opt, rnd(), "ov.offset", 19, cls, 0x338, 1),
		new MthdSifmSrcPos(opt, rnd(), "ov.pos", 20, cls, 0x33c, 1, true),
	};
	if ((cls & 0xff) == 0x88) {
		res.insert(res.begin(), {
			new MthdSync(opt, rnd(), "sync", -1, cls, 0x108),
		});
	}
	return res;
}

}
}
