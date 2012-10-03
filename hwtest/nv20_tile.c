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

#include "hwtest.h"
#include "nva.h"
#include "util.h"
#include <stdio.h>
#include <unistd.h>

int get_partbits(int cnum) {
	uint32_t cfg0 = nva_rd32(cnum, 0x100200);
	switch (cfg0 & 0xf) {
		case 0:
			return 0;
		case 1:
			return 1;
		case 3:
			return 2;
		case 7:
			return 3;
		case 0xf:
			return 4;
		default:
			printf("Unknown part bits count!\n");
			return -1;
	}
}

int get_colbits(int cnum) {
	uint32_t cfg1 = nva_rd32(cnum, 0x100204);
	return cfg1 >> 12 & 0xf;
}

int test_scan(int cnum) {
	int i;
	for (i = 0; i < 8; i++) {
		if (nva_cards[cnum].chipset < 0x30) {
			TEST_BITSCAN(0x100240 + i * 0x10, 0xffffc003, 0);
			TEST_BITSCAN(0x100244 + i * 0x10, 0xffffffff, 0x3fff);
			TEST_BITSCAN(0x100248 + i * 0x10, 0x0000ff00, 0);
			if (nva_cards[cnum].chipset == 0x20) {
				TEST_BITSCAN(0x100300 + i * 0x4, 0xbc03ffc0, 0);
			} else {
				TEST_BITSCAN(0x100300 + i * 0x4, 0x01f3ffc0, 0);
			}
		} else {
			TEST_BITSCAN(0x100240 + i * 0x10, 0xffffc031, 0);
			TEST_BITSCAN(0x100244 + i * 0x10, 0xffffffff, 0x3fff);
			TEST_BITSCAN(0x100248 + i * 0x10, 0x0000ff00, 0);
			if (nva_cards[cnum].chipset < 0x35) { /* XXX: confirm on NV30, NV36 */
				TEST_BITSCAN(0x100300 + i * 0x4, 0x1fffffff, 0);
			} else {
				TEST_BITSCAN(0x100300 + i * 0x4, 0x7fffffff, 0);
			}
		}
	}
	if (nva_cards[cnum].chipset == 0x20) {
		TEST_BITSCAN(0x100324, 0x83ffc00f, 0);
	} else {
		TEST_BITSCAN(0x100324, 0x83ffc01f, 0);
	}
	return HWTEST_RES_PASS;
}

uint32_t compute_status(uint32_t pitch) {
	/* like NV10, except pitches 0x300, 0x500, 0x700 are now valid */
	if (pitch & ~0xff00)
		abort();
	if (!pitch)
		return 0xffffffff;
	int shift = 0;
	while (!(pitch & 0x100))
		pitch >>= 1, shift++;
	if (!shift && pitch == 0x100)
		return 0xffffffff;
	int factor;
	switch (pitch) {
		case 0x100:
			factor = 0;
			break;
		case 0x300:
			factor = 1;
			break;
		case 0x500:
			factor = 2;
			break;
		case 0x700:
			factor = 3;
			break;
		default:
			return 0xffffffff;
	}
	return factor | shift << 4;
}

int test_status(int cnum) {
	int i;
	for (i = 0; i < 0x100; i++) {
		uint32_t pitch = i << 8;
		nva_wr32(cnum, 0x100240, 0);
		nva_wr32(cnum, 0x100248, pitch);
		uint32_t exp = compute_status(pitch);
		if (exp == 0xffffffff)
			exp = 0;
		uint32_t real = nva_rd32(cnum, 0x10024c);
		if (exp != real) {
			printf("Tile region status mismatch for pitch %05x: is %08x, expected %08x\n", pitch, real, exp);
			return HWTEST_RES_FAIL;
		}
		nva_wr32(cnum, 0x100240, 1);
		exp = compute_status(pitch);
		if (exp == 0xffffffff)
			exp = 0;
		else
			exp |= 0x80000000;
		real = nva_rd32(cnum, 0x10024c);
		nva_wr32(cnum, 0x100240, 0);
		if (exp != real) {
			printf("Tile region status mismatch for pitch %05x [enabled]: is %08x, expected %08x\n", pitch, real, exp);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

int test_zcomp_size(int cnum) {
	uint32_t expected;
	uint32_t real = nva_rd32(cnum, 0x100320);
	switch (nva_cards[cnum].chipset) {
		case 0x20:
			expected = 0x7fff;
			break;
		case 0x25:
		case 0x28:
			expected = 0xbfff;
			break;
		case 0x30: /* guess */
		case 0x31:
			expected = 0x3ffff;
			break;
		case 0x35:
		case 0x36: /* guess */
			expected = 0x5c7ff;
			break;
		default:
			printf("Don't know expected zcomp size for NV%02X [%08x] - please report!\n", nva_cards[cnum].chipset, real);
			return HWTEST_RES_UNPREP;
	}
	if (real != expected) {
		printf("Zcomp size mismatch: is %08x, expected %08x\n", real, expected);
		return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

void clear_zcomp(int cnum) {
	int partbits = get_partbits(cnum);
	uint32_t size = (nva_rd32(cnum, 0x100320) + 1);
	int i, j, k;
	for (i = 0; i < (1 << partbits); i++) {
		for (j = 0; j < size; j += 0x40) {
			nva_wr32(cnum, 0x1000f0, 0x1300000 | i << 16 | j);
			for (k = 0; k < 0x40; k += 4) {
				nva_wr32(cnum, 0x100100 + k, 0);
			}
		}
	}
}

int test_zcomp_access(int cnum) {
	uint32_t size = (nva_rd32(cnum, 0x100320) + 1) / 8;
	int i, j, k;
	clear_zcomp(cnum);
	for (i = 0; i < 4; i++) {
		for (j = 0; j < size; j += 0x40) {
			nva_wr32(cnum, 0x1000f0, 0x1300000 | i << 16 | j);
			for (k = 0; k < 0x40; k += 4) {
				nva_wr32(cnum, 0x100100 + k, 0xdea00000 | i << 16 | j | k);
			}
		}
	}
	for (i = 0; i < 4; i++) {
		for (j = 0; j < size; j += 0x40) {
			nva_wr32(cnum, 0x1000f0, 0x1300000 | i << 16 | j);
			for (k = 0; k < 0x40; k += 4) {
				uint32_t val = nva_rd32(cnum, 0x100100 + k);
				uint32_t x =  0xdea00000 | i << 16 | j | k;
				if (val != x) {
					printf("MISMATCH %08x %08x\n", x, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

uint32_t translate_addr(uint32_t pitch, uint32_t laddr, int colbits, int partbits, int bankoff) {
	uint32_t status = compute_status(pitch);
	uint32_t shift = status >> 4;
	int ybits = partbits + colbits + 2 - 8;
	uint32_t x = laddr % pitch;
	uint32_t y = laddr / pitch;
	uint32_t x1 = x & 0xf;
	x >>= 4;
	int part = x & ((1 << partbits) - 1);
	x >>= partbits;
	uint32_t x2 = x & ((1 << (4 - partbits)) - 1);
	x >>= 4 - partbits;
	uint32_t y1 = y & 3;
	y >>= 2;
	uint32_t y2 = y & ((1 << (ybits - 2)) - 1);
	y >>= ybits - 2;
	if (y2 & 1 && partbits >= 1)
		part += 1 << (partbits - 1);
	if (y2 & 2 && partbits >= 2)
		part += 1 << (partbits - 2);
	part &= (1 << partbits) - 1;
	uint32_t res = x + y * (pitch / 0x100);
	res ^= bankoff;
	if (y & 1 && shift)
		res ^= 1;
	res <<= ybits - 2, res |= y2;
	res <<= 4 - partbits, res |= x2;
	res <<= 2, res |= y1;
	res <<= partbits, res |= part;
	res <<= 4, res |= x1;
	return res;
}

uint32_t translate_tag(uint32_t pitch, int part, uint32_t tag, int colbits, int partbits) {
	uint32_t lx = tag & ((1 << (4 - partbits)) - 1);
	tag >>= 4 - partbits;
	uint32_t ly = tag & ((1 << (colbits + partbits - 8)) - 1);
	tag >>= colbits + partbits - 8;
	uint32_t tx = tag % (pitch >> 8);
	uint32_t ty = tag / (pitch >> 8);
	uint32_t y = (ty << (colbits + partbits - 8) | ly) << 2;
	uint32_t px = part;
	if (ly & 1 && partbits >= 1)
		px -= 1 << (partbits - 1);
	if (ly & 2 && partbits >= 2)
		px -= 1 << (partbits - 2);
	px &= (1 << partbits) - 1;
	uint32_t x = tx << 8 | lx << (partbits + 4) | px << 4;
	return y * pitch + x;
}

int test_zcomp_layout(int cnum) {
	int partbits = get_partbits(cnum);
	int colbits = get_colbits(cnum);
	uint32_t size = (nva_rd32(cnum, 0x100320) + 1);
	int i, j, m;
	clear_zcomp(cnum);
	nva_wr32(cnum, 0x100240, 0);
	for (i = 0; i < 0x200000; i += 0x10) {
		vram_wr32(cnum, i+0x0, 0x0000ffff);
		vram_wr32(cnum, i+0x4, 0x00000000);
		vram_wr32(cnum, i+0x8, 0x00000000);
		vram_wr32(cnum, i+0xc, 0x00000000);
	}
	uint32_t pitch = 0xe000;
	nva_wr32(cnum, 0x100248, pitch);
	nva_wr32(cnum, 0x100244, 0xfc000);
	nva_wr32(cnum, 0x100240, 1);
	nva_wr32(cnum, 0x100300, 0x90000000);
	for (i = 0; i < (1 << partbits); i++) {
		for (j = 0; j < size; j++) {
			nva_wr32(cnum, 0x1000f0, 0x1300000 | i << 16 | (j >> 3 & ~0x3f));
			nva_wr32(cnum, 0x100100 | (j >> 3 & 0x3c), 1 << (j & 0x1f));
			uint32_t exp = translate_tag(pitch, i, j, colbits, partbits);
			if (exp < 0x100000 && !vram_rd32(cnum, exp + 4)) {
				printf("part %d tag %05x: expected to belong to %08x", i, j, exp);
				int found = 0;
				for (m = 0; m < 0x100000; m += 0x10) {
					uint32_t mm = vram_rd32(cnum, m+4);
					if (mm) {
						printf(", found at %08x instead\n", m);
						break;
					}
				}
				if (!found) {
					printf(", but not found in surface\n");
				}
				nva_wr32(cnum, 0x100240, 0);
				nva_wr32(cnum, 0x100300, 0);
				return HWTEST_RES_FAIL;
			}
			nva_wr32(cnum, 0x100100 | (j >> 3 & 0x3c), 0);
		}
	}
	nva_wr32(cnum, 0x100240, 0);
	nva_wr32(cnum, 0x100300, 0);
	return HWTEST_RES_PASS;
}

int test_format(int cnum) {
	int i, j;
	int partbits = get_partbits(cnum);
	int colbits = get_colbits(cnum);
	if (partbits < 0)
		return HWTEST_RES_UNPREP;
	int res = HWTEST_RES_PASS;
	int bankoff;
	int banktry;
	if (nva_cards[cnum].chipset < 0x30)
		banktry = 2;
	else
		banktry = 4;
	for (i = 0; i < 8; i++)
		nva_wr32(cnum, 0x100240 + i * 0x10, 0);
	for (i = 0; i < 0x200000; i += 4)
		vram_wr32(cnum, i, 0xc0000000 | i);
	for (i = 0; i < 0x100; i++) {
		uint32_t pitch = i << 8;
		nva_wr32(cnum, 0x100300, 0);
		nva_wr32(cnum, 0x100248, pitch);
		nva_wr32(cnum, 0x100244, 0x000fc000);
		if (compute_status(pitch) == 0xffffffff)
			continue;
		for (bankoff = 0; bankoff < banktry; bankoff++) {
			if (nva_cards[cnum].chipset < 0x30)
				nva_wr32(cnum, 0x100240, 1 | bankoff << 1);
			else
				nva_wr32(cnum, 0x100240, 1 | bankoff << 4);
			for (j = 0; j < 0x100000; j += 4) {
				uint32_t real = vram_rd32(cnum, j);
				uint32_t exp = 0xc0000000 | translate_addr(pitch, j, colbits, partbits, bankoff);
				if (real != exp) {
					printf("Mismatch at %08x for pitch %05x bankoff %d: is %08x, expected %08x\n", j, pitch, bankoff, real, exp);
					res = HWTEST_RES_FAIL;
					break;
				}
			}
		}
		for (j = 0x100000; j < 0x180000; j += 4) {
			uint32_t real = vram_rd32(cnum, j);
			uint32_t exp = 0xc0000000 | j;
			if (real != exp) {
				printf("Mismatch at %08x [after limit] for pitch %05x: is %08x, expected %08x\n", j, pitch, real, exp);
				res = HWTEST_RES_FAIL;
				break;
			}
		}
		nva_wr32(cnum, 0x100240, 0x0);
	}
	return res;
}

void zcomp_decompress(int format, uint32_t dst[4][4], uint32_t src[4]) {
	int x, y;
	uint64_t w0 = src[0] | (uint64_t)src[1] << 32;
	uint64_t w1 = src[2] | (uint64_t)src[3] << 32;
	/* short flag */
	if (w0 & (uint64_t)1 << 63)
		w1 = 0;
	if (!(format & 1)) {
		/* Z16 */
		uint16_t tdst[8][4];
		uint32_t delta[8][4];
		uint32_t base = w0 & 0xffff;
		uint32_t dx = w0 >> 16 & 0xfff;
		uint32_t dy = w0 >> 28 & 0xfff;
		if (dx & 0x800)
			dx |= 0xfffff000;
		if (dy & 0x800)
			dy |= 0xfffff000;
		delta[0][0] = w0 >> 40 & 7;
		delta[1][0] = w0 >> 43 & 7;
		delta[2][0] = 0;
		delta[3][0] = w0 >> 46 & 7;
		delta[4][0] = 0;
		delta[5][0] = w0 >> 49 & 7;
		delta[6][0] = w0 >> 52 & 7;
		delta[7][0] = w0 >> 55 & 7;
		delta[0][1] = w0 >> 58 & 7;
		delta[1][1] = (w0 >> 61 & 3) | (w1 & 1) << 2;
		delta[2][1] = w1 >> 1 & 7;
		delta[3][1] = w1 >> 4 & 7;
		delta[4][1] = w1 >> 7 & 7;
		delta[5][1] = w1 >> 10 & 7;
		delta[6][1] = w1 >> 13 & 7;
		delta[7][1] = w1 >> 16 & 7;
		delta[0][2] = w1 >> 19 & 7;
		delta[1][2] = w1 >> 22 & 7;
		delta[2][2] = 0;
		delta[3][2] = w1 >> 25 & 7;
		delta[4][2] = w1 >> 28 & 7;
		delta[5][2] = w1 >> 31 & 7;
		delta[6][2] = w1 >> 34 & 7;
		delta[7][2] = w1 >> 37 & 7;
		delta[0][3] = w1 >> 40 & 7;
		delta[1][3] = w1 >> 43 & 7;
		delta[2][3] = w1 >> 46 & 7;
		delta[3][3] = w1 >> 49 & 7;
		delta[4][3] = w1 >> 52 & 7;
		delta[5][3] = w1 >> 55 & 7;
		delta[6][3] = w1 >> 58 & 7;
		delta[7][3] = w1 >> 61 & 7;
		for (x = 0; x < 8; x++) {
			for (y = 0; y < 4; y++) {
				if (delta[x][y] & 4)
					delta[x][y] |= 0xfff8;
				uint32_t tmp = base * 2 + (x - 2) * dx + y * dy + delta[x][y] * 2;
				tdst[x][y] = (tmp + 1) >> 1;
				if (format & 2) {
					tdst[x][y] = tdst[x][y] << 8 | tdst[x][y] >> 8;
				}
			}
		}
		for (x = 0; x < 4; x++) {
			for (y = 0; y < 4; y++) {
				dst[x][y] = tdst[2*x][y] | tdst[2*x+1][y] << 16;
			}
		}
	} else {
		/* Z24S8 */
		uint32_t delta[4][4];
		uint8_t stencil = w0 & 0xff;
		uint32_t base = w0 >> 8 & 0xffffff;
		uint32_t dx = w0 >> 32 & 0x7fff;
		uint32_t dy = w0 >> 47 & 0x7fff;
		if (dx & 0x4000)
			dx |= 0xff8000;
		if (dy & 0x4000)
			dy |= 0xff8000;
		delta[0][0] = (w0 >> 62 & 1) | (w1 & 0xf) << 1;
		delta[1][0] = w1 >> 4 & 0x1f;
		delta[2][0] = w1 >> 9 & 0x1f;
		delta[3][0] = w1 >> 14 & 0x1f;
		delta[0][1] = w1 >> 19 & 0x1f;
		delta[1][1] = 0;
		delta[2][1] = 0;
		delta[3][1] = w1 >> 24 & 0x1f;
		delta[0][2] = w1 >> 29 & 0x1f;
		delta[1][2] = 0;
		delta[2][2] = w1 >> 34 & 0x1f;
		delta[3][2] = w1 >> 39 & 0x1f;
		delta[0][3] = w1 >> 44 & 0x1f;
		delta[1][3] = w1 >> 49 & 0x1f;
		delta[2][3] = w1 >> 54 & 0x1f;
		delta[3][3] = w1 >> 59 & 0x1f;
		for (x = 0; x < 4; x++) {
			for (y = 0; y < 4; y++) {
				if (delta[x][y] & 0x10)
					delta[x][y] |= 0xffffe0;
				dst[x][y] = base;
				dst[x][y] += (x - 1) * dx;
				dst[x][y] += (y - 1) * dy;
				dst[x][y] += delta[x][y];
				dst[x][y] <<= 8;
				dst[x][y] |= stencil;
				if (format & 2) {
					dst[x][y] = (dst[x][y] & 0xff00ff) << 8 | (dst[x][y] & 0xff00ff00) >> 8;
					dst[x][y] = dst[x][y] << 16 | dst[x][y] >> 16;
				}
			}
		}
	}
}

int test_zcomp_format(int cnum) {
	int res = HWTEST_RES_PASS;
	int colbits = get_colbits(cnum);
	int partbits = get_partbits(cnum);
	uint32_t pitch = 0x200;
	nva_wr32(cnum, 0x100248, pitch);
	nva_wr32(cnum, 0x100244, 0);
	nva_wr32(cnum, 0x100240, 1);
	int i, j;
	for (i = 0; i < 16; i++) {
		nva_wr32(cnum, 0x100300, 0x80000000 | i << 26);
		for (j = 0; j < 100000; j++) {
			int tag = rand() & 0xff;
			int part = tag & ((1 << partbits) - 1);
			tag >>= partbits;
			uint32_t addr = translate_tag(pitch, part, tag, colbits, partbits);
			uint32_t src[4];
			int k;
			nva_wr32(cnum, 0x1000f0, 0x1300000 | part << 16);
			nva_wr32(cnum, 0x100100 | (tag >> 3 & 0x3c), 0);
			for (k = 0; k < 4; k++) {
				src[k] = rand() ^ rand() << 10 ^ rand() << 20;
				vram_wr32(cnum, addr + k * 4, src[k]);
			}
			nva_wr32(cnum, 0x100100 | (tag >> 3 & 0x3c), 1 << (tag & 0x1f));
			uint32_t dst[4][4], rdst[4][4];
			int x, y;
			for (x = 0; x < 4; x++)
				for (y = 0; y < 4; y++)
					rdst[x][y] = vram_rd32(cnum, addr + x * 4 + y * pitch);
			zcomp_decompress(i, dst, src);
			int fail = 0;
			for (x = 0; x < 4; x++)
				for (y = 0; y < 4; y++)
					if (rdst[x][y] != dst[x][y])
						fail = 1;
			if (fail) {
				printf("ZCOMP decompression mismatch iter %d: format %d part %d tag %05x, src %08x %08x %08x %08x\n", j, i, part, tag, src[0], src[1], src[2], src[3]);
				printf("expected:\n");
				for (y = 0; y < 4; y++)
					for (x = 0; x < 4; x++) 
						printf("%08x%c", dst[x][y], x == 3 ? '\n' : ' ');
				printf("real:\n");
				for (y = 0; y < 4; y++)
					for (x = 0; x < 4; x++) 
						printf("%08x%c", rdst[x][y], x == 3 ? '\n' : ' ');
			for (x = 0; x < 4; x++)
				for (y = 0; y < 4; y++)
					rdst[x][y] = vram_rd32(cnum, addr + x * 4 + y * pitch);
				printf("realer:\n");
				for (y = 0; y < 4; y++)
					for (x = 0; x < 4; x++) 
						printf("%08x%c", rdst[x][y], x == 3 ? '\n' : ' ');
				res = HWTEST_RES_FAIL;
				break;
			}
		}
	}
	nva_wr32(cnum, 0x100240, 0);
	nva_wr32(cnum, 0x100300, 0);
	return res;
}

int test_pb(int cnum, int pb, int (*fun)(int cnum)) {
	uint32_t orig = nva_rd32(cnum, 0x100200);
	uint32_t cfg = orig;
	int partbits = get_partbits(cnum);
	if (partbits < 0)
		return HWTEST_RES_UNPREP;
	if (pb > partbits)
		return HWTEST_RES_NA;
	while (pb < partbits) {
		switch (cfg & 0xf) {
			case 0:
				nva_wr32(cnum, 0x100200, orig);
				return HWTEST_RES_NA;
			case 1:
				cfg &= ~0xf;
				break;
			case 3:
				cfg &= ~0xf;
				cfg |= 1;
				break;
			default:
				abort();
		}
		nva_wr32(cnum, 0x100200, cfg);
		partbits = get_partbits(cnum);
	}
	int res = fun(cnum);
	nva_wr32(cnum, 0x100200, orig);
	return res;
}

int test_format_pb0(int cnum) {
	return test_pb(cnum, 0, test_format);
}

int test_format_pb1(int cnum) {
	return test_pb(cnum, 1, test_format);
}

int test_format_pb2(int cnum) {
	return test_pb(cnum, 2, test_format);
}

int test_format_pb3(int cnum) {
	return test_pb(cnum, 3, test_format);
}

int test_format_pb4(int cnum) {
	return test_pb(cnum, 4, test_format);
}

int test_zcomp_layout_pb0(int cnum) {
	return test_pb(cnum, 0, test_zcomp_layout);
}

int test_zcomp_layout_pb1(int cnum) {
	return test_pb(cnum, 1, test_zcomp_layout);
}

int test_zcomp_layout_pb2(int cnum) {
	return test_pb(cnum, 2, test_zcomp_layout);
}

int test_zcomp_layout_pb3(int cnum) {
	return test_pb(cnum, 3, test_zcomp_layout);
}

int test_zcomp_layout_pb4(int cnum) {
	return test_pb(cnum, 4, test_zcomp_layout);
}

struct hwtest_test nv20_tile_tests[] = {
	HWTEST_TEST(test_scan, 0),
	HWTEST_TEST(test_status, 0),
	HWTEST_TEST(test_zcomp_size, 0),
	HWTEST_TEST(test_zcomp_access, 0),
	HWTEST_TEST(test_format_pb0, 1),
	HWTEST_TEST(test_format_pb1, 1),
	HWTEST_TEST(test_format_pb2, 1),
	HWTEST_TEST(test_format_pb3, 1),
	HWTEST_TEST(test_format_pb4, 1),
	HWTEST_TEST(test_zcomp_layout_pb0, 0),
	HWTEST_TEST(test_zcomp_layout_pb1, 0),
	HWTEST_TEST(test_zcomp_layout_pb2, 0),
	HWTEST_TEST(test_zcomp_layout_pb3, 0),
	HWTEST_TEST(test_zcomp_layout_pb4, 0),
	HWTEST_TEST(test_zcomp_format, 0),
};

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
	int cnum = 0;
	int colors = 1;
	int noslow = 0;
	while ((c = getopt (argc, argv, "c:ns")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
			case 'n':
				colors = 0;
				break;
			case 's':
				noslow = 1;
				break;
		}
	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}
	if (nva_cards[cnum].chipset < 0x20 || nva_cards[cnum].chipset == 0x2a || nva_cards[cnum].chipset == 0x34 || nva_cards[cnum].chipset >= 0x40) {
		fprintf(stderr, "Test not applicable for this chipset.\n");
		return 2;
	}
	if (!(nva_rd32(cnum, 0x200) & 1 << 20)) {
		fprintf(stderr, "Mem controller not up.\n");
		return 3;
	}
	int i;
	int worst = 0;
	for (i = 0; i < ARRAY_SIZE(nv20_tile_tests); i++) {
		if (nv20_tile_tests[i].slow && noslow) {
			printf("%s: skipped\n", nv20_tile_tests[i].name);
		} else {
			int res = nv20_tile_tests[i].fun(cnum);
			if (worst < res)
				worst = res;
			const char *tab[] = {
				[HWTEST_RES_NA] = "n/a",
				[HWTEST_RES_PASS] = "passed",
				[HWTEST_RES_UNPREP] = "hw not prepared",
				[HWTEST_RES_FAIL] = "FAILED",
			};
			const char *tabc[] = {
				[HWTEST_RES_NA] = "n/a",
				[HWTEST_RES_PASS] = "\x1b[32mpassed\x1b[0m",
				[HWTEST_RES_UNPREP] = "\x1b[33mhw not prepared\x1b[0m",
				[HWTEST_RES_FAIL] = "\x1b[31mFAILED\x1b[0m",
			};
			printf("%s: %s\n", nv20_tile_tests[i].name, res[colors?tabc:tab]);
		}
	}
	if (worst == HWTEST_RES_PASS)
		return 0;
	else
		return worst + 1;
}
