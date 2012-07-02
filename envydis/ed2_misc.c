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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ed2_misc.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static int gethex(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 0xa;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 0xa;
	assert(0);
}

char *ed2_str_deescape(char *str, uint64_t *len) {
	int rlen = 0;
	int i;
	for (i = 0; str[i]; i++) {
		if (str[i] == '\\') {
			i++;
			rlen++;
			if (str[i] == 'x')
				i += 2;
		} else if (str[i] != '"') {
			rlen++;
		}
	}
	char *res = malloc (rlen + 1);
	int j;
	for (i = 0, j = 0; str[i]; i++) {
		if (str[i] == '\\') {
			i++;
			switch (str[i]) {
				case '\\':
				case '\'':
				case '\"':
				case '\?':
					res[j++] = str[i];
					break;
				case 'n':
					res[j++] = '\n';
					break;
				case 'f':
					res[j++] = '\f';
					break;
				case 't':
					res[j++] = '\t';
					break;
				case 'a':
					res[j++] = '\a';
					break;
				case 'v':
					res[j++] = '\v';
					break;
				case 'r':
					res[j++] = '\r';
					break;
				case 'x':
					res[j++] = gethex(str[i+1]) << 4 | gethex(str[i+2]);
					i += 2;
					break;
				default:
					assert(0);
			}
			if (str[i] == 'x')
				i += 2;
		} else if (str[i] != '"') {
			res[j++] = str[i];
		}
	}
	res[j] = 0;
	assert(j == rlen);
	*len = rlen;
	return res;
}

void ed2_free_strings(char **strs, int strsnum) {
	int i;
	for (i = 0; i < strsnum; i++)
		free(strs[i]);
	free(strs);
}
