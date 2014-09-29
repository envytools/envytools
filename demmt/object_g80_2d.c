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

#include "object.h"

static struct
{
	struct addr_n_buf dst;
	struct addr_n_buf src;

	uint32_t dst_linear;
	int check_dst_buffer;
	int data_offset;
} g80_2d;

static struct mthd2addr g80_2d_addresses[] =
{
	{ 0x0220, 0x0224, &g80_2d.dst },
	{ 0x0250, 0x0254, &g80_2d.src },
	{ 0, 0, NULL }
};

void decode_g80_2d_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, g80_2d_addresses))
	{
		if (pstate->mthd == 0x0224) // DST_ADDRESS_LOW
			g80_2d.check_dst_buffer = g80_2d.dst.buffer != NULL;
	}
}

void decode_g80_2d_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, g80_2d_addresses))
	{ }
	else if (mthd == 0x0204) // DST_LINEAR
		g80_2d.dst_linear = data;
	else if (mthd == 0x0838) // SIFC_WIDTH
	{
		if (g80_2d.dst.buffer)
			g80_2d.data_offset = g80_2d.dst.address - g80_2d.dst.buffer->gpu_start;
	}
	else if (mthd == 0x0860) // SIFC_DATA
	{
		if (g80_2d.check_dst_buffer)
		{
			g80_2d.check_dst_buffer = 0;

			if (!g80_2d.dst_linear)
				g80_2d.dst.buffer = NULL;
		}

		if (g80_2d.dst.buffer != NULL)
		{
			mmt_debug("2d sifc_data: 0x%08x\n", data);
			buffer_register_write(g80_2d.dst.buffer, g80_2d.data_offset, 4, &data);
			g80_2d.data_offset += 4;
		}
	}
}
