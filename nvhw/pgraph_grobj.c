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

uint32_t pgraph_grobj_get_notify_inst(struct pgraph_state *state) {
	if (state->chipset.card_type < 4) {
		return extr(state->notify, 0, 16);
	} else if (state->chipset.card_type < 0x40) {
		return extr(state->ctx_switch_b, 16, 16);
	} else {
		return extr(state->ctx_switch_b, 0, 24);
	}
}

void pgraph_grobj_set_notify_inst_a(struct pgraph_state *state, uint32_t *grobj, uint32_t val) {
	if (state->nsource)
		return;
	if (state->chipset.card_type < 0x40)
		insrt(grobj[1], 16, 16, val);
	else
		insrt(grobj[1], 0, 24, val);
}

void pgraph_grobj_set_notify_inst_b(struct pgraph_state *state, uint32_t val) {
	if (state->nsource)
		return;
	int subc = extr(state->ctx_user, 13, 3);
	state->ctx_cache_b[subc] = state->ctx_switch_b;
	if (state->chipset.card_type < 0x40)
		insrt(state->ctx_cache_b[subc], 16, 16, val);
	else
		insrt(state->ctx_cache_b[subc], 0, 24, val);
	if (extr(state->debug_b, 20, 1))
		state->ctx_switch_b = state->ctx_cache_b[subc];
}

static void pgraph_insrt_grobj_a(struct pgraph_state *state, uint32_t *grobj, int start, int size, uint32_t val) {
	if (state->nsource)
		return;
	int subc = extr(state->ctx_user, 13, 3);
	if (!nv04_pgraph_is_nv25p(&state->chipset))
		insrt(grobj[0], 8, 24, extr(state->ctx_switch_a, 8, 24));
	else
		grobj[0] = state->ctx_switch_a;
	insrt(grobj[0], start, size, val);
	state->ctx_cache_a[subc] = state->ctx_switch_a;
	insrt(state->ctx_cache_a[subc], start, size, val);
	if (extr(state->debug_b, 20, 1))
		state->ctx_switch_a = state->ctx_cache_a[subc];
}

uint32_t pgraph_grobj_get_operation(struct pgraph_state *state) {
	if (state->chipset.card_type < 0x40) {
		return extr(state->ctx_switch_a, 15, 3);
	} else {
		return extr(state->ctx_switch_c, 19, 3);
	}
}

void pgraph_grobj_set_operation(struct pgraph_state *state, uint32_t *grobj, uint32_t val) {
	if (state->chipset.card_type < 0x40) {
		pgraph_insrt_grobj_a(state, grobj, 15, 3, val);
	} else {
		pgraph_insrt_grobj_a(state, grobj, 19, 3, val);
	}
}

void pgraph_grobj_set_dither(struct pgraph_state *state, uint32_t *grobj, uint32_t val) {
	if (state->chipset.card_type < 0x40) {
		pgraph_insrt_grobj_a(state, grobj, 20, 2, val);
	} else {
		pgraph_insrt_grobj_a(state, grobj, 22, 2, val);
	}
}

uint32_t pgraph_grobj_get_color_format(struct pgraph_state *state) {
	if (state->chipset.card_type < 0x40) {
		return extr(state->ctx_switch_b, 8, 6);
	} else {
		return extr(state->ctx_switch_b, 26, 6);
	}
}

void pgraph_grobj_set_color_format(struct pgraph_state *state, uint32_t *grobj, uint32_t val) {
	if (state->nsource)
		return;
	int subc = extr(state->ctx_user, 13, 3);
	if (state->chipset.card_type < 0x40) {
		insrt(grobj[1], 8, 8, val);
		state->ctx_cache_b[subc] = state->ctx_switch_b;
		insrt(state->ctx_cache_b[subc], 8, 8, val);
		if (extr(state->debug_b, 20, 1))
			state->ctx_switch_b = state->ctx_cache_b[subc];
	} else {
		insrt(grobj[1], 24, 8, extr(state->ctx_switch_b, 24, 8));
		insrt(grobj[1], 26, 6, val);
		state->ctx_cache_b[subc] = state->ctx_switch_b;
		insrt(state->ctx_cache_b[subc], 24, 8, extr(grobj[1], 24, 8));
		if (extr(state->debug_b, 20, 1))
			state->ctx_switch_b = state->ctx_cache_b[subc];
	}
}

uint32_t pgraph_grobj_get_bitmap_format(struct pgraph_state *state) {
	if (state->chipset.card_type < 0x40) {
		return extr(state->ctx_switch_b, 0, 2);
	} else {
		return extr(state->ctx_switch_b, 24, 2);
	}
}

void pgraph_grobj_set_bitmap_format(struct pgraph_state *state, uint32_t *grobj, uint32_t val) {
	if (state->nsource)
		return;
	int subc = extr(state->ctx_user, 13, 3);
	if (state->chipset.card_type < 0x40) {
		insrt(grobj[1], 0, 8, val);
		state->ctx_cache_b[subc] = state->ctx_switch_b;
		insrt(state->ctx_cache_b[subc], 0, 2, val);
		if (extr(state->debug_b, 20, 1))
			state->ctx_switch_b = state->ctx_cache_b[subc];
	} else {
		insrt(grobj[1], 24, 8, extr(state->ctx_switch_b, 24, 8));
		insrt(grobj[1], 24, 2, val);
		state->ctx_cache_b[subc] = state->ctx_switch_b;
		insrt(state->ctx_cache_b[subc], 24, 8, extr(grobj[1], 24, 8));
		if (extr(state->debug_b, 20, 1))
			state->ctx_switch_b = state->ctx_cache_b[subc];
	}
}

uint32_t pgraph_grobj_get_endian(struct pgraph_state *state) {
	if (!nv04_pgraph_is_nv11p(&state->chipset))
		return 0;
	if (state->chipset.card_type < 0x40) {
		return extr(state->ctx_switch_a, 19, 1);
	} else {
		return extr(state->ctx_switch_c, 24, 1);
	}
}

uint32_t pgraph_grobj_get_sync(struct pgraph_state *state) {
	if (!nv04_pgraph_is_nv11p(&state->chipset))
		return 0;
	if (state->chipset.card_type < 0x40) {
		return extr(state->ctx_switch_a, 18, 1);
	} else {
		return extr(state->ctx_switch_c, 25, 1);
	}
}
