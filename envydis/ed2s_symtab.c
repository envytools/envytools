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

#include "ed2s.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

/* the last one is not a prime - elf hash only goes up to 2^28-1, no point in having more buckets */
static const int primes[] = { 127, 509, 2039, 8191, 32749, 131071, 524287, 2097143, 8388593, 33554393, 134217689, 268435456 };
static const int numprimes = sizeof primes / sizeof *primes;

struct ed2s_symtab *ed2s_symtab_new() {
	int i;
	struct ed2s_symtab *res = calloc(sizeof *res, 1);
	res->bucketsnum = primes[0];
	res->buckets = malloc(sizeof *res->buckets * res->bucketsnum);
	for (i = 0; i < res->bucketsnum; i++)
		res->buckets[i] = -1;
	return res;
}

void ed2s_symtab_del(struct ed2s_symtab *tab) {
	int i;
	for (i = 0; i < tab->symsnum; i++)
		free(tab->syms[i].name);
	free(tab->buckets);
	free(tab->syms);
	free(tab);
}

int ed2s_symtab_get(struct ed2s_symtab *tab, const char *name) {
	int i = tab->buckets[ed2s_elf_hash(name) % tab->bucketsnum];
	while (i != -1) {
		if (!strcmp(tab->syms[i].name, name))
			return i;
		i = tab->syms[i].hchain;
	}
	return -1;
}

int ed2s_symtab_put(struct ed2s_symtab *tab, const char *cname) {
	if (ed2s_symtab_get(tab, cname) != -1)
		return -1;
	char *name = strdup(cname);
	struct ed2s_sym sym;
	uint32_t hash = ed2s_elf_hash(name);
	int bucket = hash % tab->bucketsnum;
	int res = tab->symsnum;
	sym.name = name;
	sym.type = -1;
	sym.idata = -1;
	sym.data = 0;
	sym.hash = hash;
	sym.hchain = tab->buckets[bucket];
	tab->buckets[bucket] = res;
	ADDARRAY(tab->syms, sym);
	if (tab->symsnum > tab->bucketsnum && tab->bucketsnum != primes[numprimes-1]) {
		/* rehash */
		int i;
		free(tab->buckets);
		for (i = 0; i < numprimes-1; i++)
			if (tab->symsnum <= primes[i])
				break;
		tab->bucketsnum = primes[i];
		tab->buckets = malloc(tab->bucketsnum * sizeof *tab->buckets);
		for (i = 0; i < tab->bucketsnum; i++)
			tab->buckets[i] = -1;
		for (i = 0; i < tab->symsnum; i++) {
			bucket = tab->syms[i].hash % tab->bucketsnum;
			tab->syms[i].hchain = tab->buckets[bucket];
			tab->buckets[bucket] = i;
		}
	}
	return res;
}

int ed2s_symtab_get_int(struct ed2s_symtab *tab, const char *name, int type) {
	int idx = ed2s_symtab_get(tab, name);
	if (idx == -1 || tab->syms[idx].type != type)
		return -1;
	return tab->syms[idx].idata;
}
