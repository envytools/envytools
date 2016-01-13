/*
 * Copyright (C) 2016 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "nvhw/fp.h"


uint32_t fp32_minmax(uint32_t a, uint32_t b, bool min) {
	bool sa = FP32_SIGN(a);
	int ea = FP32_EXP(a);
	uint32_t fa = FP32_FRACT(a);
	bool sb = FP32_SIGN(b);
	int eb = FP32_EXP(b);
	uint32_t fb = FP32_FRACT(b);
	bool flip = min;
	/* Turns out min/max convert denormals to 0... */
	if (!ea)
		a &= ~0x7fffff;
	if (!eb)
		b &= ~0x7fffff;
	if (ea == FP32_MAXE && fa) {
		if (eb == FP32_MAXE && fb) {
			/* Both are NAN, return canonical NaN. */
			return 0x7fffffff;
		}
		/* a is NaN, return b */
		return b;
	} else if (eb == FP32_MAXE && fb) {
		/* b is NaN, return a */
		return a;
	} else if (sa != sb) {
		/* Different signs, pick the positive one */
		if (sa)
			flip = !flip;
		return flip ? b : a;
	} else {
		/* Same signs, compare exp & fract */
		if (sa)
			flip = !flip;
		if (a < b)
			flip = !flip;
		return flip ? b : a;
	}
}

int g80_fp32_cc(uint32_t x) {
	bool sign = FP32_SIGN(x);
	int exp = FP32_EXP(x);
	uint32_t fract = FP32_FRACT(x);
	if (exp == FP32_MAXE && fract) {
		/* NaN - Z and S set. */
		return 3;
	} else if (exp == 0) {
		/* Zero, or denormal (as good as 0) - Z set. */
		return 1;
	} else if (sign) {
		/* Negative finite or infinity - S set. */
		return 2;
	} else {
		/* Positive finite or infinity - nothing set. */
		return 0;
	}
}
