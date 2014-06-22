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

#include "mmt_bin2text.h"
#include <stdio.h>
#include <stdlib.h>

struct mmt_message_nv
{
	struct mmt_message msg_type;
	uint8_t subtype;
} __packed;

struct mmt_nvidia_create_object
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
	uint32_t obj2;
	uint32_t class;
	struct mmt_buf name;
} __packed;

struct mmt_nvidia_destroy_object
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
	uint32_t obj2;
} __packed;

struct mmt_nvidia_ioctl_pre
{
	struct mmt_message_nv msg_type;
	uint32_t fd;
	uint32_t id;
	struct mmt_buf data;
} __packed;

struct mmt_nvidia_ioctl_post
{
	struct mmt_message_nv msg_type;
	uint32_t fd;
	uint32_t id;
	struct mmt_buf data;
} __packed;

struct mmt_nvidia_memory_dump
{
	struct mmt_message_nv msg_type;
	uint64_t addr;
	struct mmt_buf str;
} __packed;

struct mmt_nvidia_call_method
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
} __packed;

struct mmt_nvidia_create_mapped_object
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
	uint32_t obj2;
	uint32_t type;
	uint64_t addr;
} __packed;

struct mmt_nvidia_create_dma_object
{
	struct mmt_message_nv msg_type;
	uint32_t name;
	uint32_t type;
	uint32_t parent;
} __packed;

struct mmt_nvidia_alloc_map
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
	uint32_t obj2;
	uint64_t addr;
} __packed;

struct mmt_nvidia_gpu_map
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
	uint32_t data3;
	uint32_t addr;
	uint32_t len;
} __packed;

struct mmt_nvidia_gpu_unmap
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
	uint32_t data3;
	uint32_t addr;
} __packed;

struct mmt_nvidia_bind
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
} __packed;

struct mmt_nvidia_mmap
{
	struct mmt_message_nv msg_type;
	uint64_t offset;
	uint32_t id;
	uint64_t start;
	uint64_t len;
	uint64_t data1;
	uint64_t data2;
} __packed;

struct mmt_nvidia_unmap
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
	uint32_t obj2;
	uint64_t addr;
} __packed;

struct mmt_nvidia_create_driver_object
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
	uint32_t obj2;
	uint64_t addr;
} __packed;

struct mmt_nvidia_create_device_object
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
} __packed;

struct mmt_nvidia_create_context_object
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
} __packed;

struct mmt_nvidia_call_method_data
{
	struct mmt_message_nv msg_type;
	uint32_t cnt;
	uint64_t tx;
	struct mmt_buf data;
} __packed;

struct mmt_nvidia_ioctl_4d
{
	struct mmt_message_nv msg_type;
	struct mmt_buf str;
} __packed;

struct mmt_nvidia_mmiotrace_mark
{
	struct mmt_message_nv msg_type;
	struct mmt_buf str;
} __packed;

void parse_nvidia()
{
	struct mmt_message_nv *nv = load_data(sizeof(struct mmt_message_nv));
	if (nv->subtype == 'c')
	{
		struct mmt_nvidia_create_object *create;
		create = load_data(sizeof(struct mmt_nvidia_create_object) + 1);
		create = load_data(sizeof(struct mmt_nvidia_create_object) + 1 + create->name.len);

		check_eor(buf[idx + sizeof(*create) + create->name.len]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "create gpu object 0x%08x:0x%08x type 0x%04x (%s)\n", create->obj1, create->obj2,
					create->class, create->name.data);

		idx += sizeof(*create) + create->name.len + 1;
	}
	else if (nv->subtype == 'd')
	{
		struct mmt_nvidia_destroy_object *destroy = load_data(sizeof(struct mmt_nvidia_destroy_object) + 1);

		check_eor(buf[idx + sizeof(*destroy)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "destroy object 0x%08x:0x%08x\n", destroy->obj1, destroy->obj2);

		idx += sizeof(*destroy) + 1;
	}
	else if (nv->subtype == 'i')
	{
		struct mmt_nvidia_ioctl_pre *ctl;
		ctl = load_data(sizeof(struct mmt_nvidia_ioctl_pre) + 1);
		ctl = load_data(sizeof(struct mmt_nvidia_ioctl_pre) + 1 + ctl->data.len);

		check_eor(buf[idx + sizeof(*ctl) + ctl->data.len]);

		if (PRINT_DATA)
		{
			int i;
			fprintf(stdout, PFX "pre_ioctl:  fd:%d, id:0x%02x (full:0x%x), data: ", ctl->fd, ctl->id & 0xFF, ctl->id);
			for (i = 0; i < ctl->data.len / 4; ++i)
				fprintf(stdout, "0x%08x ", ((uint32_t *)ctl->data.data)[i]);
			fprintf(stdout, "\n");
		}

		idx += sizeof(*ctl) +  1 + ctl->data.len;
	}
	else if (nv->subtype == 'j')
	{
		struct mmt_nvidia_ioctl_post *ctl;
		ctl = load_data(sizeof(struct mmt_nvidia_ioctl_post) + 1);
		ctl = load_data(sizeof(struct mmt_nvidia_ioctl_post) + 1 + ctl->data.len);

		check_eor(buf[idx + sizeof(*ctl) + ctl->data.len]);

		if (PRINT_DATA)
		{
			int i;
			fprintf(stdout, PFX "post_ioctl: fd:%d, id:0x%02x (full:0x%x), data: ", ctl->fd, ctl->id & 0xFF, ctl->id);
			for (i = 0; i < ctl->data.len / 4; ++i)
				fprintf(stdout, "0x%08x ", ((uint32_t *)ctl->data.data)[i]);
			fprintf(stdout, "\n");
		}

		idx += sizeof(*ctl) +  1 + ctl->data.len;
	}
	else if (nv->subtype == 'o')
	{
		struct mmt_nvidia_memory_dump *d;
		d = load_data(sizeof(struct mmt_nvidia_memory_dump));
		d = load_data(sizeof(struct mmt_nvidia_memory_dump) + d->str.len);
		int pfx_printed = 0;

		if (PRINT_DATA && d->str.len > 0)
		{
			fprintf(stdout, PFX "%s", d->str.data);
			pfx_printed = 1;
		}

		idx += sizeof(struct mmt_nvidia_memory_dump) + d->str.len;

		struct mmt_buf *b;
		b = load_data(4);
		b = load_data(b->len + 1);

		check_eor(buf[idx + sizeof(struct mmt_buf) + b->len]);

		if (PRINT_DATA && b->len > 0)
		{
			if (!pfx_printed)
			{
				fprintf(stdout, PFX);
				pfx_printed = 1;
			}
			int i;
			for (i = 0; i < b->len / 4; ++i)
				fprintf(stdout, "0x%08x ", ((uint32_t *)b->data)[i]);
		}
		if (pfx_printed)
			fprintf(stdout, "\n");

		idx += sizeof(struct mmt_buf) + b->len + 1;
	}
	else if (nv->subtype == 'l')
	{
		struct mmt_nvidia_call_method *m = load_data(sizeof(struct mmt_nvidia_call_method) + 1);

		check_eor(buf[idx + sizeof(*m)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "call method 0x%08x:0x%08x\n", m->data1, m->data2);

		idx += sizeof(struct mmt_nvidia_call_method) + 1;
	}
	else if (nv->subtype == 'p')
	{
		struct mmt_nvidia_create_mapped_object *p = load_data(sizeof(struct mmt_nvidia_create_mapped_object) + 1);

		check_eor(buf[idx + sizeof(*p)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "create mapped object 0x%08x:0x%08x type=0x%08x 0x%08lx\n", p->obj1, p->obj2, p->type, p->addr);

		idx += sizeof(struct mmt_nvidia_create_mapped_object) + 1;
	}
	else if (nv->subtype == 't')
	{
		struct mmt_nvidia_create_dma_object *create = load_data(sizeof(struct mmt_nvidia_create_dma_object) + 1);

		check_eor(buf[idx + sizeof(*create)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "create dma object 0x%08x, type 0x%08x, parent 0x%08x\n", create->name, create->type, create->parent);

		idx += sizeof(struct mmt_nvidia_create_dma_object) + 1;
	}
	else if (nv->subtype == 'a')
	{
		struct mmt_nvidia_alloc_map *alloc = load_data(sizeof(struct mmt_nvidia_alloc_map) + 1);

		check_eor(buf[idx + sizeof(*alloc)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "allocate map 0x%08x:0x%08x 0x%08lx\n", alloc->obj1, alloc->obj2, alloc->addr);

		idx += sizeof(struct mmt_nvidia_alloc_map) + 1;
	}
	else if (nv->subtype == 'g')
	{
		struct mmt_nvidia_gpu_map *map = load_data(sizeof(struct mmt_nvidia_gpu_map) + 1);

		check_eor(buf[idx + sizeof(*map)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "gpu map 0x%08x:0x%08x:0x%08x, addr 0x%08x, len 0x%08x\n", map->data1, map->data2, map->data3, map->addr, map->len);

		idx += sizeof(struct mmt_nvidia_gpu_map) + 1;
	}
	else if (nv->subtype == 'h')
	{
		struct mmt_nvidia_gpu_unmap *unmap = load_data(sizeof(struct mmt_nvidia_gpu_unmap) + 1);

		check_eor(buf[idx + sizeof(*unmap)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "gpu unmap 0x%08x:0x%08x:0x%08x addr 0x%08x\n", unmap->data1, unmap->data2, unmap->data3, unmap->addr);

		idx += sizeof(struct mmt_nvidia_gpu_unmap) + 1;
	}
	else if (nv->subtype == 'm')
	{
		struct mmt_nvidia_mmap *map = load_data(sizeof(struct mmt_nvidia_mmap) + 1);

		check_eor(buf[idx + sizeof(*map)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "got new mmap for 0x%08lx:0x%08lx at %p, len: 0x%08lx, offset: 0x%llx, serial: %d\n",
					map->data1, map->data2, (void *)map->start, map->len, (long long unsigned int)map->offset, map->id);

		idx += sizeof(struct mmt_nvidia_mmap) + 1;
	}
	else if (nv->subtype == 'e')
	{
		struct mmt_nvidia_unmap *unmap = load_data(sizeof(struct mmt_nvidia_unmap) + 1);

		check_eor(buf[idx + sizeof(*unmap)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "deallocate map 0x%08x:0x%08x 0x%08lx\n", unmap->obj1, unmap->obj2, unmap->addr);

		idx += sizeof(struct mmt_nvidia_unmap) + 1;
	}
	else if (nv->subtype == 'b')
	{
		struct mmt_nvidia_bind *bnd = load_data(sizeof(struct mmt_nvidia_bind) + 1);

		check_eor(buf[idx + sizeof(*bnd)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "bind 0x%08x 0x%08x\n", bnd->data1, bnd->data2);

		idx += sizeof(struct mmt_nvidia_bind) + 1;
	}
	else if (nv->subtype == 'r')
	{
		struct mmt_nvidia_create_driver_object *create = load_data(sizeof(struct mmt_nvidia_create_driver_object) + 1);

		check_eor(buf[idx + sizeof(*create)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "create driver object 0x%08x:0x%08x type 0x%04lx\n", create->obj1, create->obj2, create->addr);

		idx += sizeof(struct mmt_nvidia_create_driver_object) + 1;
	}
	else if (nv->subtype == 'v')
	{
		struct mmt_nvidia_create_device_object *create = load_data(sizeof(struct mmt_nvidia_create_device_object) + 1);

		check_eor(buf[idx + sizeof(*create)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "create device object 0x%08x\n", create->obj1);

		idx += sizeof(struct mmt_nvidia_create_device_object) + 1;
	}
	else if (nv->subtype == 'x')
	{
		struct mmt_nvidia_create_context_object *create = load_data(sizeof(struct mmt_nvidia_create_context_object) + 1);

		check_eor(buf[idx + sizeof(*create)]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "created context object 0x%08x\n", create->obj1);

		idx += sizeof(struct mmt_nvidia_create_context_object) + 1;
	}
	else if (nv->subtype == '1')
	{
		struct mmt_nvidia_call_method_data *call;
		call = load_data(sizeof(struct mmt_nvidia_call_method_data) + 1);
		call = load_data(sizeof(struct mmt_nvidia_call_method_data) + 1 + call->data.len);

		check_eor(buf[idx + sizeof(*call)] + call->data.len);

		if (PRINT_DATA)
		{
			int k;
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

		idx += sizeof(struct mmt_nvidia_call_method_data) + 1 + call->data.len;
	}
	else if (nv->subtype == '4')
	{
		struct mmt_nvidia_ioctl_4d *ctl;
		ctl = load_data(sizeof(struct mmt_nvidia_ioctl_4d) + 1);
		ctl = load_data(sizeof(struct mmt_nvidia_ioctl_4d) + 1 + ctl->str.len);

		check_eor(buf[idx + sizeof(*ctl) + ctl->str.len]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "in %s\n", ctl->str.data);

		idx += sizeof(struct mmt_nvidia_ioctl_4d) + 1 + ctl->str.len;
	}
	else if (nv->subtype == 'k')
	{
		struct mmt_nvidia_mmiotrace_mark *mark;
		mark = load_data(sizeof(struct mmt_nvidia_mmiotrace_mark) + 1);
		mark = load_data(sizeof(struct mmt_nvidia_mmiotrace_mark) + 1 + mark->str.len);

		check_eor(buf[idx + sizeof(*mark) + mark->str.len]);

		if (PRINT_DATA)
			fprintf(stdout, PFX "MARK: %s", mark->str.data);

		idx += sizeof(struct mmt_nvidia_mmiotrace_mark) + 1 + mark->str.len;
	}
	else
	{
		fprintf(stderr, "ioctl\n");
		dump_next();
		abort();
	}
}
