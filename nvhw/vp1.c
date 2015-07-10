#include "nvhw.h"
#include "util.h"

int32_t vp1_mad_input(uint8_t val, bool fractint, bool sign) {
	if (!sign)
		return val;
	else if (fractint)
		return sext(val, 7);
	else
		return sext(val, 7) << 1;
}

int vp1_mad_shift(bool fractint, bool sign, int shift) {
	if (fractint)
		return 16 - shift;
	else if (!sign)
		return 8 - shift;
	else
		return 9 - shift;
}

int32_t vp1_mad(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, bool rnd, bool fractint, bool sign, int shift, bool hilo, bool down) {
	int32_t res = a;
	if (!fractint)
		res += b * c + d * e;
	else
		res += (b * c + d * e) << 8;
	if (rnd) {
		int rshift = vp1_mad_shift(fractint, sign, shift);
		if (hilo)
			rshift -= 8;
		if (rshift > 0) {
			res += 1 << (rshift - 1);
			if (down)
				res -= 1;
		}
	}
	return sext(res, 27);
}

uint8_t vp1_mad_read(int32_t val, bool fractint, bool sign, int shift, bool hilo) {
	int rshift = vp1_mad_shift(fractint, sign, shift) - 8;
	if (rshift >= 0)
		val >>= rshift;
	else
		val <<= -rshift;
	if (!sign) {
		if (val < 0)
			val = 0;
		if (val > 0xffff)
			val = 0xffff;
	} else {
		if (val < -0x8000)
			val = -0x8000;
		if (val > 0x7fff)
			val = 0x7fff;
	}
	if (!hilo)
		val >>= 8;
	return val;
}
