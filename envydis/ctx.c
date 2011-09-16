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

#include "dis-intern.h"

#define NV40 1
#define NV50 2

/*
 * PGRAPH registers of interest, NV50
 *
 * 0x400040: disables subunits of PGRAPH. or at least disables access to them
 *           through MMIO. each bit corresponds to a unit, and the full mask
 *           is 0x7fffff. writing 1 to a given bit disables the unit, 0
 *           enables.
 * 0x400300: some sort of status register... shows 0xe when execution stopped,
 *           0xX3 when running or paused.
 * 0x400304: control register. For writes:
 *           bit 0: start execution
 *           bit 1: resume execution
 *           bit 4: pause execution
 *           For reads:
 *           bit 0: prog running or paused
 * 0x400308: Next opcode to execute, or the exit opcode if program exitted.
 * 0x40030c: RO alias of PC for some reason
 * 0x400310: PC. Readable and writable. In units of single insns.
 * 0x40031c: the scratch register, written by opcode 2, holding params to
 *           various stuff.
 * 0x400320: WO, CMD register. Writing here launches a cmd, see below.
 * 0x400324: code upload index, WO. selects address to write in ctxprog code,
 *           counted in whole insns.
 * 0x400328: code upload, WO. writes given insn to code segment and
 *           autoincrements upload address.
 * 0x40032c: current channel RAMIN address, shifted right by 12 bits. bit 31 == valid
 * 0x400330: next channel RAMIN address, shifted right by 12 bits. bit 31 == valid
 * 0x400334: $r register, offset in RAMIN context to read/write with opcode 1.
 *           in units of 32-bit words.
 * 0x400338: Some register, set with opcode 3. Used on NVAx with values 0-9 to
 *           select TP number to read/write with G[0x8000]. Maybe generic
 *           PGRAPH offset? Also set to 0x8ffff in one place for some reason.
 * 0x40033c: Some other register. Set with opcode 0x600007. Related to and
 *           modified by opcode 8. Setting with opcode 0x600007 always ands
 *           contents with 0xffff8 for some reason.
 * 0x400784: channel RAMIN address used for memory reads/writes. needs to be
 *           flushed with CMD 4 after it's changed.
 * 0x400824: Flags, 0x00-0x1f, RW
 * 0x400828: Flags, 0x20-0x3f, RW
 * 0x40082c: Flags, 0x40-0x5f, RO... this is some sort of hardware status
 * 0x400830: Flags, 0x60-0x7f, RW
 *
 * How execution works:
 *
 * Execution can be in one of 3 states: stopped, running, and paused.
 * Initially, it's stopped. The only way to start execution is to poke 0x400304
 * with bit 0 set. When you do this, the uc starts executing opcodes from
 * current PC. The only way to stop execution is to use CMD 0xc.
 * Note that CMD 0xc resets PC to 0, so ctxprog will start over from there
 * next time, unless you poke PC before that. Running <-> paused transition
 * happens by poking 0x400304 with the appropriate bit set to 1. Poking with
 * *both* bits set to 1 causes it to reload the opcode register from current
 * PC, but nothing else happens. This can be used to read the code segment.
 * Poking with bits 4 and 0 set together causes it to start with paused state.
 *
 * Oh, also, PC has 9 valid bits. So you can have ctxprogs of max. 512 insns.
 *
 * CMD: These are some special operations that can be launched either from
 * host by poking the CMD number to 0x400320, or by ctxprog by running opcode
 * 6 with the CMD number in its immediate field. See tabcmd.
 *
 */

/*
 * All of the above has been checked on my NV86 laptop card and on my NVA5.
 * Should probably apply to all NV50 family cards.
 */

/*
 * PGRAPH registers of interest, NV40
 *
 * 0x400040: disables subunits of PGRAPH. or at least disables access to them
 *           through MMIO. each bit corresponds to a unit, and the full mask
 *           is 0x7fffff. writing 1 to a given bit disables the unit, 0
 *           enables.
 * 0x400300: some sort of status register... shows 0xe when execution stopped,
 *           0xX3 when running or paused.
 * 0x400304: control register. Writing 1 startx prog, writing 2 kills it.
 * 0x400308: Current PC+opcode. PC is in high 8 bits, opcode is in low 24 bits.
 * 0x400310: Flags 0-0xf, RW
 * 0x400314: Flags 0x20-0x2f, RW
 * 0x400318: Flags 0x60-0x6f, RO, hardware status
 * 0x40031c: the scratch register, written by opcode 2, holding params to
 *           various stuff.
 * 0x400324: code upload index, RW. selects address to write in ctxprog code,
 *           counted in whole insns.
 * 0x400328: code upload. writes given insn to code segment and
 *           autoincrements upload address. RW, but returns something crazy on
 *           read
 * 0x40032c: current channel RAMIN address, shifted right by 12 bits. bit 24 == valid
 * 0x400330: next channel RAMIN address, shifted right by 12 bits. bit 24 == valid
 * 0x400338: Some register, set with opcode 3
 * 0x400784: channel RAMIN address used for memory reads/writes. needs to be
 *           flushed with CMD 4 after it's changed.
 *
 */


/*
 * Code target field
 *
 * This field represents a code address and is used for branching and the
 * likes. Target is counted in 32-bit words from the start of microcode.
 */

static struct bitfield ctargoff = { 8, 9 };
#define BTARG atombtarg, &ctargoff
#define CTARG atomctarg, &ctargoff

/* Register fields */

static struct reg sr_r = { 0, "sr" };
#define SR atomreg, &sr_r

/*
 * Misc number fields
 *
 * Used for plain numerical arguments.
 */

static struct bitfield unitoff = { 0, 5 };
static struct bitfield flagoff = { 0, 7 };
static struct bitfield gsize4off = { 14, 6 };
static struct bitfield gsize5off = { 16, 4 };
static struct bitfield immoff = { 0, 20 };
static struct bitfield dis0off = { 0, 16 };
static struct bitfield dis1off = { { 0, 16 }, BF_UNSIGNED, 16 };
static struct bitfield seekpoff = { 8, 11 };
static struct bitfield cmdoff = { 0, 5 };
#define UNIT atomimm, &unitoff
#define FLAG atomimm, &flagoff
#define GSIZE4 atomimm, &gsize4off
#define GSIZE5 atomimm, &gsize5off
#define IMM atomimm, &immoff
#define DIS0 atomimm, &dis0off
#define DIS1 atomimm, &dis1off
#define SEEKP atomimm, &seekpoff
#define CMD atomimm, &cmdoff

/*
 * Memory fields
 */

// BF, offset shift, 'l'

static struct bitfield pgmem4_imm = { { 0, 14 }, BF_UNSIGNED, 2 };
static struct bitfield pgmem5_imm = { { 0, 16 }, BF_UNSIGNED, 2 };
static struct mem pgmem4_m = { "G", 0, 0, &pgmem4_imm };
static struct mem pgmem5_m = { "G", 0, 0, &pgmem5_imm };
#define PGRAPH4 atommem, &pgmem4_m
#define PGRAPH5 atommem, &pgmem5_m

F1(p0, 0, N("a0"))
F1(p1, 1, N("a1"))
F1(p2, 2, N("a2"))
F1(p3, 3, N("a3"))
F1(p4, 4, N("a4"))
F1(p5, 5, N("a5"))
F1(p6, 6, N("a6"))
F1(p7, 7, N("a7"))

static struct insn tabarea[] = {
	{ 0, 0, T(p0), T(p1), T(p2), T(p3), T(p4), T(p5), T(p6), T(p7) },
};


static struct insn tabrpred[] = {
	{ 0x00, 0x7f, N("dir") }, // the direction flag; 0: read from memory, 1: write to memory
	{ 0x1c, 0x7f, N("pm0") },
	{ 0x1d, 0x7f, N("pm1") },
	{ 0x1e, 0x7f, N("pm2") },
	{ 0x1f, 0x7f, N("pm3") },
	{ 0x4a, 0x7f, N("newctxdone"), .vartype = NV50 },	// newctx CMD finished with loading new address... or something like that, it seems to be turned back off *very shortly* after the newctx CMD. only check it with a wait right after newctx. weird.
	{ 0x4b, 0x7f, N("xferbusy"), .vartype = NV50 },	// RAMIN xfer in progress
	{ 0x4c, 0x7f, N("delaydone"), .vartype = NV50 },
	{ 0x4d, 0x7f, .vartype = NV50 },	// always
	{ 0x60, 0x60, N("unit"), UNIT, .vartype = NV50 }, // if given unit present
	{ 0x68, 0x7f, .vartype = NV40 },	// always
	{ 0x00, 0x00, N("flag"), FLAG },
};

static struct insn tabpred[] = {
	{ 0x80, 0x80, N("not"), T(rpred) },
	{ 0x00, 0x80, T(rpred) },
	{ 0, 0, OOPS },
};

static struct insn tabcmd5[] = {
	{ 0x04, 0x1f, C("NEWCTX"), .vartype = NV50 },		// fetches grctx DMA object from channel object in 784
	{ 0x05, 0x1f, C("NEXT_TO_SWAP"), .vartype = NV50 },	// copies 330 [new channel] to 784 [channel used for ctx RAM access]
	{ 0x06, 0x1f, C("SET_CONTEXT_POINTER"), .vartype = NV50 },	// copies scratch to 334
	{ 0x07, 0x1f, C("SET_XFER_POINTER"), .vartype = NV50 },	// copies scratch to 33c, anding it with 0xffff8
	{ 0x08, 0x1f, C("DELAY"), .vartype = NV50 },		// sleeps scratch cycles, then briefly lights up delaydone
	{ 0x09, 0x1f, C("ENABLE"), .vartype = NV50 },		// resets 0x40 to 0
	{ 0x0c, 0x1f, C("END"), .vartype = NV50 },			// halts program execution, resets PC to 0
	{ 0x0d, 0x1f, C("NEXT_TO_CURRENT"), .vartype = NV50 },	// movs new channel RAMIN address to current channel RAMIN address, basically where the real switch happens
	{ 0, 0, CMD },
};

static struct insn tabcmd4[] = {
	{ 0x07, 0x1f, C("NEXT_TO_SWAP"), .vartype = NV40 },	// copies 330 [new channel] to 784 [channel used for ctx RAM access]
	{ 0x09, 0x1f, C("NEXT_TO_CURRENT"), .vartype = NV40 },	// movs new channel RAMIN address to current channel RAMIN address, basically where the real switch happens
	{ 0x0a, 0x1f, C("SET_CONTEXT_POINTER"), .vartype = NV40 },	// copies scratch to 334
	{ 0x0e, 0x1f, C("END"), .vartype = NV40 },
	{ 0, 0, CMD },
};

static struct insn tabm[] = {
	{ 0x100000, 0xff0000, N("ctx"), PGRAPH5, SR, .vartype = NV50 },
	{ 0x100000, 0xf00000, N("ctx"), PGRAPH5, GSIZE5, .vartype = NV50 },
	{ 0x100000, 0xffc000, N("ctx"), PGRAPH4, SR, .vartype = NV40 },
	{ 0x100000, 0xf00000, N("ctx"), PGRAPH4, GSIZE4, .vartype = NV40 },
	{ 0x200000, 0xf00000, N("lsr"), IMM },			// moves 20-bit immediate to scratch reg
	{ 0x300000, 0xf00000, N("lsr2"), IMM },			// moves 20-bit immediate to 338
	{ 0x400000, 0xfc0000, N("jmp"), T(pred), BTARG },		// jumps if condition true
	{ 0x440000, 0xfc0000, N("call"), T(pred), CTARG, .vartype = NV50 },	// calls if condition true, NVAx only
	{ 0x480000, 0xfc0000, N("ret"), T(pred), .vartype = NV50 },		// rets if condition true, NVAx only
	{ 0x500000, 0xf00000, N("waitfor"), T(pred) },		// waits until condition true.
	{ 0x600000, 0xf00000, N("cmd"), T(cmd5), .vartype = NV50 },		// runs a CMD.
	{ 0x600000, 0xf00000, N("cmd"), T(cmd4), .vartype = NV40 },		// runs a CMD.
	{ 0x700000, 0xf00080, N("clear"), T(rpred) },		// clears given flag
	{ 0x700080, 0xf00080, N("set"), T(rpred) },		// sets given flag
	{ 0x800000, 0xf80000, N("xfer1"), T(area), .vartype = NV50 },
	{ 0x880000, 0xf80000, N("xfer2"), T(area), .vartype = NV50 },
	{ 0x900000, 0xff0000, N("disable"), DIS0, .vartype = NV50 },		// ors 0x40 with given immediate.
	{ 0x910000, 0xff0000, N("disable"), DIS1, .vartype = NV50 },
	{ 0xa00000, 0xf00000, N("fl3"), PGRAPH5, .vartype = NV50 },		// movs given PGRAPH register to 0x400830.
	{ 0xc00000, 0xf80000, N("seek1"), T(area), SEEKP, .vartype = NV50 },
	{ 0xc80000, 0xf80000, N("seek2"), T(area), SEEKP, .vartype = NV50 },
	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0, 0, OP32, T(m) },
};

static const struct disvariant ctx_vars[] = {
	"nv40", NV40,
	"nv50", NV50,
};

const struct disisa ctx_isa_s = {
	tabroot,
	4,
	4,
	4,
	.vars = ctx_vars,
	.varsnum = sizeof ctx_vars / sizeof *ctx_vars,
};
