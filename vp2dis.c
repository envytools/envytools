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
 * Options:
 *
 * - core [complete]
 * - code density [complete]
 * - loop [complete]
 * - 16-bit integer multiply [complete]
 * - boolean [complete]
 * - exception [complete]
 * - interrupt [complete]
 * - windowed register [complete]
 * - debug [complete]
 */

/*
 * Code target fields
 */

static int jtargoff[] = { 6, 18 };
static int btarg8off[] = { 16, 8 };
static int btarg12off[] = { 12, 12 };
#define JTARG atomjtarg, jtargoff
#define BTARG8 atomjtarg, btarg8off
#define BTARG12 atomjtarg, btarg12off
static void atomjtarg APROTO {
	const int *n = v;
	uint32_t delta = BF(n[0], n[1]);
	if (delta & 1 << (n[1] - 1)) delta -= 1 << n[1];
	markbt8(ctx, pos + 4 + delta);
	if (!ctx->out)
		return;
	fprintf (ctx->out, " %s%#x", cmag, pos + 4 + delta);
}

#define LTARG atomltarg, 0
static void atomltarg APROTO {
	uint32_t delta = BF(16, 8);
	markbt8(ctx, pos + 4 + delta);
	if (!ctx->out)
		return;
	fprintf (ctx->out, " %s%#x", cmag, pos + 4 + delta);
}

#define BTARG6 atombtarg6, 0
static void atombtarg6 APROTO {
	uint32_t delta = BF(12, 4) | BF(4, 2) << 4;
	markbt8(ctx, pos + 4 + delta);
	if (!ctx->out)
		return;
	fprintf (ctx->out, " %s%#x", cmag, pos + 4 + delta);
}

#define CTARG atomctarg, 0
static void atomctarg APROTO {
	uint32_t delta = BF(6, 18);
	if (delta & 0x20000) delta += 0xfffc0000;
	uint32_t target = (pos & ~3) + 4 + delta * 4;
	markct8(ctx, target);
	if (!ctx->out)
		return;
	fprintf (ctx->out, " %s%#x", cbr, target);
}

/*
 * Register fields
 */

static int regtoff[] = { 4, 4, 'r' };
static int regsoff[] = { 8, 4, 'r' };
static int regroff[] = { 12, 4, 'r' };
static int brtoff[] = { 4, 4, 'b' };
static int brsoff[] = { 8, 4, 'b' };
static int brroff[] = { 12, 4, 'b' };
#define REGT atomreg, regtoff
#define REGS atomreg, regsoff
#define REGR atomreg, regroff
#define BRT atomreg, brtoff
#define BRS atomreg, brsoff
#define BRR atomreg, brroff
#define BRSQ atomqreg, brsoff
#define BRSO atomoreg, brsoff

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
static struct bitfield sraioff = { { 8, 4, 20, 1 } };
static struct bitfield ssaioff = { { 8, 4, 4, 1 } };
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
#define SRAI atomimm, &sraioff
#define SSAI atomimm, &ssaioff

static void atommaskimm APROTO {
	if (!ctx->out)
		return;
	fprintf (ctx->out, " %s%#x", cyel, BF(20, 4) + 1);
}
#define MASKIMM atommaskimm, 0

static void atomslli APROTO {
	if (!ctx->out)
		return;
	uint32_t num = BF(4, 4) | BF(20, 1) << 4;
	fprintf (ctx->out, " %s%#x", cyel, 32 - num);
}
#define SLLI atomslli, 0


/*
 * Memory fields
 */

#define L32R atoml32r, 0
static void atoml32r APROTO {
	if (!ctx->out)
		return;
	uint32_t delta = BF(8, 16) | 0xffff0000;
	uint32_t addr = ((pos + 3) & ~3) + (delta << 2);
	fprintf (ctx->out, " %sD[%s%#x%s]", ccy, cyel, addr, ccy);
}

#define DATA32E atomdata32e, 0
static void atomdata32e APROTO {
	if (!ctx->out)
		return;
	fprintf (ctx->out, " %sD[%s$r%lld%s-%s%#llx%s]", ccy, cbl, BF(8, 4), ccy, cyel, (16 - BF(12, 4)) << 2, ccy);
}

static int data32noff[] = { 8, 4, 12, 4, 2 };
static int datacoff[] = { 8, 4, 16, 8, 2 };
static int datacloff[] = { 8, 4, 20, 4, 4 };
static int data32off[] = { 8, 4, 16, 8, 2 };
static int data16off[] = { 8, 4, 16, 8, 1 };
static int data8off[] = { 8, 4, 16, 8, 0 };
#define DATA32N atommem, data32noff
#define DATAC atommem, datacoff
#define DATACL atommem, datacloff
#define DATA32 atommem, data32off
#define DATA16 atommem, data16off
#define DATA8 atommem, data8off
static void atommem APROTO {
	if (!ctx->out)
		return;
	const int *n = v;
	ull delta = BF(n[2], n[3]) << n[4];
	fprintf (ctx->out, " %sD[%s$r%lld%s+%s%#llx%s]", ccy, cbl, BF(n[0], n[1]), ccy, cyel, delta, ccy);
}

static struct insn tabsr[] = {
	{ AP, 0x000000, 0x00ff00, N("lbeg") },
	{ AP, 0x000100, 0x00ff00, N("lend") },
	{ AP, 0x000200, 0x00ff00, N("lcount") },
	{ AP, 0x000300, 0x00ff00, N("sar") },
	{ AP, 0x000400, 0x00ff00, N("br") },
	{ AP, 0x004800, 0x00ff00, N("windowbase") },
	{ AP, 0x004900, 0x00ff00, N("windowstart") },
	{ AP, 0x006000, 0x00ff00, N("ibreakenable") },
	{ AP, 0x006800, 0x00ff00, N("ddr") },
	{ AP, 0x008000, 0x00ff00, N("ibreaka0") },
	{ AP, 0x008100, 0x00ff00, N("ibreaka1") },
	{ AP, 0x009000, 0x00ff00, N("dbreaka0") },
	{ AP, 0x009100, 0x00ff00, N("dbreaka1") },
	{ AP, 0x00a000, 0x00ff00, N("dbreakc0") },
	{ AP, 0x00a100, 0x00ff00, N("dbreakc1") },
	{ AP, 0x00b100, 0x00ff00, N("epc1") },
	{ AP, 0x00b200, 0x00ff00, N("epc2") },
	{ AP, 0x00b300, 0x00ff00, N("epc3") },
	{ AP, 0x00b400, 0x00ff00, N("epc4") },
	{ AP, 0x00b500, 0x00ff00, N("epc5") },
	{ AP, 0x00b600, 0x00ff00, N("epc6") },
	{ AP, 0x00c000, 0x00ff00, N("depc") },
	{ AP, 0x00c200, 0x00ff00, N("eps2") },
	{ AP, 0x00c300, 0x00ff00, N("eps3") },
	{ AP, 0x00c400, 0x00ff00, N("eps4") },
	{ AP, 0x00c500, 0x00ff00, N("eps5") },
	{ AP, 0x00c600, 0x00ff00, N("eps6") },
	{ AP, 0x00d100, 0x00ff00, N("excsave1") },
	{ AP, 0x00d200, 0x00ff00, N("excsave2") },
	{ AP, 0x00d300, 0x00ff00, N("excsave3") },
	{ AP, 0x00d400, 0x00ff00, N("excsave4") },
	{ AP, 0x00d500, 0x00ff00, N("excsave5") },
	{ AP, 0x00d600, 0x00ff00, N("excsave6") },
	{ AP, 0x00e200, 0x00ff00, N("interrupt") }, /* and intset */
	{ AP, 0x00e300, 0x00ff00, N("intclear") },
	{ AP, 0x00e400, 0x00ff00, N("intenable") },
	{ AP, 0x00e600, 0x00ff00, N("ps") },
	{ AP, 0x00e800, 0x00ff00, N("exccause") },
	{ AP, 0x00e900, 0x00ff00, N("debugcause") },
	{ AP, 0x00ea00, 0x00ff00, N("ccount") },
	{ AP, 0x00ec00, 0x00ff00, N("icount") },
	{ AP, 0x00ed00, 0x00ff00, N("icountlevel") },
	{ AP, 0x00ee00, 0x00ff00, N("excvaddr") },
	{ AP, 0x00f000, 0x00ff00, N("ccompare0") },
	{ AP, 0x00f100, 0x00ff00, N("ccompare1") },
	{ AP, 0x000000, 0x000000, OOPS },
};

static struct insn tabm[] = {
	{ AP, 0x000000, 0xffffff, N("ill") },
	{ AP, 0x000080, 0xffffff, N("ret") },
	{ AP, 0x000090, 0xffffff, N("retw") },
	{ AP, 0x0000a0, 0xfff0ff, N("jx"), REGS },
	{ AP, 0x0000c0, 0xfff0cf, N("callx"), ROT, REGS },
	{ AP, 0x001000, 0xfff00f, N("movsp"), REGT, REGS },
	{ AP, 0x002000, 0xffffff, N("isync") },
	{ AP, 0x002010, 0xffffff, N("rsync") },
	{ AP, 0x002020, 0xffffff, N("esync") },
	{ AP, 0x002030, 0xffffff, N("dsync") },
	{ AP, 0x002080, 0xffffff, N("excw") },
	{ AP, 0x0020c0, 0xffffff, N("memw") },
	{ AP, 0x0020d0, 0xffffff, N("extw") },
	{ AP, 0x0020f0, 0xffffff, N("nop") },
	{ AP, 0x003000, 0xffffff, N("rfe") },
	{ AP, 0x003200, 0xffffff, N("rfde") },
	{ AP, 0x003400, 0xffffff, N("rfwo") },
	{ AP, 0x003500, 0xffffff, N("rfwu") },
	{ AP, 0x003010, 0xfff0ff, N("rfi"), WAITI },
	{ AP, 0x004000, 0xfff00f, N("break"), BREAK8, BREAK4 },
	{ AP, 0x005000, 0xffffff, N("syscall") },
	{ AP, 0x006000, 0xfff00f, N("rsil"), REGT, WAITI },
	{ AP, 0x007000, 0xfff0ff, N("waiti"), WAITI },
	{ AP, 0x008000, 0xfff00f, N("any4"), BRT, BRSQ },
	{ AP, 0x009000, 0xfff00f, N("all4"), BRT, BRSQ },
	{ AP, 0x00a000, 0xfff00f, N("any8"), BRT, BRSO },
	{ AP, 0x00b000, 0xfff00f, N("all8"), BRT, BRSO },
	{ AP, 0x100000, 0xff000f, N("and"), REGR, REGS, REGT },
	{ AP, 0x200000, 0xff000f, N("or"), REGR, REGS, REGT },
	{ AP, 0x300000, 0xff000f, N("xor"), REGR, REGS, REGT },
	{ AP, 0x400000, 0xfff0ff, N("ssr"), REGS },
	{ AP, 0x401000, 0xfff0ff, N("ssl"), REGS },
	{ AP, 0x402000, 0xfff0ff, N("ssa8l"), REGS },
	{ AP, 0x403000, 0xfff0ff, N("ssa8b"), REGS },
	{ AP, 0x404000, 0xfff0ef, N("ssai"), SSAI },
	{ AP, 0x408000, 0xffff0f, N("rotw"), ROTW },
	{ AP, 0x503000, 0xfff00f, N("ritlb0"), REGT, REGS },
	{ AP, 0x504000, 0xfff0ff, N("iitlb"), REGS },
	{ AP, 0x505000, 0xfff00f, N("pitlb"), REGT, REGS },
	{ AP, 0x506000, 0xfff00f, N("witlb"), REGT, REGS },
	{ AP, 0x507000, 0xfff00f, N("ritlb1"), REGT, REGS },
	{ AP, 0x50b000, 0xfff00f, N("rdtlb0"), REGT, REGS },
	{ AP, 0x50c000, 0xfff0ff, N("idtlb"), REGS },
	{ AP, 0x50d000, 0xfff00f, N("pdtlb"), REGT, REGS },
	{ AP, 0x50e000, 0xfff00f, N("wdtlb"), REGT, REGS },
	{ AP, 0x50f000, 0xfff00f, N("rdtlb1"), REGT, REGS },
	{ AP, 0x600000, 0xff0f0f, N("neg"), REGR, REGT },
	{ AP, 0x600100, 0xff0f0f, N("abs"), REGR, REGT },
	{ AP, 0x800000, 0xff000f, N("add"), REGR, REGS, REGT },
	{ AP, 0x900000, 0xff000f, N("addx2"), REGR, REGS, REGT },
	{ AP, 0xa00000, 0xff000f, N("addx4"), REGR, REGS, REGT },
	{ AP, 0xb00000, 0xff000f, N("addx8"), REGR, REGS, REGT },
	{ AP, 0xc00000, 0xff000f, N("sub"), REGR, REGS, REGT },
	{ AP, 0xd00000, 0xff000f, N("subx2"), REGR, REGS, REGT },
	{ AP, 0xe00000, 0xff000f, N("subx4"), REGR, REGS, REGT },
	{ AP, 0xf00000, 0xff000f, N("subx8"), REGR, REGS, REGT },
	{ AP, 0x010000, 0xef000f, N("slli"), REGR, REGS, SLLI },
	{ AP, 0x210000, 0xef000f, N("srai"), REGR, REGT, SRAI },
	{ AP, 0x410000, 0xff000f, N("srli"), REGR, REGT, SRLI },
	{ AP, 0x610000, 0xff000f, N("xsr"), REGT, T(sr) },
	{ AP, 0x810000, 0xff000f, N("src"), REGR, REGS, REGT },
	{ AP, 0x910000, 0xff0f0f, N("srl"), REGR, REGT },
	{ AP, 0xa10000, 0xff00ff, N("sll"), REGR, REGS },
	{ AP, 0xb10000, 0xff0f0f, N("sra"), REGR, REGT },
	{ AP, 0xc10000, 0xff000f, N("mul16u"), REGR, REGS, REGT },
	{ AP, 0xd10000, 0xff000f, N("mul16s"), REGR, REGS, REGT },
	{ AP, 0x020000, 0xff000f, N("andb"), BRR, BRS, BRT },
	{ AP, 0x120000, 0xff000f, N("andbc"), BRR, BRS, BRT },
	{ AP, 0x220000, 0xff000f, N("orb"), BRR, BRS, BRT },
	{ AP, 0x320000, 0xff000f, N("orbc"), BRR, BRS, BRT },
	{ AP, 0x420000, 0xff000f, N("xorb"), BRR, BRS, BRT },
	{ AP, 0x030000, 0xff000f, N("rsr"), REGT, T(sr) },
	{ AP, 0x130000, 0xff000f, N("wsr"), REGT, T(sr) },
	{ AP, 0x830000, 0xff000f, N("moveqz"), REGR, REGS, REGT },
	{ AP, 0x930000, 0xff000f, N("movnez"), REGR, REGS, REGT },
	{ AP, 0xa30000, 0xff000f, N("movltz"), REGR, REGS, REGT },
	{ AP, 0xb30000, 0xff000f, N("movgez"), REGR, REGS, REGT },
	{ AP, 0xc30000, 0xff000f, N("movf"), REGR, REGS, BRT },
	{ AP, 0xd30000, 0xff000f, N("movt"), REGR, REGS, BRT },
	{ AP, 0x040000, 0x0e000f, N("extui"), REGR, REGT, SHIFTIMM, MASKIMM },
	{ AP, 0x090000, 0xff000f, N("l32e"), REGT, DATA32E },
	{ AP, 0x490000, 0xff000f, N("s32e"), REGT, DATA32E },
	{ AP, 0x000001, 0x00000f, N("l32r"), REGT, L32R },
	{ AP, 0x000002, 0x00f00f, N("l8ui"), REGT, DATA8 },
	{ AP, 0x001002, 0x00f00f, N("l16ui"), REGT, DATA16 },
	{ AP, 0x002002, 0x00f00f, N("l32i"), REGT, DATA32 },
	{ AP, 0x004002, 0x00f00f, N("s8i"), REGT, DATA8 },
	{ AP, 0x005002, 0x00f00f, N("s16i"), REGT, DATA16 },
	{ AP, 0x006002, 0x00f00f, N("s16i"), REGT, DATA32 },
	{ AP, 0x007002, 0x00f0ff, N("dpfr"), DATAC },
	{ AP, 0x007012, 0x00f0ff, N("dpfw"), DATAC },
	{ AP, 0x007022, 0x00f0ff, N("dpfro"), DATAC },
	{ AP, 0x007032, 0x00f0ff, N("dpfwo"), DATAC },
	{ AP, 0x007042, 0x00f0ff, N("dhwb"), DATAC },
	{ AP, 0x007052, 0x00f0ff, N("dhwbi"), DATAC },
	{ AP, 0x007062, 0x00f0ff, N("dhi"), DATAC },
	{ AP, 0x007072, 0x00f0ff, N("dii"), DATAC },
	{ AP, 0x007082, 0x0ff0ff, N("dpfl"), DATACL },
	{ AP, 0x027082, 0x0ff0ff, N("dhu"), DATACL },
	{ AP, 0x037082, 0x0ff0ff, N("diu"), DATACL },
	{ AP, 0x047082, 0x0ff0ff, N("diwb"), DATACL },
	{ AP, 0x057082, 0x0ff0ff, N("diwbi"), DATACL },
	{ AP, 0x0070c2, 0x00f0ff, N("ipf"), DATAC },
	{ AP, 0x0070d2, 0x0ff0ff, N("ipfl"), DATACL },
	{ AP, 0x0270d2, 0x0ff0ff, N("ihu"), DATACL },
	{ AP, 0x0370d2, 0x0ff0ff, N("iiu"), DATACL },
	{ AP, 0x0070e2, 0x00f0ff, N("ihi"), DATAC },
	{ AP, 0x0070f2, 0x00f0ff, N("iii"), DATAC },
	{ AP, 0x009002, 0x00f00f, N("l16si"), REGT, DATA16 },
	{ AP, 0x00a002, 0x00f00f, N("movi"), REGT, MI12 },
	{ AP, 0x00c002, 0x00f00f, N("addi"), REGT, REGS, IMM8S },
	{ AP, 0x00d002, 0x00f00f, N("addmi"), REGT, REGS, IMM8S8 },
	{ AP, 0x000005, 0x00000f, N("call"), ROT, CTARG },
	{ AP, 0x000006, 0x00003f, N("j"), JTARG },
	{ AP, 0x000016, 0x0000ff, N("beqz"), REGS, BTARG12 },
	{ AP, 0x000056, 0x0000ff, N("bnez"), REGS, BTARG12 },
	{ AP, 0x000096, 0x0000ff, N("bltz"), REGS, BTARG12 },
	{ AP, 0x0000d6, 0x0000ff, N("bgez"), REGS, BTARG12 },
	{ AP, 0x000026, 0x0000ff, N("beqi"), REGS, B4CONST, BTARG8 },
	{ AP, 0x000066, 0x0000ff, N("bnei"), REGS, B4CONST, BTARG8 },
	{ AP, 0x0000a6, 0x0000ff, N("blti"), REGS, B4CONST, BTARG8 },
	{ AP, 0x0000e6, 0x0000ff, N("bgei"), REGS, B4CONST, BTARG8 },
	{ AP, 0x000036, 0x0000ff, N("entry"), REGS, IMM12M8 },
	{ AP, 0x000076, 0x00f0ff, N("bf"), BRS, LTARG },
	{ AP, 0x001076, 0x00f0ff, N("bt"), BRS, LTARG },
	{ AP, 0x008076, 0x00f0ff, N("loop"), REGS, LTARG },
	{ AP, 0x009076, 0x00f0ff, N("loopnez"), REGS, LTARG },
	{ AP, 0x00a076, 0x00f0ff, N("loopgtz"), REGS, LTARG },
	{ AP, 0x0000b6, 0x0000ff, N("bltui"), REGS, B4CONSTU, BTARG8 },
	{ AP, 0x0000f6, 0x0000ff, N("bgeui"), REGS, B4CONSTU, BTARG8 },
	{ AP, 0x000007, 0x00f00f, N("bnone"), REGS, REGT, BTARG8 },
	{ AP, 0x001007, 0x00f00f, N("beq"), REGS, REGT, BTARG8 },
	{ AP, 0x002007, 0x00f00f, N("blt"), REGS, REGT, BTARG8 },
	{ AP, 0x003007, 0x00f00f, N("bltu"), REGS, REGT, BTARG8 },
	{ AP, 0x004007, 0x00f00f, N("ball"), REGS, REGT, BTARG8 },
	{ AP, 0x005007, 0x00f00f, N("bbc"), REGS, REGT, BTARG8 },
	{ AP, 0x006007, 0x00e00f, N("bbci"), REGS, BBI, BTARG8 },
	{ AP, 0x008007, 0x00f00f, N("bany"), REGS, REGT, BTARG8 },
	{ AP, 0x009007, 0x00f00f, N("bne"), REGS, REGT, BTARG8 },
	{ AP, 0x00a007, 0x00f00f, N("bge"), REGS, REGT, BTARG8 },
	{ AP, 0x00b007, 0x00f00f, N("bgeu"), REGS, REGT, BTARG8 },
	{ AP, 0x00c007, 0x00f00f, N("bnall"), REGS, REGT, BTARG8 },
	{ AP, 0x00d007, 0x00f00f, N("bbs"), REGS, REGT, BTARG8 },
	{ AP, 0x00e007, 0x00e00f, N("bbsi"), REGS, BBI, BTARG8 },

	{ AP, 0x0008, 0x000f, N("l32i.n"), REGT, DATA32N },
	{ AP, 0x0009, 0x000f, N("s32i.n"), REGT, DATA32N },
	{ AP, 0x000a, 0x000f, N("add.n"), REGR, REGS, REGT },
	{ AP, 0x000b, 0x000f, N("addi.n"), REGR, REGS, ADDIN },
	{ AP, 0x000c, 0x008f, N("movi.n"), REGS, MI7 },
	{ AP, 0x008c, 0x00cf, N("beqz.n"), REGS, BTARG6 },
	{ AP, 0x00cc, 0x00cf, N("bnez.n"), REGS, BTARG6 },
	{ AP, 0x000d, 0xf00f, N("mov.n"), REGT, REGS },
	{ AP, 0xf00d, 0xffff, N("ret.n") },
	{ AP, 0xf01d, 0xffff, N("retw.n") },
	{ AP, 0xf02d, 0xf0ff, N("break.n"), BREAK8 },
	{ AP, 0xf03d, 0xffff, N("nop.n") },
	{ AP, 0xf06d, 0xffff, N("ill.n") },

	{ AP, 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ AP, 0x00, 0x08, OP24, T(m) },
	{ AP, 0x08, 0x08, OP16, T(m) },
};

static struct disisa vp2_isa_s = {
	tabroot,
	3,
	1,
	1,
};

struct disisa *vp2_isa = &vp2_isa_s;
