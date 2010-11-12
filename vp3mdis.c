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

/*
 * Register fields
 */

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
static struct reg src1_r = { &src1_bf, "r" };
static struct reg src2_r = { &src2_bf, "r" };
static struct reg dst_r = { &dst_bf, "r" };
static struct reg psrc1_r = { &psrc1_bf, "p", .specials = pred_sr };
static struct reg psrc2_r = { &psrc2_bf, "p", .specials = pred_sr };
static struct reg pdst_r = { &pdst_bf, "p", .specials = pred_sr };
static struct reg pred_r = { &pred_bf, "p", .specials = pred_sr };
#define SRC1 atomreg, &src1_r
#define SRC2 atomreg, &src2_r
#define DST atomreg, &dst_r
#define PSRC1 atomreg, &psrc1_r
#define PSRC2 atomreg, &psrc2_r
#define PDST atomreg, &pdst_r
#define PRED atomreg, &pred_r

static struct insn tabp[] = {
	{ -1, -1, 0x00f00000, 0x00f00000 },
	{ -1, -1, 0, 0, PRED },
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
	{ -1, -1, 0x00000048, 0x000000ff, U("48"), PDST },
	{ -1, -1, 0x00000049, 0x000000ff, U("49"), PDST },
	{ -1, -1, 0x0000004a, 0x000000ff, U("4a"), PDST },
	{ -1, -1, 0x0000004d, 0x000000ff, U("4d") },
	{ -1, -1, 0x0000004e, 0x000000ff, U("4e") },
	{ -1, -1, 0x00000052, 0x000000ff, U("52"), PDST },
	{ -1, -1, 0x00000058, 0x000000ff, U("58"), PRED }, // PRED used as destination, apparently...
	{ -1, -1, 0x0000005c, 0x000000ff, U("5c") },

	{ -1, -1, 0x00000060, 0x000000ff, U("60") },
	{ -1, -1, 0x00000061, 0x000000ff, U("61") },
	{ -1, -1, 0x00000064, 0x000000ff, U("64") },
	{ -1, -1, 0x00000065, 0x000000ff, U("65") },
	{ -1, -1, 0x00000066, 0x000000ff, U("66") },
	{ -1, -1, 0x0000006c, 0x000000ff, U("6c") },
	{ -1, -1, 0x0000006e, 0x000000ff, U("6e") },
	{ -1, -1, 0x0000006f, 0x000000ff, U("6f") },
	{ -1, -1, 0x00000070, 0x000000ff, U("70") },
	{ -1, -1, 0x00000071, 0x000000ff, U("71") },
	{ -1, -1, 0x00000074, 0x000000ff, U("74") },
	{ -1, -1, 0x00000075, 0x000000ff, U("75") },
	{ -1, -1, 0x00000076, 0x000000ff, U("76") },
	{ -1, -1, 0x00000077, 0x000000ff, U("77") },
	{ -1, -1, 0x00000078, 0x000000ff, U("78") },
	{ -1, -1, 0x00000079, 0x000000ff, U("79") },
	{ -1, -1, 0x0000007a, 0x000000ff, U("7a") },
	{ -1, -1, 0x0000007b, 0x000000ff, U("7b") },
	{ -1, -1, 0x0000007c, 0x000000ff, U("7c") },
	{ -1, -1, 0x0000007d, 0x000000ff, U("7d") },
	{ -1, -1, 0x0000007e, 0x000000ff, U("7e") },

	{ -1, -1, 0x00000080, 0x000000ff, U("80") },
	{ -1, -1, 0x00000081, 0x000000ff, U("81") },
	{ -1, -1, 0x00000083, 0x000000ff, U("83") },
	{ -1, -1, 0x00000084, 0x000000ff, U("84") },
	{ -1, -1, 0x00000089, 0x000000ff, U("89") },
	{ -1, -1, 0x0000008a, 0x000000ff, U("8a") },
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

	{ -1, -1, 0x000000c9, 0x000000ff, U("c9"), PDST },
	{ -1, -1, 0x000000ca, 0x000000ff, U("ca"), PDST },
	{ -1, -1, 0x000000d2, 0x000000ff, U("d2") },
	{ -1, -1, 0x000000d8, 0x000000ff, U("d8") },

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
