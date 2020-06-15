/*
 * Copyright (C) 2010-2011 Marcelina Kościelnicka <mwk@0x04.net>
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "dis-intern.h"

#define F_VP2 1
#define F_VP3 2
#define F_VP4 4

/*
 * Code target field
 *
 * This field represents a code address and is used for branching and the
 * likes. Target is counted in 32-bit words from the start of microcode.
 */

static struct rbitfield ctargoff = { 8, 11 };
static struct rbitfield rbrtargoff = { 34, 6, .pcrel = 1 };
#define BTARG atombtarg, &ctargoff
#define CTARG atomctarg, &ctargoff
#define RBRTARG atombtarg, &rbrtargoff

static struct rbitfield imm4off = { 12, 4 };
static struct rbitfield imm6off = { { 12, 4, 24, 2 } };
static struct rbitfield imm12off = { { 8, 8, 20, 4 } };
static struct rbitfield imm14off = { { 8, 8, 20, 6 } };
static struct rbitfield bimmldoff = { { 12, 4, 20, 6 } };
static struct rbitfield bimmldsoff = { { 12, 4, 24, 2 } };
static struct rbitfield bimmstoff = { 16, 10 };
static struct rbitfield bimmstsoff = { 16, 4, 24, 2 };
#define IMM4 atomrimm, &imm4off
#define IMM6 atomrimm, &imm6off
#define IMM12 atomrimm, &imm12off
#define IMM14 atomrimm, &imm14off

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
	{ 0, "baddr", .fmask = F_VP2 },
	{ 1, "bsel", .fmask = F_VP2 },
	{ 2, "spidx" },
	{ 3, "asel", .fmask = F_VP2 },
	{ 3, "absel", .fmask = F_VP3 },
	{ 4, "h2v" },
	{ 5, "v2h" },
	{ 6, "stat" },
	{ 7, "parm" },
	{ 8, "pc" },
	{ 9, "cspos" },
	{ 10, "cstop" },
	{ 11, "rpitab", .fmask = F_VP2 },
	{ 12, "lhi" },
	{ 13, "llo" },
	{ 14, "pred" },
	{ 15, "icnt" },
	{ 16, "mvxl0" },
	{ 17, "mvyl0" },
	{ 18, "mvxl1" },
	{ 19, "mvyl1" },
	{ 20, "refl0" },
	{ 21, "refl1" },
	{ 22, "rpil0" },
	{ 23, "rpil1" },
	{ 24, "mbflags" },
	{ 25, "qpy" },
	{ 26, "qpc" },
	{ 27, "mbpart" },
	{ 28, "mbxy" },
	{ 29, "mbaddr" },
	{ 30, "mbtype" },
	{ 31, "submbtype", .fmask = F_VP2 },
	{ 32, "amvxl0" },
	{ 33, "amvyl0" },
	{ 34, "amvxl1" },
	{ 35, "amvyl1" },
	{ 36, "arefl0" },
	{ 37, "arefl1" },
	{ 38, "arpil0" },
	{ 39, "arpil1" },
	{ 40, "ambflags" },
	{ 41, "aqpy", .fmask = F_VP2 },
	{ 42, "aqpc", .fmask = F_VP2 },
	{ 48, "bmvxl0" },
	{ 49, "bmvyl0" },
	{ 50, "bmvxl1" },
	{ 51, "bmvyl1" },
	{ 52, "brefl0" },
	{ 53, "brefl1" },
	{ 54, "brpil0" },
	{ 55, "brpil1" },
	{ 56, "bmbflags" },
	{ 57, "bqpy" },
	{ 58, "bqpc" },
	{ -1 },
};

static struct bitfield src1_bf = { 8, 4 };
static struct bitfield srsrc_bf = { 8, 4, 24, 2 };
static struct bitfield src2_bf = { 12, 4 };
static struct bitfield dst_bf = { 16, 4 };
static struct bitfield srdst_bf = { 16, 4, 24, 2 };
static struct bitfield pred_bf = { 20, 4 };
static struct bitfield rbrpred_bf = { 30, 3, .addend = 8 };
static struct reg src1_r = { &src1_bf, "r", .specials = reg_sr };
static struct reg src2_r = { &src2_bf, "r", .specials = reg_sr };
static struct reg dst_r = { &dst_bf, "r", .specials = reg_sr };
static struct reg srdst_r = { &srdst_bf, "sr", .specials = sreg_sr, .always_special = 1 };
static struct reg srsrc_r = { &srsrc_bf, "sr", .specials = sreg_sr, .always_special = 1 };
static struct reg psrc1_r = { &src1_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg psrc2_r = { &src2_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg pdst_r = { &dst_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg pred_r = { &pred_bf, "p", .cool = 1, .specials = pred_sr };
static struct reg rbrpred_r = { &rbrpred_bf, "p", .cool = 1, .specials = pred_sr };
#define SRC1 atomreg, &src1_r
#define SRC2 atomreg, &src2_r
#define DST atomreg, &dst_r
#define SRSRC atomreg, &srsrc_r
#define SRDST atomreg, &srdst_r
#define PSRC1 atomreg, &psrc1_r
#define PSRC2 atomreg, &psrc2_r
#define PDST atomreg, &pdst_r
#define PRED atomreg, &pred_r
#define RBRPRED atomreg, &rbrpred_r

static struct mem dmemldi_m = { "D", 0, &src1_r, &bimmldoff };
static struct mem dmemldis_m = { "D", 0, &src1_r, &bimmldsoff };
static struct mem dmemldr_m = { "D", 0, &src1_r, 0, &src2_r };
static struct mem dmemsti_m = { "D", 0, &src1_r, &bimmstoff };
static struct mem dmemstis_m = { "D", 0, &src1_r, &bimmstsoff };
static struct mem dmemstr_m = { "D", 0, &dst_r, 0, &src1_r, 1 };
#define DMEMLDI atommem, &dmemldi_m
#define DMEMLDIS atommem, &dmemldis_m
#define DMEMLDR atommem, &dmemldr_m
#define DMEMSTI atommem, &dmemsti_m
#define DMEMSTIS atommem, &dmemstis_m
#define DMEMSTR atommem, &dmemstr_m

static struct mem b6memldi_m = { "B6", 0, &src1_r, &bimmldoff };
static struct mem b6memldis_m = { "B6", 0, &src1_r, &bimmldsoff };
static struct mem b6memldr_m = { "B6", 0, &src1_r, 0, &src2_r };
static struct mem b6memsti_m = { "B6", 0, &src1_r, &bimmstoff };
static struct mem b6memstis_m = { "B6", 0, &src1_r, &bimmstsoff };
static struct mem b6memstr_m = { "B6", 0, &dst_r, 0, &src1_r, 1 };
#define B6MEMLDI atommem, &b6memldi_m
#define B6MEMLDIS atommem, &b6memldis_m
#define B6MEMLDR atommem, &b6memldr_m
#define B6MEMSTI atommem, &b6memsti_m
#define B6MEMSTIS atommem, &b6memstis_m
#define B6MEMSTR atommem, &b6memstr_m

static struct mem b7memldi_m = { "B7", 0, &src1_r, &bimmldoff };
static struct mem b7memldis_m = { "B7", 0, &src1_r, &bimmldsoff };
static struct mem b7memldr_m = { "B7", 0, &src1_r, 0, &src2_r };
static struct mem b7memsti_m = { "B7", 0, &src1_r, &bimmstoff };
static struct mem b7memstis_m = { "B7", 0, &src1_r, &bimmstsoff };
static struct mem b7memstr_m = { "B7", 0, &dst_r, 0, &src1_r, 1 };
#define B7MEMLDI atommem, &b7memldi_m
#define B7MEMLDIS atommem, &b7memldis_m
#define B7MEMLDR atommem, &b7memldr_m
#define B7MEMSTI atommem, &b7memsti_m
#define B7MEMSTIS atommem, &b7memstis_m
#define B7MEMSTR atommem, &b7memstr_m

static struct mem pwtldi_m = { "PWT", 0, &src1_r, &bimmldoff };
static struct mem pwtldis_m = { "PWT", 0, &src1_r, &bimmldsoff };
static struct mem pwtldr_m = { "PWT", 0, &src1_r, 0, &src2_r };
#define PWTLDI atommem, &pwtldi_m
#define PWTLDIS atommem, &pwtldis_m
#define PWTLDR atommem, &pwtldr_m

static struct mem vpsti_m = { "VP", 0, &src1_r, &bimmstoff };
static struct mem vpstis_m = { "VP", 0, &src1_r, &bimmstsoff };
static struct mem vpstr_m = { "VP", 0, &dst_r, 0, &src1_r, 1 };
#define VPSTI atommem, &vpsti_m
#define VPSTIS atommem, &vpstis_m
#define VPSTR atommem, &vpstr_m

static struct mem mvsomemsti_m = { "MVSO", 0, &src1_r, &bimmstoff };
static struct mem mvsomemstis_m = { "MVSO", 0, &src1_r, &bimmstsoff };
static struct mem mvsomemstr_m = { "MVSO", 0, &dst_r, 0, &src1_r, 1 };
#define MVSOMEMSTI atommem, &mvsomemsti_m
#define MVSOMEMSTIS atommem, &mvsomemstis_m
#define MVSOMEMSTR atommem, &mvsomemstr_m

static struct mem mvsimemldi_m = { "MVSI", 0, &src1_r, &bimmldoff };
static struct mem mvsimemldis_m = { "MVSI", 0, &src1_r, &bimmldsoff };
static struct mem mvsimemldr_m = { "MVSI", 0, &src1_r, 0, &src2_r };
#define MVSIMEMLDI atommem, &mvsimemldi_m
#define MVSIMEMLDIS atommem, &mvsimemldis_m
#define MVSIMEMLDR atommem, &mvsimemldr_m

static struct insn tabignimmf[] = {
	{ 0x00000000, 0x08000000 },
	{ 0x08000000, 0x08000000 },
	{ 0, 0, OOPS },
};

static struct insn tabp[] = {
	{ 0x00f00000, 0x00f00000 },
	{ 0, 0, PRED },
};

static struct insn tabdst[] = {
	{ 0x00000000, 0x10000000, DST },
	{ 0x10000000, 0x14000000, SRDST },
	{ 0x14000000, 0x14000000, DST },
	{ 0, 0, OOPS },
};

static struct insn tabsrc1[] = {
	{ 0x00000000, 0x04000000, SRC1 },
	{ 0x04000000, 0x14000000, SRSRC },
	{ 0x14000000, 0x14000000, SRC1 },
	{ 0, 0, OOPS },
};

static struct insn tabsrc2[] = {
	{ 0x00000000, 0x08000000, SRC2 },
	{ 0x08000000, 0x1c000000, IMM6 },
	{ 0x18000000, 0x1c000000, IMM4 },
	{ 0x0c000000, 0x1c000000, IMM4 },
	{ 0x1c000000, 0x1c000000, IMM6 },
	{ 0, 0, OOPS },
};

static struct insn tablsrc[] = {
	{ 0x00000000, 0x08000000, SRC2 },
	{ 0x08000000, 0x18000000, IMM14 },
	{ 0x18000000, 0x18000000, IMM12 },
	{ 0, 0, OOPS },
};

static struct insn tabspdst[] = {
	{ 0x20000000, 0x20000000, PDST },
	{ 0x00000000, 0x20000000, PRED },
	{ 0, 0, OOPS },
};

static struct insn tabpdst[] = {
	{ 0x00000000, 0x000000e0, N("pand"), T(spdst) },
	{ 0x00000020, 0x000000e0, N("por"), T(spdst) },
	{ 0x00000040, 0x000000e0, T(spdst) },
	{ 0x00000060, 0x00000060 }, // don't modify predicate
	{ 0x00000080, 0x000000e0, N("pandn"), T(spdst) },
	{ 0x000000a0, 0x000000e0, N("porn"), T(spdst) },
	{ 0x000000c0, 0x000000e0, N("pnot"), T(spdst) },
	{ 0, 0, OOPS },
};

static struct insn tabpnot1[] = {
	{ 0, 8 },
	{ 8, 8, N("not") },
	{ 0, 0, OOPS },
};

static struct insn tabpnot2[] = {
	{ 0, 4 },
	{ 4, 4, N("not") },
	{ 0, 0, OOPS },
};

static struct insn tabsop[] = {
	/* control flow block */
	{ 0x00000000, 0x000000ff, N("bra"), T(ignimmf), BTARG },
	{ 0x00000002, 0x000000ff, N("call"), T(ignimmf), CTARG },
	{ 0x00000003, 0x000000ff, N("ret") },
	{ 0x00000004, 0x000000ff, N("sleep") },
	{ 0x00000005, 0x000000ff, N("wstc"), T(ignimmf), IMM4 },
	{ 0x00000006, 0x000000ff, N("wsts"), T(ignimmf), IMM4 },

	/* special functions block */
	{ 0x00000020, 0x000000ff, N("clicnt") },	/* reset $icnt to 0 */
	{ 0x00000021, 0x000000ff, U("o21") }, /* ADV */
	{ 0x00000022, 0x000000ff, U("o22") }, /* ADV */
	{ 0x00000023, 0x000000ff, U("o23") }, /* VP2 */
	{ 0x00000024, 0x000000ff, N("mbiread") },	/* read data for current macroblock into registers/memory */
	{ 0x00000025, 0x000000ff, U("o25") }, /* ANEW */
	{ 0x00000026, 0x000000ff, U("o26") }, /* ANEW */
	{ 0x00000028, 0x000000ff, N("mbinext") },	/* advance input to the next macroblock */
	{ 0x00000029, 0x000000ff, N("mvsread") },	/* read data for current macroblock/macroblock pair from input MVSURF */
	{ 0x0000002a, 0x000000ff, N("mvswrite") },	/* write data for current macroblock/macroblock pair to output MVSURF */
	{ 0x0000002b, 0x000000ff, U("o2b") }, /* ADV */
	{ 0x0000002c, 0x000000ff, U("o2c") }, /* ANEW */

	/* predicate block */
	{ 0x00000040, 0x000000e3, N("and"), T(spdst), T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ 0x00000041, 0x000000e3, N("or"), T(spdst), T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ 0x00000042, 0x000000e3, N("xor"), T(spdst), T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ 0x00000043, 0x000000e3, N("nop") },

	/* memory block */
	{ 0x00000080, 0x080000ff, N("st"), DMEMSTR, SRC2 },
	{ 0x08000080, 0x280000ff, N("st"), DMEMSTI, SRC2 },
	{ 0x28000080, 0x280000ff, N("st"), DMEMSTIS, SRC2 },
	{ 0x00000081, 0x080000ff, N("ld"), DST, DMEMLDR },
	{ 0x08000081, 0x280000ff, N("ld"), DST, DMEMLDI },
	{ 0x28000081, 0x280000ff, N("ld"), DST, DMEMLDIS },
	{ 0x00000083, 0x080000ff, N("ld"), DST, PWTLDR },
	{ 0x08000083, 0x280000ff, N("ld"), DST, PWTLDI },
	{ 0x28000083, 0x280000ff, N("ld"), DST, PWTLDIS },
	{ 0x00000084, 0x080000ff, N("st"), VPSTR, SRC2 },
	{ 0x08000084, 0x280000ff, N("st"), VPSTI, SRC2 },
	{ 0x28000084, 0x280000ff, N("st"), VPSTIS, SRC2 },
	{ 0x00000089, 0x080000ff, N("ld"), DST, MVSIMEMLDR },
	{ 0x08000089, 0x280000ff, N("ld"), DST, MVSIMEMLDI },
	{ 0x28000089, 0x280000ff, N("ld"), DST, MVSIMEMLDIS },
	{ 0x0000008a, 0x080000ff, N("st"), MVSOMEMSTR, SRC2 },
	{ 0x0800008a, 0x280000ff, N("st"), MVSOMEMSTI, SRC2 },
	{ 0x2800008a, 0x280000ff, N("st"), MVSOMEMSTIS, SRC2 },
	{ 0x0000008c, 0x080000ff, N("st6"), B6MEMSTR, SRC2 },
	{ 0x0800008c, 0x280000ff, N("st6"), B6MEMSTI, SRC2 },
	{ 0x2800008c, 0x280000ff, N("st6"), B6MEMSTIS, SRC2 },
	{ 0x0000008d, 0x080000ff, N("ld6"), DST, B6MEMLDR },
	{ 0x0800008d, 0x280000ff, N("ld6"), DST, B6MEMLDI },
	{ 0x2800008d, 0x280000ff, N("ld6"), DST, B6MEMLDIS },
	{ 0x0000008e, 0x080000ff, N("st7"), B7MEMSTR, SRC2 },
	{ 0x0800008e, 0x280000ff, N("st7"), B7MEMSTI, SRC2 },
	{ 0x2800008e, 0x280000ff, N("st7"), B7MEMSTIS, SRC2 },
	{ 0x0000008f, 0x080000ff, N("ld7"), DST, B7MEMLDR },
	{ 0x0800008f, 0x280000ff, N("ld7"), DST, B7MEMLDI },
	{ 0x2800008f, 0x280000ff, N("ld7"), DST, B7MEMLDIS },

	/* long arithmetic block */
	{ 0x000000a0, 0x000000ff, N("lmulu"), T(src1), T(src2) },
	{ 0x000000a1, 0x000000ff, N("lmuls"), T(src1), T(src2) },
	{ 0x000000a2, 0x000000ff, N("lsrr"), T(src2) },
	{ 0x000000a4, 0x000000ff, N("ladd"), T(src2), .fmask = F_VP3 },
	{ 0x000000a8, 0x000000ff, N("lsar"), T(src2), .fmask = F_VP3 },
	{ 0x000000ac, 0x000000ff, N("ldivu"), T(src2), .fmask = F_VP4 },

	{ 0, 0, OOPS },
};

static struct insn tabbop[] = {
	/* data transfer */
	{ 0x00000000, 0x0000001f, N("slct"), T(pdst), T(dst), PRED, T(src1), T(src2) },
	{ 0x00000001, 0x0000001f, N("mov"), T(pdst), T(dst), T(lsrc) },

	/* addition */
	{ 0x00000004, 0x0000001f, N("add"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x00000005, 0x0000001f, N("sub"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x00000006, 0x0000001f, N("subr"), T(pdst), T(dst), T(src1), T(src2), .fmask = F_VP2 },
	{ 0x00000006, 0x0000001f, N("avgs"), T(pdst), T(dst), T(src1), T(src2), .fmask = F_VP3 },
	{ 0x00000007, 0x0000001f, N("avgu"), T(pdst), T(dst), T(src1), T(src2), .fmask = F_VP3 },

	/* comparisons */
	{ 0x00000008, 0x0000001f, N("setgt"), T(pdst), T(src1), T(src2) },
	{ 0x00000009, 0x0000001f, N("setlt"), T(pdst), T(src1), T(src2) },
	{ 0x0000000a, 0x0000001f, N("seteq"), T(pdst), T(src1), T(src2) },    
	{ 0x0000000b, 0x0000001f, N("setlep"), T(pdst), T(src1), T(src2) },

	/* clips, misc */
	{ 0x0000000c, 0x0000001f, N("clamplep"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x0000000d, 0x0000001f, N("clamps"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x0000000e, 0x0000001f, N("sext"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x0000000f, 0x0000001f, N("setzero"), T(pdst), T(src1), T(src2), .fmask = F_VP2 },
	{ 0x0000000f, 0x0000001f, N("div2s"), T(pdst), T(dst), T(src1), .fmask = F_VP3 },

	/* bit manipulation */
	{ 0x00000010, 0x0000001f, N("bset"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x00000011, 0x0000001f, N("bclr"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x00000012, 0x0000001f, N("btest"), T(pdst), T(src1), T(src2) },

	/* shifts */
	{ 0x00000014, 0x0000001f, N("hswap"), T(pdst), T(dst), T(src1) },
	{ 0x00000015, 0x0000001f, N("shl"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x00000016, 0x0000001f, N("shr"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x00000017, 0x0000001f, N("sar"), T(pdst), T(dst), T(src1), T(src2) },

	/* bitwise ops */
	{ 0x00000018, 0x0000001f, N("and"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x00000019, 0x0000001f, N("or"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x0000001a, 0x0000001f, N("xor"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x0000001b, 0x0000001f, N("not"), T(pdst), T(dst), T(src1), T(src2) },

	/* misc */
	{ 0x0000001c, 0x0000001f, N("lut"), T(pdst), T(dst), T(src1), T(src2) },
	{ 0x0000001d, 0x0000001f, N("min"), T(pdst), T(dst), T(src1), T(src2), .fmask = F_VP3 },
	{ 0x0000001e, 0x0000001f, N("max"), T(pdst), T(dst), T(src1), T(src2), .fmask = F_VP3 },
	{ 0, 0, OOPS },
};

static struct insn tabopnp[] = {
	{ 0x00000000, 0x14000000, T(bop) },
	{ 0x04000000, 0x14000000, T(bop) },
	{ 0x10000000, 0x14000000, T(bop) },
	{ 0x14000000, 0x14000000, T(sop) },
	{ 0, 0, OOPS },
};

static struct insn tabop[] = {
	{ 0x00000000, 0x20000000, T(opnp) },
	{ 0x20000000, 0x20000000, T(p), T(opnp) },
	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0, 0, OP1B, T(op), .fmask = F_VP3 },
	{ 0xffc0000000ull, 0xffc0000000ull, OP1B, T(op), .fmask = F_VP2 },
	{ 0x0000000000ull, 0x0200000000ull, OP1B, RBRPRED, N("rbra"), RBRTARG, T(op), .fmask = F_VP2 },
	{ 0x0200000000ull, 0x0200000000ull, OP1B, N("not"), RBRPRED, N("rbra"), RBRTARG, T(op), .fmask = F_VP2 },
	{ 0, 0, OOPS },
};

static void vuc_prep(struct disisa *isa) {
	isa->vardata = vardata_new("vuc");
	int f_vp2op = vardata_add_feature(isa->vardata, "vp2op", "VP2 exclusive opcodes");
	int f_vp3op = vardata_add_feature(isa->vardata, "vp3op", "VP3+ opcodes");
	int f_vp4op = vardata_add_feature(isa->vardata, "vp4op", "VP4 opcodes");
	if (f_vp2op == -1 || f_vp3op == -1 || f_vp4op == -1)
		abort();
	int vs_vp = vardata_add_varset(isa->vardata, "vp", "VP version");
	if (vs_vp == -1)
		abort();
	int v_vp2 = vardata_add_variant(isa->vardata, "vp2", "VP2", vs_vp);
	int v_vp3 = vardata_add_variant(isa->vardata, "vp3", "VP3", vs_vp);
	int v_vp4 = vardata_add_variant(isa->vardata, "vp4", "VP4", vs_vp);
	if (v_vp2 == -1 || v_vp3 == -1 || v_vp4 == -1)
		abort();
	vardata_variant_feature(isa->vardata, v_vp2, f_vp2op);
	vardata_variant_feature(isa->vardata, v_vp3, f_vp3op);
	vardata_variant_feature(isa->vardata, v_vp4, f_vp3op);
	vardata_variant_feature(isa->vardata, v_vp4, f_vp4op);
	if (vardata_validate(isa->vardata))
		abort();
}

static uint32_t vuc_getcbsz(const struct disisa *isa, struct varinfo *varinfo) {
	if (mask_get(varinfo->fmask, 0))
		return 40;
	if (mask_get(varinfo->fmask, 1))
		return 30;
	return 0;
}

struct disisa vuc_isa_s = {
	tabroot,
	1,
	1,
	.prep = vuc_prep,
	.getcbsz = vuc_getcbsz,
};
