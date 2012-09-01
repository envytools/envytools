/*
 *
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "dis-intern.h"

/*
 * $r255: bit bucket on write, 0 on read
 */

/*
 * Code target field
 */
static struct rbitfield ctargoff = { { 23, 24 }, RBF_SIGNED, .pcrel = 1, .addend = 8 };
#define BTARG atomctarg, &ctargoff
#define CTARG atomctarg, &ctargoff


/*
 * Misc number fields
 */
static struct rbitfield uimmoff = { { 0x17, 19 }, RBF_UNSIGNED };
static struct rbitfield suimmoff = { { 0x17, 18 }, RBF_UNSIGNED }; // XXX: for r/lshf, check size
static struct rbitfield iimmoff = { { 0x17, 19 }, RBF_SIGNED };
static struct rbitfield fimmoff = { { 0x17, 19 }, .shr = 12 };
static struct rbitfield limmoff = { { 0x17, 32 }, .wrapok = 1 };
static struct rbitfield dimmoff = { { 0x17, 19 }, .shr = 44 };
static struct rbitfield schedvals = { { 0x2, 56 }, .wrapok = 1 };
#define UIMM atomrimm, &uimmoff
#define SUIMM atomrimm, &suimmoff
#define IIMM atomrimm, &iimmoff
#define FIMM atomrimm, &fimmoff
#define DIMM atomrimm, &dimmoff
#define LIMM atomrimm, &limmoff
#define SCHED atomrimm, &schedvals

/*
 * Register fields
 */
static struct sreg reg_sr[] = {
	{ 255, 0, SR_ZERO },
	{ -1 },
};
static struct sreg pred_sr[] = {
	{ 7, 0, SR_ONE },
	{ -1 },
};

static struct bitfield dst_bf = { 0x2, 8 };
static struct bitfield pdst_bf = { 0x2, 3 };
static struct bitfield src1_bf = { 0xa, 8 };
static struct bitfield src2_bf = { 0x17, 8 };
static struct bitfield src3_bf = { 0x2a, 8 };
static struct bitfield pred_bf = { 0x12, 3 };
static struct bitfield psrc1_bf = { 0x2a, 3 };

static struct reg dst_r = { &dst_bf, "r", .specials = reg_sr };
static struct reg dstd_r = { &dst_bf, "r", "d", .specials = reg_sr };
static struct reg dstq_r = { &dst_bf, "r", "q" };
static struct reg src1_r = { &src1_bf, "r", .specials = reg_sr };
static struct reg src1d_r = { &src1_bf, "r", "d", .specials = reg_sr };
static struct reg src2_r = { &src2_bf, "r", .specials = reg_sr };
static struct reg src2d_r = { &src2_bf, "r", "d", .specials = reg_sr };
static struct reg src3_r = { &src3_bf, "r", .specials = reg_sr };
static struct reg src3d_r = { &src3_bf, "r", "d", .specials = reg_sr };
static struct reg psrc1_r = { &psrc1_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdst_r = { &pdst_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pred_r = { &pred_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg cc_r = { 0, "c", .cool = 1 };

#define DST atomreg, &dst_r
#define DSTD atomreg, &dstd_r
#define DSTQ atomreg, &dstq_r
#define PRED atomreg, &pred_r
#define SRC1 atomreg, &src1_r
#define SRC1D atomreg, &src1d_r
#define SRC2 atomreg, &src2_r
#define SRC2D atomreg, &src2d_r
#define SRC3 atomreg, &src3_r
#define SRC3D atomreg, &src3d_r
#define PSRC1 atomreg, &psrc1_r
#define CC atomreg, &cc_r

/*
 * Memory fields
 */

static struct rbitfield gmem_imm = { { 0x17, 32 }, RBF_SIGNED };
static struct rbitfield cmem_imm = { { 0x17, 14 }, RBF_SIGNED, .shr = 2 };
static struct bitfield cmem_idx = { 0x25, 5 };

static struct mem gmem_m = { "g", 0, &src1_r, &gmem_imm };
static struct mem gdmem_m = { "g", 0, &src1d_r, &gmem_imm };
static struct mem cmem_m = { "c", &cmem_idx, 0, &cmem_imm };

#define GLOBAL atommem, &gmem_m
#define GLOBALD atommem, &gdmem_m
#define CONST atommem, &cmem_m


/*
 * The instructions
 */

F(gmem, 0x37, GLOBAL, GLOBALD)

static struct insn tabfrmi[] = {
	{ 0x0000000000000000, 0x00000c0000000000, N("rni") },
	{ 0x0000040000000000, 0x00000c0000000000, N("rmi") },
	{ 0x0000080000000000, 0x00000c0000000000, N("rpi") },
	{ 0x00000c0000000000, 0x00000c0000000000, N("rzi") },
	{ 0, 0, OOPS },
};

static struct insn tabfrm2a[] = {
	{ 0x0000000000000000, 0x00000c0000000000, N("rn") },
	{ 0x0000040000000000, 0x00000c0000000000, N("rm") },
	{ 0x0000080000000000, 0x00000c0000000000, N("rp") },
	{ 0x00000c0000000000, 0x00000c0000000000, N("rz") },
	{ 0, 0, OOPS },
};
static struct insn tabfrm35[] = {
	{ 0x0000000000000000, 0x0060000000000000, N("rn") },
	{ 0x0020000000000000, 0x0060000000000000, N("rm") },
	{ 0x0040000000000000, 0x0060000000000000, N("rp") },
	{ 0x0060000000000000, 0x0060000000000000, N("rz") },
	{ 0, 0, OOPS },
};
static struct insn tabfrm36[] = {
	{ 0x0000000000000000, 0x00c0000000000000, N("rn") },
	{ 0x0040000000000000, 0x00c0000000000000, N("rm") },
	{ 0x0080000000000000, 0x00c0000000000000, N("rp") },
	{ 0x00c0000000000000, 0x00c0000000000000, N("rz") },
	{ 0, 0, OOPS },
};

static struct insn tablane2a[] = {
	{ 0x0000000000000000ull, 0x00003c0000000000ull, N("lnone") },
	{ 0x0000040000000000ull, 0x00003c0000000000ull, N("l0") },
	{ 0x0000080000000000ull, 0x00003c0000000000ull, N("l1") },
	{ 0x00000c0000000000ull, 0x00003c0000000000ull, N("l01") },
	{ 0x0000100000000000ull, 0x00003c0000000000ull, N("l2") },
	{ 0x0000140000000000ull, 0x00003c0000000000ull, N("l02") },
	{ 0x0000180000000000ull, 0x00003c0000000000ull, N("l12") },
	{ 0x00001c0000000000ull, 0x00003c0000000000ull, N("l012") },
	{ 0x0000200000000000ull, 0x00003c0000000000ull, N("l3") },
	{ 0x0000240000000000ull, 0x00003c0000000000ull, N("l03") },
	{ 0x0000280000000000ull, 0x00003c0000000000ull, N("l13") },
	{ 0x00002c0000000000ull, 0x00003c0000000000ull, N("l013") },
	{ 0x0000300000000000ull, 0x00003c0000000000ull, N("l23") },
	{ 0x0000340000000000ull, 0x00003c0000000000ull, N("l023") },
	{ 0x0000380000000000ull, 0x00003c0000000000ull, N("l123") },
	{ 0x00003c0000000000ull, 0x00003c0000000000ull },
	{ 0, 0, OOPS },
};

static struct insn tabcc[] = {
	{ 0x0000000000000000ull, 0x000000000000007cull, N("never"), CC },
	{ 0x0000000000000004ull, 0x000000000000007cull, N("l"), CC },
	{ 0x0000000000000008ull, 0x000000000000007cull, N("e"), CC },
	{ 0x000000000000000cull, 0x000000000000007cull, N("le"), CC },
	{ 0x0000000000000010ull, 0x000000000000007cull, N("g"), CC },
	{ 0x0000000000000014ull, 0x000000000000007cull, N("lg"), CC },
	{ 0x0000000000000018ull, 0x000000000000007cull, N("ge"), CC },
	{ 0x000000000000001cull, 0x000000000000007cull, N("lge"), CC },
	{ 0x0000000000000020ull, 0x000000000000007cull, N("u"), CC },
	{ 0x0000000000000024ull, 0x000000000000007cull, N("lu"), CC },
	{ 0x0000000000000028ull, 0x000000000000007cull, N("eu"), CC },
	{ 0x000000000000002cull, 0x000000000000007cull, N("leu"), CC },
	{ 0x0000000000000030ull, 0x000000000000007cull, N("gu"), CC },
	{ 0x0000000000000034ull, 0x000000000000007cull, N("lgu"), CC },
	{ 0x0000000000000038ull, 0x000000000000007cull, N("geu"), CC },
	{ 0x000000000000003cull, 0x000000000000007cull, },
	{ 0x0000000000000040ull, 0x000000000000007cull, N("no"), CC },
	{ 0x0000000000000044ull, 0x000000000000007cull, N("nc"), CC },
	{ 0x0000000000000048ull, 0x000000000000007cull, N("ns"), CC },
	{ 0x000000000000004cull, 0x000000000000007cull, N("na"), CC },
	{ 0x0000000000000050ull, 0x000000000000007cull, N("a"), CC },
	{ 0x0000000000000054ull, 0x000000000000007cull, N("s"), CC },
	{ 0x0000000000000058ull, 0x000000000000007cull, N("c"), CC },
	{ 0x000000000000005cull, 0x000000000000007cull, N("o"), CC },
	{ 0, 0, OOPS },
};

static struct insn tabis2[] = {
	{ 0x4000000000000000ull, 0xc000000000000000ull, CONST },
	{ 0xc000000000000000ull, 0xc000000000000000ull, SRC2 },
	{ 0, 0, OOPS },
};

static struct insn tabis2w3[] = {
	{ 0x4000000000000000ull, 0xc000000000000000ull, CONST },
	{ 0x8000000000000000ull, 0xc000000000000000ull, SRC3 },
	{ 0xc000000000000000ull, 0xc000000000000000ull, SRC2 },
	{ 0, 0, OOPS },
};

static struct insn tabis3[] = {
	{ 0x4000000000000000ull, 0xc000000000000000ull, SRC3 },
	{ 0x8000000000000000ull, 0xc000000000000000ull, CONST },
	{ 0xc000000000000000ull, 0xc000000000000000ull, SRC3 },
	{ 0, 0, OOPS },
};

static struct insn tabds2[] = {
	{ 0x4000000000000000ull, 0xc000000000000000ull, CONST },
	{ 0xc000000000000000ull, 0xc000000000000000ull, SRC2D },
	{ 0, 0, OOPS },
};
static struct insn tabds2w3[] = {
	{ 0x4000000000000000ull, 0xc000000000000000ull, CONST },
	{ 0x8000000000000000ull, 0xc000000000000000ull, SRC3D },
	{ 0xc000000000000000ull, 0xc000000000000000ull, SRC2D },
	{ 0, 0, OOPS },
};
static struct insn tabds3[] = {
	{ 0x4000000000000000ull, 0xc000000000000000ull, SRC3D },
	{ 0x8000000000000000ull, 0xc000000000000000ull, CONST },
	{ 0xc000000000000000ull, 0xc000000000000000ull, SRC3D },
	{ 0, 0, OOPS },
};

static struct insn tabii2[] = {
	{ 0xc000000000000000ull, 0xc800000000000000ull, UIMM },
	{ 0xc800000000000000ull, 0xc800000000000000ull, IIMM }, // XXX: check me
	{ 0, 0, OOPS },
};
static struct insn tabsui2a[] = {
	{ 0x8000000000000000ull, 0xc000000000000000ull, SUIMM },
	{ 0, 0, OOPS },
};
static struct insn tabsui2b[] = {
	{ 0xc000000000000000ull, 0xc000000000000000ull, SUIMM },
	{ 0, 0, OOPS },
};
static struct insn tabfi2[] = {
	{ 0xc000000000000000ull, 0xc000000000000000ull, FIMM },
	{ 0, 0, OOPS },
};
static struct insn tabdi2[] = {
	{ 0xc000000000000000ull, 0xc000000000000000ull, DIMM },
	{ 0, 0, OOPS },
};

static struct insn tabcvtf2idst[] = {
	{ 0x0000000000000400ull, 0x0000000000004c00ull, N("u16"), DST },
	{ 0x0000000000004400ull, 0x0000000000004c00ull, N("s16"), DST },
	{ 0x0000000000000800ull, 0x0000000000004c00ull, N("u32"), DST },
	{ 0x0000000000004800ull, 0x0000000000004c00ull, N("s32"), DST },
	{ 0x0000000000000c00ull, 0x0000000000004c00ull, N("u64"), DSTD },
	{ 0x0000000000004c00ull, 0x0000000000004c00ull, N("s64"), DSTD },
	{ 0, 0, OOPS },
};
static struct insn tabcvtf2isrc[] = {
	{ 0x0000000000001000ull, 0x0000000000003000ull, N("f16"), T(is2) },
	{ 0x0000000000002000ull, 0x0000000000003000ull, N("f32"), T(is2) },
	{ 0x0000000000003000ull, 0x0000000000003000ull, N("f64"), T(ds2) },
	{ 0, 0, OOPS },
};

static struct insn tabcvti2fdst[] = {
	{ 0x0000000000000400ull, 0x0000000000000c00ull, N("f16"), DST },
	{ 0x0000000000000800ull, 0x0000000000000c00ull, N("f32"), DST },
	{ 0x0000000000000c00ull, 0x0000000000000c00ull, N("f64"), DSTD },
	{ 0, 0, OOPS },
};
static struct insn tabcvti2fsrc[] = {
	{ 0x0000000000000000ull, 0x000000000000b000ull, N("u8"), T(is2) },
	{ 0x0000000000008000ull, 0x000000000000b000ull, N("s8"), T(is2) },
	{ 0x0000000000001000ull, 0x000000000000b000ull, N("u16"), T(is2) },
	{ 0x0000000000009000ull, 0x000000000000b000ull, N("s16"), T(is2) },
	{ 0x0000000000002000ull, 0x000000000000b000ull, N("u32"), T(is2) },
	{ 0x000000000000a000ull, 0x000000000000b000ull, N("s32"), T(is2) },
	{ 0x0000000000003000ull, 0x000000000000b000ull, N("u64"), T(ds2) },
	{ 0x000000000000b000ull, 0x000000000000b000ull, N("s64"), T(ds2) },
	{ 0, 0, OOPS },
};

static struct insn tabcvti2isrc[] = {
	{ 0x0000000000000000ull, 0x000000000000b000ull, N("u8"), T(is2) },
	{ 0x0000000000008000ull, 0x000000000000b000ull, N("s8"), T(is2) },
	{ 0x0000000000001000ull, 0x000000000000b000ull, N("u16"), T(is2) },
	{ 0x0000000000009000ull, 0x000000000000b000ull, N("s16"), T(is2) },
	{ 0x0000000000002000ull, 0x000000000000b000ull, N("u32"), T(is2) },
	{ 0x000000000000a000ull, 0x000000000000b000ull, N("s32"), T(is2) },
	{ 0, 0, OOPS },
};
static struct insn tabcvti2idst[] = {
	{ 0x0000000000000000ull, 0x0000000000004c00ull, N("u8"), DST },
	{ 0x0000000000004000ull, 0x0000000000004c00ull, N("s8"), DST },
	{ 0x0000000000000400ull, 0x0000000000004c00ull, N("u16"), DST },
	{ 0x0000000000004400ull, 0x0000000000004c00ull, N("s16"), DST },
	{ 0x0000000000000800ull, 0x0000000000004c00ull, N("u32"), DST },
	{ 0x0000000000004800ull, 0x0000000000004c00ull, N("s32"), DST },
	// XXX: check 64 bit variants
	{ 0, 0, OOPS },
};

F1(sat35, 0x35, N("sat")) // add,mul f32
F1(sat3a, 0x3a, N("sat")) // mul f32 long immediate
F1(ftz2f, 0x2f, N("ftz")) // add,mul f32
F1(ftz38, 0x38, N("ftz")) // fma f32
F1(ftz3a, 0x3a, N("ftz")) // add f32 long immediate
F1(neg30, 0x30, N("neg")) // add f32,f64 src2
F1(neg33, 0x33, N("neg")) // add,mul,fma f32,f64 src1
F1(neg3b, 0x3b, N("neg")) // mul f64 immediate
F1(neg34, 0x34, N("neg")) // fma f32,f64 src3
F1(abs31, 0x31, N("abs")) // add f32 src1
F1(abs34, 0x34, N("abs")) // add f32,f64 src2
F1(abs39, 0x39, N("abs")) // add f32 long immediate
F1(not2b, 0x2b, N("not")) // logop src2

F1(acout32, 0x32, CC)
F1(acin2e, 0x2e, CC)

F1(pnot2d, 0x2d, N("not")) // minmax select

F(shfclamp, 0x35, N("clamp"), N("wrap"))

static struct insn tabminmax[] = {
	{ 0x00001c0000000000ull, 0x00003c0000000000ull, N("min") },
	{ 0x00003c0000000000ull, 0x00003c0000000000ull, N("max") },
	{ 0, 0, N("minmax"), T(pnot2d), PSRC1 }, // min if true
};

static struct insn tabaddop[] = {
	{ 0x0000000000000000ull, 0x0018000000000000ull, N("add") },
	{ 0x0008000000000000ull, 0x0018000000000000ull, N("sub") },
	{ 0x0010000000000000ull, 0x0018000000000000ull, N("subr") },
	{ 0x0018000000000000ull, 0x0018000000000000ull, N("addpo") }, // XXX: check me
	{ 0, 0, OOPS },
};

static struct insn tablogop[] = {
	{ 0x0000000000000000ull, 0x0000300000000000ull, N("and") },
	{ 0x0000100000000000ull, 0x0000300000000000ull, N("or") },
	{ 0x0000200000000000ull, 0x0000300000000000ull, N("xor") },
	{ 0x0000300000000000ull, 0x0000300000000000ull, N("mov2") },
	{ 0, 0, OOPS },
};
static struct insn tablogop38[] = {
	{ 0x0000000000000000ull, 0x0300000000000000ull, N("and") },
	{ 0x0100000000000000ull, 0x0300000000000000ull, N("or") },
	{ 0x0200000000000000ull, 0x0300000000000000ull, N("xor") },
	{ 0x0300000000000000ull, 0x0300000000000000ull, N("mov2") },
	{ 0, 0, OOPS },
};

static struct insn tabsfuop[] = {
	{ 0x00000000, 0x03800000, N("cos") },
	{ 0x00800000, 0x03800000, N("sin") },
	{ 0x01000000, 0x03800000, N("ex2") },
	{ 0x01800000, 0x03800000, N("lg2") },
	{ 0x02000000, 0x03800000, N("rcp") },
	{ 0x02800000, 0x03800000, N("rsqrt") },
	{ 0x03000000, 0x03800000, N("rcp64h") },
	{ 0x03800000, 0x03800000, N("rsqrt64h") },
	{ 0, 0, OOPS },
};

/*
 * Opcode format
 *
 * 0000000000000003 type (control, immediate, normal)
 * 00000000000003fc dst
 * 000000000003fc00 1st src
 * 00000000001c0000 predicate
 * 0000000000200000 negate predicate
 * 0000000000400000 join
 * 000000007f800000 2nd src
 * 0000003fff800000 2nd src (immediate)
 * 0000007fff800000 address
 * 007fffffff800000 long immediate
 * 0003fc0000000000 3rd src
 * 007c000000000000 misc flags
 * 3fc0000000000000 opcode (can contain rounding mode, ftz)
 * c000000000000000 source type
 */

static struct insn tabm[] = {
	{ 0x8400000003000002ull, 0xbfc0000003800003ull, N("rcp64h"), DST, T(neg33), T(abs31), SRC1 },
	{ 0x8400000003800002ull, 0xbfc0000003800003ull, N("rsqrt64h"), DST, T(neg33), T(abs31), SRC1 },
	{ 0x8400000000000002ull, 0xbfc0000000000003ull, T(sfuop), N("f32"), DST, T(neg33), T(abs31), SRC1 },
	{ 0x0c00000000000002ull, 0x3e00000000000003ull, N("fma"), T(ftz38), T(sat35),  T(frm36), N("f32"), DST, T(neg33), SRC1, T(is2w3), T(neg34), T(is3) },
	{ 0x1b80000000000002ull, 0x3f80000000000003ull, N("fma"), T(frm35), N("f64"), DSTD, T(neg33), SRC1D, T(ds2w3), T(neg34), T(ds3) },
	{ 0x1fc0000000000002ull, 0x3fc0000000000003ull, N("lshf"), N("b32"), DST, SESTART, N("b64"), SRC1, SRC3, SEEND, T(shfclamp), T(is2) }, // XXX: check is2 and bits 0x29,0x33(swap srcs ?)
	{ 0x2148000000000002ull, 0x3fc8000000000003ull, N("shr"), N("s32"), DST, SRC1, T(is2) }, // XXX: find shl and wrap bits
	{ 0x2000000000000002ull, 0xfa80000000000003ull, N("mul"), T(ftz38), T(sat3a), N("f32"), DST, SRC1, LIMM },
	{ 0x2080000000000002ull, 0x3fc0000000000003ull, T(addop), T(sat35), N("b32"), DST, T(acout32), SRC1, T(is2), T(acin2e) },
	{ 0x2200000000000002ull, 0x3fc0000000000003ull, T(logop), N("b32"), DST, SRC1, T(not2b), T(is2) },
	{ 0x2280000000000002ull, 0x3fc0000000000003ull, T(minmax), N("f64"), DSTD, T(neg33), T(abs31), SRC1D, T(neg30), T(abs34), T(ds2) },
	{ 0x22c0000000000002ull, 0x3fc0000000000003ull, N("add"), T(ftz2f), T(sat35), T(frm2a), N("f32"), DST, T(neg33), T(abs31), SRC1, T(neg30), T(abs34), T(is2) },
	{ 0x2300000000000002ull, 0x3fc0000000000003ull, T(minmax), N("f32"), DST, T(neg33), T(abs31), SRC1, T(neg30), T(abs34), T(is2) },
	{ 0x2340000000000002ull, 0x3fc0000000000003ull, N("mul"), T(ftz2f), T(sat35), T(frm2a), T(neg33), N("f32"), DST, SRC1, T(is2) },
	{ 0x2380000000000002ull, 0x3fc0000000000003ull, N("add"), T(frm2a), N("f64"), DSTD, T(neg33), T(abs31), SRC1D, T(neg30), T(abs34), T(ds2) },
	{ 0x2400000000000002ull, 0x3fc0000000000003ull, N("mul"), T(frm2a), T(neg33), N("f64"), DSTD, SRC1D, T(ds2) },
	{ 0x2480000000000002ull, 0x3fc0040000000003ull, N("presin"), N("f32"), DST, T(neg30), T(abs34), T(is2) },
	{ 0x2480040000000002ull, 0x3fc0040000000003ull, N("preex2"), N("f32"), DST, T(neg30), T(abs34), T(is2) },
	{ 0x24c0000000000002ull, 0x3fc0000000000003ull, T(lane2a), N("mov"), N("b32"), DST, T(is2) },
	{ 0x2540000000001402ull, 0x3fc0000000003c03ull, N("cvt"), T(sat35), T(frmi), N("f16"), DST, N("f16"), T(neg30), T(abs34), T(is2) },
	{ 0x2540000000001802ull, 0x3fc0000000003c03ull, N("cvt"), T(ftz2f), T(sat35), N("f32"), DST, N("f16"), T(neg30), T(abs34), T(is2) },
	{ 0x2540000000002402ull, 0x3fc0000000003c03ull, N("cvt"), T(ftz2f), T(sat35), T(frm2a), N("f16"), DST, N("f32"), T(neg30), T(abs34), T(is2) },
	{ 0x2540000000002802ull, 0x3fc0000000003c03ull, N("cvt"), T(ftz2f), T(sat35), T(frmi), N("f32"), DST, N("f32"), T(neg30), T(abs34), T(is2) },
	{ 0x2540000000002c02ull, 0x3fc0000000003c03ull, N("cvt"), N("f64"), DST, N("f32"), T(neg30), T(abs34), T(is2) }, // XXX: do ftz, sat work here ?
	{ 0x2540000000003802ull, 0x3fc0000000003c03ull, N("cvt"), T(frm2a), N("f32"), DST, N("f64"), T(neg30), T(abs34), T(is2) }, // XXX: do ftz, sat work here ?
	{ 0x2580000000000002ull, 0x3fc0000000000003ull, N("cvt"), T(ftz2f), T(frmi), T(cvtf2idst), T(neg30), T(abs34), T(cvtf2isrc) },
	{ 0x2600000000000002ull, 0x3fc0000000000003ull, N("cvt"), T(sat35), T(cvti2idst), T(neg30), T(abs34), T(cvti2isrc) },
	{ 0x25c0000000000002ull, 0x3fc0000000000003ull, N("cvt"), T(frm2a), T(cvti2fdst), T(neg30), T(abs34), T(cvti2fsrc) },
	{ 0x27c0000000000002ull, 0x3fc0000000000003ull, N("rshf"), N("b32"), DST, SESTART, N("b64"), SRC1, SRC3, SEEND, T(shfclamp), T(is2) }, // XXX: check is2 and bits 0x29,0x33(swap srcs ?)
	{ 0x0, 0x0, OOPS },
};

static struct insn tabi[] = {
	{ 0x0080000000000001ull, 0x37c0000000000003ull, T(addop), T(sat35), N("b32"), DST, T(acout32), SRC1, T(ii2) },
	{ 0x0148000000000001ull, 0x37c8000000000003ull, N("shr"), N("s32"), DST, SRC1, T(sui2b) }, // XXX: find shl and wrap bits
	{ 0x02c0000000000001ull, 0x37c0000000000003ull, N("add"), T(ftz2f), T(sat35), T(frm2a), N("f32"), DST, T(neg33), T(abs31), SRC1, T(neg30), T(abs34), T(is2) }, // XXX
	{ 0x0340000000000001ull, 0x37c0000000000003ull, N("mul"), T(ftz2f), T(sat35), T(frm2a), T(neg3b), N("f32"), DST, SRC1, T(fi2) },
	{ 0x0400000000000001ull, 0x37c0000000000003ull, N("mul"), T(frm2a), T(neg3b), N("f64"), DSTD, SRC1D, T(di2) },
	{ 0x07c0000000000001ull, 0x37c0000000000003ull, N("rshf"), N("b32"), DST, SESTART, N("b64"), SRC1, SRC3, SEEND, T(sui2b) }, // d = (s1 >> s2) | (s3 << (32 - s2))
	{ 0x1400000000000001ull, 0x37c0000000000003ull, N("fma"), T(ftz38), T(sat35), T(frm36), N("f32"), DST, T(neg33), SRC1, T(is2w3), T(neg34), T(is3) }, // XXX
	{ 0x37c0000000000001ull, 0x37c0000000000003ull, N("lshf"), N("b32"), DST, SESTART, N("b64"), SRC1, SRC3, SEEND, T(sui2a) }, // d = (s3 << s2) | (s1 >> (32 - s2))
	{ 0x0, 0x0, OOPS },
};

static struct insn tabp[] = {
	{ 0x001c0000, 0x003c0000 },
	{ 0x003c0000, 0x003c0000, N("never") },
	{ 0x00000000, 0x00200000, PRED },
	{ 0x00200000, 0x00200000, SESTART, N("not"), PRED, SEEND },
	{ 0, 0, OOPS },
};

static struct insn tablcop[] = {
	{ 0x0000000000000000ull, 0x1800000000000000ull, N("ca") },
	{ 0x0800000000000000ull, 0x1800000000000000ull, N("cg") },
	{ 0x1000000000000000ull, 0x1800000000000000ull, N("cs") },
	{ 0x1800000000000000ull, 0x1800000000000000ull, N("cv") },
	{ 0, 0, OOPS },
};
static struct insn tabscop[] = {
	{ 0x0000000000000000ull, 0x1800000000000000ull, N("wb") },
	{ 0x0800000000000000ull, 0x1800000000000000ull, N("cg") },
	{ 0x1000000000000000ull, 0x1800000000000000ull, N("cs") },
	{ 0x1800000000000000ull, 0x1800000000000000ull, N("wt") },
	{ 0, 0, OOPS },
};

static struct insn tabldstt[] = {
	{ 0x0000000000000000ull, 0x0700000000000000ull, N("u8") },
	{ 0x0100000000000000ull, 0x0700000000000000ull, N("s8") },
	{ 0x0200000000000000ull, 0x0700000000000000ull, N("u16") },
	{ 0x0300000000000000ull, 0x0700000000000000ull, N("s16") },
	{ 0x0400000000000000ull, 0x0700000000000000ull, N("b32") },
	{ 0x0500000000000000ull, 0x0700000000000000ull, N("b64") },
	{ 0x0600000000000000ull, 0x0700000000000000ull, N("b128") },
	{ 0, 0, OOPS },
};
static struct insn tabldstd[] = {
	{ 0x0000000000000000ull, 0x0700000000000000ull, DST },
	{ 0x0100000000000000ull, 0x0700000000000000ull, DST },
	{ 0x0200000000000000ull, 0x0700000000000000ull, DST },
	{ 0x0300000000000000ull, 0x0700000000000000ull, DST },
	{ 0x0400000000000000ull, 0x0700000000000000ull, DST },
	{ 0x0500000000000000ull, 0x0700000000000000ull, DSTD },
	{ 0x0600000000000000ull, 0x0700000000000000ull, DSTQ },
	{ 0, 0, OOPS, DST },
};

static struct insn tabc[] = {
	{ 0x0800000000000000ull, 0xfc00000000000000ull, N("sched"), SCHED },
	{ 0x1200000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("bra"), BTARG },
	{ 0x1480000000000000ull, 0xff80000000000000ull, N("joinat"), BTARG },
	{ 0x1800000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("exit") },

	{ 0x2000000000000000ull, 0xfc80000000000000ull, T(p), T(logop38), N("b32"), DST, SRC1, LIMM },
	{ 0x4000000000000000ull, 0xf180000000000000ull, T(p), N("add"), T(ftz3a), N("f32"), DST, T(neg3b), T(abs39), SRC1, LIMM },

	{ 0xc000000000000000ull, 0xe000000000000000ull, T(p), N("ld"), T(ldstt), T(ldstd), T(lcop), T(gmem) },
	{ 0xe000000000000000ull, 0xe000000000000000ull, T(p), N("st"), T(ldstt), T(scop), T(gmem), T(ldstd) },

	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	// control instructions
	{ 0x00000000, 0x00000003, OP8B, T(c) },
	// short immediate (fugly)
	{ 0x00000001, 0x00400003, OP8B, T(p), T(i) },
	{ 0x00400001, 0x00400003, OP8B, N("join"), T(p), T(i) },
	// normal
	{ 0x00000002, 0x00400003, OP8B, T(p), T(m) },
	{ 0x00400002, 0x00400003, OP8B, N("join"), T(p), T(m) },
	{ 0, 0, OOPS },
};

static void gk110_prep(struct disisa *isa) {
	// no variants yet
}

struct disisa gk110_isa_s = {
	tabroot,
	8,
	4,
	1,
	.i_need_nv50as_hack = 1,
	.prep = gk110_prep,
};
