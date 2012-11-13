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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "pgraph.h"
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
		case 0:
			factor = replicate ? 0x21 : 0x20;
			a = (color >> 15 & 1) * 0xff;
			r = (color >> 10 & 0x1f) * factor;
			g = (color >> 5 & 0x1f) * factor;
			b = (color & 0x1f) * factor;
			break;
		case 1:
			factor = replicate ? 0x101 : 0x100;
			a = color >> 24 & 0xff;
			r = (color >> 16 & 0xff) * factor >> 6;
			g = (color >> 8 & 0xff) * factor >> 6;
			b = (color & 0xff) * factor >> 6;
			break;
		case 2:
			a = (color >> 30 & 3) * 0x55;
			r = color >> 20 & 0x3ff;
			g = color >> 10 & 0x3ff;
			b = color & 0x3ff;
			break;
		case 3:
			factor = replicate ? 0x101 : 0x100;
			a = color >> 8 & 0xff;
			r = g = b = (color & 0xff) * factor >> 6;
			break;
		case 4:
			a = color >> 24 & 0xff;
			r = g = b = color >> 6 & 0x3ff;
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
			res |= (mono >> i & 1) << (i ^ 7);
	} else {
		res = mono;
	}
	return res;
}
