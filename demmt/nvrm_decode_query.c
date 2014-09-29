/*
 * Copyright (C) 2014 Marcin Ślusarz <marcin.slusarz@gmail.com>.
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

#include "nvrm_decode.h"
#include "nvrm_query.h"
#include "util.h"

static void decode_nvrm_query_gpu_params(struct nvrm_query_gpu_params *q)
{
	print_u32(q, unk00);
	print_u32(q, unk04);
	print_u32(q, unk08);
	print_u32(q, unk0c);
	print_u32(q, unk10);
	print_u32(q, compressible_vram_size);
	print_u32(q, unk18);
	print_u32(q, unk1c);
	print_u32(q, unk20);
	print_u32(q, nv50_gpu_units);
	print_u32(q, unk28);
	print_u32(q, unk2c);
	print_ln();
}

static void decode_nvrm_query_object_classes(struct nvrm_query_object_classes *q, struct mmt_memory_dump *args, int argc)
{
	print_u32(q, cnt);
	print_pad_u32(q, _pad);
	struct mmt_buf *data = print_ptr(q, ptr, args, argc);
	print_ln();
	if (data)
		dump_mmt_buf_as_words_desc(data, nvrm_get_class_name);
}

static void decode_nvrm_query_unk019a(struct nvrm_query_unk019a *q)
{
	print_u32(q, unk00);
	print_ln();
}

#define _(MTHD, STR, FUN) { MTHD, #MTHD , sizeof(STR), FUN, NULL }
#define _a(MTHD, STR, FUN) { MTHD, #MTHD , sizeof(STR), NULL, FUN }

static struct
{
	uint32_t query;
	const char *name;
	size_t argsize;
	void *fun;
	void *fun_with_args;
} queries[] =
{
	_(NVRM_QUERY_GPU_PARAMS, struct nvrm_query_gpu_params, decode_nvrm_query_gpu_params),
	_a(NVRM_QUERY_OBJECT_CLASSES, struct nvrm_query_object_classes, decode_nvrm_query_object_classes),
	_(NVRM_QUERY_UNK019A, struct nvrm_query_unk019a, decode_nvrm_query_unk019a),
};
#undef _
#undef _a

void decode_nvrm_ioctl_query(struct nvrm_ioctl_query *s, struct mmt_memory_dump *args, int argc)
{
	print_cid(s, cid);
	print_handle(s, handle, cid);
	print_u32(s, query);
	print_u32(s, size);
	struct mmt_buf *data = print_ptr(s, ptr, args, argc);
	print_status(s, status);
	print_pad_u32(s, _pad);
	print_ln();

	if (data)
	{
		int k, found = 0;
		void (*fun)(void *) = NULL;
		void (*fun_with_args)(void *, struct mmt_memory_dump *, int argc) = NULL;

		for (k = 0; k < ARRAY_SIZE(queries); ++k)
			if (queries[k].query == s->query)
			{
				if (queries[k].argsize == data->len)
				{
					mmt_log("    %s: ", queries[k].name);
					nvrm_pfx = "";
					fun = queries[k].fun;
					if (fun)
						fun(data->data);
					fun_with_args = queries[k].fun_with_args;
					if (fun_with_args)
						fun_with_args(data->data, args, argc);
					found = 1;
				}
				break;
			}

		if (!found)
			dump_mmt_buf_as_words_horiz(data, "ptr[]:");
	}
}

