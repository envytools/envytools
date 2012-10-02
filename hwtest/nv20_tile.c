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
	switch (partbits) {
		case 0:
			break;
		case 1:
			if (y2 & 1)
				part++;
			break;
		case 2:
			if (y2 & 1)
				part += 2;
			if (y2 & 2)
				part += 1;
			break;
		default:
			abort();
	}
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

int test_format_pb(int cnum, int pb) {
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
	int res = test_format(cnum);
	nva_wr32(cnum, 0x100200, orig);
	return res;
}

int test_format_pb0(int cnum) {
	return test_format_pb(cnum, 0);
}

int test_format_pb1(int cnum) {
	return test_format_pb(cnum, 1);
}

int test_format_pb2(int cnum) {
	return test_format_pb(cnum, 2);
}

int test_format_pb3(int cnum) {
	return test_format_pb(cnum, 3);
}

int test_format_pb4(int cnum) {
	return test_format_pb(cnum, 4);
}

struct hwtest_test nv20_tile_tests[] = {
	HWTEST_TEST(test_scan),
	HWTEST_TEST(test_status),
	HWTEST_TEST(test_zcomp_size),
	HWTEST_TEST(test_format_pb0),
	HWTEST_TEST(test_format_pb1),
	HWTEST_TEST(test_format_pb2),
	HWTEST_TEST(test_format_pb3),
	HWTEST_TEST(test_format_pb4),
};

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
	int cnum = 0;
	int colors = 1;
	while ((c = getopt (argc, argv, "c:n")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
			case 'n':
				colors = 0;
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
	if (worst == HWTEST_RES_PASS)
		return 0;
	else
		return worst + 1;
}
