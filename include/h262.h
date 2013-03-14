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

#ifndef H262_H
#define H262_H

#include "vstream.h"

enum h262_start_code {
	H262_START_CODE_PICPARM = 0,
	H262_START_CODE_SLICE_BASE = 1,
	/* ... */
	H262_START_CODE_SLICE_LAST = 0xaf,
	/* 0xb0, 0xb1 reserved */
	H262_START_CODE_USER_DATA = 0xb2,
	H262_START_CODE_SEQPARM = 0xb3,
	H262_START_CODE_ERROR = 0xb4,
	H262_START_CODE_EXTENSION = 0xb5,
	/* 0xb6 reserved */
	H262_START_CODE_END = 0xb7,
	H262_START_CODE_GOP = 0xb8,
	/* 0xb9-0xff system */
};

enum h262_extension_start_code {
	/* 0 reserved */
	H262_EXT_SEQUENCE = 1,
	H262_EXT_SEQ_DISPLAY = 2,
	H262_EXT_QUANT_MATRIX = 3,
	H262_EXT_COPYRIGHT = 4,
	H262_EXT_SEQ_SCALABLE = 5,
	/* 6 reserved */
	H262_EXT_PIC_DISPLAY = 7,
	H262_EXT_PIC_CODING = 8,
	H262_EXT_PIC_SPATIAL_SCALABLE = 9,
	H262_EXT_PIC_TEMPORAL_SCALABLE = 10,
	/* 11-15 reserved */
};

enum h262_aspect_ratio {
	H262_ASPECT_RATIO_SAMPLE_SQUARE = 1,
	H262_ASPECT_RATIO_DISPLAY_4_3 = 2,
	H262_ASPECT_RATIO_DISPLAY_16_9 = 3,
	H262_ASPECT_RATIO_DISPLAY_221_100 = 4,
};

enum mprg12_picture_coding_type {
	H262_PIC_TYPE_I = 1,
	H262_PIC_TYPE_P = 2,
	H262_PIC_TYPE_B = 3,
	H262_PIC_TYPE_D = 4,
};

enum mprg12_picture_structure {
	H262_PIC_STRUCT_FIELD_TOP = 1,
	H262_PIC_STRUCT_FIELD_BOTTOM = 2,
	H262_PIC_STRUCT_FRAME = 3,
};

enum h262_frame_motion_type {
	H262_FRAME_MOTION_FIELD = 1,
	H262_FRAME_MOTION_FRAME = 2,
	H262_FRAME_MOTION_DUAL_PRIME = 3,
};

enum h262_field_motion_type {
	H262_FIELD_MOTION_FIELD = 1,
	H262_FIELD_MOTION_16X8 = 2,
	H262_FIELD_MOTION_DUAL_PRIME = 3,
};

struct h262_seqparm {
	/* sequence header - original MPEG1 part */
	uint32_t horizontal_size; /* extended for MPEG2 */
	uint32_t vertical_size; /* extended for MPEG2 */
	uint32_t aspect_ratio_information;
	uint32_t frame_rate_code;
	uint32_t bit_rate; /* extended for MPEG2 */
	uint32_t vbv_buffer_size; /* extended for MPEG2 */
	uint32_t constrained_parameters_flag;
	uint32_t load_intra_quantiser_matrix;
	uint32_t intra_quantiser_matrix[64];
	uint32_t load_non_intra_quantiser_matrix;
	uint32_t non_intra_quantiser_matrix[64];
	/* sequence extension aka MPEG2 */
	int is_ext;
	uint32_t profile_and_level_indication;
	uint32_t progressive_sequence;
	uint32_t chroma_format;
	uint32_t low_delay;
	uint32_t frame_rate_extension_n;
	uint32_t frame_rate_extension_d;
	/* sequence display extension - like H.264 VUI */
	int has_disp_ext;
	uint32_t video_format;
	uint32_t colour_description;
	uint32_t color_primaries;
	uint32_t transfer_characteristics;
	uint32_t matrix_coefficients;
	uint32_t display_horizontal_size;
	uint32_t display_vertical_size;
};

struct h262_gop {
	uint32_t drop_frame_flag;
	uint32_t time_code_hours;
	uint32_t time_code_minutes;
	uint32_t time_code_seconds;
	uint32_t time_code_pictures;
	uint32_t closed_gop;
	uint32_t broken_link;
};

struct h262_picparm {
	/* picture header - original MPEG1 part */
	uint32_t temporal_reference;
	uint32_t picture_coding_type;
	uint32_t vbv_delay;
	uint32_t full_pel_forward_vector;
	uint32_t forward_f_code;
	uint32_t full_pel_backward_vector;
	uint32_t backward_f_code;
	/* picture coding extension - MPEG2 */
	int is_ext;
	uint32_t f_code[2][2];
	uint32_t intra_dc_precision;
	uint32_t picture_structure;
	uint32_t top_field_first;
	uint32_t frame_pred_frame_dct;
	uint32_t concealment_motion_vectors;
	uint32_t q_scale_type;
	uint32_t intra_vlc_format;
	uint32_t alternate_scan;
	uint32_t repeat_first_field;
	uint32_t chroma_420_type;
	uint32_t progressive_frame;
	uint32_t composite_display_flag;
	uint32_t v_axis;
	uint32_t field_sequence;
	uint32_t sub_carrier;
	uint32_t burst_amplitude;
	uint32_t sub_carrier_phase;
	uint32_t load_intra_quantiser_matrix;
	uint32_t intra_quantiser_matrix[64];
	uint32_t load_non_intra_quantiser_matrix;
	uint32_t non_intra_quantiser_matrix[64];
	uint32_t load_chroma_intra_quantiser_matrix;
	uint32_t chroma_intra_quantiser_matrix[64];
	uint32_t load_chroma_non_intra_quantiser_matrix;
	uint32_t chroma_non_intra_quantiser_matrix[64];
	/* picture display extension */
	uint32_t frame_centre_horizontal_offset[3];
	int has_disp_ext;
	uint32_t frame_centre_vertical_offset[3];
	/* derived stuff */
	uint32_t pic_width_in_mbs;
	uint32_t pic_height_in_mbs;
	uint32_t pic_size_in_mbs;
};

struct h262_macroblock {
	uint32_t macroblock_skipped;
	uint32_t macroblock_quant;
	uint32_t macroblock_motion_forward;
	uint32_t macroblock_motion_backward;
	uint32_t macroblock_pattern;
	uint32_t macroblock_intra;
	uint32_t frame_motion_type;
	uint32_t field_motion_type;
	uint32_t dct_type;
	uint32_t quantiser_scale_code;
	uint32_t motion_vertical_field_select[2][2];
	uint32_t motion_code[2][2][2];
	uint32_t motion_residual[2][2][2];
	uint32_t dmvector[2];
	uint32_t coded_block_pattern;
	/* first component is dct_diff for intra macroblocks */
	int32_t block[12][64];
};

struct h262_slice {
	uint32_t slice_vertical_position;
	uint32_t quantiser_scale_code;
	uint32_t intra_slice_flag;
	uint32_t intra_slice;
	uint32_t first_mb_in_slice;
	uint32_t last_mb_in_slice;
	struct h262_macroblock *mbs;
};

void h262_del_seqparm(struct h262_seqparm *seqparm);
void h262_del_picparm(struct h262_picparm *picparm);
void h262_del_gop(struct h262_gop *gop);
void h262_del_slice(struct h262_slice *slice);

int h262_seqparm(struct bitstream *str, struct h262_seqparm *seqparm);
int h262_seqparm_ext(struct bitstream *str, struct h262_seqparm *seqparm);
int h262_seqparm_disp_ext(struct bitstream *str, struct h262_seqparm *seqparm);

int h262_picparm(struct bitstream *str, struct h262_seqparm *seqparm, struct h262_picparm *picparm);
int h262_picparm_ext(struct bitstream *str, struct h262_seqparm *seqparm, struct h262_picparm *picparm);
int h262_picparm_disp_ext(struct bitstream *str, struct h262_seqparm *seqparm, struct h262_picparm *picparm);

int h262_gop(struct bitstream *str, struct h262_gop *gop);

int h262_slice(struct bitstream *str, struct h262_seqparm *seqparm, struct h262_picparm *picparm, struct h262_slice *slice);

void h262_print_seqparm(struct h262_seqparm *seqparm);
void h262_print_picparm(struct h262_picparm *picparm);
void h262_print_gop(struct h262_gop *gop);
void h262_print_slice(struct h262_seqparm *seqparm, struct h262_picparm *picparm, struct h262_slice *slice);

#endif

