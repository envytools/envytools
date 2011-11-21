#include "vstream.h"
#include <stdio.h>

int main() {
	struct bitstream *str = vs_new_encode(VS_H264);
	uint32_t val;
	val = 0xde;
	vs_start(str, &val);
	val = 0x123;
	vs_u(str, &val, 12);
	val = 0x456;
	vs_u(str, &val, 12);
	vs_end(str);
	fwrite(str->bytes, str->bytesnum, 1, stdout);
	return 0;
}
