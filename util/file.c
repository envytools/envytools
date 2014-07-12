/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

FILE *open_input(const char *filename) {
	const char * const tab[][2] = {
		{ ".gz", "zcat" },
		{ ".Z", "zcat" },
		{ ".bz2", "bzcat" },
		{ ".xz", "xzcat" },
	};
	int i;
	int flen = strlen(filename);
	for (i = 0; i < sizeof tab / sizeof tab[0]; i++) {
		int elen = strlen(tab[i][0]);
		if (flen > elen && !strcmp(filename + flen - elen, tab[i][0])) {
			char *cmd = malloc(flen + strlen(tab[i][1]) + 2);
			FILE *res;
			strcpy(cmd, tab[i][1]);
			strcat(cmd, " ");
			strcat(cmd, filename);
			res = popen(cmd, "r");
			free(cmd);
			return res;
		}
	}
	return fopen(filename, "r");
}
