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
 * Registers:
 *
 *  - $r0-$r62: Normal, usable 32-bit regs. Allocated just like on tesla.
 *    Grouped into $r0d-$r60d for 64-bit quantities like doubles, into
 *    $r0q-$r56q for 128-bit quantities. There are no half-regs.
 *  - $r63: Bit bucket on write, 0 on read. Maybe just normal unallocated reg,
 *    like on tesla.
 *  - $p0-$p6: 1-bit predicate registers, usable.
 *  - $p7: Always-true predicate.
 *  - $c0: Condition code register. Don't know exact bits yet, but at least
 *    zero, sign, and carry flags exist. Probably overflow too. Likely same
 *    bits as nv50.
 */

/*
 * Instructions, from PTX manual:
 *   
 *   1. Integer Arithmetic
 *    - add		done
 *    - sub		done
 *    - addc		done
 *    - subc		done
 *    - mul		done
 *    - mul24		done
 *    - mad		done
 *    - mad24		done
 *    - sad		done
 *    - div		TODO
 *    - rem		TODO
 *    - abs		done
 *    - neg		done
 *    - min		done, check predicate selecting min/max
 *    - max		done
 *   2. Floating-Point
 *    - add		done
 *    - sub		done
 *    - mul		done
 *    - fma		done
 *    - mad		done
 *    - div.approxf32	TODO
 *    - div.full.f32	TODO
 *    - div.f64		TODO
 *    - abs		done
 *    - neg		done
 *    - min		done
 *    - max		done
 *    - rcp		TODO started
 *    - sqrt		TODO started
 *    - rsqrt		TODO started
 *    - sin		done
 *    - cos		done
 *    - lg2		TODO started
 *    - ex2		TODO started
 *   3. Comparison and selection
 *    - set		done, check these 3 bits
 *    - setp		done
 *    - selp		done
 *    - slct		done
 *   4. Logic and Shift
 *    - and		done, check not bitfields for all 4
 *    - or		done
 *    - xor		done
 *    - not		done
 *    - cnot		TODO
 *    - shl		TODO started
 *    - shr		TODO started
 *   5. Data Movement and Conversion
 *    - mov		done
 *    - ld		done
 *    - st		done
 *    - cvt		TODO started
 *   6. Texture
 *    - tex		done, needs OpenGL stuff
 *   7. Control Flow
 *    - { }		done
 *    - @		done
 *    - bra		TODO started
 *    - call		TODO started
 *    - ret		done
 *    - exit		done
 *   8. Parallel Synchronization and Communication
 *    - bar		done, but needs orthogonalising when we can test it.
 *    - membar.cta	done, but needs figuring out what each half does.
 *    - membar.gl	done
 *    - membar.sys	done
 *    - atom		TODO
 *    - red		TODO
 *    - vote		done, but needs orthogonalising.
 *   9. Miscellaneous
 *    - trap		done
 *    - brkpt		done
 *    - pmevent		done, but needs figuring out relationship with pm counters.
 *
 */

/*
 * Code target field
 */


static struct bitfield ctargoff = { { 26, 24 }, BF_SIGNED, .pcrel = 1, .addend = 8};
static struct bitfield actargoff = { 26, 32 };
#define BTARG atombtarg, &ctargoff
#define CTARG atomctarg, &ctargoff
#define NTARG atomimm, &ctargoff
#define ABTARG atombtarg, &actargoff
#define ACTARG atomctarg, &actargoff
#define ANTARG atomimm, &actargoff

/*
 * Misc number fields
 */

static struct bitfield baroff = { 0x14, 4 };
static struct bitfield pmoff = { 0x1a, 16 };
static struct bitfield tcntoff = { 0x1a, 12 };
static struct bitfield immoff = { { 0x1a, 20 }, BF_SIGNED };
static struct bitfield fimmoff = { { 0x1a, 20 }, BF_UNSIGNED, 12 };
static struct bitfield dimmoff = { { 0x1a, 20 }, BF_UNSIGNED, 44 };
static struct bitfield limmoff = { { 0x1a, 32 }, .wrapok = 1 };
static struct bitfield shcntoff = { 5, 5 };
static struct bitfield bnumoff = { 0x37, 2 };
static struct bitfield hnumoff = { 0x38, 1 };
#define BAR atomimm, &baroff
#define PM atomimm, &pmoff
#define TCNT atomimm, &tcntoff
#define IMM atomimm, &immoff
#define FIMM atomimm, &fimmoff
#define DIMM atomimm, &dimmoff
#define LIMM atomimm, &limmoff
#define SHCNT atomimm, &shcntoff
#define BNUM atomimm, &bnumoff
#define HNUM atomimm, &hnumoff

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
	{ 0x10, "vtxcnt" }, // gl_PatchVerticesIn
	{ 0x11, "invoc" }, // gl_InvocationID
	{ 0x21, "tidx" },
	{ 0x22, "tidy" },
	{ 0x23, "tidz" },
	{ 0x25, "ctaidx" },
	{ 0x26, "ctaidy" },
	{ 0x27, "ctaidz" },
	{ 0x29, "ntidx" },
	{ 0x2a, "ntidy" },
	{ 0x2b, "ntidz" },
	{ 0x2c, "gridid" },
	{ 0x2d, "nctaidx" },
	{ 0x2e, "nctaidy" },
	{ 0x2f, "nctaidz" },
	{ 0x30, "sbase" },	// the address in g[] space where s[] is.
	{ 0x34, "lbase" },	// the address in g[] space where l[] is.
	{ 0x37, "stackbase" },
	{ 0x38, "lanemask_eq" }, // I have no idea what these do, but ptxas eats them just fine.
	{ 0x39, "lanemask_lt" },
	{ 0x3a, "lanemask_le" },
	{ 0x3b, "lanemask_gt" },
	{ 0x3c, "lanemask_ge" },
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
static struct bitfield tex_bf = { 0x20, 7 };
static struct bitfield samp_bf = { 0x28, 4 };
static struct bitfield surf_bf = { 0x1a, 3 };
static struct bitfield sreg_bf = { 0x1a, 7 };
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
static struct reg tex_r = { &tex_bf, "t", .cool = 1 };
static struct reg samp_r = { &samp_bf, "s", .cool = 1 };
static struct reg surf_r = { &surf_bf, "g", .cool = 1 };
static struct reg cc_r = { 0, "c", .cool = 1 };
static struct reg sreg_r = { &sreg_bf, "sr", .specials = sreg_sr, .always_special = 1 };
static struct reg lduld_dst2_r = { &lduld_dst2_bf, "r" };
static struct reg lduld_dst2d_r = { &lduld_dst2_bf, "r", "d" };

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
#define TEX atomreg, &tex_r
#define SAMP atomreg, &samp_r
#define SURF atomreg, &surf_r
#define CC atomreg, &cc_r
#define SREG atomreg, &sreg_r
#define LDULD_DST2 atomreg, &lduld_dst2_r
#define LDULD_DST2D atomreg, &lduld_dst2d_r

static struct bitfield tdst_cnt = { .addend = 4 };
static struct bitfield tdst_mask = { 0x2e, 4 };
static struct bitfield tsrc_cnt = { { 0x34, 2 }, .addend = 1 };
static struct bitfield saddr_cnt = { { 0x2c, 2 }, .addend = 1 };
static struct bitfield esrc_cnt = { { 5, 2 }, .addend = 1 };
static struct vec tdst_v = { "r", &dst_bf, &tdst_cnt, &tdst_mask };
static struct vec tsrc_v = { "r", &src1_bf, &tsrc_cnt, 0 };
static struct vec saddr_v = { "r", &src1_bf, &saddr_cnt, 0 };
static struct vec esrc_v = { "r", &src2_bf, &esrc_cnt, 0 };
static struct vec vdst_v = { "r", &dst_bf, &esrc_cnt, 0 };
#define TDST atomvec, &tdst_v
#define TSRC atomvec, &tsrc_v
#define SADDR atomvec, &saddr_v
#define ESRC atomvec, &esrc_v
#define VDST atomvec, &vdst_v

/*
 * Memory fields
 */

static struct bitfield gmem_imm = { { 0x1a, 32 }, BF_SIGNED };
static struct bitfield gamem_imm = { { 0x1a, 17, 0x37, 3 }, BF_SIGNED };
static struct bitfield slmem_imm = { { 0x1a, 24 }, BF_SIGNED };
static struct bitfield cmem_imm = { 0x1a, 16 };
static struct bitfield fcmem_imm = { { 0x1a, 16 }, BF_SIGNED };
static struct bitfield vmem_imm = { 0x20, 16 };
static struct bitfield cmem_idx = { 0x2a, 4 };
static struct bitfield vba_imm = { 0x1a, 6 };
static struct bitfield lduld_imm = { 0x29, 11 };
static struct mem gmem_m = { "g", 0, &src1_r, &gmem_imm };
static struct mem gdmem_m = { "g", 0, &src1d_r, &gmem_imm };
static struct mem gamem_m = { "g", 0, &src1_r, &gamem_imm };
static struct mem gadmem_m = { "g", 0, &src1d_r, &gamem_imm };
static struct mem smem_m = { "s", 0, &src1_r, &slmem_imm };
static struct mem lmem_m = { "l", 0, &src1_r, &slmem_imm };
static struct mem fcmem_m = { "c", &cmem_idx, &src1_r, &fcmem_imm };
static struct mem vmem_m = { "v", 0, &src1_r, &vmem_imm };
static struct mem amem_m = { "a", 0, &src1_r, &vmem_imm, &src2_r }; // XXX: wtf?
static struct mem cmem_m = { "c", &cmem_idx, 0, &cmem_imm };
static struct mem lpmem_m = { "l", 0, &src1_r };
static struct mem gpmem_m = { "g", 0, &src1_r };
static struct mem gdpmem_m = { "g", 0, &src1d_r };
static struct mem lduld_gmem1_m = { "g", 0, &src1_r, &lduld_imm };
static struct mem lduld_gdmem1_m = { "g", 0, &src1d_r, &lduld_imm };
static struct mem lduld_gmem2_m = { "g", 0, &src2_r };
static struct mem lduld_gdmem2_m = { "g", 0, &src2d_r };
static struct mem lduld_smem_m = { "s", 0, &src1_r , &lduld_imm };
// vertex base address (for tessellation and geometry programs)
static struct mem vba_m = { 0, 0, &src1_r, &vba_imm };
#define GLOBAL atommem, &gmem_m
#define GLOBALD atommem, &gdmem_m
#define GATOM atommem, &gamem_m
#define GATOMD atommem, &gadmem_m
#define SHARED atommem, &smem_m
#define LOCAL atommem, &lmem_m
#define FCONST atommem, &fcmem_m
#define VAR atommem, &vmem_m
#define ATTR atommem, &amem_m
#define CONST atommem, &cmem_m
#define VBASRC atommem, &vba_m
#define LPMEM atommem, &lpmem_m
#define GPMEM atommem, &gpmem_m
#define GDPMEM atommem, &gdpmem_m
#define LDULD_GLOBAL1 atommem, &lduld_gmem1_m
#define LDULD_GLOBALD1 atommem, &lduld_gdmem1_m
#define LDULD_GLOBAL2 atommem, &lduld_gmem2_m
#define LDULD_GLOBALD2 atommem, &lduld_gdmem2_m
#define LDULD_SHARED atommem, &lduld_smem_m

/*
 * The instructions
 */

F(gmem, 0x3a, GLOBAL, GLOBALD)
F(gamem, 0x3a, GATOM, GATOMD)
F(lduld_gmem1, 0x3b, LDULD_GLOBAL1, LDULD_GLOBALD1)
F(lduld_gmem2, 0x3a, LDULD_GLOBAL2, LDULD_GLOBALD2)

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
	{ 0x60, 0xe0, N("b128") },
	{ 0x40, 0xe0, N("b96") },
	{ 0x20, 0xe0, N("b64") },
	{ 0x00, 0xe0, N("b32") },
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

F(setdt, 5, N("b32"), N("f32"))
F(setdti, 7, N("b32"), N("f32"))

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

static struct insn tabcs2[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull, SRC2 },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, CONST },
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
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, DIMM },
	{ 0, 0, OOPS },
};

F1(ias, 5, N("sat"))
F1(vas, 9, N("sat"))
F1(fas, 0x31, N("sat"))
F1(sat38, 0x38, N("sat"))
F1(faf, 5, N("ftz"))
F1(fmf, 6, N("ftz"))
F1(setftz, 0x3b, N("ftz"))
F1(fmz, 7, N("fmz"))
F1(fmneg, 0x39, N("neg"))
F1(neg1, 9, N("neg"))
F1(neg2, 8, N("neg"))
F1(abs1, 7, N("abs"))
F1(abs2, 6, N("abs"))
F1(rint, 7, N("rint"))
F1(rev, 8, N("rev"))
F1(nowrap, 9, N("nowrap"))

F1(not1, 9, N("not"))
F1(not2, 8, N("not"))

F1(shiftamt, 6, N("shiftamt"))

F1(acout, 0x30, CC)
F1(acout2, 0x3a, CC)
F1(acin, 6, CC)
F1(acin2, 0x37, CC)
F1(acin5, 5, CC)
F1(acin7, 7, CC)

F(us32, 5, N("u32"), N("s32"))
F(mus32, 7, N("u32"), N("s32"))
F1(high, 6, N("high"))
F(us32v, 6, N("u32"), N("s32"))
F(us32d, 0x2a, N("u32"), N("s32"))

F1(pnot1, 0x17, N("not"))
F1(pnot2, 0x1d, N("not"))
F1(pnot3, 0x34, N("not"))

F1(dtex, 0x2d, N("deriv"))
F(ltex, 9, N("all"), N("live"))

F(prefetchl, 6, N("l1"), N("l2"))

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

// TODO: this definitely needs a second pass to see which combinations really work.
static struct insn tabcvtfdst[] = {
	{ 0x0000000000100000ull, 0x0000000000700000ull, T(ias), N("f16"), DST },
	{ 0x0000000000200000ull, 0x0000000000700000ull, T(ias), N("f32"), DST },
	{ 0x0000000000300000ull, 0x0000000000700000ull, N("f64"), DSTD },
	{ 0, 0, OOPS, DST },
};

static struct insn tabcvtidst[] = {
	{ 0x0000000000000000ull, 0x0000000000700080ull, N("u8"), DST },
	{ 0x0000000000000080ull, 0x0000000000700080ull, N("s8"), DST },
	{ 0x0000000000100000ull, 0x0000000000700080ull, N("u16"), DST },
	{ 0x0000000000100080ull, 0x0000000000700080ull, N("s16"), DST },
	{ 0x0000000000200000ull, 0x0000000000700080ull, N("u32"), DST },
	{ 0x0000000000200080ull, 0x0000000000700080ull, N("s32"), DST },
	{ 0x0000000000300000ull, 0x0000000000700080ull, N("u64"), DSTD },
	{ 0x0000000000300080ull, 0x0000000000700080ull, N("s64"), DSTD },
	{ 0, 0, OOPS, DST },
};

static struct insn tabcvtfsrc[] = {
	{ 0x0000000000800000ull, 0x0000000003800000ull, T(neg2), T(abs2), N("f16"), T(cs2) },
	{ 0x0000000001000000ull, 0x0000000003800000ull, T(neg2), T(abs2), N("f32"), T(cs2) },
	{ 0x0000000001800000ull, 0x0000000003800000ull, T(neg2), T(abs2), N("f64"), SRC2D },
	{ 0, 0, OOPS, T(neg2), T(abs2), SRC2 },
};

static struct insn tabcvtisrc[] = {
	{ 0x0000000000000000ull, 0x0000000003800200ull, T(neg2), T(abs2), N("u8"), BNUM, T(cs2) },
	{ 0x0000000000000200ull, 0x0000000003800200ull, T(neg2), T(abs2), N("s8"), BNUM, T(cs2) },
	{ 0x0000000000800000ull, 0x0000000003800200ull, T(neg2), T(abs2), N("u16"), HNUM, T(cs2) },
	{ 0x0000000000800200ull, 0x0000000003800200ull, T(neg2), T(abs2), N("s16"), HNUM, T(cs2) },
	{ 0x0000000001000000ull, 0x0000000003800200ull, T(neg2), T(abs2), N("u32"), T(cs2) },
	{ 0x0000000001000200ull, 0x0000000003800200ull, T(neg2), T(abs2), N("s32"), T(cs2) },
	{ 0x0000000001800000ull, 0x0000000003800200ull, T(neg2), T(abs2), N("u64"), SRC2D },
	{ 0x0000000001800200ull, 0x0000000003800200ull, T(neg2), T(abs2), N("s64"), SRC2D },
	{ 0, 0, OOPS, T(neg2), T(abs2), SRC2 },
};

static struct insn tabmulf[] = {
	{ 0x0000000000000000ull, 0x000e000000000000ull },
	{ 0x0002000000000000ull, 0x000e000000000000ull, N("mul2") },
	{ 0x0004000000000000ull, 0x000e000000000000ull, N("mul4") },
	{ 0x0006000000000000ull, 0x000e000000000000ull, N("mul8") },
	{ 0x000a000000000000ull, 0x000e000000000000ull, N("div2") },
	{ 0x000c000000000000ull, 0x000e000000000000ull, N("div4") },
	{ 0x000e000000000000ull, 0x000e000000000000ull, N("div8") },
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
	{ 0x00, 0xe0, N("add") },
	{ 0x20, 0xe0, N("min") },
	{ 0x40, 0xe0, N("max") },
	{ 0x60, 0xe0, N("inc") },
	{ 0x80, 0xe0, N("dec") },
	{ 0xa0, 0xe0, N("and") },
	{ 0xc0, 0xe0, N("or") },
	{ 0xe0, 0xe0, N("xor") },
	{ 0, 0, OOPS },
};

static struct insn tabredops[] = {
	{ 0x00, 0xe0, N("add") },
	{ 0x20, 0xe0, N("min") },
	{ 0x40, 0xe0, N("max") },
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
	{ 0x0000000000000000ull, 0x0000000700000000ull, N("b0") },
	{ 0x0000000100000000ull, 0x0000000700000000ull, N("b1") },
	{ 0x0000000200000000ull, 0x0000000700000000ull, N("b2") },
	{ 0x0000000300000000ull, 0x0000000700000000ull, N("b3") },
	{ 0x0000000400000000ull, 0x0000000700000000ull, N("h0") },
	{ 0x0000000500000000ull, 0x0000000700000000ull, N("h1") },
	{ 0x0000000600000000ull, 0x0000000700000000ull },
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
	{ 0x0000000000000000ull, 0x0380000000000000ull, },
	{ 0x0080000000000000ull, 0x0380000000000000ull, N("shr7") },
	{ 0x0100000000000000ull, 0x0380000000000000ull, N("shr15") },
	{ 0, 0, OOPS },
};

static struct insn tabvsetop[] = {
	{ 0x080, 0x380, N("lt") },
	{ 0x100, 0x380, N("eq") },
	{ 0x180, 0x380, N("le") },
	{ 0x200, 0x380, N("gt") },
	{ 0x280, 0x380, N("ne") },
	{ 0x300, 0x380, N("ge") },
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
	{ 0x0800000000000000ull, 0xf800000000000007ull, T(minmax), T(faf), N("f32"), DST, T(acout), T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2) },
	{ 0x1000000000000000ull, 0xf000000000000007ull, N("set"), T(setftz), T(setdt), DST, T(acout), T(setit), N("f32"), T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2), T(setlop) },
	{ 0x2000000000000000ull, 0xf000000000000007ull, N("set"), T(setftz), PDST, PDSTN, T(setit), N("f32"), T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2), T(setlop) },
	{ 0x3000000000000000ull, 0xf800000000000007ull, N("add"), T(fmf), T(ias), T(farm), N("f32"), DST, T(acout), T(neg1), N("mul"), T(fmz), SRC1, T(fs2w3), T(neg2), T(is3) },
	{ 0x3800000000000000ull, 0xf800000000000007ull, N("slct"), T(faf), N("b32"), DST, SRC1, T(fs2w3), T(setit), N("f32"), T(is3) },
	// 40?
	{ 0x4800000000000000ull, 0xf800000000000007ull, N("quadop"), T(faf), T(farm), N("f32"), T(qop0), T(qop1), T(qop2), T(qop3), DST, T(acout), T(qs1), SRC1, SRC2 },
	{ 0x5000000000000000ull, 0xf800000000000007ull, N("add"), T(faf), T(fas), T(farm), N("f32"), DST, T(acout), T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2) },
	{ 0x5800000000000000ull, 0xf800000000000007ull, N("mul"), T(mulf), T(fmz), T(fmf), T(ias), T(farm), T(fmneg), N("f32"), DST, T(acout), SRC1, T(fs2) },
	{ 0x6000000000000000ull, 0xf800000000000027ull, N("presin"), N("f32"), DST, T(neg2), T(abs2), T(fs2) },
	{ 0x6000000000000020ull, 0xf800000000000027ull, N("preex2"), N("f32"), DST, T(neg2), T(abs2), T(fs2) },
	{ 0xc07e0000fc000000ull, 0xf87e0000fc0001c7ull, N("interp"), N("f32"), DST, VAR },
	{ 0xc07e000000000040ull, 0xf87e0000000001c7ull, N("interp"), N("f32"), DST, SRC2, VAR },
	{ 0xc07e0000fc000080ull, 0xf87e0000fc0001c7ull, N("interp"), N("f32"), DST, N("flat"), VAR },
	{ 0xc07e0000fc000100ull, 0xf87e0000fc0001c7ull, N("interp"), N("f32"), DST, N("cent"), VAR },
	{ 0xc07e000000000140ull, 0xf87e0000000001c7ull, N("interp"), N("f32"), DST, N("cent"), SRC2, VAR },
	{ 0xc800000000000000ull, 0xf80000001c000007ull, N("cos"), T(ias), N("f32"), DST, T(neg1), T(abs1), SRC1 },
	{ 0xc800000004000000ull, 0xf80000001c000007ull, N("sin"), T(ias), N("f32"), DST, T(neg1), T(abs1), SRC1 },
	{ 0xc800000008000000ull, 0xf80000001c000007ull, N("ex2"), T(ias), N("f32"), DST, T(neg1), T(abs1), SRC1 },
	{ 0xc80000000c000000ull, 0xf80000001c000007ull, N("lg2"), T(ias), N("f32"), DST, T(neg1), T(abs1), SRC1 },
	{ 0xc800000010000000ull, 0xf80000001c000007ull, N("rcp"), T(ias), N("f32"), DST, T(neg1), T(abs1), SRC1 },
	{ 0xc800000014000000ull, 0xf80000001c000007ull, N("rsqrt"), T(ias), N("f32"), DST, T(neg1), T(abs1), SRC1 },
	{ 0xc800000018000000ull, 0xf80000001c000007ull, N("rcp64h"), T(ias), DST, T(neg1), T(abs1), SRC1 },
	{ 0xc80000001c000000ull, 0xf80000001c000007ull, N("rsqrt64h"), T(ias), DST, T(neg1), T(abs1), SRC1 },
	{ 0x0000000000000000ull, 0x0000000000000007ull, OOPS, T(farm), N("f32"), DST, SRC1, T(fs2w3), T(is3) },


	{ 0x0800000000000001ull, 0xf800000000000007ull, T(minmax), N("f64"), DSTD, T(acout), T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2) },
	{ 0x1000000000000001ull, 0xf800000000000007ull, N("set"), T(setdt), DST, T(acout), T(setit), N("f64"), T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2), T(setlop) },
	{ 0x1800000000000001ull, 0xf800000000000007ull, N("set"), PDST, PDSTN, T(setit), N("f64"), T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2), T(setlop) },
	{ 0x2000000000000001ull, 0xf800000000000007ull, N("fma"), T(farm), N("f64"), DSTD, T(acout), T(neg1), SRC1D, T(ds2), T(neg2), SRC3D },
	{ 0x4800000000000001ull, 0xf800000000000007ull, N("add"), T(farm), N("f64"), DSTD, T(acout), T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2) },
	{ 0x5000000000000001ull, 0xf800000000000007ull, N("mul"), T(farm), T(neg1), N("f64"), DSTD, T(acout), SRC1D, T(ds2) },
	{ 0x0000000000000001ull, 0x0000000000000007ull, OOPS, T(farm), N("f64"), DSTD, SRC1D, T(ds2), SRC3D },


	{ 0x0000000000000002ull, 0xf800000000000007ull, T(addop), DST, T(acout2), N("mul"), T(high), T(mus32), SRC1, T(us32), LIMM, SRC3 },
	{ 0x0800000000000002ull, 0xf800000000000007ull, T(addop), T(ias), N("b32"), DST, T(acout2), SRC1, LIMM, T(acin) },
	{ 0x1000000000000002ull, 0xf800000000000007ull, N("mul"), T(high), DST, T(acout2), T(mus32), SRC1, T(us32), LIMM },
	{ 0x1800000000000002ull, 0xf800000000000007ull, T(lane), N("mov"), N("b32"), DST, LIMM },
	{ 0x2000000000000002ull, 0xf800000000000007ull, N("add"), T(fmf), T(ias), T(farm), N("f32"), DST, T(acout2), T(neg1), N("mul"), T(fmz), SRC1, LIMM, T(neg2), SRC3 },
	{ 0x2800000000000002ull, 0xf800000000000007ull, N("add"), T(faf), N("f32"), DST, T(acout2), T(neg1), T(abs1), SRC1, LIMM },
	{ 0x3000000000000002ull, 0xf800000000000007ull, N("mul"), T(fmz), T(fmf), T(ias), N("f32"), DST, T(acout2), SRC1, LIMM },
	{ 0x3800000000000002ull, 0xf800000000000007ull, T(logop), N("b32"), DST, T(acout2), T(not1), SRC1, T(not2), LIMM, T(acin5) },
	{ 0x4000000000000002ull, 0xf800000000000007ull, N("add"), N("b32"), DST, T(acout2), N("shl"), SRC1, SHCNT, LIMM },
	{ 0x0000000000000002ull, 0x0000000000000007ull, OOPS, N("b32"), DST, SRC1, LIMM },


	{ 0x0800000000000003ull, 0xf8000000000000c7ull, T(minmax), T(us32), DST, T(acout), SRC1, T(is2) },
	{ 0x0800000000000043ull, 0xf8000000000000c7ull, T(minmax), N("low"), T(us32), DST, T(acout), SRC1, T(is2), CC },
	{ 0x0800000000000083ull, 0xf8000000000000c7ull, T(minmax), N("med"), T(us32), DST, T(acout), SRC1, T(is2), CC },
	{ 0x08000000000000c3ull, 0xf8000000000000c7ull, T(minmax), N("high"), T(us32), DST, T(acout), SRC1, T(is2) },
	{ 0x1000000000000003ull, 0xf800000000000007ull, N("set"), T(setdti), DST, T(acout), T(setit), T(us32), SRC1, T(is2), T(acin), T(setlop) },
	{ 0x1800000000000003ull, 0xf800000000000007ull, N("set"), PDST, PDSTN, T(setit), T(us32), SRC1, T(is2), T(acin), T(setlop) },
	{ 0x2000000000000003ull, 0xf800000000000007ull, T(addop), T(sat38), DST, T(acout), N("mul"), T(high), T(mus32), SRC1, T(us32), T(is2w3), T(is3), T(acin2) },
	{ 0x2800000000000003ull, 0xf800000000000007ull, N("ins"), N("b32"), DST, T(acout), SRC1, T(is2w3), T(is3) },
	{ 0x3000000000000003ull, 0xf800000000000007ull, N("slct"), N("b32"), DST, SRC1, T(is2w3), T(setit), T(us32), T(is3) },
	{ 0x3800000000000003ull, 0xf800000000000007ull, N("sad"), T(us32), DST, T(acout), SRC1, T(is2w3), T(is3) },
	{ 0x4000000000000003ull, 0xf800000000000007ull, T(addop2), N("b32"), DST, T(acout), N("shl"), SRC1, SHCNT, T(is2) },
	{ 0x4800000000000003ull, 0xf800000000000007ull, T(addop), T(ias), N("b32"), DST, T(acout), SRC1, T(is2), T(acin) },
	{ 0x5000000000000003ull, 0xf800000000000007ull, N("mul"), T(high), DST, T(acout), T(mus32), SRC1, T(us32), T(is2) },
	{ 0x5800000000000003ull, 0xf800000000000007ull, N("shr"), T(rev), T(us32), DST, T(acout), SRC1, T(nowrap), T(is2), T(acin7) },
	{ 0x6000000000000003ull, 0xf800000000000007ull, N("shl"), N("b32"), DST, T(acout), SRC1, T(nowrap), T(is2), T(acin) },
	{ 0x6800000000000003ull, 0xf800000000000007ull, T(logop), N("b32"), DST, T(not1), SRC1, T(not2), T(is2) },
	{ 0x7000000000000003ull, 0xf800000000000007ull, N("ext"), T(rev), T(us32), DST, T(acout), SRC1, T(is2) }, // yes. this can reverse bits in a bitfield. really.
	{ 0x7800000000000003ull, 0xf800000000000007ull, N("bfind"), T(shiftamt), T(us32), DST, T(not2), T(is2) }, // index of highest bit set, counted from 0, -1 for 0 src. or highest bit different from sign for signed version. check me.
	{ 0x0000000000000003ull, 0x0000000000000007ull, OOPS, N("b32"), DST, SRC1, T(is2w3), T(is3) },


	// 08?
	{ 0x080e00001c000004ull, 0xfc0e00001c000007ull, N("mov"), DST, PSRC1 }, // likely pnot1. and likely some ops too.
	{ 0x0c0e00000001c004ull, 0xfc0e0000c001c007ull, N("and"), PDST, T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ 0x0c0e00004001c004ull, 0xfc0e0000c001c007ull, N("or"), PDST, T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ 0x0c0e00008001c004ull, 0xfc0e0000c001c007ull, N("xor"), PDST, T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ 0x1000000000000004ull, 0xfc00000000000007ull, N("cvt"), T(rint), T(fcrm), T(cvtfdst), T(cvtfsrc) },
	{ 0x1400000000000004ull, 0xfc00000000000007ull, N("cvt"), T(fcrm), T(cvtidst), T(cvtfsrc) },
	{ 0x1800000000000004ull, 0xfc00000000000007ull, N("cvt"), T(fcrm), T(cvtfdst), T(cvtisrc) },
	{ 0x1c00000000000004ull, 0xfc00000000000007ull, N("cvt"), T(ias), T(cvtidst), T(cvtisrc) },
	{ 0x2000000000000004ull, 0xfc00000000000007ull, N("selp"), N("b32"), DST, SRC1, T(is2), T(pnot3), PSRC3 },
	{ 0x2400000000000004ull, 0xfc00000000000007ull, N("prmt"), T(prmtmod), N("b32"), DST, SRC1, SRC3, T(is2) }, // NFI what this does. and sources 2 and 3 are swapped for some reason.
	{ 0x2800000000000004ull, 0xfc00000000000007ull, T(lane), N("mov"), N("b32"), DST, T(is2) },
	{ 0x2c00000000000004ull, 0xfc00000000000007ull, N("mov"), N("b32"), DST, SREG },
	{ 0x3000c3c003f00004ull, 0xfc00c3c003f00004ull, N("mov"), DST, CC },
	{ 0x3400c3c000000004ull, 0xfc00c3c000000004ull, N("mov"), CC, SRC1 },
	// 38?
	{ 0x40000000000001e4ull, 0xf8040000000001e7ull, N("nop") },
	{ 0x40040000000001e4ull, 0xf8040000000001e7ull, N("pmevent"), PM }, // ... a bitmask of triggered pmevents? with 0 ignored?
	{ 0x48000000000fc004ull, 0xf8000000000fc067ull, N("vote"), N("all"), PDST2, T(pnot1), PSRC1 },
	{ 0x48000000000fc024ull, 0xf8000000000fc067ull, N("vote"), N("any"), PDST2, T(pnot1), PSRC1 },
	{ 0x48000000000fc044ull, 0xf8000000000fc067ull, N("vote"), N("uni"), PDST2, T(pnot1), PSRC1 },
	{ 0x49c0000000000024ull, 0xf9c0000000000027ull, N("vote"), N("ballot"), DST, T(pnot1), PSRC1 }, // same insn as vote any, really... but need to check what happens for vote all and vote uni with non bit-bucked destination before unifying.
	{ 0x5000000000000004ull, 0xfc000000000000e7ull, N("bar popc"), PDST3, DST, T(bar), T(tcnt), T(pnot3), PSRC3 }, // and yes, sync is just a special case of this.
	{ 0x5000000000000024ull, 0xfc000000000000e7ull, N("bar and"), PDST3, DST, T(bar), T(tcnt), T(pnot3), PSRC3 },
	{ 0x5000000000000044ull, 0xfc000000000000e7ull, N("bar or"), PDST3, DST, T(bar), T(tcnt), T(pnot3), PSRC3 },
	{ 0x5000000000000084ull, 0xfc000000000000e7ull, N("bar arrive"), PDST3, DST, T(bar), T(tcnt), T(pnot3), PSRC3 },
	{ 0x5400000000000004ull, 0xfc00000000000007ull, N("popc"), DST, T(not1), SRC1, T(not2), T(is2) }, // XXX: popc(SRC1 & SRC2)? insane idea, but I don't have any better
	// ???
	{ 0xc000800000000004ull, 0xfc00800000000087ull, N("vadd"), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },
	{ 0xc000800000000084ull, 0xfc00800000000087ull, N("vsub"), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },
	{ 0xc800800000000004ull, 0xfc00800000000087ull, N("vmin"), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },
	{ 0xc800800000000084ull, 0xfc00800000000087ull, N("vmax"), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },
	{ 0xd000800000000004ull, 0xfc00800000000007ull, N("vabsdiff"), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },
	{ 0xd800800000000004ull, 0xfc00800000000007ull, N("vset"), T(vdst), DST, T(vsetop), T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },
	{ 0xe000800000000004ull, 0xfc00800000000007ull, N("vshr"), T(vsclamp), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), SRC2, SRC3  },
	{ 0xe800800000000004ull, 0xfc00800000000007ull, N("vshl"), T(vsclamp), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), SRC2, SRC3  },
	{ 0xf000800000000004ull, 0xfc00800000000007ull, N("vmad"), T(vmop), T(vas), T(vmshr), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },


	{ 0x1000000000000005ull, 0xf800000000000207ull, T(redop), N("u32"), T(gmem), DST },
	{ 0x1000000000000205ull, 0xf800000000000207ull, N("add"), N("u64"), T(gmem), DSTD },
	{ 0x1800000000000205ull, 0xf800000000000207ull, T(redops), N("s32"), T(gmem), DST },
	{ 0x2800000000000205ull, 0xf800000000000207ull, N("add"), N("f32"), T(gmem), DST },
	{ 0x507e000000000005ull, 0xf87e000000000307ull, N("ld"), T(redop), N("u32"), DST2, T(gamem), DST }, // yet another big ugly mess. but seems to work.
	{ 0x507e000000000205ull, 0xf87e0000000003e7ull, N("ld"), N("add"), N("u64"), DST2, T(gamem), DST },
	{ 0x507e000000000105ull, 0xf87e0000000003e7ull, N("exch"), N("b32"), DST2, T(gamem), DST },
	{ 0x507e000000000305ull, 0xf87e0000000003e7ull, N("exch"), N("b64"), DST2D, T(gamem), DSTD },
	{ 0x5000000000000125ull, 0xf8000000000003e7ull, N("cas"), N("b32"), DST2, T(gamem), DST, SRC3 },
	{ 0x5000000000000325ull, 0xf8000000000003e7ull, N("cas"), N("b64"), DST2D, T(gamem), DSTD, SRC3D },
	{ 0x587e000000000205ull, 0xf87e000000000307ull, N("ld"), T(redops), N("s32"), DST2, T(gamem), DST },
	{ 0x687e000000000205ull, 0xf87e0000000003e7ull, N("ld"), N("add"), N("f32"), DST2, T(gamem), DST },
	{ 0x8000000000000005ull, 0xf800000000000007ull, N("ld"), T(ldstt), T(ldstd), T(lcop), T(gmem) },
	{ 0x8800000000000005ull, 0xf800000000000007ull, N("ldu"), T(ldstt), T(ldstd), T(gmem) },
	{ 0x9000000000000005ull, 0xf800000000000007ull, N("st"), T(ldstt), T(scop), T(gmem), T(ldstd) },
	{ 0x9800000000000025ull, 0xfc00000000000027ull, N("prefetch"), T(prefetchl), GPMEM },
	{ 0x9c00000000000025ull, 0xfc00000000000027ull, N("prefetch"), T(prefetchl), GDPMEM },
	{ 0xb320003f00000005ull, 0xfb20003f00000007ull, N("prefetch"), DST, SRC1, SRC2 },
	{ 0xb300000000000005ull, 0xf3e0000000000007ull, N("ldu"), N("b32"), DST, T(lduld_gmem2), N("ld"), N("b32"), LDULD_DST2, T(lduld_gmem1) },
	{ 0xb320000000000005ull, 0xf3e0000000000007ull, N("ldu"), N("b64"), DSTD, T(lduld_gmem2), N("ld"), N("b32"), LDULD_DST2, T(lduld_gmem1) },
	{ 0xb360000000000005ull, 0xf3e0000000000007ull, N("ldu"), N("b32"), DST, T(lduld_gmem2), N("ld"), N("b64"), LDULD_DST2D, T(lduld_gmem1) },
	{ 0xb380000000000005ull, 0xf3e0000000000007ull, N("ldu"), N("b64"), DSTD, T(lduld_gmem2), N("ld"), N("b64"), LDULD_DST2D, T(lduld_gmem1) },
	{ 0xab00000000000005ull, 0xfbe0000000000007ull, N("ldu"), N("b32"), DST, T(lduld_gmem2), N("ld"), N("b32"), LDULD_DST2, LDULD_SHARED },
	{ 0xab20000000000005ull, 0xfbe0000000000007ull, N("ldu"), N("b64"), DSTD, T(lduld_gmem2), N("ld"), N("b32"), LDULD_DST2, LDULD_SHARED },
	{ 0xab60000000000005ull, 0xfbe0000000000007ull, N("ldu"), N("b32"), DST, T(lduld_gmem2), N("ld"), N("b64"), LDULD_DST2D, LDULD_SHARED },
	{ 0xab80000000000005ull, 0xfbe0000000000007ull, N("ldu"), N("b64"), DSTD, T(lduld_gmem2), N("ld"), N("b64"), LDULD_DST2D, LDULD_SHARED },
	{ 0xc000000000000005ull, 0xfd00000000000007ull, N("ld"), T(ldstt), T(ldstd), T(lcop), LOCAL },
	{ 0xc100000000000005ull, 0xfd00000000000007ull, N("ld"), T(ldstt), T(ldstd), SHARED },
	{ 0xc400000000000005ull, 0xfc00000000000007ull, N("ld"), N("lock"), T(ldstt), PDST4, T(ldstd), SHARED },
	{ 0xc800000000000005ull, 0xfd00000000000007ull, N("st"), T(ldstt), T(scop), LOCAL, T(ldstd) },
	{ 0xc900000000000005ull, 0xfd00000000000007ull, N("st"), T(ldstt), SHARED, T(ldstd) },
	{ 0xcc00000000000005ull, 0xfc00000000000007ull, N("st"), N("unlock"), T(ldstt), SHARED, T(ldstd) },
	{ 0xd000000000000025ull, 0xfc00000000000027ull, N("prefetch"), T(prefetchl), LPMEM },
	{ 0xd400400000000005ull, 0xfc00400000000007ull, N("suldb"), T(ldstt), T(ldstd), T(lcop), T(sclamp), SURF, SADDR },
	{ 0xd800400100000005ull, 0xfc00400100000007ull, N("suredp"), T(redop), T(sclamp), SURF, SADDR, DST },
	{ 0xdc00400000000005ull, 0xfc02400000000007ull, N("sustb"), T(ldstt), T(scop), T(sclamp), SURF, SADDR, T(ldstd) },
	{ 0xdc02400000000005ull, 0xfc02400000000007ull, N("sustp"), T(scop), T(sclamp), SURF, SADDR, DST },
	{ 0xe000000000000005ull, 0xf800000000000067ull, N("membar"), N("prep") }, // always used before all 3 other membars.
	{ 0xe000000000000025ull, 0xf800000000000067ull, N("membar"), N("gl") },
	{ 0xf000400000000085ull, 0xfc00400000000087ull, N("suleab"), PDST2, DSTD, T(ldstt), T(sclamp), SURF, SADDR },
	{ 0xe000000000000045ull, 0xf800000000000067ull, N("membar"), N("sys") },
	{ 0x0000000000000005ull, 0x0000000000000007ull, OOPS },

	{ 0x0000000000000006ull, 0xfe00000000000067ull, N("pfetch"), DST, VBASRC },
	{ 0x0600000000000006ull, 0xfe00000000000107ull, N("vfetch"), VDST, T(ldvf), ATTR }, // src2 is vertex offset
	{ 0x0600000000000106ull, 0xfe00000000000107ull, N("vfetch patch"), VDST, T(ldvf), ATTR }, // per patch input
	{ 0x0a00000003f00006ull, 0xfe7e000003f00107ull, N("export"), VAR, ESRC }, // GP
	{ 0x0a7e000003f00006ull, 0xfe7e000003f00107ull, N("export"), VAR, ESRC }, // VP
	{ 0x0a7e000003f00106ull, 0xfe7e000003f00107ull, N("export patch"), VAR, ESRC }, // per patch output
	{ 0x1400000000000006ull, 0xfc00000000000007ull, N("ld"), T(ldstt), T(ldstd), FCONST },
	{ 0x1c000000fc000026ull, 0xfe000000fc000067ull, N("emit") },
	{ 0x1c000000fc000046ull, 0xfe000000fc000067ull, N("restart") },
	{ 0x80000000fc000086ull, 0xfc000000fc000087ull, N("texauto"), T(texf), TDST, TEX, SAMP, TSRC }, // mad as a hatter.
	{ 0x90000000fc000086ull, 0xfc000000fc000087ull, N("texfetch"), T(texf), TDST, TEX, SAMP, TSRC },
	{ 0xc0000000fc000006ull, 0xfc000000fc000007ull, N("texsize"), T(texf), TDST, TEX, SAMP, TSRC },
	{ 0x0000000000000006ull, 0x0000000000000007ull, OOPS, T(texf), TDST, TEX, SAMP, TSRC }, // is assuming a tex instruction a good idea here? probably. there are loads of unknown tex insns after all.



	{ 0x0, 0x0, OOPS, DST, SRC1, T(is2), SRC3 },
};

static struct insn tabp[] = {
	{ 0x1c00, 0x3c00 },
	{ 0x3c00, 0x3c00, N("never") },	// probably.
	{ 0x0000, 0x2000, PRED },
	{ 0x2000, 0x2000, N("not"), PRED },
	{ 0, 0, OOPS },
};

F1(brawarp, 0xf, N("allwarp")) // probably jumps if the whole warp has the predicate evaluate to true.
F1(lim, 0x10, N("lim"))

static struct insn tabbtarg[] = {
	{ 0x0000000000000000ull, 0x0000000000004000ull, BTARG },
	{ 0x0000000000004000ull, 0x0000000000004000ull, N("pcrel"), CONST },
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

static struct insn tabc[] = {
	{ 0x0000000000000007ull, 0xf800000000004007ull, T(p), T(cc), N("bra"), T(lim), T(brawarp), N("abs"), ABTARG },
	{ 0x0000000000004007ull, 0xf800000000004007ull, T(p), T(cc), N("bra"), T(lim), T(brawarp), CONST },
	{ 0x0800000000000007ull, 0xf800000000004007ull, T(p), T(cc), N("bra"), T(lim), SRC1, N("abs"), ANTARG },
	{ 0x0800000000004007ull, 0xf800000000004007ull, T(p), T(cc), N("bra"), T(lim), SRC1, CONST },
	{ 0x1000000000000007ull, 0xf800000000004007ull, N("call"), T(lim), N("abs"), ACTARG },
	{ 0x1000000000004007ull, 0xf800000000004007ull, N("call"), T(lim), CONST },
	{ 0x4000000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("bra"), T(lim), T(brawarp), T(btarg) },
	{ 0x4800000000000007ull, 0xf800000000004007ull, T(p), T(cc), N("bra"), T(lim), SRC1, NTARG },
	{ 0x4800000000004007ull, 0xf800000000004007ull, T(p), T(cc), N("bra"), T(lim), SRC1, N("pcrel"), CONST },
	{ 0x5000000000000007ull, 0xf800000000004007ull, N("call"), T(lim), CTARG },
	{ 0x5000000000004007ull, 0xf800000000004007ull, N("call"), T(lim), N("pcrel"), CONST },
	{ 0x5800000000000007ull, 0xf800000000000007ull, N("prelongjmp"), T(btarg) },
	{ 0x6000000000000007ull, 0xf800000000000007ull, N("joinat"), T(btarg) },
	{ 0x6800000000000007ull, 0xf800000000000007ull, N("prebrk"), T(btarg) },
	{ 0x7000000000000007ull, 0xf800000000000007ull, N("precont"), T(btarg) },
	{ 0x7800000000000007ull, 0xf800000000000007ull, N("preret"), T(btarg) },
	{ 0x8000000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("exit") },
	{ 0x8800000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("longjmp") },
	{ 0x9000000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("ret") },
	{ 0x9800000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("discard") },
	{ 0xa800000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("brk") },
	{ 0xb000000000000007ull, 0xf800000000000007ull, T(p), T(cc), N("cont") },
	{ 0xc000000000000007ull, 0xf800000000000007ull, N("quadon") },
	{ 0xc800000000000007ull, 0xf800000000000007ull, N("quadpop") },
	{ 0xd000000000000007ull, 0xf80000000000c007ull, N("membar"), N("cta") },
	{ 0xd00000000000c007ull, 0xf80000000000c007ull, N("trap") },
	{ 0x0000000000000007ull, 0x0000000000000007ull, T(p), OOPS, BTARG },
	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 7, 7, OP64, T(c) }, // control instructions, special-cased.
	{ 0x0, 0x10, OP64, T(p), T(m) },
	{ 0x10, 0x10, OP64, N("join"), T(p), T(m), },
	{ 0, 0, OOPS },
};

static struct disisa nvc0_isa_s = {
	tabroot,
	8,
	8,
	1,
};

struct disisa *nvc0_isa = &nvc0_isa_s;
