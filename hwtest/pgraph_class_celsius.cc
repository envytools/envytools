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

class MthdWarning : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 3;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.notify, 24, 1, !!(val & 3));
			insrt(exp.notify, 25, 1, (val & 3) == 2);
			insrt(exp.notify, 28, 3, 0);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdState : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	void emulate_mthd() override {
		insrt(exp.intr, 4, 1, 1);
		exp.fifo_enable = 0;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdDmaVtx : public SingleMthdTest {
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t rval = val & 0xffff;
		int dcls = extr(pobj[0], 0, 12);
		if (dcls == 0x30)
			rval = 0;
		bool bad = false;
		if (dcls != 0x30 && dcls != 0x3d && dcls != 2)
			bad = true;
		if (bad && extr(exp.debug[3], 23, 1))
			nv04_pgraph_blowup(&exp, 2);
		bool prot_err = false;
		if (dcls != 0x30) {
			if (extr(pobj[1], 0, 8) != 0xff)
				prot_err = true;
			if (extr(pobj[0], 20, 8))
				prot_err = true;
			if (!extr(pobj[0], 12, 2)) {
				exp.intr |= 0x400;
				exp.fifo_enable = 0;
			}
		}
		if (dcls == 0x30 && nv04_pgraph_is_nv15p(&chipset)) {
			exp.intr |= 0x400;
			exp.fifo_enable = 0;
		}
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		insrt(exp.celsius_dma, 0, 16, rval);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdDmaState : public SingleMthdTest {
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t rval = val & 0xffff;
		int dcls = extr(pobj[0], 0, 12);
		if (dcls == 0x30)
			rval = 0;
		bool bad = false;
		if (dcls != 0x30 && dcls != 0x3d && dcls != 3)
			bad = true;
		if (bad && extr(exp.debug[3], 23, 1))
			nv04_pgraph_blowup(&exp, 2);
		insrt(exp.celsius_dma, 16, 16, rval);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdDmaClipid : public SingleMthdTest {
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t offset_mask = pgraph_offset_mask(&chipset) | 0xf;
		uint32_t base = (pobj[2] & ~0xfff) | extr(pobj[0], 20, 12);
		uint32_t limit = pobj[1];
		uint32_t dcls = extr(pobj[0], 0, 12);
		exp.celsius_surf_limit_clipid = (limit & offset_mask) | (dcls != 0x30) << 31;
		exp.celsius_surf_base_clipid = base & offset_mask;
		bool bad = true;
		if (dcls == 0x30 || dcls == 0x3d)
			bad = false;
		if (dcls == 3 && (cls == 0x38 || cls == 0x88))
			bad = false;
		if (extr(exp.debug[3], 23, 1) && bad) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		bool prot_err = false;
		if (extr(base, 0, 6))
			prot_err = true;
		if (extr(pobj[0], 16, 2))
			prot_err = true;
		if (dcls == 0x30)
			prot_err = false;
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		insrt(exp.ctx_valid, 28, 1, dcls != 0x30 && !(bad && extr(exp.debug[3], 23, 1)));
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdDmaZcull : public SingleMthdTest {
	bool takes_dma() override { return true; }
	void emulate_mthd() override {
		uint32_t offset_mask = 0x3fffffff;
		uint32_t base = (pobj[2] & ~0xfff) | extr(pobj[0], 20, 12);
		uint32_t limit = pobj[1];
		uint32_t dcls = extr(pobj[0], 0, 12);
		exp.celsius_surf_limit_zcull = (limit & offset_mask) | (dcls != 0x30) << 31;
		exp.celsius_surf_base_zcull = base & offset_mask;
		bool bad = true;
		if (dcls == 0x30 || dcls == 0x3d)
			bad = false;
		if (dcls == 3 && (cls == 0x38 || cls == 0x88))
			bad = false;
		if (extr(exp.debug[3], 23, 1) && bad) {
			nv04_pgraph_blowup(&exp, 0x2);
		}
		bool prot_err = false;
		if (extr(base, 0, 6))
			prot_err = true;
		if (extr(pobj[0], 16, 2))
			prot_err = true;
		if (dcls == 0x30)
			prot_err = false;
		if (prot_err)
			nv04_pgraph_blowup(&exp, 4);
		insrt(exp.ctx_valid, 29, 1, dcls != 0x30 && !(bad && extr(exp.debug[3], 23, 1)));
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurfPitchClipid : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff80;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val = (rnd() & 1) << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0xff80) && !!val;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1))
			exp.celsius_surf_pitch_clipid = val & 0xffff;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurfOffsetClipid : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffffffc0;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val = (rnd() & 1) << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0xffffffc0);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1))
			exp.celsius_surf_offset_clipid = val & 0x07ffffff;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurfPitchZcull : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff80;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val = (rnd() & 1) << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0xff80) && !!val;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1))
			exp.celsius_surf_pitch_zcull = val & 0xffff;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdSurfOffsetZcull : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffffffc0;
			val ^= 1 << (rnd() & 0x1f);
		}
		if (!(rnd() & 3)) {
			val = (rnd() & 1) << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0xffffffc0);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1))
			exp.celsius_surf_offset_zcull = val & 0x3fffffff;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdInvalidateZcull : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 4 || val == 6;
	}
	void emulate_mthd() override {
		// XXX completely broken
		skip = true;
		if (!extr(exp.nsource, 1, 1)) {
			if (val & 1) {
				exp.zcull_unka00[0] = 0;
				exp.zcull_unka00[1] = 0;
			} else {
				insrt(exp.zcull_unka00[0], 0, 13, 0x1000);
				insrt(exp.zcull_unka00[1], 0, 13, 0x1000);
				insrt(exp.zcull_unka00[0], 16, 13, extr(exp.celsius_clear_hv[0], 16, 16) + 1);
				insrt(exp.zcull_unka00[1], 16, 13, extr(exp.celsius_clear_hv[1], 16, 16) + 1);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdClearZeta : public SingleMthdTest {
	bool is_valid_val() override {
		return !extr(exp.celsius_unkf48, 8, 1);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_clear_zeta = val;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdClipidEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 2 && !extr(exp.celsius_unkf48, 8, 1);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.celsius_config_d, 6, 1, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdClipidId : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0xf) && !extr(exp.celsius_unkf48, 8, 1);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_clipid_id = val & 0xf;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdClearHv : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x10001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0x0fff0fff) && !extr(exp.celsius_unkf48, 8, 1) &&
			extr(val, 0, 16) <= extr(val, 16, 16);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_clear_hv[which] = val & 0x0fff0fff;
		}
	}
public:
	MthdClearHv(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdCelsiusUnkd84 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x8000000f;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return !(val & ~0x80000003) && !extr(exp.celsius_unkf48, 8, 1);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.celsius_config_d, 1, 2, val);
			insrt(exp.celsius_config_d, 31, 1, extr(val, 31, 1));
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusTexUnk258 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.celsius_tex_format[idx], 3, 1, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusSurfUnk2b8 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		if (extr(val, 4, 28))
			return false;
		if (extr(val, 0, 2) > 2)
			return false;
		if (extr(val, 2, 2) > 2)
			return false;
		return true;
	}
	void emulate_mthd() override {
		insrt(exp.surf_type, 2, 2, extr(val, 0, 2));
		insrt(exp.surf_type, 6, 2, extr(val, 2, 2));
		insrt(exp.unka10, 29, 1, extr(exp.debug[4], 2, 1) && !!extr(exp.surf_type, 2, 2));
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusSurfUnk2bc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		insrt(exp.surf_type, 31, 1, val);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusUnk3f8 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.celsius_config_a, 31, 1, val);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdCelsiusUnk3fc : public SingleMthdTest {
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.celsius_mthd_unk3fc = val;
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

std::vector<SingleMthdTest *> Celsius::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", 2, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", 1, cls, 0x110),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 3, cls, 0x180),
		new MthdDmaGrobj(opt, rnd(), "dma_tex_a", 4, cls, 0x184, 0, DMA_R | DMA_ALIGN),
		new MthdDmaGrobj(opt, rnd(), "dma_tex_b", 5, cls, 0x188, 1, DMA_R | DMA_ALIGN),
		new MthdDmaVtx(opt, rnd(), "dma_vtx", 6, cls, 0x18c),
		new MthdDmaState(opt, rnd(), "dma_state", 7, cls, 0x190),
		new MthdDmaSurf(opt, rnd(), "dma_surf_color", 8, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", 9, cls, 0x198, 3, SURF_NV10),
		new MthdClipHv(opt, rnd(), "clip_h", 10, cls, 0x200, 2, 0),
		new MthdClipHv(opt, rnd(), "clip_v", 10, cls, 0x204, 2, 1),
		new MthdSurf3DFormat(opt, rnd(), "surf_format", 11, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", 12, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_offset", 13, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", 14, cls, 0x214, 3, SURF_NV10),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x218, 2), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x220, 8), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x240, 6), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x260, 8), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x280, 0xe), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x2c0, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x300, 0x3e), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x400, 0x70), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x600, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x680, 7), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x6a0, 6), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x6c4, 3), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x6e8, 6), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x700, 0x12), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x800, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x804, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x808, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x80c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x810, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x814, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x818, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x81c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x820, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x824, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x828, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x82c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x830, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x834, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x838, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x83c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x840, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x844, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x848, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x84c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x850, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x854, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x858, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x85c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x860, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x864, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x868, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x86c, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x870, 8, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xc00, 3), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xc10, 11), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xc40, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xc80, 7), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xca0, 9), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xcc8, 8), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xcec, 5), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xd00, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx16.begin", -1, cls, 0xdfc), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx16.data", -1, cls, 0xe00, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx32.begin", -1, cls, 0x10fc), // XXX
		new UntestedMthd(opt, rnd(), "draw_idx32.data", -1, cls, 0x1100, 0x40), // XXX
		new UntestedMthd(opt, rnd(), "draw_arrays.begin", -1, cls, 0x13fc), // XXX
		new UntestedMthd(opt, rnd(), "draw_arrays.data", -1, cls, 0x1400, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "draw_inline.begin", -1, cls, 0x17fc), // XXX
		new UntestedMthd(opt, rnd(), "draw_inline.data", -1, cls, 0x1800, 0x200), // XXX
	};
	if (cls != 0x56) {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x114), // XXX
			new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
			new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
			new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
			new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
			new UntestedMthd(opt, rnd(), "flip", -1, cls, 0x130),
			new UntestedMthd(opt, rnd(), "color_logic_op_enable", -1, cls, 0xd40), // XXX
			new UntestedMthd(opt, rnd(), "color_logic_op_op", -1, cls, 0xd44), // XXX
		});
	}
	if (cls == 0x99) {
		res.insert(res.end(), {
			new MthdDmaClipid(opt, rnd(), "dma_clipid", -1, cls, 0x1ac),
			new MthdDmaZcull(opt, rnd(), "dma_zcull", -1, cls, 0x1b0),
			new MthdSurfPitchClipid(opt, rnd(), "surf_pitch_clipid", -1, cls, 0xd54),
			new MthdSurfOffsetClipid(opt, rnd(), "surf_offset_clipid", -1, cls, 0xd58),
			new MthdSurfPitchZcull(opt, rnd(), "surf_pitch_zcull", -1, cls, 0xd5c),
			new MthdSurfOffsetZcull(opt, rnd(), "surf_offset_zcull", -1, cls, 0xd60),
			new MthdInvalidateZcull(opt, rnd(), "invalidate_zcull", -1, cls, 0xd64),
			new MthdClearZeta(opt, rnd(), "clear_zeta", -1, cls, 0xd68),
			new UntestedMthd(opt, rnd(), "clear_zeta_trigger", -1, cls, 0xd6c), // XXX
			new UntestedMthd(opt, rnd(), "clear_clipid_trigger", -1, cls, 0xd70), // XXX
			new MthdClipidEnable(opt, rnd(), "clipid_enable", -1, cls, 0xd74),
			new MthdClipidId(opt, rnd(), "clipid_id", -1, cls, 0xd78),
			new MthdClearHv(opt, rnd(), "clear_h", -1, cls, 0xd7c, 0),
			new MthdClearHv(opt, rnd(), "clear_v", -1, cls, 0xd80, 1),
			new MthdCelsiusUnkd84(opt, rnd(), "unkd84", -1, cls, 0xd84),
			new MthdCelsiusTexUnk258(opt, rnd(), "tex_unk258", -1, cls, 0x258, 2),
			new MthdCelsiusSurfUnk2b8(opt, rnd(), "surf_unk2b8", -1, cls, 0x2b8),
			new MthdCelsiusSurfUnk2bc(opt, rnd(), "surf_unk2bc", -1, cls, 0x2bc),
			new MthdCelsiusUnk3f8(opt, rnd(), "unk3f8", -1, cls, 0x3f8),
			new MthdCelsiusUnk3fc(opt, rnd(), "unk3fc", -1, cls, 0x3fc),
		});
	} else {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "old_unk3f8", -1, cls, 0x3f8),
		});
	}
	return res;
}

}
}
