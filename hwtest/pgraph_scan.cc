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

namespace hwtest {
namespace pgraph {

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
		} else if (chipset.card_type < 4) {
			bitscan(0x400080, 0x13311110, 0);
			bitscan(0x400084, 0x10113301, 0);
			bitscan(0x400088, 0x1133f111, 0);
			bitscan(0x40008c, 0x1173ff31, 0);
		} else if (chipset.card_type < 0x10) {
			bool is_nv5 = chipset.chipset >= 5;
			bitscan(0x400080, 0x1337f000, 0);
			bitscan(0x400084, is_nv5 ? 0xf2ffb701 : 0x72113101, 0);
			bitscan(0x400088, 0x11d7fff1, 0);
			bitscan(0x40008c, is_nv5 ? 0xfbffff73 : 0x11ffff33, 0);
		} else if (chipset.card_type < 0x20) {
			bool is_nv11p = nv04_pgraph_is_nv11p(&chipset);
			bool is_nv15p = nv04_pgraph_is_nv15p(&chipset);
			bool is_nv17p = nv04_pgraph_is_nv17p(&chipset);
			bitscan(0x400080, is_nv17p ? 0x0007ffff : 0x0003ffff, 0);
			bitscan(0x400084, is_nv11p ? 0xfe71f701 : 0xfe11f701, 0);
			bitscan(0x400088, 0xffffffff, 0);
			bitscan(0x40008c, is_nv15p ? 0xffffff78 : 0xfffffc70, is_nv17p ? 0x400 : 0);
			bitscan(0x400090, is_nv17p ? 0x1fffffff : 0x00ffffff, 0);
		} else {
			bool is_nv25p = nv04_pgraph_is_nv25p(&chipset);
			bitscan(0x400080, is_nv25p ? 0x07ffefff : 0x03ffefff, 0);
			bitscan(0x400084, 0x0011f7c1, 0);
			bitscan(0x40008c, is_nv25p ? 0xffffdf7d : 0xffffd77d, 0);
			bitscan(0x400090, is_nv25p ? 0xffffffff : 0xfffff3ff, 0);
			if (!is_nv25p)
				bitscan(0x400094, 0xff, 0);
			bitscan(0x400098, 0xffffffff, 0);
			bitscan(0x40009c, 0xfff, 0);
			if (is_nv25p)
				bitscan(0x4000c0, 3, 0);
		}
		return res;
	}
public:
	ScanDebugTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanControlTest : public ScanTest {
	int run() override {
		if (chipset.card_type < 4) {
			if (chipset.card_type < 3)
				nva_wr32(cnum, 0x4006a4, 0x0f000111);
			bitscan(0x400140, 0x11111111, 0);
			bitscan(0x400144, 0x00011111, 0);
			bitscan(0x400190, 0x11010103, 0);
			if (chipset.card_type < 3) {
				bitscan(0x400180, 0x807fffff, 0);
			} else {
				bitscan(0x400180, 0x3ff3f71f, 0);
				bitscan(0x400194, 0x7f1fe000, 0);
				bitscan(0x4006a4, 0x1, 0);
				nva_wr32(cnum, 0x4006a4, 0);
				for (int i = 0 ; i < 8; i++)
					bitscan(0x4001a0 + i * 4, 0x3ff3f71f, 0);
			}
		} else {
			nva_wr32(cnum, 0x400720, 0);
			bool is_nv5 = chipset.chipset >= 5;
			bool is_nv11p = nv04_pgraph_is_nv11p(&chipset);
			bool is_nv15p = nv04_pgraph_is_nv15p(&chipset);
			bool is_nv17p = nv04_pgraph_is_nv17p(&chipset);
			bool is_nv1720p = is_nv17p || chipset.card_type >= 0x20;
			uint32_t ctxs_mask, ctxc_mask;
			nva_wr32(cnum, 0x400720, 0);
			if (chipset.card_type < 0x10) {
				ctxs_mask = ctxc_mask = is_nv5 ? 0x7f73f0ff : 0x0303f0ff;
				bitscan(0x400104, 0x00007800, 0);
				bitscan(0x400140, 0x00011311, 0);
				bitscan(0x400170, 0x11010103, 0);
				bitscan(0x400174, 0x0f00e000, 0);
				bitscan(0x400160, ctxs_mask, 0);
				bitscan(0x400164, 0xffff3f03, 0);
				bitscan(0x400168, 0xffffffff, 0);
				bitscan(0x40016c, 0x0000ffff, 0);
				nva_wr32(cnum, 0x400750, 0);
				nva_wr32(cnum, 0x400754, 0);
				bitscan(0x400750, 0x00000077, 0);
				bitscan(0x400754, is_nv5 ? 0x003fffff : 0x000fffff, 0);
				bitscan(0x400758, 0xffffffff, 0);
				bitscan(0x400720, 0x1, 0);
				for (int i = 0 ; i < 8; i++) {
					bitscan(0x400180 + i * 4, ctxc_mask, 0);
					bitscan(0x4001a0 + i * 4, 0xffff3f03, 0);
					bitscan(0x4001c0 + i * 4, 0xffffffff, 0);
					bitscan(0x4001e0 + i * 4, 0x0000ffff, 0);
				}
				for (int i = 0 ; i < 4; i++) {
					bitscan(0x400730 + i * 4, 0x00007fff, 0);
					bitscan(0x400740 + i * 4, 0xffffffff, 0);
				}
			} else {
				ctxs_mask = is_nv11p ? 0x7ffff0ff : 0x7fb3f0ff;
				ctxc_mask = is_nv11p ? 0x7ffff0ff : is_nv15p ? 0x7fb3f0ff : 0x7f33f0ff;
				if (nv04_pgraph_is_nv25p(&chipset))
					ctxs_mask = ctxc_mask = 0x7fffffff;
				bitscan(0x400104, 0x07800000, 0);
				if (chipset.card_type < 0x20)
					bitscan(0x400140, 0x01113711, 0);
				else
					bitscan(0x400140, 0x011137d1, 0);
				bitscan(0x400144, 0x11010103, 0);
				if (chipset.card_type < 0x20)
					bitscan(0x400148, is_nv15p ? 0x9f00e000 : 0x1f00e000, 0);
				else
					bitscan(0x400148, 0x9f00ff11, 0);
				bitscan(0x40014c, ctxs_mask, 0);
				bitscan(0x400150, 0xffff3f03, 0);
				bitscan(0x400154, 0xffffffff, 0);
				bitscan(0x400158, 0x0000ffff, 0);
				bitscan(0x40015c, 0xffffffff, 0);
				bitscan(0x40077c, chipset.card_type >= 0x20 ? 0x0100ffff : is_nv15p ? 0x631fffff : 0x7100ffff, 0);
				if (is_nv17p) {
					nva_wr32(cnum, 0x400090, 0);
					bitscan(0x4006b0, 0xffffffff, 0);
					bitscan(0x400838, 0xffffffff, 0);
					bitscan(0x40083c, 0xffffffff, 0);
					bitscan(0x400a00, 0x1fff1fff, 0);
					bitscan(0x400a04, 0x1fff1fff, 0);
					bitscan(0x400a10, 0xdfff3fff, 0);
				}
				nva_wr32(cnum, 0x400760, 0);
				nva_wr32(cnum, 0x400764, 0);
				bitscan(0x400760, is_nv1720p ? 0x000000ff : 0x00000077, 0);
				bitscan(0x400764, is_nv1720p ? 0x7ff71ffc : 0x3ff71ffc, 0);
				bitscan(0x400768, 0xffffffff, 0);
				bitscan(0x40076c, 0xffffffff, 0);
				bitscan(0x400720, 0x1, 0);
				for (int i = 0 ; i < 8; i++) {
					bitscan(0x400160 + i * 4, ctxc_mask, 0);
					bitscan(0x400180 + i * 4, 0xffff3f03, 0);
					bitscan(0x4001a0 + i * 4, 0xffffffff, 0);
					bitscan(0x4001c0 + i * 4, 0x0000ffff, 0);
					bitscan(0x4001e0 + i * 4, 0xffffffff, 0);
				}
				if (!is_nv1720p) {
					for (int i = 0 ; i < 4; i++) {
						bitscan(0x400730 + i * 4, 0x01171ffc, 0);
						bitscan(0x400740 + i * 4, 0xffffffff, 0);
						bitscan(0x400750 + i * 4, 0xffffffff, 0);
					}
				} else {
					for (int i = 0 ; i < 8; i++) {
						bitscan(0x4007a0 + i * 4, 0x00371ffc, 0);
						bitscan(0x4007c0 + i * 4, 0xffffffff, 0);
						bitscan(0x4007e0 + i * 4, 0xffffffff, 0);
					}
				}
				if (!is_nv15p) {
					bitscan(0x400f50, 0x00001fff, 0);
				} else if (chipset.card_type < 0x20) {
					bitscan(0x400f50, 0x00007ffc, 0);
				} else {
					bitscan(0x400f50, 0x0001fffc, 0);
				}
			}
		}
		return res;
	}
public:
	ScanControlTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanCanvasTest : public ScanTest {
	int run() override {
		if (chipset.card_type < 3) {
			nva_wr32(cnum, 0x4006a4, 0x0f000111);
		}
		for (auto &reg : pgraph_canvas_regs(chipset)) {
			if (reg->scan_test(cnum, rnd))
				res = HWTEST_RES_FAIL;
		}
		return res;
	}
public:
	ScanCanvasTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanVtxTest : public ScanTest {
	int run() override {
		int cnt = pgraph_vtx_count(&chipset);
		if (chipset.card_type < 3) {
			nva_wr32(cnum, 0x4006a4, 0x0f000111);
		}
		for (int i = 0 ; i < cnt; i++) {
			bitscan(0x400400 + i * 4, 0xffffffff, 0);
			bitscan(0x400480 + i * 4, 0xffffffff, 0);
		}
		if (chipset.card_type < 3) {
			for (int i = 0 ; i < 14; i++) {
				bitscan(0x400700 + i * 4, 0x01ffffff, 0);
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
			uaddr[2] = 0x400550;
			ustepi = 8;
			ustepxy = 4;
			ucnt = 2;
			if (chipset.card_type == 0x10)
				ucnt = 3;
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
			if (chipset.card_type < 4)
				v0 &= 0x3ffff;
			else
				v0 &= 0xffff;
			v1 &= 0x3ffff;
			if (test_read(uaddr[which] + xy * ustepxy, v0) ||
			    test_read(uaddr[which] + xy * ustepxy + ustepi, v1)) {
				printf("v0 %08x v1 %08x\n", v0, v1);
			}
			nva_wr32(cnum, uaddr[which] + xy * ustepxy + (rnd() & ustepi), v2);
			if (chipset.card_type < 4)
				v1 &= 0x3ffff;
			else
				v1 &= 0xffff;
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
		if (chipset.card_type == 4)
			nva_wr32(cnum, 0x40008c, 0x01000000);
		for (auto &reg : pgraph_rop_regs(chipset)) {
			if (reg->scan_test(cnum, rnd))
				res = HWTEST_RES_FAIL;
		}
		if (chipset.card_type < 4) {
			if (chipset.card_type < 3) {
				bitscan(0x400680, 0x0000ffff, 0);
				bitscan(0x400684, 0x0011ffff, 0);
			} else {
				bitscan(0x400680, 0x0000ffff, 0);
				bitscan(0x400684, 0x00f1ffff, 0);
				bitscan(0x400688, 0x0000ffff, 0);
				bitscan(0x40068c, 0x0001ffff, 0);
			}
		} else {
			if (chipset.card_type < 0x10) {
				bitscan(0x400714, 0x00110101, 0);
			} else {
				bitscan(0x400718, 0x73110101, 0);
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
		}
		for (auto &reg : pgraph_vstate_regs(chipset)) {
			if (reg->scan_test(cnum, rnd))
				res = HWTEST_RES_FAIL;
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

class ScanDmaTest : public ScanTest {
	bool supported() override {
		return chipset.card_type >= 3;
	}
	int run() override {
		if (chipset.card_type < 4) {
			for (auto &reg : pgraph_dma_nv3_regs(chipset)) {
				if (reg->scan_test(cnum, rnd))
					res = HWTEST_RES_FAIL;
			}
		} else {
			for (auto &reg : pgraph_dma_nv4_regs(chipset)) {
				if (reg->scan_test(cnum, rnd))
					res = HWTEST_RES_FAIL;
			}
		}
		return res;
	}
public:
	ScanDmaTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanD3D0Test : public ScanTest {
	bool supported() override {
		return chipset.card_type == 3;
	}
	int run() override {
		for (auto &reg : pgraph_d3d0_regs(chipset)) {
			if (reg->scan_test(cnum, rnd))
				res = HWTEST_RES_FAIL;
		}
		return res;
	}
public:
	ScanD3D0Test(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanD3D56Test : public ScanTest {
	bool supported() override {
		return chipset.card_type == 4;
	}
	int run() override {
		for (auto &reg : pgraph_d3d56_regs(chipset)) {
			if (reg->scan_test(cnum, rnd))
				res = HWTEST_RES_FAIL;
		}
		return res;
	}
public:
	ScanD3D56Test(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

class ScanCelsiusTest : public ScanTest {
	bool supported() override {
		return chipset.card_type == 0x10;
	}
	int run() override {
		for (auto &reg : pgraph_celsius_regs(chipset)) {
			if (reg->scan_test(cnum, rnd))
				res = HWTEST_RES_FAIL;
		}
		return res;
	}
public:
	ScanCelsiusTest(hwtest::TestOptions &opt, uint32_t seed) : ScanTest(opt, seed) {}
};

}

bool PGraphScanTests::supported() {
	return chipset.card_type < 0x30;
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
		{"dma", new ScanDmaTest(opt, rnd())},
		{"d3d0", new ScanD3D0Test(opt, rnd())},
		{"d3d56", new ScanD3D56Test(opt, rnd())},
		{"celsius", new ScanCelsiusTest(opt, rnd())},
	};
}

}
}
