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

#ifndef HWTEST_PGRAPH_MTHD_H
#define HWTEST_PGRAPH_MTHD_H

#include "pgraph.h"

namespace hwtest {
namespace pgraph {

class MthdTest : public StateTest {
protected:
	uint32_t cls, mthd, subc, val;
	int trapbit;
	bool sync;
	uint32_t grobj[4], egrobj[4], rgrobj[4], gctx, pobj[4];
	virtual void choose_mthd() = 0;
	virtual void emulate_mthd_pre() {};
	virtual void emulate_mthd() = 0;
	virtual bool special_notify() { return false; }
	virtual bool can_buf_notify() { return false; }
	virtual bool can_warn() { return false; }
	virtual bool is_valid_val() { return true; }
	virtual bool is_valid_mthd() { return true; }
	virtual bool takes_dma() { return false; }
	virtual bool takes_ctxobj() { return false; }
	virtual void adjust_orig_mthd() { }
	virtual int gen_nv3_fmt() { return rnd() & 7; }
	virtual bool fix_alt_cls() { return true; }
	void warn(uint32_t err);
	void adjust_orig() override;
	void mutate() override;
	void print_fail() override;
	bool other_fail() override;
	MthdTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed), trapbit(-1) {}
};

class SingleMthdTest : public MthdTest {
protected:
	int idx;
private:
	void choose_mthd() override {
		cls = s_cls;
		idx = rnd() % s_num;
		mthd = s_mthd + idx * s_stride;
		trapbit = s_trapbit;
	}
public:
	uint32_t s_cls, s_mthd, s_stride, s_num;
	int s_trapbit;
	std::string name;
	SingleMthdTest(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num = 1, uint32_t stride = 4)
	: MthdTest(opt, seed), s_cls(cls), s_mthd(mthd), s_stride(stride), s_num(num), s_trapbit(trapbit), name(name) {}
};

class MthdNop : public SingleMthdTest {
	bool supported() override {
		return chipset.card_type >= 4;
	}
	void adjust_orig_mthd() override;
	void emulate_mthd_pre() override;
	void emulate_mthd() override;
	bool is_valid_val() override;
	using SingleMthdTest::SingleMthdTest;
};

class MthdSync : public SingleMthdTest {
	void emulate_mthd() override {}
	bool is_valid_mthd() override { return !sync; }
	using SingleMthdTest::SingleMthdTest;
};

class MthdMissing : public SingleMthdTest {
	bool supported() override {
		return chipset.card_type >= 4 && chipset.card_type < 0x20;
	}
	void emulate_mthd() override {
		insrt(exp.intr, 4, 1, 1);
		exp.fifo_enable = 0;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdPmTrigger : public SingleMthdTest {
	bool supported() override {
		return chipset.card_type >= 0x10;
	}
	void emulate_mthd() override {
		if (!extr(exp.debug[3], 15, 1)) {
			nv04_pgraph_blowup(&exp, 0x40);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdNotify : public SingleMthdTest {
	bool special_notify() override { return true; }
	void adjust_orig_mthd() override;
	bool is_valid_val() override;
	bool is_valid_mthd() override;
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

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
		if (!extr(exp.ctx_switch[1], 16, 16)) {
			pgraph_state_error(&exp);
		}
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

class MthdDmaNotify : public SingleMthdTest {
	bool supported() override {
		return chipset.card_type >= 4;
	}
	bool takes_dma() override { return true; }
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

enum {
	DMA_R = 0,
	DMA_W = 1,
	DMA_CLR = 2,
	DMA_ALIGN = 4,
	DMA_CHECK_PREV = 8,
};

class MthdDmaGrobj : public SingleMthdTest {
	int which, ecls;
	bool clr, align, check_prev;
	bool supported() override {
		return chipset.card_type >= 4;
	}
	bool takes_dma() override { return true; }
	void emulate_mthd() override;
public:
	MthdDmaGrobj(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which, int flags)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {
		if (flags & DMA_W)
			ecls = 3;
		else
			ecls = 2;
		clr = !!(flags & DMA_CLR);
		align = !!(flags & DMA_ALIGN);
		check_prev = !!(flags & DMA_CHECK_PREV);
	}
};

class UntestedMthd : public SingleMthdTest {
	int run() override {
		return HWTEST_RES_UNPREP;
	}
	void emulate_mthd() override { }
	using SingleMthdTest::SingleMthdTest;
};

class MthdSolidColor : public SingleMthdTest {
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

class MthdBitmapColor0 : public SingleMthdTest {
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

class MthdBitmapColor1 : public SingleMthdTest {
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

class MthdBitmapFormat : public SingleMthdTest {
	bool supported() override;
	bool is_valid_val() override;
	void adjust_orig_mthd() override;
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

enum {
	SURF_NV3 = 0,
	SURF_NV4 = 1,
	SURF_NV10 = 2,
};

class MthdDmaSurf : public SingleMthdTest {
	int which;
	int kind;
	bool supported() override {
		return chipset.card_type >= 4;
	}
	bool takes_dma() override { return true; }
	void emulate_mthd() override;
public:
	MthdDmaSurf(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which, int flags)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which), kind(flags) {}
};

class MthdSurfOffset : public SingleMthdTest {
	int which;
	int kind;
	bool is_valid_val() override;
	void emulate_mthd() override;
public:
	MthdSurfOffset(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which, int flags)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which), kind(flags) {}
};

class MthdSurfPitch2 : public SingleMthdTest {
	int which_a, which_b;
	int kind;
	bool is_valid_val() override;
	void emulate_mthd() override;
public:
	MthdSurfPitch2(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which_a, int which_b, int flags)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which_a(which_a), which_b(which_b), kind(flags) {}
};

class MthdSurf3DFormat : public SingleMthdTest {
	bool is_celsius;
	bool is_valid_val() override;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x0f0f373f;
			if (rnd() & 3)
				insrt(val, 8, 4, rnd() & 1 ? 2 : 1);
			if (rnd() & 3)
				insrt(val, 12, 4, 0);
			if (rnd() & 3)
				insrt(val, 16, 4, rnd() % 0xd);
			if (rnd() & 3)
				insrt(val, 20, 4, rnd() % 0xd);
			if (rnd() & 1)
				val ^= 1 << (rnd() & 0x1f);
		}
	}
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
public:
	MthdSurf3DFormat(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, bool is_celsius)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), is_celsius(is_celsius) {}
};

class MthdPatch : public SingleMthdTest {
	bool supported() override { return chipset.card_type == 4; }
	bool is_valid_val() override {
		return val < 2 || chipset.chipset >= 5;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

class MthdOperation : public SingleMthdTest {
	bool is_new;
	bool supported() override { return chipset.chipset >= 5; }
	bool is_valid_val() override {
		return val < (is_new ? 6 : 3);
	}
	bool is_valid_mthd() override {
		return extr(exp.debug[3], 30, 1);
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	void emulate_mthd() override;
public:
	MthdOperation(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, bool is_new)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), is_new(is_new) {}
};

class MthdDither : public SingleMthdTest {
	bool supported() override { return chipset.chipset >= 0x10; }
	bool is_valid_val() override {
		return val < 3;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

class MthdCtxChroma : public SingleMthdTest {
	bool is_new;
	bool supported() override {
		return chipset.chipset >= 5;
	}
	bool is_valid_mthd() override {
		return extr(exp.debug[3], 29, 1);
	}
	bool takes_ctxobj() override { return true; }
	void emulate_mthd() override;
public:
	MthdCtxChroma(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, bool is_new)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), is_new(is_new) {}
};

class MthdCtxClip : public SingleMthdTest {
	bool supported() override {
		return chipset.chipset >= 5;
	}
	bool is_valid_mthd() override {
		return extr(exp.debug[3], 29, 1);
	}
	bool takes_ctxobj() override { return true; }
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

class MthdCtxPattern : public SingleMthdTest {
	bool is_new;
	bool supported() override {
		return chipset.chipset >= 5;
	}
	bool is_valid_mthd() override {
		return extr(exp.debug[3], 29, 1);
	}
	bool takes_ctxobj() override { return true; }
	void emulate_mthd() override;
public:
	MthdCtxPattern(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, bool is_new)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), is_new(is_new) {}
};

class MthdCtxRop : public SingleMthdTest {
	bool supported() override {
		return chipset.chipset >= 5;
	}
	bool is_valid_mthd() override {
		return extr(exp.debug[3], 29, 1);
	}
	bool takes_ctxobj() override { return true; }
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

class MthdCtxBeta : public SingleMthdTest {
	bool supported() override {
		return chipset.chipset >= 5;
	}
	bool is_valid_mthd() override {
		return extr(exp.debug[3], 29, 1);
	}
	bool takes_ctxobj() override { return true; }
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

class MthdCtxBeta4 : public SingleMthdTest {
	bool supported() override {
		return chipset.chipset >= 5;
	}
	bool is_valid_mthd() override {
		return extr(exp.debug[3], 29, 1);
	}
	bool takes_ctxobj() override { return true; }
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

class MthdCtxSurf : public SingleMthdTest {
	int which;
	bool supported() override {
		return chipset.chipset >= 5;
	}
	bool is_valid_mthd() override {
		return extr(exp.debug[3], 29, 1);
	}
	bool takes_ctxobj() override { return true; }
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
public:
	MthdCtxSurf(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

enum {
	SURF2D_NV4 = 0,
	SURF2D_NV10 = 1,
	SURF2D_SWZOK = 2,
};

class MthdCtxSurf2D : public SingleMthdTest {
	bool new_ok;
	bool swz_ok;
	bool supported() override {
		return chipset.chipset >= 5;
	}
	bool is_valid_mthd() override {
		if (chipset.card_type >= 0x20 && ((cls & 0xff) == 0x7b || cls == 0x79))
			return true;
		return extr(exp.debug[3], 29, 1);
	}
	bool takes_ctxobj() override { return true; }
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
public:
	MthdCtxSurf2D(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int flags)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd) {
		new_ok = !!(flags & SURF2D_NV10);
		swz_ok = !!(flags & SURF2D_SWZOK);
	}
};

class MthdCtxSurf3D : public SingleMthdTest {
	bool new_ok;
	bool takes_ctxobj() override { return true; }
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
public:
	MthdCtxSurf3D(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int flags)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd) {
		new_ok = !!(flags & SURF2D_NV10);
	}
};

class MthdClipXy : public SingleMthdTest {
	int which_clip;
	bool is_max;
	void adjust_orig_mthd() override {
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
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
public:
	MthdClipXy(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which_clip, bool is_max)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which_clip(which_clip), is_max(is_max) {}
};

class MthdClipRect : public SingleMthdTest {
	int which_clip;
	void adjust_orig_mthd() override {
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
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
public:
	MthdClipRect(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which_clip)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which_clip(which_clip) {}
};

class MthdClipHv : public SingleMthdTest {
	int which_clip;
	int which_hv;
	bool supported() override {
		return chipset.chipset >= 5;
	}
	void adjust_orig_mthd() override {
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
	bool is_valid_mthd() override {
		return chipset.card_type >= 0x10 || extr(exp.debug[3], 25, 1);
	}
	bool is_valid_val() override;
	void emulate_mthd() override;
public:
	MthdClipHv(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which_clip, int which_hv)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which_clip(which_clip), which_hv(which_hv) {}
};

class MthdFlipSet : public SingleMthdTest {
	int which_set;
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return val < 8;
	}
	void emulate_mthd() override;
public:
	MthdFlipSet(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which_set, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which_set(which_set), which(which) {}
};

class MthdFlipBumpWrite : public SingleMthdTest {
	int which_set;
	void emulate_mthd() override;
public:
	MthdFlipBumpWrite(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which_set)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which_set(which_set) {}
};

enum {
	VTX_FIRST = 1,
	VTX_POLY = 2,
	VTX_DRAW = 4,
	VTX_FRACT = 8,
	VTX_NOCLIP = 0x10,
	VTX_IFC = 0x20,
	VTX_CHECK = 0x40,
	VTX_TFC = 0x80,
};

class MthdVtxXy : public SingleMthdTest {
	bool first, poly, draw, fract, noclip, ifc, check, tfc;
	bool is_valid_val() override {
		if (tfc && val & 7)
			return false;
		if (!check)
			return true;
		if (chipset.card_type < 4)
			return true;
		return !(val & 0x80008000);
	}
	void adjust_orig_mthd() override;
	void emulate_mthd() override;
	bool other_fail() override;
public:
	MthdVtxXy(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, uint32_t stride, int flags)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num, stride) {
		first = !!(flags & VTX_FIRST);
		poly = !!(flags & VTX_POLY);
		draw = !!(flags & VTX_DRAW);
		fract = !!(flags & VTX_FRACT);
		noclip = !!(flags & VTX_NOCLIP);
		ifc = !!(flags & VTX_IFC);
		check = !!(flags & VTX_CHECK);
		tfc = !!(flags & VTX_TFC);
	}
};

class MthdVtxX32 : public SingleMthdTest {
	bool first, poly;
	void adjust_orig_mthd() override;
	void emulate_mthd() override;
public:
	MthdVtxX32(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, uint32_t stride, int flags)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num, stride) {
		first = !!(flags & VTX_FIRST);
		poly = !!(flags & VTX_POLY);
	}
};

class MthdVtxY32 : public SingleMthdTest {
	bool poly, draw;
	void adjust_orig_mthd() override;
	void emulate_mthd() override;
	bool other_fail() override;
public:
	MthdVtxY32(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, uint32_t stride, int flags)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num, stride) {
		poly = !!(flags & VTX_POLY);
		draw = !!(flags & VTX_DRAW);
	}
};

class MthdRect : public SingleMthdTest {
	bool draw, noclip;
	void adjust_orig_mthd() override;
	void emulate_mthd() override;
	bool other_fail() override;
public:
	MthdRect(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, uint32_t stride, int flags)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num, stride) {
		draw = !!(flags & VTX_DRAW);
		noclip = !!(flags & VTX_NOCLIP);
	}
};

enum {
	IFC_IFM = 1,
	IFC_IN = 2,
	IFC_OUT = 4,
	IFC_TFC = 8,
	IFC_BITMAP = 0x10,
};

class MthdIfcSize : public SingleMthdTest {
	bool is_ifm, is_in, is_out, is_tfc, is_bitmap;
	void adjust_orig_mthd() override {
		if (chipset.card_type < 3) {
			if (!(rnd() & 3))
				insrt(orig.access, 12, 5, rnd() & 1 ? 0x12 : 0x14);
		}
		if (!(rnd() & 3))
			val &= 0xff00ff;
		if (!(rnd() & 3))
			val &= 0xffff000f;
	}
	void emulate_mthd() override;
	bool is_valid_val() override {
		return !is_tfc || !(val & 7);
	}
public:
	MthdIfcSize(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int flags)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd) {
		is_ifm = !!(flags & IFC_IFM);
		is_in = !!(flags & IFC_IN);
		is_out = !!(flags & IFC_OUT);
		is_tfc = !!(flags & IFC_TFC);
		is_bitmap = !!(flags & IFC_BITMAP);
	}
};

class MthdBitmapData : public SingleMthdTest {
	bool twocolor;
	void adjust_orig_mthd() override;
	void emulate_mthd_pre() override;
	void emulate_mthd() override;
public:
	MthdBitmapData(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, uint32_t stride, bool twocolor)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num, stride), twocolor(twocolor) {}
};

}
}

#endif
