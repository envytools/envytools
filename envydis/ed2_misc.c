#include "ed2_misc.h"
#include <string.h>
#include <assert.h>

static int gethex(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 0xa;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 0xa;
	assert(0);
}

char *ed2_str_deescape(char *str, uint64_t *len) {
	int rlen = 0;
	int i;
	for (i = 0; str[i]; i++) {
		if (str[i] == '\\') {
			i++;
			rlen++;
			if (str[i] == 'x')
				i += 2;
		} else if (str[i] != '"') {
			rlen++;
		}
	}
	char *res = malloc (rlen + 1);
	int j;
	for (i = 0, j = 0; str[i]; i++) {
		if (str[i] == '\\') {
			i++;
			switch (str[i]) {
				case '\\':
				case '\'':
				case '\"':
				case '\?':
					res[j++] = str[i];
					break;
				case 'n':
					res[j++] = '\n';
					break;
				case 'f':
					res[j++] = '\f';
					break;
				case 't':
					res[j++] = '\t';
					break;
				case 'a':
					res[j++] = '\a';
					break;
				case 'v':
					res[j++] = '\v';
					break;
				case 'r':
					res[j++] = '\r';
					break;
				case 'x':
					res[j++] = gethex(str[i+1]) << 4 | gethex(str[i+2]);
					i += 2;
					break;
				default:
					assert(0);
			}
			if (str[i] == 'x')
				i += 2;
		} else if (str[i] != '"') {
			res[j++] = str[i];
		}
	}
	res[j] = 0;
	assert(j == rlen);
	*len = rlen;
	return res;
}
