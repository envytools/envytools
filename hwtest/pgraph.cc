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
				new Point(opt, rnd(), 0x08, "point"),
				new Line(opt, rnd(), 0x09, "line"),
				new Line(opt, rnd(), 0x0a, "lin"),
				new Tri(opt, rnd(), 0x0b, "tri"),
				new Rect(opt, rnd(), 0x0c, "rect"),
				new TexLin(opt, rnd(), 0x0d, "texlin"),
				new TexQuad(opt, rnd(), 0x0e, "texquad"),
				new Blit(opt, rnd(), 0x10, "blit"),
				new Ifc(opt, rnd(), 0x11, "ifc"),
				new Bitmap(opt, rnd(), 0x12, "bitmap"),
				new Ifm(opt, rnd(), 0x13, "ifm"),
				new Itm(opt, rnd(), 0x14, "itm"),
				new TexLinBeta(opt, rnd(), 0x1d, "texlinbeta"),
				new TexQuadBeta(opt, rnd(), 0x1e, "texquadbeta"),
			};
		} else if (chipset.card_type < 4) {
			return {
				new Beta(opt, rnd(), 0x01, "beta"),
				new Rop(opt, rnd(), 0x02, "rop"),
				new Chroma(opt, rnd(), 0x03, "chroma"),
				new Plane(opt, rnd(), 0x04, "plane"),
				new Clip(opt, rnd(), 0x05, "clip"),
				new Pattern(opt, rnd(), 0x06, "pattern"),
				new Rect(opt, rnd(), 0x07, "rect"),
				new Point(opt, rnd(), 0x08, "point"),
				new Line(opt, rnd(), 0x09, "line"),
				new Line(opt, rnd(), 0x0a, "lin"),
				new Tri(opt, rnd(), 0x0b, "tri"),
				new GdiNv3(opt, rnd(), 0x0c, "gdi"),
				new M2mf(opt, rnd(), 0x0d, "m2mf"),
				new Sifm(opt, rnd(), 0x0e, "sifm"),
				new Blit(opt, rnd(), 0x10, "blit"),
				new Ifc(opt, rnd(), 0x11, "ifc"),
				new Bitmap(opt, rnd(), 0x12, "bitmap"),
				new Itm(opt, rnd(), 0x14, "itm"),
				new Sifc(opt, rnd(), 0x15, "sifc"),
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
				new Surf2D(opt, rnd(), 0x42, "surf2d_nv4"),
				new SurfSwz(opt, rnd(), 0x52, "surfswz_nv4"),
				new Surf3D(opt, rnd(), 0x53, "surf3d_nv4"),
				new Line(opt, rnd(), 0x1c, "lin_nv1"),
				new Tri(opt, rnd(), 0x1d, "tri_nv1"),
				new Rect(opt, rnd(), 0x1e, "rect_nv1"),
				new Line(opt, rnd(), 0x5c, "lin_nv4"),
				new Tri(opt, rnd(), 0x5d, "tri_nv4"),
				new Rect(opt, rnd(), 0x5e, "rect_nv4"),
				new GdiNv3(opt, rnd(), 0x4b, "gdi_nv3"),
				new GdiNv4(opt, rnd(), 0x4a, "gdi_nv4"),
				new Blit(opt, rnd(), 0x1f, "blit_nv1"),
				new Blit(opt, rnd(), 0x5f, "blit_nv4"),
				new Ifc(opt, rnd(), 0x21, "ifc_nv1"),
				new Ifc(opt, rnd(), 0x61, "ifc_nv4"),
				new Sifc(opt, rnd(), 0x36, "sifc_nv3"),
				new Sifc(opt, rnd(), 0x76, "sifc_nv4"),
				new Iifc(opt, rnd(), 0x60, "iifc_nv4"),
				new Sifm(opt, rnd(), 0x37, "sifm_nv3"),
				new Sifm(opt, rnd(), 0x77, "sifm_nv4"),
				new Dvd(opt, rnd(), 0x38, "dvd_nv4"),
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
			} else {
				res.insert(res.end(), {
					new Ifc(opt, rnd(), 0x65, "ifc_nv5"),
					new Sifc(opt, rnd(), 0x66, "sifc_nv5"),
					new Iifc(opt, rnd(), 0x64, "iifc_nv5"),
				});
			}
			if (chipset.card_type == 4) {
				res.insert(res.end(), {
					new EmuD3D0(opt, rnd(), 0x48, "d3d0"),
					new D3D5(opt, rnd(), 0x54, "d3d5"),
					new D3D6(opt, rnd(), 0x55, "d3d6"),
				});
			} else if (chipset.card_type == 0x10) {
				res.insert(res.end(), {
					new EmuD3D5(opt, rnd(), 0x54, "d3d5_nv4"),
					new EmuD3D6(opt, rnd(), 0x55, "d3d6_nv4"),
					new EmuD3D5(opt, rnd(), 0x94, "d3d5_nv10"),
					new EmuD3D6(opt, rnd(), 0x95, "d3d6_nv10"),
					new Celsius(opt, rnd(), 0x56, "celsius_nv10"),
				});
				if (nv04_pgraph_is_nv15p(&chipset)) {
					res.insert(res.end(), {
						new Celsius(opt, rnd(), 0x96, "celsius_nv15"),
					});
				} else {
					res.insert(res.end(), {
						new EmuEmuD3D0(opt, rnd(), 0x48, "d3d0"),
					});
				}
				if (nv04_pgraph_is_nv17p(&chipset)) {
					res.insert(res.end(), {
						new Celsius(opt, rnd(), 0x98, "celsius_nv11"),
						new Celsius(opt, rnd(), 0x99, "celsius_nv17"),
					});
				}
			}
			if (chipset.card_type >= 0x10) {
				res.insert(res.end(), {
					new Ifc(opt, rnd(), 0x8a, "ifc_nv10"),
					new Tfc(opt, rnd(), 0x7b, "tfc_nv10"),
					new Sifm(opt, rnd(), 0x63, "sifm_nv5"),
					new Sifm(opt, rnd(), 0x89, "sifm_nv10"),
					new Dvd(opt, rnd(), 0x88, "dvd_nv10"),
					new Surf2D(opt, rnd(), 0x62, "surf2d_nv10"),
					new Surf3D(opt, rnd(), 0x93, "surf3d_nv10"),
				});
			}
			if (nv04_pgraph_is_nv15p(&chipset)) {
				res.insert(res.end(), {
					new Blit(opt, rnd(), 0x9f, "blit_nv15"),
					new SurfSwz(opt, rnd(), 0x9e, "surfswz_nv15"),
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
