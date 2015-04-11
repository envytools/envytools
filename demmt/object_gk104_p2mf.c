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

struct gk104_p2mf_data
{
	struct gk104_upload upload;
	int data_offset;

	struct mthd2addr *addresses;
};

static void destroy_gk104_p2mf_data(struct gpu_object *obj)
{
	struct gk104_p2mf_data *d = obj->class_data;
	free(d->addresses);
	free(d);
}

void decode_gk104_p2mf_init(struct gpu_object *obj)
{
	struct gk104_p2mf_data *d = obj->class_data = calloc(1, sizeof(struct gk104_p2mf_data));
	obj->class_data_destroy = destroy_gk104_p2mf_data;

#define SZ 3
	struct mthd2addr *tmp = d->addresses = calloc(SZ, sizeof(*d->addresses));
	m2a_set1(tmp++, 0x0188, 0x018c, &d->upload.dst);
	m2a_set1(tmp++, 0x01dc, 0x01e0, &d->upload.query);
	m2a_set1(tmp++, 0, 0, NULL);
	assert(tmp - d->addresses == SZ);
#undef SZ
}

void decode_gk104_p2mf_terse(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	struct gk104_p2mf_data *objdata = obj->class_data;

	if (check_addresses_terse(pstate, objdata->addresses))
	{
		if (pstate->mthd == 0x018c) // UPLOAD.DST_ADDRESS_LOW
		{
			if (objdata->upload.dst.gpu_mapping)
				objdata->data_offset = 0;
		}
	}
}

void decode_gk104_p2mf_verbose(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;
	struct gk104_p2mf_data *objdata = obj->class_data;

	if (check_addresses_verbose(pstate, objdata->addresses))
	{ }
	else if (mthd == 0x01b0) // UPLOAD.EXEC
	{
		int flags_ok = (data & 0x1) == 0x1 ? 1 : 0;
		mmt_debug("p2mf exec: 0x%08x linear: %d\n", data, flags_ok);

		if (!flags_ok || objdata->upload.dst.gpu_mapping == NULL)
		{
			objdata->upload.dst.address = 0;
			objdata->upload.dst.gpu_mapping = NULL;
		}
	}
	else if (mthd == 0x01b4) // UPLOAD.DATA
	{
		mmt_debug("p2mf data: 0x%08x\n", data);
		if (objdata->upload.dst.gpu_mapping)
		{
			gpu_mapping_register_write(objdata->upload.dst.gpu_mapping,
					objdata->upload.dst.address + objdata->data_offset, 4, &data);
			objdata->data_offset += 4;
		}
	}
}
