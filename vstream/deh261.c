/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "h261.h"
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
	struct bitstream *str = vs_new_decode(VS_H261, bytes, bytesnum);
	struct h261_picparm *picparm = calloc(sizeof *picparm, 1);
	while (1) {
		uint32_t start_code;
		if (vs_start(str, &start_code)) goto err;
		printf("Start code: %02x\n", start_code);
		if (start_code == 0) {
			if (h261_picparm(str, picparm)) goto err;
			h261_print_picparm(picparm);
		} else if (start_code <= 12) {
			struct h261_gob *gob = calloc (sizeof *gob, 1);
			gob->gn = start_code;
			if (h261_gob(str, gob)) {
				h261_print_gob(gob);
				h261_del_gob(gob);
				goto err;
			}
			h261_print_gob(gob);
			h261_del_gob(gob);
		} else {
			fprintf(stderr, "Invalid start code %d\n", start_code);
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
