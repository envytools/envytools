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

/* Shifts x right by shift bits, rounding according to given rounding mode.
   If shift is negative, shifts left instead.  Handles very large shifts
   correctly.  */

static uint32_t shr32(uint32_t x, int shift, enum fp_rm rm) {
	if (shift == 0) {
		return x;
	} else if (shift > 0) {
		uint32_t rest;
		/* 0: less than half,
		 * 1: exactly half,
		 * 2: greater than half
		 */
		int rncase;
		if (shift >= 32) {
			rest = x;
			x = 0;
			if (shift > 32) {
				/* With that much of a shift, it's < 0.5 */
				rncase = 0;
			} else if (rest < 0x80000000) {
				rncase = 0;
			} else if (rest == 0x80000000) {
				rncase = 1;
			} else {
				rncase = 2;
			}
		} else {
			rest = x & ((1 << shift) - 1);
			x >>= shift;
			if (rest < (1 << (shift - 1))) {
				rncase = 0;
			} else if (rest == (1 << (shift - 1))) {
				rncase = 1;
			} else {
				rncase = 2;
			}
		}
		if (rm == FP_RP) {
			if (rest)
				x++;
		} else if (rm == FP_RN) {
			if (rncase == 2 || (rncase == 1 && x & 1))
				x++;
		} else if (rm == FP_RT) {
			if (rest)
				x |= 1;
		}
		return x;
	} else if (shift <= -32) {
		if (x == 0)
			return 0;
		else
			abort();
	} else {
		if (x << -shift >> -shift != x)
			abort();
		return x << -shift;
	}
}

/* Likewise, for 64-bit numbers.  */

static uint64_t shr64(uint64_t x, int shift, enum fp_rm rm) {
	if (shift == 0) {
		return x;
	} else if (shift > 0) {
		uint64_t rest;
		/* 0: less than half,
		 * 1: exactly half,
		 * 2: greater than half
		 */
		int rncase;
		if (shift >= 64) {
			rest = x;
			x = 0;
			if (shift > 64) {
				/* With that much of a shift, it's < 0.5 */
				rncase = 0;
			} else if (rest < (1ull << 63)) {
				rncase = 0;
			} else if (rest == (1ull << 63)) {
				rncase = 1;
			} else {
				rncase = 2;
			}
		} else {
			rest = x & ((1ull << shift) - 1);
			x >>= shift;
			if (rest < (1ull << (shift - 1))) {
				rncase = 0;
			} else if (rest == (1ull << (shift - 1))) {
				rncase = 1;
			} else {
				rncase = 2;
			}
		}
		if (rm == FP_RP) {
			if (rest)
				x++;
		} else if (rm == FP_RN) {
			if (rncase == 2 || (rncase == 1 && x & 1))
				x++;
		} else if (rm == FP_RT) {
			if (rest)
				x |= 1;
		}
		return x;
	} else if (shift <= -64) {
		if (x == 0)
			return 0;
		else
			abort();
	} else {
		if (x << -shift >> -shift != x)
			abort();
		return x << -shift;
	}
}

/* Shifts a number left until given bit is the leftmost set bit.  If number
   has higher bits set, or is 0, causes an error.  Decreases *e by number
   of shifts done.  */

static uint32_t norm32(uint32_t x, int *e, int bit) {
	if (x & ~((2 << bit) - 1))
		abort();
	if (!x)
		abort();
	while (!(x & 1 << bit)) {
		x <<= 1;
		(*e)--;
	}
	return x;
}

static uint64_t norm64(uint64_t x, int *e, int bit) {
	if (x & ~((2ull << bit) - 1))
		abort();
	if (!x)
		abort();
	while (!(x & 1ull << bit)) {
		x <<= 1;
		(*e)--;
	}
	return x;
}

/* Assembles a fp32 finite number.  Subtracts implicit one from f, bumps
   exponent if rounding caused f to be 2*FP32_IONE.  f must be normalized,
   unless e == 1 (which means a denormal), or e < 0 (which gives 0).  */

static uint32_t fp32_mkfin(bool s, int e, uint32_t f, enum fp_rm rm) {
	if (e > 1 && f < FP32_IONE)
		abort();
	if (f > 2*FP32_IONE)
		abort();
	if (f == 2*FP32_IONE) {
		f >>= 1;
		e++;
	}
	if (e <= 0 || f < FP32_IONE) {
		e = 0;
		f = 0;
	} else if (e >= FP32_MAXE) {
		if (rm == FP_RZ || fp_adjust_rm(rm, s) == FP_RM) {
			e = FP32_MAXE-1;
			f = FP32_IONE-1;
		} else {
			e = FP32_MAXE;
			f = 0;
		}
	} else {
		f -= FP32_IONE;
	}
	return FP32_MAKE(s, e, f);
}

static void fp32_parsefin(uint32_t x, bool *ps, int *pe, uint32_t *pf, bool ftz) {
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	*ps = sx;
	if (!ex) {
		*pe = 1;
		*pf = ftz ? 0 : fx;
	} else {
		*pe = ex;
		*pf = FP32_IONE + fx;
	}
}

uint32_t fp32_sat(uint32_t x) {
	if (FP32_ISNAN(x))
		return FP32_NAN;
	if (FP32_SIGN(x))
		return 0;
	else if (x >= 0x3f800000)
		return 0x3f800000;
	else
		return x;
}

uint32_t fp32_rint(uint32_t x, enum fp_rm rm) {
	bool sx;
	int ex;
	uint32_t fx;
	if (FP32_ISNAN(x))
		return FP32_NAN;
	if (FP32_ISINF(x))
		return x;
	fp32_parsefin(x, &sx, &ex, &fx, true);
	/* For ex larger than that, result is already an integer,
	   and our shift would overflow.  */
	if (ex < FP32_MIDE + 23) {
		int shift = (FP32_MIDE + 23) - ex;
		fx = shr32(fx, shift, fp_adjust_rm(rm, sx));
		if (fx == 0) {
			ex = 0;
		} else {
			ex = FP32_MIDE + 23;
			fx = norm32(fx, &ex, 23);
		}
	}
	return fp32_mkfin(sx, ex, fx, rm);
}

uint32_t fp32_minmax(uint32_t a, uint32_t b, bool min) {
	if (FP32_ISNAN(a)) {
		if (FP32_ISNAN(b))
			return FP32_NAN;
		a = b;
	} else if (FP32_ISNAN(b)) {
		b = a;
	}
	bool sa, sb;
	int ea, eb;
	uint32_t fa, fb;
	/* Flush denormals.  */
	fp32_parsefin(a, &sa, &ea, &fa, true);
	a = fp32_mkfin(sa, ea, fa, FP_RN);
	fp32_parsefin(b, &sb, &eb, &fb, true);
	b = fp32_mkfin(sb, eb, fb, FP_RN);
	bool flip = min;
	if (sa != sb) {
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
	if (FP32_ISNAN(a) || FP32_ISNAN(b))
		return FP_UN;
	bool sa, sb;
	int ea, eb;
	uint32_t fa, fb;
	fp32_parsefin(a, &sa, &ea, &fa, true);
	fp32_parsefin(b, &sb, &eb, &fb, true);
	if (fa == 0 && fb == 0) {
		/* Zeros - equal regardless of sign. */
		return FP_EQ;
	} else if (sa != sb) {
		/* Different signs, both finite or infinity. */
		return sa ? FP_LT : FP_GT;
	} else if (ea == eb && fa == fb) {
		/* Get the equality case out of the way. */
		return FP_EQ;
	} else {
		/* Same signs, both finite or infinity. */
		if ((a < b) ^ sa)
			return FP_LT;
		else
			return FP_GT;
	}
}

uint32_t fp32_add(uint32_t a, uint32_t b, enum fp_rm rm) {
	if (FP32_ISNAN(a) || FP32_ISNAN(b))
		return FP32_NAN;
	if (FP32_ISINF(a)) {
		if (FP32_ISINF(b) && a != b) {
			/* Inf-Inf */
			return FP32_NAN;
		}
		return a;
	}
	if (FP32_ISINF(b))
		return b;
	/* Two honest real numbers involved. */
	bool sa, sb, sr;
	int ea, eb, er;
	uint32_t fa, fb;
	fp32_parsefin(a, &sa, &ea, &fa, true);
	fp32_parsefin(b, &sb, &eb, &fb, true);
	er = (ea > eb ? ea : eb) + 1;
	int32_t res = 0;
	fa = shr32(fa, er - ea - 4, FP_RT);
	fb = shr32(fb, er - eb - 4, FP_RT);
	res += sa ? -fa : fa;
	res += sb ? -fb : fb;
	if (res == 0) {
		/* Got a proper 0. */
		er = 0;
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
		res = norm32(res, &er, 27);
		/* Round it. */
		res = shr32(res, 4, fp_adjust_rm(rm, sr));
	}
	return fp32_mkfin(sr, er, res, rm);
}

uint32_t fp32_mul(uint32_t a, uint32_t b, enum fp_rm rm, bool zero_wins) {
	bool sa, sb, sr;
	int ea, eb, er;
	uint32_t fa, fb;
	fp32_parsefin(a, &sa, &ea, &fa, true);
	fp32_parsefin(b, &sb, &eb, &fb, true);
	if (zero_wins && (fa == 0 || fb == 0))
		return 0;
	if (FP32_ISNAN(a) || FP32_ISNAN(b))
		return FP32_NAN;
	sr = sa ^ sb;
	if (FP32_ISINF(a) || FP32_ISINF(b)) {
		if (fa == 0 || fb == 0) {
			/* Inf*0 is NaN */
			return FP32_NAN;
		} else {
			/* Inf*finite and Inf*Inf are +-Inf */
			return FP32_INF(sr);
		}
	}
	er = ea + eb - FP32_MIDE + 1;
	uint64_t res = (uint64_t)fa * fb;
	if (res == 0) {
		er = 0;
	} else {
		/* We multiplied two 24-bit numbers starting with 1s.
		   The first 1 has to be at either 46th or 47th
		   position.  Make it 47. */
		res = norm64(res, &er, 47);
		res = shr64(res, 24, fp_adjust_rm(rm, sr));
	}
	return fp32_mkfin(sr, er, res, rm);
}

uint32_t fp32_mad(uint32_t a, uint32_t b, uint32_t c, bool zero_wins) {
	bool sa, sb, sc, ss, sr;
	int ea, eb, ec, es, er;
	uint32_t fa, fb, fc;
	fp32_parsefin(a, &sa, &ea, &fa, true);
	fp32_parsefin(b, &sb, &eb, &fb, true);
	fp32_parsefin(c, &sc, &ec, &fc, true);
	if (zero_wins && (fa == 0 || fb == 0))
		return fp32_add(c, 0, FP_RN);
	ss = sa ^ sb;
	if (FP32_ISNAN(a) || FP32_ISNAN(b) || FP32_ISNAN(c))
		return FP32_NAN;
	if (FP32_ISINF(a) || FP32_ISINF(b)) {
		if (fa == 0 || fb == 0) {
			/* 0*inf */
			return FP32_NAN;
		} else if (FP32_ISINF(c) && sc != ss) {
			/* inf*inf - inf */
			return FP32_NAN;
		} else {
			return FP32_INF(ss);
		}
	}
	if (FP32_ISINF(c))
		return FP32_INF(sc);
	if (fa == 0 || fb == 0 || fc == 0)
		return fp32_add(fp32_mul(a, b, FP_RN, zero_wins), c, FP_RN);
	es = ea + eb - FP32_MIDE + 1;
	int64_t res = (uint64_t)fa * fb;
	/* We multiplied two 24-bit numbers starting with 1s.
	   The first 1 has to be at either 46th or 47th
	   position.  Make it 47. */
	res = norm64(res, &es, 47);
	res = shr64(res, 24, fc ? FP_RZ : FP_RN);
	/* Two honest real numbers involved. */
	er = (es > ec ? es : ec) + 1;
	res = shr32(res, er - es - 4, FP_RT);
	if (ss)
		res = -res;
	fc = shr32(fc, er - ec - 4, FP_RT);
	res += sc ? -(int64_t)fc : fc;
	if (!res) {
		/* Got a proper 0. */
		er = 0;
		/* Make a -0 if both inputs were negative (ie. both -0). */
		sr = ss && sc;
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
		res = norm32(res, &er, 27);
		if (er < 1) {
			res >>= 1-er;
			er = 1;
		}
		/* Round it. */
		res = shr32(res, 4, FP_RN);
	}
	return fp32_mkfin(sr, er, res, FP_RN);
}

uint32_t fp16_to_fp32(uint16_t x) {
	bool sx = FP16_SIGN(x);
	int ex = FP16_EXP(x);
	uint32_t fx = FP16_FRACT(x);
	if (FP16_ISNAN(x))
		return FP32_NAN;
	if (FP16_ISINF(x))
		return FP32_INF(sx);
	if (!ex) {
		if (fx == 0) {
			/* +- 0 */
			return FP32_MAKE(sx, 0, 0);
		} else {
			/* Denormal - the only supported kind on G80! */
			ex = FP32_MIDE - FP16_MIDE + 14;
			fx = norm32(fx, &ex, 23);
			return fp32_mkfin(sx, ex, fx, FP_RN);
		}
	} else {
		/* Finite, normalized. */
		fx <<= 13;
		ex += FP32_MIDE - FP16_MIDE;
		return FP32_MAKE(sx, ex, fx);
	}
}

uint16_t fp32_to_fp16(uint32_t x, enum fp_rm rm, bool rint) {
	bool sx;
	int ex;
	uint32_t fx;
	fp32_parsefin(x, &sx, &ex, &fx, true);
	if (FP32_ISNAN(x))
		return FP16_NAN;
	if (FP32_ISINF(x))
		return FP16_INF(sx);
	if (ex <= 0x5d)
		/* 0, rounding be damned */
		return FP16_MAKE(sx, 0, 0);
	ex += FP16_MIDE - FP32_MIDE;
	int shift = 13;
	if (ex <= 0) {
		/* Going to be a denormal... */
		shift -= ex - 1;
		ex = 0;
	}
	fx = shr32(fx, shift, rint ? FP_RZ : fp_adjust_rm(rm, sx));
	assert(fx <= FP16_IONE * 2);
	if (ex)
		fx -= FP16_IONE;
	if (fx == FP16_IONE) {
		fx = 0;
		ex++;
	}
	if (ex >= FP16_MAXE) {
		if (rm == FP_RN || (rm == FP_RP && !sx) || (rm == FP_RM && sx)) {
			ex = FP16_MAXE;
			fx = 0;
		} else {
			ex = FP16_MAXE - 1;
			fx = FP16_IONE - 1;
		}
	}
	return FP16_MAKE(sx, ex, fx);
}

uint32_t fp32_from_u64(uint64_t x, enum fp_rm rm) {
	if (!x)
		return 0;
	int ex = FP32_MIDE + 63;
	uint32_t fx;
	/* first, shift it until MSB is lit */
	x = norm64(x, &ex, 63);
	fx = shr64(x, 40, rm);
	return fp32_mkfin(false, ex, fx, rm);
}

uint64_t fp32_to_u64(uint32_t x, enum fp_rm rm) {
	bool sx;
	int ex;
	uint32_t fx;
	fp32_parsefin(x, &sx, &ex, &fx, true);
	if (FP32_ISNAN(x) || sx)
		return 0;
	if (FP32_ISINF(x))
		return -1ull;
	ex -= FP32_MIDE + 23;
	if (ex > 40)
		return -1ull;
	return shr64(fx, -ex, rm);
}
