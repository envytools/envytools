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
#include <libgen.h>

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
	struct disisa *isa = 0;
	int w = 0, bin = 0, quiet = 0;
	argv[0] = basename(argv[0]);
	if (!strcmp(argv[0], "nv50dis")) {
		isa = nv50_isa;
		w = 1;
	}
	if (!strcmp(argv[0], "nvc0dis")) {
		isa = nvc0_isa;
		w = 1;
	}
	if (!strcmp(argv[0], "ctxdis")) {
		isa = ctx_isa;
		w = 1;
	}
	if (!strcmp(argv[0], "fucdis"))
		isa = fuc_isa;
	if (!strcmp(argv[0], "pmsdis"))
		isa = pms_isa;
	if (!strcmp(argv[0], "vp2dis"))
		isa = vp2_isa;
	if (!strcmp(argv[0], "vp3mdis"))
		isa = vp3m_isa;
	if (!strcmp(argv[0], "vp3tdis"))
		isa = vp3t_isa;
	if (!strcmp(argv[0], "macrodis")) {
		isa = macro_isa;
		w = 1;
	}
	int ptype = -1;
	int c;
	unsigned base = 0, skip = 0, limit = 0;
	while ((c = getopt (argc, argv, "45vgfpcsb:d:l:m:wWinq")) != -1)
		switch (c) {
			case '4':
				ptype = NV4x;
				break;
			case '5':
				ptype = NV5x;
				break;
			case 'v':
				ptype = VP;
				break;
			case 'g':
				ptype = GP;
				break;
			case 'f':
			case 'p':
				ptype = FP;
				break;
			case 'c':
				ptype = CP;
				break;
			case 's':
				ptype = VP|GP|FP;
				break;
			case 'b':
				sscanf(optarg, "%x", &base);
				break;
			case 'd':
				sscanf(optarg, "%x", &skip);
				break;
			case 'l':
				sscanf(optarg, "%x", &limit);
				break;
			case 'w':
				w = 1;
				break;
			case 'W':
				w = 2;
				break;
			case 'i':
				bin = 1;
				break;
			case 'q':
				quiet = 1;
				break;
			case 'n':
				cnorm = "";
				cname = "";
				creg0 = "";
				creg1 = "";
				cmem = "";
				cnum = "";
				cunk = "";
				cbtarg = "";
				cctarg = "";
				cbctarg = "";
				break;
			case 'm':
				if (!strcmp(optarg, "nv50"))
					isa = nv50_isa;
				else if (!strcmp(optarg, "nvc0"))
					isa = nvc0_isa;
				else if (!strcmp(optarg, "ctx"))
					isa = ctx_isa;
				else if (!strcmp(optarg, "fuc"))
					isa = fuc_isa;
				else if (!strcmp(optarg, "pms"))
					isa = pms_isa;
				else if (!strcmp(optarg, "vp2"))
					isa = vp2_isa;
				else if (!strcmp(optarg, "vp3m"))
					isa = vp3m_isa;
				else if (!strcmp(optarg, "vp3t"))
					isa = vp3t_isa;
				else if (!strcmp(optarg, "macro"))
					isa = macro_isa;
				else {
					fprintf (stderr, "Unknown architecure \"%s\"!\n", optarg);
					return 1;
				}
				break;
		}
	if (!isa) {
		fprintf (stderr, "No architecture specified!\n");
		return 1;
	}
	int num = 0;
	int maxnum = 16;
	uint8_t *code = malloc (maxnum);
	ull t;
	if (bin) {
		int c;
		while ((c = getchar()) != EOF) {
			if (num + 3 >= maxnum) maxnum *= 2, code = realloc (code, maxnum);
			code[num++] = c;
		}
	} else {
		while (!feof(stdin) && scanf ("%llx", &t) == 1) {
			if (num + 3 >= maxnum) maxnum *= 2, code = realloc (code, maxnum);
			if (w == 2) {
				code[num++] = t & 0xff;
				t >>= 8;
				code[num++] = t & 0xff;
				t >>= 8;
				code[num++] = t & 0xff;
				t >>= 8;
				code[num++] = t & 0xff;
				t >>= 8;
				code[num++] = t & 0xff;
				t >>= 8;
				code[num++] = t & 0xff;
				t >>= 8;
				code[num++] = t & 0xff;
				t >>= 8;
				code[num++] = t & 0xff;
			} else if (w) {
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
	}
	if (num <= skip)
		return 0;
	int cnt = num - skip;
	if (limit && limit < cnt)
		cnt = limit;
	envydis (isa, stdout, code+skip, base, cnt, ptype, quiet);
	return 0;
}
