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
#include <string.h>

namespace hwtest {
namespace pgraph {

namespace {

class MthdInvalidTest : public MthdTest {
	bool special_notify() override {
		return chipset.card_type == 3;
	}
	int repeats() override {
		return 1;
	}
	void choose_mthd() override {
		// set by constructor
	}
	void emulate_mthd() override {
		if (!extr(exp.invalid, 16, 1)) {
			exp.intr |= 1;
			exp.invalid |= 1;
			if (chipset.card_type < 3) {
				exp.access &= ~0x101;
			} else {
				exp.fifo_enable = 0;
				if (!extr(exp.invalid, 4, 1)) {
					if (extr(exp.notify, 16, 1) && extr(exp.notify, 20, 4)) {
						exp.intr |= 1 << 28;
					}
				}
			}
		}
	}
public:
	MthdInvalidTest(hwtest::TestOptions &opt, uint32_t seed, uint32_t cls, uint32_t mthd) : MthdTest(opt, seed) {
		this->cls = cls;
		this->mthd = mthd;
	}
};

class ClsMthdInvalidTests : public hwtest::Test {
protected:
	virtual int cls() = 0;
	virtual bool is_valid(uint32_t mthd) = 0;
	bool subtests_boring() override {
		return true;
	}
	Subtests subtests() override {
		Subtests res;
		uint32_t mthds = chipset.card_type < 3 ? 0x10000 : 0x2000;
		for (uint32_t mthd = 0; mthd < mthds; mthd += 4) {
			if (mthd != 0 && !is_valid(mthd)) {
				char buf[5];
				snprintf(buf, sizeof buf, "%04x", mthd);
				res.push_back({strdup(buf), new MthdInvalidTest(opt, rnd(), cls(), mthd)});
			}
		}
		return res;
	}
	ClsMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class InvalidMthdInvalidTests : public ClsMthdInvalidTests {
	int cls_;
	int cls() override { return cls_; }
	bool is_valid(uint32_t mthd) override { return false; }
public:
	InvalidMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed, uint32_t cls) : ClsMthdInvalidTests(opt, seed), cls_(cls) {}
};

}

bool PGraphMthdInvalidTests::supported() {
	return chipset.card_type == 3;
}

Test::Subtests PGraphMthdInvalidTests::subtests() {
	return {
		{"invalid00", new InvalidMthdInvalidTests(opt, rnd(), 0x00)},
		{"invalid0f", new InvalidMthdInvalidTests(opt, rnd(), 0x0f)},
		{"invalid13", new InvalidMthdInvalidTests(opt, rnd(), 0x13)},
		{"invalid16", new InvalidMthdInvalidTests(opt, rnd(), 0x16)},
		{"invalid19", new InvalidMthdInvalidTests(opt, rnd(), 0x19)},
		{"invalid1a", new InvalidMthdInvalidTests(opt, rnd(), 0x1a)},
		{"invalid1b", new InvalidMthdInvalidTests(opt, rnd(), 0x1b)},
		{"invalid1d", new InvalidMthdInvalidTests(opt, rnd(), 0x1d)},
		{"invalid1e", new InvalidMthdInvalidTests(opt, rnd(), 0x1e)},
		{"invalid1f", new InvalidMthdInvalidTests(opt, rnd(), 0x1f)},
	};
}

}
}
