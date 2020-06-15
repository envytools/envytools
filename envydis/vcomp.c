/*
 * Copyright (C) 2013 Marcelina Kościelnicka <mwk@0x04.net>
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

static struct rbitfield ctargoff = { 8, 6, 17, 2 };
#define BTARG atombtarg, &ctargoff
#define CTARG atomctarg, &ctargoff
static struct rbitfield limmoff = { 8, 6, 17, 6 };
static struct rbitfield immoff = { 17, 6 };
#define LIMM atomimm, &limmoff
#define IMM atomimm, &immoff

static struct bitfield src1_bf = { 8, 3 };
static struct bitfield src2_bf = { 11, 3 };
static struct bitfield dst_bf = { 14, 3 };
static struct bitfield psrc1_bf = { 8, 2 };
static struct bitfield psrc2_bf = { 11, 2 };
static struct bitfield pdst_bf = { 14, 2 };

static struct reg src1_r = { &src1_bf, "r" };
static struct reg src2_r = { &src2_bf, "r" };
static struct reg dst_r = { &dst_bf, "r" };
static struct reg memidx_r = { 0, "memidx" };
static struct reg ififo_r = { 0, "ififo" };
#define SRC1 atomreg, &src1_r
#define SRC2 atomreg, &src2_r
#define DST atomreg, &dst_r
#define MEMIDX atomreg, &memidx_r
#define IFIFO atomreg, &ififo_r
static struct sreg pred_sr[] = {
	{ 3, "irq" },
	{ -1 },
};
static struct reg psrc1_r = { &psrc1_bf, "p", .specials = pred_sr };
static struct reg psrc2_r = { &psrc2_bf, "p", .specials = pred_sr };
static struct reg pdst_r = { &pdst_bf, "p", .specials = pred_sr };
static struct reg pred_r = { 0, "p0" };
#define PSRC1 atomreg, &psrc1_r
#define PSRC2 atomreg, &psrc2_r
#define PDST atomreg, &pdst_r
#define PRED atomreg, &pred_r

static struct rbitfield memoff = { 17, 6 };
static struct mem mem0_m = { "M0", 0, &memidx_r };
static struct mem mem1_m = { "M1", 0, &memidx_r };
static struct mem mem2_m = { "M2", 0, &memidx_r };
static struct mem mem3_m = { "M3", 0, &memidx_r };
static struct mem datar_m = { "D", 0, &src2_r };
static struct mem datai_m = { "D", 0, 0, &memoff };
static struct mem outr_m = { "O", 0, &src2_r };
static struct mem outi_m = { "O", 0, 0, &memoff };
#define MEM0 atommem, &mem0_m
#define MEM1 atommem, &mem1_m
#define MEM2 atommem, &mem2_m
#define MEM3 atommem, &mem3_m
#define DATAR atommem, &datar_m
#define DATAI atommem, &datai_m
#define OUTR atommem, &outr_m
#define OUTI atommem, &outi_m

static struct insn tabpdst[] = {
	{ 0x000000, 0x010000, PDST },
	{ 0x010000, 0x010000, N("not"), PDST },
	{ 0, 0, OOPS },
};

static struct insn tabpsrc1[] = {
	{ 0x000000, 0x000400, PSRC1 },
	{ 0x000400, 0x000400, N("not"), PSRC1 },
	{ 0, 0, OOPS },
};

static struct insn tabpsrc2[] = {
	{ 0x000000, 0x002000, PSRC2 },
	{ 0x002000, 0x002000, N("not"), PSRC2 },
	{ 0, 0, OOPS },
};

static struct insn tabsrc2[] = {
	{ 0x000000, 0x000040, SRC2 },
	{ 0x000040, 0x000040, IMM },
	{ 0, 0, OOPS },
};

static struct insn tablsrc2[] = {
	{ 0x000000, 0x000040, SRC2 },
	{ 0x000040, 0x000040, LIMM },
	{ 0, 0, OOPS },
};

static int ignimm[] = { 6, 1 };
#define IGNIMM atomign, ignimm

static struct insn tabpred[] = {
	{ 0x000000, 0x000080 },
	{ 0x000080, 0x000080, PRED },
	{ 0, 0, OOPS },
};

static struct insn tabldmem[] = {
	{ 0x000000, 0x000700, IGNIMM, MEM0 },
	{ 0x000100, 0x000700, IGNIMM, MEM1 },
	{ 0x000200, 0x000700, IGNIMM, MEM2 },
	{ 0x000300, 0x000700, IGNIMM, MEM3 },
	{ 0x000400, 0x000740, DATAR },
	{ 0x000440, 0x000740, DATAI },
	{ 0x000600, 0x000700, IGNIMM, MEMIDX },
	{ 0x000700, 0x000700, IGNIMM, IFIFO },
	{ 0, 0, OOPS },
};

static struct insn tabstmem[] = {
	{ 0x000000, 0x01c000, IGNIMM, MEM0 },
	{ 0x004000, 0x01c000, IGNIMM, MEM1 },
	{ 0x008000, 0x01c000, IGNIMM, MEM2 },
	{ 0x00c000, 0x01c000, IGNIMM, MEM3 },
	{ 0x010000, 0x01c040, DATAR },
	{ 0x010040, 0x01c040, DATAI },
	{ 0x014000, 0x01c040, OUTR },
	{ 0x014040, 0x01c040, OUTI },
	{ 0x018000, 0x01c000, IGNIMM, MEMIDX },
	{ 0, 0, OOPS },
};

static struct insn tabm[] = {
	{ 0x000000, 0x00003f, T(pred), N("add"), DST, SRC1, T(src2) },
	{ 0x000001, 0x00003f, T(pred), N("sub"), DST, SRC1, T(src2) },
	{ 0x000002, 0x00003f, T(pred), N("sar"), DST, SRC1, T(src2) }, /* only low 6 bits of SRC2 used */
	{ 0x000003, 0x00003f, T(pred), N("shl"), DST, SRC1, T(src2) }, /* only low 6 bits of SRC2 used */
	{ 0x000004, 0x00003f, T(pred), N("and"), DST, SRC1, T(src2) },
	{ 0x000005, 0x00003f, T(pred), N("or"), DST, SRC1, T(src2) },
	{ 0x000006, 0x00003f, T(pred), N("xor"), DST, SRC1, T(src2) },
	{ 0x000007, 0x00003f, T(pred), N("not"), DST, SRC1 },
	{ 0x000008, 0x00003f, T(pred), N("mov"), DST, T(lsrc2) },
	{ 0x000009, 0x00003f, T(pred), N("hswap"), DST, SRC1 }, /* swap bits 0-15 with 16-31 ... */
	{ 0x00000a, 0x00003f, T(pred), N("min"), DST, SRC1, T(src2) },
	{ 0x00000b, 0x00003f, T(pred), N("max"), DST, SRC1, T(src2) },
	{ 0x00000c, 0x00003f, T(pred), N("setgt"), T(pdst), SRC1, T(src2) },
	{ 0x00000d, 0x00003f, T(pred), N("setlt"), T(pdst), SRC1, T(src2) },
	{ 0x00000e, 0x00003f, T(pred), N("seteq"), T(pdst), SRC1, T(src2) },
	{ 0x00000f, 0x00003f, T(pred), N("btest"), T(pdst), SRC1, T(src2) },
	{ 0x000010, 0x00003f, T(pred), N("and"), T(pdst), T(psrc1), T(psrc2) },
	{ 0x000011, 0x00003f, T(pred), N("or"), T(pdst), T(psrc1), T(psrc2) },
	{ 0x000012, 0x00003f, T(pred), N("xor"), T(pdst), T(psrc1), T(psrc2) },
	{ 0x000013, 0x00003f, T(pred), N("mov"), T(pdst), T(psrc1) },
	{ 0x000014, 0x00003f, T(pred), N("set"), T(pdst) }, /* set to 1... */
	{ 0x000015, 0x00003f, T(pred), N("exp2"), DST, T(src2) }, /* DST = 1 << (SRC2 & 0x3f) */
	{ 0x000016, 0x00003f, T(pred), N("exp2m1"), DST, T(src2) }, /* DST = 1 << ((SRC2 & 0x3f)) - 1 */
	{ 0x000020, 0x00003f, N("ldu"), DST, T(ldmem) },
	{ 0x000021, 0x00003f, N("lds"), DST, T(ldmem) },
	{ 0x000022, 0x00003f, N("st"), T(stmem), SRC1 },
	{ 0x000024, 0x00003f, N("div"), DST, SRC1, T(src2) }, /* s48 by u16 -> s48, -1 on div-by-0 */
	{ 0x000025, 0x00003f, IGNIMM, N("mulsrr"), DST, SRC1, SRC2, IMM }, /* s48 by s16, then shift + round up */
	{ 0x000026, 0x00003f, IGNIMM, N("sleep") },
	{ 0x000028, 0x00003f, IGNIMM, N("bra"), BTARG },
	{ 0x00002a, 0x00003f, IGNIMM, SESTART, N("not"), PDST, SEEND, N("bra"), BTARG },
	{ 0x00002b, 0x00003f, IGNIMM, PDST, N("bra"), BTARG },
	{ 0x00002c, 0x00003f, IGNIMM, N("bra"), BTARG }, /* XXX wtf? */
	{ 0x00002e, 0x00003f, IGNIMM, N("call"), CTARG },
	{ 0x00002f, 0x00003f, IGNIMM, N("ret") },
	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0, 0, OP1B, T(m) },
};

struct disisa vcomp_isa_s = {
	tabroot,
	1,
	1,
	4,
};
