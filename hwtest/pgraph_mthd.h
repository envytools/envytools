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
	uint32_t grobj[4], gctx;
	virtual void choose_mthd() = 0;
	virtual void emulate_mthd_pre() {};
	virtual void emulate_mthd() = 0;
	virtual bool special_notify() { return false; }
	virtual bool is_valid_val() { return true; }
	virtual void adjust_orig_mthd() { }
	virtual int gen_nv3_fmt() { return rnd() & 7; }
	virtual bool fix_alt_cls() { return true; }
	void adjust_orig() override;
	void mutate() override;
	void print_fail() override;
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
		return HWTEST_RES_NA;
	}
	void emulate_mthd() override { }
	using SingleMthdTest::SingleMthdTest;
};

class MthdSolidColor : public SingleMthdTest {
	void emulate_mthd() override;
	using SingleMthdTest::SingleMthdTest;
};

}
}

#endif
