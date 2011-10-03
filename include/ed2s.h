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
int ed2s_symtab_put(struct ed2s_symtab *tab, char *name);

#endif
