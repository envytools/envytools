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

static int stdoff[] = { 0xdeaddead };
static int staoff[] = { 0xdeaddead };
#define ADDR16 atomst16, staoff
#define ADDR32 atomst32, staoff
#define DATA16 atomst16, stdoff
#define DATA32 atomst32, stdoff
static void atomst16 APROTO {
	int *n = (int*)v;
	ull num = BF(8, 16);
	n[0] &= 0xffff0000;
	n[0] |= num;
	fprintf (out, " %s%#x", cyel, n[0]);
}
static void atomst32 APROTO {
	int *n = (int*)v;
	ull num = BF(8, 32);
	n[0] = num;
	fprintf (out, " %s%#x", cyel, n[0]);
}

static struct insn tabm[] = {
	{ AP, 0x40, 0xff, N("addr"), ADDR16 },
	{ AP, 0x42, 0xff, N("data"), DATA16 },
	{ AP, 0xe0, 0xff, N("addr"), ADDR32 },
	{ AP, 0xe2, 0xff, N("data"), DATA32 },
	{ AP, 0x7f, 0xff, N("exit") },
	{ AP, 0, 0, OOPS },
};

static uint32_t optab[] = {
	0x07, 1,
	0x0b, 1,
	0x40, 3,
	0x42, 3,
	0x5f, 3,
	0x7f, 1,
	0xb0, 1,
	0xd0, 1,
	0xe0, 5,
	0xe2, 5,
};

/*
 * Disassembler driver
 *
 * You pass a block of memory to this function, disassembly goes out to given
 * FILE*.
 */

void pmdis (FILE *out, uint8_t *code, int num, int ptype) {
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
			atomtab (out, &a, &m, tabm, ptype, cur);
			a &= ~m;
			if (a) {
				fprintf (out, " %s[unknown: %08llx]%s", cred, a, cnorm);
			}
			printf ("%s\n", cnorm);
			cur += length;
		}
	}
}
