/*
 * Copyright (C) 2015 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "nvhw/sfu.h"
#include "nvhw/fp.h"
#include <assert.h>

static uint32_t shr32(uint32_t x, int y, enum fp_rm rm, int sign) {
	int up = rm == (sign ? FP_RM : FP_RP);
	if (y > 31)
		return up;
	if (up) {
		x += (1 << y) - 1;
	} else if (rm == FP_RN) {
		x += (1 << (y-1)) - 1 + (x >> y & 1);
	}
	return x >> y;
}

static uint32_t sfu_square(uint32_t x) {
	int i;
	uint32_t res = 0;
	x >>= 1;
	for (i = 0; i < 16; i++) {
		if (x & 1 << i) {
			if (i < 14)
				res += x >> (14 - i);
			else
				res += x << (i - 14);
		}
	}
	return res >> 1;
}

uint32_t sfu_preex2(uint32_t x) {
	unsigned sign = FP32_SIGN(x);
	int exp = FP32_EXP(x);
	unsigned fract = FP32_FRACT(x);
	if (exp == FP32_MAXE) {
		if (fract) {
			/* NaN */
			return 0x40000000 | sign << 31;
		} else {
			/* Inf */
			return 0x40800000 | sign << 31;
		}
	} else if (exp == 0) {
		/* 0 or denormal - too small to matter */
		return sign << 31;
	} else {
		fract |= FP32_IONE;
		/* shift fract this many bits to the left */
		int shift = exp - FP32_MIDE;
		if (shift >= 7) {
			/* overflow, Inf */
			return 0x40800000 | sign << 31;
		} else if (shift >= 0) {
			/* ok, shift left */
			fract <<= shift;
		} else {
			/* ok, shift right */
			fract = shr32(fract, -shift, FP_RN, sign);
		}
		/* fract is now a 7.23 fractional number */
		return fract | sign << 31;
	}
}

uint32_t sfu_rcp(uint32_t x) {
	unsigned sign = FP32_SIGN(x);
	int exp = FP32_EXP(x);
	unsigned fract = FP32_FRACT(x);
	if (!exp) {
		/* 1/0 -> Inf */
		exp = FP32_MAXE;
		fract = 0;
	} else if (exp == FP32_MAXE) {
		if (fract == 0) {
			/* 1/Inf -> 0 */
			exp = 0;
		} else {
			/* 1/NaN -> NaN */
			sign = 0;
			exp = FP32_MAXE;
			fract = FP32_IONE - 1;
		}
	} else {
		exp = 2 * FP32_MIDE - exp - 1;
		int idx = fract >> 16;
		uint32_t x = fract & 0xffff;
		uint32_t sx = sfu_square(x);
		int64_t p1 = (int64_t)sfu_rcp_tab[idx][0] << 13;
		int64_t p2 = (int64_t)sfu_rcp_tab[idx][1] * x;
		int64_t p3 = (int64_t)sfu_rcp_tab[idx][2] * sx;
		fract = (p1 + p2 + p3 + 0x47e7) >> 15;
		assert(fract >= FP32_IONE);
		assert(fract <= 2*FP32_IONE);
		fract -= FP32_IONE;
		if (fract == FP32_IONE) {
			fract = 0;
			exp++;
		}
		if (exp <= 0) {
			/* denormal underflow, flush to zero */
			exp = 0;
			fract = 0;
		}
	}
	return sign << 31 | exp << 23 | fract;

}
