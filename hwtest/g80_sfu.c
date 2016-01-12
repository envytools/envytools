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
#include "nva.h"
#include "util.h"
#include "nvhw/sfu.h"

static int g80_gr_intr(struct hwtest_ctx *ctx) {
	uint32_t intr = nva_rd32(ctx->cnum, 0x400100);
	if (!intr)
		return 0;
	if (intr & 0x100000)
		printf("DATA_ERROR %#x\n", nva_rd32(ctx->cnum, 0x400110));
	if (intr & 0x200000) {
		uint32_t trap = nva_rd32(ctx->cnum, 0x400108);
		printf("TRAP %#x\n", trap);
	}
	if (intr & 0x1)
		printf("NOTIFY\n");
	if (intr & 0x10)
		printf("INVALID METHOD\n");
	if (intr & 0x20)
		printf("INVALID CLASS\n");
	if (intr & 0x40)
		printf("INVALID NOTIFY\n");
	if (intr & 0x10000)
		printf("BUFFER\n");
	if (intr & 0x1000000)
		printf("SINGLE STEP\n");
	if (intr & 0x1100071)
		printf("CLASS %04x ADDR %08x DATA %08x",
			nva_rd32(ctx->cnum, 0x400814) & 0xffff,
			nva_rd32(ctx->cnum, 0x400704),
			nva_rd32(ctx->cnum, 0x400708));
	uint32_t rintr = intr & ~0x1310071;
	if (rintr)
		printf("INTERRUPT %08x\n", rintr);
	if (intr & ~0x01010001)
		return 1;
	nva_wr32(ctx->cnum, 0x400100, intr);
	nva_wr32(ctx->cnum, 0x400500, 0x10001);
	return 0;
}

static int g80_gr_mthd(struct hwtest_ctx *ctx, uint32_t subc, uint32_t mthd, uint32_t data) {
	while (nva_rd32(ctx->cnum, 0x400808) & 0x80000000) {
		if (g80_gr_intr(ctx))
			return 1;
	}
	if (g80_gr_intr(ctx))
		return 1;
	nva_wr32(ctx->cnum, 0x40080c, data);
	nva_wr32(ctx->cnum, 0x400808, mthd | subc << 16 | 0x80000000);
	return 0;
}

static int g80_gr_idle(struct hwtest_ctx *ctx) {
	while (nva_rd32(ctx->cnum, 0x400700)) {
		if (g80_gr_intr(ctx))
			return 1;
	}
	if (g80_gr_intr(ctx))
		return 1;
	return 0;
}

static int g80_sfu_prep(struct hwtest_ctx *ctx) {
	if (ctx->card_type != 0x50)
		return HWTEST_RES_NA;
	if (!(nva_rd32(ctx->cnum, 0x200) & 1 << 20)) {
		printf("Mem controller not up.\n");
		return HWTEST_RES_UNPREP;
	}
	uint32_t units = nva_rd32(ctx->cnum, 0x1540);
	/* Reset graph & fifo */
	nva_wr32(ctx->cnum, 0x200, 0xffdfeeff);
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	/* Aim window at 0x1000000, where our channel will be. */
	nva_wr32(ctx->cnum, 0x1700, 0x100);
	/* Make a DMA object 0x10 convering 4GB of VRAM. */
	/* XXX needs a different one for IGPs */
	nva_wr32(ctx->cnum, 0x700100, 0x0019003d);
	nva_wr32(ctx->cnum, 0x700104, 0xffffffff);
	nva_wr32(ctx->cnum, 0x700108, 0);
	nva_wr32(ctx->cnum, 0x70010c, 0);
	nva_wr32(ctx->cnum, 0x700110, 0);
	nva_wr32(ctx->cnum, 0x700114, 0x10000);
	/* Make m2mf object 0x12. */
	nva_wr32(ctx->cnum, 0x700120, 0x5039);
	nva_wr32(ctx->cnum, 0x700124, 0);
	nva_wr32(ctx->cnum, 0x700128, 0);
	nva_wr32(ctx->cnum, 0x70012c, 0);
	/* Make 2d object 0x13. */
	nva_wr32(ctx->cnum, 0x700130, 0x502d);
	nva_wr32(ctx->cnum, 0x700134, 0);
	nva_wr32(ctx->cnum, 0x700138, 0);
	nva_wr32(ctx->cnum, 0x70013c, 0);
	/* Make compat 3d object 0x14. */
	nva_wr32(ctx->cnum, 0x700140, 0x5097);
	nva_wr32(ctx->cnum, 0x700144, 0);
	nva_wr32(ctx->cnum, 0x700148, 0);
	nva_wr32(ctx->cnum, 0x70014c, 0);
	/* Make current 3d object 0x15. */
	uint16_t gr3d;
	if (ctx->chipset == 0x50) {
		/* G80 */
		gr3d = 0x5097;
	} else if (ctx->chipset < 0xa0) {
		/* G84 */
		gr3d = 0x8297;
	} else if (ctx->chipset == 0xa0 || ctx->chipset == 0xaa || ctx->chipset == 0xac) {
		/* G200 */
		gr3d = 0x8397;
	} else if (ctx->chipset != 0xaf) {
		/* GT215 */
		gr3d = 0x8597;
	} else {
		/* MCP89 */
		gr3d = 0x8697;
	}
	nva_wr32(ctx->cnum, 0x700150, gr3d);
	nva_wr32(ctx->cnum, 0x700154, 0);
	nva_wr32(ctx->cnum, 0x700158, 0);
	nva_wr32(ctx->cnum, 0x70015c, 0);
	/* Make old compute object 0x16. */
	nva_wr32(ctx->cnum, 0x700160, 0x50c0);
	nva_wr32(ctx->cnum, 0x700164, 0);
	nva_wr32(ctx->cnum, 0x700168, 0);
	nva_wr32(ctx->cnum, 0x70016c, 0);
	/* Make new compute object 0x17. */
	nva_wr32(ctx->cnum, 0x700170, 0x85c0);
	nva_wr32(ctx->cnum, 0x700174, 0);
	nva_wr32(ctx->cnum, 0x700178, 0);
	nva_wr32(ctx->cnum, 0x70017c, 0);
	/* Flush it. */
	nva_wr32(ctx->cnum, 0x70000, 1);
	while (nva_rd32(ctx->cnum, 0x70000));
	/* Clear exception status. */
	nva_wr32(ctx->cnum, 0x400804, 0xc0000000);
	nva_wr32(ctx->cnum, 0x406800, 0xc0000000);
	nva_wr32(ctx->cnum, 0x400c04, 0xc0000000);
	nva_wr32(ctx->cnum, 0x401800, 0xc0000000);
	nva_wr32(ctx->cnum, 0x405018, 0xc0000000);
	nva_wr32(ctx->cnum, 0x402000, 0xc0000000);
	int i;
	/* Clear TPC exception status. */
	for (i = 0; i < 16; i++)
		if (units & 1 << i) {
			if (ctx->chipset < 0xa0) {
				nva_wr32(ctx->cnum, 0x408314 | i << 12, 0xc0000000);
				nva_wr32(ctx->cnum, 0x408318 | i << 12, 0xffffffff);
				nva_wr32(ctx->cnum, 0x408900 | i << 12, 0xc0000000);
				nva_wr32(ctx->cnum, 0x408e08 | i << 12, 0xc0000000);
			} else {
				nva_wr32(ctx->cnum, 0x40831c | i << 11, 0xc0000000);
				nva_wr32(ctx->cnum, 0x408320 | i << 11, 0xffffffff);
				nva_wr32(ctx->cnum, 0x408600 | i << 11, 0xc0000000);
				nva_wr32(ctx->cnum, 0x408708 | i << 11, 0xc0000000);
			}
		}
	/* Reset all units, for a good measure. */
	nva_wr32(ctx->cnum, 0x400040, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400040, 0);
	/* Cargo. */
	nva_wr32(ctx->cnum, 0x405020, 1);
	nva_wr32(ctx->cnum, 0x400080, 0x401889f2);
	/* Reset interrupt & trap status, enable them all. */
	nva_wr32(ctx->cnum, 0x400100, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400108, 0xffffffff);
	nva_wr32(ctx->cnum, 0x40013c, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400138, 0xffffffff);
	/* Poke CTXCTL and bind the channel. */
	nva_wr32(ctx->cnum, 0x400824, 0x00004000);
	nva_wr32(ctx->cnum, 0x400784, 0x00001000);
	nva_wr32(ctx->cnum, 0x400320, 4);
	nva_wr32(ctx->cnum, 0x40032c, 0x80001000);
	/* Enable FIFO access. */
	nva_wr32(ctx->cnum, 0x400500, 0x00010001);
	/* Bind subchannels. */
	g80_gr_mthd(ctx, 0, 0, 0x12);
	g80_gr_mthd(ctx, 1, 0, 0x13);
	g80_gr_mthd(ctx, 2, 0, 0x15);
	g80_gr_mthd(ctx, 3, 0, 0x16);
	/* Initialize compute. */
	/* Notify, semaphore, cond. */
	g80_gr_mthd(ctx, 3, 0x180, 0x10);
	g80_gr_mthd(ctx, 3, 0x1a4, 0x10);
	g80_gr_mthd(ctx, 3, 0x320, 0x0);
	g80_gr_mthd(ctx, 3, 0x324, 0x0);
	g80_gr_mthd(ctx, 3, 0x328, 0x1);
	/* Cargo and misc. */
	g80_gr_mthd(ctx, 3, 0x364, 0x0);
	g80_gr_mthd(ctx, 3, 0x228, 0xffff0);
	g80_gr_mthd(ctx, 3, 0x224, 1);
	g80_gr_mthd(ctx, 3, 0x288, 0);
	g80_gr_mthd(ctx, 3, 0x290, 1);
	g80_gr_mthd(ctx, 3, 0x2a0, 0);
	g80_gr_mthd(ctx, 3, 0x2b0, 7);
	g80_gr_mthd(ctx, 3, 0x2bc, 0);
	g80_gr_mthd(ctx, 3, 0x2f0, 0);
	g80_gr_mthd(ctx, 3, 0x370, 0);
	g80_gr_mthd(ctx, 3, 0x36c, 0);
	/* Global. */
	g80_gr_mthd(ctx, 3, 0x1a0, 0x10);
	for (i = 0; i < 16; i++) {
		/* Bind each one at 32MB mark, size 16MB. */
		g80_gr_mthd(ctx, 3, 0x400 | i << 5, 0);
		g80_gr_mthd(ctx, 3, 0x404 | i << 5, 0x02000000);
		g80_gr_mthd(ctx, 3, 0x408 | i << 5, 0);
		g80_gr_mthd(ctx, 3, 0x40c | i << 5, 0xffffff);
		g80_gr_mthd(ctx, 3, 0x410 | i << 5, 1);
	}
	/* Local - aimed at 0x1010000 and disabled. */
	g80_gr_mthd(ctx, 3, 0x1b8, 0x10);
	g80_gr_mthd(ctx, 3, 0x294, 0x0);
	g80_gr_mthd(ctx, 3, 0x298, 0x1010000);
	g80_gr_mthd(ctx, 3, 0x29c, 0x0);
	g80_gr_mthd(ctx, 3, 0x2fc, 0x7);
	g80_gr_mthd(ctx, 3, 0x300, 0x0);
	/* Stack - aimed at 0x1020000 and disabled. */
	g80_gr_mthd(ctx, 3, 0x1bc, 0x10);
	g80_gr_mthd(ctx, 3, 0x218, 0x0);
	g80_gr_mthd(ctx, 3, 0x21c, 0x1020000);
	g80_gr_mthd(ctx, 3, 0x220, 0x0);
	g80_gr_mthd(ctx, 3, 0x304, 0x7);
	g80_gr_mthd(ctx, 3, 0x308, 0x0);
	/* Code+CB - at 0x1030000 and 0x1040000. */
	g80_gr_mthd(ctx, 3, 0x1c0, 0x10);
	g80_gr_mthd(ctx, 3, 0x380, 0); /* Flush for a good measure. */
	g80_gr_mthd(ctx, 3, 0x210, 0);
	g80_gr_mthd(ctx, 3, 0x214, 0x1030000);
	g80_gr_mthd(ctx, 3, 0x3b4, 0);
	g80_gr_mthd(ctx, 3, 0x2a4, 0);
	g80_gr_mthd(ctx, 3, 0x2a8, 0x1040000);
	for (i = 0; i < 0x80; i++)
		g80_gr_mthd(ctx, 3, 0x2ac, i << 16);
	for (i = 0; i < 0x10; i++)
		g80_gr_mthd(ctx, 3, 0x3c8, i << 8 | 1);
	/* Texture. */
	g80_gr_mthd(ctx, 3, 0x1cc, 0x10);
	g80_gr_mthd(ctx, 3, 0x378, 0);
	g80_gr_mthd(ctx, 3, 0x3bc, 0x22);
	g80_gr_mthd(ctx, 3, 0x32c, 0);
	g80_gr_mthd(ctx, 3, 0x358, 1);
	g80_gr_mthd(ctx, 3, 0x3d0, 0x20);
	g80_gr_mthd(ctx, 3, 0x37c, 0);
	/* TSC - at 0x1050000. */
	g80_gr_mthd(ctx, 3, 0x1c4, 0x10);
	g80_gr_mthd(ctx, 3, 0x22c, 0);
	g80_gr_mthd(ctx, 3, 0x230, 0x1050000);
	g80_gr_mthd(ctx, 3, 0x234, 0x7f);
	g80_gr_mthd(ctx, 3, 0x27c, 0);
	/* TIC - at 0x1060000. */
	g80_gr_mthd(ctx, 3, 0x1c8, 0x10);
	g80_gr_mthd(ctx, 3, 0x2c4, 0);
	g80_gr_mthd(ctx, 3, 0x2c8, 0x1060000);
	g80_gr_mthd(ctx, 3, 0x2cc, 0x7f);
	g80_gr_mthd(ctx, 3, 0x280, 0);
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
			if (ctx->chipset >= 0xa0) {
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
					printf("TPC %d MP %d SFU %03x: expected %016"PRIx64" got %016"PRIx64"\n",
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
	int i;
	/* Poke code and flush it. */
	nva_wr32(ctx->cnum, 0x1700, 0x100);
	for (i = 0; i < ARRAY_SIZE(code); i++)
		nva_wr32(ctx->cnum, 0x730000 + i * 4, code[i]);
	nva_wr32(ctx->cnum, 0x70000, 1);
	while (nva_rd32(ctx->cnum, 0x70000));
	return g80_gr_mthd(ctx, 3, 0x380, 0);
}

static int sfu_prep_grid(struct hwtest_ctx *ctx) {
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

static int sfu_check_data(struct hwtest_ctx *ctx, uint32_t base, uint32_t op1, uint32_t op2) {
	int i;
	for (i = 0; i < 0x200; i++) {
		uint32_t in = base + i;
		uint32_t rin = in;
		uint32_t real = nva_rd32(ctx->cnum, 0x700000 + i * 4);
		uint32_t exp;
		/* XXX */
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
					exp = sfu_ex2(rin, op2 >> 27 & 1);
					break;
				default:
					abort();
			}
		} else if ((op1 >> 28) == 0xb && (op2 >> 29) == 6) {
			if (op2 & 0x00100000)
				rin &= ~0x80000000;
			if (op2 & 0x04000000)
				rin ^= 0x80000000;
			exp = sfu_pre(rin, op2 >> 14 & 1);
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

static int sfu_test(struct hwtest_ctx *ctx, uint32_t base, uint32_t op1, uint32_t op2) {
	sfu_write_data(ctx, base);
	if (sfu_run(ctx))
		return 1;
	if (sfu_check_data(ctx, base, op1, op2))
		return 1;
	return 0;
}

static int test_sfu_rcp_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0x90000209;
	uint32_t op2 = 0x00000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0x3f800000; i != 0x40000000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_rcp_rnd(struct hwtest_ctx *ctx) {
	int i;
	if (sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x90000209 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0x00000780 | (jrand48(ctx->rand48) & 0x1fdff074);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_rcp_short_rnd(struct hwtest_ctx *ctx) {
	int i;
	if (sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x90000208 | (jrand48(ctx->rand48) & 0x0eff8100);
		uint32_t op2 = 0x10000000;
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_rsqrt_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0x90000209;
	uint32_t op2 = 0x40000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0x3f800000; i != 0x40800000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_rsqrt_rnd(struct hwtest_ctx *ctx) {
	int i;
	if (sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x90000209 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0x40000780 | (jrand48(ctx->rand48) & 0x1fdff074);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_sin_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0x90000209;
	uint32_t op2 = 0x80000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0x00000000; i != 0x02000000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_cos_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0x90000209;
	uint32_t op2 = 0xa0000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0x00000000; i != 0x02000000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_sincos_rnd(struct hwtest_ctx *ctx) {
	int i;
	if (sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x90000209 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0x80000780 | (jrand48(ctx->rand48) & 0x3fdff074);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_ex2_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0x90000209;
	uint32_t op2 = 0xc0000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0x80000000; i != 0x81800000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_ex2_rnd(struct hwtest_ctx *ctx) {
	int i;
	if (sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x90000209 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0xc0000780 | (jrand48(ctx->rand48) & 0x1fdff074);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_lg2_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0x90000209;
	uint32_t op2 = 0x60000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0x3e800000; i != 0x40800000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_lg2_rnd(struct hwtest_ctx *ctx) {
	int i;
	if (sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0x90000209 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0x60000780 | (jrand48(ctx->rand48) & 0x1fdff074);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_preex2_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0xb0000209;
	uint32_t op2 = 0xc0004780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0x3e800000; i != 0x40800000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_presin_one(struct hwtest_ctx *ctx) {
	int i;
	uint32_t op1 = 0xb0000209;
	uint32_t op2 = 0xc0000780;
	if (sfu_prep_code(ctx, op1, op2) || sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0x3e800000; i != 0x40800000; i += 0x200) {
		if (sfu_test(ctx, i, op1, op2))
			return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_sfu_pre_rnd(struct hwtest_ctx *ctx) {
	int i;
	if (sfu_prep_grid(ctx))
		return HWTEST_RES_FAIL;
	for (i = 0; i < 100000; i++) {
		uint32_t op1 = 0xb0000209 | (jrand48(ctx->rand48) & 0x0fff0000);
		uint32_t op2 = 0xc0000780 | (jrand48(ctx->rand48) & 0x1fdff074);
		uint32_t in = jrand48(ctx->rand48);
		if (sfu_prep_code(ctx, op1, op2))
			return HWTEST_RES_FAIL;
		if (sfu_test(ctx, in, op1, op2))
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
