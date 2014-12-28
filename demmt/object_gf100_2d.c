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

struct gf100_2d_data
{
	struct nv1_graph graph;
	struct addr_n_buf src;
	struct addr_n_buf dst;

	struct mthd2addr *addresses;
};

static void destroy_gf100_2d_data(struct gpu_object *obj)
{
	struct gf100_2d_data *d = obj->class_data;
	free(d->addresses);
	free(d);
}

void decode_gf100_2d_init(struct gpu_object *obj)
{
	struct gf100_2d_data *d = obj->class_data = calloc(1, sizeof(struct gf100_2d_data));
	obj->class_data_destroy = destroy_gf100_2d_data;

#define SZ 4
	struct mthd2addr *tmp = d->addresses = calloc(SZ, sizeof(*d->addresses));
	m2a_set1(tmp++, 0x0104, 0x0108, &d->graph.notify);
	m2a_set1(tmp++, 0x0250, 0x0254, &d->src);
	m2a_set1(tmp++, 0x0220, 0x0224, &d->dst);
	m2a_set1(tmp++, 0, 0, NULL);
	assert(tmp - d->addresses == SZ);
#undef SZ
}

void decode_gf100_2d_terse(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	struct gf100_2d_data *objdata = obj->class_data;

	if (check_addresses_terse(pstate, objdata->addresses))
	{ }
}
