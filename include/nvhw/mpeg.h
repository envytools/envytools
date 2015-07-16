/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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

#ifndef NVHW_MPEG_H
#define NVHW_MPEG_H

#include <stdint.h>

struct mpeg_crypt_state {
	uint32_t lfsra;
	uint32_t lfsrb;
	uint16_t const_key;
	uint16_t block_key;
};

extern const uint8_t mpeg_crypt_bitrev[0x40];

uint8_t mpeg_crypt_host_hash(uint16_t host_key, uint8_t host_sel);
uint8_t mpeg_crypt_sess_hash(uint16_t host_key, uint16_t mpeg_key);
int mpeg_crypt_init(struct mpeg_crypt_state *state, uint32_t host, uint32_t mpeg, uint16_t frame_key);
void mpeg_crypt_advance(struct mpeg_crypt_state *state);

#endif
