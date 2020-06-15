/*
 * Copyright (C) 2012 Marcelina Kościelnicka <mwk@0x04.net>
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

#include "dis-intern.h"

/*
 * Immediate fields
 */

static struct rbitfield abtargoff = { 0, 16, .shr = 2 };
static struct rbitfield btargoff = { { 9, 15 }, BF_SIGNED, .shr = 2, .pcrel = 1 };
static struct rbitfield imm16off = { 0, 16 };
static struct rbitfield immoff = { { 3, 11 }, BF_SIGNED };
static struct rbitfield uimmoff = { 3, 11 };
static struct rbitfield immloff = { { 0, 19 }, BF_SIGNED };
static struct rbitfield immhoff = { { 0, 16 }, .shr = 16 };
static struct bitfield xdimmoff = { { 0, 13 } };
static struct bitfield compoff = { 3, 2 };
static struct bitfield bitopoff = { 3, 4 };
static struct bitfield cmpopoff = { 19, 4 };
static struct bitfield baruoff = { 3, 2 };
static struct bitfield barwoff = { 20, 2 };
static struct bitfield rbimmoff = { 3, 8 };
static struct bitfield rbimmmuloff = { { 9, 5, 0, 1 }, .shr=2 };
static struct bitfield rbimmbadoff = { 0, 8 };
static struct bitfield shiftoff = { { 5, 3 }, BF_SIGNED };
static struct bitfield altshiftoff = { { 11, 3 }, BF_SIGNED };
static struct bitfield vcxfrmoff = { 22, 2, 0, 1 };
static struct bitfield factor1off = { { 1, 9 }, BF_SIGNED };
static struct bitfield factor2off = { { 10, 9 }, BF_SIGNED };
#define ABTARG atombtarg, &abtargoff
#define BTARG atombtarg, &btargoff
#define CTARG atomctarg, &btargoff
#define IMM16 atomrimm, &imm16off
#define IMM atomrimm, &immoff
#define UIMM atomrimm, &uimmoff
#define IMML atomrimm, &immloff
#define IMMH atomrimm, &immhoff
#define XDIMM atomimm, &xdimmoff
#define COMP atomimm, &compoff
#define BITOP atomimm, &bitopoff
#define CMPOP atomimm, &cmpopoff
#define BARU atomimm, &baruoff
#define BARW atomimm, &barwoff
#define BIMM atomimm, &rbimmoff
#define BIMMMUL atomimm, &rbimmmuloff
#define BIMMBAD atomimm, &rbimmbadoff
#define SHIFT atomimm, &shiftoff
#define ALTSHIFT atomimm, &altshiftoff
#define VCXFRM atomimm, &vcxfrmoff
#define FACTOR1 atomimm, &factor1off
#define FACTOR2 atomimm, &factor2off

/*
 * Register fields
 */

static struct sreg sreg_sr[] = {
	{ 30, "tick" },
	{ 31, "csreq" },
	{ -1 },
};

static struct sreg sreg_mi[] = {
	{ -1 },
};

static struct sreg sreg_uc[] = {
	{ 16, "uccfg" },
	{ -1 },
};

static struct sreg sreg_l[] = {
	{ 0, "l0" },
	{ 1, "l1" },
	{ 2, "l2" },
	{ 3, "l3" },
	{ -1 },
};
static struct sreg rreg_sr[] = {
	{ 31, 0, SR_ZERO },
	{ -1 },
};
static struct bitfield dst_bf = { 19, 5 };
static struct bitfield xdst_bf = { 19, 4 };
static struct bitfield ddst_bf = { 19, 3 };
static struct bitfield cdst_bf = { 19, 2 };
static struct bitfield fdst_bf = { 19, 1 };
static struct bitfield src1_bf = { 14, 5 };
static struct bitfield mdst_bf = { 19, 5, 3, 1 };
static struct bitfield xsrc1_bf = { 14, 4 };
static struct bitfield dsrc1_bf = { 14, 3 };
static struct bitfield csrc1_bf = { 14, 2 };
static struct bitfield fsrc1_bf = { 14, 1 };
static struct bitfield msrc1_bf = { 14, 5, 3, 1 };
static struct bitfield src2_bf = { 9, 5 };
static struct bitfield src3_bf = { 4, 5 };
static struct bitfield csrcp_bf = { 3, 2 };
static struct bitfield cdstp_bf = { 0, 2 };
static struct bitfield mcdst_bf = { 0, 3 };
static struct bitfield ignall_bf = { 0, 24 };
static struct reg adst_r = { &dst_bf, "a" };
static struct reg rdst_r = { &dst_bf, "r", .specials = rreg_sr };
static struct reg vdst_r = { &dst_bf, "v" };
static struct reg vdstq_r = { &dst_bf, "v", "q" };
static struct reg ddst_r = { &ddst_bf, "d" };
static struct reg cdst_r = { &cdst_bf, "c" };
static struct reg ldst_r = { &cdst_bf, "l" };
static struct reg xdst_r = { &xdst_bf, "x" };
static struct reg mdst_r = { &mdst_bf, "m" };
static struct reg fdst_r = { &fdst_bf, "f" };
static struct reg srdst_r = { &dst_bf, "sr", .specials = sreg_sr, .always_special = 1 };
static struct reg midst_r = { &dst_bf, "mi", .specials = sreg_mi, .always_special = 1 };
static struct reg ucdst_r = { &dst_bf, "uc", .specials = sreg_uc, .always_special = 1 };
static struct reg sldst_r = { &dst_bf, "l", .specials = sreg_l, .always_special = 1 };
static struct reg adstd_r = { &dst_bf, "a", "d" };
static struct reg asrc1_r = { &src1_bf, "a" };
static struct reg asrc2_r = { &src2_bf, "a" };
static struct reg rsrc2_r = { &src2_bf, "r", .specials = rreg_sr };
static struct reg vsrc2_r = { &src2_bf, "v" };
static struct reg vsrc3_r = { &src3_bf, "v" };
static struct reg asrc1d_r = { &src1_bf, "a", "d" };
static struct reg asrc2d_r = { &src2_bf, "a", "d" };
static struct reg rsrc2d_r = { &src2_bf, "r", "d" };
static struct reg vsrc2d_r = { &src2_bf, "v", "d" };
static struct reg asrc2q_r = { &src2_bf, "a", "q" };
static struct reg rsrc2q_r = { &src2_bf, "r", "q" };
static struct reg vsrc2q_r = { &src2_bf, "v", "q" };
static struct reg rsrc1_r = { &src1_bf, "r", .specials = rreg_sr };
static struct reg vsrc1_r = { &src1_bf, "v" };
static struct reg vsrc1d_r = { &src1_bf, "v", "d" };
static struct reg vsrc1q_r = { &src1_bf, "v", "q" };
static struct reg csrc1_r = { &csrc1_bf, "c" };
static struct reg dsrc1_r = { &dsrc1_bf, "d" };
static struct reg xsrc1_r = { &xsrc1_bf, "x" };
static struct reg msrc1_r = { &msrc1_bf, "m" };
static struct reg fsrc1_r = { &fsrc1_bf, "f" };
static struct reg srsrc1_r = { &src1_bf, "sr", .specials = sreg_sr, .always_special = 1 };
static struct reg misrc1_r = { &src1_bf, "mi", .specials = sreg_mi, .always_special = 1 };
static struct reg ucsrc1_r = { &src1_bf, "uc", .specials = sreg_uc, .always_special = 1 };
static struct reg slsrc1_r = { &src1_bf, "l", .specials = sreg_l, .always_special = 1 };
static struct reg csrcp_r = { &csrcp_bf, "c" };
static struct reg lsrcl_r = { &csrcp_bf, "l" };
static struct reg cdstp_r = { &cdstp_bf, "c" };
static struct reg ldstl_r = { &cdstp_bf, "l" };
static struct reg vcdst_r = { &cdstp_bf, "vc" };
static struct reg vc_r = { 0, "vc" };
static struct reg vcidx_r = { &cdst_bf, "vc" };

#define ADST atomreg, &adst_r
#define RDST atomreg, &rdst_r
#define VDST atomreg, &vdst_r
#define VDSTQ atomreg, &vdstq_r
#define DDST atomreg, &ddst_r
#define CDST atomreg, &cdst_r
#define LDST atomreg, &ldst_r
#define XDST atomreg, &xdst_r
#define MDST atomreg, &mdst_r
#define FDST atomreg, &fdst_r
#define SRDST atomreg, &srdst_r
#define MIDST atomreg, &midst_r
#define UCDST atomreg, &ucdst_r
#define SLDST atomreg, &sldst_r
#define ADSTD atomreg, &adstd_r
#define ASRC1 atomreg, &asrc1_r
#define RSRC1 atomreg, &rsrc1_r
#define VSRC1 atomreg, &vsrc1_r
#define VSRC1D atomreg, &vsrc1d_r
#define VSRC1Q atomreg, &vsrc1q_r
#define CSRC1 atomreg, &csrc1_r
#define DSRC1 atomreg, &dsrc1_r
#define XSRC1 atomreg, &xsrc1_r
#define MSRC1 atomreg, &msrc1_r
#define FSRC1 atomreg, &fsrc1_r
#define SRSRC1 atomreg, &srsrc1_r
#define MISRC1 atomreg, &misrc1_r
#define UCSRC1 atomreg, &ucsrc1_r
#define SLSRC1 atomreg, &slsrc1_r
#define ASRC1D atomreg, &asrc1d_r
#define ASRC2 atomreg, &asrc2_r
#define RSRC2 atomreg, &rsrc2_r
#define VSRC2 atomreg, &vsrc2_r
#define VSRC3 atomreg, &vsrc3_r
#define ASRC2D atomreg, &asrc2d_r
#define RSRC2D atomreg, &rsrc2d_r
#define VSRC2D atomreg, &vsrc2d_r
#define ASRC2Q atomreg, &asrc2q_r
#define RSRC2Q atomreg, &rsrc2q_r
#define VSRC2Q atomreg, &vsrc2q_r
#define CSRCP atomreg, &csrcp_r
#define CDSTP atomreg, &cdstp_r
#define VCDST atomreg, &vcdst_r
#define VC atomreg, &vc_r
#define LSRCL atomreg, &lsrcl_r
#define LDSTL atomreg, &ldstl_r
#define VCIDX atomreg, &vcidx_r
#define IGNCE atomign, &cdstp_bf
#define IGNCD atomign, &mcdst_bf
#define IGNALL atomign, &ignall_bf
#define IGNDST atomign, &dst_bf
#define IGNSRC1 atomign, &src1_bf

F1(intr, 16, N("intr"))

F(mcdst, 2, CDSTP, IGNCE);
F(vcdst, 2, VCDST, IGNCE);

F(xdimm, 13, XDIMM, );

F(barldsti, 19, N("st"), N("ld"));
F(barldstr, 0, N("st"), N("ld"));

F(s2vmode, 0, N("factor"), N("mask"))
F(signop, 28, N("s"), N("u"))
F(sign1, 2, N("u"), N("s"))
F(sign2, 1, N("u"), N("s"))
F(signs, 9, N("u"), N("s"))
F(signd, 12, N("u"), N("s"))
F1(lrp2x, 10, N("xor"))
F1(vawrite, 11, N("va"))
F(rnd, 8, N("rd"), N("rn"))
F(altrnd, 9, N("rd"), N("rn"))
F(fi, 3, N("fract"), N("int"))
F(hilo, 4, N("hi"), N("lo"))
F(vcflag, 21, N("sf"), N("zf"))
F(vcsel, 2, N("sf"), N("zf"))
F(swz, 3, N("lo"), N("hi"))

static struct insn tabaslctop[] = {
	{ 0x00000000, 0x000001e0, SESTART, N("slct"), CSRCP, N("sf"), ASRC2D, SEEND },
	{ 0x00000020, 0x000001e0, SESTART, N("slct"), CSRCP, N("zf"), ASRC2D, SEEND },
	{ 0x00000040, 0x000001e0, SESTART, N("slct"), CSRCP, N("b19"), ASRC2D, SEEND },
	{ 0x00000060, 0x000001e0, SESTART, N("slct"), CSRCP, N("b20d"), ASRC2D, SEEND },
	{ 0x00000080, 0x000001e0, SESTART, N("slct"), CSRCP, N("b20"), ASRC2Q, SEEND },
	{ 0x000000a0, 0x000001e0, SESTART, N("slct"), CSRCP, N("b21"), ASRC2D, SEEND },
	{ 0x000000c0, 0x000001e0, SESTART, N("slct"), CSRCP, N("b19a"), ASRC2D, SEEND },
	{ 0x000000e0, 0x000001e0, SESTART, N("slct"), CSRCP, N("b18"), ASRC2D, SEEND },
	{ 0x00000100, 0x000001e0, SESTART, N("slct"), CSRCP, N("asf"), ASRC2D, SEEND },
	{ 0x00000120, 0x000001e0, SESTART, N("slct"), CSRCP, N("azf"), ASRC2D, SEEND },
	{ 0x00000140, 0x000001e0, SESTART, N("slct"), CSRCP, N("aef"), ASRC2D, SEEND },
	{ 0x00000160, 0x000001e0, SESTART, N("slct"), CSRCP, U("11"), ASRC2D, SEEND },
	{ 0x00000180, 0x000001e0, SESTART, N("slct"), CSRCP, U("12"), ASRC2D, SEEND },
	{ 0x000001a0, 0x000001e0, SESTART, N("slct"), CSRCP, N("lzf"), ASRC2D, SEEND },
	{ 0x000001c0, 0x000001e0, ASRC2 },
	{ 0x000001e0, 0x000001e0, SESTART, N("slct"), CSRCP, N("true"), ASRC2D, SEEND },
	{ 0, 0, OOPS }
};

static struct insn tabslctop[] = {
	{ 0x00000000, 0x000001e0, SESTART, N("slct"), CSRCP, N("sf"), RSRC2D, SEEND },
	{ 0x00000020, 0x000001e0, SESTART, N("slct"), CSRCP, N("zf"), RSRC2D, SEEND },
	{ 0x00000040, 0x000001e0, SESTART, N("slct"), CSRCP, N("b19"), RSRC2D, SEEND },
	{ 0x00000060, 0x000001e0, SESTART, N("slct"), CSRCP, N("b20d"), RSRC2D, SEEND },
	{ 0x00000080, 0x000001e0, SESTART, N("slct"), CSRCP, N("b20"), RSRC2Q, SEEND },
	{ 0x000000a0, 0x000001e0, SESTART, N("slct"), CSRCP, N("b21"), RSRC2D, SEEND },
	{ 0x000000c0, 0x000001e0, SESTART, N("slct"), CSRCP, N("b19a"), RSRC2D, SEEND },
	{ 0x000000e0, 0x000001e0, SESTART, N("slct"), CSRCP, N("b18"), RSRC2D, SEEND },
	{ 0x00000100, 0x000001e0, SESTART, N("slct"), CSRCP, N("asf"), RSRC2D, SEEND },
	{ 0x00000120, 0x000001e0, SESTART, N("slct"), CSRCP, N("azf"), RSRC2D, SEEND },
	{ 0x00000140, 0x000001e0, SESTART, N("slct"), CSRCP, N("aef"), RSRC2D, SEEND },
	{ 0x00000160, 0x000001e0, SESTART, N("slct"), CSRCP, U("11"), RSRC2D, SEEND },
	{ 0x00000180, 0x000001e0, SESTART, N("slct"), CSRCP, U("12"), RSRC2D, SEEND },
	{ 0x000001a0, 0x000001e0, SESTART, N("slct"), CSRCP, N("lzf"), RSRC2D, SEEND },
	{ 0x000001c0, 0x000001e0, RSRC2 },
	{ 0x000001e0, 0x000001e0, SESTART, N("slct"), CSRCP, N("true"), RSRC2D, SEEND },
	{ 0, 0, OOPS }
};

static struct insn tabvslctop[] = {
	{ 0x00000000, 0x000001e0, SESTART, N("slct"), CSRCP, N("sf"), VSRC2D, SEEND },
	{ 0x00000020, 0x000001e0, SESTART, N("slct"), CSRCP, N("zf"), VSRC2D, SEEND },
	{ 0x00000040, 0x000001e0, SESTART, N("slct"), CSRCP, N("b19"), VSRC2D, SEEND },
	{ 0x00000060, 0x000001e0, SESTART, N("slct"), CSRCP, N("b20d"), VSRC2D, SEEND },
	{ 0x00000080, 0x000001e0, SESTART, N("slct"), CSRCP, N("b20"), VSRC2Q, SEEND },
	{ 0x000000a0, 0x000001e0, SESTART, N("slct"), CSRCP, N("b21"), VSRC2D, SEEND },
	{ 0x000000c0, 0x000001e0, SESTART, N("slct"), CSRCP, N("b19a"), VSRC2D, SEEND },
	{ 0x000000e0, 0x000001e0, SESTART, N("slct"), CSRCP, N("b18"), VSRC2D, SEEND },
	{ 0x00000100, 0x000001e0, SESTART, N("slct"), CSRCP, N("asf"), VSRC2D, SEEND },
	{ 0x00000120, 0x000001e0, SESTART, N("slct"), CSRCP, N("azf"), VSRC2D, SEEND },
	{ 0x00000140, 0x000001e0, SESTART, N("slct"), CSRCP, N("aef"), VSRC2D, SEEND },
	{ 0x00000160, 0x000001e0, SESTART, N("slct"), CSRCP, U("11"), VSRC2D, SEEND },
	{ 0x00000180, 0x000001e0, SESTART, N("slct"), CSRCP, U("12"), VSRC2D, SEEND },
	{ 0x000001a0, 0x000001e0, SESTART, N("slct"), CSRCP, N("lzf"), VSRC2D, SEEND },
	{ 0x000001c0, 0x000001e0, VSRC2 },
	{ 0x000001e0, 0x000001e0, SESTART, N("slct"), CSRCP, N("true"), VSRC2D, SEEND },
	{ 0, 0, OOPS }
};

static struct insn tabpred[] = {
	{ 0x00000000, 0x000001e0, CSRCP, N("sf") },
	{ 0x00000020, 0x000001e0, CSRCP, N("zf") },
	{ 0x00000040, 0x000001e0, CSRCP, N("b19") },
	{ 0x00000060, 0x000001e0, CSRCP, N("b20d") },
	{ 0x00000080, 0x000001e0, CSRCP, N("b20") },
	{ 0x000000a0, 0x000001e0, CSRCP, N("b21") },
	{ 0x000000c0, 0x000001e0, CSRCP, N("b19a") },
	{ 0x000000e0, 0x000001e0, CSRCP, N("b18") },
	{ 0x00000100, 0x000001e0, CSRCP, N("asf") },
	{ 0x00000120, 0x000001e0, CSRCP, N("azf") },
	{ 0x00000140, 0x000001e0, CSRCP, N("aef") },
	{ 0x00000160, 0x000001e0, CSRCP, U("11") },
	{ 0x00000180, 0x000001e0, CSRCP, U("12") },
	{ 0x000001a0, 0x000001e0, CSRCP, N("lzf") },
	{ 0x000001c0, 0x000001e0, CSRCP, N("false") },
	{ 0x000001e0, 0x000001e0, CSRCP, N("true") },
	{ 0, 0, OOPS }
};

static struct insn tabm[] = {
	{ 0x01000000, 0xef000000, N("bmul"), T(rnd), T(signop), RDST, T(sign1), RSRC1, T(sign2), RSRC2 },
	{ 0x02000000, 0xef000000, N("bmula"), T(rnd), T(signop), RDST, T(sign1), RSRC1, T(sign2), RSRC2 },
	{ 0x04000000, 0xff000000, N("bvecmad"), RSRC1, RSRC2Q, T(pred), VCIDX, T(vcflag), VCXFRM },
	{ 0x05000000, 0xff000000, N("bvecmadsel"), RSRC1, RSRC2Q, T(pred), VCIDX, T(vcflag), VCXFRM },
	{ 0x08000000, 0xef000000, N("bmin"), T(signop), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x09000000, 0xef000000, N("bmax"), T(signop), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x0a000000, 0xef000000, N("babs"), T(signop), RDST, T(mcdst), RSRC1 },
	{ 0x0b000000, 0xef000000, N("bneg"), T(signop), RDST, T(mcdst), RSRC1 },
	{ 0x0c000000, 0xef000000, N("badd"), T(signop), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x0d000000, 0xef000000, N("bsub"), T(signop), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x0e000000, 0xef000000, N("bshr"), T(signop), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x0f000000, 0xff000000, N("bvec"), RSRC1, VCIDX, T(vcflag), VCXFRM },
	{ 0x21000000, 0xef000000, N("bmul"), T(rnd), T(signop), RDST, T(sign1), RSRC1, T(sign2), BIMMMUL },
	{ 0x22000000, 0xef000000, N("bmula"), T(rnd), T(signop), RDST, T(sign1), RSRC1, T(sign2), BIMMBAD },
	{ 0x24000000, 0xff000000, N("vec"), FACTOR1, FACTOR2, VCIDX, T(vcflag), VCXFRM },
	{ 0x25000000, 0xff000000, N("band"), RDST, RSRC1, BIMM },
	{ 0x26000000, 0xff000000, N("bor"), RDST, RSRC1, BIMM },
	{ 0x27000000, 0xff000000, N("bxor"), RDST, RSRC1, BIMM },
	{ 0x28000000, 0xef000000, N("bmin"), T(signop), RDST, T(mcdst), RSRC1, BIMM },
	{ 0x29000000, 0xef000000, N("bmax"), T(signop), RDST, T(mcdst), RSRC1, BIMM },
	{ 0x2a000000, 0xef000000, N("babs"), T(signop), RDST, T(mcdst), RSRC1 },
	{ 0x2b000000, 0xef000000, N("bneg"), T(signop), RDST, T(mcdst), RSRC1 },
	{ 0x2c000000, 0xef000000, N("badd"), T(signop), RDST, T(mcdst), RSRC1, BIMM },
	{ 0x2d000000, 0xef000000, N("bsub"), T(signop), RDST, T(mcdst), RSRC1, BIMM },
	{ 0x2e000000, 0xef000000, N("bshr"), T(signop), RDST, T(mcdst), RSRC1, BIMM },
	{ 0x41000000, 0xef000000, N("mul"), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x42000008, 0xff000078, N("nor"), RDST, T(mcdst), RSRC1, RSRC2 },
	{ 0x42000010, 0xff000078, N("and"), RDST, T(mcdst), N("not"), RSRC1, RSRC2 },
	{ 0x42000020, 0xff000078, N("and"), RDST, T(mcdst), RSRC1, N("not"), RSRC2 },
	{ 0x42000030, 0xff000078, N("xor"), RDST, T(mcdst), RSRC1, RSRC2 },
	{ 0x42000038, 0xff000078, N("nand"), RDST, T(mcdst), RSRC1, RSRC2 },
	{ 0x42000040, 0xff000078, N("and"), RDST, T(mcdst), RSRC1, RSRC2 },
	{ 0x42000048, 0xff000078, N("nxor"), RDST, T(mcdst), RSRC1, RSRC2 },
	{ 0x42000058, 0xff000078, N("or"), RDST, T(mcdst), N("not"), RSRC1, RSRC2 },
	{ 0x42000068, 0xff000078, N("or"), RDST, T(mcdst), RSRC1, N("not"), RSRC2 },
	{ 0x42000070, 0xff000078, N("or"), RDST, T(mcdst), RSRC1, RSRC2 },
	{ 0x42000000, 0xff000000, N("bitop"), BITOP, RDST, T(mcdst), RSRC1, RSRC2 },
	{ 0x45000000, 0xff000000, N("vecms"), RSRC1, VCIDX, T(vcflag), VCXFRM },
	{ 0x48000000, 0xef000000, N("min"), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x49000000, 0xef000000, N("max"), RDST, T(mcdst), RSRC1, T(slctop) },
	/* also 7a, but who can know what the official opcode is... */
	{ 0x4a000000, 0xef000000, N("abs"), RDST, T(mcdst), RSRC1 },
	{ 0x4b000000, 0xef000000, N("neg"), RDST, T(mcdst), RSRC1 },
	{ 0x4c000000, 0xef000000, N("add"), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x4d000000, 0xef000000, N("sub"), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x4e000000, 0xff000000, N("sar"), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x4f000000, 0xff000000, N("snop"), IGNALL },
	{ 0x5e000000, 0xff000000, N("shr"), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x61000000, 0xef000000, N("mul"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x62000000, 0xff000000, N("and"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x63000000, 0xff000000, N("xor"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x64000000, 0xff000000, N("or"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x65000000, 0xff000000, N("mov"), RDST, IMML },
	{ 0x68000000, 0xef000000, N("min"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x69000000, 0xef000000, N("max"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x6a000000, 0xff0000e0, N("mov"), VDST, COMP, RSRC1, IGNCD },
	{ 0x6a000040, 0xff0000f8, N("mov"), SRDST, RSRC1, IGNCD },
	{ 0x6a000048, 0xff0000f8, N("mov"), MIDST, RSRC1, IGNCD },
	{ 0x6a000050, 0xff0000f8, N("mov"), UCDST, RSRC1, IGNCD },
	{ 0x6a000058, 0xff0000f8, N("mov"), SLDST, RSRC1, IGNCD },
	{ 0x6a000060, 0xff0000f8, N("mov"), ADST, RSRC1, IGNCD },
	{ 0x6a0000a0, 0xff0000f0, N("mov"), MDST, RSRC1, IGNCD },
	{ 0x6a0000b0, 0xff0000f8, N("mov"), DDST, RSRC1, IGNCD },
	{ 0x6a0000b8, 0xff0000f8, N("mov"), FDST, RSRC1, IGNCD },
	{ 0x6a0000c0, 0xff0000f8, N("mov"), XDST, RSRC1, IGNCD },
	{ 0x6b000000, 0xff0000e0, N("mov"), RDST, VSRC1, COMP, IGNCD },
	{ 0x6b000040, 0xff0000f8, N("mov"), RDST, SRSRC1, IGNCD },
	{ 0x6b000048, 0xff0000f8, N("mov"), RDST, MISRC1, IGNCD },
	{ 0x6b000050, 0xff0000f8, N("mov"), RDST, UCSRC1, IGNCD },
	{ 0x6b000058, 0xff0000f8, N("mov"), RDST, SLSRC1, IGNCD },
	{ 0x6b000060, 0xff0000f8, N("mov"), RDST, ASRC1, IGNCD },
	{ 0x6b000068, 0xff0000f8, N("mov"), RDST, CSRC1, IGNCD },
	{ 0x6b0000a0, 0xff0000f0, N("mov"), RDST, MSRC1, IGNCD },
	{ 0x6b0000b0, 0xff0000f8, N("mov"), RDST, DSRC1, IGNCD },
	{ 0x6b0000b8, 0xff0000f8, N("mov"), RDST, FSRC1, IGNCD },
	{ 0x6b0000c0, 0xff0000f8, N("mov"), RDST, XSRC1, IGNCD },
	{ 0x6c000000, 0xef000000, N("add"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x6d000000, 0xef000000, N("sub"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x6e000000, 0xff000000, N("sar"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x75000000, 0xff000000, N("sethi"), RDST, IMMH },
	{ 0x7a000000, 0xff000000, N("abs"), RDST, T(mcdst), RSRC1 },
	{ 0x7b000000, 0xff000000, N("neg"), RDST, T(mcdst), RSRC1 },
	{ 0x7e000000, 0xff000000, N("shr"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x80000000, 0xff000000, N("vmul"), T(signop), T(rnd), T(fi), SHIFT, T(hilo), DISCARD, T(sign1), VSRC1, T(sign2), VSRC2 },
	{ 0xa0000000, 0xff000000, N("vmul"), T(signop), T(rnd), T(fi), SHIFT, T(hilo), DISCARD, T(sign1), VSRC1, T(sign2), BIMMMUL },
	{ 0xb0000000, 0xff000000, N("vmul"), T(signop), T(rnd), T(fi), SHIFT, T(hilo), DISCARD, T(sign1), VSRC1, T(sign2), BIMMBAD },
	{ 0x81000000, 0xef000000, N("vmul"), T(signop), T(rnd), T(fi), SHIFT, T(hilo), VDST, T(sign1), VSRC1, T(sign2), VSRC2 },
	{ 0xa1000000, 0xef000000, N("vmul"), T(signop), T(rnd), T(fi), SHIFT, T(hilo), VDST, T(sign1), VSRC1, T(sign2), BIMMMUL },
	{ 0x82000000, 0xef000000, N("vmac"), T(signop), T(rnd), T(fi), SHIFT, T(hilo), VDST, T(sign1), VSRC1, T(sign2), VSRC2 },
	{ 0xa2000000, 0xef000000, N("vmac"), T(signop), T(rnd), T(fi), SHIFT, T(hilo), VDST, T(sign1), VSRC1, T(sign2), BIMMMUL },
	{ 0x83000000, 0xef000000, N("vmac"), T(signop), T(rnd), T(fi), SHIFT, T(hilo), DISCARD, T(sign1), VSRC1, T(sign2), VSRC2 },
	{ 0xa3000000, 0xff000000, N("vmac"), T(signop), T(rnd), T(fi), SHIFT, T(hilo), DISCARD, T(sign1), VSRC1, T(sign2), BIMMMUL },
	{ 0x84000000, 0xff000000, N("vmad2"), T(signop), T(s2vmode), T(rnd), T(fi), SHIFT, T(hilo), DISCARD, T(sign1), VSRC1D, T(sign2), VSRC2 },
	{ 0x85000000, 0xef000000, N("vmad2"), T(signop), T(s2vmode), T(rnd), T(fi), SHIFT, T(hilo), VDST, T(sign1), VSRC1D, T(sign2), VSRC2 },
	{ 0x86000000, 0xff000000, N("vmac2"), T(signop), T(s2vmode), T(rnd), T(fi), SHIFT, T(hilo), DISCARD, T(sign1), VSRC1D },
	{ 0x96000000, 0xff000000, N("vmac2"), T(signop), T(s2vmode), T(rnd), T(fi), SHIFT, T(hilo), DISCARD, T(sign1), VSRC1, VSRC3 },
	{ 0xa6000000, 0xff000000, N("vmac2"), T(signop), T(s2vmode), T(rnd), T(fi), SHIFT, T(hilo), DISCARD, T(sign1), VSRC1, VSRC3 },
	{ 0x87000000, 0xef000000, N("vmac2"), T(signop), T(s2vmode), T(rnd), T(fi), SHIFT, T(hilo), VDST, T(sign1), VSRC1D },
	{ 0xa7000000, 0xff000000, N("vmac2"), T(signop), T(s2vmode), T(rnd), T(fi), SHIFT, T(hilo), VDST, T(sign1), VSRC1, VSRC3 },
	{ 0xb3000000, 0xff000000, N("vlrp2"), T(signd), T(vawrite), T(rnd), SHIFT, VDST, T(signs), T(lrp2x), VSRC1Q, CSRCP, VCDST, T(vcsel) },
	{ 0xb4000000, 0xff000000, N("vlrp4a"), T(rnd), SHIFT, DISCARD, VSRC1Q, CSRCP, VCDST, T(vcsel) },
	{ 0xb5000000, 0xff000000, N("vlrpf"), T(rnd), SHIFT, DISCARD, VSRC1Q, CSRCP, VSRC2, VCDST, T(vcsel) },
	{ 0xb6000000, 0xff000000, N("vlrp4b"), N("u"), T(altrnd), ALTSHIFT, VDST, VSRC1Q, CSRCP, T(pred), VCDST, T(vcsel) },
	{ 0xb7000000, 0xff000000, N("vlrp4b"), N("s"), T(altrnd), ALTSHIFT, VDST, VSRC1Q, CSRCP, T(pred), VCDST, T(vcsel) },
	{ 0x88000000, 0xef000000, N("vmin"), T(signop), VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0x89000000, 0xef000000, N("vmax"), T(signop), VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0x8a000000, 0xef000000, N("vabs"), T(signop), VDST, T(vcdst), VSRC1 },
	{ 0x8b000000, 0xff000000, N("vneg"), T(signop), VDST, T(vcdst), VSRC1 },
	{ 0x8c000000, 0xef000000, N("vadd"), T(signop), VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0x8d000000, 0xef000000, N("vsub"), T(signop), VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0x8e000000, 0xef000000, N("vshr"), T(signop), VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0x8f000000, 0xff000000, N("vcmpad"), CMPOP, T(vcdst), VSRC1D, T(vslctop) },
	{ 0x90000000, 0xff000000, N("vlrp"), T(rnd), SHIFT, VDST, VSRC1D, VSRC2 },
	{ 0x94000008, 0xff000078, N("vnor"), VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0x94000010, 0xff000078, N("vand"), VDST, T(vcdst), N("not"), VSRC1, VSRC2 },
	{ 0x94000020, 0xff000078, N("vand"), VDST, T(vcdst), VSRC1, N("not"), VSRC2 },
	{ 0x94000030, 0xff000078, N("vxor"), VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0x94000038, 0xff000078, N("vnand"), VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0x94000040, 0xff000078, N("vand"), VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0x94000048, 0xff000078, N("vnxor"), VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0x94000058, 0xff000078, N("vor"), VDST, T(vcdst), N("not"), VSRC1, VSRC2 },
	{ 0x94000068, 0xff000078, N("vor"), VDST, T(vcdst), VSRC1, N("not"), VSRC2 },
	{ 0x94000070, 0xff000078, N("vor"), VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0x94000000, 0xff000000, N("vbitop"), BITOP, VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0x9b000000, 0xff000000, N("vswz"), VDST, VSRC1, VSRC2, T(swz), VSRC3 },
	{ 0x9f000000, 0xff000000, N("vadd9"), VDST, T(vcdst), VSRC1, VSRC2, VSRC3 },
	{ 0xa4000000, 0xff000000, N("vclip"), VDST, T(vcdst), VSRC1, VSRC2, VSRC3 },
	{ 0xa5000000, 0xff000000, N("vminabs"), VDST, T(vcdst), VSRC1, VSRC2 },
	{ 0xa8000000, 0xef000000, N("vmin"), T(signop), VDST, T(vcdst), VSRC1, BIMM },
	{ 0xa9000000, 0xef000000, N("vmax"), T(signop), VDST, T(vcdst), VSRC1, BIMM },
	{ 0xac000000, 0xef000000, N("vadd"), T(signop), VDST, T(vcdst), VSRC1, BIMM },
	{ 0xbd000000, 0xff000000, N("vsub"), T(signop), VDST, T(vcdst), VSRC1, BIMM },
	{ 0xae000000, 0xef000000, N("vshr"), T(signop), VDST, T(vcdst), VSRC1, BIMM },
	{ 0xaa000000, 0xff000000, N("vand"), VDST, T(vcdst), VSRC1, BIMM },
	{ 0xab000000, 0xff000000, N("vxor"), VDST, T(vcdst), VSRC1, BIMM },
	{ 0xaf000000, 0xff000000, N("vor"), VDST, T(vcdst), VSRC1, BIMM },
	{ 0xba000000, 0xff000000, N("mov"), VDST, T(vcdst), VSRC1 },
	{ 0xbb000000, 0xff000000, N("mov"), VDST, VC },
	{ 0xad000000, 0xff000000, N("vmov"), VDST, T(vcdst), BIMM },
	{ 0xbf000000, 0xff000000, N("vnop"), IGNALL },
	{ 0xc0000000, 0xff000000, N("ldavh"), VDST, T(mcdst), ASRC1, T(aslctop) },
	{ 0xc1000000, 0xff000000, N("ldavv"), VDST, T(mcdst), ASRC1, T(aslctop) },
	{ 0xc2000000, 0xff000000, N("ldas"), RDST, T(mcdst), ASRC1, T(aslctop) },
	{ 0xc3000000, 0xff000000, N("xdld"), ADST, ASRC1D, T(xdimm) },
	{ 0xc4000000, 0xff000000, N("stavh"), VSRC1, T(mcdst), ADST, T(aslctop) },
	{ 0xc5000000, 0xff000000, N("stavv"), VSRC1, T(mcdst), ADST, T(aslctop) },
	{ 0xc6000000, 0xff000000, N("stas"), RSRC1, T(mcdst), ADST, T(aslctop) },
	{ 0xc7000000, 0xff000000, N("xdst"), ADSTD, ASRC1, T(xdimm) },
	{ 0xc8000000, 0xff000000, N("ldaxh"), VDSTQ, T(mcdst), ASRC1, T(aslctop) },
	{ 0xc9000000, 0xff000000, N("ldaxv"), VDSTQ, T(mcdst), ASRC1, T(aslctop) },
	{ 0xca000000, 0xff000000, N("aadd"), ADST, T(mcdst), T(aslctop), IGNSRC1 },
	{ 0xcb000000, 0xff000000, N("add"), ADST, T(mcdst), ASRC1, T(aslctop) },
	{ 0xcc000000, 0xff000000, N("setlo"), ADST, IMM16 },
	{ 0xcd000000, 0xff000000, N("sethi"), ADST, IMMH },
	{ 0xce000000, 0xff010000, N("xdbar"), T(barldsti), BARW, BARU },
	{ 0xce010000, 0xff010000, N("xdbar"), T(barldstr), ADST, BARU },
	{ 0xcf000000, 0xff010000, N("xdwait"), T(barldsti), BARW, BARU },
	{ 0xcf010000, 0xff010000, N("xdwait"), T(barldstr), ADST, BARU },
	{ 0xd0000000, 0xff000000, N("ldavh"), VDST, T(mcdst), ASRC1, IMM },
	{ 0xd1000000, 0xff000000, N("ldavv"), VDST, T(mcdst), ASRC1, IMM },
	{ 0xd2000000, 0xff000000, N("ldas"), RDST, T(mcdst), ASRC1, IMM },
	{ 0xd3000008, 0xff000078, N("nor"), ADST, T(mcdst), ASRC1, ASRC2 },
	{ 0xd3000010, 0xff000078, N("and"), ADST, T(mcdst), N("not"), ASRC1, ASRC2 },
	{ 0xd3000020, 0xff000078, N("and"), ADST, T(mcdst), ASRC1, N("not"), ASRC2 },
	{ 0xd3000030, 0xff000078, N("xor"), ADST, T(mcdst), ASRC1, ASRC2 },
	{ 0xd3000038, 0xff000078, N("nand"), ADST, T(mcdst), ASRC1, ASRC2 },
	{ 0xd3000040, 0xff000078, N("and"), ADST, T(mcdst), ASRC1, ASRC2 },
	{ 0xd3000048, 0xff000078, N("nxor"), ADST, T(mcdst), ASRC1, ASRC2 },
	{ 0xd3000058, 0xff000078, N("or"), ADST, T(mcdst), N("not"), ASRC1, ASRC2 },
	{ 0xd3000068, 0xff000078, N("or"), ADST, T(mcdst), ASRC1, N("not"), ASRC2 },
	{ 0xd3000070, 0xff000078, N("or"), ADST, T(mcdst), ASRC1, ASRC2 },
	{ 0xd3000000, 0xff000000, N("bitop"), BITOP, ADST, T(mcdst), ASRC1, ASRC2 },
	{ 0xd4000000, 0xff000000, N("stavh"), VSRC1, T(mcdst), ADST, IMM },
	{ 0xd5000000, 0xff000000, N("stavv"), VSRC1, T(mcdst), ADST, IMM },
	{ 0xd6000000, 0xff000000, N("stas"), RSRC1, T(mcdst), ADST, IMM },
	{ 0xd7000000, 0xff000001, N("ldr"), VDST, ASRC1, VSRC2 },
	{ 0xd7000001, 0xff000001, N("star"), VSRC1, ADST, T(aslctop) },
	{ 0xd8000000, 0xff000000, N("ldvh"), VDST, T(mcdst), ASRC1, UIMM },
	{ 0xd9000000, 0xff000000, N("ldvv"), VDST, T(mcdst), ASRC1, UIMM },
	{ 0xda000000, 0xff000000, N("lds"), RDST, T(mcdst), ASRC1, UIMM },
	{ 0xdc000000, 0xff000000, N("stvh"), VSRC1, T(mcdst), ADST, UIMM },
	{ 0xdd000000, 0xff000000, N("stvv"), VSRC1, T(mcdst), ADST, UIMM },
	{ 0xde000000, 0xff000000, N("sts"), RSRC1, T(mcdst), ADST, UIMM },
	{ 0xdf000000, 0xff000000, N("anop"), IGNALL },
	{ 0xe00001e0, 0xff0001f8, N("bra"), T(mcdst), BTARG },
	{ 0xe0000000, 0xff000000, N("bra"), T(mcdst), T(pred), BTARG },
	{ 0xe10001e0, 0xff0001f8, N("bra"), N("loop"), LDSTL, T(mcdst), LSRCL, BTARG },
	{ 0xe1000000, 0xff000000, N("bra"), N("loop"), LDSTL, T(mcdst), LSRCL, T(pred), BTARG },
	{ 0xe2000000, 0xff000000, N("bra"), T(mcdst), N("not"), T(pred), BTARG },
	{ 0xe3000000, 0xff000000, N("bra"), N("loop"), LDSTL, T(mcdst), LSRCL, N("not"), T(pred), BTARG },
	{ 0xe40001e0, 0xff0001f8, N("call"), T(mcdst), CTARG },
	{ 0xe4000000, 0xff000000, N("call"), T(mcdst), T(pred), CTARG },
	{ 0xe50001e0, 0xff0001f8, N("call"), N("loop"), LDSTL, T(mcdst), LSRCL, BTARG },
	{ 0xe5000000, 0xff000000, N("call"), N("loop"), LDSTL, T(mcdst), LSRCL, T(pred), BTARG },
	{ 0xe6000000, 0xff000000, N("call"), T(mcdst), N("not"), T(pred), CTARG },
	{ 0xe7000000, 0xff000000, N("call"), N("loop"), LDSTL, T(mcdst), LSRCL, N("not"), T(pred), BTARG },
	{ 0xe8000000, 0xff000000, N("ret"), T(mcdst), IGNALL },
	{ 0xea000000, 0xff000000, N("abra"), IGNDST, ABTARG },
	{ 0xef000000, 0xff000000, N("bnop"), IGNALL },
	{ 0xf0000000, 0xff000000, N("mov"), LDST, CDST, IMM16 },
	{ 0xff000000, 0xff000000, N("exit"), IGNDST, T(intr), IMM16 },
	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0, 0, OP1B, T(m) },
};

struct disisa vp1_isa_s = {
	tabroot,
	1,
	1,
	4,
};
