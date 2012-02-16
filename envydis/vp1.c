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
static struct bitfield exitcoff = { 0, 16 };
static struct bitfield aimmoff = { { 3, 11 }, BF_SIGNED };
static struct bitfield aimmloff = { { 0, 19 }, BF_SIGNED };
static struct bitfield aimmhoff = { { 0, 16 }, .shr = 16 };
static struct bitfield compoff = { 3, 2 };
#define ABTARG atombtarg, &abtargoff
#define BTARG atombtarg, &btargoff
#define EXITC atomimm, &exitcoff
#define AIMM atomimm, &aimmoff
#define AIMML atomimm, &aimmloff
#define AIMMH atomimm, &aimmhoff
#define COMP atomimm, &compoff

/*
 * Register fields
 */

static struct sreg areg_sr[] = {
	{ 31, 0, SR_ZERO },
	{ -1 },
};
static struct bitfield dst_bf = { 19, 5 };
static struct bitfield ydst_bf = { 19, 4 };
static struct bitfield ddst_bf = { 19, 3 };
static struct bitfield zdst_bf = { 19, 1 };
static struct bitfield src1_bf = { 14, 5 };
static struct bitfield xdst_bf = { 19, 5, 3, 1 };
static struct bitfield srdst_bf = { 19, 5, 3, 2 };
static struct bitfield ysrc1_bf = { 14, 4 };
static struct bitfield dsrc1_bf = { 14, 3 };
static struct bitfield csrc1_bf = { 14, 2 };
static struct bitfield zsrc1_bf = { 14, 1 };
static struct bitfield xsrc1_bf = { 14, 5, 3, 1 };
static struct bitfield srsrc1_bf = { 14, 5, 3, 2 };
static struct bitfield src2_bf = { 9, 5 };
static struct bitfield cdst_bf = { 0, 2 };
static struct bitfield mcdst_bf = { 0, 3 };
static struct reg adst_r = { &dst_bf, "a", .specials = areg_sr };
static struct reg rdst_r = { &dst_bf, "r" };
static struct reg vdst_r = { &dst_bf, "v" };
static struct reg ddst_r = { &ddst_bf, "d" };
static struct reg xdst_r = { &xdst_bf, "x" };
static struct reg ydst_r = { &ydst_bf, "y" };
static struct reg zdst_r = { &zdst_bf, "z" };
static struct reg srdst_r = { &srdst_bf, "sr" };
static struct reg asrc1_r = { &src1_bf, "a", .specials = areg_sr };
static struct reg asrc2_r = { &src2_bf, "a", .specials = areg_sr };
static struct reg rsrc1_r = { &src1_bf, "r" };
static struct reg vsrc1_r = { &src1_bf, "v" };
static struct reg csrc1_r = { &csrc1_bf, "c" };
static struct reg dsrc1_r = { &dsrc1_bf, "d" };
static struct reg xsrc1_r = { &xsrc1_bf, "x" };
static struct reg ysrc1_r = { &ysrc1_bf, "y" };
static struct reg zsrc1_r = { &zsrc1_bf, "z" };
static struct reg srsrc1_r = { &srsrc1_bf, "sr" };
static struct reg cdst_r = { &cdst_bf, "c" };

#define ADST atomreg, &adst_r
#define RDST atomreg, &rdst_r
#define VDST atomreg, &vdst_r
#define DDST atomreg, &ddst_r
#define XDST atomreg, &xdst_r
#define YDST atomreg, &ydst_r
#define ZDST atomreg, &zdst_r
#define SRDST atomreg, &srdst_r
#define ASRC1 atomreg, &asrc1_r
#define RSRC1 atomreg, &rsrc1_r
#define VSRC1 atomreg, &vsrc1_r
#define CSRC1 atomreg, &csrc1_r
#define DSRC1 atomreg, &dsrc1_r
#define XSRC1 atomreg, &xsrc1_r
#define YSRC1 atomreg, &ysrc1_r
#define ZSRC1 atomreg, &zsrc1_r
#define SRSRC1 atomreg, &srsrc1_r
#define ASRC2 atomreg, &asrc2_r
#define CDST atomreg, &cdst_r
#define IGNCE atomign, &cdst_bf
#define IGNCD atomign, &mcdst_bf

F1(intr, 16, N("intr"))

F(mcdst, 2, CDST, IGNCE);

static struct insn tabm[] = {
	{ 0x4fffffff, 0xffffffff, N("nop4") },
	{ 0x62000000, 0xff000000, N("and"), ADST, T(mcdst), ASRC1, AIMM },
	{ 0x63000000, 0xff000000, N("xor"), ADST, T(mcdst), ASRC1, AIMM },
	{ 0x64000000, 0xff000000, N("or"), ADST, T(mcdst), ASRC1, AIMM },
	{ 0x65000000, 0xff000000, N("mov"), ADST, AIMML },
	{ 0x68000000, 0xff000000, N("min"), ADST, T(mcdst), ASRC1, AIMM },
	{ 0x69000000, 0xff000000, N("max"), ADST, T(mcdst), ASRC1, AIMM },
	{ 0x6a000000, 0xff0000e0, N("mov"), VDST, COMP, ASRC1, IGNCD },
	{ 0x6a000040, 0xff0000e0, N("mov"), SRDST, ASRC1, IGNCD },
	{ 0x6a000060, 0xff0000f8, N("mov"), RDST, ASRC1, IGNCD },
	{ 0x6a0000a0, 0xff0000f0, N("mov"), XDST, ASRC1, IGNCD },
	{ 0x6a0000b0, 0xff0000f8, N("mov"), DDST, ASRC1, IGNCD },
	{ 0x6a0000b8, 0xff0000f8, N("mov"), ZDST, ASRC1, IGNCD },
	{ 0x6a0000c0, 0xff0000f8, N("mov"), YDST, ASRC1, IGNCD },
	{ 0x6b000000, 0xff0000e0, N("mov"), ADST, VSRC1, COMP, IGNCD },
	{ 0x6b000040, 0xff0000e0, N("mov"), ADST, SRSRC1, IGNCD },
	{ 0x6b000060, 0xff0000f8, N("mov"), ADST, RSRC1, IGNCD },
	{ 0x6b000068, 0xff0000f8, N("mov"), ADST, CSRC1, IGNCD },
	{ 0x6b0000a0, 0xff0000f0, N("mov"), ADST, XSRC1, IGNCD },
	{ 0x6b0000b0, 0xff0000f8, N("mov"), ADST, DSRC1, IGNCD },
	{ 0x6b0000b8, 0xff0000f8, N("mov"), ADST, ZSRC1, IGNCD },
	{ 0x6b0000c0, 0xff0000f8, N("mov"), ADST, YSRC1, IGNCD },
	{ 0x6c000000, 0xff000000, N("add"), ADST, T(mcdst), ASRC1, AIMM },
	{ 0x6e000000, 0xff000000, N("sar"), ADST, T(mcdst), ASRC1, AIMM },
	{ 0x75000000, 0xff000000, N("sethi"), ADST, AIMMH },
	{ 0x7e000000, 0xff000000, N("shr"), ADST, T(mcdst), ASRC1, AIMM },
	{ 0x60000000, 0xe0000000, OOPS, ADST, ASRC1, ASRC2 },
	{ 0xbf000007, 0xffffffff, N("nopb") },
	{ 0xdf000007, 0xffffffff, N("nopd") },
	{ 0xe0000000, 0xff000000, N("bra0"), BTARG },
	{ 0xe1000000, 0xff000000, N("bra1"), BTARG },
	{ 0xe2000000, 0xff000000, N("bra2"), BTARG },
	{ 0xe4000000, 0xff000000, N("bra4"), BTARG },
	{ 0xeaf80000, 0xffff0000, N("abra"), ABTARG },
	{ 0xef0001ff, 0xffffffff, N("nope") },
	{ 0xefffffff, 0xffffffff, U("end") },
	{ 0xfff80000, 0xfff80000, N("exit"), T(intr), EXITC },
	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0, 0, OP32, T(m) },
};

const struct disisa vp1_isa_s = {
	tabroot,
	4,
	4,
	16,
};
