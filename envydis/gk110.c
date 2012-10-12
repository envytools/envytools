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
static struct rbitfield i3bimmoff = { { 0x17, 19, 0x3b, 1 }, RBF_SIGNED };
static struct rbitfield suimmoff = { { 0x17, 6 }, RBF_UNSIGNED }; // XXX: for r/lshf, check size
static struct rbitfield shcntsoff = { { 0x2a, 5 }, RBF_UNSIGNED };
static struct rbitfield shcnlsoff = { { 0x38, 5 }, RBF_UNSIGNED };
static struct rbitfield fimmoff = { { 0x17, 19 }, .shr = 12 };
static struct rbitfield limmoff = { { 0x17, 32 }, .wrapok = 1 };
static struct rbitfield dimmoff = { { 0x17, 19 }, .shr = 44 };
static struct rbitfield schedvals = { { 0x2, 56 }, .wrapok = 1 };
#define UIMM atomrimm, &uimmoff
#define SUIMM atomrimm, &suimmoff
#define SHCNT atomrimm, &shcntsoff
#define SHCNL atomrimm, &shcnlsoff
#define I3BIMM atomrimm, &i3bimmoff
#define FIMM atomrimm, &fimmoff
#define DIMM atomrimm, &dimmoff
#define LIMM atomrimm, &limmoff
#define SCHED atomrimm, &schedvals

static struct bitfield baroff = { 0xA, 4 };
static struct bitfield texbaroff = { 0x17, 6 }; // XXX: check exact size
#define BAR atomimm, &baroff
#define TEXBARIMM atomimm, &texbaroff


/*
 * Register fields
 */

static struct sreg sreg_sr[] = {
	{ 0, "laneid" },
	{ 2, "nphysid" },
	{ 3, "physid" },
	{ 4, "pm0" },
	{ 5, "pm1" },
	{ 6, "pm2" },
	{ 7, "pm3" },
	{ 8, "pm4" },
	{ 9, "pm5" },
	{ 0xa, "pm6" },
	{ 0xb, "pm7" },
	{ 0x10, "vtxcnt" },
	{ 0x11, "invoc" },
	{ 0x12, "ydir" },
	{ 0x20, "tid" },
	{ 0x21, "tidx" },
	{ 0x22, "tidy" },
	{ 0x23, "tidz" },
	{ 0x24, "launcharg" },
	{ 0x25, "ctaidx" },
	{ 0x26, "ctaidy" },
	{ 0x27, "ctaidz" },
	{ 0x28, "ntid" },
	{ 0x29, "ntidx" },
	{ 0x2a, "ntidy" },
	{ 0x2b, "ntidz" },
	{ 0x2c, "gridid" },
	{ 0x2d, "nctaidx" },
	{ 0x2e, "nctaidy" },
	{ 0x2f, "nctaidz" },
	{ 0x30, "swinbase" },
	{ 0x31, "swinsz" },
	{ 0x32, "smemsz" },
	{ 0x33, "smembanks" },
	{ 0x34, "lwinbase" },
	{ 0x35, "lwinsz" },
	{ 0x36, "lpossz" },
	{ 0x37, "lnegstart" },
	{ 0x38, "lanemask_eq" },
	{ 0x39, "lanemask_lt" },
	{ 0x3a, "lanemask_le" },
	{ 0x3b, "lanemask_gt" },
	{ 0x3c, "lanemask_ge" },
	{ 0x40, "trapstat" },
	{ 0x42, "warperr" },
	{ 0x52, "clock" },
	{ 0x53, "clockhi" },
	{ -1 },
};

static struct sreg reg_sr[] = {
	{ 255, 0, SR_ZERO },
	{ -1 },
};
static struct sreg pred_sr[] = {
	{ 7, 0, SR_ONE },
	{ -1 },
};

static struct bitfield dst_bf = { 0x2, 8 };
static struct bitfield pdst_bf = { 0x5, 3 };
static struct bitfield pdstn_bf = { 0x2, 3 };
static struct bitfield src1_bf = { 0xa, 8 };
static struct bitfield src2_bf = { 0x17, 8 };
static struct bitfield src3_bf = { 0x2a, 8 };
static struct bitfield pred_bf = { 0x12, 3 };
static struct bitfield psrc3_bf = { 0x2a, 3 };
static struct bitfield sreg_bf = { 0x17, 8 };

static struct reg dst_r = { &dst_bf, "r", .specials = reg_sr };
static struct reg dstd_r = { &dst_bf, "r", "d", .specials = reg_sr };
static struct reg dstq_r = { &dst_bf, "r", "q" };
static struct reg src1_r = { &src1_bf, "r", .specials = reg_sr };
static struct reg src1d_r = { &src1_bf, "r", "d", .specials = reg_sr };
static struct reg src2_r = { &src2_bf, "r", .specials = reg_sr };
static struct reg src2d_r = { &src2_bf, "r", "d", .specials = reg_sr };
static struct reg src3_r = { &src3_bf, "r", .specials = reg_sr };
static struct reg src3d_r = { &src3_bf, "r", "d", .specials = reg_sr };
static struct reg psrc3_r = { &psrc3_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdst_r = { &pdst_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdstn_r = { &pdstn_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pred_r = { &pred_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg cc_r = { 0, "c", .cool = 1 };
static struct reg sreg_r = { &sreg_bf, "sr", .specials = sreg_sr, .always_special = 1 };

#define DST atomreg, &dst_r
#define DSTD atomreg, &dstd_r
#define DSTQ atomreg, &dstq_r
#define PDST atomreg, &pdst_r
#define PDSTN atomreg, &pdstn_r
#define PRED atomreg, &pred_r
#define SRC1 atomreg, &src1_r
#define SRC1D atomreg, &src1d_r
#define SRC2 atomreg, &src2_r
#define SRC2D atomreg, &src2d_r
#define SRC3 atomreg, &src3_r
#define SRC3D atomreg, &src3d_r
#define PSRC3 atomreg, &psrc3_r
#define CC atomreg, &cc_r
#define SREG atomreg, &sreg_r

static struct bitfield tdst_mask = { 0x22, 4 };
static struct bitfield cnt0 = { .addend = 0 };
static struct bitfield cnt1 = { .addend = 1 };
static struct bitfield cnt2 = { .addend = 2 };
static struct bitfield cnt3 = { .addend = 3 };
static struct bitfield cnt4 = { .addend = 4 };

static struct vec tdst_v = { "r", &dst_bf, &cnt4, &tdst_mask };
static struct vec tsrc11_v = { "r", &src1_bf, &cnt1, 0 };
static struct vec tsrc12_v = { "r", &src1_bf, &cnt2, 0 };
static struct vec tsrc13_v = { "r", &src1_bf, &cnt3, 0 };
static struct vec tsrc14_v = { "r", &src1_bf, &cnt4, 0 };
static struct vec tsrc20_v = { "r", &src2_bf, &cnt0, 0 };
static struct vec tsrc21_v = { "r", &src2_bf, &cnt1, 0 };
static struct vec tsrc22_v = { "r", &src2_bf, &cnt2, 0 };
static struct vec tsrc23_v = { "r", &src2_bf, &cnt3, 0 };
static struct vec tsrc24_v = { "r", &src2_bf, &cnt4, 0 };

#define TDST atomvec, &tdst_v
#define TSRC11 atomvec, &tsrc11_v
#define TSRC12 atomvec, &tsrc12_v
#define TSRC13 atomvec, &tsrc13_v
#define TSRC14 atomvec, &tsrc14_v
#define TSRC20 atomvec, &tsrc20_v
#define TSRC21 atomvec, &tsrc21_v
#define TSRC22 atomvec, &tsrc22_v
#define TSRC23 atomvec, &tsrc23_v
#define TSRC24 atomvec, &tsrc24_v

/*
 * Memory fields
 */

static struct rbitfield gmem_imm = { { 0x17, 32 }, RBF_SIGNED };
static struct rbitfield cmem_imm = { { 0x17, 14 }, RBF_UNSIGNED, .shr = 2 };
static struct rbitfield lcmem_imm = { { 0x17, 16 }, RBF_SIGNED };
static struct rbitfield lmem_imm = { { 0x17, 24 }, RBF_SIGNED };
static struct rbitfield smem_imm = { { 0x17, 24 }, RBF_SIGNED };
static struct rbitfield tcmem_imm = { { 0x2f, 8 }, .shr = 2 }; // XXX: could be 13 bits
static struct bitfield cmem_idx = { 0x25, 5 };
static struct bitfield lcmem_idx = { 0x27, 5 };

static struct mem gmem_m = { "g", 0, &src1_r, &gmem_imm };
static struct mem lmem_m = { "l", 0, &src1_r, &lmem_imm };
static struct mem smem_m = { "s", 0, &src1_r, &smem_imm };
static struct mem gdmem_m = { "g", 0, &src1d_r, &gmem_imm };
static struct mem cmem_m = { "c", &cmem_idx, 0, &cmem_imm };
static struct mem lcmem_m = { "c", &lcmem_idx, &src1_r, &lcmem_imm };
static struct mem tcmem_m = { "c", 0, 0, &tcmem_imm };

#define GLOBAL atommem, &gmem_m
#define GLOBALD atommem, &gdmem_m
#define LOCAL atommem, &lmem_m
#define SHARED atommem, &smem_m
#define CONST atommem, &cmem_m
#define LCONST atommem, &lcmem_m
#define TCONST atommem, &tcmem_m


/*
 * The instructions
 */

F(gmem, 0x37, GLOBAL, GLOBALD)

F1(pnot2d, 0x2d, N("not"))

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

static struct insn tabisetit[] = {
	{ 0x0000000000000000ull, 0x0070000000000000ull, N("false") },
	{ 0x0010000000000000ull, 0x0070000000000000ull, N("lt") },
	{ 0x0020000000000000ull, 0x0070000000000000ull, N("eq") },
	{ 0x0030000000000000ull, 0x0070000000000000ull, N("le") },
	{ 0x0040000000000000ull, 0x0070000000000000ull, N("gt") },
	{ 0x0050000000000000ull, 0x0070000000000000ull, N("ne") },
	{ 0x0060000000000000ull, 0x0070000000000000ull, N("ge") },
	{ 0x0070000000000000ull, 0x0070000000000000ull, N("true") },
	{ 0, 0, OOPS },
};
static struct insn tabsetit[] = {
	{ 0x0000000000000000ull, 0x0078000000000000ull, N("false") },
	{ 0x0008000000000000ull, 0x0078000000000000ull, N("lt") },
	{ 0x0010000000000000ull, 0x0078000000000000ull, N("eq") },
	{ 0x0018000000000000ull, 0x0078000000000000ull, N("le") },
	{ 0x0020000000000000ull, 0x0078000000000000ull, N("gt") },
	{ 0x0028000000000000ull, 0x0078000000000000ull, N("ne") },
	{ 0x0030000000000000ull, 0x0078000000000000ull, N("ge") },
	{ 0x0038000000000000ull, 0x0078000000000000ull, N("num") },
	{ 0x0040000000000000ull, 0x0078000000000000ull, N("nan") },
	{ 0x0048000000000000ull, 0x0078000000000000ull, N("ltu") },
	{ 0x0050000000000000ull, 0x0078000000000000ull, N("equ") },
	{ 0x0058000000000000ull, 0x0078000000000000ull, N("leu") },
	{ 0x0060000000000000ull, 0x0078000000000000ull, N("gtu") },
	{ 0x0068000000000000ull, 0x0078000000000000ull, N("neu") },
	{ 0x0070000000000000ull, 0x0078000000000000ull, N("geu") },
	{ 0x0078000000000000ull, 0x0078000000000000ull, N("true") },
	{ 0, 0, OOPS },
};
static struct insn tabsetlop[] = {
	{ 0x00001c0000000000ull, 0x00031c0000000000ull, N("") }, // noop, really "and $p7"
	{ 0x0000000000000000ull, 0x0003000000000000ull, N("and"), T(pnot2d), PSRC3 },
	{ 0x0001000000000000ull, 0x0003000000000000ull, N("or"), T(pnot2d), PSRC3 },
	{ 0x0002000000000000ull, 0x0003000000000000ull, N("xor"), T(pnot2d), PSRC3 },
	{ 0, 0, OOPS, T(pnot2d), PSRC3 },
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
static struct insn tablane0e[] = {
	{ 0x00000000, 0x0003c000, N("lnone") },
	{ 0x00004000, 0x0003c000, N("l0") },
	{ 0x00008000, 0x0003c000, N("l1") },
	{ 0x0000c000, 0x0003c000, N("l01") },
	{ 0x00010000, 0x0003c000, N("l2") },
	{ 0x00014000, 0x0003c000, N("l02") },
	{ 0x00018000, 0x0003c000, N("l12") },
	{ 0x0001c000, 0x0003c000, N("l012") },
	{ 0x00020000, 0x0003c000, N("l3") },
	{ 0x00024000, 0x0003c000, N("l03") },
	{ 0x00028000, 0x0003c000, N("l13") },
	{ 0x0002c000, 0x0003c000, N("l013") },
	{ 0x00030000, 0x0003c000, N("l23") },
	{ 0x00034000, 0x0003c000, N("l023") },
	{ 0x00038000, 0x0003c000, N("l123") },
	{ 0x0003c000, 0x0003c000 },
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

static struct insn tabi3bi2[] = {
	{ 0xc000000000000000ull, 0xc000000000000000ull, I3BIMM },
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

static struct insn tabtexsrc1[] = { // XXX: find shadow and offset bits
	// target
	{ 0x0000000000000000ull, 0x100021c000000000ull, N("x###"), TSRC11 },
	{ 0x0000004000000000ull, 0x100021c000000000ull, N("ax##"), TSRC12 },
	{ 0x0000008000000000ull, 0x100021c000000000ull, N("xy##"), TSRC12 },
	{ 0x000000c000000000ull, 0x100021c000000000ull, N("axy#"), TSRC13 },
	{ 0x0000010000000000ull, 0x1000214000000000ull, N("xyz#"), TSRC13 },
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// target + lod/bias
	{ 0x0000200000000000ull, 0x100021c000000000ull, N("xl##"), TSRC12 },
	{ 0x0000204000000000ull, 0x100021c000000000ull, N("axl#"), TSRC13 },
	{ 0x0000208000000000ull, 0x100021c000000000ull, N("xyl#"), TSRC13 },
	{ 0x000020c000000000ull, 0x100021c000000000ull, N("axyl"), TSRC14 },
	{ 0x0000210000000000ull, 0x1000214000000000ull, N("xyzl"), TSRC14 },
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// target + lod/bias + dc
	{ 0x0000200000000000ull, 0x100021c000000000ull, N("xld#"), TSRC13 },
	{ 0x0000204000000000ull, 0x100021c000000000ull, N("axld"), TSRC14 },
	{ 0x0000208000000000ull, 0x100021c000000000ull, N("xyld"), TSRC14 },
	{ 0x000020c000000000ull, 0x100021c000000000ull, N("axyl"), TSRC14 },
	{ 0x0000210000000000ull, 0x1000214000000000ull, N("xyzl"), TSRC14 },
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// target + lod/bias + dc + offset
	{ 0x0000200000000000ull, 0x100021c000000000ull, N("xlod"), TSRC14 },
	{ 0x0000204000000000ull, 0x100021c000000000ull, N("axlo"), TSRC14 },
	{ 0x0000208000000000ull, 0x100021c000000000ull, N("xylo"), TSRC14 },
	{ 0x000020c000000000ull, 0x100021c000000000ull, N("axyl"), TSRC14 },
	{ 0x0000210000000000ull, 0x1000214000000000ull, N("xyzl"), TSRC14 },
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// target + lod/bias + offset
	{ 0x0000200000000000ull, 0x100021c000000000ull, N("xlo#"), TSRC13 },
	{ 0x0000204000000000ull, 0x100021c000000000ull, N("axlo"), TSRC14 },
	{ 0x0000208000000000ull, 0x100021c000000000ull, N("xylo"), TSRC14 },
	{ 0x000020c000000000ull, 0x100021c000000000ull, N("axyl"), TSRC14 },
	{ 0x0000210000000000ull, 0x1000214000000000ull, N("xyzl"), TSRC14 },
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// target + dc
	{ 0x0000000000000000ull, 0x100021c000000000ull, N("xd##"), TSRC12 },
	{ 0x0000004000000000ull, 0x100021c000000000ull, N("axd#"), TSRC13 },
	{ 0x0000008000000000ull, 0x100021c000000000ull, N("xyd#"), TSRC13 },
	{ 0x000000c000000000ull, 0x100021c000000000ull, N("axyd"), TSRC14 },
	{ 0x0000010000000000ull, 0x1000214000000000ull, N("xyzd"), TSRC14 },
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// target + dc + offset
	{ 0x0000000000000000ull, 0x100021c000000000ull, N("xod#"), TSRC13 },
	{ 0x0000004000000000ull, 0x100021c000000000ull, N("axod"), TSRC14 },
	{ 0x0000008000000000ull, 0x100021c000000000ull, N("xyod"), TSRC14 },
	{ 0x000000c000000000ull, 0x100021c000000000ull, N("axyo"), TSRC14 },
	{ 0x0000010000000000ull, 0x1000214000000000ull, N("xyzo"), TSRC14 },
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// target + offset
	{ 0x0000000000000000ull, 0x100021c000000000ull, N("xo##"), TSRC12 },
	{ 0x0000004000000000ull, 0x100021c000000000ull, N("axo#"), TSRC13 },
	{ 0x0000008000000000ull, 0x100021c000000000ull, N("xyo#"), TSRC13 },
	{ 0x000000c000000000ull, 0x100021c000000000ull, N("axyo"), TSRC14 },
	{ 0x0000010000000000ull, 0x1000214000000000ull, N("xyzo"), TSRC14 },
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// ind + target
	{ 0x1000000000000000ull, 0x100021c000000000ull, N("ix##"), TSRC12 },
	{ 0x1000004000000000ull, 0x100021c000000000ull, N("iax#"), TSRC13 },
	{ 0x1000008000000000ull, 0x100021c000000000ull, N("ixy#"), TSRC13 },
	{ 0x100000c000000000ull, 0x100021c000000000ull, N("iaxy"), TSRC14 },
	{ 0x1000010000000000ull, 0x1000214000000000ull, N("ixyz"), TSRC14 },
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("iaxy"), TSRC14 },
	// ind + target + lod/bias
	{ 0x1000200000000000ull, 0x100021c000000000ull, N("ixl#"), TSRC13 },
	{ 0x1000204000000000ull, 0x100021c000000000ull, N("iaxl"), TSRC14 },
	{ 0x1000208000000000ull, 0x100021c000000000ull, N("ixyl"), TSRC14 },
	{ 0x100020c000000000ull, 0x100021c000000000ull, N("iaxy"), TSRC14 },
	{ 0x1000210000000000ull, 0x1000214000000000ull, N("ixyz"), TSRC14 },
	{ 0x1000214000000000ull, 0x1000214000000000ull, N("iaxy"), TSRC14 },
	// ind + target + lod/bias + dc
	{ 0x1000200000000000ull, 0x100021c000000000ull, N("ixld"), TSRC14 },
	{ 0x1000204000000000ull, 0x100021c000000000ull, N("iaxl"), TSRC14 },
	{ 0x1000208000000000ull, 0x100021c000000000ull, N("ixyl"), TSRC14 },
	{ 0x100020c000000000ull, 0x100021c000000000ull, N("iaxy"), TSRC14 },
	{ 0x1000210000000000ull, 0x1000214000000000ull, N("ixyz"), TSRC14 },
	{ 0x1000214000000000ull, 0x1000214000000000ull, N("iaxy"), TSRC14 },
	// ind + target + lod/bias + dc + offset
	{ 0x1000200000000000ull, 0x100021c000000000ull, N("ixlo"), TSRC14 },
	{ 0x1000204000000000ull, 0x100021c000000000ull, N("iaxl"), TSRC14 },
	{ 0x1000208000000000ull, 0x100021c000000000ull, N("ixyl"), TSRC14 },
	{ 0x100020c000000000ull, 0x100021c000000000ull, N("iaxy"), TSRC14 },
	{ 0x1000210000000000ull, 0x1000214000000000ull, N("ixyz"), TSRC14 },
	{ 0x1000214000000000ull, 0x1000214000000000ull, N("iaxy"), TSRC14 },
	// ind + target + lod/bias + offset
	{ 0x1000200000000000ull, 0x100021c000000000ull, N("ixlo"), TSRC14 },
	{ 0x1000204000000000ull, 0x100021c000000000ull, N("iaxl"), TSRC14 },
	{ 0x1000208000000000ull, 0x100021c000000000ull, N("ixyl"), TSRC14 },
	{ 0x100020c000000000ull, 0x100021c000000000ull, N("iaxy"), TSRC14 },
	{ 0x1000210000000000ull, 0x1000214000000000ull, N("ixyz"), TSRC14 },
	{ 0x1000214000000000ull, 0x1000214000000000ull, N("iaxy"), TSRC14 },
	// ind + target + dc
	{ 0x1000000000000000ull, 0x100021c000000000ull, N("ixd#"), TSRC13 },
	{ 0x1000004000000000ull, 0x100021c000000000ull, N("iaxd"), TSRC14 },
	{ 0x1000008000000000ull, 0x100021c000000000ull, N("ixyd"), TSRC14 },
	{ 0x100000c000000000ull, 0x100021c000000000ull, N("iaxy"), TSRC14 },
	{ 0x1000010000000000ull, 0x1000214000000000ull, N("ixyz"), TSRC14 },
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("iaxy"), TSRC14 },
	// ind + target + dc + offset
	{ 0x1000000000000000ull, 0x100021c000000000ull, N("ixod"), TSRC14 },
	{ 0x1000004000000000ull, 0x100021c000000000ull, N("iaxo"), TSRC14 },
	{ 0x1000008000000000ull, 0x100021c000000000ull, N("ixyo"), TSRC14 },
	{ 0x100000c000000000ull, 0x100021c000000000ull, N("iaxy"), TSRC14 },
	{ 0x1000010000000000ull, 0x1000214000000000ull, N("ixyz"), TSRC14 },
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("iaxy"), TSRC14 },
	// ind + target + offset
	{ 0x1000000000000000ull, 0x100021c000000000ull, N("ixo#"), TSRC13 },
	{ 0x1000004000000000ull, 0x100021c000000000ull, N("iaxo"), TSRC14 },
	{ 0x1000008000000000ull, 0x100021c000000000ull, N("ixyo"), TSRC14 },
	{ 0x100000c000000000ull, 0x100021c000000000ull, N("iaxy"), TSRC14 },
	{ 0x1000010000000000ull, 0x1000214000000000ull, N("ixyz"), TSRC14 },
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("iaxy"), TSRC14 },
};
static struct insn tabtexsrc2[] = {
	// target + lod/bias
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("l###"), TSRC21 },
	// target + lod/bias + dc
	{ 0x000020c000000000ull, 0x100021c000000000ull, N("d###"), TSRC21 },
	{ 0x0000210000000000ull, 0x1000214000000000ull, N("d###"), TSRC21 },
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("ld##"), TSRC22 },
	// target + lod/bias + dc + offset
	{ 0x0000204000000000ull, 0x100021c000000000ull, N("d###"), TSRC21 },
	{ 0x0000208000000000ull, 0x100021c000000000ull, N("d###"), TSRC21 },
	{ 0x000020c000000000ull, 0x100021c000000000ull, N("od##"), TSRC22 },
	{ 0x0000210000000000ull, 0x1000214000000000ull, N("od##"), TSRC22 },
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("lod#"), TSRC23 },
	// target + lod/bias + offset
	{ 0x000020c000000000ull, 0x100021c000000000ull, N("o###"), TSRC21 },
	{ 0x0000210000000000ull, 0x1000214000000000ull, N("o###"), TSRC21 },
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("lo##"), TSRC22 },
	// target + dc
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("d###"), TSRC21 },
	// target + dc + offset
	{ 0x000000c000000000ull, 0x100021c000000000ull, N("d###"), TSRC21 },
	{ 0x0000010000000000ull, 0x1000214000000000ull, N("d###"), TSRC21 },
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("od##"), TSRC22 },
	// target + offset
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("o###"), TSRC21 },
	// ind + target
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("z###"), TSRC21 },
	// ind + target + lod/bias
	{ 0x100020c000000000ull, 0x100021c000000000ull, N("l###"), TSRC21 },
	{ 0x1000210000000000ull, 0x1000214000000000ull, N("l###"), TSRC21 },
	{ 0x1000214000000000ull, 0x1000214000000000ull, N("zl##"), TSRC22 },
	// ind + target + lod/bias + dc
	{ 0x1000204000000000ull, 0x100021c000000000ull, N("d###"), TSRC21 },
	{ 0x1000208000000000ull, 0x100021c000000000ull, N("d###"), TSRC21 },
	{ 0x100020c000000000ull, 0x100021c000000000ull, N("ld##"), TSRC22 },
	{ 0x1000210000000000ull, 0x1000214000000000ull, N("ld##"), TSRC22 },
	{ 0x1000214000000000ull, 0x1000214000000000ull, N("zld#"), TSRC23 },
	// ind + target + lod/bias + dc + offset
	{ 0x1000200000000000ull, 0x100021c000000000ull, N("d###"), TSRC21 },
	{ 0x1000204000000000ull, 0x100021c000000000ull, N("od##"), TSRC22 },
	{ 0x1000208000000000ull, 0x100021c000000000ull, N("od##"), TSRC22 },
	{ 0x100020c000000000ull, 0x100021c000000000ull, N("lod#"), TSRC23 },
	{ 0x1000210000000000ull, 0x1000214000000000ull, N("lod#"), TSRC23 },
	{ 0x1000214000000000ull, 0x1000214000000000ull, N("zlod"), TSRC24 },
	// ind + target + lod/bias + offset
	{ 0x1000204000000000ull, 0x100021c000000000ull, N("o###"), TSRC21 },
	{ 0x1000208000000000ull, 0x100021c000000000ull, N("o###"), TSRC21 },
	{ 0x100020c000000000ull, 0x100021c000000000ull, N("lo##"), TSRC22 },
	{ 0x1000210000000000ull, 0x1000214000000000ull, N("lo##"), TSRC22 },
	{ 0x1000214000000000ull, 0x1000214000000000ull, N("zlo#"), TSRC23 },
	// ind + target + dc
	{ 0x100000c000000000ull, 0x100021c000000000ull, N("d###"), TSRC21 },
	{ 0x1000010000000000ull, 0x1000214000000000ull, N("d###"), TSRC21 },
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("zd##"), TSRC22 },
	// ind + target + dc + offset
	{ 0x1000004000000000ull, 0x100021c000000000ull, N("d###"), TSRC21 },
	{ 0x1000008000000000ull, 0x100021c000000000ull, N("d###"), TSRC21 },
	{ 0x100000c000000000ull, 0x100021c000000000ull, N("od##"), TSRC22 },
	{ 0x1000010000000000ull, 0x1000214000000000ull, N("od##"), TSRC22 },
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("zod#"), TSRC23 },
	// ind + target + offset
	{ 0x100000c000000000ull, 0x100021c000000000ull, N("o###"), TSRC21 },
	{ 0x1000010000000000ull, 0x1000214000000000ull, N("o###"), TSRC21 },
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("zo##"), TSRC22 },
   // rest
	{ 0, 0, SRC2 },
};
static struct insn tabtexgrsrc1[] = {
	{ 0x0000000000000000ull, 0x080001c000000000ull, N("xgg#"), TSRC13 },
	{ 0x0000004000000000ull, 0x080001c000000000ull, N("axgg"), TSRC14 },
	{ 0x0000008000000000ull, 0x080001c000000000ull, N("xygg"), TSRC14 },
	{ 0x000000c000000000ull, 0x080001c000000000ull, N("axyg"), TSRC14 },
	{ 0x0800000000000000ull, 0x080001c000000000ull, N("ixgg"), TSRC14 },
	{ 0x0800004000000000ull, 0x080001c000000000ull, N("iaxg"), TSRC14 },
	{ 0x0800008000000000ull, 0x080001c000000000ull, N("ixyg"), TSRC14 },
	{ 0x080000c000000000ull, 0x080001c000000000ull, N("iaxy"), TSRC14 },
	{ 0, 0, OOPS },
};
static struct insn tabtexgrsrc2[] = {
	{ 0x0000008000000000ull, 0x080001c000000000ull, N("gg##"), TSRC22 },
	{ 0x000000c000000000ull, 0x080001c000000000ull, N("gg##"), TSRC22 },
	{ 0x0800004000000000ull, 0x080001c000000000ull, N("g###"), TSRC21 },
	{ 0x0800008000000000ull, 0x080001c000000000ull, N("ggg#"), TSRC23 },
	{ 0x080000c000000000ull, 0x080001c000000000ull, N("gggg"), TSRC24 },
   { 0, 0, SRC2 },
	{ 0, 0, OOPS },
};
static struct insn tabtconst[] = {
	{ 0x4000000000000000ull, 0xc000000000000000ull, TCONST },
	{ 0, 0, OOPS },
};
static struct insn tabtexm[] = {
	{ 0x0000000000000000ull, 0x0000000300000000ull },
	{ 0x0000000100000000ull, 0x0000000300000000ull, N("t") },
	{ 0x0000000200000000ull, 0x0000000300000000ull, N("p") },
	{ 0, 0, OOPS },
};
static struct insn tabtext[] = {
	{ 0x0000000000000000ull, 0x000001c000000000ull, N("t1d") },
	{ 0x0000004000000000ull, 0x000001c000000000ull, N("a1d") },
	{ 0x0000008000000000ull, 0x000001c000000000ull, N("t2d") },
	{ 0x000000c000000000ull, 0x000001c000000000ull, N("a2d") },
	{ 0x0000010000000000ull, 0x000001c000000000ull, N("t3d") },
	{ 0x0000018000000000ull, 0x000001c000000000ull, N("tcube") },
	{ 0x000001c000000000ull, 0x000001c000000000ull, N("acube") },
	{ 0, 0, OOPS },
};
static struct insn tablodt[] = {
	{ 0x0000000000000000ull, 0x0000700000000000ull, N("lauto") },
	{ 0x0000100000000000ull, 0x0000700000000000ull, N("lzero") },
	{ 0x0000200000000000ull, 0x0000700000000000ull, N("lbias") },
	{ 0x0000300000000000ull, 0x0000700000000000ull, N("llod") },
	{ 0x0000600000000000ull, 0x0000700000000000ull, N("lbiasa") },
	{ 0x0000700000000000ull, 0x0000700000000000ull, N("lloda") },
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
F1(sat39, 0x39, N("sat")) // mad s32
F1(sat3a, 0x3a, N("sat")) // mul f32 long immediate
F1(ftz2f, 0x2f, N("ftz")) // add,mul f32
F1(ftz32, 0x32, N("ftz")) // setp f32
F1(ftz38, 0x38, N("ftz")) // fma f32
F1(ftz3a, 0x3a, N("ftz")) // add f32 long immediate
F1(neg8, 0x8, N("neg")) // setp f32 src2
F1(neg2e, 0x2e, N("neg")) // setp f32 src1
F1(neg30, 0x30, N("neg")) // add f32,f64 src2
F1(neg33, 0x33, N("neg")) // add,mul,fma f32,f64 src1
F1(neg38, 0x38, N("neg")) // set f32
F1(neg3b, 0x3b, N("neg")) // mul f64 immediate
F1(neg34, 0x34, N("neg")) // fma f32,f64 src3
F1(abs9, 0x9, N("abs")) // setp f32 src1
F1(abs2f, 0x2f, N("abs")) // setp f32 src2
F1(abs31, 0x31, N("abs")) // add f32 src1
F1(abs34, 0x34, N("abs")) // add f32,f64 src2
F1(abs39, 0x39, N("abs")) // add f32 long immediate
F1(not2a, 0x2a, N("not")) // popc
F1(not2b, 0x2b, N("not")) // logop src2

F1(rint, 0x2d, T(frmi))

F1(acout32, 0x32, CC)
F1(acin2e, 0x2e, CC)
F1(acin34, 0x34, CC)

F(shfclamp, 0x35, N("clamp"), N("wrap"))

F(us64_28, 0x28, N("u64"), N("s64"))
F(us64_29, 0x29, N("u64"), N("s64"))
F(us32_2b, 0x2b, N("u32"), N("s32"))
F(us32_2c, 0x2c, N("u32"), N("s32"))
F(us32_33, 0x33, N("u32"), N("s32"))
F(us32_38, 0x38, N("u32"), N("s32"))
F(us32_39, 0x39, N("u32"), N("s32"))
F(us32_3a, 0x3a, N("u32"), N("s32"))

F1(high2a, 0x2a, N("high"))
F1(high39, 0x39, N("high"))

static struct insn tabminmax[] = {
	{ 0x00001c0000000000ull, 0x00003c0000000000ull, N("min") },
	{ 0x00003c0000000000ull, 0x00003c0000000000ull, N("max") },
	{ 0, 0, N("minmax"), T(pnot2d), PSRC3 }, // min if true
};

static struct insn tabaddop[] = {
	{ 0x0000000000000000ull, 0x0018000000000000ull, N("add") },
	{ 0x0008000000000000ull, 0x0018000000000000ull, N("sub") },
	{ 0x0010000000000000ull, 0x0018000000000000ull, N("subr") },
	{ 0x0018000000000000ull, 0x0018000000000000ull, N("addpo") }, // XXX: check me
	{ 0, 0, OOPS },
};
static struct insn tabaddop3a[] = {
	{ 0x0000000000000000ull, 0x00c0000000000000ull, N("add") },
	{ 0x0040000000000000ull, 0x00c0000000000000ull, N("sub") },
	{ 0x0080000000000000ull, 0x00c0000000000000ull, N("subr") },
	{ 0x00c0000000000000ull, 0x00c0000000000000ull, N("addpo") }, // XXX: check me
	{ 0, 0, OOPS },
};
static struct insn tabaddop3b[] = {
	{ 0x0000000000000000ull, 0x0800000000000000ull, N("add") },
	{ 0x0800000000000000ull, 0x0800000000000000ull, N("subr") },
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
static struct insn tabllcop[] = {
	{ 0x0000000000000000ull, 0x0001800000000000ull, N("ca") },
	{ 0x0000800000000000ull, 0x0001800000000000ull, N("cg") },
	{ 0x0001000000000000ull, 0x0001800000000000ull, N("cs") },
	{ 0x0001800000000000ull, 0x0001800000000000ull, N("cv") },
	{ 0, 0, OOPS },
};
static struct insn tablscop[] = {
	{ 0x0000000000000000ull, 0x0001800000000000ull, N("wb") },
	{ 0x0000800000000000ull, 0x0001800000000000ull, N("cg") },
	{ 0x0001000000000000ull, 0x0001800000000000ull, N("cs") },
	{ 0x0001800000000000ull, 0x0001800000000000ull, N("wt") },
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
static struct insn tablldstt[] = {
	{ 0x0000000000000000ull, 0x0038000000000000ull, N("u8") },
	{ 0x0008000000000000ull, 0x0038000000000000ull, N("s8") },
	{ 0x0010000000000000ull, 0x0038000000000000ull, N("u16") },
	{ 0x0018000000000000ull, 0x0038000000000000ull, N("s16") },
	{ 0x0020000000000000ull, 0x0038000000000000ull, N("b32") },
	{ 0x0028000000000000ull, 0x0038000000000000ull, N("b64") },
	{ 0x0030000000000000ull, 0x0038000000000000ull, N("b128") },
	{ 0, 0, OOPS },
};
static struct insn tablldstd[] = {
	{ 0x0000000000000000ull, 0x0038000000000000ull, DST },
	{ 0x0008000000000000ull, 0x0038000000000000ull, DST },
	{ 0x0010000000000000ull, 0x0038000000000000ull, DST },
	{ 0x0018000000000000ull, 0x0038000000000000ull, DST },
	{ 0x0020000000000000ull, 0x0038000000000000ull, DST },
	{ 0x0028000000000000ull, 0x0038000000000000ull, DSTD },
	{ 0x0030000000000000ull, 0x0038000000000000ull, DSTQ },
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

/* <WIP>
observed flag changes:
I[0]=I[1]<<29;
    00000048: 0e9c080d b7c00e00     lshf b32 $r3 (b64 $r2 $r3) 0x1d [unknown: 00000000 00000200]
    00000050: 0e9ffc09 b7c00800     lshf b32 $r2 (b64 0x0 $r2) 0x1d
I[0]=I[1]<<33;
    00000048: 109c080d b7c00e00     lshf b32 $r3 (b64 $r2 $r3) 0x21 [unknown: 00000000 00000200]
    00000050: 109ffc09 b7c00a00     lshf b32 $r2 (b64 0x0 $r2) 0x21 [unknown: 00000000 00000200]

ld.global.ca.nc.b32 %r4,[%rd1];
    00000020: 7f9c0801 60021084     tex lzero $r0:#:#:# t2d c[0x10] xy## $r2:$r3 0x0
ld.global.cs.nc.b32 %r4,[%rd1];
    00000020: 7f9c0802 70008484     $r0 $r2 0x0 $r33 ??? [unknown: 00000000 70000084] [unknown instruction]
ld.global.cg.nc.b32 %r4,[%rd1];
    00000020: 7f9c0802 70008084     $r0 $r2 0x0 $r32 ??? [unknown: 00000000 70000084] [unknown instruction]

atom.global.exch.b32 r4,  [ptr], r1;   
    00000028: 001c0802 6c080000     mul $r0 u32 $r2 s32 0x10000000 [unknown: 00000000 40000000]             //OOPS
    00000030: 001c0816 7b800000     $r5 $r2 $r0 $r0 ??? [unknown: 00000000 7b800000] [unknown instruction]
</WIP> */

static struct insn tabm[] = {
	{ 0x8400000000000002ull, 0xbfc0000000000003ull, T(sfuop), N("f32"), DST, T(neg33), T(abs31), SRC1 },
	{ 0x8640000000000002ull, 0xbfc0000000000003ull, N("mov"), N("b32"), DST, SREG },
	{ 0x0000000000000002ull, 0x38c0000000000003ull, N("set"), T(ftz3a), N("b32"), DST, T(setit), N("f32"), T(neg2e), T(abs39), SRC1, T(neg38), T(abs2f), T(is2), T(setlop) }, // XXX: find f32 dst
	{ 0x0800000000000002ull, 0x3cc0000000000003ull, N("set"), N("b32"), DST, T(setit), N("f64"), T(neg2e), T(abs39), SRC1D, T(neg38), T(abs2f), T(ds2), T(setlop) },
	{ 0x0c00000000000002ull, 0x3e00000000000003ull, N("fma"), T(ftz38), T(sat35),  T(frm36), N("f32"), DST, T(neg33), SRC1, T(is2w3), T(neg34), T(is3) },
	{ 0x1000000000000002ull, 0x3c00000000000003ull, T(addop3a), T(sat35), DST, T(acout32), SESTART, N("mul"), T(high39), T(us32_33), SRC1, T(us32_38), T(is2w3), SEEND, T(is3), T(acin34) }, // XXX: order of us32
	{ 0x1a00000000000002ull, 0x3fc0000000000003ull, N("slct"), N("b32"), DST, SRC1, T(is2w3), T(isetit), T(us32_33), T(is3) }, // XXX: check us32_33
	{ 0x1a80000000000002ull, 0x3f80000000000003ull, N("set"), N("b32"), DST, T(isetit), T(us32_33), SRC1, T(is2), T(setlop) },
	{ 0x1b00000000000002ull, 0x3f80000000000003ull, N("set"), PDST, PDSTN, T(isetit), T(us32_33), SRC1, T(is2), T(acin2e), T(setlop) },
	{ 0x1b80000000000002ull, 0x3f80000000000003ull, N("fma"), T(frm35), N("f64"), DSTD, T(neg33), SRC1D, T(ds2w3), T(neg34), T(ds3) },
	{ 0x1c00000000000002ull, 0x3f80000000000003ull, N("set"), PDST, PDSTN, T(setit), N("f64"), T(neg2e), T(abs9), SRC1D, T(neg8), T(abs2f), T(ds2), T(setlop) },
	{ 0x1d00000000000002ull, 0x3f80000000000003ull, N("slct"), T(ftz32), N("b32"), DST, SRC1, T(is2w3), T(setit), N("f32"), T(is3) },
	{ 0x1d80000000000002ull, 0x3f80000000000003ull, N("set"), T(ftz32), PDST, PDSTN, T(setit), N("f32"), T(neg2e), T(abs9), SRC1, T(neg8), T(abs2f), T(is2), T(setlop) },
	{ 0x1f40000000000002ull, 0x3fc0000000000003ull, N("sad"), T(us32_33), DST, SRC1, T(is2w3), T(is3) },
	{ 0x1f80000000000002ull, 0x3fc0000000000003ull, N("ins"), N("b32"), DST, SRC1, T(is2w3), T(is3) },
	{ 0x1fc0000000000002ull, 0x3fc0000000000003ull, N("lshf"), N("b32"), DST, SESTART, N("b64"), SRC1, SRC3, SEEND, T(shfclamp), T(is2) }, // XXX: check is2 and bits 0x29,0x33(swap srcs ?)
	{ 0x2148000000000002ull, 0x3fc8000000000003ull, N("shr"), N("s32"), DST, SRC1, T(is2) }, // XXX: find shl and wrap bits
	{ 0x2000000000000002ull, 0xfa80000000000003ull, N("mul"), T(ftz38), T(sat3a), N("f32"), DST, SRC1, LIMM },
	{ 0x2040000000000002ull, 0x3fc0000000000003ull, N("popc"), N("b32"), DST, T(not2a), SRC1, T(not2b), T(is2) }, // popc(src1 & src2)
	{ 0x2080000000000002ull, 0x3fc0000000000003ull, T(addop), T(sat35), N("b32"), DST, T(acout32), SRC1, T(is2), T(acin2e) },
	{ 0x20c0000000000002ull, 0x3fc0000000000003ull, T(addop), N("b32"), DST, N("shl"), SRC1, SHCNT, T(is2) },     //XXX: cin, cout
	{ 0x2100000000000002ull, 0x3fc0000000000003ull, T(minmax), T(us32_33), DST, SRC1, T(is2) },
	{ 0x21c0000000000002ull, 0x3fc0000000000003ull, N("mul"), T(high2a), DST, T(acout32), T(us32_2b), SRC1, T(us32_2c), T(is2) }, // XXX: order of us32
	{ 0x2200000000000002ull, 0x3fc0000000000003ull, T(logop), N("b32"), DST, SRC1, T(not2b), T(is2) },
	{ 0x2280000000000002ull, 0x3fc0000000000003ull, T(minmax), N("f64"), DSTD, T(neg33), T(abs31), SRC1D, T(neg30), T(abs34), T(ds2) },
	{ 0x22c0000000000002ull, 0x3fc0000000000003ull, N("add"), T(ftz2f), T(sat35), T(frm2a), N("f32"), DST, T(neg33), T(abs31), SRC1, T(neg30), T(abs34), T(is2) },
	{ 0x2300000000000002ull, 0x3fc0000000000003ull, T(minmax), T(ftz2f), N("f32"), DST, T(neg33), T(abs31), SRC1, T(neg30), T(abs34), T(is2) },
	{ 0x2340000000000002ull, 0x3fc0000000000003ull, N("mul"), T(ftz2f), T(sat35), T(frm2a), T(neg33), N("f32"), DST, SRC1, T(is2) },
	{ 0x2380000000000002ull, 0x3fc0000000000003ull, N("add"), T(frm2a), N("f64"), DSTD, T(neg33), T(abs31), SRC1D, T(neg30), T(abs34), T(ds2) },
	{ 0x2400000000000002ull, 0x3fc0000000000003ull, N("mul"), T(frm2a), T(neg33), N("f64"), DSTD, SRC1D, T(ds2) },
	{ 0x2480000000000002ull, 0x3fc0040000000003ull, N("presin"), N("f32"), DST, T(neg30), T(abs34), T(is2) },
	{ 0x2480040000000002ull, 0x3fc0040000000003ull, N("preex2"), N("f32"), DST, T(neg30), T(abs34), T(is2) },
	{ 0x24c0000000000002ull, 0x3fc0000000000003ull, T(lane2a), N("mov"), N("b32"), DST, T(is2) },
	{ 0x2500000000000002ull, 0x3fc0000000000003ull, N("selp"), N("b32"), DST, SRC1, T(is2), T(pnot2d), PSRC3 },
	{ 0x2540000000001402ull, 0x3fc0000000003c03ull, N("cvt"), T(sat35), T(rint), N("f16"), DST, N("f16"), T(neg30), T(abs34), T(is2) },
	{ 0x2540000000001802ull, 0x3fc0000000003c03ull, N("cvt"), T(ftz2f), T(sat35), N("f32"), DST, N("f16"), T(neg30), T(abs34), T(is2) },
	{ 0x2540000000002402ull, 0x3fc0000000003c03ull, N("cvt"), T(ftz2f), T(sat35), T(frm2a), N("f16"), DST, N("f32"), T(neg30), T(abs34), T(is2) },
	{ 0x2540000000002802ull, 0x3fc0000000003c03ull, N("cvt"), T(ftz2f), T(sat35), T(rint), N("f32"), DST, N("f32"), T(neg30), T(abs34), T(is2) },
	{ 0x2540000000002c02ull, 0x3fc0000000003c03ull, N("cvt"), N("f64"), DST, N("f32"), T(neg30), T(abs34), T(is2) }, // XXX: do ftz, sat work here ?
	{ 0x2540000000003802ull, 0x3fc0000000003c03ull, N("cvt"), T(frm2a), N("f32"), DST, N("f64"), T(neg30), T(abs34), T(is2) }, // XXX: do ftz, sat work here ?
	{ 0x2540000000003c02ull, 0x3fc0000000003c03ull, N("cvt"), T(rint), N("f64"), DST, N("f64"), T(neg30), T(abs34), T(ds2) },
	{ 0x2580000000000002ull, 0x3fc0000000000003ull, N("cvt"), T(ftz2f), T(frmi), T(cvtf2idst), T(neg30), T(abs34), T(cvtf2isrc) },
	{ 0x2600000000000002ull, 0x3fc0000000000003ull, N("cvt"), T(sat35), T(cvti2idst), T(neg30), T(abs34), T(cvti2isrc) },
	{ 0x25c0000000000002ull, 0x3fc0000000000003ull, N("cvt"), T(frm2a), T(cvti2fdst), T(neg30), T(abs34), T(cvti2fsrc) },
	{ 0x27c0000000000002ull, 0x3fc0000000000003ull, N("rshf"), N("b32"), DST, SESTART, T(us64_28), SRC1, SRC3, SEEND, T(shfclamp), T(is2) }, // XXX: check is2 and bits 0x29,0x33(swap srcs ?)
	{ 0x2800000000000002ull, 0x3980000000000003ull, N("mul"), DST, T(us32_39), SRC1, T(us32_3a), LIMM },    //XXX: wrong! 6c280000011c100a is atom-related
	{ 0x7400000000000002ull, 0x7f80000000000003ull, T(lane0e), N("mov"), N("b32"), DST, LIMM },
	{ 0x7600000000000002ull, 0x7fc0000000000003ull, N("texgrad"), T(texm), TDST, T(text), TCONST, T(texgrsrc1), T(texgrsrc2) },
	{ 0x7700000000000002ull, 0x7fc0000000000003ull, N("texbar"), TEXBARIMM },
	{ 0x7a00000000000002ull, 0x7fc0000000000003ull, N("ld"), T(lldstt), T(llcop), T(lldstd), LOCAL },
	{ 0x7a40000000000002ull, 0x7fc0000000000003ull, N("ld"), T(lldstt), T(lldstd), SHARED },
	{ 0x7a80000000000002ull, 0x7fc0000000000003ull, N("st"), T(lldstt), T(lscop), LOCAL, T(lldstd) },
	{ 0x7ac0000000000002ull, 0x7fc0000000000003ull, N("st"), T(lldstt), SHARED, T(lldstd) },
	{ 0x7c80000000000002ull, 0x7fc0000000000003ull, N("ld"), T(lldstt), T(lldstd), LCONST },
	{ 0x7d80000000000002ull, 0x7fc0000000000003ull, N("tex"), T(texm), T(lodt), TDST, T(text), N("ind"), T(texsrc1), T(texsrc2) },
	{ 0x7e00000000000002ull, 0x7fc0000000000003ull, N("texgrad"), T(texm), TDST, T(text), N("ind"), T(texgrsrc1), T(texgrsrc2) },
	{ 0x0540000000000002ull, 0x3fc0000000000002ull, N("bar"), BAR, OOPS},
	{ 0x0, 0x0, DST, SRC1, SRC2, SRC3, OOPS },
};

static struct insn tabi[] = {
	{ 0x4000000000000001ull, 0xf580000000000003ull, T(addop3b), T(sat39), N("b32"), DST, SRC1, LIMM },
	{ 0x8200000000000001ull, 0xf780000000000003ull, N("set"), T(ftz3a), N("b32"), DST, T(setit), N("f32"), T(neg2e), T(abs39), SRC1, T(neg3b), FIMM, T(setlop) }, // XXX: find f32 dst
	{ 0x0040000000000001ull, 0x37c0000000000003ull, N("popc"), N("b32"), DST, T(not2a), SRC1, T(i3bi2) },
	{ 0x0080000000000001ull, 0x37c0000000000003ull, T(addop), T(sat35), N("b32"), DST, T(acout32), SRC1, T(i3bi2) },
	{ 0x00c0000000000001ull, 0x37c0000000000003ull, T(addop), N("b32"), DST, N("shl"), SRC1, SHCNT, T(i3bi2)},
	{ 0x0100000000000001ull, 0x37c0000000000003ull, T(minmax), T(us32_33), DST, SRC1, T(i3bi2) },
	{ 0x0148000000000001ull, 0x37c8000000000003ull, N("shr"), N("s32"), DST, SRC1, T(sui2b) }, // XXX: find shl and wrap bits
	{ 0x01c0000000000001ull, 0x37c0000000000003ull, N("mul"), T(high2a), DST, T(us32_2b), SRC1, T(us32_2c), T(i3bi2) }, // XXX: order of us32, TODO: find LIMM form
	{ 0x0200000000000001ull, 0x37c0000000000003ull, T(logop), N("b32"), DST, SRC1, T(i3bi2) },
	{ 0x0280000000000001ull, 0x37c0000000000003ull, T(minmax), N("f64"), DST, T(neg33), T(abs31), SRC1, T(neg3b), T(di2) },
	{ 0x02c0000000000001ull, 0x37c0000000000003ull, N("add"), T(ftz2f), T(sat35), T(frm2a), N("f32"), DST, T(neg33), T(abs31), SRC1, T(neg3b), T(fi2) },
	{ 0x0300000000000001ull, 0x37c0000000000003ull, T(minmax), T(ftz2f), N("f32"), DST, T(neg33), T(abs31), SRC1, T(neg3b), T(fi2) },
	{ 0x0340000000000001ull, 0x37c0000000000003ull, N("mul"), T(ftz2f), T(sat35), T(frm2a), T(neg3b), N("f32"), DST, SRC1, T(fi2) },
	{ 0x0400000000000001ull, 0x37c0000000000003ull, N("mul"), T(frm2a), T(neg3b), N("f64"), DSTD, SRC1D, T(di2) },
	{ 0x0500000000000001ull, 0x37c0000000000003ull, N("selp"), DST, SRC1, T(i3bi2), T(pnot2d), PSRC3 },
	{ 0x07c0000000000001ull, 0x37c0000000000003ull, N("rshf"), N("b32"), DST, SESTART, T(us64_28), SRC1, SRC3, SEEND, T(shfclamp), T(sui2b) }, // d = (s1 >> s2) | (s3 << (32 - s2))
	{ 0x9000000000000001ull, 0xb580000000000003ull, N("set"), N("b32"), DST, T(setit), N("f64"), T(neg2e), T(abs39), SRC1D, T(neg3b), DIMM, T(setlop) },
	{ 0x9400000000000001ull, 0xb6c0000000000003ull, N("fma"), T(ftz38), T(sat35), T(frm36), N("f32"), DST, T(neg3b), SRC1, FIMM, T(neg34), SRC3 }, // TODO: find LIMM form
	{ 0xa000000000000001ull, 0xb400000000000003ull, T(addop3a), T(sat39), DST, SESTART, N("mul"), T(us32_33), SRC1, T(us32_38), I3BIMM, SEEND, SRC3 },
	{ 0xb200000000000001ull, 0xb780000000000003ull, N("slct"), N("b32"), DST, SRC1, I3BIMM, T(isetit), T(us32_33), SRC3 }, // XXX: check us32_33
	{ 0xb280000000000001ull, 0xb780000000000003ull, N("set"), N("b32"), DST, T(isetit), T(us32_33), SRC1, I3BIMM, T(setlop) },
	{ 0xb300000000000001ull, 0xb780000000000003ull, N("set"), N("b32"), PDST, PDSTN, T(isetit), T(us32_33), SRC1, I3BIMM, T(setlop) },
	{ 0xb400000000000001ull, 0xb780000000000003ull, N("set"), PDST, PDSTN, T(setit), N("f64"), T(neg2e), T(abs9), SRC1D, T(neg3b), DIMM, T(setlop) },
	{ 0xb500000000000001ull, 0xb780000000000003ull, N("slct"), T(ftz32), N("b32"), DST, SRC1, T(neg3b), FIMM, T(setit), N("f32"), SRC3 },
	{ 0xb580000000000001ull, 0xb780000000000003ull, N("set"), T(ftz32), PDST, PDSTN, T(setit), N("f32"), T(neg2e), T(abs9), SRC1, T(neg3b), FIMM, T(setlop) },
	{ 0xb740000000000001ull, 0xb7c0000000000003ull, N("sad"), T(us32_33), DST, SRC1, I3BIMM, SRC3 },
	{ 0xb780000000000001ull, 0xb7c0000000000003ull, N("ins"), N("b32"), DST, SRC1, I3BIMM, SRC3 },
	{ 0x2000000000000001ull, 0x3fc0000000000003ull, N("tex"), T(texm), T(lodt), TDST, T(text), T(tconst), T(texsrc1), T(texsrc2) },
	{ 0x37c0000000000001ull, 0x37c0000000000003ull, N("lshf"), N("b32"), DST, SESTART, N("b64"), SRC1, SRC3, SEEND, T(shfclamp), T(sui2a) }, // d = (s3 << s2) | (s1 >> (32 - s2))
	{ 0x0, 0x0, DST, SRC1, SRC2, SRC3, OOPS },
};

static struct insn tabp[] = {
	{ 0x001c0000, 0x003c0000 },
	{ 0x003c0000, 0x003c0000, N("never") },
	{ 0x00000000, 0x00200000, PRED },
	{ 0x00200000, 0x00200000, SESTART, N("not"), PRED, SEEND },
	{ 0, 0, OOPS },
};

static struct insn tabc[] = {
	{ 0x0800000000000000ull, 0xfc00000000000000ull, N("sched"), SCHED },
	{ 0x1200000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("bra"), BTARG },
	{ 0x1300000000000000ull, 0xff80000000000000ull, N("call"), CTARG },
	{ 0x1480000000000000ull, 0xff80000000000000ull, N("joinat"), BTARG },
	{ 0x1800000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("exit") },
	{ 0x1900000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("ret") },

	{ 0x2000000000000000ull, 0xfc80000000000000ull, T(p), T(logop38), N("b32"), DST, SRC1, LIMM },
	{ 0x4000000000000000ull, 0xf180000000000000ull, T(p), N("add"), T(ftz3a), N("f32"), DST, T(neg3b), T(abs39), SRC1, LIMM },
	{ 0xa000000000000000ull, 0xe000000000000000ull, T(p), N("add"), N("b32"), DST, N("shl"), SRC1, SHCNL, LIMM},

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
