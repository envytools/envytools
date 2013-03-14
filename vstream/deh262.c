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

#include "h262.h"
#include "vstream.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
	uint8_t *bytes = 0;
	int bytesnum = 0;
	int bytesmax = 0;
	int c;
	int res;
	while ((c = getchar()) != EOF) {
		ADDARRAY(bytes, c);
	}
	struct bitstream *str = vs_new_decode(VS_H262, bytes, bytesnum);
	struct h262_seqparm *seqparm = calloc(sizeof *seqparm, 1);
	struct h262_picparm *picparm = calloc(sizeof *picparm, 1);
	struct h262_gop *gop = calloc(sizeof *gop, 1);
	struct h262_slice *slice;
	while (1) {
		uint32_t start_code;
		uint32_t ext_start_code;
		if (vs_start(str, &start_code)) goto err;
		printf("Start code: %02x\n", start_code);
		switch (start_code) {
			case H262_START_CODE_SEQPARM:
				if (h262_seqparm(str, seqparm))
					goto err;
				if (vs_end(str))
					goto err;
				h262_print_seqparm(seqparm);
				break;
			case H262_START_CODE_PICPARM:
				if (h262_picparm(str, seqparm, picparm))
					goto err;
				if (vs_end(str))
					goto err;
				h262_print_picparm(picparm);
				break;
			case H262_START_CODE_GOP:
				if (h262_gop(str, gop))
					goto err;
				if (vs_end(str))
					goto err;
				h262_print_gop(gop);
				break;
			case H262_START_CODE_EXTENSION:
				if (vs_u(str, &ext_start_code, 4)) goto err;
				printf("Extension start code: %d\n", ext_start_code);
				switch (ext_start_code) {
					case H262_EXT_SEQUENCE:
						if (h262_seqparm_ext(str, seqparm))
							goto err;
						if (vs_end(str))
							goto err;
						h262_print_seqparm(seqparm);
						break;
					case H262_EXT_PIC_CODING:
						if (h262_picparm_ext(str, seqparm, picparm))
							goto err;
						if (vs_end(str))
							goto err;
						h262_print_picparm(picparm);
						break;
					default:
						fprintf(stderr, "Unknown extension start code\n");
						goto err;
				}
				break;
			case H262_START_CODE_END:
				printf ("End of sequence.\n");
				break;
			default:
				if (start_code >= H262_START_CODE_SLICE_BASE && start_code <= H262_START_CODE_SLICE_LAST) {
					slice = calloc (sizeof *slice, 1);
					slice->mbs = calloc (sizeof *slice->mbs, picparm->pic_size_in_mbs);
					slice->slice_vertical_position = start_code - H262_START_CODE_SLICE_BASE;
					if (seqparm->vertical_size > 2800) {
						uint32_t svp_ext;
						if (vs_u(str, &svp_ext, 3)) {
							h262_del_slice(slice);
							goto err;
						}
						if (slice->slice_vertical_position >= 0x80) {
							fprintf(stderr, "Invalid slice start code for large picture\n");
							goto err;
						}
						slice->slice_vertical_position += svp_ext * 0x80;
					}
					if (slice->slice_vertical_position >= picparm->pic_height_in_mbs) {
						fprintf(stderr, "slice_vertical_position too large\n");
						goto err;
					}
					if (h262_slice(str, seqparm, picparm, slice)) {
						h262_print_slice(seqparm, picparm, slice);
						h262_del_slice(slice);
						goto err;
					}
					h262_print_slice(seqparm, picparm, slice);
					if (vs_end(str)) {
						h262_del_slice(slice);
						goto err;
					}
					h262_del_slice(slice);
					break;
				} else {
					fprintf(stderr, "Unknown start code\n");
					goto err;
				}
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
