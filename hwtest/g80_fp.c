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

static const uint32_t fp_fodder[0x20] = {
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
	0x7f000000,	/* add to itself to get inf or max finite */
	0x7effffff,	/* add to above to get inf or max finite */
	/* subtract these two to underflow. */
	0x01000000,
	0x00800001,
	/* some weirdness for i2f */
	0x7ffffff1,
	0x7ffffff0,
	0x7fffffef,
	0x7fffffe0,
	0x7fffffe1,
	0x8000001f,
	0x80000020,
	0x80000021,
	0xc0000021,
	0xfe000001,
	0xfc000000,
	0xfdffffff,
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
		0x00000801,
		0xa0000780,
		0xd0000011,
		0x80c00780,
		0xd0000215,
		0x80c00780,
		0xd0000419,
		0x80c00780,
		op1,
		op2,
		(op1 & 1 && (op2 & 3) != 3) ? 0xd000001d : 0xd0000019,
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

static int fp_prep_grid(struct hwtest_ctx *ctx, uint32_t xtra) {
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

static int fp_check_data(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2, const uint32_t *src1, const uint32_t *src2, const uint32_t *src3, uint32_t xtra) {
	int i;
	for (i = 0; i < 0x200; i++) {
		uint32_t real = nva_rd32(ctx->cnum, 0x700000 + i * 4);
		uint32_t cc = nva_rd32(ctx->cnum, 0x700800 + i * 4);
		uint32_t exp, ecc = 0xf;
		uint8_t op = (op1 >> 28) << 4;
		uint32_t s1 = src1[i];
		uint32_t s2 = src2[i];
		uint32_t s3 = src3[i];
		if (!(op1 & 1)) {
			op |= 8;
		} else if ((op2 & 3) == 3) {
			op |= 8;
			s2 = (op1 >> 16 & 0x3f) | (op2 << 4 & 0xffffffc0);
		} else {
			op |= op2 >> 29;
		}
		enum fp_cmp cmp;
		static const int cmpbit[4] = { 16, 15, 14, 17 };
		int swz;
		int sop;
		int rm;
		bool neg = false;
		uint64_t t64;
		switch (op) {
			case 0xa2: /* i2f */
			case 0xa3: /* i2f */
				rm = op2 >> 17 & 3;
				if (op2 & 0x20000000)
					rm = fp_flip_rm(rm);
				if (!(op2 & 0x00010000)) {
					/* zero-extend */
					if (!(op2 & 0x00004000))
						s1 = (uint16_t)s1;
					if (op2 & 0x00008000)
						s1 = (uint8_t)s1;
				} else {
					/* sign-extend */
					if (!(op2 & 0x00004000))
						s1 = (int16_t)s1;
					if (op2 & 0x00008000)
						s1 = (int8_t)s1;
					if (s1 & 0x80000000) {
						if (!(op2 & 0x00100000))
							rm = fp_flip_rm(rm);
						s1 = -s1;
						neg = true;
					}
				}
				if (!(op2 & 0x04000000)) {
					/* simulate hardware bugs. */
					if (rm == FP_RN || rm == FP_RP) {
						if ((s1 >> 5 & 0x1ffffff) == 0x1ffffff)
							s1 = 0;
					}
					uint16_t lo = s1 - neg;
					int n = 2;
					while (lo < 0x8000 && n < 7) {
						lo <<= 1;
						n++;
					}
					s1 <<= n;
					s1 >>= n;
				}
				exp = fp32_from_u64(s1, rm);
				if (neg)
					exp ^= 0x80000000;
				if (op2 & 0x00100000)
					exp &= ~0x80000000;
				if (op2 & 0x20000000)
					exp ^= 0x80000000;
				if (op2 & 0x00080000)
					exp = fp32_sat(exp);
				if (!(op2 & 0x04000000)) {
					exp = fp32_to_fp16(exp, op2 >> 17 & 3, false);
					real &= 0xffff;
					ecc = fp32_cmp(fp16_to_fp32(exp), 0);
				} else {
					ecc = fp32_cmp(exp, 0);
				}
				break;
			case 0xa4: /* f2i */
			case 0xa5: /* f2i */
				if (!(op2 & 0x00004000))
					s1 = fp16_to_fp32(s1);
				if (op2 & 0x00100000)
					s1 &= ~0x80000000;
				if (op2 & 0x20000000)
					s1 ^= 0x80000000;
				rm = op2 >> 17 & 3;
				if (s1 & 0x80000000) {
					neg = true;
					s1 ^= 0x80000000;
					rm = fp_flip_rm(rm);
				}
				t64 = fp32_to_u64(s1, rm);
				if (t64 >> 32)
					t64 = 0xffffffff;
				exp = t64;
				if (op2 & 0x08000000) {
					uint32_t limit = op2 & 0x04000000 ? 0x80000000 : 0x8000;
					if (neg) {
						if (exp > limit)
							exp = limit;
						exp = -exp;
					} else {
						if (exp >= limit)
							exp = limit - 1;
					}
				} else if (neg) {
					exp = 0;
				} else if (!(op2 & 0x04000000) && exp >= 0x10000) {
					exp = 0xffff;
				}
				if (op2 & 0x04000000) {
					if (exp & 0x80000000)
						ecc = 2;
					else if (exp == 0)
						ecc = 1;
					else
						ecc = 0;
				} else {
					exp &= 0xffff;
					real &= 0xffff;
					if (exp & 0x8000)
						ecc = 2;
					else if (exp == 0)
						ecc = 1;
					else
						ecc = 0;
				}
				break;
			case 0xa6: /* f2f */
			case 0xa7: /* f2f */
				if (!(op2 & 0x00004000))
					s1 = fp16_to_fp32(s1);
				if (op2 & 0x00100000)
					s1 &= ~0x80000000;
				if (op2 & 0x20000000)
					s1 ^= 0x80000000;
				exp = fp32_add(s1, 0x80000000, FP_RN);
				if (op2 & 0x00080000)
					exp = fp32_sat(exp);
				if (op2 & 0x08000000)
					exp = fp32_rint(exp, op2 >> 17 & 3);
				if (!(op2 & 0x04000000)) {
					exp = fp32_to_fp16(exp, op2 >> 17 & 3, op2 >> 27 & 1);
					real &= 0xffff;
					ecc = fp32_cmp(fp16_to_fp32(exp), 0);
				} else {
					ecc = fp32_cmp(exp, 0);
				}
				break;
			case 0xb0: /* fadd */
			case 0xb1: /* fadd.sat */
				if (op2 & 0x04000000)
					s1 ^= 0x80000000;
				if (op2 & 0x08000000)
					s3 ^= 0x80000000;
				exp = fp32_add(s1, s3, op1 >> 16 & 3);
				if (op2 & 0x20000000)
					exp = fp32_sat(exp);
				ecc = fp32_cmp(exp, 0);
				break;
			case 0xb3: /* fcmp */
				if (op2 & 0x00100000)
					s1 &= ~0x80000000;
				if (op2 & 0x04000000)
					s1 ^= 0x80000000;
				if (op2 & 0x00080000)
					s2 &= ~0x80000000;
				if (op2 & 0x08000000)
					s2 ^= 0x80000000;
				if (op2 >> cmpbit[fp32_cmp(s1, s2)] & 1)
					exp = 0xffffffff, ecc = 2;
				else
					exp = 0, ecc = 1;
				break;
			case 0xb4: /* fmax */
			case 0xb5: /* fmin */
				if (op2 & 0x00100000)
					s1 &= ~0x80000000;
				if (op2 & 0x04000000)
					s1 ^= 0x80000000;
				if (op2 & 0x00080000)
					s2 &= ~0x80000000;
				if (op2 & 0x08000000)
					s2 ^= 0x80000000;
				exp = fp32_minmax(s1, s2, op2 >> 29 & 1);
				ecc = fp32_cmp(exp, 0);
				break;
			case 0xc0: /* fmul */
				if (op2 & 0x04000000)
					s1 ^= 0x80000000;
				if (op2 & 0x08000000)
					s2 ^= 0x80000000;
				exp = fp32_mul(s1, s2, op2 >> 14 & 3, xtra >> 1 & 1);
				if (op2 & 0x00100000 && ctx->chipset >= 0xa0)
					exp = fp32_sat(exp);
				ecc = fp32_cmp(exp, 0);
				break;
			case 0xc2: /* fslct */
			case 0xc3: /* fslct [negated src3] */
				if (op2 & 0x20000000)
					s3 ^= 0x80000000;
				cmp = fp32_cmp(s3, 0);
				exp = (cmp == FP_GT || cmp == FP_EQ) ? s1 : s2;
				/* huh. */
				if (exp == 0 || exp == 0x80000000)
					ecc = 1;
				else if (exp >> 31)
					ecc = 2;
				else
					ecc = 0;
				break;
			case 0xc4: /* fswz */
				swz = op1 >> 16 & 7;
				switch (swz) {
					case 0:
						s1 = src1[i & ~3];
						break;
					case 1:
						s1 = src1[(i & ~3) | 1];
						break;
					case 2:
						s1 = src1[(i & ~3) | 2];
						break;
					case 3:
						s1 = src1[(i & ~3) | 3];
						break;
					case 4:
					case 6:
						s1 = src1[i ^ 1];
						break;
					case 5:
					case 7:
						s1 = src1[i ^ 2];
						break;
					default:
						abort();
				}
				switch (i & 3) {
					case 0:
						sop = op2 >> 26 & 3;
						break;
					case 1:
						sop = op2 >> 24 & 3;
						break;
					case 2:
						sop = op2 >> 22 & 3;
						break;
					case 3:
						sop = op1 >> 20 & 3;
						break;
					default:
						abort();
				}
				switch (sop) {
					case 0:
						exp = fp32_add(s1, s3, FP_RN);
						break;
					case 1:
						exp = fp32_add(s1 ^ 0x80000000, s3, FP_RN);
						break;
					case 2:
						exp = fp32_add(s1, s3 ^ 0x80000000, FP_RN);
						break;
					case 3:
						exp = fp32_add(s3, 0x00000000, FP_RN);
						break;
				}
				if (swz & 4) {
					/* XXX: test it in non-CP modes */
					exp = (xtra & 1) ? 0x7f800000 : 0;
				}
				ecc = fp32_cmp(exp, 0);
				break;
			case 0xe0: /* fmad */
			case 0xe1: /* fmad.sat */
				if (op2 & 0x04000000)
					s1 ^= 0x80000000;
				if (op2 & 0x08000000)
					s3 ^= 0x80000000;
				exp = fp32_mad(s1, s2, s3, xtra >> 1 & 1);
				if (op2 & 0x20000000)
					exp = fp32_sat(exp);
				ecc = fp32_cmp(exp, 0);
				break;
			case 0xb8: /* fadd short */
				if (op1 & 0x00008000)
					s1 ^= 0x80000000;
				if (op1 & 0x00400000)
					s2 ^= 0x80000000;
				exp = fp32_add(s1, s2, FP_RN);
				if (op1 & 0x00000100)
					exp = fp32_sat(exp);
				ecc = 0xf;
				break;
			case 0xc8: /* fmul short */
				if (op1 & 0x00008000)
					s1 ^= 0x80000000;
				if (op1 & 0x00400000)
					s2 ^= 0x80000000;
				exp = fp32_mul(s1, s2, FP_RN, xtra >> 1 & 1);
				if (op1 & 0x00000100 && ctx->chipset >= 0xa0)
					exp = fp32_sat(exp);
				ecc = 0xf;
				break;
			case 0xe8: /* fmad short */
				if (op1 & 0x00008000)
					s1 ^= 0x80000000;
				if (op1 & 0x00400000)
					s3 ^= 0x80000000;
				exp = fp32_mad(s1, s2, s3, xtra >> 1 & 1);
				if (op1 & 0x00000100)
					exp = fp32_sat(exp);
				ecc = 0xf;
				break;
			default:
				abort();
		}
		if (real != exp || cc != ecc) {
			printf("fp %08x %08x (%08x %08x %08x %08x)[%d]: got %08x.%x expected %08x.%x diff %d\n", op1, op2, src1[i], src2[i], src3[i], s1, i&3, real, cc, exp, ecc, real-exp);
			return 1;
		}
	}
	return 0;
}

static int fp_test(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2, const uint32_t *src1, const uint32_t *src2, const uint32_t *src3, uint32_t xtra) {
	fp_write_data(ctx, src1, src2, src3);
	if (fp_run(ctx))
		return 1;
	if (fp_check_data(ctx, op1, op2, src1, src2, src3, xtra))
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
			src1[i] = fp_fodder[jrand48(ctx->rand48) & 0x1f];
		if (mode & 0xc)
			src2[i] = jrand48(ctx->rand48);
		else
			src2[i] = fp_fodder[jrand48(ctx->rand48) & 0x1f];
		if (mode & 0x30)
			src3[i] = jrand48(ctx->rand48);
		else
			src3[i] = fp_fodder[jrand48(ctx->rand48) & 0x1f];
	}
}

static int test_fadd(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xb000081d | (jrand48(ctx->rand48) & 0x0e7d0000);
		uint32_t op2 = 0x000187c0 | (jrand48(ctx->rand48) & 0x3fc03004);
		/* Ensure valid rounding mode */
		if (op1 & 0x10000)
			op1 |= 0x20000;
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_fcmp(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xb005081d | (jrand48(ctx->rand48) & 0x0e000000);
		uint32_t op2 = 0x600007c0 | (jrand48(ctx->rand48) & 0x1fdff004);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_fminmax(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xb005081d | (jrand48(ctx->rand48) & 0x0e000000);
		uint32_t op2 = 0x800007c0 | (jrand48(ctx->rand48) & 0x3fdff004);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_fmul(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xc005081d | (jrand48(ctx->rand48) & 0x0e000000);
		uint32_t op2 = 0x000007c0 | (jrand48(ctx->rand48) & 0x1fdf7004);
		/* Ensure valid rounding mode */
		if (op2 & 0x4000)
			op2 |= 0x8000;
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_fslct(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xc005081d | (jrand48(ctx->rand48) & 0x0e000000);
		uint32_t op2 = 0x400187c0 | (jrand48(ctx->rand48) & 0x3fc03004);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_fswz(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xc000081d | (jrand48(ctx->rand48) & 0x0eff0000);
		uint32_t op2 = 0x800187c0 | (jrand48(ctx->rand48) & 0x1fc03004);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_fmad(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xe005081d | (jrand48(ctx->rand48) & 0x0e000000);
		uint32_t op2 = 0x000187c0 | (jrand48(ctx->rand48) & 0x3fc00004);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_fadd_s(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xb0050818 | (jrand48(ctx->rand48) & 0x0e408100);
		uint32_t op2 = 0x10000000;
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_fmul_s(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xc0050818 | (jrand48(ctx->rand48) & 0x0e408100);
		uint32_t op2 = 0x10000000;
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_fmad_s(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xe0050818 | (jrand48(ctx->rand48) & 0x0e408100);
		uint32_t op2 = 0x10000000;
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_fadd_i(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xb0050819 | (jrand48(ctx->rand48) & 0x0e7f8100);
		uint32_t op2 = 0x00000003 | (jrand48(ctx->rand48) & 0x1ffffffc);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_fmul_i(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xc0050819 | (jrand48(ctx->rand48) & 0x0e7f8100);
		uint32_t op2 = 0x00000003 | (jrand48(ctx->rand48) & 0x1ffffffc);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_fmad_i(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xe0050819 | (jrand48(ctx->rand48) & 0x0e7f8100);
		uint32_t op2 = 0x00000003 | (jrand48(ctx->rand48) & 0x1ffffffc);
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_i2f(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xa0000001 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0x400007c0 | (jrand48(ctx->rand48) & 0x3fdff004);
		if (jrand48(ctx->rand48) & 1) {
			/* i2f x32 -> f16 is ridiculously buggy, exercise it more. */
			op2 &= ~0x04008000;
			op2 |=  0x00004000;
		}
		/* Select proper source/destination for fp32 vs fp16 */
		if (op2 & 0x04000000)
			op1 |= 0x1c;
		else
			op1 |= 0x38;
		if (op2 & 0x00004000)
			op1 |= 0x800;
		else
			op1 |= 0x1000;
		/* This bit selects fp64 on G200+ */
		if (ctx->chipset >= 0xa0)
			op2 &= ~0x00400000;
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_f2i(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xa0000001 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0x800007c0 | (jrand48(ctx->rand48) & 0x3fdff004);
		/* Select proper source/destination for fp32 vs fp16 */
		if (op2 & 0x04000000)
			op1 |= 0x1c;
		else
			op1 |= 0x38;
		if (op2 & 0x00004000)
			op1 |= 0x800;
		else
			op1 |= 0x1000;
		/* This bit selects fp64 on G200+ */
		if (ctx->chipset >= 0xa0)
			op2 &= ~0x00400000;
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_f2f(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xa0000001 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0xc00007c0 | (jrand48(ctx->rand48) & 0x3fdff004);
		/* Select proper source/destination for fp32 vs fp16 */
		if (op2 & 0x04000000)
			op1 |= 0x1c;
		else
			op1 |= 0x38;
		if (op2 & 0x00004000)
			op1 |= 0x800;
		else
			op1 |= 0x1000;
		/* This bit selects fp64 on G200+ */
		if (ctx->chipset >= 0xa0)
			op2 &= ~0x00400000;
		uint32_t xtra = jrand48(ctx->rand48);
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		fp_gen(ctx, src1, src2, src3);
		if (fp_test(ctx, op1, op2, src1, src2, src3, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(g80_fp,
	HWTEST_TEST(test_fadd, 0),
	HWTEST_TEST(test_fcmp, 0),
	HWTEST_TEST(test_fminmax, 0),
	HWTEST_TEST(test_fmul, 0),
	HWTEST_TEST(test_fslct, 0),
	HWTEST_TEST(test_fswz, 0),
	HWTEST_TEST(test_fmad, 0),
	HWTEST_TEST(test_fadd_s, 0),
	HWTEST_TEST(test_fmul_s, 0),
	HWTEST_TEST(test_fmad_s, 0),
	HWTEST_TEST(test_fadd_i, 0),
	HWTEST_TEST(test_fmul_i, 0),
	HWTEST_TEST(test_fmad_i, 0),
	HWTEST_TEST(test_i2f, 0),
	HWTEST_TEST(test_f2i, 0),
	HWTEST_TEST(test_f2f, 0),
)
