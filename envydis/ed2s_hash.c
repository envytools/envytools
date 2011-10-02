#include "ed2s.h"

uint32_t ed2s_elf_hash(const char *str) {
	const unsigned char *ustr = (const unsigned char *)str;
	uint32_t h = 0;
	while (*ustr) {
		h <<= 4;
		h += *ustr++;
		h ^= (h >> 24) & 0xf0;
		h &= 0xfffffff;
	}
	return h;
}
