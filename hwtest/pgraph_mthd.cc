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
#include "pgraph_mthd.h"
#include "pgraph_class.h"
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

static void nv04_pgraph_prep_mthd(int cnum, std::mt19937 &rnd, uint32_t grobj[4], struct pgraph_state *state, uint32_t cls, uint32_t addr, uint32_t val) {
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
		if (nv04_pgraph_is_nv15p(&state->chipset)) {
			insrt(state->ctx_user, 31, 1, 0);
			insrt(state->debug[3], 8, 1, 0);
		}
	}
	state->fifo_data_st2[0] = val;
	state->fifo_enable = 1;
	state->ctx_control |= 1 << 16;
	insrt(state->ctx_user, 13, 3, extr(addr, 13, 3));
	int old_subc = extr(state->ctx_user, 13, 3);
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
			if (rnd() & 3)
				grobj[3] = 0;
		} else {
			insrt(state->ctx_cache[subc][0], 0, 8, cls);
			if (nv04_pgraph_is_nv15p(&state->chipset)) {
				insrt(state->ctx_cache[subc][0], 23, 1, 0);
			}
			if (rnd() & 3)
				state->ctx_cache[subc][4] = 0;
		}
		inst = state->ctx_cache[subc][3];
	} else {
		insrt(state->ctx_switch[0], 0, 8, cls);
		inst = state->ctx_switch[3];
		if (rnd() & 3)
			state->ctx_switch[4] = 0;
	}
	for (int i = 0; i < 4; i++)
		nva_wr32(cnum, 0x700000 | inst << 4 | i << 2, grobj[i]);
}

static void nv04_pgraph_mthd(struct pgraph_state *state, uint32_t grobj[4], int trapbit, bool data_err) {
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
		if (extr(state->debug[3], 21, 1))
			data_err = true;
		if (extr(state->debug[3], 20, 1) && data_err && !ctxsw)
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
		insrt(orig.notify, 16, 1, 0);
		if (chipset.card_type < 3)
			insrt(orig.notify, 20, 1, 0);
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
	} else if (chipset.card_type >= 4) {
		if (chipset.card_type >= 0x10 && fix_alt_cls() && extr(orig.debug[3], 16, 1)) {
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
		nv04_pgraph_prep_mthd(cnum, rnd, grobj, &orig, cls, subc << 13 | mthd, val);
	}
}

void MthdTest::mutate() {
	bool data_err = !is_valid_val();
	emulate_mthd_pre();
	if (chipset.card_type < 3) {
		nva_wr32(cnum, 0x400000 | cls << 16 | mthd, val);
	} else if (chipset.card_type < 4) {
		nv03_pgraph_mthd(cnum, &exp, grobj, gctx, subc << 13 | mthd, val);
	} else {
		nv04_pgraph_mthd(&exp, grobj, trapbit, data_err);
	}
	if (data_err && chipset.card_type < 4) {
		if ((chipset.card_type < 3 || extr(exp.debug[3], 20, 1)) && !extr(exp.invalid, 16, 1)) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			if (chipset.card_type < 3) {
				exp.access &= ~0x101;
			} else {
				exp.fifo_enable = 0;
			}
		}
	}
	bool missing_hw = false;
	if (chipset.card_type >= 0x10 && extr(exp.intr, 4, 1))
		missing_hw = true;
	if (!missing_hw)
		emulate_mthd();
}

void MthdTest::print_fail() {
	printf("Mthd %02x.%04x <- %08x\n", cls, mthd, val);
}

namespace {

class MthdInvalid : public SingleMthdTest {
	bool special_notify() override {
		return chipset.card_type == 3;
	}
	int repeats() override {
		return 1;
	}
	void emulate_mthd() override {
		if (chipset.card_type < 4) {
			if (!extr(exp.invalid, 16, 1)) {
				exp.intr |= 1;
				exp.invalid |= 1;
				if (chipset.card_type < 3) {
					exp.access &= ~0x101;
				} else {
					exp.fifo_enable = 0;
					if (!extr(exp.invalid, 4, 1)) {
						if (extr(exp.notify, 16, 1) && extr(exp.notify, 20, 4)) {
							exp.intr |= 1 << 28;
						}
					}
				}
			}
		} else {
			nv04_pgraph_blowup(&exp, 0x40);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class ClassInvalidTests : public Test {
	std::vector<bool> valid;
	uint32_t cls;
	bool subtests_boring() override {
		return true;
	}
	Subtests subtests() override {
		Subtests res;
		uint32_t mthds = chipset.card_type < 3 ? 0x10000 : 0x2000;
		for (uint32_t mthd = 0; mthd < mthds; mthd += 4) {
			if (mthd != 0 && !valid[mthd]) {
				char buf[5];
				snprintf(buf, sizeof buf, "%04x", mthd);
				res.push_back({buf, new MthdInvalid(opt, rnd(), buf, -1, cls, mthd)});
			}
		}
		return res;
	}
public:
	ClassInvalidTests(TestOptions &opt, uint32_t seed, const std::vector<bool> &valid, uint32_t cls) : Test(opt, seed), valid(valid), cls(cls) {}
};

}

Test::Subtests ClassTest::subtests() {
	Subtests res;
	std::vector<bool> valid(0x10000);
	valid[0] = true;
	for (auto *test : cls->mthds()) {
		res.push_back({test->name, test});
		if (test->supported()) {
			for (unsigned i = 0; i < test->s_num; i++) {
				valid[test->s_mthd + test->s_stride * i] = true;
			}
		}
	}
	res.push_back({"invalid", new ClassInvalidTests(opt, rnd(), valid, cls->cls)});
	return res;
}

}
}
