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
	struct addr_n_buf offset_in;
	struct addr_n_buf offset_out;
	struct addr_n_buf query;
	int data_offset;
} gf100_m2mf;

static struct mthd2addr gf100_m2mf_addresses[] =
{
	{ 0x0238, 0x023c, &gf100_m2mf.offset_out },
	{ 0x030c, 0x0310, &gf100_m2mf.offset_in },
	{ 0x032c, 0x0330, &gf100_m2mf.query },
	{ 0, 0, NULL }
};

void decode_gf100_m2mf_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, gf100_m2mf_addresses))
	{
		if (pstate->mthd == 0x023c)
			if (gf100_m2mf.offset_out.buffer)
				gf100_m2mf.data_offset = gf100_m2mf.offset_out.address - gf100_m2mf.offset_out.buffer->gpu_start;
	}
}

void decode_gf100_m2mf_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, gf100_m2mf_addresses))
	{ }
	else if (mthd == 0x0300) // EXEC
	{
		int flags_ok = (data & 0x111) == 0x111 ? 1 : 0;
		mmt_debug("m2mf exec: 0x%08x push&linear: %d\n", data, flags_ok);

		if (!flags_ok || gf100_m2mf.offset_out.buffer == NULL)
		{
			gf100_m2mf.offset_out.address = 0;
			gf100_m2mf.offset_out.buffer = NULL;
		}
	}
	else if (mthd == 0x0304) // DATA
	{
		mmt_debug("m2mf data: 0x%08x\n", data);
		if (gf100_m2mf.offset_out.buffer)
		{
			buffer_register_write(gf100_m2mf.offset_out.buffer, gf100_m2mf.data_offset, 4, &data);
			gf100_m2mf.data_offset += 4;
		}
	}
}
