/*
 * Copyright (C) 2009-2012 Marcelina Kościelnicka <mwk@0x04.net>
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

#ifndef DIS_H
#define DIS_H

#include "util.h"
#include "var.h"
#include "colors.h"
#include <inttypes.h>
#include <stdio.h>

struct disisa {
	struct insn *troot;
	int maxoplen;
	int opunit;
	int posunit;
	int i_need_g80as_hack;
	int prepdone;
	void (*prep)(struct disisa *);
	struct vardata *vardata;
	uint32_t (*getcbsz)(const struct disisa *isa, struct varinfo *varinfo);
	struct insn *trootas;
	struct insn *tsched;
	int schedpos;
};

struct label {
	const char *name;
	unsigned long long val;
	int type;
	unsigned size;
};

const struct disisa *ed_getisa(const char *name);
void ed_freeisa(const struct disisa *isa);

uint32_t ed_getcbsz(const struct disisa *isa, struct varinfo *varinfo);

static inline int ed_getcstride(const struct disisa *isa, struct varinfo *varinfo) {
	return CEILDIV(ed_getcbsz(isa, varinfo), 8);
}

void envydis (const struct disisa *isa, FILE *out, uint8_t *code, uint32_t start, int num, struct varinfo *varinfo, int quiet, struct label *labels, int labelsnum, const struct envy_colors *cols);

#endif
