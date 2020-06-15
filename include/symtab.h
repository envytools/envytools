/*
 * Copyright (C) 2011 Marcelina Kościelnicka <mwk@0x04.net>
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

#ifndef SYMTAB_H
#define SYMTAB_H

#include <inttypes.h>

struct symtab_sym {
	char *name;
	uint32_t hash;
	int hchain;
	int type;
	int data;
};

struct symtab {
	struct symtab_sym *syms;
	int symsnum;
	int symsmax;
	int *buckets;
	int bucketsnum;
};

struct symtab *symtab_new();
void symtab_del(struct symtab *tab);
int symtab_get(struct symtab *tab, const char *name, int *ptype, int *pdata);
int symtab_get_t(struct symtab *tab, const char *name, int type, int *pdata);
int symtab_put(struct symtab *tab, const char *name, int type, int data);
static inline int symtab_get_td(struct symtab *tab, const char *name, int type) {
	int res;
	if (symtab_get_t(tab, name, type, &res) == -1)
		return -1;
	return res;
}

#endif
