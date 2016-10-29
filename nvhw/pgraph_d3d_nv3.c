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
#include "util.h"

bool nv03_pgraph_d3d_cmp(int func, uint32_t a, uint32_t b) {
	switch (func) {
	case 1:
		return false;
	case 2:
		return a < b;
	case 3:
		return a == b;
	case 0: /* alias */
	case 4:
		return a <= b;
	case 5:
		return a > b;
	case 6:
		return a != b;
	case 7:
		return a >= b;
	case 8:
	default:
		return true;
	}
}

bool nv03_pgraph_d3d_wren(int func, bool zeta_test, bool alpha_test) {
	switch (func) {
	case 0:
	default:
		return false;
	case 1:
		return alpha_test;
	case 2:
		return alpha_test && zeta_test;
	case 3:
		return zeta_test;
	case 4:
		return true;
	}
}

uint32_t nv03_pgraph_d3d_blend(uint32_t factor, uint32_t dst, uint32_t src) {
	if (factor >= 0xf)
		return src;
	if (!factor)
		return dst;
	return (dst * (0x10 - factor) + src * factor) >> 4;
}

uint16_t nv03_pgraph_zpoint_rop(struct nv03_pgraph_state *state, int32_t x, int32_t y, uint16_t pixel) {
	uint32_t src = state->misc32_0;
	uint32_t sa, sr, sg, sb;
	uint32_t srgb;
	bool dither = extr(state->debug[3], 15, 1);
	nv03_pgraph_expand_color(state->ctx_switch, src, &srgb, &sa);
	sr = extr(srgb, 20, 10);
	sg = extr(srgb, 10, 10);
	sb = extr(srgb, 0, 10);
	uint32_t dr, dg, db;
	dr = extr(pixel, 10, 5);
	dg = extr(pixel, 5, 5);
	db = extr(pixel, 0, 5);
	if (dither) {
		sr = nv01_pgraph_dither_10to5(sr, x, y, false);
		sg = nv01_pgraph_dither_10to5(sg, x, y, true);
		sb = nv01_pgraph_dither_10to5(sb, x, y, false);
	} else {
		sr >>= 5;
		sg >>= 5;
		sb >>= 5;
	}
	if (!extr(state->d3d_config, 28, 1)) {
		uint32_t br, bg, bb;
		if (extr(state->d3d_config, 29, 1)) {
			br = dr;
			bg = dg;
			bb = db;
		} else {
			br = bg = bb = nv01_pgraph_dither_10to5(sa << 2, x, y, false);
		}
		if (extr(state->d3d_config, 31, 1))
			sr = sg = sb = 0;
		if (extr(state->d3d_config, 30, 1))
			dr = dg = db = 0;
		sr = nv03_pgraph_d3d_blend(br >> 1, dr, sr);
		sg = nv03_pgraph_d3d_blend(bg >> 1, dg, sg);
		sb = nv03_pgraph_d3d_blend(bb >> 1, db, sb);
	} else {
		sr += dr;
		sg += dg;
		sb += db;
		if (sr > 0x1f)
			sr = 0x1f;
		if (sg > 0x1f)
			sg = 0x1f;
		if (sb > 0x1f)
			sb = 0x1f;
	}
	return sr << 10 | sg << 5 | sb;
}
