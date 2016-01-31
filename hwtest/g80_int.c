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

static int g80_int_prep(struct hwtest_ctx *ctx) {
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
		0xa0000790,
		0xd0000011,
		0x80c00780,
		0xd0000215,
		0x80c00780,
		0xd0000419,
		0x80c00780,
		0xd000061d,
		0x80c00780,
		0x00000e01,
		0xa0000780,
		op1,
		op2,
		(op1 & 1 && (op2 & 3) != 3) ? 0xd000001d : 0xd0000019,
		0xa0c00780,
		0x0000001d,
		0x20001780,
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
	if (g80_gr_idle(ctx))
		return 1;
	uint32_t units = nva_rd32(ctx->cnum, 0x1540);
	int tpc;
	int mp;
	for (tpc = 0; tpc < 16; tpc++) if (units & 1 << tpc)
		for (mp = 0; mp < 4; mp++) if (units & 1 << (mp + 24)) {
			uint32_t base;
			if (ctx->chipset >= 0xa0) {
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

static void fp_write_data(struct hwtest_ctx *ctx, const uint32_t *src1, const uint32_t *src2, const uint32_t *src3, uint8_t *cc) {
	int i;
	nva_wr32(ctx->cnum, 0x1700, 0x200);
	for (i = 0; i < 0x200; i++) {
		nva_wr32(ctx->cnum, 0x700000 + i * 4, src1[i]);
		nva_wr32(ctx->cnum, 0x700800 + i * 4, src2[i]);
		nva_wr32(ctx->cnum, 0x701000 + i * 4, src3[i]);
		nva_wr32(ctx->cnum, 0x701800 + i * 4, cc[i]);
	}
	nva_wr32(ctx->cnum, 0x70000, 1);
	while (nva_rd32(ctx->cnum, 0x70000));
}

static int fp_run(struct hwtest_ctx *ctx) {
	return g80_gr_mthd(ctx, 3, 0x368, 0) || g80_gr_idle(ctx);
}

static uint32_t g80_add(bool b32, int sop, bool sat, uint32_t s1, uint32_t s3, uint8_t scc, uint8_t *pecc) {
	bool xbit;
	if (sop == 0) {
		/* add */
		xbit = false;
	} else if (sop == 1) {
		/* sub */
		s3 = ~s3;
		xbit = true;
	} else if (sop == 2) {
		/* subr */
		s1 = ~s1;
		xbit = true;
	} else if (sop == 3) {
		/* addc */
		xbit = (scc >> 2 & 1);
	} else {
		abort();
	}
	if (!b32) {
		s1 &= 0xffff;
		s3 &= 0xffff;
	}
	uint32_t exp = s1 + s3 + xbit;
	uint8_t ecc = 0;
	if (!b32) {
		if (exp & 0x10000)
			ecc |= 4;
		exp &= 0xffff;
	} else {
		if (exp <= s1 && exp <= s3 && (s1 || s3))
			ecc |= 4;
	}
	int sbit = b32 ? 31 : 15;
	if (s1 >> sbit == s3 >> sbit && s1 >> sbit != exp >> sbit) {
		ecc |= 8;
	}
	if (sat && ecc & 8) {
		/* saturate */
		if (s1 >> sbit & 1)
			exp = 1 << sbit;
		else
			exp = (1 << sbit) - 1;
	}
	if (exp >> sbit)
		ecc |= 2;
	if (!exp)
		ecc |= 1;
	if (pecc)
		*pecc = ecc;
	return exp;
}

static int fp_check_data(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2, const uint32_t *src1, const uint32_t *src2, const uint32_t *src3, uint8_t *cc, uint32_t xtra) {
	int i;
	for (i = 0; i < 0x200; i++) {
		uint32_t real = nva_rd32(ctx->cnum, 0x700000 + i * 4);
		uint32_t rcc = nva_rd32(ctx->cnum, 0x700800 + i * 4);
		uint32_t exp;
		uint8_t ecc = 0xf;
		uint8_t op = (op1 >> 28) << 4;
		uint32_t s1 = src1[i];
		uint32_t s2 = src2[i];
		uint32_t s3 = src3[i];
		uint8_t scc = cc[i] & 0xf;
		if (!(op1 & 1)) {
			op |= 8;
		} else if ((op2 & 3) == 3) {
			op |= 8;
			s2 = (op1 >> 16 & 0x3f) | (op2 << 4 & 0xffffffc0);
		} else {
			op |= op2 >> 29;
		}
		int sop;
		switch (op) {
			case 0x20: /* add */
			case 0x30: /* add */
				sop = (op1 >> 22 & 1) | (op1 >> 27 & 2);
				exp = g80_add(op2 >> 26 & 1, sop, op2 >> 27 & 1, s1, s3, scc, &ecc);
				if (!(op2 >> 26 & 1))
					real &= 0xffff;
				break;
			case 0x28: /* add */
			case 0x38: /* add */
				sop = (op1 >> 22 & 1) | (op1 >> 27 & 2);
				exp = g80_add(op1 >> 15 & 1, sop, op1 >> 8 & 1, s1, s2, scc, 0);
				if (!(op1 >> 15 & 1))
					real &= 0xffff;
				break;
			default:
				abort();
		}
		if (real != exp || rcc != ecc) {
			printf("fp %08x %08x (%08x %08x %08x %x): got %08x.%x expected %08x.%x diff %d\n", op1, op2, src1[i], src2[i], src3[i], scc, real, rcc, exp, ecc, real-exp);
			return 1;
		}
	}
	return 0;
}

static int fp_test(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2, const uint32_t *src1, const uint32_t *src2, const uint32_t *src3, uint8_t *cc, uint32_t xtra) {
	fp_write_data(ctx, src1, src2, src3, cc);
	if (fp_run(ctx))
		return 1;
	if (fp_check_data(ctx, op1, op2, src1, src2, src3, cc, xtra))
		return 1;
	return 0;
}

static void fp_gen(struct hwtest_ctx *ctx, uint32_t *src1, uint32_t *src2, uint32_t *src3, uint8_t *cc) {
	int i;
	int mode = jrand48(ctx->rand48);
	for (i = 0; i < 0x200; i++) {
		src1[i] = jrand48(ctx->rand48);
		src2[i] = jrand48(ctx->rand48);
		src3[i] = jrand48(ctx->rand48);
		if (mode & 0x1)
			src1[i] = (int8_t)src1[i];
		if (mode & 0x2)
			src2[i] = (int8_t)src2[i];
		if (mode & 0x4)
			src3[i] = (int8_t)src3[i];
		cc[i] = jrand48(ctx->rand48);
	}
}

static int test_add(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x20000001 | (jrand48(ctx->rand48) & 0x1e7f0000);
		uint32_t op2 = 0x000007d0 | (jrand48(ctx->rand48) & 0x1fc00004);
		uint32_t xtra = jrand48(ctx->rand48);
		if (op2 >> 26 & 1) {
			/* 32-bit */
			op1 |= 0x81c;
			op2 |= 0x18000;
		} else {
			/* 16-bit */
			op1 |= 0x1038;
			op2 |= 0x30000;
		}
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		uint8_t cc[0x200];
		fp_gen(ctx, src1, src2, src3, cc);
		if (fp_test(ctx, op1, op2, src1, src2, src3, cc, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_add_s(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x20000000 | (jrand48(ctx->rand48) & 0x1e408100);
		uint32_t op2 = 0x10000000;
		uint32_t xtra = jrand48(ctx->rand48);
		if (op1 >> 15 & 1) {
			/* 32-bit */
			op1 |= 0x50818;
		} else {
			/* 16-bit */
			op1 |= 0xa1030;
		}
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		uint8_t cc[0x200];
		fp_gen(ctx, src1, src2, src3, cc);
		if (fp_test(ctx, op1, op2, src1, src2, src3, cc, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_add_i(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x20000001 | (jrand48(ctx->rand48) & 0x1e7f8100);
		uint32_t op2 = 0x00000003 | (jrand48(ctx->rand48) & 0x1ffffffc);
		uint32_t xtra = jrand48(ctx->rand48);
		if (op1 >> 15 & 1) {
			/* 32-bit */
			op1 |= 0x0818;
		} else {
			/* 16-bit */
			op1 |= 0x1030;
		}
		if (fp_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (fp_prep_code(ctx, op1, op2))
				return HWTEST_RES_FAIL;
		uint32_t src1[0x200], src2[0x200], src3[0x200];
		uint8_t cc[0x200];
		fp_gen(ctx, src1, src2, src3, cc);
		if (fp_test(ctx, op1, op2, src1, src2, src3, cc, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(g80_int,
	HWTEST_TEST(test_add, 0),
	HWTEST_TEST(test_add_s, 0),
	HWTEST_TEST(test_add_i, 0),
)
