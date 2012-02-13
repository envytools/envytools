/*
 *
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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

/*
 * Immediate fields
 */

static struct bitfield abtargoff = { 0, 16 };
static struct bitfield btargoff = { { 9, 15 }, BF_SIGNED, .pcrel = 1 };
static struct bitfield exitcoff = { 0, 16 };
static struct bitfield aimmoff = { { 0, 19 }, BF_SIGNED };
#define ABTARG atombtarg, &abtargoff
#define BTARG atombtarg, &btargoff
#define EXITC atomimm, &exitcoff
#define AIMM atomimm, &aimmoff

/*
 * Register fields
 */

static struct bitfield dst_bf = { 19, 5 };
static struct bitfield src1_bf = { 14, 5 };
static struct bitfield src2_bf = { 9, 5 };
static struct reg adst_r = { &dst_bf, "a" };
static struct reg asrc1_r = { &src1_bf, "a" };
static struct reg asrc2_r = { &src2_bf, "a" };

#define ADST atomreg, &adst_r
#define ASRC1 atomreg, &asrc1_r
#define ASRC2 atomreg, &asrc2_r

F1(intr, 16, N("intr"))

static struct insn tabm[] = {
	{ 0x4fffffff, 0xffffffff, N("nop4") },
	{ 0x65000000, 0xff000000, N("mov"), ADST, AIMM },
	{ 0x60000000, 0xe0000000, OOPS, ADST, ASRC1, ASRC2 },
	{ 0xbf000007, 0xffffffff, N("nopb") },
	{ 0xdf000007, 0xffffffff, N("nopd") },
	{ 0xe0000000, 0xff000000, N("bra0"), BTARG },
	{ 0xe1000000, 0xff000000, N("bra1"), BTARG },
	{ 0xe2000000, 0xff000000, N("bra2"), BTARG },
	{ 0xe4000000, 0xff000000, N("bra4"), BTARG },
	{ 0xeaf80000, 0xffff0000, N("abra"), ABTARG },
	{ 0xef0001ff, 0xffffffff, N("nope") },
	{ 0xefffffff, 0xffffffff, U("end") },
	{ 0xfff80000, 0xfff80000, N("exit"), T(intr), EXITC },
	{ 0, 0, OOPS },
};

static struct insn tabroot[] = {
	{ 0, 0, OP32, T(m) },
};

const struct disisa vp1_isa_s = {
	tabroot,
	4,
	4,
	16,
};
