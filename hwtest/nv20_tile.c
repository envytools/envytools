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
	if (nva_cards[cnum].chipset < 0x30) {
		switch (cfg0 & 0xf) {
			case 0:
				return 0;
			case 1:
				return 1;
			case 3:
				return 2;
			default:
				printf("Unknown part bits count!\n");
				return -1;
		}
	} else {
		return 1;
	}
}

int get_colbits(int cnum) {
	uint32_t cfg1 = nva_rd32(cnum, 0x100204);
	return cfg1 >> 12 & 0xf;
}

int get_mcbits(int cnum) {
	if (nva_cards[cnum].chipset < 0x30)
		return 2;
	uint32_t cfg0 = nva_rd32(cnum, 0x100200);
	int i, cnt = 0;
	for (i = 0; i < 4; i++)
		if (cfg0 & 1 << i)
			cnt++;
	switch (cnt) {
		case 1:
			return 2;
		case 2:
			return 3;
		case 4:
			return 4;
		default:
			printf("Unknown bus width!\n");
			return -1;
	}
}

int test_scan(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 8; i++) {
		if (ctx->chipset < 0x30) {
			TEST_BITSCAN(0x100240 + i * 0x10, 0xffffc003, 0);
			TEST_BITSCAN(0x100244 + i * 0x10, 0xffffffff, 0x3fff);
			TEST_BITSCAN(0x100248 + i * 0x10, 0x0000ff00, 0);
			if (ctx->chipset == 0x20) {
				TEST_BITSCAN(0x100300 + i * 0x4, 0xbc03ffc0, 0);
			} else {
				TEST_BITSCAN(0x100300 + i * 0x4, 0x01f3ffc0, 0);
			}
		} else {
			TEST_BITSCAN(0x100240 + i * 0x10, 0xffffc031, 0);
			TEST_BITSCAN(0x100244 + i * 0x10, 0xffffffff, 0x3fff);
			TEST_BITSCAN(0x100248 + i * 0x10, 0x0000ff00, 0);
			if (ctx->chipset < 0x35) { /* XXX: confirm on NV30, NV36 */
				TEST_BITSCAN(0x100300 + i * 0x4, 0x1fffffff, 0);
			} else {
				TEST_BITSCAN(0x100300 + i * 0x4, 0x7fffffff, 0);
			}
		}
	}
	if (ctx->chipset == 0x20) {
		TEST_BITSCAN(0x100324, 0x83ffc00f, 0);
	} else {
		TEST_BITSCAN(0x100324, 0x83ffc01f, 0);
	}
	return HWTEST_RES_PASS;
}

uint32_t get_prime_mask(int cnum) {
	if (nva_cards[cnum].chipset >= 0x25)
		return 1 << 0xd | 1 << 0x7 | 1 << 0x5 | 1 << 0x3 | 1 << 0x1;
	else
		return 1 << 0x7 | 1 << 0x5 | 1 << 0x3 | 1 << 0x1;
}

uint32_t compute_status(uint32_t pitch, uint32_t prime_mask) {
	/* like NV10, except pitches 0x300, 0x500, 0x700 are now valid */
	if (pitch & ~0xff00)
		abort();
	if (pitch < 0x200)
		return 0xffffffff;
	int shift = 0;
	while (!(pitch & 1))
		pitch >>= 1, shift++;
	if (!(prime_mask & (1 << pitch)))
		return 0xffffffff;
	int factor;
	switch (pitch) {
		case 0x1:
			factor = 0;
			break;
		case 0x3:
			factor = 1;
			break;
		case 0x5:
			factor = 2;
			break;
		case 0x7:
			factor = 3;
			break;
		case 0xd:
			factor = 4;
			break;
		default:
			return 0xffffffff;
	}
	return factor | (shift - 8) << 4;
}

int test_status(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 0x100; i++) {
		uint32_t pitch = i << 8;
		nva_wr32(ctx->cnum, 0x100240, 0);
		nva_wr32(ctx->cnum, 0x100248, pitch);
		uint32_t exp = compute_status(pitch, get_prime_mask(ctx->cnum));
		if (exp == 0xffffffff)
			exp = 0;
		uint32_t real = nva_rd32(ctx->cnum, 0x10024c);
		if (exp != real) {
			printf("Tile region status mismatch for pitch %05x: is %08x, expected %08x\n", pitch, real, exp);
			return HWTEST_RES_FAIL;
		}
		nva_wr32(ctx->cnum, 0x100240, 1);
		exp = compute_status(pitch, get_prime_mask(ctx->cnum));
		if (exp == 0xffffffff)
			exp = 0;
		else
			exp |= 0x80000000;
		real = nva_rd32(ctx->cnum, 0x10024c);
		nva_wr32(ctx->cnum, 0x100240, 0);
		if (exp != real) {
			printf("Tile region status mismatch for pitch %05x [enabled]: is %08x, expected %08x\n", pitch, real, exp);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

int test_zcomp_size(struct hwtest_ctx *ctx) {
	uint32_t expected;
	uint32_t real = nva_rd32(ctx->cnum, 0x100320);
	switch (ctx->chipset) {
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
			printf("Don't know expected zcomp size for NV%02X [%08x] - please report!\n", ctx->chipset, real);
			return HWTEST_RES_UNPREP;
	}
	if (real != expected) {
		printf("Zcomp size mismatch: is %08x, expected %08x\n", real, expected);
		return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

void zcomp_seek(int cnum, int part, int addr) {
	if (nva_cards[cnum].chipset == 0x20) {
		nva_wr32(cnum, 0x1000f0, 0x1300000 |
			 (part << 16) | (addr & 0x1fc0));
	} else if (nva_cards[cnum].chipset < 0x30) {
		nva_wr32(cnum, 0x1000f0, 0x2380000 |
			 ((addr << 6) & 0x40000) |
			 (part << 16) | (addr & 0xfc0));
	} else {
		nva_wr32(cnum, 0x1000f0, 0x2380000 |
			 (part << 16) | (addr & 0x7fc0));
	}
}

uint32_t zcomp_rd32(int cnum, int part, int addr) {
	zcomp_seek(cnum, part, addr);
	uint32_t res = nva_rd32(cnum, 0x100100 + (addr & 0x3c));
	nva_wr32(cnum, 0x1000f0, 0);
	return res;
}

void zcomp_wr32(int cnum, int part, int addr, uint32_t v) {
	zcomp_seek(cnum, part, addr);
	nva_wr32(cnum, 0x100100 + (addr & 0x3c), v);
	nva_rd32(cnum, 0x100100 + (addr & 0x3c));
	nva_wr32(cnum, 0x1000f0, 0);
}

void clear_zcomp(int cnum) {
	int partbits = get_partbits(cnum);
	uint32_t size = (nva_rd32(cnum, 0x100320) + 1) / 8;
	int i, j;
	for (i = 0; i < (1 << partbits); i++) {
		for (j = 0; j < size; j += 0x4)
			zcomp_wr32(cnum, i, j, 0);
	}
}

int get_maxparts(int chipset) {
	switch (chipset) {
		case 0x20:
		case 0x25:
		case 0x28:
			return 4;
		case 0x30:
		case 0x31:
			return 2;
		case 0x35:
		case 0x36:
		default:
			abort();
	}
}

int test_zcomp_access(struct hwtest_ctx *ctx) {
	uint32_t size = (nva_rd32(ctx->cnum, 0x100320) + 1) / 8;
	int i, j;
	int parts = get_maxparts(ctx->chipset);
	clear_zcomp(ctx->cnum);
	for (i = 0; i < parts; i++) {
		for (j = 0; j < size; j += 0x4)
			zcomp_wr32(ctx->cnum, i, j, 0xdea00000 | i << 16 | j);
	}
	for (i = 0; i < parts; i++) {
		for (j = 0; j < size; j += 0x4) {
			uint32_t val = zcomp_rd32(ctx->cnum, i, j);
			uint32_t x = 0xdea00000 | i << 16 | j;
			if (val != x) {
				printf("MISMATCH %08x %08x\n", x, val);
				return HWTEST_RES_FAIL;
			}
		}
	}
	return HWTEST_RES_PASS;
}

uint32_t translate_addr(uint32_t pitch, uint32_t laddr, int colbits, int partbits, int mcbits, int bankoff, int is_nv30) {
	uint32_t status = compute_status(pitch, ~0);
	uint32_t shift = status >> 4;
	int ybits = partbits + colbits + mcbits - 8;
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
	if (is_nv30) {
		int bank = res & 3;
		if (shift >= 2)
			bank ^= y << 1 & 2;
		if (shift >= 1)
			bank += y >> 1 & 1;
		bank ^= bankoff;
		bank &= 3;
		res = (res & ~3) | bank;
	} else {
		res ^= bankoff;
		if (y & 1 && shift)
			res ^= 1;
	}
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

int test_zcomp_layout(struct hwtest_ctx *ctx) {
	int partbits = get_partbits(ctx->cnum);
	int colbits = get_colbits(ctx->cnum);
	uint32_t zcomp_size = (nva_rd32(ctx->cnum, 0x100320) + 1);
	uint32_t fb_size = 0xc00000;
	int i, j, m;
	clear_zcomp(ctx->cnum);
	nva_wr32(ctx->cnum, 0x100240, 0);
	for (i = 0; i < fb_size; i += 0x10) {
		vram_wr32(ctx->cnum, i+0x0, 0x0000ffff);
		vram_wr32(ctx->cnum, i+0x4, 0x00000000);
		vram_wr32(ctx->cnum, i+0x8, 0x00000000);
		vram_wr32(ctx->cnum, i+0xc, 0x00000000);
	}
	uint32_t pitch = 0xe000;
	nva_wr32(ctx->cnum, 0x100248, pitch);
	nva_wr32(ctx->cnum, 0x100244, fb_size - 1);
	nva_wr32(ctx->cnum, 0x100240, 1);
	if (nva_cards[ctx->cnum].chipset >= 0x25) {
		nva_wr32(ctx->cnum, 0x100300, 0x00200000);
	} else {
		nva_wr32(ctx->cnum, 0x100300, 0x90000000);
	}
	for (i = 0; i < (1 << partbits); i++) {
		for (j = 0; j < zcomp_size; j++) {
			zcomp_wr32(ctx->cnum, i, j >> 3, 1 << (j & 0x1f));
			uint32_t exp = translate_tag(pitch, i, j, colbits, partbits);
			if (exp < fb_size && !vram_rd32(ctx->cnum, exp + 4)) {
				printf("part %d tag %05x: expected to belong to %08x", i, j, exp);
				int found = 0;
				for (m = 0; m < fb_size; m += 0x10) {
					uint32_t mm = vram_rd32(ctx->cnum, m + 0x4);
					if (mm) {
						printf(", found at %08x instead\n", m);
						break;
					}
				}
				if (!found) {
					printf(", but not found in surface\n");
				}
				nva_wr32(ctx->cnum, 0x100240, 0);
				nva_wr32(ctx->cnum, 0x100300, 0);
				return HWTEST_RES_FAIL;
			}
			zcomp_wr32(ctx->cnum, i, j >> 3, 0);
		}
	}
	nva_wr32(ctx->cnum, 0x100240, 0);
	nva_wr32(ctx->cnum, 0x100300, 0);
	return HWTEST_RES_PASS;
}

int test_format(struct hwtest_ctx *ctx) {
	int i, j;
	int partbits = get_partbits(ctx->cnum);
	int colbits = get_colbits(ctx->cnum);
	int mcbits = get_mcbits(ctx->cnum);
	if (partbits < 0)
		return HWTEST_RES_UNPREP;
	int res = HWTEST_RES_PASS;
	int bankoff;
	int banktry;
	if (ctx->chipset < 0x30)
		banktry = 2;
	else
		banktry = 4;
	for (i = 0; i < 8; i++)
		nva_wr32(ctx->cnum, 0x100240 + i * 0x10, 0);
	for (i = 0; i < 0x200000; i += 4)
		vram_wr32(ctx->cnum, i, 0xc0000000 | i);
	for (i = 0; i < 0x100; i++) {
		uint32_t pitch = i << 8;
		nva_wr32(ctx->cnum, 0x100300, 0);
		nva_wr32(ctx->cnum, 0x100248, pitch);
		nva_wr32(ctx->cnum, 0x100244, 0x000fc000);
		if (compute_status(pitch, get_prime_mask(ctx->cnum)) == 0xffffffff)
			continue;
		for (bankoff = 0; bankoff < banktry; bankoff++) {
			if (ctx->chipset < 0x30)
				nva_wr32(ctx->cnum, 0x100240, 1 | bankoff << 1);
			else
				nva_wr32(ctx->cnum, 0x100240, 1 | bankoff << 4);
			for (j = 0; j < 0x100000; j += 4) {
				uint32_t real = vram_rd32(ctx->cnum, j);
				uint32_t exp = 0xc0000000 | translate_addr(pitch, j, colbits, partbits, mcbits, bankoff, ctx->chipset >= 0x30);
				if (real != exp) {
					printf("Mismatch at %08x for pitch %05x bankoff %d: is %08x, expected %08x\n", j, pitch, bankoff, real, exp);
					res = HWTEST_RES_FAIL;
					break;
				}
			}
		}
		for (j = 0x100000; j < 0x180000; j += 4) {
			uint32_t real = vram_rd32(ctx->cnum, j);
			uint32_t exp = 0xc0000000 | j;
			if (real != exp) {
				printf("Mismatch at %08x [after limit] for pitch %05x: is %08x, expected %08x\n", j, pitch, real, exp);
				res = HWTEST_RES_FAIL;
				break;
			}
		}
		nva_wr32(ctx->cnum, 0x100240, 0x0);
	}
	return res;
}

struct zcomp_format {
	uint32_t value;
	int bpp;
	int x0, y0;
	int big_endian;
	int multisample;
};

static const struct zcomp_format nv20_zcomp_formats[] = {
	{ 0x80000000, 16, 2, 0, 0, 0 },
	{ 0x84000000, 32, 1, 1, 0, 0 },
	{ 0x88000000, 16, 2, 0, 1, 0 },
	{ 0x8c000000, 32, 1, 1, 1, 0 },
	{ 0x90000000, 16, 2, 0, 0, 0 },
	{ 0x94000000, 32, 1, 1, 0, 0 },
	{ 0x98000000, 16, 2, 0, 1, 0 },
	{ 0x9c000000, 32, 1, 1, 1, 0 },
	{ 0xa0000000, 16, 2, 0, 0, 0 },
	{ 0xa4000000, 32, 1, 1, 0, 0 },
	{ 0xa8000000, 16, 2, 0, 1, 0 },
	{ 0xac000000, 32, 1, 1, 1, 0 },
	{ 0xb0000000, 16, 2, 0, 0, 0 },
	{ 0xb4000000, 32, 1, 1, 0, 0 },
	{ 0xb8000000, 16, 2, 0, 1, 0 },
	{ 0xbc000000, 32, 1, 1, 1, 0 },
	{ 0 }
};

static const struct zcomp_format nv25_zcomp_formats[] = {
	{ 0x00100000, 16, 3, 0, 0, 0 },
	{ 0x00200000, 32, 1, 1, 0, 0 },
	{ 0x00300000, 16, 3, 0, 0, 1 },
	{ 0x00400000, 32, 1, 1, 0, 1 },
	{ 0x00500000, 16, 3, 0, 0, 0 },
	{ 0x00600000, 32, 1, 1, 0, 0 },
	{ 0x01100000, 16, 3, 0, 1, 0 },
	{ 0x01200000, 32, 1, 1, 1, 0 },
	{ 0x01300000, 16, 3, 0, 1, 1 },
	{ 0x01400000, 32, 1, 1, 1, 1 },
	{ 0x01500000, 16, 3, 0, 1, 0 },
	{ 0x01600000, 32, 1, 1, 1, 0 },
	{ 0 }
};

void zcomp_decompress(const struct zcomp_format *fmt, uint32_t dst[4][4], uint32_t src[4]) {
	int x, y;
	uint64_t w0 = src[0] | (uint64_t)src[1] << 32;
	uint64_t w1 = src[2] | (uint64_t)src[3] << 32;
	int32_t dz;

	/* short flag */
	if (w0 & (uint64_t)1 << 63)
		w1 = 0;

	if (fmt->bpp == 16) {
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
		if (fmt->x0 == 2) {
			delta[2][0] = 0;
			delta[3][0] = w0 >> 46 & 7;
			delta[4][0] = 0;
			delta[5][0] = w0 >> 49 & 7;
		} else if (fmt->x0 == 3) {
			delta[2][0] = w0 >> 46 & 7;
			delta[3][0] = 0;
			delta[4][0] = w0 >> 49 & 7;
			delta[5][0] = 0;
		}
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
		if (fmt->x0 == 2) {
			delta[2][2] = 0;
			delta[3][2] = w1 >> 25 & 7;
		} else if (fmt->x0 == 3) {
			delta[2][2] = w1 >> 25 & 7;
			delta[3][2] = 0;
		}
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

				if (fmt->multisample) {
					dz = (dx * 2 * (x - fmt->x0) +
					      dy * (2 * (y - fmt->y0) -
						    ((x - fmt->x0) & 1)) +
					      (y ? 2 : 3)) >> 2;
				} else {
					dz = (dx * (x - fmt->x0) +
					      dy * (y - fmt->y0) + 1) >> 1;
				}

				tdst[x][y] = base + dz + delta[x][y];
				if (fmt->big_endian) {
					tdst[x][y] = tdst[x][y] << 8 | tdst[x][y] >> 8;
				}
			}
		}
		for (x = 0; x < 4; x++) {
			for (y = 0; y < 4; y++) {
				dst[x][y] = tdst[2*x][y] | tdst[2*x+1][y] << 16;
			}
		}
	} else if (fmt->bpp == 32) {
		/* Z24S8 */
		uint32_t delta[4][4];
		uint8_t stencil = w0 & 0xff;
		uint32_t base = w0 >> 8 & 0xffffff;
		uint32_t dx = w0 >> 32 & 0x7fff;
		uint32_t dy = w0 >> 47 & 0x7fff;
		if (dx & 0x4000)
			dx |= 0xffff8000;
		if (dy & 0x4000)
			dy |= 0xffff8000;
		delta[0][0] = (w0 >> 62 & 1) | (w1 & 0xf) << 1;
		delta[1][0] = w1 >> 4 & 0x1f;
		delta[2][0] = w1 >> 9 & 0x1f;
		delta[3][0] = w1 >> 14 & 0x1f;
		delta[0][1] = w1 >> 19 & 0x1f;
		delta[1][1] = 0;
		if (fmt->multisample) {
			delta[2][1] = w1 >> 24 & 0x1f;
			delta[3][1] = 0;
		} else {
			delta[2][1] = 0;
			delta[3][1] = w1 >> 24 & 0x1f;
		}
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
					delta[x][y] |= 0xffffffe0;

				if (fmt->multisample) {
					dz = (dx * (x - fmt->x0) +
					      dy * (2 * (y - fmt->y0)
						    - ((x - fmt->x0) & 1)) + 1) >> 1;
				} else {
					dz = (x - fmt->x0) * dx + (y - fmt->y0) * dy;
				}

				dst[x][y] = (base + dz + delta[x][y]) << 8 | stencil;
				if (fmt->big_endian) {
					dst[x][y] = (dst[x][y] & 0xff00ff) << 8 | (dst[x][y] & 0xff00ff00) >> 8;
					dst[x][y] = dst[x][y] << 16 | dst[x][y] >> 16;
				}
			}
		}
	}
}

int test_zcomp_format(struct hwtest_ctx *ctx) {
	int res = HWTEST_RES_PASS;
	int colbits = get_colbits(ctx->cnum);
	int partbits = get_partbits(ctx->cnum);
	uint32_t pitch = 0x200;
	const struct zcomp_format *fmt =
		(nva_cards[ctx->cnum].chipset >= 0x25 ?
		 nv25_zcomp_formats : nv20_zcomp_formats);

	nva_wr32(ctx->cnum, 0x100248, pitch);
	nva_wr32(ctx->cnum, 0x100244, 0);
	nva_wr32(ctx->cnum, 0x100240, 1);
	int i;
	for (; fmt->value; ++fmt) {
		nva_wr32(ctx->cnum, 0x100300, fmt->value);
		for (i = 0; i < 100000; i++) {
			int tag = rand() & 0xff;
			int part = tag & ((1 << partbits) - 1);
			tag >>= partbits;
			uint32_t addr = translate_tag(pitch, part, tag, colbits, partbits);
			uint32_t src[4];
			int k;
			zcomp_wr32(ctx->cnum, part, (tag >> 3 & 0x3c), 0);
			for (k = 0; k < 4; k++) {
				src[k] = rand() ^ rand() << 10 ^ rand() << 20;
				vram_wr32(ctx->cnum, addr + k * 4, src[k]);
			}
			zcomp_wr32(ctx->cnum, part, (tag >> 3 & 0x3c), 1 << (tag & 0x1f));
			uint32_t dst[4][4], rdst[4][4];
			int x, y;
			for (x = 0; x < 4; x++)
				for (y = 0; y < 4; y++)
					rdst[x][y] = vram_rd32(ctx->cnum, addr + x * 4 + y * pitch);
			zcomp_decompress(fmt, dst, src);
			int fail = 0;
			for (x = 0; x < 4; x++)
				for (y = 0; y < 4; y++)
					if (rdst[x][y] != dst[x][y])
						fail = 1;
			if (fail) {
				printf("ZCOMP decompression mismatch iter %d: format %08x part %d tag %05x, src %08x %08x %08x %08x\n", i, fmt->value, part, tag, src[0], src[1], src[2], src[3]);
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
						rdst[x][y] = vram_rd32(ctx->cnum, addr + x * 4 + y * pitch);
				printf("realer:\n");
				for (y = 0; y < 4; y++)
					for (x = 0; x < 4; x++) 
						printf("%08x%c", rdst[x][y], x == 3 ? '\n' : ' ');
				res = HWTEST_RES_FAIL;
				break;
			}
		}
	}
	nva_wr32(ctx->cnum, 0x100240, 0);
	nva_wr32(ctx->cnum, 0x100300, 0);
	return res;
}

int test_pb(struct hwtest_ctx *ctx, int pb, int (*fun)(struct hwtest_ctx *ctx)) {
	uint32_t orig = nva_rd32(ctx->cnum, 0x100200);
	uint32_t cfg = orig;
	int partbits = get_partbits(ctx->cnum);
	if (partbits < 0)
		return HWTEST_RES_UNPREP;
	if (pb > partbits)
		return HWTEST_RES_NA;
	while (pb < partbits) {
		if (ctx->chipset < 0x30) {
			switch (cfg & 0xf) {
				case 0:
					nva_wr32(ctx->cnum, 0x100200, orig);
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
		} else {
			/* XXX */
			nva_wr32(ctx->cnum, 0x100200, orig);
			return HWTEST_RES_NA;
		}
		nva_wr32(ctx->cnum, 0x100200, cfg);
		partbits = get_partbits(ctx->cnum);
	}
	int res = fun(ctx);
	nva_wr32(ctx->cnum, 0x100200, orig);
	return res;
}

int test_format_pb0(struct hwtest_ctx *ctx) {
	return test_pb(ctx, 0, test_format);
}

int test_format_pb1(struct hwtest_ctx *ctx) {
	return test_pb(ctx, 1, test_format);
}

int test_format_pb2(struct hwtest_ctx *ctx) {
	return test_pb(ctx, 2, test_format);
}

int test_zcomp_layout_pb0(struct hwtest_ctx *ctx) {
	return test_pb(ctx, 0, test_zcomp_layout);
}

int test_zcomp_layout_pb1(struct hwtest_ctx *ctx) {
	return test_pb(ctx, 1, test_zcomp_layout);
}

int test_zcomp_layout_pb2(struct hwtest_ctx *ctx) {
	return test_pb(ctx, 2, test_zcomp_layout);
}

struct hwtest_test nv20_tile_tests[] = {
	HWTEST_TEST(test_scan, 0),
	HWTEST_TEST(test_status, 0),
	HWTEST_TEST(test_zcomp_size, 0),
	HWTEST_TEST(test_zcomp_access, 0),
	HWTEST_TEST(test_format_pb0, 1),
	HWTEST_TEST(test_format_pb1, 1),
	HWTEST_TEST(test_format_pb2, 1),
	HWTEST_TEST(test_zcomp_layout_pb0, 0),
	HWTEST_TEST(test_zcomp_layout_pb1, 0),
	HWTEST_TEST(test_zcomp_layout_pb2, 0),
	HWTEST_TEST(test_zcomp_format, 0),
};

struct hwtest_group nv20_tile = {
	nv20_tile_tests,
	ARRAY_SIZE(nv20_tile_tests),
};

int main(int argc, char **argv) {
	struct hwtest_ctx sctx;
	struct hwtest_ctx *ctx = &sctx;
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
	ctx->cnum = 0;
	ctx->colors = 1;
	ctx->noslow = 0;
	while ((c = getopt (argc, argv, "c:ns")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &ctx->cnum);
				break;
			case 'n':
				ctx->colors = 0;
				break;
			case 's':
				ctx->noslow = 1;
				break;
		}
	if (ctx->cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}
	ctx->chipset = nva_cards[ctx->cnum].chipset;
	if (ctx->chipset < 0x20 || ctx->chipset == 0x2a || ctx->chipset == 0x34 || ctx->chipset >= 0x40) {
		fprintf(stderr, "Test not applicable for this chipset.\n");
		return 2;
	}
	if (!(nva_rd32(ctx->cnum, 0x200) & 1 << 20)) {
		fprintf(stderr, "Mem controller not up.\n");
		return 3;
	}
	if (ctx->chipset >= 0x30) {
		/* accessing 1000f0/100100 is a horribly bad idea while any mem operations are in progress on NV30+ [at least NV31]... */
		/* tell PCRTC* to fuck off */
		nva_wr8(ctx->cnum, 0x0c03c4, 1);
		nva_wr8(ctx->cnum, 0x0c03c5, 0x20);
		/* tell vgacon to fuck off */
		nva_wr32(ctx->cnum, 0x001854, 0);
	}
	int res = hwtest_run_group(ctx, &nv20_tile);
	if (res == HWTEST_RES_PASS)
		return 0;
	else
		return res + 1;
}
