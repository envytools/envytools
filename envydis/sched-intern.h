/*
 * Copyright (C) 2018 Rhys Perry <pendingchaos02@gmail.com>
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

#ifndef SCHED_INTERN_H
#define SCHED_INTERN_H

#include "dis-intern.h"

#define SCHED_MAX_REGS 256

struct reginfo {
	unsigned reg;
	unsigned file;
	unsigned pos; //position within instruction
	int wr;

	// members initialized during and for scheduling
	int bar; //virtual barrier to wait on or -1
};

struct schedinfo {
	const char *name;
	uint64_t opcode;
	int regwr[16]; /* terminated by -1 */
	int latency; /* negative if it requires barriers */
	int controlflow;
	int mindelay;
	int regsize[16]; /* terminated by a size of 0 */
	uint64_t mask;
	uint64_t val;
};

struct insninfo {
	uint64_t insn;
	const struct schedinfo *sched;
	uint64_t opcode;
	unsigned regcount;
	struct reginfo regs[64];

	// members initialized during and for scheduling
	int rdbar; //virtual read barrier created or -1
	int wrbar; //virtual write barrier created or -1
};

/* scheduling data for one instruction */
struct schedinsn {
	unsigned delay:4;
	unsigned rdbar:3; /* 7 if none is created */
	unsigned wrbar:3; /* 7 if none is created */
	unsigned wait:6;
};

struct schedtarget {
	unsigned schedinfonum;
	const struct schedinfo *schedinfo;

	unsigned regfilesnum;
	const char *const* regfiles;
	const int *regbitbuckets;
	const int *regminlatency;

	unsigned numbarriers;
	unsigned barriermindelay;

	unsigned (*getregsize)(const struct insninfo *info, unsigned pos);
	void (*atomcallback)(struct insninfo *info, const struct atom *atom);
	void (*insncallback)(struct insninfo *info);

	int (*printsched)(FILE *fp, int insnsnum, int remaining, struct schedinsn *insns);
	void (*printpaddingnops)(FILE *fp, int insnsnum);
};

#endif
