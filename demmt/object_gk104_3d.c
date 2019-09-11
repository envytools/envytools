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
#include "demmt.h"
#include "log.h"
#include "macro.h"
#include "nvrm.h"
#include "object.h"

struct gk104_3d_data
{
	struct macro_state macro;

	struct addr_n_buf code;
	struct addr_n_buf tsc;
	struct addr_n_buf tic;
	struct addr_n_buf query;
	struct addr_n_buf vertex_quarantine;
	struct addr_n_buf vertex_runout;
	struct addr_n_buf temp;
	struct addr_n_buf zeta;
	struct addr_n_buf zcull;
	struct addr_n_buf zcull_limit;
	struct addr_n_buf rt[8];
	struct addr_n_buf index_array_start;
	struct addr_n_buf index_array_limit;
	struct addr_n_buf cb;
	struct addr_n_buf vertex_array_start[32];
	struct addr_n_buf vertex_array_limit[32];
	struct addr_n_buf image[8];

	struct nv1_graph graph;
	struct subchan subchan;
	struct gk104_upload upload;

	uint32_t tic2;
	uint32_t linked_tsc;
	uint32_t tex_cb_index;
	uint32_t cb_pos;

	struct addr_n_buf texcb[5];

	struct rnndeccontext *texture_ctx;

	struct mthd2addr *addresses;
};

static void destroy_gk104_3d_data(struct gpu_object *obj)
{
	struct gk104_3d_data *d = obj->class_data;
	rnndec_freecontext(d->texture_ctx);
	free(d->macro.code);
	free(d->addresses);
	free(d);
}

void decode_gk104_3d_init(struct gpu_object *obj)
{
	struct gk104_3d_data *d = obj->class_data = calloc(1, sizeof(struct gk104_3d_data));
	d->texture_ctx = create_g80_texture_ctx(obj);
	obj->class_data_destroy = destroy_gk104_3d_data;

#define SZ 22
	struct mthd2addr *tmp = d->addresses = calloc(SZ, sizeof(*d->addresses));
	m2a_set1(tmp++, 0x0010, 0x0014, &d->subchan.semaphore);
	m2a_set1(tmp++, 0x0104, 0x0108, &d->graph.notify);
	m2a_set1(tmp++, 0x0790, 0x0794, &d->temp);
	m2a_set1(tmp++, 0x07e8, 0x07ec, &d->zcull);
	m2a_set1(tmp++, 0x07f0, 0x07f4, &d->zcull_limit)->check_offset = -1;
	m2a_set1(tmp++, 0x0f84, 0x0f88, &d->vertex_runout);
	m2a_set1(tmp++, 0x0fe0, 0x0fe4, &d->zeta);
	m2a_set1(tmp++, 0x155c, 0x1560, &d->tsc);
	m2a_set1(tmp++, 0x1574, 0x1578, &d->tic);
	m2a_set1(tmp++, 0x1608, 0x160c, &d->code);
	m2a_set1(tmp++, 0x17bc, 0x17c0, &d->vertex_quarantine);
	m2a_set1(tmp++, 0x17c8, 0x17cc, &d->index_array_start);
	m2a_set1(tmp++, 0x17d0, 0x17d4, &d->index_array_limit);
	m2a_set1(tmp++, 0x1b00, 0x1b04, &d->query);
	m2a_set1(tmp++, 0x2384, 0x2388, &d->cb);
	m2a_setN(tmp++, 0x0800, 0x0804, &d->rt[0], 8, 64);
	m2a_setN(tmp++, 0x1c04, 0x1c08, &d->vertex_array_start[0], 32, 16);
	m2a_setN(tmp++, 0x1f00, 0x1f04, &d->vertex_array_limit[0], 32, 8);
	m2a_setN(tmp++, 0x2700, 0x2704, &d->image[0], 8, 0x20);
	m2a_set1(tmp++, 0x0188, 0x018c, &d->upload.dst);
	m2a_set1(tmp++, 0x01dc, 0x01e0, &d->upload.query);
	m2a_set1(tmp++, 0, 0, NULL);
	assert(tmp - d->addresses == SZ);
#undef SZ
}

void decode_gk104_3d_terse(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	struct gk104_3d_data *objdata = obj->class_data;

	if (check_addresses_terse(pstate, objdata->addresses))
	{ }
}

void decode_gk104_3d_verbose(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;
	struct gk104_3d_data *objdata = obj->class_data;

	if (check_addresses_verbose(pstate, objdata->addresses))
	{ }
	else if (mthd == 0x0f10) // SetSelectMaxwellTextureHeaders
		objdata->tic2 = data;
	else if (mthd == 0x1234) // LINKED_TSC
		objdata->linked_tsc = data;
	else if (mthd == 0x2608) // TEX_CB_INDEX
	{
		// This gets done pretty early. To be truly correct,
		// we'd have to go fish out any past CB bindings to
		// this CB index.
		objdata->tex_cb_index = data;
	}
	else if (mthd >= 0x2410 && mthd < 0x2410 + 0x20 * 5) // CB_BIND
	{
		int i;
		int cb_index = data >> 4;
		if (cb_index != objdata->tex_cb_index)
			return;

		for (i = 0; i < 5; i++)
		{
			if (mthd != 0x2410 + 0x20 * i)
				continue;
			objdata->texcb[i] = objdata->cb;
			break;
		}
	}
	else if (mthd == 0x238c) // CB_POS
	{
		objdata->cb_pos = data;
	}
	else if (mthd >= 0x2390 && mthd < 0x2390 + 4 * 16) // CB_DATA
	{
		int i;
/*
		gpu_mapping_register_write(
			objdata->cb.dst.gpu_mapping,
			objdata->cb.dst.address + objdata->cb_pos + (mthd - 0x2390), 4, &data);
*/
		for (i = 0; i < 5; i++) {
			int tic, tsc;
			if (objdata->cb.address != objdata->texcb[i].address)
				continue;
			// The handle has been written. Print out the
			// TIC/TSC contents.
			tic = data & 0xfffff;
			if (objdata->linked_tsc)
				tsc = tic;
			else
				tsc = data >> 20;

			if (dump_tsc && objdata->tsc.gpu_mapping)
			{
				uint32_t *tsc_data = gpu_mapping_get_data(objdata->tsc.gpu_mapping, objdata->tsc.address + 32 * tsc, 8 * 4);

				if (tsc_data)
					decode_tsc(objdata->texture_ctx, tsc, tsc_data);
			}

			if (dump_tic && objdata->tic.gpu_mapping)
			{
				uint32_t *tic_data = gpu_mapping_get_data(objdata->tic.gpu_mapping, objdata->tic.address + 32 * tsc, 8 * 4);
				struct rnndomain *domain = tic_domain;
				int chipset = nvrm_get_chipset(pstate->fifo);

				if (chipset >= 0x120)
					domain = tic2_domain;
				else if (chipset >= 0x110 && objdata->tic2)
					domain = tic2_domain;

				if (tic_data)
					decode_tic(objdata->texture_ctx, domain, tic, tic_data);
			}
		}
	}
	else if (mthd >= 0x2000 && mthd < 0x2000 + 0x40 * 6) // SP
	{
		int i;

		struct varinfo *var = NULL;
		const struct disisa *isa;
		int chipset = nvrm_get_chipset(pstate->fifo);
		if (chipset >= 0x117)
		{
			if (!isa_gm107)
				isa_gm107 = ed_getisa("gm107");
			isa = isa_gm107;

			var = varinfo_new(isa_gm107->vardata);

			if (chipset >= 0x130)
				varinfo_set_variant(var, "sm60");
			else if (chipset >= 0x120)
				varinfo_set_variant(var, "sm52");
			else
				varinfo_set_variant(var, "sm50");
		}
		else if (chipset >= 0xf0 || chipset == 0xea)
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

			var = varinfo_new(isa_gf100->vardata);

			varinfo_set_variant(var, "gk104");
		}

		for (i = 0; i < 6; ++i)
		{
			if (mthd != 0x2004 + i * 0x40) // SP[i].START_ID
				continue;

			mmt_debug("start id[%d]: 0x%08x\n", i, data);
			if (objdata->code.gpu_mapping)
			{
				uint64_t addr = objdata->code.address;
				struct gpu_mapping *m = objdata->code.gpu_mapping;
				uint8_t *code = gpu_mapping_get_data(m, addr, 0);
				if (addr != m->address)
					code = NULL;// FIXME

				if (code)
					gf100_3d_disassemble(code, m->object->written_regions.head,
							data, isa, var);
			}

			break;
		}

		if (var)
			varinfo_del(var);
	}
	else if (decode_macro(pstate, &objdata->macro))
	{ }
}
