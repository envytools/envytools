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

#include "buffer_decode.h"
#include "nvrm.h"
#include "config.h"
#include "log.h"

static int pb_pointer_found = 0;//TODO: per device

void flush_written_regions(struct cpu_mapping *mapping)
{
	struct region *cur;
	char pushbuf_desc[1024];
	char comment[2][50];
	comment[0][0] = 0;
	comment[1][0] = 0;
	pushbuf_desc[0] = 0;
	struct gpu_object *dev = nvrm_get_device(mapping->object);
	int chipset = nvrm_get_chipset(dev);
	int ib_supported = chipset == 0x50 || chipset >= 0x80;

	if (!is_nouveau && !pb_pointer_found)
	{
		cur = mapping->written_regions.head;

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
				uint32_t *data = (uint32_t *)mapping->data;
				if (!data[idx] || !data[idx + 1] || (data[idx] & 0x3))
				{
					cur = cur->next;
					continue;
				}

				uint64_t gpu_addr = (((uint64_t)(data[idx + 1] & 0xff)) << 32) | (data[idx] & 0xfffffffc);

				struct gpu_object *obj;
				for (obj = gpu_objects; obj != NULL; obj = obj->next)
				{
					struct gpu_mapping *gpu_mapping;
					for (gpu_mapping = obj->gpu_mappings; gpu_mapping != NULL; gpu_mapping = gpu_mapping->next)
						if (gpu_mapping->address == gpu_addr &&
							gpu_mapping->length >=  4 * ((data[idx + 1] & 0x7fffffff) >> 10))
						{
							if (info && decode_pb)
								fprintf(stdout, "IB buffer: %d\n", mapping->id);

							pb_pointer_found = 1;
							mapping->ib.is = 1;
							mapping->ib.offset = cur->start;
							break;
						}

					if (mapping->ib.is)
						break;
				}

				if (mapping->ib.is)
					break;
			}
			else
			{
				if (mapping->user.is) // already checked
				{
					cur = cur->next;
					continue;
				}
				if (cur->start != 0x40 || cur->end < 0x44)
				{
					cur = cur->next;
					continue;
				}
				uint32_t gpu_addr = *(uint32_t *)&(mapping->data[0x40]);
				if (!gpu_addr)
				{
					cur = cur->next;
					continue;
				}

				struct gpu_mapping *gpu_mapping = gpu_mapping_find(gpu_addr, dev);
				if (!gpu_mapping)
				{
					cur = cur->next;
					continue;
				}

				if (info && decode_pb)
					fprintf(stdout, "USER buffer: %d\n", mapping->id);

				pb_pointer_found = 1;
				mapping->user.is = 1;
				break;
			}

			cur = cur->next;
		}
	}

	cur = mapping->written_regions.head;
	mmt_debug("currently buffered writes for id: %d:\n", mapping->id);
	while (cur)
	{
		mmt_debug("<0x%08x, 0x%08x>\n", cur->start, cur->end);
		unsigned char *data = mapping->data;
		uint32_t addr = cur->start;

		uint64_t gpu_addr = cpu_mapping_to_gpu_addr(mapping, cur->start);
		if (gpu_addr)
			gpu_addr -= cur->start;

		int left = cur->end - addr;
		if (!mapping->ib.is && !mapping->user.is &&
				*(uint32_t *)(data + addr) == 0)
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

			mmt_debug("all zeroes between 0x%04x and 0x%04x\n", addr_start, addr);
			if (print_gpu_addresses && gpu_addr)
			{
				sprintf(comment[0], " (gpu=0x%08lx)", gpu_addr + addr_start);
				sprintf(comment[1], " (gpu=0x%08lx)", gpu_addr + addr);
			}
			if (dump_memory_writes)
				fprintf(stdout, "w %d:0x%04x%s-0x%04x%s, 0x00000000\n", mapping->id, addr_start, comment[0], addr, comment[1]);
		}

		if (mapping->ib.is)
		{
			if (addr >= mapping->ib.offset && addr < mapping->length)
			{
				ib_decode_start(&mapping->ib.state);
				mapping->ib.state.pstate.fifo = nvrm_get_fifo(mapping->object, gpu_addr + addr);
			}
		}
		else if (mapping->user.is)
		{
			if (0)
				user_decode_start(&mapping->user.state);
			mapping->user.state.pstate.fifo = nvrm_get_fifo(mapping->object, gpu_addr + addr);
		}

		while (addr < cur->end)
		{
			if (print_gpu_addresses && gpu_addr)
				sprintf(comment[0], " (gpu=0x%08lx)", gpu_addr + addr);

			if (left >= 4)
			{
				if (mapping->ib.is && addr >= mapping->ib.offset && addr < mapping->length)
				{
					ib_decode(&mapping->ib.state, *(uint32_t *)(data + addr), pushbuf_desc);

					if (dump_memory_writes)
						fprintf(stdout, "w %d:0x%04x%s, 0x%08x  %s\n", mapping->id, addr, comment[0], *(uint32_t *)(data + addr), pushbuf_desc);
				}
				else if (mapping->user.is)
				{
					user_decode(&mapping->user.state, addr, *(uint32_t *)(data + addr), pushbuf_desc);

					if (dump_memory_writes)
						fprintf(stdout, "w %d:0x%04x%s, 0x%08x  %s\n", mapping->id, addr, comment[0], *(uint32_t *)(data + addr), pushbuf_desc);
				}
				else
				{
					if (dump_memory_writes)
						fprintf(stdout, "w %d:0x%04x%s, 0x%08x\n", mapping->id, addr, comment[0], *(uint32_t *)(data + addr));
				}

				addr += 4;
				left -= 4;
			}
			else if (left >= 2)
			{
				if (dump_memory_writes)
					fprintf(stdout, "w %d:0x%04x%s, 0x%04x\n", mapping->id, addr, comment[0], *(uint16_t *)(data + addr));
				addr += 2;
				left -= 2;
			}
			else
			{
				if (dump_memory_writes)
					fprintf(stdout, "w %d:0x%04x%s, 0x%02x\n", mapping->id, addr, comment[0], *(uint8_t *)(data + addr));
				++addr;
				--left;
			}
		}

		if (mapping->ib.is && addr >= mapping->ib.offset && addr < mapping->length)
			ib_decode_end(&mapping->ib.state);
		else if (mapping->user.is)
			user_decode_end(&mapping->user.state);

		cur = cur->next;
	}
	mmt_debug("end of buffered writes for id: %d\n", mapping->id);
}
