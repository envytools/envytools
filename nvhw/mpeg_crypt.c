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
	uint8_t res = 0;
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

static const struct mpeg_crypt_tkey {
	uint8_t selector;
	uint32_t key;
} tkeys[] = {
	{ 0x55, 0xc480cc39 },
	{ 0xff, 0x444c4991 },

	{ 0x89, 0x38d13bf2 },
	{ 0x23, 0xf160787d },

	{ 0x33, 0x6a7c3b83 },
	{ 0x99, 0x19c832fd },

	{ 0xeb, 0x4e998d9f },
	{ 0x41, 0xb5bde57b },

	{ 0xef, 0xc1c42917 },
	{ 0x45, 0xf8253852 },

	{ 0x2b, 0x41d0a606 },
	{ 0x81, 0xd59dc21a },

	{ 0x8b, 0x989517a1 },
	{ 0x21, 0x508a4265 },

	{ 0x4b, 0x7553d94c },
	{ 0xe1, 0x5a83df4f },
};

int mpeg_crypt_init(struct mpeg_crypt_state *state, uint32_t host, uint32_t mpeg, uint16_t frame_key) {
	uint16_t host_key = host & 0xffff;
	uint16_t mpeg_key = mpeg & 0xffff;
	uint8_t selector = mpeg >> 16 & 0xff;

	int i;
	int ntkeys = sizeof tkeys / sizeof *tkeys;
	for (i = 0; i < ntkeys; i++)
		if (selector == tkeys[i].selector)
			break;
	if (i == ntkeys)
		return -1;
	uint32_t tkey = tkeys[i].key;

	uint32_t const_a = tkey >> 16 & mpeg_key >> 8 & 0xff;
	uint32_t const_b = mpeg_key & tkey >> 26 & 0x3f;
	uint32_t const_c = (tkey >> 24 | host_key) << 8 & 0xf00;
	uint32_t const_d = host_key & 0xfc0;
	state->const_key = const_a ^ const_b ^ const_c ^ const_d;

	uint32_t lfsr_key = frame_key << 16 ^ host_key << 8 ^ mpeg_key;
	state->lfsra = lfsr_key ^ (tkey & 0xff) << 12;
	state->lfsrb = lfsr_key ^ (tkey & 0xff00) << 4;

	mpeg_crypt_advance(state);
	return 0;	
}

static void advance_lfsr(uint32_t *lfsr, uint32_t poly) {
	int bits, xbits;
	for (bits = xbits = 0; xbits + 3 >= 2 * bits; bits++) {
		if (*lfsr & 0x00110000)
			xbits++;
		if (*lfsr & 1) {
			*lfsr >>= 1;
			*lfsr ^= poly;
		} else {
			*lfsr >>= 1;
		}
	}
}

void mpeg_crypt_advance(struct mpeg_crypt_state *state) {
	advance_lfsr(&state->lfsra, 0x8001fff9);
	advance_lfsr(&state->lfsrb, 0x8f0f0f0e);
	state->block_key = state->const_key ^ (state->lfsra & 0xfff) ^ (state->lfsrb & 0x3f);
}

const uint8_t mpeg_crypt_bitrev[0x40] = {
	0x00, 0x20, 0x10, 0x30,
	0x08, 0x28, 0x18, 0x38,
	0x04, 0x24, 0x14, 0x34,
	0x0c, 0x2c, 0x1c, 0x3c,
	0x02, 0x22, 0x12, 0x32,
	0x0a, 0x2a, 0x1a, 0x3a,
	0x06, 0x26, 0x16, 0x36,
	0x0e, 0x2e, 0x1e, 0x3e,
	0x01, 0x21, 0x11, 0x31,
	0x09, 0x29, 0x19, 0x39,
	0x05, 0x25, 0x15, 0x35,
	0x0d, 0x2d, 0x1d, 0x3d,
	0x03, 0x23, 0x13, 0x33,
	0x0b, 0x2b, 0x1b, 0x3b,
	0x07, 0x27, 0x17, 0x37,
	0x0f, 0x2f, 0x1f, 0x3f,
};
