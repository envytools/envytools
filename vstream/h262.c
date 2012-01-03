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

#include "h262.h"
#include "vstream.h"
#include <stdio.h>
#include <stdlib.h>

int h262_seqparm(struct bitstream *str, struct h262_seqparm *seqparm) {
	uint32_t hs = seqparm->horizontal_size & 0xfff;
	uint32_t vs = seqparm->vertical_size & 0xfff;
	uint32_t br = seqparm->bit_rate & 0x3ffff;
	uint32_t vbv = seqparm->vbv_buffer_size & 0x3ff;
	int i;
	if (str->dir == VS_ENCODE && !seqparm->is_ext) {
		if (hs != seqparm->horizontal_size) {
			fprintf(stderr, "horizontal_size too big for MPEG1\n");
			return 1;
		}
		if (vs != seqparm->vertical_size) {
			fprintf(stderr, "vertical_size too big for MPEG1\n");
			return 1;
		}
		if (br != seqparm->bit_rate) {
			fprintf(stderr, "bit_rate too big for MPEG1\n");
			return 1;
		}
		if (vbv != seqparm->vbv_buffer_size) {
			fprintf(stderr, "vbv_buffer_size too big for MPEG1\n");
			return 1;
		}
	}
	if (vs_u(str, &hs, 12)) return 1;
	if (vs_u(str, &vs, 12)) return 1;
	if (vs_u(str, &seqparm->aspect_ratio_information, 4)) return 1;
	if (vs_u(str, &seqparm->frame_rate_code, 4)) return 1;
	if (vs_u(str, &br, 18)) return 1;
	if (vs_mark(str, 1, 1)) return 1;
	if (vs_u(str, &vbv, 10)) return 1;
	if (vs_u(str, &seqparm->constrained_parameters_flag, 1)) return 1;
	if (vs_u(str, &seqparm->load_intra_quantiser_matrix, 1)) return 1;
	if (seqparm->load_intra_quantiser_matrix) {
		for (i = 0; i < 64; i++)
			if (vs_u(str, &seqparm->intra_quantiser_matrix[i], 8)) return 1;
	}
	if (vs_u(str, &seqparm->load_non_intra_quantiser_matrix, 1)) return 1;
	if (seqparm->load_non_intra_quantiser_matrix) {
		for (i = 0; i < 64; i++)
			if (vs_u(str, &seqparm->non_intra_quantiser_matrix[i], 8)) return 1;
	}
	if (str->dir == VS_DECODE) {
		seqparm->horizontal_size = hs;
		seqparm->vertical_size = vs;
		seqparm->bit_rate = br;
		seqparm->vbv_buffer_size = vbv;
		seqparm->is_ext = 0;
		seqparm->has_disp_ext = 0;
	}
	if (!seqparm->is_ext) {
		if (vs_infer(str, &seqparm->progressive_sequence, 1)) return 1;
		if (vs_infer(str, &seqparm->chroma_format, 1)) return 1;
		if (vs_infer(str, &seqparm->low_delay, 0)) return 1;
		if (vs_infer(str, &seqparm->low_delay, 0)) return 1;
		if (vs_infer(str, &seqparm->frame_rate_extension_n, 0)) return 1;
		if (vs_infer(str, &seqparm->frame_rate_extension_d, 0)) return 1;
	}
	return 0;
}

int h262_seqparm_ext(struct bitstream *str, struct h262_seqparm *seqparm) {
	uint32_t hsh = seqparm->horizontal_size >> 12;
	uint32_t vsh = seqparm->vertical_size >> 12;
	uint32_t brh = seqparm->bit_rate >> 18;
	uint32_t vbvh = seqparm->vbv_buffer_size >> 10;
	if (vs_u(str, &seqparm->profile_and_level_indication, 8)) return 1;
	if (vs_u(str, &seqparm->progressive_sequence, 1)) return 1;
	if (vs_u(str, &seqparm->chroma_format, 2)) return 1;
	if (vs_u(str, &hsh, 2)) return 1;
	if (vs_u(str, &vsh, 2)) return 1;
	if (vs_u(str, &brh, 12)) return 1;
	if (vs_mark(str, 1, 1)) return 1;
	if (vs_u(str, &vbvh, 8)) return 1;
	if (vs_u(str, &seqparm->low_delay, 1)) return 1;
	if (vs_u(str, &seqparm->frame_rate_extension_n, 2)) return 1;
	if (vs_u(str, &seqparm->frame_rate_extension_d, 5)) return 1;
	if (str->dir == VS_DECODE) {
		seqparm->is_ext = 1;
		seqparm->horizontal_size |= hsh << 12;
		seqparm->vertical_size |= vsh << 12;
		seqparm->bit_rate |= brh << 18;
		seqparm->vbv_buffer_size |= vbvh << 10;
	}
	return 0;
}

int h262_picparm(struct bitstream *str, struct h262_seqparm *seqparm, struct h262_picparm *picparm) {
	int f = 0, b = 0;
	if (vs_u(str, &picparm->temporal_reference, 10)) return 1;
	if (vs_u(str, &picparm->picture_coding_type, 3)) return 1;
	if (vs_u(str, &picparm->vbv_delay, 16)) return 1;
	switch (picparm->picture_coding_type) {
		case H262_PIC_TYPE_I:
			break;
		case H262_PIC_TYPE_P:
			f = 1;
			break;
		case H262_PIC_TYPE_B:
			f = b = 1;
			break;
		case H262_PIC_TYPE_D:
			break;
		default:
			fprintf(stderr, "Invalid picture_coding_type\n");
			return 1;
	}
	if (f) {
		if (vs_u(str, &picparm->full_pel_forward_vector, 1)) return 1;
		if (vs_u(str, &picparm->forward_f_code, 3)) return 1;
	} else {
		if (vs_infer(str, &picparm->full_pel_forward_vector, 1)) return 1;
		if (vs_infer(str, &picparm->forward_f_code, 7)) return 1;
	}
	if (b) {
		if (vs_u(str, &picparm->full_pel_backward_vector, 1)) return 1;
		if (vs_u(str, &picparm->backward_f_code, 3)) return 1;
	} else {
		if (vs_infer(str, &picparm->full_pel_backward_vector, 1)) return 1;
		if (vs_infer(str, &picparm->backward_f_code, 7)) return 1;
	}
	uint32_t tmp = 0;
	while(1) {
		if (vs_u(str, &tmp, 1)) return 1;
		if (!tmp)
			break;
		if (vs_u(str, &tmp, 8)) return 1;
	}
	if (str->dir == VS_DECODE) {
		picparm->is_ext = 0;
		picparm->has_disp_ext = 0;
		picparm->load_intra_quantiser_matrix = 0;
		picparm->load_non_intra_quantiser_matrix = 0;
		picparm->load_chroma_intra_quantiser_matrix = 0;
		picparm->load_chroma_non_intra_quantiser_matrix = 0;
	}
	if (!picparm->is_ext) {
		if (vs_infer(str, &picparm->f_code[0][0], picparm->forward_f_code)) return 1;
		if (vs_infer(str, &picparm->f_code[0][1], picparm->forward_f_code)) return 1;
		if (vs_infer(str, &picparm->f_code[1][0], picparm->backward_f_code)) return 1;
		if (vs_infer(str, &picparm->f_code[1][1], picparm->backward_f_code)) return 1;
		if (vs_infer(str, &picparm->intra_dc_precision, 0)) return 1;
		if (vs_infer(str, &picparm->picture_structure, H262_PIC_STRUCT_FRAME)) return 1;
		if (vs_infer(str, &picparm->top_field_first, 0)) return 1;
		if (vs_infer(str, &picparm->frame_pred_frame_dct, 1)) return 1;
		if (vs_infer(str, &picparm->concealment_motion_vectors, 0)) return 1;
		if (vs_infer(str, &picparm->intra_vlc_format, 0)) return 1;
		if (vs_infer(str, &picparm->repeat_first_field, 0)) return 1;
		if (vs_infer(str, &picparm->chroma_420_type, 1)) return 1;
		if (vs_infer(str, &picparm->progressive_frame, 1)) return 1;
		if (vs_infer(str, &picparm->composite_display_flag, 0)) return 1;
	}
	picparm->pic_width_in_mbs = (seqparm->horizontal_size + 15) / 16;
	picparm->pic_height_in_mbs = (seqparm->vertical_size + 15) / 16;
	picparm->pic_size_in_mbs = picparm->pic_width_in_mbs * picparm->pic_height_in_mbs;
	return 0;
}

int h262_picparm_ext(struct bitstream *str, struct h262_seqparm *seqparm, struct h262_picparm *picparm) {
	if (vs_u(str, &picparm->f_code[0][0], 4)) return 1;
	if (vs_u(str, &picparm->f_code[0][1], 4)) return 1;
	if (vs_u(str, &picparm->f_code[1][0], 4)) return 1;
	if (vs_u(str, &picparm->f_code[1][1], 4)) return 1;
	if (vs_u(str, &picparm->intra_dc_precision, 2)) return 1;
	if (vs_u(str, &picparm->picture_structure, 2)) return 1;
	if (vs_u(str, &picparm->top_field_first, 1)) return 1;
	if (vs_u(str, &picparm->frame_pred_frame_dct, 1)) return 1;
	if (vs_u(str, &picparm->concealment_motion_vectors, 1)) return 1;
	if (vs_u(str, &picparm->q_scale_type, 1)) return 1;
	if (vs_u(str, &picparm->intra_vlc_format, 1)) return 1;
	if (vs_u(str, &picparm->alternate_scan, 1)) return 1;
	if (vs_u(str, &picparm->repeat_first_field, 1)) return 1;
	if (vs_u(str, &picparm->chroma_420_type, 1)) return 1;
	if (vs_u(str, &picparm->progressive_frame, 1)) return 1;
	if (vs_u(str, &picparm->composite_display_flag, 1)) return 1;
	if (picparm->composite_display_flag) {
		if (vs_u(str, &picparm->v_axis, 1)) return 1;
		if (vs_u(str, &picparm->field_sequence, 3)) return 1;
		if (vs_u(str, &picparm->sub_carrier, 1)) return 1;
		if (vs_u(str, &picparm->burst_amplitude, 7)) return 1;
		if (vs_u(str, &picparm->sub_carrier_phase, 8)) return 1;
	}
	picparm->is_ext = 1;
	return 0;
}

int h262_gop(struct bitstream *str, struct h262_gop *gop) {
	if (vs_u(str, &gop->drop_frame_flag, 1)) return 1;
	if (vs_u(str, &gop->time_code_hours, 5)) return 1;
	if (vs_u(str, &gop->time_code_minutes, 6)) return 1;
	if (vs_mark(str, 1, 1)) return 1;
	if (vs_u(str, &gop->time_code_seconds, 6)) return 1;
	if (vs_u(str, &gop->time_code_pictures, 6)) return 1;
	if (vs_u(str, &gop->closed_gop, 1)) return 1;
	if (vs_u(str, &gop->broken_link, 1)) return 1;
	return 0;
}

void h262_del_seqparm(struct h262_seqparm *seqparm) {
	free(seqparm);
}

void h262_del_picparm(struct h262_picparm *picparm) {
	free(picparm);
}

void h262_del_gop(struct h262_gop *gop) {
	free(gop);
}
