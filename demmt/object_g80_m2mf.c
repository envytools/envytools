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

struct g80_m2mf_data
{
	struct subchan subchan;
	struct addr_n_buf offset_in;
	struct addr_n_buf offset_out;

	uint32_t linear_in;
	uint32_t linear_out;
	uint32_t line_length_in;
	uint32_t line_count;

	struct mthd2addr *addresses;
};

static void destroy_g80_m2mf_data(struct gpu_object *obj)
{
	struct g80_m2mf_data *d = obj->class_data;
	free(d->addresses);
	free(d);
}

void decode_g80_m2mf_init(struct gpu_object *obj)
{
	struct g80_m2mf_data *d = obj->class_data = calloc(1, sizeof(struct g80_m2mf_data));
	obj->class_data_destroy = destroy_g80_m2mf_data;

#define SZ 4
	struct mthd2addr *tmp = d->addresses = calloc(SZ, sizeof(*d->addresses));
	m2a_set1(tmp++, 0x0010, 0x0014, &d->subchan.semaphore);
	m2a_set1(tmp++, 0x0238, 0x030c, &d->offset_in);
	m2a_set1(tmp++, 0x023c, 0x0310, &d->offset_out);
	m2a_set1(tmp++, 0, 0, NULL);
	assert(tmp - d->addresses == SZ);
#undef SZ
}

void decode_g80_m2mf_terse(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	struct g80_m2mf_data *objdata = obj->class_data;

	if (check_addresses_terse(pstate, objdata->addresses))
	{ }
}

void decode_g80_m2mf_verbose(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;
	struct g80_m2mf_data *objdata = obj->class_data;

	if (check_addresses_verbose(pstate, objdata->addresses))
	{ }
	else if (mthd == 0x0200) // LINEAR_IN
		objdata->linear_in = data;
	else if (mthd == 0x021c) // LINEAR_OUT
		objdata->linear_out = data;
	else if (mthd == 0x031c) // LINE_LENGTH_IN
		objdata->line_length_in = data;
	else if (mthd == 0x0320) // LINE_COUNT
	{
		objdata->line_count = data;

		struct addr_n_buf *off_in = &objdata->offset_in;
		struct addr_n_buf *off_out = &objdata->offset_out;

		struct gpu_mapping *msrc = off_in->gpu_mapping;
		struct gpu_mapping *mdst = off_out->gpu_mapping;
		if (msrc && mdst && objdata->linear_in == 1 && objdata->linear_out == 1)
		{
			uint32_t len = objdata->line_count * objdata->line_length_in;
			gpu_mapping_register_copy(mdst, off_out->address, msrc, off_in->address, len);
		}
	}
}
