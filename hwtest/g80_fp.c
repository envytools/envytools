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
#include "nvhw/fp.h"

static const uint32_t fp_fodder[0x10] = {
	/* Various numbers that attempt to trigger special cases. */
	0x00000000,	/* +0 */
	0x00000001,	/* denormal (treated as +0) */
	0x007fffff,	/* denormal (treated as +0) */
	0x80000000,	/* -0 */
	0x80000001,	/* denormal (treated as -0) */
	0x807fffff,	/* denormal (treated as -0) */
	0x7fffffff,	/* canonical NaN */
	0xffffffff,	/* negated canonical NaN */
	0x7f800aaa,	/* funny NaN */
	0x7f812345,	/* another funny NaN */
	0xff800bbb,	/* negative funny NaN */
	0x7f800000,	/* +Inf */
	0xff800000,	/* -Inf */
	/* (1.0+eps)*(1.0+eps)-1.0 reveals fma vs separate mul-add */
	0x3f800000,	/* 1.0 */
	0x3f800001,	/* 1.0 and a bit */
	0xbf800000,	/* -1.0 */
};

static int g80_fp_prep(struct hwtest_ctx *ctx) {
	if (ctx->card_type != 0x50)
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

static int fp_prep_code(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2) {
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
		0x00000401,
		0xa0000780,
		0xd0000011,
		0x80c00780,
		0xd0000215,
		0x80c00780,
		0xd0000419,
		0x80c00780,
		op1,
		op2,
		0xd000001d,
		0xa0c00780,
		0x0000001d,
		0x20000780,
		0xd000021d,
		0xa0c00780,
		0xf0000001,
		0xe0000781,
	};
	int i;
	/* Poke code and flush it. */
	nva_wr32(ctx->cnum, 0x1700, 0x100);
	for (i = 0; i < ARRAY_SIZE(code); i++)
		nva_wr32(ctx->cnum, 0x730000 + i * 4, code[i]);
	nva_wr32(ctx->cnum, 0x70000, 1);
	while (nva_rd32(ctx->cnum, 0x70000));
	return g80_gr_mthd(ctx, 3, 0x380, 0);
}

static int fp_prep_grid(struct hwtest_ctx *ctx) {
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
		g80_gr_mthd(ctx, 3, 0x384, 0x100);
}

static void fp_write_data(struct hwtest_ctx *ctx, const uint32_t *src1, const uint32_t *src2, const uint32_t *src3) {
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

static int fp_run(struct hwtest_ctx *ctx) {
	return g80_gr_mthd(ctx, 3, 0x368, 0) || g80_gr_idle(ctx);
}

static int fp_check_data(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2, const uint32_t *src1, const uint32_t *src2, const uint32_t *src3) {
	int i;
	for (i = 0; i < 0x200; i++) {
		uint32_t real = nva_rd32(ctx->cnum, 0x700000 + i * 4);
		uint32_t cc = nva_rd32(ctx->cnum, 0x700800 + i * 4);
		uint32_t exp, ecc = 0xf;
		uint8_t op = (op1 >> 28) << 4 | (op2 >> 28 & 0xe) | (op1 & 1);
		uint32_t s1 = src1[i];
		uint32_t s2 = src2[i];
		switch (op) {
			case 0xb9: /* fmax */
			case 0xbb: /* fmin */
				if (op2 & 0x00100000)
					s1 &= ~0x80000000;
				if (op2 & 0x04000000)
					s1 ^= 0x80000000;
				if (op2 & 0x00080000)
					s2 &= ~0x80000000;
				if (op2 & 0x08000000)
					s2 ^= 0x80000000;
				exp = fp32_minmax(s1, s2, op2 >> 29 & 1);
				ecc = g80_fp32_cc(exp);
				break;
			default:
				abort();
		}
		if (real != exp || cc != ecc) {
			printf("sfu %08x %08x (%08x %08x %08x): got %08x.%x expected %08x.%x diff %d\n", op1, op2, src1[i], src2[i], src3[i], real, cc, exp, ecc, real-exp);
			return 1;
		}
	}
	return 0;
}

static int fp_test(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2, const uint32_t *src1, const uint32_t *src2, const uint32_t *src3) {
	fp_write_data(ctx, src1, src2, src3);
	if (fp_run(ctx))
		return 1;
	if (fp_check_data(ctx, op1, op2, src1, src2, src3))
		return 1;
	return 0;
}

static void fp_gen(struct hwtest_ctx *ctx, uint32_t *src1, uint32_t *src2, uint32_t *src3) {
	int i;
	int mode = jrand48(ctx->rand48);
	for (i = 0; i < 0x200; i++) {
		/* take sources from our toolbox 1/4 of the time */
		if (mode & 0x3)
			src1[i] = jrand48(ctx->rand48);
		else
			src1[i] = fp_fodder[jrand48(ctx->rand48) & 0xf];
		if (mode & 0xc)
			src2[i] = jrand48(ctx->rand48);
		else
			src2[i] = fp_fodder[jrand48(ctx->rand48) & 0xf];
		if (mode & 0x30)
			src3[i] = jrand48(ctx->rand48);
		else
			src3[i] = fp_fodder[jrand48(ctx->rand48) & 0xf];
	}
}

static int test_fminmax(struct hwtest_ctx *ctx) {
	int i;
	if (fp_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xb005081d | (jrand48(ctx->rand48) & 0x0e000000);
		uint32_t op2 = 0x800007c0 | (jrand48(ctx->rand48) & 0x3fdff004);
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(g80_fp,
	HWTEST_TEST(test_fminmax, 0),
)
