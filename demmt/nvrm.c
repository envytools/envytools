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

#include <stdlib.h>
#include <string.h>
#include "demmt.h"
#include "log.h"
#include "nvrm_decode.h"
#include "nvrm_mthd.h"
#include "nvrm.h"

static struct gpu_object *nvrm_add_object(uint32_t fd, uint32_t cid, uint32_t parent, uint32_t handle, uint32_t class_)
{
	return gpu_object_add(fd, cid, parent, handle, class_);
}

static void nvrm_destroy_gpu_object(uint32_t fd, uint32_t cid, uint32_t parent, uint32_t handle)
{
	struct gpu_object *obj = gpu_object_find(cid, handle);
	if (obj == NULL)
	{
		uint16_t cl = handle & 0xffff;
		// userspace deletes objects which it didn't create and kernel returns SUCCESS :/
		// just ignore known offenders
		switch (cl)
		{
			case 0x0014:
			case 0x0202:
			case 0x0301:
			case 0x0308:
			case 0x0360:
			case 0x0371:
			case 0x1e00:
			case 0x1e01:
			case 0x1e10:
			case 0x1e20:
				break;
			default:
				mmt_error("trying to destroy object 0x%08x / 0x%08x which does not exist!\n", cid, handle);
		}
		return;
	}

	gpu_object_destroy(obj);
}

static int cid_not_found = 0;
static void check_cid(uint32_t cid)
{
	if (cid_not_found && gpu_object_find(cid, cid) == NULL)
	{
		nvrm_add_object(-1, cid, cid, cid, 0x0041);
		cid_not_found--;
	}
}

static void handle_nvrm_ioctl_create(uint32_t fd, struct nvrm_ioctl_create *s, struct mmt_memory_dump *args, int argc)
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
			mmt_error("\"create cid\" without data - probably because this trace was obtained by old mmt version (before Sep 6 2014)%s\n", "");
			cid_not_found++;
			return;
		}
		cid = parent = handle = ((uint32_t *)data->data)[0];
	}

	check_cid(cid);

	pushbuf_add_object(handle, s->cls);
	nvrm_add_object(fd, cid, parent, handle, s->cls);
}

static void handle_nvrm_ioctl_create_unk34(uint32_t fd, struct nvrm_ioctl_create_unk34 *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;
	check_cid(s->cid);

	nvrm_add_object(fd, s->cid, s->parent, s->handle, 0xffffffff);//TODO:class
}

static void handle_nvrm_ioctl_memory(uint32_t fd, struct nvrm_ioctl_memory *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;
	check_cid(s->cid);

	if (s->handle)
		nvrm_add_object(fd, s->cid, s->parent, s->handle, s->cls);
}

static void handle_nvrm_ioctl_create_simple(uint32_t fd, struct nvrm_ioctl_create_simple *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;
	check_cid(s->cid);

	pushbuf_add_object(s->handle, s->cls);
	nvrm_add_object(fd, s->cid, s->parent, s->handle, s->cls);
}

static void handle_nvrm_ioctl_destroy(uint32_t fd, struct nvrm_ioctl_destroy *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;
	check_cid(s->cid);

	nvrm_destroy_gpu_object(fd, s->cid, s->parent, s->handle);
}

static void handle_nvrm_ioctl_create_dma(uint32_t fd, struct nvrm_ioctl_create_dma *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;
	check_cid(s->cid);

	nvrm_add_object(fd, s->cid, s->parent, s->handle, s->cls);
}

static void host_map(struct gpu_object *obj, uint32_t fd, uint64_t mmap_offset,
		uint64_t object_offset, uint64_t length, uint32_t subdev)
{
	struct cpu_mapping *mapping = calloc(sizeof(struct cpu_mapping), 1);
	mapping->fd = fd;
	mapping->subdev = subdev;
	mapping->mmap_offset = mmap_offset;
	mapping->cpu_addr = 0;
	mapping->object_offset = object_offset;
	mapping->length = roundup_to_pagesize(length);
	uint64_t min_obj_len = object_offset + mapping->length;
	if (min_obj_len > obj->length)
	{
		obj->data = realloc(obj->data, min_obj_len);
		memset(obj->data + obj->length, 0, min_obj_len - obj->length);
		obj->length = min_obj_len;
	}
	mapping->data = obj->data + object_offset;
	mapping->object = obj;
	mapping->next = obj->cpu_mappings;
	obj->cpu_mappings = mapping;
}

static void handle_nvrm_ioctl_create_vspace(uint32_t fd, struct nvrm_ioctl_create_vspace *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;
	check_cid(s->cid);

	struct gpu_object *obj = nvrm_add_object(fd, s->cid, s->parent, s->handle, s->cls);
	if (!obj)
	{
		mmt_error("nvrm_ioctl_host_map: cannot find object 0x%08x 0x%08x\n", s->cid, s->handle);
		return;
	}

	if (s->foffset)
		host_map(obj, fd, s->foffset, 0, s->limit + 1, 0);
}

static void handle_nvrm_ioctl_host_map(uint32_t fd, struct nvrm_ioctl_host_map *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;
	check_cid(s->cid);

	struct gpu_object *obj = gpu_object_find(s->cid, s->handle);
	if (!obj)
	{
		mmt_error("nvrm_ioctl_host_map: cannot find object 0x%08x 0x%08x\n", s->cid, s->handle);
		return;
	}

	// NOTE: s->subdev may not be object's parent and may not even be an ancestor of its parent
/*	if (obj->parent != s->subdev)
	{
		struct gpu_object *subdev = find_object(s->cid, s->subdev);
		while (subdev && subdev->handle != obj->parent)
			subdev = subdev->parent_object;
		if (!subdev)
		{
			mmt_error("nvrm_ioctl_host_map: subdev 0x%08x is not an ancestor of object's parent (0x%08x)\n", s->subdev, obj->parent);
			return;
		}
	}*/

	host_map(obj, fd, s->foffset, s->base, s->limit + 1, s->subdev);
}

static void handle_nvrm_ioctl_host_unmap(uint32_t fd, struct nvrm_ioctl_host_unmap *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;
	check_cid(s->cid);

	struct gpu_object *obj = gpu_object_find(s->cid, s->handle);
	if (!obj)
	{
		mmt_error("nvrm_ioctl_host_unmap: cannot find object 0x%08x 0x%08x\n", s->cid, s->handle);
		return;
	}

	struct cpu_mapping *cpu_mapping;
	for (cpu_mapping = obj->cpu_mappings; cpu_mapping != NULL; cpu_mapping = cpu_mapping->next)
		if (cpu_mapping->mmap_offset == s->foffset)
		{
			cpu_mapping->mmap_offset = 0;
			disconnect_cpu_mapping_from_gpu_object(cpu_mapping);
			return;
		}

	// weird, host_unmap accepts cpu addresses in foffset field
	for (cpu_mapping = obj->cpu_mappings; cpu_mapping != NULL; cpu_mapping = cpu_mapping->next)
		if (cpu_mapping->cpu_addr == s->foffset)
		{
			//mmt_error("host_unmap with cpu address as offset, wtf?%s\n", "");
			cpu_mapping->mmap_offset = 0;
			disconnect_cpu_mapping_from_gpu_object(cpu_mapping);
			return;
		}

	mmt_error("can't find matching mapping%s\n", "");
	abort();
}

static void handle_nvrm_ioctl_vspace_map(uint32_t fd, struct nvrm_ioctl_vspace_map *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;
	check_cid(s->cid);

	struct gpu_object *obj = gpu_object_find(s->cid, s->handle);
	if (!obj)
	{
		mmt_error("nvrm_ioctl_vspace_map: cannot find object 0x%08x 0x%08x\n", s->cid, s->handle);
		return;
	}

	struct gpu_mapping *mapping = calloc(sizeof(struct gpu_mapping), 1);
	mapping->fd = fd;
	mapping->dev = s->dev;
	mapping->vspace = s->vspace;

	mapping->address = s->addr;
	mapping->object_offset = s->base;
	mapping->length = s->size;
	if (s->size > obj->length)
	{
		obj->data = realloc(obj->data, s->size);
		memset(obj->data + obj->length, 0, s->size - obj->length);
		obj->length = s->size;
	}
	mapping->object = obj;
	mapping->next = obj->gpu_mappings;
	obj->gpu_mappings = mapping;
}

static void handle_nvrm_ioctl_vspace_unmap(uint32_t fd, struct nvrm_ioctl_vspace_unmap *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;
	check_cid(s->cid);

	struct gpu_object *obj = gpu_object_find(s->cid, s->handle);
	if (!obj)
	{
		mmt_error("nvrm_ioctl_vspace_unmap: cannot find object 0x%08x 0x%08x\n", s->cid, s->handle);
		return;
	}

	struct gpu_mapping *gpu_mapping;
	for (gpu_mapping = obj->gpu_mappings; gpu_mapping != NULL; gpu_mapping = gpu_mapping->next)
		if (gpu_mapping->address == s->addr)
		{
			gpu_mapping->address = 0;
			gpu_mapping_destroy(gpu_mapping);
			return;
		}

	mmt_error("can't find matching gpu_mappings%s\n", "");
	abort();
}

void nvrm_mmap(uint32_t id, uint32_t fd, uint64_t mmap_addr, uint64_t len, uint64_t mmap_offset)
{
	struct gpu_object *obj;
	struct cpu_mapping *cpu_mapping;
	for (obj = gpu_objects; obj != NULL; obj = obj->next)
		for (cpu_mapping = obj->cpu_mappings; cpu_mapping != NULL; cpu_mapping = cpu_mapping->next)
			//can't validate fd
			if ((cpu_mapping->mmap_offset & ~0xfff) == mmap_offset)
			{
				cpu_mapping->mmap_addr = mmap_addr;
				cpu_mapping->cpu_addr = mmap_addr | (cpu_mapping->mmap_offset & 0xfff);
				cpu_mapping->id = id;
				cpu_mappings[id] = cpu_mapping;
				return;
			}

	if (!is_nouveau)
		mmt_error("nvrm_mmap: couldn't find object/space offset: 0x%016lx\n", mmap_offset);

	buffer_mmap(id, fd, mmap_addr, len, mmap_offset);
}

void nvrm_munmap(uint32_t id, uint64_t mmap_addr, uint64_t len, uint64_t mmap_offset)
{
	struct gpu_object *obj;
	struct cpu_mapping *cpu_mapping;

	for (obj = gpu_objects; obj != NULL; obj = obj->next)
		for (cpu_mapping = obj->cpu_mappings; cpu_mapping != NULL; cpu_mapping = cpu_mapping->next)
			if (cpu_mapping->mmap_addr == mmap_addr)
			{
				disconnect_cpu_mapping_from_gpu_object(cpu_mapping);
				break;
			}

	buffer_munmap(id);
}

struct mmt_buf *find_ptr(uint64_t ptr, struct mmt_memory_dump *args, int argc)
{
	int i;
	for (i = 0; i < argc; ++i)
		if (args[i].addr == ptr)
			return args[i].data;

	return NULL;
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

int nvrm_ioctl_pre(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc)
{
	return decode_nvrm_ioctl_pre(fd, id, dir, nr, size, buf, state, args, argc);
}

int nvrm_ioctl_post(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc)
{
	int ret = decode_nvrm_ioctl_post(fd, id, dir, nr, size, buf, state, args, argc);

	void *d = buf->data;

	if (id == NVRM_IOCTL_CREATE)
		handle_nvrm_ioctl_create(fd, d, args, argc);
	else if (id == NVRM_IOCTL_CREATE_SIMPLE)
		handle_nvrm_ioctl_create_simple(fd, d);
	else if (id == NVRM_IOCTL_DESTROY)
		handle_nvrm_ioctl_destroy(fd, d);
	else if (id == NVRM_IOCTL_CREATE_VSPACE)
		handle_nvrm_ioctl_create_vspace(fd, d);
	else if (id == NVRM_IOCTL_HOST_MAP)
		handle_nvrm_ioctl_host_map(fd, d);
	else if (id == NVRM_IOCTL_VSPACE_MAP)
		handle_nvrm_ioctl_vspace_map(fd, d);
	else if (id == NVRM_IOCTL_VSPACE_UNMAP)
		handle_nvrm_ioctl_vspace_unmap(fd, d);
	else if (id == NVRM_IOCTL_HOST_UNMAP)
		handle_nvrm_ioctl_host_unmap(fd, d);
	else if (id == NVRM_IOCTL_CALL)
		handle_nvrm_ioctl_call(d, args, argc);
	else if (id == NVRM_IOCTL_CREATE_DMA)
		handle_nvrm_ioctl_create_dma(fd, d);
	else if (id == NVRM_IOCTL_CREATE_UNK34)
		handle_nvrm_ioctl_create_unk34(fd, d);
	else if (id == NVRM_IOCTL_MEMORY)
		handle_nvrm_ioctl_memory(fd, d);

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

	nvrm_mmap(mm->id, -1, mm->start, mm->len, mm->offset);
}

void demmt_nv_mmap2(struct mmt_nvidia_mmap2 *mm, void *state)
{
	if (dump_sys_mmap)
		mmt_log("mmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx, fd: %d\n",
				(void *)mm->start, mm->len, mm->id, mm->offset, mm->data1, mm->data2, mm->fd);
	nvrm_mmap(mm->id, mm->fd, mm->start, mm->len, mm->offset);
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
