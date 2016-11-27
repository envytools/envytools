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

#include "hwtest.h"
#include "pgraph.h"
#include "nva.h"

namespace {

class ScanTest : public hwtest::Test {
protected:
	int res;
	void bitscan(uint32_t reg, uint32_t all1, uint32_t all0) {
		uint32_t tmp = nva_rd32(cnum, reg);
		nva_wr32(cnum, reg, 0xffffffff);
		uint32_t rall1 = nva_rd32(cnum, reg);
		nva_wr32(cnum, reg, 0);
		uint32_t rall0 = nva_rd32(cnum, reg);
		nva_wr32(cnum, reg, tmp);
		if (rall1 != all1 || rall0 != all0) {
			printf("Bitscan mismatch for %06x: is %08x/%08x, expected %08x/%08x\n", reg, rall1, rall0, all1, all0);
			res = HWTEST_RES_FAIL;
		}
	}
	bool test_read(uint32_t reg, uint32_t exp) {
		uint32_t real = nva_rd32(cnum, reg);
		if (exp != real) {
			printf("Read mismatch for %06x: is %08x, expected %08x\n", reg, real, exp);
			res = HWTEST_RES_FAIL;
			return true;
		}
		return false;
	}
	ScanTest(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed), res(HWTEST_RES_PASS) {}
};

class ScanDebugTest : public ScanTest {
	int run() override {
		if (chipset.card_type < 3) {
			nva_wr32(cnum, 0x4006a4, 0x0f000111);
			bitscan(0x400080, 0x11111110, 0);
			bitscan(0x400084, 0x31111101, 0);
			bitscan(0x400088, 0x11111111, 0);
		} else {
			bitscan(0x400080, 0x13311110, 0);
			bitscan(0x400084, 0x10113301, 0);
			bitscan(0x400088, 0x1133f111, 0);
			bitscan(0x40008c, 0x1173ff31, 0);
		}
		return res;
	}
public:
	ScanDebugTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanControlTest : public ScanTest {
	int run() override {
		if (chipset.card_type < 3)
			nva_wr32(cnum, 0x4006a4, 0x0f000111);
		bitscan(0x400140, 0x11111111, 0);
		bitscan(0x400144, 0x00011111, 0);
		bitscan(0x400190, 0x11010103, 0);
		if (chipset.card_type < 3) {
			bitscan(0x400180, 0x807fffff, 0);
		} else {
			bitscan(0x401140, 0x00011111, 0);
			bitscan(0x400180, 0x3ff3f71f, 0);
			bitscan(0x400194, 0x7f1fe000, 0);
			bitscan(0x4006a4, 0x1, 0);
			nva_wr32(cnum, 0x4006a4, 0);
			for (int i = 0 ; i < 8; i++)
				bitscan(0x4001a0 + i * 4, 0x3ff3f71f, 0);
		}
		return res;
	}
public:
	ScanControlTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanCanvasTest : public ScanTest {
	int run() override {
		uint32_t canvas_mask;
		if (chipset.card_type < 3) {
			nva_wr32(cnum, 0x4006a4, 0x0f000111);
			bitscan(0x400634, 0x01111011, 0);
			nva_wr32(cnum, 0x400688, 0x7fff7fff);
			if(nva_rd32(cnum, 0x400688) != 0x7fff7fff) {
				res = HWTEST_RES_FAIL;
			}
			bitscan(0x400688, 0xffffffff, 0);
			bitscan(0x40068c, 0x0fff0fff, 0);
			canvas_mask = 0x0fff0fff;
		} else {
			canvas_mask = chipset.is_nv03t ? 0x7fff07ff : 0x3fff07ff;
			bitscan(0x400550, canvas_mask, 0);
			bitscan(0x400554, canvas_mask, 0);
			bitscan(0x400558, canvas_mask, 0);
			bitscan(0x40055c, canvas_mask, 0);
		}
		bitscan(0x400690, canvas_mask, 0);
		bitscan(0x400694, canvas_mask, 0);
		bitscan(0x400698, canvas_mask, 0);
		bitscan(0x40069c, canvas_mask, 0);
		bitscan(0x4006a0, 0x113, 0);
		return res;
	}
public:
	ScanCanvasTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanVtxTest : public ScanTest {
	int run() override {
		int cnt;
		if (chipset.card_type < 3) {
			nva_wr32(cnum, 0x4006a4, 0x0f000111);
			cnt = 18;
		} else {
			cnt = 32;
		}
		for (int i = 0 ; i < cnt; i++) {
			bitscan(0x400400 + i * 4, 0xffffffff, 0);
			bitscan(0x400480 + i * 4, 0xffffffff, 0);
		}
		if (chipset.card_type < 3) {
			for (int i = 0 ; i < 14; i++) {
				bitscan(0x400700 + i * 4, 0x01ffffff, 0);
			}
		} else {
			for (int i = 0 ; i < 16; i++) {
				bitscan(0x400580 + i * 4, 0x00ffffff, 0);
			}
		}
		return res;
	}
public:
	ScanVtxTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanClipTest : public ScanTest {
	int run() override {
		uint32_t iaddr, uaddr[3], ustepi, ustepxy;
		int ucnt;
		if (chipset.card_type < 3) {
			nva_wr32(cnum, 0x4006a4, 0x0f000111);
			iaddr = 0x400450;
			uaddr[0] = 0x400460;
			ustepi = 4;
			ustepxy = 8;
			ucnt = 1;
		} else {
			iaddr = 0x400534;
			uaddr[0] = 0x40053c;
			uaddr[1] = 0x400560;
			ustepi = 8;
			ustepxy = 4;
			ucnt = 2;
		}
		bitscan(iaddr + 0x0, 0x0003ffff, 0);
		bitscan(iaddr + 0x4, 0x0003ffff, 0);
		for (int i = 0; i < 1000; i++) {
			int xy = rnd() & 1;
			int which = rnd() % ucnt;
			uint32_t v0 = rnd();
			uint32_t v1 = rnd();
			uint32_t v2 = rnd();
			nva_wr32(cnum, uaddr[which] + xy * ustepxy, v0);
			nva_wr32(cnum, uaddr[which] + xy * ustepxy + ustepi, v1);
			v0 &= 0x3ffff;
			v1 &= 0x3ffff;
			if (test_read(uaddr[which] + xy * ustepxy, v0) ||
			    test_read(uaddr[which] + xy * ustepxy + ustepi, v1)) {
				printf("v0 %08x v1 %08x\n", v0, v1);
			}
			nva_wr32(cnum, uaddr[which] + xy * ustepxy + (rnd() & ustepi), v2);
			v2 &= 0x3ffff;
			if (test_read(uaddr[which] + xy * ustepxy, v1) ||
			    test_read(uaddr[which] + xy * ustepxy + ustepi, v2)) {
				printf("v0 %08x v1 %08x v2 %08x\n", v0, v1, v2);
			}
		}
		return res;
	}
public:
	ScanClipTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanContextTest : public ScanTest {
	int run() override {
		if (chipset.card_type < 3) {
			nva_wr32(cnum, 0x4006a4, 0x0f000111);
		}
		bitscan(0x400600, 0x3fffffff, 0);
		bitscan(0x400604, 0xff, 0);
		bitscan(0x400608, 0x3fffffff, 0);
		bitscan(0x40060c, 0xff, 0);
		bitscan(0x400610, 0xffffffff, 0);
		bitscan(0x400614, 0xffffffff, 0);
		bitscan(0x400618, 3, 0);
		bitscan(0x40061c, 0x7fffffff, 0);
		bitscan(0x400624, 0xff, 0);
		bitscan(0x40062c, 0x7fffffff, 0);
		uint32_t baddr;
		if (chipset.card_type < 3) {
			bitscan(0x400620, 0x7fffffff, 0);
			bitscan(0x400628, 0x7fffffff, 0);
			baddr = 0x400630;
			bitscan(0x400680, 0x0000ffff, 0);
			bitscan(0x400684, 0x0011ffff, 0);
		} else {
			bitscan(0x400680, 0x0000ffff, 0);
			bitscan(0x400684, 0x00f1ffff, 0);
			bitscan(0x400688, 0x0000ffff, 0);
			bitscan(0x40068c, 0x0001ffff, 0);
			baddr = 0x400640;
			uint32_t offset_mask = chipset.is_nv03t ? 0x007ffff0 : 0x003ffff0;
			bitscan(0x400630, offset_mask, 0);
			bitscan(0x400634, offset_mask, 0);
			bitscan(0x400638, offset_mask, 0);
			bitscan(0x40063c, offset_mask, 0);
			bitscan(0x400650, 0x00001ff0, 0);
			bitscan(0x400654, 0x00001ff0, 0);
			bitscan(0x400658, 0x00001ff0, 0);
			bitscan(0x40065c, 0x00001ff0, 0);
			bitscan(0x4006a8, 0x00007777, 0);
		}
		for (int i = 0; i < 1000; i++) {
			uint32_t orig = rnd();
			nva_wr32(cnum, baddr, orig);
			uint32_t exp = orig & 0x7f800000;
			if (orig & 0x80000000)
				exp = 0;
			uint32_t real = nva_rd32(cnum, baddr);
			if (real != exp) {
				printf("BETA scan mismatch: orig %08x expected %08x real %08x\n", orig, exp, real);
				res = HWTEST_RES_FAIL;
				break;
			}
		}
		return res;
	}
public:
	ScanContextTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanVStateTest : public ScanTest {
	int run() override {
		if (chipset.card_type < 3) {
			nva_wr32(cnum, 0x4006a4, 0x0f000111);
			bitscan(0x400640, 0xf1ff11ff, 0);
			bitscan(0x400644, 0x03177331, 0);
			bitscan(0x400648, 0x30ffffff, 0);
			bitscan(0x40064c, 0x30ffffff, 0);
			bitscan(0x400650, 0x111ff1ff, 0);
			bitscan(0x400654, 0xffffffff, 0);
			bitscan(0x400658, 0xffff00ff, 0);
			bitscan(0x40065c, 0xffff0113, 0);
		} else {
			bitscan(0x400500, 0x300000ff, 0);
			bitscan(0x400504, 0x300000ff, 0);
			bitscan(0x400508, 0xffffffff, 0);
			bitscan(0x40050c, 0xffffffff, 0);
			bitscan(0x400510, 0x00ffffff, 0);
			bitscan(0x400514, 0xf013ffff, 0);
			bitscan(0x400518, 0x0f177331, 0);
			bitscan(0x40051c, 0x0f177331, 0);
			bitscan(0x400520, 0x7f7f1111, 0);
			bitscan(0x400524, 0xffffffff, 0);
			bitscan(0x400528, 0xffffffff, 0);
			bitscan(0x40052c, 0xffffffff, 0);
			bitscan(0x400530, 0xffffffff, 0);
			bitscan(0x40054c, 0xffffffff, 0);
			bitscan(0x400570, 0x00ffffff, 0);
		}
		return res;
	}
public:
	ScanVStateTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanAccessTest : public hwtest::Test {
	bool supported() override {
		return chipset.card_type < 3;
	}
	int run() override {
		uint32_t val = nva_rd32(cnum, 0x4006a4);
		int i;
		for (i = 0; i < 1000; i++) {
			uint32_t nv = rnd();
			uint32_t next = val;
			nva_wr32(cnum, 0x4006a4, nv);
			if (nv & 1 << 24)
				insrt(next, 0, 1, extr(nv, 0, 1));
			if (nv & 1 << 25)
				insrt(next, 4, 1, extr(nv, 4, 1));
			if (nv & 1 << 26)
				insrt(next, 8, 1, extr(nv, 8, 1));
			if (nv & 1 << 27)
				insrt(next, 12, 5, extr(nv, 12, 5));
			uint32_t real = nva_rd32(cnum, 0x4006a4);
			if (real != next) {
				printf("ACCESS mismatch: prev %08x write %08x expected %08x real %08x\n", val, nv, next, real);
				return HWTEST_RES_FAIL;
			}
			val = next;
		}
		return HWTEST_RES_PASS;
	}
public:
	ScanAccessTest(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class ScanD3D0Test : public ScanTest {
	bool supported() override {
		return chipset.card_type == 3;
	}
	int run() override {
		bitscan(0x4005c0, 0xffffffff, 0);
		bitscan(0x4005c4, 0xffffffff, 0);
		bitscan(0x4005c8, 0x0000ffff, 0);
		bitscan(0x4005cc, 0xffffffff, 0);
		bitscan(0x4005d0, 0xffffffff, 0);
		bitscan(0x4005d4, 0xffffffff, 0);
		bitscan(0x400644, 0xf77fbdf3, 0);
		bitscan(0x4006c8, 0x00000fff, 0);
		return res;
	}
public:
	ScanD3D0Test(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanDmaTest : public ScanTest {
	bool supported() override {
		return chipset.card_type >= 3;
	}
	int run() override {
		bitscan(0x401200, 0x00000000, 0);
		bitscan(0x401210, 0x03010fff, 0);
		bitscan(0x401220, 0xffffffff, 0);
		bitscan(0x401230, 0xfffff003, 0);
		bitscan(0x401240, 0xfffff000, 0);
		bitscan(0x401250, 0xffffffff, 0);
		bitscan(0x401260, 0xffffffff, 0);
		uint32_t offset_mask = chipset.is_nv03t ? 0x007fffff : 0x003fffff;
		bitscan(0x401270, offset_mask, 0);
		bitscan(0x401280, 0x0000ffff, 0);
		bitscan(0x401290, 0x000007ff, 0);
		bitscan(0x401400, offset_mask, 0);
		bitscan(0x401800, 0xffffffff, 0);
		bitscan(0x401810, 0xffffffff, 0);
		bitscan(0x401820, 0xffffffff, 0);
		bitscan(0x401830, 0xffffffff, 0);
		bitscan(0x401840, 0x00000707, 0);
		return res;
	}
public:
	ScanDmaTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

}

namespace hwtest {
namespace pgraph {

bool PGraphScanTests::supported() {
	return chipset.card_type < 4;
}

Test::Subtests PGraphScanTests::subtests() {
	return {
		{"access", new ScanAccessTest(opt, rnd())},
		{"debug", new ScanDebugTest(opt, rnd())},
		{"control", new ScanControlTest(opt, rnd())},
		{"canvas", new ScanCanvasTest(opt, rnd())},
		{"vtx", new ScanVtxTest(opt, rnd())},
		{"clip", new ScanClipTest(opt, rnd())},
		{"context", new ScanContextTest(opt, rnd())},
		{"vstate", new ScanVStateTest(opt, rnd())},
		{"d3d0", new ScanD3D0Test(opt, rnd())},
		{"dma", new ScanDmaTest(opt, rnd())},
	};
}

}
}
