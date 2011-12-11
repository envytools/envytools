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
	H264_PROFILE_HIGH_444_PRED = 244,
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
};

struct h264_picparm {
	struct h264_seqparm *seqparm;
	uint32_t entropy_coding_mode_flag;
	uint32_t num_ref_idx_l0_default_active_minus1;
	uint32_t num_ref_idx_l1_default_active_minus1;
	uint32_t pic_init_qp_minus26;
	uint32_t transform_8x8_mode_flag;
};

struct h264_macroblock {
	uint32_t mb_field_decoding_flag;
	uint32_t mb_type;
	uint32_t coded_block_pattern;
	uint32_t transform_size_8x8_flag;
	int32_t mb_qp_delta;
	uint32_t pcm_sample_luma[256];
	uint32_t pcm_sample_chroma[64];
	uint32_t sub_mb_type[4];
	uint32_t intra_chroma_pred_mode;
};

struct h264_slice {
	struct h264_picparm *picparm;
	uint32_t slice_type;
	uint32_t num_ref_idx_l0_active_minus1;
	uint32_t num_ref_idx_l1_active_minus1;
	uint32_t slice_qp_delta;
	uint32_t cabac_init_idc;
	uint32_t first_mb_in_slice;
	/* derived stuff starts here */
	uint32_t sliceqpy;
	uint32_t last_mb_in_slice;
	uint32_t pic_width_in_mbs;
	uint32_t mbaff_frame_flag;
	uint32_t mbwidthc;
	uint32_t mbheightc;
	/* previous and current macroblock */
	uint32_t prev_mb_addr;
	uint32_t curr_mb_addr;
	/* macroblocks */
	struct h264_macroblock *mbs;
};

struct h264_pred_weight_table_entry {
	uint32_t luma_weight_flag;
	uint32_t luma_weight;
	uint32_t luma_offset;
	uint32_t chroma_weight_flag;
	uint32_t chroma_weight[2];
	uint32_t chroma_offset[2];
};

struct h264_pred_weight_table {
	uint32_t luma_log2_weight_denom;
	uint32_t chroma_log2_weight_denom;
	struct h264_pred_weight_table_entry l0[0x20];
	struct h264_pred_weight_table_entry l1[0x20];
};

enum h264_mb_pos {
	H264_MB_THIS,
	H264_MB_A,	/* left */
	H264_MB_B,	/* above */
	H264_MB_C,	/* right and above */
	H264_MB_D,	/* left and above */
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

static inline int h264_sub_mb_type_split_mode(uint32_t sub_mb_type) {
	switch (sub_mb_type) {
		case H264_SUB_MB_TYPE_P_L0_8X8:
		case H264_SUB_MB_TYPE_B_L0_8X8:
		case H264_SUB_MB_TYPE_B_L1_8X8:
		case H264_SUB_MB_TYPE_B_BI_8X8:
			return 0;
		case H264_SUB_MB_TYPE_P_L0_8X4:
		case H264_SUB_MB_TYPE_B_L0_8X4:
		case H264_SUB_MB_TYPE_B_L1_8X4:
		case H264_SUB_MB_TYPE_B_BI_8X4:
			return 1;
		case H264_SUB_MB_TYPE_P_L0_4X8:
		case H264_SUB_MB_TYPE_B_L0_4X8:
		case H264_SUB_MB_TYPE_B_L1_4X8:
		case H264_SUB_MB_TYPE_B_BI_4X8:
			return 2;
		case H264_SUB_MB_TYPE_P_L0_4X4:
		case H264_SUB_MB_TYPE_B_L0_4X4:
		case H264_SUB_MB_TYPE_B_L1_4X4:
		case H264_SUB_MB_TYPE_B_BI_4X4:
		case H264_SUB_MB_TYPE_B_DIRECT_8X8:
			return 3;
	}
}

int h264_mb_avail(struct h264_slice *slice, uint32_t mbaddr);
uint32_t h264_mb_nb(struct h264_slice *slice, enum h264_mb_pos pos);
uint32_t h264_next_mb_addr(struct h264_slice *slice, uint32_t mbaddr);

int h264_mb_skip_flag(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t *binVal);
int h264_mb_field_decoding_flag(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t *binVal);
int h264_mb_type(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t slice_type, uint32_t *val);
int h264_sub_mb_type(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t slice_type, uint32_t *val);
int h264_coded_block_pattern(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t mb_type, int has_chroma, uint32_t *val);
int h264_mb_transform_size_8x8_flag(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t *binVal);
int h264_mb_qp_delta(struct bitstream *str, struct h264_cabac_context *cabac, int32_t *val);
int h264_prev_intra_pred_mode_flag(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t *val);
int h264_rem_intra_pred_mode(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t *val);
int h264_intra_chroma_pred_mode(struct bitstream *str, struct h264_cabac_context *cabac, uint32_t *val);

void h264_del_seqparm(struct h264_seqparm *seqparm);

int h264_seqparm(struct bitstream *str, struct h264_seqparm *seqparm);
int h264_pred_weight_table(struct bitstream *str, struct h264_slice *slice, struct h264_pred_weight_table *table);

void h264_print_seqparm(struct h264_seqparm *seqparm);

#endif
