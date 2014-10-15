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

#include "buffer.h"
#include "config.h"
#include "object.h"

static struct
{
	struct addr_n_buf offset_in;
	struct addr_n_buf offset_out;

	uint32_t linear_in;
	uint32_t linear_out;
	uint32_t line_length_in;
	uint32_t line_count;
} g80_m2mf;

static struct mthd2addr g80_m2mf_addresses[] =
{
	{ 0x0238, 0x030c, &g80_m2mf.offset_in },
	{ 0x023c, 0x0310, &g80_m2mf.offset_out },
	{ 0, 0, NULL }
};

void decode_g80_m2mf_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, g80_m2mf_addresses))
	{ }
}

void decode_g80_m2mf_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, g80_m2mf_addresses))
	{ }
	else if (mthd == 0x0200) // LINEAR_IN
		g80_m2mf.linear_in = data;
	else if (mthd == 0x021c) // LINEAR_OUT
		g80_m2mf.linear_out = data;
	else if (mthd == 0x031c) // LINE_LENGTH_IN
		g80_m2mf.line_length_in = data;
	else if (mthd == 0x0320) // LINE_COUNT
	{
		g80_m2mf.line_count = data;

		struct addr_n_buf *off_in = &g80_m2mf.offset_in;
		struct addr_n_buf *off_out = &g80_m2mf.offset_out;

		struct gpu_mapping *msrc = off_in->gpu_mapping;
		struct gpu_mapping *mdst = off_out->gpu_mapping;
		if (msrc && mdst && g80_m2mf.linear_in == 1 && g80_m2mf.linear_out == 1)
		{
			uint32_t len = g80_m2mf.line_count * g80_m2mf.line_length_in;
			gpu_mapping_register_copy(mdst, off_out->address, msrc, off_in->address, len);
		}
	}
}
