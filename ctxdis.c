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
 *  -b  	Read input as binary ctxprog
 *  -4		Disassembles NV40 ctxprogs
 *  -5		Disassembles NV50 ctxprogs
 *  -n		Disable color escape sequences in output
 */

int main(int argc, char **argv) {
	int ptype = NV5x;
	int binary = 0;
	char c;
	while ((c = getopt (argc, argv, "b45n")) != -1)
		switch (c) {
			case 'b':
				binary = 1;
				break;
			case '4':
				ptype = NV4x;
				break;
			case '5':
				ptype = NV5x;
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
				break;
		}
	int num = 0;
	int maxnum = 16;
	uint32_t *code = malloc (maxnum * 4);
	uint32_t t;
	if(binary) {
		uint8_t hdr[7];
		uint32_t signature;
		uint8_t version;
		uint16_t length;
		uint8_t b[4];

		if(fread(&hdr, 7, 1, stdin) != 1) {
			fprintf(stderr, "unable to read the ctxprog header\n");
			return 1;
		}
		
		signature = readle32(hdr);
		version = hdr[4];
		length = readle16(hdr+5);

		if(signature != 0x5043564e || version != 0) {
			fprintf(stderr, "invalid ctxprog header\n");
			return 1;
		}

		while (num < length && !feof(stdin) && fread(&b, 4, 1, stdin) == 1) {
			if (num == maxnum) maxnum *= 2, code = realloc (code, maxnum*4);
			code[num++] = readle32(b);
		}

		if(num < length)
			fprintf(stderr, "input ctxprog was truncated\n");
	} else
		while (!feof(stdin) && scanf ("%x", &t) == 1) {
			if (num == maxnum) maxnum *= 2, code = realloc (code, maxnum*4);
			code[num++] = t;
			scanf (" ,");
		}

	ctxdis (stdout, code, num, ptype);
	return 0;
}
