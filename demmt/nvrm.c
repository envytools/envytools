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

#include <string.h>
#include "buffer.h"
#include "log.h"
#include "nvrm.h"
#include "nvrm_decode.h"
#include "nvrm_ioctl.h"
#include "nvrm_mthd.h"

#define NVRM_OBJECT_CACHE_SIZE (512 + 1)
static struct nvrm_object *nvrm_object_cache[NVRM_OBJECT_CACHE_SIZE];
static void nvrm_add_object(uint32_t cid, uint32_t parent, uint32_t handle, uint32_t class_)
{
	int bucket = (handle * (handle + 3)) % NVRM_OBJECT_CACHE_SIZE;

	struct nvrm_object *entry = nvrm_object_cache[bucket];
	while (entry && (entry->cid != cid || entry->handle != handle))
		entry = entry->next;

	if (entry)
	{
		mmt_error("object 0x%08x / 0x%08x already exists!\n", cid, handle);
		if (entry->class_ != class_ || entry->parent != parent)
		{
			mmt_error("... and its class or parent differ! 0x%04x != 0x%04x || 0x%08x != 0x%08x\n",
					entry->class_, class_, entry->parent, parent);
			fflush(stdout);
			abort();
		}
	}
	else
	{
		if (0 && nvrm_object_cache[bucket])
			mmt_error("collision%s\n", "");

		entry = malloc(sizeof(struct nvrm_object));
		entry->cid = cid;
		entry->parent = parent;
		entry->handle = handle;
		entry->class_ = class_;
		entry->next = nvrm_object_cache[bucket];
		nvrm_object_cache[bucket] = entry;
	}
}

static void nvrm_del_object(uint32_t cid, uint32_t parent, uint32_t handle)
{
	int bucket = (handle * (handle + 3)) % NVRM_OBJECT_CACHE_SIZE;

	struct nvrm_object *entry = nvrm_object_cache[bucket];
	struct nvrm_object *preventry = NULL;
	while (entry && (entry->cid != cid || entry->handle != handle))
	{
		preventry = entry;
		entry = entry->next;
	}

	if (!entry)
	{
		mmt_error("trying to delete object 0x%08x / 0x%08x which does not exist!\n", cid, handle);
		return;
	}

	if (preventry)
		preventry->next = entry->next;
	else
		nvrm_object_cache[bucket] = entry->next;

	free(entry);
}

struct nvrm_object *nvrm_get_object(uint32_t cid, uint32_t handle)
{
	int bucket = (handle * (handle + 3)) % NVRM_OBJECT_CACHE_SIZE;

	struct nvrm_object *entry = nvrm_object_cache[bucket];
	while (entry && (entry->cid != cid || entry->handle != handle))
		entry = entry->next;

	return entry;
}

struct mmt_buf *find_ptr(uint64_t ptr, struct mmt_memory_dump *args, int argc)
{
	int i;
	for (i = 0; i < argc; ++i)
		if (args[i].addr == ptr)
			return args[i].data;

	return NULL;
}

static void handle_nvrm_ioctl_create(struct nvrm_ioctl_create *s, struct mmt_memory_dump *args, int argc)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;

	uint32_t cid = s->cid;
	uint32_t parent = s->parent;
	uint32_t handle = s->handle;
	if (handle == 0)
	{
		struct mmt_buf *data = find_ptr(s->ptr, args, argc);
		if (!data || data->len < 4)
		{
			mmt_error("\"create cid\" without data - probably because of old mmt (before Sep 6 2014) was used%s\n", "");
			return;
		}
		cid = parent = handle = ((uint32_t *)data->data)[0];
	}

	pushbuf_add_object(handle, s->cls);
	nvrm_add_object(cid, parent, handle, s->cls);
}

static void handle_nvrm_ioctl_create_unk34(struct nvrm_ioctl_create_unk34 *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;

	nvrm_add_object(s->cid, s->parent, s->handle, 0xffffffff);//TODO:class
}

static void handle_nvrm_ioctl_memory(struct nvrm_ioctl_memory *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;

	if (s->handle)
		nvrm_add_object(s->cid, s->parent, s->handle, s->cls);
}

static void handle_nvrm_ioctl_create_simple(struct nvrm_ioctl_create_simple *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;

	pushbuf_add_object(s->handle, s->cls);
	nvrm_add_object(s->cid, s->parent, s->handle, s->cls);
}

static void handle_nvrm_ioctl_destroy(struct nvrm_ioctl_destroy *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;

	nvrm_del_object(s->cid, s->parent, s->handle);
}

static void handle_nvrm_ioctl_call(struct nvrm_ioctl_call *s, struct mmt_memory_dump *args, int argc)
{
	struct mmt_buf *data = find_ptr(s->ptr, args, argc);
	if (!data)
		return;

	if (s->mthd == NVRM_MTHD_FIFO_IB_OBJECT_INFO || s->mthd == NVRM_MTHD_FIFO_IB_OBJECT_INFO2)
	{
		struct nvrm_mthd_fifo_ib_object_info *s = (void *) data->data;
		pushbuf_add_object_name(s->handle, s->name);
	}
}

static void handle_nvrm_ioctl_create_dma(struct nvrm_ioctl_create_dma *s)
{
	nvrm_add_object(s->cid, s->parent, s->handle, s->cls);
}

static void handle_nvrm_ioctl_create_vspace(struct nvrm_ioctl_create_vspace *s)
{
	if (s->foffset != 0)
	{
		struct buffer *buf;
		int found = 0;
		for (buf = buffers_list; buf != NULL; buf = buf->next)
			if (buf->mmap_offset == s->foffset)
			{
				buf->data1 = s->parent;
				buf->data2 = s->handle;

				found = 1;
				break;
			}

		if (!found)
		{
			for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
			{
				if (buf->data1 == s->parent && buf->data2 == s->handle)
				{
					mmt_log("TODO: gpu only buffer found (0x%016lx), NVRM_IOCTL_CREATE_VSPACE handling needs to be updated!\n", buf->gpu_start);
					break;
				}
			}

			struct unk_map *m = malloc(sizeof(struct unk_map));
			m->data1 = s->parent;
			m->data2 = s->handle;
			m->mmap_offset = s->foffset;
			m->next = unk_maps;
			unk_maps = m;
		}
	}

	nvrm_add_object(s->cid, s->parent, s->handle, s->cls);
}

static void handle_nvrm_ioctl_host_map(struct nvrm_ioctl_host_map *s)
{
	struct buffer *buf;
	int found = 0;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->mmap_offset == s->foffset)
		{
			buf->data1 = s->subdev;
			buf->data2 = s->handle;

			found = 1;
			break;
		}

	if (!found)
	{
		for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
		{
			if (buf->data1 == s->subdev && buf->data2 == s->handle)
			{
				if (buf->mmap_offset != s->foffset)
				{
					mmt_debug("gpu only buffer found (0x%016lx), merging mmap_offset\n", buf->gpu_start);
					buf->mmap_offset = s->foffset;
				}

				found = 1;
				break;
			}
		}

		if (!found)
		{
			struct unk_map *m = malloc(sizeof(struct unk_map));
			m->data1 = s->subdev;
			m->data2 = s->handle;
			m->mmap_offset = s->foffset;
			m->next = unk_maps;
			unk_maps = m;
		}
	}
}

static void handle_nvrm_ioctl_vspace_map(struct nvrm_ioctl_vspace_map *s)
{
	struct buffer *buf;
	int found = 0;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->data1 == s->dev && buf->data2 == s->handle && buf->length == s->size)
		{
			buf->gpu_start = s->addr;
			mmt_debug("setting gpu address for buffer %d to 0x%08lx\n", buf->id, buf->gpu_start);

			found = 1;
			break;
		}

	if (!found)
	{
		struct unk_map *tmp;
		for (tmp = unk_maps; tmp != NULL; tmp = tmp->next)
		{
			if (tmp->data1 == s->dev && tmp->data2 == s->handle)
			{
				mmt_log("TODO: unk buffer found, demmt_nv_gpu_map needs to be updated!%s\n", "");
				break;
			}
		}

		register_gpu_only_buffer(s->addr, s->size, 0, s->dev, s->handle);
	}
}

static void handle_nvrm_ioctl_vspace_unmap(struct nvrm_ioctl_vspace_unmap *s)
{
	struct buffer *buf;
	int found = 0;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->data1 == s->dev && buf->data2 == s->handle &&
				buf->gpu_start == s->addr)
		{
			mmt_debug("clearing gpu address for buffer %d (was: 0x%08lx)\n", buf->id, buf->gpu_start);
			buf->gpu_start = 0;

			found = 1;
			break;
		}

	if (!found)
	{
		for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
		{
			if (buf->data1 == s->dev && buf->data2 == s->handle && buf->gpu_start == s->addr)
			{
				mmt_debug("deregistering gpu only buffer of size %ld\n", buf->length);
				buffer_free(buf);

				found = 1;
				break;
			}
		}

		if (!found)
			mmt_log("gpu only buffer not found%s\n", "");
	}
}

static void handle_nvrm_ioctl_host_unmap(struct nvrm_ioctl_host_unmap *s)
{
}

int demmt_nv_ioctl_pre(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc)
{
	return decode_nvrm_ioctl_pre(fd, id, dir, nr, size, buf, state, args, argc);
}

int demmt_nv_ioctl_post(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc)
{
	int ret = decode_nvrm_ioctl_post(fd, id, dir, nr, size, buf, state, args, argc);

	void *d = buf->data;

	if (id == NVRM_IOCTL_CREATE)
		handle_nvrm_ioctl_create(d, args, argc);
	else if (id == NVRM_IOCTL_CREATE_SIMPLE)
		handle_nvrm_ioctl_create_simple(d);
	else if (id == NVRM_IOCTL_DESTROY)
		handle_nvrm_ioctl_destroy(d);
	else if (id == NVRM_IOCTL_CREATE_VSPACE)
		handle_nvrm_ioctl_create_vspace(d);
	else if (id == NVRM_IOCTL_HOST_MAP)
		handle_nvrm_ioctl_host_map(d);
	else if (id == NVRM_IOCTL_VSPACE_MAP)
		handle_nvrm_ioctl_vspace_map(d);
	else if (id == NVRM_IOCTL_VSPACE_UNMAP)
		handle_nvrm_ioctl_vspace_unmap(d);
	else if (id == NVRM_IOCTL_HOST_UNMAP)
		handle_nvrm_ioctl_host_unmap(d);
	else if (id == NVRM_IOCTL_CALL)
		handle_nvrm_ioctl_call(d, args, argc);
	else if (id == NVRM_IOCTL_CREATE_DMA)
		handle_nvrm_ioctl_create_dma(d);
	else if (id == NVRM_IOCTL_CREATE_UNK34)
		handle_nvrm_ioctl_create_unk34(d);
	else if (id == NVRM_IOCTL_MEMORY)
		handle_nvrm_ioctl_memory(d);

	return ret;
}

void demmt_memory_dump(struct mmt_memory_dump_prefix *d, struct mmt_buf *b, void *state)
{
	// dead code, because memory dumps are passed to ioctl_pre / ioctl_post handlers
	int i;
	mmt_log("memory dump, addr: 0x%016lx, txt: \"%s\", data.len: %d, data:", d->addr, d->str.data, b->len);

	for (i = 0; i < b->len / 4; ++i)
		mmt_log_cont(" 0x%08x", ((uint32_t *)b->data)[i]);
	mmt_log_cont("%s", "\n");
}

void demmt_nv_mmap(struct mmt_nvidia_mmap *mm, void *state)
{
	if (dump_sys_mmap)
		mmt_log("mmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx\n",
				(void *)mm->start, mm->len, mm->id, mm->offset, mm->data1, mm->data2);
	buffer_mmap(mm->id, mm->start, mm->len, mm->offset, &mm->data1, &mm->data2);
}

void demmt_nv_mmap2(struct mmt_nvidia_mmap2 *mm, void *state)
{
	if (dump_sys_mmap)
		mmt_log("mmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx, fd: %d\n",
				(void *)mm->start, mm->len, mm->id, mm->offset, mm->data1, mm->data2, mm->fd);
	buffer_mmap(mm->id, mm->start, mm->len, mm->offset, &mm->data1, &mm->data2);
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
