/*
 *
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "dis-intern.h"

/*
 * Immediate fields
 */

static struct bitfield abtargoff = { 0, 16 };
static struct bitfield btargoff = { { 9, 15 }, BF_SIGNED, .pcrel = 1 };
static struct bitfield imm16off = { 0, 16 };
static struct bitfield immoff = { { 3, 11 }, BF_SIGNED };
static struct bitfield immloff = { { 0, 19 }, BF_SIGNED };
static struct bitfield immhoff = { { 0, 16 }, .shr = 16 };
static struct bitfield xdimmoff = { { 0, 13 } };
static struct bitfield compoff = { 3, 2 };
static struct bitfield logopoff = { 3, 4 };
static struct bitfield baruoff = { 3, 2 };
static struct bitfield barwoff = { 20, 2 };
#define ABTARG atombtarg, &abtargoff
#define BTARG atombtarg, &btargoff
#define CTARG atomctarg, &btargoff
#define IMM16 atomimm, &imm16off
#define IMM atomimm, &immoff
#define IMML atomimm, &immloff
#define IMMH atomimm, &immhoff
#define XDIMM atomimm, &xdimmoff
#define COMP atomimm, &compoff
#define LOGOP atomimm, &logopoff
#define BARU atomimm, &baruoff
#define BARW atomimm, &barwoff

/*
 * Register fields
 */

static struct sreg sreg_sr[] = {
	{ 30, "tick" },
	{ 31, "csreq" },
	{ 96, "l0" },
	{ 97, "l1" },
	{ 98, "l2" },
	{ 99, "l3" },
	{ -1 },
};
static struct sreg rreg_sr[] = {
	{ 31, 0, SR_ZERO },
	{ -1 },
};
static struct bitfield dst_bf = { 19, 5 };
static struct bitfield dstx_bf = { 19, 5, .xorend = 1 };
static struct bitfield ydst_bf = { 19, 4 };
static struct bitfield ddst_bf = { 19, 3 };
static struct bitfield cdst_bf = { 19, 2 };
static struct bitfield zdst_bf = { 19, 1 };
static struct bitfield src1_bf = { 14, 5 };
static struct bitfield src1x_bf = { 14, 5, .xorend = 1 };
static struct bitfield xdst_bf = { 19, 5, 3, 1 };
static struct bitfield srdst_bf = { 19, 5, 3, 2 };
static struct bitfield ysrc1_bf = { 14, 4 };
static struct bitfield dsrc1_bf = { 14, 3 };
static struct bitfield csrc1_bf = { 14, 2 };
static struct bitfield zsrc1_bf = { 14, 1 };
static struct bitfield xsrc1_bf = { 14, 5, 3, 1 };
static struct bitfield srsrc1_bf = { 14, 5, 3, 2 };
static struct bitfield src2_bf = { 9, 5 };
static struct bitfield src2x_bf = { { 9, 5 }, .xorend = 1 };
static struct bitfield csrcp_bf = { 3, 2 };
static struct bitfield cdstp_bf = { 0, 2 };
static struct bitfield mcdst_bf = { 0, 3 };
static struct bitfield ignall_bf = { 0, 24 };
static struct reg adst_r = { &dst_bf, "a" };
static struct reg rdst_r = { &dst_bf, "r", .specials = rreg_sr };
static struct reg vdst_r = { &dst_bf, "v" };
static struct reg ddst_r = { &ddst_bf, "d" };
static struct reg cdst_r = { &cdst_bf, "c" };
static struct reg ldst_r = { &cdst_bf, "l" };
static struct reg xdst_r = { &xdst_bf, "x" };
static struct reg ydst_r = { &ydst_bf, "y" };
static struct reg zdst_r = { &zdst_bf, "z" };
static struct reg srdst_r = { &srdst_bf, "sr", .specials = sreg_sr, .always_special = 1 };
static struct reg adstx_r = { &dstx_bf, "a" };
static struct reg asrc1_r = { &src1_bf, "a" };
static struct reg asrc2_r = { &src2_bf, "a" };
static struct reg rsrc2_r = { &src2_bf, "r", .specials = rreg_sr };
static struct reg asrc1x_r = { &src1x_bf, "a" };
static struct reg asrc2x_r = { &src2x_bf, "a" };
static struct reg rsrc2x_r = { &src2x_bf, "r", .specials = rreg_sr };
static struct reg rsrc1_r = { &src1_bf, "r", .specials = rreg_sr };
static struct reg vsrc1_r = { &src1_bf, "v" };
static struct reg csrc1_r = { &csrc1_bf, "c" };
static struct reg dsrc1_r = { &dsrc1_bf, "d" };
static struct reg xsrc1_r = { &xsrc1_bf, "x" };
static struct reg ysrc1_r = { &ysrc1_bf, "y" };
static struct reg zsrc1_r = { &zsrc1_bf, "z" };
static struct reg srsrc1_r = { &srsrc1_bf, "sr", .specials = sreg_sr, .always_special = 1 };
static struct reg csrcp_r = { &csrcp_bf, "c" };
static struct reg lsrcl_r = { &csrcp_bf, "l" };
static struct reg cdstp_r = { &cdstp_bf, "c" };
static struct reg ldstl_r = { &cdstp_bf, "l" };

#define ADST atomreg, &adst_r
#define RDST atomreg, &rdst_r
#define VDST atomreg, &vdst_r
#define DDST atomreg, &ddst_r
#define CDST atomreg, &cdst_r
#define LDST atomreg, &ldst_r
#define XDST atomreg, &xdst_r
#define YDST atomreg, &ydst_r
#define ZDST atomreg, &zdst_r
#define SRDST atomreg, &srdst_r
#define ADSTX atomreg, &adstx_r
#define ASRC1 atomreg, &asrc1_r
#define RSRC1 atomreg, &rsrc1_r
#define VSRC1 atomreg, &vsrc1_r
#define CSRC1 atomreg, &csrc1_r
#define DSRC1 atomreg, &dsrc1_r
#define XSRC1 atomreg, &xsrc1_r
#define YSRC1 atomreg, &ysrc1_r
#define ZSRC1 atomreg, &zsrc1_r
#define SRSRC1 atomreg, &srsrc1_r
#define ASRC1X atomreg, &asrc1x_r
#define ASRC2 atomreg, &asrc2_r
#define RSRC2 atomreg, &rsrc2_r
#define ASRC2X atomreg, &asrc2x_r
#define RSRC2X atomreg, &rsrc2x_r
#define CSRCP atomreg, &csrcp_r
#define CDSTP atomreg, &cdstp_r
#define LSRCL atomreg, &lsrcl_r
#define LDSTL atomreg, &ldstl_r
#define IGNCE atomign, &cdstp_bf
#define IGNCD atomign, &mcdst_bf
#define IGNALL atomign, &ignall_bf
#define IGNDST atomign, &dst_bf
#define IGNSRC1 atomign, &src1_bf

static struct mem memsr_m = { "", 0, &asrc1_r, 0, &asrc2_r, .postincr = 1 };
static struct mem memdr_m = { "", 0, &adst_r, 0, &asrc2_r, .postincr = 1 };
static struct mem memsi_m = { "", 0, &asrc1_r, &immoff };
static struct mem memdi_m = { "", 0, &adst_r, &immoff };
static struct mem memspi_m = { "", 0, &asrc1_r, &immoff, .postincr = 1 };
static struct mem memdpi_m = { "", 0, &adst_r, &immoff, .postincr = 1 };
#define MEMSR atommem, &memsr_m
#define MEMDR atommem, &memdr_m
#define MEMSI atommem, &memsi_m
#define MEMDI atommem, &memdi_m
#define MEMSPI atommem, &memspi_m
#define MEMDPI atommem, &memdpi_m

F1(intr, 16, N("intr"))

F(mcdst, 2, CDSTP, IGNCE);

F(xdimm, 13, XDIMM, );

F(barldsti, 19, N("st"), N("ld"));
F(barldstr, 0, N("st"), N("ld"));

static struct insn tabaslctop[] = {
	{ 0x00000000, 0x000001e0, SESTART, N("slct"), CSRCP, N("sf"), ASRC2, ASRC2X, SEEND },
	{ 0x00000020, 0x000001e0, SESTART, N("slct"), CSRCP, N("zf"), ASRC2, ASRC2X, SEEND },
	{ 0x00000040, 0x000001e0, SESTART, N("slct"), CSRCP, U("2"), ASRC2, ASRC2X, SEEND },
	{ 0x00000060, 0x000001e0, SESTART, N("slct"), CSRCP, U("3"), ASRC2, ASRC2X, SEEND },
	{ 0x00000080, 0x000001e0, SESTART, N("slct"), CSRCP, U("4"), ASRC2, ASRC1, SEEND },
	{ 0x000000a0, 0x000001e0, SESTART, N("slct"), CSRCP, U("5"), ASRC2, ASRC2X, SEEND },
	{ 0x000000c0, 0x000001e0, SESTART, N("slct"), CSRCP, U("6"), ASRC2, ASRC2X, SEEND },
	{ 0x000000e0, 0x000001e0, SESTART, N("slct"), CSRCP, U("7"), ASRC2, ASRC2X, SEEND },
	{ 0x00000100, 0x000001e0, SESTART, N("slct"), CSRCP, N("asf"), ASRC2, ASRC2X, SEEND },
	{ 0x00000120, 0x000001e0, SESTART, N("slct"), CSRCP, N("azf"), ASRC2, ASRC2X, SEEND },
	{ 0x00000140, 0x000001e0, SESTART, N("slct"), CSRCP, N("alf"), ASRC2, ASRC2X, SEEND },
	{ 0x00000160, 0x000001e0, SESTART, N("slct"), CSRCP, U("11"), ASRC2, ASRC2X, SEEND },
	{ 0x00000180, 0x000001e0, SESTART, N("slct"), CSRCP, U("12"), ASRC2, ASRC2X, SEEND },
	{ 0x000001a0, 0x000001e0, SESTART, N("slct"), CSRCP, N("lzf"), ASRC2, ASRC2X, SEEND },
	{ 0x000001c0, 0x000001e0, ASRC2 },
	{ 0x000001e0, 0x000001e0, ASRC2X },
	{ 0, 0, OOPS }
};

static struct insn tabslctop[] = {
	{ 0x00000000, 0x000001e0, SESTART, N("slct"), CSRCP, N("sf"), RSRC2, RSRC2X, SEEND },
	{ 0x00000020, 0x000001e0, SESTART, N("slct"), CSRCP, N("zf"), RSRC2, RSRC2X, SEEND },
	{ 0x00000040, 0x000001e0, SESTART, N("slct"), CSRCP, U("2"), RSRC2, RSRC2X, SEEND },
	{ 0x00000060, 0x000001e0, SESTART, N("slct"), CSRCP, U("3"), RSRC2, RSRC2X, SEEND },
	{ 0x00000080, 0x000001e0, SESTART, N("slct"), CSRCP, U("4"), RSRC2, RSRC1, SEEND },
	{ 0x000000a0, 0x000001e0, SESTART, N("slct"), CSRCP, U("5"), RSRC2, RSRC2X, SEEND },
	{ 0x000000c0, 0x000001e0, SESTART, N("slct"), CSRCP, U("6"), RSRC2, RSRC2X, SEEND },
	{ 0x000000e0, 0x000001e0, SESTART, N("slct"), CSRCP, U("7"), RSRC2, RSRC2X, SEEND },
	{ 0x00000100, 0x000001e0, SESTART, N("slct"), CSRCP, N("asf"), RSRC2, RSRC2X, SEEND },
	{ 0x00000120, 0x000001e0, SESTART, N("slct"), CSRCP, N("azf"), RSRC2, RSRC2X, SEEND },
	{ 0x00000140, 0x000001e0, SESTART, N("slct"), CSRCP, N("alf"), RSRC2, RSRC2X, SEEND },
	{ 0x00000160, 0x000001e0, SESTART, N("slct"), CSRCP, U("11"), RSRC2, RSRC2X, SEEND },
	{ 0x00000180, 0x000001e0, SESTART, N("slct"), CSRCP, U("12"), RSRC2, RSRC2X, SEEND },
	{ 0x000001a0, 0x000001e0, SESTART, N("slct"), CSRCP, N("lzf"), RSRC2, RSRC2X, SEEND },
	{ 0x000001c0, 0x000001e0, RSRC2 },
	{ 0x000001e0, 0x000001e0, RSRC2X },
	{ 0, 0, OOPS }
};

static struct insn tabpred[] = {
	{ 0x00000000, 0x000001e0, CSRCP, N("sf") },
	{ 0x00000020, 0x000001e0, CSRCP, N("zf") },
	{ 0x00000040, 0x000001e0, CSRCP, U("2") },
	{ 0x00000060, 0x000001e0, CSRCP, U("3") },
	{ 0x00000080, 0x000001e0, CSRCP, U("4") },
	{ 0x000000a0, 0x000001e0, CSRCP, U("5") },
	{ 0x000000c0, 0x000001e0, CSRCP, U("6") },
	{ 0x000000e0, 0x000001e0, CSRCP, U("7") },
	{ 0x00000100, 0x000001e0, CSRCP, N("asf") },
	{ 0x00000120, 0x000001e0, CSRCP, N("azf") },
	{ 0x00000140, 0x000001e0, CSRCP, N("alf") },
	{ 0x00000160, 0x000001e0, CSRCP, U("11") },
	{ 0x00000180, 0x000001e0, CSRCP, U("12") },
	{ 0x000001a0, 0x000001e0, CSRCP, N("lzf") },
	{ 0x000001c0, 0x000001e0, CSRCP, N("false") },
	{ 0x000001e0, 0x000001e0, CSRCP, N("true") },
	{ 0, 0, OOPS }
};

static struct insn tabm[] = {
	{ 0x41000000, 0xff000000, N("mul"), RDST, T(mcdst), RSRC1, T(slctop) },
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
	{ 0x42000000, 0xff000000, N("logop"), RDST, T(mcdst), RSRC1, RSRC2, LOGOP },
	{ 0x48000000, 0xff000000, N("min"), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x49000000, 0xff000000, N("max"), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x4c000000, 0xff000000, N("add"), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x4d000000, 0xff000000, N("sub"), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x4e000000, 0xff000000, N("sar"), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x5e000000, 0xff000000, N("shr"), RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x4f000000, 0xff000000, N("snop"), IGNALL },
	{ 0x40000000, 0xe0000000, OOPS, RDST, T(mcdst), RSRC1, T(slctop) },
	{ 0x61000000, 0xff000000, N("mul"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x62000000, 0xff000000, N("and"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x63000000, 0xff000000, N("xor"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x64000000, 0xff000000, N("or"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x65000000, 0xff000000, N("mov"), RDST, IMML },
	{ 0x68000000, 0xff000000, N("min"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x69000000, 0xff000000, N("max"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x6a000000, 0xff0000e0, N("mov"), VDST, COMP, RSRC1, IGNCD },
	{ 0x6a000040, 0xff0000e0, N("mov"), SRDST, RSRC1, IGNCD },
	{ 0x6a000060, 0xff0000f8, N("mov"), ADST, RSRC1, IGNCD },
	{ 0x6a0000a0, 0xff0000f0, N("mov"), XDST, RSRC1, IGNCD },
	{ 0x6a0000b0, 0xff0000f8, N("mov"), DDST, RSRC1, IGNCD },
	{ 0x6a0000b8, 0xff0000f8, N("mov"), ZDST, RSRC1, IGNCD },
	{ 0x6a0000c0, 0xff0000f8, N("mov"), YDST, RSRC1, IGNCD },
	{ 0x6b000000, 0xff0000e0, N("mov"), RDST, VSRC1, COMP, IGNCD },
	{ 0x6b000040, 0xff0000e0, N("mov"), RDST, SRSRC1, IGNCD },
	{ 0x6b000060, 0xff0000f8, N("mov"), RDST, ASRC1, IGNCD },
	{ 0x6b000068, 0xff0000f8, N("mov"), RDST, CSRC1, IGNCD },
	{ 0x6b0000a0, 0xff0000f0, N("mov"), RDST, XSRC1, IGNCD },
	{ 0x6b0000b0, 0xff0000f8, N("mov"), RDST, DSRC1, IGNCD },
	{ 0x6b0000b8, 0xff0000f8, N("mov"), RDST, ZSRC1, IGNCD },
	{ 0x6b0000c0, 0xff0000f8, N("mov"), RDST, YSRC1, IGNCD },
	{ 0x6c000000, 0xff000000, N("add"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x6d000000, 0xff000000, N("sub"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x6e000000, 0xff000000, N("sar"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x75000000, 0xff000000, N("sethi"), RDST, IMMH },
	{ 0x7e000000, 0xff000000, N("shr"), RDST, T(mcdst), RSRC1, IMM },
	{ 0x60000000, 0xe0000000, OOPS, RDST, T(mcdst), RSRC1, IMM },
	{ 0xbf000000, 0xff000000, N("vnop"), IGNALL },
	{ 0xc00001c0, 0xff0001f8, N("ld"), VDST, MEMSR, T(mcdst) },
	{ 0xc10001c0, 0xff0001f8, N("ld"), VDST, N("vert"), MEMSR, T(mcdst) },
	{ 0xc20001c0, 0xff0001f8, N("ld"), RDST, MEMSR, T(mcdst) },
	{ 0xc3000000, 0xff000000, N("xdld"), ADST, ASRC1, ASRC1X, T(xdimm) },
	{ 0xc40001c0, 0xff0001f8, N("st"), MEMDR, T(mcdst), VSRC1 },
	{ 0xc50001c0, 0xff0001f8, N("st"), N("vert"), MEMDR, T(mcdst), VSRC1 },
	{ 0xc60001c0, 0xff0001f8, N("st"), MEMDR, T(mcdst), RSRC1 },
	{ 0xc7000000, 0xff000000, N("xdst"), ADST, ADSTX, ASRC1, T(xdimm) },
	{ 0xca000000, 0xff000000, N("hadd"), ADST, T(mcdst), T(aslctop), IGNSRC1 },
	{ 0xcb000000, 0xff000000, N("add"), ADST, T(mcdst), ASRC1, T(aslctop) },
	{ 0xcc000000, 0xff000000, N("setlo"), ADST, IMM16 },
	{ 0xcd000000, 0xff000000, N("sethi"), ADST, IMMH },
	{ 0xce000000, 0xff010000, N("xdbar"), T(barldsti), BARW, BARU },
	{ 0xce010000, 0xff010000, N("xdbar"), T(barldstr), ADST, BARU },
	{ 0xcf000000, 0xff010000, N("xdwait"), T(barldsti), BARW, BARU },
	{ 0xcf010000, 0xff010000, N("xdwait"), T(barldstr), ADST, BARU },
	{ 0xd0000000, 0xff000000, N("ld"), VDST, MEMSPI, T(mcdst) },
	{ 0xd1000000, 0xff000000, N("ld"), VDST, N("vert"), MEMSPI, T(mcdst) },
	{ 0xd2000000, 0xff000000, N("ld"), RDST, MEMSPI, T(mcdst) },
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
	{ 0xd3000000, 0xff000000, N("logop"), ADST, T(mcdst), ASRC1, ASRC2, LOGOP },
	{ 0xd4000000, 0xff000000, N("st"), MEMDPI, T(mcdst), VSRC1 },
	{ 0xd5000000, 0xff000000, N("st"), N("vert"), MEMDPI, T(mcdst), VSRC1 },
	{ 0xd6000000, 0xff000000, N("st"), MEMDPI, T(mcdst), RSRC1 },
	{ 0xd8000000, 0xff000000, N("ld"), VDST, MEMSI, T(mcdst) },
	{ 0xd9000000, 0xff000000, N("ld"), VDST, N("vert"), MEMSI, T(mcdst) },
	{ 0xda000000, 0xff000000, N("ld"), RDST, MEMSI, T(mcdst) },
	{ 0xdc000000, 0xff000000, N("st"), MEMDI, T(mcdst), VSRC1 },
	{ 0xdd000000, 0xff000000, N("st"), N("vert"), MEMDI, T(mcdst), VSRC1 },
	{ 0xde000000, 0xff000000, N("st"), MEMDI, T(mcdst), RSRC1 },
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
	{ 0, 0, OP32, T(m) },
};

struct disisa vp1_isa_s = {
	tabroot,
	4,
	4,
	16,
};
