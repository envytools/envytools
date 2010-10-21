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
 * Code target field
 *
 * This field represents a code address and is used for branching and the
 * likes. Target is counted in 32-bit words from the start of microcode.
 */

static struct bitfield ctargoff = { { 8, 11 }, BF_UNSIGNED, 2 };
#define BTARG atombtarg, &ctargoff
#define CTARG atomctarg, &ctargoff

/*
 * Register fields
 */

static int src1off[] = { 8, 4, 'r' };
static int src2off[] = { 12, 4, 'r' };
static int dstoff[] = { 16, 4, 'r' };
static int predoff[] = { 20, 4, 'r' };
#define SRC1 atomreg, src1off
#define SRC2 atomreg, src2off
#define DST atomreg, dstoff
#define PRED atomreg, predoff

static struct insn tabp[] = {
	{ AP, 0x00f00000, 0x00f00000 },
	{ AP, 0, 0, PRED },
};

static struct insn tabm[] = {
	{ AP, 0x34000003, 0xfc0000ff, T(p), N("ret") },
	{ AP, 0x34000043, 0xfc0000ff, N("nop") },

	{ AP, 0x3c000000, 0xfc0000ff, T(p), N("bra"), BTARG },
	{ AP, 0x3c000002, 0xfc0000ff, T(p), N("call"), CTARG },
	{ AP, 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ -1, 0, 0, OP32, T(m) },
};

static struct disisa vp3m_isa_s = {
	tabroot,
	4,
	4,
	1,
};

struct disisa *vp3m_isa = &vp3m_isa_s;
