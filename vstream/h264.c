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
#include <stdio.h>
#include <stdlib.h>

void h264_del_seqparm(struct h264_seqparm *seqparm) {
	free(seqparm->vui);
	free(seqparm);
}

int h264_scaling_list(struct bitstream *str, uint32_t *scaling_list, int size, uint32_t *use_default_flag) {
	uint32_t lastScale = 8;
	uint32_t nextScale = 8;
	int32_t delta_scale;
	int i;
	if (str->dir == VS_DECODE) {
		for (i = 0; i < size; i++) {
			if (nextScale != 0) {
				if (vs_se(str, &delta_scale)) return 1;
				nextScale = (lastScale + delta_scale + 256)%256;
				*use_default_flag = (i == 0 && nextScale == 0);
			}
			lastScale = scaling_list[i] = (nextScale?nextScale:lastScale);
		}
	} else {
		int haltidx = size;
		if (*use_default_flag)
			haltidx = 0;
		while (haltidx >= 2 && scaling_list[haltidx-1] == scaling_list[haltidx-2])
			haltidx--;
		for (i = 0; i < haltidx; i++) {
			delta_scale = (scaling_list[i] - lastScale + 256)%256;
			if (delta_scale >= 128)
				delta_scale -= 256;
			if (vs_se(str, &delta_scale)) return 1;
			lastScale = scaling_list[i];
		}
		if (i != size) {
			delta_scale = (0 - lastScale + 256)%256;
			if (delta_scale >= 128)
				delta_scale -= 256;
			if (vs_se(str, &delta_scale)) return 1;
		}
	}
	return 0;
}

int h264_seqparm(struct bitstream *str, struct h264_seqparm *seqparm) {
	int i;
	if (vs_u(str, &seqparm->profile_idc, 8)) return 1;
	if (vs_u(str, &seqparm->constraint_set, 8)) return 1;
	if (vs_u(str, &seqparm->level_idc, 8)) return 1;
	if (vs_ue(str, &seqparm->seq_parameter_set_id)) return 1;
	switch (seqparm->profile_idc) {
		case H264_PROFILE_BASELINE:
		case H264_PROFILE_MAIN:
		case H264_PROFILE_EXTENDED:
			if (vs_infer(str, &seqparm->chroma_format_idc, 1)) return 1;
			if (vs_infer(str, &seqparm->separate_colour_plane_flag, 0)) return 1;
			if (vs_infer(str, &seqparm->bit_depth_luma_minus8, 0)) return 1;
			if (vs_infer(str, &seqparm->bit_depth_chroma_minus8, 0)) return 1;
			if (vs_infer(str, &seqparm->qpprime_y_zero_transform_bypass_flag, 0)) return 1;
			if (vs_infer(str, &seqparm->seq_scaling_matrix_present_flag, 0)) return 1;
			break;
		case H264_PROFILE_HIGH:
		case H264_PROFILE_HIGH_10:
		case H264_PROFILE_HIGH_422:
		case H264_PROFILE_HIGH_444_PRED:
		case H264_PROFILE_CAVLC_444:
		case H264_PROFILE_SCALABLE_BASELINE:
		case H264_PROFILE_SCALABLE_HIGH:
		case H264_PROFILE_MULTIVIEW_HIGH:
		case H264_PROFILE_STEREO_HIGH:
			if (vs_ue(str, &seqparm->chroma_format_idc)) return 1;
			if (seqparm->chroma_format_idc == 3) {
				if (vs_u(str, &seqparm->separate_colour_plane_flag, 1)) return 1;
			} else {
				if (vs_infer(str, &seqparm->separate_colour_plane_flag, 0)) return 1;
			}
			if (vs_ue(str, &seqparm->bit_depth_luma_minus8)) return 1;
			if (vs_ue(str, &seqparm->bit_depth_chroma_minus8)) return 1;
			if (vs_u(str, &seqparm->qpprime_y_zero_transform_bypass_flag, 1)) return 1;
			if (vs_u(str, &seqparm->seq_scaling_matrix_present_flag, 1)) return 1;
			if (seqparm->seq_scaling_matrix_present_flag) {
				for (i = 0; i < (seqparm->chroma_format_idc == 3 ? 12 : 8); i++) {
					if (vs_u(str, &seqparm->seq_scaling_list_present_flag[i], 1)) return 1;
					if (seqparm->seq_scaling_list_present_flag[i]) {
						if (i < 6) {
							if (h264_scaling_list(str, seqparm->seq_scaling_list_4x4[i], 16, &seqparm->use_default_scaling_matrix_flag[i])) return 1;;
						} else {
							if (h264_scaling_list(str, seqparm->seq_scaling_list_8x8[i-6], 64, &seqparm->use_default_scaling_matrix_flag[i])) return 1;;
						}
					}
				}
			}
			break;
		default:
			fprintf (stderr, "Unknown profile\n");
			return 1;
	}
	if (vs_ue(str, &seqparm->log2_max_frame_num_minus4)) return 1;
	if (vs_ue(str, &seqparm->pic_order_cnt_type)) return 1;
	switch (seqparm->pic_order_cnt_type) {
		case 0:
			if (vs_ue(str, &seqparm->log2_max_pic_order_cnt_lsb_minus4)) return 1;
			break;
		case 1:
			if (vs_u(str, &seqparm->delta_pic_order_always_zero_flag, 1)) return 1;
			if (vs_se(str, &seqparm->offset_for_non_ref_pic)) return 1;
			if (vs_se(str, &seqparm->offset_for_top_to_bottom_field)) return 1;
			if (vs_ue(str, &seqparm->num_ref_frames_in_pic_order_cnt_cycle)) return 1;
			for (i = 0; i < seqparm->num_ref_frames_in_pic_order_cnt_cycle; i++) {
				if (vs_se(str, &seqparm->offset_for_ref_frame[i])) return 1;
			}
			break;
	}
	if (vs_ue(str, &seqparm->max_num_ref_frames)) return 1;
	if (vs_u(str, &seqparm->gaps_in_frame_num_value_allowed_flag, 1)) return 1;
	if (vs_ue(str, &seqparm->pic_width_in_mbs_minus1)) return 1;
	if (vs_ue(str, &seqparm->pic_height_in_map_units_minus1)) return 1;
	if (vs_u(str, &seqparm->frame_mbs_only_flag, 1)) return 1;
	if (!seqparm->frame_mbs_only_flag) {
		if (vs_u(str, &seqparm->mb_adaptive_frame_field_flag, 1)) return 1;
	} else {
		if (vs_infer(str, &seqparm->mb_adaptive_frame_field_flag, 0)) return 1;
	}
	if (vs_u(str, &seqparm->direct_8x8_inference_flag, 1)) return 1;
	if (vs_u(str, &seqparm->frame_cropping_flag, 1)) return 1;
	if (seqparm->frame_cropping_flag) {
		if (vs_ue(str, &seqparm->frame_crop_left_offset)) return 1;
		if (vs_ue(str, &seqparm->frame_crop_right_offset)) return 1;
		if (vs_ue(str, &seqparm->frame_crop_top_offset)) return 1;
		if (vs_ue(str, &seqparm->frame_crop_bottom_offset)) return 1;
	} else {
		if (vs_infer(str, &seqparm->frame_crop_left_offset, 0)) return 1;
		if (vs_infer(str, &seqparm->frame_crop_right_offset, 0)) return 1;
		if (vs_infer(str, &seqparm->frame_crop_top_offset, 0)) return 1;
		if (vs_infer(str, &seqparm->frame_crop_bottom_offset, 0)) return 1;
	}
	uint32_t vui_parameters_present_flag = !!(seqparm->vui);
	if (vs_u(str, &vui_parameters_present_flag, 1)) return 1;
	if (vui_parameters_present_flag) {
		if (str->dir == VS_DECODE) {
			seqparm->vui = calloc (sizeof *seqparm->vui, 1);
		}
		/* XXX: implement me */
		abort();
	} else {
		seqparm->vui = 0;
	}
	return 0;
}

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

int h264_pred_weight_table(struct bitstream *str, struct h264_slice *slice, struct h264_pred_weight_table *table) {
	int ret = 0;
	ret |= vs_ue(str, &table->luma_log2_weight_denom);
	ret |= vs_ue(str, &table->chroma_log2_weight_denom);
	int i;
	for (i = 0; i <= slice->num_ref_idx_l0_active_minus1; i++)
		ret |= h264_pred_weight_table_entry(str, table, &table->l0[i]);
	if (slice->slice_type % 5 == 1)
		for (i = 0; i <= slice->num_ref_idx_l1_active_minus1; i++)
			ret |= h264_pred_weight_table_entry(str, table, &table->l1[i]);
	return ret;
}
