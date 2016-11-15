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

bool nv04_pgraph_is_3d_class(struct nv04_pgraph_state *state) {
	int cls = extr(state->ctx_switch[0], 0, 8);
	bool alt = extr(state->debug[3], 16, 1) && state->chipset.card_type >= 0x10;
	switch (cls) {
		case 0x48:
			return state->chipset.chipset <= 0x10;
		case 0x54:
		case 0x55:
			return true;
		case 0x56:
			return state->chipset.card_type >= 0x10 && !alt;
		case 0x94:
		case 0x95:
			return state->chipset.card_type >= 0x10;
		case 0x96:
			return nv04_pgraph_is_nv15p(&state->chipset);
		case 0x98:
		case 0x99:
			return nv04_pgraph_is_nv17p(&state->chipset);
		case 0x85:
			return alt;
	}
	return false;
}

bool nv04_pgraph_is_clip3d_class(struct nv04_pgraph_state *state) {
	int cls = extr(state->ctx_switch[0], 0, 8);
	bool alt = extr(state->debug[3], 16, 1) && state->chipset.card_type >= 0x10;
	if (state->chipset.card_type != 0x10)
		return false;
	switch (cls) {
		case 0x56:
			return !alt;
		case 0x85:
			return alt;
		case 0x96:
			return nv04_pgraph_is_nv15p(&state->chipset);
		case 0x98:
		case 0x99:
			return nv04_pgraph_is_nv17p(&state->chipset);
	}
	return false;
}

bool nv04_pgraph_is_oclip_class(struct nv04_pgraph_state *state) {
	int cls = extr(state->ctx_switch[0], 0, 8);
	bool alt = extr(state->debug[3], 16, 1) && state->chipset.card_type >= 0x10;
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
		case 0x63:
		case 0x89:
		case 0x7b:
			return state->chipset.card_type >= 0x10 && !alt;
		case 0x87:
		case 0x79:
		case 0x67:
			return alt;
	}
	return false;
}

bool nv04_pgraph_is_new_render_class(struct nv04_pgraph_state *state) {
	int cls = extr(state->ctx_switch[0], 0, 8);
	bool alt = extr(state->debug[3], 16, 1) && state->chipset.card_type >= 0x10;
	switch (cls) {
			return true;
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
			return true;
		case 0x64:
		case 0x65:
			return state->chipset.chipset >= 5;
		/* SIFC */
		case 0x36:
		case 0x76:
			return true;
		case 0x66:
			return state->chipset.chipset >= 5;
		case 0x63:
		case 0x8a:
		case 0x89:
		case 0x7b:
			return state->chipset.card_type >= 0x10 && !alt;
		case 0x87:
		case 0x67:
		case 0x79:
			return alt;
		case 0x88:
			return state->chipset.card_type >= 0x10;
		case 0x9f:
			return state->chipset.chipset > 0x10;
	}
	return false;
}

void nv04_pgraph_clip_bounds(struct nv04_pgraph_state *state, int32_t min[2], int32_t max[2]) {
	if (nv04_pgraph_is_clip3d_class(state)) {
		min[0] = state->clip3d_min[0];
		min[1] = state->clip3d_min[1];
		max[0] = state->clip3d_max[0];
		max[1] = state->clip3d_max[1];
	} else if (nv04_pgraph_is_oclip_class(state)) {
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

int nv04_pgraph_clip_status(struct nv04_pgraph_state *state, int32_t coord, int xy) {
	int cstat = 0;
	int32_t clip_min[2], clip_max[2];
	nv04_pgraph_clip_bounds(state, clip_min, clip_max);
	if (nv04_pgraph_is_3d_class(state)) {
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
		case 0x66:
			if (state->chipset.chipset < 5)
				break;
		/* SIFC */
		case 0x36:
		case 0x76:
			carry = oob = false;
			break;
	}
	if (nv04_pgraph_is_new_render_class(state)) {
		insrt(state->xy_misc_4[xy], (idx&3), 1, carry);
		insrt(state->xy_misc_4[xy], (idx&3)+4, 1, oob);
	}
	insrt(state->xy_clip[xy][idx >> 3], (idx & 7) * 4, 4, cstat);
}

void nv04_pgraph_vtx_fixup(struct nv04_pgraph_state *state, int xy, int idx, int32_t coord) {
	int cstat = nv04_pgraph_clip_status(state, coord, xy);
	int oob = (coord >= 0x8000 || coord < -0x8000);
	int carry = 0;
	nv04_pgraph_set_xym2(state, xy, idx, carry, oob, cstat);
}

void nv04_pgraph_iclip_fixup(struct nv04_pgraph_state *state, int xy, int32_t coord) {
	int32_t clip_min[2], clip_max[2];
	nv04_pgraph_clip_bounds(state, clip_min, clip_max);
	if (nv04_pgraph_is_3d_class(state)) {
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

void nv04_pgraph_uclip_write(struct nv04_pgraph_state *state, int which, int xy, int idx, int32_t coord) {
	uint32_t *umin;
	uint32_t *umax;
	if (which == 0) {
		umin = state->uclip_min;
		umax = state->uclip_max;
	} else if (which == 1) {
		umin = state->oclip_min;
		umax = state->oclip_max;
	} else {
		umin = state->clip3d_min;
		umax = state->clip3d_max;
	}
	umin[xy] = umax[xy] & 0xffff;
	umax[xy] = coord & 0x3ffff;
	if (which < 2) {
		insrt(state->xy_misc_1[which], 12, 1, 0);
		insrt(state->xy_misc_1[which], 16, 1, 0);
		insrt(state->xy_misc_1[which], 20, 1, 0);
		if (idx)
			insrt(state->xy_misc_1[which], 4+xy, 1, 0);
	}
	int vidx = state->chipset.card_type < 0x10 ? 13 : 9;
	(xy ? state->vtx_y : state->vtx_x)[vidx] = coord;
	int cls = extr(state->ctx_switch[0], 0, 8);
	if (cls == 0x53 && state->chipset.chipset == 4 && which == 0)
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
		bool alt = extr(state->debug[3], 16, 1) && state->chipset.card_type >= 0x10;
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
			case 0x62:
			case 0x63:
			case 0x7b:
			case 0x89:
			case 0x8a:
				new_class = state->chipset.card_type >= 0x10 && !alt;
				break;
			case 0x67:
			case 0x79:
			case 0x82:
			case 0x87:
				new_class = state->chipset.card_type >= 0x10 && alt;
				break;
			case 0x93:
				new_class = state->chipset.card_type >= 0x10;
				break;
			case 0x9e:
				new_class = state->chipset.chipset > 0x10;
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
		bool cls_nopatch = cls == 0x38 || cls == 0x39 || (cls == 0x88 && state->chipset.card_type >= 0x10);
		dither = (op_blend || (chroma && !chroma_zero)) && !cls_nopatch;
		if (state->chipset.card_type < 0x10 && extr(state->debug[2], 0, 1))
			dither = true;
		if (state->chipset.card_type >= 0x10 && !extr(state->debug[2], 25, 1)) {
			dither = true;
	}
	}
	bool checkswz = state->chipset.card_type >= 0x10;
	if (nv04_pgraph_is_3d_class(state)) {
		dither = false;
		surf = extr(state->surf_format, 8, 4);
		src = 0xd;
		checkswz = false;
	}
	uint32_t src_ov = 0;
	bool is_sifm = cls == 0x37 || cls == 0x77;
	bool alt = extr(state->debug[3], 16, 1) && state->chipset.card_type >= 0x10;
	if (state->chipset.card_type >= 0x10 && (cls == 0x63 || cls == 0x89) && !alt)
		is_sifm = true;
	if (state->chipset.card_type >= 0x10 && (cls == 0x67 || cls == 0x87) && alt)
		is_sifm = true;
	if (is_sifm) {
		src_ov = 0xd;
		checkswz = true;
	}
	if (cls == 0x60 || (state->chipset.chipset >= 5 && cls == 0x64)) {
		src_ov = 0xd;
		checkswz = state->chipset.chipset >= 5;;
	}
	if (extr(state->ctx_switch[0], 14, 1) && checkswz)
		surf = extr(state->surf_format, 20, 4);
	if (cls == 0x38 || (state->chipset.card_type >= 0x10 && cls == 0x88)) {
		surf = extr(state->surf_format, 16, 4);
		src_ov = 0x13;
	}
	if (state->chipset.card_type >= 0x10 && !nv04_pgraph_is_3d_class(state)) {
		switch (surf) {
			case 1:
				src = 1;
				break;
			case 6:
				src = 0xf;
				break;
			case 0xd:
				src = 0x14;
				break;
		}
	}
	bool is_blit = cls == 0x1f || cls == 0x5f;
	if (state->chipset.chipset >= 0x11 && cls == 0x9f)
		is_blit = true;
	if (is_blit) {
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
		uint32_t src5 = src & 0x1f, src4 = src & 0xf;
		if (state->chipset.card_type < 0x10) {
			if (src == 6 || src == 7 || src == 8 || src == 9) {
				rop = 1;
			} else if (src == 0xa || src == 0xb || src == 0xc || src == 0xf) {
				rop = 2;
			} else if (src == 0x12 || src == 0x13) {
				rop = 2;
			} else {
				rop = 5;
			}
		} else if (!nv04_pgraph_is_nv11p(&state->chipset)) {
			if (src4 == 6 || src4 == 7 || src4 == 8 || src4 == 9) {
				rop = 1;
			} else if (src4 == 0xa || src4 == 0xb || src4 == 0xc || src4 == 0xf) {
				rop = 2;
			} else if (src4 == 0 || src4 == 4 || src4 == 5) {
				rop = 0;
			} else {
				rop = 5;
			}
		} else {
			if (src5 == 6 || src5 == 7 || src5 == 8 || src5 == 9) {
				rop = 1;
			} else if (src4 == 0xa || src4 == 0xb || src4 == 0xc || src4 == 0xf) {
				rop = 2;
			} else if (src5 == 1 || src4 == 2 || src4 == 3 || src5 == 0x16 || src5 == 0x17 || src5 == 0xd || src5 == 0xe) {
				rop = 5;
			} else {
				rop = 0;
			}
		}
		if (rop == 5 && !dither && surf != 0) {
			rop = 1;
		}
		if (rop == 1 && surf == 5)
			rop = 2;
	}
	return surf << 12 | (src_ov ? src_ov : src) << 4 | rop;
}

void nv04_pgraph_volatile_reset(struct nv04_pgraph_state *state) {
	state->xy_misc_0 = 0;
	state->xy_misc_1[0] = 0;
	state->xy_misc_1[1] &= 1;
	state->xy_misc_3 &= ~0x11;
	state->xy_misc_4[0] = 0;
	state->xy_misc_4[1] = 0;
	state->xy_clip[0][0] = 0x55555555;
	state->xy_clip[0][1] = 0x55555555;
	state->xy_clip[1][0] = 0x55555555;
	state->xy_clip[1][1] = 0x55555555;
	state->valid[0] &= 0xf0000000;
	state->valid[1] &= 0xc0000000;
	state->misc32[0] = 0;
	state->dvd_format = 0;
	state->notify &= ~0x10000;
	if (state->chipset.card_type >= 0x10) {
		state->valid[0] &= 0x50000000;
		state->oclip_min[0] = 0;
		state->oclip_min[1] = 0;
		state->oclip_max[0] = 0xffff;
		state->oclip_max[1] = 0xffff;
	}
}

void nv04_pgraph_blowup(struct nv04_pgraph_state *state, uint32_t nsource) {
	uint32_t nstatus = 0;
	if (nsource & 0x1000)
		nstatus |= 0x0800;
	if (nsource & 0x4a00)
		nstatus |= 0x1000;
	if (nsource & 0x0002)
		nstatus |= 0x2000;
	if (nsource & 0x0044)
		nstatus |= 0x4000;
	state->fifo_enable = 0;
	state->intr |= 1;
	state->nsource |= nsource;
	state->nstatus |= nstatus;
}

uint32_t nv04_pgraph_expand_nv01_ctx_color(struct nv04_pgraph_state *state, uint32_t val) {
	int fmt = extr(state->ctx_switch[1], 8, 8);
	uint32_t res = val;
	switch (fmt) {
		case 0:
			if (extr(state->debug[3], 28, 1))
				nv04_pgraph_blowup(state, 0x800);
			return res;
		case 2:
			insrt(res, 0, 8, extr(val, 0, 8));
			insrt(res, 8, 8, extr(val, 0, 8));
			insrt(res, 16, 8, extr(val, 0, 8));
			insrt(res, 24, 8, extr(val, 8, 8));
			return res;
		case 3:
			insrt(res, 0, 8, extr(val, 0, 8));
			insrt(res, 8, 8, extr(val, 0, 8));
			insrt(res, 16, 8, extr(val, 0, 8));
			insrt(res, 24, 8, 1);
			return res;
		case 8:
			insrt(res, 0, 8, extr(val, 0, 5) << 3);
			insrt(res, 8, 8, extr(val, 5, 5) << 3);
			insrt(res, 16, 8, extr(val, 10, 5) << 3);
			insrt(res, 24, 8, extr(val, 15, 1) * 0xff);
			return res;
		case 9:
			insrt(res, 0, 8, extr(val, 0, 5) << 3);
			insrt(res, 8, 8, extr(val, 5, 5) << 3);
			insrt(res, 16, 8, extr(val, 10, 5) << 3);
			insrt(res, 24, 8, 1);
			return res;
		case 0xe:
			insrt(res, 0, 24, extr(val, 0, 24));
			insrt(res, 24, 8, 1);
			return res;
		default:
			return res;
	}
}

void nv04_pgraph_set_chroma_nv01(struct nv04_pgraph_state *state, uint32_t val) {
	int fmt = extr(state->ctx_switch[1], 8, 8);
	state->chroma = nv04_pgraph_expand_nv01_ctx_color(state, val);
	insrt(state->ctx_valid, 16, 1, fmt != 0);
	insrt(state->ctx_format, 24, 8, fmt);
}

void nv04_pgraph_set_pattern_mono_color_nv01(struct nv04_pgraph_state *state, int idx, uint32_t val) {
	int fmt = extr(state->ctx_switch[1], 8, 8);
	state->pattern_mono_color[idx] = nv04_pgraph_expand_nv01_ctx_color(state, val);
	insrt(state->ctx_valid, 24+idx, 1, !extr(state->nsource, 1, 1));
	insrt(state->ctx_format, 8+idx*8, 8, fmt);
}

void nv04_pgraph_set_bitmap_color_0_nv01(struct nv04_pgraph_state *state, uint32_t val) {
	int fmt = extr(state->ctx_switch[1], 8, 8);
	state->bitmap_color_0 = nv04_pgraph_expand_nv01_ctx_color(state, val);
	insrt(state->ctx_format, 0, 8, fmt);
}

uint32_t nv04_pgraph_expand_mono(struct nv04_pgraph_state *state, uint32_t mono) {
	uint32_t res = mono;
	if (extr(state->ctx_switch[1], 0, 2) == 1) {
		for (int i = 0; i < 0x20; i++)
			insrt(res, i ^ 7, 1, extr(mono, i, 1));
	}
	return res;
}

void nv04_pgraph_set_clip(struct nv04_pgraph_state *state, int which, int idx, uint32_t val) {
	bool is_size = which < 3 && idx == 1;
	bool is_o = which > 0;
	uint32_t *umin = is_o ? state->oclip_min : state->uclip_min;
	uint32_t *umax = is_o ? state->oclip_max : state->uclip_max;
	if (idx) {
		insrt(state->valid[0], 30 + is_o, 1, !extr(state->valid[0], 28 + is_o, 1));
		insrt(state->valid[0], 28 + is_o, 1, 0);
		state->xy_misc_1[is_o] &= ~0x00000330;
	} else {
		insrt(state->valid[0], 28 + is_o, 1, 1);
		insrt(state->valid[0], 30 + is_o, 1, 0);
		insrt(state->valid[0], 19, 1, 0);
		insrt(state->xy_misc_1[1], 0, 1, which != 2);
		insrt(state->xy_misc_3, 8, 1, 0);
	}
	if (!is_size)
		insrt(state->xy_misc_1[0], 0, 1, 0);
	insrt(state->xy_misc_1[is_o], 12, 1, 0);
	insrt(state->xy_misc_1[is_o], 16, 1, 0);
	insrt(state->xy_misc_1[is_o], 20, 1, 0);
	if (is_o && (state->chipset.chipset < 5 || extr(state->valid[0], 29, 1)))
		insrt(state->valid[0], 20, 1, 1);
	for (int xy = 0; xy < 2; xy++) {
		int32_t coord;
		int32_t base = xy ? state->vtx_y[13] : state->vtx_x[13];
		int32_t orig = extr(val, xy*16, 16);
		bool ovf = false;
		if (is_size) {
			coord = orig + base;
			if (extr(base, 31, 1) == extr(orig, 31, 1) &&
				extr(base, 31, 1) != extr(coord, 31, 1)) {
				coord = extr(coord, 31, 1) ? 0x7fffffff : 0x80000000;
				ovf = true;
			}
		} else {
			coord = extrs(val, xy  * 16, 16);
		}
		(xy ? state->vtx_y : state->vtx_x)[13] = coord;
		int cstat = nv04_pgraph_clip_status(state, coord, xy);
		insrt(state->xy_clip[xy][0], 4 * idx, 4, cstat);
		umin[xy] = umax[xy] & 0xffff;
		umax[xy] = coord & 0x3ffff;
		if (is_o) {
			bool oob = coord < -0x8000 || coord >= 0x8000;
			if (which == 2)
				oob = ovf;
			insrt(state->xy_misc_4[xy], 0+idx, 1, 0);
			insrt(state->xy_misc_4[xy], 4+idx, 1, oob);
		}
	}
}
