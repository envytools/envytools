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

void nv01_pgraph_clip_bounds(struct nv01_pgraph_state *state, uint32_t min[2], uint32_t max[2]) {
	int i;
	for (i = 0; i < 2; i++) {
		min[i] = state->canvas_min >> (i * 16) & 0x8fff;
		if (min[i] & 0x8000)
			min[i] = 0;
		max[i] = state->canvas_max >> (i * 16) & 0xfff;
		int conf = state->xy_misc_1 >> (12 + i * 4) & 7;
		if (conf & 1 && state->ctx_switch & 0x80)
			min[i] = state->uclip_min[i] & 0xfff;
		if (conf & 2 && state->ctx_switch & 0x80)
			max[i] = state->uclip_max[i] & 0xfff;
		if (conf & 4)
			max[i] = state->iclip[i] & 0xfff;
	}
}

void nv01_pgraph_clip_status(struct nv01_pgraph_state *state, int32_t coord, int xy, int *cstat, int *oob) {
	uint32_t class = state->access >> 12 & 0x1f;
	int is_texlin_class = (class & 0xf) == 0xd;
	int is_texquad_class = (class & 0xf) == 0xe;
	int is_tex_class = is_texlin_class || is_texquad_class;
	uint32_t clip_min[2], clip_max[2];
	nv01_pgraph_clip_bounds(state, clip_min, clip_max);
	*cstat = 0;
	*oob = 0;
	if (is_tex_class) {
		coord >>= 15;
		coord &= 0xffff;
		if (state->xy_misc_1 & 0x02000000) {
			coord >>= 4;
			if (coord & 0x800)
				coord -= 0x1000;
		} else {
			if (coord & 0x8000)
				coord -= 0x10000;
		}
	} else {
		if (coord >= 0x8000 || coord < -0x8000)
			*oob = 1;
		coord &= 0x3ffff;
		if (coord & 0x20000)
			coord -= 0x40000;
	}
	if (coord < (int32_t)clip_min[xy])
		*cstat |= 1;
	if (coord == (int32_t)clip_min[xy])
		*cstat |= 2;
	if (coord > (int32_t)clip_max[xy])
		*cstat |= 4;
	if (coord == (int32_t)clip_max[xy])
		*cstat |= 8;
}

int nv01_pgraph_use_v16(struct nv01_pgraph_state *state) {
	uint32_t class = state->access >> 12 & 0x1f;
	int d0_24 = state->debug[0] >> 24 & 1;
	int d1_8 = state->debug[1] >> 8 & 1;
	int d1_24 = state->debug[1] >> 24 & 1;
	int sd[2], sde[4];
	sd[0] = state->subdivide >> 0 & 0xf;
	sd[1] = state->subdivide >> 4 & 0xf;
	sde[0] = state->subdivide >> 16 & 0xf;
	sde[1] = state->subdivide >> 20 & 0xf;
	sde[2] = state->subdivide >> 24 & 0xf;
	sde[3] = state->subdivide >> 28 & 0xf;
	int sdl = sd[0] <= sde[0] && sd[0] <= sde[1] && sd[1] <= sde[2] && sd[1] <= sde[3];
	int biopt = d1_8 && !(state->xy_misc_2[0] & 3 << 28) && !(state->xy_misc_2[1] & 3 << 28);
	switch (class) {
		case 8:
			return 1;
		case 0xd:
		case 0x1d:
			return d1_24 && (!d0_24 || sdl) && !biopt;
		case 0xe:
		case 0x1e:
			return d1_24 && (!d0_24 || sdl);
		default:
			return 0;
	}
}

void nv01_pgraph_vtx_fixup(struct nv01_pgraph_state *state, int xy, int idx) {
	uint32_t class = state->access >> 12 & 0x1f;
	int32_t coord = xy ? state->vtx_y[idx] : state->vtx_x[idx];
	int oob, cstat;
	nv01_pgraph_clip_status(state, coord, xy, &cstat, &oob);
	state->xy_misc_2[xy] &= ~0x11;
	state->xy_misc_2[xy] |= oob << 4;
	if (idx < 16 || nv01_pgraph_use_v16(state)) {
		state->xy_misc_2[xy] &= ~0xf00;
		state->xy_misc_2[xy] |= cstat << 8;
		if (class == 8) {
			state->xy_misc_2[xy] &= ~0xf000;
			state->xy_misc_2[xy] |= cstat << 12;
		}
	}
	if (idx >= 16) {
		int shift = 16 + xy * 4 + (idx - 16) * 8;
		state->edgefill &= ~(0xf << shift);
		state->edgefill |= cstat << shift;
	}
}

void nv01_pgraph_iclip_fixup(struct nv01_pgraph_state *state, int xy, int32_t coord) {
	uint32_t class = state->access >> 12 & 0x1f;
	int is_texlin_class = (class & 0xf) == 0xd;
	int is_texquad_class = (class & 0xf) == 0xe;
	int is_tex_class = is_texlin_class || is_texquad_class;
	int max = state->canvas_max >> (xy * 16) & 0xfff;
	if (state->xy_misc_1 & 0x2000 << (xy * 4) && state->ctx_switch & 0x80)
		max = state->uclip_max[xy] & 0xfff;
	int32_t cmin = state->canvas_min >> 16 * xy & 0x8fff;
	if (cmin & 0x8000)
		cmin = 0;
	if (state->ctx_switch & 0x80 && state->xy_misc_1 & 0x1000 << xy * 4)
		cmin = state->uclip_min[xy] & 0xfff;
	if (is_tex_class) {
		coord >>= 15;
		coord &= 0xffff;
		if (state->xy_misc_1 & 0x02000000) {
			coord >>= 4;
			if (coord & 0x800)
				coord -= 0x1000;
		} else {
			if (coord & 0x8000)
				coord -= 0x10000;
		}
	} else {
		coord &= 0x3ffff;
		if (coord & 0x20000)
			coord -= 0x40000;
	}
	state->xy_misc_1 &= ~0x144000;
	state->xy_misc_1 |= (coord <= max) << (xy * 4 + 14);
	if (!xy) {
		state->xy_misc_1 &= ~0x10;
		state->xy_misc_1 |= (coord <= max) << 20;
		state->xy_misc_1 |= (coord < cmin) << 4;
	} else {
		state->xy_misc_1 &= ~0x20;
		state->xy_misc_1 |= (coord < cmin) << 5;
	}
}
