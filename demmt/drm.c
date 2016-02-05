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
#include "decode_utils.h"
#include "drm.h"
#include "log.h"
#include "nvrm.h"
#include "nvrm_object.xml.h"
#include "pushbuf.h"
#include "util.h"

#define MAX_GEM_BUFFERS 1024

static void decode_domain(uint32_t domain_, int space)
{
	uint32_t domain = domain_;
	int dprinted = 0;

	mmt_log_cont("%s", colors->eval);

	int len = 0;
	if (!domain)
	{
		mmt_log_cont("%s", "NONE");
		dprinted = 1;
		len += 4;
	}

	if (domain & NOUVEAU_GEM_DOMAIN_CPU)
	{
		mmt_log_cont("%s", "CPU");
		domain &= ~NOUVEAU_GEM_DOMAIN_CPU;
		dprinted = 1;
		len += 3;
	}
	if (domain & NOUVEAU_GEM_DOMAIN_VRAM)
	{
		mmt_log_cont("%s%s", dprinted ? ", " : "", "VRAM");
		domain &= ~NOUVEAU_GEM_DOMAIN_VRAM;
		len += 4 + (dprinted ? 2 : 0);
		dprinted = 1;
	}
	if (domain & NOUVEAU_GEM_DOMAIN_GART)
	{
		mmt_log_cont("%s%s", dprinted ? ", " : "", "GART");
		domain &= ~NOUVEAU_GEM_DOMAIN_GART;
		len += 4 + (dprinted ? 2 : 0);
		dprinted = 1;
	}
	if (domain & NOUVEAU_GEM_DOMAIN_MAPPABLE)
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
	mmt_log_cont("%s", colors->reset);

	while (len++ < space)
		mmt_log_cont("%s", " ");
	mmt_log_cont(" (0x%x)", domain_);
}

static void dump_drm_nouveau_gem_info(struct drm_nouveau_gem_info *info)
{
	mmt_log_cont("handle: %s%3d%s, domain: ", colors->num, info->handle,
			colors->reset);

	decode_domain(info->domain, 14);

	mmt_log_cont(", size: %s0x%-8" PRIx64 "%s, gpu_start: %s0x%-8" PRIx64 "%s, mmap_offset: %s0x%-10" PRIx64 "%s, tile_mode: %s0x%02x%s, tile_flags: %s0x%04x%s",
			colors->num, info->size, colors->reset,
			colors->mem, info->offset, colors->reset, colors->num, info->map_handle,
			colors->reset, colors->iname, info->tile_mode, colors->reset,
			colors->iname, info->tile_flags, colors->reset);
}

static void dump_drm_nouveau_gem_new(struct drm_nouveau_gem_new *g)
{
	dump_drm_nouveau_gem_info(&g->info);
	mmt_log_cont(", channel_hint: %s%d%s, align: %s0x%06x%s", colors->num,
			g->channel_hint, colors->reset, colors->num, g->align, colors->reset);
}

static void demmt_nouveau_decode_gem_pushbuf_data(
		int nr_buffers, struct drm_nouveau_gem_pushbuf_bo *buffers,
		int nr_push,    struct drm_nouveau_gem_pushbuf_push *push,
		int nr_relocs,  struct drm_nouveau_gem_pushbuf_reloc *relocs)
{
	int i;

	if (dump_decoded_ioctl_data)
	{
		for (i = 0; i < nr_buffers; ++i)
		{
			mmt_log("buffer[%d]: handle: %s%2d%s",
					i, colors->num, buffers[i].handle, colors->reset);
			mmt_log_cont(", read_domains: %s", "");
			decode_domain(buffers[i].read_domains, 4);
			mmt_log_cont(", write_domains: %s", "");
			decode_domain(buffers[i].write_domains, 4);
			mmt_log_cont(", valid_domains: %s", "");
			decode_domain(buffers[i].valid_domains, 4);
			mmt_log_cont(", presumed.valid: %d", buffers[i].presumed.valid);
			mmt_log_cont(", presumed.domain: %s", "");
			decode_domain(buffers[i].presumed.domain, 4);
			mmt_log_cont(", presumed.gpu_start: %s0x%" PRIx64 "%s",
					colors->eval, buffers[i].presumed.offset, colors->reset);
			mmt_log_cont_nl();
		}
		for (i = 0; i < nr_relocs; ++i)
			mmt_log("relocs[%d]: reloc_bo_index: %d, reloc_bo_offset: %d, bo_index: %d, flags: 0x%x, data: 0x%x, vor: 0x%x, tor: 0x%x\n",
					i, relocs[i].reloc_bo_index, relocs[i].reloc_bo_offset,
					relocs[i].bo_index, relocs[i].flags, relocs[i].data,
					relocs[i].vor, relocs[i].tor);
		for (i = 0; i < nr_push; ++i)
		{
			mmt_log("push[%d]: bo_index: %d, offset: %s0x%" PRIx64 "%s, length: %s0x%" PRIx64 "%s",
					i, push[i].bo_index, colors->num, push[i].offset, colors->reset,
					colors->num, push[i].length & 0x007ffffc, colors->reset);
			if (push[i].length & 1)
				mmt_log_cont(", unk8: 1%s", "");
			if (push[i].length & 2)
				mmt_log_cont(", not_main: 1%s", "");
			if (push[i].length & 0x00800000)
				mmt_log_cont(", no_prefetch: 1%s", "");
			mmt_log_cont_nl();
		}
	}

	struct pushbuf_decode_state pstate = { 0 };
	pstate.fifo = gpu_object_find(0, 0xf1f0eeee); // hack
	struct gpu_object *dev = nvrm_get_device(pstate.fifo);

	for (i = 0; i < nr_push; ++i)
	{
		uint64_t gpu_start = buffers[push[i].bo_index].presumed.offset;

		struct gpu_mapping *gmapping = gpu_mapping_find(gpu_start, dev);
		if (gmapping)
			pushbuf_print(&pstate, gmapping, gpu_start + push[i].offset, (push[i].length & 0x007ffffc) >> 2);
		else
			mmt_error("couldn't find buffer 0x%" PRIx64 "\n", gpu_start);
	}
}

void demmt_nouveau_gem_pushbuf_data(struct mmt_nouveau_pushbuf_data *data, void *state)
{
	// compat code, mmt does not generate mmt_nouveau_pushbuf_data message anymore

	int minlen = sizeof(struct mmt_buf *);
	if (data->data.len < minlen)
	{
		mmt_error("invalid mmt_nouveau_pushbuf_data %s\n", "");
		demmt_abort();
	}
	uint64_t used = 0;
	struct mmt_buf *buffers_mmt_buf = (void *)&data->data.data;
	used += buffers_mmt_buf->len + sizeof(*buffers_mmt_buf);
	if (used > data->data.len)
	{
		mmt_error("invalid mmt_nouveau_pushbuf_data %s\n", "");
		demmt_abort();
	}

	struct drm_nouveau_gem_pushbuf_bo *buffers = (void *)&buffers_mmt_buf->data;
	struct mmt_buf *push_mmt_buf = ((void *)buffers) + buffers_mmt_buf->len;
	used += push_mmt_buf->len + sizeof(*push_mmt_buf);
	if (used > data->data.len)
	{
		mmt_error("invalid mmt_nouveau_pushbuf_data %s\n", "");
		demmt_abort();
	}

	struct drm_nouveau_gem_pushbuf_push *push = (void *)&push_mmt_buf->data;
	struct mmt_buf *relocs_mmt_buf = ((void *)push) + push_mmt_buf->len;
	used += relocs_mmt_buf->len + sizeof(*relocs_mmt_buf);
	if (used > data->data.len)
	{
		mmt_error("invalid mmt_nouveau_pushbuf_data %s\n", "");
		demmt_abort();
	}

	struct drm_nouveau_gem_pushbuf_reloc *relocs = (void *)&relocs_mmt_buf->data;

	int nr_buffers = buffers_mmt_buf->len / sizeof(buffers[0]);
	int nr_push = push_mmt_buf->len / sizeof(push[0]);
	int nr_relocs = relocs_mmt_buf->len / sizeof(relocs[0]);

	if (dump_decoded_ioctl_data)
	{
		mmt_log("%sDRM_NOUVEAU_GEM_PUSHBUF%s data, nr_buffers: %s%d%s, nr_relocs: %s%d%s, nr_push: %s%d%s\n",
				colors->rname, colors->reset, colors->num, nr_buffers, colors->reset,
				colors->num, nr_relocs, colors->reset, colors->num, nr_push, colors->reset);
	}

	demmt_nouveau_decode_gem_pushbuf_data(nr_buffers, buffers, nr_push, push, nr_relocs, relocs);
}

#define _(N) #N
static char *nouveau_param_names[] = {
		"?",
		"?",
		"?",
		_(NOUVEAU_GETPARAM_PCI_VENDOR),
		_(NOUVEAU_GETPARAM_PCI_DEVICE),
		_(NOUVEAU_GETPARAM_BUS_TYPE),
		_(NOUVEAU_GETPARAM_FB_PHYSICAL),
		_(NOUVEAU_GETPARAM_AGP_PHYSICAL),
		_(NOUVEAU_GETPARAM_FB_SIZE),
		_(NOUVEAU_GETPARAM_AGP_SIZE),
		_(NOUVEAU_GETPARAM_PCI_PHYSICAL),
		_(NOUVEAU_GETPARAM_CHIPSET_ID),
		_(NOUVEAU_GETPARAM_VM_VRAM_BASE),
		_(NOUVEAU_GETPARAM_GRAPH_UNITS),
		_(NOUVEAU_GETPARAM_PTIMER_TIME),
		_(NOUVEAU_GETPARAM_HAS_BO_USAGE),
		_(NOUVEAU_GETPARAM_HAS_PAGEFLIP),
};
#undef _

#define DRM_NOUVEAU_IOCTL_GETPARAM          DRM_IOWR(DRM_COMMAND_BASE + DRM_NOUVEAU_GETPARAM, struct drm_nouveau_getparam)
#define DRM_NOUVEAU_IOCTL_SETPARAM          DRM_IOW( DRM_COMMAND_BASE + DRM_NOUVEAU_SETPARAM, struct drm_nouveau_setparam)
#define DRM_NOUVEAU_IOCTL_CHANNEL_ALLOC     DRM_IOWR(DRM_COMMAND_BASE + DRM_NOUVEAU_CHANNEL_ALLOC, struct drm_nouveau_channel_alloc)
#define DRM_NOUVEAU_IOCTL_CHANNEL_FREE      DRM_IOW( DRM_COMMAND_BASE + DRM_NOUVEAU_CHANNEL_FREE, struct drm_nouveau_channel_free)
#define DRM_NOUVEAU_IOCTL_GROBJ_ALLOC       DRM_IOW( DRM_COMMAND_BASE + DRM_NOUVEAU_GROBJ_ALLOC, struct drm_nouveau_grobj_alloc)
#define DRM_NOUVEAU_IOCTL_NOTIFIEROBJ_ALLOC DRM_IOWR(DRM_COMMAND_BASE + DRM_NOUVEAU_NOTIFIEROBJ_ALLOC, struct drm_nouveau_notifierobj_alloc)
#define DRM_NOUVEAU_IOCTL_GPUOBJ_FREE       DRM_IOW( DRM_COMMAND_BASE + DRM_NOUVEAU_GPUOBJ_FREE, struct drm_nouveau_gpuobj_free)
#define DRM_NOUVEAU_IOCTL_GEM_NEW           DRM_IOWR(DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_NEW, struct drm_nouveau_gem_new)
#define DRM_NOUVEAU_IOCTL_GEM_PUSHBUF       DRM_IOWR(DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_PUSHBUF, struct drm_nouveau_gem_pushbuf)
#define DRM_NOUVEAU_IOCTL_GEM_CPU_PREP      DRM_IOW( DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_CPU_PREP, struct drm_nouveau_gem_cpu_prep)
#define DRM_NOUVEAU_IOCTL_GEM_CPU_FINI      DRM_IOW( DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_CPU_FINI, struct drm_nouveau_gem_cpu_fini)
#define DRM_NOUVEAU_IOCTL_GEM_INFO          DRM_IOWR(DRM_COMMAND_BASE + DRM_NOUVEAU_GEM_INFO, struct drm_nouveau_gem_info)

struct drm_version64
{
	int version_major;
	int version_minor;
	int version_patchlevel;
	int _pad;
	uint64_t name_len;
	uint64_t name;
	uint64_t date_len;
	uint64_t date;
	uint64_t desc_len;
	uint64_t desc;
};

struct drm_version32
{
	int version_major;
	int version_minor;
	int version_patchlevel;
	uint32_t name_len;
	uint32_t name;
	uint32_t date_len;
	uint32_t date;
	uint32_t desc_len;
	uint32_t desc;
};

#define DRM_IOCTL_VERSION64		DRM_IOWR(0x00, struct drm_version64)
#define DRM_IOCTL_VERSION32		DRM_IOWR(0x00, struct drm_version32)

int demmt_drm_ioctl_pre(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc)
{
	void *ioctl_data = buf->data;

	if (id == DRM_NOUVEAU_IOCTL_GETPARAM)
	{
		struct drm_nouveau_getparam *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data) // -> post
			mmt_log("%sDRM_NOUVEAU_GETPARAM%s, param: %s%14s%s (0x%" PRIx64 ")\n",
					colors->rname, colors->reset, colors->eval,
					data->param < ARRAY_SIZE(nouveau_param_names) ? nouveau_param_names[data->param] : "???",
					colors->reset, data->param);
	}
	else if (id == DRM_NOUVEAU_IOCTL_SETPARAM)
	{
		struct drm_nouveau_setparam *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_SETPARAM%s, param: %s (0x%" PRIx64 "), value: 0x%" PRIx64 "\n",
					colors->rname, colors->reset,
					data->param < ARRAY_SIZE(nouveau_param_names) ? nouveau_param_names[data->param] : "???",
					data->param, data->value);
	}
	else if (id == DRM_NOUVEAU_IOCTL_CHANNEL_ALLOC)
	{
		struct drm_nouveau_channel_alloc *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_CHANNEL_ALLOC%s pre,  fb_ctxdma: 0x%0x, tt_ctxdma: 0x%0x\n",
					colors->rname, colors->reset, data->fb_ctxdma_handle, data->tt_ctxdma_handle);
	}
	else if (id == DRM_NOUVEAU_IOCTL_CHANNEL_FREE)
	{
		struct drm_nouveau_channel_free *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_CHANNEL_FREE%s, channel: %d\n", colors->rname,
					colors->reset, data->channel);
	}
	else if (id == DRM_NOUVEAU_IOCTL_GROBJ_ALLOC)
	{
		struct drm_nouveau_grobj_alloc *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GROBJ_ALLOC%s, channel: %d, handle: %s0x%x%s, class: %s0x%x%s\n",
					colors->rname, colors->reset, data->channel, colors->num,
					data->handle, colors->reset, colors->eval, data->class, colors->reset);
	}
	else if (id == DRM_NOUVEAU_IOCTL_NOTIFIEROBJ_ALLOC)
	{
		struct drm_nouveau_notifierobj_alloc *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_NOTIFIEROBJ_ALLOC%s pre,  channel: %d, handle: 0x%0x, size: %d\n",
					colors->rname, colors->reset, data->channel, data->handle, data->size);
	}
	else if (id == DRM_NOUVEAU_IOCTL_GPUOBJ_FREE)
	{
		struct drm_nouveau_gpuobj_free *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GPUOBJ_FREE%s, channel: %d, handle: 0x%0x\n",
					colors->rname, colors->reset, data->channel, data->handle);
	}
	else if (id == DRM_NOUVEAU_IOCTL_GEM_NEW)
	{
		struct drm_nouveau_gem_new *data = ioctl_data;

		if (dump_decoded_ioctl_data)
		{
			mmt_log("%sDRM_NOUVEAU_GEM_NEW%s pre,  ", colors->rname, colors->reset);
			dump_drm_nouveau_gem_new(data);
			mmt_log_cont_nl();
		}
	}
	else if (id == DRM_NOUVEAU_IOCTL_GEM_PUSHBUF)
	{
		struct drm_nouveau_gem_pushbuf *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GEM_PUSHBUF%s pre,  channel: %d, nr_buffers: %s%d%s, buffers: 0x%" PRIx64 ", nr_relocs: %s%d%s, relocs: 0x%" PRIx64 ", nr_push: %s%d%s, push: 0x%" PRIx64 ", suffix0: 0x%x, suffix1: 0x%x, vram_available: %" PRIu64 ", gart_available: %" PRIu64 "\n",
					colors->rname, colors->reset, data->channel, colors->num,
					data->nr_buffers, colors->reset, data->buffers, colors->num,
					data->nr_relocs, colors->reset,data->relocs, colors->num,
					data->nr_push, colors->reset, data->push, data->suffix0,
					data->suffix1, data->vram_available, data->gart_available);
	}
	else if (id == DRM_NOUVEAU_IOCTL_GEM_CPU_PREP)
	{
		struct drm_nouveau_gem_cpu_prep *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GEM_CPU_PREP%s, handle: %s%3d%s, flags: %s0x%0x%s\n",
					colors->rname, colors->reset, colors->num, data->handle,
					colors->reset, colors->eval, data->flags, colors->reset);
	}
	else if (id == DRM_NOUVEAU_IOCTL_GEM_CPU_FINI)
	{
		struct drm_nouveau_gem_cpu_fini *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GEM_CPU_FINI%s, handle: %d\n", colors->rname,
					colors->reset, data->handle);
	}
	else if (id == DRM_NOUVEAU_IOCTL_GEM_INFO)
	{
		struct drm_nouveau_gem_info *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GEM_INFO%s pre,  handle: %s%3d%s\n", colors->rname,
					colors->reset, colors->num, data->handle, colors->reset);
	}
	else if (id == DRM_IOCTL_VERSION64)
	{
		if (0 && dump_decoded_ioctl_data) // -> post
			mmt_log("%sDRM_IOCTL_VERSION64%s\n", colors->rname, colors->reset);
	}
	else if (id == DRM_IOCTL_VERSION32)
	{
		if (0 && dump_decoded_ioctl_data) // -> post
			mmt_log("%sDRM_IOCTL_VERSION32%s\n", colors->rname, colors->reset);
	}
	else if (id == DRM_IOCTL_GET_MAGIC)
	{
		if (0 && dump_decoded_ioctl_data) // -> post
			mmt_log("%sDRM_IOCTL_GET_MAGIC%s\n", colors->rname, colors->reset);
	}
	else if (id == DRM_IOCTL_GEM_CLOSE)
	{
		struct drm_gem_close *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_GEM_CLOSE%s, handle: %s%3d%s\n", colors->rname,
					colors->reset, colors->num, data->handle, colors->reset);
	}
	else if (id == DRM_IOCTL_GEM_OPEN)
	{
		struct drm_gem_open *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_GEM_OPEN%s pre,  name: %s%d%s\n", colors->rname,
					colors->reset, colors->eval, data->name, colors->reset);
	}
	else if (id == DRM_IOCTL_GET_CAP)
	{
		struct drm_get_cap *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data) // -> post
			mmt_log("%sDRM_IOCTL_GET_CAP%s, capability: %llu\n", colors->rname,
					colors->reset, data->capability);
	}
	else if (id == DRM_IOCTL_SET_VERSION)
	{
		struct drm_set_version *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_SET_VERSION%s pre,  di_major: %2d, di_minor: %2d, dd_major: %2d, dd_minor: %2d\n",
					colors->rname, colors->reset, data->drm_di_major,
					data->drm_di_minor, data->drm_dd_major, data->drm_dd_minor);
	}
	else if (id == DRM_IOCTL_MODE_GETRESOURCES)
	{
		struct drm_mode_card_res *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_MODE_GETRESOURCES%s pre,  fb_id_ptr: %p, crtc_id_ptr: %p, connector_id_ptr: %p, "
					"encoder_id_ptr: %p, count_fbs: %d, count_crtcs: %d, count_connectors: %d, count_encoders: %d, "
					"min_width: %d, max_width: %4d, min_height: %d, max_height: %4d\n", colors->rname,
					colors->reset,
					(void *)(uintptr_t)data->fb_id_ptr,
					(void *)(uintptr_t)data->crtc_id_ptr,
					(void *)(uintptr_t)data->connector_id_ptr,
					(void *)(uintptr_t)data->encoder_id_ptr,
					data->count_fbs, data->count_crtcs, data->count_connectors,
					data->count_encoders, data->min_width, data->max_width,
					data->min_height, data->max_height);
	}
	else if (id == DRM_IOCTL_SET_MASTER)
	{
		if (0 && dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_SET_MASTER%s pre\n", colors->rname, colors->reset);
	}
	else if (id == DRM_IOCTL_DROP_MASTER)
	{
		if (0 && dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_DROP_MASTER%s pre\n", colors->rname, colors->reset);
	}
	else if (id == DRM_IOCTL_AUTH_MAGIC)
	{
		struct drm_auth *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_AUTH_MAGIC%s pre,  magic: 0x%x\n", colors->rname, colors->reset, data->magic);
	}
	else if (id == DRM_IOCTL_GEM_FLINK)
	{
		struct drm_gem_flink *data = ioctl_data;

		if (0 && dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_GEM_FLINK%s pre,  handle: 0x%x, name: 0x%x\n",
					colors->rname, colors->reset, data->handle, data->name);
	}
	else
	{
		mmt_log("%sunknown drm ioctl%s %x\n", colors->err, colors->reset, nr);
		return 1;
	}

	return 0;
}

static const char *ret_err(uint64_t ret, uint64_t err)
{
	static char r[100];
	r[0] = 0;

	if (ret)
		sprintf(r, ", ret: %" PRId64 "", ret);
	if (err)
		sprintf(r + strlen(r), ", %serr: %" PRId64 "%s", colors->err, err, colors->reset);

	return r;
}

static void drm_nouveau_gem_new(uint32_t fd, uint32_t cid, uint32_t parent, struct drm_nouveau_gem_info *info)
{
	struct gpu_object *obj = gpu_object_add(fd, cid, parent, info->handle, 0);
	obj->length = info->size;
	obj->data = realloc(obj->data, obj->length);
	memset(obj->data, 0, obj->length);

	struct gpu_mapping *gmapping = calloc(sizeof(struct gpu_mapping), 1);
	gmapping->fd = fd;
	gmapping->address = info->offset;
	gmapping->length = info->size;
	gmapping->object = obj;
	gmapping->next = obj->gpu_mappings;
	obj->gpu_mappings = gmapping;

	struct cpu_mapping *cmapping = calloc(sizeof(struct cpu_mapping), 1);
	cmapping->fd = fd;
	cmapping->fdtype = FDDRM;
	cmapping->mmap_offset = info->map_handle;
	cmapping->length = info->size;
	cmapping->data = obj->data;
	cmapping->object = obj;
	cmapping->next = obj->cpu_mappings;
	obj->cpu_mappings = cmapping;
}

int demmt_drm_ioctl_post(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, uint64_t ret, uint64_t err, void *state,
		struct mmt_memory_dump *args, int argc)
{
	static int nouveau_chipset; // hack

	void *ioctl_data = buf->data;

	int i;
	if (id == DRM_NOUVEAU_IOCTL_GETPARAM)
	{
		struct drm_nouveau_getparam *data = ioctl_data;

		if (dump_decoded_ioctl_data)
		{
			mmt_log("%sDRM_NOUVEAU_GETPARAM%s, param: %s%14s%s (0x%" PRIx64 "), value: %s0x%" PRIx64 "%s",
					colors->rname, colors->reset, colors->eval,
					data->param < ARRAY_SIZE(nouveau_param_names) ? nouveau_param_names[data->param] + 17 : "???",
					colors->reset, data->param, colors->num, data->value, colors->reset);
			switch (data->param)
			{
				case NOUVEAU_GETPARAM_FB_SIZE:
				case NOUVEAU_GETPARAM_FB_PHYSICAL:
				case NOUVEAU_GETPARAM_AGP_SIZE:
				case NOUVEAU_GETPARAM_AGP_PHYSICAL:
				case NOUVEAU_GETPARAM_PCI_PHYSICAL:
					mmt_log_cont(" (%f MB)", ((double)data->value) / (1 << 20));
					if (data->value >> 30 > 0)
						mmt_log_cont(" (%f GB)", ((double)data->value) / (1 << 30));
					break;
				default:
					break;
			}
			mmt_log_cont("%s\n", ret_err(ret, err));
		}

		if (data->param == NOUVEAU_GETPARAM_CHIPSET_ID)
			nouveau_chipset = data->value;
	}
	else if (id == DRM_NOUVEAU_IOCTL_SETPARAM)
	{
		struct drm_nouveau_setparam *data = ioctl_data;

		if (dump_decoded_ioctl_data && (ret || err))
			mmt_log("%sDRM_NOUVEAU_SETPARAM%s, param: %s (0x%" PRIx64 "), value: 0x%" PRIx64 "%s\n",
					colors->rname, colors->reset,
					data->param < ARRAY_SIZE(nouveau_param_names) ? nouveau_param_names[data->param] : "???",
					data->param, data->value, ret_err(ret, err));
	}
	else if (id == DRM_NOUVEAU_IOCTL_CHANNEL_ALLOC)
	{
		struct drm_nouveau_channel_alloc *data = ioctl_data;

		if (dump_decoded_ioctl_data)
		{
			mmt_log("%sDRM_NOUVEAU_CHANNEL_ALLOC%s post, fb_ctxdma: 0x%0x, tt_ctxdma: 0x%0x, channel: %d, pushbuf_domains: ",
					colors->rname, colors->reset, data->fb_ctxdma_handle,
					data->tt_ctxdma_handle, data->channel);

			decode_domain(data->pushbuf_domains, 10);

			mmt_log_cont(", notifier: 0x%0x, nr_subchan: %d", data->notifier_handle, data->nr_subchan);

			for (i = 0; i < 8; ++i)
				if (data->subchan[i].handle || data->subchan[i].grclass)
					mmt_log_cont(" subchan[%d]=<h:0x%0x, c:0x%0x>", i, data->subchan[i].handle, data->subchan[i].grclass);

			mmt_log_cont("%s\n", ret_err(ret, err));
		}

		// hack, fake device
		struct gpu_object *dev = gpu_object_add(fd, data->channel, data->channel, data->channel, NVRM_DEVICE_0);
		nvrm_device_set_chipset(dev, nouveau_chipset);

		// hack, fake fifo
		struct gpu_object *fifo = gpu_object_add(fd, data->channel, data->channel, 0xf1f0eeee, NVRM_FIFO_IB_G80);
		get_fifo_state(fifo);
	}
	else if (id == DRM_NOUVEAU_IOCTL_CHANNEL_FREE)
	{
		struct drm_nouveau_channel_free *data = ioctl_data;

		if (dump_decoded_ioctl_data && (ret || err))
			mmt_log("%sDRM_NOUVEAU_CHANNEL_FREE%s, channel: %d%s\n", colors->rname,
					colors->reset, data->channel, ret_err(ret, err));

		struct gpu_object *fifo = gpu_object_find(data->channel, 0xf1f0eeee);
		int i;
		for (i = 0; i < fifo->children_space; ++i)
			if (fifo->children_objects[i])
				gpu_object_destroy(fifo->children_objects[i]);
		gpu_object_destroy(fifo);
		gpu_object_destroy(gpu_object_find(data->channel, data->channel));
	}
	else if (id == DRM_NOUVEAU_IOCTL_GROBJ_ALLOC)
	{
		struct drm_nouveau_grobj_alloc *data = ioctl_data;

		if (dump_decoded_ioctl_data && (ret || err))
			mmt_log("%sDRM_NOUVEAU_GROBJ_ALLOC%s, channel: %d, handle: %s0x%x%s, class: %s0x%x%s%s\n",
					colors->rname, colors->reset, data->channel, colors->num,
					data->handle, colors->reset, colors->eval, data->class,
					colors->reset, ret_err(ret, err));

		struct gpu_object *gpu_obj = gpu_object_add(fd, data->channel, 0xf1f0eeee, data->handle, data->class);
		pushbuf_add_object(data->handle, data->class, gpu_obj);
		pushbuf_add_object_name(data->handle, data->class, gpu_obj);
	}
	else if (id == DRM_NOUVEAU_IOCTL_NOTIFIEROBJ_ALLOC)
	{
		struct drm_nouveau_notifierobj_alloc *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_NOTIFIEROBJ_ALLOC%s post, channel: %d, handle: 0x%0x, size: %d, offset: %d%s\n",
					colors->rname, colors->reset, data->channel, data->handle,
					data->size, data->offset, ret_err(ret, err));

		gpu_object_add(fd, data->channel, data->channel, data->handle, 0);
	}
	else if (id == DRM_NOUVEAU_IOCTL_GPUOBJ_FREE)
	{
		struct drm_nouveau_gpuobj_free *data = ioctl_data;

		if (dump_decoded_ioctl_data && (ret || err))
			mmt_log("%sDRM_NOUVEAU_GPUOBJ_FREE%s, channel: %d, handle: 0x%0x%s\n",
					colors->rname, colors->reset, data->channel, data->handle,
					ret_err(ret, err));

		gpu_object_destroy(gpu_object_find(data->channel, data->handle));
	}
	else if (id == DRM_NOUVEAU_IOCTL_GEM_NEW)
	{
		struct drm_nouveau_gem_new *g = ioctl_data;

		if (dump_decoded_ioctl_data)
		{
			mmt_log("%sDRM_NOUVEAU_GEM_NEW%s post, ", colors->rname, colors->reset);
			dump_drm_nouveau_gem_new(g);
			mmt_log_cont("%s\n", ret_err(ret, err));
		}

		drm_nouveau_gem_new(fd, g->channel_hint, g->channel_hint, &g->info);
	}
	else if (id == DRM_NOUVEAU_IOCTL_GEM_PUSHBUF)
	{
		struct drm_nouveau_gem_pushbuf *data = ioctl_data;

		struct mmt_buf *buffers = find_ptr(data->buffers, args, argc);
		struct mmt_buf *push = find_ptr(data->push, args, argc);
		struct mmt_buf *relocs = find_ptr(data->relocs, args, argc);

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_NOUVEAU_GEM_PUSHBUF%s post, channel: %d, nr_buffers: %s%d%s, buffers: 0x%" PRIx64 ", nr_relocs: %s%d%s, relocs: 0x%" PRIx64 ", nr_push: %s%d%s, push: 0x%" PRIx64 ", suffix0: 0x%x, suffix1: 0x%x, vram_available: %" PRIu64 ", gart_available: %" PRIu64 "%s\n",
					colors->rname, colors->reset, data->channel, colors->num,
					data->nr_buffers, colors->reset, data->buffers, colors->num,
					data->nr_relocs, colors->reset,data->relocs, colors->num,
					data->nr_push, colors->reset, data->push, data->suffix0,
					data->suffix1, data->vram_available, data->gart_available,
					ret_err(ret, err));

		if (buffers || push || relocs)
			demmt_nouveau_decode_gem_pushbuf_data(data->nr_buffers, (void *)buffers->data,
					data->nr_push, (void *)push->data, data->nr_relocs, (void *)relocs->data);
	}
	else if (id == DRM_NOUVEAU_IOCTL_GEM_CPU_PREP)
	{
		struct drm_nouveau_gem_cpu_prep *data = ioctl_data;

		if (dump_decoded_ioctl_data && (ret || err))
			mmt_log("%sDRM_NOUVEAU_GEM_CPU_PREP%s, handle: %d, flags: 0x%0x%s\n",
					colors->rname, colors->reset, data->handle, data->flags,
					ret_err(ret, err));
	}
	else if (id == DRM_NOUVEAU_IOCTL_GEM_CPU_FINI)
	{
		struct drm_nouveau_gem_cpu_fini *data = ioctl_data;

		if (dump_decoded_ioctl_data && (ret || err))
			mmt_log("%sDRM_NOUVEAU_GEM_CPU_FINI%s, handle: %d%s\n", colors->rname,
					colors->reset, data->handle, ret_err(ret, err));
	}
	else if (id == DRM_NOUVEAU_IOCTL_GEM_INFO)
	{
		struct drm_nouveau_gem_info *data = ioctl_data;

		if (dump_decoded_ioctl_data)
		{
			mmt_log("%sDRM_NOUVEAU_GEM_INFO%s post, ", colors->rname, colors->reset);
			dump_drm_nouveau_gem_info(data);
			mmt_log_cont("%s\n", ret_err(ret, err));
		}

		struct gpu_object *tmp, *dev = NULL, *obj = NULL;
		for (tmp = gpu_objects; tmp != NULL; tmp = tmp->next)
		{
			if (tmp->class_ == NVRM_DEVICE_0 && tmp->fd == fd)
			{
				dev = tmp;

				struct gpu_object *tmpobj = gpu_object_find(tmp->cid, data->handle);
				if (tmpobj && obj)
				{
					mmt_error("can't uniquely determine object, assuming it's the first one%s\n", "");
					break;
				}
				obj = tmpobj;
			}
		}

		if (!dev)
			mmt_error("can't find matching device object%s\n", "");
		else if (obj)
			mmt_error("FIXME: DRM_NOUVEAU_GEM_INFO on known object%s\n", "");
		else
			drm_nouveau_gem_new(fd, dev->cid, dev->handle, data);
	}
	else if (id == DRM_IOCTL_VERSION64)
	{
		struct drm_version64 *data = ioctl_data;

		if (dump_decoded_ioctl_data)
		{
			mmt_log("%sDRM_IOCTL_VERSION64%s, version: %s%d.%d.%d%s, name_addr: 0x%"PRIx64", name_len: %"PRIu64", date_addr: 0x%"PRIx64", date_len: %"PRIu64", desc_addr: 0x%"PRIx64", desc_len: %"PRIu64"%s\n",
					colors->rname, colors->reset, colors->eval, data->version_major,
					data->version_minor, data->version_patchlevel, colors->reset,
					data->name, data->name_len, data->date, data->date_len,
					data->desc, data->desc_len, ret_err(ret, err));

			struct mmt_buf *b = NULL;
			b = find_ptr((uint64_t)(uintptr_t)data->name, args, argc);
			if (b)
				dump_mmt_buf_as_text(b, "name");

			b = find_ptr((uint64_t)(uintptr_t)data->date, args, argc);
			if (b)
				dump_mmt_buf_as_text(b, "date");

			b = find_ptr((uint64_t)(uintptr_t)data->desc, args, argc);
			if (b)
				dump_mmt_buf_as_text(b, "desc");
		}
	}
	else if (id == DRM_IOCTL_VERSION32)
	{
		struct drm_version32 *data = ioctl_data;

		if (dump_decoded_ioctl_data)
		{
			mmt_log("%sDRM_IOCTL_VERSION32%s, version: %s%d.%d.%d%s, name_addr: 0x%x, name_len: %d, date_addr: 0x%x, date_len: %d, desc_addr: 0x%x, desc_len: %d%s\n",
					colors->rname, colors->reset, colors->eval, data->version_major,
					data->version_minor, data->version_patchlevel, colors->reset,
					data->name, data->name_len, data->date, data->date_len,
					data->desc, data->desc_len, ret_err(ret, err));

			struct mmt_buf *b = NULL;
			b = find_ptr((uint64_t)(uintptr_t)data->name, args, argc);
			if (b)
				dump_mmt_buf_as_text(b, "name");

			b = find_ptr((uint64_t)(uintptr_t)data->date, args, argc);
			if (b)
				dump_mmt_buf_as_text(b, "date");

			b = find_ptr((uint64_t)(uintptr_t)data->desc, args, argc);
			if (b)
				dump_mmt_buf_as_text(b, "desc");
		}
	}
	else if (id == DRM_IOCTL_GET_MAGIC)
	{
		struct drm_auth *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_GET_MAGIC%s, magic: 0x%08x%s\n", colors->rname,
					colors->reset, data->magic, ret_err(ret, err));
	}
	else if (id == DRM_IOCTL_GEM_CLOSE)
	{
		struct drm_gem_close *data = ioctl_data;

		if (dump_decoded_ioctl_data && (ret || err))
			mmt_log("%sDRM_IOCTL_GEM_CLOSE%s, handle: %d%s\n", colors->rname,
					colors->reset, data->handle, ret_err(ret, err));
		struct gpu_object *obj = gpu_object_find(0, data->handle); // won't work for objects allocated by GEM_OPEN
		if (obj)
			gpu_object_destroy(obj);
	}
	else if (id == DRM_IOCTL_GEM_OPEN)
	{
		struct drm_gem_open *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_GEM_OPEN%s post, name: %s%d%s, handle: %s%d%s, size: %llu%s\n",
					colors->rname, colors->reset, colors->eval, data->name,
					colors->reset, colors->num, data->handle, colors->reset,
					data->size, ret_err(ret, err));
	}
	else if (id == DRM_IOCTL_GET_CAP)
	{
		struct drm_get_cap *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_GET_CAP%s, capability: %s%llu%s, value: %s%llu%s%s\n",
					colors->rname, colors->reset, colors->iname, data->capability,
					colors->reset, colors->num, data->value, colors->reset,
					ret_err(ret, err));
	}
	else if (id == DRM_IOCTL_SET_VERSION)
	{
		struct drm_set_version *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_SET_VERSION%s post, di_major: %2d, di_minor: %2d, dd_major: %2d, dd_minor: %2d\n",
					colors->rname, colors->reset, data->drm_di_major,
					data->drm_di_minor, data->drm_dd_major, data->drm_dd_minor);
	}
	else if (id == DRM_IOCTL_MODE_GETRESOURCES)
	{
		struct drm_mode_card_res *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_MODE_GETRESOURCES%s post, fb_id_ptr: %p, crtc_id_ptr: %p, connector_id_ptr: %p, "
					"encoder_id_ptr: %p, count_fbs: %d, count_crtcs: %d, count_connectors: %d, count_encoders: %d, "
					"min_width: %d, max_width: %4d, min_height: %d, max_height: %4d\n", colors->rname,
					colors->reset,
					(void *)(uintptr_t)data->fb_id_ptr,
					(void *)(uintptr_t)data->crtc_id_ptr,
					(void *)(uintptr_t)data->connector_id_ptr,
					(void *)(uintptr_t)data->encoder_id_ptr,
					data->count_fbs, data->count_crtcs, data->count_connectors,
					data->count_encoders, data->min_width, data->max_width,
					data->min_height, data->max_height);
	}
	else if (id == DRM_IOCTL_SET_MASTER)
	{
		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_SET_MASTER%s\n", colors->rname, colors->reset);
	}
	else if (id == DRM_IOCTL_DROP_MASTER)
	{
		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_DROP_MASTER%s\n", colors->rname, colors->reset);
	}
	else if (id == DRM_IOCTL_AUTH_MAGIC)
	{
		struct drm_auth *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_AUTH_MAGIC%s, magic: 0x%x\n", colors->rname, colors->reset, data->magic);
	}
	else if (id == DRM_IOCTL_GEM_FLINK)
	{
		struct drm_gem_flink *data = ioctl_data;

		if (dump_decoded_ioctl_data)
			mmt_log("%sDRM_IOCTL_GEM_FLINK%s, handle: 0x%x, name: 0x%x\n",
					colors->rname, colors->reset, data->handle, data->name);
	}
	else
	{
		mmt_log("%sunknown drm ioctl%s %x\n", colors->err, colors->reset, nr);
		return 1;
	}

	return 0;
}

#endif
