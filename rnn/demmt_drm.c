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
#include "demmt_drm.h"
#include "demmt.h"
#include <drm.h>
#include <nouveau_drm.h>
#include "util.h"

static void dump_drm_nouveau_gem_info(struct drm_nouveau_gem_info *info)
{
	uint32_t domain = info->domain;
	int dprinted = 0;

	mmt_log_cont("handle: %d, domain: ", info->handle);
	if (info->domain & NOUVEAU_GEM_DOMAIN_CPU)
	{
		mmt_log_cont("%s", "CPU");
		domain &= ~NOUVEAU_GEM_DOMAIN_CPU;
		dprinted = 1;
	}
	if (info->domain & NOUVEAU_GEM_DOMAIN_VRAM)
	{
		mmt_log_cont("%s%s", dprinted ? ", " : "", "VRAM");
		domain &= ~NOUVEAU_GEM_DOMAIN_VRAM;
		dprinted = 1;
	}
	if (info->domain & NOUVEAU_GEM_DOMAIN_GART)
	{
		mmt_log_cont("%s%s", dprinted ? ", " : "", "GART");
		domain &= ~NOUVEAU_GEM_DOMAIN_GART;
		dprinted = 1;
	}
	if (info->domain & NOUVEAU_GEM_DOMAIN_MAPPABLE)
	{
		mmt_log_cont("%s%s", dprinted ? ", " : "", "MAPPABLE");
		domain &= ~NOUVEAU_GEM_DOMAIN_MAPPABLE;
		dprinted = 1;
	}
	if (domain)
		mmt_log_cont("%sUNK%x", dprinted ? ", " : "", domain);

	mmt_log_cont(" (0x%x), size: %ld, offset: %ld, map_handle: %ld, tile_mode: 0x%x, tile_flags: 0x%x",
			info->domain, info->size, info->offset,
			info->map_handle, info->tile_mode, info->tile_flags);
}

static void dump_drm_nouveau_gem_new(struct drm_nouveau_gem_new *g)
{
	dump_drm_nouveau_gem_info(&g->info);
	mmt_log_cont(", channel_hint: %d, align: 0x%x\n", g->channel_hint, g->align);
}

static char *nouveau_param_names[] = {
		"?",
		"?",
		"?",
		"PCI_VENDOR",
		"PCI_DEVICE",
		"BUS_TYPE",
		"FB_PHYSICAL ",
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

int demmt_drm_ioctl_pre(uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *buf, void *state)
{
	void *ioctl_data = buf->data;

	if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GETPARAM)
	{
		struct drm_nouveau_getparam *data = ioctl_data;

		if (0) // -> post
			mmt_log("DRM_NOUVEAU_GETPARAM, param: %s (0x%lx)\n",
					data->param < ARRAY_SIZE(nouveau_param_names) ? nouveau_param_names[data->param] : "???",
					data->param);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_SETPARAM)
	{
		struct drm_nouveau_setparam *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_SETPARAM, param: %s (0x%lx), value: 0x%lx\n",
				data->param < ARRAY_SIZE(nouveau_param_names) ? nouveau_param_names[data->param] : "???",
				data->param, data->value);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_CHANNEL_ALLOC)
	{
		struct drm_nouveau_channel_alloc *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_CHANNEL_ALLOC pre,  fb_ctxdma: 0x%0x, tt_ctxdma: 0x%0x\n",
				data->fb_ctxdma_handle, data->tt_ctxdma_handle);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_CHANNEL_FREE)
	{
		struct drm_nouveau_channel_free *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_CHANNEL_FREE, channel: %d\n", data->channel);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GROBJ_ALLOC)
	{
		struct drm_nouveau_grobj_alloc *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_GROBJ_ALLOC, channel: %d, handle: 0x%x, class: 0x%x\n",
				data->channel, data->handle, data->class);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_NOTIFIEROBJ_ALLOC)
	{
		struct drm_nouveau_notifierobj_alloc *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_NOTIFIEROBJ_ALLOC pre,  channel: %d, handle: 0x%0x, size: %d\n",
				data->channel, data->handle, data->size);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GPUOBJ_FREE)
	{
		struct drm_nouveau_gpuobj_free *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_GPUOBJ_FREE, channel: %d, handle: 0x%0x\n",
				data->channel, data->handle);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_NEW)
	{
		struct drm_nouveau_gem_new *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_GEM_NEW pre,  %s", "");
		dump_drm_nouveau_gem_new(data);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_PUSHBUF)
	{
		struct drm_nouveau_gem_pushbuf *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_GEM_PUSHBUF pre,  channel: %d, nr_buffers: %d, buffers: 0x%lx, nr_relocs: %d, relocs: 0x%lx, nr_push: %d, push: 0x%lx, suffix0: 0x%x, suffix1: 0x%x, vram_available: %lu, gart_available: %lu\n",
				data->channel, data->nr_buffers, data->buffers, data->nr_relocs, data->relocs,
				data->nr_push, data->push, data->suffix0, data->suffix1, data->vram_available,
				data->gart_available);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_CPU_PREP)
	{
		struct drm_nouveau_gem_cpu_prep *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_GEM_CPU_PREP, handle: %d, flags: 0x%0x\n", data->handle, data->flags);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_CPU_FINI)
	{
		struct drm_nouveau_gem_cpu_fini *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_GEM_CPU_FINI, handle: %d\n", data->handle);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_INFO)
	{
		struct drm_nouveau_gem_info *data = ioctl_data;
		mmt_log("DRM_NOUVEAU_GEM_INFO pre,  handle: %d\n", data->handle);
	}
	else if (nr == 0x00)
	{
		if (0) // -> post
			mmt_log("DRM_IOCTL_VERSION%s\n", "");
	}
	else if (nr == 0x02)
	{
		if (0) // -> post
			mmt_log("DRM_IOCTL_GET_MAGIC%s\n", "");
	}
	else if (nr == 0x09)
	{
		struct drm_gem_close *data = ioctl_data;
		mmt_log("DRM_IOCTL_GEM_CLOSE, handle: %d\n", data->handle);
	}
	else if (nr == 0x0b)
	{
		struct drm_gem_open *data = ioctl_data;

		mmt_log("DRM_IOCTL_GEM_OPEN pre,  name: %d\n", data->name);
	}
	else if (nr == 0x0c)
	{
		struct drm_get_cap *data = ioctl_data;

		if (0) // -> post
			mmt_log("DRM_IOCTL_GET_CAP, capability: %llu\n", data->capability);
	}
	else
	{
		mmt_log("unknown drm ioctl %x\n", nr);
		return 1;
	}

	return 0;
}

int demmt_drm_ioctl_post(uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *buf, void *state)
{
	void *ioctl_data = buf->data;

	int i;
	if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GETPARAM)
	{
		struct drm_nouveau_getparam *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_GETPARAM, param: %s (0x%lx), value: 0x%lx\n",
				data->param < ARRAY_SIZE(nouveau_param_names) ? nouveau_param_names[data->param] : "???",
				data->param, data->value);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_SETPARAM)
	{
		struct drm_nouveau_setparam *data = ioctl_data;

		if (0) // -> pre
			mmt_log("DRM_NOUVEAU_SETPARAM, param: %s (0x%lx), value: 0x%lx\n",
					data->param < ARRAY_SIZE(nouveau_param_names) ? nouveau_param_names[data->param] : "???",
					data->param, data->value);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_CHANNEL_ALLOC)
	{
		struct drm_nouveau_channel_alloc *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_CHANNEL_ALLOC post, fb_ctxdma: 0x%0x, tt_ctxdma: 0x%0x, channel: %d, pushbuf_domains: %d, notifier: 0x%0x, nr_subchan: %d",
				data->fb_ctxdma_handle, data->tt_ctxdma_handle, data->channel,
				data->pushbuf_domains, data->notifier_handle, data->nr_subchan);
		for (i = 0; i < 8; ++i)
			if (data->subchan[i].handle || data->subchan[i].grclass)
				mmt_log_cont(" subchan[%d]=<h:0x%0x, c:0x%0x>", i, data->subchan[i].handle, data->subchan[i].grclass);
		mmt_log_cont("%s\n", "");
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_CHANNEL_FREE)
	{
		struct drm_nouveau_channel_free *data = ioctl_data;

		if (0) // -> pre
			mmt_log("DRM_NOUVEAU_CHANNEL_FREE, channel: %d\n", data->channel);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GROBJ_ALLOC)
	{
		struct drm_nouveau_grobj_alloc *data = ioctl_data;

		if (0) // -> pre
			mmt_log("DRM_NOUVEAU_GROBJ_ALLOC, channel: %d, handle: 0x%x, class: 0x%x\n",
					data->channel, data->handle, data->class);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_NOTIFIEROBJ_ALLOC)
	{
		struct drm_nouveau_notifierobj_alloc *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_NOTIFIEROBJ_ALLOC post, channel: %d, handle: 0x%0x, size: %d, offset: %d\n",
				data->channel, data->handle, data->size, data->offset);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GPUOBJ_FREE)
	{
		struct drm_nouveau_gpuobj_free *data = ioctl_data;

		if (0) // -> pre
			mmt_log("DRM_NOUVEAU_GPUOBJ_FREE, channel: %d, handle: 0x%0x\n",
					data->channel, data->handle);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_NEW)
	{
		struct drm_nouveau_gem_new *g = ioctl_data;

		mmt_log("DRM_NOUVEAU_GEM_NEW post, %s", "");
		dump_drm_nouveau_gem_new(g);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_PUSHBUF)
	{
		struct drm_nouveau_gem_pushbuf *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_GEM_PUSHBUF post, channel: %d, nr_buffers: %d, buffers: 0x%lx, nr_relocs: %d, relocs: 0x%lx, nr_push: %d, push: 0x%lx, suffix0: 0x%x, suffix1: 0x%x, vram_available: %lu, gart_available: %lu\n",
				data->channel, data->nr_buffers, data->buffers, data->nr_relocs, data->relocs,
				data->nr_push, data->push, data->suffix0, data->suffix1, data->vram_available,
				data->gart_available);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_CPU_PREP)
	{
		struct drm_nouveau_gem_cpu_prep *data = ioctl_data;

		if (0) // -> pre
			mmt_log("DRM_NOUVEAU_GEM_CPU_PREP, handle: %d, flags: 0x%0x\n",
					data->handle, data->flags);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_CPU_FINI)
	{
		struct drm_nouveau_gem_cpu_fini *data = ioctl_data;

		if (0) // -> pre
			mmt_log("DRM_NOUVEAU_GEM_CPU_FINI, handle: %d\n", data->handle);
	}
	else if (nr == DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_INFO)
	{
		struct drm_nouveau_gem_info *data = ioctl_data;

		mmt_log("DRM_NOUVEAU_GEM_INFO post, %s", "");
		dump_drm_nouveau_gem_info(data);
		mmt_log_cont("%s\n", "");
	}
	else if (nr == 0x00)
	{
		struct drm_version *data = ioctl_data;

		mmt_log("DRM_IOCTL_VERSION, version: %d.%d.%d, name_addr: %p, name_len: %zd, date_addr: %p, date_len: %zd, desc_addr: %p, desc_len: %zd\n",
				data->version_major, data->version_minor, data->version_patchlevel,
				data->name, data->name_len, data->date, data->date_len,
				data->desc, data->desc_len);
	}
	else if (nr == 0x02)
	{
		struct drm_auth *data = ioctl_data;

		mmt_log("DRM_IOCTL_GET_MAGIC, magic: 0x%08x\n", data->magic);
	}
	else if (nr == 0x09)
	{
		struct drm_gem_close *data = ioctl_data;

		if (0) // -> pre
			mmt_log("DRM_IOCTL_GEM_CLOSE, handle: %d\n", data->handle);
	}
	else if (nr == 0x0b)
	{
		struct drm_gem_open *data = ioctl_data;

		mmt_log("DRM_IOCTL_GEM_OPEN post, name: %d, handle: %d, size: %llu\n",
				data->name, data->handle, data->size);
	}
	else if (nr == 0x0c)
	{
		struct drm_get_cap *data = ioctl_data;

		mmt_log("DRM_IOCTL_GET_CAP, capability: %llu, value: %llu\n",
				data->capability, data->value);
	}
	else
	{
		mmt_log("unknown drm ioctl %x\n", nr);
		return 1;
	}

	return 0;
}
#endif
