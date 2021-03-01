/*
 * Copyright 2014 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs <bskeggs@redhat.com>
 */

#include "dis-intern.h"

#define F_SM50 1
#define F_SM52 2
#define F_SM60 4

#define ZN(b,n) atomtab_a, atomtab_d, (struct insn[]) { \
	{ 0ull << (b), 1ull << (b), N(#n) }, \
	{ 1ull << (b), 1ull << (b) }, \
	{ 0, 0, OOPS } \
}

#define ON(b,n) atomtab_a, atomtab_d, (struct insn[]) { \
	{ 0ull << (b), 1ull << (b) }, \
	{ 1ull << (b), 1ull << (b), N(#n) }, \
	{ 0, 0, OOPS } \
}

#define ZNV(b,n,v) atomtab_a, atomtab_d, (struct insn[]) { \
	{ 0ull << (b), 1ull << (b), N(#n), .fmask = (v) }, \
	{ 1ull << (b), 1ull << (b) }, \
	{ 0, 0, OOPS }, \
}

#define ONV(b,n,v) atomtab_a, atomtab_d, (struct insn[]) { \
	{ 0ull << (b), 1ull << (b) }, \
	{ 1ull << (b), 1ull << (b), N(#n), .fmask = (v) }, \
	{ 0, 0, OOPS }, \
}

/* sched control codes
 * (source: https://github.com/NervanaSystems/maxas/wiki/Control-Codes) */
#define ST_POS(n) (n * 21 + 0)  /* stall counts */
#define YL_POS(n) (n * 21 + 4)  /* yield hint flag */
#define WR_POS(n) (n * 21 + 5)  /* write dep bar */
#define RD_POS(n) (n * 21 + 8)  /* read dep bar */
#define WT_POS(n) (n * 21 + 11) /* wait dep bar */
#define RU_POS(n) (n * 21 + 17) /* reuse flag */

#define ST(n) static struct insn tabst##n[] = { \
	{ 0x0000000000000000ull, 0x0000000000000000ull, N("st"), \
		atomrimm, (struct bitfield[]) { { ST_POS(n), 4 } } }, \
	{ 0, 0, OOPS }, \
};

#define YL(n) static struct insn tabyl##n[] = { \
	{ 0x0000000000000000ull, 1ULL << YL_POS(n) }, \
	{ 1ULL << YL_POS(n)    , 1ULL << YL_POS(n), N("yl") }, \
	{ 0, 0, OOPS }, \
};

#define WR(n) static struct insn tabwr##n[] = { \
	{ 7ULL << WR_POS(n)    , 7ULL << WR_POS(n) }, \
	{ 0x0000000000000000ull, 0x0000000000000000ull, N("wr"), \
		atomrimm, (struct bitfield[]) { { WR_POS(n), 3 } } }, \
	{ 0, 0, OOPS }, \
};

#define RD(n) static struct insn tabrd##n[] = { \
	{ 7ULL << RD_POS(n)    , 7ULL << RD_POS(n) }, \
	{ 0x0000000000000000ull, 0x0000000000000000ull, N("rd"), \
		atomrimm, (struct bitfield[]) { { RD_POS(n), 3 } } }, \
	{ 0, 0, OOPS }, \
};

#define WT(n) static struct insn tabwt##n[] = { \
	{ 0x0000000000000000ull, 63ULL << WT_POS(n) }, \
	{ 0x0000000000000000ull, 0x0000000000000000ull, N("wt"), \
		atomrimm, (struct bitfield[]) { { WT_POS(n), 6 } } }, \
	{ 0, 0, OOPS }, \
};

#define RU(n) static struct insn tabru##n[] = { \
	{ 0x0000000000000000ull, 15ULL << RU_POS(n) }, \
	{ 0x0000000000000000ull, 0x0000000000000000ull, N("ru"), \
		atomrimm, (struct bitfield[]) { { RU_POS(n), 4 } } }, \
	{ 0, 0, OOPS }, \
};

#define SCHED(n) \
ST(n); YL(n); WR(n); RD(n); WT(n); RU(n); \
static struct insn tabsched##n[] = { \
    0, 0, SESTART, T(st##n), T(yl##n), T(wr##n), T(rd##n), T(wt##n), T(ru##n), SEEND \
};

SCHED(0);
SCHED(1);
SCHED(2);

/* register fields */
static struct sreg reg_sr[] = {
	{ 255, 0, SR_ZERO },
	{ -1 }
};

static struct sreg sys_sr[] = {
	{ 0x00, "laneid" },
	{ 0x01, "clock" },
	{ 0x02, "virtcfg" }, // bits 8-14: nwarpid, bits 20-28: nsmid
	{ 0x03, "virtid" }, // bits 8-13: warpid, bits 20-28: smid
	{ 0x04, "pm0" },
	{ 0x05, "pm1" },
	{ 0x06, "pm2" },
	{ 0x07, "pm3" },
	{ 0x08, "pm4" },
	{ 0x09, "pm5" },
	{ 0x0a, "pm6" },
	{ 0x0b, "pm7" },
	{ 0x0f, "ordering_ticket", .fmask = F_SM60 },
	{ 0x10, "prim_type" },
	{ 0x11, "invocation_id" },
	{ 0x12, "y_direction" },
	{ 0x13, "thread_kill" },
	{ 0x14, "shader_type" },
	{ 0x15, "directbewriteaddresslow" },
	{ 0x16, "directbewriteaddresshigh" },
	{ 0x17, "directbewriteenabled" },
	{ 0x18, "machine_id_0" },
	{ 0x19, "machine_id_1" },
	{ 0x1a, "machine_id_2" },
	{ 0x1b, "machine_id_3" },
	{ 0x1c, "affinity" },
	{ 0x1d, "invocation_info" },
	{ 0x1e, "wscalefactor_xy" },
	{ 0x1f, "wscalefactor_z" },
	{ 0x20, "tid" },
	{ 0x21, "tidx" },
	{ 0x22, "tidy" },
	{ 0x23, "tidz" },
	{ 0x24, "cta_param" },
	{ 0x25, "ctaidx" },
	{ 0x26, "ctaidy" },
	{ 0x27, "ctaidz" },
	{ 0x28, "ntid" },
	{ 0x29, "cirqueueincrminusone" },
	{ 0x2a, "nlatc" },
	{ 0x2c, "sm_spa_version", .fmask = F_SM60 },
	{ 0x2d, "multipassshaderinfo", .fmask = F_SM60 },
	{ 0x2e, "lwinhi", .fmask = F_SM60 },
	{ 0x2f, "swinhi", .fmask = F_SM60 },
	{ 0x30, "swinlo" },
	{ 0x31, "swinsz" },
	{ 0x32, "smemsz" },
	{ 0x33, "smembanks" },
	{ 0x34, "lwinlo" },
	{ 0x35, "lwinsz" },
	{ 0x36, "lmemlosz" },
	{ 0x37, "lmemhioff" },
	{ 0x38, "eqmask" },
	{ 0x39, "ltmask" },
	{ 0x3a, "lemask" },
	{ 0x3b, "gtmask" },
	{ 0x3c, "gemask" },
	{ 0x3d, "regalloc" },
	{ 0x3e, "ctxaddr", .fmask = F_SM50 },
	{ 0x3e, "barrieralloc", .fmask = F_SM60 },
	{ 0x40, "globalerrorstatus" },
	{ 0x42, "warperrorstatus" },
	{ 0x43, "warperrorstatusclear" },
	{ 0x48, "pm_hi0" },
	{ 0x49, "pm_hi1" },
	{ 0x4a, "pm_hi2" },
	{ 0x4b, "pm_hi3" },
	{ 0x4c, "pm_hi4" },
	{ 0x4d, "pm_hi5" },
	{ 0x4e, "pm_hi6" },
	{ 0x4f, "pm_hi7" },
	{ 0x50, "clocklo" },
	{ 0x51, "clockhi" },
	{ 0x52, "globaltimerlo" },
	{ 0x53, "globaltimerhi" },
	{ 0x60, "hwtaskid" },
	{ 0x61, "circularqueueentryindex" },
	{ 0x62, "circularqueueentryaddresslow" },
	{ 0x63, "circularqueueentryaddresshigh" },
	{ -1 }
};

static struct bitfield reg00_bf = { 0, 8 };
static struct bitfield reg08_bf = { 8, 8 };
static struct bitfield reg20_bf = { 20, 8 };
static struct bitfield reg28_bf = { 28, 8 };
static struct bitfield reg39_bf = { 39, 8 };

static struct reg reg00_r = { &reg00_bf, "r", .specials = reg_sr };
static struct reg reg08_r = { &reg08_bf, "r", .specials = reg_sr };
static struct reg reg20_r = { &reg20_bf, "r", .specials = reg_sr };
static struct reg reg28_r = { &reg28_bf, "r", .specials = reg_sr };
static struct reg reg39_r = { &reg39_bf, "r", .specials = reg_sr };
static struct reg sys20_r = { &reg20_bf, "s", .specials = sys_sr };

#define REG_00 atomreg, &reg00_r
#define REG_08 atomreg, &reg08_r
#define REG_20 atomreg, &reg20_r
#define REG_28 atomreg, &reg28_r
#define REG_39 atomreg, &reg39_r
#define SYS_20 atomreg, &sys20_r

/* immediate fields */
static struct rbitfield u0326_bf = { { 26, 3 }, RBF_UNSIGNED };
static struct rbitfield u0412_bf = { { 12, 4 }, RBF_UNSIGNED };
static struct rbitfield u0428_bf = { { 28, 4 }, RBF_UNSIGNED };
static struct rbitfield u0431_bf = { { 31, 4 }, RBF_UNSIGNED };
static struct rbitfield u0439_bf = { { 39, 4 }, RBF_UNSIGNED };
static struct rbitfield u0520_bf = { { 20, 5 }, RBF_UNSIGNED };
static struct rbitfield u0528_bf = { { 28, 5 }, RBF_UNSIGNED };
static struct rbitfield u0539_bf = { { 39, 5 }, RBF_UNSIGNED };
static struct rbitfield u0551_bf = { { 51, 5 }, RBF_UNSIGNED };
static struct rbitfield u0553_bf = { { 53, 5 }, RBF_UNSIGNED };
static struct rbitfield u0620_bf = { { 20, 6 }, RBF_UNSIGNED };
static struct rbitfield u0600_bf = { { 0, 6 }, RBF_UNSIGNED };
static struct rbitfield u0808_bf = { { 8, 8 }, RBF_UNSIGNED };
static struct rbitfield u0820_bf = { { 20, 8 }, RBF_UNSIGNED };
static struct rbitfield u0828_bf = { { 28, 8 }, RBF_UNSIGNED };
static struct rbitfield u0848_bf = { { 48, 8 }, RBF_UNSIGNED };
static struct rbitfield u0920_bf = { { 20, 9 }, RBF_UNSIGNED, .shr = 6 };
static struct rbitfield u0930_bf = { { 30, 9 }, RBF_UNSIGNED, .shr = 6 };
static struct rbitfield u1220_bf = { { 20, 12 }, RBF_UNSIGNED };
static struct rbitfield u1334_bf = { { 34, 13 }, RBF_UNSIGNED };
static struct rbitfield u1336_bf = { { 36, 13 }, RBF_UNSIGNED };
static struct rbitfield u1620_bf = { { 20, 16 }, RBF_UNSIGNED };
static struct rbitfield u1636_bf = { { 36, 16 }, RBF_UNSIGNED };
static struct rbitfield u2020_bf = { { 20, 20 }, RBF_UNSIGNED };
static struct rbitfield u2420_bf = { { 20, 24 }, RBF_UNSIGNED };
static struct rbitfield u3220_bf = { { 20, 32 }, RBF_UNSIGNED };
static struct rbitfield u3613_bf = { { 36, 13 }, RBF_UNSIGNED };
static struct rbitfield s1120_bf = { { 20, 11 }, RBF_SIGNED };
static struct rbitfield s1620_bf = { { 20, 16 }, RBF_SIGNED };
static struct rbitfield s2020_bf = { { 20, 19, 56, 1 }, RBF_SIGNED };
static struct rbitfield s2420_bf = { { 20, 24 }, RBF_SIGNED };
static struct rbitfield s3220_bf = { { 20, 32 }, RBF_SIGNED };
static struct rbitfield f1920_bf = { { 20, 19, 56, 1 }, RBF_UNSIGNED, .shr = 12 };
static struct rbitfield d1920_bf = { { 20, 19, 56, 1 }, RBF_UNSIGNED, .shr = 44 };
static struct rbitfield o1420_bf = { { 20, 14 }, RBF_SIGNED, .shr = 2 };

#define U03_26 atomrimm, &u0326_bf
#define U04_12 atomrimm, &u0412_bf
#define U04_28 atomrimm, &u0428_bf
#define U04_31 atomrimm, &u0431_bf
#define U04_39 atomrimm, &u0439_bf
#define U05_20 atomrimm, &u0520_bf
#define U05_28 atomrimm, &u0528_bf
#define U05_39 atomrimm, &u0539_bf
#define U05_51 atomrimm, &u0551_bf
#define U05_53 atomrimm, &u0553_bf
#define U06_20 atomrimm, &u0620_bf
#define U06_00 atomrimm, &u0600_bf
#define U08_08 atomrimm, &u0808_bf
#define U08_20 atomrimm, &u0820_bf
#define U08_28 atomrimm, &u0828_bf
#define U08_48 atomrimm, &u0848_bf
#define U09_20 atomrimm, &u0920_bf
#define U09_30 atomrimm, &u0930_bf
#define U12_20 atomrimm, &u1220_bf
#define U13_34 atomrimm, &u1334_bf
#define U13_36 atomrimm, &u1336_bf
#define U16_20 atomrimm, &u1620_bf
#define U16_36 atomrimm, &u1636_bf
#define U20_20 atomrimm, &u2020_bf
#define U24_20 atomrimm, &u2420_bf
#define U32_20 atomrimm, &u3220_bf
#define U36_13 atomrimm, &u3613_bf
#define S11_20 atomrimm, &s1120_bf
#define S16_20 atomrimm, &u1620_bf
#define S20_20 atomrimm, &s2020_bf
#define S24_20 atomrimm, &s2420_bf
#define S32_20 atomrimm, &s3220_bf
#define F20_20 atomrimm, &f1920_bf
#define F32_20 atomrimm, &u3220_bf
#define D20_20 atomrimm, &d1920_bf

/* branch targets */
static struct rbitfield btarg_bf = { { 20, 24 }, RBF_SIGNED, .pcrel = 1, .addend = 8 };

#define BTARG atomctarg, &btarg_bf

/* constbuf selection fields */
static struct  bitfield cmem34_idx = { 34, 5 };
static struct  bitfield cmem36_idx = { 36, 5 };

static struct mem cmem34rzo1420_m = { "c", &cmem34_idx, 0, &o1420_bf };
static struct mem cmem36rzs1620_m = { "c", &cmem36_idx, 0, &s1620_bf };
static struct mem cmem3608s1620_m = { "c", &cmem36_idx, &reg08_r, &s1620_bf };

#define C34_RZ_O14_20 atommem, &cmem34rzo1420_m
#define C36_RZ_S16_20 atommem, &cmem36rzs1620_m
#define C36_08_S16_20 atommem, &cmem3608s1620_m

/* memory fields */
static struct rbitfield amem20_imm = { { 20, 10 }, RBF_UNSIGNED };
static struct rbitfield amem28_imm = { { 28, 10 }, RBF_UNSIGNED };
static struct rbitfield smem_imm = { { 20, 24 }, RBF_SIGNED };
static struct rbitfield smem20_imm = { { 20, 20 }, RBF_SIGNED };
static struct rbitfield pldmem_imm = { { 20, 8 }, RBF_SIGNED };
static struct rbitfield cctlmem_imm = { { 22, 30 }, RBF_SIGNED, .shr = 2 };
static struct rbitfield cctllmem_imm = { { 22, 22 }, RBF_SIGNED, .shr = 2 };
static struct rbitfield atommem0_imm = { { 28, 20 }, RBF_SIGNED };
static struct rbitfield atommem1_imm = { { 30, 22 }, RBF_SIGNED, .shr = 2 };
static struct rbitfield gmem_imm = { { 20, 32 }, RBF_SIGNED };

static struct mem amem_m = { "a", 0, &reg08_r, &amem20_imm };
static struct mem amem28_m = { "a", 0, &reg08_r, &amem28_imm };
static struct mem amemidx_m = { "a", 0, &reg08_r, 0 };
static struct mem lmem_m = { "l", 0, &reg08_r, &smem_imm };
static struct mem smem_m = { "s", 0, &reg08_r, &smem_imm };
static struct mem gmem_m = { "g", 0, &reg08_r, &gmem_imm };
static struct mem ncgmem_m = { "ncg", 0, &reg08_r, &smem_imm };
static struct mem ncgmem20_m = { "ncg", 0, &reg08_r, &smem20_imm };

static struct mem cctlmem_m = { "g", 0, &reg08_r, &cctlmem_imm };
static struct mem cctllmem_m = { "l", 0, &reg08_r, &cctllmem_imm };

static struct mem pldmem_m = { "pix", 0, &reg08_r, &pldmem_imm };
static struct mem isberdmem_m = { "p", 0, &reg08_r };
static struct mem atommem0_m = { "g", 0, &reg08_r, &atommem0_imm };
static struct mem atommem1_m = { "s", 0, &reg08_r, &atommem1_imm };
static struct mem redmem0_m = { "g", 0, &reg08_r, &atommem0_imm };
static struct mem suredmem_m = { "g", 0, &reg08_r, 0 };

#define AMEM atommem, &amem_m
#define AMEM28 atommem, &amem28_m
#define AMEMIDX atommem, &amemidx_m
#define LMEM atommem, &lmem_m
#define SMEM atommem, &smem_m
#define GMEM atommem, &gmem_m
#define NCGMEM atommem, &ncgmem_m
#define NCGMEM_20 atommem, &ncgmem20_m

#define PLDMEM atommem, &pldmem_m
#define ISBERDMEM atommem, &isberdmem_m
#define CCTLMEM atommem, &cctlmem_m
#define CCTLLMEM atommem, &cctllmem_m
#define ATOMMEM0 atommem, &atommem0_m
#define ATOMMEM1 atommem, &atommem1_m
#define REDMEM0 atommem, &redmem0_m
#define SUREDMEM atommem, &suredmem_m

/* predicate fields */
static struct sreg pred_sr[] = {
	{ 7, 0, SR_ONE },
	{ -1 }
};

static struct bitfield pred00_bf = { 0, 3 };
static struct bitfield pred03_bf = { 3, 3 };
static struct bitfield pred12_bf = { 12, 3 };
static struct bitfield pred16_bf = { 16, 3 };
static struct bitfield pred29_bf = { 29, 3 };
static struct bitfield pred39_bf = { 39, 3 };
static struct bitfield pred44_bf = { 44, 3 };
static struct bitfield pred45_bf = { 45, 3 };
static struct bitfield pred47_bf = { 47, 3 };
static struct bitfield pred48_bf = { 48, 3 };
static struct bitfield pred58_bf = { 58, 3 };

static struct reg pred00_r = { &pred00_bf, "p", .specials = pred_sr };
static struct reg pred03_r = { &pred03_bf, "p", .specials = pred_sr };
static struct reg pred12_r = { &pred12_bf, "p", .specials = pred_sr };
static struct reg pred16_r = { &pred16_bf, "p", .specials = pred_sr };
static struct reg pred29_r = { &pred29_bf, "p", .specials = pred_sr };
static struct reg pred39_r = { &pred39_bf, "p", .specials = pred_sr };
static struct reg pred44_r = { &pred44_bf, "p", .specials = pred_sr };
static struct reg pred45_r = { &pred45_bf, "p", .specials = pred_sr };
static struct reg pred47_r = { &pred47_bf, "p", .specials = pred_sr };
static struct reg pred48_r = { &pred48_bf, "p", .specials = pred_sr };
static struct reg pred58_r = { &pred58_bf, "p", .specials = pred_sr };

#define PRED00 atomreg, &pred00_r
#define PRED03 atomreg, &pred03_r
#define PRED12 atomreg, &pred12_r
#define PRED16 atomreg, &pred16_r
#define PRED29 atomreg, &pred29_r
#define PRED39 atomreg, &pred39_r
#define PRED44 atomreg, &pred44_r
#define PRED45 atomreg, &pred45_r
#define PRED47 atomreg, &pred47_r
#define PRED48 atomreg, &pred48_r
#define PRED58 atomreg, &pred58_r

static struct insn tabpred12[] = { { 0, 0, ON(15, not), PRED12 }, };
static struct insn tabpred29[] = { { 0, 0, ON(32, not), PRED29 }, };
static struct insn tabpred39[] = { { 0, 0, ON(42, not), PRED39 }, };
static struct insn tabpred47[] = { { 0, 0, ON(50, not), PRED47 }, };
static struct insn tabpred[] = {
	{ 0x0000000000070000ull, 0x00000000000f0000ull },
	{ 0x00000000000f0000ull, 0x00000000000f0000ull, N("never") },
	{ 0, 0, ON(19, not), PRED16 }
};

static struct insn tabfbe0_0[] = {
	{ 0x0000008000000000ull, 0x0000018000000000ull, N("emit") },
	{ 0x0000010000000000ull, 0x0000018000000000ull, N("cut") },
	{ 0x0000018000000000ull, 0x0000018000000000ull, N("emit_then_cut") },
	{ 0, 0, OOPS },
};

static struct bitfield inv_pred41_bf = { 41, 3, .xorend = 7 };
static struct reg inv_pred41_r = { &inv_pred41_bf, "p", .specials = pred_sr };

#define INV_PRED41 atomreg, &inv_pred41_r

static struct insn tabf0f8_0[] = {
	{ 0x0000000000000000ull, 0x000000000000001full, N("f") },
	{ 0x0000000000000001ull, 0x000000000000001full, N("lt") },
	{ 0x0000000000000002ull, 0x000000000000001full, N("eq") },
	{ 0x0000000000000003ull, 0x000000000000001full, N("le") },
	{ 0x0000000000000004ull, 0x000000000000001full, N("gt") },
	{ 0x0000000000000005ull, 0x000000000000001full, N("ne") },
	{ 0x0000000000000006ull, 0x000000000000001full, N("ge") },
	{ 0x0000000000000007ull, 0x000000000000001full, N("num") },
	{ 0x0000000000000008ull, 0x000000000000001full, N("nan") },
	{ 0x0000000000000009ull, 0x000000000000001full, N("ltu") },
	{ 0x000000000000000aull, 0x000000000000001full, N("equ") },
	{ 0x000000000000000bull, 0x000000000000001full, N("leu") },
	{ 0x000000000000000cull, 0x000000000000001full, N("gtu") },
	{ 0x000000000000000dull, 0x000000000000001full, N("neu") },
	{ 0x000000000000000eull, 0x000000000000001full, N("geu") },
	{ 0x000000000000000full, 0x000000000000001full },
	{ 0x0000000000000010ull, 0x000000000000001full, N("off") },
	{ 0x0000000000000011ull, 0x000000000000001full, N("lo") },
	{ 0x0000000000000012ull, 0x000000000000001full, N("sff") },
	{ 0x0000000000000013ull, 0x000000000000001full, N("ls") },
	{ 0x0000000000000014ull, 0x000000000000001full, N("hi") },
	{ 0x0000000000000015ull, 0x000000000000001full, N("sft") },
	{ 0x0000000000000016ull, 0x000000000000001full, N("hs") },
	{ 0x0000000000000017ull, 0x000000000000001full, N("oft") },
	{ 0x0000000000000018ull, 0x000000000000001full, N("csm_ta") },
	{ 0x0000000000000019ull, 0x000000000000001full, N("csm_tr") },
	{ 0x000000000000001aull, 0x000000000000001full, N("csm_mx") },
	{ 0x000000000000001bull, 0x000000000000001full, N("fcsm_ta") },
	{ 0x000000000000001cull, 0x000000000000001full, N("fcsm_tr") },
	{ 0x000000000000001dull, 0x000000000000001full, N("fcsm_mx") },
	{ 0x000000000000001eull, 0x000000000000001full, N("rle") },
	{ 0x000000000000001full, 0x000000000000001full, N("rgt") },
	{ 0, 0, OOPS },
};

static struct insn tabf0f0_0[] = {
	{ 0x0000000000000000ull, 0x0000000020000000ull },
	{ 0x0000000020000000ull, 0x0000000020000000ull, N("le") },
	{ 0, 0, OOPS },
};

static struct insn tabf0c0_0[] = {
	{ 0x0000000000000000ull, 0x0000000300000000ull },
	{ 0x0000000100000000ull, 0x0000000300000000ull, N("result") },
	{ 0x0000000200000000ull, 0x0000000300000000ull, N("warp") },
	{ 0, 0, OOPS },
};

static struct insn tabeff0_0[] = {
	{ 0x0000000000000000ull, 0x0001800000000000ull, N("b32") },
	{ 0x0000800000000000ull, 0x0001800000000000ull, N("b64") },
	{ 0x0001000000000000ull, 0x0001800000000000ull, N("b96") },
	{ 0x0001800000000000ull, 0x0001800000000000ull, N("b128") },
	{ 0, 0, OOPS },
};

static struct insn tabefe8_0[] = {
	{ 0x0000000000000000ull, 0x0000000380000000ull },
	{ 0x0000000080000000ull, 0x0000000380000000ull, N("covmask") },
	{ 0x0000000100000000ull, 0x0000000380000000ull, N("covered") },
	{ 0x0000000180000000ull, 0x0000000380000000ull, N("offset") },
	{ 0x0000000200000000ull, 0x0000000380000000ull, N("centroid_offset") },
	{ 0x0000000280000000ull, 0x0000000380000000ull, N("my_index") },
	{ 0, 0, OOPS },
};

static struct insn tabefd0_0[] = {
	{ 0x0000000000000000ull, 0x0000000600000000ull },
	{ 0x0000000200000000ull, 0x0000000600000000ull, N("patch") },
	{ 0x0000000400000000ull, 0x0000000600000000ull, N("prim") },
	{ 0x0000000600000000ull, 0x0000000600000000ull, N("attr") },
	{ 0, 0, OOPS },
};

static struct insn tabefd0sz[] = {
	{ 0x0000000000000000ull, 0x0001800000000000ull },
	{ 0x0000800000000000ull, 0x0001800000000000ull, N("u16") },
	{ 0x0001000000000000ull, 0x0001800000000000ull, N("b32") },
	{ 0, 0, OOPS },
};

static struct insn tabef98_0[] = {
	{ 0x0000000000000000ull, 0x0000000000000300ull, N("cta") },
	{ 0x0000000000000100ull, 0x0000000000000300ull, N("gl") },
	{ 0x0000000000000200ull, 0x0000000000000300ull, N("sys") },
	{ 0x0000000000000300ull, 0x0000000000000300ull, N("vc") },
	{ 0, 0, OOPS },
};

static struct insn tabef98_1[] = {
	{ 0x0000000000000000ull, 0x0000000000000003ull },
	{ 0x0000000000000001ull, 0x0000000000000003ull, N("ivalld") },
	{ 0x0000000000000002ull, 0x0000000000000003ull, N("ivallt") },
	{ 0x0000000000000003ull, 0x0000000000000003ull, N("ivalltd") },
	{ 0, 0, OOPS },
};

static struct insn tabef90_0[] = {
	{ 0x0000000000000000ull, 0x0000300000000000ull },
	{ 0x0000100000000000ull, 0x0000300000000000ull, N("il") },
	{ 0x0000200000000000ull, 0x0000300000000000ull, N("is") },
	{ 0x0000300000000000ull, 0x0000300000000000ull, N("isl") },
	{ 0, 0, OOPS },
};

static struct insn tabef90sz[] = {
	{ 0x0000000000000000ull, 0x0007000000000000ull, N("u8") },
	{ 0x0001000000000000ull, 0x0007000000000000ull, N("s8") },
	{ 0x0002000000000000ull, 0x0007000000000000ull, N("u16") },
	{ 0x0003000000000000ull, 0x0007000000000000ull, N("s16") },
	{ 0x0004000000000000ull, 0x0007000000000000ull, N("b32") },
	{ 0x0005000000000000ull, 0x0007000000000000ull, N("b64") },
	{ 0, 0, OOPS },
};

static struct insn tabef80_0[] = {
	{ 0x0000000000000001ull, 0x000000000000000full, N("pf1") },
	{ 0x0000000000000002ull, 0x000000000000000full, N("pf1.5") },
	{ 0x0000000000000003ull, 0x000000000000000full, N("pf2") },
	{ 0x0000000000000004ull, 0x000000000000000full, N("wb") },
	{ 0x0000000000000005ull, 0x000000000000000full, N("iv") },
	{ 0x000000000000ff06ull, 0x00000fffffc0ff0full, N("ivall") },
	{ 0x0000000000000007ull, 0x000000000000000full, N("rs") },
	{ 0x0000000000000008ull, 0x000000000000000full, N("what?") }, /*XXX*/
	{ 0x0000000000000009ull, 0x000000000000000full, N("rslb") },
	{ 0, 0, OOPS },
};

static struct insn tabef80ct[] = {
	{ 0x0000000000000000ull, 0x0000000000000070ull },
	{ 0, 0, OOPS },
};

static struct insn tabef60_0[] = {
	{ 0x0000000000000000ull, 0x000000000000000full, N("qry1") },
	{ 0x0000000000000001ull, 0x000000000000000full, N("pf1") },
	{ 0x0000000000000002ull, 0x000000000000000full, N("pf1.5") },
	{ 0x0000000000000003ull, 0x000000000000000full, N("pf2") },
	{ 0x0000000000000004ull, 0x000000000000000full, N("wb") },
	{ 0x0000000000000005ull, 0x000000000000000full, N("iv") },
	{ 0x000000000000ff06ull, 0x00000fffffc0ff0full, N("ivall") },
	{ 0x0000000000000007ull, 0x000000000000000full, N("rs") },
	{ 0x0000000000000008ull, 0x000000000000000full, N("what?") }, /*XXX*/
	{ 0x0000000000000009ull, 0x000000000000000full, N("rslb") },
	{ 0, 0, OOPS },
};

static struct insn tabef60ct[] = {
	{ 0x0000000000000000ull, 0x0000000000000070ull },
	{ 0x0000000000000010ull, 0x0000000000000070ull, N("u") },
	{ 0x0000000000000020ull, 0x0000000000000070ull, N("c") },
	{ 0x0000000000000030ull, 0x0000000000000070ull, N("i") },
	{ 0x0000000000000040ull, 0x0000000000000070ull, N("crs") },
	{ 0, 0, OOPS },
};

static struct insn tabef58sz[] = {
	{ 0x0000000000000000ull, 0x0007000000000000ull, N("u8") },
	{ 0x0001000000000000ull, 0x0007000000000000ull, N("s8") },
	{ 0x0002000000000000ull, 0x0007000000000000ull, N("u16") },
	{ 0x0003000000000000ull, 0x0007000000000000ull, N("s16") },
	{ 0x0004000000000000ull, 0x0007000000000000ull, N("b32") },
	{ 0x0005000000000000ull, 0x0007000000000000ull, N("b64") },
	{ 0x0006000000000000ull, 0x0007000000000000ull, N("b128") },
	{ 0, 0, OOPS },
};

static struct insn tabef50_0[] = {
	{ 0x0000000000000000ull, 0x0000300000000000ull },
	{ 0x0000100000000000ull, 0x0000300000000000ull, N("cg") },
	{ 0x0000200000000000ull, 0x0000300000000000ull, N("cs") },
	{ 0x0000300000000000ull, 0x0000300000000000ull, N("wt") },
	{ 0, 0, OOPS },
};

static struct insn tabef40_0[] = {
	{ 0x0000000000000000ull, 0x0000300000000000ull },
	{ 0x0000100000000000ull, 0x0000300000000000ull, N("lu") },
	{ 0x0000200000000000ull, 0x0000300000000000ull, N("ci") },
	{ 0x0000300000000000ull, 0x0000300000000000ull, N("cv") },
	{ 0, 0, OOPS },
};

static struct insn tabef10_0[] = {
	{ 0x0000000000000000ull, 0x00000000c0000000ull, N("idx") },
	{ 0x0000000040000000ull, 0x00000000c0000000ull, N("up") },
	{ 0x0000000080000000ull, 0x00000000c0000000ull, N("down") },
	{ 0x00000000c0000000ull, 0x00000000c0000000ull, N("bfly") },
	{ 0, 0, OOPS },
};

static struct insn tabef10_1[] = {
	{ 0x00000000000000000ull, 0x0000000030000000ull, REG_20, REG_39 },
	{ 0x00000000010000000ull, 0x0000000030000000ull, U05_20, REG_39 },
	{ 0x00000000020000000ull, 0x0000000030000000ull, REG_20, U13_34 },
	{ 0x00000000030000000ull, 0x0000000030000000ull, U05_20, U13_34 },
	{ 0, 0, OOPS },
};

static struct insn tabeed8_0[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, N("cg") },
	{ 0x0000800000000000ull, 0x0000c00000000000ull, N("cs") },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, N("wt") },
	{ 0, 0, OOPS },
};

static struct insn tabeed0_0[] = {
	{ 0x0000000000000000ull, 0x0000c00000000000ull },
	{ 0x0000400000000000ull, 0x0000c00000000000ull, N("cg") },
	{ 0x0000800000000000ull, 0x0000c00000000000ull, N("ci") },
	{ 0x0000c00000000000ull, 0x0000c00000000000ull, N("cv") },
	{ 0, 0, OOPS },
};

static struct insn tabeed0sz[] = {
	{ 0x0000000000000000ull, 0x0007000000000000ull, N("u8") },
	{ 0x0001000000000000ull, 0x0007000000000000ull, N("s8") },
	{ 0x0002000000000000ull, 0x0007000000000000ull, N("u16") },
	{ 0x0003000000000000ull, 0x0007000000000000ull, N("s16") },
	{ 0x0004000000000000ull, 0x0007000000000000ull, N("b32") },
	{ 0x0005000000000000ull, 0x0007000000000000ull, N("b64") },
	{ 0x0006000000000000ull, 0x0007000000000000ull, N("b128") },
	{ 0x0007000000000000ull, 0x0007000000000000ull, N("u"), N("b128") },
	{ 0, 0, OOPS },
};

static struct insn tabee00_0[] = {
	{ 0x0000000000000000ull, 0x0060000000000000ull, N("cast") },
	{ 0x0020000000000000ull, 0x0060000000000000ull, N("cast"), N("spin") },
	{ 0x0040000000000000ull, 0x0060000000000000ull, N("cas") },
	{ 0, 0, OOPS },
};

static struct insn tabee00sz[] = {
	{ 0x0000000000000000ull, 0x0010000000000000ull, N("b32") },
	{ 0x0010000000000000ull, 0x0010000000000000ull, N("b64") },
	{ 0, 0, OOPS },
};

static struct insn tabeef0sz[] = {
	{ 0x0000000000000000ull, 0x0002000000000000ull, N("b32") },
	{ 0x0002000000000000ull, 0x0002000000000000ull, N("b64") },
	{ 0, 0, OOPS },
};

static struct insn tabed00_0[] = {
	{ 0x0000000000000000ull, 0x00f0000000000000ull, N("add") },
	{ 0x0010000000000000ull, 0x00f0000000000000ull, N("min") },
	{ 0x0020000000000000ull, 0x00f0000000000000ull, N("max") },
	{ 0x0030000000000000ull, 0x00f0000000000000ull, N("inc") },
	{ 0x0040000000000000ull, 0x00f0000000000000ull, N("dec") },
	{ 0x0050000000000000ull, 0x00f0000000000000ull, N("and") },
	{ 0x0060000000000000ull, 0x00f0000000000000ull, N("or") },
	{ 0x0070000000000000ull, 0x00f0000000000000ull, N("xor") },
	{ 0x0080000000000000ull, 0x00f0000000000000ull, N("exch") },
	{ 0x00a0000000000000ull, 0x00f0000000000000ull, N("safeadd") },
	{ 0, 0, OOPS },
};

static struct insn tabed00sz[] = {
	{ 0x0000000000000000ull, 0x000e000000000000ull, N("u32") },
	{ 0x0002000000000000ull, 0x000e000000000000ull, N("s32") },
	{ 0x0004000000000000ull, 0x000e000000000000ull, N("u64") },
	{ 0x0006000000000000ull, 0x000e000000000000ull, N("ftz"), N("rn"), N("f32") },
	{ 0x0008000000000000ull, 0x000e000000000000ull, N("u128") },
	{ 0x000a000000000000ull, 0x000e000000000000ull, N("s64") },
	{ 0, 0, OOPS },
};

static struct insn tabec00_0[] = {
	{ 0x0000000000000000ull, 0x00f0000000000000ull, N("add") },
	{ 0x0010000000000000ull, 0x00f0000000000000ull, N("min") },
	{ 0x0020000000000000ull, 0x00f0000000000000ull, N("max") },
	{ 0x0030000000000000ull, 0x00f0000000000000ull, N("inc") },
	{ 0x0040000000000000ull, 0x00f0000000000000ull, N("dec") },
	{ 0x0050000000000000ull, 0x00f0000000000000ull, N("and") },
	{ 0x0060000000000000ull, 0x00f0000000000000ull, N("or") },
	{ 0x0070000000000000ull, 0x00f0000000000000ull, N("xor") },
	{ 0x0080000000000000ull, 0x00f0000000000000ull, N("exch") },
	{ 0, 0, OOPS },
};

static struct insn tabec00sz[] = {
	{ 0x0000000000000000ull, 0x0000000030000000ull, N("u32") },
	{ 0x0000000010000000ull, 0x0000000030000000ull, N("s32") },
	{ 0x0000000020000000ull, 0x0000000030000000ull, N("u64") },
	{ 0x0000000030000000ull, 0x0000000030000000ull, N("s64") },
	{ 0, 0, OOPS },
};

static struct insn tabebf8_0[] = {
	{ 0x0000000000000000ull, 0x0000000003800000ull, N("add") },
	{ 0x0000000000800000ull, 0x0000000003800000ull, N("min") },
	{ 0x0000000001000000ull, 0x0000000003800000ull, N("max") },
	{ 0x0000000001800000ull, 0x0000000003800000ull, N("inc") },
	{ 0x0000000002000000ull, 0x0000000003800000ull, N("dec") },
	{ 0x0000000002800000ull, 0x0000000003800000ull, N("and") },
	{ 0x0000000003000000ull, 0x0000000003800000ull, N("or") },
	{ 0x0000000003800000ull, 0x0000000003800000ull, N("xor") },
	{ 0, 0, OOPS },
};

static struct insn tabebf8sz[] = {
	{ 0x0000000000000000ull, 0x0000000000700000ull, N("u32") },
	{ 0x0000000000100000ull, 0x0000000000700000ull, N("s32") },
	{ 0x0000000000200000ull, 0x0000000000700000ull, N("u64") },
	{ 0x0000000000300000ull, 0x0000000000700000ull, N("ftz"), N("rn"), N("f32") },
	{ 0x0000000000400000ull, 0x0000000000700000ull, N("u128") },
	{ 0x0000000000500000ull, 0x0000000000700000ull, N("s64") },
	{ 0, 0, OOPS },
};

static struct insn tabebf0_0[] = {
	{ 0x0000000000000003ull, 0x0000000000000000ull, N("ivall") },
	{ 0x0000000000000003ull, 0x0000000000000000ull, N("ivth"), U13_36 },
	{ 0, 0, OOPS },
};

static struct insn tabebe0_0[] = {
	{ 0x0000008000000000ull, 0x0000018000000000ull, N("emit") },
	{ 0x0000010000000000ull, 0x0000018000000000ull, N("cut") },
	{ 0x0000018000000000ull, 0x0000018000000000ull, N("emit_then_cut") },
	{ 0, 0, OOPS },
};

static struct insn tabeb40_0[] = {
	{ 0x0000000000000000ull, 0x0010000000000000ull, N("p") },
	{ 0x0010000000000000ull, 0x0010000000000000ull, N("d") },
	{ 0, 0, OOPS },
};

static struct insn tabeb40_1[] = {
	{ 0x0000000000000000ull, 0x0000000e00000000ull, N("t1d") },
	{ 0x0000000200000000ull, 0x0000000e00000000ull, N("b1d") },
	{ 0x0000000400000000ull, 0x0000000e00000000ull, N("a1d") },
	{ 0x0000000600000000ull, 0x0000000e00000000ull, N("t2d") },
	{ 0x0000000800000000ull, 0x0000000e00000000ull, N("a2d") },
	{ 0x0000000a00000000ull, 0x0000000e00000000ull, N("t3d") },
	{ 0, 0, OOPS },
};

static struct insn tabeb40_2[] = {
	{ 0x0000000000000000ull, 0x0000000007000000ull, N("add") },
	{ 0x0000000001000000ull, 0x0000000007000000ull, N("min") },
	{ 0x0000000002000000ull, 0x0000000007000000ull, N("max") },
	{ 0x0000000003000000ull, 0x0000000007000000ull, N("inc") },
	{ 0x0000000004000000ull, 0x0000000007000000ull, N("dec") },
	{ 0x0000000005000000ull, 0x0000000007000000ull, N("and") },
	{ 0x0000000006000000ull, 0x0000000007000000ull, N("or") },
	{ 0x0000000007000000ull, 0x0000000007000000ull, N("xor") },
	{ 0, 0, OOPS },
};

static struct insn tabeb40_3[] = {
	{ 0x0000000000000000ull, 0x0006000000000000ull, N("ign") },
	{ 0x0002000000000000ull, 0x0006000000000000ull },
	{ 0x0004000000000000ull, 0x0006000000000000ull, N("trap") },
	{ 0, 0, OOPS },
};

static struct insn tabeb40_4[] = {
	{ 0x0000000000000000ull, 0x0008000000000000ull, REG_39 },
	{ 0x0008000000000000ull, 0x0008000000000000ull, U36_13 },
	{ 0, 0, OOPS },
};

static struct insn tabeb20_0[] = {
	{ 0x0010000000000000ull, 0x0010000000800000ull, N("ba") },
	{ 0x0000000000000000ull, 0x0000000000000000ull },
};

static struct insn tabeb20_1[] = {
	{ 0x0000000000100000ull, 0x0010000000f00000ull, N("r") },
	{ 0x0000000000200000ull, 0x0010000000f00000ull, N("g") },
	{ 0x0000000000300000ull, 0x0010000000f00000ull, N("rg") },
	{ 0x0000000000400000ull, 0x0010000000f00000ull, N("b") },
	{ 0x0000000000500000ull, 0x0010000000f00000ull, N("rb") },
	{ 0x0000000000600000ull, 0x0010000000f00000ull, N("gb") },
	{ 0x0000000000700000ull, 0x0010000000f00000ull, N("rgb") },
	{ 0x0000000000800000ull, 0x0010000000f00000ull, N("a") },
	{ 0x0000000000900000ull, 0x0010000000f00000ull, N("ra") },
	{ 0x0000000000a00000ull, 0x0010000000f00000ull, N("ga") },
	{ 0x0000000000b00000ull, 0x0010000000f00000ull, N("rga") },
	{ 0x0000000000c00000ull, 0x0010000000f00000ull, N("ba") },
	{ 0x0000000000d00000ull, 0x0010000000f00000ull, N("rba") },
	{ 0x0000000000e00000ull, 0x0010000000f00000ull, N("gba") },
	{ 0x0000000000f00000ull, 0x0010000000f00000ull, N("rgba") },
	{ 0x0010000000000000ull, 0x0010000000f00000ull, N("u8") },
	{ 0x0010000000100000ull, 0x0010000000f00000ull, N("s8") },
	{ 0x0010000000200000ull, 0x0010000000f00000ull, N("u16") },
	{ 0x0010000000300000ull, 0x0010000000f00000ull, N("s16") },
	{ 0x0010000000400000ull, 0x0010000000f00000ull, N("b32") },
	{ 0x0010000000500000ull, 0x0010000000f00000ull, N("b64") },
	{ 0x0010000000600000ull, 0x0010000000f00000ull, N("b128") },
	{ 0, 0, OOPS },
};

static struct insn tabeb20_2[] = {
	{ 0x0000000000000000ull, 0x0000000003000000ull },
	{ 0x0000000001000000ull, 0x0000000003000000ull, N("cg") },
	{ 0x0000000002000000ull, 0x0000000003000000ull, N("cs") },
	{ 0x0000000003000000ull, 0x0000000003000000ull, N("wt") },
	{ 0, 0, OOPS },
};

static struct insn tabeb00_0[] = {
	{ 0x0000000000000000ull, 0x0000000003000000ull },
	{ 0x0000000001000000ull, 0x0000000003000000ull, N("cg") },
	{ 0x0000000002000000ull, 0x0000000003000000ull, N("ci") },
	{ 0x0000000003000000ull, 0x0000000003000000ull, N("cv") },
	{ 0, 0, OOPS },
};

static struct insn tabe3a0_0[] = {
	{ 0x0000000000000040ull, 0x00000000000001c0ull, N("drain_illegal") },
	{ 0x0000000000000080ull, 0x00000000000001c0ull, N("cal") },
	{ 0x00000000000000c0ull, 0x00000000000001c0ull, N("pause") },
	{ 0x0000000000000100ull, 0x00000000000001c0ull, N("trap") },
	{ 0x0000000000000140ull, 0x00000000000001c0ull, N("int") },
	{ 0x0000000000000180ull, 0x00000000000001c0ull, N("drain") },
	{ 0, 0, OOPS },
};

static struct insn tabe360_0[] = {
	{ 0x0000000000000000ull, 0x0000000000000003ull },
	{ 0x0000000000000001ull, 0x0000000000000003ull, N("terminate") },
	{ 0x0000000000000002ull, 0x0000000000000003ull, N("fallthrough") },
	{ 0x0000000000000003ull, 0x0000000000000003ull, N("preempted") },
	{ 0, 0, OOPS },
};

static struct insn tabe000_0[] = {
	{ 0x0000000000000000ull, 0x00c0000000000000ull, N("pass") },
	{ 0x0040000000000000ull, 0x00c0000000000000ull },
	{ 0x0080000000000000ull, 0x00c0000000000000ull, N("constant") },
	{ 0x00c0000000000000ull, 0x00c0000000000000ull, N("sc") },
	{ 0, 0, OOPS },
};

static struct insn tabe000_1[] = {
	{ 0x0000000000000000ull, 0x0030000000000000ull },
	{ 0x0010000000000000ull, 0x0030000000000000ull, N("centroid") },
	{ 0x0020000000000000ull, 0x0030000000000000ull, N("offset") },
	{ 0, 0, OOPS },
};

static struct insn tabdf60_0[] = {
	{ 0x0000000000000000ull, 0x0000000060000000ull, N("t1d") },
	{ 0x0000000020000000ull, 0x0000000060000000ull, N("t2d") },
	{ 0x0000000040000000ull, 0x0000000060000000ull, N("t3d") },
	{ 0x0000000060000000ull, 0x0000000060000000ull, N("tcube") },
	{ 0, 0, OOPS },
};

static struct insn tabdf50_0[] = {
	{ 0x0000000000400000ull, 0x000000000fc00000ull, N("dimension") },
	{ 0x0000000000800000ull, 0x000000000fc00000ull, N("texture_type") },
	{ 0x0000000001400000ull, 0x000000000fc00000ull, N("sample_pos") },
	{ 0x0000000004000000ull, 0x000000000fc00000ull, N("filter") },
	{ 0x0000000004800000ull, 0x000000000fc00000ull, N("lod") },
	{ 0x0000000005000000ull, 0x000000000fc00000ull, N("wrap") },
	{ 0x0000000005800000ull, 0x000000000fc00000ull, N("border_color") },
	{ 0, 0, OOPS },
};

static struct insn tabdf00_0[] = {
	{ 0x0000000000000000ull, 0x0030000000000000ull, N("r") },
	{ 0x0010000000000000ull, 0x0030000000000000ull, N("g") },
	{ 0x0020000000000000ull, 0x0030000000000000ull, N("b") },
	{ 0x0030000000000000ull, 0x0030000000000000ull, N("a") },
	{ 0, 0, OOPS },
};

static struct insn tabdef8_0[] = {
	{ 0x0000000000000000ull, 0x000000c000000000ull, N("r") },
	{ 0x0000004000000000ull, 0x000000c000000000ull, N("g") },
	{ 0x0000008000000000ull, 0x000000c000000000ull, N("b") },
	{ 0x000000c000000000ull, 0x000000c000000000ull, N("a") },
	{ 0, 0, OOPS },
};

static struct insn tabdef8_1[] = {
	{ 0x0000000000000000ull, 0x0000003000000000ull },
	{ 0x0000001000000000ull, 0x0000003000000000ull, N("aoffi") },
	{ 0x0000002000000000ull, 0x0000003000000000ull, N("ptp") },
	{ 0, 0, OOPS },
};

static struct insn tabdeb8_0[] = {
	{ 0x0000000000000000ull, 0x000000e000000000ull },
	{ 0x0000002000000000ull, 0x000000e000000000ull, N("lz") },
	{ 0x0000004000000000ull, 0x000000e000000000ull, N("lb") },
	{ 0x0000006000000000ull, 0x000000e000000000ull, N("ll") },
	{ 0x000000c000000000ull, 0x000000e000000000ull, N("lba") },
	{ 0x000000e000000000ull, 0x000000e000000000ull, N("lla") },
	{ 0, 0, OOPS },
};

static struct insn tabdc38_0[] = {
	{ 0x0000000000000000ull, 0x0080000000000000ull, N("lz") },
	{ 0x0080000000000000ull, 0x0080000000000000ull, N("ll") },
	{ 0, 0, OOPS },
};

static struct insn tabd200_0[] = {
	{ 0x0000000000000000ull, 0x01e0000000000000ull, N("lz") },
	{ 0x0020000000000000ull, 0x01e0000000000000ull, N("ll") },
	{ 0x0040000000000000ull, 0x01e0000000000000ull, N("lz") },
	{ 0x0080000000000000ull, 0x01e0000000000000ull, N("lz"), N("aoffi") },
	{ 0x00a0000000000000ull, 0x01e0000000000000ull, N("ll") },
	{ 0x00c0000000000000ull, 0x01e0000000000000ull, N("lz"), N("mz") },
	{ 0x00e0000000000000ull, 0x01e0000000000000ull, N("lz") },
	{ 0x0100000000000000ull, 0x01e0000000000000ull, N("lz") },
	{ 0x0180000000000000ull, 0x01e0000000000000ull, N("ll"), N("aoffi") },
	{ 0, 0, OOPS },
};

static struct insn tabd200_1[] = {
	{ 0x0000000000000000ull, 0x01e0000000000000ull, N("t1d") },
	{ 0x0020000000000000ull, 0x01e0000000000000ull, N("t1d") },
	{ 0x0040000000000000ull, 0x01e0000000000000ull, N("t2d") },
	{ 0x0080000000000000ull, 0x01e0000000000000ull, N("t2d") },
	{ 0x00a0000000000000ull, 0x01e0000000000000ull, N("t2d") },
	{ 0x00c0000000000000ull, 0x01e0000000000000ull, N("t2d") },
	{ 0x00e0000000000000ull, 0x01e0000000000000ull, N("t3d") },
	{ 0x0100000000000000ull, 0x01e0000000000000ull, N("a2d") },
	{ 0x0180000000000000ull, 0x01e0000000000000ull, N("t2d") },
	{ 0, 0, OOPS },
};

static struct insn tabd200_2[] = {
	{ 0x0000000ff0000000ull, 0x001c000ff0000000ull, N("r") },
	{ 0x0004000ff0000000ull, 0x001c000ff0000000ull, N("g") },
	{ 0x0008000ff0000000ull, 0x001c000ff0000000ull, N("b") },
	{ 0x000c000ff0000000ull, 0x001c000ff0000000ull, N("a") },
	{ 0x0010000ff0000000ull, 0x001c000ff0000000ull, N("rg") },
	{ 0x0014000ff0000000ull, 0x001c000ff0000000ull, N("ra") },
	{ 0x0018000ff0000000ull, 0x001c000ff0000000ull, N("ga") },
	{ 0x001c000ff0000000ull, 0x001c000ff0000000ull, N("ba") },
	{ 0x0000000000000000ull, 0x001c000000000000ull, N("rgb") },
	{ 0x0004000000000000ull, 0x001c000000000000ull, N("rga") },
	{ 0x0008000000000000ull, 0x001c000000000000ull, N("rba") },
	{ 0x000c000000000000ull, 0x001c000000000000ull, N("gba") },
	{ 0x0010000000000000ull, 0x001c000000000000ull, N("rgba") },
	{ 0, 0, OOPS },
};

static struct insn tabd000_0[] = {
	{ 0x0000000000000000ull, 0x01e0000000000000ull, N("lz") },
	{ 0x0020000000000000ull, 0x01e0000000000000ull },
	{ 0x0040000000000000ull, 0x01e0000000000000ull, N("lz") },
	{ 0x0060000000000000ull, 0x01e0000000000000ull, N("ll") },
	{ 0x0080000000000000ull, 0x01e0000000000000ull, N("dc") },
	{ 0x00a0000000000000ull, 0x01e0000000000000ull, N("ll"), N("dc") },
	{ 0x00c0000000000000ull, 0x01e0000000000000ull, N("lz"), N("dc") },
	{ 0x00e0000000000000ull, 0x01e0000000000000ull },
	{ 0x0100000000000000ull, 0x01e0000000000000ull, N("lz") },
	{ 0x0120000000000000ull, 0x01e0000000000000ull, N("lz"), N("dc") },
	{ 0x0140000000000000ull, 0x01e0000000000000ull },
	{ 0x0160000000000000ull, 0x01e0000000000000ull, N("lz") },
	{ 0x0180000000000000ull, 0x01e0000000000000ull },
	{ 0x01a0000000000000ull, 0x01e0000000000000ull, N("ll") },
	{ 0, 0, OOPS },
};

static struct insn tabd000_1[] = {
	{ 0x0000000000000000ull, 0x01e0000000000000ull, N("t1d") },
	{ 0x0020000000000000ull, 0x01e0000000000000ull, N("t2d") },
	{ 0x0040000000000000ull, 0x01e0000000000000ull, N("t2d") },
	{ 0x0060000000000000ull, 0x01e0000000000000ull, N("t2d") },
	{ 0x0080000000000000ull, 0x01e0000000000000ull, N("t2d") },
	{ 0x00a0000000000000ull, 0x01e0000000000000ull, N("t2d") },
	{ 0x00c0000000000000ull, 0x01e0000000000000ull, N("t2d") },
	{ 0x00e0000000000000ull, 0x01e0000000000000ull, N("a2d") },
	{ 0x0100000000000000ull, 0x01e0000000000000ull, N("a2d") },
	{ 0x0120000000000000ull, 0x01e0000000000000ull, N("a2d") },
	{ 0x0140000000000000ull, 0x01e0000000000000ull, N("t3d") },
	{ 0x0160000000000000ull, 0x01e0000000000000ull, N("t3d") },
	{ 0x0180000000000000ull, 0x01e0000000000000ull, N("tcube") },
	{ 0x01a0000000000000ull, 0x01e0000000000000ull, N("tcube") },
	{ 0, 0, OOPS },
};

static struct insn tabc838_0[] = {
	{ 0x0000000000000000ull, 0x0300000000000000ull, N("r") },
	{ 0x0100000000000000ull, 0x0300000000000000ull, N("g") },
	{ 0x0200000000000000ull, 0x0300000000000000ull, N("b") },
	{ 0x0300000000000000ull, 0x0300000000000000ull, N("a") },
	{ 0, 0, OOPS },
};

static struct insn tabc838_1[] = {
	{ 0x0000000000000000ull, 0x00c0000000000000ull },
	{ 0x0040000000000000ull, 0x00c0000000000000ull, N("aoffi") },
	{ 0x0080000000000000ull, 0x00c0000000000000ull, N("ptp") },
	{ 0, 0, OOPS },
};

static struct insn tabc038_0[] = {
	{ 0x0000000000000000ull, 0x0380000000000000ull },
	{ 0x0080000000000000ull, 0x0380000000000000ull, N("lz") },
	{ 0x0100000000000000ull, 0x0380000000000000ull, N("lb") },
	{ 0x0180000000000000ull, 0x0380000000000000ull, N("ll") },
	{ 0x0300000000000000ull, 0x0380000000000000ull, N("lba") },
	{ 0x0380000000000000ull, 0x0380000000000000ull, N("lla") },
	{ 0, 0, OOPS },
};

static struct insn taba000_0[] = {
	{ 0x0000000000000000ull, 0x0300000000000000ull },
	{ 0x0100000000000000ull, 0x0300000000000000ull, N("cg") },
	{ 0x0200000000000000ull, 0x0300000000000000ull, N("ct") },
	{ 0x0300000000000000ull, 0x0300000000000000ull, N("wt") },
	{ 0, 0, OOPS },
};

static struct insn taba000_1[] = {
	{ 0x0000000000000000ull, 0x00e0000000000000ull, N("u8") },
	{ 0x0020000000000000ull, 0x00e0000000000000ull, N("s8") },
	{ 0x0040000000000000ull, 0x00e0000000000000ull, N("u16") },
	{ 0x0060000000000000ull, 0x00e0000000000000ull, N("s16") },
	{ 0x0080000000000000ull, 0x00e0000000000000ull, N("b32") },
	{ 0x00a0000000000000ull, 0x00e0000000000000ull, N("b64") },
	{ 0x00c0000000000000ull, 0x00e0000000000000ull, N("b128") },
	{ 0, 0, OOPS },
};

static struct insn tab8000_0[] = {
	{ 0x0000000000000000ull, 0x0300000000000000ull },
	{ 0x0100000000000000ull, 0x0300000000000000ull, N("cg") },
	{ 0x0200000000000000ull, 0x0300000000000000ull, N("ci") },
	{ 0x0300000000000000ull, 0x0300000000000000ull, N("cv") },
	{ 0, 0, OOPS },
};

static struct insn tab7c00_0[] = {
	{ 0x0000000000000000ull, 0x001e000000000000ull, N("f") },
	{ 0x0002000000000000ull, 0x001e000000000000ull, N("lt") },
	{ 0x0004000000000000ull, 0x001e000000000000ull, N("eq") },
	{ 0x0006000000000000ull, 0x001e000000000000ull, N("le") },
	{ 0x0008000000000000ull, 0x001e000000000000ull, N("gt") },
	{ 0x000a000000000000ull, 0x001e000000000000ull, N("ne") },
	{ 0x000c000000000000ull, 0x001e000000000000ull, N("ge") },
	{ 0x000e000000000000ull, 0x001e000000000000ull, N("num") },
	{ 0x0010000000000000ull, 0x001e000000000000ull, N("nan") },
	{ 0x0012000000000000ull, 0x001e000000000000ull, N("ltu") },
	{ 0x0014000000000000ull, 0x001e000000000000ull, N("equ") },
	{ 0x0016000000000000ull, 0x001e000000000000ull, N("leu") },
	{ 0x0018000000000000ull, 0x001e000000000000ull, N("gtu") },
	{ 0x001a000000000000ull, 0x001e000000000000ull, N("neu") },
	{ 0x001c000000000000ull, 0x001e000000000000ull, N("geu") },
	{ 0x001e000000000000ull, 0x001e000000000000ull, N("t") },
	{ 0, 0, OOPS },
};

static struct insn tab6080_0[] = {
	{ 0x0000000000000000ull, 0x0600000000000000ull },
	{ 0x0200000000000000ull, 0x0600000000000000ull, N("ftz") },
	{ 0x0400000000000000ull, 0x0600000000000000ull, N("fmz") },
	{ 0, 0, OOPS },
};

static struct insn tab5f00_0[] = {
	{ 0x0000000000000000ull, 0x0001004000000000ull, N("u8") },
	{ 0x0000004000000000ull, 0x0001004000000000ull, N("u16") },
	{ 0x0001000000000000ull, 0x0001004000000000ull, N("s8") },
	{ 0x0001004000000000ull, 0x0001004000000000ull, N("s16") },
	{ 0, 0, OOPS },
};

static struct insn tab5f00_2[] = {
	{ 0x0000000000000000ull, 0x0018000000000000ull },
	{ 0x0008000000000000ull, 0x0018000000000000ull, N("shr7") },
	{ 0x0010000000000000ull, 0x0018000000000000ull, N("shr15") },
	{ 0, 0, OOPS },
};

static struct insn tab5f00_3[] = {
	{ 0x0000000000000000ull, 0x0000003000000000ull },
	{ 0x0000001000000000ull, 0x0000003000000000ull, N("b1") },
	{ 0x0000002000000000ull, 0x0000003000000000ull, N("b2") },
	{ 0x0000003000000000ull, 0x0000003000000000ull, N("b3") },
	{ 0, 0, OOPS },
};

static struct insn tab5d18_0[] = {
	{ 0x0000000000000000ull, 0x0000007800000000ull, N("f") },
	{ 0x0000000800000000ull, 0x0000007800000000ull, N("lt") },
	{ 0x0000001000000000ull, 0x0000007800000000ull, N("eq") },
	{ 0x0000001800000000ull, 0x0000007800000000ull, N("le") },
	{ 0x0000002000000000ull, 0x0000007800000000ull, N("gt") },
	{ 0x0000002800000000ull, 0x0000007800000000ull, N("ne") },
	{ 0x0000003000000000ull, 0x0000007800000000ull, N("ge") },
	{ 0x0000003800000000ull, 0x0000007800000000ull, N("num") },
	{ 0x0000004000000000ull, 0x0000007800000000ull, N("nan") },
	{ 0x0000004800000000ull, 0x0000007800000000ull, N("ltu") },
	{ 0x0000005000000000ull, 0x0000007800000000ull, N("equ") },
	{ 0x0000005800000000ull, 0x0000007800000000ull, N("leu") },
	{ 0x0000006000000000ull, 0x0000007800000000ull, N("gtu") },
	{ 0x0000006800000000ull, 0x0000007800000000ull, N("neu") },
	{ 0x0000007000000000ull, 0x0000007800000000ull, N("geu") },
	{ 0x0000007800000000ull, 0x0000007800000000ull, N("t") },
	{ 0, 0, OOPS },
};

static struct insn tab5d10_0[] = {
	{ 0x0000000000000000ull, 0x0006000000000000ull },
	{ 0x0002000000000000ull, 0x0006000000000000ull, N("f32") },
	{ 0x0004000000000000ull, 0x0006000000000000ull, N("mrg_h0") },
	{ 0x0006000000000000ull, 0x0006000000000000ull, N("mrg_h1") },
	{ 0, 0, OOPS },
};

static struct insn tab5d10_1[] = {
	{ 0x0000000000000000ull, 0x0001800000000000ull },
	{ 0x0000800000000000ull, 0x0001800000000000ull, N("f32") },
	{ 0x0001000000000000ull, 0x0001800000000000ull, N("h0_h0") },
	{ 0x0001800000000000ull, 0x0001800000000000ull, N("h1_h1") },
	{ 0, 0, OOPS },
};

static struct insn tab5d10_2[] = {
	{ 0x0000000000000000ull, 0x0000000030000000ull },
	{ 0x0000000010000000ull, 0x0000000030000000ull, N("f32") },
	{ 0x0000000020000000ull, 0x0000000030000000ull, N("h0_h0") },
	{ 0x0000000030000000ull, 0x0000000030000000ull, N("h1_h1") },
	{ 0, 0, OOPS },
};

static struct insn tab5d08_0[] = {
	{ 0x0000000000000000ull, 0x0000018000000000ull },
	{ 0x0000008000000000ull, 0x0000018000000000ull, N("ftz") },
	{ 0x0000010000000000ull, 0x0000018000000000ull, N("fmz") },
	{ 0, 0, OOPS },
};

static struct insn tab5d00_0[] = {
	{ 0x0000000000000000ull, 0x0000006000000000ull },
	{ 0x0000002000000000ull, 0x0000006000000000ull, N("ftz") },
	{ 0x0000004000000000ull, 0x0000006000000000ull, N("fmz") },
	{ 0, 0, OOPS },
};

static struct insn tab5d00_1[] = {
	{ 0x0000000000000000ull, 0x0000001800000000ull },
	{ 0x0000000800000000ull, 0x0000001800000000ull, N("f32") },
	{ 0x0000001000000000ull, 0x0000001800000000ull, N("h0_h0") },
	{ 0x0000001800000000ull, 0x0000001800000000ull, N("h1_h1") },
	{ 0, 0, OOPS },
};

static struct insn tab5cf8_1[] = {
	{ 0x0000000000000000ull, 0x0000006000000000ull },
	{ 0x0000004000000000ull, 0x0000006000000000ull, N("u64") },
	{ 0x0000006000000000ull, 0x0000006000000000ull, N("s64") },
	{ 0, 0, OOPS },
};

static struct insn tab5cf8_0[] = {
	{ 0x0000000000000000ull, 0x0003000000000000ull },
	{ 0x0001000000000000ull, 0x0003000000000000ull, N("hi") },
	{ 0x0002000000000000ull, 0x0003000000000000ull, N("x") },
	{ 0x0003000000000000ull, 0x0003000000000000ull, N("xhi") },
	{ 0, 0, OOPS },
};

static struct insn tab5cf0_0[] = {
	{ 0x0000000000000000ull, 0x0000010000000000ull, N("pr") },
	{ 0x0000010000000000ull, 0x0000010000000000ull, N("cc") },
	{ 0, 0, OOPS },
};

static struct insn tab5cf0_1[] = {
	{ 0x0000000000000000ull, 0x0000060000000000ull },
	{ 0x0000020000000000ull, 0x0000060000000000ull, N("b1") },
	{ 0x0000040000000000ull, 0x0000060000000000ull, N("b2") },
	{ 0x0000060000000000ull, 0x0000060000000000ull, N("b3") },
	{ 0, 0, OOPS },
};

static struct insn tab5ce0_0[] = {
	{ 0x0000000000000000ull, 0x0000000000001300ull, N("u8") },
	{ 0x0000000000000100ull, 0x0000000000001300ull, N("u16") },
	{ 0x0000000000000200ull, 0x0000000000001300ull, N("u32") },
	{ 0x0000000000001000ull, 0x0000000000001300ull, N("s8") },
	{ 0x0000000000001100ull, 0x0000000000001300ull, N("s16") },
	{ 0x0000000000001200ull, 0x0000000000001300ull, N("s32") },
	{ 0, 0, OOPS },
};

static struct insn tab5ce0_1[] = {
	{ 0x0000000000000000ull, 0x0000000000002c00ull, N("u8") },
	{ 0x0000000000000400ull, 0x0000000000002c00ull, N("u16") },
	{ 0x0000000000000800ull, 0x0000000000002c00ull, N("u32") },
	{ 0x0000000000002000ull, 0x0000000000002c00ull, N("s8") },
	{ 0x0000000000002400ull, 0x0000000000002c00ull, N("s16") },
	{ 0x0000000000002800ull, 0x0000000000002c00ull, N("s32") },
	{ 0, 0, OOPS },
};

static struct insn tab5cc0_0[] = {
	{ 0x0000000000000000ull, 0x0000006000000000ull },
	{ 0x0000002000000000ull, 0x0000006000000000ull, N("rs") },
	{ 0x0000004000000000ull, 0x0000006000000000ull, N("ls") },
	{ 0, 0, OOPS },
};

static struct insn tab5cc0_1[] = {
	{ 0x0000000000000000ull, 0x0000001800000000ull },
	{ 0x0000000800000000ull, 0x0000001800000000ull, N("h0") },
	{ 0x0000001000000000ull, 0x0000001800000000ull, N("h1") },
	{ 0, 0, OOPS },
};

static struct insn tab5cc0_2[] = {
	{ 0x0000000000000000ull, 0x0000000600000000ull },
	{ 0x0000000200000000ull, 0x0000000600000000ull, N("h0") },
	{ 0x0000000400000000ull, 0x0000000600000000ull, N("h1") },
	{ 0, 0, OOPS },
};

static struct insn tab5cc0_3[] = {
	{ 0x0000000000000000ull, 0x0000000180000000ull },
	{ 0x0000000080000000ull, 0x0000000180000000ull, N("h0") },
	{ 0x0000000100000000ull, 0x0000000180000000ull, N("h1") },
	{ 0, 0, OOPS },
};

static struct insn tab5cb8_0[] = {
	{ 0x0000000000000100ull, 0x0000000000000300ull, N("f16") },
	{ 0x0000000000000200ull, 0x0000000000000300ull, N("f32") },
	{ 0x0000000000000300ull, 0x0000000000000300ull, N("f64") },
	{ 0, 0, OOPS },
};

static struct insn tab5cb8_1[] = {
	{ 0x0000000000000000ull, 0x0000000000002c00ull, N("u8") },
	{ 0x0000000000000400ull, 0x0000000000002c00ull, N("u16") },
	{ 0x0000000000000800ull, 0x0000000000002c00ull, N("u32") },
	{ 0x0000000000000c00ull, 0x0000000000002c00ull, N("u64") },
	{ 0x0000000000002000ull, 0x0000000000002c00ull, N("s8") },
	{ 0x0000000000002400ull, 0x0000000000002c00ull, N("s16") },
	{ 0x0000000000002800ull, 0x0000000000002c00ull, N("s32") },
	{ 0x0000000000002c00ull, 0x0000000000002c00ull, N("s64") },
	{ 0, 0, OOPS },
};

static struct insn tab5cb8_2[] = {
	{ 0x0000000000000000ull, 0x0000018000000000ull },
	{ 0x0000008000000000ull, 0x0000018000000000ull, N("rm") },
	{ 0x0000010000000000ull, 0x0000018000000000ull, N("rp") },
	{ 0x0000018000000000ull, 0x0000018000000000ull, N("rz") },
	{ 0, 0, OOPS },
};

static struct insn tab5cb0_2[] = {
	{ 0x0000000000000000ull, 0x0000000000001300ull, N("u8") },
	{ 0x0000000000000100ull, 0x0000000000001300ull, N("u16") },
	{ 0x0000000000000200ull, 0x0000000000001300ull, N("u32") },
	{ 0x0000000000000300ull, 0x0000000000001300ull, N("u64") },
	{ 0x0000000000001000ull, 0x0000000000001300ull, N("s8") },
	{ 0x0000000000001100ull, 0x0000000000001300ull, N("s16") },
	{ 0x0000000000001200ull, 0x0000000000001300ull, N("s32") },
	{ 0x0000000000001300ull, 0x0000000000001300ull, N("u64") },
	{ 0, 0, OOPS },
};

static struct insn tab5cb0_0[] = {
	{ 0x0000000000000400ull, 0x0000000000000c00ull, N("f16") },
	{ 0x0000000000000800ull, 0x0000000000000c00ull, N("f32") },
	{ 0x0000000000000c00ull, 0x0000000000000c00ull, N("f64") },
	{ 0, 0, OOPS },
};

static struct insn tab5cb0_1[] = {
	{ 0x0000000000000000ull, 0x0000018000000000ull },
	{ 0x0000008000000000ull, 0x0000018000000000ull, N("floor") },
	{ 0x0000010000000000ull, 0x0000018000000000ull, N("ceil") },
	{ 0x0000018000000000ull, 0x0000018000000000ull, N("trunc") },
	{ 0, 0, OOPS },
};

static struct insn tab5ca8_0[] = {
	{ 0x0000000000000000ull, 0x0000058000000000ull },
	{ 0x0000018000000000ull, 0x0000058000000000ull, N("pass") },
	{ 0x0000040000000000ull, 0x0000058000000000ull, N("round") },
	{ 0x0000048000000000ull, 0x0000058000000000ull, N("floor") },
	{ 0x0000050000000000ull, 0x0000058000000000ull, N("ceil") },
	{ 0x0000058000000000ull, 0x0000058000000000ull, N("trunc") },
	{ 0, 0, OOPS },
};

static struct insn tab5c90_0[] = {
	{ 0x0000000000000000ull, 0x0000008000000000ull, N("sincos") },
	{ 0x0000008000000000ull, 0x0000008000000000ull, N("ex2") },
	{ 0, 0, OOPS },
};

static struct insn tab5c88_0[] = {
	{ 0x0000000000000000ull, 0x00001f8000000000ull, N("divide") },
	{ 0, 0, OOPS },
};

static struct insn tab5c68_0[] = {
	{ 0x0000000000000000ull, 0x0000300000000000ull },
	{ 0x0000100000000000ull, 0x0000300000000000ull, N("ftz") },
	{ 0x0000200000000000ull, 0x0000300000000000ull, N("fmz") },
	{ 0, 0, OOPS },
};

static struct insn tab5c68_1[] = {
	{ 0x0000000000000000ull, 0x00000e0000000000ull },
	{ 0x0000020000000000ull, 0x00000e0000000000ull, N("d2") },
	{ 0x0000040000000000ull, 0x00000e0000000000ull, N("d4") },
	{ 0x0000060000000000ull, 0x00000e0000000000ull, N("d8") },
	{ 0x0000080000000000ull, 0x00000e0000000000ull, N("m8") },
	{ 0x00000a0000000000ull, 0x00000e0000000000ull, N("m4") },
	{ 0x00000c0000000000ull, 0x00000e0000000000ull, N("m2") },
	{ 0, 0, OOPS },
};

static struct insn tab5c40_0[] = {
	{ 0x0000000000000000ull, 0x0000060000000000ull, N("and") },
	{ 0x0000020000000000ull, 0x0000060000000000ull, N("or") },
	{ 0x0000040000000000ull, 0x0000060000000000ull, N("xor") },
	{ 0x0000060000000000ull, 0x0000060000000000ull, N("pass_b") },
	{ 0, 0, OOPS },
};

static struct insn tab5c40_1[] = {
	{ 0x0000000000000000ull, 0x0000300000000000ull },
	{ 0x0000100000000000ull, 0x0000300000000000ull, N("t") },
	{ 0x0000200000000000ull, 0x0000300000000000ull, N("z") },
	{ 0x0000300000000000ull, 0x0000300000000000ull, N("nz") },
	{ 0, 0, OOPS },
};

static struct insn tab5c30_0[] = {
	{ 0x0000000000000000ull, 0x0001000000000000ull, N("u32") },
	{ 0x0001000000000000ull, 0x0001000000000000ull },
	{ 0, 0, OOPS },
};

static struct insn tab5c38_0[] = {
	{ 0x0000000000000000ull, 0x0000010000000000ull, N("u32") },
	{ 0x0000010000000000ull, 0x0000010000000000ull, N("s32") },
	{ 0, 0, OOPS },
};

static struct insn tab5c38_1[] = {
	{ 0x0000000000000000ull, 0x0000020000000000ull, N("u32") },
	{ 0x0000020000000000ull, 0x0000020000000000ull, N("s32") },
	{ 0, 0, OOPS },
};

static struct insn tab5c20_0[] = {
	{ 0x0000000000000000ull, 0x0000180000000000ull },
	{ 0x0000080000000000ull, 0x0000180000000000ull, N("xlo") },
	{ 0x0000100000000000ull, 0x0000180000000000ull, N("xmed") },
	{ 0x0000180000000000ull, 0x0000180000000000ull, N("xhi") },
	{ 0, 0, OOPS },
};

static struct insn tab5be0_0[] = {
	{ 0x0000000000000000ull, 0x0000003000000000ull },
	{ 0x0000001000000000ull, 0x0000003000000000ull, N("t") },
	{ 0x0000002000000000ull, 0x0000003000000000ull, N("z") },
	{ 0x0000003000000000ull, 0x0000003000000000ull, N("nz") },
	{ 0, 0, OOPS },
};

static struct insn tab5bc0_0[] = {
	{ 0x0000000000000000ull, 0x000f000000000000ull },
	{ 0x0001000000000000ull, 0x000f000000000000ull, N("f4e") },
	{ 0x0002000000000000ull, 0x000f000000000000ull, N("b4e") },
	{ 0x0003000000000000ull, 0x000f000000000000ull, N("rc8") },
	{ 0x0004000000000000ull, 0x000f000000000000ull, N("ecl") },
	{ 0x0005000000000000ull, 0x000f000000000000ull, N("ecr") },
	{ 0x0006000000000000ull, 0x000f000000000000ull, N("rc16") },
	{ 0, 0, OOPS },
};

static struct insn tab5bb0_0[] = {
	{ 0x0000000000000000ull, 0x000f000000000000ull, N("f") },
	{ 0x0001000000000000ull, 0x000f000000000000ull, N("lt") },
	{ 0x0002000000000000ull, 0x000f000000000000ull, N("eq") },
	{ 0x0003000000000000ull, 0x000f000000000000ull, N("le") },
	{ 0x0004000000000000ull, 0x000f000000000000ull, N("gt") },
	{ 0x0005000000000000ull, 0x000f000000000000ull, N("ne") },
	{ 0x0006000000000000ull, 0x000f000000000000ull, N("ge") },
	{ 0x0007000000000000ull, 0x000f000000000000ull, N("num") },
	{ 0x0008000000000000ull, 0x000f000000000000ull, N("nan") },
	{ 0x0009000000000000ull, 0x000f000000000000ull, N("ltu") },
	{ 0x000a000000000000ull, 0x000f000000000000ull, N("equ") },
	{ 0x000b000000000000ull, 0x000f000000000000ull, N("leu") },
	{ 0x000c000000000000ull, 0x000f000000000000ull, N("gtu") },
	{ 0x000d000000000000ull, 0x000f000000000000ull, N("neu") },
	{ 0x000e000000000000ull, 0x000f000000000000ull, N("geu") },
	{ 0x000f000000000000ull, 0x000f000000000000ull, N("t") },
	{ 0, 0, OOPS },
};

static struct insn tab5bb0_1[] = {
	{ 0x0000000000000000ull, 0x0000600000000000ull, N("and") },
	{ 0x0000200000000000ull, 0x0000600000000000ull, N("or") },
	{ 0x0000400000000000ull, 0x0000600000000000ull, N("xor") },
	{ 0, 0, OOPS },
};

static struct insn tab5b70_0[] = {
	{ 0x0000000000000000ull, 0x000c000000000000ull },
	{ 0x0004000000000000ull, 0x000c000000000000ull, N("rm") },
	{ 0x0008000000000000ull, 0x000c000000000000ull, N("rp") },
	{ 0x000c000000000000ull, 0x000c000000000000ull, N("rz") },
	{ 0, 0, OOPS },
};

static struct insn tab5b60_0[] = {
	{ 0x0000000000000000ull, 0x000e000000000000ull, N("f") },
	{ 0x0002000000000000ull, 0x000e000000000000ull, N("lt") },
	{ 0x0004000000000000ull, 0x000e000000000000ull, N("eq") },
	{ 0x0006000000000000ull, 0x000e000000000000ull, N("le") },
	{ 0x0008000000000000ull, 0x000e000000000000ull, N("gt") },
	{ 0x000a000000000000ull, 0x000e000000000000ull, N("ne") },
	{ 0x000c000000000000ull, 0x000e000000000000ull, N("ge") },
	{ 0x000e000000000000ull, 0x000e000000000000ull, N("t") },
	{ 0, 0, OOPS },
};

static struct insn tab5b00_0[] = {
	{ 0x0000000000000000ull, 0x0003000000000000ull },
	{ 0x0001000000000000ull, 0x0003000000000000ull, N("s16"), N("u16") },
	{ 0x0002000000000000ull, 0x0003000000000000ull, N("u16"), N("s16") },
	{ 0x0003000000000000ull, 0x0003000000000000ull, N("s16"), N("s16") },
	{ 0, 0, OOPS },
};

static struct insn tab5b00_1[] = {
	{ 0x0000000000000000ull, 0x001c000000000000ull },
	{ 0x0004000000000000ull, 0x001c000000000000ull, N("clo") },
	{ 0x0008000000000000ull, 0x001c000000000000ull, N("chi") },
	{ 0x000c000000000000ull, 0x001c000000000000ull, N("csfu") },
	{ 0x0010000000000000ull, 0x001c000000000000ull, N("cbcc") },
	{ 0, 0, OOPS },
};

static struct insn tab5a80_0[] = {
	{ 0x0000000000000000ull, 0x0007000000000000ull, N("u32") },
	{ 0x0001000000000000ull, 0x0007000000000000ull, N("s32") },
	{ 0x0002000000000000ull, 0x0007000000000000ull, N("u24") },
	{ 0x0003000000000000ull, 0x0007000000000000ull, N("s24") },
	{ 0x0004000000000000ull, 0x0007000000000000ull, N("u16h0") },
	{ 0x0005000000000000ull, 0x0007000000000000ull, N("s16h0") },
	{ 0x0006000000000000ull, 0x0007000000000000ull, N("u16h1") },
	{ 0x0007000000000000ull, 0x0007000000000000ull, N("s16h1") },
	{ 0, 0, OOPS },
};

static struct insn tab5a80_1[] = {
	{ 0x0000000000000000ull, 0x0060000000000000ull, N("u24") },
	{ 0x0020000000000000ull, 0x0060000000000000ull, N("s24") },
	{ 0x0040000000000000ull, 0x0060000000000000ull, N("u16h0") },
	{ 0x0060000000000000ull, 0x0060000000000000ull, N("s16h0") },
	{ 0, 0, OOPS },
};

static struct insn tab5a80_2[] = {
	{ 0x0000000000000000ull, 0x0019000000000000ull, N("u32") },
	{ 0x0001000000000000ull, 0x0019000000000000ull, N("s32") },
	{ 0x0008000000000000ull, 0x0019000000000000ull, N("u24") },
	{ 0x0009000000000000ull, 0x0019000000000000ull, N("s24") },
	{ 0x0010000000000000ull, 0x0019000000000000ull, N("u16h0") },
	{ 0x0011000000000000ull, 0x0019000000000000ull, N("s16h0") },
	{ 0, 0, OOPS },
};

static struct insn tab5a00_0[] = {
	{ 0x0000000000000000ull, 0x0001000000000000ull, N("u32") },
	{ 0x0001000000000000ull, 0x0001000000000000ull, N("s32") },
	{ 0, 0, OOPS },
};

static struct insn tab5a00_1[] = {
	{ 0x0000000000000000ull, 0x0020000000000000ull, N("u32") },
	{ 0x0020000000000000ull, 0x0020000000000000ull, N("s32") },
	{ 0, 0, OOPS },
};

static struct insn tab5980_0[] = {
	{ 0x0000000000000000ull, 0x0060000000000000ull },
	{ 0x0020000000000000ull, 0x0060000000000000ull, N("ftz") },
	{ 0x0040000000000000ull, 0x0060000000000000ull, N("fmz") },
	{ 0, 0, OOPS },
};

static struct insn tab5980_1[] = {
	{ 0x0000000000000000ull, 0x0018000000000000ull },
	{ 0x0008000000000000ull, 0x0018000000000000ull, N("rm") },
	{ 0x0010000000000000ull, 0x0018000000000000ull, N("rp") },
	{ 0x0018000000000000ull, 0x0018000000000000ull, N("rz") },
	{ 0, 0, OOPS },
};


static struct insn tab5700_0[] = {
	{ 0x0000000000000000ull, 0x0040000000000000ull, N("ud") },
	{ 0x0040000000000000ull, 0x0040000000000000ull },
	{ 0, 0, OOPS },
};

static struct insn tab5700_1[] = {
	{ 0x0000000000000000ull, 0x0001004000000000ull, N("u8") },
	{ 0x0000004000000000ull, 0x0001004000000000ull, N("u16") },
	{ 0x0001000000000000ull, 0x0001004000000000ull, N("s8") },
	{ 0x0001004000000000ull, 0x0001004000000000ull, N("s16") },
	{ 0, 0, OOPS },
};

static struct insn tab5700_2[] = {
	{ 0x0000000000000000ull, 0x0004000000000000ull, N("u16") },
	{ 0x0004000000000000ull, 0x0004000000000000ull, N("u8") },
	{ 0, 0, OOPS },
};

static struct insn tab5700_3[] = {
	{ 0x0000000000000000ull, 0x0038000000000000ull, N("mrg_16h") },
	{ 0x0008000000000000ull, 0x0038000000000000ull, N("mrg_16l") },
	{ 0x0010000000000000ull, 0x0038000000000000ull, N("mrg_8b0") },
	{ 0x0018000000000000ull, 0x0038000000000000ull, N("mrg_8b2") },
	{ 0x0020000000000000ull, 0x0038000000000000ull, N("acc") },
	{ 0x0028000000000000ull, 0x0038000000000000ull, N("min") },
	{ 0x0030000000000000ull, 0x0038000000000000ull, N("max") },
	{ 0x0038000000000000ull, 0x0038000000000000ull },
	{ 0, 0, OOPS },
};

static struct insn tab53d8_0[] = {
	{ 0x0000000000000000ull, 0x0007000000000000ull, N("4a"), N("u8") },
	{ 0x0001000000000000ull, 0x0007000000000000ull, N("2a"), N("lo"), N("u16") },
	{ 0x0002000000000000ull, 0x0007000000000000ull, N("4a"), N("s8") },
	{ 0x0003000000000000ull, 0x0007000000000000ull, N("2a"), N("lo"), N("s16") },
	{ 0x0005000000000000ull, 0x0007000000000000ull, N("2a"), N("hi"), N("u16") },
	{ 0x0007000000000000ull, 0x0007000000000000ull, N("2a"), N("hi"), N("s16") },
	{ 0, 0, OOPS },
};

static struct insn tab53d8_1[] = {
	{ 0x0000000000000000ull, 0x0000800000000000ull, N("u8") },
	{ 0x0000800000000000ull, 0x0000800000000000ull, N("s8") },
	{ 0, 0, OOPS },
};

static struct insn tab5100_0[] = {
	{ 0x0000000000000000ull, 0x000c000000000000ull },
	{ 0x0004000000000000ull, 0x000c000000000000ull, N("clo") },
	{ 0x0008000000000000ull, 0x000c000000000000ull, N("chi") },
	{ 0x000c000000000000ull, 0x000c000000000000ull, N("csfu") },
	{ 0, 0, OOPS },
};

static struct insn tab50f0_0[] = {
	{ 0x0000000000000000ull, 0x0000980000000000ull, N("f") },
	{ 0x0000080000000000ull, 0x0000980000000000ull, N("lt") },
	{ 0x0000100000000000ull, 0x0000980000000000ull, N("eq") },
	{ 0x0000180000000000ull, 0x0000980000000000ull, N("le") },
	{ 0x0000800000000000ull, 0x0000980000000000ull, N("gt") },
	{ 0x0000880000000000ull, 0x0000980000000000ull, N("ne") },
	{ 0x0000900000000000ull, 0x0000980000000000ull, N("ge") },
	{ 0x0000980000000000ull, 0x0000980000000000ull, N("t") },
	{ 0, 0, OOPS },
};

static struct insn tab50f0_1_const[] = {
	{ 0x0000000000000000ull, 0x0002000000000000ull, U16_20 },
	{ 0x0002000000000000ull, 0x0002000000000000ull, S16_20 },
	{ 0, 0, OOPS },
};

static struct insn tab50f0_1_reg_8[] = {
	{ 0x0000000000000000ull, 0x0000000030000000ull, N("b0") },
	{ 0x0000000010000000ull, 0x0000000030000000ull, N("b1") },
	{ 0x0000000020000000ull, 0x0000000030000000ull, N("b2") },
	{ 0x0000000030000000ull, 0x0000000030000000ull, N("b3") },
	{ 0, 0, OOPS },
};

static struct insn tab50f0_1_size[] = {
	{ 0x0000000000000000ull, 0x0002000040000000ull, N("u8"), T(50f0_1_reg_8) },
	{ 0x0002000000000000ull, 0x0002000040000000ull, N("s8"), T(50f0_1_reg_8) },
	{ 0x0000000040000000ull, 0x0002000070000000ull, N("u16"), N("h0") },
	{ 0x0002000040000000ull, 0x0002000070000000ull, N("s16"), N("h0") },
	{ 0x0000000050000000ull, 0x0002000070000000ull, N("u16"), N("h1") },
	{ 0x0002000050000000ull, 0x0002000070000000ull, N("s16"), N("h1") },
	{ 0x0000000060000000ull, 0x0002000070000000ull, N("u32") },
	{ 0x0002000060000000ull, 0x0002000070000000ull, N("s32") },
	{ 0, 0, OOPS },
};

static struct insn tab50f0_1[] = {
	{ 0x0000000000000000ull, 0x0004000000000000ull, T(50f0_1_const) },
	{ 0x0004000000000000ull, 0x0004000000000000ull, T(50f0_1_size), REG_20 },
	{ 0, 0, OOPS },
};


static struct insn tab50e0_0[] = {
	{ 0x0000000000000000ull, 0x0003000000000000ull, N("r") },
	{ 0x0001000000000000ull, 0x0003000000000000ull, N("a") },
	{ 0x0002000000000000ull, 0x0003000000000000ull, N("ra") },
	{ 0, 0, OOPS },
};

static struct insn tab50d8_0[] = {
	{ 0x0000000000000000ull, 0x0003000000000000ull, N("all") },
	{ 0x0001000000000000ull, 0x0003000000000000ull, N("any") },
	{ 0x0002000000000000ull, 0x0003000000000000ull, N("eq") },
	{ 0, 0, OOPS },
};

static struct insn tab50b0_0[] = {
	{ 0x0000000000000000ull, 0x0000000000001f00ull, N("f") },
	{ 0x0000000000000100ull, 0x0000000000001f00ull, N("lt") },
	{ 0x0000000000000200ull, 0x0000000000001f00ull, N("eq") },
	{ 0x0000000000000300ull, 0x0000000000001f00ull, N("le") },
	{ 0x0000000000000400ull, 0x0000000000001f00ull, N("gt") },
	{ 0x0000000000000500ull, 0x0000000000001f00ull, N("ne") },
	{ 0x0000000000000600ull, 0x0000000000001f00ull, N("ge") },
	{ 0x0000000000000700ull, 0x0000000000001f00ull, N("num") },
	{ 0x0000000000000800ull, 0x0000000000001f00ull, N("nan") },
	{ 0x0000000000000900ull, 0x0000000000001f00ull, N("ltu") },
	{ 0x0000000000000a00ull, 0x0000000000001f00ull, N("equ") },
	{ 0x0000000000000b00ull, 0x0000000000001f00ull, N("leu") },
	{ 0x0000000000000c00ull, 0x0000000000001f00ull, N("gtu") },
	{ 0x0000000000000d00ull, 0x0000000000001f00ull, N("neu") },
	{ 0x0000000000000e00ull, 0x0000000000001f00ull, N("geu") },
	{ 0x0000000000000f00ull, 0x0000000000001f00ull },
	{ 0x0000000000001000ull, 0x0000000000001f00ull, N("off") },
	{ 0x0000000000001100ull, 0x0000000000001f00ull, N("lo") },
	{ 0x0000000000001200ull, 0x0000000000001f00ull, N("sff") },
	{ 0x0000000000001300ull, 0x0000000000001f00ull, N("ls") },
	{ 0x0000000000001400ull, 0x0000000000001f00ull, N("hi") },
	{ 0x0000000000001500ull, 0x0000000000001f00ull, N("sft") },
	{ 0x0000000000001600ull, 0x0000000000001f00ull, N("hs") },
	{ 0x0000000000001700ull, 0x0000000000001f00ull, N("oft") },
	{ 0x0000000000001800ull, 0x0000000000001f00ull, N("csm_ta") },
	{ 0x0000000000001900ull, 0x0000000000001f00ull, N("csm_tr") },
	{ 0x0000000000001a00ull, 0x0000000000001f00ull, N("csm_mx") },
	{ 0x0000000000001b00ull, 0x0000000000001f00ull, N("fcsm_ta") },
	{ 0x0000000000001c00ull, 0x0000000000001f00ull, N("fcsm_tr") },
	{ 0x0000000000001d00ull, 0x0000000000001f00ull, N("fcsm_mx") },
	{ 0x0000000000001e00ull, 0x0000000000001f00ull, N("rle") },
	{ 0x0000000000001f00ull, 0x0000000000001f00ull, N("rgt") },
	{ 0, 0, OOPS },
};

static struct insn tab5090_0[] = {
	{ 0x0000000000000000ull, 0x0000000003000000ull, N("and") },
	{ 0x0000000001000000ull, 0x0000000003000000ull, N("or") },
	{ 0x0000000002000000ull, 0x0000000003000000ull, N("xor") },
	{ 0, 0, OOPS },
};

static struct insn tab5080_0[] = {
	{ 0x0000000000000000ull, 0x0000000000f00000ull, N("cos") },
	{ 0x0000000000100000ull, 0x0000000000f00000ull, N("sin") },
	{ 0x0000000000200000ull, 0x0000000000f00000ull, N("ex2") },
	{ 0x0000000000300000ull, 0x0000000000f00000ull, N("lg2") },
	{ 0x0000000000400000ull, 0x0000000000f00000ull, N("rcp") },
	{ 0x0000000000500000ull, 0x0000000000f00000ull, N("rsq") },
	{ 0x0000000000600000ull, 0x0000000000f00000ull, N("rcp64h") },
	{ 0x0000000000700000ull, 0x0000000000f00000ull, N("rsq64h") },
	{ 0x0000000000800000ull, 0x0000000000f00000ull, N("sqrt"), .fmask = F_SM52 },
	{ 0, 0, OOPS },
};

static struct insn tab5000_0[] = {
	{ 0x0000000000000000ull, 0x0001000000000000ull, N("u8") },
	{ 0x0001000000000000ull, 0x0001000000000000ull, N("s28") },
	{ 0, 0, OOPS },
};

static struct insn tab5000_1[] = {
	{ 0x0000000000000000ull, 0x0002000000000000ull, N("u8") },
	{ 0x0002000000000000ull, 0x0002000000000000ull, N("s8") },
	{ 0, 0, OOPS },
};

static struct insn tab5000_2[] = {
	{ 0x0000000000000000ull, 0x0018003000000000ull },
	{ 0x0008000000000000ull, 0x0018003000000000ull, N("x") },
	{ 0x0010000000000000ull, 0x0018003000000000ull, N("y") },
	{ 0x0018000000000000ull, 0x0018003000000000ull, N("xy") },
	{ 0x0000001000000000ull, 0x0018003000000000ull, N("z") },
	{ 0x0008001000000000ull, 0x0018003000000000ull, N("xz") },
	{ 0x0010001000000000ull, 0x0018003000000000ull, N("xy") },
	{ 0x0018001000000000ull, 0x0018003000000000ull, N("xyz") },
	{ 0x0000002000000000ull, 0x0018003000000000ull, N("w") },
	{ 0x0008002000000000ull, 0x0018003000000000ull, N("xw") },
	{ 0x0010002000000000ull, 0x0018003000000000ull, N("yw") },
	{ 0x0018002000000000ull, 0x0018003000000000ull, N("xyw") },
	{ 0x0000003000000000ull, 0x0018003000000000ull, N("zw") },
	{ 0x0008003000000000ull, 0x0018003000000000ull, N("xzw") },
	{ 0x0010003000000000ull, 0x0018003000000000ull, N("xyw") },
	{ 0x0018003000000000ull, 0x0018003000000000ull, N("xyzw") },
	{ 0, 0, OOPS },
};

static struct insn tab5000_3[] = {
	{ 0x0000000000000000ull, 0x0000000f00000000ull, N("0000") },
	{ 0x0000000100000000ull, 0x0000000f00000000ull, N("1111") },
	{ 0x0000000200000000ull, 0x0000000f00000000ull, N("2222") },
	{ 0x0000000300000000ull, 0x0000000f00000000ull, N("3333") },
	{ 0x0000000400000000ull, 0x0000000f00000000ull },
	{ 0x0000000500000000ull, 0x0000000f00000000ull, N("4321") },
	{ 0x0000000600000000ull, 0x0000000f00000000ull, N("5432") },
	{ 0x0000000700000000ull, 0x0000000f00000000ull, N("6543") },
	{ 0x0000000800000000ull, 0x0000000f00000000ull, N("3201") },
	{ 0x0000000900000000ull, 0x0000000f00000000ull, N("3012") },
	{ 0x0000000a00000000ull, 0x0000000f00000000ull, N("0213") },
	{ 0x0000000b00000000ull, 0x0000000f00000000ull, N("3120") },
	{ 0x0000000c00000000ull, 0x0000000f00000000ull, N("1230") },
	{ 0x0000000d00000000ull, 0x0000000f00000000ull, N("2310") },
	{ 0, 0, OOPS },
};

static struct insn tab5000_4[] = {
	{ 0x0000000000000000ull, 0x00000000f0000000ull, N("4444") },
	{ 0x0000000010000000ull, 0x00000000f0000000ull, N("5555") },
	{ 0x0000000020000000ull, 0x00000000f0000000ull, N("6666") },
	{ 0x0000000030000000ull, 0x00000000f0000000ull, N("7777") },
	{ 0x0000000040000000ull, 0x00000000f0000000ull },
	{ 0x0000000050000000ull, 0x00000000f0000000ull, N("6543") },
	{ 0x0000000060000000ull, 0x00000000f0000000ull, N("5432") },
	{ 0x0000000070000000ull, 0x00000000f0000000ull, N("4321") },
	{ 0x0000000080000000ull, 0x00000000f0000000ull, N("4567") },
	{ 0x0000000090000000ull, 0x00000000f0000000ull, N("6745") },
	{ 0x00000000a0000000ull, 0x00000000f0000000ull, N("5467") },
	{ 0, 0, OOPS },
};

static struct insn tab5000_5[] = {
	{ 0x0000000000000000ull, 0x0060000000000000ull },
	{ 0x0020000000000000ull, 0x0060000000000000ull, N("acc") },
	{ 0, 0, OOPS },
};

static struct insn tab2c00_0[] = {
	{ 0x0000000000000000ull, 0x0060000000000000ull },
	{ 0x0020000000000000ull, 0x0060000000000000ull, N("f32") },
	{ 0x0040000000000000ull, 0x0060000000000000ull, N("h0_h0") },
	{ 0x0060000000000000ull, 0x0060000000000000ull, N("h1_h1") },
	{ 0, 0, OOPS },
};

static struct insn tab2a00_0[] = {
	{ 0x0000000000000000ull, 0x0180000000000000ull },
	{ 0x0080000000000000ull, 0x0180000000000000ull, N("ftz") },
	{ 0x0100000000000000ull, 0x0180000000000000ull, N("fmz") },
	{ 0, 0, OOPS },
};

static struct insn tab1f00_0[] = {
	{ 0x0000000000000000ull, 0x0040000000000000ull, N("u32") },
	{ 0x0040000000000000ull, 0x0040000000000000ull, N("s32") },
	{ 0, 0, OOPS },
};

static struct insn tab1f00_1[] = {
	{ 0x0000000000000000ull, 0x0080000000000000ull, N("u32") },
	{ 0x0080000000000000ull, 0x0080000000000000ull, N("s32") },
	{ 0, 0, OOPS },
};

static struct insn tab1000_0[] = {
	{ 0x0000000000000000ull, 0x0200000000000000ull, N("u32") },
	{ 0x0200000000000000ull, 0x0200000000000000ull, N("s32") },
	{ 0, 0, OOPS },
};

static struct insn tab0400_0[] = {
	{ 0x0000000000000000ull, 0x0060000000000000ull, N("and") },
	{ 0x0020000000000000ull, 0x0060000000000000ull, N("or") },
	{ 0x0040000000000000ull, 0x0060000000000000ull, N("xor") },
	{ 0x0060000000000000ull, 0x0060000000000000ull, N("pass_b") },
	{ 0, 0, OOPS },
};

static struct insn tabf0a8_0[] = {
	{ 0x0000000200000000ull, 0x0000009b00000000ull, N("red"), N("popc") },
	{ 0x0000000300000000ull, 0x0000009b00000000ull, N("scan") },
	{ 0x0000000a00000000ull, 0x0000009b00000000ull, N("red"), N("and") },
	{ 0x0000001200000000ull, 0x0000009b00000000ull, N("red"), N("or") },
	{ 0x0000008000000000ull, 0x0000009b00000000ull, N("sync") },
	{ 0x0000008100000000ull, 0x0000009b00000000ull, N("arrive") },
	{ 0, 0, OOPS },
};

static struct insn tabf0a8_1[] = {
	{ 0x0000000000000000ull, 0x0000180000000000ull, REG_08, REG_20 },
	{ 0x0000080000000000ull, 0x0000180000000000ull, REG_08, U12_20 },
	{ 0x0000100000000000ull, 0x0000180000000000ull, U08_08, REG_20 },
	{ 0x0000180000000000ull, 0x0000180000000000ull, U08_08, U12_20 },
	{ 0, 0, OOPS },
};

static struct insn tabea80_0[] = {
	{ 0x0000000010000000ull, 0x0000000010000000ull, N("ba") },
	{ 0x0000000000000000ull, 0x0000000000000000ull },
	{ 0, 0, OOPS },
};

static struct insn tabea80_1[] = {
	{ 0x0000000000000000ull, 0x0000007000000000ull, N("u32") },
	{ 0x0000001000000000ull, 0x0000007000000000ull, N("s32") },
	{ 0x0000002000000000ull, 0x0000007000000000ull, N("u64") },
	{ 0x0000003000000000ull, 0x0000007000000000ull, N("f32") },
	{ 0x0000005000000000ull, 0x0000007000000000ull, N("s64") },
	{ 0x0000006000000000ull, 0x0000007000000000ull, N("sd32") },
	{ 0x0000007000000000ull, 0x0000007000000000ull, N("sd64") },
	{ 0, 0, OOPS },
};

static struct insn tabea80_2[] = {
	{ 0x0000000000000000ull, 0x0038000000000000ull, N("u32") },
	{ 0x0008000000000000ull, 0x0038000000000000ull, N("s32") },
	{ 0x0010000000000000ull, 0x0038000000000000ull, N("u64") },
	{ 0x0018000000000000ull, 0x0038000000000000ull, N("f32") },
	{ 0x0028000000000000ull, 0x0038000000000000ull, N("s64") },
	{ 0x0030000000000000ull, 0x0038000000000000ull, N("sd32") },
	{ 0x0038000000000000ull, 0x0038000000000000ull, N("sd64") },
	{ 0, 0, OOPS },
};

static struct insn tabea00_0[] = {
	{ 0x0000000000000000ull, 0x00000001e0000000ull, N("add") },
	{ 0x0000000020000000ull, 0x00000001e0000000ull, N("min") },
	{ 0x0000000040000000ull, 0x00000001e0000000ull, N("max") },
	{ 0x0000000060000000ull, 0x00000001e0000000ull, N("inc") },
	{ 0x0000000080000000ull, 0x00000001e0000000ull, N("dec") },
	{ 0x00000000a0000000ull, 0x00000001e0000000ull, N("and") },
	{ 0x00000000c0000000ull, 0x00000001e0000000ull, N("or") },
	{ 0x00000000e0000000ull, 0x00000001e0000000ull, N("xor") },
	{ 0x0000000100000000ull, 0x00000001e0000000ull, N("exch") },
	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0xfbe0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "out"), T(fbe0_0), REG_00, REG_08, REG_20 },
	{ 0xf6e0000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "out"), T(fbe0_0), REG_00, REG_08, S20_20 },
	{ 0xf0f8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "sync"), T(f0f8_0) },
	{ 0xf0f0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(     "depbar"), T(f0f0_0), U03_26, U06_20, U06_00 },
	{ 0xf0c8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "mov"), REG_00, SYS_20 },
	{ 0xf0c0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "r2b"), T(f0c0_0), U04_28, REG_20 },
	{ 0xf0b8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "b2r"), REG_00, U08_08 },
	{ 0xf0a8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "bar"), T(f0a8_0), T(f0a8_1), T(pred39) },
	{ 0xeff0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(         "st"), ON(31, p), T(eff0_0), AMEM, REG_00, REG_39 },
	{ 0xefe8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "pixld"), T(efe8_0), REG_00, PRED45, PLDMEM },
	{ 0xefd8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(         "ld"), ON(32, o), ON(31, p), T(eff0_0), REG_00, AMEM, REG_39 },
	{ 0xefd0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(     "isberd"), ON(32, o), T(efd0_0), ON(31, skew), T(efd0sz), REG_00, ISBERDMEM },
	{ 0xefa0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "al2p"), ON(32, o), T(eff0_0), PRED44, REG_00, REG_08, S11_20 },
	{ 0xef98000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(     "membar"), T(ef98_0), T(ef98_1) },
	{ 0xef90000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(         "ld"), T(ef90_0), T(ef90sz), REG_00, C36_08_S16_20 },
	{ 0xef80000000000000ull, 0xffe0000000000000ull, OP8B, T(pred), N(      "cctll"), T(ef80ct), T(ef80_0), CCTLLMEM },
	{ 0xef60000000000000ull, 0xffe0000000000000ull, OP8B, T(pred), N(       "cctl"), ON(52, e), T(ef60ct), T(ef60_0), CCTLMEM },
	{ 0xef58000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(         "st"), T(ef58sz), SMEM, REG_00 },
	{ 0xef50000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(         "st"), T(ef50_0), T(ef58sz), LMEM, REG_00 },
	{ 0xef48000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(         "ld"), ON(44, u), T(ef58sz), REG_00, SMEM },
	{ 0xef40000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(         "ld"), T(ef40_0), T(ef58sz), REG_00, LMEM },
	{ 0xef10000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "shfl"), T(ef10_0), PRED48, REG_00, REG_08, T(ef10_1) },
	{ 0xeef0000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "atom"), N("cas"), ON(48, e), T(eef0sz), REG_00, ATOMMEM0, REG_20 },
	{ 0xeed8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "stg"), ON(45, e), T(eed8_0), T(ef58sz), NCGMEM, REG_00 },
	{ 0xeed0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "ldg"), ON(45, e), T(eed0_0), T(eed0sz),             REG_00, NCGMEM },
	{ 0xeec8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "ldg"), ON(45, e), T(eed0_0), T(eed0sz), INV_PRED41, REG_00, NCGMEM_20 },
	{ 0xeea0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "stp"), ON(31, wait), U08_20 },
	{ 0xee00000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(      "atoms"), T(ee00_0), T(ee00sz), REG_00, ATOMMEM1, REG_20 /*, REG_20 + 1 */ },
	{ 0xed00000000000000ull, 0xff00000000000000ull, OP8B, T(pred), N(       "atom"), T(ed00_0), ON(48, e), T(ed00sz), REG_00, ATOMMEM0, REG_20 },
	{ 0xec00000000000000ull, 0xff00000000000000ull, OP8B, T(pred), N(      "atoms"), T(ec00_0), T(ec00sz), REG_00, ATOMMEM1, REG_20 },
	{ 0xebf8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "red"), T(ebf8_0), ON(48, e), T(ebf8sz), REDMEM0, REG_00 },
	{ 0xebf0000000000000ull, 0xfff9000000000000ull, OP8B, T(pred), N(      "cctlt"), T(ebf0_0) },
	{ 0xebe0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "out"), T(ebe0_0), REG_00, REG_08, C34_RZ_O14_20 },
	{ 0xeb40000000000000ull, 0xffe0000000000000ull, OP8B, T(pred), N(      "sured"), T(eb40_0), T(eb40_1), T(eb40_2), T(eb40_3), SUREDMEM, REG_00, T(eb40_4) },
	{ 0xeb20000000000000ull, 0xffe0000000000000ull, OP8B, T(pred), N(       "sust"), T(eb40_0), T(eb20_0), T(eb40_1), T(eb20_1), T(eb20_2), T(eb40_3), SUREDMEM, REG_00, T(eb40_4) },
	{ 0xeb00000000000000ull, 0xffe0000000000000ull, OP8B, T(pred), N(       "suld"), T(eb40_0),            T(eb40_1), T(eb20_1), T(eb00_0), T(eb40_3), REG_00, SUREDMEM, T(eb40_4) },
	{ 0xeac0000000000000ull, 0xffe0000000000000ull, OP8B, T(pred), N(     "suatom"), T(eb40_0), T(ea80_0), T(ea80_1), T(eb40_1), N("cas"), T(eb40_3), REG_00, SUREDMEM, REG_20, T(eb40_4) },
	{ 0xea80000000000000ull, 0xffc0000000000000ull, OP8B, T(pred), N(     "suatom"), N("d"), T(ea80_0), T(ea80_2), T(eb40_1), N("cas"), T(eb40_3), REG_00, SUREDMEM, REG_20, U36_13 },
	{ 0xea60000000000000ull, 0xffe0000000000000ull, OP8B, T(pred), N(     "suatom"), T(eb40_0), T(ea80_0), T(ea80_1), T(eb40_1), T(ea00_0), T(eb40_3), REG_00, SUREDMEM, REG_20, T(eb40_4) },
	{ 0xea00000000000000ull, 0xffc0000000000000ull, OP8B, T(pred), N(     "suatom"), N("d"), T(ea80_0), T(ea80_2), T(eb40_1), T(ea00_0), T(eb40_3), REG_00, SUREDMEM, REG_20, U36_13 },
	{ 0xe3a0000000000000ull, 0xfff0000000000000ull, OP8B,          N(        "bpt"), T(e3a0_0), U20_20 },
	{ 0xe390000000000000ull, 0xfff0000000000000ull, OP8B,          N(        "ide"), ZN(5, en), U16_20 },
	{ 0xe380000000000000ull, 0xfff0000000000000ull, OP8B,          N(        "ram") },
	{ 0xe370000000000000ull, 0xfff0000000000000ull, OP8B,          N(        "sam") },
	{ 0xe360000000000000ull, 0xfff0000000000000ull, OP8B,          N(        "rtt"), T(e360_0) },
	{ 0xe350000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "cont"), T(f0f8_0) },
	{ 0xe340000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(        "brk"), T(f0f8_0) },
	{ 0xe330000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(        "kil"), T(f0f8_0) },
	{ 0xe320000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(        "ret"), T(f0f8_0) },
	{ 0xe310000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(    "longjmp"), T(f0f8_0) },
	{ 0xe300000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "exit"), ON(5, "keeprefcount"), T(f0f8_0) },
	{ 0xe2f0000000000000ull, 0xfff0000000000000ull, OP8B,          N("setlmembase"), REG_08 },
	{ 0xe2e0000000000000ull, 0xfff0000000000000ull, OP8B,          N(  "setcrsptr"), REG_08 },
	{ 0xe2d0000000000000ull, 0xfff0000000000000ull, OP8B,          N("getlmembase"), REG_00 },
	{ 0xe2c0000000000000ull, 0xfff0000000000000ull, OP8B,          N(  "getcrsptr"), REG_00 },
	{ 0xe2b0000000000020ull, 0xfff0000000000020ull, OP8B,          N(       "pcnt"), C36_RZ_S16_20 },
	{ 0xe2b0000000000000ull, 0xfff0000000000020ull, OP8B,          N(       "pcnt"), BTARG },
	{ 0xe2a0000000000020ull, 0xfff0000000000020ull, OP8B,          N(        "pbk"), C36_RZ_S16_20 },
	{ 0xe2a0000000000000ull, 0xfff0000000000020ull, OP8B,          N(        "pbk"), BTARG },
	{ 0xe290000000000020ull, 0xfff0000000000020ull, OP8B,          N(        "ssy"), C36_RZ_S16_20 },
	{ 0xe290000000000000ull, 0xfff0000000000020ull, OP8B,          N(        "ssy"), BTARG },
	{ 0xe280000000000020ull, 0xfff0000000000020ull, OP8B,          N(   "plongjmp"), C36_RZ_S16_20 },
	{ 0xe280000000000000ull, 0xfff0000000000020ull, OP8B,          N(   "plongjmp"), BTARG },
	{ 0xe270000000000020ull, 0xfff0000000000020ull, OP8B,          N(       "pret"), ZN(6, noinc), C36_RZ_S16_20 },
	{ 0xe270000000000000ull, 0xfff0000000000020ull, OP8B,          N(       "pret"), ZN(6, noinc), BTARG },
	{ 0xe260000000000020ull, 0xfff0000000000020ull, OP8B,          N(        "cal"), ZN(6, noinc), C36_RZ_S16_20 },
	{ 0xe260000000000000ull, 0xfff0000000000020ull, OP8B,          N(        "cal"), ZN(6, noinc), BTARG },
	{ 0xe250000000000020ull, 0xfff0000000000020ull, OP8B, T(pred), N(        "brx"), ON(6, lmt), T(f0f8_0), C36_08_S16_20 },
	{ 0xe250000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(        "brx"), ON(6, lmt), T(f0f8_0), REG_08, S24_20 },
	{ 0xe240000000000020ull, 0xfff0000000000020ull, OP8B, T(pred), N(        "bra"), ON(7, u), ON(6, lmt), T(f0f8_0), C36_RZ_S16_20 },
	{ 0xe240000000000000ull, 0xfff0000000000020ull, OP8B, T(pred), N(        "bra"), ON(7, u), ON(6, lmt), T(f0f8_0), BTARG },
	{ 0xe230000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(      "pexit"), BTARG },
	{ 0xe220000000000020ull, 0xfff0000000000020ull, OP8B, T(pred), N(       "jcal"), ON(6, noinc), C36_RZ_S16_20 },
	{ 0xe220000000000000ull, 0xfff0000000000020ull, OP8B, T(pred), N(       "jcal"), ON(6, noinc), U32_20 },
	{ 0xe210000000000020ull, 0xfff0000000000020ull, OP8B, T(pred), N(        "jmp"), ON(7, u), ON(6, lmt), T(f0f8_0), C36_RZ_S16_20 },
	{ 0xe210000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(        "jmp"), ON(7, u), ON(6, lmt), T(f0f8_0), U32_20 },
	{ 0xe200000000000020ull, 0xfff0000000000020ull, OP8B, T(pred), N(        "jmx"), ON(6, lmt), T(f0f8_0), REG_08, C36_08_S16_20 },
	{ 0xe200000000000000ull, 0xfff0000000000020ull, OP8B, T(pred), N(        "jmx"), ON(6, lmt), T(f0f8_0), REG_08, S32_20 },
	{ 0xe00000000000ff00ull, 0xff0000400000ff00ull, OP8B, T(pred), N(        "ipa"), T(e000_0), T(e000_1), ON(51, sat), REG_00, AMEM28, REG_20, REG_39, T(pred47) },
	{ 0xe000004000000000ull, 0xff00004000000000ull, OP8B, T(pred), N(        "ipa"), N("idx"), T(e000_0), T(e000_1), ON(51, sat), REG_00, AMEMIDX, REG_20, REG_39, T(pred39) },
	{ 0xdf60000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "tmml"), N("b"), N("lod"), ON(35, ndv), ON(49, nodep), REG_00, REG_08, REG_20, N("0x0"), ON(28, array), T(df60_0), U04_31 },
	{ 0xdf58000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "tmml"),         N("lod"), ON(35, ndv), ON(49, nodep), REG_00, REG_08,           U13_36, ON(28, array), T(df60_0), U04_31 },
	{ 0xdf50000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "txq"), N("b"), ON(49, nodep), REG_00, REG_08, T(df50_0), U13_36, U04_31 },
	{ 0xdf48000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "txq"),         ON(49, nodep), REG_00, REG_08, T(df50_0), U13_36, U04_31 },
	{ 0xdf40000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "txa"), ON(35, ndv), ON(49, nodep), REG_00, REG_08, U13_36, U04_31 },
	{ 0xdf00000000000000ull, 0xff40000000000000ull, OP8B, T(pred), N(      "tld4s"), ONV(55, f16, F_SM60), T(df00_0), ON(51, aoffi), ON(50, dc), ON(49, nodep), REG_28, REG_00, REG_08, REG_20, U13_36 },
	{ 0xdef8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "tld4"), T(def8_0), N("b"), T(def8_1), ON(50, dc), ON(35, ndv), ON(49, nodep), REG_00, REG_08, REG_20, N("0x0"), ON(28, array), T(df60_0), U04_31 },
	{ 0xdeb8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "tex"), N("b"), T(deb8_0), ON(36, aoffi), ON(50, dc), ON(35, ndv), ON(49, nodep), REG_00, REG_08, REG_20, N("0x0"), ON(28, array), T(df60_0), U04_31 },
	{ 0xde78000000000000ull, 0xfffc000000000000ull, OP8B, T(pred), N(        "txd"), N("b"), ON(35, aoffi), ON(49, nodep), REG_00, REG_08, REG_20, N("0x0"), ON(28, array), T(df60_0), U04_31 },
	{ 0xde38000000000000ull, 0xfffc000000000000ull, OP8B, T(pred), N(        "txd"),         ON(35, aoffi), ON(49, nodep), REG_00, REG_08, REG_20,   U13_36, ON(28, array), T(df60_0), U04_31 },
	{ 0xdd38000000000000ull, 0xff38000000000000ull, OP8B, T(pred), N(        "tld"), N("b"), T(dc38_0), ON(35, aoffi), ON(50, ms), ON(54, cl), ON(49, nodep), REG_00, REG_08, REG_20, N("0x0"), ON(28, array), T(df60_0), U04_31 },
	{ 0xdc38000000000000ull, 0xff38000000000000ull, OP8B, T(pred), N(        "tld"),         T(dc38_0), ON(35, aoffi), ON(50, ms), ON(54, cl), ON(49, nodep), REG_00, REG_08, REG_20,   U13_36, ON(28, array), T(df60_0), U04_31 },
	{ 0xd200000000000000ull, 0xf600000000000000ull, OP8B, T(pred), N(       "tlds"), ZNV(59, f16, F_SM60), T(d200_0), ON(49, nodep), REG_28, REG_00, REG_08, REG_20, U13_36, T(d200_1), T(d200_2) },
	{ 0xd000000000000000ull, 0xf600000000000000ull, OP8B, T(pred), N(       "texs"), ZNV(59, f16, F_SM60), T(d000_0), ON(49, nodep), REG_28, REG_00, REG_08, REG_20, U13_36, T(d000_1), T(d200_2) },
	{ 0xc838000000000000ull, 0xfc38000000000000ull, OP8B, T(pred), N(       "tld4"), T(c838_0), T(c838_1), ON(50, dc), ON(35, ndv), ON(49, nodep), REG_00, REG_08, REG_20, U13_36, ON(28, array), T(df60_0), U04_31 },
	{ 0xc038000000000000ull, 0xfc38000000000000ull, OP8B, T(pred), N(        "tex"), T(c038_0), ON(54, aoffi), ON(50, dc), ON(49, nodep), REG_00, REG_08, REG_20, U13_36, ON(28, array), T(df60_0), U04_31 },
	{ 0xa000000000000000ull, 0xe000000000000000ull, OP8B, T(pred), N(         "st"), ON(52, e), T(a000_0), T(a000_1), GMEM, REG_00, PRED58},
	{ 0x8000000000000000ull, 0xe000000000000000ull, OP8B, T(pred), N(         "ld"), ON(52, e), T(8000_0), T(a000_1), REG_00, GMEM, PRED58},
	{ 0x7e80000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(     "hsetp2"), T(7c00_0), ON(53, h_and), ON(6, ftz), T(5bb0_1), PRED03, PRED00, ON(43, neg), ON(44, abs), T(5d10_1), REG_08, ON(56, neg), ON(54, abs), C34_RZ_O14_20, ON(42, not), PRED39, .fmask = F_SM60 },
	{ 0x7e00000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(     "hsetp2"), T(7c00_0), ON(53, h_and), ON(6, ftz), T(5bb0_1), PRED03, PRED00, ON(43, neg), ON(44, abs), T(5d10_1), REG_08, ON(56, neg), U09_30, ON(29, neg), U09_20, ON(42, not), PRED39, .fmask = F_SM60 },
	{ 0x7c80000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(      "hset2"), ON(53, bf), T(7c00_0), ON(54, ftz), T(5bb0_1), REG_00, ON(43, neg), ON(44, abs), T(5d10_1), REG_08, ON(56, neg), ON(54, abs), C34_RZ_O14_20, ON(42, not), PRED39, .fmask = F_SM60 },
	{ 0x7c00000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(      "hset2"), ON(53, bf), T(7c00_0), ON(54, ftz), T(5bb0_1), REG_00, ON(43, neg), ON(44, abs), T(5d10_1), REG_08, ON(56, neg), U09_30, ON(29, neg), U09_20, ON(42, not), PRED39, .fmask = F_SM60 },
	{ 0x7a00000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(      "hadd2"), ON(39, ftz), ON(52, sat), T(5d10_0), REG_00, ON(43, neg), ON(44, abs), T(5d10_1), REG_08, ON(56, neg), U09_30, ON(29, neg), U09_20, .fmask = F_SM60 },
	{ 0x7a80000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(      "hadd2"), ON(39, ftz), ON(52, sat), T(5d10_0), REG_00, ON(43, neg), ON(44, abs), T(5d10_1), REG_08, ON(56, neg), ON(54, abs), C34_RZ_O14_20, .fmask = F_SM60 },
	{ 0x7800000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(      "hmul2"), T(5d08_0), ON(52, sat), T(5d10_0), REG_00, ON(43, neg), ON(44, abs), T(5d10_1), REG_08, ON(56, neg), U09_30, ON(29, neg), U09_20, .fmask = F_SM60 },
	{ 0x7880000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(      "hmul2"), T(5d08_0), ON(52, sat), T(5d10_0), REG_00, ON(43, neg), ON(44, abs), T(5d10_1), REG_08, ON(54, abs), C34_RZ_O14_20, .fmask = F_SM60 },
	{ 0x7080000000000000ull, 0xf880000000000000ull, OP8B, T(pred), N(      "hfma2"), T(6080_0), ON(52, sat), T(5d10_0), REG_00, T(5d10_1), REG_08, ON(56, neg), C34_RZ_O14_20, ON(51, neg), T(2c00_0), REG_39, .fmask = F_SM60 },
	{ 0x7000000000000000ull, 0xf880000000000000ull, OP8B, T(pred), N(      "hfma2"), T(6080_0), ON(52, sat), T(5d10_0), REG_00, T(5d10_1), REG_08, ON(56, neg), U09_30, ON(29, neg), U09_20, ON(51, neg), T(2c00_0), REG_39, .fmask = F_SM60 },
	{ 0x6080000000000000ull, 0xf880000000000000ull, OP8B, T(pred), N(      "hfma2"), T(6080_0), ON(52, sat), T(5d10_0), REG_00, T(5d10_1), REG_08, ON(56, neg), T(2c00_0), REG_39, ON(51, neg), C34_RZ_O14_20, .fmask = F_SM60 },
	{ 0x5f00000000000000ull, 0xff00000000000000ull, OP8B, T(pred), N(       "vmad"), T(5f00_0), T(5f00_2), ON(55, sat), ON(47, cc), REG_00, T(5f00_3), REG_08, T(50f0_1), REG_39 },
	{ 0x5d20000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(     "hsetp2"), T(5d18_0), ON(49, h_and), ON(6, ftz), T(5bb0_1), PRED03, PRED00, ON(43, neg), ON(44, abs), T(5d10_1), REG_08, ON(31, neg), ON(30, abs), T(5d10_2), REG_20, ON(42, not), PRED39, .fmask = F_SM60 },
	{ 0x5d18000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "hset2"), ON(49, bf), T(5d18_0), ON(50, ftz), T(5bb0_1), REG_00, ON(43, neg), ON(44, abs), T(5d10_1), REG_08, ON(31, neg), ON(30, abs), T(5d10_2), REG_20, ON(42, not), PRED39, .fmask = F_SM60 },
	{ 0x5d10000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "hadd2"), ON(39, ftz), ON(32, sat), T(5d10_0), REG_00, ON(43, neg), ON(44, abs), T(5d10_1), REG_08, ON(31, neg), ON(30, abs), T(5d10_2), REG_20, .fmask = F_SM60 },
	{ 0x5d08000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "hmul2"), T(5d08_0), ON(32, sat), T(5d10_0), REG_00, ON(44, abs), T(5d10_1), REG_08, ON(31, neg), ON(30, abs), T(5d10_2), REG_20, .fmask = F_SM60 },
	{ 0x5d00000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "hfma2"), T(5d00_0), ON(32, sat), T(5d10_0), REG_00, T(5d10_1), REG_08, ON(31, neg), T(5d10_2), REG_20, ON(30, neg), T(5d00_1), REG_39, .fmask = F_SM60 },
	{ 0x5cf8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "shf"), N("r"), ON(50, w), T(5cf8_1), T(5cf8_0), ON(47, cc), REG_00, REG_08, REG_20, REG_39 },
	{ 0x5cf0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "r2p"), T(5cf0_0), T(5cf0_1), REG_08, REG_20 },
	{ 0x5ce8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "p2r"), T(5cf0_1), REG_00, T(5cf0_0), REG_08, REG_20 },
	{ 0x5ce0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "i2i"), T(5ce0_0), T(5ce0_1), ON(50, sat), ON(47, cc), REG_00, T(5cf0_1), ON(45, neg), ON(49, abs), REG_20 },
	{ 0x5cc0000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(      "iadd3"), T(5cc0_0), ON(48, x), ON(47, cc), REG_00, ON(51, neg), T(5cc0_1), REG_08, ON(50, neg), T(5cc0_2), REG_20, ON(49, neg), T(5cc0_3), REG_39 },
	{ 0x5cb8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "i2f"), T(5cb8_0), T(5cb8_1), T(5cb8_2), ON(47, cc), REG_00, ON(45, neg), ON(49, abs),  T(5cf0_1), REG_20 },
	{ 0x5cb0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "f2i"), ON(44, ftz), T(5cb0_2), T(5cb0_0), T(5cb0_1), ON(47, cc), REG_00, ON(45, neg), ON(49, abs), REG_20 },
	{ 0x5ca8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "f2f"), ON(44, ftz), T(5cb0_0), T(5cb8_0), T(5ca8_0), ON(50, sat), ON(47, cc), REG_00, ON(45, neg), ON(49, abs), REG_20},
	{ 0x5ca0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "sel"), REG_00, REG_08, REG_20, T(pred39) },
	{ 0x5c98000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "mov"), REG_00, REG_20, U04_39 },
	{ 0x5c90000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "rro"), T(5c90_0), REG_00, ON(45, neg), ON(49, abs), REG_20 },
	{ 0x5c88000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "fchk"), T(5c88_0), PRED03, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), REG_20 },
	{ 0x5c80000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "dmul"), T(5cb8_2), ON(47, cc), REG_00, REG_08, ON(48, neg), REG_20 },
	{ 0x5c70000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "dadd"), T(5cb8_2), ON(47, cc), REG_00, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), REG_20 },
	{ 0x5c68000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "fmul"), T(5c68_0), T(5c68_1), T(5cb8_2), ON(50, sat), ON(47, cc), REG_00, REG_08, ON(48, neg), REG_20 },
	{ 0x5c60000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "fmnmx"), ON(44, ftz), ON(47, cc), REG_00, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), REG_20, T(pred39) },
	{ 0x5c58000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "fadd"), ON(44, ftz), ON(50, sat), T(5cb8_2), ON(47, cc), REG_00, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), REG_20 },
	{ 0x5c50000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "dmnmx"), ON(47, cc), REG_00, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), REG_20, T(pred39) },
	{ 0x5c48000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "shl"), ON(39, w), ON(43, x), ON(47, cc), REG_00, REG_08, REG_20 },
	{ 0x5c40000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "lop"), T(5c40_0), ON(43, x), T(5c40_1), PRED48, ON(47, cc), REG_00, ON(39, inv), REG_08, ON(40, inv), REG_20 },
	{ 0x5c38000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "imul"), T(5c38_0), T(5c38_1), ON(39, hi), ON(47, cc), REG_00, REG_08, REG_20 },
	{ 0x5c30000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "flo"), T(5c30_0), ON(41, sh), ON(47, cc), REG_00, ON(40, inv), REG_20 },
	{ 0x5c28000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "shr"), T(5c30_0), ON(39, w), ON(44, x), ON(40, brev), ON(47, cc), REG_00, REG_08, REG_20 },
	{ 0x5c20000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "imnmx"), T(5c30_0), T(5c20_0), ON(47, cc), REG_00, REG_08, REG_20, T(pred39) },
	{ 0x5c18000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(     "iscadd"), ON(47, cc), REG_00, ON(49, neg), REG_08, ON(48, neg), REG_20, U05_39 },
	{ 0x5c10000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "iadd"), ON(50, sat), ON(43, x), ON(47, cc), REG_00, ON(49, neg), REG_08, ON(48, neg), REG_20 },
	{ 0x5c08000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "popc"), REG_00, ON(40, inv), REG_20 },
	{ 0x5c00000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "bfe"), T(5c30_0), ON(40, brev), ON(47, cc), REG_00, REG_08, REG_20 },
	{ 0x5bf8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "shf"), N("l"), ON(50, w), T(5cf8_1), T(5cf8_0), ON(47, cc), REG_00, REG_08, REG_20, REG_39  },
	{ 0x5bf0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "bfi"), ON(47, cc), REG_00, REG_08, REG_20, REG_39 },
	{ 0x5be0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "lop3"), N("lut"), ON(38, x), T(5be0_0), PRED48, ON(47, cc), REG_00, REG_08, REG_20, REG_39, U08_28 },
	{ 0x5bd8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "lea"), N("hi"), ON(38, x), PRED48, ON(47, cc), REG_00, ON(37, neg), REG_08, REG_20, REG_39, U05_28 },
	{ 0x5bd0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "lea"), ON(46, x), PRED48, ON(47, cc), REG_00, ON(45, neg), REG_08, REG_20, U05_39 },
	{ 0x5bc0000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "prmt"), T(5bc0_0), REG_00, REG_08, REG_20, REG_39 },
	{ 0x5bb0000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(      "fsetp"), T(5bb0_0), ON(47, ftz), T(5bb0_1), PRED03, PRED00, ON(43, neg), ON(7, abs), REG_08, ON(6, neg), ON(44, abs), REG_20, T(pred39) },
	{ 0x5ba0000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "fcmp"), T(5bb0_0), ON(47, ftz), REG_00, REG_08, REG_20, REG_39 },
	{ 0x5b80000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(      "dsetp"), T(5bb0_0), T(5bb0_1), PRED03, PRED00, ON(43, neg), ON(7, abs), REG_08, ON(6, neg), ON(44, abs), REG_20, T(pred39) },
	{ 0x5b70000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "dfma"), T(5b70_0), ON(47, cc), REG_00, REG_08, ON(48, neg), REG_20, ON(49, neg), REG_39 },
	{ 0x5b60000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(      "isetp"), T(5b60_0), T(5c30_0), ON(43, x), T(5bb0_1), PRED03, PRED00, REG_08, REG_20, T(pred39) },
	{ 0x5b50000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "iset"), ON(44, bf), T(5b60_0), T(5c30_0), ON(43, x), T(5bb0_1), ON(47, cc), REG_00, REG_08, REG_20, T(pred39) },
	{ 0x5b40000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "icmp"), T(5b60_0), T(5c30_0), REG_00, REG_08, REG_20, REG_39 },
	{ 0x5b00000000000000ull, 0xffc0000000000000ull, OP8B, T(pred), N(       "xmad"), T(5b00_0), ON(36, psl), ON(37, mrg), T(5b00_1), ON(38, x), ON(47, cc), REG_00, ON(53, h1), REG_08, ON(35, h1), REG_20, REG_39  },
	{ 0x5a80000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(     "imadsp"), T(5a80_0), T(5a80_1), T(5a80_2),  ON(47, cc), REG_00, REG_08, REG_20, REG_39 },
	{ 0x5a00000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(       "imad"), T(5a00_0), T(5a00_1), ON(54, hi), ON(50, sat), ON(49, x), ON(47, c), REG_00, REG_08, REG_20, REG_39 },
	{ 0x5980000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(       "ffma"), T(5980_0), T(5980_1), ON(50, sat), ON(47, cc), REG_00, REG_08, ON(48, neg), REG_20, ON(49, neg), REG_39 },
	{ 0x5900000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(       "dset"), ON(52, bf), T(5bb0_0), T(5bb0_1), ON(47, cc), REG_00, ON(43, neg), ON(54, abs), REG_08, ON(53, neg), ON(44, abs), REG_20, T(pred39)  },
	{ 0x5800000000000000ull, 0xff00000000000000ull, OP8B, T(pred), N(       "fset"), ON(52, bf), T(5bb0_0), T(5bb0_1), ON(55, ftz), ON(47, cc), REG_00, ON(43, neg), ON(54, abs), REG_08, ON(53, neg), ON(44, abs), REG_20, T(pred39) },
	{ 0x5700000000000000ull, 0xff00000000000000ull, OP8B, T(pred), N(       "vshl"), T(5700_0), T(5700_1), T(5700_2), ON(49, w), ON(55, sat), T(5700_3), ON(47, cc), REG_00, T(5f00_3), REG_08, U16_20, REG_39 },
	{ 0x5600000000000000ull, 0xff00000000000000ull, OP8B, T(pred), N(       "vshr"), T(5700_0), T(5700_1), T(5700_2), ON(49, w), ON(55, sat), T(5700_3), ON(47, cc), REG_00, T(5f00_3), REG_08, U16_20, REG_39 },
	{ 0x5400000000000000ull, 0xff00000000000000ull, OP8B, T(pred), N(   "vabsdiff"), ON(54, sd), T(5700_1), ON(55, sat), T(5700_3), ON(47, cc), REG_00, T(5f00_3), REG_08, T(50f0_1), REG_39 },
	{ 0x53f8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "idp"), T(53d8_0), REG_00, REG_08, T(53d8_1), REG_20, REG_39, .fmask = F_SM60 },
	{ 0x53f0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "bfi"), ON(47, cc), REG_00, REG_08, REG_39, C34_RZ_O14_20 },
	{ 0x53d8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "idp"), T(53d8_0), REG_00, REG_08, T(53d8_1), C34_RZ_O14_20, REG_39, .fmask = F_SM60 },
	{ 0x53c0000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "prmt"), T(5bc0_0), REG_00, REG_08, REG_39, C34_RZ_O14_20 },
	{ 0x53a0000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "fcmp"), T(5bb0_0), ON(47, ftz), REG_00, REG_08, REG_39, C34_RZ_O14_20 },
	{ 0x5370000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "dfma"), T(5b70_0), ON(47, cc), REG_00, REG_08, ON(48, neg), REG_39, ON(49, neg), C34_RZ_O14_20 },
	{ 0x5340000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "icmp"), T(5b60_0), T(5c30_0), REG_00, REG_08, REG_39, C34_RZ_O14_20 },
	{ 0x5280000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(     "imadsp"), T(5a80_0), T(5a80_1), T(5a80_2),  ON(47, cc), REG_00, REG_08, REG_39, C34_RZ_O14_20 },
	{ 0x5200000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(       "imad"), T(5a00_0), T(5a00_1), ON(54, hi), ON(50, sat), ON(49, x), ON(47, c), REG_00, REG_08, REG_39, C34_RZ_O14_20 },
	{ 0x5180000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(       "ffma"), T(5980_0), T(5980_1), ON(50, sat), ON(47, cc), REG_00, REG_08, ON(48, neg), REG_39, ON(49, neg), C34_RZ_O14_20 },
	{ 0x5100000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(       "xmad"), T(5b00_0), T(5100_0), ON(54, x), ON(47, cc), REG_00, ON(53, h1), REG_08, ON(52, h1), REG_39, C34_RZ_O14_20 },
	{ 0x50f8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(    "fswzadd"), ON(44, ftz), T(5cb8_2), ON(38, ndv), ON(47, cc), REG_00, REG_08, REG_20, U08_28 },
	{ 0x50f0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "vsetp"), T(50f0_0), T(5700_1), T(5bb0_1), PRED03, PRED00, T(5f00_3), REG_08, T(50f0_1),  T(pred39) },
	{ 0x50e0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "vote"), N("vtg"), T(50e0_0), U24_20 },
	{ 0x50d8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "vote"), T(50d8_0), REG_00, PRED45, T(pred39) },
	{ 0x50d0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "lepc"), REG_00 },
	{ 0x50c8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "cs2r"), REG_00, SYS_20 },
	{ 0x50b0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "nop"), ON(13, trig), T(50b0_0), U16_20 },
	{ 0x50a0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "csetp"), T(50b0_0), T(5bb0_1), PRED03, PRED00, N("cc"), T(pred39) },
	{ 0x5098000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "cset"), ON(44, bf), T(50b0_0), T(5bb0_1), REG_00, N("cc"), T(pred39) },
	{ 0x5090000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "psetp"), T(5090_0), T(5bb0_1), PRED03, PRED00, T(pred12), T(pred29), T(pred39) },
	{ 0x5088000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "pset"), ON(44, bf), T(5090_0), T(5bb0_1), ON(47, cc), REG_00, T(pred12), T(pred29), T(pred39) },
	{ 0x5080000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "mufu"), T(5080_0), ON(50, sat), REG_00, ON(48, neg), ON(46, abs), REG_08 },
	{ 0x5000000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(  "vabsdiff4"), ON(38, sd), T(5000_0), T(5000_1), ON(50, sat), T(5000_5), T(5000_2), REG_00, T(5000_3), REG_08, T(5000_4), REG_20, REG_39 },
	{ 0x4e00000000000000ull, 0xfe00000000000000ull, OP8B, T(pred), N(       "xmad"), T(5b00_0), ON(55, psl), ON(56, mrg), T(5100_0), ON(54, x), ON(47, cc), REG_00, ON(53, h1), REG_08, ON(52, h1), C34_RZ_O14_20, REG_39 },
	{ 0x4cf0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "r2p"), T(5cf0_0), T(5cf0_1), REG_08, C34_RZ_O14_20 },
	{ 0x4ce8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "p2r"), T(5cf0_1), REG_00, T(5cf0_0), REG_08, C34_RZ_O14_20 },
	{ 0x4ce0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "i2i"), T(5ce0_0), T(5ce0_1), ON(50, sat), ON(47, cc), REG_00, T(5cf0_1), ON(45, neg), ON(49, abs), C34_RZ_O14_20 },
	{ 0x4cc0000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(      "iadd3"), ON(48, x), ON(47, cc), REG_00, ON(51, neg), REG_08, ON(50, neg), C34_RZ_O14_20, ON(49, neg), REG_39 },
	{ 0x4cb8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "i2f"), T(5cb8_0), T(5cb8_1), T(5cb8_2), ON(47, cc), REG_00, ON(45, neg), ON(49, abs),  T(5cf0_1), C34_RZ_O14_20 },
	{ 0x4cb0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "f2i"), ON(44, ftz), T(5cb0_2), T(5cb0_0), T(5cb0_1), ON(47, cc), REG_00, ON(45, neg), ON(49, abs), C34_RZ_O14_20 },
	{ 0x4ca8000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "f2f"), ON(44, ftz), T(5cb0_0), T(5cb8_0), T(5ca8_0), ON(50, sat), ON(47, cc), REG_00, ON(45, neg), ON(49, abs), C34_RZ_O14_20 },
	{ 0x4ca0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "sel"), REG_00, REG_08, C34_RZ_O14_20, T(pred39) },
	{ 0x4c98000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "mov"), REG_00, C34_RZ_O14_20, U04_39 },
	{ 0x4c90000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "rro"), T(5c90_0), REG_00, ON(45, neg), ON(49, abs), C34_RZ_O14_20 },
	{ 0x4c88000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "fchk"), T(5c88_0), PRED03, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), C34_RZ_O14_20 },
	{ 0x4c80000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "dmul"), T(5cb8_2), ON(47, cc), REG_00, REG_08, ON(48, neg), C34_RZ_O14_20 },
	{ 0x4c70000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "dadd"), T(5cb8_2), ON(47, cc), REG_00, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), C34_RZ_O14_20 },
	{ 0x4c68000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "fmul"), T(5c68_0), T(5c68_1), T(5cb8_2), ON(50, sat), ON(47, cc), REG_00, REG_08, ON(48, neg), C34_RZ_O14_20 },
	{ 0x4c60000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "fmnmx"), ON(44, ftz), ON(47, cc), REG_00, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), C34_RZ_O14_20, T(pred39) },
	{ 0x4c58000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "fadd"), ON(44, ftz), ON(50, sat), T(5cb8_2), ON(47, cc), REG_00, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), C34_RZ_O14_20 },
	{ 0x4c50000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "dmnmx"), ON(47, cc), REG_00, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), C34_RZ_O14_20, T(pred39) },
	{ 0x4c48000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "shl"), ON(39, w), ON(43, x), ON(47, cc), REG_00, REG_08, C34_RZ_O14_20 },
	{ 0x4c40000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "lop"), T(5c40_0), ON(43, x), T(5c40_1), PRED48, ON(47, cc), REG_00, ON(39, inv), REG_08, ON(40, inv), C34_RZ_O14_20 },
	{ 0x4c38000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "imul"), T(5c38_0), T(5c38_1), ON(39, hi), ON(47, cc), REG_00, REG_08, C34_RZ_O14_20 },
	{ 0x4c30000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "flo"), T(5c30_0), ON(41, sh), ON(47, cc), REG_00, ON(40, inv), C34_RZ_O14_20 },
	{ 0x4c28000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "shr"), T(5c30_0), ON(39, w), ON(44, x), ON(40, brev), ON(47, cc), REG_00, REG_08, C34_RZ_O14_20 },
	{ 0x4c20000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(      "imnmx"), T(5c30_0), T(5c20_0), ON(47, cc), REG_00, REG_08, C34_RZ_O14_20, T(pred39) },
	{ 0x4c18000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(     "iscadd"), ON(47, cc), REG_00, ON(49, neg), REG_08, ON(48, neg), C34_RZ_O14_20, U05_39 },
	{ 0x4c10000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "iadd"), ON(50, sat), ON(43, x), ON(47, cc), REG_00, ON(49, neg), REG_08, ON(48, neg), C34_RZ_O14_20 },
	{ 0x4c08000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(       "popc"), REG_00, ON(40, inv), C34_RZ_O14_20 },
	{ 0x4c00000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "bfe"), T(5c30_0), ON(40, brev), ON(47, cc), REG_00, REG_08, C34_RZ_O14_20 },
	{ 0x4bf0000000000000ull, 0xfff8000000000000ull, OP8B, T(pred), N(        "bfi"), ON(47, cc), REG_00, REG_08, C34_RZ_O14_20, REG_39 },
	{ 0x4bd0000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(        "lea"), ON(46, x), PRED48, ON(47, cc), REG_00, ON(45, neg), REG_08, C34_RZ_O14_20, U05_39 },
	{ 0x4bc0000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "prmt"), T(5bc0_0), REG_00, REG_08, C34_RZ_O14_20, REG_39 },
	{ 0x4bb0000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(      "fsetp"), T(5bb0_0), ON(47, ftz), T(5bb0_1), PRED03, PRED00, ON(43, neg), ON(7, abs), REG_08, ON(6, neg), ON(44, abs), C34_RZ_O14_20, T(pred39) },
	{ 0x4ba0000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "fcmp"), T(5bb0_0), ON(47, ftz), REG_00, REG_08, C34_RZ_O14_20, REG_39 },
	{ 0x4b80000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(      "dsetp"), T(5bb0_0), T(5bb0_1), PRED03, PRED00, ON(43, neg), ON(7, abs), REG_08, ON(6, neg), ON(44, abs), C34_RZ_O14_20, T(pred39) },
	{ 0x4b70000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "dfma"), T(5b70_0), ON(47, cc), REG_00, REG_08, ON(48, neg), C34_RZ_O14_20, ON(49, neg), REG_39 },
	{ 0x4b60000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(      "isetp"), T(5b60_0), T(5c30_0), ON(43, x), T(5bb0_1), PRED03, PRED00, REG_08, C34_RZ_O14_20, T(pred39) },
	{ 0x4b50000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "iset"), ON(44, bf), T(5b60_0), T(5c30_0), ON(43, x), T(5bb0_1), ON(47, cc), REG_00, REG_08, C34_RZ_O14_20, T(pred39) },
	{ 0x4b40000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(       "icmp"), T(5b60_0), T(5c30_0), REG_00, REG_08, C34_RZ_O14_20, REG_39 },
	{ 0x4a80000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(     "imadsp"), T(5a80_0), T(5a80_1), T(5a80_2),  ON(47, cc), REG_00, REG_08, C34_RZ_O14_20, REG_39 },
	{ 0x4a00000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(       "imad"), T(5a00_0), T(5a00_1), ON(54, hi), ON(50, sat), ON(49, x), ON(47, cc), REG_00, REG_08, C34_RZ_O14_20, REG_39 },
	{ 0x4980000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(       "ffma"), T(5980_0), T(5980_1), ON(50, sat), ON(47, cc), REG_00, REG_08, ON(48, neg), C34_RZ_O14_20, ON(49, neg), REG_39 },
	{ 0x4900000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(       "dset"), ON(52, bf), T(5bb0_0), T(5bb0_1), ON(47, cc), REG_00, ON(43, neg), ON(54, abs), REG_08, ON(53, neg), ON(44, abs), C34_RZ_O14_20, T(pred39)  },
	{ 0x4800000000000000ull, 0xfe00000000000000ull, OP8B, T(pred), N(       "fset"), ON(52, bf), T(5bb0_0), T(5bb0_1), ON(55, ftz), ON(47, cc), REG_00, ON(43, neg), ON(54, abs), REG_08, ON(53, neg), ON(44, abs), C34_RZ_O14_20, T(pred39) },
	{ 0x4000000000000000ull, 0xfe00000000000000ull, OP8B, T(pred), N(       "vset"), T(50f0_0), T(5700_1), T(5700_3), ON(47, cc), REG_00, T(5f00_3), REG_08, T(50f0_1), REG_39 },
	{ 0x3c00000000000000ull, 0xfc00000000000000ull, OP8B, T(pred), N(       "lop3"), N("lut"), ON(47, cc), REG_00, REG_08, S20_20, U08_48 },
	{ 0x3a00000000000000ull, 0xfe00000000000000ull, OP8B, T(pred), N(      "vmnmx"), T(5700_0), T(5700_1), T(5700_3), ON(47, cc), REG_00, T(5f00_3), REG_08, T(50f0_1), REG_39 },
	{ 0x38f8000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "shf"), N("r"), ON(50, w), T(5cf8_1), T(5cf8_0), ON(47, cc), REG_00, REG_08, U06_20, REG_39 },
	{ 0x38f0000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "r2p"), T(5cf0_0), T(5cf0_1), REG_08, S20_20 },
	{ 0x38e8000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "p2r"), T(5cf0_1), REG_00, T(5cf0_0), REG_08, S20_20 },
	{ 0x38e0000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "i2i"), T(5ce0_0), T(5ce0_1), ON(50, sat), ON(47, cc), REG_00, T(5cf0_1), ON(45, neg), ON(49, abs), S20_20 },
	{ 0x38c0000000000000ull, 0xfef0000000000000ull, OP8B, T(pred), N(      "iadd3"), ON(48, x), ON(47, cc), REG_00, ON(51, neg), REG_08, ON(50, neg), S20_20, ON(49, neg), REG_39 },
	{ 0x38b8000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "i2f"), T(5cb8_0), T(5cb8_1), T(5cb8_2), ON(47, cc), REG_00, ON(45, neg), ON(49, abs),  T(5cf0_1), S20_20 },
	{ 0x38b0000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "f2i"), ON(44, ftz), T(5cb0_2), T(5cb0_0), T(5cb0_1), ON(47, cc), REG_00, ON(45, neg), ON(49, abs), F20_20 },
	{ 0x38a8000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "f2f"), ON(44, ftz), T(5cb0_0), T(5cb8_0), T(5ca8_0), ON(50, sat), ON(47, cc), REG_00, ON(45, neg), ON(49, abs), F20_20},
	{ 0x38a0000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "sel"), REG_00, REG_08, S20_20, T(pred39) },
	{ 0x3898000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "mov"), REG_00, S20_20, U04_39 },
	{ 0x3890000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "rro"), T(5c90_0), REG_00, ON(45, neg), ON(49, abs), F20_20 },
	{ 0x3888000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(       "fchk"), T(5c88_0), PRED03, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), F20_20 },
	{ 0x3880000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(       "dmul"), T(5cb8_2), ON(47, cc), REG_00, REG_08, ON(48, neg), D20_20 },
	{ 0x3870000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(       "dadd"), T(5cb8_2), ON(47, cc), REG_00, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), D20_20 },
	{ 0x3868000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(       "fmul"), T(5c68_0), T(5c68_1), T(5cb8_2), ON(50, sat), ON(47, cc), REG_00, REG_08, ON(48, neg), F20_20 },
	{ 0x3860000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(      "fmnmx"), ON(44, ftz), ON(47, cc), REG_00, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), F20_20, T(pred39) },
	{ 0x3858000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(       "fadd"), ON(44, ftz), ON(50, sat), T(5cb8_2), ON(47, cc), REG_00, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), F20_20 },
	{ 0x3850000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(      "dmnmx"), ON(47, cc), REG_00, ON(48, neg), ON(46, abs), REG_08, ON(45, neg), ON(49, abs), D20_20, T(pred39) },
	{ 0x3848000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "shl"), ON(39, w), ON(43, x), ON(47, cc), REG_00, REG_08, S20_20 },
	{ 0x3840000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "lop"), T(5c40_0), ON(43, x), T(5c40_1), PRED48, ON(47, cc), REG_00, ON(39, inv), REG_08, ON(40, inv), S20_20 },
	{ 0x3838000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(       "imul"), T(5c38_0), T(5c38_1), ON(39, hi), ON(47, cc), REG_00, REG_08, S20_20 },
	{ 0x3830000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "flo"), T(5c30_0), ON(41, sh), ON(47, cc), REG_00, ON(40, inv), S20_20 },
	{ 0x3828000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "shr"), T(5c30_0), ON(39, w), ON(44, x), ON(40, brev), ON(47, cc), REG_00, REG_08, S20_20 },
	{ 0x3820000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(      "imnmx"), T(5c30_0), T(5c20_0), ON(47, cc), REG_00, REG_08, S20_20, T(pred39) },
	{ 0x3818000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(     "iscadd"), ON(47, cc), REG_00, ON(49, neg), REG_08, ON(48, neg), S20_20, U05_39 },
	{ 0x3810000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(       "iadd"), ON(50, sat), ON(43, x), ON(47, cc), REG_00, ON(49, neg), REG_08, ON(48, neg), S20_20 },
	{ 0x3808000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(       "popc"), REG_00, ON(40, inv), S20_20 },
	{ 0x3800000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "bfe"), T(5c30_0), ON(40, brev), ON(47, cc), REG_00, REG_08, S20_20 },
	{ 0x36f8000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "shf"), N("l"), ON(50, w), T(5cf8_1), T(5cf8_0), ON(47, cc), REG_00, REG_08, S20_20, REG_39  },
	{ 0x36f0000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "bfi"), ON(47, cc), REG_00, REG_08, S20_20, REG_39 },
	{ 0x36d0000000000000ull, 0xfef8000000000000ull, OP8B, T(pred), N(        "lea"), ON(46, x), PRED48, ON(47, cc), REG_00, ON(45, neg), REG_08, S20_20, U05_39 },
	{ 0x36c0000000000000ull, 0xfef0000000000000ull, OP8B, T(pred), N(       "prmt"), T(5bc0_0), REG_00, REG_08, S20_20, REG_39 },
	{ 0x36b0000000000000ull, 0xfef0000000000000ull, OP8B, T(pred), N(      "fsetp"), T(5bb0_0), ON(47, ftz), T(5bb0_1), PRED03, PRED00, ON(43, neg), ON(7, abs), REG_08, ON(6, neg), ON(44, abs), F20_20, T(pred39) },
	{ 0x36a0000000000000ull, 0xfef0000000000000ull, OP8B, T(pred), N(       "fcmp"), T(5bb0_0), ON(47, ftz), REG_00, REG_08, F20_20, REG_39 },
	{ 0x3680000000000000ull, 0xfef0000000000000ull, OP8B, T(pred), N(      "dsetp"), T(5bb0_0), T(5bb0_1), PRED03, PRED00, ON(43, neg), ON(7, abs), REG_08, ON(6, neg), ON(44, abs), D20_20, T(pred39) },
	{ 0x3670000000000000ull, 0xfef0000000000000ull, OP8B, T(pred), N(       "dfma"), T(5b70_0), ON(47, cc), REG_00, REG_08, ON(48, neg), D20_20, ON(49, neg), REG_39 },
	{ 0x3660000000000000ull, 0xfef0000000000000ull, OP8B, T(pred), N(      "isetp"), T(5b60_0), T(5c30_0), ON(43, x), T(5bb0_1), PRED03, PRED00, REG_08, S20_20, T(pred39) },
	{ 0x3650000000000000ull, 0xfef0000000000000ull, OP8B, T(pred), N(       "iset"), ON(44, bf), T(5b60_0), T(5c30_0), ON(43, x), T(5bb0_1), ON(47, cc), REG_00, REG_08, S20_20, T(pred39) },
	{ 0x3640000000000000ull, 0xfef0000000000000ull, OP8B, T(pred), N(       "icmp"), T(5b60_0), T(5c30_0), REG_00, REG_08, S20_20, REG_39 },
	{ 0x3600000000000000ull, 0xfec0000000000000ull, OP8B, T(pred), N(       "xmad"), T(5b00_0), ON(36, psl), ON(37, mrg), T(5b00_1), ON(38, x), ON(47, cc), REG_00, ON(53, h1), REG_08, U16_20, REG_39  },
	{ 0x3480000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(     "imadsp"), T(5a80_0), T(5a80_1), T(5a80_2),  ON(47, cc), REG_00, REG_08, S20_20, REG_39 },
	{ 0x3400000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(       "imad"), T(5a00_0), T(5a00_1), ON(54, hi), ON(50, sat), ON(49, x), ON(47, c), REG_00, REG_08, S20_20, REG_39 },
	{ 0x3280000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(       "ffma"), T(5980_0), T(5980_1), ON(50, sat), ON(47, cc), REG_00, REG_08, ON(48, neg), F20_20, ON(49, neg), REG_39 },
	{ 0x3200000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(       "dset"), ON(52, bf), T(5bb0_0), T(5bb0_1), ON(47, cc), REG_00, ON(43, neg), ON(54, abs), REG_08, ON(53, neg), ON(44, abs), D20_20, T(pred39)  },
	{ 0x3000000000000000ull, 0xfe00000000000000ull, OP8B, T(pred), N(       "fset"), ON(52, bf), T(5bb0_0), T(5bb0_1), ON(55, ftz), ON(47, cc), REG_00, ON(43, neg), ON(54, abs), REG_08, ON(53, neg), ON(44, abs), F20_20, T(pred39) },
	{ 0x2c00000000000000ull, 0xfe00000000000000ull, OP8B, T(pred), N(  "hadd2_32i"), ON(55, ftz), ON(52, sat), REG_00, ON(56, neg), T(2c00_0), REG_08, U16_36, U16_20, .fmask = F_SM60 },
	{ 0x2a00000000000000ull, 0xfe00000000000000ull, OP8B, T(pred), N(  "hmul2_32i"), T(2a00_0), ON(52, sat), REG_00, T(2c00_0), REG_08, U16_36, U16_20, .fmask = F_SM60 },
	{ 0x2800000000000000ull, 0xfe00000000000000ull, OP8B, T(pred), N(  "hfma2_32i"), T(2a00_0), REG_00, T(2c00_0), REG_08, U16_36, U16_20, ON(52, neg), REG_00, .fmask = F_SM60 },
	{ 0x2000000000000000ull, 0xfc00000000000000ull, OP8B, T(pred), N(       "vadd"), T(5700_0), T(5700_1), T(5700_3), ON(47, cc), REG_00, T(5f00_3), REG_08, T(50f0_1), REG_39 },
	{ 0x1f00000000000000ull, 0xff00000000000000ull, OP8B, T(pred), N(    "imul32i"), T(1f00_0), T(1f00_1), ON(53, hi), ON(52, cc), REG_00, REG_08, S32_20 },
	{ 0x1e00000000000000ull, 0xff00000000000000ull, OP8B, T(pred), N(    "fmul32i"), T(5980_0), ON(55, sat), ON(52, cc), REG_00, REG_08, F32_20 },
	{ 0x1d80000000000000ull, 0xff80000000000000ull, OP8B, T(pred), N(    "iadd32i"), N("po"), ON(54, sat), ON(53, x), ON(52, cc), REG_00, REG_08, S32_20 },
	{ 0x1c00000000000000ull, 0xfe80000000000000ull, OP8B, T(pred), N(    "iadd32i"), ON(54, sat), ON(53, x), ON(52, cc), REG_00, ON(56, neg), REG_08, S32_20 },
	{ 0x1800000000000000ull, 0xfc00000000000000ull, OP8B, T(pred), N(        "lea"), N("hi"), ON(57, x), PRED48, ON(47, cc), REG_00, ON(56, neg), REG_08, C34_RZ_O14_20, REG_39, U05_51 },
	{ 0x1400000000000000ull, 0xfc00000000000000ull, OP8B, T(pred), N(  "iscadd32i"), ON(52, cc), REG_00, REG_08, S32_20, U05_53 },
	{ 0x1000000000000000ull, 0xfc00000000000000ull, OP8B, T(pred), N(    "imad32i"), T(1f00_0), T(1000_0), ON(53, hi), ON(52, cc), REG_00, ON(56, neg), REG_08, S32_20, ON(55, neg), REG_00 /*hmm*/ },
	{ 0x0c00000000000000ull, 0xfc00000000000000ull, OP8B, T(pred), N(    "ffma32i"), T(5980_0), ON(55, sat), ON(52, cc), REG_00, ON(56, neg), REG_08, F32_20, ON(57, neg), REG_00 /*hmm*/ },
	{ 0x0800000000000000ull, 0xfc00000000000000ull, OP8B, T(pred), N(    "fadd32i"), ON(55, ftz), ON(52, cc), REG_00, ON(56, neg), ON(54, abs), REG_08, ON(53, neg), ON(57, abs), F32_20 },
	{ 0x0400000000000000ull, 0xfc00000000000000ull, OP8B, T(pred), N(     "lop32i"), T(0400_0), ON(57, x), ON(52, cc), REG_00, ON(55, inv), REG_08, ON(56, inv), U32_20 },
	{ 0x0200000000000000ull, 0xfe00000000000000ull, OP8B, T(pred), N(       "lop3"), N("lut"), ON(56, x), ON(47, cc), REG_00, REG_08, C34_RZ_O14_20, REG_39, U08_48 },
	{ 0x0100000000000000ull, 0xfff0000000000000ull, OP8B, T(pred), N(     "mov32i"), REG_00, U32_20, U04_12 },
	{ 0, 0, OP8B, OOPS },
};

static struct insn tabsched[] = {
	{ 0x0000000000000000ull, 0x8000000000000000ull, OP8B,          N(      "sched"), T(sched0), T(sched1), T(sched2) },
	{ 0, 0, OP8B, OOPS },
};

static struct insn tabrootas[] = {
	{ 0x0000000000000000ull, 0x8000000000000000ull, OP8B,          N(      "sched"), T(sched0), T(sched1), T(sched2) },
	{ 0, 0, OP8B, T(root) },
};

static void gm107_prep(struct disisa *isa) {
	isa->vardata = vardata_new("sm50isa");
	int f_sm50op = vardata_add_feature(isa->vardata, "sm50op", "SM50 exclusive opcodes");
	int f_sm52op = vardata_add_feature(isa->vardata, "sm52op", "SM52+ opcodes");
	int f_sm60op = vardata_add_feature(isa->vardata, "sm60op", "SM60 exclusive opcodes");
	if (f_sm50op == -1 || f_sm52op == -1 || f_sm60op == -1)
		abort();
	int vs_sm = vardata_add_varset(isa->vardata, "sm", "SM version");
	if (vs_sm == -1)
		abort();
	int v_sm50 = vardata_add_variant(isa->vardata, "sm50", "SM50:SM60", vs_sm);
	int v_sm52 = vardata_add_variant(isa->vardata, "sm52", "SM52+", vs_sm);
	int v_sm60 = vardata_add_variant(isa->vardata, "sm60", "SM60+", vs_sm);
	if (v_sm50 == -1 || v_sm52 == -1 || v_sm60 == -1)
		abort();
	vardata_variant_feature(isa->vardata, v_sm50, f_sm50op);
	vardata_variant_feature(isa->vardata, v_sm52, f_sm50op);
	vardata_variant_feature(isa->vardata, v_sm52, f_sm52op);
	vardata_variant_feature(isa->vardata, v_sm60, f_sm52op);
	vardata_variant_feature(isa->vardata, v_sm60, f_sm60op);
	if (vardata_validate(isa->vardata))
		abort();
}

struct disisa gm107_isa_s = {
	tabroot,
	8,
	4,
	1,
	.prep = gm107_prep,
	.trootas = tabrootas,
	.tsched = tabsched,
	.schedpos = 0x20,
};
