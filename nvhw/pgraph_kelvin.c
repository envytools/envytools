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

void pgraph_kelvin_clear_idx(struct pgraph_state *state) {
	insrt(state->idx_state_b, 10, 6, 0);
	int first = 0;
	for (int i = 0; i < 16; i++)
		if (extr(state->idx_state_c, i, 1)) {
			first = i;
			break;
		}
	insrt(state->idx_state_c, 16, 4, first);
	insrt(state->idx_state_c, 20, 2, 0);
	insrt(state->idx_state_c, 24, 1, 0);
}

void pgraph_store_idx_fifo(struct pgraph_state *state, uint32_t a, uint32_t b, uint32_t c) {
	state->idx_fifo[state->idx_fifo_ptr][0] = a;
	state->idx_fifo[state->idx_fifo_ptr][1] = b;
	state->idx_fifo[state->idx_fifo_ptr][2] = c;
	state->idx_fifo_ptr++;
	state->idx_fifo_ptr &= 0x3f;
	insrt(state->idx_state_b, 16, 5, 0);
	if ((pgraph_class(state) & 0xff) == 0x97) {
		pgraph_kelvin_clear_idx(state);
	}
}

void pgraph_bundle(struct pgraph_state *state, int bundle, uint32_t val, bool last) {
	if (state->chipset.card_type == 0x20) {
		pgraph_store_idx_fifo(state, bundle << 2, val, 0xea00);
		int uctr = extr(state->idx_state_b, 24, 5);
		uctr++;
		if (uctr == 0x18)
			uctr = 0;
		insrt(state->idx_state_b, 24, 5, uctr);
	}
}

void pgraph_kelvin_xf_mode(struct pgraph_state *state) {
	if (state->chipset.card_type == 0x20) {
		pgraph_store_idx_fifo(state, state->kelvin_xf_mode_a, state->kelvin_xf_mode_b, 0xee00);
		pgraph_store_idx_fifo(state, state->kelvin_xf_mode_c[0], state->kelvin_xf_mode_c[1], 0xee01);
		if (extr(state->debug[3], 28, 1)) {
			// XXX
		}
	}
}

void pgraph_ld_xfctx2(struct pgraph_state *state, uint32_t addr, uint32_t a, uint32_t b) {
	pgraph_store_idx_fifo(state, a, b, addr >> 3 | 0xf200);
}

void pgraph_ld_xfctx(struct pgraph_state *state, uint32_t addr, uint32_t a) {
	pgraph_store_idx_fifo(state, a, a, addr >> 3 | (addr & 4 ? 0xb200 : 0x7200));
}

void pgraph_ld_ltctx2(struct pgraph_state *state, uint32_t addr, uint32_t a, uint32_t b) {
	pgraph_store_idx_fifo(state, a, b, addr >> 3 | 0xf400);
}

void pgraph_ld_ltctx(struct pgraph_state *state, uint32_t addr, uint32_t a) {
	pgraph_store_idx_fifo(state, a, a, addr >> 3 | (addr & 4 ? 0xb400 : 0x7400));
}

void pgraph_ld_ltc(struct pgraph_state *state, int space, uint32_t addr, uint32_t a) {
	pgraph_store_idx_fifo(state, a, a, addr >> 3 | (addr & 4 ? 0x8000 : 0x4000) | (0x7600 + space * 0x200));
}

void pgraph_ld_xfpr(struct pgraph_state *state, uint32_t addr, uint32_t a) {
	pgraph_store_idx_fifo(state, a, a, addr >> 3 | (addr & 4 ? 0xa400 : 0x6400));
}
