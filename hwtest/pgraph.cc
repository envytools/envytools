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
#include "pgraph_class.h"
#include "nva.h"

namespace {

using namespace hwtest::pgraph;

class PGraphClassTests : public hwtest::Test {
public:
	bool supported() override {
		return chipset.card_type < 0x20;
	}
	std::vector<Class *> classes() {
		if (chipset.card_type < 3) {
			return {
				new Beta(opt, rnd(), 0x01, "beta"),
				new Rop(opt, rnd(), 0x02, "rop"),
				new Chroma(opt, rnd(), 0x03, "chroma"),
				new Plane(opt, rnd(), 0x04, "plane"),
				new Clip(opt, rnd(), 0x05, "clip"),
				new Pattern(opt, rnd(), 0x06, "pattern"),
			};
		} else if (chipset.card_type < 4) {
			return {
				new Beta(opt, rnd(), 0x01, "beta"),
				new Rop(opt, rnd(), 0x02, "rop"),
				new Chroma(opt, rnd(), 0x03, "chroma"),
				new Plane(opt, rnd(), 0x04, "plane"),
				new Clip(opt, rnd(), 0x05, "clip"),
				new Pattern(opt, rnd(), 0x06, "pattern"),
				new M2mf(opt, rnd(), 0x0d, "m2mf"),
				new D3D0(opt, rnd(), 0x17, "d3d0"),
				new ZPoint(opt, rnd(), 0x18, "zpoint"),
				new Surf(opt, rnd(), 0x1c, "surf"),
			};
		} else {
			std::vector<Class *> res = {
				new Beta(opt, rnd(), 0x12, "beta"),
				new Beta4(opt, rnd(), 0x72, "beta4"),
				new Rop(opt, rnd(), 0x43, "rop"),
				new Chroma(opt, rnd(), 0x17, "chroma_nv1"),
				new Chroma(opt, rnd(), 0x57, "chroma_nv4"),
				new Pattern(opt, rnd(), 0x18, "pattern_nv1"),
				new CPattern(opt, rnd(), 0x44, "pattern_nv4"),
				new Clip(opt, rnd(), 0x19, "clip"),
				new M2mf(opt, rnd(), 0x39, "m2mf"),
				new Surf(opt, rnd(), 0x58, "surf_dst"),
				new Surf(opt, rnd(), 0x59, "surf_src"),
				new Surf(opt, rnd(), 0x5a, "surf_color"),
				new Surf(opt, rnd(), 0x5b, "surf_zeta"),
			};
			if (chipset.chipset < 5) {
				res.insert(res.end(), {
					new OpClip(opt, rnd(), 0x10, "op_clip"),
					new OpBlendAnd(opt, rnd(), 0x11, "op_blend_and"),
					new OpRopAnd(opt, rnd(), 0x13, "op_rop_and"),
					new OpChroma(opt, rnd(), 0x15, "op_chroma"),
					new OpSrccopyAnd(opt, rnd(), 0x64, "op_srccopy_and"),
					new OpSrccopy(opt, rnd(), 0x65, "op_srccopy"),
					new OpSrccopyPremult(opt, rnd(), 0x66, "op_srccopy_premult"),
					new OpBlendPremult(opt, rnd(), 0x67, "op_blend_premult"),
				});
			}
			return res;
		}
	}
	Subtests subtests() override {
		Subtests res;
		for (auto *cls : classes()) {
			res.push_back({cls->name, new ClassTest(opt, rnd(), cls)});
		}
		return res;
	}
	using Test::Test;
};

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
	Subtests subtests() override {
		return {
			{"scan", new PGraphScanTests(opt, rnd())},
			{"state", new PGraphStateTests(opt, rnd())},
			{"class", new PGraphClassTests(opt, rnd())},
			{"mthd_misc", new PGraphMthdMiscTests(opt, rnd())},
			{"mthd_simple", new PGraphMthdSimpleTests(opt, rnd())},
			{"mthd_sifm", new PGraphMthdSifmTests(opt, rnd())},
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
