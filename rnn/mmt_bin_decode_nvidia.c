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
	if (nv->subtype == 'c')
	{
		struct mmt_nvidia_create_object *create;
		create = mmt_load_data(sizeof(struct mmt_nvidia_create_object) + 1);
		create = mmt_load_data(sizeof(struct mmt_nvidia_create_object) + 1 + create->name.len);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*create) + create->name.len]);

		if (funcs->create_object)
			funcs->create_object(create, state);

		mmt_idx += sizeof(*create) + create->name.len + 1;
	}
	else if (nv->subtype == 'd')
	{
		struct mmt_nvidia_destroy_object *destroy = mmt_load_data(sizeof(struct mmt_nvidia_destroy_object) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*destroy)]);

		if (funcs->destroy_object)
			funcs->destroy_object(destroy, state);

		mmt_idx += sizeof(*destroy) + 1;
	}
	else if (nv->subtype == 'i')
	{
		struct mmt_nvidia_ioctl_pre *ctl;
		ctl = mmt_load_data(sizeof(struct mmt_nvidia_ioctl_pre) + 1);
		ctl = mmt_load_data(sizeof(struct mmt_nvidia_ioctl_pre) + 1 + ctl->data.len);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*ctl) + ctl->data.len]);

		if (funcs->ioctl_pre)
			funcs->ioctl_pre(ctl, state);

		mmt_idx += sizeof(*ctl) +  1 + ctl->data.len;
	}
	else if (nv->subtype == 'j')
	{
		struct mmt_nvidia_ioctl_post *ctl;
		ctl = mmt_load_data(sizeof(struct mmt_nvidia_ioctl_post) + 1);
		ctl = mmt_load_data(sizeof(struct mmt_nvidia_ioctl_post) + 1 + ctl->data.len);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*ctl) + ctl->data.len]);

		if (funcs->ioctl_post)
			funcs->ioctl_post(ctl, state);

		mmt_idx += sizeof(*ctl) +  1 + ctl->data.len;
	}
	else if (nv->subtype == 'o')
	{
		struct mmt_nvidia_memory_dump *d;
		d = mmt_load_data(sizeof(struct mmt_nvidia_memory_dump));
		d = mmt_load_data(sizeof(struct mmt_nvidia_memory_dump) + d->str.len);

		if (funcs->memory_dump)
			funcs->memory_dump(d, state);

		mmt_idx += sizeof(struct mmt_nvidia_memory_dump) + d->str.len;

		struct mmt_buf *b;
		b = mmt_load_data(4);
		b = mmt_load_data(b->len + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(struct mmt_buf) + b->len]);

		if (funcs->memory_dump_cont)
			funcs->memory_dump_cont(b, state);

		mmt_idx += sizeof(struct mmt_buf) + b->len + 1;
	}
	else if (nv->subtype == 'l')
	{
		struct mmt_nvidia_call_method *m = mmt_load_data(sizeof(struct mmt_nvidia_call_method) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*m)]);

		if (funcs->call_method)
			funcs->call_method(m, state);

		mmt_idx += sizeof(struct mmt_nvidia_call_method) + 1;
	}
	else if (nv->subtype == 'p')
	{
		struct mmt_nvidia_create_mapped_object *p = mmt_load_data(sizeof(struct mmt_nvidia_create_mapped_object) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*p)]);

		if (funcs->create_mapped)
			funcs->create_mapped(p, state);

		mmt_idx += sizeof(struct mmt_nvidia_create_mapped_object) + 1;
	}
	else if (nv->subtype == 't')
	{
		struct mmt_nvidia_create_dma_object *create = mmt_load_data(sizeof(struct mmt_nvidia_create_dma_object) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*create)]);

		if (funcs->create_dma_object)
			funcs->create_dma_object(create, state);

		mmt_idx += sizeof(struct mmt_nvidia_create_dma_object) + 1;
	}
	else if (nv->subtype == 'a')
	{
		struct mmt_nvidia_alloc_map *alloc = mmt_load_data(sizeof(struct mmt_nvidia_alloc_map) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*alloc)]);

		if (funcs->alloc_map)
			funcs->alloc_map(alloc, state);

		mmt_idx += sizeof(struct mmt_nvidia_alloc_map) + 1;
	}
	else if (nv->subtype == 'g')
	{
		struct mmt_nvidia_gpu_map *map = mmt_load_data(sizeof(struct mmt_nvidia_gpu_map) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*map)]);

		if (funcs->gpu_map)
			funcs->gpu_map(map, state);

		mmt_idx += sizeof(struct mmt_nvidia_gpu_map) + 1;
	}
	else if (nv->subtype == 'h')
	{
		struct mmt_nvidia_gpu_unmap *unmap = mmt_load_data(sizeof(struct mmt_nvidia_gpu_unmap) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*unmap)]);

		if (funcs->gpu_unmap)
			funcs->gpu_unmap(unmap, state);

		mmt_idx += sizeof(struct mmt_nvidia_gpu_unmap) + 1;
	}
	else if (nv->subtype == 'm')
	{
		struct mmt_nvidia_mmap *map = mmt_load_data(sizeof(struct mmt_nvidia_mmap) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*map)]);

		if (funcs->mmap)
			funcs->mmap(map, state);

		mmt_idx += sizeof(struct mmt_nvidia_mmap) + 1;
	}
	else if (nv->subtype == 'e')
	{
		struct mmt_nvidia_unmap *unmap = mmt_load_data(sizeof(struct mmt_nvidia_unmap) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*unmap)]);

		if (funcs->unmap)
			funcs->unmap(unmap, state);

		mmt_idx += sizeof(struct mmt_nvidia_unmap) + 1;
	}
	else if (nv->subtype == 'b')
	{
		struct mmt_nvidia_bind *bnd = mmt_load_data(sizeof(struct mmt_nvidia_bind) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*bnd)]);

		if (funcs->bind)
			funcs->bind(bnd, state);

		mmt_idx += sizeof(struct mmt_nvidia_bind) + 1;
	}
	else if (nv->subtype == 'r')
	{
		struct mmt_nvidia_create_driver_object *create = mmt_load_data(sizeof(struct mmt_nvidia_create_driver_object) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*create)]);

		if (funcs->create_driver_object)
			funcs->create_driver_object(create, state);

		mmt_idx += sizeof(struct mmt_nvidia_create_driver_object) + 1;
	}
	else if (nv->subtype == 'v')
	{
		struct mmt_nvidia_create_device_object *create = mmt_load_data(sizeof(struct mmt_nvidia_create_device_object) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*create)]);

		if (funcs->create_device_object)
			funcs->create_device_object(create, state);

		mmt_idx += sizeof(struct mmt_nvidia_create_device_object) + 1;
	}
	else if (nv->subtype == 'x')
	{
		struct mmt_nvidia_create_context_object *create = mmt_load_data(sizeof(struct mmt_nvidia_create_context_object) + 1);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*create)]);

		if (funcs->create_context_object)
			funcs->create_context_object(create, state);

		mmt_idx += sizeof(struct mmt_nvidia_create_context_object) + 1;
	}
	else if (nv->subtype == '1')
	{
		struct mmt_nvidia_call_method_data *call;
		call = mmt_load_data(sizeof(struct mmt_nvidia_call_method_data) + 1);
		call = mmt_load_data(sizeof(struct mmt_nvidia_call_method_data) + 1 + call->data.len);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*call)] + call->data.len);

		if (funcs->call_method_data)
			funcs->call_method_data(call, state);

		mmt_idx += sizeof(struct mmt_nvidia_call_method_data) + 1 + call->data.len;
	}
	else if (nv->subtype == '4')
	{
		struct mmt_nvidia_ioctl_4d *ctl;
		ctl = mmt_load_data(sizeof(struct mmt_nvidia_ioctl_4d) + 1);
		ctl = mmt_load_data(sizeof(struct mmt_nvidia_ioctl_4d) + 1 + ctl->str.len);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*ctl) + ctl->str.len]);

		if (funcs->ioctl_4d)
			funcs->ioctl_4d(ctl, state);

		mmt_idx += sizeof(struct mmt_nvidia_ioctl_4d) + 1 + ctl->str.len;
	}
	else if (nv->subtype == 'k')
	{
		struct mmt_nvidia_mmiotrace_mark *mark;
		mark = mmt_load_data(sizeof(struct mmt_nvidia_mmiotrace_mark) + 1);
		mark = mmt_load_data(sizeof(struct mmt_nvidia_mmiotrace_mark) + 1 + mark->str.len);

		mmt_check_eor(mmt_buf[mmt_idx + sizeof(*mark) + mark->str.len]);

		if (funcs->mmiotrace_mark)
			funcs->mmiotrace_mark(mark, state);

		mmt_idx += sizeof(struct mmt_nvidia_mmiotrace_mark) + 1 + mark->str.len;
	}
	else
	{
		fprintf(stderr, "ioctl\n");
		mmt_dump_next();
		abort();
	}
}
