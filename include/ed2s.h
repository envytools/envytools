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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ED2S_H
#define ED2S_H

#include <stdint.h>

uint32_t ed2s_elf_hash(const char *str);

struct ed2s_sym {
	char *name;
	uint32_t hash;
	int hchain;
	int type;
	void *data;
	int idata;
};

struct ed2s_symtab {
	struct ed2s_sym *syms;
	int symsnum;
	int symsmax;
	int *buckets;
	int bucketsnum;
};

struct ed2s_symtab *ed2s_symtab_new();
void ed2s_symtab_del(struct ed2s_symtab *tab);
int ed2s_symtab_get(struct ed2s_symtab *tab, const char *name);
int ed2s_symtab_put(struct ed2s_symtab *tab, const char *name);
int ed2s_symtab_get_int(struct ed2s_symtab *tab, const char *name, int type);

#endif
