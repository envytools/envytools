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

#include "symtab.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

/* the last one is not a prime - elf hash only goes up to 2^28-1, no point in having more buckets */
static const int primes[] = { 127, 509, 2039, 8191, 32749, 131071, 524287, 2097143, 8388593, 33554393, 134217689, 268435456 };
static const int numprimes = sizeof primes / sizeof *primes;

struct symtab *symtab_new() {
	int i;
	struct symtab *res = calloc(sizeof *res, 1);
	res->bucketsnum = primes[0];
	res->buckets = malloc(sizeof *res->buckets * res->bucketsnum);
	for (i = 0; i < res->bucketsnum; i++)
		res->buckets[i] = -1;
	return res;
}

void symtab_del(struct symtab *tab) {
	int i;
	for (i = 0; i < tab->symsnum; i++)
		free(tab->syms[i].name);
	free(tab->buckets);
	free(tab->syms);
	free(tab);
}

int symtab_get(struct symtab *tab, const char *name, int *ptype, int *pdata) {
	int i = tab->buckets[elf_hash(name) % tab->bucketsnum];
	while (i != -1) {
		if (!strcmp(tab->syms[i].name, name)) {
			if (ptype)
				*ptype = tab->syms[i].type;
			if (pdata)
				*pdata = tab->syms[i].data;
			return i;
		}
		i = tab->syms[i].hchain;
	}
	return -1;
}

int symtab_get_t(struct symtab *tab, const char *name, int type, int *pdata) {
	int rtype;
	int res = symtab_get(tab, name, &rtype, pdata);
	if (res == -1)
		return res;
	if (rtype != type)
		return -1;
	return res;
}

int symtab_put(struct symtab *tab, const char *cname, int type, int data) {
	if (symtab_get(tab, cname, 0, 0) != -1)
		return -1;
	char *name = strdup(cname);
	struct symtab_sym sym;
	uint32_t hash = elf_hash(name);
	int bucket = hash % tab->bucketsnum;
	int res = tab->symsnum;
	sym.name = name;
	sym.type = type;
	sym.data = data;
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
