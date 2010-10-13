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

#define CTARG atomctarg, 0
static void atomctarg APROTO {
	if (!out)
		return;
	fprintf (out, " %s%#llx", cbr, BF(8, 9));
}

/*
 * CMDs
 */
#define C(x) atomcmd, x
static void atomcmd APROTO {
	if (!out)
		return;
	fprintf (out, " %s%s", cmag, (char *)v);
}

/*
 * Misc number fields
 *
 * Used for plain numerical arguments.
 */

static int unitoff[] = { 0, 5, 0, 0 };
static int flagoff[] = { 0, 7, 0, 0 };
static int gsize4off[] = { 14, 6, 0, 0 };
static int gsize5off[] = { 16, 4, 0, 0 };
static int immoff[] = { 0, 20, 0, 0 };
static int dis0off[] = { 0, 16, 0, 0 };
static int dis1off[] = { 0, 16, 16, 0 };
static int seekpoff[] = { 8, 11, 0, 0 };
#define UNIT atomnum, unitoff
#define FLAG atomnum, flagoff
#define GSIZE4 atomnum, gsize4off
#define GSIZE5 atomnum, gsize5off
#define IMM atomnum, immoff
#define DIS0 atomnum, dis0off
#define DIS1 atomnum, dis1off
#define SEEKP atomnum, seekpoff

/*
 * Memory fields
 */

// BF, offset shift, 'l'

static int pgmem4[] = { 0, 14, 2, 'G' };
static int pgmem5[] = { 0, 16, 2, 'G' };

#define PGRAPH4 atommem, pgmem4
#define PGRAPH5 atommem, pgmem5
static void atommem APROTO {
	if (!out)
		return;
	const int *n = v;
	fprintf (out, " %s%c[", ccy, n[3]);
//	if (n[3] == 'G') printf("%s$g%s+", cbl, ccy);
	int mo = BF(n[0], n[1])<<n[2];
	fprintf (out, "%s%#x%s]", cyel, mo, ccy);
}

F1(p0, 0, N("a0"))
F1(p1, 1, N("a1"))
F1(p2, 2, N("a2"))
F1(p3, 3, N("a3"))
F1(p4, 4, N("a4"))
F1(p5, 5, N("a5"))
F1(p6, 6, N("a6"))
F1(p7, 7, N("a7"))

static struct insn tabarea[] = {
	{ NVxx, 0, 0, T(p0), T(p1), T(p2), T(p3), T(p4), T(p5), T(p6), T(p7) },
};


static struct insn tabrpred[] = {
	{ NVxx, 0x00, 0x7f, N("dir") }, // the direction flag
	{ NV5x, 0x4a, 0x7f, N("newctxdone") },	// newctx CMD finished with loading new address... or something like that, it seems to be turned back off *very shortly* after the newctx CMD. only check it with a wait right after newctx. weird.
	{ NV5x, 0x4b, 0x7f, N("xferbusy") },	// RAMIN xfer in progress
//	{ NV5x, 0x4c, 0x7f, OOPS },	// CMD 8 done.
	{ NV5x, 0x4d, 0x7f },	// always
	{ NV5x, 0x60, 0x60, N("unit"), UNIT }, // if given unit present
	{ NV4x, 0x68, 0x7f },	// always
	{ NVxx, 0x00, 0x00, N("flag"), FLAG },
};

static struct insn tabpred[] = {
	{ NVxx, 0x80, 0x80, N("not"), T(rpred) },
	{ NVxx, 0x00, 0x80, T(rpred) },
};

static struct insn tabcmd5[] = {
	{ NV5x, 0x04, 0x1f, C("NEWCTX") },		// fetches grctx DMA object from channel object in 784
	{ NV5x, 0x05, 0x1f, C("NEXT_TO_SWAP") },	// copies 330 [new channel] to 784 [channel used for ctx RAM access]
	{ NV5x, 0x06, 0x1f, C("SET_CONTEXT_POINTER") },	// copies scratch to 334
	{ NV5x, 0x07, 0x1f, C("SET_XFER_POINTER") },	// copies scratch to 33c, anding it with 0xffff8
//	{ NV5x, 0x08, 0x1f, OOPS },			// does something with scratch contents...
	{ NV5x, 0x09, 0x1f, C("ENABLE") },		// resets 0x40 to 0
	{ NV5x, 0x0c, 0x1f, C("END") },			// halts program execution, resets PC to 0
	{ NV5x, 0x0d, 0x1f, C("NEXT_TO_CURRENT") },	// movs new channel RAMIN address to current channel RAMIN address, basically where the real switch happens
	{ NVxx, 0, 0, OOPS },
};

static struct insn tabcmd4[] = {
	{ NV4x, 0x07, 0x1f, C("NEXT_TO_SWAP") },	// copies 330 [new channel] to 784 [channel used for ctx RAM access]
	{ NV4x, 0x09, 0x1f, C("NEXT_TO_CURRENT") },	// movs new channel RAMIN address to current channel RAMIN address, basically where the real switch happens
	{ NV4x, 0x0a, 0x1f, C("SET_CONTEXT_POINTER") },	// copies scratch to 334
	{ NV4x, 0x0e, 0x1f, C("END") },
	{ NVxx, 0, 0, OOPS },
};

static struct insn tabm[] = {
	{ NV5x, 0x100000, 0xff0000, N("ctx"), PGRAPH5, N("sr") },
	{ NV5x, 0x100000, 0xf00000, N("ctx"), PGRAPH5, GSIZE5 },
	{ NV4x, 0x100000, 0xffc000, N("ctx"), PGRAPH4, N("sr") },
	{ NV4x, 0x100000, 0xf00000, N("ctx"), PGRAPH4, GSIZE4 },
	{ NVxx, 0x200000, 0xf00000, N("lsr"), IMM },			// moves 20-bit immediate to scratch reg
	{ NVxx, 0x300000, 0xf00000, N("lsr2"), IMM },			// moves 20-bit immediate to 338
	{ NVxx, 0x400000, 0xfc0000, N("jmp"), T(pred), CTARG },		// jumps if condition true
	{ NV5x, 0x440000, 0xfc0000, N("call"), T(pred), CTARG },	// calls if condition true, NVAx only
	{ NV5x, 0x480000, 0xfc0000, N("ret"), T(pred) },		// rets if condition true, NVAx only
	{ NVxx, 0x500000, 0xf00000, N("waitfor"), T(pred) },		// waits until condition true.
	{ NV5x, 0x600000, 0xf00000, N("cmd"), T(cmd5) },		// runs a CMD.
	{ NV4x, 0x600000, 0xf00000, N("cmd"), T(cmd4) },		// runs a CMD.
	{ NVxx, 0x700000, 0xf00080, N("clear"), T(rpred) },		// clears given flag
	{ NVxx, 0x700080, 0xf00080, N("set"), T(rpred) },		// sets given flag
	{ NV5x, 0x800000, 0xf80000, N("xfer1"), T(area) },
	{ NV5x, 0x880000, 0xf80000, N("xfer2"), T(area) },
	{ NV5x, 0x900000, 0x9f0000, N("disable"), DIS0 },		// ors 0x40 with given immediate.
	{ NV5x, 0x910000, 0x9f0000, N("disable"), DIS1 },
	{ NV5x, 0xa00000, 0xf00000, N("fl3"), PGRAPH5 },		// movs given PGRAPH register to 0x400830.
	{ NV5x, 0xc00000, 0xf80000, N("seek1"), T(area), SEEKP },
	{ NV5x, 0xc80000, 0xf80000, N("seek2"), T(area), SEEKP },
	{ NVxx, 0, 0, OOPS },
};

/*
 * Disassembler driver
 *
 * You pass a block of memory to this function, disassembly goes out to given
 * FILE*.
 */

void ctxdis (FILE *out, uint32_t *code, uint32_t start, int num, int ptype) {
	int cur = 0;
	int *labels = calloc(num, sizeof *labels);
	while (cur < num) {
		ull a = code[cur], m = 0;
		fprintf (out, "%s%08x: %s", cgray, cur + start, cnorm);
		fprintf (out, "%08llx", a);
		atomtab (out, &a, &m, tabm, ptype, cur + start, labels, num);
		a &= ~m;
		if (a) {
			fprintf (out, " %s[unknown: %08llx]%s", cred, a, cnorm);
		}
		printf ("%s\n", cnorm);
		cur++;
	}
	free(labels);
}
