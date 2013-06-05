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
#include "nva.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct vp2_macro_ctx {
	uint32_t lut[32];
	uint32_t lut_idx;
	uint32_t param[2][8];
	uint32_t param_sel;
	uint32_t reg[6];
	uint32_t pred;
	uint32_t data_hi;
	uint32_t data_lo;
	uint32_t dacc;
	uint32_t cmd;
	uint32_t cacc;
};

static void write_reg(struct vp2_macro_ctx *ctx, int idx, uint32_t val) {
	if (idx < 8)
		ctx->param[ctx->param_sel][idx] = val;
	else if (idx < 14)
		ctx->reg[idx-8] = val;
	else if (idx == 15)
		ctx->pred = (val & 0xf) | 1;
}

static uint32_t read_reg(struct vp2_macro_ctx *ctx, int idx) {
	if (idx < 8)
		return ctx->param[ctx->param_sel][idx];
	else if (idx < 14)
		return ctx->reg[idx-8];
	else if (idx == 14)
		return ctx->lut[ctx->lut_idx];
	else if (idx == 15)
		return ctx->pred;
	abort();
}

static void simulate_op(struct vp2_macro_ctx *ctx, uint64_t op) {
	uint32_t data_res;
	int op_pred = ctx->pred >> (op & 3) & 1;
	if (op & 1 << 2)
		op_pred ^= 1;
	if (!op_pred)
		return;
	if (op & 0x10) {
		int inc = 0;
		if ((ctx->cmd & 0x1fe80) == 0xb000)
			inc = 1;
		if (inc)
			ctx->cmd += 4;
	}
	int data_pdst = op >> 31 & 3;
	int data_rdst = op >> 56 & 0xf;
	int data_sdst = op >> 60 & 1;
	int data_op = op >> 61 & 7;
	int cmd_op = op >> 29 & 3;
	int data_rsrc = op >> 52 & 0xf;
	int data_ssrc = op >> 50 & 3;
	int cmd_ssrc = op >> 21 & 3;
	uint32_t data_src1, data_src2;
	uint32_t data_src3 = read_reg(ctx, op >> 23 & 0xf);
	uint32_t cmd_res;
	uint32_t cmd2data;
	int cbf_start = op >> 5 & 0x1f;
	int cbf_end = op >> 10 & 0x1f;
	uint32_t cmask = 0;
	if (cbf_end >= cbf_start)
		cmask = (2 << cbf_end) - (1 << cbf_start);
	data_src1 = read_reg(ctx, data_rsrc);
	int pres = 0;
	switch (data_ssrc) {
		case 0: {
			data_src2 = 0;
			break;
		}
		case 1:
			data_src2 = ctx->cacc;
			break;
		case 2:
			data_src2 = ctx->dacc;
			break;
		case 3:
			data_src2 = data_src1;
			break;
	}
	uint32_t cmd_src2;
	switch (cmd_ssrc) {
		case 0: {
			cmd_src2 = 0;
			break;
		}
		case 1:
			cmd_src2 = ctx->cacc;
			break;
		case 2:
			cmd_src2 = ctx->dacc;
			break;
		case 3:
			cmd_src2 = data_src3;
			break;
	}
	switch (cmd_op) {
		case 0: {
			int cbf_shift = op >> 15 & 0x1f;
			int cbf_shdir = op >> 20 & 1;
			uint32_t ssrc;
			if (cbf_shdir) {
				ssrc = data_src3 >> cbf_shift;
			} else {
				ssrc = data_src3 << cbf_shift;
			}
			cmd_res = cmd_src2 & ~cmask;
			cmd_res |= ssrc & cmask;
			cmd2data = cmd_res;
			pres = !(ssrc & cmask);
			break;
		}
		case 1: {
			int cbf_imm = op >> 15 & 0x3f;
			cmd_res = cmd_src2 & ~cmask;
			cmd_res |= cbf_imm << cbf_start & cmask;
			cmd2data = cmd_res;
			break;
		}			
		case 2: {
			uint32_t cmd_imm = op >> 5 & 0x3ffff;
			if (cmd_imm & 1 << 17)
				cmd_imm |= -1 << 17;
			cmd_res = cmd_imm;
			cmd2data = cmd_res;
			break;
		}
		case 3: {
			uint32_t cmd_imm = op >> 15 & 0xff;
//			printf("NOW %08x %02x %08x\n", data_src3, cmd_imm, cmask);
			uint32_t msrc = (data_src3 & cmask) >> cbf_start;
			cmd_res = ((msrc + cmd_imm) & 0xff) | (msrc & ~0xff);
			cmd2data = msrc;
			break;
		}
		default:
			abort();
	}
	int data_skip_sdst = 0;
	switch (data_op) {
		case 1:
		case 0: {
			int bf_start = op >> 33 & 0x1f;
			int bf_end = op >> 38 & 0x1f;
			int bf_shift = op >> 43 & 0x1f;
			int bf_shdir = op >> 48 & 1;
			int bf_imm = op >> 43 & 0x3f;
			int bf_cres = op >> 49 & 1;
			data_res = data_src2;
			uint32_t mask = 0;
			if (bf_end >= bf_start)
				mask = (2 << bf_end) - (1 << bf_start);
			data_res &= ~mask;
			uint32_t ssrc;
			if (data_op == 0) {
				if (bf_shdir) {
					ssrc = data_src1 >> bf_shift;
					if (bf_shift && data_src1 & 1 << 31)
						ssrc |= -1 << (32 - bf_shift);
				} else {
					ssrc = data_src1 << bf_shift;
				}
			} else { 
				ssrc = bf_imm << bf_start;
			}
			data_res |= ssrc & mask;
			if (data_op == 0) {
				pres = !(ssrc & mask);
			}
			if (bf_cres) {
				data_res &= ~cmask;
				data_res |= cmd2data & cmask;
			}
			break;
		}
		case 2: {
			data_res = op >> 33 & 0x7fffff;
			if (data_res & 1 << 22)
				data_res |= -1 << 22;
			break;
		}
		case 3: {
			uint32_t data_imm = op >> 33 & 0xffff;
			int which = op >> 51 & 1;
			if (which == 0) {
				data_res = ((data_src1 + data_imm) & 0xffff) | (data_src1 & 0xffff0000);
				pres = data_res >> 15 & 1;
			} else {
				data_res = (data_src1 + data_imm * 0x10000);
				pres = data_res >> 31 & 1;
			}
			data_skip_sdst = op >> 49 & 1;
			break;
		}
		case 4: {
			uint32_t data_imm = op >> 33 & 0xffff;
			uint32_t subop = op >> 49 & 3;
			uint32_t which = op >> 51 & 1;
			uint32_t halfop;
			if (which)
				halfop = data_src1 >> 16;
			else
				halfop = data_src1 & 0xffff;
			switch (subop) {
				case 0:
					halfop = data_imm;
					break;
				case 1:
					halfop &= data_imm;
					break;
				case 2:
					halfop |= data_imm;
					break;
				case 3:
					halfop ^= data_imm;
					break;
			}
			if (which)
				data_res = (data_src1 & 0xffff) | halfop << 16;
			else
				data_res = (data_src1 & 0xffff0000) | halfop;
			pres = !halfop;
			break;
		}
		case 5: {
			int shdir = op >> 48 & 1;
			int shift = data_src3 & 0x1f;
			if (shdir) {
				data_res = data_src1 >> shift;
				if (data_src1 & 1 << 31 && shift)
					data_res |= -1 << (32 - shift);
			} else {
				data_res = data_src1 << shift;
			}
			break;
		}
		case 6: {
			int bf_start = op >> 33 & 0x1f;
			int bf_end = op >> 38 & 0x1f;
			int bf_pos = op >> 43 & 0x1f;
			int bf_cres = op >> 49 & 1;
			if (bf_pos > bf_start)
				bf_start = bf_pos;
			uint32_t mask = 0;
			if (bf_end >= bf_start && bf_end >= bf_pos)
				mask = (2 << bf_end) - (1 << bf_start);
			data_res = data_src2 & ~mask;
			pres = data_src2 >> bf_pos & 1;
			if (pres) {
				data_res |= mask;
			}
			if (bf_cres) {
				data_res &= ~cmask;
				data_res |= cmd2data & cmask;
			}
			break;
		}
		case 7: {
			int subop = op >> 49 & 1;
			uint32_t which = op >> 51 & 1;
			uint32_t which2 = op >> 50 & 1;
			uint32_t halfop, halfop2;
			if (which)
				halfop = data_src1 >> 16;
			else
				halfop = data_src1 & 0xffff;
			if (which2)
				halfop2 = data_src3 >> 16;
			else
				halfop2 = data_src3 & 0xffff;
			if (subop)
				halfop = (halfop - halfop2) & 0xffff;
			else
				halfop = (halfop + halfop2) & 0xffff;
			if (which)
				data_res = (data_src1 & 0xffff) | halfop << 16;
			else
				data_res = (data_src1 & 0xffff0000) | halfop;
			pres = halfop >> 15 & 1;
			break;
		}
		default:
			abort();
	}
	int cmd_dst = op >> 27 & 3;
	switch (cmd_dst) {
		case 0:
			ctx->cacc = cmd_res;
			break;
		case 1:
			ctx->cmd = cmd_res & 0x1fffc;
			break;
		case 2:
			ctx->lut_idx = cmd_res & 0x1f;
			break;
		case 3:
			ctx->data_hi = cmd_res & 0xff;
			break;
	}
	write_reg(ctx, data_rdst, data_res);
	if (!data_skip_sdst) {
		if (data_sdst == 1)
			ctx->data_lo = data_res;
		else
			ctx->dacc = data_res;
	}
	if (data_pdst) {
		ctx->pred &= ~(1 << data_pdst);
		ctx->pred |= pres << data_pdst;
	}
}

static int test_isa(struct hwtest_ctx *ctx) {
	int i, j;
	for (i = 0; i < 10000000; i++) {
		nva_wr32(ctx->cnum, 0x200, 0xfffdffff);
		nva_wr32(ctx->cnum, 0x200, 0xffffffff);
		uint64_t op = (uint32_t)jrand48(ctx->rand48);
		op |= (uint64_t)jrand48(ctx->rand48) << 32;
		if ((op >> 29 & 3) == 2 && jrand48(ctx->rand48) & 1) {
			op &= ~0x00000000007ff000ull;
		}
		if (op >> 61 == 2 && jrand48(ctx->rand48) & 1) {
			op &= ~0x00ffff0000000000ull;
		}
		struct vp2_macro_ctx octx, ectx, nctx;
		for (j = 0; j < 32; j++)
			octx.lut[j] = jrand48(ctx->rand48);
		for (j = 0; j < 8; j++) {
			octx.param[0][j] = jrand48(ctx->rand48);
			octx.param[1][j] = jrand48(ctx->rand48);
			if (jrand48(ctx->rand48) & 1) {
				octx.param[0][j] &= 0xff;
				octx.param[1][j] &= 0xff;
			}
		}
		for (j = 0; j < 6; j++) {
			octx.reg[j] = jrand48(ctx->rand48);
			if (jrand48(ctx->rand48) & 1) {
				octx.reg[j] &= 0xff;
			}
		}
		octx.lut_idx = jrand48(ctx->rand48) & 0x1f;
		octx.param_sel = jrand48(ctx->rand48) & 0x1;
		octx.pred = (jrand48(ctx->rand48) & 0xf) | 1;
		octx.data_hi = jrand48(ctx->rand48) & 0xff;
		octx.data_lo = jrand48(ctx->rand48);
		octx.cmd = jrand48(ctx->rand48) & 0x1fffc;
		octx.dacc = jrand48(ctx->rand48);
		octx.cacc = jrand48(ctx->rand48);
		ectx = octx;
		for (j = 0; j < 32; j++) {
			nva_wr32(ctx->cnum, 0xfcfc, j);
			nva_wr32(ctx->cnum, 0xf640, octx.lut[j]);
		}
		for (j = 0; j < 8; j++) {
			nva_wr32(ctx->cnum, 0xfcfc, j);
			nva_wr32(ctx->cnum, 0xf644, octx.param[0][j]);
			nva_wr32(ctx->cnum, 0xf648, octx.param[1][j]);
		}
		for (j = 0; j < 6; j++) {
			nva_wr32(ctx->cnum, 0xfcfc, j);
			nva_wr32(ctx->cnum, 0xf64c, octx.reg[j]);
		}
		nva_wr32(ctx->cnum, 0xfcfc, 7);
		nva_wr32(ctx->cnum, 0xf64c, octx.pred);
		nva_wr32(ctx->cnum, 0xf65c, octx.param_sel);
		nva_wr32(ctx->cnum, 0xf668, octx.data_hi);
		nva_wr32(ctx->cnum, 0xf66c, octx.lut_idx);
		nva_wr32(ctx->cnum, 0xf670, octx.cacc);
		nva_wr32(ctx->cnum, 0xf674, octx.cmd);
		nva_wr32(ctx->cnum, 0xf678, octx.dacc);
		nva_wr32(ctx->cnum, 0xf67c, octx.data_lo);
		nva_wr32(ctx->cnum, 0xf6bc, 0);
		nva_wr32(ctx->cnum, 0xfcfc, 0);
		nva_wr32(ctx->cnum, 0xf6c0, op);
		nva_wr32(ctx->cnum, 0xfcfc, 1);
		nva_wr32(ctx->cnum, 0xf6c0, op >> 32);
		nva_wr32(ctx->cnum, 0xfcfc, 2);
		nva_wr32(ctx->cnum, 0xf6c0, 0x00200028);
		nva_wr32(ctx->cnum, 0xfcfc, 3);
		nva_wr32(ctx->cnum, 0xf6c0, 0x0e080002);
		nva_wr32(ctx->cnum, 0xf680, 0x80000000);
		nva_wr32(ctx->cnum, 0xf684, 0xc100 >> 2);
		nva_wr32(ctx->cnum, 0xf688, 2);
		ectx.param_sel ^= 1;
		simulate_op(&ectx, op);
		while (!nva_rd32(ctx->cnum, 0xf60c));
		for (j = 0; j < 32; j++) {
			nva_wr32(ctx->cnum, 0xfcfc, j);
			nctx.lut[j] = nva_rd32(ctx->cnum, 0xf640);
		}
		for (j = 0; j < 8; j++) {
			nva_wr32(ctx->cnum, 0xfcfc, j);
			nctx.param[0][j] = nva_rd32(ctx->cnum, 0xf644);
			nctx.param[1][j] = nva_rd32(ctx->cnum, 0xf648);
		}
		for (j = 0; j < 6; j++) {
			nva_wr32(ctx->cnum, 0xfcfc, j);
			nctx.reg[j] = nva_rd32(ctx->cnum, 0xf64c);
		}
		nva_wr32(ctx->cnum, 0xfcfc, 7);
		nctx.pred = nva_rd32(ctx->cnum, 0xf64c);
		nctx.param_sel = nva_rd32(ctx->cnum, 0xf65c);
		nctx.data_hi = nva_rd32(ctx->cnum, 0xf668);
		nctx.lut_idx = nva_rd32(ctx->cnum, 0xf66c);
		nctx.cacc = nva_rd32(ctx->cnum, 0xf670);
		nctx.cmd = nva_rd32(ctx->cnum, 0xf674);
		nctx.dacc = nva_rd32(ctx->cnum, 0xf678);
		nctx.data_lo = nva_rd32(ctx->cnum, 0xf67c);
		int bad = 0;
		if (memcmp(&ectx, &nctx, sizeof ectx))
			bad = 1;
		uint32_t pc = nva_rd32(ctx->cnum, 0xf664);
		uint32_t of = nva_rd32(ctx->cnum, 0xf6a4);
		uint32_t epc = 3;
		uint32_t eof = 8;
		if (op & 8)
			epc = 2;
		if (op & 0x10)
			eof = 0xb;
		if (pc != epc || of != eof)
			bad = 1;
		if (bad) {
			printf("Mismatch on try %d for insn 0x%016"PRIx64"\n", i, op);
			printf("what        initial    expected   real\n");
#define PRINT(name, x) printf(name "0x%08x 0x%08x 0x%08x%s\n", octx.x, ectx.x, nctx.x, (nctx.x != ectx.x ? " *" : ""))
#define IPRINT(name, x) printf(name "0x%08x 0x%08x 0x%08x%s\n", j, octx.x, ectx.x, nctx.x, (nctx.x != ectx.x ? " *" : ""))
			for (j = 0; j < 32; j++) {
				IPRINT("LUT[0x%02x]   ", lut[j]);
			}
			for (j = 0; j < 8; j++) {
				IPRINT("PARAM[0][%d] ", param[0][j]);
			}
			for (j = 0; j < 8; j++) {
				IPRINT("PARAM[1][%d] ", param[1][j]);
			}
			for (j = 0; j < 6; j++) {
				IPRINT("REG[%d]      ", reg[j]);
			}
			PRINT("PRED        ", pred);
			PRINT("PARAM_SEL   ", param_sel);
			PRINT("DATA_HI     ", data_hi);
			PRINT("LUT_IDX     ", lut_idx);
			PRINT("CACC        ", cacc);
			PRINT("CMD         ", cmd);
			PRINT("DATA_LO     ", data_lo);
			PRINT("DACC        ", dacc);
			if (pc != epc) {
				printf("expected PC %03x real %03x\n", epc, pc);
			}
			if (of != eof) {
				printf("expected OFS %01x real %01x\n", eof, of);
			}
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int vp2_macro_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset < 0x84 || ctx->chipset > 0xa0 || ctx->chipset == 0x98)
		return HWTEST_RES_NA;
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(vp2_macro,
	HWTEST_TEST(test_isa, 0),
)
