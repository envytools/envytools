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

#ifndef OLD_H
#define OLD_H

#include "hwtest.h"

namespace hwtest {
	class OldTest : public Test {
	private:
		int (*fun) (struct hwtest_ctx *);
	public:
		OldTest(TestOptions &opt, uint32_t seed, int (*fun) (struct hwtest_ctx *))
			: Test(opt, seed), fun(fun) {}
		int run() override {
			struct hwtest_ctx ctx;
			ctx.cnum = cnum;
			ctx.chipset = chipset;
			ctx.rand48[0] = rnd();
			ctx.rand48[1] = rnd();
			ctx.rand48[2] = rnd();
			return fun(&ctx);
		}
	};

	class OldTestGroup : public Test {
	private:
		const struct hwtest_group *group;
	public:
		OldTestGroup(TestOptions &opt, uint32_t seed, const struct hwtest_group *group)
			: Test(opt, seed), group(group) {}
		int run() override {
			struct hwtest_ctx ctx;
			ctx.cnum = cnum;
			ctx.chipset = chipset;
			ctx.rand48[0] = rnd();
			ctx.rand48[1] = rnd();
			ctx.rand48[2] = rnd();
			return group->prep(&ctx);
		}
		virtual std::vector<std::pair<const char *, Test *>> subtests() {
			std::vector<std::pair<const char *, Test *>> res;
			for (int i = 0; i < group->testsnum; i++) {
				auto &test = group->tests[i];
				res.push_back({
					test.name,
					test.group ? static_cast<Test*>(new OldTestGroup(opt, rnd(), test.group))
					: new OldTest(opt, rnd(), test.fun)
				});
			}
			return res;
		}
	};
}

#endif
