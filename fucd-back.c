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
static int strapoff[] = { 8, 2, 0, 0 };
#define IMM16 atomnum, imm16off
#define IMM8 atomnum, imm8off
#define IMM16S atomnum, imm16soff
#define IMM8S atomnum, imm8soff
#define IMM16H atomnum, imm16hoff
#define IMM8H atomnum, imm8hoff
#define STRAP atomnum, strapoff

static struct insn tabcm[];

#define COCMD8 atomcocmd, imm8off
#define COCMD16 atomcocmd, imm16off
void atomcocmd APROTO {
	const int *n = v;
	ull na = BF(n[0], n[1]), nm = 0;
	fprintf (out, " %s%04llx", cgray, na);
	atomtab (out, &na, &nm, tabcm, ptype, pos);
	na &= ~nm;
	if (na) {
		fprintf (out, " %s[unknown: %04llx]%s", cred, na, cnorm);
	}
}

#define BITF8 atombf, imm8off
#define BITF16 atombf, imm16off
void atombf APROTO {
	const int *n = v;
	uint32_t i = BF(n[0], n[1]);
	if (n[3] && i&1ull<<(n[1]-1))
		i -= 1ull<<(n[1]);
	uint32_t j = i >> 5 & 0x1f;
	i &= 0x1f;
	fprintf (out, " %s%d:%d", cyel, i, i+j);
}

/*
 * Memory fields
 */

static int data8off[] = { 0 };
static int data16off[] = { 1 };
static int data32off[] = { 2 };
#define DATAR atomdatar, 0
#define DATARI8 atomdatari, data8off
#define DATARI16 atomdatari, data16off
#define DATARI32 atomdatari, data32off
#define DATARR8 atomdatarr, data8off
#define DATARR16 atomdatarr, data16off
#define DATARR32 atomdatarr, data32off
#define DATA8SP atomdatasp, data8off
#define DATA16SP atomdatasp, data16off
#define DATA32SP atomdatasp, data32off
void atomdatar APROTO {
	fprintf (out, " %sD[%s$r%lld%s]", ccy, cbl, BF(12, 4), ccy);
}

void atomdatari APROTO {
	const int *n = v;
	fprintf (out, " %sD[%s$r%lld%s+%s%#llx%s]", ccy, cbl, BF(12, 4), ccy, cyel, BF(16,8) << n[0], ccy);
}

void atomdatasp APROTO {
	const int *n = v;
	fprintf (out, " %sD[%ssp%s+%s%#llx%s]", ccy, cgr, ccy, cyel, BF(16,8) << n[0], ccy);
}

void atomdatarr APROTO {
	const int *n = v;
	fprintf (out, " %sD[%s$r%lld%s+%s$r%lld%s*%s%d%s]", ccy, cbl, BF(12, 4), ccy, cbl, BF(8, 4), ccy, cyel, 1 << n[0], ccy);
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
	{ AP, 0x00000800, 0x00001f00, N("c") },
	{ AP, 0x00000900, 0x00001f00, N("o") },
	{ AP, 0x00000a00, 0x00001f00, N("s") },
	{ AP, 0x00000b00, 0x00001f00, N("e") }, /* or z */
	{ AP, 0x00000c00, 0x00001f00, N("a") }, /* !c && !z */
	{ AP, 0x00000d00, 0x00001f00, N("na") }, /* c || z */
	{ AP, 0x00000e00, 0x00001f00 }, /* always true */
	{ AP, 0x00001000, 0x00001800, N("not"), PRED1 },
	{ AP, 0x00001800, 0x00001f00, N("nc") }, /* or nc */
	{ AP, 0x00001900, 0x00001f00, N("no") },
	{ AP, 0x00001a00, 0x00001f00, N("ns") },
	{ AP, 0x00001b00, 0x00001f00, N("ne") }, /* or nz */
	/* next 4 are NVA3+ */
	{ AP, 0x00001c00, 0x00001f00, N("g") }, /* !(o^s) && !z */
	{ AP, 0x00001d00, 0x00001f00, N("le") }, /* (o^s) || z */
	{ AP, 0x00001e00, 0x00001f00, N("l") }, /* o^s */
	{ AP, 0x00001f00, 0x00001f00, N("ge") }, /* !(o^s) */
	{ AP, 0, 0, OOPS },
};

static struct insn tabfl[] = {
	{ AP, 0x00000000, 0x00180000, PRED2 },
	{ AP, 0x00080000, 0x001f0000, N("c") },
	{ AP, 0x00090000, 0x001f0000, N("o") },
	{ AP, 0x000a0000, 0x001f0000, N("s") },
	{ AP, 0x000b0000, 0x001f0000, N("z") },
	{ AP, 0x00100000, 0x001f0000, N("ie0") },
	{ AP, 0x00110000, 0x001f0000, N("ie1") },
	{ AP, 0x00140000, 0x001f0000, N("ia0") },
	{ AP, 0x00150000, 0x001f0000, N("ia1") },
	{ AP, 0x00180000, 0x001f0000, N("ta") },
	{ AP, 0x00000000, 0x00000000, OOPS },
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

static struct insn tabcocmd[] = {
	{ AP, 0x00000000, 0x00000001, COCMD8 },
	{ AP, 0x00000001, 0x00000001, COCMD16 },
};

static struct insn tabsz[] = {
	{ AP, 0x00000000, 0x000000c0, N("b8") },
	{ AP, 0x00000040, 0x000000c0, N("b16") },
	{ AP, 0x00000080, 0x000000c0, N("b32") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabdatari[] = {
	{ AP, 0x00000000, 0x000000c0, DATARI8 },
	{ AP, 0x00000040, 0x000000c0, DATARI16 },
	{ AP, 0x00000080, 0x000000c0, DATARI32 },
	{ AP, 0, 0, OOPS },
};

static struct insn tabdatasp[] = {
	{ AP, 0x00000000, 0x000000c0, DATA8SP },
	{ AP, 0x00000040, 0x000000c0, DATA16SP },
	{ AP, 0x00000080, 0x000000c0, DATA32SP },
	{ AP, 0, 0, OOPS },
};

static struct insn tabdatarr[] = {
	{ AP, 0x00000000, 0x000000c0, DATARR8 },
	{ AP, 0x00000040, 0x000000c0, DATARR16 },
	{ AP, 0x00000080, 0x000000c0, DATARR32 },
	{ AP, 0, 0, OOPS },
};

static struct insn tabsrs[] = {
	{ AP, 0x00000000, 0x0000f000, N("iv0") },
	{ AP, 0x00001000, 0x0000f000, N("iv1") },
	{ AP, 0x00003000, 0x0000f000, N("tv") },
	{ AP, 0x00004000, 0x0000f000, N("sp") },
	{ AP, 0x00005000, 0x0000f000, N("pc") },
	{ AP, 0x00006000, 0x0000f000, N("xcbase") },
	{ AP, 0x00007000, 0x0000f000, N("xdbase") },
	{ AP, 0x00008000, 0x0000f000, N("flags") },
	{ AP, 0x00009000, 0x0000f000, U("cx") }, /* coprocessor xfer */
	{ AP, 0x0000a000, 0x0000f000, U("a") },
	{ AP, 0x0000b000, 0x0000f000, N("xtargets") },
	{ AP, 0x0000c000, 0x0000f000, N("tstatus") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabsrd[] = {
	{ AP, 0x00000000, 0x00000f00, N("iv0") },
	{ AP, 0x00000100, 0x00000f00, N("iv1") },
	{ AP, 0x00000300, 0x00000f00, N("tv") },
	{ AP, 0x00000400, 0x00000f00, N("sp") },
	{ AP, 0x00000600, 0x00000f00, N("xcbase") },
	{ AP, 0x00000700, 0x00000f00, N("xdbase") },
	{ AP, 0x00000800, 0x00000f00, N("flags") },
	{ AP, 0x00000a00, 0x00000f00, U("a") },
	{ AP, 0x00000b00, 0x00000f00, N("xtargets") },
	{ AP, 0x00000c00, 0x00000f00, N("tstatus") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabsi[] = {
	{ AP, 0x00000000, 0x0000003f, N("st"), T(sz), T(datari), REG1 },

	{ AP, 0x00000010, 0x0000003f, N("add"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000011, 0x0000003f, N("adc"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000012, 0x0000003f, N("sub"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000013, 0x0000003f, N("sbb"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000014, 0x0000003f, N("shl"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000015, 0x0000003f, N("shr"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000017, 0x0000003f, N("sar"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000018, 0x0000003f, N("ld"), T(sz), REG1, T(datari) },
	{ AP, 0x0000001c, 0x0000003f, N("shlc"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x0000001d, 0x0000003f, N("shrc"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000010, 0x00000030, OOPS, T(sz), REG1, REG2 },

	{ AP, 0x00000020, 0x0000003f, N("add"), T(sz), REG1, REG2, IMM16 },
	{ AP, 0x00000021, 0x0000003f, N("adc"), T(sz), REG1, REG2, IMM16 },
	{ AP, 0x00000022, 0x0000003f, N("sub"), T(sz), REG1, REG2, IMM16 },
	{ AP, 0x00000023, 0x0000003f, N("sbb"), T(sz), REG1, REG2, IMM16 },
	{ AP, 0x00000020, 0x00000030, OOPS, T(sz), REG1, REG2, IMM16 },

	{ AP, 0x00000130, 0x00000f3f, N("st"), T(sz), T(datasp), REG2 },
	{ AP, 0x00000430, 0x00000f3e, N("cmpu"), T(sz), REG2, T(i) },
	{ AP, 0x00000530, 0x00000f3e, N("cmps"), T(sz), REG2, T(is) },
	{ AP, 0x00000630, 0x00000f3e, N("cmp"), T(sz), REG2, T(is) },
	{ AP, 0x00000030, 0x0000003e, OOPS, T(sz), REG2, T(i) },

	{ AP, 0x00000034, 0x00000f3f, N("ld"), T(sz), REG2, T(datasp) },

	{ AP, 0x00000036, 0x00000f3e, N("add"), T(sz), REG2, T(i) },
	{ AP, 0x00000136, 0x00000f3e, N("adc"), T(sz), REG2, T(i) },
	{ AP, 0x00000236, 0x00000f3e, N("sub"), T(sz), REG2, T(i) },
	{ AP, 0x00000336, 0x00000f3e, N("sbb"), T(sz), REG2, T(i) },
	{ AP, 0x00000436, 0x00000f3e, N("shl"), T(sz), REG2, T(i) },
	{ AP, 0x00000536, 0x00000f3e, N("shr"), T(sz), REG2, T(i) },
	{ AP, 0x00000736, 0x00000f3e, N("sar"), T(sz), REG2, T(i) },
	{ AP, 0x00000c36, 0x00000f3e, N("shlc"), T(sz), REG2, T(i) },
	{ AP, 0x00000d36, 0x00000f3e, N("shrc"), T(sz), REG2, T(i) },
	{ AP, 0x00000036, 0x0000003e, OOPS, T(sz), REG2, T(i) },

	{ AP, 0x00000038, 0x000f003f, N("st"), T(sz), DATAR, REG1 },
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

	{ AP, 0x0000003c, 0x000f003f, N("add"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0001003c, 0x000f003f, N("adc"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0002003c, 0x000f003f, N("sub"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0003003c, 0x000f003f, N("sbb"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0004003c, 0x000f003f, N("shl"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0005003c, 0x000f003f, N("shr"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0007003c, 0x000f003f, N("sar"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0008003c, 0x000f003f, N("ld"), T(sz), REG3, T(datarr) },
	{ AP, 0x000c003c, 0x000f003f, N("shlc"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x000d003c, 0x000f003f, N("shrc"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0000003c, 0x0000003f, OOPS, T(sz), REG3, REG2, REG1 },
	
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
	{ AP, 0x000000c3, 0x000000ff, N("extrs"), REG1, REG2, BITF8 }, /* extracts a signed bitfield */
	{ AP, 0x000000c4, 0x000000ff, N("and"), REG1, REG2, IMM8 },
	{ AP, 0x000000c5, 0x000000ff, N("or"), REG1, REG2, IMM8 },
	{ AP, 0x000000c6, 0x000000ff, N("xor"), REG1, REG2, IMM8 },
	{ AP, 0x000000c7, 0x000000ff, N("extr"), REG1, REG2, BITF8 },
	{ AP, 0x000000c8, 0x000000ff, N("xbit"), REG1, REG2, IMM8 },
	{ AP, 0x000000cb, 0x000000ff, N("ins"), REG1, REG2, BITF8 }, /* inserts [overwrites] a bitfield */
	{ AP, 0x000000cc, 0x000000ff, N("div"), REG1, REG2, IMM8 },
	{ AP, 0x000000cd, 0x000000ff, N("mod"), REG1, REG2, IMM8 },
	{ AP, 0x000000cf, 0x000000ff, N("iord"), REG1, IORI },
	{ AP, 0x000000c0, 0x000000f0, OOPS, REG1, REG2, IMM8 },

	{ AP, 0x000000d0, 0x000000ff, N("iowr"), IORI, REG1 },
	{ AP, 0x000000d1, 0x000000ff, N("iowrs"), IORI, REG1 },

	{ AP, 0x000000e0, 0x000000ff, N("mulu"), REG1, REG2, IMM16 },
	{ AP, 0x000000e1, 0x000000ff, N("muls"), REG1, REG2, IMM16S },
	{ AP, 0x000000e3, 0x000000ff, N("extrs"), REG1, REG2, BITF16 },
	{ AP, 0x000000e4, 0x000000ff, N("and"), REG1, REG2, IMM16 },
	{ AP, 0x000000e5, 0x000000ff, N("or"), REG1, REG2, IMM16 },
	{ AP, 0x000000e6, 0x000000ff, N("xor"), REG1, REG2, IMM16 },
	{ AP, 0x000000e7, 0x000000ff, N("extr"), REG1, REG2, BITF16 },
	{ AP, 0x000000eb, 0x000000ff, N("ins"), REG1, REG2, BITF16 },
	{ AP, 0x000000ec, 0x000000ff, N("div"), REG1, REG2, IMM16 }, /* NVA3+ */
	{ AP, 0x000000ed, 0x000000ff, N("mod"), REG1, REG2, IMM16 }, /* NVA3+ */
	{ AP, 0x000000e0, 0x000000f0, OOPS, REG1, REG2, IMM16 },

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
	{ AP, 0x00000cf0, 0x00000ffe, N("set"), REG2, T(fl) },
	{ AP, 0x000000f0, 0x000000fe, OOPS, REG2, T(i) },

	{ AP, 0x000008f2, 0x00000fff, N("setp"), T(fl), REG2 }, /* set given flag if bit0 of ARG2 set */

	{ AP, 0x000000f4, 0x0000e0fe, N("bra"), T(p), T(bt) },
	{ AP, 0x000020f4, 0x0000fffe, N("bra"), T(ct) },
	{ AP, 0x000021f4, 0x0000fffe, N("call"), T(ct) },
	{ AP, 0x000028f4, 0x0000fffe, N("sleep"), T(fl) }, /* sleeps while given flag is true */
	{ AP, 0x000030f4, 0x0000fffe, N("add"), N("sp"), T(is) },
	{ AP, 0x000031f4, 0x0000fffe, N("bset"), N("flags"), T(fl) },
	{ AP, 0x000032f4, 0x0000fffe, N("bclr"), N("flags"), T(fl) },
	{ AP, 0x000033f4, 0x0000fffe, N("btgl"), N("flags"), T(fl) },
	{ AP, 0x00003cf4, 0x0000fffe, T(cocmd) },
	{ AP, 0x000000f4, 0x000000fe, OOPS, T(i) },

	{ AP, 0x000000f8, 0x0000ffff, N("ret") },
	{ AP, 0x000001f8, 0x0000ffff, N("iret") },
	{ AP, 0x000002f8, 0x0000ffff, N("exit") },
	{ AP, 0x000003f8, 0x0000ffff, N("xdwait") },
	{ AP, 0x000006f8, 0x0000ffff, U("f8/6") },
	{ AP, 0x000007f8, 0x0000ffff, N("xcwait") },
	{ AP, 0x000008f8, 0x0000fcff, N("trap"), STRAP },

	{ AP, 0x000000f9, 0x00000fff, N("push"), REG2 },
	{ AP, 0x000001f9, 0x00000fff, N("add"), N("sp"), REG2 },
	{ AP, 0x000004f9, 0x00000fff, N("bra"), REG2 },
	{ AP, 0x000005f9, 0x00000fff, N("call"), REG2 },
	{ AP, 0x000008f9, 0x00000fff, N("unbind"), REG2 }, // drops given physical page's VM tag. page specified as index, not as address.
	{ AP, 0x000009f9, 0x00000fff, N("bset"), N("flags"), REG2 },
	{ AP, 0x00000af9, 0x00000fff, N("bclr"), N("flags"), REG2 },
	{ AP, 0x00000bf9, 0x00000fff, N("btgl"), N("flags"), REG2 },
	{ AP, 0x000000f9, 0x000000ff, OOPS, REG2 },

	/* iowr is asynchronous, iowrs waits for completion. */
	{ AP, 0x000000fa, 0x000f00ff, N("iowr"), IOR, REG1 },
	{ AP, 0x000100fa, 0x000f00ff, N("iowrs"), IOR, REG1 },
	/* the transfer ops
	 *
	 * operand 1 is external offset and virtual address for code,
	 * operand 2 is size << 16 | fuc physical address for data,
	 * fuc physical address for code.
	 *
	 * These three ops xfer 4 << size bytes between external storage
	 * at address (external base << 8) + external offset and fuc code/data
	 * at address (fuc physical address). For xcld, the newly-loaded
	 * page is also bound to virtual address (external offset). size is
	 * forced to 6 for code. For data, it has to be in range 2-6.
	 *
	 * external base is taken from xdbase for data, xcbase for code.
	 * target is taken from xtarget bits ???.
	 * 
	 * Use xdwait/xcwait to wait for transfers to complete.
	 */
	{ AP, 0x000400fa, 0x000f00ff, N("xcld"), REG2, REG1 },
	{ AP, 0x000500fa, 0x000f00ff, N("xdld"), REG2, REG1 },
	{ AP, 0x000600fa, 0x000f00ff, N("xdst"), REG2, REG1 },
	{ AP, 0x000000fa, 0x000000ff, OOPS, REG2, REG1 },

	{ AP, 0x000000fc, 0x00000fff, N("pop"), REG2 },
	{ AP, 0x000000fc, 0x000000ff, OOPS, REG2 },

	{ AP, 0x000000fe, 0x00ff00ff, N("mov"), T(srd), REG2 },
	{ AP, 0x000100fe, 0x00ff00ff, N("mov"), REG1, T(srs) },
	/*
	 * REG1 = info about physical page in REG2, REG2 in pages
	 *
	 * info is:
	 *  bits 8:15 virtual page contained
	 *  bits 24:25 status
	 *  	0: empty: not mapped to any virtual address
	 *  	1: present: mapped to virtual address and usable
	 *  	2: busy: mapped to virtual address, but in the process of uploading stuff. will hang if used.
	 */
	{ AP, 0x000200fe, 0x00ff00ff, N("ptlb"), REG1, REG2 },
	/*
	 * REG1 = info about virtual address in REG2, REG2 in bytes
	 *
	 * info is:
	 *  bits 0:7 physical page containing it [if there is one]
	 *  bits 24:25 status
	 *  	ORed across all pages mapped to this virtual address
	 *  bit 30: multiple match
	 *  	found more than one physical page
	 *  bit 31: no match
	 *  	didn't find any physical page
	 */
	{ AP, 0x000300fe, 0x00ff00ff, N("vtlb"), REG1, REG2 },

	{ AP, 0x000000fd, 0x000f00ff, N("mulu"), REG2, REG1 },
	{ AP, 0x000100fd, 0x000f00ff, N("muls"), REG2, REG1 },
	{ AP, 0x000200fd, 0x000f00ff, N("sex"), REG2, REG1 },
	{ AP, 0x000400fd, 0x000f00ff, N("and"), REG2, REG1 },
	{ AP, 0x000500fd, 0x000f00ff, N("or"), REG2, REG1 },
	{ AP, 0x000600fd, 0x000f00ff, N("xor"), REG2, REG1 },
	{ AP, 0x000900fd, 0x000f00ff, N("bset"), REG2, REG1 },
	{ AP, 0x000a00fd, 0x000f00ff, N("bclr"), REG2, REG1 },
	{ AP, 0x000b00fd, 0x000f00ff, N("btgl"), REG2, REG1 },
	{ AP, 0x000000fd, 0x000000ff, OOPS, REG2, REG1 },

	{ AP, 0x000000ff, 0x000f00ff, N("mulu"), REG3, REG2, REG1 },
	{ AP, 0x000100ff, 0x000f00ff, N("muls"), REG3, REG2, REG1 },
	{ AP, 0x000200ff, 0x000f00ff, N("sex"), REG3, REG2, REG1 },
	{ AP, 0x000300ff, 0x000f00ff, N("extrs"), REG3, REG2, REG1 },
	{ AP, 0x000400ff, 0x000f00ff, N("and"), REG3, REG2, REG1 },
	{ AP, 0x000500ff, 0x000f00ff, N("or"), REG3, REG2, REG1 },
	{ AP, 0x000600ff, 0x000f00ff, N("xor"), REG3, REG2, REG1 },
	{ AP, 0x000700ff, 0x000f00ff, N("extr"), REG3, REG2, REG1 },
	{ AP, 0x000800ff, 0x000f00ff, N("xbit"), REG3, REG2, REG1 }, /* NV98: ARG1 = (ARG1 & 0xfffffffe) | (ARG2 >> ARG3 & 1) ; NVA3+: ARG1 = ARG2 >> ARG3 & 1 */
	{ AP, 0x000c00ff, 0x000f00ff, N("div"), REG3, REG2, REG1 },
	{ AP, 0x000d00ff, 0x000f00ff, N("mod"), REG3, REG2, REG1 },
	{ AP, 0x000f00ff, 0x000f00ff, N("iord"), REG3, IORR },
	{ AP, 0x000000ff, 0x000000ff, OOPS, REG3, REG2, REG1 },
	{ AP, 0, 0, OOPS },
};

static uint32_t optab[] = {
	0x00, 0xff, 3,
	0x10, 0xf0, 3,
	0x20, 0xf0, 4,
	0x30, 0xff, 3,
	0x31, 0xff, 4,
	0x34, 0xff, 3,
	0x36, 0xff, 3,
	0x37, 0xff, 4,
	0x38, 0xff, 3,
	0x39, 0xff, 3,
	0x3b, 0xff, 3,
	0x3c, 0xff, 3,
	0x3d, 0xff, 2,

	0xc0, 0xf0, 3,
	0xd0, 0xf0, 3,
	0xe0, 0xf0, 4,
	0xf0, 0xff, 3,
	0xf1, 0xff, 4,
	0xf2, 0xff, 3,
	0xf4, 0xff, 3,
	0xf5, 0xff, 4,
	0xf8, 0xff, 2,
	0xf9, 0xff, 2,
	0xfa, 0xff, 3,
	0xfc, 0xff, 2,
	0xfd, 0xff, 3,
	0xfe, 0xff, 3,
	0xff, 0xff, 3,
};

/* The coprocessor stuff */

/*
 * Immediate fields
 */

static int cimmcxoff[] = { 0, 8, 0, 0 };
static int cimm2off[] = { 4, 6, 0, 0 };
#define CIMMCX atomnum, cimmcxoff
#define CIMM2 atomnum, cimm2off

/*
 * Register fields
 */

static int creg1off[] = { 0, 3, 'c' };
static int creg2off[] = { 4, 3, 'c' };
#define CREG1 atomreg, creg1off
#define CREG2 atomreg, creg2off

static struct insn tabcm[] = {
	{ AP, 0x0000, 0x8000, N("cxset"), CIMMCX },
	{ AP, 0x8400, 0xfc00, N("cmov"), CREG1, CREG2 },
	{ AP, 0xac00, 0xfc00, N("cxor"), CREG1, CREG2 },
	{ AP, 0xb000, 0xfc00, N("cadd"), CREG1, CIMM2 }, /* add immediate to register, modulo 2^64 */
	{ AP, 0xb400, 0xfc00, N("cand"), CREG1, CREG2 },
	{ AP, 0xb800, 0xfc00, N("crev"), CREG1, CREG2 }, /* reverse bytes */
	{ AP, 0xbc00, 0xfc00, N("clfsr"), CREG1, CREG2 }, /* shift left by 1. if bit shifted out the left side was set, xor with 0x87. */
	{ AP, 0, 0, OOPS },
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
		for (i = 0; i < sizeof optab / sizeof *optab / 3; i++)
			if ((op & optab[3*i+1]) == optab[3*i])
				length = optab[3*i+2];
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
