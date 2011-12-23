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
	int i, j, k;
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
	if (seqparm->is_svc) {
		printf("\tinter_layer_deblocking_filter_control_present_flag = %d\n", seqparm->inter_layer_deblocking_filter_control_present_flag);
		printf("\textended_spatial_scalability_idc = %d\n", seqparm->extended_spatial_scalability_idc);
		printf("\tchroma_phase_x_plus1_flag = %d\n", seqparm->chroma_phase_x_plus1_flag);
		printf("\tchroma_phase_y_plus1 = %d\n", seqparm->chroma_phase_y_plus1);
		printf("\tseq_ref_layer_chroma_phase_x_plus1_flag = %d\n", seqparm->seq_ref_layer_chroma_phase_x_plus1_flag);
		printf("\tseq_ref_layer_chroma_phase_y_plus1 = %d\n", seqparm->seq_ref_layer_chroma_phase_y_plus1);
		printf("\tseq_ref_layer_left_offset = %d\n", seqparm->seq_ref_layer_left_offset);
		printf("\tseq_ref_layer_top_offset = %d\n", seqparm->seq_ref_layer_top_offset);
		printf("\tseq_ref_layer_right_offset = %d\n", seqparm->seq_ref_layer_right_offset);
		printf("\tseq_ref_layer_bottom_offset = %d\n", seqparm->seq_ref_layer_bottom_offset);
		printf("\tseq_tcoeff_level_prediction_flag = %d\n", seqparm->seq_tcoeff_level_prediction_flag);
		printf("\tadaptive_tcoeff_level_prediction_flag = %d\n", seqparm->adaptive_tcoeff_level_prediction_flag);
		printf("\tslice_header_restriction_flag = %d\n", seqparm->slice_header_restriction_flag);
		if (seqparm->svc_vui) {
			/* XXX */
		}
	}
	if (seqparm->is_mvc) {
		printf("\tnum_views_minus1 = %d\n", seqparm->num_views_minus1);
		for (i = 0; i <= seqparm->num_views_minus1; i++) {
			printf("\tview_id[%d] = %d\n", i, seqparm->views[i].view_id);
			printf("\tnum_anchor_refs_l0[%d] = %d\n", i, seqparm->views[i].num_anchor_refs_l0);
			for (j = 1; j < seqparm->views[i].num_anchor_refs_l0; j++)
				printf("\tanchor_ref_l0[%d][%d] = %d\n", i, j, seqparm->views[i].anchor_ref_l0[j]);
			printf("\tnum_anchor_refs_l1[%d] = %d\n", i, seqparm->views[i].num_anchor_refs_l1);
			for (j = 1; j < seqparm->views[i].num_anchor_refs_l1; j++)
				printf("\tanchor_ref_l1[%d][%d] = %d\n", i, j, seqparm->views[i].anchor_ref_l1[j]);
			printf("\tnum_non_anchor_refs_l0[%d] = %d\n", i, seqparm->views[i].num_non_anchor_refs_l0);
			for (j = 1; j < seqparm->views[i].num_non_anchor_refs_l0; j++)
				printf("\tnon_anchor_ref_l0[%d][%d] = %d\n", i, j, seqparm->views[i].non_anchor_ref_l0[j]);
			printf("\tnum_non_anchor_refs_l1[%d] = %d\n", i, seqparm->views[i].num_non_anchor_refs_l1);
			for (j = 1; j < seqparm->views[i].num_non_anchor_refs_l1; j++)
				printf("\tnon_anchor_ref_l1[%d][%d] = %d\n", i, j, seqparm->views[i].non_anchor_ref_l1[j]);
		}
		printf("\tnum_level_values_signalled_minus1 = %d\n", seqparm->num_level_values_signalled_minus1);
		for (i = 0; i <= seqparm->num_level_values_signalled_minus1; i++) {
			printf("\tlevel_idc[%d] = %d.%d\n", i, seqparm->levels[i].level_idc / 10, seqparm->levels[i].level_idc % 10);
			printf("\tnum_applicable_ops_minus1[%d] = %d\n", i, seqparm->levels[i].num_applicable_ops_minus1);
			for (j = 0; j <= seqparm->levels[i].num_applicable_ops_minus1; j++) {
				struct h264_seqparm_mvc_applicable_op *op = &seqparm->levels[i].applicable_ops[j];
				printf("\tapplicable_op_temporal_id[%d][%d] = %d\n", i, j, op->temporal_id);
				printf("\tapplicable_op_num_target_views_minus1[%d][%d] = %d\n", i, j, op->num_target_views_minus1);
				for (k = 0; k <= op->num_target_views_minus1; k++)
					printf("\tapplicable_op_target_view_id[%d][%d][%d] = %d\n", i, j, k, op->target_view_id[k]);
				printf("\tapplicable_op_num_views_minus1[%d][%d] = %d\n", i, j, op->num_views_minus1);
			}
		}
		if (seqparm->mvc_vui) {
			/* XXX */
		}
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

void h264_print_picparm(struct h264_picparm *picparm) {
	printf("Picture parameter set:\n");
	printf ("\tpic_parameter_set_id = %d\n", picparm->pic_parameter_set_id);
	printf ("\tseq_parameter_set_id = %d\n", picparm->seq_parameter_set_id);
	printf ("\tentropy_coding_mode_flag = %d\n", picparm->entropy_coding_mode_flag);
	printf ("\tbottom_field_pic_order_in_frame_present_flag = %d\n", picparm->bottom_field_pic_order_in_frame_present_flag);
	printf ("\tnum_slice_groups_minus1 = %d\n", picparm->num_slice_groups_minus1);
	if (picparm->num_slice_groups_minus1) {
		int i;
		printf ("\tslice_group_map_type = %d\n", picparm->slice_group_map_type);
		switch (picparm->slice_group_map_type) {
			case H264_SLICE_GROUP_MAP_INTERLEAVED:
				for (i = 0; i <= picparm->num_slice_groups_minus1; i++) {
					printf ("\trun_length_minus1[%d] = %d\n", i, picparm->run_length_minus1[i]);

				}
				break;
			case H264_SLICE_GROUP_MAP_DISPERSED:
				break;
			case H264_SLICE_GROUP_MAP_FOREGROUND:
				for (i = 0; i < picparm->num_slice_groups_minus1; i++) {
					printf ("\ttop_left[%d] = %d\n", i, picparm->top_left[i]);
					printf ("\tbottom_right[%d] = %d\n", i, picparm->bottom_right[i]);

				}
				break;
			case H264_SLICE_GROUP_MAP_CHANGING_BOX:
			case H264_SLICE_GROUP_MAP_CHANGING_VERTICAL:
			case H264_SLICE_GROUP_MAP_CHANGING_HORIZONTAL:
				printf ("\tslice_group_change_direction_flag = %d\n", picparm->slice_group_change_direction_flag);
				printf ("\tslice_group_change_rate_minus1 = %d\n", picparm->slice_group_change_rate_minus1);
				break;
			case H264_SLICE_GROUP_MAP_EXPLICIT:
				printf ("\tpic_size_in_map_units_minus1 = %d\n", picparm->pic_size_in_map_units_minus1);
				for (i = 0; i <= picparm->pic_size_in_map_units_minus1; i++)
					printf ("\tslice_group_id[%d] = %d\n", i, picparm->slice_group_id[i]);
				break;
		}
	}
	printf ("\tnum_ref_idx_l0_default_active_minus1 = %d\n", picparm->num_ref_idx_l0_default_active_minus1);
	printf ("\tnum_ref_idx_l1_default_active_minus1 = %d\n", picparm->num_ref_idx_l1_default_active_minus1);
	printf ("\tweighted_pred_flag = %d\n", picparm->weighted_pred_flag);
	printf ("\tweighted_bipred_idc = %d\n", picparm->weighted_bipred_idc);
	printf ("\tpic_init_qp_minus26 = %d\n", picparm->pic_init_qp_minus26);
	printf ("\tpic_init_qs_minus26 = %d\n", picparm->pic_init_qs_minus26);
	printf ("\tchroma_qp_index_offset = %d\n", picparm->chroma_qp_index_offset);
	printf ("\tdeblocking_filter_control_present_flag = %d\n", picparm->deblocking_filter_control_present_flag);
	printf ("\tconstrained_intra_pred_flag = %d\n", picparm->constrained_intra_pred_flag);
	printf ("\tredundant_pic_cnt_present_flag = %d\n", picparm->redundant_pic_cnt_present_flag);
	printf ("\ttransform_8x8_mode_flag = %d\n", picparm->transform_8x8_mode_flag);
	printf ("\tpic_scaling_matrix_present_flag = %d\n", picparm->pic_scaling_matrix_present_flag);
	if (picparm->pic_scaling_matrix_present_flag) {
		int i, j;
		for (i = 0; i < (picparm->chroma_format_idc == 3 ? 12 : 8); i++) {
			printf ("\tpic_scaling_list_present_flag[%d] = %d\n", i, picparm->pic_scaling_list_present_flag[i]);
			if (picparm->pic_scaling_list_present_flag[i]) {
				printf ("\tuse_default_scaling_matrix_flag[%d] = %d\n", i, picparm->use_default_scaling_matrix_flag[i]);
				if (!picparm->use_default_scaling_matrix_flag[i]) {
					for (j = 0; j < (i < 6 ? 16 : 64); j++) {
						if (i < 6)
							printf ("\tpic_scaling_list[%d][%d] = %d\n", i, j, picparm->pic_scaling_list_4x4[i][j]);
						else
							printf ("\tpic_scaling_list[%d][%d] = %d\n", i, j, picparm->pic_scaling_list_8x8[i-6][j]);
					}
				}
			}
		}
	}
	printf ("\tsecond_chroma_qp_index_offset = %d\n", picparm->second_chroma_qp_index_offset);
}

void h264_print_ref_pic_list_modification(struct h264_ref_pic_list_modification *list, char *which) {
	static const char *const opnames[6] = { "pic_num sub", "pic_num add", "long term", "end", "view idx sub", "view idx add" };
	static const char *const argnames[6] = { "abs_diff_pic_num_minus1", "abs_diff_pic_num_minus1", "long_term_pic_num", 0, "abs_diff_view_idx_minus1", "abs_diff_view_idx_minus1" };
	int i;
	printf("\tref_pic_list_modification_flag_%s = %d\n", which, list->flag);
	for (i = 0; list->list[i].op != 3; i++) {
		int op = list->list[i].op;
		printf("\tmodification_of_pic_nums_idc = %d [%s]\n", op, opnames[op]);
		printf("\t%s = %d\n", argnames[i], list->list[i].op);
	}
	printf("\tmodification_of_pic_nums_idc = 3 [END]\n");
}

void h264_print_pred_weight_table(struct h264_slice *slice, struct h264_pred_weight_table *table) {
	printf("\tluma_log2_weight_denom = %d\n", table->luma_log2_weight_denom);
	printf("\tchroma_log2_weight_denom = %d\n", table->chroma_log2_weight_denom);
	int i;
	for (i = 0; i <= slice->num_ref_idx_l0_active_minus1; i++) {
		printf("\tluma_weight_l0_flag[%d] = %d\n", i, table->l0[i].luma_weight_flag);
		printf("\tluma_weight_l0[%d] = %d\n", i, table->l0[i].luma_weight);
		printf("\tluma_offset_l0[%d] = %d\n", i, table->l0[i].luma_offset);
		printf("\tchroma_weight_l0_flag[%d] = %d\n", i, table->l0[i].chroma_weight_flag);
		printf("\tchroma_weight_l0[%d][0] = %d\n", i, table->l0[i].chroma_weight[0]);
		printf("\tchroma_weight_l0[%d][1] = %d\n", i, table->l0[i].chroma_weight[1]);
		printf("\tchroma_offset_l0[%d][0] = %d\n", i, table->l0[i].chroma_offset[0]);
		printf("\tchroma_offset_l0[%d][1] = %d\n", i, table->l0[i].chroma_offset[1]);
	}
	if (slice->slice_type == H264_SLICE_TYPE_B) {
		for (i = 0; i <= slice->num_ref_idx_l1_active_minus1; i++) {
			printf("\tluma_weight_l1_flag[%d] = %d\n", i, table->l1[i].luma_weight_flag);
			printf("\tluma_weight_l1[%d] = %d\n", i, table->l1[i].luma_weight);
			printf("\tluma_offset_l1[%d] = %d\n", i, table->l1[i].luma_offset);
			printf("\tchroma_weight_l1_flag[%d] = %d\n", i, table->l1[i].chroma_weight_flag);
			printf("\tchroma_weight_l1[%d][0] = %d\n", i, table->l1[i].chroma_weight[0]);
			printf("\tchroma_weight_l1[%d][1] = %d\n", i, table->l1[i].chroma_weight[1]);
			printf("\tchroma_offset_l1[%d][0] = %d\n", i, table->l1[i].chroma_offset[0]);
			printf("\tchroma_offset_l1[%d][1] = %d\n", i, table->l1[i].chroma_offset[1]);
		}
	}
}

void h264_print_dec_ref_pic_marking(int idr_pic_flag, struct h264_dec_ref_pic_marking *ref) {
	if (idr_pic_flag) {
		printf("\tno_output_of_prior_pics_flag = %d\n", ref->no_output_of_prior_pics_flag);
		printf("\tlong_term_reference_flag = %d\n", ref->long_term_reference_flag);
	} else {
		printf("\tadaptive_ref_pic_marking_mode_flag = %d\n", ref->adaptive_ref_pic_marking_mode_flag);
		if (ref->adaptive_ref_pic_marking_mode_flag) {
			int i = 0;
			do {
				printf("\tmemory_management_control_operation = %d\n", ref->mmcos[i].memory_management_control_operation);
				switch (ref->mmcos[i].memory_management_control_operation) {
					case H264_MMCO_END:
						break;
					case H264_MMCO_FORGET_SHORT:
						printf("\tdifference_of_pic_nums_minus1 = %d\n", ref->mmcos[i].difference_of_pic_nums_minus1);
						break;
					case H264_MMCO_FORGET_LONG:
						printf("\tlong_term_pic_num = %d\n", ref->mmcos[i].long_term_pic_num);
						break;
					case H264_MMCO_SHORT_TO_LONG:
						printf("\tdifference_of_pic_nums_minus1 = %d\n", ref->mmcos[i].difference_of_pic_nums_minus1);
						printf("\tlong_term_frame_idx = %d\n", ref->mmcos[i].long_term_frame_idx);
						break;
					case H264_MMCO_FORGET_LONG_MANY:
						printf("\tmax_long_term_frame_idx_plus1 = %d\n", ref->mmcos[i].max_long_term_frame_idx_plus1);
						break;
					case H264_MMCO_FORGET_ALL:
						break;
					case H264_MMCO_THIS_TO_LONG:
						printf("\tlong_term_frame_idx = %d\n", ref->mmcos[i].long_term_frame_idx);
						break;
				}
			} while (ref->mmcos[i++].memory_management_control_operation != H264_MMCO_END);
		}
	}
}

void h264_print_dec_ref_base_pic_marking(struct h264_nal_svc_header *svc, struct h264_dec_ref_base_pic_marking *ref) {
	printf("\tstore_ref_base_pic_flag = %d\n", ref->store_ref_base_pic_flag);
	if ((svc->use_ref_base_pic_flag || ref->store_ref_base_pic_flag) && !svc->idr_flag) {
		printf("\tadaptive_ref_base_pic_marking_mode_flag = %d\n", ref->adaptive_ref_base_pic_marking_mode_flag);
		if (ref->adaptive_ref_base_pic_marking_mode_flag) {
			int i = 0;
			do {
				printf("\tmemory_management_control_operation = %d\n", ref->mmcos[i].memory_management_control_operation);
				switch (ref->mmcos[i].memory_management_control_operation) {
					case H264_MMCO_END:
						break;
					case H264_MMCO_FORGET_SHORT:
						printf("\tdifference_of_pic_nums_minus1 = %d\n", ref->mmcos[i].difference_of_pic_nums_minus1);
						break;
					case H264_MMCO_FORGET_LONG:
						printf("\tlong_term_pic_num = %d\n", ref->mmcos[i].long_term_pic_num);
						break;
				}
			} while (ref->mmcos[i++].memory_management_control_operation != H264_MMCO_END);
		}
	}
}

void h264_print_slice_header(struct h264_slice *slice) {
	printf("Slice header:\n");
	printf("\tfirst_mb_in_slice = %d\n", slice->first_mb_in_slice);
	static const char *const stypes[5] = { "P", "B", "I", "SP", "SI" };
	printf("\tslice_type = %d [%s%s]\n", slice->slice_type + slice->slice_all_same * 5, slice->slice_all_same ? "all ":"", stypes[slice->slice_type]);
	printf("\tpic_parameter_set_id = %d\n", slice->picparm->pic_parameter_set_id);
	if (slice->seqparm->separate_colour_plane_flag)
		printf("\tcolour_plane_id = %d\n", slice->colour_plane_id);
	printf("\tframe_num = %d\n", slice->frame_num);
	printf("\tfield_pic_flag = %d\n", slice->field_pic_flag);
	printf("\tbottom_field_flag = %d\n", slice->bottom_field_flag);
	if (slice->idr_pic_flag)
		printf("\tidr_pic_id = %d\n", slice->idr_pic_id);
	switch (slice->seqparm->pic_order_cnt_type) {
		case 0:
			printf("\tpic_order_cnt_lsb = %d\n", slice->pic_order_cnt_lsb);
			printf("\tdelta_pic_order_cnt_bottom = %d\n", slice->delta_pic_order_cnt_bottom);
			break;
		case 1:
			printf("\tdelta_pic_order_cnt[0] = %d\n", slice->delta_pic_order_cnt[0]);
			printf("\tdelta_pic_order_cnt[1] = %d\n", slice->delta_pic_order_cnt[1]);
			break;
	}
	printf("\tredundant_pic_cnt = %d\n", slice->redundant_pic_cnt);
	if (slice->slice_type == H264_SLICE_TYPE_B)
		printf("\tdirect_spatial_mb_pred_flag = %d\n", slice->direct_spatial_mb_pred_flag);
	if (slice->slice_type != H264_SLICE_TYPE_I && slice->slice_type != H264_SLICE_TYPE_SI) {
		printf("\tnum_ref_idx_active_override_flag = %d\n", slice->num_ref_idx_active_override_flag);
		printf("\tnum_ref_idx_l0_active_minus1 = %d\n", slice->num_ref_idx_l0_active_minus1);
		if (slice->slice_type == H264_SLICE_TYPE_B)
			printf("\tnum_ref_idx_l1_active_minus1 = %d\n", slice->num_ref_idx_l1_active_minus1);
		h264_print_ref_pic_list_modification(&slice->ref_pic_list_modification_l0, "l0");
		if (slice->slice_type == H264_SLICE_TYPE_B)
			h264_print_ref_pic_list_modification(&slice->ref_pic_list_modification_l1, "l1");
	}
	if ((slice->picparm->weighted_pred_flag && (slice->slice_type == H264_SLICE_TYPE_P || slice->slice_type == H264_SLICE_TYPE_SP)) || (slice->picparm->weighted_bipred_idc == 1 && slice->slice_type == H264_SLICE_TYPE_B)) {
		printf("\tbase_pred_weight_table_flag = %d\n", slice->base_pred_weight_table_flag);
		if (!slice->base_pred_weight_table_flag) {
			h264_print_pred_weight_table(slice, &slice->pred_weight_table);
		}
	}
	if (slice->nal_ref_idc) {
		h264_print_dec_ref_pic_marking(slice->idr_pic_flag, &slice->dec_ref_pic_marking);
		if (slice->seqparm->is_svc && !slice->seqparm->slice_header_restriction_flag)
			h264_print_dec_ref_base_pic_marking(&slice->svc, &slice->dec_ref_base_pic_marking);
	}
	if (slice->slice_type != H264_SLICE_TYPE_I && slice->slice_type != H264_SLICE_TYPE_SI)
		printf("\tcabac_init_idc = %d\n", slice->cabac_init_idc);
	printf("\tslice_qp_delta = %d\n", slice->slice_qp_delta);
	if (slice->slice_type == H264_SLICE_TYPE_SP)
		printf("\tsp_for_switch_flag = %d\n", slice->sp_for_switch_flag);
	if (slice->slice_type == H264_SLICE_TYPE_SP || slice->slice_type == H264_SLICE_TYPE_SI)
		printf("\tslice_qs_delta = %d\n", slice->slice_qs_delta);
	printf("\tdisable_deblocking_filter_idc = %d\n", slice->disable_deblocking_filter_idc);
	printf("\tslice_alpha_c0_offset_div2 = %d\n", slice->slice_alpha_c0_offset_div2);
	printf("\tslice_beta_offset_div2 = %d\n", slice->slice_beta_offset_div2);
	if (slice->picparm->num_slice_groups_minus1 && slice->picparm->slice_group_map_type >= 3 && slice->picparm->slice_group_map_type <= 5)
		printf("\tslice_group_change_cycle = %d\n", slice->slice_group_change_cycle);
	if (slice->seqparm->is_svc) {
		/* XXX */
	}
}

void h264_print_block(int16_t *block, int num) {
	int i;
	for (i = 0; i < num; i++)
		printf(" %d", block[i]);
	printf("\n");
}

void h264_print_pcm(uint32_t *pcm, int num) {
	int i;
	for (i = 0; i < num; i++)
		printf(" %d", pcm[i]);
	printf("\n");
}

void h264_print_macroblock(struct h264_slice *slice, struct h264_macroblock *mb) {
	static const char *const mbtypenames[] = {
		"I_NxN",
		"I_16X16_0_0_0",
		"I_16X16_1_0_0",
		"I_16X16_2_0_0",
		"I_16X16_3_0_0",
		"I_16X16_0_1_0",
		"I_16X16_1_1_0",
		"I_16X16_2_1_0",
		"I_16X16_3_1_0",
		"I_16X16_0_2_0",
		"I_16X16_1_2_0",
		"I_16X16_2_2_0",
		"I_16X16_3_2_0",
		"I_16X16_0_0_1",
		"I_16X16_1_0_1",
		"I_16X16_2_0_1",
		"I_16X16_3_0_1",
		"I_16X16_0_1_1",
		"I_16X16_1_1_1",
		"I_16X16_2_1_1",
		"I_16X16_3_1_1",
		"I_16X16_0_2_1",
		"I_16X16_1_2_1",
		"I_16X16_2_2_1",
		"I_16X16_3_2_1",
		"I_PCM",
		"SI",
		"P_L0_16X16",
		"P_L0_L0_16X8",
		"P_L0_L0_8X16",
		"P_8X8",
		"P_8X8REF0",
		"P_SKIP",
		"B_DIRECT_16X16",
		"B_L0_16X16",
		"B_L1_16X16",
		"B_BI_16X16",
		"B_L0_L0_16X8",
		"B_L0_L0_8X16",
		"B_L1_L1_16X8",
		"B_L1_L1_8X16",
		"B_L0_L1_16X8",
		"B_L0_L1_8X16",
		"B_L1_L0_16X8",
		"B_L1_L0_8X16",
		"B_L0_BI_16X8",
		"B_L0_BI_8X16",
		"B_L1_BI_16X8",
		"B_L1_BI_8X16",
		"B_BI_L0_16X8",
		"B_BI_L0_8X16",
		"B_BI_L1_16X8",
		"B_BI_L1_8X16",
		"B_BI_BI_16X8",
		"B_BI_BI_8X16",
		"B_8X8",
		"B_SKIP",
		"UNAVAIL",
	};
	static const char *const aname[3] = { "Luma", "Cb", "Cr" };
	printf("\t\tmb_field_decoding_flag = %d\n", mb->mb_field_decoding_flag);
	printf("\t\tmb_type = %d [%s]\n", mb->mb_type, mbtypenames[mb->mb_type]);
	int i, j;
	if (mb->mb_type == H264_MB_TYPE_I_PCM) {
		printf("\t\tLuma PCM:");
		h264_print_pcm(mb->pcm_sample_luma, 256);
		switch (slice->chroma_array_type) {
			case 0:
				break;
			case 1:
				printf("\t\tChroma PCM:");
				h264_print_pcm(mb->pcm_sample_chroma, 128);
				break;
			case 2:
				printf("\t\tChroma PCM:");
				h264_print_pcm(mb->pcm_sample_chroma, 256);
				break;
			case 3:
				printf("\t\tChroma PCM:");
				h264_print_pcm(mb->pcm_sample_chroma, 512);
				break;
		}
	} else {
		printf("\t\ttransform_size_8x8_flag = %d\n", mb->transform_size_8x8_flag);
		printf("\t\tcoded_block_pattern = %d\n", mb->coded_block_pattern);
		if (mb->mb_type == H264_MB_TYPE_I_NXN || mb->mb_type == H264_MB_TYPE_SI) {
			if (mb->transform_size_8x8_flag) {
				for (i = 0; i < 4; i++) {
					printf("\t\tprev_intra8x8_pred_mode_flag[%d] = %d\n", i, mb->prev_intra8x8_pred_mode_flag[i]);
					if (!mb->prev_intra8x8_pred_mode_flag[i])
						printf("\t\trem_intra8x8_pred_mode[%d] = %d\n", i, mb->rem_intra8x8_pred_mode[i]);
				}
			} else {
				for (i = 0; i < 16; i++) {
					printf("\t\tprev_intra4x4_pred_mode_flag[%d] = %d\n", i, mb->prev_intra4x4_pred_mode_flag[i]);
					if (!mb->prev_intra4x4_pred_mode_flag[i])
						printf("\t\trem_intra4x4_pred_mode[%d] = %d\n", i, mb->rem_intra4x4_pred_mode[i]);
				}
			}
		}
		if (mb->mb_type < H264_MB_TYPE_P_BASE)
			printf("\t\tintra_chroma_pred_mode = %d\n", mb->intra_chroma_pred_mode);
		printf("\t\tmb_qp_delta = %d\n", mb->mb_qp_delta);
		int n = (slice->chroma_array_type == 3 ? 3 : 1);
		if (h264_is_intra_16x16_mb_type(mb->mb_type)) {
			for (i = 0; i < n; i++) {
				printf("\t\t%s DC:", aname[i]);
				h264_print_block(mb->block_luma_dc[i], 16);
				for (j = 0; j < 16; j++) {
					printf("\t\t%s AC %d:", aname[i], j);
					h264_print_block(mb->block_luma_ac[i][j], 15);
				}
			}
		} else if (mb->transform_size_8x8_flag) {
			for (i = 0; i < n; i++) {
				for (j = 0; j < 4; j++) {
					printf("\t\t%s 8x8 %d:", aname[i], j);
					h264_print_block(mb->block_luma_8x8[i][j], 64);
				}
			}
		} else {
			for (i = 0; i < n; i++) {
				for (j = 0; j < 16; j++) {
					printf("\t\t%s 4x4 %d:", aname[i], j);
					h264_print_block(mb->block_luma_4x4[i][j], 16);
				}
			}
		}
		if (slice->chroma_array_type == 1 || slice->chroma_array_type == 2) {
			for (i = 0; i < 2; i++) {
				printf("\t\t%s DC:", aname[i+1]);
				h264_print_block(mb->block_chroma_dc[i], slice->chroma_array_type * 4);
				for (j = 0; j < slice->chroma_array_type * 4; j++) {
					printf("\t\t%s AC %d:", aname[i+1], j);
					h264_print_block(mb->block_chroma_ac[i][j], 15);
				}
			}
		}
	}
}

void h264_print_slice_data(struct h264_slice *slice) {
	printf("Slice data:\n");
	int mb = slice->first_mb_in_slice * (1 + slice->mbaff_frame_flag);
	while (1) {
		if (slice->mbaff_frame_flag)
			printf("\tMacroblock %d (%d, %d, %d):\n", mb, mb/2 % slice->pic_width_in_mbs, mb/2 / slice->pic_width_in_mbs, mb%2);
		else
			printf("\tMacroblock %d (%d, %d):\n", mb, mb % slice->pic_width_in_mbs, mb / slice->pic_width_in_mbs);
		h264_print_macroblock(slice, &slice->mbs[mb]);
		if (mb == slice->last_mb_in_slice)
			break;
		mb = h264_next_mb_addr(slice, mb);
	}
}
