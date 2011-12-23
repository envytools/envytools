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
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

void h264_del_seqparm(struct h264_seqparm *seqparm) {
	int i, j;
	if (seqparm->vui) {
		free(seqparm->vui->nal_hrd_parameters);
		free(seqparm->vui->vcl_hrd_parameters);
		free(seqparm->vui);
	}
	if (seqparm->is_mvc) {
		free(seqparm->views);
		for (i = 0; i <= seqparm->num_level_values_signalled_minus1; i++) {
			for (j = 0; j <= seqparm->levels[i].num_applicable_ops_minus1; j++) {
				free(seqparm->levels[i].applicable_ops[j].target_view_id);
			}
			free(seqparm->levels[i].applicable_ops);
		}
		free(seqparm->levels);
	}
	free(seqparm);
}

void h264_del_picparm(struct h264_picparm *picparm) {
	free(picparm->slice_group_id);
	free(picparm);
}

void h264_del_slice(struct h264_slice *slice) {
	free(slice->dec_ref_pic_marking.mmcos);
	free(slice->dec_ref_base_pic_marking.mmcos);
	free(slice->mbs);
	free(slice);
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

int h264_hrd_parameters(struct bitstream *str, struct h264_hrd_parameters *hrd) {
	if (vs_ue(str, &hrd->cpb_cnt_minus1)) return 1;
	if (hrd->cpb_cnt_minus1 > 31) {
		fprintf(stderr, "cpb_cnt_minus1 out of range\n");
		return 1;
	}
	if (vs_u(str, &hrd->bit_rate_scale, 4)) return 1;
	if (vs_u(str, &hrd->cpb_size_scale, 4)) return 1;
	int i;
	for (i = 0; i <= hrd->cpb_cnt_minus1; i++) {
		if (vs_ue(str, &hrd->bit_rate_value_minus1[i])) return 1;
		if (vs_ue(str, &hrd->cpb_size_value_minus1[i])) return 1;
		if (vs_u(str, &hrd->cbr_flag[i], 1)) return 1;
	}
	if (vs_u(str, &hrd->initial_cpb_removal_delay_length_minus1, 5)) return 1;
	if (vs_u(str, &hrd->cpb_removal_delay_length_minus1, 5)) return 1;
	if (vs_u(str, &hrd->dpb_output_delay_length_minus1, 5)) return 1;
	if (vs_u(str, &hrd->time_offset_length, 5)) return 1;
	return 0;
}

static const uint32_t aspect_ratios[][2] = {
	{ 0, 0 },
	{ 1, 1 },
	{ 12, 11 },
	{ 10, 11 },
	{ 16, 11 },
	{ 40, 33 },
	{ 24, 11 },
	{ 20, 11 },
	{ 32, 11 },
	{ 80, 33 },
	{ 18, 11 },
	{ 15, 11 },
	{ 64, 33 },
	{ 160, 99 },
	{ 4, 3 },
	{ 3, 2 },
	{ 2, 1 },
};

int h264_vui_parameters(struct bitstream *str, struct h264_vui *vui) {
	if (vs_u(str, &vui->aspect_ratio_present_flag, 1)) return 1;
	if (vui->aspect_ratio_present_flag) {
		if (vs_u(str, &vui->aspect_ratio_idc, 8)) return 1;
		if (vui->aspect_ratio_idc == 255) {
			if (vs_u(str, &vui->sar_width, 16)) return 1;
			if (vs_u(str, &vui->sar_height, 16)) return 1;
		} else {
			if (vui->aspect_ratio_idc < ARRAY_SIZE(aspect_ratios)) {
				if (vs_infer(str, &vui->sar_width, aspect_ratios[vui->aspect_ratio_idc][0])) return 1;
				if (vs_infer(str, &vui->sar_width, aspect_ratios[vui->aspect_ratio_idc][1])) return 1;
			} else {
				fprintf(stderr, "WARNING: unknown aspect_ratio_idc %d\n", vui->aspect_ratio_idc);
				if (vs_infer(str, &vui->sar_width, 0)) return 1;
				if (vs_infer(str, &vui->sar_width, 0)) return 1;
			}
		}
	} else {
		if (vs_infer(str, &vui->aspect_ratio_idc, 0)) return 1;
		if (vs_infer(str, &vui->sar_width, 0)) return 1;
		if (vs_infer(str, &vui->sar_width, 0)) return 1;
	}
	if (vs_u(str, &vui->overscan_info_present_flag, 1)) return 1;
	if (vui->overscan_info_present_flag) {
		if (vs_u(str, &vui->overscan_appropriate_flag, 1)) return 1;
	}
	if (vs_u(str, &vui->video_signal_type_present_flag, 1)) return 1;
	if (vui->video_signal_type_present_flag) {
		if (vs_u(str, &vui->video_format, 3)) return 1;
		if (vs_u(str, &vui->video_full_range_flag, 1)) return 1;
		if (vs_u(str, &vui->colour_description_present_flag, 1)) return 1;
	} else {
		if (vs_infer(str, &vui->video_format, 5)) return 1;
		if (vs_infer(str, &vui->video_full_range_flag, 0)) return 1;
		if (vs_infer(str, &vui->colour_description_present_flag, 0)) return 1;
	}
	if (vui->colour_description_present_flag) {
		if (vs_u(str, &vui->colour_primaries, 8)) return 1;
		if (vs_u(str, &vui->transfer_characteristics, 8)) return 1;
		if (vs_u(str, &vui->matrix_coefficients, 8)) return 1;
	} else {
		if (vs_infer(str, &vui->colour_primaries, 2)) return 1;
		if (vs_infer(str, &vui->transfer_characteristics, 2)) return 1;
		if (vs_infer(str, &vui->matrix_coefficients, 2)) return 1;
	}
	if (vs_u(str, &vui->chroma_loc_info_present_flag, 1)) return 1;
	if (vui->chroma_loc_info_present_flag) {
		if (vs_ue(str, &vui->chroma_sample_loc_type_top_field)) return 1;
		if (vs_ue(str, &vui->chroma_sample_loc_type_bottom_field)) return 1;
	} else {
		if (vs_infer(str, &vui->chroma_sample_loc_type_top_field, 0)) return 1;
		if (vs_infer(str, &vui->chroma_sample_loc_type_bottom_field, 0)) return 1;
	}
	if (vs_u(str, &vui->timing_info_present_flag, 1)) return 1;
	if (vui->timing_info_present_flag) {
		if (vs_u(str, &vui->num_units_in_tick, 32)) return 1;
		if (vs_u(str, &vui->time_scale, 32)) return 1;
		if (vs_u(str, &vui->fixed_frame_rate_flag, 1)) return 1;
	} else {
		if (vs_infer(str, &vui->fixed_frame_rate_flag, 0)) return 1;
	}
	uint32_t tmp;
	tmp = !!vui->nal_hrd_parameters;
	if (vs_u(str, &tmp, 1)) return 1;
	if (tmp) {
		if (str->dir == VS_DECODE)
			vui->nal_hrd_parameters = calloc(sizeof *vui->nal_hrd_parameters, 1);
		if (h264_hrd_parameters(str, vui->nal_hrd_parameters)) return 1;
	}
	tmp = !!vui->vcl_hrd_parameters;
	if (vs_u(str, &tmp, 1)) return 1;
	if (tmp) {
		if (str->dir == VS_DECODE)
			vui->vcl_hrd_parameters = calloc(sizeof *vui->vcl_hrd_parameters, 1);
		if (h264_hrd_parameters(str, vui->vcl_hrd_parameters)) return 1;
	}
	if (vui->nal_hrd_parameters || vui->vcl_hrd_parameters) {
		if (vs_u(str, &vui->low_delay_hrd_flag, 1)) return 1;
	}
	if (vs_u(str, &vui->pic_struct_present_flag, 1)) return 1;
	if (vs_u(str, &vui->bitstream_restriction_present_flag, 1)) return 1;
	if (vui->bitstream_restriction_present_flag) {
		if (vs_u(str, &vui->motion_vectors_over_pic_bounduaries_flag, 1)) return 1;
		if (vs_ue(str, &vui->max_bytes_per_pic_denom)) return 1;
		if (vs_ue(str, &vui->max_bits_per_mb_denom)) return 1;
		if (vs_ue(str, &vui->log2_max_mv_length_horizontal)) return 1;
		if (vs_ue(str, &vui->log2_max_mv_length_vertical)) return 1;
		if (vs_ue(str, &vui->num_reorder_frames)) return 1;
		if (vs_ue(str, &vui->max_dec_frame_buffering)) return 1;
	} else {
		if (vs_infer(str, &vui->motion_vectors_over_pic_bounduaries_flag, 1)) return 1;
		if (vs_infer(str, &vui->max_bytes_per_pic_denom, 2)) return 1;
		if (vs_infer(str, &vui->max_bits_per_mb_denom, 1)) return 1;
		if (vs_infer(str, &vui->log2_max_mv_length_horizontal, 16)) return 1;
		if (vs_infer(str, &vui->log2_max_mv_length_vertical, 16)) return 1;
		/* XXX: not entirely correct */
		if (vs_infer(str, &vui->num_reorder_frames, 16)) return 1;
		if (vs_infer(str, &vui->max_dec_frame_buffering, 16)) return 1;
	}
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
		if (h264_vui_parameters(str, seqparm->vui)) return 1;
	} else {
		seqparm->vui = 0;
	}
	return 0;
}

int h264_svc_vui_parameters(struct bitstream *str, struct h264_vui *vui) {
	/* XXX */
	abort();
	return 0;
}

int h264_seqparm_svc(struct bitstream *str, struct h264_seqparm *seqparm) {
	if (str->dir == VS_DECODE)
		seqparm->is_svc = 1;
	if (vs_u(str, &seqparm->inter_layer_deblocking_filter_control_present_flag, 1)) return 1;
	if (vs_u(str, &seqparm->extended_spatial_scalability_idc, 2)) return 1;
	if (seqparm->chroma_format_idc == 1 || seqparm->chroma_format_idc == 2) {
		if (vs_u(str, &seqparm->chroma_phase_x_plus1_flag, 1)) return 1;
	} else {
		if (vs_infer(str, &seqparm->chroma_phase_x_plus1_flag, 1)) return 1;
	}
	if (seqparm->chroma_format_idc == 1) {
		if (vs_u(str, &seqparm->chroma_phase_y_plus1, 2)) return 1;
	} else {
		if (vs_infer(str, &seqparm->chroma_phase_y_plus1, 1)) return 1;
	}
	if (seqparm->extended_spatial_scalability_idc == 1) {
		if (seqparm->chroma_format_idc && !seqparm->separate_colour_plane_flag) {
			if (vs_u(str, &seqparm->seq_ref_layer_chroma_phase_x_plus1_flag, 1)) return 1;
			if (vs_u(str, &seqparm->seq_ref_layer_chroma_phase_y_plus1, 2)) return 1;
		} else {
			if (vs_infer(str, &seqparm->seq_ref_layer_chroma_phase_x_plus1_flag, 1)) return 1;
			if (vs_infer(str, &seqparm->seq_ref_layer_chroma_phase_y_plus1, 1)) return 1;
		}
		if (vs_se(str, &seqparm->seq_ref_layer_left_offset)) return 1;
		if (vs_se(str, &seqparm->seq_ref_layer_top_offset)) return 1;
		if (vs_se(str, &seqparm->seq_ref_layer_right_offset)) return 1;
		if (vs_se(str, &seqparm->seq_ref_layer_bottom_offset)) return 1;
	} else {
		if (vs_infer(str, &seqparm->seq_ref_layer_chroma_phase_x_plus1_flag, seqparm->chroma_phase_x_plus1_flag)) return 1;
		if (vs_infer(str, &seqparm->seq_ref_layer_chroma_phase_y_plus1, seqparm->chroma_phase_y_plus1)) return 1;
		if (vs_infer(str, &seqparm->seq_ref_layer_left_offset, 0)) return 1;
		if (vs_infer(str, &seqparm->seq_ref_layer_top_offset, 0)) return 1;
		if (vs_infer(str, &seqparm->seq_ref_layer_right_offset, 0)) return 1;
		if (vs_infer(str, &seqparm->seq_ref_layer_bottom_offset, 0)) return 1;
	}
	if (vs_u(str, &seqparm->seq_tcoeff_level_prediction_flag, 1)) return 1;
	if (seqparm->seq_tcoeff_level_prediction_flag) {
		if (vs_u(str, &seqparm->adaptive_tcoeff_level_prediction_flag, 1)) return 1;
	} else {
		if (vs_infer(str, &seqparm->adaptive_tcoeff_level_prediction_flag, 0)) return 1;
	}
	if (vs_u(str, &seqparm->slice_header_restriction_flag, 1)) return 1;
	uint32_t svc_vui_parameters_present_flag = !!(seqparm->svc_vui);
	if (vs_u(str, &svc_vui_parameters_present_flag, 1)) return 1;
	if (svc_vui_parameters_present_flag) {
		if (str->dir == VS_DECODE) {
			seqparm->svc_vui = calloc (sizeof *seqparm->svc_vui, 1);
		}
		if (h264_svc_vui_parameters(str, seqparm->svc_vui)) return 1;
	} else {
		seqparm->svc_vui = 0;
	}
	return 0;
}

int h264_mvc_vui_parameters(struct bitstream *str, struct h264_vui *vui) {
	/* XXX */
	abort();
	return 0;
}

int h264_seqparm_mvc(struct bitstream *str, struct h264_seqparm *seqparm) {
	uint32_t bit_equal_to_one = 1;
	int i, j, k;
	if (str->dir == VS_DECODE)
		seqparm->is_mvc = 1;
	if (vs_u(str, &bit_equal_to_one, 1)) return 1;
	if (!bit_equal_to_one) {
		fprintf(stderr, "bit_equal_to_one invalid\n");
		return 1;
	}
	if (vs_ue(str, &seqparm->num_views_minus1)) return 1;
	if (str->dir == VS_DECODE)
		seqparm->views = calloc(sizeof *seqparm->views, seqparm->num_views_minus1 + 1);
	for (i = 0; i <= seqparm->num_views_minus1; i++)
		if (vs_ue(str, &seqparm->views[i].view_id)) return 1;
	for (i = 1; i <= seqparm->num_views_minus1; i++) {
		if (vs_ue(str, &seqparm->views[i].num_anchor_refs_l0)) return 1;
		if (seqparm->views[i].num_anchor_refs_l0 > 15) {
			fprintf (stderr, "num_anchor_refs_l0 over limit\n");
			return 1;
		}
		for (j = 0; j < seqparm->views[i].num_anchor_refs_l0; j++)
			if (vs_ue(str, &seqparm->views[i].anchor_ref_l0[j])) return 1;
		if (vs_ue(str, &seqparm->views[i].num_anchor_refs_l1)) return 1;
		if (seqparm->views[i].num_anchor_refs_l1 > 15) {
			fprintf (stderr, "num_anchor_refs_l1 over limit\n");
			return 1;
		}
		for (j = 0; j < seqparm->views[i].num_anchor_refs_l1; j++)
			if (vs_ue(str, &seqparm->views[i].anchor_ref_l1[j])) return 1;
	}
	for (i = 1; i <= seqparm->num_views_minus1; i++) {
		if (vs_ue(str, &seqparm->views[i].num_non_anchor_refs_l0)) return 1;
		if (seqparm->views[i].num_non_anchor_refs_l0 > 15) {
			fprintf (stderr, "num_non_anchor_refs_l0 over limit\n");
			return 1;
		}
		for (j = 0; j < seqparm->views[i].num_non_anchor_refs_l0; j++)
			if (vs_ue(str, &seqparm->views[i].non_anchor_ref_l0[j])) return 1;
		if (vs_ue(str, &seqparm->views[i].num_non_anchor_refs_l1)) return 1;
		if (seqparm->views[i].num_non_anchor_refs_l1 > 15) {
			fprintf (stderr, "num_non_anchor_refs_l1 over limit\n");
			return 1;
		}
		for (j = 0; j < seqparm->views[i].num_non_anchor_refs_l1; j++)
			if (vs_ue(str, &seqparm->views[i].non_anchor_ref_l1[j])) return 1;
	}
	if (vs_ue(str, &seqparm->num_level_values_signalled_minus1)) return 1;
	if (str->dir == VS_DECODE)
		seqparm->levels = calloc(sizeof *seqparm->levels, seqparm->num_level_values_signalled_minus1 + 1);
	for (i = 0; i <= seqparm->num_level_values_signalled_minus1; i++) {
		if (vs_u(str, &seqparm->levels[i].level_idc, 8)) return 1;
		if (vs_ue(str, &seqparm->levels[i].num_applicable_ops_minus1)) return 1;
		if (str->dir == VS_DECODE)
			seqparm->levels[i].applicable_ops = calloc(sizeof *seqparm->levels[i].applicable_ops, seqparm->levels[i].num_applicable_ops_minus1 + 1);
		for (j = 0; j <= seqparm->levels[i].num_applicable_ops_minus1; j++) {
			if (vs_u(str, &seqparm->levels[i].applicable_ops[j].temporal_id, 3)) return 1;
			if (vs_ue(str, &seqparm->levels[i].applicable_ops[j].num_target_views_minus1)) return 1;
			if (str->dir == VS_DECODE)
				seqparm->levels[i].applicable_ops[j].target_view_id = calloc(sizeof *seqparm->levels[i].applicable_ops[j].target_view_id, seqparm->levels[i].applicable_ops[j].num_target_views_minus1 + 1);
			for (k = 0; k <= seqparm->levels[i].applicable_ops[j].num_target_views_minus1; k++)
				if (vs_ue(str, &seqparm->levels[i].applicable_ops[j].target_view_id[k])) return 1;
			if (vs_ue(str, &seqparm->levels[i].applicable_ops[j].num_views_minus1)) return 1;
		}
	}
	uint32_t mvc_vui_parameters_present_flag = !!(seqparm->mvc_vui);
	if (vs_u(str, &mvc_vui_parameters_present_flag, 1)) return 1;
	if (mvc_vui_parameters_present_flag) {
		if (str->dir == VS_DECODE) {
			seqparm->mvc_vui = calloc (sizeof *seqparm->mvc_vui, 1);
		}
		if (h264_mvc_vui_parameters(str, seqparm->mvc_vui)) return 1;
	} else {
		seqparm->mvc_vui = 0;
	}
	return 0;
}

int h264_seqparm_ext(struct bitstream *str, struct h264_seqparm **seqparms, uint32_t *pseq_parameter_set_id) {
	if (vs_ue(str, pseq_parameter_set_id)) return 1;
	if (*pseq_parameter_set_id > 31) {
		fprintf(stderr, "seq_parameter_set_id out of bounds\n");
		return 1;
	}
	struct h264_seqparm *seqparm = seqparms[*pseq_parameter_set_id];
	if (!seqparm) {
		fprintf(stderr, "seqparm extension for nonexistent seqparm\n");
		return 1;
	}
	if (vs_ue(str, &seqparm->aux_format_idc)) return 1;
	if (seqparm->aux_format_idc) {
		if (vs_ue(str, &seqparm->bit_depth_aux_minus8)) return 1;
		if (vs_u(str, &seqparm->alpha_incr_flag, 1)) return 1;
		if (vs_u(str, &seqparm->alpha_opaque_value, seqparm->bit_depth_aux_minus8 + 9)) return 1;
		if (vs_u(str, &seqparm->alpha_transparent_value, seqparm->bit_depth_aux_minus8 + 9)) return 1;
	}
	uint32_t additional_extension_flag = 0;
	if (vs_u(str, &additional_extension_flag, 1)) return 1;
	if (additional_extension_flag) {
		fprintf(stderr, "WARNING: additional data in seqparm extension\n");
		while (vs_has_more_data(str)) {
			if (vs_u(str, &additional_extension_flag, 1)) return 1;
		}
	}
	if (str->dir == VS_DECODE)
		seqparm->has_ext = 1;
	return 0;
}

int h264_picparm(struct bitstream *str, struct h264_seqparm **seqparms, struct h264_seqparm **subseqparms, struct h264_picparm *picparm) {
	int i;
	if (vs_ue(str, &picparm->pic_parameter_set_id)) return 1;
	if (vs_ue(str, &picparm->seq_parameter_set_id)) return 1;
	if (picparm->seq_parameter_set_id > 31) {
		fprintf(stderr, "seq_parameter_set_id out of bounds\n");
		return 1;
	}
	if (vs_u(str, &picparm->entropy_coding_mode_flag, 1)) return 1;
	if (vs_u(str, &picparm->bottom_field_pic_order_in_frame_present_flag, 1)) return 1;
	if (vs_ue(str, &picparm->num_slice_groups_minus1)) return 1;
	if (picparm->num_slice_groups_minus1) {
		if (picparm->num_slice_groups_minus1 > 7) {
			fprintf(stderr, "num_slice_groups_minus1 over limit\n");
			return 1;
		}
		if (vs_ue(str, &picparm->slice_group_map_type)) return 1;
		switch (picparm->slice_group_map_type) {
			case H264_SLICE_GROUP_MAP_INTERLEAVED:
				for (i = 0; i <= picparm->num_slice_groups_minus1; i++)
					if (vs_ue(str, &picparm->run_length_minus1[i])) return 1;

				break;
			case H264_SLICE_GROUP_MAP_DISPERSED:
				break;
			case H264_SLICE_GROUP_MAP_FOREGROUND:
				for (i = 0; i < picparm->num_slice_groups_minus1; i++) {
					if (vs_ue(str, &picparm->top_left[i])) return 1;
					if (vs_ue(str, &picparm->bottom_right[i])) return 1;

				}
				break;
			case H264_SLICE_GROUP_MAP_CHANGING_BOX:
			case H264_SLICE_GROUP_MAP_CHANGING_VERTICAL:
			case H264_SLICE_GROUP_MAP_CHANGING_HORIZONTAL:
				if (vs_u(str, &picparm->slice_group_change_direction_flag, 1)) return 1;
				if (vs_ue(str, &picparm->slice_group_change_rate_minus1)) return 1;
				break;
			case H264_SLICE_GROUP_MAP_EXPLICIT:
				if (vs_ue(str, &picparm->pic_size_in_map_units_minus1)) return 1;
				if (str->dir == VS_DECODE)
					picparm->slice_group_id = calloc(sizeof *picparm->slice_group_id, picparm->pic_size_in_map_units_minus1 + 1);
				static const int sizes[8] = { 0, 1, 2, 2, 3, 3, 3, 3 };
				for (i = 0; i <= picparm->pic_size_in_map_units_minus1; i++)
					if (vs_u(str, &picparm->slice_group_id[i], sizes[picparm->num_slice_groups_minus1])) return 1;
				break;
			default:
				fprintf(stderr, "Unknown slice_group_map_type %d!\n", picparm->slice_group_map_type);
				return 1;
		}
	}
	if (vs_ue(str, &picparm->num_ref_idx_l0_default_active_minus1)) return 1;
	if (vs_ue(str, &picparm->num_ref_idx_l1_default_active_minus1)) return 1;
	if (vs_u(str, &picparm->weighted_pred_flag, 1)) return 1;
	if (vs_u(str, &picparm->weighted_bipred_idc, 2)) return 1;
	if (vs_se(str, &picparm->pic_init_qp_minus26)) return 1;
	if (vs_se(str, &picparm->pic_init_qs_minus26)) return 1;
	if (vs_se(str, &picparm->chroma_qp_index_offset)) return 1;
	if (vs_u(str, &picparm->deblocking_filter_control_present_flag, 1)) return 1;
	if (vs_u(str, &picparm->constrained_intra_pred_flag, 1)) return 1;
	if (vs_u(str, &picparm->redundant_pic_cnt_present_flag, 1)) return 1;
	int more;
	if (str->dir == VS_ENCODE) {
		more = picparm->transform_8x8_mode_flag || picparm->pic_scaling_matrix_present_flag || picparm->second_chroma_qp_index_offset != picparm->chroma_qp_index_offset;
	} else {
		more = vs_has_more_data(str);
	}
	if (more) {
		if (vs_u(str, &picparm->transform_8x8_mode_flag, 1)) return 1;
		if (vs_u(str, &picparm->pic_scaling_matrix_present_flag, 1)) return 1;
		if (picparm->pic_scaling_matrix_present_flag) {
			/* brain damage workaround start */
			struct h264_seqparm *seqparm = seqparms[picparm->seq_parameter_set_id], *subseqparm = subseqparms[picparm->seq_parameter_set_id];;
			if (seqparm) {
				picparm->chroma_format_idc = seqparm->chroma_format_idc;
				if (subseqparm) {
					if (subseqparm->chroma_format_idc != picparm->chroma_format_idc) {
						fprintf(stderr, "conflicting chroma_format_idc between seqparm and subseqparm, please complain to ITU/ISO about retarded spec and to bitstream source about retarded bitstream.\n");
						return 1;
					}
				}
			} else if (subseqparm) {
				picparm->chroma_format_idc = subseqparm->chroma_format_idc;
			} else {
				fprintf(stderr, "picparm for nonexistent seqparm/subseqparm!\n");
				return 1;
			}
			/* brain damage workaround end */
			int i;
			for (i = 0; i < (picparm->chroma_format_idc == 3 ? 12 : 8); i++) {
				if (vs_u(str, &picparm->pic_scaling_list_present_flag[i], 1)) return 1;
				if (picparm->pic_scaling_list_present_flag[i]) {
					if (i < 6) {
						if (h264_scaling_list(str, picparm->pic_scaling_list_4x4[i], 16, &picparm->use_default_scaling_matrix_flag[i])) return 1;;
					} else {
						if (h264_scaling_list(str, picparm->pic_scaling_list_8x8[i-6], 64, &picparm->use_default_scaling_matrix_flag[i])) return 1;;
					}
				}
			}
		}
		if (vs_se(str, &picparm->second_chroma_qp_index_offset)) return 1;
	} else {
		if (vs_infer(str, &picparm->transform_8x8_mode_flag, 0)) return 1;
		if (vs_infer(str, &picparm->pic_scaling_matrix_present_flag, 0)) return 1;
		if (vs_infer(str, &picparm->second_chroma_qp_index_offset, picparm->chroma_qp_index_offset)) return 1;
	}
	return 0;
}

int h264_ref_pic_list_modification(struct bitstream *str, struct h264_slice *slice, struct h264_ref_pic_list_modification *list) {
	int i;
	if (vs_u(str, &list->flag, 1)) return 1;
	if (list->flag) {
		i = 0;
		do {
			if (vs_ue(str, &list->list[i].op)) return 1;
			if (list->list[i].op != 3) {
				if (vs_ue(str, &list->list[i].param)) return 1;
				if (i == 32) {
					fprintf(stderr, "Too many ref_pic_list_modification entries\n");
					return 1;
				}
			}
		} while (list->list[i++].op != 3);
	} else {
		if (vs_infer(str, &list->list[0].op, 3)) return 1;
	}
	return 0;
}

int h264_dec_ref_pic_marking(struct bitstream *str, int idr_pic_flag, struct h264_dec_ref_pic_marking *ref) {
	if (idr_pic_flag) {
		if (vs_u(str, &ref->no_output_of_prior_pics_flag, 1)) return 1;
		if (vs_u(str, &ref->long_term_reference_flag, 1)) return 1;
	} else {
		if (vs_u(str, &ref->adaptive_ref_pic_marking_mode_flag, 1)) return 1;
		if (ref->adaptive_ref_pic_marking_mode_flag) {
			struct h264_mmco_entry mmco;
			int i = 0;
			do {
				if (str->dir == VS_ENCODE)
					mmco = ref->mmcos[i];
				if (vs_ue(str, &mmco.memory_management_control_operation)) return 1;
				switch (mmco.memory_management_control_operation) {
					case H264_MMCO_END:
						break;
					case H264_MMCO_FORGET_SHORT:
						if (vs_ue(str, &mmco.difference_of_pic_nums_minus1)) return 1;
						break;
					case H264_MMCO_FORGET_LONG:
						if (vs_ue(str, &mmco.long_term_pic_num)) return 1;
						break;
					case H264_MMCO_SHORT_TO_LONG:
						if (vs_ue(str, &mmco.difference_of_pic_nums_minus1)) return 1;
						if (vs_ue(str, &mmco.long_term_frame_idx)) return 1;
						break;
					case H264_MMCO_FORGET_LONG_MANY:
						if (vs_ue(str, &mmco.max_long_term_frame_idx_plus1)) return 1;
						break;
					case H264_MMCO_FORGET_ALL:
						break;
					case H264_MMCO_THIS_TO_LONG:
						if (vs_ue(str, &mmco.long_term_frame_idx)) return 1;
						break;
					default:
						fprintf (stderr, "Unknown MMCO %d\n", mmco.memory_management_control_operation);
						return 1;
				}
				if (str->dir == VS_DECODE) 
					ADDARRAY(ref->mmcos, mmco);
				i++;
			} while (mmco.memory_management_control_operation != H264_MMCO_END);
		}
	}
	return 0;
}

int h264_dec_ref_base_pic_marking(struct bitstream *str, struct h264_nal_svc_header *svc, struct h264_dec_ref_base_pic_marking *ref) {
	if (vs_u(str, &ref->store_ref_base_pic_flag, 1)) return 1;
	if ((svc->use_ref_base_pic_flag || ref->store_ref_base_pic_flag) && !svc->idr_flag) {
		if (vs_u(str, &ref->adaptive_ref_base_pic_marking_mode_flag, 1)) return 1;
		if (ref->adaptive_ref_base_pic_marking_mode_flag) {
			struct h264_mmco_entry mmco;
			int i = 0;
			do {
				if (str->dir == VS_ENCODE)
					mmco = ref->mmcos[i];
				if (vs_ue(str, &mmco.memory_management_control_operation)) return 1;
				switch (mmco.memory_management_control_operation) {
					case H264_MMCO_END:
						break;
					case H264_MMCO_FORGET_SHORT:
						if (vs_ue(str, &mmco.difference_of_pic_nums_minus1)) return 1;
						break;
					case H264_MMCO_FORGET_LONG:
						if (vs_ue(str, &mmco.long_term_pic_num)) return 1;
						break;
					default:
						fprintf (stderr, "Unknown MMCO %d\n", mmco.memory_management_control_operation);
						return 1;
				}
				if (str->dir == VS_DECODE) 
					ADDARRAY(ref->mmcos, mmco);
				i++;
			} while (mmco.memory_management_control_operation != H264_MMCO_END);
		}
	}
	return 0;
}

int h264_slice_header(struct bitstream *str, struct h264_seqparm **seqparms, struct h264_picparm **picparms, struct h264_slice *slice) {
	if (vs_ue(str, &slice->first_mb_in_slice)) return 1;
	uint32_t slice_type = slice->slice_type + slice->slice_all_same * 5;
	if (vs_ue(str, &slice_type)) return 1;
	slice->slice_type = slice_type % 5;
	slice->slice_all_same = slice_type / 5;
	uint32_t pic_parameter_set_id;
	if (str->dir == VS_ENCODE)
		pic_parameter_set_id = slice->picparm->pic_parameter_set_id;
	if (vs_ue(str, &pic_parameter_set_id)) return 1;
	if (str->dir == VS_DECODE) {
		if (pic_parameter_set_id > 255) {
			fprintf(stderr, "pic_parameter_set_id out of range\n");
			return 1;
		}
		slice->picparm = picparms[pic_parameter_set_id];
		if (!slice->picparm) {
			fprintf(stderr, "pic_parameter_set_id doesn't specify a picparm\n");
			return 1;
		}
		slice->seqparm = seqparms[slice->picparm->seq_parameter_set_id];
		if (!slice->seqparm) {
			fprintf(stderr, "seq_parameter_set_id doesn't specify a seqparm\n");
			return 1;
		}
		if (slice->nal_unit_type == H264_NAL_UNIT_TYPE_SLICE_AUX) {
			slice->chroma_array_type = 0;
			slice->bit_depth_luma_minus8 = slice->seqparm->bit_depth_aux_minus8;
			slice->bit_depth_chroma_minus8 = 0;
		} else {
			slice->chroma_array_type = (slice->seqparm->separate_colour_plane_flag?0:slice->seqparm->chroma_format_idc);
			slice->bit_depth_luma_minus8 = slice->seqparm->bit_depth_luma_minus8;
			slice->bit_depth_chroma_minus8 = slice->seqparm->bit_depth_chroma_minus8;
		}
		slice->pic_width_in_mbs = slice->seqparm->pic_width_in_mbs_minus1 + 1;
		switch (slice->chroma_array_type) {
			case 0:
				slice->mbwidthc = 0;
				slice->mbheightc = 0;
				break;
			case 1:
				slice->mbwidthc = 8;
				slice->mbheightc = 8;
				break;
			case 2:
				slice->mbwidthc = 8;
				slice->mbheightc = 16;
				break;
			case 3:
				slice->mbwidthc = 16;
				slice->mbheightc = 16;
				break;
		}
	}
	if (slice->seqparm->separate_colour_plane_flag)
		if (vs_u(str, &slice->colour_plane_id, 2)) return 1;
	if (vs_u(str, &slice->frame_num, slice->seqparm->log2_max_frame_num_minus4 + 4)) return 1;
	if (!slice->seqparm->frame_mbs_only_flag) {
		if (vs_u(str, &slice->field_pic_flag, 1)) return 1;
	} else {
		if (vs_infer(str, &slice->field_pic_flag, 0)) return 1;
	}
	if (slice->field_pic_flag) {
		if (vs_u(str, &slice->bottom_field_flag, 1)) return 1;
	} else {
		if (vs_infer(str, &slice->bottom_field_flag, 0)) return 1;
	}
	slice->pic_height_in_mbs = (slice->seqparm->pic_height_in_map_units_minus1 + 1);
	if (!slice->seqparm->frame_mbs_only_flag)
		slice->pic_height_in_mbs *= 2;
	if (slice->field_pic_flag)
		slice->pic_height_in_mbs /= 2;
	slice->pic_size_in_mbs = slice->pic_width_in_mbs * slice->pic_height_in_mbs;
	if (str->dir == VS_DECODE) {
		slice->mbaff_frame_flag = slice->seqparm->mb_adaptive_frame_field_flag && !slice->field_pic_flag;
	}
	if (slice->idr_pic_flag) {
		if (vs_ue(str, &slice->idr_pic_id)) return 1;
	}
	switch (slice->seqparm->pic_order_cnt_type) {
		case 0:
			if (vs_u(str, &slice->pic_order_cnt_lsb, slice->seqparm->log2_max_pic_order_cnt_lsb_minus4 + 4)) return 1;
			if (slice->picparm->bottom_field_pic_order_in_frame_present_flag && !slice->field_pic_flag) {
				if (vs_se(str, &slice->delta_pic_order_cnt_bottom)) return 1;
			} else {
				if (vs_infer(str, &slice->delta_pic_order_cnt_bottom, 0)) return 1;
			}
			break;
		case 1:
			if (!slice->seqparm->delta_pic_order_always_zero_flag) {
				if (vs_se(str, &slice->delta_pic_order_cnt[0])) return 1;
				if (slice->picparm->bottom_field_pic_order_in_frame_present_flag && !slice->field_pic_flag) {
					if (vs_se(str, &slice->delta_pic_order_cnt[1])) return 1;
				} else {
					if (vs_infer(str, &slice->delta_pic_order_cnt[1], 0)) return 1;
				}
			} else {
				if (vs_infer(str, &slice->delta_pic_order_cnt[0], 0)) return 1;
				if (vs_infer(str, &slice->delta_pic_order_cnt[1], 0)) return 1;
			}
			break;
	}
	if (slice->picparm->redundant_pic_cnt_present_flag) {
		if (vs_ue(str, &slice->redundant_pic_cnt)) return 1;
	} else {
		if (vs_infer(str, &slice->redundant_pic_cnt, 0)) return 1;
	}
	if (!slice->seqparm->is_svc || slice->svc.quality_id == 0) {
		if (slice->slice_type == H264_SLICE_TYPE_B)
			if (vs_u(str, &slice->direct_spatial_mb_pred_flag, 1)) return 1;
		if (slice->slice_type != H264_SLICE_TYPE_I && slice->slice_type != H264_SLICE_TYPE_SI) {
			if (vs_u(str, &slice->num_ref_idx_active_override_flag, 1)) return 1;
			if (slice->num_ref_idx_active_override_flag) {
				if (vs_ue(str, &slice->num_ref_idx_l0_active_minus1)) return 1;
				if (slice->slice_type == H264_SLICE_TYPE_B)
					if (vs_ue(str, &slice->num_ref_idx_l1_active_minus1)) return 1;
			} else {
				if (vs_infer(str, &slice->num_ref_idx_l0_active_minus1, slice->picparm->num_ref_idx_l0_default_active_minus1)) return 1;
				if (slice->slice_type == H264_SLICE_TYPE_B)
					if (vs_infer(str, &slice->num_ref_idx_l1_active_minus1, slice->picparm->num_ref_idx_l1_default_active_minus1)) return 1;

			}
			if (slice->num_ref_idx_l0_active_minus1 > 31) {
				fprintf(stderr, "num_ref_idx_l0_active_minus1 out of range\n");
				return 1;
			}
			if (slice->num_ref_idx_l1_active_minus1 > 31) {
				fprintf(stderr, "num_ref_idx_l1_active_minus1 out of range\n");
				return 1;
			}
			/* ref_pic_list_modification */
			if (h264_ref_pic_list_modification(str, slice, &slice->ref_pic_list_modification_l0)) return 1;
			if (slice->slice_type == H264_SLICE_TYPE_B) {
				if (h264_ref_pic_list_modification(str, slice, &slice->ref_pic_list_modification_l1)) return 1;
			}
		}
		if ((slice->picparm->weighted_pred_flag && (slice->slice_type == H264_SLICE_TYPE_P || slice->slice_type == H264_SLICE_TYPE_SP)) || (slice->picparm->weighted_bipred_idc == 1 && slice->slice_type == H264_SLICE_TYPE_B)) {
			if (slice->seqparm->is_svc && !slice->svc.no_inter_layer_pred_flag) {
				if (vs_u(str, &slice->base_pred_weight_table_flag, 1)) return 1;
			} else {
				if (vs_infer(str, &slice->base_pred_weight_table_flag, 0)) return 1;
			}
			if (!slice->base_pred_weight_table_flag)
				if (h264_pred_weight_table(str, slice, &slice->pred_weight_table)) return 1;
		}
		if (slice->nal_ref_idc) {
			if (h264_dec_ref_pic_marking(str, slice->idr_pic_flag, &slice->dec_ref_pic_marking)) return 1;
			if (slice->seqparm->is_svc && !slice->seqparm->slice_header_restriction_flag) {
				if (h264_dec_ref_base_pic_marking(str, &slice->svc, &slice->dec_ref_base_pic_marking)) return 1;
			}
		}
	} else {
		/* XXX: infer me */
	}
	if (slice->picparm->entropy_coding_mode_flag && slice->slice_type != H264_SLICE_TYPE_I && slice->slice_type != H264_SLICE_TYPE_SI) {
		if (vs_ue(str, &slice->cabac_init_idc)) return 1;
		if (slice->cabac_init_idc > 2) {
			fprintf(stderr, "cabac_init_idc out of range!\n");
			return 1;
		}
	}
	if (vs_se(str, &slice->slice_qp_delta)) return 1;
	slice->sliceqpy = slice->picparm->pic_init_qp_minus26 + 26 + slice->slice_qp_delta;
	if (slice->slice_type == H264_SLICE_TYPE_SP)
		if (vs_u(str, &slice->sp_for_switch_flag, 1)) return 1;
	if (slice->slice_type == H264_SLICE_TYPE_SP || slice->slice_type == H264_SLICE_TYPE_SI)
		if (vs_se(str, &slice->slice_qs_delta)) return 1;
	if (slice->picparm->deblocking_filter_control_present_flag) {
		if (vs_ue(str, &slice->disable_deblocking_filter_idc)) return 1;
		if (slice->disable_deblocking_filter_idc != 1) {
			if (vs_se(str, &slice->slice_alpha_c0_offset_div2)) return 1;
			if (vs_se(str, &slice->slice_beta_offset_div2)) return 1;
		} else {
			if (vs_infer(str, &slice->slice_alpha_c0_offset_div2, 0)) return 1;
			if (vs_infer(str, &slice->slice_beta_offset_div2, 0)) return 1;
		}
	} else {
		if (vs_infer(str, &slice->disable_deblocking_filter_idc, 0)) return 1;
		if (vs_infer(str, &slice->slice_alpha_c0_offset_div2, 0)) return 1;
		if (vs_infer(str, &slice->slice_beta_offset_div2, 0)) return 1;
	}
	if (slice->picparm->num_slice_groups_minus1 && slice->picparm->slice_group_map_type >= 3 && slice->picparm->slice_group_map_type <= 5)
		if (vs_u(str, &slice->slice_group_change_cycle, clog2(((slice->seqparm->pic_width_in_mbs_minus1 + 1) * (slice->seqparm->pic_height_in_map_units_minus1 + 1) + slice->picparm->slice_group_change_rate_minus1) / (slice->picparm->slice_group_change_rate_minus1 + 1) + 1))) return 1;
	if (slice->seqparm->is_svc) {
		/* XXX */
		fprintf(stderr, "SVC\n");
		return 1;
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
