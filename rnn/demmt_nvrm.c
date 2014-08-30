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
#include "nvrm_ioctl.h"
#include "util.h"

#define OLD_MSGS 0

static void decode_nvrm_ioctl_check_version_str(struct nvrm_ioctl_check_version_str *s)
{
	mmt_log_cont("cmd: %d, reply: %d, vernum: %s\n", s->cmd, s->reply, s->vernum);
}

static void decode_nvrm_ioctl_env_info(struct nvrm_ioctl_env_info *s)
{
	mmt_log_cont("pat_supported: %d\n", s->pat_supported);
}

static void decode_nvrm_ioctl_card_info2(struct nvrm_ioctl_card_info2 *s)
{
	int i, j;
	for (i = 0; i < 32; ++i)
	{
		int valid = 0;
		for (j = 0; j < sizeof(s->card[i]); ++j)
			if (((unsigned char *)&s->card[i])[j] != 0)
			{
				valid = 1;
				break;
			}
		if (valid)
			mmt_log_cont("%d: flags: 0x%08x, domain: 0x%08x, bus: %d, slot: %d, function: %d, "
					"vendor_id: 0x%04x, device_id: 0x%04x, gpu_id: 0x%08x, interrupt: 0x%08x, reg_address: 0x%016lx, "
					"reg_size: 0x%016lx, fb_address: 0x%016lx, fb_size: 0x%016lx, index: %d\n", i,
					s->card[i].flags, s->card[i].domain, s->card[i].bus, s->card[i].slot,
					s->card[i].function, s->card[i].vendor_id, s->card[i].device_id,
					s->card[i].gpu_id, s->card[i].interrupt, s->card[i].reg_address,
					s->card[i].reg_size, s->card[i].fb_address, s->card[i].fb_size,
					s->card[i].index);
	}

}

static void decode_nvrm_ioctl_create(struct nvrm_ioctl_create *s)
{
	mmt_log_cont("cid: 0x%08x, parent: 0x%08x, handle: 0x%08x, class: 0x%08x, ptr: 0x%016lx, status: 0x%08x\n",
			s->cid, s->parent, s->handle, s->cls, s->ptr, s->status);
}

static void decode_nvrm_ioctl_call(struct nvrm_ioctl_call *s)
{
	mmt_log_cont("cid: 0x%08x, handle: 0x%08x, mthd: 0x%08x, ptr: 0x%016lx, size: 0x%08x, status: 0x%08x\n",
			s->cid, s->handle, s->mthd, s->ptr, s->size, s->status);
}

static void decode_nvrm_ioctl_unk4d(struct nvrm_ioctl_unk4d *s)
{
	mmt_log_cont("cid: 0x%08x, handle: 0x%08x, unk08: 0x%016lx, unk10: 0x%016lx, slen: 0x%016lx, "
			"sptr: 0x%016lx, unk28: 0x%016lx, unk30: 0x%016lx, unk38: 0x%016lx, status: 0x%08x\n",
			s->cid, s->handle, s->unk08, s->unk10, s->slen, s->sptr, s->unk28, s->unk30, s->unk38, s->status);
}

static void decode_nvrm_ioctl_create_vspace(struct nvrm_ioctl_create_vspace *s)
{
	mmt_log_cont("cid: 0x%08x, parent: 0x%08x, handle: 0x%08x, class: 0x%08x, flags: 0x%08x, "
			"unk14: 0x%08x, foffset: 0x%016lx, limit: 0x%016lx, status: 0x%08x\n",
			s->cid, s->parent, s->handle, s->cls, s->flags, s->unk14, s->foffset, s->limit, s->status);
}

static void decode_nvrm_ioctl_create_dma(struct nvrm_ioctl_create_dma *s)
{
	mmt_log_cont("cid: 0x%08x, handle: 0x%08x, class: 0x%08x, flags: 0x%08x, "
			"unk10: 0x%08x, parent: 0x%08x, base: 0x%016lx, limit: 0x%016lx, status: 0x%08x\n",
			s->cid, s->handle, s->cls, s->flags, s->unk10, s->parent, s->base, s->limit, s->status);
}

static void decode_nvrm_ioctl_host_map(struct nvrm_ioctl_host_map *s)
{
	mmt_log_cont("cid: 0x%08x, subdev: 0x%08x, handle: 0x%08x, base: 0x%016lx, limit: 0x%016lx, foffset: 0x%016lx, status: 0x%08x\n",
			s->cid, s->subdev, s->handle, s->base, s->limit, s->foffset, s->status);
}

static void decode_nvrm_ioctl_destroy(struct nvrm_ioctl_destroy *s)
{
	mmt_log_cont("cid: 0x%08x, parent: 0x%08x, handle: 0x%08x, status: 0x%08x\n",
			s->cid, s->parent, s->handle, s->status);
}

static void decode_nvrm_ioctl_vspace_map(struct nvrm_ioctl_vspace_map *s)
{
	mmt_log_cont("cid: 0x%08x, dev: 0x%08x, vspace: 0x%08x, handle: 0x%08x, base: 0x%016lx, "
			"size: 0x%016lx, flags: 0x%08x, unk24: 0x%08x, addr: 0x%016lx, status: 0x%08x\n",
			s->cid, s->dev, s->vspace, s->handle, s->base, s->size, s->flags, s->unk24, s->addr, s->status);
}

static void decode_nvrm_ioctl_host_unmap(struct nvrm_ioctl_host_unmap *s)
{
	mmt_log_cont("cid: 0x%08x, subdev: 0x%08x, handle: 0x%08x, foffset: 0x%016lx, status: 0x%08x\n",
			s->cid, s->subdev, s->handle, s->foffset, s->status);
}

static void decode_nvrm_ioctl_memory(struct nvrm_ioctl_memory *s)
{
	mmt_log_cont("cid: 0x%08x, parent: 0x%08x, class: 0x%08x, unk0c: 0x%08x, status: 0x%08x, "
			"unk14: 0x%08x, vram_total: 0x%016lx, vram_free: 0x%016lx, vspace: 0x%08x, handle: 0x%08x, "
			"unk30: 0x%08x, flags1: 0x%08x, unk38: 0x%016lx, flags2: 0x%08x, unk44: 0x%08x, "
			"unk48: 0x%016lx, flags3: 0x%08x, unk54: 0x%08x, unk58: 0x%016lx, size: 0x%016lx, "
			"align: 0x%016lx, base: 0x%016lx, limit: 0x%016lx, unk80: 0x%016lx, unk88: 0x%016lx, "
			"unk90: 0x%016lx, unk98: 0x%016lx\n",
			s->cid, s->parent, s->cls, s->unk0c, s->status, s->unk14, s->vram_total, s->vram_free,
			s->vspace, s->handle, s->unk30, s->flags1, s->unk38, s->flags2, s->unk44, s->unk48,
			s->flags3, s->unk54, s->unk58, s->size, s->align, s->base, s->limit, s->unk80, s->unk88,
			s->unk90, s->unk98);
}

static void decode_nvrm_ioctl_vspace_unmap(struct nvrm_ioctl_vspace_unmap *s)
{
	mmt_log_cont("cid: 0x%08x, dev: 0x%08x, vspace: 0x%08x, handle: 0x%08x, unk10: 0x%016lx, addr: 0x%016lx, status: 0x%08x\n",
			s->cid, s->dev, s->vspace, s->handle, s->unk10, s->addr, s->status);
}

static void decode_nvrm_ioctl_unk5e(struct nvrm_ioctl_unk5e *s)
{
	mmt_log_cont("cid: 0x%08x, subdev: 0x%08x, handle: 0x%08x, foffset: 0x%016lx, ptr: 0x%016lx, unk20: 0x%08x, unk24: 0x%08x\n",
			s->cid, s->subdev, s->handle, s->foffset, s->ptr, s->unk20, s->unk24);
}

static void decode_nvrm_ioctl_bind(struct nvrm_ioctl_bind *s)
{
	mmt_log_cont("cid: 0x%08x, target: 0x%08x, handle: 0x%08x, status: 0x%08x\n",
			s->cid, s->target, s->handle, s->status);
}

static void decode_nvrm_ioctl_create_os_event(struct nvrm_ioctl_create_os_event *s)
{
	mmt_log_cont("cid: 0x%08x, handle: 0x%08x, ehandle: 0x%08x, fd: %d, status: 0x%08x\n",
			s->cid, s->handle, s->ehandle, s->fd, s->status);
}

static void decode_nvrm_ioctl_destroy_os_event(struct nvrm_ioctl_destroy_os_event *s)
{
	mmt_log_cont("cid: 0x%08x, handle: 0x%08x, fd: %d, status: 0x%08x\n",
			s->cid, s->handle, s->fd, s->status);
}

static void decode_nvrm_ioctl_create_simple(struct nvrm_ioctl_create_simple *s)
{
	mmt_log_cont("cid: 0x%08x, parent: 0x%08x, handle: 0x%08x, class: 0x%08x, status: 0x%08x\n",
			s->cid, s->parent, s->handle, s->cls, s->status);
}

static void decode_nvrm_ioctl_get_param(struct nvrm_ioctl_get_param *s)
{
	mmt_log_cont("cid: 0x%08x, handle: 0x%08x, key: 0x%08x, value: 0x%08x, status: 0x%08x\n",
			s->cid, s->handle, s->key, s->value, s->status);
}

static void decode_nvrm_ioctl_query(struct nvrm_ioctl_query *s)
{
	mmt_log_cont("cid: 0x%08x, handle: 0x%08x, query: 0x%08x, size: 0x%08x, ptr: 0x%016lx, status: 0x%08x\n",
			s->cid, s->handle, s->query, s->size, s->ptr, s->status);
}

#define _(CTL, STR, FUN) { CTL, #CTL , sizeof(STR), FUN }
struct
{
	uint32_t id;
	const char *name;
	int size;
	void *fun;
} ioctls[] =
{
		_(NVRM_IOCTL_CHECK_VERSION_STR, struct nvrm_ioctl_check_version_str, decode_nvrm_ioctl_check_version_str),
		_(NVRM_IOCTL_ENV_INFO, struct nvrm_ioctl_env_info, decode_nvrm_ioctl_env_info),
		_(NVRM_IOCTL_CARD_INFO2, struct nvrm_ioctl_card_info2, decode_nvrm_ioctl_card_info2),
		_(NVRM_IOCTL_CREATE, struct nvrm_ioctl_create, decode_nvrm_ioctl_create),
		_(NVRM_IOCTL_CALL, struct nvrm_ioctl_call, decode_nvrm_ioctl_call),
		_(NVRM_IOCTL_UNK4D, struct nvrm_ioctl_unk4d, decode_nvrm_ioctl_unk4d),
		_(NVRM_IOCTL_CREATE_VSPACE, struct nvrm_ioctl_create_vspace, decode_nvrm_ioctl_create_vspace),
		_(NVRM_IOCTL_CREATE_DMA, struct nvrm_ioctl_create_dma, decode_nvrm_ioctl_create_dma),
		_(NVRM_IOCTL_HOST_MAP, struct nvrm_ioctl_host_map, decode_nvrm_ioctl_host_map),
		_(NVRM_IOCTL_DESTROY, struct nvrm_ioctl_destroy, decode_nvrm_ioctl_destroy),
		_(NVRM_IOCTL_VSPACE_MAP, struct nvrm_ioctl_vspace_map, decode_nvrm_ioctl_vspace_map),
		_(NVRM_IOCTL_HOST_UNMAP, struct nvrm_ioctl_host_unmap, decode_nvrm_ioctl_host_unmap),
		_(NVRM_IOCTL_MEMORY, struct nvrm_ioctl_memory, decode_nvrm_ioctl_memory),
		_(NVRM_IOCTL_VSPACE_UNMAP, struct nvrm_ioctl_vspace_unmap, decode_nvrm_ioctl_vspace_unmap),
		_(NVRM_IOCTL_UNK5E, struct nvrm_ioctl_unk5e, decode_nvrm_ioctl_unk5e),
		_(NVRM_IOCTL_BIND, struct nvrm_ioctl_bind, decode_nvrm_ioctl_bind),
		_(NVRM_IOCTL_CREATE_OS_EVENT, struct nvrm_ioctl_create_os_event, decode_nvrm_ioctl_create_os_event),
		_(NVRM_IOCTL_DESTROY_OS_EVENT, struct nvrm_ioctl_destroy_os_event, decode_nvrm_ioctl_destroy_os_event),
		_(NVRM_IOCTL_CREATE_SIMPLE, struct nvrm_ioctl_create_simple, decode_nvrm_ioctl_create_simple),
		_(NVRM_IOCTL_GET_PARAM, struct nvrm_ioctl_get_param, decode_nvrm_ioctl_get_param),
		_(NVRM_IOCTL_QUERY, struct nvrm_ioctl_query, decode_nvrm_ioctl_query),
};
#undef _

static void handle_nvrm_ioctl_create(struct nvrm_ioctl_create *s)
{
	if (OLD_MSGS)
		mmt_log("create object: parent: 0x%08x, handle: 0x%08x, class: 0x%08x\n", s->parent, s->handle, s->cls);

	pushbuf_add_object(s->handle, s->cls);
}

static void handle_nvrm_ioctl_destroy(struct nvrm_ioctl_destroy *s)
{
	if (OLD_MSGS)
		mmt_log("destroy object: parent: 0x%08x, handle: 0x%08x\n", s->parent, s->handle);
}

static void handle_nvrm_ioctl_call(struct nvrm_ioctl_call *s)
{
	if (OLD_MSGS)
		mmt_log("call method: handle: 0x%08x, mthd: 0x%08x\n", s->handle, s->mthd);
}

static void handle_nvrm_ioctl_create_dma(struct nvrm_ioctl_create_dma *s)
{
	if (OLD_MSGS)
		mmt_log("create dma object, handle: 0x%08x, class: 0x%08x, parent: 0x%08x\n",
				s->handle, s->cls, s->parent);
}

static void handle_nvrm_ioctl_create_vspace(struct nvrm_ioctl_create_vspace *s)
{
	if (OLD_MSGS)
		mmt_log("create mapped object: mmap_offset: %p, parent: 0x%08x, handle: 0x%08x, class: 0x%08x\n",
				(void *)s->foffset, s->parent, s->handle, s->cls);

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
					mmt_log("TODO: gpu only buffer found, NVRM_IOCTL_CREATE_VSPACE handling needs to be updated!%s\n", "");
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
}

static void handle_nvrm_ioctl_host_map(struct nvrm_ioctl_host_map *s)
{
	if (OLD_MSGS)
		mmt_log("allocate map: mmap_offset: %p, subdev: 0x%08x, handle: 0x%08x\n",
				(void *)s->foffset, s->subdev, s->handle);

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
				mmt_log("gpu only buffer found, merging%s\n", "");
				buf->mmap_offset = s->foffset;

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
	if (OLD_MSGS)
		mmt_log("gpu map: dev: 0x%08x, vspace: 0x%08x, handle: 0x%08x, gpu_start: 0x%08lx, len: 0x%08lx\n",
				s->dev, s->vspace, s->handle, s->addr, s->size);

	struct buffer *buf;
	int found = 0;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->data1 == s->dev && buf->data2 == s->handle && buf->length == s->size)
		{
			buf->gpu_start = s->addr;
			mmt_log("setting gpu address for buffer %d to 0x%08lx\n", buf->id, buf->gpu_start);

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
	if (OLD_MSGS)
		mmt_log("gpu unmap: dev: 0x%08x, vspace: 0x%08x, handle: 0x%08x, gpu_start: 0x%08lx\n",
				s->dev, s->vspace, s->handle, s->addr);

	struct buffer *buf;
	int found = 0;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->data1 == s->dev && buf->data2 == s->handle &&
				buf->gpu_start == s->addr)
		{
			mmt_log("clearing gpu address for buffer %d (was: 0x%08lx)\n", buf->id, buf->gpu_start);
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
				mmt_log("deregistering gpu only buffer of size %ld\n", buf->length);
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
	if (OLD_MSGS)
		mmt_log("nv_munmap: mmap_offset: %p, subdev: 0x%08x, handle: 0x%08x\n",
				(void *)s->foffset, s->subdev, s->handle);

	struct buffer *buf;
	int found = 0;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->mmap_offset == s->foffset)
		{
			// clear cpu_start only?
			buffer_free(buf);

			found = 1;
			break;
		}

	if (!found)
		mmt_log("couldn't find buffer to free%s\n", ""); // find by subdev/handle?
}

static void handle_nvrm_ioctl_bind(struct nvrm_ioctl_bind *s)
{
	if (OLD_MSGS)
		mmt_log("bind: target: 0x%08x, handle: 0x%08x\n", s->target, s->handle);
}

int demmt_nv_ioctl_pre(uint32_t id, uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *buf, void *state)
{
	int k, found = 0;
	for (k = 0; k < ARRAY_SIZE(ioctls); ++k)
		if (ioctls[k].id == id)
		{
			if (ioctls[k].size == buf->len)
			{
				mmt_log("%s pre,  ", ioctls[k].name);
				void (*fun)(void *) = ioctls[k].fun;
				fun(buf->data);
				found = 1;
			}
			break;
		}

	if (!found)
		return 1;

	void *d = buf->data;

	if (id == NVRM_IOCTL_CREATE)
		handle_nvrm_ioctl_create(d);
	else if (id == NVRM_IOCTL_DESTROY)
		handle_nvrm_ioctl_destroy(d);
	else if (id == NVRM_IOCTL_CALL)
		handle_nvrm_ioctl_call(d);
	else if (id == NVRM_IOCTL_CREATE_DMA)
		handle_nvrm_ioctl_create_dma(d);

	return 0;
}

int demmt_nv_ioctl_post(uint32_t id, uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *buf, void *state)
{
	int k, found = 0;
	for (k = 0; k < ARRAY_SIZE(ioctls); ++k)
		if (ioctls[k].id == id)
		{
			if (ioctls[k].size == buf->len)
			{
				mmt_log("%s post, ", ioctls[k].name);
				void (*fun)(void *) = ioctls[k].fun;
				fun(buf->data);
				found = 1;
			}
			break;
		}

	if (!found)
		return 1;

	void *d = buf->data;

	if (id == NVRM_IOCTL_CREATE_VSPACE)
		handle_nvrm_ioctl_create_vspace(d);
	else if (id == NVRM_IOCTL_HOST_MAP)
		handle_nvrm_ioctl_host_map(d);
	else if (id == NVRM_IOCTL_VSPACE_MAP)
		handle_nvrm_ioctl_vspace_map(d);
	else if (id == NVRM_IOCTL_VSPACE_UNMAP)
		handle_nvrm_ioctl_vspace_unmap(d);
	else if (id == NVRM_IOCTL_HOST_UNMAP)
		handle_nvrm_ioctl_host_unmap(d);
	else if (id == NVRM_IOCTL_BIND)
		handle_nvrm_ioctl_bind(d);

	return 0;
}

void demmt_nv_memory_dump(struct mmt_nvidia_memory_dump *d, void *state)
{
}

void demmt_nv_memory_dump_cont(struct mmt_buf *b, void *state)
{
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
