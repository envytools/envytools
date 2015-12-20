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

struct gf100_3d_data
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
	uint32_t linked_tsc;

	struct nv1_graph graph;
	struct subchan subchan;

	struct rnndeccontext *texture_ctx;

	struct mthd2addr *addresses;
};

void decode_gf100_p_header(int idx, uint32_t *data, struct rnndomain *header_domain)
{
	struct rnndecaddrinfo *ai = rnndec_decodeaddr(gf100_shaders_ctx, header_domain, idx * 4, 1);
	char *dec_val = rnndec_decodeval(gf100_shaders_ctx, ai->typeinfo, data[idx], ai->width);

	mmt_printf("0x%08x   %s = %s\n", data[idx], ai->name, dec_val);

	rnndec_free_decaddrinfo(ai);
	free(dec_val);
}

static struct rnndomain *gf100_p_header_domain(int program)
{
	if (program >= 0 && program < 5) // VP - GP
		return gf100_sp_header_domain;
	else if (program == 5) // FP
		return gf100_fp_header_domain;
	else
		return NULL;
}

static void gf100_set_kind_variant(int program) {
	if (program == 0)
		rnndec_varmod(gf100_shaders_ctx, "GF100_SHADER_KIND", "VP_A");
	else if (program == 1)
		rnndec_varmod(gf100_shaders_ctx, "GF100_SHADER_KIND", "VP_B");
	else if (program == 2)
		rnndec_varmod(gf100_shaders_ctx, "GF100_SHADER_KIND", "TCP");
	else if (program == 3)
		rnndec_varmod(gf100_shaders_ctx, "GF100_SHADER_KIND", "TEP");
	else if (program == 4)
		rnndec_varmod(gf100_shaders_ctx, "GF100_SHADER_KIND", "GP");
	else if (program == 5)
		rnndec_varmod(gf100_shaders_ctx, "GF100_SHADER_KIND", "FP");

}

static int gf100_p_dump(int program)
{
	if (program == 0 || program == 1) // VP
		return dump_vp;
	else if (program == 2) // TCP
		return dump_tcp;
	else if (program == 3) // TEP
		return dump_tep;
	else if (program == 4) // GP
		return dump_gp;
	else if (program == 5) // FP
		return dump_fp;
	else
		return 1;
}

static void destroy_gf100_3d_data(struct gpu_object *obj)
{
	struct gf100_3d_data *d = obj->class_data;
	rnndec_freecontext(d->texture_ctx);
	free(d->macro.code);
	free(d->addresses);
	free(d);
}

void decode_gf100_3d_init(struct gpu_object *obj)
{
	struct gf100_3d_data *d = obj->class_data = calloc(1, sizeof(struct gf100_3d_data));
	d->texture_ctx = create_g80_texture_ctx(obj);
	obj->class_data_destroy = destroy_gf100_3d_data;

#define SZ 20
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
	m2a_set1(tmp++, 0, 0, NULL);
	assert(tmp - d->addresses == SZ);
#undef SZ
}

void decode_gf100_3d_terse(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	struct gf100_3d_data *objdata = obj->class_data;

	if (check_addresses_terse(pstate, objdata->addresses))
	{ }
}

void gf100_3d_disassemble(uint8_t *data, struct region *reg,
		uint32_t start_id, const struct disisa *isa, struct varinfo *var)
{
	for (; reg != NULL; reg = reg->next)
	{
		if (reg->start != start_id)
			continue;

		uint32_t x;
		x = *(uint32_t *)(data + reg->start);
		int program = (x >> 10) & 0x7;
		if (!gf100_p_dump(program))
			break;

		struct rnndomain *header_domain = gf100_p_header_domain(program);
		gf100_set_kind_variant(program);
		mmt_printf("HEADER:%s\n", "");
		if (header_domain)
			for (x = 0; x < 20; ++x)
				decode_gf100_p_header(x, (uint32_t *)(data + reg->start), header_domain);
		else
			for (x = reg->start; x < reg->start + 20 * 4; x += 4)
				mmt_printf("0x%08x\n", *(uint32_t *)(data + x));

		mmt_printf("CODE:%s\n", "");
		if (MMT_DEBUG)
		{
			uint32_t x;
			mmt_debug("%s", "");
			for (x = reg->start + 20 * 4; x < reg->end; x += 4)
				mmt_debug_cont("0x%08x ", *(uint32_t *)(data + x));
			mmt_debug_cont("%s\n", "");
		}

		envydis(isa, stdout, data + reg->start + 20 * 4, 0,
				reg->end - reg->start - 20 * 4, var, 0, NULL, 0, colors);

		break;
	}
}

void decode_gf100_3d_verbose(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;
	struct gf100_3d_data *objdata = obj->class_data;

	if (check_addresses_verbose(pstate, objdata->addresses))
	{ }
	else if (mthd == 0x1234)
		objdata->linked_tsc = data;
	else if (mthd >= 0x2000 && mthd < 0x2000 + 0x40 * 6) // SP
	{
		int i;
		struct varinfo *var;

		if (!isa_gf100)
			isa_gf100 = ed_getisa("gf100");

		var = varinfo_new(isa_gf100->vardata);
		varinfo_set_variant(var, "gf100");

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
							data, isa_gf100, var);
			}

			break;
		}

		varinfo_del(var);
	}
	else if (mthd >= 0x2400 && mthd < 0x2400 + 0x20 * 5)
	{
		int i, is_tsc = 0;

		for (i = 0; i < 5; ++i)
		{
			if (mthd == 0x2400 + i * 0x20) // BIND_TSC[i]
			{
				is_tsc = 1;
				break;
			}
			else if (mthd == 0x2404 + i * 0x20) // BIND_TIC[i]
			{
				break;
			}
		}
		if (i < 5) {
			if (dump_tsc && objdata->tsc.gpu_mapping && (
				    is_tsc || objdata->linked_tsc))
			{
				int tsc;
				if (is_tsc)
					tsc = (data >> 12) & 0xfff;
				else
					tsc = (data >> 9) & 0x1ffff;
				mmt_debug("bind tsc[%d]: 0x%08x\n", i, tsc);
				uint32_t *tsc_data = gpu_mapping_get_data(objdata->tsc.gpu_mapping, objdata->tsc.address + 32 * tsc, 8 * 4);

				decode_tsc(objdata->texture_ctx, tsc, tsc_data);

			}
			if (dump_tic && objdata->tic.gpu_mapping && !is_tsc)
			{
				int tic = (data >> 9) & 0x1ffff;
				mmt_debug("bind tic[%d]: 0x%08x\n", i, tic);
				uint32_t *tic_data = gpu_mapping_get_data(objdata->tic.gpu_mapping, objdata->tic.address + 32 * tic, 8 * 4);

				decode_tic(objdata->texture_ctx, tic_domain, tic, tic_data);

			}
		}
	}
	else if (decode_macro(pstate, &objdata->macro))
	{ }
}
