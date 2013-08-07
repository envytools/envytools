/*
 * Copyright (C) 2013 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "nvhw.h"

uint8_t mpeg_crypt_host_hash(uint16_t host_key, uint8_t host_sel) {
	int kbits[16];
	int obits[8];
	int sbits[3];
	int i;
	for (i = 0; i < 16; i++)
		kbits[i] = host_key >> i & 1;
	for (i = 0; i < 3; i++)
		sbits[i] = host_sel >> i & 1;
	obits[0] = (kbits[3] | kbits[6]) ^ kbits[13] ^ kbits[14] ^ sbits[0];
	obits[1] = (kbits[1] | kbits[5]) ^ kbits[11] ^ kbits[15] ^ sbits[1] ^ 1;
	obits[2] = (kbits[2] | kbits[4]) ^ kbits[14] ^ kbits[15] ^ sbits[2];

	obits[3] = (kbits[0] | kbits[7]) ^ kbits[11] ^ kbits[13];
	obits[4] = (kbits[8] & kbits[10]) ^ kbits[4] ^ 1;

	obits[5] = kbits[2] ^ (kbits[7] & (kbits[12] | sbits[2]));
	obits[6] = kbits[1] ^ (kbits[5] & (kbits[9] | sbits[1])) ^ 1;
	obits[7] = kbits[0] ^ (kbits[3] & (kbits[6] | sbits[0]));
	uint8_t res = 0;
	for (i = 0; i < 8; i++)
		res |= obits[i] << i;
	return res;
}

uint8_t mpeg_crypt_sess_hash(uint16_t host_key, uint16_t mpeg_key) {
	int tab[8][4] = {
		{ 15,  0, 15, 10 },
		{ 13, 14,  4,  9 },
		{ 11, 12,  1,  2 },
		{  9, 10,  5,  8 },
		{  7,  8, 14, 11 },
		{  5,  6,  0,  3 },
		{  3,  4,  7,  6 },
		{  1,  2, 12, 13 },
	};
	int i;
	uint8_t res;
	for (i = 0; i < 8; i++) {
		int which = host_key >> tab[i][0] & 1;
		int flip = host_key >> tab[i][1] & 1;
		int bit = mpeg_key >> tab[i][2+which] & 1;
		bit ^= which;
		bit ^= flip;
		bit ^= i & 1;
		res |= bit << i;
	}
	return res;
}
