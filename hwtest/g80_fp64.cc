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

static const uint64_t fp64_fodder[0x20] = {
	/* Various numbers that attempt to trigger special cases. */
	0x0000000000000000ull,	/* +0 */
	0x0000000000000001ull,	/* denormal */
	0x000fffffffffffffull,	/* denormal */
	0x8000000000000000ull,	/* -0 */
	0x8000000000000001ull,	/* denormal */
	0x800fffffffffffffull,	/* denormal */
	0x7fffffffffffffffull,	/* canonical NaN */
	0xffffffffffffffffull,	/* negated canonical NaN */
	0x7ff00aaa00000000ull,	/* funny NaN */
	0x7ff123456789abcdull,	/* another funny NaN */
	0xfff00bbb00000000ull,	/* negative funny NaN */
	0x7ff0000000000000ull,	/* +Inf */
	0xfff0000000000000ull,	/* -Inf */
	/* (1.0+eps)*(1.0+eps)-1.0 reveals fma vs separate mul-add */
	0x3ff0000000000000ull,	/* 1.0 */
	0x3ff0000000000001ull,	/* 1.0 and a bit */
	0xbff0000000000000ull,	/* -1.0 */
	0x7fe0000000000000ull,	/* add to itself to get inf or max finite */
	0x7fdfffffffffffffull,	/* add to above to get inf or max finite */
	/* subtract these two to underflow. */
	0x0020000000000000ull,
	0x0010000000000001ull,
	/* integers */
	0x0000000000000002ull,
	0x0000000000000123ull,
	0xffffffffffffffffull,
	0xffffffffffffff12ull,
	0xfffffffffffff123ull,
	0xffffffffffff1234ull,
	0x0000000000001234ull,
	/* will give a fp32 denormal */
	0x1dd0123456789abcull,
	/* place for 4 more */
};

static int g80_fp64_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset != 0xa0)
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

static int fp64_prep_code(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2) {
	uint32_t code[] = {
/* XXX */
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
0xd0000011,
0x80800780,
0xd0000219,
0x80800780,
0xd0000421,
0x80800780,
0xd0000629,
0x80800780,
op1,
op2,
0xd0000029,
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

static int fp64_prep_grid(struct hwtest_ctx *ctx, uint32_t xtra) {
	if (g80_gr_idle(ctx))
		return 1;
	uint32_t units = nva_rd32(ctx->cnum, 0x1540);
	int tpc;
	int mp;
	for (tpc = 0; tpc < 16; tpc++) if (units & 1 << tpc)
		for (mp = 0; mp < 4; mp++) if (units & 1 << (mp + 24)) {
			uint32_t base;
			if (ctx->chipset.chipset >= 0xa0) {
				base = 0x408100 + tpc * 0x800 + mp * 0x80;
			} else {
				base = 0x408200 + tpc * 0x1000 + mp * 0x80;
			}
			nva_wr32(ctx->cnum, base+0x60, xtra);
			nva_wr32(ctx->cnum, base+0x64, 0);
		}
	return
		/* CTA config. */
		g80_gr_mthd(ctx, 3, 0x3a8, 0x40) ||
		g80_gr_mthd(ctx, 3, 0x3ac, 0x00010200) ||
		g80_gr_mthd(ctx, 3, 0x3b0, 0x00000001) ||
		g80_gr_mthd(ctx, 3, 0x2c0, 0x0000000c) || /* regs */
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

static void fp64_write_data(struct hwtest_ctx *ctx, const uint64_t *src1, const uint64_t *src2, const uint64_t *src3) {
	int i;
	nva_wr32(ctx->cnum, 0x1700, 0x200);
	for (i = 0; i < 0x200; i++) {
		nva_wr32(ctx->cnum, 0x700000 + i * 8, src1[i]);
		nva_wr32(ctx->cnum, 0x700004 + i * 8, src1[i] >> 32);
		nva_wr32(ctx->cnum, 0x701000 + i * 8, src2[i]);
		nva_wr32(ctx->cnum, 0x701004 + i * 8, src2[i] >> 32);
		nva_wr32(ctx->cnum, 0x702000 + i * 8, src3[i]);
		nva_wr32(ctx->cnum, 0x702004 + i * 8, src3[i] >> 32);
		nva_wr32(ctx->cnum, 0x703000 + i * 8, 0xcccccccc);
		nva_wr32(ctx->cnum, 0x703004 + i * 8, 0xdeadbeef);
	}
	nva_wr32(ctx->cnum, 0x70000, 1);
	while (nva_rd32(ctx->cnum, 0x70000));
}

static int fp64_run(struct hwtest_ctx *ctx) {
	return g80_gr_mthd(ctx, 3, 0x368, 0) || g80_gr_idle(ctx);
}

static int fp64_check_data(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2, const uint64_t *src1, const uint64_t *src2, const uint64_t *src3, uint32_t xtra) {
	int i;
	for (i = 0; i < 0x200; i++) {
		uint64_t real = nva_rd32(ctx->cnum, 0x700000 + i * 8);
		real |= (uint64_t)nva_rd32(ctx->cnum, 0x700004 + i * 8) << 32;
		uint32_t cc = nva_rd32(ctx->cnum, 0x701000 + i * 8);
		uint64_t exp;
		uint32_t ecc = 0xf;
		uint8_t op = (op1 >> 28) << 4 | op2 >> 29;
		uint64_t s1 = src1[i];
		uint64_t s2 = src2[i];
		uint64_t s3 = src3[i];
		static const int cmpbit[4] = { 16, 15, 14, 17 };
		enum fp_rm rm;
		bool neg = false;
		switch (op) {
			case 0xa2: /* i2f */
			case 0xa3: /* i2f */
				{
					rm = (enum fp_rm)(op2 >> 17 & 3);
					if (op2 >> 16 & 1) {
						if (!(op2 >> 14 & 1))
							s1 = (int32_t)s1;
						if (s1 & 1ull << 63) {
							s1 = -s1;
							neg = true;
						}
					} else {
						if (!(op2 >> 14 & 1))
							s1 = (uint32_t)s1;
					}
					if (op2 & 0x00100000)
						neg = false;
					neg ^= op2 >> 29 & 1;
					if (op2 >> 16 & 1)
						rm = fp_adjust_rm(rm, neg);
					if (op2 & 0x04000000) {
						exp = fp64_from_u64(s1, rm);
						if (neg)
							exp ^= 1ull << 63;
						ecc = fp64_cmp(exp, 0);
					} else {
						exp = fp32_from_u64(s1, rm);
						if (neg)
							exp ^= 1ull << 31;
						ecc = fp32_cmp(exp, 0, false);
						real &= 0xffffffff;
					}
					break;
				}
			case 0xa4: /* f2i */
			case 0xa5: /* f2i */
				{
					rm = (enum fp_rm)(op2 >> 17 & 3);
					int sbit = op2 >> 14 & 1 ? 63 : 31;
					if (op2 >> 20 & 1)
						s1 &= ~(1ull << sbit);
					if (op2 >> 29 & 1)
						s1 ^= 1ull << sbit;
					bool neg = s1 >> sbit & 1;
					bool ovf = false;
					if (neg) {
						s1 ^= 1ull << sbit;
						rm = fp_adjust_rm(rm, true);
					}
					if (op2 >> 14 & 1 ? FP64_ISNAN(s1) : FP32_ISNAN(s1)) {
						exp = 1ull << (op2 >> 26 & 1 ? 63 : 31);
						ovf = true;
					} else {
						if (op2 >> 14 & 1)
							exp = fp64_to_u64(s1, rm);
						else
							exp = fp32_to_u64(s1, rm, false);
						if (exp == -1u)
							ovf = true;
						if (op2 >> 27 & 1) {
							uint64_t limit = 1ull << 63;
							if (!(op2 >> 26 & 1))
								limit = 1ull << 31;
							if (neg) {
								if (exp > limit) {
									exp = limit;
									ovf = true;
								}
								exp = -exp;
							} else {
								if (exp >= limit) {
									exp = limit - 1;
									ovf = true;
								}
							}
						} else {
							if (neg && exp) {
								exp = 0;
								ovf = true;
							}
							if (!(op2 >> 26 & 1)) {
								if (exp >= 1ull << 32) {
									exp = 0xffffffff;
									ovf = true;
								}
							}
						}
					}
					if (!(op2 >> 26 & 1)) {
						real &= 0xffffffff;
						exp &= 0xffffffff;
					}
					if (exp == 0) {
						ecc = 1;
					} else if (exp & 1ull << (op2 >> 26 & 1 ? 63 : 31)) {
						ecc = 2;
					} else {
						ecc = 0;
					}
					ecc |= ovf << 3;
					break;
				}
			case 0xa6: /* f2f */
			case 0xa7: /* f2f */
				{
					rm = (enum fp_rm)(op2 >> 17 & 3);
					if (op2 >> 14 & 1) {
						exp = s1;
					} else {
						exp = fp32_to_fp64(s1);
					}
					if (FP64_ISNAN(exp)) {
						exp |= 1ull << 51;
					} else {
						if (op2 >> 20 & 1)
							exp &= ~(1ull << 63);
						if (op2 >> 29 & 1)
							exp ^= 1ull << 63;
					}
					if (op2 >> 27 & 1 && op2 >> 26 & 1 && op2 >> 14 & 1)
						exp = fp64_rint(exp, rm);
					if (op2 >> 26 & 1) {
						ecc = fp64_cmp(exp, 0);
					} else {
						exp = fp64_to_fp32(exp, rm, false);
						ecc = fp32_cmp(exp, 0, false);
						real &= 0xffffffff;
					}
					break;
				}
			case 0xe2: /* dfma */
				if (FP64_ISNAN(s1)) {
					s1 |= 1ull << 51;
				} else {
					if (op2 & 0x04000000)
						s1 ^= (1ull << 63);
				}
				if (FP64_ISNAN(s2)) {
					s2 |= 1ull << 51;
				}
				if (FP64_ISNAN(s3)) {
					s3 |= 1ull << 51;
				} else {
					if (op2 & 0x08000000)
						s3 ^= (1ull << 63);
				}
				exp = fp64_fma(s1, s2, s3, (enum fp_rm)(op2 >> 22 & 3));
				ecc = fp64_cmp(exp, 0);
				break;
			case 0xe3: /* dadd */
				if (FP64_ISNAN(s1)) {
					s1 |= 1ull << 51;
				} else {
					if (op2 & 0x04000000)
						s1 ^= (1ull << 63);
				}
				if (FP64_ISNAN(s3)) {
					s3 |= 1ull << 51;
				} else {
					if (op2 & 0x08000000)
						s3 ^= (1ull << 63);
				}
				exp = fp64_add(s1, s3, (enum fp_rm)(op1 >> 16 & 3));
				ecc = fp64_cmp(exp, 0);
				break;
			case 0xe4: /* dmul */
				if (FP64_ISNAN(s1)) {
					s1 |= 1ull << 51;
				} else {
					if (op2 & 0x04000000)
						s1 ^= (1ull << 63);
				}
				if (FP64_ISNAN(s2)) {
					s2 |= 1ull << 51;
				}
				exp = fp64_mul(s1, s2, (enum fp_rm)(op2 >> 17 & 3));
				ecc = fp64_cmp(exp, 0);
				break;
			case 0xe5: /* dmin */
			case 0xe6: /* dmax */
				if (FP64_ISNAN(s1)) {
					s1 |= 1ull << 51;
				} else {
					if (op2 & 0x00100000)
						s1 &= ~(1ull << 63);
					if (op2 & 0x04000000)
						s1 ^= (1ull << 63);
				}
				if (FP64_ISNAN(s2)) {
					s2 |= 1ull << 51;
				} else {
					if (op2 & 0x00080000)
						s2 &= ~(1ull << 63);
					if (op2 & 0x08000000)
						s2 ^= (1ull << 63);
				}
				exp = fp64_minmax(s1, s2, op2 >> 29 & 1);
				ecc = fp64_cmp(exp, 0);
				break;
			case 0xe7: /* dcmp */
				if (op2 & 0x00100000)
					s1 &= ~(1ull << 63);
				if (op2 & 0x04000000)
					s1 ^= (1ull << 63);
				if (op2 & 0x00080000)
					s2 &= ~(1ull << 63);
				if (op2 & 0x08000000)
					s2 ^= (1ull << 63);
				if (op2 >> cmpbit[fp64_cmp(s1, s2)] & 1)
					exp = 0xffffffff, ecc = 2;
				else
					exp = 0, ecc = 1;
				real &= 0xffffffff;
				break;
			default:
				abort();
		}
		if (real != exp || cc != ecc) {
			printf("fp64 %08x %08x (%016" PRIx64 " %016" PRIx64 " %016" PRIx64 "): got %016" PRIx64 ".%x expected %016" PRIx64 ".%x diff %" PRId64 "\n", op1, op2, src1[i], src2[i], src3[i], real, cc, exp, ecc, real-exp);
			return 1;
		}
	}
	return 0;
}

static int fp64_test(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2, const uint64_t *src1, const uint64_t *src2, const uint64_t *src3, uint32_t xtra) {
	fp64_write_data(ctx, src1, src2, src3);
	if (fp64_run(ctx))
		return 1;
	if (fp64_check_data(ctx, op1, op2, src1, src2, src3, xtra))
		return 1;
	return 0;
}

static void fp64_gen(struct hwtest_ctx *ctx, uint64_t *src1, uint64_t *src2, uint64_t *src3) {
	int i;
	int mode = jrand48(ctx->rand48);
	for (i = 0; i < 0x200; i++) {
		/* take sources from our toolbox 1/4 of the time */
		if (mode & 0x3)
			src1[i] = (uint32_t)jrand48(ctx->rand48) | (uint64_t)jrand48(ctx->rand48) << 32;
		else
			src1[i] = fp64_fodder[jrand48(ctx->rand48) & 0x1f];
		if (mode & 0xc)
			src2[i] = (uint32_t)jrand48(ctx->rand48) | (uint64_t)jrand48(ctx->rand48) << 32;
		else
			src2[i] = fp64_fodder[jrand48(ctx->rand48) & 0x1f];
		if (mode & 0x30)
			src3[i] = (uint32_t)jrand48(ctx->rand48) | (uint64_t)jrand48(ctx->rand48) << 32;
		else
			src3[i] = fp64_fodder[jrand48(ctx->rand48) & 0x1f];
	}
}

static int test_dfma(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xe0060829 | (jrand48(ctx->rand48) & 0x0e000000);
		uint32_t op2 = 0x400207c0 | (jrand48(ctx->rand48) & 0x1fc03004);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp64_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp64_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		uint64_t src1[0x200], src2[0x200], src3[0x200];
		fp64_gen(ctx, src1, src2, src3);
		if (fp64_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_dadd(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xe0000829 | (jrand48(ctx->rand48) & 0x0e7f0000);
		uint32_t op2 = 0x600207c0 | (jrand48(ctx->rand48) & 0x1fc03004);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp64_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp64_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		uint64_t src1[0x200], src2[0x200], src3[0x200];
		fp64_gen(ctx, src1, src2, src3);
		if (fp64_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_dmul(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xe0060829 | (jrand48(ctx->rand48) & 0x0e000000);
		uint32_t op2 = 0x800007c0 | (jrand48(ctx->rand48) & 0x1fdff004);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp64_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp64_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		uint64_t src1[0x200], src2[0x200], src3[0x200];
		fp64_gen(ctx, src1, src2, src3);
		if (fp64_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_dmin(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xe0060829 | (jrand48(ctx->rand48) & 0x0e000000);
		uint32_t op2 = 0xa00007c0 | (jrand48(ctx->rand48) & 0x1fdff004);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp64_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp64_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		uint64_t src1[0x200], src2[0x200], src3[0x200];
		fp64_gen(ctx, src1, src2, src3);
		if (fp64_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_dmax(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xe0060829 | (jrand48(ctx->rand48) & 0x0e000000);
		uint32_t op2 = 0xc00007c0 | (jrand48(ctx->rand48) & 0x1fdff004);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp64_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp64_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		uint64_t src1[0x200], src2[0x200], src3[0x200];
		fp64_gen(ctx, src1, src2, src3);
		if (fp64_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_dcmp(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xe0060829 | (jrand48(ctx->rand48) & 0x0e000000);
		uint32_t op2 = 0xe00007c0 | (jrand48(ctx->rand48) & 0x1fdff004);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp64_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp64_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		uint64_t src1[0x200], src2[0x200], src3[0x200];
		fp64_gen(ctx, src1, src2, src3);
		if (fp64_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_i2f64(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xa0000829 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0x404007c0 | (jrand48(ctx->rand48) & 0x3fdff004);
		/* Avoid crap_i2f64 encoding (tested in g80_fp.c) */
		if (!(op2 & 0x04004000))
			op2 |= 0x04000000;
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp64_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp64_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		uint64_t src1[0x200], src2[0x200], src3[0x200];
		fp64_gen(ctx, src1, src2, src3);
		if (fp64_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_f2i64(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xa0000829 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0x804007c0 | (jrand48(ctx->rand48) & 0x3fdff004);
		uint32_t xtra = jrand48(ctx->rand48);
		/* Avoid crap_f2i64 encoding (tested in g80_fp.c) */
		if (!(op2 & 0x04004000))
			op2 |= 0x04000000;
		if (fp64_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp64_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		uint64_t src1[0x200], src2[0x200], src3[0x200];
		fp64_gen(ctx, src1, src2, src3);
		if (fp64_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_f2f64(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xa0000829 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0xc04007c0 | (jrand48(ctx->rand48) & 0x3fdff004);
		uint32_t xtra = jrand48(ctx->rand48);
		/* Avoid crap_f2f64 encoding (tested in g80_fp.c) */
		if (!(op2 & 0x04004000))
			op2 |= 0x04000000;
		if (fp64_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp64_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		uint64_t src1[0x200], src2[0x200], src3[0x200];
		fp64_gen(ctx, src1, src2, src3);
		if (fp64_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(g80_fp64,
	HWTEST_TEST(test_dfma, 0),
	HWTEST_TEST(test_dadd, 0),
	HWTEST_TEST(test_dmul, 0),
	HWTEST_TEST(test_dmin, 0),
	HWTEST_TEST(test_dmax, 0),
	HWTEST_TEST(test_dcmp, 0),
	HWTEST_TEST(test_i2f64, 0),
	HWTEST_TEST(test_f2i64, 0),
	HWTEST_TEST(test_f2f64, 0),
)
