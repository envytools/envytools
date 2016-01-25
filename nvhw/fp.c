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

uint32_t fp32_sat(uint32_t x) {
	if (x >= 0x3f800000 && x <= 0x7f800000)
		return 0x3f800000;
	else if (x & 0x80000000)
		return 0;
	else
		return x;
}

uint32_t fp32_rint(uint32_t x, enum fp_rm rm) {
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (ex == FP32_MAXE && fx) {
		/* NaN. */
		return FP32_NAN;
	} else if (ex == FP32_MAXE) {
		/* Inf. */
		return x;
	} else if (ex == 0) {
		/* +-0. */
		return FP32_MAKE(sx, 0, 0);
	} else if (ex >= FP32_MIDE + 23) {
		/* Big exponent, already an integer. */
		return x;
	} else {
		fx += FP32_IONE;
		int shift = (FP32_MIDE + 23) - ex;
		fx = shr32(fx, shift, fp_adjust_rm(rm, sx));
		if (fx == 0) {
			ex = 0;
		} else {
			ex = FP32_MIDE + 23;
			while (fx < FP32_IONE) {
				fx <<= 1;
				ex--;
			}
			fx -= FP32_IONE;
		}
		return FP32_MAKE(sx, ex, fx);
	}
}

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
			return FP32_NAN;
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
				f += FP32_IONE;
				int shr = er - e - 3;
				f = shr32(f, shr, FP_RT);
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
			/* Round it. */
			res = shr32(res, 4, fp_adjust_rm(rm, sr));
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
	return FP32_MAKE(sr, er, fr);
}

uint32_t fp32_mul(uint32_t a, uint32_t b, enum fp_rm rm, bool zero_wins) {
	bool sa = FP32_SIGN(a);
	int ea = FP32_EXP(a);
	uint32_t fa = FP32_FRACT(a);
	bool sb = FP32_SIGN(b);
	int eb = FP32_EXP(b);
	uint32_t fb = FP32_FRACT(b);
	bool sr = sa ^ sb;
	int er;
	uint32_t fr;
	if (zero_wins && (ea == 0 || eb == 0))
		return 0;
	if (ea == FP32_MAXE && fa) {
		/* NaN*b is NaN. */
		sr = false;
		er = FP32_MAXE;
		fr = 0x7fffff;
	} else if (eb == FP32_MAXE && fb) {
		/* a*NaN is NaN. */
		sr = false;
		er = FP32_MAXE;
		fr = 0x7fffff;
	} else if (ea == FP32_MAXE || eb == FP32_MAXE) {
		if (eb == 0 || ea == 0) {
			/* Inf*0 is NaN */
			sr = false;
			er = FP32_MAXE;
			fr = 0x7fffff;
		} else {
			/* Inf*finite and Inf*Inf are +-Inf */
			er = FP32_MAXE;
			fr = 0;
		}
	} else if (ea == 0 || eb == 0) {
		/* 0*finite is 0. */
		er = 0;
		fr = 0;
	} else {
		er = ea + eb - FP32_MIDE;
		fa |= FP32_IONE;
		fb |= FP32_IONE;
		uint64_t res = (uint64_t)fa * fb;
		/* We multiplied two 24-bit numbers starting with 1s.
		   The first 1 has to be at either 46th or 47th
		   position.  Make it 47. */
		if (!(res & 1ull << 47)) {
			res <<= 1;
		} else {
			er++;
		}
		res = shr64(res, 24, fp_adjust_rm(rm, sr));
		assert(res >= FP32_IONE);
		assert(res <= 2*FP32_IONE);
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
	return FP32_MAKE(sr, er, fr);
}

uint32_t fp32_mad(uint32_t a, uint32_t b, uint32_t c, bool zero_wins) {
	bool sa = FP32_SIGN(a);
	int ea = FP32_EXP(a);
	uint32_t fa = FP32_FRACT(a);
	bool sb = FP32_SIGN(b);
	int eb = FP32_EXP(b);
	uint32_t fb = FP32_FRACT(b);
	bool sc = FP32_SIGN(c);
	int ec = FP32_EXP(c);
	uint32_t fc = FP32_FRACT(c);
	bool ss = sa ^ sb;
	if (zero_wins && (ea == 0 || eb == 0))
		return fp32_add(c, 0, FP_RN);
	if ((ea == FP32_MAXE && fa) || (eb == FP32_MAXE && fb) || (ec == FP32_MAXE && fc)) {
		/* NaN in, NaN out. */
		return FP32_NAN;
	} else if (ea == FP32_MAXE || eb == FP32_MAXE) {
		if (!ea || !eb) {
			/* 0*inf */
			return FP32_NAN;
		} else if (ec == FP32_MAXE && sc != ss) {
			/* inf*inf - inf */
			return FP32_NAN;
		} else {
			return FP32_MAKE(ss, FP32_MAXE, 0);
		}
	} else if (ec == FP32_MAXE) {
		return FP32_MAKE(sc, FP32_MAXE, 0);
	}
	if (!ea || !eb || !ec)
		return fp32_add(fp32_mul(a, b, FP_RN, zero_wins), c, FP_RN);
	int es = ea + eb - FP32_MIDE;
	fa |= FP32_IONE;
	fb |= FP32_IONE;
	int64_t res = (uint64_t)fa * fb;
	/* We multiplied two 24-bit numbers starting with 1s.
	   The first 1 has to be at either 46th or 47th
	   position.  Make it 47. */
	if (!(res & 1ull << 47)) {
		res <<= 1;
	} else {
		es++;
	}
	res = shr64(res, 24, ec ? FP_RZ : FP_RN);
	assert(res >= FP32_IONE);
	assert(res <= 2*FP32_IONE);
	/* Get rid of implicit one. */
	res -= FP32_IONE;
	/* We normalized it before, but rounding could bump it
	   to the next exponent. */
	if (res == FP32_IONE) {
		res = 0;
		es++;
	}
	uint32_t fs = res;
	/* Two honest real numbers involved. */
	int er = (es > ec ? es : ec);
	res = 0;
	int i;
	for (i = 0; i < 2; i++) {
		int e = i ? ec : es;
		uint32_t f = i ? fc : fs;
		bool s = i ? sc : ss;
		if (i || ec) {
			/* Non-0 */
			f += FP32_IONE;
			int shr = er - e - 3;
			f = shr32(f, shr, FP_RT);
			/* Stuff it into the accumulator. */
			if (s)
				res -= f;
			else
				res += f;
		}
	}
	bool sr;
	uint32_t fr;
	if (!res) {
		/* Got a proper 0. */
		er = 0;
		fr = 0;
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
		while (!(res & 1 << 27) && er > 0) {
			res <<= 1;
			er--;
		}
		assert(res >= 16*FP32_IONE || er == 0);
		assert(res < 32*FP32_IONE);
		/* Round it. */
		res = shr32(res, 4, FP_RN);
		assert(res >= FP32_IONE || er == 0);
		assert(res <= 2*FP32_IONE);
		/* Accumulator is now 2.22, inputs were 1.23 - correct. */
		er++;
		/* We normalized it before, but rounding could bump it
		   to the next exponent. */
		if (res == 2*FP32_IONE) {
			res = FP32_IONE;
			er++;
		}
		if (res < FP32_IONE) {
			/* Well, underflow. Too bad. */
			er = 0;
			fr = 0;
		} else if (er >= FP32_MAXE) {
			/* Overflow. Likewise. */
			er = FP32_MAXE;
			fr = 0;
		} else {
			/* Get rid of implicit one. */
			res -= FP32_IONE;
			fr = res;
		}
	}
	return FP32_MAKE(sr, er, fr);
}

uint32_t fp16_to_fp32(uint16_t x) {
	bool sx = FP16_SIGN(x);
	int ex = FP16_EXP(x);
	uint32_t fx = FP16_FRACT(x);
	if (ex == FP16_MAXE) {
		if (fx) {
			/* NaN. */
			return FP32_NAN;
		} else {
			/* Inf. */
			return FP32_MAKE(sx, FP32_MAXE, 0);
		}
	} else if (!ex) {
		if (fx == 0) {
			/* +- 0 */
			return FP32_MAKE(sx, 0, 0);
		} else {
			/* Denormal - the only supported kind on G80! */
			ex = FP32_MIDE - FP16_MIDE + 14;
			while (fx < FP32_IONE) {
				fx <<= 1;
				ex--;
			}
			fx -= FP32_IONE;
			return FP32_MAKE(sx, ex, fx);
		}
	} else {
		/* Finite, normalized. */
		fx <<= 13;
		ex += FP32_MIDE - FP16_MIDE;
		return FP32_MAKE(sx, ex, fx);
	}
}

uint16_t fp32_to_fp16(uint32_t x, enum fp_rm rm, bool rint) {
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (ex == FP32_MAXE) {
		if (fx) {
			/* NaN */
			return FP16_NAN;
		} else {
			/* Inf. */
			return FP16_MAKE(sx, FP16_MAXE, 0);
		}
	} else if (ex <= 0x5d) {
		/* +-0 */
		return FP16_MAKE(sx, 0, 0);
	} else {
		ex += FP16_MIDE - FP32_MIDE;
		fx += FP32_IONE;
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
}

uint32_t fp32_from_u64(uint64_t x, enum fp_rm rm) {
	if (!x)
		return 0;
	int ex = FP32_MIDE + 63;
	uint32_t fx;
	/* first, shift it until MSB is lit */
	while (!(x & (1ull << 63))) {
		x <<= 1;
		ex--;
	}
	fx = shr64(x, 40, rm);
	fx -= FP32_IONE;
	if (fx == FP32_IONE) {
		ex++;
		fx = 0;
	}
	return FP32_MAKE(false, ex, fx);
}

uint64_t fp32_to_u64(uint32_t x, enum fp_rm rm) {
	x = fp32_rint(x, rm);
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint64_t fx = FP32_FRACT(x);
	if (ex == FP32_MAXE && fx)
		return 0;
	if (sx || !ex)
		return 0;
	fx |= FP32_IONE;
	ex -= FP32_MIDE + 23;
	if (ex > 40)
		return -1ull;
	return shr64(fx, -ex, FP_RZ);
}
