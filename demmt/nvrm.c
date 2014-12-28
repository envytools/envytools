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
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "demmt.h"
#include "log.h"
#include "nvrm_create.h"
#include "nvrm_decode.h"
#include "nvrm_mthd.h"
#include "nvrm.h"
#include "nvrm_object.xml.h"

struct nvrm_device
{
	int chipset;
	int fifos;
};

static inline struct nvrm_device *nvrm_dev(struct gpu_object *dev)
{
	if (dev->class_ != NVRM_DEVICE_0)
		abort();
	return dev->class_data;
}

static void dump_object_tree(struct gpu_object *obj, int level, struct gpu_object *highlight)
{
	int i, indent_level = level * 4 + 2;
	mmt_log("%s", "");
	if (obj == highlight)
	{
		mmt_log_cont("%s", "*");
		indent_level--;
	}

	for (i = 0; i < indent_level; ++i)
		mmt_log_cont("%s", " ");
//	mmt_log_cont("cid: 0x%08x, ", obj->cid);
	mmt_log_cont("handle: 0x%08x", obj->handle);
	describe_nvrm_object(obj->cid, obj->handle, "");
	if (obj->class_ == NVRM_DEVICE_0)
	{
		struct nvrm_device *d = nvrm_dev(obj);
		if (d)
			mmt_log_cont(", chipset: 0x%x", d->chipset);
	}

	mmt_log_cont("%s\n", "");
	for (i = 0; i < obj->children_space; ++i)
		if (obj->children_objects[i])
			dump_object_tree(obj->children_objects[i], level + 1, highlight);
}

static void dump_object_trees(struct gpu_object *highlight)
{
	int orphaned = 0;
	struct gpu_object *obj;
	for (obj = gpu_objects; obj != NULL; obj = obj->next)
		if (obj->parent == obj->handle)
			dump_object_tree(obj, 0, highlight);
		else if (obj->parent_object == NULL)
			orphaned = 1;

	if (orphaned)
	{
		mmt_log("Orphaned objects: %s\n", "");
		for (obj = gpu_objects; obj != NULL; obj = obj->next)
			if (obj->parent != obj->handle && obj->parent_object == NULL)
				dump_object_tree(obj, 0, highlight);
	}
}

struct gpu_object *nvrm_get_device(struct gpu_object *obj)
{
	while (obj)
	{
		if (obj->class_ == NVRM_DEVICE_0)
			return obj;
		obj = obj->parent_object;
	}

	return NULL;
}

static struct gpu_object *nvrm_find_object_by_func(struct gpu_object *parent,
		int (*check)(struct gpu_object *, uint64_t), uint64_t ctx)
{
	struct gpu_object *ret;
	int i;
	if (check(parent, ctx))
		return parent;

	for (i = 0; i < parent->children_space; ++i)
		if (parent->children_objects[i])
		{
			ret = nvrm_find_object_by_func(parent->children_objects[i], check, ctx);
			if (ret)
				return ret;
		}
	return NULL;
}

static inline int is_fifo_dma_class(uint32_t cls)
{
	switch (cls)
	{
		case NVRM_FIFO_DMA_NV40:
		case NVRM_FIFO_DMA_NV44:
			return 1;
		default:
			return 0;
	}
}

static inline int is_fifo_ib_class(uint32_t cls)
{
	switch (cls)
	{
		case NVRM_FIFO_IB_G80:
		case NVRM_FIFO_IB_G82:
		case NVRM_FIFO_IB_MCP89:
		case NVRM_FIFO_IB_GF100:
		case NVRM_FIFO_IB_GK104:
		case NVRM_FIFO_IB_GK110:
		case NVRM_FIFO_IB_UNKA2:
		case NVRM_FIFO_IB_UNKB0:
			return 1;
		default:
			return 0;
	}
}

static inline int is_fifo(struct gpu_object *obj, uint64_t ctx)
{
	return is_fifo_dma_class(obj->class_) || is_fifo_ib_class(obj->class_);
}

static inline int is_fifo_and_addr_belongs(struct gpu_object *obj, uint64_t ctx)
{
	if (is_fifo_ib_class(obj->class_))
	{
		struct fifo_state *state = get_fifo_state(obj);
		return ctx >= state->ib.addr &&
				ctx < state->ib.addr + state->ib.entries * 8;
	}

	//TODO: figure out how to get buffer boundary
	if (0 && is_fifo_dma_class(obj->class_))
	{
		struct fifo_state *state = get_fifo_state(obj);
		return ctx >= state->user.addr &&
				ctx < state->user.addr /*+ what?*/;
	}

	return 0;
}

struct gpu_object *nvrm_get_parent_fifo(struct gpu_object *obj)
{
	while (obj)
	{
		if (is_fifo(obj, 0))
			return obj;
		obj = obj->parent_object;
	}

	return NULL;
}

struct gpu_object *nvrm_get_fifo(struct gpu_object *obj, uint64_t gpu_addr)
{
	struct gpu_object *last = NULL;
	while (obj)
	{
		if (is_fifo(obj, 0))
			last = obj;
		if (is_fifo_and_addr_belongs(obj, gpu_addr))
			return obj;
		if (obj->class_ == NVRM_DEVICE_0)
		{
			struct gpu_object *fifo = nvrm_find_object_by_func(obj, is_fifo_and_addr_belongs, gpu_addr);
			if (!fifo) // fallback, for traces without ioctl_create args
			{
				fifo = nvrm_find_object_by_func(obj, is_fifo, 0);

				struct gpu_object *dev = nvrm_get_device(obj);
				int fifos = 0;
				if (dev && dev->class_data)
					fifos = nvrm_dev(dev)->fifos;

				static int warned = 0;
				if (fifos > 1 && !warned)
				{
					int chipset = nvrm_get_chipset(dev);
					if (chipset > 0x80 || chipset == 0x50)
						mmt_error("This trace may not be decoded accurately because there are multiple fifo objects "
								"and ioctl_creates for some of them were not captured with argument data%s\n", "");
					else
						mmt_error("This trace may not be decoded accurately because there are multiple fifo objects "
								"and USER buffer detection is not implemented yet%s\n", "");
					warned = 1;
				}
			}
			return fifo;
		}

		obj = obj->parent_object;
	}

	return last;
}

int nvrm_get_chipset(struct gpu_object *obj)
{
	struct gpu_object *dev = nvrm_get_device(obj);

	if (dev && dev->class_data)
		return nvrm_dev(dev)->chipset;

	if (chipset)
		return chipset;

	mmt_error("Can't detect chipset, you need to use -m option or regenerate trace with newer mmt (> Sep 7 2014)%s\n", "");
	abort();
}

static struct gpu_object *nvrm_add_object(uint32_t fd, uint32_t cid, uint32_t parent, uint32_t handle, uint32_t class_)
{
	struct gpu_object *obj = gpu_object_add(fd, cid, parent, handle, class_);
	if (dump_object_tree_on_create_destroy)
	{
		mmt_log("Object tree after create: %s\n", "");
		dump_object_trees(obj);
	}

	if (is_fifo_ib_class(class_) || is_fifo_dma_class(class_))
	{
		struct gpu_object *dev = nvrm_get_device(obj);
		if (dev && dev->class_data)
			nvrm_dev(dev)->fifos++;
	}

	return obj;
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

	if (dump_object_tree_on_create_destroy)
	{
		mmt_log("Object tree before destroy: %s\n", "");
		dump_object_trees(obj);
	}

	if (is_fifo_ib_class(obj->class_) || is_fifo_dma_class(obj->class_))
	{
		struct gpu_object *dev = nvrm_get_device(obj);
		if (dev && dev->class_data)
			nvrm_dev(dev)->fifos--;
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

	struct mmt_buf *data = NULL;
	if (s->ptr)
		data = find_ptr(s->ptr, args, argc);

	if (handle == 0)
	{
		if (!data || data->len < 4)
		{
			mmt_error("\"create cid\" without data - probably because this trace was obtained by old mmt version (before Sep 6 2014)%s\n", "");
			cid_not_found++;
			return;
		}
		cid = parent = handle = ((uint32_t *)data->data)[0];
	}

	check_cid(cid);

	struct gpu_object *obj = nvrm_add_object(fd, cid, parent, handle, s->cls);
	pushbuf_add_object(handle, s->cls, obj);

	if (is_fifo_ib_class(s->cls))
	{
		if (data)
		{
			struct fifo_state *state = get_fifo_state(obj);
			struct nvrm_create_fifo_ib *create_data = (void *)data->data;
			state->ib.addr = create_data->ib_addr;
			state->ib.entries = create_data->ib_entries;
		}
	}
	else if (is_fifo_dma_class(s->cls))
	{
		if (data)
		{
			struct fifo_state *state = get_fifo_state(obj);
			struct nvrm_create_fifo_dma *create_data = (void *)data->data;
			state->user.addr = create_data->user_addr;
		}
	}
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

	struct gpu_object *obj = nvrm_add_object(fd, s->cid, s->parent, s->handle, s->cls);
	pushbuf_add_object(s->handle, s->cls, obj);
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

				if (dump_sys_mmap)
				{
					if (!is_nouveau)
					{
						mmt_log_cont(", cid: 0x%08x, handle: 0x%08x", obj->cid, obj->handle);
						describe_nvrm_object(obj->cid, obj->handle, "");
					}
					mmt_log_cont_nl();
				}
				return;
			}
	if (dump_sys_mmap)
		mmt_log_cont_nl();

	if (!is_nouveau)
		mmt_error("nvrm_mmap: couldn't find object/space offset: 0x%016" PRIx64 "\n", mmap_offset);

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
				if (dump_sys_munmap)
				{
					if (!is_nouveau)
					{
						mmt_log_cont(", cid: 0x%08x, handle: 0x%08x", obj->cid, obj->handle);
						describe_nvrm_object(obj->cid, obj->handle, "");
					}
				}

				disconnect_cpu_mapping_from_gpu_object(cpu_mapping);
				break;
			}

	if (dump_sys_munmap)
		mmt_log_cont_nl();

	buffer_munmap(id);
}

static void device_destroy(struct gpu_object *dev)
{
	free(dev->class_data);
}

void nvrm_device_set_chipset(struct gpu_object *dev, int chipset)
{
	struct nvrm_device *d = nvrm_dev(dev);
	if (!d)
	{
		d = dev->class_data = calloc(1, sizeof(*d));
		dev->class_data_destroy = device_destroy;
	}

	if (chipset != d->chipset)
	{
		d->chipset = chipset;
		mmt_log("Chipset: NV%02X\n", chipset);
	}
}

static void handle_nvrm_ioctl_call(struct nvrm_ioctl_call *s, struct mmt_memory_dump *args, int argc)
{
	struct mmt_buf *data = find_ptr(s->ptr, args, argc);
	if (!data)
		return;

	if (s->mthd == NVRM_MTHD_FIFO_IB_OBJECT_INFO || s->mthd == NVRM_MTHD_FIFO_IB_OBJECT_INFO2)
	{
		struct nvrm_mthd_fifo_ib_object_info *mthd_data = (void *) data->data;
		struct gpu_object *obj = gpu_object_find(s->cid, s->handle);
		pushbuf_add_object_name(mthd_data->handle, mthd_data->name, obj);
	}
	else if (s->mthd == NVRM_MTHD_SUBDEVICE_GET_CHIPSET)
	{
		struct nvrm_mthd_subdevice_get_chipset *mthd_data = (void *) data->data;
		struct gpu_object *obj = gpu_object_find(s->cid, s->handle);
		if (obj)
			obj = nvrm_get_device(obj);
		if (obj)
			nvrm_device_set_chipset(obj, mthd_data->major | mthd_data->minor);
	}
}

int nvrm_ioctl_pre(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc)
{
	return decode_nvrm_ioctl_pre(fd, id, dir, nr, size, buf, state, args, argc);
}

int nvrm_ioctl_post(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, uint64_t ret, uint64_t err, void *state,
		struct mmt_memory_dump *args, int argc)
{
	int r = decode_nvrm_ioctl_post(fd, id, dir, nr, size, buf, ret, err, state, args, argc);

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

	return r;
}

void demmt_memory_dump(struct mmt_memory_dump_prefix *d, struct mmt_buf *b, void *state)
{
	// dead code, because memory dumps are passed to ioctl_pre / ioctl_post handlers
	int i;
	mmt_log("memory dump, addr: 0x%016" PRIx64 ", txt: \"%s\", data.len: %d, data:", d->addr, d->str.data, b->len);

	for (i = 0; i < b->len / 4; ++i)
		mmt_log_cont(" 0x%08x", ((uint32_t *)b->data)[i]);
	mmt_log_cont_nl();
}

void __demmt_mmap(uint64_t start, uint64_t len, uint32_t id, uint64_t offset, void *state)
{
	buffer_flush();

	if (dump_sys_mmap)
		mmt_log("mmap: address: 0x%" PRIx64 ", length: 0x%08" PRIx64 ", id: %d, offset: 0x%08" PRIx64 "",
				start, len, id, offset);

	nvrm_mmap(id, -1, start, len, offset);
}

void demmt_nv_mmap(struct mmt_nvidia_mmap *mm, void *state)
{
	__demmt_mmap(mm->start, mm->len, mm->id, mm->offset, state);
}

void __demmt_mmap2(uint64_t start, uint64_t len, uint32_t id, uint64_t offset,
		uint32_t fd, uint32_t prot, uint32_t flags, void *state)
{
	buffer_flush();

	if (dump_sys_mmap)
	{
		mmt_log("mmap: address: 0x%" PRIx64 ", length: 0x%08" PRIx64 ", id: %d, offset: 0x%08" PRIx64 ", fd: %d",
				start, len, id, offset, fd);

		if (dump_sys_mmap_details || prot != (PROT_READ | PROT_WRITE))
		{
			mmt_log_cont(", prot: %s", "");
			decode_mmap_prot(prot);
		}

		if (dump_sys_mmap_details || flags != MAP_SHARED)
		{
			mmt_log_cont(", flags: %s", "");
			decode_mmap_flags(flags);
		}
	}

	nvrm_mmap(id, fd, start, len, offset);
}

void demmt_nv_mmap2(struct mmt_nvidia_mmap2 *mm, void *state)
{
	__demmt_mmap2(mm->start, mm->len, mm->id, mm->offset, mm->fd, mm->prot, mm->flags, state);
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
