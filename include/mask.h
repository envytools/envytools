/*
 * Copyright (C) 2011 Marcelina Kościelnicka <mwk@0x04.net>
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

#ifndef MASK_H
#define MASK_H

#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define MASK_CHUNK_SIZE 32
#define MASK_SIZE(num) CEILDIV((num), MASK_CHUNK_SIZE)

static inline int mask_get(uint32_t *mask, int num) {
	return (mask[num/32] >> (num % 32)) & 1;
}

static inline void mask_set(uint32_t *mask, int num) {
	mask[num/32] |= 1 << (num % 32);
}

static inline uint32_t *mask_new(int num) {
	uint32_t *res = calloc(sizeof *res, MASK_SIZE(num));
	return res;
}

static inline uint32_t *mask_dup(uint32_t *mask, int num) {
	uint32_t *res = mask_new(num);
	memcpy(res, mask, MASK_SIZE(num) * sizeof *res);
	return res;
}

void mask_or(uint32_t *dmask, uint32_t *smask, int size);
int mask_or_r(uint32_t *dmask, uint32_t *smask, int size);

int mask_intersect(uint32_t *a, uint32_t *b, int size);
int mask_contains(uint32_t *a, uint32_t *b, int size);
void mask_print(FILE *out, uint32_t *mask, int size);

#endif
