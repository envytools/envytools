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

#include "h264_cabac.h"
#include <stdlib.h>
#include <stdio.h>

static struct h264_cabac_ctx_init h264_cabac_ctx_init[H264_CABAC_CTXIDX_NUM][4] = {
	/* XXX: fill me */
};

static uint8_t rangeTabLPS[64][4] = {
	{ 128, 176, 208, 240 },
	{ 128, 167, 197, 227 },
	{ 128, 158, 187, 216 },
	{ 123, 150, 178, 205 },
	{ 116, 142, 169, 195 },
	{ 111, 135, 160, 185 },
	{ 105, 128, 152, 175 },
	{ 100, 122, 144, 166 },
	{ 95, 116, 137, 158 },
	{ 90, 110, 130, 150 },
	{ 85, 104, 123, 142 },
	{ 81, 99, 117, 135 },
	{ 77, 94, 111, 128 },
	{ 73, 89, 105, 122 },
	{ 69, 85, 100, 116 },
	{ 66, 80, 95, 110 },
	{ 62, 76, 90, 104 },
	{ 59, 72, 86, 99 },
	{ 56, 69, 81, 94 },
	{ 53, 65, 77, 89 },
	{ 51, 62, 73, 85 },
	{ 48, 59, 69, 80 },
	{ 46, 56, 66, 76 },
	{ 43, 53, 63, 72 },
	{ 41, 50, 59, 69 },
	{ 39, 48, 56, 65 },
	{ 37, 45, 54, 62 },
	{ 35, 43, 51, 59 },
	{ 33, 41, 48, 56 },
	{ 32, 39, 46, 53 },
	{ 30, 37, 43, 50 },
	{ 29, 35, 41, 48 },
	{ 27, 33, 39, 45 },
	{ 26, 31, 37, 43 },
	{ 24, 30, 35, 41 },
	{ 23, 28, 33, 39 },
	{ 22, 27, 32, 37 },
	{ 21, 26, 30, 35 },
	{ 20, 24, 29, 33 },
	{ 19, 23, 27, 31 },
	{ 18, 22, 26, 30 },
	{ 17, 21, 25, 28 },
	{ 16, 20, 23, 27 },
	{ 15, 19, 22, 25 },
	{ 14, 18, 21, 24 },
	{ 14, 17, 20, 23 },
	{ 13, 16, 19, 22 },
	{ 12, 15, 18, 21 },
	{ 12, 14, 17, 20 },
	{ 11, 14, 16, 19 },
	{ 11, 13, 15, 18 },
	{ 10, 12, 15, 17 },
	{ 10, 12, 14, 16 },
	{ 9, 11, 13, 15 },
	{ 9, 11, 12, 14 },
	{ 8, 10, 12, 14 },
	{ 8, 9, 11, 13 },
	{ 7, 9, 11, 12 },
	{ 7, 9, 10, 12 },
	{ 7, 8, 10, 11 },
	{ 6, 8, 9, 11 },
	{ 6, 7, 9, 10 },
	{ 6, 7, 8, 9 },
	{ 2, 2, 2, 2 },
};

static uint8_t transIdxLPS[64] = {
	0, 0, 1, 2, 2, 4, 4, 5, 6, 7, 8, 9, 9, 11, 11, 12,
	13, 13, 15, 15, 16, 16, 18, 18, 19, 19, 21, 21, 22, 22, 23, 24,
	24, 25, 26, 26, 27, 27, 28, 29, 29, 30, 30, 30, 31, 32, 32, 33,
	33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 63,
};

static uint8_t transIdxMPS[64] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 62, 63,
};

struct h264_cabac_context *h264_cabac_new(struct h264_slice *slice) {
	struct h264_cabac_context *cabac = calloc (sizeof *cabac, 1);
	/* XXX: write me */
	return cabac;
}

int h264_cabac_init_arith (struct bitstream *str, struct h264_cabac_context *cabac) {
	if (!cabac)
		return 0;
	if (str->bitpos != 7) {
		fprintf (stderr, "Trying to init CABAC when not byte aligned\n");
		return 1;
	}
	if (str->dir == VS_ENCODE) {
		cabac->codIRange = 510;
		cabac->codIOffset = 0;
		cabac->firstBitFlag = 1;
		cabac->bitsOutstanding = 0;
	} else {
		cabac->codIRange = 510;
		if (vs_u(str, &cabac->codIOffset, 9))
			return 1;
		if (cabac->codIOffset >= 510) {
			fprintf (stderr, "Initial codIOffset >= 510\n");
			return 1;
		}
	}
}

static int put_bit(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t bit) {
	uint32_t nbit = !bit;
	if (cabac->firstBitFlag) {
		cabac->firstBitFlag = 0;
		if (bit != 0) {
			fprintf (stderr, "CABAC initial skipped bit not 0\n");
			return 1;
		}
	} else {
		if (vs_u(str, &bit, 1))
			return 1;
	}
	while (cabac->bitsOutstanding--) {
		if (vs_u(str, &nbit, 1))
			return 1;
	}
	return 0;
}

int h264_cabac_renorm(struct bitstream *str, struct h264_cabac_context *cabac) {
	while (cabac->codIRange < 256) {
		cabac->codIRange <<= 1;
		cabac->codIOffset <<= 1;
		if (str->dir == VS_ENCODE) {
			if (cabac->codIOffset < 512) {
				if (put_bit(str, cabac, 0))
					return 1;
			} else if (cabac->codIOffset >= 1024) {
				if (put_bit(str, cabac, 1))
					return 1;
				cabac->codIOffset -= 1024;
			} else {
				cabac->bitsOutstanding++;
				cabac->codIOffset -= 512;
			}
		} else {
			uint32_t tmp;
			if (vs_u(str, &tmp, 1))
				return 1;
			cabac->codIOffset |= tmp;
		}
	}
	return 0;
}

int h264_cabac_decision(struct bitstream *str, struct h264_cabac_context *cabac, int ctxIdx, int *binVal) {
	int qCodIRangeIdx = cabac->codIRange >> 6 & 3;
	int codIRangeLPS = rangeTabLPS[cabac->pStateIdx[ctxIdx]][qCodIRangeIdx];
	cabac->codIRange -= codIRangeLPS;
	if (str->dir == VS_ENCODE) {
		if (*binVal != cabac->valMPS[ctxIdx]) {
			cabac->codIOffset += cabac->codIRange;
			cabac->codIRange = codIRangeLPS;
		}
	} else {
		if (cabac->codIOffset >= cabac->codIRange) {
			*binVal = !cabac->valMPS[ctxIdx];
			cabac->codIOffset -= cabac->codIRange;
			cabac->codIRange = codIRangeLPS;
		} else {
			*binVal = cabac->valMPS[ctxIdx];
		}
	}
	if (*binVal == cabac->valMPS[ctxIdx]) {
		cabac->pStateIdx[ctxIdx] = transIdxMPS[cabac->pStateIdx[ctxIdx]];
	} else {
		if (cabac->pStateIdx[ctxIdx] == 0)
			cabac->valMPS[ctxIdx] = !cabac->valMPS[ctxIdx];
		cabac->pStateIdx[ctxIdx] = transIdxLPS[cabac->pStateIdx[ctxIdx]];
	}
	if (h264_cabac_renorm(str, cabac))
		return 1;
	cabac->BinCount++;
	return 0;
}

int h264_cabac_bypass(struct bitstream *str, struct h264_cabac_context *cabac, int *binVal) {
	cabac->codIOffset <<= 1;
	if (str->dir == VS_ENCODE) {
		if (*binVal)
			cabac->codIOffset += cabac->codIRange;
		if (cabac->codIOffset < 512) {
			if (put_bit(str, cabac, 0))
				return 1;
		} else if (cabac->codIOffset >= 1024) {
			if (put_bit(str, cabac, 1))
				return 1;
			cabac->codIOffset -= 1024;
		} else {
			cabac->bitsOutstanding++;
			cabac->codIOffset -= 512;
		}
	} else {
		uint32_t tmp;
		if (vs_u(str, &tmp, 1))
			return 1;
		cabac->codIOffset |= tmp;
		if (cabac->codIOffset >= cabac->codIRange) {
			*binVal = 1;
			cabac->codIOffset -= cabac->codIRange;
		} else {
			*binVal = 0;
		}
	}
	cabac->BinCount++;
	return 0;
}

int h264_cabac_terminate(struct bitstream *str, struct h264_cabac_context *cabac, int *binVal) {
	cabac->codIRange -= 2;
	if (str->dir == VS_ENCODE) {
		if (*binVal) {
			cabac->codIOffset += cabac->codIRange;
			/* end of the road */
			cabac->codIRange = 2;
			if (h264_cabac_renorm(str, cabac))
				return 1;
			if (put_bit(str, cabac, cabac->codIOffset >> 9 & 1))
				return 1;
			if (put_bit(str, cabac, cabac->codIOffset >> 8 & 1))
				return 1;
			if (put_bit(str, cabac, 1)) /* the last bit, doubling as RBSP terminator if terminating due to end of slice */
				return 1;
		} else {
			if (h264_cabac_renorm(str, cabac))
				return 1;
		}
	} else {
		if (cabac->codIOffset >= cabac->codIRange) {
			*binVal = 1;
			if (!(cabac->codIOffset & 1)) {
				fprintf (stderr, "Last CABAC bit not 1\n");
				return 1;
			}
		} else {
			*binVal = 0;
			if (h264_cabac_renorm(str, cabac))
				return 1;
		}
	}
	cabac->BinCount++;
	return 0;
}

void h264_cabac_destroy(struct h264_cabac_context *cabac) {
	free(cabac);
}
