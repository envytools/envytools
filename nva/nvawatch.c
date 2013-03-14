/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "nva.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>

int cnum = 0;
int32_t a;

static const int SZ = 1024 * 1024;

uint32_t queue[1024 * 1024];
uint32_t tqueue[1024 * 1024];
volatile int get = 0, put = 0;

void *watchfun(void *x) {
	uint32_t val = nva_rd32(cnum, a);
	queue[put] = val;
	put = (put + 1) % SZ;
	while (1) {
		uint32_t nval = nva_rd32(cnum, a);
		if (nval != val) {
			queue[put] = nval;
			put = (put + 1) % SZ;
		}
		val = nval;
	}
}

void *twatchfun(void *x) {
	uint32_t val = nva_rd32(cnum, a);
	queue[put] = val;
	tqueue[put] = nva_rd32(cnum, 0x9400);
	put = (put + 1) % SZ;
	while (1) {
		uint32_t nval = nva_rd32(cnum, a);
		if (nval != val) {
			queue[put] = nval;
			tqueue[put] = nva_rd32(cnum, 0x9400);
			put = (put + 1) % SZ;
		}
		val = nval;
	}
}

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
	int wanttime = 0;
	while ((c = getopt (argc, argv, "tc:")) != -1)
		switch (c) {
			case 't':
				wanttime = 1;
				break;
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
		}
	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}
	if (optind >= argc) {
		fprintf (stderr, "No address specified.\n");
		return 1;
	}
	sscanf (argv[optind], "%x", &a);
	pthread_t thr;
	pthread_create(&thr, 0, wanttime ? twatchfun : watchfun, 0);
	uint32_t ptime = 0;
	while (1) {
		while (get == put)
			sched_yield();
		if (wanttime)
			printf("%08x[+%d]: %08x\n", tqueue[get], tqueue[get]-ptime, queue[get]);
		else
			printf("%08x\n", queue[get]);
		ptime = tqueue[get];
		get = (get + 1) % SZ;
	}
}
