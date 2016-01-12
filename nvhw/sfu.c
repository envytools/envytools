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
	for (i = 0; i < 17; i++)
		if (x & 1 << i)
			res += x >> (18 - i);
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
		uint32_t sx = sfu_square(x << 1);
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

uint32_t sfu_rsqrt(uint32_t x) {
	unsigned sign = FP32_SIGN(x);
	int exp = FP32_EXP(x);
	unsigned fract = FP32_FRACT(x);
	if (!exp) {
		/* 1/sqrt(0) -> Inf, 1/sqrt(-0) -> -Inf */
		exp = FP32_MAXE;
		fract = 0;
	} else if (sign) {
		/* 1/sqrt(negative) -> NaN */
		sign = 0;
		exp = FP32_MAXE;
		fract = FP32_IONE - 1;
	} else if (exp == FP32_MAXE) {
		if (fract == 0) {
			/* 1/sqrt(Inf) -> 0 */
			exp = 0;
		} else {
			/* 1/sqrt(NaN) -> NaN */
			sign = 0;
			exp = FP32_MAXE;
			fract = FP32_IONE - 1;
		}
	} else {
		exp -= FP32_MIDE;
		fract |= (exp & 1) << 23;
		exp >>= 1;
		int idx = fract >> 17;
		exp = FP32_MIDE - exp - 1;
		if (!fract) {
			exp++;
		} else {
			uint32_t x = fract & 0x1ffff;
			uint32_t sx = sfu_square(x);
			int64_t p1 = (int64_t)sfu_rsqrt_tab[idx][0] << 14;
			int64_t p2 = (int64_t)sfu_rsqrt_tab[idx][1] * x;
			int64_t p3 = (int64_t)sfu_rsqrt_tab[idx][2] * sx << 2;
			fract = (p1 + p2 + p3 + 0x7fff) >> 16;
			assert(fract >= FP32_IONE);
			assert(fract <= 2*FP32_IONE);
			fract -= FP32_IONE;
		}
		if (exp <= 0) {
			/* denormal underflow, flush to zero */
			exp = 0;
			fract = 0;
		}
	}
	return sign << 31 | exp << 23 | fract;
}

uint32_t sfu_sincos(uint32_t x, bool cos) {
	unsigned sign = FP32_SIGN(x);
	int exp = FP32_EXP(x);
	unsigned fract = FP32_FRACT(x);
	if (exp & 0x80) {
		/* sin(NaN) -> NaN, sin(inf) -> NaN */
		sign = 0;
		exp = FP32_MAXE;
		fract = FP32_IONE - 1;
	} else {
		if (cos) {
			exp++;
			sign = 0;
		}
		if (exp & 1)
			fract = ~fract;
		sign ^= exp >> 1 & 1;
		int idx = fract >> 17 & 0x3f;
		uint32_t x = fract & 0x1ffff;
		uint32_t sx = sfu_square(x);
		int64_t p1 = (int64_t)sfu_sin_tab[idx][0] << 11;
		int64_t p2 = (int64_t)sfu_sin_tab[idx][1] * x;
		int64_t p3 = (int64_t)sfu_sin_tab[idx][2] * sx;
		int64_t res = p1 + p2 + p3;
		assert(res >= 0);
		assert(res < 1ull << 38);
		exp = 0x7f;
		if (!res) {
			exp = 0;
			fract = 0;
		} else {
			while (!(res & 1ull << 37)) {
				res <<= 1;
				exp--;
			}
			fract = res >> 14;
			assert(fract >= FP32_IONE);
			assert(fract < 2*FP32_IONE);
			fract -= FP32_IONE;
		}
	}
	return sign << 31 | exp << 23 | fract;
}

uint32_t sfu_ex2(uint32_t x, bool sat) {
	unsigned sign = FP32_SIGN(x);
	int exp = FP32_EXP(x);
	unsigned fract = FP32_FRACT(x);
	if (exp & 0x80) {
		if(exp & 1) {
			if (sign == 0) {
				if (sat) {
					/* ex2.sat(inf) -> 1.0 */
					sign = 0;
					exp = 0x7f;
					fract = 0;
				} else {
					/* ex2(inf) -> inf */
					sign = 0;
					exp = FP32_MAXE;
					fract = 0;
				}
			} else {
				/* ex2(-inf) -> 0 */
				sign = 0;
				exp = 0;
				fract = 0;
			}
		} else {
			/* ex2(NaN) -> NaN */
			sign = 0;
			exp = FP32_MAXE;
			fract = FP32_IONE - 1;
		}
	} else {
		if (sign) {
			exp = -exp - 1;
			if (fract) {
				fract = ~fract;
			} else {
				exp++;
			}
		}
		exp += 0x7f;
		sign = 0;
		int idx = fract >> 17 & 0x3f;
		uint32_t x = fract & 0x1ffff;
		uint32_t sx = sfu_square(x);
		int64_t p1 = (int64_t)sfu_ex2_tab[idx][0] << 13;
		int64_t p2 = (int64_t)sfu_ex2_tab[idx][1] * x;
		int64_t p3 = (int64_t)sfu_ex2_tab[idx][2] * sx;
		fract = (p1 + p2 + p3 + 0x77e2) >> 15;
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
		if (exp >= 0x7f && sat) {
			exp = 0x7f;
			fract = 0;
		}
	}
	return sign << 31 | exp << 23 | fract;
}

uint32_t sfu_lg2(uint32_t x) {
	unsigned sign = FP32_SIGN(x);
	int exp = FP32_EXP(x);
	unsigned fract = FP32_FRACT(x);
	if (!exp) {
		/* lg2(0) -> -Inf */
		sign = 1;
		exp = FP32_MAXE;
		fract = 0;
	} else if (sign) {
		/* lg2(negative) -> NaN */
		sign = 0;
		exp = FP32_MAXE;
		fract = FP32_IONE - 1;
	} else if (exp == FP32_MAXE) {
		if (fract == 0) {
			/* lg2(Inf) -> Inf */
		} else {
			/* lg2(NaN) -> NaN */
			fract = FP32_IONE - 1;
		}
	} else {
		int64_t res;
		if (!fract && exp == 0x7f) {
			res = 0;
		} else {
			int idx = fract >> 17;
			uint32_t x = fract & 0x1ffff;
			uint32_t sx = sfu_square(x);
			int64_t p1 = (int64_t)sfu_lg2_tab[idx][0] << 12;
			int64_t p2 = (int64_t)sfu_lg2_tab[idx][1] * x;
			int64_t p3 = (int64_t)sfu_lg2_tab[idx][2] * sx << 1;
			res = p1 + p2 + p3 + 0x3345;
			res >>= 2;
		}
		assert(res >= 0);
		assert(res < (1ll << 36));
		res += (int64_t)(exp - 0x7f) << 36;
		if (res < 0) {
			sign = 1;
			res = ~res;
		}
		if (!res) {
			fract = exp = 0;
		} else {
			exp = 0x7f + 6;
			while (!(res & 1ll << 42)) {
				res <<= 1;
				exp--;
			}
			fract = res >> 19;
			assert(fract >= FP32_IONE);
			assert(fract < 2*FP32_IONE);
			fract -= FP32_IONE;
		}
	}
	return sign << 31 | exp << 23 | fract;
}
