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
#include "vstream.h"
#include "util.h"
#include <stdio.h>

int main() {
	uint8_t *bytes = 0;
	int bytesnum = 0;
	int bytesmax = 0;
	int c;
	while ((c = getchar()) != EOF) {
		ADDARRAY(bytes, c);
	}
	struct bitstream *str = vs_new_decode(VS_H264, bytes, bytesnum);
	struct h264_seqparm *seqparms[32] = { 0 };
	int res;
	while (1) {
		uint32_t start_code;
		if (vs_start(str, &start_code)) goto err;
		if (start_code & 0x80) {
			fprintf(stderr, "forbidden_zero_bit not 0\n");
			goto err;
		}
		uint32_t nal_ref_idc = start_code >> 5;
		uint32_t nal_unit_type = start_code & 0x1f;
		printf("NAL unit:\n");
		printf("\tnal_ref_idc = %d\n", nal_ref_idc);
		printf("\tnal_unit_type = %d\n", nal_unit_type);
		struct h264_seqparm *sp;
		switch (nal_unit_type) {
			case H264_NAL_UNIT_TYPE_SEQPARM:
				sp = calloc (sizeof *sp, 1);
				if (h264_seqparm(str, sp)) {
					free(sp);
					goto err;
				} else {
					if (vs_end(str)) goto err;
					h264_print_seqparm(sp);
					if (sp->seq_parameter_set_id > 31) {
						fprintf(stderr, "seq_parameter_set_id out of bounds\n");
						goto err;
					} else {
						if (seqparms[sp->seq_parameter_set_id]) {
							h264_del_seqparm(seqparms[sp->seq_parameter_set_id]);
						}
						seqparms[sp->seq_parameter_set_id] = sp;
					}
				}
				break;
			case H264_NAL_UNIT_TYPE_ACC_UNIT_DELIM: {
				uint32_t primary_pic_type;
				if (vs_u(str, &primary_pic_type, 3)) goto err;
				if (vs_end(str)) goto err;
				printf ("Access unit delimiter:\n");
				static const char *const names[8] = {
					"I",
					"P+I",
					"P+B+I",
					"SI",
					"SP+SI",
					"I+SI",
					"P+I+SP+SI",
					"P+B+I+SP+SI",
				};
				printf ("\tprimary_pic_type = %d [%s]\n", primary_pic_type, names[primary_pic_type]);
				break;
			}
			case H264_NAL_UNIT_TYPE_END_SEQ:
				printf ("End of sequence.\n");
				break;
			case H264_NAL_UNIT_TYPE_END_STREAM:
				printf ("End of stream.\n");
				break;
			default:
				fprintf(stderr, "Unknown NAL type\n");
				goto err;
		}
		printf("NAL decoded successfully\n\n");
		continue;
err:
		res = vs_search_start(str);
		if (res == -1)
			return 1;
		if (!res)
			break;
		printf("\n");
	}
	return 0;
}
