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
 * Immediate fields
 */

static struct bitfield mthdoff = { { 14, 12 }, BF_UNSIGNED, 2 };
static struct bitfield bfsrcposoff = { 17, 5 };
static struct bitfield bfszoff = { 22, 5 };
static struct bitfield bfdstposoff = { 27, 5 };
static struct bitfield mimmoff = { { 14, 18 }, BF_SIGNED };
static struct bitfield btargoff = { { 14, 18 }, BF_SIGNED, .pcrel = 1 };
#define MTHD atomimm, &mthdoff
#define BFSRCPOS atomimm, &bfsrcposoff
#define BFDSTPOS atomimm, &bfdstposoff
#define BFSZ atomimm, &bfszoff
#define MIMM atomimm, &mimmoff
#define BTARG atombtarg, &btargoff

static int reg1off[] = { 8, 3, 'r' };
static int reg2off[] = { 11, 3, 'r' };
static int reg3off[] = { 14, 3, 'r' };
#define REG1 atomreg, reg1off
#define REG2 atomreg, reg2off
#define REG3 atomreg, reg3off

F1(exit, 7, N("exit"));

static struct insn tabsrc[] = {
	{ AP, 0x00000000, 0x003e000f, N("add"), REG2, REG3 },
	{ AP, 0x00020000, 0x003e000f, N("adc"), REG2, REG3 },
	{ AP, 0x00040000, 0x003e000f, N("sub"), REG2, REG3 },
	{ AP, 0x00060000, 0x003e000f, N("sbb"), REG2, REG3 },
	{ AP, 0x00100000, 0x003e000f, N("xor"), REG2, REG3 },
	{ AP, 0x00120000, 0x003e000f, N("or"), REG2, REG3 },
	{ AP, 0x00140000, 0x003e000f, N("and"), REG2, REG3 },
	{ AP, 0x00160000, 0x003e000f, N("andn"), REG2, REG3 }, // REG2 & ~REG3
	{ AP, 0x00180000, 0x003e000f, N("nand"), REG2, REG3 }, // ~(REG2 & REG3)
	{ AP, 0x00000001, 0x0000000f, N("add"), REG2, MIMM },
	// take REG2, replace BFSZ bits starting at BFDSTPOS with BFSZ bits starting at BFSRCPOS in REG3.
	{ AP, 0x00000002, 0x0000000f, N("extr"), REG2, REG3, BFSRCPOS, BFSZ, BFDSTPOS },
	{ AP, 0, 0, OOPS, REG2 },
};

static struct insn tabm[] = {
	{ AP, 0x00000001, 0x0000007f, N("parm"), REG1 },
	{ AP, 0x00000007, 0x0000007f, N("braz"), REG2, BTARG },
	{ AP, 0x00000017, 0x0000007f, N("branz"), REG2, BTARG },
	{ AP, 0x00000015, 0x0000007f, N("read"), REG1, MTHD },
	{ AP, 0x00000010, 0x00000070, N("mov"), REG1, T(src) },
	{ AP, 0x00000020, 0x00000070, N("maddr"), REG1, T(src) },
	{ AP, 0x00000030, 0x00000070, N("parm"), REG1, N("send"), T(src) },
	{ AP, 0x00000040, 0x00000070, N("send"), REG1, T(src) },
	{ AP, 0x00000050, 0x00000070, N("parm"), REG1, N("maddr"), T(src) },
	{ AP, 0, 0, OOPS, REG1, T(src) },
};

static struct insn tabroot[] = {
	{ AP, 0, 0, OP32, T(exit), T(m) },
};

static struct disisa macro_isa_s = {
	tabroot,
	4,
	4,
	4,
};

struct disisa *macro_isa = &macro_isa_s;
