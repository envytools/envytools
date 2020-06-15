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

#define F_NV41P	1
#define F_NV17F	2
#define F_NV41F	4

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
static struct bitfield eventoff = { 8, 5 };
#define EVENT atomimm, &eventoff
static struct bitfield evaloff = { 16, 1 };
#define EVAL atomimm, &evaloff

static struct insn tabevent[] = {
	{ 0x0000, 0xff00, C("FB_PAUSED") },
	{ 0x0100, 0xff00, C("HEAD0_VBLANK") },
	{ 0x0200, 0xff00, C("HEAD0_HBLANK") },
	{ 0x0300, 0xff00, C("HEAD1_VBLANK") },
	{ 0x0400, 0xff00, C("HEAD1_HBLANK") },
	{ 0, 0, EVENT },
};

static struct insn tabfl[] = {
	{ 0x00, 0x1f, C("GPIO_2_OUT"), .fmask = F_NV17F },
	{ 0x01, 0x1f, C("GPIO_2_OE"), .fmask = F_NV17F },
	{ 0x02, 0x1f, C("GPIO_3_OUT"), .fmask = F_NV17F },
	{ 0x03, 0x1f, C("GPIO_3_OE"), .fmask = F_NV17F },
	{ 0x04, 0x1f, C("PRAMDAC0_UNK880_28"), .fmask = F_NV17F },
	{ 0x05, 0x1f, C("PRAMDAC1_UNK880_28"), .fmask = F_NV17F },
	{ 0x06, 0x1f, C("PRAMDAC0_UNK880_29"), .fmask = F_NV17F },
	{ 0x07, 0x1f, C("PRAMDAC1_UNK880_29"), .fmask = F_NV17F },
	{ 0x0e, 0x1f, C("GPIO_9_OUT"), .fmask = F_NV17F },
	{ 0x0f, 0x1f, C("GPIO_9_OE"), .fmask = F_NV17F },
	{ 0x10, 0x1f, C("FB_PAUSE"), .fmask = F_NV41P },
	{ 0x19, 0x1f, C("PWM_2_ENABLE"), .fmask = F_NV41F },
	{ 0x1a, 0x1f, C("PWM_1_ENABLE"), .fmask = F_NV41F },
	{ 0x1b, 0x1f, C("PWM_0_ENABLE"), .fmask = F_NV17F },
	{ 0x1c, 0x1f, C("PBUS_DEBUG_1_UNK22"), .fmask = F_NV17F },
	{ 0x1d, 0x1f, C("PBUS_DEBUG_1_UNK24"), .fmask = F_NV17F },
	{ 0x1e, 0x1f, C("PBUS_DEBUG_1_UNK26"), .fmask = F_NV17F },
	{ 0x1f, 0x1f, C("PBUS_DEBUG_1_UNK27"), .fmask = F_NV17F },
	{ 0, 0, FLAG },
};

static struct insn tabm[] = {
	{ 0x00, 0xff, OP1B, N("nop") },
	{ 0x00, 0xc0, OP1B, N("wait"), WAIT, N("shl"), WAITS },
	{ 0x40, 0xff, OP3B, N("addrlo"), IMM16, .fmask = F_NV41P },
	{ 0x42, 0xff, OP3B, N("datalo"), IMM16, .fmask = F_NV41P },
	{ 0x5f, 0xff, OP3B, N("ewait"), T(event), EVAL, .fmask = F_NV41P },
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
	int f_nv41op = vardata_add_feature(isa->vardata, "nv41op", "NV41+ features");
	int f_nv17f = vardata_add_feature(isa->vardata, "nv17f", "NV17:G80 flags");
	int f_nv41f = vardata_add_feature(isa->vardata, "nv41f", "NV41:G80 flags");
	if (f_nv41op == -1 || f_nv17f == -1 || f_nv41f == -1)
		abort();
	int vs_chipset = vardata_add_varset(isa->vardata, "chipset", "GPU chipset");
	if (vs_chipset == -1)
		abort();
	int v_nv17 = vardata_add_variant(isa->vardata, "nv17", "NV17:NV41", vs_chipset);
	int v_nv41 = vardata_add_variant(isa->vardata, "nv41", "NV41:G80", vs_chipset);
	int v_g80 = vardata_add_variant(isa->vardata, "g80", "G80+", vs_chipset);
	if (v_nv17 == -1 || v_nv41 == -1 || v_g80 == -1)
		abort();
	vardata_variant_feature(isa->vardata, v_nv17, f_nv17f);
	vardata_variant_feature(isa->vardata, v_nv41, f_nv17f);
	vardata_variant_feature(isa->vardata, v_nv41, f_nv41f);
	vardata_variant_feature(isa->vardata, v_nv41, f_nv41op);
	vardata_variant_feature(isa->vardata, v_g80, f_nv41op);
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
