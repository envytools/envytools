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
		state->xy_a &= 0x1000;
		state->xy_misc_1[0] &= 0x03000000;
		state->xy_misc_4[0] &= 0xff000000;
		state->xy_misc_4[0] |= 0x00555500;
		state->xy_misc_4[1] &= 0xff000000;
		state->xy_misc_4[1] |= 0x00555500;
	} else {
		state->bitmap_color[0] &= 0x3fffffff;
		state->dma_intr_en = 0;
		state->xy_a &= 0x100000;
		state->xy_misc_1[0] &= 0x0f000000;
		state->xy_misc_1[1] &= 0x0f000000;
		state->xy_misc_3 &= ~0x00001100;
		state->xy_misc_4[0] &= 0x30000000;
		state->xy_misc_4[1] &= 0x30000000;
		state->surf_offset[0] = 0;
		state->surf_offset[1] = 0;
		state->surf_offset[2] = 0;
		state->surf_offset[3] = 0;
		state->d3d0_config = 0;
		state->misc24[0] = 0;
		state->misc24[1] = 0;
		state->valid[0] = 0;
		state->xy_clip[0][0] = 0x55555555;
		state->xy_clip[0][1] = 0x55555555;
		state->xy_clip[1][0] = 0x55555555;
		state->xy_clip[1][1] = 0x55555555;
		if (state->chipset.chipset == 0x34)
			state->xy_a |= 0x40000;
	}
}

void pgraph_volatile_reset(struct pgraph_state *state) {
	state->xy_a = 0;
	if (state->chipset.chipset == 0x34)
		state->xy_a |= 0x40000;
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
			state->d3d0_alpha = 0x800;
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
		state->uclip_min[1][0] = 0;
		state->uclip_min[1][1] = 0;
		state->uclip_max[1][0] = 0xffff;
		state->uclip_max[1][1] = 0xffff;
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

uint32_t nv01_pgraph_upconvert_r5g5b5(struct pgraph_state *state, uint32_t pixel) {
	int factor = extr(state->canvas_config, 20, 1) ? 0x21 : 0x20;
	uint16_t r = extr(pixel, 10, 5) * factor;
	uint16_t g = extr(pixel, 5, 5) * factor;
	uint16_t b = extr(pixel, 0, 5) * factor;
	return r << 20 | g << 10 | b;
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

int pgraph_cpp_in(struct pgraph_state *state) {
	if (state->chipset.card_type < 3) {
		int src_format = extr(state->ctx_switch[0], 9, 4) % 5;
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
	} else if (state->chipset.card_type < 4) {
		int src_format = extr(state->ctx_switch[0], 0, 3);
		int cls = pgraph_class(state);
		switch (src_format) {
			case 0: /* (X16)A1R5G5B5 */
			default:
				return 2;
			case 7:
			case 1: /* A8R8G8B8 */
				return 4;
			case 2: /* A2R10G10B10 */
				return 4;
			case 3: /* (X16A8)Y8 */
				return 1;
			case 4: /* A16Y16 */
				if (cls == 0x17)
					return 1;
				return 4;
		}
	} else {
		// XXX
		return 1;
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
	int step = extr(val, 2, 3);
	val >>= 5;
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
	if (val < 0x1f)
		val += d;
	return val;
}

uint8_t nv01_pgraph_dither_8to5(uint8_t val, uint16_t x, uint16_t y, bool isg) {
	return nv01_pgraph_dither_10to5(val << 2, x, y, isg);
}

uint32_t nv01_pgraph_state_downconvert(struct pgraph_state *state, int mode, uint32_t val) {
	if (mode == COLOR_MODE_Y8){
		return extr(val, 2, 8);
	} else if (mode == COLOR_MODE_RGB5 || mode == COLOR_MODE_Y16) {
		return extr(val, 25, 5) << 10 | extr(val, 15, 5) << 5 | extr(val, 5, 5);
	} else {
		return extr(val, 0, 30);
	}
}

void nv01_pgraph_pattern_pixel(struct pgraph_state *state, int x, int y, int mode, uint32_t *ppat, uint8_t *ppa) {
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
	bool bit = extr(state->pattern_mono_bitmap[bidx >> 5], bidx & 0x1f, 1);
	*ppat = nv01_pgraph_state_downconvert(state, mode, state->pattern_mono_rgb[bit]);
	*ppa = state->pattern_mono_a[bit];
}

uint8_t nv01_pgraph_xlat_rop(int op, uint8_t rop) {
	if (op == 0x17)
		return 0xcc;
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
	for (i = 0; i < 32; i++) {
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

uint16_t nv01_pgraph_do_blend_one(uint8_t factor, uint16_t dst, uint16_t src) {
	return ((dst >> 2) * (0xff - factor) + (src >> 2) * factor) >> 6;
}

uint32_t nv01_pgraph_downconvert_to_r5g5b5(struct pgraph_state *state, uint16_t x, uint16_t y, uint32_t val) {
	bool dither = state->chipset.card_type >= 3 || extr(state->canvas_config, 16, 1);
	uint8_t fr, fg, fb;
	if (dither) {
		fr = nv01_pgraph_dither_8to5(extr(val, 22, 8), x, y, false);
		fg = nv01_pgraph_dither_8to5(extr(val, 12, 8), x, y, true);
		fb = nv01_pgraph_dither_8to5(extr(val, 2, 8), x, y, false);
	} else {
		fr = extr(val, 25, 5);
		fg = extr(val, 15, 5);
		fb = extr(val, 5, 5);
	}
	return fr << 10 | fg << 5 | fb;
}

uint32_t nv01_pgraph_upconvert_from_r5g5b5_simple(struct pgraph_state *state, uint16_t color) {
	return extr(color, 10, 5) << 25 | extr(color, 5, 5) << 15 | extr(color, 0, 5) << 5;
}

uint32_t nv01_pgraph_do_blend(struct pgraph_state *state, uint8_t factor, uint32_t dst, uint32_t src) {
	if (factor == 0xff)
		return src;
	if (!factor)
		return dst;
	uint16_t r = nv01_pgraph_do_blend_one(factor, extr(dst, 20, 10), extr(src, 20, 10));
	uint16_t g = nv01_pgraph_do_blend_one(factor, extr(dst, 10, 10), extr(src, 10, 10));
	uint16_t b = nv01_pgraph_do_blend_one(factor, extr(dst, 0, 10), extr(src, 0, 10));
	return r << 20 | g << 10 | b;
}

uint32_t nv01_pgraph_solid_rop(struct pgraph_state *state, int x, int y, uint32_t pixel) {
	struct pgraph_color s = pgraph_expand_color(state, state->misc32[0]);
	return nv01_pgraph_rop(state, x, y, pixel, s);
}

uint32_t nv01_pgraph_rop(struct pgraph_state *state, int x, int y, uint32_t pixel, struct pgraph_color s) {
	bool bypass = extr(state->canvas_config, 0, 1);
	bool expand_y8 = extr(state->canvas_config, 12, 1);
	int cpp = nv01_pgraph_cpp(state->pfb_config);
	int op = extr(state->ctx_switch[0], 0, 5);
	int blend_en = op >= 0x18;
	bool dither = extr(state->canvas_config, 16, 1);
	int mode;
	if (cpp == 1 || (s.mode == COLOR_MODE_Y8 && !expand_y8 && !blend_en)) {
		mode = COLOR_MODE_Y8;
	} else if (s.mode == COLOR_MODE_RGB5 && cpp == 2) {
		mode = COLOR_MODE_RGB5;
	} else if (cpp == 2 && blend_en && !dither) {
		mode = COLOR_MODE_RGB5;
	} else {
		mode = COLOR_MODE_RGB10;
	}
	uint32_t mask;
	uint32_t src;
	uint32_t dst;
	if (mode == COLOR_MODE_Y8) {
		mask = 0xff;
		src = s.i;
		dst = pixel & mask;
	} else if (mode == COLOR_MODE_RGB5) {
		mask = 0x7fff;
		src = extr(s.r, 5, 5) << 10 | extr(s.g, 5, 5) << 5 | extr(s.b, 5, 5);
		dst = pixel & mask;
	} else {
		mask = 0x3fffffff;
		src = s.r << 20 | s.g << 10 | s.b;
		if (cpp == 2)
			dst = nv01_pgraph_upconvert_r5g5b5(state, pixel);
		else
			dst = pixel & mask;
	}
	if (!s.a)
		return pixel;
	int plane_alpha_en = extr(state->debug[0], 28, 1);
	uint32_t pat;
	uint8_t pa;
	nv01_pgraph_pattern_pixel(state, x, y, mode, &pat, &pa);
	if ((op >= 9 && op < 0x16) || op >= 0x1b) {
		if (!pa)
			return pixel;
	}
	uint32_t ropres;
	if (!blend_en) {
		bool worop = extr(state->debug[0], 20, 1);
		uint8_t rop = nv01_pgraph_xlat_rop(op, state->rop);
		if (rop == 0xaa && worop && !(state->ctx_switch[0] & 0x40))
			return pixel;
		ropres = nv01_pgraph_do_rop(rop, dst, src, pat) & mask;
		if (extr(state->ctx_switch[0], 5, 1)) {
			uint32_t chr = nv01_pgraph_state_downconvert(state, mode, state->chroma);
			if (chr == ropres && extr(state->chroma, 30, 1))
				return pixel;
		}
		if (extr(state->ctx_switch[0], 6, 1)) {
			uint32_t pma = nv01_pgraph_state_downconvert(state, mode, state->plane);
			if (!extr(state->plane, 30, 1) && plane_alpha_en)
				return pixel;
			ropres = (pma & ropres) | (~pma & dst);
		}
	} else {
		uint8_t beta = extr(state->beta, 23, 8);
		uint8_t factor;
		uint32_t other;
		if (op == 0x18) {
			factor = nv01_pgraph_blend_factor_square(s.a);
			other = dst;
		} else if (op == 0x19) {
			if (!beta)
				return pixel;
			factor = nv01_pgraph_blend_factor(s.a, beta);
			other = dst;
		} else if (op == 0x1a) {
			if (beta == 0xff)
				return pixel;
			factor = nv01_pgraph_blend_factor(s.a, 0xff-beta);
			other = dst;
		} else if (op == 0x1b) {
			factor = beta;
			other = pat;
		} else if (op == 0x1c) {
			factor = 0xff - beta;
			other = pat;
		} else {
			abort();
		}
		if (mode == COLOR_MODE_RGB5) {
			src = nv01_pgraph_upconvert_from_r5g5b5_simple(state, src);
			other = nv01_pgraph_upconvert_from_r5g5b5_simple(state, other);
		}
		ropres = nv01_pgraph_do_blend(state, factor, other, src);
	}
	if (cpp == 2 && (mode == COLOR_MODE_RGB10 || blend_en)) {
		ropres = nv01_pgraph_downconvert_to_r5g5b5(state, x, y, ropres);
	}
	switch (cpp) {
		case 1:
			return ropres;
		case 2:
			return bypass << 15 | ropres;
		case 4:
			return bypass << 31 | ropres;
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

uint32_t nv03_pgraph_do_blend_one(uint32_t factor, uint32_t dst, uint32_t src) {
	return ((dst >> 2) * (0x20 - factor) + (src >> 2) * factor) >> 3;
}

uint32_t nv03_pgraph_do_blend(struct pgraph_state *state, uint8_t factor, uint32_t dst, uint32_t src) {
	factor >>= 3;
	if (factor == 0x1f)
		return src;
	if (!factor)
		return dst;
	uint16_t r = nv03_pgraph_do_blend_one(factor, extr(dst, 20, 10), extr(src, 20, 10));
	uint16_t g = nv03_pgraph_do_blend_one(factor, extr(dst, 10, 10), extr(src, 10, 10));
	uint16_t b = nv03_pgraph_do_blend_one(factor, extr(dst, 0, 10), extr(src, 0, 10));
	return r << 20 | g << 10 | b;
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
	int mode;
	if (fmt == 1) {
		mode = COLOR_MODE_Y8;
	} else if (fmt == 3) {
		mode = COLOR_MODE_RGB10;
	} else if (fmt == 0 && s.mode == COLOR_MODE_Y16) {
		mode = COLOR_MODE_Y16;
	} else if (is_rgb5) {
		mode = COLOR_MODE_RGB5;
	} else {
		mode = COLOR_MODE_RGB10;
	}
	if (!s.a)
		return pixel;
	uint32_t src, dst;
	uint32_t mask;
	if (mode == COLOR_MODE_Y8) {
		mask = 0xff;
		src = s.i;
		dst = pixel & mask;
	} else if (mode == COLOR_MODE_Y16) {
		mask = 0xffff;
		src = s.i16;
		dst = pixel & mask;
	} else if (mode == COLOR_MODE_RGB5) {
		mask = 0x7fff;
		src = extr(s.r, 5, 5) << 10 | extr(s.g, 5, 5) << 5 | extr(s.b, 5, 5);
		dst = pixel & mask;
	} else if (mode == COLOR_MODE_RGB10) {
		mask = 0x3fffffff;
		src = s.r << 20 | s.g << 10 | s.b;
		if (fmt == 3) {
			dst = 0;
			insrt(dst, 0, 2, extr(pixel, 24, 2));
			insrt(dst, 2, 8, extr(pixel, 0, 8));
			insrt(dst, 10, 2, extr(pixel, 26, 2));
			insrt(dst, 12, 8, extr(pixel, 8, 8));
			insrt(dst, 20, 2, extr(pixel, 28, 2));
			insrt(dst, 22, 8, extr(pixel, 16, 8));
		} else {
			dst = nv01_pgraph_upconvert_r5g5b5(state, pixel);
		}
	}
	uint32_t pat;
	uint8_t pa;
	nv01_pgraph_pattern_pixel(state, x, y, mode, &pat, &pa);
	if (op >= 9 && op < 0x16 && !pa)
		return pixel;
	uint32_t ropres;
	if (!blend_en) {
		bool worop = extr(state->debug[0], 20, 1);
		uint8_t rop = nv01_pgraph_xlat_rop(op, state->rop);
		if (rop == 0xaa && worop && !extr(state->ctx_switch[0], 14, 1))
			return pixel;
		ropres = nv01_pgraph_do_rop(rop, dst, src, pat) & mask;
		if (extr(state->ctx_switch[0], 13, 1)) {
			uint32_t chr = nv01_pgraph_state_downconvert(state, mode, state->chroma);
			if (chr == ropres && extr(state->chroma, 30, 1))
				return pixel;
		}
	} else {
		uint8_t beta = extr(state->beta, 23, 8);
		uint8_t factor;
		uint32_t other;
		if (op == 0x19) {
			if (!beta)
				return pixel;
			factor = nv03_pgraph_blend_factor(s.a, beta);
			other = dst;
		} else if (op == 0x1a) {
			if (beta == 0xff)
				return pixel;
			factor = nv03_pgraph_blend_factor(s.a, 0xff-beta);
			other = dst;
		} else if (op == 0x1d) {
			factor = s.a;
			other = dst;
		} else {
			abort();
		}
		if (mode == COLOR_MODE_RGB5) {
			src = nv01_pgraph_upconvert_from_r5g5b5_simple(state, src);
			other = nv01_pgraph_upconvert_from_r5g5b5_simple(state, other);
		} else {
			insrt(src, 0, 2, extr(src, 8, 2));
			insrt(src, 10, 2, extr(src, 18, 2));
			insrt(src, 20, 2, extr(src, 28, 2));
		}
		ropres = nv03_pgraph_do_blend(state, factor, other, src);
	}
	if (fmt != 3 && (mode == COLOR_MODE_RGB10 || blend_en)) {
		ropres = nv01_pgraph_downconvert_to_r5g5b5(state, x, y, ropres);
	}
	switch (fmt) {
		case 1:
			return ropres;
		case 0:
		case 2:
			if (mode == COLOR_MODE_Y16)
				return ropres;
			return bd << 15 | ropres;
		case 3:
			return bd << 31 |
				extr(ropres, 20, 2) << 28 |
				extr(ropres, 10, 2) << 26 |
				extr(ropres, 0, 2) << 24 |
				extr(ropres, 22, 8) << 16 |
				extr(ropres, 12, 8) << 8 |
				extr(ropres, 2, 8);
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

void pgraph_prep_draw(struct pgraph_state *state, bool poly, bool noclip) {
	if (state->chipset.card_type < 3) {
		uint32_t class = extr(state->access, 12, 5);
		if (!nv01_pgraph_is_drawable_class(class))
			return;
		int vidx = pgraph_vtxid(state);
		if (class == 0x08) {
			if ((state->valid[0] & 0x1001) != 0x1001)
				state->intr |= 1 << 16;
			state->valid[0] &= ~0xffffff;
		} else if (class == 0x0c) {
			if ((state->valid[0] & 0x3103) != 0x3103)
				state->intr |= 1 << 16;
			state->valid[0] &= ~0xffffff;
		} else if (class == 0x10) {
			if ((state->valid[0] & 0xf10f) != 0xf10f)
				state->intr |= 1 << 16;
			state->valid[0] &= ~0xffffff;
		} else if (class >= 0x09 && class <= 0x0a) {
			if (!poly) {
				if ((state->valid[0] & 0x3103) != 0x3103)
					state->intr |= 1 << 16;
				state->valid[0] &= ~0x00f00f;
			} else {
				if ((state->valid[0] & 0x3f13f) != 0x30130)
					state->intr |= 1 << 16;
				state->valid[0] &= ~(0x10010 << (vidx & 3));
				if (state->valid[0] & 0xf00f)
					state->valid[0] &= ~0x100;
			}
		} else if (class == 0x0b) {
			if (!poly) {
				if ((state->valid[0] & 0x7107) != 0x7107)
					state->intr |= 1 << 16;
				state->valid[0] &= ~0x00f00f;
			} else {
				if ((state->valid[0] & 0x7f17f) != 0x70170)
					state->intr |= 1 << 16;
				state->valid[0] &= ~(0x10010 << (vidx & 3));
				if (state->valid[0] & 0xf00f)
					state->valid[0] &= ~0x100;
			}
		} else if (class == 0x11 || class == 0x12) {
			if ((state->valid[0] & 0x38038) != 0x38038)
				state->intr |= 1 << 16;
			/* XXX: this steps the IFC machine */
		} else if (class == 0x13) {
			/* XXX: this steps the IFM machine */
		}
		if (state->xy_misc_4[0] & 0xf0)
			state->intr |= 1 << 12;
		if (state->xy_misc_4[1] & 0xf0)
			state->intr |= 1 << 12;
		if (state->valid[0] & 0x11000000 && state->ctx_switch[0] & 0x80)
			state->intr |= 1 << 16;
		if (extr(state->canvas_config, 24, 1))
			state->intr |= 1 << 20;
		if (extr(state->cliprect_ctrl, 8, 1))
			state->intr |= 1 << 24;
		if (state->intr)
			state->access &= ~0x101;
	} else {
		int cls = extr(state->ctx_user, 16, 5);
		if (extr(state->cliprect_ctrl, 8, 1))
			insrt(state->intr, 24, 1, 1);
		if (cls != 0x17) {
			if (extr(state->xy_misc_4[0], 4, 4) || extr(state->xy_misc_4[1], 4, 4))
				insrt(state->intr, 12, 1, 1);
		}
		if (cls == 0xd) {
			// nop
		} else if (cls == 0xc || cls == 0xe || cls == 0x15) {
			if (!noclip && state->valid[0] & 0xa0000000)
				insrt(state->intr, 16, 1, 1);
		} else {
			if (state->valid[0] & 0x50000000 && extr(state->ctx_switch[0], 15, 1))
				insrt(state->intr, 16, 1, 1);
		}
		if (extr(state->debug[3], 22, 1)) {
			bool bad = false;
			int cfmt = extr(state->ctx_switch[0], 0, 3);
			int msk = extr(state->ctx_switch[0], 20, 4);
			if (cls == 0x0d) {
				// nop
			} else if (cls == 0x14) {
				int sidx = extr(state->ctx_switch[0], 16, 2);
				int sfmt = extr(state->surf_format, 4*sidx, 3);
				int dfmt = 3;
				if (cfmt == 0)
					dfmt = 2;
				if (cfmt == 3)
					dfmt = 1;
				if (cfmt == 4)
					bad = true;
				if ((sfmt & 3) != dfmt)
					bad = true;
				for (int j = 0; j < 4; j++) {
					if (msk & 1 << j) {
						int fmt = extr(state->surf_format, 4*j, 3);
						if (!(fmt & 4))
							bad = true;
					}
				}
			} else {
				bool passthru = extr(state->ctx_switch[0], 24, 5) == 0x17 && extr(state->ctx_switch[0], 13, 2) == 0;
				int fmt = -1;
				int cnt = 0;
				for (int j = 0; j < 4; j++) {
					if (msk & 1 << j || (msk == 0 && j == 3)) {
						cnt++;
						if (fmt == -1)
							fmt = extr(state->surf_format, 4*j, 3);
						else if (fmt != (int)extr(state->surf_format, 4*j, 3))
							bad = true;
					}
				}
				if (fmt == 0 && msk)
					bad = true;
				if (cls == 0x17 || cls == 0x18) {
					if (fmt != 6 && fmt != 2)
						bad = true;
					if (fmt == 2 && msk)
						bad = true;
				}
				if (cls == 0xe) {
					if (fmt < 4 && msk)
						bad = true;
				} else if (cls == 0x10) {
					int sidx = extr(state->ctx_switch[0], 16, 2);
					int sfmt = extr(state->surf_format, 4*sidx, 3);
					if (cnt == 1) {
						if ((sfmt & 3) != (fmt & 3))
							bad = true;
					} else {
						if (sfmt != (fmt & 3) && sfmt != fmt)
							bad = true;
					}
					if (fmt < 4 && msk)
						bad = true;
				} else {
					if (fmt == 0 || fmt == 4) {
						if (cls == 0xc)
							bad = true;
						if (!passthru)
							bad = true;
						if (cfmt != 4)
							bad = true;
					}
				}
				if ((fmt == 5 || fmt == 1) && (cfmt != 3 || cls == 0x0e))
					bad = true;
			}
			if (bad)
				insrt(state->intr, 20, 1, 1);
		}
		int vidx = pgraph_vtxid(state);
		switch (cls) {
			case 8:
				if ((state->valid[0] & 0x010101) != 0x010101)
					insrt(state->intr, 16, 1, 1);
				if (!(state->intr & 0x01111000)) {
					insrt(state->valid[0], 0, 16, 0);
					insrt(state->valid[0], 21, 1, 0);
				}
				break;
			case 0x18:
				if ((state->valid[0] & 0x4210000) != 0x4210000)
					insrt(state->intr, 16, 1, 1);
				break;
			case 9:
			case 0xa:
				if (!poly) {
					if ((state->valid[0] & 0x210303) != 0x210303)
						insrt(state->intr, 16, 1, 1);
					if (!(state->intr & 0x01111000)) {
						insrt(state->valid[0], 0, 4, 0);
						insrt(state->valid[0], 8, 4, 0);
					}
				} else {
					if ((state->valid[0] & 0x213030) != 0x213030)
						insrt(state->intr, 16, 1, 1);
					if (!(state->intr & 0x01111000)) {
						insrt(state->valid[0], 4+(vidx&3), 1, 0);
						insrt(state->valid[0], 12+(vidx&3), 1, 0);
					}
				}
				break;
			case 0xb:
				if (!poly) {
					if ((state->valid[0] & 0x210707) != 0x210707)
						insrt(state->intr, 16, 1, 1);
					if (!(state->intr & 0x01111000)) {
						insrt(state->valid[0], 0, 4, 0);
						insrt(state->valid[0], 8, 4, 0);
					}
				} else {
					if ((state->valid[0] & 0x217070) != 0x217070)
						insrt(state->intr, 16, 1, 1);
					if (!(state->intr & 0x01111000)) {
						insrt(state->valid[0], 4+(vidx&3), 1, 0);
						insrt(state->valid[0], 12+(vidx&3), 1, 0);
					}
				}
				break;
			case 0x7:
				if ((state->valid[0] & 0x10203) != 0x10203)
					insrt(state->intr, 16, 1, 1);
				if ((uint32_t)extrs(state->vtx_xy[1][0], 0, 16) != state->vtx_xy[1][0])
					insrt(state->intr, 12, 1, 1);
				if ((uint32_t)extrs(state->vtx_xy[1][1], 0, 16) != state->vtx_xy[1][1])
					insrt(state->intr, 12, 1, 1);
				if (!(state->intr & 0x01111000)) {
					insrt(state->valid[0], 0, 16, 0);
					insrt(state->valid[0], 21, 1, 0);
				}
				break;
			case 0xc:
				if ((state->valid[0] & 0x10203) != 0x10203)
					insrt(state->intr, 16, 1, 1);
				if (!noclip && !extr(state->valid[0], 20, 1))
					insrt(state->intr, 16, 1, 1);
				if ((uint32_t)extrs(state->vtx_xy[1][0], 0, 16) != state->vtx_xy[1][0])
					insrt(state->intr, 12, 1, 1);
				if ((uint32_t)extrs(state->vtx_xy[1][1], 0, 16) != state->vtx_xy[1][1])
					insrt(state->intr, 12, 1, 1);
				if (!(state->intr & 0x01111000)) {
					insrt(state->valid[0], 0, 2, 0);
					insrt(state->valid[0], 8, 2, 0);
					insrt(state->valid[0], 21, 1, 0);
				}
				break;
			case 0x10:
				if ((state->valid[0] & 0x00003) != 0x00003)
					insrt(state->intr, 16, 1, 1);
				if (!(state->intr & 0x01111000)) {
					insrt(state->valid[0], 0, 16, 0);
					insrt(state->valid[0], 21, 1, 0);
				}
				break;
			case 0x14:
				if ((state->valid[0] & 0x2c25) != 0x2c25)
					insrt(state->intr, 16, 1, 1);
				break;
			case 0xd:
				if ((state->valid[0] & 0xff) != 0xff)
					insrt(state->intr, 16, 1, 1);
				break;
			case 0xe:
				if ((state->valid[0] & 0x100033) != 0x100033)
					insrt(state->intr, 16, 1, 1);
				if (extr(state->valid[0], 8, 4) != 0xf &&
					extr(state->valid[0], 12, 6) != 0x3f)
					insrt(state->intr, 16, 1, 1);
				break;
			case 0x17:
				for (int t = 0; t < 2; t++) {
					int idx[3];
					for (int j = 0; j < 3; j++)
						idx[j] = extr(state->misc24[1], t*12+j*4, 4);
					if (idx[0] != idx[1] && idx[1] != idx[2] && idx[0] != idx[2]) {
						for (int j = 0; j < 3; j++) {
							if (!extr(state->valid[0], idx[j], 1))
								insrt(state->intr, 16, 1, 1);
						}
						if (extr(state->valid[0], 23, 5) != 0x1f)
							insrt(state->intr, 16, 1, 1);
					}
				}
				break;
			default:
				abort();
		}
		if (state->intr)
			state->fifo_enable = 0;
	}
}

void pgraph_set_surf_format(struct pgraph_state *state, int which, uint32_t fmt) {
	if (state->chipset.card_type < 0x30)
		insrt(state->surf_format, which*4, 4, fmt);
	else
		insrt(state->surf_format, which*5, 5, fmt);
}

uint32_t pgraph_surf_format(struct pgraph_state *state, int which) {
	if (state->chipset.card_type < 0x30)
		return extr(state->surf_format, which*4, 4);
	else
		return extr(state->surf_format, which*5, 5);
}

bool pgraph_state3d_ok(struct pgraph_state *state) {
	if (state->chipset.card_type < 0x10)
		return true;
	if (!extr(state->debug[3], 27, 1))
		return true;
	if (!nv04_pgraph_is_3d_class(state))
		return true;
	uint32_t cls = pgraph_class(state);
	bool state3d_ok = false;
	if (extr(state->state3d, 0, 16) == state->ctx_switch[3] && extr(state->state3d, 24, 1))
		state3d_ok = true;
	if (!extr(state->debug[3], 13, 1)) {
		if (cls == 0x48) {
			state3d_ok = extr(state->state3d, 28, 1);
		} else if (cls == 0x54 || cls == 0x94) {
			state3d_ok = extr(state->state3d, 29, 1);
		} else if (cls == 0x55 || cls == 0x95) {
			state3d_ok = extr(state->state3d, 30, 1);
		}
	}
	if (state->chipset.card_type < 0x20 && state->chipset.chipset != 0x10) {
		if (extr(state->debug[3], 9, 1))
			state3d_ok = extr(state->state3d, 25, 1) && extr(state->state3d, 16, 5) == extr(state->ctx_user, 24, 5);
	}
	return state3d_ok;
}
