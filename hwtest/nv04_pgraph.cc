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

#include "pgraph.h"
#include "hwtest.h"
#include "old.h"
#include "nvhw/pgraph.h"
#include "nva.h"
#include <initializer_list>

static void nv04_pgraph_gen_state(struct hwtest_ctx *ctx, struct pgraph_state *state) {
	std::mt19937 rnd(jrand48(ctx->rand48));
	pgraph_gen_state(ctx->cnum, rnd, state);
}

static void nv04_pgraph_load_state(struct hwtest_ctx *ctx, struct pgraph_state *state) {
	pgraph_load_state(ctx->cnum, state);
}

static void nv04_pgraph_dump_state(struct hwtest_ctx *ctx, struct pgraph_state *state) {
	pgraph_dump_state(ctx->cnum, state);
}

static int nv04_pgraph_cmp_state(struct pgraph_state *orig, struct pgraph_state *exp, struct pgraph_state *real, bool broke = false) {
	return pgraph_cmp_state(orig, exp, real, broke);
}

static uint32_t nv04_pgraph_gen_dma(struct hwtest_ctx *ctx, struct pgraph_state *state, uint32_t dma[3]) {
	int old_subc = extr(state->ctx_user, 13, 3);
	int new_subc = extr(state->fifo_mthd_st2, 12, 3);
	uint32_t inst;
	if (old_subc != new_subc && extr(state->debug[1], 20, 1)) {
		inst = state->ctx_cache[new_subc][3];
	} else {
		inst = state->ctx_switch[3];
	}
	inst ^= 1 << (jrand48(ctx->rand48) & 0xf);
	for (int i = 0; i < 3; i++) {
		dma[i] = jrand48(ctx->rand48);
	}
	if (jrand48(ctx->rand48) & 1) {
		uint32_t classes[4] = {0x2, 0x3, 0x3d, 0x30};
		insrt(dma[0], 0, 12, classes[jrand48(ctx->rand48) & 3]);
	}
	if (jrand48(ctx->rand48) & 1) {
		insrt(dma[0], 20, 4, 0);
	}
	if (jrand48(ctx->rand48) & 1) {
		insrt(dma[0], 24, 4, 0);
	}
	if (jrand48(ctx->rand48) & 1) {
		dma[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
	}
	if (jrand48(ctx->rand48) & 1) {
		dma[1] |= 0x0000007f;
		dma[1] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
	}
	for (int i = 0; i < 3; i++) {
		nva_wr32(ctx->cnum, 0x700000 | inst << 4 | i << 2, dma[i]);
	}
	inst |= jrand48(ctx->rand48) << 16;
	return inst;
}

static uint32_t nv04_pgraph_gen_ctxobj(struct hwtest_ctx *ctx, struct pgraph_state *state, uint32_t ctxobj[4]) {
	int old_subc = extr(state->ctx_user, 13, 3);
	int new_subc = extr(state->fifo_mthd_st2, 12, 3);
	uint32_t inst;
	if (old_subc != new_subc && extr(state->debug[1], 20, 1)) {
		inst = state->ctx_cache[new_subc][3];
	} else {
		inst = state->ctx_switch[3];
	}
	inst ^= 1 << (jrand48(ctx->rand48) & 0xf);
	for (int i = 0; i < 4; i++) {
		ctxobj[i] = jrand48(ctx->rand48);
	}
	if (jrand48(ctx->rand48) & 1) {
		uint32_t classes[] = {0x30, 0x12, 0x72, 0x19, 0x43, 0x17, 0x57, 0x18, 0x44, 0x42, 0x52, 0x53, 0x58, 0x59, 0x5a, 0x5b, 0x62, 0x93, 0x82, 0x9e};
		insrt(ctxobj[0], 0, 12, classes[nrand48(ctx->rand48) % ARRAY_SIZE(classes)]);
	}
	if (jrand48(ctx->rand48) & 1) {
		ctxobj[0] ^= 1 << (jrand48(ctx->rand48) & 0x1f);
	}
	for (int i = 0; i < 4; i++) {
		nva_wr32(ctx->cnum, 0x700000 | inst << 4 | i << 2, ctxobj[i]);
	}
	inst |= jrand48(ctx->rand48) << 16;
	return inst;
}

static void nv04_pgraph_prep_mthd(struct hwtest_ctx *ctx, uint32_t grobj[4], struct pgraph_state *state, uint32_t cls, uint32_t addr, uint32_t val, bool fix_cls = true) {
	if (extr(state->debug[3], 16, 1) && ctx->chipset.card_type >= 0x10 && fix_cls) {
		if (cls == 0x56)
			cls = 0x85;
		if (cls == 0x62)
			cls = 0x82;
		if (cls == 0x63)
			cls = 0x67;
		if (cls == 0x89)
			cls = 0x87;
		if (cls == 0x7b)
			cls = 0x79;
	}
	int chid = extr(state->ctx_user, 24, 7);
	state->fifo_ptr = 0;
	int subc = extr(addr, 13, 3);
	uint32_t mthd = extr(addr, 2, 11);
	if (state->chipset.card_type < 0x10) {
		state->fifo_mthd_st2 = chid << 15 | addr >> 1 | 1;
	} else {
		state->fifo_mthd_st2 = chid << 20 | subc << 16 | mthd << 2 | 1 << 26;
		insrt(state->ctx_switch[0], 23, 1, 0);
		// XXX: figure this out
		if (mthd != 0x104)
			insrt(state->debug[3], 12, 1, 0);
		uint32_t save = state->ctx_switch[0];
		state->ctx_switch[0] = cls;
		if (nv04_pgraph_is_3d_class(state))
			insrt(state->debug[3], 27, 1, 0);
		state->ctx_switch[0] = save;
		if (nv04_pgraph_is_nv15p(&ctx->chipset)) {
			insrt(state->ctx_user, 31, 1, 0);
			insrt(state->debug[3], 8, 1, 0);
		}
	}
	state->fifo_data_st2[0] = val;
	state->fifo_enable = 1;
	state->ctx_control |= 1 << 16;
	insrt(state->ctx_user, 13, 3, extr(addr, 13, 3));
	int old_subc = extr(state->ctx_user, 13, 3);
	for (int i = 0; i < 4; i++)
		grobj[i] = jrand48(ctx->rand48);
	uint32_t inst;
	if (extr(addr, 2, 11) == 0) {
		insrt(grobj[0], 0, 8, cls);
		if (state->chipset.card_type >= 0x10) {
			insrt(grobj[0], 23, 1, 0);
		}
		inst = val & 0xffff;
	} else if (old_subc != subc && extr(state->debug[1], 20, 1)) {
		bool reload = false;
		if (state->chipset.card_type < 0x10)
			reload = extr(state->debug[1], 15, 1);
		else
			reload = extr(state->debug[3], 14, 1);
		if (reload) {
			insrt(grobj[0], 0, 8, cls);
			if (state->chipset.card_type >= 0x10) {
				insrt(grobj[0], 23, 1, 0);
			}
			if (jrand48(ctx->rand48) & 3)
				grobj[3] = 0;
		} else {
			insrt(state->ctx_cache[subc][0], 0, 8, cls);
			if (nv04_pgraph_is_nv15p(&ctx->chipset)) {
				insrt(state->ctx_cache[subc][0], 23, 1, 0);
			}
			if (jrand48(ctx->rand48) & 3)
				state->ctx_cache[subc][4] = 0;
		}
		inst = state->ctx_cache[subc][3];
	} else {
		insrt(state->ctx_switch[0], 0, 8, cls);
		inst = state->ctx_switch[3];
		if (jrand48(ctx->rand48) & 3)
			state->ctx_switch[4] = 0;
	}
	for (int i = 0; i < 4; i++)
		nva_wr32(ctx->cnum, 0x700000 | inst << 4 | i << 2, grobj[i]);
}

static void nv04_pgraph_mthd(struct pgraph_state *state, uint32_t grobj[4], int trapbit = -1) {
	int subc, old_subc = extr(state->ctx_user, 13, 3);
	bool ctxsw;
	if (state->chipset.card_type < 0x10) {
		subc = extr(state->fifo_mthd_st2, 12, 3);
		state->fifo_mthd_st2 &= ~1;
		ctxsw = extr(state->fifo_mthd_st2, 1, 11) == 0;
	} else {
		subc = extr(state->fifo_mthd_st2, 16, 3);
		state->fifo_mthd_st2 &= ~(1 << 26);
		ctxsw = extr(state->fifo_mthd_st2, 2, 11) == 0;
	}
	if (old_subc != subc || ctxsw) {
		if (ctxsw) {
			state->ctx_cache[subc][3] = state->fifo_data_st2[0] & 0xffff;
		}
		uint32_t ctx_mask;
		if (state->chipset.chipset < 5)
			ctx_mask = 0x0303f0ff;
		else if (state->chipset.card_type < 0x10)
			ctx_mask = 0x7f73f0ff;
		else
			ctx_mask = 0x7f33f0ff;
		bool reload = false;
		if (state->chipset.card_type < 0x10)
			reload = extr(state->debug[1], 15, 1);
		else
			reload = extr(state->debug[3], 14, 1);
		if (reload || ctxsw) {
			state->ctx_cache[subc][0] = grobj[0] & ctx_mask;
			state->ctx_cache[subc][1] = grobj[1] & 0xffff3f03;
			state->ctx_cache[subc][2] = grobj[2];
			state->ctx_cache[subc][4] = grobj[3];
		}
		bool reset = extr(state->debug[2], 28, 1);
		if (state->chipset.card_type >= 0x10)
			reset = extr(state->debug[3], 19, 1);
		insrt(state->debug[1], 0, 1, reset);
		if (reset)
			pgraph_volatile_reset(state);
		insrt(state->ctx_user, 13, 3, subc);
		if (extr(state->debug[1], 20, 1)) {
			for (int i = 0; i < 5; i++)
				state->ctx_switch[i] = state->ctx_cache[subc][i];
		}
	}
	if (trapbit >= 0 && extr(state->ctx_switch[4], trapbit, 1) && state->chipset.card_type >= 0x10) {
		state->intr |= 0x10;
		state->fifo_enable = 0;
	} else {
		if (extr(state->debug[3], 20, 2) == 3 && !ctxsw)
			nv04_pgraph_blowup(state, 2);
	}
	if (state->chipset.card_type < 0x10) {
		insrt(state->trap_addr, 2, 14, extr(state->fifo_mthd_st2, 1, 14));
		insrt(state->trap_addr, 24, 4, extr(state->fifo_mthd_st2, 15, 4));
	} else {
		insrt(state->trap_addr, 2, 11, extr(state->fifo_mthd_st2, 2, 11));
		insrt(state->trap_addr, 16, 3, extr(state->fifo_mthd_st2, 16, 3));
		insrt(state->trap_addr, 20, 5, extr(state->fifo_mthd_st2, 20, 5));
	}
	state->trap_data[0] = state->fifo_data_st2[0];
	state->trap_data[1] = state->fifo_data_st2[1];
}

static uint32_t get_random_class(struct hwtest_ctx *ctx) {
	uint32_t classes_nv4[] = {
		0x10, 0x11, 0x13, 0x15, 0x64, 0x65, 0x66, 0x67,
		0x12, 0x72, 0x43, 0x19, 0x17, 0x57, 0x18, 0x44,
		0x42, 0x52, 0x53, 0x58, 0x59, 0x5a, 0x5b,
		0x38, 0x39,
		0x1c, 0x1d, 0x1e, 0x1f, 0x5c, 0x5d, 0x5e, 0x5f,
		0x21, 0x61, 0x60, 0x37, 0x77, 0x36, 0x76, 0x4a,
		0x4b,
		0x54, 0x55, 0x48,
	};
	uint32_t classes_nv5[] = {
		0x12, 0x72, 0x43, 0x19, 0x17, 0x57, 0x18, 0x44,
		0x42, 0x52, 0x53, 0x58, 0x59, 0x5a, 0x5b,
		0x38, 0x39,
		0x1c, 0x1d, 0x1e, 0x1f, 0x5c, 0x5d, 0x5e, 0x5f,
		0x21, 0x61, 0x60, 0x37, 0x77, 0x36, 0x76, 0x4a,
		0x4b, 0x64, 0x65, 0x66,
		0x54, 0x55, 0x48,
	};
	uint32_t classes_nv10[] = {
		0x12, 0x72, 0x43, 0x19, 0x17, 0x57, 0x18, 0x44,
		0x42, 0x52, 0x53, 0x58, 0x59, 0x5a, 0x5b,
		0x38, 0x39,
		0x1c, 0x1d, 0x1e, 0x1f, 0x5c, 0x5d, 0x5e, 0x5f,
		0x21, 0x61, 0x60, 0x37, 0x77, 0x36, 0x76, 0x4a,
		0x4b, 0x64, 0x65, 0x66,
		0x54, 0x55, 0x48,
		0x62, 0x63, 0x89, 0x8a, 0x93, 0x94, 0x95, 0x7b, 0x56,
	};
	uint32_t classes_nv15[] = {
		0x12, 0x72, 0x43, 0x19, 0x17, 0x57, 0x18, 0x44,
		0x42, 0x52, 0x53, 0x58, 0x59, 0x5a, 0x5b,
		0x38, 0x39,
		0x1c, 0x1d, 0x1e, 0x1f, 0x5c, 0x5d, 0x5e, 0x5f,
		0x21, 0x61, 0x60, 0x37, 0x77, 0x36, 0x76, 0x4a,
		0x4b, 0x64, 0x65, 0x66,
		0x54, 0x55,
		0x62, 0x63, 0x89, 0x8a, 0x93, 0x94, 0x95, 0x7b, 0x56,
		0x96, 0x9e, 0x9f,
	};
	uint32_t classes_nv17[] = {
		0x12, 0x72, 0x43, 0x19, 0x17, 0x57, 0x18, 0x44,
		0x42, 0x52, 0x53, 0x58, 0x59, 0x5a, 0x5b,
		0x38, 0x39,
		0x1c, 0x1d, 0x1e, 0x1f, 0x5c, 0x5d, 0x5e, 0x5f,
		0x21, 0x61, 0x60, 0x37, 0x77, 0x36, 0x76, 0x4a,
		0x4b, 0x64, 0x65, 0x66,
		0x54, 0x55,
		0x62, 0x63, 0x89, 0x8a, 0x93, 0x94, 0x95, 0x7b, 0x56,
		0x96, 0x9e, 0x9f, 0x98, 0x99,
	};
	if (ctx->chipset.chipset == 4)
		return classes_nv4[nrand48(ctx->rand48) % ARRAY_SIZE(classes_nv4)];
	else if (ctx->chipset.card_type < 0x10)
		return classes_nv5[nrand48(ctx->rand48) % ARRAY_SIZE(classes_nv5)];
	else if (!nv04_pgraph_is_nv15p(&ctx->chipset))
		return classes_nv10[nrand48(ctx->rand48) % ARRAY_SIZE(classes_nv10)];
	else if (!nv04_pgraph_is_nv17p(&ctx->chipset))
		return classes_nv15[nrand48(ctx->rand48) % ARRAY_SIZE(classes_nv15)];
	else
		return classes_nv17[nrand48(ctx->rand48) % ARRAY_SIZE(classes_nv17)];
}

static uint32_t get_random_dvd(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type < 0x10 || jrand48(ctx->rand48) & 1)
		return 0x38;
	else
		return 0x88;
}

static uint32_t get_random_surf2d(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type < 0x10 || jrand48(ctx->rand48) & 1)
		return 0x42;
	else
		return 0x62;
}

static uint32_t get_random_surf3d(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type < 0x10 || jrand48(ctx->rand48) & 1)
		return 0x53;
	else
		return 0x93;
}

static uint32_t get_random_swzsurf(struct hwtest_ctx *ctx) {
	if (!nv04_pgraph_is_nv15p(&ctx->chipset) || jrand48(ctx->rand48) & 1)
		return 0x52;
	else
		return 0x9e;
}

static uint32_t get_random_celsius(struct hwtest_ctx *ctx) {
	uint32_t classes[] = {0x56, 0x96, 0x98, 0x99};
	if (!nv04_pgraph_is_nv15p(&ctx->chipset))
		return 0x56;
	else if (!nv04_pgraph_is_nv17p(&ctx->chipset))
		return classes[jrand48(ctx->rand48) & 1];
	else
		return classes[jrand48(ctx->rand48) & 3];
}

static uint32_t get_random_iifc(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset < 5 || jrand48(ctx->rand48) & 1)
		return 0x60;
	else
		return 0x64;
}

static uint32_t get_random_ifc(struct hwtest_ctx *ctx, bool need_new = false) {
	uint32_t classes[] = {0x21, 0x61, 0x65, 0x8a};
	if (ctx->chipset.chipset < 5)
		return classes[need_new + nrand48(ctx->rand48) % (2 - need_new)];
	else if (ctx->chipset.card_type < 0x10)
		return classes[need_new + nrand48(ctx->rand48) % (3 - need_new)];
	else
		return classes[need_new + nrand48(ctx->rand48) % (4 - need_new)];
}

static uint32_t get_random_sifc(struct hwtest_ctx *ctx, bool need_new = false) {
	uint32_t classes[] = {0x36, 0x76, 0x66};
	if (ctx->chipset.chipset < 5)
		return classes[need_new + nrand48(ctx->rand48) % (2 - need_new)];
	else
		return classes[need_new + nrand48(ctx->rand48) % (3 - need_new)];
}

static uint32_t get_random_sifm(struct hwtest_ctx *ctx, bool need_new = false) {
	uint32_t classes[] = {0x37, 0x77, 0x63, 0x89};
	if (ctx->chipset.card_type < 0x10)
		return classes[need_new + nrand48(ctx->rand48) % (2 - need_new)];
	else
		return classes[need_new + nrand48(ctx->rand48) % (4 - need_new)];
}

static uint32_t get_random_blit(struct hwtest_ctx *ctx, bool need_new = false) {
	if (need_new)
		return 0x5f;
	uint32_t classes[] = {0x1f, 0x5f};
	return classes[nrand48(ctx->rand48) % 2];
}

static uint32_t get_random_d3d5(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type < 0x10 || jrand48(ctx->rand48) & 1)
		return 0x54;
	else
		return 0x94;
}

static uint32_t get_random_d3d6(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type < 0x10 || jrand48(ctx->rand48) & 1)
		return 0x55;
	else
		return 0x95;
}

static int test_invalid_class(struct hwtest_ctx *ctx) {
	int i;
	for (int cls = 0; cls < 0x100; cls++) {
		switch (cls) {
			case 0x10:
			case 0x11:
			case 0x13:
			case 0x15:
				if (ctx->chipset.chipset == 4)
					continue;
				break;
			case 0x67:
				if (ctx->chipset.chipset == 4 || ctx->chipset.card_type >= 0x10)
					continue;
				break;
			case 0x64:
			case 0x65:
			case 0x66:
			case 0x12:
			case 0x72:
			case 0x43:
			case 0x19:
			case 0x17:
			case 0x57:
			case 0x18:
			case 0x44:
			case 0x42:
			case 0x52:
			case 0x53:
			case 0x58:
			case 0x59:
			case 0x5a:
			case 0x5b:
			case 0x38:
			case 0x39:
			case 0x1c:
			case 0x1d:
			case 0x1e:
			case 0x1f:
			case 0x21:
			case 0x36:
			case 0x37:
			case 0x5c:
			case 0x5d:
			case 0x5e:
			case 0x5f:
			case 0x60:
			case 0x61:
			case 0x76:
			case 0x77:
			case 0x4a:
			case 0x4b:
			case 0x54:
			case 0x55:
				continue;
			case 0x48:
				if (!nv04_pgraph_is_nv15p(&ctx->chipset))
					continue;
				break;
			case 0x56:
			case 0x62:
			case 0x63:
			case 0x79:
			case 0x7b:
			case 0x82:
			case 0x85:
			case 0x87:
			case 0x88:
			case 0x89:
			case 0x8a:
			case 0x93:
			case 0x94:
			case 0x95:
				if (ctx->chipset.card_type >= 0x10)
					continue;
				break;
			case 0x96:
			case 0x9e:
			case 0x9f:
				if (nv04_pgraph_is_nv15p(&ctx->chipset))
					continue;
			case 0x98:
			case 0x99:
				if (nv04_pgraph_is_nv17p(&ctx->chipset))
					continue;
				break;
		}
		for (i = 0; i < 20; i++) {
			uint32_t val = jrand48(ctx->rand48);
			uint32_t mthd = jrand48(ctx->rand48) & 0x1ffc;
			if (mthd == 0 || mthd == 0x100 || i < 10)
				mthd = 0x104;
			uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
			struct pgraph_state orig, exp, real;
			nv04_pgraph_gen_state(ctx, &orig);
			orig.notify &= ~0x10000;
			uint32_t grobj[4];
			nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
			nv04_pgraph_load_state(ctx, &orig);
			exp = orig;
			nv04_pgraph_mthd(&exp, grobj);
			nv04_pgraph_blowup(&exp, 0x0040);
			nv04_pgraph_dump_state(ctx, &real);
			if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
				printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
				return HWTEST_RES_FAIL;
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_op(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset != 4)
		return HWTEST_RES_NA;
	for (int cls : {0x10, 0x11, 0x13, 0x15, 0x64, 0x65, 0x66, 0x67}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200 || mthd == 0x204)
				continue;
			if (mthd == 0x208 && (cls == 0x11 || cls == 0x13 || cls == 0x66 || cls == 0x67))
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_ctx(struct hwtest_ctx *ctx) {
	for (int cls : {0x12, 0x72, 0x43, 0x19, 0x17, 0x57, 0x18, 0x44}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if ((cls == 0x12 || cls == 0x72 || cls == 0x43) && mthd == 0x300)
				continue;
			if (cls == 0x19 && (mthd == 0x300 || mthd == 0x304))
				continue;
			if ((cls == 0x17 || cls == 0x57) && (mthd == 0x300 || mthd == 0x304))
				continue;
			if ((cls == 0x18 || cls == 0x44) && (mthd == 0x300 || mthd == 0x304 || mthd == 0x308 || (mthd & 0x1ff0) == 0x310))
				continue;
			if (cls == 0x44 && (mthd == 0x30c ||
				(mthd & 0x1fc0) == 0x400 ||
				(mthd & 0x1f80) == 0x500 ||
				(mthd & 0x1f80) == 0x600 ||
				(mthd & 0x1f00) == 0x700))
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_surf(struct hwtest_ctx *ctx) {
	for (int cls : {0x42, 0x62, 0x82, 0x52, 0x9e, 0x53, 0x93, 0x58, 0x59, 0x5a, 0x5b}) {
		if ((cls == 0x62 || cls == 0x82 || cls == 0x93) && ctx->chipset.card_type < 0x10)
			continue;
		if (cls == 0x9e && !nv04_pgraph_is_nv15p(&ctx->chipset))
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if ((cls == 0x62 || cls == 0x82) && mthd == 0x108)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x184)
				continue;
			if ((cls == 0x42 || cls == 0x62 || cls == 0x82 || cls == 0x53 || cls == 0x93) && mthd == 0x188)
				continue;
			if ((cls == 0x42 || cls == 0x62 || cls == 0x82 || cls == 0x52 || cls == 0x9e) && (mthd & 0x1f00) == 0x200)
				continue;
			if ((cls == 0x42 || cls == 0x62 || cls == 0x82) && (mthd >= 0x300 && mthd <= 0x30c))
				continue;
			if ((cls == 0x52 || cls == 0x9e) && (mthd >= 0x300 && mthd <= 0x304))
				continue;
			if ((cls == 0x53 || cls == 0x93) && (mthd >= 0x300 && mthd <= 0x310))
				continue;
			if ((cls == 0x53 || cls == 0x93) && (mthd == 0x2f8 || mthd == 0x2fc) && ctx->chipset.chipset >= 5)
				continue;
			if ((cls & 0xfc) == 0x58 && ((mthd & 0x1ff8) == 0x200 || mthd == 0x300 || mthd == 0x308 || mthd == 0x30c))
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_dvd(struct hwtest_ctx *ctx) {
	for (int cls : {0x38, 0x88}) {
		if (cls == 0x88 && ctx->chipset.card_type < 0x10)
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x108 && cls == 0x88)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd >= 0x180 && mthd <= 0x18c)
				continue;
			if (mthd >= 0x300 && mthd <= 0x33c)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_m2mf(struct hwtest_ctx *ctx) {
	for (int cls : {0x39}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd >= 0x180 && mthd <= 0x188)
				continue;
			if (mthd >= 0x30c && mthd <= 0x328)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_lin(struct hwtest_ctx *ctx) {
	for (int cls : {0x1c, 0x5c}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x10c && ctx->chipset.card_type < 0x10)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x300 || mthd == 0x304)
				continue;
			if (mthd >= 0x400 && mthd <= 0x67c)
				continue;
			if (cls == 0x1c && mthd >= 0x184 && mthd <= 0x194 && ctx->chipset.chipset >= 5)
				continue;
			if (cls == 0x5c && mthd >= 0x184 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_tri(struct hwtest_ctx *ctx) {
	for (int cls : {0x1d, 0x5d}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x10c && ctx->chipset.card_type < 0x10)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x300 || mthd == 0x304)
				continue;
			if (mthd >= 0x310 && mthd <= 0x318)
				continue;
			if (mthd >= 0x320 && mthd <= 0x334)
				continue;
			if (mthd >= 0x400 && mthd <= 0x5fc)
				continue;
			if (cls == 0x1d && mthd >= 0x184 && mthd <= 0x194 && ctx->chipset.chipset >= 5)
				continue;
			if (cls == 0x5d && mthd >= 0x184 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_rect(struct hwtest_ctx *ctx) {
	for (int cls : {0x1e, 0x5e}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x10c && ctx->chipset.card_type < 0x10)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x300 || mthd == 0x304)
				continue;
			if (mthd >= 0x400 && mthd <= 0x47c)
				continue;
			if (cls == 0x1e && mthd >= 0x184 && mthd <= 0x194 && ctx->chipset.chipset >= 5)
				continue;
			if (cls == 0x5e && mthd >= 0x184 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_blit(struct hwtest_ctx *ctx) {
	for (int cls : {0x1f, 0x5f, 0x9f}) {
		if (cls == 0x9f && !nv04_pgraph_is_nv15p(&ctx->chipset))
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if ((mthd == 0x108 || mthd == 0x10c) && cls == 0x9f)
				continue;
			if ((mthd >= 0x120 && mthd <= 0x134) && cls == 0x9f)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x10c && ctx->chipset.card_type < 0x10)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200 || mthd == 0x204)
				continue;
			if (mthd == 0x300 || mthd == 0x304 || mthd == 0x308)
				continue;
			if (mthd >= 0x184 && mthd <= 0x19c && ctx->chipset.chipset >= 5)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_ifc(struct hwtest_ctx *ctx) {
	for (int cls : {0x21, 0x61, 0x65, 0x8a}) {
		if (cls == 0x65 && ctx->chipset.chipset < 5)
			continue;
		if (cls == 0x8a && ctx->chipset.card_type < 0x10)
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x108 && cls == 0x8a)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x10c && ctx->chipset.card_type < 0x10)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x300 || mthd == 0x304 || mthd == 0x308 || mthd == 0x30c)
				continue;
			if (cls == 0x21 && mthd >= 0x400 && mthd < 0x480)
				continue;
			if (cls != 0x21 && mthd >= 0x400)
				continue;
			if (mthd == 0x2f8 && (cls != 0x21 && cls != 0x61) && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			if (cls == 0x21 && mthd >= 0x184 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			if (cls != 0x21 && mthd >= 0x184 && mthd <= 0x19c && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_sifc(struct hwtest_ctx *ctx) {
	for (int cls : {0x36, 0x76, 0x66}) {
		if (cls == 0x66 && ctx->chipset.chipset < 5)
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x10c && ctx->chipset.card_type < 0x10)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd >= 0x300 && mthd <= 0x318)
				continue;
			if (mthd >= 0x400)
				continue;
			if (mthd == 0x2f8 && (cls != 0x36 && cls != 0x76) && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			if (cls == 0x36 && mthd >= 0x184 && mthd <= 0x194 && ctx->chipset.chipset >= 5)
				continue;
			if (cls != 0x36 && mthd >= 0x184 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_iifc(struct hwtest_ctx *ctx) {
	for (int cls : {0x60, 0x64}) {
		if (cls == 0x64 && ctx->chipset.chipset < 5)
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x10c && ctx->chipset.card_type < 0x10)
				continue;
			if (mthd == 0x180 || mthd == 0x184)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd >= 0x3e8)
				continue;
			if (mthd == 0x3e0 && cls != 0x60 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x3e4 && ctx->chipset.chipset >= 5)
				continue;
			if (mthd >= 0x188 && mthd <= 0x1a0 && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_sifm(struct hwtest_ctx *ctx) {
	for (int cls : {0x37, 0x77, 0x63, 0x89, 0x67, 0x87}) {
		if ((cls == 0x63 || cls == 0x89 || cls == 0x67 || cls == 0x87) && ctx->chipset.card_type < 0x10)
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if ((cls == 0x89 || cls == 0x87) && mthd == 0x108)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x10c && ctx->chipset.card_type < 0x10)
				continue;
			if (mthd == 0x180 || mthd == 0x184)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x300)
				continue;
			if (mthd == 0x304 && ctx->chipset.chipset >= 5)
				continue;
			if (mthd >= 0x308 && mthd <= 0x31c)
				continue;
			if (mthd >= 0x400 && mthd <= 0x40c)
				continue;
			if (cls != 0x37 && cls != 0x77 && mthd == 0x2fc)
				continue;
			if (cls == 0x37 && mthd >= 0x188 && mthd <= 0x194 && ctx->chipset.chipset >= 5)
				continue;
			if (cls != 0x37 && mthd >= 0x188 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_gdi_nv3(struct hwtest_ctx *ctx) {
	for (int cls : {0x4b}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x10c && ctx->chipset.card_type < 0x10)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			if (mthd == 0x300 || mthd == 0x304)
				continue;
			if (mthd >= 0x3fc && mthd <= 0x5fc)
				continue;
			if (mthd >= 0x7f4 && mthd <= 0x9fc)
				continue;
			if (mthd >= 0xbec && mthd <= 0xdfc)
				continue;
			if (mthd >= 0xfe8 && mthd <= 0x11fc)
				continue;
			if (mthd >= 0x13e4 && mthd <= 0x15fc)
				continue;
			if (mthd >= 0x184 && mthd <= 0x190 && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_gdi_nv4(struct hwtest_ctx *ctx) {
	for (int cls : {0x4a}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x10c && ctx->chipset.card_type < 0x10)
				continue;
			if (mthd == 0x180 || mthd == 0x184)
				continue;
			if (mthd == 0x200)
				continue;
			if (mthd == 0x2fc && ctx->chipset.chipset >= 5)
				continue;
			if (mthd == 0x300 || mthd == 0x304)
				continue;
			if (mthd >= 0x3fc && mthd <= 0x4fc)
				continue;
			if (mthd >= 0x5f4 && mthd <= 0x6fc)
				continue;
			if (mthd >= 0x7ec && mthd <= 0xbfc)
				continue;
			if (mthd >= 0xbe4 && mthd <= 0xffc)
				continue;
			if (mthd >= 0xff0 && mthd <= 0x13fc)
				continue;
			if (mthd >= 0x17f0 && mthd <= 0x1ffc)
				continue;
			if (mthd >= 0x188 && mthd <= 0x198 && ctx->chipset.chipset >= 5)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_tfc(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type < 0x10)
		return HWTEST_RES_NA;
	for (int cls : {0x79, 0x7b}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x108)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x180)
				continue;
			if (mthd == 0x184)
				continue;
			if (mthd >= 0x2fc && mthd <= 0x310)
				continue;
			if (mthd >= 0x400)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_d3d0(struct hwtest_ctx *ctx) {
	if (nv04_pgraph_is_nv15p(&ctx->chipset))
		return HWTEST_RES_NA;
	for (int cls : {0x48}) {
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x10c && ctx->chipset.card_type < 0x10)
				continue;
			if (mthd == 0x180 || mthd == 0x184)
				continue;
			if ((mthd == 0x188 || mthd == 0x18c || mthd == 0x190) && ctx->chipset.chipset >= 5)
				continue;
			if ((mthd & 0x1ff0) == 0x200)
				continue;
			if (mthd >= 0x304 && mthd <= 0x318)
				continue;
			if (mthd >= 0x1000)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_d3d5(struct hwtest_ctx *ctx) {
	for (int cls : {0x54, 0x94}) {
		if (cls == 0x94 && ctx->chipset.card_type < 0x10)
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x180 || mthd == 0x184 || mthd == 0x188 || mthd == 0x18c)
				continue;
			if ((mthd & 0x1ff0) == 0x200)
				continue;
			if (mthd >= 0x300 && mthd <= 0x318)
				continue;
			if (mthd >= 0x400 && mthd < 0x700)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_d3d6(struct hwtest_ctx *ctx) {
	for (int cls : {0x55, 0x95}) {
		if (cls == 0x95 && ctx->chipset.card_type < 0x10)
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd == 0x100)
				continue;
			if (mthd == 0x104)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd == 0x180 || mthd == 0x184 || mthd == 0x188 || mthd == 0x18c)
				continue;
			if ((mthd & 0x1ff0) == 0x200)
				continue;
			if (mthd >= 0x308 && mthd <= 0x324)
				continue;
			if (mthd >= 0x32c && mthd <= 0x348)
				continue;
			if (mthd >= 0x400 && mthd < 0x600)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_invalid_mthd_celsius(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type < 0x10)
		return HWTEST_RES_NA;
	for (int cls : {0x85, 0x56, 0x96, 0x98, 0x99}) {
		if (cls == 0x96 && !nv04_pgraph_is_nv15p(&ctx->chipset))
			continue;
		if ((cls == 0x98 || cls == 0x99) && !nv04_pgraph_is_nv17p(&ctx->chipset))
			continue;
		for (int mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0)
				continue;
			if (mthd >= 0x100 && mthd <= 0x110)
				continue;
			if (mthd == 0x114 && cls >= 0x96)
				continue;
			if ((mthd >= 0x120 && mthd <= 0x130) && cls >= 0x96)
				continue;
			if (mthd == 0x140 && ctx->chipset.card_type >= 0x10)
				continue;
			if (mthd >= 0x180 && mthd <= 0x198)
				continue;
			if (mthd >= 0x1ac && mthd <= 0x1b0 && cls == 0x99)
				continue;
			if (mthd >= 0x200 && mthd < 0x258)
				continue;
			if (mthd >= 0x258 && mthd < 0x260 && cls == 0x99)
				continue;
			if (mthd >= 0x260 && mthd < 0x2b8)
				continue;
			if (mthd >= 0x2b8 && mthd < 0x2c0 && cls == 0x99)
				continue;
			if (mthd >= 0x2c0 && mthd < 0x3fc)
				continue;
			if (mthd == 0x3fc && cls == 0x99)
				continue;
			if (mthd >= 0x400 && mthd < 0x5c0)
				continue;
			if (mthd >= 0x600 && mthd < 0x69c)
				continue;
			if (mthd >= 0x6a0 && mthd < 0x6b8)
				continue;
			if (mthd >= 0x6c4 && mthd < 0x6d0)
				continue;
			if (mthd >= 0x6e8 && mthd < 0x728)
				continue;
			if ((mthd & 0x1c7c) >= 0x800 && (mthd & 0x1c7c) < 0x874)
				continue;
			if (mthd >= 0xc00 && mthd < 0xc0c)
				continue;
			if (mthd >= 0xc10 && mthd < 0xc3c)
				continue;
			if (mthd >= 0xc40 && mthd < 0xc9c)
				continue;
			if (mthd >= 0xca0 && mthd < 0xcc4)
				continue;
			if (mthd >= 0xcc8 && mthd < 0xce8)
				continue;
			if (mthd >= 0xcec && mthd < 0xd40)
				continue;
			if ((mthd >= 0xd40 && mthd <= 0xd44) && cls >= 0x96)
				continue;
			if (mthd >= 0xd54 && mthd < 0xd88 && cls == 0x99)
				continue;
			if (mthd >= 0xdfc && mthd < 0x1000)
				continue;
			if (mthd >= 0x10fc && mthd < 0x1200)
				continue;
			if (mthd >= 0x13fc && mthd < 0x1600)
				continue;
			if (mthd >= 0x17fc)
				continue;
			for (int i = 0; i < 10; i++) {
				uint32_t val = jrand48(ctx->rand48);
				uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
				struct pgraph_state orig, exp, real;
				nv04_pgraph_gen_state(ctx, &orig);
				orig.notify &= ~0x10000;
				uint32_t grobj[4];
				nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
				nv04_pgraph_load_state(ctx, &orig);
				exp = orig;
				nv04_pgraph_mthd(&exp, grobj);
				nv04_pgraph_blowup(&exp, 0x0040);
				nv04_pgraph_dump_state(ctx, &real);
				if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
					printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
					return HWTEST_RES_FAIL;
				}
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctxsw(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = jrand48(ctx->rand48) & 0xff;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000);
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val, false);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_nop(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = jrand48(ctx->rand48) & 0xff;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x100;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val, false);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_notify(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xf;
		uint32_t cls = get_random_class(ctx);
		int trapbit = 0;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x104;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[1] &= 0x000fffff;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		if (nv04_pgraph_is_async(&exp))
			trapbit = -1;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (nv04_pgraph_is_async(&exp)) {
			nv04_pgraph_blowup(&exp, 0x40);
		} else if (!extr(exp.intr, 4, 1)) {
			int rval = val & 1;
			if (ctx->chipset.card_type >= 0x10)
				rval = val & 3;
			if (extr(exp.notify, 16, 1))
				nv04_pgraph_blowup(&exp, 0x1000);
			if (extr(exp.debug[3], 20, 1) && val > 1)
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.ctx_switch[1], 16, 16))
				pgraph_state_error(&exp);
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.notify, 16, 1, rval < 2);
				insrt(exp.notify, 20, 1, rval & 1);
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sync(struct hwtest_ctx *ctx) {
	int i;
	if (ctx->chipset.card_type < 0x10)
		return HWTEST_RES_NA;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xf;
		uint32_t cls, mthd;
		int trapbit = -1;
		switch (nrand48(ctx->rand48) % 10) {
			default:
				cls = get_random_celsius(ctx);
				mthd = 0x110;
				trapbit = 1;
				break;
			case 1:
				cls = 0x62;
				mthd = 0x108;
				break;
			case 2:
				cls = 0x8a;
				mthd = 0x108;
				break;
			case 3:
				cls = 0x7b;
				mthd = 0x108;
				trapbit = 1;
				break;
			case 4:
				cls = 0x89;
				mthd = 0x108;
				break;
			case 5:
				cls = 0x88;
				mthd = 0x108;
				break;
			case 6:
				if (!nv04_pgraph_is_nv15p(&ctx->chipset))
					continue;
				cls = 0x9f;
				mthd = 0x108;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pm_trigger(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type < 0x10)
		return HWTEST_RES_NA;
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = jrand48(ctx->rand48) & 0xff;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x140;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val, false);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		if (!extr(exp.debug[3], 15, 1)) {
			nv04_pgraph_blowup(&exp, 0x40);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_missing(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls;
		uint32_t mthd;
		switch (nrand48(ctx->rand48) % 50) {
			case 0:
			default:
				cls = 0x10;
				mthd = 0x200 + (nrand48(ctx->rand48) % 2) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 1:
				cls = 0x11;
				mthd = 0x200 + (nrand48(ctx->rand48) % 3) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 2:
				cls = 0x13;
				mthd = 0x200 + (nrand48(ctx->rand48) % 3) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 3:
				cls = 0x15;
				mthd = 0x200 + (nrand48(ctx->rand48) % 2) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 4:
				cls = 0x64;
				mthd = 0x200 + (nrand48(ctx->rand48) % 2) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 5:
				cls = 0x65;
				mthd = 0x200 + (nrand48(ctx->rand48) % 2) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 6:
				cls = 0x66;
				mthd = 0x200 + (nrand48(ctx->rand48) % 3) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 7:
				cls = 0x67;
				mthd = 0x200 + (nrand48(ctx->rand48) % 3) * 4;
				if (ctx->chipset.chipset != 4)
					continue;
				break;
			case 8:
				cls = 0x12;
				mthd = 0x200;
				break;
			case 9:
				cls = 0x72;
				mthd = 0x200;
				break;
			case 10:
				cls = 0x43;
				mthd = 0x200;
				break;
			case 11:
				cls = 0x17;
				mthd = 0x200;
				break;
			case 12:
				cls = 0x57;
				mthd = 0x200;
				break;
			case 13:
				cls = 0x18;
				mthd = 0x200;
				break;
			case 14:
				cls = 0x44;
				mthd = 0x200;
				break;
			case 15:
				cls = 0x19;
				mthd = 0x200;
				break;
			case 16:
				cls = get_random_surf2d(ctx);
				mthd = 0x200 | (jrand48(ctx->rand48) & 0xfc);
				break;
			case 17:
				cls = get_random_swzsurf(ctx);
				mthd = 0x200 | (jrand48(ctx->rand48) & 0xfc);
				break;
			case 18:
				cls = 0x58;
				mthd = 0x200 | (jrand48(ctx->rand48) & 4);
				break;
			case 19:
				cls = 0x59;
				mthd = 0x200 | (jrand48(ctx->rand48) & 4);
				break;
			case 20:
				cls = 0x5a;
				mthd = 0x200 | (jrand48(ctx->rand48) & 4);
				break;
			case 21:
				cls = 0x5b;
				mthd = 0x200 | (jrand48(ctx->rand48) & 4);
				break;
			case 22:
				cls = 0x1c;
				mthd = 0x200;
				break;
			case 23:
				cls = 0x5c;
				mthd = 0x200;
				break;
			case 24:
				cls = 0x1d;
				mthd = 0x200;
				break;
			case 25:
				cls = 0x5d;
				mthd = 0x200;
				break;
			case 26:
				cls = 0x1e;
				mthd = 0x200;
				break;
			case 27:
				cls = 0x5e;
				mthd = 0x200;
				break;
			case 28:
				cls = get_random_blit(ctx);
				mthd = 0x200 | (jrand48(ctx->rand48) & 4);
				break;
			case 29:
				cls = get_random_ifc(ctx);
				mthd = 0x200;
				break;
			case 30:
				cls = get_random_sifc(ctx);
				mthd = 0x200;
				break;
			case 31:
				cls = get_random_sifm(ctx);
				mthd = 0x200;
				break;
			case 32:
				cls = get_random_iifc(ctx);
				mthd = 0x200;
				break;
			case 33:
				cls = 0x4a;
				mthd = 0x200;
				break;
			case 34:
				cls = 0x4b;
				mthd = 0x200;
				break;
			case 35:
				if (nv04_pgraph_is_nv15p(&ctx->chipset))
					continue;
				cls = 0x48;
				mthd = 0x200 | (jrand48(ctx->rand48) & 0xc);
				break;
			case 36:
				if (ctx->chipset.card_type < 0x10)
					continue;
				cls = get_random_celsius(ctx);
				mthd = 0x10c;
				break;
		}
		if (jrand48(ctx->rand48) & 1)
			val &= 0xf;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.intr |= 0x10;
		exp.fifo_enable = 0;
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_beta(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x12;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x300;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 2);
		if (!extr(exp.intr, 4, 1)) {
			exp.beta = val;
			if (exp.beta & 0x80000000)
				exp.beta = 0;
			exp.beta &= 0x7f800000;
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_beta4(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x72;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x300;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 2);
		if (!extr(exp.intr, 4, 1)) {
			exp.beta4 = val;
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_rop(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x43;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x300;
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 2);
		if (!extr(exp.intr, 4, 1)) {
			exp.rop = val & 0xff;
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_chroma_nv1(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x17;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x304;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 3);
		if (!extr(exp.intr, 4, 1)) {
			nv04_pgraph_set_chroma_nv01(&exp, val);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_chroma_nv4(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x57;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x304;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 3);
		if (!extr(exp.intr, 4, 1)) {
			exp.chroma = val;
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_shape(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = (jrand48(ctx->rand48) & 1 ? 0x18 : 0x44);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x308;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1)
			val &= 0xf;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 4);
		if (!extr(exp.intr, 4, 1)) {
			insrt(exp.pattern_config, 0, 2, val);
			if (extr(exp.debug[3], 20, 1) && val > 2)
				nv04_pgraph_blowup(&exp, 2);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_select(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x44;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x30c;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1)
			val &= 0xf;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 5);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rv = val & 3;
			insrt(exp.pattern_config, 4, 1, rv > 1);
			if (extr(exp.debug[3], 20, 1) && (val > 2 || val == 0)) {
				nv04_pgraph_blowup(&exp, 2);
			}
			insrt(exp.ctx_valid, 22, 1, !extr(exp.nsource, 1, 1));
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_mono_color_nv1(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x18;
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x310 | idx << 2;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 5+idx);
		if (!extr(exp.intr, 4, 1)) {
			nv04_pgraph_set_pattern_mono_color_nv01(&exp, idx, val);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_mono_color_nv4(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x44;
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x310 | idx << 2;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 6+idx);
		if (!extr(exp.intr, 4, 1)) {
			exp.pattern_mono_color[idx] = val;
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_mono_bitmap(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = (jrand48(ctx->rand48) & 1 ? 0x18 : 0x44);
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x318 | idx << 2;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1)
			val &= 0xf;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, (cls == 0x18 ? 7 : 8)+idx);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rval = nv04_pgraph_bswap(&exp, val);
			exp.pattern_mono_bitmap[idx] = pgraph_expand_mono(&exp, rval);
			if (cls == 0x18) {
				uint32_t fmt = extr(exp.ctx_switch[1], 0, 2);
				if (fmt != 1 && fmt != 2) {
					pgraph_state_error(&exp);
				}
				insrt(exp.ctx_valid, 26+idx, 1, !extr(exp.nsource, 1, 1));
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_color_y8(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x44;
		int idx = jrand48(ctx->rand48) & 0xf;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x400 | idx << 2;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 10);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rval = nv04_pgraph_bswap(&exp, val);
			for (int i = 0; i < 4; i++)
				exp.pattern_color[idx*4+i] = extr(rval, 8*i, 8) * 0x010101;
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_color_r5g6b5(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x44;
		int idx = jrand48(ctx->rand48) & 0x1f;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x500 | idx << 2;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 12);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rval = nv04_pgraph_hswap(&exp, val);
			for (int i = 0; i < 2; i++) {
				uint8_t b = extr(rval, i * 16 + 0, 5) * 0x21 >> 2;
				uint8_t g = extr(rval, i * 16 + 5, 6) * 0x41 >> 4;
				uint8_t r = extr(rval, i * 16 + 11, 5) * 0x21 >> 2;
				exp.pattern_color[idx*2+i] = b | g << 8 | r << 16;
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_color_r5g5b5(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x44;
		int idx = jrand48(ctx->rand48) & 0x1f;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x600 | idx << 2;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 11);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rval = nv04_pgraph_hswap(&exp, val);
			for (int i = 0; i < 2; i++) {
				uint8_t b = extr(rval, i * 16 + 0, 5) * 0x21 >> 2;
				uint8_t g = extr(rval, i * 16 + 5, 5) * 0x21 >> 2;
				uint8_t r = extr(rval, i * 16 + 10, 5) * 0x21 >> 2;
				exp.pattern_color[idx*2+i] = b | g << 8 | r << 16;
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_color_r8g8b8(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x44;
		int idx = jrand48(ctx->rand48) & 0x3f;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x700 | idx << 2;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 13);
		if (!extr(exp.intr, 4, 1)) {
			exp.pattern_color[idx] = extr(val, 0, 24);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_offset(struct hwtest_ctx *ctx) {
	int i;
	uint32_t offset_mask = pgraph_offset_mask(&ctx->chipset);
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1)
			val &= ~0xf;
		if (jrand48(ctx->rand48) & 1)
			val &= ~0xfc000000;
		uint32_t cls;
		uint32_t mthd;
		int idx;
		bool isnew = false;
		bool isnv10 = false;
		int trapbit;
		switch (nrand48(ctx->rand48) % 12) {
			case 0:
			default:
				cls = 0x58;
				mthd = 0x30c;
				idx = 0;
				trapbit = 5;
				break;
			case 1:
				cls = 0x59;
				mthd = 0x30c;
				idx = 1;
				trapbit = 5;
				break;
			case 2:
				cls = 0x5a;
				mthd = 0x30c;
				idx = 2;
				trapbit = 5;
				break;
			case 3:
				cls = 0x5b;
				mthd = 0x30c;
				idx = 3;
				trapbit = 5;
				break;
			case 4:
				cls = get_random_surf2d(ctx);
				mthd = 0x308;
				idx = 1;
				isnew = true;
				isnv10 = cls != 0x42;
				trapbit = 6;
				break;
			case 5:
				cls = get_random_surf2d(ctx);
				mthd = 0x30c;
				idx = 0;
				isnew = true;
				isnv10 = cls != 0x42;
				trapbit = 7;
				break;
			case 6:
				cls = get_random_swzsurf(ctx);
				mthd = 0x304;
				idx = 5;
				isnew = true;
				isnv10 = cls != 0x52;
				trapbit = 4;
				break;
			case 7:
				cls = get_random_dvd(ctx);
				mthd = 0x30c;
				idx = 4;
				isnew = true;
				trapbit = 8;
				break;
			case 8:
				cls = get_random_surf3d(ctx);
				mthd = 0x30c;
				idx = 2;
				isnew = true;
				isnv10 = cls != 0x53;
				trapbit = 9;
				break;
			case 9:
				cls = get_random_surf3d(ctx);
				mthd = 0x310;
				idx = 3;
				isnew = true;
				isnv10 = cls != 0x53;
				trapbit = 10;
				break;
			case 10:
				cls = get_random_celsius(ctx);
				if (ctx->chipset.card_type < 0x10)
					continue;
				mthd = 0x210;
				idx = 2;
				isnew = isnv10 = true;
				trapbit = 13;
				break;
			case 11:
				cls = get_random_celsius(ctx);
				if (ctx->chipset.card_type < 0x10)
					continue;
				mthd = 0x214;
				idx = 3;
				isnew = isnv10 = true;
				trapbit = 14;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			exp.surf_offset[idx] = val & offset_mask;
			exp.valid[0] |= 8;
			bool bad = !!(val & 0xf);
			if (isnew)
				bad |= !!(val & 0x1f);
			if (isnv10)
				bad |= !!(val & 0x3f);
			if (extr(exp.debug[3], 20, 1) && bad) {
				nv04_pgraph_blowup(&exp, 0x2);
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_pitch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff000f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls;
		uint32_t mthd;
		int idx;
		switch (nrand48(ctx->rand48) % 4) {
			case 0:
			default:
				cls = 0x58;
				mthd = 0x308;
				idx = 0;
				break;
			case 1:
				cls = 0x59;
				mthd = 0x308;
				idx = 1;
				break;
			case 2:
				cls = 0x5a;
				mthd = 0x308;
				idx = 2;
				break;
			case 3:
				cls = 0x5b;
				mthd = 0x308;
				idx = 3;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 4);
		if (!extr(exp.intr, 4, 1)) {
			exp.surf_pitch[idx] = val & pgraph_pitch_mask(&ctx->chipset);
			exp.valid[0] |= 4;
			bool bad = !!(val & ~0x1ff0) || !(val & 0x1ff0);
			if (extr(exp.debug[3], 20, 1) && bad) {
				nv04_pgraph_blowup(&exp, 0x2);
			}
			insrt(exp.ctx_valid, 8+idx, 1, !extr(exp.nsource, 1, 1));
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_pitch_2(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xe00fe00f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff0000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls;
		uint32_t mthd;
		int idx0, idx1;
		int trapbit;
		bool isnv10 = false;
		switch (nrand48(ctx->rand48) % 3) {
			case 0:
			default:
				cls = get_random_surf2d(ctx);
				mthd = 0x304;
				idx0 = 1;
				idx1 = 0;
				trapbit = 5;
				isnv10 = cls != 0x42;
				break;
			case 1:
				cls = get_random_surf3d(ctx);
				mthd = 0x308;
				idx0 = 2;
				idx1 = 3;
				trapbit = 8;
				isnv10 = cls != 0x53;
				break;
			case 2:
				cls = get_random_celsius(ctx);
				if (ctx->chipset.card_type < 0x10)
					continue;
				mthd = 0x20c;
				idx0 = 2;
				idx1 = 3;
				isnv10 = true;
				trapbit = 12;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t pitch_mask = pgraph_pitch_mask(&ctx->chipset);
			exp.surf_pitch[idx0] = val & pitch_mask;
			exp.surf_pitch[idx1] = val >> 16 & pitch_mask;
			exp.valid[0] |= 4;
			bool bad = false;
			if (!isnv10) {
				if (val & ~0x1fe01fe0)
					bad = true;
				if (!(val & 0x1ff0))
					bad = true;
				if (!(val & 0x1ff00000))
					bad = true;
			} else {
				if (val & ~0xffc0ffc0)
					bad = true;
				if (!(val & 0xfff0))
					bad = true;
				if (!(val & 0xfff00000))
					bad = true;
			}
			if (extr(exp.debug[3], 20, 1) && bad) {
				nv04_pgraph_blowup(&exp, 0x2);
			}
			insrt(exp.ctx_valid, 8+idx0, 1, !extr(exp.nsource, 1, 1));
			insrt(exp.ctx_valid, 8+idx1, 1, !extr(exp.nsource, 1, 1));
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_nv3_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x01010003;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls;
		uint32_t mthd;
		int idx;
		switch (nrand48(ctx->rand48) % 4) {
			case 0:
			default:
				cls = 0x58;
				mthd = 0x300;
				idx = 0;
				break;
			case 1:
				cls = 0x59;
				mthd = 0x300;
				idx = 1;
				break;
			case 2:
				cls = 0x5a;
				mthd = 0x300;
				idx = 2;
				break;
			case 3:
				cls = 0x5b;
				mthd = 0x300;
				idx = 3;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 3);
		if (!extr(exp.intr, 4, 1)) {
			int fmt = 0;
			if (val == 1) {
				fmt = 7;
			} else if (val == 0x01010000) {
				fmt = 1;
			} else if (val == 0x01000000) {
				fmt = 2;
			} else if (val == 0x01010001) {
				fmt = 6;
			} else {
				if (extr(exp.debug[3], 20, 1)) {
					nv04_pgraph_blowup(&exp, 0x2);
				}
			}
			insrt(exp.surf_format, idx*4, 4, fmt);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_2d_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x1f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = get_random_surf2d(ctx);
		uint32_t mthd = 0x300;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 4);
		if (!extr(exp.intr, 4, 1)) {
			int fmt = 0;
			switch (val & 0xf) {
				case 0x01:
					fmt = 1;
					break;
				case 0x02:
					fmt = 2;
					break;
				case 0x03:
					fmt = 3;
					break;
				case 0x04:
					fmt = 5;
					break;
				case 0x05:
					fmt = 6;
					break;
				case 0x06:
					fmt = 7;
					break;
				case 0x07:
					fmt = 0xb;
					break;
				case 0x08:
					fmt = 0x9;
					break;
				case 0x09:
					fmt = 0xa;
					break;
				case 0x0a:
					fmt = 0xc;
					break;
				case 0x0b:
					fmt = 0xd;
					break;
			}
			if (extr(exp.debug[3], 20, 1) && (val == 0 || val > 0xd)) {
				nv04_pgraph_blowup(&exp, 0x2);
			}
			insrt(exp.surf_format, 0, 4, fmt);
			insrt(exp.surf_format, 4, 4, fmt);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_swz_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x0f0f001f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = get_random_swzsurf(ctx);
		uint32_t mthd = 0x300;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 3);
		if (!extr(exp.intr, 4, 1)) {
			int fmt = 0;
			switch (val & 0xf) {
				case 0x01:
					fmt = 1;
					break;
				case 0x02:
					fmt = 2;
					break;
				case 0x03:
					fmt = 3;
					break;
				case 0x04:
					fmt = 5;
					break;
				case 0x05:
					fmt = 6;
					break;
				case 0x06:
					fmt = 7;
					break;
				case 0x07:
					fmt = 0xb;
					break;
				case 0x08:
					fmt = 0x9;
					break;
				case 0x09:
					fmt = 0xa;
					break;
				case 0x0a:
					fmt = 0xc;
					break;
				case 0x0b:
					fmt = 0xd;
					break;
			}
			int sfmt = extr(val, 0, 16);
			int swzx = extr(val, 16, 4);
			int swzy = extr(val, 24, 4);
			int max_swz = 0xb;
			int min_swz = 1;
			bool bad = false;
			if (cls == 0x9e) {
				min_swz = 0;
				max_swz = 0xc;
				fmt = 0;
				if (sfmt == 3 || sfmt == 5 || sfmt == 0xa || sfmt == 0xd)
					bad = true;
			}
			if (sfmt == 0 || sfmt > 0xd)
				bad = true;
			if (val & 0xf0f00000)
				bad = true;
			if (swzx > max_swz || swzy > max_swz || swzx < min_swz || swzy < min_swz)
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad) {
				nv04_pgraph_blowup(&exp, 0x2);
			}
			insrt(exp.surf_format, 20, 4, fmt);
			insrt(exp.surf_swizzle[1], 16, 4, swzx);
			insrt(exp.surf_swizzle[1], 24, 4, swzy);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_dvd_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x000fe01f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = get_random_dvd(ctx);
		uint32_t mthd = 0x308;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 7);
		if (!extr(exp.intr, 4, 1)) {
			int fmt = 0;
			switch (val >> 16 & 0xf) {
				case 0x01:
					fmt = 0xe;
					break;
				case 0x02:
					fmt = 0xf;
					break;
			}
			int sfmt = extr(val, 16, 16);
			bool bad = false;
			if (sfmt == 0 || sfmt > 2)
				bad = true;
			if (val & (cls == 0x88 ? 0x1f : 0xe01f) || !(val & 0xffff))
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 0x2);
			exp.valid[0] |= 4;
			exp.surf_pitch[4] = val & pgraph_pitch_mask(&ctx->chipset);
			insrt(exp.ctx_valid, 12, 1, !extr(exp.nsource, 1, 1));
			insrt(exp.surf_format, 16, 4, fmt);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_surf_3d_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x0f0f033f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int trapbit;
		bool celsius;
		switch (nrand48(ctx->rand48) % 2) {
			case 0:
				cls = get_random_surf3d(ctx);
				mthd = 0x300;
				trapbit = 6;
				celsius = false;
				break;
			case 1:
				if (ctx->chipset.card_type < 0x10)
					continue;
				cls = get_random_celsius(ctx);
				mthd = 0x208;
				trapbit = 11;
				celsius = true;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			int fmt = 0;
			switch (val & 0xf) {
				case 1:
					fmt = 0x2;
					break;
				case 2:
					fmt = 0x3;
					break;
				case 3:
					fmt = 0x5;
					break;
				case 4:
					fmt = 0x7;
					break;
				case 5:
					fmt = 0xb;
					break;
				case 6:
					fmt = 0x9;
					break;
				case 7:
					fmt = 0xa;
					break;
				case 8:
					fmt = 0xc;
					break;
				case 9:
					if (celsius)
						fmt = 0x1;
					break;
				case 0xa:
					if (celsius)
						fmt = 0x6;
					break;
			}
			int sfmt = extr(val, 0, 8);
			int mode = extr(val, 8, 3);
			int swzx = extr(val, 16, 4);
			int swzy = extr(val, 24, 4);
			if (extr(exp.debug[3], 20, 1) && (sfmt == 0 || sfmt > (celsius ? 0xa : 8) || val & 0xf0f0fc00 || swzx >= 0xc || swzy >= 0xc || mode == 0 || mode == 3)) {
				nv04_pgraph_blowup(&exp, 0x2);
			}
			insrt(exp.surf_format, 8, 4, fmt);
			insrt(exp.surf_type, 0, 2, mode);
			insrt(exp.surf_swizzle[0], 16, 4, swzx);
			insrt(exp.surf_swizzle[0], 24, 4, swzy);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_dma_surf(struct hwtest_ctx *ctx) {
	int i;
	uint32_t offset_mask = pgraph_offset_mask(&ctx->chipset);
	for (i = 0; i < 10000; i++) {
		uint32_t cls;
		uint32_t mthd;
		int idx;
		bool isnew = false;
		bool isnv10 = false;
		int trapbit;
		switch (nrand48(ctx->rand48) % 12) {
			case 0:
			default:
				cls = 0x58;
				mthd = 0x184;
				idx = 0;
				trapbit = 2;
				break;
			case 1:
				cls = 0x59;
				mthd = 0x184;
				idx = 1;
				trapbit = 2;
				break;
			case 2:
				cls = 0x5a;
				mthd = 0x184;
				idx = 2;
				trapbit = 2;
				break;
			case 3:
				cls = 0x5b;
				mthd = 0x184;
				idx = 3;
				trapbit = 2;
				break;
			case 4:
				cls = get_random_surf2d(ctx);
				mthd = 0x184;
				idx = 1;
				isnew = true;
				isnv10 = cls != 0x42;
				trapbit = 2;
				break;
			case 5:
				cls = get_random_surf2d(ctx);
				mthd = 0x188;
				idx = 0;
				isnew = true;
				isnv10 = cls != 0x42;
				trapbit = 3;
				break;
			case 6:
				cls = get_random_swzsurf(ctx);
				mthd = 0x184;
				idx = 5;
				isnew = true;
				isnv10 = cls != 0x52;
				trapbit = 2;
				break;
			case 7:
				cls = get_random_dvd(ctx);
				mthd = 0x18c;
				idx = 4;
				isnew = true;
				isnv10 = cls != 0x38;
				trapbit = 4;
				break;
			case 8:
				cls = get_random_surf3d(ctx);
				mthd = 0x184;
				idx = 2;
				isnew = true;
				isnv10 = cls != 0x53;
				trapbit = 2;
				break;
			case 9:
				cls = get_random_surf3d(ctx);
				mthd = 0x188;
				idx = 3;
				isnew = true;
				isnv10 = cls != 0x53;
				trapbit = 3;
				break;
			case 10:
				if (ctx->chipset.card_type < 0x10)
					continue;
				cls = get_random_celsius(ctx);
				mthd = 0x194;
				idx = 2;
				isnew = true;
				isnv10 = true;
				trapbit = 8;
				break;
			case 11:
				if (ctx->chipset.card_type < 0x10)
					continue;
				cls = get_random_celsius(ctx);
				mthd = 0x198;
				idx = 3;
				isnew = true;
				isnv10 = true;
				trapbit = 9;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t dma[3];
		uint32_t val = nv04_pgraph_gen_dma(ctx, &orig, dma);
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t base = (dma[2] & ~0xfff) | extr(dma[0], 20, 12);
			uint32_t limit = dma[1];
			uint32_t dcls = extr(dma[0], 0, 12);
			exp.surf_limit[idx] = (limit & offset_mask) | 0xf | (dcls == 0x30) << 31;
			exp.surf_base[idx] = base & offset_mask;
			bool bad = true;
			if (dcls == 0x30 || dcls == 0x3d)
				bad = false;
			if (dcls == 3 && (cls == 0x38 || cls == 0x88))
				bad = false;
			if (dcls == 2 && (cls == 0x42 || cls == 0x62) && idx == 1)
				bad = false;
			if (extr(exp.debug[3], 23, 1) && bad) {
				nv04_pgraph_blowup(&exp, 0x2);
			}
			bool prot_err = false;
			if (extr(base, 0, isnv10 ? 6 : isnew ? 5 : 4))
				prot_err = true;
			if (extr(dma[0], 16, 2))
				prot_err = true;
			if (ctx->chipset.chipset >= 5 && ((bad && ctx->chipset.card_type < 0x10) || dcls == 0x30))
				prot_err = false;
			if (prot_err)
				nv04_pgraph_blowup(&exp, 4);
			insrt(exp.ctx_valid, idx, 1, dcls != 0x30 && !(bad && extr(exp.debug[3], 23, 1)));
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("DMA %08x %08x %08x\n", dma[0], dma[1], dma[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_clip(struct hwtest_ctx *ctx) {
	int i;
	int vidx = ctx->chipset.card_type < 0x10 ? 13 : 9;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t cls, mthd;
		int which;
		int trapbit = -1;
		switch (nrand48(ctx->rand48) % 12) {
			default:
				cls = 0x19;
				mthd = 0x300 + idx * 4;
				which = 0;
				trapbit = 2 + idx;
				break;
			case 1:
				cls = get_random_sifc(ctx);
				mthd = 0x310 + idx * 4;
				which = 2;
				trapbit = 13 + idx;
				break;
			case 2:
				cls = get_random_sifm(ctx);
				mthd = 0x308 + idx * 4;
				which = 1;
				trapbit = 10 + idx;
				break;
			case 3:
				cls = 0x4b;
				mthd = 0x7f4 + idx * 4;
				which = 3;
				trapbit = 16 + idx;
				break;
			case 4:
				cls = 0x4b;
				mthd = 0xbec + idx * 4;
				which = 3;
				trapbit = 16 + idx;
				break;
			case 5:
				cls = 0x4b;
				mthd = 0xfe8 + idx * 4;
				which = 3;
				trapbit = 16 + idx;
				break;
			case 6:
				cls = 0x4b;
				mthd = 0x13e4 + idx * 4;
				which = 3;
				trapbit = 16 + idx;
				break;
			case 7:
				cls = 0x4a;
				mthd = 0x5f4 + idx * 4;
				which = 3;
				trapbit = 16 + idx;
				break;
			case 8:
				cls = 0x4a;
				mthd = 0x7ec + idx * 4;
				which = 3;
				trapbit = 16 + idx;
				break;
			case 9:
				cls = 0x4a;
				mthd = 0xbe4 + idx * 4;
				which = 3;
				trapbit = 16 + idx;
				break;
			case 10:
				cls = 0x4a;
				mthd = 0xff4 + idx * 4;
				which = 3;
				trapbit = 16 + idx;
				break;
			case 11:
				cls = 0x4a;
				mthd = 0x17f4 + idx * 4;
				which = 3;
				trapbit = 16 + idx;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_xy[vidx][0], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_xy[vidx][1], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
		}
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			nv04_pgraph_set_clip(&exp, which, idx, val);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_clip_zero_size(struct hwtest_ctx *ctx) {
	int i;
	int vidx = ctx->chipset.card_type < 0x10 ? 13 : 9;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		cls = get_random_surf3d(ctx);
		mthd = 0x304;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_xy[vidx][0], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_xy[vidx][1], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
		}
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 7);
		if (!extr(exp.intr, 4, 1)) {
			exp.vtx_xy[vidx][0] = extr(val, 0, 16);
			exp.vtx_xy[vidx][1] = extr(val, 16, 16);
			int xcstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[vidx][0], 0);
			int ycstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[vidx][1], 1);
			pgraph_set_xy_d(&exp, 0, 1, 1, false, false, false, xcstat);
			pgraph_set_xy_d(&exp, 1, 1, 1, false, false, false, ycstat);
			exp.uclip_min[0][0] = 0;
			exp.uclip_min[0][1] = 0;
			exp.uclip_max[0][0] = exp.vtx_xy[vidx][0];
			exp.uclip_max[0][1] = exp.vtx_xy[vidx][1];
			insrt(exp.valid[0], 28, 1, 0);
			insrt(exp.valid[0], 30, 1, 0);
			insrt(exp.xy_misc_1[0], 4, 2, 0);
			insrt(exp.xy_misc_1[0], 12, 1, 0);
			insrt(exp.xy_misc_1[0], 16, 1, 0);
			insrt(exp.xy_misc_1[0], 20, 1, 0);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_clip_hv(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset < 5)
		return HWTEST_RES_NA;
	int i;
	int vidx = ctx->chipset.card_type < 0x10 ? 13 : 9;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int trapbit;
		int xy = jrand48(ctx->rand48) & 1;
		int which;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				cls = get_random_surf3d(ctx);
				mthd = 0x2f8 + xy * 4;
				trapbit = 4 + xy;
				which = 0;
				break;
			case 1:
				if (ctx->chipset.card_type < 0x10)
					continue;
				cls = 0x7b;
				mthd = 0x30c + xy * 4;
				trapbit = 8 + xy;
				which = 1;
				break;
			case 2:
				if (ctx->chipset.card_type < 0x10)
					continue;
				cls = get_random_celsius(ctx);
				mthd = 0x200 + xy * 4;
				trapbit = 10;
				which = 2;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_xy[vidx][0], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
			insrt(orig.vtx_xy[vidx][1], 16, 16, (jrand48(ctx->rand48) & 1) ? 0x8000 : 0x7fff);
		}
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (extr(exp.debug[3], 25, 1) || ctx->chipset.card_type >= 0x10) {
				if (which == 0) {
					insrt(exp.valid[0], 28, 1, 0);
					insrt(exp.valid[0], 30, 1, 0);
				} else if (which == 1) {
					insrt(exp.valid[0], 29, 1, 0);
					insrt(exp.valid[0], 31, 1, 0);
					insrt(exp.valid[0], 20, 1, 1);
				} else {
					insrt(exp.valid[1], 30, 1, 0);
					insrt(exp.valid[1], 31, 1, 0);
				}
				insrt(exp.valid[0], 19, 1, 0);
				if (which < 2) {
					insrt(exp.xy_misc_1[which], 4+xy, 1, 0);
					insrt(exp.xy_misc_1[which], 12, 1, 0);
					insrt(exp.xy_misc_1[which], 16, 1, 0);
					insrt(exp.xy_misc_1[which], 20, 1, 0);
				}
				insrt(exp.xy_misc_1[1], 0, 1, 1);
				insrt(exp.xy_misc_1[0], 0, 1, 0);
				insrt(exp.xy_misc_3, 8, 1, 0);
				uint32_t min = extr(val, 0, 16);
				uint32_t max;
				if (ctx->chipset.card_type < 0x10)
					max = min + extrs(val, 16, 16);
				else
					max = min + extr(val, 16, 16);
				exp.vtx_xy[vidx][xy] = min;
				int cstat = nv04_pgraph_clip_status(&exp, min, xy);
				pgraph_set_xy_d(&exp, xy, 0, 0, false, min != extrs(min, 0, 16), false, cstat);
				if (!xy) {
					exp.uclip_min[which][xy] = exp.uclip_max[which][xy] & 0xffff;
					exp.uclip_max[which][xy] = min & 0x3ffff;
					exp.vtx_xy[vidx][0] = exp.vtx_xy[vidx][1] = max;
					if (ctx->chipset.card_type < 0x10) {
						int xcstat = nv04_pgraph_clip_status(&exp, max, 0);
						int ycstat = nv04_pgraph_clip_status(&exp, max, 1);
						pgraph_set_xy_d(&exp, 0, 1, 1, false, false, false, xcstat);
						pgraph_set_xy_d(&exp, 1, 1, 1, false, false, false, ycstat);
					}
				}
				exp.uclip_min[which][xy] = min;
				exp.uclip_max[which][xy] = max & 0x3ffff;
				if (extr(exp.debug[3], 20, 1) && extr(val, 15, 1))
					nv04_pgraph_blowup(&exp, 2);
			} else {
				nv04_pgraph_blowup(&exp, 0x40);
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_solid_color(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int which = 0;
		int trapbit = -1;
		switch (nrand48(ctx->rand48) % 18) {
			default:
				cls = (jrand48(ctx->rand48) & 1 ? 0x5c : 0x1c);
				mthd = 0x304;
				trapbit = 10;
				break;
			case 1:
				cls = (jrand48(ctx->rand48) & 1 ? 0x5c : 0x1c);
				mthd = 0x600 | (jrand48(ctx->rand48) & 0x78);
				trapbit = 20;
				break;
			case 2:
				cls = (jrand48(ctx->rand48) & 1 ? 0x5d : 0x1d);
				mthd = 0x304;
				trapbit = 10;
				break;
			case 3:
				cls = (jrand48(ctx->rand48) & 1 ? 0x5d : 0x1d);
				mthd = 0x500 | (jrand48(ctx->rand48) & 0x70);
				trapbit = 23;
				break;
			case 4:
				cls = (jrand48(ctx->rand48) & 1 ? 0x5d : 0x1d);
				mthd = 0x580 | (jrand48(ctx->rand48) & 0x78);
				trapbit = 27;
				break;
			case 5:
				cls = (jrand48(ctx->rand48) & 1 ? 0x5e : 0x1e);
				mthd = 0x304;
				trapbit = 10;
				break;
			case 6:
				cls = (jrand48(ctx->rand48) & 1 ? 0x4a : 0x4b);
				mthd = 0x3fc;
				trapbit = 11;
				break;
			case 7:
				cls = 0x4b;
				mthd = 0x7fc;
				trapbit = 11;
				break;
			case 8:
				cls = 0x4a;
				mthd = 0x5fc;
				trapbit = 11;
				break;
			case 9:
				cls = 0x4b;
				mthd = 0xbf4;
				which = 1;
				trapbit = 12;
				break;
			case 10:
				cls = 0x4b;
				mthd = 0xff0;
				which = 1;
				trapbit = 12;
				break;
			case 11:
				cls = 0x4b;
				mthd = 0x13f0;
				which = 1;
				trapbit = 12;
				break;
			case 12:
				cls = 0x4a;
				mthd = 0x7f4;
				which = 1;
				trapbit = 12;
				break;
			case 13:
				cls = 0x4a;
				mthd = 0xbf0;
				which = 1;
				trapbit = 12;
				break;
			case 14:
				cls = 0x4a;
				mthd = 0xffc;
				which = 1;
				trapbit = 12;
				break;
			case 15:
				cls = 0x4a;
				mthd = 0x17fc;
				which = 1;
				trapbit = 12;
				break;
			case 16:
				cls = 0x4b;
				mthd = 0x13ec;
				which = 2;
				trapbit = 13;
				break;
			case 17:
				cls = 0x4a;
				mthd = 0xbec;
				which = 3;
				trapbit = 13;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (which == 0) {
				exp.misc32[0] = val;
				insrt(exp.valid[0], 16, 1, 1);
			} else if (which == 1) {
				exp.misc32[1] = val;
				insrt(exp.valid[0], 18, 1, 1);
			} else if (which == 2) {
				nv04_pgraph_set_bitmap_color_0_nv01(&exp, val);
				insrt(exp.valid[0], 17, 1, 1);
			} else if (which == 3) {
				exp.bitmap_color[0] = val;
				insrt(exp.valid[0], 17, 1, 1);
				bool likes_format = false;
				if (nv04_pgraph_is_nv17p(&ctx->chipset) || (ctx->chipset.chipset >= 5 && !nv04_pgraph_is_nv15p(&ctx->chipset)))
					likes_format = true;
				if (!likes_format)
					insrt(exp.ctx_format, 0, 8, extr(exp.ctx_switch[1], 8, 8));
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifm_pitch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = get_random_sifm(ctx), mthd = 0x404;
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xfefc000f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 17);
		uint32_t pitch = extr(val, 0, 16);
		exp.dma_pitch = pitch * 0x10001;
		if (!extr(exp.intr, 4, 1)) {
			insrt(exp.valid[0], 10, 1, 1);
			bool bad = false;
			if (cls != 0x37) {
				insrt(exp.sifm_mode, 16, 2, extr(val, 16, 2));
				insrt(exp.sifm_mode, 24, 1, extr(val, 24, 1));
				if (!extr(val, 16, 8))
					bad = true;
				if (extr(val, 16, 8) > 2)
					bad = true;
				if (extr(val, 24, 8) > 1)
					bad = true;
			} else {
				if (extr(val, 16, 16))
					bad = true;
			}
			if (cls == 0x37 || cls == 0x77 || cls == 0x63) {
				if (pitch & ~0x1fff)
					bad = true;
			}
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_dvd_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = get_random_dvd(ctx);
		int which = jrand48(ctx->rand48) & 1;
		uint32_t mthd = which ? 0x334 : 0x31c;
		int trapbit = which ? 18 : 12;
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xfffc000f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		uint32_t pitch = extr(val, 0, 16);
		insrt(exp.dma_pitch, 16 * which, 16, pitch);
		if (!extr(exp.intr, 4, 1)) {
			insrt(exp.valid[0], which ? 16 : 10, 1, 1);
			bool bad = false;
			int sfmt = extr(val, 16, 2);
			int fmt = 0;
			if (which == 0) {
				if (sfmt == 1)
					fmt = 0x12;
				if (sfmt == 2)
					fmt = 0x13;
				insrt(exp.dvd_format, 0, 6, fmt);
			} else {
				fmt = sfmt;
				insrt(exp.dvd_format, 8, 2, fmt);
			}
			if (!fmt || extr(val, 18, 14))
				bad = true;
			if (pitch & ~0x1fff && (cls == 0x38 || which == 1))
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_index_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0x00000001;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = get_random_iifc(ctx);
		uint32_t mthd = 0x3ec;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 12);
		if (!extr(exp.intr, 4, 1)) {
			insrt(exp.dvd_format, 0, 6, val & 1);
			if (extr(exp.debug[3], 20, 1) && val > 1)
				nv04_pgraph_blowup(&exp, 2);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_dma_offset(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int which;
		int vbit;
		int trapbit = -1;
		switch (nrand48(ctx->rand48) % 6) {
			default:
				cls = get_random_sifm(ctx);
				mthd = 0x408;
				which = 0;
				vbit = 11;
				trapbit = 18;
				break;
			case 1:
				cls = 0x39;
				mthd = 0x30c;
				which = 0;
				vbit = 0;
				trapbit = 4;
				break;
			case 2:
				cls = 0x39;
				mthd = 0x310;
				which = 1;
				vbit = 1;
				trapbit = 5;
				break;
			case 3:
				cls = get_random_dvd(ctx);
				mthd = 0x320;
				which = 0;
				vbit = 11;
				trapbit = 13;
				break;
			case 4:
				cls = get_random_dvd(ctx);
				mthd = 0x338;
				which = 1;
				vbit = 17;
				trapbit = 19;
				break;
			case 5:
				cls = get_random_iifc(ctx);
				mthd = 0x3f0;
				which = 0;
				vbit = 22;
				trapbit = 13;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			exp.dma_offset[which] = val;
			insrt(exp.valid[0], vbit, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_pitch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff0000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		int which = jrand48(ctx->rand48) & 1;;
		uint32_t cls = 0x39, mthd = 0x314 + which * 4;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 6 + which);
		insrt(exp.dma_pitch, which * 16, 16, val);
		if (!extr(exp.intr, 4, 1)) {
			insrt(exp.valid[0], 2+which, 1, 1);
			bool bad = false;
			if (val & ~0x7fff)
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_line_length(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xff000000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x39, mthd = 0x31c;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 8);
		if (!extr(exp.intr, 4, 1)) {
			insrt(exp.valid[0], 4, 1, 1);
			bool bad = false;
			if (val & ~0x3fffff)
				bad = true;
			exp.dma_length = val & 0x3fffff;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_line_count(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xfffff000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x39, mthd = 0x320;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 9);
		if (!extr(exp.intr, 4, 1)) {
			insrt(exp.valid[0], 5, 1, 1);
			bool bad = false;
			if (val & ~0x7ff)
				bad = true;
			insrt(exp.dma_misc, 0, 16, val & 0x7ff);
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_m2mf_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xfffff0f0;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x39, mthd = 0x324;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 10);
		if (!extr(exp.intr, 4, 1)) {
			insrt(exp.valid[0], 6, 1, 1);
			bool bad = false;
			int fmtx = extr(val, 0, 3);
			int fmty = extr(val, 8, 3);
			if (fmtx != 1 && fmtx != 2 && fmtx != 4)
				bad = true;
			if (fmty != 1 && fmty != 2 && fmty != 4)
				bad = true;
			if (val & 0xf8)
				bad = true;
			if (val & 0xfffff800 && ctx->chipset.chipset >= 5)
				bad = true;
			insrt(exp.dma_misc, 16, 3, extr(val, 0, 3));
			insrt(exp.dma_misc, 20, 3, extr(val, 8, 3));
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_font(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		int which = jrand48(ctx->rand48) & 1;
		uint32_t cls = 0x4a, mthd = which ? 0x17f0 : 0xff0;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 26);
		if (!extr(exp.intr, 4, 1)) {
			exp.dma_offset[0] = val;
			int pitch = extr(val, 28, 4);
			switch (pitch) {
				default:
					exp.dma_length = 1 << pitch;
					break;
				case 0:
					exp.dma_length = 0x18;
					break;
				case 1:
					exp.dma_length = 0x28;
					break;
				case 2:
					exp.dma_length = 0x48;
					break;
				case 0xb:
				case 0xd:
					exp.dma_length = 0x200;
					break;
				case 0xf:
					exp.dma_length = 0x280;
					break;
				case 0xc:
					exp.dma_length = 0x100;
					break;
				case 0xa:
				case 0xe:
					exp.dma_length = 0x140;
					break;
			}
			insrt(exp.valid[0], 22, 1, 1);
			if (extr(exp.debug[3], 20, 1) && (pitch < 3 || pitch > 9))
				nv04_pgraph_blowup(&exp, 2);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_xy(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int trapbit;
		bool first;
		bool check = false;
		bool noclip = false;
		bool ifc = false;
		bool tfc = false;
		switch (jrand48(ctx->rand48) % 21) {
			default:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5c : 0x1c;
				mthd = 0x400 | (jrand48(ctx->rand48) & 0x78);
				trapbit = 11;
				first = true;
				break;
			case 1:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5d : 0x1d;
				mthd = 0x310;
				trapbit = 11;
				first = true;
				break;
			case 2:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5d : 0x1d;
				mthd = 0x314;
				trapbit = 12;
				first = false;
				break;
			case 3:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5d : 0x1d;
				mthd = 0x504 | (jrand48(ctx->rand48) & 0x70);
				trapbit = 24;
				first = true;
				break;
			case 4:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5d : 0x1d;
				mthd = 0x508 | (jrand48(ctx->rand48) & 0x70);
				trapbit = 25;
				first = false;
				break;
			case 5:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5e : 0x1e;
				mthd = 0x400 | (jrand48(ctx->rand48) & 0x70);
				trapbit = 11;
				first = true;
				break;
			case 6:
				cls = get_random_blit(ctx);
				mthd = 0x300;
				trapbit = 11;
				first = true;
				check = true;
				break;
			case 7:
				cls = get_random_blit(ctx);
				mthd = 0x304;
				trapbit = 12;
				first = false;
				break;
			case 8:
				cls = 0x4b;
				mthd = 0x400 | (jrand48(ctx->rand48) & 0x1f8);
				trapbit = 14;
				first = true;
				noclip = true;
				break;
			case 9:
				cls = 0x4b;
				mthd = 0x800 | (jrand48(ctx->rand48) & 0x1f8);
				trapbit = 18;
				first = true;
				break;
			case 10:
				cls = 0x4b;
				mthd = 0xbfc;
				trapbit = 20;
				first = true;
				ifc = true;
				break;
			case 11:
				cls = 0x4b;
				mthd = 0xffc;
				trapbit = 20;
				first = true;
				ifc = true;
				break;
			case 12:
				cls = 0x4b;
				mthd = 0x13fc;
				trapbit = 20;
				first = true;
				ifc = true;
				break;
			case 13:
				cls = get_random_ifc(ctx);
				mthd = 0x304;
				trapbit = 11;
				first = true;
				ifc = true;
				break;
			case 14:
				cls = get_random_iifc(ctx);
				mthd = 0x3f4;
				trapbit = 14;
				first = true;
				ifc = true;
				break;
			case 15:
				if (ctx->chipset.card_type < 0x10)
					continue;
				cls = 0x7b;
				mthd = 0x304;
				trapbit = 6;
				first = true;
				ifc = true;
				tfc = true;
				break;
			case 16:
				cls = 0x4a;
				mthd = 0x400 | (jrand48(ctx->rand48) & 0xf8);
				trapbit = 14;
				first = true;
				noclip = true;
				break;
			case 17:
				cls = 0x4a;
				mthd = 0x600 | (jrand48(ctx->rand48) & 0xf8);
				trapbit = 18;
				first = true;
				break;
			case 18:
				cls = 0x4a;
				mthd = 0x7fc;
				trapbit = 20;
				first = true;
				ifc = true;
				break;
			case 19:
				cls = 0x4a;
				mthd = 0xbfc;
				trapbit = 20;
				first = true;
				ifc = true;
				break;
			case 20:
				cls = 0x4a;
				mthd = 0x1800 | (jrand48(ctx->rand48) & 0x7f8);
				trapbit = 20;
				first = true;
				ifc = true;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (extr(exp.debug[3], 20, 1) && check && val & 0x80008000)
				nv04_pgraph_blowup(&exp, 2);
			if (extr(exp.debug[3], 20, 1) && tfc && val & 7)
				nv04_pgraph_blowup(&exp, 2);
			if (first) {
				pgraph_clear_vtxid(&exp);
				if (cls == 0x1c || cls == 0x1d || cls == 0x5c || cls == 0x5d) {
					exp.valid[0] &= ~0xffff;
					insrt(exp.valid[0], 21, 1, 1);
				}
			}
			int vidx = pgraph_vtxid(&exp);
			int rvidx = ifc ? 4 : vidx;
			int svidx = vidx & 3;
			pgraph_bump_vtxid(&exp);
			insrt(exp.xy_misc_1[0], 0, 1, 0);
			insrt(exp.xy_misc_1[1], 0, 1, 1);
			insrt(exp.xy_misc_3, 8, 1, 0);
				insrt(exp.valid[0], rvidx, 1, 1);
				insrt(exp.valid[0], 8|rvidx, 1, 1);
			if (cls == 0x1c || cls == 0x1d || cls == 0x5c || cls == 0x5d) {
				insrt(exp.valid[0], 4|svidx, 1, 1);
				insrt(exp.valid[0], 0xc|svidx, 1, 1);
			}
			insrt(exp.valid[0], 19, 1, noclip);
			if (noclip) {
				exp.vtx_xy[rvidx][0] = extrs(val, 16, 16);
				exp.vtx_xy[rvidx][1] = extrs(val, 0, 16);
			} else {
				exp.vtx_xy[rvidx][0] = extrs(val, 0, 16);
				exp.vtx_xy[rvidx][1] = extrs(val, 16, 16);
			}
			int xcstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[rvidx][0], 0);
			int ycstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[rvidx][1], 1);
			pgraph_set_xy_d(&exp, 0, vidx, vidx, false, false, false, xcstat);
			pgraph_set_xy_d(&exp, 1, vidx, vidx, false, false, false, ycstat);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_x32(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int trapbit;
		bool first = false;
		bool poly = false;
		switch (jrand48(ctx->rand48) % 7) {
			default:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5c : 0x1c;
				mthd = 0x480 | (jrand48(ctx->rand48) & 0x70);
				trapbit = 13;
				first = true;
				break;
			case 1:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5c : 0x1c;
				mthd = 0x488 | (jrand48(ctx->rand48) & 0x70);
				trapbit = 15;
				break;
			case 2:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5c : 0x1c;
				mthd = 0x580 | (jrand48(ctx->rand48) & 0x78);
				trapbit = 18;
				poly = true;
				break;
			case 3:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5d : 0x1d;
				mthd = 0x320;
				trapbit = 14;
				first = true;
				break;
			case 4:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5d : 0x1d;
				mthd = 0x328;
				trapbit = 16;
				break;
			case 5:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5d : 0x1d;
				mthd = 0x330;
				trapbit = 18;
				break;
			case 6:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5d : 0x1d;
				mthd = 0x480 | (jrand48(ctx->rand48) & 0x78);
				trapbit = 21;
				poly = true;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (first) {
				pgraph_clear_vtxid(&exp);
				if (cls == 0x1c || cls == 0x1d || cls == 0x5c || cls == 0x5d) {
					exp.valid[0] &= ~0xffff;
					insrt(exp.valid[0], 21, 1, 1);
				}
			}
			int vidx = pgraph_vtxid(&exp);
			int svidx = vidx & 3;
			insrt(exp.xy_misc_1[0], 0, 1, 0);
			insrt(exp.xy_misc_1[1], 0, 1, 1);
			insrt(exp.xy_misc_3, 8, 1, 0);
			if (poly && (exp.valid[0] & 0xf0f))
				insrt(exp.valid[0], 21, 1, 0);
			if (!poly)
				insrt(exp.valid[0], vidx, 1, 1);
			if (cls == 0x1c || cls == 0x1d || cls == 0x5c || cls == 0x5d) {
				insrt(exp.valid[0], 4|svidx, 1, 1);
			}
			insrt(exp.valid[0], 19, 1, false);
			exp.vtx_xy[vidx][0] = val;
			int xcstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[vidx][0], 0);
			pgraph_set_xy_d(&exp, 0, vidx, vidx, 0, (int32_t)val != sext(val, 15), false, xcstat);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_y32(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int trapbit;
		switch (jrand48(ctx->rand48) % 3) {
			default:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5c : 0x1c;
				mthd = 0x484 | (jrand48(ctx->rand48) & 0x70);
				trapbit = 14;
				break;
			case 1:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5d : 0x1d;
				mthd = 0x324;
				trapbit = 15;
				break;
			case 2:
				cls = (jrand48(ctx->rand48) & 1) ? 0x5d : 0x1d;
				mthd = 0x32c;
				trapbit = 17;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			int vidx = pgraph_vtxid(&exp);
			int svidx = vidx & 3;
			pgraph_bump_vtxid(&exp);
			insrt(exp.xy_misc_1[0], 0, 1, 0);
			insrt(exp.xy_misc_1[1], 0, 1, 1);
			insrt(exp.xy_misc_3, 8, 1, 0);
			insrt(exp.valid[0], vidx|8, 1, 1);
			if (cls == 0x1c || cls == 0x1d || cls == 0x5c || cls == 0x5d) {
				insrt(exp.valid[0], 0xc|svidx, 1, 1);
			}
			insrt(exp.valid[0], 19, 1, false);
			exp.vtx_xy[vidx][1] = val;
			int ycstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[vidx][1], 1);
			pgraph_set_xy_d(&exp, 1, vidx, vidx, 0, (int32_t)val != sext(val, 15), false, ycstat);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ifc_size(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int trapbit;
		bool is_in = false, is_out = false;
		bool is_sifc = false, is_gdi = false, is_tfc = false;
		switch (jrand48(ctx->rand48) % 14) {
			default:
				cls = get_random_ifc(ctx);
				mthd = 0x308;
				is_out = true;
				trapbit = 12;
				break;
			case 1:
				cls = get_random_ifc(ctx);
				mthd = 0x30c;
				is_in = true;
				trapbit = 13;
				break;
			case 2:
				cls = get_random_sifc(ctx);
				mthd = 0x304;
				is_in = true;
				is_sifc = true;
				trapbit = 10;
				break;
			case 3:
				cls = get_random_iifc(ctx);
				mthd = 0x3f8;
				is_out = true;
				trapbit = 15;
				break;
			case 4:
				cls = get_random_iifc(ctx);
				mthd = 0x3fc;
				is_in = true;
				trapbit = 16;
				break;
			case 5:
				cls = 0x7b;
				if (ctx->chipset.card_type < 0x10)
					continue;
				mthd = 0x308;
				trapbit = 7;
				is_in = is_out = true;
				is_tfc = true;
				break;
			case 6:
				cls = 0x4b;
				mthd = 0xbf8;
				trapbit = 21;
				is_in = is_out = true;
				is_gdi = true;
				break;
			case 7:
				cls = 0x4b;
				mthd = 0xff4;
				trapbit = 22;
				is_in = true;
				is_gdi = true;
				break;
			case 8:
				cls = 0x4b;
				mthd = 0xff8;
				trapbit = 23;
				is_out = true;
				is_gdi = true;
				break;
			case 9:
				cls = 0x4b;
				mthd = 0x13f4;
				trapbit = 22;
				is_in = true;
				is_gdi = true;
				break;
			case 10:
				cls = 0x4b;
				mthd = 0x13f8;
				trapbit = 23;
				is_out = true;
				is_gdi = true;
				break;
			case 11:
				cls = 0x4a;
				mthd = 0x7f8;
				trapbit = 21;
				is_in = is_out = true;
				is_gdi = true;
				break;
			case 12:
				cls = 0x4a;
				mthd = 0xbf4;
				trapbit = 22;
				is_in = true;
				is_gdi = true;
				break;
			case 13:
				cls = 0x4a;
				mthd = 0xbf8;
				trapbit = 23;
				is_out = true;
				is_gdi = true;
				break;
		}
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xff00ff;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (extr(exp.debug[3], 20, 1) && is_tfc && val & 7)
				nv04_pgraph_blowup(&exp, 2);
			if (is_out) {
				exp.valid[0] |= 0x2020;
				exp.vtx_xy[5][0] = extr(val, 0, 16);
				exp.vtx_xy[5][1] = extr(val, 16, 16);
				pgraph_vtx_cmp(&exp, 0, 5, false);
				pgraph_vtx_cmp(&exp, 1, 5, false);
			}
			if (is_in) {
				exp.valid[0] |= 0x8008;
				exp.vtx_xy[3][0] = extr(val, 0, 16);
				exp.vtx_xy[3][1] = -extr(val, 16, 16);
				exp.vtx_xy[7][1] = extr(val, 16, 16);
				if (!is_out)
					exp.misc24[0] = extr(val, 0, 16);
				pgraph_vtx_cmp(&exp, 0, 3, false);
				pgraph_vtx_cmp(&exp, 1, 7, false);
				bool zero = false;
				if (!extr(exp.xy_misc_4[0], 28, 4))
					zero = true;
				if (!extr(exp.xy_misc_4[1], 28, 4))
					zero = true;
				pgraph_set_image_zero(&exp, zero);
				if (!is_sifc) {
					insrt(exp.xy_misc_3, 12, 1, extr(val, 0, 16) < 0x20 && is_gdi && !extr(exp.xy_a, 24, 1));
				}
			}
			if (cls != 0x8a || !extr(exp.debug[3], 16, 1))
				insrt(exp.xy_misc_1[0], 0, 1, 0);
			insrt(exp.xy_misc_1[1], 0, 1, !is_sifc);
			insrt(exp.xy_misc_3, 8, 1, 0);
			pgraph_clear_vtxid(&exp);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_color_key(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x54, mthd = 0x300;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.misc32[2] = val;
		insrt(exp.valid[1], 12, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_offset(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0x3ff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int which;
		switch (nrand48(ctx->rand48) % 4) {
			default:
				cls = 0x48;
				mthd = 0x304;
				which = 3;
				break;
			case 1:
				cls = 0x54;
				mthd = 0x304;
				which = 3;
				break;
			case 2:
				cls = 0x55;
				mthd = 0x308;
				which = 1;
				break;
			case 3:
				cls = 0x55;
				mthd = 0x30c;
				which = 2;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		if (which & 1) {
			exp.misc32[0] = val;
			insrt(exp.valid[1], 14, 1, 1);
		}
		if (which & 2) {
			exp.misc32[1] = val;
			insrt(exp.valid[1], 22, 1, 1);
		}
		if (extr(exp.debug[3], 20, 1) && (val & 0xff) && cls != 0x48)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_format_d3d0(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 22, 2, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 17, 3, 0);
		}
		uint32_t cls = 0x48, mthd = 0x308;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		exp.misc32[2] = val;
		int fmt = 5;
		int sfmt = extr(val, 20, 4);
		if (sfmt == 0)
			fmt = 2;
		else if (sfmt == 1)
			fmt = 3;
		else if (sfmt == 2)
			fmt = 4;
		else if (sfmt == 3)
			fmt = 5;
		else
			bad = true;
		int max_l = extr(val, 28, 4);
		int min_l = extr(val, 24, 4);
		if (min_l < 2)
			bad = true;
		if (min_l > max_l)
			bad = true;
		if (max_l > 0xb)
			bad = true;
		if (extr(val, 17, 3))
			bad = true;
		for (int i = 0; i < 2; i++) {
			insrt(exp.d3d56_tex_format[i], 1, 1, 0);
			insrt(exp.d3d56_tex_format[i], 2, 1, extr(val, 16, 1));
			insrt(exp.d3d56_tex_format[i], 7, 1, 0);
			insrt(exp.d3d56_tex_format[i], 8, 3, fmt);
			insrt(exp.d3d56_tex_format[i], 12, 4, max_l - min_l + 1);
			insrt(exp.d3d56_tex_format[i], 16, 4, max_l);
			insrt(exp.d3d56_tex_format[i], 20, 4, max_l);
		}
		insrt(exp.valid[1], 21, 1, 1);
		insrt(exp.valid[1], 15, 1, 1);
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 0, 2, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 2, 2, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 4, 2, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 6, 2, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 11, 1, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 24, 3, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 28, 3, 1);
		}
		uint32_t cls, mthd;
		int which;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				cls = 0x54;
				mthd = 0x308;
				which = 3;
				break;
			case 1:
				cls = 0x55;
				mthd = 0x310;
				which = 1;
				break;
			case 2:
				cls = 0x55;
				mthd = 0x314;
				which = 2;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (!extr(val, 0, 2) || extr(val, 0, 2) == 3)
			bad = true;
		if (!extr(val, 4, 2) || extr(val, 4, 2) == 3)
			bad = true;
		if (!extr(val, 6, 2) || extr(val, 6, 2) == 3)
			bad = true;
		if (!extr(val, 8, 3))
			bad = true;
		if (extr(val, 11, 1))
			bad = true;
		if (!extr(val, 12, 4))
			bad = true;
		if (extr(val, 16, 4) > 0xb)
			bad = true;
		if (extr(val, 20, 4) > 0xb)
			bad = true;
		if (extr(val, 24, 3) < 1 || extr(val, 24, 3) > 4)
			bad = true;
		if (extr(val, 28, 3) < 1 || extr(val, 28, 3) > 4)
			bad = true;
		uint32_t rval = val & 0xfffff7a6;
		if (cls == 0x54) {
			if (extr(val, 8, 3) == 1)
				insrt(rval, 8, 3, 0);
			if (extr(val, 2, 2) > 1)
				bad = true;
		} else {
			if (extr(val, 2, 2))
				bad = true;
		}
		if (which & 1) {
			insrt(exp.valid[1], 15, 1, 1);
			exp.d3d56_tex_format[0] = rval;
		}
		if (which & 2) {
			insrt(exp.valid[1], 21, 1, 1);
			exp.d3d56_tex_format[1] = rval;
		}
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_filter_d3d0(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x48, mthd = 0x30c;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		for (int i = 0; i < 2; i++) {
			insrt(exp.d3d56_tex_filter[i], 1, 4, extr(val, 1, 4));
			insrt(exp.d3d56_tex_filter[i], 9, 4, extr(val, 9, 4));
			insrt(exp.d3d56_tex_filter[i], 15, 1, 0);
			insrt(exp.d3d56_tex_filter[i], 16, 8, extrs(val, 17, 7));
			insrt(exp.d3d56_tex_filter[i], 27, 1, 1);
			insrt(exp.d3d56_tex_filter[i], 31, 1, 1);
		}
		insrt(exp.valid[1], 23, 1, 1);
		insrt(exp.valid[1], 16, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tex_filter(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 5, 3, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 13, 2, 0);
		}
		uint32_t cls, mthd;
		int which;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				cls = 0x54;
				mthd = 0x30c;
				which = 3;
				break;
			case 1:
				cls = 0x55;
				mthd = 0x318;
				which = 1;
				break;
			case 2:
				cls = 0x55;
				mthd = 0x31c;
				which = 2;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (extr(val, 13, 2))
			bad = true;
		if (extr(val, 5, 3))
			bad = true;
		if (extr(val, 24, 3) == 0 || extr(val, 24, 3) > 6)
			bad = true;
		if (extr(val, 28, 3) == 0 || extr(val, 28, 3) > 6)
			bad = true;
		bool is_mip = extr(val, 24, 3) == 5 || extr(val, 24, 3) == 6;
		uint32_t rval = val & 0xffff9e1e;
		if (which & 1) {
			insrt(exp.valid[1], 16, 1, 1);
			exp.d3d56_tex_filter[0] = rval;
		}
		if (cls == 0x54 && is_mip && !extr(val, 15, 1)) {
			int bias = extr(val, 16, 8);
			if (bias < 0x78 || bias >= 0x80)
				bias += 8;
			insrt(rval, 16, 8, bias);
		}
		if (which & 2) {
			insrt(exp.valid[1], 23, 1, 1);
			exp.d3d56_tex_filter[1] = rval;
		}
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_blend(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 9, 3, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 13, 3, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 17, 3, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 21, 3, 0);
			if (jrand48(ctx->rand48) & 1)
				insrt(val, 0, 4, 0);
		}
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				cls = 0x54;
				mthd = 0x310;
				break;
			case 1:
				cls = 0x55;
				mthd = 0x338;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (cls == 0x54) {
			if (extr(val, 0, 4) < 1 || extr(val, 0, 4) > 8)
				bad = true;
		} else {
			if (extr(val, 0, 4))
				bad = true;
		}
		if (extr(val, 4, 2) < 1 || extr(val, 4, 2) > 2)
			bad = true;
		if (!extr(val, 6, 2))
			bad = true;
		if (extr(val, 9, 3))
			bad = true;
		if (extr(val, 13, 3))
			bad = true;
		if (extr(val, 17, 3))
			bad = true;
		if (extr(val, 21, 3))
			bad = true;
		if (extr(val, 24, 4) < 1 || extr(val, 24, 4) > 0xb)
			bad = true;
		if (extr(val, 28, 4) < 1 || extr(val, 28, 4) > 0xb)
			bad = true;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		uint32_t rval = val & 0xff1111ff;
		if (cls == 0x55)
			insrt(rval, 0, 4, 0);
		exp.d3d56_blend = rval;
		insrt(exp.valid[1], 20, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_config(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 8, 4, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 15, 1, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 16, 4, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 25, 5, 0);
		}
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				cls = 0x54;
				mthd = 0x314;
				break;
			case 1:
				cls = 0x55;
				mthd = 0x33c;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (extr(val, 8, 4) < 1 || extr(val, 8, 4) > 8)
			bad = true;
		if (extr(val, 15, 1))
			bad = true;
		if (extr(val, 16, 4) < 1 || extr(val, 16, 4) > 8)
			bad = true;
		if (extr(val, 20, 2) < 1)
			bad = true;
		if (extr(val, 25, 5) && cls == 0x54)
			bad = true;
		if (extr(val, 30, 2) < 1 || extr(val, 30, 2) > 2)
			bad = true;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		uint32_t rval = val & 0xffff5fff;
		if (cls == 0x54)
			insrt(rval, 25, 5, 0x1e);
		exp.d3d56_config = rval;
		insrt(exp.dvd_format, 0, 8, extr(val, 13, 1));
		insrt(exp.valid[1], 17, 1, 1);
		if (cls == 0x54) {
			exp.d3d56_stencil_func = 0x80;
			exp.d3d56_stencil_op = 0x222;
			insrt(exp.valid[1], 18, 1, 1);
			insrt(exp.valid[1], 19, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_config_d3d0(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 0, 4, 1);
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x48, mthd = 0x314;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (extr(val, 9, 1))
			bad = true;
		if (!extr(val, 12, 2))
			bad = true;
		if (extr(val, 14, 1))
			bad = true;
		if (extr(val, 16, 4) < 1 || extr(val, 16, 4) > 8)
			bad = true;
		if (extr(val, 20, 4) > 4)
			bad = true;
		if (extr(val, 24, 4) > 4)
			bad = true;
		exp.misc32[3] = val;
		int filt = 1;
		int origin = 0;
		int sinterp = extr(val, 0, 4);
		if (sinterp == 0)
			origin = 2;
		if (sinterp == 2)
			filt = 2;
		if (sinterp > 2)
			bad = true;
		int wrapu = extr(val, 4, 2);
		if (wrapu == 0)
			wrapu = 9;
		int wrapv = extr(val, 6, 2);
		if (wrapv == 0)
			wrapv = 9;
		int scull = extr(val, 12, 2);
		int cull = 0;
		if (scull == 0) {
			cull = 1;
		} else if (scull == 1) {
			cull = 1;
		} else if (scull == 2) {
			cull = 3;
		} else if (scull == 3) {
			cull = 2;
		}
		int dblend = 6;
		int sblend = 5;
		if (extr(val, 29, 1)) {
			dblend = 0xa;
			sblend = 0x9;
		}
		if (extr(val, 28, 1))
			dblend = sblend = 2;
		if (extr(val, 30, 1))
			dblend = 1;
		if (extr(val, 31, 1))
			sblend = 1;
		for (int i = 0; i < 2; i++) {
			insrt(exp.d3d56_tex_format[i], 4, 2, origin);
			insrt(exp.d3d56_tex_format[i], 24, 4, wrapu);
			insrt(exp.d3d56_tex_format[i], 28, 4, wrapv);
			insrt(exp.d3d56_tex_filter[i], 24, 3, filt);
			insrt(exp.d3d56_tex_filter[i], 28, 3, filt);
		}
		insrt(exp.d3d56_blend, 4, 2, 1);
		insrt(exp.d3d56_blend, 6, 2, 2);
		insrt(exp.d3d56_blend, 8, 1, 1);
		insrt(exp.d3d56_blend, 12, 1, 0);
		insrt(exp.d3d56_blend, 16, 1, 1);
		insrt(exp.d3d56_blend, 20, 1, 1);
		insrt(exp.d3d56_blend, 24, 4, sblend);
		insrt(exp.d3d56_blend, 28, 4, dblend);
		insrt(exp.d3d56_config, 14, 1, !!extr(val, 20, 3));
		insrt(exp.d3d56_config, 16, 4, extr(val, 16, 4));
		insrt(exp.d3d56_config, 20, 2, cull);
		insrt(exp.d3d56_config, 22, 1, 1);
		insrt(exp.d3d56_config, 23, 1, extr(val, 15, 1));
		insrt(exp.d3d56_config, 24, 1, !!extr(val, 20, 3));
		insrt(exp.d3d56_config, 25, 1, 0);
		insrt(exp.d3d56_config, 26, 1, 0);
		insrt(exp.d3d56_config, 27, 3, extr(val, 24, 3) ? 0x7 : 0);
		insrt(exp.d3d56_config, 30, 2, 1);
		exp.d3d56_stencil_func = 0x80;
		exp.d3d56_stencil_op = 0x222;
		insrt(exp.valid[1], 17, 1, 1);
		insrt(exp.valid[1], 24, 1, 1);
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_alpha_d3d0(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 8, 4, 1);
			val &= ~0xfffff000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x48, mthd = 0x318;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (extr(val, 8, 4) < 1 || extr(val, 8, 4) > 8)
			bad = true;
		if (extr(val, 12, 20))
			bad = true;
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		insrt(exp.d3d56_config, 0, 12, val);
		insrt(exp.d3d56_config, 12, 1, 1);
		insrt(exp.valid[1], 18, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_stencil_func(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x55, mthd = 0x340;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (extr(val, 1, 3))
			bad = true;
		if (extr(val, 4, 4) < 1 || extr(val, 4, 4) > 8)
			bad = true;
		exp.d3d56_stencil_func = val & 0xfffffff1;
		insrt(exp.valid[1], 18, 1, 1);
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_stencil_op(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xfffff000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x55, mthd = 0x344;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (extr(val, 0, 4) < 1 || extr(val, 0, 4) > 8)
			bad = true;
		if (extr(val, 4, 4) < 1 || extr(val, 4, 4) > 8)
			bad = true;
		if (extr(val, 8, 4) < 1 || extr(val, 8, 4) > 8)
			bad = true;
		if (extr(val, 12, 20))
			bad = true;
		exp.d3d56_stencil_op = val & 0xfff;
		insrt(exp.valid[1], 19, 1, 1);
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_fog_color(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				cls = 0x48;
				mthd = 0x310;
				break;
			case 1:
				cls = 0x54;
				mthd = 0x318;
				break;
			case 2:
				cls = 0x55;
				mthd = 0x348;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.misc24[0] = val & 0xffffff;
		insrt(exp.valid[1], 13, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_rc_cw(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0x00e0e0e0;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		int idx = jrand48(ctx->rand48) & 1;
		int ac = jrand48(ctx->rand48) & 1;
		uint32_t cls = 0x55, mthd = 0x320 + idx * 0xc + ac * 4;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		bool bad = false;
		if (val & 0x00e0e0e0)
			bad = true;
		if (!extr(val, 2, 3))
			bad = true;
		if (!extr(val, 10, 3))
			bad = true;
		if (!extr(val, 18, 3))
			bad = true;
		if (!extr(val, 26, 3))
			bad = true;
		if (!extr(val, 29, 3))
			bad = true;
		if (idx) {
			if (extr(val, 2, 3) == 7)
				bad = true;
			if (extr(val, 10, 3) == 7)
				bad = true;
			if (extr(val, 18, 3) == 7)
				bad = true;
			if (extr(val, 26, 3) == 7)
				bad = true;
		}
		if (ac)
			exp.d3d56_rc_color[idx] = val & 0xff1f1f1f;
		else
			exp.d3d56_rc_alpha[idx] = val & 0xfd1d1d1d;
		insrt(exp.valid[1], 28 - idx * 2 - ac, 1, 1);
		if (extr(exp.debug[3], 20, 1) && bad)
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_rc_factor(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x55, mthd = 0x334;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.misc32[3] = val;
		insrt(exp.valid[1], 24, 1, 1);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tlv_fog_tri_col1(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		int idx;
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				idx = jrand48(ctx->rand48) & 0x7f;
				cls = 0x48;
				mthd = 0x1000 + idx * 0x20;
				break;
			case 1:
				idx = jrand48(ctx->rand48) & 0xf;
				cls = 0x54;
				mthd = 0x414 + idx * 0x20;
				break;
			case 2:
				idx = jrand48(ctx->rand48) & 0x7;
				cls = 0x55;
				mthd = 0x414 + idx * 0x28;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		if (cls == 0x48) {
			exp.d3d56_tlv_fog_tri_col1 = val ^ 0xff000000;
			insrt(exp.valid[1], 0, 7, 1);
		} else {
			exp.d3d56_tlv_fog_tri_col1 = val;
			insrt(exp.valid[1], 0, 1, 1);
			insrt(exp.valid[0], idx, 1, 0);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tlv_color(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		int idx;
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				idx = jrand48(ctx->rand48) & 0x7f;
				cls = 0x48;
				mthd = 0x1004 + idx * 0x20;
				break;
			case 1:
				idx = jrand48(ctx->rand48) & 0xf;
				cls = 0x54;
				mthd = 0x410 + idx * 0x20;
				break;
			case 2:
				idx = jrand48(ctx->rand48) & 0x7;
				cls = 0x55;
				mthd = 0x410 + idx * 0x28;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.d3d56_tlv_color = val;
		insrt(exp.valid[1], 1, 1, 1);
		int vidx = (cls == 0x48 ? extr(exp.d3d56_tlv_fog_tri_col1, 0, 4) : idx);
		insrt(exp.valid[0], vidx, 1, 0);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tlv_xy(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		int idx;
		uint32_t cls, mthd;
		int xy = nrand48(ctx->rand48) % 2;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				idx = jrand48(ctx->rand48) & 0x7f;
				cls = 0x48;
				mthd = 0x1008 + idx * 0x20 + xy * 4;
				break;
			case 1:
				idx = jrand48(ctx->rand48) & 0xf;
				cls = 0x54;
				mthd = 0x400 + idx * 0x20 + xy * 4;
				break;
			case 2:
				idx = jrand48(ctx->rand48) & 0x7;
				cls = 0x55;
				mthd = 0x400 + idx * 0x28 + xy * 4;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		int e = extr(val, 23, 8);
		bool s = extr(val, 31, 1);
		uint32_t tv = extr(val, 0, 23) | 1 << 23;
		if (e > 0x7f+10) {
			tv = 0x7fff + s;
		} else if (e < 0x7f-4) {
			tv = 0;
		} else {
			tv >>= 0x7f + 10 - e + 24 - 15;
			if (s)
				tv = -tv;
		}
		tv &= 0xffff;
		if (cls != 0x48 && extr(exp.debug[0], 14, 1) && exp.dvd_format & 1) {
			if (tv >= 0x8000 && tv < 0x8008)
				tv = 0x8000;
			else
				tv -= 8;
		}
		insrt(exp.d3d56_tlv_xy, 16*xy, 16, tv);
		insrt(exp.valid[1], 5 - xy, 1, 1);
		int vidx = (cls == 0x48 ? extr(exp.d3d56_tlv_fog_tri_col1, 0, 4) : idx);
		insrt(exp.valid[0], vidx, 1, 0);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tlv_z(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		int idx;
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				idx = jrand48(ctx->rand48) & 0x7f;
				cls = 0x48;
				mthd = 0x1010 + idx * 0x20;
				break;
			case 1:
				idx = jrand48(ctx->rand48) & 0xf;
				cls = 0x54;
				mthd = 0x408 + idx * 0x20;
				break;
			case 2:
				idx = jrand48(ctx->rand48) & 0x7;
				cls = 0x55;
				mthd = 0x408 + idx * 0x28;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.d3d56_tlv_z = val;
		insrt(exp.valid[1], 3, 1, 1);
		int vidx = (cls == 0x48 ? extr(exp.d3d56_tlv_fog_tri_col1, 0, 4) : idx);
		insrt(exp.valid[0], vidx, 1, 0);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tlv_rhw(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		int idx;
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				idx = jrand48(ctx->rand48) & 0x7f;
				cls = 0x48;
				mthd = 0x1014 + idx * 0x20;
				break;
			case 1:
				idx = jrand48(ctx->rand48) & 0xf;
				cls = 0x54;
				mthd = 0x40c + idx * 0x20;
				break;
			case 2:
				idx = jrand48(ctx->rand48) & 0x7;
				cls = 0x55;
				mthd = 0x40c + idx * 0x28;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.d3d56_tlv_rhw = val & ~0x3f;
		if (extr(exp.debug[2], 9, 1) && ctx->chipset.chipset >= 5) {
			if (extr(exp.d3d56_tlv_rhw, 0, 31) < 0xa800000)
				insrt(exp.d3d56_tlv_rhw, 0, 31, 0xa800000);
		}
		insrt(exp.valid[1], 2, 1, 1);
		int vidx = (cls == 0x48 ? extr(exp.d3d56_tlv_fog_tri_col1, 0, 4) : idx);
		insrt(exp.valid[0], vidx, 1, 0);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_d3d_tlv_uv(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		int idx;
		uint32_t cls, mthd;
		int which;
		bool fin = false;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				which = 0;
				idx = jrand48(ctx->rand48) & 0x7f;
				cls = 0x48;
				mthd = 0x1018 + idx * 0x20;
				break;
			case 1:
				which = jrand48(ctx->rand48) & 1;
				idx = jrand48(ctx->rand48) & 0xf;
				cls = 0x54;
				mthd = 0x418 + idx * 0x20 + which * 4;
				fin = which == 1;
				break;
			case 2:
				which = jrand48(ctx->rand48) & 3;
				idx = jrand48(ctx->rand48) & 0x7;
				cls = 0x55;
				mthd = 0x418 + idx * 0x28 + which * 4;
				fin = which == 3;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1) {
			orig.valid[1] |= 0xf;
			if (jrand48(ctx->rand48) & 1) {
				orig.valid[1] ^= 1 << (jrand48(ctx->rand48) & 7);
			}
		}
		if (fin)
			orig.notify &= ~1;
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		exp.d3d56_tlv_uv[which >> 1][which & 1] = val & ~0x3f;
		int vidx = (cls == 0x48 ? extr(exp.d3d56_tlv_fog_tri_col1, 0, 4) : idx);
		if (which < 3)
			insrt(exp.valid[1], 6 + which, 1, 1);
		if (fin) {
			exp.vtx_xy[vidx][0] = extr(exp.d3d56_tlv_xy, 0, 16) | extr(exp.d3d56_tlv_color, 16, 16) << 16;
			exp.vtx_xy[vidx][1] = extr(exp.d3d56_tlv_xy, 16, 16) | extr(exp.d3d56_tlv_color, 0, 16) << 16;
			int xcstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[vidx][0], 0);
			int ycstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[vidx][1], 1);
			pgraph_set_xy_d(&exp, 0, vidx, vidx, false, false, false, xcstat);
			pgraph_set_xy_d(&exp, 1, vidx, vidx, false, false, false, ycstat);
			exp.vtx_u[vidx] = exp.d3d56_tlv_uv[0][0];
			exp.vtx_v[vidx] = exp.d3d56_tlv_uv[0][1];
			exp.vtx_m[vidx] = exp.d3d56_tlv_rhw;
			uint32_t msk = (cls == 0x55 ? 0x1ff : 0x7f);
			int ovidx = extr(exp.valid[0], 16, 4);
			if ((exp.valid[1] & msk) == msk) {
				insrt(exp.valid[0], vidx, 1, 1);
				insrt(exp.valid[0], 16, 4, vidx);
				exp.valid[1] &= ~msk;
			} else if (!extr(exp.valid[0], ovidx, 1)) {
				nv04_pgraph_blowup(&exp, 0x4000);
			}
			// sounds buggy...
			if (extr(exp.debug[3], 5, 1))
				vidx = extr(exp.valid[0], 16, 4);
			if (cls == 0x55) {
				vidx &= 7;
				exp.vtx_u[vidx + 8] = exp.d3d56_tlv_uv[1][0];
				exp.vtx_v[vidx + 8] = exp.d3d56_tlv_uv[1][1];
				exp.vtx_xy[vidx + 8][0] = exp.d3d56_tlv_fog_tri_col1;
				exp.vtx_xy[vidx + 8][1] = exp.d3d56_tlv_z;
			} else {
				exp.vtx_xy[vidx + 16][0] = exp.d3d56_tlv_fog_tri_col1;
				exp.vtx_xy[vidx + 16][1] = exp.d3d56_tlv_z;
			}
			if (!extr(exp.surf_format, 8, 4) && extr(exp.debug[3], 22, 1))
				nv04_pgraph_blowup(&exp, 0x0200);
		} else {
			insrt(exp.valid[0], vidx, 1, 0);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 4) {
			default:
				cls = 0x17;
				mthd = 0x300;
				break;
			case 1:
				cls = 0x18;
				mthd = 0x300;
				break;
			case 2:
				cls = 0x57;
				mthd = 0x300;
				break;
			case 3:
				cls = 0x44;
				mthd = 0x300;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj, 2);
		if (!extr(exp.intr, 4, 1)) {
			if (extr(exp.debug[3], 20, 1) && (val > 3 || val == 0))
				nv04_pgraph_blowup(&exp, 2);
			int sfmt = val & 3;
			int fmt = 0;
			if (sfmt == 1) {
				if (cls == 0x44 || cls == 0x57)
					fmt = 0xb;
				else
					fmt = 0x2;
			}
			if (sfmt == 2)
				fmt = 0x8;
			if (sfmt == 3)
				fmt = 0xd;
			if (!extr(exp.nsource, 1, 1)) {
				insrt(egrobj[1], 8, 8, fmt);
				int subc = extr(exp.ctx_user, 13, 3);
				exp.ctx_cache[subc][1] = exp.ctx_switch[1];
				insrt(exp.ctx_cache[subc][1], 8, 8, fmt);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[1] = exp.ctx_cache[subc][1];
			}
			if (cls == 0x57) {
				insrt(exp.ctx_format, 24, 8, extr(exp.ctx_switch[1], 8, 8));
				insrt(exp.ctx_valid, 17, 1, !extr(exp.nsource, 1, 1));
			}
			if (cls == 0x44) {
				insrt(exp.ctx_format, 8, 8, extr(exp.ctx_switch[1], 8, 8));
				insrt(exp.ctx_format, 16, 8, extr(exp.ctx_switch[1], 8, 8));
				insrt(exp.ctx_valid, 20, 1, !extr(exp.nsource, 1, 1));
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_solid_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		bool isnew;
		int trapbit;
		switch (nrand48(ctx->rand48) % 8) {
			default:
				cls = 0x1c;
				mthd = 0x300;
				isnew = false;
				trapbit = 9;
				break;
			case 1:
				cls = 0x1d;
				mthd = 0x300;
				isnew = false;
				trapbit = 9;
				break;
			case 2:
				cls = 0x1e;
				mthd = 0x300;
				isnew = false;
				trapbit = 9;
				break;
			case 3:
				cls = 0x4b;
				mthd = 0x300;
				isnew = false;
				trapbit = 9;
				break;
			case 4:
				cls = 0x5c;
				mthd = 0x300;
				isnew = true;
				trapbit = 9;
				break;
			case 5:
				cls = 0x5d;
				mthd = 0x300;
				isnew = true;
				trapbit = 9;
				break;
			case 6:
				cls = 0x5e;
				mthd = 0x300;
				isnew = true;
				trapbit = 9;
				break;
			case 7:
				cls = 0x4a;
				mthd = 0x300;
				isnew = true;
				trapbit = 9;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = (val > (isnew || cls == 0x4b ? 3 : 4) || val == 0);
			int sfmt = val & 0xf;
			int fmt = 0;
			if (sfmt == 1)
				fmt = isnew ? 0xc : 0x3;
			if (sfmt == 2)
				fmt = 0x9;
			if (sfmt == 3)
				fmt = 0xe;
			if (sfmt == 4 && !isnew)
				fmt = 0x11;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1)) {
				insrt(egrobj[1], 8, 8, fmt);
				int subc = extr(exp.ctx_user, 13, 3);
				exp.ctx_cache[subc][1] = exp.ctx_switch[1];
				insrt(exp.ctx_cache[subc][1], 8, 8, fmt);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[1] = exp.ctx_cache[subc][1];
			}
			bool likes_format = false;
			if (nv04_pgraph_is_nv17p(&ctx->chipset) || (ctx->chipset.chipset >= 5 && !nv04_pgraph_is_nv15p(&ctx->chipset)))
				likes_format = true;
			if (cls == 0x4a && likes_format) {
				insrt(exp.ctx_format, 0, 8, extr(exp.ctx_switch[1], 8, 8));
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ifc_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		bool isnew;
		int trapbit;
		switch (nrand48(ctx->rand48) % 4) {
			default:
				cls = get_random_ifc(ctx);
				mthd = 0x300;
				isnew = cls != 0x21;
				trapbit = 10;
				break;
			case 1:
				cls = get_random_sifc(ctx);
				mthd = 0x300;
				isnew = cls != 0x36;
				trapbit = 9;
				break;
			case 2:
				cls = get_random_iifc(ctx);
				mthd = 0x3e8;
				isnew = true;
				trapbit = 11;
				break;
			case 3:
				if (ctx->chipset.card_type < 0x10)
					continue;
				cls = 0x7b;
				mthd = 0x300;
				isnew = true;
				trapbit = 5;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = (val > 5 || val == 0);
			int sfmt = val & 7;
			int fmt = 0;
			if (sfmt == 1)
				fmt = isnew ? 0xa : 0x1;
			if (sfmt == 2)
				fmt = 0x6;
			if (sfmt == 3)
				fmt = 0x7;
			if (sfmt == 4)
				fmt = 0xd;
			if (sfmt == 5)
				fmt = 0xe;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1)) {
				insrt(egrobj[1], 8, 8, fmt);
				int subc = extr(exp.ctx_user, 13, 3);
				exp.ctx_cache[subc][1] = exp.ctx_switch[1];
				insrt(exp.ctx_cache[subc][1], 8, 8, fmt);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[1] = exp.ctx_cache[subc][1];
			}
			if (cls == 0x4a) {
				insrt(exp.ctx_format, 0, 8, extr(exp.ctx_switch[1], 8, 8));
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_sifm_src_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls = get_random_sifm(ctx), mthd = 0x300;
		bool isnew = cls != 0x37;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj, 8);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t max = isnew ? 7 : 6;
			if (cls == 0x89)
				max = 9;
			bool bad = (val > max || val == 0);
			int sfmt = val & 7;
			if (ctx->chipset.card_type >= 0x10)
				sfmt = val & 0xf;
			int fmt = 0;
			if (sfmt == 1)
				fmt = 0x6;
			if (sfmt == 2)
				fmt = 0x7;
			if (sfmt == 3)
				fmt = 0xd;
			if (sfmt == 4)
				fmt = 0xe;
			if (sfmt == 5)
				fmt = 0x12;
			if (sfmt == 6)
				fmt = 0x13;
			if (sfmt == 7)
				fmt = 0xa;
			if (sfmt == 8)
				fmt = 0x1;
			if (sfmt == 9)
				fmt = 0x15;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1)) {
				insrt(egrobj[1], 8, 8, fmt);
				int subc = extr(exp.ctx_user, 13, 3);
				exp.ctx_cache[subc][1] = exp.ctx_switch[1];
				insrt(exp.ctx_cache[subc][1], 8, 8, fmt);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[1] = exp.ctx_cache[subc][1];
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_mono_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		bool is_ctx = false;
		int trapbit;
		switch (nrand48(ctx->rand48) % 4) {
			default:
				cls = 0x18;
				mthd = 0x304;
				trapbit = 3;
				break;
			case 1:
				is_ctx = true;
				cls = 0x44;
				mthd = 0x304;
				trapbit = 3;
				break;
			case 2:
				cls = 0x4b;
				mthd = 0x304;
				trapbit = 10;
				break;
			case 3:
				cls = 0x4a;
				mthd = 0x304;
				trapbit = 10;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (extr(exp.debug[3], 20, 1) && (val > 2 || val == 0))
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1)) {
				int fmt = val & 3;
				insrt(egrobj[1], 0, 8, fmt);
				int subc = extr(exp.ctx_user, 13, 3);
				exp.ctx_cache[subc][1] = exp.ctx_switch[1];
				insrt(exp.ctx_cache[subc][1], 0, 2, fmt);
				if (is_ctx)
					insrt(exp.ctx_valid, 21, 1, 1);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[1] = exp.ctx_cache[subc][1];
			} else {
				if (is_ctx)
					insrt(exp.ctx_valid, 21, 1, 0);
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_dma_grobj(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t cls, mthd;
		int which, mode;
		bool is3d = false;
		bool clr = false;
		int trapbit = -1;
		switch (nrand48(ctx->rand48) % 10) {
			default:
				cls = get_random_class(ctx);
				mthd = 0x180;
				which = 2;
				mode = 3;
				trapbit = 1;
				if (cls == 0x56 || cls == 0x96)
					trapbit = 3;
				if (cls == 0x7b)
					trapbit = 2;
				break;
			case 1:
				cls = get_random_sifm(ctx);
				mthd = 0x184;
				which = 0;
				mode = 2;
				trapbit = 2;
				break;
			case 2:
				cls = 0x4a;
				mthd = 0x184;
				which = 0;
				mode = 2;
				trapbit = 2;
				break;
			case 3:
				cls = get_random_iifc(ctx);
				mthd = 0x184;
				which = 0;
				mode = 2;
				trapbit = 2;
				break;
			case 4:
				which = jrand48(ctx->rand48) & 1;
				cls = get_random_dvd(ctx);
				mthd = 0x188 - which * 4;
				mode = 2;
				trapbit = 3 - which;
				break;
			case 5:
				which = jrand48(ctx->rand48) & 1;
				cls = 0x39;
				mthd = 0x184 + which * 4;
				mode = which ? 3 : 2;
				clr = !which;
				trapbit = 2 + which;
				break;
			case 6:
				if (nv04_pgraph_is_nv15p(&ctx->chipset))
					continue;
				cls = 0x48;
				mthd = 0x184;
				which = 0;
				mode = 2;
				is3d = true;
				trapbit = 2;
				break;
			case 7:
				which = jrand48(ctx->rand48) & 1;
				cls = get_random_d3d5(ctx);
				mthd = 0x184 + which * 4;
				mode = 2;
				is3d = true;
				trapbit = 2 + which;
				break;
			case 8:
				which = jrand48(ctx->rand48) & 1;
				cls = get_random_d3d6(ctx);
				mthd = 0x184 + which * 4;
				mode = 2;
				is3d = true;
				trapbit = 2 + which;
				break;
			case 9:
				if (ctx->chipset.card_type < 0x10)
					continue;
				which = jrand48(ctx->rand48) & 1;
				cls = get_random_celsius(ctx);
				mthd = 0x184 + which * 4;
				mode = 2;
				is3d = true;
				trapbit = 4 + which;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t dma[3];
		uint32_t val = nv04_pgraph_gen_dma(ctx, &orig, dma);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rval = val & 0xffff;
			int dcls = extr(dma[0], 0, 12);
			if (dcls == 0x30)
				rval = 0;
			bool bad = false;
			if (dcls != 0x30 && dcls != 0x3d && dcls != mode)
				bad = true;
			if (which == 2 && extr(exp.notify, 24, 1))
				nv04_pgraph_blowup(&exp, 0x2000);
			if (bad && extr(exp.debug[3], 23, 1) && ctx->chipset.chipset != 5)
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1)) {
				if (which == 2)
					insrt(egrobj[1], 16, 16, rval);
				else {
					insrt(egrobj[2], 16 * which, 16, rval);
					if (clr)
						insrt(egrobj[2], 16, 16, 0);
				}
			}
			if (bad && extr(exp.debug[3], 23, 1))
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.nsource) {
				int subc = extr(exp.ctx_user, 13, 3);
				if (which == 2) {
					exp.ctx_cache[subc][1] = exp.ctx_switch[1];
					insrt(exp.ctx_cache[subc][1], 16, 16, rval);
					if (extr(exp.debug[1], 20, 1))
						exp.ctx_switch[1] = exp.ctx_cache[subc][1];
				} else {
					exp.ctx_cache[subc][2] = exp.ctx_switch[2];
					insrt(exp.ctx_cache[subc][2], 16 * which, 16, rval);
					if (clr)
						insrt(exp.ctx_cache[subc][2], 16, 16, 0);
					if (extr(exp.debug[1], 20, 1))
						exp.ctx_switch[2] = exp.ctx_cache[subc][2];
				}
			}
			bool prot_err = false;
			bool check_prot = true;
			if (ctx->chipset.card_type >= 0x10)
				check_prot = dcls != 0x30;
			else if (ctx->chipset.chipset >= 5)
				check_prot = dcls == 2 || dcls == 0x3d;
			if (is3d && check_prot) {
				if (extr(dma[1], 0, 8) != 0xff)
					prot_err = true;
				if (cls != 0x48 && extr(dma[0], 20, 8))
					prot_err = true;
			}
			if (prot_err)
				nv04_pgraph_blowup(&exp, 4);
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("DMA %08x %08x %08x\n", dma[0], dma[1], dma[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_dma_celsius(struct hwtest_ctx *ctx) {
	int i;
	if (ctx->chipset.card_type < 0x10)
		return HWTEST_RES_NA;
	for (i = 0; i < 10000; i++) {
		uint32_t cls = get_random_celsius(ctx);
		int which = jrand48(ctx->rand48) & 1;
		uint32_t mthd = 0x18c + which * 4;
		int trapbit = 6 + which;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		int mode = which ? 3 : 2;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t dma[3];
		uint32_t val = nv04_pgraph_gen_dma(ctx, &orig, dma);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rval = val & 0xffff;
			int dcls = extr(dma[0], 0, 12);
			if (dcls == 0x30)
				rval = 0;
			bool bad = false;
			if (dcls != 0x30 && dcls != 0x3d && dcls != mode)
				bad = true;
			if (bad && extr(exp.debug[3], 23, 1))
				nv04_pgraph_blowup(&exp, 2);
			bool prot_err = false;
			if (which == 0 && dcls != 0x30) {
				if (extr(dma[1], 0, 8) != 0xff)
					prot_err = true;
				if (extr(dma[0], 20, 8))
					prot_err = true;
				if (!extr(dma[0], 12, 2)) {
					exp.intr |= 0x400;
					exp.fifo_enable = 0;
				}
			}
			if (prot_err)
				nv04_pgraph_blowup(&exp, 4);
			insrt(exp.celsius_unkf4c, 16 * which, 16, rval);
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("DMA %08x %08x %08x\n", dma[0], dma[1], dma[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_clip(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset < 5)
		return HWTEST_RES_NA;
	for (int i = 0; i < 10000; i++) {
		uint32_t cls, mthd;
		int trapbit = -1;
		switch (nrand48(ctx->rand48) % 10) {
			default:
				cls = 0x1c;
				mthd = 0x184;
				trapbit = 2;
				break;
			case 1:
				cls = 0x5c;
				mthd = 0x184;
				trapbit = 2;
				break;
			case 2:
				cls = 0x1d;
				mthd = 0x184;
				trapbit = 2;
				break;
			case 3:
				cls = 0x5d;
				mthd = 0x184;
				trapbit = 2;
				break;
			case 4:
				cls = 0x1e;
				mthd = 0x184;
				trapbit = 2;
				break;
			case 5:
				cls = 0x5e;
				mthd = 0x184;
				trapbit = 2;
				break;
			case 6:
				cls = get_random_blit(ctx);
				mthd = 0x188;
				trapbit = 3;
				break;
			case 7:
				cls = get_random_ifc(ctx);
				mthd = 0x188;
				trapbit = 3;
				break;
			case 8:
				cls = get_random_iifc(ctx);
				mthd = 0x18c;
				trapbit = 4;
				break;
			case 9:
				if (nv04_pgraph_is_nv15p(&ctx->chipset))
					continue;
				cls = 0x48;
				mthd = 0x188;
				trapbit = 3;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t ctxobj[4];
		uint32_t val = nv04_pgraph_gen_ctxobj(ctx, &orig, ctxobj);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		if (!extr(exp.debug[3], 29, 1))
			trapbit = -1;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (extr(exp.debug[3], 29, 1)) {
				int ccls = extr(ctxobj[0], 0, 12);
				bool bad = ccls != 0x19 && ccls != 0x30;
				if (ctx->chipset.card_type < 0x10 && !extr(exp.nsource, 1, 1)) {
					insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
					insrt(egrobj[0], 13, 1, ccls != 0x30);
				}
				if (bad && extr(exp.debug[3], 23, 1))
					nv04_pgraph_blowup(&exp, 2);
				if (!extr(exp.nsource, 1, 1)) {
					if (ctx->chipset.card_type >= 0x10) {
						insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
						insrt(egrobj[0], 13, 1, ccls != 0x30);
					}
					int subc = extr(exp.ctx_user, 13, 3);
					exp.ctx_cache[subc][0] = exp.ctx_switch[0];
					insrt(exp.ctx_cache[subc][0], 13, 1, ccls != 0x30);
					if (extr(exp.debug[1], 20, 1))
						exp.ctx_switch[0] = exp.ctx_cache[subc][0];
				}
			} else {
				nv04_pgraph_blowup(&exp, 0x40);
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("CTX %08x %08x %08x\n", ctxobj[0], ctxobj[1], ctxobj[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_chroma(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset < 5)
		return HWTEST_RES_NA;
	for (int i = 0; i < 10000; i++) {
		uint32_t cls, mthd;
		int trapbit;
		bool isnew;
		switch (nrand48(ctx->rand48) % 4) {
			default:
				cls = get_random_blit(ctx);
				mthd = 0x184;
				trapbit = 2;
				isnew = cls != 0x1f;
				break;
			case 1:
				cls = get_random_ifc(ctx);
				mthd = 0x184;
				trapbit = 2;
				isnew = cls != 0x21;
				break;
			case 2:
				cls = get_random_sifc(ctx);
				mthd = 0x184;
				trapbit = 2;
				isnew = cls != 0x36;
				break;
			case 3:
				cls = get_random_iifc(ctx);
				mthd = 0x188;
				trapbit = 3;
				isnew = true;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t ctxobj[4];
		uint32_t val = nv04_pgraph_gen_ctxobj(ctx, &orig, ctxobj);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		if (!extr(exp.debug[3], 29, 1))
			trapbit = -1;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (extr(exp.debug[3], 29, 1)) {
				int ccls = extr(ctxobj[0], 0, 12);
				int ecls = (isnew || ctx->chipset.card_type < 0x10 ? 0x57 : 0x17);
				bool bad = ccls != ecls && ccls != 0x30;
				if (ctx->chipset.card_type < 0x10 && !extr(exp.nsource, 1, 1)) {
					insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
					insrt(egrobj[0], 12, 1, ccls != 0x30);
				}
				if (bad && extr(exp.debug[3], 23, 1))
					nv04_pgraph_blowup(&exp, 2);
				if (!extr(exp.nsource, 1, 1)) {
					if (ctx->chipset.card_type >= 0x10) {
						insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
						insrt(egrobj[0], 12, 1, ccls != 0x30);
					}
					int subc = extr(exp.ctx_user, 13, 3);
					exp.ctx_cache[subc][0] = exp.ctx_switch[0];
					insrt(exp.ctx_cache[subc][0], 12, 1, ccls != 0x30);
					if (extr(exp.debug[1], 20, 1))
						exp.ctx_switch[0] = exp.ctx_cache[subc][0];
				}
			} else {
				nv04_pgraph_blowup(&exp, 0x40);
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("CTX %08x %08x %08x\n", ctxobj[0], ctxobj[1], ctxobj[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_rop(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset < 5)
		return HWTEST_RES_NA;
	for (int i = 0; i < 10000; i++) {
		uint32_t cls, mthd;
		bool isnew = true;
		int which;
		int trapbit;
		switch (nrand48(ctx->rand48) % 13) {
			default:
				which = nrand48(ctx->rand48) % 3;
				cls = 0x1c;
				mthd = 0x188 + which * 4;
				isnew = false;
				trapbit = 3 + which;
				break;
			case 1:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x5c;
				mthd = 0x188 + which * 4;
				trapbit = 3 + which;
				break;
			case 2:
				which = nrand48(ctx->rand48) % 3;
				cls = 0x1d;
				mthd = 0x188 + which * 4;
				isnew = false;
				trapbit = 3 + which;
				break;
			case 3:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x5d;
				mthd = 0x188 + which * 4;
				trapbit = 3 + which;
				break;
			case 4:
				which = nrand48(ctx->rand48) % 3;
				cls = 0x1e;
				mthd = 0x188 + which * 4;
				isnew = false;
				trapbit = 3 + which;
				break;
			case 5:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x5e;
				mthd = 0x188 + which * 4;
				trapbit = 3 + which;
				break;
			case 6:
				which = nrand48(ctx->rand48) % 3;
				cls = get_random_blit(ctx);
				mthd = 0x18c + which * 4;
				isnew = cls != 0x1f;
				trapbit = 4 + which;
				break;
			case 7:
				which = nrand48(ctx->rand48) % 3;
				cls = get_random_ifc(ctx);
				mthd = 0x18c + which * 4;
				isnew = cls != 0x21;
				trapbit = 4 + which;
				break;
			case 8:
				which = nrand48(ctx->rand48) % 3;
				cls = get_random_sifc(ctx);
				mthd = 0x188 + which * 4;
				isnew = cls != 0x36;
				trapbit = 3 + which;
				break;
			case 9:
				which = nrand48(ctx->rand48) % 3;
				cls = get_random_sifm(ctx);
				mthd = 0x188 + which * 4;
				isnew = cls != 0x37;
				trapbit = 3 + which;
				break;
			case 10:
				which = nrand48(ctx->rand48) % 3;
				cls = 0x4b;
				mthd = 0x184 + which * 4;
				isnew = false;
				trapbit = 3 + which;
				break;
			case 11:
				which = nrand48(ctx->rand48) % 4;
				cls = 0x4a;
				mthd = 0x188 + which * 4;
				trapbit = 3 + which;
				break;
			case 12:
				which = nrand48(ctx->rand48) % 4;
				cls = get_random_iifc(ctx);
				mthd = 0x190 + which * 4;
				trapbit= 5 + which;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t ctxobj[4];
		uint32_t val = nv04_pgraph_gen_ctxobj(ctx, &orig, ctxobj);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		if (!extr(exp.debug[3], 29, 1))
			trapbit = -1;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (extr(exp.debug[3], 29, 1)) {
				int ccls = extr(ctxobj[0], 0, 12);
				bool bad = false;
				int ecls = 0;
				if (which == 0)
					ecls = isnew ? 0x44 : 0x18;
				if (which == 1)
					ecls = 0x43;
				if (which == 2)
					ecls = 0x12;
				if (which == 3)
					ecls = 0x72;
				bad = ccls != ecls && ccls != 0x30;
				if (ctx->chipset.card_type < 0x10 && !extr(exp.nsource, 1, 1)) {
					insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
					insrt(egrobj[0], 27 + which, 1, ccls != 0x30);
				}
				if (bad && extr(exp.debug[3], 23, 1))
					nv04_pgraph_blowup(&exp, 2);
				if (!extr(exp.nsource, 1, 1)) {
					if (ctx->chipset.card_type >= 0x10) {
						insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
						insrt(egrobj[0], 27 + which, 1, ccls != 0x30);
					}
					int subc = extr(exp.ctx_user, 13, 3);
					exp.ctx_cache[subc][0] = exp.ctx_switch[0];
					insrt(exp.ctx_cache[subc][0], 27 + which, 1, ccls != 0x30);
					if (extr(exp.debug[1], 20, 1))
						exp.ctx_switch[0] = exp.ctx_cache[subc][0];
				}
			} else {
				nv04_pgraph_blowup(&exp, 0x40);
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("CTX %08x %08x %08x\n", ctxobj[0], ctxobj[1], ctxobj[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_surf_nv3(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset < 5)
		return HWTEST_RES_NA;
	for (int i = 0; i < 10000; i++) {
		uint32_t cls, mthd;
		int which;
		int trapbit;
		switch (nrand48(ctx->rand48) % 11) {
			default:
				which = 0;
				cls = 0x1c;
				mthd = 0x194;
				trapbit = 7;
				break;
			case 1:
				which = 0;
				cls = 0x1d;
				mthd = 0x194;
				trapbit = 7;
				break;
			case 2:
				which = 0;
				cls = 0x1e;
				mthd = 0x194;
				trapbit = 7;
				break;
			case 3:
				which = 0;
				cls = 0x1f;
				mthd = 0x19c;
				trapbit = 9;
				break;
			case 4:
				which = 1;
				cls = 0x1f;
				mthd = 0x198;
				trapbit = 8;
				break;
			case 5:
				which = 0;
				cls = 0x21;
				mthd = 0x198;
				trapbit = 8;
				break;
			case 6:
				which = 0;
				cls = 0x36;
				mthd = 0x194;
				trapbit = 7;
				break;
			case 7:
				which = 0;
				cls = 0x37;
				mthd = 0x194;
				trapbit = 7;
				break;
			case 8:
				which = 0;
				cls = 0x4b;
				mthd = 0x190;
				trapbit = 7;
				break;
			case 9:
				if (nv04_pgraph_is_nv15p(&ctx->chipset))
					continue;
				which = 2;
				cls = 0x48;
				mthd = 0x18c;
				trapbit = 4;
				break;
			case 10:
				if (nv04_pgraph_is_nv15p(&ctx->chipset))
					continue;
				which = 3;
				cls = 0x48;
				mthd = 0x190;
				trapbit = 5;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t ctxobj[4];
		uint32_t val = nv04_pgraph_gen_ctxobj(ctx, &orig, ctxobj);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		if (!extr(exp.debug[3], 29, 1))
			trapbit = -1;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (extr(exp.debug[3], 29, 1)) {
				int ccls = extr(ctxobj[0], 0, 12);
				bool bad = false;
				int ecls = 0x58 + which;
				bad = ccls != ecls && ccls != 0x30;
				bool isswz = ccls == 0x52;
				if (nv04_pgraph_is_nv15p(&ctx->chipset) && ccls == 0x9e)
					isswz = true;
				if (ctx->chipset.card_type < 0x10 && !extr(exp.nsource, 1, 1)) {
					insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
					insrt(egrobj[0], 25 + (which & 1), 1, ccls != 0x30);
					if (which == 0 || which == 2)
						insrt(egrobj[0], 14, 1, isswz);
				}
				if (bad && extr(exp.debug[3], 23, 1))
					nv04_pgraph_blowup(&exp, 2);
				if (!extr(exp.nsource, 1, 1)) {
					if (ctx->chipset.card_type >= 0x10) {
						insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
						insrt(egrobj[0], 25 + (which & 1), 1, ccls != 0x30);
						if (which == 0 || which == 2)
							insrt(egrobj[0], 14, 1, isswz);
					}
					int subc = extr(exp.ctx_user, 13, 3);
					exp.ctx_cache[subc][0] = exp.ctx_switch[0];
					insrt(exp.ctx_cache[subc][0], 25 + (which & 1), 1, ccls != 0x30);
					if (which == 0 || which == 2)
						insrt(exp.ctx_cache[subc][0], 14, 1, isswz);
					if (extr(exp.debug[1], 20, 1))
						exp.ctx_switch[0] = exp.ctx_cache[subc][0];
				}
			} else {
				nv04_pgraph_blowup(&exp, 0x40);
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("CTX %08x %08x %08x\n", ctxobj[0], ctxobj[1], ctxobj[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_surf_nv4(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t cls, mthd;
		bool swzok = false;
		bool is3d = false;
		int trapbit = -1;
		bool newok = true;
		switch (nrand48(ctx->rand48) % 12) {
			default:
				cls = 0x5c;
				mthd = 0x198;
				trapbit = 7;
				break;
			case 1:
				cls = 0x5d;
				mthd = 0x198;
				trapbit = 7;
				break;
			case 2:
				cls = 0x5e;
				mthd = 0x198;
				trapbit = 7;
				break;
			case 3:
				cls = get_random_blit(ctx, true);
				mthd = 0x19c;
				trapbit = 8;
				break;
			case 4:
				cls = get_random_ifc(ctx, true);
				mthd = 0x19c;
				trapbit = 8;
				break;
			case 5:
				cls = get_random_sifc(ctx, true);
				mthd = 0x198;
				trapbit = 7;
				break;
			case 6:
				cls = get_random_sifm(ctx, true);
				mthd = 0x198;
				swzok = true;
				newok = cls != 0x77;
				trapbit = 7;
				break;
			case 7:
				cls = 0x4a;
				mthd = 0x198;
				trapbit = 7;
				break;
			case 8:
				cls = get_random_iifc(ctx);
				mthd = 0x1a0;
				swzok = true;
				newok = cls != 0x60;
				trapbit = 9;
				break;
			case 9:
				if (ctx->chipset.card_type < 0x10)
					continue;
				cls = 0x7b;
				mthd = 0x184;
				swzok = true;
				trapbit = 3;
				break;
			case 10:
				cls = get_random_d3d5(ctx);
				mthd = 0x18c;
				is3d = true;
				trapbit = 4;
				newok = cls != 0x54;
				break;
			case 11:
				cls = get_random_d3d6(ctx);
				mthd = 0x18c;
				is3d = true;
				trapbit = 4;
				newok = cls != 0x55;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t ctxobj[4];
		uint32_t val = nv04_pgraph_gen_ctxobj(ctx, &orig, ctxobj);
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		if (!extr(exp.debug[3], 29, 1) && !is3d)
			trapbit = -1;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (extr(exp.debug[3], 29, 1) || is3d) {
				int ccls = extr(ctxobj[0], 0, 12);
				bool bad = false;
				if (is3d) {
					bad = ccls != 0x53 && ccls != 0x30;
					if (ccls == 0x93 && ctx->chipset.card_type >= 0x10 && newok)
						bad = false;
				} else {
					bad = ccls != 0x42 && ccls != 0x52 && ccls != 0x30;
					if (ccls == (extr(exp.debug[3], 16, 1) ? 0x82 : 0x62) && ctx->chipset.card_type >= 0x10 && newok)
						bad = false;
					if (nv04_pgraph_is_nv15p(&ctx->chipset) && ccls == 0x9e && newok)
						bad = false;
				}
				bool isswz = ccls == 0x52;
				if (nv04_pgraph_is_nv15p(&ctx->chipset) && ccls == 0x9e)
					isswz = true;
				if (isswz && !swzok)
					bad = true;
				if (ctx->chipset.chipset >= 5 && ctx->chipset.card_type < 0x10 && !extr(exp.nsource, 1, 1)) {
					insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
					insrt(egrobj[0], 25, 1, ccls != 0x30);
					insrt(egrobj[0], 14, 1, isswz);
				}
				if (bad && extr(exp.debug[3], 23, 1))
					nv04_pgraph_blowup(&exp, 2);
				if (ctx->chipset.chipset < 5) {
					int subc = extr(exp.ctx_user, 13, 3);
					exp.ctx_cache[subc][0] = exp.ctx_switch[0];
					insrt(exp.ctx_cache[subc][0], 25, 1, ccls == 0x53);
					if (extr(exp.debug[1], 20, 1))
						exp.ctx_switch[0] = exp.ctx_cache[subc][0];
					if (!extr(exp.nsource, 1, 1))
						insrt(egrobj[0], 24, 8, extr(exp.ctx_cache[subc][0], 24, 8));
				} else {
					if (!extr(exp.nsource, 1, 1)) {
						if (ctx->chipset.card_type >= 0x10) {
							insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
							insrt(egrobj[0], 25, 1, ccls != 0x30);
							insrt(egrobj[0], 14, 1, isswz);
						}
						int subc = extr(exp.ctx_user, 13, 3);
						exp.ctx_cache[subc][0] = exp.ctx_switch[0];
						insrt(exp.ctx_cache[subc][0], 25, 1, ccls != 0x30);
						insrt(exp.ctx_cache[subc][0], 14, 1, isswz);
						if (extr(exp.debug[1], 20, 1))
							exp.ctx_switch[0] = exp.ctx_cache[subc][0];
					}
				}
			} else {
				nv04_pgraph_blowup(&exp, 0x40);
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("CTX %08x %08x %08x\n", ctxobj[0], ctxobj[1], ctxobj[2]);
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_operation(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		bool isnew = true;
		int trapbit;
		switch (nrand48(ctx->rand48) % 13) {
			default:
				cls = 0x1c;
				mthd = 0x2fc;
				isnew = false;
				trapbit = 8;
				break;
			case 1:
				cls = 0x5c;
				mthd = 0x2fc;
				trapbit = 8;
				break;
			case 2:
				cls = 0x1d;
				mthd = 0x2fc;
				isnew = false;
				trapbit = 8;
				break;
			case 3:
				cls = 0x5d;
				mthd = 0x2fc;
				trapbit = 8;
				break;
			case 4:
				cls = 0x1e;
				mthd = 0x2fc;
				isnew = false;
				trapbit = 8;
				break;
			case 5:
				cls = 0x5e;
				mthd = 0x2fc;
				trapbit = 8;
				break;
			case 6:
				cls = get_random_blit(ctx);
				mthd = 0x2fc;
				trapbit = 10;
				isnew = cls != 0x1f;
				break;
			case 7:
				cls = get_random_ifc(ctx);
				mthd = 0x2fc;
				trapbit = 9;
				isnew = cls != 0x21;
				break;
			case 8:
				cls = get_random_sifc(ctx);
				mthd = 0x2fc;
				trapbit = 8;
				isnew = cls != 0x36;
				break;
			case 9:
				cls = get_random_sifm(ctx);
				mthd = 0x304;
				trapbit = 9;
				isnew = cls != 0x37;
				break;
			case 10:
				cls = 0x4b;
				mthd = 0x2fc;
				trapbit = 8;
				isnew = false;
				break;
			case 11:
				cls = 0x4a;
				trapbit = 8;
				mthd = 0x2fc;
				break;
			case 12:
				cls = get_random_iifc(ctx);
				mthd = 0x3e4;
				trapbit = 10;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		if (!extr(exp.debug[3], 30, 1))
			trapbit = -1;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (extr(exp.debug[3], 30, 1)) {
				bool bad = false;
				if (val > (isnew ? 5 : 2))
					bad = true;
				if (bad && extr(exp.debug[3], 20, 1))
					nv04_pgraph_blowup(&exp, 2);
				if (!extr(exp.nsource, 1, 1)) {
					insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
					insrt(egrobj[0], 15, 3, val);
					int subc = extr(exp.ctx_user, 13, 3);
					exp.ctx_cache[subc][0] = exp.ctx_switch[0];
					insrt(exp.ctx_cache[subc][0], 15, 3, val);
					if (extr(exp.debug[1], 20, 1))
						exp.ctx_switch[0] = exp.ctx_cache[subc][0];
				}
			} else {
				nv04_pgraph_blowup(&exp, 0x40);
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_dither(struct hwtest_ctx *ctx) {
	int i;
	if (ctx->chipset.card_type < 0x10)
		return HWTEST_RES_NA;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		int trapbit = -1;
		switch (nrand48(ctx->rand48) % 5) {
			default:
				cls = (jrand48(ctx->rand48) & 1 ? 0x8a : 0x65);
				mthd = 0x2f8;
				trapbit = 15;
				break;
			case 1:
				cls = 0x66;
				mthd = 0x2f8;
				trapbit = 17;
				break;
			case 2:
				cls = 0x64;
				mthd = 0x3e0;
				trapbit = 18;
				break;
			case 3:
				cls = 0x7b;
				mthd = 0x2fc;
				trapbit = 4;
				break;
			case 4:
				cls = (jrand48(ctx->rand48) & 1 ? 0x89 : 0x63);
				mthd = 0x2fc;
				trapbit = 20;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			if (val > 2)
				bad = true;
			uint32_t rval = 0;
			if ((val & 3) == 0)
				rval = 1;
			if ((val & 3) == 1)
				rval = 2;
			if ((val & 3) == 2)
				rval = 3;
			if ((val & 3) == 3)
				rval = 3;
			if (bad && extr(exp.debug[3], 20, 1))
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1)) {
				insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
				insrt(egrobj[0], 20, 2, rval);
				int subc = extr(exp.ctx_user, 13, 3);
				exp.ctx_cache[subc][0] = exp.ctx_switch[0];
				insrt(exp.ctx_cache[subc][0], 20, 2, rval);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[0] = exp.ctx_cache[subc][0];
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_patch(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type >= 0x10)
		return HWTEST_RES_NA;
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd = 0x10c;
		switch (nrand48(ctx->rand48) % 13) {
			default:
				cls = 0x1c;
				break;
			case 1:
				cls = 0x5c;
				break;
			case 2:
				cls = 0x1d;
				break;
			case 3:
				cls = 0x5d;
				break;
			case 4:
				cls = 0x1e;
				break;
			case 5:
				cls = 0x5e;
				break;
			case 6:
				cls = get_random_blit(ctx);
				break;
			case 7:
				cls = get_random_ifc(ctx);
				break;
			case 8:
				cls = get_random_sifc(ctx);
				break;
			case 9:
				cls = get_random_sifm(ctx);
				break;
			case 10:
				cls = 0x4b;
				break;
			case 11:
				cls = 0x4a;
				break;
			case 12:
				cls = get_random_iifc(ctx);
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4], egrobj[4], rgrobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		for (int j = 0; j < 4; j++)
			egrobj[j] = grobj[j];
		nv04_pgraph_mthd(&exp, grobj);
		if (ctx->chipset.chipset < 5) {
			bool bad = false;
			if (val < 2) {
				if (extr(exp.debug[1], 8, 1) && val == 0) {
					int subc = extr(exp.ctx_user, 13, 3);
					exp.ctx_cache[subc][0] = exp.ctx_switch[0];
					insrt(exp.ctx_cache[subc][0], 24, 1, 0);
					if (extr(exp.debug[1], 20, 1))
						exp.ctx_switch[0] = exp.ctx_cache[subc][0];
				} else {
					exp.intr |= 0x10;
					exp.fifo_enable = 0;
				}
			} else {
				bad = true;
			}
			if (bad && extr(exp.debug[3], 20, 1))
				nv04_pgraph_blowup(&exp, 2);
			if (!extr(exp.nsource, 1, 1) && !extr(exp.intr, 4, 1)) {
				insrt(egrobj[0], 24, 8, extr(exp.ctx_switch[0], 24, 8));
				insrt(egrobj[0], 24, 1, 0);
			}
		} else {
			exp.intr |= 0x10;
			exp.fifo_enable = 0;
		}
		nv04_pgraph_dump_state(ctx, &real);
		bool err = false;
		uint32_t inst = exp.ctx_switch[3] & 0xffff;
		for (int j = 0; j < 4; j++) {
			rgrobj[j] = nva_rd32(ctx->cnum, 0x700000 | inst << 4 | j << 2);
			if (rgrobj[j] != egrobj[j]) {
				err = true;
				printf("Difference in GROBJ[%d]: expected %08x, real %08x\n", j, egrobj[j], rgrobj[j]);
			}
		}
		if (nv04_pgraph_cmp_state(&orig, &exp, &real, err)) {
			for (int j = 0; j < 4; j++) {
				printf("%08x %08x %08x GROBJ[%d] %s\n", grobj[j], egrobj[j], rgrobj[j], j, egrobj[j] != rgrobj[j] ? "*" : "");
			}
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_flip_set(struct hwtest_ctx *ctx) {
	if (!nv04_pgraph_is_nv15p(&ctx->chipset))
		return HWTEST_RES_NA;
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		int which = nrand48(ctx->rand48) % 3;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				cls = get_random_celsius(ctx);
				if (cls == 0x56)
					cls = 0x96;
				mthd = 0x120 + which * 4;
				break;
			case 1:
				cls = 0x9f;
				mthd = 0x120 + which * 4;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		int idx = 0;
		if (which == 0)
			idx = 1;
		if (which == 2)
			idx = 2;
		if (cls != 0x9f)
			idx += 3;
		insrt(exp.surf_type, 8+4*idx, 3, val);
		bool bad = val > 7;
		if (bad && extr(exp.debug[3], 20, 1))
			nv04_pgraph_blowup(&exp, 2);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_flip_inc_write(struct hwtest_ctx *ctx) {
	if (!nv04_pgraph_is_nv15p(&ctx->chipset))
		return HWTEST_RES_NA;
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		uint32_t cls, mthd;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				cls = get_random_celsius(ctx);
				if (cls == 0x56)
					cls = 0x96;
				mthd = 0x12c;
				break;
			case 1:
				cls = 0x9f;
				mthd = 0x12c;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj);
		int idx = 0;
		if (cls != 0x9f)
			idx = 3;
		int cur = extr(exp.surf_type, 8+4*idx, 3);
		int mod = extr(exp.surf_type, 16+4*idx, 3);
		int rval = cur + 1;
		if (rval == mod)
			rval = 0;
		insrt(exp.surf_type, 8+4*idx, 3, rval);
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_offset(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0x3ff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int idx;
		int which;
		int trapbit;
		uint32_t mask;
		switch (nrand48(ctx->rand48) % 4) {
			default:
				idx = jrand48(ctx->rand48) & 1;
				cls = get_random_celsius(ctx);
				mthd = 0x218 + idx * 4;
				trapbit = 15;
				mask = 0x7f;
				which = 1 << idx;
				break;
			case 1:
				idx = jrand48(ctx->rand48) & 1;
				cls = get_random_d3d6(ctx);
				mthd = 0x308 + idx * 4;
				trapbit = 5 + idx;
				mask = 0xff;
				which = 1 << idx;
				break;
			case 2:
				idx = 0;
				cls = get_random_d3d5(ctx);
				mthd = 0x304;
				trapbit = 6;
				mask = 0xff;
				which = 1;
				break;
			case 3:
				if (nv04_pgraph_is_nv15p(&ctx->chipset))
					continue;
				idx = 0;
				cls = 0x48;
				mthd = 0x304;
				trapbit = 6;
				mask = 0;
				which = 3;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (extr(exp.debug[3], 20, 1) && (val & mask))
				nv04_pgraph_blowup(&exp, 2);
			for (int j = 0; j < 2; j++) {
				if (which & (1 << j)) {
					if (!exp.intr)
						exp.celsius_tex_offset[j] = val;
					insrt(exp.valid[1], j ? 22 : 14, 1, 1);
				}
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_format(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 0, 2, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 3, 2, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 5, 2, 1);
			if (!(jrand48(ctx->rand48) & 3))
				insrt(val, 7, 5, 0x19 + nrand48(ctx->rand48) % 4);
			if (jrand48(ctx->rand48) & 1)
				insrt(val, 20, 4, extr(val, 16, 4));
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 24, 3, 3);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 28, 3, 3);
			if (jrand48(ctx->rand48) & 1) {
				if (jrand48(ctx->rand48) & 3)
					insrt(val, 2, 1, 0);
				if (jrand48(ctx->rand48) & 3)
					insrt(val, 12, 4, 1);
			}
		}
		uint32_t cls, mthd;
		int idx;
		int trapbit;
		idx = jrand48(ctx->rand48) & 1;
		cls = get_random_celsius(ctx);
		mthd = 0x220 + idx * 4;
		trapbit = 16;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rval = val & 0xffffffd6;
			int omips = extr(val, 12, 4);
			int mips = omips;
			int su = extr(val, 16, 4);
			int sv = extr(val, 20, 4);
			int fmt = extr(val, 7, 5);
			if (mips > su && mips > sv)
				mips = (su > sv ? su : sv) + 1;
			insrt(rval, 12, 4, mips);
			bool bad = false;
			if (!extr(val, 0, 2) || extr(val, 0, 2) == 3)
				bad = true;
			if (extr(val, 2, 1)) {
				if (su != sv)
					bad = true;
				if (su >= 0xa && (fmt == 6 || fmt == 7 || fmt == 0xb || fmt == 0xe || fmt == 0xf))
					bad = true;
				if (su >= 0xb)
					bad = true;
			}
			if (!extr(val, 3, 2) || extr(val, 3, 2) == 3)
				bad = true;
			if (!extr(val, 5, 2) || extr(val, 5, 2) == 3)
				bad = true;
			if (fmt == 0xd)
				bad = true;
			if (fmt >= 0x1d)
				bad = true;
			if (omips > 0xc || omips == 0)
				bad = true;
			if (fmt >= 0x19) {
				if (extr(val, 3, 2) != 1)
					bad = true;
				if (cls != 0x99)
					bad = true;
			}
			if (fmt >= 0x10) {
				if (extr(val, 2, 1))
					bad = true;
				if (extr(val, 24, 3) != 3)
					bad = true;
				if (extr(val, 28, 3) != 3)
					bad = true;
				if (omips != 1)
					bad = true;
			}
			if (su > 0xb || sv > 0xb)
				bad = true;
			if (extr(val, 24, 3) < 1 || extr(val, 24, 3) > 5)
				bad = true;
			if (extr(val, 28, 3) < 1 || extr(val, 28, 3) > 5)
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr)
				exp.celsius_tex_format[idx] = rval | (exp.celsius_tex_format[idx] & 8);
			insrt(exp.valid[1], idx ? 21 : 15, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_format_d3d56(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 0, 2, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 2, 2, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 4, 2, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 6, 2, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 11, 1, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 24, 3, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 28, 3, 1);
		}
		uint32_t cls, mthd;
		int idx;
		int trapbit;
		int which;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				which = 3;
				cls = get_random_d3d5(ctx);
				mthd = 0x308;
				trapbit = 7;
				break;
			case 1:
				idx = jrand48(ctx->rand48) & 1;
				cls = get_random_d3d6(ctx);
				mthd = 0x310 + idx * 4;
				trapbit = 7 + idx;
				which = 1 << idx;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rval = val & 0xfffff002;
			int omips = extr(val, 12, 4);
			int sfmt = extr(val, 8, 4);
			int fmt = sfmt;
			int mips = omips;
			int su = extr(val, 16, 4);
			int sv = extr(val, 20, 4);
			int wrapu = extr(val, 24, 3);
			int wrapv = extr(val, 28, 3);
			if (wrapu == 2)
				insrt(rval, 24, 4, 2);
			if (wrapu != 1)
				insrt(rval, 27, 1, 0);
			if (wrapu == 0)
				insrt(rval, 24, 3, nv04_pgraph_is_nv15p(&ctx->chipset) ? 2 : 3);
			if (wrapu == 4)
				insrt(rval, 24, 3, 3);
			if (wrapu == 5)
				insrt(rval, 24, 3, nv04_pgraph_is_nv15p(&ctx->chipset) ? 1 : 0);
			if (wrapu > 5)
				insrt(rval, 24, 3, 0);
			if (wrapv != 1)
				insrt(rval, 31, 1, 0);
			if (wrapv == 0)
				insrt(rval, 28, 3, nv04_pgraph_is_nv15p(&ctx->chipset) ? 0 : 2);
			if (wrapv == 7)
				insrt(rval, 28, 3, 0);
			if (wrapv == 4)
				insrt(rval, 28, 3, 3);
			if (wrapv == 5)
				insrt(rval, 28, 3, 1);
			if (wrapv == 6)
				insrt(rval, 28, 3, nv04_pgraph_is_nv15p(&ctx->chipset) ? 2 : 0);
			if (mips > su && mips > sv)
				mips = (su > sv ? su : sv) + 1;
			if (cls == 0x54 || cls == 0x94) {
				if (sfmt == 1)
					fmt = 0;
			}
			insrt(rval, 4, 1, extr(val, 5, 1));
			insrt(rval, 6, 1, extr(val, 7, 1));
			insrt(rval, 7, 5, fmt);
			insrt(rval, 12, 4, mips);
			bool bad = false;
			if (!extr(val, 0, 2) || extr(val, 0, 2) == 3)
				bad = true;
			if (!extr(val, 4, 2) || extr(val, 4, 2) == 3)
				bad = true;
			if (!extr(val, 6, 2) || extr(val, 6, 2) == 3)
				bad = true;
			if (!extr(val, 8, 3))
				bad = true;
			if (extr(val, 11, 1))
				bad = true;
			if (!extr(val, 12, 4))
				bad = true;
			if (extr(val, 16, 4) > 0xb)
				bad = true;
			if (extr(val, 20, 4) > 0xb)
				bad = true;
			if (extr(val, 24, 3) < 1 || extr(val, 24, 3) > 4)
				bad = true;
			if (extr(val, 28, 3) < 1 || extr(val, 28, 3) > 4)
				bad = true;
			if (cls == 0x54 || cls == 0x94) {
				if (extr(val, 2, 2) > 1)
					bad = true;
			} else {
				if (extr(val, 2, 2))
					bad = true;
			}
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				for (int idx = 0; idx < 2; idx++)
					if (which & 1 << idx)
						exp.celsius_tex_format[idx] = rval | (exp.celsius_tex_format[idx] & 8);
				if (cls == 0x94 || cls == 0x54) {
					exp.celsius_tex_control[0] = 0x4003ffc0 | extr(val, 2, 2);
					exp.celsius_tex_control[1] = 0x3ffc0 | extr(val, 2, 2);
					insrt(exp.celsius_unkf44, 2, 1,
						extr(exp.celsius_config_b, 6, 1) &&
						!extr(rval, 27, 1) && !extr(rval, 31, 1));
					insrt(exp.celsius_unkf44, 16, 1, 0);
				} else {
					if (idx == 1) {
						insrt(exp.celsius_unkf44, 16, 1,
							extr(exp.celsius_config_b, 6, 1) &&
							!extr(rval, 27, 1) && !extr(rval, 31, 1));
					}
				}
				if (which & 1) {
					insrt(exp.celsius_unkf44, 2, 1,
						extr(exp.celsius_config_b, 6, 1) &&
						!extr(rval, 27, 1) && !extr(rval, 31, 1));
				}
				insrt(exp.celsius_unkf40, 28, 1, 1);
			}
			if (which & 1)
				insrt(exp.valid[1], 15, 1, 1);
			if (which & 2)
				insrt(exp.valid[1], 21, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_format_d3d0(struct hwtest_ctx *ctx) {
	int i;
	if (nv04_pgraph_is_nv15p(&ctx->chipset))
		return HWTEST_RES_NA;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 22, 2, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 17, 3, 0);
		}
		uint32_t cls = 0x48, mthd = 0x308;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 7);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			int fmt = 5;
			int sfmt = extr(val, 20, 4);
			if (sfmt == 0)
				fmt = 2;
			else if (sfmt == 1)
				fmt = 3;
			else if (sfmt == 2)
				fmt = 4;
			else if (sfmt == 3)
				fmt = 5;
			else
				bad = true;
			int max_l = extr(val, 28, 4);
			int min_l = extr(val, 24, 4);
			if (min_l < 2)
				bad = true;
			if (min_l > max_l)
				bad = true;
			if (max_l > 0xb)
				bad = true;
			if (extr(val, 17, 3))
				bad = true;
			insrt(exp.valid[1], 21, 1, 1);
			insrt(exp.valid[1], 15, 1, 1);
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				for (int i = 0; i < 2; i++) {
					insrt(exp.celsius_tex_format[i], 1, 1, 0);
					insrt(exp.celsius_tex_format[i], 2, 1, 0);
					insrt(exp.celsius_tex_format[i], 6, 1, 0);
					insrt(exp.celsius_tex_format[i], 7, 5, fmt);
					insrt(exp.celsius_tex_format[i], 12, 4, max_l - min_l + 1);
					insrt(exp.celsius_tex_format[i], 16, 4, max_l);
					insrt(exp.celsius_tex_format[i], 20, 4, max_l);
				}
				exp.celsius_tex_control[0] = 0x4003ffc0 | extr(val, 16, 2);
				exp.celsius_tex_control[1] = 0x3ffc0 | extr(val, 16, 2);
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_control(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int idx;
		int trapbit;
		idx = jrand48(ctx->rand48) & 1;
		cls = get_random_celsius(ctx);
		mthd = 0x228 + idx * 4;
		trapbit = 17;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rval = val & 0x7fffffff;
			bool bad = false;
			if (extr(val, 31, 1))
				bad = true;
			if (extr(val, 5, 1))
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				exp.celsius_tex_control[idx] = rval;
				if (idx == 0) {
					insrt(exp.celsius_unkf44, 0, 1, extr(val, 30, 1));
				} else {
					insrt(exp.celsius_unkf44, 14, 1, extr(val, 30, 1));
				}
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_pitch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xffff;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int idx;
		int trapbit;
		idx = jrand48(ctx->rand48) & 1;
		cls = get_random_celsius(ctx);
		mthd = 0x230 + idx * 4;
		trapbit = 18;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			if (extr(val, 0, 16))
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				exp.celsius_tex_pitch[idx] = val & 0xffff0000;
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_unk238(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int idx;
		int trapbit;
		idx = jrand48(ctx->rand48) & 1;
		cls = get_random_celsius(ctx);
		mthd = 0x238 + idx * 4;
		trapbit = 19;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (!exp.intr) {
				exp.celsius_tex_unk238[idx] = val;
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_rect(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 1) {
				val &= ~0xf000f800;
			}
			if (jrand48(ctx->rand48) & 1) {
				if (jrand48(ctx->rand48) & 1) {
					val &= ~0xffff;
				} else {
					val &= ~0xffff0000;
				}
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int idx;
		int trapbit;
		idx = jrand48(ctx->rand48) & 1;
		cls = get_random_celsius(ctx);
		mthd = 0x240 + idx * 4;
		trapbit = 20;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			if (extr(val, 16, 1))
				bad = true;
			if (!extr(val, 0, 16) || extr(val, 0, 16) >= 0x800)
				bad = true;
			if (!extr(val, 17, 15) || extr(val, 17, 15) >= 0x800)
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				exp.celsius_tex_rect[idx] = val & 0x07ff07ff;
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_filter(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 13, 11, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 24, 4, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 28, 4, 1);
			if (jrand48(ctx->rand48) & 1) {
				val ^= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val ^= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int idx;
		int trapbit;
		idx = jrand48(ctx->rand48) & 1;
		cls = get_random_celsius(ctx);
		mthd = 0x248 + idx * 4;
		trapbit = 21;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rval = val & 0x77001fff;
			bool bad = false;
			if (extr(val, 13, 11))
				bad = true;
			if (extr(val, 24, 4) < 1 || extr(val, 24, 4) > 6)
				bad = true;
			if (extr(val, 28, 4) < 1 || extr(val, 28, 4) > 2)
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr)
				exp.celsius_tex_filter[idx] = rval;
			insrt(exp.valid[1], idx ? 23 : 16, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_filter_d3d56(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 13, 11, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 24, 4, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 28, 4, 1);
			if (jrand48(ctx->rand48) & 1) {
				val ^= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val ^= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int idx;
		int trapbit;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				idx = 0;
				cls = get_random_d3d5(ctx);
				mthd = 0x30c;
				trapbit = 8;
				break;
			case 1:
				idx = jrand48(ctx->rand48) & 1;
				cls = get_random_d3d6(ctx);
				mthd = 0x318 + idx * 4;
				trapbit = 9 + idx;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rval = val & 0x77000000;
			int fmag = extr(val, 28, 3);
			if (fmag & 1)
				insrt(rval, 28, 3, 1);
			else
				insrt(rval, 28, 3, 2);
			bool bad = false;
			if (extr(val, 13, 2))
				bad = true;
			if (extr(val, 5, 3))
				bad = true;
			if (extr(val, 24, 3) == 0 || extr(val, 24, 3) > 6)
				bad = true;
			if (extr(val, 28, 3) == 0 || extr(val, 28, 3) > 6)
				bad = true;
			insrt(rval, 5, 8, extr(val, 16, 8));
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				exp.celsius_tex_filter[idx] = rval;
			}
			insrt(exp.valid[1], idx ? 23 : 16, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_filter_d3d0(struct hwtest_ctx *ctx) {
	int i;
	if (nv04_pgraph_is_nv15p(&ctx->chipset))
		return HWTEST_RES_NA;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = 0x48, mthd = 0x30c;
		int trapbit = 8;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (!exp.intr) {
				insrt(exp.celsius_tex_filter[0], 0, 13, extrs(val, 16, 8) << 4);
				insrt(exp.celsius_tex_filter[1], 0, 13, extrs(val, 16, 8) << 4);
			}
			insrt(exp.valid[1], 16, 1, 1);
			insrt(exp.valid[1], 23, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_palette(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0x3c;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int idx;
		int trapbit;
		idx = jrand48(ctx->rand48) & 1;
		cls = get_random_celsius(ctx);
		mthd = 0x250 + idx * 4;
		trapbit = 22;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			uint32_t rval = val & 0xffffffc1;
			bool bad = false;
			if (val & 0x3e)
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				exp.celsius_tex_palette[idx] = rval;
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_rc_in(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (jrand48(ctx->rand48) & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (jrand48(ctx->rand48) & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (jrand48(ctx->rand48) & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int idx;
		int trapbit;
		bool color = jrand48(ctx->rand48) & 1;
		idx = jrand48(ctx->rand48) & 1;
		cls = get_random_celsius(ctx);
		mthd = 0x260 + idx * 4 + color * 8;
		trapbit = 23 + color;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			for (int j = 0; j < 4; j++) {
				int reg = extr(val, 8 * j, 4);
				if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb || reg >= 0xe)
					bad = true;
			}
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				exp.celsius_rc_in[color][idx] = val;
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_rc_factor(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int idx;
		int trapbit;
		bool celsius;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				idx = jrand48(ctx->rand48) & 1;
				cls = get_random_celsius(ctx);
				mthd = 0x270 + idx * 4;
				trapbit = 25;
				celsius = true;
				break;
			case 1:
				idx = 0;
				cls = get_random_d3d6(ctx);
				mthd = 0x334;
				trapbit = 15;
				celsius = false;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (!exp.intr) {
				if (celsius) {
					exp.celsius_rc_factor[idx] = val;
				} else {
					exp.celsius_rc_factor[0] = val;
					exp.celsius_rc_factor[1] = val;
				}
			}
			if (!celsius)
				insrt(exp.valid[1], 24, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_rc_out(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 1) {
				insrt(val, 18, 14, 0);
			}
			if (jrand48(ctx->rand48) & 1) {
				insrt(val, 28, 2, 1);
			}
			if (jrand48(ctx->rand48) & 1) {
				insrt(val, 12, 2, 0);
			}
			if (jrand48(ctx->rand48) & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (jrand48(ctx->rand48) & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (jrand48(ctx->rand48) & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int idx;
		int trapbit;
		bool color = jrand48(ctx->rand48) & 1;
		idx = jrand48(ctx->rand48) & 1;
		cls = get_random_celsius(ctx);
		mthd = 0x278 + idx * 4 + color * 8;
		trapbit = 26 + color;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			uint32_t rval;
			int op = extr(val, 15, 3);
			if (op == 5 || op == 7)
				bad = true;
			if (!color) {
				rval = val & 0x3cfff;
				if (extr(val, 12, 2))
					bad = true;
				if (extr(val, 27, 3))
					bad = true;
			} else if (!idx) {
				rval = val & 0x3ffff;
				if (extr(val, 27, 3))
					bad = true;
			} else {
				rval = val & 0x3803ffff;
				int cnt = extr(val, 28, 2);
				if (cnt == 0 || cnt == 3)
					bad = true;
			}
			for (int j = 0; j < 3; j++) {
				int reg = extr(val, 4 * j, 4);
				if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xc && reg != 0xd)
					bad = true;
			}
			if (extr(val, 18, 9))
				bad = true;
			if (extr(val, 30, 2))
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				exp.celsius_rc_out[color][idx] = rval;
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_rc_final(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		int which = jrand48(ctx->rand48) & 1;
		if (jrand48(ctx->rand48) & 1) {
			val &= which ? 0x3f3f3fe0 : 0x3f3f3f3f;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int trapbit;
		cls = get_random_celsius(ctx);
		mthd = 0x288 + which * 4;
		trapbit = 28 + which;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			uint32_t rval;
			if (!which) {
				rval = val & 0x3f3f3f3f;
				if (val & 0xc0c0c0c0)
					bad = true;
				for (int j = 0; j < 4; j++) {
					int reg = extr(val, 8 * j, 4);
					if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb)
						bad = true;
				}
			} else {
				rval = val & 0x3f3f3fe0;
				if (val & 0xc0c0c01f)
					bad = true;
				for (int j = 1; j < 4; j++) {
					int reg = extr(val, 8 * j, 4);
					if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb || reg >= 0xe)
						bad = true;
				}
			}
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				exp.celsius_rc_final[which] = rval;
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_rc_d3d6(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0x00e0e0e0;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls, mthd;
		int idx = jrand48(ctx->rand48) & 1;
		int ac = jrand48(ctx->rand48) & 1;
		int trapbit;
		cls = get_random_d3d6(ctx);
		mthd = 0x320 + idx * 0xc + ac * 4;
		trapbit = 11 + idx * 2 + ac;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			uint32_t rc_in = 0, rc_out = 0;
			bool t1 = false;
			bool comp = false;
			int sop = extr(val, 29, 3);
			for (int j = 0; j < 4; j++) {
				int sreg = extr(val, 8 * j + 2, 3);
				bool neg = extr(val, 8 * j, 1);
				bool alpha = extr(val, 1 + 8 * j, 1) || !ac;
				int reg = 0;
				if (sreg == 1) {
					reg = 0;
				} else if (sreg == 2) {
					reg = 2;
				} else if (sreg == 3) {
					reg = 4;
				} else if (sreg == 4) {
					reg = idx ? 0xc : 8;
				} else if (sreg == 5) {
					reg = 8;
				} else if (sreg == 6) {
					reg = 9;
					t1 = true;
				} else if (sreg == 7 && idx == 0 && cls == 0x55) {
					reg = 0;
				} else {
					bad = true;
				}
				insrt(rc_in, 5 + (3 - j) * 8, 1, neg);
				insrt(rc_in, 4 + (3 - j) * 8, 1, alpha);
				insrt(rc_in, 0 + (3 - j) * 8, 4, reg);
			}
			rc_out |= 0xc00;
			int op = 3;
			bool mux = false;
			if (sop == 1)
				op = 0;
			else if (sop == 2)
				op = 2;
			else if (sop == 3)
				op = 4;
			else if (sop == 4)
				op = 1;
			else if (sop == 5) {
				op = 0;
				mux = true;
			} else if (sop == 6) {
				op = 0;
				comp = true;
			} else if (sop == 7) {
				op = 3;
			} else {
				bad = true;
			}
			insrt(rc_out, 14, 1, mux);
			insrt(rc_out, 15, 3, op);
			if (val & 0x00e0e0e0)
				bad = true;
			if (ac)
				exp.d3d56_rc_color[idx] = val & 0xff1f1f1f;
			else
				exp.d3d56_rc_alpha[idx] = val & 0xfd1d1d1d;
			insrt(exp.valid[1], 28 - idx * 2 - ac, 1, 1);
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				exp.celsius_rc_in[ac][idx] = rc_in;
				insrt(exp.celsius_rc_out[ac][idx], 0, 18, rc_out);
				exp.celsius_tex_control[1] = 0x4003ffc0;
				if (idx == 0) {
					insrt(exp.celsius_unkf48, 16 + ac, 1, !t1);
				} else {
					insrt(exp.celsius_unkf48, 18 + ac, 1, 0);
					insrt(exp.celsius_rc_out[1][1], 28, 3, 2);
					if (ac == 1)
						exp.celsius_tex_control[0] = 0x4003ffc0;
				}
				insrt(exp.celsius_unkf48, 20 + ac + idx * 2, 1, comp);
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_unk290(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xceeeeefe;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = get_random_celsius(ctx);
		uint32_t mthd = 0x290;
		int trapbit = 30;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			if (val & 0xceeeeefe)
				bad = true;
			if (extr(val, 28, 2) && cls != 0x99)
				bad = true;
			if (extr(val, 8, 1) && !extr(val, 16, 1))
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			insrt(exp.valid[1], 17, 1, 1);
			if (!exp.intr) {
				insrt(exp.celsius_config_a, 25, 1, extr(val, 0, 1));
				insrt(exp.celsius_config_a, 23, 1, extr(val, 16, 1));
				insrt(exp.celsius_config_b, 2, 1, extr(val, 24, 1));
				insrt(exp.celsius_config_b, 6, 1, extr(val, 20, 1));
				insrt(exp.celsius_config_b, 10, 4, extr(val, 8, 4));
				if (nv04_pgraph_is_nv17p(&ctx->chipset))
					insrt(exp.celsius_config_b, 14, 2, extr(val, 28, 2));
				insrt(exp.celsius_unke88, 29, 1, extr(val, 12, 1));
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_light_model(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xfffefff8;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = get_random_celsius(ctx);
		uint32_t mthd = 0x294;
		int trapbit = 31;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			if (val & 0xfffefff8)
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				insrt(exp.celsius_unkf40, 18, 1, extr(val, 2, 1));
				insrt(exp.celsius_unkf40, 19, 1, extr(val, 0, 1));
				insrt(exp.celsius_unkf40, 20, 1, extr(val, 1, 1));
				insrt(exp.celsius_unkf44, 28, 1, extr(val, 16, 1));
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_fog_color_d3d(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls;
		uint32_t mthd;
		int trapbit;
		switch (nrand48(ctx->rand48) % 3) {
			default:
				if (nv04_pgraph_is_nv15p(&ctx->chipset))
					continue;
				cls = 0x48;
				mthd = 0x310;
				trapbit = 9;
				break;
			case 1:
				cls = get_random_d3d5(ctx);
				mthd = 0x318;
				trapbit = 11;
				break;
			case 2:
				cls = get_random_d3d6(ctx);
				mthd = 0x348;
				trapbit = 20;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			insrt(exp.valid[1], 13, 1, 1);
			if (!exp.intr) {
				exp.celsius_fog_color = val;
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_config_d3d0(struct hwtest_ctx *ctx) {
	if (nv04_pgraph_is_nv15p(&ctx->chipset))
		return HWTEST_RES_NA;
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 0, 4, 1);
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x48;
		uint32_t mthd = 0x314;
		int trapbit = 10;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			if (extr(val, 9, 1))
				bad = true;
			if (!extr(val, 12, 2))
				bad = true;
			if (extr(val, 14, 1))
				bad = true;
			if (extr(val, 16, 4) < 1 || extr(val, 16, 4) > 8)
				bad = true;
			if (extr(val, 20, 4) > 4)
				bad = true;
			if (extr(val, 24, 4) > 4)
				bad = true;
			int filt = 1;
			int origin = 0;
			int sinterp = extr(val, 0, 4);
			if (sinterp == 0)
				origin = 1;
			if (sinterp == 2)
				filt = 2;
			if (sinterp > 2)
				bad = true;
			int wrapu = extr(val, 4, 2);
			if (wrapu == 0)
				wrapu = 9;
			int wrapv = extr(val, 6, 2);
			if (wrapv == 0)
				wrapv = 9;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			uint32_t rc_in_alpha_a = 0x18;;
			uint32_t rc_in_alpha_b = 0x14;;
			if (!extr(val, 8, 1))
				rc_in_alpha_b = 0x20;
			uint32_t rc_final_1 = 0x1c80;
			int srcmode = extr(val, 10, 2);
			if (srcmode == 1)
				rc_final_1 |= 0x2020;
			if (srcmode == 2)
				rc_final_1 |= 0x2000;
			if (srcmode == 3) {
				rc_in_alpha_a = 0x20;
				rc_in_alpha_b = 0x20;
			}
			int scull = extr(val, 12, 2);
			int sblend = 4, dblend = 5;
			if (extr(val, 29, 1)) {
				dblend = 9;
				sblend = 8;
			}
			if (extr(val, 28, 1))
				sblend = dblend = 1;
			if (extr(val, 30, 1))
				dblend = 0;
			if (extr(val, 31, 1))
				sblend = 0;
			if (!exp.intr) {
				for (int i = 0; i < 2; i++) {
					insrt(exp.celsius_tex_format[i], 4, 1, origin);
					insrt(exp.celsius_tex_format[i], 24, 4, wrapu);
					insrt(exp.celsius_tex_format[i], 28, 4, wrapv);
					insrt(exp.celsius_tex_filter[i], 24, 3, filt);
					insrt(exp.celsius_tex_filter[i], 28, 3, filt);
				}
				insrt(exp.celsius_config_a, 14, 1, !!extr(val, 20, 3));
				insrt(exp.celsius_config_a, 16, 4, extr(val, 16, 4) - 1);
				insrt(exp.celsius_config_a, 22, 1, 1);
				insrt(exp.celsius_config_a, 23, 1, extr(val, 15, 1));
				insrt(exp.celsius_config_a, 24, 1, !!extr(val, 20, 3));
				insrt(exp.celsius_config_a, 25, 1, 0);
				insrt(exp.celsius_config_a, 26, 1, 0);
				insrt(exp.celsius_config_a, 27, 3, extr(val, 24, 3) ? 0x7 : 0);
				exp.celsius_stencil_func = 0x70;
				exp.celsius_stencil_op = 0x222;
				insrt(exp.celsius_config_b, 0, 9, 0x1c0);
				exp.celsius_blend = 0xa | sblend << 4 | dblend << 8;
				insrt(exp.celsius_unke88, 0, 21, 0);
				insrt(exp.celsius_unke88, 21, 2, scull == 3 ? 1 : 2);
				insrt(exp.celsius_unke88, 23, 5, 8);
				insrt(exp.celsius_unke88, 28, 2, scull != 1);
				exp.celsius_rc_factor[0] = val;
				exp.celsius_rc_factor[1] = val;
				exp.celsius_rc_in[0][0] = rc_in_alpha_a << 24 | rc_in_alpha_b << 16;
				exp.celsius_rc_in[1][0] = 0x08040000;
				exp.celsius_rc_out[0][0] = 0xc00;
				exp.celsius_rc_out[1][0] = 0xc00;
				insrt(exp.celsius_rc_out[1][1], 27, 3, 2);
				exp.celsius_rc_final[0] = 0xc;
				exp.celsius_rc_final[1] = rc_final_1;
			}
			insrt(exp.valid[1], 17, 1, 1);
			insrt(exp.valid[1], 24, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_alpha_d3d0(struct hwtest_ctx *ctx) {
	if (nv04_pgraph_is_nv15p(&ctx->chipset))
		return HWTEST_RES_NA;
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 8, 4, 1);
			val &= ~0xfffff000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = 0x48, mthd = 0x318;
		int trapbit = 11;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			if (extr(val, 8, 4) < 1 || extr(val, 8, 4) > 8)
				bad = true;
			if (extr(val, 12, 20))
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				insrt(exp.celsius_config_a, 0, 8, val);
				insrt(exp.celsius_config_a, 8, 4, extr(val, 8, 4) - 1);
				insrt(exp.celsius_config_a, 12, 1, 1);
			}
			insrt(exp.valid[1], 18, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_tex_color_key_d3d5(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls, mthd;
		int trapbit;
		cls = get_random_d3d5(ctx);
		mthd = 0x300;
		trapbit = 5;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			if (!exp.intr)
				exp.celsius_tex_color_key[0] = val;
			insrt(exp.valid[1], 12, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_blend_d3d56(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 9, 3, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 13, 3, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 17, 3, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 21, 3, 0);
			if (jrand48(ctx->rand48) & 1)
				insrt(val, 0, 4, 0);
		}
		uint32_t cls, mthd;
		int trapbit;
		bool is_d3d6;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				cls = get_random_d3d5(ctx);
				mthd = 0x310;
				trapbit = 9;
				is_d3d6 = false;
				break;
			case 1:
				cls = get_random_d3d6(ctx);
				mthd = 0x338;
				trapbit = 16;
				is_d3d6 = true;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			if (!is_d3d6) {
				if (extr(val, 0, 4) < 1 || extr(val, 0, 4) > 8)
					bad = true;
			} else {
				if (extr(val, 0, 4))
					bad = true;
			}
			if (extr(val, 4, 2) < 1 || extr(val, 4, 2) > 2)
				bad = true;
			if (!extr(val, 6, 2))
				bad = true;
			if (extr(val, 9, 3))
				bad = true;
			if (extr(val, 13, 3))
				bad = true;
			if (extr(val, 17, 3))
				bad = true;
			if (extr(val, 21, 3))
				bad = true;
			if (extr(val, 24, 4) < 1 || extr(val, 24, 4) > 0xb)
				bad = true;
			if (extr(val, 28, 4) < 1 || extr(val, 28, 4) > 0xb)
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			int colreg = extr(val, 12, 1) ? 0xe : 0xc;
			int op = extr(val, 0, 4);
			if (is_d3d6)
				op = 0;
			if (!exp.intr) {
				exp.celsius_rc_final[0] = extr(val, 16, 1) ? 0x13000300 | colreg << 16 : colreg;
				exp.celsius_rc_final[1] = 0x1c80;
				insrt(exp.celsius_blend, 0, 3, 2);
				insrt(exp.celsius_blend, 3, 1, extr(val, 20, 1));
				insrt(exp.celsius_blend, 4, 4, extr(val, 24, 4) - 1);
				insrt(exp.celsius_blend, 8, 4, extr(val, 28, 4) - 1);
				insrt(exp.celsius_blend, 12, 5, 0);
				insrt(exp.celsius_config_b, 0, 5, 0);
				insrt(exp.celsius_config_b, 5, 1, extr(val, 12, 1));
				insrt(exp.celsius_config_b, 6, 1, extr(val, 8, 1));
				insrt(exp.celsius_config_b, 7, 1, extr(val, 7, 1));
				insrt(exp.celsius_config_b, 8, 1, extr(val, 16, 1));
				insrt(exp.celsius_unkf40, 28, 1, 1);
				insrt(exp.celsius_unkf44, 2, 1, 0);
				insrt(exp.celsius_rc_out[1][1], 27, 1, extr(val, 5, 1));
				if (cls == 0x55 || cls == 0x95) {
					insrt(exp.celsius_rc_out[1][1], 28, 2, extr(exp.celsius_unkf48, 18, 2) == 3 ? 1 : 2);
				} else {
					insrt(exp.celsius_rc_out[1][1], 28, 2, 1);
				}
				insrt(exp.celsius_unkf44, 2, 1,
					extr(exp.celsius_config_b, 6, 1) &&
					!extr(exp.celsius_tex_format[0], 27, 1) &&
					!extr(exp.celsius_tex_format[0], 31, 1));
				insrt(exp.celsius_unkf44, 16, 1,
					is_d3d6 && extr(exp.celsius_config_b, 6, 1) &&
					!extr(exp.celsius_tex_format[1], 27, 1) &&
					!extr(exp.celsius_tex_format[1], 31, 1));
				if (op == 1 || op == 7) {
					exp.celsius_rc_in[0][0] = 0x18200000;
					exp.celsius_rc_in[1][0] = 0x08200000;
					exp.celsius_rc_out[0][0] = 0x00c00;
					exp.celsius_rc_out[1][0] = 0x00c00;
				} else if (op == 2) {
					exp.celsius_rc_in[0][0] = 0x18200000;
					exp.celsius_rc_in[1][0] = 0x08040000;
					exp.celsius_rc_out[0][0] = 0x00c00;
					exp.celsius_rc_out[1][0] = 0x00c00;
				} else if (op == 3) {
					exp.celsius_rc_in[0][0] = 0x14200000;
					exp.celsius_rc_in[1][0] = 0x08180438;
					exp.celsius_rc_out[0][0] = 0x00c00;
					exp.celsius_rc_out[1][0] = 0x00c00;
				} else if (op == 4) {
					exp.celsius_rc_in[0][0] = 0x18140000;
					exp.celsius_rc_in[1][0] = 0x08040000;
					exp.celsius_rc_out[0][0] = 0x00c00;
					exp.celsius_rc_out[1][0] = 0x00c00;
				} else if (op == 5) {
					exp.celsius_rc_in[0][0] = 0x14201420;
					exp.celsius_rc_in[1][0] = 0x04200820;
					exp.celsius_rc_out[0][0] = 0x04c00;
					exp.celsius_rc_out[1][0] = 0x04c00;
				} else if (op == 6) {
					exp.celsius_rc_in[0][0] = 0x14201420;
					exp.celsius_rc_in[1][0] = 0x04200408;
					exp.celsius_rc_out[0][0] = 0x04c00;
					exp.celsius_rc_out[1][0] = 0x04c00;
				} else if (op == 8) {
					exp.celsius_rc_in[0][0] = 0x14200000;
					exp.celsius_rc_in[1][0] = 0x08200420;
					exp.celsius_rc_out[0][0] = 0x00c00;
					exp.celsius_rc_out[1][0] = 0x00c00;
				}
			}
			insrt(exp.valid[1], 19, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_config_d3d56(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 8, 4, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 15, 1, 0);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 16, 4, 1);
			if (jrand48(ctx->rand48) & 3)
				insrt(val, 25, 5, 0);
		}
		uint32_t cls, mthd;
		int trapbit;
		bool is_d3d6;
		switch (nrand48(ctx->rand48) % 2) {
			default:
				cls = get_random_d3d5(ctx);
				mthd = 0x314;
				trapbit = 10;
				is_d3d6 = false;
				break;
			case 1:
				cls = get_random_d3d6(ctx);
				mthd = 0x33c;
				trapbit = 17;
				is_d3d6 = true;
				break;
		}
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, trapbit);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			if (extr(val, 8, 4) < 1 || extr(val, 8, 4) > 8)
				bad = true;
			if (extr(val, 15, 1))
				bad = true;
			if (extr(val, 16, 4) < 1 || extr(val, 16, 4) > 8)
				bad = true;
			if (extr(val, 20, 2) < 1)
				bad = true;
			if (extr(val, 25, 5) && !is_d3d6)
				bad = true;
			if (extr(val, 30, 2) < 1 || extr(val, 30, 2) > 2)
				bad = true;
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				uint32_t rval = val & 0x3fcf5fff;
				insrt(rval, 16, 4, extr(val, 16, 4) - 1);
				insrt(rval, 8, 4, extr(val, 8, 4) - 1);
				if (!is_d3d6)
					insrt(rval, 25, 5, 0x1e);
				int scull = extr(val, 20, 2);
				insrt(exp.celsius_config_a, 0, 30, rval);
				insrt(exp.celsius_unke88, 0, 21, 0);
				insrt(exp.celsius_unke88, 21, 2, scull == 2 ? 1 : 2);
				insrt(exp.celsius_unke88, 23, 5, 8);
				insrt(exp.celsius_unke88, 28, 2, scull != 1);
				insrt(exp.celsius_unke88, 29, 1, extr(val, 31, 1));
				insrt(exp.celsius_unkf40, 28, 1, 1);
				insrt(exp.celsius_unkf40, 29, 1, !extr(val, 13, 1));
				insrt(exp.celsius_unkf44, 0, 1, 0);
				insrt(exp.celsius_unkf44, 14, 1, 0);
				if (!is_d3d6) {
					exp.celsius_stencil_func = 0x70;
				}
			}
			insrt(exp.valid[1], 17, 1, 1);
			if (!is_d3d6)
				insrt(exp.valid[1], 18, 1, 1);
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_stencil_func_d3d6(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t cls = get_random_d3d6(ctx), mthd = 0x340;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 18);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			if (extr(val, 1, 3))
				bad = true;
			if (extr(val, 4, 4) < 1 || extr(val, 4, 4) > 8)
				bad = true;
			insrt(exp.valid[1], 18, 1, 1);
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				uint32_t rval = val & 0xfffffff1;
				insrt(rval, 4, 4, extr(val, 4, 4) - 1);
				exp.celsius_stencil_func = rval;
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_celsius_stencil_op_d3d6(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= ~0xfffff000;
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
			if (jrand48(ctx->rand48) & 1) {
				val |= 1 << (jrand48(ctx->rand48) & 0x1f);
			}
		}
		uint32_t cls = get_random_d3d6(ctx), mthd = 0x344;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		struct pgraph_state orig, exp, real;
		nv04_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		uint32_t grobj[4];
		nv04_pgraph_prep_mthd(ctx, grobj, &orig, cls, addr, val);
		nv04_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv04_pgraph_mthd(&exp, grobj, 19);
		if (!extr(exp.intr, 4, 1)) {
			bool bad = false;
			if (extr(val, 0, 4) < 1 || extr(val, 0, 4) > 8)
				bad = true;
			if (extr(val, 4, 4) < 1 || extr(val, 4, 4) > 8)
				bad = true;
			if (extr(val, 8, 4) < 1 || extr(val, 8, 4) > 8)
				bad = true;
			if (extr(val, 12, 20))
				bad = true;
			insrt(exp.valid[1], 20, 1, 1);
			if (extr(exp.debug[3], 20, 1) && bad)
				nv04_pgraph_blowup(&exp, 2);
			if (!exp.intr) {
				exp.celsius_stencil_op = val & 0xfff;
			}
		}
		nv04_pgraph_dump_state(ctx, &real);
		if (nv04_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d mthd %02x.%04x %08x\n", i, cls, addr, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int invalid_mthd_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int xy_mthd_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int simple_mthd_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int d3d56_mthd_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type != 4)
		return HWTEST_RES_NA;
	return HWTEST_RES_PASS;
}

static int celsius_mthd_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type != 0x10)
		return HWTEST_RES_NA;
	return HWTEST_RES_PASS;
}

static int nv04_pgraph_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.card_type < 0x04 || ctx->chipset.card_type > 0x10)
		return HWTEST_RES_NA;
	if (!(nva_rd32(ctx->cnum, 0x200) & 1 << 24)) {
		printf("Mem controller not up.\n");
		return HWTEST_RES_UNPREP;
	}
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	return HWTEST_RES_PASS;
}

namespace {

HWTEST_DEF_GROUP(invalid_mthd,
	HWTEST_TEST(test_invalid_class, 0),
	HWTEST_TEST(test_invalid_mthd_op, 0),
	HWTEST_TEST(test_invalid_mthd_ctx, 0),
	HWTEST_TEST(test_invalid_mthd_surf, 0),
	HWTEST_TEST(test_invalid_mthd_dvd, 0),
	HWTEST_TEST(test_invalid_mthd_m2mf, 0),
	HWTEST_TEST(test_invalid_mthd_lin, 0),
	HWTEST_TEST(test_invalid_mthd_tri, 0),
	HWTEST_TEST(test_invalid_mthd_rect, 0),
	HWTEST_TEST(test_invalid_mthd_blit, 0),
	HWTEST_TEST(test_invalid_mthd_ifc, 0),
	HWTEST_TEST(test_invalid_mthd_sifc, 0),
	HWTEST_TEST(test_invalid_mthd_iifc, 0),
	HWTEST_TEST(test_invalid_mthd_sifm, 0),
	HWTEST_TEST(test_invalid_mthd_gdi_nv3, 0),
	HWTEST_TEST(test_invalid_mthd_gdi_nv4, 0),
	HWTEST_TEST(test_invalid_mthd_tfc, 0),
	HWTEST_TEST(test_invalid_mthd_d3d0, 0),
	HWTEST_TEST(test_invalid_mthd_d3d5, 0),
	HWTEST_TEST(test_invalid_mthd_d3d6, 0),
	HWTEST_TEST(test_invalid_mthd_celsius, 0),
)

HWTEST_DEF_GROUP(simple_mthd,
	HWTEST_TEST(test_mthd_ctxsw, 0),
	HWTEST_TEST(test_mthd_nop, 0),
	HWTEST_TEST(test_mthd_notify, 0),
	HWTEST_TEST(test_mthd_sync, 0),
	HWTEST_TEST(test_mthd_pm_trigger, 0),
	HWTEST_TEST(test_mthd_missing, 0),
	HWTEST_TEST(test_mthd_beta, 0),
	HWTEST_TEST(test_mthd_beta4, 0),
	HWTEST_TEST(test_mthd_rop, 0),
	HWTEST_TEST(test_mthd_chroma_nv1, 0),
	HWTEST_TEST(test_mthd_chroma_nv4, 0),
	HWTEST_TEST(test_mthd_pattern_shape, 0),
	HWTEST_TEST(test_mthd_pattern_select, 0),
	HWTEST_TEST(test_mthd_pattern_mono_color_nv1, 0),
	HWTEST_TEST(test_mthd_pattern_mono_color_nv4, 0),
	HWTEST_TEST(test_mthd_pattern_mono_bitmap, 0),
	HWTEST_TEST(test_mthd_pattern_color_y8, 0),
	HWTEST_TEST(test_mthd_pattern_color_r5g6b5, 0),
	HWTEST_TEST(test_mthd_pattern_color_r5g5b5, 0),
	HWTEST_TEST(test_mthd_pattern_color_r8g8b8, 0),
	HWTEST_TEST(test_mthd_surf_offset, 0),
	HWTEST_TEST(test_mthd_surf_pitch, 0),
	HWTEST_TEST(test_mthd_surf_pitch_2, 0),
	HWTEST_TEST(test_mthd_surf_nv3_format, 0),
	HWTEST_TEST(test_mthd_surf_2d_format, 0),
	HWTEST_TEST(test_mthd_surf_swz_format, 0),
	HWTEST_TEST(test_mthd_surf_dvd_format, 0),
	HWTEST_TEST(test_mthd_surf_3d_format, 0),
	HWTEST_TEST(test_mthd_dma_surf, 0),
	HWTEST_TEST(test_mthd_clip, 0),
	HWTEST_TEST(test_mthd_clip_zero_size, 0),
	HWTEST_TEST(test_mthd_clip_hv, 0),
	HWTEST_TEST(test_mthd_solid_color, 0),
	HWTEST_TEST(test_mthd_sifm_pitch, 0),
	HWTEST_TEST(test_mthd_dvd_format, 0),
	HWTEST_TEST(test_mthd_index_format, 0),
	HWTEST_TEST(test_mthd_dma_offset, 0),
	HWTEST_TEST(test_mthd_m2mf_pitch, 0),
	HWTEST_TEST(test_mthd_m2mf_line_length, 0),
	HWTEST_TEST(test_mthd_m2mf_line_count, 0),
	HWTEST_TEST(test_mthd_m2mf_format, 0),
	HWTEST_TEST(test_mthd_font, 0),
	HWTEST_TEST(test_mthd_ctx_format, 0),
	HWTEST_TEST(test_mthd_solid_format, 0),
	HWTEST_TEST(test_mthd_ifc_format, 0),
	HWTEST_TEST(test_mthd_sifm_src_format, 0),
	HWTEST_TEST(test_mthd_mono_format, 0),
	HWTEST_TEST(test_mthd_dma_grobj, 0),
	HWTEST_TEST(test_mthd_dma_celsius, 0),
	HWTEST_TEST(test_mthd_ctx_clip, 0),
	HWTEST_TEST(test_mthd_ctx_chroma, 0),
	HWTEST_TEST(test_mthd_ctx_rop, 0),
	HWTEST_TEST(test_mthd_ctx_surf_nv3, 0),
	HWTEST_TEST(test_mthd_ctx_surf_nv4, 0),
	HWTEST_TEST(test_mthd_operation, 0),
	HWTEST_TEST(test_mthd_dither, 0),
	HWTEST_TEST(test_mthd_patch, 0),
	HWTEST_TEST(test_mthd_flip_set, 0),
	HWTEST_TEST(test_mthd_flip_inc_write, 0),
)

HWTEST_DEF_GROUP(xy_mthd,
	HWTEST_TEST(test_mthd_xy, 0),
	HWTEST_TEST(test_mthd_x32, 0),
	HWTEST_TEST(test_mthd_y32, 0),
	HWTEST_TEST(test_mthd_ifc_size, 0),
)

HWTEST_DEF_GROUP(d3d56_mthd,
	HWTEST_TEST(test_mthd_d3d_tex_color_key, 0),
	HWTEST_TEST(test_mthd_d3d_tex_offset, 0),
	HWTEST_TEST(test_mthd_d3d_tex_format_d3d0, 0),
	HWTEST_TEST(test_mthd_d3d_tex_format, 0),
	HWTEST_TEST(test_mthd_d3d_tex_filter_d3d0, 0),
	HWTEST_TEST(test_mthd_d3d_tex_filter, 0),
	HWTEST_TEST(test_mthd_d3d_blend, 0),
	HWTEST_TEST(test_mthd_d3d_config, 0),
	HWTEST_TEST(test_mthd_d3d_config_d3d0, 0),
	HWTEST_TEST(test_mthd_d3d_alpha_d3d0, 0),
	HWTEST_TEST(test_mthd_d3d_stencil_func, 0),
	HWTEST_TEST(test_mthd_d3d_stencil_op, 0),
	HWTEST_TEST(test_mthd_d3d_fog_color, 0),
	HWTEST_TEST(test_mthd_d3d_rc_cw, 0),
	HWTEST_TEST(test_mthd_d3d_rc_factor, 0),
	HWTEST_TEST(test_mthd_d3d_tlv_fog_tri_col1, 0),
	HWTEST_TEST(test_mthd_d3d_tlv_color, 0),
	HWTEST_TEST(test_mthd_d3d_tlv_xy, 0),
	HWTEST_TEST(test_mthd_d3d_tlv_z, 0),
	HWTEST_TEST(test_mthd_d3d_tlv_rhw, 0),
	HWTEST_TEST(test_mthd_d3d_tlv_uv, 0),
)

HWTEST_DEF_GROUP(celsius_mthd,
	HWTEST_TEST(test_mthd_celsius_tex_offset, 0),
	HWTEST_TEST(test_mthd_celsius_tex_format, 0),
	HWTEST_TEST(test_mthd_celsius_tex_format_d3d56, 0),
	HWTEST_TEST(test_mthd_celsius_tex_format_d3d0, 0),
	HWTEST_TEST(test_mthd_celsius_tex_control, 0),
	HWTEST_TEST(test_mthd_celsius_tex_pitch, 0),
	HWTEST_TEST(test_mthd_celsius_tex_unk238, 0),
	HWTEST_TEST(test_mthd_celsius_tex_rect, 0),
	HWTEST_TEST(test_mthd_celsius_tex_filter, 0),
	HWTEST_TEST(test_mthd_celsius_tex_filter_d3d56, 0),
	HWTEST_TEST(test_mthd_celsius_tex_filter_d3d0, 0),
	HWTEST_TEST(test_mthd_celsius_tex_palette, 0),
	HWTEST_TEST(test_mthd_celsius_rc_in, 0),
	HWTEST_TEST(test_mthd_celsius_rc_factor, 0),
	HWTEST_TEST(test_mthd_celsius_rc_out, 0),
	HWTEST_TEST(test_mthd_celsius_rc_final, 0),
	HWTEST_TEST(test_mthd_celsius_rc_d3d6, 0),
	HWTEST_TEST(test_mthd_celsius_unk290, 0),
	HWTEST_TEST(test_mthd_celsius_light_model, 0),
	HWTEST_TEST(test_mthd_celsius_fog_color_d3d, 0),
	HWTEST_TEST(test_mthd_celsius_config_d3d0, 0),
	HWTEST_TEST(test_mthd_celsius_alpha_d3d0, 0),
	HWTEST_TEST(test_mthd_celsius_tex_color_key_d3d5, 0),
	HWTEST_TEST(test_mthd_celsius_blend_d3d56, 0),
	HWTEST_TEST(test_mthd_celsius_config_d3d56, 0),
	HWTEST_TEST(test_mthd_celsius_stencil_func_d3d6, 0),
	HWTEST_TEST(test_mthd_celsius_stencil_op_d3d6, 0),
)

}

HWTEST_DEF_GROUP(nv04_pgraph,
	HWTEST_GROUP(invalid_mthd),
	HWTEST_GROUP(simple_mthd),
	HWTEST_GROUP(xy_mthd),
	HWTEST_GROUP(d3d56_mthd),
	HWTEST_GROUP(celsius_mthd),
)
