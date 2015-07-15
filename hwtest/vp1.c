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
#include "nvhw.h"
#include "util.h"
#include <stdio.h>
#include <stdbool.h>
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

struct vp1_bs {
	int chipset;
	/* decode - read ports */
	int d_a_asrc1;
	int d_a_asrc2;
	int d_a_vsrc;
	int d_a_rsrc;
	int d_a_csrc;
	int d_a_slct;
	int d_x_rsrc;
	int d_x_csrc;
	int d_x_xsrc;
	int d_x_msrc;
	int d_x_vsrc;
	int d_x_asrc;
	int d_x_lsrc;
	int d_s_rsrc1;
	int d_s_rsrc2;
	int d_s_csrc;
	int d_s_slct;
	int d_v_vsrc1;
	int d_v_vsrc2;
	int d_v_vsrc3;
	int d_v_csrc;
	int d_v_slct;
	int d_v_vcsrc;
	int d_v_vcpart;
	int d_b_lsrc;
	/* decode - write port config */
	/* $c and $vc */
	int d_a_cdst_s;
	int d_a_cdst_l;
	int d_s_cdst;
	int d_b_cdst;
	int d_v_cdst;
	/* $a */
	int d_a_adst;
	int d_s_adst;
	/* $r */
	int d_a_rdst;
	int d_s_rdst;
	int d_s_xrdst;
	/* $v */
	int d_a_vdst;
	int d_s_vdst;
	int d_s_vslot;
	int d_v_vdst;
	bool d_a_vxdst;
	bool d_v_vadst;
	/* $l */
	int d_s_ldst;
	int d_b_ldst;
	/* others */
	int d_s_mdst;
	int d_s_xdst;
	/* decode - immediates */
	uint16_t d_a_imm;
	uint8_t d_a_logop;
	uint32_t d_s_imm;
	int16_t d_s_fimm[2];
	uint8_t d_s_logop;
	uint8_t d_v_imm;
	uint8_t d_v_logop;
	uint16_t d_b_imm;
	/* decode - s2v */
	bool d_s2v_valid;
	int d_s2v_vcsrc;
	int d_s2v_vcpart;
	int d_s2v_vcmode;
	/* decode - address source routing */
	enum {
		A_ROUTE_NOP,
		A_ROUTE_S2S,
		A_ROUTE_QUAD,
	} d_a_route;
	/* decode - scalar source routing */
	enum {
		S_ROUTE_NOP,
		S_ROUTE_S2S,
		S_ROUTE_VECMAD,
	} d_s_route;
	/* decode - vector source routing */
	enum {
		V_ROUTE_NOP,	/* SRC1, SRC2, SRC3 */
		V_ROUTE_S1D,	/* SRC1, SRC2, SRC1 | 1 */
		V_ROUTE_S2SS1D,	/* SRC1, mangled SRC2, SRC1 | 1 */
		V_ROUTE_Q230,	/* SRC1Q[2], SRC1Q[3], SRC1Q[0] */
		V_ROUTE_Q23S2,	/* SRC1Q[2], SRC2, SRC1Q[3] */
		V_ROUTE_Q10X,	/* SRC1Q[1], VX, SRC1Q[0] */
	} d_v_route;
	/* decode - address pipeline config */
	enum {
		EA_NOP,
		EA_HORIZ,
		EA_VERT,
		EA_RAW_LOAD,
		EA_RAW_STORE,
	} d_a_eamode;
	bool d_a_eaimm;
	bool d_a_useimm;
	enum {
		A_MODE_NOP,
		A_MODE_SETHI,
		A_MODE_SETLO,
		A_MODE_ADD,
		A_MODE_AADD,
		A_MODE_LOGOP,
	} d_a_mode;
	enum {
		A_SMODE_NONE,
		A_SMODE_SCALAR,
		A_SMODE_VECTOR,
	} d_a_smode;
	/* decode - scalar pipeline config */
	bool d_s_useimm;
	bool d_s_signd;
	bool d_s_sign1;
	bool d_s_sign2;
	bool d_s_sign3;
	bool d_s_clip;
	bool d_s_rnd;
	enum {
		S_MODE_NOP,
		S_MODE_R2X,
		S_MODE_X2R,
		S_MODE_LDIMM,
		S_MODE_SETHI,
		S_MODE_MUL,
		S_MODE_MIN,
		S_MODE_MAX,
		S_MODE_ABS,
		S_MODE_NEG,
		S_MODE_ADD,
		S_MODE_SUB,
		S_MODE_SHR,
		S_MODE_LOGOP,
		S_MODE_BYTE,
		S_MODE_BMIN,
		S_MODE_BMAX,
		S_MODE_BABS,
		S_MODE_BNEG,
		S_MODE_BADD,
		S_MODE_BSUB,
		S_MODE_BSHR,
		S_MODE_BMUL,
		S_MODE_VECMS,
		S_MODE_VEC,
		S_MODE_BVEC,
		S_MODE_BVECMAD,
		S_MODE_BVECMADSEL,
	} d_s_mode;
	enum {
		S_CMODE_ZERO,
		S_CMODE_PART,
		S_CMODE_FULL_ZERO,
		S_CMODE_FULL,
	} d_s_cmode;
	/* decode - vector pipeline config */
	bool d_v_sign1;
	bool d_v_sign2;
	bool d_v_sign3;
	bool d_v_signd;
	bool d_v_useimm;
	bool d_v_hilo;
	bool d_v_rnd;
	bool d_v_fractint;
	bool d_v_lrp2x;
	bool d_v_mask;
	bool d_v_use_s2v_vc;
	int d_v_shift;
	enum {
		V_MAD_A_ZERO,
		V_MAD_A_VA,
		V_MAD_A_US2,
		V_MAD_A_S2,
		V_MAD_A_S3X,
	} d_v_mad_a;
	enum {
		V_MAD_B_S1,
		V_MAD_B_S1MS3,
	} d_v_mad_b;
	enum {
		V_MAD_D_S3,
		V_MAD_D_S2MS3,
	} d_v_mad_d;
	bool d_v_mad_s2v;
	enum {
		V_MODE_NOP,
		V_MODE_MIN,
		V_MODE_MAX,
		V_MODE_ABS,
		V_MODE_NEG,
		V_MODE_ADD,
		V_MODE_SUB,
		V_MODE_SHR,
		V_MODE_LOGOP,
		V_MODE_LDIMM,
		V_MODE_LDVC,
		V_MODE_CLIP,
		V_MODE_MINABS,
		V_MODE_ADD9,
		V_MODE_SWZ,
		V_MODE_MAD,
	} d_v_mode;
	enum {
		V_FMODE_RAW,
		V_FMODE_RAW_ZERO,
		V_FMODE_RAW_SIGN,
		V_FMODE_CLIP,
		V_FMODE_RAW_SPEC,
		V_FMODE_CMPAD,
	} d_v_fmode;
	/* decode - branch pipeline config */
	enum {
		B_LMODE_NOP,
		B_LMODE_MOV,
		B_LMODE_LOOP,
	} d_b_lmode;
	/* preread - inputs */
	uint16_t p_a_cin;
	uint16_t p_s_cin;
	int32_t p_v_vain[16];
	uint32_t p_v_vcin[4];
	uint16_t p_v_cin;
	bool p_down;
	/* adjusted read ports */
	int a_a_asrc2;
	int a_s_rsrc2;
	int a_s_rsrc3;
	int a_v_vsrc1;
	int a_v_vsrc2;
	int a_v_vsrc3;
	int a_a_vdst;
	/* read - inputs */
	uint32_t r_a_ain1;
	uint32_t r_a_ain2;
	uint8_t r_a_vin[16];
	uint32_t r_s_rin1;
	uint32_t r_s_rin2;
	uint32_t r_s_rin3;
	uint32_t r_x_rin;
	uint32_t r_x_xin;
	uint16_t r_b_lin;
	uint8_t r_v_vin1[16];
	uint8_t r_v_vin2[16];
	uint8_t r_v_vin3[16];
	/* execute - s2v */
	int16_t e_s2v_factor[4];
	uint16_t e_s2v_mask[2];
	/* execute - memory addressing */
	uint16_t e_mem_cell[16];
	uint8_t e_mem_bank;
	bool e_mem_wide;
	uint8_t e_mem_wmask;
	/* execute - results */
	/* $c and $vc */
	uint8_t e_a_cres_s;
	uint8_t e_a_cres_l;
	uint8_t e_s_cres;
	uint32_t e_v_cres;
	uint8_t e_b_cres;
	/* address unit */
	uint8_t e_a_vres[16];
	int e_a_rslot;
	uint32_t e_a_ares;
	/* scalar unit */
	uint32_t e_s_res;
	/* vector unit */
	uint8_t e_v_vres[16];
	int32_t e_v_vares[16];
	/* branch unit */
	uint16_t e_b_lres;
};

static uint32_t b2w(const uint8_t bytes[4]) {
	uint32_t res = 0;
	int i;
	for (i = 0; i < 4; i++)
		res |= bytes[i] << i * 8;
	return res;
}

static void w2b(uint8_t bytes[4], uint32_t word) {
	int i;
	for (i = 0; i < 4; i++)
		bytes[i] = word >> i * 8;
}

static uint32_t read_r(struct vp1_ctx *ctx, int idx) {
	if (idx < 31)
		return ctx->r[idx];
	return 0;
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

static int vp1_mangle_reg(int reg, int cond, int flag, int param) {
	if (flag == 4) {
		return (reg & 0x1c) | ((reg + (cond >> flag) + param) & 3);
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

static uint8_t mem_bank(uint16_t addr, int stride) {
	int bank;
	switch (stride) {
		case 0:
			bank = (addr >> 5) & 7;
			break;
		case 1:
			bank = addr >> 5;
			break;
		case 2:
			bank = addr >> 6;
			break;
		case 3:
			bank = addr >> 7;
			break;
		default:
			abort();
	}
	return (bank + addr) & 0xf;
}

static void decode_op_a(struct vp1_bs *bs, uint32_t opcode) {
	uint32_t op = opcode >> 24 & 0x1f;
	uint32_t src1 = opcode >> 14 & 0x1f;
	uint32_t src2 = opcode >> 9 & 0x1f;
	uint32_t dst = opcode >> 19 & 0x1f;
	uint32_t imm = extrs(opcode, 3, 11);
	int subop = op & 3;
	bs->d_a_csrc = opcode >> 3 & 3;
	bs->d_a_slct = opcode >> 5 & 0xf;
	bs->d_a_cdst_l = -1;
	bs->d_a_cdst_s = -1;
	bs->d_a_vdst = -1;
	bs->d_a_rdst = -1;
	bs->d_a_adst = -1;
	bs->d_a_vxdst = false;
	bs->d_a_eamode = EA_NOP;
	bs->d_a_mode = A_MODE_NOP;
	bs->d_a_smode = A_SMODE_NONE;
	bs->d_a_eaimm = false;
	bs->d_a_useimm = false;
	bs->d_a_vsrc = -1;
	bs->d_a_rsrc = -1;
	bs->d_a_asrc2 = src2;
	bs->d_a_route = A_ROUTE_S2S;
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
			bs->d_a_cdst_s = opcode & 7;
			if (subop == 1)
				bs->d_a_eamode = EA_VERT;
			else
				bs->d_a_eamode = EA_HORIZ;
			bs->d_a_mode = A_MODE_AADD;
			if (op & 8) {
				bs->d_a_imm = imm & 0x7ff;
				bs->d_a_eaimm = true;
				bs->d_a_useimm = true;
			} else {
				bs->d_a_imm = imm;
				bs->d_a_useimm = !!(op & 0x10);
				bs->d_a_adst = src1;
			}
			if (subop == 0 || subop == 1) {
				bs->d_a_vdst = dst;
			} else {
				bs->d_a_rdst = dst;
			}
			bs->d_a_asrc1 = src1;
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
			bs->d_a_cdst_s = opcode & 7;
			if (subop == 1)
				bs->d_a_eamode = EA_VERT;
			else
				bs->d_a_eamode = EA_HORIZ;
			bs->d_a_mode = A_MODE_AADD;
			if (op & 8) {
				bs->d_a_imm = imm & 0x7ff;
				bs->d_a_eaimm = true;
				bs->d_a_useimm = true;
			} else {
				bs->d_a_imm = imm;
				bs->d_a_useimm = !!(op & 0x10);
				bs->d_a_adst = dst;
			}
			bs->d_a_asrc1 = dst;
			if (subop == 0 || subop == 1) {
				bs->d_a_smode = A_SMODE_VECTOR;
				bs->d_a_vsrc = src1;
			} else if (subop == 2) {
				bs->d_a_smode = A_SMODE_SCALAR;
				bs->d_a_rsrc = src1;
			}
			break;
		case 0x08:
		case 0x09:
			bs->d_a_cdst_s = opcode & 7;
			if (subop == 1)
				bs->d_a_eamode = EA_VERT;
			else
				bs->d_a_eamode = EA_HORIZ;
			bs->d_a_vxdst = true;
			bs->d_a_adst = src1;
			bs->d_a_mode = A_MODE_AADD;
			bs->d_a_asrc1 = src1;
			bs->d_a_route = A_ROUTE_QUAD;
			bs->d_a_vdst = dst;
			break;
		case 0x0a:
			bs->d_a_adst = dst;
			bs->d_a_cdst_s = opcode & 7;
			bs->d_a_mode = A_MODE_AADD;
			bs->d_a_asrc1 = dst;
			break;
		case 0x0b:
			bs->d_a_adst = dst;
			bs->d_a_cdst_l = opcode & 7;
			bs->d_a_mode = A_MODE_ADD;
			bs->d_a_asrc1 = src1;
			break;
		case 0x0c:
			bs->d_a_adst = dst;
			bs->d_a_imm = opcode & 0xffff;
			bs->d_a_mode = A_MODE_SETLO;
			bs->d_a_asrc1 = dst;
			break;
		case 0x0d:
			bs->d_a_adst = dst;
			bs->d_a_imm = opcode & 0xffff;
			bs->d_a_mode = A_MODE_SETHI;
			bs->d_a_asrc1 = dst;
			break;
		case 0x13:
			bs->d_a_adst = dst;
			bs->d_a_cdst_l = opcode & 7;
			bs->d_a_mode = A_MODE_LOGOP;
			bs->d_a_logop = opcode >> 3 & 0xf;
			bs->d_a_asrc1 = src1;
			bs->d_a_route = A_ROUTE_NOP;
			break;
		case 0x17:
			if (opcode & 1) {
				bs->d_a_adst = dst;
				bs->d_a_eamode = EA_RAW_STORE;
				bs->d_a_smode = A_SMODE_VECTOR;
				bs->d_a_mode = A_MODE_AADD;
				bs->d_a_asrc1 = dst;
				bs->d_a_vsrc = src1;
			} else {
				bs->d_a_vdst = dst;
				bs->d_a_eamode = EA_RAW_LOAD;
				bs->d_a_asrc1 = src1;
				bs->d_a_vsrc = src2;
			}
			break;
	}
}

static void preread_op_a(struct vp1_bs *bs, struct vp1_ctx *octx) {
	bs->p_a_cin = octx->c[bs->d_a_csrc];
	/* adjust phase */
	switch (bs->d_a_route) {
		case A_ROUTE_NOP:
			bs->a_a_asrc2 = bs->d_a_asrc2;
			bs->a_a_vdst = bs->d_a_vdst;
			break;
		case A_ROUTE_S2S:
			bs->a_a_asrc2 = vp1_mangle_reg(bs->d_a_asrc2, bs->p_a_cin, bs->d_a_slct, 0);
			bs->a_a_vdst = bs->d_a_vdst;
			break;
		case A_ROUTE_QUAD:
			bs->a_a_asrc2 = vp1_mangle_reg(bs->d_a_asrc2, bs->p_a_cin, bs->d_a_slct, 0);
			if (bs->p_a_cin & 1 << bs->d_a_slct) {
				bs->a_a_vdst = vp1_mangle_reg(bs->d_a_vdst, bs->p_a_cin, 4, 0);
			} else {
				bs->a_a_vdst = -1;
			}
			break;
	}
}

static void read_op_a(struct vp1_bs *bs, struct vp1_ctx *octx) {
	bs->r_a_ain1 = octx->a[bs->d_a_asrc1];
	bs->r_a_ain2 = octx->a[bs->a_a_asrc2];
	if (bs->d_a_rsrc != -1) {
		/* XXX */
		int xsrc1 = bs->d_a_rsrc;
		if (bs->a_s_rsrc3 != -1)
			xsrc1 = bs->a_s_rsrc3;
		int i;
		uint32_t val = read_r(octx, xsrc1);
		for (i = 0; i < 16; i++) {
			bs->r_a_vin[i] = val >> 8 * (i&3);
		}
	}
	if (bs->d_x_asrc != -1) {
		bs->r_x_xin = octx->a[bs->d_x_asrc];
	}
}

static void execute_op_a(struct vp1_bs *bs) {
	uint32_t s1 = bs->r_a_ain1;
	uint32_t s2 = bs->d_a_useimm ? bs->d_a_imm : bs->r_a_ain2;
	switch (bs->d_a_mode) {
		case A_MODE_NOP:
			break;
		case A_MODE_SETHI:
			bs->e_a_ares = bs->d_a_imm << 16 | (s1 & 0xffff);
			break;
		case A_MODE_SETLO:
			bs->e_a_ares = bs->d_a_imm | (s1 & ~0xffff);
			break;
		case A_MODE_ADD:
			bs->e_a_ares = s1 + s2;
			break;
		case A_MODE_LOGOP:
			bs->e_a_ares = vp1_logop(s1, s2, bs->d_a_logop);
			break;
		case A_MODE_AADD:
			bs->e_a_ares = vp1_hadd(s1, s2);
			break;
		default:
			abort();
	}
	uint32_t cr = 0;
	if (bs->e_a_ares & 0x80000000)
		cr |= 0x1;
	if (!bs->e_a_ares)
		cr |= 0x2;
	bs->e_a_cres_l = cr;
	uint32_t lo = bs->e_a_ares & 0xffff;
	uint32_t hi = bs->e_a_ares >> 16 & 0x3fff;
	bs->e_a_cres_s = lo >= hi;
}

static void ea_op_a(struct vp1_bs *bs) {
	int i;
	uint16_t addr = bs->r_a_ain1 & 0x1fff;
	uint8_t stride = bs->r_a_ain1 >> 30;
	if (bs->d_a_eaimm)
		addr |= bs->d_a_imm;
	switch (bs->d_a_smode) {
		case A_SMODE_NONE:
			bs->e_mem_wmask = 0;
			break;
		case A_SMODE_SCALAR:
			bs->e_mem_wmask = 1 << (addr >> 2 & 3);
			break;
		case A_SMODE_VECTOR:
			bs->e_mem_wmask = 0xf;
			break;
	}
	bs->e_a_rslot = addr >> 2 & 3;
	switch (bs->d_a_eamode) {
		case EA_NOP:
			break;
		case EA_RAW_LOAD:
			bs->e_mem_wide = false;
			bs->e_mem_bank = 0;
			for (i = 0; i < 16; i++) {
				bs->e_mem_cell[i] = addr >> 4 | bs->r_a_vin[i];
			}
			break;
		case EA_RAW_STORE:
			bs->e_mem_wide = false;
			bs->e_mem_bank = 0;
			for (i = 0; i < 16; i++) {
				bs->e_mem_cell[i] = addr >> 4;
			}
			break;
		case EA_HORIZ:
			addr &= ~0xf;
			bs->e_mem_bank = mem_bank(addr, stride);
			bs->e_mem_wide = false;
			for (i = 0; i < 16; i++) {
				bs->e_mem_cell[i] = addr >> 4;
			}
			break;
		case EA_VERT:
			addr &= ~(0xf << (4 + stride));
			bs->e_mem_bank = mem_bank(addr, stride);
			if (stride == 0) {
				bs->e_mem_wide = true;
				for (i = 0; i < 8; i++) {
					bs->e_mem_cell[i] = addr >> 4 | i << 1;
				}
			} else {
				bs->e_mem_wide = false;
				for (i = 0; i < 16; i++) {
					bs->e_mem_cell[i] = addr >> 4 | i << stride;
				}
			}
			break;
		default:
			abort();
	}
}

static void memory_op_a(struct vp1_bs *bs, struct vp1_ctx *ectx) {
	int i;
	if (bs->e_mem_wide) {
		for (i = 0; i < 8; i++) {
			int bank = (bs->e_mem_bank + i)&0xf;
			int cell = bs->e_mem_cell[i];
			bs->e_a_vres[i*2] = ectx->ds[bank][cell];
			bs->e_a_vres[i*2+1] = ectx->ds[bank][cell+1];
			if (bs->e_mem_wmask & 1 << (i >> 1)) {
				ectx->ds[bank][cell] = bs->r_a_vin[i*2];
				ectx->ds[bank][cell+1] = bs->r_a_vin[i*2+1];
			}
		}
	} else {
		for (i = 0; i < 16; i++) {
			int bank = (bs->e_mem_bank + i)&0xf;
			int cell = bs->e_mem_cell[i];
			bs->e_a_vres[i] = ectx->ds[bank][cell];
			if (bs->e_mem_wmask & 1 << (i >> 2)) {
				ectx->ds[bank][cell] = bs->r_a_vin[i];
			}
		}
	}	
}

static void write_op_a(struct vp1_bs *bs, struct vp1_ctx *ectx) {
	if (bs->d_a_cdst_s != -1 && bs->d_a_cdst_s < 4) {
		ectx->c[bs->d_a_cdst_s] &= ~0x400;
		ectx->c[bs->d_a_cdst_s] |= bs->e_a_cres_s << 10;
	}
	if (bs->d_a_cdst_l != -1 && bs->d_a_cdst_l < 4) {
		ectx->c[bs->d_a_cdst_l] &= ~0x300;
		ectx->c[bs->d_a_cdst_l] |= bs->e_a_cres_l << 8;
	}
	if (bs->d_a_adst != -1) {
		ectx->a[bs->d_a_adst] = bs->e_a_ares;
	}
	if (bs->d_s_adst != -1) {
		ectx->a[bs->d_s_adst] = bs->e_s_res;
	}
}

static void decode_op_s(struct vp1_bs *bs, uint32_t opcode, int op_b) {
	uint32_t op = opcode >> 24 & 0x7f;
	uint32_t src1 = opcode >> 14 & 0x1f;
	uint32_t src2 = opcode >> 9 & 0x1f;
	uint32_t dst = opcode >> 19 & 0x1f;
	uint32_t imm = extrs(opcode, 3, 11);
	uint32_t cdst = opcode & 7;
	int rfile = opcode >> 3 & 0x1f;
	bs->d_s_vdst = -1;
	bs->d_s_rdst = -1;
	bs->d_s_xrdst = -1;
	bs->d_s_cdst = cdst;
	bs->d_s_ldst = -1;
	bs->d_s_adst = -1;
	bs->d_s_xdst = -1;
	bs->d_s_mdst = -1;
	bs->d_s2v_valid = false;
	bs->d_s_cmode = S_CMODE_ZERO;
	bs->d_s_mode = S_MODE_NOP;
	bs->d_s_useimm = false;
	bs->d_s_signd = false;
	bs->d_s_sign1 = false;
	bs->d_s_sign2 = false;
	bs->d_s_rsrc1 = src1;
	bs->d_s_rsrc2 = src2;
	bs->d_s_csrc = opcode >> 3 & 3;
	bs->d_s_slct = opcode >> 5 & 0xf;
	bs->d_s_route = S_ROUTE_NOP;
	bs->a_s_rsrc3 = -1;
	bs->d_x_rsrc = -1;
	bs->d_x_xsrc = -1;
	bs->d_x_msrc = -1;
	bs->d_x_asrc = -1;
	bs->d_x_csrc = -1;
	bs->d_x_lsrc = -1;
	bs->d_x_vsrc = -1;
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
			bs->d_s_sign1 = !(op & 0x10);
			bs->d_s_sign2 = !(op & 0x10);
			bs->d_s_signd = !(op & 0x10);
			bs->d_s_imm = (opcode >> 3 & 0xff) * 0x01010101;
			bs->d_s_useimm = !!(op & 0x20);
			bs->d_s_clip = true;
			switch (op & 0xf) {
				case 0x8:
					bs->d_s_mode = S_MODE_BMIN;
					break;
				case 0x9:
					bs->d_s_mode = S_MODE_BMAX;
					break;
				case 0xa:
					bs->d_s_mode = S_MODE_BABS;
					break;
				case 0xb:
					bs->d_s_mode = S_MODE_BNEG;
					break;
				case 0xc:
					bs->d_s_mode = S_MODE_BADD;
					break;
				case 0xd:
					bs->d_s_mode = S_MODE_BSUB;
					break;
				case 0xe:
					bs->d_s_mode = S_MODE_BSHR;
					bs->d_s_clip = false;
					break;
				default:
					abort();
			}
			bs->d_s_rdst = dst;
			bs->d_s_route = S_ROUTE_S2S;
			break;
		case 0x01:
		case 0x02:
		case 0x11:
		case 0x12:
		case 0x21:
		case 0x22:
		case 0x31:
		case 0x32:
			bs->d_s_rdst = dst;
			bs->d_s_cdst = -1;
			bs->d_s_sign1 = !!(opcode & 4);
			bs->d_s_sign2 = !!(opcode & 2);
			bs->d_s_signd = !(op & 0x10);
			bs->d_s_useimm = !!(op & 0x20);
			bs->d_s_mode = S_MODE_BMUL;
			bs->d_s_rnd = !!(opcode & 0x100);
			bs->d_s_clip = true;
			if (op & 2) {
				bs->d_s_imm = (opcode & 0xff) * 0x01010101;
			} else {
				bs->d_s_imm = ((opcode & 1) << 5 | src2) * 0x04040404;
			}
			bs->d_s_route = S_ROUTE_NOP;
			break;
		case 0x25:
		case 0x26:
		case 0x27:
			bs->d_s_rdst = dst;
			bs->d_s_imm = (opcode >> 3 & 0xff) * 0x01010101;
			bs->d_s_useimm = true;
			bs->d_s_mode = S_MODE_LOGOP;
			if (op == 0x25) {
				bs->d_s_logop = 0x8;
			} else if (op == 0x26) {
				bs->d_s_logop = 0xe;
			} else if (op == 0x27) {
				bs->d_s_logop = 0x6;
			} else {
				abort();
			}
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
			bs->d_s_rdst = dst;
			bs->d_s_cmode = S_CMODE_FULL;
			bs->d_s_route = S_ROUTE_S2S;
			if (op == 0x41 || op == 0x51) {
				bs->d_s_mode = S_MODE_MUL;
			} else if (op == 0x42) {
				bs->d_s_logop = opcode >> 3 & 0xf;
				bs->d_s_cmode = S_CMODE_PART;
				bs->d_s_mode = S_MODE_LOGOP;
				bs->d_s_route = S_ROUTE_NOP;
			} else if (op == 0x48 || op == 0x58) {
				bs->d_s_mode = S_MODE_MIN;
			} else if (op == 0x49 || op == 0x59) {
				bs->d_s_mode = S_MODE_MAX;
			} else if (op == 0x4a || op == 0x5a) {
				bs->d_s_mode = S_MODE_ABS;
			} else if (op == 0x4b || op == 0x5b) {
				bs->d_s_mode = S_MODE_NEG;
				bs->d_s_cmode = S_CMODE_FULL_ZERO;
			} else if (op == 0x4c || op == 0x5c) {
				bs->d_s_mode = S_MODE_ADD;
			} else if (op == 0x4d || op == 0x5d) {
				bs->d_s_mode = S_MODE_SUB;
			} else if (op == 0x4e) {
				bs->d_s_mode = S_MODE_SHR;
				bs->d_s_signd = false;
				bs->d_s_sign1 = false;
				bs->d_s_sign2 = false;
			} else if (op == 0x5e) {
				bs->d_s_mode = S_MODE_SHR;
				bs->d_s_signd = true;
				bs->d_s_sign1 = true;
				bs->d_s_sign2 = true;
			} else {
				abort();
			}
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
			bs->d_s_rdst = dst;
			bs->d_s_cmode = S_CMODE_FULL;
			bs->d_s_useimm = true;
			bs->d_s_imm = imm;
			if (op == 0x61 || op == 0x71) {
				bs->d_s_mode = S_MODE_MUL;
			} else if (op == 0x62) {
				bs->d_s_logop = 0x8;
				bs->d_s_mode = S_MODE_LOGOP;
				bs->d_s_cmode = S_CMODE_PART;
			} else if (op == 0x63) {
				bs->d_s_logop = 0x6;
				bs->d_s_mode = S_MODE_LOGOP;
				bs->d_s_cmode = S_CMODE_PART;
			} else if (op == 0x64) {
				bs->d_s_logop = 0xe;
				bs->d_s_mode = S_MODE_LOGOP;
				bs->d_s_cmode = S_CMODE_PART;
			} else if (op == 0x68 || op == 0x78) {
				bs->d_s_mode = S_MODE_MIN;
			} else if (op == 0x69 || op == 0x79) {
				bs->d_s_mode = S_MODE_MAX;
			} else if (op == 0x6c || op == 0x7c) {
				bs->d_s_mode = S_MODE_ADD;
			} else if (op == 0x6d || op == 0x7d) {
				bs->d_s_mode = S_MODE_SUB;
			} else if (op == 0x6e) {
				bs->d_s_mode = S_MODE_SHR;
				bs->d_s_sign1 = false;
			} else if (op == 0x7e) {
				bs->d_s_mode = S_MODE_SHR;
				bs->d_s_sign1 = true;
			} else if (op == 0x7a) {
				bs->d_s_mode = S_MODE_ABS;
			} else if (op == 0x7b) {
				bs->d_s_mode = S_MODE_NEG;
				bs->d_s_cmode = S_CMODE_FULL_ZERO;
			} else {
				abort();
			}
			break;
		case 0x65:
			bs->d_s_rdst = dst;
			bs->d_s_cdst = -1;
			bs->d_s_imm = extrs(opcode, 0, 19);
			bs->d_s_mode = S_MODE_LDIMM;
			break;
		case 0x75:
			bs->d_s_rdst = dst;
			bs->d_s_cdst = -1;
			bs->d_s_imm = opcode & 0xffff;
			bs->d_s_mode = S_MODE_SETHI;
			bs->d_s_rsrc1 = dst;
			break;
		case 0x6a:
			/* decode */
			switch (rfile) {
				case 0x00:
				case 0x01:
				case 0x02:
				case 0x03:
				case 0x12:
					bs->d_s_vdst = dst;
					bs->d_s_vslot = rfile & 3;
					break;
				case 0x0b:
					bs->d_s_ldst = dst;
					break;
				case 0x0c:
					bs->d_s_adst = dst;
					break;
				case 0x14:
					bs->d_s_mdst = dst;
					break;
				case 0x15:
					bs->d_s_mdst = dst + 32;
					break;
				case 0x18:
					bs->d_s_xdst = dst & 0xf;
					break;
			}
			bs->d_x_rsrc = src1;
			bs->d_s_mode = S_MODE_R2X;
			break;
		case 0x6b:
			switch (rfile) {
				case 0x00:
				case 0x01:
				case 0x02:
				case 0x03:
					bs->d_s_xrdst = dst;
					bs->d_s_vslot = rfile;
					bs->d_s_mode = S_MODE_X2R;
					bs->d_x_vsrc = src1;
					break;
				case 0x0b:
					if (op_b != 0x1f)
						bs->d_s_xrdst = dst;
					bs->d_s_mode = S_MODE_X2R;
					bs->d_x_lsrc = src1 & 3;
					break;
				case 0x0c:
					bs->d_s_xrdst = dst;
					bs->d_s_mode = S_MODE_X2R;
					bs->d_x_asrc = src1;
					break;
				case 0x0d:
					bs->d_s_xrdst = dst;
					bs->d_s_mode = S_MODE_X2R;
					bs->d_x_csrc = src1;
					break;
				case 0x14:
					bs->d_s_rdst = dst;
					bs->d_s_mode = S_MODE_X2R;
					bs->d_x_msrc = src1;
					break;
				case 0x15:
					bs->d_s_rdst = dst;
					bs->d_s_mode = S_MODE_X2R;
					bs->d_x_msrc = 32 + src1;
					break;
				case 0x18:
					bs->d_s_rdst = dst;
					bs->d_s_mode = S_MODE_X2R;
					bs->d_x_xsrc = src1 & 0xf;
					break;
			}
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
			break;
		case 0x45:
			bs->d_s2v_valid = true;
			bs->d_s_rdst = src1;
			bs->d_s_cdst = -1;
			bs->d_s_mode = S_MODE_VECMS;
			break;
		case 0x04:
			bs->d_s2v_valid = true;
			bs->d_s_cdst = -1;
			bs->d_s_sign2 = true;
			bs->d_s_sign3 = true;
			bs->d_s_route = S_ROUTE_VECMAD;
			bs->d_s_mode = S_MODE_BVECMAD;
			break;
		case 0x05:
			bs->d_s2v_valid = true;
			bs->d_s_cdst = -1;
			bs->d_s_sign2 = true;
			bs->d_s_sign3 = true;
			bs->d_s_route = S_ROUTE_VECMAD;
			bs->d_s_mode = S_MODE_BVECMADSEL;
			break;
		case 0x0f:
			bs->d_s_cdst = -1;
			bs->d_s2v_valid = true;
			bs->d_s_sign1 = true;
			bs->d_s_mode = S_MODE_BVEC;
			break;
		case 0x24:
			bs->d_s_cdst = -1;
			bs->d_s2v_valid = true;
			bs->d_s_fimm[0] = extrs(opcode, 1, 9);
			bs->d_s_fimm[1] = extrs(opcode, 10, 9);
			bs->d_s_mode = S_MODE_VEC;
			break;
		default:
			bs->d_s_cdst = -1;
			break;
	}
	if (bs->d_s2v_valid) {
		bs->d_s2v_vcsrc = opcode >> 19 & 3;
		bs->d_s2v_vcpart = opcode >> 21 & 1;
		bs->d_s2v_vcmode = (opcode >> 22 & 3) | (opcode << 2 & 4);
	} else {
		bs->d_s2v_vcpart = 0;
		bs->d_s2v_vcmode = 0;
	}
}

static void preread_op_s(struct vp1_bs *bs, struct vp1_ctx *ctx) {
	bs->p_s_cin = ctx->c[bs->d_s_csrc];
	/* adjust phase */
	int u;
	switch (bs->d_s_route) {
		case S_ROUTE_NOP:
			bs->a_s_rsrc2 = bs->d_s_rsrc2;
			bs->a_s_rsrc3 = -1;
			break;
		case S_ROUTE_S2S:
			bs->a_s_rsrc2 = vp1_mangle_reg(bs->d_s_rsrc2, bs->p_s_cin, bs->d_s_slct, 0);
			bs->a_s_rsrc3 = -1;
			break;
		case S_ROUTE_VECMAD:
			if (bs->d_s_slct == 4) {
				u = bs->p_s_cin >> 4 & 3;
			} else {
				u = bs->p_s_cin >> bs->d_s_slct & 1;
			}
			bs->a_s_rsrc2 = bs->d_s_rsrc2 | u;
			bs->a_s_rsrc3 = bs->d_s_rsrc2 | 2 | u;
			break;
	}
}

static void read_op_s(struct vp1_bs *bs, struct vp1_ctx *ctx) {
	bs->r_s_rin1 = read_r(ctx, bs->d_s_rsrc1);
	bs->r_s_rin2 = read_r(ctx, bs->a_s_rsrc2);
	if (bs->a_s_rsrc3 != -1)
		bs->r_s_rin3 = read_r(ctx, bs->a_s_rsrc3);
	if (bs->d_x_rsrc != -1) {
		int xsrc = bs->d_x_rsrc;
		if (bs->d_a_rsrc != -1)
			xsrc = bs->d_a_rsrc;
		bs->r_x_rin = read_r(ctx, xsrc);
	}
	if (bs->d_x_xsrc != -1)
		bs->r_x_xin = ctx->x[bs->d_x_xsrc];
	if (bs->d_x_msrc != -1)
		bs->r_x_xin = ctx->m[bs->d_x_msrc];
	if (bs->d_x_csrc != -1) {
		if (bs->d_x_csrc < 4)
			bs->r_x_xin = ctx->c[bs->d_x_csrc];
		else
			bs->r_x_xin = 0;
	}
}

static void execute_op_s(struct vp1_bs *bs) {
	uint32_t s2 = bs->d_s_useimm ? bs->d_s_imm : bs->r_s_rin2;
	uint32_t s1 = bs->r_s_rin1;
	uint8_t b1[4];
	uint8_t b2[4];
	uint8_t b3[4];
	uint8_t br[4];
	int i;
	w2b(b1, s1);
	w2b(b2, s2);
	w2b(b3, bs->r_s_rin3);
	switch (bs->d_s_mode) {
		case S_MODE_NOP:
			break;
		case S_MODE_R2X:
			bs->e_s_res = bs->r_x_rin;
			break;
		case S_MODE_X2R:
			bs->e_s_res = bs->r_x_xin;
			break;
		case S_MODE_LDIMM:
			bs->e_s_res = bs->d_s_imm;
			break;
		case S_MODE_SETHI:
			bs->e_s_res = (s1 & 0xffff) | bs->d_s_imm << 16;
			break;
		case S_MODE_MUL:
			bs->e_s_res = sext(s1, 15) * sext(s2, 15);
			break;
		case S_MODE_MIN:
			bs->e_s_res = vp1_min(s1, s2);
			break;
		case S_MODE_MAX:
			bs->e_s_res = vp1_max(s1, s2);
			break;
		case S_MODE_ABS:
			bs->e_s_res = vp1_abs(s1);
			break;
		case S_MODE_NEG:
			bs->e_s_res = -s1;
			break;
		case S_MODE_ADD:
			bs->e_s_res = s1 + s2;
			break;
		case S_MODE_SUB:
			bs->e_s_res = s1 - s2;
			break;
		case S_MODE_SHR:
			bs->e_s_res = vp1_shr(s1, s2, bs->d_s_sign1);
			break;
		case S_MODE_LOGOP:
			bs->e_s_res = vp1_logop(s1, s2, bs->d_s_logop);
			break;
		case S_MODE_VECMS:
			{
				uint16_t mask = 0;
				for (i = 0; i < 4; i++)
					if (s1 & 1 << i)
						mask |= 0xf << i * 4;
				bs->e_s2v_factor[0] = (mask & 0xff) << 1;
				bs->e_s2v_factor[1] = (mask >> 8 & 0xff) << 1;
				bs->e_s2v_factor[2] = 0;
				bs->e_s2v_factor[3] = 0;
				bs->e_s_res = (int32_t)s1 >> 4;
			}
			break;
		case S_MODE_VEC:
			bs->e_s2v_factor[0] = bs->d_s_fimm[0];
			bs->e_s2v_factor[1] = bs->d_s_fimm[0];
			bs->e_s2v_factor[2] = bs->d_s_fimm[1];
			bs->e_s2v_factor[3] = bs->d_s_fimm[1];
			break;
		default:
			for (i = 0; i < 4; i++) {
				int32_t s1 = bs->d_s_sign1 ? (int8_t)b1[i] : b1[i];
				int32_t s2 = bs->d_s_sign2 ? (int8_t)b2[i] : b2[i];
				int32_t s3 = bs->d_s_sign2 ? (int8_t)b3[i] : b3[i];
				int32_t res = 0;
				switch (bs->d_s_mode) {
					case S_MODE_BMIN:
						res = vp1_min(s1, s2);
						break;
					case S_MODE_BMAX:
						res = vp1_max(s1, s2);
						break;
					case S_MODE_BABS:
						res = vp1_abs(s1);
						break;
					case S_MODE_BNEG:
						res = -s1;
						break;
					case S_MODE_BADD:
						res = s1 + s2;
						break;
					case S_MODE_BSUB:
						res = s1 - s2;
						break;
					case S_MODE_BSHR:
						res = vp1_shrb(s1, s2);
						break;
					case S_MODE_BMUL:
						if (bs->d_s_sign1)
							s1 <<= 1;
						if (bs->d_s_sign2)
							s2 <<= 1;
						res = s1 * s2;
						if (!bs->d_s_signd)
							res <<= 1;
						if (bs->d_s_rnd)
							res += 0x100;
						res >>= 9;
						break;
					case S_MODE_BVEC:
						bs->e_s2v_factor[i] = s1 << 1;
						break;
					case S_MODE_BVECMAD:
					case S_MODE_BVECMADSEL:
						{
							uint8_t s1 = bs->r_s_rin1 >> 11 & 0xff;
							if (bs->d_s_mode == S_MODE_BVECMADSEL)
								s1 &= 0x7f;
							res = s2 * 0x100 + s1 * s3 + 0x40;
							bs->e_s2v_factor[i] = res >> 7;
						}
						break;
					default:
						abort();
				}
				if (bs->d_s_clip) {
					if (bs->d_s_signd) {
						if (res < -0x80)
							res = -0x80;
						if (res > 0x7f)
							res = 0x7f;
					} else {
						if (res < 0)
							res = 0;
						if (res > 0xff)
							res = 0xff;
					}
				}
				br[i] = res;
			}
			if (bs->d_s_mode == S_MODE_BVECMADSEL) {
				bool which = (bs->d_s_slct == 2 && bs->p_s_cin & 0x80);
				for (i = 0; i < 2; i++) {
					int16_t f = bs->e_s2v_factor[i*2 + which];
					bs->e_s2v_factor[i*2] = f;
					bs->e_s2v_factor[i*2+1] = f;
				}
			}
			bs->e_s_res = b2w(br);
			break;
	}
	switch (bs->d_s_cmode) {
		case S_CMODE_ZERO:
			bs->e_s_cres = 0;
			break;
		case S_CMODE_PART:
			bs->e_s_cres = cond_s(bs->e_s_res, bs->chipset);
			break;
		case S_CMODE_FULL:
			bs->e_s_cres = cond_s_f(bs->e_s_res, bs->r_s_rin1, bs->chipset);
			break;
		case S_CMODE_FULL_ZERO:
			bs->e_s_cres = cond_s_f(bs->e_s_res, 0, bs->chipset);
			break;
		default:
			abort();
	}
	bs->e_s2v_mask[0] = (bs->e_s2v_factor[0] >> 1 & 0xff) | (bs->e_s2v_factor[1] << 7 & 0xff00);
	bs->e_s2v_mask[1] = (bs->e_s2v_factor[2] >> 1 & 0xff) | (bs->e_s2v_factor[3] << 7 & 0xff00);
}

static void write_op_s(struct vp1_bs *bs, struct vp1_ctx *ectx) {
	if (bs->d_s_cdst != -1 && bs->d_s_cdst < 4) {
		ectx->c[bs->d_s_cdst] &= ~0xff;
		ectx->c[bs->d_s_cdst] |= bs->e_s_cres;
	}
	if (bs->d_s_xrdst != -1 && bs->d_s_xrdst != 31) {
		ectx->r[bs->d_s_xrdst] = bs->e_s_res;
	}
	if (bs->d_a_rdst != -1 && bs->d_a_rdst != 31) {
		ectx->r[bs->d_a_rdst] = b2w(bs->e_a_vres + bs->e_a_rslot * 4);
	}
	if (bs->d_s_rdst != -1 && bs->d_s_rdst != 31) {
		ectx->r[bs->d_s_rdst] = bs->e_s_res;
	}
	if (bs->d_s_xdst != -1) {
		ectx->x[bs->d_s_xdst] = bs->e_s_res;
	}
	if (bs->d_s_mdst != -1) {
		ectx->m[bs->d_s_mdst] = bs->e_s_res;
	}
}

static void decode_op_v(struct vp1_bs *bs, uint32_t opcode) {
	bs->d_v_mode = V_MODE_NOP;
	bs->d_v_fmode = V_FMODE_RAW;
	bs->d_v_vadst = false;
	bs->d_v_vdst = opcode >> 19 & 0x1f;
	bs->d_v_vsrc1 = opcode >> 14 & 0x1f;
	bs->d_v_vsrc2 = opcode >> 9 & 0x1f;
	bs->d_v_vsrc3 = opcode >> 4 & 0x1f;
	bs->d_v_csrc = opcode >> 3 & 3;
	bs->d_v_slct = opcode >> 5 & 0xf;
	bs->d_v_vcsrc = opcode & 3;
	bs->d_v_vcpart = opcode >> 2 & 1;
	bs->d_v_useimm = false;
	bs->d_v_sign1 = false;
	bs->d_v_sign2 = false;
	bs->d_v_sign3 = false;
	bs->d_v_mask = false;
	bs->d_v_route = V_ROUTE_NOP;
	bs->d_v_imm = opcode >> 3 & 0xff;
	bs->d_v_use_s2v_vc = false;
	uint32_t op = opcode >> 24 & 0x3f;
	int subop = op & 0xf;
	uint32_t vci = opcode & 7;
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
			bs->d_v_cdst = -1;
			if (subop == 0 || subop == 3 || subop == 4 || subop == 6) {
				bs->d_v_vdst = -1;
			}
			bs->d_v_vadst = true;
			bs->d_v_fractint = !!(opcode & 8);
			bs->d_v_sign1 = !!(opcode & 4);
			bs->d_v_sign2 = !!(opcode & 2);
			bs->d_v_sign3 = !!(opcode & 4);
			bs->d_v_signd = !(op & 0x10);
			bs->d_v_hilo = !!(opcode & 0x10);
			bs->d_v_rnd = !!(opcode & 0x100);
			bs->d_v_shift = extrs(opcode, 5, 3);
			bs->d_v_useimm = !!(op & 0x20);
			bs->d_v_mask = !!(opcode & 1);
			if (op == 0x30)
				bs->d_v_imm = opcode & 0xff;
			else
				bs->d_v_imm = ((opcode & 1) << 5 | bs->d_v_vsrc2) << 2;
			bs->d_v_mode = V_MODE_MAD;
			if (subop == 2 || subop == 3 || subop == 6 || subop == 7) {
				bs->d_v_mad_a = V_MAD_A_VA;
			} else if (subop == 4 || subop == 5) {
				bs->d_v_mad_a = V_MAD_A_S2;
			} else {
				bs->d_v_mad_a = V_MAD_A_ZERO;
			}
			if (subop < 4) {
				bs->d_v_mad_s2v = false;
			} else {
				bs->d_v_mad_s2v = true;
			}
			bs->d_v_mad_b = V_MAD_B_S1;
			bs->d_v_mad_d = V_MAD_D_S3;
			if (op == 0x16 || op == 0x26 || op == 0x27)
				bs->d_v_route = V_ROUTE_NOP;
			else
				bs->d_v_route = V_ROUTE_S1D;
			bs->d_v_use_s2v_vc = true;
			break;
		case 0x33:
			bs->d_v_route = V_ROUTE_Q230;
			bs->d_v_cdst = -1;
			bs->d_v_vadst = !!(opcode & 0x800);
			bs->d_v_mode = V_MODE_MAD;
			bs->d_v_mad_a = V_MAD_A_S3X;
			bs->d_v_mad_s2v = true;
			bs->d_v_mad_b = V_MAD_B_S1MS3;
			bs->d_v_mad_d = V_MAD_D_S2MS3;
			bs->d_v_signd = !!(opcode & 0x1000);
			bs->d_v_sign1 = !!(opcode & 0x200);
			bs->d_v_sign2 = !!(opcode & 0x200);
			bs->d_v_sign3 = !!(opcode & 0x200);
			bs->d_v_shift = extrs(opcode, 5, 3);
			bs->d_v_hilo = false;
			bs->d_v_rnd = !!(opcode & 0x100);
			bs->d_v_fractint = false;
			bs->d_v_lrp2x = !!(opcode & 0x400);
			break;
		case 0x34:
			bs->d_v_route = V_ROUTE_Q230;
			bs->d_v_cdst = -1;
			bs->d_v_vdst = -1;
			bs->d_v_vadst = true;
			bs->d_v_rnd = !!(opcode & 0x100);
			bs->d_v_fractint = false;
			bs->d_v_signd = false;
			bs->d_v_shift = extrs(opcode, 5, 3);
			bs->d_v_hilo = true;
			bs->d_v_mode = V_MODE_MAD;
			bs->d_v_mad_a = V_MAD_A_S3X;
			bs->d_v_mad_b = V_MAD_B_S1MS3;
			bs->d_v_mad_d = V_MAD_D_S2MS3;
			bs->d_v_mad_s2v = true;
			break;
		case 0x35:
			bs->d_v_route = V_ROUTE_Q23S2;
			bs->d_v_cdst = -1;
			bs->d_v_vdst = -1;
			bs->d_v_vadst = true;
			bs->d_v_rnd = !!(opcode & 0x100);
			bs->d_v_fractint = false;
			bs->d_v_sign2 = true;
			bs->d_v_signd = false;
			bs->d_v_shift = extrs(opcode, 5, 3);
			bs->d_v_hilo = true;
			bs->d_v_mode = V_MODE_MAD;
			bs->d_v_mad_a = V_MAD_A_US2;
			bs->d_v_mad_b = V_MAD_B_S1MS3;
			bs->d_v_mad_d = V_MAD_D_S3;
			bs->d_v_mad_s2v = true;
			break;
		case 0x36:
		case 0x37:
			bs->d_v_cdst = -1;
			bs->d_v_vadst = true;
			bs->d_v_fractint = false;
			bs->d_v_shift = extrs(opcode, 11, 3);
			bs->d_v_rnd = !!(opcode & 0x200);
			bs->d_v_hilo = false;
			bs->d_v_signd = op == 0x37;
			bs->d_v_mode = V_MODE_MAD;
			bs->d_v_mad_a = V_MAD_A_VA;
			bs->d_v_mad_b = V_MAD_B_S1MS3;
			bs->d_v_mad_d = V_MAD_D_S2MS3;
			bs->d_v_mad_s2v = true;
			bs->d_v_route = V_ROUTE_Q10X;
			break;
		case 0x2a:
		case 0x2b:
		case 0x2f:
		case 0x3a:
			bs->d_v_cdst = vci;
			if (op == 0x2a)
				bs->d_v_logop = 0x8;
			else if (op == 0x2b)
				bs->d_v_logop = 0x6;
			else if (op == 0x2f)
				bs->d_v_logop = 0xe;
			else if (op == 0x3a)
				bs->d_v_logop = 0xc;
			else
				abort();
			bs->d_v_useimm = true;
			bs->d_v_mode = V_MODE_LOGOP;
			bs->d_v_fmode = V_FMODE_RAW_ZERO;
			break;
		case 0x0f:
			bs->d_v_route = V_ROUTE_S2SS1D;
			bs->d_v_cdst = vci;
			bs->d_v_vdst = -1;
			bs->d_v_fmode = V_FMODE_CMPAD;
			bs->d_v_logop = opcode >> 19 & 0xf;
			bs->d_v_use_s2v_vc = true;
			break;
		case 0x10:
			bs->d_v_route = V_ROUTE_S1D;
			bs->d_v_cdst = -1;
			bs->d_v_mode = V_MODE_MAD;
			bs->d_v_fractint = false;
			bs->d_v_hilo = false;
			bs->d_v_rnd = !!(opcode & 0x100);
			bs->d_v_shift = extrs(opcode, 5, 3);
			bs->d_v_mad_s2v = false;
			bs->d_v_mad_a = V_MAD_A_S3X;
			bs->d_v_mad_b = V_MAD_B_S1MS3;
			break;
		case 0x1b:
			bs->d_v_cdst = -1;
			bs->d_v_mode = V_MODE_SWZ;
			bs->d_v_hilo = !!(opcode & 8);
			break;
		case 0x1f:
			bs->d_v_cdst = vci;
			bs->d_v_mode = V_MODE_ADD9;
			bs->d_v_fmode = V_FMODE_CLIP;
			break;
		case 0x24:
			bs->d_v_cdst = vci;
			bs->d_v_sign1 = true;
			bs->d_v_sign2 = true;
			bs->d_v_sign3 = true;
			bs->d_v_mode = V_MODE_CLIP;
			bs->d_v_fmode = V_FMODE_RAW_SPEC;
			break;
		case 0x25:
			bs->d_v_cdst = vci;
			bs->d_v_sign1 = true;
			bs->d_v_sign2 = true;
			bs->d_v_signd = true;
			bs->d_v_mode = V_MODE_MINABS;
			bs->d_v_fmode = V_FMODE_CLIP;
			break;
		case 0x14:
			bs->d_v_cdst = vci;
			bs->d_v_logop = opcode >> 3 & 0xf;
			bs->d_v_mode = V_MODE_LOGOP;
			bs->d_v_fmode = V_FMODE_RAW_ZERO;
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
			bs->d_v_cdst = vci;
			bs->d_v_sign1 = !(op & 0x10);
			bs->d_v_sign2 = !(op & 0x10);
			bs->d_v_signd = !(op & 0x10);
			bs->d_v_useimm = !!(op & 0x20);
			bs->d_v_fmode = V_FMODE_CLIP;
			switch (op & 0xf) {
				case 0x8:
					bs->d_v_mode = V_MODE_MIN;
					break;
				case 0x9:
					bs->d_v_mode = V_MODE_MAX;
					break;
				case 0xa:
					bs->d_v_mode = V_MODE_ABS;
					break;
				case 0xb:
					bs->d_v_mode = V_MODE_NEG;
					break;
				case 0xc:
					bs->d_v_mode = V_MODE_ADD;
					break;
				case 0xd:
					bs->d_v_mode = V_MODE_SUB;
					break;
				case 0xe:
					bs->d_v_mode = V_MODE_SHR;
					bs->d_v_fmode = V_FMODE_RAW_SIGN;
					break;
				default:
					abort();
			}
			break;
		case 0x2d:
			bs->d_v_cdst = vci;
			bs->d_v_mode = V_MODE_LDIMM;
			bs->d_v_fmode = V_FMODE_RAW_SIGN;
			break;
		case 0x3b:
			bs->d_v_cdst = -1;
			bs->d_v_mode = V_MODE_LDVC;
			bs->d_v_fmode = V_FMODE_RAW;
			break;
		default:
			bs->d_v_cdst = -1;
			bs->d_v_vdst = -1;
			break;
	}
}

static void preread_op_v(struct vp1_bs *bs, struct vp1_ctx *octx) {
	int i;
	bs->p_down = !!(octx->uc_cfg & 1);
	for (i = 0; i < 16; i++)
		bs->p_v_vain[i] = octx->va[i];
	for (i = 0; i < 4; i++)
		bs->p_v_vcin[i] = octx->vc[i];
	bs->p_v_cin = octx->c[bs->d_v_csrc];
	/* adjust phase */
	bs->a_v_vsrc1 = bs->d_v_vsrc1;
	bs->a_v_vsrc2 = bs->d_v_vsrc2;
	bs->a_v_vsrc3 = bs->d_v_vsrc3;
	switch (bs->d_v_route) {
		case V_ROUTE_NOP:
			break;
		case V_ROUTE_S2SS1D:
			bs->a_v_vsrc2 = vp1_mangle_reg(bs->d_v_vsrc2, bs->p_v_cin, bs->d_v_slct, 0);
			/* fall thru */
		case V_ROUTE_S1D:
			bs->a_v_vsrc3 = bs->d_v_vsrc1 | 1;
			break;
		case V_ROUTE_Q230:
			bs->a_v_vsrc1 = vp1_mangle_reg(bs->d_v_vsrc1, bs->p_v_cin, 4, 2);
			bs->a_v_vsrc2 = vp1_mangle_reg(bs->d_v_vsrc1, bs->p_v_cin, 4, 3);
			bs->a_v_vsrc3 = vp1_mangle_reg(bs->d_v_vsrc1, bs->p_v_cin, 4, 0);
			break;
		case V_ROUTE_Q23S2:
			bs->a_v_vsrc1 = vp1_mangle_reg(bs->d_v_vsrc1, bs->p_v_cin, 4, 2);
			bs->a_v_vsrc3 = vp1_mangle_reg(bs->d_v_vsrc1, bs->p_v_cin, 4, 3);
			break;
		case V_ROUTE_Q10X:
			bs->a_v_vsrc1 = vp1_mangle_reg(bs->d_v_vsrc1, bs->p_v_cin, bs->d_v_slct, 1);
			bs->a_v_vsrc2 = 32;
			bs->a_v_vsrc3 = vp1_mangle_reg(bs->d_v_vsrc1, bs->p_v_cin, bs->d_v_slct, 0);
			break;
		default:
			abort();
	}
}

static void read_op_v(struct vp1_bs *bs, struct vp1_ctx *octx) {
	int i;
	if (bs->a_v_vsrc1 != -1) {
		for (i = 0; i < 16; i++)
			bs->r_v_vin1[i] = octx->v[bs->a_v_vsrc1][i];
	}
	if (bs->a_v_vsrc2 == 32) {
		for (i = 0; i < 16; i++)
			bs->r_v_vin2[i] = octx->vx[i];
	} else if (bs->a_v_vsrc2 != -1) {
		for (i = 0; i < 16; i++)
			bs->r_v_vin2[i] = octx->v[bs->a_v_vsrc2][i];
	}
	if (bs->a_v_vsrc3 != -1) {
		for (i = 0; i < 16; i++)
			bs->r_v_vin3[i] = octx->v[bs->a_v_vsrc3][i];
	}
	if (bs->d_a_vsrc != -1 || bs->d_x_vsrc != -1) {
		/* XXX */
		int xsrc1 = bs->d_a_vsrc;
		if (bs->d_x_vsrc != -1)
			xsrc1 = bs->d_x_vsrc;
		int i;
		if (bs->d_a_vsrc != -1) {
			for (i = 0; i < 16; i++) {
				bs->r_a_vin[i] = octx->v[xsrc1][i];
			}
		}
		if (bs->d_x_vsrc != -1)
			bs->r_x_xin = b2w(octx->v[xsrc1] + bs->d_s_vslot * 4);
	}
}

static const int vc_xlat[8][16] = {
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
	{  2,  2,  2,  2,  6,  6,  6,  6, 10, 10, 10, 10, 14, 14, 14, 14 },
	{  4,  5,  4,  5,  4,  5,  4,  5, 12, 13, 12, 13, 12, 13, 12, 13 },
	{  0,  0,  2,  0,  4,  4,  6,  4,  8,  8, 10,  8, 12, 12, 14, 12 },
	{  1,  1,  1,  3,  5,  5,  5,  7,  9,  9,  9, 11, 13, 13, 13, 15 },
	{  0,  0,  2,  2,  4,  4,  6,  6,  8,  8, 10, 10, 12, 12, 14, 14 },
	{  1,  1,  1,  1,  5,  5,  5,  5,  9,  9,  9,  9, 13, 13, 13, 13 },
	{  0,  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 },
};

static void execute_op_v(struct vp1_bs *bs) {
	uint32_t cr = 0;
	int vcsel, vcpart, vcmode;
	if (bs->d_v_use_s2v_vc) {
		vcsel = bs->d_s2v_valid ? bs->d_s2v_vcsrc : bs->d_v_vcsrc;
		vcpart = bs->d_s2v_vcpart;
		vcmode = bs->d_s2v_vcmode;
	} else {
		vcsel = bs->d_v_vcsrc;
		vcpart = bs->d_v_vcpart;
		vcmode = 0;
	}
	int i;
	uint32_t vcond = bs->p_v_vcin[vcsel] >> (vcpart * 16) & 0xffff;
	vcond |= bs->p_v_vcin[vcsel | 1] >> (vcpart * 16) << 16;
	uint16_t scond = 0;
	for (i = 0; i < 16; i++) {
		if (vcond & 1 << vc_xlat[vcmode][i])
			scond |= 1 << i;
	}
	for (i = 0; i < 16; i++) {
		int32_t s1 = bs->r_v_vin1[i];
		int32_t s2 = bs->r_v_vin2[i];
		int32_t s3 = bs->r_v_vin3[i];
		if (bs->d_v_useimm)
			s2 = bs->d_v_imm;
		if (bs->d_v_sign1)
			s1 = (int8_t)s1;
		if (bs->d_v_sign2)
			s2 = (int8_t)s2;
		if (bs->d_v_sign3)
			s3 = (int8_t)s3;
		bool ssf = false;
		int32_t res = 0;
		switch (bs->d_v_mode) {
			case V_MODE_NOP:
				break;
			case V_MODE_MIN:
				res = vp1_min(s1, s2);
				break;
			case V_MODE_MAX:
				res = vp1_max(s1, s2);
				break;
			case V_MODE_ABS:
				res = vp1_abs(s1);
				break;
			case V_MODE_NEG:
				res = -s1;
				break;
			case V_MODE_ADD:
				res = s1 + s2;
				break;
			case V_MODE_SUB:
				res = s1 - s2;
				break;
			case V_MODE_SHR:
				res = vp1_shrb(s1, s2);
				break;
			case V_MODE_LOGOP:
				res = vp1_logop(s1, s2, bs->d_v_logop);
				break;
			case V_MODE_LDIMM:
				res = bs->d_v_imm;
				break;
			case V_MODE_LDVC:
				res = bs->p_v_vcin[i >> 2] >> (i & 3) * 8;
				break;
			case V_MODE_CLIP:
				if (s1 > s3 && s2 > s3) {
					res = vp1_min(s1, s2);
				} else if (s1 < s3 && s2 < s3) {
					res = vp1_max(s1, s2);
				} else {
					res = s3;
				}
				ssf = !(s2 < s1 && s1 < s3);
				break;
			case V_MODE_MINABS:
				res = vp1_min(vp1_abs(s1), vp1_abs(s2));
				break;
			case V_MODE_ADD9:
				{
					uint8_t lo = (i < 8 ? bs->r_v_vin2 : bs->r_v_vin3)[i << 1 & 0xe];
					uint8_t hi = (i < 8 ? bs->r_v_vin2 : bs->r_v_vin3)[(i << 1 & 0xe) | 1];
					int32_t sx = sext(hi << 8 | lo, 8);
					res = s1 + sx;
				}
				break;
			case V_MODE_SWZ:
				{
					int comp, which;
					if (bs->d_v_hilo) {
						comp = s3 >> 4 & 0xf;
						which = s3 & 1;
					} else {
						comp = s3 & 0xf;
						which = s3 >> 4 & 1;
					}
					res = (which ? bs->r_v_vin2 : bs->r_v_vin1)[comp];
				}
				break;
			case V_MODE_MAD:
				{
					int32_t ms1 = vp1_mad_input(s1, bs->d_v_fractint, bs->d_v_sign1);
					int32_t ms2 = vp1_mad_input(s2, bs->d_v_fractint, bs->d_v_sign2);
					int32_t ms3 = vp1_mad_input(s3, bs->d_v_fractint, bs->d_v_sign3);
					int32_t ms3x = vp1_mad_input(s3 ^ bs->d_v_lrp2x << 7, bs->d_v_fractint, bs->d_v_sign3);
					int32_t A;
					switch (bs->d_v_mad_a) {
						case V_MAD_A_ZERO:
							A = 0;
							break;
						case V_MAD_A_VA:
							A = bs->p_v_vain[i];
							break;
						case V_MAD_A_US2:
							A = s2 << vp1_mad_shift(bs->d_v_fractint, bs->d_v_signd, bs->d_v_shift);
							break;
						case V_MAD_A_S2:
							A = ms2 << vp1_mad_shift(bs->d_v_fractint, bs->d_v_signd, bs->d_v_shift);
							break;
						case V_MAD_A_S3X:
							A = ms3x << vp1_mad_shift(bs->d_v_fractint, bs->d_v_signd, bs->d_v_shift);
							break;
						default:
							abort();
					}
					int32_t B;
					int32_t C;
					int32_t D;
					int32_t E;
					if (bs->d_v_mad_b == V_MAD_B_S1) {
						B = ms1;
					} else if (bs->d_v_mad_b == V_MAD_B_S1MS3) {
						B = ms1 - ms3;
					} else {
						abort();
					}
					if (bs->d_v_mad_s2v) {
						if (bs->d_v_mad_d == V_MAD_D_S3) {
							D = ms3;
						} else if (bs->d_v_mad_d == V_MAD_D_S2MS3) {
							D = ms2 - ms3;
						} else {
							abort();
						}
						if (bs->d_v_mask) {
							if (bs->e_s2v_mask[0] & 1 << i)
								C = 0x100;
							else
								C = 0;
							if (bs->e_s2v_mask[1] & 1 << i)
								E = 0x100;
							else
								E = 0;
						} else {
							int cc = scond >> i & 1;
							C = bs->e_s2v_factor[0 | cc];
							E = bs->e_s2v_factor[2 | cc];
						}
					} else {
						C = ms2;
						D = E = 0;
					}
					int32_t acc = vp1_mad(
						A, B, C, D, E,
						bs->d_v_rnd,
						bs->d_v_fractint,
						bs->d_v_signd,
						bs->d_v_shift,
						bs->d_v_hilo,
						bs->p_down);
					bs->e_v_vares[i] = acc;
					res = vp1_mad_read(acc, bs->d_v_fractint, bs->d_v_signd, bs->d_v_shift, bs->d_v_hilo);
				}
				break;
			default:
				abort();
		}
		uint8_t fres = 0;
		bool sf = false;
		bool zf = false;
		switch (bs->d_v_fmode) {
			case V_FMODE_RAW:
				fres = res;
				break;
			case V_FMODE_RAW_ZERO:
				fres = res;
				zf = !fres;
				break;
			case V_FMODE_RAW_SIGN:
				fres = res;
				zf = !fres;
				sf = !!(fres & 0x80);
				break;
			case V_FMODE_CLIP:
				if (!bs->d_v_signd) {
					sf = !!(res & 0x100);
					if (res >= 0x100)
						res = 0x100 - 1;
					if (res < 0)
						res = 0;
				} else {
					sf = res < 0;
					if (res >= 0x80)
						res = 0x80 - 1;
					if (res < -0x80)
						res = -0x80;
				}
				fres = res;
				zf = !fres;
				break;
			case V_FMODE_RAW_SPEC:
				fres = res;
				zf = !fres;
				sf = ssf;
				break;
			case V_FMODE_CMPAD:
				{
					int32_t ad = vp1_abs(s1 - s2);
					int cond = (ad < s3) << 1 | (scond >> i & 1);
					zf = ad == s3;
					sf = !!(bs->d_v_logop & 1 << cond);
				}
				break;
			default:
				abort();
		}
		bs->e_v_vres[i] = fres;
		if (sf)
			cr |= 1 << i;
		if (zf)
			cr |= 1 << (16 + i);
	}
	bs->e_v_cres = cr;
}

static void write_op_v(struct vp1_bs *bs, struct vp1_ctx *ectx) {
	int i;
	if (bs->d_v_cdst != -1 && bs->d_v_cdst < 4) {
		ectx->vc[bs->d_v_cdst] = bs->e_v_cres;
	}
	if (bs->d_s_vdst != -1) {
		for (i = 0; i < 4; i++)
			ectx->v[bs->d_s_vdst][bs->d_s_vslot * 4 + i] = bs->e_s_res >> i * 8;
	}
	if (bs->a_a_vdst != -1) {
		for (i = 0; i < 16; i++)
			ectx->v[bs->a_a_vdst][i] = bs->e_a_vres[i];
	}
	if (bs->d_a_vxdst) {
		for (i = 0; i < 16; i++)
			ectx->vx[i] = bs->e_a_vres[i];
	}
	if (bs->d_v_vdst != -1) {
		for (i = 0; i < 16; i++)
			ectx->v[bs->d_v_vdst][i] = bs->e_v_vres[i];
	}
	if (bs->d_v_vadst) {
		for (i = 0; i < 16; i++)
			ectx->va[i] = bs->e_v_vares[i] & 0xfffffff;
	}
}

static void decode_op_b(struct vp1_bs *bs, uint32_t opcode) {
	switch (opcode >> 24 & 0x1f) {
		case 0x01:
		case 0x03:
		case 0x05:
		case 0x07:
			bs->d_b_lmode = B_LMODE_LOOP;
			bs->d_b_lsrc = opcode >> 3 & 3;
			bs->d_b_ldst = opcode & 3;
			bs->d_b_cdst = opcode & 7;
			break;
		case 0x10:
			bs->d_b_lmode = B_LMODE_MOV;
			bs->d_b_ldst = bs->d_b_cdst = opcode >> 19 & 3;
			bs->d_b_lsrc = -1;
			bs->d_b_imm = opcode & 0xffff;
			break;
		case 0x0a:
		case 0x0f:
		case 0x1f:
			bs->d_b_lmode = B_LMODE_NOP;
			bs->d_b_ldst = -1;
			bs->d_b_lsrc = -1;
			bs->d_b_cdst = -1;
			break;
		default:
			bs->d_b_lmode = B_LMODE_NOP;
			bs->d_b_ldst = -1;
			bs->d_b_lsrc = -1;
			bs->d_b_cdst = opcode & 7;
			break;
	}
}

static void read_op_b(struct vp1_bs *bs, struct vp1_ctx *octx) {
	if (bs->d_b_lsrc != -1) {
		bs->r_b_lin = octx->b[bs->d_b_lsrc];
	}
	if (bs->d_x_lsrc != -1) {
		bs->r_x_xin = octx->b[bs->d_x_lsrc];
	}
}

static void execute_op_b(struct vp1_bs *bs) {
	uint16_t val;
	switch (bs->d_b_lmode) {
		case B_LMODE_LOOP:
			val = bs->r_b_lin;
			if (val & 0xff) {
				val -= 1;
			} else {
				val |= val >> 8;
			}
			break;
		case B_LMODE_MOV:
			val = bs->d_b_imm;
			break;
		case B_LMODE_NOP:
			val = 0;
			break;
		default:
			abort();
	}
	bs->e_b_lres = val;
	bs->e_b_cres = !(val & 0xff);
}

static void write_op_b(struct vp1_bs *bs, struct vp1_ctx *ectx) {
	if (bs->d_s_ldst != -1 && bs->d_s_ldst < 4) {
		ectx->b[bs->d_s_ldst] = bs->e_s_res & 0xffff;
	}
	if (bs->d_b_ldst != -1) {
		ectx->b[bs->d_b_ldst] = bs->e_b_lres;
	}
	if (bs->d_b_cdst != -1 && bs->d_b_cdst < 4) {
		ectx->c[bs->d_b_cdst] &= ~0x2000;
		ectx->c[bs->d_b_cdst] |= bs->e_b_cres << 13;
	}
}

static void simulate_bundle(struct vp1_ctx *ctx, const uint32_t opcode[4], int chipset) {
	struct vp1_bs bs = { 0 };
	bs.chipset = chipset;
	decode_op_a(&bs, opcode[0]);
	decode_op_s(&bs, opcode[1], opcode[3] >> 24 & 0x1f);
	decode_op_v(&bs, opcode[2]);
	decode_op_b(&bs, opcode[3]);
	preread_op_a(&bs, ctx);
	preread_op_s(&bs, ctx);
	preread_op_v(&bs, ctx);
	read_op_a(&bs, ctx);
	read_op_s(&bs, ctx);
	read_op_v(&bs, ctx);
	read_op_b(&bs, ctx);
	execute_op_a(&bs);
	execute_op_s(&bs);
	execute_op_v(&bs);
	execute_op_b(&bs);
	ea_op_a(&bs);
	memory_op_a(&bs, ctx);
	write_op_a(&bs, ctx);
	write_op_s(&bs, ctx);
	write_op_v(&bs, ctx);
	write_op_b(&bs, ctx);
}

static void read_v(struct hwtest_ctx *ctx, int idx, uint8_t *v) {
	int i, j;
	for (i = 0; i < 4; i++) {
		uint32_t val = nva_rd32(ctx->cnum, 0xf000 + idx * 4 + i * 0x80);
		for (j = 0; j < 4; j++)
			v[i*4+j] = val >> 8 * j;
	}
}

static void write_v(struct hwtest_ctx *ctx, int idx, const uint8_t *v) {
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
static void write_va(struct hwtest_ctx *ctx, const uint32_t *va) {
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
static void write_vx(struct hwtest_ctx *ctx, const uint8_t *val) {
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

static void gen_ctx(struct hwtest_ctx *ctx, struct vp1_ctx *gctx) {
	gctx->uc_cfg = jrand48(ctx->rand48) & 0x111;
	int j, k;
	for (j = 0; j < 16; j++) {
		uint32_t val = jrand48(ctx->rand48);
		int which = jrand48(ctx->rand48) & 0xf;
		if (which < 4) {
			val &= ~(0x7f << which * 8);
		}
		gctx->x[j] = val;
	}
	for (j = 0; j < 32; j++) {
		uint32_t val = jrand48(ctx->rand48);
		int which = jrand48(ctx->rand48) & 0xf;
		if (which < 4) {
			val &= ~(0x7f << which * 8);
		}
		gctx->a[j] = val;
	}
	for (j = 0; j < 31; j++) {
		uint32_t val = jrand48(ctx->rand48);
		int which = jrand48(ctx->rand48) & 0xf;
		if (which < 4) {
			val &= ~(0x7f << which * 8);
		}
		gctx->r[j] = val;
	}
	for (j = 0; j < 32; j++) {
		for (k = 0; k < 16; k++) {
			uint32_t val = jrand48(ctx->rand48);
			int which = jrand48(ctx->rand48) & 0x3;
			if (!which) {
				val &= ~0x7f;
			}
			gctx->v[j][k] = val;
		}
	}
	for (j = 0; j < 4; j++) {
		uint32_t val = jrand48(ctx->rand48) & 0xffff;
		int which = jrand48(ctx->rand48) & 0x7;
		if (which < 2) {
			val &= ~(0x7f << which * 8);
		}
		gctx->b[j] = val;
	}
	for (j = 0; j < 4; j++)
		gctx->vc[j] = jrand48(ctx->rand48);
	for (j = 0; j < 16; j++)
		gctx->va[j] = jrand48(ctx->rand48) & 0xfffffff;
	for (j = 0; j < 16; j++)
		gctx->vx[j] = jrand48(ctx->rand48);
	for (j = 0; j < 4; j++)
		gctx->c[j] = clean_c(jrand48(ctx->rand48), ctx->chipset);
	for (j = 0; j < 64; j++) {
		uint32_t val = jrand48(ctx->rand48);
		int which = jrand48(ctx->rand48) & 0xf;
		if (which < 4) {
			val &= ~(0x7f << which * 8);
		}
		gctx->m[j] = val;
	}
	for (j = 0; j < 0x10; j++) {
		for (k = 0; k < 0x200; k++) {
			gctx->ds[j][k] = jrand48(ctx->rand48);
		}
	}
}

static void write_ctx(struct hwtest_ctx *ctx, const struct vp1_ctx *octx) {
	int j, k;
	write_vx(ctx, octx->vx);
	for (j = 0; j < 0x200; j++) {
		uint8_t row[16];
		for (k = 0; k < 16; k++)
			row[k] = octx->ds[k][j];
		write_v(ctx, 0, row);
		nva_wr32(ctx->cnum, 0xf600, j << 4);
		nva_wr32(ctx->cnum, 0xf448, 0xd7000001);
		nva_wr32(ctx->cnum, 0xf458, 1);
	}
	for (j = 0; j < 4; j++) {
		write_c(ctx, j, octx->c[j]);
		write_vc(ctx, j, octx->vc[j]);
	}
	write_va(ctx, octx->va);
	for (j = 0; j < 16; j++) {
		nva_wr32(ctx->cnum, 0xf780, octx->x[j]);
		nva_wr32(ctx->cnum, 0xf44c, 0x6a0000c7 | j << 19);
		nva_wr32(ctx->cnum, 0xf458, 1);
	}
	nva_wr32(ctx->cnum, 0xf540, octx->uc_cfg);
	for (j = 0; j < 32; j++) {
		nva_wr32(ctx->cnum, 0xf600 + j * 4, octx->a[j]);
	}
	for (j = 0; j < 31; j++) {
		nva_wr32(ctx->cnum, 0xf780 + j * 4, octx->r[j]);
	}
	for (j = 0; j < 32; j++) {
		write_v(ctx, j, octx->v[j]);
	}
	for (j = 0; j < 4; j++) {
		nva_wr32(ctx->cnum, 0xf580 + j * 4, octx->b[j]);
	}
	for (j = 0; j < 64; j++) {
		nva_wr32(ctx->cnum, 0xfa00 + j * 4, octx->m[j]);
	}
}

static void read_ctx(struct hwtest_ctx *ctx, struct vp1_ctx *nctx) {
	int j, k;
	nctx->uc_cfg = nva_rd32(ctx->cnum, 0xf540);
	for (j = 0; j < 32; j++)
		nctx->a[j] = nva_rd32(ctx->cnum, 0xf600 + j * 4);
	for (j = 0; j < 31; j++)
		nctx->r[j] = nva_rd32(ctx->cnum, 0xf780 + j * 4);
	for (j = 0; j < 32; j++)
		read_v(ctx, j, nctx->v[j]);
	for (j = 0; j < 4; j++)
		nctx->c[j] = nva_rd32(ctx->cnum, 0xf680 + j * 4);
	for (j = 0; j < 4; j++)
		nctx->b[j] = nva_rd32(ctx->cnum, 0xf580 + j * 4);
	for (j = 0; j < 64; j++)
		nctx->m[j] = nva_rd32(ctx->cnum, 0xfa00 + j * 4);
	for (j = 0; j < 16; j++) {
		nva_wr32(ctx->cnum, 0xf44c, 0x6b0000c0 | j << 14);
		nva_wr32(ctx->cnum, 0xf458, 1);
		nctx->x[j] = nva_rd32(ctx->cnum, 0xf780);
	}
	read_vc(ctx, nctx->vc);
	read_va(ctx, nctx->va);
	uint8_t zero[16] = { 0 };
	write_v(ctx, 1, zero);
	for (j = 0; j < 0x200; j++) {
		uint8_t row[16];
		nva_wr32(ctx->cnum, 0xf600, j << 4);
		nva_wr32(ctx->cnum, 0xf448, 0xd7000200);
		nva_wr32(ctx->cnum, 0xf458, 1);
		read_v(ctx, 0, row);
		for (k = 0; k < 16; k++)
			nctx->ds[k][j] = row[k];
	}
	read_vx(ctx, nctx->vx);
}

static void execute_single(struct hwtest_ctx *ctx, const uint32_t opcode[4]) {
	nva_wr32(ctx->cnum, 0xf448, opcode[0]);
	nva_wr32(ctx->cnum, 0xf44c, opcode[1]);
	nva_wr32(ctx->cnum, 0xf450, opcode[2]);
	nva_wr32(ctx->cnum, 0xf454, opcode[3]);
	nva_wr32(ctx->cnum, 0xf458, 1);
}

static void gen_safe_bundle(struct hwtest_ctx *ctx, uint32_t opcode[4]) {
	int j;
	for (j = 0; j < 4; j++)
		opcode[j] = (uint32_t)jrand48(ctx->rand48);
	if (!(jrand48(ctx->rand48) & 0xf)) {
		/* help luck a bit */
		opcode[1] &= ~0x7e000000;
		opcode[1] |= 0x6a000000;
	}
	uint32_t op_a = opcode[0] >> 24 & 0x1f;
	uint32_t op_s = opcode[1] >> 24 & 0x7f;
	uint32_t op_v = opcode[2] >> 24 & 0x3f;
	if (op_s == 0x6a || op_s == 0x6b) {
		int rfile = opcode[1] >> 3 & 0x1f;
		if (
			rfile == 8 ||
			rfile == 9 ||
			rfile == 0xa ||
			rfile == 0x16 ||
			rfile == 0x17 ||
			0)
			opcode[1] = 0x4f000000;
	}
	if (
		op_a == 0x03 || /* [xdld] */
		op_a == 0x07 || /* [xdst] fuckup */
		op_a == 0x0e || /* [xdbar] fuckup */
		op_a == 0x0f || /* [xdwait] fuckup */
		op_a == 0x1b || /* fuckup */
		0)
		opcode[0] = 0xdf000000;
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
		opcode[1] &= 0x00ffffff;
		switch (op_s & 7) {
			case 0:
				opcode[1] |= 0x0f000000;
				break;
			case 1:
				opcode[1] |= 0x24000000;
				break;
			case 2:
				opcode[1] |= 0x45000000;
				break;
			case 3:
				opcode[1] |= 0x04000000;
				break;
			case 4:
				opcode[1] |= 0x05000000;
				break;
			default:
				opcode[1] |= 0x05000000;
				break;
		}
		op_s = opcode[1] >> 24;
	}
}

static void diff_ctx(struct vp1_ctx *octx, struct vp1_ctx *ectx, struct vp1_ctx *nctx) {
	int j, k;
	printf("what        initial    expected   real\n");
#define PRINT(name, x) printf(name "0x%08x 0x%08x 0x%08x%s\n", octx->x, ectx->x, nctx->x, (nctx->x != ectx->x ? " *" : ""))
#define IPRINT(name, x) printf(name "0x%08x 0x%08x 0x%08x%s\n", j, octx->x, ectx->x, nctx->x, (nctx->x != ectx->x ? " *" : ""))
#define IIPRINT(name, x) printf(name "0x%08x 0x%08x 0x%08x%s\n", j, k, octx->x, ectx->x, nctx->x, (nctx->x != ectx->x ? " *" : ""))
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
}

static int test_isa_s(struct hwtest_ctx *ctx) {
	int i;
	nva_wr32(ctx->cnum, 0x200, 0xfffffffd);
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	for (i = 0; i < 5000000; i++) {
		uint32_t opcode[4];
		gen_safe_bundle(ctx, opcode);
		struct vp1_ctx octx, ectx, nctx;
		gen_ctx(ctx, &octx);
		ectx = octx;
		write_ctx(ctx, &octx);
		execute_single(ctx, opcode);
		simulate_bundle(&ectx, opcode, ctx->chipset);
		/* XXX wait? */
		read_ctx(ctx, &nctx);
		if (memcmp(&ectx, &nctx, sizeof ectx)) {
			printf("Mismatch on try %d for insn 0x%08"PRIx32" 0x%08"PRIx32" 0x%08"PRIx32" 0x%08"PRIx32"\n", i, opcode[0], opcode[1], opcode[2], opcode[3]);
			diff_ctx(&octx, &ectx, &nctx);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

enum vp1_kind {
	VP1_KIND_A = 0,
	VP1_KIND_S = 1,
	VP1_KIND_V = 2,
	VP1_KIND_B = 3,
};

static const char vp1_kinds[4] = "ASVB";

static void execute(struct hwtest_ctx *ctx, uint32_t *insns, int num) {
	/* upload code */
	int j;
	for (j = 0; j < num; j++)
		nva_wr32(ctx->cnum, 0x700000 + j * 4, insns[j]);
	/* flush */
	nva_wr32(ctx->cnum, 0x330c, 1);
	while (nva_rd32(ctx->cnum, 0x330c));
	/* kick */
	nva_wr32(ctx->cnum, 0xfc94, 0xc0000000);
	/* wait until done */
	while (nva_rd32(ctx->cnum, 0xf43c));
}

static int bundle(int kind[4]) {
	int i;
	int res = 0;
	for (i = 0; i < 3; i++) {
		if (kind[i+1] > kind[i])
			res |= 1 << i;
	}
	return res;
}

static void exec_prepare(struct hwtest_ctx *ctx) {
	/* reset VP1 */
	nva_wr32(ctx->cnum, 0x200, 0xfffffffd);
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	/* aim memory window at 0x40000 */
	nva_wr32(ctx->cnum, 0x1700, 0x00000004);
	/* aim UCODE portals */
	nva_wr32(ctx->cnum, 0xf464, 0x00040000);
	nva_wr32(ctx->cnum, 0xf468, 0x00040000);
	nva_wr32(ctx->cnum, 0xf46c, 0x00040000);
	/* enable direct FIFO interface */
	nva_wr32(ctx->cnum, 0xfc90, 1);
	/* enable vp1 */
	nva_wr32(ctx->cnum, 0xf474, 0x111);
}

static int test_isa_bundle(struct hwtest_ctx *ctx) {
	uint32_t combo = 0;
	int i;
	for (combo = 0; combo < 0x10000; combo++) {
		int kind[8];
		char seps[7] = "???????";
		for (i = 0; i < 8; i++)
			kind[i] = combo >> i * 2 & 3;
		for (i = 0; i < 7; i++) {
			int first = i, second = i+1;
			char diff_sep = '|';
			if (kind[first] == VP1_KIND_V && kind[second] == VP1_KIND_B) {
				if (i && seps[i-1] == ' ') {
					/* V is in bundle with the previous instruction - check the previous one instead */
					first = i-1;
				} else if (i < 6) {
					/* there is another instruction after B - use it as second, but if | found, stuff * instead - to be fixed later to ? or | depending on result of next test */
					second = i+2;
					diff_sep = '*';
				} else {
					seps[i] = '?';
					continue;
				}
			}
			bool diff;
			uint32_t insns[128];
			int j;
			/* prepare nops of appropriate kinds */
			for (j = 0; j < 8; j++) {
				static const uint32_t nops[4] = {
					0xdfffffff,
					0x4fffffff,
					0xbfffffff,
					0xefffffff,
				};
				insns[j] = nops[kind[j]];
			}
			/* runway for branch slots */
			for (j = 8; j < 16; j++)
				insns[j] = 0xefffffff;
			/* after the test, use the exit insn */
			insns[16] = 0xff00dead;
			/* exit runway */
			for (j = 17; j < 64; j++)
				insns[j] = 0xefffffff;
			/* branch target runway */
			for (j = 64; j < 72; j++)
				insns[j] = 0xefffffff;
			/* mov $r2 1 for branch testing */
			insns[72] = 0x65100001;
			/* secondary exit and runway */
			insns[73] = 0xff00dead;
			for (j = 74; j < 128; j++)
				insns[j] = 0xefffffff;
			exec_prepare(ctx);
			/* do test */
			if (kind[first] == kind[second] && kind[first] == VP1_KIND_V) {
				diff = true;
			} else if (kind[first] == VP1_KIND_V) {
				if (kind[second] == VP1_KIND_B) {
					/* can only happen after adjustment above, which means there were two same kinds in a row, which means different bundles */
					diff = true;
				} else if (kind[second] == VP1_KIND_A) {
					/* vmov $v0 0 */
					insns[first] = 0xad000004;
					/* ldavh $v0 $a0 0 */
					insns[second] = 0xd0000004;
					/* 0 to $a0 */
					nva_wr32(ctx->cnum, 0xf600, 0);
					/* 1 to $v0 */
					nva_wr32(ctx->cnum, 0xf000, 0x01010101);
					nva_wr32(ctx->cnum, 0xf080, 0x01010101);
					nva_wr32(ctx->cnum, 0xf100, 0x01010101);
					nva_wr32(ctx->cnum, 0xf180, 0x01010101);
					/* immediate stavh $v0 $a0 0 */
					nva_wr32(ctx->cnum, 0xf448, 0xd4000004);
					nva_wr32(ctx->cnum, 0xf458, 1);
					/* 2 to $v0 to detect botched execution */
					nva_wr32(ctx->cnum, 0xf000, 0x02020202);
					nva_wr32(ctx->cnum, 0xf080, 0x02020202);
					nva_wr32(ctx->cnum, 0xf100, 0x02020202);
					nva_wr32(ctx->cnum, 0xf180, 0x02020202);
					execute(ctx, insns, 128);
					uint32_t val = nva_rd32(ctx->cnum, 0xf000);
					if (val == 0) {
						diff = false;
					} else if (val == 0x01010101) {
						diff = true;
					} else {
						printf("OOPS, microcode not executed\n");
						continue;
					}
				} else if (kind[second] == VP1_KIND_S) {
					/* vmov $v0 0 */
					insns[first] = 0xad000004;
					/* mov $v0 0 $r0 */
					insns[second] = 0x6a000000;
					/* 1 to $r0 */
					nva_wr32(ctx->cnum, 0xf780, 1);
					execute(ctx, insns, 128);
					diff = nva_rd32(ctx->cnum, 0xf000) == 1;
				} else {
					abort();
				}
			} else {
				int cbit;
				/* VP1 was reset, all $c are 0x8000 */
				if (kind[first] == VP1_KIND_A) {
					/* 0 to $a0 */
					nva_wr32(ctx->cnum, 0xf600, 0);
					/* add $a0 $c0 $a0 $a0 will set $c0 bit 9 */
					insns[first] = 0xcb0001c0;
					cbit = 9;
				} else if (kind[first] == VP1_KIND_S) {
					/* add $r31 $c0 $r31 $r31 will set $c0 bit 1 */
					insns[first] = 0x4cffffc0;
					cbit = 1;
				} else if (kind[first] == VP1_KIND_B) {
					/* mov $l0 $c0 0 */
					insns[first] = 0xf0000000;
					cbit = 13;
				} else {
					abort();
				}
				if (kind[second] == VP1_KIND_A) {
					/* add $a2 $a2 (slct $c0 <flag> $a2d) */
					insns[second] = 0xcb108404 | cbit << 5;
					/* 0 to $a2 */
					nva_wr32(ctx->cnum, 0xf608, 0);
					/* 1 to $a3 */
					nva_wr32(ctx->cnum, 0xf60c, 1);
					execute(ctx, insns, 128);
					diff = nva_rd32(ctx->cnum, 0xf608) == 1;
				} else if (kind[second] == VP1_KIND_S) {
					/* add $r2 $r2 (slct $c0 <flag> $r2d) */
					insns[second] = 0x4c108404 | cbit << 5;
					/* 0 to $a2 */
					nva_wr32(ctx->cnum, 0xf788, 0);
					/* 1 to $a3 */
					nva_wr32(ctx->cnum, 0xf78c, 1);
					execute(ctx, insns, 128);
					diff = nva_rd32(ctx->cnum, 0xf788) == 1;
				} else if (kind[second] == VP1_KIND_V) {
					/* vcmpad 3 $vc0 $v0d (slct $c0 <flag> $v2d) */
					insns[second] = 0x8f180400 | cbit << 5;
					nva_wr32(ctx->cnum, 0xf000, 0);
					nva_wr32(ctx->cnum, 0xf004, 1);
					nva_wr32(ctx->cnum, 0xf008, 0);
					nva_wr32(ctx->cnum, 0xf00c, 1);
					execute(ctx, insns, 128);
					/* immediate mov $v0 $vc */
					nva_wr32(ctx->cnum, 0xf450, 0xbb000000);
					nva_wr32(ctx->cnum, 0xf458, 1);
					diff = !!(nva_rd32(ctx->cnum, 0xf000) & 1);
				} else if (kind[second] == VP1_KIND_B) {
					/* bra $c0 <flag> 0x40 */
					insns[second] = 0xe0002004 | cbit << 5;
					/* 0 to $r2 */
					nva_wr32(ctx->cnum, 0xf788, 0);
					execute(ctx, insns, 128);
					diff = nva_rd32(ctx->cnum, 0xf788) == 1;
				} else {
					abort();
				}
			}
			seps[i] = diff ? diff_sep : ' ';
			if (i && seps[i-1] == '*') {
				seps[i-1] = diff ? '?' : '|';
			}
		}
		int exp0 = bundle(kind);
		int exp1 = bundle(kind+4);
		int our0 = 0, our1 = 0;
		for (i = 0; i < 3; i++) {
			if (seps[i] != '|')
				our0 |= 1 << i;
		}
		for (i = 0; i < 3; i++) {
			if (seps[i+4] != '|')
				our1 |= 1 << i;
		}
		if (exp0 == our0 && exp1 == our1 && (seps[3] == '|' || seps[3] == '?'))
			continue;
		printf("COMBO |%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c real %x %x exp %x %x\n",
			vp1_kinds[kind[0]],
			seps[0],
			vp1_kinds[kind[1]],
			seps[1],
			vp1_kinds[kind[2]],
			seps[2],
			vp1_kinds[kind[3]],
			seps[3],
			vp1_kinds[kind[4]],
			seps[4],
			vp1_kinds[kind[5]],
			seps[5],
			vp1_kinds[kind[6]],
			seps[6],
			vp1_kinds[kind[7]],
			our0, our1,
			exp0, exp1
		);
		return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static int test_isa_delay_slots(struct hwtest_ctx *ctx) {
	uint32_t combo = 0;
	int i;
	for (combo = 0; combo < 0x10000; combo++) {
		int kind[8];
		for (i = 0; i < 8; i++)
			kind[i] = combo >> i * 2 & 3;
		int bslot;
		for (bslot = 0; bslot < 4; bslot++) {
			if (kind[bslot] != VP1_KIND_B)
				continue;
			uint32_t insns[128];
			/* prepare nops of appropriate kinds */
			int j;
			for (j = 0; j < 8; j++) {
				static const uint32_t nops[4] = {
					0xdfffffff,
					0x4fffffff,
					0xbfffffff,
					0xefffffff,
				};
				insns[j] = nops[kind[j]];
			}
			insns[bslot] = 0xe40021e4;
			/* runway just in case */
			for (j = 8; j < 16; j++)
				insns[j] = 0xefffffff;
			/* safety exit insn */
			insns[16] = 0xff00dead;
			/* exit runway */
			for (j = 17; j < 64; j++)
				insns[j] = 0xefffffff;
			/* branch target runway */
			for (j = 64; j < 72; j++)
				insns[j] = 0xefffffff;
			/* exit and runway */
			insns[72] = 0xff00dead;
			for (j = 73; j < 128; j++)
				insns[j] = 0xefffffff;
			exec_prepare(ctx);
			execute(ctx, insns, 128);
			uint32_t ret = nva_rd32(ctx->cnum, 0xf500);
			uint32_t exp = bslot + 1;
			int last = -1;
			while (exp < 8 && kind[exp] > last && (exp != 4 || last == -1)) {
				last = kind[exp];
				exp++;
			}
			if (ret == exp)
				continue;
			printf("COMBO %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c b %d ret %x exp %x\n",
				vp1_kinds[kind[0]],
				bslot == 0 ? '*' : ret == 1 ? '|' : ' ',
				vp1_kinds[kind[1]],
				bslot == 1 ? '*' : ret == 2 ? '|' : ' ',
				vp1_kinds[kind[2]],
				bslot == 2 ? '*' : ret == 3 ? '|' : ' ',
				vp1_kinds[kind[3]],
				bslot == 3 ? '*' : ret == 4 ? '|' : ' ',
				vp1_kinds[kind[4]],
				bslot == 4 ? '*' : ret == 5 ? '|' : ' ',
				vp1_kinds[kind[5]],
				bslot == 5 ? '*' : ret == 6 ? '|' : ' ',
				vp1_kinds[kind[6]],
				bslot == 6 ? '*' : ret == 7 ? '|' : ' ',
				vp1_kinds[kind[7]],
				bslot == 7 ? '*' : ret == 8 ? '|' : ' ',
				bslot,
				ret, exp
			);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_isa_double_delay(struct hwtest_ctx *ctx) {
	uint32_t insns[192];
	int j;
	/* aim at 0x40 */
	insns[0] = 0xe00021e4;
	/* aim at 0x80 */
	insns[1] = 0xe00041e4;
	/* delay runway */
	for (j = 2; j < 10; j++)
		insns[j] = 0x65000000 | j;
	/* delay exit insn */
	insns[10] = 0xff00dead;
	/* exit runway */
	for (j = 11; j < 64; j++)
		insns[j] = 0xefffffff;
	/* branch 1 target runway */
	for (j = 64; j < 72; j++)
		insns[j] = 0x65080000 | j;
	/* exit and runway */
	insns[72] = 0xff00dead;
	for (j = 73; j < 128; j++)
		insns[j] = 0xefffffff;
	/* branch 2 target runway */
	for (j = 128; j < 136; j++)
		insns[j] = 0x65100000 | j;
	/* exit and runway */
	insns[136] = 0xff00dead;
	for (j = 137; j < 192; j++)
		insns[j] = 0xefffffff;
	exec_prepare(ctx);
	nva_wr32(ctx->cnum, 0xf780, 0xdeadbeef);
	nva_wr32(ctx->cnum, 0xf784, 0xdeadbeef);
	nva_wr32(ctx->cnum, 0xf788, 0xdeadbeef);
	execute(ctx, insns, 192);
	uint32_t r0 = nva_rd32(ctx->cnum, 0xf780);
	uint32_t r1 = nva_rd32(ctx->cnum, 0xf784);
	uint32_t r2 = nva_rd32(ctx->cnum, 0xf788);
	if (r0 == 0xdeadbeef && r1 == 0x40 && r2 == 0x87)
		return HWTEST_RES_PASS;
	printf("%08x %08x %08x\n", r0, r1, r2);
	return HWTEST_RES_FAIL;
}

static int vp1_prep(struct hwtest_ctx *ctx) {
	/* XXX some cards have missing VP1 */
	if (ctx->chipset < 0x41 || ctx->chipset >= 0x84)
		return HWTEST_RES_NA;
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(vp1,
	HWTEST_TEST(test_isa_s, 0),
	HWTEST_TEST(test_isa_bundle, 0),
	HWTEST_TEST(test_isa_delay_slots, 0),
	HWTEST_TEST(test_isa_double_delay, 0),
)
