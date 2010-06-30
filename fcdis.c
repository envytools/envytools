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

#include "coredis.h"

#define AP -1

/*
 * Code target fields
 */

#define SBTARG atomsbtarg, 0
void atomsbtarg APROTO {
	uint32_t delta = BF(16, 8);
	if (delta & 0x80) delta += 0xffffff00;
	fprintf (out, " %s%#x", cbr, pos + delta);
}

#define LBTARG atomlbtarg, 0
void atomlbtarg APROTO {
	uint32_t delta = BF(16, 16);
	if (delta & 0x8000) delta += 0xffff0000;
	fprintf (out, " %s%#x", cbr, pos + delta);
}

#define CTARG atomctarg, 0
void atomctarg APROTO {
	fprintf (out, " %s%#llx", cbr, BF(16, 16));
}

/*
 * Register fields
 */

int reg1off[] = { 8, 4, 'r' };
int reg2off[] = { 12, 4, 'r' };
#define REG1 atomreg, reg1off
#define REG2 atomreg, reg2off

/*
 * Immediate fields
 */

int imm16off[] = { 16, 16, 0, 0 };
int imm8off[] = { 16, 8, 0, 0 };
int imm16soff[] = { 16, 16, 0, 1 };
int imm8soff[] = { 16, 8, 0, 1 };
int imm16hoff[] = { 16, 16, 16, 0 };
int imm8hoff[] = { 16, 8, 16, 0 };
#define IMM16 atomnum, imm16off
#define IMM8 atomnum, imm8off
#define IMM16S atomnum, imm16soff
#define IMM8S atomnum, imm8soff
#define IMM16H atomnum, imm16hoff
#define IMM8H atomnum, imm8hoff

/*
 * Memory fields
 */

int data8off[] = { 0 };
int data16off[] = { 1 };
int data32off[] = { 2 };
#define DATA8 atomdata, data8off
#define DATA16 atomdata, data16off
#define DATA32 atomdata, data32off
void atomdata APROTO {
	const int *n = v;
	fprintf (out, " %sD[%s$r%lld%s+%s%#llx%s]", ccy, cbl, BF(12, 4), ccy, cyel, BF(16,8) << n[0], ccy);
}

struct insn tabp[] = {
	{ AP, 0x00000800, 0x00001f00, N("lt") },
	{ AP, 0x00000b00, 0x00001f00, N("eq") },
	{ AP, 0x00000c00, 0x00001f00, N("gt") },
	{ AP, 0x00000d00, 0x00001f00, N("le") },
	{ AP, 0x00000e00, 0x00001f00 }, /* always true */
	{ AP, 0x00001800, 0x00001f00, N("ge") },
	{ AP, 0x00001b00, 0x00001f00, N("ne") },
	{ AP, 0, 0, OOPS },
};

struct insn tabaop[] = {
	{ AP, 0x00000000, 0x00000f00, N("add") },
	{ AP, 0x00000100, 0x00000f00, N("adc") },
	{ AP, 0x00000200, 0x00000f00, N("sub") },
	{ AP, 0x00000300, 0x00000f00, N("sbb") },
	{ AP, 0x00000400, 0x00000f00, N("shl") },
	{ AP, 0x00000500, 0x00000f00, N("shr") },
	{ AP, 0x00000700, 0x00000f00, N("sar") },
	{ AP, 0x00000c00, 0x00000f00, N("shlc") },
	{ AP, 0x00000d00, 0x00000f00, N("shrc") },
};

struct insn tabi[] = {
	{ AP, 0x00000000, 0x00000001, IMM8 },
	{ AP, 0x00000001, 0x00000001, IMM16 },
};

struct insn tabis[] = {
	{ AP, 0x00000000, 0x00000001, IMM8S },
	{ AP, 0x00000001, 0x00000001, IMM16S },
};

struct insn tabih[] = {
	{ AP, 0x00000000, 0x00000001, IMM8H },
	{ AP, 0x00000001, 0x00000001, IMM16H },
};

struct insn tabbt[] = {
	{ AP, 0x00000000, 0x00000001, SBTARG },
	{ AP, 0x00000001, 0x00000001, LBTARG },
};

struct insn tabrr[] = {
	{ AP, 0x00000000, 0x00100000, REG2, REG1 },
	{ AP, 0x00100000, 0x00100000, REG1, REG2 },
};

struct insn tabm[] = {
	{ AP, 0x00000000, 0x000000ff, N("stb"), DATA8, REG1 },

	{ AP, 0x00000018, 0x000000ff, N("ldb"), REG1, DATA16 },

	{ AP, 0x00000040, 0x000000ff, N("sth"), DATA16, REG1 },

	{ AP, 0x00000058, 0x000000ff, N("ldh"), REG1, DATA16 },

	{ AP, 0x00000080, 0x000000ff, N("st"), DATA32, REG1 },

	{ AP, 0x00000098, 0x000000ff, N("ld"), REG1, DATA32 },

	{ AP, 0x000004b0, 0x00000ffe, N("cmpu"), REG2, T(i) },
	{ AP, 0x000005b0, 0x00000ffe, N("cmps"), REG2, T(is) },

	{ AP, 0x000000b6, 0x000000fe, T(aop), REG2, T(i) },

	{ AP, 0x000400b8, 0x000f00ff, N("cmpu"), REG2, REG1 },
	{ AP, 0x000500b8, 0x000f00ff, N("cmps"), REG2, REG1 },

	{ AP, 0x000000f0, 0x00000ffe, N("mulu"), REG2, T(i) },
	{ AP, 0x000001f0, 0x00000ffe, N("muls"), REG2, T(is) },
	{ AP, 0x000002f0, 0x00000ffe, N("sex"), REG2, T(i) }, /* funky instruction. bits ARG2+1 through 31 of ARG1 are replaced with copy of bit ARG2. */
	{ AP, 0x000003f0, 0x00000ffe, N("sethi"), REG2, T(ih) },
	{ AP, 0x000004f0, 0x00000ffe, N("and"), REG2, T(i) },
	{ AP, 0x000005f0, 0x00000ffe, N("or"), REG2, T(i) },
	{ AP, 0x000006f0, 0x00000ffe, N("xor"), REG2, T(i) },
	{ AP, 0x000007f0, 0x00000ffe, N("mov"), REG2, T(is) },
	{ AP, 0x000009f0, 0x00000ffe, N("bset"), REG2, T(i) },
	{ AP, 0x00000af0, 0x00000ffe, N("bclr"), REG2, T(i) },
	{ AP, 0x00000bf0, 0x00000ffe, N("btgl"), REG2, T(i) },
	/* XXX: 00000cf0 */

	{ AP, 0x000000f4, 0x0000e0fe, N("bra"), T(p), T(bt) },
	{ AP, 0x000021f5, 0x0000ffff, N("call"), CTARG },
	{ AP, 0x000030f4, 0x0000fffe, N("add"), N("sp"), T(is) },

	{ AP, 0x000000f8, 0x0000ffff, N("ret") },
	{ AP, 0x000002f8, 0x0000ffff, N("exit") },

	{ AP, 0x000000f9, 0x00000fff, N("push"), REG2 },
	{ AP, 0x000005f9, 0x00000fff, N("call"), REG2 },

	{ AP, 0x000000fc, 0x00000fff, N("pop"), REG2 },

	{ AP, 0x000000ff, 0x00ef00ff, N("mulu"), T(rr) },
	{ AP, 0x000100ff, 0x00ef00ff, N("muls"), T(rr) },
	{ AP, 0x000200ff, 0x00ff00ff, N("sex"), T(rr) },
	{ AP, 0x001200ff, 0x00ff00ff, N("sexr"), T(rr) }, /* SRC args reversed */
	{ AP, 0x000400ff, 0x00ef00ff, N("and"), T(rr) },
	{ AP, 0x000500ff, 0x00ef00ff, N("or"), T(rr) },
	{ AP, 0x000600ff, 0x00ef00ff, N("xor"), T(rr) },
	{ AP, 0x000800ff, 0x00ff00ff, N("xbit"), REG2, REG1 }, /* ARG1 = (ARG1 & 0xfffffffe) | (ARG1 >> ARG2 & 1) */
	{ AP, 0x001800ff, 0x00ff00ff, N("xbitr"), REG1, REG2 }, /* ARG1 = (ARG1 & 0xfffffffe) | (ARG2 >> ARG1 & 1) */
	{ AP, 0, 0, OOPS },
};

uint32_t optab[] = {
	0x00, 3,
	0x18, 3,
	0x40, 3,
	0x58, 3,
	0x78, 3,
	0x80, 3,
	0x90, 3,
	0x92, 3,
	0x94, 3,
	0x95, 3,
	0x97, 3,
	0x98, 3,
	0xa0, 4,
	0xb0, 3,
	0xb1, 4,
	0xb4, 3,
	0xb6, 3,
	0xb7, 4,
	0xb8, 3,
	0xb9, 3,
	0xbb, 3,
	0xbc, 3,
	0xbd, 2,
	0xc4, 3,
	0xc5, 3,
	0xc7, 3,
	0xcf, 3,
	0xe3, 4,
	0xe4, 4,
	0xe7, 4,
	0xf0, 3,
	0xf1, 4,
	0xf4, 3,
	0xf5, 4,
	0xf8, 2,
	0xf9, 2,
	0xfa, 3,
	0xfc, 2,
	0xfd, 3,
	0xfe, 3,
	0xff, 3,
};

/*
 * Disassembler driver
 *
 * You pass a block of memory to this function, disassembly goes out to given
 * FILE*.
 */

void fcdis (FILE *out, uint8_t *code, int num) {
	int cur = 0, i;
	while (cur < num) {
		fprintf (out, "%s%08x:%s", cgray, cur, cnorm);
		uint8_t op = code[cur];
		int length = 0;
		for (i = 0; i < sizeof optab / sizeof *optab / 2; i++)
			if (op == optab[2*i])
				length = optab[2*i+1];
		if (!length || cur + length > num) {
			fprintf (out, " %s%02x                ???%s\n", cred, op, cnorm);
			cur++;
		} else {
			ull a = 0, m = 0;
			for (i = cur; i < cur + length; i++) {
				fprintf (out, " %02x", code[i]);
				a |= (ull)code[i] << (i-cur)*8;
			}
			for (i = 0; i < 6 - length; i++)
				fprintf (out, "   ");
			atomtab (out, &a, &m, tabm, -1, cur);
			a &= ~m;
			if (a) {
				fprintf (out, " %s[unknown: %08llx]%s", cred, a, cnorm);
			}
			printf ("%s\n", cnorm);
			cur += length;
		}
	}
}

/*
 * Options:
 *
 *  -b  	Read input as binary ctxprog
 *  -4		Disassembles NV40 ctxprogs
 *  -5		Disassembles NV50 ctxprogs
 *  -n		Disable color escape sequences in output
 */

int main(int argc, char **argv) {
	int num = 0;
	int maxnum = 16;
	uint8_t *code = malloc (maxnum * 4);
	uint32_t t;
	while (!feof(stdin) && scanf ("%x", &t) == 1) {
		if (num == maxnum) maxnum *= 2, code = realloc (code, maxnum*4);
		code[num++] = t;
		scanf (" ,");
	}
	fcdis (stdout, code, num);
	return 0;
}
