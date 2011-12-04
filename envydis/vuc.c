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
static struct bitfield bimmldoff = { { 12, 4, 20, 6 } };
static struct bitfield bimmldsoff = { { 12, 4, 24, 2 } };
static struct bitfield bimmstoff = { 16, 10 };
static struct bitfield bimmstsoff = { 16, 4, 24, 2 };
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
static struct sreg sreg_sr[] = {
    { 8, "pc" },
    { 12, "arthi" },
    { 13, "artlo" },
    { 14, "pred" },
    { 15, "cnt" },
    { -1 },
};

static struct bitfield src1_bf = { 8, 4 };
static struct bitfield srsrc_bf = { 8, 4, 24, 2 };
static struct bitfield src2_bf = { 12, 4 };
static struct bitfield dst_bf = { 16, 4 };
static struct bitfield srdst_bf = { 16, 4, 24, 2 };
static struct bitfield psrc1_bf = { 8, 4 };
static struct bitfield psrc2_bf = { 12, 4 };
static struct bitfield pdst_bf = { 16, 4 };
static struct bitfield pred_bf = { 20, 4 };
static struct reg src1_r = { &src1_bf, "r", .specials = reg_sr };
static struct reg src2_r = { &src2_bf, "r", .specials = reg_sr };
static struct reg dst_r = { &dst_bf, "r", .specials = reg_sr };
static struct reg srdst_r = { &srdst_bf, "sr", .specials = sreg_sr, .cool = 1 };
static struct reg srsrc_r = { &srsrc_bf, "sr", .specials = sreg_sr, .cool = 1 };
static struct reg psrc1_r = { &psrc1_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg psrc2_r = { &psrc2_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg pdst_r = { &pdst_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg pred_r = { &pred_bf, "p", .cool = 1, .specials = pred_sr };
#define SRC1 atomreg, &src1_r
#define SRC2 atomreg, &src2_r
#define DST atomreg, &dst_r
#define SRSRC atomreg, &srsrc_r
#define SRDST atomreg, &srdst_r
#define PSRC1 atomreg, &psrc1_r
#define PSRC2 atomreg, &psrc2_r
#define PDST atomreg, &pdst_r
#define PRED atomreg, &pred_r

static struct mem dmemldi_m = { "D", 0, &src1_r, &bimmldoff };
static struct mem dmemldis_m = { "D", 0, &src1_r, &bimmldsoff };
static struct mem dmemldr_m = { "D", 0, &src1_r, 0, &src2_r };
static struct mem dmemsti_m = { "D", 0, &src1_r, &bimmstoff };
static struct mem dmemstis_m = { "D", 0, &src1_r, &bimmstsoff };
static struct mem dmemstr_m = { "D", 0, &src1_r, 0, &dst_r };
#define DMEMLDI atommem, &dmemldi_m
#define DMEMLDIS atommem, &dmemldis_m
#define DMEMLDR atommem, &dmemldr_m
#define DMEMSTI atommem, &dmemsti_m
#define DMEMSTIS atommem, &dmemstis_m
#define DMEMSTR atommem, &dmemstr_m

static struct mem ememldi_m = { "E", 0, &src1_r, &bimmldoff };
static struct mem ememldis_m = { "E", 0, &src1_r, &bimmldsoff };
static struct mem ememldr_m = { "E", 0, &src1_r, 0, &src2_r };
static struct mem ememsti_m = { "E", 0, &src1_r, &bimmstoff };
static struct mem ememstis_m = { "E", 0, &src1_r, &bimmstsoff };
static struct mem ememstr_m = { "E", 0, &src1_r, 0, &dst_r };
#define EMEMLDI atommem, &ememldi_m
#define EMEMLDIS atommem, &ememldis_m
#define EMEMLDR atommem, &ememldr_m
#define EMEMSTI atommem, &ememsti_m
#define EMEMSTIS atommem, &ememstis_m
#define EMEMSTR atommem, &ememstr_m

static struct mem fmemldi_m = { "F", 0, &src1_r, &bimmldoff };
static struct mem fmemldis_m = { "F", 0, &src1_r, &bimmldsoff };
static struct mem fmemldr_m = { "F", 0, &src1_r, 0, &src2_r };
#define FMEMLDI atommem, &fmemldi_m
#define FMEMLDIS atommem, &fmemldis_m
#define FMEMLDR atommem, &fmemldr_m

static struct mem gmemldi_m = { "G", 0, &src1_r, &bimmldoff };
static struct mem gmemldis_m = { "G", 0, &src1_r, &bimmldsoff };
static struct mem gmemldr_m = { "G", 0, &src1_r, 0, &src2_r };
#define GMEMLDI atommem, &gmemldi_m
#define GMEMLDIS atommem, &gmemldis_m
#define GMEMLDR atommem, &gmemldr_m

static struct insn tabp[] = {
	{ 0x00f00000, 0x00f00000 },
	{ 0, 0, PRED },
};

static struct insn tabdst[] = {
	{ 0x00000000, 0x10000000, DST },
	{ 0x10000000, 0x10000000, SRDST },
	{ 0, 0, OOPS },
};

static struct insn tabsrc1[] = {
	{ 0x00000000, 0x04000000, SRC1 },
	{ 0x04000000, 0x04000000, SRSRC },
	{ 0, 0, OOPS },
};

static struct insn tabsrc2[] = {
	{ 0x00000000, 0x08000000, SRC2 },
	{ 0x08000000, 0x1c000000, IMM6 },
	{ 0x18000000, 0x1c000000, IMM4 },
	{ 0x0c000000, 0x0c000000, IMM4 },
	{ 0, 0, OOPS },
};

static struct insn tabarithsrc2[] = {
    { 0x00000000, 0x08000000, SRC2 },
    { 0x08000000, 0x08000000, IMM4 },
    { 0, 0, OOPS },
};

// Tables describing parameters for "set" functions

static struct insn tabpdst[] = {
    { 0x20000000, 0x20000000, PDST },
    { 0x00000000, 0x20000000, PRED },
    { 0, 0, OOPS },
};

static struct insn tabovr[] = {
    { 0x3c000000, 0x3c0000ff, N("bra"), BTARG },
    { 0x3c000002, 0x3c0000ff, N("call"), CTARG },
    { 0x34000003, 0x3c0000ff, N("ret") },
    { 0x14000004, 0x140000ff, U("04") },
    { 0x14000005, 0x140000ff, U("05"), T(src2) },
    { 0x14000006, 0x140000ff, U("06"), T(src2) },

    { 0x14000020, 0x140000ff, U("20") },
    { 0x14000021, 0x140000ff, U("21") },
    { 0x14000022, 0x140000ff, U("22") },

    { 0x14000024, 0x140000ff, U("24") },
    { 0x14000025, 0x140000ff, U("25") },
    { 0x14000026, 0x140000ff, U("26") },
    { 0x14000028, 0x140000ff, U("28") },
    { 0x14000029, 0x140000ff, U("29") },
    { 0x1400002a, 0x140000ff, U("2a") },
    { 0x1400002b, 0x140000ff, U("2b") },
    { 0x1400002c, 0x140000ff, U("2c") },
    
    { 0x34000040, 0x3c0000ff, N("and"), PDST, PSRC1, PSRC2 },
    { 0x34000041, 0x3c0000ff, N("or"), PDST, PSRC1, PSRC2 },
    { 0x34000042, 0x3c0000ff, N("xor"), PDST, PSRC1, PSRC2 },
    { 0x34000043, 0x3c0000ff, N("nop") },
    { 0x34000044, 0x3c0000ff, N("andn"), PDST, PSRC1, PSRC2 },
    { 0x34000045, 0x3c0000ff, N("orn"), PDST, PSRC1, PSRC2 },
    { 0x34000046, 0x3c0000ff, N("xorn"), PDST, PSRC1, PSRC2 },
    
    { 0x14000048, 0x140000ff, N("setg"), T(pdst), PSRC1, PSRC2 }, // signed
    { 0x14000049, 0x140000ff, N("setle"), T(pdst), PSRC1, PSRC2 },
    { 0x1400004a, 0x140000ff, N("sete"), T(pdst), PSRC1, PSRC2 },

    { 0x1400004d, 0x140000ff, N("orsetnand"), T(pdst), PSRC1, PSRC2 },
    { 0x1400004e, 0x140000ff, N("orsetneq"), T(pdst), PSRC1, PSRC2 },
    
    { 0x1c000080, 0x3c0000ff, N("st"), DMEMSTI, SRC2 },
    { 0x3c000080, 0x3c0000ff, N("st"), DMEMSTIS, SRC2 },
    { 0x14000080, 0x1c0000ff, N("st"), DMEMSTR, SRC2 },
    { 0x14000081, 0x1c0000ff, N("ld"), DST, DMEMLDR },
    { 0x1c000081, 0x3c0000ff, N("ld"), DST, DMEMLDI },
    { 0x3c000081, 0x3c0000ff, N("ld"), DST, DMEMLDIS },
    { 0x14000083, 0x1c0000ff, U("83"), DST, FMEMLDR },
    { 0x1c000083, 0x3c0000ff, U("83"), DST, FMEMLDI },
    { 0x3c000083, 0x3c0000ff, U("83"), DST, FMEMLDIS },
    { 0x1c000084, 0x3c0000ff, U("84"), DMEMSTI, SRC2 },
    { 0x3c000084, 0x3c0000ff, U("84"), DMEMSTIS, SRC2 },
    { 0x14000084, 0x1c0000ff, U("84"), DMEMSTR, SRC2 }, // store?
    { 0x14000089, 0x1c0000ff, U("89"), DST, GMEMLDR },
    { 0x1c000089, 0x3c0000ff, U("89"), DST, GMEMLDI },
    { 0x3c000089, 0x3c0000ff, U("89"), DST, GMEMLDIS },
    { 0x1c00008a, 0x3c0000ff, U("8a"), DMEMSTI, SRC2 }, // store?
    { 0x3c00008a, 0x3c0000ff, U("8a"), DMEMSTIS, SRC2 },
    { 0x1400008a, 0x1c0000ff, U("8a"), DMEMSTR, SRC2 },
    { 0x1c00008c, 0x3c0000ff, U("8c"), DMEMSTI, SRC2 }, // store? space really unknown
    { 0x3c00008c, 0x3c0000ff, U("8c"), DMEMSTIS, SRC2 },
    { 0x3400008c, 0x1c0000ff, U("8c"), DMEMSTR, SRC2 },
    { 0x1400008d, 0x140000ff, U("8d"), DST }, // load?
    { 0x1c00008e, 0x3c0000ff, N("iowr"), EMEMSTI, SRC2 },
    { 0x3c00008e, 0x3c0000ff, N("iowr"), EMEMSTIS, SRC2 },
    { 0x1400008e, 0x1c0000ff, N("iowr"), EMEMSTR, SRC2 },
    { 0x1400008f, 0x1c0000ff, N("iord"), DST, EMEMLDR },
    { 0x1c00008f, 0x3c0000ff, N("iord"), DST, EMEMLDI },
    { 0x3c00008f, 0x3c0000ff, N("iord"), DST, EMEMLDIS },

    // arithmetic subsystem block
    { 0x140000a0, 0x140000ff, N("mul"), SRC1, T(arithsrc2) },
    { 0x140000a1, 0x140000ff, N("muls"), SRC1, T(arithsrc2) },
    { 0x140000a2, 0x140000ff, N("shift"), T(arithsrc2) },
    { 0x140000a4, 0x140000ff, U("a4"), T(src2) },
    { 0x140000a8, 0x140000ff, U("a8"), T(src2) },
    { 0x140000ac, 0x140000ff, U("ac"), T(src2) },
    
    { 0, 0, OOPS },
};

static struct insn tabm[] = {
    // Most frequently used form first for asm, later other accepted forms for disasm/analysis.
    { 0x0000006e, 0x000000ff, N("sex"), T(dst), T(src1), T(src2) }, // like fuc insn of the same name
    { 0x00000064, 0x000000ff, N("add"), T(dst), T(src1), T(src2) },
    { 0x00000065, 0x000000ff, N("sub"), T(dst), T(src1), T(src2) },
    { 0x00000066, 0x000000ff, N("subr"), T(dst), T(src1), T(src2), .vartype = VP2 },
    { 0x0000006c, 0x000000ff, N("minsz"), T(dst), T(src1), T(src2) }, // (a > b) ? b : max(a, 0)
    { 0x0000006d, 0x000000ff, N("clampsex"), T(dst), T(src1), T(src2) }, // clamp to -2^b..2^b-1
    { 0x00000052, 0x000000ff, N("btest"), T(pdst), T(src1), T(src2) },
    { 0x000000d2, 0x000000ff, N("btestn"), T(pdst), T(src1), T(src2) },
    
    // all accepted forms. 0xe0 seems to be flags
    { 0x0000000e, 0x0000001f, N("sex"), T(dst), T(src1), T(src2) },
    { 0x00000004, 0x0000001f, N("add"), T(dst), T(src1), T(src2) },
    { 0x00000005, 0x0000001f, N("sub"), T(dst), T(src1), T(src2) },
    { 0x00000006, 0x0000001f, N("subr"), T(dst), T(src1), T(src2), .vartype = VP2 },
    
    
    { 0x0000000c, 0x0000001f, N("minsz"), T(dst), T(src1), T(src2) },
    { 0x0000000d, 0x0000001f, N("clampsex"), T(dst), T(src1), T(src2) },
    
    // other accepted forms for instructions that can't be masked easily
    { 0x00000032, 0x000000ff, N("btest"), T(pdst), T(src1), T(src2) },
    { 0x000000c2, 0x000000ff, N("btestn"), T(pdst), T(src1), T(src2) },
    

	{ 0x00000008, 0x000000ff, N("andsetsg"), T(pdst), T(src1), T(src2) },
	{ 0x00000009, 0x000000ff, U("09"), T(pdst), T(src1), T(src2) },
	{ 0x0000000a, 0x000000ff, U("0a"), T(pdst), T(src1), T(src2) },
	{ 0x0000000b, 0x000000ff, U("0b"), T(pdst), T(src1), T(src2) },

    // never used by the blob in the below form, probably opcode's "proper" function is the override
    { 0x00000028, 0x000000ff, N("orsetsg"), T(pdst), T(src1), T(src2) }, /* PDST |= (src1 > src2) */
    { 0x00000029, 0x000000ff, N("orsetsl"), T(pdst), T(src1), T(src2) }, /* PDST |= (src1 < src2) */
    { 0x0000002a, 0x000000ff, N("orsetse"), T(pdst), T(src1), T(src2) }, /* PDST |= (src1 == src2) */
	

    { 0x00000048, 0x000000ff, N("setsg"), T(pdst), T(src1), T(src2) }, // signed
    { 0x00000049, 0x000000ff, N("setsl"), T(pdst), T(src1), T(src2) }, /* signed */
    { 0x0000004a, 0x000000ff, N("setse"), T(pdst), T(src1), T(src2) },

    
	{ 0x00000058, 0x200000ff, N("andsnz"), PRED, T(dst), T(src1), T(src2) }, /* normal and, then set pred if result != 0 */
	{ 0x00000059, 0x200000ff, N("orsnz"), PRED, T(dst), T(src1), T(src2) },
	{ 0x0000005c, 0x200000ff, U("5c"), PRED, T(dst), T(src1), T(src2) },

	{ 0x00000060, 0x200000ff, N("slct"), T(dst), PRED, T(src1), T(src2) }, // dst = PRED ? src1 : src2
	{ 0x00000061, 0x080000ff, N("mov"), T(dst), T(src2) },
	{ 0x18000061, 0x180000ff, N("mov"), SRDST, IMM12 },
	{ 0x08000061, 0x180000ff, N("mov"), DST, IMM14 },
	
	{ 0x00000067, 0x000000ff, N("clear"), T(dst), .vartype = VP2 },
	{ 0x00000066, 0x000000ff, N("avgs"), T(dst), T(src1), T(src2), .vartype = VP3 }, // (a+b)/2, rounding UP, signed
	{ 0x00000067, 0x000000ff, N("avgu"), T(dst), T(src1), T(src2), .vartype = VP3 }, // (a+b)/2, rounding UP, unsigned
	

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

    { 0x000000c8, 0x000000ff, N("setsle"), T(pdst), T(src1), T(src2) },
    { 0x000000c9, 0x000000ff, N("setsge"), T(pdst), T(src1), T(src2) },
    { 0x000000ca, 0x000000ff, N("setsne"), T(pdst), T(src1), T(src2) },
    
	{ 0x000000d8, 0x200000ff, N("andsz"), PRED, T(dst), T(src1), T(src2) }, /* normal and, then set pred if result == 0 */
	{ 0x000000d9, 0x200000ff, N("orsz"), PRED, T(dst), T(src1), T(src2) },

	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0x34000043, 0x3c0000ff, OP32, N("nop") },
	{ 0x14000000, 0x34000000, OP32, T(ovr) },
    { 0x34000000, 0x34000000, OP32, T(p), T(ovr) },
	{ 0x00000000, 0x20000000, OP32, T(m) },
	{ 0x20000000, 0x20000000, OP32, T(p), T(m) },

	{ 0, 0, OOPS },
};

static const struct disvariant vuc_vars[] = {
    "vp2", VP2,
    "vp3", VP3,
};

const struct disisa vuc_isa_s = {
	tabroot,
	4,
	4,
	4,
    .vars = vuc_vars,
    .varsnum = sizeof vuc_vars / sizeof *vuc_vars,
};
