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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "mmt_bin_decode.h"
#include "mmt_bin_decode_nvidia.h"
#include "demmt.h"
#include "demmt_drm.h"
#include "demmt_macro.h"
#include "demmt_nvrm.h"
#include "demmt_objects.h"
#include "demmt_pushbuf.h"
#include "rnndec.h"
#include "util.h"

struct buffer *buffers[MAX_ID] = { NULL };

struct buffer *buffers_list = NULL;
struct buffer *gpu_only_buffers_list = NULL; // merge it into buffers_list?

static int wreg_count = 0;
static int writes_since_last_full_dump = 0; // NOTE: you cannot rely too much on this value - it includes overwrites
static int writes_since_last_dump = 0;
static uint32_t last_wreg_id = UINT32_MAX;
uint32_t pb_pointer_buffer = UINT32_MAX;
uint32_t pb_pointer_offset = 0;
int dump_ioctls = 0;
static int print_gpu_addresses = 0;

struct rnndomain *domain;
struct rnndb *rnndb;
static struct rnndb *rnndb_g80_texture;
static struct rnndb *rnndb_gf100_shaders;
struct rnndb *rnndb_nvrm_object;
struct rnndeccontext *g80_texture_ctx;
struct rnndeccontext *gf100_shaders_ctx;
struct rnndomain *tsc_domain;
struct rnndomain *tic_domain;
struct rnndomain *gf100_vp_header_domain, *gf100_fp_header_domain,
	*gf100_gp_header_domain, *gf100_tcp_header_domain, *gf100_tep_header_domain;

int chipset;
int ib_supported;
int find_pb_pointer = 0;
int quiet = 0;
int decode_object_state = 1;
int indent_logs = 1;
int is_nouveau = 0;
int force_pushbuf_decoding = 0;
const struct envy_colors *colors = NULL;

static void dump(struct buffer *buf)
{
	if (quiet)
		return;
	struct region *cur = buf->written_regions;
	mmt_log("currently buffered writes for id: %d:\n", buf->id);
	while (cur)
	{
		mmt_log("<0x%08x, 0x%08x>\n", cur->start, cur->end);
		cur = cur->next;
	}
	mmt_log("end of buffered writes for id: %d\n", buf->id);
}

static void dump_and_abort(struct buffer *buf)
{
	dump(buf);
	abort();
}

static void dump_writes(struct buffer *buf)
{
	struct region *cur;
	struct pushbuf_decode_state *state = &buf->state.pushbuf;
	struct ib_decode_state *ibstate = &buf->state.ib;
	struct user_decode_state *ustate = &buf->state.user;
	char pushbuf_desc[1024];
	char comment[2][50];
	comment[0][0] = 0;
	comment[1][0] = 0;

	if (find_pb_pointer || (!is_nouveau && pb_pointer_buffer == UINT32_MAX))
	{
		cur = buf->written_regions;

		while (cur)
		{
			if (ib_supported)
			{
				if (cur->end - cur->start < 8)
				{
					cur = cur->next;
					continue;
				}

				int idx = cur->start / 4;
				uint32_t *data = (uint32_t *)buf->data;
				if (!data[idx] || !data[idx + 1] || (data[idx] & 0x3))
				{
					cur = cur->next;
					continue;
				}

				uint64_t gpu_addr = (((uint64_t)(data[idx + 1] & 0xff)) << 32) | (data[idx] & 0xfffffffc);
				struct buffer *buf2;
				for (buf2 = buffers_list; buf2 != NULL; buf2 = buf2->next)
				{
					if (!buf2->gpu_start)
						continue;
					if (buf2->gpu_start == gpu_addr &&
						buf2->length >= 4 * ((data[idx + 1] & 0x7fffffff) >> 10))
					{
						if (!quiet)
							fprintf(stdout, "%sIB buffer: %d\n", find_pb_pointer ? "possible " : "", buf->id);
						pb_pointer_buffer = buf->id;
						pb_pointer_offset = cur->start;
						buf->type |= IB;
						buf->ib_offset = pb_pointer_offset;
						break;
					}
				}

				if (buf->type & IB)
					break;
			}
			else
			{
				if (buf->type == USER) // already checked
				{
					cur = cur->next;
					continue;
				}
				if (cur->start != 0x40 || cur->end < 0x44)
				{
					cur = cur->next;
					continue;
				}
				uint32_t gpu_addr = *(uint32_t *)&(buf->data[0x40]);
				if (!gpu_addr)
				{
					cur = cur->next;
					continue;
				}

				struct buffer *buf2;
				for (buf2 = buffers_list; buf2 != NULL; buf2 = buf2->next)
				{
					if (!buf2->gpu_start)
						continue;
					if (gpu_addr >= buf2->gpu_start && gpu_addr < buf2->gpu_start + buf2->length)
					{
						if (!quiet)
							fprintf(stdout, "%sUSER buffer: %d\n", find_pb_pointer ? "possible " : "", buf->id);
						pb_pointer_buffer = buf->id;
						buf->type = USER;
						break;
					}
				}
				if (buf->type & USER)
					break;
			}

			cur = cur->next;
		}

		if (find_pb_pointer)
			return;
	}

	cur = buf->written_regions;
	mmt_debug("currently buffered writes for id: %d:\n", buf->id);
	while (cur)
	{
		mmt_debug("<0x%08x, 0x%08x>\n", cur->start, cur->end);
		unsigned char *data = buf->data;
		uint32_t addr = cur->start;

		int left = cur->end - addr;
		if (!quiet && (buf->type & PUSH) && *(uint32_t *)(data + addr) == 0)
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

			mmt_debug("all zeroes between 0x%04x and 0x%04x\n", addr_start, addr);
			if (print_gpu_addresses && buf->gpu_start)
			{
				sprintf(comment[0], " (gpu=0x%08lx)", buf->gpu_start + addr_start);
				sprintf(comment[1], " (gpu=0x%08lx)", buf->gpu_start + addr);
			}
			fprintf(stdout, "w %d:0x%04x%s-0x%04x%s, 0x00000000\n", buf->id, addr_start, comment[0], addr, comment[1]);
		}

		if ((buf->type & IB) && addr >= buf->ib_offset && addr < buf->length)
			ib_decode_start(ibstate);
		else if (buf->type & PUSH)
		{
			if (force_pushbuf_decoding)
			{
				if (addr != state->next_command_offset)
				{
					mmt_log("restarting pushbuf decode on buffer %d: %x != %x\n", buf->id, addr, state->next_command_offset);
					pushbuf_decode_start(state);
				}

				if (state->pushbuf_invalid == 1)
				{
					mmt_log("restarting pushbuf decode on buffer %d\n", buf->id);
					pushbuf_decode_start(state);
				}
			}
		}
		else if (buf->type & USER)
		{
			if (0)
				user_decode_start(ustate);
		}

		while (addr < cur->end)
		{
			if (print_gpu_addresses && buf->gpu_start)
				sprintf(comment[0], " (gpu=0x%08lx)", buf->gpu_start + addr);

			if (left >= 4)
			{
				if ((buf->type & IB) && addr >= buf->ib_offset && addr < buf->length)
				{
					ib_decode(ibstate, *(uint32_t *)(data + addr), pushbuf_desc);
					if (!quiet)
						fprintf(stdout, "w %d:0x%04x%s, 0x%08x  %s\n", buf->id, addr, comment[0], *(uint32_t *)(data + addr), pushbuf_desc);
				}
				else if (buf->type & PUSH)
				{
					if (!force_pushbuf_decoding)
						pushbuf_desc[0] = 0;
					else
						pushbuf_decode(state, *(uint32_t *)(data + addr), pushbuf_desc, 0);

					state->next_command_offset = addr + 4;
					if (!quiet)
						fprintf(stdout, "w %d:0x%04x%s, 0x%08x  %s%s\n", buf->id, addr, comment[0], *(uint32_t *)(data + addr), state->pushbuf_invalid ? "INVALID " : "", pushbuf_desc);
				}
				else if (buf->type & USER)
				{
					user_decode(ustate, addr, *(uint32_t *)(data + addr), pushbuf_desc);
					if (!quiet)
						fprintf(stdout, "w %d:0x%04x%s, 0x%08x  %s\n", buf->id, addr, comment[0], *(uint32_t *)(data + addr), pushbuf_desc);
				}

				addr += 4;
				left -= 4;
			}
			else if (left >= 2)
			{
				if (!quiet)
					fprintf(stdout, "w %d:0x%04x%s, 0x%04x\n", buf->id, addr, comment[0], *(uint16_t *)(data + addr));
				addr += 2;
				left -= 2;
			}
			else
			{
				if (!quiet)
					fprintf(stdout, "w %d:0x%04x%s, 0x%02x\n", buf->id, addr, comment[0], *(uint8_t *)(data + addr));
				++addr;
				--left;
			}
		}

		if ((buf->type & IB) && addr >= buf->ib_offset && addr < buf->length)
			ib_decode_end(ibstate);
		else if (buf->type & PUSH)
		{
			if (force_pushbuf_decoding)
				pushbuf_decode_end(state);
		}
		else if (buf->type & USER)
			user_decode_end(ustate);

		cur = cur->next;
	}
	mmt_debug("end of buffered writes for id: %d\n", buf->id);
}

static void check_sanity(struct buffer *buf)
{
	struct region *cur = buf->written_regions;
	if (!cur)
		return;

	if (cur->prev)
	{
		mmt_error("%s", "start->prev != NULL\n");
		dump_and_abort(buf);
	}

	while (cur)
	{
		if (cur->start >= cur->end)
		{
			mmt_error("cur->start >= cur->end 0x%x 0x%x\n", cur->start, cur->end);
			dump_and_abort(buf);
		}

		if (cur->next)
		{
			if (cur->end >= cur->next->start)
			{
				mmt_error("cur->end >= cur->next->start 0x%x 0x%x\n", cur->end, cur->next->start);
				dump_and_abort(buf);
			}
		}

		cur = cur->next;
	}
}

static void check_coverage(const struct mmt_write *w, struct buffer *buf)
{
	struct region *cur = buf->written_regions;

	while (cur)
	{
		if (w->offset >= cur->start && w->offset + w->len <= cur->end)
			return;

		cur = cur->next;
	}

	mmt_error("region <0x%08x, 0x%08x> was not added!\n", w->offset, w->offset + w->len);
	dump_and_abort(buf);
}

static void drop_entry(struct region *reg, struct buffer *buf)
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
		buf->written_regions = next;
	}

	if (next)
		next->prev = prev;
	else
	{
		mmt_debug("new last is at <0x%08x, 0x%08x>\n", prev->start, prev->end);
		buf->written_region_last = prev;
	}
}

static void maybe_merge_with_next(struct region *cur, struct buffer *buf)
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
		drop_entry(cur->next, buf);

		// and see if next^2 should be merged
		maybe_merge_with_next(cur, buf);
	}
	else
	{
		// now next ends after this one

		// extend current end
		mmt_debug("extending entry <0x%08x, 0x%08x> right to 0x%08x\n",
				cur->start, cur->end, cur->next->end);
		cur->end = cur->next->end;

		// and drop next entry
		drop_entry(cur->next, buf);
	}
}

static void maybe_merge_with_previous(struct region *cur, struct buffer *buf)
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
		drop_entry(cur->prev, buf);

		// and see if previous^2 should be merged
		maybe_merge_with_previous(cur, buf);
	}
	else
	{
		// now previous starts before this one

		// extend current start
		mmt_debug("extending entry <0x%08x, 0x%08x> left to 0x%08x\n",
				cur->start, cur->end, cur->prev->start);
		cur->start = cur->prev->start;

		// and drop previous entry
		drop_entry(cur->prev, buf);
	}
}

void buffer_register_write(struct buffer *buf, uint32_t offset, uint8_t len, const void *data)
{
	if (buf->length < offset + len)
	{
		mmt_error("buffer %d is too small (%ld) for write starting at %d and length %d\n",
				buf->id, buf->length, offset, len);
		dump_and_abort(buf);
	}

	memcpy(buf->data + offset, data, len);

	struct region *cur = buf->written_regions;

	if (cur == NULL)
	{
		buf->written_regions = cur = malloc(sizeof(struct region));
		cur->start = offset;
		cur->end = offset + len;
		cur->prev = NULL;
		cur->next = NULL;
		buf->written_region_last = cur;
		return;
	}

	struct region *last_reg = buf->written_region_last;
	if (offset == last_reg->end)
	{
		mmt_debug("extending last entry <0x%08x, 0x%08x> right by %d\n",
				last_reg->start, last_reg->end, len);
		last_reg->end += len;
		return;
	}

	if (offset > last_reg->end)
	{
		mmt_debug("adding last entry <0x%08x, 0x%08x> after <0x%08x, 0x%08x>\n",
				offset, offset + len, last_reg->start, last_reg->end);
		cur = malloc(sizeof(struct region));
		cur->start = offset;
		cur->end = offset + len;
		cur->prev = last_reg;
		cur->next = NULL;
		last_reg->next = cur;
		buf->written_region_last = cur;
		return;
	}

	if (offset + len > last_reg->end)
	{
		mmt_debug("extending last entry <0x%08x, 0x%08x> right to 0x%08x\n",
				last_reg->start, last_reg->end, offset + len);
		last_reg->end = offset + len;
		if (offset < last_reg->start)
		{
			mmt_debug("... and extending last entry <0x%08x, 0x%08x> left to 0x%08x\n",
					last_reg->start, last_reg->end, offset);
			last_reg->start = offset;
			maybe_merge_with_previous(last_reg, buf);
		}
		return;
	}


	while (offset + len > cur->end) // if we will ever crash on cur being NULL then it means we screwed up earlier
	{
		if (cur->end == offset) // optimization
		{
			mmt_debug("extending entry <0x%08x, 0x%08x> by %d\n", cur->start, cur->end, len);
			cur->end += len;
			maybe_merge_with_next(cur, buf);
			return;
		}
		cur = cur->next;
	}

	// now it ends before end of current

	// does it start in this one?
	if (offset >= cur->start)
		return;

	// does it end before start of current one?
	if (offset + len < cur->start)
	{
		mmt_debug("adding new entry <0x%08x, 0x%08x> before <0x%08x, 0x%08x>\n",
				offset, offset + len, cur->start, cur->end);
		struct region *tmp = malloc(sizeof(struct region));
		tmp->start = offset;
		tmp->end = offset + len;
		tmp->prev = cur->prev;
		tmp->next = cur;
		if (cur->prev)
			cur->prev->next = tmp;
		else
			buf->written_regions = tmp;
		cur->prev = tmp;
		maybe_merge_with_previous(tmp, buf);
		return;
	}

	// now it ends in current and starts before
	cur->start = offset;
	maybe_merge_with_previous(cur, buf);
}

static void memwrite(const struct mmt_write *w)
{
	uint32_t id = w->id;
	if (id >= MAX_ID)
	{
		mmt_error("id >= %d\n", MAX_ID);
		abort();
	}

	struct buffer *buf = buffers[id];
	if (buf == NULL)
	{
		mmt_error("buffer %d does not exist\n", id);
		abort();
	}

	buffer_register_write(buf, w->offset, w->len, w->data);
}

static void dump_buffered_writes(int full)
{
	struct buffer *buf;
	if (MMT_DEBUG)
	{
		mmt_log("CURRENTLY BUFFERED WRITES, number of buffers: %d, buffered writes: %d (0x%x) (/4 = %d (0x%x))\n",
				wreg_count, writes_since_last_full_dump, writes_since_last_full_dump,
				writes_since_last_full_dump / 4, writes_since_last_full_dump / 4);
		for (buf = buffers_list; buf != NULL; buf = buf->next)
			if (buf->written_regions)
				dump(buf);
		mmt_log("%s\n", "END OF CURRENTLY BUFFERED WRITES");
	}

	if (full)
	{
		for (buf = buffers_list; buf != NULL; buf = buf->next)
			if (buf->written_regions)
				dump_writes(buf);
		writes_since_last_full_dump = 0;
	}

	writes_since_last_dump = 0;
}

static void free_written_regions(struct buffer *buf)
{
	struct region *cur = buf->written_regions, *next;
	if (!cur)
		return;

	while (cur)
	{
		next = cur->next;
		free(cur);
		cur = next;
	}

	buf->written_regions = NULL;
	buf->written_region_last = NULL;
}

static void clear_buffered_writes()
{
	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		free_written_regions(buf);
	wreg_count = 0;
	last_wreg_id = UINT32_MAX;
}

static void demmt_memread(struct mmt_read *w, void *state)
{
	if (find_pb_pointer)
		return;

	if (wreg_count)
	{
		if (1)
		{
			mmt_debug("%s\n", "read registered, flushing currently buffered writes");
			dump_buffered_writes(1);
			clear_buffered_writes();
			mmt_debug("%s\n", "");
		}
		else
		{
			if (writes_since_last_dump)
			{
				mmt_debug("%s\n", "read registered, NOT flushing currently buffered writes");
				dump_buffered_writes(0);
				mmt_debug("%s\n", "");
			}
			else
			{
				//mmt_log("%s\n", "read registered, NO new writes registered, NOT flushing buffered writes");
			}
		}
	}

	char comment[50];
	struct buffer *buf = buffers[w->id];
	if (print_gpu_addresses && buf->gpu_start)
		sprintf(comment, " (gpu=0x%08lx)", buf->gpu_start + w->offset);
	else
		comment[0] = 0;

	if (!quiet)
	{
		unsigned char *data = &w->data[0];
		if (w->len == 1)
			fprintf(stdout, "r %d:0x%04x%s, 0x%02x\n", w->id, w->offset, comment, data[0]);
		else if (w->len == 2)
			fprintf(stdout, "r %d:0x%04x%s, 0x%04x\n", w->id, w->offset, comment, *(uint16_t *)&data[0]);
		else if (w->len == 4 || w->len == 8 || w->len == 16 || w->len == 32)
		{
			fprintf(stdout, "r %d:0x%04x%s, ", w->id, w->offset, comment);
			int i;
			for (i = 0; i < w->len; i += 4)
				fprintf(stdout, "0x%08x ", *(uint32_t *)&data[i]);
			fprintf(stdout, "\n");
		}
	}
}

static void demmt_memwrite(struct mmt_write *w, void *state)
{
	if (last_wreg_id != w->id)
	{
		if (last_wreg_id == UINT32_MAX)
			last_wreg_id = w->id;
		else
		{
			if (wreg_count)
			{
				mmt_debug("new region write registered (new: %d, old: %d), flushing buffered writes\n",
						w->id, last_wreg_id);
				dump_buffered_writes(1);
				clear_buffered_writes();
				mmt_debug("%s\n", "");
				last_wreg_id = w->id;
			}
		}
	}
	struct buffer *buf = buffers[w->id];
	int wreg_existed = buf->written_regions != NULL;
	memwrite(w);
	check_sanity(buf);
	check_coverage(w, buf);
	if (!wreg_existed)
		wreg_count++;
	writes_since_last_dump += w->len;
	writes_since_last_full_dump += w->len;
}

void buffer_dump(struct buffer *buf)
{
	mmt_log("buffer %d, len: 0x%08lx, mmap_offset: 0x%08lx, cpu_addr: 0x%016lx, gpu_addr: 0x%016lx, "
			"data1: 0x%08lx, data2: 0x%08lx, type: %d, ib_offset: 0x%08x, usage: %s\n",
			buf->id, buf->length, buf->mmap_offset, buf->cpu_start, buf->gpu_start,
			buf->data1, buf->data2, buf->type, buf->ib_offset,
			buf->usage[0].desc ? buf->usage[0].desc : "");
}

void buffer_remove(struct buffer *buf)
{
	if (buf->prev)
		buf->prev->next = buf->next;
	if (buf->next)
		buf->next->prev = buf->prev;

	if (buf->id >= 0)
	{
		buffers[buf->id] = NULL;
		if (buffers_list == buf)
			buffers_list = buf->next;
		buf->id = -1;
	}
	else
	{
		if (gpu_only_buffers_list == buf)
			gpu_only_buffers_list = buf->next;
	}

	buf->next = NULL;
	buf->prev = NULL;
}

void buffer_free(struct buffer *buf)
{
	int i;
	buffer_remove(buf);
	free_written_regions(buf);
	for (i = 0; i < MAX_USAGES; ++i)
		free(buf->usage[i].desc);
	free(buf->data);
	free(buf);
}

struct unk_map *unk_maps = NULL; // merge it into buffers_list?

void __demmt_mmap(uint32_t id, uint64_t cpu_start, uint64_t len, uint64_t mmap_offset,
		const uint64_t *data1, const uint64_t *data2)
{
	struct buffer *buf;

	for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
	{
		if ((data1 && buf->data1 == *data1 && data2 && buf->data2 == *data2)
			|| (buf->mmap_offset == mmap_offset))
		{
			mmt_debug("gpu only buffer found (0x%016lx), merging\n", buf->gpu_start);
			if (buf->gpu_start == 0)
			{
				mmt_error("buffer with gpu address == 0 on gpu buffer list, wtf?%s\n", "");
				buffer_dump(buf);
				abort();
			}
			buffer_remove(buf);
			break;
		}
	}

	if (!buf)
	{
		buf = calloc(1, sizeof(struct buffer));
		buf->type = PUSH;
	}

	buf->id = id;

	if (!buf->data)
		buf->data = calloc(len, 1);

	buf->cpu_start = cpu_start;

	if (buf->length)
	{
		if (len != buf->length)
			buf->data = realloc(buf->data, len);

		if (len > buf->length)
			memset(buf->data + buf->length, 0, len - buf->length);
	}
	buf->length = len;

	if (buf->mmap_offset && buf->mmap_offset != mmap_offset)
		mmt_error("different mmap offset of gpu only buffer 0x%lx != 0x%lx\n", buf->mmap_offset, mmap_offset);
	buf->mmap_offset = mmap_offset;

	if (data1)
		buf->data1 = *data1;
	if (data2)
		buf->data2 = *data2;

	struct unk_map *tmp = unk_maps, *prev = NULL;
	while (tmp)
	{
		if (tmp->mmap_offset == mmap_offset)
		{
			if (buf->data1 != tmp->data1 || buf->data2 != tmp->data2)
			{
				mmt_debug("binding data1: 0x%08x, data2: 0x%08x to buffer id: %d\n", tmp->data1, tmp->data2, id);
				buf->data1 = tmp->data1;
				buf->data2 = tmp->data2;
			}

			if (tmp == unk_maps)
				unk_maps = tmp->next;
			else
				prev->next = tmp->next;
			free(tmp);

			break;
		}
		prev = tmp;
		tmp = tmp->next;
	}

	if (id == pb_pointer_buffer)
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

static void demmt_mmap(struct mmt_mmap *mm, void *state)
{
	mmt_log("mmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx\n",
			(void *)mm->start, mm->len, mm->id, mm->offset);
	__demmt_mmap(mm->id, mm->start, mm->len, mm->offset, NULL, NULL);
}

static void demmt_mmap2(struct mmt_mmap2 *mm, void *state)
{
	mmt_log("mmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx, fd: %d\n",
			(void *)mm->start, mm->len, mm->id, mm->offset, mm->fd);
	__demmt_mmap(mm->id, mm->start, mm->len, mm->offset, NULL, NULL);
}

static void demmt_munmap(struct mmt_unmap *mm, void *state)
{
	mmt_log("munmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx\n",
			(void *)mm->start, mm->len, mm->id, mm->offset, mm->data1, mm->data2);

	struct buffer *buf = buffers[mm->id];

	if (buf->gpu_start)
	{
		buffer_remove(buf);
		buf->cpu_start = 0;
		buf->next = gpu_only_buffers_list;
		if (gpu_only_buffers_list)
			gpu_only_buffers_list->prev = buf;
		gpu_only_buffers_list = buf;
	}
	else
		buffer_free(buf);
}

static void demmt_mremap(struct mmt_mremap *mm, void *state)
{
	mmt_log("mremap: old_address: %p, new_address: %p, old_length: 0x%08lx, new_length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx\n",
			(void *)mm->old_start, (void *)mm->start, mm->old_len, mm->len, mm->id, mm->offset, mm->data1, mm->data2);

	if (wreg_count)
	{
		mmt_debug("mremap, flushing buffered writes%s\n", "");
		dump_buffered_writes(1);
		clear_buffered_writes();
		mmt_debug("%s\n", "");
	}

	struct buffer *buf = buffers[mm->id];

	if (mm->len != buf->length)
	{
		buf->data = realloc(buf->data, mm->len);
		if (mm->len > buf->length)
			memset(buf->data + buf->length, 0, mm->len - buf->length);
	}

	buf->cpu_start = mm->start;
	buf->length = mm->len;
	buf->mmap_offset = mm->offset;
	buf->data1 = mm->data1;
	buf->data2 = mm->data2;
}

static void demmt_open(struct mmt_open *o, void *state)
{
	mmt_log("sys_open: %s, flags: 0x%x, mode: 0x%x, ret: %d\n", o->path.data, o->flags, o->mode, o->ret);
}

static void demmt_msg(uint8_t *data, int len, void *state)
{
	if (!find_pb_pointer && !quiet)
	{
		mmt_log("MSG: %s", "");
		fwrite(data, 1, len, stdout);
		fprintf(stdout, "\n");
	}
}

static void demmt_write_syscall(struct mmt_write_syscall *o, void *state)
{
	fwrite(o->data.data, 1, o->data.len, stdout);
}

static void ioctl_data_print(struct mmt_buf *data)
{
	uint32_t i;
	if (!dump_ioctls)
		return;

	mmt_log_cont(", data:%s", "");
	for (i = 0; i < data->len / 4; ++i)
		mmt_log_cont(" 0x%08x", ((uint32_t *)data->data)[i]);
}

static void decode_ioctl_id(uint32_t id, uint8_t *dir, uint8_t *type, uint8_t *nr, uint16_t *size)
{
	*nr =    id & 0x000000ff;
	*type = (id & 0x0000ff00) >> 8;
	*size = (id & 0x3fff0000) >> 16;
	*dir =  (id & 0xc0000000) >> 30;
}

static char *dir_desc[] = { "?", "w", "r", "rw" };

void demmt_ioctl_pre(struct mmt_ioctl_pre *ctl, void *state, struct mmt_memory_dump *args, int argc)
{
	uint8_t dir, type, nr;
	uint16_t size;
	decode_ioctl_id(ctl->id, &dir, &type, &nr, &size);
	int print_raw = 1;

	if (type == 0x64) // DRM
	{
		is_nouveau = 1;
		print_raw = demmt_drm_ioctl_pre(dir, nr, size, &ctl->data, state);
	}
	else if (type == 0x46) // nvidia
		print_raw = demmt_nv_ioctl_pre(ctl->fd, ctl->id, dir, nr, size, &ctl->data, state, args, argc);

	print_raw = print_raw || dump_ioctls;

	if (print_raw)
	{
		mmt_log("ioctl pre  0x%02x (0x%08x), fd: %d, dir: %2s, size: %4d",
				nr, ctl->fd, ctl->id, dir_desc[dir], size);
		if (size != ctl->data.len)
			mmt_log_cont(", data.len: %d", ctl->data.len);

		ioctl_data_print(&ctl->data);
	}

	if (1)
	{
		if (wreg_count)
		{
			if (print_raw)
				mmt_log_cont(", flushing buffered writes%s\n", "");
			else
				mmt_debug("flushing buffered writes%s\n", "");
			dump_buffered_writes(1);
			clear_buffered_writes();
			mmt_debug("%s\n", "");
		}
		else if (print_raw)
			mmt_log_cont(", no dirty buffers%s\n", "");
	}
	else if (print_raw)
		mmt_log_cont("%s\n", "");
}

void demmt_ioctl_post(struct mmt_ioctl_post *ctl, void *state, struct mmt_memory_dump *args, int argc)
{
	uint8_t dir, type, nr;
	uint16_t size;
	decode_ioctl_id(ctl->id, &dir, &type, &nr, &size);
	int print_raw = 0;

	if (type == 0x64) // DRM
		print_raw = demmt_drm_ioctl_post(dir, nr, size, &ctl->data, state);
	else if (type == 0x46) // nvidia
		print_raw = demmt_nv_ioctl_post(ctl->fd, ctl->id, dir, nr, size, &ctl->data, state, args, argc);

	print_raw = print_raw || dump_ioctls;

	if (print_raw)
	{
		mmt_log("ioctl post 0x%02x (0x%08x), fd: %d, dir: %2s, size: %4d",
				nr, ctl->id, ctl->fd, dir_desc[dir], size);
		if (size != ctl->data.len)
			mmt_log_cont(", data.len: %d", ctl->data.len);
		ioctl_data_print(&ctl->data);
		mmt_log_cont("%s\n", "");
	}
}

void register_gpu_only_buffer(uint64_t gpu_start, int len, uint64_t mmap_offset, uint64_t data1, uint64_t data2)
{
	struct buffer *buf;
	mmt_debug("registering gpu only buffer, gpu_address: 0x%lx, size: 0x%x\n", gpu_start, len);
	buf = calloc(1, sizeof(struct buffer));
	buf->id = -1;
	buf->type = PUSH;
	//will allocate when needed
	//buf->data = calloc(map->len, 1);
	buf->cpu_start = 0;
	buf->gpu_start = gpu_start;
	buf->length = len;
	buf->mmap_offset = mmap_offset;
	buf->data1 = data1;
	buf->data2 = data2;
	if (gpu_only_buffers_list)
		gpu_only_buffers_list->prev = buf;
	buf->next = gpu_only_buffers_list;
	gpu_only_buffers_list = buf;
}

const struct mmt_nvidia_decode_funcs demmt_funcs =
{
	{ demmt_memread, demmt_memwrite, demmt_mmap, demmt_mmap2, demmt_munmap, demmt_mremap, demmt_open, demmt_msg, demmt_write_syscall },
	NULL,
	NULL,
	demmt_ioctl_pre,
	demmt_ioctl_post,
	demmt_memory_dump,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	demmt_nv_mmap,
	demmt_nv_mmap2,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	demmt_nv_call_method_data,
	demmt_nv_ioctl_4d,
	demmt_nv_mmiotrace_mark,
	demmt_nouveau_gem_pushbuf_data
};

static void usage()
{
	fprintf(stderr, "Usage: demmt [OPTION]\n"
			"Decodes binary trace files generated by Valgrind MMT. Reads standard input or file passed by -l.\n\n"
			"  -l file\t\tuse \"file\" as input\n"
			"         \t\t- it can be compressed by gzip, bzip2 or xz\n"
			"         \t\t- demmt extracts chipset version from characters following \"nv\"\n"
			"  -m 'chipset'\t\tset chipset version (required, but see -l)\n"
			"  -q\t\t\t(quiet) print only the most important data (pushbufs from IB / USER, disassembled code, TSCs, TICs, etc)\n"
			"  -c 0/1\t\tdisable/enable colors (default: 0)\n"
			"  -g 0/1\t\tdisable/enable printing of gpu addresses (default: 0)\n"
			"  -o 0/1\t\tdisable/enable dumping of raw ioctl data (default: 0)\n"
			"  -i 0/1\t\tdisable/enable indenting of logs (default: 1)\n"
			"  -r 0/1\t\tdisable/enable verbose macro interpreter (default: 0)\n"
			"  -f\t\t\tfind possible pushbuf pointers (IB / USER)\n"
			"  -n id[,offset]\tset pushbuf pointer to \"id\" and optionally offset within this buffer to \"offset\"\n"
			"  -a\t\t\tdisable decoding of object state (shader disassembly, TSCs, TICs, etc)\n"
			"  -x\t\t\tforce pushbuf decoding even without pushbuf pointer\n"
			"\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	char *filename = NULL;
	if (argc < 2)
		usage();

	int c;
	while ((c = getopt (argc, argv, "m:n:o:g:fqac:l:i:xr:h")) != -1)
	{
		switch (c)
		{
			case 'm':
			{
				char *chip = optarg;
				if (strncasecmp(chip, "NV", 2) == 0)
					chip += 2;
				chipset = strtoul(chip, NULL, 16);
				break;
			}
			case 'n':
			{
				char *endptr;
				pb_pointer_buffer = strtoul(optarg, &endptr, 10);
				if (endptr && endptr[0] == ',')
					pb_pointer_offset = strtoul(endptr + 1, NULL, 0);
				break;
			}
			case 'o':
				if (optarg[0] == '1')
					dump_ioctls = 1;
				else if (optarg[0] == '0')
					dump_ioctls = 0;
				else
				{
					fprintf(stderr, "-o accepts only 0 and 1\n");
					exit(1);
				}
				break;
			case 'g':
				if (optarg[0] == '1')
					print_gpu_addresses = 1;
				else if (optarg[0] == '0')
					print_gpu_addresses = 0;
				else
				{
					fprintf(stderr, "-g accepts only 0 and 1\n");
					exit(1);
				}
				break;
			case 'f':
				find_pb_pointer = 1;
				break;
			case 'q':
				quiet = 1;
				break;
			case 'a':
				decode_object_state = 0;
				break;
			case 'c':
				if (optarg[0] == '1')
					colors = &envy_def_colors;
				else if (optarg[0] == '0')
					colors = &envy_null_colors;
				else
				{
					fprintf(stderr, "-c accepts only 0 and 1\n");
					exit(1);
				}
				break;
			case 'l':
			{
				filename = strdup(optarg);
				const char *base = basename(filename);
				if (chipset == 0 && strncasecmp(base, "nv", 2) == 0)
				{
					chipset = strtoul(base + 2, NULL, 16);
					fprintf(stdout, "Chipset: NV%02X\n", chipset);
				}
				break;
			}
			case 'i':
				if (optarg[0] == '1')
					indent_logs = 1;
				else if (optarg[0] == '0')
					indent_logs = 0;
				else
				{
					fprintf(stderr, "-i accepts only 0 and 1\n");
					exit(1);
				}
				break;
			case 'x':
				force_pushbuf_decoding = 1;
				break;
			case 'r':
				if (optarg[0] == '1')
					macro_verbose = 1;
				else if (optarg[0] == '0')
					macro_verbose = 0;
				else
				{
					fprintf(stderr, "-r accepts only 0 and 1\n");
					exit(1);
				}
				break;
			case 'h':
			case '?':
				usage();
				break;
		}
	}

	if (chipset == 0)
		usage();
	ib_supported = chipset >= 0x80 || chipset == 0x50;

	if (!colors)
		colors = &envy_null_colors;

	/* set up an rnn context */
	rnn_init();
	rnndb = rnn_newdb();
	rnn_parsefile(rnndb, "fifo/nv_objects.xml");
	if (rnndb->estatus)
		abort();
	rnn_prepdb(rnndb);
	domain = rnn_finddomain(rnndb, "SUBCHAN");
	if (!domain)
		abort();

	rnndb_g80_texture = rnn_newdb();
	rnn_parsefile(rnndb_g80_texture, "graph/g80_texture.xml");
	if (rnndb_g80_texture->estatus)
		abort();
	rnn_prepdb(rnndb_g80_texture);

	g80_texture_ctx = rnndec_newcontext(rnndb_g80_texture);
	g80_texture_ctx->colors = colors;

	rnndb_gf100_shaders = rnn_newdb();
	rnn_parsefile(rnndb_gf100_shaders, "graph/gf100_shaders.xml");
	if (rnndb_gf100_shaders->estatus)
		abort();
	rnn_prepdb(rnndb_gf100_shaders);

	gf100_shaders_ctx = rnndec_newcontext(rnndb_gf100_shaders);
	gf100_shaders_ctx->colors = colors;

	rnndb_nvrm_object = rnn_newdb();
	rnn_parsefile(rnndb_nvrm_object, "../docs/nvrm/rnndb/nvrm_object.xml");
	if (rnndb_nvrm_object->estatus)
		abort();
	rnn_prepdb(rnndb_nvrm_object);

	struct rnnvalue *v = NULL;
	struct rnnenum *chs = rnn_findenum(rnndb, "chipset");
	FINDARRAY(chs->vals, v, v->value == (uint64_t)chipset);
	rnndec_varadd(g80_texture_ctx, "chipset", v ? v->name : "NV1");
	tic_domain = rnn_finddomain(rnndb_g80_texture, "TIC");
	tsc_domain = rnn_finddomain(rnndb_g80_texture, "TSC");

	gf100_vp_header_domain = rnn_finddomain(rnndb_gf100_shaders, "GF100_VP_HEADER");
	gf100_fp_header_domain = rnn_finddomain(rnndb_gf100_shaders, "GF100_FP_HEADER");
	gf100_gp_header_domain = rnn_finddomain(rnndb_gf100_shaders, "GF100_GP_HEADER");
	gf100_tcp_header_domain = rnn_finddomain(rnndb_gf100_shaders, "GF100_TCP_HEADER");
	gf100_tep_header_domain = rnn_finddomain(rnndb_gf100_shaders, "GF100_TEP_HEADER");
	if (!gf100_vp_header_domain || !gf100_fp_header_domain || !gf100_gp_header_domain ||
			!gf100_tcp_header_domain || !gf100_tep_header_domain)
		abort();

	if (filename)
	{
		close(0);
		if (open_input(filename) == NULL)
		{
			perror("open");
			exit(1);
		}
		free(filename);
	}

	mmt_decode(&demmt_funcs.base, NULL);
	dump_buffered_writes(1);
	return 0;
}
