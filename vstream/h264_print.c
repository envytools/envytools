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

void h264_print_seqparm(struct h264_seqparm *seqparm) {
	printf("Sequence parameter set:\n");
	const char *profile_name = "???";
	switch (seqparm->profile_idc) {
		case H264_PROFILE_BASELINE:
			profile_name = "Baseline";
			break;
		case H264_PROFILE_MAIN:
			profile_name = "Main";
			break;
		case H264_PROFILE_EXTENDED:
			profile_name = "Extended";
			break;
		case H264_PROFILE_HIGH:
			profile_name = "High";
			break;
		case H264_PROFILE_HIGH_10:
			profile_name = "High 10";
			break;
		case H264_PROFILE_HIGH_422:
			profile_name = "High 4:2:2";
			break;
		case H264_PROFILE_HIGH_444_PRED:
			profile_name = "High 4:4:4 Predictive";
			break;
		case H264_PROFILE_CAVLC_444:
			profile_name = "CAVLC 4:4:4";
			break;
		case H264_PROFILE_SCALABLE_BASELINE:
			profile_name = "Scalable Baseline";
			break;
		case H264_PROFILE_SCALABLE_HIGH:
			profile_name = "Scalable High";
			break;
		case H264_PROFILE_MULTIVIEW_HIGH:
			profile_name = "Multiview High";
			break;
		case H264_PROFILE_STEREO_HIGH:
			profile_name = "Stereo High";
			break;
	}
	printf ("\tprofile_idc = %d [%s]\n", seqparm->profile_idc, profile_name);
	printf ("\tconstraint_set =");
	int i, j;
	for (i = 7; i >= 0; i--)
		printf(" %d", seqparm->constraint_set >> i & 1);
	printf("\n");
	printf ("\tlevel_idc = %d.%d\n", seqparm->level_idc / 10, seqparm->level_idc % 10);
	printf ("\tseq_parameter_set_id = %d\n", seqparm->seq_parameter_set_id);
	printf ("\tchroma_format_idc = %d\n", seqparm->chroma_format_idc);
	printf ("\tseparate_colour_plane_flag = %d\n", seqparm->separate_colour_plane_flag);
	printf ("\tbit_depth_luma_minus8 = %d\n", seqparm->bit_depth_luma_minus8);
	printf ("\tbit_depth_chroma_minus8 = %d\n", seqparm->bit_depth_chroma_minus8);
	printf ("\tqpprime_y_zero_transform_bypass_flag = %d\n", seqparm->qpprime_y_zero_transform_bypass_flag);
	printf ("\tseq_scaling_matrix_present_flag = %d\n", seqparm->seq_scaling_matrix_present_flag);
	if (seqparm->seq_scaling_matrix_present_flag) {
		for (i = 0; i < (seqparm->chroma_format_idc == 3 ? 12 : 8); i++) {
			printf ("\tseq_scaling_list_present_flag[%d] = %d\n", i, seqparm->seq_scaling_list_present_flag[i]);
			if (seqparm->seq_scaling_list_present_flag[i]) {
				printf ("\tuse_default_scaling_matrix_flag[%d] = %d\n", i, seqparm->use_default_scaling_matrix_flag[i]);
				if (!seqparm->use_default_scaling_matrix_flag[i]) {
					for (j = 0; j < (i < 6 ? 16 : 64); j++) {
						if (i < 6)
							printf ("\tseq_scaling_list[%d][%d] = %d\n", i, j, seqparm->seq_scaling_list_4x4[i][j]);
						else
							printf ("\tseq_scaling_list[%d][%d] = %d\n", i, j, seqparm->seq_scaling_list_8x8[i-6][j]);
					}
				}
			}
		}
	}
	printf ("\tlog2_max_frame_num_minus4 = %d\n", seqparm->log2_max_frame_num_minus4);
	printf ("\tpic_order_cnt_type = %d\n", seqparm->pic_order_cnt_type);
	switch (seqparm->pic_order_cnt_type) {
		case 0:
			printf ("\tlog2_max_pic_order_cnt_lsb_minus4 = %d\n", seqparm->log2_max_pic_order_cnt_lsb_minus4);
			break;
		case 1:
			printf ("\tdelta_pic_order_always_zero_flag = %d\n", seqparm->delta_pic_order_always_zero_flag);
			printf ("\toffset_for_non_ref_pic = %d\n", seqparm->offset_for_non_ref_pic);
			printf ("\toffset_for_top_to_bottom_field = %d\n", seqparm->offset_for_top_to_bottom_field);
			printf ("\tnum_ref_frames_in_pic_order_cnt_cycle = %d\n", seqparm->num_ref_frames_in_pic_order_cnt_cycle);
			for (i = 0; i < seqparm->num_ref_frames_in_pic_order_cnt_cycle; i++) {
				printf("\toffset_for_ref_frame[%d] = %d\n", i, seqparm->offset_for_ref_frame[i]);
			}
			break;
	}
	printf ("\tmax_num_ref_frames = %d\n", seqparm->max_num_ref_frames);
	printf ("\tgaps_in_frame_num_value_allowed_flag = %d\n", seqparm->gaps_in_frame_num_value_allowed_flag);
	printf ("\tpic_width_in_mbs_minus1 = %d\n", seqparm->pic_width_in_mbs_minus1);
	printf ("\tpic_height_in_map_units_minus1 = %d\n", seqparm->pic_height_in_map_units_minus1);
	printf ("\tframe_mbs_only_flag = %d\n", seqparm->frame_mbs_only_flag);
	printf ("\tmb_adaptive_frame_field_flag = %d\n", seqparm->mb_adaptive_frame_field_flag);
	printf ("\tdirect_8x8_inference_flag = %d\n", seqparm->direct_8x8_inference_flag);
	printf ("\tframe_cropping_flag = %d\n", seqparm->frame_cropping_flag);
	printf ("\tframe_crop_left_offset = %d\n", seqparm->frame_crop_left_offset);
	printf ("\tframe_crop_right_offset = %d\n", seqparm->frame_crop_right_offset);
	printf ("\tframe_crop_top_offset = %d\n", seqparm->frame_crop_top_offset);
	printf ("\tframe_crop_bottom_offset = %d\n", seqparm->frame_crop_bottom_offset);
	if (seqparm->vui) {
		/* XXX */
	}
}
