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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "h261.h"
#include "vstream.h"
#include <stdio.h>
#include <stdlib.h>

void h261_del_picparm(struct h261_picparm *picparm) {
	free(picparm);
}

void h261_del_gob(struct h261_gob *gob) {
	free(gob);
}

int h261_picparm(struct bitstream *str, struct h261_picparm *picparm) {
	if (vs_u(str, &picparm->tr, 5)) return 1;
	if (vs_u(str, &picparm->ptype, 6)) return 1;
	if (!(picparm->ptype & 1)) {
		fprintf(stderr, "Spare bit unset - H.263 stream?\n");
		return 1;
	}
	uint32_t pei = 0;
	if (vs_u(str, &pei, 1)) return 1;
	while (pei) {
		if (vs_u(str, &pei, 8)) return 1;
		if (vs_u(str, &pei, 1)) return 1;
	}
	return 0;
}

static const struct vs_vlc_val block_vlc[] = {
	{ 0x00000,  2, 1,0 },
	{ 0x00001,  2, 1,1 },
	{ 0x00002,  4, 0,1,0,0 },
	{ 0x00003,  5, 0,0,1,0,1 },
	{ 0x00004,  7, 0,0,0,0,1,1,0 },
	{ 0x00005,  8, 0,0,1,0,0,1,1,0 },
	{ 0x00006,  8, 0,0,1,0,0,0,0,1 },
	{ 0x00007, 10, 0,0,0,0,0,0,1,0,1,0 },
	{ 0x00008, 12, 0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x00009, 12, 0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x0000a, 12, 0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x0000b, 12, 0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x0000c, 13, 0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x0000d, 13, 0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x0000e, 13, 0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x0000f, 13, 0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x01001,  3, 0,1,1 },
	{ 0x01002,  6, 0,0,0,1,1,0 },
	{ 0x01003,  8, 0,0,1,0,0,1,0,1 },
	{ 0x01004, 10, 0,0,0,0,0,0,1,1,0,0 },
	{ 0x01005, 12, 0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x01006, 13, 0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x01007, 13, 0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x02001,  4, 0,1,0,1 },
	{ 0x02002,  7, 0,0,0,0,1,0,0 },
	{ 0x02003, 10, 0,0,0,0,0,0,1,0,1,1 },
	{ 0x02004, 12, 0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x02005, 13, 0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x03001,  5, 0,0,1,1,1 },
	{ 0x03002,  8, 0,0,1,0,0,1,0,0 },
	{ 0x03003, 12, 0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x03004, 13, 0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x04001,  5, 0,0,1,1,0 },
	{ 0x04002, 10, 0,0,0,0,0,0,1,1,1,1 },
	{ 0x04003, 12, 0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x05001,  6, 0,0,0,1,1,1 },
	{ 0x05002, 10, 0,0,0,0,0,0,1,0,0,1 },
	{ 0x05003, 13, 0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x06001,  6, 0,0,0,1,0,1 },
	{ 0x06002, 12, 0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x07001,  6, 0,0,0,1,0,0 },
	{ 0x07002, 12, 0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x08001,  7, 0,0,0,0,1,1,1 },
	{ 0x08002, 12, 0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x09001,  7, 0,0,0,0,1,0,1 },
	{ 0x09002, 13, 0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x0a001,  8, 0,0,1,0,0,1,1,1 },
	{ 0x0a002, 13, 0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x0b001,  8, 0,0,1,0,0,0,1,1 },
	{ 0x0c001,  8, 0,0,1,0,0,0,1,0 },
	{ 0x0d001,  8, 0,0,1,0,0,0,0,0 },
	{ 0x0e001, 10, 0,0,0,0,0,0,1,1,1,0 },
	{ 0x0f001, 10, 0,0,0,0,0,0,1,1,0,1 },
	{ 0x10001, 10, 0,0,0,0,0,0,1,0,0,0 },
	{ 0x11001, 12, 0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x12001, 12, 0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x13001, 12, 0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x14001, 12, 0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x15001, 12, 0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x16001, 13, 0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x17001, 13, 0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x18001, 13, 0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x19001, 13, 0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x1a001, 13, 0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0xfffff,  6, 0,0,0,0,0,1 },
	{ 0 },
};

static const struct vs_vlc_val block_0_vlc[] = {
	{ 0x00001,  1, 1 },
	{ 0x00002,  4, 0,1,0,0 },
	{ 0x00003,  5, 0,0,1,0,1 },
	{ 0x00004,  7, 0,0,0,0,1,1,0 },
	{ 0x00005,  8, 0,0,1,0,0,1,1,0 },
	{ 0x00006,  8, 0,0,1,0,0,0,0,1 },
	{ 0x00007, 10, 0,0,0,0,0,0,1,0,1,0 },
	{ 0x00008, 12, 0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x00009, 12, 0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x0000a, 12, 0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x0000b, 12, 0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x0000c, 13, 0,0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x0000d, 13, 0,0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x0000e, 13, 0,0,0,0,0,0,0,0,1,1,0,0,0 },
	{ 0x0000f, 13, 0,0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x01001,  3, 0,1,1 },
	{ 0x01002,  6, 0,0,0,1,1,0 },
	{ 0x01003,  8, 0,0,1,0,0,1,0,1 },
	{ 0x01004, 10, 0,0,0,0,0,0,1,1,0,0 },
	{ 0x01005, 12, 0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0x01006, 13, 0,0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x01007, 13, 0,0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x02001,  4, 0,1,0,1 },
	{ 0x02002,  7, 0,0,0,0,1,0,0 },
	{ 0x02003, 10, 0,0,0,0,0,0,1,0,1,1 },
	{ 0x02004, 12, 0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x02005, 13, 0,0,0,0,0,0,0,0,1,0,1,0,0 },
	{ 0x03001,  5, 0,0,1,1,1 },
	{ 0x03002,  8, 0,0,1,0,0,1,0,0 },
	{ 0x03003, 12, 0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x03004, 13, 0,0,0,0,0,0,0,0,1,0,0,1,1 },
	{ 0x04001,  5, 0,0,1,1,0 },
	{ 0x04002, 10, 0,0,0,0,0,0,1,1,1,1 },
	{ 0x04003, 12, 0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x05001,  6, 0,0,0,1,1,1 },
	{ 0x05002, 10, 0,0,0,0,0,0,1,0,0,1 },
	{ 0x05003, 13, 0,0,0,0,0,0,0,0,1,0,0,1,0 },
	{ 0x06001,  6, 0,0,0,1,0,1 },
	{ 0x06002, 12, 0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x07001,  6, 0,0,0,1,0,0 },
	{ 0x07002, 12, 0,0,0,0,0,0,0,1,0,1,0,1 },
	{ 0x08001,  7, 0,0,0,0,1,1,1 },
	{ 0x08002, 12, 0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x09001,  7, 0,0,0,0,1,0,1 },
	{ 0x09002, 13, 0,0,0,0,0,0,0,0,1,0,0,0,1 },
	{ 0x0a001,  8, 0,0,1,0,0,1,1,1 },
	{ 0x0a002, 13, 0,0,0,0,0,0,0,0,1,0,0,0,0 },
	{ 0x0b001,  8, 0,0,1,0,0,0,1,1 },
	{ 0x0c001,  8, 0,0,1,0,0,0,1,0 },
	{ 0x0d001,  8, 0,0,1,0,0,0,0,0 },
	{ 0x0e001, 10, 0,0,0,0,0,0,1,1,1,0 },
	{ 0x0f001, 10, 0,0,0,0,0,0,1,1,0,1 },
	{ 0x10001, 10, 0,0,0,0,0,0,1,0,0,0 },
	{ 0x11001, 12, 0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x12001, 12, 0,0,0,0,0,0,0,1,1,0,1,0 },
	{ 0x13001, 12, 0,0,0,0,0,0,0,1,1,0,0,1 },
	{ 0x14001, 12, 0,0,0,0,0,0,0,1,0,1,1,1 },
	{ 0x15001, 12, 0,0,0,0,0,0,0,1,0,1,1,0 },
	{ 0x16001, 13, 0,0,0,0,0,0,0,0,1,1,1,1,1 },
	{ 0x17001, 13, 0,0,0,0,0,0,0,0,1,1,1,1,0 },
	{ 0x18001, 13, 0,0,0,0,0,0,0,0,1,1,1,0,1 },
	{ 0x19001, 13, 0,0,0,0,0,0,0,0,1,1,1,0,0 },
	{ 0x1a001, 13, 0,0,0,0,0,0,0,0,1,1,0,1,1 },
	{ 0xfffff,  6, 0,0,0,0,0,1 },
	{ 0 },
};

int h261_block(struct bitstream *str, int32_t *block, int intra) {
	int i = 0;
	if (intra) {
		uint32_t val = block[0];
		if (str->dir == VS_ENCODE) {
			if (val >= 0xff || val == 0) {
				fprintf(stderr, "Invalid INTRA DC coeff\n");
				return 1;
			}
			if (val == 0x80)
				val = 0xff;
		}
		if (vs_u(str, &val, 8)) return 1;
		if (str->dir == VS_DECODE) {
			if (val == 0 || val == 0x80) {
				fprintf(stderr, "Invalid INTRA DC coeff\n");
				return 1;
			}
			if (val == 0xff)
				val = 0x80;
			block[0] = val;
		}
		i = 1;
	}
	while (1) {
		uint32_t tmp, run, eb, sign;
		int32_t coeff;
		const struct vs_vlc_val *tab;
		if (i == 0)
			tab = block_0_vlc;
		else
			tab = block_vlc;
		if (str->dir == VS_ENCODE) {
			run = 0;
			while (i < 64 && !block[i])
				i++, run++;
			if (i == 64) {
				tmp = 0;
			} else {
				coeff = block[i];
				if (abs(coeff) > 127) {
					fprintf(stderr, "Coeff too large\n");
					return 1;
				}
				tmp = abs(coeff) | run << 12;
				sign = coeff < 0;
				int j;
				for (j = 0; tab[j].blen; j++) {
					if (tab[j].val == tmp) {
						break;
					}
				}
				if (!tab[j].val != tmp)
					tmp = 0xfffff;
				eb = coeff & 0xff;
				i++;
			}
		}
		if (vs_vlc(str, &tmp, tab)) return 1;
		if (tmp == 0) {
			/* end of block */
			if (str->dir == VS_DECODE) {
				while (i < 64)
					block[i++] = 0;
			}
			return 0;
		} else if (tmp == 0xfffff) {
			/* escape */
			if (vs_u(str, &run, 6)) return 1;
			if (vs_u(str, &eb, 8)) return 1;
			if (eb == 0 || eb == 0x80) {
				fprintf(stderr, "Invalid escape code\n");
				return 1;
			}
			coeff = eb;
			if (coeff > 0x80)
				coeff |= -0x100;
		} else {
			/* normal coefficient */
			if (vs_u(str, &sign, 1)) return 1;
			run = tmp >> 12;
			if (sign)
				coeff = -(tmp & 0xfff);
			else
				coeff = tmp & 0xfff;
		}
		if (str->dir == VS_DECODE) {
			if (i >= 64) {
				fprintf(stderr, "block overflow\n");
				return 1;
			}
			while (run--) {
				if (i >= 64) {
					fprintf(stderr, "block overflow\n");
					return 1;
				}
				block[i++] = 0;
			}
			block[i++] = coeff;
		}
	}
}

static const struct vs_vlc_val mba_vlc[] = {
	{  0,  1, 1 },
	{  1,  3, 0,1,1 },
	{  2,  3, 0,1,0 },
	{  3,  4, 0,0,1,1 },
	{  4,  4, 0,0,1,0 },
	{  5,  5, 0,0,0,1,1 },
	{  6,  5, 0,0,0,1,0 },
	{  7,  7, 0,0,0,0,1,1,1 },
	{  8,  7, 0,0,0,0,1,1,0 },
	{  9,  8, 0,0,0,0,1,0,1,1 },
	{ 10,  8, 0,0,0,0,1,0,1,0 },
	{ 11,  8, 0,0,0,0,1,0,0,1 },
	{ 12,  8, 0,0,0,0,1,0,0,0 },
	{ 13,  8, 0,0,0,0,0,1,1,1 },
	{ 14,  8, 0,0,0,0,0,1,1,0 },
	{ 15, 10, 0,0,0,0,0,1,0,1,1,1 },
	{ 16, 10, 0,0,0,0,0,1,0,1,1,0 },
	{ 17, 10, 0,0,0,0,0,1,0,1,0,1 },
	{ 18, 10, 0,0,0,0,0,1,0,1,0,0 },
	{ 19, 10, 0,0,0,0,0,1,0,0,1,1 },
	{ 20, 10, 0,0,0,0,0,1,0,0,1,0 },
	{ 21, 11, 0,0,0,0,0,1,0,0,0,1,1 },
	{ 22, 11, 0,0,0,0,0,1,0,0,0,1,0 },
	{ 23, 11, 0,0,0,0,0,1,0,0,0,0,1 },
	{ 24, 11, 0,0,0,0,0,1,0,0,0,0,0 },
	{ 25, 11, 0,0,0,0,0,0,1,1,1,1,1 },
	{ 26, 11, 0,0,0,0,0,0,1,1,1,1,0 },
	{ 27, 11, 0,0,0,0,0,0,1,1,1,0,1 },
	{ 28, 11, 0,0,0,0,0,0,1,1,1,0,0 },
	{ 29, 11, 0,0,0,0,0,0,1,1,0,1,1 },
	{ 30, 11, 0,0,0,0,0,0,1,1,0,1,0 },
	{ 31, 11, 0,0,0,0,0,0,1,1,0,0,1 },
	{ 32, 11, 0,0,0,0,0,0,1,1,0,0,0 },
	{ 33, 11, 0,0,0,0,0,0,0,1,1,1,1 },	/* stuffing */
	{ 34,  8, 0,0,0,0,0,0,0,0 },		/* end */
	{ 0 },
};

#define C H261_MTYPE_FLAG_CODED
#define I H261_MTYPE_FLAG_INTRA
#define Q H261_MTYPE_FLAG_QUANT
#define M H261_MTYPE_FLAG_MC
#define F H261_MTYPE_FLAG_FIL

static const struct vs_vlc_val mtype_vlc[] = {
	{ C      ,  1, 1 },
	{ C  |M|F,  2, 0,1 },
	{     M|F,  3, 0,0,1 },
	{ I      ,  4, 0,0,0,1 },
	{ C|Q    ,  5, 0,0,0,0,1 },
	{ C|Q|M|F,  6, 0,0,0,0,0,1 },
	{ I|Q    ,  7, 0,0,0,0,0,0,1 },
	{ C  |M  ,  8, 0,0,0,0,0,0,0,1 },
	{     M  ,  9, 0,0,0,0,0,0,0,0,1 },
	{ C|Q|M  , 10, 0,0,0,0,0,0,0,0,0,1 },
	{ 0 },
};

#undef C
#undef I
#undef Q
#undef M
#undef F

static const struct vs_vlc_val mvd_vlc[] = {
	{   0,  1, 1 },
	{   1,  3, 0,1,0 },
	{  -1,  3, 0,1,1 },
	{   2,  4, 0,0,1,0 },
	{  -2,  4, 0,0,1,1 },
	{   3,  5, 0,0,0,1,0 },
	{  -3,  5, 0,0,0,1,1 },
	{   4,  7, 0,0,0,0,1,1,0 },
	{  -4,  7, 0,0,0,0,1,1,1 },
	{   5,  8, 0,0,0,0,1,0,1,0 },
	{  -5,  8, 0,0,0,0,1,0,1,1 },
	{   6,  8, 0,0,0,0,1,0,0,0 },
	{  -6,  8, 0,0,0,0,1,0,0,1 },
	{   7,  8, 0,0,0,0,0,1,1,0 },
	{  -7,  8, 0,0,0,0,0,1,1,1 },
	{   8, 10, 0,0,0,0,0,1,0,1,1,0 },
	{  -8, 10, 0,0,0,0,0,1,0,1,1,1 },
	{   9, 10, 0,0,0,0,0,1,0,1,0,0 },
	{  -9, 10, 0,0,0,0,0,1,0,1,0,1 },
	{  10, 10, 0,0,0,0,0,1,0,0,1,0 },
	{ -10, 10, 0,0,0,0,0,1,0,0,1,1 },
	{  11, 11, 0,0,0,0,0,1,0,0,0,1,0 },
	{ -11, 11, 0,0,0,0,0,1,0,0,0,1,1 },
	{  12, 11, 0,0,0,0,0,1,0,0,0,0,0 },
	{ -12, 11, 0,0,0,0,0,1,0,0,0,0,1 },
	{  13, 11, 0,0,0,0,0,0,1,1,1,1,0 },
	{ -13, 11, 0,0,0,0,0,0,1,1,1,1,1 },
	{  14, 11, 0,0,0,0,0,0,1,1,1,0,0 },
	{ -14, 11, 0,0,0,0,0,0,1,1,1,0,1 },
	{  15, 11, 0,0,0,0,0,0,1,1,0,1,0 },
	{ -15, 11, 0,0,0,0,0,0,1,1,0,1,1 },
	{ -16, 11, 0,0,0,0,0,0,1,1,0,0,1 },
	{ 0 },
};

static const struct vs_vlc_val cbp_vlc[] = {
	{ 0x0f, 3, 1,1,1 },
	{ 0x08, 4, 1,1,0,1 },
	{ 0x04, 4, 1,1,0,0 },
	{ 0x02, 4, 1,0,1,1 },
	{ 0x01, 4, 1,0,1,0 },
	{ 0x0c, 5, 1,0,0,1,1 },
	{ 0x03, 5, 1,0,0,1,0 },
	{ 0x0a, 5, 1,0,0,0,1 },
	{ 0x05, 5, 1,0,0,0,0 },
	{ 0x0e, 5, 0,1,1,1,1 },
	{ 0x0d, 5, 0,1,1,1,0 },
	{ 0x0b, 5, 0,1,1,0,1 },
	{ 0x07, 5, 0,1,1,0,0 },
	{ 0x20, 5, 0,1,0,1,1 },
	{ 0x2f, 5, 0,1,0,1,0 },
	{ 0x10, 5, 0,1,0,0,1 },
	{ 0x1f, 5, 0,1,0,0,0 },
	{ 0x06, 6, 0,0,1,1,1,1 },
	{ 0x09, 6, 0,0,1,1,1,0 },
	{ 0x30, 6, 0,0,1,1,0,1 },
	{ 0x3f, 6, 0,0,1,1,0,0 },
	{ 0x28, 7, 0,0,1,0,1,1,1 },
	{ 0x24, 7, 0,0,1,0,1,1,0 },
	{ 0x22, 7, 0,0,1,0,1,0,1 },
	{ 0x21, 7, 0,0,1,0,1,0,0 },
	{ 0x18, 7, 0,0,1,0,0,1,1 },
	{ 0x14, 7, 0,0,1,0,0,1,0 },
	{ 0x12, 7, 0,0,1,0,0,0,1 },
	{ 0x11, 7, 0,0,1,0,0,0,0 },
	{ 0x38, 8, 0,0,0,1,1,1,1,1 },
	{ 0x34, 8, 0,0,0,1,1,1,1,0 },
	{ 0x32, 8, 0,0,0,1,1,1,0,1 },
	{ 0x31, 8, 0,0,0,1,1,1,0,0 },
	{ 0x2c, 8, 0,0,0,1,1,0,1,1 },
	{ 0x23, 8, 0,0,0,1,1,0,1,0 },
	{ 0x2a, 8, 0,0,0,1,1,0,0,1 },
	{ 0x25, 8, 0,0,0,1,1,0,0,0 },
	{ 0x1c, 8, 0,0,0,1,0,1,1,1 },
	{ 0x13, 8, 0,0,0,1,0,1,1,0 },
	{ 0x1a, 8, 0,0,0,1,0,1,0,1 },
	{ 0x15, 8, 0,0,0,1,0,1,0,0 },
	{ 0x3c, 8, 0,0,0,1,0,0,1,1 },
	{ 0x33, 8, 0,0,0,1,0,0,1,0 },
	{ 0x3a, 8, 0,0,0,1,0,0,0,1 },
	{ 0x35, 8, 0,0,0,1,0,0,0,0 },
	{ 0x26, 8, 0,0,0,0,1,1,1,1 },
	{ 0x29, 8, 0,0,0,0,1,1,1,0 },
	{ 0x16, 8, 0,0,0,0,1,1,0,1 },
	{ 0x19, 8, 0,0,0,0,1,1,0,0 },
	{ 0x2e, 8, 0,0,0,0,1,0,1,1 },
	{ 0x2d, 8, 0,0,0,0,1,0,1,0 },
	{ 0x2b, 8, 0,0,0,0,1,0,0,1 },
	{ 0x27, 8, 0,0,0,0,1,0,0,0 },
	{ 0x1e, 8, 0,0,0,0,0,1,1,1 },
	{ 0x1d, 8, 0,0,0,0,0,1,1,0 },
	{ 0x1b, 8, 0,0,0,0,0,1,0,1 },
	{ 0x17, 8, 0,0,0,0,0,1,0,0 },
	{ 0x3e, 9, 0,0,0,0,0,0,1,1,1 },
	{ 0x3d, 9, 0,0,0,0,0,0,1,1,0 },
	{ 0x3b, 9, 0,0,0,0,0,0,1,0,1 },
	{ 0x37, 9, 0,0,0,0,0,0,1,0,0 },
	{ 0x36, 9, 0,0,0,0,0,0,0,1,1 },
	{ 0x39, 9, 0,0,0,0,0,0,0,1,0 },
	{ 0 },
};

int h261_gob(struct bitstream *str, struct h261_gob *gob) {
	if (vs_u(str, &gob->gquant, 5)) return 1;
	uint32_t gei = 0;
	if (vs_u(str, &gei, 1)) return 1;
	while (gei) {
		if (vs_u(str, &gei, 8)) return 1;
		if (vs_u(str, &gei, 1)) return 1;
	}
	int mbi = 0;
	uint32_t quant = gob->gquant;
	while (1) {
		uint32_t mba = 0;
		if (str->dir == VS_ENCODE) {
			while (mbi < H261_GOB_MBS && gob->mbs[mbi].mtype == 0) {
				if (vs_infer(str, &gob->mbs[mbi].cbp, 0)) return 1;
				mbi++, mba++;
			}
		       	if (mbi == H261_GOB_MBS)
				return 0;
		}
		while (1) {
			if (vs_vlc(str, &mba, mba_vlc)) return 1;
			if (str->dir == VS_ENCODE || mba < 33)
				break;
			if (mba == 33) /* stuffing */
				continue;
			/* end */
			while (mbi < H261_GOB_MBS) {
				gob->mbs[mbi].mtype = 0;
				if (vs_infer(str, &gob->mbs[mbi].cbp, 0)) return 1;
				mbi++;
			}
			return 0;
		}
		if (str->dir == VS_DECODE) {
			while (mbi < H261_GOB_MBS && mba) {
				gob->mbs[mbi].mtype = 0;
				if (vs_infer(str, &gob->mbs[mbi].cbp, 0)) return 1;
				mbi++, mba--;
			}
			if (mbi == H261_GOB_MBS) {
				fprintf(stderr, "Macroblock address overrun\n");
				return 1;
			}
		}
		struct h261_macroblock *mb = &gob->mbs[mbi];
		if (vs_vlc(str, &mb->mtype, mtype_vlc)) return 1;
		if (mb->mtype & (H261_MTYPE_FLAG_CODED | H261_MTYPE_FLAG_INTRA)) {
			if (mb->mtype & H261_MTYPE_FLAG_QUANT) {
				if (vs_u(str, &mb->mquant, 5)) return 1;
				quant = mb->mquant;
			} else {
				if (vs_infer(str, &mb->mquant, quant)) return 1;
			}
		}
		if (mb->mtype & H261_MTYPE_FLAG_MC) {
			if (vs_vlc(str, &mb->mvd[0], mvd_vlc)) return 1;
			if (vs_vlc(str, &mb->mvd[1], mvd_vlc)) return 1;
		}
		if (mb->mtype & H261_MTYPE_FLAG_CODED) {
			if (vs_vlc(str, &mb->cbp, cbp_vlc)) return 1;
		} else if (mb->mtype & H261_MTYPE_FLAG_INTRA) {
			if (vs_infer(str, &mb->cbp, 0x3f)) return 1;
		} else {
			if (vs_infer(str, &mb->cbp, 0)) return 1;
		}
		int j, k;
		for (j = 0; j < 6; j++) {
			if (mb->cbp & (1 << j)) {
				if (h261_block(str, mb->block[j], mb->mtype & H261_MTYPE_FLAG_INTRA)) return 1;
			} else {
				for (k = 0; k < 64; k++)
					if (vs_infers(str, &mb->block[j][k], 0)) return 1;
			}
		}
		mbi++;
	}
	return 0;
}

void h261_print_picparm(struct h261_picparm *picparm) {
	printf("Picture header:\n");
	printf("\tTR = %d\n", picparm->tr);
	printf("\tPTYPE = 0x%02x [", picparm->ptype);
	if (picparm->ptype & 0x20)
		printf("SPLIT_SCREEN ");
	if (picparm->ptype & 0x10)
		printf("DOCUMENT_CAMERA ");
	if (picparm->ptype & 0x08)
		printf("FREEZE_RELEASE ");
	if (!(picparm->ptype & 0x04))
		printf("QCIF");
	else
		printf("CIF");
	if (!(picparm->ptype & 0x02))
		printf(" HI_RES");
	printf("]\n");
}

void h261_print_gob(struct h261_gob *gob) {
	int i;
	printf("Group of blocks:\n");
	printf("\tGN = %d\n", gob->gn);
	printf("\tGQUANT = %d\n", gob->gquant);
	int gx = (gob->gn & 1 ? 0 : H261_GOB_WIDTH);
	int gy = ((gob->gn - 1) >> 1) * H261_GOB_HEIGHT;
	for (i = 0; i < H261_GOB_MBS; i++) {
		int x = gx + i % H261_GOB_WIDTH;
		int y = gy + i / H261_GOB_WIDTH;
		printf("\tMacroblock %d (%d, %d):\n", i, x, y);
		printf("\t\tMTYPE =");
		struct h261_macroblock *mb = &gob->mbs[i];
		if (!mb->mtype) printf(" [SKIPPED]");
		if (mb->mtype & H261_MTYPE_FLAG_CODED) printf(" CODED");
		if (mb->mtype & H261_MTYPE_FLAG_INTRA) printf(" INTRA");
		if (mb->mtype & H261_MTYPE_FLAG_QUANT) printf(" QUANT");
		if (mb->mtype & H261_MTYPE_FLAG_MC) printf(" MC");
		if (mb->mtype & H261_MTYPE_FLAG_FIL) printf(" FIL");
		printf("\n");
		if (mb->mtype & (H261_MTYPE_FLAG_CODED | H261_MTYPE_FLAG_INTRA))
			printf("\t\tMQUANT = %d\n", mb->mquant);
		if (mb->mtype & H261_MTYPE_FLAG_MC) {
			printf("\t\tMVD = %d %d\n", mb->mvd[0], mb->mvd[1]);
		}
		printf("\t\tCBP = 0x%02x\n", mb->cbp);
		int j;
		for (j = 0; j < 6; j++) {
			if (mb->cbp & (1 << j)) {
				printf("\t\tBlock %d:", j);
				int k;
				for (k = 0; k < 64; k++)
					printf(" %d", mb->block[j][k]);
				printf("\n");
			}
		}
	}
}
