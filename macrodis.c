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
static struct bitfield incroff = { { 26, 6 }, BF_UNSIGNED, 2 };
static struct bitfield bfposoff = { 17, 5 };
static struct bitfield bfszoff = { 22, 5 };
#define MTHD atomimm, &mthdoff
#define INCR atomimm, &incroff
#define BFPOS atomimm, &bfposoff
#define BFSZ atomimm, &bfszoff

static int reg1off[] = { 8, 3, 'r' };
static int reg2off[] = { 11, 3, 'r' };
static int reg3off[] = { 14, 3, 'r' };
#define REG1 atomreg, reg1off
#define REG2 atomreg, reg2off
#define REG3 atomreg, reg3off

F1(exit, 7, N("exit"));

static struct insn tabm[] = {
	{ AP, 0x00000015, 0x0000007f, N("read"), REG1, MTHD },
	{ AP, 0x00000021, 0x0000007f, N("prep"), MTHD, INCR },
	{ AP, 0x00000041, 0x0000007f, N("send"), REG2 },
	{ AP, 0x00000042, 0x0000007f, N("sendbf"), REG3, BFPOS, BFSZ },
	{ AP, 0, 0, OOPS },
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
