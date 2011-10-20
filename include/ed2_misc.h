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
#include <string.h>
#include <inttypes.h>

struct ed2_loc {
	int lstart;
	int cstart;
	int lend;
	int cend;
	const char *file;
};

#define ED2_LOC_FORMAT(loc, str) "%s:%d.%d-%d.%d: " str, (loc).file, (loc).lstart, (loc).cstart, (loc).lend, (loc).cend

#define ED2_MASK_CHUNK_SIZE 32
#define ED2_MASK_SIZE(num) CEILDIV((num), ED2_MASK_CHUNK_SIZE)

static inline int ed2_mask_get(uint32_t *mask, int num) {
	return (mask[num/32] >> (num % 32)) & 1;
}

static inline void ed2_mask_set(uint32_t *mask, int num) {
	mask[num/32] |= 1 << (num % 32);
}

static inline uint32_t *ed2_mask_new(int num) {
	uint32_t *res = calloc(sizeof *res, ED2_MASK_SIZE(num));
	return res;
}

static inline uint32_t *ed2_mask_dup(uint32_t *mask, int num) {
	uint32_t *res = ed2_mask_new(num);
	memcpy(res, mask, ED2_MASK_SIZE(num) * sizeof *res);
	return res;
}

void ed2_mask_or(uint32_t *dmask, uint32_t *smask, int size);
int ed2_mask_or_r(uint32_t *dmask, uint32_t *smask, int size);

int ed2_mask_intersect(uint32_t *a, uint32_t *b, int size);

void ed2_mask_print(FILE *out, uint32_t *mask, int size);

char *ed2_str_deescape(char *str, uint64_t *len);

struct ed2_astr {
	char *str;
	uint64_t len;
};

void ed2_free_strings(char **strs, int strsnum);

FILE *ed2_find_file(const char *name, const char *path, char **pfullname);

static inline int ed2_lg2ceil(unsigned num) {
	int i = 0;
	if (!num)
		return 0;
	while ((1u << i) - 1u < num - 1u) i++;
	return i;
}

#endif
