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
	struct addr_n_buf upload_dst;
	int data_offset;
} gk104_p2mf;

static struct mthd2addr gk104_p2mf_addresses[] =
{
	{ 0x0188, 0x018c, &gk104_p2mf.upload_dst },
	{ 0, 0, NULL }
};

void decode_gk104_p2mf_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, gk104_p2mf_addresses))
	{
		if (pstate->mthd == 0x018c) // UPLOAD.DST_ADDRESS_LOW
			if (gk104_p2mf.upload_dst.buffer)
				gk104_p2mf.data_offset = gk104_p2mf.upload_dst.address - gk104_p2mf.upload_dst.buffer->gpu_start;
	}
}

void decode_gk104_p2mf_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, gk104_p2mf_addresses))
	{ }
	else if (mthd == 0x01b0) // UPLOAD.EXEC
	{
		int flags_ok = (data & 0x1) == 0x1 ? 1 : 0;
		mmt_debug("p2mf exec: 0x%08x linear: %d\n", data, flags_ok);

		if (!flags_ok || gk104_p2mf.upload_dst.buffer == NULL)
		{
			gk104_p2mf.upload_dst.address = 0;
			gk104_p2mf.upload_dst.buffer = NULL;
		}
	}
	else if (mthd == 0x01b4) // UPLOAD.DATA
	{
		mmt_debug("p2mf data: 0x%08x\n", data);
		if (gk104_p2mf.upload_dst.buffer)
		{
			buffer_register_write(gk104_p2mf.upload_dst.buffer, gk104_p2mf.data_offset, 4, &data);
			gk104_p2mf.data_offset += 4;
		}
	}
}
