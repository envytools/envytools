/*
 *
 * Copyright (C) 2009 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "dis.h"
#include "rnn.h"

/*
 * Immediate fields
 */

static int stdoff[] = { 0xdeaddead };
static int staoff[] = { 0xdeaddead };
#define ADDR16 atomst16, staoff
#define ADDR32 atomst32, staoff
#define DATA16 atomst16, stdoff
#define DATA32 atomst32, stdoff
static struct matches *atomst16 APROTO {
	if (ctx->reverse)
		return 0;
	unsigned int *n = (unsigned int*)v;
	ull num = BF(8, 16);
	n[0] &= 0xffff0000;
	n[0] |= num;
	struct expr *expr = makeex(EXPR_NUM);
	expr->num1 = n[0];
	RNN_ADDARRAY(ctx->atoms, expr);
}
static struct matches *atomst32 APROTO {
	if (ctx->reverse)
		return 0;
	unsigned int *n = (unsigned int*)v;
	ull num = BF(8, 32);
	n[0] = num;
	struct expr *expr = makeex(EXPR_NUM);
	expr->num1 = num;
	RNN_ADDARRAY(ctx->atoms, expr);
}

static struct bitfield maskoff = { 8, 8 };
#define CRTCMASK atomimm, &maskoff
static struct bitfield flagoff = { 0, 5 };
#define FLAG atomimm, &flagoff
static struct bitfield waitoff = { 0, 2 };
#define WAIT atomimm, &waitoff
static struct bitfield waitsoff = { 2, 4 };
#define WAITS atomimm, &waitsoff

static struct insn tabcrtcevent[] = {
	{ 0x000000, 0xff0000, N("active") },
	{ 0x010000, 0xff0000, N("vblank") },
	{ 0, 0, OOPS },
};

static struct insn tabfl[] = {
	{ 0x10, 0x1f, N("buspause") },
	{ 0, 0, FLAG },
};

static struct insn tabm[] = {
	{ 0x00, 0xff, OP8, N("nop") },
	{ 0x00, 0xc0, OP8, N("wait"), WAIT, N("shl"), WAITS },
	{ 0x40, 0xff, OP24, N("addr"), ADDR16, .vartype = PMS_NV50 },
	{ 0x42, 0xff, OP24, N("data"), DATA16, .vartype = PMS_NV50 },
	{ 0x5f, 0xff, OP24, N("crtcwait"), CRTCMASK, T(crtcevent), .vartype = PMS_NV41 | PMS_NV50 },
	{ 0x7f, 0xff, OP8, N("exit") },
	{ 0x80, 0xe0, OP8, N("unset"), T(fl) }, 
	{ 0xa0, 0xe0, OP8, N("set1"), T(fl) }, 
	{ 0xc0, 0xe0, OP8, N("set0"), T(fl) }, 
	{ 0xe0, 0xff, OP40, N("addr"), ADDR32, .vartype = PMS_NV50 },
	{ 0xe2, 0xff, OP40, N("data"), DATA32, .vartype = PMS_NV50 },
	{ 0, 0, OP8, OOPS },
};

static struct disisa hwsq_isa_s = {
	tabm,
	5,
	1,
	1,
};

struct disisa *hwsq_isa = &hwsq_isa_s;
