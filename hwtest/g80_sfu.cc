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
#include "nvhw/sfu.h"

static int g80_sfu_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type != 0x50)
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

static int test_sfu_tab(struct hwtest_ctx *ctx) {
	int i, tpc, mp;
	uint32_t units = nva_rd32(ctx->cnum, 0x1540);
	uint32_t exp[0x400];
	for (i = 0; i < 0x80; i++) {
		exp[0x000+i] = sfu_rcp_tab[i][0];
		exp[0x080+i] = sfu_rsqrt_tab[i][0];
		exp[0x200+i] = (sfu_rcp_tab[i][1] & 0xffff) << 10 | sfu_rcp_tab[i][2];
		exp[0x280+i] = (sfu_rsqrt_tab[i][1] & 0xffff) << 10 | sfu_rsqrt_tab[i][2];
	}
	for (i = 0; i < 0x40; i++) {
		exp[0x100+i] = sfu_sin_tab[i][0];
		exp[0x140+i] = sfu_ex2_tab[i][0];
		exp[0x180+i] = sfu_lg2_tab[i][0];
		exp[0x1c0+i] = 0;
		exp[0x300+i] = sfu_sin_tab[i][1] << 11 | (sfu_sin_tab[i][2] & 0x7ff);
		exp[0x340+i] = sfu_ex2_tab[i][1] << 10 | sfu_ex2_tab[i][2];
		exp[0x380+i] = sfu_lg2_tab[i][1] << 10 | (sfu_lg2_tab[i][2] & 0x3ff);
		exp[0x3c0+i] = 0;
	}
	for (tpc = 0; tpc < 16; tpc++) if (units & 1 << tpc)
		for (mp = 0; mp < 4; mp++) if (units & 1 << (24 + mp)) {
			uint32_t base = 0x408000;
			uint32_t sfu;
			if (ctx->chipset.chipset >= 0xa0) {
				base |= tpc << 11;
				sfu = 0x30;
			} else {
				base |= tpc << 12;
				sfu = 0x2c;
			}
			for (i = 0; i < 0x400; i++) {
				nva_wr32(ctx->cnum, base+0x3b0, i << 6 | sfu | mp);
				uint64_t e = exp[i] | (uint64_t)exp[i] << 26;
				uint32_t lo = nva_rd32(ctx->cnum, base+0x3b4);
				uint32_t hi = nva_rd32(ctx->cnum, base+0x3b8);
				/* XXX: why is the and necessary? */
				uint64_t r = ((uint64_t)hi << 32 | lo) & 0xfffffffffffffull;
				if (e != r) {
					printf("TPC %d MP %d SFU %03x: expected %016" PRIx64 " got %016" PRIx64 "\n",
						tpc, mp, i, e, r);
					return HWTEST_RES_FAIL;
				}
			}
		}
	return HWTEST_RES_PASS;
}

static int sfu_prep_code(struct hwtest_ctx *ctx, uint32_t op1, uint32_t op2) {
	uint32_t code[] = {
		0x30020001,
		0xc4100780,
		0xd0000005,
		0x80c00780,
		op1,
		op2,
		0xd0000009,
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

static int sfu_prep_grid(struct hwtest_ctx *ctx, uint32_t xtra) {
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

static void sfu_write_data(struct hwtest_ctx *ctx, uint32_t base) {
	int i;
	nva_wr32(ctx->cnum, 0x1700, 0x200);
	for (i = 0; i < 0x200; i++) {
		nva_wr32(ctx->cnum, 0x700000 + i * 4, base+i);
	}
	nva_wr32(ctx->cnum, 0x70000, 1);
	while (nva_rd32(ctx->cnum, 0x70000));
}

static int sfu_run(struct hwtest_ctx *ctx) {
	return g80_gr_mthd(ctx, 3, 0x368, 0) || g80_gr_idle(ctx);
}

static int sfu_check_data(struct hwtest_ctx *ctx, uint32_t base, uint32_t op1, uint32_t op2, uint32_t xtra) {
	int i;
	for (i = 0; i < 0x200; i++) {
		uint32_t in = base + i;
		uint32_t rin = in;
		uint32_t real = nva_rd32(ctx->cnum, 0x700000 + i * 4);
		uint32_t exp;
		bool fnz = (ctx->chipset.chipset >= 0xa0 && ctx->chipset.chipset != 0xaa && ctx->chipset.chipset != 0xac) && (xtra & 0x80000);
		if ((op1 >> 28) == 0x9) {
			if (!(op1 & 1)) {
				if (op1 & 0x00008000)
					rin &= ~0x80000000;
				if (op1 & 0x00400000)
					rin ^= 0x80000000;
				exp = sfu_rcp(rin);
				break;
			} else switch (op2 >> 29) {
				case 0:
					if (op2 & 0x00100000)
						rin &= ~0x80000000;
					if (op2 & 0x04000000)
						rin ^= 0x80000000;
					exp = sfu_rcp(rin);
					break;
				case 2:
					if (op2 & 0x00100000)
						rin &= ~0x80000000;
					if (op2 & 0x04000000)
						rin ^= 0x80000000;
					exp = sfu_rsqrt(rin);
					break;
				case 3:
					if (op2 & 0x00100000)
						rin &= ~0x80000000;
					if (op2 & 0x04000000)
						rin ^= 0x80000000;
					exp = sfu_lg2(rin);
					break;
				case 4:
					exp = sfu_sincos(rin, false);
					break;
				case 5:
					exp = sfu_sincos(rin, true);
					break;
				case 6:
					exp = sfu_ex2(rin);
					if (op2 >> 27 & 1)
						exp = fp32_sat(exp, fnz);
					break;
				default:
					abort();
			}
		} else if ((op1 >> 28) == 0xb && (op2 >> 29) == 6) {
			if (op2 & 0x00100000)
				rin &= ~0x80000000;
			if (op2 & 0x04000000)
				rin ^= 0x80000000;
			exp = sfu_pre(rin, (enum sfu_pre_mode)(op2 >> 14 & 1));
		} else {
			abort();
		}
		if (real != exp) {
			printf("sfu %08x %08x (%08x): got %08x expected %08x diff %d\n", op1, op2, in, real, exp, real-exp);
			return 1;
		}
	}
	return 0;
}

static int sfu_test(struct hwtest_ctx *ctx, uint32_t base, uint32_t op1, uint32_t op2, uint32_t xtra) {
	sfu_write_data(ctx, base);
	if (sfu_run(ctx))
		return 1;
	if (sfu_check_data(ctx, base, op1, op2, xtra))
		return 1;
	return 0;
}

static int test_sfu_rcp_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0x90000209;
	uint32_t op2 = 0x00000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx, 0))
		return HWTEST_RES_FAIL;
	for (i = 0x3f800000; i != 0x40000000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2, 0))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_rcp_rnd(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x90000209 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0x00000780 | (jrand48(ctx->rand48) & 0x1fdff074);
		uint32_t xtra = jrand48(ctx->rand48);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_rcp_short_rnd(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x90000208 | (jrand48(ctx->rand48) & 0x0eff8100);
		uint32_t op2 = 0x10000000;
		uint32_t xtra = jrand48(ctx->rand48);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_rsqrt_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0x90000209;
	uint32_t op2 = 0x40000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx, 0))
		return HWTEST_RES_FAIL;
	for (i = 0x3f800000; i != 0x40800000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2, 0))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_rsqrt_rnd(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x90000209 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0x40000780 | (jrand48(ctx->rand48) & 0x1fdff074);
		uint32_t xtra = jrand48(ctx->rand48);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_sin_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0x90000209;
	uint32_t op2 = 0x80000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx, 0))
		return HWTEST_RES_FAIL;
	for (i = 0x00000000; i != 0x02000000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2, 0))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_cos_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0x90000209;
	uint32_t op2 = 0xa0000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx, 0))
		return HWTEST_RES_FAIL;
	for (i = 0x00000000; i != 0x02000000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2, 0))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_sincos_rnd(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x90000209 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0x80000780 | (jrand48(ctx->rand48) & 0x3fdff074);
		uint32_t xtra = jrand48(ctx->rand48);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_ex2_one(struct hwtest_ctx *ctx) {
	uint32_t i;
	uint32_t op1 = 0x90000209;
	uint32_t op2 = 0xc0000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx, 0))
		return HWTEST_RES_FAIL;
	for (i = 0x80000000; i != 0x81800000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2, 0))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_ex2_rnd(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x90000209 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0xc0000780 | (jrand48(ctx->rand48) & 0x1fdff074);
		uint32_t xtra = jrand48(ctx->rand48);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_lg2_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0x90000209;
	uint32_t op2 = 0x60000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx, 0))
		return HWTEST_RES_FAIL;
	for (i = 0x3e800000; i != 0x40800000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2, 0))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_lg2_rnd(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x90000209 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0x60000780 | (jrand48(ctx->rand48) & 0x1fdff074);
		uint32_t xtra = jrand48(ctx->rand48);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_preex2_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0xb0000209;
	uint32_t op2 = 0xc0004780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx, 0))
		return HWTEST_RES_FAIL;
	for (i = 0x3e800000; i != 0x40800000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2, 0))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_presin_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0xb0000209;
	uint32_t op2 = 0xc0000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx, 0))
		return HWTEST_RES_FAIL;
	for (i = 0x3e800000; i != 0x40800000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2, 0))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_pre_rnd(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xb0000209 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0xc0000780 | (jrand48(ctx->rand48) & 0x1fdff074);
		uint32_t xtra = jrand48(ctx->rand48);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_grid(ctx, xtra))
			return HWTEST_RES_FAIL;
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2, xtra))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(g80_sfu,
	HWTEST_TEST(test_sfu_tab, 0),
	HWTEST_TEST(test_sfu_rcp_one, 0),
	HWTEST_TEST(test_sfu_rcp_rnd, 0),
	HWTEST_TEST(test_sfu_rcp_short_rnd, 0),
	HWTEST_TEST(test_sfu_rsqrt_one, 0),
	HWTEST_TEST(test_sfu_rsqrt_rnd, 0),
	HWTEST_TEST(test_sfu_sin_one, 0),
	HWTEST_TEST(test_sfu_cos_one, 0),
	HWTEST_TEST(test_sfu_sincos_rnd, 0),
	HWTEST_TEST(test_sfu_ex2_one, 0),
	HWTEST_TEST(test_sfu_ex2_rnd, 0),
	HWTEST_TEST(test_sfu_lg2_one, 0),
	HWTEST_TEST(test_sfu_lg2_rnd, 0),
	HWTEST_TEST(test_sfu_preex2_one, 0),
	HWTEST_TEST(test_sfu_presin_one, 0),
	HWTEST_TEST(test_sfu_pre_rnd, 0),
)
