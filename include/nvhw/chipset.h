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

#ifndef NVHW_H
#define NVHW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

struct chipset_info {
	uint32_t pmc_id;
	int chipset;
	int card_type;
	int is_nv03t;
	int endian;
	const char *name;
};

int parse_pmc_id(uint32_t pmc_id, struct chipset_info *info);

int is_igp(int chipset);
int is_g7x(int chipset);
int pfb_type(int chipset);
int get_maxparts(int chipset);

#ifdef __cplusplus
}
#endif

#endif
