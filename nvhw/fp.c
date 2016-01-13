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
#include <stdlib.h>
#include <assert.h>


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

enum fp_cmp fp32_cmp(uint32_t a, uint32_t b) {
	bool sa = FP32_SIGN(a);
	int ea = FP32_EXP(a);
	uint32_t fa = FP32_FRACT(a);
	bool sb = FP32_SIGN(b);
	int eb = FP32_EXP(b);
	uint32_t fb = FP32_FRACT(b);
	if (ea == FP32_MAXE && fa) {
		/* a is NaN. */
		return FP_UN;
	} else if (eb == FP32_MAXE && fb) {
		/* b is NaN. */
		return FP_UN;
	} else if (ea == 0 && eb == 0) {
		/* Zeros (or denormals). */
		return FP_EQ;
	} else if (a == b) {
		/* Get the equality case out of the way. */
		return FP_EQ;
	} else if (sa != sb) {
		/* Different signs, both finite or infinity. */
		return sa ? FP_LT : FP_GT;
	} else {
		/* Same signs, both finite or infinity. */
		if ((a < b) ^ sa)
			return FP_LT;
		else
			return FP_GT;
	}
}

uint32_t fp32_add(uint32_t a, uint32_t b, enum fp_rm rm) {
	bool sa = FP32_SIGN(a);
	int ea = FP32_EXP(a);
	uint32_t fa = FP32_FRACT(a);
	bool sb = FP32_SIGN(b);
	int eb = FP32_EXP(b);
	uint32_t fb = FP32_FRACT(b);
	bool sr;
	int er;
	uint32_t fr;
	if (ea == FP32_MAXE && fa) {
		/* NaN+b is NaN. */
		sr = false;
		er = FP32_MAXE;
		fr = 0x7fffff;
	} else if (eb == FP32_MAXE && fb) {
		/* a+NaN is NaN. */
		sr = false;
		er = FP32_MAXE;
		fr = 0x7fffff;
	} else if (ea == FP32_MAXE) {
		/* a is +Inf or -Inf. */
		if (eb == FP32_MAXE && sa != sb) {
			/* Inf-Inf is NaN. */
			sr = false;
			er = FP32_MAXE;
			fr = 0x7fffff;
		} else {
			/* +-Inf+finite is +-Inf. */
			sr = sa;
			er = FP32_MAXE;
			fr = 0;
		}
	} else if (eb == FP32_MAXE) {
		/* finite+-Inf is +-Inf. */
		sr = sb;
		er = FP32_MAXE;
		fr = 0;
	} else {
		/* Two honest real numbers involved. */
		er = (ea > eb ? ea : eb);
		/* Implicit ones. */
		int32_t res = 0;
		int i;
		for (i = 0; i < 2; i++) {
			int e = i ? eb : ea;
			uint32_t f = i ? fb : fa;
			bool s = i ? sb : sa;
			if (e) {
				/* Non-0 */
				/* Add implicit one. */
				f |= FP32_IONE;
				int shr = er -e - 3;
				if (shr <= 0) {
					/* Same exponent as max, or close enough. */
					f <<= -shr;
				} else if (shr < 32) {
					/* We'll have a special shift to do. */
					bool sticky = !!(f & ((1 << shr) - 1));
					f >>= shr;
					f |= sticky;
				} else {
					/* Longer shift than acc width. Only sticky left. */
					f = 1;
				}
				/* Stuff it into the accumulator. */
				if (s)
					res -= f;
				else
					res += f;
			}
		}
		if (!res) {
			/* Got a proper 0. */
			er = 0;
			fr = 0;
			/* Make a -0 if both inputs were negative (ie. both -0). */
			sr = sa && sb;
		} else {
			/* Compute sign, make sure accumulator is positive. */
			sr = res < 0;
			if (sr)
				res = -res;
			/* 23 bits of input
			   +1 bit of implicit 0
			   +3 bits for proper rounding
			   +1 bit because we add two numbers
			  ---
			   28 bit accumulator.
			   Make sure bit 27 is actually set - normalize the number. */
			while (!(res & 1 << 27)) {
				res <<= 1;
				er--;
			}
			assert(res >= 16*FP32_IONE);
			assert(res < 32*FP32_IONE);
			/* Not supported yet... */
			if (rm == FP_RM || rm == FP_RP)
				abort();
			/* Round it. */
			int rest = res & 0xf;
			res >>= 4;
			if (rm == FP_RN) {
				if (rest > 8 || (rest == 8 && res & 1))
					res++;
			}
			assert(res >= FP32_IONE);
			assert(res <= 2*FP32_IONE);
			/* Accumulator is now 2.22, inputs were 1.23 - correct. */
			er++;
			/* Get rid of implicit one. */
			res -= FP32_IONE;
			/* We normalized it before, but rounding could bump it
			   to the next exponent. */
			if (res == FP32_IONE) {
				res = 0;
				er++;
			}
			if (er <= 0) {
				/* Well, underflow. Too bad. */
				er = 0;
				fr = 0;
			} else if (er >= FP32_MAXE) {
				/* Overflow. Likewise. */
				if (rm == FP_RZ) {
					er = FP32_MAXE-1;
					fr = FP32_IONE-1;
				} else {
					er = FP32_MAXE;
					fr = 0;
				}
			} else {
				fr = res;
			}
		}
	}
	return sr << 31 | er << 23 | fr;
}
