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
#include <stdio.h>

#include "mmt_bin2dedma.h"
#include "mmt_bin_decode_nvidia.h"
#include "nvrm_ioctl.h"
#ifdef LIBDRM_AVAILABLE
#include <drm.h>
#include <nouveau_drm.h>
#endif

static void dump_args(struct mmt_memory_dump *args, int argc)
{
	int i, j;

	for (i = 0; i < argc; ++i)
	{
		struct mmt_memory_dump *arg = &args[i];
		struct mmt_buf *data = arg->data;
		fprintf(stdout, PFX "address: 0x%llx", (unsigned long long)arg->addr);
		if (arg->str)
			fprintf(stdout, ", txt: \"%s\"", arg->str->data);
		fprintf(stdout, ", data.len: %d, data:", data->len);
		for (j = 0; j < data->len / 4; ++j)
			fprintf(stdout, " 0x%08x", ((uint32_t *)data->data)[j]);
		if (data->len & 3)
			for (j = data->len & 0xfffffffc; j < data->len; ++j)
				fprintf(stdout, " %02x", data->data[j]);
		fprintf(stdout, "\n");
	}
}

static void __txt_nv_ioctl_pre(uint32_t fd, uint32_t id, struct mmt_buf *data, void *state, struct mmt_memory_dump *args, int argc)
{
	uint32_t i;
	fprintf(stdout, PFX "pre_ioctl:  fd:%d, id:0x%02x (full:0x%x), data: ", fd, id & 0xFF, id);
	for (i = 0; i < data->len / 4; ++i)
		fprintf(stdout, "0x%08x ", ((uint32_t *)data->data)[i]);
	fprintf(stdout, "\n");

	void *d = data->data;
	if (id == NVRM_IOCTL_CREATE)
	{
		struct nvrm_ioctl_create *s = d;
		fprintf(stdout, PFX "create gpu object 0x%08x:0x%08x type 0x%04x (%s)\n", s->parent, s->handle, s->cls, "");
	}
	else if (id == NVRM_IOCTL_CREATE_SIMPLE)
	{
		struct nvrm_ioctl_create_simple *s = d;
		fprintf(stdout, PFX "create gpu object 0x%08x:0x%08x type 0x%04x (%s)\n", s->parent, s->handle, s->cls, "");
	}
	else if (id == NVRM_IOCTL_DESTROY)
	{
		struct nvrm_ioctl_destroy *s = d;
		fprintf(stdout, PFX "destroy object 0x%08x:0x%08x\n", s->parent, s->handle);
	}
	else if (id == NVRM_IOCTL_CALL)
	{
		struct nvrm_ioctl_call *s = d;
		fprintf(stdout, PFX "call method 0x%08x:0x%08x\n", s->handle, s->mthd);
	}
#ifdef LIBDRM_AVAILABLE
	else if (id == DRM_IOW(DRM_COMMAND_BASE + DRM_NOUVEAU_GROBJ_ALLOC, struct drm_nouveau_grobj_alloc))
	{
		struct drm_nouveau_grobj_alloc *s = d;
		fprintf(stdout, PFX "create gpu object 0x%08x:0x%08x type 0x%04x (%s)\n", 0, s->handle, s->class, "");
	}
#endif

	dump_args(args, argc);
}

static void __txt_nv_ioctl_post(uint32_t fd, uint32_t id, struct mmt_buf *data, void *state, struct mmt_memory_dump *args, int argc)
{
	uint32_t i;
	fprintf(stdout, PFX "post_ioctl: fd:%d, id:0x%02x (full:0x%x), data: ", fd, id & 0xFF, id);
	for (i = 0; i < data->len / 4; ++i)
		fprintf(stdout, "0x%08x ", ((uint32_t *)data->data)[i]);
	fprintf(stdout, "\n");

	void *d = data->data;
	if (id == NVRM_IOCTL_CREATE_DMA)
	{
		struct nvrm_ioctl_create_dma *s = d;
		fprintf(stdout, PFX "create dma object 0x%08x, type 0x%08x, parent 0x%08x\n",
						s->handle, s->cls, s->parent);
	}
	else if (id == NVRM_IOCTL_CREATE_VSPACE)
	{
		struct nvrm_ioctl_create_vspace *s = d;
		fprintf(stdout, PFX "create mapped object 0x%08x:0x%08x type=0x%08x 0x%08" PRIx64 "\n",
						s->parent, s->handle, s->cls, s->foffset);
	}
	else if (id == NVRM_IOCTL_HOST_MAP)
	{
		struct nvrm_ioctl_host_map *s = d;
		fprintf(stdout, PFX "allocate map 0x%08x:0x%08x 0x%08" PRIx64 "\n",
						s->subdev, s->handle, s->foffset);
	}
	else if (id == NVRM_IOCTL_VSPACE_MAP)
	{
		struct nvrm_ioctl_vspace_map *s = d;
		fprintf(stdout, PFX "gpu map 0x%08x:0x%08x:0x%08x, addr 0x%08" PRIx64 ", len 0x%08" PRIx64 "\n",
						s->dev, s->vspace, s->handle, s->addr, s->size);
	}
	else if (id == NVRM_IOCTL_VSPACE_UNMAP)
	{
		struct nvrm_ioctl_vspace_unmap *s = d;
		fprintf(stdout, PFX "gpu unmap 0x%08x:0x%08x:0x%08x addr 0x%08" PRIx64 "\n",
						s->dev, s->vspace, s->handle, s->addr);
	}
	else if (id == NVRM_IOCTL_HOST_UNMAP)
	{
		struct nvrm_ioctl_host_unmap *s = d;
		fprintf(stdout, PFX "deallocate map 0x%08x:0x%08x 0x%08" PRIx64 "\n",
						s->subdev, s->handle, s->foffset);
	}
	else if (id == NVRM_IOCTL_BIND)
	{
		struct nvrm_ioctl_bind *s = d;
		fprintf(stdout, PFX "bind 0x%08x 0x%08x\n", s->target, s->handle);
	}
	else if (id == NVRM_IOCTL_CREATE_DRV_OBJ)
	{
		struct nvrm_ioctl_create_drv_obj *s = d;
		fprintf(stdout, PFX "create driver object 0x%08x:0x%08x type 0x%04x\n",
				s->parent, s->handle, s->cls);
	}
	else if (id == NVRM_IOCTL_CREATE_DEV_OBJ)
	{
		struct nvrm_ioctl_create_dev_obj *s = d;
		fprintf(stdout, PFX "create device object 0x%08x\n", s->handle);
	}
	else if (id == NVRM_IOCTL_CREATE_CTX)
	{
		struct nvrm_ioctl_create_ctx *s = d;
		fprintf(stdout, PFX "created context object 0x%08x\n", s->handle);
	}

	dump_args(args, argc);
}

static void txt_nv_ioctl_pre(struct mmt_ioctl_pre *ctl, void *state, struct mmt_memory_dump *args, int argc)
{
	__txt_nv_ioctl_pre(ctl->fd, ctl->id, &ctl->data, state, args, argc);
}

static void txt_nv_ioctl_pre_v2(struct mmt_ioctl_pre_v2 *ctl, void *state, struct mmt_memory_dump *args, int argc)
{
	__txt_nv_ioctl_pre(ctl->fd, ctl->id, &ctl->data, state, args, argc);
}

static void txt_nv_ioctl_post(struct mmt_ioctl_post *ctl, void *state, struct mmt_memory_dump *args, int argc)
{
	__txt_nv_ioctl_post(ctl->fd, ctl->id, &ctl->data, state, args, argc);
}

static void txt_nv_ioctl_post_v2(struct mmt_ioctl_post_v2 *ctl, void *state, struct mmt_memory_dump *args, int argc)
{
	__txt_nv_ioctl_post(ctl->fd, ctl->id, &ctl->data, state, args, argc);
}

static void txt_memory_dump(struct mmt_memory_dump_prefix *d, struct mmt_buf *b, void *state)
{
	if (d->str.len == 0)
		return;

	fprintf(stdout, PFX "%s", d->str.data);

	uint32_t i;
	for (i = 0; i < b->len / 4; ++i)
		fprintf(stdout, "0x%08x ", ((uint32_t *)b->data)[i]);
	if (b->len & 3)
		for (i = b->len & 0xfffffffc; i < b->len; ++i)
			fprintf(stdout, "%02x ", b->data[i]);

	fprintf(stdout, "\n");
}

static void txt_nv_mmap(struct mmt_nvidia_mmap *map, void *state)
{
	fprintf(stdout, PFX "got new mmap for 0x%08" PRIx64 ":0x%08" PRIx64 " at 0x%" PRIx64 ", len: 0x%08" PRIx64 ", offset: 0x%" PRIx64 ", serial: %d\n",
						map->data1, map->data2, map->start, map->len,
						map->offset, map->id);
}

static void txt_nv_mmap2(struct mmt_nvidia_mmap2 *map, void *state)
{
	fprintf(stdout, PFX "got new mmap for 0x%08" PRIx64 ":0x%08" PRIx64 " at 0x%" PRIx64 ", len: 0x%08" PRIx64 ", offset: 0x%" PRIx64 ", serial: %d, fd: %d\n",
						map->data1, map->data2, map->start, map->len,
						map->offset, map->id, map->fd);
}

static void txt_nv_call_method_data(struct mmt_nvidia_call_method_data *call, void *state)
{
	uint32_t k;
	uint32_t *tx = (uint32_t *)call->data.data;
	fprintf(stdout, PFX "<==(%u at 0x%" PRIx64 ")\n", call->cnt, call->tx);
	for (k = 0; k < call->cnt; ++k)
		fprintf(stdout, PFX "DIR=%x MMIO=%x VALUE=%08x MASK=%08x UNK=%08x,%08x,%08x,%08x\n",
					tx[k * 8 + 0],
					tx[k * 8 + 3],
					tx[k * 8 + 5],
					tx[k * 8 + 7],
					tx[k * 8 + 1],
					tx[k * 8 + 2],
					tx[k * 8 + 4],
					tx[k * 8 + 6]);
}

static void txt_nv_ioctl_4d(struct mmt_nvidia_ioctl_4d *ctl, void *state)
{
	fprintf(stdout, PFX "in %s\n", ctl->str.data);
}

static void txt_nv_mmiotrace_mark(struct mmt_nvidia_mmiotrace_mark *mark, void *state)
{
	fprintf(stdout, PFX "MARK: %s", mark->str.data);
}

const struct mmt_nvidia_decode_funcs txt_nvidia_funcs =
{
	{ txt_memread, txt_memwrite, txt_mmap, txt_mmap2, txt_munmap, txt_mremap,
	  txt_open, txt_msg, txt_write_syscall, txt_dup, NULL, txt_nv_ioctl_pre_v2,
	  txt_nv_ioctl_post_v2 },
	NULL,
	NULL,
	txt_nv_ioctl_pre,
	txt_nv_ioctl_post,
	txt_memory_dump,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	txt_nv_mmap,
	txt_nv_mmap2,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	txt_nv_call_method_data,
	txt_nv_ioctl_4d,
	txt_nv_mmiotrace_mark,
	NULL // nouveau_gem_pushbuf_data
};

const struct mmt_nvidia_decode_funcs txt_nvidia_funcs_empty =
{
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};
