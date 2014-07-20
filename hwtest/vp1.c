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

static uint32_t vp1_abs(uint32_t x) {
	if (x & 0x80000000)
		return -x;
	else
		return x;
}

static uint8_t vp1_shrb(int32_t a, uint8_t b) {
	int shr = b & 0xf;
	if (shr & 8)
		shr |= -8;
	if (shr < 0) {
		return a << -shr;
	} else {
		return a >> shr;
	}
}

static void simulate_op(struct vp1_ctx *ctx, uint32_t opcode) {
	uint32_t op = opcode >> 24 & 0x7f;
	uint32_t src1 = opcode >> 14 & 0x1f;
	uint32_t src2 = opcode >> 9 & 0x1f;
	uint32_t dst = opcode >> 19 & 0x1f;
	uint32_t imm = extrs(opcode, 3, 11);
	uint32_t cdst = opcode & 7;
	uint32_t cr, res, s1, s2, s2s;
	int flag = opcode >> 5 & 0xf;
	uint32_t cond;
	int src2s;
	int32_t sub, ss1, ss2;
	int i;
	int u;
	cond = ctx->c[opcode >> 3 & 3];
	if (flag == 4) {
		src2s = (src2 & 0x1c) | ((src2 + (cond >> flag)) & 3);
	} else {
		src2s = src2 ^ ((cond >> flag) & 1);
	}
	switch (op) {
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2b:
		case 0x2c:
		case 0x2d:
		case 0x2e:
		case 0x38:
		case 0x39:
		case 0x3a:
		case 0x3b:
		case 0x3c:
		case 0x3d:
		case 0x3e:
			u = !!(op & 0x10);
			s1 = read_r(ctx, src1);
			s2 = read_r(ctx, src2s);
			res = 0;
			for (i = 0; i < 4; i++) {
				ss1 = s1 >> i * 8;
				if (op & 0x20)
					ss2 = opcode >> 3;
				else
					ss2 = s2 >> i * 8;
				ss1 = (int8_t)ss1;
				ss2 = (int8_t)ss2;
				if (u) {
					ss1 &= 0xff;
					ss2 &= 0xff;
				}
				switch (op & 0xf) {
					case 0x8:
						sub = vp1_min(ss1, ss2);
						break;
					case 0x9:
						sub = vp1_max(ss1, ss2);
						break;
					case 0xa:
						sub = vp1_abs(ss1);
						break;
					case 0xb:
						sub = -ss1;
						break;
					case 0xc:
						sub = ss1 + ss2;
						break;
					case 0xd:
						sub = ss1 - ss2;
						break;
					case 0xe:
						sub = vp1_shrb(ss1, ss2);
						break;
				}
				if ((op & 0xf) != 0xe) {
					if (u) {
						if (sub < 0)
							sub = 0;
						if (sub > 0xff)
							sub = 0xff;
					} else {
						if (sub < -0x80)
							sub = -0x80;
						if (sub > 0x7f)
							sub = 0x7f;
					}
				}
				sub &= 0xff;
				res |= sub << i * 8;
			}
			write_r(ctx, dst, res);
			write_c_s(ctx, cdst, 0);
			break;
		case 0x01:
		case 0x02:
		case 0x11:
		case 0x12:
		case 0x21:
		case 0x22:
		case 0x31:
		case 0x32:
			u = !!(op & 0x10);
			s1 = read_r(ctx, src1);
			s2 = read_r(ctx, src2);
			res = 0;
			for (i = 0; i < 4; i++) {
				if (opcode & 4)
					ss1 = (int8_t)(s1 >> i * 8) << 1;
				else
					ss1 = s1 >> i * 8 & 0xff;
				if (op & 0x20) {
					if (op & 2) {
						ss2 = (int8_t)opcode;
					} else {
						imm = (opcode & 1) << 5 | src2;
						ss2 = (int8_t)(imm << 2);
					}
				} else {
					ss2 = (int8_t)(s2 >> i * 8);
				}
				if (opcode & 2)
					ss2 <<= 1;
				else
					ss2 &= 0xff;
				sub = ss1 * ss2;
				if (u) {
					if (sub & 0x80000000)
						sub = 0;
					if (opcode & 0x100)
						sub += 0x80;
					sub >>= 8;
					if (sub >= 0x100)
						sub = 0xff;
				} else {
					if (opcode & 0x100)
						sub += 0x100;
					sub >>= 9;
					if (sub == 0x80)
						sub = 0x7f;
				}
				sub &= 0xff;
				res |= sub << i * 8;
			}
			write_r(ctx, dst, res);
			break;
		case 0x25:
		case 0x26:
		case 0x27:
			s1 = read_r(ctx, src1);
			imm = opcode >> 3 & 0xff;
			imm *= 0x01010101;
			if (op == 0x25) {
				res = s1 & imm;
			} else if (op == 0x26) {
				res = s1 | imm;
			} else if (op == 0x27) {
				res = s1 ^ imm;
			}
			write_r(ctx, dst, res);
			write_c_s(ctx, cdst, 0);
			break;
		case 0x41:
		case 0x42:
		case 0x48:
		case 0x49:
		case 0x4a:
		case 0x4b:
		case 0x4c:
		case 0x4d:
		case 0x4e:
		case 0x51:
		case 0x58:
		case 0x59:
		case 0x5a:
		case 0x5b:
		case 0x5c:
		case 0x5d:
		case 0x5e:
			s1 = read_r(ctx, src1);
			s2 = read_r(ctx, src2);
			s2s = read_r(ctx, src2s);
			if (op == 0x41 || op == 0x51) {
				res = sext(s1, 15) * sext(s2s, 15);
				cr = cond_s_f(res, s1);
			} else if (op == 0x42) {
				switch (opcode >> 3 & 0xf) {
					case 0x0:
						res = 0;
						break;
					case 0x1:
						res = ~s1 & ~s2;
						break;
					case 0x2:
						res = ~s1 & s2;
						break;
					case 0x3:
						res = ~s1;
						break;
					case 0x4:
						res = s1 & ~s2;
						break;
					case 0x5:
						res = ~s2;
						break;
					case 0x6:
						res = s1 ^ s2;
						break;
					case 0x7:
						res = ~s1 | ~s2;
						break;
					case 0x8:
						res = s1 & s2;
						break;
					case 0x9:
						res = ~s1 ^ s2;
						break;
					case 0xa:
						res = s2;
						break;
					case 0xb:
						res = ~s1 | s2;
						break;
					case 0xc:
						res = s1;
						break;
					case 0xd:
						res = s1 | ~s2;
						break;
					case 0xe:
						res = s1 | s2;
						break;
					case 0xf:
						res = 0xffffffff;
						break;
				}
				cr = cond_s(res);
			} else if (op == 0x48 || op == 0x58) {
				res = vp1_min(s1, s2s);
				cr = cond_s_f(res, s1);
			} else if (op == 0x49 || op == 0x59) {
				res = vp1_max(s1, s2s);
				cr = cond_s_f(res, s1);
			} else if (op == 0x4a || op == 0x5a) {
				res = vp1_abs(s1);
				cr = cond_s_f(res, s1);
			} else if (op == 0x4b || op == 0x5b) {
				res = -s1;
				cr = cond_s_f(res, 0);
			} else if (op == 0x4c || op == 0x5c) {
				res = s1 + s2s;
				cr = cond_s_f(res, s1);
			} else if (op == 0x4d || op == 0x5d) {
				res = s1 - s2s;
				cr = cond_s_f(res, s1);
			} else if (op == 0x4e || op == 0x5e) {
				res = vp1_shr(s1, s2s, op == 0x5e);
				cr = cond_s_f(res, s1);
			}
			write_r(ctx, dst, res);
			write_c_s(ctx, cdst, cr);
			break;
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
				res = vp1_abs(s1);
				cr = cond_s_f(res, s1);
			} else if (op == 0x7b) {
				res = -s1;
				cr = cond_s_f(res, 0);
			}
			write_r(ctx, dst, res);
			write_c_s(ctx, cdst, cr);
			break;
		case 0x65:
			write_r(ctx, dst, extrs(opcode, 0, 19));
			break;
		case 0x75:
			write_r(ctx, dst, (read_r(ctx, dst) & 0xffff) | (opcode & 0xffff) << 16);
			break;
		case 0x45:
			write_r(ctx, src1, ((int32_t)read_r(ctx, src1) >> 4));
			break;
		case 0x1f:
		case 0x2f:
		case 0x3f:
		case 0x40:
		case 0x43:
		case 0x44:
		case 0x46:
		case 0x47:
		case 0x50:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x56:
		case 0x57:
		case 0x5f:
		case 0x60:
		case 0x66:
		case 0x67:
		case 0x6f:
		case 0x70:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x76:
		case 0x77:
		case 0x7f:
			write_c_s(ctx, cdst, 0);
			break;
	}
}

static int test_isa_s(struct hwtest_ctx *ctx) {
	int i, j;
	nva_wr32(ctx->cnum, 0x200, 0xfffffffd);
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	for (i = 0; i < 1000000; i++) {
		uint32_t opcode = (uint32_t)jrand48(ctx->rand48);
		uint32_t op = opcode >> 24 & 0x7f;
		if (op == 0x6a || op == 0x6b)
			continue;
		struct vp1_ctx octx, ectx, nctx;
		for (j = 0; j < 31; j++) {
			uint32_t val = jrand48(ctx->rand48);
			int which = jrand48(ctx->rand48) & 0xf;
			if (which < 4) {
				val &= ~(0x7f << which * 8);
			}
			octx.r[j] = val;
		}
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
