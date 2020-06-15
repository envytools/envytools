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

#include "mask.h"

void mask_or(uint32_t *dmask, uint32_t *smask, int size) {
	int rsize = MASK_SIZE(size);
	int i;
	for (i = 0; i < rsize; i++)
		dmask[i] |= smask[i];
}
int mask_or_r(uint32_t *dmask, uint32_t *smask, int size) {
	int rsize = MASK_SIZE(size);
	int i;
	int res = 0;
	for (i = 0; i < rsize; i++) {
		uint32_t n = dmask[i] | smask[i];
		if (n != dmask[i])
			res = 1;
		dmask[i] = n;
	}
	return res;
}

int mask_intersect(uint32_t *a, uint32_t *b, int size) {
	int rsize = MASK_SIZE(size);
	int i;
	for (i = 0; i < rsize; i++)
		if (a[i] & b[i]) {
			int j;
			for (j = 0; j < MASK_CHUNK_SIZE; j++)
				if (a[i] & b[i] & (uint32_t)1 << j)
					return j;
		}
	return -1;
}

int mask_contains(uint32_t *a, uint32_t *b, int size) {
	int rsize = MASK_SIZE(size);
	int i;
	for (i = 0; i < rsize; i++)
		if ((a[i] & b[i]) != b[i]) {
			return 0;
		}
	return 1;
}

void mask_print(FILE *out, uint32_t *mask, int size) {
	int rsize = MASK_SIZE(size);
	int i;
	for (i = 0; i < rsize; i++) {
		if (i)
			fprintf(out, " ");
		fprintf (out, "%08"PRIx32, mask[i]);
	}
}
