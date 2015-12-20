/*
 * Copyright (C) 2015 Samuel Pitoiset <samuel.pitoiset@gmail.com>
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
#include "demmt.h"
#include "log.h"
#include "nvrm.h"
#include "object.h"

struct gf100_compute_data
{
	struct addr_n_buf code;
	struct addr_n_buf tsc;
	uint32_t linked_tsc;
	struct addr_n_buf tic;
	struct addr_n_buf cb;
	int cb_pos;
	struct addr_n_buf image[8];

	struct rnndeccontext *texture_ctx;

	struct mthd2addr *addresses;
};

static void destroy_gf100_compute_data(struct gpu_object *obj)
{
	struct gf100_compute_data *d = obj->class_data;
	rnndec_freecontext(d->texture_ctx);
	free(d->addresses);
	free(d);
}

void decode_gf100_compute_init(struct gpu_object *obj)
{
	struct gf100_compute_data *d = obj->class_data = calloc(1, sizeof(struct gf100_compute_data));
	d->texture_ctx = create_g80_texture_ctx(obj);
	obj->class_data_destroy = destroy_gf100_compute_data;

#define SZ 6
	struct mthd2addr *tmp = d->addresses = calloc(SZ, sizeof(*d->addresses));
	m2a_set1(tmp++, 0x155c, 0x1560, &d->tsc);
	m2a_set1(tmp++, 0x1574, 0x1578, &d->tic);
	m2a_set1(tmp++, 0x1608, 0x160c, &d->code);
	m2a_set1(tmp++, 0x2384, 0x2388, &d->cb);
	m2a_setN(tmp++, 0x2700, 0x2704, &d->image[0], 8, 0x20);
	m2a_set1(tmp++, 0, 0, NULL);
	assert(tmp - d->addresses == SZ);
#undef SZ
}

void decode_gf100_compute_terse(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	struct gf100_compute_data *objdata = obj->class_data;

	if (check_addresses_terse(pstate, objdata->addresses))
	{ }
}

void decode_gf100_compute_verbose(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;
	struct gf100_compute_data *objdata = obj->class_data;

	if (check_addresses_verbose(pstate, objdata->addresses))
	{ }
	else if (mthd == 0x1234)
	{
		objdata->linked_tsc = data;
	}
	else if (mthd == 0x238c) // CB_POS
	{
		mmt_debug("cb_pos: 0x%08x\n", data);
		if (objdata->cb.gpu_mapping)
		{
			objdata->cb_pos = data;
		}
	}
	else if (mthd == 0x2390) // CB_DATA[0]
	{
		mmt_debug("cb_data: 0x%08x\n", data);
		if (objdata->cb.gpu_mapping)
		{
			gpu_mapping_register_write(objdata->cb.gpu_mapping,
						   objdata->cb.address + objdata->cb_pos,
						   4, &data);
			objdata->cb_pos += 4;
		}
	}
	else if (mthd == 0x03b4) // CP_START_ID
	{
		struct varinfo *var;

		if (!isa_gf100)
			isa_gf100 = ed_getisa("gf100");

		var = varinfo_new(isa_gf100->vardata);
		varinfo_set_variant(var, "gf100");

		uint8_t *code = NULL;
		uint64_t code_addr = objdata->code.address;
		uint32_t start_id = data;
		struct gpu_mapping *m = objdata->code.gpu_mapping;
		if (!m) {
			// blob has been observed to feed an unmapped address
			// as the base of the code page, and then use larger
			// start id's.
			m = gpu_mapping_find(code_addr + start_id, nvrm_get_device(pstate->fifo));
		}
		if (m)
		{
			code_addr = m->address;
			start_id -= m->address - objdata->code.address;
			code = gpu_mapping_get_data(m, code_addr, 0);

			if (code_addr != m->address)
				code = NULL; // FIXME
		}

		mmt_printf("CODE:%s\n", "");
		if (code)
		{
			struct region *reg;
			for (reg = m->object->written_regions.head; reg != NULL; reg = reg->next)
			{
				if (reg->start != start_id + code_addr - m->address)
					continue;

				envydis(isa_gf100, stdout, code + reg->start, 0,
						reg->end - reg->start, var, 0, NULL, 0, colors);
				break;
			}
		}

		if (var)
			varinfo_del(var);
	}
	else if (mthd >= 0x0228 && mthd <= 0x0234) // BIND_TIC / BIND_TSC
	{
		int is_tsc = (mthd == 0x0228 || mthd == 0x0230) ? 1 : 0;
		if (dump_tsc && objdata->tsc.gpu_mapping && (is_tsc || objdata->linked_tsc))
		{
			int tsc;
			if (is_tsc)
				tsc = (data >> 12) & 0xfff;
			else
				tsc = (data >> 9) & 0x1ffff;
			mmt_debug("bind tsc%d: 0x%08x\n", (mthd == 0x0228) ? 1 : 2, tsc);
			uint32_t *tsc_data = gpu_mapping_get_data(objdata->tsc.gpu_mapping, objdata->tsc.address + 32 * tsc, 8 * 4);

                        decode_tsc(objdata->texture_ctx, tsc, tsc_data);
		}
		if (dump_tic && objdata->tic.gpu_mapping && !is_tsc)
		{
			int tic = (data >> 9) & 0x1ffff;
			mmt_debug("bind tic%d: 0x%08x\n", (mthd == 0x022c) ? 1 : 2, tic);
			uint32_t *tic_data = gpu_mapping_get_data(objdata->tic.gpu_mapping, objdata->tic.address + 32 * tic, 8 * 4);

			decode_tic(objdata->texture_ctx, tic, tic_data);

		}
	}
}
