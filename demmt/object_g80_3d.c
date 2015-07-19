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

struct gf80_3d_data
{
	struct subchan subchan;
	struct addr_n_buf vp;
	struct addr_n_buf fp;
	struct addr_n_buf gp;
	struct addr_n_buf tsc;
	struct addr_n_buf tic;
	struct addr_n_buf zeta;
	struct addr_n_buf query;
	struct addr_n_buf cb_def;
	struct addr_n_buf vertex_runout;
	struct addr_n_buf rt[8];
	struct addr_n_buf vertex_array_start[16];
	struct addr_n_buf vertex_array_limit[16];
	struct addr_n_buf local;
	struct addr_n_buf stack;
	uint32_t linked_tsc;

	struct rnndeccontext *texture_ctx;

	struct mthd2addr *addresses;
};

static void __g80_3d_disassemble(uint8_t *data, struct region *reg,
		const char *mode, int32_t offset, uint32_t start_id,
		struct varinfo *var)
{
	mmt_debug("%s_start id 0x%08x\n", mode, start_id);
	for ( ; reg != NULL; reg = reg->next)
	{
		if (reg->start != start_id + offset)
			continue;

		if (MMT_DEBUG)
		{
			uint32_t x;
			mmt_debug("CODE: %s", "");
			for (x = reg->start; x < reg->end; x += 4)
				mmt_debug_cont("0x%08x ", *(uint32_t *)(data + x));
			mmt_debug_cont("%s\n", "");
		}

		envydis(isa_g80, stdout, data + reg->start, start_id,
				reg->end - reg->start, var, 0, NULL, 0, colors);
		break;
	}
}

static void g80_3d_disassemble(struct pushbuf_decode_state *pstate,
		struct addr_n_buf *anb, const char *mode, uint32_t start_id)
{
	uint8_t *data = NULL;
	struct region *reg;
	int32_t offset = 0;

	struct gpu_mapping *m = anb->gpu_mapping;
	if (m)
	{
		data = gpu_mapping_get_data(m, m->address, 0);
		reg = m->object->written_regions.head;
		offset = anb->address - m->address;
	}
	if (data)
	{
		if (!isa_g80)
			isa_g80 = ed_getisa("g80");

		struct varinfo *var = varinfo_new(isa_g80->vardata);
		int chipset = nvrm_get_chipset(pstate->fifo);

		if (chipset == 0x50)
			varinfo_set_variant(var, "g80");
		else if (chipset >= 0x84 && chipset <= 0x98)
			varinfo_set_variant(var, "g84");
		else if (chipset == 0xa0)
			varinfo_set_variant(var, "g200");
		else if (chipset >= 0xaa && chipset <= 0xac)
			varinfo_set_variant(var, "mcp77");
		else if ((chipset >= 0xa3 && chipset <= 0xa8) || chipset == 0xaf)
			varinfo_set_variant(var, "gt215");

		varinfo_set_mode(var, mode);

		__g80_3d_disassemble(data, reg, mode, offset, start_id, var);

		varinfo_del(var);
	}
}

static void destroy_g80_3d_data(struct gpu_object *obj)
{
	struct gf80_3d_data *d = obj->class_data;
	rnndec_freecontext(d->texture_ctx);
	free(d->addresses);
	free(d);
}

void decode_g80_3d_init(struct gpu_object *obj)
{
	struct gf80_3d_data *d = obj->class_data = calloc(1, sizeof(struct gf80_3d_data));
	d->texture_ctx = create_g80_texture_ctx(obj);
	obj->class_data_destroy = destroy_g80_3d_data;

#define SZ 16
	struct mthd2addr *tmp = d->addresses = calloc(SZ, sizeof(*d->addresses));
	m2a_set1(tmp++, 0x0010, 0x0014, &d->subchan.semaphore);
	m2a_setN(tmp++, 0x0200, 0x0204, &d->rt[0], 8, 32);
	m2a_setN(tmp++, 0x0904, 0x0908, &d->vertex_array_start[0], 16, 16);
	m2a_setN(tmp++, 0x1080, 0x1084, &d->vertex_array_limit[0], 16, 8);
	m2a_set1(tmp++, 0x0f70, 0x0f74, &d->gp);
	m2a_set1(tmp++, 0x0f7c, 0x0f80, &d->vp);
	m2a_set1(tmp++, 0x0f84, 0x0f88, &d->vertex_runout);
	m2a_set1(tmp++, 0x0fa4, 0x0fa8, &d->fp);
	m2a_set1(tmp++, 0x0fe0, 0x0fe4, &d->zeta);
	m2a_set1(tmp++, 0x155c, 0x1560, &d->tsc);
	m2a_set1(tmp++, 0x1574, 0x1578, &d->tic);
	m2a_set1(tmp++, 0x1280, 0x1284, &d->cb_def);
	m2a_set1(tmp++, 0x1b00, 0x1b04, &d->query);
	m2a_set1(tmp++, 0x12d8, 0x12dc, &d->local);
	m2a_set1(tmp++, 0x0d94, 0x0d98, &d->stack);
	m2a_set1(tmp++, 0, 0, NULL);
	assert(tmp - d->addresses == SZ);
#undef SZ
}

void decode_g80_3d_terse(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	struct gf80_3d_data *objdata = obj->class_data;

	if (check_addresses_terse(pstate, objdata->addresses))
	{ }
}

void decode_g80_3d_verbose(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;
	struct gf80_3d_data *objdata = obj->class_data;

	if (check_addresses_verbose(pstate, objdata->addresses))
	{ }
	else if (mthd == 0x1234)
		objdata->linked_tsc = data;
	else if (mthd == 0x140c && dump_vp) // VP_START_ID
		g80_3d_disassemble(pstate, &objdata->vp, "vp", data);
	else if (mthd == 0x1414 && dump_fp) // FP_START_ID
		g80_3d_disassemble(pstate, &objdata->fp, "fp", data);
	else if (mthd == 0x1410 && dump_gp) // GP_START_ID
		g80_3d_disassemble(pstate, &objdata->gp, "gp", data);
	else if (mthd >= 0x1444 && mthd < 0x1444 + 0x8 * 3)
	{
		int i;
		uint32_t is_tsc = 0;

		for (i = 0; i < 3; ++i)
		{
			if (mthd == 0x1444 + i * 0x8) // BIND_TSC[i]
			{
				is_tsc = 1;
				break;
			}
			else if (mthd == 0x1448 + i * 0x8) // BIND_TIC[i]
			{
				break;
			}
		}
		if (i < 3)
		{
			if (dump_tsc && objdata->tsc.gpu_mapping && (
					    is_tsc || objdata->linked_tsc))
			{
				int j, tsc;
				if (is_tsc)
					tsc = (data >> 12) & 0xff;
				else
					tsc = (data >> 9) & 0x3fffff;
				mmt_debug("bind tsc[%d]: 0x%08x\n", i, tsc);
				uint32_t *tsc_data = gpu_mapping_get_data(objdata->tsc.gpu_mapping, objdata->tsc.address + 32 * tsc, 8 * 4);

				for (j = 0; j < 8; ++j)
					decode_tsc(objdata->texture_ctx, tsc, j, tsc_data);

			}
			if (dump_tic && objdata->tic.gpu_mapping && !is_tsc)
			{
				int j, tic = (data >> 9) & 0x3fffff;
				mmt_debug("bind tic[%d]: 0x%08x\n", i, tic);
				uint32_t *tic_data = gpu_mapping_get_data(objdata->tic.gpu_mapping, objdata->tic.address + 32 * tic, 8 * 4);

				for (j = 0; j < 8; ++j)
					decode_tic(objdata->texture_ctx, tic, j, tic_data);

			}
		}
	}
}
