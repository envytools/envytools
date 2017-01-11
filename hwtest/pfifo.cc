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

#include "hwtest.h"
#include "nva.h"
#include <vector>
#include <memory>

namespace {

struct pfifo_state {
	struct chipset_info chipset;
	uint32_t wait_retry;
	uint32_t cache_error;
	uint32_t intr;
	uint32_t intr_en;
	uint32_t config;
	uint32_t runout_put;
	uint32_t runout_get;
	uint32_t chsw_enable;
	uint32_t cache0_push_en;
	uint32_t cache0_chid;
	uint32_t cache0_put;
	uint32_t cache0_pull_en;
	uint32_t cache0_pull_state;
	uint32_t cache0_get;
	uint32_t cache0_ctx;
	uint32_t cache0_addr;
	uint32_t cache0_data;
	uint32_t cache1_push_en;
	uint32_t cache1_chid;
	uint32_t cache1_put;
	uint32_t cache1_pull_en;
	uint32_t cache1_pull_state;
	uint32_t cache1_get;
	uint32_t cache1_ctx[8];
	uint32_t cache1_addr[0x20];
	uint32_t cache1_data[0x20];
	bool cache1_runout;
	uint32_t ram_size;
};

uint32_t pfifo_runout_next(pfifo_state *state) {
	uint32_t next = state->runout_put + 8;
	next &= (1 << (state->ram_size + 11)) - 8;
	return next;
}

uint32_t pfifo_cache1_next(uint32_t ptr) {
	// yay Gray!
	int bits = 5;
	uint32_t tmp = ptr >> 2;
	if (tmp == 1u << (bits - 1))
		return 0;
	for (int bit = bits - 1; bit > 0; bit--) {
		if (tmp == 1u << (bit - 1)) {
			return ptr ^ 1 << (2 + bit);
		}
		if (tmp & 1 << bit)
			tmp ^= 3 << (bit - 1);
	}
	return ptr ^ 4;
}

class Register {
public:
	uint32_t mask;
	uint32_t fixed;
	Register(uint32_t mask, uint32_t fixed = 0) : mask(mask), fixed(fixed) {}
	virtual ~Register() {}
	virtual std::string name() = 0;
	virtual uint32_t &ref(pfifo_state *state) = 0;
	virtual uint32_t sim_read(pfifo_state *state) { return ref(state); }
	virtual void sim_write(pfifo_state *state, uint32_t val) { ref(state) = (val & mask) | fixed; }
	virtual uint32_t read(int cnum) = 0;
	virtual void write(int cnum, uint32_t val) = 0;
	virtual void gen(pfifo_state *state, int cnum, std::mt19937 &rnd) {
		ref(state) = (rnd() & mask) | fixed;
	}
	virtual bool diff(pfifo_state *exp, pfifo_state *real) {
		return ref(exp) != ref(real);
	}
	virtual bool scan_test(int cnum, std::mt19937 &rnd) {
		uint32_t tmp = read(cnum);
		write(cnum, 0xffffffff);
		uint32_t rall1 = read(cnum);
		write(cnum, 0);
		uint32_t rall0 = read(cnum);
		write(cnum, tmp);
		std::string name_ = name();
		if (rall1 != (mask | fixed) || rall0 != fixed) {
			printf("Bitscan mismatch for %s: is %08x/%08x, expected %08x/%08x\n", name_.c_str(), rall1, rall0, mask | fixed, fixed);
			return true;
		}
		return false;
	}
};

class MmioRegister : public Register {
public:
	uint32_t addr;
	MmioRegister(uint32_t addr, uint32_t mask, uint32_t fixed = 0) : Register(mask, fixed), addr(addr) {}
	uint32_t read(int cnum) override {
		return nva_rd32(cnum, addr);
	}
	void write(int cnum, uint32_t val) override {
		return nva_wr32(cnum, addr, val);
	}
};

class SimpleMmioRegister : public MmioRegister {
public:
	std::string name_;
	uint32_t pfifo_state::*ptr;
	SimpleMmioRegister(uint32_t addr, uint32_t mask, std::string name, uint32_t pfifo_state::*ptr, uint32_t fixed = 0) :
		MmioRegister(addr, mask, fixed), name_(name), ptr(ptr) {}
	std::string name() override { return name_; }
	uint32_t &ref(pfifo_state *state) override { return state->*ptr; }
};

template<int num>
class IndexedMmioRegister : public MmioRegister {
public:
	std::string name_;
	uint32_t (pfifo_state::*ptr)[num];
	int idx;
	IndexedMmioRegister(uint32_t addr, uint32_t mask, std::string name, uint32_t (pfifo_state::*ptr)[num], int idx, uint32_t fixed = 0) :
		MmioRegister(addr, mask, fixed), name_(name), ptr(ptr), idx(idx) {}
	std::string name() override {
		return name_ + "[" + std::to_string(idx) + "]";
	}
	uint32_t &ref(pfifo_state *state) override { return (state->*ptr)[idx]; }
};

#define REG(a, m, n, f) res.push_back(std::unique_ptr<Register>(new SimpleMmioRegister(a, m, n, &pfifo_state::f)))
#define IREGF(a, m, n, f, i, x, fx) res.push_back(std::unique_ptr<Register>(new IndexedMmioRegister<x>(a, m, n, &pfifo_state::f, i, fx)))
#define IREG(a, m, n, f, i, x) IREGF(a, m, n, f, i, x, 0)

std::vector<std::unique_ptr<Register>> pfifo_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	REG(0x602200, 0x3, "RAM_SIZE", ram_size);
	REG(0x2040, 0xff, "WAIT_RETRY", wait_retry);
	REG(0x2080, 0, "CACHE_ERROR", cache_error);
	REG(0x2100, 0, "INTR", intr);
	REG(0x2140, 0x111, "INTR_EN", intr_en);
	REG(0x2200, 3, "CONFIG", config);
	REG(0x2410, 0x3ff8, "RUNOUT_PUT", runout_put);
	REG(0x2420, 0x3ff8, "RUNOUT_GET", runout_get);
	REG(0x2500, 1, "CHSW_ENABLE", chsw_enable);
	REG(0x3010, 0x7f, "CACHE0.CHID", cache0_chid);
	REG(0x3030, 0x4, "CACHE0.PUT", cache0_put);
	REG(0x3050, 0x100, "CACHE0.PULL_STATE", cache0_pull_state);
	REG(0x3070, 0x4, "CACHE0.GET", cache0_get);
	REG(0x3080, 0x007fffff, "CACHE0.CTX", cache0_ctx);
	REG(0x3100, 0x0000fffc, "CACHE0.ADDR", cache0_addr);
	REG(0x3104, 0xffffffff, "CACHE0.DATA", cache0_data);
	REG(0x3210, 0x7f, "CACHE1.CHID", cache1_chid);
	REG(0x3230, 0x7c, "CACHE1.PUT", cache1_put);
	REG(0x3250, 0x117, "CACHE1.PULL_STATE", cache1_pull_state);
	REG(0x3270, 0x7c, "CACHE1.GET", cache1_get);
	for (int i = 0; i < 0x8; i++) {
		IREG(0x3280 + i * 0x10, 0x017fffff, "CACHE1.CTX", cache1_ctx, i, 8);
	}
	for (int i = 0; i < 0x20; i++) {
		IREG(0x3300 + i * 8, 0x0000fffc, "CACHE1.ADDR", cache1_addr, i, 0x20);
		IREG(0x3304 + i * 8, 0xffffffff, "CACHE1.DATA", cache1_data, i, 0x20);
	}
	REG(0x3000, 1, "CACHE0.PUSH_EN", cache0_push_en);
	REG(0x3040, 1, "CACHE0.PULL_EN", cache0_pull_en);
	REG(0x3200, 1, "CACHE1.PUSH_EN", cache1_push_en);
	REG(0x3240, 1, "CACHE1.PULL_EN", cache1_pull_en);
	return res;
}

void pfifo_gen_state(int cnum, std::mt19937 &rnd, pfifo_state *state) {
	state->chipset = nva_cards[cnum]->chipset;
	for (auto &reg : pfifo_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
	if (!(rnd() & 3)) {
		state->cache1_get = pfifo_cache1_next(state->cache1_put);
	}
	if (rnd() & 1) {
		state->runout_get = state->runout_put;
	}
	if (rnd() & 1) {
		state->cache1_chid = state->cache0_chid;
	}
	if (!(rnd() & 3)) {
		state->runout_get = pfifo_runout_next(state);
	}
	state->cache1_runout = rnd() & 1;
	if (state->runout_get == state->runout_put) {
		state->cache1_runout = false;
	}
	// Ensure no accidental submission.
	if (state->cache0_pull_en) {
		state->cache0_put = state->cache0_get;
	}
	if (state->cache1_pull_en) {
		state->cache1_put = state->cache1_get;
	}
}

void pfifo_load_state(int cnum, pfifo_state *state) {
	nva_wr32(cnum, 0x200, 0xffffeeff);
	nva_wr32(cnum, 0x200, 0xffffffff);
	if (state->cache1_runout) {
		int val = 0;
		insrt(val, 5, 1, !extr(state->runout_get, 5, 1));
		insrt(val, 6, 1, !extr(state->runout_put, 6, 1));
		nva_wr32(cnum, 0x2410, val);
		nva_wr32(cnum, 0x2420, val);
		nva_wr32(cnum, 0x3200, 1);
		nva_wr32(cnum, 0x3210, 0);
		nva_wr32(cnum, 0x800010, 0xdeabdeef);
	} else {
		nva_wr32(cnum, 0x2410, 0);
		nva_wr32(cnum, 0x2420, 0);
	}
	nva_wr32(cnum, 0x2100, 0xffffffff);
	for (auto &reg : pfifo_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pfifo_dump_state(int cnum, pfifo_state *state) {
	state->chipset = nva_cards[cnum]->chipset;
	for (auto &reg : pfifo_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
	state->cache1_runout = nva_rd32(cnum, 0x3220) & 1;
}

bool pfifo_cmp_state(pfifo_state *exp, pfifo_state *real) {
	bool broke = false;
	for (auto &reg : pfifo_regs(exp->chipset)) {
		std::string name = reg->name();
		if (reg->diff(exp, real)) {
			printf("Difference in reg %s: expected %08x real %08x\n",
				name.c_str(), reg->ref(exp), reg->ref(real));
			broke = true;
		}
	}
	if (exp->cache1_runout != real->cache1_runout) {
		printf("Difference in CACHE1.RUNOUT: expected %d real %d\n",
			exp->cache1_runout, real->cache1_runout);
		broke = true;
	}
	return broke;
}

bool pfifo_print_state(pfifo_state *orig, pfifo_state *exp, pfifo_state *real) {
	bool broke = false;
	for (auto &reg : pfifo_regs(exp->chipset)) {
		std::string name = reg->name();
		printf("%08x %08x %08x %s %s\n",
			reg->ref(orig), reg->ref(exp), reg->ref(real), name.c_str(),
			(!reg->diff(exp, real) ? "" : "*"));
	}
	return broke;
}

class PFifoStateTest : public hwtest::RepeatTest {
protected:
	bool skip;
	pfifo_state orig, exp, real;
	virtual void adjust_orig() {}
	virtual void mutate() {}
	virtual bool other_fail() { return false; }
	virtual void print_fail() {}
	int run_once() override {
		skip = false;
		pfifo_gen_state(cnum, rnd, &orig);
		adjust_orig();
		pfifo_load_state(cnum, &orig);
		exp = orig;
		mutate();
		pfifo_dump_state(cnum, &real);
		bool fail = other_fail();
		if (skip)
			return HWTEST_RES_NA;
		if (pfifo_cmp_state(&exp, &real) || fail) {
			pfifo_print_state(&orig, &exp, &real);
			print_fail();
			return HWTEST_RES_FAIL;
		}
		return HWTEST_RES_PASS;
	}
	using RepeatTest::RepeatTest;
};

class PFifoScanTest : public hwtest::Test {
	int run() override {
		int res = HWTEST_RES_PASS;
		nva_wr32(cnum, 0x2100, 0xffffffff);
		nva_wr32(cnum, 0x3030, 0);
		nva_wr32(cnum, 0x3070, 0);
		nva_wr32(cnum, 0x3230, 0);
		nva_wr32(cnum, 0x3270, 0);
		for (auto &reg : pfifo_regs(chipset)) {
			if (reg->scan_test(cnum, rnd))
				res = HWTEST_RES_FAIL;
		}
		return res;
	}
	using Test::Test;
};

uint32_t pfifo_runout_status(pfifo_state *state) {
	uint32_t res = 0;
	uint32_t get = state->runout_get;
	uint32_t put = state->runout_put;
	if (get == put)
		res |= 0x10;
	else
		res |= 1;
	if (get == pfifo_runout_next(state))
		res |= 0x100;
	return res;
}

class PFifoStatusTest : public PFifoStateTest {
	bool other_fail() override {
		uint32_t real_rs = nva_rd32(cnum, 0x2400);
		uint32_t real_c0s = nva_rd32(cnum, 0x3020);
		uint32_t real_c1s = nva_rd32(cnum, 0x3220);
		uint32_t exp_rs = pfifo_runout_status(&exp);
		uint32_t exp_c0s = 0x0;
		uint32_t exp_c1s = 0x00;
		if (exp.cache0_get == exp.cache0_put)
			exp_c0s |= 0x10;
		else
			exp_c0s |= 0x100;
		if (exp.cache1_get == exp.cache1_put)
			exp_c1s |= 0x10;
		else if (pfifo_cache1_next(exp.cache1_put) == exp.cache1_get)
			exp_c1s |= 0x100;
		if (exp.cache1_runout)
			exp_c1s |= 1;
		bool err = false;
		if (real_rs != exp_rs) {
			printf("RUNOUT_STATUS exp %08x real %08x\n", exp_rs, real_rs);
			err = true;
		}
		if (real_c0s != exp_c0s) {
			printf("CACHE0.STATUS exp %08x real %08x\n", exp_c0s, real_c0s);
			err = true;
		}
		if (real_c1s != exp_c1s) {
			printf("CACHE1.STATUS exp %08x real %08x\n", exp_c1s, real_c1s);
			err = true;
		}
		return err;
	}
	using PFifoStateTest::PFifoStateTest;
};

void pfifo_runout(pfifo_state *state, uint32_t *dst, uint32_t a, uint32_t b, bool intr) {
	if (intr)
		state->intr |= 0x10;
	uint32_t next = pfifo_runout_next(state);
	if (next == state->runout_get) {
		if (intr)
			state->intr |= 0x100;
	} else {
		uint32_t put = state->runout_put;
		put &= (1 << (state->ram_size + 11)) - 8;
		if (intr) {
			dst[put/4 + 0] = a;
			dst[put/4 + 1] = b;
		}
		state->runout_put = next;
	}
}

void pfifo_sim_user_write(pfifo_state *state, uint32_t *ramfc, uint32_t *runout, uint32_t addr, uint32_t val, uint32_t be) {
	bool intr = true;
	int err = -1;
	uint32_t nchid = extr(addr, 16, 7);
	if ((addr & 0x1ff0) == 0x20) {
		// Write to the password area - special.
		if (be != 0xf) {
			err = 0;
		} else if (state->cache1_chid != nchid) {
			if (!state->chsw_enable) {
				err = 0;
			} else if (state->runout_get != state->runout_put) {
				err = 0;
			} else if (state->cache1_get != state->cache1_put) {
				err = 0;
			}
		}
		intr = false;
	} else if ((addr & 0x1ff0) == 0x10 && (be == 0xc || be == 0x3 || be == 0xf)) {
		// 16-bit or 32-bit write to the FREE row - considered to be a reserved access.
		err = 5;
	} else if (be != 0xf) {
		// Any other non-32-bit write - considered to be an invalid access.
		err = 0;
	} else if ((addr & 0x1f00) == 0 && (addr & 0x1ffc) != 0) {
		// Write to 0-0xfc area other than CTX_SWITCH - reserved access.
		err = 5;
	} else if (!state->cache1_push_en) {
		// Pusher disabled - cache unavailable.
		err = 1;
	} else if (nchid == state->cache0_chid && state->cache0_push_en) {
		// Channel active on the evil cache - cache unavailable.
		err = 1;
	} else if (nchid != state->cache1_chid) {
		// We're on a different channel...
		if (state->runout_get != state->runout_put) {
			// Runout occured already - cache unavailable.
			err = 1;
		} else if (!state->chsw_enable) {
			// Channel switch disabled - cache unavailable.
			err = 1;
		} else if (state->cache1_get != state->cache1_put) {
			// Cache in use by a different channel - cache unavailable.
			err = 1;
		} else {
			// This is a switch.  Dump old channel.
			int rchid = state->cache1_chid;
			if (state->ram_size == 0)
				rchid &= 0x3f;
			for (int i = 0; i < 8; i++) {
				uint32_t val = state->cache1_ctx[i];
				insrt(val, 28, 3, extr(state->cache1_pull_state, 0, 3));
				insrt(val, 31, 1, extr(state->cache1_pull_state, 8, 1));
				if (i == 0 || extr(state->cache1_pull_state, 4, 1))
					ramfc[rchid * 8 + i] = val;
			}
			// Load new one.
			state->cache1_chid = nchid;
			rchid = state->cache1_chid;
			if (state->ram_size == 0)
				rchid &= 0x3f;
			for (int i = 0; i < 8; i++) {
				state->cache1_ctx[i] = ramfc[rchid * 8 + i] & 0x017fffff;
			}
			insrt(state->cache1_pull_state, 0, 3, extr(ramfc[rchid * 8], 28, 3));
			insrt(state->cache1_pull_state, 4, 1, 0);
			insrt(state->cache1_pull_state, 8, 1, extr(ramfc[rchid * 8], 31, 1));
			goto ok;
		}
	} else if (state->cache1_runout) {
		// Cache access OK, but runout already occured - cannot stop now.
		err = 2;
	} else if (state->cache1_get == pfifo_cache1_next(state->cache1_put)) {
		// Cache full.
		int subc = extr(state->cache1_pull_state, 0, 3);
		if (extr(state->cache1_ctx[subc], 24, 1)) {
			// We claimed there would be space - caught lying!
			err = 4;
		} else {
			// User didn't pay attention - free count overrun.
			err = 3;
		}
	} else {
ok:
		// Everything alright, put it in.
		int put = state->cache1_put >> 2;
		state->cache1_addr[put] = addr & 0xfffc;
		state->cache1_data[put] = val;
		state->cache1_put = pfifo_cache1_next(state->cache1_put);
	}
	if (err != -1) {
		uint32_t w = (addr & 0x7fffff) | err << 28;
		if (nchid == state->cache1_chid)
			state->cache1_runout = true;
		pfifo_runout(state, runout, w, val, intr);
	}
}

class PFifoUserWriteTest : public PFifoStateTest {
	uint32_t addr;
	int sz;
	uint32_t val;
	uint32_t oro[0x1000];
	uint32_t ero[0x1000];
	uint32_t rro[0x1000];
	uint32_t ofc[0x400];
	uint32_t efc[0x400];
	uint32_t rfc[0x400];
	void adjust_orig() override {
		orig.cache1_pull_en = 0;
	}
	void mutate() override {
		int ramfc_size = orig.ram_size ? 0x400 : 0x200;
		for (int i = 0; i < 1 << (orig.ram_size + 9); i++) {
			ero[i] = oro[i] = rnd();
			nva_wr32(cnum, 0x650000 | i << 2, oro[i]);
		}
		for (int i = 0; i < ramfc_size; i++) {
			efc[i] = ofc[i] = rnd();
			nva_wr32(cnum, 0x648000 | i << 2, ofc[i]);
		}

		sz = rnd() % 3;
		addr = (rnd() & 0x7fffff);
		if (!(rnd() & 7)) {
			insrt(addr, 4, 9, 2);
		}
		if (!(rnd() & 7)) {
			insrt(addr, 4, 9, 1);
		}
		if (rnd() & 1) {
			sz = 2;
			addr &= ~3;
		}
		if (rnd() & 1) {
			insrt(addr, 16, 7, exp.cache1_chid);
		}
		if (!(rnd() & 3)) {
			insrt(addr, 16, 7, exp.cache0_chid);
		}
		if (addr + (1 << sz) > 0x800000) {
			addr -= 4;
		}
		val = rnd();

		addr += 0x800000;

		if (sz == 0) {
			val &= 0xff;
			nva_wr8(cnum, addr, val);
		} else if (sz == 1) {
			val &= 0xffff;
			*((volatile uint16_t*)(((volatile uint8_t *)nva_cards[cnum]->bar0) + addr)) = val;
		} else {
			nva_wr32(cnum, addr, val);
		}
		uint8_t be = ((1 << (1 << sz)) - 1) << (addr & 3);
		pfifo_sim_user_write(&exp, efc, ero, addr, val << ((addr & 3) * 8), be & 0xf);
		if (be & 0xf0) {
			pfifo_sim_user_write(&exp, efc, ero, (addr | 3) + 1, val >> ((-addr & 3) * 8), be >> 4);
		}

		for (int i = 0; i < 1 << (orig.ram_size + 9); i++) {
			rro[i] = nva_rd32(cnum, 0x650000 | i << 2);
		}
		for (int i = 0; i < ramfc_size; i++) {
			rfc[i] = nva_rd32(cnum, 0x648000 | i << 2);
		}
	}
	void print_fail() override {
		printf("After writing %08x < %02x sz %d\n", addr, val, sz);
	}
	bool other_fail() override {
		int ramfc_size = orig.ram_size ? 0x400 : 0x200;
		bool err = false;
		for (int i = 0; i < (1 << (orig.ram_size + 8)); i++) {
			if (ero[i*2] != rro[i*2] || ero[i*2+1] != rro[i*2+1]) {
				printf("runout %04x exp %08x %08x real %08x %08x\n", i * 8, ero[i*2+0], ero[i*2+1], rro[i*2+0], rro[i*2+1]);
				err = true;
			}
		}
		for (int i = 0; i < ramfc_size; i += 8) {
			bool bad = false;
			for (int j = 0; j < 8; j++)
				if (efc[i+j] != rfc[i+j])
					bad = true;
			if (bad) {
				for (int j = 0; j < 8; j++) {
					printf("ramfc %02x.%d exp %08x real %08x\n", i / 8, j, efc[i+j], rfc[i+j]);
				}
				err = true;
			}
		}
		return err;
	}
	using PFifoStateTest::PFifoStateTest;
};

class PFifoTests : public hwtest::Test {
	bool supported() override {
		return chipset.card_type < 3;
	}
	int run() override {
		if (!(nva_rd32(cnum, 0x200) & 1 << 24)) {
			printf("Mem controller not up.\n");
			return HWTEST_RES_UNPREP;
		}
		nva_wr32(cnum, 0x200, 0xffffeeff);
		nva_wr32(cnum, 0x200, 0xffffffff);
		return HWTEST_RES_PASS;
	}
	Subtests subtests() override {
		return {
			{"scan", new PFifoScanTest(opt, rnd())},
			{"state", new PFifoStateTest(opt, rnd())},
			{"status", new PFifoStatusTest(opt, rnd())},
			{"user_write", new PFifoUserWriteTest(opt, rnd())},
		};
	}
	using Test::Test;
};

}

hwtest::Test *pfifo_tests(hwtest::TestOptions &opt, uint32_t seed) {
	return new PFifoTests(opt, seed);
}
