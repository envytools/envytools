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


int h264_pred_weight_table_entry(struct bitstream *str, struct h264_pred_weight_table *table, struct h264_pred_weight_table_entry *entry) {
	int ret = 0;
	ret |= vs_u(str, &entry->luma_weight_flag, 1);
	if (entry->luma_weight_flag) {
		ret |= vs_se(str, &entry->luma_weight);
		ret |= vs_se(str, &entry->luma_offset);
	} else {
		ret |= vs_infer(str, &entry->luma_weight, 1 << table->luma_log2_weight_denom);
		ret |= vs_infer(str, &entry->luma_offset, 0);
	}
	ret |= vs_u(str, &entry->chroma_weight_flag, 1);
	if (entry->chroma_weight_flag) {
		ret |= vs_se(str, &entry->chroma_weight[0]);
		ret |= vs_se(str, &entry->chroma_offset[0]);
		ret |= vs_se(str, &entry->chroma_weight[1]);
		ret |= vs_se(str, &entry->chroma_offset[1]);
	} else {
		ret |= vs_infer(str, &entry->chroma_weight[0], 1 << table->chroma_log2_weight_denom);
		ret |= vs_infer(str, &entry->chroma_offset[0], 0);
		ret |= vs_infer(str, &entry->chroma_weight[1], 1 << table->chroma_log2_weight_denom);
		ret |= vs_infer(str, &entry->chroma_offset[1], 0);
	}
	return ret;
}

int h264_pred_weight_table(struct bitstream *str, struct h264_sliceparm *slp, struct h264_pred_weight_table *table) {
	int ret = 0;
	ret |= vs_ue(str, &table->luma_log2_weight_denom);
	ret |= vs_ue(str, &table->chroma_log2_weight_denom);
	int i;
	for (i = 0; i <= slp->num_ref_idx_l0_active_minus1; i++)
		ret |= h264_pred_weight_table_entry(str, table, &table->l0[i]);
	if (slp->slice_type % 5 == 1)
		for (i = 0; i <= slp->num_ref_idx_l1_active_minus1; i++)
			ret |= h264_pred_weight_table_entry(str, table, &table->l1[i]);
	return ret;
}
