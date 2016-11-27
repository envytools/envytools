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
#include "nvhw/pgraph.h"
#include "hwtest.h"

void nv01_pgraph_gen_state(int cnum, std::mt19937 &rnd, struct pgraph_state *state);
void nv01_pgraph_load_state(int cnum, struct pgraph_state *state);
void nv01_pgraph_dump_state(int cnum, struct pgraph_state *state);
int nv01_pgraph_cmp_state(struct pgraph_state *orig, struct pgraph_state *exp, struct pgraph_state *real, bool broke = false);

namespace hwtest {
namespace pgraph {

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

class PGraphMthdMiscTests : public Test {
	bool supported() override;
	Subtests subtests() override;
public:
	PGraphMthdMiscTests(TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
};

class PGraphMthdSimpleTests : public Test {
	bool supported() override;
	Subtests subtests() override;
public:
	PGraphMthdSimpleTests(TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
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
