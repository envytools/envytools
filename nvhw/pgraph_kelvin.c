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

int pgraph_vtx_attr_xlat_celsius(struct pgraph_state *state, int idx) {
	// POS, COL0, COL1, TXC0, TXC1, NRM, WEI, FOG
	const int xlat[8] = {0, 3, 4, 9, 0xa, 2, 1, 5};
	return xlat[idx];
}

int pgraph_vtx_attr_xlat_kelvin(struct pgraph_state *state, int idx) {
	if (state->chipset.card_type == 0x20)
		return idx;
	const int xlat[16] = {
		// POS, WEI, NRM, COL0, COL1, FOG, ???, ???
		0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
		// ???, TXC0, TXC1, TXC2, TXC3, ???, ???, ???
		0xf, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe,
	};
	return xlat[idx];
}

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
		for (int i = 7; i > 0; i--) {
			int ridx = pgraph_vtx_attr_xlat_celsius(state, i);
			if (extr(state->idx_state_c, ridx, 1)) {
				first = ridx;
				break;
			}
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

void pgraph_store_idx_prefifo(struct pgraph_state *state, uint32_t addr, int be, uint32_t a, uint32_t b) {
	state->idx_prefifo[state->idx_prefifo_ptr][0] = a;
	state->idx_prefifo[state->idx_prefifo_ptr][1] = b;
	if (state->chipset.card_type == 0x20)
		state->idx_prefifo[state->idx_prefifo_ptr][2] = (addr >> 3) | be << 14;
	else
		state->idx_prefifo[state->idx_prefifo_ptr][2] = (addr >> 3) | be << 15;
	state->idx_prefifo_ptr++;
	state->idx_prefifo_ptr &= 0x3f;
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
	if (addr == 0x80 && state->chipset.card_type == 0x30) {
		insrt(state->idx_state_c, 20, 2, 0);
		return;
	}
	if (!nv04_pgraph_is_celsius_class(state)) {
		pgraph_kelvin_clear_idx(state);
	}
}

void pgraph_xf_cmd(struct pgraph_state *state, int cmd, uint32_t addr, int be, uint32_t a, uint32_t b) {
	if (state->chipset.card_type == 0x20)
		pgraph_store_idx_fifo(state, 0x10000 | cmd << 12 | addr, be, a, b);
	else {
		pgraph_store_idx_prefifo(state, 0x20000 | cmd << 13 | addr, be, a, b);
		pgraph_store_idx_fifo(state, 0x20000 | cmd << 13 | addr, be, a, b);
	}
}

uint32_t pgraph_xlat_bundle(struct chipset_info *chipset, int bundle, int idx) {
	if (chipset->card_type == 0x00) {
		switch (bundle) {
		case BUNDLE_TEX_OFFSET:		return 0x00 + idx;
		case BUNDLE_TEX_PALETTE:	return 0x02 + idx;
		case BUNDLE_TEX_FORMAT:		return 0x04 + idx;
		case BUNDLE_TEX_CONTROL:	return 0x06 + idx;
		case BUNDLE_TEX_PITCH:		return 0x08 + idx;
		case BUNDLE_TEX_UNK238:		return 0x0a + idx;
		case BUNDLE_TEX_RECT:		return 0x0c + idx;
		case BUNDLE_TEX_FILTER:		return 0x0e + idx;
		case BUNDLE_RC_IN_ALPHA:	return 0x10 + idx;
		case BUNDLE_RC_IN_COLOR:	return 0x12 + idx;
		case BUNDLE_RC_FACTOR_A:	return 0x14;
		case BUNDLE_RC_FACTOR_B:	return 0x15;
		case BUNDLE_RC_OUT_ALPHA:	return 0x16 + idx;
		case BUNDLE_RC_OUT_COLOR:	return 0x18 + idx;
		case BUNDLE_RC_FINAL_A:		return 0x1a;
		case BUNDLE_RC_FINAL_B:		return 0x1b;
		case BUNDLE_CONFIG_A:		return 0x1c;
		case BUNDLE_STENCIL_A:		return 0x1d;
		case BUNDLE_STENCIL_B:		return 0x1e;
		case BUNDLE_CONFIG_B:		return 0x1f;
		case BUNDLE_BLEND:		return 0x20;
		case BUNDLE_BLEND_COLOR:	return 0x21;
		case BUNDLE_RASTER:		return 0x22;
		case BUNDLE_FOG_COLOR:		return 0x23;
		case BUNDLE_POLYGON_OFFSET_FACTOR:	return 0x24;
		case BUNDLE_POLYGON_OFFSET_UNITS:	return 0x25;
		case BUNDLE_DEPTH_RANGE_NEAR:	return 0x26;
		case BUNDLE_DEPTH_RANGE_FAR:	return 0x27;
		case BUNDLE_TEX_COLOR_KEY:	return 0x28 + idx;
		case BUNDLE_POINT_SIZE:		return 0x2a;
		case BUNDLE_CLEAR_HV:		return 0x2b + idx;
		case BUNDLE_SURF_BASE_ZCULL:	return 0x2d;
		case BUNDLE_SURF_LIMIT_ZCULL:	return 0x2e;
		case BUNDLE_SURF_OFFSET_ZCULL:	return 0x2f;
		case BUNDLE_SURF_PITCH_ZCULL:	return 0x30;
		case BUNDLE_SURF_BASE_CLIPID:	return 0x31;
		case BUNDLE_SURF_LIMIT_CLIPID:	return 0x32;
		case BUNDLE_SURF_OFFSET_CLIPID:	return 0x33;
		case BUNDLE_SURF_PITCH_CLIPID:	return 0x34;
		case BUNDLE_CLIPID_ID:		return 0x35;
		case BUNDLE_Z_CONFIG:		return 0x36;
		case BUNDLE_CLEAR_ZETA:		return 0x37;
		// 38...
		default:
			abort();
		}
	} else if (chipset->card_type == 0x20) {
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
		case BUNDLE_RC_FACTOR_A:	return 0x20 + idx;
		case BUNDLE_RC_FACTOR_B:	return 0x28 + idx;
		case BUNDLE_RC_IN_ALPHA:	return 0x30 + idx;
		case BUNDLE_RC_OUT_ALPHA:	return 0x38 + idx;
		case BUNDLE_RC_IN_COLOR:	return 0x40 + idx;
		case BUNDLE_RC_OUT_COLOR:	return 0x48 + idx;
		case BUNDLE_RC_CONFIG:		return 0x50;
		case BUNDLE_RC_FINAL_A:		return 0x51;
		case BUNDLE_RC_FINAL_B:		return 0x52;
		case BUNDLE_CONFIG_A:		return 0x53;
		case BUNDLE_STENCIL_A:		return 0x54;
		case BUNDLE_STENCIL_B:		return 0x55;
		case BUNDLE_CONFIG_B:		return 0x56;
		case BUNDLE_CLIPID_ID:		return 0x59;
		case BUNDLE_SURF_BASE_CLIPID:	return 0x5a;
		case BUNDLE_SURF_LIMIT_CLIPID:	return 0x5b;
		case BUNDLE_SURF_OFFSET_CLIPID:	return 0x5c;
		case BUNDLE_SURF_PITCH_CLIPID:	return 0x5d;
		case BUNDLE_LINE_STIPPLE:	return 0x5e;
		case BUNDLE_RT_ENABLE:		return 0x5f;
		case BUNDLE_FOG_COLOR:		return 0x60;
		case BUNDLE_FOG_COEFF:		return 0x61 + idx;
		case BUNDLE_POINT_SIZE:		return 0x63;
		case BUNDLE_RASTER:		return 0x64;
		case BUNDLE_TEX_SHADER_CULL_MODE:	return 0x65;
		case BUNDLE_TEX_SHADER_MISC:	return 0x66;
		case BUNDLE_TEX_SHADER_OP:	return 0x67;
		case BUNDLE_FENCE_OFFSET:	return 0x68;
		case BUNDLE_TEX_ZCOMP:		return 0x69;
		case BUNDLE_UNK1E68:		return 0x6a;
		case BUNDLE_RC_FINAL_FACTOR:	return 0x6b + idx;
		case BUNDLE_CLIP_HV:		return 0x6d + idx;
		case BUNDLE_TEX_WRAP:		return 0x6f + idx;
		case BUNDLE_TEX_CONTROL:	return 0x73 + idx;
		case BUNDLE_TEX_PITCH:		return 0x77 + idx;
		case BUNDLE_TEX_UNK238:		return 0x7b + idx;
		case BUNDLE_TEX_FILTER:		return 0x7d + idx;
		case BUNDLE_TEX_FORMAT:		return 0x81 + idx;
		case BUNDLE_TEX_RECT:		return 0x85 + idx;
		case BUNDLE_TEX_OFFSET:		return 0x89 + idx;
		case BUNDLE_TEX_PALETTE:	return 0x8d + idx;
		case BUNDLE_CLIP_RECT_HORIZ:	return 0x91 + idx;
		case BUNDLE_CLIP_RECT_VERT:	return 0x99 + idx;
		case BUNDLE_Z_CONFIG:		return 0xa1;
		case BUNDLE_CLEAR_ZETA:		return 0xa2;
		case BUNDLE_DEPTH_RANGE_FAR:	return 0xa3;
		case BUNDLE_DEPTH_RANGE_NEAR:	return 0xa4;
		case BUNDLE_DMA_TEX:		return 0xa5 + idx;
		case BUNDLE_DMA_VTX:		return 0xa7 + idx;
		case BUNDLE_POLYGON_OFFSET_UNITS:	return 0xa9;
		case BUNDLE_POLYGON_OFFSET_FACTOR:	return 0xaa;
		case BUNDLE_TEX_SHADER_CONST_EYE:	return 0xab + idx;
		// ae..
		case BUNDLE_SURF_BASE_ZCULL:	return 0xb0;
		case BUNDLE_SURF_LIMIT_ZCULL:	return 0xb1;
		case BUNDLE_SURF_OFFSET_ZCULL:	return 0xb2;
		case BUNDLE_SURF_PITCH_ZCULL:	return 0xb3;
		case BUNDLE_UNK0B4:		return 0xb4 + idx;
		case BUNDLE_UNK0B8:		return 0xb8;
		case BUNDLE_POLYGON_STIPPLE:	return 0x100 + idx;
		case BUNDLE_ZPASS_COUNTER_RESET:	return 0x1fd;
		default:
			abort();
		}
	} else if (chipset->card_type == 0x30) {
		switch (bundle) {
		case BUNDLE_POLYGON_STIPPLE:	return 0x00 + idx;
		case BUNDLE_RC_FACTOR_A:	return 0x20 + idx;
		case BUNDLE_RC_FACTOR_B:	return 0x28 + idx;
		case BUNDLE_RC_IN_ALPHA:	return 0x30 + idx;
		case BUNDLE_RC_OUT_ALPHA:	return 0x38 + idx;
		case BUNDLE_RC_IN_COLOR:	return 0x40 + idx;
		case BUNDLE_RC_OUT_COLOR:	return 0x48 + idx;
		case BUNDLE_RC_CONFIG:		return 0x50;
		case BUNDLE_RC_FINAL_A:		return 0x51;
		case BUNDLE_RC_FINAL_B:		return 0x52;
		case BUNDLE_CONFIG_A:		return 0x53;
		case BUNDLE_STENCIL_A:		return 0x54;
		case BUNDLE_STENCIL_B:		return 0x55;
		case BUNDLE_CONFIG_B:		return 0x56;
		case BUNDLE_VIEWPORT_OFFSET:	return 0x57;
		case BUNDLE_PS_OFFSET:		return 0x58;
		case BUNDLE_CLIPID_ID:		return 0x59;
		case BUNDLE_SURF_BASE_CLIPID:	return 0x5a;
		case BUNDLE_SURF_LIMIT_CLIPID:	return 0x5b;
		case BUNDLE_SURF_OFFSET_CLIPID:	return 0x5c;
		case BUNDLE_SURF_PITCH_CLIPID:	return 0x5d;
		case BUNDLE_LINE_STIPPLE:	return 0x5e;
		case BUNDLE_RT_ENABLE:		return 0x5f;
		case BUNDLE_FOG_COLOR:		return 0x60;
		case BUNDLE_FOG_COEFF:		return 0x61 + idx;
		case BUNDLE_POINT_SIZE:		return 0x63;
		case BUNDLE_RASTER:		return 0x64;
		case BUNDLE_TEX_SHADER_CULL_MODE:	return 0x65;
		case BUNDLE_TEX_SHADER_MISC:	return 0x66;
		case BUNDLE_TEX_SHADER_OP:	return 0x67;
		case BUNDLE_FENCE_OFFSET:	return 0x68;
		// 69..
		case BUNDLE_UNK1E68:		return 0x6a;
		case BUNDLE_RC_FINAL_FACTOR:	return 0x6b + idx;
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
		case BUNDLE_CLIP_PLANE_ENABLE:	return 0x89;
		case BUNDLE_VIEWPORT_HV:	return 0x8b + idx;
		case BUNDLE_SCISSOR_HV:		return 0x8d + idx;
		case BUNDLE_CLIP_RECT_HORIZ:	return 0x91 + idx;
		case BUNDLE_CLIP_RECT_VERT:	return 0x99 + idx;
		case BUNDLE_Z_CONFIG:		return 0xa1;
		case BUNDLE_CLEAR_ZETA:		return 0xa2;
		case BUNDLE_DEPTH_RANGE_FAR:	return 0xa3;
		case BUNDLE_DEPTH_RANGE_NEAR:	return 0xa4;
		case BUNDLE_DMA_TEX:		return 0xa5 + idx;
		case BUNDLE_DMA_VTX:		return 0xa7 + idx;
		case BUNDLE_POLYGON_OFFSET_UNITS:	return 0xa9;
		case BUNDLE_POLYGON_OFFSET_FACTOR:	return 0xaa;
		case BUNDLE_TEX_SHADER_CONST_EYE:	return 0xab + idx;
		case BUNDLE_UNK0AF:		return 0xaf;
		case BUNDLE_SURF_BASE_ZCULL:	return 0xb0;
		case BUNDLE_SURF_LIMIT_ZCULL:	return 0xb1;
		case BUNDLE_SURF_OFFSET_ZCULL:	return 0xb2;
		case BUNDLE_SURF_PITCH_ZCULL:	return 0xb3;
		case BUNDLE_UNK0B4:		return 0xb4 + idx;
		case BUNDLE_UNK0B8:		return 0xb8;
		case BUNDLE_PRIMITIVE_RESTART_ENABLE:	return 0xb9;
		case BUNDLE_PRIMITIVE_RESTART_INDEX:	return 0xba;
		case BUNDLE_TXC_CYLWRAP:	return 0xbb;
		case BUNDLE_PS_PREFETCH:	return 0xbc + idx;
		case BUNDLE_PS_CONTROL:		return 0xc4;
		case BUNDLE_TXC_ENABLE:		return 0xc5;
		// c6..
		case BUNDLE_WINDOW_CONFIG:	return 0xc7;
		case BUNDLE_TEX_OFFSET:		return 0x100 + idx;
		case BUNDLE_TEX_FORMAT:		return 0x110 + idx;
		case BUNDLE_TEX_WRAP:		return 0x120 + idx;
		case BUNDLE_TEX_CONTROL:	return 0x130 + idx;
		case BUNDLE_TEX_PITCH:		return 0x140 + idx;
		case BUNDLE_TEX_FILTER:		return 0x150 + idx;
		case BUNDLE_TEX_RECT:		return 0x160 + idx;
		case BUNDLE_TEX_BORDER_COLOR:	return 0x170 + idx;
		case BUNDLE_TEX_PALETTE:	return 0x180 + idx;
		case BUNDLE_TEX_COLOR_KEY:	return 0x190 + idx;
		case BUNDLE_UNK1F7:		return 0x1f7;
		case BUNDLE_ZPASS_COUNTER_RESET:	return 0x1fd;
		default:
			abort();
		}
	} else {
		abort();
	}
}

void pgraph_kelvin_bundle(struct pgraph_state *state, int bundle, uint32_t val, bool last) {
	if (state->chipset.card_type == 0x20)
		pgraph_store_idx_fifo(state, 0x15000, 3, bundle << 2, val);
	else {
		uint32_t addr = 0x800 | bundle << 2;
		pgraph_store_idx_prefifo(state, addr, bundle & 1 ? 2 : 1, val, val);
		if (bundle == 0xb9 || bundle == 0xba)
			return;
		pgraph_store_idx_fifo(state, 0x2a000, 3, bundle << 2, val);
	}
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
		if (nv04_pgraph_is_rankine_class(state)) {
			insrt(state->idx_state_b, 10, 6, 0);
		}
	}
}

void pgraph_ld_xfctx2(struct pgraph_state *state, uint32_t which, int comp, uint32_t a, uint32_t b) {
	uint32_t addr = which << 4 | comp << 2;
	pgraph_xf_cmd(state, 9, addr, 3, a, b);
	state->vab[0x10][comp] = a;
	state->vab[0x10][comp + 1] = b;
	if (nv04_pgraph_is_rankine_class(state)) {
		insrt(state->idx_state_b, 10, 6, 0);
	}
}

void pgraph_ld_xfctx(struct pgraph_state *state, uint32_t which, int comp, uint32_t a) {
	uint32_t addr = which << 4 | comp << 2;
	pgraph_xf_cmd(state, 9, addr, addr & 4 ? 2 : 1, a, a);
	state->vab[0x10][comp] = a;
	if (nv04_pgraph_is_rankine_class(state)) {
		insrt(state->idx_state_b, 10, 6, 0);
	}
}

void pgraph_ld_ltctx2(struct pgraph_state *state, uint32_t which, int comp, uint32_t a, uint32_t b) {
	uint32_t addr = which << 4 | comp << 2;
	pgraph_xf_cmd(state, 10, addr, 3, a, b);
	state->vab[0x10][comp] = a;
	state->vab[0x10][comp + 1] = b;
	if (nv04_pgraph_is_rankine_class(state)) {
		insrt(state->idx_state_b, 10, 6, 0);
	}
}

void pgraph_ld_ltctx(struct pgraph_state *state, uint32_t which, int comp, uint32_t a) {
	uint32_t addr = which << 4 | comp << 2;
	pgraph_xf_cmd(state, 10, addr, addr & 4 ? 2 : 1, a, a);
	state->vab[0x10][comp] = a;
	if (nv04_pgraph_is_rankine_class(state)) {
		insrt(state->idx_state_b, 10, 6, 0);
	}
}

void pgraph_ld_ltc(struct pgraph_state *state, int space, uint32_t which, uint32_t a) {
	uint32_t addr = which << 4;
	pgraph_xf_cmd(state, 11 + space, addr, addr & 4 ? 2 : 1, a, a);
	state->vab[0x10][0] = a;
	if (nv04_pgraph_is_rankine_class(state)) {
		insrt(state->idx_state_b, 10, 6, 0);
	}
}

void pgraph_ld_xfpr(struct pgraph_state *state, uint32_t which, int comp, uint32_t a) {
	uint32_t addr = which << 4 | comp << 2;
	if (nv04_pgraph_is_rankine_class(state)) {
		insrt(state->idx_state_b, 10, 6, 0);
	}
	pgraph_xf_cmd(state, 2, addr, addr & 4 ? 2 : 1, a, a);
	state->vab[0x10][comp] = a;
	if (state->chipset.card_type == 0x20) {
		if (which >= 0x88)
			return;
		state->xfpr[which][0] = state->vab[0x10][3];
		state->xfpr[which][1] = state->vab[0x10][2];
		state->xfpr[which][2] = state->vab[0x10][1] & 0x0fffffff;
		state->xfpr[which][3] = 0;
	} else {
		if (which >= 0x118)
			return;
		if (nv04_pgraph_is_kelvin_class(state)) {
			uint32_t swa = state->vab[0x10][3];
			uint32_t swb = state->vab[0x10][2];
			uint32_t swc = state->vab[0x10][1];
			uint32_t wa = 0, wb = 0, wc = 0, wd = 0;
			insrt(wa, 0, 1, extr(swa, 0, 1)); // END
			insrt(wa, 1, 1, extr(swa, 1, 1)); // INDEX_CONST
			int rdst_wm_sca = extr(swa, 16, 4);
			int rdst = extr(swa, 20, 4);
			int rdst_wm_vec = extr(swa, 24, 4);
			int vop = extr(swc, 21, 4);
			int sop = extr(swc, 25, 3);
			if (vop == 0xd)
				rdst = 0;
			if (rdst > 0xb) {
				rdst = 0xb;
				rdst_wm_vec = 0;
				if (vop == 0 || vop > 0xd)
					rdst_wm_sca = 0;
			}
			int dst = extr(swa, 3, 8);
			if (!extr(swa, 11, 1)) {
				dst += 0x3c;
			} else {
				switch (dst) {
				case 0x7: dst = 0x1; break;
				case 0x8: dst = 0x2; break;
				case 0x9: dst = 0x8; break;
				case 0xa: dst = 0x9; break;
				case 0xb: dst = 0xa; break;
				case 0xc: dst = 0xb; break;
				case 0xd: dst = 0xc; break;
				case 0xe: dst = 0xd; break;
				case 0xf: dst = 0xe; break;
				case 0xff: dst = 0x1ff; break; // wtf?
				}
			}
			insrt(wa, 2, 9, dst);
			insrt(wa, 11, 1, extr(swa, 11, 1)); // DST_TYPE
			if (extr(swa, 2, 1))
				insrt(wa, 16, 4, extr(swa, 12, 4));
			else
				insrt(wa, 12, 4, extr(swa, 12, 4));
			insrt(wa, 20, 4, rdst_wm_vec);
			insrt(wa, 24, 4, rdst_wm_sca);
			int src[3];
			src[2] = extr(swa, 28, 4) | extr(swb, 0, 11) << 4;
			src[1] = extr(swb, 11, 15);
			src[0] = extr(swb, 26, 6) | extr(swc, 0, 9) << 6;
			for (int i = 0; i < 3; i++) {
				int type = extr(src[i], 0, 2);
				if (type == 0 && i != 2)
					insrt(src[i], 0, 2, 2);
				int reg = extr(src[i], 2, 4);
				if (reg > 0xc)
					insrt(src[i], 2, 4, 0xc);
			}
			insrt(wa, 28, 4, extr(src[2], 0, 4));
			insrt(wb, 0, 11, extr(src[2], 4, 11));
			insrt(wb, 11, 15, src[1]);
			insrt(wb, 26, 6, extr(src[0], 0, 6));
			insrt(wc, 0, 9, extr(src[0], 6, 9));
			insrt(wc, 9, 4, pgraph_vtx_attr_xlat_kelvin(state, extr(swc, 9, 4)));
			insrt(wc, 14, 9, extr(swc, 13, 8) + 0x3c);
			if (vop > 0xd)
				vop = 0;
			insrt(wc, 23, 5, vop);
			insrt(wc, 28, 4, extr(sop, 0, 4));
			insrt(wd, 0, 1, extr(sop, 4, 1));
			insrt(wd, 3, 8, 0x1b);
			insrt(wd, 11, 3, 7);
			insrt(wd, 16, 4, rdst);
			state->xfpr[which][0] = wa;
			state->xfpr[which][1] = wb;
			state->xfpr[which][2] = wc;
			state->xfpr[which][3] = wd;
		} else {
			// Wtf happens to bit 121?
			state->xfpr[which][0] = state->vab[0x10][3];
			state->xfpr[which][1] = state->vab[0x10][2];
			state->xfpr[which][2] = state->vab[0x10][1];
			state->xfpr[which][3] = state->vab[0x10][0] & 0x01ffffff;
		}
	}
}

void pgraph_ld_xfunk4(struct pgraph_state *state, uint32_t addr, uint32_t a) {
	pgraph_xf_cmd(state, 4, addr, addr & 4 ? 2 : 1, a, a);
	state->vab[0x10][addr >> 2 & 3] = a;
	if (nv04_pgraph_is_rankine_class(state)) {
		insrt(state->idx_state_b, 10, 6, 0);
	}
}

void pgraph_ld_xfunk8(struct pgraph_state *state, uint32_t addr, uint32_t a) {
	pgraph_xf_cmd(state, 8, addr, addr & 4 ? 2 : 1, a, a);
	state->vab[0x10][addr >> 2 & 3] = a;
	if (nv04_pgraph_is_rankine_class(state)) {
		insrt(state->idx_state_b, 10, 6, 0);
	}
	uint32_t which = addr >> 4;
	if (which > 4)
		return;
	uint32_t src[4];
	src[0] = state->vab[0x10][0];
	src[1] = state->vab[0x10][1];
	src[2] = state->vab[0x10][2];
	src[3] = state->vab[0x10][3];
	if (which < 2) {
		uint32_t res[3] = {0};
		insrt(res[0], 0, 9, extr(src[0], 0, 9));
		insrt(res[0], 9, 9, extr(src[0], 16, 9));
		insrt(res[0], 18, 9, extr(src[1], 0, 9));
		insrt(res[0], 27, 5, extr(src[1], 16, 5));
		insrt(res[1], 0, 4, extr(src[1], 21, 4));
		insrt(res[1], 4, 9, extr(src[2], 0, 9));
		insrt(res[1], 13, 9, extr(src[2], 16, 9));
		insrt(res[1], 22, 9, extr(src[3], 0, 9));
		insrt(res[1], 31, 1, extr(src[3], 16, 1));
		insrt(res[2], 0, 8, extr(src[3], 17, 8));
		state->xfprunk1[which][0] = res[0];
		state->xfprunk1[which][1] = res[1];
		state->xfprunk1[which][2] = res[2];
	} else if (which == 2) {
		state->xfprunk2 = src[0] & 0xffff;
	}
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
	uint32_t addr = fmt << 10 | (num & 3) << 8 | which << 4 | comp << 2;
	insrt(state->idx_state_b, 10, 6, 0);
	if (state->chipset.card_type == 0x30)
		pgraph_store_idx_prefifo(state, addr, be, a, a);
	pgraph_store_idx_fifo(state, addr, be, a, a);
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

void pgraph_set_edge_flag(struct pgraph_state *state, bool val) {
	insrt(state->idx_state_a, 24, 1, val);
	if (state->chipset.card_type == 0x30) {
		pgraph_store_idx_prefifo(state, 0x80, 2, val, val);
		if (extr(state->idx_state_b, 10, 6))
			pgraph_store_idx_fifo(state, 0x80, 2, val, val);
	}
	if (!nv04_pgraph_is_celsius_class(state))
		insrt(state->idx_state_b, 10, 6, 0);
}

void pgraph_idx_unkf8(struct pgraph_state *state, uint32_t val) {
	if (state->chipset.card_type == 0x30)
		pgraph_store_idx_prefifo(state, 0xf8, 1, val, val);
	if (!nv04_pgraph_is_celsius_class(state))
		insrt(state->idx_state_b, 10, 6, 0);
}

void pgraph_idx_unkfc(struct pgraph_state *state, uint32_t val) {
	if (state->chipset.card_type == 0x30)
		pgraph_store_idx_prefifo(state, 0xfc, 2, val, val);
	if (!nv04_pgraph_is_celsius_class(state))
		insrt(state->idx_state_b, 10, 6, 0);
}

void pgraph_xf_nop(struct pgraph_state *state, uint32_t val) {
	pgraph_xf_cmd(state, 0, 8, 3, val, val);
	state->vab[0x10][2] = val;
	state->vab[0x10][3] = val;
	if (nv04_pgraph_is_rankine_class(state)) {
		insrt(state->idx_state_b, 10, 6, 0);
	}
}

void pgraph_xf_sync(struct pgraph_state *state, uint32_t val) {
	pgraph_xf_cmd(state, 15, 8, 3, val, val);
	state->vab[0x10][2] = val;
	state->vab[0x10][3] = val;
	if (nv04_pgraph_is_rankine_class(state)) {
		insrt(state->idx_state_b, 10, 6, 0);
	}
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

void pgraph_set_vtxbuf_offset(struct pgraph_state *state, int which, uint32_t val) {
	if (state->chipset.card_type == 0x30)
		pgraph_store_idx_prefifo(state, which << 3, 1, val, val);
	insrt(state->idx_state_b, 10, 6, 0);
	if (state->chipset.card_type == 0x30)
		state->idx_state_vtxbuf_offset[which] = val & 0x9fffffff;
	else
		state->idx_state_vtxbuf_offset[which] = val & 0x8fffffff;
}

void pgraph_set_vtxbuf_format(struct pgraph_state *state, int which, uint32_t val) {
	if (state->chipset.card_type == 0x30)
		pgraph_store_idx_prefifo(state, which << 3, 2, val, val);
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

void pgraph_set_idxbuf_offset(struct pgraph_state *state, uint32_t val) {
	insrt(state->idx_state_d, 0, 29, extr(val, 0, 29) & ~1);
}

void pgraph_set_idxbuf_format(struct pgraph_state *state, uint32_t val) {
	insrt(state->idx_state_d, 29, 1, extr(val, 4, 4) != 0);
	insrt(state->idx_state_d, 30, 1, extr(val, 0, 1));
}
