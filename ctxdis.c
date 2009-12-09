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

#include "coredis.h"

/*
 * PGRAPH registers of interest
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
 * 0x400324: code upload index, WO. selects address to write in ctxprog code,
 *           counted in whole insns.
 * 0x400328: code upload, WO. writes given insn to code segment and
 *           autoincrements upload address.
 * 0x40032c: current context RAMIN address, shifted right by 12 bits [?]
 * 0x400330: next context RAMIN address, shifted right by 12 bits
 * 0x400334: $r register, offset in RAMIN context to read/write with opcode 1.
 *           in units of 32-bit words.
 * 0x400338: Some register, set with opcode 3. Used on NVAx with values 0-9 to
 *           select TP number to read/write with G[0x8000]. Maybe generic
 *           PGRAPH offset? Also set to 0x8ffff in one place for some reason.
 * 0x40033c: Some other register. Set with opcode 0x600007. Related to and
 *           modified by opcode 8. Setting with opcode 0x600007 always ands
 *           contents with 0xffff8 for some reason.
 * 0x400824-0x400830: Flags, see below.
 *
 * How execution works:
 *
 * Execution can be in one of 3 states: stopped, running, and paused.
 * Initially, it's stopped. The only way to start execution is to poke 0x400304
 * with bit 0 set. When you do this, the uc starts executing opcodes from
 * current PC. The only way to stop execution is to hit opcode 0x60000c.
 * Note that 0x60000c resets PC to 0, so ctxprog will start over from there
 * next time, unless you poke PC before that. Running <-> paused transition
 * happens by poking 0x400304 with the appropriate bit set to 1. Poking with
 * *both* bits set to 1 causes it to reload the opcode register from current
 * PC, but nothing else happens. This can be used to read the code segment.
 * Poking with bits 4 and 0 set together causes it to start with paused state.
 *
 * Oh, also, PC has 9 valid bits. So you can have ctxprogs of max. 512 insns.
 */

/*
 * All of the above has been checked on my NV86 laptop card and on my NVA5.
 * Should probably apply to all NV50 family cards. NV40 is rather different,
 * but should follow the same idea. Needs to be checked some day.
 */

#define NV4x 1
#define NV5x 2 /* and 8x and 9x and Ax */
#define NVxx 3

/*
 * Code target field
 *
 * This field represents a code address and is used for branching and the
 * likes. Target is counted in 32-bit words from the start of microcode.
 */

#define CTARG atomctarg, 0
void atomctarg APROTO {
	fprintf (out, " %s%#llx", cbr, BF(8, 9)<<2);
}

/*
 * Misc number fields
 *
 * Used for plain numerical arguments.
 */

int unitoff[] = { 0, 5, 0, 0 };
int flagoff[] = { 0, 7, 0, 0 };
int gsize4off[] = { 14, 6, 0, 0 };
int gsize5off[] = { 16, 4, 0, 0 };
int immoff[] = { 0, 20, 0, 0 };
int dis0off[] = { 0, 16, 0, 0 };
int dis1off[] = { 0, 16, 16, 0 };
#define UNIT atomnum, unitoff
#define FLAG atomnum, flagoff
#define GSIZE4 atomnum, gsize4off
#define GSIZE5 atomnum, gsize5off
#define IMM atomnum, immoff
#define DIS0 atomnum, dis0off
#define DIS1 atomnum, dis1off

/*
 * Registers
 */

#define R(x) atomsreg, x
void atomsreg APROTO {
	fprintf (out, " %s$%s", cbl, (char *)v);
}
#define RA R("a")	// The scratch register. 0x40031c
#define RG R("g")	// PGRAPH offset register [?]. 0x400338
#define RR R("r")	// RAMIN offset register. 0x400334
#define RM R("m")	// ??? 0x40033c, writing from ctxprog aligns to 8.

/*
 * Memory fields
 */

// BF, offset shift, 'l'

int pgmem4[] = { 0, 14, 2, 'G' };
int pgmem5[] = { 0, 16, 2, 'G' };

#define PGRAPH4 atommem, pgmem4
#define PGRAPH5 atommem, pgmem5
void atommem APROTO {
	const int *n = v;
	fprintf (out, " %s%c[", ccy, n[3]);
//	if (n[3] == 'G') printf("%s$g%s+", cbl, ccy);
	int mo = BF(n[0], n[1])<<n[2];
	fprintf (out, "%s%#x%s]", cyel, mo, ccy);
}

/*
 * The flags, used as predicates for jump and wait insns.
 *
 * There are 0x80 of them, mapping directly to bits in PGRAPH regs 0x824
 * through 0x830. Known ones:
 *
 *  - 0x824 bit 0 [0x00]: direction flag used by pgraph opcode to select
 *    transfer direction.
 *  - 0x824 and 0x828, remaining bits [0x01-0x3f]: ??? RW
 *  - 0x82c [0x40-0x5f]: RO, some sort of PGRAPH status or something...
 *  - 0x82c bit 13 [0x4d]: always set, used for unconditional jumps
 *  - 0x830 bits 0-10 [0x60-0x69]: used as the TP enable bits. copied from
 *    0x8fc, which in turn is a mirror of some bits from PMC+0x1540.
 *
 * None of this checked on NV40.
 */
struct insn tabrpred[] = {
	{ NVxx, 0x00, 0x7f, N("dir") }, // the direction flag
	{ NV5x, 0x4d, 0x7f },	// always?
	{ NV5x, 0x60, 0x60, N("unit"), UNIT }, // if given unit present
	{ NVxx, 0x00, 0x00, N("flag"), FLAG },
};

struct insn tabpred[] = {
	{ NVxx, 0x80, 0x80, N("not"), T(rpred) },
	{ NVxx, 0x00, 0x80, T(rpred) },
};

struct insn tabm[] = {
	{ NV5x, 0x100000, 0xff0000, N("pgraph"), PGRAPH5, RA },
	{ NV5x, 0x100000, 0xf00000, N("pgraph"), PGRAPH5, GSIZE5 },
	{ NV4x, 0x100000, 0xffc000, N("pgraph"), PGRAPH4, RA },
	{ NV4x, 0x100000, 0xf00000, N("pgraph"), PGRAPH4, GSIZE4 },
	{ NVxx, 0x200000, 0xf00000, N("mov"), RA, IMM },		// moves 20-bit immediate to scratch reg
	{ NV5x, 0x300000, 0xf00000, N("mov"), RG, IMM },		// moves 20-bit immediate to 338
	{ NVxx, 0x400000, 0xfc0000, N("jmp"), T(pred), CTARG },		// jumps if condition true
	{ NV5x, 0x440000, 0xfc0000, N("call"), T(pred), CTARG },	// calls if condition true, NVAx only
	{ NV5x, 0x480000, 0xfc0000, N("ret"), T(pred) },		// rets if condition true, NVAx only
	{ NV5x, 0x500000, 0xf00000, N("wait"), T(pred) },		// waits until condition true.
	{ NV5x, 0x600006, 0xffffff, N("mov"), RR, RA },			// copies scratch to 334
	{ NV5x, 0x600007, 0xffffff, N("mov"), RM, RA },			// copies scratch to 33c, anding it with 0xffff8
	{ NV5x, 0x600009, 0xffffff, N("enable") },			// resets 0x40 to 0
	{ NV5x, 0x60000c, 0xffffff, N("exit") },			// halts program execution, resets PC to 0
	{ NV5x, 0x60000d, 0xffffff, N("ctxsw") },			// movs new RAMIN address to current RAMIN address, basically where the real switch happens
	{ NV4x, 0x60000e, 0xffffff, N("exit") },
	{ NVxx, 0x700000, 0xf00080, N("clear"), T(rpred) },		// clears given flag
	{ NVxx, 0x700080, 0xf00080, N("set"), T(rpred) },		// sets given flag
	{ NV5x, 0x900000, 0x9f0000, N("disable"), DIS0 },		// ors 0x40 with given immediate.
	{ NV5x, 0x910000, 0x9f0000, N("disable"), DIS1 },
	{ NV5x, 0xa00000, 0xf10000, N("mov"), N("units"), PGRAPH5 },	// movs given PGRAPH register to 0x400830.
	{ NVxx, 0, 0, OOPS },
};

/*
 * Disassembler driver
 *
 * You pass a block of memory to this function, disassembly goes out to given
 * FILE*.
 */

void nv50dis (FILE *out, uint32_t *code, int num, int ptype) {
	int cur = 0;
	while (cur < num) {
		ull a = code[cur], m = 0;
		fprintf (out, "%s%08x: %s", cgray, cur*4, cnorm);
		fprintf (out, "%08llx", a);
		atomtab (out, &a, &m, tabm, ptype, cur);
		a &= ~m;
		if (a) {
			fprintf (out, " %s[unknown: %08llx]%s", cred, a, cnorm);
		}
		printf ("%s\n", cnorm);
		cur++;
	}
}

int main(int argc, char **argv) {
	int ptype = NV5x;
	char c;
	while ((c = getopt (argc, argv, "45")) != -1)
		switch (c) {
			case '4':
				ptype = NV4x;
				break;
			case '5':
				ptype = NV5x;
				break;
		}
	int num = 0;
	int maxnum = 16;
	uint32_t *code = malloc (maxnum * 4);
	uint32_t t;
	while (!feof(stdin) && scanf ("%x", &t) == 1) {
		if (num == maxnum) maxnum *= 2, code = realloc (code, maxnum*4);
		code[num++] = t;
		scanf (" ,");
	}
	nv50dis (stdout, code, num, ptype);
	return 0;
}
