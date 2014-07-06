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

void mmt_decode_nvidia(struct mmt_nvidia_decode_funcs *funcs, void *state)
{
	struct mmt_message_nv *nv = mmt_load_data(sizeof(struct mmt_message_nv));
	int size;
	if (nv->subtype == 'c')
	{
		size = sizeof(struct mmt_nvidia_create_object) + 1;
		struct mmt_nvidia_create_object *create;
		create = mmt_load_data(size);
		size += create->name.len;
		create = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->create_object)
			funcs->create_object(create, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'd')
	{
		size = sizeof(struct mmt_nvidia_destroy_object) + 1;
		struct mmt_nvidia_destroy_object *destroy = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->destroy_object)
			funcs->destroy_object(destroy, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'i')
	{
		size = sizeof(struct mmt_nvidia_ioctl_pre) + 1;
		struct mmt_nvidia_ioctl_pre *ctl;
		ctl = mmt_load_data(size);
		size += ctl->data.len;
		ctl = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->ioctl_pre)
			funcs->ioctl_pre(ctl, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'j')
	{
		size = sizeof(struct mmt_nvidia_ioctl_post) + 1;
		struct mmt_nvidia_ioctl_post *ctl;
		ctl = mmt_load_data(size);
		size += ctl->data.len;
		ctl = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->ioctl_post)
			funcs->ioctl_post(ctl, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'o')
	{
		size = sizeof(struct mmt_nvidia_memory_dump);
		struct mmt_nvidia_memory_dump *d;
		d = mmt_load_data(size);
		size += d->str.len;
		d = mmt_load_data(size);

		if (funcs->memory_dump)
			funcs->memory_dump(d, state);

		mmt_idx += size;

		struct mmt_buf *b;
		size = 4;
		b = mmt_load_data(size);
		size += b->len + 1;
		b = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->memory_dump_cont)
			funcs->memory_dump_cont(b, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'l')
	{
		size = sizeof(struct mmt_nvidia_call_method) + 1;
		struct mmt_nvidia_call_method *m = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->call_method)
			funcs->call_method(m, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'p')
	{
		size = sizeof(struct mmt_nvidia_create_mapped_object) + 1;
		struct mmt_nvidia_create_mapped_object *p = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->create_mapped)
			funcs->create_mapped(p, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 't')
	{
		size = sizeof(struct mmt_nvidia_create_dma_object) + 1;
		struct mmt_nvidia_create_dma_object *create = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->create_dma_object)
			funcs->create_dma_object(create, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'a')
	{
		size = sizeof(struct mmt_nvidia_alloc_map) + 1;
		struct mmt_nvidia_alloc_map *alloc = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->alloc_map)
			funcs->alloc_map(alloc, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'g')
	{
		size = sizeof(struct mmt_nvidia_gpu_map) + 1;
		struct mmt_nvidia_gpu_map *map = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->gpu_map)
			funcs->gpu_map(map, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'G')
	{
		size = sizeof(struct mmt_nvidia_gpu_map2) + 1;
		struct mmt_nvidia_gpu_map2 *map = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->gpu_map2)
			funcs->gpu_map2(map, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'h')
	{
		size = sizeof(struct mmt_nvidia_gpu_unmap) + 1;
		struct mmt_nvidia_gpu_unmap *unmap = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->gpu_unmap)
			funcs->gpu_unmap(unmap, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'H')
	{
		size = sizeof(struct mmt_nvidia_gpu_unmap2) + 1;
		struct mmt_nvidia_gpu_unmap2 *unmap = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->gpu_unmap2)
			funcs->gpu_unmap2(unmap, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'm')
	{
		size = sizeof(struct mmt_nvidia_mmap) + 1;
		struct mmt_nvidia_mmap *map = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->mmap)
			funcs->mmap(map, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'e')
	{
		size = sizeof(struct mmt_nvidia_unmap) + 1;
		struct mmt_nvidia_unmap *unmap = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->unmap)
			funcs->unmap(unmap, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'b')
	{
		size = sizeof(struct mmt_nvidia_bind) + 1;
		struct mmt_nvidia_bind *bnd = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->bind)
			funcs->bind(bnd, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'r')
	{
		size = sizeof(struct mmt_nvidia_create_driver_object) + 1;
		struct mmt_nvidia_create_driver_object *create = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->create_driver_object)
			funcs->create_driver_object(create, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'v')
	{
		size = sizeof(struct mmt_nvidia_create_device_object) + 1;
		struct mmt_nvidia_create_device_object *create = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->create_device_object)
			funcs->create_device_object(create, state);

		mmt_idx += size;
	}
	else if (nv->subtype == 'x')
	{
		size = sizeof(struct mmt_nvidia_create_context_object) + 1;
		struct mmt_nvidia_create_context_object *create = mmt_load_data(size);

		mmt_check_eor(size);

		if (funcs->create_context_object)
			funcs->create_context_object(create, state);

		mmt_idx += size;
	}
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
	else
	{
		fprintf(stderr, "ioctl\n");
		mmt_dump_next();
		abort();
	}
}
