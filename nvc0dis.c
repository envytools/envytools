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


static struct bitfield ctargoff = { { 26, 24 }, BF_SIGNED, .pcrel = 1 };
#define BTARG atombtarg, &ctargoff
#define CTARG atomctarg, &ctargoff

/*
 * Misc number fields
 */

static struct bitfield baroff = { 0x14, 4 };
static struct bitfield pmoff = { 0x1a, 16 };
static struct bitfield tcntoff = { 0x1a, 12 };
static struct bitfield immoff = { { 0x1a, 20 }, BF_SIGNED };
static struct bitfield fimmoff = { { 0x1a, 20 }, BF_UNSIGNED, 12 };
static struct bitfield dimmoff = { { 0x1a, 20 }, BF_UNSIGNED, 44 };
static struct bitfield limmoff = { 0x1a, 32 };
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
static struct bitfield pdst2_bf = { 0x36, 3 };
static struct bitfield pdst3_bf = { 0x35, 3 }; // ...the hell?
static struct bitfield pdst4_bf = { 0x32, 3 }; // yay.
static struct bitfield tex_bf = { 0x20, 7 };
static struct bitfield samp_bf = { 0x28, 4 };
static struct bitfield surf_bf = { 0x1a, 3 };
static struct bitfield sreg_bf = { 0x1a, 7 };

static struct reg dst_r = { &dst_bf, "r" };
static struct reg dstd_r = { &dst_bf, "r", "d" };
static struct reg dstq_r = { &dst_bf, "r", "q" };
static struct reg src1_r = { &src1_bf, "r" };
static struct reg src1d_r = { &src1_bf, "r", "d" };
static struct reg src2_r = { &src2_bf, "r" };
static struct reg src2d_r = { &src2_bf, "r", "d" };
static struct reg src3_r = { &src3_bf, "r" };
static struct reg src3d_r = { &src3_bf, "r", "d" };
static struct reg dst2_r = { &dst2_bf, "r" };
static struct reg dst2d_r = { &dst2_bf, "r", "d" };
static struct reg psrc1_r = { &psrc1_bf, "p" };
static struct reg psrc2_r = { &psrc2_bf, "p" };
static struct reg psrc3_r = { &psrc3_bf, "p" };
static struct reg pred_r = { &pred_bf, "p" };
static struct reg pdst_r = { &pdst_bf, "p" };
static struct reg pdst2_r = { &pdst2_bf, "p" };
static struct reg pdst3_r = { &pdst3_bf, "p" };
static struct reg pdst4_r = { &pdst4_bf, "p" };
static struct reg tex_r = { &tex_bf, "t" };
static struct reg samp_r = { &samp_bf, "s" };
static struct reg surf_r = { &surf_bf, "g" };
static struct reg cc_r = { 0, "c" };
static struct reg sreg_r = { &sreg_bf, "sr", .specials = sreg_sr, .always_special = 1 };

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
#define PDST2 atomreg, &pdst2_r
#define PDST3 atomreg, &pdst3_r
#define PDST4 atomreg, &pdst4_r
#define TEX atomreg, &tex_r
#define SAMP atomreg, &samp_r
#define SURF atomreg, &surf_r
#define CC atomreg, &cc_r
#define SREG atomreg, &sreg_r

#define TDST atomtdst, 0
static void atomtdst APROTO {
	if (!ctx->out)
		return;
	int base = BF(0xe, 6);
	int mask = BF(0x2e, 4);
	int k = 0, i;
	fprintf (ctx->out, " %s{", cnorm);
	for (i = 0; i < 4; i++)
		if (mask & 1<<i)
			fprintf (ctx->out, " %s$r%d", cbl, base+k++);
		else
			fprintf (ctx->out, " %s_", cbl);
	fprintf (ctx->out, " %s}", cnorm);
}
#define TSRC atomtsrc, 0
static void atomtsrc APROTO {
	if (!ctx->out)
		return;
	int base = BF(0x14, 6);
	int cnt = BF(0x34, 2);
	int i;
	fprintf (ctx->out, " %s{", cnorm);
	for (i = 0; i <= cnt; i++)
		fprintf (ctx->out, " %s$r%d", cbl, base+i);
	fprintf (ctx->out, " %s}", cnorm);
}

#define SADDR atomsaddr, 0
static void atomsaddr APROTO {
	if (!ctx->out)
		return;
	int base = BF(0x14, 6);
	int cnt = BF(0x2c, 2);
	int i;
	fprintf (ctx->out, " %s{", cnorm);
	for (i = 0; i <= cnt; i++)
		fprintf (ctx->out, " %s$r%d", cbl, base+i);
	fprintf (ctx->out, " %s}", cnorm);
}

#define ESRC atomesrc, 0
static void atomesrc APROTO {
	if (!ctx->out)
		return;
	int base = BF(26, 6);
	int cnt = BF(5, 2);
	int i;
	fprintf (ctx->out, " %s{", cnorm);
	for (i = 0; i <= cnt; i++)
		fprintf (ctx->out, " %s$r%d", cbl, base+i);
	fprintf (ctx->out, " %s}", cnorm);
}

#define VDST atomvdst, 0
static void atomvdst APROTO {
	if (!ctx->out)
		return;
	int base = BF(14, 6);
	int cnt = BF(5, 2);
	int i;
	fprintf (ctx->out, " %s{", cnorm);
	for (i = 0; i <= cnt; i++)
		fprintf (ctx->out, " %s$r%d", cbl, base+i);
	fprintf (ctx->out, " %s}", cnorm);
}

// vertex base address (for tessellation and geometry programs)
#define VBASRC atomvbasrc, 0
static void atomvbasrc APROTO {
	if (!ctx->out)
		return;
	int s1 = BF(20, 6);
	int s2 = BF(26, 6);
	fprintf (ctx->out, " %sp<%s$r%d%s+%s%i%s>", ccy, cbl, s1, ccy, cyel, s2, ccy);
}

/*
 * Memory fields
 */

static int gmem[] = { 'g', 0, 32 };
static int gdmem[] = { 'g', 1, 32 };
static int smem[] = { 's', 0, 24 };
static int lmem[] = { 'l', 0, 24 };
static int fcmem[] = { 'c', 0, 16 };
#define GLOBAL atommem, gmem
#define GLOBALD atommem, gdmem
#define SHARED atommem, smem
#define LOCAL atommem, lmem
#define FCONST atommem, fcmem
static void atommem APROTO {
	if (!ctx->out)
		return;
	const int *n = v;
	uint32_t delta = BF(26, n[2]);
	fprintf (ctx->out, " %s%c", ccy, n[0]);
	if (n[0] == 'c')
		fprintf (ctx->out, "%lld", BF(0x2a, 4));
	fprintf (ctx->out, "[%s$r%lld%s", cbl, BF(20, 6), n[1]?"d":"");
	if (delta & 1ull<<(n[2]-1))
		fprintf (ctx->out, "%s-%s%#x%s]", ccy, cyel, (1u<<n[2])-delta, ccy);
	else
		fprintf (ctx->out, "%s+%s%#x%s]", ccy, cyel, delta, ccy);
}

#define VAR atomvar, 0
static void atomvar APROTO {
	if (!ctx->out)
		return;
	int s1 = BF(20, 6);
	uint32_t delta =  BF(32, 16);
	fprintf (ctx->out, " %sv[%s$r%d%s+%s%#x%s]", ccy, cbl, s1, ccy, cyel, delta, ccy);
}

#define ATTR atomattr, 0
static void atomattr APROTO {
	if (!ctx->out)
		return;
	int s1 = BF(20, 6);
	int s2 = BF(26, 6);
	uint32_t delta = BF(32, 16);
	if (s2 < 63)
		s1 = s2;
	fprintf (ctx->out, " %sa[%s$r%d%s+%s%#x%s]", ccy, cbl, s1, ccy, cyel, delta, ccy);
}

#define CONST atomconst, 0
static void atomconst APROTO {
	if (!ctx->out)
		return;
	int delta = BF(0x1a, 16);
	int space = BF(0x2a, 4);
	fprintf (ctx->out, " %sc%d[%s%#x%s]", ccy, space, cyel, delta, ccy);
}

#define GATOM atomgatom, 0
static void atomgatom APROTO {
	if (!ctx->out)
		return;
	const int *n = v;
	uint32_t delta = BF(0x1a, 17) | BF(0x37, 3)<<17;
	fprintf (ctx->out, " %sg[%s$r%lld%s", ccy, cbl, BF(20, 6), BF(0x3a, 1)?"d":"");
	if (delta & 0x80000)
		fprintf (ctx->out, "%s-%s%#x%s]", ccy, cyel, 0x100000-delta, ccy);
	else
		fprintf (ctx->out, "%s+%s%#x%s]", ccy, cyel, delta, ccy);
}

/*
 * The instructions
 */

F(gmem, 0x3a, GLOBAL, GLOBALD)

static struct insn tabldstt[] = {
	{ AP, 0x00, 0xe0, N("u8") },
	{ AP, 0x20, 0xe0, N("s8") },
	{ AP, 0x40, 0xe0, N("u16") },
	{ AP, 0x60, 0xe0, N("s16") },
	{ AP, 0x80, 0xe0, N("b32") },
	{ AP, 0xa0, 0xe0, N("b64") },
	{ AP, 0xc0, 0xe0, N("b128") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabldstd[] = {
	{ AP, 0x00, 0xe0, DST },
	{ AP, 0x20, 0xe0, DST },
	{ AP, 0x40, 0xe0, DST },
	{ AP, 0x60, 0xe0, DST },
	{ AP, 0x80, 0xe0, DST },
	{ AP, 0xa0, 0xe0, DSTD },
	{ AP, 0xc0, 0xe0, DSTQ },
	{ AP, 0, 0, OOPS, DST },
};

static struct insn tabldvf[] = {
	{ AP, 0x60, 0xe0, N("b128") },
	{ AP, 0x40, 0xe0, N("b96") },
	{ AP, 0x20, 0xe0, N("b64") },
	{ AP, 0x00, 0xe0, N("b32") },
};

static struct insn tabfarm[] = {
	{ AP, 0x0000000000000000ull, 0x0180000000000000ull, N("rn") },
	{ AP, 0x0080000000000000ull, 0x0180000000000000ull, N("rm") },
	{ AP, 0x0100000000000000ull, 0x0180000000000000ull, N("rp") },
	{ AP, 0x0180000000000000ull, 0x0180000000000000ull, N("rz") },
};

static struct insn tabfcrm[] = {
	{ AP, 0x0000000000000000ull, 0x0006000000000000ull, N("rn") },
	{ AP, 0x0002000000000000ull, 0x0006000000000000ull, N("rm") },
	{ AP, 0x0004000000000000ull, 0x0006000000000000ull, N("rp") },
	{ AP, 0x0006000000000000ull, 0x0006000000000000ull, N("rz") },
};

static struct insn tabsetit[] = {
	{ AP, 0x0080000000000000ull, 0x0780000000000000ull, N("lt") },
	{ AP, 0x0100000000000000ull, 0x0780000000000000ull, N("eq") },
	{ AP, 0x0180000000000000ull, 0x0780000000000000ull, N("le") },
	{ AP, 0x0200000000000000ull, 0x0780000000000000ull, N("gt") },
	{ AP, 0x0280000000000000ull, 0x0780000000000000ull, N("ne") },
	{ AP, 0x0300000000000000ull, 0x0780000000000000ull, N("ge") },
	{ AP, 0x0380000000000000ull, 0x0780000000000000ull, N("num") },
	{ AP, 0x0400000000000000ull, 0x0780000000000000ull, N("nan") },
	{ AP, 0x0480000000000000ull, 0x0780000000000000ull, N("ltu") },
	{ AP, 0x0500000000000000ull, 0x0780000000000000ull, N("equ") },
	{ AP, 0x0580000000000000ull, 0x0780000000000000ull, N("leu") },
	{ AP, 0x0600000000000000ull, 0x0780000000000000ull, N("gtu") },
	{ AP, 0x0680000000000000ull, 0x0780000000000000ull, N("neu") },
	{ AP, 0x0700000000000000ull, 0x0780000000000000ull, N("geu") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabis2[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, SRC2 },
	{ AP, 0x0000400000000000ull, 0x0000c00000000000ull, CONST },
	{ AP, 0x0000800000000000ull, 0x0000c00000000000ull, SRC3 },
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, IMM },
	{ AP, 0, 0, OOPS },
};

static struct insn tabis3[] = {
	{ AP, 0x0000800000000000ull, 0x0000c00000000000ull, CONST },
	{ AP, 0, 0, SRC3 },
};

static struct insn tabcs2[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, SRC2 },
	{ AP, 0x0000400000000000ull, 0x0000c00000000000ull, CONST },
	{ AP, 0, 0, OOPS },
};

static struct insn tabfs2[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, SRC2 },
	{ AP, 0x0000400000000000ull, 0x0000c00000000000ull, CONST },
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, FIMM },
	{ AP, 0, 0, OOPS },
};

static struct insn tabds2[] = {
	{ AP, 0x0000000000000000ull, 0x0000c00000000000ull, SRC2D },
	{ AP, 0x0000c00000000000ull, 0x0000c00000000000ull, DIMM },
	{ AP, 0, 0, OOPS },
};

F1(ias, 5, N("sat"))
F1(vas, 9, N("sat"))
F1(fas, 0x31, N("sat"))
F1(faf, 5, N("ftz"))
F1(fmf, 6, N("ftz"))
F1(fmneg, 0x39, N("neg"))
F1(neg1, 9, N("neg"))
F1(neg2, 8, N("neg"))
F1(abs1, 7, N("abs"))
F1(abs2, 6, N("abs"))
F1(rint, 7, N("rint"))
F1(rev, 8, N("rev"))

F1(not1, 9, N("not"))
F1(not2, 8, N("not"))

F1(shiftamt, 6, N("shiftamt"))

F1(acout, 0x30, CC)
F1(acout2, 0x3a, CC)
F1(acin, 6, CC)
F1(acin2, 0x37, CC)

F(us32, 5, N("u32"), N("s32"))
F1(high, 6, N("high"))
F(us32v, 6, N("u32"), N("s32"))
F(us32d, 0x2a, N("u32"), N("s32"))

F1(pnot1, 0x17, N("not"))
F1(pnot2, 0x1d, N("not"))
F1(pnot3, 0x34, N("not"))

F1(dtex, 0x2d, N("deriv"))
F(ltex, 9, N("all"), N("live"))

static struct insn tabtexf[] = {
	{ AP, 0, 0, T(ltex), T(dtex) },
};

static struct insn tablane[] = {
	{ AP, 0x0000000000000000ull, 0x00000000000001e0ull, N("lnone") },
	{ AP, 0x0000000000000020ull, 0x00000000000001e0ull, N("l0") },
	{ AP, 0x0000000000000040ull, 0x00000000000001e0ull, N("l1") },
	{ AP, 0x0000000000000060ull, 0x00000000000001e0ull, N("l01") },
	{ AP, 0x0000000000000080ull, 0x00000000000001e0ull, N("l2") },
	{ AP, 0x00000000000000a0ull, 0x00000000000001e0ull, N("l02") },
	{ AP, 0x00000000000000c0ull, 0x00000000000001e0ull, N("l12") },
	{ AP, 0x00000000000000e0ull, 0x00000000000001e0ull, N("l012") },
	{ AP, 0x0000000000000100ull, 0x00000000000001e0ull, N("l3") },
	{ AP, 0x0000000000000120ull, 0x00000000000001e0ull, N("l03") },
	{ AP, 0x0000000000000140ull, 0x00000000000001e0ull, N("l13") },
	{ AP, 0x0000000000000160ull, 0x00000000000001e0ull, N("l013") },
	{ AP, 0x0000000000000180ull, 0x00000000000001e0ull, N("l23") },
	{ AP, 0x00000000000001a0ull, 0x00000000000001e0ull, N("l023") },
	{ AP, 0x00000000000001c0ull, 0x00000000000001e0ull, N("l123") },
	{ AP, 0x00000000000001e0ull, 0x00000000000001e0ull },
};

// for quadop
static struct insn tabqs1[] = {
	{ AP, 0x0000000000000000ull, 0x00000000000001c0ull, N("l0") },
	{ AP, 0x0000000000000040ull, 0x00000000000001c0ull, N("l1") },
	{ AP, 0x0000000000000080ull, 0x00000000000001c0ull, N("l2") },
	{ AP, 0x00000000000000c0ull, 0x00000000000001c0ull, N("l3") },
	{ AP, 0x0000000000000100ull, 0x00000000000001c0ull, N("dx") },
	{ AP, 0x0000000000000140ull, 0x00000000000001c0ull, N("dy") },
	{ AP, 0, 0 },
};

static struct insn tabqop0[] = {
	{ AP, 0x0000000000000000ull, 0x000000c000000000ull, N("add") },
	{ AP, 0x0000004000000000ull, 0x000000c000000000ull, N("subr") },
	{ AP, 0x0000008000000000ull, 0x000000c000000000ull, N("sub") },
	{ AP, 0x000000c000000000ull, 0x000000c000000000ull, N("mov2") },
};

static struct insn tabqop1[] = {
	{ AP, 0x0000000000000000ull, 0x0000003000000000ull, N("add") },
	{ AP, 0x0000001000000000ull, 0x0000003000000000ull, N("subr") },
	{ AP, 0x0000002000000000ull, 0x0000003000000000ull, N("sub") },
	{ AP, 0x0000003000000000ull, 0x0000003000000000ull, N("mov2") },
};

static struct insn tabqop2[] = {
	{ AP, 0x0000000000000000ull, 0x0000000c00000000ull, N("add") },
	{ AP, 0x0000000400000000ull, 0x0000000c00000000ull, N("subr") },
	{ AP, 0x0000000800000000ull, 0x0000000c00000000ull, N("sub") },
	{ AP, 0x0000000c00000000ull, 0x0000000c00000000ull, N("mov2") },
};

static struct insn tabqop3[] = {
	{ AP, 0x0000000000000000ull, 0x0000000300000000ull, N("add") },
	{ AP, 0x0000000100000000ull, 0x0000000300000000ull, N("subr") },
	{ AP, 0x0000000200000000ull, 0x0000000300000000ull, N("sub") },
	{ AP, 0x0000000300000000ull, 0x0000000300000000ull, N("mov2") },
};

static struct insn tabsetlop[] = {
	{ AP, 0x000e000000000000ull, 0x006e000000000000ull },	// noop, really "and $p7"
	{ AP, 0x0000000000000000ull, 0x0060000000000000ull, N("and"), T(pnot3), PSRC3 },
	{ AP, 0x0020000000000000ull, 0x0060000000000000ull, N("or"), T(pnot3), PSRC3 },
	{ AP, 0x0040000000000000ull, 0x0060000000000000ull, N("xor"), T(pnot3), PSRC3 },
	{ AP, 0, 0, OOPS, T(pnot3), PSRC3 },
};

// TODO: this definitely needs a second pass to see which combinations really work.
static struct insn tabcvtfdst[] = {
	{ AP, 0x0000000000100000ull, 0x0000000000700000ull, T(ias), N("f16"), DST },
	{ AP, 0x0000000000200000ull, 0x0000000000700000ull, T(ias), N("f32"), DST },
	{ AP, 0x0000000000300000ull, 0x0000000000700000ull, N("f64"), DSTD },
	{ AP, 0, 0, OOPS, DST },
};

static struct insn tabcvtidst[] = {
	{ AP, 0x0000000000000000ull, 0x0000000000700080ull, N("u8"), DST },
	{ AP, 0x0000000000000080ull, 0x0000000000700080ull, N("s8"), DST },
	{ AP, 0x0000000000100000ull, 0x0000000000700080ull, N("u16"), DST },
	{ AP, 0x0000000000100080ull, 0x0000000000700080ull, N("s16"), DST },
	{ AP, 0x0000000000200000ull, 0x0000000000700080ull, N("u32"), DST },
	{ AP, 0x0000000000200080ull, 0x0000000000700080ull, N("s32"), DST },
	{ AP, 0x0000000000300000ull, 0x0000000000700080ull, N("u64"), DSTD },
	{ AP, 0x0000000000300080ull, 0x0000000000700080ull, N("s64"), DSTD },
	{ AP, 0, 0, OOPS, DST },
};

static struct insn tabcvtfsrc[] = {
	{ AP, 0x0000000000800000ull, 0x0000000003800000ull, T(neg2), T(abs2), N("f16"), T(cs2) },
	{ AP, 0x0000000001000000ull, 0x0000000003800000ull, T(neg2), T(abs2), N("f32"), T(cs2) },
	{ AP, 0x0000000001800000ull, 0x0000000003800000ull, T(neg2), T(abs2), N("f64"), SRC2D },
	{ AP, 0, 0, OOPS, T(neg2), T(abs2), SRC2 },
};

static struct insn tabcvtisrc[] = {
	{ AP, 0x0000000000000000ull, 0x0000000003800200ull, T(neg2), T(abs2), N("u8"), BNUM, T(cs2) },
	{ AP, 0x0000000000000200ull, 0x0000000003800200ull, T(neg2), T(abs2), N("s8"), BNUM, T(cs2) },
	{ AP, 0x0000000000800000ull, 0x0000000003800200ull, T(neg2), T(abs2), N("u16"), HNUM, T(cs2) },
	{ AP, 0x0000000000800200ull, 0x0000000003800200ull, T(neg2), T(abs2), N("s16"), HNUM, T(cs2) },
	{ AP, 0x0000000001000000ull, 0x0000000003800200ull, T(neg2), T(abs2), N("u32"), T(cs2) },
	{ AP, 0x0000000001000200ull, 0x0000000003800200ull, T(neg2), T(abs2), N("s32"), T(cs2) },
	{ AP, 0x0000000001800000ull, 0x0000000003800200ull, T(neg2), T(abs2), N("u64"), SRC2D },
	{ AP, 0x0000000001800200ull, 0x0000000003800200ull, T(neg2), T(abs2), N("s64"), SRC2D },
	{ AP, 0, 0, OOPS, T(neg2), T(abs2), SRC2 },
};

static struct insn tabaddop[] = {
	{ AP, 0x0000000000000000ull, 0x0000000000000300ull, N("add") },
	{ AP, 0x0000000000000100ull, 0x0000000000000300ull, N("sub") },
	{ AP, 0x0000000000000200ull, 0x0000000000000300ull, N("subr") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabaddop2[] = {
	{ AP, 0x0000000000000000ull, 0x0180000000000000ull, N("add") },
	{ AP, 0x0080000000000000ull, 0x0180000000000000ull, N("sub") },
	{ AP, 0x0100000000000000ull, 0x0180000000000000ull, N("subr") },
	{ AP, 0, 0, OOPS },
};

F(bar, 0x2f, SRC1, BAR)
F(tcnt, 0x2e, SRC2, TCNT)

static struct insn tabprmtmod[] = {
	{ AP, 0x00, 0xe0 },
	{ AP, 0x20, 0xe0, N("f4e") },
	{ AP, 0x40, 0xe0, N("b4e") },
	{ AP, 0x60, 0xe0, N("rc8") },
	{ AP, 0x80, 0xe0, N("ecl") },
	{ AP, 0xa0, 0xe0, N("ecr") },
	{ AP, 0xc0, 0xe0, N("rc16") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabminmax[] = {
	{ AP, 0x000e000000000000ull, 0x001e000000000000ull, N("min") }, // looks like min/max is selected by a normal predicate. fun. needs to be checked
	{ AP, 0x001e000000000000ull, 0x001e000000000000ull, N("max") },
	{ AP, 0, 0, OOPS },
};

// XXX: orthogonalise it. if possible.
static struct insn tabredop[] = {
	{ AP, 0x00, 0xe0, N("add") },
	{ AP, 0x20, 0xe0, N("min") },
	{ AP, 0x40, 0xe0, N("max") },
	{ AP, 0x60, 0xe0, N("inc") },
	{ AP, 0x80, 0xe0, N("dec") },
	{ AP, 0xa0, 0xe0, N("and") },
	{ AP, 0xc0, 0xe0, N("or") },
	{ AP, 0xe0, 0xe0, N("xor") },
};

static struct insn tabredops[] = {
	{ AP, 0x00, 0xe0, N("add") },
	{ AP, 0x20, 0xe0, N("min") },
	{ AP, 0x40, 0xe0, N("max") },
	{ AP, 0, 0, OOPS },
};

static struct insn tablcop[] = {
	{ AP, 0x000, 0x300, N("ca") },
	{ AP, 0x100, 0x300, N("cg") },
	{ AP, 0x200, 0x300, N("cs") },
	{ AP, 0x300, 0x300, N("cv") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabscop[] = {
	{ AP, 0x000, 0x300, N("wb") },
	{ AP, 0x100, 0x300, N("cg") },
	{ AP, 0x200, 0x300, N("cs") },
	{ AP, 0x300, 0x300, N("wt") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabsclamp[] = {
	{ AP, 0x0000000000000000ull, 0x0001800000000000ull, N("zero") },
	{ AP, 0x0000800000000000ull, 0x0001800000000000ull, N("clamp") },
	{ AP, 0x0001000000000000ull, 0x0001800000000000ull, N("trap") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabvdst[] = {
	{ AP, 0x0000000000000000ull, 0x0380000000000000ull, N("h1") },
	{ AP, 0x0080000000000000ull, 0x0380000000000000ull, N("h0") },
	{ AP, 0x0100000000000000ull, 0x0380000000000000ull, N("b0") },
	{ AP, 0x0180000000000000ull, 0x0380000000000000ull, N("b2") },
	{ AP, 0x0200000000000000ull, 0x0380000000000000ull, N("add") },
	{ AP, 0x0280000000000000ull, 0x0380000000000000ull, N("min") },
	{ AP, 0x0300000000000000ull, 0x0380000000000000ull, N("max") },
	{ AP, 0x0380000000000000ull, 0x0380000000000000ull },
	{ AP, 0, 0, OOPS },
};

static struct insn tabvsrc1[] = {
	{ AP, 0x0000000000000000ull, 0x0000700000000000ull, N("b0") },
	{ AP, 0x0000100000000000ull, 0x0000700000000000ull, N("b1") },
	{ AP, 0x0000200000000000ull, 0x0000700000000000ull, N("b2") },
	{ AP, 0x0000300000000000ull, 0x0000700000000000ull, N("b3") },
	{ AP, 0x0000400000000000ull, 0x0000700000000000ull, N("h0") },
	{ AP, 0x0000500000000000ull, 0x0000700000000000ull, N("h1") },
	{ AP, 0x0000600000000000ull, 0x0000700000000000ull },
	{ AP, 0, 0, OOPS },
};

static struct insn tabvsrc2[] = {
	{ AP, 0x0000000000000000ull, 0x0000000700000000ull, N("b0") },
	{ AP, 0x0000000100000000ull, 0x0000000700000000ull, N("b1") },
	{ AP, 0x0000000200000000ull, 0x0000000700000000ull, N("b2") },
	{ AP, 0x0000000300000000ull, 0x0000000700000000ull, N("b3") },
	{ AP, 0x0000000400000000ull, 0x0000000700000000ull, N("h0") },
	{ AP, 0x0000000500000000ull, 0x0000000700000000ull, N("h1") },
	{ AP, 0x0000000600000000ull, 0x0000000700000000ull },
	{ AP, 0, 0, OOPS },
};

F(vsclamp, 0x7, N("clamp"), N("wrap"))

static struct insn tabvmop[] = {
	{ AP, 0x000, 0x180, N("add") },
	{ AP, 0x080, 0x180, N("sub") },
	{ AP, 0x100, 0x180, N("subr") },
	{ AP, 0x180, 0x180, N("addpo") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabvmshr[] = {
	{ AP, 0x0000000000000000ull, 0x0380000000000000ull, },
	{ AP, 0x0080000000000000ull, 0x0380000000000000ull, N("shr7") },
	{ AP, 0x0100000000000000ull, 0x0380000000000000ull, N("shr15") },
	{ AP, 0, 0, OOPS },
};

static struct insn tabvsetop[] = {
	{ AP, 0x080, 0x380, N("lt") },
	{ AP, 0x100, 0x380, N("eq") },
	{ AP, 0x180, 0x380, N("le") },
	{ AP, 0x200, 0x380, N("gt") },
	{ AP, 0x280, 0x380, N("ne") },
	{ AP, 0x300, 0x380, N("ge") },
	{ AP, 0, 0, OOPS },
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
	{ AP, 0x0800000000000000ull, 0xf800000000000007ull, T(minmax), N("f32"), DST, T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2) },
	// 10?
	{ AP, 0x1800000000000000ull, 0xf800000000000007ull, N("set"), DST, T(setit), N("f32"), T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2), T(setlop) },
	{ AP, 0x200000000001c000ull, 0xf80000000001c007ull, N("set"), PDST, T(setit), N("f32"), T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2), T(setlop) }, // and these unknown bits are what? another predicate?
	// 28?
	{ AP, 0x3000000000000000ull, 0xf800000000000007ull, N("add"), T(fmf), T(ias), T(farm), N("f32"), DST, T(neg1), N("mul"), SRC1, T(fs2), T(neg2), SRC3 },
	{ AP, 0x3800000000000000ull, 0xf800000000000007ull, N("slct"), N("b32"), DST, SRC1, T(fs2), T(setit), N("f32"), SRC3 },
	// 40?
	{ AP, 0x4800000000000000ull, 0xf800000000000007ull, N("quadop"), N("f32"), T(qop0), T(qop1), T(qop2), T(qop3), DST, T(qs1), SRC1, T(fs2) },
	{ AP, 0x5000000000000000ull, 0xf800000000000007ull, N("add"), T(faf), T(fas), T(farm), N("f32"), DST, T(neg1), T(abs1), SRC1, T(neg2), T(abs2), T(fs2) },
	{ AP, 0x5800000000000000ull, 0xf800000000000007ull, N("mul"), T(fmf), T(ias), T(farm), T(fmneg), N("f32"), DST, SRC1, T(fs2) },
	{ AP, 0x6000000000000000ull, 0xf800000000000027ull, N("presin"), N("f32"), DST, T(fs2) },
	{ AP, 0x6000000000000020ull, 0xf800000000000027ull, N("preex2"), N("f32"), DST, T(fs2) },
	// 68-b8?
	{ AP, 0xc07e0000fc000000ull, 0xf87e0000fc0001c7ull, N("interp"), N("f32"), DST, VAR },
	{ AP, 0xc07e000000000040ull, 0xf87e0000000001c7ull, N("interp"), N("f32"), DST, SRC2, VAR },
	{ AP, 0xc07e0000fc000080ull, 0xf87e0000fc0001c7ull, N("interp"), N("f32"), DST, N("flat"), VAR },
	{ AP, 0xc07e0000fc000100ull, 0xf87e0000fc0001c7ull, N("interp"), N("f32"), DST, N("cent"), VAR },
	{ AP, 0xc07e000000000140ull, 0xf87e0000000001c7ull, N("interp"), N("f32"), DST, N("cent"), SRC2, VAR },
	{ AP, 0xc800000000000000ull, 0xf80000001c000007ull, N("cos"), N("f32"), DST, SRC1 },
	{ AP, 0xc800000004000000ull, 0xf80000001c000007ull, N("sin"), N("f32"), DST, SRC1 },
	{ AP, 0xc800000008000000ull, 0xf80000001c000007ull, N("ex2"), N("f32"), DST, SRC1 },
	{ AP, 0xc80000000c000000ull, 0xf80000001c000007ull, N("lg2"), N("f32"), DST, SRC1 },
	{ AP, 0xc800000010000000ull, 0xf80000001c000007ull, N("rcp"), N("f32"), DST, SRC1 },
	{ AP, 0xc800000014000000ull, 0xf80000001c000007ull, N("rsqrt"), N("f32"), DST, SRC1 },
	{ AP, 0x0000000000000000ull, 0x0000000000000007ull, OOPS, T(farm), N("f32"), DST, SRC1, T(fs2), SRC3 },


	{ AP, 0x0800000000000001ull, 0xf800000000000007ull, T(minmax), N("f64"), DSTD, T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2) },
	{ AP, 0x1000000000000001ull, 0xf800000000000007ull, N("set"), DST, T(setit), N("f64"), T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2), T(setlop) },
	{ AP, 0x180000000001c001ull, 0xf80000000001c007ull, N("set"), PDST, T(setit), N("f64"), T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2), T(setlop) },
	{ AP, 0x2000000000000001ull, 0xf800000000000007ull, N("fma"), T(farm), N("f64"), DSTD, T(neg1), SRC1D, T(ds2), T(neg2), SRC3D },
	// 28?
	// 30?
	// 38?
	// 40?
	{ AP, 0x4800000000000001ull, 0xf800000000000007ull, N("add"), T(farm), N("f64"), DSTD, T(neg1), T(abs1), SRC1D, T(neg2), T(abs2), T(ds2) },
	{ AP, 0x5000000000000001ull, 0xf800000000000007ull, N("mul"), T(farm), T(neg1), N("f64"), DSTD, SRC1D, T(ds2) },
	{ AP, 0x0000000000000001ull, 0x0000000000000007ull, OOPS, T(farm), N("f64"), DSTD, SRC1D, T(ds2), SRC3D },


	{ AP, 0x0800000000000002ull, 0xf800000000000007ull, T(addop), T(ias), N("b32"), T(acout2), DST, SRC1, LIMM, T(acin) },
	{ AP, 0x1000000000000002ull, 0xf8000000000000a7ull, N("mul"), N("u32"), T(acout2), DST, SRC1, LIMM },
	{ AP, 0x10000000000000a2ull, 0xf8000000000000a7ull, N("mul"), N("s32"), DST, SRC1, LIMM },
	{ AP, 0x18000000000001e2ull, 0xf8000000000001e7ull, N("mov"), N("b32"), DST, LIMM }, // wanna bet these unknown bits are tesla-like lanemask?
	// 20?
	{ AP, 0x2800000000000002ull, 0xf800000000000007ull, N("add"), T(faf), N("f32"), DST, T(neg1), T(abs1), SRC1, LIMM },
	{ AP, 0x3000000000000002ull, 0xf800000000000007ull, N("mul"), T(fmf), T(ias), N("f32"), DST, SRC1, LIMM },
	{ AP, 0x3800000000000002ull, 0xf8000000000000c7ull, N("and"), N("b32"), DST, SRC1, LIMM },
	{ AP, 0x3800000000000042ull, 0xf8000000000000c7ull, N("or"), N("b32"), DST, SRC1, LIMM },
	{ AP, 0x3800000000000082ull, 0xf8000000000000c7ull, N("xor"), N("b32"), DST, SRC1, LIMM },
	{ AP, 0x0000000000000002ull, 0x0000000000000007ull, OOPS, N("b32"), DST, SRC1, LIMM },


	{ AP, 0x0800000000000003ull, 0xf8010000000000c7ull, T(minmax), T(us32), DST, SRC1, T(is2) },
	{ AP, 0x0800000000000043ull, 0xf8010000000000c7ull, T(minmax), T(us32), DST, SRC1, T(is2), CC }, // NFI what these bits mean, exactly.
	{ AP, 0x08010000000000c3ull, 0xf8010000000000c7ull, T(minmax), T(us32), CC, DST, SRC1, T(is2) },
	{ AP, 0x1000000000000003ull, 0xf800000000000007ull, N("set"), DST, T(setit), T(us32), SRC1, T(is2), T(acin), T(setlop) },
	{ AP, 0x180000000001c003ull, 0xf80000000001c007ull, N("set"), PDST, T(setit), T(us32), SRC1, T(is2), T(acin), T(setlop) },
	{ AP, 0x2000000000000003ull, 0xf8000000000000a7ull, T(addop), T(acout), DST, N("mul"), T(high), N("u32"), SRC1, T(is2), T(is3), T(acin2) }, // bet you these bits are independent s/u for each source, like on tesla?
	{ AP, 0x20000000000000a3ull, 0xf8000000000000a7ull, T(addop), T(acout), DST, N("mul"), T(high), N("s32"), SRC1, T(is2), T(is3), T(acin2) },
	{ AP, 0x2800000000000003ull, 0xf800000000000007ull, N("ins"), N("b32"), DST, SRC1, T(is2), T(is3) },
	{ AP, 0x3000000000000003ull, 0xf800000000000007ull, N("slct"), N("b32"), DST, SRC1, T(is2), T(setit), T(us32), T(is3) },
	{ AP, 0x3800000000000003ull, 0xf800000000000007ull, N("sad"), T(us32), DST, SRC1, T(is2), T(is3) },
	{ AP, 0x4000000000000003ull, 0xf800000000000007ull, T(addop2), N("b32"), DST, N("shl"), SRC1, SHCNT, T(is2) },
	{ AP, 0x4800000000000003ull, 0xf800000000000007ull, T(addop), T(ias), N("b32"), T(acout), DST, SRC1, T(is2), T(acin) },
	{ AP, 0x5000000000000003ull, 0xf8000000000000a7ull, N("mul"), T(high), N("u32"), T(acout), DST, SRC1, T(is2) },	// looks like acout, but... wouldn't it always be 0? hm.
	{ AP, 0x50000000000000a3ull, 0xf8000000000000a7ull, N("mul"), T(high), N("s32"), T(acout), DST, SRC1, T(is2) },
	{ AP, 0x5800000000000003ull, 0xf800000000000007ull, N("shr"), T(us32), DST, SRC1, T(is2) },
	{ AP, 0x6000000000000003ull, 0xf800000000000007ull, N("shl"), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x6800000000000003ull, 0xf8000000000000c7ull, N("and"), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x6800000000000043ull, 0xf8000000000000c7ull, N("or"), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x6800000000000083ull, 0xf8000000000000c7ull, N("xor"), N("b32"), DST, SRC1, T(is2) },
	{ AP, 0x68000000000001c3ull, 0xf8000000000001c7ull, N("not2"), N("b32"), DST, SRC1, T(is2) }, // yes, this is probably just a mov2 with a not bit set.
	{ AP, 0x7000000000000003ull, 0xf800000000000007ull, N("ext"), T(rev), T(us32), DST, SRC1, T(is2) }, // yes. this can reverse bits in a bitfield. really.
	{ AP, 0x7800000000000003ull, 0xf800000000000007ull, N("bfind"), T(shiftamt), T(us32), DST, T(not2), T(is2) }, // index of highest bit set, counted from 0, -1 for 0 src. or highest bit different from sign for signed version. check me.
	{ AP, 0x0000000000000003ull, 0x0000000000000007ull, OOPS, N("b32"), DST, SRC1, T(is2), T(is3) },


	// 08?
	{ AP, 0x080e00001c000004ull, 0xfc0e00001c000007ull, N("mov"), DST, PSRC1 }, // likely pnot1. and likely some ops too.
	{ AP, 0x0c0e00000001c004ull, 0xfc0e0000c001c007ull, N("and"), PDST, T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ AP, 0x0c0e00004001c004ull, 0xfc0e0000c001c007ull, N("or"), PDST, T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ AP, 0x0c0e00008001c004ull, 0xfc0e0000c001c007ull, N("xor"), PDST, T(pnot1), PSRC1, T(pnot2), PSRC2 },
	{ AP, 0x1000000000000004ull, 0xfc00000000000007ull, N("cvt"), T(rint), T(fcrm), T(cvtfdst), T(cvtfsrc) },
	{ AP, 0x1400000000000004ull, 0xfc00000000000007ull, N("cvt"), T(fcrm), T(cvtidst), T(cvtfsrc) },
	{ AP, 0x1800000000000004ull, 0xfc00000000000007ull, N("cvt"), T(fcrm), T(cvtfdst), T(cvtisrc) },
	{ AP, 0x1c00000000000004ull, 0xfc00000000000007ull, N("cvt"), T(ias), T(cvtidst), T(cvtisrc) },
	{ AP, 0x2000000000000004ull, 0xfc00000000000007ull, N("selp"), N("b32"), DST, SRC1, T(is2), T(pnot3), PSRC3 },
	{ AP, 0x2400000000000004ull, 0xfc00000000000007ull, N("prmt"), T(prmtmod), N("b32"), DST, SRC1, SRC3, T(is2) }, // NFI what this does. and sources 2 and 3 are swapped for some reason.
	{ AP, 0x2800000000000004ull, 0xfc00000000000007ull, T(lane), N("mov"), N("b32"), DST, T(is2) },
	{ AP, 0x2c00000000000004ull, 0xfc00000000000007ull, N("mov"), N("b32"), DST, SREG },
	{ AP, 0x3000c3c003f00004ull, 0xfc00c3c003f00004ull, N("mov"), DST, CC },
	{ AP, 0x3400c3c000000004ull, 0xfc00c3c000000004ull, N("mov"), CC, SRC1 },
	// 38?
	{ AP, 0x40000000000001e4ull, 0xf8040000000001e7ull, N("nop") },
	{ AP, 0x40040000000001e4ull, 0xf8040000000001e7ull, N("pmevent"), PM }, // ... a bitmask of triggered pmevents? with 0 ignored?
	{ AP, 0x48000000000fc004ull, 0xf8000000000fc067ull, N("vote"), N("all"), PDST2, T(pnot1), PSRC1 },
	{ AP, 0x48000000000fc024ull, 0xf8000000000fc067ull, N("vote"), N("any"), PDST2, T(pnot1), PSRC1 },
	{ AP, 0x48000000000fc044ull, 0xf8000000000fc067ull, N("vote"), N("uni"), PDST2, T(pnot1), PSRC1 },
	{ AP, 0x49c0000000000024ull, 0xf9c0000000000027ull, N("vote"), N("ballot"), DST, T(pnot1), PSRC1 }, // same insn as vote any, really... but need to check what happens for vote all and vote uni with non bit-bucked destination before unifying.
	{ AP, 0x5000000000000004ull, 0xfc000000000000e7ull, N("bar popc"), PDST3, DST, T(bar), T(tcnt), T(pnot3), PSRC3 }, // and yes, sync is just a special case of this.
	{ AP, 0x5000000000000024ull, 0xfc000000000000e7ull, N("bar and"), PDST3, DST, T(bar), T(tcnt), T(pnot3), PSRC3 },
	{ AP, 0x5000000000000044ull, 0xfc000000000000e7ull, N("bar or"), PDST3, DST, T(bar), T(tcnt), T(pnot3), PSRC3 },
	{ AP, 0x5000000000000084ull, 0xfc000000000000e7ull, N("bar arrive"), PDST3, DST, T(bar), T(tcnt), T(pnot3), PSRC3 },
	{ AP, 0x5400000000000004ull, 0xfc00000000000007ull, N("popc"), DST, T(not1), SRC1, T(not2), T(is2) }, // XXX: popc(SRC1 & SRC2)? insane idea, but I don't have any better
	// ???
	{ AP, 0xc000800000000004ull, 0xfc00800000000087ull, N("vadd"), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },
	{ AP, 0xc000800000000084ull, 0xfc00800000000087ull, N("vsub"), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },
	{ AP, 0xc800800000000004ull, 0xfc00800000000087ull, N("vmin"), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },
	{ AP, 0xc800800000000084ull, 0xfc00800000000087ull, N("vmax"), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },
	{ AP, 0xd000800000000004ull, 0xfc00800000000007ull, N("vabsdiff"), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },
	{ AP, 0xd800800000000004ull, 0xfc00800000000007ull, N("vset"), T(vdst), DST, T(vsetop), T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },
	{ AP, 0xe000800000000004ull, 0xfc00800000000007ull, N("vshr"), T(vsclamp), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), SRC2, SRC3  },
	{ AP, 0xe800800000000004ull, 0xfc00800000000007ull, N("vshl"), T(vsclamp), T(vas), T(vdst), T(us32d), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), SRC2, SRC3  },
	{ AP, 0xf000800000000004ull, 0xfc00800000000007ull, N("vmad"), T(vmop), T(vas), T(vmshr), DST, T(vsrc1), T(us32v), SRC1, T(vsrc2), T(us32), SRC2, SRC3  },


	{ AP, 0x1000000000000005ull, 0xf800000000000207ull, T(redop), N("u32"), T(gmem), DST },
	{ AP, 0x1000000000000205ull, 0xf800000000000207ull, N("add"), N("u64"), T(gmem), DSTD },
	{ AP, 0x1800000000000205ull, 0xf800000000000207ull, T(redops), N("s32"), T(gmem), DST },
	{ AP, 0x2800000000000205ull, 0xf800000000000207ull, N("add"), N("f32"), T(gmem), DST },
	{ AP, 0x507e000000000005ull, 0xf87e000000000307ull, N("ld"), T(redop), N("u32"), DST2, GATOM, DST }, // yet another big ugly mess. but seems to work.
	{ AP, 0x507e000000000205ull, 0xf87e0000000003e7ull, N("ld"), N("add"), N("u64"), DST2, GATOM, DST },
	{ AP, 0x507e000000000105ull, 0xf87e0000000003e7ull, N("exch"), N("b32"), DST2, GATOM, DST },
	{ AP, 0x507e000000000305ull, 0xf87e0000000003e7ull, N("exch"), N("b64"), DST2D, GATOM, DSTD },
	{ AP, 0x5000000000000125ull, 0xf8000000000003e7ull, N("cas"), N("b32"), DST2, GATOM, DST, SRC3 },
	{ AP, 0x5000000000000325ull, 0xf8000000000003e7ull, N("cas"), N("b64"), DST2D, GATOM, DSTD, SRC3D },
	{ AP, 0x587e000000000205ull, 0xf87e000000000307ull, N("ld"), T(redops), N("s32"), DST2, GATOM, DST },
	{ AP, 0x687e000000000205ull, 0xf87e0000000003e7ull, N("ld"), N("add"), N("f32"), DST2, GATOM, DST },
	{ AP, 0x8000000000000005ull, 0xf800000000000007ull, N("ld"), T(ldstt), T(ldstd), T(lcop), T(gmem) },
	{ AP, 0x8800000000000005ull, 0xf800000000000007ull, N("ldu"), T(ldstt), T(ldstd), T(gmem) },
	{ AP, 0x9000000000000005ull, 0xf800000000000007ull, N("st"), T(ldstt), T(scop), T(gmem), T(ldstd) },
	{ AP, 0xb320003f00000005ull, 0xfb20003f00000007ull, N("prefetch"), DST, SRC1, SRC2 },
	{ AP, 0xc000000000000005ull, 0xfd00000000000007ull, N("ld"), T(ldstt), T(ldstd), T(lcop), LOCAL },
	{ AP, 0xc100000000000005ull, 0xfd00000000000007ull, N("ld"), T(ldstt), T(ldstd), SHARED },
	{ AP, 0xc400000000000005ull, 0xfc00000000000007ull, N("ld"), N("lock"), T(ldstt), PDST4, T(ldstd), SHARED },
	{ AP, 0xc800000000000005ull, 0xfd00000000000007ull, N("st"), T(ldstt), T(scop), LOCAL, T(ldstd) },
	{ AP, 0xc900000000000005ull, 0xfd00000000000007ull, N("st"), T(ldstt), SHARED, T(ldstd) },
	{ AP, 0xcc00000000000005ull, 0xfc00000000000007ull, N("st"), N("unlock"), T(ldstt), SHARED, T(ldstd) },
	{ AP, 0xd400400000000005ull, 0xfc00400000000007ull, N("suldb"), T(ldstt), T(ldstd), T(lcop), T(sclamp), SURF, SADDR },
	{ AP, 0xd800400100000005ull, 0xfc00400100000007ull, N("suredp"), T(redop), T(sclamp), SURF, SADDR, DST },
	{ AP, 0xdc00400000000005ull, 0xfc02400000000007ull, N("sustb"), T(ldstt), T(scop), T(sclamp), SURF, SADDR, T(ldstd) },
	{ AP, 0xdc02400000000005ull, 0xfc02400000000007ull, N("sustp"), T(scop), T(sclamp), SURF, SADDR, DST },
	{ AP, 0xe000000000000005ull, 0xf800000000000067ull, N("membar"), N("prep") }, // always used before all 3 other membars.
	{ AP, 0xe000000000000025ull, 0xf800000000000067ull, N("membar"), N("gl") },
	{ AP, 0xf000400000000085ull, 0xfc00400000000087ull, N("suleab"), PDST2, DSTD, T(ldstt), T(sclamp), SURF, SADDR },
	{ AP, 0xe000000000000045ull, 0xf800000000000067ull, N("membar"), N("sys") },
	{ AP, 0x0000000000000005ull, 0x0000000000000007ull, OOPS },

	{ AP, 0x0000000000000006ull, 0xfe00000000000067ull, N("pfetch"), DST, VBASRC },
	{ AP, 0x06000000fc000006ull, 0xfe000000fc000107ull, N("vfetch"), VDST, T(ldvf), ATTR }, // VP
	{ AP, 0x0600000003f00006ull, 0xfe00000003f00107ull, N("vfetch"), VDST, T(ldvf), ATTR }, // GP
	{ AP, 0x0600000003f00106ull, 0xfe00000003f00107ull, N("vfetch patch"), VDST, T(ldvf), ATTR }, // per patch input
	{ AP, 0x0a00000003f00006ull, 0xfe7e000003f00107ull, N("export"), VAR, ESRC }, // GP
	{ AP, 0x0a7e000003f00006ull, 0xfe7e000003f00107ull, N("export"), VAR, ESRC }, // VP
	{ AP, 0x0a7e000003f00106ull, 0xfe7e000003f00107ull, N("export patch"), VAR, ESRC }, // per patch output
	{ AP, 0x1400000000000006ull, 0xfc00000000000007ull, N("ld"), T(ldstt), T(ldstd), FCONST },
	{ AP, 0x1c000000fc000026ull, 0xfe000000fc000067ull, N("emit") },
	{ AP, 0x1c000000fc000046ull, 0xfe000000fc000067ull, N("restart") },
	{ AP, 0x80000000fc000086ull, 0xfc000000fc000087ull, N("texauto"), T(texf), TDST, TEX, SAMP, TSRC }, // mad as a hatter.
	{ AP, 0x90000000fc000086ull, 0xfc000000fc000087ull, N("texfetch"), T(texf), TDST, TEX, SAMP, TSRC },
	{ AP, 0xc0000000fc000006ull, 0xfc000000fc000007ull, N("texsize"), T(texf), TDST, TEX, SAMP, TSRC },
	{ AP, 0x0000000000000006ull, 0x0000000000000007ull, OOPS, T(texf), TDST, TEX, SAMP, TSRC }, // is assuming a tex instruction a good idea here? probably. there are loads of unknown tex insns after all.



	{ AP, 0x0, 0x0, OOPS, DST, SRC1, T(is2), SRC3 },
};

static struct insn tabp[] = {
	{ AP, 0x1c00, 0x3c00 },
	{ AP, 0x3c00, 0x3c00, N("never") },	// probably.
	{ AP, 0x0000, 0x2000, PRED },
	{ AP, 0x2000, 0x2000, N("not"), PRED },
};

F1(brawarp, 0xf, N("allwarp")) // probably jumps if the whole warp has the predicate evaluate to true.

static struct insn tabc[] = {
	{ AP, 0x40000000000001e7ull, 0xf0000000000001e7ull, T(brawarp), T(p), N("bra"), BTARG },
	{ AP, 0x5000000000010007ull, 0xf000000000010007ull, N("call"), CTARG },
	{ AP, 0x6000000000000007ull, 0xf000000000000007ull, N("joinat"), BTARG },
	{ AP, 0x80000000000001e7ull, 0xf0000000000001e7ull, T(p), N("exit") },
	{ AP, 0x90000000000001e7ull, 0xf8000000000001e7ull, T(p), N("ret") },
	{ AP, 0x98000000000001e7ull, 0xf8000000000001e7ull, T(p), N("discard") },
	{ AP, 0xc000000000000007ull, 0xf800000000000007ull, N("quadon") },
	{ AP, 0xc800000000000007ull, 0xf800000000000007ull, N("quadpop") },
	{ AP, 0xd000000000000007ull, 0xf00000000000c007ull, N("membar"), N("cta") },
	{ AP, 0xd00000000000c007ull, 0xf00000000000c007ull, N("trap") },
	{ AP, 0x0000000000000007ull, 0x0000000000000007ull, T(p), OOPS, BTARG },
};

static struct insn tabroot[] = {
	{ AP, 7, 7, OP64, T(c) }, // control instructions, special-cased.
	{ AP, 0x0, 0x10, OP64, T(p), T(m) },
	{ AP, 0x10, 0x10, OP64, N("join"), T(p), T(m), },
};

static struct disisa nvc0_isa_s = {
	tabroot,
	8,
	8,
	1,
};

struct disisa *nvc0_isa = &nvc0_isa_s;
