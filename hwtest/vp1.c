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
	uint32_t uc_cfg;
	uint32_t a[32];
	uint32_t r[31];
	uint32_t v[32][4];
	uint32_t vc[4];
	uint32_t va[16];
	uint32_t b[4];
	uint32_t c[4];
	uint32_t m[64];
	uint32_t x[16];
};

struct vp1_s2v {
	uint8_t vcsel;
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

static void write_v(struct vp1_ctx *ctx, int idx, uint8_t *val) {
	int i, j;
	for (i = 0; i < 4; i++) {
		uint32_t tmp = 0;
		for (j = 0; j < 4; j++) {
			tmp |= val[i*4 + j] << j * 8;
		}
		ctx->v[idx][i] = tmp;
	}
}

static void read_v(struct vp1_ctx *ctx, int idx, uint8_t *val) {
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			val[i*4 + j] = ctx->v[idx][i] >> 8 * j;
		}
	}
}

static void read_v16(struct vp1_ctx *ctx, int idx, uint16_t *val) {
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 2; j++) {
			val[i*2 + j] = ctx->v[idx][i] >> 16 * j;
		}
	}
}

static void write_c_a_f(struct vp1_ctx *ctx, int idx, uint32_t val) {
	uint32_t cr = 0;
	if (val & 0x80000000)
		cr |= 0x100;
	if (!val)
		cr |= 0x200;
	if (idx < 4) {
		ctx->c[idx] &= ~0x300;
		ctx->c[idx] |= cr;
	}
}

static void write_c_a_h(struct vp1_ctx *ctx, int idx, uint32_t val) {
	uint32_t cr = 0;
	uint32_t lo = val & 0xffff;
	uint32_t hi = val >> 16 & 0x3fff;
	if (lo >= hi)
		cr |= 0x400;
	if (idx < 4) {
		ctx->c[idx] &= ~0x400;
		ctx->c[idx] |= cr;
	}
}

static void write_c_s(struct vp1_ctx *ctx, int idx, uint32_t val) {
	if (idx < 4) {
		ctx->c[idx] &= ~0xff;
		ctx->c[idx] |= val;
	}
}

static uint32_t cond_s(uint32_t val, int chipset) {
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
	if (chipset != 0x50)
		res &= 0x3f;
	return res;
}

static uint32_t cond_s_f(uint32_t res, uint32_t s1, int chipset) {
	uint32_t cr = cond_s(res, chipset);
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

static int vp1_mangle_reg(int reg, int cond, int flag) {
	if (flag == 4) {
		return (reg & 0x1c) | ((reg + (cond >> flag)) & 3);
	} else {
		return reg ^ ((cond >> flag) & 1);
	}
}

static uint32_t vp1_hadd(uint32_t a, uint32_t b) {
	return (a & ~0xffff) | ((a + b) & 0xffff);
}

static uint32_t vp1_logop(uint32_t s1, uint32_t s2, int logop) {
	uint32_t res = 0;
	switch (logop) {
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
	return res;
}

static void simulate_op_a(struct vp1_ctx *octx, struct vp1_ctx *ctx, uint32_t opcode) {
	uint32_t op = opcode >> 24 & 0x1f;
	uint32_t src1 = opcode >> 14 & 0x1f;
	uint32_t src2 = opcode >> 9 & 0x1f;
	uint32_t dst = opcode >> 19 & 0x1f;
	uint32_t s1 = octx->a[src1];
	uint32_t s2 = octx->a[src2];
	int flag = opcode >> 5 & 0xf;
	uint32_t cond = octx->c[opcode >> 3 & 3];
	uint32_t res;
	int src2s = vp1_mangle_reg(src2, cond, flag);
	uint32_t s2s = octx->a[src2s];
	switch (op) {
		case 0x0a:
			res = octx->a[dst];
			res = vp1_hadd(res, s2s);
			ctx->a[dst] = res;
			write_c_a_h(ctx, opcode & 7, res);
			break;
		case 0x0b:
			res = s1 + s2s;
			ctx->a[dst] = res;
			write_c_a_f(ctx, opcode & 7, res);
			break;
		case 0x0c:
			ctx->a[dst] &= ~0xffff;
			ctx->a[dst] |= opcode & 0xffff;
			break;
		case 0x0d:
			ctx->a[dst] &= ~0xffff0000;
			ctx->a[dst] |= opcode << 16 & 0xffff0000;
			break;
		case 0x13:
			res = vp1_logop(s1, s2, opcode >> 3 & 0xf);
			ctx->a[dst] = res;
			write_c_a_f(ctx, opcode & 7, res);
			break;
	}
}

static void simulate_op_s(struct vp1_ctx *octx, struct vp1_ctx *ctx, uint32_t opcode, struct vp1_s2v *s2v, int chipset, int op_b) {
	uint32_t op = opcode >> 24 & 0x7f;
	uint32_t src1 = opcode >> 14 & 0x1f;
	uint32_t src2 = opcode >> 9 & 0x1f;
	uint32_t dst = opcode >> 19 & 0x1f;
	uint32_t imm = extrs(opcode, 3, 11);
	uint32_t cdst = opcode & 7;
	uint32_t cr, res, s1, s2, s2s;
	int flag = opcode >> 5 & 0xf;
	int32_t sub, ss1, ss2;
	int i;
	int u;
	uint32_t cond = octx->c[opcode >> 3 & 3];
	int src2s = vp1_mangle_reg(src2, cond, flag);
	int rfile = opcode >> 3 & 0x1f;
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
			s1 = read_r(octx, src1);
			s2 = read_r(octx, src2s);
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
			s1 = read_r(octx, src1);
			s2 = read_r(octx, src2);
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
			s1 = read_r(octx, src1);
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
			s1 = read_r(octx, src1);
			s2 = read_r(octx, src2);
			s2s = read_r(octx, src2s);
			if (op == 0x41 || op == 0x51) {
				res = sext(s1, 15) * sext(s2s, 15);
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x42) {
				res = vp1_logop(s1, s2, opcode >> 3 & 0xf);
				cr = cond_s(res, chipset);
			} else if (op == 0x48 || op == 0x58) {
				res = vp1_min(s1, s2s);
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x49 || op == 0x59) {
				res = vp1_max(s1, s2s);
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x4a || op == 0x5a) {
				res = vp1_abs(s1);
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x4b || op == 0x5b) {
				res = -s1;
				cr = cond_s_f(res, 0, chipset);
			} else if (op == 0x4c || op == 0x5c) {
				res = s1 + s2s;
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x4d || op == 0x5d) {
				res = s1 - s2s;
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x4e || op == 0x5e) {
				res = vp1_shr(s1, s2s, op == 0x5e);
				cr = cond_s_f(res, s1, chipset);
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
			s1 = read_r(octx, src1);
			if (op == 0x61 || op == 0x71) {
				res = sext(s1, 15) * imm;
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x62) {
				res = s1 & imm;
				cr = cond_s(res, chipset);
			} else if (op == 0x63) {
				res = s1 ^ imm;
				cr = cond_s(res, chipset);
			} else if (op == 0x64) {
				res = s1 | imm;
				cr = cond_s(res, chipset);
			} else if (op == 0x68 || op == 0x78) {
				res = vp1_min(s1, imm);
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x69 || op == 0x79) {
				res = vp1_max(s1, imm);
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x6c || op == 0x7c) {
				res = s1 + imm;
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x6d || op == 0x7d) {
				res = s1 - imm;
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x6e || op == 0x7e) {
				res = vp1_shr(s1, imm, op == 0x7e);
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x7a) {
				res = vp1_abs(s1);
				cr = cond_s_f(res, s1, chipset);
			} else if (op == 0x7b) {
				res = -s1;
				cr = cond_s_f(res, 0, chipset);
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
			s2v->vcsel = 0x80 | (opcode >> 19 & 0x1f) | (opcode << 5 & 0x20);
			break;
		case 0x6a:
			switch (rfile) {
				case 0x00:
				case 0x01:
				case 0x02:
				case 0x03:
				case 0x12:
					ctx->v[dst][rfile & 3] = read_r(ctx, src1);
					break;
				case 0x0b:
					if (dst < 4) 
						ctx->b[dst] = read_r(ctx, src1) & 0xffff;
					break;
				case 0x0c:
					ctx->a[dst] = read_r(ctx, src1);
					break;
				case 0x14:
					ctx->m[dst] = read_r(ctx, src1);
					break;
				case 0x15:
					ctx->m[32 + dst] = read_r(ctx, src1);
					break;
				case 0x18:
					ctx->x[dst & 0xf] = read_r(ctx, src1);
					break;
			}
			write_c_s(ctx, cdst, 0);
			break;
		case 0x6b:
			switch (rfile) {
				case 0x00:
				case 0x01:
				case 0x02:
				case 0x03:
					write_r(ctx, dst, octx->v[src1][rfile]);
					break;
				case 0x0b:
					if (op_b != 0x1f)
						write_r(ctx, dst, octx->b[src1 & 3]);
					break;
				case 0x0c:
					write_r(ctx, dst, octx->a[src1]);
					break;
				case 0x0d:
					write_r(ctx, dst, src1 < 4 ? octx->c[src1] : 0);
					break;
				case 0x14:
					write_r(ctx, dst, octx->m[src1]);
					break;
				case 0x15:
					write_r(ctx, dst, octx->m[32 + src1]);
					break;
				case 0x18:
					write_r(ctx, dst, octx->x[src1 & 0xf]);
					break;
			}
			write_c_s(ctx, cdst, 0);
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
		case 0x04:
		case 0x05:
		case 0x0f:
		case 0x24:
			s2v->vcsel = 0x80 | (opcode >> 19 & 0x1f) | (opcode << 5 & 0x20);
			break;
	}
}

static void simulate_op_v(struct vp1_ctx *octx, struct vp1_ctx *ctx, uint32_t opcode, struct vp1_s2v *s2v) {
	uint32_t op = opcode >> 24 & 0x3f;
	uint32_t src1 = opcode >> 14 & 0x1f;
	uint32_t src2 = opcode >> 9 & 0x1f;
	uint32_t src3 = opcode >> 4 & 0x1f;
	uint32_t dst = opcode >> 19 & 0x1f;
	uint32_t imm = opcode >> 3 & 0xff;
	uint32_t vci = opcode & 7;
	uint8_t s1[16];
	uint8_t s2[16];
	uint8_t s3[16];
	uint16_t s16[16];
	uint8_t d[16];
	int32_t ss1, ss2, ss3, sub, sres;
	uint32_t cr = 0;
	uint16_t scond;
	int i;
	int shift;
	int flag = opcode >> 5 & 0xf;
	uint32_t cond = octx->c[opcode >> 3 & 3];
	int src2s = vp1_mangle_reg(src2, cond, flag);
	int vcsel = opcode & 3, vcpart = 0, vcmode = 0;
	int subop = op & 0xf;
	if (s2v->vcsel) {
		vcsel = s2v->vcsel & 3;
		vcpart = s2v->vcsel >> 2 & 1;
		vcmode = s2v->vcsel >> 3 & 7;
	}
	switch (op) {
		case 0x00:
		case 0x20:
		case 0x30:
		case 0x01:
		case 0x11:
		case 0x21:
		case 0x31:
		case 0x02:
		case 0x12:
		case 0x22:
		case 0x32:
		case 0x03:
		case 0x13:
		case 0x23:
			read_v(octx, src1, s1);
			read_v(octx, src2, s2);
			for (i = 0; i < 16; i++) {
				int n = !!(opcode & 8);
				ss1 = s1[i];
				ss2 = s2[i];
				if (op & 0x20) {
					if (op == 0x30)
						ss2 = opcode & 0xff;
					else
						ss2 = ((opcode & 1) << 5 | src2) << 2;
				}
				if (opcode & 4)
					ss1 = (int8_t)ss1 << (n ? 0 : 1);
				if (opcode & 2)
					ss2 = (int8_t)ss2 << (n ? 0 : 1);
				shift = extrs(opcode, 5, 3);
				sub = ss1 * ss2;
				if (n)
					sub <<= 8;
				int rshift = -shift;
				if (n)
					rshift += 7;
				else if (op & 0x10)
					rshift -= 1;
				int hlshift = rshift + 1;
				if (!(opcode & 0x10))
					rshift += 8;
				if (opcode & 0x100 && rshift >= 0) {
					sub += 1 << rshift;
					if (octx->uc_cfg & 1)
						sub--;
				}
				if (subop == 2 || subop == 3) {
					sub += ctx->va[i];
				}
				ctx->va[i] = sub;
				if (subop == 1 || subop == 2) {
					if (hlshift >= 0)
						sres = sub >> hlshift;
					else
						sres = sub << -hlshift;
					if (op & 0x10) {
						if (sres < -0)
							sres = -0;
						if (sres > 0xffff)
							sres = 0xffff;
					} else {
						if (sres < -0x8000)
							sres = -0x8000;
						if (sres > 0x7fff)
							sres = 0x7fff;
					}
					if (!(opcode & 0x10))
						sres >>= 8;
					d[i] = sres;
				}
			}
			if (subop == 1 || subop == 2)
				write_v(ctx, dst, d);
			break;
		case 0x2a:
		case 0x2b:
		case 0x2f:
		case 0x3a:
			read_v(octx, src1, s1);
			for (i = 0; i < 16; i++) {
				if (op == 0x2a)
					d[i] = s1[i] & imm;
				else if (op == 0x2b)
					d[i] = s1[i] ^ imm;
				else if (op == 0x2f)
					d[i] = s1[i] | imm;
				else if (op == 0x3a)
					d[i] = s1[i];
				if (!d[i])
					cr |= 1 << (16 + i);
			}
			write_v(ctx, dst, d);
			if (vci < 4)
				ctx->vc[vci] = cr;
			break;
		case 0x0f:
			read_v(octx, src1, s1);
			read_v(octx, src1 | 1, s2);
			read_v(octx, src2s, s3);
			cond = octx->vc[vcsel] >> (vcpart * 16) & 0xffff;
			cond |= octx->vc[vcsel | 1] >> (vcpart * 16) << 16;
			static const int tab[8][16] = {
				{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
				{  2,  2,  2,  2,  6,  6,  6,  6, 10, 10, 10, 10, 14, 14, 14, 14 },
				{  4,  5,  4,  5,  4,  5,  4,  5, 12, 13, 12, 13, 12, 13, 12, 13 },
				{  0,  0,  2,  0,  4,  4,  6,  4,  8,  8, 10,  8, 12, 12, 14, 12 },
				{  1,  1,  1,  3,  5,  5,  5,  7,  9,  9,  9, 11, 13, 13, 13, 15 },
				{  0,  0,  2,  2,  4,  4,  6,  6,  8,  8, 10, 10, 12, 12, 14, 14 },
				{  1,  1,  1,  1,  5,  5,  5,  5,  9,  9,  9,  9, 13, 13, 13, 13 },
				{  0,  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 },
			};
			scond = 0;
			for (i = 0; i < 16; i++) {
				if (cond & 1 << tab[vcmode][i])
					scond |= 1 << i;
			}
			for (i = 0; i < 16; i++) {
				ss1 = vp1_abs(s1[i] - s3[i]);
				if (ss1 == s2[i])
					cr |= 1 << (16 + i);
				cond = ss1 < s2[i];
				cond <<= 1;
				cond |= scond >> i & 1;
				if (opcode & 1 << (19 + cond))
					cr |= 1 << i;
			}
			if (vci < 4)
				ctx->vc[vci] = cr;
			break;
		case 0x10:
			read_v(octx, src1, s1);
			read_v(octx, src1 | 1, s2);
			read_v(octx, src2, s3);
			shift = extrs(opcode, 5, 3);
			for (i = 0; i < 16; i++) {
				int32_t factor = s3[i] << (shift + 4);
				sres = s2[i] * (0x1000 - factor) + s1[i] * factor;
				if (opcode & 0x100) {
					sres += 0x7ff + !(ctx->uc_cfg & 1);
				}
				if (sres > 0xfffff)
					sres = 0xfffff;
				if (sres < 0)
					sres = 0;
				sres >>= 12;
				d[i] = sres;
			}
			write_v(ctx, dst, d);
			break;
		case 0x1b:
			read_v(octx, src1, s1);
			read_v(octx, src2, s2);
			read_v(octx, src3, s3);
			for (i = 0; i < 16; i++) {
				int comp, which;
				if (opcode & 8) {
					comp = s3[i] >> 4 & 0xf;
					which = s3[i] & 1;
				} else {
					comp = s3[i] & 0xf;
					which = s3[i] >> 4 & 1;
				}
				if (which) {
					d[i] = s2[comp];
				} else {
					d[i] = s1[comp];
				}
			}
			write_v(ctx, dst, d);
			break;
		case 0x1f:
			read_v(octx, src1, s1);
			read_v16(octx, src2, s16);
			read_v16(octx, src3, s16+8);
			for (i = 0; i < 16; i++) {
				if (s16[i] & 0x100)
					ss2 = s16[i] | -0x100;
				else
					ss2 = s16[i] & 0xff;
				sres = s1[i] + ss2;
				if (sres & 0x100)
					cr |= 1 << i;
				if (sres < 0)
					sres = 0;
				if (sres > 0xff)
					sres = 0xff;
				d[i] = sres;
				if (!d[i])
					cr |= 1 << (16 + i);
			}
			write_v(ctx, dst, d);
			if (vci < 4)
				ctx->vc[vci] = cr;
			break;
		case 0x24:
		case 0x25:
			read_v(octx, src1, s1);
			read_v(octx, src2, s2);
			read_v(octx, src3, s3);
			for (i = 0; i < 16; i++) {
				ss1 = (int8_t) s1[i];
				ss2 = (int8_t) s2[i];
				ss3 = (int8_t) s3[i];
				if (op == 0x25) {
					ss1 = vp1_abs(ss1);
					ss2 = vp1_abs(ss2);
					sres = vp1_min(ss1, ss2);
					if (sres == 0x80)
						sres = 0x7f;
					if (sres & 0x80)
						cr |= 1 << i;
				} else {
					if (ss1 > ss3 && ss2 > ss3) {
						sres = vp1_min(ss1, ss2);
					} else if (ss1 < ss3 && ss2 < ss3) {
						sres = vp1_max(ss1, ss2);
					} else {
						sres = ss3;
					}
					if (!(ss2 < ss1 && ss1 < ss3))
						cr |= 1 << i;
				}
				d[i] = sres;
				if (!d[i])
					cr |= 1 << (16 + i);
			}
			write_v(ctx, dst, d);
			if (vci < 4)
				ctx->vc[vci] = cr;
			break;
		case 0x14:
			read_v(octx, src1, s1);
			read_v(octx, src2, s2);
			for (i = 0; i < 16; i++) {
				d[i] = vp1_logop(s1[i], s2[i], opcode >> 3 & 0xf) & 0xff;
				if (!d[i])
					cr |= 1 << (16 + i);
			}
			write_v(ctx, dst, d);
			if (vci < 4)
				ctx->vc[vci] = cr;
			break;
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
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x28:
		case 0x29:
		case 0x2c:
		case 0x2e:
		case 0x38:
		case 0x39:
		case 0x3c:
		case 0x3d:
		case 0x3e:
			read_v(octx, src1, s1);
			read_v(octx, src2, s2);
			for (i = 0; i < 16; i++) {
				ss1 = s1[i];
				ss2 = s2[i];
				if (op & 0x20) {
					ss2 = imm;
				}
				if (!(op & 0x10)) {
					ss1 = (int8_t)ss1;
					ss2 = (int8_t)ss2;
				}
				switch (op & 0xf) {
					case 0x8:
						sres = vp1_min(ss1, ss2);
						break;
					case 0x9:
						sres = vp1_max(ss1, ss2);
						break;
					case 0xa:
						sres = vp1_abs(ss1);
						break;
					case 0xb:
						sres = -ss1;
						break;
					case 0xc:
						sres = ss1 + ss2;
						break;
					case 0xd:
						sres = ss1 - ss2;
						break;
					case 0xe:
						sres = vp1_shrb(ss1, ss2);
						if (sres & 0x80)
							cr |= 1 << i;
						break;
				}
				if ((op & 0xf) != 0xe) {
					if (op & 0x10) {
						if (sres & 0x100)
							cr |= 1 << i;
						if (sres >= 0x100)
							sres = 0x100 - 1;
						if (sres < 0)
							sres = 0;
					} else {
						if (sres < 0)
							cr |= 1 << i;
						if (sres >= 0x80)
							sres = 0x80 - 1;
						if (sres < -0x80)
							sres = -0x80;
					}
				}
				d[i] = sres;
				if (!d[i])
					cr |= 1 << (16 + i);
			}
			write_v(ctx, dst, d);
			if (vci < 4)
				ctx->vc[vci] = cr;
			break;
		case 0x2d:
			for (i = 0; i < 16; i++) {
				d[i] = imm;
				if (!d[i])
					cr |= 1 << (16 + i);
				if (d[i] & 0x80)
					cr |= 1 << i;
			}
			write_v(ctx, dst, d);
			if (vci < 4)
				ctx->vc[vci] = cr;
			break;
		case 0x3b:
			for (i = 0; i < 4; i++)
				ctx->v[dst][i] = ctx->vc[i];
			break;
	}
}

static void simulate_op_b(struct vp1_ctx *octx, struct vp1_ctx *ctx, uint32_t opcode) {
	uint32_t op = opcode >> 24 & 0x1f;
	uint32_t cr = 0x2000;
	uint32_t cond = opcode & 7;
	uint32_t val;
	switch (op) {
		case 0x01:
		case 0x03:
		case 0x05:
		case 0x07:
			val = octx->b[opcode >> 3 & 3];
			if (val & 0xff) {
				val -= 1;
			} else {
				val |= val >> 8;
			}
			ctx->b[cond & 3] = val;
			if (val & 0xff)
				cr = 0;
			break;
		case 0x10:
			cond = opcode >> 19 & 3;
			ctx->b[cond] = opcode & 0xffff;
			if (opcode & 0xff)
				cr = 0;
			break;
	}
	if (cond < 4 && op != 0x0f && op != 0x0a && op != 0x1f) {
		ctx->c[cond] &= ~0x2000;
		ctx->c[cond] |= cr;
	}
}

static void read_va(struct hwtest_ctx *ctx, uint32_t *va) {
	int i;
	/*
	 * Read vector accumulator.
	 *
	 * This is a complex process, performed in seven steps for each component:
	 *
	 * 1. Prepare consts in vector registers: 0, -1, 0.5
	 * 2. As long as the value is non-negative (as determined by signed readout),
	 *    decrease it by 1.
	 * 3. As long as the value is negative, increase it by 1.
	 * 4. The value is now in [0, 1) range. Determine original integer part
	 *    from the number of increases and decreases.
	 * 5. Read out high 8 bits of the fractional part, using unsigned readout.
	 * 6. Read out low 8 bits of the fractional part, using unsigned readout.
	 * 7. Increase/decrease by 1 the correct number of times to restore original
	 *    integer part.
	 */
	for (i = 0; i < 16; i++) {
		/* prepare the consts */
		uint8_t consts[3][16] = { 0 };
		int j, k, l;
		consts[1][i] = 0x80;
		consts[2][i] = 0x40;
		for (j = 0; j < 3; j++) {
			for (k = 0; k < 4; k++) {
				uint32_t val = 0;
				for (l = 0; l < 4; l++)
					val |= consts[j][k * 4 + l] << l * 8;
				nva_wr32(ctx->cnum, 0xf000 + j * 4 + k * 0x80, val);
			}
		}
		/* init int part counter */
		int ctr = 0;
		/* while non-negative, decrement... */
		while (1) {
			/* read */
			nva_wr32(ctx->cnum, 0xf450, 0x82180000); /* $v3 = $va += 0 * 0, signed */
			nva_wr32(ctx->cnum, 0xf458, 1);
			uint8_t val = nva_rd32(ctx->cnum, 0xf00c + (i >> 2) * 0x80) >> (i & 3) * 8;
			/* if negative, break */
			if (val & 0x80)
				break;
			nva_wr32(ctx->cnum, 0xf450, 0x82184406); /* $va += -1 * 0.5 */
			nva_wr32(ctx->cnum, 0xf458, 1);
			nva_wr32(ctx->cnum, 0xf450, 0x82184406); /* $va += -1 * 0.5 */
			nva_wr32(ctx->cnum, 0xf458, 1);
			ctr++;
		}
		/* while negative, increment... */
		while (1) {
			/* read */
			nva_wr32(ctx->cnum, 0xf450, 0x82180000); /* $v3 = $va += 0 * 0, signed */
			nva_wr32(ctx->cnum, 0xf458, 1);
			uint8_t val = nva_rd32(ctx->cnum, 0xf00c + (i >> 2) * 0x80) >> (i & 3) * 8;
			/* if non-negative, break */
			if (!(val & 0x80))
				break;
			nva_wr32(ctx->cnum, 0xf450, 0x82184206); /* $va += -1 * -1 */
			nva_wr32(ctx->cnum, 0xf458, 1);
			ctr--;
		}
		/* read high */
		nva_wr32(ctx->cnum, 0xf450, 0x92180000); /* $v3 = #va += 0 * 0, unsigned */
		nva_wr32(ctx->cnum, 0xf458, 1);
		uint8_t fh = nva_rd32(ctx->cnum, 0xf00c + (i >> 2) * 0x80) >> (i & 3) * 8;
		/* read low */
		nva_wr32(ctx->cnum, 0xf450, 0x92180010); /* $v3 = #va += 0 * 0, unsigned, low */
		nva_wr32(ctx->cnum, 0xf458, 1);
		uint8_t fl = nva_rd32(ctx->cnum, 0xf00c + (i >> 2) * 0x80) >> (i & 3) * 8;
		/* write the result */
		va[i] = ctr << 16 | fh << 8 | fl;
		/* restore */
		while (ctr > 0) {
			nva_wr32(ctx->cnum, 0xf450, 0x82184206); /* $va += -1 * -1 */
			nva_wr32(ctx->cnum, 0xf458, 1);
			ctr--;
		}
		while (ctr < 0) {
			nva_wr32(ctx->cnum, 0xf450, 0x82184406); /* $va += -1 * 0.5 */
			nva_wr32(ctx->cnum, 0xf458, 1);
			nva_wr32(ctx->cnum, 0xf450, 0x82184406); /* $va += -1 * 0.5 */
			nva_wr32(ctx->cnum, 0xf458, 1);
			ctr++;
		}
	}
}

static int test_isa_s(struct hwtest_ctx *ctx) {
	int i, j, k;
	nva_wr32(ctx->cnum, 0x200, 0xfffffffd);
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	/* clear $va */
	nva_wr32(ctx->cnum, 0xf000, 0x00000000);
	nva_wr32(ctx->cnum, 0xf080, 0x00000000);
	nva_wr32(ctx->cnum, 0xf100, 0x00000000);
	nva_wr32(ctx->cnum, 0xf180, 0x00000000);
	nva_wr32(ctx->cnum, 0xf450, 0x80000000);
	nva_wr32(ctx->cnum, 0xf458, 0x00000001);
	for (i = 0; i < 10000000; i++) {
		struct vp1_s2v s2v = { 0 };
		uint32_t opcode_a = (uint32_t)jrand48(ctx->rand48);
		uint32_t opcode_s = (uint32_t)jrand48(ctx->rand48);
		uint32_t opcode_v = (uint32_t)jrand48(ctx->rand48);
		uint32_t opcode_b = (uint32_t)jrand48(ctx->rand48);
		if (!(jrand48(ctx->rand48) & 0xf)) {
			/* help luck a bit */
			opcode_s &= ~0x7e000000;
			opcode_s |= 0x6a000000;
		}
		uint32_t op_a = opcode_a >> 24 & 0x1f;
		uint32_t op_s = opcode_s >> 24 & 0x7f;
		uint32_t op_v = opcode_v >> 24 & 0x3f;
		uint32_t op_b = opcode_b >> 24 & 0x1f;
		if (op_s == 0x6a || op_s == 0x6b) {
			int rfile = opcode_s >> 3 & 0x1f;
			int reg = opcode_s >> 14 & 0x1f;
			if (
				rfile == 8 ||
				rfile == 9 ||
				rfile == 0xa ||
				rfile == 0x16 ||
				rfile == 0x17)
				opcode_s = 0x4f000000;
		}
		if (
			op_v == 0x07 || /* $va */
			op_v == 0x17 || /* $va */
			op_v == 0x27 || /* $va */
			op_v == 0x37 || /* $va */
			op_v == 0x36 || /* $va */
			op_v == 0x04 || /* $va */
			op_v == 0x06 || /* $va */
			op_v == 0x16 || /* $va */
			op_v == 0x26 || /* $va */
			op_v == 0x34 || /* $va */
			op_v == 0x35 || /* $va */
			op_v == 0x05 || /* use scalar input */
			op_v == 0x15 || /* use scalar input */
			op_v == 0x33 || /* use scalar input */
			0)
			opcode_v = 0xbf000000;
		if (
			op_a == 0x00 || /* vector load + autoincr */
			op_a == 0x01 || /* vector load + autoincr */
			op_a == 0x02 || /* scalar load + autoincr */
			op_a == 0x03 || /* [xdld] */
			op_a == 0x04 || /* [store] */
			op_a == 0x05 || /* [store] */
			op_a == 0x06 || /* [store] */
			op_a == 0x07 || /* [xdst] fuckup */
			op_a == 0x08 || /* vector load + autoincr */
			op_a == 0x09 || /* vector load + autoincr */
			op_a == 0x0e || /* [xdbar] fuckup */
			op_a == 0x0f || /* [xdwait] fuckup */
			op_a == 0x10 || /* vector load + autoincr */
			op_a == 0x11 || /* vector load + autoincr */
			op_a == 0x12 || /* scalar load + autoincr */
			op_a == 0x14 || /* [store] */
			op_a == 0x15 || /* [store] */
			op_a == 0x16 || /* [store] */
			op_a == 0x17 || /* vector load */
			op_a == 0x18 || /* vector load */
			op_a == 0x19 || /* vector load */
			op_a == 0x1a || /* scalar load */
			op_a == 0x1b || /* fuckup */
			op_a == 0x1c || /* [store] */
			op_a == 0x1d || /* [store] */
			op_a == 0x1e) /* [store] */
			opcode_a = 0xdf000000;
		struct vp1_ctx octx, ectx, nctx;
		octx.uc_cfg = jrand48(ctx->rand48) & 0x111;
		for (j = 0; j < 16; j++) {
			uint32_t val = jrand48(ctx->rand48);
			int which = jrand48(ctx->rand48) & 0xf;
			if (which < 4) {
				val &= ~(0x7f << which * 8);
			}
			octx.x[j] = val;
		}
		for (j = 0; j < 32; j++) {
			uint32_t val = jrand48(ctx->rand48);
			int which = jrand48(ctx->rand48) & 0xf;
			if (which < 4) {
				val &= ~(0x7f << which * 8);
			}
			octx.a[j] = val;
		}
		for (j = 0; j < 31; j++) {
			uint32_t val = jrand48(ctx->rand48);
			int which = jrand48(ctx->rand48) & 0xf;
			if (which < 4) {
				val &= ~(0x7f << which * 8);
			}
			octx.r[j] = val;
		}
		for (j = 0; j < 32; j++) {
			for (k = 0; k < 4; k++) {
				uint32_t val = jrand48(ctx->rand48);
				int which = jrand48(ctx->rand48) & 0xf;
				if (which < 4) {
					val &= ~(0x7f << which * 8);
				}
				octx.v[j][k] = val;
			}
		}
		for (j = 0; j < 32; j++) {
			uint32_t val = jrand48(ctx->rand48) & 0xffff;;
			int which = jrand48(ctx->rand48) & 0x7;
			if (which < 2) {
				val &= ~(0x7f << which * 8);
			}
			octx.b[j] = val;
		}
		for (j = 0; j < 4; j++)
			octx.c[j] = nva_rd32(ctx->cnum, 0xf680 + j * 4);
		for (j = 0; j < 64; j++) {
			uint32_t val = jrand48(ctx->rand48);
			int which = jrand48(ctx->rand48) & 0xf;
			if (which < 4) {
				val &= ~(0x7f << which * 8);
			}
			octx.m[j] = val;
		}
		read_va(ctx, octx.va);
		nva_wr32(ctx->cnum, 0xf450, 0xbb000000);
		nva_wr32(ctx->cnum, 0xf458, 1);
		octx.vc[0] = nva_rd32(ctx->cnum, 0xf000);
		octx.vc[1] = nva_rd32(ctx->cnum, 0xf080);
		octx.vc[2] = nva_rd32(ctx->cnum, 0xf100);
		octx.vc[3] = nva_rd32(ctx->cnum, 0xf180);
		ectx = octx;
		for (j = 0; j < 16; j++) {
			nva_wr32(ctx->cnum, 0xf780, octx.x[j]);
			nva_wr32(ctx->cnum, 0xf44c, 0x6a0000c0 | j << 19);
			nva_wr32(ctx->cnum, 0xf458, 1);
		}
		nva_wr32(ctx->cnum, 0xf540, octx.uc_cfg);
		for (j = 0; j < 32; j++) {
			nva_wr32(ctx->cnum, 0xf600 + j * 4, octx.a[j]);
		}
		for (j = 0; j < 31; j++) {
			nva_wr32(ctx->cnum, 0xf780 + j * 4, octx.r[j]);
		}
		for (j = 0; j < 32; j++) {
			for (k = 0; k < 4; k++) {
				nva_wr32(ctx->cnum, 0xf000 + j * 4 + k * 0x80, octx.v[j][k]);
			}
		}
		for (j = 0; j < 4; j++) {
			nva_wr32(ctx->cnum, 0xf580 + j * 4, octx.b[j]);
		}
		for (j = 0; j < 64; j++) {
			nva_wr32(ctx->cnum, 0xfa00 + j * 4, octx.m[j]);
		}
		nva_wr32(ctx->cnum, 0xf448, opcode_a);
		nva_wr32(ctx->cnum, 0xf44c, opcode_s);
		nva_wr32(ctx->cnum, 0xf450, opcode_v);
		nva_wr32(ctx->cnum, 0xf454, opcode_b);
		nva_wr32(ctx->cnum, 0xf458, 1);
		simulate_op_a(&octx, &ectx, opcode_a);
		simulate_op_s(&octx, &ectx, opcode_s, &s2v, ctx->chipset, op_b);
		simulate_op_v(&octx, &ectx, opcode_v, &s2v);
		simulate_op_b(&octx, &ectx, opcode_b);
		/* XXX wait? */
		nctx.uc_cfg = nva_rd32(ctx->cnum, 0xf540);
		for (j = 0; j < 32; j++)
			nctx.a[j] = nva_rd32(ctx->cnum, 0xf600 + j * 4);
		for (j = 0; j < 31; j++)
			nctx.r[j] = nva_rd32(ctx->cnum, 0xf780 + j * 4);
		for (j = 0; j < 32; j++)
			for (k = 0; k < 4; k++)
				nctx.v[j][k] = nva_rd32(ctx->cnum, 0xf000 + j * 4 + k * 0x80);
		for (j = 0; j < 4; j++)
			nctx.c[j] = nva_rd32(ctx->cnum, 0xf680 + j * 4);
		for (j = 0; j < 4; j++)
			nctx.b[j] = nva_rd32(ctx->cnum, 0xf580 + j * 4);
		for (j = 0; j < 64; j++)
			nctx.m[j] = nva_rd32(ctx->cnum, 0xfa00 + j * 4);
		nva_wr32(ctx->cnum, 0xf450, 0xbb000000);
		nva_wr32(ctx->cnum, 0xf458, 1);
		nctx.vc[0] = nva_rd32(ctx->cnum, 0xf000);
		nctx.vc[1] = nva_rd32(ctx->cnum, 0xf080);
		nctx.vc[2] = nva_rd32(ctx->cnum, 0xf100);
		nctx.vc[3] = nva_rd32(ctx->cnum, 0xf180);
		for (j = 0; j < 16; j++) {
			nva_wr32(ctx->cnum, 0xf44c, 0x6b0000c0 | j << 14);
			nva_wr32(ctx->cnum, 0xf458, 1);
			nctx.x[j] = nva_rd32(ctx->cnum, 0xf780);
		}
		read_va(ctx, nctx.va);
		if (memcmp(&ectx, &nctx, sizeof ectx)) {
			printf("Mismatch on try %d for insn 0x%08"PRIx32" 0x%08"PRIx32" 0x%08"PRIx32" 0x%08"PRIx32"\n", i, opcode_a, opcode_s, opcode_v, opcode_b);
			printf("what        initial    expected   real\n");
#define PRINT(name, x) printf(name "0x%08x 0x%08x 0x%08x%s\n", octx.x, ectx.x, nctx.x, (nctx.x != ectx.x ? " *" : ""))
#define IPRINT(name, x) printf(name "0x%08x 0x%08x 0x%08x%s\n", j, octx.x, ectx.x, nctx.x, (nctx.x != ectx.x ? " *" : ""))
#define IIPRINT(name, x) printf(name "0x%08x 0x%08x 0x%08x%s\n", j, k, octx.x, ectx.x, nctx.x, (nctx.x != ectx.x ? " *" : ""))
			PRINT("UC_CFG ", uc_cfg);
			for (j = 0; j < 4; j++) {
				IPRINT("VC[%d] ", vc[j]);
			}
			for (j = 0; j < 16; j++) {
				IPRINT("VA[%d] ", va[j]);
			}
			for (j = 0; j < 32; j++) {
				IPRINT("A[0x%02x]   ", a[j]);
			}
			for (j = 0; j < 31; j++) {
				IPRINT("R[0x%02x]   ", r[j]);
			}
			for (j = 0; j < 32; j++) {
				for (k = 0; k < 4; k++) {
					IIPRINT("V[0x%02x][%d]   ", v[j][k]);
				}
			}
			for (j = 0; j < 4; j++) {
				IPRINT("B[%d] ", b[j]);
			}
			for (j = 0; j < 4; j++) {
				IPRINT("C[%d] ", c[j]);
			}
			for (j = 0; j < 64; j++) {
				IPRINT("M[%02x] ", m[j]);
			}
			for (j = 0; j < 16; j++) {
				IPRINT("X[%01x] ", x[j]);
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
