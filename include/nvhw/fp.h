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

#ifndef NVHW_FP_H
#define NVHW_FP_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

/* Helpful enums.  */

enum fp_rm {
	/* Values for first 4 same as in G80 ISA. */
	FP_RN = 0, /* to nearest, ties to even */
	FP_RM = 1, /* to -Inf */
	FP_RP = 2, /* to +Inf */
	FP_RZ = 3, /* to zero */
	FP_RT = 4, /* round for double rounding (round to odd) */
};

enum fp_cmp {
	/* Values also happen to be G80 CC codes. */
	FP_GT = 0, /* a > b */
	FP_EQ = 1, /* a == b */
	FP_LT = 2, /* a < b */
	FP_UN = 3, /* unordered */
};

static inline enum fp_rm fp_adjust_rm(enum fp_rm rm, bool sign) {
	if (rm == FP_RM && sign) return FP_RP;
	if (rm == FP_RP && sign) return FP_RM;
	return rm;
}

/* Helpful helpers.  */

/* Shifts x right by shift bits, rounding according to given rounding mode.
   If shift is negative, shifts left instead.  Handles very large shifts
   correctly.  */

static inline uint32_t shr32(uint32_t x, int shift, enum fp_rm rm) {
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
		assert(x == 0);
		return 0;
	} else {
		assert(x << -shift >> -shift == x);
		return x << -shift;
	}
}

/* Likewise, for 64-bit numbers.  */

static inline uint64_t shr64(uint64_t x, int shift, enum fp_rm rm) {
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
		assert(x == 0);
		return 0;
	} else {
		assert(x << -shift >> -shift == x);
		return x << -shift;
	}
}

/* Shifts a number left until given bit is the leftmost set bit.  If number
   has higher bits set, causes an error.  If number is 0, returns 0.
   Decreases *e by number of shifts done.  */

static inline uint32_t norm32(uint32_t x, int *e, int bit) {
	assert((x & ~((2 << bit) - 1)) == 0);
	if (!x)
		return 0;
	while (!(x & 1 << bit)) {
		x <<= 1;
		(*e)--;
	}
	return x;
}

static inline uint64_t norm64(uint64_t x, int *e, int bit) {
	assert((x & ~((2ull << bit) - 1)) == 0);
	if (!x)
		return 0;
	while (!(x & 1ull << bit)) {
		x <<= 1;
		(*e)--;
	}
	return x;
}

/* fp16 helpers */

#define FP16_SIGN(x) ((x) >> 15 & 1)
#define FP16_EXP(x) ((x) >> 10 & 0x1f)
#define FP16_FRACT(x) ((x) & 0x3ff)
#define FP16_IONE 0x400
#define FP16_MAXE 0x1f
#define FP16_MIDE 0x0f
#define FP16_NAN 0x7fff
#define FP16_ISNAN(x) (FP16_EXP(x) == FP16_MAXE && FP16_FRACT(x) != 0)
#define FP16_ISINF(x) (FP16_EXP(x) == FP16_MAXE && FP16_FRACT(x) == 0)
#define FP16_INF(s) (FP16_MAKE(s, FP16_MAXE, 0))

static inline uint16_t FP16_MAKE(bool sign, int exp, uint32_t fract) {
	assert(exp >= 0 && exp <= FP16_MAXE);
	assert(fract < FP16_IONE);
	return sign << 15 | exp << 10 | fract;
}

/* Assembles a fp16 finite number.  Subtracts implicit one from f, bumps
   exponent if rounding caused f to be 2*FP16_IONE.  f must be normalized,
   unless e == 1 (which means a denormal).  */

static inline uint16_t fp16_mkfin(bool s, int e, uint16_t f, enum fp_rm rm) {
	if (!f)
		e = 1;
	assert(e >= 1);
	assert(e == 1 || f >= FP16_IONE);
	assert(f <= 2*FP16_IONE);
	if (f == 2*FP16_IONE) {
		f >>= 1;
		e++;
	}
	if (f < FP16_IONE) {
		e = 0;
	} else if (e >= FP16_MAXE) {
		if (rm == FP_RZ || fp_adjust_rm(rm, s) == FP_RM) {
			e = FP16_MAXE-1;
			f = FP16_IONE-1;
		} else {
			e = FP16_MAXE;
			f = 0;
		}
	} else {
		f -= FP16_IONE;
	}
	return FP16_MAKE(s, e, f);
}

static inline void fp16_parsefin(uint16_t x, bool *ps, int *pe, uint16_t *pf, bool ftz) {
	bool sx = FP16_SIGN(x);
	int ex = FP16_EXP(x);
	uint16_t fx = FP16_FRACT(x);
	*ps = sx;
	if (!ex) {
		*pe = 1;
		*pf = ftz ? 0 : fx;
	} else {
		*pe = ex;
		*pf = FP16_IONE + fx;
	}
}

/* fp32 helpers */

#define FP32_SIGN(x) ((x) >> 31 & 1)
#define FP32_EXP(x) ((x) >> 23 & 0xff)
#define FP32_FRACT(x) ((x) & 0x7fffff)
#define FP32_IONE 0x800000
#define FP32_MAXE 0xff
#define FP32_MIDE 0x7f
#define FP32_CNAN 0x7fffffff
#define FP32_ISNAN(x) (FP32_EXP(x) == FP32_MAXE && FP32_FRACT(x) != 0)
#define FP32_ISINF(x) (FP32_EXP(x) == FP32_MAXE && FP32_FRACT(x) == 0)
#define FP32_INF(s) (FP32_MAKE(s, FP32_MAXE, 0))

static inline uint32_t FP32_MAKE(bool sign, int exp, uint32_t fract) {
	assert(exp >= 0 && exp <= FP32_MAXE);
	assert(fract < FP32_IONE);
	return sign << 31 | exp << 23 | fract;
}

static inline uint32_t FP32_NAN(bool sign, uint32_t payload) {
	assert(payload);
	assert(payload < FP32_IONE);
	return FP32_MAKE(sign, FP32_MAXE, payload);
}

/* Assembles a fp32 finite number.  Subtracts implicit one from f, bumps
   exponent if rounding caused f to be 2*FP32_IONE.  f must be normalized,
   unless e == 1 (which means a denormal), or e < 0 (which gives 0).  */

static inline uint32_t fp32_mkfin(bool s, int e, uint32_t f, enum fp_rm rm, bool ftz) {
	if (!f)
		e = 1;
	assert(e <= 1 || f >= FP32_IONE);
	assert(f <= 2*FP32_IONE);
	if (f == 2*FP32_IONE) {
		f >>= 1;
		e++;
	}
	if (e <= 0 || f < FP32_IONE) {
		if (ftz)
			f = 0;
		else
			assert (e == 1 || f == 0);
		e = 0;
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

static inline void fp32_parsefin(uint32_t x, bool *ps, int *pe, uint32_t *pf, bool ftz) {
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

/* fp64 helpers */

#define FP64_SIGN(x) ((x) >> 63 & 1)
#define FP64_EXP(x) ((x) >> 52 & 0x7ff)
#define FP64_FRACT(x) ((x) & 0xfffffffffffffLL)
#define FP64_IONE 0x10000000000000LL
#define FP64_MAXE 0x7ff
#define FP64_MIDE 0x3ff
#define FP64_ISNAN(x) (FP64_EXP(x) == FP64_MAXE && FP64_FRACT(x) != 0)
#define FP64_ISINF(x) (FP64_EXP(x) == FP64_MAXE && FP64_FRACT(x) == 0)
#define FP64_INF(s) (FP64_MAKE(s, FP64_MAXE, 0))

static inline uint64_t FP64_MAKE(bool sign, int exp, uint64_t fract) {
	assert(exp >= 0 && exp <= FP64_MAXE);
	assert(fract < FP64_IONE);
	return (uint64_t)sign << 63 | (uint64_t)exp << 52 | fract;
}

static inline uint64_t FP64_NAN(bool sign, uint64_t payload) {
	assert(payload);
	assert(payload < FP64_IONE);
	return FP64_MAKE(sign, FP64_MAXE, payload);
}

/* Assembles a fp64 finite number.  Subtracts implicit one from f, bumps
   exponent if rounding caused f to be 2*FP64_IONE.  f must be normalized,
   unless e == 1 (which means a denormal), or e < 0 (which gives 0).  */

static inline uint64_t fp64_mkfin(bool s, int e, uint64_t f, enum fp_rm rm) {
	if (!f)
		e = 1;
	assert(e == 1 || f >= FP64_IONE);
	assert(f <= 2*FP64_IONE);
	if (f == 2*FP64_IONE) {
		f >>= 1;
		e++;
	}
	if (f < FP64_IONE) {
		assert (e == 1);
		e = 0;
	} else if (e >= FP64_MAXE) {
		if (rm == FP_RZ || fp_adjust_rm(rm, s) == FP_RM) {
			e = FP64_MAXE-1;
			f = FP64_IONE-1;
		} else {
			e = FP64_MAXE;
			f = 0;
		}
	} else {
		f -= FP64_IONE;
	}
	return FP64_MAKE(s, e, f);
}

static inline void fp64_parsefin(uint64_t x, bool *ps, int *pe, uint64_t *pf) {
	bool sx = FP64_SIGN(x);
	int ex = FP64_EXP(x);
	uint64_t fx = FP64_FRACT(x);
	*ps = sx;
	if (!ex) {
		*pe = 1;
		*pf = fx;
	} else {
		*pe = ex;
		*pf = FP64_IONE + fx;
	}
}

/* fp32 ops */
uint32_t fp32_add(uint32_t a, uint32_t b, enum fp_rm rm);
uint32_t fp32_mul(uint32_t a, uint32_t b, enum fp_rm rm, bool zero_wins);
uint32_t fp32_mad(uint32_t a, uint32_t b, uint32_t c, bool zero_wins);
#if 0
uint32_t fp32_add(uint32_t a, uint32_t b, enum fp_rm rm, bool ftz);
uint32_t fp32_mul(uint32_t a, uint32_t b, enum fp_rm rm, bool ftz, bool fmz, int shift);
uint32_t fp32_fma(uint32_t a, uint32_t b, uint32_t c, enum fp_rm rm, bool ftz, bool fmz);
#endif
uint32_t fp32_sat(uint32_t x);
uint32_t fp32_rint(uint32_t x, enum fp_rm rm);
enum fp_cmp fp32_cmp(uint32_t a, uint32_t b, bool ftz);
uint32_t fp32_minmax(uint32_t a, uint32_t b, bool min);

/* f64 ops */
uint64_t fp64_add(uint64_t a, uint64_t b, enum fp_rm rm);
#if 0
uint64_t fp64_mul(uint64_t a, uint64_t b);
uint64_t fp64_fma(uint64_t a, uint64_t b, uint64_t c);
#endif
uint64_t fp64_rint(uint64_t x, enum fp_rm rm);
enum fp_cmp fp64_cmp(uint64_t a, uint64_t b);
uint64_t fp64_minmax(uint64_t a, uint64_t b, bool min);

/* f2f */
uint32_t fp16_to_fp32(uint16_t x);
uint16_t fp32_to_fp16(uint32_t x, enum fp_rm rm, bool rint);
uint64_t fp32_to_fp64(uint32_t x);
uint32_t fp64_to_fp32(uint64_t x, enum fp_rm rm, bool rint);

/* f2i */
uint64_t fp32_to_u64(uint32_t x, enum fp_rm rm, bool ftz);
uint64_t fp64_to_u64(uint64_t x, enum fp_rm rm);

/* i2f */
uint32_t fp32_from_u64(uint64_t x, enum fp_rm rm);
uint64_t fp64_from_u64(uint64_t x, enum fp_rm rm);

#endif
