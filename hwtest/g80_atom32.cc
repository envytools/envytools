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

static int g80_atom32_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type != 0x50)
		return HWTEST_RES_NA;
	if (ctx->chipset.chipset == 0x50)
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

static int atom32_prep_code(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2) {
	uint32_t code[] = {
		0x30020001,
		0xc4100780,
		0x20008005,
		0x00000083,
		0x20008009,
		0x00000103,
		0x2000800d,
		0x00000183,
		0x100f8011,
		0x00000003,
		0x00000801,
		0xa0000780,
		0xd0000215,
		0x80c00780,
		0xd0000419,
		0x80c00780,
		op1,
		op2,
		0xd000041d,
		0xa0c00780,
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

static int atom32_prep_grid(struct hwtest_ctx *ctx, uint32_t xtra) {
	return
		/* CTA config. */
		g80_gr_mthd(ctx, 3, 0x3a8, 0x40) ||
		g80_gr_mthd(ctx, 3, 0x3ac, 0x00010200) ||
		g80_gr_mthd(ctx, 3, 0x3b0, 0x00000001) ||
		g80_gr_mthd(ctx, 3, 0x2c0, 0x00000008) || /* regs */
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

static void atom32_write_data(struct hwtest_ctx *ctx, const uint32_t *src1, const uint32_t *src2, const uint32_t *src3) {
	int i;
	nva_wr32(ctx->cnum, 0x1700, 0x200);
	for (i = 0; i < 0x200; i++) {
		nva_wr32(ctx->cnum, 0x700000 + i * 4, src1[i]);
		nva_wr32(ctx->cnum, 0x700800 + i * 4, src2[i]);
		nva_wr32(ctx->cnum, 0x701000 + i * 4, src3[i]);
	}
	nva_wr32(ctx->cnum, 0x70000, 1);
	while (nva_rd32(ctx->cnum, 0x70000));
}

static int atom32_run(struct hwtest_ctx *ctx) {
	return g80_gr_mthd(ctx, 3, 0x368, 0) || g80_gr_idle(ctx);
}

static int atom32_check_data(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2, const uint32_t *src1, const uint32_t *src2, const uint32_t *src3, uint32_t xtra) {
	int i;
	for (i = 0; i < 0x200; i++) {
		uint32_t real = nva_rd32(ctx->cnum, 0x700000 + i * 4);
		uint32_t cc = nva_rd32(ctx->cnum, 0x700800 + i * 4);
		uint32_t realold = nva_rd32(ctx->cnum, 0x701000 + i * 4);
		uint32_t exp, ecc = 0xf;
		uint8_t op = (op2 >> 2) & 0xf;
		uint8_t sz = (op2 >> 21) & 7;
		uint32_t s1 = src1[i];
		uint32_t s2 = src2[i];
		uint32_t s3 = src3[i];
		uint32_t expold = s1;
		bool is_atom = (op2 >> 29) == 7;
		if (sz < 2)
			s2 = (uint8_t)s2 * 0x01010101;
		else if (sz < 4)
			s2 = (uint16_t)s2 * 0x00010001;
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
			case 4: /* inc */
				exp = s1 + 1;
				if (exp > s2)
					exp = 0;
				if (ctx->chipset.chipset < 0xa0 && sz == 7 && (int32_t)s2 < 0) {
					exp = s1 + 1;
					if ((int32_t)exp > 0 || (int32_t)s2 > (int32_t)exp)
						exp = s2;
				}
				break;
			case 5: /* dec */
				exp = s1 - 1;
				if (exp > s2)
					exp = s2;
				if (ctx->chipset.chipset < 0xa0 && sz == 7 && (int32_t)s2 < 0) {
					exp = s1 - 1;
					if ((int32_t)exp > 0 || (int32_t)s2 > (int32_t)exp)
						exp = 0;
				}
				break;
			case 6: /* max */
				if (sz == 7)
					exp = (int32_t)s1 > (int32_t)s2 ? s1 : s2;
				else
					exp = s1 > s2 ? s1 : s2;
				break;
			case 7: /* min */
				if (sz == 7)
					exp = (int32_t)s1 < (int32_t)s2 ? s1 : s2;
				else
					exp = s1 < s2 ? s1 : s2;
				break;
			case 0xa:
				exp = s1 & s2;
				break;
			case 0xb:
				exp = s1 | s2;
				break;
			case 0xc:
				exp = s1 ^ s2;
				break;
			case 0x8:
			case 0x9:
			case 0xd:
			case 0xe:
			case 0xf:
				exp = s1;
				break;
			default:
				abort();
				break;
		}
		if (sz < 4 && ctx->chipset.chipset >= 0xa0)
			exp = s1;
		switch (sz) {
			case 0:
				expold = (uint8_t)expold;
				break;
			case 1:
				expold = (int8_t)expold;
				break;
			case 2:
				expold = (uint16_t)expold;
				break;
			case 3:
				expold = (int16_t)expold;
				break;
		}
		if (real != exp || cc != ecc || (is_atom && realold != expold)) {
			printf("atom32 %08x %08x (%08x %08x %08x %08x)[%d]: got %08x.%x %08x expected %08x.%x %08x\n", op1, op2, src1[i], src2[i], src3[i], s1, i&3, real, cc, realold, exp, ecc, expold);
			return 1;
		}
	}
	return 0;
}

static int atom32_test(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2, const uint32_t *src1, const uint32_t *src2, const uint32_t *src3, uint32_t xtra) {
	atom32_write_data(ctx, src1, src2, src3);
	if (atom32_run(ctx))
		return 1;
	if (atom32_check_data(ctx, op1, op2, src1, src2, src3, xtra))
		return 1;
	return 0;
}

static void atom32_gen(struct hwtest_ctx *ctx, uint32_t *src1, uint32_t *src2, uint32_t *src3) {
	int i;
	int mode = jrand48(ctx->rand48);
	for (i = 0; i < 0x200; i++) {
		src1[i] = jrand48(ctx->rand48);
		src2[i] = jrand48(ctx->rand48);
		src3[i] = jrand48(ctx->rand48);
		/* take sources from -0x80..0x7f range 1/2 of the time. */
		if (mode & 1)
			src1[i] = (int8_t)src1[i];
		if (mode & 2)
			src2[i] = (int8_t)src2[i];
		if (mode & 4)
			src3[i] = (int8_t)src3[i];
	}
}

static int test_atom32(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xd0000015 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0xc00007c0 | (jrand48(ctx->rand48) & 0x3ffff03c);
		uint32_t xtra = jrand48(ctx->rand48);
		int op = op2 >> 2 & 0xf;
		int sz = op2 >> 21 & 7;
		if (op2 & 0x20000000) {
			/* if atom, use different operands than red */
			op1 &= ~0x7f01fc;
			op1 |= 0x05001c;
			if (op == 2) {
				/* cas selected, aim the third source */
				op2 &= ~0x001fc000;
				op2 |= 0x00018000;
				/* and make sure b32 is used */
				op2 |= 0x00c00000;
			}
		} else if (op == 2) {
			/* red cas selected, bail */
			continue;
		}
		/* bail if b64 or b128 selected */
		if (sz == 4 || sz == 5)
			continue;
		/* op 3 is a trap */
		if (op == 3)
			continue;
		if (atom32_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (atom32_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		atom32_gen(ctx, src1, src2, src3);
		if (atom32_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(g80_atom32,
	HWTEST_TEST(test_atom32, 0),
)
