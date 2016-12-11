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

using namespace hwtest::pgraph;

class PGraphTests : public hwtest::Test {
public:
	bool supported() override {
		return chipset.card_type < 0x50;
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
	std::vector<std::pair<const char *, Test *>> subtests() override {
		return {
			{"scan", new PGraphScanTests(opt, rnd())},
			{"state", new PGraphStateTests(opt, rnd())},
			{"mthd_misc", new PGraphMthdMiscTests(opt, rnd())},
			{"mthd_simple", new PGraphMthdSimpleTests(opt, rnd())},
			{"mthd_surf", new PGraphMthdSurfTests(opt, rnd())},
			{"mthd_m2mf", new PGraphMthdM2mfTests(opt, rnd())},
			{"mthd_sifm", new PGraphMthdSifmTests(opt, rnd())},
			{"mthd_d3d0", new PGraphMthdD3D0Tests(opt, rnd())},
			{"mthd_xy", new PGraphMthdXyTests(opt, rnd())},
			{"mthd_invalid", new PGraphMthdInvalidTests(opt, rnd())},
			{"rop", new PGraphRopTests(opt, rnd())},
		};
	}
	PGraphTests(hwtest::TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

}

hwtest::Test *pgraph_tests(hwtest::TestOptions &opt, uint32_t seed) {
	return new PGraphTests(opt, seed);
}
