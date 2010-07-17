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
 * Code target fields
 */

#define SBTARG atomsbtarg, 0
void atomsbtarg APROTO {
	uint32_t delta = BF(16, 8);
	if (delta & 0x80) delta += 0xffffff00;
	fprintf (out, " %s%#x", cbr, pos + delta);
}

#define LBTARG atomlbtarg, 0
void atomlbtarg APROTO {
	uint32_t delta = BF(16, 16);
	if (delta & 0x8000) delta += 0xffff0000;
	fprintf (out, " %s%#x", cbr, pos + delta);
}

#define LCTARG atomlctarg, 0
void atomlctarg APROTO {
	fprintf (out, " %s%#llx", cbr, BF(16, 16));
}

#define SCTARG atomsctarg, 0
void atomsctarg APROTO {
	fprintf (out, " %s%#llx", cbr, BF(16, 16));
}

/*
 * Register fields
 */

static int reg1off[] = { 8, 4, 'r' };
static int reg2off[] = { 12, 4, 'r' };
static int reg3off[] = { 20, 4, 'r' };
static int pred1off[] = { 8, 3, 'p' };
static int pred2off[] = { 16, 3, 'p' };
#define REG1 atomreg, reg1off
#define REG2 atomreg, reg2off
#define REG3 atomreg, reg3off
#define PRED1 atomreg, pred1off
#define PRED2 atomreg, pred2off

/*
 * Immediate fields
 */

static int imm16off[] = { 16, 16, 0, 0 };
static int imm8off[] = { 16, 8, 0, 0 };
static int imm16soff[] = { 16, 16, 0, 1 };
static int imm8soff[] = { 16, 8, 0, 1 };
static int imm16hoff[] = { 16, 16, 16, 0 };
static int imm8hoff[] = { 16, 8, 16, 0 };
#define IMM16 atomnum, imm16off
#define IMM8 atomnum, imm8off
#define IMM16S atomnum, imm16soff
#define IMM8S atomnum, imm8soff
#define IMM16H atomnum, imm16hoff
#define IMM8H atomnum, imm8hoff

/*
 * Memory fields
 */

static int datanoff[] = { -1 };
static int data8off[] = { 0 };
static int data16off[] = { 1 };
static int data32off[] = { 2 };
#define DATAN atomdata, datanoff
#define DATA8 atomdata, data8off
#define DATA16 atomdata, data16off
#define DATA32 atomdata, data32off
#define DATA8SP atomdatasp, data8off
#define DATA16SP atomdatasp, data16off
#define DATA32SP atomdatasp, data32off
void atomdata APROTO {
	const int *n = v;
	if (n[0] != -1)
		fprintf (out, " %sD[%s$r%lld%s+%s%#llx%s]", ccy, cbl, BF(12, 4), ccy, cyel, BF(16,8) << n[0], ccy);
	else
		fprintf (out, " %sD[%s$r%lld%s]", ccy, cbl, BF(12, 4), ccy);
}

void atomdatasp APROTO {
	const int *n = v;
	fprintf (out, " %sD[%ssp%s+%s%#llx%s]", ccy, cgr, ccy, cyel, BF(16,8) << n[0], ccy);
}

/*
 * MMIO space
 *
 * bits 8-17 specify register. It corresponds to MMIO reg fuc_base + ((addr >> 6) & 0xffc).
 * bits 2-7, for indexed registers, specify reg index. for host MMIO accesses, the index comes
 * from fuc_base + 0xffc instead.
 *
 * fuc_base + 0xff0 and up are host-only
 */
#define IOR atomior, 0
void atomior APROTO {
	fprintf (out, " %sI[%s$r%lld%s]", ccy, cbl, BF(12, 4), ccy);
}

#define IORR atomiorr, 0
void atomiorr APROTO {
	fprintf (out, " %sI[%s$r%lld%s+%s$r%lld%s*%s4%s]", ccy, cbl, BF(12, 4), ccy, cbl, BF(8, 4), ccy, cyel, ccy);
}

#define IORI atomiori, 0
void atomiori APROTO {
	fprintf (out, " %sI[%s$r%lld%s+%s%#llx%s]", ccy, cbl, BF(12, 4), ccy, cyel, BF(16, 8) << 2, ccy);
}

static struct insn tabp[] = {
	{ AP, 0x00000000, 0x00001800, PRED1 },
	{ AP, 0x00000800, 0x00001f00, N("lt") }, /* or c */
	{ AP, 0x00000900, 0x00001f00, N("o") },
	{ AP, 0x00000a00, 0x00001f00, N("s") },
	{ AP, 0x00000b00, 0x00001f00, N("eq") }, /* or z */
	{ AP, 0x00000c00, 0x00001f00, N("gt") },
	{ AP, 0x00000d00, 0x00001f00, N("le") },
	{ AP, 0x00000e00, 0x00001f00 }, /* always true */
	{ AP, 0x00001000, 0x00001800, N("not"), PRED1 },
	{ AP, 0x00001800, 0x00001f00, N("ge") }, /* or nc */
	{ AP, 0x00001900, 0x00001f00, N("no") },
	{ AP, 0x00001a00, 0x00001f00, N("ns") },
	{ AP, 0x00001b00, 0x00001f00, N("ne") }, /* or nz */
	{ AP, 0, 0, OOPS },
};

static struct insn tabfl[] = {
	{ AP, 0x00000000, 0x00180000, PRED2 },
	{ AP, 0x00080000, 0x001f0000, N("c") },
	{ AP, 0x00090000, 0x001f0000, N("o") },
	{ AP, 0x000a0000, 0x001f0000, N("s") },
	{ AP, 0x000b0000, 0x001f0000, N("z") },
	{ AP, 0x00180000, 0x001f0000, N("ex") },
};

static struct insn tabaop[] = {
	{ AP, 0x00000000, 0x00000f00, N("add") },
	{ AP, 0x00000100, 0x00000f00, N("adc") },
	{ AP, 0x00000200, 0x00000f00, N("sub") },
	{ AP, 0x00000300, 0x00000f00, N("sbb") },
	{ AP, 0x00000400, 0x00000f00, N("shl") },
	{ AP, 0x00000500, 0x00000f00, N("shr") },
	{ AP, 0x00000700, 0x00000f00, N("sar") },
	{ AP, 0x00000c00, 0x00000f00, N("shlc") },
	{ AP, 0x00000d00, 0x00000f00, N("shrc") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabi[] = {
	{ AP, 0x00000000, 0x00000001, IMM8 },
	{ AP, 0x00000001, 0x00000001, IMM16 },
};

static struct insn tabis[] = {
	{ AP, 0x00000000, 0x00000001, IMM8S },
	{ AP, 0x00000001, 0x00000001, IMM16S },
};

static struct insn tabih[] = {
	{ AP, 0x00000000, 0x00000001, IMM8H },
	{ AP, 0x00000001, 0x00000001, IMM16H },
};

static struct insn tabbt[] = {
	{ AP, 0x00000000, 0x00000001, SBTARG },
	{ AP, 0x00000001, 0x00000001, LBTARG },
};

static struct insn tabct[] = {
	{ AP, 0x00000000, 0x00000001, SCTARG },
	{ AP, 0x00000001, 0x00000001, LCTARG },
};

static struct insn tabrrr[] = {
	{ AP, 0x00000000, 0x00000000, REG3, REG1, REG2 },
};

static struct insn tabsz[] = {
	{ AP, 0x00000000, 0x000000c0, N("b8") },
	{ AP, 0x00000040, 0x000000c0, N("b16") },
	{ AP, 0x00000080, 0x000000c0, N("b32") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabdata[] = {
	{ AP, 0x00000000, 0x000000c0, DATA8 },
	{ AP, 0x00000040, 0x000000c0, DATA16 },
	{ AP, 0x00000080, 0x000000c0, DATA32 },
	{ AP, 0, 0, OOPS },
};

static struct insn tabdatasp[] = {
	{ AP, 0x00000000, 0x000000c0, DATA8SP },
	{ AP, 0x00000040, 0x000000c0, DATA16SP },
	{ AP, 0x00000080, 0x000000c0, DATA32SP },
	{ AP, 0, 0, OOPS },
};

static struct insn tabsrs[] = {
	{ AP, 0x00003000, 0x0000f000, N("ehandler") },
	{ AP, 0x00004000, 0x0000f000, N("sp") },
	{ AP, 0x00005000, 0x0000f000, N("pc") },
	{ AP, 0x00008000, 0x0000f000, N("flags") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabsrd[] = {
	{ AP, 0x00000300, 0x00000f00, N("ehandler") },
	{ AP, 0x00000400, 0x00000f00, N("sp") },
	{ AP, 0x00000800, 0x00000f00, N("flags") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabsi[] = {
	{ AP, 0x00000000, 0x0000003f, N("st"), T(sz), T(data), REG1 },

	{ AP, 0x00000010, 0x0000003f, N("add"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000011, 0x0000003f, N("adc"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000012, 0x0000003f, N("sub"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000013, 0x0000003f, N("sbb"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000014, 0x0000003f, N("shl"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000015, 0x0000003f, N("shr"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000017, 0x0000003f, N("sar"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000018, 0x0000003f, N("ld"), T(sz), REG1, T(data) },
	{ AP, 0x0000001c, 0x0000003f, N("shlc"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x0000001d, 0x0000003f, N("shrc"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000010, 0x000000f0, OOPS, REG1, REG2 },

	{ AP, 0x00000130, 0x00000f3f, N("st"), T(sz), T(datasp), REG2 },
	{ AP, 0x00000430, 0x00000f3e, N("cmpu"), T(sz), REG2, T(i) },
	{ AP, 0x00000530, 0x00000f3e, N("cmps"), T(sz), REG2, T(is) },
	{ AP, 0x00000630, 0x00000f3e, N("cmp"), T(sz), REG2, T(is) },
	{ AP, 0x00000030, 0x0000003e, OOPS, T(sz), REG2, T(i) },

	{ AP, 0x00000034, 0x00000f3f, N("ld"), T(sz), REG2, T(datasp) },

	{ AP, 0x00000036, 0x0000003e, T(aop), T(sz), REG2, T(i) },

	{ AP, 0x00000038, 0x000f003f, N("st"), T(sz), DATAN, REG1 },
	{ AP, 0x00040038, 0x000f003f, N("cmpu"), T(sz), REG2, REG1 },
	{ AP, 0x00050038, 0x000f003f, N("cmps"), T(sz), REG2, REG1 },
	{ AP, 0x00060038, 0x000f003f, N("cmp"), T(sz), REG2, REG1 }, /* NVA3+ */
	{ AP, 0x00000038, 0x0000003f, OOPS, T(sz), REG2, REG1 },

	{ AP, 0x00000039, 0x000f003f, N("not"), T(sz), REG1, REG2 },
	{ AP, 0x00010039, 0x000f003f, N("neg"), T(sz), REG1, REG2 },
	{ AP, 0x00020039, 0x000f003f, N("movf"), T(sz), REG1, REG2 }, /* mov and set ZF+SF according to val */
	{ AP, 0x00030039, 0x000f003f, N("hswap"), T(sz), REG1, REG2 }, /* swap halves - ie. rotate by half the size in bits. */
	{ AP, 0x00000039, 0x0000003f, OOPS, T(sz), REG1, REG2 },

	{ AP, 0x0000003b, 0x000f003f, N("add"), T(sz), REG2, REG1 },
	{ AP, 0x0001003b, 0x000f003f, N("adc"), T(sz), REG2, REG1 },
	{ AP, 0x0002003b, 0x000f003f, N("sub"), T(sz), REG2, REG1 },
	{ AP, 0x0003003b, 0x000f003f, N("sbb"), T(sz), REG2, REG1 },
	{ AP, 0x0004003b, 0x000f003f, N("shl"), T(sz), REG2, REG1 },
	{ AP, 0x0005003b, 0x000f003f, N("shr"), T(sz), REG2, REG1 },
	{ AP, 0x0007003b, 0x000f003f, N("sar"), T(sz), REG2, REG1 },
	{ AP, 0x000c003b, 0x000f003f, N("shlc"), T(sz), REG2, REG1 },
	{ AP, 0x000d003b, 0x000f003f, N("shrc"), T(sz), REG2, REG1 },
	{ AP, 0x0000003b, 0x0000003f, OOPS, T(sz), REG2, REG1 },

	{ AP, 0x0000003c, 0x000f003f, N("add"), T(sz), T(rrr) },
	{ AP, 0x0001003c, 0x000f003f, N("adc"), T(sz), T(rrr) },
	{ AP, 0x0002003c, 0x000f003f, N("sub"), T(sz), T(rrr) },
	{ AP, 0x0003003c, 0x000f003f, N("sbb"), T(sz), T(rrr) },
	{ AP, 0x0004003c, 0x000f003f, N("shl"), T(sz), T(rrr) },
	{ AP, 0x0005003c, 0x000f003f, N("shr"), T(sz), T(rrr) },
	{ AP, 0x0007003c, 0x000f003f, N("sar"), T(sz), T(rrr) },
	{ AP, 0x000c003c, 0x000f003f, N("shlc"), T(sz), T(rrr) },
	{ AP, 0x000d003c, 0x000f003f, N("shrc"), T(sz), T(rrr) },
	{ AP, 0x0000003c, 0x0000003f, OOPS, T(sz), T(rrr) },
	
	{ AP, 0x0000003d, 0x00000f3f, N("not"), T(sz), REG2 },
	{ AP, 0x0000013d, 0x00000f3f, N("neg"), T(sz), REG2 },
	{ AP, 0x0000023d, 0x00000f3f, N("movf"), T(sz), REG2 },
	{ AP, 0x0000033d, 0x00000f3f, N("hswap"), T(sz), REG2 },
	{ AP, 0x0000043d, 0x00000f3f, N("clear"), T(sz), REG2 }, /* set to 0. */
	{ AP, 0x0000003d, 0x0000003f, OOPS, T(sz), REG2 },

	{ AP, 0, 0, OOPS, T(sz) },
};

static struct insn tabm[] = {
	{ AP, 0x00000000, 0x000000c0, T(si) },
	{ AP, 0x00000040, 0x000000c0, T(si) },
	{ AP, 0x00000080, 0x000000c0, T(si) },

	{ AP, 0x000000c0, 0x000000ff, N("mulu"), REG1, REG2, IMM8 },
	{ AP, 0x000000c1, 0x000000ff, N("muls"), REG1, REG2, IMM8S },
	{ AP, 0x000000c2, 0x000000ff, N("sex"), REG1, REG2, IMM8 },
	{ AP, 0x000000c3, 0x000000ff, N("sbext"), REG1, REG2, IMM8 },
	{ AP, 0x000000c4, 0x000000ff, N("and"), REG1, REG2, IMM8 },
	{ AP, 0x000000c5, 0x000000ff, N("or"), REG1, REG2, IMM8 },
	{ AP, 0x000000c6, 0x000000ff, N("xor"), REG1, REG2, IMM8 },
	{ AP, 0x000000c7, 0x000000ff, N("bext"), REG1, REG2, IMM8 },
	{ AP, 0x000000c8, 0x000000ff, N("xbit"), REG1, REG2, IMM8 },
	{ AP, 0x000000cc, 0x000000ff, N("div"), REG1, REG2, IMM8 },
	{ AP, 0x000000cd, 0x000000ff, N("mod"), REG1, REG2, IMM8 },
	{ AP, 0x000000cf, 0x000000ff, N("iord"), REG1, IORI },
	{ AP, 0x000000c0, 0x000000f0, OOPS, REG1, REG2 },

	{ AP, 0x000000f0, 0x00000ffe, N("mulu"), REG2, T(i) },
	{ AP, 0x000001f0, 0x00000ffe, N("muls"), REG2, T(is) },
	{ AP, 0x000002f0, 0x00000ffe, N("sex"), REG2, T(i) }, /* funky instruction. bits ARG2+1 through 31 of ARG1 are replaced with copy of bit ARG2. */
	{ AP, 0x000003f0, 0x00000ffe, N("sethi"), REG2, T(ih) },
	{ AP, 0x000004f0, 0x00000ffe, N("and"), REG2, T(i) },
	{ AP, 0x000005f0, 0x00000ffe, N("or"), REG2, T(i) },
	{ AP, 0x000006f0, 0x00000ffe, N("xor"), REG2, T(i) },
	{ AP, 0x000007f0, 0x00000ffe, N("mov"), REG2, T(is) },
	{ AP, 0x000009f0, 0x00000ffe, N("bset"), REG2, T(i) },
	{ AP, 0x00000af0, 0x00000ffe, N("bclr"), REG2, T(i) },
	{ AP, 0x00000bf0, 0x00000ffe, N("btgl"), REG2, T(i) },
	{ AP, 0x00000cf0, 0x00000ffe, N("div"), REG2, T(i) },
	{ AP, 0x00000df0, 0x00000ffe, N("mod"), REG2, T(i) },
	{ AP, 0x000000f0, 0x000000fe, OOPS, REG2, T(i) },

	{ AP, 0x000008f2, 0x00000fff, N("setp"), T(fl), REG2 }, /* set given flag if bit0 of ARG2 set */

	{ AP, 0x000000f4, 0x0000e0fe, N("bra"), T(p), T(bt) },
	{ AP, 0x000020f4, 0x0000fffe, N("bra"), T(ct) },
	{ AP, 0x000021f4, 0x0000fffe, N("call"), T(ct) },
	{ AP, 0x000030f4, 0x0000fffe, N("add"), N("sp"), T(is) },
	{ AP, 0x00003cf4, 0x0000fffe, N("cmd"), T(i) },
	{ AP, 0x000000f4, 0x000000fe, OOPS, T(i) },

	{ AP, 0x000000f8, 0x0000ffff, N("ret") },
	{ AP, 0x000002f8, 0x0000ffff, N("exit") },

	{ AP, 0x000000f9, 0x00000fff, N("push"), REG2 },
	{ AP, 0x000005f9, 0x00000fff, N("call"), REG2 },
	{ AP, 0x000000f9, 0x000000ff, OOPS, REG2 },

	{ AP, 0x000000fa, 0x000f00ff, N("iowr"), IOR, REG1 },
	/* recv: read 16 bytes from cell (ARG1 >> 16) & 7 into memory at address ARG1 & 0xffff
	 * this and send have to be preceded by some cmd, or you lose
	 */
	{ AP, 0x000500fa, 0x000f00ff, N("recv"), REG1 },
	/* send: send 16 bytes to cell (ARG1 >> 16) & 7 from memory at address ARG1 & 0xffff */
	{ AP, 0x000600fa, 0x000f00ff, N("send"), REG1 },
	/* what the hell. how many ops do these insns have anyway. I've seen recv/send using REG2 too, but... for what? */
	{ AP, 0x000000fa, 0x000000ff, OOPS, REG2, REG1 },

	{ AP, 0x000000fc, 0x00000fff, N("pop"), REG2 },
	{ AP, 0x000000fc, 0x000000ff, OOPS, REG2 },

	{ AP, 0x000000fe, 0x00ff00ff, N("mov"), T(srd), REG2 },
	{ AP, 0x000100fe, 0x00ff00ff, N("mov"), REG1, T(srs) },

	{ AP, 0x000000ff, 0x000f00ff, N("mulu"), T(rrr) },
	{ AP, 0x000100ff, 0x000f00ff, N("muls"), T(rrr) },
	{ AP, 0x000200ff, 0x000f00ff, N("sex"), T(rrr) },
	{ AP, 0x000300f0, 0x000f00ff, N("sbext"), T(rrr) }, /* ARG1 = (ARG2 >> ARG3 & 1)?0xffffffff:0, NVA3+ */
	{ AP, 0x000400ff, 0x000f00ff, N("and"), T(rrr) },
	{ AP, 0x000500ff, 0x000f00ff, N("or"), T(rrr) },
	{ AP, 0x000600ff, 0x000f00ff, N("xor"), T(rrr) },
	{ AP, 0x000700f0, 0x000f00ff, N("bext"), T(rrr) }, /* ARG1 = ARG2 >> ARG3 & 1, NVA3+ */
	{ AP, 0x000800ff, 0x000f00ff, N("xbit"), T(rrr) }, /* NV98: ARG1 = (ARG1 & 0xfffffffe) | (ARG2 >> ARG3 & 1) ; NVA3+: same as bext */
	{ AP, 0x000c00ff, 0x000f00ff, N("div"), T(rrr) },
	{ AP, 0x000d00ff, 0x000f00ff, N("mod"), T(rrr) },
	{ AP, 0x000f00ff, 0x000f00ff, N("iord"), REG3, IORR },
	{ AP, 0x000000ff, 0x000000ff, OOPS, T(rrr) },
	{ AP, 0, 0, OOPS },
};

static uint32_t optab[] = {
	0x00, 3,
	0x10, 3,
	0x11, 3,
	0x12, 3,
	0x13, 3,
	0x14, 3,
	0x15, 3,
	0x17, 3,
	0x18, 3,
	0x1c, 3,
	0x1d, 3,
	0x20, 4,
	0x22, 4,
	0x30, 3,
	0x31, 4,
	0x34, 3,
	0x36, 3,
	0x37, 4,
	0x38, 3,
	0x39, 3,
	0x3b, 3,
	0x3c, 3,
	0x3d, 2,

	0xc2, 3,
	0xc4, 3,
	0xc5, 3,
	0xc6, 3,
	0xc7, 3,
	0xc8, 3,
	0xcf, 3,
	0xd0, 3,
	0xe0, 4,
	0xe3, 4,
	0xe4, 4,
	0xe5, 4,
	0xe7, 4,
	0xec, 4,
	0xf0, 3,
	0xf1, 4,
	0xf2, 3,
	0xf4, 3,
	0xf5, 4,
	0xf8, 2,
	0xf9, 2,
	0xfa, 3,
	0xfc, 2,
	0xfd, 3,
	0xfe, 3,
	0xff, 3,
};

/*
 * Disassembler driver
 *
 * You pass a block of memory to this function, disassembly goes out to given
 * FILE*.
 */

void fucdis (FILE *out, uint8_t *code, uint32_t start, int num, int ptype) {
	int cur = 0, i;
	while (cur < num) {
		fprintf (out, "%s%08x:%s", cgray, cur + start, cnorm);
		uint8_t op = code[cur];
		int length = 0;
		if (op < 0xc0)
			op &= 0x3f;
		for (i = 0; i < sizeof optab / sizeof *optab / 2; i++)
			if (op == optab[2*i])
				length = optab[2*i+1];
		if (!length || cur + length > num) {
			fprintf (out, " %s%02x                ??? [unknown op length]%s\n", cred, op, cnorm);
			cur++;
		} else {
			ull a = 0, m = 0;
			for (i = cur; i < cur + length; i++) {
				fprintf (out, " %02x", code[i]);
				a |= (ull)code[i] << (i-cur)*8;
			}
			for (i = 0; i < 6 - length; i++)
				fprintf (out, "   ");
			atomtab (out, &a, &m, tabm, ptype, cur + start);
			a &= ~m;
			if (a) {
				fprintf (out, " %s[unknown: %08llx]%s", cred, a, cnorm);
			}
			printf ("%s\n", cnorm);
			cur += length;
		}
	}
}
