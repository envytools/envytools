/*
 * Copyright (C) 2009-2011 Marcelina Kościelnicka <mwk@0x04.net>
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

#define F_NV40	1
#define F_G80	2
#define F_CALLRET	4

/*
 * PGRAPH registers of interest, G80
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
 * 0x40031c: register $ra (20-bit), holding parameters to various stuff.
 * 0x400320: WO, CMD register. Writing here launches a cmd, see below.
 * 0x400324: code upload index, WO. selects address to write in ctxprog code,
 *           counted in whole insns.
 * 0x400328: code upload, WO. writes given insn to code segment and
 *           autoincrements upload address.
 * 0x40032c: current channel RAMIN address, shifted right by 12 bits. bit 31 == valid
 * 0x400330: next channel RAMIN address, shifted right by 12 bits. bit 31 == valid
 * 0x400334: $r register, offset in RAMIN context to read/write with opcode 1.
 *           in units of 32-bit words.
 * 0x400338: register $rb (20-bit), holding parameters to various stuff.
 *           Used on NVAx with values 0-9 to
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

static struct rbitfield ctargoff = { 8, 9 };
#define BTARG atombtarg, &ctargoff
#define CTARG atomctarg, &ctargoff

/* Register fields */

static struct reg ra_r = { 0, "ra" };
static struct reg rb_r = { 0, "rb" };
#define RA atomreg, &ra_r
#define RB atomreg, &rb_r

/*
 * Misc number fields
 *
 * Used for plain numerical arguments.
 */

static struct bitfield scratchbitoff = { 0, 5 };
static struct bitfield flagoff = { 0, 7 };
static struct bitfield gsize4off = { 14, 6 };
static struct bitfield gsize5off = { 16, 4 };
static struct bitfield immoff = { 0, 20 };
static struct bitfield rmaskoff = { 0, 16 };
static struct bitfield ridxoff = { 16, 4 };
static struct bitfield xstart4off = { 3, 7 };
static struct bitfield xstart5off = { 8, 11 };
static struct bitfield xmaskoff = { 0, 8 };
static struct bitfield cmdoff = { 0, 5 };
#define SCRATCHBIT atomimm, &scratchbitoff
#define FLAG atomimm, &flagoff
#define GSIZE4 atomimm, &gsize4off
#define GSIZE5 atomimm, &gsize5off
#define IMM atomimm, &immoff
#define RMASK atomimm, &rmaskoff
#define RIDX atomimm, &ridxoff
#define XSTART4 atomimm, &xstart4off
#define XSTART5 atomimm, &xstart5off
#define XMASK atomimm, &xmaskoff
#define CMD atomimm, &cmdoff

/*
 * Memory fields
 */

// BF, offset shift, 'l'

static struct rbitfield pgmem4_imm = { { 0, 14 }, RBF_UNSIGNED, 2 };
static struct rbitfield pgmem5_imm = { { 0, 16 }, RBF_UNSIGNED, 2 };
static struct mem pgmem4_m = { "G", 0, 0, &pgmem4_imm };
static struct mem pgmem5_m = { "G", 0, 0, &pgmem5_imm };
#define PGRAPH4 atommem, &pgmem4_m
#define PGRAPH5 atommem, &pgmem5_m


static struct insn tabrpred[] = {
	{ 0x00, 0x7f, N("dirsave") }, // the direction flag; 0: read from memory, 1: write to memory
	{ 0x03, 0x7f, N("ctxfreeze"), .fmask = F_G80 }, // disallows all modification of context, so that S/R can proceed
	{ 0x05, 0x7f, N("swsavereq") }, // SW defined
	{ 0x06, 0x7f, N("swrestorereq") }, // SW defined
	{ 0x09, 0x7f, N("swxferreq") }, // SW defined
	{ 0x1c, 0x7f, N("pm0"), .fmask = F_G80 },
	{ 0x1d, 0x7f, N("pm1"), .fmask = F_G80 },
	{ 0x1e, 0x7f, N("pm2"), .fmask = F_G80 },
	{ 0x1f, 0x7f, N("pm3"), .fmask = F_G80 },
	// NV40 hardware RO flags
	{ 0x60, 0x7f, N("idle"), .fmask = F_NV40 },
	{ 0x64, 0x7f, N("hwsavereq"), .fmask = F_NV40 },
	{ 0x65, 0x7f, N("hwrestorereq"), .fmask = F_NV40 },
	{ 0x66, 0x7f, N("state3d"), .fmask = F_NV40 },	// wired straight to the CTX_USER bit.
	{ 0x68, 0x7f, .fmask = F_NV40 },	// always
	{ 0x69, 0x7f, N("timeridle"), .fmask = F_NV40 },
	// G80 hardware RO flags
	{ 0x40, 0x7f, N("idle"), .fmask = F_G80 },
	{ 0x44, 0x7f, N("hwsavereq"), .fmask = F_G80 },
	{ 0x45, 0x7f, N("hwrestorereq"), .fmask = F_G80 },
	{ 0x4a, 0x7f, N("binddmaack"), .fmask = F_G80 },	// bind ctx dma CMD finished with loading new address... or something like that, it seems to be turned back off *very shortly* after the CMD. only check it with a wait right after cmd. weird.
	{ 0x4b, 0x7f, N("xferbusy"), .fmask = F_G80 },	// RAMIN xfer in progress
	{ 0x4c, 0x7f, N("timeridle"), .fmask = F_G80 },
	{ 0x4d, 0x7f, .fmask = F_G80 },	// always
	{ 0x4f, 0x7f, N("intrpending"), .fmask = F_G80 },
	// G80 scratch reg
	{ 0x60, 0x60, N("scratch"), SCRATCHBIT, .fmask = F_G80 },
	// unknown
	{ 0x00, 0x00, N("flag"), FLAG, OOPS },
};

static struct insn tabpred[] = {
	{ 0x80, 0x80, N("not"), T(rpred) },
	{ 0x00, 0x80, T(rpred) },
	{ 0, 0, OOPS },
};

static struct insn tabcmd4[] = {
	{ 0x07, 0x1f, C("BIND_CHANNEL") },	// copies 330 [new channel] to 784 [channel used for ctx RAM access]
	{ 0x09, 0x1f, C("FINISH_CHSW") },	// movs new channel RAMIN address to current channel RAMIN address, making PFIFO happy
	{ 0x0a, 0x1f, C("SET_CTX_POINTER") },	// copies scratch to 334
	{ 0x0b, 0x1f, C("START_TIMER") },	// sets the timer to count down $ra cycles
	{ 0x0c, 0x1f, C("CLEAR_RESET") },	// undoes all reset commands
	{ 0x0d, 0x1f, C("HALT") },		// halt execution *without* resetting PC to 0
	{ 0x0e, 0x1f, C("END") },		// halts program execution, resets PC to 0
	{ 0x0f, 0x1f, C("INTR") },		// trigger context switch interrupt and halt
	{ 0, 0, OOPS, CMD },
};

static struct insn tabcmd5[] = {
	{ 0x04, 0x1f, C("BIND_CTX_DMA") },	// rebinds DMA object for ctx access
	{ 0x05, 0x1f, C("BIND_CHANNEL") },	// copies 330 [new channel] to 784 [channel used for MMU accesses]
	{ 0x06, 0x1f, C("SET_REG_POINTER") },	// copies scratch to 334
	{ 0x07, 0x1f, C("SET_XFER_POINTER") },	// copies scratch to 33c, anding it with 0xffff8
	{ 0x08, 0x1f, C("START_TIMER") },
	{ 0x09, 0x1f, C("CLEAR_RESET") },
	{ 0x0a, 0x1f, C("HALT") },
	{ 0x0b, 0x1f, C("INTR") },
	{ 0x0c, 0x1f, C("END") },
	{ 0x0d, 0x1f, C("FINISH_CHSW") },
	// 0x11 seen
	// 0x16 seen on G200+
	{ 0, 0, OOPS, CMD },
};

static struct insn tabxarea4[] = {
	{ 0x000001, 0x000007, N("main") },
	{ 0x000002, 0x000007, N("unk2") },
	{ 0, 0, OOPS },
};

static struct insn tabxarea5[] = {
	{ 0x000000, 0x080000, N("main") },
	{ 0x080000, 0x080000, N("mp") },
	{ 0, 0, OOPS },
};

static struct insn tabm[] = {
	{ 0x000000, 0xf00000, N("nop") },
	{ 0x100000, 0xff0000, N("reg"), PGRAPH5, RA, .fmask = F_G80 },
	{ 0x100000, 0xf00000, N("reg"), PGRAPH5, GSIZE5, .fmask = F_G80 },
	{ 0x100000, 0xffc000, N("reg"), PGRAPH4, RA, .fmask = F_NV40 },
	{ 0x100000, 0xf00000, N("reg"), PGRAPH4, GSIZE4, .fmask = F_NV40 },
	{ 0x200000, 0xf00000, N("li"), RA, IMM },				// moves 20-bit immediate to $ra
	{ 0x300000, 0xf00000, N("li"), RB, IMM },				// moves 20-bit immediate to $rb
	{ 0x400000, 0xfc0000, N("jmp"), T(pred), BTARG },			// jumps if condition true
	{ 0x440000, 0xfc0000, N("call"), T(pred), CTARG, .fmask = F_CALLRET },	// calls if condition true
	{ 0x480000, 0xfc0000, N("ret"), T(pred), .fmask = F_CALLRET },		// rets if condition true
	{ 0x500000, 0xf00000, N("waitfor"), T(pred) },				// waits until condition true.
	{ 0x600000, 0xf00000, N("cmd"), T(cmd5), .fmask = F_G80 },		// runs a CMD.
	{ 0x600000, 0xf00000, N("cmd"), T(cmd4), .fmask = F_NV40 },		// runs a CMD.
	{ 0x700000, 0xf00080, N("clear"), T(rpred) },				// clears given flag
	{ 0x700080, 0xf00080, N("set"), T(rpred) },				// sets given flag
	{ 0x800000, 0xf00000, N("xfer"), T(xarea4), XSTART4, .fmask = F_NV40 },
	{ 0x800000, 0xf00000, N("xfer"), T(xarea5), XMASK, .fmask = F_G80 },
	{ 0x900000, 0xf00000, N("reset"), RIDX, RMASK },			// ors 0x40 with given immediate.
	{ 0xa00000, 0xf00000, N("ldsr"), PGRAPH5, .fmask = F_G80 },		// movs given PGRAPH register to 0x400830 aka the scratch reg.
	{ 0xc00000, 0xf00000, N("seek"), T(xarea5), XMASK, XSTART5, .fmask = F_G80 },
	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0, 0, OP1B, T(m) },
};

static void ctx_prep(struct disisa *isa) {
	isa->vardata = vardata_new("ctxisa");
	int f_nv40op = vardata_add_feature(isa->vardata, "nv40op", "NV40 family exclusive opcodes");
	int f_g80op = vardata_add_feature(isa->vardata, "g80op", "G80 family exclusive opcodes");
	int f_callret = vardata_add_feature(isa->vardata, "callret", "call and ret ops");
	if (f_nv40op == -1 || f_g80op == -1 || f_callret == -1)
		abort();
	int vs_chipset = vardata_add_varset(isa->vardata, "chipset", "GPU chipset");
	if (vs_chipset == -1)
		abort();
	int v_nv40 = vardata_add_variant(isa->vardata, "nv40", "NV40:G80", vs_chipset);
	int v_g80 = vardata_add_variant(isa->vardata, "g80", "G80:G200", vs_chipset);
	int v_g200 = vardata_add_variant(isa->vardata, "g200", "G200:GF100", vs_chipset);
	if (v_nv40 == -1 || v_g80 == -1 || v_g200 == -1)
		abort();
	vardata_variant_feature(isa->vardata, v_nv40, f_nv40op);
	vardata_variant_feature(isa->vardata, v_g80, f_g80op);
	vardata_variant_feature(isa->vardata, v_g200, f_g80op);
	vardata_variant_feature(isa->vardata, v_g200, f_callret);
	if (vardata_validate(isa->vardata))
		abort();
}

struct disisa ctx_isa_s = {
	tabroot,
	1,
	1,
	4,
	.prep = ctx_prep,
};
