/*
 * Copyright (C) 2017 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "nvhw/pgraph.h"
#include "nvhw/fp.h"

uint32_t pgraph_celsius_xfrm_squash(uint32_t val) {
	if (extr(val, 24, 7) == 0x7f)
		val &= 0xff000000;
	return val;
}

uint32_t pgraph_celsius_xfrm_squash_xy(uint32_t val) {
	int exp = extr(val, 23, 8);
	if (exp <= 0x7a)
		return 0;
	if (exp < 0x92)
		insrt(val, 0, 0x92 - exp, 0);
	return val;
}

uint32_t pgraph_celsius_xfrm_squash_z(struct pgraph_state *state, uint32_t val) {
	if (extr(state->debug[4], 22, 1) && !extr(state->celsius_raster, 30, 1)) {
		if (extr(val, 31, 1))
			return 0;
	}
	return val;
}

uint32_t pgraph_celsius_xfrm_squash_w(struct pgraph_state *state, uint32_t val) {
	int exp = extr(val, 23, 8);
	if (extr(state->debug[4], 22, 1) && !extr(state->celsius_raster, 30, 1)) {
		if (exp < 0x3f || extr(val, 31, 1)) {
			return 0x1f800000;
		}
	}
	if (exp >= 0xbf)
		insrt(val, 0, 31, 0x5f800000);
	val &= 0xffffff00;
	return val;
}

uint8_t pgraph_celsius_xfrm_f2b(uint32_t val) {
	if (extr(val, 31, 1))
		return 0;
	int exp = extr(val, 23, 8);
	if (exp >= 0x7f)
		return 0xff;
	if (exp < 0x76)
		return 0;
	uint32_t fr = extr(val, 0, 23);
	fr |= 1 << 23;
	fr <<= exp - 0x76;
	uint8_t res = fr >> 24;
	if (res >= 0x80)
		res--;
	uint32_t target = res * 0x01010101 + 0x00808080;
	if (fr > target)
		res++;
	return res;
}

uint32_t pgraph_celsius_xfrm_mul(uint32_t a, uint32_t b) {
	bool sign = extr(a, 31, 1) ^ extr(b, 31, 1);
	int expa = extr(a, 23, 8);
	int expb = extr(b, 23, 8);
	uint32_t fra = extr(a, 0, 23) | 1 << 23;
	uint32_t frb = extr(b, 0, 23) | 1 << 23;
	if ((expa == 0xff && fra > 0x800000) || (expb == 0xff && frb > 0x800000))
		return 0x7fffffff;
	if (!expa || !expb)
		return 0;
	if (expa == 0xff || expb == 0xff)
		return 0x7f800000;
	uint32_t fr;
	int exp;
	exp = expa + expb - 0x7f;
	uint64_t frx = (uint64_t)fra * frb;
	frx >>= 23;
	if (frx > 0xffffff) {
		frx >>= 1;
		exp++;
	}
	fr = frx;
	if (exp <= 0 || expa == 0 || expb == 0) {
		exp = 0;
		fr = 0;
	} else if (exp >= 0xff) {
		exp = 0xfe;
		fr = 0x7fffff;
	}
	uint32_t res = sign << 31 | exp << 23 | (fr & 0x7fffff);
	return res;
}

uint32_t pgraph_celsius_xfrm_add(uint32_t a, uint32_t b) {
	bool sa, sb, sr;
	int ea, eb, er;
	uint32_t fa, fb;
	sa = FP32_SIGN(a);
	ea = FP32_EXP(a);
	fa = FP32_FRACT(a);
	sb = FP32_SIGN(b);
	eb = FP32_EXP(b);
	fb = FP32_FRACT(b);
	if (FP32_ISNAN(a) || FP32_ISNAN(b))
		return FP32_CNAN;
	if (FP32_ISINF(a) || FP32_ISINF(b)) {
		if (FP32_ISINF(a) && FP32_ISINF(b) && sa != sb)
			return FP32_CNAN;
		else
			return FP32_INF(0);
	}
	/* Two honest real numbers involved. */
	if (ea != 0)
		fa |= FP32_IONE;
	if (eb != 0)
		fb |= FP32_IONE;
	er = max(ea, eb) + 1;
	int32_t res = 0;
	fa = shr32(fa, er - ea - 6, FP_RZ);
	fb = shr32(fb, er - eb - 6, FP_RZ);
	res += sa ? -fa : fa;
	res += sb ? -fb : fb;
	if (res == 0) {
		/* Got a proper 0. */
		er = 0;
		sr = 0;
	} else {
		/* Compute sign, make sure accumulator is positive. */
		sr = res < 0;
		if (sr)
			res = -res;
		res = norm32(res, &er, 29);
		/* Round it. */
		res = shr32(res, 6, FP_RZ);
	}
	uint32_t r = fp32_mkfin(sr, er, res, FP_RZ, true);
	return r;
}

uint32_t pgraph_celsius_xfrm_add4(uint32_t *v) {
	for (int i = 0; i < 4; i++) {
		if (FP32_ISNAN(v[i]))
			return FP32_CNAN;
	}
	for (int i = 0; i < 4; i++) {
		if (FP32_ISINF(v[i]))
			return FP32_INF(0);
	}
	/* Only honest real numbers involved. */
	bool sv[4], sr;
	int ev[4], er = 0;
	uint32_t fv[4];
	for (int i = 0; i < 4; i++) {
		sv[i] = FP32_SIGN(v[i]);
		ev[i] = FP32_EXP(v[i]);
		fv[i] = FP32_FRACT(v[i]);
		if (ev[i])
			fv[i] |= FP32_IONE;
		if (ev[i] + 2 > er)
			er = ev[i] + 2;
	}
	int32_t res = 0;
	for (int i = 0; i < 4; i++) {
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
	uint32_t r = fp32_mkfin(sr, er, res, FP_RZ, true);
	return r;
}

uint32_t pgraph_celsius_xfrm_rcp_core(uint32_t x) {
	static const uint8_t lut[0x40] = {
		0x3f, 0x3d, 0x3b, 0x39, 0x37, 0x35, 0x34, 0x32, 0x30, 0x2f, 0x2d, 0x2c, 0x2b, 0x29, 0x28, 0x27,
		0x25, 0x24, 0x23, 0x22, 0x20, 0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15,
		0x14, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0f, 0x0e, 0x0d, 0x0c, 0x0c, 0x0b, 0x0a, 0x0a, 0x09,
		0x08, 0x08, 0x07, 0x06, 0x06, 0x05, 0x05, 0x04, 0x03, 0x03, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00,
	};
	if (x >= 0x800000)
		abort();
	x += 0x800000;
	uint64_t s0 = lut[x >> 17 & 0x3f] | 0x40;
	uint64_t s1 = ((1u << 31) - s0 * x) * s0 >> 24;
	uint64_t s2 = ((1ull << 37) - s1 * x) * s1 >> 25;
	s2 -= 0x800000;
	if (s2 >= 0x800000)
		abort();
	return s2;
}

uint32_t pgraph_celsius_xfrm_rcc(uint32_t x) {
	if (FP32_ISNAN(x))
		return FP32_CNAN;
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	int er = 0xfe - ex;
	uint32_t fr = 0;
	if (ex == 0)
		fx = 0;
	if (fx) {
		fr = pgraph_celsius_xfrm_rcp_core(fx);
		er--;
	}
	if (er < 0x3f)
		er = 0x3f;
	if (er > 0xbf)
		er = 0xbf;
	return sx << 31 | er << 23 | fr;
}

uint32_t pgraph_celsius_xfrm_rcp(uint32_t x) {
	if (FP32_ISNAN(x))
		return FP32_CNAN;
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (!ex)
		return FP32_INF(0);
	int er = 0xfe - ex;
	uint32_t fr = 0;
	if (ex == 0)
		fx = 0;
	if (fx) {
		fr = pgraph_celsius_xfrm_rcp_core(fx);
		er--;
	}
	if (er <= 0) {
		er = 0;
		fr = 0;
	}
	return sx << 31 | er << 23 | fr;
}

uint32_t pgraph_celsius_xfrm_rsqrt_core(uint32_t x) {
	static const uint8_t lut[0x80] = {
		0x3f, 0x3e, 0x3d, 0x3c, 0x3b, 0x3a, 0x39, 0x39, 0x38, 0x37, 0x36, 0x35, 0x35, 0x34, 0x33, 0x32,
		0x32, 0x31, 0x30, 0x30, 0x2f, 0x2e, 0x2e, 0x2d, 0x2c, 0x2c, 0x2b, 0x2b, 0x2a, 0x29, 0x29, 0x28,
		0x28, 0x27, 0x27, 0x26, 0x26, 0x25, 0x25, 0x24, 0x24, 0x23, 0x23, 0x22, 0x22, 0x21, 0x21, 0x20,
		0x20, 0x20, 0x1f, 0x1f, 0x1e, 0x1e, 0x1e, 0x1d, 0x1d, 0x1c, 0x1c, 0x1c, 0x1b, 0x1b, 0x1b, 0x1a,

		0x1a, 0x19, 0x18, 0x18, 0x17, 0x16, 0x16, 0x15, 0x15, 0x14, 0x13, 0x13, 0x12, 0x12, 0x11, 0x11,
		0x10, 0x10, 0x0f, 0x0f, 0x0e, 0x0e, 0x0d, 0x0d, 0x0c, 0x0c, 0x0c, 0x0b, 0x0b, 0x0a, 0x0a, 0x0a,
		0x09, 0x09, 0x08, 0x08, 0x08, 0x07, 0x07, 0x07, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x04, 0x04,
		0x04, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	};
	if (x >= 0x1000000)
		abort();
	uint64_t s0 = lut[x >> 17 & 0x7f] | 0x40;
	if (x >= 0x800000)
		x *= 2;
	else
		x += 0x800000;
	uint64_t s1 = ((3ull << 37) - s0 * s0 * x) * s0 >> 32;
	uint64_t s2 = ((3ull << 49) - s1 * s1 * x) * s1 >> 39;
	s2 -= 0x800000;
	if (s2 >= 0x800000)
		abort();
	return s2;
}

uint32_t pgraph_celsius_xfrm_rsqrt(uint32_t x) {
	if (FP32_ISNAN(x))
		return FP32_CNAN;
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (!ex || sx)
		return FP32_INF(0);
	if (FP32_ISINF(x))
		return 0;
	int er;
	uint32_t fr;
	if (ex & 1) {
		er = 0x7f - (ex - 0x7f) / 2 - 1;
		fr = pgraph_celsius_xfrm_rsqrt_core(fx);
	} else {
		er = 0x7f - (ex - 0x80) / 2 - 1;
		fr = pgraph_celsius_xfrm_rsqrt_core(fx | 0x800000);
	}
	return er << 23 | fr;
}

void pgraph_celsius_xfrm_vmul(uint32_t dst[4], uint32_t a[4], uint32_t b[4]) {
	for (int i = 0; i < 4; i++) {
		dst[i] = pgraph_celsius_xfrm_mul(a[i], b[i]);
	}
}

void pgraph_celsius_xfrm_vmula(uint32_t dst[4], uint32_t a[4], uint32_t b[4]) {
	for (int i = 0; i < 3; i++) {
		dst[i] = pgraph_celsius_xfrm_mul(a[i], b[i]);
	}
	dst[3] = a[3];
}

void pgraph_celsius_xfrm_vadda(uint32_t dst[4], uint32_t a[4], uint32_t b[4]) {
	for (int i = 0; i < 3; i++) {
		dst[i] = pgraph_celsius_xfrm_add(a[i], b[i]);
	}
	dst[3] = a[3];
}

void pgraph_celsius_xfrm_vsuba(uint32_t dst[4], uint32_t a[4], uint32_t b[4]) {
	for (int i = 0; i < 3; i++) {
		dst[i] = pgraph_celsius_xfrm_add(a[i], b[i] ^ 0x80000000);
	}
	dst[3] = a[3];
}

void pgraph_celsius_xfrm_vsmr(uint32_t dst[4], uint32_t a) {
	for (int i = 0; i < 4; i++)
		dst[i] = a;
}

void pgraph_celsius_xfrm_vmov(uint32_t dst[4], uint32_t a[4]) {
	for (int i = 0; i < 4; i++)
		dst[i] = a[i];
}

uint32_t pgraph_celsius_xfrm_dp4(uint32_t a[4], uint32_t b[4]) {
	uint32_t tmp[4];
	pgraph_celsius_xfrm_vmul(tmp, a, b);
	return pgraph_celsius_xfrm_add4(tmp);
}

uint32_t pgraph_celsius_xfrm_dp3(uint32_t a[4], uint32_t b[4]) {
	uint32_t tmp[4];
	pgraph_celsius_xfrm_vmul(tmp, a, b);
	tmp[3] = 0;
	return pgraph_celsius_xfrm_add4(tmp);
}

void pgraph_celsius_xfrm_mmul(uint32_t dst[4], uint32_t a[4], uint32_t b[4][4]) {
	uint32_t res[4];
	for (int i = 0; i < 4; i++) {
		res[i] = pgraph_celsius_xfrm_dp4(a, b[i]);
	}
	pgraph_celsius_xfrm_vmov(dst, res);
}

void pgraph_celsius_xfrm_mmul3(uint32_t dst[4], uint32_t a[4], uint32_t b[4][4]) {
	uint32_t res[4];
	for (int i = 0; i < 4; i++) {
		res[i] = pgraph_celsius_xfrm_dp3(a, b[i]);
	}
	pgraph_celsius_xfrm_vmov(dst, res);
}

struct pgraph_celsius_xf_res {
	uint32_t pos[4];
	uint32_t txc[2][4];
};

void pgraph_celsius_xf_bypass(struct pgraph_celsius_xf_res *res, struct pgraph_state *state) {
	uint32_t *vab = state->celsius_pipe_vtx;
	uint32_t (*xfctx)[4] = state->celsius_pipe_xfrm;
	uint32_t mode_a = state->celsius_xf_misc_a;
	uint32_t mode_b = state->celsius_xf_misc_b;

	// Compute POS.
	uint32_t *ipos = &vab[0];
	uint32_t rw[4];
	int offset_idx = extr(mode_a, 29, 1) ? 0x0e : 0x0d;
	pgraph_celsius_xfrm_vmul(res->pos, ipos, xfctx[0x0c]);
	pgraph_celsius_xfrm_vadda(res->pos, res->pos, xfctx[offset_idx]);
	pgraph_celsius_xfrm_vsmr(rw, res->pos[3]);

	// Compute TXC.
	uint32_t *itxc[2] = {&vab[3*4], &vab[4*4]};
	for (int i = 0; i < 2; i++) {
		if (extr(mode_b, i * 14 + 2, 1)) {
			pgraph_celsius_xfrm_vmul(res->txc[i], itxc[i], rw);
		} else {
			pgraph_celsius_xfrm_vmov(res->txc[i], itxc[i]);
		}
	}
}

void pgraph_celsius_xf_full(struct pgraph_celsius_xf_res *res, struct pgraph_state *state) {
	uint32_t *vab = state->celsius_pipe_vtx;
	uint32_t (*xfctx)[4] = state->celsius_pipe_xfrm;
	uint32_t mode_a = state->celsius_xf_misc_a;
	uint32_t mode_b = state->celsius_xf_misc_b;

	// Compute WEI.
	bool weight = extr(mode_a, 27, 1);
	uint32_t vwei[4], viwei[4];
	if (weight) {
		uint32_t iwei = vab[6*4];
		pgraph_celsius_xfrm_vsmr(vwei, iwei);
		pgraph_celsius_xfrm_vsuba(viwei, xfctx[0x3a], vwei);
	}

	// Compute POS.
	uint32_t *ipos = &vab[0];
	uint32_t rw[4];
	uint32_t epos[4];
	{
		uint32_t mepos[2][4];
		uint32_t cpos[4];
		pgraph_celsius_xfrm_mmul(mepos[0], ipos, &xfctx[0]);
		pgraph_celsius_xfrm_mmul(mepos[1], ipos, &xfctx[0xc]);
		if (weight) {
			uint32_t tmp[2][4];
			pgraph_celsius_xfrm_vmula(tmp[0], mepos[0], vwei);
			pgraph_celsius_xfrm_vmula(tmp[1], mepos[1], viwei);
			pgraph_celsius_xfrm_vadda(epos, tmp[0], tmp[1]);
			pgraph_celsius_xfrm_mmul(cpos, epos, &xfctx[8]);
		} else {
			pgraph_celsius_xfrm_vmov(epos, mepos[0]);
			pgraph_celsius_xfrm_mmul(cpos, ipos, &xfctx[8]);
		}
		pgraph_celsius_xfrm_vsmr(rw, pgraph_celsius_xfrm_rcc(cpos[3]));
		pgraph_celsius_xfrm_vmula(res->pos, rw, cpos);
		pgraph_celsius_xfrm_vadda(res->pos, res->pos, xfctx[0x39]);
	}

	// Compute NRM.
	uint32_t rnrm[4];
	{
		uint32_t *inrm = &vab[5*4];
		uint32_t enrm[4];
		if (weight) {
			uint32_t menrm[2][4];
			pgraph_celsius_xfrm_mmul3(menrm[0], inrm, &xfctx[4]);
			pgraph_celsius_xfrm_mmul3(menrm[1], inrm, &xfctx[0x10]);
			uint32_t tmp[2][4];
			pgraph_celsius_xfrm_vmula(tmp[0], menrm[0], vwei);
			pgraph_celsius_xfrm_vmula(tmp[1], menrm[1], viwei);
			pgraph_celsius_xfrm_vadda(enrm, tmp[0], tmp[1]);
		} else {
			pgraph_celsius_xfrm_mmul3(enrm, inrm, &xfctx[4]);
		}
		if (extr(mode_b, 30, 1)) {
			uint32_t rnf[4];
			uint32_t d = pgraph_celsius_xfrm_dp3(enrm, enrm);
			pgraph_celsius_xfrm_vsmr(rnf, pgraph_celsius_xfrm_rsqrt(d));
			pgraph_celsius_xfrm_vmul(rnrm, enrm, rnf);
		} else {
			pgraph_celsius_xfrm_vmov(rnrm, enrm);
		}
	}

	// Compute R.
	uint32_t rp[4] = { 0 };
	uint32_t rm[4];
	{
		uint32_t u[4], tmp[4], ru[4], dp[4];
		uint32_t rwe[4];
		pgraph_celsius_xfrm_vsmr(rwe, pgraph_celsius_xfrm_rcp(epos[3]));
		pgraph_celsius_xfrm_vmul(u, epos, rwe);
		pgraph_celsius_xfrm_vsuba(u, xfctx[0x34], u);
		uint32_t d = pgraph_celsius_xfrm_dp3(u, u);
		pgraph_celsius_xfrm_vsmr(ru, pgraph_celsius_xfrm_rsqrt(d));
		pgraph_celsius_xfrm_vmul(u, u, ru);

		pgraph_celsius_xfrm_vmul(tmp, xfctx[0x35], rnrm);
		d = pgraph_celsius_xfrm_dp3(u, tmp);
		pgraph_celsius_xfrm_vsmr(dp, d);
		pgraph_celsius_xfrm_vmul(tmp, rnrm, dp);
		pgraph_celsius_xfrm_vadda(rp, tmp, xfctx[0x36]);
		pgraph_celsius_xfrm_vsuba(rp, rp, u);
		pgraph_celsius_xfrm_vsuba(rm, rp, xfctx[0x36]);
	}
	uint32_t sm[4];
	{
		uint32_t d = pgraph_celsius_xfrm_dp3(rp, rp);
		uint32_t rs[4];
		pgraph_celsius_xfrm_vsmr(rs, pgraph_celsius_xfrm_rsqrt(d));
		uint32_t tmp[4];
		pgraph_celsius_xfrm_vmul(tmp, rp, xfctx[0x37]);
		pgraph_celsius_xfrm_vmul(tmp, tmp, rs);
		pgraph_celsius_xfrm_vadda(sm, tmp, xfctx[0x37]);
	}

	// Compute TXC.
	uint32_t *itxc[2] = {&vab[3*4], &vab[4*4]};
	for (int i = 0; i < 2; i++) {
		uint32_t txconf = extr(mode_b, i * 14, 14);
		if (!extr(txconf, 0, 1))
			continue;
		if (!extr(txconf, 2, 1)) {
			// Without enabled perspective, result is non-deterministic.
			abort();
		}
		bool mat_en = extr(txconf, 1, 1);
		uint32_t ptxc[4];
		uint32_t mtxc[4];
		pgraph_celsius_xfrm_vmov(ptxc, itxc[i]);
		for (int j = 0; j < 4; j++) {
			int mode = extr(txconf, j * 3 + 3, 3);
			switch (mode) {
				case 1:
					if (!mat_en)
						abort();
					ptxc[j] = pgraph_celsius_xfrm_dp4(epos, xfctx[0x14 + i * 8 + j]);
					break;
				case 2:
					if (!mat_en)
						abort();
					ptxc[j] = pgraph_celsius_xfrm_dp4(ipos, xfctx[0x14 + i * 8 + j]);
					break;
				case 3:
					if (j >= 2)
						break;
					if (!mat_en)
						abort();
					ptxc[j] = sm[j];
					break;
				case 4:
					if (!mat_en)
						abort();
					ptxc[j] = rnrm[j];
					break;
				case 5:
					if (!mat_en)
						abort();
					ptxc[j] = rm[j];
					break;
				case 6:
					if (i != 1 || j != 0)
						break;
					if (!mat_en)
						abort();
					// XXX emboss
					abort();
					break;
			}
		}
		if (mat_en) {
			pgraph_celsius_xfrm_mmul(mtxc, ptxc, &xfctx[0x18 + i * 8]);
		} else {
			pgraph_celsius_xfrm_vmov(mtxc, ptxc);
		}
		pgraph_celsius_xfrm_vmul(res->txc[i], mtxc, rw);
	}

	// XXX Compute things for light.
}

void pgraph_celsius_xfrm(struct pgraph_state *state, int idx) {
	bool bypass = extr(state->celsius_xf_misc_a, 28, 1);

	uint32_t opos[4];
	uint32_t otxc[2][4];
	uint32_t icol[2][4];
	uint32_t optsz;
	uint32_t ifog, ofog;
	uint8_t ocol[2][4];
	icol[0][0] = state->celsius_pipe_vtx[1*4+0] & 0xfffffc00;
	icol[0][1] = state->celsius_pipe_vtx[1*4+1] & 0xfffffc00;
	icol[0][2] = state->celsius_pipe_vtx[1*4+2] & 0xfffffc00;
	icol[0][3] = state->celsius_pipe_vtx[1*4+3] & 0xfffffc00;
	icol[1][0] = state->celsius_pipe_vtx[2*4+0] & 0xfffffc00;
	icol[1][1] = state->celsius_pipe_vtx[2*4+1] & 0xfffffc00;
	icol[1][2] = state->celsius_pipe_vtx[2*4+2] & 0xfffffc00;
	ifog = pgraph_celsius_convert_light_sx(state->celsius_pipe_vtx[2*4+3]);

	struct pgraph_celsius_xf_res xf;
	if (bypass)
		pgraph_celsius_xf_bypass(&xf, state);
	else
		pgraph_celsius_xf_full(&xf, state);

	// Compute COL.
	ocol[0][0] = pgraph_celsius_xfrm_f2b(icol[0][0]);
	ocol[0][1] = pgraph_celsius_xfrm_f2b(icol[0][1]);
	ocol[0][2] = pgraph_celsius_xfrm_f2b(icol[0][2]);
	ocol[0][3] = pgraph_celsius_xfrm_f2b(icol[0][3]);
	ocol[1][0] = pgraph_celsius_xfrm_f2b(icol[1][0]);
	ocol[1][1] = pgraph_celsius_xfrm_f2b(icol[1][1]);
	ocol[1][2] = pgraph_celsius_xfrm_f2b(icol[1][2]);
	if (!bypass) {
		if (!extr(state->celsius_xf_misc_a, 20, 1) || !extr(state->celsius_xf_misc_a, 19, 1)) {
			ocol[1][0] = 0;
			ocol[1][1] = 0;
			ocol[1][2] = 0;
		}
	}

	// Compute FOG.
	ofog = ifog;
	insrt(ofog, 10, 1, extr(ofog, 11, 1));

	// Compute PTSZ.
	optsz = pgraph_celsius_convert_light_v(state->celsius_pipe_vtx[6*4]);
	optsz &= 0x001ff000;

	opos[0] = pgraph_celsius_xfrm_squash_xy(xf.pos[0]);
	opos[1] = pgraph_celsius_xfrm_squash_xy(xf.pos[1]);
	opos[2] = pgraph_celsius_xfrm_squash_z(state, xf.pos[2]);
	opos[3] = pgraph_celsius_xfrm_squash_w(state, xf.pos[3]);

	opos[0] = pgraph_celsius_xfrm_squash(opos[0]);
	opos[1] = pgraph_celsius_xfrm_squash(opos[1]);
	opos[2] = pgraph_celsius_xfrm_squash(opos[2]);
	opos[3] = pgraph_celsius_xfrm_squash(opos[3]);
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 4; j++) {
			otxc[i][j] = pgraph_celsius_xfrm_squash(xf.txc[i][j]);
		}
	}
	insrt(opos[3], 0, 1, extr(opos[0], 31, 1));

	if (bypass || extr(state->celsius_xf_misc_b, 14, 1)) {
		state->celsius_pipe_ovtx[idx][0] = otxc[1][0];
		state->celsius_pipe_ovtx[idx][1] = otxc[1][1];
		state->celsius_pipe_ovtx[idx][3] = otxc[1][3];
	}
	if (bypass) {
		insrt(state->celsius_pipe_ovtx[idx][2], 0, 31, optsz);
	}
	insrt(state->celsius_pipe_ovtx[idx][2], 31, 1, state->celsius_pipe_edge_flag);
	if (bypass || extr(state->celsius_xf_misc_b, 0, 1)) {
		state->celsius_pipe_ovtx[idx][4] = otxc[0][0];
		state->celsius_pipe_ovtx[idx][5] = otxc[0][1];
		state->celsius_pipe_ovtx[idx][7] = otxc[0][3];
	}
	if (bypass) {
		state->celsius_pipe_ovtx[idx][9] = ofog;
	}
	insrt(state->celsius_pipe_ovtx[idx][10], 0, 8, ocol[0][0]);
	insrt(state->celsius_pipe_ovtx[idx][10], 8, 8, ocol[0][1]);
	insrt(state->celsius_pipe_ovtx[idx][10], 16, 8, ocol[0][2]);
	insrt(state->celsius_pipe_ovtx[idx][10], 24, 8, ocol[0][3]);
	insrt(state->celsius_pipe_ovtx[idx][11], 0, 8, state->celsius_pipe_edge_flag);
	insrt(state->celsius_pipe_ovtx[idx][11], 8, 8, ocol[1][0]);
	insrt(state->celsius_pipe_ovtx[idx][11], 16, 8, ocol[1][1]);
	insrt(state->celsius_pipe_ovtx[idx][11], 24, 8, ocol[1][2]);
	state->celsius_pipe_ovtx[idx][12] = opos[0];
	state->celsius_pipe_ovtx[idx][13] = opos[1];
	state->celsius_pipe_ovtx[idx][14] = opos[2];
	state->celsius_pipe_ovtx[idx][15] = opos[3];
}

void pgraph_celsius_post_xfrm(struct pgraph_state *state, int idx) {
	uint32_t cb = state->celsius_config_b_shadow;
	if (!extr(cb, 5, 1)) {
		insrt(state->celsius_pipe_xvtx[2][11], 8, 24, 0);
	}
	if (nv04_pgraph_is_celsius_class(state)) {
		state->celsius_pipe_xvtx[0][9] = state->celsius_pipe_ovtx[idx][9] & 0xfffff800;
		state->celsius_pipe_xvtx[0][10] = state->celsius_pipe_ovtx[idx][10];
		insrt(state->celsius_pipe_xvtx[0][11], 8, 24, extr(state->celsius_pipe_ovtx[idx][11], 8, 24));
		if (state->chipset.chipset != 0x10) {
			state->celsius_pipe_xvtx[0][0] = state->celsius_pipe_ovtx[idx][0];
			state->celsius_pipe_xvtx[0][1] = state->celsius_pipe_ovtx[idx][1];
			state->celsius_pipe_xvtx[0][3] = state->celsius_pipe_ovtx[idx][3];
			state->celsius_pipe_xvtx[0][6] = state->celsius_pipe_ovtx[idx][2] & 0x001ff000;
			insrt(state->celsius_pipe_xvtx[0][11], 0, 1, 1);
		}
		if (!extr(cb, 7, 1) && extr(cb, 0, 1)) {
			state->celsius_pipe_xvtx[1][10] = state->celsius_pipe_xvtx[2][10];
			insrt(state->celsius_pipe_xvtx[1][11], 8, 24, extr(state->celsius_pipe_xvtx[2][11], 8, 24));
			state->celsius_pipe_xvtx[2][10] = state->celsius_pipe_ovtx[idx][10];
			insrt(state->celsius_pipe_xvtx[2][11], 8, 24, extr(state->celsius_pipe_ovtx[idx][11], 8, 24));
		}
	}
	if (!extr(cb, 9, 1)) {
		state->celsius_pipe_xvtx[0][6] = state->celsius_point_size << 12;
		state->celsius_pipe_xvtx[1][6] = state->celsius_point_size << 12;
		state->celsius_pipe_xvtx[2][6] = state->celsius_point_size << 12;
	}
	if (!extr(cb, 5, 1)) {
		insrt(state->celsius_pipe_xvtx[0][11], 8, 24, 0);
		insrt(state->celsius_pipe_xvtx[1][11], 8, 24, 0);
	}
}
