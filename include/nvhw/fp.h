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

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

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
enum fp_cmp fp32_cmp(uint32_t a, uint32_t b);
uint32_t fp32_minmax(uint32_t a, uint32_t b, bool min);

#if 0
/* f64 ops */
uint64_t fp64_add(uint64_t a, uint64_t b);
uint64_t fp64_mul(uint64_t a, uint64_t b);
uint64_t fp64_fma(uint64_t a, uint64_t b, uint64_t c);
enum fp_cmp fp64_cmp(uint64_t a, uint64_t b);
#endif

/* f2f */
uint32_t fp16_to_fp32(uint16_t x);
uint16_t fp32_to_fp16(uint32_t x, enum fp_rm rm, bool rint);
#if 0
uint64_t fp32_to_fp64(uint32_t x);
uint32_t fp64_to_fp32(uint64_t x);
uint64_t fp16_to_fp64(uint16_t x);
uint16_t fp64_to_fp16(uint64_t x);
#endif

/* f2i */
uint64_t fp32_to_u64(uint32_t x, enum fp_rm rm);
#if 0
uint64_t fp64_to_u64(uint64_t x);
#endif

/* i2f */
uint32_t fp32_from_u64(uint64_t x, enum fp_rm rm);
#if 0
uint64_t fp64_from_u64(uint64_t x);
#endif

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
	if (exp < 0 || exp > FP16_MAXE)
		abort();
	if (fract >= FP16_IONE)
		abort();
	return sign << 15 | exp << 10 | fract;
}

#define FP32_SIGN(x) ((x) >> 31 & 1)
#define FP32_EXP(x) ((x) >> 23 & 0xff)
#define FP32_FRACT(x) ((x) & 0x7fffff)
#define FP32_IONE 0x800000
#define FP32_MAXE 0xff
#define FP32_MIDE 0x7f
#define FP32_NAN 0x7fffffff
#define FP32_ISNAN(x) (FP32_EXP(x) == FP32_MAXE && FP32_FRACT(x) != 0)
#define FP32_ISINF(x) (FP32_EXP(x) == FP32_MAXE && FP32_FRACT(x) == 0)
#define FP32_INF(s) (FP32_MAKE(s, FP32_MAXE, 0))
static inline uint32_t FP32_MAKE(bool sign, int exp, uint32_t fract) {
	if (exp < 0 || exp > FP32_MAXE)
		abort();
	if (fract >= FP32_IONE)
		abort();
	return sign << 31 | exp << 23 | fract;
}

#define FP64_SIGN(x) ((x) >> 63 & 1)
#define FP64_EXP(x) ((x) >> 52 & 0x7ff)
#define FP64_FRACT(x) ((x) & 0xfffffffffffffLL)
#define FP64_IONE 0x10000000000000LL
#define FP64_MAXE 0x7ff
#define FP64_MIDE 0x3ff

#endif
