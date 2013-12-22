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
#include <inttypes.h>

int cnum = 0;
int32_t a;

static const int SZ = 1024 * 1024;

uint32_t queue[1024 * 1024];
uint32_t tqueue[1024 * 1024];
uint64_t tqueue64[1024 * 1024];
volatile int get = 0, put = 0;
uint32_t mask = 0xffffffff;

#define NV04_PTIMER_TIME_0                                 0x00009400
#define NV04_PTIMER_TIME_1                                 0x00009410
uint64_t get_time(unsigned int card)
{
	uint64_t low;

	uint64_t high2 = nva_rd32(card, NV04_PTIMER_TIME_1);
	uint64_t high1;
	do {
		high1 = high2;
		low = nva_rd32(card, NV04_PTIMER_TIME_0);
		high2 = nva_rd32(card, NV04_PTIMER_TIME_1);
	} while (high1 != high2);
	return ((((uint64_t)high2) << 32) | (uint64_t)low);
}

void *watchfun(void *x) {
	uint32_t val = nva_rd32(cnum, a);
	queue[put] = val;
	put = (put + 1) % SZ;
	while (1) {
		uint32_t nval = nva_rd32(cnum, a);
		if ((nval & mask) != (val & mask)) {
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
		if ((nval & mask) != (val & mask)) {
			queue[put] = nval;
			tqueue[put] = nva_rd32(cnum, 0x9400);
			put = (put + 1) % SZ;
		}
		val = nval;
	}
}

void *t64watchfun(void *x) {
	uint32_t val = nva_rd32(cnum, a);
	queue[put] = val;
	tqueue64[put] = get_time(cnum);
	put = (put + 1) % SZ;
	while (1) {
		uint32_t nval = nva_rd32(cnum, a);
		if ((nval & mask) != (val & mask)) {
			queue[put] = nval;
			tqueue64[put] = get_time(cnum);
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
	unsigned int wanttime = 0;
	while ((c = getopt (argc, argv, "tc:m:")) != -1)
		switch (c) {
			case 't':
				wanttime++;
				if (wanttime > 2)
					wanttime = 2;
				break;
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
			case 'm':
				sscanf(optarg, "%x", &mask);
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
	if (wanttime == 0)
		pthread_create(&thr, 0, watchfun, 0);
	else if (wanttime == 1)
		pthread_create(&thr, 0, twatchfun, 0);
	else
		pthread_create(&thr, 0, t64watchfun, 0);

	uint32_t ptime = 0;
	uint64_t ptime64 = 0;
	while (1) {
		while (get == put)
			sched_yield();

		if (wanttime == 0)
			printf("%08x\n", queue[get]);
		else if (wanttime == 1) {
			printf("%08x[+%d]: %08x\n", tqueue[get], tqueue[get]-ptime, queue[get]);
			ptime = tqueue[get];
		} else {
			printf("%016"PRIu64"[+%"PRIu64"]: %08x\n", tqueue64[get], tqueue64[get]-ptime64, queue[get]);
			ptime64 = tqueue64[get];
		}

		get = (get + 1) % SZ;
	}
}
