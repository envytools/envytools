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

#ifndef HWTEST_PGRAPH_H
#define HWTEST_PGRAPH_H

#include <random>
#include <string>
#include <memory>
#include <vector>
#include "nvhw/pgraph.h"
#include "hwtest.h"
#include "nva.h"

void pgraph_gen_state(int cnum, std::mt19937 &rnd, struct pgraph_state *state);
void pgraph_load_state(int cnum, struct pgraph_state *state);
void pgraph_dump_state(int cnum, struct pgraph_state *state);
int pgraph_cmp_state(struct pgraph_state *orig, struct pgraph_state *exp, struct pgraph_state *real, bool broke = false);

namespace hwtest {
namespace pgraph {

class Register {
public:
	uint32_t mask;
	uint32_t fixed;
	Register(uint32_t mask, uint32_t fixed = 0) : mask(mask), fixed(fixed) {}
	virtual ~Register() {}
	virtual std::string name() = 0;
	virtual uint32_t &ref(struct pgraph_state *state) = 0;
	virtual uint32_t sim_read(struct pgraph_state *state) { return ref(state); }
	virtual void sim_write(struct pgraph_state *state, uint32_t val) { ref(state) = (val & mask) | fixed; }
	virtual uint32_t read(int cnum) = 0;
	virtual void write(int cnum, uint32_t val) = 0;
	virtual void gen(struct pgraph_state *state, int cnum, std::mt19937 &rnd) {
		ref(state) = (rnd() & mask) | fixed;
	}
	virtual bool diff(struct pgraph_state *exp, struct pgraph_state *real) {
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
	uint32_t pgraph_state::*ptr;
	SimpleMmioRegister(uint32_t addr, uint32_t mask, std::string name, uint32_t pgraph_state::*ptr, uint32_t fixed = 0) :
		MmioRegister(addr, mask, fixed), name_(name), ptr(ptr) {}
	std::string name() override { return name_; }
	uint32_t &ref(struct pgraph_state *state) override { return state->*ptr; }
};

template<int num>
class IndexedMmioRegister : public MmioRegister {
public:
	std::string name_;
	uint32_t (pgraph_state::*ptr)[num];
	int idx;
	IndexedMmioRegister(uint32_t addr, uint32_t mask, std::string name, uint32_t (pgraph_state::*ptr)[num], int idx, uint32_t fixed = 0) :
		MmioRegister(addr, mask, fixed), name_(name), ptr(ptr), idx(idx) {}
	std::string name() override {
		return name_ + "[" + std::to_string(idx) + "]";
	}
	uint32_t &ref(struct pgraph_state *state) override { return (state->*ptr)[idx]; }
};

std::vector<std::unique_ptr<Register>> pgraph_rop_regs(const chipset_info &chipset);
std::vector<std::unique_ptr<Register>> pgraph_canvas_regs(const chipset_info &chipset);
std::vector<std::unique_ptr<Register>> pgraph_vstate_regs(const chipset_info &chipset);
std::vector<std::unique_ptr<Register>> pgraph_d3d0_regs(const chipset_info &chipset);
std::vector<std::unique_ptr<Register>> pgraph_d3d56_regs(const chipset_info &chipset);
std::vector<std::unique_ptr<Register>> pgraph_celsius_regs(const chipset_info &chipset);
std::vector<std::unique_ptr<Register>> pgraph_dma_nv3_regs(const chipset_info &chipset);
std::vector<std::unique_ptr<Register>> pgraph_dma_nv4_regs(const chipset_info &chipset);

class StateTest : public RepeatTest {
protected:
	bool skip;
	struct pgraph_state orig, exp, real;
	virtual void adjust_orig() {}
	virtual void mutate() {}
	virtual bool other_fail() { return false; }
	virtual void print_fail() {}
	int run_once() override;
public:
	StateTest(TestOptions &opt, uint32_t seed) : RepeatTest(opt, seed) {}
};

class PGraphScanTests : public Test {
	bool supported() override;
	Subtests subtests() override;
public:
	PGraphScanTests(TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class PGraphStateTests : public Test {
	bool supported() override;
	Subtests subtests() override;
public:
	PGraphStateTests(TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class PGraphMthdInvalidTests : public Test {
	bool supported() override;
	Subtests subtests() override;
public:
	PGraphMthdInvalidTests(TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class PGraphMthdMiscTests : public Test {
	bool supported() override;
	Subtests subtests() override;
public:
	PGraphMthdMiscTests(TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class PGraphMthdXyTests : public Test {
	bool supported() override;
	Subtests subtests() override;
public:
	PGraphMthdXyTests(TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class PGraphRopTests : public Test {
	bool supported() override;
	Subtests subtests() override;
public:
	PGraphRopTests(TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

}
}

#endif
