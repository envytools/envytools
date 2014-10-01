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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "config.h"
#include "log.h"

struct buffer *buffers[MAX_ID] = { NULL };

struct buffer *buffers_list = NULL;
struct buffer *gpu_only_buffers_list = NULL; // merge it into buffers_list?

static int wreg_count = 0;
static int writes_since_last_dump = 0;
static uint32_t last_wreg_id = UINT32_MAX;
struct unk_map *unk_maps = NULL; // merge it into buffers_list?

uint32_t pb_pointer_buffer = UINT32_MAX;
uint32_t pb_pointer_offset = 0;
int find_pb_pointer = 0;
int force_pushbuf_decoding = 0;

static void dump(struct buffer *buf)
{
	mmt_log("currently buffered writes for id: %d:\n", buf->id);
	dump_regions(&buf->written_regions);
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
	pushbuf_desc[0] = 0;

	if (find_pb_pointer || (!is_nouveau && pb_pointer_buffer == UINT32_MAX))
	{
		cur = buf->written_regions.head;

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
						if ((info && decode_pb) || find_pb_pointer)
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
						if ((info && decode_pb) || find_pb_pointer)
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

	cur = buf->written_regions.head;
	mmt_debug("currently buffered writes for id: %d:\n", buf->id);
	while (cur)
	{
		mmt_debug("<0x%08x, 0x%08x>\n", cur->start, cur->end);
		unsigned char *data = buf->data;
		uint32_t addr = cur->start;

		int left = cur->end - addr;
		if ((buf->type & PUSH) && *(uint32_t *)(data + addr) == 0)
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
			if (dump_memory_writes)
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

					if (dump_memory_writes)
						fprintf(stdout, "w %d:0x%04x%s, 0x%08x  %s\n", buf->id, addr, comment[0], *(uint32_t *)(data + addr), pushbuf_desc);
				}
				else if (buf->type & PUSH)
				{
					if (!force_pushbuf_decoding)
						pushbuf_desc[0] = 0;
					else
						pushbuf_decode(state, *(uint32_t *)(data + addr), pushbuf_desc, 0);

					state->next_command_offset = addr + 4;

					if (dump_memory_writes)
						fprintf(stdout, "w %d:0x%04x%s, 0x%08x  %s%s\n", buf->id, addr, comment[0], *(uint32_t *)(data + addr), state->pushbuf_invalid ? "INVALID " : "", pushbuf_desc);
				}
				else if (buf->type & USER)
				{
					user_decode(ustate, addr, *(uint32_t *)(data + addr), pushbuf_desc);

					if (dump_memory_writes)
						fprintf(stdout, "w %d:0x%04x%s, 0x%08x  %s\n", buf->id, addr, comment[0], *(uint32_t *)(data + addr), pushbuf_desc);
				}

				addr += 4;
				left -= 4;
			}
			else if (left >= 2)
			{
				if (dump_memory_writes)
					fprintf(stdout, "w %d:0x%04x%s, 0x%04x\n", buf->id, addr, comment[0], *(uint16_t *)(data + addr));
				addr += 2;
				left -= 2;
			}
			else
			{
				if (dump_memory_writes)
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

static void clear_buffered_writes()
{
	struct buffer *buf;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		free_regions(&buf->written_regions);
	wreg_count = 0;
	last_wreg_id = UINT32_MAX;
}

static void dump_buffered_writes()
{
	struct buffer *buf;
	if (MMT_DEBUG)
	{
		mmt_log("CURRENTLY BUFFERED WRITES, number of buffers: %d, buffered writes: %d bytes\n",
				wreg_count, writes_since_last_dump);
		for (buf = buffers_list; buf != NULL; buf = buf->next)
			if (buf->written_regions.head)
				dump(buf);
		mmt_log("%s\n", "END OF CURRENTLY BUFFERED WRITES");
	}

	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->written_regions.head)
			dump_writes(buf);

	writes_since_last_dump = 0;

	clear_buffered_writes();
}

void buffer_flush()
{
	dump_buffered_writes();
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

	if (!regions_add_range(&buf->written_regions, offset, len))
		dump_and_abort(buf);
}

static void buffer_dump(struct buffer *buf)
{
	mmt_log("buffer %d, len: 0x%08lx, mmap_offset: 0x%08lx, cpu_addr: 0x%016lx, gpu_addr: 0x%016lx, "
			"data1: 0x%08lx, data2: 0x%08lx, type: %d, ib_offset: 0x%08x, usage: %s\n",
			buf->id, buf->length, buf->mmap_offset, buf->cpu_start, buf->gpu_start,
			buf->data1, buf->data2, buf->type, buf->ib_offset,
			buf->usage[0].desc ? buf->usage[0].desc : "");
}

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

void buffer_mmap(uint32_t id, uint64_t cpu_start, uint64_t len, uint64_t mmap_offset,
		const uint64_t *data1, const uint64_t *data2)
{
	struct buffer *buf;
	uint64_t pg = sysconf(_SC_PAGESIZE);
	len = (len + pg - 1) & ~(pg - 1);

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

void buffer_munmap(uint32_t id)
{
	struct buffer *buf = buffers[id];

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

void buffer_mremap(struct mmt_mremap *mm)
{
	if (wreg_count)
	{
		mmt_debug("mremap, flushing buffered writes%s\n", "");
		dump_buffered_writes();
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

void buffer_ioctl_pre(int print_raw)
{
	if (wreg_count)
	{
		if (print_raw)
			mmt_log_cont(", flushing buffered writes%s\n", "");
		else
			mmt_debug("flushing buffered writes%s\n", "");
		dump_buffered_writes();
		mmt_debug("%s\n", "");
	}
	else if (print_raw)
		mmt_log_cont(", no dirty buffers%s\n", "");
}

void buffer_register_mmt_read(struct mmt_read *r)
{
	if (wreg_count)
	{
		mmt_debug("%s\n", "read registered, flushing currently buffered writes");
		dump_buffered_writes();
		mmt_debug("%s\n", "");
	}
}

void buffer_register_mmt_write(struct mmt_write *w)
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

	if (last_wreg_id != id)
	{
		if (last_wreg_id == UINT32_MAX)
			last_wreg_id = id;
		else
		{
			if (wreg_count)
			{
				mmt_debug("new region write registered (new: %d, old: %d), flushing buffered writes\n",
						id, last_wreg_id);
				dump_buffered_writes();

				mmt_debug("%s\n", "");
				last_wreg_id = id;
			}
		}
	}

	int wreg_existed = buf->written_regions.head != NULL;

	buffer_register_write(buf, w->offset, w->len, w->data);

	if (!wreg_existed)
		wreg_count++;
	writes_since_last_dump += w->len;
}

void buffer_free(struct buffer *buf)
{
	int i;
	buffer_remove(buf);
	free_regions(&buf->written_regions);
	for (i = 0; i < MAX_USAGES; ++i)
		free(buf->usage[i].desc);
	free(buf->data);
	free(buf);
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
