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

void pgraph_reset(struct pgraph_state *state) {
	if (state->chipset.card_type < 3) {
		state->valid[0] = 0;
		state->edgefill &= 0xffff0000;
		state->xy_misc_0 &= 0x1000;
		state->xy_misc_1[0] &= 0x03000000;
		state->xy_misc_4[0] &= 0xff000000;
		state->xy_misc_4[0] |= 0x00555500;
		state->xy_misc_4[1] &= 0xff000000;
		state->xy_misc_4[1] |= 0x00555500;
	} else {
		state->bitmap_color[0] &= 0x3fffffff;
		state->dma_intr_en = 0;
		state->xy_misc_0 &= 0x100000;
		state->xy_misc_1[0] &= 0x0f000000;
		state->xy_misc_1[1] &= 0x0f000000;
		state->xy_misc_3 &= ~0x00001100;
		state->xy_misc_4[0] &= 0x30000000;
		state->xy_misc_4[1] &= 0x30000000;
		state->surf_offset[0] = 0;
		state->surf_offset[1] = 0;
		state->surf_offset[2] = 0;
		state->surf_offset[3] = 0;
		state->d3d_config = 0;
		state->misc24[0] = 0;
		state->misc24[1] = 0;
		state->valid[0] = 0;
		state->xy_clip[0][0] = 0x55555555;
		state->xy_clip[0][1] = 0x55555555;
		state->xy_clip[1][0] = 0x55555555;
		state->xy_clip[1][1] = 0x55555555;
	}
}

void pgraph_volatile_reset(struct pgraph_state *state) {
	state->xy_misc_0 = 0;
	if (state->chipset.card_type < 3) {
		state->bitmap_color[0] &= 0x3fffffff;
		state->bitmap_color[1] &= 0x3fffffff;
		state->valid[0] &= 0x11000000;
		state->xy_misc_1[0] &= 0x33300;
		state->xy_misc_4[0] = 0x555500;
		state->xy_misc_4[1] = 0x555500;
		state->misc32[0] &= 0x00ff00ff;
		state->subdivide &= 0xffff0000;
	} else {
		if (state->chipset.card_type < 4) {
			state->bitmap_color[0] &= 0x3fffffff;
			state->xy_misc_1[0] &= ~0x001440ff;
			state->xy_misc_1[1] &= ~0x001440fe;
			state->d3d_alpha = 0x800;
			state->misc32[0] &= 0x00ff00ff;
		} else {
			state->xy_misc_1[0] = 0;
			state->xy_misc_1[1] &= 1;
			state->valid[1] &= 0xc0000000;
			state->misc32[0] = 0;
			state->dvd_format = 0;
			state->notify &= ~0x10000;
		}
		state->xy_misc_3 &= ~0x00000011;
		state->xy_misc_4[0] = 0;
		state->xy_misc_4[1] = 0;
		state->valid[0] &= 0xf0000000;
		state->xy_clip[0][0] = 0x55555555;
		state->xy_clip[0][1] = 0x55555555;
		state->xy_clip[1][0] = 0x55555555;
		state->xy_clip[1][1] = 0x55555555;
	}
	if (state->chipset.card_type >= 0x10) {
		state->valid[0] &= 0x50000000;
		state->oclip_min[0] = 0;
		state->oclip_min[1] = 0;
		state->oclip_max[0] = 0xffff;
		state->oclip_max[1] = 0xffff;
	}
}

uint32_t pgraph_to_a1r10g10b10(struct pgraph_color color) {
	return !!color.a << 30 | color.r << 20 | color.g << 10 | color.b;
}

struct pgraph_color pgraph_expand_color(struct pgraph_state *state, uint32_t color) {
	if (state->chipset.card_type < 4) {
		int format;
		bool fa;
		bool replicate;
		if (state->chipset.card_type < 3) {
			format = extr(state->ctx_switch[0], 9, 4) % 5;
			fa = extr(state->ctx_switch[0], 13, 1);
			replicate = extr(state->canvas_config, 20, 1);
		} else {
			format = extr(state->ctx_switch[0], 0, 3);
			fa = extr(state->ctx_switch[0], 3, 1);
			replicate = false;
		}
		int a, r, g, b;
		int factor;
		int mode;
		switch (format) {
			case 0: /* (X16)A1R5G5B5 */
				factor = replicate ? 0x21 : 0x20;
				a = extr(color, 15, 1) * 0xff;
				r = extr(color, 10, 5) * factor;
				g = extr(color, 5, 5) * factor;
				b = extr(color, 0, 5) * factor;
				mode = COLOR_MODE_RGB5;
				break;
			default:
			case 1: /* A8R8G8B8 */
				factor = replicate ? 0x101 : 0x100;
				a = extr(color, 24, 8);
				r = extr(color, 16, 8) * factor >> 6;
				g = extr(color, 8, 8) * factor >> 6;
				b = extr(color, 0, 8) * factor >> 6;
				mode = COLOR_MODE_RGB8;
				break;
			case 2: /* A2R10G10B10 */
				a = extr(color, 30, 2) * 0x55;
				r = extr(color, 20, 10);
				g = extr(color, 10, 10);
				b = extr(color, 0, 10);
				mode = COLOR_MODE_RGB10;
				break;
			case 3: /* (X16)A8Y8 */
				factor = replicate ? 0x101 : 0x100;
				a = extr(color, 8, 8);
				r = g = b = extr(color, 0, 8) * factor >> 6;
				mode = COLOR_MODE_Y8;
				break;
			case 4: /* A16Y16 */
				a = extr(color, 16, 16) >> 8;
				r = g = b = extr(color, 0, 16) >> 6;
				mode = COLOR_MODE_Y16;
				break;
		}
		uint8_t i = extr(color, 0, 8);
		uint16_t i16 = extr(color, 0, 16);
		if (!fa)
			a = 0xff;
		return (struct pgraph_color){r, g, b, a, i, i16, mode};
	}
	// XXX
	return (struct pgraph_color){};
}

struct pgraph_color nv01_pgraph_expand_surf(struct pgraph_state *state, uint32_t pixel) {
	int fmt = extr(state->pfb_config, 8, 2);
	struct pgraph_color res;
	res.i16 = pixel & 0xffff;
	res.i = pixel & 0xff;
	res.a = 0xff;
	int factor = extr(state->canvas_config, 20, 1) ? 0x21 : 0x20;
	switch (fmt) {
		case 0:
		case 1:
			res.r = res.g = res.b = 0;
			res.mode = COLOR_MODE_Y8;
			break;
		case 2:
			res.mode = COLOR_MODE_RGB5;
			res.r = extr(pixel, 10, 5) * factor;
			res.g = extr(pixel, 5, 5) * factor;
			res.b = extr(pixel, 0, 5) * factor;
			break;
		case 3:
			res.mode = COLOR_MODE_RGB10;
			res.r = extr(pixel, 20, 10);
			res.g = extr(pixel, 10, 10);
			res.b = extr(pixel, 0, 10);
			break;
		default:
			abort();
	}
	return res;
}

struct pgraph_color nv03_pgraph_expand_surf(int fmt, uint32_t pixel) {
	struct pgraph_color res;
	res.i16 = pixel & 0xffff;
	res.i = pixel & 0xff;
	res.a = 0xff;
	switch (fmt) {
		case 1:
			res.r = res.g = res.b = 0;
			res.mode = COLOR_MODE_Y8;
			break;
		case 0:
			res.mode = COLOR_MODE_Y16;
			res.r = extr(pixel, 10, 5) << 5;
			res.g = extr(pixel, 5, 5) << 5;
			res.b = extr(pixel, 0, 5) << 5;
			break;
		case 2:
			res.mode = COLOR_MODE_RGB5;
			res.r = extr(pixel, 10, 5) << 5;
			res.g = extr(pixel, 5, 5) << 5;
			res.b = extr(pixel, 0, 5) << 5;
			break;
		case 3:
			res.mode = COLOR_MODE_RGB10;
			res.r = extr(pixel, 16, 8) << 2 | extr(pixel, 28, 2);
			res.g = extr(pixel, 8, 8) << 2 | extr(pixel, 26, 2);
			res.b = extr(pixel, 0, 8) << 2 | extr(pixel, 24, 2);
			break;
		default:
			abort();
	}
	return res;
}

uint32_t pgraph_expand_mono(struct pgraph_state *state, uint32_t mono) {
	uint32_t res = mono;
	bool swap = false;
	if (state->chipset.card_type < 3) {
		swap = extr(state->ctx_switch[0], 14, 1);
	} else if (state->chipset.card_type < 4) {
		swap = extr(state->ctx_switch[0], 8, 1);
	} else {
		swap = extr(state->ctx_switch[1], 0, 2) == 1;
	}
	if (swap) {
		for (int i = 0; i < 0x20; i++)
			insrt(res, i ^ 7, 1, extr(mono, i, 1));
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

uint32_t nv01_pgraph_pixel_addr(struct pgraph_state *state, int x, int y, int buf) {
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

int nv01_pgraph_dither_10to5(int val, int x, int y, bool isg) {
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

struct pgraph_color nv01_pgraph_pattern_pixel(struct pgraph_state *state, int x, int y) {
	int bidx;
	switch (extr(state->pattern_config, 0, 2)) {
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
	int bit = state->pattern_mono_bitmap[bidx >> 5] >> (bidx & 0x1f) & 1;
	return (struct pgraph_color){
		state->pattern_mono_rgb[bit] >> 20 & 0x3ff,
		state->pattern_mono_rgb[bit] >> 10 & 0x3ff,
		state->pattern_mono_rgb[bit] >> 00 & 0x3ff,
		state->pattern_mono_a[bit],
		state->pattern_mono_rgb[bit] >> 2 & 0xff,
		0,
		COLOR_MODE_RGB10,
	};
}

struct pgraph_color nv03_pgraph_pattern_pixel(struct pgraph_state *state, int x, int y) {
	int bidx;
	switch (extr(state->pattern_config, 0, 2)) {
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
	int bit = state->pattern_mono_bitmap[bidx >> 5] >> (bidx & 0x1f) & 1;
	return (struct pgraph_color){
		extr(state->pattern_mono_rgb[bit], 20, 10),
		extr(state->pattern_mono_rgb[bit], 10, 10),
		extr(state->pattern_mono_rgb[bit], 0, 10),
		state->pattern_mono_a[bit],
		extr(state->pattern_mono_rgb[bit], 2, 8),
		extr(state->pattern_mono_rgb[bit], 5, 5) |
		extr(state->pattern_mono_rgb[bit], 15, 5) << 5 |
		extr(state->pattern_mono_rgb[bit], 25, 5) << 10,
		COLOR_MODE_RGB10,
	};
}

uint8_t nv01_pgraph_xlat_rop(int op, uint8_t rop) {
	uint8_t res = 0;
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
		if (rop & 0x01)
			res |= 0x11;
		if (rop & 0x16)
			res |= 0x44;
		if (rop & 0x68)
			res |= 0x22;
		if (rop & 0x80)
			res |= 0x88;
	} else if (op == 0xf) {
		if (rop & 0x01)
			res |= 0x03;
		if (rop & 0x16)
			res |= 0x0c;
		if (rop & 0x68)
			res |= 0x30;
		if (rop & 0x80)
			res |= 0xc0;
	} else {
		int i;
		for (i = 0; i < 8; i++) {
			int s0 = i >> swizzle[0] & 1;
			int s1 = i >> swizzle[1] & 1;
			int s2 = i >> swizzle[2] & 1;
			int s = s2 << 2 | s1 << 1 | s0;
			if (rop >> s & 1)
				res |= 1 << i;
		}
	}
	return res;
}

uint32_t nv01_pgraph_do_rop(uint8_t rop, uint32_t dst, uint32_t src, uint32_t pat) {
	int i;
	uint32_t res = 0;
	for (i = 0; i < 16; i++) {
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

uint32_t nv01_pgraph_solid_rop(struct pgraph_state *state, int x, int y, uint32_t pixel) {
	struct pgraph_color s = pgraph_expand_color(state, state->misc32[0]);
	return nv01_pgraph_rop(state, x, y, pixel, s);
}

uint32_t nv01_pgraph_rop(struct pgraph_state *state, int x, int y, uint32_t pixel, struct pgraph_color s) {
	int cpp = nv01_pgraph_cpp(state->pfb_config);
	int op = extr(state->ctx_switch[0], 0, 5);
	int blend_en = op >= 0x18;
	int mode_idx = (cpp == 1 || (s.mode == COLOR_MODE_Y8 && !extr(state->canvas_config, 12, 1))) && !blend_en;
	int bypass = extr(state->canvas_config, 0, 1);
	bool dither = extr(state->canvas_config, 16, 1) && (s.mode != COLOR_MODE_RGB5 || blend_en) && cpp == 2;
	if (!s.a)
		return pixel;
	if (extr(state->canvas_config, 24, 1))
		return pixel;
	struct pgraph_color d = nv01_pgraph_expand_surf(state, pixel);
	int alpha_en = extr(state->debug[0], 28, 1);
	struct pgraph_color p = nv01_pgraph_pattern_pixel(state, x, y);
	if (!blend_en) {
		if (op < 0x16) {
			int worop = extr(state->debug[0], 20, 1);
			if (op >= 9 && !p.a)
				return pixel;
			uint8_t rop = nv01_pgraph_xlat_rop(op, state->rop);
			if (rop == 0xaa && worop && !(state->ctx_switch[0] & 0x40))
				return pixel;
			s.r = nv01_pgraph_do_rop(rop, d.r, s.r, p.r) & 0x3ff;
			s.g = nv01_pgraph_do_rop(rop, d.g, s.g, p.g) & 0x3ff;
			s.b = nv01_pgraph_do_rop(rop, d.b, s.b, p.b) & 0x3ff;
			s.i = nv01_pgraph_do_rop(rop, d.i, s.i, p.i) & 0xff;
		}
		if (extr(state->ctx_switch[0], 5, 1)) {
			uint32_t ca = state->chroma >> 30 & 1;
			uint32_t cr = state->chroma >> 20 & 0x3ff;
			uint32_t cg = state->chroma >> 10 & 0x3ff;
			uint32_t cb = state->chroma >> 00 & 0x3ff;
			uint32_t ci = state->chroma >> 02 & 0xff;
			if (mode_idx) {
				if (ci == s.i && ca) {
					return pixel;
				}
			} else if (cpp == 2 && s.mode == COLOR_MODE_RGB5) {
				if ((cr >> 5) == (s.r >> 5) && (cg >> 5) == (s.g >> 5) && (cb >> 5) == (s.b >> 5) && ca) {
					return pixel;
				}
			} else {
				if (cr == s.r && cg == s.g && cb == s.b && ca) {
					return pixel;
				}
			}
		}
		if (extr(state->ctx_switch[0], 6, 1)) {
			uint32_t planemask = state->plane;
			uint32_t pa = planemask >> 30 & 1;
			uint32_t pr = planemask >> 20 & 0x3ff;
			uint32_t pg = planemask >> 10 & 0x3ff;
			uint32_t pb = planemask >> 00 & 0x3ff;
			uint32_t pi = planemask >> 02 & 0xff;
			if (!pa && alpha_en)
				return pixel;
			s.i = (pi & s.i) | (~pi & d.i);
			s.r = (pr & s.r) | (~pr & d.r);
			s.g = (pg & s.g) | (~pg & d.g);
			s.b = (pb & s.b) | (~pb & d.b);
		}
	} else {
		int is_r5g5b5 = cpp == 2 && !dither;
		uint32_t beta = state->beta >> 23;
		uint8_t factor;
		if (op >= 0x1b)
			s.a = 0xff;
		if (op == 0x18) {
			factor = nv01_pgraph_blend_factor_square(s.a);
		} else if (op == 0x19 || op == 0x1b) {
			if (!beta && op == 0x19)
				return pixel;
			factor = nv01_pgraph_blend_factor(s.a, beta);
		} else if (op == 0x1a || op == 0x1c) {
			if (beta == 0xff && op == 0x1a)
				return pixel;
			factor = nv01_pgraph_blend_factor(s.a, 0xff-beta);
		} else {
			abort();
		}
		if (cpp == 2 && s.mode == COLOR_MODE_RGB5) {
			s.r &= 0x3e0;
			s.g &= 0x3e0;
			s.b &= 0x3e0;
			p.r &= 0x3e0;
			p.g &= 0x3e0;
			p.b &= 0x3e0;
			d.r &= 0x3e0;
			d.g &= 0x3e0;
			d.b &= 0x3e0;
		}
		if (op < 0x1b) {
			s.r = nv01_pgraph_do_blend(factor, d.r, s.r, is_r5g5b5);
			s.g = nv01_pgraph_do_blend(factor, d.g, s.g, is_r5g5b5);
			s.b = nv01_pgraph_do_blend(factor, d.b, s.b, is_r5g5b5);
		} else {
			if (!p.a)
				return pixel;
			s.r = nv01_pgraph_do_blend(factor, p.r, s.r, is_r5g5b5);
			s.g = nv01_pgraph_do_blend(factor, p.g, s.g, is_r5g5b5);
			s.b = nv01_pgraph_do_blend(factor, p.b, s.b, is_r5g5b5);
		}
	}
	switch (cpp) {
		case 1:
			return s.i;
		case 2:
			if (mode_idx)
				return bypass << 15 | s.i;
			if (dither) {
				s.r = nv01_pgraph_dither_10to5(s.r, x, y, false);
				s.g = nv01_pgraph_dither_10to5(s.g, x, y, true);
				s.b = nv01_pgraph_dither_10to5(s.b, x, y, false);
			} else {
				s.r >>= 5;
				s.g >>= 5;
				s.b >>= 5;
			}
			return bypass << 15 | s.r << 10 | s.g << 5 | s.b;
		case 4:
			if (mode_idx)
				return bypass << 31 | s.i;
			return bypass << 31 | s.r << 20 | s.g << 10 | s.b;
		default:
			abort();
	}
}

int nv03_pgraph_surf_format(struct pgraph_state *state) {
	int i;
	for (i = 0; i < 4; i++)
		if (extr(state->ctx_switch[0], 20+i, 1))
			return extr(state->surf_format, 4*i, 4);
	return extr(state->surf_format, 12, 4);
}

int nv03_pgraph_cpp(struct pgraph_state *state) {
	switch (nv03_pgraph_surf_format(state) & 3) {
		case 1:
			return 1;
		case 0:
		case 2:
			return 2;
		case 3:
			return 4;
		default:
			abort();
	}
}

uint32_t nv03_pgraph_blend_factor(uint32_t alpha, uint32_t beta) {
	if (beta == 0xff)
		return alpha;
	if (alpha == 0xff)
		return beta;
	alpha >>= 4;
	beta >>= 3;
	return (alpha * beta) >> 1;
}

uint32_t nv03_pgraph_do_blend(uint32_t factor, uint32_t dst, uint32_t src, int is_r5g5b5) {
	factor >>= 3;
	if (factor == 0x1f)
		return src;
	if (!factor)
		return dst;
	src >>= 2;
	dst >>= 2;
	if (is_r5g5b5) {
		src &= 0xf8;
		dst &= 0xf8;
	}
	return (dst * (0x20 - factor) + src * factor) >> 3;
}

uint32_t nv03_pgraph_solid_rop(struct pgraph_state *state, int x, int y, uint32_t pixel) {
	return nv03_pgraph_rop(state, x, y, pixel, pgraph_expand_color(state, state->misc32[0]));
}

uint32_t nv03_pgraph_rop(struct pgraph_state *state, int x, int y, uint32_t pixel, struct pgraph_color s) {
	bool bd = extr(state->ctx_switch[0], 9, 1);
	int op = extr(state->ctx_switch[0], 24, 5);
	bool blend_en = op > 0x17;
	int fmt = nv03_pgraph_surf_format(state) & 3;
	bool is_rgb5 = s.mode == COLOR_MODE_RGB5;
	bool dither = (!is_rgb5 || blend_en) && fmt != 3;
	if (!s.a)
		return pixel;
	struct pgraph_color d = nv03_pgraph_expand_surf(fmt, pixel);
	struct pgraph_color p = nv03_pgraph_pattern_pixel(state, x, y);
	if (blend_en) {
		int is_r5g5b5 = fmt != 3 && !dither;
		uint32_t beta = state->beta >> 23;
		uint8_t factor;
		if (op >= 0x1b && op != 0x1d)
			s.a = 0xff;
		if (op == 0x1d) {
			factor = s.a;
		} else if (op == 0x19) {
			if (!beta && op == 0x19)
				return pixel;
			factor = nv03_pgraph_blend_factor(s.a, beta);
		} else if (op == 0x1a) {
			if (beta == 0xff && op == 0x1a)
				return pixel;
			factor = nv03_pgraph_blend_factor(s.a, 0xff-beta);
		} else {
			abort();
		}
		insrt(s.r, 0, 2, extr(s.r, 8, 2));
		insrt(s.g, 0, 2, extr(s.g, 8, 2));
		insrt(s.b, 0, 2, extr(s.b, 8, 2));
		if (fmt != 3 && is_rgb5) {
			s.r &= 0x3e0;
			s.g &= 0x3e0;
			s.b &= 0x3e0;
			p.r &= 0x3e0;
			p.g &= 0x3e0;
			p.b &= 0x3e0;
			d.r &= 0x3e0;
			d.g &= 0x3e0;
			d.b &= 0x3e0;
		}
		s.r = nv03_pgraph_do_blend(factor, d.r, s.r, is_r5g5b5);
		s.g = nv03_pgraph_do_blend(factor, d.g, s.g, is_r5g5b5);
		s.b = nv03_pgraph_do_blend(factor, d.b, s.b, is_r5g5b5);
	} else {
		if (op < 0x16) {
			if (op >= 9 && !p.a)
				return pixel;
			uint8_t rop = nv01_pgraph_xlat_rop(op, state->rop);
			bool worop = extr(state->debug[0], 20, 1);
			if (rop == 0xaa && worop && !extr(state->ctx_switch[0], 14, 1))
				return pixel;
			s.r = nv01_pgraph_do_rop(rop, d.r, s.r, p.r) & 0x3ff;
			s.g = nv01_pgraph_do_rop(rop, d.g, s.g, p.g) & 0x3ff;
			s.b = nv01_pgraph_do_rop(rop, d.b, s.b, p.b) & 0x3ff;
			s.i = nv01_pgraph_do_rop(rop, d.i, s.i, p.i) & 0xff;
			s.i16 = nv01_pgraph_do_rop(rop, d.i16, s.i16, p.i16) & 0xffff;
		}
		if (extr(state->ctx_switch[0], 13, 1)) {
			uint32_t ca = extr(state->chroma, 30, 1);
			uint32_t cr = extr(state->chroma, 20, 10);
			uint32_t cg = extr(state->chroma, 10, 10);
			uint32_t cb = extr(state->chroma, 0, 10);
			uint32_t ci = extr(state->chroma, 2, 8);
			uint32_t ci16 = (cb >> 5) | (cg >> 5) << 5 | (cr >> 5) << 10;
			if (fmt == 1) {
				if (ci == s.i && ca) {
					return pixel;
				}
			} else if (fmt == 0 && s.mode == COLOR_MODE_Y16) {
				if (ci16 == s.i16 && ca) {
					return pixel;
				}
			} else if (fmt != 3 && is_rgb5) {
				if ((cr >> 5) == (s.r >> 5) && (cg >> 5) == (s.g >> 5) && (cb >> 5) == (s.b >> 5) && ca) {
					return pixel;
				}
			} else {
				if (cr == s.r && cg == s.g && cb == s.b && ca) {
					return pixel;
				}
			}
		}
	}
	switch (fmt) {
		case 1:
			return s.i;
		case 0:
			if (s.mode == COLOR_MODE_Y16)
				return s.i16;
		case 2:
			if (dither) {
				s.r = nv01_pgraph_dither_10to5(s.r, x, y, false);
				s.g = nv01_pgraph_dither_10to5(s.g, x, y, true);
				s.b = nv01_pgraph_dither_10to5(s.b, x, y, false);
			} else {
				s.r >>= 5;
				s.g >>= 5;
				s.b >>= 5;
			}
			return bd << 15 | s.r << 10 | s.g << 5 | s.b;
		case 3:
			return bd << 31 | extr(s.r, 0, 2) << 28 | extr(s.g, 0, 2) << 26 | extr(s.b, 0, 2) << 24 | extr(s.r, 2, 8) << 16 | extr(s.g, 2, 8) << 8 | extr(s.b, 2, 8);;
		default:
			abort();
	}
}

bool pgraph_cliprect_pass(struct pgraph_state *state, int32_t x, int32_t y) {
	int num = extr(state->cliprect_ctrl, 0, 2);
	if (!num)
		return true;
	if (num == 3)
		num = 2;
	bool covered = false;
	for (int i = 0; i < num; i++) {
		bool rect = true;
		if (x < extr(state->cliprect_min[i], 0, 16))
			rect = false;
		if (y < extr(state->cliprect_min[i], 16, 16))
			rect = false;
		if (x >= extr(state->cliprect_max[i], 0, 16))
			rect = false;
		if (y >= extr(state->cliprect_max[i], 16, 16))
			rect = false;
		if (rect)
			covered = true;
	}
	return covered ^ extr(state->cliprect_ctrl, 4, 1);
}
