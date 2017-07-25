/*
 * Copyright (C) 2017 Marcin Kościelnicki <koriakin@0x04.net>
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

bool pgraph_in_begin_end(struct pgraph_state *state) {
	if (state->chipset.card_type == 0x20) {
		return extr(state->kelvin_unkf5c, 0, 1);
	} else if (state->chipset.card_type == 0x30) {
		return extr(state->rankine_unkf5c, 0, 1);
	} else {
		abort();
	}
}

void pgraph_kelvin_clear_idx(struct pgraph_state *state) {
	if (!nv04_pgraph_is_rankine_class(state)) {
		insrt(state->idx_state_b, 10, 6, 0);
	}
	int first = 0;
	if (nv04_pgraph_is_celsius_class(state)) {
		int xlat[8] = {0, 3, 4, 9, 0xa, 2, 1, 5};
		for (int i = 7; i > 0; i--)
			if (extr(state->idx_state_c, xlat[i], 1)) {
				first = xlat[i];
				break;
			}
	} else {
		for (int i = 0; i < 16; i++)
			if (extr(state->idx_state_c, i, 1)) {
				first = i;
				break;
			}
	}
	insrt(state->idx_state_c, 16, 4, first);
	insrt(state->idx_state_c, 20, 2, 0);
	insrt(state->idx_state_c, 24, 1, 0);
}

void pgraph_store_idx_fifo(struct pgraph_state *state, uint32_t addr, int be, uint32_t a, uint32_t b) {
	state->idx_fifo[state->idx_fifo_ptr][0] = a;
	state->idx_fifo[state->idx_fifo_ptr][1] = b;
	if (state->chipset.card_type == 0x20)
		state->idx_fifo[state->idx_fifo_ptr][2] = (addr >> 3) | be << 14;
	else
		state->idx_fifo[state->idx_fifo_ptr][2] = (addr >> 3) | be << 15;
	state->idx_fifo_ptr++;
	state->idx_fifo_ptr &= 0x3f;
	insrt(state->idx_state_b, 16, 5, 0);
	if (!nv04_pgraph_is_celsius_class(state)) {
		pgraph_kelvin_clear_idx(state);
	}
}

void pgraph_xf_cmd(struct pgraph_state *state, int cmd, uint32_t addr, int be, uint32_t a, uint32_t b) {
	if (state->chipset.card_type == 0x20)
		pgraph_store_idx_fifo(state, 0x10000 | cmd << 12 | addr, be, a, b);
	else
		pgraph_store_idx_fifo(state, 0x20000 | cmd << 13 | addr, be, a, b);
}

uint32_t pgraph_xlat_bundle(struct chipset_info *chipset, int bundle, int idx) {
	if (chipset->card_type == 0x20) {
		switch (bundle) {
		case BUNDLE_MULTISAMPLE:	return 0x00;
		case BUNDLE_BLEND:		return 0x01;
		case BUNDLE_BLEND_COLOR:	return 0x02;
		case BUNDLE_TEX_BORDER_COLOR:	return 0x03 + idx;
		case BUNDLE_TEX_UNK10:		return 0x07 + idx;
		case BUNDLE_TEX_UNK11:		return 0x0a + idx;
		case BUNDLE_TEX_UNK13:		return 0x0d + idx;
		case BUNDLE_TEX_UNK12:		return 0x10 + idx;
		case BUNDLE_TEX_UNK15:		return 0x13 + idx;
		case BUNDLE_TEX_UNK14:		return 0x16 + idx;
		case BUNDLE_CLEAR_HV:		return 0x19 + idx;
		case BUNDLE_CLEAR_COLOR:	return 0x1b;
		case BUNDLE_TEX_COLOR_KEY:	return 0x1c + idx;
		// 20..
		case BUNDLE_STENCIL_A:		return 0x54;
		case BUNDLE_STENCIL_B:		return 0x55;
		// 56..
		case BUNDLE_CLIP_HV:		return 0x6d + idx;
		// 6f..
		default:
			abort();
		}
	} else if (chipset->card_type == 0x30) {
		switch (bundle) {
		// 40...
		case BUNDLE_STENCIL_A:		return 0x54;
		case BUNDLE_STENCIL_B:		return 0x55;
		// 56...
		case BUNDLE_CLIP_HV:		return 0x6d + idx;
		case BUNDLE_MULTISAMPLE:	return 0x6f;
		case BUNDLE_TEX_UNK10:		return 0x70 + idx;
		case BUNDLE_TEX_UNK11:		return 0x73 + idx;
		case BUNDLE_TEX_UNK13:		return 0x76 + idx;
		case BUNDLE_TEX_UNK12:		return 0x79 + idx;
		case BUNDLE_TEX_UNK15:		return 0x7c + idx;
		case BUNDLE_TEX_UNK14:		return 0x7f + idx;
		case BUNDLE_BLEND:		return 0x82;
		case BUNDLE_BLEND_COLOR:	return 0x83;
		case BUNDLE_CLEAR_HV:		return 0x84 + idx;
		case BUNDLE_CLEAR_COLOR:	return 0x86;
		case BUNDLE_STENCIL_C:		return 0x87;
		case BUNDLE_STENCIL_D:		return 0x88;
		// 89...
		case BUNDLE_VIEWPORT_HV:	return 0x8b + idx;
		case BUNDLE_SCISSOR_HV:		return 0x8d + idx;
		// 8f...
		case BUNDLE_TEX_BORDER_COLOR:	return 0x170 + idx;
		case BUNDLE_TEX_COLOR_KEY:	return 0x190 + idx;
		default:
			abort();
		}
	} else {
		abort();
	}
}

void pgraph_kelvin_bundle(struct pgraph_state *state, int bundle, uint32_t val, bool last) {
	pgraph_xf_cmd(state, 5, 0, 3, bundle << 2, val);
	if (state->chipset.card_type == 0x20) {
		int uctr = extr(state->idx_state_b, 24, 5);
		uctr++;
		if (uctr == 0x18)
			uctr = 0;
		insrt(state->idx_state_b, 24, 5, uctr);
	}
	if (nv04_pgraph_is_nv25p(&state->chipset)) {
		// XXX: really?
		state->idx_unk27[state->idx_unk27_ptr] = 0x40 | (state->idx_unk27_ptr & 7);
		state->idx_unk27_ptr++;
		state->idx_unk27_ptr &= 0x3f;
	}
	state->vab[0x10][0] = bundle << 2;
	state->vab[0x10][1] = val;
}

void pgraph_bundle(struct pgraph_state *state, int bundle, int idx, uint32_t val, bool last) {
	bundle = pgraph_xlat_bundle(&state->chipset, bundle, idx);
	pgraph_kelvin_bundle(state, bundle, val, last);
}

void pgraph_flush_xf_mode(struct pgraph_state *state) {
	if (state->chipset.card_type == 0x20) {
		pgraph_xf_cmd(state, 7, 0, 3, state->xf_mode_b, state->xf_mode_a);
		pgraph_xf_cmd(state, 7, 8, 3, state->xf_mode_t[1], state->xf_mode_t[0]);
		state->vab[0x10][0] = state->xf_mode_b;
		state->vab[0x10][1] = state->xf_mode_a;
		state->vab[0x10][2] = state->xf_mode_t[1];
		state->vab[0x10][3] = state->xf_mode_t[0];
		if (extr(state->debug[3], 28, 1)) {
			// XXX
		}
	} else if (state->chipset.card_type == 0x30) {
		pgraph_xf_cmd(state, 7, 0x00, 3, 0, state->xf_mode_c);
		pgraph_xf_cmd(state, 7, 0x08, 3, state->xf_mode_b, state->xf_mode_a);
		pgraph_xf_cmd(state, 7, 0x10, 3, state->xf_mode_t[3], state->xf_mode_t[2]);
		pgraph_xf_cmd(state, 7, 0x18, 3, state->xf_mode_t[1], state->xf_mode_t[0]);
		state->vab[0x10][0] = state->xf_mode_t[3];
		state->vab[0x10][1] = state->xf_mode_t[2];
		state->vab[0x10][2] = state->xf_mode_t[1];
		state->vab[0x10][3] = state->xf_mode_t[0];
	}
}

void pgraph_ld_xfctx2(struct pgraph_state *state, uint32_t addr, uint32_t a, uint32_t b) {
	pgraph_xf_cmd(state, 9, addr, 3, a, b);
	state->vab[0x10][addr >> 2 & 2] = a;
	state->vab[0x10][addr >> 2 & 2 | 1] = b;
}

void pgraph_ld_xfctx(struct pgraph_state *state, uint32_t addr, uint32_t a) {
	pgraph_xf_cmd(state, 9, addr, addr & 4 ? 2 : 1, a, a);
	state->vab[0x10][addr >> 2 & 3] = a;
}

void pgraph_ld_ltctx2(struct pgraph_state *state, uint32_t addr, uint32_t a, uint32_t b) {
	pgraph_xf_cmd(state, 10, addr, 3, a, b);
	state->vab[0x10][addr >> 2 & 2] = a;
	state->vab[0x10][addr >> 2 & 2 | 1] = b;
}

void pgraph_ld_ltctx(struct pgraph_state *state, uint32_t addr, uint32_t a) {
	pgraph_xf_cmd(state, 10, addr, addr & 4 ? 2 : 1, a, a);
	state->vab[0x10][addr >> 2 & 3] = a;
}

void pgraph_ld_ltc(struct pgraph_state *state, int space, uint32_t addr, uint32_t a) {
	pgraph_xf_cmd(state, 11 + space, addr, addr & 4 ? 2 : 1, a, a);
	state->vab[0x10][addr >> 2 & 3] = a;
}

void pgraph_ld_xfpr(struct pgraph_state *state, uint32_t addr, uint32_t a) {
	pgraph_xf_cmd(state, 2, addr, addr & 4 ? 2 : 1, a, a);
	state->vab[0x10][addr >> 2 & 3] = a;
}

void pgraph_ld_xfunk4(struct pgraph_state *state, uint32_t addr, uint32_t a) {
	pgraph_xf_cmd(state, 4, addr, addr & 4 ? 2 : 1, a, a);
	state->vab[0x10][addr >> 2 & 3] = a;
}

void pgraph_ld_vab_raw(struct pgraph_state *state, int which, int comp, uint32_t a) {
	if (nv04_pgraph_is_celsius_class(state) && which == 4 && comp == 3) {
		which = 5;
		comp = 0;
	}
	state->vab[which][comp] = a;
	if (comp == 0) {
		for (int i = 1; i < 4; i++) {
			state->vab[which][i] = (i == 3 ? 0x3f800000 : 0);
		}
	}
}

void pgraph_ld_vtx(struct pgraph_state *state, int fmt, int which, int num, int comp, uint32_t a) {
	uint32_t be = (comp & 1 ? 2 : 1);
	pgraph_store_idx_fifo(state, fmt << 10 | (num & 3) << 8 | which << 4 | comp << 2, be, a, a);
	switch (fmt) {
		case 4:
			for (int i = 0; i < 4; i++) {
				if (i < num) {
					uint32_t fl = pgraph_idx_ubyte_to_float(a >> 8 * i);
					pgraph_ld_vab_raw(state, which, i, fl);
				}
			}
			break;
		case 5:
			pgraph_ld_vab_raw(state, which, comp * 2, pgraph_idx_short_to_float(state, a));
			if (comp * 2 + 1 < num)
				pgraph_ld_vab_raw(state, which, comp * 2 + 1, pgraph_idx_short_to_float(state, a >> 16));
			break;
		case 6:
			pgraph_ld_vab_raw(state, which, comp * 2, pgraph_idx_nshort_to_float(a));
			if (comp * 2 + 1 < num)
				pgraph_ld_vab_raw(state, which, comp * 2 + 1, pgraph_idx_nshort_to_float(a >> 16));
			break;
		case 7:
			pgraph_ld_vab_raw(state, which, comp, a);
			break;
	}
}

void pgraph_xf_nop(struct pgraph_state *state, uint32_t val) {
	pgraph_xf_cmd(state, 0, 0, 3, val, val);
	state->vab[0x10][2] = val;
	state->vab[0x10][3] = val;
}

void pgraph_xf_sync(struct pgraph_state *state, uint32_t val) {
	pgraph_xf_cmd(state, 15, 0, 3, val, val);
	state->vab[0x10][2] = val;
	state->vab[0x10][3] = val;
}

static int pgraph_vtxbuf_format_size(int fmt, int comp) {
	int rcomp = comp & 3;
	if (!rcomp)
		rcomp = 4;
	if (fmt == 0 || fmt == 4)
		return 1;
	else if (fmt == 1 || fmt == 5)
		return (rcomp + 1) / 2;
	else
		return rcomp;
}

void pgraph_set_vtxbuf_format(struct pgraph_state *state, int which, uint32_t val) {
	pgraph_store_idx_fifo(state, which << 3, 2, val, val);
	uint32_t rval = 0;
	int fmt = extr(val, 0, 3);
	int comp = extr(val, 4, 3);
	insrt(rval, 0, 3, fmt);
	insrt(rval, 3, 3, comp);
	insrt(rval, 6, 8, extr(val, 8, 8));
	insrt(rval, 14, 3, fmt);
	insrt(rval, 17, 3, comp);
	int sz = pgraph_vtxbuf_format_size(fmt, comp);
	insrt(rval, 20, 3, sz);
	state->idx_state_vtxbuf_format[which] = rval;
	insrt(state->idx_state_a, which, 1, comp != 0);
	insrt(state->idx_state_c, which, 1, comp != 0);
	pgraph_kelvin_clear_idx(state);
	int total = 0;
	for (int i = 0; i < 16; i++) {
		if (extr(state->idx_state_a, i, 1)) {
			int cfmt = extr(state->idx_state_vtxbuf_format[i], 0, 3);
			int ccomp = extr(state->idx_state_vtxbuf_format[i], 3, 3);
			int csz = pgraph_vtxbuf_format_size(cfmt, ccomp);
			total += csz;
		}
	}
	insrt(state->idx_state_b, 2, 6, total);
}
