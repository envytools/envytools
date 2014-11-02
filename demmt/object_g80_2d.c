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

struct g80_2d_data
{
	struct addr_n_buf dst;
	struct addr_n_buf src;

	uint32_t dst_linear;

	int check_dst_mapping;
	int data_offset;

	struct mthd2addr *addresses;
};

void decode_g80_2d_init(struct gpu_object *obj)
{
	struct g80_2d_data *d = obj->class_data = calloc(1, sizeof(struct g80_2d_data));

#define SZ 3
	struct mthd2addr *tmp = d->addresses = calloc(SZ, sizeof(*d->addresses));
	m2a_set1(tmp++, 0x0220, 0x0224, &d->dst);
	m2a_set1(tmp++, 0x0250, 0x0254, &d->src);
	m2a_set1(tmp++, 0, 0, NULL);
	assert(tmp - d->addresses == SZ);
#undef SZ
}

void decode_g80_2d_terse(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	struct g80_2d_data *objdata = obj->class_data;

	if (check_addresses_terse(pstate, objdata->addresses))
	{
		if (pstate->mthd == 0x0224) // DST_ADDRESS_LOW
			objdata->check_dst_mapping = objdata->dst.gpu_mapping != NULL;
	}
}

void decode_g80_2d_verbose(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;
	struct g80_2d_data *objdata = obj->class_data;

	if (check_addresses_verbose(pstate, objdata->addresses))
	{ }
	else if (mthd == 0x0204) // DST_LINEAR
		objdata->dst_linear = data;
	else if (mthd == 0x0838) // SIFC_WIDTH
	{
		if (objdata->dst.gpu_mapping)
			objdata->data_offset = 0;
	}
	else if (mthd == 0x0860) // SIFC_DATA
	{
		if (objdata->check_dst_mapping)
		{
			objdata->check_dst_mapping = 0;

			if (!objdata->dst_linear)
				objdata->dst.gpu_mapping = NULL;
		}

		if (objdata->dst.gpu_mapping != NULL)
		{
			mmt_debug("2d sifc_data: 0x%08x\n", data);
			gpu_mapping_register_write(objdata->dst.gpu_mapping, objdata->dst.address + objdata->data_offset, 4, &data);
			objdata->data_offset += 4;
		}
	}
}
