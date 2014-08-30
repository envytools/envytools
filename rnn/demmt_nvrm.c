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

#include "demmt.h"
#include "demmt_pushbuf.h"
#include "demmt_nvrm.h"

int demmt_nv_ioctl_pre(uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *buf, void *state)
{
	return 0;
}

int demmt_nv_ioctl_post(uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *buf, void *state)
{
	return 0;
}

void demmt_nv_create_object(struct mmt_nvidia_create_object *create, void *state)
{
	mmt_log("create object: obj1: 0x%08x, obj2: 0x%08x, class: 0x%08x\n", create->obj1, create->obj2, create->class);
	pushbuf_add_object(create->obj2, create->class);
}

void demmt_nv_destroy_object(struct mmt_nvidia_destroy_object *destroy, void *state)
{
	mmt_log("destroy object: obj1: 0x%08x, obj2: 0x%08x\n", destroy->obj1, destroy->obj2);
}

void demmt_nv_memory_dump(struct mmt_nvidia_memory_dump *d, void *state)
{
}

void demmt_nv_memory_dump_cont(struct mmt_buf *b, void *state)
{
}

void demmt_nv_call_method(struct mmt_nvidia_call_method *m, void *state)
{
	mmt_log("call method: data1: 0x%08x, data2: 0x%08x\n", m->data1, m->data2);
}

void demmt_nv_create_mapped(struct mmt_nvidia_create_mapped_object *p, void *state)
{
	mmt_log("create mapped object: mmap_offset: %p, data1: 0x%08x, data2: 0x%08x, type: 0x%08x\n",
			(void *)p->mmap_offset, p->data1, p->data2, p->type);

	if (p->mmap_offset == 0)
		return;

	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->mmap_offset == p->mmap_offset)
		{
			buf->data1 = p->data1;
			buf->data2 = p->data2;
			return;
		}

	for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
	{
		if (buf->data1 == p->data1 && buf->data2 == p->data2)
		{
			mmt_log("TODO: gpu only buffer found, demmt_nv_create_mapped needs to be updated!%s\n", "");
			break;
		}
	}

	struct unk_map *m = malloc(sizeof(struct unk_map));
	m->data1 = p->data1;
	m->data2 = p->data2;
	m->mmap_offset = p->mmap_offset;
	m->next = unk_maps;
	unk_maps = m;
}

void demmt_nv_create_dma_object(struct mmt_nvidia_create_dma_object *create, void *state)
{
	mmt_log("create dma object, name: 0x%08x, type: 0x%08x, parent: 0x%08x\n",
			create->name, create->type, create->parent);
}

void demmt_nv_alloc_map(struct mmt_nvidia_alloc_map *alloc, void *state)
{
	mmt_log("allocate map: mmap_offset: %p, data1: 0x%08x, data2: 0x%08x\n",
			(void *)alloc->mmap_offset, alloc->data1, alloc->data2);

	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->mmap_offset == alloc->mmap_offset)
		{
			buf->data1 = alloc->data1;
			buf->data2 = alloc->data2;
			return;
		}

	for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
	{
		if (buf->data1 == alloc->data1 && buf->data2 == alloc->data2)
		{
			mmt_log("gpu only buffer found, merging%s\n", "");
			buf->mmap_offset = alloc->mmap_offset;
			return;
		}
	}

	struct unk_map *m = malloc(sizeof(struct unk_map));
	m->data1 = alloc->data1;
	m->data2 = alloc->data2;
	m->mmap_offset = alloc->mmap_offset;
	m->next = unk_maps;
	unk_maps = m;
}

static void demmt_nv_gpu_map(uint32_t data1, uint32_t data2, uint32_t data3, uint64_t gpu_start, uint32_t len, void *state)
{
	mmt_log("gpu map: data1: 0x%08x, data2: 0x%08x, data3: 0x%08x, gpu_start: 0x%08lx, len: 0x%08x\n",
			data1, data2, data3, gpu_start, len);
	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->data1 == data1 && buf->data2 == data3 && buf->length == len)
		{
			buf->gpu_start = gpu_start;
			mmt_log("setting gpu address for buffer %d to 0x%08lx\n", buf->id, buf->gpu_start);
			return;
		}

	struct unk_map *tmp;
	for (tmp = unk_maps; tmp != NULL; tmp = tmp->next)
	{
		if (tmp->data1 == data1 && tmp->data2 == data3)
		{
			mmt_log("TODO: unk buffer found, demmt_nv_gpu_map needs to be updated!%s\n", "");
			break;
		}
	}

	register_gpu_only_buffer(gpu_start, len, 0, data1, data3);
}

void demmt_nv_gpu_map1(struct mmt_nvidia_gpu_map *map, void *state)
{
	demmt_nv_gpu_map(map->data1, map->data2, map->data3, map->gpu_start, map->len, state);
}

void demmt_nv_gpu_map2(struct mmt_nvidia_gpu_map2 *map, void *state)
{
	demmt_nv_gpu_map(map->data1, map->data2, map->data3, map->gpu_start, map->len, state);
}

static void demmt_nv_gpu_unmap(uint32_t data1, uint32_t data2, uint32_t data3, uint64_t gpu_start, void *state)
{
	mmt_log("gpu unmap: data1: 0x%08x, data2: 0x%08x, data3: 0x%08x, gpu_start: 0x%08lx\n",
			data1, data2, data3, gpu_start);
	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->data1 == data1 && buf->data2 == data3 &&
				buf->gpu_start == gpu_start)
		{
			mmt_log("clearing gpu address for buffer %d (was: 0x%08lx)\n", buf->id, buf->gpu_start);
			buf->gpu_start = 0;
			return;
		}

	for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
	{
		if (buf->data1 == data1 && buf->data2 == data3 && buf->gpu_start == gpu_start)
		{
			mmt_log("deregistering gpu only buffer of size %ld\n", buf->length);
			buffer_free(buf);
			return;
		}
	}

	mmt_log("gpu only buffer not found%s\n", "");
}

void demmt_nv_gpu_unmap1(struct mmt_nvidia_gpu_unmap *unmap, void *state)
{
	demmt_nv_gpu_unmap(unmap->data1, unmap->data2, unmap->data3, unmap->gpu_start, state);
}

void demmt_nv_gpu_unmap2(struct mmt_nvidia_gpu_unmap2 *unmap, void *state)
{
	demmt_nv_gpu_unmap(unmap->data1, unmap->data2, unmap->data3, unmap->gpu_start, state);
}

void demmt_nv_mmap(struct mmt_nvidia_mmap *mm, void *state)
{
	mmt_log("mmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx\n",
			(void *)mm->start, mm->len, mm->id, mm->offset, mm->data1, mm->data2);
	struct buffer *buf;

	for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
	{
		if (buf->data1 == mm->data1 && buf->data2 == mm->data2)
		{
			mmt_log("gpu only buffer found, merging%s\n","");
			buffer_remove(buf);
			break;
		}
	}

	if (!buf)
	{
		buf = calloc(1, sizeof(struct buffer));
		buf->type = PUSH;
	}

	buf->id = mm->id;

	if (!buf->data)
		buf->data = calloc(mm->len, 1);

	buf->cpu_start = mm->start;

	if (buf->length && buf->length != mm->len)
		mmt_log("different length of gpu only buffer 0x%lx != 0x%lx\n", buf->length, mm->len);
	buf->length = mm->len;

	if (buf->mmap_offset && buf->mmap_offset != mm->offset)
		mmt_log("different mmap offset of gpu only buffer 0x%lx != 0x%lx\n", buf->mmap_offset, mm->offset);
	buf->mmap_offset = mm->offset;

	buf->data1 = mm->data1;
	buf->data2 = mm->data2;

	if (mm->id == pb_pointer_buffer)
	{
		if (ib_supported)
		{
			buf->type = IB;
			if (pb_pointer_offset)
			{
				buf->type |= PUSH;
				buf->ib_offset = pb_pointer_offset;
			}
		}
		else
			buf->type = USER;
	}
	else
		buf->type = PUSH;

	if (buffers_list)
		buffers_list->prev = buf;
	buf->next = buffers_list;

	buffers[buf->id] = buf;
	buffers_list = buf;
}

void demmt_nv_unmap(struct mmt_nvidia_unmap *mm, void *state)
{
	mmt_log("nv_munmap: mmap_offset: %p, data1: 0x%08x, data2: 0x%08x\n",
			(void *)mm->mmap_offset, mm->data1, mm->data2);

	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->mmap_offset == mm->mmap_offset)
		{
			// clear cpu_start only?
			buffer_free(buf);
			return;
		}

	mmt_log("couldn't find buffer to free%s\n", ""); // find by data1/data2?
}

void demmt_nv_bind(struct mmt_nvidia_bind *bnd, void *state)
{
	mmt_log("bind: data1: 0x%08x, data2: 0x%08x\n", bnd->data1, bnd->data2);
}

void demmt_nv_create_driver_object(struct mmt_nvidia_create_driver_object *create, void *state)
{
	mmt_log("create driver object: obj1: 0x%08x, obj2: 0x%08x, addr: 0x%08lx\n", create->obj1, create->obj2, create->addr);
}

void demmt_nv_create_device_object(struct mmt_nvidia_create_device_object *create, void *state)
{
	mmt_log("create device object: obj1: 0x%08x\n", create->obj1);
}

void demmt_nv_create_context_object(struct mmt_nvidia_create_context_object *create, void *state)
{
	mmt_log("create context object: obj1: 0x%08x\n", create->obj1);
}

void demmt_nv_call_method_data(struct mmt_nvidia_call_method_data *call, void *state)
{
}

void demmt_nv_ioctl_4d(struct mmt_nvidia_ioctl_4d *ctl, void *state)
{
	mmt_log("ioctl4d: %s\n", ctl->str.data);
}

void demmt_nv_mmiotrace_mark(struct mmt_nvidia_mmiotrace_mark *mark, void *state)
{
}
