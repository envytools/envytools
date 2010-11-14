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

#define NV98 FUC_NV98
#define NVA3P FUC_NVA3

/*
 * Code target fields
 */

static struct bitfield pcrel16off = { { 16, 16 }, BF_SIGNED, .pcrel = 1, .wrapok = 1 };
static struct bitfield pcrel8off = { { 16, 8 }, BF_SIGNED, .pcrel = 1 };
#define SBTARG atombtarg, &pcrel8off
#define LBTARG atombtarg, &pcrel16off
#define LABTARG atombtarg, &imm16woff
#define SABTARG atombtarg, &imm8off
#define LCTARG atomctarg, &imm16woff
#define SCTARG atomctarg, &imm8off

/*
 * Register fields
 */

static struct sreg sreg_sr[] = {
	{ 0, "iv0" },
	{ 1, "iv1" },
	{ 3, "tv" },
	{ 4, "sp" },
	{ 5, "pc" },
	{ 6, "xcbase" },
	{ 7, "xdbase" },
	{ 8, "flags" },
	{ 9, "cx" },
	{ 0xa, "cauth" },
	{ 0xb, "xtargets" },
	{ 0xc, "tstatus", .vartype = NVA3P },
	{ -1 },
};
static struct bitfield reg1_bf = { 8, 4 };
static struct bitfield reg2_bf = { 12, 4 };
static struct bitfield reg3_bf = { 20, 4 };
static struct bitfield pred1_bf = { 8, 3 };
static struct bitfield pred2_bf = { 16, 3 };
static struct bitfield creg1_bf = { 16, 3 };
static struct bitfield creg2_bf = { 20, 3 };
static struct reg reg1_r = { &reg1_bf, "r" };
static struct reg reg2_r = { &reg2_bf, "r" };
static struct reg reg3_r = { &reg3_bf, "r" };
static struct reg pred1_r = { &pred1_bf, "p", .cool = 1 };
static struct reg pred2_r = { &pred2_bf, "p", .cool = 1 };
static struct reg creg1_r = { &creg1_bf, "c" };
static struct reg creg2_r = { &creg2_bf, "c" };
static struct reg sreg1_r = { &reg1_bf, "s", .specials = sreg_sr, .always_special = 1 };
static struct reg sreg2_r = { &reg2_bf, "s", .specials = sreg_sr, .always_special = 1 };
static struct reg sp_r = { 0, "sp", .cool = 1 };
static struct reg flags_r = { 0, "flags", .cool = 1 };
#define REG1 atomreg, &reg1_r
#define REG2 atomreg, &reg2_r
#define REG3 atomreg, &reg3_r
#define PRED1 atomreg, &pred1_r
#define PRED2 atomreg, &pred2_r
#define CREG1 atomreg, &creg1_r
#define CREG2 atomreg, &creg2_r
#define SREG1 atomreg, &sreg1_r
#define SREG2 atomreg, &sreg2_r
#define SP atomreg, &sp_r
#define FLAGS atomreg, &flags_r

/*
 * Immediate fields
 */

static struct bitfield imm16off = { 16, 16 };
static struct bitfield imm8off = { 16, 8 };
static struct bitfield imm16soff = { { 16, 16 }, BF_SIGNED };
static struct bitfield imm8soff = { { 16, 8 }, BF_SIGNED };
static struct bitfield imm16woff = { { 16, 16 }, .wrapok = 1 };
static struct bitfield imm16hoff = { { 16, 16 }, BF_UNSIGNED, 16 };
static struct bitfield imm8hoff = { { 16, 8 }, BF_UNSIGNED, 16 };
static struct bitfield strapoff = { 8, 2 };
static struct bitfield cimm2off = { 20, 6 };
#define IMM16 atomimm, &imm16off
#define IMM8 atomimm, &imm8off
#define IMM16S atomimm, &imm16soff
#define IMM8S atomimm, &imm8soff
#define IMM16W atomimm, &imm16woff
#define IMM16H atomimm, &imm16hoff
#define IMM8H atomimm, &imm8hoff
#define STRAP atomimm, &strapoff
#define CIMM2 atomimm, &cimm2off

static struct bitfield bitf8bf[] = { { 16, 5 }, { 21, 3 } };
static struct bitfield bitf16bf[] = { { 16, 5 }, { 21, 5 }, };
#define BITF8 atombf, bitf8bf
#define BITF16 atombf, bitf16bf

/*
 * Memory fields
 */

static struct bitfield off8_bf = { { 16, 8 }, BF_UNSIGNED, 0 };
static struct bitfield off16_bf = { { 16, 8 }, BF_UNSIGNED, 1 };
static struct bitfield off32_bf = { { 16, 8 }, BF_UNSIGNED, 2 };
static struct mem datar_m = { "D", 0, &reg2_r };
static struct mem datari8_m = { "D", 0, &reg2_r, &off8_bf };
static struct mem datari16_m = { "D", 0, &reg2_r, &off16_bf };
static struct mem datari32_m = { "D", 0, &reg2_r, &off32_bf };
static struct mem datarr8_m = { "D", 0, &reg2_r, 0, &reg1_r, 0 };
static struct mem datarr16_m = { "D", 0, &reg2_r, 0, &reg1_r, 1 };
static struct mem datarr32_m = { "D", 0, &reg2_r, 0, &reg1_r, 2 };
static struct mem datasp8_m = { "D", 0, &sp_r, &off8_bf };
static struct mem datasp16_m = { "D", 0, &sp_r, &off16_bf };
static struct mem datasp32_m = { "D", 0, &sp_r, &off32_bf };
static struct mem dataspr8_m = { "D", 0, &sp_r, 0, &reg1_r, 0 };
static struct mem dataspr16_m = { "D", 0, &sp_r, 0, &reg1_r, 1 };
static struct mem dataspr32_m = { "D", 0, &sp_r, 0, &reg1_r, 2 };
#define DATAR atommem, &datar_m
#define DATARI8 atommem, &datari8_m
#define DATARI16 atommem, &datari16_m
#define DATARI32 atommem, &datari32_m
#define DATARR8 atommem, &datarr8_m
#define DATARR16 atommem, &datarr16_m
#define DATARR32 atommem, &datarr32_m
#define DATA8SP atommem, &datasp8_m
#define DATA16SP atommem, &datasp16_m
#define DATA32SP atommem, &datasp32_m
#define DATA8SPR atommem, &dataspr8_m
#define DATA16SPR atommem, &dataspr16_m
#define DATA32SPR atommem, &dataspr32_m

/*
 * MMIO space
 *
 * bits 8-17 specify register. It corresponds to MMIO reg fuc_base + ((addr >> 6) & 0xffc).
 * bits 2-7, for indexed registers, specify reg index. for host MMIO accesses, the index comes
 * from fuc_base + 0xffc instead.
 *
 * fuc_base + 0xff0 and up are host-only
 */

static struct mem ior_m = { "I", 0, &reg2_r };
static struct mem iorr_m = { "I", 0, &reg2_r, 0, &reg1_r, 2 };
static struct mem iori_m = { "I", 0, &reg2_r, &off32_bf };
#define IOR atommem, &ior_m
#define IORR atommem, &iorr_m
#define IORI atommem, &iori_m

static struct insn tabp[] = {
	{ -1,    -1, 0x00000000, 0x00001800, PRED1 },
	{ -1,    -1, 0x00000800, 0x00001f00, N("c") },
	{ -1,    -1, 0x00000900, 0x00001f00, N("o") },
	{ -1,    -1, 0x00000a00, 0x00001f00, N("s") },
	{ -1,    -1, 0x00000b00, 0x00001f00, N("e") }, /* or z */
	{ -1,    -1, 0x00000c00, 0x00001f00, N("a") }, /* !c && !z */
	{ -1,    -1, 0x00000d00, 0x00001f00, N("na") }, /* c || z */
	{ -1,    -1, 0x00000e00, 0x00001f00, ENDMARK }, /* always true */
	{ -1,    -1, 0x00001000, 0x00001800, N("not"), PRED1 },
	{ -1,    -1, 0x00001800, 0x00001f00, N("nc") }, /* or nc */
	{ -1,    -1, 0x00001900, 0x00001f00, N("no") },
	{ -1,    -1, 0x00001a00, 0x00001f00, N("ns") },
	{ -1,    -1, 0x00001b00, 0x00001f00, N("ne") }, /* or nz */
	{ NVA3P, -1, 0x00001c00, 0x00001f00, N("g") }, /* !(o^s) && !z */
	{ NVA3P, -1, 0x00001d00, 0x00001f00, N("le") }, /* (o^s) || z */
	{ NVA3P, -1, 0x00001e00, 0x00001f00, N("l") }, /* o^s */
	{ NVA3P, -1, 0x00001f00, 0x00001f00, N("ge") }, /* !(o^s) */
	{ -1,    -1, 0, 0, OOPS },
};

static struct insn tabfl[] = {
	{ -1, -1, 0x00000000, 0x00180000, PRED2 },
	{ -1, -1, 0x00080000, 0x001f0000, N("c") },
	{ -1, -1, 0x00090000, 0x001f0000, N("o") },
	{ -1, -1, 0x000a0000, 0x001f0000, N("s") },
	{ -1, -1, 0x000b0000, 0x001f0000, N("z") },
	{ -1, -1, 0x00100000, 0x001f0000, N("ie0") },
	{ -1, -1, 0x00110000, 0x001f0000, N("ie1") },
	{ -1, -1, 0x00140000, 0x001f0000, N("is0") },
	{ -1, -1, 0x00150000, 0x001f0000, N("is1") },
	{ -1, -1, 0x00180000, 0x001f0000, N("ta") },
	{ -1, -1, 0x00000000, 0x00000000, OOPS },
};

F(i, 0, IMM8, IMM16);
F(is, 0, IMM8S, IMM16S);
F(ih, 0, IMM8H, IMM16H);
F(bt, 0, SBTARG, LBTARG);
F(abt, 0, SABTARG, LABTARG);
F(ct, 0, SCTARG, LCTARG);
F(ol0, 0, OP24, OP32);

static struct insn tabcocmd[] = {
	{ -1, -1, 0x00000000, 0x80000000, N("cxset"), IMM8 },
	{ -1, -1, 0x84000000, 0xfc000000, N("cmov"), CREG1, CREG2 },
	/* pull 16 bytes from crypt xfer stream */
	{ -1, -1, 0x88000000, 0xfc000000, N("cxsin"), CREG1 },
	/* send 16 bytes to crypt xfer stream */
	{ -1, -1, 0x8c000000, 0xfc000000, N("cxsout"), CREG1 },
	/* next ARG coprocessor instructions are stored as a "script" to be executed later. */
	{ -1, -1, 0x94000000, 0xfc000000, N("cs0begin"), CIMM2 },
	/* the script stored by previous insn is executed ARG times. */
	{ -1, -1, 0x98000000, 0xfc000000, N("cs0exec"), CIMM2 },
	/* like above, but operates on another script slot */
	{ -1, -1, 0x9c000000, 0xfc000000, N("cs1begin"), CIMM2 },
	{ -1, -1, 0xa0000000, 0xfc000000, N("cs1exec"), CIMM2 },
	{ -1, -1, 0xac000000, 0xfc000000, N("cxor"), CREG1, CREG2 },
	{ -1, -1, 0xb0000000, 0xfc000000, N("cadd"), CREG1, CIMM2 }, /* add immediate to register, modulo 2^64 */
	{ -1, -1, 0xb4000000, 0xfc000000, N("cand"), CREG1, CREG2 },
	{ -1, -1, 0xb8000000, 0xfc000000, N("crev"), CREG1, CREG2 }, /* reverse bytes */
	{ -1, -1, 0xbc000000, 0xfc000000, N("cprecmac"), CREG1, CREG2 }, /* shift left by 1. if bit shifted out the left side was set, xor with 0x87. */
	/* Load selected secret to register. The register is subsequently
	 * marked "hidden", and all attempts to xfer stuff from it by
	 * non-authed code will result in just getting 0s. The hidden flag
	 * also propagates through all normal crypto ops to destination reg,
	 * so there's basically no way to get information out of hidden
	 * registers, other than via authed code. */
	{ -1, -1, 0xc0000000, 0xfc000000, N("csecret"), CREG1, CIMM2 },
	/* Binds a register as the key for enc/dec operations. A *register*,
	 * not its contents. So if you bind $c3, then change its value,
	 * it'll use the new value.
	 */
	{ -1, -1, 0xc4000000, 0xfc000000, N("ckeyreg"), CREG1 },
	/* key expansion - go through AES key schedule given the key [aka
	 * first 16 bytes of 176-byte expanded key], return last 16 bytes
	 * of the expended key. Used to get decryption keys.
	 */
	{ -1, -1, 0xc8000000, 0xfc000000, N("ckexp"), CREG1, CREG2 },
	/* reverse operation to the above - get the original key given
	 * last 16 bytes of the expanded key*/
	{ -1, -1, 0xcc000000, 0xfc000000, N("ckrexp"), CREG1, CREG2 },
	/* encrypts a block using "key" register value as key */
	{ -1, -1, 0xd0000000, 0xfc000000, N("cenc"), CREG1, CREG2 },
	/* decrypts a block using "key" register value as last 16 bytes of
	 * expanded key - ie. you need to stuff the key through ckexp before
	 * use for decryption. */
	{ -1, -1, 0xd4000000, 0xfc000000, N("cdec"), CREG1, CREG2 },
	/* auth only: encrypt code sig with ARG2 as key */
	{ -1, -1, 0xdc000000, 0xfc000000, N("csigenc"), CREG1, CREG2 },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabsz[] = {
	{ -1, -1, 0x00000000, 0x000000c0, N("b8") },
	{ -1, -1, 0x00000040, 0x000000c0, N("b16") },
	{ -1, -1, 0x00000080, 0x000000c0, N("b32") },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabdatari[] = {
	{ -1, -1, 0x00000000, 0x000000c0, DATARI8 },
	{ -1, -1, 0x00000040, 0x000000c0, DATARI16 },
	{ -1, -1, 0x00000080, 0x000000c0, DATARI32 },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabdatasp[] = {
	{ -1, -1, 0x00000000, 0x000000c0, DATA8SP },
	{ -1, -1, 0x00000040, 0x000000c0, DATA16SP },
	{ -1, -1, 0x00000080, 0x000000c0, DATA32SP },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabdatarr[] = {
	{ -1, -1, 0x00000000, 0x000000c0, DATARR8 },
	{ -1, -1, 0x00000040, 0x000000c0, DATARR16 },
	{ -1, -1, 0x00000080, 0x000000c0, DATARR32 },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabdataspr[] = {
	{ -1, -1, 0x00000000, 0x000000c0, DATA8SPR },
	{ -1, -1, 0x00000040, 0x000000c0, DATA16SPR },
	{ -1, -1, 0x00000080, 0x000000c0, DATA32SPR },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabsi[] = {
	{ -1,    -1, 0x00000000, 0x0000003f, OP24, N("st"), T(sz), T(datari), REG1 },
	{ -1,    -1, 0x00000000, 0x00000030, OP24, OOPS, T(sz), REG1, REG2, IMM8 },

	{ -1,    -1, 0x00000010, 0x0000003f, OP24, N("add"), T(sz), REG1, REG2, IMM8 },
	{ -1,    -1, 0x00000011, 0x0000003f, OP24, N("adc"), T(sz), REG1, REG2, IMM8 },
	{ -1,    -1, 0x00000012, 0x0000003f, OP24, N("sub"), T(sz), REG1, REG2, IMM8 },
	{ -1,    -1, 0x00000013, 0x0000003f, OP24, N("sbb"), T(sz), REG1, REG2, IMM8 },
	{ -1,    -1, 0x00000014, 0x0000003f, OP24, N("shl"), T(sz), REG1, REG2, IMM8 },
	{ -1,    -1, 0x00000015, 0x0000003f, OP24, N("shr"), T(sz), REG1, REG2, IMM8 },
	{ -1,    -1, 0x00000017, 0x0000003f, OP24, N("sar"), T(sz), REG1, REG2, IMM8 },
	{ -1,    -1, 0x00000018, 0x0000003f, OP24, N("ld"), T(sz), REG1, T(datari) },
	{ -1,    -1, 0x0000001c, 0x0000003f, OP24, N("shlc"), T(sz), REG1, REG2, IMM8 },
	{ -1,    -1, 0x0000001d, 0x0000003f, OP24, N("shrc"), T(sz), REG1, REG2, IMM8 },
	{ -1,    -1, 0x00000010, 0x00000030, OP24, OOPS, T(sz), REG1, REG2 },

	{ -1,    -1, 0x00000020, 0x0000003f, OP32, N("add"), T(sz), REG1, REG2, IMM16 },
	{ -1,    -1, 0x00000021, 0x0000003f, OP32, N("adc"), T(sz), REG1, REG2, IMM16 },
	{ -1,    -1, 0x00000022, 0x0000003f, OP32, N("sub"), T(sz), REG1, REG2, IMM16 },
	{ -1,    -1, 0x00000023, 0x0000003f, OP32, N("sbb"), T(sz), REG1, REG2, IMM16 },
	{ -1,    -1, 0x00000020, 0x00000030, OP32, OOPS, T(sz), REG1, REG2, IMM16 },

	{ -1,    -1, 0x00000130, 0x00000f3f, T(ol0), N("st"), T(sz), T(datasp), REG2 },
	{ -1,    -1, 0x00000430, 0x00000f3e, T(ol0), N("cmpu"), T(sz), REG2, T(i) },
	{ -1,    -1, 0x00000530, 0x00000f3e, T(ol0), N("cmps"), T(sz), REG2, T(is) },
	{ NVA3P, -1, 0x00000630, 0x00000f3e, T(ol0), N("cmp"), T(sz), REG2, T(is) },
	{ -1,    -1, 0x00000030, 0x0000003e, T(ol0), OOPS, T(sz), REG2, T(i) },

	{ -1,    -1, 0x00000034, 0x00000f3f, OP24, N("ld"), T(sz), REG2, T(datasp) },
	{ -1,    -1, 0x00000034, 0x0000003f, OP24, OOPS, T(sz), REG2, IMM8 },

	{ -1,    -1, 0x00000036, 0x00000f3e, T(ol0), N("add"), T(sz), REG2, T(i) },
	{ -1,    -1, 0x00000136, 0x00000f3e, T(ol0), N("adc"), T(sz), REG2, T(i) },
	{ -1,    -1, 0x00000236, 0x00000f3e, T(ol0), N("sub"), T(sz), REG2, T(i) },
	{ -1,    -1, 0x00000336, 0x00000f3e, T(ol0), N("sbb"), T(sz), REG2, T(i) },
	{ -1,    -1, 0x00000436, 0x00000f3f, T(ol0), N("shl"), T(sz), REG2, T(i) },
	{ -1,    -1, 0x00000536, 0x00000f3f, T(ol0), N("shr"), T(sz), REG2, T(i) },
	{ -1,    -1, 0x00000736, 0x00000f3f, T(ol0), N("sar"), T(sz), REG2, T(i) },
	{ -1,    -1, 0x00000c36, 0x00000f3f, T(ol0), N("shlc"), T(sz), REG2, T(i) },
	{ -1,    -1, 0x00000d36, 0x00000f3f, T(ol0), N("shrc"), T(sz), REG2, T(i) },
	{ -1,    -1, 0x00000036, 0x0000003e, T(ol0), OOPS, T(sz), REG2, T(i) },

	{ -1,    -1, 0x00000038, 0x000f003f, OP24, N("st"), T(sz), DATAR, REG1 },
	{ -1,    -1, 0x00010038, 0x000f003f, OP24, N("st"), T(sz), T(dataspr), REG2 },
	{ -1,    -1, 0x00040038, 0x000f003f, OP24, N("cmpu"), T(sz), REG2, REG1 },
	{ -1,    -1, 0x00050038, 0x000f003f, OP24, N("cmps"), T(sz), REG2, REG1 },
	{ NVA3P, -1, 0x00060038, 0x000f003f, OP24, N("cmp"), T(sz), REG2, REG1 },
	{ -1,    -1, 0x00000038, 0x0000003f, OP24, OOPS, T(sz), REG2, REG1 },

	{ -1,    -1, 0x00000039, 0x000f003f, OP24, N("not"), T(sz), REG1, REG2 },
	{ -1,    -1, 0x00010039, 0x000f003f, OP24, N("neg"), T(sz), REG1, REG2 },
	{ NV98,  -1, 0x00020039, 0x000f003f, OP24, N("movf"), T(sz), REG1, REG2 }, /* mov and set ZF+SF according to val */
	{ NVA3P, -1, 0x00020039, 0x000f003f, OP24, N("mov"), T(sz), REG1, REG2 },
	{ -1,    -1, 0x00030039, 0x000f003f, OP24, N("hswap"), T(sz), REG1, REG2 }, /* swap halves - ie. rotate by half the size in bits. */
	{ -1,    -1, 0x00000039, 0x0000003f, OP24, OOPS, T(sz), REG1, REG2 },

	{ -1,    -1, 0x0000003a, 0x000f003f, OP24, N("ld"), T(sz), REG2, T(dataspr) },
	{ -1,    -1, 0x0000003a, 0x0000003f, OP24, OOPS, T(sz), REG2, REG1 },

	{ -1,    -1, 0x0000003b, 0x000f003f, OP24, N("add"), T(sz), REG2, REG1 },
	{ -1,    -1, 0x0001003b, 0x000f003f, OP24, N("adc"), T(sz), REG2, REG1 },
	{ -1,    -1, 0x0002003b, 0x000f003f, OP24, N("sub"), T(sz), REG2, REG1 },
	{ -1,    -1, 0x0003003b, 0x000f003f, OP24, N("sbb"), T(sz), REG2, REG1 },
	{ -1,    -1, 0x0004003b, 0x000f003f, OP24, N("shl"), T(sz), REG2, REG1 },
	{ -1,    -1, 0x0005003b, 0x000f003f, OP24, N("shr"), T(sz), REG2, REG1 },
	{ -1,    -1, 0x0007003b, 0x000f003f, OP24, N("sar"), T(sz), REG2, REG1 },
	{ -1,    -1, 0x000c003b, 0x000f003f, OP24, N("shlc"), T(sz), REG2, REG1 },
	{ -1,    -1, 0x000d003b, 0x000f003f, OP24, N("shrc"), T(sz), REG2, REG1 },
	{ -1,    -1, 0x0000003b, 0x0000003f, OP24, OOPS, T(sz), REG2, REG1 },

	{ -1,    -1, 0x0000003c, 0x000f003f, OP24, N("add"), T(sz), REG3, REG2, REG1 },
	{ -1,    -1, 0x0001003c, 0x000f003f, OP24, N("adc"), T(sz), REG3, REG2, REG1 },
	{ -1,    -1, 0x0002003c, 0x000f003f, OP24, N("sub"), T(sz), REG3, REG2, REG1 },
	{ -1,    -1, 0x0003003c, 0x000f003f, OP24, N("sbb"), T(sz), REG3, REG2, REG1 },
	{ -1,    -1, 0x0004003c, 0x000f003f, OP24, N("shl"), T(sz), REG3, REG2, REG1 },
	{ -1,    -1, 0x0005003c, 0x000f003f, OP24, N("shr"), T(sz), REG3, REG2, REG1 },
	{ -1,    -1, 0x0007003c, 0x000f003f, OP24, N("sar"), T(sz), REG3, REG2, REG1 },
	{ -1,    -1, 0x0008003c, 0x000f003f, OP24, N("ld"), T(sz), REG3, T(datarr) },
	{ -1,    -1, 0x000c003c, 0x000f003f, OP24, N("shlc"), T(sz), REG3, REG2, REG1 },
	{ -1,    -1, 0x000d003c, 0x000f003f, OP24, N("shrc"), T(sz), REG3, REG2, REG1 },
	{ -1,    -1, 0x0000003c, 0x0000003f, OP24, OOPS, T(sz), REG3, REG2, REG1 },
	
	{ -1,    -1, 0x0000003d, 0x00000f3f, OP16, N("not"), T(sz), REG2 },
	{ -1,    -1, 0x0000013d, 0x00000f3f, OP16, N("neg"), T(sz), REG2 },
	{ NV98,  -1, 0x0000023d, 0x00000f3f, OP16, N("movf"), T(sz), REG2 },
	{ NVA3P, -1, 0x0000023d, 0x00000f3f, OP16, N("mov"), T(sz), REG2 }, /* about as useless as you'd expect */
	{ -1,    -1, 0x0000033d, 0x00000f3f, OP16, N("hswap"), T(sz), REG2 },
	{ -1,    -1, 0x0000043d, 0x00000f3f, OP16, N("clear"), T(sz), REG2 }, /* set to 0. */
	{ NVA3P, -1, 0x0000053d, 0x00000f3f, OP16, N("setf"), T(sz), REG2 }, /* set flags according to val */
	{ -1,    -1, 0x0000003d, 0x0000003f, OP16, OOPS, T(sz), REG2 },

	{ -1,    -1, 0, 0, OOPS, T(sz) },
};

static struct insn tabm[] = {
	{ -1,    -1, 0x00000000, 0x000000c0, T(si) },
	{ -1,    -1, 0x00000040, 0x000000c0, T(si) },
	{ -1,    -1, 0x00000080, 0x000000c0, T(si) },

	{ -1,    -1, 0x000000c0, 0x000000ff, OP24, N("mulu"), REG1, REG2, IMM8 },
	{ -1,    -1, 0x000000c1, 0x000000ff, OP24, N("muls"), REG1, REG2, IMM8S },
	{ -1,    -1, 0x000000c2, 0x000000ff, OP24, N("sex"), REG1, REG2, IMM8 },
	{ NVA3P, -1, 0x000000c3, 0x000000ff, OP24, N("extrs"), REG1, REG2, BITF8 }, /* extracts a signed bitfield */
	{ -1,    -1, 0x000000c4, 0x000000ff, OP24, N("and"), REG1, REG2, IMM8 },
	{ -1,    -1, 0x000000c5, 0x000000ff, OP24, N("or"), REG1, REG2, IMM8 },
	{ -1,    -1, 0x000000c6, 0x000000ff, OP24, N("xor"), REG1, REG2, IMM8 },
	{ NVA3P, -1, 0x000000c7, 0x000000ff, OP24, N("extr"), REG1, REG2, BITF8 },
	{ -1,    -1, 0x000000c8, 0x000000ff, OP24, N("xbit"), REG1, REG2, IMM8 },
	{ NVA3P, -1, 0x000000cb, 0x000000ff, OP24, N("ins"), REG1, REG2, BITF8 }, /* inserts [overwrites] a bitfield */
	{ NVA3P, -1, 0x000000cc, 0x000000ff, OP24, N("div"), REG1, REG2, IMM8 },
	{ NVA3P, -1, 0x000000cd, 0x000000ff, OP24, N("mod"), REG1, REG2, IMM8 },
	{ -1,    -1, 0x000000ce, 0x000000ff, OP24, U("ce"), REG1, IORI },
	{ -1,    -1, 0x000000cf, 0x000000ff, OP24, N("iord"), REG1, IORI },
	{ -1,    -1, 0x000000c0, 0x000000f0, OP24, OOPS, REG1, REG2, IMM8 },

	{ -1,    -1, 0x000000d0, 0x000000ff, OP24, N("iowr"), IORI, REG1 },
	{ NVA3P, -1, 0x000000d1, 0x000000ff, OP24, N("iowrs"), IORI, REG1 },
	{ -1,    -1, 0x000000d0, 0x000000f0, OP24, OOPS, REG1, REG2, IMM8 },

	{ -1,    -1, 0x000000e0, 0x000000ff, OP32, N("mulu"), REG1, REG2, IMM16 },
	{ -1,    -1, 0x000000e1, 0x000000ff, OP32, N("muls"), REG1, REG2, IMM16S },
	{ NVA3P, -1, 0x000000e3, 0x000000ff, OP32, N("extrs"), REG1, REG2, BITF16 },
	{ -1,    -1, 0x000000e4, 0x000000ff, OP32, N("and"), REG1, REG2, IMM16 },
	{ -1,    -1, 0x000000e5, 0x000000ff, OP32, N("or"), REG1, REG2, IMM16 },
	{ -1,    -1, 0x000000e6, 0x000000ff, OP32, N("xor"), REG1, REG2, IMM16 },
	{ NVA3P, -1, 0x000000e7, 0x000000ff, OP32, N("extr"), REG1, REG2, BITF16 },
	{ NVA3P, -1, 0x000000eb, 0x000000ff, OP32, N("ins"), REG1, REG2, BITF16 },
	{ NVA3P, -1, 0x000000ec, 0x000000ff, OP32, N("div"), REG1, REG2, IMM16 },
	{ NVA3P, -1, 0x000000ed, 0x000000ff, OP32, N("mod"), REG1, REG2, IMM16 },
	{ -1,    -1, 0x000000e0, 0x000000f0, OP32, OOPS, REG1, REG2, IMM16 },

	{ -1,    -1, 0x000000f0, 0x00000ffe, T(ol0), N("mulu"), REG2, T(i) },
	{ -1,    -1, 0x000001f0, 0x00000ffe, T(ol0), N("muls"), REG2, T(is) },
	{ -1,    -1, 0x000002f0, 0x00000fff, T(ol0), N("sex"), REG2, T(i) }, /* funky instruction. bits ARG2+1 through 31 of ARG1 are replaced with copy of bit ARG2. */
	{ -1,    -1, 0x000003f0, 0x00000ffe, T(ol0), N("sethi"), REG2, T(ih) },
	{ -1,    -1, 0x000004f0, 0x00000ffe, T(ol0), N("and"), REG2, T(i) },
	{ -1,    -1, 0x000005f0, 0x00000ffe, T(ol0), N("or"), REG2, T(i) },
	{ -1,    -1, 0x000006f0, 0x00000ffe, T(ol0), N("xor"), REG2, T(i) },
	{ -1,    -1, 0x000007f0, 0x00000ffe, T(ol0), N("mov"), REG2, T(is) },
	{ -1,    -1, 0x000007f1, 0x00000fff, T(ol0), N("movw"), REG2, IMM16W },
	{ -1,    -1, 0x000009f0, 0x00000fff, T(ol0), N("bset"), REG2, T(i) },
	{ -1,    -1, 0x00000af0, 0x00000fff, T(ol0), N("bclr"), REG2, T(i) },
	{ -1,    -1, 0x00000bf0, 0x00000fff, T(ol0), N("btgl"), REG2, T(i) },
	{ -1,    -1, 0x00000cf0, 0x00000fff, T(ol0), N("set"), REG2, FLAGS, T(fl) },
	{ -1,    -1, 0x000000f0, 0x000000fe, T(ol0), OOPS, REG2, T(i) },

	{ -1,    -1, 0x000008f2, 0x00000fff, OP24, N("setp"), T(fl), REG2 }, /* set given flag if bit0 of ARG2 set */
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
	{ -1,    -1, 0x00010cf2, 0x001f0fff, OP24, N("cimov"), REG2 },
	{ -1,    -1, 0x00020cf2, 0x001f0fff, OP24, N("cixsin"), REG2 },
	{ -1,    -1, 0x00030cf2, 0x001f0fff, OP24, N("cixsout"), REG2 },
	{ -1,    -1, 0x00050cf2, 0x001f0fff, OP24, N("cis0begin"), REG2 },
	{ -1,    -1, 0x00060cf2, 0x001f0fff, OP24, N("cis0exec"), REG2 },
	{ -1,    -1, 0x00070cf2, 0x001f0fff, OP24, N("cis1begin"), REG2 },
	{ -1,    -1, 0x00080cf2, 0x001f0fff, OP24, N("cis1exec"), REG2 },
	{ -1,    -1, 0x000b0cf2, 0x001f0fff, OP24, N("cixor"), REG2 },
	{ -1,    -1, 0x000c0cf2, 0x001f0fff, OP24, N("ciadd"), REG2 },
	{ -1,    -1, 0x000d0cf2, 0x001f0fff, OP24, N("ciand"), REG2 },
	{ -1,    -1, 0x000e0cf2, 0x001f0fff, OP24, N("cirev"), REG2 },
	{ -1,    -1, 0x000f0cf2, 0x001f0fff, OP24, N("ciprecmac"), REG2 },
	{ -1,    -1, 0x00100cf2, 0x001f0fff, OP24, N("cisecret"), REG2 },
	{ -1,    -1, 0x00110cf2, 0x001f0fff, OP24, N("cikeyreg"), REG2 },
	{ -1,    -1, 0x00120cf2, 0x001f0fff, OP24, N("cikexp"), REG2 },
	{ -1,    -1, 0x00130cf2, 0x001f0fff, OP24, N("cikrexp"), REG2 },
	{ -1,    -1, 0x00140cf2, 0x001f0fff, OP24, N("cienc"), REG2 },
	{ -1,    -1, 0x00150cf2, 0x001f0fff, OP24, N("cidec"), REG2 },
	{ -1,    -1, 0x00170cf2, 0x001f0fff, OP24, N("cisigenc"), REG2 },
	{ -1,    -1, 0x00000cf2, 0x00000fff, OP24, OOPS, REG2 },
	{ -1,    -1, 0x000000f2, 0x000000ff, OP24, OOPS, REG2, IMM8 },

	{ -1,    -1, 0x000000f4, 0x000020fe, T(ol0), N("bra"), T(p), T(bt) },
	{ -1,    -1, 0x000020f4, 0x00003ffe, T(ol0), N("bra"), T(abt), ENDMARK },
	{ -1,    -1, 0x000021f4, 0x00003ffe, T(ol0), N("call"), T(ct) },
	{ -1,    -1, 0x000028f4, 0x00003ffe, T(ol0), N("sleep"), T(fl) }, /* sleeps while given flag is true */
	{ -1,    -1, 0x000030f4, 0x00003ffe, T(ol0), N("add"), SP, T(is) },
	{ -1,    -1, 0x000031f4, 0x00003ffe, T(ol0), N("bset"), FLAGS, T(fl) },
	{ -1,    -1, 0x000032f4, 0x00003ffe, T(ol0), N("bclr"), FLAGS, T(fl) },
	{ -1,    -1, 0x000033f4, 0x00003ffe, T(ol0), N("btgl"), FLAGS, T(fl) },
	{ -1,    -1, 0x00003cf4, 0x00003fff, T(ol0), N("cxset"), IMM8 },
	{ -1,    -1, 0x00003cf5, 0x00003fff, T(ol0), T(cocmd) },
	{ -1,    -1, 0x000000f4, 0x000000fe, T(ol0), OOPS, T(i) },

	{ -1,    -1, 0x000000f8, 0x00000fff, OP16, N("ret"), ENDMARK },
	{ -1,    -1, 0x000001f8, 0x00000fff, OP16, N("iret"), ENDMARK },
	{ -1,    -1, 0x000002f8, 0x00000fff, OP16, N("exit"), ENDMARK },
	{ -1,    -1, 0x000003f8, 0x00000fff, OP16, N("xdwait") },
	{ -1,    -1, 0x000006f8, 0x00000fff, OP16, U("f8/6") },
	{ -1,    -1, 0x000007f8, 0x00000fff, OP16, N("xcwait") },
	{ -1,    -1, 0x000008f8, 0x00000cff, OP16, N("trap"), STRAP },
	{ -1,    -1, 0x000000f8, 0x000000ff, OP16, OOPS },

	{ -1,    -1, 0x000000f9, 0x00000fff, OP16, N("push"), REG2 },
	{ -1,    -1, 0x000001f9, 0x00000fff, OP16, N("add"), SP, REG2 },
	{ -1,    -1, 0x000004f9, 0x00000fff, OP16, N("bra"), REG2 },
	{ -1,    -1, 0x000005f9, 0x00000fff, OP16, N("call"), REG2 },
	{ NVA3P, -1, 0x000008f9, 0x00000fff, OP16, N("unbind"), REG2 }, // drops given physical page's VM tag. page specified as index, not as address.
	{ -1,    -1, 0x000009f9, 0x00000fff, OP16, N("bset"), FLAGS, REG2 },
	{ -1,    -1, 0x00000af9, 0x00000fff, OP16, N("bclr"), FLAGS, REG2 },
	{ -1,    -1, 0x00000bf9, 0x00000fff, OP16, N("btgl"), FLAGS, REG2 },
	{ -1,    -1, 0x000000f9, 0x000000ff, OP16, OOPS, REG2 },

	/* iowr is asynchronous, iowrs waits for completion. */
	{ -1,    -1, 0x000000fa, 0x000f00ff, OP24, N("iowr"), IOR, REG1 },
	{ NVA3P, -1, 0x000100fa, 0x000f00ff, OP24, N("iowrs"), IOR, REG1 },
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
	{ -1,    -1, 0x000400fa, 0x000f00ff, OP24, N("xcld"), REG2, REG1 },
	{ -1,    -1, 0x000500fa, 0x000f00ff, OP24, N("xdld"), REG2, REG1 },
	{ -1,    -1, 0x000600fa, 0x000f00ff, OP24, N("xdst"), REG2, REG1 },
	{ -1,    -1, 0x000800fa, 0x000f00ff, OP24, N("setp"), REG1, REG2 }, /* set given flag if bit0 of ARG2 set */
	{ -1,    -1, 0x000000fa, 0x000000ff, OP24, OOPS, REG2, REG1 },

	{ -1,    -1, 0x000000fc, 0x00000fff, OP16, N("pop"), REG2 },
	{ -1,    -1, 0x000000fc, 0x000000ff, OP16, OOPS, REG2 },

	{ -1,    -1, 0x000000fd, 0x000f00ff, OP24, N("mulu"), REG2, REG1 }, /* no flags */
	{ -1,    -1, 0x000100fd, 0x000f00ff, OP24, N("muls"), REG2, REG1 }, /* no flags */
	{ -1,    -1, 0x000200fd, 0x000f00ff, OP24, N("sex"), REG2, REG1 }, /* flags: SZ */
	{ -1,    -1, 0x000400fd, 0x000f00ff, OP24, N("and"), REG2, REG1 }, /* flags: COSZ [CO: 0] */
	{ -1,    -1, 0x000500fd, 0x000f00ff, OP24, N("or"), REG2, REG1 }, /* flags: COSZ [CO: 0] */
	{ -1,    -1, 0x000600fd, 0x000f00ff, OP24, N("xor"), REG2, REG1 }, /* flags: COSZ [CO: 0] */
	{ -1,    -1, 0x000900fd, 0x000f00ff, OP24, N("bset"), REG2, REG1 }, /* no flags */
	{ -1,    -1, 0x000a00fd, 0x000f00ff, OP24, N("bclr"), REG2, REG1 }, /* no flags */
	{ -1,    -1, 0x000b00fd, 0x000f00ff, OP24, N("btgl"), REG2, REG1 }, /* no flags */
	{ -1,    -1, 0x000000fd, 0x000000ff, OP24, OOPS, REG2, REG1 },

	{ -1,    -1, 0x000000fe, 0x000f00ff, OP24, N("mov"), SREG1, REG2 },
	{ -1,    -1, 0x000100fe, 0x000f00ff, OP24, N("mov"), REG1, SREG2 },
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
	{ NVA3P, -1, 0x000200fe, 0x000f00ff, OP24, N("ptlb"), REG1, REG2 }, /* no flags */
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
	{ NVA3P, -1, 0x000300fe, 0x000f00ff, OP24, N("vtlb"), REG1, REG2 }, /* no flags */
	{ -1,    -1, 0x000c00fe, 0x000f00ff, OP24, N("set"), REG1, FLAGS, REG2 }, /* flags: SZ, sets dst to 1 if given bit set in $flags. XXX: check nv98 */
	{ -1,    -1, 0x000000fe, 0x000000ff, OP24, OOPS, REG1, REG2 },

	{ -1,    -1, 0x000000ff, 0x000f00ff, OP24, N("mulu"), REG3, REG2, REG1 }, /* no flags */
	{ -1,    -1, 0x000100ff, 0x000f00ff, OP24, N("muls"), REG3, REG2, REG1 }, /* no flags */
	{ -1,    -1, 0x000200ff, 0x000f00ff, OP24, N("sex"), REG3, REG2, REG1 }, /* flags: SZ */
	{ NVA3P, -1, 0x000300ff, 0x000f00ff, OP24, N("extrs"), REG3, REG2, REG1 }, /* flags: SZ */
	{ -1,    -1, 0x000400ff, 0x000f00ff, OP24, N("and"), REG3, REG2, REG1 }, /* flags: COSZ [CO: 0] */
	{ -1,    -1, 0x000500ff, 0x000f00ff, OP24, N("or"), REG3, REG2, REG1 }, /* flags: COSZ [CO: 0] */
	{ -1,    -1, 0x000600ff, 0x000f00ff, OP24, N("xor"), REG3, REG2, REG1 }, /* flags: COSZ [CO: 0] */
	{ NVA3P, -1, 0x000700ff, 0x000f00ff, OP24, N("extr"), REG3, REG2, REG1 }, /* flags: SZ */
	{ -1,    -1, 0x000800ff, 0x000f00ff, OP24, N("xbit"), REG3, REG2, REG1 }, /* flags: SZ, NV98: ARG1 = (ARG1 & 0xfffffffe) | (ARG2 >> ARG3 & 1) ; NVA3+: ARG1 = ARG2 >> ARG3 & 1 */
	{ NVA3P, -1, 0x000c00ff, 0x000f00ff, OP24, N("div"), REG3, REG2, REG1 }, /* no flags */
	{ NVA3P, -1, 0x000d00ff, 0x000f00ff, OP24, N("mod"), REG3, REG2, REG1 }, /* no flags */
	{ -1,    -1, 0x000e00ff, 0x000f00ff, OP24, U("ff/e"), REG3, IORR }, /* no flags */
	{ -1,    -1, 0x000f00ff, 0x000f00ff, OP24, N("iord"), REG3, IORR }, /* no flags */
	{ -1,    -1, 0x000000ff, 0x000000ff, OP24, OOPS, REG3, REG2, REG1 },

	{ -1, -1, 0, 0, OOPS },
};

static struct disisa fuc_isa_s = {
	tabm,
	4,
	1,
	1,
};

struct disisa *fuc_isa = &fuc_isa_s;
