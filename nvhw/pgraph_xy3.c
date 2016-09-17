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
#include <stdlib.h>

void nv03_pgraph_clip_bounds(struct nv03_pgraph_state *state, int32_t min[2], int32_t max[2]) {
	int i;
	int cls = extr(state->ctx_user, 16, 5);
	bool oce = cls == 0xc || cls == 0xe || cls == 0x15;
	for (i = 0; i < 2; i++) {
		min[i] = extr(state->dst_canvas_min, i*16, 16);
		max[i] = extr(state->dst_canvas_max, i*16, 16);
		int sel = extr(state->xy_misc_1[0], 12+i*4, 3);
		int sel2 = extr(state->xy_misc_1[1], 12+i*4, 3);
		bool uce = extr(state->ctx_switch, 15, 1);
		if (sel & 1 && uce && !oce)
			min[i] = state->uclip_min[i];
		if (sel & 2 && uce && !oce)
			max[i] = state->uclip_max[i];
		if (sel2 & 1 && oce)
			min[i] = state->oclip_min[i];
		if (sel2 & 2 && oce)
			max[i] = state->oclip_max[i];
		if (i == 0) {
			min[i] &= 0x7ff;
			max[i] &= 0x7ff;
		} else {
			min[i] &= 0x3fff;
			max[i] &= 0x3fff;
		}
	}
}

int nv03_pgraph_clip_status(struct nv03_pgraph_state *state, int32_t coord, int xy, int is_tex_class) {
	int32_t clip_min[2], clip_max[2];
	nv03_pgraph_clip_bounds(state, clip_min, clip_max);
	int cstat = 0;
	int cls = extr(state->ctx_user, 16, 5);
	if (cls == 0x17)
		coord = extrs(coord, 4, 12);
	else
		coord = extrs(coord, 0, 18);
	if (coord < clip_min[xy])
		cstat |= 1;
	if (coord == clip_min[xy])
		cstat |= 2;
	if (coord > clip_max[xy])
		cstat |= 4;
	if (coord == clip_max[xy])
		cstat |= 8;
	return cstat;
}

void nv03_pgraph_set_xym2(struct nv03_pgraph_state *state, int xy, int idx, bool carry, bool oob, int cstat) {
	int cls = extr(state->ctx_user, 16, 5);
	if ((cls >= 0x8 && cls <= 0xc) || cls == 0xe || (cls >= 0x10 && cls <= 0x12) || cls == 0x14 || cls == 0x15) {
		if (cls == 0x15)
			oob = false;
		insrt(state->xy_misc_4[xy], (idx&3), 1, carry);
		insrt(state->xy_misc_4[xy], (idx&3)+4, 1, oob);
	}
	if (cls == 8 || cls == 0x18) {
		insrt(state->xy_clip[xy][0], 0, 4, cstat);
		insrt(state->xy_clip[xy][0], 4, 4, cstat);
	} else {
		insrt(state->xy_clip[xy][idx >> 3], idx * 4 & 0x1f, 4, cstat);
	}
}

void nv03_pgraph_vtx_fixup(struct nv03_pgraph_state *state, int xy, int idx, int32_t coord) {
	int cls = extr(state->ctx_user, 16, 5);
	int cstat = nv03_pgraph_clip_status(state, coord, xy, cls);
	int oob = (coord >= 0x8000 || coord < -0x8000);
	int carry = 0;
	nv03_pgraph_set_xym2(state, xy, idx, carry, oob, cstat);
}

void nv03_pgraph_iclip_fixup(struct nv03_pgraph_state *state, int xy, int32_t coord) {
	int cls = extr(state->ctx_user, 16, 5);
	int i;
	int32_t clip_min[2], clip_max[2];
	nv03_pgraph_clip_bounds(state, clip_min, clip_max);
	state->iclip[xy] = coord & 0x3ffff;
	if (cls == 0x17)
		coord = extrs(coord, 4, 12);
	else
		coord = extrs(coord, 0, 18);
	for (i = 0; i < 2; i++) {
		insrt(state->xy_misc_1[i], 14+xy*4, 1, coord <= clip_max[xy]);
		if (!xy) {
			insrt(state->xy_misc_1[i], 20, 1, coord <= clip_max[xy]);
			insrt(state->xy_misc_1[i], 4, 1, coord < clip_min[xy]);
		} else {
			insrt(state->xy_misc_1[i], 5, 1, coord < clip_min[xy]);
		}
	}
}

void nv03_pgraph_uclip_fixup(struct nv03_pgraph_state *state, int uo, int xy, int idx, int32_t coord) {
	int cls = extr(state->ctx_user, 16, 5);
	uint32_t *umin = uo ? state->oclip_min : state->uclip_min;
	uint32_t *umax = uo ? state->oclip_max : state->uclip_max;
	umin[xy] = umax[xy];
	umax[xy] = coord & 0x3ffff;
	if (!xy)
		state->vtx_x[13] = coord;
	else
		state->vtx_y[13] = coord;
	state->xy_misc_1[uo] &= ~0x00177000;
	if (idx) {
		int32_t clip_min[2], clip_max[2];
		nv03_pgraph_clip_bounds(state, clip_min, clip_max);
		int32_t ucmin = extrs(umin[xy], 0, 18);
		int32_t ucmax = extrs(umax[xy], 0, 18);
		if (cls == 0x17) {
			ucmin = extrs(ucmin, 4, 12);
			ucmax = extrs(ucmax, 4, 12);
		}
		insrt(state->xy_misc_1[uo], 4 + xy, 1, 0);
		insrt(state->xy_misc_1[uo], 8 + xy, 1,
			ucmax < clip_min[xy] || (extr(state->xy_clip[xy][0], 2, 1) && ucmax >= 0));
		insrt(state->xy_misc_1[uo], 12 + xy * 4, 1, !extr(state->xy_clip[xy][0], 0, 1));
		insrt(state->xy_misc_1[uo], 13 + xy * 4, 1, ucmax <= clip_max[xy]);
	}
}
