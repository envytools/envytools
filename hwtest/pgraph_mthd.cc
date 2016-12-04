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
#include "nva.h"

namespace hwtest {
namespace pgraph {

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

static void nv03_pgraph_mthd(int cnum, struct pgraph_state *state, uint32_t *grobj, uint32_t gctx, uint32_t addr, uint32_t val) {
	if (extr(state->debug[1], 16, 1) || extr(addr, 0, 13) == 0) {
		uint32_t inst = gctx & 0xffff;
		nva_gwr32(nva_cards[cnum]->bar1, 0xc00000 + inst * 0x10, grobj[0]);
		nva_gwr32(nva_cards[cnum]->bar1, 0xc00004 + inst * 0x10, grobj[1]);
		nva_gwr32(nva_cards[cnum]->bar1, 0xc00008 + inst * 0x10, grobj[2]);
		nva_gwr32(nva_cards[cnum]->bar1, 0xc0000c + inst * 0x10, grobj[3]);
	}
	if ((addr & 0x1ffc) == 0) {
		int i;
		for (i = 0; i < 0x200; i++) {
			nva_gwr32(nva_cards[cnum]->bar1, 0xc00000 + i * 8, val);
			nva_gwr32(nva_cards[cnum]->bar1, 0xc00004 + i * 8, gctx | 1 << 23);
		}
	}
	nva_wr32(cnum, 0x2100, 0xffffffff);
	nva_wr32(cnum, 0x2140, 0xffffffff);
	nva_wr32(cnum, 0x2210, 0);
	nva_wr32(cnum, 0x2214, 0x1000);
	nva_wr32(cnum, 0x2218, 0x12000);
	nva_wr32(cnum, 0x2410, 0);
	nva_wr32(cnum, 0x2420, 0);
	nva_wr32(cnum, 0x3000, 0);
	nva_wr32(cnum, 0x3040, 0);
	nva_wr32(cnum, 0x3004, gctx >> 24);
	nva_wr32(cnum, 0x3010, 4);
	nva_wr32(cnum, 0x3070, 0);
	nva_wr32(cnum, 0x3080, (addr & 0x1ffc) == 0 ? 0xdeadbeef : gctx | 1 << 23);
	nva_wr32(cnum, 0x3100, addr);
	nva_wr32(cnum, 0x3104, val);
	nva_wr32(cnum, 0x3000, 1);
	nva_wr32(cnum, 0x3040, 1);
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

void MthdTest::adjust_orig() {
	grobj[0] = rnd();
	grobj[1] = rnd();
	grobj[2] = rnd();
	grobj[3] = rnd();
	val = rnd();
	subc = rnd() & 7;
	gctx = rnd();
	choose_mthd();
	adjust_orig_mthd();
	if (!special_notify()) {
		if (chipset.card_type < 4) {
			insrt(orig.notify, 16, 1, 0);
			if (chipset.card_type < 3)
				insrt(orig.notify, 20, 1, 0);
		}
	}
	if (chipset.card_type == 3) {
		nv03_pgraph_prep_mthd(&orig, &gctx, cls, subc << 13 | mthd);
		if (rnd() & 1)
			orig.ctx_switch[3] = gctx & 0xffff;
		uint32_t fmt = gen_nv3_fmt();
		uint32_t old_subc = extr(orig.ctx_user, 13, 3);
		bool will_reload_dma = (
			extr(orig.debug[1], 16, 1) && (gctx & 0xffff) != orig.ctx_switch[3] &&
			(cls == 0x0d || cls == 0x0e || cls == 0x14 || cls == 0x17 ||
			 mthd == 0x104));
		if (!will_reload_dma || subc == old_subc || !extr(orig.debug[1], 20, 1)) {
			insrt(orig.ctx_switch[0], 0, 3, fmt);
		} else {
			insrt(orig.ctx_cache[subc][0], 0, 3, fmt);
		}
	}
}

void MthdTest::mutate() {
	if (chipset.card_type < 3) {
		nva_wr32(cnum, 0x400000 | cls << 16 | mthd, val);
	} else {
		nv03_pgraph_mthd(cnum, &exp, grobj, gctx, subc << 13 | mthd, val);
	}
	bool data_err = !is_valid_val();
	if (data_err) {
		if (chipset.card_type < 4) {
			if (chipset.card_type < 3 || extr(exp.debug[3], 20, 1)) {
				exp.intr |= 1;
				exp.invalid |= 0x10;
				if (chipset.card_type < 3) {
					exp.access &= ~0x101;
				} else {
					exp.fifo_enable = 0;
				}
			}
		} else {
			// ...
		}
	}
	emulate_mthd();
}

void MthdTest::print_fail() {
	printf("Mthd %02x.%04x <- %08x\n", cls, mthd, val);
}

}
}
