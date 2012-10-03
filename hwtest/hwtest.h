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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef HWTEST_H
#define HWTEST_H

#include <stdint.h>

struct hwtest_test {
	int (*fun) (int cnum);
	const char *name;
	int slow;
};

#define HWTEST_TEST(a, slow) { a, #a, slow }

enum hwtest_res {
	HWTEST_RES_NA,
	HWTEST_RES_PASS,
	HWTEST_RES_UNPREP,
	HWTEST_RES_FAIL,
};

#define TEST_BITSCAN(reg, all1, all0) do { \
	uint32_t _reg = reg; \
	uint32_t _all0 = all0; \
	uint32_t _all1 = all1; \
	uint32_t _tmp = nva_rd32(cnum, _reg); \
	nva_wr32(cnum, _reg, 0xffffffff); \
	uint32_t _rall1 = nva_rd32(cnum, _reg); \
	nva_wr32(cnum, _reg, 0); \
	uint32_t _rall0 = nva_rd32(cnum, _reg); \
	nva_wr32(cnum, _reg, _tmp); \
	if (_rall1 != _all1) { \
		fprintf(stderr, "Bitscan mismatch for %06x: is %08x/%08x, expected %08x/%08x\n", _reg, _rall1, _rall0, _all1, _all0); \
		return HWTEST_RES_FAIL; \
	} \
} while (0)

uint32_t vram_rd32(int card, uint64_t addr);
void vram_wr32(int card, uint64_t addr, uint32_t val);

#endif
