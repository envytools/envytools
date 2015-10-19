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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "h262.h"
#include "vstream.h"
#include <stdio.h>
#include <stdlib.h>

static const struct vs_vlc_val mbai_vlc[] = {
	{  0,  1, 1 },
	{  1,  3, 0,1,1 },
	{  2,  3, 0,1,0 },
	{  3,  4, 0,0,1,1 },
	{  4,  4, 0,0,1,0 },
	{  5,  5, 0,0,0,1,1 },
	{  6,  5, 0,0,0,1,0 },
	{  7,  7, 0,0,0,0,1,1,1 },
	{  8,  7, 0,0,0,0,1,1,0 },
	{  9,  8, 0,0,0,0,1,0,1,1 },
	{ 10,  8, 0,0,0,0,1,0,1,0 },
	{ 11,  8, 0,0,0,0,1,0,0,1 },
	{ 12,  8, 0,0,0,0,1,0,0,0 },
	{ 13,  8, 0,0,0,0,0,1,1,1 },
	{ 14,  8, 0,0,0,0,0,1,1,0 },
	{ 15, 10, 0,0,0,0,0,1,0,1,1,1 },
	{ 16, 10, 0,0,0,0,0,1,0,1,1,0 },
	{ 17, 10, 0,0,0,0,0,1,0,1,0,1 },
	{ 18, 10, 0,0,0,0,0,1,0,1,0,0 },
	{ 19, 10, 0,0,0,0,0,1,0,0,1,1 },
	{ 20, 10, 0,0,0,0,0,1,0,0,1,0 },
	{ 21, 11, 0,0,0,0,0,1,0,0,0,1,1 },
	{ 22, 11, 0,0,0,0,0,1,0,0,0,1,0 },
	{ 23, 11, 0,0,0,0,0,1,0,0,0,0,1 },
	{ 24, 11, 0,0,0,0,0,1,0,0,0,0,0 },
	{ 25, 11, 0,0,0,0,0,0,1,1,1,1,1 },
	{ 26, 11, 0,0,0,0,0,0,1,1,1,1,0 },
	{ 27, 11, 0,0,0,0,0,0,1,1,1,0,1 },
	{ 28, 11, 0,0,0,0,0,0,1,1,1,0,0 },
	{ 29, 11, 0,0,0,0,0,0,1,1,0,1,1 },
	{ 30, 11, 0,0,0,0,0,0,1,1,0,1,0 },
	{ 31, 11, 0,0,0,0,0,0,1,1,0,0,1 },
	{ 32, 11, 0,0,0,0,0,0,1,1,0,0,0 },
	{ 33, 11, 0,0,0,0,0,0,0,1,0,0,0 },	/* escape */
	{ 34, 11, 0,0,0,0,0,0,0,1,1,1,1 },	/* stuffing */
	{ 0 },
};

int h262_mb_addr_inc(struct bitstream *str, uint32_t *val) {
	if (str->dir == VS_ENCODE) {
		uint32_t tmp = *val, c33 = 33;
		while (tmp > 32) {
			if (vs_vlc(str, &c33, mbai_vlc)) return 1;
			tmp -= 33;
		}
		if (vs_vlc(str, &tmp, mbai_vlc)) return 1;
	} else {
		*val = 0;
		uint32_t tmp;
		while (1) {
			if (vs_vlc(str, &tmp, mbai_vlc)) return 1;
			if (tmp == 33) {
				*val += 33;
			} else if (tmp == 34) {
				/* stuffing */
			} else {
				*val += tmp;
				break;
			}
		}
	}
	return 0;
}

static const struct vs_vlc_val motion_code_vlc[] = {
	{   0,  1, 1 },
	{   1,  3, 0,1,0 },
	{  -1,  3, 0,1,1 },
	{   2,  4, 0,0,1,0 },
	{  -2,  4, 0,0,1,1 },
	{   3,  5, 0,0,0,1,0 },
	{  -3,  5, 0,0,0,1,1 },
	{   4,  7, 0,0,0,0,1,1,0 },
	{  -4,  7, 0,0,0,0,1,1,1 },
	{   5,  8, 0,0,0,0,1,0,1,0 },
	{  -5,  8, 0,0,0,0,1,0,1,1 },
	{   6,  8, 0,0,0,0,1,0,0,0 },
	{  -6,  8, 0,0,0,0,1,0,0,1 },
	{   7,  8, 0,0,0,0,0,1,1,0 },
	{  -7,  8, 0,0,0,0,0,1,1,1 },
	{   8, 10, 0,0,0,0,0,1,0,1,1,0 },
	{  -8, 10, 0,0,0,0,0,1,0,1,1,1 },
	{   9, 10, 0,0,0,0,0,1,0,1,0,0 },
	{  -9, 10, 0,0,0,0,0,1,0,1,0,1 },
	{  10, 10, 0,0,0,0,0,1,0,0,1,0 },
	{ -10, 10, 0,0,0,0,0,1,0,0,1,1 },
	{  11, 11, 0,0,0,0,0,1,0,0,0,1,0 },
	{ -11, 11, 0,0,0,0,0,1,0,0,0,1,1 },
	{  12, 11, 0,0,0,0,0,1,0,0,0,0,0 },
	{ -12, 11, 0,0,0,0,0,1,0,0,0,0,1 },
	{  13, 11, 0,0,0,0,0,0,1,1,1,1,0 },
	{ -13, 11, 0,0,0,0,0,0,1,1,1,1,1 },
	{  14, 11, 0,0,0,0,0,0,1,1,1,0,0 },
	{ -14, 11, 0,0,0,0,0,0,1,1,1,0,1 },
	{  15, 11, 0,0,0,0,0,0,1,1,0,1,0 },
	{ -15, 11, 0,0,0,0,0,0,1,1,0,1,1 },
	{  16, 11, 0,0,0,0,0,0,1,1,0,0,0 },
	{ -16, 11, 0,0,0,0,0,0,1,1,0,0,1 },
	{ 0 },
};

static const struct vs_vlc_val dmvector_vlc[] = {
	{  0, 1, 0 },
	{  1, 2, 1,0 },
	{ -1, 2, 1,1 },
	{ 0 },
};

int h262_motion_vectors(struct bitstream *str, struct h262_seqparm *seqparm, struct h262_picparm *picparm, struct h262_macroblock *mb, int s) {
	int mvc, mfs, dmv;
	if (picparm->picture_structure == H262_PIC_STRUCT_FRAME) {
		switch (mb->frame_motion_type) {
			case H262_FRAME_MOTION_FIELD:
				mvc = 2;
				mfs = 1;
				dmv = 0;
				break;
			case H262_FRAME_MOTION_FRAME:
				mvc = 1;
				mfs = 0;
				dmv = 0;
				break;
			case H262_FRAME_MOTION_DUAL_PRIME:
				mvc = 1;
				mfs = 0;
				dmv = 1;
				break;
			default:
				abort();
		}
	} else {
		switch (mb->field_motion_type) {
			case H262_FIELD_MOTION_FIELD:
				mvc = 1;
				mfs = 1;
				dmv = 0;
				break;
			case H262_FIELD_MOTION_16X8:
				mvc = 2;
				mfs = 1;
				dmv = 0;
				break;
			case H262_FIELD_MOTION_DUAL_PRIME:
				mvc = 1;
				mfs = 0;
				dmv = 1;
				break;
			default:
				abort();
		}
	}
	int r, t;
	for (r = 0; r < 2; r++) {
		if (r < mvc) {
			if (mfs)
				if (vs_u(str, &mb->motion_vertical_field_select[r][s], 1)) return 1;
			for (t = 0; t < 2; t++) {
				if (vs_vlc(str, &mb->motion_code[r][s][t], motion_code_vlc)) return 1;
				if (mb->motion_code[r][s][t]) {
					if (vs_u(str, &mb->motion_residual[r][s][t], picparm->f_code[s][t]-1)) return 1;
				} else {
					if (vs_infer(str, &mb->motion_residual[r][s][t], 0)) return 1;
				}
				if (dmv) {
					if (vs_vlc(str, &mb->dmvector[t], dmvector_vlc)) return 1;
				}
			}
		} else {
			if (vs_infer(str, &mb->motion_code[r][s][0], 0)) return 1;
			if (vs_infer(str, &mb->motion_code[r][s][1], 0)) return 1;
			if (vs_infer(str, &mb->motion_residual[r][s][0], 0)) return 1;
			if (vs_infer(str, &mb->motion_residual[r][s][1], 0)) return 1;
		}
	}
	return 0;
}

int h262_infer_vectors(struct bitstream *str, struct h262_seqparm *seqparm, struct h262_picparm *picparm, struct h262_macroblock *mb, int s) {
	if (picparm->picture_structure != H262_PIC_STRUCT_FRAME) {
		int bottom = picparm->picture_structure == H262_PIC_STRUCT_FIELD_BOTTOM;
		if (vs_infer(str, &mb->motion_vertical_field_select[s][0], bottom)) return 1;
		if (vs_infer(str, &mb->motion_vertical_field_select[s][1], bottom)) return 1;
	}
	if (vs_infer(str, &mb->motion_code[0][s][0], 0)) return 1;
	if (vs_infer(str, &mb->motion_code[0][s][1], 0)) return 1;
	if (vs_infer(str, &mb->motion_code[1][s][0], 0)) return 1;
	if (vs_infer(str, &mb->motion_code[1][s][1], 0)) return 1;
	if (vs_infer(str, &mb->motion_residual[0][s][0], 0)) return 1;
	if (vs_infer(str, &mb->motion_residual[0][s][1], 0)) return 1;
	if (vs_infer(str, &mb->motion_residual[1][s][0], 0)) return 1;
	if (vs_infer(str, &mb->motion_residual[1][s][1], 0)) return 1;
	if (!s) {
		if (vs_infer(str, &mb->dmvector[0], 0)) return 1;
		if (vs_infer(str, &mb->dmvector[1], 0)) return 1;
	}
	return 0;
}

static const struct vs_vlc_val cbp_vlc[] = {
	{ 0x0f, 3, 1,1,1 },
	{ 0x08, 4, 1,1,0,1 },
	{ 0x04, 4, 1,1,0,0 },
	{ 0x02, 4, 1,0,1,1 },
	{ 0x01, 4, 1,0,1,0 },
	{ 0x0c, 5, 1,0,0,1,1 },
	{ 0x03, 5, 1,0,0,1,0 },
	{ 0x0a, 5, 1,0,0,0,1 },
	{ 0x05, 5, 1,0,0,0,0 },
	{ 0x0e, 5, 0,1,1,1,1 },
	{ 0x0d, 5, 0,1,1,1,0 },
	{ 0x0b, 5, 0,1,1,0,1 },
	{ 0x07, 5, 0,1,1,0,0 },
	{ 0x20, 5, 0,1,0,1,1 },
	{ 0x2f, 5, 0,1,0,1,0 },
	{ 0x10, 5, 0,1,0,0,1 },
	{ 0x1f, 5, 0,1,0,0,0 },
	{ 0x06, 6, 0,0,1,1,1,1 },
	{ 0x09, 6, 0,0,1,1,1,0 },
	{ 0x30, 6, 0,0,1,1,0,1 },
	{ 0x3f, 6, 0,0,1,1,0,0 },
	{ 0x28, 7, 0,0,1,0,1,1,1 },
	{ 0x24, 7, 0,0,1,0,1,1,0 },
	{ 0x22, 7, 0,0,1,0,1,0,1 },
	{ 0x21, 7, 0,0,1,0,1,0,0 },
	{ 0x18, 7, 0,0,1,0,0,1,1 },
	{ 0x14, 7, 0,0,1,0,0,1,0 },
	{ 0x12, 7, 0,0,1,0,0,0,1 },
	{ 0x11, 7, 0,0,1,0,0,0,0 },
	{ 0x38, 8, 0,0,0,1,1,1,1,1 },
	{ 0x34, 8, 0,0,0,1,1,1,1,0 },
	{ 0x32, 8, 0,0,0,1,1,1,0,1 },
	{ 0x31, 8, 0,0,0,1,1,1,0,0 },
	{ 0x2c, 8, 0,0,0,1,1,0,1,1 },
	{ 0x23, 8, 0,0,0,1,1,0,1,0 },
	{ 0x2a, 8, 0,0,0,1,1,0,0,1 },
	{ 0x25, 8, 0,0,0,1,1,0,0,0 },
	{ 0x1c, 8, 0,0,0,1,0,1,1,1 },
	{ 0x13, 8, 0,0,0,1,0,1,1,0 },
	{ 0x1a, 8, 0,0,0,1,0,1,0,1 },
	{ 0x15, 8, 0,0,0,1,0,1,0,0 },
	{ 0x3c, 8, 0,0,0,1,0,0,1,1 },
	{ 0x33, 8, 0,0,0,1,0,0,1,0 },
	{ 0x3a, 8, 0,0,0,1,0,0,0,1 },
	{ 0x35, 8, 0,0,0,1,0,0,0,0 },
	{ 0x26, 8, 0,0,0,0,1,1,1,1 },
	{ 0x29, 8, 0,0,0,0,1,1,1,0 },
	{ 0x16, 8, 0,0,0,0,1,1,0,1 },
	{ 0x19, 8, 0,0,0,0,1,1,0,0 },
	{ 0x2e, 8, 0,0,0,0,1,0,1,1 },
	{ 0x2d, 8, 0,0,0,0,1,0,1,0 },
	{ 0x2b, 8, 0,0,0,0,1,0,0,1 },
	{ 0x27, 8, 0,0,0,0,1,0,0,0 },
	{ 0x1e, 8, 0,0,0,0,0,1,1,1 },
	{ 0x1d, 8, 0,0,0,0,0,1,1,0 },
	{ 0x1b, 8, 0,0,0,0,0,1,0,1 },
	{ 0x17, 8, 0,0,0,0,0,1,0,0 },
	{ 0x3e, 9, 0,0,0,0,0,0,1,1,1 },
	{ 0x3d, 9, 0,0,0,0,0,0,1,1,0 },
	{ 0x3b, 9, 0,0,0,0,0,0,1,0,1 },
	{ 0x37, 9, 0,0,0,0,0,0,1,0,0 },
	{ 0x36, 9, 0,0,0,0,0,0,0,1,1 },
	{ 0x39, 9, 0,0,0,0,0,0,0,1,0 },
	{ 0x00, 9, 0,0,0,0,0,0,0,0,1 },
	{ 0 },
};

int h262_coded_block_pattern(struct bitstream *str, int chroma_format, uint32_t *cbp) {
	uint32_t cbplo = *cbp & 0x3f;
	int cbphs;
	int i;
	uint32_t val;
	switch (chroma_format) {
		case 1:
			cbphs = 0;
			break;
		case 2:
			cbphs = 2;
			break;
		case 3:
			cbphs = 6;
			break;
		default:
			fprintf(stderr, "Invalid chroma format\n");
			return 1;
	}
	if (vs_vlc(str, &cbplo, cbp_vlc)) return 1;
	val = cbplo;
	for (i = 0; i < cbphs; i++) {
		uint32_t tmp = *cbp >> i & 1;
		if (vs_u(str, &tmp, 1)) return 1;
		val |= tmp << i;
	}	
	if (str->dir == VS_DECODE)
		*cbp = val;
	return 0;
}

static const struct vs_vlc_val dcs_luma_vlc[] = {
	{  0,  3, 1,0,0 },
	{  1,  2, 0,0 },
	{  2,  2, 0,1 },
	{  3,  3, 1,0,1 },
	{  4,  3, 1,1,0 },
	{  5,  4, 1,1,1,0 },
	{  6,  5, 1,1,1,1,0 },
	{  7,  6, 1,1,1,1,1,0 },
	{  8,  7, 1,1,1,1,1,1,0 },
	{  9,  8, 1,1,1,1,1,1,1,0 },
	{ 10,  9, 1,1,1,1,1,1,1,1,0 },
	{ 11,  9, 1,1,1,1,1,1,1,1,1 },
	{ 0 },
};

static const struct vs_vlc_val dcs_chroma_vlc[] = {
	{  0,  2, 0,0 },
	{  1,  2, 0,1 },
	{  2,  2, 1,0 },
	{  3,  3, 1,1,0 },
	{  4,  4, 1,1,1,0 },
	{  5,  5, 1,1,1,1,0 },
	{  6,  6, 1,1,1,1,1,0 },
	{  7,  7, 1,1,1,1,1,1,0 },
	{  8,  8, 1,1,1,1,1,1,1,0 },
	{  9,  9, 1,1,1,1,1,1,1,1,0 },
	{ 10, 10, 1,1,1,1,1,1,1,1,1,0 },
	{ 11, 10, 1,1,1,1,1,1,1,1,1,1 },
	{ 0 },
};

static const struct vs_vlc_val block_vlc[] = {
	{ 0x00000,  2, 1,0 },
	{ 0x00001,  2, 1,1 },
	{ 0x00002,  4, 0,1,0,0 },
	{ 0x00003,  5, 0,0,1,0,1 },
	{ 0x00004,  7, 0,0,0,0,1,1,0 },
	{ 0x00005,  8, 0,0,1,0,0,1,1,0 },
	{ 0x00006,  8, 0,0,1,0,0,0,0,1 },
	{ 0x00007, 10, 0,0,0,0,0,0,1,0,1,0 },
	{ 0x00008, 12, 0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x00009, 12, 0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x0000a, 12, 0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x0000b, 12, 0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x0000c, 13, 0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x0000d, 13, 0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x0000e, 13, 0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x0000f, 13, 0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x00010, 14, 0,0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x00011, 14, 0,0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x00012, 14, 0,0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x00013, 14, 0,0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x00014, 14, 0,0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x00015, 14, 0,0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x00016, 14, 0,0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x00017, 14, 0,0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x00018, 14, 0,0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x00019, 14, 0,0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x0001a, 14, 0,0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x0001b, 14, 0,0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x0001c, 14, 0,0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x0001d, 14, 0,0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x0001e, 14, 0,0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x0001f, 14, 0,0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x00020, 15, 0,0,0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x00021, 15, 0,0,0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x00022, 15, 0,0,0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x00023, 15, 0,0,0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x00024, 15, 0,0,0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x00025, 15, 0,0,0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x00026, 15, 0,0,0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x00027, 15, 0,0,0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x00028, 15, 0,0,0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x01001,  3, 0,1,1 },
	{ 0x01002,  6, 0,0,0,1,1,0 },
	{ 0x01003,  8, 0,0,1,0,0,1,0,1 },
	{ 0x01004, 10, 0,0,0,0,0,0,1,1,0,0 },
	{ 0x01005, 12, 0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x01006, 13, 0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x01007, 13, 0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x01008, 15, 0,0,0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x01009, 15, 0,0,0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x0100a, 15, 0,0,0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x0100b, 15, 0,0,0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x0100c, 15, 0,0,0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x0100d, 15, 0,0,0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x0100e, 15, 0,0,0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x0100f, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x01010, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x01011, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x01012, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x02001,  4, 0,1,0,1 },
	{ 0x02002,  7, 0,0,0,0,1,0,0 },
	{ 0x02003, 10, 0,0,0,0,0,0,1,0,1,1 },
	{ 0x02004, 12, 0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x02005, 13, 0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x03001,  5, 0,0,1,1,1 },
	{ 0x03002,  8, 0,0,1,0,0,1,0,0 },
	{ 0x03003, 12, 0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x03004, 13, 0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x04001,  5, 0,0,1,1,0 },
	{ 0x04002, 10, 0,0,0,0,0,0,1,1,1,1 },
	{ 0x04003, 12, 0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x05001,  6, 0,0,0,1,1,1 },
	{ 0x05002, 10, 0,0,0,0,0,0,1,0,0,1 },
	{ 0x05003, 13, 0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x06001,  6, 0,0,0,1,0,1 },
	{ 0x06002, 12, 0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x06003, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x07001,  6, 0,0,0,1,0,0 },
	{ 0x07002, 12, 0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x08001,  7, 0,0,0,0,1,1,1 },
	{ 0x08002, 12, 0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x09001,  7, 0,0,0,0,1,0,1 },
	{ 0x09002, 13, 0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x0a001,  8, 0,0,1,0,0,1,1,1 },
	{ 0x0a002, 13, 0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x0b001,  8, 0,0,1,0,0,0,1,1 },
	{ 0x0b002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x0c001,  8, 0,0,1,0,0,0,1,0 },
	{ 0x0c002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x0d001,  8, 0,0,1,0,0,0,0,0 },
	{ 0x0d002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x0e001, 10, 0,0,0,0,0,0,1,1,1,0 },
	{ 0x0e002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x0f001, 10, 0,0,0,0,0,0,1,1,0,1 },
	{ 0x0f002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x10001, 10, 0,0,0,0,0,0,1,0,0,0 },
	{ 0x10002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x11001, 12, 0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x12001, 12, 0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x13001, 12, 0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x14001, 12, 0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x15001, 12, 0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x16001, 13, 0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x17001, 13, 0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x18001, 13, 0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x19001, 13, 0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x1a001, 13, 0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x1b001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x1c001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x1d001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x1e001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x1f001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0xfffff,  6, 0,0,0,0,0,1 },
	{ 0 },
};

static const struct vs_vlc_val block_0_vlc[] = {
	{ 0x00001,  1, 1 },
	{ 0x00002,  4, 0,1,0,0 },
	{ 0x00003,  5, 0,0,1,0,1 },
	{ 0x00004,  7, 0,0,0,0,1,1,0 },
	{ 0x00005,  8, 0,0,1,0,0,1,1,0 },
	{ 0x00006,  8, 0,0,1,0,0,0,0,1 },
	{ 0x00007, 10, 0,0,0,0,0,0,1,0,1,0 },
	{ 0x00008, 12, 0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x00009, 12, 0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x0000a, 12, 0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x0000b, 12, 0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x0000c, 13, 0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x0000d, 13, 0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x0000e, 13, 0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x0000f, 13, 0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x00010, 14, 0,0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x00011, 14, 0,0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x00012, 14, 0,0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x00013, 14, 0,0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x00014, 14, 0,0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x00015, 14, 0,0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x00016, 14, 0,0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x00017, 14, 0,0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x00018, 14, 0,0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x00019, 14, 0,0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x0001a, 14, 0,0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x0001b, 14, 0,0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x0001c, 14, 0,0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x0001d, 14, 0,0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x0001e, 14, 0,0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x0001f, 14, 0,0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x00020, 15, 0,0,0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x00021, 15, 0,0,0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x00022, 15, 0,0,0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x00023, 15, 0,0,0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x00024, 15, 0,0,0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x00025, 15, 0,0,0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x00026, 15, 0,0,0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x00027, 15, 0,0,0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x00028, 15, 0,0,0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x01001,  3, 0,1,1 },
	{ 0x01002,  6, 0,0,0,1,1,0 },
	{ 0x01003,  8, 0,0,1,0,0,1,0,1 },
	{ 0x01004, 10, 0,0,0,0,0,0,1,1,0,0 },
	{ 0x01005, 12, 0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x01006, 13, 0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x01007, 13, 0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x01008, 15, 0,0,0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x01009, 15, 0,0,0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x0100a, 15, 0,0,0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x0100b, 15, 0,0,0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x0100c, 15, 0,0,0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x0100d, 15, 0,0,0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x0100e, 15, 0,0,0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x0100f, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x01010, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x01011, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x01012, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x02001,  4, 0,1,0,1 },
	{ 0x02002,  7, 0,0,0,0,1,0,0 },
	{ 0x02003, 10, 0,0,0,0,0,0,1,0,1,1 },
	{ 0x02004, 12, 0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x02005, 13, 0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x03001,  5, 0,0,1,1,1 },
	{ 0x03002,  8, 0,0,1,0,0,1,0,0 },
	{ 0x03003, 12, 0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x03004, 13, 0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x04001,  5, 0,0,1,1,0 },
	{ 0x04002, 10, 0,0,0,0,0,0,1,1,1,1 },
	{ 0x04003, 12, 0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x05001,  6, 0,0,0,1,1,1 },
	{ 0x05002, 10, 0,0,0,0,0,0,1,0,0,1 },
	{ 0x05003, 13, 0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x06001,  6, 0,0,0,1,0,1 },
	{ 0x06002, 12, 0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x06003, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x07001,  6, 0,0,0,1,0,0 },
	{ 0x07002, 12, 0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x08001,  7, 0,0,0,0,1,1,1 },
	{ 0x08002, 12, 0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x09001,  7, 0,0,0,0,1,0,1 },
	{ 0x09002, 13, 0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x0a001,  8, 0,0,1,0,0,1,1,1 },
	{ 0x0a002, 13, 0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x0b001,  8, 0,0,1,0,0,0,1,1 },
	{ 0x0b002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x0c001,  8, 0,0,1,0,0,0,1,0 },
	{ 0x0c002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x0d001,  8, 0,0,1,0,0,0,0,0 },
	{ 0x0d002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x0e001, 10, 0,0,0,0,0,0,1,1,1,0 },
	{ 0x0e002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x0f001, 10, 0,0,0,0,0,0,1,1,0,1 },
	{ 0x0f002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x10001, 10, 0,0,0,0,0,0,1,0,0,0 },
	{ 0x10002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x11001, 12, 0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x12001, 12, 0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x13001, 12, 0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x14001, 12, 0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x15001, 12, 0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x16001, 13, 0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x17001, 13, 0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x18001, 13, 0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x19001, 13, 0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x1a001, 13, 0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x1b001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x1c001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x1d001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x1e001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x1f001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0xfffff,  6, 0,0,0,0,0,1 },
	{ 0 },
};

static const struct vs_vlc_val block_intra_vlc[] = {
	{ 0x00000,  4, 0,1,1,0 },
	{ 0x00001,  2, 1,0 },
	{ 0x00002,  3, 1,1,0 },
	{ 0x00003,  4, 0,1,1,1 },
	{ 0x00004,  5, 1,1,1,0,0 },
	{ 0x00005,  5, 1,1,1,0,1 },
	{ 0x00006,  6, 0,0,0,1,0,1 },
	{ 0x00007,  6, 0,0,0,1,0,0 },
	{ 0x00008,  7, 1,1,1,1,0,1,1 },
	{ 0x00009,  7, 1,1,1,1,1,0,0 },
	{ 0x0000a,  8, 0,0,1,0,0,0,1,1 },
	{ 0x0000b,  8, 0,0,1,0,0,0,1,0 },
	{ 0x0000c,  8, 1,1,1,1,1,0,1,0 },
	{ 0x0000d,  8, 1,1,1,1,1,0,1,1 },
	{ 0x0000e,  8, 1,1,1,1,1,1,1,0 },
	{ 0x0000f,  8, 1,1,1,1,1,1,1,1 },
	{ 0x00010, 14, 0,0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x00011, 14, 0,0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x00012, 14, 0,0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x00013, 14, 0,0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x00014, 14, 0,0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x00015, 14, 0,0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x00016, 14, 0,0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x00017, 14, 0,0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x00018, 14, 0,0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x00019, 14, 0,0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x0001a, 14, 0,0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x0001b, 14, 0,0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x0001c, 14, 0,0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x0001d, 14, 0,0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x0001e, 14, 0,0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x0001f, 14, 0,0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x00020, 15, 0,0,0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x00021, 15, 0,0,0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x00022, 15, 0,0,0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x00023, 15, 0,0,0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x00024, 15, 0,0,0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x00025, 15, 0,0,0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x00026, 15, 0,0,0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x00027, 15, 0,0,0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x00028, 15, 0,0,0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x01001,  3, 0,1,0 },
	{ 0x01002,  5, 0,0,1,1,0 },
	{ 0x01003,  7, 1,1,1,1,0,0,1 },
	{ 0x01004,  8, 0,0,1,0,0,1,1,1 },
	{ 0x01005,  8, 0,0,1,0,0,0,0,0 },
	{ 0x01006, 13, 0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x01007, 13, 0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x01008, 15, 0,0,0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x01009, 15, 0,0,0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x0100a, 15, 0,0,0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x0100b, 15, 0,0,0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x0100c, 15, 0,0,0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x0100d, 15, 0,0,0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x0100e, 15, 0,0,0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x0100f, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x01010, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x01011, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x01012, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x02001,  5, 0,0,1,0,1 },
	{ 0x02002,  7, 0,0,0,0,1,1,1 },
	{ 0x02003,  8, 1,1,1,1,1,1,0,0 },
	{ 0x02004, 10, 0,0,0,0,0,0,1,1,0,0 },
	{ 0x02005, 13, 0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x03001,  5, 0,0,1,1,1 },
	{ 0x03002,  8, 0,0,1,0,0,1,1,0 },
	{ 0x03003, 12, 0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x03004, 13, 0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x04001,  6, 0,0,0,1,1,0 },
	{ 0x04002,  8, 1,1,1,1,1,1,0,1 },
	{ 0x04003, 12, 0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x05001,  6, 0,0,0,1,1,1 },
	{ 0x05002,  9, 0,0,0,0,0,0,1,0,0 },
	{ 0x05003, 13, 0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x06001,  7, 0,0,0,0,1,1,0 },
	{ 0x06002, 12, 0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x06003, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x07001,  7, 0,0,0,0,1,0,0 },
	{ 0x07002, 12, 0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x08001,  7, 0,0,0,0,1,0,1 },
	{ 0x08002, 12, 0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x09001,  7, 1,1,1,1,0,0,0 },
	{ 0x09002, 13, 0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x0a001,  7, 1,1,1,1,0,1,0 },
	{ 0x0a002, 13, 0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x0b001,  8, 0,0,1,0,0,0,0,1 },
	{ 0x0b002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x0c001,  8, 0,0,1,0,0,1,0,1 },
	{ 0x0c002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x0d001,  8, 0,0,1,0,0,1,0,0 },
	{ 0x0d002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x0e001,  9, 0,0,0,0,0,0,1,0,1 },
	{ 0x0e002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x0f001,  9, 0,0,0,0,0,0,1,1,1 },
	{ 0x0f002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x10001, 10, 0,0,0,0,0,0,1,1,0,1 },
	{ 0x10002, 16, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x11001, 12, 0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x12001, 12, 0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x13001, 12, 0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x14001, 12, 0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x15001, 12, 0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x16001, 13, 0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x17001, 13, 0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x18001, 13, 0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x19001, 13, 0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x1a001, 13, 0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x1b001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x1c001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x1d001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x1e001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x1f001, 16, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0xfffff,  6, 0,0,0,0,0,1 },
	{ 0 },
};

int h262_block(struct bitstream *str, struct h262_seqparm *seqparm, struct h262_picparm *picparm, int32_t *block, int intra, int chroma) {
	int i = 0;
	if (intra) {
		uint32_t dcs;
		uint32_t dcd;
		if (str->dir == VS_ENCODE) {
			dcs = 0;
			while (abs(block[0]) >= (1 << dcs))
				dcs++;
			if (block[0] >= 0) {
				dcd = block[0];
			} else {
				dcd = block[0] + (1 << dcs) - 1;
			}
		}
		if (vs_vlc(str, &dcs, chroma?dcs_chroma_vlc:dcs_luma_vlc)) return 1;
		if (vs_u(str, &dcd, dcs)) return 1;
		if (str->dir == VS_DECODE) {
			if (!dcs) {
				block[0] = 0;
			} else {
				if (dcd >= (1 << (dcs - 1)))
					block[0] = dcd;
				else
					block[0] = dcd - ((1 << dcs) - 1);
			}
		}
		i = 1;
	}
	if (picparm->picture_coding_type == H262_PIC_TYPE_D)
		return 0;
	while (1) {
		uint32_t tmp, run, el, eb1, eb2, sign;
		int32_t coeff;
		const struct vs_vlc_val *tab;
		if (intra && picparm->intra_vlc_format)
			tab = block_intra_vlc;
		else if (i == 0)
			tab = block_0_vlc;
		else
			tab = block_vlc;
		if (str->dir == VS_ENCODE) {
			run = 0;
			while (i < 64 && !block[i])
				i++, run++;
			if (i == 64) {
				tmp = 0;
			} else {
				coeff = block[i];
				if (coeff <= -0x800 || coeff >= 0x800) {
					fprintf(stderr, "Coeff too large\n");
					return 1;
				}
				el = coeff & 0xfff;
				tmp = abs(coeff) | run << 12;
				sign = coeff < 0;
				int j;
				for (j = 0; tab[j].blen; j++) {
					if (tab[j].val == tmp) {
						break;
					}
				}
				if (tab[j].val != tmp)
					tmp = 0xfffff;
				if (tmp == 0xfffff && !seqparm->is_ext) {
					/* make MPEG1 escape codes */
					if (abs(coeff) < 127) {
						eb1 = coeff & 0xff;
					} else if (abs(coeff) < 255) {
						if (coeff < 0)
							eb1 = 0x80;
						else
							eb1 = 0;
						eb2 = coeff & 0xff;
					} else {
						fprintf(stderr, "Coeff too large\n");
						return 1;
					}
				}
				i++;
			}
		}
		if (vs_vlc(str, &tmp, tab)) return 1;
		if (tmp == 0) {
			/* end of block */
			if (str->dir == VS_DECODE) {
				while (i < 64)
					block[i++] = 0;
			}
			return 0;
		} else if (tmp == 0xfffff) {
			if (vs_u(str, &run, 6)) return 1;
			if (seqparm->is_ext) {
				/* MPEG2 escape */
				if (vs_u(str, &el, 12)) return 1;
				if (el & 0x800)
					coeff = el | -0x1000;
				else
					coeff = el;
				if (!(el & 0x7ff)) {
					fprintf(stderr, "Invalid escape code\n");
					return 1;
				}
			} else {
				/* MPEG1 escape */
				if (vs_u(str, &eb1, 8)) return 1;
				if (eb1 == 0) {
					if (vs_u(str, &eb2, 8)) return 1;
					if (eb2 < 0x80) {
						fprintf(stderr, "Invalid escape code\n");
						return 1;
					}
					coeff = eb2;
				} else if (eb1 == 0x80) {
					if (vs_u(str, &eb2, 8)) return 1;
					if (eb2 == 0 || eb2 > 0x80) {
						fprintf(stderr, "Invalid escape code\n");
						return 1;
					}
					coeff = eb2 | -0x100;
				} else {
					coeff = eb1;
					if (coeff > 0x80)
						coeff |= -0x100;
				}
			}
		} else {
			/* normal coefficient */
			if (vs_u(str, &sign, 1)) return 1;
			run = tmp >> 12;
			if (sign)
				coeff = -(tmp & 0xfff);
			else
				coeff = tmp & 0xfff;
		}
		if (str->dir == VS_DECODE) {
			if (i >= 64) {
				fprintf(stderr, "block overflow\n");
				return 1;
			}
			while (run--) {
				if (i >= 64) {
					fprintf(stderr, "block overflow\n");
					return 1;
				}
				block[i++] = 0;
			}
			block[i++] = coeff;
		}
	}
}

static const struct vs_vlc_val mbf_i_vlc[] = {
	{ 0x10, 1, 1 },		/* INTRA */
	{ 0x11, 2, 0,1 },	/* QUANT INTRA */
	{ 0 },
};

static const struct vs_vlc_val mbf_p_vlc[] = {
	{ 0x0a, 1, 1 },			/* FWD PATTERN */
	{ 0x08, 2, 0,1 },		/* PATTERN */
	{ 0x02, 3, 0,0,1 },		/* FWD */
	{ 0x10, 5, 0,0,0,1,1 },		/* INTRA */
	{ 0x0b, 5, 0,0,0,1,0 },		/* QUANT FWD PATTERN */
	{ 0x09, 5, 0,0,0,0,1 },		/* QUANT PATTERN */
	{ 0x11, 6, 0,0,0,0,0,1 },	/* QUANT INTRA */
	{ 0 },
};

static const struct vs_vlc_val mbf_b_vlc[] = {
	{ 0x0e, 2, 1,1 },		/* FWD BWD PATTERN */
	{ 0x06, 2, 1,0 },		/* FWD BWD */
	{ 0x0c, 3, 0,1,1 },		/* BWD PATTERN */
	{ 0x04, 3, 0,1,0 },		/* BWD */
	{ 0x0a, 4, 0,0,1,1 },		/* FWD PATTERN */
	{ 0x02, 4, 0,0,1,0 },		/* FWD */
	{ 0x10, 5, 0,0,0,1,1 },		/* INTRA */
	{ 0x0f, 5, 0,0,0,1,0 },		/* QUANT FWD BWD PATTERN */
	{ 0x0b, 6, 0,0,0,0,1,1 },	/* QUANT FWD PATTERN */
	{ 0x0d, 6, 0,0,0,0,1,0 },	/* QUANT BWD PATTERN */
	{ 0x11, 6, 0,0,0,0,0,1 },	/* QUANT INTRA */
	{ 0 },
};

static const struct vs_vlc_val mbf_d_vlc[] = {
	{ 0x10, 1, 1 },		/* INTRA */
	{ 0 },
};

int h262_macroblock(struct bitstream *str, struct h262_seqparm *seqparm, struct h262_picparm *picparm, struct h262_macroblock *mb, uint32_t *qsc) {
	uint32_t mb_flags = mb->macroblock_quant
		| mb->macroblock_motion_forward << 1
		| mb->macroblock_motion_backward << 2
		| mb->macroblock_pattern << 3
		| mb->macroblock_intra << 4;
	switch (picparm->picture_coding_type) {
		case H262_PIC_TYPE_I:
			if (vs_vlc(str, &mb_flags, mbf_i_vlc)) return 1;
			break;
		case H262_PIC_TYPE_P:
			if (vs_vlc(str, &mb_flags, mbf_p_vlc)) return 1;
			break;
		case H262_PIC_TYPE_B:
			if (vs_vlc(str, &mb_flags, mbf_b_vlc)) return 1;
			break;
		case H262_PIC_TYPE_D:
			if (vs_vlc(str, &mb_flags, mbf_d_vlc)) return 1;
			break;
		default:
			fprintf(stderr, "Invalid picture type\n");
			return 1;
	}
	if (str->dir == VS_DECODE) {
		mb->macroblock_skipped = 0;
		mb->macroblock_quant = mb_flags >> 0 & 1;
		mb->macroblock_motion_forward = mb_flags >> 1 & 1;
		mb->macroblock_motion_backward = mb_flags >> 2 & 1;
		mb->macroblock_pattern = mb_flags >> 3 & 1;
		mb->macroblock_intra = mb_flags >> 4 & 1;
	}
	if (mb->macroblock_motion_forward || mb->macroblock_motion_backward) {
		if (picparm->picture_structure == H262_PIC_STRUCT_FRAME) {
			if (picparm->frame_pred_frame_dct) {
				if (vs_infer(str, &mb->frame_motion_type, H262_FRAME_MOTION_FRAME)) return 1;
			} else {
				if (vs_u(str, &mb->frame_motion_type, 2)) return 1;
				if (!mb->frame_motion_type) {
					fprintf(stderr, "Invalid frame_motion_type\n");
					return 1;
				}
			}
		} else {
			if (vs_u(str, &mb->field_motion_type, 2)) return 1;
			if (!mb->field_motion_type) {
				fprintf(stderr, "Invalid field_motion_type\n");
				return 1;
			}
		}
	} else {
		if (picparm->picture_structure == H262_PIC_STRUCT_FRAME) {
			if (vs_infer(str, &mb->frame_motion_type, H262_FRAME_MOTION_FRAME)) return 1;
		} else {
			if (vs_infer(str, &mb->field_motion_type, H262_FIELD_MOTION_FIELD)) return 1;
		}
	}
	if (picparm->picture_structure == H262_PIC_STRUCT_FRAME && !picparm->frame_pred_frame_dct && (mb->macroblock_intra || mb->macroblock_pattern)) {
		if (vs_u(str, &mb->dct_type, 1)) return 1;
	} else {
		if (vs_infer(str, &mb->dct_type, 0)) return 1;
	}
	if (mb->macroblock_quant) {
		if (vs_u(str, &mb->quantiser_scale_code, 5)) return 1;
	} else {
		if (vs_infer(str, &mb->quantiser_scale_code, *qsc)) return 1;
	}
	*qsc = mb->quantiser_scale_code;
	if (mb->macroblock_motion_forward || (mb->macroblock_intra && picparm->concealment_motion_vectors)) {
		if (h262_motion_vectors(str, seqparm, picparm, mb, 0)) return 1;
	} else {
		if (h262_infer_vectors(str, seqparm, picparm, mb, 0)) return 1;
	}
	if (mb->macroblock_motion_backward) {
		if (h262_motion_vectors(str, seqparm, picparm, mb, 1)) return 1;
	} else {
		if (h262_infer_vectors(str, seqparm, picparm, mb, 1)) return 1;
	}
	if (mb->macroblock_intra && picparm->concealment_motion_vectors)
		if (vs_mark(str, 1, 1)) return 1;
	static const int block_count[4] = { 4, 6, 8, 12 };
	if (mb->macroblock_intra) {
		if (vs_infer(str, &mb->coded_block_pattern, (1 << block_count[seqparm->chroma_format]) - 1)) return 1;
	} else if (mb->macroblock_pattern) {
		if (h262_coded_block_pattern(str, seqparm->chroma_format, &mb->coded_block_pattern)) return 1;
	} else {
		if (vs_infer(str, &mb->coded_block_pattern, 0)) return 1;
	}
	int i;
	for (i = 0; i < block_count[seqparm->chroma_format]; i++)
		if (mb->coded_block_pattern & 1 << i) {
			if (h262_block(str, seqparm, picparm, mb->block[i], mb->macroblock_intra, i >= 4)) return 1;
		}
	if (picparm->picture_coding_type == H262_PIC_TYPE_D)
		if (vs_mark(str, 1, 1)) return 1;
	return 0;
}

int h262_slice(struct bitstream *str, struct h262_seqparm *seqparm, struct h262_picparm *picparm, struct h262_slice *slice) {
	if (vs_u(str, &slice->quantiser_scale_code, 5))
		return 1;
	if (vs_u(str, &slice->intra_slice_flag, 1)) return 1;
	if (slice->intra_slice_flag) {
		if (vs_u(str, &slice->intra_slice, 1)) return 1;
		uint32_t tmp = 0;
		if (vs_u(str, &tmp, 8)) return 1;
		while(tmp & 1) {
			if (vs_u(str, &tmp, 9)) return 1;
		}
	}
	uint32_t tmp = slice->first_mb_in_slice % picparm->pic_width_in_mbs;
	if (h262_mb_addr_inc(str, &tmp)) return 1;
	if (tmp >= picparm->pic_width_in_mbs) {
		fprintf(stderr, "Initial mb_addr_inc too large\n");
		return 1;
	}
	slice->first_mb_in_slice = slice->slice_vertical_position * picparm->pic_width_in_mbs + tmp;
	uint32_t qsc = slice->quantiser_scale_code;
	uint32_t curr_mb_addr = slice->first_mb_in_slice;
	while (1) {
		if (h262_macroblock(str, seqparm, picparm, &slice->mbs[curr_mb_addr], &qsc)) return 1;
		if (str->dir == VS_DECODE) {
			slice->last_mb_in_slice = curr_mb_addr;
			curr_mb_addr++;
			if (!vs_has_more_data(str))
				return 0;
			if (h262_mb_addr_inc(str, &tmp)) return 1;
			if (curr_mb_addr >= picparm->pic_size_in_mbs) {
				fprintf(stderr, "MB index overflow\n");
				return 1;
			}
			while (tmp) {
				slice->mbs[curr_mb_addr].macroblock_skipped = 1;
				slice->mbs[curr_mb_addr].macroblock_quant = 0;
				slice->mbs[curr_mb_addr].macroblock_motion_forward = 0;
				slice->mbs[curr_mb_addr].macroblock_motion_backward = 0;
				slice->mbs[curr_mb_addr].macroblock_pattern = 0;
				slice->mbs[curr_mb_addr].macroblock_intra = 0;
				if (h262_infer_vectors(str, seqparm, picparm, &slice->mbs[curr_mb_addr], 0)) return 1;
				if (h262_infer_vectors(str, seqparm, picparm, &slice->mbs[curr_mb_addr], 1)) return 1;
				curr_mb_addr++;
				if (curr_mb_addr >= picparm->pic_size_in_mbs) {
					fprintf(stderr, "MB index overflow\n");
					return 1;
				}
				tmp--;
			}
		} else {
			if (slice->last_mb_in_slice == curr_mb_addr) {
				return 0;
			}
			tmp = 0;
			while (slice->last_mb_in_slice != curr_mb_addr && slice->mbs[curr_mb_addr].macroblock_skipped) {
				tmp++;
				curr_mb_addr++;
			}
			if (slice->last_mb_in_slice == curr_mb_addr) {
				fprintf(stderr, "Last MB in slice is skipped\n");
				return 1;
			}
			if (h262_mb_addr_inc(str, &tmp)) return 1;
		}
	}
	return 0;
}

void h262_del_slice(struct h262_slice *slice) {
	free(slice->mbs);
	free(slice);
}
