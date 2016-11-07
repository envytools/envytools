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

bool nv04_pgraph_is_oclip_class(struct nv04_pgraph_state *state) {
	int cls = extr(state->ctx_switch[0], 0, 8);
	switch (cls) {
		/* SIFC */
		case 0x36:
		case 0x76:
		/* SIFM */
		case 0x37:
		case 0x77:
		/* GDI */
		case 0x4a:
		case 0x4b:
			return true;
		case 0x66:
			return state->chipset.chipset >= 5;
	}
	return false;
}

void nv04_pgraph_clip_bounds(struct nv04_pgraph_state *state, int32_t min[2], int32_t max[2]) {
	if (nv04_pgraph_is_oclip_class(state)) {
		min[0] = state->oclip_min[0];
		min[1] = state->oclip_min[1];
		max[0] = state->oclip_max[0];
		max[1] = state->oclip_max[1];
	} else {
		min[0] = state->uclip_min[0];
		min[1] = state->uclip_min[1];
		max[0] = state->uclip_max[0];
		max[1] = state->uclip_max[1];
	}
	min[0] = sext(min[0], 15);
	min[1] = sext(min[1], 15);
	max[0] = sext(max[0], 17);
	max[1] = sext(max[1], 17);
}

int nv04_pgraph_clip_status(struct nv04_pgraph_state *state, int32_t coord, int xy, bool canvas_only) {
	int cstat = 0;
	int cls = extr(state->ctx_switch[0], 0, 8);
	int32_t clip_min[2], clip_max[2];
	nv04_pgraph_clip_bounds(state, clip_min, clip_max);
	if (cls == 0x48 || cls == 0x54 || cls == 0x55) {
		coord = extrs(coord, 4, 12);
	} else {
		coord = extrs(coord, 0, 18);
	}
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

void nv04_pgraph_set_xym2(struct nv04_pgraph_state *state, int xy, int idx, bool carry, bool oob, int cstat) {
	int cls = extr(state->ctx_switch[0], 0, 8);
	switch (cls) {
		case 0x64:
		case 0x65:
			if (state->chipset.chipset < 5)
				break;
		/* LIN */
		case 0x1c:
		case 0x5c:
		/* TRI */
		case 0x1d:
		case 0x5d:
		/* BLIT */
		case 0x1f:
		case 0x5f:
		/* IIFC */
		case 0x60:
		/* IFC */
		case 0x21:
		case 0x61:
		/* GDI */
		case 0x4a:
		case 0x4b:
		/* SIFM */
		case 0x37:
		case 0x77:
		/* DVD */
		case 0x38:
			insrt(state->xy_misc_4[xy], (idx&3), 1, carry);
			insrt(state->xy_misc_4[xy], (idx&3)+4, 1, oob);
			break;
		case 0x66:
			if (state->chipset.chipset < 5)
				break;
		/* SIFC */
		case 0x36:
		case 0x76:
			insrt(state->xy_misc_4[xy], (idx&3), 1, 0);
			insrt(state->xy_misc_4[xy], (idx&3)+4, 1, 0);
			break;
	}
	insrt(state->xy_clip[xy][idx >> 3], (idx & 7) * 4, 4, cstat);
}

void nv04_pgraph_vtx_fixup(struct nv04_pgraph_state *state, int xy, int idx, int32_t coord) {
	int cstat = nv04_pgraph_clip_status(state, coord, xy, false);
	int oob = (coord >= 0x8000 || coord < -0x8000);
	int carry = 0;
	nv04_pgraph_set_xym2(state, xy, idx, carry, oob, cstat);
}

void nv04_pgraph_iclip_fixup(struct nv04_pgraph_state *state, int xy, int32_t coord) {
	int32_t clip_min[2], clip_max[2];
	nv04_pgraph_clip_bounds(state, clip_min, clip_max);
	int cls = extr(state->ctx_switch[0], 0, 8);
	if (cls == 0x48 || cls == 0x54 || cls == 0x55) {
		coord = extrs(coord, 4, 12);
	} else {
		coord = extrs(coord, 0, 18);
	}
	for (int i = 0; i < 2; i++) {
		bool ce = extr(state->ctx_switch[0], 13, 1) || i;
		bool ok = coord <= clip_max[xy] || !ce;
		insrt(state->xy_misc_1[i], 12+xy*4, 1, ok);
		if (!xy) {
			insrt(state->xy_misc_1[i], 20, 1, ok);
		}
		if (ce)
			insrt(state->xy_misc_1[i], 4+xy, 1, coord < clip_min[xy]);
	}
}

void nv04_pgraph_uclip_write(struct nv04_pgraph_state *state, int uo, int xy, int idx, int32_t coord) {
	uint32_t *umin = uo ? state->oclip_min : state->uclip_min;
	uint32_t *umax = uo ? state->oclip_max : state->uclip_max;
	umin[xy] = umax[xy] & 0xffff;
	umax[xy] = coord & 0x3ffff;
	insrt(state->xy_misc_1[uo], 12, 1, 0);
	insrt(state->xy_misc_1[uo], 16, 1, 0);
	insrt(state->xy_misc_1[uo], 20, 1, 0);
	if (idx)
		insrt(state->xy_misc_1[uo], 4+xy, 1, 0);
	(xy ? state->vtx_y : state->vtx_x)[13] = coord;
	int cls = extr(state->ctx_switch[0], 0, 8);
	if (cls == 0x53 && state->chipset.chipset == 4 && uo == 0)
		umin[xy] = 0;
}

uint32_t nv04_pgraph_formats(struct nv04_pgraph_state *state) {
	int cls = extr(state->ctx_switch[0], 0, 8);
	uint32_t src = extr(state->ctx_switch[1], 8, 8);
	uint32_t surf = extr(state->surf_format, 0, 4);
	bool dither = extr(state->debug[3], 12, 1);
	if (state->chipset.chipset >= 5) {
		int op = extr(state->ctx_switch[0], 15, 3);
		bool op_blend = op == 2 || op == 4 || op == 5;
		bool chroma = extr(state->ctx_switch[0], 12, 1);
		bool new_class = false;
		switch (cls) {
			case 0x42: /* SURF2D */
			case 0x44: /* PATTERN_NV4 */
			case 0x4a: /* GDI_NV4 */
			case 0x52: /* SWZSURF */
			case 0x53: /* SURF3D */
			case 0x57: /* CHROMA_NV4 */
			case 0x5c: /* LIN_NV4 */
			case 0x5d: /* TRI_NV4 */
			case 0x5e: /* RECT_NV4 */
			case 0x60: /* IIFC_NV4 */
			case 0x61: /* IFC_NV4 */
			case 0x64: /* IIFC_NV5 */
			case 0x65: /* IFC_NV5 */
			case 0x66: /* SIFC_NV5 */
			case 0x72: /* BETA4 */
			case 0x76: /* SIFC_NV4 */
			case 0x77: /* SIFM_NV4 */
				new_class = true;
				break;
		}
		bool chroma_zero = false;
		if (!new_class) {
			chroma_zero = extr(state->chroma, 24, 8) == 0;
		} else {
			switch (extr(state->ctx_format, 24, 8)) {
				case 2:
					chroma_zero = extr(state->chroma, 8, 8) == 0;
					break;
				case 6:
				case 8:
					chroma_zero = extr(state->chroma, 15, 1) == 0;
					break;
				case 0xb:
				case 0xd:
				case 0x10:
					chroma_zero = extr(state->chroma, 24, 8) == 0;
					break;
			}
		}
		bool cls_nopatch = cls == 0x38 || cls == 0x39;
		dither = (op_blend || (chroma && !chroma_zero)) && !cls_nopatch;
		if (extr(state->debug[2], 0, 1))
			dither = true;
	}
	if (cls == 0x1f || cls == 0x5f) {
		switch (surf) {
			case 0:
			default:
				src = 0xe;
				break;
			case 1:
				src = 1;
				break;
			case 2:
			case 3:
			case 4:
				src = 7;
				break;
			case 5:
				src = 0xa;
				break;
			case 6:
				src = 0xf;
				break;
			case 0xd:
				src = 0x14;
				break;
		}
	}
	uint32_t src_ov = src;
	if (cls == 0x48 || cls == 0x54 || cls == 0x55) {
		dither = false;
		surf = extr(state->surf_format, 8, 4);
		src = src_ov = 0xd;
	}
	if (cls == 0x37 || cls == 0x77) {
		src_ov = 0xd;
		if (extr(state->ctx_switch[0], 14, 1))
			surf = extr(state->surf_format, 20, 4);
	}
	if (cls == 0x60 || (state->chipset.chipset >= 5 && cls == 0x64)) {
		src_ov = 0xd;
		if (extr(state->ctx_switch[0], 14, 1) && state->chipset.chipset >= 5)
			surf = extr(state->surf_format, 20, 4);
	}
	if (cls == 0x38) {
		surf = extr(state->surf_format, 16, 4);
		src_ov = 0x13;
	}
	uint32_t rop = 0;
	if (surf == 1) {
		rop = 0;
	} else if (surf == 6) {
		rop = 3;
	} else if (surf == 0xd) {
		rop = 7;
	} else if (surf >= 0xe) {
		rop = 3;
	} else if (surf >= 7) {
		rop = 5;
	} else {
		if (src == 6 || src == 7 || src == 8 || src == 9) {
			if (surf == 5)
				rop = 2;
			else
				rop = 1;
		} else if (src == 0xa || src == 0xb || src == 0xc || src == 0xf || src == 0x12 || src == 0x13) {
			rop = 2;
		} else {
			if (dither || surf == 0) {
				rop = 5;
			} else {
				if (surf == 5)
					rop = 2;
				else
					rop = 1;
			}
		}
	}
	return surf << 12 | src_ov << 4 | rop;
}
