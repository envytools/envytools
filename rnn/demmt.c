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
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>

#include "mmt_bin_decode.h"
#include "mmt_bin_decode_nvidia.h"
#include "demmt.h"
#include "demmt_drm.h"
#include "demmt_macro.h"
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
static int compress_clears = 1;
static uint32_t pb_pointer_buffer = UINT32_MAX;
static int dump_ioctls = 0;
static int print_gpu_addresses = 0;

struct rnndomain *domain;
struct rnndb *rnndb;
static struct rnndb *rnndb_nv50_texture;
static struct rnndb *rnndb_nvc0_shaders;
struct rnndeccontext *nv50_texture_ctx;
struct rnndeccontext *nvc0_shaders_ctx;
struct rnndomain *tsc_domain;
struct rnndomain *tic_domain;
struct rnndomain *nvc0_vp_header_domain, *nvc0_fp_header_domain, *nvc0_gp_header_domain, *nvc0_tcp_header_domain, *nvc0_tep_header_domain;

int chipset;
int ib_supported;
int guess_invalid_pushbuf = 1;
int invalid_pushbufs_visible = 1;
int decode_invalid_buffers = 1;
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
	struct region *cur = buf->written_regions;
	struct pushbuf_decode_state *state = &buf->state.pushbuf;
	struct ib_decode_state *ibstate = &buf->state.ib;
	struct user_decode_state *ustate = &buf->state.user;
	char pushbuf_desc[1024];
	char comment[2][50];
	comment[0][0] = 0;
	comment[1][0] = 0;

	if (find_pb_pointer || (!is_nouveau && pb_pointer_buffer == UINT32_MAX))
	{
		do
		{
			if (ib_supported)
			{
				if (cur->start != 0 || cur->end < 8)
					break;
				uint32_t *data = (uint32_t *)buf->data;
				if (!data[0] || !data[1])
					break;
				if (data[0] & 0x3)
					break;
				uint64_t gpu_addr = (((uint64_t)(data[1] & 0xff)) << 32) | (data[0] & 0xfffffffc);
				struct buffer *buf2;
				for (buf2 = buffers_list; buf2 != NULL; buf2 = buf2->next)
				{
					if (buf2->gpu_start == gpu_addr &&
						buf2->length >= 4 * ((data[1] & 0x7fffffff) >> 10))
					{
						if (!quiet)
							fprintf(stdout, "%sIB buffer: %d\n", find_pb_pointer ? "possible " : "", buf->id);
						pb_pointer_buffer = buf->id;
						buf->type = IB;
						break;
					}
				}
			}
			else
			{
				if (buf->type == USER) // already checked
					break;
				if (cur->start != 0x40 || cur->end < 0x44)
					break;
				uint32_t gpu_addr = *(uint32_t *)&(buf->data[0x40]);
				if (!gpu_addr)
					break;

				struct buffer *buf2;
				for (buf2 = buffers_list; buf2 != NULL; buf2 = buf2->next)
				{
					if (gpu_addr >= buf2->gpu_start && gpu_addr < buf2->gpu_start + buf2->length)
					{
						if (!quiet)
							fprintf(stdout, "%sUSER buffer: %d\n", find_pb_pointer ? "possible " : "", buf->id);
						pb_pointer_buffer = buf->id;
						buf->type = USER;
						break;
					}
				}
			}
		} while (0);

		if (find_pb_pointer)
			return;
	}

	mmt_log("currently buffered writes for id: %d:\n", buf->id);
	while (cur)
	{
		mmt_log("<0x%08x, 0x%08x>\n", cur->start, cur->end);
		unsigned char *data = buf->data;
		uint32_t addr = cur->start;

		int left = cur->end - addr;
		if (compress_clears && !quiet && buf->type == PUSH && *(uint32_t *)(data + addr) == 0)
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
			if (print_gpu_addresses && buf->gpu_start)
			{
				sprintf(comment[0], " (gpu=0x%08lx)", buf->gpu_start + addr_start);
				sprintf(comment[1], " (gpu=0x%08lx)", buf->gpu_start + addr);
			}
			fprintf(stdout, "w %d:0x%04x%s-0x%04x%s, 0x00000000\n", buf->id, addr_start, comment[0], addr, comment[1]);
		}

		if (buf->type == IB)
			ib_decode_start(ibstate);
		else if (buf->type == PUSH)
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
		else if (buf->type == USER)
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
				if (buf->type == IB)
				{
					ib_decode(ibstate, *(uint32_t *)(data + addr), pushbuf_desc);
					if (!quiet)
						fprintf(stdout, "w %d:0x%04x%s, 0x%08x  %s\n", buf->id, addr, comment[0], *(uint32_t *)(data + addr), pushbuf_desc);
				}
				else if (buf->type == PUSH)
				{
					if (!force_pushbuf_decoding)
						pushbuf_desc[0] = 0;
					else if (state->pushbuf_invalid == 0 || decode_invalid_buffers)
						pushbuf_decode(state, *(uint32_t *)(data + addr), pushbuf_desc, 0);
					else
						pushbuf_desc[0] = 0;

					if (state->pushbuf_invalid == 1 && invalid_pushbufs_visible == 0)
						break;

					state->next_command_offset = addr + 4;
					if (!quiet)
						fprintf(stdout, "w %d:0x%04x%s, 0x%08x  %s%s\n", buf->id, addr, comment[0], *(uint32_t *)(data + addr), state->pushbuf_invalid ? "INVALID " : "", pushbuf_desc);
				}
				else if (buf->type == USER)
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

		if (buf->type == IB)
			ib_decode_end(ibstate);
		else if (buf->type == PUSH)
		{
			if (force_pushbuf_decoding)
				pushbuf_decode_end(state);
		}
		else if (buf->type == USER)
			user_decode_end(ustate);

		cur = cur->next;
	}
	mmt_log("end of buffered writes for id: %d\n", buf->id);
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
	mmt_log("CURRENTLY BUFFERED WRITES, number of buffers: %d, buffered writes: %d (0x%x) (/4 = %d (0x%x))\n",
			wreg_count, writes_since_last_full_dump, writes_since_last_full_dump,
			writes_since_last_full_dump / 4, writes_since_last_full_dump / 4);
	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->written_regions)
			dump(buf);
	mmt_log("%s\n", "END OF CURRENTLY BUFFERED WRITES");
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
		if (last_wreg_id == UINT32_MAX)
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
	struct buffer *buf = buffers[w->id];
	int wreg_existed = buf->written_regions != NULL;
	memwrite(w);
	check_sanity(buf);
	check_coverage(w, buf);
	if (!wreg_existed)
		wreg_count++;
	writes_since_last_full_dump += w->len;
}

struct unk_map
{
	uint32_t data1;
	uint32_t data2;
	uint64_t mmap_offset;
	struct unk_map *next;
};

static void buffer_remove(struct buffer *buf)
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

static void buffer_free(struct buffer *buf)
{
	buffer_remove(buf);
	free_written_regions(buf);
	free(buf->data);
	free(buf);
}

static struct unk_map *unk_maps = NULL; // merge it into buffers_list?

static void demmt_mmap(struct mmt_mmap *mm, void *state)
{
	mmt_log("mmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx\n",
			(void *)mm->start, mm->len, mm->id, mm->offset);
	struct buffer *buf;
	buf = calloc(1, sizeof(struct buffer));
	buf->id = mm->id;
	buf->data = calloc(mm->len, 1);
	buf->cpu_start = mm->start;
	buf->length = mm->len;
	buf->mmap_offset = mm->offset;
	if (mm->id == pb_pointer_buffer)
	{
		if (ib_supported)
			buf->type = IB;
		else
			buf->type = USER;
	}
	if (buffers_list)
		buffers_list->prev = buf;
	buf->next = buffers_list;

	buffers[buf->id] = buf;
	buffers_list = buf;

	struct unk_map *tmp = unk_maps, *prev = NULL;
	while (tmp)
	{
		if (tmp->mmap_offset == mm->offset)
		{
			mmt_log("binding data1: 0x%08x, data2: 0x%08x to buffer id: %d\n", tmp->data1, tmp->data2, mm->id);
			buf->data1 = tmp->data1;
			buf->data2 = tmp->data2;

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

	struct buffer *gpubuf;
	for (gpubuf = gpu_only_buffers_list; gpubuf != NULL; gpubuf = gpubuf->next)
	{
		if (buf->mmap_offset == gpubuf->mmap_offset)
		{
			buf->gpu_start = gpubuf->gpu_start;
			buf->data1 = gpubuf->data1;
			buf->data2 = gpubuf->data2;
			if (gpubuf->data && gpubuf->length == buf->length)
				memcpy(buf->data, gpubuf->data, buf->length);
			buffer_free(gpubuf);
			break;
		}
	}
}

static void demmt_munmap(struct mmt_unmap *mm, void *state)
{
	mmt_log("munmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx\n",
			(void *)mm->start, mm->len, mm->id, mm->offset, mm->data1, mm->data2);

	struct buffer *buf = buffers[mm->id];

	buffer_remove(buf);
	buf->cpu_start = 0;
	buf->next = gpu_only_buffers_list;
	if (gpu_only_buffers_list)
		gpu_only_buffers_list->prev = buf;
	gpu_only_buffers_list = buf;
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

static void demmt_nv_ioctl_pre(struct mmt_nvidia_ioctl_pre *ctl, void *state)
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
	{ }
	print_raw = print_raw || dump_ioctls;

	if (print_raw)
	{
		mmt_log("ioctl pre  0x%02x (0x%08x), dir: %2s, size: %4d", nr, ctl->id, dir_desc[dir], size);
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
				mmt_log("flushing buffered writes%s\n", "");
			dump_buffered_writes(1);
			clear_buffered_writes();
			mmt_log("%s\n", "");
		}
		else if (print_raw)
			mmt_log_cont(", no dirty buffers%s\n", "");
	}
	else if (print_raw)
		mmt_log_cont("%s\n", "");
}

static void demmt_nv_ioctl_post(struct mmt_nvidia_ioctl_post *ctl, void *state)
{
	uint8_t dir, type, nr;
	uint16_t size;
	decode_ioctl_id(ctl->id, &dir, &type, &nr, &size);
	int print_raw = 0;

	if (type == 0x64) // DRM
		print_raw = demmt_drm_ioctl_post(dir, nr, size, &ctl->data, state);
	else if (type == 0x46) // nvidia
	{ }
	print_raw = print_raw || dump_ioctls;

	if (print_raw)
	{
		mmt_log("ioctl post 0x%02x (0x%08x), dir: %2s, size: %4d", nr, ctl->id, dir_desc[dir], size);
		if (size != ctl->data.len)
			mmt_log_cont(", data.len: %d", ctl->data.len);
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
	mmt_log("create mapped object: mmap_offset: %p, data1: 0x%08x, data2: 0x%08x, type: 0x%08x\n",
			(void *)p->mmap_offset, p->data1, p->data2, p->type);

	if (p->mmap_offset == 0)
		return;

	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->mmap_offset == p->mmap_offset)
		{
			buf->data1 = p->data1;
			buf->data2 = p->data2;
			return;
		}

	for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
	{
		if (buf->data1 == p->data1 && buf->data2 == p->data2)
		{
			mmt_log("TODO: gpu only buffer found, demmt_nv_create_mapped needs to be updated!%s\n", "");
			break;
		}
	}

	struct unk_map *m = malloc(sizeof(struct unk_map));
	m->data1 = p->data1;
	m->data2 = p->data2;
	m->mmap_offset = p->mmap_offset;
	m->next = unk_maps;
	unk_maps = m;
}

static void demmt_nv_create_dma_object(struct mmt_nvidia_create_dma_object *create, void *state)
{
	mmt_log("create dma object, name: 0x%08x, type: 0x%08x, parent: 0x%08x\n",
			create->name, create->type, create->parent);
}

static void demmt_nv_alloc_map(struct mmt_nvidia_alloc_map *alloc, void *state)
{
	mmt_log("allocate map: mmap_offset: %p, data1: 0x%08x, data2: 0x%08x\n",
			(void *)alloc->mmap_offset, alloc->data1, alloc->data2);

	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->mmap_offset == alloc->mmap_offset)
		{
			buf->data1 = alloc->data1;
			buf->data2 = alloc->data2;
			return;
		}

	for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
	{
		if (buf->data1 == alloc->data1 && buf->data2 == alloc->data2)
		{
			mmt_log("gpu only buffer found, merging%s\n", "");
			buf->mmap_offset = alloc->mmap_offset;
			return;
		}
	}

	struct unk_map *m = malloc(sizeof(struct unk_map));
	m->data1 = alloc->data1;
	m->data2 = alloc->data2;
	m->mmap_offset = alloc->mmap_offset;
	m->next = unk_maps;
	unk_maps = m;
}

void register_gpu_only_buffer(uint64_t gpu_start, int len, uint64_t mmap_offset, uint64_t data1, uint64_t data2)
{
	struct buffer *buf;
	mmt_log("registering gpu only buffer, gpu_address: 0x%lx, size: 0x%x\n", gpu_start, len);
	buf = calloc(1, sizeof(struct buffer));
	buf->id = -1;
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

static void demmt_nv_gpu_map(uint32_t data1, uint32_t data2, uint32_t data3, uint64_t gpu_start, uint32_t len, void *state)
{
	mmt_log("gpu map: data1: 0x%08x, data2: 0x%08x, data3: 0x%08x, gpu_start: 0x%08lx, len: 0x%08x\n",
			data1, data2, data3, gpu_start, len);
	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->data1 == data1 && buf->data2 == data3 && buf->length == len)
		{
			buf->gpu_start = gpu_start;
			mmt_log("setting gpu address for buffer %d to 0x%08lx\n", buf->id, buf->gpu_start);
			return;
		}

	struct unk_map *tmp;
	for (tmp = unk_maps; tmp != NULL; tmp = tmp->next)
	{
		if (tmp->data1 == data1 && tmp->data2 == data3)
		{
			mmt_log("TODO: unk buffer found, demmt_nv_gpu_map needs to be updated!%s\n", "");
			break;
		}
	}

	register_gpu_only_buffer(gpu_start, len, 0, data1, data3);
}

static void demmt_nv_gpu_map1(struct mmt_nvidia_gpu_map *map, void *state)
{
	demmt_nv_gpu_map(map->data1, map->data2, map->data3, map->gpu_start, map->len, state);
}

static void demmt_nv_gpu_map2(struct mmt_nvidia_gpu_map2 *map, void *state)
{
	demmt_nv_gpu_map(map->data1, map->data2, map->data3, map->gpu_start, map->len, state);
}

static void demmt_nv_gpu_unmap(uint32_t data1, uint32_t data2, uint32_t data3, uint64_t gpu_start, void *state)
{
	mmt_log("gpu unmap: data1: 0x%08x, data2: 0x%08x, data3: 0x%08x, gpu_start: 0x%08lx\n",
			data1, data2, data3, gpu_start);
	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->data1 == data1 && buf->data2 == data3 &&
				buf->gpu_start == gpu_start)
		{
			mmt_log("clearing gpu address for buffer %d (was: 0x%08lx)\n", buf->id, buf->gpu_start);
			buf->gpu_start = 0;
			return;
		}

	for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
	{
		if (buf->data1 == data1 && buf->data2 == data3 && buf->gpu_start == gpu_start)
		{
			mmt_log("deregistering gpu only buffer of size %ld\n", buf->length);
			buffer_free(buf);
			return;
		}
	}

	mmt_log("gpu only buffer not found%s\n", "");
}

static void demmt_nv_gpu_unmap1(struct mmt_nvidia_gpu_unmap *unmap, void *state)
{
	demmt_nv_gpu_unmap(unmap->data1, unmap->data2, unmap->data3, unmap->gpu_start, state);
}

static void demmt_nv_gpu_unmap2(struct mmt_nvidia_gpu_unmap2 *unmap, void *state)
{
	demmt_nv_gpu_unmap(unmap->data1, unmap->data2, unmap->data3, unmap->gpu_start, state);
}

static void demmt_nv_mmap(struct mmt_nvidia_mmap *mm, void *state)
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
		buf = calloc(1, sizeof(struct buffer));

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
			buf->type = IB;
		else
			buf->type = USER;
	}

	if (buffers_list)
		buffers_list->prev = buf;
	buf->next = buffers_list;

	buffers[buf->id] = buf;
	buffers_list = buf;
}

static void demmt_nv_unmap(struct mmt_nvidia_unmap *mm, void *state)
{
	mmt_log("nv_munmap: mmap_offset: %p, data1: 0x%08x, data2: 0x%08x\n",
			(void *)mm->mmap_offset, mm->data1, mm->data2);

	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->mmap_offset == mm->mmap_offset)
		{
			// clear cpu_start only?
			buffer_free(buf);
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
	demmt_nv_gpu_map1,
	demmt_nv_gpu_map2,
	demmt_nv_gpu_unmap1,
	demmt_nv_gpu_unmap2,
	demmt_nv_mmap,
	demmt_nv_unmap,
	demmt_nv_bind,
	demmt_nv_create_driver_object,
	demmt_nv_create_device_object,
	demmt_nv_create_context_object,
	demmt_nv_call_method_data,
	demmt_nv_ioctl_4d,
	demmt_nv_mmiotrace_mark,
	demmt_nouveau_gem_pushbuf_data
};

static void usage()
{
	fprintf(stderr, "Usage: demmt [OPTION]\n"
			"Decodes binary trace files generated by Valgrind MMT. Reads standard input or file passed by -l.\n\n"
			"  -l file\tuse \"file\" as input\n"
			"         \t- it can be compressed by gzip, bzip2 or xz\n"
			"         \t- demmt extracts chipset version from characters following \"nv\"\n"
			"  -m 'chipset'\tset chipset version (required, but see -l)\n"
			"  -q\t\t(quiet) print only the most important data (pushbufs from IB / USER, disassembled code, TSCs, TICs, etc)\n"
			"  -c\t\tenable colors\n"
			"  -f\t\tfind possible pushbuf pointers (IB / USER)\n"
			"  -n id\t\tset pushbuf pointer to \"id\"\n"
			"  -g\t\tprint gpu addresses\n"
			"  -o\t\tdump ioctl data\n"
			"  -a\t\tdisable decoding of object state (shader disassembly, TSCs, TICs, etc)\n"
			"  -t\t\tdeindent logs\n"
			"  -x\t\tforce pushbuf decoding even without pushbuf pointer\n"
			"  -r\t\tenable verbose macro interpreter\n"
			"\n"
			"  -s\t\tdo not \"compress\" obvious buffer clears\n"
			"  -i\t\tdo not guess invalid pushbufs\n"
			"  -d\t\thide invalid pushbufs\n"
			"  -e\t\tdo not decode invalid pushbufs\n"
			"\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	const char *filename = NULL;
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
		else if (!strcmp(argv[i], "-s"))
			compress_clears = 0;
		else if (!strcmp(argv[i], "-i"))
			guess_invalid_pushbuf = 0;
		else if (!strcmp(argv[i], "-d"))
			invalid_pushbufs_visible = 0;
		else if (!strcmp(argv[i], "-e"))
			decode_invalid_buffers = 0;
		else if (!strcmp(argv[i], "-n"))
		{
			if (i + 1 >= argc)
				usage();
			pb_pointer_buffer = strtoul(argv[++i], NULL, 10);
		}
		else if (!strcmp(argv[i], "-o"))
			dump_ioctls = 1;
		else if (!strcmp(argv[i], "-g"))
			print_gpu_addresses = 1;
		else if (!strcmp(argv[i], "-f"))
			find_pb_pointer = 1;
		else if (!strcmp(argv[i], "-q"))
			quiet = 1;
		else if (!strcmp(argv[i], "-a"))
			decode_object_state = 0;
		else if (!strcmp(argv[i], "-c"))
			colors = &envy_def_colors;
		else if (!strcmp(argv[i], "-l"))
		{
			if (i + 1 >= argc)
				usage();
			filename = argv[++i];
			const char *base = basename(argv[i]);
			if (chipset == 0 && strncasecmp(base, "nv", 2) == 0)
			{
				chipset = strtoul(base + 2, NULL, 16);
				fprintf(stdout, "Chipset: NV%02X\n", chipset);
			}
		}
		else if (!strcmp(argv[i], "-t"))
			indent_logs = 0;
		else if (!strcmp(argv[i], "-x"))
			force_pushbuf_decoding = 1;
		else if (!strcmp(argv[i], "-r"))
			macro_verbose = 1;
		else
			usage();
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
	rnn_prepdb(rnndb);
	domain = rnn_finddomain(rnndb, "NV01_SUBCHAN");

	rnndb_nv50_texture = rnn_newdb();
	rnn_parsefile(rnndb_nv50_texture, "graph/nv50_texture.xml");
	rnn_prepdb(rnndb_nv50_texture);

	nv50_texture_ctx = rnndec_newcontext(rnndb_nv50_texture);
	nv50_texture_ctx->colors = colors;

	rnndb_nvc0_shaders = rnn_newdb();
	rnn_parsefile(rnndb_nvc0_shaders, "graph/nvc0_shaders.xml");
	rnn_prepdb(rnndb_nvc0_shaders);

	nvc0_shaders_ctx = rnndec_newcontext(rnndb_nvc0_shaders);
	nvc0_shaders_ctx->colors = colors;

	struct rnnvalue *v = NULL;
	struct rnnenum *chs = rnn_findenum(rnndb, "chipset");
	FINDARRAY(chs->vals, v, v->value == (uint64_t)chipset);
	rnndec_varadd(nv50_texture_ctx, "chipset", v ? v->name : "NV01");
	tic_domain = rnn_finddomain(rnndb_nv50_texture, "TIC");
	tsc_domain = rnn_finddomain(rnndb_nv50_texture, "TSC");

	nvc0_vp_header_domain = rnn_finddomain(rnndb_nvc0_shaders, "NVC0_VP_HEADER");
	nvc0_fp_header_domain = rnn_finddomain(rnndb_nvc0_shaders, "NVC0_FP_HEADER");
	nvc0_gp_header_domain = rnn_finddomain(rnndb_nvc0_shaders, "NVC0_GP_HEADER");
	nvc0_tcp_header_domain = rnn_finddomain(rnndb_nvc0_shaders, "NVC0_TCP_HEADER");
	nvc0_tep_header_domain = rnn_finddomain(rnndb_nvc0_shaders, "NVC0_TEP_HEADER");

	if (filename)
	{
		close(0);
		if (open_input(filename) == NULL)
		{
			perror("open");
			exit(1);
		}
	}

	mmt_decode(&demmt_funcs.base, NULL);
	dump_buffered_writes(1);
	return 0;
}
