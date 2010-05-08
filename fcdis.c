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

int limmoff[] = { 16, 16, 0, 0 };
int simmoff[] = { 16, 8, 0, 1 };
int limmhoff[] = { 16, 16, 16, 0 };
int simmhoff[] = { 16, 8, 16, 1 };
#define LIMM atomnum, limmoff
#define SIMM atomnum, simmoff
#define LIMMH atomnum, limmhoff
#define SIMMH atomnum, simmhoff

struct insn tabp[] = {
	{ AP, 0x00000e00, 0x00001f00 }, /* always true */
	{ AP, 0, 0, OOPS },
};

struct insn tabm[] = {
	{ AP, 0x000000f4, 0x0000e0ff, N("bra"), T(p), SBTARG },
	{ AP, 0x000000f5, 0x0000e0ff, N("bra"), T(p), LBTARG },
	{ AP, 0x000021f5, 0x0000ffff, N("call"), CTARG },
	{ AP, 0x000000f8, 0x0000ffff, N("ret") },
	{ AP, 0x000003f0, 0x00000fff, N("lhigh"), REG2, SIMMH },
	{ AP, 0x000003f1, 0x00000fff, N("lhigh"), REG2, LIMMH },
	{ AP, 0x000007f0, 0x00000fff, N("limm"), REG2, SIMM },
	{ AP, 0x000007f1, 0x00000fff, N("limm"), REG2, LIMM },
	{ AP, 0x000000b6, 0x00000fff, N("add"), REG2, SIMM },
	{ AP, 0x000000b7, 0x00000fff, N("add"), REG2, LIMM },
	{ AP, 0x000002b6, 0x00000fff, N("sub"), REG2, SIMM },
	{ AP, 0x000002b7, 0x00000fff, N("sub"), REG2, LIMM },
	{ AP, 0, 0, OOPS },
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
		switch (op) {
			case 0xa0:
			case 0xb1:
			case 0xb7:
			case 0xe3:
			case 0xe4:
			case 0xe7:
			case 0xf1:
			case 0xf5:
				length = 4;
				break;
			case 0x80:
			case 0x90:
			case 0x92:
			case 0x94:
			case 0x95:
			case 0x97:
			case 0x98:
			case 0xb0:
			case 0xb4:
			case 0xb6:
			case 0xb8:
			case 0xb9:
			case 0xbb:
			case 0xbc:
			case 0xc4:
			case 0xc5:
			case 0xc7:
			case 0xf0:
			case 0xfa:
			case 0xf4:
			case 0xfd:
			case 0xfe:
			case 0xff:
				length = 3;
				break;
			case 0xbd:
			case 0xf8:
			case 0xf9:
			case 0xfc:
				length = 2;
				break;
		}
		if (!length || cur + length > num) {
			fprintf (out, " %s%02x           ???%s\n", cred, op, cnorm);
			cur++;
		} else {
			ull a = 0, m = 0;
			for (i = cur; i < cur + length; i++) {
				fprintf (out, " %02x", code[i]);
				a |= (ull)code[i] << (i-cur)*8;
			}
			for (i = 0; i < 4 - length; i++)
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
