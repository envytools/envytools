/*
 * Copyright (C) 2012-2016 Marcin Kościelnicki <koriakin@0x04.net>
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

void pgraph_set_xy_d(struct pgraph_state *state, int xy, int idx, int sid, bool carry, bool oob, bool ovf, int cstat) {
	sid &= 3;
	bool is_point = false;
	bool is_sifc = false;
	bool set_oob_carry = false;
	if (state->chipset.card_type < 3) {
		uint32_t class = extr(state->access, 12, 5);
		is_point = class == 8;
		set_oob_carry = true;
	} else if (state->chipset.card_type < 4) {
		int cls = extr(state->ctx_user, 16, 5);
		if ((cls >= 0x8 && cls <= 0xc) || cls == 0xe || (cls >= 0x10 && cls <= 0x12) || cls == 0x14 || cls == 0x15) {
			set_oob_carry = true;
		}
		is_sifc = cls == 0x15;
		is_point = cls == 0x08 || cls == 0x18;
	} else {
		int cls = extr(state->ctx_switch[0], 0, 8);
		switch (cls) {
			case 0x66:
				if (state->chipset.chipset < 5)
					break;
			/* SIFC */
			case 0x36:
			case 0x76:
				is_sifc = true;
				break;
		}
		set_oob_carry = nv04_pgraph_is_new_render_class(state);
	}

	if (is_sifc) {
		oob = ovf;
	}
	if (set_oob_carry) {
		insrt(state->xy_misc_4[xy], sid, 1, carry);
		insrt(state->xy_misc_4[xy], sid+4, 1, oob);
	}

	if (state->chipset.card_type < 3) {
		if (is_point) {
			insrt(state->xy_misc_4[xy], 8, 4, cstat);
			insrt(state->xy_misc_4[xy], 12, 4, cstat);
		} else if (idx < 16 || nv01_pgraph_use_v16(state)) {
			insrt(state->xy_misc_4[xy], 8 + sid * 4, 4, cstat);
		}
		if (idx >= 16) {
			insrt(state->edgefill, 16 + xy * 4 + (idx - 16) * 8, 4, cstat);
		}
	} else {
		if (is_point) {
			insrt(state->xy_clip[xy][0], 0, 4, cstat);
			insrt(state->xy_clip[xy][0], 4, 4, cstat);
		} else {
			insrt(state->xy_clip[xy][idx >> 3], (idx & 7) * 4, 4, cstat);
		}
	}
}

void nv01_pgraph_clip_bounds(struct pgraph_state *state, int32_t min[2], int32_t max[2]) {
	int i;
	for (i = 0; i < 2; i++) {
		min[i] = extrs(state->dst_canvas_min, i*16, 16);
		max[i] = extr(state->dst_canvas_max, i*16, 12);
		int sel = extr(state->xy_misc_1[0], 12+i*4, 3);
		bool uce = extr(state->ctx_switch[0], 7, 1);
		if (sel & 1 && uce)
			min[i] = state->uclip_min[0][i];
		if (sel & 2 && uce)
			max[i] = state->uclip_max[0][i];
		if (sel & 4)
			max[i] = state->iclip[i];
		if (min[i] < 0)
			min[i] = 0;
		min[i] &= 0xfff;
		max[i] &= 0xfff;
	}
}

int nv01_pgraph_clip_status(struct pgraph_state *state, int32_t coord, int xy, int is_tex_class) {
	int32_t clip_min[2], clip_max[2];
	nv01_pgraph_clip_bounds(state, clip_min, clip_max);
	int cstat = 0;
	if (is_tex_class) {
		coord = extrs(coord, 15, 16);
		if (extr(state->xy_misc_1[0], 25, 1))
			coord >>= 4;
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

int nv01_pgraph_use_v16(struct pgraph_state *state) {
	uint32_t class = extr(state->access, 12, 5);
	int d0_24 = extr(state->debug[0], 24, 1);
	int d1_8 = extr(state->debug[1], 8, 1);
	int d1_24 = extr(state->debug[1], 24, 1);
	int j;
	int sdl = 1;
	for (j = 0; j < 4; j++)
		if (extr(state->subdivide, (j/2)*4, 4) > extr(state->subdivide, 16+j*4, 4))
			sdl = 0;
	int biopt = d1_8 && !extr(state->xy_misc_4[0], 28, 2) && !extr(state->xy_misc_4[1], 28, 2);
	switch (class) {
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

void nv01_pgraph_vtx_fixup(struct pgraph_state *state, int xy, int idx, int32_t coord, int rel, int ridx, int sid) {
	bool is_tex_class = nv01_pgraph_is_tex_class(state);
	int32_t cbase = extrs(state->dst_canvas_min, 16 * xy, 16);
	if (ridx != -1)
		cbase = state->vtx_xy[ridx][xy];
	if (is_tex_class && state->xy_misc_1[0] & 1 << 25 && ridx == -1)
		cbase <<= 4;
	if (rel)
		coord += cbase;
	state->vtx_xy[idx][xy] = coord;
	int oob = !is_tex_class && (coord >= 0x8000 || coord < -0x8000);
	int cstat = nv01_pgraph_clip_status(state, coord, xy, is_tex_class);
	int carry = rel && (uint32_t)coord < (uint32_t)cbase;
	pgraph_set_xy_d(state, xy, idx, sid, carry, oob, false, cstat);
}

void nv01_pgraph_vtx_add(struct pgraph_state *state, int xy, int idx, int sid, uint32_t a, uint32_t b, uint32_t c) {
	bool is_tex_class = nv01_pgraph_is_tex_class(state);
	uint64_t val = (uint64_t)a + b + c;
	state->vtx_xy[idx][xy] = val;
	int oob = !is_tex_class && ((int32_t)val >= 0x8000 || (int32_t)val < -0x8000);
	int cstat = nv01_pgraph_clip_status(state, val, xy, is_tex_class);
	pgraph_set_xy_d(state, xy, sid, sid, val >> 32 & 1, oob, false, cstat);
}

void nv01_pgraph_iclip_fixup(struct pgraph_state *state, int xy, int32_t coord, int rel) {
	int is_tex_class = nv01_pgraph_is_tex_class(state);
	int max = extr(state->dst_canvas_max, xy * 16, 12);
	if (state->xy_misc_1[0] & 0x2000 << (xy * 4) && state->ctx_switch[0] & 0x80)
		max = state->uclip_max[0][xy] & 0xfff;
	int32_t cbase = extrs(state->dst_canvas_min, 16 * xy, 16);
	int32_t cmin = cbase;
	if (cmin < 0)
		cmin = 0;
	if (state->ctx_switch[0] & 0x80 && state->xy_misc_1[0] & 0x1000 << xy * 4)
		cmin = state->uclip_min[0][xy];
	cmin &= 0xfff;
	if (is_tex_class && state->xy_misc_1[0] & 1 << 25)
		cbase <<= 4;
	if (rel)
		coord += cbase;
	state->iclip[xy] = coord & 0x3ffff;
	if (is_tex_class) {
		coord = extrs(coord, 15, 16);
		if (state->xy_misc_1[0] & 0x02000000)
			coord >>= 4;
	} else {
		coord = extrs(coord, 0, 18);
	}
	insrt(state->xy_misc_1[0], 14+xy*4, 1, coord <= max);
	if (!xy) {
		insrt(state->xy_misc_1[0], 20, 1, coord <= max);
		insrt(state->xy_misc_1[0], 4, 1, coord < cmin);
	} else {
		insrt(state->xy_misc_1[0], 5, 1, coord < cmin);
	}
}

void nv01_pgraph_uclip_fixup(struct pgraph_state *state, int xy, int idx, int32_t coord, int rel) {
	int is_tex_class = nv01_pgraph_is_tex_class(state);
	int32_t cbase = extrs(state->dst_canvas_min, 16 * xy, 16);
	if (rel)
		coord += cbase;
	state->uclip_min[0][xy] = state->uclip_max[0][xy];
	state->uclip_max[0][xy] = coord & 0x3ffff;
	state->vtx_xy[15][xy] = coord;
	state->xy_misc_1[0] &= ~0x0177000;
	if (!idx) {
		insrt(state->xy_misc_1[0], 24, 1, 0);
	} else {
		int32_t cmin = extr(state->dst_canvas_min, 16 * xy, 12);
		if (extr(state->dst_canvas_min, 16 * xy + 15, 1))
			cmin = 0;
		int32_t cmax = extr(state->dst_canvas_max, 16 * xy, 12);
		int32_t ucmin = extrs(state->uclip_min[0][xy], 0, 18);
		int32_t ucmax = extrs(state->uclip_max[0][xy], 0, 18);
		if (is_tex_class && extr(state->xy_misc_1[0], 25, 1))
			ucmin >>= 4, ucmax >>= 4;
		insrt(state->xy_misc_1[0], 4 + xy, 1, 0);
		insrt(state->xy_misc_1[0], 8 + xy, 1,
			ucmax < cmin || (extr(state->xy_misc_4[xy], 10, 1) && ucmax >= 0));
		insrt(state->xy_misc_1[0], 12 + xy * 4, 1, !extr(state->xy_misc_4[xy], 8, 1));
		insrt(state->xy_misc_1[0], 13 + xy * 4, 1, ucmax <= cmax);
	}
}

void nv01_pgraph_set_clip(struct pgraph_state *state, int is_size, uint32_t val) {
	uint32_t class = extr(state->access, 12, 5);
	int is_tex_class = nv01_pgraph_is_tex_class(state);
	int is_tex = is_tex_class && !is_size;
	int xy;
	if (is_size) {
		int n = state->valid[0] >> 24 & 1;
		state->valid[0] &= ~0x11000000;
		if (!n)
			state->valid[0] |= 1 << 28;
		state->xy_misc_1[0] &= 0x03000001;
		if (class == 0x14) {
			pgraph_set_image_zero(state, !extr(val, 0, 16) || !extr(val, 16, 16));
		}
	} else {
		if (is_tex_class && (state->xy_misc_1[0] >> 24 & 3) == 3) {
			state->valid[0] = 0;
			state->xy_misc_1[0] &= ~0x02000000;
		}
		state->valid[0] &= ~0x11000000;
		state->valid[0] |= 1 << 24;
		state->xy_misc_1[0] &= 0x00000330;
		state->xy_misc_1[0] |= 0x01000000;
	}
	for (xy = 0; xy < 2; xy++) {
		int32_t new = (uint16_t)(val >> xy * 16);
		int32_t cbase = (int16_t)(state->dst_canvas_min >> 16 * xy);
		uint32_t base;
		int svidx = is_tex ? 4 : 15;
		int dvidx = svidx;
		int mvidx = is_size;
		if ((class == 0xc || class == 0x11 || class == 0x12 || class == 0x13) && is_size) {
			svidx = 0;
			dvidx = pgraph_vtxid(state);
			mvidx = dvidx%4;
		}
		if (class == 0x14 && is_size) {
			svidx = 0;
			dvidx = 2;
			mvidx = dvidx%4;
			state->vtx_xy[3][xy] = extr(val, xy * 16, 16);
		}
		if (is_tex_class && state->xy_misc_1[0] & 1 << 25)
			cbase <<= 4;
		if (is_size) {
			base = state->vtx_xy[svidx][xy];
		} else {
			new = (int16_t)new;
			base = cbase;
		}
		new += base;
		uint32_t m2 = state->xy_misc_4[xy];
		int32_t vcoord = new;
		if (is_tex)
			vcoord <<= 15, vcoord |= 1 << 14;
		state->vtx_xy[dvidx][xy] = vcoord;
		state->uclip_min[0][xy] = state->uclip_max[0][xy];
		state->uclip_max[0][xy] = new & 0x3ffff;
		int cstat = nv01_pgraph_clip_status(state, new, xy, is_tex_class && is_size);
		int oob = (new >= 0x8000 || new < -0x8000);
		if (is_tex_class) {
			if (is_size) {
				oob = 0;
			} else {
				oob |= m2 >> (mvidx + 4) & 1;
			}
		}
		int carry = ((uint32_t)new < (uint32_t)base);
		pgraph_set_xy_d(state, xy, mvidx, mvidx, carry, oob, false, cstat);
		if (is_size) {
			int32_t cmin = state->dst_canvas_min >> 16 * xy & 0x8fff;
			if (cmin & 0x8000)
				cmin = 0;
			int32_t cmax = state->dst_canvas_max >> 16 * xy & 0x0fff;
			int32_t ucmax = sext(state->uclip_max[0][xy], 17);
			if (is_tex_class)
				ucmax = extrs(vcoord, 15, 16);
			if (is_tex_class && state->xy_misc_1[0] & 1 << 25)
				ucmax >>= 4;
			state->xy_misc_1[0] |= (ucmax < cmin) << (8 + xy);
			if (m2 >> 10 & 1)
				state->xy_misc_1[0] |= (ucmax >= 0) << (8 + xy);
			if (!(m2 >> 8 & 1))
				state->xy_misc_1[0] |= 1 << (12 + xy * 4);
			state->xy_misc_1[0] |= (ucmax <= cmax) << (13 + xy * 4);
		}
	}
}

void nv01_pgraph_set_vtx(struct pgraph_state *state, int xy, int idx, int32_t coord, bool is32) {
	bool is_tex_class = nv01_pgraph_is_tex_class(state);
	int sid = idx & 3;
	int32_t cbase = extrs(state->dst_canvas_min, 16 * xy, 16);
	int fract = is_tex_class && extr(state->xy_misc_1[0], 25, 1);
	if (fract)
		cbase <<= 4;
	coord += cbase;
	int carry = (uint32_t)coord < (uint32_t)cbase;
	int oob = (coord >= 0x8000 || coord < -0x8000);
	if (is_tex_class)
		oob |= extr(state->xy_misc_4[xy], sid+4, 1);
	uint32_t val = (is_tex_class && !is32) ? coord << 15 | 0x4000 : coord;
	state->vtx_xy[idx][xy] = val;
	int cstat = nv01_pgraph_clip_status(state, is32 ? coord : extrs(coord, fract * 4, 18 - fract * 4), xy, is_tex_class && is32);
	pgraph_set_xy_d(state, xy, idx, idx, carry, oob, false, cstat);
}

uint32_t pgraph_vtxid(struct pgraph_state *state) {
	return extr(state->xy_a, 28, 4);
}

void pgraph_clear_vtxid(struct pgraph_state *state) {
	insrt(state->xy_a, 28, 4, 0);
}

void pgraph_bump_vtxid(struct pgraph_state *state) {
	uint32_t class = pgraph_class(state);
	if (state->chipset.card_type < 3) {
		if (class < 8 || class == 0x0f || (class > 0x14 && class < 0x1d) || class == 0x1f)
			return;
	}
	int vtxid = extr(state->xy_a, 28, 4);
	vtxid++;
	if (state->chipset.card_type < 4) {
		if (class == 0x0b) {
			if (vtxid == 3)
				vtxid = 0;
		} else if (class == 0x10 || class == 0x14) {
			if (vtxid == 4)
				vtxid = 0;
		} else if (nv01_pgraph_is_tex_class(state)) {
			if (vtxid == 3 + nv01_pgraph_use_v16(state))
				vtxid = 0;
		} else {
			vtxid &= 1;
		}
	} else {
		if (class == 0x1d || class == 0x5d) {
			if (vtxid == 3)
				vtxid = 0;
		} else if (class == 0x1f || class == 0x5f || class == 0x9f) {
			if (vtxid == 4)
				vtxid = 0;
		} else if (class == 0x8a && extr(state->debug[3], 16, 1)) {
			vtxid = 0;
		} else {
			vtxid &= 1;
		}
	}
	insrt(state->xy_a, 28, 4, vtxid);
}

bool pgraph_image_zero(struct pgraph_state *state) {
	if (state->chipset.card_type < 3) {
		return extr(state->xy_a, 12, 1);
	} else {
		return extr(state->xy_a, 20, 1);
	}
}

void pgraph_set_image_zero(struct pgraph_state *state, bool val) {
	if (state->chipset.card_type < 3) {
		insrt(state->xy_a, 12, 1, val);
	} else {
		insrt(state->xy_a, 20, 1, val);
	}
}
