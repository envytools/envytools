/*
 * Copyright (C) 2018 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "util.h"
#include "nvhw/xf.h"
#include "nvhw/fp.h"

uint32_t xf_s2lt(uint32_t x) {
	if (extr(x, 10, 8) != 0xff)
		x += 0x200;
	return x >> 10;
}

void xf_v2lt(uint32_t dst[3], const uint32_t src[3]) {
	for (int i = 0; i < 3; i++)
		dst[i] = xf_s2lt(src[i]);
}

uint32_t xf_sum(const uint32_t *v, int n) {
	assert(n <= 4);
	for (int i = 0; i < n; i++) {
		if (FP32_ISNAN(v[i]))
			return FP32_CNAN;
	}
	bool infn = false, infp = false;
	for (int i = 0; i < n; i++) {
		if (FP32_ISINF(v[i])) {
			if (FP32_SIGN(v[i]))
				infn = true;
			else
				infp = true;
		}
	}
	if (infn && infp)
		return FP32_CNAN;
	if (infn || infp)
		return FP32_INF(infn);
	for (int i = 0; i < n; i++) {
		if (FP32_ISINF(v[i]))
			return FP32_INF(1);
	}
	/* Only honest real numbers involved. */
	bool sv[4], sr;
	int ev[4], er = 0;
	uint32_t fv[4];
	for (int i = 0; i < n; i++) {
		sv[i] = FP32_SIGN(v[i]);
		ev[i] = FP32_EXP(v[i]);
		fv[i] = FP32_FRACT(v[i]);
		if (ev[i])
			fv[i] |= FP32_IONE;
		if (ev[i] + 2 > er)
			er = ev[i] + 2;
	}
	int32_t res = 0;
	for (int i = 0; i < n; i++) {
		fv[i] = shr32(fv[i], er - ev[i] - 7, FP_RZ);
		res += sv[i] ? -fv[i] : fv[i];
	}
	if (res == 0) {
		/* Got a proper 0. */
		er = 0;
		sr = 0;
	} else {
		/* Compute sign, make sure accumulator is positive. */
		sr = res < 0;
		if (sr)
			res = -res;
		res = norm32(res, &er, 30);
		/* Round it. */
		res = shr32(res, 7, FP_RZ);
	}
	uint32_t r = fp32_mkfin(sr, er, res, FP_RZ | FP_FTZ);
	return r;
}

bool xf_islt(uint32_t a, uint32_t b) {
	if (a & 0x80000000)
		a ^= 0x7fffffff;
	if (b & 0x80000000)
		b ^= 0x7fffffff;
	a ^= 0x80000000;
	b ^= 0x80000000;
	return a < b;
}

uint32_t xf_min(uint32_t a, uint32_t b) {
	return xf_islt(a, b) ? a : b;
}

uint32_t xf_max(uint32_t a, uint32_t b) {
	return xf_islt(a, b) ? b : a;
}

const uint8_t xf_rcp_lut_v1[0x40] = {
	0x7f, 0x7d, 0x7b, 0x79, 0x77, 0x75, 0x74, 0x72, 0x70, 0x6f, 0x6d, 0x6c, 0x6b, 0x69, 0x68, 0x67,
	0x65, 0x64, 0x63, 0x62, 0x60, 0x5f, 0x5e, 0x5d, 0x5c, 0x5b, 0x5a, 0x59, 0x58, 0x57, 0x56, 0x55,
	0x54, 0x54, 0x53, 0x52, 0x51, 0x50, 0x4f, 0x4f, 0x4e, 0x4d, 0x4c, 0x4c, 0x4b, 0x4a, 0x4a, 0x49,
	0x48, 0x48, 0x47, 0x46, 0x46, 0x45, 0x45, 0x44, 0x43, 0x43, 0x42, 0x42, 0x41, 0x41, 0x40, 0x40,
};

const uint8_t xf_rcp_lut_v2[0x40] = {
	0x7f, 0x7d, 0x7b, 0x79, 0x78, 0x76, 0x74, 0x73, 0x71, 0x6f, 0x6e, 0x6d, 0x6b, 0x6a, 0x68, 0x67,
	0x66, 0x65, 0x63, 0x62, 0x61, 0x60, 0x5f, 0x5e, 0x5d, 0x5c, 0x5b, 0x5a, 0x59, 0x58, 0x57, 0x56,
	0x55, 0x54, 0x53, 0x52, 0x52, 0x51, 0x50, 0x4f, 0x4e, 0x4e, 0x4d, 0x4c, 0x4c, 0x4b, 0x4a, 0x49,
	0x49, 0x48, 0x48, 0x47, 0x46, 0x46, 0x45, 0x45, 0x44, 0x43, 0x43, 0x42, 0x42, 0x41, 0x41, 0x40,
};

uint32_t xf_rcp(uint32_t x, bool rcc, bool v2) {
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (FP32_ISNAN(x))
		return FP32_CNAN;
	if (!ex && !rcc)
		return FP32_INF(sx);
	int er = 0xfe - ex;
	uint32_t fr = 0;
	if (fx) {
		const uint8_t *lut = v2 ? xf_rcp_lut_v2 : xf_rcp_lut_v1;
		if (fx >= 0x800000)
			abort();
		fx += 0x800000;
		uint64_t s0 = lut[fx >> 17 & 0x3f];
		uint64_t s1 = ((1u << 31) - s0 * fx) * s0 >> 24;
		uint64_t s2 = ((1ull << 37) - s1 * fx) * s1 >> 25;
		fr = s2 - 0x800000;
		if (fr >= 0x800000)
			abort();
		er--;
	}
	if (rcc) {
		if (er < 0x3f) {
			er = 0x3f;
			if (v2)
				fr = 0;
		}
		if (er >= 0xbf) {
			er = 0xbf;
			if (v2)
				fr = 0;
		}
	} else if (er <= 0) {
		er = 0;
		fr = 0;
	}
	return sx << 31 | er << 23 | fr;
}

const uint8_t xf_rsq_lut_v1[0x80] = {
	0x7f, 0x7e, 0x7d, 0x7c, 0x7b, 0x7a, 0x79, 0x79, 0x78, 0x77, 0x76, 0x75, 0x75, 0x74, 0x73, 0x72,
	0x72, 0x71, 0x70, 0x70, 0x6f, 0x6e, 0x6e, 0x6d, 0x6c, 0x6c, 0x6b, 0x6b, 0x6a, 0x69, 0x69, 0x68,
	0x68, 0x67, 0x67, 0x66, 0x66, 0x65, 0x65, 0x64, 0x64, 0x63, 0x63, 0x62, 0x62, 0x61, 0x61, 0x60,
	0x60, 0x60, 0x5f, 0x5f, 0x5e, 0x5e, 0x5e, 0x5d, 0x5d, 0x5c, 0x5c, 0x5c, 0x5b, 0x5b, 0x5b, 0x5a,
	0x5a, 0x59, 0x58, 0x58, 0x57, 0x56, 0x56, 0x55, 0x55, 0x54, 0x53, 0x53, 0x52, 0x52, 0x51, 0x51,
	0x50, 0x50, 0x4f, 0x4f, 0x4e, 0x4e, 0x4d, 0x4d, 0x4c, 0x4c, 0x4c, 0x4b, 0x4b, 0x4a, 0x4a, 0x4a,
	0x49, 0x49, 0x48, 0x48, 0x48, 0x47, 0x47, 0x47, 0x46, 0x46, 0x46, 0x45, 0x45, 0x45, 0x44, 0x44,
	0x44, 0x43, 0x43, 0x43, 0x43, 0x42, 0x42, 0x42, 0x41, 0x41, 0x41, 0x41, 0x40, 0x40, 0x40, 0x40
};

const uint8_t xf_rsq_lut_v2[0x80] = {
	0x7f, 0x7f, 0x7e, 0x7d, 0x7c, 0x7b, 0x7a, 0x79, 0x78, 0x77, 0x77, 0x76, 0x75, 0x74, 0x74, 0x73,
	0x72, 0x71, 0x71, 0x70, 0x6f, 0x6f, 0x6e, 0x6d, 0x6d, 0x6c, 0x6c, 0x6b, 0x6a, 0x6a, 0x69, 0x69,
	0x68, 0x68, 0x67, 0x67, 0x66, 0x66, 0x65, 0x65, 0x64, 0x64, 0x63, 0x63, 0x62, 0x62, 0x61, 0x61,
	0x61, 0x60, 0x60, 0x5f, 0x5f, 0x5e, 0x5e, 0x5e, 0x5d, 0x5d, 0x5d, 0x5c, 0x5c, 0x5b, 0x5b, 0x5b,
	0x5a, 0x59, 0x59, 0x58, 0x57, 0x57, 0x56, 0x56, 0x55, 0x54, 0x54, 0x53, 0x53, 0x52, 0x52, 0x51,
	0x51, 0x50, 0x50, 0x4f, 0x4f, 0x4e, 0x4e, 0x4d, 0x4d, 0x4d, 0x4c, 0x4c, 0x4b, 0x4b, 0x4a, 0x4a,
	0x4a, 0x49, 0x49, 0x49, 0x48, 0x48, 0x48, 0x47, 0x47, 0x46, 0x46, 0x46, 0x46, 0x45, 0x45, 0x45,
	0x44, 0x44, 0x44, 0x43, 0x43, 0x43, 0x43, 0x42, 0x42, 0x42, 0x41, 0x41, 0x41, 0x41, 0x40, 0x40,
};

uint32_t xf_rsq(uint32_t x, bool v2) {
	if (FP32_ISNAN(x))
		return FP32_CNAN;
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (!ex)
		return FP32_INF(0);
	if (FP32_ISINF(x))
		return 0;
	int er;
	uint32_t fr;
	if (ex & 1) {
		er = 0x7f - (ex - 0x7f) / 2 - 1;
	} else {
		er = 0x7f - (ex - 0x80) / 2 - 1;
		fx |= 0x800000;
	}
	const uint8_t *lut = v2 ? xf_rsq_lut_v2 : xf_rsq_lut_v1;
	uint64_t s0 = lut[fx >> 17 & 0x7f];
	if (fx >= 0x800000)
		fx *= 2;
	else
		fx += 0x800000;
	uint64_t s1 = ((3ull << 37) - s0 * s0 * fx) * s0 >> 32;
	uint64_t s2 = ((3ull << 49) - s1 * s1 * fx) * s1 >> 39;
	fr = s2 - 0x800000;
	if (fr >= 0x800000)
		abort();
	return er << 23 | fr;
}

int32_t xf_pre_exp(uint32_t x) {
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (FP32_ISNAN(x))
		return INT32_MAX;
	if (ex >= FP32_MIDE + 18)
		return sx ? INT32_MIN : INT32_MAX;
	if (ex < FP32_MIDE - 23)
		return 0;
	fx |= FP32_IONE;
	fx = shr32(fx, FP32_MIDE + 10 - ex, sx ? FP_RP : FP_RZ);
	if (sx)
		return -(int32_t)fx;
	else
		return fx;
}

uint32_t xf_exp_frc(uint32_t x) {
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (FP32_ISNAN(x))
		return x;
	if (!ex)
		return 0;
	if (ex >= FP32_MIDE + 23)
		return 0;
	fx |= FP32_IONE;
	if (!sx) {
		if (ex < FP32_MIDE)
			return x;
	} else {
		if (ex < FP32_MIDE - 23)
			return FP32_ONE;
		if (ex < FP32_MIDE) {
			fx = shr32(fx, FP32_MIDE - ex, FP_RP);
			ex = FP32_MIDE;
		}
		fx = -fx;
	}
	fx &= ((1 << (FP32_MIDE + 23 - ex)) - 1);
	if (!fx)
		return 0;
	fx = norm32(fx, &ex, 23);
	fx -= FP32_IONE;
	return ex << 23 | fx;
}

const uint16_t xf_exp_lut_a[0x20] = {
	0x0000,
	0x0167,
	0x02d5,
	0x044c,
	0x05cb,
	0x0752,
	0x08e2,
	0x0a7b,
	0x0c1c,
	0x0dc7,
	0x0f7b,
	0x1138,
	0x12ff,
	0x14d1,
	0x16ac,
	0x1892,
	0x1a83,
	0x1c7e,
	0x1e84,
	0x2096,
	0x22b4,
	0x24dd,
	0x2712,
	0x2954,
	0x2ba3,
	0x2dfe,
	0x3067,
	0x32dc,
	0x3561,
	0x37f2,
	0x3a93,
	0x3d44,
};

const uint16_t xf_exp_lut_b[0x20] = {
	0x167,
	0x16e,
	0x177,
	0x17f,
	0x187,
	0x190,
	0x199,
	0x1a1,
	0x1ab,
	0x1b4,
	0x1bd,
	0x1c7,
	0x1d2,
	0x1db,
	0x1e6,
	0x1f1,
	0x1fb,
	0x206,
	0x212,
	0x21e,
	0x229,
	0x235,
	0x242,
	0x24f,
	0x25b,
	0x269,
	0x275,
	0x285,
	0x291,
	0x2a1,
	0x2b1,
	0x2bb,
};

uint32_t xf_exp_core(int32_t x) {
	int32_t flr = x >> 13;
	if (flr <= -FP32_MIDE)
		return 0;
	if (flr >= FP32_MAXE - FP32_MIDE)
		return FP32_INF(0);
	uint32_t er = flr + FP32_MIDE;
	uint32_t h = x >> 8 & 0x1f;
	uint32_t l = x & 0xff;
	uint32_t fr = xf_exp_lut_a[h] * 0x100 + xf_exp_lut_b[h] * l;
	return er << 23 | fr << 1;
}

uint32_t xf_exp_flr(uint32_t x) {
	return xf_exp_core(xf_pre_exp(x) & ~0x1fff);
}

uint32_t xf_exp(uint32_t x) {
	if (FP32_ISNAN(x))
		return FP32_CNAN;
	return xf_exp_core(xf_pre_exp(x));
}

uint32_t xf_log_e(uint32_t x) {
	int ex = FP32_EXP(x);
	if (FP32_ISNAN(x))
		return x & 0x7fffffff;
	if (!ex)
		return FP32_INF(1);
	if (ex == FP32_MAXE)
		return FP32_INF(0);
	ex -= FP32_MIDE;
	bool sr = false;
	if (ex < 0) {
		sr = true;
		ex = -ex;
	}
	if (!ex)
		return 0;
	int er = FP32_MIDE + 23;
	uint32_t fr = norm32(ex, &er, 23);
	fr -= FP32_IONE;
	return sr << 31 | er << 23 | fr;
}

uint32_t xf_log_f(uint32_t x) {
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (FP32_ISNAN(x))
		return x & 0x7fffffff;
	if (!ex)
		return FP32_ONE;
	return FP32_MIDE << 23 | fx;
}

static uint32_t xf_log_lut_a[0x20] = {
	0x00000,
	0x02db6,
	0x059bd,
	0x08493,
	0x0ae2d,
	0x0d6a6,
	0x0fe20,
	0x12468,
	0x149ce,
	0x16e47,
	0x191df,
	0x1b4a1,
	0x1d696,
	0x1f7c9,
	0x2183f,
	0x23804,
	0x2571e,
	0x27592,
	0x2936b,
	0x2b0ab,
	0x2cd5a,
	0x2e97c,
	0x3051a,
	0x32035,
	0x33ad3,
	0x354f7,
	0x36ea9,
	0x387e9,
	0x3a0bd,
	0x3b928,
	0x3d12b,
	0x3e8d5,
};

static uint16_t xf_log_lut_b[0x20] = {
	0x2db6,
	0x2c07,
	0x2ad6,
	0x299a,
	0x2879,
	0x277a,
	0x2648,
	0x2566,
	0x2479,
	0x2398,
	0x22c2,
	0x21f5,
	0x2133,
	0x2076,
	0x1fc5,
	0x1f1a,
	0x1e74,
	0x1dd9,
	0x1d40,
	0x1caf,
	0x1c22,
	0x1b9e,
	0x1b1b,
	0x1a9e,
	0x1a24,
	0x19b2,
	0x1940,
	0x18d4,
	0x186b,
	0x1803,
	0x17aa,
	0x172a,
};

int64_t xf_log_core(uint32_t x) {
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	uint32_t l = fx >> 10 & 0xff;
	uint32_t h = fx >> 18 & 0x1f;
	int64_t fr = xf_log_lut_a[h] * 0x100 + xf_log_lut_b[h] * l;
	return fr + (ex - FP32_MIDE) * 0x4000000ll;
}

uint32_t xf_log(uint32_t x) {
	int ex = FP32_EXP(x);
	if (FP32_ISNAN(x))
		return x & 0x7fffffff;
	if (!ex)
		return FP32_INF(1);
	if (ex == FP32_MAXE)
		return FP32_INF(0);
	int64_t fr = xf_log_core(x);
	bool sr = fr < 0;
	if (!fr)
		return 0;
	int er = FP32_MIDE + 6;
	fr = norm64(sr ? -fr : fr, &er, 32);
	fr >>= 9;
	fr -= FP32_IONE;
	return sr << 31 | er << 23 | fr;
}

void xf_lit(uint32_t dst[4], uint32_t src[4]) {
	dst[0] = FP32_ONE;
	dst[3] = FP32_ONE;
	if (FP32_SIGN(src[0])) {
		dst[1] = 0;
		dst[2] = 0;
		return;
	}
	dst[1] = src[0];
	if (!FP32_EXP(src[0])) {
		dst[2] = 0;
		return;
	}
	if (!FP32_EXP(src[3])) {
		dst[2] = FP32_ONE;
		return;
	}
	if (FP32_ISNAN(src[1]) || FP32_ISNAN(src[3])) {
		dst[2] = FP32_CNAN;
		return;
	}
	if (FP32_SIGN(src[1]) || !FP32_EXP(src[1])) {
		if (FP32_SIGN(src[3])) {
			dst[2] = FP32_INF(0);
		} else {
			dst[2] = 0;
		}
		return;
	}
	if (FP32_ISINF(src[1])) {
		if (FP32_SIGN(src[3])) {
			dst[2] = 0;
		} else {
			dst[2] = FP32_INF(0);
		}
		return;
	}
	int64_t log = xf_log_core(src[1]);
	int32_t pow = xf_pre_exp(src[3]);
	if (pow < -0x100000)
		pow = -0x100000;
	if (pow > 0xfffff)
		pow = 0xfffff;
	log *= pow;
	log >>= 26;
	if (log < INT32_MIN) {
		dst[2] = 0;
		return;
	}
	if (log > INT32_MAX) {
		dst[2] = FP32_INF(0);
		return;
	}
	dst[2] = xf_exp_core(log);
	return;
}
