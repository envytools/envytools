#include "vstream.h"
#include "h264.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	FILE *out = 0;
	if (argc >= 2) {
		out = fopen(argv[1], "w");
		if (!out) {
			perror("fopen");
			return 1;
		}
	}
	struct bitstream *str = vs_new_encode(VS_H264);
	uint32_t val;
	val = 0xde;
	if (vs_start(str, &val))
		return 1;

	struct h264_seqparm *seqparm = calloc(sizeof *seqparm, 1);
	struct h264_picparm *picparm = calloc(sizeof *picparm, 1);
	struct h264_slice *slice = calloc(sizeof *slice, 1);
	seqparm->frame_mbs_only_flag = 0;
	seqparm->direct_8x8_inference_flag = 0;
	picparm->num_slice_groups_minus1 = 0;
	picparm->constrained_intra_pred_flag = 0;
	picparm->transform_8x8_mode_flag = 1;
	picparm->entropy_coding_mode_flag = 1;
	slice->seqparm = seqparm;
	slice->picparm = picparm;
	slice->sliceqpy = 26;
	slice->pic_width_in_mbs = 8;
	slice->pic_size_in_mbs = slice->pic_width_in_mbs * 24;
	slice->first_mb_in_slice = 0;
	slice->nal_unit_type = 1;
	slice->slice_type = H264_SLICE_TYPE_P;
	slice->mbaff_frame_flag = 0;
	slice->field_pic_flag = 1;
	slice->cabac_init_idc = 2;
	slice->chroma_array_type = 1;
	slice->num_ref_idx_l0_active_minus1 = 31;
	slice->num_ref_idx_l1_active_minus1 = 31;
	slice->bit_depth_luma_minus8 = 0;
	slice->bit_depth_chroma_minus8 = 0;
	slice->mbwidthc = 8;
	slice->mbheightc = 8;
	slice->mbs = calloc(sizeof *slice->mbs, slice->pic_size_in_mbs);
	int i, j, k;
	for (i = 0; i < 96; i++) {
		slice->mbs[i].mb_field_decoding_flag = i >> 2 & 1;
		slice->mbs[i].coded_block_pattern = i * 3 % 48;
		slice->mbs[i].transform_size_8x8_flag = i& 1;
		slice->mbs[i].mb_qp_delta = (i%5) - 2;
		if (i < 16) {
			slice->mbs[i].mb_type = 0;
		} else if (i < 40) {
			slice->mbs[i].mb_type = 1 + (i- 16);
		} else if (i >= 44 && slice->slice_type == H264_SLICE_TYPE_P) {
			if (i < 48) {
				slice->mbs[i].mb_type = H264_MB_TYPE_P_SKIP;
			} else if (i < 72) {
				int x = (i - 48)/8;
				slice->mbs[i].mb_type = H264_MB_TYPE_P_L0_16X16 + x;
				if (x == 0) {
					for (j = 0; j < 4; j++)
						slice->mbs[i].ref_idx[0][j] = 15 + (i & 7);
					for (j = 0; j < 16; j++) {
						slice->mbs[i].mvd[0][j][0] = -((i & 7) + 3);
						slice->mbs[i].mvd[0][j][1] = (i&3);
					}
				} else if (x == 1) {
					for (j = 0; j < 4; j++)
						slice->mbs[i].ref_idx[0][j] = 4 + (j >> 1);
					for (j = 0; j < 16; j++) {
						slice->mbs[i].mvd[0][j][0] = 12 + (j >> 3);
						slice->mbs[i].mvd[0][j][1] = -(j >> 3);
					}
				} else if (x == 2) {
					for (j = 0; j < 4; j++)
						slice->mbs[i].ref_idx[0][j] = 2 + (j & 1);
					for (j = 0; j < 16; j++) {
						slice->mbs[i].mvd[0][j][0] = 6 + (j>>2 & 1);
						slice->mbs[i].mvd[0][j][1] = -(j>>2 & 1);
					}
				}
			} else {
				slice->mbs[i].mb_type = H264_MB_TYPE_P_8X8;
				slice->mbs[i].transform_size_8x8_flag = 0;
				for (j = 0; j < 4; j++) {
					slice->mbs[i].sub_mb_type[j] = j;
					slice->mbs[i].ref_idx[0][j] = j+1;
					int tt[4] = { 0, 2, 1, 3};
					for (k = 0; k < 4; k++) {
						int kk = j * 4 + (k & j[tt]);
						slice->mbs[i].mvd[0][j*4+k][0] = 16+kk;
						slice->mbs[i].mvd[0][j*4+k][1] = kk - 32;;
					}
				}
			}
		} else if (i >= 44 && slice->slice_type == H264_SLICE_TYPE_B) {
			if (i < 48) {
				slice->mbs[i].mb_type = H264_MB_TYPE_B_SKIP;
			} else if (i < 72) {
				/* XXX */
				slice->mbs[i].mb_type = H264_MB_TYPE_B_SKIP;
			} else {
				/* XXX */
				slice->mbs[i].mb_type = H264_MB_TYPE_B_SKIP;
			}
		} else {
			slice->mbs[i].mb_type = H264_MB_TYPE_I_PCM;
		}
		if (slice->mbs[i].mb_type == H264_MB_TYPE_I_NXN || slice->mbs[i].mb_type == H264_MB_TYPE_SI) {
			for (j = 0; j < 16; j++) {
				slice->mbs[i].prev_intra4x4_pred_mode_flag[j] = j & 1;
				slice->mbs[i].rem_intra4x4_pred_mode[j] = j >> 1;
			}
			for (j = 0; j < 4; j++) {
				slice->mbs[i].prev_intra8x8_pred_mode_flag[j] = j & 1;
				slice->mbs[i].rem_intra8x8_pred_mode[j] = j >> 1;
			}
		}
		if (slice->mbs[i].mb_type < H264_MB_TYPE_P_BASE) {
			slice->mbs[i].intra_chroma_pred_mode = i >> 2 & 3;
		}
		if (slice->mbs[i].mb_type == H264_MB_TYPE_P_SKIP || slice->mbs[i].mb_type == H264_MB_TYPE_B_SKIP) {
			slice->mbs[i].mb_field_decoding_flag = 0;
			slice->mbs[i].coded_block_pattern = 0;
			slice->mbs[i].transform_size_8x8_flag = 0;
			slice->mbs[i].mb_qp_delta = 0;
			slice->mbs[i].intra_chroma_pred_mode = 0;
		} else if (slice->mbs[i].mb_type == H264_MB_TYPE_I_PCM) {
			slice->mbs[i].coded_block_pattern = 0x2f;
			slice->mbs[i].transform_size_8x8_flag = 0;
			slice->mbs[i].mb_qp_delta = 0;
			slice->mbs[i].intra_chroma_pred_mode = 0;
			for (j = 0; j < 256; j++) {
				if (i & 1)
					slice->mbs[i].pcm_sample_chroma[j] = j;
				if (i & 2)
					slice->mbs[i].pcm_sample_luma[j] = j;
			}
		} else {
			if (h264_is_intra_16x16_mb_type(slice->mbs[i].mb_type)) {
				int mbt = slice->mbs[i].mb_type;
				int infer_cbp = (((mbt - H264_MB_TYPE_I_16X16_0_0_0) >> 2) % 3) << 4;
				if (mbt >= H264_MB_TYPE_I_16X16_0_0_1)
					infer_cbp |= 0xf;
				slice->mbs[i].coded_block_pattern = infer_cbp;
				slice->mbs[i].transform_size_8x8_flag = 0;
				for (j = 0; j < 16; j++) {
					slice->mbs[i].block_luma_dc[0][j] = 0x100 + j;
					if (slice->mbs[i].coded_block_pattern >> (j >> 2) & 1) {
						for (k = 0; k < 15; k++) {
							if (j) {
								slice->mbs[i].block_luma_ac[0][j][k] = j * 16 + k + 1;
							}
						}
					}
				}
			} else {
				if (!slice->mbs[i].coded_block_pattern)
					slice->mbs[i].mb_qp_delta = 0;
				for (j = 0; j < 16; j++) {
					if (slice->mbs[i].coded_block_pattern >> (j >> 2) & 1) {
						for (k = 0; k < 16; k++) {
							if (j) {
								slice->mbs[i].block_luma_4x4[0][j][k] = j * 16 + k;
								slice->mbs[i].block_luma_8x8[0][j>>2][(j&3)*16+k] = j*16 + k;
							}
						}
					}
				}
			}
			if (slice->mbs[i].coded_block_pattern & 0x30) {
				for (k = 0; k < 4; k++) {
					slice->mbs[i].block_chroma_dc[i&1][k] = -0x10 + k;
				}
			}
			if (slice->mbs[i].coded_block_pattern & 0x20) {
				for (j = 0; j < 4; j++) {
					for (k = 0; k < 15; k++) {
						if (j != 1)
							slice->mbs[i].block_chroma_ac[i>>1&1][j][k] = k - 0x1000 + j * 0x100;
					}
				}
			}
		}
		if (!slice->mbaff_frame_flag)
			slice->mbs[i].mb_field_decoding_flag = slice->field_pic_flag;
	}
	slice->last_mb_in_slice = i - 1;
	
	if (h264_slice_data(str, slice)) return 1;

	if (out)
		fwrite(str->bytes, str->bytesnum, 1, out);

	struct bitstream *nstr = vs_new_decode(VS_H264, str->bytes, str->bytesnum);
	if (vs_start(nstr, &val))
		return 1;
	if (val != 0xde) {
		fprintf (stderr, "Fail 1\n");
		return 1;
	}
	if (h264_slice_data(nstr, slice)) {
		h264_print_slice_data(slice);
		return 1;
	}
	h264_print_slice_data(slice);

	fprintf (stderr, "All ok!\n");

	return 0;
}
