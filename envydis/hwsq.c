/*
 *
 * Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
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

#define NV17 1
#define NV41 2

/*
 * Immediate fields
 */

static struct bitfield imm16off = { 8, 16 };
#define IMM16 atomimm, &imm16off
static struct bitfield imm32off = { 8, 32 };
#define IMM32 atomimm, &imm32off
static struct bitfield maskoff = { 8, 8 };
#define EVMASK atomimm, &maskoff
static struct bitfield flagoff = { 0, 5 };
#define FLAG atomimm, &flagoff
static struct bitfield waitoff = { 0, 2 };
#define WAIT atomimm, &waitoff
static struct bitfield waitsoff = { 2, 4, .shr = 1 };
#define WAITS atomimm, &waitsoff

F1(vblank0, 8, N("vblank0"))
F1(vblank1, 9, N("vblank1"))

static struct insn tabevmask[] = {
	{ 0, 0, T(vblank0), T(vblank1) },
};

static struct insn tabevent[] = {
	{ 0x000000, 0xff0000, N("all0") },
	{ 0x010000, 0xff0000, N("all1") },
	{ 0, 0, OOPS },
};

static struct insn tabfl[] = {
	{ 0x10, 0x1f, N("buspause") },
	{ 0, 0, FLAG },
};

static struct insn tabm[] = {
	{ 0x00, 0xff, OP8, N("nop") },
	{ 0x00, 0xc0, OP8, N("wait"), WAIT, N("shl"), WAITS },
	{ 0x40, 0xff, OP24, N("addrlo"), IMM16, .vartype = NV41 },
	{ 0x42, 0xff, OP24, N("datalo"), IMM16, .vartype = NV41 },
	{ 0x5f, 0xff, OP24, N("ewait"), T(evmask), T(event), .vartype = NV41 },
	{ 0x7f, 0xff, OP8, N("exit") },
	{ 0x80, 0xe0, OP8, N("unset"), T(fl) }, 
	{ 0xa0, 0xe0, OP8, N("set1"), T(fl) }, 
	{ 0xc0, 0xe0, OP8, N("set0"), T(fl) }, 
	{ 0xe0, 0xff, OP40, N("addr"), IMM32, .vartype = NV41 },
	{ 0xe2, 0xff, OP40, N("data"), IMM32, .vartype = NV41 },
	{ 0, 0, OP8, OOPS },
};

static const struct disvariant hwsq_vars[] = {
	"nv17", NV17,
	"nv41", NV41,
};

const struct disisa hwsq_isa_s = {
	tabm,
	5,
	1,
	1,
	.vars = hwsq_vars,
	.varsnum = sizeof hwsq_vars / sizeof *hwsq_vars,
};
