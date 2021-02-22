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

#define F_FUC0	1
#define F_FUC3P	2
#define F_FUC4P	4
#define F_CRYPT	8
#define F_FUC5P	0x10
#define F_FUCOLD	0x20
#define F_FUC6P	0x40

/*
 * Code target fields
 */

static struct rbitfield pcrel16off = { { 16, 16 }, RBF_SIGNED, .pcrel = 1, .wrapok = 1 };
static struct rbitfield pcrel8off = { { 16, 8 }, RBF_SIGNED, .pcrel = 1 };
static struct rbitfield fpcrel16off = { { 24, 16 }, RBF_SIGNED, .pcrel = 1, .wrapok = 1 };
static struct rbitfield fpcrel8off = { { 24, 8 }, RBF_SIGNED, .pcrel = 1 };
static struct rbitfield ffpcrel16off = { { 32, 16 }, RBF_SIGNED, .pcrel = 1, .wrapok = 1 };
static struct rbitfield ffpcrel8off = { { 32, 8 }, RBF_SIGNED, .pcrel = 1 };
#define SBTARG atombtarg, &pcrel8off
#define LBTARG atombtarg, &pcrel16off
#define SFBTARG atombtarg, &fpcrel8off
#define LFBTARG atombtarg, &fpcrel16off
#define SFFBTARG atombtarg, &ffpcrel8off
#define LFFBTARG atombtarg, &ffpcrel16off
#define LABTARG atombtarg, &imm16woff
#define SABTARG atombtarg, &imm8off
#define LCTARG atomctarg, &imm16woff
#define SCTARG atomctarg, &imm8off
#define FCTARG atomctarg, &fimm16off
#define LLBTARG atombtarg, &fimm24off
#define LLCTARG atomctarg, &fimm24off

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
	{ 9, "cx", .fmask = F_CRYPT },
	{ 0xa, "cauth", .fmask = F_CRYPT },
	{ 0xb, "xtargets" },
	{ 0xc, "tstatus", .fmask = F_FUC3P },
	{ 0xd, "cauth1", .fmask = F_FUC6P | F_CRYPT },
	{ 0xe, "xcbase1", .fmask = F_FUC6P },
	{ 0xf, "xdbase1", .fmask = F_FUC6P },
	{ -1 },
};
static struct bitfield reg0_bf = { 0, 4 };
static struct bitfield reg1_bf = { 12, 4 };
static struct bitfield reg2_bf = { 8, 4 };
static struct bitfield reg3_bf = { 20, 4 };
static struct bitfield pred1_bf = { 8, 3 };
static struct bitfield pred2_bf = { 16, 3 };
static struct bitfield creg1_bf = { 16, 3 };
static struct bitfield creg2_bf = { 20, 3 };
static struct reg reg0_r = { &reg0_bf, "r" };
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
#define REG0 atomreg, &reg0_r
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

static struct rbitfield imm16off = { 16, 16 };
static struct rbitfield imm8off = { 16, 8 };
static struct rbitfield imm16soff = { { 16, 16 }, RBF_SIGNED };
static struct rbitfield imm8soff = { { 16, 8 }, RBF_SIGNED };
static struct rbitfield imm16woff = { { 16, 16 }, .wrapok = 1 };
static struct rbitfield imm16hoff = { { 16, 16 }, RBF_UNSIGNED, 16 };
static struct rbitfield imm8hoff = { { 16, 8 }, RBF_UNSIGNED, 16 };
static struct rbitfield fimm8soff = { { 8, 8 }, RBF_SIGNED };
static struct rbitfield fimm16soff = { { 8, 16 }, RBF_SIGNED };
static struct rbitfield fimm24soff = { { 8, 24 }, RBF_SIGNED };
static struct rbitfield fimm16off = { 8, 16 };
static struct rbitfield fimm24off = { 8, 24 };
static struct rbitfield fimm32off = { 8, 32 };
static struct bitfield strapoff = { 8, 2 };
static struct bitfield cimm2off = { 20, 6 };
#define IMM16 atomrimm, &imm16off
#define IMM8 atomrimm, &imm8off
#define IMM16S atomrimm, &imm16soff
#define IMM8S atomrimm, &imm8soff
#define IMM16W atomrimm, &imm16woff
#define IMM16H atomrimm, &imm16hoff
#define IMM8H atomrimm, &imm8hoff
#define FIMM8S atomrimm, &fimm8soff
#define FIMM16S atomrimm, &fimm16soff
#define FIMM24S atomrimm, &fimm24soff
#define FIMM24 atomrimm, &fimm24off
#define FIMM32 atomrimm, &fimm32off
#define STRAP atomimm, &strapoff
#define CIMM2 atomimm, &cimm2off

static struct bitfield bitf8bf[] = { { 16, 5 }, { 21, 3 } };
static struct bitfield bitf16bf[] = { { 16, 5 }, { 21, 5 }, };
#define BITF8 atombf, bitf8bf
#define BITF16 atombf, bitf16bf

/*
 * Memory fields
 */

static struct rbitfield off8_bf = { { 16, 8 }, RBF_UNSIGNED, 0 };
static struct rbitfield off16_bf = { { 16, 8 }, RBF_UNSIGNED, 1 };
static struct rbitfield off32_bf = { { 16, 8 }, RBF_UNSIGNED, 2 };
static struct mem datar_m = { "D", 0, &reg1_r };
static struct mem datari8_m = { "D", 0, &reg1_r, &off8_bf };
static struct mem datari16_m = { "D", 0, &reg1_r, &off16_bf };
static struct mem datari32_m = { "D", 0, &reg1_r, &off32_bf };
static struct mem datarr8_m = { "D", 0, &reg1_r, 0, &reg2_r, 0 };
static struct mem datarr16_m = { "D", 0, &reg1_r, 0, &reg2_r, 1 };
static struct mem datarr32_m = { "D", 0, &reg1_r, 0, &reg2_r, 2 };
static struct mem datarralt8_m = { "D", 0, &reg1_r, 0, &reg3_r, 0 };
static struct mem datarralt16_m = { "D", 0, &reg1_r, 0, &reg3_r, 1 };
static struct mem datarralt32_m = { "D", 0, &reg1_r, 0, &reg3_r, 2 };
static struct mem datasp8_m = { "D", 0, &sp_r, &off8_bf };
static struct mem datasp16_m = { "D", 0, &sp_r, &off16_bf };
static struct mem datasp32_m = { "D", 0, &sp_r, &off32_bf };
static struct mem dataspr8_m = { "D", 0, &sp_r, 0, &reg2_r, 0 };
static struct mem dataspr16_m = { "D", 0, &sp_r, 0, &reg2_r, 1 };
static struct mem dataspr32_m = { "D", 0, &sp_r, 0, &reg2_r, 2 };
#define DATAR atommem, &datar_m
#define DATARI8 atommem, &datari8_m
#define DATARI16 atommem, &datari16_m
#define DATARI32 atommem, &datari32_m
#define DATARR8 atommem, &datarr8_m
#define DATARR16 atommem, &datarr16_m
#define DATARR32 atommem, &datarr32_m
#define DATARRALT8 atommem, &datarralt8_m
#define DATARRALT16 atommem, &datarralt16_m
#define DATARRALT32 atommem, &datarralt32_m
#define DATA8SP atommem, &datasp8_m
#define DATA16SP atommem, &datasp16_m
#define DATA32SP atommem, &datasp32_m
#define DATA8SPR atommem, &dataspr8_m
#define DATA16SPR atommem, &dataspr16_m
#define DATA32SPR atommem, &dataspr32_m

/*
 * IO space
 */

static struct mem ior_m = { "I", 0, &reg1_r };
static struct mem iorr_m = { "I", 0, &reg1_r, 0, &reg2_r, 2 };
static struct mem iori_m = { "I", 0, &reg1_r, &off32_bf };
#define IOR atommem, &ior_m
#define IORR atommem, &iorr_m
#define IORI atommem, &iori_m

static struct insn tabp[] = {
	{ 0x00000000, 0x00001800, PRED1 },
	{ 0x00000800, 0x00001f00, N("b") },
	{ 0x00000800, 0x00001f00, N("c") },
	{ 0x00000900, 0x00001f00, N("o") },
	{ 0x00000a00, 0x00001f00, N("s") },
	{ 0x00000b00, 0x00001f00, N("e") },
	{ 0x00000b00, 0x00001f00, N("z") },
	{ 0x00000c00, 0x00001f00, N("a") },
	{ 0x00000d00, 0x00001f00, N("be") },
	{ 0x00000d00, 0x00001f00, N("na") },
	{ 0x00000e00, 0x00001f00, ENDMARK },
	{ 0x00001000, 0x00001800, N("not"), PRED1 },
	{ 0x00001800, 0x00001f00, N("ae") },
	{ 0x00001800, 0x00001f00, N("nb") },
	{ 0x00001800, 0x00001f00, N("nc") },
	{ 0x00001900, 0x00001f00, N("no") },
	{ 0x00001a00, 0x00001f00, N("ns") },
	{ 0x00001b00, 0x00001f00, N("ne") },
	{ 0x00001b00, 0x00001f00, N("nz") },
	{ 0x00001c00, 0x00001f00, N("g"), .fmask = F_FUC3P },
	{ 0x00001d00, 0x00001f00, N("le"), .fmask = F_FUC3P },
	{ 0x00001e00, 0x00001f00, N("l"), .fmask = F_FUC3P },
	{ 0x00001f00, 0x00001f00, N("ge"), .fmask = F_FUC3P },
	{ 0, 0, OOPS },
};

static struct insn tabfl[] = {
	{ 0x00000000, 0x00180000, PRED2 },
	{ 0x00080000, 0x001f0000, N("c") },
	{ 0x00090000, 0x001f0000, N("o") },
	{ 0x000a0000, 0x001f0000, N("s") },
	{ 0x000b0000, 0x001f0000, N("z") },
	{ 0x00100000, 0x001f0000, N("ie0") },
	{ 0x00110000, 0x001f0000, N("ie1") },
	{ 0x00120000, 0x001f0000, N("ie2"), .fmask = F_FUC4P },
	{ 0x00140000, 0x001f0000, N("is0") },
	{ 0x00150000, 0x001f0000, N("is1") },
	{ 0x00160000, 0x001f0000, N("is2"), .fmask = F_FUC4P },
	{ 0x00180000, 0x001f0000, N("ta") },
	{ 0x00000000, 0x00000000, OOPS },
};

F(i, 0, IMM8, IMM16);
F(is, 0, IMM8S, IMM16S);
F(ih, 0, IMM8H, IMM16H);
F(bt, 0, SBTARG, LBTARG);
F(abt, 0, SABTARG, LABTARG);
F(ct, 0, SCTARG, LCTARG);
F(ol0, 0, OP3B, OP4B);

static struct insn tabcocmd[] = {
	{ 0x00000000, 0x80000000, N("cxset"), IMM8 },
	{ 0x84000000, 0xfc000000, N("cmov"), CREG1, CREG2 },
	/* pull 16 bytes from crypt xfer stream */
	{ 0x88000000, 0xfc000000, N("cxsin"), CREG1 },
	/* send 16 bytes to crypt xfer stream */
	{ 0x8c000000, 0xfc000000, N("cxsout"), CREG1 },
	/* fill ARG with 16 random bytes */
	{ 0x90000000, 0xfc000000, N("crnd"), CREG1 },
	/* next ARG coprocessor instructions are stored as a "script" to be executed later. */
	{ 0x94000000, 0xfc000000, N("cs0begin"), CIMM2 },
	/* the script stored by previous insn is executed ARG times. */
	{ 0x98000000, 0xfc000000, N("cs0exec"), CIMM2 },
	/* like above, but operates on another script slot */
	{ 0x9c000000, 0xfc000000, N("cs1begin"), CIMM2 },
	{ 0xa0000000, 0xfc000000, N("cs1exec"), CIMM2 },
	{ 0xa8000000, 0xfc000000, N("cchmod"), CREG1, CIMM2 },
	{ 0xac000000, 0xfc000000, N("cxor"), CREG1, CREG2 },
	{ 0xb0000000, 0xfc000000, N("cadd"), CREG1, CIMM2 }, /* add immediate to register, modulo 2^64 */
	{ 0xb4000000, 0xfc000000, N("cand"), CREG1, CREG2 },
	{ 0xb8000000, 0xfc000000, N("crev"), CREG1, CREG2 }, /* reverse bytes */
	{ 0xbc000000, 0xfc000000, N("cgfmul"), CREG1, CREG2 }, /* shift left by 1. if bit shifted out the left side was set, xor with 0x87. */
	/* Load selected secret to register. The register is subsequently
	 * marked "hidden", and all attempts to xfer stuff from it by
	 * non-authed code will result in just getting 0s. The hidden flag
	 * also propagates through all normal crypto ops to destination reg,
	 * so there's basically no way to get information out of hidden
	 * registers, other than via authed code. */
	{ 0xc0000000, 0xfc000000, N("csecret"), CREG1, CIMM2 },
	/* Binds a register as the key for enc/dec operations. A *register*,
	 * not its contents. So if you bind $c3, then change its value,
	 * it'll use the new value.
	 */
	{ 0xc4000000, 0xfc000000, N("ckeyreg"), CREG1 },
	/* key expansion - go through AES key schedule given the key [aka
	 * first 16 bytes of 176-byte expanded key], return last 16 bytes
	 * of the expended key. Used to get decryption keys.
	 */
	{ 0xc8000000, 0xfc000000, N("ckexp"), CREG1, CREG2 },
	/* reverse operation to the above - get the original key given
	 * last 16 bytes of the expanded key*/
	{ 0xcc000000, 0xfc000000, N("ckrexp"), CREG1, CREG2 },
	/* encrypts a block using "key" register value as key */
	{ 0xd0000000, 0xfc000000, N("cenc"), CREG1, CREG2 },
	/* decrypts a block using "key" register value as last 16 bytes of
	 * expanded key - ie. you need to stuff the key through ckexp before
	 * use for decryption. */
	{ 0xd4000000, 0xfc000000, N("cdec"), CREG1, CREG2 },
	/* Secure BootROM only: compare ARG1 with ARG2 and enter HS mode if they match */
	{ 0xd8000000, 0xfc000000, N("csigcmp"), CREG1, CREG2 },
	/* HS mode only: encrypt the active code signature with ARG2 as key and store the result in ARG1 */
	{ 0xdc000000, 0xfc000000, N("csigenc"), CREG1, CREG2 },
	/* HS mode only: clear the active code signature */
	{ 0xe0000000, 0xfc000000, N("csigclr") },
	{ 0, 0, OOPS },
};

static struct insn tabsz[] = {
	{ 0x00000000, 0x000000c0, N("b8") },
	{ 0x00000040, 0x000000c0, N("b16") },
	{ 0x00000080, 0x000000c0, N("b32") },
	{ 0, 0, OOPS },
};

static struct insn tabdatari[] = {
	{ 0x00000000, 0x000000c0, DATARI8 },
	{ 0x00000040, 0x000000c0, DATARI16 },
	{ 0x00000080, 0x000000c0, DATARI32 },
	{ 0, 0, OOPS },
};

static struct insn tabdatasp[] = {
	{ 0x00000000, 0x000000c0, DATA8SP },
	{ 0x00000040, 0x000000c0, DATA16SP },
	{ 0x00000080, 0x000000c0, DATA32SP },
	{ 0, 0, OOPS },
};

static struct insn tabdatarr[] = {
	{ 0x00000000, 0x000000c0, DATARR8 },
	{ 0x00000040, 0x000000c0, DATARR16 },
	{ 0x00000080, 0x000000c0, DATARR32 },
	{ 0, 0, OOPS },
};

static struct insn tabdatarralt[] = {
	{ 0x00000000, 0x000000c0, DATARRALT8 },
	{ 0x00000040, 0x000000c0, DATARRALT16 },
	{ 0x00000080, 0x000000c0, DATARRALT32 },
	{ 0, 0, OOPS },
};

static struct insn tabdataspr[] = {
	{ 0x00000000, 0x000000c0, DATA8SPR },
	{ 0x00000040, 0x000000c0, DATA16SPR },
	{ 0x00000080, 0x000000c0, DATA32SPR },
	{ 0, 0, OOPS },
};

static struct insn tabsi[] = {
	{ 0x00000000, 0x0000003f, OP3B, N("st"), T(sz), T(datari), REG2, .fmask = F_FUCOLD },
	{ 0x00000000, 0x00000030, OP3B, OOPS, T(sz), REG1, REG2, IMM8, .fmask = F_FUCOLD },

	{ 0x00000000, 0x000000f0, OP2B, N("mov"), REG0, FIMM8S, .fmask = F_FUC5P },
	{ 0x00000040, 0x000000f0, OP3B, N("mov"), REG0, FIMM16S, .fmask = F_FUC5P },
	{ 0x00000080, 0x000000f0, OP4B, N("mov"), REG0, FIMM24S, .fmask = F_FUC5P },

	{ 0x00000010, 0x0000003f, OP3B, N("add"), T(sz), REG2, REG1, IMM8 },
	{ 0x00000011, 0x0000003f, OP3B, N("adc"), T(sz), REG2, REG1, IMM8 },
	{ 0x00000012, 0x0000003f, OP3B, N("sub"), T(sz), REG2, REG1, IMM8 },
	{ 0x00000013, 0x0000003f, OP3B, N("sbb"), T(sz), REG2, REG1, IMM8 },
	{ 0x00000014, 0x0000003f, OP3B, N("shl"), T(sz), REG2, REG1, IMM8 },
	{ 0x00000015, 0x0000003f, OP3B, N("shr"), T(sz), REG2, REG1, IMM8 },
	{ 0x00000017, 0x0000003f, OP3B, N("sar"), T(sz), REG2, REG1, IMM8 },
	{ 0x00000018, 0x0000003f, OP3B, N("ld"), T(sz), REG2, T(datari) },
	{ 0x0000001c, 0x0000003f, OP3B, N("shlc"), T(sz), REG2, REG1, IMM8 },
	{ 0x0000001d, 0x0000003f, OP3B, N("shrc"), T(sz), REG2, REG1, IMM8 },
	{ 0x00000010, 0x00000030, OP3B, OOPS, T(sz), REG2, REG1 },

	{ 0x00000020, 0x0000003f, OP4B, N("add"), T(sz), REG2, REG1, IMM16, .fmask = F_FUCOLD },
	{ 0x00000021, 0x0000003f, OP4B, N("adc"), T(sz), REG2, REG1, IMM16, .fmask = F_FUCOLD },
	{ 0x00000022, 0x0000003f, OP4B, N("sub"), T(sz), REG2, REG1, IMM16, .fmask = F_FUCOLD },
	{ 0x00000023, 0x0000003f, OP4B, N("sbb"), T(sz), REG2, REG1, IMM16, .fmask = F_FUCOLD },
	{ 0x00000020, 0x00000030, OP4B, OOPS, T(sz), REG2, REG1, IMM16, .fmask = F_FUCOLD },

	{ 0x00000020, 0x0000003f, OP2B, N("st"), T(sz), DATAR, REG2, .fmask = F_FUC5P },
	{ 0x00000021, 0x0000003f, OP2B, N("st"), T(sz), T(dataspr), REG1, .fmask = F_FUC5P },
	{ 0x00000024, 0x0000003f, OP2B, N("cmpu"), T(sz), REG1, REG2, .fmask = F_FUC5P },
	{ 0x00000025, 0x0000003f, OP2B, N("cmps"), T(sz), REG1, REG2, .fmask = F_FUC5P },
	{ 0x00000026, 0x0000003f, OP2B, N("cmp"), T(sz), REG1, REG2, .fmask = F_FUC5P },
	{ 0x00000020, 0x00000030, OP2B, OOPS, T(sz), REG1, REG2, .fmask = F_FUC5P },


	{ 0x00000130, 0x00000f3f, T(ol0), N("st"), T(sz), T(datasp), REG1 },
	{ 0x00000430, 0x00000f3e, T(ol0), N("cmpu"), T(sz), REG1, T(i) },
	{ 0x00000530, 0x00000f3e, T(ol0), N("cmps"), T(sz), REG1, T(is) },
	{ 0x00000630, 0x00000f3e, T(ol0), N("cmp"), T(sz), REG1, T(is), .fmask = F_FUC3P },
	{ 0x00000030, 0x0000003e, T(ol0), OOPS, T(sz), REG1, T(i) },

	{ 0x00000032, 0x0000003f, OP2B, N("mov"), T(sz), REG2, REG1, .fmask = F_FUC5P },

	/* XXX not verified yet */
	{ 0x00000033, 0x00000f3f, OP4B, N("bra"), T(sz), REG1, IMM8, N("e"), SFBTARG, .fmask = F_FUC5P },
	{ 0x00000433, 0x00000f3f, OP4B, N("bra"), T(sz), REG1, IMM8, N("ne"), SFBTARG, .fmask = F_FUC5P },
	{ 0x00000933, 0x00000f3f, OP5B, N("bra"), T(sz), REG1, IMM8, N("e"), LFBTARG, .fmask = F_FUC5P },
	{ 0x00000a33, 0x00000f3f, OP5B, N("bra"), T(sz), REG1, IMM16, N("e"), SFFBTARG, .fmask = F_FUC5P },
	{ 0x00000b33, 0x00000f3f, OP6B, N("bra"), T(sz), REG1, IMM16, N("e"), LFFBTARG, .fmask = F_FUC5P },
	{ 0x00000d33, 0x00000f3f, OP5B, N("bra"), T(sz), REG1, IMM8, N("ne"), LFBTARG, .fmask = F_FUC5P },
	{ 0x00000e33, 0x00000f3f, OP5B, N("bra"), T(sz), REG1, IMM16, N("ne"), SFFBTARG, .fmask = F_FUC5P },
	{ 0x00000f33, 0x00000f3f, OP6B, N("bra"), T(sz), REG1, IMM16, N("ne"), LFFBTARG, .fmask = F_FUC5P },

	{ 0x00000034, 0x00000f3f, OP3B, N("ld"), T(sz), REG1, T(datasp) },
	{ 0x00000034, 0x0000003f, OP3B, OOPS, T(sz), REG1, IMM8 },

	{ 0x00000035, 0x0000003f, OP3B, N("st"), T(sz), T(datari), REG2, .fmask = F_FUC5P },

	{ 0x00000036, 0x00000f3e, T(ol0), N("add"), T(sz), REG1, T(i) },
	{ 0x00000136, 0x00000f3e, T(ol0), N("adc"), T(sz), REG1, T(i) },
	{ 0x00000236, 0x00000f3e, T(ol0), N("sub"), T(sz), REG1, T(i) },
	{ 0x00000336, 0x00000f3e, T(ol0), N("sbb"), T(sz), REG1, T(i) },
	{ 0x00000436, 0x00000f3f, T(ol0), N("shl"), T(sz), REG1, T(i) },
	{ 0x00000536, 0x00000f3f, T(ol0), N("shr"), T(sz), REG1, T(i) },
	{ 0x00000736, 0x00000f3f, T(ol0), N("sar"), T(sz), REG1, T(i) },
	{ 0x00000c36, 0x00000f3f, T(ol0), N("shlc"), T(sz), REG1, T(i) },
	{ 0x00000d36, 0x00000f3f, T(ol0), N("shrc"), T(sz), REG1, T(i) },
	{ 0x00000036, 0x0000003e, T(ol0), OOPS, T(sz), REG1, T(i) },

	{ 0x00000038, 0x000f003f, OP3B, N("st"), T(sz), DATAR, REG2, .fmask = F_FUCOLD },
	{ 0x00010038, 0x000f003f, OP3B, N("st"), T(sz), T(dataspr), REG1, .fmask = F_FUCOLD },
	{ 0x00040038, 0x000f003f, OP3B, N("cmpu"), T(sz), REG1, REG2, .fmask = F_FUCOLD },
	{ 0x00050038, 0x000f003f, OP3B, N("cmps"), T(sz), REG1, REG2, .fmask = F_FUCOLD },
	{ 0x00060038, 0x000f003f, OP3B, N("cmp"), T(sz), REG1, REG2, .fmask = F_FUC3P | F_FUCOLD },
	{ 0x00000038, 0x0000003f, OP3B, OOPS, T(sz), REG1, REG2, .fmask = F_FUCOLD },

	{ 0x0000000038, 0x0f0000003f, OP5B, N("add"), T(sz), REG2, REG1, IMM16, .fmask = F_FUC5P },
	{ 0x0100000038, 0x0f0000003f, OP5B, N("adc"), T(sz), REG2, REG1, IMM16, .fmask = F_FUC5P },
	{ 0x0200000038, 0x0f0000003f, OP5B, N("sub"), T(sz), REG2, REG1, IMM16, .fmask = F_FUC5P },
	{ 0x0300000038, 0x0f0000003f, OP5B, N("sbb"), T(sz), REG2, REG1, IMM16, .fmask = F_FUC5P },
	{ 0x00000038, 0x0000003f, OP5B, OOPS, T(sz), REG2, REG1, IMM16, .fmask = F_FUC5P },

	{ 0x00000039, 0x000f003f, OP3B, N("not"), T(sz), REG2, REG1 },
	{ 0x00010039, 0x000f003f, OP3B, N("neg"), T(sz), REG2, REG1 },
	{ 0x00020039, 0x000f003f, OP3B, N("movf"), T(sz), REG2, REG1, .fmask = F_FUC0 },
	{ 0x00020039, 0x000f003f, OP3B, N("mov"), T(sz), REG2, REG1, .fmask = F_FUC3P | F_FUCOLD },
	{ 0x00030039, 0x000f003f, OP3B, N("hswap"), T(sz), REG2, REG1 },
	{ 0x00000039, 0x0000003f, OP3B, OOPS, T(sz), REG2, REG1 },

	{ 0x0000003a, 0x000f003f, OP3B, N("ld"), T(sz), REG1, T(dataspr) },
	{ 0x0000003a, 0x0000003f, OP3B, OOPS, T(sz), REG1, REG2 },

	{ 0x0000003b, 0x000f003f, OP3B, N("add"), T(sz), REG1, REG2 },
	{ 0x0001003b, 0x000f003f, OP3B, N("adc"), T(sz), REG1, REG2 },
	{ 0x0002003b, 0x000f003f, OP3B, N("sub"), T(sz), REG1, REG2 },
	{ 0x0003003b, 0x000f003f, OP3B, N("sbb"), T(sz), REG1, REG2 },
	{ 0x0004003b, 0x000f003f, OP3B, N("shl"), T(sz), REG1, REG2 },
	{ 0x0005003b, 0x000f003f, OP3B, N("shr"), T(sz), REG1, REG2 },
	{ 0x0007003b, 0x000f003f, OP3B, N("sar"), T(sz), REG1, REG2 },
	{ 0x000c003b, 0x000f003f, OP3B, N("shlc"), T(sz), REG1, REG2 },
	{ 0x000d003b, 0x000f003f, OP3B, N("shrc"), T(sz), REG1, REG2 },
	{ 0x0000003b, 0x0000003f, OP3B, OOPS, T(sz), REG1, REG2 },

	{ 0x0000003c, 0x000f003f, OP3B, N("add"), T(sz), REG3, REG1, REG2 },
	{ 0x0001003c, 0x000f003f, OP3B, N("adc"), T(sz), REG3, REG1, REG2 },
	{ 0x0002003c, 0x000f003f, OP3B, N("sub"), T(sz), REG3, REG1, REG2 },
	{ 0x0003003c, 0x000f003f, OP3B, N("sbb"), T(sz), REG3, REG1, REG2 },
	{ 0x0004003c, 0x000f003f, OP3B, N("shl"), T(sz), REG3, REG1, REG2 },
	{ 0x0005003c, 0x000f003f, OP3B, N("shr"), T(sz), REG3, REG1, REG2 },
	{ 0x0007003c, 0x000f003f, OP3B, N("sar"), T(sz), REG3, REG1, REG2 },
	{ 0x0008003c, 0x000f003f, OP3B, N("ld"), T(sz), REG3, T(datarr) },
	{ 0x0009003c, 0x000f003f, OP3B, N("st"), T(sz), T(datarralt), REG2, .fmask = F_FUC5P },
	{ 0x000c003c, 0x000f003f, OP3B, N("shlc"), T(sz), REG3, REG1, REG2 },
	{ 0x000d003c, 0x000f003f, OP3B, N("shrc"), T(sz), REG3, REG1, REG2 },
	{ 0x0000003c, 0x0000003f, OP3B, OOPS, T(sz), REG3, REG1, REG2 },
	
	{ 0x0000003d, 0x00000f3f, OP2B, N("not"), T(sz), REG1 },
	{ 0x0000013d, 0x00000f3f, OP2B, N("neg"), T(sz), REG1 },
	{ 0x0000023d, 0x00000f3f, OP2B, N("movf"), T(sz), REG1, .fmask = F_FUC0 },
	{ 0x0000023d, 0x00000f3f, OP2B, N("mov"), T(sz), REG1, .fmask = F_FUC3P },
	{ 0x0000033d, 0x00000f3f, OP2B, N("hswap"), T(sz), REG1 },
	{ 0x0000043d, 0x00000f3f, OP2B, N("clear"), T(sz), REG1 },
	{ 0x0000053d, 0x00000f3f, OP2B, N("setf"), T(sz), REG1, .fmask = F_FUC3P },
	{ 0x0000003d, 0x0000003f, OP2B, OOPS, T(sz), REG1 },

	{ 0x0000003e, 0x000000ff, OP4B, N("lbra"), LLBTARG, .fmask = F_FUC4P },
	{ 0x0000007e, 0x000000ff, OP4B, N("lcall"), LLCTARG, .fmask = F_FUC4P },
	{ 0x000000be, 0x000000ff, OP4B, OOPS, FIMM24, .fmask = F_FUC4P },

	{ 0x0000003f, 0x0000003f, OP2B, N("ld"), T(sz), REG2, DATAR, .fmask = F_FUC5P },

	{ 0, 0, OOPS, T(sz) },
};

static struct insn tabm[] = {
	{ 0x00000000, 0x000000c0, T(si) },
	{ 0x00000040, 0x000000c0, T(si) },
	{ 0x00000080, 0x000000c0, T(si) },

	{ 0x000000c0, 0x000000ff, OP3B, N("mulu"), REG2, REG1, IMM8 },
	{ 0x000000c1, 0x000000ff, OP3B, N("muls"), REG2, REG1, IMM8S },
	{ 0x000000c2, 0x000000ff, OP3B, N("sext"), REG2, REG1, IMM8 },
	{ 0x000000c3, 0x000000ff, OP3B, N("extrs"), REG2, REG1, BITF8, .fmask = F_FUC3P },
	{ 0x000000c4, 0x000000ff, OP3B, N("and"), REG2, REG1, IMM8 },
	{ 0x000000c5, 0x000000ff, OP3B, N("or"), REG2, REG1, IMM8 },
	{ 0x000000c6, 0x000000ff, OP3B, N("xor"), REG2, REG1, IMM8 },
	{ 0x000000c7, 0x000000ff, OP3B, N("extr"), REG2, REG1, BITF8, .fmask = F_FUC3P },
	{ 0x000000c8, 0x000000ff, OP3B, N("xbit"), REG2, REG1, IMM8 },
	{ 0x000000cb, 0x000000ff, OP3B, N("ins"), REG2, REG1, BITF8, .fmask = F_FUC3P },
	{ 0x000000cc, 0x000000ff, OP3B, N("div"), REG2, REG1, IMM8, .fmask = F_FUC3P },
	{ 0x000000cd, 0x000000ff, OP3B, N("mod"), REG2, REG1, IMM8, .fmask = F_FUC3P },
	{ 0x000000ce, 0x000000ff, OP3B, N("iords"), REG2, IORI },
	{ 0x000000cf, 0x000000ff, OP3B, N("iord"), REG2, IORI },
	{ 0x000000c0, 0x000000f0, OP3B, OOPS, REG2, REG1, IMM8 },

	{ 0x000000d0, 0x000000ff, OP3B, N("iowr"), IORI, REG2, .fmask = F_FUCOLD },
	{ 0x000000d1, 0x000000ff, OP3B, N("iowrs"), IORI, REG2, .fmask = F_FUC3P | F_FUCOLD },
	{ 0x000000d0, 0x000000f0, OP3B, OOPS, REG1, REG2, IMM8, .fmask = F_FUCOLD },

	{ 0x000000d0, 0x000000f0, OP5B, N("mov"), REG0, FIMM32, .fmask = F_FUC5P },

	{ 0x000000e0, 0x000000ff, OP4B, N("mulu"), REG2, REG1, IMM16 },
	{ 0x000000e1, 0x000000ff, OP4B, N("muls"), REG2, REG1, IMM16S },
	{ 0x000000e3, 0x000000ff, OP4B, N("extrs"), REG2, REG1, BITF16, .fmask = F_FUC3P },
	{ 0x000000e4, 0x000000ff, OP4B, N("and"), REG2, REG1, IMM16 },
	{ 0x000000e5, 0x000000ff, OP4B, N("or"), REG2, REG1, IMM16 },
	{ 0x000000e6, 0x000000ff, OP4B, N("xor"), REG2, REG1, IMM16 },
	{ 0x000000e7, 0x000000ff, OP4B, N("extr"), REG2, REG1, BITF16, .fmask = F_FUC3P },
	{ 0x000000eb, 0x000000ff, OP4B, N("ins"), REG2, REG1, BITF16, .fmask = F_FUC3P },
	{ 0x000000ec, 0x000000ff, OP4B, N("div"), REG2, REG1, IMM16, .fmask = F_FUC3P },
	{ 0x000000ed, 0x000000ff, OP4B, N("mod"), REG2, REG1, IMM16, .fmask = F_FUC3P },
	{ 0x000000e0, 0x000000f0, OP4B, OOPS, REG2, REG1, IMM16 },

	{ 0x000000f0, 0x00000ffe, T(ol0), N("mulu"), REG1, T(i) },
	{ 0x000001f0, 0x00000ffe, T(ol0), N("muls"), REG1, T(is) },
	{ 0x000002f0, 0x00000fff, T(ol0), N("sext"), REG1, T(i) },
	{ 0x000003f0, 0x00000ffe, T(ol0), N("sethi"), REG1, T(ih) },
	{ 0x000004f0, 0x00000ffe, T(ol0), N("and"), REG1, T(i) },
	{ 0x000005f0, 0x00000ffe, T(ol0), N("or"), REG1, T(i) },
	{ 0x000006f0, 0x00000ffe, T(ol0), N("xor"), REG1, T(i) },
	{ 0x000007f0, 0x00000ffe, T(ol0), N("mov"), REG1, T(is), .fmask = F_FUCOLD },
	{ 0x000007f1, 0x00000fff, T(ol0), N("movw"), REG1, IMM16W, .fmask = F_FUCOLD },
	{ 0x000009f0, 0x00000fff, T(ol0), N("bset"), REG1, T(i) },
	{ 0x00000af0, 0x00000fff, T(ol0), N("bclr"), REG1, T(i) },
	{ 0x00000bf0, 0x00000fff, T(ol0), N("btgl"), REG1, T(i) },
	{ 0x00000cf0, 0x00000fff, T(ol0), N("xbit"), REG1, FLAGS, T(fl) },
	{ 0x000000f0, 0x000000fe, T(ol0), OOPS, REG1, T(i) },

	{ 0x000008f2, 0x00000fff, OP3B, N("setp"), T(fl), REG1 },
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
	{ 0x00010cf2, 0x001f0fff, OP3B, N("cimov"), REG1, .fmask = F_CRYPT },
	{ 0x00020cf2, 0x001f0fff, OP3B, N("cixsin"), REG1, .fmask = F_CRYPT },
	{ 0x00030cf2, 0x001f0fff, OP3B, N("cixsout"), REG1, .fmask = F_CRYPT },
	{ 0x00040cf2, 0x001f0fff, OP3B, N("cirnd"), REG1, .fmask = F_CRYPT },
	{ 0x00050cf2, 0x001f0fff, OP3B, N("cis0begin"), REG1, .fmask = F_CRYPT },
	{ 0x00060cf2, 0x001f0fff, OP3B, N("cis0exec"), REG1, .fmask = F_CRYPT },
	{ 0x00070cf2, 0x001f0fff, OP3B, N("cis1begin"), REG1, .fmask = F_CRYPT },
	{ 0x00080cf2, 0x001f0fff, OP3B, N("cis1exec"), REG1, .fmask = F_CRYPT },
	{ 0x000a0cf2, 0x001f0fff, OP3B, N("cichmod"), REG1, .fmask = F_CRYPT },
	{ 0x000b0cf2, 0x001f0fff, OP3B, N("cixor"), REG1, .fmask = F_CRYPT },
	{ 0x000c0cf2, 0x001f0fff, OP3B, N("ciadd"), REG1, .fmask = F_CRYPT },
	{ 0x000d0cf2, 0x001f0fff, OP3B, N("ciand"), REG1, .fmask = F_CRYPT },
	{ 0x000e0cf2, 0x001f0fff, OP3B, N("cirev"), REG1, .fmask = F_CRYPT },
	{ 0x000f0cf2, 0x001f0fff, OP3B, N("cigfmul"), REG1, .fmask = F_CRYPT },
	{ 0x00100cf2, 0x001f0fff, OP3B, N("cisecret"), REG1, .fmask = F_CRYPT },
	{ 0x00110cf2, 0x001f0fff, OP3B, N("cikeyreg"), REG1, .fmask = F_CRYPT },
	{ 0x00120cf2, 0x001f0fff, OP3B, N("cikexp"), REG1, .fmask = F_CRYPT },
	{ 0x00130cf2, 0x001f0fff, OP3B, N("cikrexp"), REG1, .fmask = F_CRYPT },
	{ 0x00140cf2, 0x001f0fff, OP3B, N("cienc"), REG1, .fmask = F_CRYPT },
	{ 0x00150cf2, 0x001f0fff, OP3B, N("cidec"), REG1, .fmask = F_CRYPT },
	{ 0x00160cf2, 0x001f0fff, OP3B, N("cisigcmp"), REG1, .fmask = F_CRYPT },
	{ 0x00170cf2, 0x001f0fff, OP3B, N("cisigenc"), REG1, .fmask = F_CRYPT },
	{ 0x00180cf2, 0x001f0fff, OP3B, N("cisigclr"), REG1, .fmask = F_CRYPT },
	{ 0x00000cf2, 0x00000fff, OP3B, OOPS, REG1, .fmask = F_CRYPT },
	{ 0x000000f2, 0x000000ff, OP3B, OOPS, REG1, IMM8 },

	{ 0x000000f3, 0x000000ff, OP3B, N("call"), FCTARG, .fmask = F_FUC5P },

	{ 0x000000f4, 0x000020fe, T(ol0), N("bra"), T(p), T(bt) },
	{ 0x000020f4, 0x00003ffe, T(ol0), N("bra"), T(abt), ENDMARK },
	{ 0x000021f4, 0x00003fff, T(ol0), N("call"), T(ct) },
	{ 0x000021f5, 0x00003fff, T(ol0), N("call"), T(ct), .fmask = F_FUCOLD },
	{ 0x000028f4, 0x00003fff, T(ol0), N("sleep"), T(fl) }, /* sleeps while given flag is true */
	{ 0x000030f4, 0x00003ffe, T(ol0), N("add"), SP, T(is) },
	{ 0x000031f4, 0x00003fff, T(ol0), N("bset"), FLAGS, T(fl) },
	{ 0x000032f4, 0x00003fff, T(ol0), N("bclr"), FLAGS, T(fl) },
	{ 0x000033f4, 0x00003fff, T(ol0), N("btgl"), FLAGS, T(fl) },
	{ 0x00003cf4, 0x00003fff, T(ol0), N("cxset"), IMM8, .fmask = F_CRYPT },
	{ 0x00003cf5, 0x00003fff, T(ol0), T(cocmd), .fmask = F_CRYPT },
	{ 0x000000f4, 0x000000fe, T(ol0), OOPS, T(i) },

	{ 0x000000f6, 0x000000ff, OP3B, N("iowr"), IORI, REG2, .fmask = F_FUC5P },
	{ 0x000000f7, 0x000000ff, OP3B, N("iowrs"), IORI, REG2, .fmask = F_FUC5P },

	{ 0x000000f8, 0x00000fff, OP2B, N("ret"), ENDMARK },
	{ 0x000001f8, 0x00000fff, OP2B, N("iret"), ENDMARK },
	{ 0x000002f8, 0x00000fff, OP2B, N("exit"), ENDMARK },
	{ 0x000003f8, 0x00000fff, OP2B, N("xdwait") },
	{ 0x000006f8, 0x00000fff, OP2B, N("xdfence") },
	{ 0x000007f8, 0x00000fff, OP2B, N("xcwait") },
	{ 0x000008f8, 0x00000cff, OP2B, N("trap"), STRAP },
	{ 0x000000f8, 0x000000ff, OP2B, OOPS },

	{ 0x000000f9, 0x00000fff, OP2B, N("push"), REG1 },
	{ 0x000001f9, 0x00000fff, OP2B, N("add"), SP, REG1 },
	/* Display these in a better way, perhaps? */
	{ 0x000002f9, 0x00000fff, OP2B, N("mpush"), REG1, .fmask = F_FUC5P },
	{ 0x000004f9, 0x00000fff, OP2B, N("bra"), REG1 },
	{ 0x000005f9, 0x00000fff, OP2B, N("call"), REG1 },
	{ 0x000008f9, 0x00000fff, OP2B, N("itlb"), REG1, .fmask = F_FUC3P },
	{ 0x000009f9, 0x00000fff, OP2B, N("bset"), FLAGS, REG1 },
	{ 0x00000af9, 0x00000fff, OP2B, N("bclr"), FLAGS, REG1 },
	{ 0x00000bf9, 0x00000fff, OP2B, N("btgl"), FLAGS, REG1 },
	{ 0x000000f9, 0x000000ff, OP2B, OOPS, REG1 },

	{ 0x000000fa, 0x000f00ff, OP3B, N("iowr"), IOR, REG2 },
	{ 0x000100fa, 0x000f00ff, OP3B, N("iowrs"), IOR, REG2, .fmask = F_FUC3P },
	{ 0x000400fa, 0x000f00ff, OP3B, N("xcld"), REG1, REG2 },
	{ 0x000500fa, 0x000f00ff, OP3B, N("xdld"), REG1, REG2 },
	{ 0x000600fa, 0x000f00ff, OP3B, N("xdst"), REG1, REG2 },
	{ 0x000700fa, 0x000f00ff, OP3B, U("fa/7"), REG2, REG1, .fmask = F_FUC6P },
	{ 0x000800fa, 0x000f00ff, OP3B, N("setp"), REG2, REG1 },
	{ 0x000000fa, 0x000000ff, OP3B, OOPS, REG1, REG2 },

	/* Display these in a better way, perhaps? */
	{ 0x000000fb, 0x000007ff, OP2B, N("mpop"), REG1, .fmask = F_FUC5P },
	{ 0x000001fb, 0x000007ff, OP2B, N("mpopret"), REG1, ENDMARK, .fmask = F_FUC5P },
	{ 0x000002fb, 0x000007ff, OP4B, N("mpopadd"), REG1, IMM16S, .fmask = F_FUC5P },
	{ 0x000003fb, 0x000007ff, OP4B, N("mpopaddret"), REG1, IMM16S, ENDMARK, .fmask = F_FUC5P },
	{ 0x000004fb, 0x000007ff, OP3B, N("mpopadd"), REG1, IMM8S, .fmask = F_FUC5P },
	{ 0x000005fb, 0x000007ff, OP3B, N("mpopaddret"), REG1, IMM8S, ENDMARK, .fmask = F_FUC5P },

	{ 0x000000fc, 0x00000fff, OP2B, N("pop"), REG1 },
	{ 0x000000fc, 0x000000ff, OP2B, OOPS, REG1 },

	{ 0x000000fd, 0x000f00ff, OP3B, N("mulu"), REG1, REG2 },
	{ 0x000100fd, 0x000f00ff, OP3B, N("muls"), REG1, REG2 },
	{ 0x000200fd, 0x000f00ff, OP3B, N("sext"), REG1, REG2 },
	{ 0x000400fd, 0x000f00ff, OP3B, N("and"), REG1, REG2 },
	{ 0x000500fd, 0x000f00ff, OP3B, N("or"), REG1, REG2 },
	{ 0x000600fd, 0x000f00ff, OP3B, N("xor"), REG1, REG2 },
	{ 0x000900fd, 0x000f00ff, OP3B, N("bset"), REG1, REG2 },
	{ 0x000a00fd, 0x000f00ff, OP3B, N("bclr"), REG1, REG2 },
	{ 0x000b00fd, 0x000f00ff, OP3B, N("btgl"), REG1, REG2 },
	{ 0x000000fd, 0x000000ff, OP3B, OOPS, REG1, REG2 },

	{ 0x000000fe, 0x000f00ff, OP3B, N("mov"), SREG2, REG1 },
	{ 0x000100fe, 0x000f00ff, OP3B, N("mov"), REG2, SREG1 },
	{ 0x000200fe, 0x000f00ff, OP3B, N("ptlb"), REG2, REG1, .fmask = F_FUC3P },
	{ 0x000300fe, 0x000f00ff, OP3B, N("vtlb"), REG2, REG1, .fmask = F_FUC3P },
	{ 0x000500fe, 0x000f00ff, OP3B, U("fe/5"), REG2, REG1, .fmask = F_FUC6P },
	{ 0x000c00fe, 0x000f00ff, OP3B, N("xbit"), REG2, FLAGS, REG1 },
	{ 0x000000fe, 0x000000ff, OP3B, OOPS, REG2, REG1 },

	{ 0x000000ff, 0x000f00ff, OP3B, N("mulu"), REG3, REG1, REG2 },
	{ 0x000100ff, 0x000f00ff, OP3B, N("muls"), REG3, REG1, REG2 },
	{ 0x000200ff, 0x000f00ff, OP3B, N("sext"), REG3, REG1, REG2 },
	{ 0x000300ff, 0x000f00ff, OP3B, N("extrs"), REG3, REG1, REG2, .fmask = F_FUC3P },
	{ 0x000400ff, 0x000f00ff, OP3B, N("and"), REG3, REG1, REG2 },
	{ 0x000500ff, 0x000f00ff, OP3B, N("or"), REG3, REG1, REG2 },
	{ 0x000600ff, 0x000f00ff, OP3B, N("xor"), REG3, REG1, REG2 },
	{ 0x000700ff, 0x000f00ff, OP3B, N("extr"), REG3, REG1, REG2, .fmask = F_FUC3P },
	{ 0x000800ff, 0x000f00ff, OP3B, N("xbit"), REG3, REG1, REG2 },
	{ 0x000c00ff, 0x000f00ff, OP3B, N("div"), REG3, REG1, REG2, .fmask = F_FUC3P },
	{ 0x000d00ff, 0x000f00ff, OP3B, N("mod"), REG3, REG1, REG2, .fmask = F_FUC3P },
	{ 0x000e00ff, 0x000f00ff, OP3B, N("iords"), REG3, IORR },
	{ 0x000f00ff, 0x000f00ff, OP3B, N("iord"), REG3, IORR },
	{ 0x000000ff, 0x000000ff, OP3B, OOPS, REG3, REG1, REG2 },

	{ 0, 0, OOPS },
};

static void falcon_prep(struct disisa *isa) {
	isa->vardata = vardata_new("falcon");
	int f_fuc0op = vardata_add_feature(isa->vardata, "fuc0op", "v0 exclusive opcodes");
	/* XXX rename variants */
	int f_fuc3op = vardata_add_feature(isa->vardata, "fuc3op", "v3+ opcodes");
	int f_fuc4op = vardata_add_feature(isa->vardata, "fuc4op", "v4+ opcodes");
	int f_crypt = vardata_add_feature(isa->vardata, "crypt", "Cryptographic coprocessor");
	int f_fuc5op = vardata_add_feature(isa->vardata, "fuc5op", "v5+ opcodes");
	int f_fucold = vardata_add_feature(isa->vardata, "fucold", "v0-v4 opcodes");
	int f_fuc6op = vardata_add_feature(isa->vardata, "fuc6op", "v6+ opcodes");
	if (f_fuc0op == -1 || f_fuc3op == -1 || f_fuc4op == -1 || f_crypt == -1 || f_fuc5op == -1 || f_fucold == -1 || f_fuc6op == -1 )
		abort();
	int vs_fucver = vardata_add_varset(isa->vardata, "version", "falcon version");
	if (vs_fucver == -1)
		abort();
	int v_fuc0 = vardata_add_variant(isa->vardata, "fuc0", "falcon v0", vs_fucver);
	int v_fuc3 = vardata_add_variant(isa->vardata, "fuc3", "falcon v3", vs_fucver);
	int v_fuc4 = vardata_add_variant(isa->vardata, "fuc4", "falcon v4", vs_fucver);
	int v_fuc5 = vardata_add_variant(isa->vardata, "fuc5", "falcon v5", vs_fucver);
	int v_fuc6 = vardata_add_variant(isa->vardata, "fuc6", "falcon v6", vs_fucver);
	if (v_fuc0 == -1 || v_fuc3 == -1 || v_fuc4 == -1 || v_fuc5 == -1 || v_fuc6 == -1)
		abort();
	vardata_variant_feature(isa->vardata, v_fuc0, f_fuc0op);
	vardata_variant_feature(isa->vardata, v_fuc0, f_fucold);
	vardata_variant_feature(isa->vardata, v_fuc3, f_fuc3op);
	vardata_variant_feature(isa->vardata, v_fuc3, f_fucold);
	vardata_variant_feature(isa->vardata, v_fuc4, f_fuc3op);
	vardata_variant_feature(isa->vardata, v_fuc4, f_fucold);
	vardata_variant_feature(isa->vardata, v_fuc4, f_fuc4op);
	vardata_variant_feature(isa->vardata, v_fuc5, f_fuc3op);
	vardata_variant_feature(isa->vardata, v_fuc5, f_fuc4op);
	vardata_variant_feature(isa->vardata, v_fuc5, f_fuc5op);
	vardata_variant_feature(isa->vardata, v_fuc6, f_fuc3op);
	vardata_variant_feature(isa->vardata, v_fuc6, f_fuc4op);
	vardata_variant_feature(isa->vardata, v_fuc6, f_fuc5op);
	vardata_variant_feature(isa->vardata, v_fuc6, f_fuc6op);
	if (vardata_validate(isa->vardata))
		abort();
}

struct disisa falcon_isa_s = {
	tabm,
	6,
	1,
	1,
	.prep = falcon_prep,
};
