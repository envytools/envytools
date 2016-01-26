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

uint32_t sfu_pre(uint32_t x, enum sfu_pre_mode mode) {
	bool sx;
	int ex;
	uint32_t fx;
	fp32_parsefin(x, &sx, &ex, &fx, true);
	if (FP32_ISNAN(x)) {
		fx = 0x40000000;
	} else if (FP32_ISINF(x)) {
		fx = 0x40800000;
	} else {
		ex -= FP32_MIDE;
		if (mode == SFU_PRE_SIN) {
			fx = (uint64_t)fx * 0xa2f983 >> 16;
			ex -= 8;
		}
		/* shift fract exp bits to the left */
		if (ex >= 7 && mode == SFU_PRE_EX2) {
			/* Overflow, return Inf. */
			fx = 0x40800000;
		} else {
			if (mode == SFU_PRE_SIN && ex >= 0) {
				/* Mask high bits off (shr32 would complain about those) */
				if (ex >= 32)
					fx = 0;
				else
					fx = fx << ex;
			} else {
				fx = shr32(fx, -ex, FP_RZ);
			}
			if (mode == SFU_PRE_SIN)
				fx &= 0x1ffffff;
		}
	}
	/* fx is now a 7.23 fractional number, or one of the specials. */
	return fx | sx << 31;
}

static uint32_t sfu_square(uint32_t x) {
	int i;
	uint32_t res = 0;
	for (i = 0; i < 17; i++)
		if (x & 1 << i)
			res += x >> (18 - i);
	return res >> 1;
}

uint32_t sfu_rcp(uint32_t x) {
	bool sx;
	int ex;
	uint32_t fx;
	fp32_parsefin(x, &sx, &ex, &fx, true);
	if (FP32_ISNAN(x)) {
		return FP32_CNAN;
	} else if (FP32_ISINF(x)) {
		/* 1/Inf -> 0 */
		ex = 1;
		fx = 0;
	} else if (fx == 0) {
		/* 1/0 -> Inf */
		return FP32_INF(sx);
	} else {
		ex = 2 * FP32_MIDE - ex - 1;
		int idx = fx >> 16 & 0x7f;
		uint32_t x = fx & 0xffff;
		uint32_t x2 = sfu_square(x << 1);
		int64_t p1 = (int64_t)sfu_rcp_tab[idx][0] << 13;
		int64_t p2 = (int64_t)sfu_rcp_tab[idx][1] * x;
		int64_t p3 = (int64_t)sfu_rcp_tab[idx][2] * x2;
		fx = (p1 + p2 + p3 + 0x47e7) >> 15;
	}
	return fp32_mkfin(sx, ex, fx, FP_RN, true);
}

uint32_t sfu_rsqrt(uint32_t x) {
	bool sx;
	int ex;
	uint32_t fx;
	fp32_parsefin(x, &sx, &ex, &fx, true);
	if (FP32_ISNAN(x)) {
		return FP32_CNAN;
	} else if (fx == 0) {
		/* 1/sqrt(0) -> Inf, 1/sqrt(-0) -> -Inf */
		return FP32_INF(sx);
	} else if (sx) {
		/* 1/sqrt(negative) -> NaN */
		return FP32_CNAN;
	} else if (FP32_ISINF(x)) {
		/* 1/sqrt(Inf) -> 0 */
		ex = 1;
		fx = 0;
	} else {
		ex -= FP32_MIDE;
		fx -= FP32_IONE;
		fx |= (ex & 1) << 23;
		ex >>= 1;
		int idx = fx >> 17;
		ex = FP32_MIDE - ex - 1;
		if (fx == 0) {
			ex++;
			fx = FP32_IONE;
		} else {
			uint32_t x = fx & 0x1ffff;
			uint32_t x2 = sfu_square(x);
			int64_t p1 = (int64_t)sfu_rsqrt_tab[idx][0] << 14;
			int64_t p2 = (int64_t)sfu_rsqrt_tab[idx][1] * x;
			int64_t p3 = (int64_t)sfu_rsqrt_tab[idx][2] * x2 << 2;
			fx = (p1 + p2 + p3 + 0x7fff) >> 16;
		}
	}
	return fp32_mkfin(sx, ex, fx, FP_RN, true);
}

uint32_t sfu_sincos(uint32_t x, bool cos) {
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (ex & 0x80) {
		/* sin(NaN) -> NaN, sin(inf) -> NaN */
		return FP32_CNAN;
	} else {
		if (cos) {
			ex++;
			sx = 0;
		}
		if (ex & 1)
			fx = ~fx;
		sx ^= ex >> 1 & 1;
		int idx = fx >> 17 & 0x3f;
		uint32_t x = fx & 0x1ffff;
		uint32_t x2 = sfu_square(x);
		int64_t p1 = (int64_t)sfu_sin_tab[idx][0] << 11;
		int64_t p2 = (int64_t)sfu_sin_tab[idx][1] * x;
		int64_t p3 = (int64_t)sfu_sin_tab[idx][2] * x2;
		int64_t res = p1 + p2 + p3;
		assert(res >= 0);
		assert(res < 1ull << 38);
		ex = FP32_MIDE;
		res = norm64(res, &ex, 37);
		fx = res >> 14;
		return fp32_mkfin(sx, ex, fx, FP_RZ, true);
	}
}

uint32_t sfu_ex2(uint32_t x) {
	unsigned sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	unsigned fx = FP32_FRACT(x);
	if (ex & 0x80) {
		if(ex & 1) {
			if (!sx) {
				/* ex2(inf) -> inf */
				return FP32_INF(false);
			} else {
				/* ex2(-inf) -> 0 */
				return 0;
			}
		} else {
			/* ex2(NaN) -> NaN */
			return FP32_CNAN;
		}
	} else {
		if (sx) {
			ex = -ex - 1;
			if (fx)
				fx = ~fx;
			else
				ex++;
		}
		ex += 0x7f;
		sx = false;
		int idx = fx >> 17 & 0x3f;
		uint32_t x = fx & 0x1ffff;
		uint32_t x2 = sfu_square(x);
		int64_t p1 = (int64_t)sfu_ex2_tab[idx][0] << 13;
		int64_t p2 = (int64_t)sfu_ex2_tab[idx][1] * x;
		int64_t p3 = (int64_t)sfu_ex2_tab[idx][2] * x2;
		fx = (p1 + p2 + p3 + 0x77e2) >> 15;
		return fp32_mkfin(sx, ex, fx, FP_RN, true);
	}
}

uint32_t sfu_lg2(uint32_t x) {
	bool sx;
	int ex;
	uint32_t fx;
	fp32_parsefin(x, &sx, &ex, &fx, true);
	if (FP32_ISNAN(x))
		return FP32_CNAN;
	if (fx == 0)
		/* lg2(0) -> -Inf */
		return FP32_INF(true);
	if (sx)
		/* lg2(negative) -> NaN */
		return FP32_CNAN;
	if (FP32_ISINF(x))
		/* lg2(Inf) -> Inf */
		return x;
	int64_t res;
	if (fx == FP32_IONE && ex == FP32_MIDE) {
		/* Yup, a special case.  */
		res = 0;
	} else {
		int idx = fx >> 17 & 0x3f;
		uint32_t x = fx & 0x1ffff;
		uint32_t x2 = sfu_square(x);
		int64_t p1 = (int64_t)sfu_lg2_tab[idx][0] << 12;
		int64_t p2 = (int64_t)sfu_lg2_tab[idx][1] * x;
		int64_t p3 = (int64_t)sfu_lg2_tab[idx][2] * x2 << 1;
		res = p1 + p2 + p3 + 0x3345;
		res >>= 2;
	}
	assert(res >= 0);
	assert(res < (1ll << 36));
	res += (int64_t)(ex - 0x7f) << 36;
	if (res < 0) {
		sx = 1;
		res = ~res;
	}
	ex = FP32_MIDE + 6;
	res = norm64(res, &ex, 42);
	fx = res >> 19;
	return fp32_mkfin(sx, ex, fx, FP_RN, true);
}
