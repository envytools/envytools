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

#ifndef NVHW_H
#define NVHW_H

#include <stdint.h>

enum pfb_type {
	PFB_NONE,
	PFB_NV01,
	PFB_NV03,
	PFB_NV10,
	PFB_NV20,
	PFB_NV40,
	PFB_NV41,
	PFB_NV44,
	PFB_NV50,
	PFB_NVC0,
};

struct mc_config {
	int mcbits;
	int parts;
	int partbits;
	int colbits;
	int colbits_lo;
	int bankbits_lo;
	int ranks;
	int rank_interleave;
	int rowbits[2];
	int bankbits[2];
	int burstbits;
	int partshift;
};

int pfb_type(int chipset);

int tile_pitch_valid(int chipset, uint32_t pitch, int *pshift, int *pfactor);
int has_large_tile(int chipset);
int tile_bankoff_bits(int chipset);
int has_vram_alt_tile(int chipset);
uint32_t tile_translate_addr(int chipset, uint32_t pitch, uint32_t address, int mode, int bankoff, const struct mc_config *mcc, int *ppart, int *ptag);

int is_igp(int chipset);
int is_g7x(int chipset);
int get_maxparts(int chipset);

enum comp_type {
	COMP_NONE,
	COMP_NV20,
	COMP_NV25,
	COMP_NV30,
	COMP_NV35,
	COMP_NV36,
	COMP_NV40,
};
int comp_type(int chipset);
int num_tile_regions(int chipset);
uint32_t tile_mmio_region(int chipset);
uint32_t tile_mmio_comp(int chipset);

enum comp_format_type {
	COMP_FORMAT_OFF,
	COMP_FORMAT_FLAT,
	COMP_FORMAT_Z16_GRAD,
	COMP_FORMAT_Z24S8_GRAD,
	COMP_FORMAT_A8R8G8B8_GRAD,
	COMP_FORMAT_A8R8G8B8_INTERP,
	COMP_FORMAT_Z24S8_SPLIT,
	COMP_FORMAT_Z24S8_SPLIT_GRAD,
};
int comp_format_type(int chipset, int format);
int comp_format_endian(int chipset, int format);
int comp_format_ms(int chipset, int format);
int comp_format_bpp(int chipset, int format);

void comp_decompress(int chipset, int format, uint8_t *data, int tag);

#endif
