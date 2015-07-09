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
	uint8_t v[32][16];
	uint32_t vc[4];
	uint32_t va[16];
	uint8_t vx[16];
	uint16_t b[4];
	uint16_t c[4];
	uint32_t m[64];
	uint32_t x[16];
	uint8_t ds[0x10][0x200];
};

struct vp1_s2v {
	uint8_t vcsel;
	uint16_t mask[2];
	int16_t factor[4];
};

struct vp1_ru {
	int stored_v;
	int stored_r;
	int loaded_v;
	int loaded_r;
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

static uint8_t read_ds(struct vp1_ctx *ctx, uint16_t addr, int stride) {
	int bank = addr;
	switch (stride) {
		case 0:
			bank += (addr >> 5) & 7;
			break;
		case 1:
			bank += addr >> 5;
			break;
		case 2:
			bank += addr >> 6;
			break;
		case 3:
			bank += addr >> 7;
			break;
	}
	addr >>= 4;
	bank &= 0xf;
	return ctx->ds[bank][addr];
}

static void write_ds(struct vp1_ctx *ctx, uint16_t addr, int stride, uint8_t val) {
	int bank = addr;
	switch (stride) {
		case 0:
			bank += (addr >> 5) & 7;
			break;
		case 1:
			bank += addr >> 5;
			break;
		case 2:
			bank += addr >> 6;
			break;
		case 3:
			bank += addr >> 7;
			break;
	}
	addr >>= 4;
	bank &= 0xf;
	ctx->ds[bank][addr] = val;
}

static void simulate_op_a(struct vp1_ctx *octx, struct vp1_ctx *ctx, uint32_t opcode, struct vp1_ru *ru) {
	uint32_t op = opcode >> 24 & 0x1f;
	uint32_t src1 = opcode >> 14 & 0x1f;
	uint32_t src2 = opcode >> 9 & 0x1f;
	uint32_t dst = opcode >> 19 & 0x1f;
	uint32_t imm = extrs(opcode, 3, 11);
	uint32_t s1 = octx->a[src1];
	uint32_t s2 = octx->a[src2];
	int flag = opcode >> 5 & 0xf;
	uint32_t cond = octx->c[opcode >> 3 & 3];
	uint32_t res;
	uint32_t addr;
	int stride;
	int src2s = vp1_mangle_reg(src2, cond, flag);
	uint32_t s2s = octx->a[src2s];
	int subop = op & 3;
	int i;
	ru->stored_r = -1;
	ru->loaded_v = -1;
	switch (op) {
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x18:
		case 0x19:
		case 0x1a:
			addr = octx->a[src1];
			if (op & 8) {
				res = vp1_hadd(addr, imm & 0x7ff);
				write_c_a_h(ctx, opcode & 7, res);
				addr = addr | (imm & 0x7ff);
			} else {
				if (op & 0x10) {
					res = vp1_hadd(addr, imm);
				} else {
					res = vp1_hadd(addr, s2s);
				}
				write_c_a_h(ctx, opcode & 7, res);
				ctx->a[src1] = res;
			}
			stride = addr >> 30;
			if (subop == 0) {
				addr &= 0x1ff0;
				for (i = 0; i < 16; i++) {
					ctx->v[dst][i] = read_ds(octx, addr | i, stride);
				}
				ru->loaded_v = dst;
			} else if (subop == 1) {
				addr &= 0x1fff;
				addr &= ~(0xf << (4 + stride));
				for (i = 0; i < 16; i++) {
					ctx->v[dst][i] = read_ds(octx, addr | i << (4 + stride), stride);
				}
				ru->loaded_v = dst;
			} else if (subop == 2) {
				addr &= 0x1ffc;
				res = 0;
				for (i = 0; i < 4; i++) {
					res |= read_ds(octx, addr | i, stride) << 8 * i;
				}
				write_r(ctx, dst, res);
				ru->loaded_r = dst;
			}
			break;
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x1c:
		case 0x1d:
		case 0x1e:
			addr = octx->a[dst];
			if (op & 8) {
				res = vp1_hadd(addr, imm & 0x7ff);
				write_c_a_h(ctx, opcode & 7, res);
				addr = addr | (imm & 0x7ff);
			} else {
				if (op & 0x10) {
					res = vp1_hadd(addr, imm);
				} else {
					res = vp1_hadd(addr, s2s);
				}
				write_c_a_h(ctx, opcode & 7, res);
				ctx->a[dst] = res;
			}
			stride = addr >> 30;
			if (subop == 0) {
				addr &= 0x1ff0;
				int xsrc1 = src1;
				if (ru->stored_v != -1)
					xsrc1 = ru->stored_v;
				for (i = 0; i < 16; i++) {
					write_ds(ctx, addr | i, stride, octx->v[xsrc1][i]);
				}
			} else if (subop == 1) {
				addr &= 0x1fff;
				addr &= ~(0xf << (4 + stride));
				int xsrc1 = src1;
				if (ru->stored_v != -1)
					xsrc1 = ru->stored_v;
				for (i = 0; i < 16; i++) {
					write_ds(ctx, addr | i << (4 + stride), stride, octx->v[xsrc1][i]);
				}
			} else if (subop == 2) {
				addr &= 0x1ffc;
				res = read_r(octx, src1);
				ru->stored_r = src1;
				for (i = 0; i < 4; i++) {
					write_ds(ctx, addr | i, stride, res >> 8 * i);
				}
			}
			break;
		case 0x08:
		case 0x09:
			addr = octx->a[src1];
			res = vp1_hadd(addr, s2s);
			write_c_a_h(ctx, opcode & 7, res);
			ctx->a[src1] = res;
			stride = addr >> 30;
			if (subop == 0) {
				addr &= 0x1ff0;
				for (i = 0; i < 16; i++) {
					ctx->vx[i] = read_ds(octx, addr | i, stride);
				}
			} else if (subop == 1) {
				addr &= 0x1fff;
				addr &= ~(0xf << (4 + stride));
				for (i = 0; i < 16; i++) {
					ctx->vx[i] = read_ds(octx, addr | i << (4 + stride), stride);
				}
			}
			if (cond & 1 << flag) {
				int dsts = (dst & 0x1c) | ((dst + (cond >> 4)) & 3);
				for (i = 0; i < 16; i++) {
					ctx->v[dsts][i] = ctx->vx[i];
				}
			}
			break;
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
		case 0x17:
			if (opcode & 1) {
				addr = octx->a[dst] >> 4 & 0x1ff;
				int xsrc1 = src1;
				if (ru->stored_v != -1)
					xsrc1 = ru->stored_v;
				for (i = 0; i < 16; i++) {
					ctx->ds[i][addr] = octx->v[xsrc1][i];
				}
				ctx->a[dst] = vp1_hadd(ctx->a[dst], s2s);
			} else {
				int xsrc2 = src2;
				if (ru->stored_v != -1)
					xsrc2 = ru->stored_v;
				for (i = 0; i < 16; i++) {
					addr = s1 >> 4 | octx->v[xsrc2][i];
					ctx->v[dst][i] = octx->ds[i][addr & 0x1ff];
				}
				ru->loaded_v = dst;
			}
			break;
	}
}

static void simulate_op_s(struct vp1_ctx *octx, struct vp1_ctx *ctx, uint32_t opcode, struct vp1_s2v *s2v, struct vp1_ru *ru, int chipset, int op_b) {
	uint32_t op = opcode >> 24 & 0x7f;
	uint32_t src1 = opcode >> 14 & 0x1f;
	uint32_t src2 = opcode >> 9 & 0x1f;
	uint32_t dst = opcode >> 19 & 0x1f;
	uint32_t imm = extrs(opcode, 3, 11);
	uint32_t cdst = opcode & 7;
	uint32_t cr, res, s1, s2, s2s;
	int flag = opcode >> 5 & 0xf;
	int32_t sub, ss1, ss2, ss3;
	int i;
	int u;
	uint32_t cond = octx->c[opcode >> 3 & 3];
	int src2s = vp1_mangle_reg(src2, cond, flag);
	int rfile = opcode >> 3 & 0x1f;
	uint16_t mask;
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
					default:
						abort();
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
			} else {
				abort();
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
			} else {
				abort();
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
			} else {
				abort();
			}
			write_r(ctx, dst, res);
			write_c_s(ctx, cdst, cr);
			break;
		case 0x65:
			write_r(ctx, dst, extrs(opcode, 0, 19));
			break;
		case 0x75:
			write_r(ctx, dst, (read_r(octx, dst) & 0xffff) | (opcode & 0xffff) << 16);
			break;
		case 0x6a:
			if (ru->stored_r != -1)
				s1 = read_r(octx, ru->stored_r);
			else
				s1 = read_r(octx, src1);
			switch (rfile) {
				case 0x00:
				case 0x01:
				case 0x02:
				case 0x03:
				case 0x12:
					if (ru->loaded_v == dst)
						break;
					for (i = 0; i < 4; i++)
						ctx->v[dst][(rfile & 3) * 4 + i] = s1 >> i * 8;
					break;
				case 0x0b:
					if (dst < 4) 
						ctx->b[dst] = s1 & 0xffff;
					break;
				case 0x0c:
					ctx->a[dst] = s1;
					break;
				case 0x14:
					ctx->m[dst] = s1;
					break;
				case 0x15:
					ctx->m[32 + dst] = s1;
					break;
				case 0x18:
					ctx->x[dst & 0xf] = s1;
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
					res = 0;
					for (i = 0; i < 4; i++)
						res |= octx->v[src1][(rfile & 3) * 4 + i] << i * 8;
					if (dst != ru->loaded_r)
						write_r(ctx, dst, res);
					break;
				case 0x0b:
					if (dst != ru->loaded_r && op_b != 0x1f)
						write_r(ctx, dst, octx->b[src1 & 3]);
					break;
				case 0x0c:
					if (dst != ru->loaded_r)
						write_r(ctx, dst, octx->a[src1]);
					break;
				case 0x0d:
					if (dst != ru->loaded_r)
						write_r(ctx, dst, src1 < 4 ? octx->c[src1] : 0);
					break;
				case 0x14:
					/* ignore loaded_r */
					write_r(ctx, dst, octx->m[src1]);
					break;
				case 0x15:
					/* ignore loaded_r */
					write_r(ctx, dst, octx->m[32 + src1]);
					break;
				case 0x18:
					/* ignore loaded_r */
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
		case 0x45:
			s1 = read_r(octx, src1);
			mask = 0;
			for (i = 0; i < 4; i++) {
				if (s1 & 1 << i)
					mask |= 0xf << i * 4;
			}
			s2v->mask[0] = mask;
			s2v->mask[1] = 0;
			s2v->factor[0] = (mask & 0xff) << 1;
			s2v->factor[1] = (mask >> 8 & 0xff) << 1;
			s2v->factor[2] = 0;
			s2v->factor[3] = 0;
			write_r(ctx, src1, ((int32_t)s1 >> 4));
			s2v->vcsel = 0x80 | (opcode >> 19 & 0x1f) | (opcode << 5 & 0x20);
			break;
		case 0x04:
		case 0x05:
			/* want to have some fun? */
			if (flag == 4) {
				u = cond >> 4 & 3;
			} else {
				u = cond >> flag & 1;
			}
			/* we're going to have so much fun */
			s1 = read_r(octx, src1);
			s2 = read_r(octx, src2 | u);
			s2s = read_r(octx, src2 | 2 | u);
			ss1 = s1 >> 11 & 0xff;
			if (op == 0x05)
				ss1 &= 0x7f;
			/* are you having fun yet? */
			for (i = 0; i < 4; i++) {
				int ri = i;
				if (op == 0x05) {
					ri &= 2;
					if (flag == 2 && cond & 0x80)
						ri |= 1;
					/* if you're still not having fun, you must be physically incapable of handling fun at all. */
				}
				ss2 = extrs(s2, ri * 8, 8);
				ss3 = extrs(s2s, ri * 8, 8);
				sub = ss2 * 0x100 + ss1 * ss3 + 0x40;
				s2v->factor[i] = sub >> 7;
			}
			/* I hope you had fun. one last thing... */
			s2v->mask[0] = (s2v->factor[0] >> 1 & 0xff) | (s2v->factor[1] << 7 & 0xff00);
			s2v->mask[1] = (s2v->factor[2] >> 1 & 0xff) | (s2v->factor[3] << 7 & 0xff00);
			s2v->vcsel = 0x80 | (opcode >> 19 & 0x1f) | (opcode << 5 & 0x20);
			break;
		case 0x0f:
			s1 = read_r(octx, src1);
			s2v->factor[0] = extrs(s1, 0, 8) << 1;
			s2v->factor[1] = extrs(s1, 8, 8) << 1;
			s2v->factor[2] = extrs(s1, 16, 8) << 1;
			s2v->factor[3] = extrs(s1, 24, 8) << 1;
			s2v->mask[0] = s1 & 0xffff;
			s2v->mask[1] = s1 >> 16 & 0xffff;
			s2v->vcsel = 0x80 | (opcode >> 19 & 0x1f) | (opcode << 5 & 0x20);
			break;
		case 0x24:
			s2v->factor[0] = extrs(opcode, 1, 9);
			s2v->factor[1] = extrs(opcode, 1, 9);
			s2v->factor[2] = extrs(opcode, 10, 9);
			s2v->factor[3] = extrs(opcode, 10, 9);
			s2v->mask[0] = extr(opcode, 2, 8) * 0x0101;
			s2v->mask[1] = extr(opcode, 11, 8) * 0x0101;
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
	uint8_t *s1;
	uint8_t *s2;
	uint8_t *s3;
	uint16_t s16[16];
	uint8_t *d;
	int32_t ss1, ss2, ss3, ss4, sub, sres;
	uint32_t cr = 0;
	int i, j;
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
	uint32_t vcond = octx->vc[vcsel] >> (vcpart * 16) & 0xffff;
	vcond |= octx->vc[vcsel | 1] >> (vcpart * 16) << 16;
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
	uint16_t scond = 0;
	for (i = 0; i < 16; i++) {
		if (vcond & 1 << tab[vcmode][i])
			scond |= 1 << i;
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
		case 0x04:
		case 0x05:
		case 0x15:
		case 0x06:
		case 0x16:
		case 0x26:
		case 0x07:
		case 0x17:
		case 0x27:
			s1 = octx->v[src1];
			s2 = octx->v[src2];
			if (op == 0x16 || op == 0x26 || op == 0x27)
				s3 = octx->v[src3];
			else
				s3 = octx->v[src1 | 1];
			d = ctx->v[dst];
			for (i = 0; i < 16; i++) {
				int n = !!(opcode & 8);
				ss1 = s1[i];
				ss2 = s2[i];
				ss3 = s3[i];
				if (op & 0x20) {
					if (op == 0x30)
						ss2 = opcode & 0xff;
					else
						ss2 = ((opcode & 1) << 5 | src2) << 2;
				}
				if (opcode & 4) {
					ss1 = (int8_t)ss1 << (n ? 0 : 1);
					ss3 = (int8_t)ss3 << (n ? 0 : 1);
				}
				if (opcode & 2)
					ss2 = (int8_t)ss2 << (n ? 0 : 1);
				shift = extrs(opcode, 5, 3);
				if (subop < 4) {
					sub = ss1 * ss2;
					if (n)
						sub <<= 8;
				} else {
					sub = 0;
					if (opcode & 1) {
						if (s2v->mask[0] & 1 << i)
							sub += ss1 << 8;
						if (s2v->mask[1] & 1 << i)
							sub += ss3 << 8;
					} else {
						int cc = scond >> i & 1;
						sub += ss1 * s2v->factor[0 | cc];
						sub += ss3 * s2v->factor[2 | cc];
					}
					if (n)
						sub <<= 8;
					if (subop < 6) {
						if (n)
							sub += ss2 << (16  - shift);
						else if (op & 0x10)
							sub += ss2 << (8 - shift);
						else
							sub += ss2 << (9 - shift);
					}
				}
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
				if (subop == 2 || subop == 3 || subop == 6 || subop == 7) {
					sub += ctx->va[i];
				}
				sub &= 0xfffffff;
				ctx->va[i] = sub;
				sub = sext(sub, 27);
				if (subop == 1 || subop == 2 || subop == 5 || subop == 7) {
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
			break;
		case 0x33:
			i = octx->c[opcode >> 3 & 3] >> 4;
			s1 = octx->v[(src1 & 0x1c) | ((src1 + i) & 3)];
			s2 = octx->v[(src1 & 0x1c) | ((src1 + i + 2) & 3)];
			s3 = octx->v[(src1 & 0x1c) | ((src1 + i + 3) & 3)];
			scond = octx->vc[opcode & 3] >> (opcode >> 2 & 1) * 16;
			d = ctx->v[dst];
			for (i = 0; i < 16; i++) {
				ss1 = s1[i];
				ss2 = s2[i];
				ss3 = s3[i];
				ss4 = s1[i];
				if (opcode & 0x400)
					ss1 ^= 0x80;
				if (opcode & 0x200) {
					ss1 = (int8_t)ss1 << 1;
					ss2 = (int8_t)ss2 << 1;
					ss3 = (int8_t)ss3 << 1;
					ss4 = (int8_t)ss4 << 1;
				}
				sub = 0;
				j = scond >> i & 1;
				sub += s2v->factor[j] * (ss2 - ss4);
				sub += s2v->factor[2 + j] * (ss3 - ss4);
				shift = extrs(opcode, 5, 3);
				int rshift = -shift;
				if (!(opcode & 0x1000))
					rshift -= 1;
				int hlshift = rshift + 1;
				sub += ss1 << (hlshift + 8);
				rshift += 8;
				if (opcode & 0x100 && rshift >= 0) {
					sub += 1 << rshift;
					if (octx->uc_cfg & 1)
						sub--;
				}
				sub &= 0xfffffff;
				if (opcode & 0x800)
					ctx->va[i] = sub;
				sub = sext(sub, 27);
				if (hlshift >= 0)
					sres = sub >> hlshift;
				else
					sres = sub << -hlshift;
				if (!(opcode & 0x1000)) {
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
				sres >>= 8;
				d[i] = sres;
			}
			break;
		case 0x34:
		case 0x35:
			i = octx->c[opcode >> 3 & 3] >> 4;
			if (op == 0x34)
				s1 = octx->v[(src1 & 0x1c) | ((src1 + i) & 3)];
			else
				s1 = octx->v[src2];
			s2 = octx->v[(src1 & 0x1c) | ((src1 + i + 2) & 3)];
			s3 = octx->v[(src1 & 0x1c) | ((src1 + i + 3) & 3)];
			scond = octx->vc[opcode & 3] >> (opcode >> 2 & 1) * 16;
			d = ctx->v[dst];
			for (i = 0; i < 16; i++) {
				ss1 = s1[i];
				ss2 = s2[i];
				ss3 = s3[i];
				if (op == 0x35)
					ss1 = (int8_t)ss1;
				sub = 0;
				j = scond >> i & 1;
				if (op == 0x34) {
					sub += s2v->factor[j] * (ss2 - ss1);
					sub += s2v->factor[2 + j] * (ss3 - ss1);
				} else {
					sub += s2v->factor[j] * (ss2 - ss3);
					sub += s2v->factor[2 + j] * ss3;
				}
				shift = extrs(opcode, 5, 3);
				int rshift = -shift;
				rshift -= 1;
				int hlshift = rshift + 1;
				sub += ss1 << (hlshift + 8);
				if (opcode & 0x100 && rshift >= 0) {
					sub += 1 << rshift;
					if (octx->uc_cfg & 1)
						sub--;
				}
				sub &= 0xfffffff;
				ctx->va[i] = sub;
			}
			break;
		case 0x36:
		case 0x37:
			if (flag == 4) {
				i = cond >> 4 & 3;
				s1 = octx->v[(src1 & 0x1c) | ((src1 + i) & 3)];
				s2 = octx->v[(src1 & 0x1c) | ((src1 + i + 1) & 3)];
			} else {
				i = cond >> flag & 1;
				s1 = octx->v[src1 ^ i];
				s2 = octx->v[src1 ^ i];
			}
			scond = octx->vc[opcode & 3] >> (opcode >> 2 & 1) * 16;
			d = ctx->v[dst];
			for (i = 0; i < 16; i++) {
				ss1 = s1[i];
				ss2 = s2[i];
				sub = octx->va[i];
				j = scond >> i & 1;
				sub += s2v->factor[j] * (ss2 - ss1);
				sub += s2v->factor[2 + j] * (octx->vx[i] - ss1);
				shift = extrs(opcode, 11, 3);
				int rshift = -shift;
				if (op == 0x36)
					rshift -= 1;
				int hlshift = rshift + 1;
				rshift += 8;
				if (opcode & 0x200 && rshift >= 0) {
					sub += 1 << rshift;
					if (octx->uc_cfg & 1)
						sub--;
				}
				sub &= 0xfffffff;
				ctx->va[i] = sub;
				sub = sext(sub, 27);
				if (hlshift >= 0)
					sres = sub >> hlshift;
				else
					sres = sub << -hlshift;
				if (op == 0x36) {
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
				d[i] = sres >> 8;
			}
			break;
		case 0x2a:
		case 0x2b:
		case 0x2f:
		case 0x3a:
			s1 = octx->v[src1];
			d = ctx->v[dst];
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
			if (vci < 4)
				ctx->vc[vci] = cr;
			break;
		case 0x0f:
			s1 = octx->v[src1];
			s2 = octx->v[src1 | 1];
			s3 = octx->v[src2s];
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
			s1 = octx->v[src1];
			s2 = octx->v[src1 | 1];
			s3 = octx->v[src2];
			d = ctx->v[dst];
			shift = extrs(opcode, 5, 3);
			for (i = 0; i < 16; i++) {
				int32_t factor = s3[i] << (shift + 4);
				sres = s2[i] * (0x1000 - factor) + s1[i] * factor;
				if (opcode & 0x100) {
					sres += 0x7ff + !(octx->uc_cfg & 1);
				}
				if (sres > 0xfffff)
					sres = 0xfffff;
				if (sres < 0)
					sres = 0;
				sres >>= 12;
				d[i] = sres;
			}
			break;
		case 0x1b:
			s1 = octx->v[src1];
			s2 = octx->v[src2];
			s3 = octx->v[src3];
			d = ctx->v[dst];
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
			break;
		case 0x1f:
			s1 = octx->v[src1];
			s2 = octx->v[src2];
			s3 = octx->v[src3];
			d = ctx->v[dst];
			for (i = 0; i < 8; i++) {
				s16[i] = s2[2*i] | s2[2*i+1] << 8;
				s16[i+8] = s3[2*i] | s3[2*i+1] << 8;
			}
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
			if (vci < 4)
				ctx->vc[vci] = cr;
			break;
		case 0x24:
		case 0x25:
			s1 = octx->v[src1];
			s2 = octx->v[src2];
			s3 = octx->v[src3];
			d = ctx->v[dst];
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
			if (vci < 4)
				ctx->vc[vci] = cr;
			break;
		case 0x14:
			s1 = octx->v[src1];
			s2 = octx->v[src2];
			d = ctx->v[dst];
			for (i = 0; i < 16; i++) {
				d[i] = vp1_logop(s1[i], s2[i], opcode >> 3 & 0xf) & 0xff;
				if (!d[i])
					cr |= 1 << (16 + i);
			}
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
			s1 = octx->v[src1];
			s2 = octx->v[src2];
			d = ctx->v[dst];
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
					default:
						abort();
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
			if (vci < 4)
				ctx->vc[vci] = cr;
			break;
		case 0x2d:
			d = ctx->v[dst];
			for (i = 0; i < 16; i++) {
				d[i] = imm;
				if (!d[i])
					cr |= 1 << (16 + i);
				if (d[i] & 0x80)
					cr |= 1 << i;
			}
			if (vci < 4)
				ctx->vc[vci] = cr;
			break;
		case 0x3b:
			for (i = 0; i < 4; i++)
				for (j = 0; j < 4; j++)
					ctx->v[dst][i*4+j] = octx->vc[i] >> j * 8;
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

static void read_v(struct hwtest_ctx *ctx, int idx, uint8_t *v) {
	int i, j;
	for (i = 0; i < 4; i++) {
		uint32_t val = nva_rd32(ctx->cnum, 0xf000 + idx * 4 + i * 0x80);
		for (j = 0; j < 4; j++)
			v[i*4+j] = val >> 8 * j;
	}
}

static void write_v(struct hwtest_ctx *ctx, int idx, uint8_t *v) {
	int i, j;
	for (i = 0; i < 4; i++) {
		uint32_t val = 0;
		for (j = 0; j < 4; j++)
			val |= v[i*4+j] << 8 * j;
		nva_wr32(ctx->cnum, 0xf000 + idx * 4 + i * 0x80, val);
	}
}

/* Destroys $v and $va */
static void read_va(struct hwtest_ctx *ctx, uint32_t *va) {
	uint8_t a[16], b[16];
	int i;
	for (i = 0; i < 16; i++) {
		a[i] = 0;
	}
	write_v(ctx, 0, a);
	/* read bits 12-27 */
	nva_wr32(ctx->cnum, 0xf450, 0x82080088);
	nva_wr32(ctx->cnum, 0xf458, 1);
	nva_wr32(ctx->cnum, 0xf450, 0x82100098);
	nva_wr32(ctx->cnum, 0xf458, 1);
	read_v(ctx, 1, a);
	read_v(ctx, 2, b);
	for (i = 0; i < 16; i++) {
		va[i] = a[i] << 20 | b[i] << 12;
	}
	/* neutralize bits 20-27 */
	for (i = 0; i < 16; i++) {
		a[i] = -(va[i] >> 20) & 0xff;
		b[i] = 0x80;
	}
	write_v(ctx, 1, a);
	write_v(ctx, 2, b);
	for (i = 0; i < 32; i++) {
		nva_wr32(ctx->cnum, 0xf450, 0x83004408); /* $va += a * b, int */
		nva_wr32(ctx->cnum, 0xf458, 1);
	}
	/* and bits 16-19 */
	for (i = 0; i < 16; i++) {
		a[i] = (va[i] >> 16 & 0xf) << 4;
		b[i] = 0xf0;
	}
	write_v(ctx, 1, a);
	write_v(ctx, 2, b);
	nva_wr32(ctx->cnum, 0xf450, 0x8300440a); /* $va += a * b, int */
	nva_wr32(ctx->cnum, 0xf458, 1);
	/* now read low 16 bits */
	nva_wr32(ctx->cnum, 0xf450, 0x92080000);
	nva_wr32(ctx->cnum, 0xf458, 1);
	nva_wr32(ctx->cnum, 0xf450, 0x92100010);
	nva_wr32(ctx->cnum, 0xf458, 1);
	read_v(ctx, 1, a);
	read_v(ctx, 2, b);
	for (i = 0; i < 16; i++) {
		va[i] |= a[i] << 8 | b[i];
	}
}

/* Destroys $v */
static void write_va(struct hwtest_ctx *ctx, uint32_t *va) {
	uint8_t a[16], b[16];
	int i;
	/* bits 0-7 */
	for (i = 0; i < 16; i++) {
		a[i] = va[i] & 0xff;
		b[i] = 1;
	}
	write_v(ctx, 0, a);
	write_v(ctx, 1, b);
	nva_wr32(ctx->cnum, 0xf450, 0x80000200); /* $va = a * b, fract */
	nva_wr32(ctx->cnum, 0xf458, 1);
	/* bits 8-15 */
	for (i = 0; i < 16; i++) {
		a[i] = va[i] >> 8 & 0xff;
	}
	write_v(ctx, 0, a);
	nva_wr32(ctx->cnum, 0xf450, 0x83000208); /* $va += a * b, int */
	nva_wr32(ctx->cnum, 0xf458, 1);
	/* bits 16-19 */
	for (i = 0; i < 16; i++) {
		a[i] = (va[i] >> 16 & 0xf) << 4;
		b[i] = 0x10;
	}
	write_v(ctx, 0, a);
	write_v(ctx, 1, b);
	nva_wr32(ctx->cnum, 0xf450, 0x83000208); /* $va += a * b, int */
	nva_wr32(ctx->cnum, 0xf458, 1);
	/* bits 20-27 */
	for (i = 0; i < 16; i++) {
		a[i] = va[i] >> 20 & 0xff;
		b[i] = 0x80;
	}
	write_v(ctx, 0, a);
	write_v(ctx, 1, b);
	for (i = 0; i < 32; i++) {
		nva_wr32(ctx->cnum, 0xf450, 0x83000208); /* $va += a * b, int */
		nva_wr32(ctx->cnum, 0xf458, 1);
	}
}

/* Destroys $v */
static void read_vc(struct hwtest_ctx *ctx, uint32_t *vc) {
	nva_wr32(ctx->cnum, 0xf450, 0xbb000000);
	nva_wr32(ctx->cnum, 0xf458, 1);
	vc[0] = nva_rd32(ctx->cnum, 0xf000);
	vc[1] = nva_rd32(ctx->cnum, 0xf080);
	vc[2] = nva_rd32(ctx->cnum, 0xf100);
	vc[3] = nva_rd32(ctx->cnum, 0xf180);
}

/* Destroys $r, $a, $l */
static void write_c(struct hwtest_ctx *ctx, int idx, uint16_t val) {
	/* first, scalar flags */
	uint32_t r0, r1;
	r0 = 1;
	if (val & 1)
		r0 |= 1 << 31;
	if (val & 2)
		r0 = 0;
	if (val & 4)
		r0 |= 1 << 19;
	if (val & 0x10)
		r0 |= 1 << 20;
	if (val & 0x20)
		r0 |= 1 << 21;
	if (val & 0x80)
		r0 |= 1 << 18;
	r1 = 0;
	if (val & 8) {
		r0 -= 1 << 20;
		r1 = 1 << 20;
	}
	nva_wr32(ctx->cnum, 0xf780, r0);
	nva_wr32(ctx->cnum, 0xf784, r1);
	nva_wr32(ctx->cnum, 0xf44c, 0x4c0001c0 | idx | 1 << 9);
	nva_wr32(ctx->cnum, 0xf458, 1);
	/* second, address flags, full version */
	r0 = 1;
	if (val & 0x100)
		r0 = 1 << 31;
	if (val & 0x200)
		r0 = 0;
	nva_wr32(ctx->cnum, 0xf600, r0);
	nva_wr32(ctx->cnum, 0xf604, 0);
	nva_wr32(ctx->cnum, 0xf448, 0xcb0001c0 | idx | 1 << 9);
	nva_wr32(ctx->cnum, 0xf458, 1);
	/* third, address flags, half version */
	if (val & 0x400)
		r0 = 0;
	else
		r0 = 0x10000;
	nva_wr32(ctx->cnum, 0xf600, r0);
	nva_wr32(ctx->cnum, 0xf448, 0xca0001c0 | idx | 1 << 9);
	nva_wr32(ctx->cnum, 0xf458, 1);
	/* fourth, branch flags */
	if (val & 0x2000)
		nva_wr32(ctx->cnum, 0xf454, 0xf0000000 | idx << 19);
	else
		nva_wr32(ctx->cnum, 0xf454, 0xf0000001 | idx << 19);
	nva_wr32(ctx->cnum, 0xf458, 1);
}

/* Destroys $v */
static void write_vc(struct hwtest_ctx *ctx, int idx, uint32_t val) {
	int i;
	uint8_t a[16], b[16], c[16];
	for (i = 0; i < 16; i++) {
		if (val & 1 << i) {
			b[i] = 0x10;
			c[i] = 0xf0;
		} else {
			b[i] = 0xf0;
			c[i] = 0x10;
		}
		if (val & 1 << (16 + i))
			a[i] = 0;
		else
			a[i] = 1;
	}
	write_v(ctx, 0, a);
	write_v(ctx, 1, b);
	write_v(ctx, 2, c);
	nva_wr32(ctx->cnum, 0xf450, 0xa4000000 | idx | 1 << 9 | 2 << 4);
	nva_wr32(ctx->cnum, 0xf458, 1);
}

/* Destroys $v, $a */
static void write_vx(struct hwtest_ctx *ctx, uint8_t *val) {
	write_v(ctx, 0, val);
	nva_wr32(ctx->cnum, 0xf600, 0);
	nva_wr32(ctx->cnum, 0xf448, 0xc4000000);
	nva_wr32(ctx->cnum, 0xf458, 1);
	nva_wr32(ctx->cnum, 0xf600, 0);
	nva_wr32(ctx->cnum, 0xf448, 0xc80001c0);
	nva_wr32(ctx->cnum, 0xf458, 1);
}

/* Destroys $v, $va, $c */
static void read_vx(struct hwtest_ctx *ctx, uint8_t *val) {
	uint8_t zero[16] = { 0 };
	write_v(ctx, 0, zero);
	write_c(ctx, 0, 0x8000);
	nva_wr32(ctx->cnum, 0xf450, 0x80000000);
	nva_wr32(ctx->cnum, 0xf458, 1);
	nva_wr32(ctx->cnum, 0xf44c, 0x24020000);
	nva_wr32(ctx->cnum, 0xf450, 0xb6000880);
	nva_wr32(ctx->cnum, 0xf458, 1);
	read_v(ctx, 0, val);
}

/* Not all values are valid for $c registers. Clean a given value to a settable one. */
static uint16_t clean_c(uint16_t val, int chipset) {
	/* bits 11, 12, 14 cannot be set */
	val &= 0xa7ff;
	/* bit 15 is always set */
	val |= 0x8000;
	/* if bit 1 is set, bits 0, 2, 4-7 cannot be set */
	if (val & 2)
		val &= ~0xf5;
	/* if bit 9 is set, bit 8 cannot be set */
	if (val & 0x200)
		val &= ~0x100;
	/* bits 2 and 6 are tied */
	val &= ~0x40;
	if (val & 4)
		val |= 0x40;
	/* <NV50 don't have bits 6-7 */
	if (chipset != 0x50)
		val &= ~0xc0;
	return val;
}

static int test_isa_s(struct hwtest_ctx *ctx) {
	int i, j, k;
	nva_wr32(ctx->cnum, 0x200, 0xfffffffd);
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	for (i = 0; i < 1000000; i++) {
		struct vp1_s2v s2v = { 0 };
		struct vp1_ru ru = { -1, -1, -1, -1 };
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
			if (
				rfile == 8 ||
				rfile == 9 ||
				rfile == 0xa ||
				rfile == 0x16 ||
				rfile == 0x17 ||
				0)
				opcode_s = 0x4f000000;
		}
		if (
			op_a == 0x03 || /* [xdld] */
			op_a == 0x07 || /* [xdst] fuckup */
			op_a == 0x0e || /* [xdbar] fuckup */
			op_a == 0x0f || /* [xdwait] fuckup */
			op_a == 0x1b || /* fuckup */
			0)
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
			for (k = 0; k < 16; k++) {
				uint32_t val = jrand48(ctx->rand48);
				int which = jrand48(ctx->rand48) & 0x3;
				if (!which) {
					val &= ~0x7f;
				}
				octx.v[j][k] = val;
			}
		}
		for (j = 0; j < 4; j++) {
			uint32_t val = jrand48(ctx->rand48) & 0xffff;
			int which = jrand48(ctx->rand48) & 0x7;
			if (which < 2) {
				val &= ~(0x7f << which * 8);
			}
			octx.b[j] = val;
		}
		for (j = 0; j < 4; j++)
			octx.vc[j] = jrand48(ctx->rand48);
		for (j = 0; j < 16; j++)
			octx.va[j] = jrand48(ctx->rand48) & 0xfffffff;
		for (j = 0; j < 16; j++)
			octx.vx[j] = jrand48(ctx->rand48);
		for (j = 0; j < 4; j++)
			octx.c[j] = clean_c(jrand48(ctx->rand48), ctx->chipset);
		for (j = 0; j < 64; j++) {
			uint32_t val = jrand48(ctx->rand48);
			int which = jrand48(ctx->rand48) & 0xf;
			if (which < 4) {
				val &= ~(0x7f << which * 8);
			}
			octx.m[j] = val;
		}
		for (j = 0; j < 0x10; j++) {
			for (k = 0; k < 0x200; k++) {
				octx.ds[j][k] = jrand48(ctx->rand48);
			}
		}
		if (
			op_v == 0x04 ||
			op_v == 0x05 ||
			op_v == 0x15 ||
			op_v == 0x06 ||
			op_v == 0x16 ||
			op_v == 0x26 ||
			op_v == 0x07 ||
			op_v == 0x17 ||
			op_v == 0x27 ||
			op_v == 0x33 ||
			op_v == 0x34 ||
			op_v == 0x35 ||
			op_v == 0x36 ||
			op_v == 0x37 ||
			0) {
			opcode_s &= 0x00ffffff;
			switch (op_s & 7) {
				case 0:
					opcode_s |= 0x0f000000;
					break;
				case 1:
					opcode_s |= 0x24000000;
					break;
				case 2:
					opcode_s |= 0x45000000;
					break;
				case 3:
					opcode_s |= 0x04000000;
					break;
				case 4:
					opcode_s |= 0x05000000;
					break;
				default:
					opcode_s |= 0x05000000;
					break;
			}
			op_s = opcode_s >> 24;
		}
		if ((op_s == 0x04 || op_s == 0x05) && (op_a == 0x06 || op_a == 0x16 || op_a == 0x1e)) {
			opcode_a = 0xdf000000;
		}
		ectx = octx;
		write_vx(ctx, octx.vx);
		for (j = 0; j < 0x200; j++) {
			uint8_t row[16];
			for (k = 0; k < 16; k++)
				row[k] = octx.ds[k][j];
			write_v(ctx, 0, row);
			nva_wr32(ctx->cnum, 0xf600, j << 4);
			nva_wr32(ctx->cnum, 0xf448, 0xd7000001);
			nva_wr32(ctx->cnum, 0xf458, 1);
		}
		for (j = 0; j < 4; j++) {
			write_c(ctx, j, octx.c[j]);
			write_vc(ctx, j, octx.vc[j]);
		}
		write_va(ctx, octx.va);
		for (j = 0; j < 16; j++) {
			nva_wr32(ctx->cnum, 0xf780, octx.x[j]);
			nva_wr32(ctx->cnum, 0xf44c, 0x6a0000c7 | j << 19);
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
			write_v(ctx, j, octx.v[j]);
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
		if ((opcode_s >> 24 & 0x7f) == 0x6b) {
			int rfile = opcode_s >> 3 & 0x1f;
			int reg = opcode_s >> 14 & 0x1f;
			if (rfile < 4) {
				ru.stored_v = reg;
			}
		}
		simulate_op_a(&octx, &ectx, opcode_a, &ru);
		simulate_op_s(&octx, &ectx, opcode_s, &s2v, &ru, ctx->chipset, op_b);
		simulate_op_v(&octx, &ectx, opcode_v, &s2v);
		simulate_op_b(&octx, &ectx, opcode_b);
		/* XXX wait? */
		nctx.uc_cfg = nva_rd32(ctx->cnum, 0xf540);
		for (j = 0; j < 32; j++)
			nctx.a[j] = nva_rd32(ctx->cnum, 0xf600 + j * 4);
		for (j = 0; j < 31; j++)
			nctx.r[j] = nva_rd32(ctx->cnum, 0xf780 + j * 4);
		for (j = 0; j < 32; j++)
			read_v(ctx, j, nctx.v[j]);
		for (j = 0; j < 4; j++)
			nctx.c[j] = nva_rd32(ctx->cnum, 0xf680 + j * 4);
		for (j = 0; j < 4; j++)
			nctx.b[j] = nva_rd32(ctx->cnum, 0xf580 + j * 4);
		for (j = 0; j < 64; j++)
			nctx.m[j] = nva_rd32(ctx->cnum, 0xfa00 + j * 4);
		for (j = 0; j < 16; j++) {
			nva_wr32(ctx->cnum, 0xf44c, 0x6b0000c0 | j << 14);
			nva_wr32(ctx->cnum, 0xf458, 1);
			nctx.x[j] = nva_rd32(ctx->cnum, 0xf780);
		}
		read_vc(ctx, nctx.vc);
		read_va(ctx, nctx.va);
		uint8_t zero[16] = { 0 };
		write_v(ctx, 1, zero);
		for (j = 0; j < 0x200; j++) {
			uint8_t row[16];
			nva_wr32(ctx->cnum, 0xf600, j << 4);
			nva_wr32(ctx->cnum, 0xf448, 0xd7000200);
			nva_wr32(ctx->cnum, 0xf458, 1);
			read_v(ctx, 0, row);
			for (k = 0; k < 16; k++)
				nctx.ds[k][j] = row[k];
		}
		read_vx(ctx, nctx.vx);
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
			for (j = 0; j < 16; j++) {
				IPRINT("VX[%d] ", vx[j]);
			}
			for (j = 0; j < 32; j++) {
				IPRINT("A[0x%02x]   ", a[j]);
			}
			for (j = 0; j < 31; j++) {
				IPRINT("R[0x%02x]   ", r[j]);
			}
			for (j = 0; j < 32; j++) {
				for (k = 0; k < 16; k++) {
					IIPRINT("V[0x%02x][%x]   ", v[j][k]);
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
			for (k = 0; k < 0x200; k++) {
				for (j = 0; j < 16; j++) {
					IIPRINT("DS[%x][0x%03x]   ", ds[j][k]);
				}
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
