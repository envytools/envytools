/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef VSTREAM_H
#define VSTREAM_H

#include <inttypes.h>

struct bitstream {
	enum vs_dir {
		VS_ENCODE,
		VS_DECODE,
	} dir;
	uint8_t *bytes;
	int bytesnum;
	int bytesmax;
	uint8_t curbyte;
	int bitpos;
	int bytepos;
	int zero_bytes;
	enum vs_type {
		VS_MPEG12,
		VS_H264,
	} type;
	int hasbyte;
};

enum vs_align_byte_mode {
	VS_ALIGN_0, /* 000...00 */
	VS_ALIGN_1, /* 111...11 */
	VS_ALIGN_10, /* 100...00 */
};

int vs_ue(struct bitstream *str, uint32_t *val);
int vs_se(struct bitstream *str, int32_t *val);
int vs_u(struct bitstream *str, uint32_t *val, int size);
int vs_start(struct bitstream *str, uint32_t *val);
int vs_align_byte(struct bitstream *str, enum vs_align_byte_mode mode);
int vs_end(struct bitstream *str);
int vs_infer(struct bitstream *str, uint32_t *val, uint32_t ival);

struct bitstream *vs_new_encode(enum vs_type type);
struct bitstream *vs_new_decode(enum vs_type type, uint8_t *bytes, int bytesnum);

#endif
