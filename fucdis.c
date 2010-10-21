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

static struct bitfield pcrel16off = { { 16, 16 }, BF_SIGNED, .pcrel = 1 };
static struct bitfield pcrel8off = { { 16, 8 }, BF_SIGNED, .pcrel = 1 };
#define SBTARG atombtarg, &pcrel8off
#define LBTARG atombtarg, &pcrel16off
#define LABTARG atombtarg, &imm16off
#define SABTARG atombtarg, &imm8off
#define LCTARG atomctarg, &imm16off
#define SCTARG atomctarg, &imm8off

/*
 * Register fields
 */

static int reg1off[] = { 8, 4, 'r' };
static int reg2off[] = { 12, 4, 'r' };
static int reg3off[] = { 20, 4, 'r' };
static int pred1off[] = { 8, 3, 'p' };
static int pred2off[] = { 16, 3, 'p' };
static int creg1off[] = { 16, 3, 'c' };
static int creg2off[] = { 20, 3, 'c' };
#define REG1 atomreg, reg1off
#define REG2 atomreg, reg2off
#define REG3 atomreg, reg3off
#define PRED1 atomreg, pred1off
#define PRED2 atomreg, pred2off
#define CREG1 atomreg, creg1off
#define CREG2 atomreg, creg2off

/*
 * Immediate fields
 */

static struct bitfield imm16off = { 16, 16 };
static struct bitfield imm8off = { 16, 8 };
static struct bitfield imm16soff = { { 16, 16 }, BF_SIGNED };
static struct bitfield imm8soff = { { 16, 8 }, BF_SIGNED };
static struct bitfield imm16hoff = { { 16, 16 }, BF_UNSIGNED, 16 };
static struct bitfield imm8hoff = { { 16, 8 }, BF_UNSIGNED, 16 };
static struct bitfield strapoff = { 8, 2 };
static struct bitfield cimm2off = { 20, 6 };
#define IMM16 atomimm, &imm16off
#define IMM8 atomimm, &imm8off
#define IMM16S atomimm, &imm16soff
#define IMM8S atomimm, &imm8soff
#define IMM16H atomimm, &imm16hoff
#define IMM8H atomimm, &imm8hoff
#define STRAP atomimm, &strapoff
#define CIMM2 atomimm, &cimm2off

#define BITF8 atombf, &imm8off
#define BITF16 atombf, &imm16off
static void atombf APROTO {
	if (!ctx->out)
		return;
	uint32_t i = GETBF(v);
	uint32_t j = i >> 5 & 0x1f;
	i &= 0x1f;
	fprintf (ctx->out, " %s%d:%d", cyel, i, i+j);
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
static void atomdatar APROTO {
	if (!ctx->out)
		return;
	fprintf (ctx->out, " %sD[%s$r%lld%s]", ccy, cbl, BF(12, 4), ccy);
}

static void atomdatari APROTO {
	if (!ctx->out)
		return;
	const int *n = v;
	fprintf (ctx->out, " %sD[%s$r%lld%s+%s%#llx%s]", ccy, cbl, BF(12, 4), ccy, cyel, BF(16,8) << n[0], ccy);
}

static void atomdatasp APROTO {
	if (!ctx->out)
		return;
	const int *n = v;
	fprintf (ctx->out, " %sD[%ssp%s+%s%#llx%s]", ccy, cgr, ccy, cyel, BF(16,8) << n[0], ccy);
}

static void atomdatarr APROTO {
	if (!ctx->out)
		return;
	const int *n = v;
	fprintf (ctx->out, " %sD[%s$r%lld%s+%s$r%lld%s*%s%d%s]", ccy, cbl, BF(12, 4), ccy, cbl, BF(8, 4), ccy, cyel, 1 << n[0], ccy);
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
static void atomior APROTO {
	if (!ctx->out)
		return;
	fprintf (ctx->out, " %sI[%s$r%lld%s]", ccy, cbl, BF(12, 4), ccy);
}

#define IORR atomiorr, 0
static void atomiorr APROTO {
	if (!ctx->out)
		return;
	fprintf (ctx->out, " %sI[%s$r%lld%s+%s$r%lld%s*%s4%s]", ccy, cbl, BF(12, 4), ccy, cbl, BF(8, 4), ccy, cyel, ccy);
}

#define IORI atomiori, 0
static void atomiori APROTO {
	if (!ctx->out)
		return;
	fprintf (ctx->out, " %sI[%s$r%lld%s+%s%#llx%s]", ccy, cbl, BF(12, 4), ccy, cyel, BF(16, 8) << 2, ccy);
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

static struct insn tababt[] = {
	{ AP, 0x00000000, 0x00000001, SABTARG },
	{ AP, 0x00000001, 0x00000001, LABTARG },
};

static struct insn tabct[] = {
	{ AP, 0x00000000, 0x00000001, SCTARG },
	{ AP, 0x00000001, 0x00000001, LCTARG },
};

static struct insn tabol0[] = {
	{ AP, 0x00000000, 0x00000001, OP24 },
	{ AP, 0x00000001, 0x00000001, OP32 },
};

static struct insn tabcocmd[] = {
	{ AP, 0x00000000, 0x80000000, N("cxset"), IMM8 },
	{ AP, 0x84000000, 0xfc000000, N("cmov"), CREG1, CREG2 },
	/* pull 16 bytes from crypt xfer stream */
	{ AP, 0x88000000, 0xfc000000, N("cxsin"), CREG1 },
	/* send 16 bytes to crypt xfer stream */
	{ AP, 0x8c000000, 0xfc000000, N("cxsout"), CREG1 },
	/* next ARG coprocessor instructions are stored as a "script" to be executed later. */
	{ AP, 0x94000000, 0xfc000000, N("cs0begin"), CIMM2 },
	/* the script stored by previous insn is executed ARG times. */
	{ AP, 0x98000000, 0xfc000000, N("cs0exec"), CIMM2 },
	/* like above, but operates on another script slot */
	{ AP, 0x9c000000, 0xfc000000, N("cs1begin"), CIMM2 },
	{ AP, 0xa0000000, 0xfc000000, N("cs1exec"), CIMM2 },
	{ AP, 0xac000000, 0xfc000000, N("cxor"), CREG1, CREG2 },
	{ AP, 0xb0000000, 0xfc000000, N("cadd"), CREG1, CIMM2 }, /* add immediate to register, modulo 2^64 */
	{ AP, 0xb4000000, 0xfc000000, N("cand"), CREG1, CREG2 },
	{ AP, 0xb8000000, 0xfc000000, N("crev"), CREG1, CREG2 }, /* reverse bytes */
	{ AP, 0xbc000000, 0xfc000000, N("cprecmac"), CREG1, CREG2 }, /* shift left by 1. if bit shifted out the left side was set, xor with 0x87. */
	/* Load selected secret to register. The register is subsequently
	 * marked "hidden", and all attempts to xfer stuff from it by
	 * non-authed code will result in just getting 0s. The hidden flag
	 * also propagates through all normal crypto ops to destination reg,
	 * so there's basically no way to get information out of hidden
	 * registers, other than via authed code. */
	{ AP, 0xc0000000, 0xfc000000, N("csecret"), CREG1, CIMM2 },
	/* Binds a register as the key for enc/dec operations. A *register*,
	 * not its contents. So if you bind $c3, then change its value,
	 * it'll use the new value.
	 */
	{ AP, 0xc4000000, 0xfc000000, N("ckeyreg"), CREG1 },
	/* key expansion - go through AES key schedule given the key [aka
	 * first 16 bytes of 176-byte expanded key], return last 16 bytes
	 * of the expended key. Used to get decryption keys.
	 */
	{ AP, 0xc8000000, 0xfc000000, N("ckexp"), CREG1, CREG2 },
	/* reverse operation to the above - get the original key given
	 * last 16 bytes of the expanded key*/
	{ AP, 0xcc000000, 0xfc000000, N("ckrexp"), CREG1, CREG2 },
	/* encrypts a block using "key" register value as key */
	{ AP, 0xd0000000, 0xfc000000, N("cenc"), CREG1, CREG2 },
	/* decrypts a block using "key" register value as last 16 bytes of
	 * expanded key - ie. you need to stuff the key through ckexp before
	 * use for decryption. */
	{ AP, 0xd4000000, 0xfc000000, N("cdec"), CREG1, CREG2 },
	/* auth only: encrypt code sig with ARG2 as key */
	{ AP, 0xdc000000, 0xfc000000, N("csigenc"), CREG1, CREG2 },
	{ AP, 0, 0, OOPS },
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
	{ AP, 0x00009000, 0x0000f000, N("cx") }, /* coprocessor xfer */
	{ AP, 0x0000a000, 0x0000f000, N("cauth") },
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
	{ AP, 0x00000a00, 0x00000f00, N("cauth") },
	{ AP, 0x00000b00, 0x00000f00, N("xtargets") },
	{ AP, 0x00000c00, 0x00000f00, N("tstatus") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabsi[] = {
	{ AP, 0x00000000, 0x0000003f, OP24, N("st"), T(sz), T(datari), REG1 },

	{ AP, 0x00000010, 0x0000003f, OP24, N("add"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000011, 0x0000003f, OP24, N("adc"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000012, 0x0000003f, OP24, N("sub"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000013, 0x0000003f, OP24, N("sbb"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000014, 0x0000003f, OP24, N("shl"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000015, 0x0000003f, OP24, N("shr"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000017, 0x0000003f, OP24, N("sar"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000018, 0x0000003f, OP24, N("ld"), T(sz), REG1, T(datari) },
	{ AP, 0x0000001c, 0x0000003f, OP24, N("shlc"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x0000001d, 0x0000003f, OP24, N("shrc"), T(sz), REG1, REG2, IMM8 },
	{ AP, 0x00000010, 0x00000030, OP24, OOPS, T(sz), REG1, REG2 },

	{ AP, 0x00000020, 0x0000003f, OP32, N("add"), T(sz), REG1, REG2, IMM16 },
	{ AP, 0x00000021, 0x0000003f, OP32, N("adc"), T(sz), REG1, REG2, IMM16 },
	{ AP, 0x00000022, 0x0000003f, OP32, N("sub"), T(sz), REG1, REG2, IMM16 },
	{ AP, 0x00000023, 0x0000003f, OP32, N("sbb"), T(sz), REG1, REG2, IMM16 },
	{ AP, 0x00000020, 0x00000030, OP32, OOPS, T(sz), REG1, REG2, IMM16 },

	{ AP, 0x00000130, 0x00000f3f, T(ol0), N("st"), T(sz), T(datasp), REG2 },
	{ AP, 0x00000430, 0x00000f3e, T(ol0), N("cmpu"), T(sz), REG2, T(i) },
	{ AP, 0x00000530, 0x00000f3e, T(ol0), N("cmps"), T(sz), REG2, T(is) },
	{ AP, 0x00000630, 0x00000f3e, T(ol0), N("cmp"), T(sz), REG2, T(is) },
	{ AP, 0x00000030, 0x0000003e, T(ol0), OOPS, T(sz), REG2, T(i) },

	{ AP, 0x00000034, 0x00000f3f, OP24, N("ld"), T(sz), REG2, T(datasp) },

	{ AP, 0x00000036, 0x00000f3e, T(ol0), N("add"), T(sz), REG2, T(i) },
	{ AP, 0x00000136, 0x00000f3e, T(ol0), N("adc"), T(sz), REG2, T(i) },
	{ AP, 0x00000236, 0x00000f3e, T(ol0), N("sub"), T(sz), REG2, T(i) },
	{ AP, 0x00000336, 0x00000f3e, T(ol0), N("sbb"), T(sz), REG2, T(i) },
	{ AP, 0x00000436, 0x00000f3e, T(ol0), N("shl"), T(sz), REG2, T(i) },
	{ AP, 0x00000536, 0x00000f3e, T(ol0), N("shr"), T(sz), REG2, T(i) },
	{ AP, 0x00000736, 0x00000f3e, T(ol0), N("sar"), T(sz), REG2, T(i) },
	{ AP, 0x00000c36, 0x00000f3e, T(ol0), N("shlc"), T(sz), REG2, T(i) },
	{ AP, 0x00000d36, 0x00000f3e, T(ol0), N("shrc"), T(sz), REG2, T(i) },
	{ AP, 0x00000036, 0x0000003e, T(ol0), OOPS, T(sz), REG2, T(i) },

	{ AP, 0x00000038, 0x000f003f, OP24, N("st"), T(sz), DATAR, REG1 },
	{ AP, 0x00040038, 0x000f003f, OP24, N("cmpu"), T(sz), REG2, REG1 },
	{ AP, 0x00050038, 0x000f003f, OP24, N("cmps"), T(sz), REG2, REG1 },
	{ AP, 0x00060038, 0x000f003f, OP24, N("cmp"), T(sz), REG2, REG1 }, /* NVA3+ */
	{ AP, 0x00000038, 0x0000003f, OP24, OOPS, T(sz), REG2, REG1 },

	{ AP, 0x00000039, 0x000f003f, OP24, N("not"), T(sz), REG1, REG2 },
	{ AP, 0x00010039, 0x000f003f, OP24, N("neg"), T(sz), REG1, REG2 },
	{ AP, 0x00020039, 0x000f003f, OP24, N("movf"), T(sz), REG1, REG2 }, /* mov and set ZF+SF according to val */
	{ AP, 0x00030039, 0x000f003f, OP24, N("hswap"), T(sz), REG1, REG2 }, /* swap halves - ie. rotate by half the size in bits. */
	{ AP, 0x00000039, 0x0000003f, OP24, OOPS, T(sz), REG1, REG2 },

	{ AP, 0x0000003b, 0x000f003f, OP24, N("add"), T(sz), REG2, REG1 },
	{ AP, 0x0001003b, 0x000f003f, OP24, N("adc"), T(sz), REG2, REG1 },
	{ AP, 0x0002003b, 0x000f003f, OP24, N("sub"), T(sz), REG2, REG1 },
	{ AP, 0x0003003b, 0x000f003f, OP24, N("sbb"), T(sz), REG2, REG1 },
	{ AP, 0x0004003b, 0x000f003f, OP24, N("shl"), T(sz), REG2, REG1 },
	{ AP, 0x0005003b, 0x000f003f, OP24, N("shr"), T(sz), REG2, REG1 },
	{ AP, 0x0007003b, 0x000f003f, OP24, N("sar"), T(sz), REG2, REG1 },
	{ AP, 0x000c003b, 0x000f003f, OP24, N("shlc"), T(sz), REG2, REG1 },
	{ AP, 0x000d003b, 0x000f003f, OP24, N("shrc"), T(sz), REG2, REG1 },
	{ AP, 0x0000003b, 0x0000003f, OP24, OOPS, T(sz), REG2, REG1 },

	{ AP, 0x0000003c, 0x000f003f, OP24, N("add"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0001003c, 0x000f003f, OP24, N("adc"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0002003c, 0x000f003f, OP24, N("sub"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0003003c, 0x000f003f, OP24, N("sbb"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0004003c, 0x000f003f, OP24, N("shl"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0005003c, 0x000f003f, OP24, N("shr"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0007003c, 0x000f003f, OP24, N("sar"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0008003c, 0x000f003f, OP24, N("ld"), T(sz), REG3, T(datarr) },
	{ AP, 0x000c003c, 0x000f003f, OP24, N("shlc"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x000d003c, 0x000f003f, OP24, N("shrc"), T(sz), REG3, REG2, REG1 },
	{ AP, 0x0000003c, 0x0000003f, OP24, OOPS, T(sz), REG3, REG2, REG1 },
	
	{ AP, 0x0000003d, 0x00000f3f, OP16, N("not"), T(sz), REG2 },
	{ AP, 0x0000013d, 0x00000f3f, OP16, N("neg"), T(sz), REG2 },
	{ AP, 0x0000023d, 0x00000f3f, OP16, N("movf"), T(sz), REG2 },
	{ AP, 0x0000033d, 0x00000f3f, OP16, N("hswap"), T(sz), REG2 },
	{ AP, 0x0000043d, 0x00000f3f, OP16, N("clear"), T(sz), REG2 }, /* set to 0. */
	{ AP, 0x0000003d, 0x0000003f, OP16, OOPS, T(sz), REG2 },

	{ AP, 0, 0, OOPS, T(sz) },
};

static struct insn tabm[] = {
	{ AP, 0x00000000, 0x000000c0, T(si) },
	{ AP, 0x00000040, 0x000000c0, T(si) },
	{ AP, 0x00000080, 0x000000c0, T(si) },

	{ AP, 0x000000c0, 0x000000ff, OP24, N("mulu"), REG1, REG2, IMM8 },
	{ AP, 0x000000c1, 0x000000ff, OP24, N("muls"), REG1, REG2, IMM8S },
	{ AP, 0x000000c2, 0x000000ff, OP24, N("sex"), REG1, REG2, IMM8 },
	{ AP, 0x000000c3, 0x000000ff, OP24, N("extrs"), REG1, REG2, BITF8 }, /* extracts a signed bitfield */
	{ AP, 0x000000c4, 0x000000ff, OP24, N("and"), REG1, REG2, IMM8 },
	{ AP, 0x000000c5, 0x000000ff, OP24, N("or"), REG1, REG2, IMM8 },
	{ AP, 0x000000c6, 0x000000ff, OP24, N("xor"), REG1, REG2, IMM8 },
	{ AP, 0x000000c7, 0x000000ff, OP24, N("extr"), REG1, REG2, BITF8 },
	{ AP, 0x000000c8, 0x000000ff, OP24, N("xbit"), REG1, REG2, IMM8 },
	{ AP, 0x000000cb, 0x000000ff, OP24, N("ins"), REG1, REG2, BITF8 }, /* inserts [overwrites] a bitfield */
	{ AP, 0x000000cc, 0x000000ff, OP24, N("div"), REG1, REG2, IMM8 },
	{ AP, 0x000000cd, 0x000000ff, OP24, N("mod"), REG1, REG2, IMM8 },
	{ AP, 0x000000cf, 0x000000ff, OP24, N("iord"), REG1, IORI },
	{ AP, 0x000000c0, 0x000000f0, OP24, OOPS, REG1, REG2, IMM8 },

	{ AP, 0x000000d0, 0x000000ff, OP24, N("iowr"), IORI, REG1 },
	{ AP, 0x000000d1, 0x000000ff, OP24, N("iowrs"), IORI, REG1 },

	{ AP, 0x000000e0, 0x000000ff, OP32, N("mulu"), REG1, REG2, IMM16 },
	{ AP, 0x000000e1, 0x000000ff, OP32, N("muls"), REG1, REG2, IMM16S },
	{ AP, 0x000000e3, 0x000000ff, OP32, N("extrs"), REG1, REG2, BITF16 },
	{ AP, 0x000000e4, 0x000000ff, OP32, N("and"), REG1, REG2, IMM16 },
	{ AP, 0x000000e5, 0x000000ff, OP32, N("or"), REG1, REG2, IMM16 },
	{ AP, 0x000000e6, 0x000000ff, OP32, N("xor"), REG1, REG2, IMM16 },
	{ AP, 0x000000e7, 0x000000ff, OP32, N("extr"), REG1, REG2, BITF16 },
	{ AP, 0x000000eb, 0x000000ff, OP32, N("ins"), REG1, REG2, BITF16 },
	{ AP, 0x000000ec, 0x000000ff, OP32, N("div"), REG1, REG2, IMM16 }, /* NVA3+ */
	{ AP, 0x000000ed, 0x000000ff, OP32, N("mod"), REG1, REG2, IMM16 }, /* NVA3+ */
	{ AP, 0x000000e0, 0x000000f0, OP32, OOPS, REG1, REG2, IMM16 },

	{ AP, 0x000000f0, 0x00000ffe, T(ol0), N("mulu"), REG2, T(i) },
	{ AP, 0x000001f0, 0x00000ffe, T(ol0), N("muls"), REG2, T(is) },
	{ AP, 0x000002f0, 0x00000ffe, T(ol0), N("sex"), REG2, T(i) }, /* funky instruction. bits ARG2+1 through 31 of ARG1 are replaced with copy of bit ARG2. */
	{ AP, 0x000003f0, 0x00000ffe, T(ol0), N("sethi"), REG2, T(ih) },
	{ AP, 0x000004f0, 0x00000ffe, T(ol0), N("and"), REG2, T(i) },
	{ AP, 0x000005f0, 0x00000ffe, T(ol0), N("or"), REG2, T(i) },
	{ AP, 0x000006f0, 0x00000ffe, T(ol0), N("xor"), REG2, T(i) },
	{ AP, 0x000007f0, 0x00000ffe, T(ol0), N("mov"), REG2, T(is) },
	{ AP, 0x000009f0, 0x00000ffe, T(ol0), N("bset"), REG2, T(i) },
	{ AP, 0x00000af0, 0x00000ffe, T(ol0), N("bclr"), REG2, T(i) },
	{ AP, 0x00000bf0, 0x00000ffe, T(ol0), N("btgl"), REG2, T(i) },
	{ AP, 0x00000cf0, 0x00000ffe, T(ol0), N("set"), REG2, T(fl) },
	{ AP, 0x000000f0, 0x000000fe, T(ol0), OOPS, REG2, T(i) },

	{ AP, 0x000008f2, 0x00000fff, OP24, N("setp"), T(fl), REG2 }, /* set given flag if bit0 of ARG2 set */
	/* The indirect crypt opcodes.
	 *
	 * For one-reg opcodes, the crypt register to use is selected by
	 * bits 8-10 of the operand. For two-reg opcodes, destination is in
	 * bits 8-10, source in 0-2. For reg, imm opcodes, reg is in
	 * bits 8-10, immediate in 0-5. For imm-only opcodes, immediate is in
	 * bits 0-5.
	 *
	 * Or, in other words, the insn is converted into f5/3c op as follows:
	 *
	 *  bits 0-3: bits 8-11 of reg operand
	 *  bits 4-9: bits 0-5 of reg operand
	 *  bits 10-14: bits 0-4 of immediate byte
	 *  bit 15: always set.
	 */
	{ AP, 0x00010cf2, 0x001f0fff, OP24, N("cimov"), REG2 },
	{ AP, 0x00020cf2, 0x001f0fff, OP24, N("cixsin"), REG2 },
	{ AP, 0x00030cf2, 0x001f0fff, OP24, N("cixsout"), REG2 },
	{ AP, 0x00050cf2, 0x001f0fff, OP24, N("cis0begin"), REG2 },
	{ AP, 0x00060cf2, 0x001f0fff, OP24, N("cis0exec"), REG2 },
	{ AP, 0x00070cf2, 0x001f0fff, OP24, N("cis1begin"), REG2 },
	{ AP, 0x00080cf2, 0x001f0fff, OP24, N("cis1exec"), REG2 },
	{ AP, 0x000b0cf2, 0x001f0fff, OP24, N("cixor"), REG2 },
	{ AP, 0x000c0cf2, 0x001f0fff, OP24, N("ciadd"), REG2 },
	{ AP, 0x000d0cf2, 0x001f0fff, OP24, N("ciand"), REG2 },
	{ AP, 0x000e0cf2, 0x001f0fff, OP24, N("cirev"), REG2 },
	{ AP, 0x000f0cf2, 0x001f0fff, OP24, N("ciprecmac"), REG2 },
	{ AP, 0x00100cf2, 0x001f0fff, OP24, N("cisecret"), REG2 },
	{ AP, 0x00110cf2, 0x001f0fff, OP24, N("cikeyreg"), REG2 },
	{ AP, 0x00120cf2, 0x001f0fff, OP24, N("cikexp"), REG2 },
	{ AP, 0x00130cf2, 0x001f0fff, OP24, N("cikrexp"), REG2 },
	{ AP, 0x00140cf2, 0x001f0fff, OP24, N("cienc"), REG2 },
	{ AP, 0x00150cf2, 0x001f0fff, OP24, N("cidec"), REG2 },
	{ AP, 0x00170cf2, 0x001f0fff, OP24, N("cisigenc"), REG2 },
	{ AP, 0x00000cf2, 0x00000fff, OP24, OOPS, REG2 },

	{ AP, 0x000000f4, 0x0000e0fe, T(ol0), N("bra"), T(p), T(bt) },
	{ AP, 0x000020f4, 0x0000fffe, T(ol0), N("bra"), T(abt) },
	{ AP, 0x000021f4, 0x0000fffe, T(ol0), N("call"), T(ct) },
	{ AP, 0x000028f4, 0x0000fffe, T(ol0), N("sleep"), T(fl) }, /* sleeps while given flag is true */
	{ AP, 0x000030f4, 0x0000fffe, T(ol0), N("add"), N("sp"), T(is) },
	{ AP, 0x000031f4, 0x0000fffe, T(ol0), N("bset"), N("flags"), T(fl) },
	{ AP, 0x000032f4, 0x0000fffe, T(ol0), N("bclr"), N("flags"), T(fl) },
	{ AP, 0x000033f4, 0x0000fffe, T(ol0), N("btgl"), N("flags"), T(fl) },
	{ AP, 0x00003cf4, 0x0000ffff, T(ol0), N("cxset"), IMM8 },
	{ AP, 0x00003cf5, 0x0000ffff, T(ol0), T(cocmd) },
	{ AP, 0x000000f4, 0x000000fe, T(ol0), OOPS, T(i) },

	{ AP, 0x000000f8, 0x0000ffff, OP16, N("ret") },
	{ AP, 0x000001f8, 0x0000ffff, OP16, N("iret") },
	{ AP, 0x000002f8, 0x0000ffff, OP16, N("exit") },
	{ AP, 0x000003f8, 0x0000ffff, OP16, N("xdwait") },
	{ AP, 0x000006f8, 0x0000ffff, U("f8/6") },
	{ AP, 0x000007f8, 0x0000ffff, OP16, N("xcwait") },
	{ AP, 0x000008f8, 0x0000fcff, OP16, N("trap"), STRAP },

	{ AP, 0x000000f9, 0x00000fff, OP16, N("push"), REG2 },
	{ AP, 0x000001f9, 0x00000fff, OP16, N("add"), N("sp"), REG2 },
	{ AP, 0x000004f9, 0x00000fff, OP16, N("bra"), REG2 },
	{ AP, 0x000005f9, 0x00000fff, OP16, N("call"), REG2 },
	{ AP, 0x000008f9, 0x00000fff, OP16, N("unbind"), REG2 }, // drops given physical page's VM tag. page specified as index, not as address.
	{ AP, 0x000009f9, 0x00000fff, OP16, N("bset"), N("flags"), REG2 },
	{ AP, 0x00000af9, 0x00000fff, OP16, N("bclr"), N("flags"), REG2 },
	{ AP, 0x00000bf9, 0x00000fff, OP16, N("btgl"), N("flags"), REG2 },
	{ AP, 0x000000f9, 0x000000ff, OP16, OOPS, REG2 },

	/* iowr is asynchronous, iowrs waits for completion. */
	{ AP, 0x000000fa, 0x000f00ff, OP24, N("iowr"), IOR, REG1 },
	{ AP, 0x000100fa, 0x000f00ff, OP24, N("iowrs"), IOR, REG1 },
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
	{ AP, 0x000400fa, 0x000f00ff, OP24, N("xcld"), REG2, REG1 },
	{ AP, 0x000500fa, 0x000f00ff, OP24, N("xdld"), REG2, REG1 },
	{ AP, 0x000600fa, 0x000f00ff, OP24, N("xdst"), REG2, REG1 },
	{ AP, 0x000000fa, 0x000000ff, OP24, OOPS, REG2, REG1 },

	{ AP, 0x000000fc, 0x00000fff, OP16, N("pop"), REG2 },
	{ AP, 0x000000fc, 0x000000ff, OP16, OOPS, REG2 },

	{ AP, 0x000000fe, 0x00ff00ff, OP24, N("mov"), T(srd), REG2 },
	{ AP, 0x000100fe, 0x00ff00ff, OP24, N("mov"), REG1, T(srs) },
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
	{ AP, 0x000200fe, 0x00ff00ff, OP24, N("ptlb"), REG1, REG2 },
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
	{ AP, 0x000300fe, 0x00ff00ff, OP24, N("vtlb"), REG1, REG2 },

	{ AP, 0x000000fd, 0x000f00ff, OP24, N("mulu"), REG2, REG1 },
	{ AP, 0x000100fd, 0x000f00ff, OP24, N("muls"), REG2, REG1 },
	{ AP, 0x000200fd, 0x000f00ff, OP24, N("sex"), REG2, REG1 },
	{ AP, 0x000400fd, 0x000f00ff, OP24, N("and"), REG2, REG1 },
	{ AP, 0x000500fd, 0x000f00ff, OP24, N("or"), REG2, REG1 },
	{ AP, 0x000600fd, 0x000f00ff, OP24, N("xor"), REG2, REG1 },
	{ AP, 0x000900fd, 0x000f00ff, OP24, N("bset"), REG2, REG1 },
	{ AP, 0x000a00fd, 0x000f00ff, OP24, N("bclr"), REG2, REG1 },
	{ AP, 0x000b00fd, 0x000f00ff, OP24, N("btgl"), REG2, REG1 },
	{ AP, 0x000000fd, 0x000000ff, OP24, OOPS, REG2, REG1 },

	{ AP, 0x000000ff, 0x000f00ff, OP24, N("mulu"), REG3, REG2, REG1 },
	{ AP, 0x000100ff, 0x000f00ff, OP24, N("muls"), REG3, REG2, REG1 },
	{ AP, 0x000200ff, 0x000f00ff, OP24, N("sex"), REG3, REG2, REG1 },
	{ AP, 0x000300ff, 0x000f00ff, OP24, N("extrs"), REG3, REG2, REG1 },
	{ AP, 0x000400ff, 0x000f00ff, OP24, N("and"), REG3, REG2, REG1 },
	{ AP, 0x000500ff, 0x000f00ff, OP24, N("or"), REG3, REG2, REG1 },
	{ AP, 0x000600ff, 0x000f00ff, OP24, N("xor"), REG3, REG2, REG1 },
	{ AP, 0x000700ff, 0x000f00ff, OP24, N("extr"), REG3, REG2, REG1 },
	{ AP, 0x000800ff, 0x000f00ff, OP24, N("xbit"), REG3, REG2, REG1 }, /* NV98: ARG1 = (ARG1 & 0xfffffffe) | (ARG2 >> ARG3 & 1) ; NVA3+: ARG1 = ARG2 >> ARG3 & 1 */
	{ AP, 0x000c00ff, 0x000f00ff, OP24, N("div"), REG3, REG2, REG1 },
	{ AP, 0x000d00ff, 0x000f00ff, OP24, N("mod"), REG3, REG2, REG1 },
	{ AP, 0x000f00ff, 0x000f00ff, OP24, N("iord"), REG3, IORR },
	{ AP, 0x000000ff, 0x000000ff, OP24, OOPS, REG3, REG2, REG1 },
	{ AP, 0, 0, OOPS },
};

static struct disisa fuc_isa_s = {
	tabm,
	4,
	1,
	1,
};

struct disisa *fuc_isa = &fuc_isa_s;
