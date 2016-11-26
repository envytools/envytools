/*
 * Copyright (C) 2013 Marcin Kościelnicki <koriakin@0x04.net>
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
#include "old.h"
#include "nva.h"
#include "util.h"
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int get_pred(uint64_t *regs, uint32_t op) {
	if (op & 0x80)
		return regs[10] & 1;
	else
		return 1;
}

static int get_psrc1(uint64_t *regs, uint32_t op) {
	int pidx = op >> 8 & 3;
	int pred = regs[10] >> pidx & 1;
	int pnot = op >> 10 & 1;
	return pred ^ pnot;
}

static int get_psrc2(uint64_t *regs, uint32_t op) {
	int pidx = op >> 11 & 3;
	int pred = regs[10] >> pidx & 1;
	int pnot = op >> 13 & 1;
	return pred ^ pnot;
}

static void set_pdst(uint64_t *regs, uint32_t op, uint32_t val) {
	int pidx = op >> 14 & 3;
	int pnot = op >> 16 & 1;
	val ^= pnot;
	if (pidx != 3) {
		regs[10] &= ~(1 << pidx);
		regs[10] |= (val << pidx);
	}
}

static int get_bpred(uint64_t *regs, uint32_t op) {
	int pidx = op >> 14 & 3;
	return regs[10] >> pidx & 1;
}

static int64_t get_src1(uint64_t *regs, uint32_t op) {
	return sext(regs[op >> 8 & 7], 47);
}

static int64_t get_rsrc2(uint64_t *regs, uint32_t op) {
	return sext(regs[op >> 11 & 7], 47);
}

static int64_t get_imm(uint64_t *regs, uint32_t op) {
	return op >> 17 & 0x3f;
}

static int64_t get_limm(uint64_t *regs, uint32_t op) {
	return (op >> 8 & 0x3f) | (op >> 17 & 0x3f) << 6;
}

static int64_t get_src2(uint64_t *regs, uint32_t op) {
	if (op & 0x40)
		return get_imm(regs, op);
	else
		return get_rsrc2(regs, op);
}

static int64_t get_lsrc2(uint64_t *regs, uint32_t op) {
	if (op & 0x40)
		return get_limm(regs, op);
	else
		return get_rsrc2(regs, op);
}

static void set_dst(uint64_t *regs, uint32_t op, uint64_t val) {
	regs[op >> 14 & 7] = val & 0xffffffffffffull;
}

static void simulate_op(uint64_t *regs, uint32_t op) {
	if (!(op & 0x20) && !get_pred(regs, op))
		return;
	int64_t tmp1, tmp2, res;
	switch (op & 0x3f) {
		case 0x00:
			set_dst(regs, op, get_src1(regs, op) + get_src2(regs, op));
			break;
		case 0x01:
			set_dst(regs, op, get_src1(regs, op) - get_src2(regs, op));
			break;
		case 0x02:
			set_dst(regs, op, get_src1(regs, op) >> (get_src2(regs, op) & 0x3f));
			break;
		case 0x03:
			set_dst(regs, op, get_src1(regs, op) << (get_src2(regs, op) & 0x3f));
			break;
		case 0x04:
			set_dst(regs, op, get_src1(regs, op) & get_src2(regs, op));
			break;
		case 0x05:
			set_dst(regs, op, get_src1(regs, op) | get_src2(regs, op));
			break;
		case 0x06:
			set_dst(regs, op, get_src1(regs, op) ^ get_src2(regs, op));
			break;
		case 0x07:
			set_dst(regs, op, ~get_src1(regs, op));
			break;
		case 0x08:
			set_dst(regs, op, get_lsrc2(regs, op));
			break;
		case 0x09:
			tmp1 = get_src1(regs, op);
			res = tmp1 & 0xffff00000000ull;
			res |= (tmp1 & 0xffff) << 16;
			res |= (tmp1 & 0xffff0000) >> 16;
			set_dst(regs, op, res);
			break;
		case 0x0a:
			set_dst(regs, op, std::min(get_src1(regs, op), get_src2(regs, op)));
			break;
		case 0x0b:
			set_dst(regs, op, std::max(get_src1(regs, op), get_src2(regs, op)));
			break;
		case 0x0c:
			set_pdst(regs, op, get_src1(regs, op) > get_src2(regs, op));
			break;
		case 0x0d:
			set_pdst(regs, op, get_src1(regs, op) < get_src2(regs, op));
			break;
		case 0x0e:
			set_pdst(regs, op, get_src1(regs, op) == get_src2(regs, op));
			break;
		case 0x0f:
			set_pdst(regs, op, get_src1(regs, op) >> (get_src2(regs, op) & 0x3f) & 1);
			break;
		case 0x10:
			set_pdst(regs, op, get_psrc1(regs, op) & get_psrc2(regs, op));
			break;
		case 0x11:
			set_pdst(regs, op, get_psrc1(regs, op) | get_psrc2(regs, op));
			break;
		case 0x12:
			set_pdst(regs, op, get_psrc1(regs, op) ^ get_psrc2(regs, op));
			break;
		case 0x13:
			set_pdst(regs, op, get_psrc1(regs, op));
			break;
		case 0x14:
			set_pdst(regs, op, 1);
			break;
		case 0x15:
			set_dst(regs, op, 1ull << (get_src2(regs, op) & 0x3f));
			break;
		case 0x16:
			set_dst(regs, op, ((1ull << (get_src2(regs, op) & 0x3f)) - 1));
			break;
		case 0x24:
			tmp1 = get_src2(regs, op) & 0xffff;
			if (tmp1)
				set_dst(regs, op, get_src1(regs, op) / tmp1);
			else
				set_dst(regs, op, get_src1(regs, op) < 0 ? 1ull : -1ull);
			break;
		case 0x25:
			tmp1 = sext(get_rsrc2(regs, op), 15);
			tmp1 *= get_src1(regs, op);
			tmp2 = get_imm(regs, op) & 0xf;
			if (tmp2)
				tmp1 += 1 << (tmp2-1);
			tmp1 >>= tmp2;
			set_dst(regs, op, tmp1);
			break;
		case 0x2a:
			if (!get_bpred(regs, op))
				regs[8] = get_limm(regs, op) & 0xff;
			break;
		case 0x2b:
			if (get_bpred(regs, op))
				regs[8] = get_limm(regs, op) & 0xff;
			break;
		/* XXX: why two bras? */
		case 0x28:
		case 0x2c:
		case 0x2e: /* call */
			regs[8] = get_limm(regs, op) & 0xff;
			break;
	}
}

static int test_arith(struct hwtest_ctx *ctx) {
	int i, j;
	for (i = 0; i < 100000; i++) {
		uint32_t op = jrand48(ctx->rand48) & 0x7fffff;
		int opc = op & 0x3f;
		uint64_t regs[12], eregs[12], nregs[12];
		for (j = 0; j < 8; j++) {
			regs[j] = (uint32_t)jrand48(ctx->rand48);
			regs[j] |= (uint64_t)jrand48(ctx->rand48) << 32;
			regs[j] &= 0xffffffffffffull;
		}
		uint32_t pc = regs[8] = jrand48(ctx->rand48) & 0xff;
		if (pc == 0xff)
			continue;
		regs[9] = 0;
		regs[10] = jrand48(ctx->rand48) & 7;
		regs[11] = jrand48(ctx->rand48) & 0xf;
		uint32_t btarg = get_limm(regs, op) & 0xff;
		/* load unsigned & load signed */
		if (opc == 0x20 || opc == 0x21)
			continue;
		/* store */
		if (opc == 0x22)
			continue;
		/* sleep */
		if (opc == 0x26)
			continue;
		/* prevent bra to self */
		if ((opc == 0x28 || opc == 0x2a || opc == 0x2b || opc == 0x2c || opc == 0x2e) && (btarg == 0 || btarg == pc))
			continue;
		/* ret */
		if (opc == 0x2f)
			continue;
		for (j = 0; j < 0x100; j++) {
			nva_wr32(ctx->cnum, 0x1c17c8, (j) << 2);
			nva_wr32(ctx->cnum, 0x1c17cc, 0x26);
		}
		nva_wr32(ctx->cnum, 0x1c107c, 1);
		nva_wr32(ctx->cnum, 0x1c17c8, 0);
		nva_wr32(ctx->cnum, 0x1c17cc, 0x28 | (pc & 0x3f) << 8 | (pc >> 6) << 17);
		nva_wr32(ctx->cnum, 0x1c17c8, pc << 2);
		nva_wr32(ctx->cnum, 0x1c17cc, op);
		nva_wr32(ctx->cnum, 0x1c17c8, (pc+1) << 2);
		nva_wr32(ctx->cnum, 0x1c17cc, 0x26);
		for (j = 0; j < 12; j++) {
			nva_wr32(ctx->cnum, 0x1c17d0, j);
			nva_wr32(ctx->cnum, 0x1c17d4, regs[j]);
			nva_wr32(ctx->cnum, 0x1c17d8, regs[j] >> 32);
			eregs[j] = regs[j];
		}
		nva_wr32(ctx->cnum, 0x1c17c0, 1);
		eregs[8] = pc+1;
		simulate_op(eregs, op);
		eregs[8]++;
		eregs[8] &= 0xff;
		eregs[9] = 0x26;
		nva_rd32(ctx->cnum, 0x1c17c4);
		nva_rd32(ctx->cnum, 0x1c17c4);
		nva_rd32(ctx->cnum, 0x1c17c4);
		nva_rd32(ctx->cnum, 0x1c17c4);
		int bad = 0;
		if (nva_rd32(ctx->cnum, 0x1c17c4)) {
			printf("HUNG!\n");
			bad = 1;
		}
		nva_wr32(ctx->cnum, 0x1c17c0, 2);
		for (j = 0; j < 12; j++) {
			nva_wr32(ctx->cnum, 0x1c17d0, j);
			nregs[j] = nva_rd32(ctx->cnum, 0x1c17d4);
			nregs[j] |= (uint64_t)nva_rd32(ctx->cnum, 0x1c17d8) << 32;
			if (nregs[j] != eregs[j]) {
				bad = 1;
			}
		}
		if (bad) {
			printf("Mismatch on try %d for insn 0x%06x\n", i, op);
			printf("initial      expected     real\n");
			for (j = 0; j < 12; j++)
				printf("%012" PRIx64 " %012" PRIx64 " %012" PRIx64 "%s\n", regs[j], eregs[j], nregs[j], (nregs[j] != eregs[j] ? " XXX" : ""));
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int pvcomp_isa_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset != 0xaf)
		return HWTEST_RES_NA;
	nva_wr32(ctx->cnum, 0x200, 0xffffbfff);
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(pvcomp_isa,
	HWTEST_TEST(test_arith, 0),
)
