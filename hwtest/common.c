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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "hwtest.h"
#include <stdlib.h>
#include <stdio.h>

int hwtest_run_group(struct hwtest_ctx *ctx, const struct hwtest_group *group) {
	int i;
	int worst = 0;
	for (i = 0; i < group->testsnum; i++) {
		if (group->tests[i].slow && ctx->noslow) {
			printf("%s: skipped\n", group->tests[i].name);
		} else {
			int res = group->tests[i].fun(ctx);
			if (worst < res)
				worst = res;
			const char *tab[] = {
				[HWTEST_RES_NA] = "n/a",
				[HWTEST_RES_PASS] = "passed",
				[HWTEST_RES_UNPREP] = "hw not prepared",
				[HWTEST_RES_FAIL] = "FAILED",
			};
			const char *tabc[] = {
				[HWTEST_RES_NA] = "n/a",
				[HWTEST_RES_PASS] = "\x1b[32mpassed\x1b[0m",
				[HWTEST_RES_UNPREP] = "\x1b[33mhw not prepared\x1b[0m",
				[HWTEST_RES_FAIL] = "\x1b[31mFAILED\x1b[0m",
			};
			printf("%s: %s\n", group->tests[i].name, res[ctx->colors?tabc:tab]);
		}
	}
	return worst;
}
