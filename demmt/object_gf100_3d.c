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

#include "config.h"
#include "demmt.h"
#include "log.h"
#include "macro.h"
#include "object.h"

static struct
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
} gf100_3d;

static void decode_gf100_p_header(int idx, uint32_t *data, struct rnndomain *header_domain)
{
	struct rnndecaddrinfo *ai = rnndec_decodeaddr(gf100_shaders_ctx, header_domain, idx * 4, 1);

	char *dec_addr = ai->name;
	char *dec_val = rnndec_decodeval(gf100_shaders_ctx, ai->typeinfo, data[idx], ai->width);

	fprintf(stdout, "0x%08x   %s = %s\n", data[idx], dec_addr, dec_val);

	free(ai);
	free(dec_val);
	free(dec_addr);
}

static struct rnndomain *gf100_p_header_domain(int program)
{
	if (program == 0 || program == 1) // VP
		return gf100_vp_header_domain;
	else if (program == 2) // TCP
		return gf100_tcp_header_domain;
	else if (program == 3) // TEP
		return gf100_tep_header_domain;
	else if (program == 4) // GP
		return gf100_gp_header_domain;
	else if (program == 5) // FP
		return gf100_fp_header_domain;
	else
		return NULL;
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

static struct mthd2addr gf100_3d_addresses[] =
{
	{ 0x0010, 0x0014, &gf100_3d.subchan.semaphore },
	{ 0x0104, 0x0108, &gf100_3d.graph.notify },

	{ 0x0790, 0x0794, &gf100_3d.temp },
	{ 0x07e8, 0x07ec, &gf100_3d.zcull },
	{ 0x07f0, 0x07f4, &gf100_3d.zcull_limit },
	{ 0x0f84, 0x0f88, &gf100_3d.vertex_runout },
	{ 0x0fe0, 0x0fe4, &gf100_3d.zeta },
	{ 0x155c, 0x1560, &gf100_3d.tsc },
	{ 0x1574, 0x1578, &gf100_3d.tic },
	{ 0x1608, 0x160c, &gf100_3d.code },
	{ 0x17bc, 0x17c0, &gf100_3d.vertex_quarantine },
	{ 0x17c8, 0x17cc, &gf100_3d.index_array_start },
	{ 0x17d0, 0x17d4, &gf100_3d.index_array_limit },
	{ 0x1b00, 0x1b04, &gf100_3d.query },
	{ 0x2384, 0x2388, &gf100_3d.cb },

	{ 0x0800, 0x0804, &gf100_3d.rt[0], 8, 64 },
	{ 0x1c04, 0x1c08, &gf100_3d.vertex_array_start[0], 32, 16 },
	{ 0x1f00, 0x1f04, &gf100_3d.vertex_array_limit[0], 32, 8 },
	{ 0x2700, 0x2704, &gf100_3d.image[0], 8, 0x20 },

	{ 0, 0, NULL }
};

void decode_gf100_3d_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, gf100_3d_addresses))
	{ }
}

void decode_gf100_3d_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, gf100_3d_addresses))
	{ }
	else if (gf100_3d.code.buffer && mthd >= 0x2000 && mthd < 0x2000 + 0x40 * 6) // SP
	{
		int i;
		for (i = 0; i < 6; ++i)
		{
			if (mthd != 0x2004 + i * 0x40) // SP[i].START_ID
				continue;

			mmt_debug("start id[%d]: 0x%08x\n", i, data);
			struct region *reg;
			for (reg = gf100_3d.code.buffer->written_regions.head; reg != NULL; reg = reg->next)
			{
				if (reg->start != data)
					continue;

				uint32_t x;
				x = *(uint32_t *)(gf100_3d.code.buffer->data + reg->start);
				int program = (x >> 10) & 0x7;
				if (!gf100_p_dump(program))
					break;

				struct rnndomain *header_domain = gf100_p_header_domain(program);
				fprintf(stdout, "HEADER:\n");
				if (header_domain)
					for (x = 0; x < 20; ++x)
						decode_gf100_p_header(x, (uint32_t *)(gf100_3d.code.buffer->data + reg->start), header_domain);
				else
					for (x = reg->start; x < reg->start + 20 * 4; x += 4)
						fprintf(stdout, "0x%08x\n", *(uint32_t *)(gf100_3d.code.buffer->data + x));

				fprintf(stdout, "CODE:\n");
				if (MMT_DEBUG)
				{
					uint32_t x;
					mmt_debug("%s", "");
					for (x = reg->start + 20 * 4; x < reg->end; x += 4)
						mmt_debug_cont("0x%08x ", *(uint32_t *)(gf100_3d.code.buffer->data + x));
					mmt_debug_cont("%s\n", "");
				}

				struct varinfo *var = NULL;
				const struct disisa *isa;
				if (chipset >= 0x117)
				{
					if (!isa_gm107)
						isa_gm107 = ed_getisa("gm107");
					isa = isa_gm107;
				}
				else if (chipset >= 0xf0)
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

					if (chipset >= 0xe4)
						varinfo_set_variant(var, "gk104");
				}

				envydis(isa, stdout, gf100_3d.code.buffer->data + reg->start + 20 * 4, 0,
						reg->end - reg->start - 20 * 4, var, 0, NULL, 0, colors);

				if (var)
					varinfo_del(var);
				break;
			}
			break;
		}
	}
	else if (chipset < 0xe0 && mthd >= 0x2400 && mthd < 0x2404 + 0x20 * 5)
	{
		int i;
		for (i = 0; i < 5; ++i)
		{
			if (dump_tsc && gf100_3d.tsc.buffer && mthd == 0x2400 + i * 0x20) // BIND_TSC[i]
			{
				int j, tsc = (data >> 12) & 0xfff;
				mmt_debug("bind tsc[%d]: 0x%08x\n", i, tsc);
				uint32_t *tsc_data = (uint32_t *)&gf100_3d.tsc.buffer->data[32 * tsc];

				for (j = 0; j < 8; ++j)
					decode_tsc(tsc, j, tsc_data);

				break;
			}
			if (dump_tic && gf100_3d.tic.buffer && mthd == 0x2404 + i * 0x20) // BIND_TIC[i]
			{
				int j, tic = (data >> 9) & 0x1ffff;
				mmt_debug("bind tic[%d]: 0x%08x\n", i, tic);
				uint32_t *tic_data = (uint32_t *)&gf100_3d.tic.buffer->data[32 * tic];

				for (j = 0; j < 8; ++j)
					decode_tic(tic, j, tic_data);

				break;
			}
		}
	}
	else if (decode_macro(pstate, &gf100_3d.macro))
	{ }
}
