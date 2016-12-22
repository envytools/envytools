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

std::vector<SingleMthdTest *> OpClip::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 2),
	};
}

std::vector<SingleMthdTest *> OpBlendAnd::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 3),
	};
}

std::vector<SingleMthdTest *> OpRopAnd::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 3),
	};
}

std::vector<SingleMthdTest *> OpChroma::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 2),
	};
}

std::vector<SingleMthdTest *> OpSrccopyAnd::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 2),
	};
}

std::vector<SingleMthdTest *> OpSrccopy::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 2),
	};
}

std::vector<SingleMthdTest *> OpSrccopyPremult::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 3),
	};
}

std::vector<SingleMthdTest *> OpBlendPremult::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 1, cls, 0x180),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 3),
	};
}

}
}
