/*
 * Copyright (C) 2009-2011 Marcelina Kościelnicka <mwk@0x04.net>
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

/*
 * $r255: bit bucket on write, 0 on read
 */

/*
 * Code target field
 */
static struct rbitfield ctargoff = { { 23, 24 }, RBF_SIGNED, .pcrel = 1, .addend = 8 };
#define BTARG atomctarg, &ctargoff
#define CTARG atomctarg, &ctargoff

static struct rbitfield bptargoff = { { 23, 20 } };
#define BPTARG atomctarg, &bptargoff

static struct rbitfield actargoff = { { 23, 32 } };
#define ACTARG atomctarg, &actargoff


/*
 * Misc number fields
 */
static struct rbitfield i3bimmoff = { { 0x17, 19, 0x3b, 1 }, RBF_SIGNED };
static struct rbitfield shfimmoff = { { 0x17, 6 }, RBF_UNSIGNED }; // XXX: for r/lshf, check size
static struct rbitfield suimmoff = { { 0x2a, 6 }, RBF_SIGNED };
static struct rbitfield shcntsoff = { { 0x2a, 5 }, RBF_UNSIGNED };
static struct rbitfield shcnlsoff = { { 0x38, 5 }, RBF_UNSIGNED };
static struct rbitfield fimmoff = { { 0x17, 19, 0x3b, 1 }, .shr = 12 };
static struct rbitfield limmoff = { { 0x17, 32 }, .wrapok = 1 };
static struct rbitfield dimmoff = { { 0x17, 19, 0x3b, 1 }, .shr = 44 };
static struct rbitfield sflnimmoff = { { 0x17, 5 }, RBF_UNSIGNED };
static struct rbitfield sflmimmoff = { { 0x25, 13 }, RBF_UNSIGNED };
static struct rbitfield barimm1off = { { 0xa, 8 }, RBF_UNSIGNED };
static struct rbitfield barimm2off = { { 0x17, 12 }, RBF_UNSIGNED };
static struct rbitfield vimmoff = { { 0x17, 16 }, RBF_UNSIGNED };
#define SUIMM atomrimm, &suimmoff
#define SHFIMM atomrimm, &shfimmoff
#define SHCNT atomrimm, &shcntsoff
#define SHCNL atomrimm, &shcnlsoff
#define I3BIMM atomrimm, &i3bimmoff
#define FIMM atomrimm, &fimmoff
#define DIMM atomrimm, &dimmoff
#define LIMM atomrimm, &limmoff
#define SFLNIMM atomrimm, &sflnimmoff
#define SFLMIMM atomrimm, &sflmimmoff
#define BARIMM1 atomrimm, &barimm1off
#define BARIMM2 atomrimm, &barimm2off
#define VIMM atomrimm, &vimmoff

static struct bitfield texbaroff = { 0x17, 6 }; // XXX: check exact size
static struct rbitfield texsoff = { { 0x17, 11 }, RBF_SIGNED };
static struct bitfield schedval0 = { 0x02, 8 };
static struct bitfield schedval1 = { 0x0a, 8 };
static struct bitfield schedval2 = { 0x12, 8 };
static struct bitfield schedval3 = { 0x1a, 8 };
static struct bitfield schedval4 = { 0x22, 8 };
static struct bitfield schedval5 = { 0x2a, 8 };
static struct bitfield schedval6 = { 0x32, 8 };
#define TEXBARIMM atomimm, &texbaroff
#define TEXSIMM atomimm, &texsoff
#define SCHED(n) atomimm, &schedval##n


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
	{ 0x13, "thread_kill" },
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
	{ 0x50, "clocklo" },
	{ 0x51, "clockhi" },
	{ 0x52, "globaltimerlo" },
	{ 0x53, "globaltimerhi" },
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
static struct bitfield pdstl_bf = { 0x30, 3 };
static struct bitfield pdstn_bf = { 0x2, 3 };
static struct bitfield pdst2_bf = { 0x33, 3 };
static struct bitfield src1_bf = { 0xa, 8 };
static struct bitfield src2_bf = { 0x17, 8 };
static struct bitfield src2p1_bf = { 0x17, 8, .addend = 1 };
static struct bitfield src2p2_bf = { 0x17, 8, .addend = 2 };
static struct bitfield src3_bf = { 0x2a, 8 };
static struct bitfield pred_bf = { 0x12, 3 };
static struct bitfield psrc1_bf = { 0xe, 3 };
static struct bitfield psrc2_bf = { 0x20, 3 };
static struct bitfield psrc3_bf = { 0x2a, 3 };
static struct bitfield psrc4_bf = { 0x30, 3 };
static struct bitfield psrc5_bf = { 0x32, 3 };
static struct bitfield sreg_bf = { 0x17, 8 };

static struct reg dst_r = { &dst_bf, "r", .specials = reg_sr };
static struct reg dstd_r = { &dst_bf, "r", "d", .specials = reg_sr };
static struct reg dstt_r = { &dst_bf, "r", "t", .specials = reg_sr };
static struct reg dstq_r = { &dst_bf, "r", "q" };
static struct reg src1_r = { &src1_bf, "r", .specials = reg_sr };
static struct reg src1d_r = { &src1_bf, "r", "d", .specials = reg_sr };
static struct reg src2_r = { &src2_bf, "r", .specials = reg_sr };
static struct reg src2d_r = { &src2_bf, "r", "d", .specials = reg_sr };
static struct reg src2q_r = { &src2_bf, "r", "q", .specials = reg_sr };
static struct reg src2p1_r = { &src2p1_bf, "r", .specials = reg_sr };
static struct reg src2dp1_r = { &src2p2_bf, "r", "d", .specials = reg_sr };
static struct reg src3_r = { &src3_bf, "r", .specials = reg_sr };
static struct reg src3d_r = { &src3_bf, "r", "d", .specials = reg_sr };
static struct reg psrc1_r = { &psrc1_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg psrc2_r = { &psrc2_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg psrc3_r = { &psrc3_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg psrc4_r = { &psrc4_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg psrc5_r = { &psrc5_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdst_r = { &pdst_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdstl_r = { &pdstl_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdstn_r = { &pdstn_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdst2_r = { &pdst2_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pred_r = { &pred_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg cc_r = { 0, "c", .cool = 1 };
static struct reg sreg_r = { &sreg_bf, "sr", .specials = sreg_sr, .always_special = 1 };

#define DST atomreg, &dst_r
#define DSTD atomreg, &dstd_r
#define DSTT atomreg, &dstt_r
#define DSTQ atomreg, &dstq_r
#define PDST atomreg, &pdst_r
#define PDSTL atomreg, &pdstl_r
#define PDSTN atomreg, &pdstn_r
#define PDST2 atomreg, &pdst2_r
#define PRED atomreg, &pred_r
#define SRC1 atomreg, &src1_r
#define SRC1D atomreg, &src1d_r
#define SRC2 atomreg, &src2_r
#define SRC2D atomreg, &src2d_r
#define SRC2Q atomreg, &src2q_r
#define SRC2P1 atomreg, &src2p1_r
#define SRC2DP1 atomreg, &src2dp1_r
#define SRC3 atomreg, &src3_r
#define SRC3D atomreg, &src3d_r
#define PSRC1 atomreg, &psrc1_r
#define PSRC2 atomreg, &psrc2_r
#define PSRC3 atomreg, &psrc3_r
#define PSRC4 atomreg, &psrc4_r
#define PSRC5 atomreg, &psrc5_r
#define CC atomreg, &cc_r
#define SREG atomreg, &sreg_r

static struct bitfield tdst_mask = { 0x22, 4 };
static struct bitfield cnt1 = { .addend = 1 };
static struct bitfield cnt2 = { .addend = 2 };
static struct bitfield cnt3 = { .addend = 3 };
static struct bitfield cnt4 = { .addend = 4 };

static struct vec tdst_v = { "r", &dst_bf, &cnt4, &tdst_mask };
static struct vec tsrc11_v = { "r", &src1_bf, &cnt1, 0 };
static struct vec tsrc12_v = { "r", &src1_bf, &cnt2, 0 };
static struct vec tsrc13_v = { "r", &src1_bf, &cnt3, 0 };
static struct vec tsrc14_v = { "r", &src1_bf, &cnt4, 0 };
static struct vec tsrc21_v = { "r", &src2_bf, &cnt1, 0 };
static struct vec tsrc22_v = { "r", &src2_bf, &cnt2, 0 };
static struct vec tsrc23_v = { "r", &src2_bf, &cnt3, 0 };
static struct vec tsrc24_v = { "r", &src2_bf, &cnt4, 0 };

#define TDST atomvec, &tdst_v
#define TSRC11 atomvec, &tsrc11_v
#define TSRC12 atomvec, &tsrc12_v
#define TSRC13 atomvec, &tsrc13_v
#define TSRC14 atomvec, &tsrc14_v
#define TSRC21 atomvec, &tsrc21_v
#define TSRC22 atomvec, &tsrc22_v
#define TSRC23 atomvec, &tsrc23_v
#define TSRC24 atomvec, &tsrc24_v

static struct bitfield ssrc_mask = { 0x19, 4 };
static struct bitfield ssrc_mask1 = { 0x4, 4 };
static struct bitfield scnt4 = { .addend = 4 };

static struct vec ssrc3_v = { "r", &src3_bf, &scnt4, &ssrc_mask };
static struct vec ssrc3_v1 = { "r", &src3_bf, &scnt4, &ssrc_mask1 };

#define SUSTPSRC atomvec, &ssrc3_v
#define SUSTPSRC1 atomvec, &ssrc3_v1

/*
 * Memory fields
 */

static struct rbitfield amem_imm = { { 0x17, 10 }, RBF_UNSIGNED };
static struct rbitfield iamem_imm = { { 0x1f, 10 }, RBF_UNSIGNED };
static struct rbitfield gmem_imm = { { 0x17, 32 }, RBF_SIGNED };
static struct rbitfield gamem_imm = { { 0x1f, 20 }, RBF_SIGNED };
static struct rbitfield cmem_imm = { { 0x17, 14 }, RBF_UNSIGNED, .shr = 2 };
static struct rbitfield lcmem_imm = { { 0x17, 16 }, RBF_SIGNED };
static struct rbitfield lmem_imm = { { 0x17, 24 }, RBF_SIGNED };
static struct rbitfield smem_imm = { { 0x17, 24 }, RBF_SIGNED };
static struct rbitfield tcmem_imm = { { 0x2f, 8 }, .shr = 2 }; // XXX: could be 13 bits
static struct rbitfield fcmem_imm = { { 0x17, 8 }, RBF_SIGNED };
static struct bitfield cmem_idx = { 0x25, 5 };
static struct bitfield lcmem_idx = { 0x27, 5 };

static struct mem amem_m = { "a", 0, 0, &amem_imm };
static struct mem iamem_m = { "a", 0, 0, &iamem_imm };
static struct mem gmem_m = { "g", 0, &src1_r, &gmem_imm };
static struct mem gamem_m = { "g", 0, &src1_r, &gamem_imm };
static struct mem lmem_m = { "l", 0, &src1_r, &lmem_imm };
static struct mem smem_m = { "s", 0, &src1_r, &smem_imm };
static struct mem gdmem_m = { "g", 0, &src1d_r, &gmem_imm };
static struct mem gadmem_m = { "g", 0, &src1d_r, &gamem_imm };
static struct mem gdmemsu_m = { "g", 0, &src1d_r };
static struct mem cmem_m = { "c", &cmem_idx, 0, &cmem_imm };
static struct mem lcmem_m = { "c", &lcmem_idx, &src1_r, &lcmem_imm };
static struct mem tcmem_m = { "c", 0, 0, &tcmem_imm };
static struct mem vba_m = { "p", 0, &src1_r, &fcmem_imm };

#define ATTR atommem, &amem_m
#define IATTR atommem, &iamem_m
#define GLOBAL atommem, &gmem_m
#define GLOBALD atommem, &gdmem_m
#define GLOBALDSU atommem, &gdmemsu_m
#define GATOM atommem, &gamem_m
#define GATOMD atommem, &gadmem_m
#define LOCAL atommem, &lmem_m
#define SHARED atommem, &smem_m
#define CONST atommem, &cmem_m
#define LCONST atommem, &lcmem_m
#define TCONST atommem, &tcmem_m
#define VBA atommem, &vba_m


/*
 * The instructions
 */

F(gmem, 0x37, GLOBAL, GLOBALD)
F(gamem, 0x33, GATOM, GATOMD)

F1(pnot2d, 0x2d, N("not"))
F1(pnot23, 0x23, N("not"))
F1(pnot11, 0x11, N("not"))
F(shclamp, 0x2a, N("clamp"), N("wrap"))

static struct insn tabfrmi[] = {
	{ 0x0000000000000000ull, 0x00000c0000000000ull, N("rni") },
	{ 0x0000040000000000ull, 0x00000c0000000000ull, N("rmi") },
	{ 0x0000080000000000ull, 0x00000c0000000000ull, N("rpi") },
	{ 0x00000c0000000000ull, 0x00000c0000000000ull, N("rzi") },
	{ 0, 0, OOPS },
};

static struct insn tabfrm2a[] = {
	{ 0x0000000000000000ull, 0x00000c0000000000ull, N("rn") },
	{ 0x0000040000000000ull, 0x00000c0000000000ull, N("rm") },
	{ 0x0000080000000000ull, 0x00000c0000000000ull, N("rp") },
	{ 0x00000c0000000000ull, 0x00000c0000000000ull, N("rz") },
	{ 0, 0, OOPS },
};
static struct insn tabfrm35[] = {
	{ 0x0000000000000000ull, 0x0060000000000000ull, N("rn") },
	{ 0x0020000000000000ull, 0x0060000000000000ull, N("rm") },
	{ 0x0040000000000000ull, 0x0060000000000000ull, N("rp") },
	{ 0x0060000000000000ull, 0x0060000000000000ull, N("rz") },
	{ 0, 0, OOPS },
};
static struct insn tabfrm36[] = {
	{ 0x0000000000000000ull, 0x00c0000000000000ull, N("rn") },
	{ 0x0040000000000000ull, 0x00c0000000000000ull, N("rm") },
	{ 0x0080000000000000ull, 0x00c0000000000000ull, N("rp") },
	{ 0x00c0000000000000ull, 0x00c0000000000000ull, N("rz") },
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
static struct insn tabsetlop3[] = {
	{ 0x00001c0000000000ull, 0x00031c0000000000ull }, // noop, really "and $p7"
	{ 0x0000000000000000ull, 0x0003000000000000ull, N("and"), T(pnot2d), PSRC3 },
	{ 0x0001000000000000ull, 0x0003000000000000ull, N("or"), T(pnot2d), PSRC3 },
	{ 0x0002000000000000ull, 0x0003000000000000ull, N("xor"), T(pnot2d), PSRC3 },
	{ 0, 0, OOPS, T(pnot2d), PSRC3 },
};
static struct insn tabsetlop2[] = {
	{ 0x0000000700000000ull, 0x0000000718000000ull }, // noop, really "and $p7"
	{ 0x0000000000000000ull, 0x0000000018000000ull, N("and"), T(pnot23), PSRC2 },
	{ 0x0000000008000000ull, 0x0000000018000000ull, N("or"), T(pnot23), PSRC2 },
	{ 0x0000000010000000ull, 0x0000000018000000ull, N("xor"), T(pnot23), PSRC2 },
	{ 0, 0, OOPS, T(pnot23), PSRC2 },
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

static struct insn tabcc2[] = {
	{ 0x0000000000000000ull, 0x0000000000007c00ull, N("never"), CC },
	{ 0x0000000000000400ull, 0x0000000000007c00ull, N("l"), CC },
	{ 0x0000000000000800ull, 0x0000000000007c00ull, N("e"), CC },
	{ 0x0000000000000c00ull, 0x0000000000007c00ull, N("le"), CC },
	{ 0x0000000000001000ull, 0x0000000000007c00ull, N("g"), CC },
	{ 0x0000000000001400ull, 0x0000000000007c00ull, N("lg"), CC },
	{ 0x0000000000001800ull, 0x0000000000007c00ull, N("ge"), CC },
	{ 0x0000000000001c00ull, 0x0000000000007c00ull, N("lge"), CC },
	{ 0x0000000000002000ull, 0x0000000000007c00ull, N("u"), CC },
	{ 0x0000000000002400ull, 0x0000000000007c00ull, N("lu"), CC },
	{ 0x0000000000002800ull, 0x0000000000007c00ull, N("eu"), CC },
	{ 0x0000000000002c00ull, 0x0000000000007c00ull, N("leu"), CC },
	{ 0x0000000000003000ull, 0x0000000000007c00ull, N("gu"), CC },
	{ 0x0000000000003400ull, 0x0000000000007c00ull, N("lgu"), CC },
	{ 0x0000000000003800ull, 0x0000000000007c00ull, N("geu"), CC },
	{ 0x0000000000003c00ull, 0x0000000000007c00ull, },
	{ 0x0000000000004000ull, 0x0000000000007c00ull, N("no"), CC },
	{ 0x0000000000004400ull, 0x0000000000007c00ull, N("nc"), CC },
	{ 0x0000000000004800ull, 0x0000000000007c00ull, N("ns"), CC },
	{ 0x0000000000004c00ull, 0x0000000000007c00ull, N("na"), CC },
	{ 0x0000000000005000ull, 0x0000000000007c00ull, N("a"), CC },
	{ 0x0000000000005400ull, 0x0000000000007c00ull, N("s"), CC },
	{ 0x0000000000005800ull, 0x0000000000007c00ull, N("c"), CC },
	{ 0x0000000000005c00ull, 0x0000000000007c00ull, N("o"), CC },
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
	{ 0x8000000000000000ull, 0xc000000000000000ull, SHFIMM },
	{ 0, 0, OOPS },
};
static struct insn tabsui2b[] = {
	{ 0xc000000000000000ull, 0xc000000000000000ull, SHFIMM },
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

static struct insn tabinterpmode[] = {
	{ 0x0000000000000000ull, 0x0060000000000000ull, N("pass") },
	{ 0x0020000000000000ull, 0x0060000000000000ull, N("mul") },
	{ 0x0040000000000000ull, 0x0060000000000000ull, N("flat") },
	{ 0x0060000000000000ull, 0x0060000000000000ull, N("sc") },
	{ 0, 0, OOPS },
};

static struct insn tabtexsrc1[] = { // XXX: find shadow and offset bits
	// target
	{ 0x0000000000000000ull, 0x100021c000000000ull, N("x___"), TSRC11 },
	{ 0x0000004000000000ull, 0x100021c000000000ull, N("ax__"), TSRC12 },
	{ 0x0000008000000000ull, 0x100021c000000000ull, N("xy__"), TSRC12 },
	{ 0x000000c000000000ull, 0x100021c000000000ull, N("axy_"), TSRC13 },
	{ 0x0000010000000000ull, 0x1000214000000000ull, N("xyz_"), TSRC13 },
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// target + lod/bias
	{ 0x0000200000000000ull, 0x100021c000000000ull, N("xl__"), TSRC12 },
	{ 0x0000204000000000ull, 0x100021c000000000ull, N("axl_"), TSRC13 },
	{ 0x0000208000000000ull, 0x100021c000000000ull, N("xyl_"), TSRC13 },
	{ 0x000020c000000000ull, 0x100021c000000000ull, N("axyl"), TSRC14 },
	{ 0x0000210000000000ull, 0x1000214000000000ull, N("xyzl"), TSRC14 },
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// target + lod/bias + dc
	{ 0x0000200000000000ull, 0x100021c000000000ull, N("xld_"), TSRC13 },
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
	{ 0x0000200000000000ull, 0x100021c000000000ull, N("xlo_"), TSRC13 },
	{ 0x0000204000000000ull, 0x100021c000000000ull, N("axlo"), TSRC14 },
	{ 0x0000208000000000ull, 0x100021c000000000ull, N("xylo"), TSRC14 },
	{ 0x000020c000000000ull, 0x100021c000000000ull, N("axyl"), TSRC14 },
	{ 0x0000210000000000ull, 0x1000214000000000ull, N("xyzl"), TSRC14 },
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// target + dc
	{ 0x0000000000000000ull, 0x100021c000000000ull, N("xd__"), TSRC12 },
	{ 0x0000004000000000ull, 0x100021c000000000ull, N("axd_"), TSRC13 },
	{ 0x0000008000000000ull, 0x100021c000000000ull, N("xyd_"), TSRC13 },
	{ 0x000000c000000000ull, 0x100021c000000000ull, N("axyd"), TSRC14 },
	{ 0x0000010000000000ull, 0x1000214000000000ull, N("xyzd"), TSRC14 },
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// target + dc + offset
	{ 0x0000000000000000ull, 0x100021c000000000ull, N("xod_"), TSRC13 },
	{ 0x0000004000000000ull, 0x100021c000000000ull, N("axod"), TSRC14 },
	{ 0x0000008000000000ull, 0x100021c000000000ull, N("xyod"), TSRC14 },
	{ 0x000000c000000000ull, 0x100021c000000000ull, N("axyo"), TSRC14 },
	{ 0x0000010000000000ull, 0x1000214000000000ull, N("xyzo"), TSRC14 },
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// target + offset
	{ 0x0000000000000000ull, 0x100021c000000000ull, N("xo__"), TSRC12 },
	{ 0x0000004000000000ull, 0x100021c000000000ull, N("axo_"), TSRC13 },
	{ 0x0000008000000000ull, 0x100021c000000000ull, N("xyo_"), TSRC13 },
	{ 0x000000c000000000ull, 0x100021c000000000ull, N("axyo"), TSRC14 },
	{ 0x0000010000000000ull, 0x1000214000000000ull, N("xyzo"), TSRC14 },
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("axyz"), TSRC14 },
	// ind + target
	{ 0x1000000000000000ull, 0x100021c000000000ull, N("ix__"), TSRC12 },
	{ 0x1000004000000000ull, 0x100021c000000000ull, N("iax_"), TSRC13 },
	{ 0x1000008000000000ull, 0x100021c000000000ull, N("ixy_"), TSRC13 },
	{ 0x100000c000000000ull, 0x100021c000000000ull, N("iaxy"), TSRC14 },
	{ 0x1000010000000000ull, 0x1000214000000000ull, N("ixyz"), TSRC14 },
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("iaxy"), TSRC14 },
	// ind + target + lod/bias
	{ 0x1000200000000000ull, 0x100021c000000000ull, N("ixl_"), TSRC13 },
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
	{ 0x1000000000000000ull, 0x100021c000000000ull, N("ixd_"), TSRC13 },
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
	{ 0x1000000000000000ull, 0x100021c000000000ull, N("ixo_"), TSRC13 },
	{ 0x1000004000000000ull, 0x100021c000000000ull, N("iaxo"), TSRC14 },
	{ 0x1000008000000000ull, 0x100021c000000000ull, N("ixyo"), TSRC14 },
	{ 0x100000c000000000ull, 0x100021c000000000ull, N("iaxy"), TSRC14 },
	{ 0x1000010000000000ull, 0x1000214000000000ull, N("ixyz"), TSRC14 },
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("iaxy"), TSRC14 },
};
static struct insn tabtexsrc2[] = {
	// target + lod/bias
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("l___"), TSRC21 },
	// target + lod/bias + dc
	{ 0x000020c000000000ull, 0x100021c000000000ull, N("d___"), TSRC21 },
	{ 0x0000210000000000ull, 0x1000214000000000ull, N("d___"), TSRC21 },
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("ld__"), TSRC22 },
	// target + lod/bias + dc + offset
	{ 0x0000204000000000ull, 0x100021c000000000ull, N("d___"), TSRC21 },
	{ 0x0000208000000000ull, 0x100021c000000000ull, N("d___"), TSRC21 },
	{ 0x000020c000000000ull, 0x100021c000000000ull, N("od__"), TSRC22 },
	{ 0x0000210000000000ull, 0x1000214000000000ull, N("od__"), TSRC22 },
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("lod_"), TSRC23 },
	// target + lod/bias + offset
	{ 0x000020c000000000ull, 0x100021c000000000ull, N("o___"), TSRC21 },
	{ 0x0000210000000000ull, 0x1000214000000000ull, N("o___"), TSRC21 },
	{ 0x0000214000000000ull, 0x1000214000000000ull, N("lo__"), TSRC22 },
	// target + dc
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("d___"), TSRC21 },
	// target + dc + offset
	{ 0x000000c000000000ull, 0x100021c000000000ull, N("d___"), TSRC21 },
	{ 0x0000010000000000ull, 0x1000214000000000ull, N("d___"), TSRC21 },
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("od__"), TSRC22 },
	// target + offset
	{ 0x0000014000000000ull, 0x1000214000000000ull, N("o___"), TSRC21 },
	// ind + target
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("z___"), TSRC21 },
	// ind + target + lod/bias
	{ 0x100020c000000000ull, 0x100021c000000000ull, N("l___"), TSRC21 },
	{ 0x1000210000000000ull, 0x1000214000000000ull, N("l___"), TSRC21 },
	{ 0x1000214000000000ull, 0x1000214000000000ull, N("zl__"), TSRC22 },
	// ind + target + lod/bias + dc
	{ 0x1000204000000000ull, 0x100021c000000000ull, N("d___"), TSRC21 },
	{ 0x1000208000000000ull, 0x100021c000000000ull, N("d___"), TSRC21 },
	{ 0x100020c000000000ull, 0x100021c000000000ull, N("ld__"), TSRC22 },
	{ 0x1000210000000000ull, 0x1000214000000000ull, N("ld__"), TSRC22 },
	{ 0x1000214000000000ull, 0x1000214000000000ull, N("zld_"), TSRC23 },
	// ind + target + lod/bias + dc + offset
	{ 0x1000200000000000ull, 0x100021c000000000ull, N("d___"), TSRC21 },
	{ 0x1000204000000000ull, 0x100021c000000000ull, N("od__"), TSRC22 },
	{ 0x1000208000000000ull, 0x100021c000000000ull, N("od__"), TSRC22 },
	{ 0x100020c000000000ull, 0x100021c000000000ull, N("lod_"), TSRC23 },
	{ 0x1000210000000000ull, 0x1000214000000000ull, N("lod_"), TSRC23 },
	{ 0x1000214000000000ull, 0x1000214000000000ull, N("zlod"), TSRC24 },
	// ind + target + lod/bias + offset
	{ 0x1000204000000000ull, 0x100021c000000000ull, N("o___"), TSRC21 },
	{ 0x1000208000000000ull, 0x100021c000000000ull, N("o___"), TSRC21 },
	{ 0x100020c000000000ull, 0x100021c000000000ull, N("lo__"), TSRC22 },
	{ 0x1000210000000000ull, 0x1000214000000000ull, N("lo__"), TSRC22 },
	{ 0x1000214000000000ull, 0x1000214000000000ull, N("zlo_"), TSRC23 },
	// ind + target + dc
	{ 0x100000c000000000ull, 0x100021c000000000ull, N("d___"), TSRC21 },
	{ 0x1000010000000000ull, 0x1000214000000000ull, N("d___"), TSRC21 },
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("zd__"), TSRC22 },
	// ind + target + dc + offset
	{ 0x1000004000000000ull, 0x100021c000000000ull, N("d___"), TSRC21 },
	{ 0x1000008000000000ull, 0x100021c000000000ull, N("d___"), TSRC21 },
	{ 0x100000c000000000ull, 0x100021c000000000ull, N("od__"), TSRC22 },
	{ 0x1000010000000000ull, 0x1000214000000000ull, N("od__"), TSRC22 },
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("zod_"), TSRC23 },
	// ind + target + offset
	{ 0x100000c000000000ull, 0x100021c000000000ull, N("o___"), TSRC21 },
	{ 0x1000010000000000ull, 0x1000214000000000ull, N("o___"), TSRC21 },
	{ 0x1000014000000000ull, 0x1000214000000000ull, N("zo__"), TSRC22 },
   // rest
	{ 0, 0, SRC2 },
};
static struct insn tabtexgrsrc1[] = {
	{ 0x0000000000000000ull, 0x080001c000000000ull, N("xgg_"), TSRC13 },
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
	{ 0x0000008000000000ull, 0x080001c000000000ull, N("gg__"), TSRC22 },
	{ 0x000000c000000000ull, 0x080001c000000000ull, N("gg__"), TSRC22 },
	{ 0x0800004000000000ull, 0x080001c000000000ull, N("g___"), TSRC21 },
	{ 0x0800008000000000ull, 0x080001c000000000ull, N("ggg_"), TSRC23 },
	{ 0x080000c000000000ull, 0x080001c000000000ull, N("gggg"), TSRC24 },
	{ 0, 0, SRC2 },
	{ 0, 0, OOPS },
};
F(ltex, 31, N("all"), N("live"))
F(dtex, 41, N("dfp"), N("dall"))
F1(texms, 0x2b, N("ms"))
F(lodf, 44, N("lzero"), N("llod"))
F1(texoff, 43, N("off"))
F1(texoff2, 41, N("off"))

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
static struct insn tabtexquery[] = {
	{ 0x0000000002000000ull, 0x000000007e000000ull, N("tdims") },
	{ 0x0000000004000000ull, 0x000000007e000000ull, N("ttype") },
	{ 0x000000000a000000ull, 0x000000007e000000ull, N("tsamplepos") },
	{ 0x0000000020000000ull, 0x000000007e000000ull, N("sfilter") },
	{ 0x0000000024000000ull, 0x000000007e000000ull, N("slod") },
	{ 0x0000000028000000ull, 0x000000007e000000ull, N("swrap") },
	{ 0x000000002c000000ull, 0x000000007e000000ull, N("sbcolor") },
	{ 0, 0, OOPS },
};
static struct insn tabtexf[] = {
	{ 0, 0, T(ltex), T(dtex) },
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
static struct insn tabcvtf2isrci[] = {
	{ 0x0000000000001000ull, 0x0000000000003000ull, N("f16"), I3BIMM },
	{ 0x0000000000002000ull, 0x0000000000003000ull, N("f32"), FIMM },
	{ 0x0000000000003000ull, 0x0000000000003000ull, N("f64"), DIMM },
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
static struct insn tabcvti2fsrci[] = {
	{ 0x0000000000000000ull, 0x000000000000b000ull, N("u8"), I3BIMM },
	{ 0x0000000000008000ull, 0x000000000000b000ull, N("s8"), I3BIMM },
	{ 0x0000000000001000ull, 0x000000000000b000ull, N("u16"), I3BIMM },
	{ 0x0000000000009000ull, 0x000000000000b000ull, N("s16"), I3BIMM },
	{ 0x0000000000002000ull, 0x000000000000b000ull, N("u32"), I3BIMM },
	{ 0x000000000000a000ull, 0x000000000000b000ull, N("s32"), I3BIMM },
	{ 0x0000000000003000ull, 0x000000000000b000ull, N("u64"), I3BIMM },
	{ 0x000000000000b000ull, 0x000000000000b000ull, N("s64"), I3BIMM },
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
static struct insn tabcvti2isrci[] = {
	{ 0x0000000000000000ull, 0x000000000000b000ull, N("u8"), I3BIMM },
	{ 0x0000000000008000ull, 0x000000000000b000ull, N("s8"), I3BIMM },
	{ 0x0000000000001000ull, 0x000000000000b000ull, N("u16"), I3BIMM },
	{ 0x0000000000009000ull, 0x000000000000b000ull, N("s16"), I3BIMM },
	{ 0x0000000000002000ull, 0x000000000000b000ull, N("u32"), I3BIMM },
	{ 0x000000000000a000ull, 0x000000000000b000ull, N("s32"), I3BIMM },
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

F1(sat22, 0x22, N("sat")) // vshl/vshr
F1(sat35, 0x35, N("sat")) // add,mul f32
F1(sat39, 0x39, N("sat")) // mad s32
F1(sat3a, 0x3a, N("sat")) // mul f32 long immediate
F1(ftz2f, 0x2f, N("ftz")) // add,mul f32
F1(ftz32, 0x32, N("ftz")) // setp f32
F1(ftz38, 0x38, N("ftz")) // fma f32
F1(ftz3a, 0x3a, N("ftz")) // add f32 long immediate
F1(fmz30, 0x30, N("fmz")) // mul f32
F1(fmz39, 0x39, N("fmz")) // fma f32, mul f32 long immediate
F1(neg8, 0x8, N("neg")) // setp f32 src2
F1(neg2e, 0x2e, N("neg")) // setp f32 src1
F1(neg30, 0x30, N("neg")) // add f32,f64 src2
F1(neg33, 0x33, N("neg")) // add,mul,fma f32,f64 src1
F1(neg34, 0x34, N("neg")) // fma f32,f64 src3
F1(neg38, 0x38, N("neg")) // set f32
F1(neg3b, 0x3b, N("neg")) // add,fma long immediate src1
F1(neg3c, 0x3c, N("neg")) // fma f32 long immediate
F1(abs9, 0x9, N("abs")) // setp f32 src1
F1(abs2f, 0x2f, N("abs")) // setp f32 src2
F1(abs31, 0x31, N("abs")) // add f32 src1
F1(abs34, 0x34, N("abs")) // add f32,f64 src2
F1(abs39, 0x39, N("abs")) // add f32 long immediate src1
F1(abs3c, 0x3c, N("abs")) // add f32 long immediate src2
F1(not2a, 0x2a, N("not")) // popc, logop src1
F1(not2b, 0x2b, N("not")) // logop src2
F1(not3a, 0x3a, N("not")) // logop src1
F1(not3b, 0x3b, N("not")) // logop src2

F1(rev2b, 0x2b, N("rev"))

F1(rint, 0x2d, T(frmi))

F1(acout32, 0x32, CC)
F1(acout37, 0x37, CC)
F1(acin2e, 0x2e, CC)
F1(acin34, 0x34, CC)
F1(acin38, 0x38, CC)
F1(acin3c, 0x3c, CC)

F1(shiftamt, 0x2c, N("shiftamt"))

F(shfclamp, 0x35, N("clamp"), N("wrap"))

F(sflane,0x1f,SRC2,SFLNIMM)
F(sfmask,0x20,SRC3,SFLMIMM)

F(us32_2b, 0x2b, N("u32"), N("s32"))
F(us32_2c, 0x2c, N("u32"), N("s32"))
F(us32_33, 0x33, N("u32"), N("s32"))
F(us32_38, 0x38, N("u32"), N("s32"))
F(us32_39, 0x39, N("u32"), N("s32"))
F(us32_3a, 0x3a, N("u32"), N("s32"))

F1(high2a, 0x2a, N("high"))
F1(high33, 0x33, N("high"))
F1(high38, 0x38, N("high"))
F1(high39, 0x39, N("high"))

F1(su3d, 0x32, N("3d"))
F(su1d, 0x38, N("1d"), N("2d"))
F(vsclamp, 0x34, N("clamp"), N("wrap"))

static struct insn tabus64_28[] = {
	{ 0x0000000000000000ull, 0x0000030000000000ull },
	{ 0x0000020000000000ull, 0x0000030000000000ull, N("u64") },
	{ 0x0000030000000000ull, 0x0000030000000000ull, N("s64") },
	{ 0, 0, OOPS },
};

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
static struct insn tabaldstt[] = {
	{ 0x0000000000000000ull, 0x000c000000000000ull, N("b32") },
	{ 0x0004000000000000ull, 0x000c000000000000ull, N("b64") },
	{ 0x0008000000000000ull, 0x000c000000000000ull, N("b96") },
	{ 0x000c000000000000ull, 0x000c000000000000ull, N("b128") },
	{ 0, 0, OOPS },
};
static struct insn tabaldstd[] = {
	{ 0x0000000000000000ull, 0x000c000000000000ull, DST },
	{ 0x0004000000000000ull, 0x000c000000000000ull, DSTD },
	{ 0x0008000000000000ull, 0x000c000000000000ull, DSTT },
	{ 0x000c000000000000ull, 0x000c000000000000ull, DSTQ },
	{ 0, 0, OOPS },
};
static struct insn tabatomd[] = {
	{ 0x0000000000000000ull, 0x0070000000000000ull, N("u32"), DST },
	{ 0x0010000000000000ull, 0x0070000000000000ull, N("s32"), DST },
	{ 0x0020000000000000ull, 0x0070000000000000ull, N("u64"), DSTD },
	{ 0x0030000000000000ull, 0x0070000000000000ull, N("f32"), DST },
	{ 0x0040000000000000ull, 0x0070000000000000ull, N("u128"), DSTQ },
	{ 0x0050000000000000ull, 0x0070000000000000ull, N("s64"), DSTD },
	{ 0, 0, OOPS },
};
static struct insn tabatoms[] = {
	{ 0x0000000000000000ull, 0x0070000000000000ull, SRC2 },
	{ 0x0010000000000000ull, 0x0070000000000000ull, SRC2 },
	{ 0x0020000000000000ull, 0x0070000000000000ull, SRC2D },
	{ 0x0030000000000000ull, 0x0070000000000000ull, SRC2 },
	{ 0x0040000000000000ull, 0x0070000000000000ull, SRC2Q },
	{ 0x0050000000000000ull, 0x0070000000000000ull, SRC2D },
	{ 0, 0, OOPS },
};
static struct insn tabredop[] = {
	{ 0x0000000000000000ull, 0x0780000000000000ull, N("add") },
	{ 0x0080000000000000ull, 0x0780000000000000ull, N("min") },
	{ 0x0100000000000000ull, 0x0780000000000000ull, N("max") },
	{ 0x0180000000000000ull, 0x0780000000000000ull, N("inc") },
	{ 0x0200000000000000ull, 0x0780000000000000ull, N("dec") },
	{ 0x0280000000000000ull, 0x0780000000000000ull, N("and") },
	{ 0x0300000000000000ull, 0x0780000000000000ull, N("or") },
	{ 0x0380000000000000ull, 0x0780000000000000ull, N("xor") },
	{ 0x0400000000000000ull, 0x0780000000000000ull, N("exch") },
	{ 0x0500000000000000ull, 0x0780000000000000ull, N("safeadd") },
	{ 0, 0, OOPS },
};
static struct insn tabcass[] = {
	{ 0x0020000000000000ull, 0x0070000000000000ull, SRC2, N("zero") },
	{ 0x0040000000000000ull, 0x0070000000000000ull, N("zero"), SRC2 },
	{ 0x0000000000000000ull, 0x0070000000000000ull, SRC2, SRC2P1 },
	{ 0, 0, OOPS },
};
static struct insn tabcasd[] = {
	{ 0x0010000000000000ull, 0x0070000000000000ull, SRC2D, SRC2DP1 },
	{ 0x0030000000000000ull, 0x0070000000000000ull, SRC2D, N("zero") },
	{ 0x0050000000000000ull, 0x0070000000000000ull, N("zero"), SRC2D },
	{ 0, 0, OOPS },
};
static struct insn tabprmtmod[] = {
	{ 0x0000000000000000ull, 0x0038000000000000ull },
	{ 0x0008000000000000ull, 0x0038000000000000ull, N("f4e")},
	{ 0x0010000000000000ull, 0x0038000000000000ull, N("b4e")},
	{ 0x0018000000000000ull, 0x0038000000000000ull, N("rc8")},
	{ 0x0020000000000000ull, 0x0038000000000000ull, N("ecl")},
	{ 0x0028000000000000ull, 0x0038000000000000ull, N("ecr")},
	{ 0x0030000000000000ull, 0x0038000000000000ull, N("rc16")},
	{ 0, 0, OOPS },
};
static struct insn tabshflmod[] = {
	{ 0x0000000000000000ull, 0x0000000600000000ull, N("idx")},
	{ 0x0000000200000000ull, 0x0000000600000000ull, N("up")},
	{ 0x0000000400000000ull, 0x0000000600000000ull, N("down")},
	{ 0x0000000600000000ull, 0x0000000600000000ull, N("bfly")},
	{ 0, 0, OOPS },
};
static struct insn tabrtt[] = {
	{ 0x0, 0xc, N("") },
	{ 0x4, 0xc, N("terminate") },
	{ 0x8, 0xc, N("fallthrough") },
	{ 0xc, 0xc, N("preempted") },
	{ 0, 0, OOPS },
};
static struct insn tabbpt[] = {
	{ 0x000, 0x700, N("drain_illegal") },
	{ 0x100, 0x700, N("cal") },
	{ 0x200, 0x700, N("pause") },
	{ 0x300, 0x700, N("trap") },
	{ 0x400, 0x700, N("int") },
	{ 0x500, 0x700, N("drain") },
	{ 0, 0, OOPS },
};
F1(emit, 0x2a, N("emit"))
F1(restart, 0x2b, N("restart"))
F1(patch, 0x22, N("patch"))
F1(outa, 0x23, N("out"))

// for quadop
static struct insn tabqs1[] = {
	{ 0x0000000000000000ull, 0x0000700000000000ull, N("l0") },
	{ 0x0000100000000000ull, 0x0000700000000000ull, N("l1") },
	{ 0x0000200000000000ull, 0x0000700000000000ull, N("l2") },
	{ 0x0000300000000000ull, 0x0000700000000000ull, N("l3") },
	{ 0x0000400000000000ull, 0x0000700000000000ull, N("dx") },
	{ 0x0000500000000000ull, 0x0000700000000000ull, N("dy") },
	{ 0, 0, OOPS },
};

static struct insn tabqop0[] = {
	{ 0x0000000000000000ull, 0x0000006000000000ull, N("add") },
	{ 0x0000002000000000ull, 0x0000006000000000ull, N("subr") },
	{ 0x0000004000000000ull, 0x0000006000000000ull, N("sub") },
	{ 0x0000006000000000ull, 0x0000006000000000ull, N("mov2") },
	{ 0, 0, OOPS },
};

static struct insn tabqop1[] = {
	{ 0x0000000000000000ull, 0x0000001800000000ull, N("add") },
	{ 0x0000000800000000ull, 0x0000001800000000ull, N("subr") },
	{ 0x0000001000000000ull, 0x0000001800000000ull, N("sub") },
	{ 0x0000001800000000ull, 0x0000001800000000ull, N("mov2") },
	{ 0, 0, OOPS },
};

static struct insn tabqop2[] = {
	{ 0x0000000000000000ull, 0x0000000600000000ull, N("add") },
	{ 0x0000000200000000ull, 0x0000000600000000ull, N("subr") },
	{ 0x0000000400000000ull, 0x0000000600000000ull, N("sub") },
	{ 0x0000000600000000ull, 0x0000000600000000ull, N("mov2") },
	{ 0, 0, OOPS },
};

static struct insn tabqop3[] = {
	{ 0x0000000000000000ull, 0x0000000180000000ull, N("add") },
	{ 0x0000000080000000ull, 0x0000000180000000ull, N("subr") },
	{ 0x0000000100000000ull, 0x0000000180000000ull, N("sub") },
	{ 0x0000000180000000ull, 0x0000000180000000ull, N("mov2") },
	{ 0, 0, OOPS },
};

static struct insn tabmulf[] = {
	{ 0x0000000000000000ull, 0x0000700000000000ull },
	{ 0x0000100000000000ull, 0x0000700000000000ull, N("div2") },
	{ 0x0000200000000000ull, 0x0000700000000000ull, N("div4") },
	{ 0x0000300000000000ull, 0x0000700000000000ull, N("div8") },
	{ 0x0000400000000000ull, 0x0000700000000000ull, N("mul8") },
	{ 0x0000500000000000ull, 0x0000700000000000ull, N("mul4") },
	{ 0x0000600000000000ull, 0x0000700000000000ull, N("mul2") },
	{ 0, 0, OOPS },
};

static struct insn tabsup[] = {
	{ 0x0000000000000000ull, 0x0000200000000000ull, PSRC3 },
	{ 0x0000200000000000ull, 0x0000200000000000ull, SESTART, N("not"), PSRC3, SEEND },
	{ 0, 0, OOPS },
};

static struct insn tabsup2[] = {
	{ 0x0000000000000000ull, 0x0020000000000000ull, PSRC5 },
	{ 0x0020000000000000ull, 0x0020000000000000ull, SESTART, N("not"), PSRC5, SEEND },
	{ 0, 0, OOPS },
};

static struct insn tabsulcop[] = {
	{ 0x0000000000000000ull, 0x0000000180000000ull, N("ca") },
	{ 0x0000000080000000ull, 0x0000000180000000ull, N("cg") },
	{ 0x0000000100000000ull, 0x0000000180000000ull, N("cs") },
	{ 0x0000000180000000ull, 0x0000000180000000ull, N("cv") },
	{ 0, 0, OOPS },
};

static struct insn tabsulcop2[] = {
	{ 0x0000000000000000ull, 0x00c0000000000000ull, N("ca") },
	{ 0x0040000000000000ull, 0x00c0000000000000ull, N("cg") },
	{ 0x0080000000000000ull, 0x00c0000000000000ull, N("cs") },
	{ 0x00c0000000000000ull, 0x00c0000000000000ull, N("cv") },
	{ 0, 0, OOPS },
};

static struct insn tabsuscop[] = {
	{ 0x0000000000000000ull, 0x0000000180000000ull, N("wb") },
	{ 0x0000000080000000ull, 0x0000000180000000ull, N("cg") },
	{ 0x0000000100000000ull, 0x0000000180000000ull, N("cs") },
	{ 0x0000000180000000ull, 0x0000000180000000ull, N("wt") },
	{ 0, 0, OOPS },
};

static struct insn tabsuscop2[] = {
	{ 0x0000000000000000ull, 0x00c0000000000000ull, N("wb") },
	{ 0x0040000000000000ull, 0x00c0000000000000ull, N("cg") },
	{ 0x0080000000000000ull, 0x00c0000000000000ull, N("cs") },
	{ 0x00c0000000000000ull, 0x00c0000000000000ull, N("wt") },
	{ 0, 0, OOPS },
};

static struct insn tabsclamp2s[] = {
	{ 0x0000000000000000ull, 0x0000000001800000ull, N("ign") },
	{ 0x0000000000800000ull, 0x0000000001800000ull, N("trap") },
	{ 0x0000000001800000ull, 0x0000000001800000ull, N("sdcl") },
	{ 0, 0, OOPS },
};

static struct insn tabsclamp2s2[] = {
	{ 0x0000000000000000ull, 0x000000000000000cull, N("ign") },
	{ 0x0000000000000004ull, 0x000000000000000cull, N("trap") },
	{ 0x000000000000000cull, 0x000000000000000cull, N("sdcl") },
	{ 0, 0, OOPS },
};

static struct insn tabsudst[] = {
	{ 0x0000000000000000ull, 0x0000000e00000000ull, N("u8"), DST },
	{ 0x0000000200000000ull, 0x0000000e00000000ull, N("s8"), DST },
	{ 0x0000000400000000ull, 0x0000000e00000000ull, N("u16"), DST },
	{ 0x0000000600000000ull, 0x0000000e00000000ull, N("s16"), DST },
	{ 0x0000000800000000ull, 0x0000000e00000000ull, N("b32"), DST },
	{ 0x0000000a00000000ull, 0x0000000e00000000ull, N("b64"), DSTD },
	{ 0x0000000c00000000ull, 0x0000000e00000000ull, N("b128"), DSTQ },
	{ 0, 0, OOPS },
};

static struct insn tabsudst1[] = {
	{ 0x0000000000000000ull, 0x0700000000000000ull, N("u8"), DST },
	{ 0x0100000000000000ull, 0x0700000000000000ull, N("s8"), DST },
	{ 0x0200000000000000ull, 0x0700000000000000ull, N("u16"), DST },
	{ 0x0300000000000000ull, 0x0700000000000000ull, N("s16"), DST },
	{ 0x0400000000000000ull, 0x0700000000000000ull, N("b32"), DST },
	{ 0x0500000000000000ull, 0x0700000000000000ull, N("b64"), DSTD },
	{ 0x0600000000000000ull, 0x0700000000000000ull, N("b128"), DSTQ },
	{ 0, 0, OOPS },
};

static struct insn tabsclamp2l[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull, N("zero") },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, N("trap") },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, N("sdcl") },
	{ 0, 0, OOPS },
};

static struct insn tabsuldty[] = {
	{ 0x0000000000000000ull, 0x0030000000000000ull, N("u32") },
	{ 0x0010000000000000ull, 0x0030000000000000ull, N("s32") },
	{ 0x0020000000000000ull, 0x0030000000000000ull, N("u8") },
	{ 0x0030000000000000ull, 0x0030000000000000ull, N("s8") },
	{ 0, 0, OOPS },
};

static struct insn tabsustty[] = {
	{ 0x0000000000000000ull, 0x0000000060000000ull, N("u32") },
	{ 0x0000000020000000ull, 0x0000000060000000ull, N("s32") },
	{ 0x0000000040000000ull, 0x0000000060000000ull, N("u8") },
	{ 0x0000000060000000ull, 0x0000000060000000ull, N("s8") },
	{ 0, 0, OOPS },
};

static struct insn tabsustty2[] = {
	{ 0x0000000000000000ull, 0x0000000000000300ull, N("u32") },
	{ 0x0000000000000100ull, 0x0000000000000300ull, N("s32") },
	{ 0x0000000000000200ull, 0x0000000000000300ull, N("u8") },
	{ 0x0000000000000300ull, 0x0000000000000300ull, N("s8") },
	{ 0, 0, OOPS },
};

static struct insn tabsucm[] = {
	{ 0x0000000000000000ull, 0x00f0000000000000ull, N("sd"), N("r1") }, // probably
	{ 0x0010000000000000ull, 0x00f0000000000000ull, N("sd"), N("r2") },
	{ 0x0020000000000000ull, 0x00f0000000000000ull, N("sd"), N("r4") },
	{ 0x0030000000000000ull, 0x00f0000000000000ull, N("sd"), N("r8") },
	{ 0x0040000000000000ull, 0x00f0000000000000ull, N("sd"), N("r16") },

	{ 0x0050000000000000ull, 0x00f0000000000000ull, N("pl"), N("r1") },
	{ 0x0060000000000000ull, 0x00f0000000000000ull, N("pl"), N("r2") },
	{ 0x0070000000000000ull, 0x00f0000000000000ull, N("pl"), N("r4") },
	{ 0x0080000000000000ull, 0x00f0000000000000ull, N("pl"), N("r8") },
	{ 0x0090000000000000ull, 0x00f0000000000000ull, N("pl"), N("r16") },
	{ 0x00a0000000000000ull, 0x00f0000000000000ull, N("bl"), N("r1") },
	{ 0x00b0000000000000ull, 0x00f0000000000000ull, N("bl"), N("r2") },
	{ 0x00c0000000000000ull, 0x00f0000000000000ull, N("bl"), N("r4") },
	{ 0x00d0000000000000ull, 0x00f0000000000000ull, N("bl"), N("r8") },
	{ 0x00e0000000000000ull, 0x00f0000000000000ull, N("bl"), N("r16") },
	{ 0, 0, OOPS },
};

static struct insn tabcctlop[] = {
	{ 0x0000000000000000ull, 0x000000000000003cull, N("query1") },
	{ 0x0000000000000004ull, 0x000000000000003cull, N("pf1") },
	{ 0x0000000000000008ull, 0x000000000000003cull, N("pf15") },
	{ 0x000000000000000cull, 0x000000000000003cull, N("pf2") },
	{ 0x0000000000000010ull, 0x000000000000003cull, N("wb") },
	{ 0x0000000000000014ull, 0x000000000000003cull, N("iv") },
	{ 0x0000000000000018ull, 0x000000000000003cull, N("ivall") },
	{ 0x000000000000001cull, 0x000000000000003cull, N("rs") },
	{ 0x0000000000000020ull, 0x000000000000003cull, N("wball") },
	{ 0x0000000000000024ull, 0x000000000000003cull, N("rslb") },
	{ 0, 0, OOPS },
};

static struct insn tabcctlmod[] = {
	{ 0x0000000000000000ull, 0x00000000000000c0ull },
	{ 0x0000000000000040ull, 0x00000000000000c0ull, N("u") },
	{ 0x0000000000000080ull, 0x00000000000000c0ull, N("c") },
	{ 0x00000000000000c0ull, 0x00000000000000c0ull, N("i") },
	{ 0, 0, OOPS },
};

static struct insn tabbar[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull, SRC1, SRC2 },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, SRC1, BARIMM2 },
	{ 0x0000800000000000ull, 0x0000c00000000000ull, BARIMM1, SRC2 },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, BARIMM1, BARIMM2 },
	{ 0, 0, OOPS },
};

static struct insn tabvdst[] = {
	{ 0x0000000000000000ull, 0x01c0000000000000ull, N("h1") },
	{ 0x0040000000000000ull, 0x01c0000000000000ull, N("h0") },
	{ 0x0080000000000000ull, 0x01c0000000000000ull, N("b0") },
	{ 0x00c0000000000000ull, 0x01c0000000000000ull, N("b2") },
	{ 0x0100000000000000ull, 0x01c0000000000000ull, N("add") },
	{ 0x0140000000000000ull, 0x01c0000000000000ull, N("min") },
	{ 0x0180000000000000ull, 0x01c0000000000000ull, N("max") },
	{ 0x01c0000000000000ull, 0x01c0000000000000ull },
	{ 0, 0, OOPS },
};

static struct insn tabvsrc1[] = {
	{ 0x0000000000000000ull, 0x0000038000000000ull, N("b0") },
	{ 0x0000008000000000ull, 0x0000038000000000ull, N("b1") },
	{ 0x0000010000000000ull, 0x0000038000000000ull, N("b2") },
	{ 0x0000018000000000ull, 0x0000038000000000ull, N("b3") },
	{ 0x0000020000000000ull, 0x0000038000000000ull, N("h0") },
	{ 0x0000028000000000ull, 0x0000038000000000ull, N("h1") },
	{ 0x0000030000000000ull, 0x0000038000000000ull },
	{ 0, 0, OOPS },
};

static struct insn tabvsrc2[] = {
	{ 0x0000000000000000ull, 0x0020000380000000ull },
	{ 0x0020000000000000ull, 0x0020000380000000ull, N("b0") },
	{ 0x0020000080000000ull, 0x0020000380000000ull, N("b1") },
	{ 0x0020000100000000ull, 0x0020000380000000ull, N("b2") },
	{ 0x0020000180000000ull, 0x0020000380000000ull, N("b3") },
	{ 0x0020000200000000ull, 0x0020000380000000ull, N("h0") },
	{ 0x0020000300000000ull, 0x0020000380000000ull, N("h1") },
	{ 0x0020000380000000ull, 0x0020000380000000ull },
	{ 0, 0, OOPS },
};

static struct insn tabvs2[] = {
	{ 0x0000000000000000ull, 0x0020000000000000ull, VIMM },
	{ 0x0020000000000000ull, 0x0020000000000000ull, SRC2 },
	{ 0, 0, OOPS },
};

static struct insn tabimadp1[] = {
	{ 0x0000000000000000ull, 0x0038000000000000ull, N("u32") },
	{ 0x0008000000000000ull, 0x0038000000000000ull, N("s32") },
	{ 0x0010000000000000ull, 0x0038000000000000ull, N("u24") },
	{ 0x0018000000000000ull, 0x0038000000000000ull, N("s24") },
	{ 0x0020000000000000ull, 0x0038000000000000ull, N("u16l") },
	{ 0x0028000000000000ull, 0x0038000000000000ull, N("s16l") },
	{ 0x0030000000000000ull, 0x0038000000000000ull, N("u16h") },
	{ 0x0038000000000000ull, 0x0038000000000000ull, N("s16h") },
	{ 0, 0, OOPS },
};

static struct insn tabimadp2[] = {
	{ 0x0000000000000000ull, 0x0300000000000000ull, N("u24") },
	{ 0x0100000000000000ull, 0x0300000000000000ull, N("s24") },
	{ 0x0200000000000000ull, 0x0300000000000000ull, N("u16l") },
	{ 0x0300000000000000ull, 0x0300000000000000ull, N("s16l") },
	{ 0, 0, OOPS },
};

static struct insn tabimadp3[] = {
	{ 0x0000000000000000ull, 0x0188000000000000ull, N("u32") },
	{ 0x0008000000000000ull, 0x0188000000000000ull, N("s32") },
	{ 0x0100000000000000ull, 0x0188000000000000ull, N("s32") },
	{ 0x0108000000000000ull, 0x0188000000000000ull, N("s32") },
	{ 0x0040000000000000ull, 0x0188000000000000ull, N("u24") },
	{ 0x0048000000000000ull, 0x0188000000000000ull, N("s24") },
	{ 0x0140000000000000ull, 0x0188000000000000ull, N("s24") },
	{ 0x0148000000000000ull, 0x0188000000000000ull, N("s24") },
	{ 0x0080000000000000ull, 0x0188000000000000ull, N("u16l") },
	{ 0x0088000000000000ull, 0x0188000000000000ull, N("s16l") },
	{ 0x0180000000000000ull, 0x0188000000000000ull, N("s16l") },
	{ 0x0188000000000000ull, 0x0188000000000000ull, N("s16l") },
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
 * 000003ffff800000 float/double immediate
 * 007fffffff800000 long immediate
 * 0003fc0000000000 3rd src
 * 007c000000000000 misc flags
 * 3fc0000000000000 opcode (can contain rounding mode, ftz)
 * c000000000000000 source type
 */

/*
<WIP>
NOTE: auto-scan for unknowns: type "make" in ptxgen directory
observed flag changes:
    I[0]=I[1]<<29;
        00000048: 0e9c080d b7c00e00     lshf b32 $r3 (b64 $r2 $r3) 0x1d [unknown: 00000000 00000200]
        00000050: 0e9ffc09 b7c00800     lshf b32 $r2 (b64 0x0 $r2) 0x1d
    I[0]=I[1]<<33;
        00000048: 109c080d b7c00e00     lshf b32 $r3 (b64 $r2 $r3) 0x21 [unknown: 00000000 00000200]
        00000050: 109ffc09 b7c00a00     lshf b32 $r2 (b64 0x0 $r2) 0x21 [unknown: 00000000 00000200]
</WIP> 
*/

static struct insn tabm[] = {
	{ 0x8400000000000002ull, 0xffc0000000000003ull, T(sfuop), T(sat35), N("f32"), DST, T(neg33), T(abs31), SRC1 },
	{ 0x8440000000000002ull, 0xffc0000000000003ull, N("set"), DST, T(acout32), T(pnot11), PSRC1, T(setlop2), T(setlop3) },
	{ 0x8480000000000002ull, 0xffc0000000000003ull, N("set"), PDST, PDSTN, T(pnot11), PSRC1, T(setlop2), T(setlop3)},
	{ 0x8540000000000002ull, 0xffc000f800000003ull, N("bar"), N("sync"), T(bar), PSRC3 },
	{ 0x8540000800000002ull, 0xffc000f800000003ull, N("bar"), N("arrive"), T(bar), PSRC3 },
	{ 0x8540001000000002ull, 0xffc000f800000003ull, N("bar"), N("red"), N("popc"), T(bar), T(pnot2d), PSRC3 },
	{ 0x8540001800000002ull, 0xffc000f800000003ull, N("bar"), N("scan"), T(bar), T(pnot2d), PSRC3 },
	{ 0x8540002000000002ull, 0xffc000f800000003ull, N("bar"), N("syncall") },
	{ 0x8540005000000002ull, 0xffc000f800000003ull, N("bar"), N("red"), N("and"), T(bar), T(pnot2d), PSRC3 },
	{ 0x8540009000000002ull, 0xffc000f800000003ull, N("bar"), N("red"), N("or"), T(bar), T(pnot2d), PSRC3 },
        { 0x8580000000000002ull, 0xffc0000000000003ull, T(cc2), N("nop") },
        { 0x8680000000000002ull, 0xffc0000000000003ull, N("lepc"), DST },
        { 0x86c0000000000002ull, 0xfff8000000000003ull, N("vote"), N("all"), DST, PSRC4, T(pnot2d), PSRC3 },
        { 0x86c8000000000002ull, 0xfff8000000000003ull, N("vote"), N("any"), DST, PSRC4, T(pnot2d), PSRC3 },
        { 0x86d0000000000002ull, 0xfff8000000000003ull, N("vote"), N("uni"), DST, PSRC4, T(pnot2d), PSRC3 },
	{ 0x8640000000000002ull, 0xbfc0000000000003ull, N("mov"), N("b32"), DST, SREG },
	{ 0x0000000000000002ull, 0x3880000000000003ull, N("set"), T(ftz3a), N("b32"), DST, T(acout32), T(setit), N("f32"), T(neg2e), T(abs39), SRC1, T(neg38), T(abs2f), T(is2), T(setlop3) }, // XXX: find f32 dst
	{ 0x0800000000000002ull, 0x3cc0000000000003ull, N("set"), N("b32"), DST, T(acout32), T(setit), N("f64"), T(neg2e), T(abs39), SRC1D, T(neg38), T(abs2f), T(ds2), T(setlop3) },
	{ 0x0c00000000000002ull, 0x3c00000000000003ull, N("fma"), T(ftz38), T(fmz39), T(sat35),  T(frm36), N("f32"), DST, T(neg33), SRC1, T(is2w3), T(neg34), T(is3) },
	{ 0x1000000000000002ull, 0x3c00000000000003ull, T(addop3a), T(sat35), DST, T(acout32), SESTART, N("mul"), T(high39), T(us32_33), SRC1, T(us32_38), T(is2w3), SEEND, T(is3), T(acin34) }, // XXX: order of us32
	{ 0x14c0000000000002ull, 0x3cc0000000000003ull, N("mad"), N("b32"), N("sd"), DST, T(acout32), SRC1, T(is2w3), T(is3) },
	{ 0x1400000000000002ull, 0x3c00000000000003ull, N("mad"), N("b32"), DST, T(acout32), T(imadp1), SRC1, T(imadp2), T(is2w3), T(imadp3), T(is3) },
	{ 0x1a00000000000002ull, 0x3f80000000000003ull, N("slct"), N("b32"), DST, SRC1, T(is2w3), T(isetit), T(us32_33), T(is3) }, // XXX: check us32_33
	{ 0x1a80000000000002ull, 0x3f80000000000003ull, N("set"), N("b32"), DST, T(acout32), T(isetit), T(us32_33), SRC1, T(is2), T(acin2e), T(setlop3) },
	{ 0x1b00000000000002ull, 0x3f80000000000003ull, N("set"), PDST, PDSTN, T(isetit), T(us32_33), SRC1, T(is2), T(acin2e), T(setlop3) },
	{ 0x1b80000000000002ull, 0x3f80000000000003ull, N("fma"), T(frm35), N("f64"), DSTD, T(neg33), SRC1D, T(ds2w3), T(neg34), T(ds3) },
	{ 0x1c00000000000002ull, 0x3f80000000000003ull, N("set"), PDST, PDSTN, T(setit), N("f64"), T(neg2e), T(abs9), SRC1D, T(neg8), T(abs2f), T(ds2), T(setlop3) },
	{ 0x1d00000000000002ull, 0x3f80000000000003ull, N("slct"), T(ftz32), N("b32"), DST, SRC1, T(is2w3), T(setit), N("f32"), T(is3) },
	{ 0x1d80000000000002ull, 0x3f80000000000003ull, N("set"), T(ftz32), PDST, PDSTN, T(setit), N("f32"), T(neg2e), T(abs9), SRC1, T(neg8), T(abs2f), T(is2), T(setlop3) },
	{ 0x1e00000000000002ull, 0x3fc0000000000003ull, N("prmt"), T(prmtmod), N("b32"), DST, SRC1, T(is2w3), T(is3) },
	{ 0x1e80000000000002ull, 0x3fc0000000000003ull, N("subfm"), T(su3d), PDST2, DST, SRC1, T(is2w3), T(is3) },
	{ 0x1ec0000000000002ull, 0x3fc0000000000003ull, N("sueau"), DST, SRC1, T(is2w3), T(is3) },
	{ 0x1f00000000000002ull, 0x3fc0000000000003ull, N("out"), T(emit), T(restart), DST, SRC1, T(is2) },
	{ 0x1f40000000000002ull, 0x3fc0000000000003ull, N("sad"), T(us32_33), DST, SRC1, T(is2w3), T(is3) },
	{ 0x1f80000000000002ull, 0x3fc0000000000003ull, N("ins"), N("b32"), DST, SRC1, T(is2w3), T(is3) },
	{ 0x2140000000000002ull, 0x3fc0000000000003ull, N("shr"), T(us32_33), DST, SRC1, T(shclamp), T(is2) },
	{ 0x2000000000000002ull, 0xf800000000000003ull, N("mul"), T(ftz38), T(fmz39), T(sat3a), N("f32"), DST, T(acout37), SRC1, LIMM }, // XXX: double-check LIMM offset
	{ 0x2040000000000002ull, 0x3fc0000000000003ull, N("popc"), N("b32"), DST, T(not2a), SRC1, T(not2b), T(is2) }, // popc(src1 & src2)
	{ 0x2080000000000002ull, 0x3fc0000000000003ull, T(addop), T(sat35), N("b32"), DST, T(acout32), SRC1, T(is2), T(acin2e) },
	{ 0x20c0000000000002ull, 0x3fc0000000000003ull, T(addop), N("b32"), DST, T(acout32), SESTART, N("shl"), SRC1, SHCNT, SEEND, T(is2) },
	{ 0x2100000000000002ull, 0x3fc0000000000003ull, T(minmax), T(us32_33), DST, SRC1, T(is2) },
	{ 0x2180000000000002ull, 0x3fc0000000000003ull, N("bfind"), T(shiftamt), T(us32_33), DST, T(not2b), T(is2) },   //XXX: eyeballed is2, check cout
	{ 0x21c0000000000002ull, 0x3fc0000000000003ull, N("mul"), T(high2a), DST, T(acout32), T(us32_2b), SRC1, T(us32_2c), T(is2) }, // XXX: order of us32
	{ 0x2200000000000002ull, 0x3fc0000000000003ull, T(logop), N("b32"), DST, T(not2a), SRC1, T(not2b), T(is2) },
	{ 0x2240000000000002ull, 0x3fc0000000000003ull, N("shl"), N("b32"), DST, SRC1, T(shclamp), T(is2) },
	{ 0x2280000000000002ull, 0x3fc0000000000003ull, T(minmax), N("f64"), DSTD, T(neg33), T(abs31), SRC1D, T(neg30), T(abs34), T(ds2) },
	{ 0x22c0000000000002ull, 0x3fc0000000000003ull, N("add"), T(ftz2f), T(sat35), T(frm2a), N("f32"), DST, T(neg33), T(abs31), SRC1, T(neg30), T(abs34), T(is2) },
	{ 0x2300000000000002ull, 0x3fc0000000000003ull, T(minmax), T(ftz2f), N("f32"), DST, T(neg33), T(abs31), SRC1, T(neg30), T(abs34), T(is2) },
	{ 0x2340000000000002ull, 0x3fc0000000000003ull, N("mul"), T(mulf), T(ftz2f), T(fmz30), T(sat35), T(frm2a), T(neg33), N("f32"), DST, SRC1, T(is2) },
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
	{ 0x2540000000002c02ull, 0x3fc0000000003c03ull, N("cvt"), N("f64"), DSTD, N("f32"), T(neg30), T(abs34), T(is2) }, // XXX: do ftz, sat work here ?
	{ 0x2540000000003802ull, 0x3fc0000000003c03ull, N("cvt"), T(frm2a), N("f32"), DST, N("f64"), T(neg30), T(abs34), T(ds2) }, // XXX: do ftz, sat work here ?
	{ 0x2540000000003c02ull, 0x3fc0000000003c03ull, N("cvt"), T(rint), N("f64"), DSTD, N("f64"), T(neg30), T(abs34), T(ds2) },
	{ 0x2580000000000002ull, 0x3fc0000000000003ull, N("cvt"), T(ftz2f), T(frmi), T(cvtf2idst), T(neg30), T(abs34), T(cvtf2isrc) },
	{ 0x25c0000000000002ull, 0x3fc0000000000003ull, N("cvt"), T(frm2a), T(cvti2fdst), T(neg30), T(abs34), T(cvti2fsrc) },
	{ 0x2600000000000002ull, 0x3fc0000000000003ull, N("cvt"), T(sat35), T(cvti2idst), T(neg30), T(abs34), T(cvti2isrc) },
	{ 0x27c0000000000002ull, 0x3fc0000000000003ull, N("rshf"), T(high33), N("b32"), DST, SESTART, T(us64_28), SRC1, SRC3, SEEND, T(shfclamp), T(is2) }, // XXX: check is2 and bits 0x29,0x33(swap srcs ?)
	{ 0x2800000000000002ull, 0xf800000000000003ull, N("mul"), T(high38), DST, T(acout37), T(us32_39), SRC1, T(us32_3a), LIMM },
	{ 0x3000000000000002ull, 0xf800000000000003ull, N("suldgb"), T(sudst1), T(sulcop2), T(sclamp2l), T(suldty), GLOBALDSU, CONST, T(sup) },
	{ 0x3800000000000002ull, 0xf8000000000000f3ull, N("sustgb"), T(suscop2), T(sclamp2s2), T(sustty2), GLOBALDSU, CONST, SRC3, T(sup2) },
	{ 0x3800000000000002ull, 0xf800000000000003ull, N("sustgp"), T(suscop2), T(sclamp2s2), T(sustty2), GLOBALDSU, CONST, SUSTPSRC1, T(sup2) },
	{ 0x6800000000000002ull, 0xf800000000000003ull, N("ld"), T(redop), T(atomd), T(gamem), T(atoms) },
	{ 0x7000000000000002ull, 0x7c00000000000003ull, N("texfetch"), T(texm), T(lodf), T(texms), T(texoff2), T(ltex), TDST, T(text), T(texsrc1), T(texsrc2) }, // XXX: args are wrong
	{ 0x7400000000000002ull, 0x7f80000000000003ull, T(lane0e), N("mov"), N("b32"), DST, LIMM },
	{ 0x7480000000000002ull, 0x7f80000000000003ull, N("interp"), T(interpmode), N("f32"), DST, IATTR, SRC2, SRC1, SRC3 },
	{ 0x5800000000000002ull, 0x7e00000000000003ull, N("suclamp"), T(su1d), T(us32_33), T(sucm), PSRC4, DST, SRC1, T(is2), SUIMM },
	{ 0x7540000000000002ull, 0x7fc0000000000003ull, N("texquery"), T(texm), T(ltex), TDST, T(text), T(texquery), SRC1, SRC2 }, // XXX: check src args
	{ 0x7600000000000002ull, 0x7fc0000000000003ull, N("texgrad"), T(texm), T(ltex), TDST, T(text), TCONST, T(texgrsrc1), T(texgrsrc2) },
	{ 0x76c0000000000002ull, 0x7fc0000000000003ull, N("texcsaa"), T(texm), T(texf), TDST, TSRC13 },
	{ 0x7700000000000002ull, 0x7fc0000000000003ull, N("texbar"), TEXBARIMM },
	{ 0x7740000000000002ull, 0x7fc0000000000003ull, N("ld"), N("lock"), T(lldstt), PDSTL, T(lldstd), SHARED },
	{ 0x7780000000000002ull, 0xff90000000000003ull, N("cas"), N("b32"), DST, T(gamem), T(cass) },
	{ 0x7790000000000002ull, 0xff90000000000003ull, N("cas"), N("b64"), DSTD, T(gamem), T(casd) },
	{ 0x7800000000000002ull, 0x7fc0000000000003ull, N("texfetch"), T(texm), T(lodf), T(texms), T(texoff2), T(ltex), TDST, T(text), N("ind"), T(texsrc1), T(texsrc2) }, // XXX: args are wrong
	{ 0x7840000000000002ull, 0x7fc0000000000003ull, N("st"), N("unlock"), T(lldstt), PDSTL, SHARED, T(lldstd) },
	{ 0x7880000000000002ull, 0x7fc0000000000003ull, N("shfl"), T(shflmod), N("b32"), PDST2, DST, SRC1, T(sflane), T(sfmask) },
	{ 0x7980000000000002ull, 0xffc0000000000003ull, N("suldgb"), T(sudst), T(sulcop), T(sclamp2l), T(suldty), GLOBALDSU, SRC2, T(sup) },
	{ 0x79c0000000000002ull, 0xffc000001e000003ull, N("sustgb"), T(suscop), T(sclamp2s), T(sustty), GLOBALDSU, DST, SRC3, T(sup2) },
	{ 0x79c0000000000002ull, 0xffc0000000000003ull, N("sustgp"), T(suscop), T(sclamp2s), T(sustty), GLOBALDSU, DST, SUSTPSRC, T(sup2) },

	{ 0x7a00000000000002ull, 0x7fc0000000000003ull, N("ld"), T(lldstt), T(llcop), T(lldstd), LOCAL },
	{ 0x7a40000000000002ull, 0x7fc0000000000003ull, N("ld"), T(lldstt), T(lldstd), SHARED },
	{ 0x7a80000000000002ull, 0x7fc0000000000003ull, N("st"), T(lldstt), T(lscop), LOCAL, T(lldstd) },
	{ 0x7ac0000000000002ull, 0x7fc0000000000003ull, N("st"), T(lldstt), SHARED, T(lldstd) },
	{ 0x7b00000000000002ull, 0x7f00000000000003ull, N("cctl"), T(cctlmod), T(cctlop), T(gmem) },
	{ 0x7c00000000000002ull, 0x7f80000000000003ull, N("cctl"), T(cctlop), LOCAL },
	{ 0x7c80000000000002ull, 0x7fc0000000000003ull, N("ld"), T(lldstt), T(lldstd), LCONST },
	{ 0x7cc00000001c0002ull, 0x7fc00000001c0c03ull, N("membar"), N("cta") },
	{ 0x7cc00000001c0402ull, 0x7fc00000001c0c03ull, N("membar"), N("gl") },
	{ 0x7cc00000001c0802ull, 0x7fc00000001c0c03ull, N("membar"), N("sys") },
	{ 0x7d00000000000002ull, 0x7fc0000000000003ull, N("al2p"), DST, T(outa), SRC1, TEXSIMM },
	{ 0x7d40000000000002ull, 0x7fc0000000000003ull, N("texquery"), T(texm), T(ltex), TDST, T(text), T(texquery), N("ind"), SRC1, SRC2 }, // XXX: check src args
	{ 0x7d80000000000002ull, 0x7fc0000000000003ull, N("tex"), T(texm), T(lodt), T(texoff), T(texf), TDST, T(text), N("ind"), T(texsrc1), T(texsrc2) },
	{ 0x7e00000000000002ull, 0x7fc0000000000003ull, N("texgrad"), T(texm), T(ltex), TDST, T(text), N("ind"), T(texgrsrc1), T(texgrsrc2) },
	{ 0x7ec0000000000002ull, 0x7fc0000000000003ull, N("ld"), T(patch), T(aldstt), T(aldstd), T(outa), ATTR, SRC1, SRC3 },
	{ 0x7f00000000000002ull, 0x7fc0000000000003ull, N("st"), T(patch), T(aldstt), ATTR, T(aldstd), SRC1, SRC3 },
	{ 0x7f80000000000002ull, 0x7fc0000000000003ull, N("ld"), N("b32"), DST, VBA },
	{ 0x7fc0000000000002ull, 0x7fc0000000000003ull, N("quadop"), T(ftz2f), T(frm2a), N("f32"), T(qop0), T(qop1), T(qop2), T(qop3), DST, T(acout32), T(dtex), T(qs1), SRC1, SRC2 },
	{ 0xb000000000000002ull, 0xf800000000000003ull, N("vshr"), T(vsclamp), T(sat22), T(vdst), T(us32_39), DST, T(acout32), T(vsrc1), T(us32_33), SRC1, T(vsrc2), T(us32_33), T(vs2), SRC3 },
	{ 0xb800000000000002ull, 0xf800000000000003ull, N("vshl"), T(vsclamp), T(sat22), T(vdst), T(us32_39), DST, T(acout32), T(vsrc1), T(us32_33), SRC1, T(vsrc2), T(us32_33), T(vs2), SRC3 },
	{ 0xe000000000000002ull, 0xffc0000000000003ull, N("ext"), T(rev2b), T(us32_33), DST, SRC1, SRC2},  //XXX? can't find CONST
	{ 0xdfc0000000000002ull, 0xffc0000000000003ull, N("lshf"), T(high33), N("b32"), DST, SESTART, T(us64_28), SRC1, SRC3, SEEND, T(shfclamp), SRC2 },
	{ 0x0, 0x0, DST, SRC1, SRC2, SRC3, OOPS },
};

static struct insn tabi[] = {
	{ 0x4000000000000001ull, 0xf400000000000003ull, T(addop3b), T(sat39), N("b32"), DST, T(acout37), SRC1, LIMM , T(acin38) },
	{ 0x8000000000000001ull, 0xf180000000000003ull, N("set"), T(ftz3a), N("b32"), DST, T(acout32), T(setit), N("f32"), T(neg2e), T(abs39), SRC1, FIMM, T(setlop3) }, // XXX: find f32 dst
	{ 0x0040000000000001ull, 0x37c0000000000003ull, N("popc"), N("b32"), DST, T(not2a), SRC1, T(i3bi2) },
	{ 0x0080000000000001ull, 0x37c0000000000003ull, T(addop), T(sat35), N("b32"), DST, T(acout32), SRC1, T(i3bi2) },
	{ 0x00c0000000000001ull, 0x37c0000000000003ull, T(addop), N("b32"), DST, T(acout32), SESTART, N("shl"), SRC1, SHCNT, SEEND, T(i3bi2)},
	{ 0x0100000000000001ull, 0x37c0000000000003ull, T(minmax), T(us32_33), DST, SRC1, T(i3bi2) },
	{ 0x0140000000000001ull, 0x37c0000000000003ull, N("shr"), T(us32_33), DST, SRC1, T(shclamp), T(sui2b) },
	{ 0x01c0000000000001ull, 0x37c0000000000003ull, N("mul"), T(high2a), DST, T(us32_2b), SRC1, T(us32_2c), T(i3bi2) }, // XXX: order of us32, TODO: find LIMM form
	{ 0x0200000000000001ull, 0x37c0000000000003ull, T(logop), N("b32"), DST, T(not3a), SRC1, T(not3b), T(i3bi2) },
	{ 0x0240000000000001ull, 0x37c0000000000003ull, N("shl"), N("b32"), DST, SRC1, T(shclamp), T(sui2b) },
	{ 0x0280000000000001ull, 0x37c0000000000003ull, T(minmax), N("f64"), DSTD, T(neg33), T(abs31), SRC1D, T(di2) },
	{ 0x02c0000000000001ull, 0x37c0000000000003ull, N("add"), T(ftz2f), T(sat35), T(frm2a), N("f32"), DST, T(neg33), T(abs31), SRC1, T(fi2) },
	{ 0x0300000000000001ull, 0x37c0000000000003ull, T(minmax), T(ftz2f), N("f32"), DST, T(neg33), T(abs31), SRC1, T(fi2) },
	{ 0x0340000000000001ull, 0x37c0000000000003ull, N("mul"), T(ftz2f), T(fmz30), T(sat35), T(frm2a), N("f32"), DST, SRC1, T(fi2) },
	{ 0x0380000000000001ull, 0x37c0000000000003ull, N("add"), T(frm2a), N("f64"), DSTD, T(neg33), T(abs31), SRC1D, T(di2) },
	{ 0x0400000000000001ull, 0x37c0000000000003ull, N("mul"), T(frm2a), N("f64"), DSTD, SRC1D, T(di2) },
	{ 0x0500000000000001ull, 0x37c0000000000003ull, N("selp"), DST, SRC1, T(i3bi2), T(pnot2d), PSRC3 },
	{ 0x07c0000000000001ull, 0x37c0000000000003ull, N("rshf"), T(high33), N("b32"), DST, SESTART, T(us64_28), SRC1, SRC3, SEEND, T(shfclamp), T(sui2b) }, // d = (s1 >> s2) | (s3 << (32 - s2))
	{ 0x9000000000000001ull, 0xb580000000000003ull, N("set"), N("b32"), DST, T(acout32), T(setit), N("f64"), T(neg2e), T(abs39), SRC1D, DIMM, T(setlop3) },
	{ 0x9400000000000001ull, 0xb4c0000000000003ull, N("fma"), T(ftz38), T(fmz39), T(sat35), T(frm36), N("f32"), DST, T(neg33), SRC1, FIMM, T(neg34), SRC3 },
	{ 0xa000000000000001ull, 0xb400000000000003ull, T(addop3a), T(sat39), DST, SESTART, N("mul"), T(us32_33), SRC1, T(us32_38), I3BIMM, SEEND, SRC3 },
	{ 0xb200000000000001ull, 0xb780000000000003ull, N("slct"), N("b32"), DST, SRC1, I3BIMM, T(isetit), T(us32_33), SRC3 }, // XXX: check us32_33
	{ 0xb280000000000001ull, 0xb780000000000003ull, N("set"), N("b32"), DST, T(acout32), T(isetit), T(us32_33), SRC1, I3BIMM, T(setlop3) },
	{ 0xb300000000000001ull, 0xb780000000000003ull, N("set"), N("b32"), PDST, PDSTN, T(isetit), T(us32_33), SRC1, I3BIMM, T(setlop3) },
	{ 0xb400000000000001ull, 0xb780000000000003ull, N("set"), PDST, PDSTN, T(setit), N("f64"), T(neg2e), T(abs9), SRC1D, DIMM, T(setlop3) },
	{ 0xb500000000000001ull, 0xb780000000000003ull, N("slct"), T(ftz32), N("b32"), DST, SRC1, FIMM, T(setit), N("f32"), SRC3 },
	{ 0xb580000000000001ull, 0xb780000000000003ull, N("set"), T(ftz32), PDST, PDSTN, T(setit), N("f32"), T(neg2e), T(abs9), SRC1, FIMM, T(setlop3) },
	{ 0xb700000000000001ull, 0xb7c0000000000003ull, N("out"), T(emit), T(restart), DST, SRC1, I3BIMM },
	{ 0xb740000000000001ull, 0xb7c0000000000003ull, N("sad"), T(us32_33), DST, SRC1, I3BIMM, SRC3 },
	{ 0xb780000000000001ull, 0xb7c0000000000003ull, N("ins"), N("b32"), DST, SRC1, I3BIMM, SRC3 },
	{ 0x2000000000000001ull, 0x3fc0000000000003ull, N("tex"), T(texm), T(lodt), T(texoff), T(texf), TDST, T(text), T(tconst), T(texsrc1), T(texsrc2) },
	{ 0x37c0000000000001ull, 0x37c0000000000003ull, N("lshf"), T(high33), N("b32"), DST, SESTART, T(us64_28), SRC1, SRC3, SEEND, T(shfclamp), T(sui2a) }, // d = (s3 << s2) | (s1 >> (32 - s2))
	{ 0xc000000000000001ull, 0xffc0000000000003ull, N("ext"), T(rev2b), T(us32_33), DST, SRC1, I3BIMM},
	{ 0xb600000000000001ull, 0xb7c0000000000003ull, N("prmt"), T(prmtmod), N("b32"), DST, SRC1, SRC3, I3BIMM},
	{ 0xc540000000001401ull, 0xf7c0000000003c03ull, N("cvt"), T(sat35), T(rint), N("f16"), DST, N("f16"), T(neg30), T(abs34), I3BIMM },
	{ 0xc540000000001801ull, 0xf7c0000000003c03ull, N("cvt"), T(ftz2f), T(sat35), N("f32"), DST, N("f16"), T(neg30), T(abs34), I3BIMM },
	{ 0xc540000000002401ull, 0xf7c0000000003c03ull, N("cvt"), T(ftz2f), T(sat35), T(frm2a), N("f16"), DST, N("f32"), T(neg30), T(abs34), FIMM },
	{ 0xc540000000002801ull, 0xf7c0000000003c03ull, N("cvt"), T(ftz2f), T(sat35), T(rint), N("f32"), DST, N("f32"), T(neg30), T(abs34), FIMM },
	{ 0xc540000000002c01ull, 0xf7c0000000003c03ull, N("cvt"), N("f64"), DSTD, N("f32"), T(neg30), T(abs34), FIMM }, // XXX: do ftz, sat work here ?
	{ 0xc540000000003801ull, 0xf7c0000000003c03ull, N("cvt"), T(frm2a), N("f32"), DST, N("f64"), T(neg30), T(abs34), DIMM }, // XXX: do ftz, sat work here ?
	{ 0xc540000000003c01ull, 0xf7c0000000003c03ull, N("cvt"), T(rint), N("f64"), DSTD, N("f64"), T(neg30), T(abs34), DIMM },
	{ 0xc580000000000001ull, 0xf7c0000000000003ull, N("cvt"), T(ftz2f), T(frmi), T(cvtf2idst), T(neg30), T(abs34), T(cvtf2isrci) },
	{ 0xc5c0000000000001ull, 0xf7c0000000000003ull, N("cvt"), T(frm2a), T(cvti2fdst), T(neg30), T(abs34), T(cvti2fsrci) },
	{ 0xc600000000000001ull, 0xf7c0000000000003ull, N("cvt"), T(sat35), T(cvti2idst), T(neg30), T(abs34), T(cvti2isrci) },
	{ 0x0, 0x0, DST, SRC1, SRC2, SRC3, I3BIMM, LIMM, DIMM, FIMM, SHCNT, SHCNL, OOPS },
};

static struct insn tabp[] = {
	{ 0x001c0000, 0x003c0000 },
	{ 0x003c0000, 0x003c0000, N("never") },
	{ 0x00000000, 0x00200000, PRED },
	{ 0x00200000, 0x00200000, SESTART, N("not"), PRED, SEEND },
	{ 0, 0, OOPS },
};

F1(lim, 0x8, N("lim"))
F1(brawarp, 0x9, N("allwarp"))
F1(join, 0x16, N("join"))

static struct insn tabc[] = {
	{ 0x0000000000000000ull, 0xff80000000000000ull, N("bpt"), T(bpt), BPTARG },
	{ 0x0800000000000000ull, 0xfc00000000000000ull, N("sched"), SCHED(0), SCHED(1), SCHED(2), SCHED(3), SCHED(4), SCHED(5), SCHED(6) },
	{ 0x1080000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("bra"), T(lim), T(brawarp), N("abs"), ACTARG },
	{ 0x1100000000000000ull, 0xff80000000000000ull, N("call"), T(lim), N("abs"), ACTARG },
	{ 0x1200000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("bra"), T(lim), T(brawarp), BTARG },
	{ 0x1300000000000000ull, 0xff80000000000000ull, N("call"), T(lim), CTARG },
	{ 0x1380000000000000ull, 0xff80000000000000ull, N("preret"), T(lim), BTARG },
	{ 0x1400000000000000ull, 0xff80000000000000ull, N("prelongjmp"), BTARG },
	{ 0x1480000000000000ull, 0xff80000000000000ull, N("joinat"), BTARG },
	{ 0x1500000000000000ull, 0xff80000000000000ull, N("prebrk"), BTARG },
	{ 0x1580000000000000ull, 0xff80000000000000ull, N("precont"), BTARG },
	{ 0x1800000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("exit") },
	{ 0x1880000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("longjmp") },
	{ 0x1900000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("ret") },
	{ 0x1980000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("discard") },
	{ 0x1a00000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("brk") },
	{ 0x1a80000000000000ull, 0xff80000000000000ull, T(p), T(cc), N("cont") },
	{ 0x1b00000000000000ull, 0xff80000000000000ull, N("rtt"), T(rtt) },
	{ 0x1b80000000000000ull, 0xff80000000000000ull, N("quadon") },
	{ 0x1c00000000000000ull, 0xff80000000000000ull, N("quadpop") },

	{ 0x2000000000000000ull, 0xe000000000000000ull, T(join), T(p), T(logop38), N("b32"), DST, T(acout37), T(not3a), SRC1, T(not3b), LIMM, T(acin3c) },
	{ 0x4000000000000000ull, 0xe000000000000000ull, T(join), T(p), N("add"), T(ftz3a), N("f32"), DST, T(acout37), T(neg3b), T(abs39), SRC1, T(neg38), T(abs3c), LIMM },
	{ 0x6000000000000000ull, 0xe000000000000003ull, T(join), T(p), N("fma"), T(ftz38), T(fmz39), T(sat3a), N("f32"), DST, T(acout37), T(neg3b), SRC1, LIMM, T(neg3c), DST },
	{ 0xa000000000000000ull, 0xe000000000000000ull, T(join), T(p), N("add"), N("b32"), DST, T(acout37), SESTART, N("shl"), SRC1, SHCNL, SEEND, LIMM},

	{ 0xc000000000000000ull, 0xe000000000000000ull, T(join), T(p), N("ld"), T(ldstt), T(ldstd), T(lcop), T(gmem) },
	{ 0xe000000000000000ull, 0xe000000000000000ull, T(join), T(p), N("st"), T(ldstt), T(scop), T(gmem), T(ldstd) },

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
	.prep = gk110_prep,
};
