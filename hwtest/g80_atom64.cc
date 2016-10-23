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
#include "g80_gr.h"
#include "nva.h"
#include "util.h"
#include <stdbool.h>

static int g80_atom64_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type != 0x50)
		return HWTEST_RES_NA;
	if (ctx->chipset.chipset < 0xa0)
		return HWTEST_RES_NA;
	if (!(nva_rd32(ctx->cnum, 0x200) & 1 << 20)) {
		printf("Mem controller not up.\n");
		return HWTEST_RES_UNPREP;
	}
	if (g80_gr_prep(ctx))
		return HWTEST_RES_FAIL;
	if (g80_gr_compute_prep(ctx))
		return HWTEST_RES_FAIL;
	return HWTEST_RES_PASS;
}

static int atom64_prep_code(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2) {
	uint32_t code[] = {
0x30030001,
0xc4100780,
0x20008005,
0x00000103,
0x20008009,
0x00000203,
0x2000800d,
0x00000303,
0x100f8011,
0x00000003,
0x00000801,
0xa0000780,
0xd0000211,
0x80800780,
0xd0000419,
0x80800780,
op1,
op2,
0xd0000421,
0xa0800780,
0x0000001d,
0x20000780,
0xd000021d,
0xa0c00780,
0xf0000001,
0xe0000781,
	};
	unsigned i;
	/* Poke code and flush it. */
	nva_wr32(ctx->cnum, 0x1700, 0x100);
	for (i = 0; i < ARRAY_SIZE(code); i++)
		nva_wr32(ctx->cnum, 0x730000 + i * 4, code[i]);
	nva_wr32(ctx->cnum, 0x70000, 1);
	while (nva_rd32(ctx->cnum, 0x70000));
	return g80_gr_mthd(ctx, 3, 0x380, 0);
}

static int atom64_prep_grid(struct hwtest_ctx *ctx, uint32_t xtra) {
	return
		/* CTA config. */
		g80_gr_mthd(ctx, 3, 0x3a8, 0x40) ||
		g80_gr_mthd(ctx, 3, 0x3ac, 0x00010200) ||
		g80_gr_mthd(ctx, 3, 0x3b0, 0x00000001) ||
		g80_gr_mthd(ctx, 3, 0x2c0, 0x0000000a) || /* regs */
		g80_gr_mthd(ctx, 3, 0x2b4, 0x00010200) || /* threads & barriers */
		g80_gr_mthd(ctx, 3, 0x2b8, 0x00000001) ||
		g80_gr_mthd(ctx, 3, 0x3b8, 0x00000002) ||
		g80_gr_mthd(ctx, 3, 0x2f8, 0x00000001) || /* init */
		/* Grid config. */
		g80_gr_mthd(ctx, 3, 0x388, 0) ||
		g80_gr_mthd(ctx, 3, 0x3a4, 0x00010001) ||
		g80_gr_mthd(ctx, 3, 0x374, 0) ||
		g80_gr_mthd(ctx, 3, 0x384, 0x100) ||
		/* Funny stuff */
		g80_gr_mthd(ctx, 3, 0x37c, (xtra & 1) | (xtra << 15 & 0x10000));
}

static void atom64_write_data(struct hwtest_ctx *ctx, const uint64_t *src1, const uint64_t *src2, const uint64_t *src3) {
	int i;
	nva_wr32(ctx->cnum, 0x1700, 0x200);
	for (i = 0; i < 0x200; i++) {
		nva_wr32(ctx->cnum, 0x700000 + i * 8, src1[i]);
		nva_wr32(ctx->cnum, 0x700004 + i * 8, src1[i] >> 32);
		nva_wr32(ctx->cnum, 0x701000 + i * 8, src2[i]);
		nva_wr32(ctx->cnum, 0x701004 + i * 8, src2[i] >> 32);
		nva_wr32(ctx->cnum, 0x702000 + i * 8, src3[i]);
		nva_wr32(ctx->cnum, 0x702004 + i * 8, src3[i] >> 32);
	}
	nva_wr32(ctx->cnum, 0x70000, 1);
	while (nva_rd32(ctx->cnum, 0x70000));
}

static int atom64_run(struct hwtest_ctx *ctx) {
	return g80_gr_mthd(ctx, 3, 0x368, 0) || g80_gr_idle(ctx);
}

static int atom64_check_data(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2, const uint64_t *src1, const uint64_t *src2, const uint64_t *src3, uint64_t xtra) {
	int i;
	for (i = 0; i < 0x200; i++) {
		uint64_t real = nva_rd32(ctx->cnum, 0x700000 + i * 8);
		real |= (uint64_t)nva_rd32(ctx->cnum, 0x700004 + i * 8) << 32;
		uint32_t cc = nva_rd32(ctx->cnum, 0x701000 + i * 8);
		uint64_t realold = nva_rd32(ctx->cnum, 0x702000 + i * 8);
		realold |= (uint64_t)nva_rd32(ctx->cnum, 0x702004 + i * 8) << 32;
		uint64_t exp;
		uint32_t ecc = 0xf;
		uint8_t op = (op2 >> 2) & 0xf;
		uint64_t s1 = src1[i];
		uint64_t s2 = src2[i];
		uint64_t s3 = src3[i];
		uint64_t expold = s1;
		bool is_atom = (op2 >> 29) == 7;
		switch (op) {
			case 0: /* add */
				exp = s1 + s2;
				break;
			case 1: /* exch */
				exp = s2;
				break;
			case 2: /* cas */
				if (s1 == s2)
					exp = s3;
				else
					exp = s1;
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			case 0x8:
			case 0x9:
			case 0xa:
			case 0xb:
			case 0xc:
			case 0xd:
			case 0xe:
			case 0xf:
				exp = s1;
				break;
			default:
				abort();
				break;
		}
		if (real != exp || cc != ecc || (is_atom && realold != expold)) {
			printf("atom64 %08x %08x (%016" PRIx64 " %016" PRIx64 " %016" PRIx64 " %016" PRIx64 ")[%d]: got %016" PRIx64 ".%x %016" PRIx64 " expected %016" PRIx64 ".%x %016" PRIx64 "\n", op1, op2, src1[i], src2[i], src3[i], s1, i&3, real, cc, realold, exp, ecc, expold);
			return 1;
		}
	}
	return 0;
}

static int atom64_test(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2, const uint64_t *src1, const uint64_t *src2, const uint64_t *src3, uint32_t xtra) {
	atom64_write_data(ctx, src1, src2, src3);
	if (atom64_run(ctx))
		return 1;
	if (atom64_check_data(ctx, op1, op2, src1, src2, src3, xtra))
		return 1;
	return 0;
}

static void atom64_gen(struct hwtest_ctx *ctx, uint64_t *src1, uint64_t *src2, uint64_t *src3) {
	int i;
	int mode = jrand48(ctx->rand48);
	for (i = 0; i < 0x200; i++) {
		src1[i] = (uint32_t)jrand48(ctx->rand48) | (uint64_t)jrand48(ctx->rand48) << 32;
		src2[i] = (uint32_t)jrand48(ctx->rand48) | (uint64_t)jrand48(ctx->rand48) << 32;
		src3[i] = (uint32_t)jrand48(ctx->rand48) | (uint64_t)jrand48(ctx->rand48) << 32;
		/* take sources from -0x80..0x7f range 1/2 of the time. */
		if (mode & 1)
			src1[i] = (int8_t)src1[i];
		if (mode & 2)
			src2[i] = (int8_t)src2[i];
		if (mode & 4)
			src3[i] = (int8_t)src3[i];
	}
}

static int test_atom64(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xd0000011 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0xc08007c0 | (jrand48(ctx->rand48) & 0x3f1ff03c);
		uint32_t xtra = jrand48(ctx->rand48);
		int op = op2 >> 2 & 0xf;
		if (op2 & 0x20000000) {
			/* if atom, use different operands than red */
			op1 &= ~0x7f01fc;
			op1 |= 0x040020;
			if (op == 2) {
				/* cas selected, aim the third source */
				op2 &= ~0x001fc000;
				op2 |= 0x00018000;
			}
		} else if (op == 2) {
			/* red cas selected, bail */
			continue;
		}
		/* op 3 is a trap */
		if (op == 3)
			continue;
		if (atom64_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (atom64_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint64_t src1[0x200], src2[0x200], src3[0x200];
		atom64_gen(ctx, src1, src2, src3);
		if (atom64_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(g80_atom64,
	HWTEST_TEST(test_atom64, 0),
)
