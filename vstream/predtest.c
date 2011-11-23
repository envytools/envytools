#include "h264.h"
#include "vstream.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
	struct bitstream *str = vs_new_encode(VS_H264);
	struct h264_sliceparm *slp = calloc (sizeof *slp, 1);
	slp->slice_type = 1;
	slp->num_ref_idx_l0_active_minus1 = 7;
	slp->num_ref_idx_l1_active_minus1 = 0x13;
	struct h264_pred_weight_table *weights = calloc (sizeof *weights, 1);
	weights->luma_log2_weight_denom = 3;
	weights->chroma_log2_weight_denom = 5;
	int i;
	for (i = 0; i <= slp->num_ref_idx_l0_active_minus1; i++) {
		weights->l0[i].luma_weight_flag = 1;
		weights->l0[i].chroma_weight_flag = 1;
	}
	for (i = 0; i <= slp->num_ref_idx_l1_active_minus1; i++) {
		weights->l1[i].luma_weight_flag = 1;
		weights->l1[i].chroma_weight_flag = 1;
	}
	weights->l0[3].luma_weight_flag = 0;
	weights->l0[3].luma_weight = 8;
	weights->l0[4].luma_weight = 0x3;
	weights->l0[5].luma_weight = -0x3;
	weights->l0[4].luma_offset = -5;
	weights->l0[5].luma_weight = 5;
	weights->l1[0x10].chroma_weight_flag = 0;
	weights->l1[0x10].chroma_weight[0] = 32;
	weights->l1[0x10].chroma_weight[1] = 32;
	weights->l1[0x11].chroma_weight[0] = 7;
	weights->l1[0x12].chroma_weight[1] = 8;
	weights->l1[0x13].chroma_offset[0] = 9;
	weights->l1[0x14].chroma_offset[1] = 10;
	if (h264_pred_weight_table(str, slp, weights)) {
		fprintf(stderr, "Failed\n");
		return 1;
	}
	if (vs_end(str)) {
		fprintf(stderr, "Failed\n");
		return 1;
	}
	fwrite(str->bytes, str->bytesnum, 1, stdout);
	return 0;
}
