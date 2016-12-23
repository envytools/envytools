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
#include "pgraph_class.h"
#include "nva.h"

namespace hwtest {
namespace pgraph {

class MthdSubdivide : public SingleMthdTest {
	bool supported() override {
		return chipset.card_type < 3;
	}
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			val &= ~0xff00;
	}
	bool is_valid_val() override {
		bool err = false;
		if (val & 0xff00)
			err = true;
		for (int j = 0; j < 8; j++) {
			if (extr(val, 4*j, 4) > 8)
				err = true;
			if (j < 2 && extr(val, 4*j, 4) < 2)
				err = true;
		}
		return !err;
	}
	void emulate_mthd() override {
		exp.subdivide = val & 0xffff00ff;
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdVtxBeta : public SingleMthdTest {
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
	using SingleMthdTest::SingleMthdTest;
};

std::vector<SingleMthdTest *> TexLin::mthds() {
	return {
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdSubdivide(opt, rnd(), "subdivide", -1, cls, 0x304),
		new MthdVtxXy(opt, rnd(), "xy.0", -1, cls, 0x310, 1, 4, VTX_FIRST),
		new MthdVtxXy(opt, rnd(), "xy.1+", -1, cls, 0x314, 3, 4, 0),
		new MthdVtxXy(opt, rnd(), "xyf.0", -1, cls, 0x350, 1, 4, VTX_FIRST | VTX_FRACT),
		new MthdVtxXy(opt, rnd(), "xyf.1+", -1, cls, 0x354, 3, 4, VTX_FRACT),
		new UntestedMthd(opt, rnd(), "tex_data", -1, cls, 0x400, 0x20), // XXX
	};
}

std::vector<SingleMthdTest *> TexQuad::mthds() {
	return {
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdSubdivide(opt, rnd(), "subdivide", -1, cls, 0x304),
		new MthdVtxXy(opt, rnd(), "xy.0", -1, cls, 0x310, 1, 4, VTX_FIRST),
		new MthdVtxXy(opt, rnd(), "xy.1+", -1, cls, 0x314, 8, 4, 0),
		new MthdVtxXy(opt, rnd(), "xyf.0", -1, cls, 0x350, 1, 4, VTX_FIRST | VTX_FRACT),
		new MthdVtxXy(opt, rnd(), "xyf.1+", -1, cls, 0x354, 8, 4, VTX_FRACT),
		new UntestedMthd(opt, rnd(), "tex_data", -1, cls, 0x400, 0x20), // XXX
	};
}

std::vector<SingleMthdTest *> TexLinBeta::mthds() {
	return {
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdSubdivide(opt, rnd(), "subdivide", -1, cls, 0x304),
		new MthdVtxXy(opt, rnd(), "xy.0", -1, cls, 0x310, 1, 4, VTX_FIRST),
		new MthdVtxXy(opt, rnd(), "xy.1+", -1, cls, 0x314, 3, 4, 0),
		new MthdVtxXy(opt, rnd(), "xyf.0", -1, cls, 0x350, 1, 4, VTX_FIRST | VTX_FRACT),
		new MthdVtxXy(opt, rnd(), "xyf.1+", -1, cls, 0x354, 3, 4, VTX_FRACT),
		new MthdVtxBeta(opt, rnd(), "beta", -1, cls, 0x380, 2),
		new UntestedMthd(opt, rnd(), "tex_data", -1, cls, 0x400, 0x20), // XXX
	};
}

std::vector<SingleMthdTest *> TexQuadBeta::mthds() {
	return {
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdSubdivide(opt, rnd(), "subdivide", -1, cls, 0x304),
		new MthdVtxXy(opt, rnd(), "xy.0", -1, cls, 0x310, 1, 4, VTX_FIRST),
		new MthdVtxXy(opt, rnd(), "xy.1+", -1, cls, 0x314, 8, 4, 0),
		new MthdVtxXy(opt, rnd(), "xyf.0", -1, cls, 0x350, 1, 4, VTX_FIRST | VTX_FRACT),
		new MthdVtxXy(opt, rnd(), "xyf.1+", -1, cls, 0x354, 8, 4, VTX_FRACT),
		new MthdVtxBeta(opt, rnd(), "beta", -1, cls, 0x380, 5),
		new UntestedMthd(opt, rnd(), "tex_data", -1, cls, 0x400, 0x20), // XXX
	};
}

}
}
