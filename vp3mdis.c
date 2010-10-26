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
	{ AP, 0x00f00000, 0x00f00000 },
	{ AP, 0, 0, PRED },
};

static struct insn tabm[] = {
	{ AP, 0x3c000000, 0xfc0000ff, T(p), N("bra"), BTARG },
	{ AP, 0x3c000002, 0xfc0000ff, T(p), N("call"), CTARG },
	{ AP, 0x34000003, 0xfc0000ff, T(p), N("ret") },
	{ AP, 0x00000004, 0x000000ff, U("04") },
	{ AP, 0x00000005, 0x000000ff, U("05") },
	{ AP, 0x00000006, 0x000000ff, U("06") },
	{ AP, 0x00000008, 0x000000ff, U("08") },
	{ AP, 0x00000009, 0x000000ff, U("09") },
	{ AP, 0x0000000a, 0x000000ff, U("0a") },
	{ AP, 0x0000000b, 0x000000ff, U("0b") },
	{ AP, 0x0000000e, 0x000000ff, U("0e") },

	{ AP, 0x00000020, 0x000000ff, U("20") },
	{ AP, 0x00000021, 0x000000ff, U("21") },
	{ AP, 0x00000022, 0x000000ff, U("22") },
	{ AP, 0x00000024, 0x000000ff, U("24") },
	{ AP, 0x00000025, 0x000000ff, U("25") },
	{ AP, 0x00000026, 0x000000ff, U("26") },
	{ AP, 0x00000028, 0x000000ff, U("28") },
	{ AP, 0x00000029, 0x000000ff, U("29") },
	{ AP, 0x0000002a, 0x000000ff, U("2a") },
	{ AP, 0x0000002b, 0x000000ff, U("2b") },
	{ AP, 0x0000002c, 0x000000ff, U("2c") },
	{ AP, 0x00000032, 0x000000ff, U("32") },

	{ AP, 0x34000040, 0xfc0000ff, T(p), N("and"), PDST, PSRC1, PSRC2 },
	{ AP, 0x34000041, 0xfc0000ff, T(p), N("or"), PDST, PSRC1, PSRC2 },
	{ AP, 0x34000042, 0xfc0000ff, T(p), N("xor"), PDST, PSRC1, PSRC2 },
	{ AP, 0x34000043, 0xfc0000ff, N("nop") },
	{ AP, 0x34000044, 0xfc0000ff, U("44") },
	{ AP, 0x34000045, 0xfc0000ff, U("45") },
	// XXX: these three seem to actually set two predicates, at least when p0 is the dst.
	{ AP, 0x00000048, 0x000000ff, U("48"), PDST },
	{ AP, 0x00000049, 0x000000ff, U("49"), PDST },
	{ AP, 0x0000004a, 0x000000ff, U("4a"), PDST },
	{ AP, 0x0000004d, 0x000000ff, U("4d") },
	{ AP, 0x0000004e, 0x000000ff, U("4e") },
	{ AP, 0x00000052, 0x000000ff, U("52"), PDST },
	{ AP, 0x00000058, 0x000000ff, U("58"), PRED }, // PRED used as destination, apparently...
	{ AP, 0x0000005c, 0x000000ff, U("5c") },

	{ AP, 0x00000060, 0x000000ff, T(p), U("60") },
	{ AP, 0x00000061, 0x000000ff, U("61") },
	{ AP, 0x00000064, 0x000000ff, T(p), U("64") },
	{ AP, 0x00000065, 0x000000ff, T(p), U("65") },
	{ AP, 0x00000066, 0x000000ff, U("66") },
	{ AP, 0x0000006c, 0x000000ff, U("6c") },
	{ AP, 0x0000006e, 0x000000ff, U("6e") },
	{ AP, 0x0000006f, 0x000000ff, U("6f") },
	{ AP, 0x00000070, 0x000000ff, T(p), U("70") },
	{ AP, 0x00000071, 0x000000ff, U("71") },
	{ AP, 0x00000074, 0x000000ff, U("74") },
	{ AP, 0x00000075, 0x000000ff, U("75") },
	{ AP, 0x00000076, 0x000000ff, U("76") },
	{ AP, 0x00000077, 0x000000ff, U("77") },
	{ AP, 0x00000078, 0x000000ff, U("78") },
	{ AP, 0x00000079, 0x000000ff, U("79") },
	{ AP, 0x0000007a, 0x000000ff, U("7a") },
	{ AP, 0x0000007b, 0x000000ff, U("7b") },
	{ AP, 0x0000007c, 0x000000ff, U("7c") },
	{ AP, 0x0000007d, 0x000000ff, U("7d") },
	{ AP, 0x0000007e, 0x000000ff, U("7e") },

	{ AP, 0x00000080, 0x000000ff, U("80") },
	{ AP, 0x00000081, 0x000000ff, U("81") },
	{ AP, 0x00000083, 0x000000ff, U("83") },
	{ AP, 0x00000084, 0x000000ff, U("84") },
	{ AP, 0x00000089, 0x000000ff, U("89") },
	{ AP, 0x0000008a, 0x000000ff, U("8a") },
	{ AP, 0x0000008c, 0x000000ff, U("8c") },
	{ AP, 0x0000008d, 0x000000ff, U("8d") },
	{ AP, 0x0000008e, 0x000000ff, U("8e") },
	{ AP, 0x0000008f, 0x000000ff, U("8f") },

	{ AP, 0x000000a0, 0x000000ff, U("a0") },
	{ AP, 0x000000a1, 0x000000ff, U("a1") },
	{ AP, 0x000000a2, 0x000000ff, U("a2") },
	{ AP, 0x000000a4, 0x000000ff, U("a4") },
	{ AP, 0x000000a8, 0x000000ff, U("a8") },
	{ AP, 0x000000ac, 0x000000ff, U("ac") },

	{ AP, 0x000000c9, 0x000000ff, U("c9"), PDST },
	{ AP, 0x000000ca, 0x000000ff, T(p), U("ca"), PDST },
	{ AP, 0x000000d2, 0x000000ff, U("d2") },
	{ AP, 0x000000d8, 0x000000ff, U("d8") },

	{ AP, 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ -1, 0, 0, OP32, T(m) },
};

static struct disisa vp3m_isa_s = {
	tabroot,
	4,
	4,
	1,
};

struct disisa *vp3m_isa = &vp3m_isa_s;
