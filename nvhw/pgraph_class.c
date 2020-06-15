/*
 * Copyright (C) 2017 Marcelina Kościelnicka <mwk@0x04.net>
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

bool nv04_pgraph_is_syncable_class(struct pgraph_state *state) {
	if (!nv04_pgraph_is_nv15p(&state->chipset))
		return false;
	int cls = pgraph_class(state);
	bool alt = extr(state->debug_d, 16, 1) && state->chipset.card_type >= 0x10;
	switch (cls) {
		case 0x88:
			return state->chipset.card_type >= 0x10;
		case 0x62:
		case 0x7b:
		case 0x89:
		case 0x56:
			return state->chipset.card_type >= 0x10 && !alt;
		case 0x8a:
			return (state->chipset.card_type >= 0x10 && !alt) || nv04_pgraph_is_nv11p(&state->chipset);
		case 0x79:
		case 0x82:
		case 0x87:
		case 0x85:
			return state->chipset.card_type >= 0x10 && alt;
		case 0x94:
		case 0x95:
			return nv04_pgraph_is_nv15p(&state->chipset) && state->chipset.card_type < 0x20;
		case 0x96:
		case 0x9e:
		case 0x9f:
			return nv04_pgraph_is_nv15p(&state->chipset);
		case 0x72:
		case 0x43:
		case 0x57:
		case 0x44:
		case 0x19:
		case 0x39:
		case 0x42:
		case 0x52:
		case 0x5c:
		case 0x5d:
		case 0x5e:
		case 0x4a:
		case 0x5f:
		case 0x61:
		case 0x76:
		case 0x77:
		case 0x38:
		case 0x65:
		case 0x66:
		case 0x64:
		case 0x60:
			return nv04_pgraph_is_nv11p(&state->chipset);
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x93:
			return nv04_pgraph_is_nv11p(&state->chipset) && state->chipset.card_type < 0x20;
		case 0x63:
			return nv04_pgraph_is_nv11p(&state->chipset) && !alt;
		case 0x67:
			return nv04_pgraph_is_nv11p(&state->chipset) && alt;
		case 0x98:
		case 0x99:
			return nv04_pgraph_is_nv17p(&state->chipset);
		case 0x97:
			return state->chipset.card_type >= 0x20 && state->chipset.chipset != 0x34;
		case 0x597:
			return nv04_pgraph_is_nv25p(&state->chipset);
		case 0x397:
			return state->chipset.card_type == 0x30;
		case 0x497:
			return state->chipset.chipset == 0x35 || state->chipset.chipset == 0x36;
		case 0x697:
			return state->chipset.chipset == 0x34;
		case 0x35c:
		case 0x38a:
		case 0x364:
		case 0x366:
		case 0x37b:
		case 0x389:
		case 0x362:
		case 0x39e:
			return state->chipset.card_type == 0x30;
		default:
			return false;
	}
}

bool nv04_pgraph_is_sync_class(struct pgraph_state *state) {
	int cls = pgraph_class(state);
	bool alt = extr(state->debug_d, 16, 1) && state->chipset.card_type >= 0x10;
	switch (cls) {
		case 0x8a:
		case 0x88:
			return state->chipset.card_type >= 0x10;
		case 0x62:
		case 0x7b:
		case 0x89:
		case 0x56:
			return state->chipset.card_type >= 0x10 && !alt;
		case 0x79:
		case 0x82:
		case 0x87:
		case 0x85:
			return state->chipset.card_type >= 0x10 && alt;
		case 0x94:
		case 0x95:
			return nv04_pgraph_is_nv15p(&state->chipset) && state->chipset.card_type < 0x20;
		case 0x96:
		case 0x9e:
		case 0x9f:
			return nv04_pgraph_is_nv15p(&state->chipset);
		case 0x93:
			return nv04_pgraph_is_nv11p(&state->chipset) && state->chipset.card_type < 0x20;
		case 0x98:
		case 0x99:
			return nv04_pgraph_is_nv17p(&state->chipset);
		case 0x97:
			return state->chipset.card_type >= 0x20 && state->chipset.chipset != 0x34;
		case 0x597:
			return nv04_pgraph_is_nv25p(&state->chipset);
		case 0x397:
			return state->chipset.card_type == 0x30;
		case 0x497:
			return state->chipset.chipset == 0x35 || state->chipset.chipset == 0x36;
		case 0x697:
			return state->chipset.chipset == 0x34;
		case 0x362:
		case 0x39e:
		case 0x389:
		case 0x38a:
		case 0x37b:
			return state->chipset.card_type == 0x30;
		default:
			return false;
	}
}

bool nv04_pgraph_is_sync(struct pgraph_state *state) {
	if (state->chipset.card_type < 0x10)
		return false;
	if (nv04_pgraph_is_nv11p(&state->chipset)) {
		if (nv04_pgraph_is_syncable_class(state)) {
			if (pgraph_grobj_get_sync(state))
				return true;
		}
	} else if (nv04_pgraph_is_nv15p(&state->chipset)) {
		if (nv04_pgraph_is_syncable_class(state)) {
			if (extr(state->ctx_switch_a, 26, 1))
				return true;
		}
	}
	if (!nv04_pgraph_is_sync_class(state))
		return false;
	return extr(state->debug_d, 12, 1);
}

int pgraph_3d_class(struct pgraph_state *state) {
	int cls = pgraph_class(state);
	bool alt = extr(state->debug_d, 16, 1) && state->chipset.card_type >= 0x10;
	switch (cls) {
		case 0x48:
			if (state->chipset.chipset <= 0x10)
				return PGRAPH_3D_D3D0;
			else
				return PGRAPH_3D_NONE;
		case 0x54:
		case 0x55:
			if (state->chipset.card_type < 0x20)
				return PGRAPH_3D_D3D56;
			else
				return PGRAPH_3D_NONE;
		case 0x94:
		case 0x95:
			if (state->chipset.card_type >= 0x10 && state->chipset.card_type < 0x20)
				return PGRAPH_3D_D3D56;
			else
				return PGRAPH_3D_NONE;
		case 0x56:
			if (state->chipset.card_type >= 0x10 && !alt && state->chipset.card_type < 0x30)
				return PGRAPH_3D_CELSIUS;
			else
				return PGRAPH_3D_NONE;
		case 0x96:
			if (nv04_pgraph_is_nv15p(&state->chipset) && state->chipset.card_type < 0x30)
				return PGRAPH_3D_CELSIUS;
			else
				return PGRAPH_3D_NONE;
		case 0x98:
		case 0x99:
			if (nv04_pgraph_is_nv17p(&state->chipset))
				return PGRAPH_3D_CELSIUS;
			else
				return PGRAPH_3D_NONE;
		case 0x85:
			if (alt && state->chipset.card_type < 0x30)
				return PGRAPH_3D_CELSIUS;
			else
				return PGRAPH_3D_NONE;
		case 0x97:
			if (state->chipset.card_type >= 0x20 && state->chipset.card_type < 0x40)
				return PGRAPH_3D_KELVIN;
			else
				return PGRAPH_3D_NONE;
		case 0x597:
			if (nv04_pgraph_is_nv25p(&state->chipset) && state->chipset.card_type < 0x40)
				return PGRAPH_3D_KELVIN;
			else
				return PGRAPH_3D_NONE;
		case 0x397:
			if (state->chipset.card_type == 0x30)
				return PGRAPH_3D_RANKINE;
			else
				return PGRAPH_3D_NONE;
		case 0x497:
			if (state->chipset.chipset == 0x35 || state->chipset.chipset == 0x36)
				return PGRAPH_3D_RANKINE;
			else
				return PGRAPH_3D_NONE;
		case 0x697:
			if (state->chipset.chipset == 0x34)
				return PGRAPH_3D_RANKINE;
			else
				return PGRAPH_3D_NONE;
		case 0x3597:
			if (state->chipset.chipset == 0x40)
				return PGRAPH_3D_RANKINE;
			else
				return PGRAPH_3D_NONE;
		case 0x4097:
			if (state->chipset.card_type == 0x40 && !nv04_pgraph_is_nv44p(&state->chipset))
				return PGRAPH_3D_CURIE;
			else
				return PGRAPH_3D_NONE;
		case 0x4497:
			if (state->chipset.card_type == 0x40 && nv04_pgraph_is_nv44p(&state->chipset))
				return PGRAPH_3D_CURIE;
			else
				return PGRAPH_3D_NONE;
	}
	return false;
}

bool nv04_pgraph_is_3d_class(struct pgraph_state *state) {
	return pgraph_3d_class(state) != PGRAPH_3D_NONE;
}

bool nv04_pgraph_is_celsius_class(struct pgraph_state *state) {
	return pgraph_3d_class(state) == PGRAPH_3D_CELSIUS;
}

bool nv04_pgraph_is_kelvin_class(struct pgraph_state *state) {
	return pgraph_3d_class(state) == PGRAPH_3D_KELVIN;
}

bool nv04_pgraph_is_rankine_class(struct pgraph_state *state) {
	return pgraph_3d_class(state) == PGRAPH_3D_RANKINE;
}
