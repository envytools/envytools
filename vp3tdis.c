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

static struct bitfield reg1_bf = { 0, 5 };
static struct bitfield reg2_bf = { 5, 5 };
static struct bitfield reg3_bf = { 29, 5 };
static struct reg reg1_r = { &reg1_bf, "r" };
static struct reg reg2_r = { &reg2_bf, "r" };
static struct reg reg3_r = { &reg3_bf, "r" };
#define REG1 atomreg, &reg1_r
#define REG2 atomreg, &reg2_r
#define REG3 atomreg, &reg3_r

static struct insn tabm[] = {
	{ -1, -1, 0x0000000000ull, 0x001f000000ull, U("00"), REG1, REG2, REG3 },
	{ -1, -1, 0x0001000000ull, 0x001f000000ull, U("01"), REG1, REG2, REG3 },
	{ -1, -1, 0x0003000000ull, 0x001f000000ull, U("03"), REG1, REG2, REG3 },
	{ -1, -1, 0x0006000000ull, 0x001f000000ull, U("06"), REG1, REG2, REG3 },
	{ -1, -1, 0x0007000000ull, 0x001f000000ull, U("07"), REG1, REG2, REG3 },
	{ -1, -1, 0x000c000000ull, 0x001f000000ull, U("0c"), REG1, REG2, REG3 },
	{ -1, -1, 0x000d000000ull, 0x001f000000ull, U("0d"), REG1, REG2, REG3 },
	{ -1, -1, 0x000e000000ull, 0x001f000000ull, U("0e"), REG1, REG2, REG3 },
	{ -1, -1, 0x000f000000ull, 0x001f000000ull, U("0f"), REG1, REG2, REG3 },
	{ -1, -1, 0x0010000000ull, 0x001f000000ull, U("10"), REG1, REG2, REG3 },
	{ -1, -1, 0x0011000000ull, 0x001f000000ull, U("11"), REG1, REG2, REG3 },
	{ -1, -1, 0x0013000000ull, 0x001f000000ull, U("13"), REG1, REG2, REG3 },
	{ -1, -1, 0x0000000000ull, 0x0000000000ull, OOPS, REG1, REG2, REG3 },
};

static struct insn tabroot[] = {
	{ -1, -1, 0, 0, OP40, T(m) },
};

static struct disisa vp3t_isa_s = {
	tabroot,
	5,
	5,
	5,
};

struct disisa *vp3t_isa = &vp3t_isa_s;
