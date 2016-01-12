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

#include <stdint.h>
#include <stdbool.h>

enum fp_rm {
	FP_RN, /* to nearest, ties to even */
	FP_RM, /* to -Inf */
	FP_RP, /* to +Inf */
	FP_RZ, /* to zero */
};

enum fp_cmp {
	FP_LT, /* a < b */
	FP_EQ, /* a == b */
	FP_GT, /* a > b */
	FP_UN, /* unordered */
};

#if 0
/* fp32 ops */
uint32_t fp32_add(uint32_t a, uint32_t b, enum fp_rm rm, bool ftz);
uint32_t fp32_mul(uint32_t a, uint32_t b, enum fp_rm rm, bool ftz, bool fmz, int shift);
uint32_t fp32_fma(uint32_t a, uint32_t b, uint32_t c, enum fp_rm rm, bool ftz, bool fmz);
enum fp_cmp fp32_cmp(uint32_t a, uint32_t b);

/* f64 ops */
uint64_t fp64_add(uint64_t a, uint64_t b);
uint64_t fp64_mul(uint64_t a, uint64_t b);
uint64_t fp64_fma(uint64_t a, uint64_t b, uint64_t c);
enum fp_cmp fp64_cmp(uint64_t a, uint64_t b);

/* f2f */
uint64_t fp32_to_fp64(uint32_t x);
uint64_t fp16_to_fp64(uint16_t x);
uint32_t fp64_to_fp32(uint64_t x);
uint32_t fp16_to_fp32(uint16_t x);
uint16_t fp64_to_fp16(uint64_t x);
uint16_t fp32_to_fp16(uint32_t x);

/* f2i */
uint64_t fp32_to_u64(uint32_t x);
int64_t fp32_to_s64(uint32_t x);
uint64_t fp64_to_u64(uint64_t x);
int64_t fp64_to_s64(uint64_t x);

/* i2f */
uint32_t fp32_from_u64(uint64_t x);
uint32_t fp32_from_s64(int64_t x);
uint64_t fp64_from_u64(uint64_t x);
uint64_t fp64_from_s64(int64_t x);
#endif

#define FP32_SIGN(x) ((x) >> 31 & 1)
#define FP32_EXP(x) ((x) >> 23 & 0xff)
#define FP32_FRACT(x) ((x) & 0x7fffff)
#define FP32_IONE 0x800000
#define FP32_MAXE 0xff
#define FP32_MIDE 0x7f

#define FP64_SIGN(x) ((x) >> 63 & 1)
#define FP64_EXP(x) ((x) >> 52 & 0x7ff)
#define FP64_FRACT(x) ((x) & 0xfffffffffffffLL)
#define FP64_IONE 0x10000000000000LL
#define FP64_MAXE 0x7ff
#define FP64_MIDE 0x3ff

#endif
