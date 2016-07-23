/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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
#include <stdlib.h>

int pgraph_type(int chipset) {
	if (chipset <3)
		return PGRAPH_NV01;
	if (chipset < 4)
		return PGRAPH_NV03;
	if (chipset < 0x10)
		return PGRAPH_NV04;
	if (chipset < 0x20)
		return PGRAPH_NV10;
	if (chipset < 0x40)
		return PGRAPH_NV20;
	if (chipset != 0x50 && chipset < 0x80)
		return PGRAPH_NV40;
	if (chipset < 0xc0)
		return PGRAPH_NV50;
	return PGRAPH_NVC0;
}

void nv01_pgraph_expand_color(uint32_t ctx, uint32_t config, uint32_t color, uint32_t *rgb, uint32_t *alpha) {
	int format = ctx >> 9 & 0xf;
	int fa = ctx >> 13 & 1;
	int a, r, g, b;
	int replicate = config >> 20 & 1;
	int factor;
	switch (format % 5) {
		case 0: /* (X16)A1R5G5B5 */
			factor = replicate ? 0x21 : 0x20;
			a = extr(color, 15, 1) * 0xff;
			r = extr(color, 10, 5) * factor;
			g = extr(color, 5, 5) * factor;
			b = extr(color, 0, 5) * factor;
			break;
		case 1: /* A8R8G8B8 */
			factor = replicate ? 0x101 : 0x100;
			a = extr(color, 24, 8);
			r = extr(color, 16, 8) * factor >> 6;
			g = extr(color, 8, 8) * factor >> 6;
			b = extr(color, 0, 8) * factor >> 6;
			break;
		case 2: /* A2R10G10B10 */
			a = extr(color, 30, 2) * 0x55;
			r = extr(color, 20, 10);
			g = extr(color, 10, 10);
			b = extr(color, 0, 10);
			break;
		case 3: /* (X16)A8Y8 */
			factor = replicate ? 0x101 : 0x100;
			a = extr(color, 8, 8);
			r = g = b = extr(color, 0, 8) * factor >> 6;
			break;
		case 4: /* A16Y16 */
			a = extr(color, 16, 16) >> 8;
			r = g = b = extr(color, 0, 16) >> 6;
			break;
		default:
			abort();
			break;
	}
	if (!fa)
		a = 0xff;
	*rgb = r << 20 | g << 10 | b;
	*alpha = a;
}

uint32_t nv01_pgraph_expand_a1r10g10b10(uint32_t ctx, uint32_t config, uint32_t color) {
	uint32_t rgb; uint32_t alpha;
	nv01_pgraph_expand_color(ctx, config, color, &rgb, &alpha);
	if (alpha)
		rgb |= 0x40000000;
	return rgb;
}

uint32_t nv01_pgraph_expand_mono(uint32_t ctx, uint32_t mono) {
	uint32_t res = 0;
	if (ctx & 0x4000) {
		int i;
		for (i = 0; i < 32; i++)
			insrt(res, i^7, 1, extr(mono, i, 1));
	} else {
		res = mono;
	}
	return res;
}

int nv01_pgraph_cpp(uint32_t pfb_config) {
	switch (extr(pfb_config, 8, 2)) {
		case 0:
		case 1:
			return 1;
		case 2:
			return 2;
		case 3:
			return 4;
		default:
			abort();
	}
}

int nv01_pgraph_cpp_in(uint32_t ctx_switch) {
	int src_format = extr(ctx_switch, 9, 4) % 5;
	switch (src_format) {
		case 0: /* (X16)A1R5G5B5 */
			return 2;
		case 1: /* A8R8G8B8 */
			return 4;
		case 2: /* A2R10G10B10 */
			return 4;
		case 3: /* (X16A8)Y8 */
			return 1;
		case 4: /* A16Y16 */
			return 4;
		default:
			abort();
	}
}

int nv01_pgraph_width(uint32_t pfb_config) {
	switch (extr(pfb_config, 4, 3)) {
		case 0:
			return 576;
		case 1:
			return 640;
		case 2:
			return 800;
		case 3:
			return 1024;
		case 4:
			return 1152;
		case 5:
			return 1280;
		case 6:
			return 1600;
		case 7:
			return 1856;
		default:
			abort();
	}
}

uint32_t nv01_pgraph_pixel_addr(struct nv01_pgraph_state *state, int x, int y, int buf) {
	uint32_t addr = (y * nv01_pgraph_width(state->pfb_config) + x) * nv01_pgraph_cpp(state->pfb_config);
	uint32_t memsize = 1 << (20 + extr(state->pfb_boot, 0, 2));
	if (extr(state->pfb_config, 12, 1)) {
		addr &= memsize/2 - 1;
		buf &= 1;
		addr += buf * memsize/2;
	} else {
		addr &= memsize - 1;
	}
	return addr;
}

int nv01_pgraph_dither_10to5(int val, int x, int y, int isg) {
	int step = val>>2&7;
	static const int tab1[4][4] = {
		{ 0, 1, 1, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 1, 1 },
		{ 1, 1, 1, 1 },
	};
	int w = (x^y) >> 1 & 1;;
	int z = tab1[y>>2&3][x>>2&3] ^ isg;
	if (step & 1)
		z ^= w;
	int tx = x&1;
	int ty = y&1;
	int d;
	switch (step) {
		case 0:
			d = 0;
			break;
		case 1:
			d = !tx && !ty && z;
			break;
		case 2:
			d = !(tx^ty) && (tx^z);
			break;
		case 3:
			d = !(tx^ty) && (!tx || z);
			break;
		case 4:
			d = !(tx^ty);
			break;
		case 5:
			d = !(tx^ty) || (tx && !ty && z);
			break;
		case 6:
			d = !(tx^ty) || (ty^z);
			break;
		case 7:
			d = tx || !ty || z;
			break;
		default:
			d = 0;
			break;
	}
	val += d << 5;
	if (val > 0x3ff)
		val = 0x3ff;
	return val >> 5;
}

void nv01_pgraph_pattern_pixel(struct nv01_pgraph_state *state, int x, int y, uint32_t *ta, uint32_t *tr, uint32_t *tg, uint32_t *tb, uint32_t *ti) {
	int bidx;
	switch (state->pattern_shape) {
		case 0:
			bidx = (x & 7) | (y & 7) << 3;
			break;
		case 1:
			bidx = x & 0x3f;
			break;
		case 2:
			bidx = y & 0x3f;
			break;
		case 3:
			bidx = (y & 0x3f) | (x & 0x3c);
			break;
		default:
			abort();
	}
	int bit = state->pattern_bitmap[bidx >> 5] >> (bidx & 0x1f) & 1;
	*ta = state->pattern_a[bit];
	*tr = state->pattern_rgb[bit] >> 20 & 0x3ff;
	*tg = state->pattern_rgb[bit] >> 10 & 0x3ff;
	*tb = state->pattern_rgb[bit] >> 00 & 0x3ff;
	*ti = state->pattern_rgb[bit] >> 2 & 0xff;
}

uint8_t nv01_pgraph_xlat_rop(struct nv01_pgraph_state *state) {
	uint8_t res = 0;
	int op = extr(state->ctx_switch, 0, 5);
	int swizzle[3];
	if (op < 8) {
		swizzle[0] = op >> 0 & 1;
		swizzle[1] = op >> 1 & 1;
		swizzle[2] = op >> 2 & 1;
	} else if (op < 0x10) {
		swizzle[0] = (op >> 0 & 1)+1;
		swizzle[1] = (op >> 1 & 1)+1;
		swizzle[2] = (op >> 2 & 1)+1;
	} else if (op == 0x10) {
		swizzle[0] = 0, swizzle[1] = 1, swizzle[2] = 2;
	} else if (op == 0x11) {
		swizzle[0] = 1, swizzle[1] = 0, swizzle[2] = 2;
	} else if (op == 0x12) {
		swizzle[0] = 0, swizzle[1] = 2, swizzle[2] = 1;
	} else if (op == 0x13) {
		swizzle[0] = 2, swizzle[1] = 0, swizzle[2] = 1;
	} else if (op == 0x14) {
		swizzle[0] = 1, swizzle[1] = 2, swizzle[2] = 0;
	} else if (op == 0x15) {
		swizzle[0] = 2, swizzle[1] = 1, swizzle[2] = 0;
	} else {
		abort();
	}
	if (op == 0) {
		if (state->rop & 0x01)
			res |= 0x11;
		if (state->rop & 0x16)
			res |= 0x44;
		if (state->rop & 0x68)
			res |= 0x22;
		if (state->rop & 0x80)
			res |= 0x88;
	} else if (op == 0xf) {
		if (state->rop & 0x01)
			res |= 0x03;
		if (state->rop & 0x16)
			res |= 0x0c;
		if (state->rop & 0x68)
			res |= 0x30;
		if (state->rop & 0x80)
			res |= 0xc0;
	} else {
		int i;
		for (i = 0; i < 8; i++) {
			int s0 = i >> swizzle[0] & 1;
			int s1 = i >> swizzle[1] & 1;
			int s2 = i >> swizzle[2] & 1;
			int s = s2 << 2 | s1 << 1 | s0;
			if (state->rop >> s & 1)
				res |= 1 << i;
		}
	}
	return res;
}

uint32_t nv01_pgraph_do_rop(uint8_t rop, uint32_t dst, uint32_t src, uint32_t pat) {
	int i;
	uint32_t res = 0;
	for (i = 0; i < 10; i++) {
		int d = dst >> i & 1;
		int s = src >> i & 1;
		int p = pat >> i & 1;
		int bit = p << 2 | s << 1 | d;
		int r = rop >> bit & 1;
		res |= r << i;
	}
	return res;
}

uint32_t nv01_pgraph_blend_factor_square(uint32_t alpha) {
	if (alpha == 0xff)
		return alpha;
	return (alpha >> 4) * (alpha >> 4);
}

uint32_t nv01_pgraph_blend_factor(uint32_t alpha, uint32_t beta) {
	if (beta == 0xff)
		return alpha;
	if (alpha == 0xff)
		return beta;
	return ((alpha >> 4) * beta) >> 4;
}

uint32_t nv01_pgraph_do_blend(uint32_t factor, uint32_t dst, uint32_t src, int is_r5g5b5) {
	if (factor == 0xff)
		return src;
	if (!factor)
		return dst;
	if (is_r5g5b5) {
		src &= 0x3e0;
		dst &= 0x3e0;
	}
	dst >>= 2;
	src >>= 2;
	return (dst * (0xff - factor) + src * factor) >> 6;
}

uint32_t nv01_pgraph_rop(struct nv01_pgraph_state *state, int x, int y, uint32_t pixel) {
	int src_format = extr(state->ctx_switch, 9, 4) % 5;
	int cpp = nv01_pgraph_cpp(state->pfb_config);
	int op = extr(state->ctx_switch, 0, 5);
	int blend_en = op >= 0x18;
	int mode_idx = (cpp == 1 || (src_format == 3 && !extr(state->canvas_config, 12, 1))) && !blend_en;
	int bypass = extr(state->canvas_config, 0, 1);
	int dither = extr(state->canvas_config, 16, 1) && (src_format != 0 || blend_en) && cpp == 2;
	uint32_t rgb, sa;
	nv01_pgraph_expand_color(state->ctx_switch, state->canvas_config, state->source_color, &rgb, &sa);
	if (!sa)
		return pixel;
	if (extr(state->canvas_config, 24, 1))
		return pixel;
	uint32_t sr = rgb >> 20 & 0x3ff;
	uint32_t sg = rgb >> 10 & 0x3ff;
	uint32_t sb = rgb >> 00 & 0x3ff;
	uint32_t si = state->source_color & 0xff;
	uint32_t dr = 0, dg = 0, db = 0, di = 0;
	di = pixel & 0xff;
	if (cpp == 2) {
		int factor = extr(state->canvas_config, 20, 1) ? 0x21 : 0x20;
		dr = (pixel >> 10 & 0x1f) * factor;
		dg = (pixel >> 5 & 0x1f) * factor;
		db = (pixel >> 0 & 0x1f) * factor;
	} else {
		dr = pixel >> 20 & 0x3ff;
		dg = pixel >> 10 & 0x3ff;
		db = pixel >> 00 & 0x3ff;
	}
	int alpha_en = extr(state->debug[0], 28, 1);
	uint32_t ta, tr, tg, tb, ti;
	nv01_pgraph_pattern_pixel(state, x, y, &ta, &tr, &tg, &tb, &ti);
	if (!blend_en) {
		if (op < 0x16) {
			int worop = extr(state->debug[0], 20, 1);
			if (op >= 9 && !ta)
				return pixel;
			uint8_t rop = nv01_pgraph_xlat_rop(state);
			if (rop == 0xaa && worop && !(state->ctx_switch & 0x40))
				return pixel;
			sr = nv01_pgraph_do_rop(rop, dr, sr, tr) & 0x3ff;
			sg = nv01_pgraph_do_rop(rop, dg, sg, tg) & 0x3ff;
			sb = nv01_pgraph_do_rop(rop, db, sb, tb) & 0x3ff;
			si = nv01_pgraph_do_rop(rop, di, si, ti) & 0xff;
		}
		if (extr(state->ctx_switch, 5, 1)) {
			uint32_t ca = state->chroma >> 30 & 1;
			uint32_t cr = state->chroma >> 20 & 0x3ff;
			uint32_t cg = state->chroma >> 10 & 0x3ff;
			uint32_t cb = state->chroma >> 00 & 0x3ff;
			uint32_t ci = state->chroma >> 02 & 0xff;
			if (mode_idx) {
				if (ci == si && ca) {
					return pixel;
				}
			} else if (cpp == 2 && src_format == 0) {
				if ((cr >> 5) == (sr >> 5) && (cg >> 5) == (sg >> 5) && (cb >> 5) == (sb >> 5) && ca) {
					return pixel;
				}
			} else {
				if (cr == sr && cg == sg && cb == sb && ca) {
					return pixel;
				}
			}
		}
		if (extr(state->ctx_switch, 6, 1)) {
			uint32_t planemask = state->plane;
			uint32_t pa = planemask >> 30 & 1;
			uint32_t pr = planemask >> 20 & 0x3ff;
			uint32_t pg = planemask >> 10 & 0x3ff;
			uint32_t pb = planemask >> 00 & 0x3ff;
			uint32_t pi = planemask >> 02 & 0xff;
			if (!pa && alpha_en)
				return pixel;
			si = (pi & si) | (~pi & di);
			sr = (pr & sr) | (~pr & dr);
			sg = (pg & sg) | (~pg & dg);
			sb = (pb & sb) | (~pb & db);
		}
	} else {
		int is_r5g5b5 = cpp == 2 && !dither;
		uint32_t beta = state->beta >> 23;
		uint8_t factor;
		if (op >= 0x1b)
			sa = 0xff;
		if (op == 0x18) {
			factor = nv01_pgraph_blend_factor_square(sa);
		} else if (op == 0x19 || op == 0x1b) {
			if (!beta && op == 0x19)
				return pixel;
			factor = nv01_pgraph_blend_factor(sa, beta);
		} else if (op == 0x1a || op == 0x1c) {
			if (beta == 0xff && op == 0x1a)
				return pixel;
			factor = nv01_pgraph_blend_factor(sa, 0xff-beta);
		} else {
			abort();
		}
		if (cpp == 2 && src_format == 0) {
			sr &= 0x3e0;
			sg &= 0x3e0;
			sb &= 0x3e0;
			tr &= 0x3e0;
			tg &= 0x3e0;
			tb &= 0x3e0;
			dr &= 0x3e0;
			dg &= 0x3e0;
			db &= 0x3e0;
		}
		if (op < 0x1b) {
			sr = nv01_pgraph_do_blend(factor, dr, sr, is_r5g5b5);
			sg = nv01_pgraph_do_blend(factor, dg, sg, is_r5g5b5);
			sb = nv01_pgraph_do_blend(factor, db, sb, is_r5g5b5);
		} else {
			if (!ta)
				return pixel;
			sr = nv01_pgraph_do_blend(factor, tr, sr, is_r5g5b5);
			sg = nv01_pgraph_do_blend(factor, tg, sg, is_r5g5b5);
			sb = nv01_pgraph_do_blend(factor, tb, sb, is_r5g5b5);
		}
	}
	switch (cpp) {
		case 1:
			return si;
		case 2:
			if (mode_idx)
				return bypass << 15 | si;
			if (dither) {
				sr = nv01_pgraph_dither_10to5(sr, x, y, 0);
				sg = nv01_pgraph_dither_10to5(sg, x, y, 1);
				sb = nv01_pgraph_dither_10to5(sb, x, y, 0);
			} else {
				sr >>= 5;
				sg >>= 5;
				sb >>= 5;
			}
			return bypass << 15 | sr << 10 | sg << 5 | sb;
		case 4:
			if (mode_idx)
				return bypass << 31 | si;
			return bypass << 31 | sr << 20 | sg << 10 | sb;
		default:
			abort();
	}
}
