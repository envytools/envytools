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

#ifndef HWTEST_H
#define HWTEST_H

#include "util.h"
#include "nvhw/chipset.h"
#include <stdint.h>
#include <vector>
#include <string>
#include <utility>
#include <random>

enum hwtest_res {
	HWTEST_RES_NA,
	HWTEST_RES_PASS,
	HWTEST_RES_UNPREP,
	HWTEST_RES_FAIL,
};

namespace hwtest {
	struct TestOptions {
		int cnum;
		bool noslow;
		bool colors;
		bool keep_going;
		int repeat_factor;
	};

	class Test {
	protected:
		std::mt19937 rnd;
		TestOptions &opt;
		int cnum;
		struct chipset_info chipset;
	public:
		typedef std::vector<std::pair<std::string, Test *>> Subtests;
		virtual ~Test() {}
		virtual int run() {
			return HWTEST_RES_PASS;
		}
		virtual Subtests subtests() {
			return {};
		}
		virtual bool supported() {
			return true;
		}
		virtual bool subtests_boring() {
			return false;
		}
		Test(TestOptions &opt, uint32_t seed);
	};

	class RepeatTest : public Test {
	protected:
		virtual int run_once() = 0;
		virtual int repeats() {
			return 1000;
		}
		RepeatTest(TestOptions &opt, uint32_t seed) : Test(opt, seed) {}
	public:
		int run() override;
	};
}

uint32_t vram_rd32(int card, uint64_t addr);
void vram_wr32(int card, uint64_t addr, uint32_t val);

hwtest::Test *pgraph_tests(hwtest::TestOptions &opt, uint32_t seed);
hwtest::Test *pfifo_tests(hwtest::TestOptions &opt, uint32_t seed);
hwtest::Test *g80_pgraph_test(hwtest::TestOptions &opt, uint32_t seed);

#endif
