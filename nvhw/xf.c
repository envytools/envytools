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

uint32_t xf_sum(const uint32_t *v, int n, int version) {
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
		if (version >= 3 && !ev[i])
			continue;
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
	int flags = FP_FTZ;
	if (version < 3)
		flags |= FP_RZ;
	else
		flags |= FP_RZO;
	uint32_t r = fp32_mkfin(sr, er, res, flags);
	return r;
}

int xf_cond(uint32_t a, uint32_t b, int flags) {
	if (!(flags & FP_ZERO_WINS)) {
		if (FP32_ISNAN(a) || FP32_ISNAN(b))
			return XF_U;
		if (!FP32_EXP(a))
			a = 0;
		if (!FP32_EXP(b))
			b = 0;
	}
	if (a == b)
		return XF_E;
	if (a & 0x80000000)
		a ^= 0x7fffffff;
	if (b & 0x80000000)
		b ^= 0x7fffffff;
	a ^= 0x80000000;
	b ^= 0x80000000;
	return a < b ? XF_L : XF_G;
}

bool xf_test_cond(int cond, int test) {
	switch (test) {
		case XF_FL:
			return false;
		case XF_LT:
			return cond == XF_L;
		case XF_EQ:
			return cond == XF_E;
		case XF_LE:
			return cond == XF_L || cond == XF_E;
		case XF_GT:
			return cond == XF_G;
		case XF_NE:
			return cond != XF_E;
		case XF_GE:
			return cond == XF_E || cond == XF_G;
		case XF_TR:
			return true;
		default:
			abort();
	}
}

uint32_t xf_set(uint32_t a, uint32_t b, int test, int flags) {
	int cond = xf_cond(a, b, flags);
	if (cond == XF_U)
		return FP32_CNAN;
	return xf_test_cond(cond, test) ? FP32_ONE : 0;
}

uint32_t xf_minmax(uint32_t a, uint32_t b, bool min, int flags) {
	int cond = xf_cond(a, b, flags);
	if (cond == XF_U)
		return FP32_CNAN;
	if (min)
		return cond == XF_L ? a : b;
	else
		return cond == XF_L ? b : a;
}

uint32_t xf_ssg(uint32_t x, int flags) {
	switch (xf_cond(x, 0, flags)) {
		case XF_L:
			return FP32_ONE ^ 0x80000000;
		case XF_E:
			return 0;
		case XF_G:
			return FP32_ONE;
		case XF_U:
			return FP32_CNAN;
		default:
			abort();
	}
}

uint32_t xf_frc(uint32_t x) {
	return xf_add(x, xf_flr(x) ^ 0x80000000, 3);
}

uint32_t xf_flr(uint32_t x) {
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (!ex)
		return 0;
	if (FP32_ISNAN(x))
		return FP32_CNAN;
	if (FP32_ISINF(x))
		return x;
	if (ex >= FP32_MIDE + 23)
		return x;
	int shift = FP32_MIDE + 23 - ex;
	if (!sx) {
		if (ex < FP32_MIDE)
			return 0;
		fx &= -(1 << shift);
	} else {
		if (ex < FP32_MIDE)
			return FP32_ONE ^ 0x80000000;
		fx--;
		fx &= -(1 << shift);
		fx += 1 << shift;
		if (fx == FP32_IONE) {
			fx = 0;
			ex++;
		}
	}
	return sx << 31 | ex << 23 | fx;
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

uint32_t xf_rsq(uint32_t x, int version, bool abs) {
	if (FP32_ISNAN(x))
		return FP32_CNAN;
	bool sx = FP32_SIGN(x) && !abs;
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (!ex)
		return FP32_INF(sx);
	if (sx)
		return FP32_CNAN;
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
	const uint8_t *lut = version >= 2 ? xf_rsq_lut_v2 : xf_rsq_lut_v1;
	uint64_t s0 = lut[fx >> 17 & 0x7f];
	if (version >= 3 && fx == 0) {
		er++;
		fr = 0;
	} else {
		if (fx >= 0x800000)
			fx *= 2;
		else
			fx += 0x800000;
		uint64_t s1 = ((3ull << 37) - s0 * s0 * fx) * s0 >> 32;
		uint64_t s2 = ((3ull << 49) - s1 * s1 * fx) * s1 >> 39;
		fr = s2 - 0x800000;
		if (fr >= 0x800000)
			abort();
	}
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

uint32_t xf_log_e(uint32_t x, int version, int flags) {
	int sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	if (!(flags & FP_ZERO_WINS) && sx)
		return FP32_CNAN;
	if (FP32_ISNAN(x)) {
		if (version < 3)
			return x & 0x7fffffff;
		else
			return FP32_CNAN;
	}
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

uint32_t xf_log_f(uint32_t x, int version, int flags) {
	int sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (!(flags & FP_ZERO_WINS) && sx)
		return FP32_CNAN;
	if (FP32_ISNAN(x)) {
		if (version < 3)
			return x & 0x7fffffff;
		else
			return FP32_CNAN;
	}
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

uint32_t xf_log(uint32_t x, int version, int flags) {
	int sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	if (!(flags & FP_ZERO_WINS) && sx)
		return FP32_CNAN;
	if (FP32_ISNAN(x)) {
		if (version < 3)
			return x & 0x7fffffff;
		else
			return FP32_CNAN;
	}
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

struct xf_sf_lut_el {
	uint32_t c0;
	uint32_t c1;
	uint32_t c2;
};

static uint32_t xf_sf_square(int32_t x) {
	uint32_t res = (x >> 7) * (x >> 7);
	if (x & 1 << 6)
		res += x >> 7;
	for (int i = 0; i < 6; i++)
		if (x & 1 << i)
			res += (x + (1 << (12 - i))) >> (13 - i);
	return res;
}

static int64_t xf_sf_shl(int32_t x, int shift) {
	if (shift >= 0) {
		return (int64_t)x << shift;
	} else {
		return (x + (1 << (-shift - 1))) >> -shift;
	}
}

static int64_t xf_sf_mul(uint32_t c, int32_t x) {
	int64_t res = 0;
	uint32_t tmp = c << 1;
	int shift = -14;
	while (tmp) {
		switch (tmp & 7) {
			case 0:
			case 7:
				break;
			case 1:
			case 2:
				res += xf_sf_shl(x, shift);
				break;
			case 3:
				res += xf_sf_shl(x, shift + 1);
				break;
			case 4:
				res -= xf_sf_shl(x, shift + 1);
				break;
			case 5:
			case 6:
				res -= xf_sf_shl(x, shift);
				break;
		}
		tmp >>= 2;
		shift += 2;
	}
	return res;
}

static uint64_t xf_sf(const struct xf_sf_lut_el *lut, uint32_t x, bool n2) {
	assert(x < (1 << 23));
	uint8_t t = x >> 16;
	int32_t rx;
	uint8_t rt;
	if (t & 1) {
		rt = (t + 1) >> 1;
		rx = (x & 0xffff) - 0x10000;
	} else {
		rt = t >> 1;
		rx = x & 0xffff;
	}
	int32_t sx = xf_sf_square(rx);
	int64_t res = (uint64_t)lut[rt].c0 << 15;
	res += xf_sf_mul(lut[rt].c1, rx) << 6;
	if (n2) {
		int32_t dsx = (sx - 0x400) >> 12;
		res -= ((int64_t)lut[rt].c2 * sx + dsx + 0x20) >> 5;
	} else {
		res += ((int64_t)lut[rt].c2 * sx) >> 5;
	}
	return res;
}

static const struct xf_sf_lut_el xf_lg2_lut[65] = {
	{0x000000, 0xb8aa3b, 0x5c0},
	{0x02dcf3, 0xb5d303, 0x598},
	{0x05aeb4, 0xb311cc, 0x568},
	{0x08759b, 0xb06584, 0x540},
	{0x0b31fa, 0xadcd64, 0x518},
	{0x0de420, 0xab48a5, 0x4f0},
	{0x108c58, 0xa8d627, 0x4d0},
	{0x132aea, 0xa67566, 0x4b0},
	{0x15c019, 0xa4258b, 0x488},
	{0x184c2a, 0xa1e5e6, 0x468},
	{0x1acf5e, 0x9fb5d2, 0x450},
	{0x1d49ee, 0x9d94ad, 0x430},
	{0x1fbc15, 0x9b81e0, 0x410},
	{0x22260f, 0x997cde, 0x3f8},
	{0x24880f, 0x97851c, 0x3e0},
	{0x26e249, 0x959a1c, 0x3c8},
	{0x2934f0, 0x93bb62, 0x3b0},
	{0x2b8034, 0x91e87a, 0x398},
	{0x2dc443, 0x9020f5, 0x380},
	{0x30014a, 0x8e646b, 0x368},
	{0x323775, 0x8cb276, 0x358},
	{0x3466ec, 0x8b0ab7, 0x340},
	{0x368fd8, 0x896cd2, 0x330},
	{0x38b25e, 0x87d872, 0x318},
	{0x3acea7, 0x864d42, 0x308},
	{0x3ce4d5, 0x84caf3, 0x2f8},
	{0x3ef50a, 0x83513b, 0x2e8},
	{0x40ff6a, 0x81dfcf, 0x2d8},
	{0x430414, 0x80766b, 0x2c8},
	{0x450328, 0x7f14cd, 0x2b8},
	{0x46fcc4, 0x7dbab5, 0x2a8},
	{0x48f107, 0x7c67e7, 0x298},
	{0x4ae00d, 0x7b1c27, 0x290},
	{0x4cc9f1, 0x79d73e, 0x280},
	{0x4eaed0, 0x7898f7, 0x270},
	{0x508ec2, 0x77611e, 0x268},
	{0x5269e1, 0x762f82, 0x258},
	{0x544046, 0x7503f2, 0x250},
	{0x561208, 0x73de43, 0x240},
	{0x57df40, 0x72be47, 0x238},
	{0x59a802, 0x71a3d5, 0x228},
	{0x5b6c65, 0x708ec4, 0x220},
	{0x5d2c7f, 0x6f7eee, 0x218},
	{0x5ee864, 0x6e742c, 0x210},
	{0x60a027, 0x6d6e5b, 0x200},
	{0x6253dd, 0x6c6d58, 0x1f8},
	{0x640398, 0x6b7101, 0x1f0},
	{0x65af6b, 0x6a7936, 0x1e8},
	{0x675768, 0x6985d8, 0x1e0},
	{0x68fba0, 0x6896c9, 0x1d8},
	{0x6a9c24, 0x67abeb, 0x1d0},
	{0x6c3904, 0x66c523, 0x1c8},
	{0x6dd252, 0x65e255, 0x1c0},
	{0x6f681c, 0x650368, 0x1b8},
	{0x70fa72, 0x642842, 0x1b0},
	{0x728963, 0x6350cb, 0x1a8},
	{0x7414fd, 0x627cec, 0x1a0},
	{0x759d4f, 0x61ac8d, 0x198},
	{0x772266, 0x60df98, 0x190},
	{0x78a450, 0x6015f9, 0x188},
	{0x7a231b, 0x5f4f9a, 0x188},
	{0x7b9ed2, 0x5e8c68, 0x180},
	{0x7d1782, 0x5dcc4e, 0x178},
	{0x7e8d38, 0x5d0f3c, 0x170},
	{0x800000, 0x5c551d, 0x170},
};

uint32_t xf_lg2(uint32_t x) {
	int sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	int fx = FP32_FRACT(x);
	if (!ex)
		return FP32_INF(1);
	if (FP32_ISNAN(x) || sx)
		return FP32_CNAN;
	if (FP32_ISINF(x))
		return FP32_INF(0);
	int64_t fr = xf_sf(xf_lg2_lut, fx, true);
	fr += (int64_t)(ex - FP32_MIDE) << 38;
	bool sr = fr < 0;
	if (sr)
		fr = ~fr;
	if (!fr)
		return 0;
	int er = FP32_MIDE + 8;
	fr = norm64(fr, &er, 23 + 15 + 8);
	fr >>= 15 + 8;
	fr -= FP32_IONE;
	return sr << 31 | er << 23 | fr;
}

static const struct xf_sf_lut_el xf_ex2_lut[65] = {
	{0x000001, 0x58b90b, 0x1e8},
	{0x0164d2, 0x59b060, 0x1f0},
	{0x02cd88, 0x5aaa66, 0x1f0},
	{0x043a2a, 0x5ba724, 0x1f8},
	{0x05aac4, 0x5ca6a3, 0x200},
	{0x071f63, 0x5da8eb, 0x200},
	{0x089810, 0x5eae02, 0x208},
	{0x0a14d6, 0x5fb5f1, 0x210},
	{0x0b95c2, 0x60c0c0, 0x218},
	{0x0d1ae1, 0x61ce77, 0x218},
	{0x0ea43b, 0x62df1e, 0x220},
	{0x1031dd, 0x63f2bc, 0x228},
	{0x11c3d3, 0x65095b, 0x230},
	{0x135a2d, 0x662303, 0x230},
	{0x14f4f1, 0x673fbc, 0x238},
	{0x16942e, 0x685f8f, 0x240},
	{0x1837f0, 0x698284, 0x248},
	{0x19e047, 0x6aa8a4, 0x248},
	{0x1b8d3b, 0x6bd1f8, 0x250},
	{0x1d3edb, 0x6cfe89, 0x258},
	{0x1ef533, 0x6e2e5f, 0x260},
	{0x20b051, 0x6f6185, 0x268},
	{0x227043, 0x709803, 0x270},
	{0x243517, 0x71d1e3, 0x270},
	{0x25fed8, 0x730f2d, 0x278},
	{0x27cd95, 0x744fec, 0x280},
	{0x29a15c, 0x759429, 0x288},
	{0x2b7a3a, 0x76dbee, 0x290},
	{0x2d583f, 0x782745, 0x298},
	{0x2f3b79, 0x797637, 0x2a0},
	{0x3123f6, 0x7ac8cf, 0x2a8},
	{0x3311c4, 0x7c1f17, 0x2b0},
	{0x3504f5, 0x7d7919, 0x2b0},
	{0x36fd94, 0x7ed6df, 0x2b8},
	{0x38fbb1, 0x803875, 0x2c0},
	{0x3aff5c, 0x819de5, 0x2c8},
	{0x3d08a5, 0x830738, 0x2d0},
	{0x3f179b, 0x84747b, 0x2d8},
	{0x412c4e, 0x85e5b9, 0x2e0},
	{0x4346ce, 0x875afb, 0x2e8},
	{0x45672c, 0x88d44e, 0x2f0},
	{0x478d76, 0x8a51bd, 0x2f8},
	{0x49b9bf, 0x8bd353, 0x300},
	{0x4bec17, 0x8d591c, 0x308},
	{0x4e248c, 0x8ee324, 0x318},
	{0x506334, 0x907176, 0x320},
	{0x52a81e, 0x92041f, 0x328},
	{0x54f35b, 0x939b2a, 0x330},
	{0x5744fe, 0x9536a3, 0x338},
	{0x599d17, 0x96d698, 0x340},
	{0x5bfbb9, 0x987b14, 0x348},
	{0x5e60f6, 0x9a2425, 0x350},
	{0x60ccdf, 0x9bd1d6, 0x360},
	{0x633f8a, 0x9d8435, 0x368},
	{0x65b908, 0x9f3b4f, 0x370},
	{0x68396b, 0xa0f731, 0x378},
	{0x6ac0c9, 0xa2b7e9, 0x380},
	{0x6d4f30, 0xa47d83, 0x390},
	{0x6fe4ba, 0xa6480e, 0x398},
	{0x728178, 0xa81797, 0x3a0},
	{0x75257f, 0xa9ec2d, 0x3a8},
	{0x77d0e0, 0xabc5dc, 0x3b8},
	{0x7a83b4, 0xada4b4, 0x3c0},
	{0x7d3e0e, 0xaf88c3, 0x3c8},
	{0x800001, 0xb17217, 0x3d0},
};

uint32_t xf_ex2(uint32_t x) {
	int sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	int fx = FP32_FRACT(x);
	if (FP32_ISNAN(x))
		return FP32_CNAN;
	if (ex < 0x46)
		return FP32_ONE;
	if (ex >= 0x86)
		return sx ? 0 : FP32_INF(0);
	int64_t t = FP32_IONE + fx;
	int shift = ex - FP32_MIDE;
	if (shift > 0)
		t <<= shift;
	else
		t >>= -shift;
	if (sx)
		t = ~t;
	int64_t fr = xf_sf(xf_ex2_lut, t & 0x7fffff, false) >> 15;
	int er = FP32_MIDE + (t >> 23);
	if (er <= 0)
		return 0;
	return er << 23 | fr;
}
