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

#ifndef H264_H
#define H264_H

#include "vstream.h"

enum h264_nal_unit_type {
	H264_NAL_UNIT_TYPE_SLICE_NONIDR = 1,
	H264_NAL_UNIT_TYPE_SLICE_PART_A = 2,		/* main data */
	H264_NAL_UNIT_TYPE_SLICE_PART_B = 3,		/* I, SI residual blocks */
	H264_NAL_UNIT_TYPE_SLICE_PART_C = 4,		/* P, B residual blocks */
	H264_NAL_UNIT_TYPE_SLICE_IDR = 5,
	H264_NAL_UNIT_TYPE_SEI = 6,
	H264_NAL_UNIT_TYPE_SEQPARM = 7,
	H264_NAL_UNIT_TYPE_PICPARM = 8,
	H264_NAL_UNIT_TYPE_ACC_UNIT_DELIM = 9,
	H264_NAL_UNIT_TYPE_END_SEQ = 10,
	H264_NAL_UNIT_TYPE_END_STREAM = 11,
	H264_NAL_UNIT_TYPE_FILLER_DATA = 12,
	H264_NAL_UNIT_TYPE_SEQPARM_EXT = 13,
	H264_NAL_UNIT_TYPE_PREFIX_NAL_UNIT = 14,	/* SVC/MVC */
	H264_NAL_UNIT_TYPE_SUBSET_SEQPARM = 15,		/* SVC/MVC */
	H264_NAL_UNIT_TYPE_SLICE_AUX = 19,
	H264_NAL_UNIT_TYPE_SLICE_EXT = 20,		/* SVC/MVC */
};

enum h264_profile_idc {
	H264_PROFILE_CAVLC_444 = 44,
	H264_PROFILE_BASELINE = 66,
	H264_PROFILE_MAIN = 77,
	H264_PROFILE_SCALABLE_BASELINE = 83,
	H264_PROFILE_SCALABLE_HIGH = 86,
	H264_PROFILE_EXTENDED = 88,
	H264_PROFILE_HIGH = 100,
	H264_PROFILE_HIGH_10 = 110,
	H264_PROFILE_MULTIVIEW_HIGH = 118,
	H264_PROFILE_HIGH_422 = 122,
	H264_PROFILE_STEREO_HIGH = 128,
	H264_PROFILE_HIGH_444 = 144,
	H264_PROFILE_HIGH_444_PRED = 244,
};

/* constraints:
 *  0: conforms to baseline / scalable baseline
 *  1: conforms to main / scalable high
 *  2: conforms to extended
 *  3: for BASELINE, MAIN, EXTENDED: level 1b
 *     for HIGH, HIGH_10, HIGH_422, HIGH_444, CAVLC_444, SCALABLE_HIGH: intra-only [must be set for CAVLC_444]
 *  4: conforms to multiview high
 *  5: if MULTIVIEW_HIGH, conforms to stereo high
 */

enum h264_slice_group_map_type {
	H264_SLICE_GROUP_MAP_INTERLEAVED = 0,
	H264_SLICE_GROUP_MAP_DISPERSED = 1,
	H264_SLICE_GROUP_MAP_FOREGROUND = 2,
	H264_SLICE_GROUP_MAP_CHANGING_BOX = 3,
	H264_SLICE_GROUP_MAP_CHANGING_VERTICAL = 4,
	H264_SLICE_GROUP_MAP_CHANGING_HORIZONTAL = 5,
	H264_SLICE_GROUP_MAP_EXPLICIT = 6,
};

enum h264_primary_pic_type {
	H264_PRIMARY_PIC_TYPE_I = 0,
	H264_PRIMARY_PIC_TYPE_P_I = 1,
	H264_PRIMARY_PIC_TYPE_P_B_I = 2,
	H264_PRIMARY_PIC_TYPE_SI = 3,
	H264_PRIMARY_PIC_TYPE_SP_SI = 4,
	H264_PRIMARY_PIC_TYPE_I_SI = 5,
	H264_PRIMARY_PIC_TYPE_P_I_SP_SI = 6,
	H264_PRIMARY_PIC_TYPE_P_B_I_SP_SI = 7,
};

enum h264_ref_pic_mod { /* modification_of_pic_nums_idc */
	H264_REF_PIC_MOD_PIC_NUM_SUB = 0,
	H264_REF_PIC_MOD_PIC_NUM_ADD = 1,
	H264_REF_PIC_MOD_LONG_TERM = 2,
	H264_REF_PIC_MOD_END = 3,
	/* MVC */
	H264_REF_PIC_MOD_VIEW_IDX_SUB = 4,
	H264_REF_PIC_MOD_VIEW_IDX_ADD = 5,
};

enum h264_mmco { /* memory_management_control_operation */
	H264_MMCO_END = 0,
	H264_MMCO_FORGET_SHORT = 1,
	H264_MMCO_FORGET_LONG = 2,
	H264_MMCO_SHORT_TO_LONG = 3,
	H264_MMCO_FORGET_LONG_MANY = 4,
	H264_MMCO_FORGET_ALL = 5,
	H264_MMCO_THIS_TO_LONG = 6,
};

enum h264_slice_type {
	H264_SLICE_TYPE_P = 0,
	H264_SLICE_TYPE_B = 1,
	H264_SLICE_TYPE_I = 2,
	H264_SLICE_TYPE_SP = 3,
	H264_SLICE_TYPE_SI = 4,
};

enum h264_mb_type {
	/* I */
	H264_MB_TYPE_I_BASE = 0,
	H264_MB_TYPE_I_NXN		= H264_MB_TYPE_I_BASE + 0,
	H264_MB_TYPE_I_16X16_0_0_0	= H264_MB_TYPE_I_BASE + 1,
	H264_MB_TYPE_I_16X16_1_0_0	= H264_MB_TYPE_I_BASE + 2,
	H264_MB_TYPE_I_16X16_2_0_0	= H264_MB_TYPE_I_BASE + 3,
	H264_MB_TYPE_I_16X16_3_0_0	= H264_MB_TYPE_I_BASE + 4,
	H264_MB_TYPE_I_16X16_0_1_0	= H264_MB_TYPE_I_BASE + 5,
	H264_MB_TYPE_I_16X16_1_1_0	= H264_MB_TYPE_I_BASE + 6,
	H264_MB_TYPE_I_16X16_2_1_0	= H264_MB_TYPE_I_BASE + 7,
	H264_MB_TYPE_I_16X16_3_1_0	= H264_MB_TYPE_I_BASE + 8,
	H264_MB_TYPE_I_16X16_0_2_0	= H264_MB_TYPE_I_BASE + 9,
	H264_MB_TYPE_I_16X16_1_2_0	= H264_MB_TYPE_I_BASE + 10,
	H264_MB_TYPE_I_16X16_2_2_0	= H264_MB_TYPE_I_BASE + 11,
	H264_MB_TYPE_I_16X16_3_2_0	= H264_MB_TYPE_I_BASE + 12,
	H264_MB_TYPE_I_16X16_0_0_1	= H264_MB_TYPE_I_BASE + 13,
	H264_MB_TYPE_I_16X16_1_0_1	= H264_MB_TYPE_I_BASE + 14,
	H264_MB_TYPE_I_16X16_2_0_1	= H264_MB_TYPE_I_BASE + 15,
	H264_MB_TYPE_I_16X16_3_0_1	= H264_MB_TYPE_I_BASE + 16,
	H264_MB_TYPE_I_16X16_0_1_1	= H264_MB_TYPE_I_BASE + 17,
	H264_MB_TYPE_I_16X16_1_1_1	= H264_MB_TYPE_I_BASE + 18,
	H264_MB_TYPE_I_16X16_2_1_1	= H264_MB_TYPE_I_BASE + 19,
	H264_MB_TYPE_I_16X16_3_1_1	= H264_MB_TYPE_I_BASE + 20,
	H264_MB_TYPE_I_16X16_0_2_1	= H264_MB_TYPE_I_BASE + 21,
	H264_MB_TYPE_I_16X16_1_2_1	= H264_MB_TYPE_I_BASE + 22,
	H264_MB_TYPE_I_16X16_2_2_1	= H264_MB_TYPE_I_BASE + 23,
	H264_MB_TYPE_I_16X16_3_2_1	= H264_MB_TYPE_I_BASE + 24,
	H264_MB_TYPE_I_PCM		= H264_MB_TYPE_I_BASE + 25,
	H264_MB_TYPE_I_END		= H264_MB_TYPE_I_BASE + 26,
	/* SI */
	H264_MB_TYPE_SI_BASE		= H264_MB_TYPE_I_END,
	H264_MB_TYPE_SI			= H264_MB_TYPE_SI_BASE + 0,
	H264_MB_TYPE_SI_END		= H264_MB_TYPE_SI_BASE + 1,
	/* P */
	H264_MB_TYPE_P_BASE		= H264_MB_TYPE_SI_END,
	H264_MB_TYPE_P_L0_16X16		= H264_MB_TYPE_P_BASE + 0,
	H264_MB_TYPE_P_L0_L0_16X8	= H264_MB_TYPE_P_BASE + 1,
	H264_MB_TYPE_P_L0_L0_8X16	= H264_MB_TYPE_P_BASE + 2,
	H264_MB_TYPE_P_8X8		= H264_MB_TYPE_P_BASE + 3,
	H264_MB_TYPE_P_8X8REF0		= H264_MB_TYPE_P_BASE + 4,
	H264_MB_TYPE_P_SKIP		= H264_MB_TYPE_P_BASE + 5,
	H264_MB_TYPE_P_END		= H264_MB_TYPE_P_BASE + 6,
	/* B */
	H264_MB_TYPE_B_BASE		= H264_MB_TYPE_P_END,
	H264_MB_TYPE_B_DIRECT_16X16	= H264_MB_TYPE_B_BASE + 0,
	H264_MB_TYPE_B_L0_16X16		= H264_MB_TYPE_B_BASE + 1,
	H264_MB_TYPE_B_L1_16X16		= H264_MB_TYPE_B_BASE + 2,
	H264_MB_TYPE_B_BI_16X16		= H264_MB_TYPE_B_BASE + 3,
	H264_MB_TYPE_B_L0_L0_16X8	= H264_MB_TYPE_B_BASE + 4,
	H264_MB_TYPE_B_L0_L0_8X16	= H264_MB_TYPE_B_BASE + 5,
	H264_MB_TYPE_B_L1_L1_16X8	= H264_MB_TYPE_B_BASE + 6,
	H264_MB_TYPE_B_L1_L1_8X16	= H264_MB_TYPE_B_BASE + 7,
	H264_MB_TYPE_B_L0_L1_16X8	= H264_MB_TYPE_B_BASE + 8,
	H264_MB_TYPE_B_L0_L1_8X16	= H264_MB_TYPE_B_BASE + 9,
	H264_MB_TYPE_B_L1_L0_16X8	= H264_MB_TYPE_B_BASE + 10,
	H264_MB_TYPE_B_L1_L0_8X16	= H264_MB_TYPE_B_BASE + 11,
	H264_MB_TYPE_B_L0_BI_16X8	= H264_MB_TYPE_B_BASE + 12,
	H264_MB_TYPE_B_L0_BI_8X16	= H264_MB_TYPE_B_BASE + 13,
	H264_MB_TYPE_B_L1_BI_16X8	= H264_MB_TYPE_B_BASE + 14,
	H264_MB_TYPE_B_L1_BI_8X16	= H264_MB_TYPE_B_BASE + 15,
	H264_MB_TYPE_B_BI_L0_16X8	= H264_MB_TYPE_B_BASE + 16,
	H264_MB_TYPE_B_BI_L0_8X16	= H264_MB_TYPE_B_BASE + 17,
	H264_MB_TYPE_B_BI_L1_16X8	= H264_MB_TYPE_B_BASE + 18,
	H264_MB_TYPE_B_BI_L1_8X16	= H264_MB_TYPE_B_BASE + 19,
	H264_MB_TYPE_B_BI_BI_16X8	= H264_MB_TYPE_B_BASE + 20,
	H264_MB_TYPE_B_BI_BI_8X16	= H264_MB_TYPE_B_BASE + 21,
	H264_MB_TYPE_B_8X8		= H264_MB_TYPE_B_BASE + 22,
	H264_MB_TYPE_B_SKIP		= H264_MB_TYPE_B_BASE + 23,
	H264_MB_TYPE_B_END		= H264_MB_TYPE_B_BASE + 24,
	/* for internal use only */
	H264_MB_TYPE_UNAVAIL		= H264_MB_TYPE_B_END,
};

enum h264_sub_mb_type {
	/* P */
	H264_SUB_MB_TYPE_P_BASE = 0,
	H264_SUB_MB_TYPE_P_L0_8X8 = H264_SUB_MB_TYPE_P_BASE + 0,
	H264_SUB_MB_TYPE_P_L0_8X4 = H264_SUB_MB_TYPE_P_BASE + 1,
	H264_SUB_MB_TYPE_P_L0_4X8 = H264_SUB_MB_TYPE_P_BASE + 2,
	H264_SUB_MB_TYPE_P_L0_4X4 = H264_SUB_MB_TYPE_P_BASE + 3,
	H264_SUB_MB_TYPE_P_END = H264_SUB_MB_TYPE_P_BASE + 4,
	/* B */
	H264_SUB_MB_TYPE_B_BASE = H264_SUB_MB_TYPE_P_END,
	H264_SUB_MB_TYPE_B_DIRECT_8X8 = H264_SUB_MB_TYPE_B_BASE + 0,
	H264_SUB_MB_TYPE_B_L0_8X8 = H264_SUB_MB_TYPE_B_BASE + 1,
	H264_SUB_MB_TYPE_B_L1_8X8 = H264_SUB_MB_TYPE_B_BASE + 2,
	H264_SUB_MB_TYPE_B_BI_8X8 = H264_SUB_MB_TYPE_B_BASE + 3,
	H264_SUB_MB_TYPE_B_L0_8X4 = H264_SUB_MB_TYPE_B_BASE + 4,
	H264_SUB_MB_TYPE_B_L0_4X8 = H264_SUB_MB_TYPE_B_BASE + 5,
	H264_SUB_MB_TYPE_B_L1_8X4 = H264_SUB_MB_TYPE_B_BASE + 6,
	H264_SUB_MB_TYPE_B_L1_4X8 = H264_SUB_MB_TYPE_B_BASE + 7,
	H264_SUB_MB_TYPE_B_BI_8X4 = H264_SUB_MB_TYPE_B_BASE + 8,
	H264_SUB_MB_TYPE_B_BI_4X8 = H264_SUB_MB_TYPE_B_BASE + 9,
	H264_SUB_MB_TYPE_B_L0_4X4 = H264_SUB_MB_TYPE_B_BASE + 10,
	H264_SUB_MB_TYPE_B_L1_4X4 = H264_SUB_MB_TYPE_B_BASE + 11,
	H264_SUB_MB_TYPE_B_BI_4X4 = H264_SUB_MB_TYPE_B_BASE + 12,
	H264_SUB_MB_TYPE_B_END = H264_SUB_MB_TYPE_B_BASE + 13,
};

enum h264_cabac_ctxblockcat {
	H264_CTXBLOCKCAT_LUMA_DC = 0,
	H264_CTXBLOCKCAT_LUMA_AC = 1,
	H264_CTXBLOCKCAT_LUMA_4X4 = 2,
	H264_CTXBLOCKCAT_CHROMA_DC = 3,
	H264_CTXBLOCKCAT_CHROMA_AC = 4,
	/* 8x8 mode only */
	H264_CTXBLOCKCAT_LUMA_8X8 = 5,
	/* 4:4:4 mode only */
	H264_CTXBLOCKCAT_CB_DC = 6,
	H264_CTXBLOCKCAT_CB_AC = 7,
	H264_CTXBLOCKCAT_CB_4X4 = 8,
	H264_CTXBLOCKCAT_CB_8X8 = 9,
	H264_CTXBLOCKCAT_CR_DC = 10,
	H264_CTXBLOCKCAT_CR_AC = 11,
	H264_CTXBLOCKCAT_CR_4X4 = 12,
	H264_CTXBLOCKCAT_CR_8X8 = 13,
};

struct h264_cabac_context;

struct h264_hrd_parameters {
	uint32_t cpb_cnt_minus1;
	uint32_t bit_rate_scale;
	uint32_t cpb_size_scale;
	uint32_t bit_rate_value_minus1[32];
	uint32_t cpb_size_value_minus1[32];
	uint32_t cbr_flag[32];
	uint32_t initial_cpb_removal_delay_length_minus1;
	uint32_t cpb_removal_delay_length_minus1;
	uint32_t dpb_output_delay_length_minus1;
	uint32_t time_offset_length;
};

struct h264_vui {
	uint32_t aspect_ratio_present_flag;
	uint32_t aspect_ratio_idc;
	uint32_t sar_width;
	uint32_t sar_height;
	uint32_t overscan_info_present_flag;
	uint32_t overscan_appropriate_flag;
	uint32_t video_signal_type_present_flag;
	uint32_t video_format;
	uint32_t video_full_range_flag;
	uint32_t colour_description_present_flag;
	uint32_t colour_primaries;
	uint32_t transfer_characteristics;
	uint32_t matrix_coefficients;
	uint32_t chroma_loc_info_present_flag;
	uint32_t chroma_sample_loc_type_top_field;
	uint32_t chroma_sample_loc_type_bottom_field;
	uint32_t timing_info_present_flag;
	uint32_t num_units_in_tick;
	uint32_t time_scale;
	uint32_t fixed_frame_rate_flag;
	struct h264_hrd_parameters *nal_hrd_parameters;
	struct h264_hrd_parameters *vcl_hrd_parameters;
	uint32_t low_delay_hrd_flag;
	uint32_t pic_struct_present_flag;
	uint32_t bitstream_restriction_present_flag;
	uint32_t motion_vectors_over_pic_bounduaries_flag;
	uint32_t max_bytes_per_pic_denom;
	uint32_t max_bits_per_mb_denom;
	uint32_t log2_max_mv_length_horizontal;
	uint32_t log2_max_mv_length_vertical;
	uint32_t num_reorder_frames;
	uint32_t max_dec_frame_buffering;
};

struct h264_seqparm {
	uint32_t profile_idc;
	uint32_t constraint_set;
	uint32_t level_idc;
	uint32_t seq_parameter_set_id;
	/* start of new profile only stuff */
	uint32_t chroma_format_idc;
	uint32_t separate_colour_plane_flag;
	uint32_t bit_depth_luma_minus8;
	uint32_t bit_depth_chroma_minus8;
	uint32_t qpprime_y_zero_transform_bypass_flag;
	uint32_t seq_scaling_matrix_present_flag;
	uint32_t seq_scaling_list_present_flag[12];
	uint32_t use_default_scaling_matrix_flag[12];
	uint32_t seq_scaling_list_4x4[6][16];
	uint32_t seq_scaling_list_8x8[6][64];
	/* end of new profile only stuff */
	uint32_t log2_max_frame_num_minus4;
	uint32_t pic_order_cnt_type;
	uint32_t log2_max_pic_order_cnt_lsb_minus4;
	uint32_t delta_pic_order_always_zero_flag;
	int32_t offset_for_non_ref_pic;
	int32_t offset_for_top_to_bottom_field;
	uint32_t num_ref_frames_in_pic_order_cnt_cycle;
	int32_t offset_for_ref_frame[255];
	uint32_t max_num_ref_frames;
	uint32_t gaps_in_frame_num_value_allowed_flag;
	uint32_t pic_width_in_mbs_minus1;
	uint32_t pic_height_in_map_units_minus1;
	uint32_t frame_mbs_only_flag;
	uint32_t mb_adaptive_frame_field_flag;
	uint32_t direct_8x8_inference_flag;
	uint32_t frame_cropping_flag;
	uint32_t frame_crop_left_offset;
	uint32_t frame_crop_right_offset;
	uint32_t frame_crop_top_offset;
	uint32_t frame_crop_bottom_offset;
	struct h264_vui *vui;
	/* SVC part */
	int is_svc;
	uint32_t inter_layer_deblocking_filter_control_present_flag;
	uint32_t extended_spatial_scalability_idc;
	uint32_t chroma_phase_x_plus1_flag;
	uint32_t chroma_phase_y_plus1;
	uint32_t seq_ref_layer_chroma_phase_x_plus1_flag;
	uint32_t seq_ref_layer_chroma_phase_y_plus1;
	int32_t seq_ref_layer_left_offset;
	int32_t seq_ref_layer_top_offset;
	int32_t seq_ref_layer_right_offset;
	int32_t seq_ref_layer_bottom_offset;
	uint32_t seq_tcoeff_level_prediction_flag;
	uint32_t adaptive_tcoeff_level_prediction_flag;
	uint32_t slice_header_restriction_flag;
	struct h264_vui *svc_vui;
	/* MVC part */
	int is_mvc;
	uint32_t num_views_minus1;
	struct h264_seqparm_mvc_view {
		uint32_t view_id;
		uint32_t num_anchor_refs_l0;
		uint32_t anchor_ref_l0[15];
		uint32_t num_anchor_refs_l1;
		uint32_t anchor_ref_l1[15];
		uint32_t num_non_anchor_refs_l0;
		uint32_t non_anchor_ref_l0[15];
		uint32_t num_non_anchor_refs_l1;
		uint32_t non_anchor_ref_l1[15];
	} *views;
	uint32_t num_level_values_signalled_minus1;
	struct h264_seqparm_mvc_level {
		uint32_t level_idc;
		uint32_t num_applicable_ops_minus1;
		struct h264_seqparm_mvc_applicable_op {
			uint32_t temporal_id;
			uint32_t num_target_views_minus1;
			uint32_t *target_view_id;
			uint32_t num_views_minus1;
		} *applicable_ops;
	} *levels;
	struct h264_vui *mvc_vui;
	/* extension [alpha] part */
	int has_ext;
	uint32_t aux_format_idc;
	uint32_t bit_depth_aux_minus8;
	uint32_t alpha_incr_flag;
	uint32_t alpha_opaque_value;
	uint32_t alpha_transparent_value;
};

struct h264_picparm {
	uint32_t pic_parameter_set_id;
	uint32_t seq_parameter_set_id;
	uint32_t entropy_coding_mode_flag;
	uint32_t bottom_field_pic_order_in_frame_present_flag;
	uint32_t num_slice_groups_minus1;
	uint32_t slice_group_map_type;
	uint32_t run_length_minus1[8];
	uint32_t top_left[8];
	uint32_t bottom_right[8];
	uint32_t slice_group_change_direction_flag;
	uint32_t slice_group_change_rate_minus1;
	uint32_t pic_size_in_map_units_minus1;
	uint32_t *slice_group_id;
	uint32_t num_ref_idx_l0_default_active_minus1;
	uint32_t num_ref_idx_l1_default_active_minus1;
	uint32_t weighted_pred_flag;
	uint32_t weighted_bipred_idc;
	int32_t pic_init_qp_minus26;
	int32_t pic_init_qs_minus26;
	int32_t chroma_qp_index_offset;
	uint32_t deblocking_filter_control_present_flag;
	uint32_t constrained_intra_pred_flag;
	uint32_t redundant_pic_cnt_present_flag;
	/* start of new stuff */
	uint32_t transform_8x8_mode_flag;
	uint32_t chroma_format_idc;
	uint32_t pic_scaling_matrix_present_flag;
	uint32_t pic_scaling_list_present_flag[12];
	uint32_t use_default_scaling_matrix_flag[12];
	uint32_t pic_scaling_list_4x4[6][16];
	uint32_t pic_scaling_list_8x8[6][64];
	int32_t second_chroma_qp_index_offset;
};

struct h264_macroblock {
	uint32_t mb_field_decoding_flag;
	uint32_t mb_type;
	uint32_t coded_block_pattern;
	uint32_t transform_size_8x8_flag;
	int32_t mb_qp_delta;
	uint32_t pcm_sample_luma[256];
	uint32_t pcm_sample_chroma[512];
	uint32_t prev_intra4x4_pred_mode_flag[16];
	uint32_t rem_intra4x4_pred_mode[16];
	uint32_t prev_intra8x8_pred_mode_flag[4];
	uint32_t rem_intra8x8_pred_mode[4];
	uint32_t intra_chroma_pred_mode;
	uint32_t sub_mb_type[4];
	uint32_t ref_idx[2][4];
	int32_t mvd[2][16][2];
	int32_t block_luma_dc[3][16]; /* [0 luma, 1 cb, 2 cr][coeff] */
	int32_t block_luma_ac[3][16][15]; /* [0 luma, 1 cb, 2 cr][blkIdx][coeff] */
	int32_t block_luma_4x4[3][16][16]; /* [0 luma, 1 cb, 2 cr][blkIdx][coeff] */
	int32_t block_luma_8x8[3][4][64]; /* [0 luma, 1 cb, 2 cr][blkIdx][coeff] */
	int32_t block_chroma_dc[2][8]; /* [0 cb, 1 cr][coeff] */
	int32_t block_chroma_ac[2][8][15]; /* [0 cb, 1 cr][blkIdx][coeff] */
	int total_coeff[3][16]; /* [0 luma, 1 cb, 2 cr][blkIdx] */
	int coded_block_flag[3][17]; /* [0 luma, 1 cb, 2 cr][blkIdx], with blkIdx == 16 being DC */
};

struct h264_ref_pic_list_modification {
	uint32_t flag;
	struct h264_slice_ref_pic_list_modification_entry {
		uint32_t op;
		uint32_t param;
		uint32_t param2;
	} list[33];
};

struct h264_pred_weight_table_entry {
	uint32_t luma_weight_flag;
	int32_t luma_weight;
	int32_t luma_offset;
	uint32_t chroma_weight_flag;
	int32_t chroma_weight[2];
	int32_t chroma_offset[2];
};

struct h264_pred_weight_table {
	uint32_t luma_log2_weight_denom;
	uint32_t chroma_log2_weight_denom;
	struct h264_pred_weight_table_entry l0[0x20];
	struct h264_pred_weight_table_entry l1[0x20];
};

struct h264_dec_ref_pic_marking {
	uint32_t no_output_of_prior_pics_flag;
	uint32_t long_term_reference_flag;
	uint32_t adaptive_ref_pic_marking_mode_flag;
	struct h264_mmco_entry {
		uint32_t memory_management_control_operation;
		uint32_t difference_of_pic_nums_minus1;
		uint32_t long_term_pic_num;
		uint32_t long_term_frame_idx;
		uint32_t max_long_term_frame_idx_plus1;
	} *mmcos;
	int mmcosnum;
	int mmcosmax;
};

struct h264_dec_ref_base_pic_marking {
	uint32_t store_ref_base_pic_flag;
	uint32_t adaptive_ref_base_pic_marking_mode_flag;
	struct h264_mmco_entry *mmcos;
	int mmcosnum;
	int mmcosmax;
};

struct h264_nal_svc_header {
	uint32_t idr_flag;
	uint32_t priority_id;
	uint32_t no_inter_layer_pred_flag;
	uint32_t dependency_id;
	uint32_t quality_id;
	uint32_t temporal_id;
	uint32_t use_ref_base_pic_flag;
	uint32_t discardable_flag;
	uint32_t output_flag;
};

struct h264_slice {
	uint32_t nal_ref_idc;
	uint32_t nal_unit_type;
	struct h264_nal_svc_header svc;
	uint32_t first_mb_in_slice;
	struct h264_seqparm *seqparm;
	struct h264_picparm *picparm;
	uint32_t slice_type;
	uint32_t slice_all_same;
	uint32_t colour_plane_id;
	uint32_t frame_num;
	uint32_t field_pic_flag;
	uint32_t bottom_field_flag;
	uint32_t idr_pic_id;
	uint32_t pic_order_cnt_lsb;
	int32_t delta_pic_order_cnt_bottom;
	int32_t delta_pic_order_cnt[2];
	uint32_t redundant_pic_cnt;
	uint32_t direct_spatial_mb_pred_flag;
	uint32_t num_ref_idx_active_override_flag;
	uint32_t num_ref_idx_l0_active_minus1;
	uint32_t num_ref_idx_l1_active_minus1;
	struct h264_ref_pic_list_modification ref_pic_list_modification_l0;
	struct h264_ref_pic_list_modification ref_pic_list_modification_l1;
	uint32_t base_pred_weight_table_flag;
	struct h264_pred_weight_table pred_weight_table;
	struct h264_dec_ref_pic_marking dec_ref_pic_marking;
	struct h264_dec_ref_base_pic_marking dec_ref_base_pic_marking;
	int32_t slice_qp_delta;
	uint32_t sp_for_switch_flag;
	int32_t slice_qs_delta;
	uint32_t cabac_init_idc;
	uint32_t disable_deblocking_filter_idc;
	int32_t slice_alpha_c0_offset_div2;
	int32_t slice_beta_offset_div2;
	uint32_t slice_group_change_cycle;
	/* derived stuff starts here */
	uint32_t idr_pic_flag;
	uint32_t chroma_array_type;
	uint32_t bit_depth_luma_minus8;
	uint32_t bit_depth_chroma_minus8;
	uint32_t pic_width_in_mbs;
	uint32_t pic_height_in_mbs;
	uint32_t pic_size_in_mbs;
	int32_t sliceqpy;
	uint32_t mbaff_frame_flag;
	uint32_t last_mb_in_slice;
	/* previous and current macroblock */
	uint32_t prev_mb_addr;
	uint32_t curr_mb_addr;
	/* macroblocks */
	int *sgmap;
	struct h264_macroblock *mbs;
};

enum h264_mb_pos {
	H264_MB_THIS,
	H264_MB_A,	/* left */
	H264_MB_B,	/* above */
	H264_MB_C,	/* right and above */
	H264_MB_D,	/* left and above */
};

enum h264_block_size {
	H264_BLOCK_4X4,
	H264_BLOCK_CHROMA,
	H264_BLOCK_8X8,
};

static inline int h264_is_skip_mb_type(uint32_t mb_type) {
	switch (mb_type) {
		case H264_MB_TYPE_P_SKIP:
		case H264_MB_TYPE_B_SKIP:
			return 1;
		default:
			return 0;
	}
}

static inline int h264_is_submb_mb_type(uint32_t mb_type) {
	switch (mb_type) {
		case H264_MB_TYPE_P_8X8:
		case H264_MB_TYPE_P_8X8REF0:
		case H264_MB_TYPE_B_8X8:
			return 1;
		default:
			return 0;
	}
}

static inline int h264_is_intra_16x16_mb_type(uint32_t mb_type) {
	return (mb_type >= H264_MB_TYPE_I_16X16_0_0_0 && mb_type <= H264_MB_TYPE_I_16X16_3_2_1);
}

static inline int h264_is_inter_mb_type(uint32_t mb_type) {
	return mb_type >= H264_MB_TYPE_P_BASE;
}

int h264_mb_avail(struct h264_slice *slice, uint32_t mbaddr);
const struct h264_macroblock *h264_mb_unavail(int inter);

/* 6.4.8, 6.4.9 */
const struct h264_macroblock *h264_mb_nb_p(struct h264_slice *slice, enum h264_mb_pos pos, int inter);
/* 6.4.10.1 */
const struct h264_macroblock *h264_mb_nb(struct h264_slice *slice, enum h264_mb_pos pos, int inter);
/* 6.4.10.2+ */
const struct h264_macroblock *h264_mb_nb_b(struct h264_slice *slice, enum h264_mb_pos pos, enum h264_block_size bs, int inter, int idx, int *pidx);
const struct h264_macroblock *h264_inter_filter(struct h264_slice *slice, const struct h264_macroblock *mb, int inter);

uint32_t h264_next_mb_addr(struct h264_slice *slice, uint32_t mbaddr);

int h264_mb_skip_flag(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t *binVal);
int h264_mb_field_decoding_flag(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t *binVal);
int h264_mb_type(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t slice_type, uint32_t *val);
int h264_sub_mb_type(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t slice_type, uint32_t *val);
int h264_coded_block_pattern(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t mb_type, int has_chroma, uint32_t *val);
int h264_transform_size_8x8_flag(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t *binVal);
int h264_mb_qp_delta(struct bitstream *str, struct h264_cabac_context *cabac, int32_t *val);
int h264_prev_intra_pred_mode_flag(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t *val);
int h264_rem_intra_pred_mode(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t *val);
int h264_intra_chroma_pred_mode(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t *val);
int h264_ref_idx(struct bitstream *str, struct h264_cabac_context *cabac, int idx, int which, int max, uint32_t *val);
int h264_mvd(struct bitstream *str, struct h264_cabac_context *cabac, int idx, int comp, int which, int32_t *val);

int h264_coded_block_flag(struct bitstream *str, struct h264_cabac_context *cabac, int cat, int idx, uint32_t *val);
int h264_significant_coeff_flag(struct bitstream *str, struct h264_cabac_context *cabac, int field, int cat, int idx, int last, uint32_t *val);
int h264_coeff_abs_level_minus1(struct bitstream *str, struct h264_cabac_context *cabac, int cat, int num1, int numgt1, int32_t *val);

int h264_coeff_token(struct bitstream *str, struct h264_slice *slice, int cat, int idx, uint32_t *trailingOnes, uint32_t *totalCoeff);
int h264_level(struct bitstream *str, int suffixLength, int onebias, int32_t *val);
int h264_total_zeros(struct bitstream *str, int mode, int tzVlcIndex, uint32_t *val);
int h264_run_before(struct bitstream *str, int zerosLeft, uint32_t *val);

void h264_del_seqparm(struct h264_seqparm *seqparm);
void h264_del_picparm(struct h264_picparm *picparm);
void h264_del_slice(struct h264_slice *slice);

int h264_seqparm(struct bitstream *str, struct h264_seqparm *seqparm);
int h264_seqparm_svc(struct bitstream *str, struct h264_seqparm *seqparm);
int h264_seqparm_mvc(struct bitstream *str, struct h264_seqparm *seqparm);
int h264_seqparm_ext(struct bitstream *str, struct h264_seqparm **seqparms, uint32_t *pseq_parameter_set_id);
int h264_picparm(struct bitstream *str, struct h264_seqparm **seqparms, struct h264_seqparm **subseqparms, struct h264_picparm *picparm);
int h264_slice_header(struct bitstream *str, struct h264_seqparm **seqparms, struct h264_picparm **picparms, struct h264_slice *slice);
int h264_slice_data(struct bitstream *str, struct h264_slice *slice);
int h264_pred_weight_table(struct bitstream *str, struct h264_slice *slice, struct h264_pred_weight_table *table);
int h264_residual(struct bitstream *str, struct h264_cabac_context *cabac, struct h264_slice *slice, struct h264_macroblock *mb, int start, int end);

void h264_print_seqparm(struct h264_seqparm *seqparm);
void h264_print_seqparm_ext(struct h264_seqparm *seqparm);
void h264_print_picparm(struct h264_picparm *picparm);
void h264_print_slice_header(struct h264_slice *slice);
void h264_print_slice_data(struct h264_slice *slice);

#endif
