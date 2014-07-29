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
	uint32_t b[4];
	uint32_t c[4];
	uint32_t m[64];
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

static void simulate_op_s(struct vp1_ctx *octx, struct vp1_ctx *ctx, uint32_t opcode, int chipset, int op_b) {
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
	}
}

static void simulate_op_v(struct vp1_ctx *octx, struct vp1_ctx *ctx, uint32_t opcode) {
	uint32_t op = opcode >> 24 & 0x3f;
	uint32_t src1 = opcode >> 14 & 0x1f;
	uint32_t src2 = opcode >> 9 & 0x1f;
	uint32_t dst = opcode >> 19 & 0x1f;
	uint32_t imm = opcode >> 3 & 0xff;
	uint8_t s1[16];
	uint8_t s2[16];
	uint8_t d[16];
	int32_t ss1, ss2, sub, sres;
	int i;
	int shift;
	switch (op) {
		case 0x01:
		case 0x11:
		case 0x21:
		case 0x31:
			read_v(octx, src1, s1);
			read_v(octx, src2, s2);
			for (i = 0; i < 16; i++) {
				int n = !!(opcode & 8);
				ss1 = s1[i];
				ss2 = s2[i];
				if (op & 0x20) {
					ss2 = ((opcode & 1) << 5 | src2) << 2;
				}
				if (opcode & 4)
					ss1 = (int8_t)ss1 << (n ? 0 : 1);
				if (opcode & 2)
					ss2 = (int8_t)ss2 << (n ? 0 : 1);
				sub = ss1 * ss2;
				sres = sub;
				/* to deal with the shift easier */
				sres <<= 4;
				/* first, shift */
				shift = extrs(opcode, 5, 3);
				if (shift < 0) 
					sres >>= -shift;
				else
					sres <<= shift;
				if (op & 0x10 || n) {
					if (opcode & 0x100) {
						if (opcode & 0x10)
							sres += 0x7 + !(ctx->uc_cfg & 1);
						else
							sres += 0x7ff + !(ctx->uc_cfg & 1);
					}
					if (op & 0x10) {
						if (sres < 0)
							sres = 0;
						if (sres >= 0x100000)
							sres = 0x100000 - 1;
					} else {
						if (sres < -0x80000)
							sres = -0x80000;
						if (sres >= 0x80000)
							sres = 0x80000 - 1;
					}
					if (opcode & 0x10)
						sres >>= 4;
					else
						sres >>= 12;
				} else {
					if (opcode & 0x100) {
						if (opcode & 0x10)
							sres += 0xf + !(ctx->uc_cfg & 1);
						else
							sres += 0xfff + !(ctx->uc_cfg & 1);
					}
					if (sres < -0x100000)
						sres = -0x100000;
					if (sres >= 0x100000)
						sres = 0x100000 - 1;
					if (opcode & 0x10)
						sres >>= 5;
					else
						sres >>= 13;
				}
				d[i] = sres;
			}
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
			}
			write_v(ctx, dst, d);
			break;
		case 0x25:
			read_v(octx, src1, s1);
			read_v(octx, src2, s2);
			for (i = 0; i < 16; i++) {
				ss1 = (int8_t) s1[i];
				ss2 = (int8_t) s2[i];
				ss1 = vp1_abs(ss1);
				ss2 = vp1_abs(ss2);
				sres = vp1_min(ss1, ss2);
				if (sres == 0x80)
					sres = 0x7f;
				d[i] = sres;
			}
			write_v(ctx, dst, d);
			break;
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x14:
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
					case 0x4:
						sres = vp1_logop(ss1, ss2, opcode >> 3 & 0xf) & 0xff;
						break;
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
						break;
				}
				if ((op & 0xf) != 0xe) {
					if (op & 0x10) {
						if (sres >= 0x100)
							sres = 0x100 - 1;
						if (sres < 0)
							sres = 0;
					} else {
						if (sres >= 0x80)
							sres = 0x80 - 1;
						if (sres < -0x80)
							sres = -0x80;
					}
				}
				d[i] = sres;
			}
			write_v(ctx, dst, d);
			break;
		case 0x2d:
			for (i = 0; i < 16; i++)
				d[i] = imm;
			write_v(ctx, dst, d);
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

static int test_isa_s(struct hwtest_ctx *ctx) {
	int i, j, k;
	nva_wr32(ctx->cnum, 0x200, 0xfffffffd);
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	for (i = 0; i < 10000000; i++) {
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
				rfile == 0x17 ||
				rfile == 0x18)
				opcode_s = 0x4f000000;
		}
		if (
			op_v == 0x02 ||
			op_v == 0x05 ||
			op_v == 0x07 ||
			op_v == 0x10 ||
			op_v == 0x12 ||
			op_v == 0x15 ||
			op_v == 0x17 ||
			op_v == 0x1b ||
			op_v == 0x1f ||
			op_v == 0x22 ||
			op_v == 0x24 ||
			op_v == 0x27 ||
			op_v == 0x32 ||
			op_v == 0x33 ||
			op_v == 0x36 ||
			op_v == 0x37 ||
			op_v == 0x3b)
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
		ectx = octx;
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
		simulate_op_s(&octx, &ectx, opcode_s, ctx->chipset, op_b);
		simulate_op_v(&octx, &ectx, opcode_v);
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
		if (memcmp(&ectx, &nctx, sizeof ectx)) {
			printf("Mismatch on try %d for insn 0x%08"PRIx32" 0x%08"PRIx32" 0x%08"PRIx32" 0x%08"PRIx32"\n", i, opcode_a, opcode_s, opcode_v, opcode_b);
			printf("what        initial    expected   real\n");
#define PRINT(name, x) printf(name "0x%08x 0x%08x 0x%08x%s\n", octx.x, ectx.x, nctx.x, (nctx.x != ectx.x ? " *" : ""))
#define IPRINT(name, x) printf(name "0x%08x 0x%08x 0x%08x%s\n", j, octx.x, ectx.x, nctx.x, (nctx.x != ectx.x ? " *" : ""))
#define IIPRINT(name, x) printf(name "0x%08x 0x%08x 0x%08x%s\n", j, k, octx.x, ectx.x, nctx.x, (nctx.x != ectx.x ? " *" : ""))
			PRINT("UC_CFG ", uc_cfg);
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
