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

#include "buffer_decode.h"
#include "nvrm.h"
#include "config.h"
#include "log.h"

static int pb_pointer_found = 0;//TODO: per device

void buffer_decode_register_write(struct cpu_mapping *mapping, uint32_t start, uint32_t len)
{
	char pushbuf_desc[1024];
	char comment[50];
	comment[0] = 0;
	pushbuf_desc[0] = 0;

	unsigned char *data = mapping->data;
	uint32_t addr = start;
	uint32_t left = len;

	uint64_t gpu_addr = cpu_mapping_to_gpu_addr(mapping, start);
	if (gpu_addr)
		gpu_addr -= start;

	if (mapping->fdtype == FDNVIDIA && !pb_pointer_found)
	{
		struct gpu_object *dev = nvrm_get_device(mapping->object);
		int chipset = nvrm_get_chipset(dev);
		int ib_supported = chipset == 0x50 || chipset >= 0x80;

		if (ib_supported)
		{
			uint32_t idx = start / 4;
			uint32_t *data = (uint32_t *)mapping->data;
			if ((idx & 1) == 1 && data[idx - 1] && data[idx] && !(data[idx - 1] & 0x3))
			{
				if (gpu_addr)
				{
					struct gpu_object *fifo = nvrm_get_fifo(mapping->object, gpu_addr + addr, 1);
					if (fifo && is_fifo_and_addr_belongs(fifo, gpu_addr + addr))
					{
						pb_pointer_found = 1;
						mapping->ib.entries = get_fifo_state(fifo)->ib.entries;
					}
				}

				if (!pb_pointer_found)
				{
					uint64_t pb_gpu_addr = (((uint64_t)(data[idx] & 0xff)) << 32) | (data[idx - 1] & 0xfffffffc);
					struct gpu_object *obj;
					for (obj = gpu_objects; obj != NULL; obj = obj->next)
					{
						struct gpu_mapping *gpu_mapping;
						for (gpu_mapping = obj->gpu_mappings; gpu_mapping != NULL; gpu_mapping = gpu_mapping->next)
						{
							if (gpu_mapping->address == pb_gpu_addr &&
								gpu_mapping->length >=  4 * ((data[idx] & 0x7fffffff) >> 10))
							{
								pb_pointer_found = 1;
								break;
							}
						}

						if (pb_pointer_found)
							break;
					}
				}

				if (pb_pointer_found)
				{
					if (info && decode_pb)
						mmt_printf("IB buffer: %d\n", mapping->id);

					mapping->ib.is = 1;
					mapping->ib.offset = start - 4;
				}
			}
		}
		else
		{
			if (start == 0x40)
			{
				uint32_t pb_gpu_addr = *(uint32_t *)&(mapping->data[0x40]);
				if (pb_gpu_addr)
				{
					struct gpu_object *fifo = nvrm_get_fifo(mapping->object, gpu_addr + addr, 1);
					if (fifo && is_fifo_and_addr_belongs(fifo, gpu_addr + addr))
						pb_pointer_found = 1;

					if (!pb_pointer_found)
					{
						struct gpu_mapping *gpu_mapping = gpu_mapping_find(pb_gpu_addr, dev);
						if (gpu_mapping)
							pb_pointer_found = 1;
					}

					if (pb_pointer_found)
					{
						if (info && decode_pb)
							mmt_printf("USER buffer: %d\n", mapping->id);

						mapping->user.is = 1;
					}
				}
			}
		}
	}

	if (mapping->ib.is)
	{
		if (addr >= mapping->ib.offset && addr < mapping->length)
			mapping->ib.state.pstate.fifo = nvrm_get_fifo(mapping->object, gpu_addr + addr, 0);
	}
	else if (mapping->user.is)
		mapping->user.state.pstate.fifo = nvrm_get_fifo(mapping->object, gpu_addr + addr, 0);

	while (addr < start + len)
	{
		if (print_gpu_addresses && gpu_addr)
			sprintf(comment, " (gpu=0x%08" PRIx64 ")", gpu_addr + addr);

		if (left >= 4)
		{
			if (mapping->ib.is &&
					addr >= mapping->ib.offset &&
					addr < mapping->length &&
					(!mapping->ib.entries || addr < mapping->ib.offset + 8 * mapping->ib.entries))
			{
				if ((addr & 4) == 4)
				{
					ib_decode(&mapping->ib.state, *(uint32_t *)(data + addr - 4), pushbuf_desc);
					ib_decode(&mapping->ib.state, *(uint32_t *)(data + addr), pushbuf_desc);
				}

				if (dump_memory_writes)
					mmt_printf("w %d:0x%04x%s, 0x%08x  %s\n", mapping->id, addr, comment, *(uint32_t *)(data + addr), pushbuf_desc);

				if ((addr & 4) == 4)
					ib_decode_end(&mapping->ib.state);
			}
			else if (mapping->user.is)
			{
				user_decode(&mapping->user.state, addr, *(uint32_t *)(data + addr), pushbuf_desc);

				if (dump_memory_writes)
					mmt_printf("w %d:0x%04x%s, 0x%08x  %s\n", mapping->id, addr, comment, *(uint32_t *)(data + addr), pushbuf_desc);

				user_decode_end(&mapping->user.state);
			}
			else
			{
				if (dump_memory_writes)
					mmt_printf("w %d:0x%04x%s, 0x%08x\n", mapping->id, addr, comment, *(uint32_t *)(data + addr));
			}

			addr += 4;
			left -= 4;
		}
		else if (left >= 2)
		{
			if (dump_memory_writes)
				mmt_printf("w %d:0x%04x%s, 0x%04x\n", mapping->id, addr, comment, *(uint16_t *)(data + addr));
			addr += 2;
			left -= 2;
		}
		else
		{
			if (dump_memory_writes)
				mmt_printf("w %d:0x%04x%s, 0x%02x\n", mapping->id, addr, comment, *(uint8_t *)(data + addr));
			++addr;
			--left;
		}
	}
}
