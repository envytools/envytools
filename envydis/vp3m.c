/*
 * Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
 * Copyright (C) 2011 Paweł Czaplejewicz <pcz@porcupinefactory.org>
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

#define VP2 1
#define VP3 2

/*
 * Code target field
 *
 * This field represents a code address and is used for branching and the
 * likes. Target is counted in 32-bit words from the start of microcode.
 */

static struct bitfield ctargoff = { 8, 11 };
#define BTARG atombtarg, &ctargoff
#define CTARG atomctarg, &ctargoff

static struct bitfield imm4off = { 12, 4 };
static struct bitfield imm6off = { { 12, 4, 24, 2 } };
static struct bitfield imm12off = { { 8, 8, 20, 4 } };
static struct bitfield imm14off = { { 8, 8, 20, 6 } };
static struct bitfield bimm8off = { { 12, 4, 20, 4 } };
static struct bitfield bimmstoff = { 16, 8 };
static struct bitfield bimmstsoff = { 16, 4 };
#define IMM4 atomimm, &imm4off
#define IMM6 atomimm, &imm6off
#define IMM12 atomimm, &imm12off
#define IMM14 atomimm, &imm14off

/*
 * Register fields
 */

static struct sreg reg_sr[] = {
	{ 0, 0, SR_ZERO },
	{ -1 },
};
static struct sreg pred_sr[] = {
	{ 1, "np0" },
	{ 15, 0, SR_ONE },
	{ -1 },
};
static struct bitfield src1_bf = { 8, 4 };
static struct bitfield isrc_bf = { 8, 4, 24, 1 };
static struct bitfield src2_bf = { 12, 4 };
static struct bitfield dst_bf = { 16, 4 };
static struct bitfield ldst_bf = { 16, 4, 24, 1 };
static struct bitfield psrc1_bf = { 8, 4 };
static struct bitfield psrc2_bf = { 12, 4 };
static struct bitfield pdst_bf = { 16, 4 };
static struct bitfield pred_bf = { 20, 4 };
static struct reg src1_r = { &src1_bf, "r", .specials = reg_sr };
static struct reg src2_r = { &src2_bf, "r", .specials = reg_sr };
static struct reg dst_r = { &dst_bf, "r", .specials = reg_sr };
static struct reg odst_r = { &ldst_bf, "o", .cool = 1 };
static struct reg isrc_r = { &isrc_bf, "i", .cool = 1 };
static struct reg psrc1_r = { &psrc1_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg psrc2_r = { &psrc2_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg pdst_r = { &pdst_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg pred_r = { &pred_bf, "p", .cool = 1, .specials = pred_sr };
#define SRC1 atomreg, &src1_r
#define SRC2 atomreg, &src2_r
#define DST atomreg, &dst_r
#define ISRC atomreg, &isrc_r
#define ODST atomreg, &odst_r
#define PSRC1 atomreg, &psrc1_r
#define PSRC2 atomreg, &psrc2_r
#define PDST atomreg, &pdst_r
#define PRED atomreg, &pred_r

static struct mem mem1c_m = { "D", 0, &src1_r, &bimm8off };
static struct mem mem34_m = { "D", 0, &src1_r, 0, &src2_r };
static struct mem memst_m = { "D", 0, &src1_r, &bimmstoff };
static struct mem memsts_m = { "D", 0, &src1_r, &bimmstsoff };
#define MEM1C atommem, &mem1c_m
#define MEMST atommem, &memst_m
#define MEMSTS atommem, &memsts_m
#define MEM34 atommem, &mem34_m

static struct insn tabp[] = {
	{ 0x00f00000, 0x00f00000 },
	{ 0, 0, PRED },
};

static struct insn tabdst[] = {
	{ 0x00000000, 0x10000000, DST },
	{ 0x10000000, 0x10000000, ODST },
	{ 0, 0, OOPS },
};

static struct insn tabsrc1[] = {
	{ 0x00000000, 0x04000000, SRC1 },
	{ 0x04000000, 0x04000000, ISRC },
	{ 0, 0, OOPS },
};

static struct insn tabsrc2[] = {
	{ 0x00000000, 0x08000000, SRC2 },
	{ 0x08000000, 0x1c000000, IMM6 },
	{ 0x18000000, 0x1c000000, IMM4 },
	{ 0x0c000000, 0x0c000000, IMM4 },
	{ 0, 0, OOPS },
};

static struct insn tabm[] = {
	{ 0x3c000000, 0x3c0000ff, N("bra"), BTARG },
	{ 0x3c000002, 0x3c0000ff, N("call"), CTARG },
	{ 0x34000003, 0x3c0000ff, N("ret") },
	{ 0x00000004, 0x000000ff, U("04") },
	{ 0x00000005, 0x000000ff, U("05") },
	{ 0x00000006, 0x000000ff, U("06") },
	{ 0x00000008, 0x000000ff, U("08") },
	{ 0x00000009, 0x000000ff, U("09") },
	{ 0x0000000a, 0x000000ff, U("0a") },
	{ 0x0000000b, 0x000000ff, U("0b") },
	{ 0x0000000e, 0x000000ff, U("0e") },

	{ 0x00000020, 0x000000ff, U("20") },
	{ 0x00000021, 0x000000ff, U("21") },
	{ 0x00000022, 0x000000ff, U("22") },
	{ 0x00000023, 0x000000ff, U("23") },
	{ 0x00000024, 0x000000ff, U("24") },
	{ 0x00000025, 0x000000ff, U("25") },
	{ 0x00000026, 0x000000ff, U("26") },
    { 0x00000028, 0x200000ff, N("orsetsg"), PRED, T(src1), T(src2) }, /* PDST |= (src1 > src2) */
    { 0x20000028, 0x200000ff, N("orsetsg"), PDST, T(src1), T(src2) },
    { 0x00000029, 0x200000ff, N("orsetsl"), PRED, T(src1), T(src2) }, /* PDST |= (src1 < src2) */
    { 0x20000029, 0x200000ff, N("orsetsl"), PDST, T(src1), T(src2) },
    { 0x0000002a, 0x200000ff, N("orsetse"), PRED, T(src1), T(src2) }, /* PDST |= (src1 == src2) */
    { 0x2000002a, 0x200000ff, N("orsetse"), PDST, T(src1), T(src2) },
	{ 0x0000002b, 0x000000ff, U("2b") },
	{ 0x0000002c, 0x000000ff, U("2c") },
	{ 0x00000032, 0x000000ff, U("32") },

	{ 0x34000040, 0x3c0000ff, N("and"), PDST, PSRC1, PSRC2 },
	{ 0x34000041, 0x3c0000ff, N("or"), PDST, PSRC1, PSRC2 },
	{ 0x34000042, 0x3c0000ff, N("xor"), PDST, PSRC1, PSRC2 },
	{ 0x34000043, 0x3c0000ff, N("nop") },
	{ 0x34000044, 0x3c0000ff, N("andn"), PDST, PSRC1, PSRC2 },
	{ 0x34000045, 0x3c0000ff, N("orn"), PDST, PSRC1, PSRC2 },
	{ 0x34000046, 0x3c0000ff, N("xorn"), PDST, PSRC1, PSRC2 },
    { 0x00000048, 0x200000ff, N("setsg"), PRED, T(src1), T(src2) },
    { 0x20000048, 0x200000ff, N("setsg"), PDST, T(src1), T(src2) },
    { 0x00000049, 0x200000ff, N("setsl"), PRED, T(src1), T(src2) }, /* signed */
    { 0x20000049, 0x200000ff, N("setsl"), PDST, T(src1), T(src2) },
    { 0x0000004a, 0x200000ff, N("setse"), PRED, T(src1), T(src2) },
    { 0x2000004a, 0x200000ff, N("setse"), PDST, T(src1), T(src2) },
	{ 0x0000004d, 0x000000ff, U("4d") },
	{ 0x0000004e, 0x000000ff, U("4e") },
    { 0x00000052, 0x200000ff, N("btest"), PRED, T(src1), T(src2) },
    { 0x20000052, 0x200000ff, N("btest"), PDST, T(src1), T(src2) },
	{ 0x00000058, 0x200000ff, N("andsnz"), PRED, T(dst), T(src1), T(src2) }, /* normal and, then set pred if result != 0 */
	{ 0x00000059, 0x200000ff, N("orsnz"), PRED, T(dst), T(src1), T(src2) },
	{ 0x0000005c, 0x000000ff, U("5c") },

	{ 0x00000060, 0x200000ff, N("slct"), T(dst), PRED, T(src1), T(src2) }, // dst = PRED ? src1 : src2
	{ 0x00000061, 0x080000ff, N("mov"), T(dst), T(src2) },
	{ 0x18000061, 0x180000ff, N("mov"), ODST, IMM12 },
	{ 0x08000061, 0x180000ff, N("mov"), DST, IMM14 },
	{ 0x00000064, 0x000000ff, N("add"), T(dst), T(src1), T(src2) },
	{ 0x00000065, 0x000000ff, N("sub"), T(dst), T(src1), T(src2) },
	{ 0x00000066, 0x000000ff, N("subr"), T(dst), T(src1), T(src2), .vartype = VP2 },
	{ 0x00000067, 0x000000ff, N("clear"), T(dst), .vartype = VP2 },
	{ 0x00000066, 0x000000ff, N("avgs"), T(dst), T(src1), T(src2), .vartype = VP3 }, // (a+b)/2, rounding UP, signed
	{ 0x00000067, 0x000000ff, N("avgu"), T(dst), T(src1), T(src2), .vartype = VP3 }, // (a+b)/2, rounding UP, unsigned
	{ 0x0000006c, 0x000000ff, N("minsz"), T(dst), T(src1), T(src2) }, // (a > b) ? b : max(a, 0)
	{ 0x0000006d, 0x000000ff, N("clampsex"), T(dst), T(src1), T(src2) }, // clamp to -2^b..2^b-1
	{ 0x0000006e, 0x000000ff, N("sex"), T(dst), T(src1), T(src2) }, // like fuc insn of the same name
	{ 0x0000006f, 0x000000ff, N("div2s"), T(dst), T(src1), .vartype = VP3 }, // signed div by 2, round to 0. Not present on vp2?
	{ 0x00000070, 0x000000ff, N("bset"), T(dst), T(src1), T(src2) },
	{ 0x00000071, 0x000000ff, N("bclr"), T(dst), T(src1), T(src2) },
	{ 0x00000074, 0x000000ff, N("rot8"), T(dst), T(src1) },
	{ 0x00000075, 0x000000ff, N("shl"), T(dst), T(src1), T(src2) },
	{ 0x00000076, 0x000000ff, N("shr"), T(dst), T(src1), T(src2) },
	{ 0x00000077, 0x000000ff, N("sar"), T(dst), T(src1), T(src2) },
	{ 0x00000078, 0x000000ff, N("and"), T(dst), T(src1), T(src2) },
	{ 0x00000079, 0x000000ff, N("or"), T(dst), T(src1), T(src2) },
	{ 0x0000007a, 0x000000ff, N("xor"), T(dst), T(src1), T(src2) },
	{ 0x0000007b, 0x000000ff, N("not"), T(dst), T(src1) },
	{ 0x0000007c, 0x000000ff, U("7c"), T(dst), T(src1), T(src2) },
	{ 0x0000007d, 0x000000ff, N("min"), T(dst), T(src1), T(src2), .vartype = VP3 },
	{ 0x0000007e, 0x000000ff, N("max"), T(dst), T(src1), T(src2), .vartype = VP3},

	{ 0x1c000080, 0x3c0000ff, N("st"), MEMST, SRC2 },
	{ 0x3c000080, 0x3c0000ff, N("st"), MEMSTS, SRC2 },
	{ 0x1c000081, 0x3c0000ff, N("ld"), DST, MEM1C },
	{ 0x34000081, 0x3c0000ff, N("ld"), DST, MEM34 },
	{ 0x00000083, 0x000000ff, U("83") },
	{ 0x00000084, 0x000000ff, U("84") },
	{ 0x00000089, 0x000000ff, U("89") },
	{ 0x1c00008a, 0x3c0000ff, U("8a"), MEMST, SRC2 },
	{ 0x0000008c, 0x000000ff, U("8c") },
	{ 0x0000008d, 0x000000ff, U("8d") },
	{ 0x0000008e, 0x000000ff, U("8e") },
	{ 0x0000008f, 0x000000ff, U("8f") },

	{ 0x000000a0, 0x000000ff, U("a0") },
	{ 0x000000a1, 0x000000ff, U("a1") },
	{ 0x000000a2, 0x000000ff, U("a2") },
	{ 0x000000a4, 0x000000ff, U("a4") },
    { 0x000000a8, 0x200000ff, N("orsetsng"), PRED, T(src1), T(src2) },
    { 0x200000a8, 0x200000ff, N("orsetsng"), PDST, T(src1), T(src2) },
    { 0x000000a9, 0x200000ff, N("orsetsnl"), PRED, T(src1), T(src2) },
    { 0x200000a9, 0x200000ff, N("orsetsnl"), PDST, T(src1), T(src2) },
    { 0x000000aa, 0x200000ff, N("orsetsne"), PRED, T(src1), T(src2) },
    { 0x200000aa, 0x200000ff, N("orsetsne"), PDST, T(src1), T(src2) },
	{ 0x000000ac, 0x000000ff, U("ac") },

    { 0x000000c8, 0x200000ff, N("setsle"), PRED, T(src1), T(src2) },
    { 0x200000c8, 0x200000ff, N("setsle"), PDST, T(src1), T(src2) },
    { 0x000000c9, 0x200000ff, N("setsge"), PRED, T(src1), T(src2) },
    { 0x200000c9, 0x200000ff, N("setsge"), PDST, T(src1), T(src2) },
    { 0x000000ca, 0x200000ff, N("setsne"), PRED, T(src1), T(src2) },
    { 0x200000ca, 0x200000ff, N("setsne"), PDST, T(src1), T(src2) },
    { 0x000000d2, 0x200000ff, N("btestn"), PRED, T(src1), T(src2) },
    { 0x200000d2, 0x200000ff, N("btestn"), PDST, T(src1), T(src2) },
	{ 0x000000d8, 0x200000ff, N("andsz"), PRED, T(dst), T(src1), T(src2) }, /* normal and, then set pred if result == 0 */
	{ 0x000000d9, 0x200000ff, N("orsz"), PRED, T(dst), T(src1), T(src2) },

	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0x34000043, 0x3c0000ff, OP32, N("nop") },
	{ 0x00000000, 0x20000000, OP32, T(m) },
	{ 0x20000000, 0x20000000, OP32, T(p), T(m) },

	{ 0, 0, OOPS },
};

static const struct disvariant vuc_vars[] = {
    "vp2", VP2,
    "vp3", VP3,
};

const struct disisa vp3m_isa_s = {
	tabroot,
	4,
	4,
	4,
    .vars = vuc_vars,
    .varsnum = sizeof vuc_vars / sizeof *vuc_vars,
};
