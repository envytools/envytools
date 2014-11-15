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

#ifdef LIBDRM_AVAILABLE
#include <stddef.h>
#include <stdint.h>
#include <drm.h>
#include <nouveau_drm.h>
#include <string.h>

#include "buffer.h"
#include "config.h"
#include "drm.h"
#include "log.h"
#include "nvrm.h"
#include "nvrm_object.xml.h"
#include "pushbuf.h"
#include "util.h"

#define MAX_GEM_BUFFERS 1024

static void dump_drm_nouveau_gem_info(struct drm_nouveau_gem_info *info)
{
	uint32_t domain = info->domain;
	int dprinted = 0;

	mmt_log_cont("handle: %s%3d%s, domain: %s", colors->num, info->handle,
			colors->reset, colors->eval);
	int len = 0;
	if (info->domain & NOUVEAU_GEM_DOMAIN_CPU)
	{
		mmt_log_cont("%s", "CPU");
		domain &= ~NOUVEAU_GEM_DOMAIN_CPU;
		dprinted = 1;
		len += 3;
	}
	if (info->domain & NOUVEAU_GEM_DOMAIN_VRAM)
	{
		mmt_log_cont("%s%s", dprinted ? ", " : "", "VRAM");
		domain &= ~NOUVEAU_GEM_DOMAIN_VRAM;
		len += 4 + (dprinted ? 2 : 0);
		dprinted = 1;
	}
	if (info->domain & NOUVEAU_GEM_DOMAIN_GART)
	{
		mmt_log_cont("%s%s", dprinted ? ", " : "", "GART");
		domain &= ~NOUVEAU_GEM_DOMAIN_GART;
		len += 4 + (dprinted ? 2 : 0);
		dprinted = 1;
	}
	if (info->domain & NOUVEAU_GEM_DOMAIN_MAPPABLE)
	{
		mmt_log_cont("%s%s", dprinted ? ", " : "", "MAPPABLE");
		domain &= ~NOUVEAU_GEM_DOMAIN_MAPPABLE;
		len += 8 + (dprinted ? 2 : 0);
		dprinted = 1;
	}
	if (domain)
	{
		mmt_log_cont("%sUNK%x", dprinted ? ", " : "", domain);
		len += 4 + (dprinted ? 2 : 0);
	}

	while (len++ < 14)
		mmt_log_cont("%s", " ");

	mmt_log_cont("%s (0x%x), size: %s0x%-8lx%s, gpu_start: %s0x%-8lx%s, mmap_offset: %s0x%-10lx%s, tile_mode: %s0x%02x%s, tile_flags: %s0x%04x%s",
			colors->reset, info->domain, colors->num, info->size, colors->reset,
			colors->mem, info->offset, colors->reset, colors->num, info->map_handle,
			colors->reset, colors->iname, info->tile_mode, colors->reset,
			colors->iname, info->tile_flags, colors->reset);
}

static void dump_drm_nouveau_gem_new(struct drm_nouveau_gem_new *g)
{
	dump_drm_nouveau_gem_info(&g->info);
	mmt_log_cont(", channel_hint: %s%d%s, align: %s0x%06x%s\n", colors->num,
			g->channel_hint, colors->reset, colors->num, g->align, colors->reset);
}

static char *nouveau_param_names[] = {
		"?",
		"?",
		"?",
		"PCI_VENDOR",
		"PCI_DEVICE",
		"BUS_TYPE",
		"FB_PHYSICAL",
		"AGP_PHYSICAL",
		"FB_SIZE",
		"AGP_SIZE",
		"PCI_PHYSICAL",
		"CHIPSET_ID",
		"VM_VRAM_BASE",
		"GRAPH_UNITS",
		"PTIMER_TIME",
		"HAS_BO_USAGE",
		"HAS_PAGEFLIP",
};

int demmt_drm_ioctl_pre(uint32_t fd, uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *buf, void *state)
{
	void *ioctl_data = buf->data;

	if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GETPARAM)
	{
		struct drm_nouveau_getparam *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data) // -> post
			mmt_log("%sDRM_NOUVEAU_GETPARAM%s, param: %s%14s%s (0x%lx)\n",
					colors->rname, colors->reset, colors->eval,
					data->param < ARRAY_SIZE(nouveau_param_names) ? nouveau_param_names[data->param] : "???",
					colors->reset, data->param);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_SETPARAM)
	{
		struct drm_nouveau_setparam *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_SETPARAM%s, param: %s (0x%lx), value: 0x%lx\n",
					colors->rname, colors->reset,
					data->param < ARRAY_SIZE(nouveau_param_names) ? nouveau_param_names[data->param] : "???",
					data->param, data->value);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_CHANNEL_ALLOC)
	{
		struct drm_nouveau_channel_alloc *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_CHANNEL_ALLOC%s pre,  fb_ctxdma: 0x%0x, tt_ctxdma: 0x%0x\n",
					colors->rname, colors->reset, data->fb_ctxdma_handle, data->tt_ctxdma_handle);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_CHANNEL_FREE)
	{
		struct drm_nouveau_channel_free *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_CHANNEL_FREE%s, channel: %d\n", colors->rname,
					colors->reset, data->channel);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GROBJ_ALLOC)
	{
		struct drm_nouveau_grobj_alloc *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GROBJ_ALLOC%s, channel: %d, handle: %s0x%x%s, class: %s0x%x%s\n",
					colors->rname, colors->reset, data->channel, colors->num,
					data->handle, colors->reset, colors->eval, data->class, colors->reset);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_NOTIFIEROBJ_ALLOC)
	{
		struct drm_nouveau_notifierobj_alloc *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_NOTIFIEROBJ_ALLOC%s pre,  channel: %d, handle: 0x%0x, size: %d\n",
					colors->rname, colors->reset, data->channel, data->handle, data->size);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GPUOBJ_FREE)
	{
		struct drm_nouveau_gpuobj_free *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GPUOBJ_FREE%s, channel: %d, handle: 0x%0x\n",
					colors->rname, colors->reset, data->channel, data->handle);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_NEW)
	{
		struct drm_nouveau_gem_new *data = ioctl_data;

		if (dump_decoded_ioctl_data)
		{
			mmt_log("%sDRM_NOUVEAU_GEM_NEW%s pre,  ", colors->rname, colors->reset);
			dump_drm_nouveau_gem_new(data);
		}
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_PUSHBUF)
	{
		struct drm_nouveau_gem_pushbuf *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GEM_PUSHBUF%s pre,  channel: %d, nr_buffers: %s%d%s, buffers: 0x%lx, nr_relocs: %s%d%s, relocs: 0x%lx, nr_push: %s%d%s, push: 0x%lx, suffix0: 0x%x, suffix1: 0x%x, vram_available: %lu, gart_available: %lu\n",
					colors->rname, colors->reset, data->channel, colors->num,
					data->nr_buffers, colors->reset, data->buffers, colors->num,
					data->nr_relocs, colors->reset,data->relocs, colors->num,
					data->nr_push, colors->reset, data->push, data->suffix0,
					data->suffix1, data->vram_available, data->gart_available);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_CPU_PREP)
	{
		struct drm_nouveau_gem_cpu_prep *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GEM_CPU_PREP%s, handle: %s%3d%s, flags: %s0x%0x%s\n",
					colors->rname, colors->reset, colors->num, data->handle,
					colors->reset, colors->eval, data->flags, colors->reset);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_CPU_FINI)
	{
		struct drm_nouveau_gem_cpu_fini *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GEM_CPU_FINI%s, handle: %d\n", colors->rname,
					colors->reset, data->handle);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_INFO)
	{
		struct drm_nouveau_gem_info *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GEM_INFO%s pre,  handle: %s%3d%s\n", colors->rname,
					colors->reset, colors->num, data->handle, colors->reset);
	}
	else if (nr == 0x00)
	{
		if (0 && dump_decoded_ioctl_data) // -> post
			mmt_log("%sDRM_IOCTL_VERSION%s\n", colors->rname, colors->reset);
	}
	else if (nr == 0x02)
	{
		if (0 && dump_decoded_ioctl_data) // -> post
			mmt_log("%sDRM_IOCTL_GET_MAGIC%s\n", colors->rname, colors->reset);
	}
	else if (nr == 0x09)
	{
		struct drm_gem_close *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_GEM_CLOSE%s, handle: %s%3d%s\n", colors->rname,
					colors->reset, colors->num, data->handle, colors->reset);
	}
	else if (nr == 0x0b)
	{
		struct drm_gem_open *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_GEM_OPEN%s pre,  name: %s%d%s\n", colors->rname,
					colors->reset, colors->eval, data->name, colors->reset);
	}
	else if (nr == 0x0c)
	{
		struct drm_get_cap *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data) // -> post
			mmt_log("%sDRM_IOCTL_GET_CAP%s, capability: %llu\n", colors->rname,
					colors->reset, data->capability);
	}
	else
	{
		mmt_log("%sunknown drm ioctl%s %x\n", colors->err, colors->reset, nr);
		return 1;
	}

	return 0;
}

int demmt_drm_ioctl_post(uint32_t fd, uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *buf, void *state)
{
	static int nouveau_chipset; // hack

	void *ioctl_data = buf->data;

	int i;
	if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GETPARAM)
	{
		struct drm_nouveau_getparam *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GETPARAM%s, param: %s%14s%s (0x%lx), value: %s0x%lx%s\n",
					colors->rname, colors->reset, colors->eval,
					data->param < ARRAY_SIZE(nouveau_param_names) ? nouveau_param_names[data->param] : "???",
					colors->reset, data->param, colors->num, data->value, colors->reset);
		if (data->param == NOUVEAU_GETPARAM_CHIPSET_ID)
			nouveau_chipset = data->value;
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_SETPARAM)
	{
		struct drm_nouveau_setparam *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data) // -> pre
			mmt_log("%sDRM_NOUVEAU_SETPARAM%s, param: %s (0x%lx), value: 0x%lx\n",
					colors->rname, colors->reset,
					data->param < ARRAY_SIZE(nouveau_param_names) ? nouveau_param_names[data->param] : "???",
					data->param, data->value);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_CHANNEL_ALLOC)
	{
		struct drm_nouveau_channel_alloc *data = ioctl_data;

		if (dump_decoded_ioctl_data)
		{
			mmt_log("%sDRM_NOUVEAU_CHANNEL_ALLOC%s post, fb_ctxdma: 0x%0x, tt_ctxdma: 0x%0x, channel: %d, pushbuf_domains: %d, notifier: 0x%0x, nr_subchan: %d",
					colors->rname, colors->reset, data->fb_ctxdma_handle,
					data->tt_ctxdma_handle, data->channel, data->pushbuf_domains,
					data->notifier_handle, data->nr_subchan);
			for (i = 0; i < 8; ++i)
				if (data->subchan[i].handle || data->subchan[i].grclass)
					mmt_log_cont(" subchan[%d]=<h:0x%0x, c:0x%0x>", i, data->subchan[i].handle, data->subchan[i].grclass);
			mmt_log_cont("%s\n", "");
		}
		// hack, fake device
		struct gpu_object *dev = gpu_object_add(fd, data->channel, data->channel, data->channel, NVRM_DEVICE_0);
		nvrm_device_set_chipset(dev, nouveau_chipset);

		// hack, fake fifo
		struct gpu_object *fifo = gpu_object_add(fd, data->channel, data->channel, 0xf1f0eeee, NVRM_FIFO_IB_G80);
		get_fifo_state(fifo);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_CHANNEL_FREE)
	{
		struct drm_nouveau_channel_free *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data) // -> pre
			mmt_log("%sDRM_NOUVEAU_CHANNEL_FREE%s, channel: %d\n", colors->rname,
					colors->reset, data->channel);

		gpu_object_destroy(gpu_object_find(data->channel, data->channel));
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GROBJ_ALLOC)
	{
		struct drm_nouveau_grobj_alloc *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data) // -> pre
			mmt_log("%sDRM_NOUVEAU_GROBJ_ALLOC%s, channel: %d, handle: %s0x%x%s, class: %s0x%x%s\n",
					colors->rname, colors->reset, data->channel, colors->num,
					data->handle, colors->reset, colors->eval, data->class,
					colors->reset);

		struct gpu_object *gpu_obj = gpu_object_add(fd, data->channel, 0xf1f0eeee, data->handle, data->class);
		pushbuf_add_object(data->class, data->class, gpu_obj); // yes, class x2
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_NOTIFIEROBJ_ALLOC)
	{
		struct drm_nouveau_notifierobj_alloc *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_NOTIFIEROBJ_ALLOC%s post, channel: %d, handle: 0x%0x, size: %d, offset: %d\n",
					colors->rname, colors->reset, data->channel, data->handle,
					data->size, data->offset);
		gpu_object_add(fd, data->channel, data->channel, data->handle, 0);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GPUOBJ_FREE)
	{
		struct drm_nouveau_gpuobj_free *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data) // -> pre
			mmt_log("%sDRM_NOUVEAU_GPUOBJ_FREE%s, channel: %d, handle: 0x%0x\n",
					colors->rname, colors->reset, data->channel, data->handle);

		gpu_object_destroy(gpu_object_find(0, data->handle));
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_NEW)
	{
		struct drm_nouveau_gem_new *g = ioctl_data;

		if (dump_decoded_ioctl_data)
		{
			mmt_log("%sDRM_NOUVEAU_GEM_NEW%s post, ", colors->rname, colors->reset);
			dump_drm_nouveau_gem_new(g);
		}

		struct gpu_object *obj = gpu_object_add(fd, g->channel_hint, g->channel_hint, g->info.handle, 0);
		obj->length = g->info.size;
		obj->data = realloc(obj->data, obj->length);
		memset(obj->data, 0, obj->length);

		struct gpu_mapping *gmapping = calloc(sizeof(struct gpu_mapping), 1);
		gmapping->fd = fd;
		gmapping->address = g->info.offset;
		gmapping->length = g->info.size;
		gmapping->object = obj;
		gmapping->next = obj->gpu_mappings;
		obj->gpu_mappings = gmapping;

		struct cpu_mapping *cmapping = calloc(sizeof(struct cpu_mapping), 1);
		cmapping->fd = fd;
		cmapping->mmap_offset = g->info.map_handle;
		cmapping->length = g->info.size;
		cmapping->data = obj->data;
		cmapping->object = obj;
		cmapping->next = obj->cpu_mappings;
		obj->cpu_mappings = cmapping;
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_PUSHBUF)
	{
		struct drm_nouveau_gem_pushbuf *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GEM_PUSHBUF%s post, channel: %d, nr_buffers: %s%d%s, buffers: 0x%lx, nr_relocs: %s%d%s, relocs: 0x%lx, nr_push: %s%d%s, push: 0x%lx, suffix0: 0x%x, suffix1: 0x%x, vram_available: %lu, gart_available: %lu\n",
					colors->rname, colors->reset, data->channel, colors->num,
					data->nr_buffers, colors->reset, data->buffers, colors->num,
					data->nr_relocs, colors->reset,data->relocs, colors->num,
					data->nr_push, colors->reset, data->push, data->suffix0,
					data->suffix1, data->vram_available, data->gart_available);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_CPU_PREP)
	{
		struct drm_nouveau_gem_cpu_prep *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data) // -> pre
			mmt_log("%sDRM_NOUVEAU_GEM_CPU_PREP%s, handle: %d, flags: 0x%0x\n",
					colors->rname, colors->reset, data->handle, data->flags);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_CPU_FINI)
	{
		struct drm_nouveau_gem_cpu_fini *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data) // -> pre
			mmt_log("%sDRM_NOUVEAU_GEM_CPU_FINI%s, handle: %d\n", colors->rname,
					colors->reset, data->handle);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_INFO)
	{
		struct drm_nouveau_gem_info *data = ioctl_data;

		if (dump_decoded_ioctl_data)
		{
			mmt_log("%sDRM_NOUVEAU_GEM_INFO%s post, ", colors->rname, colors->reset);
			dump_drm_nouveau_gem_info(data);
			mmt_log_cont("%s\n", "");
		}
	}
	else if (nr == 0x00)
	{
		struct drm_version *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_VERSION%s, version: %s%d.%d.%d%s, name_addr: %p, name_len: %zd, date_addr: %p, date_len: %zd, desc_addr: %p, desc_len: %zd\n",
					colors->rname, colors->reset, colors->eval, data->version_major,
					data->version_minor, data->version_patchlevel, colors->reset,
					data->name, data->name_len, data->date, data->date_len,
					data->desc, data->desc_len);
	}
	else if (nr == 0x02)
	{
		struct drm_auth *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_GET_MAGIC%s, magic: 0x%08x\n", colors->rname,
					colors->reset, data->magic);
	}
	else if (nr == 0x09)
	{
		struct drm_gem_close *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data) // -> pre
			mmt_log("%sDRM_IOCTL_GEM_CLOSE%s, handle: %d\n", colors->rname,
					colors->reset, data->handle);
	}
	else if (nr == 0x0b)
	{
		struct drm_gem_open *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_GEM_OPEN%s post, name: %s%d%s, handle: %s%d%s, size: %llu\n",
					colors->rname, colors->reset, colors->eval, data->name, colors->reset,
					colors->num, data->handle, colors->reset, data->size);
	}
	else if (nr == 0x0c)
	{
		struct drm_get_cap *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_GET_CAP%s, capability: %s%llu%s, value: %s%llu%s\n",
					colors->rname, colors->reset, colors->iname, data->capability,
					colors->reset, colors->num, data->value, colors->reset);
	}
	else
	{
		mmt_log("%sunknown drm ioctl%s %x\n", colors->err, colors->reset, nr);
		return 1;
	}

	return 0;
}

void demmt_nouveau_gem_pushbuf_data(struct mmt_nouveau_pushbuf_data *data, void *state)
{
	struct mmt_buf *buffers_mmt_buf = (void *)&data->data.data;
	struct drm_nouveau_gem_pushbuf_bo *buffers = (void *)&buffers_mmt_buf->data;
	struct mmt_buf *push_mmt_buf = ((void *)buffers) + buffers_mmt_buf->len;
	struct drm_nouveau_gem_pushbuf_push *push = (void *)&push_mmt_buf->data;
	struct mmt_buf *relocs_mmt_buf = ((void *)push) + push_mmt_buf->len;
	struct drm_nouveau_gem_pushbuf_reloc *relocs = (void *)&relocs_mmt_buf->data;

	int nr_buffers = buffers_mmt_buf->len / sizeof(buffers[0]);
	int nr_push = push_mmt_buf->len / sizeof(push[0]);
	int nr_relocs = relocs_mmt_buf->len / sizeof(relocs[0]);
	int i;

	if (dump_decoded_ioctl_data)
	{
		mmt_log("%sDRM_NOUVEAU_GEM_PUSHBUF%s data, nr_buffers: %s%d%s, nr_relocs: %s%d%s, nr_push: %s%d%s\n",
				colors->rname, colors->reset, colors->num, nr_buffers, colors->reset,
				colors->num, nr_relocs, colors->reset, colors->num, nr_push, colors->reset);
		for (i = 0; i < nr_buffers; ++i)
			mmt_log("buffer[%d]: handle: %s%d%s, read_domains: 0x%x, write_domains: 0x%x, valid_domains: 0x%x, presumed.valid: %d, presumed.domain: 0x%x, presumed.gpu_start: %s0x%lx%s\n",
					i, colors->num, buffers[i].handle, colors->reset, buffers[i].read_domains,
					buffers[i].write_domains, buffers[i].valid_domains,
					buffers[i].presumed.valid, buffers[i].presumed.domain, colors->eval,
					buffers[i].presumed.offset, colors->reset);
		for (i = 0; i < nr_relocs; ++i)
			mmt_log("relocs[%d]: reloc_bo_index: %d, reloc_bo_offset: %d, bo_index: %d, flags: 0x%x, data: 0x%x, vor: 0x%x, tor: 0x%x\n",
					i, relocs[i].reloc_bo_index, relocs[i].reloc_bo_offset,
					relocs[i].bo_index, relocs[i].flags, relocs[i].data,
					relocs[i].vor, relocs[i].tor);
		for (i = 0; i < nr_push; ++i)
			mmt_log("push[%d]: bo_index: %d, offset: %s0x%lx%s, length: %s0x%lx%s\n",
					i, push[i].bo_index, colors->num, push[i].offset, colors->reset,
					colors->num, push[i].length, colors->reset);
	}

	struct pushbuf_decode_state pstate;
	pushbuf_decode_start(&pstate);
	pstate.fifo = gpu_object_find(0, 0xf1f0eeee); // hack
	struct gpu_object *dev = nvrm_get_device(pstate.fifo);

	for (i = 0; i < nr_push; ++i)
	{
		uint64_t gpu_start = buffers[push[i].bo_index].presumed.offset;

		struct gpu_mapping *gmapping = gpu_mapping_find(gpu_start, dev);
		if (gmapping)
			pushbuf_print(&pstate, gmapping, gpu_start + push[i].offset, push[i].length / 4);
		else
			mmt_error("couldn't find buffer 0x%lx\n", gpu_start);
	}

	pushbuf_decode_end(&pstate);
}
#endif
