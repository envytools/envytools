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

#include "mmt_bin_decode_nvidia.h"
#include <stdio.h>
#include <stdlib.h>

static int load_create_object(struct mmt_nvidia_create_object **create_, int pfx)
{
	int size = sizeof(struct mmt_nvidia_create_object) + 1;
	struct mmt_nvidia_create_object *create;
	create = mmt_load_data_with_prefix(size, pfx);
	size += create->name.len;
	create = mmt_load_data_with_prefix(size, pfx);

	mmt_check_eor(size + pfx);

	if (create_)
		*create_ = create;
	return size;
}

#define CREATE_LOADER(name, type) \
static int name(type **obj_, int pfx)                 \
{                                                     \
	int size = sizeof(type) + 1;                      \
	type *obj = mmt_load_data_with_prefix(size, pfx); \
                                                      \
	mmt_check_eor(size + pfx);                        \
                                                      \
	if (obj_)                                         \
		*obj_ = obj;                                  \
	return size;                                      \
}

CREATE_LOADER(load_destroy_object, struct mmt_nvidia_destroy_object)
CREATE_LOADER(load_call_method, struct mmt_nvidia_call_method)
CREATE_LOADER(load_create_mapped_object, struct mmt_nvidia_create_mapped_object)
CREATE_LOADER(load_create_dma_object, struct mmt_nvidia_create_dma_object)
CREATE_LOADER(load_alloc_map, struct mmt_nvidia_alloc_map)
CREATE_LOADER(load_gpu_map, struct mmt_nvidia_gpu_map)
CREATE_LOADER(load_gpu_map2, struct mmt_nvidia_gpu_map2)
CREATE_LOADER(load_gpu_unmap, struct mmt_nvidia_gpu_unmap)
CREATE_LOADER(load_gpu_unmap2, struct mmt_nvidia_gpu_unmap2)
CREATE_LOADER(load_mmap2, struct mmt_nvidia_mmap2)
CREATE_LOADER(load_mmap, struct mmt_nvidia_mmap)
CREATE_LOADER(load_unmap, struct mmt_nvidia_unmap)
CREATE_LOADER(load_bind, struct mmt_nvidia_bind)
CREATE_LOADER(load_create_driver_object, struct mmt_nvidia_create_driver_object)
CREATE_LOADER(load_create_device_object, struct mmt_nvidia_create_device_object)
CREATE_LOADER(load_create_context_object, struct mmt_nvidia_create_context_object)

static int load_memory_dump(int pfx, struct mmt_memory_dump_prefix **dump, struct mmt_buf **buf, struct mmt_nvidia_decode_funcs *funcs)
{
	int size1, size2, omitted = 0;
	struct mmt_memory_dump_prefix *d;
	struct mmt_buf *b;
	*dump = NULL;
	*buf = NULL;
	struct mmt_message_nv *nv;

	do
	{
		struct mmt_message *msg = mmt_load_data_with_prefix(1, pfx);
		if (msg == NULL || msg->type != 'n')
			return 0;

		mmt_load_data_with_prefix(sizeof(struct mmt_message_nv), pfx);

		nv = mmt_load_data_with_prefix(sizeof(struct mmt_message_nv), pfx);
		if (nv == NULL)
			return 0;

		if (nv->subtype != 'o')
		{
			int omit;
			if (nv->subtype == 'c' && funcs->create_object == NULL)
				omit = load_create_object(NULL, pfx);
			else if (nv->subtype == 'd' && funcs->destroy_object == NULL)
				omit = load_destroy_object(NULL, pfx);
			else if (nv->subtype == 'l' && funcs->call_method == NULL)
				omit = load_call_method(NULL, pfx);
			else if (nv->subtype == 'p' && funcs->create_mapped == NULL)
				omit = load_create_mapped_object(NULL, pfx);
			else if (nv->subtype == 't' && funcs->create_dma_object == NULL)
				omit = load_create_dma_object(NULL, pfx);
			else if (nv->subtype == 'a' && funcs->alloc_map == NULL)
				omit = load_alloc_map(NULL, pfx);
			else if (nv->subtype == 'g' && funcs->gpu_map == NULL)
				omit = load_gpu_map(NULL, pfx);
			else if (nv->subtype == 'G' && funcs->gpu_map2 == NULL)
				omit = load_gpu_map2(NULL, pfx);
			else if (nv->subtype == 'h' && funcs->gpu_unmap == NULL)
				omit = load_gpu_unmap(NULL, pfx);
			else if (nv->subtype == 'H' && funcs->gpu_unmap2 == NULL)
				omit = load_gpu_unmap2(NULL, pfx);
			else if (nv->subtype == 'm' /*&& funcs->mmap == NULL*/)
				omit = load_mmap(NULL, pfx);
			else if (nv->subtype == 'e' && funcs->unmap == NULL)
				omit = load_unmap(NULL, pfx);
			else if (nv->subtype == 'b' && funcs->bind == NULL)
				omit = load_bind(NULL, pfx);
			else if (nv->subtype == 'r' && funcs->create_driver_object == NULL)
				omit = load_create_driver_object(NULL, pfx);
			else if (nv->subtype == 'v' && funcs->create_device_object == NULL)
				omit = load_create_device_object(NULL, pfx);
			else if (nv->subtype == 'x' && funcs->create_context_object == NULL)
				omit = load_create_context_object(NULL, pfx);
			else
			{
				//if (nv->subtype != 'j' && nv->subtype != 'i')
				//	printf("%d '%c'\n", nv->subtype, nv->subtype);
				return 0;
			}

			omitted += omit;
			pfx += omit;
		}
	}
	while (nv->subtype != 'o');

	size1 = sizeof(struct mmt_memory_dump_prefix);
	d = mmt_load_data_with_prefix(size1, pfx);
	size1 += d->str.len;
	d = mmt_load_data_with_prefix(size1, pfx);

	size2 = 4;
	b = mmt_load_data_with_prefix(size2, size1 + pfx);
	size2 += b->len + 1;
	b = mmt_load_data_with_prefix(size2, size1 + pfx);

	mmt_check_eor(size2 + size1 + pfx);

	*dump = d;
	*buf = b;

	return size1 + size2 + omitted;
}

#define GENERATE_HANDLER(subtype_, type, loader, func) \
		else if (nv->subtype == subtype_)     \
		{                                     \
			type *obj;                        \
			size = loader(&obj, 0);           \
                                              \
			if (funcs->func)                  \
				funcs->func(obj, state);      \
                                              \
			mmt_idx += size;                  \
		}

#define MAX_ARGS 10
void mmt_decode_nvidia(struct mmt_nvidia_decode_funcs *funcs, void *state)
{
	struct mmt_message_nv *nv = mmt_load_data(sizeof(struct mmt_message_nv));
	int size;

	if (nv->subtype == 'i')
	{
		int size2, pfx;
		struct mmt_memory_dump args[MAX_ARGS];
		struct mmt_ioctl_pre *ctl;
		int argc;

		do
		{
			size = sizeof(struct mmt_ioctl_pre) + 1;
			ctl = mmt_load_data(size);
			size += ctl->data.len;
			ctl = mmt_load_data(size);

			mmt_check_eor(size);

			argc = 0;

			struct mmt_memory_dump_prefix *d;
			struct mmt_buf *b;
			pfx = size;

			while ((size2 = load_memory_dump(pfx, &d, &b, funcs)))
			{
				args[argc].addr = d->addr;
				args[argc].str = &d->str;
				args[argc].data = b;
				argc++;
				pfx += size2;
				if (argc == MAX_ARGS)
					break;
			}
		}
		while (ctl != mmt_load_data(size));

		if (funcs->ioctl_pre)
			funcs->ioctl_pre(ctl, state, args, argc);

		mmt_idx += pfx;
	}
	else if (nv->subtype == 'j')
	{
		int size2, pfx;
		struct mmt_memory_dump args[MAX_ARGS];
		struct mmt_ioctl_post *ctl;
		int argc;

		do
		{
			size = sizeof(struct mmt_ioctl_post) + 1;
			ctl = mmt_load_data(size);
			size += ctl->data.len;
			ctl = mmt_load_data(size);

			mmt_check_eor(size);

			argc = 0;

			struct mmt_memory_dump_prefix *d;
			struct mmt_buf *b;
			pfx = size;

			while ((size2 = load_memory_dump(pfx, &d, &b, funcs)))
			{
				args[argc].addr = d->addr;
				args[argc].str = &d->str;
				args[argc].data = b;
				argc++;
				pfx += size2;
				if (argc == MAX_ARGS)
					break;
			}
		}
		while (ctl != mmt_load_data(size));

		if (funcs->ioctl_post)
			funcs->ioctl_post(ctl, state, args, argc);

		mmt_idx += pfx;
	}
	else if (nv->subtype == 'o')
	{
		struct mmt_memory_dump_prefix *d;
		struct mmt_buf *b;

		size = load_memory_dump(0, &d, &b, funcs);

		if (funcs->memory_dump)
			funcs->memory_dump(d, b, state);

		mmt_idx += size;
	}
	GENERATE_HANDLER('c', struct mmt_nvidia_create_object,         load_create_object,         create_object)
	GENERATE_HANDLER('d', struct mmt_nvidia_destroy_object,        load_destroy_object,        destroy_object)
	GENERATE_HANDLER('l', struct mmt_nvidia_call_method,           load_call_method,           call_method)
	GENERATE_HANDLER('p', struct mmt_nvidia_create_mapped_object,  load_create_mapped_object,  create_mapped)
	GENERATE_HANDLER('t', struct mmt_nvidia_create_dma_object,     load_create_dma_object,     create_dma_object)
	GENERATE_HANDLER('a', struct mmt_nvidia_alloc_map,             load_alloc_map,             alloc_map)
	GENERATE_HANDLER('g', struct mmt_nvidia_gpu_map,               load_gpu_map,               gpu_map)
	GENERATE_HANDLER('G', struct mmt_nvidia_gpu_map2,              load_gpu_map2,              gpu_map2)
	GENERATE_HANDLER('h', struct mmt_nvidia_gpu_unmap,             load_gpu_unmap,             gpu_unmap)
	GENERATE_HANDLER('H', struct mmt_nvidia_gpu_unmap2,            load_gpu_unmap2,            gpu_unmap2)
	GENERATE_HANDLER('M', struct mmt_nvidia_mmap2,                 load_mmap2,                 mmap2)
	GENERATE_HANDLER('m', struct mmt_nvidia_mmap,                  load_mmap,                  mmap)
	GENERATE_HANDLER('b', struct mmt_nvidia_bind,                  load_bind,                  bind)
	GENERATE_HANDLER('e', struct mmt_nvidia_unmap,                 load_unmap,                 unmap)
	GENERATE_HANDLER('r', struct mmt_nvidia_create_driver_object,  load_create_driver_object,  create_driver_object)
	GENERATE_HANDLER('v', struct mmt_nvidia_create_device_object,  load_create_device_object,  create_device_object)
	GENERATE_HANDLER('x', struct mmt_nvidia_create_context_object, load_create_context_object, create_context_object)
	else if (nv->subtype == '1')
	{
		struct mmt_nvidia_call_method_data *call;
		size = sizeof(struct mmt_nvidia_call_method_data) + 1;
		call = mmt_load_data(size);
		size += call->data.len;
		call = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->call_method_data)
			funcs->call_method_data(call, state);

		mmt_idx += size;
	}
	else if (nv->subtype == '4')
	{
		struct mmt_nvidia_ioctl_4d *ctl;
		size = sizeof(struct mmt_nvidia_ioctl_4d) + 1;
		ctl = mmt_load_data(size);
		size += ctl->str.len;
		ctl = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->ioctl_4d)
			funcs->ioctl_4d(ctl, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'k')
	{
		struct mmt_nvidia_mmiotrace_mark *mark;
		size = sizeof(struct mmt_nvidia_mmiotrace_mark) + 1;
		mark = mmt_load_data(size);
		size += mark->str.len;
		mark = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->mmiotrace_mark)
			funcs->mmiotrace_mark(mark, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'P')
	{
		struct mmt_nouveau_pushbuf_data *data;
		size = sizeof(struct mmt_nouveau_pushbuf_data) + 1;
		data = mmt_load_data(size);
		size += data->data.len;
		data = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->nouveau_gem_pushbuf_data)
			funcs->nouveau_gem_pushbuf_data(data, state);

		mmt_idx += size;
	}
	else
	{
		fprintf(stderr, "ioctl\n");
		mmt_dump_next();
		abort();
	}
}
