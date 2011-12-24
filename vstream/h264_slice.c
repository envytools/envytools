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
#include "h264_cabac.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

int h264_mb_slice_group(struct h264_slice *slice, uint32_t mbaddr) {
	if (mbaddr < 0 || mbaddr >= slice->pic_width_in_mbs)
		return 0;
	if (!slice->picparm->num_slice_groups_minus1)
		return 0;
	if (slice->seqparm->frame_mbs_only_flag || slice->field_pic_flag)
		return slice->sgmap[mbaddr];
	if (slice->mbaff_frame_flag)
		return slice->sgmap[mbaddr/2];
	int x = mbaddr % slice->pic_width_in_mbs;
	int y = mbaddr / slice->pic_width_in_mbs;
	return slice->sgmap[y / 2 * slice->pic_width_in_mbs + x];
}

int h264_mb_avail(struct h264_slice *slice, uint32_t mbaddr) {
	if (mbaddr < slice->first_mb_in_slice * (slice->mbaff_frame_flag + 1)
		|| mbaddr > slice->curr_mb_addr)
		return 0;
	return h264_mb_slice_group(slice, mbaddr) == h264_mb_slice_group(slice, slice->curr_mb_addr);
}

uint32_t h264_next_mb_addr(struct h264_slice *slice, uint32_t mbaddr) {
	int sg = h264_mb_slice_group(slice, mbaddr);
	mbaddr++;
	while (mbaddr < slice->pic_width_in_mbs && h264_mb_slice_group(slice, mbaddr) != sg)
		mbaddr++;
	return mbaddr;
}

static const struct h264_macroblock mb_unavail_intra = {
	/* filled with "default" values assumed by prediction for unavailable mbs, to avoid special cases */
	.mb_type = H264_MB_TYPE_UNAVAIL,
	.mb_field_decoding_flag = 0,
	.coded_block_pattern = 0x0f,
	.transform_size_8x8_flag = 0,
	.intra_chroma_pred_mode = 0,
	.coded_block_flag = {
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	},
};

static const struct h264_macroblock mb_unavail_inter = {
	/* filled with "default" values assumed by prediction for unavailable mbs, to avoid special cases */
	.mb_type = H264_MB_TYPE_UNAVAIL,
	.mb_field_decoding_flag = 0,
	.coded_block_pattern = 0x0f,
	.transform_size_8x8_flag = 0,
	.intra_chroma_pred_mode = 0,
	.coded_block_flag = { 0 },
};

const struct h264_macroblock *h264_mb_unavail(int inter) {
	return (inter ? &mb_unavail_inter : &mb_unavail_intra);
}

const struct h264_macroblock *h264_mb_nb_p(struct h264_slice *slice, enum h264_mb_pos pos, int inter) {
	uint32_t mbaddr = slice->curr_mb_addr;
	if (slice->mbaff_frame_flag)
		mbaddr /= 2;
	switch (pos) {
		case H264_MB_THIS:
			return &slice->mbs[slice->curr_mb_addr];
		case H264_MB_A:
			if ((mbaddr % slice->pic_width_in_mbs) == 0)
				return h264_mb_unavail(inter);
			mbaddr--;
			break;
		case H264_MB_B:
			mbaddr -= slice->pic_width_in_mbs;
			break;
		case H264_MB_C:
			if (((mbaddr+1) % slice->pic_width_in_mbs) == 0)
				return h264_mb_unavail(inter);
			mbaddr -= slice->pic_width_in_mbs - 1;
			break;
		case H264_MB_D:
			if ((mbaddr % slice->pic_width_in_mbs) == 0)
				return h264_mb_unavail(inter);
			mbaddr -= slice->pic_width_in_mbs + 1;
			break;
	}
	if (slice->mbaff_frame_flag)
		mbaddr *= 2;
	if (!h264_mb_avail(slice, mbaddr))
		return h264_mb_unavail(inter);
	return &slice->mbs[mbaddr];
}

const struct h264_macroblock *h264_mb_nb(struct h264_slice *slice, enum h264_mb_pos pos, int inter) {
	const struct h264_macroblock *mbp = h264_mb_nb_p(slice, pos, inter);
	const struct h264_macroblock *mbt = &slice->mbs[slice->curr_mb_addr];
	switch (pos) {
		case H264_MB_THIS:
			return mbp;
		case H264_MB_A:
			/* to the left */
			/* if not MBAFF - simply use mbp */
			/* if MBAFF and mbp not available - just pass mbp */
			/* if MBAFF and mbp available and frame/field coding differs - use mbp[0] */
			/* if MBAFF and mbp available and frame/field coding same - use mbp[curr_mb & 1] */
			if (slice->mbaff_frame_flag
					&& mbp->mb_type != H264_MB_TYPE_UNAVAIL
					&& (slice->curr_mb_addr & 1)
					&& mbp->mb_field_decoding_flag == mbt->mb_field_decoding_flag)
				return mbp + 1;
			return mbp;
		case H264_MB_B:
			if (slice->mbaff_frame_flag) {
				if (mbt->mb_field_decoding_flag) {
					/* MBAFF and in field mb */
					if (mbp->mb_type == H264_MB_TYPE_UNAVAIL)
						return mbp;
					/* MBAFF and in field mb, with mb pair above available */
					/* if above mb pair is frame coded, use bottom mb */
					/* if above is field coded, use mb of same parity */
					if (!(slice->curr_mb_addr & 1)
							&& mbp->mb_field_decoding_flag)
						return mbp;
					else
						return mbp+1;
				} else {
					/* MBAFF and in frame mb */
					/* if in bottom mb of the pair, use the top mb */
					if (slice->curr_mb_addr & 1)
						return mbt-1;
					/* otherwise, use the bottom mb of the above pair, whether field or frame */
					else if (mbp->mb_type != H264_MB_TYPE_UNAVAIL)
						return mbp+1;
					else
						return mbp;
				}
			}
			return mbp;
		default:
			abort();
	}
}

const struct h264_macroblock *h264_mb_nb_b(struct h264_slice *slice, enum h264_mb_pos pos, enum h264_block_size bs, int inter, int idx, int *pidx) {
	const struct h264_macroblock *mbp = h264_mb_nb_p(slice, pos, inter);
	const struct h264_macroblock *mbo = h264_mb_nb(slice, pos, inter);
	const struct h264_macroblock *mbt = &slice->mbs[slice->curr_mb_addr];
	if (slice->chroma_array_type == 1 && bs == H264_BLOCK_CHROMA)
		bs = H264_BLOCK_8X8;
	/* from now on BLOCK_CHROMA means 8x4 blocks */
	enum {
		M_SAME = 0,
		M_FRAME_FROM_FIELD = 1,
		M_FIELD_FROM_FRAME = 2,
	} mode = M_SAME;
	if (!mbt->mb_field_decoding_flag && mbp->mb_field_decoding_flag && mbp->mb_type != H264_MB_TYPE_UNAVAIL) {
		mode = M_FRAME_FROM_FIELD;
	}
	if (mbt->mb_field_decoding_flag && !mbp->mb_field_decoding_flag && mbp->mb_type != H264_MB_TYPE_UNAVAIL) {
		mode = M_FIELD_FROM_FRAME;
	}
	int par = slice->curr_mb_addr & 1;
	switch(pos) {
		case H264_MB_A:
			switch (bs) {
				case H264_BLOCK_4X4:
					if (idx & 1) {
						*pidx = idx - 1;
						return mbt;
					} else if (idx & 4) {
						*pidx = idx - 3;
						return mbt;
					} else {
						switch (mode) {
							case M_SAME:
								*pidx = idx + 5;
								return mbo;
							case M_FRAME_FROM_FIELD:
								*pidx = (idx >> 2 & 2) + par * 8 + 5;
								return mbo;
							case M_FIELD_FROM_FRAME:
								*pidx = (idx << 2 & 8) + 5;
								return mbp + (idx >> 3);
						}
					}
				case H264_BLOCK_CHROMA:
					if (idx & 1) {
						*pidx = idx - 1;
						return mbt;
					} else {
						switch (mode) {
							case M_SAME:
								*pidx = idx + 1;
								return mbo;
							case M_FRAME_FROM_FIELD:
								*pidx = (idx >> 1 & 2) + par * 4 + 1;
								return mbo;
							case M_FIELD_FROM_FRAME:
								*pidx = (idx << 1 & 4) + 1;
								return mbp + (idx >> 2);
						}
					}
				case H264_BLOCK_8X8:
					if (idx & 1) {
						*pidx = idx - 1;
						return mbt;
					} else {
						switch (mode) {
							case M_SAME:
								*pidx = idx + 1;
								return mbo;
							case M_FRAME_FROM_FIELD:
								*pidx = par * 2 + 1;
								return mbo;
							case M_FIELD_FROM_FRAME:
								*pidx = 1;
								return mbp + (idx >> 1);
						}
					}
				default:
					abort();
			}
		case H264_MB_B:
			switch (bs) {
				case H264_BLOCK_4X4:
					if (idx & 2) {
						*pidx = idx - 2;
						return mbt;
					} else if (idx & 8) {
						*pidx = idx - 6;
						return mbt;
					} else {
						*pidx = idx + 10;
						return mbo;
					}
				case H264_BLOCK_CHROMA:
					if (idx & 6) {
						*pidx = idx - 2;
						return mbt;
					} else {
						*pidx = idx + 6;
						return mbo;
					}
				case H264_BLOCK_8X8:
					if (idx & 2) {
						*pidx = idx - 2;
						return mbt;
					} else {
						*pidx = idx + 2;
						return mbo;
					}
				default:
					abort();
			}
		default:
			abort();
	}
}

const struct h264_macroblock *h264_inter_filter(struct h264_slice *slice, const struct h264_macroblock *mb, int inter) {
	if (!inter
			&& slice->picparm->constrained_intra_pred_flag
			&& slice->nal_unit_type == H264_NAL_UNIT_TYPE_SLICE_PART_A
			&& h264_is_inter_mb_type(mb->mb_type))
		return h264_mb_unavail(1);
	else
		return mb;
}


/* part mode, part 0 type, part 1 type */
static const int mb_part_info[][3] = {
	[H264_MB_TYPE_P_L0_16X16] = { 0, 1 },
	[H264_MB_TYPE_B_L0_16X16] = { 0, 1 },
	[H264_MB_TYPE_B_L1_16X16] = { 0, 2 },
	[H264_MB_TYPE_B_BI_16X16] = { 0, 3 },
	[H264_MB_TYPE_P_L0_L0_16X8] = { 1, 1, 1 },
	[H264_MB_TYPE_B_L0_L0_16X8] = { 1, 1, 1 },
	[H264_MB_TYPE_B_L0_L1_16X8] = { 1, 1, 2 },
	[H264_MB_TYPE_B_L0_BI_16X8] = { 1, 1, 3 },
	[H264_MB_TYPE_B_L1_L0_16X8] = { 1, 2, 1 },
	[H264_MB_TYPE_B_L1_L1_16X8] = { 1, 2, 2 },
	[H264_MB_TYPE_B_L1_BI_16X8] = { 1, 2, 3 },
	[H264_MB_TYPE_B_BI_L0_16X8] = { 1, 3, 1 },
	[H264_MB_TYPE_B_BI_L1_16X8] = { 1, 3, 2 },
	[H264_MB_TYPE_B_BI_BI_16X8] = { 1, 3, 3 },
	[H264_MB_TYPE_P_L0_L0_8X16] = { 2, 1, 1 },
	[H264_MB_TYPE_B_L0_L0_8X16] = { 2, 1, 1 },
	[H264_MB_TYPE_B_L0_L1_8X16] = { 2, 1, 2 },
	[H264_MB_TYPE_B_L0_BI_8X16] = { 2, 1, 3 },
	[H264_MB_TYPE_B_L1_L0_8X16] = { 2, 2, 1 },
	[H264_MB_TYPE_B_L1_L1_8X16] = { 2, 2, 2 },
	[H264_MB_TYPE_B_L1_BI_8X16] = { 2, 2, 3 },
	[H264_MB_TYPE_B_BI_L0_8X16] = { 2, 3, 1 },
	[H264_MB_TYPE_B_BI_L1_8X16] = { 2, 3, 2 },
	[H264_MB_TYPE_B_BI_BI_8X16] = { 2, 3, 3 },
};

static const int sub_mb_part_info[][2] = {
	[H264_SUB_MB_TYPE_B_DIRECT_8X8] = { 0, 0 },
	[H264_SUB_MB_TYPE_P_L0_8X8] = { 0, 1 },
	[H264_SUB_MB_TYPE_B_L0_8X8] = { 0, 1 },
	[H264_SUB_MB_TYPE_B_L1_8X8] = { 0, 2 },
	[H264_SUB_MB_TYPE_B_BI_8X8] = { 0, 3 },
	[H264_SUB_MB_TYPE_P_L0_8X4] = { 1, 1 },
	[H264_SUB_MB_TYPE_B_L0_8X4] = { 1, 1 },
	[H264_SUB_MB_TYPE_B_L1_8X4] = { 1, 2 },
	[H264_SUB_MB_TYPE_B_BI_8X4] = { 1, 3 },
	[H264_SUB_MB_TYPE_P_L0_4X8] = { 2, 1 },
	[H264_SUB_MB_TYPE_B_L0_4X8] = { 2, 1 },
	[H264_SUB_MB_TYPE_B_L1_4X8] = { 2, 2 },
	[H264_SUB_MB_TYPE_B_BI_4X8] = { 2, 3 },
	[H264_SUB_MB_TYPE_P_L0_4X4] = { 3, 1 },
	[H264_SUB_MB_TYPE_B_L0_4X4] = { 3, 1 },
	[H264_SUB_MB_TYPE_B_L1_4X4] = { 3, 2 },
	[H264_SUB_MB_TYPE_B_BI_4X4] = { 3, 3 },
};

static int infer_intra(struct bitstream *str, struct h264_macroblock *mb, int which) {
	int i;
	for (i = 0; i < 4; i++) {
		if (vs_infer(str, &mb->ref_idx[which][i], 0)) return 1;
	}
	for (i = 0; i < 16; i++) {
		if (vs_infer(str, &mb->mvd[which][i][0], 0)) return 1;
		if (vs_infer(str, &mb->mvd[which][i][1], 0)) return 1;
	}
	return 0;
}

int h264_mb_pred(struct bitstream *str, struct h264_cabac_context *cabac, struct h264_slice *slice, struct h264_macroblock *mb) {
	int i, j;
	if (mb->mb_type < H264_MB_TYPE_P_BASE) {
		if (!h264_is_intra_16x16_mb_type(mb->mb_type)) {
			if (!mb->transform_size_8x8_flag) {
				for (i = 0; i < 16; i++) {
					if (h264_prev_intra_pred_mode_flag(str, cabac, &mb->prev_intra4x4_pred_mode_flag[i])) return 1;
					if (!mb->prev_intra4x4_pred_mode_flag[i])
						if (h264_rem_intra_pred_mode(str, cabac, &mb->rem_intra4x4_pred_mode[i])) return 1;
				}
			} else {
				for (i = 0; i < 4; i++) {
					if (h264_prev_intra_pred_mode_flag(str, cabac, &mb->prev_intra8x8_pred_mode_flag[i])) return 1;
					if (!mb->prev_intra8x8_pred_mode_flag[i])
						if (h264_rem_intra_pred_mode(str, cabac, &mb->rem_intra8x8_pred_mode[i])) return 1;
				}
			}
		}
		if (slice->chroma_array_type == 1 || slice->chroma_array_type == 2) {
			if (h264_intra_chroma_pred_mode(str, cabac, &mb->intra_chroma_pred_mode)) return 1;
		} else {
			if (vs_infer(str, &mb->intra_chroma_pred_mode, 0)) return 1;
		}
		if (infer_intra(str, mb, 0)) return 1;
		if (infer_intra(str, mb, 1)) return 1;
	} else if (mb->mb_type != H264_MB_TYPE_B_DIRECT_16X16) {
		int ifrom[4], pmode[4];
		ifrom[0] = -1;
		pmode[0] = mb_part_info[mb->mb_type][1];
		switch (mb_part_info[mb->mb_type][0]) {
			case 0: /* 16x16 */
				ifrom[1] = 0;
				ifrom[2] = 0;
				ifrom[3] = 0;
				break;
			case 1: /* 16x8 */
				ifrom[1] = 0;
				ifrom[2] = -1;
				ifrom[3] = 2;
				pmode[2] = mb_part_info[mb->mb_type][2];
				break;
			case 2:
				ifrom[1] = -1;
				ifrom[2] = 0;
				ifrom[3] = 1;
				pmode[1] = mb_part_info[mb->mb_type][2];
				break;
		}
		int max = slice->num_ref_idx_l0_active_minus1;
		if (slice->mbaff_frame_flag && mb->mb_field_decoding_flag)
			max *= 2, max++;
		for (i = 0; i < 4; i++) {
			if (ifrom[i] == -1) {
				if (pmode[i] & 1) {
					if (h264_ref_idx(str, cabac, i, 0, max, &mb->ref_idx[0][i])) return 1;
				} else {
					if (vs_infer(str, &mb->ref_idx[0][i], 0)) return 1;
				}
			} else {
				if (vs_infer(str, &mb->ref_idx[0][i], mb->ref_idx[0][ifrom[i]])) return 1;
			}
		}
		max = slice->num_ref_idx_l1_active_minus1;
		if (slice->mbaff_frame_flag && mb->mb_field_decoding_flag)
			max *= 2, max++;
		for (i = 0; i < 4; i++) {
			if (ifrom[i] == -1) {
				if (pmode[i] & 2) {
					if (h264_ref_idx(str, cabac, i, 1, max, &mb->ref_idx[1][i])) return 1;
				} else {
					if (vs_infer(str, &mb->ref_idx[1][i], 0)) return 1;
				}
			} else {
				if (vs_infer(str, &mb->ref_idx[1][i], mb->ref_idx[1][ifrom[i]])) return 1;
			}
		}
		for (i = 0; i < 4; i++) {
			if (ifrom[i] == -1) {
				if (pmode[i] & 1) {
					if (h264_mvd(str, cabac, i * 4, 0, 0, &mb->mvd[0][i*4][0])) return 1;
					if (h264_mvd(str, cabac, i * 4, 1, 0, &mb->mvd[0][i*4][1])) return 1;
				} else {
					if (vs_infer(str, &mb->mvd[0][i*4][0], 0)) return 1;
					if (vs_infer(str, &mb->mvd[0][i*4][1], 0)) return 1;
				}
			} else {
				if (vs_infer(str, &mb->mvd[0][i*4][0], mb->mvd[0][ifrom[i]*4][0])) return 1;
				if (vs_infer(str, &mb->mvd[0][i*4][1], mb->mvd[0][ifrom[i]*4][1])) return 1;
			}
			for (j = 1; j < 4; j++) {
				if (vs_infer(str, &mb->mvd[0][i*4+j][0], mb->mvd[0][i*4][0])) return 1;
				if (vs_infer(str, &mb->mvd[0][i*4+j][1], mb->mvd[0][i*4][1])) return 1;
			}
		}
		for (i = 0; i < 4; i++) {
			if (ifrom[i] == -1) {
				if (pmode[i] & 2) {
					if (h264_mvd(str, cabac, i * 4, 0, 1, &mb->mvd[1][i*4][0])) return 1;
					if (h264_mvd(str, cabac, i * 4, 1, 1, &mb->mvd[1][i*4][1])) return 1;
				} else {
					if (vs_infer(str, &mb->mvd[1][i*4][0], 0)) return 1;
					if (vs_infer(str, &mb->mvd[1][i*4][1], 0)) return 1;
				}
			} else {
				if (vs_infer(str, &mb->mvd[1][i*4][0], mb->mvd[1][ifrom[i]*4][0])) return 1;
				if (vs_infer(str, &mb->mvd[1][i*4][1], mb->mvd[1][ifrom[i]*4][1])) return 1;
			}
			for (j = 1; j < 4; j++) {
				if (vs_infer(str, &mb->mvd[1][i*4+j][0], mb->mvd[1][i*4][0])) return 1;
				if (vs_infer(str, &mb->mvd[1][i*4+j][1], mb->mvd[1][i*4][1])) return 1;
			}
		}
		if (vs_infer(str, &mb->intra_chroma_pred_mode, 0)) return 1;
	} else {
		if (vs_infer(str, &mb->intra_chroma_pred_mode, 0)) return 1;
		if (infer_intra(str, mb, 0)) return 1;
		if (infer_intra(str, mb, 1)) return 1;
	}
	return 0;
}

int h264_sub_mb_pred(struct bitstream *str, struct h264_cabac_context *cabac, struct h264_slice *slice, struct h264_macroblock *mb) {
	int i;
	int pmode[4];
	int ifrom[16];
	for (i = 0; i < 4; i++) {
		if (h264_sub_mb_type(str, cabac, slice->slice_type, &mb->sub_mb_type[i])) return 1;
		pmode[i] = sub_mb_part_info[mb->sub_mb_type[i]][1];
		int sm = sub_mb_part_info[mb->sub_mb_type[i]][0];
		ifrom[i*4] = -1;
		switch (sm) {
			case 0:
				ifrom[i*4 + 1] = i*4;
				ifrom[i*4 + 2] = i*4;
				ifrom[i*4 + 3] = i*4;
				break;
			case 1:
				ifrom[i*4 + 1] = i*4;
				ifrom[i*4 + 2] = -1;
				ifrom[i*4 + 3] = i*4 + 2;
				break;
			case 2:
				ifrom[i*4 + 1] = -1;
				ifrom[i*4 + 2] = i*4;
				ifrom[i*4 + 3] = i*4 + 1;
				break;
			case 3:
				ifrom[i*4 + 1] = -1;
				ifrom[i*4 + 2] = -1;
				ifrom[i*4 + 3] = -1;
				break;
		}
	}
	int max = slice->num_ref_idx_l0_active_minus1;
	if (slice->mbaff_frame_flag && mb->mb_field_decoding_flag)
		max *= 2, max++;
	for (i = 0; i < 4; i++) {
		if (pmode[i] & 1 && mb->mb_type != H264_MB_TYPE_P_8X8REF0) {
			if (h264_ref_idx(str, cabac, i, 0, max, &mb->ref_idx[0][i])) return 1;
		} else {
			if (vs_infer(str, &mb->ref_idx[0][i], 0)) return 1;
		}
	}
	max = slice->num_ref_idx_l1_active_minus1;
	if (slice->mbaff_frame_flag && mb->mb_field_decoding_flag)
		max *= 2, max++;
	for (i = 0; i < 4; i++) {
		if (pmode[i] & 2) {
			if (h264_ref_idx(str, cabac, i, 1, max, &mb->ref_idx[1][i])) return 1;
		} else {
			if (vs_infer(str, &mb->ref_idx[1][i], 0)) return 1;
		}
	}
	for (i = 0; i < 16; i++) {
		if (ifrom[i] == -1) {
			if (pmode[i/4] & 1) {
				if (h264_mvd(str, cabac, i, 0, 0, &mb->mvd[0][i][0])) return 1;
				if (h264_mvd(str, cabac, i, 1, 0, &mb->mvd[0][i][1])) return 1;
			} else {
				if (vs_infer(str, &mb->mvd[0][i][0], 0)) return 1;
				if (vs_infer(str, &mb->mvd[0][i][1], 0)) return 1;
			}
		} else {
			if (vs_infer(str, &mb->mvd[0][i][0], mb->mvd[0][ifrom[i]][0])) return 1;
			if (vs_infer(str, &mb->mvd[0][i][1], mb->mvd[0][ifrom[i]][1])) return 1;
		}
	}
	for (i = 0; i < 16; i++) {
		if (ifrom[i] == -1) {
			if (pmode[i/4] & 2) {
				if (h264_mvd(str, cabac, i, 0, 1, &mb->mvd[1][i][0])) return 1;
				if (h264_mvd(str, cabac, i, 1, 1, &mb->mvd[1][i][1])) return 1;
			} else {
				if (vs_infer(str, &mb->mvd[1][i][0], 0)) return 1;
				if (vs_infer(str, &mb->mvd[1][i][1], 0)) return 1;
			}
		} else {
			if (vs_infer(str, &mb->mvd[1][i][0], mb->mvd[1][ifrom[i]][0])) return 1;
			if (vs_infer(str, &mb->mvd[1][i][1], mb->mvd[1][ifrom[i]][1])) return 1;
		}
	}
	if (vs_infer(str, &mb->intra_chroma_pred_mode, 0)) return 1;
	return 0;
}

int h264_macroblock_layer(struct bitstream *str, struct h264_cabac_context *cabac, struct h264_slice *slice, struct h264_macroblock *mb) {
	struct h264_picparm *picparm = slice->picparm;
	struct h264_seqparm *seqparm = slice->seqparm;
	if (h264_mb_type(str, cabac, slice->slice_type, &mb->mb_type)) return 1;
	if (mb->mb_type == H264_MB_TYPE_I_PCM) {
		if (vs_align_byte(str, VS_ALIGN_0)) return 1;
		int i;
		for (i = 0; i < 256; i++)
			if (vs_u(str, &mb->pcm_sample_luma[i], seqparm->bit_depth_luma_minus8 + 8)) return 1;
		for (i = 0; i < 2 * slice->mbwidthc * slice->mbheightc; i++)
			if (vs_u(str, &mb->pcm_sample_chroma[i], seqparm->bit_depth_chroma_minus8 + 8)) return 1;
		if (cabac)
			if (h264_cabac_init_arith(str, cabac)) return 1;
		if (vs_infer(str, &mb->mb_qp_delta, 0)) return 1;
		if (vs_infer(str, &mb->transform_size_8x8_flag, 0)) return 1;
		if (vs_infer(str, &mb->coded_block_pattern, 0x2f)) return 1;
		if (vs_infer(str, &mb->intra_chroma_pred_mode, 0)) return 1;
		if (infer_intra(str, mb, 0)) return 1;
		if (infer_intra(str, mb, 1)) return 1;
		for (i = 0; i < 17; i++) {
			mb->coded_block_flag[0][i] = 1;
			mb->coded_block_flag[1][i] = 1;
			mb->coded_block_flag[2][i] = 1;
		}
		for (i = 0; i < 16; i++) {
			mb->total_coeff[0][i] = 16;
			mb->total_coeff[1][i] = 16;
			mb->total_coeff[2][i] = 16;
		}
	} else {
		int noSubMbPartSizeLessThan8x8Flag = 1;
		if (h264_is_submb_mb_type(mb->mb_type)) {
			if (h264_sub_mb_pred(str, cabac, slice, mb)) return 1;
			int i;
			for (i = 0; i < 4; i++) {
				if (mb->sub_mb_type[i] != H264_SUB_MB_TYPE_B_DIRECT_8X8) {
					if (h264_sub_mb_type_split_mode(mb->sub_mb_type[i]))
						noSubMbPartSizeLessThan8x8Flag = 0;
				} else {
					if (!seqparm->direct_8x8_inference_flag)
						noSubMbPartSizeLessThan8x8Flag = 0;
				}
			}
			if (vs_infer(str, &mb->intra_chroma_pred_mode, 0)) return 1;
		} else {
			if (mb->mb_type == H264_MB_TYPE_I_NXN || mb->mb_type == H264_MB_TYPE_SI) {
				if (picparm->transform_8x8_mode_flag) {
					if (h264_transform_size_8x8_flag(str, cabac, &mb->transform_size_8x8_flag)) return 1;
				} else {
					if (vs_infer(str, &mb->transform_size_8x8_flag, 0)) return 1;
				}
			}
			if (h264_mb_pred(str, cabac, slice, mb)) return 1;
		}
		if (mb->mb_type == H264_MB_TYPE_I_NXN || mb->mb_type == H264_MB_TYPE_SI || mb->mb_type >= H264_MB_TYPE_I_END) {
			int has_chroma = slice->chroma_array_type < 3 && slice->chroma_array_type != 0;
			if (h264_coded_block_pattern(str, cabac, mb->mb_type, has_chroma, &mb->coded_block_pattern)) return 1;
			if (mb->mb_type >= H264_MB_TYPE_I_END) {
				if ((mb->coded_block_pattern & 0xf) && picparm->transform_8x8_mode_flag && noSubMbPartSizeLessThan8x8Flag && (mb->mb_type != H264_MB_TYPE_B_DIRECT_16X16 || seqparm->direct_8x8_inference_flag)) {
					if (h264_transform_size_8x8_flag(str, cabac, &mb->transform_size_8x8_flag)) return 1;
				} else {
					if (vs_infer(str, &mb->transform_size_8x8_flag, 0)) return 1;
				}
			}
		} else {
			int infer_cbp = (((mb->mb_type - H264_MB_TYPE_I_16X16_0_0_0) >> 2) % 3) << 4;
			if (mb->mb_type >= H264_MB_TYPE_I_16X16_0_0_1)
				infer_cbp |= 0xf;
			if (vs_infer(str, &mb->coded_block_pattern, infer_cbp)) return 1;
			if (vs_infer(str, &mb->transform_size_8x8_flag, 0)) return 1;
		}
		if (mb->coded_block_pattern || h264_is_intra_16x16_mb_type(mb->mb_type)) {
			if (h264_mb_qp_delta(str, cabac, &mb->mb_qp_delta))
				return 1;
		}
		if (h264_residual(str, cabac, slice, mb, 0, 15)) return 1;
	}
	return 0;
}

static int inferred_mb_field_decoding_flag(struct h264_slice *slice) {
	if (slice->mbaff_frame_flag) {
		const struct h264_macroblock *mbA = h264_mb_nb_p(slice, H264_MB_A, 0);
		const struct h264_macroblock *mbB = h264_mb_nb_p(slice, H264_MB_B, 0);
		if (mbA->mb_type != H264_MB_TYPE_UNAVAIL) {
			return mbA->mb_field_decoding_flag;
		} else if (mbB->mb_type != H264_MB_TYPE_UNAVAIL) {
			return mbB->mb_field_decoding_flag;
		} else {
			return 0;
		}
	} else {
		return slice->field_pic_flag;
	}
}

static int infer_skip(struct bitstream *str, struct h264_slice *slice, struct h264_macroblock *mb) {
	uint32_t skip_type = (slice->slice_type == H264_SLICE_TYPE_B ? H264_MB_TYPE_B_SKIP : H264_MB_TYPE_P_SKIP);
	if (slice->mbaff_frame_flag) {
		if (slice->curr_mb_addr & 1) {
			if (h264_is_skip_mb_type(slice->mbs[slice->curr_mb_addr & ~1].mb_type)) {
				int val = inferred_mb_field_decoding_flag(slice);
				if (vs_infer(str, &mb[-1].mb_field_decoding_flag, val)) return 1;
			}
			if (vs_infer(str, &mb->mb_field_decoding_flag, mb[-1].mb_field_decoding_flag)) return 1;
		}
	} else {
		if (vs_infer(str, &slice->mbs[slice->curr_mb_addr].mb_field_decoding_flag, slice->field_pic_flag)) return 1;

	}
	if (vs_infer(str, &mb->mb_type, skip_type)) return 1;
	if (vs_infer(str, &mb->mb_qp_delta, 0)) return 1;
	if (vs_infer(str, &mb->transform_size_8x8_flag, 0)) return 1;
	if (vs_infer(str, &mb->coded_block_pattern, 0)) return 1;
	if (vs_infer(str, &mb->intra_chroma_pred_mode, 0)) return 1;
	if (infer_intra(str, mb, 0)) return 1;
	if (infer_intra(str, mb, 1)) return 1;
	int i;
	for (i = 0; i < 17; i++) {
		mb->coded_block_flag[0][i] = 0;
		mb->coded_block_flag[1][i] = 0;
		mb->coded_block_flag[2][i] = 0;
	}
	for (i = 0; i < 16; i++) {
		mb->total_coeff[0][i] = 0;
		mb->total_coeff[1][i] = 0;
		mb->total_coeff[2][i] = 0;
	}
	return 0;
}

int h264_slice_data(struct bitstream *str, struct h264_slice *slice) {
	slice->prev_mb_addr = -1;
	slice->curr_mb_addr = slice->first_mb_in_slice * (1 + slice->mbaff_frame_flag);
	if (str->dir == VS_DECODE)
		slice->last_mb_in_slice = slice->curr_mb_addr;
	uint32_t skip_type = (slice->slice_type == H264_SLICE_TYPE_B ? H264_MB_TYPE_B_SKIP : H264_MB_TYPE_P_SKIP);
	if (slice->picparm->entropy_coding_mode_flag) {
		if (vs_align_byte(str, VS_ALIGN_1)) return 1;
		struct h264_cabac_context *cabac = h264_cabac_new(slice);
		if (h264_cabac_init_arith(str, cabac)) { h264_cabac_destroy(cabac); return 1; }
		while (1) {
			uint32_t mb_skip_flag = 0;
			if (slice->slice_type != H264_SLICE_TYPE_I && slice->slice_type != H264_SLICE_TYPE_SI) {
				if (str->dir == VS_ENCODE) {
					mb_skip_flag = slice->mbs[slice->curr_mb_addr].mb_type == skip_type;
				}
				/* mb_field_decoding_flag is decoded *after* mb_skip_flag in some circumstances, have to use an inferred value for CABAC prediction */
				int save = slice->mbs[slice->curr_mb_addr].mb_field_decoding_flag;
				int ival;
				if (slice->mbaff_frame_flag
						&& slice->curr_mb_addr & 1
						&& slice->mbs[slice->curr_mb_addr - 1].mb_type != skip_type) {
					ival = slice->mbs[slice->curr_mb_addr - 1].mb_field_decoding_flag;
				} else {
					ival = inferred_mb_field_decoding_flag(slice);
				}
				slice->mbs[slice->curr_mb_addr].mb_field_decoding_flag = ival;
				if (h264_mb_skip_flag(str, cabac, &mb_skip_flag)) { h264_cabac_destroy(cabac); return 1; }
				slice->mbs[slice->curr_mb_addr].mb_field_decoding_flag = save;
			}
			if (mb_skip_flag) {
				if (infer_skip(str, slice, &slice->mbs[slice->curr_mb_addr])) { h264_cabac_destroy(cabac); return 1; }
			} else {
				if (slice->mbaff_frame_flag) {
					uint32_t first_addr = slice->curr_mb_addr & ~1;
					if (slice->curr_mb_addr == first_addr) {
						if (h264_mb_field_decoding_flag(str, cabac, &slice->mbs[first_addr].mb_field_decoding_flag)) { h264_cabac_destroy(cabac); return 1; }
					} else {
						if (slice->mbs[first_addr].mb_type == skip_type) {
							if (h264_mb_field_decoding_flag(str, cabac, &slice->mbs[first_addr].mb_field_decoding_flag)) { h264_cabac_destroy(cabac); return 1; }
						}
						if (vs_infer(str, &slice->mbs[first_addr + 1].mb_field_decoding_flag, slice->mbs[first_addr].mb_field_decoding_flag)) { h264_cabac_destroy(cabac); return 1; }
					}
				} else {
					if (vs_infer(str, &slice->mbs[slice->curr_mb_addr].mb_field_decoding_flag, slice->field_pic_flag)) { h264_cabac_destroy(cabac); return 1; }
				}
				if (h264_macroblock_layer(str, cabac, slice, &slice->mbs[slice->curr_mb_addr])) { h264_cabac_destroy(cabac); return 1; }
			}
			if (!slice->mbaff_frame_flag || (slice->curr_mb_addr & 1)) {
				uint32_t end_of_slice_flag = slice->last_mb_in_slice == slice->curr_mb_addr;
				if (h264_cabac_terminate(str, cabac, &end_of_slice_flag)) { h264_cabac_destroy(cabac); return 1; }
				if (end_of_slice_flag) {
					slice->last_mb_in_slice = slice->curr_mb_addr;
					h264_cabac_destroy(cabac);
					/* XXX: cabac_zero_word crap */
					return vs_align_byte(str, VS_ALIGN_0);
				}
			}
			slice->prev_mb_addr = slice->curr_mb_addr;
			if (str->dir == VS_DECODE)
				slice->last_mb_in_slice = slice->curr_mb_addr;
			slice->curr_mb_addr = h264_next_mb_addr(slice, slice->curr_mb_addr);
			if (slice->curr_mb_addr >= slice->pic_size_in_mbs) {
				fprintf(stderr, "MB index out of range!\n");
				return 1;
			}
		}
	} else {
		while (1) {
			int end = 0;
			uint32_t mb_skip_run = 0;
			if (slice->slice_type != H264_SLICE_TYPE_I && slice->slice_type != H264_SLICE_TYPE_SI) {
				if (str->dir == VS_ENCODE) {
					mb_skip_run = 0;
					while (slice->mbs[slice->curr_mb_addr].mb_type == skip_type) {
						mb_skip_run++;
						if (infer_skip(str, slice, &slice->mbs[slice->curr_mb_addr])) return 1;
						if (slice->curr_mb_addr == slice->last_mb_in_slice) {
							end = 1;
							break;
						}
						slice->prev_mb_addr = slice->curr_mb_addr;
						slice->curr_mb_addr = h264_next_mb_addr(slice, slice->curr_mb_addr);
					}
					if (vs_ue(str, &mb_skip_run)) return 1;
					if (end)
						goto out_cavlc;
				} else {
					if (vs_ue(str, &mb_skip_run)) return 1;
					while (mb_skip_run--) {
						if (slice->curr_mb_addr >= slice->pic_size_in_mbs) {
							fprintf(stderr, "MB index out of range!\n");
							return 1;
						}
						slice->last_mb_in_slice = slice->curr_mb_addr;
						slice->mbs[slice->curr_mb_addr].mb_type = skip_type;
						if (infer_skip(str, slice, &slice->mbs[slice->curr_mb_addr])) return 1;
						slice->prev_mb_addr = slice->curr_mb_addr;
						slice->last_mb_in_slice = slice->curr_mb_addr;
						slice->curr_mb_addr = h264_next_mb_addr(slice, slice->curr_mb_addr);
					}
					int more = vs_has_more_data(str);
					if (more == -1)
						return 1;
					if (more == 0)
						goto out_cavlc;
				}
			}
			if (slice->curr_mb_addr >= slice->pic_size_in_mbs) {
				fprintf(stderr, "MB index out of range!\n");
				return 1;
			}
			if (slice->mbaff_frame_flag) {
				uint32_t first_addr = slice->curr_mb_addr & ~1;
				if (slice->curr_mb_addr == first_addr) {
					if (h264_mb_field_decoding_flag(str, 0, &slice->mbs[first_addr].mb_field_decoding_flag)) return 1;
				} else {
					if (slice->mbs[first_addr].mb_type == skip_type)
						if (h264_mb_field_decoding_flag(str, 0, &slice->mbs[first_addr].mb_field_decoding_flag)) return 1;
					if (vs_infer(str, &slice->mbs[first_addr + 1].mb_field_decoding_flag, slice->mbs[first_addr].mb_field_decoding_flag)) return 1;
				}
			} else {
				if (vs_infer(str, &slice->mbs[slice->curr_mb_addr].mb_field_decoding_flag, slice->field_pic_flag)) return 1;
			}
			if (h264_macroblock_layer(str, 0, slice, &slice->mbs[slice->curr_mb_addr])) return 1;
			if(str->dir == VS_ENCODE) {
				if (slice->last_mb_in_slice == slice->curr_mb_addr)
					goto out_cavlc;
			} else {
				slice->last_mb_in_slice = slice->curr_mb_addr;
				int more = vs_has_more_data(str);
				if (more == -1)
					return 1;
				if (more == 0)
					goto out_cavlc;
			}
			slice->prev_mb_addr = slice->curr_mb_addr;
			if (str->dir == VS_DECODE)
				slice->last_mb_in_slice = slice->curr_mb_addr;
			slice->curr_mb_addr = h264_next_mb_addr(slice, slice->curr_mb_addr);
			if (slice->curr_mb_addr >= slice->pic_size_in_mbs) {
				fprintf(stderr, "MB index out of range!\n");
				return 1;
			}
		}
out_cavlc:
		if (vs_end(str)) return 1;
		return 0;
	}
}
