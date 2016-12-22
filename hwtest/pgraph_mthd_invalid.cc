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

class PointMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x08; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x400 && mthd < 0x580)
			return true;
		return false;
	}
public:
	PointMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class LineMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x09; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x400 && mthd < 0x680)
			return true;
		return false;
	}
public:
	LineMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class LinMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x0a; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x400 && mthd < 0x680)
			return true;
		return false;
	}
public:
	LinMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class TriMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x0b; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x310 && mthd < 0x31c)
			return true;
		if (mthd >= 0x320 && mthd < 0x338)
			return true;
		if (mthd >= 0x400 && mthd < 0x600)
			return true;
		return false;
	}
public:
	TriMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class RectMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return chipset.card_type < 3 ? 0x0c : 0x07; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	RectMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class TexLinMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x0d; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x310 && mthd < 0x320)
			return true;
		if (mthd >= 0x350 && mthd < 0x360)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	TexLinMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class TexQuadMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x0e; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x310 && mthd < 0x334)
			return true;
		if (mthd >= 0x350 && mthd < 0x374)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	TexQuadMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class TexLinBetaMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x1d; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x310 && mthd < 0x320)
			return true;
		if (mthd >= 0x350 && mthd < 0x360)
			return true;
		if (mthd >= 0x380 && mthd < 0x388)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	TexLinBetaMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class TexQuadBetaMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x1e; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304)
			return true;
		if (mthd >= 0x310 && mthd < 0x334)
			return true;
		if (mthd >= 0x350 && mthd < 0x374)
			return true;
		if (mthd >= 0x380 && mthd < 0x394)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	TexQuadBetaMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class BlitMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x10; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x300 || mthd == 0x304 || mthd == 0x308)
			return true;
		return false;
	}
public:
	BlitMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class IfcMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x11; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x304 || mthd == 0x308 || mthd == 0x30c)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	IfcMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class BitmapMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x12; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd >= 0x308 && mthd < 0x31c)
			return true;
		if (mthd >= 0x400 && mthd < 0x480)
			return true;
		return false;
	}
public:
	BitmapMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class IfmMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x13; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd >= 0x308 && mthd < 0x318)
			return true;
		if (mthd >= 0x40 && mthd < 0x80)
			return true;
		return false;
	}
public:
	IfmMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class ItmMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x14; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd >= 0x308 && mthd < 0x318)
			return true;
		return false;
	}
public:
	ItmMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class GdiMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x0c; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd >= 0x3fc && mthd < 0x600)
			return true;
		if (mthd >= 0x7f4 && mthd < 0xa00)
			return true;
		if (mthd >= 0xbec && mthd < 0xe00)
			return true;
		if (mthd >= 0xfe8 && mthd < 0x1200)
			return true;
		if (mthd >= 0x13e4 && mthd < 0x1600)
			return true;
		return false;
	}
public:
	GdiMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class SifmMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x0e; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd >= 0x308 && mthd < 0x320)
			return true;
		if (mthd >= 0x400 && mthd < 0x418)
			return true;
		return false;
	}
public:
	SifmMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class SifcMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x15; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd >= 0x304 && mthd < 0x31c)
			return true;
		if (mthd >= 0x400 && mthd < 0x2000)
			return true;
		return false;
	}
public:
	SifcMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
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
	return chipset.card_type < 4;
}

Test::Subtests PGraphMthdInvalidTests::subtests() {
	if (chipset.card_type < 3) {
		return {
			{"point", new PointMthdInvalidTests(opt, rnd())},
			{"line", new LineMthdInvalidTests(opt, rnd())},
			{"lin", new LinMthdInvalidTests(opt, rnd())},
			{"tri", new TriMthdInvalidTests(opt, rnd())},
			{"rect", new RectMthdInvalidTests(opt, rnd())},
			{"texlin", new TexLinMthdInvalidTests(opt, rnd())},
			{"texquad", new TexQuadMthdInvalidTests(opt, rnd())},
			{"texlinbeta", new TexLinBetaMthdInvalidTests(opt, rnd())},
			{"texquadbeta", new TexQuadBetaMthdInvalidTests(opt, rnd())},
			{"blit", new BlitMthdInvalidTests(opt, rnd())},
			{"ifc", new IfcMthdInvalidTests(opt, rnd())},
			{"bitmap", new BitmapMthdInvalidTests(opt, rnd())},
			{"ifm", new IfmMthdInvalidTests(opt, rnd())},
			{"itm", new ItmMthdInvalidTests(opt, rnd())},
		};
	} else {
		return {
			{"invalid00", new InvalidMthdInvalidTests(opt, rnd(), 0x00)},
			{"rect", new RectMthdInvalidTests(opt, rnd())},
			{"point", new PointMthdInvalidTests(opt, rnd())},
			{"line", new LineMthdInvalidTests(opt, rnd())},
			{"lin", new LinMthdInvalidTests(opt, rnd())},
			{"tri", new TriMthdInvalidTests(opt, rnd())},
			{"gdi", new GdiMthdInvalidTests(opt, rnd())},
			{"sifm", new SifmMthdInvalidTests(opt, rnd())},
			{"invalid0f", new InvalidMthdInvalidTests(opt, rnd(), 0x0f)},
			{"blit", new BlitMthdInvalidTests(opt, rnd())},
			{"ifc", new IfcMthdInvalidTests(opt, rnd())},
			{"bitmap", new BitmapMthdInvalidTests(opt, rnd())},
			{"invalid13", new InvalidMthdInvalidTests(opt, rnd(), 0x13)},
			{"itm", new ItmMthdInvalidTests(opt, rnd())},
			{"sifc", new SifcMthdInvalidTests(opt, rnd())},
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
}
