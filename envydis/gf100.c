/*
 * Copyright (C) 2009-2011 Marcelina Kościelnicka <mwk@0x04.net>
 * Copyright (C) 2010 Witold Waligóra <witold.waligora@gmail.com>
 * Copyright (C) 2010-2011 Christoph Bumiller <e0425955@student.tuwien.ac.at>
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

#define F_GF100 1
#define F_GK104 2

/*
 * Registers:
 *
 *  - $r0-$r62: Normal, usable 32-bit regs. Allocated just like on tesla.
 *    Grouped into $r0d-$r60d for 64-bit quantities like doubles, into
 *    $r0q-$r56q for 128-bit quantities. There are no half-regs.
 *  - $r63: Bit bucket on write, 0 on read.
 *  - $p0-$p6: 1-bit predicate registers, usable.
 *  - $p7: Always-true predicate.
 *  - $c: Condition code register, like G80 $cX registers. Has zero, sign,
 *    carry, overflow bits, in that order.
 *  - $flags: A meta-register consisting of $p0-$p6 at bits 0-6 and $c
 *    at bits 12-15.
 */

/*
 * Code target field
 */


static struct rbitfield ctargoff = { { 26, 24 }, RBF_SIGNED, .pcrel = 1, .addend = 8};
static struct rbitfield btargsoff = { { 0xe, 14 }, RBF_SIGNED, 2, .pcrel = 1, .addend = 4};
static struct rbitfield ctargsoff = { { 6, 22 }, RBF_SIGNED, 2, .pcrel = 1, .addend = 4};
static struct rbitfield actargoff = { 26, 32 };
#define BTARG atombtarg, &ctargoff
#define CTARG atomctarg, &ctargoff
#define NTARG atomrimm, &ctargoff
#define ABTARG atombtarg, &actargoff
#define ACTARG atomctarg, &actargoff
#define ANTARG atomrimm, &actargoff
#define BTARGS atombtarg, &btargsoff
#define CTARGS atomctarg, &ctargsoff

/*
 * Misc number fields
 */

static struct rbitfield pmoff = { 0x1a, 16 };
static struct rbitfield tcntoff = { 0x1a, 12 };
static struct rbitfield bptoff = { 0x1a, 20 };
static struct rbitfield bptsoff = { 7, 20 };
static struct rbitfield immoff = { { 0x1a, 20 }, RBF_SIGNED };
static struct rbitfield fimmoff = { { 0x1a, 20 }, RBF_UNSIGNED, 12 };
static struct rbitfield dimmoff = { { 0x1a, 20 }, RBF_UNSIGNED, 44 };
static struct rbitfield limmoff = { { 0x1a, 32 }, .wrapok = 1 };
static struct rbitfield vimmoff = { 0x1a, 16 };
static struct rbitfield v4immoff = { 0x1a, 8 };
static struct rbitfield suimmoff = { { 0x31, 6 }, RBF_SIGNED };
static struct rbitfield immsoff = { { 0x14, 12 }, RBF_SIGNED };
static struct rbitfield fimmsoff = { { 0x14, 12 }, RBF_UNSIGNED, 20 };
static struct rbitfield simmsoff = { { 0x1a, 6, 8, 2 }, RBF_SIGNED };
#define PM atomrimm, &pmoff
#define TCNT atomrimm, &tcntoff
#define BPT atomrimm, &bptoff
#define BPTS atomrimm, &bptsoff
#define IMM atomrimm, &immoff
#define FIMM atomrimm, &fimmoff
#define DIMM atomrimm, &dimmoff
#define LIMM atomrimm, &limmoff
#define VIMM atomrimm, &vimmoff
#define V4IMM atomrimm, &v4immoff
#define SUIMM atomrimm, &suimmoff
#define IMMS atomrimm, &immsoff
#define FIMMS atomrimm, &fimmsoff
#define SIMMS atomrimm, &simmsoff

static struct bitfield baroff = { 0x14, 4 };
static struct bitfield shcntoff = { 5, 5 };
static struct bitfield shcntsoff = { 0x1a, 5 };
static struct bitfield texbaroff = { 0x1a, 6 };
static struct bitfield bnumoff = { 0x37, 2 };
static struct bitfield hnumoff = { 0x38, 1 };
static struct bitfield shfloff1 = { 0x1a, 5 };
static struct bitfield shfloff2 = { 0x2a, 13 };
static struct bitfield schedval0 = { 0x04, 8 };
static struct bitfield schedval1 = { 0x0c, 8 };
static struct bitfield schedval2 = { 0x14, 8 };
static struct bitfield schedval3 = { 0x1c, 8 };
static struct bitfield schedval4 = { 0x24, 8 };
static struct bitfield schedval5 = { 0x2c, 8 };
static struct bitfield schedval6 = { 0x34, 8 };
#define BAR atomimm, &baroff
#define SHCNT atomimm, &shcntoff
#define SHCNTS atomimm, &shcntsoff
#define BNUM atomimm, &bnumoff
#define HNUM atomimm, &hnumoff
#define TEXBARIMM atomimm, &texbaroff
#define SHFLIMM1 atomimm, &shfloff1
#define SHFLIMM2 atomimm, &shfloff2
#define SCHED(n) atomimm, &schedval##n

/*
 * Register fields
 */

static struct sreg sreg_sr[] = {
	{ 0, "laneid" },
	{ 2, "nphysid" }, // bits 8-14: nwarpid, bits 20-28: nsmid
	{ 3, "physid" }, // bits 8-12: warpid, bits 20-28: smid
	{ 4, "pm0" },
	{ 5, "pm1" },
	{ 6, "pm2" },
	{ 7, "pm3" },
	{ 8, "pm4" },
	{ 9, "pm5" },
	{ 0xa, "pm6" },
	{ 0xb, "pm7" },
	{ 0x10, "vtxcnt" }, // gl_PatchVerticesIn
	{ 0x11, "invoc" }, // gl_InvocationID
	{ 0x12, "ydir" }, // f32 +-1
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
	{ 0x30, "swinbase" },	// the address in g[] space where s[] is.
	{ 0x31, "swinsz" },
	{ 0x32, "smemsz" },
	{ 0x33, "smembanks" },
	{ 0x34, "lwinbase" },	// the address in g[] space where l[] is.
	{ 0x35, "lwinsz" },
	{ 0x36, "lpossz" }, // size of positive l[]
	{ 0x37, "lnegstart" }, // min valid address address of negative l[]
	{ 0x38, "lanemask_eq" }, // I have no idea what these do, but ptxas eats them just fine.
	{ 0x39, "lanemask_lt" },
	{ 0x3a, "lanemask_le" },
	{ 0x3b, "lanemask_gt" },
	{ 0x3c, "lanemask_ge" },
	{ 0x40, "trapstat" },
	{ 0x42, "warperr" },
	{ 0x50, "clock" }, // XXX some weird shift happening here.
	{ 0x51, "clockhi" },
	{ -1 },
};
static struct sreg reg_sr[] = {
	{ 63, 0, SR_ZERO },
	{ -1 },
};
static struct sreg pred_sr[] = {
	{ 7, 0, SR_ONE },
	{ -1 },
};

static struct bitfield dst_bf = { 0xe, 6 };
static struct bitfield src1_bf = { 0x14, 6 };
static struct bitfield src2_bf = { 0x1a, 6 };
static struct bitfield src3_bf = { 0x31, 6 };
static struct bitfield src3s_bf = { 8, 6 };
static struct bitfield dst2_bf = { 0x2b, 6 };
static struct bitfield psrc1_bf = { 0x14, 3 };
static struct bitfield psrc2_bf = { 0x1a, 3 };
static struct bitfield psrc3_bf = { 0x31, 3 };
static struct bitfield pred_bf = { 0xa, 3 };
static struct bitfield pdst_bf = { 0x11, 3 };
static struct bitfield pdstn_bf = { 0x0e, 3 };
static struct bitfield pdst2_bf = { 0x36, 3 };
static struct bitfield pdst3_bf = { 0x35, 3 }; // ...the hell?
static struct bitfield pdst4_bf = { 0x32, 3 }; // yay.
static struct bitfield pdst5_bf = { 0x37, 3 }; // erm.
static struct bitfield pdstl_bf = { 8, 2, 0x3a, 1 }; // argh...
static struct bitfield tex_bf = { 0x20, 8 };
static struct rbitfield texsoff_bf = { { 0x20, 11 }, RBF_SIGNED };
static struct bitfield samp_bf = { 0x28, 5 };
static struct bitfield surf_bf = { 0x1a, 3 };
static struct bitfield sreg_bf = { 0x1a, 8 };
static struct bitfield sregs_bf = { 0x14, 8 };
static struct bitfield lduld_dst2_bf = { 0x20, 6 };

static struct reg dst_r = { &dst_bf, "r", .specials = reg_sr };
static struct reg dstd_r = { &dst_bf, "r", "d" };
static struct reg dstq_r = { &dst_bf, "r", "q" };
static struct reg src1_r = { &src1_bf, "r", .specials = reg_sr };
static struct reg src1d_r = { &src1_bf, "r", "d" };
static struct reg src2_r = { &src2_bf, "r", .specials = reg_sr };
static struct reg src2d_r = { &src2_bf, "r", "d" };
static struct reg src3_r = { &src3_bf, "r", .specials = reg_sr };
static struct reg src3d_r = { &src3_bf, "r", "d" };
static struct reg src3s_r = { &src3s_bf, "r", .specials = reg_sr };
static struct reg dst2_r = { &dst2_bf, "r", .specials = reg_sr };
static struct reg dst2d_r = { &dst2_bf, "r", "d" };
static struct reg psrc1_r = { &psrc1_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg psrc2_r = { &psrc2_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg psrc3_r = { &psrc3_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pred_r = { &pred_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdst_r = { &pdst_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdstn_r = { &pdstn_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdst2_r = { &pdst2_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdst3_r = { &pdst3_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdst4_r = { &pdst4_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdst5_r = { &pdst5_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg pdstl_r = { &pdstl_bf, "p", .specials = pred_sr, .cool = 1 };
static struct reg tex_r = { &tex_bf, "t", .cool = 1 };
static struct reg samp_r = { &samp_bf, "s", .cool = 1 };
static struct reg surf_r = { &surf_bf, "g", .cool = 1 };
static struct reg cc_r = { 0, "c", .cool = 1 };
static struct reg flags_r = { 0, "flags", .cool = 1 };
static struct reg sreg_r = { &sreg_bf, "sr", .specials = sreg_sr, .always_special = 1 };
static struct reg sregs_r = { &sregs_bf, "sr", .specials = sreg_sr, .always_special = 1 };
static struct reg lduld_dst2_r = { &lduld_dst2_bf, "r" };
static struct reg lduld_dst2d_r = { &lduld_dst2_bf, "r", "d" };
static struct reg lduld_dst2q_r = { &lduld_dst2_bf, "r", "q" };

#define DST atomreg, &dst_r
#define DSTD atomreg, &dstd_r
#define DSTQ atomreg, &dstq_r
#define SRC1 atomreg, &src1_r
#define SRC1D atomreg, &src1d_r
#define PSRC1 atomreg, &psrc1_r
#define SRC2 atomreg, &src2_r
#define SRC2D atomreg, &src2d_r
#define PSRC2 atomreg, &psrc2_r
#define SRC3 atomreg, &src3_r
#define SRC3S atomreg, &src3s_r
#define SRC3D atomreg, &src3d_r
#define PSRC3 atomreg, &psrc3_r
#define DST2 atomreg, &dst2_r
#define DST2D atomreg, &dst2d_r
#define PRED atomreg, &pred_r
#define PDST atomreg, &pdst_r
#define PDSTN atomreg, &pdstn_r
#define PDST2 atomreg, &pdst2_r
#define PDST3 atomreg, &pdst3_r
#define PDST4 atomreg, &pdst4_r
#define PDST5 atomreg, &pdst5_r
#define PDSTL atomreg, &pdstl_r
#define TEX atomreg, &tex_r
#define SAMP atomreg, &samp_r
#define SURF atomreg, &surf_r
#define CC atomreg, &cc_r
#define SREG atomreg, &sreg_r
#define SREGS atomreg, &sregs_r
#define LDULD_DST2 atomreg, &lduld_dst2_r
#define LDULD_DST2D atomreg, &lduld_dst2d_r
#define LDULD_DST2Q atomreg, &lduld_dst2q_r
#define FLAGS atomreg, &flags_r
#define TEXIMM atomimm, &texsoff_bf

static struct bitfield tdst_mask = { 0x2e, 4 };
static struct bitfield sustpsrc_gf100_mask = { 0x31, 4 };
static struct bitfield sustpsrc_gk104_mask = { 0x36, 4 };
static struct bitfield cnt0 = { .addend = 0 };
static struct bitfield cnt1 = { .addend = 1 };
static struct bitfield cnt2 = { .addend = 2 };
static struct bitfield cnt3 = { .addend = 3 };
static struct bitfield cnt4 = { .addend = 4 };
static struct bitfield saddr_cnt = { { 0x2c, 2 }, .addend = 1 };
static struct bitfield asrc_cnt = { { 5, 2 }, .addend = 1 };
static struct bitfield asrcs_cnt = { { 6, 2 }, .addend = 1 };
static struct vec tdst_v = { "r", &dst_bf, &cnt4, &tdst_mask };
static struct vec sustpsrc_gf100_v = { "r", &dst_bf, &cnt4, &sustpsrc_gf100_mask };
static struct vec sustpsrc_gk104_v = { "r", &dst_bf, &cnt4, &sustpsrc_gk104_mask };
static struct vec tsrc11_v = { "r", &src1_bf, &cnt1, 0 };
static struct vec tsrc12_v = { "r", &src1_bf, &cnt2, 0 };
static struct vec tsrc13_v = { "r", &src1_bf, &cnt3, 0 };
static struct vec tsrc14_v = { "r", &src1_bf, &cnt4, 0 };
static struct vec tsrc20_v = { "r", &src2_bf, &cnt0, 0 };
static struct vec tsrc21_v = { "r", &src2_bf, &cnt1, 0 };
static struct vec tsrc22_v = { "r", &src2_bf, &cnt2, 0 };
static struct vec tsrc23_v = { "r", &src2_bf, &cnt3, 0 };
static struct vec tsrc24_v = { "r", &src2_bf, &cnt4, 0 };
static struct vec saddr_v = { "r", &src1_bf, &saddr_cnt, 0 };
static struct vec asrc_v = { "r", &src2_bf, &asrc_cnt, 0 };
static struct vec adst_v = { "r", &dst_bf, &asrc_cnt, 0 };
static struct vec asrcs_v = { "r", &src2_bf, &asrcs_cnt, 0 };
static struct vec adsts_v = { "r", &dst_bf, &asrcs_cnt, 0 };
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
#define SUSTPSRC_GF100 atomvec, &sustpsrc_gf100_v
#define SUSTPSRC_GK104 atomvec, &sustpsrc_gk104_v
#define SADDR atomvec, &saddr_v
#define ASRC atomvec, &asrc_v
#define ADST atomvec, &adst_v
#define ASRCS atomvec, &asrcs_v
#define ADSTS atomvec, &adsts_v

static int ignpred[] = { 0xa, 4 };
#define IGNPRED atomign, ignpred

/*
 * Memory fields
 */

static struct rbitfield gmem_imm = { { 0x1a, 32 }, RBF_SIGNED };
static struct rbitfield gcmem_imm = { { 0x1c, 30 }, RBF_SIGNED, 2 };
static struct rbitfield gamem_imm = { { 0x1a, 17, 0x37, 3 }, RBF_SIGNED };
static struct rbitfield slmem_imm = { { 0x1a, 24 }, RBF_SIGNED };
static struct rbitfield cmem_imm = { 0x1c, 14, .shr = 2 };
static struct rbitfield fcmem_imm = { { 0x1a, 16 }, RBF_SIGNED };
static struct rbitfield sucmem_imm = { { 0x1a, 14 }, RBF_UNSIGNED, 2 };
static struct rbitfield amem_imm = { { 0x20, 10 } };
static struct rbitfield img_imm = { { 0x1a, 3 }, RBF_UNSIGNED };
static struct rbitfield pix_imm = { { 0x1a, 8 }, RBF_SIGNED };
static struct rbitfield as1mem_imm = { { 8, 2, 0x14, 6 }, RBF_UNSIGNED, 2 };
static struct rbitfield as2mem_imm = { { 8, 2, 0x1a, 6 }, RBF_UNSIGNED, 2 };
static struct rbitfield ss2mem_imm = { { 8, 2, 0x1a, 6 }, RBF_SIGNED, 2 };
static struct rbitfield ss3mem_imm = { { 8, 2, 0x1a, 6 }, RBF_SIGNED, 3 };
static struct bitfield cmem_idx = { 0x2a, 4, 0x1a, 1 };
static struct bitfield fcmem_idx = { 0x2a, 5 };
static struct bitfield fcsmem_idx = { 0x5, 3 };
static struct bitfield sucmem_idx = { 0x28, 4 };
static struct rbitfield sc1mem_imm = { { 0x14, 6 }, RBF_UNSIGNED, 2 };
static struct rbitfield sc2mem_imm = { { 0x1a, 6 }, RBF_UNSIGNED, 2 };
static struct rbitfield sc3mem_imm = { { 8, 6 }, RBF_UNSIGNED, 2 };
static struct bitfield sc0mem_idx = { 0 };
static struct bitfield sc1mem_idx = { .addend = 1 };
static struct bitfield sc16mem_idx = { .addend = 16 };
static struct rbitfield lduld_imm = { { 0x2b, 10 }, RBF_SIGNED };
static struct rbitfield lduld2_imm = { { 5, 5, 0x26, 5 }, RBF_SIGNED };
static struct rbitfield ldulds1_imm = { { 0x2b, 10 }, RBF_SIGNED, 1 };
static struct rbitfield lduld2s1_imm = { { 5, 5, 0x26, 5 }, RBF_SIGNED, 1 };
static struct rbitfield ldulds2_imm = { { 0x2b, 10 }, RBF_SIGNED, 2 };
static struct rbitfield lduld2s2_imm = { { 5, 5, 0x26, 5 }, RBF_SIGNED, 2 };
static struct rbitfield ldulds3_imm = { { 0x2b, 10 }, RBF_SIGNED, 3 };
static struct rbitfield lduld2s3_imm = { { 5, 5, 0x26, 5 }, RBF_SIGNED, 3 };
static struct rbitfield ldulds4_imm = { { 0x2b, 10 }, RBF_SIGNED, 4 };
static struct rbitfield lduld2s4_imm = { { 5, 5, 0x26, 5 }, RBF_SIGNED, 4 };
static struct mem gmem_m = { "g", 0, &src1_r, &gmem_imm };
static struct mem gdmem_m = { "g", 0, &src1d_r, &gmem_imm };
static struct mem gdmemsu_m = { "g", 0, &src1d_r };
static struct mem gamem_m = { "g", 0, &src1_r, &gamem_imm };
static struct mem gadmem_m = { "g", 0, &src1d_r, &gamem_imm };
static struct mem smem_m = { "s", 0, &src1_r, &slmem_imm };
static struct mem lmem_m = { "l", 0, &src1_r, &slmem_imm };
static struct mem fcmem_m = { "c", &fcmem_idx, &src1_r, &fcmem_imm };
static struct mem fcmemnr_m = { "c", &fcmem_idx, 0, &fcmem_imm };
static struct mem fcsmem_m = { "c", &fcsmem_idx, &src1_r, &ss2mem_imm };
static struct mem amem_m = { "a", 0, &src1_r, &amem_imm }; // XXX: wtf?
static struct mem as1mem_m = { "a", 0, 0, &as1mem_imm };
static struct mem as2mem_m = { "a", 0, 0, &as2mem_imm };
static struct mem ls2mem_m = { "l", 0, &src1_r, &ss2mem_imm };
static struct mem ls3mem_m = { "l", 0, &src1_r, &ss3mem_imm };
static struct mem ss2mem_m = { "s", 0, &src1_r, &ss2mem_imm };
static struct mem ss3mem_m = { "s", 0, &src1_r, &ss3mem_imm };
static struct mem gs2mem_m = { "g", 0, &src1_r, &ss2mem_imm };
static struct mem gs3mem_m = { "g", 0, &src1_r, &ss3mem_imm };
static struct mem gds2mem_m = { "g", 0, &src1d_r, &ss2mem_imm };
static struct mem gds3mem_m = { "g", 0, &src1d_r, &ss3mem_imm };
static struct mem cmem_m = { "c", &cmem_idx, 0, &cmem_imm };
static struct mem sucmem_m = { "c", &sucmem_idx, 0, &sucmem_imm };
static struct mem lcmem_m = { "l", 0, &src1_r, &slmem_imm };
static struct mem gcmem_m = { "g", 0, &src1_r, &gcmem_imm };
static struct mem gdcmem_m = { "g", 0, &src1d_r, &gcmem_imm };
static struct mem lduld_gmem1_m = { "g", 0, &src1_r, &lduld_imm };
static struct mem lduld_gdmem1_m = { "g", 0, &src1d_r, &lduld_imm };
static struct mem lduld_smem_m = { "s", 0, &src1_r , &lduld_imm };
static struct mem lduld_gmem2_m = { "g", 0, &src2_r, &lduld2_imm };
static struct mem lduld_gdmem2_m = { "g", 0, &src2d_r, &lduld2_imm };
static struct mem lduld_gmem1s1_m = { "g", 0, &src1_r, &ldulds1_imm };
static struct mem lduld_gdmem1s1_m = { "g", 0, &src1d_r, &ldulds1_imm };
static struct mem lduld_smems1_m = { "s", 0, &src1_r , &ldulds1_imm };
static struct mem lduld_gmem2s1_m = { "g", 0, &src2_r, &lduld2s1_imm };
static struct mem lduld_gdmem2s1_m = { "g", 0, &src2d_r, &lduld2s1_imm };
static struct mem lduld_gmem1s2_m = { "g", 0, &src1_r, &ldulds2_imm };
static struct mem lduld_gdmem1s2_m = { "g", 0, &src1d_r, &ldulds2_imm };
static struct mem lduld_smems2_m = { "s", 0, &src1_r , &ldulds2_imm };
static struct mem lduld_gmem2s2_m = { "g", 0, &src2_r, &lduld2s2_imm };
static struct mem lduld_gdmem2s2_m = { "g", 0, &src2d_r, &lduld2s2_imm };
static struct mem lduld_gmem1s3_m = { "g", 0, &src1_r, &ldulds3_imm };
static struct mem lduld_gdmem1s3_m = { "g", 0, &src1d_r, &ldulds3_imm };
static struct mem lduld_smems3_m = { "s", 0, &src1_r , &ldulds3_imm };
static struct mem lduld_gmem2s3_m = { "g", 0, &src2_r, &lduld2s3_imm };
static struct mem lduld_gdmem2s3_m = { "g", 0, &src2d_r, &lduld2s3_imm };
static struct mem lduld_gmem1s4_m = { "g", 0, &src1_r, &ldulds4_imm };
static struct mem lduld_gdmem1s4_m = { "g", 0, &src1d_r, &ldulds4_imm };
static struct mem lduld_smems4_m = { "s", 0, &src1_r , &ldulds4_imm };
static struct mem lduld_gmem2s4_m = { "g", 0, &src2_r, &lduld2s4_imm };
static struct mem lduld_gdmem2s4_m = { "g", 0, &src2d_r, &lduld2s4_imm };
static struct mem imgi_m = { "img", 0, 0, &img_imm };
static struct mem imgr_m = { "img", 0, &src2_r, 0 };
// vertex base address (for tessellation and geometry programs)
static struct mem vba_m = { "p", 0, &src1_r, &fcmem_imm };
static struct mem pix_m = { "pix", 0, &src1_r, &pix_imm };
static struct mem sc1_0mem_m = { "c", &sc0mem_idx, 0, &sc1mem_imm };
static struct mem sc1_1mem_m = { "c", &sc1mem_idx, 0, &sc1mem_imm };
static struct mem sc1_16mem_m = { "c", &sc16mem_idx, 0, &sc1mem_imm };
static struct mem sc2_0mem_m = { "c", &sc0mem_idx, 0, &sc2mem_imm };
static struct mem sc2_1mem_m = { "c", &sc1mem_idx, 0, &sc2mem_imm };
static struct mem sc2_16mem_m = { "c", &sc16mem_idx, 0, &sc2mem_imm };
static struct mem sc3_0mem_m = { "c", &sc0mem_idx, 0, &sc3mem_imm };
static struct mem sc3_1mem_m = { "c", &sc1mem_idx, 0, &sc3mem_imm };
static struct mem sc3_16mem_m = { "c", &sc16mem_idx, 0, &sc3mem_imm };
#define GLOBAL atommem, &gmem_m
#define GLOBALD atommem, &gdmem_m
#define GLOBALDSU atommem, &gdmemsu_m
#define GATOM atommem, &gamem_m
#define GATOMD atommem, &gadmem_m
#define SHARED atommem, &smem_m
#define LOCAL atommem, &lmem_m
#define FCONST atommem, &fcmem_m
#define FCONSTNR atommem, &fcmemnr_m
#define FCONSTS atommem, &fcsmem_m
#define ATTR atommem, &amem_m
#define ATTRS1 atommem, &as1mem_m
#define ATTRS2 atommem, &as2mem_m
#define LOCALS2 atommem, &ls2mem_m
#define LOCALS3 atommem, &ls3mem_m
#define SHAREDS2 atommem, &ss2mem_m
#define SHAREDS3 atommem, &ss3mem_m
#define GLOBALS2 atommem, &gs2mem_m
#define GLOBALS3 atommem, &gs3mem_m
#define GLOBALDS2 atommem, &gds2mem_m
#define GLOBALDS3 atommem, &gds3mem_m
#define CONST atommem, &cmem_m
#define SUCONST atommem, &sucmem_m
#define VBASRC atommem, &vba_m
#define PIX atommem, &pix_m
#define LCMEM atommem, &lcmem_m
#define GCMEM atommem, &gcmem_m
#define GDCMEM atommem, &gdcmem_m
#define LDULD_GLOBAL1 atommem, &lduld_gmem1_m
#define LDULD_GLOBALD1 atommem, &lduld_gdmem1_m
#define LDULD_GLOBAL2 atommem, &lduld_gmem2_m
#define LDULD_GLOBALD2 atommem, &lduld_gdmem2_m
#define LDULD_SHARED atommem, &lduld_smem_m
#define LDULD_GLOBAL1S1 atommem, &lduld_gmem1s1_m
#define LDULD_GLOBALD1S1 atommem, &lduld_gdmem1s1_m
#define LDULD_GLOBAL2S1 atommem, &lduld_gmem2s1_m
#define LDULD_GLOBALD2S1 atommem, &lduld_gdmem2s1_m
#define LDULD_SHAREDS1 atommem, &lduld_smems1_m
#define LDULD_GLOBAL1S2 atommem, &lduld_gmem1s2_m
#define LDULD_GLOBALD1S2 atommem, &lduld_gdmem1s2_m
#define LDULD_GLOBAL2S2 atommem, &lduld_gmem2s2_m
#define LDULD_GLOBALD2S2 atommem, &lduld_gdmem2s2_m
#define LDULD_SHAREDS2 atommem, &lduld_smems2_m
#define LDULD_GLOBAL1S3 atommem, &lduld_gmem1s3_m
#define LDULD_GLOBALD1S3 atommem, &lduld_gdmem1s3_m
#define LDULD_GLOBAL2S3 atommem, &lduld_gmem2s3_m
#define LDULD_GLOBALD2S3 atommem, &lduld_gdmem2s3_m
#define LDULD_SHAREDS3 atommem, &lduld_smems3_m
#define LDULD_GLOBAL1S4 atommem, &lduld_gmem1s4_m
#define LDULD_GLOBALD1S4 atommem, &lduld_gdmem1s4_m
#define LDULD_GLOBAL2S4 atommem, &lduld_gmem2s4_m
#define LDULD_GLOBALD2S4 atommem, &lduld_gdmem2s4_m
#define LDULD_SHAREDS4 atommem, &lduld_smems4_m
#define SCONST1_0 atommem, &sc1_0mem_m
#define SCONST1_1 atommem, &sc1_1mem_m
#define SCONST1_16 atommem, &sc1_16mem_m
#define SCONST2_0 atommem, &sc2_0mem_m
#define SCONST2_1 atommem, &sc2_1mem_m
#define SCONST2_16 atommem, &sc2_16mem_m
#define SCONST3_0 atommem, &sc3_0mem_m
#define SCONST3_1 atommem, &sc3_1mem_m
#define SCONST3_16 atommem, &sc3_16mem_m
#define IMGI atommem, &imgi_m
#define IMGR atommem, &imgr_m

/*
 * The instructions
 */

F(gmem, 0x3a, GLOBAL, GLOBALD)
F(gamem, 0x3a, GATOM, GATOMD)
F(gcmem, 0x3a, GCMEM, GDCMEM)
F(lduld_gmem1, 0x3b, LDULD_GLOBAL1, LDULD_GLOBALD1)
F(lduld_gmem2, 0x3a, LDULD_GLOBAL2, LDULD_GLOBALD2)
F(lduld_gmem1s1, 0x3b, LDULD_GLOBAL1S1, LDULD_GLOBALD1S1)
F(lduld_gmem2s1, 0x3a, LDULD_GLOBAL2S1, LDULD_GLOBALD2S1)
F(lduld_gmem1s2, 0x3b, LDULD_GLOBAL1S2, LDULD_GLOBALD1S2)
F(lduld_gmem2s2, 0x3a, LDULD_GLOBAL2S2, LDULD_GLOBALD2S2)
F(lduld_gmem1s3, 0x3b, LDULD_GLOBAL1S3, LDULD_GLOBALD1S3)
F(lduld_gmem2s3, 0x3a, LDULD_GLOBAL2S3, LDULD_GLOBALD2S3)
F(lduld_gmem1s4, 0x3b, LDULD_GLOBAL1S4, LDULD_GLOBALD1S4)
F(lduld_gmem2s4, 0x3a, LDULD_GLOBAL2S4, LDULD_GLOBALD2S4)

static struct insn tabldstt[] = {
	{ 0x00, 0xe0, N("u8") },
	{ 0x20, 0xe0, N("s8") },
	{ 0x40, 0xe0, N("u16") },
	{ 0x60, 0xe0, N("s16") },
	{ 0x80, 0xe0, N("b32") },
	{ 0xa0, 0xe0, N("b64") },
	{ 0xc0, 0xe0, N("b128") },
	{ 0, 0, OOPS },
};

static struct insn tabldstd[] = {
	{ 0x00, 0xe0, DST },
	{ 0x20, 0xe0, DST },
	{ 0x40, 0xe0, DST },
	{ 0x60, 0xe0, DST },
	{ 0x80, 0xe0, DST },
	{ 0xa0, 0xe0, DSTD },
	{ 0xc0, 0xe0, DSTQ },
	{ 0, 0, OOPS, DST },
};

static struct insn tabldvf[] = {
	{ 0x60, 0x60, N("b128") },
	{ 0x40, 0x60, N("b96") },
	{ 0x20, 0x60, N("b64") },
	{ 0x00, 0x60, N("b32") },
	{ 0, 0, OOPS },
};

static struct insn tabldvfs[] = {
	{ 0xc0, 0xc0, N("b128") },
	{ 0x80, 0xc0, N("b96") },
	{ 0x40, 0xc0, N("b64") },
	{ 0x00, 0xc0, N("b32") },
	{ 0, 0, OOPS },
};

static struct insn tabldulddst1[] = {
	{ 0x0000000000000000ull, 0x03e0000000000000ull, N("u8"), DST },
	{ 0x0020000000000000ull, 0x03e0000000000000ull, N("s8"), DST },
	{ 0x0040000000000000ull, 0x03e0000000000000ull, N("u16"), DST },
	{ 0x0060000000000000ull, 0x03e0000000000000ull, N("s16"), DST },
	{ 0x0080000000000000ull, 0x03e0000000000000ull, N("b32"), DST },
	{ 0x00a0000000000000ull, 0x03e0000000000000ull, N("u8"), DST },
	{ 0x00c0000000000000ull, 0x03e0000000000000ull, N("s8"), DST },
	{ 0x00e0000000000000ull, 0x03e0000000000000ull, N("u16"), DST },
	{ 0x0100000000000000ull, 0x03e0000000000000ull, N("s16"), DST },
	{ 0x0120000000000000ull, 0x03e0000000000000ull, N("b32"), DST },
	{ 0x0140000000000000ull, 0x03e0000000000000ull, N("u8"), DST },
	{ 0x0160000000000000ull, 0x03e0000000000000ull, N("s8"), DST },
	{ 0x0180000000000000ull, 0x03e0000000000000ull, N("u16"), DST },
	{ 0x01a0000000000000ull, 0x03e0000000000000ull, N("s16"), DST },
	{ 0x01c0000000000000ull, 0x03e0000000000000ull, N("b32"), DST },
	{ 0x01e0000000000000ull, 0x03e0000000000000ull, N("u8"), DST },
	{ 0x0200000000000000ull, 0x03e0000000000000ull, N("s8"), DST },
	{ 0x0220000000000000ull, 0x03e0000000000000ull, N("u16"), DST },
	{ 0x0240000000000000ull, 0x03e0000000000000ull, N("s16"), DST },
	{ 0x0260000000000000ull, 0x03e0000000000000ull, N("b32"), DST },
	{ 0x0280000000000000ull, 0x03e0000000000000ull, N("u8"), DST },
	{ 0x02a0000000000000ull, 0x03e0000000000000ull, N("s8"), DST },
	{ 0x02c0000000000000ull, 0x03e0000000000000ull, N("u16"), DST },
	{ 0x02e0000000000000ull, 0x03e0000000000000ull, N("s16"), DST },
	{ 0x0300000000000000ull, 0x03e0000000000000ull, N("b32"), DST },
	{ 0x0320000000000000ull, 0x03e0000000000000ull, N("b64"), DSTD },
	{ 0x0340000000000000ull, 0x03e0000000000000ull, N("b128"), DSTQ },
	{ 0x0360000000000000ull, 0x03e0000000000000ull, N("b32"), DST },
	{ 0x0380000000000000ull, 0x03e0000000000000ull, N("b64"), DSTD },
	{ 0x03a0000000000000ull, 0x03e0000000000000ull, N("b128"), DSTQ },
	{ 0x03c0000000000000ull, 0x03e0000000000000ull, N("b32"), DST },
	{ 0x03e0000000000000ull, 0x03e0000000000000ull, N("b64"), DSTD },
	{ 0, 0, OOPS },
};

static struct insn tablduldsrc1g[] = {
	{ 0x0000000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2) },
	{ 0x0020000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2) },
	{ 0x0040000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s1) },
	{ 0x0060000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s1) },
	{ 0x0080000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s2) },
	{ 0x00a0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2) },
	{ 0x00c0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2) },
	{ 0x00e0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s1) },
	{ 0x0100000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s1) },
	{ 0x0120000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s2) },
	{ 0x0140000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2) },
	{ 0x0160000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2) },
	{ 0x0180000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s1) },
	{ 0x01a0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s1) },
	{ 0x01c0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s2) },
	{ 0x01e0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2) },
	{ 0x0200000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2) },
	{ 0x0220000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s1) },
	{ 0x0240000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s1) },
	{ 0x0260000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s2) },
	{ 0x0280000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2) },
	{ 0x02a0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2) },
	{ 0x02c0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s1) },
	{ 0x02e0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s1) },
	{ 0x0300000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s2) },
	{ 0x0320000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s3) },
	{ 0x0340000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s4) },
	{ 0x0360000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s2) },
	{ 0x0380000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s3) },
	{ 0x03a0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s4) },
	{ 0x03c0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s2) },
	{ 0x03e0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem2s3) },
	{ 0, 0, OOPS },
};

static struct insn tabldulddst2[] = {
	{ 0x0000000000000000ull, 0x03e0000000000000ull, N("u8"), LDULD_DST2 },
	{ 0x0020000000000000ull, 0x03e0000000000000ull, N("u8"), LDULD_DST2 },
	{ 0x0040000000000000ull, 0x03e0000000000000ull, N("u8"), LDULD_DST2 },
	{ 0x0060000000000000ull, 0x03e0000000000000ull, N("u8"), LDULD_DST2 },
	{ 0x0080000000000000ull, 0x03e0000000000000ull, N("u8"), LDULD_DST2 },
	{ 0x00a0000000000000ull, 0x03e0000000000000ull, N("s8"), LDULD_DST2 },
	{ 0x00c0000000000000ull, 0x03e0000000000000ull, N("s8"), LDULD_DST2 },
	{ 0x00e0000000000000ull, 0x03e0000000000000ull, N("s8"), LDULD_DST2 },
	{ 0x0100000000000000ull, 0x03e0000000000000ull, N("s8"), LDULD_DST2 },
	{ 0x0120000000000000ull, 0x03e0000000000000ull, N("s8"), LDULD_DST2 },
	{ 0x0140000000000000ull, 0x03e0000000000000ull, N("u16"), LDULD_DST2 },
	{ 0x0160000000000000ull, 0x03e0000000000000ull, N("u16"), LDULD_DST2 },
	{ 0x0180000000000000ull, 0x03e0000000000000ull, N("u16"), LDULD_DST2 },
	{ 0x01a0000000000000ull, 0x03e0000000000000ull, N("u16"), LDULD_DST2 },
	{ 0x01c0000000000000ull, 0x03e0000000000000ull, N("u16"), LDULD_DST2 },
	{ 0x01e0000000000000ull, 0x03e0000000000000ull, N("s16"), LDULD_DST2 },
	{ 0x0200000000000000ull, 0x03e0000000000000ull, N("s16"), LDULD_DST2 },
	{ 0x0220000000000000ull, 0x03e0000000000000ull, N("s16"), LDULD_DST2 },
	{ 0x0240000000000000ull, 0x03e0000000000000ull, N("s16"), LDULD_DST2 },
	{ 0x0260000000000000ull, 0x03e0000000000000ull, N("s16"), LDULD_DST2 },
	{ 0x0280000000000000ull, 0x03e0000000000000ull, N("b32"), LDULD_DST2 },
	{ 0x02a0000000000000ull, 0x03e0000000000000ull, N("b32"), LDULD_DST2 },
	{ 0x02c0000000000000ull, 0x03e0000000000000ull, N("b32"), LDULD_DST2 },
	{ 0x02e0000000000000ull, 0x03e0000000000000ull, N("b32"), LDULD_DST2 },
	{ 0x0300000000000000ull, 0x03e0000000000000ull, N("b32"), LDULD_DST2 },
	{ 0x0320000000000000ull, 0x03e0000000000000ull, N("b32"), LDULD_DST2 },
	{ 0x0340000000000000ull, 0x03e0000000000000ull, N("b32"), LDULD_DST2 },
	{ 0x0360000000000000ull, 0x03e0000000000000ull, N("b64"), LDULD_DST2D },
	{ 0x0380000000000000ull, 0x03e0000000000000ull, N("b64"), LDULD_DST2D },
	{ 0x03a0000000000000ull, 0x03e0000000000000ull, N("b64"), LDULD_DST2D },
	{ 0x03c0000000000000ull, 0x03e0000000000000ull, N("b128"), LDULD_DST2Q },
	{ 0x03e0000000000000ull, 0x03e0000000000000ull, N("b128"), LDULD_DST2Q },
	{ 0, 0, OOPS },
};

static struct insn tablduldsrc2s[] = {
	{ 0x0000000000000000ull, 0x03e0000000000000ull, LDULD_SHARED },
	{ 0x0020000000000000ull, 0x03e0000000000000ull, LDULD_SHARED },
	{ 0x0040000000000000ull, 0x03e0000000000000ull, LDULD_SHARED },
	{ 0x0060000000000000ull, 0x03e0000000000000ull, LDULD_SHARED },
	{ 0x0080000000000000ull, 0x03e0000000000000ull, LDULD_SHARED },
	{ 0x00a0000000000000ull, 0x03e0000000000000ull, LDULD_SHARED },
	{ 0x00c0000000000000ull, 0x03e0000000000000ull, LDULD_SHARED },
	{ 0x00e0000000000000ull, 0x03e0000000000000ull, LDULD_SHARED },
	{ 0x0100000000000000ull, 0x03e0000000000000ull, LDULD_SHARED },
	{ 0x0120000000000000ull, 0x03e0000000000000ull, LDULD_SHARED },
	{ 0x0140000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS1 },
	{ 0x0160000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS1 },
	{ 0x0180000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS1 },
	{ 0x01a0000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS1 },
	{ 0x01c0000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS1 },
	{ 0x01e0000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS1 },
	{ 0x0200000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS1 },
	{ 0x0220000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS1 },
	{ 0x0240000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS1 },
	{ 0x0260000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS1 },
	{ 0x0280000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS2 },
	{ 0x02a0000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS2 },
	{ 0x02c0000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS2 },
	{ 0x02e0000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS2 },
	{ 0x0300000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS2 },
	{ 0x0320000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS2 },
	{ 0x0340000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS2 },
	{ 0x0360000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS3 },
	{ 0x0380000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS3 },
	{ 0x03a0000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS3 },
	{ 0x03c0000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS4 },
	{ 0x03e0000000000000ull, 0x03e0000000000000ull, LDULD_SHAREDS4 },
	{ 0, 0, OOPS },
};

static struct insn tablduldsrc2g[] = {
	{ 0x0000000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1) },
	{ 0x0020000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1) },
	{ 0x0040000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1) },
	{ 0x0060000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1) },
	{ 0x0080000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1) },
	{ 0x00a0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1) },
	{ 0x00c0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1) },
	{ 0x00e0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1) },
	{ 0x0100000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1) },
	{ 0x0120000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1) },
	{ 0x0140000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s1) },
	{ 0x0160000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s1) },
	{ 0x0180000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s1) },
	{ 0x01a0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s1) },
	{ 0x01c0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s1) },
	{ 0x01e0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s1) },
	{ 0x0200000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s1) },
	{ 0x0220000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s1) },
	{ 0x0240000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s1) },
	{ 0x0260000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s1) },
	{ 0x0280000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s2) },
	{ 0x02a0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s2) },
	{ 0x02c0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s2) },
	{ 0x02e0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s2) },
	{ 0x0300000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s2) },
	{ 0x0320000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s2) },
	{ 0x0340000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s2) },
	{ 0x0360000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s3) },
	{ 0x0380000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s3) },
	{ 0x03a0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s3) },
	{ 0x03c0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s4) },
	{ 0x03e0000000000000ull, 0x03e0000000000000ull, T(lduld_gmem1s4) },
	{ 0, 0, OOPS },
};

static struct insn tabimadp1[] = {
	{ 0x0000000000000000ull, 0x0000000000000380ull, N("u32") },
	{ 0x0000000000000080ull, 0x0000000000000380ull, N("s32") },
	{ 0x0000000000000100ull, 0x0000000000000380ull, N("u24") },
	{ 0x0000000000000180ull, 0x0000000000000380ull, N("s24") },
	{ 0x0000000000000200ull, 0x0000000000000380ull, N("u16l") },
	{ 0x0000000000000280ull, 0x0000000000000380ull, N("s16l") },
	{ 0x0000000000000300ull, 0x0000000000000380ull, N("u16h") },
	{ 0x0000000000000380ull, 0x0000000000000380ull, N("s16h") },
	{ 0, 0, OOPS },
};

static struct insn tabimadp2[] = {
	{ 0x0000000000000000ull, 0x0000000000000060ull, N("u24") },
	{ 0x0000000000000020ull, 0x0000000000000060ull, N("s24") },
	{ 0x0000000000000040ull, 0x0000000000000060ull, N("u16l") },
	{ 0x0000000000000060ull, 0x0000000000000060ull, N("s16l") },
	{ 0, 0, OOPS },
};

static struct insn tabimadp3[] = {
	{ 0x0000000000000000ull, 0x01800000000000a0ull, N("u32") },
	{ 0x0000000000000020ull, 0x01800000000000a0ull, N("s32") },
	{ 0x0000000000000080ull, 0x01800000000000a0ull, N("s32") },
	{ 0x00000000000000a0ull, 0x01800000000000a0ull, N("s32") },
	{ 0x0080000000000000ull, 0x01800000000000a0ull, N("u24") },
	{ 0x0080000000000020ull, 0x01800000000000a0ull, N("s24") },
	{ 0x0080000000000080ull, 0x01800000000000a0ull, N("s24") },
	{ 0x00800000000000a0ull, 0x01800000000000a0ull, N("s24") },
	{ 0x0100000000000000ull, 0x01800000000000a0ull, N("u16l") },
	{ 0x0100000000000020ull, 0x01800000000000a0ull, N("s16l") },
	{ 0x0100000000000080ull, 0x01800000000000a0ull, N("s16l") },
	{ 0x01000000000000a0ull, 0x01800000000000a0ull, N("s16l") },
	{ 0, 0, OOPS },
};

static struct insn tabsugty[] = {
	{ 0x0000000000000000ull, 0x0000600000000000ull, N("u32") },
	{ 0x0000200000000000ull, 0x0000600000000000ull, N("s32") },
	{ 0x0000400000000000ull, 0x0000600000000000ull, N("u8") },
	{ 0x0000600000000000ull, 0x0000600000000000ull, N("s8") },
	{ 0, 0, OOPS },
};

static struct insn tabfarm[] = {
	{ 0x0000000000000000ull, 0x0180000000000000ull, N("rn") },
	{ 0x0080000000000000ull, 0x0180000000000000ull, N("rm") },
	{ 0x0100000000000000ull, 0x0180000000000000ull, N("rp") },
	{ 0x0180000000000000ull, 0x0180000000000000ull, N("rz") },
	{ 0, 0, OOPS },
};

static struct insn tabfcrm[] = {
	{ 0x0000000000000000ull, 0x0006000000000000ull, N("rn") },
	{ 0x0002000000000000ull, 0x0006000000000000ull, N("rm") },
	{ 0x0004000000000000ull, 0x0006000000000000ull, N("rp") },
	{ 0x0006000000000000ull, 0x0006000000000000ull, N("rz") },
	{ 0, 0, OOPS },
};

static struct insn tabfcrmi[] = {
	{ 0x0000000000000000ull, 0x0006000000000000ull, N("rni") },
	{ 0x0002000000000000ull, 0x0006000000000000ull, N("rmi") },
	{ 0x0004000000000000ull, 0x0006000000000000ull, N("rpi") },
	{ 0x0006000000000000ull, 0x0006000000000000ull, N("rzi") },
	{ 0, 0, OOPS },
};

static struct insn tabsetit[] = {
	{ 0x0000000000000000ull, 0x0780000000000000ull, N("false") },
	{ 0x0080000000000000ull, 0x0780000000000000ull, N("lt") },
	{ 0x0100000000000000ull, 0x0780000000000000ull, N("eq") },
	{ 0x0180000000000000ull, 0x0780000000000000ull, N("le") },
	{ 0x0200000000000000ull, 0x0780000000000000ull, N("gt") },
	{ 0x0280000000000000ull, 0x0780000000000000ull, N("ne") },
	{ 0x0300000000000000ull, 0x0780000000000000ull, N("ge") },
	{ 0x0380000000000000ull, 0x0780000000000000ull, N("num") },
	{ 0x0400000000000000ull, 0x0780000000000000ull, N("nan") },
	{ 0x0480000000000000ull, 0x0780000000000000ull, N("ltu") },
	{ 0x0500000000000000ull, 0x0780000000000000ull, N("equ") },
	{ 0x0580000000000000ull, 0x0780000000000000ull, N("leu") },
	{ 0x0600000000000000ull, 0x0780000000000000ull, N("gtu") },
	{ 0x0680000000000000ull, 0x0780000000000000ull, N("neu") },
	{ 0x0700000000000000ull, 0x0780000000000000ull, N("geu") },
	{ 0x0780000000000000ull, 0x0780000000000000ull, N("true") },
	{ 0, 0, OOPS },
};

static struct insn tabsetct[] = {
	{ 0x0000000000000000ull, 0x0000000001f00000ull, N("never") },
	{ 0x0000000000100000ull, 0x0000000001f00000ull, N("l") },
	{ 0x0000000000200000ull, 0x0000000001f00000ull, N("e") },
	{ 0x0000000000300000ull, 0x0000000001f00000ull, N("le") },
	{ 0x0000000000400000ull, 0x0000000001f00000ull, N("g") },
	{ 0x0000000000500000ull, 0x0000000001f00000ull, N("lg") },
	{ 0x0000000000600000ull, 0x0000000001f00000ull, N("ge") },
	{ 0x0000000000700000ull, 0x0000000001f00000ull, N("lge") },
	{ 0x0000000000800000ull, 0x0000000001f00000ull, N("u") },
	{ 0x0000000000900000ull, 0x0000000001f00000ull, N("lu") },
	{ 0x0000000000a00000ull, 0x0000000001f00000ull, N("eu") },
	{ 0x0000000000b00000ull, 0x0000000001f00000ull, N("leu") },
	{ 0x0000000000c00000ull, 0x0000000001f00000ull, N("gu") },
	{ 0x0000000000d00000ull, 0x0000000001f00000ull, N("lgu") },
	{ 0x0000000000e00000ull, 0x0000000001f00000ull, N("geu") },
	{ 0x0000000000f00000ull, 0x0000000001f00000ull, N("true") },
	{ 0x0000000001000000ull, 0x0000000001f00000ull, N("no") },
	{ 0x0000000001100000ull, 0x0000000001f00000ull, N("nc") },
	{ 0x0000000001200000ull, 0x0000000001f00000ull, N("ns") },
	{ 0x0000000001300000ull, 0x0000000001f00000ull, N("na") },
	{ 0x0000000001400000ull, 0x0000000001f00000ull, N("a") },
	{ 0x0000000001500000ull, 0x0000000001f00000ull, N("s") },
	{ 0x0000000001600000ull, 0x0000000001f00000ull, N("c") },
	{ 0x0000000001700000ull, 0x0000000001f00000ull, N("o") },
	{ 0, 0, OOPS },
};

static struct insn tabcc[] = {
	{ 0x0000000000000000ull, 0x00000000000003e0ull, N("never"), CC },
	{ 0x0000000000000020ull, 0x00000000000003e0ull, N("l"), CC },
	{ 0x0000000000000040ull, 0x00000000000003e0ull, N("e"), CC },
	{ 0x0000000000000060ull, 0x00000000000003e0ull, N("le"), CC },
	{ 0x0000000000000080ull, 0x00000000000003e0ull, N("g"), CC },
	{ 0x00000000000000a0ull, 0x00000000000003e0ull, N("lg"), CC },
	{ 0x00000000000000c0ull, 0x00000000000003e0ull, N("ge"), CC },
	{ 0x00000000000000e0ull, 0x00000000000003e0ull, N("lge"), CC },
	{ 0x0000000000000100ull, 0x00000000000003e0ull, N("u"), CC },
	{ 0x0000000000000120ull, 0x00000000000003e0ull, N("lu"), CC },
	{ 0x0000000000000140ull, 0x00000000000003e0ull, N("eu"), CC },
	{ 0x0000000000000160ull, 0x00000000000003e0ull, N("leu"), CC },
	{ 0x0000000000000180ull, 0x00000000000003e0ull, N("gu"), CC },
	{ 0x00000000000001a0ull, 0x00000000000003e0ull, N("lgu"), CC },
	{ 0x00000000000001c0ull, 0x00000000000003e0ull, N("geu"), CC },
	{ 0x00000000000001e0ull, 0x00000000000003e0ull, },
	{ 0x0000000000000200ull, 0x00000000000003e0ull, N("no"), CC },
	{ 0x0000000000000220ull, 0x00000000000003e0ull, N("nc"), CC },
	{ 0x0000000000000240ull, 0x00000000000003e0ull, N("ns"), CC },
	{ 0x0000000000000260ull, 0x00000000000003e0ull, N("na"), CC },
	{ 0x0000000000000280ull, 0x00000000000003e0ull, N("a"), CC },
	{ 0x00000000000002a0ull, 0x00000000000003e0ull, N("s"), CC },
	{ 0x00000000000002c0ull, 0x00000000000003e0ull, N("c"), CC },
	{ 0x00000000000002e0ull, 0x00000000000003e0ull, N("o"), CC },
	{ 0, 0, OOPS },
};


F(setdt5, 5, N("b32"), N("f32"))
F(setdt7, 7, N("b32"), N("f32"))

static struct insn tabis2[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull, SRC2 },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, CONST },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, IMM },
	{ 0, 0, OOPS },
};

static struct insn tabis2w3[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull, SRC2 },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, CONST },
	{ 0x0000800000000000ull, 0x0000c00000000000ull, SRC3 },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, IMM },
	{ 0, 0, OOPS },
};

static struct insn tabis3[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull, SRC3 },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, SRC3 },
	{ 0x0000800000000000ull, 0x0000c00000000000ull, CONST },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, SRC3 },
	{ 0, 0, OOPS },
};

static struct insn tabds3[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull, SRC3D },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, SRC3D },
	{ 0x0000800000000000ull, 0x0000c00000000000ull, CONST },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, SRC3D },
	{ 0, 0, OOPS },
};

static struct insn tabfs2[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull, SRC2 },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, CONST },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, FIMM },
	{ 0, 0, OOPS },
};

static struct insn tabfs2w3[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull, SRC2 },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, CONST },
	{ 0x0000800000000000ull, 0x0000c00000000000ull, SRC3 },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, FIMM },
	{ 0, 0, OOPS },
};

static struct insn tabds2[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull, SRC2D },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, CONST },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, DIMM },
	{ 0, 0, OOPS },
};

static struct insn tabds2w3[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull, SRC2D },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, CONST },
	{ 0x0000800000000000ull, 0x0000c00000000000ull, SRC3D },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, DIMM },
	{ 0, 0, OOPS },
};

static struct insn tabls2[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull, SRC2D },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, IMM },
	{ 0, 0, OOPS },
};

static struct insn tabvs2[] = {
	{ 0x0000000000000000ull, 0x0000800000000000ull, VIMM },
	{ 0x0000800000000000ull, 0x0000800000000000ull, SRC2 },
	{ 0, 0, OOPS },
};

static struct insn tabv4s2[] = {
	{ 0x0000000000000000ull, 0x0000800000000000ull, V4IMM },
	{ 0x0000800000000000ull, 0x0000800000000000ull, SRC2 },
	{ 0, 0, OOPS },
};

static struct insn tabsus2[] = {
	{ 0x0000000000000000ull, 0x0020000000000000ull, SRC2 },
	{ 0x0020000000000000ull, 0x0020000000000000ull, SUCONST },
	{ 0, 0, OOPS },
};

static struct insn tabsup[] = {
	{ 0x0000000000000000ull, 0x0010000000000000ull, PSRC3 },
	{ 0x0010000000000000ull, 0x0010000000000000ull, SESTART, N("not"), PSRC3, SEEND },
	{ 0, 0, OOPS },
};

static struct insn tabsuaddr[] = {
	{ 0x0000000000000000ull, 0x0000400000000000ull, IMGR },
	{ 0x0000400000000000ull, 0x0000400000000000ull, IMGI },
};

static struct insn tabsudim[] = {
	{ 0x0000000000000000ull, 0x0000300000000000ull, N("1d"), TSRC11 },
	{ 0x0000100000000000ull, 0x0000300000000000ull, N("2d"), TSRC12 },
	{ 0x0000200000000000ull, 0x0000300000000000ull, N("3d"), TSRC13 },
	{ 0x0000300000000000ull, 0x0000300000000000ull, N("e2d"), TSRC13 },
};

F1(sat5, 5, N("sat"))
F1(sat9, 9, N("sat"))
F1(sat18, 0x18, N("sat"))
F1(sat31, 0x31, N("sat"))
F1(sat38, 0x38, N("sat"))
F1(ftz5, 5, N("ftz"))
F1(ftz6, 6, N("ftz"))
F1(ftz3b, 0x3b, N("ftz"))
F1(ftz37, 0x37, N("ftz"))
F1(fmz7, 7, N("fmz"))
F1(neg39, 0x39, N("neg"))
F1(neg9, 9, N("neg"))
F1(neg8, 8, N("neg"))
F1(neg7, 7, N("neg"))
F1(neg4, 4, N("neg"))
F1(neg16, 0x16, N("neg"))
F1(abs7, 7, N("abs"))
F1(abs6, 6, N("abs"))
F1(abs19, 0x19, N("abs"))
F1(abs1e, 0x1e, N("abs"))
F1(rint, 7, T(fcrmi))
F1(rev, 8, N("rev"))
F(shclamp, 0x9, N("clamp"), N("wrap"))

F1(not9, 9, N("not"))
F1(not8, 8, N("not"))

F1(shiftamt, 6, N("shiftamt"))

F1(acout17, 0x17, CC)
F1(acout30, 0x30, CC)
F1(acout3a, 0x3a, CC)
F1(acin6, 6, CC)
F1(acin37, 0x37, CC)
F1(acin5, 5, CC)
F1(acin7, 7, CC)

F(us32_5, 5, N("u32"), N("s32"))
F(us32_7, 7, N("u32"), N("s32"))
F(us32_6, 6, N("u32"), N("s32"))
F(us32_9, 9, N("u32"), N("s32"))
F(us32_c, 0xc, N("u32"), N("s32"))
F(us32_d, 0xd, N("u32"), N("s32"))
F(us32_18, 0x18, N("u32"), N("s32"))
F(us32_2a, 0x2a, N("u32"), N("s32"))

F(us8_5, 5, N("u8"), N("s8"))
F(us8_6, 6, N("u8"), N("s8"))
F(us8_b, 0xb, N("u8"), N("s8"))
F(us8_c, 0xc, N("u8"), N("s8"))
F(us8_d, 0xd, N("u8"), N("s8"))
F(us8_39, 0x39, N("u8"), N("s8"))

F(us16_5, 5, N("u16"), N("s16"))
F(us16_6, 6, N("u16"), N("s16"))
F(us16_b, 0xb, N("u16"), N("s16"))
F(us16_c, 0xc, N("u16"), N("s16"))
F(us16_d, 0xd, N("u16"), N("s16"))
F(us16_39, 0x39, N("u16"), N("s16"))

F1(high5, 0x5, N("high"))
F1(high6, 6, N("high"))

F1(pnot1, 0x17, N("not"))
F1(pnot2, 0x1d, N("not"))
F1(pnot3, 0x34, N("not"))
F1(pnotn, 0x18, N("not"))

F1(outa, 0x9, N("out")) // read outputs to a[] from parallel tess control shader
F1(unk39, 0x39, N("unk39"))
F1(patch, 0x8, N("patch"))
F1(emit, 0x5, N("emit"))
F1(restart, 0x6, N("restart"))
F(dtex, 0x2d, N("dfp"), N("dall"))
F(dquadop, 0x9, N("dfp"), N("dall"))
F(ltex, 9, N("all"), N("live"))

F(sucdim, 0x30, N("1d"), N("2d"))
F(subfmdim, 0x30, N("2d"), N("3d"))

static struct insn tabsucm[] = {
	{ 0x0000000000000000ull, 0x00000000000001e0ull, N("sd"), N("r1") }, // probably
	{ 0x0000000000000020ull, 0x00000000000001e0ull, N("sd"), N("r2") },
	{ 0x0000000000000040ull, 0x00000000000001e0ull, N("sd"), N("r4") },
	{ 0x0000000000000060ull, 0x00000000000001e0ull, N("sd"), N("r8") },
	{ 0x0000000000000080ull, 0x00000000000001e0ull, N("sd"), N("r16") },
	{ 0x00000000000000a0ull, 0x00000000000001e0ull, N("pl"), N("r1") },
	{ 0x00000000000000c0ull, 0x00000000000001e0ull, N("pl"), N("r2") },
	{ 0x00000000000000e0ull, 0x00000000000001e0ull, N("pl"), N("r4") },
	{ 0x0000000000000100ull, 0x00000000000001e0ull, N("pl"), N("r8") },
	{ 0x0000000000000120ull, 0x00000000000001e0ull, N("pl"), N("r16") },
	{ 0x0000000000000140ull, 0x00000000000001e0ull, N("bl"), N("r1") },
	{ 0x0000000000000160ull, 0x00000000000001e0ull, N("bl"), N("r2") },
	{ 0x0000000000000180ull, 0x00000000000001e0ull, N("bl"), N("r4") },
	{ 0x00000000000001a0ull, 0x00000000000001e0ull, N("bl"), N("r8") },
	{ 0x00000000000001c0ull, 0x00000000000001e0ull, N("bl"), N("r16") },
	{ 0, 0, OOPS },
};

static struct insn tabcctlop[] = {
	{ 0x0000000000000000ull, 0x00000000000003e0ull, N("query1") },
	{ 0x0000000000000020ull, 0x00000000000003e0ull, N("pf1") },
	{ 0x0000000000000040ull, 0x00000000000003e0ull, N("pf15") },
	{ 0x0000000000000060ull, 0x00000000000003e0ull, N("pf2") },
	{ 0x0000000000000080ull, 0x00000000000003e0ull, N("wb") },
	{ 0x00000000000000a0ull, 0x00000000000003e0ull, N("iv") },
	{ 0x00000000000000c0ull, 0x00000000000003e0ull, N("ivall") },
	{ 0x00000000000000e0ull, 0x00000000000003e0ull, N("rs") },
	{ 0, 0, OOPS },
};

static struct insn tabcctlmod[] = {
	{ 0x0000000000000000ull, 0x000000000c000000ull },
	{ 0x0000000004000000ull, 0x000000000c000000ull, N("u") },
	{ 0x0000000008000000ull, 0x000000000c000000ull, N("c") },
	{ 0x000000000c000000ull, 0x000000000c000000ull, N("i") },
	{ 0, 0, OOPS },
};

static struct insn tabtexrgba[] = {
	{ 0x0000000000000000ull, 0x0000000000000060ull, N("r") },
	{ 0x0000000000000020ull, 0x0000000000000060ull, N("g") },
	{ 0x0000000000000040ull, 0x0000000000000060ull, N("b") },
	{ 0x0000000000000060ull, 0x0000000000000060ull, N("a") },
	{ 0, 0, OOPS },
};

static struct insn tabtexm[] = {
	{ 0x0000000000000000ull, 0x0000000000000180ull },
	{ 0x0000000000000080ull, 0x0000000000000180ull, N("t") },
	{ 0x0000000000000100ull, 0x0000000000000180ull, N("p") },
	{ 0, 0, OOPS },
};

static struct insn tabtext[] = {
	{ 0x0000000000000000ull, 0x0038000000000000ull, N("t1d") },
	{ 0x0008000000000000ull, 0x0038000000000000ull, N("a1d") },
	{ 0x0010000000000000ull, 0x0038000000000000ull, N("t2d") },
	{ 0x0018000000000000ull, 0x0038000000000000ull, N("a2d") },
	{ 0x0020000000000000ull, 0x0038000000000000ull, N("t3d") },
	{ 0x0030000000000000ull, 0x0038000000000000ull, N("tcube") },
	{ 0x0038000000000000ull, 0x0038000000000000ull, N("acube") },
	{ 0, 0, OOPS },
};

static struct insn tabtexsrc1[] = {
	{ 0x0000000000000000ull, 0x003c000000000000ull, TSRC11 }, // x
	{ 0x0004000000000000ull, 0x003c000000000000ull, TSRC12 }, // t, x
	{ 0x0008000000000000ull, 0x003c000000000000ull, TSRC12 }, // i, x
	{ 0x000c000000000000ull, 0x003c000000000000ull, TSRC12 }, // t+i, x
	{ 0x0010000000000000ull, 0x003c000000000000ull, TSRC12 }, // x, y
	{ 0x0014000000000000ull, 0x003c000000000000ull, TSRC13 }, // t, x, y
	{ 0x0018000000000000ull, 0x003c000000000000ull, TSRC13 }, // i, x, y
	{ 0x001c000000000000ull, 0x003c000000000000ull, TSRC13 }, // t+i, x, y
	{ 0x0020000000000000ull, 0x003c000000000000ull, TSRC13 }, // x, y, z
	{ 0x0024000000000000ull, 0x003c000000000000ull, TSRC14 }, // t, x, y, z
	{ 0x0030000000000000ull, 0x003c000000000000ull, TSRC13 }, // x, y, z
	{ 0x0034000000000000ull, 0x003c000000000000ull, TSRC14 }, // t, x, y, z
	{ 0x0038000000000000ull, 0x003c000000000000ull, TSRC14 }, // i, x, y, z
	{ 0x003c000000000000ull, 0x003c000000000000ull, TSRC14 }, // t+i, x, y, z
	{ 0, 0, SRC1, OOPS },
};

static struct insn tabtex4src1[] = {
	{ 0x0010000000000000ull, 0x007c000000000000ull, TSRC12 }, // x, y
	{ 0x0014000000000000ull, 0x007c000000000000ull, TSRC13 }, // t, x, y
	{ 0x0018000000000000ull, 0x007c000000000000ull, TSRC13 }, // i, x, y
	{ 0x001c000000000000ull, 0x007c000000000000ull, TSRC13 }, // t+i, x, y
	{ 0x0050000000000000ull, 0x007c000000000000ull, TSRC13 }, // x, y, o
	{ 0x0054000000000000ull, 0x007c000000000000ull, TSRC14 }, // t, x, y, o
	{ 0x0058000000000000ull, 0x007c000000000000ull, TSRC14 }, // i, x, y, o
	{ 0x005c000000000000ull, 0x007c000000000000ull, TSRC14 }, // t+i, x, y, o
	{ 0, 0, SRC1, OOPS },
};

static struct insn tabtexgrsrc1[] = {
	{ 0x0000000000000000ull, 0x00fc000000000000ull, TSRC11 }, // x
	{ 0x0004000000000000ull, 0x00fc000000000000ull, TSRC12 }, // t, x
	{ 0x0008000000000000ull, 0x00fc000000000000ull, TSRC12 }, // i, x
	{ 0x000c000000000000ull, 0x00fc000000000000ull, TSRC12 }, // t+i, x
	{ 0x0010000000000000ull, 0x00fc000000000000ull, TSRC12 }, // x, y
	{ 0x0014000000000000ull, 0x00fc000000000000ull, TSRC13 }, // t, x, y
	{ 0x0018000000000000ull, 0x00fc000000000000ull, TSRC13 }, // i, x, y
	{ 0x001c000000000000ull, 0x00fc000000000000ull, TSRC13 }, // t+i, x, y
	{ 0x0040000000000000ull, 0x00fc000000000000ull, TSRC12 }, // x, o
	{ 0x0044000000000000ull, 0x00fc000000000000ull, TSRC13 }, // t, x, o
	{ 0x0048000000000000ull, 0x00fc000000000000ull, TSRC13 }, // i, x, o
	{ 0x004c000000000000ull, 0x00fc000000000000ull, TSRC13 }, // t+i, x, o
	{ 0x0050000000000000ull, 0x00fc000000000000ull, TSRC13 }, // x, y, o
	{ 0x0054000000000000ull, 0x00fc000000000000ull, TSRC14 }, // t, x, y, o
	{ 0x0058000000000000ull, 0x00fc000000000000ull, TSRC14 }, // i, x, y, o
	{ 0x005c000000000000ull, 0x00fc000000000000ull, TSRC14 }, // t+i, x, y, o
	{ 0, 0, SRC1, OOPS },
};

static struct insn tabtexsrc2[] = {
	{ 0x0000000000000000ull, 0x0540000000000000ull, TSRC20 }, //
	{ 0x0040000000000000ull, 0x0540000000000000ull, TSRC21 }, // o
	{ 0x0100000000000000ull, 0x0540000000000000ull, TSRC21 }, // d
	{ 0x0140000000000000ull, 0x0540000000000000ull, TSRC22 }, // o, d
	{ 0x0400000000000000ull, 0x0540000000000000ull, TSRC21 }, // l
	{ 0x0440000000000000ull, 0x0540000000000000ull, TSRC22 }, // l, o
	{ 0x0500000000000000ull, 0x0540000000000000ull, TSRC22 }, // l, d
	{ 0x0540000000000000ull, 0x0540000000000000ull, TSRC23 }, // l, o, d
	{ 0, 0, SRC2, OOPS },
};

static struct insn tabtexfsrc2[] = {
	{ 0x0000000000000000ull, 0x02c0000000000000ull, TSRC20 }, //
	{ 0x0040000000000000ull, 0x02c0000000000000ull, TSRC21 }, // o
	{ 0x0080000000000000ull, 0x02c0000000000000ull, TSRC21 }, // ms
	{ 0x00c0000000000000ull, 0x02c0000000000000ull, TSRC22 }, // o, ms
	{ 0x0200000000000000ull, 0x02c0000000000000ull, TSRC21 }, // l
	{ 0x0240000000000000ull, 0x02c0000000000000ull, TSRC22 }, // l, o
	{ 0x0280000000000000ull, 0x02c0000000000000ull, TSRC22 }, // l, ms
	{ 0x02c0000000000000ull, 0x02c0000000000000ull, TSRC23 }, // l, o, ms
	{ 0, 0, SRC2, OOPS },
};

static struct insn tabtex4src2[] = {
	{ 0x0000000000000000ull, 0x01c0000000000000ull, TSRC20 }, //
	{ 0x0040000000000000ull, 0x01c0000000000000ull, TSRC21 }, // o
	{ 0x0080000000000000ull, 0x01c0000000000000ull, TSRC22 }, // ol, oh
	{ 0x0100000000000000ull, 0x01c0000000000000ull, TSRC21 }, // d
	{ 0x0140000000000000ull, 0x01c0000000000000ull, TSRC22 }, // o, d
	{ 0x0180000000000000ull, 0x01c0000000000000ull, TSRC23 }, // ol, oh, d
	{ 0, 0, SRC2, OOPS },
};

static struct insn tabtexgrsrc2[] = {
	{ 0x0000000000000000ull, 0x0030000000000000ull, TSRC22 }, // dtx/dsx, dtx/dsy
	{ 0x0010000000000000ull, 0x0030000000000000ull, TSRC24 }, // dtx/dsx, dtx/dsy, dty/dsx, dty/dsy
	{ 0, 0, SRC2, OOPS },
};

static struct insn tabtexquery[] = {
	{ 0x0000000000000000ull, 0x0fc0000000000000ull, N("tdims") },
	{ 0x0040000000000000ull, 0x0fc0000000000000ull, N("ttype") },
	{ 0x0080000000000000ull, 0x0fc0000000000000ull, N("tsamplepos") },
	{ 0x0200000000000000ull, 0x0fc0000000000000ull, N("sfilter") },
	{ 0x0240000000000000ull, 0x0fc0000000000000ull, N("slod") },
	{ 0x0280000000000000ull, 0x0fc0000000000000ull, N("swrap") },
	{ 0x02c0000000000000ull, 0x0fc0000000000000ull, N("sbcolor") },
	{ 0, 0, OOPS },
};

static struct insn tablodt[] = {
	{ 0x0000000000000000ull, 0x0e00000000000000ull, N("lauto") },
	{ 0x0200000000000000ull, 0x0e00000000000000ull, N("lzero") },
	{ 0x0400000000000000ull, 0x0e00000000000000ull, N("lbias") },
	{ 0x0600000000000000ull, 0x0e00000000000000ull, N("llod") },
	{ 0x0c00000000000000ull, 0x0e00000000000000ull, N("lbiasa") },
	{ 0x0e00000000000000ull, 0x0e00000000000000ull, N("lloda") },
	{ 0, 0, OOPS },
};

static struct insn tablodf[] = {
	{ 0x0000000000000000ull, 0x0200000000000000ull, N("lzero") },
	{ 0x0200000000000000ull, 0x0200000000000000ull, N("llod") },
	{ 0, 0, OOPS },
};

F1(texi, 0x32, N("ind"))
F1(texshd, 0x38, N("shadow"))
F1(texcl, 0x38, N("cl"))
F1(texoff, 0x36, N("off"))
F1(texms, 0x37, N("ms"))

static struct insn tabtoffg[] = {
	{ 0x0000000000000000ull, 0x00c0000000000000ull },
	{ 0x0040000000000000ull, 0x00c0000000000000ull, N("off") },
	{ 0x0080000000000000ull, 0x00c0000000000000ull, N("off8") },
	{ 0, 0, OOPS },
};

static struct insn tabtexf[] = {
	{ 0, 0, T(ltex), T(dtex) },
};

static struct insn tablane[] = {
	{ 0x0000000000000000ull, 0x00000000000001e0ull, N("lnone") },
	{ 0x0000000000000020ull, 0x00000000000001e0ull, N("l0") },
	{ 0x0000000000000040ull, 0x00000000000001e0ull, N("l1") },
	{ 0x0000000000000060ull, 0x00000000000001e0ull, N("l01") },
	{ 0x0000000000000080ull, 0x00000000000001e0ull, N("l2") },
	{ 0x00000000000000a0ull, 0x00000000000001e0ull, N("l02") },
	{ 0x00000000000000c0ull, 0x00000000000001e0ull, N("l12") },
	{ 0x00000000000000e0ull, 0x00000000000001e0ull, N("l012") },
	{ 0x0000000000000100ull, 0x00000000000001e0ull, N("l3") },
	{ 0x0000000000000120ull, 0x00000000000001e0ull, N("l03") },
	{ 0x0000000000000140ull, 0x00000000000001e0ull, N("l13") },
	{ 0x0000000000000160ull, 0x00000000000001e0ull, N("l013") },
	{ 0x0000000000000180ull, 0x00000000000001e0ull, N("l23") },
	{ 0x00000000000001a0ull, 0x00000000000001e0ull, N("l023") },
	{ 0x00000000000001c0ull, 0x00000000000001e0ull, N("l123") },
	{ 0x00000000000001e0ull, 0x00000000000001e0ull },
	{ 0, 0, OOPS },
};

// for quadop
static struct insn tabqs1[] = {
	{ 0x0000000000000000ull, 0x00000000000001c0ull, N("l0") },
	{ 0x0000000000000040ull, 0x00000000000001c0ull, N("l1") },
	{ 0x0000000000000080ull, 0x00000000000001c0ull, N("l2") },
	{ 0x00000000000000c0ull, 0x00000000000001c0ull, N("l3") },
	{ 0x0000000000000100ull, 0x00000000000001c0ull, N("dx") },
	{ 0x0000000000000140ull, 0x00000000000001c0ull, N("dy") },
	{ 0, 0, OOPS },
};

static struct insn tabqop0[] = {
	{ 0x0000000000000000ull, 0x000000c000000000ull, N("add") },
	{ 0x0000004000000000ull, 0x000000c000000000ull, N("subr") },
	{ 0x0000008000000000ull, 0x000000c000000000ull, N("sub") },
	{ 0x000000c000000000ull, 0x000000c000000000ull, N("mov2") },
	{ 0, 0, OOPS },
};

static struct insn tabqop1[] = {
	{ 0x0000000000000000ull, 0x0000003000000000ull, N("add") },
	{ 0x0000001000000000ull, 0x0000003000000000ull, N("subr") },
	{ 0x0000002000000000ull, 0x0000003000000000ull, N("sub") },
	{ 0x0000003000000000ull, 0x0000003000000000ull, N("mov2") },
	{ 0, 0, OOPS },
};

static struct insn tabqop2[] = {
	{ 0x0000000000000000ull, 0x0000000c00000000ull, N("add") },
	{ 0x0000000400000000ull, 0x0000000c00000000ull, N("subr") },
	{ 0x0000000800000000ull, 0x0000000c00000000ull, N("sub") },
	{ 0x0000000c00000000ull, 0x0000000c00000000ull, N("mov2") },
	{ 0, 0, OOPS },
};

static struct insn tabqop3[] = {
	{ 0x0000000000000000ull, 0x0000000300000000ull, N("add") },
	{ 0x0000000100000000ull, 0x0000000300000000ull, N("subr") },
	{ 0x0000000200000000ull, 0x0000000300000000ull, N("sub") },
	{ 0x0000000300000000ull, 0x0000000300000000ull, N("mov2") },
	{ 0, 0, OOPS },
};

static struct insn tabsetlop[] = {
	{ 0x000e000000000000ull, 0x006e000000000000ull },	// noop, really "and $p7"
	{ 0x0000000000000000ull, 0x0060000000000000ull, N("and"), T(pnot3), PSRC3 },
	{ 0x0020000000000000ull, 0x0060000000000000ull, N("or"), T(pnot3), PSRC3 },
	{ 0x0040000000000000ull, 0x0060000000000000ull, N("xor"), T(pnot3), PSRC3 },
	{ 0, 0, OOPS, T(pnot3), PSRC3 },
};

static struct insn tabcvtfdst[] = {
	{ 0x0000000000100000ull, 0x0000000000300000ull, N("f16"), DST, T(acout30) },
	{ 0x0000000000200000ull, 0x0000000000300000ull, N("f32"), DST, T(acout30) },
	{ 0x0000000000300000ull, 0x0000000000300000ull, N("f64"), DSTD, T(acout30) },
	{ 0, 0, OOPS, DST },
};

static struct insn tabcvtidst[] = {
	{ 0x0000000000000000ull, 0x0000000000300080ull, N("u8"), DST, T(acout30) },
	{ 0x0000000000000080ull, 0x0000000000300080ull, N("s8"), DST, T(acout30) },
	{ 0x0000000000100000ull, 0x0000000000300080ull, N("u16"), DST, T(acout30) },
	{ 0x0000000000100080ull, 0x0000000000300080ull, N("s16"), DST, T(acout30) },
	{ 0x0000000000200000ull, 0x0000000000300080ull, N("u32"), DST, T(acout30) },
	{ 0x0000000000200080ull, 0x0000000000300080ull, N("s32"), DST, T(acout30) },
	{ 0x0000000000300000ull, 0x0000000000300080ull, N("u64"), DSTD, T(acout30) },
	{ 0x0000000000300080ull, 0x0000000000300080ull, N("s64"), DSTD, T(acout30) },
	{ 0, 0, OOPS, DST },
};

static struct insn tabcvtf2idst[] = {
	{ 0x0000000000100000ull, 0x0000000000300080ull, N("u16"), DST, T(acout30) },
	{ 0x0000000000100080ull, 0x0000000000300080ull, N("s16"), DST, T(acout30) },
	{ 0x0000000000200000ull, 0x0000000000300080ull, N("u32"), DST, T(acout30) },
	{ 0x0000000000200080ull, 0x0000000000300080ull, N("s32"), DST, T(acout30) },
	{ 0x0000000000300000ull, 0x0000000000300080ull, N("u64"), DSTD, T(acout30) },
	{ 0x0000000000300080ull, 0x0000000000300080ull, N("s64"), DSTD, T(acout30) },
	{ 0, 0, OOPS, DST },
};

static struct insn tabcvtf2isrc[] = {
	{ 0x0000000000800000ull, 0x0000000003800000ull, T(neg8), T(abs6), N("f16"), HNUM, T(is2) },
	{ 0x0000000001000000ull, 0x0000000003800000ull, T(neg8), T(abs6), N("f32"), T(fs2) },
	{ 0x0000000001800000ull, 0x0000000003800000ull, T(neg8), T(abs6), N("f64"), T(ds2) },
	{ 0, 0, OOPS, T(neg8), T(abs6), SRC2 },
};

static struct insn tabcvtisrc[] = {
	{ 0x0000000000000000ull, 0x0000000003800200ull, T(neg8), T(abs6), N("u8"), BNUM, T(is2) },
	{ 0x0000000000000200ull, 0x0000000003800200ull, T(neg8), T(abs6), N("s8"), BNUM, T(is2) },
	{ 0x0000000000800000ull, 0x0000000003800200ull, T(neg8), T(abs6), N("u16"), HNUM, T(is2) },
	{ 0x0000000000800200ull, 0x0000000003800200ull, T(neg8), T(abs6), N("s16"), HNUM, T(is2) },
	{ 0x0000000001000000ull, 0x0000000003800200ull, T(neg8), T(abs6), N("u32"), T(is2) },
	{ 0x0000000001000200ull, 0x0000000003800200ull, T(neg8), T(abs6), N("s32"), T(is2) },
	{ 0x0000000001800000ull, 0x0000000003800200ull, T(neg8), T(abs6), N("u64"), T(ls2) },
	{ 0x0000000001800200ull, 0x0000000003800200ull, T(neg8), T(abs6), N("s64"), T(ls2) },
	{ 0, 0, OOPS, T(neg8), T(abs6), SRC2 },
};

static struct insn tabcvti2isrc[] = {
	{ 0x0000000000000000ull, 0x0000000003800200ull, T(neg8), T(abs6), N("u8"), BNUM, T(is2) },
	{ 0x0000000000000200ull, 0x0000000003800200ull, T(neg8), T(abs6), N("s8"), BNUM, T(is2) },
	{ 0x0000000000800000ull, 0x0000000003800200ull, T(neg8), T(abs6), N("u16"), HNUM, T(is2) },
	{ 0x0000000000800200ull, 0x0000000003800200ull, T(neg8), T(abs6), N("s16"), HNUM, T(is2) },
	{ 0x0000000001000000ull, 0x0000000003800200ull, T(neg8), T(abs6), N("u32"), T(is2) },
	{ 0x0000000001000200ull, 0x0000000003800200ull, T(neg8), T(abs6), N("s32"), T(is2) },
	{ 0, 0, OOPS, T(neg8), T(abs6), SRC2 },
};

static struct insn tabmulf[] = {
	{ 0x0000000000000000ull, 0x000e000000000000ull },
	{ 0x0002000000000000ull, 0x000e000000000000ull, N("div2") },
	{ 0x0004000000000000ull, 0x000e000000000000ull, N("div4") },
	{ 0x0006000000000000ull, 0x000e000000000000ull, N("div8") },
	{ 0x0008000000000000ull, 0x000e000000000000ull, N("mul8") },
	{ 0x000a000000000000ull, 0x000e000000000000ull, N("mul4") },
	{ 0x000c000000000000ull, 0x000e000000000000ull, N("mul2") },
	{ 0, 0, OOPS },
};

static struct insn tabaddop[] = {
	{ 0x0000000000000000ull, 0x0000000000000300ull, N("add") },
	{ 0x0000000000000100ull, 0x0000000000000300ull, N("sub") },
	{ 0x0000000000000200ull, 0x0000000000000300ull, N("subr") },
	{ 0x0000000000000300ull, 0x0000000000000300ull, N("addpo") },
	{ 0, 0, OOPS },
};

static struct insn tablogop[] = {
	{ 0x0000000000000000ull, 0x00000000000000c0ull, N("and") },
	{ 0x0000000000000040ull, 0x00000000000000c0ull, N("or") },
	{ 0x0000000000000080ull, 0x00000000000000c0ull, N("xor") },
	{ 0x00000000000000c0ull, 0x00000000000000c0ull, N("mov2") },
	{ 0, 0, OOPS },
};

static struct insn tabaddop2[] = {
	{ 0x0000000000000000ull, 0x0180000000000000ull, N("add") },
	{ 0x0080000000000000ull, 0x0180000000000000ull, N("sub") },
	{ 0x0100000000000000ull, 0x0180000000000000ull, N("subr") },
	{ 0x0180000000000000ull, 0x0180000000000000ull, N("addpo") },
	{ 0, 0, OOPS },
};

F(bar, 0x2f, SRC1, BAR)
F(bars, 0x1a, SRC1, BAR)
F(tcnt, 0x2e, SRC2, TCNT)

static struct insn tabprmtmod[] = {
	{ 0x00, 0xe0 },
	{ 0x20, 0xe0, N("f4e") },
	{ 0x40, 0xe0, N("b4e") },
	{ 0x60, 0xe0, N("rc8") },
	{ 0x80, 0xe0, N("ecl") },
	{ 0xa0, 0xe0, N("ecr") },
	{ 0xc0, 0xe0, N("rc16") },
	{ 0, 0, OOPS },
};

static struct insn tabminmax[] = {
	{ 0x000e000000000000ull, 0x001e000000000000ull, N("min") },
	{ 0x001e000000000000ull, 0x001e000000000000ull, N("max") },
	{ 0, 0, N("minmax"), T(pnot3), PSRC3 }, // min if true
};

// XXX: orthogonalise it. if possible.
static struct insn tabredop[] = {
	{ 0x00, 0x1e0, N("add") },
	{ 0x20, 0x1e0, N("min") },
	{ 0x40, 0x1e0, N("max") },
	{ 0x60, 0x1e0, N("inc") },
	{ 0x80, 0x1e0, N("dec") },
	{ 0xa0, 0x1e0, N("and") },
	{ 0xc0, 0x1e0, N("or") },
	{ 0xe0, 0x1e0, N("xor") },
	{ 0, 0, OOPS },
};

static struct insn tabredops[] = {
	{ 0x00, 0x1e0, N("add") },
	{ 0x20, 0x1e0, N("min") },
	{ 0x40, 0x1e0, N("max") },
	{ 0, 0, OOPS },
};

static struct insn tablcop[] = {
	{ 0x000, 0x300, N("ca") },
	{ 0x100, 0x300, N("cg") },
	{ 0x200, 0x300, N("cs") },
	{ 0x300, 0x300, N("cv") },
	{ 0, 0, OOPS },
};

static struct insn tabscop[] = {
	{ 0x000, 0x300, N("wb") },
	{ 0x100, 0x300, N("cg") },
	{ 0x200, 0x300, N("cs") },
	{ 0x300, 0x300, N("wt") },
	{ 0, 0, OOPS },
};

static struct insn tabsclamp[] = {
	{ 0x0000000000000000ull, 0x0001800000000000ull, N("zero") },
	{ 0x0000800000000000ull, 0x0001800000000000ull, N("clamp") },
	{ 0x0001000000000000ull, 0x0001800000000000ull, N("trap") },
	{ 0, 0, OOPS },
};

static struct insn tabsclamp2s[] = {
	{ 0x0000000000000000ull, 0x0001800000000000ull, N("ign") },
	{ 0x0000800000000000ull, 0x0001800000000000ull, N("trap") },
	{ 0x0001800000000000ull, 0x0001800000000000ull, N("sdcl") },
	{ 0, 0, OOPS },
};

static struct insn tabsclamp2l[] = {
	{ 0x0000000000000000ull, 0x0001800000000000ull, N("zero") },
	{ 0x0000800000000000ull, 0x0001800000000000ull, N("trap") },
	{ 0x0001800000000000ull, 0x0001800000000000ull, N("sdcl") },
	{ 0, 0, OOPS },
};

static struct insn tabvdst[] = {
	{ 0x0000000000000000ull, 0x0380000000000000ull, N("h1") },
	{ 0x0080000000000000ull, 0x0380000000000000ull, N("h0") },
	{ 0x0100000000000000ull, 0x0380000000000000ull, N("b0") },
	{ 0x0180000000000000ull, 0x0380000000000000ull, N("b2") },
	{ 0x0200000000000000ull, 0x0380000000000000ull, N("add") },
	{ 0x0280000000000000ull, 0x0380000000000000ull, N("min") },
	{ 0x0300000000000000ull, 0x0380000000000000ull, N("max") },
	{ 0x0380000000000000ull, 0x0380000000000000ull },
	{ 0, 0, OOPS },
};

static struct insn tabvsrc1[] = {
	{ 0x0000000000000000ull, 0x0000700000000000ull, N("b0") },
	{ 0x0000100000000000ull, 0x0000700000000000ull, N("b1") },
	{ 0x0000200000000000ull, 0x0000700000000000ull, N("b2") },
	{ 0x0000300000000000ull, 0x0000700000000000ull, N("b3") },
	{ 0x0000400000000000ull, 0x0000700000000000ull, N("h0") },
	{ 0x0000500000000000ull, 0x0000700000000000ull, N("h1") },
	{ 0x0000600000000000ull, 0x0000700000000000ull },
	{ 0, 0, OOPS },
};

static struct insn tabvsrc2[] = {
	{ 0x0000000000000000ull, 0x0000800000000000ull },
	{ 0x0000800000000000ull, 0x0000800700000000ull, N("b0") },
	{ 0x0000800100000000ull, 0x0000800700000000ull, N("b1") },
	{ 0x0000800200000000ull, 0x0000800700000000ull, N("b2") },
	{ 0x0000800300000000ull, 0x0000800700000000ull, N("b3") },
	{ 0x0000800400000000ull, 0x0000800700000000ull, N("h0") },
	{ 0x0000800500000000ull, 0x0000800700000000ull, N("h1") },
	{ 0x0000800600000000ull, 0x0000800700000000ull },
	{ 0, 0, OOPS },
};

static struct insn tabv4dst[] = {
	{ 0x0000000000000000ull, 0x0000700000000000ull },
	{ 0x0000100000000000ull, 0x0000700000000000ull, N("simd_min") },
	{ 0x0000200000000000ull, 0x0000700000000000ull, N("simd_max") },
	{ 0x0000400000000000ull, 0x0000700000000000ull, N("add") },
	{ 0x0000500000000000ull, 0x0000700000000000ull, N("min") },
	{ 0x0000600000000000ull, 0x0000700000000000ull, N("max") },
	{ 0, 0, OOPS },
};

static struct insn tabv4dmask[] = {
	{ 0x0000000000000000ull, 0x0180000c00000000ull, N("none") },
	{ 0x0080000000000000ull, 0x0180000c00000000ull, N("x") },
	{ 0x0100000000000000ull, 0x0180000c00000000ull, N("y") },
	{ 0x0180000000000000ull, 0x0180000c00000000ull, N("xy") },
	{ 0x0000000400000000ull, 0x0180000c00000000ull, N("z") },
	{ 0x0080000400000000ull, 0x0180000c00000000ull, N("xz") },
	{ 0x0100000400000000ull, 0x0180000c00000000ull, N("yz") },
	{ 0x0180000400000000ull, 0x0180000c00000000ull, N("xyz") },
	{ 0x0000000800000000ull, 0x0180000c00000000ull, N("w") },
	{ 0x0080000800000000ull, 0x0180000c00000000ull, N("xw") },
	{ 0x0100000800000000ull, 0x0180000c00000000ull, N("yw") },
	{ 0x0180000800000000ull, 0x0180000c00000000ull, N("xyw") },
	{ 0x0000000c00000000ull, 0x0180000c00000000ull, N("zw") },
	{ 0x0080000c00000000ull, 0x0180000c00000000ull, N("xzw") },
	{ 0x0100000c00000000ull, 0x0180000c00000000ull, N("yzw") },
	{ 0x0180000c00000000ull, 0x0180000c00000000ull },
	{ 0, 0, OOPS },
};

static struct insn tabv2dmask[] = {
	{ 0x0000000000000000ull, 0x0180000000000000ull, N("none") },
	{ 0x0080000000000000ull, 0x0180000000000000ull, N("x") },
	{ 0x0100000000000000ull, 0x0180000000000000ull, N("y") },
	{ 0x0180000000000000ull, 0x0180000000000000ull },
	{ 0, 0, OOPS },
};

static struct insn tabv4src1[] = {
	{ 0x0000000000000000ull, 0x00000f0000000000ull, N("b0") },
	{ 0x0000010000000000ull, 0x00000f0000000000ull, N("b1") },
	{ 0x0000020000000000ull, 0x00000f0000000000ull, N("b2") },
	{ 0x0000030000000000ull, 0x00000f0000000000ull, N("b3") },
	{ 0x0000040000000000ull, 0x00000f0000000000ull  },
	{ 0x0000050000000000ull, 0x00000f0000000000ull, N("b1234") },
	{ 0x0000060000000000ull, 0x00000f0000000000ull, N("b2345") },
	{ 0x0000070000000000ull, 0x00000f0000000000ull, N("b3456") },
	{ 0x0000080000000000ull, 0x00000f0000000000ull, N("b1023") },
	{ 0x0000090000000000ull, 0x00000f0000000000ull, N("b2103") },
	{ 0x00000a0000000000ull, 0x00000f0000000000ull, N("b3120") },
	{ 0x00000b0000000000ull, 0x00000f0000000000ull, N("b0213") },
	{ 0x00000c0000000000ull, 0x00000f0000000000ull, N("b0321") },
	{ 0x00000d0000000000ull, 0x00000f0000000000ull, N("b0132") },
	{ 0, 0, OOPS },
};

static struct insn tabv2src1[] = {
	{ 0x0000000000000000ull, 0x00008c0000000000ull, N("h0") },
	{ 0x0000040000000000ull, 0x00008c0000000000ull, N("h10") },
	{ 0x0000080000000000ull, 0x00008c0000000000ull },
	{ 0x00000c0000000000ull, 0x00008c0000000000ull, N("h1") },
	{ 0x0000800000000000ull, 0x00008f0000000000ull, N("h0") },
	{ 0x0000810000000000ull, 0x00008f0000000000ull, N("h10") },
	{ 0x0000820000000000ull, 0x00008f0000000000ull, N("h20") },
	{ 0x0000830000000000ull, 0x00008f0000000000ull, N("h30") },
	{ 0x0000840000000000ull, 0x00008f0000000000ull },
	{ 0x0000850000000000ull, 0x00008f0000000000ull, N("h1") },
	{ 0x0000860000000000ull, 0x00008f0000000000ull, N("h21") },
	{ 0x0000870000000000ull, 0x00008f0000000000ull, N("h31") },
	{ 0x0000880000000000ull, 0x00008f0000000000ull, N("h02") },
	{ 0x0000890000000000ull, 0x00008f0000000000ull, N("h12") },
	{ 0x00008a0000000000ull, 0x00008f0000000000ull, N("h2") },
	{ 0x00008b0000000000ull, 0x00008f0000000000ull, N("h32") },
	{ 0x00008c0000000000ull, 0x00008f0000000000ull, N("h03") },
	{ 0x00008d0000000000ull, 0x00008f0000000000ull, N("h13") },
	{ 0x00008e0000000000ull, 0x00008f0000000000ull, N("h23") },
	{ 0x00008f0000000000ull, 0x00008f0000000000ull, N("h3") },
	{ 0, 0, OOPS },
};

static struct insn tabv4src2[] = {
	{ 0x0000000000000000ull, 0x000000f000000000ull, N("b4") },
	{ 0x0000001000000000ull, 0x000000f000000000ull, N("b5") },
	{ 0x0000002000000000ull, 0x000000f000000000ull, N("b6") },
	{ 0x0000003000000000ull, 0x000000f000000000ull, N("b7") },
	{ 0x0000004000000000ull, 0x000000f000000000ull },
	{ 0x0000005000000000ull, 0x000000f000000000ull, N("b3456") },
	{ 0x0000006000000000ull, 0x000000f000000000ull, N("b2345") },
	{ 0x0000007000000000ull, 0x000000f000000000ull, N("b1234") },
	{ 0x0000008000000000ull, 0x000000f000000000ull, N("b7654") },
	{ 0x0000009000000000ull, 0x000000f000000000ull, N("b5476") },
	{ 0x000000a000000000ull, 0x000000f000000000ull, N("b6745") },
	{ 0, 0, OOPS },
};

static struct insn tabv2src2[] = {
	{ 0x0000000000000000ull, 0x0000800000000000ull },
	{ 0x0000800000000000ull, 0x000080f000000000ull, N("h0") },
	{ 0x0000801000000000ull, 0x000080f000000000ull, N("h10") },
	{ 0x0000802000000000ull, 0x000080f000000000ull, N("h20") },
	{ 0x0000803000000000ull, 0x000080f000000000ull, N("h30") },
	{ 0x0000804000000000ull, 0x000080f000000000ull, N("h01") },
	{ 0x0000805000000000ull, 0x000080f000000000ull, N("h1") },
	{ 0x0000806000000000ull, 0x000080f000000000ull, N("h21") },
	{ 0x0000807000000000ull, 0x000080f000000000ull, N("h31") },
	{ 0x0000808000000000ull, 0x000080f000000000ull, N("h02") },
	{ 0x0000809000000000ull, 0x000080f000000000ull, N("h12") },
	{ 0x000080a000000000ull, 0x000080f000000000ull, N("h2") },
	{ 0x000080b000000000ull, 0x000080f000000000ull, N("h32") },
	{ 0x000080c000000000ull, 0x000080f000000000ull, N("h03") },
	{ 0x000080d000000000ull, 0x000080f000000000ull, N("h13") },
	{ 0x000080e000000000ull, 0x000080f000000000ull },
	{ 0x000080f000000000ull, 0x000080f000000000ull, N("h3") },
	{ 0, 0, OOPS },
};

F(vsclamp, 0x7, N("clamp"), N("wrap"))

static struct insn tabvmop[] = {
	{ 0x000, 0x180, N("add") },
	{ 0x080, 0x180, N("sub") },
	{ 0x100, 0x180, N("subr") },
	{ 0x180, 0x180, N("addpo") },
	{ 0, 0, OOPS },
};

static struct insn tabvmshr[] = {
	{ 0x0000000000000000ull, 0x0180000000000000ull, },
	{ 0x0080000000000000ull, 0x0180000000000000ull, N("shr7") },
	{ 0x0100000000000000ull, 0x0180000000000000ull, N("shr15") },
	{ 0, 0, OOPS },
};

static struct insn tabinterpmode[] = {
	{ 0x0000000000000000ull, 0x00000000000000c0ull, N("pass") },
	{ 0x0000000000000040ull, 0x00000000000000c0ull, N("mul") },
	{ 0x0000000000000080ull, 0x00000000000000c0ull, N("flat") },
	{ 0x00000000000000c0ull, 0x00000000000000c0ull, N("sc") },
	{ 0, 0, OOPS },
};

static struct insn tabinterpsample[] = {
	{ 0x0000000000000000ull, 0x0000000000000300ull },
	{ 0x0000000000000100ull, 0x0000000000000300ull, N("cent") },
	{ 0x0000000000000200ull, 0x0000000000000300ull, N("off") },
	{ 0, 0, OOPS },
};

static struct insn tabpixinfo[] = {
	{ 0x0000000000000000ull, 0x00000000000000e0ull, N("samplecnt") },
	{ 0x0000000000000020ull, 0x00000000000000e0ull, N("covmask") },
	{ 0x0000000000000040ull, 0x00000000000000e0ull, N("covered") },
	{ 0x0000000000000060ull, 0x00000000000000e0ull, N("offset") },
	{ 0x0000000000000080ull, 0x00000000000000e0ull, N("cent_offset") },
	{ 0x00000000000000a0ull, 0x00000000000000e0ull, N("sampleid") },
	{ 0, 0, OOPS },
};

static struct insn tabvsetop[] = {
	{ 0x000, 0x380, N("false") },
	{ 0x080, 0x380, N("lt") },
	{ 0x100, 0x380, N("eq") },
	{ 0x180, 0x380, N("le") },
	{ 0x200, 0x380, N("gt") },
	{ 0x280, 0x380, N("ne") },
	{ 0x300, 0x380, N("ge") },
	{ 0x380, 0x380, N("true") },
	{ 0, 0, OOPS },
};

static struct insn tabpsrc[] = {
	{ 0x0000000000000000ull, 0x00000000fc000000ull, T(pnot1), PSRC1 },
	{ 0x0000000000000000ull, 0x00000000c0000000ull, T(pnot1), PSRC1, N("and"), T(pnot2), PSRC2 },
	{ 0x0000000040000000ull, 0x00000000c0000000ull, T(pnot1), PSRC1, N("or"), T(pnot2), PSRC2 },
	{ 0x0000000080000000ull, 0x00000000c0000000ull, T(pnot1), PSRC1, N("xor"), T(pnot2), PSRC2 },
	{ 0, 0, OOPS },
};

static struct insn tabshfl1[] = {
	{ 0x0000000000000000ull, 0x0180000000000000ull, N("idx") },
	{ 0x0080000000000000ull, 0x0180000000000000ull, N("up") },
	{ 0x0100000000000000ull, 0x0180000000000000ull, N("down") },
	{ 0x0180000000000000ull, 0x0180000000000000ull, N("bfly") },
	{ 0, 0, OOPS },
};

static struct insn tabshfl2[] = {
	{ 0x0000000000000000ull, 0x0000000000000060ull, SRC2, SRC3 },
	{ 0x0000000000000020ull, 0x0000000000000060ull, SHFLIMM1 , SRC3 },
	{ 0x0000000000000040ull, 0x0000000000000060ull, SRC2, SHFLIMM2 },
	{ 0x0000000000000060ull, 0x0000000000000060ull, SHFLIMM1, SHFLIMM2 },
	{ 0, 0, OOPS },
};

static struct insn tabsuq[] = {
	{ 0x0000000000000000ull, 0x00000000000000e0ull, N("rank") },
	{ 0x0000000000000020ull, 0x00000000000000e0ull, N("pixfmt") },
	{ 0x0000000000000040ull, 0x00000000000000e0ull, N("smplsz") },
	{ 0x0000000000000060ull, 0x00000000000000e0ull, N("dim") },
	{ 0x0000000000000080ull, 0x00000000000000e0ull, N("rgba") },
	{ 0x00000000000000a0ull, 0x00000000000000e0ull, N("blksz") },
	{ 0, 0, OOPS },
};

/*
 * Opcode format
 *
 * 0000000000000007 insn type, roughly: 0: float 1: double 2: long immediate 3: integer 4: moving and converting 5: g/s/l[] memory access 6: c[] and texture access 7: control
 * 0000000000000018 ??? never seen used
 * 00000000000003e0 misc flags
 * 0000000000001c00 used predicate [7 is always true]
 * 0000000000002000 negate predicate
 * 00000000000fc000 DST
 * 0000000003f00000 SRC1
 * 00000000fc000000 SRC2
 * 000003fffc000000 CONST offset
 * 00003c0000000000 CONST space
 * 00003ffffc000000 IMM/FIMM/DIMM
 * 0000c00000000000 0 = use SRC2, 1 = use CONST, 2 = ???, 3 = IMM/FIMM/DIMM
 * 0001000000000000 misc flag
 * 007e000000000000 SRC3
 * 0780000000000000 misc field. rounding mode or comparison subop or...
 * f800000000000000 opcode
 */

static struct insn tabm[] = {
	{ 0x0800000000000000ull, 0xf800000000000007ull, T(minmax), T(ftz5), N("f32"), DST, T(acout30), T(neg9), T(abs7), SRC1, T(neg8), T(abs6), T(fs2) },
	{ 0x1000000000000000ull, 0xf000000000000007ull, N("set"), T(ftz3b), T(setdt5), DST, T(acout30), T(setit), N("f32"), T(neg9), T(abs7), SRC1, T(neg8), T(abs6), T(fs2), T(setlop) },
	{ 0x2000000000000000ull, 0xf000000000000007ull, N("set"), T(ftz3b), PDST, PDSTN, T(setit), N("f32"), T(neg9), T(abs7), SRC1, T(neg8), T(abs6), T(fs2), T(setlop) },
	{ 0x3000000000000000ull, 0xf800000000000007ull, N("fma"), T(ftz6), T(fmz7), T(sat5), T(farm), N("f32"), DST, T(acout30), T(neg9), SRC1, T(fs2w3), T(neg8), T(is3) },
	{ 0x3800000000000000ull, 0xf800000000000007ull, N("slct"), T(ftz5), N("b32"), DST, SRC1, T(fs2w3), T(setit), N("f32"), T(is3) },
	// 40?
	{ 0x4800000000000000ull, 0xf800000000000007ull, N("quadop"), T(ftz5), T(farm), N("f32"), T(qop0), T(qop1), T(qop2), T(qop3), DST, T(acout30), T(dquadop), T(qs1), SRC1, SRC2 },
	{ 0x5000000000000000ull, 0xf800000000000007ull, N("add"), T(ftz5), T(sat31), T(farm), N("f32"), DST, T(acout30), T(neg9), T(abs7), SRC1, T(neg8), T(abs6), T(fs2) },
	{ 0x5800000000000000ull, 0xf800000000000007ull, N("mul"), T(mulf), T(fmz7), T(ftz6), T(sat5), T(farm), T(neg39), N("f32"), DST, T(acout30), SRC1, T(fs2) },
	{ 0x6000000000000000ull, 0xf800000000000027ull, N("presin"), N("f32"), DST, T(neg8), T(abs6), T(fs2) },
	{ 0x6000000000000020ull, 0xf800000000000027ull, N("preex2"), N("f32"), DST, T(neg8), T(abs6), T(fs2) },
	{ 0xc000000000000000ull, 0xf800000000000007ull, N("interp"), T(interpmode), T(sat5), N("f32"), DST, T(interpsample), ATTR, SRC2, SRC3 },
	{ 0xc800000000000000ull, 0xf80000001c000007ull, N("cos"), T(sat5), N("f32"), DST, T(neg9), T(abs7), SRC1 },
	{ 0xc800000004000000ull, 0xf80000001c000007ull, N("sin"), T(sat5), N("f32"), DST, T(neg9), T(abs7), SRC1 },
	{ 0xc800000008000000ull, 0xf80000001c000007ull, N("ex2"), T(sat5), N("f32"), DST, T(neg9), T(abs7), SRC1 },
	{ 0xc80000000c000000ull, 0xf80000001c000007ull, N("lg2"), T(sat5), N("f32"), DST, T(neg9), T(abs7), SRC1 },
	{ 0xc800000010000000ull, 0xf80000001c000007ull, N("rcp"), T(sat5), N("f32"), DST, T(neg9), T(abs7), SRC1 },
	{ 0xc800000014000000ull, 0xf80000001c000007ull, N("rsqrt"), T(sat5), N("f32"), DST, T(neg9), T(abs7), SRC1 },
	{ 0xc800000018000000ull, 0xf80000001c000007ull, N("rcp64h"), T(sat5), DST, T(neg9), T(abs7), SRC1 },
	{ 0xc80000001c000000ull, 0xf80000001c000007ull, N("rsqrt64h"), T(sat5), DST, T(neg9), T(abs7), SRC1 },
	{ 0x0000000000000000ull, 0x0000000000000007ull, OOPS, T(farm), N("f32"), DST, SRC1, T(fs2w3), T(is3) },


	{ 0x0800000000000001ull, 0xf800000000000007ull, T(minmax), N("f64"), DSTD, T(acout30), T(neg9), T(abs7), SRC1D, T(neg8), T(abs6), T(ds2) },
	{ 0x1000000000000001ull, 0xf800000000000007ull, N("set"), T(setdt5), DST, T(acout30), T(setit), N("f64"), T(neg9), T(abs7), SRC1D, T(neg8), T(abs6), T(ds2), T(setlop) },
	{ 0x1800000000000001ull, 0xf800000000000007ull, N("set"), PDST, PDSTN, T(setit), N("f64"), T(neg9), T(abs7), SRC1D, T(neg8), T(abs6), T(ds2), T(setlop) },
	{ 0x2000000000000001ull, 0xf800000000000007ull, N("fma"), T(farm), N("f64"), DSTD, T(acout30), T(neg9), SRC1D, T(ds2w3), T(neg8), T(ds3) },
	{ 0x4800000000000001ull, 0xf800000000000007ull, N("add"), T(farm), N("f64"), DSTD, T(acout30), T(neg9), T(abs7), SRC1D, T(neg8), T(abs6), T(ds2) },
	{ 0x5000000000000001ull, 0xf800000000000007ull, N("mul"), T(farm), T(neg9), N("f64"), DSTD, T(acout30), SRC1D, T(ds2) },
	{ 0x0000000000000001ull, 0x0000000000000007ull, OOPS, T(farm), N("f64"), DSTD, SRC1D, T(ds2), SRC3D },


	{ 0x0000000000000002ull, 0xf800000000000007ull, T(addop), DST, T(acout3a), SESTART, N("mul"), T(high6), T(us32_7), SRC1, T(us32_5), LIMM, SEEND, DST },
	{ 0x0800000000000002ull, 0xf800000000000007ull, T(addop), T(sat5), N("b32"), DST, T(acout3a), SRC1, LIMM, T(acin6) },
	{ 0x1000000000000002ull, 0xf800000000000007ull, N("mul"), T(high6), DST, T(acout3a), T(us32_7), SRC1, T(us32_5), LIMM },
	{ 0x1800000000000002ull, 0xf800000000000007ull, T(lane), N("mov"), N("b32"), DST, LIMM },
	{ 0x2000000000000002ull, 0xf800000000000007ull, N("fma"), T(ftz6), T(fmz7), T(sat5), N("f32"), DST, T(acout3a), T(neg9), SRC1, LIMM, T(neg8), DST },
	{ 0x2800000000000002ull, 0xf800000000000007ull, N("add"), T(ftz5), N("f32"), DST, T(acout3a), T(neg9), T(abs7), SRC1, LIMM },
	{ 0x3000000000000002ull, 0xf800000000000007ull, N("mul"), T(fmz7), T(ftz6), T(sat5), N("f32"), DST, T(acout3a), SRC1, LIMM },
	{ 0x3800000000000002ull, 0xf800000000000007ull, T(logop), N("b32"), DST, T(acout3a), T(not9), SRC1, T(not8), LIMM, T(acin5) },
	{ 0x4000000000000002ull, 0xf800000000000007ull, N("add"), N("b32"), DST, T(acout3a), SESTART, N("shl"), SRC1, SHCNT, SEEND, LIMM },
	{ 0x0000000000000002ull, 0x0000000000000007ull, OOPS, N("b32"), DST, SRC1, LIMM },


	{ 0x0180000000000003ull, 0xf980000000000007ull, N("mad"), N("b32"), N("sd"), DST, T(acout30), SRC1, T(is2w3), T(is3), .fmask = F_GK104 }, // ?
	{ 0x0000000000000003ull, 0xf800000000000007ull, N("mad"), N("b32"), DST, T(acout30), T(imadp1), SRC1, T(imadp2), T(is2w3), T(imadp3), T(is3), .fmask = F_GK104 },
	{ 0x0800000000000003ull, 0xf8000000000000c7ull, T(minmax), T(us32_5), DST, T(acout30), SRC1, T(is2) },
	{ 0x0800000000000043ull, 0xf8000000000000c7ull, T(minmax), N("low"), T(us32_5), DST, T(acout30), SRC1, T(is2), CC },
	{ 0x0800000000000083ull, 0xf8000000000000c7ull, T(minmax), N("med"), T(us32_5), DST, T(acout30), SRC1, T(is2), CC },
	{ 0x08000000000000c3ull, 0xf8000000000000c7ull, T(minmax), N("high"), T(us32_5), DST, T(acout30), SRC1, T(is2) },
	{ 0x1000000000000003ull, 0xf800000000000007ull, N("set"), T(setdt7), DST, T(acout30), T(setit), T(us32_5), SRC1, T(is2), T(acin6), T(setlop) },
	{ 0x1800000000000003ull, 0xf800000000000007ull, N("set"), PDST, PDSTN, T(setit), T(us32_5), SRC1, T(is2), T(acin6), T(setlop) },
	{ 0x2000000000000003ull, 0xf800000000000007ull, T(addop), T(sat38), DST, T(acout30), SESTART, N("mul"), T(high6), T(us32_7), SRC1, T(us32_5), T(is2w3), SEEND, T(is3), T(acin37) },
	{ 0x2800000000000003ull, 0xf800000000000007ull, N("ins"), N("b32"), DST, T(acout30), SRC1, T(is2w3), T(is3) },
	{ 0x3000000000000003ull, 0xf800000000000007ull, N("slct"), N("b32"), DST, SRC1, T(is2w3), T(setit), T(us32_5), T(is3) },
	{ 0x3800000000000003ull, 0xf800000000000007ull, N("sad"), T(us32_5), DST, T(acout30), SRC1, T(is2w3), T(is3) },
	{ 0x4000000000000003ull, 0xf800000000000007ull, T(addop2), N("b32"), DST, T(acout30), SESTART, N("shl"), SRC1, SHCNT, SEEND, T(is2) },
	{ 0x4800000000000003ull, 0xf800000000000007ull, T(addop), T(sat5), N("b32"), DST, T(acout30), SRC1, T(is2), T(acin6) },
	{ 0x5000000000000003ull, 0xf800000000000007ull, N("mul"), T(high6), DST, T(acout30), T(us32_7), SRC1, T(us32_5), T(is2) },
	{ 0x5800000000000003ull, 0xf800000000000007ull, N("shr"), T(rev), T(us32_5), DST, T(acout30), SRC1, T(shclamp), T(is2), T(acin7) },
	{ 0x6000000000000003ull, 0xf800000000000007ull, N("shl"), N("b32"), DST, T(acout30), SRC1, T(shclamp), T(is2), T(acin6) },
	{ 0x6800000000000003ull, 0xf800000000000007ull, T(logop), N("b32"), DST, T(acout30), T(not9), SRC1, T(not8), T(is2), T(acin5) },
	{ 0x7000000000000003ull, 0xf800000000000007ull, N("ext"), T(rev), T(us32_5), DST, T(acout30), SRC1, T(is2) }, // yes. this can reverse bits in a bitfield. really.
	{ 0x7800000000000003ull, 0xf800000000000007ull, N("bfind"), T(shiftamt), T(us32_5), DST, T(acout30), T(not8), T(is2) }, // index of highest bit set, counted from 0, -1 for 0 src. or highest bit different from sign for signed version. check me.
	{ 0x0000000000000003ull, 0x0000000000000007ull, OOPS, N("b32"), DST, SRC1, T(is2w3), T(is3) },


	// 08?
	{ 0x0000000000000004ull, 0xfc00000000000007ull, N("set"), T(setdt5), DST, T(acout30), T(setct), CC, T(setlop) },
	{ 0x0400000000000004ull, 0xfc00000000000007ull, N("set"), PDST, PDSTN, T(setct), CC, T(setlop) },
	{ 0x0800000000000004ull, 0xfc00000000000007ull, N("set"), T(setdt5), DST, T(acout30), T(psrc), T(setlop) },
	{ 0x0c00000000000004ull, 0xfc00000000000007ull, N("set"), PDST, PDSTN, T(psrc), T(setlop) },
	{ 0x1000000000900004ull, 0xfc00000001b00007ull, N("cvt"), T(ftz37), T(sat5), T(rint), N("f16"), DST, T(acout30), T(neg8), T(abs6), N("f16"), T(is2), HNUM },
	{ 0x1000000001200004ull, 0xfc00000001b00007ull, N("cvt"), T(ftz37), T(sat5), T(rint), N("f32"), DST, T(acout30), T(neg8), T(abs6), N("f32"), T(fs2) },
	{ 0x1000000001b00004ull, 0xfc00000001b00007ull, N("cvt"), T(ftz37), T(sat5), T(rint), N("f64"), DSTD, T(acout30), T(neg8), T(abs6), N("f64"), T(ds2) },
	{ 0x1000000001100004ull, 0xfc00000001b00007ull, N("cvt"), T(ftz37), T(sat5), T(fcrm), N("f16"), DST, T(acout30), T(neg8), T(abs6), N("f32"), T(fs2) },
	{ 0x1000000001a00004ull, 0xfc00000001b00007ull, N("cvt"), T(ftz37), T(sat5), T(fcrm), N("f32"), DST, T(acout30), T(neg8), T(abs6), N("f64"), T(ds2) },
	{ 0x1000000000a00004ull, 0xfc00000001b00007ull, N("cvt"), T(ftz37), T(sat5), N("f32"), DST, T(acout30), T(neg8), T(abs6), N("f16"), T(is2), HNUM },
	{ 0x1000000001300004ull, 0xfc00000001b00007ull, N("cvt"), T(ftz37), T(sat5), N("f64"), DSTD, T(acout30), T(neg8), T(abs6), N("f32"), T(fs2) },
	{ 0x1400000000000004ull, 0xfc00000000000007ull, N("cvt"), T(ftz37), T(fcrmi), T(cvtf2idst), T(cvtf2isrc) },
	{ 0x1800000000000004ull, 0xfc00000000000007ull, N("cvt"), T(fcrm), T(cvtfdst), T(cvtisrc) },
	{ 0x1c00000000000004ull, 0xfc00000000000007ull, N("cvt"), T(sat5), T(cvtidst), T(cvti2isrc) },
	{ 0x2000000000000004ull, 0xfc00000000000007ull, N("selp"), N("b32"), DST, SRC1, T(is2), T(pnot3), PSRC3 },
	{ 0x2400000000000004ull, 0xfc00000000000007ull, N("prmt"), T(prmtmod), N("b32"), DST, SRC1, T(is2w3), T(is3) },
	{ 0x2800000000000004ull, 0xfc00000000000007ull, T(lane), N("mov"), N("b32"), DST, T(is2) },
	{ 0x2c00000000000004ull, 0xfc00000000000007ull, N("mov"), N("b32"), DST, SREG },
	{ 0x3000000003f00004ull, 0xfc00000003f00007ull, N("mov"), DST, T(high5), FLAGS, N("mask"), T(is2) },
	{ 0x3000000000000004ull, 0xfc00000000000007ull, N("mov"), DST, SRC1, N("or"), T(high5), FLAGS, N("mask"), T(is2) },
	{ 0x3400000000000004ull, 0xfc00000000000007ull, N("mov"), T(high5), FLAGS, SRC1, N("mask"), T(is2) },
	{ 0x3800000000000004ull, 0xfc00000000000007ull, N("bar"), N("read"), DST, BAR },
	// 3c?
	{ 0x4000000000000004ull, 0xfc04000000000007ull, T(cc), N("nop") },
	{ 0x4004000000000004ull, 0xfc04000000000007ull, T(cc), N("pmevent"), PM }, // ... a bitmask of triggered pmevents? with 0 ignored?
	{ 0x4400000000000004ull, 0xfc00000000000007ull, N("lepc"), DST },
	{ 0x4800000000000004ull, 0xfc00000000000067ull, N("vote"), N("all"), DST, PDST2, T(pnot1), PSRC1 },
	{ 0x4800000000000024ull, 0xfc00000000000067ull, N("vote"), N("any"), DST, PDST2, T(pnot1), PSRC1 },
	{ 0x4800000000000044ull, 0xfc00000000000067ull, N("vote"), N("uni"), DST, PDST2, T(pnot1), PSRC1 },
	{ 0x5000000000000004ull, 0xfc000000000000e7ull, N("bar"), N("popc"), PDST3, DST, T(bar), T(tcnt), T(pnot3), PSRC3 }, // and yes, sync is just a special case of this.
	{ 0x5000000000000024ull, 0xfc000000000000e7ull, N("bar"), N("and"), PDST3, DST, T(bar), T(tcnt), T(pnot3), PSRC3 },
	{ 0x5000000000000044ull, 0xfc000000000000e7ull, N("bar"), N("or"), PDST3, DST, T(bar), T(tcnt), T(pnot3), PSRC3 },
	{ 0x5000000000000084ull, 0xfc000000000000e7ull, N("bar"), N("arrive"), PDST3, DST, T(bar), T(tcnt), T(pnot3), PSRC3 },
	{ 0x5400000000000004ull, 0xfc00000000000007ull, N("popc"), DST, T(not9), SRC1, T(not8), T(is2) }, // XXX: popc(SRC1 & SRC2)? insane idea, but I don't have any better
	{ 0x5800000000000004ull, 0xfc00000000000007ull, N("suclamp"), T(sucdim), T(us32_9), T(sucm), PDST5, DST, SRC1, T(is2), SUIMM, .fmask = F_GK104 }, // src0+suimm coord, src1 limit
	{ 0x5c00000000000004ull, 0xfc00000000000007ull, N("subfm"), T(subfmdim), PDST5, DST, SRC1, T(is2w3), T(is3), .fmask = F_GK104 },
	{ 0x6000000000000004ull, 0xfc00000000000007ull, N("sueau"), DST, SRC1, T(is2w3), T(is3), .fmask = F_GK104 },
	{ 0x8000000000000004ull, 0xfc00000000000187ull, N("vadd4"), T(sat9), T(v4dst), T(v4dmask), T(us8_39), DST, T(acout30), T(v4src1), T(us8_6), SRC1, T(v4src2), T(us8_5), T(v4s2), SRC3  },
	{ 0x8000000000000084ull, 0xfc00000000000187ull, N("vsub4"), T(sat9), T(v4dst), T(v4dmask), T(us8_39), DST, T(acout30), T(v4src1), T(us8_6), SRC1, T(v4src2), T(us8_5), T(v4s2), SRC3  },
	{ 0x8000000000000104ull, 0xfc00000000000187ull, N("vsubr4"), T(sat9), T(v4dst), T(v4dmask), T(us8_39), DST, T(acout30), T(v4src1), T(us8_6), SRC1, T(v4src2), T(us8_5), T(v4s2), SRC3  },
	{ 0x8000000000000184ull, 0xfc00000000000187ull, N("vavg4"), T(sat9), T(v4dst), T(v4dmask), T(us8_39), DST, T(acout30), T(v4src1), T(us8_6), SRC1, T(v4src2), T(us8_5), T(v4s2), SRC3  },
	{ 0x8400000000000004ull, 0xfc00000000000087ull, N("vmin4"), T(sat9), T(v4dst), T(v4dmask), T(us8_39), DST, T(acout30), T(v4src1), T(us8_6), SRC1, T(v4src2), T(us8_5), T(v4s2), SRC3  },
	{ 0x8400000000000084ull, 0xfc00000000000087ull, N("vmax4"), T(sat9), T(v4dst), T(v4dmask), T(us8_39), DST, T(acout30), T(v4src1), T(us8_6), SRC1, T(v4src2), T(us8_5), T(v4s2), SRC3  },
	{ 0x8800000000000004ull, 0xfc00000000000007ull, N("vabsdiff4"), T(sat9), T(v4dst), T(v4dmask), T(us8_39), DST, T(acout30), T(v4src1), T(us8_6), SRC1, T(v4src2), T(us8_5), T(v4s2), SRC3  },
	{ 0x8c00000000000004ull, 0xfc00000000000007ull, N("vset4"), T(v4dst), T(v4dmask), DST, T(acout30), T(vsetop), T(v4src1), T(us8_6), SRC1, T(v4src2), T(us8_5), T(v4s2), SRC3  },
	{ 0x9000000000000004ull, 0xfc00000000000007ull, N("vshr4"), T(vsclamp), T(sat9), T(v4dst), T(v4dmask), T(us8_39), DST, T(acout30), T(v4src1), T(us8_6), SRC1, T(v4src2), T(v4s2), SRC3  },
	{ 0x9400000000000004ull, 0xfc00000000000007ull, N("vshl4"), T(vsclamp), T(sat9), T(v4dst), T(v4dmask), T(us8_39), DST, T(acout30), T(v4src1), T(us8_6), SRC1, T(v4src2), T(v4s2), SRC3  },
	{ 0x9800000000000004ull, 0xfc00000000000007ull, N("vsel4"), T(sat9), T(v4dst), T(v4dmask), T(us8_39), DST, T(acout30), T(v4src1), T(us8_6), SRC1, T(v4src2), T(us8_5), T(v4s2), T(pnot3), PSRC3  },
	{ 0xa000000000000004ull, 0xfc00000000000187ull, N("vadd2"), T(sat9), T(v4dst), T(v2dmask), T(us16_39), DST, T(acout30), T(v2src1), T(us16_6), SRC1, T(v2src2), T(us16_5), T(vs2), SRC3  },
	{ 0xa000000000000084ull, 0xfc00000000000187ull, N("vsub2"), T(sat9), T(v4dst), T(v2dmask), T(us16_39), DST, T(acout30), T(v2src1), T(us16_6), SRC1, T(v2src2), T(us16_5), T(vs2), SRC3  },
	{ 0xa000000000000104ull, 0xfc00000000000187ull, N("vsubr2"), T(sat9), T(v4dst), T(v2dmask), T(us16_39), DST, T(acout30), T(v2src1), T(us16_6), SRC1, T(v2src2), T(us16_5), T(vs2), SRC3  },
	{ 0xa000000000000184ull, 0xfc00000000000187ull, N("vavg2"), T(sat9), T(v4dst), T(v2dmask), T(us16_39), DST, T(acout30), T(v2src1), T(us16_6), SRC1, T(v2src2), T(us16_5), T(vs2), SRC3  },
	{ 0xa400000000000004ull, 0xfc00000000000087ull, N("vmin2"), T(sat9), T(v4dst), T(v2dmask), T(us16_39), DST, T(acout30), T(v2src1), T(us16_6), SRC1, T(v2src2), T(us16_5), T(vs2), SRC3  },
	{ 0xa400000000000084ull, 0xfc00000000000087ull, N("vmax2"), T(sat9), T(v4dst), T(v2dmask), T(us16_39), DST, T(acout30), T(v2src1), T(us16_6), SRC1, T(v2src2), T(us16_5), T(vs2), SRC3  },
	{ 0xa800000000000004ull, 0xfc00000000000007ull, N("vabsdiff2"), T(sat9), T(v4dst), T(v2dmask), T(us16_39), DST, T(acout30), T(v2src1), T(us16_6), SRC1, T(v2src2), T(us16_5), T(vs2), SRC3  },
	{ 0xac00000000000004ull, 0xfc00000000000007ull, N("vset2"), T(v4dst), T(v2dmask), DST, T(acout30), T(vsetop), T(v2src1), T(us16_6), SRC1, T(v2src2), T(us16_5), T(vs2), SRC3  },
	{ 0xb000000000000004ull, 0xfc00000000000007ull, N("vshr2"), T(vsclamp), T(sat9), T(v4dst), T(v2dmask), T(us16_39), DST, T(acout30), T(v2src1), T(us16_6), SRC1, T(v2src2), T(vs2), SRC3  },
	{ 0xb400000000000004ull, 0xfc00000000000007ull, N("vshl2"), T(vsclamp), T(sat9), T(v4dst), T(v2dmask), T(us16_39), DST, T(acout30), T(v2src1), T(us16_6), SRC1, T(v2src2), T(vs2), SRC3  },
	{ 0xb800000000000004ull, 0xfc00000000000007ull, N("vsel2"), T(sat9), T(v4dst), T(v2dmask), T(us16_39), DST, T(acout30), T(v2src1), T(us16_6), SRC1, T(v2src2), T(us16_5), T(vs2), T(pnot3), PSRC3  },
	{ 0xc000000000000004ull, 0xf800000000000187ull, N("vadd"), T(sat9), T(vdst), T(us32_2a), DST, T(acout30), T(vsrc1), T(us32_6), SRC1, T(vsrc2), T(us32_5), T(vs2), SRC3  },
	{ 0xc000000000000084ull, 0xf800000000000187ull, N("vsub"), T(sat9), T(vdst), T(us32_2a), DST, T(acout30), T(vsrc1), T(us32_6), SRC1, T(vsrc2), T(us32_5), T(vs2), SRC3  },
	{ 0xc000000000000104ull, 0xf800000000000187ull, N("vsubr"), T(sat9), T(vdst), T(us32_2a), DST, T(acout30), T(vsrc1), T(us32_6), SRC1, T(vsrc2), T(us32_5), T(vs2), SRC3  },
	{ 0xc000000000000184ull, 0xf800000000000187ull, N("vaddpo"), T(sat9), T(vdst), T(us32_2a), DST, T(acout30), T(vsrc1), T(us32_6), SRC1, T(vsrc2), T(us32_5), T(vs2), SRC3  },
	{ 0xc800000000000004ull, 0xf800000000000087ull, N("vmin"), T(sat9), T(vdst), T(us32_2a), DST, T(acout30), T(vsrc1), T(us32_6), SRC1, T(vsrc2), T(us32_5), T(vs2), SRC3  },
	{ 0xc800000000000084ull, 0xf800000000000087ull, N("vmax"), T(sat9), T(vdst), T(us32_2a), DST, T(acout30), T(vsrc1), T(us32_6), SRC1, T(vsrc2), T(us32_5), T(vs2), SRC3  },
	{ 0xd000000000000004ull, 0xf800000000000007ull, N("vabsdiff"), T(sat9), T(vdst), T(us32_2a), DST, T(acout30), T(vsrc1), T(us32_6), SRC1, T(vsrc2), T(us32_5), T(vs2), SRC3  },
	{ 0xd800000000000004ull, 0xf800000000000007ull, N("vset"), T(vdst), DST, T(acout30), T(vsetop), T(vsrc1), T(us32_6), SRC1, T(vsrc2), T(us32_5), T(vs2), SRC3  },
	{ 0xe000000000000004ull, 0xf800000000000007ull, N("vshr"), T(vsclamp), T(sat9), T(vdst), T(us32_2a), DST, T(acout30), T(vsrc1), T(us32_6), SRC1, T(vsrc2), T(us32_5), T(vs2), SRC3  },
	{ 0xe800000000000004ull, 0xf800000000000007ull, N("vshl"), T(vsclamp), T(sat9), T(vdst), T(us32_2a), DST, T(acout30), T(vsrc1), T(us32_6), SRC1, T(vsrc2), T(us32_5), T(vs2), SRC3  },
	{ 0xf000000000000004ull, 0xf800000000000007ull, N("vmad"), T(vmop), T(sat9), T(vmshr), DST, T(acout30), T(vsrc1), T(us32_6), SRC1, T(vsrc2), T(us32_5), T(vs2), SRC3  },
	{ 0xf800000000000004ull, 0xf800000000000007ull, N("vset"), PDST, PDSTN, T(vsetop), T(vsrc1), T(us32_6), SRC1, T(vsrc2), T(us32_5), T(vs2), T(setlop)  },


	{ 0x1000000000000005ull, 0xf800000000000207ull, N("red"), T(redop), N("u32"), T(gmem), DST },
	{ 0x1000000000000205ull, 0xf800000000000207ull, N("add"), N("u64"), T(gmem), DSTD },
	{ 0x1800000000000205ull, 0xf800000000000207ull, N("red"), T(redops), N("s32"), T(gmem), DST },
	{ 0x2800000000000205ull, 0xf8000000000003e7ull, N("red add"), N("f32"), T(gmem), DST },
	{ 0x507e000000000005ull, 0xf87e000000000307ull, N("ld"), T(redop), N("u32"), DST2, T(gamem), DST }, // yet another big ugly mess. but seems to work.
	{ 0x507e000000000205ull, 0xf87e0000000003e7ull, N("ld"), N("add"), N("u64"), DST2, T(gamem), DST },
	{ 0x507e000000000105ull, 0xf87e0000000003e7ull, N("exch"), N("b32"), DST2, T(gamem), DST },
	{ 0x507e000000000305ull, 0xf87e0000000003e7ull, N("exch"), N("b64"), DST2D, T(gamem), DSTD },
	{ 0x5000000000000125ull, 0xf8000000000003e7ull, N("cas"), N("b32"), DST2, T(gamem), DST, SRC3 },
	{ 0x5000000000000325ull, 0xf8000000000003e7ull, N("cas"), N("b64"), DST2D, T(gamem), DSTD, SRC3D },
	{ 0x587e000000000205ull, 0xf87e000000000307ull, N("ld"), T(redops), N("s32"), DST2, T(gamem), DST },
	{ 0x687e000000000205ull, 0xf87e0000000003e7ull, N("ld"), N("add"), N("f32"), DST2, T(gamem), DST },
	{ 0x8000000000000005ull, 0xf800000000000007ull, N("ld"), T(ldstt), T(ldstd), T(lcop), T(gmem) },
	{ 0x8800000000000005ull, 0xf800000000000007ull, N("ldu"), T(ldstt), T(ldstd), T(gmem), .fmask = F_GF100 },
	{ 0x8800000000000005ull, 0xf800000000000007ull, N("shfl"), T(shfl1), PDSTL, DST, SRC1, T(shfl2), .fmask = F_GK104 },
	{ 0x9000000000000005ull, 0xf800000000000007ull, N("st"), T(ldstt), T(scop), T(gmem), T(ldstd) },
	{ 0x9800000000000005ull, 0xf800000000000007ull, N("cctl"), T(cctlop), T(cctlmod), DST, T(gcmem) },
	{ 0xa000000000000005ull, 0xf800000000000007ull, N("ld"), N("lock"), T(ldstt), PDSTL, T(ldstd), GLOBAL, .fmask = F_GF100 },
	{ 0xa800000000000005ull, 0xf800000000000007ull, N("ld"), T(ldulddst2), T(lduldsrc2s), N("ldu"), T(ldulddst1), T(lduldsrc1g), .fmask = F_GF100 },
	{ 0xa800000000000005ull, 0xf800000000000007ull, N("ld"), N("lock"), PDSTL, T(ldstd), SHARED, .fmask = F_GK104 },
	{ 0xb000000000000005ull, 0xf000000000000007ull, N("ld"), T(ldulddst2), T(lduldsrc2g), N("ldu"), T(ldulddst1), T(lduldsrc1g), .fmask = F_GF100 },
	{ 0xb800000000000005ull, 0xf800000000000007ull, N("st"), N("unlock"), PDSTL, SHARED, T(ldstd), .fmask = F_GK104 },
	{ 0xc000000000000005ull, 0xfd00000000000007ull, N("ld"), T(ldstt), T(ldstd), T(lcop), LOCAL },
	{ 0xc100000000000005ull, 0xfd00000000000007ull, N("ld"), T(ldstt), T(ldstd), SHARED },
	{ 0xc400000000000005ull, 0xfc00000000000007ull, N("ld"), N("lock"), T(ldstt), PDST4, T(ldstd), SHARED, .fmask = F_GF100 },
	{ 0xc800000000000005ull, 0xfd00000000000007ull, N("st"), T(ldstt), T(scop), LOCAL, T(ldstd) },
	{ 0xc900000000000005ull, 0xfd00000000000007ull, N("st"), T(ldstt), SHARED, T(ldstd) },
	{ 0xcc00000000000005ull, 0xfc00000000000007ull, N("st"), N("unlock"), T(ldstt), SHARED, T(ldstd), .fmask = F_GF100 },
	{ 0xd000000000000005ull, 0xfc00000000000007ull, N("cctl"), T(cctlop), DST, LCMEM },
	{ 0xd400000000000005ull, 0xfc00000000000007ull, N("suldgb"), T(ldstt), T(ldstd), T(lcop), T(sclamp2l), T(sugty), GLOBALDSU, T(sus2), T(sup), .fmask = F_GK104 },
	{ 0xd400000000000005ull, 0xfc1e000000000007ull, N("suldb"), T(ldstt), T(ldstd), T(lcop), T(sclamp), T(suaddr), T(sudim), .fmask = F_GF100 },
	{ 0xd800400100000005ull, 0xfc00400100000007ull, N("suredp"), T(redop), T(sclamp), SURF, SADDR, DST },
	{ 0xdc00000000000005ull, 0xffc0000000000007ull, N("sustgb"), T(ldstt), T(scop), T(sclamp2s), T(sugty), GLOBALDSU, T(sus2), T(ldstd), T(sup), .fmask = F_GK104 },
	{ 0xdc00000000000005ull, 0xfc00000000000007ull, N("sustgp"), T(scop), T(sclamp2s), T(sugty), GLOBALDSU, T(sus2), SUSTPSRC_GK104, T(sup), .fmask = F_GK104 },
	{ 0xdc00000000000005ull, 0xfc1e000000000007ull, N("sustb"), T(scop), T(sclamp), T(suaddr), T(sudim), T(ldstt), T(ldstd), .fmask = F_GF100 },
	{ 0xdc00000000000005ull, 0xfc00000000000007ull, N("sustp"), T(scop), T(sclamp), T(suaddr), T(sudim), SUSTPSRC_GF100, .fmask = F_GF100 },
	{ 0xe000000000000005ull, 0xfc00000000000067ull, N("membar"), N("cta") },
	{ 0xe000000000000025ull, 0xfc00000000000067ull, N("membar"), N("gl") },
	{ 0xe000000000000045ull, 0xfc00000000000067ull, N("membar"), N("sys") },
	{ 0xe400000000000005ull, 0xfc00000000000007ull, N("suq"), T(suq), DST, T(suaddr), .fmask = F_GF100 },
	{ 0xe800000000000005ull, 0xfc00000000000007ull, N("st"), N("unlock"), T(ldstt), GLOBAL, T(ldstd) },
	{ 0xf000000000000005ull, 0xfc00000000000007ull, N("suleab"), PDST2, DSTD, T(ldstt), T(sclamp), T(suaddr), T(sudim), .fmask = F_GF100 },
	{ 0x0000000000000005ull, 0x0000000000000007ull, OOPS },

	{ 0x0000000000000006ull, 0xfc00000000000007ull, N("ld"), DST, VBASRC },
	/* XXX: what was bit 0x39 for, again? */
	{ 0x0400000000000006ull, 0xfc00000000000007ull, N("ld"), T(patch), T(ldvf), ADST, T(outa), ATTR, SRC2, T(unk39) },
	{ 0x0800000000000006ull, 0xfc00000000000007ull, N("st"), T(patch), T(ldvf), ATTR, ASRC, SRC3, T(unk39) },
	{ 0x0c00000000000006ull, 0xfc00000000000007ull, N("al2p"), DST, T(outa), SRC1, TEXIMM, .fmask = F_GK104 },
	{ 0x1000000000000006ull, 0xfc00000000000007ull, N("pixinfo"), DST, PDST3, T(pixinfo), PIX },
	{ 0x1400000000000006ull, 0xfc00000000000007ull, N("ld"), T(ldstt), T(ldstd), FCONST },
	{ 0x1c00000000000006ull, 0xfc00000000000007ull, N("out"), T(emit), T(restart), DST, SRC1, T(is2) },
	{ 0x8000000000000006ull, 0xf000000000000007ull, N("tex"), T(texm), T(lodt), T(texshd), T(texoff), T(texf), TDST, T(text), T(texi), TEX, SAMP, T(texsrc1), T(texsrc2) },
	{ 0x9000000000000006ull, 0xf000000000000007ull, N("texfetch"), T(texm), T(lodf), T(texcl), T(texms), T(texoff), T(ltex), TDST, T(text), T(texi), TEX, SAMP, T(texsrc1), T(texfsrc2) },
	{ 0xa000000000000006ull, 0xf000000000000007ull, N("texgather"), T(texm), T(texshd), T(toffg), T(texf), T(texrgba), TDST, T(text), T(texi), TEX, SAMP, T(tex4src1), T(tex4src2) },
	{ 0xb000000000000006ull, 0xf000000000000007ull, N("texquerylod"), T(texm), T(texf), TDST, T(text), T(texi), TEX, SAMP, T(texsrc1) },
	{ 0xc000000000000006ull, 0xf000000000000007ull, N("texquery"), T(texm), T(ltex), TDST, T(texquery), T(texi), TEX, SAMP, SRC1, SRC2 },
	{ 0xd000000000000006ull, 0xf000000000000007ull, N("texcsaa"), T(texm), T(texf), TDST, TEX, SAMP, TSRC13 },
	{ 0xe000000000000006ull, 0xf000000000000007ull, N("texgrad"), T(texm), T(texoff), T(ltex), TDST, T(text), T(texi), TEX, SAMP, T(texgrsrc1), T(texgrsrc2) },
	{ 0xf000000000000006ull, 0xf000000000000007ull, N("texbar"), T(cc), TEXBARIMM, .fmask = F_GK104 }, // imm = max outstanding texes after barrier
	{ 0x0000000000000006ull, 0x0000000000000007ull, OOPS, T(texf), TDST, TEX, SAMP, SRC1, SRC2 },



	{ 0x0, 0x0, OOPS, DST, SRC1, T(is2), SRC3 },
};

static struct insn tabp[] = {
	{ 0x1c00, 0x3c00 },
	{ 0x3c00, 0x3c00, N("never") },	// probably.
	{ 0x0000, 0x2000, PRED },
	{ 0x2000, 0x2000, SESTART, N("not"), PRED, SEEND },
	{ 0, 0, OOPS },
};

F1(brawarp, 0xf, N("allwarp")) // probably jumps if the whole warp has the predicate evaluate to true.
F1(brawarps, 0x1c, N("allwarp"))
F1(lim, 0x10, N("lim"))
F1(lims, 0x1d, N("lim"))
F1(clims, 5, N("lim"))
F1(terminate, 0xe, N("terminate"))

static struct insn tabbtarg[] = {
	{ 0x0000000000000000ull, 0x0000000000004000ull, BTARG },
	{ 0x0000000000004000ull, 0x0000000000004000ull, N("pcrel"), FCONSTNR },
	{ 0, 0, OOPS },
};

static struct insn tabc[] = {
	{ 0x0000000000000007ull, 0xf800000000004007ull, T(p), T(cc), N("bra"), T(lim), T(brawarp), N("abs"), ABTARG },
	{ 0x0000000000004007ull, 0xf800000000004007ull, T(p), T(cc), N("bra"), T(lim), T(brawarp), FCONSTNR },
	{ 0x0800000000000007ull, 0xf800000000004007ull, T(p), T(cc), N("bra"), T(lim), SRC1, N("abs"), ANTARG },
	{ 0x0800000000004007ull, 0xf800000000004007ull, T(p), T(cc), N("bra"), T(lim), FCONST },
	{ 0x1000000000000007ull, 0xf800000000004007ull, IGNPRED, N("call"), T(lim), N("abs"), ACTARG },
	{ 0x1000000000004007ull, 0xf800000000004007ull, IGNPRED, N("call"), T(lim), FCONSTNR },
	{ 0x2000000000000007ull, 0xf000000000000007ull, N("sched"), SCHED(0), SCHED(1), SCHED(2), SCHED(3), SCHED(4), SCHED(5), SCHED(6), .fmask = F_GK104 },
	{ 0x4000000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("bra"), T(lim), T(brawarp), T(btarg) },
	{ 0x4800000000000007ull, 0xf800000000004007ull, T(p), T(cc), N("bra"), T(lim), SRC1, NTARG },
	{ 0x4800000000004007ull, 0xf800000000004007ull, T(p), T(cc), N("bra"), T(lim), N("pcrel"), FCONST },
	{ 0x5000000000000007ull, 0xf800000000004007ull, IGNPRED, N("call"), T(lim), CTARG },
	{ 0x5000000000004007ull, 0xf800000000004007ull, IGNPRED, N("call"), T(lim), N("pcrel"), FCONSTNR },
	{ 0x5800000000000007ull, 0xf800000000000007ull, IGNPRED, N("prelongjmp"), T(btarg) },
	{ 0x6000000000000007ull, 0xf800000000000007ull, IGNPRED, N("joinat"), T(btarg) },
	{ 0x6800000000000007ull, 0xf800000000000007ull, IGNPRED, N("prebrk"), T(btarg) },
	{ 0x7000000000000007ull, 0xf800000000000007ull, IGNPRED, N("precont"), T(btarg) },
	{ 0x7800000000000007ull, 0xf800000000000007ull, IGNPRED, N("preret"), T(lim), T(btarg) },
	{ 0x8000000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("exit") },
	{ 0x8800000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("longjmp") },
	{ 0x9000000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("ret") },
	{ 0x9800000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("discard") },
	{ 0xa000000000000007ull, 0xf800000000000007ull, IGNPRED, N("rtt"), T(terminate) },
	{ 0xa800000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("brk") },
	{ 0xb000000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("cont") },
	{ 0xc000000000000007ull, 0xf800000000000007ull, IGNPRED, N("quadon") },
	{ 0xc800000000000007ull, 0xf800000000000007ull, IGNPRED, N("quadpop") },
	{ 0xd000000000000007ull, 0xf80000000000c007ull, IGNPRED, N("bpt"), N("drain"), BPT },
	{ 0xd000000000004007ull, 0xf80000000000c007ull, IGNPRED, N("bpt"), N("cal"), BPT },
	{ 0xd000000000008007ull, 0xf80000000000c007ull, IGNPRED, N("bpt"), N("pause"), BPT },
	{ 0xd00000000000c007ull, 0xf80000000000c007ull, IGNPRED, N("bpt"), N("trap"), BPT },
	{ 0x0000000000000007ull, 0x0000000000000007ull, T(p), OOPS, BTARG },
	{ 0, 0, OOPS },
};

static struct insn tabpsrcs[] = {
	{ 0x0001c000, 0x0601c000, T(pnot1), PSRC1 },
	{ 0x00000000, 0x06000000, T(pnot1), PSRC1, N("and"), T(pnotn), PDSTN },
	{ 0x02000000, 0x06000000, T(pnot1), PSRC1, N("or"), T(pnotn), PDSTN },
	{ 0x04000000, 0x06000000, T(pnot1), PSRC1, N("xor"), T(pnotn), PDSTN },
	{ 0, 0, OOPS },
};

static struct insn tabi2isd[] = {
	{ 0x00000000, 0x00300000, N("u16") },
	{ 0x00100000, 0x00300000, N("s16") },
	{ 0x00200000, 0x00300000, N("u32") },
	{ 0x00300000, 0x00300000, N("s32") },
	{ 0, 0, OOPS },
};

static struct insn tabi2iss[] = {
	{ 0x00000000, 0x01000200, N("u16") },
	{ 0x00000200, 0x01000200, N("s16") },
	{ 0x01000000, 0x01000200, N("u32") },
	{ 0x01000200, 0x01000200, N("s32") },
	{ 0, 0, OOPS },
};

static struct insn tabss1[] = {
	{ 0x00000000, 0x00000300, SRC1 },
	{ 0x00000100, 0x00000300, SCONST1_0 },
	{ 0x00000200, 0x00000300, SCONST1_1 },
	{ 0x00000300, 0x00000300, SCONST1_16 },
	{ 0, 0, OOPS },
};

static struct insn tabss2[] = {
	{ 0x00000000, 0x00000300, SRC2 },
	{ 0x00000100, 0x00000300, SCONST2_0 },
	{ 0x00000200, 0x00000300, SCONST2_1 },
	{ 0x00000300, 0x00000300, SCONST2_16 },
	{ 0, 0, OOPS },
};

static struct insn tabss2a[] = {
	{ 0x00000000, 0x000000c0, SRC2 },
	{ 0x00000040, 0x000000c0, SCONST2_0 },
	{ 0x00000080, 0x000000c0, SCONST2_1 },
	{ 0x000000c0, 0x000000c0, SCONST2_16 },
	{ 0, 0, OOPS },
};

static struct insn tabss3a[] = {
	{ 0x00000040, 0x000000c0, SCONST3_0 },
	{ 0x00000080, 0x000000c0, SCONST3_1 },
	{ 0x000000c0, 0x000000c0, SCONST3_16 },
	{ 0, 0, OOPS },
};

static struct insn tabrms[] = {
	{ 0x00000000, 0x00300000, N("rn") },
	{ 0x00100000, 0x00300000, N("rm") },
	{ 0x00200000, 0x00300000, N("rp") },
	{ 0x00300000, 0x00300000, N("rz") },
	{ 0, 0, OOPS },
};

static struct insn tabrmsi[] = {
	{ 0x00000000, 0x00300000, N("rni") },
	{ 0x00100000, 0x00300000, N("rmi") },
	{ 0x00200000, 0x00300000, N("rpi") },
	{ 0x00300000, 0x00300000, N("rzi") },
	{ 0, 0, OOPS },
};

static struct insn tabsetits[] = {
	{ 0x00000000, 0x0001c000, N("false") },
	{ 0x00004000, 0x0001c000, N("lt") },
	{ 0x00008000, 0x0001c000, N("eq") },
	{ 0x0000c000, 0x0001c000, N("le") },
	{ 0x00010000, 0x0001c000, N("gt") },
	{ 0x00014000, 0x0001c000, N("ne") },
	{ 0x00018000, 0x0001c000, N("ge") },
	{ 0x0001c000, 0x0001c000, N("true") },
	{ 0, 0, OOPS },
};

static struct insn tabsetfts[] = {
	{ 0x00000000, 0x0001c080, N("false") },
	{ 0x00000080, 0x0001c080, N("lt") },
	{ 0x00004000, 0x0001c080, N("eq") },
	{ 0x00004080, 0x0001c080, N("le") },
	{ 0x00008000, 0x0001c080, N("gt") },
	{ 0x00008080, 0x0001c080, N("ne") },
	{ 0x0000c000, 0x0001c080, N("ge") },
	{ 0x0000c080, 0x0001c080, N("num") },
	{ 0x00010000, 0x0001c080, N("nan") },
	{ 0x00010080, 0x0001c080, N("ltu") },
	{ 0x00014000, 0x0001c080, N("equ") },
	{ 0x00014080, 0x0001c080, N("leu") },
	{ 0x00018000, 0x0001c080, N("gtu") },
	{ 0x00018080, 0x0001c080, N("neu") },
	{ 0x0001c000, 0x0001c080, N("geu") },
	{ 0x0001c080, 0x0001c080, N("true") },
	{ 0, 0, OOPS },
};

static struct insn tabvmadss1[] = {
	{ 0x00000000, 0x00000e00, N("b0") },
	{ 0x00000200, 0x00000e00, N("b1") },
	{ 0x00000400, 0x00000e00, N("b2") },
	{ 0x00000600, 0x00000e00, N("b3") },
	{ 0x00000800, 0x00000e00 },
	{ 0x00000a00, 0x00000f80, N("b0") },
	{ 0x00000a80, 0x00000f80, N("b1") },
	{ 0x00000b00, 0x00000f80, N("b2") },
	{ 0x00000b80, 0x00000f80, N("b3") },
	{ 0x00000c00, 0x00000f00, N("h0") },
	{ 0x00000d00, 0x00000f00, N("h1") },
	{ 0x00000e00, 0x00000f00 },
	{ 0x00000f00, 0x00000f80, N("h0") },
	{ 0x00000f80, 0x00000f80, N("h1") },
	{ 0, 0, OOPS },
};

static struct insn tabvmadss2[] = {
	{ 0x00000000, 0x00000980, N("b0") },
	{ 0x00000080, 0x00000980, N("b1") },
	{ 0x00000100, 0x00000980, N("b2") },
	{ 0x00000180, 0x00000980, N("b3") },
	{ 0x00000800, 0x00000f80, N("b0") },
	{ 0x00000880, 0x00000f80, N("b1") },
	{ 0x00000900, 0x00000f80, N("b2") },
	{ 0x00000980, 0x00000f80, N("b3") },
	{ 0x00000a00, 0x00000e00 },
	{ 0x00000c00, 0x00000e80, N("h0") },
	{ 0x00000c80, 0x00000e80, N("h1") },
	{ 0x00000e00, 0x00000f80, N("h0") },
	{ 0x00000e80, 0x00000f80, N("h1") },
	{ 0x00000f00, 0x00000f00 },
	{ 0, 0, OOPS },
};

static struct insn tabinterpmodes[] = {
	{ 0x0000000000000000ull, 0x0000000000000080ull, N("mul") },
	{ 0x0000000000000080ull, 0x0000000000000080ull, N("sc") },
	{ 0, 0, OOPS },
};


static struct insn tabs[] = {
	{ 0x00000008, 0xf80003ff, T(p), N("nop") },
	{ 0x10000008, 0xf80003ff, T(p), N("set"), PDST, T(setct), CC },
	{ 0x18000008, 0xf80003ff, T(p), N("set"), PDST, T(psrcs) },
	{ 0x40000008, 0xf00003ff, T(p), N("mov"), N("b32"), DST, SREGS },
	{ 0x50000008, 0xf80003ff, T(p), N("bar"), N("sync"), T(bars) },
	{ 0x58000008, 0xf80003ff, T(p), N("bar"), N("arrive"), T(bars) },
	{ 0x70000008, 0xf40003ff, T(p), N("presin"), N("f32"), DST, SRC1 },
	{ 0x74000008, 0xf40003ff, T(p), N("preex2"), N("f32"), DST, SRC1 },
	{ 0x80000008, 0xbc0003ff, T(p), N("cos"), N("f32"), DST, T(abs1e), SRC1 },
	{ 0x84000008, 0xbc0003ff, T(p), N("sin"), N("f32"), DST, T(abs1e), SRC1 },
	{ 0x88000008, 0xbc0003ff, T(p), N("ex2"), N("f32"), DST, T(abs1e), SRC1 },
	{ 0x8c000008, 0xbc0003ff, T(p), N("lg2"), N("f32"), DST, T(abs1e), SRC1 },
	{ 0x90000008, 0xbc0003ff, T(p), N("rcp"), N("f32"), DST, T(abs1e), SRC1 },
	{ 0x94000008, 0xbc0003ff, T(p), N("rsqrt"), N("f32"), DST, T(abs1e), SRC1 },
	{ 0x98000008, 0xbc0003ff, T(p), N("rcp64h"), DST, T(abs1e), SRC1 },
	{ 0x9c000008, 0xbc0003ff, T(p), N("rsqrt64h"), DST, T(abs1e), SRC1 },
	{ 0x00000208, 0x800003ff, T(p), N("shl"), N("b32"), DST, SRC1, SHCNTS },
	{ 0x80000208, 0x800003ff, T(p), N("shr"), N("s32"), DST, SRC1, SHCNTS },
	{ 0x00000108, 0x000007ff, N("vabsdiff2"), T(us16_d), DST, T(us16_c), SRC1, T(us16_b), SRC2 },
	{ 0x00000308, 0x00000fff, N("vadd2"), T(us16_d), DST, T(us16_c), SRC1, T(us16_c), SRC2 },
	{ 0x00000b08, 0x00000fff, N("vsubr2"), T(us16_d), DST, T(us16_c), SRC1, T(us16_c), SRC2 },
	{ 0x00000508, 0x000007ff, N("vabsdiff4"), T(us8_d), DST, T(us8_c), SRC1, T(us8_b), SRC2 },
	{ 0x00000708, 0x00000fff, N("vadd4"), T(us8_d), DST, T(us8_c), SRC1, T(us8_c), SRC2 },
	{ 0x00000f08, 0x00000fff, N("vsubr4"), T(us8_d), DST, T(us8_c), SRC1, T(us8_c), SRC2 },

	{ 0x00000018, 0x000001ff, T(p), N("cvt"), T(i2isd), DST, T(acout17), T(neg16), T(abs19), T(i2iss), SRC2 },
	{ 0x00000118, 0x000003ff, T(p), N("mov"), N("b32"), DST, IMMS },
	{ 0x00000318, 0x000003ff, T(p), N("mov"), N("b32"), DST, FIMMS },

	{ 0x00000028, 0x000000ff, T(p), N("mov"), N("b32"), DST, T(ss1) },

	{ 0x00000088, 0x000003ff, T(p), N("cvt"), T(rms), N("f32"), DST, T(acout17), T(neg16), T(abs19), T(us32_18), SRC2 },
	{ 0x00000288, 0x000003ff, T(p), N("cvt"), T(rmsi), T(us32_18), DST, T(acout17), T(neg16), T(abs19), N("f32"), SRC2 },
	{ 0x00000098, 0x000003ff, T(p), N("cvt"), T(sat18), N("f32"), DST, T(acout17), T(neg16), T(abs19), N("f32"), SRC2 },
	{ 0x00000298, 0x000003ff, T(p), N("cvt"), T(sat18), T(rmsi), N("f32"), DST, T(acout17), T(neg16), T(abs19), N("f32"), SRC2 },

	{ 0x000000a8, 0x000000ff, T(p), N("mul"), N("f32"), DST, SRC1, T(ss2) },

	{ 0x00000048, 0x000000ff, T(p), N("set"), PDST, T(setits), N("s32"), SRC1, T(ss2) },
	{ 0x000000c8, 0x000000ff, T(p), N("set"), PDST, T(setits), N("s32"), SRC1, SIMMS },

	{ 0x00000058, 0x0000007f, N("vmad"), DST, T(us32_d), T(vmadss1), SRC1, T(us32_c), T(vmadss2), SRC2 },

	{ 0x00000009, 0x0000007f, T(p), N("interp"), T(interpmodes), DST, ATTRS2, SRC1 },
	{ 0x00000049, 0x0000007f, T(p), N("add"), N("f32"), DST, T(neg7), SRC1, T(ss2) },
	{ 0x00000059, 0x0000007f, T(p), N("set"), PDST, T(setfts), N("f32"), SRC1, T(ss2) },
	{ 0x00000029, 0x0000003f, T(p), N("ld"), T(ldvfs), ADSTS, ATTRS1, SRC2 },
	{ 0x00000039, 0x0000003f, T(p), N("st"), T(ldvfs), ATTRS1, ASRCS, DST },

	{ 0x0000000a, 0x000000ff, T(p), N("ld"), N("b32"), DST, LOCALS2 },
	{ 0x0000004a, 0x000000ff, T(p), N("ld"), N("b32"), DST, SHAREDS2 },
	{ 0x0000008a, 0x000000ff, T(p), N("ld"), N("b64"), DSTD, LOCALS3 },
	{ 0x000000ca, 0x000000ff, T(p), N("ld"), N("b64"), DSTD, SHAREDS3 },
	{ 0x0000001a, 0x000000ff, T(p), N("st"), N("b32"), LOCALS2, DST },
	{ 0x0000005a, 0x000000ff, T(p), N("st"), N("b32"), SHAREDS2, DST },
	{ 0x0000009a, 0x000000ff, T(p), N("st"), N("b64"), LOCALS3, DSTD },
	{ 0x000000da, 0x000000ff, T(p), N("st"), N("b64"), SHAREDS3, DSTD },
	{ 0x0000002a, 0x000000bf, T(p), N("mul"), DST, T(us32_6), SRC1, T(us32_6), T(ss2) },
	{ 0x000000aa, 0x000000bf, T(p), N("mul"), DST, T(us32_6), SRC1, T(us32_6), SIMMS },

	{ 0x0000000b, 0x000000ff, T(p), N("ld"), N("b32"), DST, GLOBALS2 },
	{ 0x0000004b, 0x000000ff, T(p), N("ld"), N("b32"), DST, GLOBALDS2 },
	{ 0x0000008b, 0x000000ff, T(p), N("ld"), N("b64"), DSTD, GLOBALS3 },
	{ 0x000000cb, 0x000000ff, T(p), N("ld"), N("b64"), DSTD, GLOBALDS3 },
	{ 0x0000001b, 0x000000ff, T(p), N("st"), N("b32"), GLOBALS2, DST },
	{ 0x0000005b, 0x000000ff, T(p), N("st"), N("b32"), GLOBALDS2, DST },
	{ 0x0000009b, 0x000000ff, T(p), N("st"), N("b64"), GLOBALS3, DSTD },
	{ 0x000000db, 0x000000ff, T(p), N("st"), N("b64"), GLOBALDS3, DSTD },

	{ 0x0000002c, 0x000000ff, T(p), N("add"), N("b32"), DST, SRC1, T(ss2) },
	{ 0x0000006c, 0x000000ff, T(p), N("subr"), N("b32"), DST, SRC1, T(ss2) },
	{ 0x000000ac, 0x000000ff, T(p), N("add"), N("b32"), DST, SRC1, SIMMS },
	{ 0x000000ec, 0x000000ff, T(p), N("subr"), N("b32"), DST, SRC1, SIMMS },
	{ 0x0000001c, 0x0000001f, T(p), N("ld"), N("b32"), DST, FCONSTS },

	{ 0x0000000d, 0x0000001f, N("add"), DST, SESTART, N("mul"), T(us32_5), SRC1, T(us32_5), T(ss2a), SEEND, SRC3S },
	{ 0x0000001d, 0x000000ff, T(p), N("and"), N("b32"), DST, SRC1, T(ss2) },
	{ 0x0000003d, 0x000000ff, T(p), N("or"), N("b32"), DST, SRC1, T(ss2) },
	{ 0x0000005d, 0x000000ff, T(p), N("xor"), N("b32"), DST, SRC1, T(ss2) },
	{ 0x0000007d, 0x000000ff, T(p), N("mov2"), N("b32"), DST, SRC1, T(ss2) },
	{ 0x0000009d, 0x000000ff, T(p), N("and"), N("b32"), DST, SRC1, SIMMS },
	{ 0x000000bd, 0x000000ff, T(p), N("or"), N("b32"), DST, SRC1, SIMMS },
	{ 0x000000dd, 0x000000ff, T(p), N("xor"), N("b32"), DST, SRC1, SIMMS },
	{ 0x000000fd, 0x000000ff, T(p), N("mov2"), N("b32"), DST, SRC1, SIMMS },

	{ 0x0000000e, 0x0000002f, N("fma"), N("f32"), DST, T(neg4), SRC1, T(ss2a), SRC3S },
	{ 0x0000002e, 0x0000002f, N("fma"), N("f32"), DST, T(neg4), SRC1, SRC2, T(ss3a) },

	{ 0x0000001f, 0xc000001f, T(p), T(cc), N("bra"), T(lims), T(brawarps), BTARGS },
	{ 0x4000001f, 0xf000001f, N("joinat"), BTARGS },
	{ 0x5000001f, 0xf000001f, N("call"), T(clims), CTARGS },
	{ 0x8000001f, 0xf800007f, N("bpt"), N("drain"), BPTS },
	{ 0x8000003f, 0xf800007f, N("bpt"), N("cal"), BPTS },
	{ 0x8000005f, 0xf800007f, N("bpt"), N("pause"), BPTS },
	{ 0x8000007f, 0xf800007f, N("bpt"), N("trap"), BPTS },
	{ 0x8800001f, 0xf800001f, T(p), T(cc), N("exit") },
	{ 0x9000001f, 0xf800001f, T(p), T(cc), N("ret") },
	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0x8, 0x8, OP4B, T(s) },
	{ 7, 0xf, OP8B, T(c) }, // control instructions, special-cased.
	{ 0x0, 0x18, OP8B, T(p), T(m) },
	{ 0x10, 0x18, OP8B, N("join"), T(p), T(m), }, // affected by $p but not by $c, happens after instruction
	{ 7, 0xf, OP8B, N("long"), T(c) }, // control instructions, special-cased.
	{ 0x0, 0x18, OP8B, N("long"), T(p), T(m) },
	{ 0x10, 0x18, OP8B, N("long"), N("join"), T(p), T(m), },
	{ 0, 0, OOPS },
};

static void gf100_prep(struct disisa *isa) {
	isa->vardata = vardata_new("gf100isa");
	int f_gf100op = vardata_add_feature(isa->vardata, "gf100op", "GF100 family exclusive opcodes");
	int f_gk104op = vardata_add_feature(isa->vardata, "gk104op", "GK104 family exclusive opcodes");
	if (f_gf100op == -1 || f_gk104op == -1)
		abort();
	int vs_chipset = vardata_add_varset(isa->vardata, "chipset", "GPU chipset");
	if (vs_chipset == -1)
		abort();
	int v_gf100 = vardata_add_variant(isa->vardata, "gf100", "GF100:GK104", vs_chipset);
	int v_gk104 = vardata_add_variant(isa->vardata, "gk104", "GK104+", vs_chipset);
	if (v_gf100 == -1 || v_gk104 == -1)
		abort();
	vardata_variant_feature(isa->vardata, v_gf100, f_gf100op);
	vardata_variant_feature(isa->vardata, v_gk104, f_gk104op);
	if (vardata_validate(isa->vardata))
		abort();
}

struct disisa gf100_isa_s = {
	tabroot,
	8,
	4,
	1,
	.i_need_g80as_hack = 1,
	.prep = gf100_prep,
};
