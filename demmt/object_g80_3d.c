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
#include "object.h"

static struct
{
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
} g80_3d;

static const struct disisa *isa_g80 = NULL;

static void __g80_3d_disassemble(uint8_t *data, struct region *reg, const char *mode, uint32_t start_id)
{
	mmt_debug("%s_start id 0x%08x\n", mode, start_id);
	for ( ; reg != NULL; reg = reg->next)
	{
		if (reg->start != start_id)
			continue;

		if (MMT_DEBUG)
		{
			uint32_t x;
			mmt_debug("CODE: %s", "");
			for (x = reg->start; x < reg->end; x += 4)
				mmt_debug_cont("0x%08x ", *(uint32_t *)(data + x));
			mmt_debug_cont("%s\n", "");
		}

		if (!isa_g80)
			isa_g80 = ed_getisa("g80");

		struct varinfo *var = varinfo_new(isa_g80->vardata);

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

		envydis(isa_g80, stdout, data + reg->start, 0,
				reg->end - reg->start, var, 0, NULL, 0, colors);
		varinfo_del(var);
		break;
	}
}

static void g80_3d_disassemble(struct addr_n_buf *anb, const char *mode, uint32_t start_id)
{
	uint8_t *data = NULL;
	struct region *reg;

	struct gpu_mapping *m = anb->gpu_mapping;
	if (m)
	{
		data = gpu_mapping_get_data(m, anb->address, 0);
		reg = m->object->written_regions.head;
		if (anb->address != m->address)
			data = NULL;// FIXME, something similar to code below
		/*
		if (anb->address > m->address)
			start_id += anb->address - m->address;
		else if (anb->address < m->address)
			start_id -= m->address - anb->address;
		 */
	}
	if (data)
		__g80_3d_disassemble(data, reg, mode, start_id);
}

static struct mthd2addr g80_3d_addresses[] =
{
	{ 0x0200, 0x0204, &g80_3d.rt[0], 8, 32 },
	{ 0x0904, 0x0908, &g80_3d.vertex_array_start[0], 16, 16 },
	{ 0x1080, 0x1084, &g80_3d.vertex_array_limit[0], 16, 8 },

	{ 0x0f70, 0x0f74, &g80_3d.gp },
	{ 0x0f7c, 0x0f80, &g80_3d.vp },
	{ 0x0f84, 0x0f88, &g80_3d.vertex_runout },
	{ 0x0fa4, 0x0fa8, &g80_3d.fp },
	{ 0x0fe0, 0x0fe4, &g80_3d.zeta },
	{ 0x155c, 0x1560, &g80_3d.tsc },
	{ 0x1574, 0x1578, &g80_3d.tic },
	{ 0x1280, 0x1284, &g80_3d.cb_def },
	{ 0x1b00, 0x1b04, &g80_3d.query },
	{ 0, 0, NULL }
};

void decode_g80_3d_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, g80_3d_addresses))
	{ }
}

void decode_g80_3d_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, g80_3d_addresses))
	{ }
	else if (mthd == 0x140c && dump_vp) // VP_START_ID
		g80_3d_disassemble(&g80_3d.vp, "vp", data);
	else if (mthd == 0x1414 && dump_fp) // FP_START_ID
		g80_3d_disassemble(&g80_3d.fp, "fp", data);
	else if (mthd == 0x1410 && dump_gp) // GP_START_ID
		g80_3d_disassemble(&g80_3d.gp, "gp", data);
	else if (mthd >= 0x1444 && mthd < 0x1448 + 0x8 * 3)
	{
		int i;

		for (i = 0; i < 3; ++i)
		{
			if (dump_tsc && g80_3d.tsc.gpu_mapping && mthd == 0x1444 + i * 0x8) // BIND_TSC[i]
			{
				int j, tsc = (data >> 12) & 0xff;
				mmt_debug("bind tsc[%d]: 0x%08x\n", i, tsc);
				uint32_t *tsc_data = gpu_mapping_get_data(g80_3d.tsc.gpu_mapping, g80_3d.tsc.address + 8 * tsc, 8 * 4);

				for (j = 0; j < 8; ++j)
					decode_tsc(tsc, j, tsc_data);

				break;
			}
			if (dump_tic && g80_3d.tic.gpu_mapping && mthd == 0x1448 + i * 0x8) // BIND_TIC[i]
			{
				int j, tic = (data >> 9) & 0x1ffff;
				mmt_debug("bind tic[%d]: 0x%08x\n", i, tic);
				uint32_t *tic_data = gpu_mapping_get_data(g80_3d.tic.gpu_mapping, g80_3d.tic.address + 8 * tic, 8 * 4);

				for (j = 0; j < 8; ++j)
					decode_tic(tic, j, tic_data);

				break;
			}
		}
	}
}
