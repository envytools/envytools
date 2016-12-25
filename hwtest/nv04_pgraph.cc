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

static uint32_t get_random_celsius(struct hwtest_ctx *ctx) {
	uint32_t classes[] = {0x56, 0x96, 0x98, 0x99};
	if (!nv04_pgraph_is_nv15p(&ctx->chipset))
		return 0x56;
	else if (!nv04_pgraph_is_nv17p(&ctx->chipset))
		return classes[jrand48(ctx->rand48) & 1];
	else
		return classes[jrand48(ctx->rand48) & 3];
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
		int trapbit;
		uint32_t mask;
		idx = jrand48(ctx->rand48) & 1;
		cls = get_random_celsius(ctx);
		mthd = 0x218 + idx * 4;
		trapbit = 15;
		mask = 0x7f;
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
			if (!exp.intr)
				exp.celsius_tex_offset[idx] = val;
			insrt(exp.valid[1], idx ? 22 : 14, 1, 1);
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
		idx = jrand48(ctx->rand48) & 1;
		cls = get_random_celsius(ctx);
		mthd = 0x270 + idx * 4;
		trapbit = 25;
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
				exp.celsius_rc_factor[idx] = val;
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

static int invalid_mthd_prep(struct hwtest_ctx *ctx) {
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
	HWTEST_TEST(test_invalid_mthd_celsius, 0),
)

HWTEST_DEF_GROUP(celsius_mthd,
	HWTEST_TEST(test_mthd_celsius_tex_offset, 0),
	HWTEST_TEST(test_mthd_celsius_tex_format, 0),
	HWTEST_TEST(test_mthd_celsius_tex_control, 0),
	HWTEST_TEST(test_mthd_celsius_tex_pitch, 0),
	HWTEST_TEST(test_mthd_celsius_tex_unk238, 0),
	HWTEST_TEST(test_mthd_celsius_tex_rect, 0),
	HWTEST_TEST(test_mthd_celsius_tex_filter, 0),
	HWTEST_TEST(test_mthd_celsius_tex_palette, 0),
	HWTEST_TEST(test_mthd_celsius_rc_in, 0),
	HWTEST_TEST(test_mthd_celsius_rc_factor, 0),
	HWTEST_TEST(test_mthd_celsius_rc_out, 0),
	HWTEST_TEST(test_mthd_celsius_rc_final, 0),
	HWTEST_TEST(test_mthd_celsius_unk290, 0),
	HWTEST_TEST(test_mthd_celsius_light_model, 0),
)

}

HWTEST_DEF_GROUP(nv04_pgraph,
	HWTEST_GROUP(invalid_mthd),
	HWTEST_GROUP(celsius_mthd),
)
