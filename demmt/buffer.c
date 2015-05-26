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

#include <inttypes.h>
#include <string.h>

#include "buffer.h"
#include "buffer_decode.h"
#include "log.h"
#include "nvrm.h"

struct gpu_object *gpu_objects = NULL;
static struct cpu_mapping **cpu_mappings = NULL;
static uint32_t last_mmap_id = UINT32_MAX;
static int writes_buffered = 0;
uint32_t max_id = UINT32_MAX;
static uint32_t preallocated_cpu_mappings = 0;

void set_cpu_mapping(uint32_t id, struct cpu_mapping *mapping)
{
	if (max_id == UINT32_MAX || id > max_id)
		max_id = id;

	if (id >= preallocated_cpu_mappings)
	{
		if (preallocated_cpu_mappings == 0)
		{
			if (id > 4)
				preallocated_cpu_mappings = id;
			else
				preallocated_cpu_mappings = 4;
		}
		preallocated_cpu_mappings *= 2;

		cpu_mappings = realloc(cpu_mappings, sizeof(void *) * preallocated_cpu_mappings);
	}

	cpu_mappings[id] = mapping;
}

struct cpu_mapping *get_cpu_mapping(uint32_t id)
{
	if (max_id == UINT32_MAX || id > max_id)
		return NULL;

	return cpu_mappings[id];
}

static void dump(struct cpu_mapping *mapping)
{
	mmt_log("currently buffered writes for id: %d:\n", mapping->id);
	dump_regions(&mapping->written_regions);
	mmt_log("end of buffered writes for id: %d\n", mapping->id);
}

static void dump_and_abort(struct cpu_mapping *mapping)
{
	dump(mapping);
	demmt_abort();
}

static void gpu_object_add_child(struct gpu_object *parent, struct gpu_object *child)
{
	int i;
	for (i = 0; i < parent->children_space; ++i)
		if (parent->children_objects[i] == NULL)
		{
			parent->children_objects[i] = child;
			return;
		}
	parent->children_objects = realloc(parent->children_objects, (parent->children_space + 10) * sizeof(parent->children_objects[0]));
	parent->children_objects[parent->children_space] = child;
	memset(&parent->children_objects[parent->children_space + 1], 0, 9 * sizeof(parent->children_objects[0]));
	parent->children_space += 10;
}

static void gpu_object_disconnect_from_parent(struct gpu_object *parent, struct gpu_object *child, int compact)
{
	int i;
	for (i = 0; i < parent->children_space; ++i)
		if (parent->children_objects[i] == child)
		{
			parent->children_objects[i] = NULL;
			child->parent_object = NULL;
			if (compact)
			{
				memmove(parent->children_objects + i, parent->children_objects + i + 1,
						sizeof(void *) * (parent->children_space - i - 1));
				parent->children_objects[parent->children_space - 1] = NULL;
			}
			return;
		}
}

struct gpu_object *gpu_object_add(uint32_t fd, uint32_t cid, uint32_t parent, uint32_t handle, uint32_t class_)
{
	struct gpu_object *obj = calloc(sizeof(struct gpu_object), 1);
	obj->fd = fd;
	obj->cid = cid;
	obj->handle = handle;
	obj->parent = parent;
	obj->parent_object = gpu_object_find(cid, parent);
	if (obj->parent_object)
		gpu_object_add_child(obj->parent_object, obj);
	obj->class_ = class_;

	obj->next = gpu_objects;
	gpu_objects = obj;
	return obj;
}

uint64_t cpu_mapping_to_gpu_addr(struct cpu_mapping *mapping, uint64_t offset)
{
	uint64_t gpu_addr = 0;
	struct gpu_object *obj = mapping->object;
	uint64_t off = mapping->object_offset + offset;

	if (!obj)
		return 0;

	struct gpu_mapping *gpu_mapping;
	for (gpu_mapping = obj->gpu_mappings; gpu_mapping != NULL; gpu_mapping = gpu_mapping->next)
		if (off >= gpu_mapping->object_offset &&
			off <  gpu_mapping->object_offset + gpu_mapping->length)
			break;

	if (gpu_mapping)
		gpu_addr = gpu_mapping->address + (off - gpu_mapping->object_offset);

	return gpu_addr;
}

struct cpu_mapping *gpu_addr_to_cpu_mapping(struct gpu_mapping *gpu_mapping, uint64_t gpu_address)
{
	uint64_t off = gpu_mapping->object_offset + gpu_address - gpu_mapping->address;
	struct cpu_mapping *mapping;
	for (mapping = gpu_mapping->object->cpu_mappings; mapping != NULL; mapping = mapping->next)
		if (off >= mapping->object_offset && off < mapping->object_offset + mapping->length)
			break;
	return mapping;
}

struct gpu_object *gpu_object_find(uint32_t cid, uint32_t handle)
{
	struct gpu_object *obj;
	for (obj = gpu_objects; obj != NULL; obj = obj->next)
		if (obj->cid == cid && obj->handle == handle)
			return obj;
	return NULL;
}

struct gpu_mapping *gpu_mapping_find(uint64_t address, struct gpu_object *dev)
{
	if (address == 0)
		return NULL;

	struct gpu_object *obj;
	struct gpu_mapping *gpu_mapping;
	for (obj = gpu_objects; obj != NULL; obj = obj->next)
	{
		if (nvrm_get_device(obj) != dev)
			continue;
		for (gpu_mapping = obj->gpu_mappings; gpu_mapping != NULL; gpu_mapping = gpu_mapping->next)
			if (address >= gpu_mapping->address && address < gpu_mapping->address + gpu_mapping->length)
				return gpu_mapping;
	}
	return NULL;
}

void *gpu_mapping_get_data(struct gpu_mapping *mapping, uint64_t address, uint64_t length)
{
	struct gpu_object *obj = mapping->object;
	uint64_t offset = address - mapping->address + mapping->object_offset;
	if (offset + length > obj->length)
		return NULL;
	return obj->data + offset;
}

void disconnect_cpu_mapping_from_gpu_object(struct cpu_mapping *cpu_mapping)
{
	struct gpu_object *obj = cpu_mapping->object;
	struct cpu_mapping *it, *prev = NULL;
	for (it = obj->cpu_mappings; it != NULL; prev = it, it = it->next)
		if (it == cpu_mapping)
		{
			if (prev)
				prev->next = it->next;
			else
				obj->cpu_mappings = it->next;
			it->next = NULL;
			cpu_mapping->object = NULL;
			cpu_mapping->object_offset = 0;
			cpu_mapping->data = NULL;

			if (cpu_mapping->id < 0 || get_cpu_mapping(cpu_mapping->id) != cpu_mapping)
				free(cpu_mapping);

			return;
		}
	mmt_error("can't find cpu_mapping on object's cpu_mappings list%s\n", "");
	demmt_abort();
}

void gpu_mapping_destroy(struct gpu_mapping *gpu_mapping)
{
	struct gpu_object *obj = gpu_mapping->object;

	struct gpu_mapping *it, *prev = NULL;
	for (it = obj->gpu_mappings; it != NULL; prev = it, it = it->next)
		if (it == gpu_mapping)
		{
			if (prev)
				prev->next = it->next;
			else
				obj->gpu_mappings = it->next;
			it->next = NULL;
			free(it);

			return;
		}
	mmt_error("can't find gpu_mapping on object's gpu_mappings list%s\n", "");
	demmt_abort();
}

void gpu_object_destroy(struct gpu_object *obj)
{
	if (obj->class_data_destroy)
		obj->class_data_destroy(obj);

	while (obj->cpu_mappings)
	{
		obj->cpu_mappings->mmap_offset = 0;
		disconnect_cpu_mapping_from_gpu_object(obj->cpu_mappings);
	}

	while (obj->gpu_mappings)
		gpu_mapping_destroy(obj->gpu_mappings);

	if (obj->parent_object)
		gpu_object_disconnect_from_parent(obj->parent_object, obj, 1);
	if (obj->children_space)
	{
		int i;
		for (i = 0; i < obj->children_space; ++i)
			if (obj->children_objects[i])
				gpu_object_disconnect_from_parent(obj, obj->children_objects[i], 0);
		free(obj->children_objects);
		obj->children_space = 0;
	}

	int i;
	for (i = 0; i < MAX_USAGES; ++i)
		free(obj->usage[i].desc);

	free_regions(&obj->written_regions);

	struct gpu_object *it, *prev = NULL;
	for (it = gpu_objects; it != NULL; prev = it, it = it->next)
		if (it == obj)
		{
			if (prev)
				prev->next = obj->next;
			else
				gpu_objects = obj->next;

			obj->next = NULL;
			free(obj->data);
			free(obj);
			//mmt_debug("object destroyed%s\n", "");

			return;
		}
	mmt_error("can't find object on global gpu_objects list%s\n", "");
	demmt_abort();
}

static void dump_buffered_writes()
{
	struct cpu_mapping *mapping;
	if (last_mmap_id == UINT32_MAX)
		return;

	mapping = get_cpu_mapping(last_mmap_id);

	if (MMT_DEBUG)
		dump(mapping);

	flush_written_regions(mapping);

	free_regions(&mapping->written_regions);
	writes_buffered = 0;
	last_mmap_id = UINT32_MAX;
}

void buffer_ioctl_pre()
{
	if (writes_buffered)
		dump_buffered_writes();
}

void buffer_mmap(uint32_t id, uint32_t fd, uint64_t cpu_start, uint64_t len, uint64_t mmap_offset)
{
	struct cpu_mapping *mapping = calloc(sizeof(struct cpu_mapping), 1);
	mapping->fd = fd;
	mapping->fdtype = demmt_get_fdtype(fd);
	mapping->mmap_offset = mmap_offset;
	mapping->data = calloc(len, 1);
	mapping->length = len;
	mapping->id = id;
	mapping->cpu_addr = cpu_start;

	set_cpu_mapping(id, mapping);
}

void buffer_munmap(uint32_t id)
{
	struct cpu_mapping *mapping = get_cpu_mapping(id);
	if (!mapping || mapping->object || mapping->next)
	{
		mmt_error("inconsistent mapping data%s\n", "");
		demmt_abort();
	}
	free(mapping->data);
	free(mapping);
	set_cpu_mapping(id, NULL);
}

void buffer_mremap(struct mmt_mremap *mm)
{
	if (writes_buffered)
	{
		mmt_debug("mremap, flushing buffered writes%s\n", "");
		dump_buffered_writes();
		mmt_debug("%s\n", "");
	}

	struct cpu_mapping *mapping = get_cpu_mapping(mm->id);
	if (mm->len != mapping->length)
	{
		mapping->data = realloc(mapping->data, mm->len);
		if (mm->len > mapping->length)
			memset(mapping->data + mapping->length, 0, mm->len - mapping->length);
	}

	mapping->mmap_offset = mm->offset;
	mapping->cpu_addr = mm->start;
	mapping->length = mm->len;
}

void buffer_register_mmt_read(struct mmt_read *r)
{
	if (writes_buffered)
	{
		mmt_debug("%s\n", "read registered, flushing currently buffered writes");
		dump_buffered_writes();
		mmt_debug("%s\n", "");
	}
}

static void buffer_register_cpu_write(struct cpu_mapping *mapping, uint32_t offset, uint8_t len, const void *data)
{
	if (mapping->length < offset + len)
	{
		mmt_error("buffer %d is too small (%" PRId64 ") for write starting at %d and length %d\n",
				mapping->id, mapping->length, offset, len);
		dump_and_abort(mapping);
	}

	memcpy(mapping->data + offset, data, len);

	if (!regions_add_range(&mapping->written_regions, offset, len))
		dump_and_abort(mapping);
	if (mapping->object)
		if (!regions_add_range(&mapping->object->written_regions, offset + mapping->object_offset, len))
			dump_and_abort(mapping);
}

void gpu_mapping_register_write(struct gpu_mapping *mapping, uint64_t address, uint32_t len, const void *data)
{
	struct gpu_object *obj = mapping->object;
	uint64_t gpu_object_offset = address - mapping->address + mapping->object_offset;

	if (obj->length < gpu_object_offset + len)
	{
		mmt_error("object 0x%x:0x%x is too small (%" PRId64 ") for write starting at %" PRId64 " and length %d\n",
				obj->cid, obj->handle, obj->length, gpu_object_offset, len);
		demmt_abort();
	}

	memcpy(obj->data + gpu_object_offset, data, len);

	if (!regions_add_range(&obj->written_regions, gpu_object_offset, len))
		demmt_abort();
}

void gpu_mapping_register_copy(struct gpu_mapping *dst_mapping, uint64_t dst_address,
		struct gpu_mapping *src_mapping, uint64_t src_address, uint32_t len)
{
	void *src = gpu_mapping_get_data(src_mapping, src_address, len);
	gpu_mapping_register_write(dst_mapping, dst_address, len, src);
}

void buffer_register_mmt_write(struct mmt_write *w)
{
	uint32_t id = w->id;

	struct cpu_mapping *mapping = get_cpu_mapping(id);
	if (mapping == NULL)
	{
		mmt_error("buffer %d does not exist\n", id);
		demmt_abort();
	}

	if (last_mmap_id != id && last_mmap_id != UINT32_MAX && writes_buffered)
	{
		mmt_debug("new region write registered (new: %d, old: %d), flushing buffered writes\n",
				id, last_mmap_id);
		dump_buffered_writes();

		mmt_debug("%s\n", "");
	}

	buffer_register_cpu_write(mapping, w->offset, w->len, w->data);
	writes_buffered = 1;
	last_mmap_id = id;
}

void buffer_flush()
{
	dump_buffered_writes();
}
