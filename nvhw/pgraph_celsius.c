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
