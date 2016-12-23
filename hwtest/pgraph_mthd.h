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
	uint32_t grobj[4], egrobj[4], rgrobj[4], gctx;
	virtual void choose_mthd() = 0;
	virtual void emulate_mthd_pre() {};
	virtual void emulate_mthd() = 0;
	virtual bool special_notify() { return false; }
	virtual bool is_valid_val() { return true; }
	virtual bool is_valid_mthd() { return true; }
	virtual void adjust_orig_mthd() { }
	virtual int gen_nv3_fmt() { return rnd() & 7; }
	virtual bool fix_alt_cls() { return true; }
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
	void emulate_mthd() override {
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdMissing : public SingleMthdTest {
	bool supported() override {
		return chipset.card_type >= 4;
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
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

class MthdDmaNotify : public SingleMthdTest {
	bool supported() override {
		return chipset.card_type >= 4;
	}
	int run() override {
		return HWTEST_RES_NA;
	}
	void emulate_mthd() override {
		// XXX
	}
	using SingleMthdTest::SingleMthdTest;
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

class MthdPatch : public SingleMthdTest {
	bool is_new;
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

}
}

#endif
