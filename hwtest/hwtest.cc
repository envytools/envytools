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
#include "nva.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pciaccess.h>

hwtest::Test::Test(TestOptions &opt, uint32_t seed) : rnd(seed), opt(opt), cnum(opt.cnum) {
	chipset = nva_cards[cnum]->chipset;
}

int hwtest_run_group(hwtest::TestOptions &opt, const char *gname, hwtest::Test *group, int indent, const char *filter) {
	static const char *const tabn[] = {
		[HWTEST_RES_NA] = "n/a",
		[HWTEST_RES_PASS] = "passed",
		[HWTEST_RES_UNPREP] = "hw not prepared",
		[HWTEST_RES_FAIL] = "FAILED",
	};
	static const char *const tabc[] = {
		[HWTEST_RES_NA] = "n/a",
		[HWTEST_RES_PASS] = "\x1b[32mpassed\x1b[0m",
		[HWTEST_RES_UNPREP] = "\x1b[33mhw not prepared\x1b[0m",
		[HWTEST_RES_FAIL] = "\x1b[31mFAILED\x1b[0m",
	};
	const char *const *tab = opt.colors ? tabc : tabn;
	int pres = group->run();
	auto subtests = group->subtests();
	if (pres != HWTEST_RES_PASS || !subtests.size()) {
		if (gname) {
			for (int j = 0; j < indent; j++)
				printf("  ");
			printf("%s: %s\n", gname, tab[pres]);
		}
		return pres;
	}
	if (gname) {
		for (int j = 0; j < indent; j++)
			printf("  ");
		printf("%s...\n", gname);
		indent++;
	}
	int worst = 0;
	size_t flen = 0;
	const char *fnext = 0;
	if (filter) {
		fnext = strchr(filter, '/');
		if (fnext) {
			flen = fnext - filter;
			fnext++;
		} else {
			flen = strlen(filter);
		}
	}
	bool found = false;
	for (auto &sub : subtests) {
		auto &name = sub.first;
		auto &test = sub.second;
		if (filter && (strlen(name) != flen || strncmp(name, filter, flen)))
			continue;
		found = true;
		int res = hwtest_run_group(opt, name, test, indent, fnext);
		if (worst < res)
			worst = res;
	}
	if (!found) {
		printf("No tests found for %s!\n", filter);
		return HWTEST_RES_NA;
	}
	if (gname) {
		indent--;
		for (int j = 0; j < indent; j++)
			printf("  ");
		printf("%s: %s\n", gname, tab[worst]);
	}
	return worst;
}

int hwtest_root_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(hwtest_root,
	HWTEST_GROUP(nv01_pgraph),
	HWTEST_GROUP(nv03_pgraph),
	HWTEST_GROUP(nv04_pgraph),
	HWTEST_GROUP(nv10_tile),
	HWTEST_GROUP(mpeg_crypt),
	HWTEST_GROUP(nv50_ptherm),
	HWTEST_GROUP(nv84_ptherm),
	HWTEST_GROUP(vp2_macro),
	HWTEST_GROUP(pvcomp_isa),
	HWTEST_GROUP(vp1),
	HWTEST_GROUP(g80_int),
	HWTEST_GROUP(g80_fp),
	HWTEST_GROUP(g80_sfu),
	HWTEST_GROUP(g80_fp64),
	HWTEST_GROUP(g80_atom32),
	HWTEST_GROUP(g80_atom64),
)

int main(int argc, char **argv) {
	hwtest::TestOptions opt;
	opt.noslow = false;
	opt.colors = true;
	opt.cnum = 0;
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c, force = 0;
	while ((c = getopt (argc, argv, "c:nsf")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &opt.cnum);
				break;
			case 'n':
				opt.colors = 0;
				break;
			case 's':
				opt.noslow = 1;
				break;
			case 'f':
				force = 1;
				break;
		}
	if (opt.cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}
	if (nva_cards[opt.cnum]->bus_type == NVA_BUS_PCI &&
		pci_device_has_kernel_driver(nva_cards[opt.cnum]->bus.pci)) {
		if (force) {
			fprintf(stderr, "WARNING: Kernel driver in use.\n");
		} else {
			fprintf(stderr, "ERROR: Kernel driver in use. If you know what you are doing, use -f option.\n");
			return 1;
		}
	}
	hwtest::Test *root = new hwtest::OldTestGroup(opt, 1, &hwtest_root_group);
	int worst = 0;
	if (optind == argc) {
		printf("Running all tests...\n");
		worst = hwtest_run_group(opt, nullptr, root, 0, nullptr);
	} else while (optind < argc) {
		int res = hwtest_run_group(opt, nullptr, root, 0, argv[optind++]);
		if (res > worst)
			worst = res;
	}
	if (worst == HWTEST_RES_PASS)
		return 0;
	else
		return worst + 1;
}
