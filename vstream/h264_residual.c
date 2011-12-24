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

#include "h264.h"
#include "h264_cabac.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int h264_residual_cavlc(struct bitstream *str, struct h264_slice *slice, int16_t *block, int *num, int cat, int idx, int start, int end, int maxnumcoeff) {
	uint32_t total_coeff, trailing_ones;
	int i, j;
	int32_t tb[maxnumcoeff];
	uint32_t run[maxnumcoeff];
	int last;
	uint32_t total_zeros;
	if (str->dir == VS_ENCODE) {
		total_coeff = 0;
		trailing_ones = 0;
		for (i = 0; i < maxnumcoeff; i++)
			if (block[i]) {
				if (i < start || i > end) {
					fprintf(stderr, "Non-zero coord outside of start..end\n");
					return 1;
				}
				total_coeff++;
				if (abs(block[i]) == 1)
					trailing_ones++;
				else
					trailing_ones = 0;
				total_zeros = i + 1 - total_coeff - start;
			}
		last = start - 1;
		for (i = 0, j = total_coeff - 1; i < maxnumcoeff; i++) {
			if (block[i]) {
				run[j] = i - last - 1;
				tb[j] = block[i];
				last = i;
				j--;
			}
		}
		if (trailing_ones > 3)
			trailing_ones = 3;
	}
	if (h264_coeff_token(str, slice, cat, idx, &trailing_ones, &total_coeff)) return 1;
	if (num)
		*num = total_coeff;
	if (total_coeff) {
		int suffixLength;
		if (total_coeff > 10 && trailing_ones < 3)
			suffixLength = 1;
		else
			suffixLength = 0;
		for (i = 0; i < total_coeff; i++) {
			if (i < trailing_ones) {
				uint32_t s = tb[i] < 0;
				if (vs_u(str, &s, 1)) return 1;
				tb[i] = 1 - 2 * s;
			} else {
				int onebias = (i == trailing_ones && trailing_ones < 3);
				if (h264_level(str, suffixLength, onebias, &tb[i])) return 1;
				if (suffixLength == 0)
					suffixLength = 1;
				if (abs(tb[i]) > (3 << (suffixLength - 1)) && suffixLength < 6)
					suffixLength++;
			}
		}
		int mode;
		if (cat == H264_CTXBLOCKCAT_CHROMA_DC) {
			mode = slice->chroma_array_type;
		} else {
			mode = 0;
		}
		if (total_coeff < end - start + 1) {
			if (h264_total_zeros(str, mode, total_coeff, &total_zeros)) return 1;
		} else {
			if (vs_infer(str, &total_zeros, 0)) return 1;
		}
		int zerosLeft = total_zeros;
		for (i = 0; i < total_coeff - 1; i++) {
			if (h264_run_before(str, zerosLeft, &run[i])) return 1;
			zerosLeft -= run[i];
			if (zerosLeft < 0) {
				fprintf(stderr, "zerosLeft underflow\n");
				return 1;
			}
		}
		run[total_coeff-1] = zerosLeft;
	}
	if (str->dir == VS_DECODE) {
		for (i = 0; i < maxnumcoeff; i++)
			block[i] = 0;
		for (i = total_coeff - 1, j = -1; i >= 0; i--) {
			j += run[i] + 1;
			block[start + j] = tb[i];
		}
	}
	return 0;
}

int h264_residual_cabac(struct bitstream *str, struct h264_cabac_context *cabac, struct h264_slice *slice, struct h264_macroblock *mb, int16_t *block, int cat, int idx, int start, int end, int maxnumcoeff, int coded) {
	int i;
	uint32_t coded_block_flag = 0;
	for (i = 0 ; i < maxnumcoeff; i++)
		if (block[i])
			coded_block_flag = 1;
	if (coded) {
		if (maxnumcoeff != 64 || slice->chroma_array_type == 3) {
			if (h264_coded_block_flag(str, cabac, cat, idx, &coded_block_flag)) return 1;
		} else {
			if (vs_infer(str, &coded_block_flag, 1)) return 1;
		}
	} else {
		if (vs_infer(str, &coded_block_flag, 0)) return 1;
	}
	switch (cat) {
		case H264_CTXBLOCKCAT_LUMA_DC:
			mb->coded_block_flag[0][16] = coded_block_flag;
			break;
		case H264_CTXBLOCKCAT_LUMA_AC:
		case H264_CTXBLOCKCAT_LUMA_4X4:
			mb->coded_block_flag[0][idx] = coded_block_flag;
			break;
		case H264_CTXBLOCKCAT_LUMA_8X8:
			mb->coded_block_flag[0][idx * 4 + 0] = coded_block_flag;
			mb->coded_block_flag[0][idx * 4 + 1] = coded_block_flag;
			mb->coded_block_flag[0][idx * 4 + 2] = coded_block_flag;
			mb->coded_block_flag[0][idx * 4 + 3] = coded_block_flag;
			break;
		case H264_CTXBLOCKCAT_CB_DC:
			mb->coded_block_flag[1][16] = coded_block_flag;
			break;
		case H264_CTXBLOCKCAT_CB_AC:
		case H264_CTXBLOCKCAT_CB_4X4:
			mb->coded_block_flag[1][idx] = coded_block_flag;
			break;
		case H264_CTXBLOCKCAT_CB_8X8:
			mb->coded_block_flag[1][idx * 4 + 0] = coded_block_flag;
			mb->coded_block_flag[1][idx * 4 + 1] = coded_block_flag;
			mb->coded_block_flag[1][idx * 4 + 2] = coded_block_flag;
			mb->coded_block_flag[1][idx * 4 + 3] = coded_block_flag;
			break;
		case H264_CTXBLOCKCAT_CR_DC:
			mb->coded_block_flag[2][16] = coded_block_flag;
			break;
		case H264_CTXBLOCKCAT_CR_AC:
		case H264_CTXBLOCKCAT_CR_4X4:
			mb->coded_block_flag[2][idx] = coded_block_flag;
			break;
		case H264_CTXBLOCKCAT_CR_8X8:
			mb->coded_block_flag[2][idx * 4 + 0] = coded_block_flag;
			mb->coded_block_flag[2][idx * 4 + 1] = coded_block_flag;
			mb->coded_block_flag[2][idx * 4 + 2] = coded_block_flag;
			mb->coded_block_flag[2][idx * 4 + 3] = coded_block_flag;
			break;
		case H264_CTXBLOCKCAT_CHROMA_DC:
			mb->coded_block_flag[idx+1][16] = coded_block_flag;
			break;
		case H264_CTXBLOCKCAT_CHROMA_AC:
			mb->coded_block_flag[(idx >> 3)+1][idx & 7] = coded_block_flag;
			break;
		default:
			abort();
	}
	if (coded_block_flag) {
		int field = mb->mb_field_decoding_flag || cabac->slice->field_pic_flag;
		uint32_t significant_coeff_flag[64] = { 0 };
		uint32_t last_significant_coeff_flag[64] = { 0 };
		if (str->dir == VS_ENCODE) {
			int last = -1;
			for (i = 0; i < maxnumcoeff; i++) {
				if (block[i]) {
					if (i < start || i > end) {
						fprintf (stderr, "Non-zero coordinate outside start..end!\n");
						return 1;
					}
					significant_coeff_flag[i] = 1;
					last = i;
				} else {
					significant_coeff_flag[i] = 0;
				}
				last_significant_coeff_flag[i] = 0;
			}
			assert(last != -1);
			last_significant_coeff_flag[last] = 1;
		}
		int numcoeff = end + 1;
		for (i = start; i < numcoeff - 1; i++) {
			if (h264_significant_coeff_flag(str, cabac, field, cat, i, 0, &significant_coeff_flag[i])) return 1;
			if (significant_coeff_flag[i]) {
				if (h264_significant_coeff_flag(str, cabac, field, cat, i, 1, &last_significant_coeff_flag[i])) return 1;
				if (last_significant_coeff_flag[i]) {
					numcoeff = i + 1;
				}
			}
		}
		significant_coeff_flag[numcoeff-1] = 1;
		if (str->dir == VS_DECODE) {
			for (i = 0; i < maxnumcoeff; i++)
				block[i] = 0;
		}
		int num1 = 0, numgt1 = 0;
		for (i = numcoeff - 1; i >= start; i--) {
			if (significant_coeff_flag[i]) {
				int32_t cam1 = abs(block[i]) - 1;
				uint32_t s = block[i] < 0;
				if (h264_coeff_abs_level_minus1(str, cabac, cat, num1, numgt1, &cam1)) return 1;
				if (h264_cabac_bypass(str, cabac, &s)) return 1;
				if (cam1)
					numgt1++;
				else
					num1++;
				if (str->dir == VS_DECODE)
					block[i] = (s ? -(cam1 + 1) : cam1 + 1);
			}
		}
	} else {
		for (i = 0; i < maxnumcoeff; i++) {
			if (str->dir == VS_ENCODE) {
				if (block[i]) {
					fprintf(stderr, "Non-zero coordinate in a skipped block!\n");
					return 1;
				}
			} else {
				block[i] = 0;
			}
		}
	}
	return 0;
}

int h264_residual_block(struct bitstream *str, struct h264_cabac_context *cabac, struct h264_slice *slice, struct h264_macroblock *mb, int16_t *block, int *num, int cat, int idx, int start, int end, int maxnumcoeff, int coded) {
	if (!cabac) {
		if (!coded) {
			int i;
			for (i = 0; i < maxnumcoeff; i++) {
				if (str->dir == VS_ENCODE) {
					if (block[i]) {
						fprintf(stderr, "Non-zero coordinate in a skipped block!\n");
						return 1;
					}
				} else {
					block[i] = 0;
				}
			}
			if (num)
				*num = 0;
			return 0;
		} else {
			return h264_residual_cavlc(str, slice, block, num, cat, idx, start, end, maxnumcoeff);
		}
	} else {
		return h264_residual_cabac(str, cabac, slice, mb, block, cat, idx, start, end, maxnumcoeff, coded);
	}
}

int h264_residual_luma(struct bitstream *str, struct h264_cabac_context *cabac, struct h264_slice *slice, struct h264_macroblock *mb, int start, int end, int which) {
	static const int cattab[3][4] = {
		{ H264_CTXBLOCKCAT_LUMA_DC, H264_CTXBLOCKCAT_LUMA_AC, H264_CTXBLOCKCAT_LUMA_4X4, H264_CTXBLOCKCAT_LUMA_8X8 }, 
		{ H264_CTXBLOCKCAT_CB_DC, H264_CTXBLOCKCAT_CB_AC, H264_CTXBLOCKCAT_CB_4X4, H264_CTXBLOCKCAT_CB_8X8 }, 
		{ H264_CTXBLOCKCAT_CR_DC, H264_CTXBLOCKCAT_CR_AC, H264_CTXBLOCKCAT_CR_4X4, H264_CTXBLOCKCAT_CR_8X8 }, 
	};
	if (start == 0 && h264_is_intra_16x16_mb_type(mb->mb_type)) {
		if (h264_residual_block(str, cabac, slice, mb, mb->block_luma_dc[which], 0, cattab[which][0], 0, 0, 15, 16, 1)) return 1;
	} else {
		mb->coded_block_flag[which][16] = 0;
	}
	int i, j;
	int n = (h264_is_intra_16x16_mb_type(mb->mb_type) ? 15 : 16);
	int ss = (h264_is_intra_16x16_mb_type(mb->mb_type) ? (start?start-1:0) : start);
	int se = (h264_is_intra_16x16_mb_type(mb->mb_type) ? end - 1 : end);
	if (!mb->transform_size_8x8_flag || !cabac) {
		for (i = 0; i < 16; i++) {
			int16_t tmp[16];
			int cat;
			if (mb->transform_size_8x8_flag) {
				for (j = 0; j < 16; j++)
					tmp[j] = mb->block_luma_8x8[which][i >> 2][4 * j + (i & 3)];
				cat = cattab[which][3];
			} else if (h264_is_intra_16x16_mb_type(mb->mb_type)) {
				for (j = 0; j < 15; j++)
					tmp[j] = mb->block_luma_ac[which][i][j];
				cat = cattab[which][1];
			} else {
				for (j = 0; j < 16; j++)
					tmp[j] = mb->block_luma_4x4[which][i][j];
				cat = cattab[which][2];
			}
			if (h264_residual_block(str, cabac, slice, mb, tmp, &mb->total_coeff[which][i], cat, i, ss, se, n, mb->coded_block_pattern >> (i >> 2) & 1)) return 1;
			if (mb->transform_size_8x8_flag) {
				for (j = 0; j < 16; j++)
					mb->block_luma_8x8[which][i >> 2][4 * j + (i & 3)] = tmp[j];
			} else if (h264_is_intra_16x16_mb_type(mb->mb_type)) {
				for (j = 0; j < 15; j++)
					mb->block_luma_ac[which][i][j] = tmp[j];
			} else {
				for (j = 0; j < 16; j++)
					mb->block_luma_4x4[which][i][j] = tmp[j];
			}
		}
	} else {
		for (i = 0; i < 4; i++) {
			if (h264_residual_block(str, cabac, slice, mb, mb->block_luma_8x8[which][i], 0, cattab[which][3], i, 4*start, 4*end + 3, 64, mb->coded_block_pattern >> i & 1)) return 1;
		}
	}
	return 0;
}

int h264_residual(struct bitstream *str, struct h264_cabac_context *cabac, struct h264_slice *slice, struct h264_macroblock *mb, int start, int end) {
	if (h264_residual_luma(str, cabac, slice, mb, start, end, 0)) return 1;
	if (slice->chroma_array_type == 1 || slice->chroma_array_type == 2) {
		int i, j;
		for (i = 0; i < 2; i++) {
			if (h264_residual_block(str, cabac, slice, mb, mb->block_chroma_dc[i], 0, H264_CTXBLOCKCAT_CHROMA_DC, i, 0, 4 * slice->chroma_array_type - 1, 4 * slice->chroma_array_type, (mb->coded_block_pattern & 0x30) && start == 0)) return 1;
		}
		for (i = 0; i < 2; i++) {
			for (j = 0; j < 4 * slice->chroma_array_type; j++) {
				if (h264_residual_block(str, cabac, slice, mb, mb->block_chroma_ac[i][j], &mb->total_coeff[i+1][j], H264_CTXBLOCKCAT_CHROMA_AC, i * 8 + j, (start?start-1:0), end-1, 15, mb->coded_block_pattern & 0x20)) return 1;
			}
		}
	} else if (slice->chroma_array_type == 3) {
		if (h264_residual_luma(str, cabac, slice, mb, start, end, 1)) return 1;
		if (h264_residual_luma(str, cabac, slice, mb, start, end, 2)) return 1;
	}
	return 0;
}
