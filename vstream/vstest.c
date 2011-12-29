#include "vstream.h"
#include <stdio.h>

int main() {
	struct bitstream *str = vs_new_encode(VS_H264);
	uint32_t val;
	int32_t sval;
	val = 0xde;
	if (vs_start(str, &val))
		return 1;
	val = 0x123;
	if (vs_u(str, &val, 12))
		return 1;
	val = 0x456;
	if (vs_u(str, &val, 12))
		return 1;
	val = 0x8;
	if (vs_ue(str, &val))
		return 1;
	val = 0x7;
	if (vs_ue(str, &val))
		return 1;
	val = 0x6;
	if (vs_ue(str, &val))
		return 1;
	sval = 0;
	if (vs_se(str, &sval))
		return 1;
	sval = -3;
	if (vs_se(str, &sval))
		return 1;
	sval = 3;
	if (vs_se(str, &sval))
		return 1;
	if (vs_end(str))
		return 1;
	fwrite(str->bytes, str->bytesnum, 1, stdout);
	struct bitstream *nstr = vs_new_decode(VS_H264, str->bytes, str->bytesnum);
	if (vs_start(nstr, &val))
		return 1;
	if (val != 0xde) {
		fprintf (stderr, "Fail 1\n");
		return 1;
	}

	if (vs_u(nstr, &val, 12))
		return 1;
	if (val != 0x123) {
		fprintf (stderr, "Fail 2: %x\n", val);
		return 1;
	}

	if (vs_u(nstr, &val, 12))
		return 1;
	if (val != 0x456) {
		fprintf (stderr, "Fail 3: %x\n", val);
		return 1;
	}

	if (vs_ue(nstr, &val))
		return 1;
	if (val != 0x8) {
		fprintf (stderr, "Fail 4: %x\n", val);
		return 1;
	}

	if (vs_ue(nstr, &val))
		return 1;
	if (val != 0x7) {
		fprintf (stderr, "Fail 5: %x\n", val);
		return 1;
	}

	if (vs_ue(nstr, &val))
		return 1;
	if (val != 0x6) {
		fprintf (stderr, "Fail 6: %x\n", val);
		return 1;
	}

	if (vs_se(nstr, &sval))
		return 1;
	if (sval != 0) {
		fprintf (stderr, "Fail 7: %x\n", sval);
		return 1;
	}

	if (vs_se(nstr, &sval))
		return 1;
	if (sval != -3) {
		fprintf (stderr, "Fail 8: %x\n", sval);
		return 1;
	}

	if (vs_se(nstr, &sval))
		return 1;
	if (sval != 3) {
		fprintf (stderr, "Fail 9: %x\n", sval);
		return 1;
	}

	if (vs_end(nstr))
		return 1;
	if (nstr->bytepos != nstr->bytesnum || nstr->hasbyte) {
		fprintf (stderr, "Bitstream not fully consumed!\n");
		return 1;
	}
	fprintf (stderr, "All ok!\n");

	return 0;
}
