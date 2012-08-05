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

#define F_NV41P	1

/*
 * Immediate fields
 */

static struct rbitfield imm16off = { 8, 16 };
#define IMM16 atomrimm, &imm16off
static struct rbitfield imm32off = { 8, 32 };
#define IMM32 atomrimm, &imm32off
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
	{ 0x00, 0xff, OP1B, N("nop") },
	{ 0x00, 0xc0, OP1B, N("wait"), WAIT, N("shl"), WAITS },
	{ 0x40, 0xff, OP3B, N("addrlo"), IMM16, .fmask = F_NV41P },
	{ 0x42, 0xff, OP3B, N("datalo"), IMM16, .fmask = F_NV41P },
	{ 0x5f, 0xff, OP3B, N("ewait"), T(evmask), T(event), .fmask = F_NV41P },
	{ 0x7f, 0xff, OP1B, N("exit") },
	{ 0x80, 0xe0, OP1B, N("unset"), T(fl) }, 
	{ 0xa0, 0xe0, OP1B, N("set1"), T(fl) }, 
	{ 0xc0, 0xe0, OP1B, N("set0"), T(fl) }, 
	{ 0xe0, 0xff, OP5B, N("addr"), IMM32, .fmask = F_NV41P },
	{ 0xe2, 0xff, OP5B, N("data"), IMM32, .fmask = F_NV41P },
	{ 0, 0, OP1B, OOPS },
};

static void hwsq_prep(struct disisa *isa) {
	isa->vardata = vardata_new("hwsq");
	int f_nv41op = vardata_add_feature(isa->vardata, "nv41op", "NV41+ opcodes");
	if (f_nv41op == -1)
		abort();
	int vs_chipset = vardata_add_varset(isa->vardata, "chipset", "GPU chipset");
	if (vs_chipset == -1)
		abort();
	int v_nv17 = vardata_add_variant(isa->vardata, "nv17", "NV17:NV41", vs_chipset);
	int v_nv41 = vardata_add_variant(isa->vardata, "nv41", "NV41+", vs_chipset);
	if (v_nv17 == -1 || v_nv41 == -1)
		abort();
	vardata_variant_feature(isa->vardata, v_nv41, f_nv41op);
	if (vardata_validate(isa->vardata))
		abort();
}

struct disisa hwsq_isa_s = {
	tabm,
	5,
	1,
	1,
	.prep = hwsq_prep,
};
