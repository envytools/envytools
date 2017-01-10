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

uint32_t pgraph_celsius_ub_to_float(uint8_t val) {
	if (!val)
		return 0;
	if (val == 0xff)
		return 0x3f800000;
	uint32_t res = val * 0x010101;
	uint32_t exp = 0x7e;
	while (!extr(res, 23, 1))
		res <<= 1, exp--;
	insrt(res, 23, 8, exp);
	return res;
}

uint32_t pgraph_celsius_short_to_float(struct pgraph_state *state, int16_t val) {
	if (!val)
		return 0;
	if (state->chipset.chipset == 0x10 && val == -0x8000)
		return 0x80000000;
	bool sign = val < 0;
	uint32_t res = (sign ? -val : val) << 8;
	uint32_t exp = 0x7f + 15;
	while (!extr(res, 23, 1))
		res <<= 1, exp--;
	insrt(res, 23, 8, exp);
	insrt(res, 31, 1, sign);
	return res;
}

uint32_t pgraph_celsius_nshort_to_float(int16_t val) {
	bool sign = val < 0;
	if (val == -0x8000)
		return 0xbf800000;
	if (val == 0x7fff)
		return 0x3f800000;
	int32_t rv = val << 1 | 1;
	uint32_t res = (sign ? -rv : rv) * 0x10001;
	uint32_t exp = 0x7f - 9;
	while (extr(res, 24, 8))
		res >>= 1, exp++;
	while (!extr(res, 23, 1))
		res <<= 1, exp--;
	insrt(res, 23, 8, exp);
	insrt(res, 31, 1, sign);
	return res;
}

void pgraph_celsius_pre_icmd(struct pgraph_state *state) {
	uint32_t cb = state->celsius_config_b_shadow;
	if (!extr(cb, 9, 1)) {
		state->celsius_pipe_xvtx[0][6] = state->celsius_point_size << 12;
		state->celsius_pipe_xvtx[1][6] = state->celsius_point_size << 12;
		state->celsius_pipe_xvtx[2][6] = state->celsius_point_size << 12;
	}
}

void pgraph_celsius_icmd(struct pgraph_state *state, int cmd, uint32_t val, bool last) {
	int vs = extr(state->celsius_pipe_vtx_state, 28, 3);
	int pt = state->celsius_pipe_begin_end;
	uint32_t cb = state->celsius_config_b_shadow;
	if (!extr(cb, 7, 1)) {
		if (!extr(cb, 0, 1)) {
			if (pt == 3 && vs != 0 && vs != 4) {
				state->celsius_pipe_xvtx[1][10] = state->celsius_pipe_xvtx[0][10];
				state->celsius_pipe_xvtx[2][10] = state->celsius_pipe_xvtx[0][10];
				insrt(state->celsius_pipe_xvtx[1][11], 8, 24, extr(state->celsius_pipe_xvtx[0][11], 8, 24));
				insrt(state->celsius_pipe_xvtx[2][11], 8, 24, extr(state->celsius_pipe_xvtx[0][11], 8, 24));
			}
		} else {
			if (pt == 3 && vs != 0 && vs != 4) {
				int ctr = state->celsius_pipe_ovtx_pos;
				state->celsius_pipe_xvtx[0][10] = state->celsius_pipe_xvtx[2][10];
				state->celsius_pipe_xvtx[1][10] = state->celsius_pipe_xvtx[2][10];
				insrt(state->celsius_pipe_xvtx[0][11], 8, 24, extr(state->celsius_pipe_xvtx[2][11], 8, 24));
				insrt(state->celsius_pipe_xvtx[1][11], 8, 24, extr(state->celsius_pipe_xvtx[2][11], 8, 24));
				state->celsius_pipe_xvtx[2][10] = state->celsius_pipe_ovtx[ctr][10];
				insrt(state->celsius_pipe_xvtx[2][11], 8, 24, extr(state->celsius_pipe_ovtx[ctr][11], 8, 24));
			}
			if (pt == 0xa && vs != 0 && vs != 4 && vs != 7) {
				int ctr = state->celsius_pipe_ovtx_pos;
				state->celsius_pipe_xvtx[1][10] = state->celsius_pipe_xvtx[2][10];
				insrt(state->celsius_pipe_xvtx[1][11], 8, 24, extr(state->celsius_pipe_xvtx[2][11], 8, 24));
				state->celsius_pipe_xvtx[2][10] = state->celsius_pipe_ovtx[ctr][10];
				insrt(state->celsius_pipe_xvtx[2][11], 8, 24, extr(state->celsius_pipe_ovtx[ctr][11], 8, 24));
			}
		}
	}
	uint32_t cls = extr(state->ctx_switch[0], 0, 8);
	uint32_t cc = state->celsius_config_c;
	if (cls == 0x55 || cls == 0x95) {
		if (cmd == 0x11 || cmd == 0x13) {
			bool color = cmd == 0x13;
			if (extr(cc, 20 + color, 1) && !extr(cc, 18 + color, 1)) {
				for (int i = 0; i < 4; i++)
					if (extr(val, i * 8, 4) == 0xc)
						val ^= 1 << (i * 8 + 5);
			}
		}
		if (cmd == 7)
			insrt(val, 30, 1, extr(cc, 16, 4) != 0xf);
		if (cmd == 0x1b) {
			if (extr(cc, 23, 1) || (extr(cc, 19, 1) && extr(cc, 21, 1)))
				insrt(val, 5, 1, 1);
			if (extr(cc, 22, 1) || (extr(cc, 18, 1) && extr(cc, 20, 1)))
				insrt(val, 13, 1, 1);
		}
	}
	pgraph_celsius_raw_icmd(state, cmd, val, last);
}

void pgraph_celsius_raw_icmd(struct pgraph_state *state, int cmd, uint32_t val, bool last) {
	state->celsius_pipe_junk[0] = cmd << 2;
	state->celsius_pipe_junk[1] = val;
	insrt(state->celsius_pipe_vtx_state, 28, 3, 0);
	if (state->chipset.chipset == 0x10) {
		int prev = state->celsius_pipe_prev_ovtx_pos;
		state->celsius_pipe_xvtx[0][0] = state->celsius_pipe_ovtx[prev][0];
		state->celsius_pipe_xvtx[0][1] = state->celsius_pipe_ovtx[prev][1];
		state->celsius_pipe_xvtx[0][3] = state->celsius_pipe_ovtx[prev][3];
		insrt(state->celsius_pipe_xvtx[0][11], 0, 1, extr(state->celsius_pipe_ovtx[prev][2], 31, 1));
		if (extr(state->celsius_config_b_shadow, 9, 1)) {
			state->celsius_pipe_xvtx[0][6] = state->celsius_pipe_ovtx[prev][2] & 0x001ff000;
		}
	}
	int ctr = state->celsius_pipe_ovtx_pos;
	state->celsius_pipe_ovtx[ctr][2] = val;
	insrt(state->celsius_pipe_ovtx[ctr][9], 0, 6, cmd);
	insrt(state->celsius_pipe_ovtx[ctr][11], 0, 1, extr(val, 31, 1));
	state->celsius_pipe_xvtx[0][4] = cmd << 2;
	state->celsius_pipe_xvtx[0][5] = val;
	state->celsius_pipe_xvtx[0][7] = state->celsius_pipe_ovtx[ctr][3];
	state->celsius_pipe_xvtx[0][12] = val;
	state->celsius_pipe_xvtx[0][13] = cmd;
	uint32_t fract = extr(state->celsius_pipe_ovtx[ctr][9], 11, 12);
	if (extr(state->celsius_pipe_ovtx[ctr][9], 23, 8))
		fract |= 0x1000;
	if (extr(state->celsius_pipe_ovtx[ctr][9], 31, 1))
		fract = -fract;
	insrt(state->celsius_pipe_xvtx[0][13], 10, 14, fract);
	insrt(state->celsius_pipe_xvtx[0][13], 24, 8, extr(state->celsius_pipe_ovtx[ctr][9], 23, 8));
	state->celsius_pipe_xvtx[0][14] = state->celsius_pipe_ovtx[ctr][10];
	if (extr(state->celsius_config_b_shadow, 6, 1) && last)
		state->celsius_pipe_xvtx[0][15] = state->celsius_pipe_ovtx[ctr][11] & ~1;
	state->celsius_pipe_prev_ovtx_pos = state->celsius_pipe_ovtx_pos;
	state->celsius_pipe_ovtx_pos++;
	state->celsius_pipe_ovtx_pos &= 0xf;
	if (cmd == 0x1f) {
		if (!extr(state->celsius_config_b_shadow, 7, 1) && extr(state->celsius_config_b_shadow, 0, 1) && (extr(val, 7, 1) || !extr(val, 0, 1))) {
			state->celsius_pipe_xvtx[1][10] = state->celsius_pipe_xvtx[0][10];
			insrt(state->celsius_pipe_xvtx[1][11], 8, 24, extr(state->celsius_pipe_xvtx[0][11], 8, 24));
			state->celsius_pipe_xvtx[2][10] = state->celsius_pipe_xvtx[0][10];
			insrt(state->celsius_pipe_xvtx[2][11], 8, 24, extr(state->celsius_pipe_xvtx[0][11], 8, 24));
		}
		state->celsius_config_b_shadow = val;
	}
}

uint32_t pgraph_celsius_fixup_vtxbuf_format(struct pgraph_state *state, int idx, uint32_t val) {
	uint32_t res = 0;
	insrt(res, 10, 6, extr(val, 10, 6));
	int comp = extr(val, 4, 3);
	int fmt = extr(val, 0, 3);
	bool w = extr(val, 24, 1);
	switch (idx) {
		case 0:
			if (comp > 4)
				comp &= 3;
			if (comp == 0)
				comp = 4;
			break;
		case 1:
		case 2:
			if (comp < 2)
				comp = 0;
			else if (comp < 4)
				comp = 3;
			else
				comp = 4;
			break;
		case 3:
		case 4:
			if (comp > 4)
				comp &= 3;
			break;
		case 5:
			if ((comp & 3) == 3)
				comp = 3;
			else
				comp = 0;
			break;
		case 6:
		case 7:
			if (comp & 1)
				comp = 1;
			else
				comp = 0;
			break;
	}
	if (idx == 1 || idx == 2) {
		if (fmt == 2 || fmt == 3 || fmt == 7)
			fmt = 6;
		if (fmt == 1)
			fmt = 0;
		if (fmt == 5)
			fmt = 4;
	} else {
		if (fmt & 2)
			fmt = 6;
		else
			fmt = 5;
	}
	insrt(res, 0, 3, fmt);
	insrt(res, 4, 3, comp);
	insrt(res, 24, 1, w);
	return res;
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
	if (FP32_ISNAN(a) || FP32_ISNAN(b))
		return FP32_CNAN;
	if (FP32_ISINF(a) || FP32_ISINF(b))
		return FP32_INF(0);
	/* Two honest real numbers involved. */
	bool sa, sb, sr;
	int ea, eb, er;
	uint32_t fa, fb;
	sa = FP32_SIGN(a);
	ea = FP32_EXP(a);
	fa = FP32_FRACT(a);
	sb = FP32_SIGN(b);
	eb = FP32_EXP(b);
	fb = FP32_FRACT(b);
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

