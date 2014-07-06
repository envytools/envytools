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

#include "demmt.h"
#include "dis.h"
#include <stdlib.h>

static uint64_t code_address = 0;
static struct buffer *code_buffer = NULL;
static uint64_t m2mf_dest_address = 0;
static struct buffer *m2mf_buffer = NULL;
static int m2mf_data_cur_offset;

static struct buffer *find_buffer_by_gpu_address(uint64_t addr)
{
	struct buffer *buf, *ret = NULL;

	for (buf = buffers_list; buf != NULL; buf = buf->next)
	{
		if (addr >= buf->gpu_start && addr < buf->gpu_start + buf->length)
		{
			ret = buf;
			break;
		}
	}

	if (ret == NULL)
		for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
		{
			if (addr >= buf->gpu_start && addr < buf->gpu_start + buf->length)
			{
				ret = buf;
				break;
			}
		}

	if (ret && ret->data == NULL)
		ret->data = calloc(ret->length, 1);

	return ret;
}

static const struct disisa *isa_nvc0 = NULL;

void demmt_parse_command(uint32_t class_, int mthd, uint32_t data)
{
	if (!disassembly_shaders)
		return;

	if (class_ == 0x9197) // NVC1_3D
	{
		if (mthd == 0x1608) // CODE_ADDRESS_HIGH
		{
			mmt_debug("code address high: 0x%08x\n", data);
			code_address = ((uint64_t)data) << 32;
		}
		else if (mthd == 0x160c) // CODE_ADDRESS_LOW
		{
			mmt_debug("code address low: 0x%08x\n", data);
			code_address |= data;
			code_buffer = find_buffer_by_gpu_address(code_address);
			mmt_debug("code address: 0x%08lx, buffer found: %d\n", code_address, code_buffer ? 1 : 0);
		}
		else if (code_buffer && mthd >= 0x2000 && mthd < 0x2000 + 0x40 * 6 && (mthd & 0x4) == 4) // SP
		{
			int i;
			for (i = 0; i < 6; ++i)
				if (mthd == 0x2004 + i * 0x40) // SP[i].START_ID
				{
					mmt_debug("start id[%d]: 0x%08x\n", i, data);
					struct region *reg;
					for (reg = code_buffer->written_regions; reg != NULL; reg = reg->next)
					{
						if (reg->start == data)
						{
							if (MMT_DEBUG)
							{
								uint32_t x;
								mmt_debug("CODE: %s\n", "");
								for (x = reg->start + 20 * 4; x < reg->end; x += 4)
									mmt_debug("0x%08x ", *(uint32_t *)(code_buffer->data + x));
								mmt_debug("%s\n", "");
							}

							if (!isa_nvc0)
								isa_nvc0 = ed_getisa("nvc0");
							struct varinfo *var = varinfo_new(isa_nvc0->vardata);

							envydis(isa_nvc0, stdout, code_buffer->data + reg->start + 20 * 4, 0, \
									reg->end - reg->start - 20 * 4, var, 0, NULL, 0,
									/*1 ? &envy_def_colors :*/ &envy_null_colors);
							varinfo_del(var);
							break;
						}
					}
					break;
				}
		}
	}
	else if (class_ == 0x9039) // NVC0_M2MF
	{
		if (mthd == 0x0238) // OFFSET_OUT_HIGH
		{
			mmt_debug("m2mf offset out high: 0x%08x\n", data);
			m2mf_dest_address = ((uint64_t)data) << 32;
			m2mf_buffer = NULL;
		}
		else if (mthd == 0x023c) // OFFSET_OUT_LOW
		{
			mmt_debug("m2mf offset out low: 0x%08x\n", data);
			m2mf_dest_address |= data;
		}
		else if (mthd == 0x0300) // EXEC
		{
			int flags_ok = (data & 0x111) == 0x111 ? 1 : 0;
			mmt_debug("m2mf exec: 0x%08x push&linear: %d\n", data, flags_ok);
			if (flags_ok)
				m2mf_buffer = find_buffer_by_gpu_address(m2mf_dest_address);

			if (!flags_ok || m2mf_buffer == NULL)
			{
				m2mf_dest_address = 0;
				m2mf_buffer = NULL;
			}
			if (m2mf_buffer)
				m2mf_data_cur_offset = m2mf_dest_address - m2mf_buffer->gpu_start;
		}
		else if (mthd == 0x0304) // DATA
		{
			mmt_debug("m2mf data: 0x%08x\n", data);
			if (m2mf_buffer)
			{
				buffer_register_write(m2mf_buffer, m2mf_data_cur_offset, 4, &data);
				m2mf_data_cur_offset += 4;
			}
		}
	}
}
