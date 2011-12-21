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

void h264_print_hrd(struct h264_hrd_parameters *hrd) {
	printf ("\t\t\tcpb_cnt_minus1 = %d\n", hrd->cpb_cnt_minus1);
	printf ("\t\t\tbit_rate_scale = %d\n", hrd->bit_rate_scale);
	printf ("\t\t\tcpb_size_scale = %d\n", hrd->cpb_size_scale);
	int i;
	for (i = 0; i <= hrd->cpb_cnt_minus1; i++) {
		printf ("\t\t\tbit_rate_value_minus1[%d] = %d\n", i, hrd->bit_rate_value_minus1[i]);
		printf ("\t\t\tcpb_size_value_minus1[%d] = %d\n", i, hrd->cpb_size_value_minus1[i]);
		printf ("\t\t\tcbr_flag[%d] = %d\n", i, hrd->cbr_flag[i]);
	}
	printf ("\t\t\tinitial_cpb_removal_delay_length_minus1 = %d\n", hrd->initial_cpb_removal_delay_length_minus1);
	printf ("\t\t\tcpb_removal_delay_length_minus1 = %d\n", hrd->cpb_removal_delay_length_minus1);
	printf ("\t\t\tdpb_output_delay_length_minus1 = %d\n", hrd->dpb_output_delay_length_minus1);
	printf ("\t\t\ttime_offset_length = %d\n", hrd->time_offset_length);
}

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
		printf("\tVUI parameters:\n");
		printf("\t\taspect_ratio_present_flag = %d\n", seqparm->vui->aspect_ratio_present_flag);
		printf("\t\taspect_ratio_idc = %d\n", seqparm->vui->aspect_ratio_idc);
		printf("\t\tsar_width = %d\n", seqparm->vui->sar_width);
		printf("\t\tsar_height = %d\n", seqparm->vui->sar_height);
		printf("\t\toverscan_info_present_flag = %d\n", seqparm->vui->overscan_info_present_flag);
		if (seqparm->vui->overscan_info_present_flag) {
			printf("\t\toverscan_appropriate_flag = %d\n", seqparm->vui->overscan_appropriate_flag);
		}
		printf("\t\tvideo_signal_type_present_flag = %d\n", seqparm->vui->video_signal_type_present_flag);
		printf("\t\tvideo_format = %d\n", seqparm->vui->video_format);
		printf("\t\tvideo_full_range_flag = %d\n", seqparm->vui->video_full_range_flag);
		printf("\t\tcolour_description_present_flag = %d\n", seqparm->vui->colour_description_present_flag);
		printf("\t\tcolour_primaries = %d\n", seqparm->vui->colour_primaries);
		printf("\t\ttransfer_characteristics = %d\n", seqparm->vui->transfer_characteristics);
		printf("\t\tmatrix_coefficients = %d\n", seqparm->vui->matrix_coefficients);
		printf("\t\tchroma_loc_info_present_flag = %d\n", seqparm->vui->chroma_loc_info_present_flag);
		printf("\t\tchroma_sample_loc_type_top_field = %d\n", seqparm->vui->chroma_sample_loc_type_top_field);
		printf("\t\tchroma_sample_loc_type_bottom_field = %d\n", seqparm->vui->chroma_sample_loc_type_bottom_field);
		printf("\t\ttiming_info_present_flag = %d\n", seqparm->vui->timing_info_present_flag);
		if (seqparm->vui->timing_info_present_flag) {
			printf("\t\tnum_units_in_tick = %d\n", seqparm->vui->num_units_in_tick);
			printf("\t\ttime_scale = %d\n", seqparm->vui->time_scale);
		}
		printf("\t\tfixed_frame_rate_flag = %d\n", seqparm->vui->fixed_frame_rate_flag);
		if (seqparm->vui->nal_hrd_parameters) {
			printf("\t\tNAL HRD parameters:\n");
			h264_print_hrd(seqparm->vui->nal_hrd_parameters);
		}
		if (seqparm->vui->vcl_hrd_parameters) {
			printf("\t\tVCL HRD parameters:\n");
			h264_print_hrd(seqparm->vui->vcl_hrd_parameters);
		}
		if (seqparm->vui->nal_hrd_parameters || seqparm->vui->vcl_hrd_parameters) {
			printf("\t\tlow_delay_hrd_flag = %d\n", seqparm->vui->low_delay_hrd_flag);
		}
		printf("\t\tpic_struct_present_flag = %d\n", seqparm->vui->pic_struct_present_flag);
		printf("\t\tbitstream_restriction_present_flag = %d\n", seqparm->vui->bitstream_restriction_present_flag);
		printf("\t\tmotion_vectors_over_pic_bounduaries_flag = %d\n", seqparm->vui->motion_vectors_over_pic_bounduaries_flag);
		printf("\t\tmax_bytes_per_pic_denom = %d\n", seqparm->vui->max_bytes_per_pic_denom);
		printf("\t\tmax_bits_per_mb_denom = %d\n", seqparm->vui->max_bits_per_mb_denom);
		printf("\t\tlog2_max_mv_length_horizontal = %d\n", seqparm->vui->log2_max_mv_length_horizontal);
		printf("\t\tlog2_max_mv_length_vertical = %d\n", seqparm->vui->log2_max_mv_length_vertical);
		printf("\t\tnum_reorder_frames = %d\n", seqparm->vui->num_reorder_frames);
		printf("\t\tmax_dec_frame_buffering = %d\n", seqparm->vui->max_dec_frame_buffering);
	}
}

void h264_print_seqparm_ext(struct h264_seqparm *seqparm) {
	printf("Sequence parameter set extension:\n");
	printf("\taux_format_idc = %d\n", seqparm->aux_format_idc);
	if (seqparm->aux_format_idc) {
		printf("\tbit_depth_aux_minus8 = %d\n", seqparm->bit_depth_aux_minus8);
		printf("\talpha_incr_flag = %d\n", seqparm->alpha_incr_flag);
		printf("\talpha_opaque_value = %d\n", seqparm->alpha_opaque_value);
		printf("\talpha_transparent_value = %d\n", seqparm->alpha_transparent_value);
	}
}
