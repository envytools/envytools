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

#ifndef H264_CABAC_H
#define H264_CABAC_H

#include "h264.h"
#include <inttypes.h>

struct h264_cabac_ctx_init {
	int8_t m;
	int8_t n;
};

enum h264_cabac_ctxidx {
	H264_CABAC_CTXIDX_MB_TYPE_SI = 0,
	H264_CABAC_CTXIDX_MB_TYPE_I = 3, /* also SI suffix */
	H264_CABAC_CTXIDX_MB_SKIP_FLAG_P = 11, /* and SP */
	H264_CABAC_CTXIDX_MB_TYPE_P = 14,
	H264_CABAC_CTXIDX_MB_TYPE_P_SUF = 17,
	H264_CABAC_CTXIDX_SUB_MB_TYPE_P = 21,
	H264_CABAC_CTXIDX_MB_SKIP_FLAG_B = 24,
	H264_CABAC_CTXIDX_MB_TYPE_B = 27,
	H264_CABAC_CTXIDX_MB_TYPE_B_SUF = 32,
	H264_CABAC_CTXIDX_SUB_MB_TYPE_B = 36,
	H264_CABAC_CTXIDX_MVD_X = 40,
	H264_CABAC_CTXIDX_MVD_Y = 47,
	H264_CABAC_CTXIDX_REF_IDX = 54,
	H264_CABAC_CTXIDX_MB_QP_DELTA = 60,
	H264_CABAC_CTXIDX_INTRA_CHROMA_PRED_MODE = 64,
	H264_CABAC_CTXIDX_PREV_INTRA_PRED_MODE_FLAG = 68,
	H264_CABAC_CTXIDX_REM_INTRA_PRED_MODE = 69,
	H264_CABAC_CTXIDX_MB_FIELD_DECODING_FLAG = 70,
	H264_CABAC_CTXIDX_CODED_BLOCK_PATTERN_LUMA = 73,
	H264_CABAC_CTXIDX_CODED_BLOCK_PATTERN_CHROMA = 77,
	H264_CABAC_CTXIDX_CODED_BLOCK_FLAG_CAT_LT5 = 85,
	/* XXX: complete me */
	/* 85... */
	H264_CABAC_CTXIDX_TERMINATE = 276, /* special */
	/* 277... */
	H264_CABAC_CTXIDX_TRANSFORM_SIZE_8X8_FLAG = 399,
	/* 402... */
	H264_CABAC_CTXIDX_NUM = 1024,
};

enum h264_cabac_se {
	/* slice data */
	H264_CABAC_SE_MB_SKIP_FLAG_P,
	H264_CABAC_SE_MB_SKIP_FLAG_B,
	H264_CABAC_SE_MB_FIELD_DECODING_FLAG,
	H264_CABAC_SE_END_OF_SLICE_FLAG,
	/* macroblock layer */
	H264_CABAC_SE_MB_TYPE_I,
	H264_CABAC_SE_MB_TYPE_SI,
	H264_CABAC_SE_MB_TYPE_P, /* and SP */
	H264_CABAC_SE_MB_TYPE_B,
	H264_CABAC_SE_CODED_BLOCK_PATTERN,
	H264_CABAC_SE_TRANSFORM_SIZE_8X8_FLAG,
	H264_CABAC_SE_MB_QP_DELTA,
	/* macroblock and sub-macroblock prediction */
	H264_CABAC_SE_PREV_INTRA_PRED_MODE_FLAG,
	H264_CABAC_SE_REM_INTRA_PRED_MODE,
	H264_CABAC_SE_INTRA_CHROMA_PRED_MODE,
	H264_CABAC_SE_REF_IDX,
	H264_CABAC_SE_MVD_X,
	H264_CABAC_SE_MVD_Y,
	H264_CABAC_SE_SUB_MB_TYPE_P,
	H264_CABAC_SE_SUB_MB_TYPE_B,
	/* residual block */
	H264_CABAC_SE_CODED_BLOCK_FLAG,
	H264_CABAC_SE_SIGNIFICANT_COEFF_FLAG,
	H264_CABAC_SE_LAST_SIGNIFICANT_COEFF_FLAG,
	H264_CABAC_SE_COEFF_ABS_LEVEL_MINUS1,
	H264_CABAC_SE_COEFF_SIGN_FLAG,
};

struct h264_cabac_context {
	struct h264_sliceparm *slp;
	uint8_t pStateIdx[H264_CABAC_CTXIDX_NUM];
	uint8_t valMPS[H264_CABAC_CTXIDX_NUM];
	uint32_t codIOffset; /* and codILow */
	uint32_t codIRange;
	int firstBitFlag;
	int bitsOutstanding;
	int BinCount;
};

struct h264_cabac_context *h264_cabac_new(struct h264_sliceparm *slp);
int h264_cabac_init_arith(struct bitstream *str, struct h264_cabac_context *cabac);
int h264_cabac_renorm(struct bitstream *str, struct h264_cabac_context *cabac);
int h264_cabac_decision(struct bitstream *str, struct h264_cabac_context *cabac, int ctxIdx, int *binVal);
int h264_cabac_bypass(struct bitstream *str, struct h264_cabac_context *cabac, int *binVal);
int h264_cabac_terminate(struct bitstream *str, struct h264_cabac_context *cabac, int *binVal);
void h264_cabac_destroy(struct h264_cabac_context *cabac);

#endif
