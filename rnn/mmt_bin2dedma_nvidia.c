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

#include "mmt_bin2dedma.h"
#include "mmt_bin_decode_nvidia.h"
#include <stdio.h>

static void txt_nv_create_object(struct mmt_nvidia_create_object *create, void *state)
{
	fprintf(stdout, PFX "create gpu object 0x%08x:0x%08x type 0x%04x (%s)\n", create->obj1, create->obj2,
						create->class, create->name.data);
}

static void txt_nv_destroy_object(struct mmt_nvidia_destroy_object *destroy, void *state)
{
	fprintf(stdout, PFX "destroy object 0x%08x:0x%08x\n", destroy->obj1, destroy->obj2);
}

static void txt_nv_ioctl_pre(struct mmt_ioctl_pre *ctl, void *state)
{
	uint32_t i;
	fprintf(stdout, PFX "pre_ioctl:  fd:%d, id:0x%02x (full:0x%x), data: ", ctl->fd, ctl->id & 0xFF, ctl->id);
	for (i = 0; i < ctl->data.len / 4; ++i)
		fprintf(stdout, "0x%08x ", ((uint32_t *)ctl->data.data)[i]);
	fprintf(stdout, "\n");
}

static void txt_nv_ioctl_post(struct mmt_ioctl_post *ctl, void *state)
{
	uint32_t i;
	fprintf(stdout, PFX "post_ioctl: fd:%d, id:0x%02x (full:0x%x), data: ", ctl->fd, ctl->id & 0xFF, ctl->id);
	for (i = 0; i < ctl->data.len / 4; ++i)
		fprintf(stdout, "0x%08x ", ((uint32_t *)ctl->data.data)[i]);
	fprintf(stdout, "\n");
}

static void txt_nv_memory_dump(struct mmt_nvidia_memory_dump *d, void *state)
{
	if (d->str.len == 0)
		return;
	struct mmt_txt_nvidia_state *nstate = state;

	fprintf(stdout, PFX "%s", d->str.data);
	nstate->memdump_pfx_printed = 1;
}

static void txt_nv_memory_dump_cont(struct mmt_buf *b, void *state)
{
	struct mmt_txt_nvidia_state *nstate = state;

	if (b->len > 0)
	{
		if (!nstate->memdump_pfx_printed)
		{
			fprintf(stdout, PFX);
			nstate->memdump_pfx_printed = 1;
		}
		uint32_t i;
		for (i = 0; i < b->len / 4; ++i)
			fprintf(stdout, "0x%08x ", ((uint32_t *)b->data)[i]);
	}

	if (nstate->memdump_pfx_printed)
		fprintf(stdout, "\n");
	nstate->memdump_pfx_printed = 0;
}

static void txt_nv_call_method(struct mmt_nvidia_call_method *m, void *state)
{
	fprintf(stdout, PFX "call method 0x%08x:0x%08x\n", m->data1, m->data2);
}

static void txt_nv_create_mapped(struct mmt_nvidia_create_mapped_object *p, void *state)
{
	fprintf(stdout, PFX "create mapped object 0x%08x:0x%08x type=0x%08x 0x%08lx\n", p->data1, p->data2, p->type, p->mmap_offset);
}

static void txt_nv_create_dma_object(struct mmt_nvidia_create_dma_object *create, void *state)
{
	fprintf(stdout, PFX "create dma object 0x%08x, type 0x%08x, parent 0x%08x\n", create->name, create->type, create->parent);
}

static void txt_nv_alloc_map(struct mmt_nvidia_alloc_map *alloc, void *state)
{
	fprintf(stdout, PFX "allocate map 0x%08x:0x%08x 0x%08lx\n", alloc->data1, alloc->data2, alloc->mmap_offset);
}

static void txt_nv_gpu_map(struct mmt_nvidia_gpu_map *map, void *state)
{
	fprintf(stdout, PFX "gpu map 0x%08x:0x%08x:0x%08x, addr 0x%08x, len 0x%08x\n", map->data1, map->data2, map->data3, map->gpu_start, map->len);
}

static void txt_nv_gpu_map2(struct mmt_nvidia_gpu_map2 *map, void *state)
{
	fprintf(stdout, PFX "gpu map 0x%08x:0x%08x:0x%08x, addr 0x%08lx, len 0x%08x\n", map->data1, map->data2, map->data3, map->gpu_start, map->len);
}

static void txt_nv_gpu_unmap(struct mmt_nvidia_gpu_unmap *unmap, void *state)
{
	fprintf(stdout, PFX "gpu unmap 0x%08x:0x%08x:0x%08x addr 0x%08x\n", unmap->data1, unmap->data2, unmap->data3, unmap->gpu_start);
}

static void txt_nv_gpu_unmap2(struct mmt_nvidia_gpu_unmap2 *unmap, void *state)
{
	fprintf(stdout, PFX "gpu unmap 0x%08x:0x%08x:0x%08x addr 0x%08lx\n", unmap->data1, unmap->data2, unmap->data3, unmap->gpu_start);
}

static void txt_nv_mmap(struct mmt_nvidia_mmap *map, void *state)
{
	fprintf(stdout, PFX "got new mmap for 0x%08lx:0x%08lx at %p, len: 0x%08lx, offset: 0x%llx, serial: %d\n",
						map->data1, map->data2, (void *)map->start, map->len, (long long unsigned int)map->offset, map->id);
}

static void txt_nv_unmap(struct mmt_nvidia_unmap *unmap, void *state)
{
	fprintf(stdout, PFX "deallocate map 0x%08x:0x%08x 0x%08lx\n", unmap->data1, unmap->data2, unmap->mmap_offset);
}

static void txt_nv_bind(struct mmt_nvidia_bind *bnd, void *state)
{
	fprintf(stdout, PFX "bind 0x%08x 0x%08x\n", bnd->data1, bnd->data2);
}

static void txt_nv_create_driver_object(struct mmt_nvidia_create_driver_object *create, void *state)
{
	fprintf(stdout, PFX "create driver object 0x%08x:0x%08x type 0x%04lx\n", create->obj1, create->obj2, create->addr);
}

static void txt_nv_create_device_object(struct mmt_nvidia_create_device_object *create, void *state)
{
	fprintf(stdout, PFX "create device object 0x%08x\n", create->obj1);
}

static void txt_nv_create_context_object(struct mmt_nvidia_create_context_object *create, void *state)
{
	fprintf(stdout, PFX "created context object 0x%08x\n", create->obj1);
}

static void txt_nv_call_method_data(struct mmt_nvidia_call_method_data *call, void *state)
{
	uint32_t k;
	uint32_t *tx = (uint32_t *)call->data.data;
	fprintf(stdout, PFX "<==(%u at %p)\n", call->cnt, (void *)call->tx);
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
	{ txt_memread, txt_memwrite, txt_mmap, txt_munmap, txt_mremap, txt_open, txt_msg },
	txt_nv_create_object,
	txt_nv_destroy_object,
	txt_nv_ioctl_pre,
	txt_nv_ioctl_post,
	txt_nv_memory_dump,
	txt_nv_memory_dump_cont,
	txt_nv_call_method,
	txt_nv_create_mapped,
	txt_nv_create_dma_object,
	txt_nv_alloc_map,
	txt_nv_gpu_map,
	txt_nv_gpu_map2,
	txt_nv_gpu_unmap,
	txt_nv_gpu_unmap2,
	txt_nv_mmap,
	txt_nv_unmap,
	txt_nv_bind,
	txt_nv_create_driver_object,
	txt_nv_create_device_object,
	txt_nv_create_context_object,
	txt_nv_call_method_data,
	txt_nv_ioctl_4d,
	txt_nv_mmiotrace_mark,
	NULL // nouveau_gem_pushbuf_data
};

const struct mmt_nvidia_decode_funcs txt_nvidia_funcs_empty =
{
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL },
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
