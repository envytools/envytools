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

#ifndef H261_H
#define H261_H

#include "vstream.h"

enum {
	H261_GOB_WIDTH = 11,
	H261_GOB_HEIGHT = 3,
	H261_GOB_MBS = H261_GOB_WIDTH * H261_GOB_HEIGHT,
};

enum {
	H261_MTYPE_FLAG_CODED = 1,
	H261_MTYPE_FLAG_INTRA = 2,
	H261_MTYPE_FLAG_QUANT = 4,
	H261_MTYPE_FLAG_MC = 8,
	H261_MTYPE_FLAG_FIL = 0x10,
};

struct h261_picparm {
	uint32_t tr;
	uint32_t ptype;
};

struct h261_macroblock {
	uint32_t mtype;
	uint32_t mquant;
	uint32_t mvd[2];
	uint32_t cbp;
	int32_t block[6][64];
};

struct h261_gob {
	uint32_t gn;
	uint32_t gquant;
	struct h261_macroblock mbs[H261_GOB_MBS];
};

int h261_picparm(struct bitstream *str, struct h261_picparm *picparm);
int h261_gob(struct bitstream *str, struct h261_gob *gob);
void h261_del_picparm(struct h261_picparm *picparm);
void h261_del_gob(struct h261_gob *gob);
void h261_print_picparm(struct h261_picparm *picparm);
void h261_print_gob(struct h261_gob *gob);

#endif
