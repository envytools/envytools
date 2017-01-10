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
#include "nva.h"

namespace {

class PFifoScanTest : public hwtest::Test {
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
	int run() override {
		nva_wr32(cnum, 0x3030, 0);
		nva_wr32(cnum, 0x3070, 0);
		nva_wr32(cnum, 0x3230, 0);
		nva_wr32(cnum, 0x3270, 0);
		bitscan(0x2040, 0xff, 0);
		bitscan(0x2140, 0x111, 0);
		bitscan(0x2200, 0x3, 0);
		bitscan(0x2410, 0x3ff8, 0);
		bitscan(0x2420, 0x3ff8, 0);
		bitscan(0x2500, 0x1, 0);
		bitscan(0x3000, 0x1, 0);
		bitscan(0x3010, 0x7f, 0);
		bitscan(0x3030, 0x4, 0);
		bitscan(0x3040, 0x1, 0);
		bitscan(0x3050, 0x100, 0);
		bitscan(0x3070, 0x4, 0);
		bitscan(0x3080, 0x007fffff, 0);
		bitscan(0x3100, 0xfffc, 0);
		bitscan(0x3104, 0xffffffff, 0);
		bitscan(0x3200, 0x1, 0);
		bitscan(0x3210, 0x7f, 0);
		bitscan(0x3230, 0x7c, 0);
		bitscan(0x3240, 0x1, 0);
		bitscan(0x3250, 0x117, 0);
		bitscan(0x3270, 0x7c, 0);
		for (int i = 0; i < 8; i++) {
			bitscan(0x3280 + i * 0x10, 0x017fffff, 0);
		}
		for (int i = 0; i < 0x20; i++) {
			bitscan(0x3300 + i * 8, 0xfffc, 0);
			bitscan(0x3304 + i * 8, 0xffffffff, 0);
		}
		return res;
	}
public:
	PFifoScanTest(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed), res(HWTEST_RES_PASS) {}
};

class PFifoTests : public hwtest::Test {
public:
	bool supported() override {
		return chipset.card_type < 3;
	}
	int run() override {
		if (!(nva_rd32(cnum, 0x200) & 1 << 24)) {
			printf("Mem controller not up.\n");
			return HWTEST_RES_UNPREP;
		}
		nva_wr32(cnum, 0x200, 0xffffeeff);
		nva_wr32(cnum, 0x200, 0xffffffff);
		return HWTEST_RES_PASS;
	}
	Subtests subtests() override {
		return {
			{"scan", new PFifoScanTest(opt, rnd())},
		};
	}
	using Test::Test;
};

}

hwtest::Test *pfifo_tests(hwtest::TestOptions &opt, uint32_t seed) {
	return new PFifoTests(opt, seed);
}
