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

void nv03_pgraph_clip_bounds(struct nv03_pgraph_state *state, int32_t min[2], int32_t max[2], bool canvas_only) {
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
		if (sel2 & 1 && oce && !canvas_only)
			min[i] = state->oclip_min[i];
		if (sel2 & 2 && oce && !canvas_only)
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

int nv03_pgraph_clip_status(struct nv03_pgraph_state *state, int32_t coord, int xy, bool canvas_only) {
	int32_t clip_min[2], clip_max[2];
	nv03_pgraph_clip_bounds(state, clip_min, clip_max, canvas_only);
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
	int cstat = nv03_pgraph_clip_status(state, coord, xy, false);
	int oob = (coord >= 0x8000 || coord < -0x8000);
	int carry = 0;
	nv03_pgraph_set_xym2(state, xy, idx, carry, oob, cstat);
}

void nv03_pgraph_iclip_fixup(struct nv03_pgraph_state *state, int xy, int32_t coord) {
	int cls = extr(state->ctx_user, 16, 5);
	int i;
	int32_t clip_min[2], clip_max[2];
	nv03_pgraph_clip_bounds(state, clip_min, clip_max, false);
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
		nv03_pgraph_clip_bounds(state, clip_min, clip_max, false);
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

void nv03_pgraph_set_clip(struct nv03_pgraph_state *state, int which, int idx, uint32_t val, bool prev_inited) {
	int xy;
	bool is_size = which < 3 && idx == 1;
	bool is_o = which >= 1;
	state->xy_misc_1[is_o] &= ~0x00177000;
	if (idx) {
		int n = extr(state->valid, 28 + is_o, 1);
		insrt(state->valid, 28 + is_o, 1, 0);
		insrt(state->valid, 30 + is_o, 1, !n);
		state->xy_misc_1[is_o] &= ~0x00000330;
	} else {
		insrt(state->valid, 19, 1, 0);
		insrt(state->valid, 28 + is_o, 1, 1);
		insrt(state->valid, 30 + is_o, 1, 0);
		insrt(state->xy_misc_1[1], 0, 1, which != 2);
		insrt(state->xy_misc_3, 8, 1, 0);
	}
	if (!is_size)
		insrt(state->xy_misc_1[0], 0, 1, 0);
	if (is_o)
		insrt(state->valid, 20, 1, 1);
	for (xy = 0; xy < 2; xy++) {
		int32_t coord = extr(val, xy*16, 16);
		int32_t orig = coord;
		int32_t ovcoord;
		int32_t base = xy ? state->vtx_y[13] : state->vtx_x[13];
		bool ovf = false;
		if (is_size) {
			coord += (uint32_t)base;
			ovcoord = coord;
			if (extr(base, 31, 1) == extr(orig, 31, 1) &&
				extr(base, 31, 1) != extr(coord, 31, 1)) {
				ovcoord = extr(coord, 31, 1) ? 0x7fffffff : 0x80000000;
				ovf = true;
			}
		} else {
			ovcoord = coord = (int16_t)coord;
		}
		if (!xy)
			state->vtx_x[13] = ovcoord;
		else
			state->vtx_y[13] = ovcoord;
		if (is_o) {
			state->oclip_min[xy] = state->oclip_max[xy];
			state->oclip_max[xy] = coord & 0x3ffff;
		} else {
			state->uclip_min[xy] = state->uclip_max[xy];
			state->uclip_max[xy] = coord & 0x3ffff;
		}
		int cstat = nv03_pgraph_clip_status(state, coord, xy, false);
		insrt(state->xy_clip[xy][0], 4 * idx, 4, cstat);
		if (is_o) {
			bool oob = coord < -0x8000 || coord >= 0x8000;
			if (which == 3)
				oob = false;
			if (which == 2) {
				oob = ovf;
			}
			insrt(state->xy_misc_4[xy], 0+idx, 1, 0);
			insrt(state->xy_misc_4[xy], 4+idx, 1, oob);
		}
		if (is_size) {
			if (which == 2 && prev_inited) {
				nv03_pgraph_vtx_cmp(state, xy, 5, false);
			} else {
				nv03_pgraph_vtx_cmp(state, xy, 8, true);
			}
		}
		if (idx) {
			insrt(state->xy_misc_1[is_o], 8+xy, 1,
				extr(state->xy_clip[xy][0], 2, 1) ||
				extr(state->xy_clip[xy][0], 4, 1));
			insrt(state->xy_misc_1[is_o], 12+xy*4, 1, !extr(state->xy_clip[xy][0], 0, 1));
			insrt(state->xy_misc_1[is_o], 13+xy*4, 1, !extr(state->xy_clip[xy][0], 6, 1));
		}
	}
}

void nv03_pgraph_vtx_add(struct nv03_pgraph_state *state, int xy, int idx, uint32_t a, uint32_t b, uint32_t c, bool noclip) {
	uint64_t val = (uint64_t)a + b + c;
	uint32_t ovval = val;
	if (extr(a, 31, 1) == extr(b, 31, 1) && extr(a, 31, 1) != extr(val, 31, 1)) {
		ovval = extr(a, 31, 1) ? 0x80000000 : 0x7fffffff;
	}
	if (xy == 0)
		state->vtx_x[idx] = ovval;
	else
		state->vtx_y[idx] = ovval;
	int oob = ((int32_t)val >= 0x8000 || (int32_t)val < -0x8000);
	int cstat = nv03_pgraph_clip_status(state, val, xy, noclip);
	nv03_pgraph_set_xym2(state, xy, idx, val >> 32 & 1, oob, cstat);
}

void nv03_pgraph_prep_draw(struct nv03_pgraph_state *state, bool poly, bool noclip) {
	int cls = extr(state->ctx_user, 16, 5);
	if (extr(state->cliprect_ctrl, 8, 1))
		insrt(state->intr, 24, 1, 1);
	if (extr(state->xy_misc_4[0], 4, 4) || extr(state->xy_misc_4[1], 4, 4))
		insrt(state->intr, 12, 1, 1);
	if (cls == 0xc) {
	} else {
		if (state->valid & 0x50000000 && extr(state->ctx_switch, 15, 1))
			insrt(state->intr, 16, 1, 1);
	}
	if (!extr(state->valid, 16, 1))
		insrt(state->intr, 16, 1, 1);
	if (extr(state->debug[3], 22, 1)) {
		bool passthru = extr(state->ctx_switch, 24, 5) == 0x17 && extr(state->ctx_switch, 13, 2) == 0;
		int msk = extr(state->ctx_switch, 20, 4);
		int cfmt = extr(state->ctx_switch, 0, 3);
		bool bad = false;
		int fmt = -1;
		for (int j = 0; j < 4; j++) {
			if (msk & 1 << j || (msk == 0 && j == 3)) {
				if (fmt == -1)
					fmt = extr(state->surf_format, 4*j, 3);
				else if (fmt != (int)extr(state->surf_format, 4*j, 3))
					bad = true;
			}
		}
		if (fmt == 0 && msk)
			bad = true;
		if ((fmt == 0 || fmt == 4) && (cfmt != 4 || !passthru || cls == 0xc))
			bad = true;
		if ((fmt == 5 || fmt == 1) && (cfmt != 3))
			bad = true;
		if (bad)
			insrt(state->intr, 20, 1, 1);
	}
	switch (cls) {
		case 8:
			if ((state->valid & 0x010101) != 0x010101)
				insrt(state->intr, 16, 1, 1);
			if (!(state->intr & 0x01111000)) {
				insrt(state->valid, 0, 16, 0);
				insrt(state->valid, 21, 1, 0);
			}
			break;
		case 9:
		case 0xa:
			if (!poly) {
				if ((state->valid & 0x210303) != 0x210303)
					insrt(state->intr, 16, 1, 1);
				if (!(state->intr & 0x01111000)) {
					insrt(state->valid, 0, 4, 0);
					insrt(state->valid, 8, 4, 0);
				}
			} else {
				if ((state->valid & 0x213030) != 0x213030)
					insrt(state->intr, 16, 1, 1);
			}
			break;
		case 0xb:
			if (!poly) {
				if ((state->valid & 0x210707) != 0x210707)
					insrt(state->intr, 16, 1, 1);
				if (!(state->intr & 0x01111000)) {
					insrt(state->valid, 0, 4, 0);
					insrt(state->valid, 8, 4, 0);
				}
			} else {
				if ((state->valid & 0x217070) != 0x217070)
					insrt(state->intr, 16, 1, 1);
			}
			break;
		case 0x7:
			if ((state->valid & 0x10203) != 0x10203)
				insrt(state->intr, 16, 1, 1);
			if ((uint32_t)extrs(state->vtx_x[1], 0, 16) != state->vtx_x[1])
				insrt(state->intr, 12, 1, 1);
			if ((uint32_t)extrs(state->vtx_y[1], 0, 16) != state->vtx_y[1])
				insrt(state->intr, 12, 1, 1);
			if (!(state->intr & 0x01111000)) {
				insrt(state->valid, 0, 16, 0);
				insrt(state->valid, 21, 1, 0);
			}
			break;
		case 0xc:
			if ((state->valid & 0x10203) != 0x10203)
				insrt(state->intr, 16, 1, 1);
			if ((uint32_t)extrs(state->vtx_x[1], 0, 16) != state->vtx_x[1])
				insrt(state->intr, 12, 1, 1);
			if ((uint32_t)extrs(state->vtx_y[1], 0, 16) != state->vtx_y[1])
				insrt(state->intr, 12, 1, 1);
			if (!(state->intr & 0x01111000)) {
				insrt(state->valid, 0, 2, 0);
				insrt(state->valid, 8, 2, 0);
				insrt(state->valid, 21, 1, 0);
			}
			break;
		default:
			abort();
	}
	if (state->intr)
		state->access = 0;
}
