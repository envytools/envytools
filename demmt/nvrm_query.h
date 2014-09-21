/*
 * Copyright (C) 2013 Marcin Kościelnicki <koriakin@0x04.net>
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

#ifndef NVRM_QUERY_H
#define NVRM_QUERY_H

#include <inttypes.h>

/* only seen on device so far */

/* XXX reads PBFB.CFG1 on NVCF */
struct nvrm_query_gpu_params {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t compressible_vram_size;
	uint32_t unk18;
	uint32_t unk1c;
	uint32_t unk20;
	uint32_t nv50_gpu_units;
	uint32_t unk28;
	uint32_t unk2c;
};
#define NVRM_QUERY_GPU_PARAMS 0x00000125

struct nvrm_query_object_classes {
	uint32_t cnt;
	uint32_t _pad;
	uint64_t ptr;
};
#define NVRM_QUERY_OBJECT_CLASSES 0x0000014c

struct nvrm_query_unk019a {
	uint32_t unk00;
};
#define NVRM_QUERY_UNK019A 0x0000019a

#endif
