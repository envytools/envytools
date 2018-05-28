/*
 * Copyright (C) 2018 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "pgraph.h"
#include "nva.h"
#include "nvhw/fp.h"
#include "nvhw/xf.h"

namespace hwtest {
namespace pgraph {

class XfBaseTest: public StateTest {
protected:
	int vop, sop;
	int xfctx_user_base;
	uint32_t iws[3];
	uint32_t src[3][4];
	uint32_t res[4];
	int wm;
	bool want_scalar;
	bool is_vp2;
	int version;
	int flags;
	virtual void adjust_orig_xf() = 0;
	void adjust_orig() override {
		uint32_t iws_mask;
		vop = sop = 0;
		if (chipset.card_type == 0x20) {
			xfctx_user_base = 0x60;
			iws_mask = 0x7fc0;
		} else {
			xfctx_user_base = 0x9c;
			iws_mask = 0xffc0;
		}
		uint32_t opa = 0;
		uint32_t opb = 0;
		uint32_t opc = 0;
		uint32_t opd = 0;
		iws[0] = (rnd() & iws_mask) | 0x0001;
		iws[1] = (rnd() & iws_mask) | 0x0005;
		iws[2] = (rnd() & iws_mask) | 0x0003;
		wm = rnd() & 0xf;
		adjust_orig_xf();
		if (chipset.card_type == 0x20) {
			insrt(opa, 0, 1, 1);
			insrt(opa, 3, 8, 0x63);
			insrt(opa, 13, 1, want_scalar);
			insrt(opa, 12, 4, wm);
			insrt(opa, 28, 4, iws[2]);
			insrt(opb, 0, 11, iws[2] >> 4);
			insrt(opb, 11, 15, iws[1]);
			insrt(opb, 26, 6, iws[0]);
			insrt(opc, 0, 9, iws[0] >> 6);
			insrt(opc, 13, 8, 0x62);
			insrt(opc, 21, 4, vop);
			insrt(opc, 25, 3, sop);
			orig.xfpr[0][0] = 0x0f000000;
			orig.xfpr[0][1] = 0x0c000000;
			orig.xfpr[0][2] = 0x002c001b;
			orig.xfpr[1][0] = 0x0f100000;
			orig.xfpr[1][1] = 0x0c000000;
			orig.xfpr[1][2] = 0x002c201b;
		} else {
			insrt(opa, 0, 1, 1);
			insrt(opa, 2, 9, 0x9c+3);
			if (!want_scalar) {
				insrt(opa, 12, 4, wm);
			} else {
				insrt(opa, 16, 4, wm);
			}
			insrt(opa, 28, 4, iws[2]);
			insrt(opb, 0, 11, iws[2] >> 4);
			insrt(opb, 11, 15, iws[1]);
			insrt(opb, 26, 6, iws[0]);
			insrt(opc, 0, 9, iws[0] >> 6);
			insrt(opc, 14, 9, 0x9c + 2);
			insrt(opc, 23, 5, vop);
			insrt(opc, 28, 4, sop);
			insrt(opd, 0, 1, sop >> 4);
			insrt(opd, 21, 1, iws[0] >> 15);
			insrt(opd, 22, 1, iws[1] >> 15);
			insrt(opd, 23, 1, iws[2] >> 15);
			orig.xfpr[0][0] = 0x00f00000;
			orig.xfpr[0][1] = 0x0c000000;
			orig.xfpr[0][2] = 0x00a7001b;
			orig.xfpr[0][3] = 0x00000000;
			orig.xfpr[1][0] = 0x00f00000;
			orig.xfpr[1][1] = 0x0c000000;
			orig.xfpr[1][2] = 0x00a7401b;
			orig.xfpr[1][3] = 0x00010000;
		}
		orig.xfpr[2][0] = opa;
		orig.xfpr[2][1] = opb;
		orig.xfpr[2][2] = opc;
		orig.xfpr[2][3] = opd;
		insrt(orig.xf_mode_a, 8, 8, 0);

		/* Have some fun. */
		for (int i = xfctx_user_base; i < xfctx_user_base + 3; i++)
			for (int j = 0; j < 4; j++) {
				switch (rnd() & 0xf) {
					case 0x0:
						orig.xfctx[i][j] &= 0x807fffff;
						break;
					case 0x1:
						orig.xfctx[i][j] &= ~0x7fffff;
						break;
					case 0x2:
						orig.xfctx[i][j] &= 0x80000000;
						orig.xfctx[i][j] |= 0x7f800000;
						break;
					case 0x3:
						orig.xfctx[i][j] |= 0x7f800000;
						break;
				}
			}
		if (!(rnd() & 3))
			insrt(orig.ctx_switch_a, 0, 8, 0x97);
		if (rnd() & 1)
			orig.xf_timeout &= 0xff;

		/* Ensure clear path. */
		insrt(orig.idx_state_a, 20, 4, 0);
		orig.fd_state_unk18 = 0;
		orig.fd_state_unk20 = 0;
		orig.fd_state_unk30 = 0;
		orig.idx_state_a = 0;
		orig.idx_state_b = 0;
		orig.debug_d &= 0xefffffff;
	}
	virtual void mutate_xf() = 0;
	void mutate() override {
		if (chipset.card_type == 0x20) {
			nva_wr32(cnum, 0x400f50, 0x16000);
			is_vp2 = false;
			version = 2;
			flags = FP_RZ | FP_FTZ | FP_ZERO_WINS | FP_MUL_POS_ZERO;
		} else {
			nva_wr32(cnum, 0x400f50, 0x2c000);
			int mode = extr(orig.xf_mode_b, 30, 2);
			is_vp2 = (mode == 1 || mode == 3) && !nv04_pgraph_is_kelvin_class(&orig);
			flags = FP_RZO | FP_FTZ | FP_MUL_POS_ZERO;
			if (!extr(orig.xf_mode_b, 17, 1))
				flags |= FP_ZERO_WINS;
			version = 3;
		}
		nva_wr32(cnum, 0x400f54, 0);
		pgraph_xf_cmd(&exp, 6, 0, 1, 0, 0);
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 4; j++) {
				src[i][j] = orig.xfctx[xfctx_user_base+i][extr(iws[i], 12 - j * 2, 2)];
				if (extr(iws[i], 15, 1) && is_vp2)
					src[i][j] &= ~0x80000000;
				if (extr(iws[i], 14, 1))
					src[i][j] ^= 0x80000000;
			}
		mutate_xf();
		if (chipset.card_type == 0x30 && orig.xf_timeout < 3)
			return;
		for (int i = 0; i < 4; i++)
			if (wm & 1 << (3 - i))
				exp.xfctx[xfctx_user_base+3][i] = res[i];
	}
	virtual void print_fail() override {
		printf("OP %02x %02x\n", vop, sop);
		printf("SRC0 %04x %08x %08x %08x %08x\n", iws[0], src[0][0], src[0][1], src[0][2], src[0][3]);
		printf("SRC1 %04x %08x %08x %08x %08x\n", iws[1], src[1][0], src[1][1], src[1][2], src[1][3]);
		printf("SRC2 %04x %08x %08x %08x %08x\n", iws[2], src[2][0], src[2][1], src[2][2], src[2][3]);
	}
	using StateTest::StateTest;
};

class XfVecTest : public XfBaseTest {
	void adjust_orig_xf() override {
		if (chipset.card_type == 0x20) {
			vop = rnd() & 0xf;
		} else {
			vop = rnd() & 0x1f;
		}
		want_scalar = false;
	}
	void mutate_xf() override {
		switch (vop) {
			case 0x00:
			default:
				/* NOP */
				wm = 0;
				break;
			case 0x01:
				/* MOV */
				for (int i = 0; i < 4; i++)
					res[i] = src[0][i];
				break;
			case 0x02:
				/* MUL */
				for (int i = 0; i < 4; i++)
					res[i] = fp32_mul(src[0][i], src[1][i], flags);
				break;
			case 0x03:
				/* ADD */
				for (int i = 0; i < 4; i++)
					res[i] = xf_add(src[0][i], src[2][i], version);
				break;
			case 0x04:
				/* MAD */
				for (int i = 0; i < 4; i++)
					res[i] = xf_add(fp32_mul(src[0][i], src[1][i], flags), src[2][i], version);
				break;
			case 0x05:
				/* DP3 */
				for (int i = 0; i < 3; i++)
					res[i] = fp32_mul(src[0][i], src[1][i], flags);
				res[0] = res[1] = res[2] = res[3] = xf_sum3(res, version);
				break;
			case 0x06:
				/* DPH */
				for (int i = 0; i < 3; i++)
					res[i] = fp32_mul(src[0][i], src[1][i], flags);
				res[3] = fp32_mul(FP32_ONE, src[1][3], flags);
				res[0] = res[1] = res[2] = res[3] = xf_sum4(res, version);
				break;
			case 0x07:
				/* DP4 */
				for (int i = 0; i < 4; i++)
					res[i] = fp32_mul(src[0][i], src[1][i], flags);
				res[0] = res[1] = res[2] = res[3] = xf_sum4(res, version);
				break;
			case 0x08:
				/* DST */
				res[0] = FP32_ONE;
				res[1] = fp32_mul(src[0][1], src[1][1], flags);
				res[2] = src[0][2];
				res[3] = src[1][3];
				break;
			case 0x09:
				/* MIN */
				for (int i = 0; i < 4; i++)
					res[i] = xf_minmax(src[0][i], src[1][i], true, flags);
				break;
			case 0x0a:
				/* MAX */
				for (int i = 0; i < 4; i++)
					res[i] = xf_minmax(src[0][i], src[1][i], false, flags);
				break;
			case 0x0b:
				/* SLT */
				for (int i = 0; i < 4; i++)
					res[i] = xf_set(src[0][i], src[1][i], XF_LT, flags);
				break;
			case 0x0c:
				/* SGE */
				for (int i = 0; i < 4; i++)
					res[i] = xf_set(src[0][i], src[1][i], XF_GE, flags);
				break;
			case 0x0e:
				/* FRC */
				if (!is_vp2)
					wm = 0;
				for (int i = 0; i < 4; i++)
					res[i] = xf_frc(src[0][i]);
				break;
			case 0x0f:
				/* FLR */
				if (!is_vp2)
					wm = 0;
				for (int i = 0; i < 4; i++)
					res[i] = xf_flr(src[0][i]);
				break;
			case 0x10:
				/* SEQ */
				if (!is_vp2)
					wm = 0;
				for (int i = 0; i < 4; i++)
					res[i] = xf_set(src[0][i], src[1][i], XF_EQ, flags);
				break;
			case 0x11:
				/* SFL */
				if (!is_vp2)
					wm = 0;
				for (int i = 0; i < 4; i++)
					res[i] = 0;
				break;
			case 0x12:
				/* SGT */
				if (!is_vp2)
					wm = 0;
				for (int i = 0; i < 4; i++)
					res[i] = xf_set(src[0][i], src[1][i], XF_GT, flags);
				break;
			case 0x13:
				/* SLE */
				if (!is_vp2)
					wm = 0;
				for (int i = 0; i < 4; i++)
					res[i] = xf_set(src[0][i], src[1][i], XF_LE, flags);
				break;
			case 0x14:
				/* SNE */
				if (!is_vp2)
					wm = 0;
				for (int i = 0; i < 4; i++)
					res[i] = xf_set(src[0][i], src[1][i], XF_NE, flags);
				break;
			case 0x15:
				/* STR */
				if (!is_vp2)
					wm = 0;
				for (int i = 0; i < 4; i++)
					res[i] = FP32_ONE;
				break;
			case 0x16:
				/* SSG */
				if (!is_vp2)
					wm = 0;
				for (int i = 0; i < 4; i++)
					res[i] = xf_ssg(src[0][i], flags);
				break;
		}
	}
public:
	using XfBaseTest::XfBaseTest;
};

class XfScaTest : public XfBaseTest {
protected:
	void adjust_orig_xf() override {
		if (chipset.card_type == 0x20 || rnd() & 1) {
			sop = rnd() & 7;
		} else {
			// XXX
			sop = 0xd + rnd() % 2;
		}
		want_scalar = true;
	}
	void mutate_xf() override {
		switch (sop) {
			case 0x00:
			default:
				/* NOP */
				wm = 0;
				break;
			case 0x01:
				/* MOV */
				for (int i = 0; i < 4; i++)
					res[i] = src[2][i];
				break;
			case 0x02:
				/* RCP */
				res[0] = xf_rcp(src[2][0], false, true);
				for (int i = 0; i < 4; i++)
					res[i] = res[0];
				break;
			case 0x03:
				/* RCC */
				res[0] = xf_rcp(src[2][0], true, true);
				for (int i = 0; i < 4; i++)
					res[i] = res[0];
				break;
			case 0x04:
				/* RSQ */
				res[0] = xf_rsq(src[2][0], version, !!(flags & FP_ZERO_WINS));
				for (int i = 0; i < 4; i++)
					res[i] = res[0];
				break;
			case 0x05:
				/* EXP */
				res[0] = xf_exp_flr(src[2][0]);
				res[1] = xf_exp_frc(src[2][0]);
				res[2] = xf_exp(src[2][0]);
				res[3] = FP32_ONE;
				break;
			case 0x06:
				/* LOG */
				res[0] = xf_log_e(src[2][0], version, flags);
				res[1] = xf_log_f(src[2][0], version, flags);
				res[2] = xf_log(src[2][0], version, flags);
				res[3] = FP32_ONE;
				break;
			case 0x07:
				xf_lit(res, src[2]);
				break;
			case 0x0d:
				/* LG2 */
				if (!is_vp2)
					wm = 0;
				res[0] = xf_lg2(src[2][0]);
				for (int i = 0; i < 4; i++)
					res[i] = res[0];
				break;
			case 0x0e:
				/* EX2 */
				if (!is_vp2)
					wm = 0;
				res[0] = xf_ex2(src[2][0]);
				for (int i = 0; i < 4; i++)
					res[i] = res[0];
				break;
		}
	}
public:
	using XfBaseTest::XfBaseTest;
};

bool XfTests::supported() {
	return chipset.card_type == 0x20 || chipset.card_type == 0x30;
}

Test::Subtests XfTests::subtests() {
	return {
		{"vec", new XfVecTest(opt, rnd())},
		{"sca", new XfScaTest(opt, rnd())},
	};
}

}
}
