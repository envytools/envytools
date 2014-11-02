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

struct gf100_m2mf_data
{
	struct addr_n_buf offset_in;
	struct addr_n_buf offset_out;
	struct addr_n_buf query;
	int data_offset;

	struct mthd2addr *addresses;
};

void decode_gf100_m2mf_init(struct gpu_object *obj)
{
	struct gf100_m2mf_data *d = obj->class_data = calloc(1, sizeof(struct gf100_m2mf_data));

#define SZ 4
	struct mthd2addr *tmp = d->addresses = calloc(SZ, sizeof(*d->addresses));
	m2a_set1(tmp++, 0x0238, 0x023c, &d->offset_out);
	m2a_set1(tmp++, 0x030c, 0x0310, &d->offset_in);
	m2a_set1(tmp++, 0x032c, 0x0330, &d->query);
	m2a_set1(tmp++, 0, 0, NULL);
	assert(tmp - d->addresses == SZ);
#undef SZ

}

void decode_gf100_m2mf_terse(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	struct gf100_m2mf_data *objdata = obj->class_data;

	if (check_addresses_terse(pstate, objdata->addresses))
	{
		if (pstate->mthd == 0x023c)
		{
			if (objdata->offset_out.gpu_mapping)
				objdata->data_offset = 0;
		}
	}
}

void decode_gf100_m2mf_verbose(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;
	struct gf100_m2mf_data *objdata = obj->class_data;

	if (check_addresses_verbose(pstate, objdata->addresses))
	{ }
	else if (mthd == 0x0300) // EXEC
	{
		int flags_ok = (data & 0x111) == 0x111 ? 1 : 0;
		mmt_debug("m2mf exec: 0x%08x push&linear: %d\n", data, flags_ok);

		if (!flags_ok || objdata->offset_out.gpu_mapping == NULL)
		{
			objdata->offset_out.address = 0;
			objdata->offset_out.gpu_mapping = NULL;
		}
	}
	else if (mthd == 0x0304) // DATA
	{
		mmt_debug("m2mf data: 0x%08x\n", data);
		if (objdata->offset_out.gpu_mapping)
		{
			gpu_mapping_register_write(objdata->offset_out.gpu_mapping,
					objdata->offset_out.address + objdata->data_offset, 4, &data);
			objdata->data_offset += 4;
		}
	}
}
