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

uint32_t pgraph_celsius_convert_light_v(uint32_t val) {
	if ((val & 0x3ffff) < 0x3fe00)
		val += 0x200;
	return val & 0xfffffc00;
}

uint32_t pgraph_celsius_convert_light_sx(uint32_t val) {
	if (!extr(val, 23, 8))
		return 0;
	if (extr(val, 23, 8) == 0xff) {
		if (extr(val, 9, 14))
			return 0x7ffffc00;
		return 0x7f800000;
	}
	if ((val & 0x3ffff) < 0x3fe00)
		val += 0x200;
	return val & 0xfffffc00;
}

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
	if (extr(state->debug[4], 22, 1) && !extr(state->bundle_raster, 30, 1)) {
		if (extr(val, 31, 1))
			return 0;
	}
	return val;
}

uint32_t pgraph_celsius_xfrm_squash_w(struct pgraph_state *state, uint32_t val) {
	int exp = extr(val, 23, 8);
	if (extr(state->debug[4], 22, 1) && !extr(state->bundle_raster, 30, 1)) {
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
	uint32_t fr = extr(val, 10, 13);
	fr |= 1 << 13;
	fr <<= exp - 0x74;
	fr *= 0xff;
	fr >>= 23;
	fr++;
	fr >>= 1;
	if (fr > 0xff)
		fr = 0xff;
	return fr;
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

static const uint8_t pgraph_celsius_xf_rcp_lut[0x40] = {
	0x7f, 0x7d, 0x7b, 0x79, 0x77, 0x75, 0x74, 0x72, 0x70, 0x6f, 0x6d, 0x6c, 0x6b, 0x69, 0x68, 0x67,
	0x65, 0x64, 0x63, 0x62, 0x60, 0x5f, 0x5e, 0x5d, 0x5c, 0x5b, 0x5a, 0x59, 0x58, 0x57, 0x56, 0x55,
	0x54, 0x54, 0x53, 0x52, 0x51, 0x50, 0x4f, 0x4f, 0x4e, 0x4d, 0x4c, 0x4c, 0x4b, 0x4a, 0x4a, 0x49,
	0x48, 0x48, 0x47, 0x46, 0x46, 0x45, 0x45, 0x44, 0x43, 0x43, 0x42, 0x42, 0x41, 0x41, 0x40, 0x40,
};

uint32_t pgraph_celsius_xfrm_rcp_core(uint32_t x) {
	if (x >= 0x800000)
		abort();
	x += 0x800000;
	uint64_t s0 = pgraph_celsius_xf_rcp_lut[x >> 17 & 0x3f];
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
	if (ex == 0) {
		fx = 0;
		sx = 0;
	}
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
	if (ex == 0) {
		fx = 0;
		sx = 0;
	}
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

static const uint8_t pgraph_celsius_xf_rsqrt_lut[] = {
	0x7f, 0x7e, 0x7d, 0x7c, 0x7b, 0x7a, 0x79, 0x79, 0x78, 0x77, 0x76, 0x75, 0x75, 0x74, 0x73, 0x72,
	0x72, 0x71, 0x70, 0x70, 0x6f, 0x6e, 0x6e, 0x6d, 0x6c, 0x6c, 0x6b, 0x6b, 0x6a, 0x69, 0x69, 0x68,
	0x68, 0x67, 0x67, 0x66, 0x66, 0x65, 0x65, 0x64, 0x64, 0x63, 0x63, 0x62, 0x62, 0x61, 0x61, 0x60,
	0x60, 0x60, 0x5f, 0x5f, 0x5e, 0x5e, 0x5e, 0x5d, 0x5d, 0x5c, 0x5c, 0x5c, 0x5b, 0x5b, 0x5b, 0x5a,
	0x5a, 0x59, 0x58, 0x58, 0x57, 0x56, 0x56, 0x55, 0x55, 0x54, 0x53, 0x53, 0x52, 0x52, 0x51, 0x51,
	0x50, 0x50, 0x4f, 0x4f, 0x4e, 0x4e, 0x4d, 0x4d, 0x4c, 0x4c, 0x4c, 0x4b, 0x4b, 0x4a, 0x4a, 0x4a,
	0x49, 0x49, 0x48, 0x48, 0x48, 0x47, 0x47, 0x47, 0x46, 0x46, 0x46, 0x45, 0x45, 0x45, 0x44, 0x44,
	0x44, 0x43, 0x43, 0x43, 0x43, 0x42, 0x42, 0x42, 0x41, 0x41, 0x41, 0x41, 0x40, 0x40, 0x40, 0x40
};

uint32_t pgraph_celsius_xfrm_rsqrt_core(uint32_t x) {
	if (x >= 0x1000000)
		abort();
	uint64_t s0 = pgraph_celsius_xf_rsqrt_lut[x >> 17 & 0x7f];
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

void pgraph_celsius_xfrm_cp(uint32_t dst[4], uint32_t a[4], uint32_t b[4]) {
	uint32_t rap[4] = { a[1], a[2], a[0] };
	uint32_t rbp[4] = { b[2], b[0], b[1] };
	uint32_t ram[4] = { a[2], a[0], a[1] };
	uint32_t rbm[4] = { b[1], b[2], b[0] };
	uint32_t rp[4], rm[4];
	pgraph_celsius_xfrm_vmul(rp, rap, rbp);
	pgraph_celsius_xfrm_vmul(rm, ram, rbm);
	pgraph_celsius_xfrm_vsuba(dst, rp, rm);
}

void pgraph_celsius_xfrm_vnormf(uint32_t dst[4], uint32_t a[4]) {
	uint32_t d = pgraph_celsius_xfrm_dp3(a, a);
	pgraph_celsius_xfrm_vsmr(dst, pgraph_celsius_xfrm_rsqrt(d));
}

void pgraph_celsius_xfrm_vnorm(uint32_t dst[4], uint32_t a[4]) {
	uint32_t rsq[4];
	pgraph_celsius_xfrm_vnormf(rsq, a);
	pgraph_celsius_xfrm_vmul(dst, a, rsq);
}

struct pgraph_celsius_xf_res {
	uint32_t pos[4];
	uint32_t txc[2][4];
	uint32_t fog[4];
	uint32_t ed[4];
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

	// Compute FOG.
	int fog_mode = extr(mode_a, 16, 2);
	switch (fog_mode) {
		case 0:
			res->fog[0] = vab[2*4+3];
			break;
		case 1:
			res->fog[0] = vab[0*4+2];
			break;
		case 2:
			res->fog[0] = pgraph_celsius_xfrm_rcp(res->pos[3]);
			break;
		case 3:
			if (state->chipset.chipset == 0x10) {
				res->fog[0] = pgraph_celsius_xfrm_rcp(res->pos[3]);
			} else {
				res->fog[0] = vab[2*4+3];
			}
			break;
	}

	// Compute PTSZ.
	res->ed[0] = vab[6*4];
}

void pgraph_celsius_xf_full(struct pgraph_celsius_xf_res *res, struct pgraph_state *state) {
	uint32_t *vab = state->celsius_pipe_vtx;
	uint32_t (*xfctx)[4] = state->celsius_pipe_xfrm;
	uint32_t mode_a = state->celsius_xf_misc_a;
	uint32_t mode_b = state->celsius_xf_misc_b;

	bool light = extr(mode_b, 29, 1);
	int lm[8];
	for (int i = 0; i < 8; i++) {
		if (light)
			lm[i] = extr(mode_a, 2 * i, 2);
		else
			lm[i] = 0;
	}

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
			pgraph_celsius_xfrm_vnorm(rnrm, enrm);
		} else {
			pgraph_celsius_xfrm_vmov(rnrm, enrm);
		}
	}

	// Compute R.
	uint32_t rp[4] = { 0 };
	uint32_t epos3[4];
	uint32_t ev[4];
	uint32_t nev[4];
	uint32_t rm[4];
	{
		uint32_t tmp[4], dp[4];
		uint32_t rwe[4];
		pgraph_celsius_xfrm_vsmr(rwe, pgraph_celsius_xfrm_rcp(epos[3]));
		pgraph_celsius_xfrm_vmul(epos3, epos, rwe);
		pgraph_celsius_xfrm_vsuba(ev, xfctx[0x34], epos3);
		ev[3] = pgraph_celsius_xfrm_dp3(ev, ev);
		pgraph_celsius_xfrm_vnorm(nev, ev);

		pgraph_celsius_xfrm_vmul(tmp, xfctx[0x35], rnrm);
		uint32_t d = pgraph_celsius_xfrm_dp3(nev, tmp);
		pgraph_celsius_xfrm_vsmr(dp, d);
		pgraph_celsius_xfrm_vmul(tmp, rnrm, dp);
		pgraph_celsius_xfrm_vadda(rp, tmp, xfctx[0x36]);
		pgraph_celsius_xfrm_vsuba(rp, rp, nev);
		pgraph_celsius_xfrm_vsuba(rm, rp, xfctx[0x36]);
	}
	uint32_t sm[4];
	{
		uint32_t rs[4];
		pgraph_celsius_xfrm_vnormf(rs, rp);
		uint32_t tmp[4];
		pgraph_celsius_xfrm_vmul(tmp, rp, xfctx[0x37]);
		pgraph_celsius_xfrm_vmul(tmp, tmp, rs);
		pgraph_celsius_xfrm_vadda(sm, tmp, xfctx[0x37]);
	}

	// Compute TXC.
	uint32_t *itxc[2] = {&vab[3*4], &vab[4*4]};
	for (int i = 0; i < 2; i++) {
		uint32_t txconf = extr(mode_b, i * 14, 14);
		bool div_en = extr(txconf, 2, 1);
		bool mat_en = extr(txconf, 1, 1);
		if (!extr(txconf, 0, 1))
			continue;
		if (!div_en && !mat_en) {
			// Without enabled perspective, result is non-deterministic.
			abort();
		}
		uint32_t ptxc[4];
		uint32_t mtxc[4];
		bool emboss = i == 1 && extr(txconf, 3, 3) == 6;
		uint32_t edp[4] = {0};
		if (emboss) {
			uint32_t b[4], bn[4], t[4], l[4], ct[4], bnf[4];
			pgraph_celsius_xfrm_mmul3(ct, itxc[1], &xfctx[4]);
			pgraph_celsius_xfrm_cp(b, rnrm, ct);
			pgraph_celsius_xfrm_vnormf(bnf, b);
			pgraph_celsius_xfrm_vmul(bn, b, bnf);
			pgraph_celsius_xfrm_cp(t, b, rnrm);
			pgraph_celsius_xfrm_vmul(t, t, bnf);
			if (lm[0] == 0 || lm[0] == 1) {
				pgraph_celsius_xfrm_vmov(l, xfctx[0x24]);
			} else {
				pgraph_celsius_xfrm_vsuba(l, xfctx[0x24], epos3);
				pgraph_celsius_xfrm_vnorm(l, l);
			}
			uint32_t tmp[4];
			pgraph_celsius_xfrm_vmul(tmp, xfctx[0x1c], l);
			edp[0] = pgraph_celsius_xfrm_dp3(tmp, bn);
			pgraph_celsius_xfrm_vmul(tmp, xfctx[0x1d], l);
			edp[1] = pgraph_celsius_xfrm_dp3(tmp, t);
			uint32_t df[4];
			pgraph_celsius_xfrm_vsmr(df, itxc[0][3]);
			pgraph_celsius_xfrm_vmul(df, df, edp);
			pgraph_celsius_xfrm_vadda(ptxc, itxc[0], df);
		} else {
			pgraph_celsius_xfrm_vmov(ptxc, itxc[i]);
		}
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
					if (emboss)
						ptxc[j] = edp[j];
					else
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
					if (emboss)
						ptxc[j] = itxc[0][3];
					else
						ptxc[j] = rm[j];
					break;
			}
		}
		if (mat_en) {
			pgraph_celsius_xfrm_mmul(mtxc, ptxc, &xfctx[0x18 + i * 8]);
			if (state->chipset.chipset == 0x10) {
				uint32_t ftxc[4];
				pgraph_celsius_xfrm_vmov(ftxc, ptxc);
				ftxc[0] = mtxc[0];
				mtxc[3] = pgraph_celsius_xfrm_dp4(ftxc, xfctx[0x1b + i * 8]);
			}
		} else {
			pgraph_celsius_xfrm_vmov(mtxc, ptxc);
		}
		if (div_en)
			pgraph_celsius_xfrm_vmul(res->txc[i], mtxc, rw);
		else
			pgraph_celsius_xfrm_vmov(res->txc[i], mtxc);
	}

	// Compute ED.
	res->ed[0] = 0x3f800000;
	res->ed[1] = nev[3];
	res->ed[2] = ev[3];

	// Compute FOG.
	int fog_mode = extr(mode_a, 16, 2);
	switch (fog_mode) {
		case 0:
			res->fog[0] = xfctx[0x36][2];
			res->fog[1] = vab[2*4+3];
			res->fog[2] = pgraph_celsius_xfrm_mul(res->fog[1], res->fog[1]);
			break;
		case 1:
			res->fog[0] = res->ed[0];
			res->fog[1] = res->ed[1];
			res->fog[2] = res->ed[2];
			break;
		case 2:
		case 3:
			res->fog[0] = xfctx[0x36][2];
			res->fog[1] = pgraph_celsius_xfrm_dp4(epos3, xfctx[0x38]);
			res->fog[2] = pgraph_celsius_xfrm_mul(res->fog[1], res->fog[1]);
			if (fog_mode == 3) {
				res->fog[0] &= 0x7fffffff;
				res->fog[1] &= 0x7fffffff;
				res->fog[2] &= 0x7fffffff;
			}
			break;
	}

	// XXX Compute things for light.
}

uint32_t pgraph_celsius_lt_mul(uint32_t a, uint32_t b) {
	bool sign = extr(a, 31, 1) ^ extr(b, 31, 1);
	int expa = extr(a, 23, 8);
	int expb = extr(b, 23, 8);
	uint32_t fra = extr(a, 10, 13) | 1 << 13;
	uint32_t frb = extr(b, 10, 13) | 1 << 13;
	if ((expa == 0xff && fra > 0x2000) || (expb == 0xff && frb > 0x2000))
		return 0x7ffffc00;
	if (!expa || !expb)
		return 0;
	if (expa == 0xff || expb == 0xff)
		return 0x7f800000;
	int exp = expa + expb - 0x7f;
	uint32_t fr = fra * frb;
	fr >>= 13;
	if (fr > 0x3fff) {
		fr >>= 1;
		exp++;
	}
	if (exp <= 0 || expa == 0 || expb == 0) {
		exp = 0;
		fr = 0;
	} else if (exp >= 0xff) {
		exp = 0xfe;
		fr = 0x1fff;
	}
	uint32_t res = sign << 31 | exp << 23 | (fr & 0x1fff) << 10;
	return res;
}

uint32_t pgraph_celsius_lt_add3(uint32_t *v) {
	for (int i = 0; i < 3; i++) {
		if (FP32_ISNAN(v[i]))
			return FP32_CNAN & ~0x3ff;
	}
	bool pinf = false;
	bool ninf = false;
	for (int i = 0; i < 3; i++) {
		if (FP32_ISINF(v[i])) {
			if (FP32_SIGN(v[i]))
				ninf = true;
			else
				pinf = true;
		}
	}
	if (pinf && ninf)
		return FP32_CNAN & ~0x3ff;
	else if (pinf || ninf)
		return FP32_INF(0);
	/* Only honest real numbers involved. */
	bool sv[3], sr;
	int ev[3], er = 0;
	uint32_t fv[3];
	for (int i = 0; i < 3; i++) {
		sv[i] = FP32_SIGN(v[i]);
		ev[i] = FP32_EXP(v[i]);
		fv[i] = FP32_FRACT(v[i]);
		if (ev[i])
			fv[i] |= FP32_IONE;
		fv[i] >>= 10;
		if (ev[i] + 2 > er)
			er = ev[i] + 2;
	}
	int32_t res = 0;
	for (int i = 0; i < 3; i++) {
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
		res = norm32(res, &er, 20);
		/* Round it. */
		res = shr32(res, 7, FP_RZ);
	}
	if (er >= 0xff) {
		er = 0xfe;
		res = 0x3fff;
	}
	uint32_t r = fp32_mkfin(sr, er, res << 10, FP_RZ, true);
	return r;
}

uint32_t pgraph_celsius_lt_add(uint32_t a, uint32_t b) {
	uint32_t v[3] = {a, b, 0};
	return pgraph_celsius_lt_add3(v);
}

uint32_t pgraph_celsius_lts_mul(uint32_t a, uint32_t b) {
	bool sign = extr(a, 31, 1) ^ extr(b, 31, 1);
	int expa = extr(a, 23, 8);
	int expb = extr(b, 23, 8);
	uint32_t fra = extr(a, 10, 13) | 1 << 13;
	uint32_t frb = extr(b, 10, 13) | 1 << 13;
	if ((expa == 0xff && fra > 0x2000) || (expb == 0xff && frb > 0x2000))
		return 0x7ffffc00;
	if (!expa || !expb)
		return 0;
	if (expa == 0xff || expb == 0xff)
		return FP32_INF(sign);
	int exp = expa + expb - 0x7f;
	if (exp >= 0xff) {
		return FP32_INF(sign);
	}
	uint32_t fr = fra * frb;
	fr >>= 13;
	if (fr > 0x3fff) {
		fr >>= 1;
		exp++;
	}
	if (exp <= 0 || expa == 0 || expb == 0) {
		exp = 0;
		fr = 0;
	} else if (exp >= 0xff) {
		exp = 0xfe;
		fr = 0x1fff;
	}
	uint32_t res = sign << 31 | exp << 23 | (fr & 0x1fff) << 10;
	return res;
}

uint32_t pgraph_celsius_lts_add(uint32_t a, uint32_t b) {
	if (FP32_ISNAN(a) || FP32_ISNAN(b))
		return FP32_CNAN & ~0x3ff;
	bool sa, sb, sr;
	int ea, eb, er;
	uint32_t fa, fb;
	sa = FP32_SIGN(a);
	ea = FP32_EXP(a);
	fa = FP32_FRACT(a) >> 10;
	sb = FP32_SIGN(b);
	eb = FP32_EXP(b);
	fb = FP32_FRACT(b) >> 10;
	if (FP32_ISINF(a) || FP32_ISINF(b)) {
		if (FP32_ISINF(a) && FP32_ISINF(b) && sa != sb)
			return FP32_CNAN & ~0x3ff;
		else
			return FP32_INF(0);
	}
	/* Only honest real numbers involved. */
	if (ea != 0)
		fa |= FP32_IONE >> 10;
	else
		fa = 0;
	if (eb != 0)
		fb |= FP32_IONE >> 10;
	else
		fb = 0;
	er = max(ea, eb) + 1;
	int32_t res = 0;
	fa = shr32(fa, er - ea - 1, FP_RZ);
	fb = shr32(fb, er - eb - 1, FP_RZ);
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
		res = norm32(res, &er, 14);
		res = shr32(res, 1, FP_RZ);
	}
	uint32_t r = fp32_mkfin(sr, er, res << 10, FP_RZ, true);
	return r;
}

uint32_t pgraph_celsius_lt_rcp(uint32_t x) {
	if (FP32_ISNAN(x))
		return FP32_CNAN & ~0x3ff;
	bool sx = FP32_SIGN(x);
	int ex = FP32_EXP(x);
	uint32_t fx = FP32_FRACT(x);
	if (!ex || sx)
		return FP32_INF(0);
	if (FP32_ISINF(x))
		return 0;
	int er = 0xfd - ex;
	if (fx >= 0x800000)
		abort();
	fx += 0x800000;
	fx >>= 10;
	uint64_t s0 = pgraph_celsius_xf_rcp_lut[fx >> 7 & 0x3f];
	uint64_t s1 = ((1u << 21) - s0 * fx) * s0 >> 14;
	s1 <<= 11;
	s1 -= 0x800000;
	if (s1 >= 0x800000)
		abort();
	uint32_t fr = s1;
	if (er <= 0) {
		er = 0;
		fr = 0;
	}
	return sx << 31 | er << 23 | fr;
}

uint32_t pgraph_celsius_lt_rsqrt(uint32_t x) {
	if (FP32_ISNAN(x))
		return FP32_CNAN & ~0x3ff;
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
		fr = pgraph_celsius_xf_rsqrt_lut[extr(fx, 17, 6)] << 17;
	} else {
		er = 0x7f - (ex - 0x80) / 2 - 1;
		fr = pgraph_celsius_xf_rsqrt_lut[extr(fx, 17, 6) + 0x40] << 17;
	}
	return er << 23 | (fr & 0x7ffc00);
}

void pgraph_celsius_lt_vmov(uint32_t dst[3], uint32_t a[3]) {
	for (int i = 0; i < 3; i++) {
		dst[i] = a[i];
	}
}

void pgraph_celsius_lt_vmul(uint32_t dst[3], uint32_t a[3], uint32_t b[3]) {
	for (int i = 0; i < 3; i++) {
		dst[i] = pgraph_celsius_lt_mul(a[i], b[i]);
	}
}

void pgraph_celsius_lt_vadd(uint32_t dst[3], uint32_t a[3], uint32_t b[3]) {
	for (int i = 0; i < 3; i++) {
		dst[i] = pgraph_celsius_lt_add(a[i], b[i]);
	}
}

uint32_t pgraph_celsius_lt_dp(uint32_t a[3], uint32_t b[3]) {
	uint32_t tmp[3];
	pgraph_celsius_lt_vmul(tmp, a, b);
	return pgraph_celsius_lt_add3(tmp);
}

struct pgraph_celsius_lt_in {
	uint32_t fog[3];
	uint32_t ed[3];
	uint32_t col0[3];
	uint32_t col1[3];
};

struct pgraph_celsius_lt_res {
	uint32_t fog;
	uint32_t ptsz;
	uint32_t col0[3];
	uint32_t col1[3];
	uint32_t alpha;
};

void pgraph_celsius_lt_bypass(struct pgraph_celsius_lt_res *res, struct pgraph_celsius_lt_in *in, struct pgraph_state *state) {
	uint32_t mode_a = state->celsius_xf_misc_a;
	uint32_t (*ltctx)[3] = state->celsius_pipe_light_v;

	// Compute FOG.
	int fog_mode = extr(mode_a, 16, 2);
	uint32_t afog;
	if (fog_mode == 0 || (fog_mode == 3 && state->chipset.chipset != 0x10))
		afog = pgraph_celsius_lt_add3(ltctx[0x28]);
	else
		afog = pgraph_celsius_lt_add3(ltctx[0x29]);
	uint32_t mfog = pgraph_celsius_lt_mul(ltctx[0x2b][0], in->fog[0]);
	res->fog = pgraph_celsius_lt_add(mfog, afog);

	// Compute PTSZ.
	res->ptsz = in->ed[0];
}

void pgraph_celsius_lt_full(struct pgraph_celsius_lt_res *res, struct pgraph_celsius_lt_in *in, struct pgraph_state *state) {
	uint32_t (*ltctx)[3] = state->celsius_pipe_light_v;
	uint32_t *ltc0 = state->celsius_pipe_light_sa;
	uint32_t *ltc1 = state->celsius_pipe_light_sb;
	uint32_t *ltc2 = state->celsius_pipe_light_sc;
	uint32_t *ltc3 = state->celsius_pipe_light_sd;
	uint32_t mode_a = state->celsius_xf_misc_a;
	uint32_t mode_b = state->celsius_xf_misc_b;
	bool spec_out = extr(mode_a, 19, 1);
	bool spec_in = extr(mode_a, 20, 1);
	bool lm_e = extr(mode_a, 21, 1);
	bool lm_a = extr(mode_a, 22, 1);
	bool lm_d = extr(mode_a, 23, 1);
	bool lm_s = extr(mode_a, 24, 1);
	bool light = extr(mode_b, 29, 1);

	// Compute FOG.
	res->fog = pgraph_celsius_lt_dp(ltctx[0x2b], in->fog);

	// Compute PTSZ.
	uint32_t pd = pgraph_celsius_lt_dp(ltctx[0x2d], in->ed);
	pd = pgraph_celsius_lt_rsqrt(pd);
	pd = pgraph_celsius_lts_add(pd, ltc1[2]);
	if (pd & 0x80000000)
		pd = 0;
	if (pd > 0x3f800000)
		pd = 0x3f800000;
	pd = pgraph_celsius_lt_mul(pd, ltctx[0x2e][0]);
	uint32_t pdo = pgraph_celsius_lts_add(ltctx[0x2c][0], ltc3[1]);
	res->ptsz = pgraph_celsius_lt_add(pd, pdo);

	// Compute colors.
	if (!light) {
		pgraph_celsius_lt_vmov(res->col0, in->col0);
		if (spec_out && spec_in) {
			pgraph_celsius_lt_vmov(res->col1, in->col1);
		} else {
			pgraph_celsius_lt_vmov(res->col1, ltctx[0x2c]);
		}
	} else {
		if (lm_a) {
			pgraph_celsius_lt_vmul(res->col0, in->col0, ltctx[0x29]);
			pgraph_celsius_lt_vadd(res->col0, res->col0, ltctx[0x2a]);
		} else {
			pgraph_celsius_lt_vmov(res->col0, ltctx[0x29]);
			if (lm_e) {
				pgraph_celsius_lt_vadd(res->col0, res->col0, in->col0);
			}
		}

		bool was_off = false;
		for (int i = 0; i < 8; i++) {
			int mode = extr(mode_a, i * 2, 2);
			if (mode) {
				if (was_off)
					abort();
				// XXX compute lights
				abort();
			} else {
				was_off = true;
			}
		}

		pgraph_celsius_lt_vmov(res->col1, ltctx[0x2c]);
		res->alpha = ltc3[0xb];
	}
}

void pgraph_celsius_xfrm(struct pgraph_state *state, int idx) {
	uint32_t mode_a = state->celsius_xf_misc_a;
	uint32_t mode_b = state->celsius_xf_misc_b;
	bool bypass = extr(mode_a, 28, 1);
	bool light = extr(mode_b, 29, 1);
	bool lm_d = extr(mode_a, 23, 1);

	uint32_t opos[4];
	uint32_t otxc[2][4];
	uint32_t icol[2][4];
	uint32_t optsz;
	uint32_t ofog;
	uint32_t ocol[2][4];
	icol[0][0] = state->celsius_pipe_vtx[1*4+0];
	icol[0][1] = state->celsius_pipe_vtx[1*4+1];
	icol[0][2] = state->celsius_pipe_vtx[1*4+2];
	icol[0][3] = state->celsius_pipe_vtx[1*4+3];
	icol[1][0] = state->celsius_pipe_vtx[2*4+0];
	icol[1][1] = state->celsius_pipe_vtx[2*4+1];
	icol[1][2] = state->celsius_pipe_vtx[2*4+2];

	struct pgraph_celsius_xf_res xf;
	struct pgraph_celsius_lt_res lt;
	struct pgraph_celsius_lt_in lti;
	if (bypass) {
		pgraph_celsius_xf_bypass(&xf, state);
		lti.fog[0] = pgraph_celsius_convert_light_v(xf.fog[0]);
		lti.ed[0] = pgraph_celsius_convert_light_v(xf.ed[0]);
		pgraph_celsius_lt_bypass(&lt, &lti, state);
		ocol[0][0] = icol[0][0];
		ocol[0][1] = icol[0][1];
		ocol[0][2] = icol[0][2];
		ocol[1][0] = icol[1][0];
		ocol[1][1] = icol[1][1];
		ocol[1][2] = icol[1][2];
		ocol[0][3] = icol[0][3];
	} else {
		pgraph_celsius_xf_full(&xf, state);
		lti.fog[0] = pgraph_celsius_convert_light_v(xf.fog[0]);
		lti.fog[1] = pgraph_celsius_convert_light_v(xf.fog[1]);
		lti.fog[2] = pgraph_celsius_convert_light_v(xf.fog[2]);
		lti.ed[0] = pgraph_celsius_convert_light_v(xf.ed[0]);
		lti.ed[1] = pgraph_celsius_convert_light_v(xf.ed[1]);
		lti.ed[2] = pgraph_celsius_convert_light_v(xf.ed[2]);
		lti.col0[0] = pgraph_celsius_convert_light_v(icol[0][0]);
		lti.col0[1] = pgraph_celsius_convert_light_v(icol[0][1]);
		lti.col0[2] = pgraph_celsius_convert_light_v(icol[0][2]);
		lti.col1[0] = pgraph_celsius_convert_light_v(icol[1][0]);
		lti.col1[1] = pgraph_celsius_convert_light_v(icol[1][1]);
		lti.col1[2] = pgraph_celsius_convert_light_v(icol[1][2]);
		pgraph_celsius_lt_full(&lt, &lti, state);
		ocol[0][0] = lt.col0[0];
		ocol[0][1] = lt.col0[1];
		ocol[0][2] = lt.col0[2];
		ocol[1][0] = lt.col1[0];
		ocol[1][1] = lt.col1[1];
		ocol[1][2] = lt.col1[2];
		if (light && !lm_d) {
			ocol[0][3] = lt.alpha;
		} else {
			ocol[0][3] = icol[0][3];
		}
	}
	ofog = lt.fog;

	// Convert COL.
	ocol[0][0] = pgraph_celsius_xfrm_f2b(ocol[0][0]);
	ocol[0][1] = pgraph_celsius_xfrm_f2b(ocol[0][1]);
	ocol[0][2] = pgraph_celsius_xfrm_f2b(ocol[0][2]);
	ocol[0][3] = pgraph_celsius_xfrm_f2b(ocol[0][3]);
	ocol[1][0] = pgraph_celsius_xfrm_f2b(ocol[1][0]);
	ocol[1][1] = pgraph_celsius_xfrm_f2b(ocol[1][1]);
	ocol[1][2] = pgraph_celsius_xfrm_f2b(ocol[1][2]);

	// Convert FOG.
	insrt(ofog, 10, 1, extr(ofog, 11, 1));

	// Convert PTSZ.
	optsz = lt.ptsz & 0x001ff000;

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
	if (bypass || extr(mode_a, 25, 1)) {
		insrt(state->celsius_pipe_ovtx[idx][2], 0, 31, optsz);
	}
	insrt(state->celsius_pipe_ovtx[idx][2], 31, 1, state->celsius_pipe_edge_flag);
	if (bypass || extr(state->celsius_xf_misc_b, 0, 1)) {
		state->celsius_pipe_ovtx[idx][4] = otxc[0][0];
		state->celsius_pipe_ovtx[idx][5] = otxc[0][1];
		state->celsius_pipe_ovtx[idx][7] = otxc[0][3];
	}
	if (bypass || extr(mode_b, 31, 1)) {
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
	uint32_t cb = state->shadow_config_b;
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
		state->celsius_pipe_xvtx[0][6] = state->bundle_point_size << 12;
		state->celsius_pipe_xvtx[1][6] = state->bundle_point_size << 12;
		state->celsius_pipe_xvtx[2][6] = state->bundle_point_size << 12;
	}
	if (!extr(cb, 5, 1)) {
		insrt(state->celsius_pipe_xvtx[0][11], 8, 24, 0);
		insrt(state->celsius_pipe_xvtx[1][11], 8, 24, 0);
	}
}
