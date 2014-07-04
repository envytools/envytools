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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mmt_bin_decode.h"
#include "mmt_bin_decode_nvidia.h"
#include "demmt.h"
#include "demmt_pushbuf.h"
#include "rnndec.h"
#include "util.h"

struct region
{
	struct region *prev;
	uint32_t start;
	uint32_t end;
	struct region *next;
};

struct buffer
{
	unsigned char *data;
	int length;
	uint64_t offset;
	uint64_t start;
	uint64_t data1;
	uint64_t data2;
	enum BUFTYPE { PUSH, IB } type;
	union
	{
		struct pushbuf_decode_state pushbuf;
		struct ib_decode_state ib;
	} state;
};
#define MAX_ID 1024

static struct buffer *buffers[MAX_ID] = { NULL };
static struct region *wreg_head[MAX_ID] = { NULL };
static struct region *wreg_last[MAX_ID] = { NULL };
static int wreg_count = 0;
static int writes_since_last_full_dump = 0; // NOTE: you cannot rely too much on this value - it includes overwrites
static int writes_since_last_dump = 0;
static int last_wreg_id = -1;
static int compress_clears = 1;
static int ib_buffer = -1;
static int dump_ioctls = 0;

struct rnndomain *domain;
struct rnndb *rnndb;
int chipset;
int guess_invalid_pushbuf = 1;
int invalid_pushbufs_visible = 1;
int decode_invalid_buffers = 1;
int m2mf_hack_enabled = 0;

static void dump(int id)
{
	struct region *cur = wreg_head[id];
	mmt_log("currently buffered writes for id: %d:\n", id);
	while (cur)
	{
		mmt_log("<0x%08x, 0x%08x>\n", cur->start, cur->end);
		cur = cur->next;
	}
	mmt_log("end of buffered writes for id: %d\n", id);
}

static void dump_and_abort(int id)
{
	dump(id);
	abort();
}

static void dump_writes(int id)
{
	struct region *cur = wreg_head[id];
	struct pushbuf_decode_state *state = &buffers[id]->state.pushbuf;
	struct ib_decode_state *ibstate = &buffers[id]->state.ib;
	char pushbuf_desc[1024];
	mmt_log("currently buffered writes for id: %d:\n", id);
	while (cur)
	{
		mmt_log("<0x%08x, 0x%08x>\n", cur->start, cur->end);
		unsigned char *data = buffers[id]->data;
		uint32_t addr = cur->start;

		int left = cur->end - addr;
		if (compress_clears && buffers[id]->type == PUSH && *(uint32_t *)(data + addr) == 0)
		{
			uint32_t addr_start = addr;
			while (addr < cur->end)
			{
				if (left >= 4)
				{
					if (*(uint32_t *)(data + addr) == 0)
					{
						addr += 4;
						left -= 4;
					}
					else
						break;
				}
				else if (left >= 2)
				{
					if (*(uint16_t *)(data + addr) == 0)
					{
						addr += 2;
						left -= 2;
					}
					else
						break;
				}
				else
				{
					if (*(uint8_t *)(data + addr) == 0)
					{
						++addr;
						--left;
					}
					else
						break;
				}
			}
			if (addr_start == state->next_command_offset)
				state->next_command_offset = addr;

			mmt_log("all zeroes between 0x%04x and 0x%04x\n", addr_start, addr);
			fprintf(stdout, "w %d:0x%04x-0x%04x, 0x00000000\n", id, addr_start, addr);
		}

		if (buffers[id]->type == IB)
			ib_decode_start(ibstate);
		else
		{
			if (addr != state->next_command_offset)
			{
				mmt_log("restarting pushbuf decode on buffer %d: %x != %x\n", id, addr, state->next_command_offset);
				pushbuf_decode_start(state);
			}

			// this is temporary, will be removed when proper IB support lands
			if (state->pushbuf_invalid == 1)
			{
				mmt_log("restarting pushbuf decode on buffer %d\n", id);
				pushbuf_decode_start(state);
			}
		}

		while (addr < cur->end)
		{
			if (left >= 4)
			{
				if (buffers[id]->type == IB)
				{
					ib_decode(ibstate, *(uint32_t *)(data + addr), pushbuf_desc);
					fprintf(stdout, "w %d:0x%04x, 0x%08x  %s\n", id, addr, *(uint32_t *)(data + addr), pushbuf_desc);
				}
				else
				{
					if (state->pushbuf_invalid == 0 || decode_invalid_buffers)
						pushbuf_decode(state, *(uint32_t *)(data + addr), pushbuf_desc);
					else
						pushbuf_desc[0] = 0;

					if (state->pushbuf_invalid == 1 && invalid_pushbufs_visible == 0)
						break;

					state->next_command_offset = addr + 4;
					fprintf(stdout, "w %d:0x%04x, 0x%08x  %s%s\n", id, addr, *(uint32_t *)(data + addr), state->pushbuf_invalid ? "INVALID " : "", pushbuf_desc);
				}

				addr += 4;
				left -= 4;
			}
			else if (left >= 2)
			{
				fprintf(stdout, "w %d:0x%04x, 0x%04x\n", id, addr, *(uint16_t *)(data + addr));
				addr += 2;
				left -= 2;
			}
			else
			{
				fprintf(stdout, "w %d:0x%04x, 0x%02x\n", id, addr, *(uint8_t *)(data + addr));
				++addr;
				--left;
			}
		}

		if (buffers[id]->type == IB)
			ib_decode_end(ibstate);
		else
			pushbuf_decode_end(state);

		cur = cur->next;
	}
	mmt_log("end of buffered writes for id: %d\n", id);
}

static void check_sanity(int id)
{
	struct region *cur = wreg_head[id];
	if (!cur)
		return;

	if (cur->prev)
	{
		mmt_error("%s", "start->prev != NULL\n");
		dump_and_abort(id);
	}

	while (cur)
	{
		if (cur->start >= cur->end)
		{
			mmt_error("cur->start >= cur->end 0x%x 0x%x\n", cur->start, cur->end);
			dump_and_abort(id);
		}

		if (cur->next)
		{
			if (cur->end >= cur->next->start)
			{
				mmt_error("cur->end >= cur->next->start 0x%x 0x%x\n", cur->end, cur->next->start);
				dump_and_abort(id);
			}
		}

		cur = cur->next;
	}
}

static void check_coverage(const struct mmt_write *w)
{
	struct region *cur = wreg_head[w->id];

	while (cur)
	{
		if (w->offset >= cur->start && w->offset + w->len <= cur->end)
			return;

		cur = cur->next;
	}

	mmt_error("region <0x%08x, 0x%08x> was not added!\n", w->offset, w->offset + w->len);
	dump_and_abort(w->id);
}

static void drop_entry(struct region *reg, int id)
{
	struct region *prev = reg->prev;
	struct region *next = reg->next;
	mmt_debug("dropping entry <0x%08x, 0x%08x>\n", reg->start, reg->end);
	free(reg);
	if (prev)
		prev->next = next;
	else
	{
		mmt_debug("new head is at <0x%08x, 0x%08x>\n", next->start, next->end);
		wreg_head[id] = next;
	}

	if (next)
		next->prev = prev;
	else
	{
		mmt_debug("new last is at <0x%08x, 0x%08x>\n", prev->start, prev->end);
		wreg_last[id] = prev;
	}
}

static void maybe_merge_with_next(struct region *cur, int id)
{
	// next exists?
	if (!cur->next)
		return;

	// next starts after this one?
	if (cur->next->start > cur->end)
		return;

	// now next starts within this one

	// next ends within this one?
	if (cur->next->end <= cur->end)
	{
		// drop next one
		drop_entry(cur->next, id);

		// and see if next^2 should be merged
		maybe_merge_with_next(cur, id);
	}
	else
	{
		// now next ends after this one

		// extend current end
		mmt_debug("extending entry <0x%08x, 0x%08x> right to 0x%08x\n",
				cur->start, cur->end, cur->next->end);
		cur->end = cur->next->end;

		// and drop next entry
		drop_entry(cur->next, id);
	}
}

static void maybe_merge_with_previous(struct region *cur, int id)
{
	// previous exists?
	if (!cur->prev)
		return;

	// previous ends before start of this one?
	if (cur->prev->end < cur->start)
		return;

	// now previous ends within this one

	// previous starts within this one?
	if (cur->prev->start >= cur->start)
	{
		// just drop previous entry
		drop_entry(cur->prev, id);

		// and see if previous^2 should be merged
		maybe_merge_with_previous(cur, id);
	}
	else
	{
		// now previous starts before this one

		// extend current start
		mmt_debug("extending entry <0x%08x, 0x%08x> left to 0x%08x\n",
				cur->start, cur->end, cur->prev->start);
		cur->start = cur->prev->start;

		// and drop previous entry
		drop_entry(cur->prev, id);
	}
}

static void register_write(const struct mmt_write *w)
{
	uint32_t id = w->id;
	if (id >= MAX_ID)
	{
		mmt_error("id >= %d\n", MAX_ID);
		abort();
	}

	if (buffers[id] == NULL)
	{
		mmt_error("buffer %d does not exist\n", id);
		dump_and_abort(id);
	}

	if (buffers[id]->length < w->offset + w->len)
	{
		mmt_error("buffer %d is too small (%d) for write starting at %d and length %d\n",
				id, buffers[id]->length, w->offset, w->len);
		dump_and_abort(id);
	}

	memcpy(buffers[id]->data + w->offset, w->data, w->len);

	struct region *cur = wreg_head[id];

	if (cur == NULL)
	{
		wreg_head[id] = cur = malloc(sizeof(struct region));
		cur->start = w->offset;
		cur->end = w->offset + w->len;
		cur->prev = NULL;
		cur->next = NULL;
		wreg_last[id] = cur;
		return;
	}

	if (w->offset == wreg_last[id]->end)
	{
		mmt_debug("extending last entry <0x%08x, 0x%08x> right by %d\n",
				wreg_last[id]->start, wreg_last[id]->end, w->len);
		wreg_last[id]->end += w->len;
		return;
	}

	if (w->offset > wreg_last[id]->end)
	{
		mmt_debug("adding last entry <0x%08x, 0x%08x> after <0x%08x, 0x%08x>\n",
				w->offset, w->offset + w->len, wreg_last[id]->start, wreg_last[id]->end);
		cur = malloc(sizeof(struct region));
		cur->start = w->offset;
		cur->end = w->offset + w->len;
		cur->prev = wreg_last[id];
		cur->next = NULL;
		wreg_last[id]->next = cur;
		wreg_last[id] = cur;
		return;
	}

	if (w->offset + w->len > wreg_last[id]->end)
	{
		mmt_debug("extending last entry <0x%08x, 0x%08x> right to 0x%08x\n",
				wreg_last[id]->start, wreg_last[id]->end, w->offset + w->len);
		wreg_last[id]->end = w->offset + w->len;
		if (w->offset < wreg_last[id]->start)
		{
			mmt_debug("... and extending last entry <0x%08x, 0x%08x> left to 0x%08x\n",
					wreg_last[id]->start, wreg_last[id]->end, w->offset);
			wreg_last[id]->start = w->offset;
			maybe_merge_with_previous(wreg_last[id], id);
		}
		return;
	}


	while (w->offset + w->len > cur->end) // if we will ever crash on cur being NULL then it means we screwed up earlier
	{
		if (cur->end == w->offset) // optimization
		{
			mmt_debug("extending entry <0x%08x, 0x%08x> by %d\n", cur->start, cur->end, w->len);
			cur->end += w->len;
			maybe_merge_with_next(cur, id);
			return;
		}
		cur = cur->next;
	}

	// now it ends before end of current

	// does it start in this one?
	if (w->offset >= cur->start)
		return;

	// does it end before start of current one?
	if (w->offset + w->len < cur->start)
	{
		mmt_debug("adding new entry <0x%08x, 0x%08x> before <0x%08x, 0x%08x>\n",
				w->offset, w->offset + w->len, cur->start, cur->end);
		struct region *tmp = malloc(sizeof(struct region));
		tmp->start = w->offset;
		tmp->end = w->offset + w->len;
		tmp->prev = cur->prev;
		tmp->next = cur;
		if (cur->prev)
			cur->prev->next = tmp;
		else
			wreg_head[id] = tmp;
		cur->prev = tmp;
		maybe_merge_with_previous(tmp, id);
		return;
	}

	// now it ends in current and starts before
	cur->start = w->offset;
	maybe_merge_with_previous(cur, id);
}

static void dump_buffered_writes(int full)
{
	int i;
	mmt_log("CURRENTLY BUFFERED WRITES, number of buffers: %d, buffered writes: %d (0x%x) (/4 = %d (0x%x))\n",
			wreg_count, writes_since_last_full_dump, writes_since_last_full_dump,
			writes_since_last_full_dump / 4, writes_since_last_full_dump / 4);
	for (i = 0; i < MAX_ID; ++i)
		if (wreg_head[i])
			dump(i);
	mmt_log("%s\n", "END OF CURRENTLY BUFFERED WRITES");
	if (full)
	{
		for (i = 0; i < MAX_ID; ++i)
			if (wreg_head[i])
				dump_writes(i);
		writes_since_last_full_dump = 0;
	}
	writes_since_last_dump = 0;
}

static void clear_buffered_writes()
{
	int i;
	for (i = 0; i < MAX_ID; ++i)
	{
		struct region *cur = wreg_head[i], *next;
		if (!cur)
			continue;

		while (cur)
		{
			next = cur->next;
			free(cur);
			cur = next;
		}

		wreg_head[i] = NULL;
		wreg_last[i] = NULL;
	}
	wreg_count = 0;
	last_wreg_id = -1;
}

static void demmt_memread(struct mmt_read *w, void *state)
{
	if (w->len == 1)
		fprintf(stdout, "r %d:0x%04x, 0x%02x\n", w->id, w->offset, w->data[0]);
	else if (w->len == 2)
		fprintf(stdout, "r %d:0x%04x, 0x%04x\n", w->id, w->offset, *(uint16_t *)(w->data));
	else if (w->len == 4 || w->len == 8 || w->len == 16 || w->len == 32)
	{
		fprintf(stdout, "r %d:0x%04x, ", w->id, w->offset);
		int i;
		for (i = 0; i < w->len; i += 4)
			fprintf(stdout, "0x%08x ", *(uint32_t *)(w->data + i));
		fprintf(stdout, "\n");
	}

	if (wreg_count)
	{
		if (0)
		{
			mmt_log("%s\n", "read registered, flushing currently buffered writes");
			dump_buffered_writes(1);
			clear_buffered_writes();
			mmt_log("%s\n", "");
		}
		else
		{
			if (writes_since_last_dump)
			{
				mmt_log("%s\n", "read registered, NOT flushing currently buffered writes");
				dump_buffered_writes(0);
				mmt_log("%s\n", "");
			}
			else
			{
				//mmt_log("%s\n", "read registered, NO new writes registered, NOT flushing buffered writes");
			}
		}
	}
}

static void demmt_memwrite(struct mmt_write *w, void *state)
{
	if (last_wreg_id != w->id)
	{
		if (last_wreg_id == -1)
			last_wreg_id = w->id;
		else
		{
			if (wreg_count)
			{
				mmt_log("new region write registered (new: %d, old: %d), flushing buffered writes\n",
						w->id, last_wreg_id);
				dump_buffered_writes(1);
				clear_buffered_writes();
				mmt_log("%s\n", "");
				last_wreg_id = w->id;
			}
		}
	}
	int wreg_existed = wreg_head[w->id] != NULL;
	register_write(w);
	check_sanity(w->id);
	check_coverage(w);
	if (!wreg_existed)
		wreg_count++;
	writes_since_last_full_dump += w->len;
}

static void demmt_mmap(struct mmt_mmap *mm, void *state)
{
	mmt_log("mmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx\n",
			(void *)mm->start, mm->len, mm->id, mm->offset);
	buffers[mm->id] = calloc(1, sizeof(struct buffer));
	buffers[mm->id]->data = calloc(mm->len, 1);
	buffers[mm->id]->start = mm->start;
	buffers[mm->id]->length = mm->len;
	buffers[mm->id]->offset = mm->offset;
	if (mm->id == ib_buffer)
		buffers[mm->id]->type = IB;
}

static void demmt_munmap(struct mmt_unmap *mm, void *state)
{
	mmt_log("munmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx\n",
			(void *)mm->start, mm->len, mm->id, mm->offset, mm->data1, mm->data2);
	free(buffers[mm->id]->data);
	free(buffers[mm->id]);
	buffers[mm->id] = NULL;
}

static void demmt_mremap(struct mmt_mremap *mm, void *state)
{
	mmt_log("mremap: old_address: %p, new_address: %p, old_length: 0x%08lx, new_length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx\n",
			(void *)mm->old_start, (void *)mm->start, mm->old_len, mm->len, mm->id, mm->offset, mm->data1, mm->data2);

	if (wreg_count)
	{
		mmt_log("mremap, flushing buffered writes%s\n", "");
		dump_buffered_writes(1);
		clear_buffered_writes();
		mmt_log("%s\n", "");
	}

	struct buffer *oldbuf = buffers[mm->id];

	buffers[mm->id] = calloc(1, sizeof(struct buffer));
	buffers[mm->id]->data = calloc(mm->len, 1);
	buffers[mm->id]->start = mm->start;
	buffers[mm->id]->length = mm->len;
	buffers[mm->id]->offset = mm->offset;
	buffers[mm->id]->data1 = mm->data1;
	buffers[mm->id]->data2 = mm->data2;
	buffers[mm->id]->type = oldbuf->type;
	memcpy(&buffers[mm->id]->state, &oldbuf->state, sizeof(buffers[mm->id]->state));
	memcpy(buffers[mm->id]->data, oldbuf->data, min(mm->len, oldbuf->length));

	free(oldbuf->data);
	free(oldbuf);
}

static void demmt_open(struct mmt_open *o, void *state)
{
}

static void demmt_nv_create_object(struct mmt_nvidia_create_object *create, void *state)
{
	mmt_log("create object: obj1: 0x%08x, obj2: 0x%08x, class: 0x%08x\n", create->obj1, create->obj2, create->class);
	pushbuf_add_object(create->obj2, create->class);
}

static void demmt_nv_destroy_object(struct mmt_nvidia_destroy_object *destroy, void *state)
{
	mmt_log("destroy object: obj1: 0x%08x, obj2: 0x%08x\n", destroy->obj1, destroy->obj2);
}

static void ioctl_data_print(struct mmt_buf *data)
{
	int i;
	if (!dump_ioctls)
		return;

	mmt_log_cont(", data:%s", "");
	for (i = 0; i < data->len; ++i)
		mmt_log_cont(" 0x%08x", ((uint32_t *)data->data)[i]);
}

static void demmt_nv_ioctl_pre(struct mmt_nvidia_ioctl_pre *ctl, void *state)
{
	mmt_log("ioctl pre  0x%02x (0x%08x)", ctl->id & 0xff, ctl->id);
	ioctl_data_print(&ctl->data);
	if (1)
	{
		if (wreg_count)
		{
			mmt_log_cont(", flushing buffered writes%s\n", "");
			dump_buffered_writes(1);
			clear_buffered_writes();
			mmt_log("%s\n", "");
		}
		else
			mmt_log_cont(", no dirty buffers%s\n", "");
	}
	else
		mmt_log_cont("%s\n", "");
}

static void demmt_nv_ioctl_post(struct mmt_nvidia_ioctl_post *ctl, void *state)
{
	if (dump_ioctls)
	{
		mmt_log("ioctl post 0x%02x (0x%08x)", ctl->id & 0xff, ctl->id);
		ioctl_data_print(&ctl->data);
		mmt_log_cont("%s\n", "");
	}
}

static void demmt_nv_memory_dump(struct mmt_nvidia_memory_dump *d, void *state)
{
}

static void demmt_nv_memory_dump_cont(struct mmt_buf *b, void *state)
{
}

static void demmt_nv_call_method(struct mmt_nvidia_call_method *m, void *state)
{
	mmt_log("call method: data1: 0x%08x, data2: 0x%08x\n", m->data1, m->data2);
}

static void demmt_nv_create_mapped(struct mmt_nvidia_create_mapped_object *p, void *state)
{
	int i;
	mmt_log("create mapped object: address: %p, data1: 0x%08x, data2: 0x%08x, type: 0x%08x\n",
			(void *)p->addr, p->obj1, p->obj2, p->type);

	if (p->addr == 0)
		return;

	for (i = 0; i < MAX_ID; ++i)
		if (buffers[i] && buffers[i]->offset == p->addr)
		{
			buffers[i]->data1 = p->obj1;
			buffers[i]->data2 = p->obj2;
			return;
		}

	mmt_log("couldn't find buffer%s\n", "");
}

static void demmt_nv_create_dma_object(struct mmt_nvidia_create_dma_object *create, void *state)
{
	mmt_log("create dma object, name: 0x%08x, type: 0x%08x, parent: 0x%08x\n",
			create->name, create->type, create->parent);
}

static void demmt_nv_alloc_map(struct mmt_nvidia_alloc_map *alloc, void *state)
{
	int i;
	mmt_log("allocate map: address: %p, data1: 0x%08x, data2: 0x%08x\n",
			(void *)alloc->addr, alloc->obj1, alloc->obj2);

	for (i = 0; i < MAX_ID; ++i)
		if (buffers[i] && buffers[i]->offset == alloc->addr)
		{
			buffers[i]->data1 = alloc->obj1;
			buffers[i]->data2 = alloc->obj2;
			return;
		}

	mmt_log("couldn't find buffer%s\n", "");
}

static void demmt_nv_gpu_map(struct mmt_nvidia_gpu_map *map, void *state)
{
	mmt_log("gpu map: data1: 0x%08x, data2: 0x%08x, data3: 0x%08x, addr: 0x%08x, len: 0x%08x\n",
			map->data1, map->data2, map->data3, map->addr, map->len);
}

static void demmt_nv_gpu_unmap(struct mmt_nvidia_gpu_unmap *unmap, void *state)
{
	mmt_log("gpu unmap: data1: 0x%08x, data2: 0x%08x, data3: 0x%08x, addr: 0x%08x\n",
			unmap->data1, unmap->data2, unmap->data3, unmap->addr);
}

static void demmt_nv_mmap(struct mmt_nvidia_mmap *mm, void *state)
{
	mmt_log("mmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx\n",
			(void *)mm->start, mm->len, mm->id, mm->offset, mm->data1, mm->data2);
	buffers[mm->id] = calloc(1, sizeof(struct buffer));
	buffers[mm->id]->data = calloc(mm->len, 1);
	buffers[mm->id]->start = mm->start;
	buffers[mm->id]->length = mm->len;
	buffers[mm->id]->offset = mm->offset;
	buffers[mm->id]->data1 = mm->data1;
	buffers[mm->id]->data2 = mm->data2;
	if (mm->id == ib_buffer)
		buffers[mm->id]->type = IB;
}

static void demmt_nv_unmap(struct mmt_nvidia_unmap *mm, void *state)
{
	int i;
	mmt_log("nv_munmap: address: %p, data1: 0x%08x, data2: 0x%08x\n",
			(void *)mm->addr, mm->obj1, mm->obj2);

	for (i = 0; i < MAX_ID; ++i)
		if (buffers[i] && buffers[i]->offset == mm->addr)
		{
			free(buffers[i]->data);
			free(buffers[i]);
			buffers[i] = NULL;
			return;
		}

	mmt_log("couldn't find buffer to free%s\n", ""); // find by data1/data2?
}

static void demmt_nv_bind(struct mmt_nvidia_bind *bnd, void *state)
{
	mmt_log("bind: data1: 0x%08x, data2: 0x%08x\n", bnd->data1, bnd->data2);
}

static void demmt_nv_create_driver_object(struct mmt_nvidia_create_driver_object *create, void *state)
{
	mmt_log("create driver object: obj1: 0x%08x, obj2: 0x%08x, addr: 0x%08lx\n", create->obj1, create->obj2, create->addr);
}

static void demmt_nv_create_device_object(struct mmt_nvidia_create_device_object *create, void *state)
{
	mmt_log("create device object: obj1: 0x%08x\n", create->obj1);
}

static void demmt_nv_create_context_object(struct mmt_nvidia_create_context_object *create, void *state)
{
	mmt_log("create context object: obj1: 0x%08x\n", create->obj1);
}

static void demmt_nv_call_method_data(struct mmt_nvidia_call_method_data *call, void *state)
{
}

static void demmt_nv_ioctl_4d(struct mmt_nvidia_ioctl_4d *ctl, void *state)
{
	mmt_log("ioctl4d: %s\n", ctl->str.data);
}

static void demmt_nv_mmiotrace_mark(struct mmt_nvidia_mmiotrace_mark *mark, void *state)
{
}

const struct mmt_nvidia_decode_funcs demmt_funcs =
{
	{ demmt_memread, demmt_memwrite, demmt_mmap, demmt_munmap, demmt_mremap, demmt_open },
	demmt_nv_create_object,
	demmt_nv_destroy_object,
	demmt_nv_ioctl_pre,
	demmt_nv_ioctl_post,
	demmt_nv_memory_dump,
	demmt_nv_memory_dump_cont,
	demmt_nv_call_method,
	demmt_nv_create_mapped,
	demmt_nv_create_dma_object,
	demmt_nv_alloc_map,
	demmt_nv_gpu_map,
	demmt_nv_gpu_unmap,
	demmt_nv_mmap,
	demmt_nv_unmap,
	demmt_nv_bind,
	demmt_nv_create_driver_object,
	demmt_nv_create_device_object,
	demmt_nv_create_context_object,
	demmt_nv_call_method_data,
	demmt_nv_ioctl_4d,
	demmt_nv_mmiotrace_mark
};

static void usage()
{
	fprintf(stderr, "Usage: demmt [OPTION]\n"
			"Decodes binary trace files generated by Valgrind MMT. Reads standard input.\n\n"
			"  -m 'chipset'\tset chipset version\n"
			"  -c\t\tdo not \"compress\" obvious buffer clears\n"
			"  -i\t\tdo not guess invalid pushbufs\n"
			"  -d\t\thide invalid pushbufs\n"
			"  -e\t\tdo not decode invalid pushbufs\n"
			"  -b\t\tenable hacky IB trick detection, will be removed when proper IB support lands\n"
			"  -n id\t\tassume buffer \"id\" contains IB entries\n"
			"  -o\t\tdump ioctl data\n"
			"\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int i;
	if (argc < 2)
		usage();

	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-m"))
		{
			if (i + 1 >= argc)
				usage();
			char *chip = argv[++i];
			if (strncasecmp(argv[i], "NV", 2) == 0)
				chip += 2;
			chipset = strtoul(chip, NULL, 16);
		}
		else if (!strcmp(argv[i], "-c"))
			compress_clears = 0;
		else if (!strcmp(argv[i], "-i"))
			guess_invalid_pushbuf = 0;
		else if (!strcmp(argv[i], "-d"))
			invalid_pushbufs_visible = 0;
		else if (!strcmp(argv[i], "-e"))
			decode_invalid_buffers = 0;
		else if (!strcmp(argv[i], "-b"))
			m2mf_hack_enabled = 1;
		else if (!strcmp(argv[i], "-n"))
		{
			if (i + 1 >= argc)
				usage();
			ib_buffer = strtoul(argv[++i], NULL, 10);
		}
		else if (!strcmp(argv[i], "-o"))
			dump_ioctls = 1;
		else
			usage();
	}

	if (chipset == 0)
		usage();

	/* set up an rnn context */
	rnn_init();
	rnndb = rnn_newdb();
	rnn_parsefile(rnndb, "fifo/nv_objects.xml");
	rnn_prepdb(rnndb);
	domain = rnn_finddomain(rnndb, "NV01_SUBCHAN");

	mmt_decode(&demmt_funcs.base, NULL);
	dump_buffered_writes(1);
	return 0;
}
