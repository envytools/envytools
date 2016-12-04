/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "hwtest.h"
#include "old.h"
#include "pgraph.h"
#include "nvhw/pgraph.h"
#include "nvhw/chipset.h"
#include "nva.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void nv03_pgraph_gen_state(struct hwtest_ctx *ctx, struct pgraph_state *state) {
	std::mt19937 rnd(jrand48(ctx->rand48));
	nv01_pgraph_gen_state(ctx->cnum, rnd, state);
}

static void nv03_pgraph_load_state(struct hwtest_ctx *ctx, struct pgraph_state *state) {
	nv01_pgraph_load_state(ctx->cnum, state);
}

static void nv03_pgraph_dump_state(struct hwtest_ctx *ctx, struct pgraph_state *state) {
	nv01_pgraph_dump_state(ctx->cnum, state);
}

static void nv03_pgraph_prep_mthd(struct pgraph_state *state, uint32_t *pgctx, int cls, uint32_t addr) {
	state->fifo_enable = 1;
	state->ctx_control = 0x10010003;
	state->ctx_user = (state->ctx_user & 0xffffff) | (*pgctx & 0x7f000000);
	int old_subc = extr(state->ctx_user, 13, 3);
	int new_subc = extr(addr, 13, 3);
	if (old_subc != new_subc) {
		*pgctx &= ~0x1f0000;
		*pgctx |= cls << 16;
	} else {
		state->ctx_user &= ~0x1f0000;
		state->ctx_user |= cls << 16;
	}
	if (extr(state->debug[1], 16, 1) || extr(addr, 0, 13) == 0) {
		*pgctx &= ~0xf000;
		*pgctx |= 0x1000;
	}
}

static void nv03_pgraph_mthd(struct hwtest_ctx *ctx, struct pgraph_state *state, uint32_t *grobj, uint32_t gctx, uint32_t addr, uint32_t val) {
	if (extr(state->debug[1], 16, 1) || extr(addr, 0, 13) == 0) {
		uint32_t inst = gctx & 0xffff;
		nva_gwr32(nva_cards[ctx->cnum]->bar1, 0xc00000 + inst * 0x10, grobj[0]);
		nva_gwr32(nva_cards[ctx->cnum]->bar1, 0xc00004 + inst * 0x10, grobj[1]);
		nva_gwr32(nva_cards[ctx->cnum]->bar1, 0xc00008 + inst * 0x10, grobj[2]);
		nva_gwr32(nva_cards[ctx->cnum]->bar1, 0xc0000c + inst * 0x10, grobj[3]);
	}
	if ((addr & 0x1ffc) == 0) {
		int i;
		for (i = 0; i < 0x200; i++) {
			nva_gwr32(nva_cards[ctx->cnum]->bar1, 0xc00000 + i * 8, val);
			nva_gwr32(nva_cards[ctx->cnum]->bar1, 0xc00004 + i * 8, gctx | 1 << 23);
		}
	}
	nva_wr32(ctx->cnum, 0x2100, 0xffffffff);
	nva_wr32(ctx->cnum, 0x2140, 0xffffffff);
	nva_wr32(ctx->cnum, 0x2210, 0);
	nva_wr32(ctx->cnum, 0x2214, 0x1000);
	nva_wr32(ctx->cnum, 0x2218, 0x12000);
	nva_wr32(ctx->cnum, 0x2410, 0);
	nva_wr32(ctx->cnum, 0x2420, 0);
	nva_wr32(ctx->cnum, 0x3000, 0);
	nva_wr32(ctx->cnum, 0x3040, 0);
	nva_wr32(ctx->cnum, 0x3004, gctx >> 24);
	nva_wr32(ctx->cnum, 0x3010, 4);
	nva_wr32(ctx->cnum, 0x3070, 0);
	nva_wr32(ctx->cnum, 0x3080, (addr & 0x1ffc) == 0 ? 0xdeadbeef : gctx | 1 << 23);
	nva_wr32(ctx->cnum, 0x3100, addr);
	nva_wr32(ctx->cnum, 0x3104, val);
	nva_wr32(ctx->cnum, 0x3000, 1);
	nva_wr32(ctx->cnum, 0x3040, 1);
	int old_subc = extr(state->ctx_user, 13, 3);
	int new_subc = extr(addr, 13, 3);
	if (old_subc != new_subc || extr(addr, 0, 13) == 0) {
		if (extr(addr, 0, 13) == 0)
			state->ctx_cache[addr >> 13 & 7][0] = grobj[0] & 0x3ff3f71f;
		state->ctx_user &= ~0x1fe000;
		state->ctx_user |= gctx & 0x1f0000;
		state->ctx_user |= addr & 0xe000;
		if (extr(state->debug[1], 20, 1))
			state->ctx_switch[0] = state->ctx_cache[addr >> 13 & 7][0];
		if (extr(state->debug[2], 28, 1)) {
			pgraph_volatile_reset(state);
			insrt(state->debug[1], 0, 1, 1);
		} else {
			insrt(state->debug[1], 0, 1, 0);
		}
		if (extr(state->notify, 16, 1)) {
			state->intr |= 1;
			state->invalid |= 0x10000;
			state->fifo_enable = 0;
		}
	}
	if (!state->invalid && extr(state->debug[3], 20, 2) == 3 && extr(addr, 0, 13)) {
		state->invalid |= 0x10;
		state->intr |= 0x1;
		state->fifo_enable = 0;
	}
	int ncls = extr(gctx, 16, 5);
	if (extr(state->debug[1], 16, 1) && (gctx & 0xffff) != state->ctx_switch[3] &&
		(ncls == 0x0d || ncls == 0x0e || ncls == 0x14 || ncls == 0x17 ||
		 extr(addr, 0, 13) == 0x104)) {
		state->ctx_switch[3] = gctx & 0xffff;
		state->ctx_switch[1] = grobj[1] & 0xffff;
		state->notify &= 0xf10000;
		state->notify |= grobj[1] >> 16 & 0xffff;
		state->ctx_switch[2] = grobj[2] & 0x1ffff;
	}
}

static int test_mthd_ctxsw(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int cls = jrand48(ctx->rand48) & 0x1f;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000);
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		nv03_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_notify(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int classes[22] = {
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
			0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
			0x0d, 0x0e, 0x10, 0x11, 0x12, 0x14,
			0x15, 0x17, 0x18, 0x1c,
		};
		int cls = classes[nrand48(ctx->rand48) % 22];
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1)
			val &= 0x3f;
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | 0x104;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		if (!extr(exp.invalid, 16, 1)) {
			if (val & ~0xf && extr(exp.debug[3], 20, 1)) {
				exp.intr |= 1;
				exp.invalid |= 0x10;
				exp.fifo_enable = 0;
			}
			if (extr(exp.notify, 16, 1)) {
				exp.intr |= 1;
				exp.invalid |= 0x1000;
				exp.fifo_enable = 0;
			}
		}
		if (!extr(exp.invalid, 16, 1) && !extr(exp.invalid, 4, 1)) {
			insrt(exp.notify, 16, 1, 1);
			insrt(exp.notify, 20, 4, val);
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int check_mthd_invalid(struct hwtest_ctx *ctx, int cls, int mthd) {
	int i;
	for (i = 0; i < 4; i++) {
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = (jrand48(ctx->rand48) & 0xe000) | mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		if (!extr(exp.invalid, 16, 1)) {
			exp.intr |= 1;
			exp.invalid |= 1;
			exp.fifo_enable = 0;
			if (!extr(exp.invalid, 4, 1)) {
				if (extr(exp.notify, 16, 1) && extr(exp.notify, 20, 4)) {
					exp.intr |= 1 << 28;
				}
			}
		}
		nv03_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return 0;
}

static int test_mthd_invalid(struct hwtest_ctx *ctx) {
	int cls;
	int res = 0;
	for (cls = 0; cls < 0x20; cls++) {
		int mthd;
		for (mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0) /* CTX_SWITCH */
				continue;
			if (((cls >= 1 && cls <= 0xe) || (cls >= 0x10 && cls <= 0x12) ||
			     cls >= 0x14 || cls == 0x15 || cls == 0x17 || cls == 0x18 ||
			     cls == 0x1c) && mthd == 0x104) /* NOTIFY */
				continue;
			if ((cls == 1 || cls == 2) && mthd == 0x300) /* ROP, BETA */
				continue;
			if ((cls == 3 || cls == 4) && mthd == 0x304) /* CHROMA, PLANE */
				continue;
			if (cls == 5 && (mthd == 0x300 || mthd == 0x304)) /* CLIP */
				continue;
			if (cls == 6 && (mthd == 0x308 || (mthd >= 0x310 && mthd <= 0x31c))) /* PATTERN */
				continue;
			if (cls >= 7 && cls <= 0xb && mthd == 0x304) /* COLOR */
				continue;
			if (cls == 7 && mthd >= 0x400 && mthd < 0x480) /* RECT */
				continue;
			if (cls == 8 && mthd >= 0x400 && mthd < 0x580) /* POINT */
				continue;
			if (cls >= 9 && cls <= 0xa && mthd >= 0x400 && mthd < 0x680) /* LINE/LIN */
				continue;
			if (cls == 0xb && mthd >= 0x310 && mthd < 0x31c) /* TRI.TRIANGLE */
				continue;
			if (cls == 0xb && mthd >= 0x320 && mthd < 0x338) /* TRI.TRIANGLE32 */
				continue;
			if (cls == 0xb && mthd >= 0x400 && mthd < 0x600) /* TRI.TRIMESH, TRI.TRIMESH32, TRI.CTRIANGLE, TRI.CTRIMESH */
				continue;
			if (cls == 0xc && mthd >= 0x3fc && mthd < 0x600) /* GDIRECT.RECT_NCLIP */
				continue;
			if (cls == 0xc && mthd >= 0x7f4 && mthd < 0xa00) /* GDIRECT.RECT_CLIP */
				continue;
			if (cls == 0xc && mthd >= 0xbec && mthd < 0xe00) /* GDIRECT.BITMAP_1COLOR */
				continue;
			if (cls == 0xc && mthd >= 0xfe8 && mthd < 0x1200) /* GDIRECT.BITMAP_1COLOR_ICLIP */
				continue;
			if (cls == 0xc && mthd >= 0x13e4 && mthd < 0x1600) /* GDIRECT.BITMAP_2COLOR_ICLIP */
				continue;
			if (cls == 0xd && mthd >= 0x30c && mthd < 0x32c) /* M2MF */
				continue;
			if (cls == 0xe && mthd >= 0x308 && mthd < 0x320) /* SIFM */
				continue;
			if (cls == 0xe && mthd >= 0x400 && mthd < 0x418) /* SIFM */
				continue;
			if (cls == 0x10 && mthd >= 0x300 && mthd < 0x30c) /* BLIT */
				continue;
			if (cls == 0x11 && mthd >= 0x304 && mthd < 0x310) /* IFC setup */
				continue;
			if (cls == 0x12 && mthd >= 0x308 && mthd < 0x31c) /* BITMAP setup */
				continue;
			if (cls >= 0x11 && cls <= 0x12 && mthd >= 0x400 && mthd < 0x480) /* IFC/BITMAP data */
				continue;
			if (cls == 0x14 && mthd >= 0x308 && mthd < 0x318) /* ITM */
				continue;
			if (cls == 0x15 && mthd >= 0x304 && mthd < 0x31c) /* SIFC */
				continue;
			if (cls == 0x15 && mthd >= 0x400 && mthd < 0x2000) /* SIFC data */
				continue;
			if (cls == 0x17 && mthd >= 0x304 && mthd < 0x31c) /* D3D */
				continue;
			if (cls == 0x17 && mthd >= 0x1000 && mthd < 0x2000) /* D3D */
				continue;
			if (cls == 0x18 && mthd >= 0x304 && mthd < 0x30c) /* ZPOINT */
				continue;
			if (cls == 0x18 && mthd >= 0x7fc && mthd < 0x1000) /* ZPOINT */
				continue;
			if (cls == 0x1c && (mthd == 0x300 || mthd == 0x308 || mthd == 0x30c)) /* SURF */
				continue;
			res |= check_mthd_invalid(ctx, cls, mthd);
		}
	}
	return res ? HWTEST_RES_FAIL : HWTEST_RES_PASS;
}

static int test_rop_simple(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int idx = jrand48(ctx->rand48) & 0x1f;
		int cls = 0x8;
		uint32_t mthd = 0x0400 | idx << 2;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		uint32_t paddr[4], pixel[4], epixel[4], rpixel[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000100;
		orig.ctx_user &= ~0xe000;
		orig.ctx_switch[0] &= ~0x8000;
		int op = extr(orig.ctx_switch[0], 24, 5);
		if (!((op >= 0x00 && op <= 0x15) || op == 0x17 || (op >= 0x19 && op <= 0x1a) || op == 0x1d))
			op = 0x17;
		insrt(orig.ctx_switch[0], 24, 5, op);
		orig.pattern_config = nrand48(ctx->rand48)%3; /* shape 3 is a rather ugly hole in Karnough map */
		insrt(orig.cliprect_ctrl, 8, 1, 0);
		orig.xy_misc_0 = 0;
		orig.xy_misc_1[0] = 0;
		orig.xy_misc_1[1] = 0;
		orig.xy_misc_3 = 0;
		orig.xy_misc_4[0] = 0;
		orig.xy_misc_4[1] = 0;
		orig.valid[0] = 0x10000;
		orig.surf_offset[0] = 0x000000;
		orig.surf_offset[1] = 0x040000;
		orig.surf_offset[2] = 0x080000;
		orig.surf_offset[3] = 0x0c0000;
		orig.surf_pitch[0] = 0x0400;
		orig.surf_pitch[1] = 0x0400;
		orig.surf_pitch[2] = 0x0400;
		orig.surf_pitch[3] = 0x0400;
		if (op > 0x17)
			orig.surf_format |= 0x2222;
		orig.debug[3] &= ~(1 << 22);
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_min[jrand48(ctx->rand48)&1], 0, 16, extr(val, 0, 8));
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_min[jrand48(ctx->rand48)&1], 16, 16, extr(val, 16, 8));
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_max[jrand48(ctx->rand48)&1], 0, 16, extr(val, 0, 8));
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_max[jrand48(ctx->rand48)&1], 16, 16, extr(val, 16, 8));
		if (jrand48(ctx->rand48)&1) {
			/* it's vanishingly rare for the chroma key to match perfectly by random, so boost the odds */
			uint32_t ckey;
			if ((nv03_pgraph_surf_format(&orig) & 3) == 0 && extr(orig.ctx_switch[0], 0, 3) == 4) {
				uint32_t save_ctxsw = orig.ctx_switch[0];
				orig.ctx_switch[0] &= ~7;
				ckey = pgraph_to_a1r10g10b10(pgraph_expand_color(&orig, orig.misc32[0]));
				orig.ctx_switch[0] = save_ctxsw;
			} else {
				ckey = pgraph_to_a1r10g10b10(pgraph_expand_color(&orig, orig.misc32[0]));
			}
			ckey ^= (jrand48(ctx->rand48) & 1) << 30; /* perturb alpha */
			if (jrand48(ctx->rand48)&1) {
				/* perturb it a bit to check which bits have to match */
				ckey ^= 1 << (nrand48(ctx->rand48) % 30);
			}
			orig.chroma = ckey;
		}
		val &= 0x00ff00ff;
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		int cpp = nv03_pgraph_cpp(&orig);
		int x = extr(val, 0, 16);
		int y = extr(val, 16, 16);
		for (int j = 0; j < 4; j++) {
			paddr[j] = (x * cpp + y * 0x400 + j * 0x40000);
			pixel[j] = epixel[j] = jrand48(ctx->rand48);
			nva_gwr32(nva_cards[ctx->cnum]->bar1, paddr[j] & ~3, pixel[j]);
		}
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.vtx_x[0] = x;
		exp.vtx_y[0] = y;
		insrt(exp.xy_misc_0, 28, 4, 1);
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		int xcstat = nv03_pgraph_clip_status(&exp, exp.vtx_x[0], 0, false);
		int ycstat = nv03_pgraph_clip_status(&exp, exp.vtx_y[0], 1, false);
		insrt(exp.xy_clip[0][0], 0, 4, xcstat);
		insrt(exp.xy_clip[0][0], 4, 4, xcstat);
		insrt(exp.xy_clip[1][0], 0, 4, ycstat);
		insrt(exp.xy_clip[1][0], 4, 4, ycstat);
		if (pgraph_cliprect_pass(&exp, x, y)) {
			for (int j = 0; j < 4; j++) {
				if (extr(exp.ctx_switch[0], 20 + j, 1)) {
					uint32_t src = extr(pixel[j], (paddr[j] & 3) * 8, cpp * 8);
					uint32_t res = nv03_pgraph_solid_rop(&exp, x, y, src);
					insrt(epixel[j], (paddr[j] & 3) * 8, cpp * 8, res);
				}
			}
		}
		nv03_pgraph_dump_state(ctx, &real);
		bool mismatch = false;
		for (int j = 0; j < 4; j++) {
			rpixel[j] = nva_grd32(nva_cards[ctx->cnum]->bar1, paddr[j] & ~3);
			if (rpixel[j] != epixel[j])
			mismatch = true;
		}
		if (nv01_pgraph_cmp_state(&orig, &exp, &real, mismatch)) {
			for (int j = 0; j < 4; j++) {
				printf("pixel%d: %08x %08x %08x%s\n", j, pixel[j], epixel[j], rpixel[j], epixel[j] == rpixel[j] ? "" : " *");
			}
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_rop_zpoint(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int idx = jrand48(ctx->rand48) & 0xff;
		int cls = 0x18;
		uint32_t mthd = 0x0804 | idx << 3;
		uint32_t val = jrand48(ctx->rand48);
		uint32_t addr = mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		uint32_t paddr[4], pixel[4], epixel[4], rpixel[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000100;
		orig.ctx_user &= ~0xe000;
		orig.ctx_switch[0] &= ~0x8000;
		insrt(orig.cliprect_ctrl, 8, 1, 0);
		orig.xy_misc_0 = 0;
		orig.xy_misc_1[0] = 0;
		orig.xy_misc_1[1] = 0;
		orig.xy_misc_3 = 0;
		orig.xy_misc_4[0] = 0;
		orig.xy_misc_4[1] = 0;
		orig.xy_clip[0][0] = 0;
		orig.xy_clip[1][0] = 0;
		orig.valid[0] = 0x4210101;
		orig.surf_offset[0] = 0x000000;
		orig.surf_offset[1] = 0x040000;
		orig.surf_offset[2] = 0x080000;
		orig.surf_offset[3] = 0x0c0000;
		orig.surf_pitch[0] = 0x0400;
		orig.surf_pitch[1] = 0x0400;
		orig.surf_pitch[2] = 0x0400;
		orig.surf_pitch[3] = 0x0400;
		orig.surf_format = 0x6666;
		if (extr(orig.d3d_config, 20, 3) > 4)
			insrt(orig.d3d_config, 22, 1, 0);
		orig.debug[2] &= 0xffdfffff;
		orig.debug[3] &= 0xfffeffff;
		orig.debug[3] &= 0xfffdffff;
		int x = jrand48(ctx->rand48) & 0xff;
		int y = jrand48(ctx->rand48) & 0xff;
		orig.vtx_x[0] = x;
		orig.vtx_y[0] = y;
		orig.debug[3] &= ~(1 << 22);
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_min[jrand48(ctx->rand48)&1], 0, 16, extr(val, 0, 8));
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_min[jrand48(ctx->rand48)&1], 16, 16, extr(val, 16, 8));
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_max[jrand48(ctx->rand48)&1], 0, 16, extr(val, 0, 8));
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_max[jrand48(ctx->rand48)&1], 16, 16, extr(val, 16, 8));
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		int cpp = nv03_pgraph_cpp(&orig);
		for (int j = 0; j < 4; j++) {
			paddr[j] = (x * cpp + y * 0x400 + j * 0x40000);
			pixel[j] = epixel[j] = jrand48(ctx->rand48);
			nva_gwr32(nva_cards[ctx->cnum]->bar1, paddr[j] & ~3, pixel[j]);
		}
		uint32_t zcur = extr(pixel[3], (paddr[3] & 3) * 8, cpp * 8);
		if (!(jrand48(ctx->rand48) & 3)) {
			insrt(val, 16, 16, zcur);
		}
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		insrt(exp.misc32[1], 16, 16, extr(val, 16, 16));
		nv03_pgraph_vtx_add(&exp, 0, 0, exp.vtx_x[0], 1, 0, false);
		uint32_t zeta = extr(val, 16, 16);
		struct pgraph_color s = pgraph_expand_color(&exp, exp.misc32[0]);
		uint8_t ra = nv01_pgraph_dither_10to5(s.a << 2, x, y, false) >> 1;
		if (pgraph_cliprect_pass(&exp, x, y)) {
			bool zeta_test = nv03_pgraph_d3d_cmp(extr(exp.d3d_config, 16, 4), zeta, zcur);
			if (!extr(exp.ctx_switch[0], 12, 1))
				zeta_test = true;
			bool alpha_test = nv03_pgraph_d3d_cmp(extr(exp.d3d_alpha, 8, 4), extr(exp.misc32[0], 24, 8), extr(exp.d3d_alpha, 0, 8));
			bool alpha_nz = !!ra;
			bool zeta_wr = nv03_pgraph_d3d_wren(extr(exp.d3d_config, 20, 3), zeta_test, alpha_nz);
			bool color_wr = nv03_pgraph_d3d_wren(extr(exp.d3d_config, 24, 3), zeta_test, alpha_nz);
			if (!alpha_test) {
				color_wr = false;
				zeta_wr = false;
			}
			if (extr(exp.ctx_switch[0], 12, 1) && extr(exp.ctx_switch[0], 20, 4) && zeta_wr) {
				insrt(epixel[3], (paddr[3] & 3) * 8, cpp * 8, zeta);
			}
			for (int j = 0; j < 4; j++) {
				if (extr(exp.ctx_switch[0], 20 + j, 1) && color_wr) {
					uint32_t src = extr(pixel[j], (paddr[j] & 3) * 8, cpp * 8);
					uint32_t res = nv03_pgraph_zpoint_rop(&exp, x, y, src);
					insrt(epixel[j], (paddr[j] & 3) * 8, cpp * 8, res);
				}
			}
		}
		nv03_pgraph_dump_state(ctx, &real);
		bool mismatch = false;
		for (int j = 0; j < 4; j++) {
			rpixel[j] = nva_grd32(nva_cards[ctx->cnum]->bar1, paddr[j] & ~3);
			if (rpixel[j] != epixel[j])
			mismatch = true;
		}
		if (nv01_pgraph_cmp_state(&orig, &exp, &real, mismatch)) {
			for (int j = 0; j < 4; j++) {
				printf("pixel%d: %08x %08x %08x%s\n", j, pixel[j], epixel[j], rpixel[j], epixel[j] == rpixel[j] ? "" : " *");
			}
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_rop_blit(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int cls = 0x10;
		uint32_t val = 0x00010001;
		uint32_t mthd = 0x0308;
		uint32_t addr = mthd;
		uint32_t gctx = jrand48(ctx->rand48);
		uint32_t grobj[4];
		uint32_t spaddr[4], spixel[4];
		uint32_t paddr[4], pixel[4], epixel[4], rpixel[4];
		grobj[0] = jrand48(ctx->rand48);
		grobj[1] = jrand48(ctx->rand48);
		grobj[2] = jrand48(ctx->rand48);
		grobj[3] = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv03_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x10000;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000080;
		orig.ctx_user &= ~0xe000;
		orig.ctx_switch[0] &= ~0x8000;
		int op = extr(orig.ctx_switch[0], 24, 5);
		if (!((op >= 0x00 && op <= 0x15) || op == 0x17 || (op >= 0x19 && op <= 0x1a) || op == 0x1d))
			op = 0x17;
		insrt(orig.ctx_switch[0], 24, 5, op);
		orig.pattern_config = nrand48(ctx->rand48)%3; /* shape 3 is a rather ugly hole in Karnough map */
		// XXX: if source pixel hits cliprect, bad things happen
		orig.cliprect_ctrl = 0;
		orig.xy_misc_0 = 0x20000000;
		orig.xy_misc_1[0] = 0;
		orig.xy_misc_1[1] = 0;
		orig.xy_misc_3 = 0;
		orig.xy_misc_4[0] = 0;
		orig.xy_misc_4[1] = 0;
		orig.xy_clip[0][0] = 0;
		orig.xy_clip[1][0] = 0;
		orig.valid[0] = 0x0303;
		orig.surf_offset[0] = 0x000000;
		orig.surf_offset[1] = 0x040000;
		orig.surf_offset[2] = 0x080000;
		orig.surf_offset[3] = 0x0c0000;
		orig.surf_pitch[0] = 0x0400;
		orig.surf_pitch[1] = 0x0400;
		orig.surf_pitch[2] = 0x0400;
		orig.surf_pitch[3] = 0x0400;
		if (op > 0x17)
			orig.surf_format |= 0x2222;
		orig.src_canvas_min = 0x00000080;
		orig.src_canvas_max = 0x01000100;
		int x = jrand48(ctx->rand48) & 0x7f;
		int y = jrand48(ctx->rand48) & 0xff;
		int sx = jrand48(ctx->rand48) & 0xff;
		int sy = jrand48(ctx->rand48) & 0xff;
		orig.vtx_x[0] = sx;
		orig.vtx_y[0] = sy;
		orig.vtx_x[1] = x;
		orig.vtx_y[1] = y;
		orig.debug[3] &= ~(1 << 22);
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, addr);
		if (jrand48(ctx->rand48) & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		int cpp = nv03_pgraph_cpp(&orig);
		for (int j = 0; j < 4; j++) {
			spaddr[j] = (sx * cpp + sy * 0x400 + j * 0x40000);
			spixel[j] = jrand48(ctx->rand48);
			if (sx >= 0x80)
				nva_gwr32(nva_cards[ctx->cnum]->bar1, spaddr[j] & ~3, spixel[j]);
		}
		for (int j = 0; j < 4; j++) {
			paddr[j] = (x * cpp + y * 0x400 + j * 0x40000);
			pixel[j] = epixel[j] = jrand48(ctx->rand48);
			nva_gwr32(nva_cards[ctx->cnum]->bar1, paddr[j] & ~3, pixel[j]);
		}
		nv03_pgraph_load_state(ctx, &orig);
		exp = orig;
		nv03_pgraph_mthd(ctx, &exp, grobj, gctx, addr, val);
		exp.valid[0] = 0;
		insrt(exp.xy_misc_0, 28, 4, 0);
		insrt(exp.xy_misc_1[1], 0, 1, 1);
		nv03_pgraph_vtx_add(&exp, 0, 2, exp.vtx_x[0], extr(val, 0, 16), 0, false);
		nv03_pgraph_vtx_add(&exp, 1, 2, exp.vtx_y[0], extr(val, 16, 16), 0, false);
		nv03_pgraph_vtx_add(&exp, 0, 3, exp.vtx_x[1], extr(val, 0, 16), 0, false);
		nv03_pgraph_vtx_add(&exp, 1, 3, exp.vtx_y[1], extr(val, 16, 16), 0, false);
		nv03_pgraph_vtx_cmp(&exp, 0, 8, true);
		nv03_pgraph_vtx_cmp(&exp, 1, 8, true);
		if (pgraph_cliprect_pass(&exp, x, y)) {
			int fmt = nv03_pgraph_surf_format(&exp) & 3;
			int ss = extr(exp.ctx_switch[0], 16, 2);
			uint32_t sp = extr(spixel[ss], (spaddr[ss] & 3) * 8, cpp * 8);
			if (sx < 0x80)
				sp = 0;
			if (!pgraph_cliprect_pass(&exp, sx, sy))
				sp = 0xffffffff;
			struct pgraph_color s = nv03_pgraph_expand_surf(fmt, sp);
			for (int j = 0; j < 4; j++) {
				if (extr(exp.ctx_switch[0], 20 + j, 1)) {
					uint32_t src = extr(pixel[j], (paddr[j] & 3) * 8, cpp * 8);
					uint32_t res = nv03_pgraph_rop(&exp, x, y, src, s);
					insrt(epixel[j], (paddr[j] & 3) * 8, cpp * 8, res);
				}
			}
		}
		nv03_pgraph_dump_state(ctx, &real);
		bool mismatch = false;
		for (int j = 0; j < 4; j++) {
			rpixel[j] = nva_grd32(nva_cards[ctx->cnum]->bar1, paddr[j] & ~3);
			if (rpixel[j] != epixel[j])
			mismatch = true;
		}
		if (nv01_pgraph_cmp_state(&orig, &exp, &real, mismatch)) {
			for (int j = 0; j < 4; j++) {
				printf("spixel%d: %08x\n", j, spixel[j]);
			}
			for (int j = 0; j < 4; j++) {
				printf("pixel%d: %08x %08x %08x%s\n", j, pixel[j], epixel[j], rpixel[j], epixel[j] == rpixel[j] ? "" : " *");
			}
			printf("Mthd %08x %08x %08x iter %d\n", gctx, addr, val, i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int nv03_pgraph_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset != 0x03)
		return HWTEST_RES_NA;
	if (!(nva_rd32(ctx->cnum, 0x200) & 1 << 24)) {
		printf("Mem controller not up.\n");
		return HWTEST_RES_UNPREP;
	}
	nva_wr32(ctx->cnum, 0x200, 0xffffeeff);
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	return HWTEST_RES_PASS;
}

static int simple_mthd_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int rop_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

namespace {

HWTEST_DEF_GROUP(simple_mthd,
	HWTEST_TEST(test_mthd_invalid, 0),
	HWTEST_TEST(test_mthd_ctxsw, 0),
	HWTEST_TEST(test_mthd_notify, 0),
)

HWTEST_DEF_GROUP(rop,
	HWTEST_TEST(test_rop_simple, 0),
	HWTEST_TEST(test_rop_zpoint, 0),
	HWTEST_TEST(test_rop_blit, 0),
)

}

HWTEST_DEF_GROUP(nv03_pgraph,
	HWTEST_GROUP(simple_mthd),
	HWTEST_GROUP(rop),
)
