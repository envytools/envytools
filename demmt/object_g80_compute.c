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
#include "log.h"
#include "nvrm.h"
#include "object.h"

struct g80_compute_data
{
	struct subchan subchan;
	struct addr_n_buf cp;
	struct addr_n_buf stack;
	struct addr_n_buf tsc;
	struct addr_n_buf local;
	struct addr_n_buf cb_def;
	struct addr_n_buf tic;
	struct addr_n_buf query;
	struct addr_n_buf cond;
	struct addr_n_buf g[16];
	uint32_t linked_tsc;

	struct rnndeccontext *texture_ctx;

	struct mthd2addr *addresses;
};

static void destroy_g80_compute_data(struct gpu_object *obj)
{
	struct g80_compute_data *d = obj->class_data;
	rnndec_freecontext(d->texture_ctx);
	free(d->addresses);
	free(d);
}

void decode_g80_compute_init(struct gpu_object *obj)
{
	struct g80_compute_data *d = obj->class_data = calloc(1, sizeof(struct g80_compute_data));
	d->texture_ctx = create_g80_texture_ctx(obj);
	obj->class_data_destroy = destroy_g80_compute_data;

#define SZ 11
	struct mthd2addr *tmp = d->addresses = calloc(SZ, sizeof(*d->addresses));
	m2a_set1(tmp++, 0x0010, 0x0014, &d->subchan.semaphore);
	m2a_set1(tmp++, 0x0210, 0x0214, &d->cp);
	m2a_set1(tmp++, 0x0218, 0x021c, &d->stack);
	m2a_set1(tmp++, 0x022c, 0x0230, &d->tsc);
	m2a_set1(tmp++, 0x0294, 0x0298, &d->local);
	m2a_set1(tmp++, 0x02a4, 0x02a8, &d->cb_def);
	m2a_set1(tmp++, 0x02c4, 0x02c8, &d->tic);
	m2a_set1(tmp++, 0x0310, 0x0314, &d->query);
	m2a_set1(tmp++, 0x0320, 0x0324, &d->cond);
	m2a_setN(tmp++, 0x0400, 0x0404, &d->g[0], 16, 32);
	m2a_set1(tmp++, 0, 0, NULL);
	assert(tmp - d->addresses == SZ);
#undef SZ
}

void decode_g80_compute_terse(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	struct g80_compute_data *objdata = obj->class_data;

	if (check_addresses_terse(pstate, objdata->addresses))
	{ }
}

void decode_g80_compute_verbose(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;
	struct g80_compute_data *objdata = obj->class_data;

	if (check_addresses_verbose(pstate, objdata->addresses))
	{ }
	else if (mthd == 0x0378)
		objdata->linked_tsc = data;
	else if (mthd == 0x03b4 && dump_cp) // CP_START_ID
		g80_3d_disassemble(pstate, &objdata->cp, "cp", data);
	else if (mthd == 0x3c0 || mthd == 0x3c4)
	{
		uint32_t is_tsc = mthd == 0x3c0;

		if (dump_tsc && objdata->tsc.gpu_mapping && (
			    is_tsc || objdata->linked_tsc))
		{
			int tsc;
			if (is_tsc)
				tsc = (data >> 12) & 0xff;
			else
				tsc = (data >> 9) & 0x3fffff;
			mmt_debug("bind tsc: 0x%08x\n", tsc);
			uint32_t *tsc_data = gpu_mapping_get_data(objdata->tsc.gpu_mapping, objdata->tsc.address + 32 * tsc, 8 * 4);

			decode_tsc(objdata->texture_ctx, tsc, tsc_data);
		}
		if (dump_tic && objdata->tic.gpu_mapping && !is_tsc)
		{
			int tic = (data >> 9) & 0x3fffff;
			mmt_debug("bind tic: 0x%08x\n", tic);
			uint32_t *tic_data = gpu_mapping_get_data(objdata->tic.gpu_mapping, objdata->tic.address + 32 * tic, 8 * 4);

			decode_tic(objdata->texture_ctx, tic, tic_data);
		}
	}
}
