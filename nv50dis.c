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
 * Table of Contents
 *  0. Table of Contents
 *  1. Instructions
 *  2. Differences from PTX
 *  3. General instruction format
 *  4. Misc. hack alerts
 *  5. Color scheme
 *  6. Table format
 *  7. Code target field
 *  8. Immediate field
 *  9. Misc number fields
 * 10. Register fields
 * 11. Memory fields
 * 12. Short instructions
 * 13. Immediate instructions
 * 14. Long instructions
 * 15. Non-predicated long instructions
 */

/*
 * Instructions, from PTX manual:
 *   
 *   1. Integer Arithmetic
 *    - sad.64		TODO	emulated
 *    - div		TODO	emulated
 *    - rem		TODO	emulated
 *    - abs		desc...
 *    - neg		desc...
 *   2. Floating-Point
 *    - add		desc...
 *    - sub		desc...
 *    - mul		desc...
 *    - fma		XXX
 *    - mad		XXX
 *    - div.approxf32	desc...
 *    - div.full.f32	TODO	emulated
 *    - div.f64		TODO	emulated
 *    - abs		desc...
 *    - neg		desc...
 *    - min		desc...
 *    - max		desc...
 *    - rcp.f32		desc...
 *    - rcp.f64		TODO	emulated
 *    - sqrt.f32	desc...
 *    - sqrt.f64	TODO	emulated
 *    - rsqrt.f32	desc...
 *    - rsqrt.f64	TODO	emulated
 *    - sin		XXX
 *    - cos		XXX
 *    - lg2		desc
 *    - ex2		XXX
 *   3. Comparison and selection
 *    - selp		desc...
 *    - slct		desc...
 *   5. Data Movement and Conversion
 *    - mov		desc...
 *    - ld		desc...
 *    - st		desc...
 *    - cvt		XXX cleanup, rounding modes
 *   6. Texture
 *    - tex		desc...
 *   7. Control Flow
 *    - { }		done
 *    - @		done
 *    - bra		desc...
 *    - call		XXX WTF?
 *    - ret		desc...
 *    - exit		desc...
 *   8. Parallel Synchronization and Communication
 *    - bar		desc...
 *    - membar.cta	desc...
 *    - membar.gl	sweet mother of God, what?
 *    - atom		desc...
 *    - red		desc...
 *    - vote		desc...
 *   9. Miscellaneous
 *    - trap		desc...
 *    - brkpt		desc...
 *    - pmevent		desc... XXX
 *  10. Undocumented/implicit stuff
 *    - join		XXX what does it do?
 *    - joinat		desc...
 *    - interp		XXX Needs figuring out.
 *    - discard		XXX what happens after you use it?
 *    - emit/reset	XXX can do combo?
 *
 */

/*
 * ISA differences from what's published about PTX:
 *
 * Instructions that don't really exist and are emulated:
 *  - sqrt.f32 -> rsqrt, rcp
 *  - neg and abs on f32 -> cvt
 *  - cvt.sat to f64 is implemented in software.
 *  - cvt with floating point can only convert up/down one size level, you
 *    need a chain of floats to go f32->u8 and others.
 *  - no 64-bit loads/stores except in l[] and g[].
 *  - vote.uni is emulated by vote.all
 *  - membar.cta is actually a nop that just forces instruction ordering
 *    before and after it.
 *  - div.approx.f32 doesn't exist and is replaced by mul by rcp
 *  - selp and slct. Emulated with set and predicated mov's.
 * Additional instructions and other stuff:
 *  - a lot of instructions can accept c?[], s[] directly
 *  - cvt can optionally abs and neg stuff before use. the sequence of
 *    operations is: abs, neg, convert+round, sat
 * Instructions that behave otherwise than you'd expect:
 *  - sin, cos, ex2 are split into two parts: function and prefunction. you
 *    need to run prefunction on input, and the real function on output from
 *    prefunction. no idea yet what the prefunctions are. presin is same as
 *    precos. presin could be mod-pi, what about preex2?
 *  - cvt to integer types always saturates. if you need chopping, just ignore
 *    the high part [or upconvert from a low part of register to full]
 *  - cvt.sat to f32 doesn't convert NaNs to 0. max.f32 with 0 to finish it off
 *  - cvt.rpi.f64 may or may not work on 1.25, add a specific check for it
 *  - same for rmi and -1.25.
 * Misc:
 *  - explicit join points
 *  - $a registers exist
 *  - there are actually 16 const spaces and global spaces.
 * Incl. graphics stuff:
 *  - v[] space exists
 *  - interp instruction
 *  - discard instruction
 *  - $o registers exist
 *
 * XXX this section is probably useless, remove it once proper ISA
 * documentation exists.
 */

/*
 * General instruction format
 * 
 * Machine code is read in 64-bit naturaly-aligned chunks, consisting of two
 * 32-bit words. If bit 0 of first word is 0, this chunk consists of two
 * single-word short instructions. Otherwise, it's a double-word instruction.
 * For double-word instructions, bits 0x20-0x21 further determine its type:
 *  00: long
 *  01: long with exit after it
 *  10: long with join before it
 *  11: immediate
 *
 * Short, long, and immediate instructions each have separate encoding tables.
 *
 * All [?] long instructions can be predicated. $c register number to be used
 * is in 0x2c-0x2d, 0x27-0x2b specifies condition to be checked in that register:
 *  code   $c formula		meaning for sub/cmp	meaning for set-to-res
 *  00000: never
 *  00001: (S & ~Z) ^ O		s/f <			<0
 *  00010: Z & ~S		=			0
 *  00011: S ^ (Z | O)		s/f <=			<=0
 *  00100: ~Z & ~(S ^ O)	s/f >			>0
 *  00101: ~Z			<>			<>0
 *  00110: ~(S ^ O)		s/f >=			>=0
 *  00111: ~Z | ~S		s/f ordered		not nan
 *  01000: Z & S		unordered		nan
 *  01001: S ^ O		s/f < or unordered	<0 or nan
 *  01010: Z			= or unordered		0 or nan
 *  01011: Z | (S ^ O)		s/f <= or unordered	<=0 or nan
 *  01100: ~S ^ (Z | O)		s/f > or undordered	>0 or nan
 *  01101: S | ~Z		s/f <> or unordered	not 0
 *  01110: (Z | ~S) ^ O		s/f >= or unordered	>=0
 *  01111: always
 *  10000: O			signed overflow
 *  10001: C			u >=
 *  10010: C & ~Z		u >
 *  10011: S			result <0		<0
 *  11100: ~S			result >=0		>=0
 *  11101: Z | ~C		u <=
 *  11110: ~C			u <
 *  11111: ~O			no signed overflow
 *
 * Bit 1 is yet another instruction subtype: 0 is normal instruction, 1 is
 * a control instruction [branches and stuff like that]. For all types and
 * subtypes, the major opcode field is 0x1c-0x1f. For long instructions,
 * 0x3d-0x3f is subopcode. From there on, there is little regularity.
 */

/*
 * Misc. hack alerts:
 *  - 0x2c-0x2d is read twice in addc instructions: it selects $c to use for
 *    a carry flag, and selects $c to use for conditional execution. Printing
 *    it depends on zeroing happening after decoding the instruction.
 *    Same thing also happens in mov-from-$c instruction.
 *  - If instruction has both s[] and c?[] memory operands and sets the AREG
 *    field to non-0, specified $a only applies to the first one. This depends
 *    on bits in *a and *b getting zeroed out by function printing out the first
 *    argument. For added fun, a[] behaves differently.
 *  - long instruction always need to be aligned to even word bounduary.
 *    However, I disabled checking for this case, since renouveau dumps contain
 *    shader code starting at random positions, and the check is mostly useless
 *    anyway.
 */

#define NV84P NV84|NVA0|NVAA|NVA3
#define NVA0P NVA0|NVAA|NVA3
#define NVA3P NVA3

/*
 * Code target field
 *
 * This field represents a code address and is used for branching and the
 * likes. Target is counted in 32-bit words from the start point of code
 * segment. Low part of target is in 0/11-26, high part is in 1/14-19. Code
 * addresses max out at 0xfffffc.
 */

static struct bitfield ctargoff = { { 0xb, 16, 0x2e, 6 }, BF_UNSIGNED, 2 };
#define BTARG atombtarg, &ctargoff
#define CTARG atomctarg, &ctargoff

/*
 * Immediate fields
 *
 *  - IMM: Used by all immediate insns [tabi table] for last argument. Full 32-bit.
 *  - PM: PM event id for pmevent
 *  - BAR: barrier id for bar.sync
 *  - OFFS: an offset to add in add $a, num, $a, which is basically a LEA
 *  - SHCNT: used in shift-by-immediate insns for shift amount
 *  - HSHCNT: used in $a shift-by-immediate insn for shift amount
 */

static struct bitfield immbf = { { 0x10, 6, 0x22, 26 }, .wrapok = 1 };
static struct bitfield pmoff = { 0xa, 4 };
static struct bitfield baroff = { 0x15, 4 };
static struct bitfield offoff = { { 9, 16 }, .wrapok = 1 };
static struct bitfield shcntoff = { 0x10, 7 };
static struct bitfield hshcntoff = { 0x10, 4 };
static struct bitfield toffxoff = { { 0x38, 4 }, BF_SIGNED };
static struct bitfield toffyoff = { { 0x34, 4 }, BF_SIGNED };
static struct bitfield toffzoff = { { 0x30, 4 }, BF_SIGNED };
#define IMM atomimm, &immbf
#define PM atomimm, &pmoff
#define BAR atomimm, &baroff
#define OFFS atomimm, &offoff
#define SHCNT atomimm, &shcntoff
#define HSHCNT atomimm, &hshcntoff
#define TOFFX atomimm, &toffxoff
#define TOFFY atomimm, &toffyoff
#define TOFFZ atomimm, &toffzoff

/*
 * Ignored fields
 *
 * Used in cases when some bitfield doesn't do anything, but nvidia's blob
 * sets it due to stupi^H^H^H^H^Hdesign decisions.
 *  - IGNCE: $c write enable
 *  - IGNPRED: predicates, for instructions that don't use them.
 *
 * The whole point of this op is to kill spurious red when disassembling
 * ptxas output and blob shaders. Don't include all random unused fields here.
 */

static int ignce[] = { 0x26, 1 };
static int ignpred[] = { 0x27, 7 };
#define IGNCE atomign, ignce
#define IGNPRED atomign, ignpred

/*
 * Register fields
 *
 * There are four locations of register fields for $r:
 *  - DST: 2 and up, used for destination reg of most insns, but for source
 *         in store instructions. Insn seems to get ignored if this field is
 *         > number of allocated regs, so don't use that for storing 0s.
 *  - SRC: 9 and up, used as a source register in all types of insns.
 *  - SRC2: 0x10 and up, used as a source register in short and long insns.
 *  - SRC3: 0x2e and up, used as s source register in long insns.
 *
 * These fields can be unused, or used as part of opcode or modifiers if insn
 * doesn't need all available fields.
 *
 * All fields are either 6-bit [S] or 7-bit [L]. Short insns and non-mov
 * immediate insns have 6-bit fields and use the high bit as opcode/mods,
 * while mov immediate and long insns use full 7-bit fields.
 *
 * If encoded registers are 32-bit [no modifier letter] or 64-bit [D], the
 * field contains register number directly. For 16-bit halves of registers [H],
 * lowest bit selects low [0] or high [1] half of given register, and higher
 * bits contain the number of register. For 64-bit registers, the number needs
 * to be even.
 *
 * DST field seems to sometimes store $o registers instead of $r. If it can,
 * using $o instead of $r is indicated by bit 0x23 being set. LLDST means
 * either $o or $r in DST. It's unknown yet in what instructions it can be
 * used [probably all non-tex non-access-g[] longs?]
 *
 * ADST is a smaller version of DST field used for storing $a destination in
 * insns that allow one.
 *
 * COND is $c register used as input, both for predicating and other purposes,
 * like addition with carry or mov from $c.
 *
 * CDST is $c register used for output. It's set according to insn result for
 * most insns that allow it. Writing to $c in this field needs to be enabled
 * by setting bit 0x26. MCDST is an op that reflects that and is used for insns
 * that can function both with or without $c output.
 * 
 * AREG is a field storing $a register used in insn for memory addressing,
 * or as a normal source register sometimes. It's especially ugly, because
 * it's spread out across 0x1a-0x1b [low part] and 0x22 [high part], with the
 * high part assumed 0 if insn is short or immediate. Also, if two memory
 * accesses are used in the same insn, this field applies only to the first.
 *
 */

static struct sreg areg_sr[] = {
	{ 0, 0, SR_ZERO },
	{ -1 },
};
static struct sreg oreg_sr[] = {
	{ 127, 0, SR_DISCARD },
	{ -1 },
};
static struct sreg sreg_sr[] = {
	{ 0, "physid" },
	{ 1, "clock" },
	{ 4, "pm0" },
	{ 5, "pm1" },
	{ 6, "pm2" },
	{ 7, "pm3" },
	{ 8, "sampleid", .vartype = NVA3P },
	{ -1 },
};
static struct bitfield sdst_bf = { 2, 6 };
static struct bitfield ldst_bf = { 2, 7 };
static struct bitfield ssrc_bf = { 9, 6 };
static struct bitfield lsrc_bf = { 9, 7 };
static struct bitfield ssrc2_bf = { 0x10, 6 };
static struct bitfield lsrc2_bf = { 0x10, 7 };
static struct bitfield lsrc3_bf = { 0x2e, 7 };
static struct bitfield odst_bf = { 2, 7 };
static struct bitfield sareg_bf = { 0x1a, 2 };
static struct bitfield lareg_bf = { { 0x1a, 2, 0x22, 1 } };
static struct bitfield adst_bf = { 2, 3 };
static struct bitfield cond_bf = { 0x2c, 2 };
static struct bitfield c0_bf = { 0, 0 };
static struct bitfield cdst_bf = { 0x24, 2 };
static struct bitfield tex_bf = { 9, 7 };
static struct bitfield samp_bf = { 0x11, 4 };
static struct bitfield sreg_bf = { 0x2e, 4 };

static struct reg sdst_r = { &sdst_bf, "r" };
static struct reg ldst_r = { &ldst_bf, "r" };
static struct reg shdst_r = { &sdst_bf, "r", .hilo = 1 };
static struct reg lhdst_r = { &ldst_bf, "r", .hilo = 1 };
static struct reg lddst_r = { &ldst_bf, "r", "d" };
static struct reg lqdst_r = { &ldst_bf, "r", "q" };
static struct reg ssrc_r = { &ssrc_bf, "r" };
static struct reg lsrc_r = { &lsrc_bf, "r" };
static struct reg shsrc_r = { &ssrc_bf, "r", .hilo = 1 };
static struct reg lhsrc_r = { &lsrc_bf, "r", .hilo = 1 };
static struct reg ldsrc_r = { &lsrc_bf, "r", "d" };
static struct reg ssrc2_r = { &ssrc2_bf, "r" };
static struct reg lsrc2_r = { &lsrc2_bf, "r" };
static struct reg shsrc2_r = { &ssrc2_bf, "r", .hilo = 1 };
static struct reg lhsrc2_r = { &lsrc2_bf, "r", .hilo = 1 };
static struct reg ldsrc2_r = { &lsrc2_bf, "r", "d" };
static struct reg lsrc3_r = { &lsrc3_bf, "r" };
static struct reg lhsrc3_r = { &lsrc3_bf, "r", .hilo = 1 };
static struct reg ldsrc3_r = { &lsrc3_bf, "r", "d" };
static struct reg odst_r = { &odst_bf, "o", .specials = oreg_sr, .cool = 1 };
static struct reg ohdst_r = { &odst_bf, "o", .specials = oreg_sr, .hilo = 1, .cool = 1 };
static struct reg sareg_r = { &sareg_bf, "a", .specials = areg_sr, .cool = 1 }; // for mem operands only
static struct reg lareg_r = { &lareg_bf, "a", .specials = areg_sr, .cool = 1 };
static struct reg adst_r = { &adst_bf, "a", .specials = areg_sr, .cool = 1 };
static struct reg cond_r = { &cond_bf, "c", .cool = 1 };
static struct reg c0_r = { &c0_bf, "c", .cool = 1 };
static struct reg cdst_r = { &cdst_bf, "c", .cool = 1 };
static struct reg tex_r = { &tex_bf, "t", .cool = 1 };
static struct reg samp_r = { &samp_bf, "s", .cool = 1 };
static struct reg sreg_r = { &sreg_bf, "sr", .specials = sreg_sr, .always_special = 1 };
#define SDST atomreg, &sdst_r
#define LDST atomreg, &ldst_r
#define SHDST atomreg, &shdst_r
#define LHDST atomreg, &lhdst_r
#define LDDST atomreg, &lddst_r
#define LQDST atomreg, &lqdst_r
#define SSRC atomreg, &ssrc_r
#define LSRC atomreg, &lsrc_r
#define SHSRC atomreg, &shsrc_r
#define LHSRC atomreg, &lhsrc_r
#define LDSRC atomreg, &ldsrc_r
#define SSRC2 atomreg, &ssrc2_r
#define LSRC2 atomreg, &lsrc2_r
#define SHSRC2 atomreg, &shsrc2_r
#define LHSRC2 atomreg, &lhsrc2_r
#define LDSRC2 atomreg, &ldsrc2_r
#define LSRC3 atomreg, &lsrc3_r
#define LHSRC3 atomreg, &lhsrc3_r
#define LDSRC3 atomreg, &ldsrc3_r
#define LAREG atomreg, &lareg_r
#define ODST atomreg, &odst_r
#define OHDST atomreg, &ohdst_r
#define ADST atomreg, &adst_r
#define COND atomreg, &cond_r
#define C0 atomreg, &c0_r
#define CDST atomreg, &cdst_r
#define TEX atomreg, &tex_r
#define SAMP atomreg, &samp_r
#define SREG atomreg, &sreg_r

static struct bitfield tdst_cnt = { .addend = 4 };
static struct bitfield ltdst_mask = { { 0x19, 2, 0x2e, 2 } };
static struct bitfield tsrc_cnt = { { 0x16, 2 }, .addend = 1 };
static struct vec ltdst_v = { "r", &ldst_bf, &tdst_cnt, &ltdst_mask };
static struct vec stdst_v = { "r", &sdst_bf, &tdst_cnt, 0 };
static struct vec ltsrc_v = { "r", &ldst_bf, &tsrc_cnt, 0 };
static struct vec stsrc_v = { "r", &sdst_bf, &tsrc_cnt, 0 };
#define LTDST atomvec, &ltdst_v
#define STDST atomvec, &stdst_v
#define LTSRC atomvec, &ltsrc_v
#define STSRC atomvec, &stsrc_v

#define LLDST T(lldst)
F(lldst, 0x23, LDST, ODST)
#define LLHDST T(llhdst)
F(llhdst, 0x23, LHDST, OHDST)

#define MCDST T(mcdst)
F1(mcdst, 0x26, CDST)

/*
 * Memory fields
 *
 * Memory accesses on nv50 are generally of the form x[$a+offset]. $a register
 * is taken from AREG and applies only to first memory operand actually used
 * in a given insn. Offset is taken from a field in insn and maybe multiplied
 * by operand size before use.
 *
 * Fields for reading shared/attribute spaces:
 *  S*SHARED: s/a space, multiplied, shares field with S*SRC.
 *    Used in many short/immediate insns instead of S*SRC when 0/24 set.
 *  L*SHARED, s/a space, multiplied, shares field with L*SRC.
 *    Used in many long insns instead of L*SRC if 1/21 set.
 * High 2 bits of both of these fields are some sort of subspace specifier.
 * Known ones include:
 *  For 8-bit operands: 00 - s[] space.
 *  For 16-bit operands: 01 - s[] space for unsigned operands, 10 - s[] space
 *    for signed operands. I have no idea what's the difference.
 *  For 32-bit operands: 00 - a[] space, 11 - s[] space.
 * Low 4 or 5 bits are offset that gets multiplied by access size before use.
 *
 * Fields for reading const spaces:
 *  S*CONST: c* space, multiplied, shares field with S*SRC2.
 *    Used in many short insns instead of S*SRC2 if 0/23 set.
 *  L*CONST2: c* space, multiplied, shares field with L*SRC2.
 *    Used in many long insns instead of L*SRC2 if 1/23 set.
 *  L*CONST3: c* space, multiplied, shares field with L*SRC3.
 *    Used in many long insns instead of L*SRC3 if 1/24 set.
 *  F*CONST: c* space, multiplied, uses 15-bit offset. Seen only on special
 *    movs from c* spaces.
 * L*CONST* and F*CONST have constant space number in 1/22-25. S*CONST uses
 * only 5 bits of its 6-bit field, presumably 0/21 selects c1 or c0, but needs
 * to be checked.
 *
 * For some reason ptxas hasn't been seen emitting both S*CONST and S*SHARED
 * nor L*CONST2 and L*CONST3 in the same insn. But L*SHARED with L*CONST[23]
 * works ok.
 *
 * Fields for storing to shared space:
 *  F*SHARED: s space, multiplied. Used only in mov to s[].
 *
 * Fields for local space:
 *  LOCAL: l space, not multiplied. Used only in movs to/from l[].
 *
 * g[] space is totally different: its only addressing mode is g[$r], where
 * $r is taken from LSRC operand.
 */

// BF, offset shift, 'l', flags, const space num BF. flags: 1 supports $a, 2 supports full 3-bit $a, 4 supports autoincrement

static struct bitfield fmem8_imm = { { 9, 16 }, BF_UNSIGNED, 0, .wrapok = 1 };
static struct bitfield fmem16_imm = { { 9, 15 }, BF_UNSIGNED, 1, .wrapok = 1 };
static struct bitfield fmem32_imm = { { 9, 14 }, BF_UNSIGNED, 2, .wrapok = 1 };

static struct bitfield ssmem8_imm = { { 9, 4 }, BF_UNSIGNED, 0 };
static struct bitfield ssmem16_imm = { { 9, 4 }, BF_UNSIGNED, 1 };
static struct bitfield ssmem32_imm = { { 9, 4 }, BF_UNSIGNED, 2 };
static struct bitfield lsmem8_imm = { { 9, 5 }, BF_UNSIGNED, 0 };
static struct bitfield lsmem16_imm = { { 9, 5 }, BF_UNSIGNED, 1 };
static struct bitfield lsmem32_imm = { { 9, 5 }, BF_UNSIGNED, 2 };
static struct bitfield ssmem8pi_imm = { { 9, 4 }, BF_SIGNED, 0 };
static struct bitfield ssmem16pi_imm = { { 9, 4 }, BF_SIGNED, 1 };
static struct bitfield ssmem32pi_imm = { { 9, 4 }, BF_SIGNED, 2 };
static struct bitfield lsmem8pi_imm = { { 9, 5 }, BF_SIGNED, 0 };
static struct bitfield lsmem16pi_imm = { { 9, 5 }, BF_SIGNED, 1 };
static struct bitfield lsmem32pi_imm = { { 9, 5 }, BF_SIGNED, 2 };
static struct mem ssmem8_m = { "s", 0, &sareg_r, &ssmem8_imm };
static struct mem ssmem16_m = { "s", 0, &sareg_r, &ssmem16_imm };
static struct mem ssmem32_m = { "s", 0, &sareg_r, &ssmem32_imm };
static struct mem lsmem8_m = { "s", 0, &lareg_r, &lsmem8_imm };
static struct mem lsmem16_m = { "s", 0, &lareg_r, &lsmem16_imm };
static struct mem lsmem32_m = { "s", 0, &lareg_r, &lsmem32_imm };
static struct mem ssmem8pi_m = { "s", 0, &sareg_r, &ssmem8pi_imm, .postincr = 1 };
static struct mem ssmem16pi_m = { "s", 0, &sareg_r, &ssmem16pi_imm, .postincr = 1 };
static struct mem ssmem32pi_m = { "s", 0, &sareg_r, &ssmem32pi_imm, .postincr = 1 };
static struct mem lsmem8pi_m = { "s", 0, &lareg_r, &lsmem8pi_imm, .postincr = 1 };
static struct mem lsmem16pi_m = { "s", 0, &lareg_r, &lsmem16pi_imm, .postincr = 1 };
static struct mem lsmem32pi_m = { "s", 0, &lareg_r, &lsmem32pi_imm, .postincr = 1 };
static struct mem fsmem8_m = { "s", 0, &lareg_r, &fmem8_imm };
static struct mem fsmem16_m = { "s", 0, &lareg_r, &fmem16_imm };
static struct mem fsmem32_m = { "s", 0, &lareg_r, &fmem32_imm };
static struct mem fsmem8pi_m = { "s", 0, &lareg_r, &fmem8_imm, .postincr = 1 };
static struct mem fsmem16pi_m = { "s", 0, &lareg_r, &fmem16_imm, .postincr = 1 };
static struct mem fsmem32pi_m = { "s", 0, &lareg_r, &fmem32_imm, .postincr = 1 };
#define SSHARED8 atommem, &ssmem8_m
#define SSHARED16 atommem, &ssmem16_m
#define SSHARED32 atommem, &ssmem32_m
#define LSHARED8 atommem, &lsmem8_m
#define LSHARED16 atommem, &lsmem16_m
#define LSHARED32 atommem, &lsmem32_m
#define SSHARED8PI atommem, &ssmem8pi_m
#define SSHARED16PI atommem, &ssmem16pi_m
#define SSHARED32PI atommem, &ssmem32pi_m
#define LSHARED8PI atommem, &lsmem8pi_m
#define LSHARED16PI atommem, &lsmem16pi_m
#define LSHARED32PI atommem, &lsmem32pi_m
#define FSHARED8 atommem, &fsmem8_m
#define FSHARED16 atommem, &fsmem16_m
#define FSHARED32 atommem, &fsmem32_m
#define FSHARED8PI atommem, &fsmem8pi_m
#define FSHARED16PI atommem, &fsmem16pi_m
#define FSHARED32PI atommem, &fsmem32pi_m
F(fs8, 0x19, FSHARED8, FSHARED8PI);
F(fs16, 0x19, FSHARED16, FSHARED16PI);
F(fs32, 0x19, FSHARED32, FSHARED32PI);

// XXX: try postincr on remaining types
static struct bitfield scmem_idx = { 0x15, 1 };
static struct bitfield lcmem_idx = { 0x36, 4 };
static struct bitfield scmem16_imm = { { 0x10, 5 }, BF_UNSIGNED, 1 };
static struct bitfield scmem32_imm = { { 0x10, 5 }, BF_UNSIGNED, 2 };
static struct bitfield scmem16pi_imm = { { 0x10, 5 }, BF_SIGNED, 1 };
static struct bitfield scmem32pi_imm = { { 0x10, 5 }, BF_SIGNED, 2 };
static struct bitfield l2cmem16_imm = { { 0x10, 7 }, BF_UNSIGNED, 1 };
static struct bitfield l2cmem32_imm = { { 0x10, 7 }, BF_UNSIGNED, 2 };
static struct bitfield l3cmem16_imm = { { 0x2e, 7 }, BF_UNSIGNED, 1 };
static struct bitfield l3cmem32_imm = { { 0x2e, 7 }, BF_UNSIGNED, 2 };
static struct bitfield l2cmem16pi_imm = { { 0x10, 7 }, BF_SIGNED, 1 };
static struct bitfield l2cmem32pi_imm = { { 0x10, 7 }, BF_SIGNED, 2 };
static struct bitfield l3cmem16pi_imm = { { 0x2e, 7 }, BF_SIGNED, 1 };
static struct bitfield l3cmem32pi_imm = { { 0x2e, 7 }, BF_SIGNED, 2 };
static struct mem scmem16_m = { "c", &scmem_idx, &sareg_r, &scmem16_imm };
static struct mem scmem32_m = { "c", &scmem_idx, &sareg_r, &scmem32_imm };
static struct mem scmem16pi_m = { "c", &scmem_idx, &sareg_r, &scmem16pi_imm, .postincr = 1 };
static struct mem scmem32pi_m = { "c", &scmem_idx, &sareg_r, &scmem32pi_imm, .postincr = 1 };
static struct mem l2cmem16_m = { "c", &lcmem_idx, &lareg_r, &l2cmem16_imm };
static struct mem l2cmem32_m = { "c", &lcmem_idx, &lareg_r, &l2cmem32_imm };
static struct mem l3cmem16_m = { "c", &lcmem_idx, &lareg_r, &l3cmem16_imm };
static struct mem l3cmem32_m = { "c", &lcmem_idx, &lareg_r, &l3cmem32_imm };
static struct mem l2cmem16pi_m = { "c", &lcmem_idx, &lareg_r, &l2cmem16pi_imm, .postincr = 1 };
static struct mem l2cmem32pi_m = { "c", &lcmem_idx, &lareg_r, &l2cmem32pi_imm, .postincr = 1 };
static struct mem l3cmem16pi_m = { "c", &lcmem_idx, &lareg_r, &l3cmem16pi_imm, .postincr = 1 };
static struct mem l3cmem32pi_m = { "c", &lcmem_idx, &lareg_r, &l3cmem32pi_imm, .postincr = 1 };
static struct mem l2cmem16na_m = { "c", &lcmem_idx, 0, &l2cmem16_imm };
static struct mem l2cmem32na_m = { "c", &lcmem_idx, 0, &l2cmem32_imm };
static struct mem l3cmem16na_m = { "c", &lcmem_idx, 0, &l3cmem16_imm };
static struct mem l3cmem32na_m = { "c", &lcmem_idx, 0, &l3cmem32_imm };
static struct mem fcmem8_m = { "c", &lcmem_idx, &lareg_r, &fmem8_imm };
static struct mem fcmem16_m = { "c", &lcmem_idx, &lareg_r, &fmem16_imm };
static struct mem fcmem32_m = { "c", &lcmem_idx, &lareg_r, &fmem32_imm };
static struct mem fcmem8pi_m = { "c", &lcmem_idx, &lareg_r, &fmem8_imm, .postincr = 1 };
static struct mem fcmem16pi_m = { "c", &lcmem_idx, &lareg_r, &fmem16_imm, .postincr = 1 };
static struct mem fcmem32pi_m = { "c", &lcmem_idx, &lareg_r, &fmem32_imm, .postincr = 1 };
#define SCONST16 atommem, &scmem16_m
#define SCONST32 atommem, &scmem32_m
#define SCONST16PI atommem, &scmem16pi_m
#define SCONST32PI atommem, &scmem32pi_m
#define L2CONST16 atommem, &l2cmem16_m
#define L2CONST32 atommem, &l2cmem32_m
#define L3CONST16 atommem, &l3cmem16_m
#define L3CONST32 atommem, &l3cmem32_m
#define L2CONST16PI atommem, &l2cmem16pi_m
#define L2CONST32PI atommem, &l2cmem32pi_m
#define L3CONST16PI atommem, &l3cmem16pi_m
#define L3CONST32PI atommem, &l3cmem32pi_m
#define L2CONST16NA atommem, &l2cmem16na_m
#define L2CONST32NA atommem, &l2cmem32na_m
#define L3CONST16NA atommem, &l3cmem16na_m
#define L3CONST32NA atommem, &l3cmem32na_m
#define FCONST8 atommem, &fcmem8_m
#define FCONST16 atommem, &fcmem16_m
#define FCONST32 atommem, &fcmem32_m
#define FCONST8PI atommem, &fcmem8pi_m
#define FCONST16PI atommem, &fcmem16pi_m
#define FCONST32PI atommem, &fcmem32pi_m

static struct mem lmem_m = { "l", 0, &lareg_r, &fmem8_imm };
static struct mem lmempi_m = { "l", 0, &lareg_r, &fmem8_imm, .postincr = 1 };
#define LOCAL atommem, &lmem_m
#define LOCALPI atommem, &lmempi_m
F(local, 0x19, LOCAL, LOCALPI);

// XXX: try for postincr
static struct bitfield samem_imm = { { 9, 6 }, BF_UNSIGNED, 2 };
static struct bitfield lamem_imm = { { 9, 7 }, BF_UNSIGNED, 2 };
static struct mem samem_m = { "a", 0, 0, &samem_imm };
static struct mem lamem_m = { "a", 0, 0, &lamem_imm };
static struct mem famem_m = { "a", 0, &lareg_r, &lamem_imm };
static struct mem spmem_m = { "p", 0, &sareg_r, &samem_imm };
static struct mem lpmem_m = { "p", 0, &lareg_r, &lamem_imm };
#define SATTR atommem, &samem_m
#define LATTR atommem, &lamem_m
#define FATTR atommem, &famem_m
#define SPRIM atommem, &spmem_m
#define LPRIM atommem, &lpmem_m

// XXX: try for postincr
static struct bitfield vmem_imm = { { 0x10, 8 }, BF_UNSIGNED, 2 };
static struct mem svmem_m = { "v", 0, &sareg_r, &vmem_imm };
static struct mem lvmem_m = { "v", 0, &lareg_r, &vmem_imm };
#define SVAR atommem, &svmem_m
#define LVAR atommem, &lvmem_m

static struct bitfield global_idx = { 0x10, 4 };
static struct bitfield global2_idx = { 0x17, 4 };
static struct mem global_m = { "g", &global_idx, &lsrc_r };
static struct mem global2_m = { "g", &global2_idx, &lsrc_r };
#define GLOBAL atommem, &global_m
#define GLOBAL2 atommem, &global2_m

static struct insn tabss[] = {
	{ -1, GP, 0x01800000, 0x01800000, SPRIM },	// XXX check
	{ -1, CP, 0x01000000, 0x03806000, N("u8"), SSHARED8 },
	{ -1, CP, 0x01002000, 0x03806000, N("u16"), SSHARED16 },
	{ -1, CP, 0x01004000, 0x03806000, N("s16"), SSHARED16 },
	{ -1, CP, 0x01006000, 0x03806000, N("b32"), SSHARED32 },
	{ -1, CP, 0x03000000, 0x03806000, N("u8"), SSHARED8PI },
	{ -1, CP, 0x03002000, 0x03806000, N("u16"), SSHARED16PI },
	{ -1, CP, 0x03004000, 0x03806000, N("s16"), SSHARED16PI },
	{ -1, CP, 0x03006000, 0x03806000, N("b32"), SSHARED32PI },
	{ -1, VP|GP, 0x01000000, 0x01800000, SATTR },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabrealls[] = {
	{ -1, CP, 0x00000000, 0x0200c000, N("u8"), LSHARED8 },
	{ -1, CP, 0x00004000, 0x0200c000, N("u16"), LSHARED16 },
	{ -1, CP, 0x00008000, 0x0200c000, N("s16"), LSHARED16 },
	{ -1, CP, 0x0000c000, 0x0200c000, N("b32"), LSHARED32 },
	{ -1, CP, 0x02000000, 0x0200c000, N("u8"), LSHARED8PI },
	{ -1, CP, 0x02004000, 0x0200c000, N("u16"), LSHARED16PI },
	{ -1, CP, 0x02008000, 0x0200c000, N("s16"), LSHARED16PI },
	{ -1, CP, 0x0200c000, 0x0200c000, N("b32"), LSHARED32PI },
	{ -1, VP|GP, 0x00000000, 0x00000000, LATTR },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabls[] = {
	{ -1, -1, 0x00000000, 0x01800000, T(realls) },
	{ -1, -1, 0x00800000, 0x01800000, T(realls) },
	{ -1, -1, 0x01000000, 0x01800000, T(realls) },
	{ -1, GP, 0x01800000, 0x01800000, LPRIM },	// XXX check
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabaddop[] = {
	{ -1, -1, 0x00000000, 0x10400000, N("add") },
	{ -1, -1, 0x00400000, 0x10400000, N("sub") },
	{ -1, -1, 0x10000000, 0x10400000, N("subr") },
	{ -1, -1, 0x10400000, 0x10400000, N("addc") },
	{ -1, -1, 0, 0, OOPS },
};

/*
 * Short instructions
 */

// various stuff available for filling the misc bits.

F1(sm1sat, 8, N("sat"))
F(sm1us16, 8, N("u16"), N("s16"))
F(sm1us32, 8, N("u32"), N("s32"))
F1(sm1high, 8, N("high"))
F(sm1tex, 8, N("all"), N("live"))

F1(sm2neg, 0xf, N("neg"))
F1(sm2abs, 0xf, N("abs"))
F(sm2us16, 0xf, N("u16"), N("s16"))
F(sm2us24, 0xf, N("u24"), N("s24"))

F1(sm3neg, 0x16, N("neg"))
F1(sm3not, 0x16, N("not"))

F1(splease, 0x11, N("please"))

static struct insn tabssh[] = {
	{ -1, -1, 0x00000000, 0x01000000, SHSRC },
	{ -1, -1, 0x01000000, 0x01000000, T(ss) },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabssw[] = {
	{ -1, -1, 0x00000000, 0x01000000, SSRC },
	{ -1, -1, 0x01000000, 0x01000000, T(ss) },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabsch[] = {
	{ -1, -1, 0x00000000, 0x01800000, SHSRC2 },
	{ -1, -1, 0x00800000, 0x03800000, SCONST16 },
	{ -1, -1, 0x02800000, 0x03800000, SCONST16PI },
	{ -1, -1, 0x01000000, 0x01800000, SHSRC2 },
	{ -1, -1, 0x01800000, 0x01800000, SHSRC2 },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabscw[] = {
	{ -1, -1, 0x00000000, 0x01800000, SSRC2 },
	{ -1, -1, 0x00800000, 0x03800000, SCONST32 },
	{ -1, -1, 0x02800000, 0x03800000, SCONST32PI },
	{ -1, -1, 0x01000000, 0x01800000, SSRC2 },
	{ -1, -1, 0x01800000, 0x01800000, SSRC2 },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabaddc0[] = {
	{ -1, -1, 0x00000000, 0x10400000 },
	{ -1, -1, 0x00400000, 0x10400000 },
	{ -1, -1, 0x10000000, 0x10400000 },
	{ -1, -1, 0x10400000, 0x10400000, C0 },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabas[] = {
	{ -1, -1, 0x00000000, 0x00008000, T(sm1sat), N("b16"), SHDST, T(ssh), T(sch) },
	{ -1, -1, 0x00008000, 0x00008000, T(sm1sat), N("b32"), SDST, T(ssw), T(scw) },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabs[] = {
	{ -1, -1, 0x10000000, 0xf0008002, T(splease), N("mov"), N("b16"), SHDST, T(ssh) },
	{ -1, -1, 0x10008000, 0xf0008002, T(splease), N("mov"), N("b32"), SDST, T(ssw) },

	{ -1, -1, 0x20000000, 0xe0008002, T(addop), T(sm1sat), N("b16"), SHDST, T(ssh), T(sch), T(addc0) },
	{ -1, -1, 0x20008000, 0xe0008002, T(addop), T(sm1sat), N("b32"), SDST, T(ssw), T(scw), T(addc0) },

	{ -1, -1, 0x40000000, 0xf0400002, N("mul"), SDST, T(sm2us16), T(ssh), T(sm1us16), T(sch) },
	{ -1, -1, 0x40400000, 0xf0400002, N("mul"), SDST, T(sm1high), T(sm2us24), T(ssw), T(scw) },

	{ -1, -1, 0x50000000, 0xf0008002, N("sad"), SDST, T(sm1us16), SHSRC, SHSRC2, SDST },
	{ -1, -1, 0x50008000, 0xf0008002, N("sad"), SDST, T(sm1us32), SSRC, SSRC2, SDST },

	{ -1, -1, 0x60000000, 0xe0008102, T(addop), SDST, N("mul"), N("u16"), T(ssh), T(sch), SDST, T(addc0) },
	{ -1, -1, 0x60000100, 0xe0008102, T(addop), SDST, N("mul"), N("s16"), T(ssh), T(sch), SDST, T(addc0) },
	{ -1, -1, 0x60008000, 0xe0008102, T(addop), N("sat"), SDST, N("mul"), N("s16"), T(ssh), T(sch), SDST, T(addc0) },
	{ -1, -1, 0x60008100, 0xe0008102, T(addop), SDST, N("mul"), N("u24"), T(ssw), T(scw), SDST, T(addc0) },

	// desc VVV
	{ -1, FP, 0x80000000, 0xf3000102, N("interp"), SDST, SVAR },		// most likely something is wrong with this.
	{ -1, FP, 0x81000000, 0xf3000102, N("interp"), SDST, N("cent"), SVAR },
	{ -1, FP, 0x82000000, 0xf3000102, N("interp"), SDST, SVAR, SSRC },
	{ -1, FP, 0x83000000, 0xf3000102, N("interp"), SDST, N("cent"), SVAR, SSRC },
	{ -1, FP, 0x80000100, 0xf3000102, N("interp"), SDST, N("flat"), SVAR },

	{ -1, -1, 0x90000000, 0xf0000002, N("rcp f32"), SDST, T(sm3neg), T(sm2abs), SSRC },

	{ -1, -1, 0xb0000000, 0xf0000002, N("add"), T(sm1sat), N("f32"), SDST, T(sm2neg), T(ssw), T(sm3neg), T(scw) },

	{ -1, -1, 0xc0000000, 0xf0000002, N("mul f32"), SDST, T(sm2neg), T(ssw), T(sm3neg), T(scw) },

	{ -1, -1, 0xe0000000, 0xf0000002, N("add"), T(sm1sat), N("f32"), SDST, T(sm2neg), N("mul"), T(ssw), T(scw), T(sm3neg), SDST },

	{ -1, -1, 0xf0000000, 0xf1000002, N("texauto"), T(sm1tex), STDST, TEX, SAMP, STSRC },
	{ -1, -1, 0xf1000000, 0xf1000002, N("texfetch"), T(sm1tex), STDST, TEX, SAMP, STSRC},
	// desc ^^^

	{ -1, -1, 0, 2, OOPS, SDST, T(ssw), T(scw) },

	{ NV84P, -1, 0xb0000002, 0xf0000002, N("brkpt") },

	{ -1, -1, 0, 0, OOPS }
};

/*
 * Immediate instructions
 */

static struct insn tabi[] = {
	{ -1, -1, 0x10000000, 0xf0008000, N("mov"), N("b16"), LHDST, IMM },
	{ -1, -1, 0x10008000, 0xf0008000, N("mov"), N("b32"), LDST, IMM },	// yes. LDST. special case.

	{ -1, -1, 0x20000000, 0xe0008000, T(addop), T(sm1sat), N("b16"), SHDST, T(ssh), IMM, T(addc0) },
	{ -1, -1, 0x20008000, 0xe0008000, T(addop), T(sm1sat), N("b32"), SDST, T(ssw), IMM, T(addc0) },

	{ -1, -1, 0x40000000, 0xf0400000, N("mul"), SDST, T(sm2us16), T(ssh), T(sm1us16), IMM },
	{ -1, -1, 0x40400000, 0xf0400000, N("mul"), SDST, T(sm1high), T(sm2us24), T(ssw), IMM },

	{ -1, -1, 0x60000000, 0xe0008100, T(addop), SDST, N("mul"), N("u16"), T(ssh), IMM, SDST, T(addc0) },
	{ -1, -1, 0x60000100, 0xe0008100, T(addop), SDST, N("mul"), N("s16"), T(ssh), IMM, SDST, T(addc0) },
	{ -1, -1, 0x60008000, 0xe0008100, T(addop), N("sat"), SDST, N("mul"), N("s16"), T(ssh), IMM, SDST, T(addc0) },
	{ -1, -1, 0x60008100, 0xe0008100, T(addop), SDST, N("mul"), N("u24"), T(ssw), IMM, SDST, T(addc0) },

	// desc VVV
	{ -1, -1, 0xb0000000, 0xf0000000, N("add"), T(sm1sat), N("f32"), SDST, T(sm2neg), T(ssw), T(sm3neg), IMM },

	{ -1, -1, 0xc0000000, 0xf0000000, N("mul f32"), SDST, T(sm2neg), T(ssw), T(sm3neg), IMM },
	// desc ^^^

	{ -1, -1, 0xd0000000, 0xf0008100, N("and"), N("b32"), SDST, T(sm3not), T(ssw), IMM },
	{ -1, -1, 0xd0000100, 0xf0008100, N("or"), N("b32"), SDST, T(sm3not), T(ssw), IMM },
	{ -1, -1, 0xd0008000, 0xf0008100, N("xor"), N("b32"), SDST, T(sm3not), T(ssw), IMM },
	{ -1, -1, 0xd0008100, 0xf0008100, N("mov2"), N("b32"), SDST, T(sm3not), T(ssw), IMM },

	// desc VVV
	{ -1, -1, 0xe0000000, 0xf0000000, N("add"), T(sm1sat), N("f32"), SDST, T(sm2neg), N("mul"), T(ssw), IMM, T(sm3neg), SDST },
	// desc ^^^

	{ -1, -1, 0, 0, OOPS, SDST, T(ssw), IMM },
};

/*
 * Long instructions
 */

// A few helper tables for usual operand types.

F(lsh, 0x35, LHSRC, T(ls))
F(lsw, 0x35, LSRC, T(ls))

static struct insn tablc2h[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0000000001800000ull, LHSRC2 },
	{ -1, -1, 0x0000000000800000ull, 0x0020000003800000ull, L2CONST16 },
	{ -1, -1, 0x0000000002800000ull, 0x0020000003800000ull, L2CONST16PI },
	{ -1, -1, 0x0020000000800000ull, 0x0020000001800000ull, L2CONST16NA },
	{ -1, -1, 0x0000000001000000ull, 0x0000000001800000ull, LHSRC2 },
	{ -1, -1, 0x0000000001800000ull, 0x0000000001800000ull, LHSRC2 },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tablc2w[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0000000001800000ull, LSRC2 },
	{ -1, -1, 0x0000000000800000ull, 0x0020000003800000ull, L2CONST32 },
	{ -1, -1, 0x0000000002800000ull, 0x0020000003800000ull, L2CONST32 },
	{ -1, -1, 0x0020000000800000ull, 0x0020000001800000ull, L2CONST32NA },
	{ -1, -1, 0x0000000001000000ull, 0x0000000001800000ull, LSRC2 },
	{ -1, -1, 0x0000000001800000ull, 0x0000000001800000ull, LSRC2 },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tablc3h[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0000000001800000ull, LHSRC3 },
	{ -1, -1, 0x0000000000800000ull, 0x0000000001800000ull, LHSRC3 },
	{ -1, -1, 0x0000000001000000ull, 0x0020000003800000ull, L3CONST16 },
	{ -1, -1, 0x0000000003000000ull, 0x0020000003800000ull, L3CONST16PI },
	{ -1, -1, 0x0020000001000000ull, 0x0020000001800000ull, L3CONST16NA },
	{ -1, -1, 0x0000000001800000ull, 0x0000000001800000ull, LHSRC3 },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tablc3w[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0000000001800000ull, LSRC3 },
	{ -1, -1, 0x0000000000800000ull, 0x0000000001800000ull, LSRC3 },
	{ -1, -1, 0x0000000001000000ull, 0x0020000003800000ull, L3CONST32 },
	{ -1, -1, 0x0000000003000000ull, 0x0020000003800000ull, L3CONST32PI },
	{ -1, -1, 0x0020000001000000ull, 0x0020000001800000ull, L3CONST32NA },
	{ -1, -1, 0x0000000001800000ull, 0x0000000001800000ull, LSRC3 },
	{ -1, -1, 0, 0, OOPS },
};

F(shcnt, 0x34, T(lc2w), SHCNT)
F(hshcnt, 0x34, T(lc2h), SHCNT)

// various stuff available for filling the misc bits.

// stolen from SRC3
F(s30us16, 0x2e, N("u16"), N("s16"))
F1(s30high, 0x2e, N("high"))
F(s31us16, 0x2f, N("u16"), N("s16"))
F(s31us24, 0x2f, N("u24"), N("s24"))
F1(s32not, 0x30, N("not"))
F1(s33not, 0x31, N("not"))
F1(s35sat, 0x33, N("sat"))
F1(s35abs, 0x33, N("abs"))
F1(s36abs, 0x34, N("abs"))
// the actual misc field
F1(m1neg, 0x3a, N("neg"))
F1(m2neg, 0x3b, N("neg"))
F1(m2sat, 0x3b, N("sat"))
F(lm2us16, 0x3b, N("u16"), N("s16"))
F(lm2us32, 0x3b, N("u32"), N("s32"))
// stolen from opcode field.
F1(o0neg, 0x3d, N("neg"))
F1(o0sat, 0x3d, N("sat"))
F1(lplease, 0x39, N("please"))

static struct insn tablfm1[] = {
	{ -1, -1, 0, 0, T(m1neg), T(s36abs) }
};

static struct insn tablfm2[] = {
	{ -1, -1, 0, 0, T(m2neg), T(s35abs) }
};

static struct insn tabcvtmod[] = {
	{ -1, -1, 0, 0, T(o0neg), T(s36abs) },
};

// for g[] and l[] insns.
static struct insn tabldstm[] = { // we seriously need to unify these, if possible. I wonder if atomics will work with smaller sizes.
	{ -1, -1, 0x0000000000000000ull, 0x00e0000000000000ull, N("u8") },
	{ -1, -1, 0x0020000000000000ull, 0x00e0000000000000ull, N("s8") },
	{ -1, -1, 0x0040000000000000ull, 0x00e0000000000000ull, N("u16") },
	{ -1, -1, 0x0060000000000000ull, 0x00e0000000000000ull, N("s16") },
	{ -1, -1, 0x0080000000000000ull, 0x00e0000000000000ull, N("b64") },
	{ -1, -1, 0x00a0000000000000ull, 0x00e0000000000000ull, N("b128") },
	{ -1, -1, 0x00c0000000000000ull, 0x00e0000000000000ull, N("b32") },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabraadd[] = {
	{ NVA0P, -1, 0x0080000000000000ull, 0x00e0000000000000ull, N("u64") },
	{ -1, -1, 0x00c0000000000000ull, 0x00e0000000000000ull, N("u32") },
	{ -1, -1, 0x00e0000000000000ull, 0x00e0000000000000ull, N("s32") },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabrab[] = {
	{ NVA0P, -1, 0x0080000000000000ull, 0x00e0000000000000ull, N("b64") },
	{ -1, -1, 0x00c0000000000000ull, 0x00e0000000000000ull, N("b32") },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabramm[] = {
	{ -1, -1, 0x00c0000000000000ull, 0x00e0000000000000ull, N("u32") },
	{ -1, -1, 0x00e0000000000000ull, 0x00e0000000000000ull, N("s32") },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabrab32[] = {
	{ -1, -1, 0x00c0000000000000ull, 0x00e0000000000000ull, N("b32") },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabrau32[] = {
	{ -1, -1, 0x00c0000000000000ull, 0x00e0000000000000ull, N("u32") },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabldsto[] = {	// hack alert: reads the bitfield second time.
	{ -1, -1, 0x0000000000000000ull, 0x00e0000000000000ull, LDST },
	{ -1, -1, 0x0020000000000000ull, 0x00e0000000000000ull, LDST },
	{ -1, -1, 0x0040000000000000ull, 0x00e0000000000000ull, LDST },
	{ -1, -1, 0x0060000000000000ull, 0x00e0000000000000ull, LDST },
	{ -1, -1, 0x0080000000000000ull, 0x00e0000000000000ull, LDDST },
	{ -1, -1, 0x00a0000000000000ull, 0x00e0000000000000ull, LQDST },
	{ -1, -1, 0x00c0000000000000ull, 0x00e0000000000000ull, LDST },
	{ -1, -1, 0x00e0000000000000ull, 0x00e0000000000000ull, LDST },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabldsts2[] = {
	{ -1, -1, 0x0080000000000000ull, 0x00e0000000000000ull, LDSRC2 },
	{ -1, -1, 0x00c0000000000000ull, 0x00e0000000000000ull, LSRC2 },
	{ -1, -1, 0x00e0000000000000ull, 0x00e0000000000000ull, LSRC2 },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabldsts3[] = {
	{ -1, -1, 0x0080000000000000ull, 0x00e0000000000000ull, LDSRC3 },
	{ -1, -1, 0x00c0000000000000ull, 0x00e0000000000000ull, LSRC3 },
	{ -1, -1, 0x00e0000000000000ull, 0x00e0000000000000ull, LSRC3 },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabmldsts3[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0000003800000000ull },
	{ -1, -1, 0x0000000800000000ull, 0x0000003c00000000ull, T(ldsts3) },
	{ -1, -1, 0x0000000c00000000ull, 0x0000003c00000000ull },
	{ -1, -1, 0x0000001000000000ull, 0x0000003000000000ull },
	{ -1, -1, 0x0000002000000000ull, 0x0000002000000000ull },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabredm[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0000003c00000000ull, N("add"), T(raadd), },
	{ -1, -1, 0x0000001000000000ull, 0x0000003c00000000ull, N("inc"), T(rau32), },
	{ -1, -1, 0x0000001400000000ull, 0x0000003c00000000ull, N("dec"), T(rau32), },
	{ -1, -1, 0x0000001800000000ull, 0x0000003c00000000ull, N("max"), T(ramm), },
	{ -1, -1, 0x0000001c00000000ull, 0x0000003c00000000ull, N("min"), T(ramm), },
	{ -1, -1, 0x0000002800000000ull, 0x0000003c00000000ull, N("and"), T(rab32), },
	{ -1, -1, 0x0000002c00000000ull, 0x0000003c00000000ull, N("or"), T(rab32), },
	{ -1, -1, 0x0000003000000000ull, 0x0000003c00000000ull, N("xor"), T(rab32), },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabatomm[] = {
	{ -1, -1, 0x0000000400000000ull, 0x0000003c00000000ull, N("exch"), T(rab), },
	{ -1, -1, 0x0000000800000000ull, 0x0000003c00000000ull, N("cas"), T(rab), },
	{ -1, -1, 0, 0, N("ld"), T(redm) },
};

// rounding modes
static struct insn tabcvtrnd[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0006000000000000ull, N("rn") },
	{ -1, -1, 0x0002000000000000ull, 0x0006000000000000ull, N("rm") },
	{ -1, -1, 0x0004000000000000ull, 0x0006000000000000ull, N("rp") },
	{ -1, -1, 0x0006000000000000ull, 0x0006000000000000ull, N("rz") },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabcvtrint[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0006000000000000ull, N("rni") },
	{ -1, -1, 0x0002000000000000ull, 0x0006000000000000ull, N("rmi") },
	{ -1, -1, 0x0004000000000000ull, 0x0006000000000000ull, N("rpi") },
	{ -1, -1, 0x0006000000000000ull, 0x0006000000000000ull, N("rzi") },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabaf64r[] = {
	{ -1, -1, 0x00000000, 0x00030000, N("rn") },
	{ -1, -1, 0x00010000, 0x00030000, N("rm") },
	{ -1, -1, 0x00020000, 0x00030000, N("rp") },
	{ -1, -1, 0x00030000, 0x00030000, N("rz") },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabaf32r[] = {
	{ -1, -1, 0x00000000, 0x00030000, N("rn") },
	{ -1, -1, 0x00030000, 0x00030000, N("rz") },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabmf32r[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0000c00000000000ull, N("rn") },
	{ -1, -1, 0x0000c00000000000ull, 0x0000c00000000000ull, N("rz") },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabmad64r[] = {
	{ -1, -1, 0x0000000000000000ull, 0x00c0000000000000ull, N("rn") },
	{ -1, -1, 0x0040000000000000ull, 0x00c0000000000000ull, N("rm") },
	{ -1, -1, 0x0080000000000000ull, 0x00c0000000000000ull, N("rp") },
	{ -1, -1, 0x00c0000000000000ull, 0x00c0000000000000ull, N("rz") },
	{ -1, -1, 0, 0, OOPS }
};

// ops for set
static struct insn tabseti[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0001c00000000000ull, N("never") },
	{ -1, -1, 0x0000400000000000ull, 0x0001c00000000000ull, N("l") },
	{ -1, -1, 0x0000800000000000ull, 0x0001c00000000000ull, N("e") },
	{ -1, -1, 0x0000c00000000000ull, 0x0001c00000000000ull, N("le") },
	{ -1, -1, 0x0001000000000000ull, 0x0001c00000000000ull, N("g") },
	{ -1, -1, 0x0001400000000000ull, 0x0001c00000000000ull, N("lg") },
	{ -1, -1, 0x0001800000000000ull, 0x0001c00000000000ull, N("ge") },
	{ -1, -1, 0x0001c00000000000ull, 0x0001c00000000000ull, N("always") },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabsetf[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0003c00000000000ull, N("never") },
	{ -1, -1, 0x0000400000000000ull, 0x0003c00000000000ull, N("l") },
	{ -1, -1, 0x0000800000000000ull, 0x0003c00000000000ull, N("e") },
	{ -1, -1, 0x0000c00000000000ull, 0x0003c00000000000ull, N("le") },
	{ -1, -1, 0x0001000000000000ull, 0x0003c00000000000ull, N("g") },
	{ -1, -1, 0x0001400000000000ull, 0x0003c00000000000ull, N("lg") },
	{ -1, -1, 0x0001800000000000ull, 0x0003c00000000000ull, N("ge") },
	{ -1, -1, 0x0001c00000000000ull, 0x0003c00000000000ull, N("lge") },
	{ -1, -1, 0x0002000000000000ull, 0x0003c00000000000ull, N("u") },
	{ -1, -1, 0x0002400000000000ull, 0x0003c00000000000ull, N("lu") },
	{ -1, -1, 0x0002800000000000ull, 0x0003c00000000000ull, N("eu") },
	{ -1, -1, 0x0002c00000000000ull, 0x0003c00000000000ull, N("leu") },
	{ -1, -1, 0x0003000000000000ull, 0x0003c00000000000ull, N("gu") },
	{ -1, -1, 0x0003400000000000ull, 0x0003c00000000000ull, N("lgu") },
	{ -1, -1, 0x0003800000000000ull, 0x0003c00000000000ull, N("geu") },
	{ -1, -1, 0x0003c00000000000ull, 0x0003c00000000000ull, N("always") },
	{ -1, -1, 0, 0, OOPS }
};

// for cvt
static struct insn tabcvtiisrc[] ={
	{ -1, -1, 0x0000000000000000ull, 0x0001c00000000000ull, N("u16"), T(lsh) },
	{ -1, -1, 0x0000400000000000ull, 0x0001c00000000000ull, N("u32"), T(lsw) },
	{ -1, -1, 0x0000800000000000ull, 0x0001c00000000000ull, N("u8"), T(lsh) },
	{ -1, -1, 0x0000c00000000000ull, 0x0001c00000000000ull, N("u8"), LSRC },	// what about mem?
	{ -1, -1, 0x0001000000000000ull, 0x0001c00000000000ull, N("s16"), T(lsh) },
	{ -1, -1, 0x0001400000000000ull, 0x0001c00000000000ull, N("s32"), T(lsw) },
	{ -1, -1, 0x0001800000000000ull, 0x0001c00000000000ull, N("s8"), T(lsh) },
	{ -1, -1, 0x0001c00000000000ull, 0x0001c00000000000ull, N("s8"), LSRC },	// what about mem?
	{ -1, -1, 0, 0, OOPS }
};


// for tex
F1(dtex, 0x23, N("deriv")) // suspected to enable implicit derivatives on non-FPs.
F(ltex, 0x22, N("all"), N("live"))

static struct insn tabtexf[] = {
	{ -1, -1, 0, 0, T(ltex), T(dtex) },
};

// for mov
static struct insn tablane[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0003c00000000000ull, N("lnone") },
	{ -1, -1, 0x0000400000000000ull, 0x0003c00000000000ull, N("l0") },
	{ -1, -1, 0x0000800000000000ull, 0x0003c00000000000ull, N("l1") },
	{ -1, -1, 0x0000c00000000000ull, 0x0003c00000000000ull, N("l01") },
	{ -1, -1, 0x0001000000000000ull, 0x0003c00000000000ull, N("l2") },
	{ -1, -1, 0x0001400000000000ull, 0x0003c00000000000ull, N("l02") },
	{ -1, -1, 0x0001800000000000ull, 0x0003c00000000000ull, N("l12") },
	{ -1, -1, 0x0001c00000000000ull, 0x0003c00000000000ull, N("l012") },
	{ -1, -1, 0x0002000000000000ull, 0x0003c00000000000ull, N("l3") },
	{ -1, -1, 0x0002400000000000ull, 0x0003c00000000000ull, N("l03") },
	{ -1, -1, 0x0002800000000000ull, 0x0003c00000000000ull, N("l13") },
	{ -1, -1, 0x0002c00000000000ull, 0x0003c00000000000ull, N("l013") },
	{ -1, -1, 0x0003000000000000ull, 0x0003c00000000000ull, N("l23") },
	{ -1, -1, 0x0003400000000000ull, 0x0003c00000000000ull, N("l023") },
	{ -1, -1, 0x0003800000000000ull, 0x0003c00000000000ull, N("l123") },
	{ -1, -1, 0x0003c00000000000ull, 0x0003c00000000000ull },
	{ -1, -1, 0, 0, OOPS },
};

// for mov from c[]
static struct insn tabfcon[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0000c00002000000ull, N("u8"), FCONST8 },
	{ -1, -1, 0x0000400000000000ull, 0x0000c00002000000ull, N("u16"), FCONST16 },
	{ -1, -1, 0x0000800000000000ull, 0x0000c00002000000ull, N("s16"), FCONST16 },
	{ -1, -1, 0x0000c00000000000ull, 0x0000c00002000000ull, N("b32"), FCONST32 },
	{ -1, -1, 0x0000000002000000ull, 0x0000c00002000000ull, N("u8"), FCONST8PI },
	{ -1, -1, 0x0000400002000000ull, 0x0000c00002000000ull, N("u16"), FCONST16PI },
	{ -1, -1, 0x0000800002000000ull, 0x0000c00002000000ull, N("s16"), FCONST16PI },
	{ -1, -1, 0x0000c00002000000ull, 0x0000c00002000000ull, N("b32"), FCONST32PI },
	{ -1, -1, 0, 0, OOPS }
};

// for quadop
static struct insn tabqs1[] = {
	{ -1, -1, 0x00000000, 0x00070000, N("l0") },
	{ -1, -1, 0x00010000, 0x00070000, N("l1") },
	{ -1, -1, 0x00020000, 0x00070000, N("l2") },
	{ -1, -1, 0x00030000, 0x00070000, N("l3") },
	{ -1, -1, 0x00040000, 0x00050000, N("dx") },
	{ -1, -1, 0x00050000, 0x00050000, N("dy") },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabqop0[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0c00000000000000ull, N("add") },
	{ -1, -1, 0x0400000000000000ull, 0x0c00000000000000ull, N("subr") },
	{ -1, -1, 0x0800000000000000ull, 0x0c00000000000000ull, N("sub") },
	{ -1, -1, 0x0c00000000000000ull, 0x0c00000000000000ull, N("mov2") },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabqop1[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0300000000000000ull, N("add") },
	{ -1, -1, 0x0100000000000000ull, 0x0300000000000000ull, N("subr") },
	{ -1, -1, 0x0200000000000000ull, 0x0300000000000000ull, N("sub") },
	{ -1, -1, 0x0300000000000000ull, 0x0300000000000000ull, N("mov2") },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabqop2[] = {
	{ -1, -1, 0x0000000000000000ull, 0x00c0000000000000ull, N("add") },
	{ -1, -1, 0x0040000000000000ull, 0x00c0000000000000ull, N("subr") },
	{ -1, -1, 0x0080000000000000ull, 0x00c0000000000000ull, N("sub") },
	{ -1, -1, 0x00c0000000000000ull, 0x00c0000000000000ull, N("mov2") },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabqop3[] = {
	{ -1, -1, 0x00000000, 0x00300000, N("add") },
	{ -1, -1, 0x00100000, 0x00300000, N("subr") },
	{ -1, -1, 0x00200000, 0x00300000, N("sub") },
	{ -1, -1, 0x00300000, 0x00300000, N("mov2") },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tablogop[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0000c00000000000ull, N("and") },
	{ -1, -1, 0x0000400000000000ull, 0x0000c00000000000ull, N("or") },
	{ -1, -1, 0x0000800000000000ull, 0x0000c00000000000ull, N("xor") },
	{ -1, -1, 0x0000c00000000000ull, 0x0000c00000000000ull, N("mov2") },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabaddcond[] = {
	{ -1, -1, 0x00000000, 0x10400000 },
	{ -1, -1, 0x00400000, 0x10400000 },
	{ -1, -1, 0x10000000, 0x10400000 },
	{ -1, -1, 0x10400000, 0x10400000, COND },
	{ -1, -1, 0, 0, OOPS }
};

static struct insn tabaddop2[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0c00000000000000ull, N("add") },
	{ -1, -1, 0x0400000000000000ull, 0x0c00000000000000ull, N("sub") },
	{ -1, -1, 0x0800000000000000ull, 0x0c00000000000000ull, N("subr") },
	{ -1, -1, 0x0c00000000000000ull, 0x0c00000000000000ull, N("addc") },
	{ -1, -1, 0, 0, OOPS },
};

static struct insn tabaddcond2[] = {
	{ -1, -1, 0x0000000000000000ull, 0x0c00000000000000ull },
	{ -1, -1, 0x0400000000000000ull, 0x0c00000000000000ull },
	{ -1, -1, 0x0800000000000000ull, 0x0c00000000000000ull },
	{ -1, -1, 0x0c00000000000000ull, 0x0c00000000000000ull, COND },
	{ -1, -1, 0, 0, OOPS }
};

F(sstreg, 0x35, LHSRC3, LSRC3)
F1V(unlock, NVA0P, 0x37, N("unlock"))
F(csldreg, 0x3a, LLHDST, LLDST)

static struct insn tabl[] = {
	// 0
	// desc VVV
	{ -1, VP|GP, 0x0420000000000000ull, 0xe4200000f0000000ull,
		T(lane), N("ld"), N("b32"), LLDST, FATTR },
	// desc ^^^
	{ -1, -1, 0x2000000000000000ull, 0xe0000000f0000000ull,
		N("mov"), LDST, COND },
	{ -1, -1, 0x4000000000000000ull, 0xe0000000f0000000ull,
		N("mov"), LDST, LAREG },
	{ -1, -1, 0x6000000000000000ull, 0xe0000000f0000000ull,
		N("mov"), LDST, SREG },

	{ -1, -1, 0xa000000000000000ull, 0xe0000000f0000000ull,
		N("mov"), CDST, LSRC, IGNCE },
	{ -1, -1, 0xc000000000000000ull, 0xe0000000f0000000ull,
		N("shl"), ADST, T(lsw), HSHCNT },
	
	// desc VVV
	{ -1, CP, 0xe040000000000000ull, 0xe0400000f0000000ull,
		N("st"), N("b8"), T(fs8), T(sstreg) },
	{ -1, CP, 0xe000000000000000ull, 0xe4400000f0000000ull,
		N("st"), N("b16"), T(fs16), T(sstreg) },
	{ -1, CP, 0xe400000000000000ull, 0xe4400000f0000000ull,
		N("st"), T(unlock), N("b32"), T(fs32), T(sstreg) },
	// desc ^^^

	// 1
	{ -1, -1, 0x0000000010000000ull, 0xe4000000f0000000ull,
		T(lane), T(lplease), N("mov"), N("b16"), LLHDST, T(lsh) },
	{ -1, -1, 0x0400000010000000ull, 0xe4000000f0000000ull,
		T(lane), T(lplease), N("mov"), N("b32"), LLDST, T(lsw) },

	// desc VVV
	{ -1, -1, 0x2000000010000000ull, 0xe0000000f0000000ull,
		N("ld"), T(csldreg), T(fcon) },

	{ NV84P, CP, 0x4000000010000000ull, 0xe000c000f0000000ull,
		N("ld"), T(csldreg), N("u8"), T(fs8) },
	{ NV84P, CP, 0x4000400010000000ull, 0xe000c000f0000000ull,
		N("ld"), T(csldreg), N("u16"), T(fs16) },
	{ NV84P, CP, 0x4000800010000000ull, 0xe000c000f0000000ull,
		N("ld"), T(csldreg), N("s16"), T(fs16) },
	{ NV84P, CP, 0x4000c00010000000ull, 0xe080c000f0000000ull,
		N("ld"), T(csldreg), N("b32"), T(fs32) },
	{ NVA0P, CP, 0x4080c04010000000ull, 0xe080c040f0000000ull,
		N("ld"), N("lock"), CDST, T(csldreg), N("b32"), T(fs32) },

	{ NVA0P, -1, 0x6000000010000200ull, 0xe0000000f0000600ull,
		N("vote any"), CDST, IGNCE },
	{ NVA0P, -1, 0x6000000010000400ull, 0xe0000000f0000600ull,
		N("vote all"), CDST, IGNCE },
	// desc ^^^

	// 2 and 3
	{ -1, -1, 0x0000000020000000ull, 0xe4000000e0000000ull, T(addop), N("b16"), T(m2sat), MCDST, LLHDST, T(lsh), T(lc3h), T(addcond) },
	{ -1, -1, 0x0400000020000000ull, 0xe4000000e0000000ull, T(addop), N("b32"), T(m2sat), MCDST, LLDST, T(lsw), T(lc3w), T(addcond) },

	{ -1, -1, 0x6000000030000000ull, 0xe4000000f0000000ull,
		N("set"), MCDST, LLHDST, T(seti), T(lm2us16), T(lsh), T(lc2h) },
	{ -1, -1, 0x6400000030000000ull, 0xe4000000f0000000ull,
		N("set"), MCDST, LLDST, T(seti), T(lm2us32), T(lsw), T(lc2w) },

	{ -1, -1, 0x8000000030000000ull, 0xe4000000f0000000ull,
		N("max"), T(lm2us16), MCDST, LLHDST, T(lsh), T(lc2h) },
	{ -1, -1, 0x8400000030000000ull, 0xe4000000f0000000ull,
		N("max"), T(lm2us32), MCDST, LLDST, T(lsw), T(lc2w) },

	{ -1, -1, 0xa000000030000000ull, 0xe4000000f0000000ull,
		N("min"), T(lm2us16), MCDST, LLHDST, T(lsh), T(lc2h) },
	{ -1, -1, 0xa400000030000000ull, 0xe4000000f0000000ull,
		N("min"), T(lm2us32), MCDST, LLDST, T(lsw), T(lc2w) },

	{ -1, -1, 0xc000000030000000ull, 0xe4000000f0000000ull,
		N("shl"), N("b16"), MCDST, LLHDST, T(lsh), T(hshcnt) },
	{ -1, -1, 0xc400000030000000ull, 0xe4000000f0000000ull,
		N("shl"), N("b32"), MCDST, LLDST, T(lsw), T(shcnt) },

	{ -1, -1, 0xe000000030000000ull, 0xe4000000f0000000ull,
		N("shr"), T(lm2us16), MCDST, LLHDST, T(lsh), T(hshcnt) },
	{ -1, -1, 0xe400000030000000ull, 0xe4000000f0000000ull,
		N("shr"), T(lm2us32), MCDST, LLDST, T(lsw), T(shcnt) },

	// 4
	{ -1, -1, 0x0000000040000000ull, 0xe0010000f0000000ull,
		N("mul"), MCDST, LLDST, T(s31us16), T(lsh), T(s30us16), T(lc2h) },
	{ -1, -1, 0x0001000040000000ull, 0xe0010000f0000000ull,
		N("mul"), MCDST, LLDST, T(s30high), T(s31us24), T(lsw), T(lc2w) },

	// 5
	{ -1, -1, 0x0000000050000000ull, 0xe4000000f0000000ull,
		N("sad"), MCDST, LLDST, T(lm2us16), T(lsh), T(lc2h), T(lc3w) },
	{ -1, -1, 0x0400000050000000ull, 0xe4000000f0000000ull,
		N("sad"), MCDST, LLDST, T(lm2us32), T(lsw), T(lc2w), T(lc3w) },

	// 6 and 7
	{ -1, -1, 0x0000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), MCDST, LLDST, N("mul"), N("u16"), T(lsh), T(lc2h), T(lc3w), T(addcond2) },
	{ -1, -1, 0x2000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), MCDST, LLDST, N("mul"), N("s16"), T(lsh), T(lc2h), T(lc3w), T(addcond2) },
	{ -1, -1, 0x4000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), N("sat"), MCDST, LLDST, N("mul"), N("s16"), T(lsh), T(lc2h), T(lc3w), T(addcond2) },
	{ -1, -1, 0x6000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), MCDST, LLDST, N("mul"), N("u24"), T(lsw), T(lc2w), T(lc3w), T(addcond2) },
	{ -1, -1, 0x8000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), MCDST, LLDST, N("mul"), N("s24"), T(lsw), T(lc2w), T(lc3w), T(addcond2) },
	{ -1, -1, 0xa000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), N("sat"), MCDST, LLDST, N("mul"), N("s24"), T(lsw), T(lc2w), T(lc3w), T(addcond2) },
	{ -1, -1, 0xc000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), MCDST, LLDST, N("mul"), N("high"), N("u24"), T(lsw), T(lc2w), T(lc3w), T(addcond2) },
	{ -1, -1, 0xe000000060000000ull, 0xe0000000f0000000ull,
		T(addop2), MCDST, LLDST, N("mul"), N("high"), N("s24"), T(lsw), T(lc2w), T(lc3w), T(addcond2) },
	{ -1, -1, 0x0000000070000000ull, 0xe0000000f0000000ull,
		T(addop2), N("sat"), MCDST, LLDST, N("mul"), N("high"), N("s24"), T(lsw), T(lc2w), T(lc3w), T(addcond2) },

	// desc VVV
	// 8
	{ -1, FP, 0x0000000080000000ull, 0x00070000f0000000ull,
		N("interp"), LDST, LVAR },
	{ -1, FP, 0x0001000080000000ull, 0x00070000f0000000ull,
		N("interp"), LDST, N("cent"), LVAR },
	{ -1, FP, 0x0002000080000000ull, 0x00070000f0000000ull,
		N("interp"), LDST, LVAR, LSRC },
	{ -1, FP, 0x0003000080000000ull, 0x00070000f0000000ull,
		N("interp"), LDST, N("cent"), LVAR, LSRC },
	{ -1, FP, 0x0004000080000000ull, 0x00070000f0000000ull,
		N("interp"), LDST, N("flat"), LVAR },

	// 9
	{ -1, -1, 0x0000000090000000ull, 0xe0000000f0000000ull,
		N("rcp f32"), LLDST, T(lfm1), LSRC },
	{ -1, -1, 0x4000000090000000ull, 0xe0000000f0000000ull,
		N("rsqrt f32"), LLDST, T(lfm1), LSRC },
	{ -1, -1, 0x6000000090000000ull, 0xe0000000f0000000ull,
		N("lg2 f32"), LLDST, T(lfm1), LSRC },
	{ -1, -1, 0x8000000090000000ull, 0xe0000000f0000000ull,
		N("sin f32"), LLDST, LSRC },
	{ -1, -1, 0xa000000090000000ull, 0xe0000000f0000000ull,
		N("cos f32"), LLDST, LSRC },
	{ -1, -1, 0xc000000090000000ull, 0xe0000000f0000000ull,
		N("ex2 f32"), LLDST, LSRC },

	// a
	{ -1, -1, 0xc0000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), N("f16"), MCDST, LLHDST, N("f16"), T(lsh) },
	{ -1, -1, 0xc8000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrint), N("f16"), MCDST, LLHDST, N("f16"), T(lsh) },
	{ -1, -1, 0xc0004000a0000000ull, 0xc4404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f16"), MCDST, LLHDST, N("f32"), T(lsw) },
	{ -1, -1, 0xc4004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), N("f32"), MCDST, LLDST, N("f32"), T(lsw) },
	{ -1, -1, 0xcc004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrint), N("f32"), MCDST, LLDST, N("f32"), T(lsw) },
	{ -1, -1, 0xc4000000a0000000ull, 0xc4404000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), N("f32"), MCDST, LLDST, N("f16"), T(lsh) },

	{ NVA0, -1, 0xc0404000a0000000ull, 0xc4404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrnd), N("f32"), MCDST, LLDST, N("f64"), LDSRC },
	{ NVA0, -1, 0xc4404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), N("f64"), MCDST, LDDST, N("f64"), LDSRC },
	{ NVA0, -1, 0xcc404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("f64"), MCDST, LDDST, N("f64"), LDSRC },
	{ NVA0, -1, 0xc4400000a0000000ull, 0xc4404000f0000000ull,
		N("cvt"), T(cvtmod), N("f64"), MCDST, LDDST, N("f32"), T(lsw) },

	{ -1, -1, 0x80000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u16"), MCDST, LLHDST, N("f16"), T(lsh) },
	{ -1, -1, 0x80004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u16"), MCDST, LLHDST, N("f32"), T(lsw) },
	{ -1, -1, 0x88000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s16"), MCDST, LLHDST, N("f16"), T(lsh) },
	{ -1, -1, 0x88004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s16"), MCDST, LLHDST, N("f32"), T(lsw) },
	{ -1, -1, 0x84000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u32"), MCDST, LLDST, N("f16"), T(lsh) },
	{ -1, -1, 0x84004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u32"), MCDST, LLDST, N("f32"), T(lsw) },
	{ -1, -1, 0x8c000000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s32"), MCDST, LLDST, N("f16"), T(lsh) },
	{ -1, -1, 0x8c004000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s32"), MCDST, LLDST, N("f32"), T(lsw) },

	{ NVA0, -1, 0x80404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u32"), MCDST, LLDST, N("f64"), LDSRC },
	{ NVA0, -1, 0x88404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s32"), MCDST, LLDST, N("f64"), LDSRC },
	{ NVA0, -1, 0x84400000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u64"), MCDST, LDDST, N("f32"), T(lsw) },
	{ NVA0, -1, 0x84404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("u64"), MCDST, LDDST, N("f64"), LDSRC },
	{ NVA0, -1, 0x8c400000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s64"), MCDST, LDDST, N("f32"), T(lsw) },
	{ NVA0, -1, 0x8c404000a0000000ull, 0xcc404000f0000000ull,
		N("cvt"), T(cvtmod), T(cvtrint), N("s64"), MCDST, LDDST, N("f64"), LDSRC },

	{ -1, -1, 0x40000000a0000000ull, 0xc4400000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f16"), MCDST, LLHDST, T(cvtiisrc) },
	{ -1, -1, 0x44000000a0000000ull, 0xc4400000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f32"), MCDST, LLDST, T(cvtiisrc) },

	{ NVA0, -1, 0x44400000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f64"), MCDST, LDDST, N("u32"), T(lsw) },
	{ NVA0, -1, 0x44410000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f64"), MCDST, LDDST, N("s32"), T(lsw) },
	{ NVA0, -1, 0x40404000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f32"), MCDST, LLDST, N("u64"), LDSRC },
	{ NVA0, -1, 0x40414000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f32"), MCDST, LLDST, N("s64"), LDSRC },
	{ NVA0, -1, 0x44404000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f64"), MCDST, LDDST, N("u64"), LDSRC },
	{ NVA0, -1, 0x44414000a0000000ull, 0xc4414000f0000000ull,
		N("cvt"), T(cvtmod), T(s35sat), T(cvtrnd), N("f64"), MCDST, LDDST, N("s64"), LDSRC },

	{ -1, -1, 0x00000000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("u16"), MCDST, LLHDST, T(cvtiisrc) },
	{ -1, -1, 0x00080000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("u8"), MCDST, LLHDST, T(cvtiisrc) },
	{ -1, -1, 0x04000000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("u32"), MCDST, LLDST, T(cvtiisrc) },
	{ -1, -1, 0x04080000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("u8"), MCDST, LLDST, T(cvtiisrc) },
	{ -1, -1, 0x08000000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("s16"), MCDST, LLHDST, T(cvtiisrc) },
	{ -1, -1, 0x08080000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("s8"), MCDST, LLHDST, T(cvtiisrc) },
	{ -1, -1, 0x0c000000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("s32"), MCDST, LLDST, T(cvtiisrc) },
	{ -1, -1, 0x0c080000a0000000ull, 0xcc080000f0000000ull,
		N("cvt"), T(cvtmod), N("s8"), MCDST, LLDST, T(cvtiisrc) },


	// b
	{ -1, -1, 0x00000000b0000000ull, 0xc0000000f0000000ull,
		N("add"), T(o0sat), T(af32r), N("f32"), MCDST, LLDST, T(m1neg), T(lsw), T(m2neg), T(lc3w) },

	{ -1, -1, 0x60000000b0000000ull, 0xe0000000f0000000ull,
		N("set"), MCDST, LLDST, T(setf), N("f32"), T(lfm1), T(lsw), T(lfm2), T(lc2w) },

	{ -1, -1, 0x80000000b0000000ull, 0xe0000000f0000000ull,
		N("max"), N("f32"), MCDST, LLDST, T(lfm1), T(lsw), T(lfm2), T(lc2w) },

	{ -1, -1, 0xa0000000b0000000ull, 0xe0000000f0000000ull,
		N("min"), N("f32"), MCDST, LLDST, T(lfm1), T(lsw), T(lfm2), T(lc2w) },

	{ -1, -1, 0xc0000000b0000000ull, 0xe0004000f0000000ull,
		N("presin f32"), LLDST, T(lfm1), T(lsw) },
	{ -1, -1, 0xc0004000b0000000ull, 0xe0004000f0000000ull,
		N("preex2 f32"), LLDST, T(lfm1), T(lsw) },
	/* preex2 converts float to fixed point, results:
	 * 0-0x3fffffff: 7.23 fixed-point number
	 * 0x40000000: +nan
	 * 0x40800000: +infinity
	 * flip bit 0x80000000 in any of the above for negative numbers.
	 * presin divides by pi/2, mods with 4 [or with 2*pi, pre-div], then does preex2
	 */

	// c
	{ -1, -1, 0x00000000c0000000ull, 0xe0000000f0000000ull,
		N("mul"), T(mf32r), N("f32"), MCDST, LLDST, T(m1neg), T(lsw), T(m2neg), T(lc2w) },

	{ -1, -1, 0x40000000c0000000ull, 0xc0000000f0000000ull,
		N("slct"), N("b32"), MCDST, LLDST, T(lsw), T(lc2w), N("f32"), T(o0neg), T(lc3w) },

	{ -1, -1, 0x80000000c0000000ull, 0xf0000000f0000000ull,
		N("quadop f32"), T(qop0), T(qop1), T(qop2), T(qop3), MCDST, LLDST, T(qs1), LSRC, LSRC3 },
	// desc ^^^

	// d
	{ -1, -1, 0x00000000d0000000ull, 0xe4000000f0000000ull,
		T(logop), N("b16"), MCDST, LLHDST, T(s32not), T(lsh), T(s33not), T(lc2h) },
	{ -1, -1, 0x04000000d0000000ull, 0xe4000000f0000000ull,
		T(logop), N("b32"), MCDST, LLDST, T(s32not), T(lsw), T(s33not), T(lc2w) },

	{ -1, -1, 0x20000000d0000000ull, 0xe0000000f0000000ull,
		N("add"), ADST, LAREG, OFFS },

	// desc VVV
	{ -1, -1, 0x40000000d0000000ull, 0xe0000000f0000000ull,
		N("ld"), T(ldstm), T(ldsto), T(local) },
	{ -1, -1, 0x60000000d0000000ull, 0xe0000000f0000000ull,
		N("st"), T(ldstm), T(local), T(ldsto) },

	{ -1, CP, 0x80000000d0000000ull, 0xe0000000f0000000ull,
		N("ld"), T(ldstm), T(ldsto), GLOBAL },
	{ -1, CP, 0xa0000000d0000000ull, 0xe0000000f0000000ull,
		N("st"), T(ldstm), GLOBAL, T(ldsto) },
	{ NV84P, CP, 0xc0000000d0000000ull, 0xe0000000f0000000ull,
		T(redm), GLOBAL, T(ldsto) },
	{ NV84P, CP, 0xe0000000d0000000ull, 0xe0000000f0000000ull,
		T(atomm), T(ldsto), GLOBAL2, T(ldsts2), T(mldsts3) },

	// e
	{ -1, -1, 0x00000000e0000000ull, 0xc0000000f0000000ull,
		N("add"), T(o0sat), N("f32"), MCDST, LLDST, T(m1neg), N("mul"), T(lsw), T(lc2w), T(m2neg), T(lc3w) },	// multiply, round, add, round. meh.

	{ NVA0, -1, 0x40000000e0000000ull, 0xe0000000f0000000ull,
		N("fma"), T(mad64r), N("f64"), MCDST, LDDST, T(m1neg), LDSRC, LDSRC2, T(m2neg), LDSRC3 },	// *fused* multiply-add, no intermediate rounding :)
	{ NVA0, -1, 0x60000000e0000000ull, 0xe0000000f0000000ull,
		N("add"), T(af64r), N("f64"), MCDST, LDDST, T(m1neg), LDSRC, T(m2neg), LDSRC3 },
	{ NVA0, -1, 0x80000000e0000000ull, 0xe0000000f0000000ull,
		N("mul"), T(cvtrnd), N("f64"), MCDST, LDDST, T(m1neg), LDSRC, LDSRC2 },
	{ NVA0, -1, 0xa0000000e0000000ull, 0xe0000000f0000000ull,
		N("min"), N("f64"), MCDST, LDDST, T(lfm1), LDSRC, T(lfm2), LDSRC2 },
	{ NVA0, -1, 0xc0000000e0000000ull, 0xe0000000f0000000ull,
		N("max"), N("f64"), MCDST, LDDST, T(lfm1), LDSRC, T(lfm2), LDSRC2 },
	{ NVA0, -1, 0xe0000000e0000000ull, 0xe0000000f0000000ull,
		N("set"), MCDST, LLDST, T(setf), N("f64"), T(lfm1), LDSRC, T(lfm2), LDSRC2 },

	// f
	{ -1, -1, 0x00000000f0000000ull, 0xf0000000f9000000ull, // order of inputs: x, y, z, index, dref, bias/lod. index is integer, others float.
		N("texauto"), T(texf), LTDST, TEX, SAMP, LTSRC, TOFFX, TOFFY, TOFFZ },
	{ -1, -1, 0x00000000f8000000ull, 0xf0000000f9000000ull,
		N("texauto cube"), T(texf), LTDST, TEX, SAMP, LTSRC },

	{ -1, -1, 0x00000000f1000000ull, 0xf0000000f1000000ull, // takes integer inputs.
		N("texfetch"), T(texf), LTDST, TEX, SAMP, LTSRC, TOFFX, TOFFY, TOFFZ },

	{ -1, -1, 0x20000000f0000000ull, 0xf0000000f8000000ull, // bias needs to be same for everything, or else.
		N("texbias"), T(texf), LTDST, TEX, SAMP, LTSRC, TOFFX, TOFFY, TOFFZ },
	{ -1, -1, 0x20000000f8000000ull, 0xf0000000f8000000ull,
		N("texbias cube"), T(texf), LTDST, TEX, SAMP, LTSRC },

	{ -1, -1, 0x40000000f0000000ull, 0xf0000000f8000000ull, // lod needs to be same for everything, or else.
		N("texlod"), T(texf), LTDST, TEX, SAMP, LTSRC, TOFFX, TOFFY, TOFFZ },
	{ -1, -1, 0x40000000f8000000ull, 0xf0000000f8000000ull,
		N("texlod cube"), T(texf), LTDST, TEX, SAMP, LTSRC },

	{ -1, -1, 0x60000000f0000000ull, 0xf00f0000f0000000ull, // integer input and output.
		N("texsize"), T(texf), LTDST, TEX, SAMP, LDST }, // in: LOD, out: size.x, size.y, size.z
	{ NVA3P, -1, 0x60010000f8000000ull, 0xf00f0000f8000000ull, // input: 3 normalized cube coords [float], layer [int]; output: equivalent x, y, combined layer coords to pass to non-cube tex variants.
		N("texprep cube"), T(texf), LTDST, TEX, SAMP, LTSRC },
	{ NVA3P, -1, 0x60020000f0000000ull, 0xf00f0000f8000000ull, // returned values are FIXED-point with 6 fractional bits
		N("texquerylod"), T(texf), LTDST, TEX, SAMP, LTSRC },
	{ NVA3P, -1, 0x60020000f8000000ull, 0xf00f0000f8000000ull,
		N("texquerylod cube"), T(texf), LTDST, TEX, SAMP, LTSRC },

	{ -1, -1, 0x80000000f0000000ull, 0xf0000000f1000000ull, // no idea what this is. but it *is* texturing.
		U("f/8/0"), T(texf), LTDST, TEX, SAMP, LTSRC, TOFFX, TOFFY, TOFFZ },

	{ NVA3P, -1, 0x80000000f1000000ull, 0xf0000000f9000000ull,
		N("texgather"), T(texf), LTDST, TEX, SAMP, LTSRC, TOFFX, TOFFY, TOFFZ },
	{ NVA3P, -1, 0x80000000f9000000ull, 0xf0000000f9000000ull,
		N("texgather cube"), T(texf), LTDST, TEX, SAMP, LTSRC },

	{ -1, GP, 0xc0000000f0000200ull, 0xe0000000f0000600ull, N("emit") },
	{ -1, GP, 0xc0000000f0000400ull, 0xe0000000f0000600ull, N("restart") },

	{ -1, -1, 0xe0000000f0000000ull, 0xe0000004f0000000ull, N("nop") },
	{ -1, -1, 0xe0000004f0000000ull, 0xe0000004f0000000ull, N("pmevent"), PM },

	// desc ^^^
	
	// try to print out *some* info.
	{ -1, -1, 0, 0, OOPS, MCDST, LLDST, T(lsw), T(lc2w), T(lc3w) },

};

/*
 * Predicates
 */
static struct insn tabp[] = {
	{ -1, -1, 0x0000000000000000ull, 0x00000f8000000000ull, N("never") },
	{ -1, -1, 0x0000008000000000ull, 0x00000f8000000000ull, N("l"), COND },
	{ -1, -1, 0x0000010000000000ull, 0x00000f8000000000ull, N("e"), COND },
	{ -1, -1, 0x0000018000000000ull, 0x00000f8000000000ull, N("le"), COND },
	{ -1, -1, 0x0000020000000000ull, 0x00000f8000000000ull, N("g"), COND },
	{ -1, -1, 0x0000028000000000ull, 0x00000f8000000000ull, N("lg"), COND },
	{ -1, -1, 0x0000030000000000ull, 0x00000f8000000000ull, N("ge"), COND },
	{ -1, -1, 0x0000038000000000ull, 0x00000f8000000000ull, N("lge"), COND },
	{ -1, -1, 0x0000040000000000ull, 0x00000f8000000000ull, N("u"), COND },
	{ -1, -1, 0x0000048000000000ull, 0x00000f8000000000ull, N("lu"), COND },
	{ -1, -1, 0x0000050000000000ull, 0x00000f8000000000ull, N("eu"), COND },
	{ -1, -1, 0x0000058000000000ull, 0x00000f8000000000ull, N("leu"), COND },
	{ -1, -1, 0x0000060000000000ull, 0x00000f8000000000ull, N("gu"), COND },
	{ -1, -1, 0x0000068000000000ull, 0x00000f8000000000ull, N("lgu"), COND },
	{ -1, -1, 0x0000070000000000ull, 0x00000f8000000000ull, N("geu"), COND },
	{ -1, -1, 0x0000078000000000ull, 0x00000f8000000000ull },
	{ -1, -1, 0x0000080000000000ull, 0x00000f8000000000ull, N("o"), COND },
	{ -1, -1, 0x0000088000000000ull, 0x00000f8000000000ull, N("c"), COND },
	{ -1, -1, 0x0000090000000000ull, 0x00000f8000000000ull, N("a"), COND },
	{ -1, -1, 0x0000098000000000ull, 0x00000f8000000000ull, N("s"), COND },
	{ -1, -1, 0x00000e0000000000ull, 0x00000f8000000000ull, N("ns"), COND },
	{ -1, -1, 0x00000e8000000000ull, 0x00000f8000000000ull, N("na"), COND },
	{ -1, -1, 0x00000f0000000000ull, 0x00000f8000000000ull, N("nc"), COND },
	{ -1, -1, 0x00000f8000000000ull, 0x00000f8000000000ull, N("no"), COND },
	{ -1, -1, 0, 0, OOPS },
};

F1(lim, 0x26, N("lim")) // if set, call is limited, and will be ignored if X limited calls already active on the stack. X is settable by CALL_LIMIT_LOG method.

static struct insn tabc[] = {
	{ -1, FP, 0x00000000, 0xf0000000, T(p), N("discard") },
	{ -1, -1, 0x10000000, 0xf0000000, T(p), N("bra"), BTARG },
	{ -1, -1, 0x20000000, 0xf0000000, IGNPRED, N("call"), T(lim), CTARG },
	{ -1, -1, 0x30000000, 0xf0000000, T(p), N("ret") },
	{ -1, -1, 0x40000000, 0xf0000000, IGNPRED, N("breakaddr"), BTARG },
	{ -1, -1, 0x50000000, 0xf0000000, T(p), N("break") },
	{ -1, -1, 0x60000000, 0xf0000000, IGNPRED, N("quadon") },
	{ -1, -1, 0x70000000, 0xf0000000, IGNPRED, N("quadpop") },
	{ -1, -1, 0x861ffe00, 0xf61ffe00, IGNPRED, N("bar"), N("sync"), BAR },
	{ -1, -1, 0x90000000, 0xf0000000, IGNPRED, N("trap") },
	{ -1, -1, 0xa0000000, 0xf0000000, IGNPRED, N("joinat"), BTARG },
	{ NV84P, -1, 0xb0000000, 0xf0000000, T(p), N("brkpt") },
	{ -1, -1, 0, 0, T(p), OOPS, BTARG },
};

static struct insn tabroot[] = {
	{ -1, -1,         0x00000000ull,         0x00000001ull, OP32, T(s) },
	{ -1, -1, 0x0000000000000003ull, 0x0000000000000003ull, OP64, T(c) },
	{ -1, -1, 0x0000000000000001ull, 0x0000000300000003ull, OP64, T(p), T(l) },
	{ -1, -1, 0x0000000100000001ull, 0x0000000300000003ull, OP64, N("exit"), T(p), T(l) },
	{ -1, -1, 0x0000000200000001ull, 0x0000000300000003ull, OP64, N("join"), T(p), T(l) },
	{ -1, -1, 0x0000000300000001ull, 0x0000000300000003ull, OP64, T(i) },
	{ -1, -1, 0, 0, OOPS },
};

static struct disisa nv50_isa_s = {
	tabroot,
	8,
	4,
	1,
	.i_need_nv50as_hack = 1,
};

struct disisa *nv50_isa = &nv50_isa_s;
