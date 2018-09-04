/*
 * Copyright (C) 2018 Rhys Perry <pendingchaos02@gmail.com>
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

#include "sched-intern.h"

static const char * const gm107_regfiles[] = {"r", "p", ""/*< flags register*/};
static const int gm107_regbitbuckets[] = {255, 7, -1};
static const int gm107_regminlatency[] = {0, 13, 0};

/* Mesa has many of the latencies of barrier-requiring instructions to 15.
   Here they're set to the lowest latency found to be used in tested or
   NVIDIA-generated code.
 */
static const struct schedinfo gm107_schedinfo[] = {
	{"mov", 0x5c98000000000000ull, {0, -1}, 6},
	{"mov", 0x4c98000000000000ull, {0, -1}, 6},
	{"mov", 0x3898000000000000ull, {0, -1}, 6},
	{"mov32i", 0x0100000000000000ull, {0, -1}, 6},
	{"iadd", 0x5c10000000000000ull, {0, -1}, 6},
	{"iadd", 0x4c10000000000000ull, {0, -1}, 6},
	{"iadd", 0x3810000000000000ull, {0, -1}, 6},
	{"iadd32i", 0x1d80000000000000ull, {0, -1}, 6},
	{"iadd32i", 0x1c00000000000000ull, {0, -1}, 6},
	{"imul", 0x5c38000000000000ull, {0, -1}, -6},
	{"imul", 0x4c38000000000000ull, {0, -1}, -6},
	{"imul", 0x3838000000000000ull, {0, -1}, -6},
	{"imad", 0x5a00000000000000ull, {0, -1}, -6},
	{"imad", 0x5200000000000000ull, {0, -1}, -6},
	{"imad", 0x4a00000000000000ull, {0, -1}, -6},
	{"imad", 0x3400000000000000ull, {0, -1}, -6},
	{"fadd", 0x5c58000000000000ull, {0, -1}, 6},
	{"fadd", 0x4c58000000000000ull, {0, -1}, 6},
	{"fadd", 0x3858000000000000ull, {0, -1}, 6},
	{"fmul", 0x5c68000000000000ull, {0, -1}, 6},
	{"fmul", 0x4c68000000000000ull, {0, -1}, 6},
	{"fmul", 0x3868000000000000ull, {0, -1}, 6},
	{"ffma", 0x5980000000000000ull, {0, -1}, 6},
	{"ffma", 0x5180000000000000ull, {0, -1}, 6},
	{"ffma", 0x4980000000000000ull, {0, -1}, 6},
	{"ffma", 0x3280000000000000ull, {0, -1}, 6},
	{"dmul", 0x5c80000000000000ull, {0, -1}, -13, .regsize={0, 2, 8, 2, 20, 2}},
	{"dmul", 0x4c80000000000000ull, {0, -1}, -13, .regsize={0, 2, 8, 2, 20, 2}},
	{"dmul", 0x3880000000000000ull, {0, -1}, -13, .regsize={0, 2, 8, 2, 20, 2}},
	{"dfma", 0x5b70000000000000ull, {0, -1}, -13, .regsize={0, 2, 8, 2, 20, 2, 39, 2}},
	{"dfma", 0x5370000000000000ull, {0, -1}, -13, .regsize={0, 2, 8, 2, 39, 2}},
	{"dfma", 0x4b70000000000000ull, {0, -1}, -13, .regsize={0, 2, 8, 2, 39, 2}},
	{"dfma", 0x3670000000000000ull, {0, -1}, -13, .regsize={0, 2, 8, 2, 39, 2}},
	{"flo", 0x5c30000000000000ull, {0, -1}, -13},
	{"flo", 0x4c30000000000000ull, {0, -1}, -13},
	{"flo", 0x3830000000000000ull, {0, -1}, -13},
	{"lop", 0x5c40000000000000ull, {0, 48, -1}, 6},
	{"lop", 0x4c40000000000000ull, {0, 48, -1}, 6},
	{"lop", 0x3840000000000000ull, {0, 48, -1}, 6},
	{"lop32i", 0x0400000000000000ull, {0, -1}, 6},
	{"shl", 0x5c48000000000000ull, {0, -1}, 6},
	{"shl", 0x4c48000000000000ull, {0, -1}, 6},
	{"shl", 0x3848000000000000ull, {0, -1}, 6},
	{"i2i", 0x5ce0000000000000ull, {0, -1}, -6},
	{"i2i", 0x4ce0000000000000ull, {0, -1}, -6},
	{"i2i", 0x38e0000000000000ull, {0, -1}, -6},
	/* f2f instructions with fp64 sources */
	{"f2f", 0x5ca8000000000000ull, {0, -1}, -13, .regsize={20, 2, -1}, .val=0x0000000000000c00ull, .mask=0x0000000000000c00ull},
	{"f2f", 0x4ca8000000000000ull, {0, -1}, -13, .val=0x0000000000000c00ull, .mask=0x0000000000000c00ull},
	{"f2f", 0x38a8000000000000ull, {0, -1}, -13, .val=0x0000000000000c00ull, .mask=0x0000000000000c00ull},
	/* f2f instructions with fp64 destinations */
	{"f2f", 0x5ca8000000000000ull, {0, -1}, -13, .regsize={0, 2, -1}, .val=0x0000000000000300ull, .mask=0x0000000000000300ull},
	{"f2f", 0x4ca8000000000000ull, {0, -1}, -13, .regsize={0, 2, -1}, .val=0x0000000000000300ull, .mask=0x0000000000000300ull},
	{"f2f", 0x38a8000000000000ull, {0, -1}, -13, .regsize={0, 2, -1}, .val=0x0000000000000300ull, .mask=0x0000000000000300ull},
	/* all f2f instructions */
	{"f2f", 0x5ca8000000000000ull, {0, -1}, -13},
	{"f2f", 0x4ca8000000000000ull, {0, -1}, -13},
	{"f2f", 0x38a8000000000000ull, {0, -1}, -13},
	{"iset", 0x5b50000000000000ull, {0, -1}, 6},
	{"iset", 0x4b50000000000000ull, {0, -1}, 6},
	{"iset", 0x3650000000000000ull, {0, -1}, 6},
	{"isetp", 0x5b60000000000000ull, {0, 3, -1}, 6},
	{"isetp", 0x4b60000000000000ull, {0, 3, -1}, 6},
	{"isetp", 0x3660000000000000ull, {0, 3, -1}, 6},
	{"dsetp", 0x5b80000000000000ull, {0, 3, -1}, -13, .regsize={8, 2, -1}},
	{"dsetp", 0x4b80000000000000ull, {0, 3, -1}, -13, .regsize={8, 2, -1}},
	{"dsetp", 0x3680000000000000ull, {0, 3, -1}, -13, .regsize={8, 2, -1}},
	{"bfe", 0x5c00000000000000ull, {0, -1}, 6},
	{"bfe", 0x4c00000000000000ull, {0, -1}, 6},
	{"bfe", 0x3800000000000000ull, {0, -1}, 6},
	{"bfi", 0x5bf0000000000000ull, {0, -1}, 6},
	{"bfi", 0x53f0000000000000ull, {0, -1}, 6},
	{"bfi", 0x4bf0000000000000ull, {0, -1}, 6},
	{"bfi", 0x36f0000000000000ull, {0, -1}, 6},
	{"xmad", 0x5b00000000000000ull, {0, -1}, 6},
	{"xmad", 0x5100000000000000ull, {0, -1}, 6},
	{"xmad", 0x4e00000000000000ull, {0, -1}, 6},
	{"iadd3", 0x5cc0000000000000ull, {0, -1}, 6},
	{"iadd3", 0x4cc0000000000000ull, {0, -1}, 6},
	{"iadd3", 0x38c0000000000000ull, {0, -1}, 6},
	{"ipa", 0xe00000000000ff00ull, {0, -1}, -15},
	{"ipa", 0xe000004000000000ull, {0, -1}, -15},
	{"mufu", 0x5080000000000000ull, {0, -1}, -13},
	{"tex", 0xdeb8000000000000ull, {0, -1}, -15},
	{"tex", 0xc038000000000000ull, {0, -1}, -15},
	{"depbar", 0xf0f0000000000000ull, {-1}, 0},
	{"ld", 0xefd8000000000000ull, {0, -1}, -15},
	{"st", 0xeff0000000000000ull, {-1}, -15},
	{"ld", 0xef90000000000000ull, {0, -1}, -15},
	{"nop", 0x50b0000000000000ull, {-1}, 0},
	{"ssy", 0xe290000000000020ull, {-1}, 0, .mindelay=13},
	{"ssy", 0xe290000000000000ull, {-1}, 0, .mindelay=13},
	{"sync", 0xf0f8000000000000ull, {-1}, 0, .controlflow=1, .mindelay=13},
	{"bra", 0xe240000000000000ull, {-1}, 0, .controlflow=1, .mindelay=13},
	{"ret", 0xe320000000000000ull, {-1}, 0, .controlflow=1, .mindelay=13},
	{"exit", 0xe300000000000000ull, {-1}, 0, .controlflow=1, .mindelay=15},
};

static unsigned gm107_getregsize(const struct insninfo *info, unsigned pos) {
	if (info->opcode == 0xdeb8000000000000ull || info->opcode == 0xc038000000000000ull) {
		/* tex */
		switch (pos) {
		case 0: {
			unsigned mask = (info->insn >> 31) & 0xf;
			/* use a LUT */
			return (0x8da691691448llu >> (mask * 3)) & 7;
		}
		case 8: {
			int shadow = (info->insn >> 50) & 1;
			int type = (info->insn >> 29) & 3; //0 = 1d, 1 = 2d, 2 = 3d, 3 = cube
			int array = (info->insn >> 28) & 1;
			assert(!shadow && "Shadow texture samples not handled"); /* TODO */
			return array + min(type + 1, 3);
		}
		}
	} else if (info->opcode == 0xefd8000000000000ull || info->opcode == 0xeff0000000000000ull) {
		/* attribute load/store instruction */
		if (pos == 0)
			return ((info->insn >> 47) & 3) + 1;
	} else if (info->opcode == 0xef90000000000000ull) {
		/* constant buffer load instruction */
		if (pos == 0)
			return ((info->insn >> 48) & 7) == 5 ? 2 : 1;
	}

	return 1;
}

static void gm107_atomcallback(struct insninfo *info, const struct atom *atom) {
	if (atom->fun_dis == atomname_d) {
		if (!strcmp(atom->arg, "cc")) {
			static const struct reginfo rinfo = {0, 2, -1, 1, -1};
			info->regs[info->regcount++] = rinfo;
		} else if (!strcmp(atom->arg, "x")) {
			static const struct reginfo rinfo = {0, 2, -1, 0, -1};
			info->regs[info->regcount++] = rinfo;
		}
	}
}

static int gm107_printsched(FILE *fp, int insnsnum, int remaining, struct schedinsn *insns) {
	if (insnsnum % 3)
		return 0;
	int num = min(remaining, 3);

	fprintf(fp, "sched");
	int i = 0;
	for (; i < num; i++) {
		fprintf(fp, " (st 0x%x", insns[i].delay);
		if (insns[i].wrbar != 7)
			fprintf(fp, " wr 0x%x", insns[i].wrbar);
		if (insns[i].rdbar != 7)
			fprintf(fp, " rd 0x%x", insns[i].rdbar);
		if (insns[i].wait)
			fprintf(fp, " wt 0x%x", insns[i].wait);
		fprintf(fp, ")");
	}
	for (; i < 3; i++)
		fprintf(fp, " (st 0x1)");
	fprintf(fp, "\n");

	return num;
}

static void gm107_printpaddingnops(FILE *fp, int insnsnum) {
	while (insnsnum % 3) {
		fprintf(fp, "nop 0x0\n");
		insnsnum++;
	}
}

struct schedtarget gm107_target = {
	.schedinfonum = ARRAY_SIZE(gm107_schedinfo),
	.schedinfo = gm107_schedinfo,

	.regfilesnum = ARRAY_SIZE(gm107_regfiles),
	.regfiles = gm107_regfiles,
	.regbitbuckets = gm107_regbitbuckets,
	.regminlatency = gm107_regminlatency,

	.numbarriers = 6,
	.barriermindelay = 2,

	.getregsize = &gm107_getregsize,
	.atomcallback = &gm107_atomcallback,
	.insncallback = NULL,

	.printsched = &gm107_printsched,
	.printpaddingnops = &gm107_printpaddingnops
};
