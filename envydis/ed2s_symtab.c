#include "ed2s.h"
#include <stdlib.h>

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

int ed2s_symtab_put(struct ed2s_symtab *tab, char *name) {
	if (ed2_symtab_get(tab, name) != -1)
		return -1;
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
