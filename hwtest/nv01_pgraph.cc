/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "hwtest.h"
#include "pgraph.h"
#include "nvhw/pgraph.h"
#include "nvhw/chipset.h"
#include "nva.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * TODO:
 *  - cliprects
 *  - point VTX methods
 *  - lin/line VTX methods
 *  - tri VTX methods
 *  - rect VTX methods
 *  - tex* COLOR methods
 *  - ifc/bitmap VTX methods
 *  - ifc COLOR methods
 *  - bitmap MONO methods
 *  - blit VTX methods
 *  - blit pixel ops
 *  - tex* pixel ops
 *  - point rasterization
 *  - line rasterization
 *  - lin rasterization
 *  - tri rasterization
 *  - rect rasterization
 *  - texlin calculations
 *  - texquad calculations
 *  - quad rasterization
 *  - blit/ifc/bitmap rasterization
 *  - itm methods
 *  - ifm methods
 *  - notify
 *  - range interrupt
 *  - interrupt handling
 *  - ifm rasterization
 *  - ifm dma
 *  - itm rasterization
 *  - itm dma
 *  - itm pixel ops
 *  - cleanup & refactor everything
 */

namespace {

using namespace hwtest::pgraph;

class MthdTest : public StateTest {
protected:
	uint32_t cls, mthd, val;
	virtual void choose_mthd() = 0;
	virtual void emulate_mthd() = 0;
	void adjust_orig() override {
		insrt(orig.notify, 16, 1, 0);
		insrt(orig.notify, 20, 1, 0);
	}
	void mutate() override {
		val = rnd();
		choose_mthd();
		nva_wr32(cnum, 0x400000 | cls << 16 | mthd, val);
		emulate_mthd();
	}
	void print_fail() {
		printf("Mthd %02x.%04x <- %08x\n", cls, mthd, val);
	}
	MthdTest(hwtest::TestOptions &opt, uint32_t seed) : StateTest(opt, seed) {}
};

class MthdCtxSwitchTest : public MthdTest {
	void adjust_orig() override {
		// no super call by design
		insrt(orig.notify, 16, 1, 0);
	}
	void choose_mthd() override {
		int classes[20] = {
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
			0x08, 0x09, 0x0a, 0x0b, 0x0c,
			0x0d, 0x0e, 0x1d, 0x1e,
			0x10, 0x11, 0x12, 0x13, 0x14,
		};
		cls = classes[rnd() % 20];
		mthd = 0;
	}
	void emulate_mthd() override {
		bool chsw = false;
		int och = extr(exp.ctx_switch[0], 16, 7);
		int nch = extr(val, 16, 7);
		if ((val & 0x007f8000) != (exp.ctx_switch[0] & 0x007f8000))
			chsw = true;
		if (!extr(exp.ctx_control, 16, 1))
			chsw = true;
		bool volatile_reset = extr(val, 31, 1) && extr(exp.debug[2], 28, 1) && (!extr(exp.ctx_control, 16, 1) || och == nch);
		if (chsw) {
			exp.ctx_control |= 0x01010000;
			exp.intr |= 0x10;
			exp.access &= ~0x101;
		} else {
			exp.ctx_control &= ~0x01000000;
		}
		insrt(exp.access, 12, 5, cls);
		insrt(exp.debug[1], 0, 1, volatile_reset);
		if (volatile_reset) {
			pgraph_volatile_reset(&exp);
		}
		exp.ctx_switch[0] = val & 0x807fffff;
		if (exp.notify & 0x100000) {
			exp.intr |= 0x10000001;
			exp.invalid |= 0x10000;
			exp.access &= ~0x101;
			exp.notify &= ~0x100000;
		}
	}
public:
	MthdCtxSwitchTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdNotifyTest : public MthdTest {
	void adjust_orig() override {
		// no super call by design
	}
	void choose_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
		}
		int classes[20] = {
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
			0x08, 0x09, 0x0a, 0x0b, 0x0c,
			0x0d, 0x0e, 0x1d, 0x1e,
			0x10, 0x11, 0x12, 0x13, 0x14,
		};
		cls = classes[rnd() % 20];
		mthd = 0x104;
	}
	void emulate_mthd() override {
		if (val && (cls & 0xf) != 0xd && (cls & 0xf) != 0xe)
			exp.invalid |= 0x10;
		if (exp.notify & 0x100000 && !exp.invalid)
			exp.intr |= 0x10000000;
		if (!(exp.ctx_switch[0] & 0x100))
			exp.invalid |= 0x100;
		if (exp.notify & 0x110000)
			exp.invalid |= 0x1000;
		if (exp.invalid) {
			exp.intr |= 1;
			exp.access &= ~0x101;
		} else {
			exp.notify |= 0x10000;
		}
	}
public:
	MthdNotifyTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdInvalidTest : public MthdTest {
	int repeats() override {
		return 1;
	}
	void choose_mthd() override {
		// set by constructor
	}
	void emulate_mthd() override {
		exp.intr |= 1;
		exp.invalid |= 1;
		exp.access &= ~0x101;
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
		for (uint32_t mthd = 0; mthd < 0x10000; mthd += 4) {
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

class BetaMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x01; }
	bool is_valid(uint32_t mthd) override {
		return mthd == 0x104 || mthd == 0x300;
	}
public:
	BetaMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class RopMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x02; }
	bool is_valid(uint32_t mthd) override {
		return mthd == 0x104 || mthd == 0x300;
	}
public:
	RopMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class ChromaMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x03; }
	bool is_valid(uint32_t mthd) override {
		return mthd == 0x104 || mthd == 0x304;
	}
public:
	ChromaMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class PlaneMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x04; }
	bool is_valid(uint32_t mthd) override {
		return mthd == 0x104 || mthd == 0x304;
	}
public:
	PlaneMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class ClipMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x05; }
	bool is_valid(uint32_t mthd) override {
		return mthd == 0x104 || mthd == 0x300 || mthd == 0x304;
	}
public:
	ClipMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
};

class PatternMthdInvalidTests : public ClsMthdInvalidTests {
	int cls() override { return 0x06; }
	bool is_valid(uint32_t mthd) override {
		if (mthd == 0x104)
			return true;
		if (mthd == 0x308)
			return true;
		if (mthd >= 0x310 && mthd <= 0x31c)
			return true;
		return false;
	}
public:
	PatternMthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : ClsMthdInvalidTests(opt, seed) {}
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
	int cls() override { return 0x0c; }
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

class MthdInvalidTests : public hwtest::Test {
	Subtests subtests() override {
		return {
			{"beta", new BetaMthdInvalidTests(opt, rnd())},
			{"rop", new RopMthdInvalidTests(opt, rnd())},
			{"chroma", new ChromaMthdInvalidTests(opt, rnd())},
			{"plane", new PlaneMthdInvalidTests(opt, rnd())},
			{"clip", new ClipMthdInvalidTests(opt, rnd())},
			{"pattern", new PatternMthdInvalidTests(opt, rnd())},
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
	}
public:
	MthdInvalidTests(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class MthdBetaTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x01;
		mthd = 0x300;
	}
	void emulate_mthd() override {
		exp.beta = val;
		if (exp.beta & 0x80000000)
			exp.beta = 0;
		exp.beta &= 0x7f800000;
	}
public:
	MthdBetaTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdRopTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x02;
		mthd = 0x300;
	}
	void emulate_mthd() override {
		exp.rop = val & 0xff;
		if (val & ~0xff) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access &= ~0x101;
		}
	}
public:
	MthdRopTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdChromaPlaneTest : public MthdTest {
	void choose_mthd() override {
		if (rnd() & 1)
			cls = 0x03;
		else
			cls = 0x04;
		mthd = 0x304;
	}
	void emulate_mthd() override {
		uint32_t color = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
		if (cls == 0x04)
			exp.plane = color;
		else
			exp.chroma = color;
	}
public:
	MthdChromaPlaneTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternShapeTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x06;
		mthd = 0x308;
		if (rnd() & 1)
			val &= 0xf;
	}
	void emulate_mthd() override {
		exp.pattern_config = val & 3;
		if (val > 2) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access &= ~0x101;
		}
	}
public:
	MthdPatternShapeTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternMonoColorTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		cls = 0x06;
		idx = rnd() & 1;
		mthd = 0x310 + idx * 4;
	}
	void emulate_mthd() override {
		struct pgraph_color c = pgraph_expand_color(&exp, val);
		exp.pattern_mono_rgb[idx] = c.r << 20 | c.g << 10 | c.b;
		exp.pattern_mono_a[idx] = c.a;
	}
public:
	MthdPatternMonoColorTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPatternMonoBitmapTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		cls = 0x06;
		idx = rnd() & 1;
		mthd = 0x318 + idx * 4;
	}
	void emulate_mthd() override {
		exp.pattern_mono_bitmap[idx] = pgraph_expand_mono(&exp, val);
	}
public:
	MthdPatternMonoBitmapTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSolidColorTest : public MthdTest {
	void choose_mthd() override {
		switch (rnd() % 5) {
			case 0:
				mthd = 0x304;
				cls = 8 + rnd() % 5;
				break;
			case 1:
				mthd = 0x500 | (rnd()&0x78);
				cls = 8;
				break;
			case 2:
				mthd = 0x600 | (rnd()&0x78);
				cls = 9 + (rnd() & 1);
				break;
			case 3:
				mthd = 0x500 | (rnd()&0x70);
				cls = 0xb;
				break;
			case 4:
				mthd = 0x580 | (rnd()&0x78);
				cls = 0xb;
				break;
			default:
				abort();
		}
	}
	void emulate_mthd() override {
		exp.misc32[0] = val;
	}
public:
	MthdSolidColorTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdBitmapColorTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		idx = rnd() & 1;
		cls = 0x12;
		mthd = 0x308 + idx * 4;
	}
	void emulate_mthd() override {
		exp.bitmap_color[idx] = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
	}
public:
	MthdBitmapColorTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdSubdivideTest : public MthdTest {
	void choose_mthd() override {
		bool beta = rnd() & 1;
		bool quad = rnd() & 1;
		cls = 0xd + beta * 0x10 + quad;
		mthd = 0x304;
		if (rnd() & 1)
			val &= ~0xff00;
	}
	void emulate_mthd() override {
		exp.subdivide = val & 0xffff00ff;
		bool err = false;
		if (val & 0xff00)
			err = true;
		for (int j = 0; j < 8; j++) {
			if (extr(val, 4*j, 4) > 8)
				err = true;
			if (j < 2 && extr(val, 4*j, 4) < 2)
				err = true;
		}
		if (err) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access &= ~0x101;
		}
	}
public:
	MthdSubdivideTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdVtxBetaTest : public MthdTest {
	int idx;
	void choose_mthd() override {
		bool quad = rnd() & 1;
		cls = 0x1d + quad;
		idx = rnd() % (quad ? 5 : 2);
		mthd = 0x380 + idx * 4;
	}
	void emulate_mthd() override {
		uint32_t rclass = extr(exp.access, 12, 5);
		for (int j = 0; j < 2; j++) {
			int vid = idx * 2 + j;
			if (vid == 9 && (rclass & 0xf) == 0xe)
				break;
			uint32_t beta = extr(val, j*16, 16);
			beta &= 0xff80;
			beta <<= 8;
			beta |= 0x4000;
			if (beta & 1 << 23)
				beta |= 1 << 24;
			exp.vtx_beta[vid] = beta;
		}
		if (rclass == 0x1d || rclass == 0x1e)
			exp.valid[0] |= 1 << (12 + idx);
	}
public:
	MthdVtxBetaTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdClipTest : public MthdTest {
	bool is_size;
	int repeats() override { return 10000; };
	void adjust_orig() override {
		is_size = rnd() & 1;
		if (rnd() & 1) {
			insrt(orig.vtx_x[15], 16, 15, (rnd() & 1) ? 0 : 0x7fff);
			insrt(orig.vtx_y[15], 16, 15, (rnd() & 1) ? 0 : 0x7fff);
		}
		/* XXX: submitting on BLIT causes an actual blit */
		if (is_size && extr(orig.access, 12, 5) == 0x10)
			insrt(orig.access, 12, 5, 0);
		MthdTest::adjust_orig();
	}
	void choose_mthd() override {
		cls = 0x05;
		mthd = 0x300 + is_size * 4;;
	}
	void emulate_mthd() override {
		nv01_pgraph_set_clip(&exp, is_size, val);
	}
public:
	MthdClipTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdVtxTest : public MthdTest {
	bool first, poly, draw, fract;
	int repeats() override { return 10000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		if (rnd() & 1) {
			insrt(orig.access, 12, 5, 8 + rnd() % 4);
		}
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
	}
	void choose_mthd() override {
		first = poly = draw = fract = false;;
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
	}
	void emulate_mthd() override {
		int rcls = extr(exp.access, 12, 5);
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
	}
	bool other_fail() override {
		int rcls = extr(exp.access, 12, 5);
		if (real.status && (rcls == 0x0b || rcls == 0x0c)) {
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
	void adjust_orig() override {
		MthdTest::adjust_orig();
		if (rnd() & 1) {
			insrt(orig.access, 12, 5, 8 + rnd() % 4);
		}
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
	}
public:
	MthdX32Test(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdY32Test : public MthdTest {
	bool poly, draw;
	int repeats() override { return 10000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		if (rnd() & 1) {
			insrt(orig.access, 12, 5, 8 + rnd() % 4);
		}
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
	}
	bool other_fail() override {
		int rcls = extr(exp.access, 12, 5);
		if (real.status && (rcls == 0x0b || rcls == 0x0c || rcls == 0x10)) {
			/* Hung PGRAPH... */
			skip = true;
		}
		return false;
	}
public:
	MthdY32Test(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdIfcSizeInTest : public MthdTest {
	bool is_ifm;
	int repeats() override { return 10000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		if (!(rnd() & 3))
			insrt(orig.access, 12, 5, rnd() & 1 ? 0x12 : 0x14);
	}
	void choose_mthd() override {
		if (!(rnd() & 3))
			val &= 0xff00ff;
		if (!(rnd() & 3))
			val &= 0xffff000f;
		switch (rnd() % 3) {
			case 0:
				cls = 0x11;
				mthd = 0x30c;
				is_ifm = false;
				break;
			case 1:
				cls = 0x12;
				mthd = 0x318;
				is_ifm = false;
				break;
			case 2:
				cls = 0x13;
				mthd = 0x30c;
				is_ifm = true;
				break;
			default:
				abort();
		}
	}
	void emulate_mthd() override {
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
public:
	MthdIfcSizeInTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdIfcSizeOutTest : public MthdTest {
	int repeats() override { return 10000; };
	void choose_mthd() override {
		bool is_bitmap = rnd() & 1;
		cls = 0x11 + is_bitmap;
		mthd = is_bitmap ? 0x314 : 0x308;
	}
	void emulate_mthd() override {
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
public:
	MthdIfcSizeOutTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdPitchTest : public MthdTest {
	void choose_mthd() override {
		cls = 0x13 + (rnd() & 1);
		mthd = 0x310;
	}
	void emulate_mthd() override {
		exp.vtx_x[6] = val;
		exp.valid[0] |= 0x040040;
	}
public:
	MthdPitchTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class MthdRectTest : public MthdTest {
	bool draw;
	int repeats() override { return 10000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
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
	}
	void choose_mthd() override {
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
	}
	void emulate_mthd() override {
		int rcls = exp.access >> 12 & 0x1f;
		if (rcls == 0x14) {
			exp.vtx_x[3] = extr(val, 0, 16);
			exp.vtx_y[3] = extr(val, 16, 16);
			nv01_pgraph_vtx_fixup(&exp, 0, 2, exp.vtx_x[3], 1, 0, 2);
			nv01_pgraph_vtx_fixup(&exp, 1, 2, exp.vtx_y[3], 1, 0, 2);
			exp.valid[0] |= 0x4004;
			insrt(exp.xy_misc_0, 12, 1, 0);
			nv01_pgraph_bump_vtxid(&exp);
		} else if (rcls == 0x10) {
			nv01_pgraph_vtx_fixup(&exp, 0, 2, extr(val, 0, 16), 1, 0, 2);
			nv01_pgraph_vtx_fixup(&exp, 1, 2, extr(val, 16, 16), 1, 0, 2);
			nv01_pgraph_vtx_fixup(&exp, 0, 3, extr(val, 0, 16), 1, 1, 3);
			nv01_pgraph_vtx_fixup(&exp, 1, 3, extr(val, 16, 16), 1, 1, 3);
			nv01_pgraph_bump_vtxid(&exp);
			nv01_pgraph_bump_vtxid(&exp);
			exp.valid[0] |= 0x00c00c;
		} else if (rcls == 0x0c || rcls == 0x11 || rcls == 0x12 || rcls == 0x13) {
			int idx = extr(exp.xy_misc_0, 28, 4);
			nv01_pgraph_vtx_fixup(&exp, 0, idx, extr(val, 0, 16), 1, 0, idx & 3);
			nv01_pgraph_vtx_fixup(&exp, 1, idx, extr(val, 16, 16), 1, 0, idx & 3);
			nv01_pgraph_bump_vtxid(&exp);
			if (idx <= 8)
				exp.valid[0] |= 0x1001 << idx;
		} else {
			nv01_pgraph_vtx_fixup(&exp, 0, 15, extr(val, 0, 16), 1, 15, 1);
			nv01_pgraph_vtx_fixup(&exp, 1, 15, extr(val, 16, 16), 1, 15, 1);
			nv01_pgraph_bump_vtxid(&exp);
			if (rcls >= 0x09 && rcls <= 0x0b) {
				exp.valid[0] |= 0x080080;
			}
		}
		if (draw)
			nv01_pgraph_prep_draw(&exp, false);
		// XXX
		if (draw && (rcls == 0x11 || rcls == 0x12 || rcls == 0x13))
			skip = true;
	}
	bool other_fail() override {
		int rcls = extr(exp.access, 12, 5);
		if (real.status && (rcls == 0x0b || rcls == 0x0c)) {
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
	void adjust_orig() override {
		MthdTest::adjust_orig();
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
						if ((exp.xy_misc_4[0] & 0x30) == 0x30)
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

class RopSolidTest : public MthdTest {
	int x, y;
	uint32_t addr[2], pixel[2], epixel[2], rpixel[2];
	int repeats() override { return 100000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000400;
		/* XXX bits 8-9 affect rendering */
		/* XXX bits 12-19 affect xy_misc_4 clip status */
		orig.xy_misc_1[0] &= 0xfff00cff;
		/* avoid invalid ops */
		orig.ctx_switch[0] &= ~0x001f;
		if (rnd()&1) {
			int ops[] = {
				0x00, 0x0f,
				0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
				0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x17,
			};
			orig.ctx_switch[0] |= ops[rnd() % ARRAY_SIZE(ops)];
		} else {
			/* BLEND needs more testing */
			int ops[] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c };
			orig.ctx_switch[0] |= ops[rnd() % ARRAY_SIZE(ops)];
			/* XXX Y8 blend? */
			orig.pfb_config |= 0x200;
		}
		orig.pattern_config = rnd()%3; /* shape 3 is a rather ugly hole in Karnough map */
		if (rnd()&1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		/* XXX causes interrupts */
		orig.valid[0] &= ~0x11000000;
		insrt(orig.access, 12, 5, 8);
		insrt(orig.pfb_config, 4, 3, 3);
		x = rnd() & 0x3ff;
		y = rnd() & 0xff;
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 0, 16, x);
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 16, 16, y);
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 0, 16, x);
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 16, 16, y);
		if (rnd()&1) {
			/* it's vanishingly rare for the chroma key to match perfectly by random, so boost the odds */
			uint32_t ckey = pgraph_to_a1r10g10b10(pgraph_expand_color(&orig, orig.misc32[0]));
			ckey ^= (rnd() & 1) << 30; /* perturb alpha */
			if (rnd()&1) {
				/* perturb it a bit to check which bits have to match */
				ckey ^= 1 << (rnd() % 30);
			}
			orig.chroma = ckey;
		}
	}
	void choose_mthd() override {
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			addr[i] = nv01_pgraph_pixel_addr(&orig, x, y, i);
			nva_wr32(cnum, 0x1000000+(addr[i]&~3), rnd());
			pixel[i] = nva_rd32(cnum, 0x1000000+(addr[i]&~3)) >> (addr[i] & 3) * 8;
			pixel[i] &= bflmask(nv01_pgraph_cpp(orig.pfb_config)*8);
		}
		cls = 0x08;
		mthd = 0x400;
		val = y << 16 | x;
	}
	void emulate_mthd() override {
		int bfmt = extr(orig.ctx_switch[0], 9, 4);
		int bufmask = (bfmt / 5 + 1) & 3;
		if (!extr(orig.pfb_config, 12, 1))
			bufmask = 1;
		bool cliprect_pass = pgraph_cliprect_pass(&exp, x, y);
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			if (bufmask & (1 << i) && (cliprect_pass || (i == 1 && extr(exp.canvas_config, 4, 1))))
				epixel[i] = nv01_pgraph_solid_rop(&exp, x, y, pixel[i]);
			else
				epixel[i] = pixel[i];
		}
		exp.vtx_x[0] = x;
		exp.vtx_y[0] = y;
		exp.xy_misc_0 &= ~0xf0000000;
		exp.xy_misc_0 |= 0x10000000;
		exp.xy_misc_1[0] &= ~0x03000001;
		exp.xy_misc_1[0] |= 0x01000000;
		nv01_pgraph_set_xym2(&exp, 0, 0, 0, 0, 0, x == 0x400 ? 8 : x ? 0 : 2);
		nv01_pgraph_set_xym2(&exp, 1, 0, 0, 0, 0, y == 0x400 ? 8 : y ? 0 : 2);
		exp.valid[0] &= ~0xffffff;
		if (extr(exp.cliprect_ctrl, 8, 1)) {
			exp.intr |= 1 << 24;
			exp.access &= ~0x101;
		}
		if (extr(exp.canvas_config, 24, 1)) {
			exp.intr |= 1 << 20;
			exp.access &= ~0x101;
		}
		if (extr(exp.xy_misc_4[0], 4, 4) || extr(exp.xy_misc_4[1], 4, 4)) {
			exp.intr |= 1 << 12;
			exp.access &= ~0x101;
		}
		if (exp.intr & 0x01101000) {
			for (int i = 0; i < 2; i++) {
				epixel[i] = pixel[i];
			}
		}
	}
	bool other_fail() override {
		bool res = false;
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			rpixel[i] = nva_rd32(cnum, 0x1000000+(addr[i]&~3)) >> (addr[i] & 3) * 8;
			rpixel[i] &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
			if (rpixel[i] != epixel[i]) {
			
				printf("Difference in PIXEL[%d]: expected %08x real %08x\n", i, epixel[i], rpixel[i]);
				res = true;
			}
		}
		return res;
	}
	void print_fail() override {
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			printf("%08x %08x %08x PIXEL[%d]\n", pixel[i], epixel[i], rpixel[i], i);
		}
		MthdTest::print_fail();
	}
public:
	RopSolidTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

class RopBlitTest : public MthdTest {
	int x, y, sx, sy;
	uint32_t addr[2], saddr[2], pixel[2], epixel[2], rpixel[2], spixel[2];
	int repeats() override { return 100000; };
	void adjust_orig() override {
		MthdTest::adjust_orig();
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000400;
		/* XXX bits 8-9 affect rendering */
		/* XXX bits 12-19 affect xy_misc_4 clip status */
		orig.xy_misc_1[0] &= 0xfff00cff;
		/* avoid invalid ops */
		orig.ctx_switch[0] &= ~0x001f;
		if (rnd()&1) {
			int ops[] = {
				0x00, 0x0f,
				0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
				0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x17,
			};
			orig.ctx_switch[0] |= ops[rnd() % ARRAY_SIZE(ops)];
		} else {
			/* BLEND needs more testing */
			int ops[] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c };
			orig.ctx_switch[0] |= ops[rnd() % ARRAY_SIZE(ops)];
			/* XXX Y8 blend? */
			orig.pfb_config |= 0x200;
		}
		orig.pattern_config = rnd()%3; /* shape 3 is a rather ugly hole in Karnough map */
		if (rnd()&1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		orig.xy_misc_4[0] &= ~0xf000;
		orig.xy_misc_4[1] &= ~0xf000;
		orig.valid[0] &= ~0x11000000;
		orig.valid[0] |= 0xf10f;
		insrt(orig.access, 12, 5, 0x10);
		insrt(orig.pfb_config, 4, 3, 3);
		x = rnd() & 0x1ff;
		y = rnd() & 0xff;
		sx = (rnd() & 0x3ff) + 0x200;
		sy = rnd() & 0xff;
		orig.vtx_x[0] = sx;
		orig.vtx_y[0] = sy;
		orig.vtx_x[1] = x;
		orig.vtx_y[1] = y;
		orig.xy_misc_0 &= ~0xf0000000;
		orig.xy_misc_0 |= 0x20000000;
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 0, 16, x);
		if (rnd()&1)
			insrt(orig.cliprect_min[rnd()&1], 16, 16, y);
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 0, 16, x);
		if (rnd()&1)
			insrt(orig.cliprect_max[rnd()&1], 16, 16, y);
		if (rnd()&1) {
			/* it's vanishingly rare for the chroma key to match perfectly by random, so boost the odds */
			uint32_t ckey = pgraph_to_a1r10g10b10(pgraph_expand_color(&orig, orig.misc32[0]));
			ckey ^= (rnd() & 1) << 30; /* perturb alpha */
			if (rnd()&1) {
				/* perturb it a bit to check which bits have to match */
				ckey ^= 1 << (rnd() % 30);
			}
			orig.chroma = ckey;
		}
	}
	void choose_mthd() override {
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			addr[i] = nv01_pgraph_pixel_addr(&orig, x, y, i);
			saddr[i] = nv01_pgraph_pixel_addr(&orig, sx, sy, i);
			nva_wr32(cnum, 0x1000000+(addr[i]&~3), rnd());
			nva_wr32(cnum, 0x1000000+(saddr[i]&~3), rnd());
			pixel[i] = nva_rd32(cnum, 0x1000000+(addr[i]&~3)) >> (addr[i] & 3) * 8;
			spixel[i] = nva_rd32(cnum, 0x1000000+(saddr[i]&~3)) >> (saddr[i] & 3) * 8;
			pixel[i] &= bflmask(nv01_pgraph_cpp(orig.pfb_config)*8);
			spixel[i] &= bflmask(nv01_pgraph_cpp(orig.pfb_config)*8);
			if (sx >= 0x400)
				spixel[i] = 0;
			if (!pgraph_cliprect_pass(&exp, sx, sy) && (i == 0 || !extr(exp.canvas_config, 4, 1))) {
				spixel[i] = 0;
			}
		}
		if (!extr(exp.pfb_config, 12, 1))
			spixel[1] = spixel[0];
		cls = 0x10;
		mthd = 0x308;
		val = 1 << 16 | 1;
	}
	void emulate_mthd() override {
		int bfmt = extr(exp.ctx_switch[0], 9, 4);
		int bufmask = (bfmt / 5 + 1) & 3;
		if (!extr(exp.pfb_config, 12, 1))
			bufmask = 1;
		bool cliprect_pass = pgraph_cliprect_pass(&exp, x, y);
		struct pgraph_color s = nv01_pgraph_expand_surf(&exp, spixel[extr(exp.ctx_switch[0], 13, 1)]);
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			if (bufmask & (1 << i) && (cliprect_pass || (i == 1 && extr(exp.canvas_config, 4, 1))))
				epixel[i] = nv01_pgraph_rop(&exp, x, y, pixel[i], s);
			else
				epixel[i] = pixel[i];
		}
		nv01_pgraph_vtx_fixup(&exp, 0, 2, 1, 1, 0, 2);
		nv01_pgraph_vtx_fixup(&exp, 1, 2, 1, 1, 0, 2);
		nv01_pgraph_vtx_fixup(&exp, 0, 3, 1, 1, 1, 3);
		nv01_pgraph_vtx_fixup(&exp, 1, 3, 1, 1, 1, 3);
		exp.xy_misc_0 &= ~0xf0000000;
		exp.xy_misc_0 |= 0x00000000;
		exp.valid[0] &= ~0xffffff;
		if (extr(exp.cliprect_ctrl, 8, 1)) {
			exp.intr |= 1 << 24;
			exp.access &= ~0x101;
		}
		if (extr(exp.canvas_config, 24, 1)) {
			exp.intr |= 1 << 20;
			exp.access &= ~0x101;
		}
		if (extr(exp.xy_misc_4[0], 4, 4) || extr(exp.xy_misc_4[1], 4, 4)) {
			exp.intr |= 1 << 12;
			exp.access &= ~0x101;
		}
		if (exp.intr & 0x01101000) {
			for (int i = 0; i < 2; i++) {
				epixel[i] = pixel[i];
			}
		}
	}
	bool other_fail() override {
		bool res = false;
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			rpixel[i] = nva_rd32(cnum, 0x1000000+(addr[i]&~3)) >> (addr[i] & 3) * 8;
			rpixel[i] &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
			if (rpixel[i] != epixel[i]) {
			
				printf("Difference in PIXEL[%d]: expected %08x real %08x\n", i, epixel[i], rpixel[i]);
				res = true;
			}
		}
		return res;
	}
	void print_fail() override {
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			printf("%08x %08x %08x PIXEL[%d]\n", pixel[i], epixel[i], rpixel[i], i);
		}
		for (unsigned i = 0; i < 1 + extr(orig.pfb_config, 12, 1); i++) {
			printf("%08x SPIXEL[%d]\n", spixel[i], i);
		}
		MthdTest::print_fail();
	}
public:
	RopBlitTest(hwtest::TestOptions &opt, uint32_t seed) : MthdTest(opt, seed) {}
};

}

namespace hwtest {
namespace pgraph {

bool PGraphMthdMiscTests::supported() {
	return chipset.card_type < 3;
}

Test::Subtests PGraphMthdMiscTests::subtests() {
	return {
		{"ctx_switch", new MthdCtxSwitchTest(opt, rnd())},
		{"notify", new MthdNotifyTest(opt, rnd())},
		{"invalid", new MthdInvalidTests(opt, rnd())},
	};
}

bool PGraphMthdSimpleTests::supported() {
	return chipset.card_type < 3;
}

Test::Subtests PGraphMthdSimpleTests::subtests() {
	return {
		{"beta", new MthdBetaTest(opt, rnd())},
		{"rop", new MthdRopTest(opt, rnd())},
		{"chroma_plane", new MthdChromaPlaneTest(opt, rnd())},
		{"pattern_shape", new MthdPatternShapeTest(opt, rnd())},
		{"pattern_mono_color", new MthdPatternMonoColorTest(opt, rnd())},
		{"pattern_mono_bitmap", new MthdPatternMonoBitmapTest(opt, rnd())},
		{"solid_color", new MthdSolidColorTest(opt, rnd())},
		{"bitmap_color", new MthdBitmapColorTest(opt, rnd())},
		{"subdivide", new MthdSubdivideTest(opt, rnd())},
		{"vtx_beta", new MthdVtxBetaTest(opt, rnd())},
	};
}

bool PGraphMthdXyTests::supported() {
	return chipset.card_type < 3;
}

Test::Subtests PGraphMthdXyTests::subtests() {
	return {
		{"clip", new MthdClipTest(opt, rnd())},
		{"vtx", new MthdVtxTest(opt, rnd())},
		{"vtx_x32", new MthdX32Test(opt, rnd())},
		{"vtx_y32", new MthdY32Test(opt, rnd())},
		{"ifc_size_in", new MthdIfcSizeInTest(opt, rnd())},
		{"ifc_size_out", new MthdIfcSizeOutTest(opt, rnd())},
		{"pitch", new MthdPitchTest(opt, rnd())},
		{"rect", new MthdRectTest(opt, rnd())},
		{"ifc_data", new MthdIfcDataTest(opt, rnd())},
	};
}

bool PGraphRopTests::supported() {
	return chipset.card_type < 3;
}

Test::Subtests PGraphRopTests::subtests() {
	return {
		{"solid", new RopSolidTest(opt, rnd())},
		{"blit", new RopBlitTest(opt, rnd())},
	};
}

}
}
