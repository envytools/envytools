/*
 * Copyright (C) 2010-2011 Marcelina Kościelnicka <mwk@0x04.net>
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
 * Immediate fields
 */

static struct bitfield bfsrcposoff = { 17, 5 };
static struct bitfield bfszoff = { 22, 5 };
static struct bitfield bfdstposoff = { 27, 5 };
static struct rbitfield mimmoff = { { 14, 18 }, BF_SIGNED };
static struct rbitfield btargoff = { { 14, 18 }, BF_SIGNED, .pcrel = 1 };
#define BFSRCPOS atomimm, &bfsrcposoff
#define BFDSTPOS atomimm, &bfdstposoff
#define BFSZ atomimm, &bfszoff
#define MIMM atomrimm, &mimmoff
#define BTARG atombtarg, &btargoff

/*
 * Register fields
 */

static struct sreg reg_sr[] = {
	{ 0, 0, SR_ZERO },
	{ -1 },
};
static struct bitfield reg1_bf = { 8, 3 };
static struct bitfield reg2_bf = { 11, 3 };
static struct bitfield reg3_bf = { 14, 3 };
static struct reg reg1_r = { &reg1_bf, "r", .specials = reg_sr };
static struct reg reg2_r = { &reg2_bf, "r", .specials = reg_sr };
static struct reg reg3_r = { &reg3_bf, "r", .specials = reg_sr };
#define REG1 atomreg, &reg1_r
#define REG2 atomreg, &reg2_r
#define REG3 atomreg, &reg3_r

/*
 * The Fermi PGRAPH MACRO ISA.
 *
 * Code space is 0x800 32-bit words long and part of PGRAPH context. Code
 * addresses are counted in 32-bit words.  The MACRO unit sits between PFIFO
 * and PGRAPH_DISPATCH. It processes the following methods:
 *
 *  - 0x114: set upload address [in words]
 *  - 0x118: upload a single word of code to current upload index, increment
 *           the index
 *  - 0x11c: set binding macro index
 *  - 0x120: bind macro specified by 0x11c to code starting at given address
 *  - 0x3800+i*8: launch macro #i [XXX: check range]
 *  - 0x3804+i*8: send an extra param
 *  - any other: pass through to DISPATCH unchanged
 *
 * There are 8 GPRs, $r0-$r7, $r0 is hardwired to 0. $r1 is set on launch to
 * the data sent to the 0x3800+i*8 method that caused the launch. There's
 * a carry flag for adc and sbb opcodes.
 *
 * The state also includes "current method address" and "current method
 * increment". They're both set using various "maddr" ops as listed below -
 * bits 0-11 of operand are shifted left by 2 and become the method address,
 * bits 12-17 are shifted left by 2 and become the increment. Other bits
 * are ignored. A "send" op sends a method specified by the current address
 * with the param used as data, then increments current address by current
 * increment.
 *
 * A macro is also allowed to submit method reads. This is done by using
 * the read op. The operand's bits 0-11 are shifted left by 2 and used as
 * method address to read.
 *
 * A macro can also pull extra parameters by the "parm" op and its variations.
 * "parm" will wait until a 0x3804+i*8 method is submitted from PFIFO and load
 * its data to the destination. If an unconsumed parameter remains after macro
 * exits, or if a non-3804 method is submitted while macro is waiting for
 * a parameter, a trap occurs.
 *
 * As for flow control, there are branch on zero / non-zero instructions, and
 * every instruction can be marked with the "exit" modifier which causes the
 * macro to end. Normal branches and "exit" have delay slots - they don't
 * actually take effect until the next instruction finishes. Branches also
 * have "annul" variants which don't have delay slots. Combining "exit" with
 * a branch is a special case: the exit happens if and only if the branch
 * is not taken. It's an error to use a branch in a delay slot.
 *
 * Stuff marked SC below is not actually a separate instruction, just
 * a special case of a more generic insn.
 */

F1(exit, 7, N("exit"));

/* various stuff that can be done to result of arith/logic operations */
static struct insn tabdst[] = {
	{ 0x00000000, 0x00000070, N("parm"), REG1, N("ign") },	// ignore result, fetch param to REG1
	{ 0x00000010, 0x00000070, N("mov"), REG1 },			// store result to REG1
	{ 0x00000020, 0x00000770, N("maddr") }, // SC
	{ 0x00000020, 0x00000070, N("maddr"), REG1 },		// use result as maddr and store it to REG1
	{ 0x00000030, 0x00000070, N("parm"), REG1, N("send") },	// send result, then fetch param to REG1
	{ 0x00000040, 0x00000770, N("send") }, // SC
	{ 0x00000040, 0x00000070, N("send"), REG1 },		// send result and store it to REG1
	{ 0x00000050, 0x00000070, N("parm"), REG1, N("maddr") },	// use result as maddr, then fetch param to REG1
	{ 0x00000060, 0x00000770, N("parmsend"), N("maddr") }, // SC
	{ 0x00000060, 0x00000070, N("parmsend"), N("maddr"), REG1 },// use result as maddr and store it to REG1, then fetch param and send it. 
	{ 0x00000070, 0x00000770, N("maddrsend") }, // SC
	{ 0x00000070, 0x00000070, N("maddrsend"), REG1 },		// use result as maddr, then send bits 12-17 of result, then store result to REG1
	{ 0, 0, OOPS },
};

F1(annul, 5, N("annul")); // if set, delay slot insn is annuled if branch taken [ie. branch behaves as if delay slots didn't exist]

static struct insn tabm[] = {
	{ 0x00000000, 0x003e0007, T(dst), SESTART, N("add"), REG2, REG3, SEEND },
	{ 0x00020000, 0x003e0007, T(dst), SESTART, N("adc"), REG2, REG3, SEEND },
	{ 0x00040000, 0x003e0007, T(dst), SESTART, N("sub"), REG2, REG3, SEEND },
	{ 0x00060000, 0x003e0007, T(dst), SESTART, N("sbb"), REG2, REG3, SEEND },
	{ 0x00100000, 0x003e0007, T(dst), SESTART, N("xor"), REG2, REG3, SEEND },
	{ 0x00120000, 0x003e0007, T(dst), SESTART, N("or"), REG2, REG3, SEEND },
	{ 0x00140000, 0x003e0007, T(dst), SESTART, N("and"), REG2, REG3, SEEND },
	{ 0x00160000, 0x003e0007, T(dst), SESTART, N("andn"), REG2, REG3, SEEND }, // REG2 & ~REG3
	{ 0x00180000, 0x003e0007, T(dst), SESTART, N("nand"), REG2, REG3, SEEND }, // ~(REG2 & REG3)
	{ 0x00000011, 0xffffffff, N("nop") }, // SC
	{ 0x00000091, 0xffffffff }, // SC [just exit]
	{ 0x00000001, 0xfffff87f, N("parm"), REG1 }, // SC
	{ 0x00000001, 0x00003807, T(dst), MIMM }, // SC
	{ 0x00000001, 0xffffc007, T(dst), REG2 }, // SC
	{ 0x00000001, 0x00000007, T(dst), SESTART, N("add"), REG2, MIMM, SEEND },
	// take REG2, replace BFSZ bits starting at BFDSTPOS with BFSZ bits starting at BFSRCPOS in REG3.
	{ 0x00000002, 0x00000007, T(dst), SESTART, N("extrinsrt"), REG2, REG3, BFSRCPOS, BFSZ, BFDSTPOS, SEEND },
	// take BFSZ bits starting at REG2 in REG3, shift left by BFDSTPOS
	{ 0x00000003, 0x00000007, T(dst), SESTART, N("extrshl"), REG3, REG2, BFSZ, BFDSTPOS, SEEND },
	// take BFSZ bits starting at BFSRCPOS in REG3, shift left by REG2
	{ 0x00000004, 0x00000007, T(dst), SESTART, N("extrshl"), REG3, BFSRCPOS, BFSZ, REG2, SEEND },
	{ 0x00000015, 0x00003877, N("read"), REG1, MIMM }, // SC
	{ 0x00000015, 0x00000077, N("read"), REG1, SESTART, N("add"), REG2, MIMM, SEEND },
	{ 0x00000007, 0x00003817, N("bra"), T(annul), BTARG },
	{ 0x00000007, 0x00000017, N("braz"), T(annul), REG2, BTARG },
	{ 0x00000017, 0x00000017, N("branz"), T(annul), REG2, BTARG },
	{ 0, 0, T(dst), OOPS, REG2 },
};

static struct insn tabroot[] = {
	{ 0, 0, OP1B, T(exit), T(m) },
};

struct disisa macro_isa_s = {
	tabroot,
	1,
	1,
	4,
};
