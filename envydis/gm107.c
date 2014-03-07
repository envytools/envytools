/*
 * Copyright (C) 2009-2011 Marcin Kościelnicki <koriakin@0x04.net>
 * Copyright (C) 2012 Christoph Bumiller <e0425955@student.tuwien.ac.at>
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

static struct rbitfield btargoff = { { 20, 24 }, RBF_SIGNED, .pcrel = 1, .addend = 8 };
#define BTARG atomctarg, &btargoff
#define CTARG atomctarg, &btargoff

static struct bitfield sched0_bf = {  0, 21 };
static struct bitfield sched1_bf = { 21, 21 };
static struct bitfield sched2_bf = { 42, 21 };

#define SCHED0 atomrimm, &sched0_bf
#define SCHED1 atomrimm, &sched1_bf
#define SCHED2 atomrimm, &sched2_bf

static struct rbitfield imm19off = { { 20, 19 }, RBF_SIGNED };
static struct rbitfield imm32off = { { 20, 32 }, RBF_SIGNED };
static struct rbitfield fimmoff = { { 20, 20 }, RBF_SIGNED, .shr = 12 };
static struct rbitfield dimmoff = { { 20, 20 }, RBF_SIGNED, .shr = 44 };

#define IMM19 atomrimm, &imm19off
#define IMM32 atomrimm, &imm32off
#define FIMM atomrimm, &fimmoff
#define DIMM atomrimm, &dimmoff

static struct sreg reg_sr[] = {
	{ 255, 0, SR_ZERO },
	{ -1 },
};

static struct bitfield dst_bf = { 0, 8 };
static struct bitfield src0_bf = { 8, 8 };
static struct bitfield src1_bf = { 20, 8 };
static struct bitfield src2_bf = { 39, 8 };

static struct reg dst_r = { &dst_bf, "r", .specials = reg_sr };
static struct reg src0_r = { &src0_bf, "r", .specials = reg_sr };
static struct reg src1_r = { &src1_bf, "r", .specials = reg_sr };
static struct reg src2_r = { &src2_bf, "r", .specials = reg_sr };

#define DST atomreg, &dst_r
#define SRC0 atomreg, &src0_r
#define SRC1 atomreg, &src1_r
#define SRC2 atomreg, &src2_r

static struct rbitfield cmem_imm = { 20, 16, RBF_UNSIGNED }; /*XXX: check width */
static struct rbitfield cmem0_imm = { 20, 16, RBF_UNSIGNED, .shr = 2 }; /*XXX: check width */
static struct bitfield cmem_idx = { 36, 5 }; /*XXX: check width */

static struct mem cmem_m = { "c", &cmem_idx, &src0_r, &cmem_imm };
static struct mem cmem0_m = { "c", 0, 0, &cmem0_imm };
static struct mem gmem_m = { "g", 0, &src0_r };
static struct mem lmem_m = { "l", 0, &src0_r };
static struct mem smem_m = { "s", 0, &src0_r };

#define CMEM atommem, &cmem_m
#define CMEM0 atommem, &cmem0_m
#define GMEM atommem, &gmem_m
#define LMEM atommem, &lmem_m
#define SMEM atommem, &smem_m

F1( rs37, 37, N( "rs"))

F1(ftz44, 44, N("ftz"))

F1( cc47, 47, N( "cc"))

F1(sat50, 50, N("sat"))

F1(  x43, 43, N(  "x"))
F1(  x48, 48, N(  "x"))

F1(neg45, 45, N("neg"))
F1(neg48, 48, N("neg"))
F1(neg49, 49, N("neg"))

F1(abs49, 49, N("abs"))

F1( h135, 35, N( "h1"))
F1( h153, 53, N( "h1"))

F1(mrg37, 37, N("mrg"))

F1(s1648, 48, N("s16"))
F1(s1649, 49, N("s16"))

static struct insn tabpred[] = {
	{ 0x0000000000000000ull, 0x0000000000070000ull, N("p0") },
	{ 0x0000000000010000ull, 0x0000000000070000ull, N("p1") },
	{ 0x0000000000020000ull, 0x0000000000070000ull, N("p2") },
	{ 0x0000000000030000ull, 0x0000000000070000ull, N("p3") },
	{ 0x0000000000040000ull, 0x0000000000070000ull, N("p4") },
	{ 0x0000000000050000ull, 0x0000000000070000ull, N("p5") },
	{ 0x0000000000060000ull, 0x0000000000070000ull, N("p6") },
	{ 0x0000000000070000ull, 0x0000000000070000ull },
	{ 0, 0, OOPS },
};

static struct insn tabldst[] = {
	{ 0x0000000000000000ull, 0x0007000000000000ull, N("u8") },
	{ 0x0001000000000000ull, 0x0007000000000000ull, N("s8") },
	{ 0x0002000000000000ull, 0x0007000000000000ull, N("u16") },
	{ 0x0003000000000000ull, 0x0007000000000000ull, N("s16") },
	{ 0x0004000000000000ull, 0x0007000000000000ull, N("b32") },
	{ 0x0005000000000000ull, 0x0007000000000000ull, N("b64") },
	{ 0, 0, OOPS },
};

static struct insn tabge[] = {
	{ 0x0000000000000000ull, 0x0000200000000000ull },
	{ 0x0000200000000000ull, 0x0000200000000000ull, N("e") },
	{ 0, 0, OOPS },
};

static struct insn tabstgcop[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, N("cg") },
	{ 0x0000800000000000ull, 0x0000c00000000000ull, N("cs") },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, N("wt") },
	{ 0, 0, OOPS },
};

static struct insn tabldgcop[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, N("cg") },
	{ 0x0000800000000000ull, 0x0000c00000000000ull, N("ci") },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, N("cv") },
	{ 0, 0, OOPS },
};

static struct insn tabstlcop[] = {
	{ 0x0000000000000000ull, 0x0000300000000000ull },
	{ 0x0000100000000000ull, 0x0000300000000000ull, N("cg") },
	{ 0x0000200000000000ull, 0x0000300000000000ull, N("cs") },
	{ 0x0000300000000000ull, 0x0000300000000000ull, N("wt") },
	{ 0, 0, OOPS },
};

static struct insn tabldlcop[] = {
	{ 0x0000000000000000ull, 0x0000300000000000ull },
	{ 0x0000100000000000ull, 0x0000300000000000ull, N("lu") },
	{ 0x0000200000000000ull, 0x0000300000000000ull, N("ci") },
	{ 0x0000300000000000ull, 0x0000300000000000ull, N("cv") },
	{ 0, 0, OOPS },
};

static struct insn tabrnd[] = {
	{ 0x0000000000000000ull, 0x0000018000000000ull },
	{ 0x0000008000000000ull, 0x0000018000000000ull, N("rm") },
	{ 0x0000010000000000ull, 0x0000018000000000ull, N("rp") },
	{ 0x0000018000000000ull, 0x0000018000000000ull, N("rz") },
	{ 0, 0, OOPS },
};

static struct insn tabi2id[] = {
	{ 0x0000000000000000, 0x0000000000001300, N("u8") },
	{ 0x0000000000000100, 0x0000000000001300, N("u16") },
	{ 0x0000000000000200, 0x0000000000001300, N("u32") },
	{ 0x0000000000000300, 0x0000000000001300, N("u64") },
	{ 0x0000000000001000, 0x0000000000001300, N("s8") },
	{ 0x0000000000001100, 0x0000000000001300, N("s16") },
	{ 0x0000000000001200, 0x0000000000001300, N("s32") },
	{ 0x0000000000001300, 0x0000000000001300, N("s64") },
	{ 0, 0, OOPS },
};

static struct insn tabi2is[] = {
	{ 0x0000000000000000, 0x0000000000002c00, N("u8") },
	{ 0x0000000000000400, 0x0000000000002c00, N("u16") },
	{ 0x0000000000000800, 0x0000000000002c00, N("u32") },
	{ 0x0000000000000c00, 0x0000000000002c00, N("u64") },
	{ 0x0000000000002000, 0x0000000000002c00, N("s8") },
	{ 0x0000000000002400, 0x0000000000002c00, N("s16") },
	{ 0x0000000000002800, 0x0000000000002c00, N("s32") },
	{ 0x0000000000002c00, 0x0000000000002c00, N("s64") },
	{ 0, 0, OOPS },
};

static struct insn tabf2isr[] = {
	{ 0x0000000000000000, 0x0000018000000000 },
	{ 0x0000008000000000, 0x0000018000000000, N("floor") },
	{ 0x0000010000000000, 0x0000018000000000, N("ceil") },
	{ 0x0000018000000000, 0x0000018000000000, N("trunc") },
	{ 0, 0, OOPS },
};

static struct insn tabi2fd[] = {
	{ 0x0000000000000200, 0x0000000000000300, N("f32") },
	{ 0, 0, OOPS },
};

static struct insn tabf2is[] = {
	{ 0x0000000000000800, 0x0000000000000c00, N("f32") },
	{ 0, 0, OOPS },
};

static struct insn tabxmad1[] = {
	{ 0x0000000000000000ull, 0x000c000000000000ull },
	{ 0x0004000000000000ull, 0x000c000000000000ull, N("clo") },
	{ 0x0008000000000000ull, 0x000c000000000000ull, N("chi") },
	{ 0x000c000000000000ull, 0x000c000000000000ull, N("csfu") },
	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0xeed8000000000000ull, 0xfff8000000000000ull, OP8B, N("st"), T(pred), T(ldst), T(ge), T(stgcop), GMEM, DST },
	{ 0xeed0000000000000ull, 0xfff8000000000000ull, OP8B, N("ld"), T(pred), T(ldst), T(ge), T(ldgcop), DST, GMEM },
	{ 0xef90000000000000ull, 0xfff8000000000000ull, OP8B, N("ld"), T(pred), T(ldst), DST, CMEM },
	{ 0xef58000000000000ull, 0xfff8000000000000ull, OP8B, N("st"), T(pred), T(ldst), SMEM, DST },
	{ 0xef50000000000000ull, 0xfff8000000000000ull, OP8B, N("st"), T(pred), T(ldst), T(stlcop), LMEM, DST },
	{ 0xef48000000000000ull, 0xfff8000000000000ull, OP8B, N("ld"), T(pred), T(ldst), DST, SMEM },
	{ 0xef40000000000000ull, 0xfff8000000000000ull, OP8B, N("ld"), T(pred), T(ldst), T(ldlcop), DST, LMEM },

	{ 0xe24000000000000full, 0xfff800000000000full, OP8B, N("bra"), T(pred), BTARG },
	{ 0xe260000000000040ull, 0xfff8000000000040ull, OP8B, N("cal"), CTARG },
	{ 0xe30000000000000full, 0xfff800000000000full, OP8B, N("exit"), T(pred) },

	{ 0x5cb8000000000000ull, 0xfff8000000000000ull, OP8B, N("cvt"), T(pred), T(i2fd), DST, T(abs49), T(rnd), T(i2is), SRC1 },
	{ 0x5cb0000000000000ull, 0xfff8000000000000ull, OP8B, N("cvt"), T(pred), T(ftz44), T(i2id), DST, T(f2isr), T(f2is), SRC1 },
	{ 0x5ce0000000000000ull, 0xfff8000000000000ull, OP8B, N("cvt"), T(pred), T(sat50), T(i2id), DST, T(neg45), T(abs49), T(i2is), SRC1 },

	{ 0x5cc0000000000000ull, 0xfff8000000000000ull, OP8B, N("add"), T(pred), T(rs37), T(x48), DST, SRC0, SRC1, SRC2 },
	{ 0x5c98078000000000ull, 0xfff8078000000000ull, OP8B, N("mov"), T(pred), DST, SRC1 },
	{ 0x5c80000000000000ull, 0xfff8000000000000ull, OP8B, N("mul"), T(pred), T(rnd), N("f64"), DST, SRC0, SRC1 },
	{ 0x5c70000000000000ull, 0xfff8000000000000ull, OP8B, N("add"), T(pred), T(rnd), N("f64"), DST, SRC0, T(neg45), SRC1 },
	{ 0x5c68000000000000ull, 0xfff8000000000000ull, OP8B, N("mul"), T(pred), T(ftz44), T(rnd), T(sat50), N("f32"), DST, SRC0, SRC1 },
	{ 0x5c58000000000000ull, 0xfff8000000000000ull, OP8B, N("add"), T(pred), T(ftz44), T(rnd), T(sat50), N("f32"), DST, SRC0, T(neg45), SRC1 },
	{ 0x5c10000000000000ull, 0xfff8000000000000ull, OP8B, N("add"), T(pred), T(x43), T(sat50), T(cc47), DST, T(neg49), SRC0, T(neg48), SRC1 },

	//  0010001000000000 PSL/CBCC */
	{ 0x5b00000000000000ull, 0xff00000000000000ull, OP8B, N("xmad"), T(pred), T(mrg37), T(xmad1), T(cc47), DST, T(s1648), T(h153), SRC0, T(s1649), T(h135), SRC1, SRC2 },

	{ 0x4c98078000000000ull, 0xfff8078000000000ull, OP8B, N("mov"), T(pred), DST, CMEM0 },
	{ 0x4c10000000000000ull, 0xfff8000000000000ull, OP8B, N("add"), T(pred), DST, SRC0, CMEM0 },

	{ 0x3880000000000000ull, 0xfff8000000000000ull, OP8B, N("mul"), T(pred), N("f64"), DST, SRC0, DIMM },
	{ 0x3868000000000000ull, 0xfff8000000000000ull, OP8B, N("mul"), T(pred), T(ftz44), N("f32"), DST, SRC0, FIMM },
	{ 0x3810000000000000ull, 0xfef8000000000000ull, OP8B, N("add"), T(pred), T(cc47), DST, T(neg49), SRC0, T(neg48), IMM19 },

	{ 0x1c00000000000000ull, 0xfff0000000000000ull, OP8B, N("add"), T(pred), DST, SRC0, IMM32 },
	{ 0x010000000000f000ull, 0xfff000000000f000ull, OP8B, N("mov"), T(pred), DST, IMM32 },

	{ 0x0000000000000000ull, 0x0000000000000000ull, OP8B, N("unknown") },
	{ 0, 0, OOPS },
};

static void gm107_prep(struct disisa *isa) {
	// no variants yet
}

struct disisa gm107_isa_s = {
	tabroot,
	8,
	4,
	1,
	.i_need_nv50as_hack = 1,
	.prep = gm107_prep,
};
