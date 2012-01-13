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

void h262_print_seqparm(struct h262_seqparm *seqparm) {
	int i;
	printf("%s sequence header:\n", seqparm->is_ext?"MPEG2":"MPEG1");
	printf("\thorizontal_size = %d\n", seqparm->horizontal_size);
	printf("\tvertical_size = %d\n", seqparm->vertical_size);
	const char *astr = "???";
	switch (seqparm->aspect_ratio_information) {
		case H262_ASPECT_RATIO_SAMPLE_SQUARE:
			astr = "square sample";
			break;
		case H262_ASPECT_RATIO_DISPLAY_4_3:
			astr = "4:3 display";
			break;
		case H262_ASPECT_RATIO_DISPLAY_16_9:
			astr = "16:9 display";
			break;
		case H262_ASPECT_RATIO_DISPLAY_221_100:
			astr = "2.21:1 display";
			break;
	}
	printf("\taspect_ratio_information = %d [%s]\n", seqparm->aspect_ratio_information, astr);
	printf("\tframe_rate_code = %d\n", seqparm->frame_rate_code);
	printf("\tbit_rate = %d\n", seqparm->bit_rate);
	printf("\tvbv_buffer_size = %d\n", seqparm->vbv_buffer_size);
	printf("\tconstrained_parameters_flag = %d\n", seqparm->constrained_parameters_flag);
	if (seqparm->load_intra_quantiser_matrix) {
		printf("\tintra_quantiser_matrix =");
		for (i = 0; i < 64; i++)
			printf(" %d", seqparm->intra_quantiser_matrix[i]);
		printf("\n");
	}
	if (seqparm->load_non_intra_quantiser_matrix) {
		printf("\tnon_intra_quantiser_matrix =");
		for (i = 0; i < 64; i++)
			printf(" %d", seqparm->non_intra_quantiser_matrix[i]);
		printf("\n");
	}
	if (seqparm->is_ext) {
		printf("\tprofile_and_level_indication = 0x%02x\n", seqparm->profile_and_level_indication);
		printf("\tprogressive_sequence = %d\n", seqparm->progressive_sequence);
		printf("\tchroma_format = %d\n", seqparm->chroma_format);
		printf("\tlow_delay = %d\n", seqparm->low_delay);
		printf("\tframe_rate_extension_n = %d\n", seqparm->frame_rate_extension_n);
		printf("\tframe_rate_extension_d = %d\n", seqparm->frame_rate_extension_d);
	}
}

void h262_print_picparm(struct h262_picparm *picparm) {
	int f = 0, b = 0;
	const char *pctstr = "???";
	printf("Picture header:\n");
	printf("\ttemporal_reference = %d\n", picparm->temporal_reference);
	switch (picparm->picture_coding_type) {
		case H262_PIC_TYPE_I:
			pctstr = "I";
			break;
		case H262_PIC_TYPE_P:
			pctstr = "P";
			f = 1;
			break;
		case H262_PIC_TYPE_B:
			pctstr = "B";
			f = b = 1;
			break;
		case H262_PIC_TYPE_D:
			pctstr = "D";
			break;
	}
	printf("\tpicture_coding_type = %d [%s]\n", picparm->picture_coding_type, pctstr);
	printf("\tvbv_delay = %d\n", picparm->vbv_delay);
	if (f) {
		printf("\tfull_pel_forward_vector = %d\n", picparm->full_pel_forward_vector);
		printf("\tforward_f_code = %d\n", picparm->forward_f_code);
	}
	if (b) {
		printf("\tfull_pel_backward_vector = %d\n", picparm->full_pel_backward_vector);
		printf("\tbackward_f_code = %d\n", picparm->backward_f_code);
	}
	if (picparm->is_ext) {
		int i, j;
		for (i = 0; i < 2; i++)
			for (j = 0; j < 2; j++)
				printf("\tf_code[%d][%d] = %d\n", i, j, picparm->f_code[i][j]);
		printf("\tintra_dc_precision = %d\n", picparm->intra_dc_precision);
		const char *ps = "???";
		switch (picparm->picture_structure) {
			case H262_PIC_STRUCT_FIELD_TOP:
				ps = "top field";
				break;
			case H262_PIC_STRUCT_FIELD_BOTTOM:
				ps = "bottom field";
				break;
			case H262_PIC_STRUCT_FRAME:
				ps = "frame";
				break;
		}
		printf("\tpicture_structure = %d [%s]\n", picparm->picture_structure, ps);
		printf("\ttop_field_first = %d\n", picparm->top_field_first);
		printf("\tframe_pred_frame_dct = %d\n", picparm->frame_pred_frame_dct);
		printf("\tconcealment_motion_vectors = %d\n", picparm->concealment_motion_vectors);
		printf("\tq_scale_type = %d\n", picparm->q_scale_type);
		printf("\tintra_vlc_format = %d\n", picparm->intra_vlc_format);
		printf("\talternate_scan = %d\n", picparm->alternate_scan);
		printf("\trepeat_first_field = %d\n", picparm->repeat_first_field);
		printf("\tchroma_420_type = %d\n", picparm->chroma_420_type);
		printf("\tprogressive_frame = %d\n", picparm->progressive_frame);
		printf("\tcomposite_display_flag = %d\n", picparm->composite_display_flag);
		if (picparm->composite_display_flag) {
			printf("\tv_axis = %d\n", picparm->v_axis);
			printf("\tfield_sequence = %d\n", picparm->field_sequence);
			printf("\tsub_carrier = %d\n", picparm->sub_carrier);
			printf("\tburst_amplitude = %d\n", picparm->burst_amplitude);
			printf("\tsub_carrier_phase = %d\n", picparm->sub_carrier_phase);
		}
	}
}

void h262_print_gop(struct h262_gop *gop) {
	printf("GOP header:\n");
	printf("\tdrop_frame_flag = %d\n", gop->drop_frame_flag);
	printf("\ttime_code_hours = %d\n", gop->time_code_hours);
	printf("\ttime_code_minutes = %d\n", gop->time_code_minutes);
	printf("\ttime_code_seconds = %d\n", gop->time_code_seconds);
	printf("\ttime_code_pictures = %d\n", gop->time_code_pictures);
	printf("\tclosed_gop = %d\n", gop->closed_gop);
	printf("\tbroken_link = %d\n", gop->broken_link);
}

void h262_print_macroblock(struct h262_seqparm *seqparm, struct h262_picparm *picparm, struct h262_macroblock *mb, int addr) {
	int i;
	static const int block_count[4] = { 4, 6, 8, 12 };
	static const char *const frpms[] = { "???", "field", "frame", "dual-prime" };
	static const char *const fipms[] = { "???", "field", "16x8", "dual-prime" };
	printf("\tMacroblock %d: (%d, %d)\n", addr, addr % picparm->pic_width_in_mbs, addr / picparm->pic_width_in_mbs);
	printf("\t\tmacroblock_flags =");
	if (mb->macroblock_skipped)
		printf(" SKIP");
	if (mb->macroblock_quant)
		printf(" QUANT");
	if (mb->macroblock_motion_forward)
		printf(" FWD");
	if (mb->macroblock_motion_backward)
		printf(" BWD");
	if (mb->macroblock_pattern)
		printf(" PATTERN");
	if (mb->macroblock_intra)
		printf(" INTRA");
	printf("\n");
	if (!mb->macroblock_intra) {
		if (picparm->picture_structure == H262_PIC_STRUCT_FRAME) {
			printf("\t\tframe_motion_type = %d [%s]\n", mb->frame_motion_type, frpms[mb->frame_motion_type]);
		} else {
			printf("\t\tfield_motion_type = %d [%s]\n", mb->field_motion_type, fipms[mb->field_motion_type]);
		}
	}
	if (mb->macroblock_intra || mb->macroblock_pattern)
		printf("\t\tdct_type = %d\n", mb->dct_type);
	printf("\t\tquantiser_scale_code = %d\n", mb->quantiser_scale_code);
	if (!mb->macroblock_intra && !mb->macroblock_skipped) {
		int mvc, mfs, dmv;
		if (picparm->picture_structure == H262_PIC_STRUCT_FRAME) {
			switch (mb->frame_motion_type) {
				case H262_FRAME_MOTION_FIELD:
					mvc = 2;
					mfs = 1;
					dmv = 0;
					break;
				case H262_FRAME_MOTION_FRAME:
					mvc = 1;
					mfs = 0;
					dmv = 0;
					break;
				case H262_FRAME_MOTION_DUAL_PRIME:
					mvc = 1;
					mfs = 0;
					dmv = 1;
					break;
				default:
					mvc = 0;
			}
		} else {
			switch (mb->field_motion_type) {
				case H262_FIELD_MOTION_FIELD:
					mvc = 1;
					mfs = 1;
					dmv = 0;
					break;
				case H262_FIELD_MOTION_16X8:
					mvc = 2;
					mfs = 1;
					dmv = 0;
					break;
				case H262_FIELD_MOTION_DUAL_PRIME:
					mvc = 1;
					mfs = 0;
					dmv = 1;
					break;
				default:
					mvc = 0;
			}
		}
		int r, s, t;
		for (r = 0; r < mvc; r++) {
			for (s = 0; s < 2; s++) {
				if (s == 0 && !mb->macroblock_motion_forward)
					continue;
				if (s == 1 && !mb->macroblock_motion_backward)
					continue;
				if (mfs)
					printf("\t\tmotion_vertical_field_select[%d][%d] = %d\n", r, s, mb->motion_vertical_field_select[r][s]);
				for (t = 0; t < 2; t++) {
					printf("\t\tmotion_code[%d][%d][%d] = %d\n", r, s, t, mb->motion_code[r][s][t]);
					printf("\t\tmotion_residual[%d][%d][%d] = %d\n", r, s, t, mb->motion_residual[r][s][t]);
					if (dmv)
						printf("\t\tdmvector[%d] = %d\n", t, mb->dmvector[t]);
				}
			}
		}
	}
	printf("\t\tcoded_block_pattern = 0x%x\n", mb->coded_block_pattern);
	for (i = 0; i < block_count[seqparm->chroma_format]; i++) {
		printf("\t\tBlock %d:", i);
		int j;
		for (j = 0; j < 64; j++)
			printf(" %d", mb->block[i][j]);
		printf("\n");
	}
}

void h262_print_slice(struct h262_seqparm *seqparm, struct h262_picparm *picparm, struct h262_slice *slice) {
	int i;
	printf("Slice:\n");
	printf("\tslice_vertical_position = %d\n", slice->slice_vertical_position);
	printf("\tquantiser_scale_code = %d\n", slice->quantiser_scale_code);
	/* XXX**/
	if (slice->first_mb_in_slice == -1)
		return;
	for (i = slice->first_mb_in_slice; i <= slice->last_mb_in_slice; i++) {
		h262_print_macroblock(seqparm, picparm, &slice->mbs[i], i);
	}
}
