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
 * Options:
 *
 *  -b <base>	Use a fake base address
 *  -s <num>	Skip this many initial bytes of the input
 *  -l <num>	Limit disassembling to <num> bytes of input
 *  -w		Treat input as a sequence of 32-bit words instead of bytes
 *  -n		Disable color escape sequences in output
 */

int main(int argc, char **argv) {
	int ptype = AP;
	int w = 0;
	int c;
	unsigned base = 0, skip = 0, limit = 0;
	while ((c = getopt (argc, argv, "b:s:l:wn")) != -1)
		switch (c) {
			case 'b':
				sscanf(optarg, "%x", &base);
				break;
			case 's':
				sscanf(optarg, "%x", &skip);
				break;
			case 'l':
				sscanf(optarg, "%x", &limit);
				break;
			case 'w':
				w = 1;
				break;
			case 'n':
				cnorm = "";
				cgray = "";
				cgr = "";
				cbl= "";
				ccy = "";
				cyel = "";
				cred = "";
				cbr = "";
				cmag = "";
				cbrmag = "";
				break;
		}
	int num = 0;
	int maxnum = 16;
	uint8_t *code = malloc (maxnum * 4);
	uint32_t t;
	while (!feof(stdin) && scanf ("%x", &t) == 1) {
		if (num + 3 >= maxnum) maxnum *= 2, code = realloc (code, maxnum*4);
		if (w) {
			code[num++] = t & 0xff;
			t >>= 8;
			code[num++] = t & 0xff;
			t >>= 8;
			code[num++] = t & 0xff;
			t >>= 8;
			code[num++] = t & 0xff;
		} else
			code[num++] = t;
		scanf (" ,");
	}
	if (num <= skip)
		return 0;
	int cnt = num - skip;
	if (limit && limit < cnt)
		cnt = limit;
	fucdis (stdout, code+skip, base, cnt, ptype);
	return 0;
}
