/*
 * Copyright (C) 2014 Marcin Kościelnicki <koriakin@0x04.net>
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct vp1_ctx {
	uint32_t r[31];
	uint32_t c[4];
	uint32_t res;
	uint32_t s1;
	uint32_t imm;
};

static uint32_t read_r(struct vp1_ctx *ctx, int idx) {
	if (idx < 31)
		return ctx->r[idx];
	return 0;
}

static void write_r(struct vp1_ctx *ctx, int idx, uint32_t val) {
	if (idx < 31)
		ctx->r[idx] = val;
}

static void write_c_s(struct vp1_ctx *ctx, int idx, uint32_t val) {
	if (idx < 4) {
		ctx->c[idx] &= ~0xff;
		ctx->c[idx] |= val;
	}
}

static uint32_t cond_s(uint32_t val) {
	uint32_t res = 0;
	if (!val)
		res |= 2;
	if (val & 1 << 18)
		res |= 0x80;
	if (val & 1 << 19)
		res |= 0x44;
	if (val & 1 << 20)
		res |= 0x10;
	if (val & 1 << 21)
		res |= 0x20;
	return res;
}

static uint32_t cond_s_f(uint32_t res, uint32_t s1) {
	uint32_t cr = cond_s(res);
	if (res & 0x80000000)
		cr |= 1;
	if ((res ^ s1) & 0x00100000)
		cr |= 8;
	return cr;
}

static uint32_t vp1_min(int32_t a, int32_t b) {
	if (a < b)
		return a;
	else
		return b;
}

static uint32_t vp1_max(int32_t a, int32_t b) {
	if (a > b)
		return a;
	else
		return b;
}

static uint32_t vp1_shr(uint32_t a, int32_t b, int u) {
	b = sext(b, 5);
	if (b < 0) {
		if (b == -0x20)
			return a;
		else
			return a << -b;
	} else {
		if (u)
			return a >> b;
		else
			return (int32_t)a >> b;
	}
}

static void simulate_op(struct vp1_ctx *ctx, uint32_t opcode) {
	uint32_t op = opcode >> 24;
	uint32_t src1 = opcode >> 14 & 0x1f;
	uint32_t dst = opcode >> 19 & 0x1f;
	uint32_t imm = extrs(opcode, 3, 11);
	uint32_t cdst = opcode & 7;
	uint32_t cr, res, s1;
	switch (op) {
		case 0x61:
		case 0x62:
		case 0x63:
		case 0x64:
		case 0x68:
		case 0x69:
		case 0x6c:
		case 0x6d:
		case 0x6e:
		case 0x71:
		case 0x78:
		case 0x79:
		case 0x7a:
		case 0x7b:
		case 0x7c:
		case 0x7d:
		case 0x7e:
			s1 = read_r(ctx, src1);
			if (op == 0x61 || op == 0x71) {
				res = sext(s1, 15) * imm;
				cr = cond_s_f(res, s1);
			} else if (op == 0x62) {
				res = s1 & imm;
				cr = cond_s(res);
			} else if (op == 0x63) {
				res = s1 ^ imm;
				cr = cond_s(res);
			} else if (op == 0x64) {
				res = s1 | imm;
				cr = cond_s(res);
			} else if (op == 0x68 || op == 0x78) {
				res = vp1_min(s1, imm);
				cr = cond_s_f(res, s1);
			} else if (op == 0x69 || op == 0x79) {
				res = vp1_max(s1, imm);
				cr = cond_s_f(res, s1);
			} else if (op == 0x6c || op == 0x7c) {
				res = s1 + imm;
				cr = cond_s_f(res, s1);
			} else if (op == 0x6d || op == 0x7d) {
				res = s1 - imm;
				cr = cond_s_f(res, s1);
			} else if (op == 0x6e || op == 0x7e) {
				res = vp1_shr(s1, imm, op == 0x7e);
				cr = cond_s_f(res, s1);
			} else if (op == 0x7a) {
				if (s1 & 0x80000000)
					res = -s1;
				else
					res = s1;
				cr = cond_s_f(res, s1);
			} else if (op == 0x7b) {
				res = -s1;
				cr = cond_s_f(res, 0);
			}
			ctx->s1 = s1;
			ctx->imm = imm;
			ctx->res = res;
			write_r(ctx, dst, res);
			write_c_s(ctx, cdst, cr);
			break;
		case 0x65:
			write_r(ctx, dst, extrs(opcode, 0, 19));
			break;
		case 0x75:
			write_r(ctx, dst, (read_r(ctx, dst) & 0xffff) | (opcode & 0xffff) << 16);
			break;
	}
}

static int test_isa_s(struct hwtest_ctx *ctx) {
	int i, j;
	for (i = 0; i < 1000000; i++) {
		nva_wr32(ctx->cnum, 0x200, 0xfffffffd);
		nva_wr32(ctx->cnum, 0x200, 0xffffffff);
		uint32_t opcode = (uint32_t)jrand48(ctx->rand48);
		opcode &= 0x1fffffff;
		opcode |= 0x60000000;
		uint32_t op = opcode >> 24;
		if (op == 0x6a || op == 0x6b)
			continue;
		struct vp1_ctx octx, ectx, nctx;
		for (j = 0; j < 31; j++)
			octx.r[j] = jrand48(ctx->rand48);
		for (j = 0; j < 4; j++)
			octx.c[j] = nva_rd32(ctx->cnum, 0xf680 + j * 4);
		ectx = octx;
		for (j = 0; j < 31; j++) {
			nva_wr32(ctx->cnum, 0xf780 + j * 4, octx.r[j]);
		}
		nva_wr32(ctx->cnum, 0xf44c, opcode);
		nva_wr32(ctx->cnum, 0xf458, 1);
		simulate_op(&ectx, opcode);
		/* XXX wait? */
		for (j = 0; j < 31; j++)
			nctx.r[j] = nva_rd32(ctx->cnum, 0xf780 + j * 4);
		for (j = 0; j < 4; j++)
			nctx.c[j] = nva_rd32(ctx->cnum, 0xf680 + j * 4);
		if (memcmp(&ectx, &nctx, sizeof ectx)) {
			printf("Mismatch on try %d for insn 0x%08"PRIx32"\n", i, opcode);
			printf("what        initial    expected   real\n");
#define IPRINT(name, x) printf(name "0x%08x 0x%08x 0x%08x%s\n", j, octx.x, ectx.x, nctx.x, (nctx.x != ectx.x ? " *" : ""))
			for (j = 0; j < 31; j++) {
				IPRINT("R[0x%02x]   ", r[j]);
			}
			for (j = 0; j < 4; j++) {
				IPRINT("C[%d] ", c[j]);
			}
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int vp1_prep(struct hwtest_ctx *ctx) {
	/* XXX some cards have missing VP1 */
	if (ctx->chipset < 0x41 || ctx->chipset >= 0x84)
		return HWTEST_RES_NA;
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(vp1,
	HWTEST_TEST(test_isa_s, 0),
)
