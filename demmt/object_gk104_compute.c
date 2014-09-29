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

#include "config.h"
#include "object.h"

static struct
{
	struct addr_n_buf temp;
	struct addr_n_buf query;
	struct addr_n_buf tsc;
	struct addr_n_buf tic;
	struct addr_n_buf code;
	struct addr_n_buf upload_dst;
	int data_offset;
	struct addr_n_buf launch_desc;
} gk104_compute;

static struct mthd2addr gk104_compute_addresses[] =
{
	{ 0x0790, 0x0794, &gk104_compute.temp },
	{ 0x1b00, 0x1b04, &gk104_compute.query },
	{ 0x155c, 0x1560, &gk104_compute.tsc },
	{ 0x1574, 0x1578, &gk104_compute.tic },
	{ 0x1608, 0x160c, &gk104_compute.code },
	{ 0x0188, 0x018c, &gk104_compute.upload_dst },
	{ 0, 0, NULL }
};

void decode_gk104_compute_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, gk104_compute_addresses))
	{
		if (pstate->mthd == 0x018c) // UPLOAD.DST_ADDRESS_LOW
			if (gk104_compute.upload_dst.buffer)
				gk104_compute.data_offset = gk104_compute.upload_dst.address - gk104_compute.upload_dst.buffer->gpu_start;
	}
}

void decode_gk104_compute_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, gk104_compute_addresses))
	{ }
	else if (mthd == 0x01b0) // UPLOAD.EXEC
	{
		int flags_ok = (data & 0x1) == 0x1 ? 1 : 0;
		mmt_debug("exec: 0x%08x linear: %d\n", data, flags_ok);

		if (!flags_ok || gk104_compute.upload_dst.buffer == NULL)
		{
			gk104_compute.upload_dst.address = 0;
			gk104_compute.upload_dst.buffer = NULL;
		}
	}
	else if (mthd == 0x01b4) // UPLOAD.DATA
	{
		mmt_debug("data: 0x%08x\n", data);
		if (gk104_compute.upload_dst.buffer)
		{
			buffer_register_write(gk104_compute.upload_dst.buffer, gk104_compute.data_offset, 4, &data);
			gk104_compute.data_offset += 4;
		}
	}
	else if (mthd == 0x02b4) // LAUNCH_DESC_ADDRESS
	{
		gk104_compute.launch_desc.address = ((uint64_t)data) << 8;
		gk104_compute.launch_desc.buffer = find_buffer_by_gpu_address(gk104_compute.launch_desc.address);
	}
	else if (mthd == 0x02bc) // LAUNCH
	{
		struct buffer *buf = gk104_compute.code.buffer;

		if (buf && dump_cp)
		{
			const struct disisa *isa;
			if (chipset >= 0xf0)
			{
				if (!isa_gk110)
					isa_gk110 = ed_getisa("gk110");
				isa = isa_gk110;
			}
			else
			{
				if (!isa_gf100)
					isa_gf100 = ed_getisa("gf100");
				isa = isa_gf100;
			}

			struct varinfo *var = varinfo_new(isa->vardata);

			if (chipset < 0xf0)
				varinfo_set_variant(var, "gk104");

			struct region *reg;
			for (reg = buf->written_regions.head; reg != NULL; reg = reg->next)
			{
				if (reg->start != ((uint32_t *)gk104_compute.launch_desc.buffer->data)[0x8] +
						gk104_compute.code.address - buf->gpu_start)
					continue;

				envydis(isa, stdout, buf->data + reg->start, 0,
						reg->end - reg->start, var, 0, NULL, 0, colors);
				break;
			}

			varinfo_del(var);
		}
	}
}
