/*
 *
 * Copyright (C) 2009 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "dis.h"

/*
 * Code target field
 *
 * This field represents a code address and is used for branching and the
 * likes. Target is counted in 32-bit words from the start of microcode.
 */

static struct bitfield ctargoff = { { 8, 11 }, BF_UNSIGNED, 2 };
#define BTARG atombtarg, &ctargoff
#define CTARG atomctarg, &ctargoff

static struct bitfield imm4off = { 12, 4 };
static struct bitfield imm8off = { 8, 8 };
static struct bitfield bimm8off = { { 12, 4, 20, 4 } };
static struct bitfield bimmstoff = { 16, 8 };
static struct bitfield bimmstsoff = { 16, 4 };
#define IMM4 atomimm, &imm4off
#define IMM8 atomimm, &imm8off

/*
 * Register fields
 */

static struct sreg reg_sr[] = {
	{ 0, 0, SR_ZERO },
	{ -1 },
};
static struct sreg pred_sr[] = {
	{ 15, 0, SR_ONE },
	{ -1 },
};
static struct bitfield src1_bf = { 8, 4 };
static struct bitfield src2_bf = { 12, 4 };
static struct bitfield dst_bf = { 16, 4 };
static struct bitfield psrc1_bf = { 8, 4 };
static struct bitfield psrc2_bf = { 12, 4 };
static struct bitfield pdst_bf = { 16, 4 };
static struct bitfield pred_bf = { 20, 4 };
static struct reg src1_r = { &src1_bf, "r", .specials = reg_sr };
static struct reg src2_r = { &src2_bf, "r", .specials = reg_sr };
static struct reg dst_r = { &dst_bf, "r", .specials = reg_sr };
static struct reg adst_r = { &dst_bf, "a", .cool = 1 };
static struct reg idst_r = { &dst_bf, "i", .cool = 1 };
static struct reg psrc1_r = { &psrc1_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg psrc2_r = { &psrc2_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg pdst_r = { &pdst_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg pdstd_r = { &pdst_bf, "p", "d", .cool = 1, .specials = pred_sr };
static struct reg pred_r = { &pred_bf, "p", .cool = 1, .specials = pred_sr };
#define SRC1 atomreg, &src1_r
#define SRC2 atomreg, &src2_r
#define DST atomreg, &dst_r
#define ADST atomreg, &adst_r
#define IDST atomreg, &idst_r
#define PSRC1 atomreg, &psrc1_r
#define PSRC2 atomreg, &psrc2_r
#define PDST atomreg, &pdst_r
#define PDSTD atomreg, &pdstd_r
#define PRED atomreg, &pred_r

struct mem mem1c_m = { "D", 0, &src1_r, &bimm8off };
struct mem mem34_m = { "D", 0, &src1_r, 0, &src2_r };
struct mem memst_m = { "D", 0, &src1_r, &bimmstoff };
struct mem memsts_m = { "D", 0, &src1_r, &bimmstsoff };
struct mem ext_m = { "E", 0, 0, &src1_bf };
#define MEM1C atommem, &mem1c_m
#define MEMST atommem, &memst_m
#define MEMSTS atommem, &memsts_m
#define MEM34 atommem, &mem34_m
#define EXT atommem, &ext_m

static struct insn tabp[] = {
	{ -1, -1, 0x00f00000, 0x00f00000 },
	{ -1, -1, 0, 0, PRED },
};

static struct insn tabdst[] = {
	{ -1, -1, 0x00000000, 0x10000000, DST },
	{ -1, -1, 0x10000000, 0x11000000, ADST },
	{ -1, -1, 0x11000000, 0x11000000, IDST },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabsrc1[] = {
	{ -1, -1, 0x00000000, 0x04000000, SRC1 },
	{ -1, -1, 0x04000000, 0x05000000, OOPS },
	{ -1, -1, 0x05000000, 0x05000000, EXT },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabsrc2[] = {
	{ -1, -1, 0x00000000, 0x08000000, SRC2 },
	{ -1, -1, 0x08000000, 0x08000000, IMM4 },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabm[] = {
	{ -1, -1, 0x3c000000, 0x3c0000ff, N("bra"), BTARG },
	{ -1, -1, 0x3c000002, 0x3c0000ff, N("call"), CTARG },
	{ -1, -1, 0x34000003, 0x3c0000ff, N("ret") },
	{ -1, -1, 0x00000004, 0x000000ff, U("04") },
	{ -1, -1, 0x00000005, 0x000000ff, U("05") },
	{ -1, -1, 0x00000006, 0x000000ff, U("06") },
	{ -1, -1, 0x00000008, 0x000000ff, U("08") },
	{ -1, -1, 0x00000009, 0x000000ff, U("09") },
	{ -1, -1, 0x0000000a, 0x000000ff, U("0a") },
	{ -1, -1, 0x0000000b, 0x000000ff, U("0b") },
	{ -1, -1, 0x0000000e, 0x000000ff, U("0e") },

	{ -1, -1, 0x00000020, 0x000000ff, U("20") },
	{ -1, -1, 0x00000021, 0x000000ff, U("21") },
	{ -1, -1, 0x00000022, 0x000000ff, U("22") },
	{ -1, -1, 0x00000023, 0x000000ff, U("23") },
	{ -1, -1, 0x00000024, 0x000000ff, U("24") },
	{ -1, -1, 0x00000025, 0x000000ff, U("25") },
	{ -1, -1, 0x00000026, 0x000000ff, U("26") },
	{ -1, -1, 0x00000028, 0x000000ff, U("28") },
	{ -1, -1, 0x00000029, 0x000000ff, U("29") },
	{ -1, -1, 0x0000002a, 0x000000ff, U("2a") },
	{ -1, -1, 0x0000002b, 0x000000ff, U("2b") },
	{ -1, -1, 0x0000002c, 0x000000ff, U("2c") },
	{ -1, -1, 0x00000032, 0x000000ff, U("32") },

	{ -1, -1, 0x34000040, 0x3c0000ff, N("and"), PDST, PSRC1, PSRC2 },
	{ -1, -1, 0x34000041, 0x3c0000ff, N("or"), PDST, PSRC1, PSRC2 },
	{ -1, -1, 0x34000042, 0x3c0000ff, N("xor"), PDST, PSRC1, PSRC2 },
	{ -1, -1, 0x34000043, 0x3c0000ff, N("nop") },
	{ -1, -1, 0x34000044, 0x3c0000ff, U("44") },
	{ -1, -1, 0x34000045, 0x3c0000ff, U("45") },
	// XXX: these three seem to actually set two predicates, at least when p0 is the dst.
	{ -1, -1, 0x00000048, 0x000000ff, U("setsg"), PDSTD, T(src1), T(src2) },
	{ -1, -1, 0x00000049, 0x000000ff, N("setsl"), PDSTD, T(src1), T(src2) }, /* signed */
	{ -1, -1, 0x0000004a, 0x000000ff, N("setse"), PDSTD, T(src1), T(src2) },
	{ -1, -1, 0x0000004d, 0x000000ff, U("4d") },
	{ -1, -1, 0x0000004e, 0x000000ff, U("4e") },
	{ -1, -1, 0x00000052, 0x000000ff, U("52"), PDSTD, T(src1), T(src2) },
	{ -1, -1, 0x00000058, 0x000000ff, U("58"), PRED }, // PRED used as destination, apparently...
	{ -1, -1, 0x0000005c, 0x000000ff, U("5c") },

	{ -1, -1, 0x00000060, 0x000000ff, U("60"), T(dst) },
	{ -1, -1, 0x08000061, 0x080000ff, N("li"), T(dst), IMM8 },
	{ -1, -1, 0x00000064, 0x000000ff, N("add"), T(dst), T(src1), T(src2) },
	{ -1, -1, 0x00000065, 0x000000ff, N("sub"), T(dst), T(src1), T(src2) },
	{ -1, -1, 0x00000066, 0x000000ff, U("66"), T(dst) },
	{ -1, -1, 0x0000006c, 0x000000ff, U("6c"), T(dst) },
	{ -1, -1, 0x0000006e, 0x000000ff, U("6e"), T(dst), T(src1), T(src2) },
	{ -1, -1, 0x0000006f, 0x000000ff, U("6f"), T(dst), T(src1) },
	{ -1, -1, 0x00000070, 0x000000ff, U("70"), T(dst), T(src1), T(src2) },
	{ -1, -1, 0x00000071, 0x000000ff, U("71"), T(dst), T(src1), T(src2) },
	{ -1, -1, 0x00000074, 0x000000ff, U("74"), T(dst) },
	{ -1, -1, 0x00000075, 0x000000ff, N("shl"), T(dst), T(src1), T(src2) },
	{ -1, -1, 0x00000076, 0x000000ff, N("shr"), T(dst), T(src1), T(src2) },
	{ -1, -1, 0x00000077, 0x000000ff, N("sar"), T(dst), T(src1), T(src2) },
	{ -1, -1, 0x00000078, 0x000000ff, N("and"), T(dst), T(src1), T(src2) },
	{ -1, -1, 0x00000079, 0x000000ff, N("or"), T(dst), T(src1), T(src2) },
	{ -1, -1, 0x0000007a, 0x000000ff, N("xor"), T(dst), T(src1), T(src2) },
	{ -1, -1, 0x0000007b, 0x000000ff, U("7b"), T(dst), T(src1), T(src2) },
	{ -1, -1, 0x0000007c, 0x000000ff, U("7c"), T(dst) },
	{ -1, -1, 0x0000007d, 0x000000ff, U("7d"), T(dst) },
	{ -1, -1, 0x0000007e, 0x000000ff, U("7e"), T(dst) },

	{ -1, -1, 0x1c000080, 0x3c0000ff, N("st"), MEMST, SRC2 },
	{ -1, -1, 0x3c000080, 0x3c0000ff, N("st"), MEMSTS, SRC2 },
	{ -1, -1, 0x1c000081, 0x3c0000ff, N("ld"), DST, MEM1C },
	{ -1, -1, 0x34000081, 0x3c0000ff, N("ld"), DST, MEM34 },
	{ -1, -1, 0x00000083, 0x000000ff, U("83") },
	{ -1, -1, 0x00000084, 0x000000ff, U("84") },
	{ -1, -1, 0x00000089, 0x000000ff, U("89") },
	{ -1, -1, 0x1c00008a, 0x3c0000ff, U("8a"), MEMST, SRC2 },
	{ -1, -1, 0x0000008c, 0x000000ff, U("8c") },
	{ -1, -1, 0x0000008d, 0x000000ff, U("8d") },
	{ -1, -1, 0x0000008e, 0x000000ff, U("8e") },
	{ -1, -1, 0x0000008f, 0x000000ff, U("8f") },

	{ -1, -1, 0x000000a0, 0x000000ff, U("a0") },
	{ -1, -1, 0x000000a1, 0x000000ff, U("a1") },
	{ -1, -1, 0x000000a2, 0x000000ff, U("a2") },
	{ -1, -1, 0x000000a4, 0x000000ff, U("a4") },
	{ -1, -1, 0x000000a8, 0x000000ff, U("a8") },
	{ -1, -1, 0x000000ac, 0x000000ff, U("ac") },

	{ -1, -1, 0x000000c9, 0x000000ff, N("setul"), PDSTD, T(src1), T(src2) }, /* signed */
	{ -1, -1, 0x000000ca, 0x000000ff, N("setue"), PDSTD, T(src1), T(src2) },
	{ -1, -1, 0x000000d2, 0x000000ff, U("d2"), PDST },
	{ -1, -1, 0x000000d8, 0x000000ff, U("d8"), PRED },

	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ -1, -1, 0x34000043, 0x3c0000ff, OP32, N("nop") },
	{ -1, -1, 0x00000000, 0x20000000, OP32, T(m) },
	{ -1, -1, 0x20000000, 0x20000000, OP32, T(p), T(m) },

	{ -1, -1, 0, 0, OOPS },
};

static struct disisa vp3m_isa_s = {
	tabroot,
	4,
	4,
	1,
};

struct disisa *vp3m_isa = &vp3m_isa_s;
