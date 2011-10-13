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

#ifndef ED2_MISC_H
#define ED2_MISC_H

#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

struct ed2_loc {
	int lstart;
	int cstart;
	int lend;
	int cend;
	const char *file;
};

#define ED2_LOC_FORMAT(loc, str) "%s:%d.%d-%d.%d: " str, (loc).file, (loc).lstart, (loc).cstart, (loc).lend, (loc).cend

char *ed2_str_deescape(char *str, uint64_t *len);

struct ed2_astr {
	char *str;
	uint64_t len;
};

void ed2_free_strings(char **strs, int strsnum);

FILE *ed2_find_file(const char *name, const char *path, char **pfullname);

#endif
