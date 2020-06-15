/*
 * Copyright (C) 2010-2011 Marcelina Kościelnicka <mwk@0x04.net>
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
 * Options:
 *
 * - core [complete]
 * - code density [complete]
 * - loop [complete]
 * - 16-bit integer multiply [complete]
 * - misc operations [minmax] [complete]
 * - boolean [complete]
 * - exception [complete]
 * - interrupt [complete]
 * - windowed register [complete]
 * - debug [complete]
 */

/*
 * Code target fields
 */

static struct rbitfield jtargoff = { { 6, 18 }, BF_SIGNED, 0, 1, 4 };
static struct rbitfield btarg8off = { { 16, 8 }, BF_SIGNED, 0, 1, 4 };
static struct rbitfield btarg12off = { { 12, 12 }, BF_SIGNED, 0, 1, 4 };
static struct rbitfield ltargoff = { { 16, 8 }, BF_UNSIGNED, 0, 1, 4 };
static struct rbitfield btarg6off = { { 12, 4, 4, 2 }, BF_UNSIGNED, 0, 1, 4 };
static struct rbitfield ctargoff = { { 6, 18 }, BF_SIGNED, 2, 1, 4 };
#define JTARG atombtarg, &jtargoff
#define BTARG8 atombtarg, &btarg8off
#define BTARG12 atombtarg, &btarg12off
#define LTARG atombtarg, &ltargoff
#define BTARG6 atombtarg, &btarg6off
#define CTARG atomctarg, &ctargoff

/*
 * Register fields
 */

static struct sreg sreg_sr[] = {
	{ 0x00, "lbeg" },
	{ 0x01, "lend" },
	{ 0x02, "lcount" },
	{ 0x03, "sar" },
	{ 0x04, "br" },
	{ 0x48, "windowbase" },
	{ 0x49, "windowstart" },
	{ 0x60, "ibreakenable" },
	{ 0x68, "ddr" },
	{ 0x80, "ibreaka0" },
	{ 0x81, "ibreaka1" },
	{ 0x90, "dbreaka0" },
	{ 0x91, "dbreaka1" },
	{ 0xa0, "dbreakc0" },
	{ 0xa1, "dbreakc1" },
	{ 0xb1, "epc1" },
	{ 0xb2, "epc2" },
	{ 0xb3, "epc3" },
	{ 0xb4, "epc4" },
	{ 0xb5, "epc5" },
	{ 0xb6, "epc6" },
	{ 0xc0, "depc" },
	{ 0xc2, "eps2" },
	{ 0xc3, "eps3" },
	{ 0xc4, "eps4" },
	{ 0xc5, "eps5" },
	{ 0xc6, "eps6" },
	{ 0xd1, "excsave1" },
	{ 0xd2, "excsave2" },
	{ 0xd3, "excsave3" },
	{ 0xd4, "excsave4" },
	{ 0xd5, "excsave5" },
	{ 0xd6, "excsave6" },
	{ 0xe2, "interrupt" }, /* and intset */
	{ 0xe3, "intclear" },
	{ 0xe4, "intenable" },
	{ 0xe6, "ps" },
	{ 0xe8, "exccause" },
	{ 0xe9, "debugcause" },
	{ 0xea, "ccount" },
	{ 0xec, "icount" },
	{ 0xed, "icountlevel" },
	{ 0xee, "excvaddr" },
	{ 0xf0, "ccompare0" },
	{ 0xf1, "ccompare1" },
	{ -1 },
};

static struct bitfield t_bf = { 4, 4 };
static struct bitfield s_bf = { 8, 4 };
static struct bitfield r_bf = { 12, 4 };
static struct bitfield sreg_bf = { 8, 8 };

static struct reg at_r = { &t_bf, "a" };
static struct reg as_r = { &s_bf, "a" };
static struct reg ar_r = { &r_bf, "a" };
static struct reg bt_r = { &t_bf, "b" };
static struct reg bs_r = { &s_bf, "b" };
static struct reg br_r = { &r_bf, "b" };
static struct reg bsq_r = { &s_bf, "b", "q" };
static struct reg bso_r = { &s_bf, "b", "o" };
static struct reg sreg_r = { &sreg_bf, "s", .specials = sreg_sr, .always_special = 1 };

#define REGT atomreg, &at_r
#define REGS atomreg, &as_r
#define REGR atomreg, &ar_r
#define BRT atomreg, &bt_r
#define BRS atomreg, &bs_r
#define BRR atomreg, &br_r
#define BRSQ atomreg, &bsq_r
#define BRSO atomreg, &bso_r
#define SREG atomreg, &sreg_r

/*
 * Immediate fields
 */

static ull b4constlut[] = {
	-1, 1, 2, 3,
	4, 5, 6, 7,
	8, 10, 12, 16,
	32, 64, 128, 256,
};
static ull b4constulut[] = {
	0x8000, 0x10000, 2, 3,
	4, 5, 6, 7,
	8, 10, 12, 16,
	32, 64, 128, 256, 
};
static ull addinlut[] = {
	-1, 1, 2, 3,
	4, 5, 6, 7,
	8, 9, 10, 11,
	12, 13, 14, 15,
};
static ull maskimmlut[] = {
	1, 2, 3, 4,
	5, 6, 7, 8,
	9, 10, 11, 12,
	13, 14, 15, 16,
};
static ull sllilut[] = {
	32, 31, 30, 29,
	28, 27, 26, 25,
	24, 23, 22, 21,
	20, 19, 18, 17,
	16, 15, 14, 13,
	12, 11, 10, 9,
	8, 7, 6, 5,
	4, 3, 2, 1,
};

static struct bitfield imm8soff = { { 16, 8 }, BF_SIGNED };
static struct bitfield imm8s8off = { { 16, 8 }, BF_SIGNED, 8 };
static struct bitfield imm12m8off = { { 12, 12 }, BF_UNSIGNED, 3 };
static struct bitfield rotoff = { { 4, 2 }, BF_UNSIGNED, 2 };
static struct bitfield rotwoff = { { 4, 4 }, BF_SIGNED };
static struct bitfield srlioff = { 8, 4 };
static struct bitfield waitioff = { 8, 4 };
static struct bitfield break8off = { 8, 4 };
static struct bitfield break4off = { 4, 4 };
static struct bitfield b4constoff = { { 12, 4 }, BF_LUT, .lut = b4constlut };
static struct bitfield b4constuoff = { { 12, 4 }, BF_LUT, .lut = b4constulut };
static struct bitfield addinoff = { { 4, 4 }, BF_LUT, .lut = addinlut };
static struct bitfield bbioff = { { 4, 4, 12, 1 } };
static struct bitfield mi12off = { { 16, 8, 8, 4 }, BF_SIGNED };
static struct bitfield i7off = { { 12, 4, 4, 3 }, BF_SLIGHTLY_SIGNED };
static struct bitfield shiftimmoff = { { 8, 4, 16, 1 } };
static struct bitfield maskimmoff = { { 20, 4 }, BF_LUT, .lut = maskimmlut };
static struct bitfield sraioff = { { 8, 4, 20, 1 } };
static struct bitfield ssaioff = { { 8, 4, 4, 1 } };
static struct bitfield sllioff = { { 4, 4, 20, 1 }, BF_LUT, .lut = sllilut };
static struct bitfield sextoff = { { 4, 4 }, .addend = 7 };
#define IMM8S atomimm, &imm8soff
#define IMM8S8 atomimm, &imm8s8off
#define IMM12M8 atomimm, &imm12m8off
#define ROT atomimm, &rotoff
#define ROTW atomimm, &rotwoff
#define SRLI atomimm, &srlioff
#define WAITI atomimm, &waitioff
#define BREAK8 atomimm, &break8off
#define BREAK4 atomimm, &break4off
#define B4CONST atomimm, &b4constoff
#define B4CONSTU atomimm, &b4constuoff
#define ADDIN atomimm, &addinoff
#define BBI atomimm, &bbioff
#define MI12 atomimm, &mi12off
#define MI7 atomimm, &i7off
#define SHIFTIMM atomimm, &shiftimmoff
#define MASKIMM atomimm, &maskimmoff
#define SRAI atomimm, &sraioff
#define SSAI atomimm, &ssaioff
#define SLLI atomimm, &sllioff
#define SEXT atomimm, &sextoff

/*
 * Memory fields
 */

static struct rbitfield l32r_imm = { { 8, 16 }, BF_ULTRASIGNED, 2, .pcrel = 1, .pospreadd = 3 };
static struct rbitfield data32e_imm = { { 12, 4 }, BF_ULTRASIGNED, 2 };
static struct rbitfield data32n_imm = { { 12, 4 }, BF_UNSIGNED, 2 };
static struct rbitfield datacl_imm = { { 20, 4 }, BF_UNSIGNED, 4 };
static struct rbitfield data32_imm = { { 16, 8 }, BF_UNSIGNED, 2 };
static struct rbitfield data16_imm = { { 16, 8 }, BF_UNSIGNED, 1 };
static struct rbitfield data8_imm = { { 16, 8 }, BF_UNSIGNED, 0 };
static struct mem l32r_m = { "", 0, 0, &l32r_imm, .literal = 1 };
static struct mem data32e_m = { "", 0, &as_r, &data32e_imm };
static struct mem data32n_m = { "", 0, &as_r, &data32n_imm };
static struct mem datacl_m = { "", 0, &as_r, &datacl_imm };
static struct mem data32_m = { "", 0, &as_r, &data32_imm };
static struct mem data16_m = { "", 0, &as_r, &data16_imm };
static struct mem data8_m = { "", 0, &as_r, &data8_imm };
#define L32R atommem, &l32r_m
#define DATA32E atommem, &data32e_m
#define DATA32N atommem, &data32n_m
#define DATACL atommem, &datacl_m
#define DATA32 atommem, &data32_m
#define DATA16 atommem, &data16_m
#define DATA8 atommem, &data8_m

static struct insn tabm[] = {
	{ 0x000000, 0xffffff, N("ill") },
	{ 0x000080, 0xffffff, N("ret"), ENDMARK },
	{ 0x000090, 0xffffff, N("retw"), ENDMARK },
	{ 0x0000a0, 0xfff0ff, N("jx"), REGS, ENDMARK },
	{ 0x0000c0, 0xfff0cf, N("callx"), ROT, REGS },
	{ 0x001000, 0xfff00f, N("movsp"), REGT, REGS },
	{ 0x002000, 0xffffff, N("isync") },
	{ 0x002010, 0xffffff, N("rsync") },
	{ 0x002020, 0xffffff, N("esync") },
	{ 0x002030, 0xffffff, N("dsync") },
	{ 0x002080, 0xffffff, N("excw") },
	{ 0x0020c0, 0xffffff, N("memw") },
	{ 0x0020d0, 0xffffff, N("extw") },
	{ 0x0020f0, 0xffffff, N("nop") },
	{ 0x003000, 0xffffff, N("rfe"), ENDMARK },
	{ 0x003200, 0xffffff, N("rfde"), ENDMARK },
	{ 0x003400, 0xffffff, N("rfwo"), ENDMARK },
	{ 0x003500, 0xffffff, N("rfwu"), ENDMARK },
	{ 0x003010, 0xfff0ff, N("rfi"), WAITI, ENDMARK },
	{ 0x004000, 0xfff00f, N("break"), BREAK8, BREAK4 },
	{ 0x005000, 0xffffff, N("syscall") },
	{ 0x005100, 0xffffff, N("simcall") },
	{ 0x006000, 0xfff00f, N("rsil"), REGT, WAITI },
	{ 0x007000, 0xfff0ff, N("waiti"), WAITI },
	{ 0x008000, 0xfff00f, N("any4"), BRT, BRSQ },
	{ 0x009000, 0xfff00f, N("all4"), BRT, BRSQ },
	{ 0x00a000, 0xfff00f, N("any8"), BRT, BRSO },
	{ 0x00b000, 0xfff00f, N("all8"), BRT, BRSO },
	{ 0x100000, 0xff000f, N("and"), REGR, REGS, REGT },
	{ 0x200000, 0xff000f, N("or"), REGR, REGS, REGT },
	{ 0x300000, 0xff000f, N("xor"), REGR, REGS, REGT },
	{ 0x400000, 0xfff0ff, N("ssr"), REGS },
	{ 0x401000, 0xfff0ff, N("ssl"), REGS },
	{ 0x402000, 0xfff0ff, N("ssa8l"), REGS },
	{ 0x403000, 0xfff0ff, N("ssa8b"), REGS },
	{ 0x404000, 0xfff0ef, N("ssai"), SSAI },
	{ 0x408000, 0xffff0f, N("rotw"), ROTW },
	{ 0x503000, 0xfff00f, N("ritlb0"), REGT, REGS },
	{ 0x504000, 0xfff0ff, N("iitlb"), REGS },
	{ 0x505000, 0xfff00f, N("pitlb"), REGT, REGS },
	{ 0x506000, 0xfff00f, N("witlb"), REGT, REGS },
	{ 0x507000, 0xfff00f, N("ritlb1"), REGT, REGS },
	{ 0x50b000, 0xfff00f, N("rdtlb0"), REGT, REGS },
	{ 0x50c000, 0xfff0ff, N("idtlb"), REGS },
	{ 0x50d000, 0xfff00f, N("pdtlb"), REGT, REGS },
	{ 0x50e000, 0xfff00f, N("wdtlb"), REGT, REGS },
	{ 0x50f000, 0xfff00f, N("rdtlb1"), REGT, REGS },
	{ 0x600000, 0xff0f0f, N("neg"), REGR, REGT },
	{ 0x600100, 0xff0f0f, N("abs"), REGR, REGT },
	{ 0x800000, 0xff000f, N("add"), REGR, REGS, REGT },
	{ 0x900000, 0xff000f, N("addx2"), REGR, REGS, REGT },
	{ 0xa00000, 0xff000f, N("addx4"), REGR, REGS, REGT },
	{ 0xb00000, 0xff000f, N("addx8"), REGR, REGS, REGT },
	{ 0xc00000, 0xff000f, N("sub"), REGR, REGS, REGT },
	{ 0xd00000, 0xff000f, N("subx2"), REGR, REGS, REGT },
	{ 0xe00000, 0xff000f, N("subx4"), REGR, REGS, REGT },
	{ 0xf00000, 0xff000f, N("subx8"), REGR, REGS, REGT },
	{ 0x010000, 0xef000f, N("slli"), REGR, REGS, SLLI },
	{ 0x210000, 0xef000f, N("srai"), REGR, REGT, SRAI },
	{ 0x410000, 0xff000f, N("srli"), REGR, REGT, SRLI },
	{ 0x610000, 0xff000f, N("xsr"), REGT, SREG },
	{ 0x810000, 0xff000f, N("src"), REGR, REGS, REGT },
	{ 0x910000, 0xff0f0f, N("srl"), REGR, REGT },
	{ 0xa10000, 0xff00ff, N("sll"), REGR, REGS },
	{ 0xb10000, 0xff0f0f, N("sra"), REGR, REGT },
	{ 0xc10000, 0xff000f, N("mul16u"), REGR, REGS, REGT },
	{ 0xd10000, 0xff000f, N("mul16s"), REGR, REGS, REGT },
	{ 0x020000, 0xff000f, N("andb"), BRR, BRS, BRT },
	{ 0x120000, 0xff000f, N("andbc"), BRR, BRS, BRT },
	{ 0x220000, 0xff000f, N("orb"), BRR, BRS, BRT },
	{ 0x320000, 0xff000f, N("orbc"), BRR, BRS, BRT },
	{ 0x420000, 0xff000f, N("xorb"), BRR, BRS, BRT },
	{ 0x820000, 0xff000f, N("mull"), REGR, REGS, REGT },
	{ 0xa20000, 0xff000f, N("muluh"), REGR, REGS, REGT },
	{ 0xb20000, 0xff000f, N("mulsh"), REGR, REGS, REGT },
	{ 0xc20000, 0xff000f, N("quou"), REGR, REGS, REGT },
	{ 0xd20000, 0xff000f, N("quos"), REGR, REGS, REGT },
	{ 0xe20000, 0xff000f, N("remu"), REGR, REGS, REGT },
	{ 0xf20000, 0xff000f, N("rems"), REGR, REGS, REGT },
	{ 0x030000, 0xff000f, N("rsr"), REGT, SREG },
	{ 0x130000, 0xff000f, N("wsr"), REGT, SREG },
	{ 0x230000, 0xff000f, N("sext"), REGR, REGS, SEXT },
	{ 0x430000, 0xff000f, N("min"), REGR, REGS, REGT },
	{ 0x530000, 0xff000f, N("max"), REGR, REGS, REGT },
	{ 0x630000, 0xff000f, N("minu"), REGR, REGS, REGT },
	{ 0x730000, 0xff000f, N("maxu"), REGR, REGS, REGT },
	{ 0x830000, 0xff000f, N("moveqz"), REGR, REGS, REGT },
	{ 0x930000, 0xff000f, N("movnez"), REGR, REGS, REGT },
	{ 0xa30000, 0xff000f, N("movltz"), REGR, REGS, REGT },
	{ 0xb30000, 0xff000f, N("movgez"), REGR, REGS, REGT },
	{ 0xc30000, 0xff000f, N("movf"), REGR, REGS, BRT },
	{ 0xd30000, 0xff000f, N("movt"), REGR, REGS, BRT },
	{ 0x040000, 0x0e000f, N("extui"), REGR, REGT, SHIFTIMM, MASKIMM },
	{ 0x090000, 0xff000f, N("l32e"), REGT, DATA32E },
	{ 0x490000, 0xff000f, N("s32e"), REGT, DATA32E },
	{ 0x000001, 0x00000f, N("l32r"), REGT, L32R },
	{ 0x000002, 0x00f00f, N("l8ui"), REGT, DATA8 },
	{ 0x001002, 0x00f00f, N("l16ui"), REGT, DATA16 },
	{ 0x002002, 0x00f00f, N("l32i"), REGT, DATA32 },
	{ 0x004002, 0x00f00f, N("s8i"), REGT, DATA8 },
	{ 0x005002, 0x00f00f, N("s16i"), REGT, DATA16 },
	{ 0x006002, 0x00f00f, N("s32i"), REGT, DATA32 },
	{ 0x007002, 0x00f0ff, N("dpfr"), DATA32 },
	{ 0x007012, 0x00f0ff, N("dpfw"), DATA32 },
	{ 0x007022, 0x00f0ff, N("dpfro"), DATA32 },
	{ 0x007032, 0x00f0ff, N("dpfwo"), DATA32 },
	{ 0x007042, 0x00f0ff, N("dhwb"), DATA32 },
	{ 0x007052, 0x00f0ff, N("dhwbi"), DATA32 },
	{ 0x007062, 0x00f0ff, N("dhi"), DATA32 },
	{ 0x007072, 0x00f0ff, N("dii"), DATA32 },
	{ 0x007082, 0x0ff0ff, N("dpfl"), DATACL },
	{ 0x027082, 0x0ff0ff, N("dhu"), DATACL },
	{ 0x037082, 0x0ff0ff, N("diu"), DATACL },
	{ 0x047082, 0x0ff0ff, N("diwb"), DATACL },
	{ 0x057082, 0x0ff0ff, N("diwbi"), DATACL },
	{ 0x0070c2, 0x00f0ff, N("ipf"), DATA32 },
	{ 0x0070d2, 0x0ff0ff, N("ipfl"), DATACL },
	{ 0x0270d2, 0x0ff0ff, N("ihu"), DATACL },
	{ 0x0370d2, 0x0ff0ff, N("iiu"), DATACL },
	{ 0x0070e2, 0x00f0ff, N("ihi"), DATA32 },
	{ 0x0070f2, 0x00f0ff, N("iii"), DATA32 },
	{ 0x009002, 0x00f00f, N("l16si"), REGT, DATA16 },
	{ 0x00a002, 0x00f00f, N("movi"), REGT, MI12 },
	{ 0x00c002, 0x00f00f, N("addi"), REGT, REGS, IMM8S },
	{ 0x00d002, 0x00f00f, N("addmi"), REGT, REGS, IMM8S8 },
	{ 0x000005, 0x00000f, N("call"), ROT, CTARG },
	{ 0x000006, 0x00003f, N("j"), JTARG, ENDMARK },
	{ 0x000016, 0x0000ff, N("beqz"), REGS, BTARG12 },
	{ 0x000056, 0x0000ff, N("bnez"), REGS, BTARG12 },
	{ 0x000096, 0x0000ff, N("bltz"), REGS, BTARG12 },
	{ 0x0000d6, 0x0000ff, N("bgez"), REGS, BTARG12 },
	{ 0x000026, 0x0000ff, N("beqi"), REGS, B4CONST, BTARG8 },
	{ 0x000066, 0x0000ff, N("bnei"), REGS, B4CONST, BTARG8 },
	{ 0x0000a6, 0x0000ff, N("blti"), REGS, B4CONST, BTARG8 },
	{ 0x0000e6, 0x0000ff, N("bgei"), REGS, B4CONST, BTARG8 },
	{ 0x000036, 0x0000ff, N("entry"), REGS, IMM12M8 },
	{ 0x000076, 0x00f0ff, N("bf"), BRS, LTARG },
	{ 0x001076, 0x00f0ff, N("bt"), BRS, LTARG },
	{ 0x008076, 0x00f0ff, N("loop"), REGS, LTARG },
	{ 0x009076, 0x00f0ff, N("loopnez"), REGS, LTARG },
	{ 0x00a076, 0x00f0ff, N("loopgtz"), REGS, LTARG },
	{ 0x0000b6, 0x0000ff, N("bltui"), REGS, B4CONSTU, BTARG8 },
	{ 0x0000f6, 0x0000ff, N("bgeui"), REGS, B4CONSTU, BTARG8 },
	{ 0x000007, 0x00f00f, N("bnone"), REGS, REGT, BTARG8 },
	{ 0x001007, 0x00f00f, N("beq"), REGS, REGT, BTARG8 },
	{ 0x002007, 0x00f00f, N("blt"), REGS, REGT, BTARG8 },
	{ 0x003007, 0x00f00f, N("bltu"), REGS, REGT, BTARG8 },
	{ 0x004007, 0x00f00f, N("ball"), REGS, REGT, BTARG8 },
	{ 0x005007, 0x00f00f, N("bbc"), REGS, REGT, BTARG8 },
	{ 0x006007, 0x00e00f, N("bbci"), REGS, BBI, BTARG8 },
	{ 0x008007, 0x00f00f, N("bany"), REGS, REGT, BTARG8 },
	{ 0x009007, 0x00f00f, N("bne"), REGS, REGT, BTARG8 },
	{ 0x00a007, 0x00f00f, N("bge"), REGS, REGT, BTARG8 },
	{ 0x00b007, 0x00f00f, N("bgeu"), REGS, REGT, BTARG8 },
	{ 0x00c007, 0x00f00f, N("bnall"), REGS, REGT, BTARG8 },
	{ 0x00d007, 0x00f00f, N("bbs"), REGS, REGT, BTARG8 },
	{ 0x00e007, 0x00e00f, N("bbsi"), REGS, BBI, BTARG8 },

	{ 0x0008, 0x000f, N("l32i.n"), REGT, DATA32N },
	{ 0x0009, 0x000f, N("s32i.n"), REGT, DATA32N },
	{ 0x000a, 0x000f, N("add.n"), REGR, REGS, REGT },
	{ 0x000b, 0x000f, N("addi.n"), REGR, REGS, ADDIN },
	{ 0x000c, 0x008f, N("movi.n"), REGS, MI7 },
	{ 0x008c, 0x00cf, N("beqz.n"), REGS, BTARG6 },
	{ 0x00cc, 0x00cf, N("bnez.n"), REGS, BTARG6 },
	{ 0x000d, 0xf00f, N("mov.n"), REGT, REGS },
	{ 0xf00d, 0xffff, N("ret.n"), ENDMARK },
	{ 0xf01d, 0xffff, N("retw.n"), ENDMARK },
	{ 0xf02d, 0xf0ff, N("break.n"), BREAK8 },
	{ 0xf03d, 0xffff, N("nop.n") },
	{ 0xf06d, 0xffff, N("ill.n") },

	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0x00, 0x08, OP3B, T(m) },
	{ 0x08, 0x08, OP2B, T(m) },
	{ 0, 0, OOPS },
};

struct disisa xtensa_isa_s = {
	tabroot,
	3,
	1,
	1,
};
