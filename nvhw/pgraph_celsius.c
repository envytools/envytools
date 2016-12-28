/*
 * Copyright (C) 2016 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "nvhw/pgraph.h"

uint32_t pgraph_celsius_convert_light_v(uint32_t val) {
	if ((val & 0x3ffff) < 0x3fe00)
		val += 0x200;
	return val & 0xfffffc00;
}

uint32_t pgraph_celsius_convert_light_sx(uint32_t val) {
	if (!extr(val, 23, 8))
		return 0;
	if (extr(val, 23, 8) == 0xff) {
		if (extr(val, 9, 14))
			return 0x7ffffc00;
		return 0x7f800000;
	}
	if ((val & 0x3ffff) < 0x3fe00)
		val += 0x200;
	return val & 0xfffffc00;
}

uint32_t pgraph_celsius_ub_to_float(uint8_t val) {
	if (!val)
		return 0;
	if (val == 0xff)
		return 0x3f800000;
	uint32_t res = val * 0x010101;
	uint32_t exp = 0x7e;
	while (!extr(res, 23, 1))
		res <<= 1, exp--;
	insrt(res, 23, 8, exp);
	return res;
}

uint32_t pgraph_celsius_short_to_float(struct pgraph_state *state, int16_t val) {
	if (!val)
		return 0;
	if (state->chipset.chipset == 0x10 && val == -0x8000)
		return 0x80000000;
	bool sign = val < 0;
	uint32_t res = (sign ? -val : val) << 8;
	uint32_t exp = 0x7f + 15;
	while (!extr(res, 23, 1))
		res <<= 1, exp--;
	insrt(res, 23, 8, exp);
	insrt(res, 31, 1, sign);
	return res;
}

uint32_t pgraph_celsius_nshort_to_float(int16_t val) {
	bool sign = val < 0;
	if (val == -0x8000)
		return 0xbf800000;
	if (val == 0x7fff)
		return 0x3f800000;
	int32_t rv = val << 1 | 1;
	uint32_t res = (sign ? -rv : rv) * 0x10001;
	uint32_t exp = 0x7f - 9;
	while (extr(res, 24, 8))
		res >>= 1, exp++;
	while (!extr(res, 23, 1))
		res <<= 1, exp--;
	insrt(res, 23, 8, exp);
	insrt(res, 31, 1, sign);
	return res;
}
