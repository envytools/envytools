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
			fprintf (out, " %s%02x          ???%s", cred, op, cnorm);
			cur++;
		} else {
			for (i = cur; i < cur + length; i++)
				fprintf (out, " %02x", code[i]);
			for (i = 0; i < 4 - length; i++)
				fprintf (out, "   ");
			fprintf (out, " ");
			if ((op == 0xf0 || op == 0xf1) && (code[cur+1] & 0xb) == 3) {
				int reg = code[cur+1]>>4;
				int imm = code[cur+2];
				/* hmmm... maybe 0xf0 does sign-extension? */
				if (op == 0xf1)
					imm += code[cur+3] << 8;
				if (!(code[cur+1]&4))
					imm <<= 16;
				fprintf(out, " %s%s%s", cgr, code[cur+1]&4?"limm":"lhigh", cnorm);
				fprintf(out, " %s$r%d%s", cbl, reg, cnorm);
				fprintf(out, " %s%#x%s", cyel, imm, cnorm);
			} else if (op == 0xf5 && code[cur+1] == 0x21) {
				fprintf(out, " %s%s %s%#x%s", cgr, "call", cbr, code[cur+3]<<8 | code[cur+2], cnorm);
			} else if (op == 0xf8 && code[cur+1] == 0) {
				fprintf(out, " %s%s%s", cgr, "ret", cnorm);
			} else {
				fprintf(out, " %s???%s", cred, cnorm);
			}
			cur += length;
		}
		printf ("\n");
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
