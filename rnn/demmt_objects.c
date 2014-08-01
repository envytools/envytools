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

#include "demmt.h"
#include "demmt_objects.h"
#include "demmt_macro.h"
#include "dis.h"
#include "rnndec.h"
#include <stdlib.h>

static struct buffer *find_buffer_by_gpu_address(uint64_t addr)
{
	struct buffer *buf, *ret = NULL;

	for (buf = buffers_list; buf != NULL; buf = buf->next)
	{
		if (addr >= buf->gpu_start && addr < buf->gpu_start + buf->length)
		{
			ret = buf;
			break;
		}
	}

	if (ret == NULL)
		for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
		{
			if (addr >= buf->gpu_start && addr < buf->gpu_start + buf->length)
			{
				ret = buf;
				break;
			}
		}

	if (ret && ret->data == NULL)
		ret->data = calloc(ret->length, 1);

	return ret;
}

static void decode_tsc(uint32_t tsc, int idx, uint32_t *data)
{
	struct rnndecaddrinfo *ai = rnndec_decodeaddr(nv50_texture_ctx, tsc_domain, idx * 4, 1);

	char *dec_addr = ai->name;
	char *dec_val = rnndec_decodeval(nv50_texture_ctx, ai->typeinfo, data[idx], ai->width);

	fprintf(stdout, "TSC[%d]: 0x%08x   %s = %s\n", tsc, data[idx], dec_addr, dec_val);

	free(ai);
	free(dec_val);
	free(dec_addr);
}

static void decode_tic(uint32_t tic, int idx, uint32_t *data)
{
	struct rnndecaddrinfo *ai = rnndec_decodeaddr(nv50_texture_ctx, tic_domain, idx * 4, 1);

	char *dec_addr = ai->name;
	char *dec_val = rnndec_decodeval(nv50_texture_ctx, ai->typeinfo, data[idx], ai->width);

	fprintf(stdout, "TIC[%d]: 0x%08x   %s = %s\n", tic, data[idx], dec_addr, dec_val);

	free(ai);
	free(dec_val);
	free(dec_addr);
}

struct addr_n_buf
{
	uint64_t address;
	struct buffer *buffer;
};

static void anb_set_high(struct addr_n_buf *s, uint32_t data);
static struct buffer *anb_set_low(struct addr_n_buf *s, uint32_t data);

struct nv01_subchan
{
	struct addr_n_buf semaphore;
};

static int decode_nv01_subchan_terse(struct pushbuf_decode_state *pstate, int mthd, uint32_t data, struct nv01_subchan *subchan)
{
	if (chipset >= 0x84 && mthd == 0x0010) // SUBCHAN.SEMAPHORE_ADDRESS_HIGH
		anb_set_high(&subchan->semaphore, data);
	else if (chipset >= 0x84 && mthd == 0x0014) // SUBCHAN.SEMAPHORE_ADDRESS_LOW
		anb_set_low(&subchan->semaphore, data);
	else
		return 0;

	return 1;
}

static struct
{
	struct addr_n_buf offset_in;
	struct addr_n_buf offset_out;
} nv50_m2mf;

static void decode_nv50_m2mf_terse(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x0238) // OFFSET_IN_HIGH
		anb_set_high(&nv50_m2mf.offset_in, data);
	else if (mthd == 0x030c) // OFFSET_IN_LOW
		anb_set_low(&nv50_m2mf.offset_in, data);
	else if (mthd == 0x023c) // OFFSET_OUT_HIGH
		anb_set_high(&nv50_m2mf.offset_out, data);
	else if (mthd == 0x0310) // OFFSET_OUT_LOW
		anb_set_low(&nv50_m2mf.offset_out, data);
}

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
} nv50_3d;

static const struct disisa *isa_nv50 = NULL;

static void nv50_3d_disassemble(struct buffer *buf, const char *mode, uint32_t start_id)
{
	if (!buf)
		return;

	mmt_debug("%s_start id 0x%08x\n", mode, start_id);
	struct region *reg;
	for (reg = buf->written_regions; reg != NULL; reg = reg->next)
	{
		if (reg->start != start_id)
			continue;

		if (MMT_DEBUG)
		{
			uint32_t x;
			mmt_debug("CODE: %s", "");
			for (x = reg->start; x < reg->end; x += 4)
				mmt_debug_cont("0x%08x ", *(uint32_t *)(buf->data + x));
			mmt_debug_cont("%s\n", "");
		}

		if (!isa_nv50)
			isa_nv50 = ed_getisa("nv50");

		struct varinfo *var = varinfo_new(isa_nv50->vardata);

		if (chipset == 0x50)
			varinfo_set_variant(var, "nv50");
		else if (chipset >= 0x84 && chipset <= 0x98)
			varinfo_set_variant(var, "nv84");
		else if (chipset == 0xa0)
			varinfo_set_variant(var, "nva0");
		else if (chipset >= 0xaa && chipset <= 0xac)
			varinfo_set_variant(var, "nvaa");
		else if ((chipset >= 0xa3 && chipset <= 0xa8) || chipset == 0xaf)
			varinfo_set_variant(var, "nva3");

		varinfo_set_mode(var, mode);

		envydis(isa_nv50, stdout, buf->data + reg->start, 0,
				reg->end - reg->start, var, 0, NULL, 0, colors);
		varinfo_del(var);
		break;
	}
}

static void decode_nv50_3d_terse(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x0f7c) // VP_ADDRESS_HIGH
		anb_set_high(&nv50_3d.vp, data);
	else if (mthd == 0x0f80) // VP_ADDRESS_LOW
		anb_set_low(&nv50_3d.vp, data);
	else if (mthd == 0x0fa4) // FP_ADDRESS_HIGH
		anb_set_high(&nv50_3d.fp, data);
	else if (mthd == 0x0fa8) // FP_ADDRESS_LOW
		anb_set_low(&nv50_3d.fp, data);
	else if (mthd == 0x0f70) // GP_ADDRESS_HIGH
		anb_set_high(&nv50_3d.gp, data);
	else if (mthd == 0x0f74) // GP_ADDRESS_LOW
		anb_set_low(&nv50_3d.gp, data);
	else if (mthd == 0x155c) // TSC_ADDRESS_HIGH
		anb_set_high(&nv50_3d.tsc, data);
	else if (mthd == 0x1560) // TSC_ADDRESS_LOW
		anb_set_low(&nv50_3d.tsc, data);
	else if (mthd == 0x1574) // TIC_ADDRESS_HIGH
		anb_set_high(&nv50_3d.tic, data);
	else if (mthd == 0x1578) // TIC_ADDRESS_LOW
		anb_set_low(&nv50_3d.tic, data);
	else if (mthd == 0x0fe0) // ZETA_ADDRESS_HIGH
		anb_set_high(&nv50_3d.zeta, data);
	else if (mthd == 0x0fe4) // ZETA_ADDRESS_LOW
		anb_set_low(&nv50_3d.zeta, data);
	else if (mthd == 0x1b00) // QUERY_ADDRESS_HIGH
		anb_set_high(&nv50_3d.query, data);
	else if (mthd == 0x1b04) // QUERY_ADDRESS_LOW
		anb_set_low(&nv50_3d.query, data);
	else if (mthd == 0x1280) // CB_DEF_ADDRESS_HIGH
		anb_set_high(&nv50_3d.cb_def, data);
	else if (mthd == 0x1284) // CB_DEF_ADDRESS_LOW
		anb_set_low(&nv50_3d.cb_def, data);
	else if (mthd == 0x0f84) // VERTEX_RUNOUT_ADDRESS_HIGH
		anb_set_high(&nv50_3d.vertex_runout, data);
	else if (mthd == 0x0f88) // VERTEX_RUNOUT_ADDRESS_LOW
		anb_set_low(&nv50_3d.vertex_runout, data);
	else if (mthd >= 0x0200 && mthd < 0x0200 + 8 * 32)
	{
		int i;
		for (i = 0; i < 8; ++i)
		{
			if (mthd == 0x0200 + i * 32) // RT[i]_ADDRESS_HIGH
			{
				anb_set_high(&nv50_3d.rt[i], data);
				break;
			}
			else if (mthd == 0x0204 + i * 32) // RT[i]_ADDRESS_LOW
			{
				anb_set_low(&nv50_3d.rt[i], data);
				break;
			}
		}
	}
	else if (mthd >= 0x0904 && mthd < 0x0904 + 16 * 16)
	{
		int i;
		for (i = 0; i < 16; ++i)
		{
			if (mthd == 0x0904 + i * 16) // VERTEX_ARRAY_START_HIGH[i]
			{
				anb_set_high(&nv50_3d.vertex_array_start[i], data);
				break;
			}
			else if (mthd == 0x0908 + i * 16) // VERTEX_ARRAY_START_LOW[i]
			{
				anb_set_low(&nv50_3d.vertex_array_start[i], data);
				break;
			}
		}
	}
	else if (mthd >= 0x1080 && mthd < 0x1080 + 16 * 8)
	{
		int i;
		for (i = 0; i < 16; ++i)
		{
			if (mthd == 0x1080 + i * 8) // VERTEX_ARRAY_LIMIT_HIGH[i]
			{
				anb_set_high(&nv50_3d.vertex_array_limit[i], data);
				break;
			}
			else if (mthd == 0x1084 + i * 8) // VERTEX_ARRAY_LIMIT_LOW[i]
			{
				anb_set_low(&nv50_3d.vertex_array_limit[i], data);
				break;
			}
		}
	}
}

static void decode_nv50_3d_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x0f80) // VP_ADDRESS_LOW
		mmt_debug("buffer found: %d\n", nv50_3d.vp.buffer ? 1 : 0);
	else if (mthd == 0x0fa8) // FP_ADDRESS_LOW
		mmt_debug("buffer found: %d\n", nv50_3d.fp.buffer ? 1 : 0);
	else if (mthd == 0x0f74) // GP_ADDRESS_LOW
		mmt_debug("buffer found: %d\n", nv50_3d.gp.buffer ? 1 : 0);
	else if (mthd == 0x140c) // VP_START_ID
		nv50_3d_disassemble(nv50_3d.vp.buffer, "vp", data);
	else if (mthd == 0x1414) // FP_START_ID
		nv50_3d_disassemble(nv50_3d.fp.buffer, "fp", data);
	else if (mthd == 0x1410) // GP_START_ID
		nv50_3d_disassemble(nv50_3d.gp.buffer, "gp", data);
	else if (mthd == 0x1560) // TSC_ADDRESS_LOW
		mmt_debug("buffer found: %d\n", nv50_3d.tsc.buffer ? 1 : 0);
	else if (mthd == 0x1578) // TIC_ADDRESS_LOW
		mmt_debug("buffer found: %d\n", nv50_3d.tic.buffer ? 1 : 0);
	else if (mthd >= 0x1444 && mthd < 0x1448 + 0x8 * 3)
	{
		int i;
		for (i = 0; i < 3; ++i)
		{
			if (nv50_3d.tsc.buffer && mthd == 0x1444 + i * 0x8) // BIND_TSC[i]
			{
				int j, tsc = (data >> 12) & 0xff;
				mmt_debug("bind tsc[%d]: 0x%08x\n", i, tsc);
				uint32_t *tsc_data = (uint32_t *)&nv50_3d.tsc.buffer->data[8 * tsc];

				for (j = 0; j < 8; ++j)
					decode_tsc(tsc, j, tsc_data);

				break;
			}
			if (nv50_3d.tic.buffer && mthd == 0x1448 + i * 0x8) // BIND_TIC[i]
			{
				int j, tic = (data >> 9) & 0x1ffff;
				mmt_debug("bind tic[%d]: 0x%08x\n", i, tic);
				uint32_t *tic_data = (uint32_t *)&nv50_3d.tic.buffer->data[8 * tic];

				for (j = 0; j < 8; ++j)
					decode_tic(tic, j, tic_data);

				break;
			}
		}
	}
}

static struct
{
	struct addr_n_buf dst;
	struct addr_n_buf src;

	uint32_t dst_linear;
	int check_dst_buffer;
	int data_offset;
} nv50_2d;

static void decode_nv50_2d_terse(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x0220) // DST_ADDRESS_HIGH
		anb_set_high(&nv50_2d.dst, data);
	else if (mthd == 0x0224) // DST_ADDRESS_LOW
	{
		anb_set_low(&nv50_2d.dst, data);
		nv50_2d.check_dst_buffer = nv50_2d.dst.buffer != NULL;

		if (nv50_2d.dst.buffer)
			nv50_2d.data_offset = nv50_2d.dst.address - nv50_2d.dst.buffer->gpu_start;
	}
	else if (mthd == 0x0250) // SRC_ADDRESS_HIGH
		anb_set_high(&nv50_2d.src, data);
	else if (mthd == 0x0254) // SRC_ADDRESS_LOW
		anb_set_low(&nv50_2d.src, data);
}

static void decode_nv50_2d_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x0224) // DST_ADDRESS_LOW
		mmt_debug("buffer found: %d\n", nv50_2d.dst.buffer ? 1 : 0);
	else if (mthd == 0x0204) // DST_LINEAR
		nv50_2d.dst_linear = data;
	else if (mthd == 0x0860) // SIFC_DATA
	{
		if (nv50_2d.check_dst_buffer)
		{
			nv50_2d.check_dst_buffer = 0;

			if (!nv50_2d.dst_linear)
				nv50_2d.dst.buffer = NULL;
		}

		if (nv50_2d.dst.buffer != NULL)
		{
			mmt_debug("2d sifc_data: 0x%08x\n", data);
			buffer_register_write(nv50_2d.dst.buffer, nv50_2d.data_offset, 4, &data);
			nv50_2d.data_offset += 4;
		}
	}
}

struct nv01_graph
{
	struct addr_n_buf notify;
};

static int decode_nv01_graph_terse(struct pushbuf_decode_state *pstate, int mthd, uint32_t data, struct nv01_graph *graph)
{
	if (chipset >= 0xc0 && mthd == 0x0104) // GRAPH.NOTIFY_ADDRESS_HIGH
		anb_set_high(&graph->notify, data);
	else if (chipset >= 0xc0 && mthd == 0x0108) // GRAPH.NOTIFY_ADDRESS_LOW
		anb_set_low(&graph->notify, data);
	else
		return 0;

	return 1;
}

static const struct disisa *isa_nvc0 = NULL;

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

	struct nv01_graph graph;
	struct nv01_subchan subchan;
} nvc0_3d;

static void decode_nvc0_p_header(int idx, uint32_t *data, struct rnndomain *header_domain)
{
	struct rnndecaddrinfo *ai = rnndec_decodeaddr(nvc0_shaders_ctx, header_domain, idx * 4, 1);

	char *dec_addr = ai->name;
	char *dec_val = rnndec_decodeval(nvc0_shaders_ctx, ai->typeinfo, data[idx], ai->width);

	fprintf(stdout, "0x%08x   %s = %s\n", data[idx], dec_addr, dec_val);

	free(ai);
	free(dec_val);
	free(dec_addr);
}

static struct rnndomain *nvc0_p_header_domain(int program)
{
	if (program == 0 || program == 1) // VP
		return nvc0_vp_header_domain;
	else if (program == 2) // TCP
		return nvc0_tcp_header_domain;
	else if (program == 3) // TEP
		return nvc0_tep_header_domain;
	else if (program == 4) // GP
		return nvc0_gp_header_domain;
	else if (program == 5) // FP
		return nvc0_fp_header_domain;
	else
		return NULL;
}

static void decode_nvc0_3d_terse(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x1608) // CODE_ADDRESS_HIGH
		anb_set_high(&nvc0_3d.code, data);
	else if (mthd == 0x160c) // CODE_ADDRESS_LOW
		anb_set_low(&nvc0_3d.code, data);
	else if (mthd == 0x155c) // TSC_ADDRESS_HIGH
		anb_set_high(&nvc0_3d.tsc, data);
	else if (mthd == 0x1560) // TSC_ADDRESS_LOW
		anb_set_low(&nvc0_3d.tsc, data);
	else if (mthd == 0x1574) // TIC_ADDRESS_HIGH
		anb_set_high(&nvc0_3d.tic, data);
	else if (mthd == 0x1578) // TIC_ADDRESS_LOW
		anb_set_low(&nvc0_3d.tic, data);
	else if (mthd == 0x1b00) // QUERY_ADDRESS_HIGH
		anb_set_high(&nvc0_3d.query, data);
	else if (mthd == 0x1b04) // QUERY_ADDRESS_LOW
		anb_set_low(&nvc0_3d.query, data);
	else if (mthd == 0x17bc) // VERTEX_QUARANTINE_ADDRESS_HIGH
		anb_set_high(&nvc0_3d.vertex_quarantine, data);
	else if (mthd == 0x17c0) // VERTEX_QUARANTINE_ADDRESS_LOW
		anb_set_low(&nvc0_3d.vertex_quarantine, data);
	else if (mthd == 0x0f84) // VERTEX_RUNOUT_ADDRESS_HIGH
		anb_set_high(&nvc0_3d.vertex_runout, data);
	else if (mthd == 0x0f88) // VERTEX_RUNOUT_ADDRESS_LOW
		anb_set_low(&nvc0_3d.vertex_runout, data);
	else if (mthd == 0x0790) // TEMP_ADDRESS_HIGH
		anb_set_high(&nvc0_3d.temp, data);
	else if (mthd == 0x0794) // TEMP_ADDRESS_LOW
		anb_set_low(&nvc0_3d.temp, data);
	else if (mthd == 0x0fe0) // ZETA_ADDRESS_HIGH
		anb_set_high(&nvc0_3d.zeta, data);
	else if (mthd == 0x0fe4) // ZETA_ADDRESS_LOW
		anb_set_low(&nvc0_3d.zeta, data);
	else if (mthd == 0x07e8) // ZCULL_ADDRESS_HIGH
		anb_set_high(&nvc0_3d.zcull, data);
	else if (mthd == 0x07ec) // ZCULL_ADDRESS_LOW
		anb_set_low(&nvc0_3d.zcull, data);
	else if (mthd == 0x07f0) // ZCULL_LIMIT_HIGH
		anb_set_high(&nvc0_3d.zcull_limit, data);
	else if (mthd == 0x07f4) // ZCULL_LIMIT_LOW
		anb_set_low(&nvc0_3d.zcull_limit, data);
	else if (mthd == 0x17c8) // INDEX_ARRAY_START_HIGH
		anb_set_high(&nvc0_3d.index_array_start, data);
	else if (mthd == 0x17cc) // INDEX_ARRAY_START_LOW
		anb_set_low(&nvc0_3d.index_array_start, data);
	else if (mthd == 0x17d0) // INDEX_ARRAY_LIMIT_HIGH
		anb_set_high(&nvc0_3d.index_array_limit, data);
	else if (mthd == 0x17d4) // INDEX_ARRAY_LIMIT_LOW
		anb_set_low(&nvc0_3d.index_array_limit, data);
	else if (mthd == 0x2384) // CB_ADDRESS_HIGH
		anb_set_high(&nvc0_3d.cb, data);
	else if (mthd == 0x2388) // CB_ADDRESS_LOW
		anb_set_low(&nvc0_3d.cb, data);
	else if (decode_nv01_subchan_terse(pstate, mthd, data, &nvc0_3d.subchan))
	{ }
	else if (decode_nv01_graph_terse(pstate, mthd, data, &nvc0_3d.graph))
	{ }
	else if (mthd >= 0x0800 && mthd < 0x0800 + 8 * 64)
	{
		int i;
		for (i = 0; i < 8; ++i)
		{
			if (mthd == 0x0800 + i * 64) // RT[i]_ADDRESS_HIGH
			{
				anb_set_high(&nvc0_3d.rt[i], data);
				break;
			}
			else if (mthd == 0x0804 + i * 64) // RT[i]_ADDRESS_LOW
			{
				anb_set_low(&nvc0_3d.rt[i], data);
				break;
			}
		}
	}
	else if (mthd >= 0x1c04 && mthd < 0x1c04 + 32 * 16)
	{
		int i;
		for (i = 0; i < 32; ++i)
		{
			if (mthd == 0x1c04 + i * 16) // VERTEX_ARRAY_START_HIGH[i]
			{
				anb_set_high(&nvc0_3d.vertex_array_start[i], data);
				break;
			}
			else if (mthd == 0x1c08 + i * 16) // VERTEX_ARRAY_START_LOW[i]
			{
				anb_set_low(&nvc0_3d.vertex_array_start[i], data);
				break;
			}
		}
	}
	else if (mthd >= 0x1f00 && mthd < 0x1f00 + 32 * 8)
	{
		int i;
		for (i = 0; i < 32; ++i)
		{
			if (mthd == 0x1f00 + i * 8) // VERTEX_ARRAY_LIMIT_HIGH[i]
			{
				anb_set_high(&nvc0_3d.vertex_array_limit[i], data);
				break;
			}
			else if (mthd == 0x1f04 + i * 8) // VERTEX_ARRAY_LIMIT_LOW[i]
			{
				anb_set_low(&nvc0_3d.vertex_array_limit[i], data);
				break;
			}
		}
	}
	else if (chipset < 0xe0 && mthd >= 0x2700 && mthd < 0x2700 + 8 * 0x20)
	{
		int i;
		for (i = 0; i < 8; ++i)
		{
			if (mthd == 0x2700 + i * 0x20) // IMAGE.ADDRESS_HIGH[i]
			{
				anb_set_high(&nvc0_3d.image[i], data);
				break;
			}
			else if (mthd == 0x2704 + i * 0x20) // IMAGE.ADDRESS_LOW[i]
			{
				anb_set_low(&nvc0_3d.image[i], data);
				break;
			}
		}
	}
}

static void decode_nvc0_3d_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x160c) // CODE_ADDRESS_LOW
		mmt_debug("buffer found: %d\n", nvc0_3d.code.buffer ? 1 : 0);
	else if (nvc0_3d.code.buffer && mthd >= 0x2000 && mthd < 0x2000 + 0x40 * 6) // SP
	{
		int i;
		for (i = 0; i < 6; ++i)
		{
			if (mthd != 0x2004 + i * 0x40) // SP[i].START_ID
				continue;

			mmt_debug("start id[%d]: 0x%08x\n", i, data);
			struct region *reg;
			for (reg = nvc0_3d.code.buffer->written_regions; reg != NULL; reg = reg->next)
			{
				if (reg->start != data)
					continue;

				uint32_t x;
				x = *(uint32_t *)(nvc0_3d.code.buffer->data + reg->start);
				int program = (x >> 10) & 0x7;
				struct rnndomain *header_domain = nvc0_p_header_domain(program);
				fprintf(stdout, "HEADER:\n");
				if (header_domain)
					for (x = 0; x < 20; ++x)
						decode_nvc0_p_header(x, (uint32_t *)(nvc0_3d.code.buffer->data + reg->start), header_domain);
				else
					for (x = reg->start; x < reg->start + 20 * 4; x += 4)
						fprintf(stdout, "0x%08x\n", *(uint32_t *)(nvc0_3d.code.buffer->data + x));

				fprintf(stdout, "CODE:\n");
				if (MMT_DEBUG)
				{
					uint32_t x;
					mmt_debug("%s", "");
					for (x = reg->start + 20 * 4; x < reg->end; x += 4)
						mmt_debug_cont("0x%08x ", *(uint32_t *)(nvc0_3d.code.buffer->data + x));
					mmt_debug_cont("%s\n", "");
				}

				if (!isa_nvc0)
					isa_nvc0 = ed_getisa("nvc0");
				struct varinfo *var = varinfo_new(isa_nvc0->vardata);

				if (chipset >= 0xe4)
					varinfo_set_variant(var, "nve4");

				envydis(isa_nvc0, stdout, nvc0_3d.code.buffer->data + reg->start + 20 * 4, 0,
						reg->end - reg->start - 20 * 4, var, 0, NULL, 0, colors);
				varinfo_del(var);
				break;
			}
			break;
		}
	}
	else if (mthd == 0x1560) // TSC_ADDRESS_LOW
		mmt_debug("buffer found: %d\n", nvc0_3d.tsc.buffer ? 1 : 0);
	else if (mthd == 0x1578) // TIC_ADDRESS_LOW
		mmt_debug("buffer found: %d\n", nvc0_3d.tic.buffer ? 1 : 0);
	else if (chipset < 0xe0 && mthd >= 0x2400 && mthd < 0x2404 + 0x20 * 5)
	{
		int i;
		for (i = 0; i < 5; ++i)
		{
			if (nvc0_3d.tsc.buffer && mthd == 0x2400 + i * 0x20) // BIND_TSC[i]
			{
				int j, tsc = (data >> 12) & 0xfff;
				mmt_debug("bind tsc[%d]: 0x%08x\n", i, tsc);
				uint32_t *tsc_data = (uint32_t *)&nvc0_3d.tsc.buffer->data[32 * tsc];

				for (j = 0; j < 8; ++j)
					decode_tsc(tsc, j, tsc_data);

				break;
			}
			if (nvc0_3d.tic.buffer && mthd == 0x2404 + i * 0x20) // BIND_TIC[i]
			{
				int j, tic = (data >> 9) & 0x1ffff;
				mmt_debug("bind tic[%d]: 0x%08x\n", i, tic);
				uint32_t *tic_data = (uint32_t *)&nvc0_3d.tic.buffer->data[32 * tic];

				for (j = 0; j < 8; ++j)
					decode_tic(tic, j, tic_data);

				break;
			}
		}
	}
	else if (decode_macro(pstate, &nvc0_3d.macro))
	{ }
}

static struct
{
	struct nv01_graph graph;
	struct addr_n_buf src;
	struct addr_n_buf dst;
} nvc0_2d;

static void decode_nvc0_2d_terse(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (decode_nv01_graph_terse(pstate, mthd, data, &nvc0_2d.graph))
	{ }
	else if (mthd == 0x0250) // SRC_ADDRESS_HIGH
		anb_set_high(&nvc0_2d.src, data);
	else if (mthd == 0x0254) // SRC_ADDRESS_LOW
		anb_set_low(&nvc0_2d.src, data);
	else if (mthd == 0x0220) // DST_ADDRESS_HIGH
		anb_set_high(&nvc0_2d.dst, data);
	else if (mthd == 0x0224) // DST_ADDRESS_LOW
		anb_set_low(&nvc0_2d.dst, data);
}

static struct
{
	struct addr_n_buf offset_in;
	struct addr_n_buf offset_out;
	struct addr_n_buf query;
	int data_offset;
} nvc0_m2mf;

static void decode_nvc0_m2mf_terse(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x0238) // OFFSET_OUT_HIGH
		anb_set_high(&nvc0_m2mf.offset_out, data);
	else if (mthd == 0x023c) // OFFSET_OUT_LOW
	{
		anb_set_low(&nvc0_m2mf.offset_out, data);

		if (nvc0_m2mf.offset_out.buffer)
			nvc0_m2mf.data_offset = nvc0_m2mf.offset_out.address - nvc0_m2mf.offset_out.buffer->gpu_start;
	}
	else if (mthd == 0x030c) // OFFSET_IN_HIGH
		anb_set_high(&nvc0_m2mf.offset_in, data);
	else if (mthd == 0x0310) // OFFSET_IN_LOW
		anb_set_low(&nvc0_m2mf.offset_in, data);
	else if (mthd == 0x032c) // QUERY_ADDRESS_HIGH
		anb_set_high(&nvc0_m2mf.query, data);
	else if (mthd == 0x0330) // QUERY_ADDRESS_LOW
		anb_set_low(&nvc0_m2mf.query, data);
}

static void decode_nvc0_m2mf_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x023c) // OFFSET_OUT_LOW
		mmt_debug("buffer found: %d\n", nvc0_m2mf.offset_out.buffer ? 1 : 0);
	else if (mthd == 0x0300) // EXEC
	{
		int flags_ok = (data & 0x111) == 0x111 ? 1 : 0;
		mmt_debug("m2mf exec: 0x%08x push&linear: %d\n", data, flags_ok);

		if (!flags_ok || nvc0_m2mf.offset_out.buffer == NULL)
		{
			nvc0_m2mf.offset_out.address = 0;
			nvc0_m2mf.offset_out.buffer = NULL;
		}
	}
	else if (mthd == 0x0304) // DATA
	{
		mmt_debug("m2mf data: 0x%08x\n", data);
		if (nvc0_m2mf.offset_out.buffer)
		{
			buffer_register_write(nvc0_m2mf.offset_out.buffer, nvc0_m2mf.data_offset, 4, &data);
			nvc0_m2mf.data_offset += 4;
		}
	}
}

static struct
{
	struct addr_n_buf upload_dst;
	int data_offset;
} nve0_p2mf;

static void decode_nve0_p2mf_terse(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x0188) // UPLOAD.DST_ADDRESS_HIGH
		anb_set_high(&nve0_p2mf.upload_dst, data);
	else if (mthd == 0x018c) // UPLOAD.DST_ADDRESS_LOW
	{
		anb_set_low(&nve0_p2mf.upload_dst, data);

		if (nve0_p2mf.upload_dst.buffer)
			nve0_p2mf.data_offset = nve0_p2mf.upload_dst.address - nve0_p2mf.upload_dst.buffer->gpu_start;
	}
}

static void decode_nve0_p2mf_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x018c) // UPLOAD.DST_ADDRESS_LOW
		mmt_debug("buffer found: %d\n", nve0_p2mf.upload_dst.buffer ? 1 : 0);
	else if (mthd == 0x01b0) // UPLOAD.EXEC
	{
		int flags_ok = (data & 0x1) == 0x1 ? 1 : 0;
		mmt_debug("p2mf exec: 0x%08x linear: %d\n", data, flags_ok);

		if (!flags_ok || nve0_p2mf.upload_dst.buffer == NULL)
		{
			nve0_p2mf.upload_dst.address = 0;
			nve0_p2mf.upload_dst.buffer = NULL;
		}
	}
	else if (mthd == 0x01b4) // UPLOAD.DATA
	{
		mmt_debug("p2mf data: 0x%08x\n", data);
		if (nve0_p2mf.upload_dst.buffer)
		{
			buffer_register_write(nve0_p2mf.upload_dst.buffer, nve0_p2mf.data_offset, 4, &data);
			nve0_p2mf.data_offset += 4;
		}
	}
}

static struct
{
	struct addr_n_buf src;
	struct addr_n_buf dst;
	struct addr_n_buf query;
} nve0_copy;

static void decode_nve0_copy_terse(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x0400) // SRC_ADDRESS_HIGH
		anb_set_high(&nve0_copy.src, data);
	else if (mthd == 0x0404) // SRC_ADDRESS_LOW
		anb_set_low(&nve0_copy.src, data);
	else if (mthd == 0x0408) // DST_ADDRESS_HIGH
		anb_set_high(&nve0_copy.dst, data);
	else if (mthd == 0x040c) // DST_ADDRESS_LOW
		anb_set_low(&nve0_copy.dst, data);
	else if (mthd == 0x0240) // QUERY_ADDRESS_HIGH
		anb_set_high(&nve0_copy.query, data);
	else if (mthd == 0x0244) // QUERY_ADDRESS_LOW
		anb_set_low(&nve0_copy.query, data);
}

static void anb_set_high(struct addr_n_buf *s, uint32_t data)
{
	s->address = ((uint64_t)data) << 32;
	s->buffer = NULL;
}

static struct buffer *anb_set_low(struct addr_n_buf *s, uint32_t data)
{
	s->address |= data;
	struct buffer *buf = s->buffer = find_buffer_by_gpu_address(s->address);
	fprintf(stdout, " [0x%lx]", s->address);
	if (buf)
	{
		if (s->address != buf->gpu_start)
			fprintf(stdout, " [0x%lx+0x%lx]", buf->gpu_start, s->address - buf->gpu_start);

		if (chipset >= 0xc0)
		{
			if (buf == nvc0_3d.code.buffer && s->address >= nvc0_3d.code.address)
				fprintf(stdout, " [CODE_ADDRESS+0x%lx]", s->address - nvc0_3d.code.address);
			if (buf == nvc0_3d.tic.buffer && s->address >= nvc0_3d.tic.address)
				fprintf(stdout, " [TIC+0x%lx]", s->address - nvc0_3d.tic.address);
			if (buf == nvc0_3d.tsc.buffer && s->address >= nvc0_3d.tsc.address)
				fprintf(stdout, " [TSC+0x%lx]", s->address - nvc0_3d.tsc.address);
		}
		else if (chipset >= 0x84 || chipset == 0x50)
		{
			if (buf == nv50_3d.vp.buffer && s->address >= nv50_3d.vp.address)
				fprintf(stdout, " [VP+0x%lx]", s->address - nv50_3d.vp.address);
			if (buf == nv50_3d.fp.buffer && s->address >= nv50_3d.fp.address)
				fprintf(stdout, " [FP+0x%lx]", s->address - nv50_3d.fp.address);
			if (buf == nv50_3d.gp.buffer && s->address >= nv50_3d.gp.address)
				fprintf(stdout, " [GP+0x%lx]", s->address - nv50_3d.gp.address);
			if (buf == nv50_3d.tic.buffer && s->address >= nv50_3d.tic.address)
				fprintf(stdout, " [TIC+0x%lx]", s->address - nv50_3d.tic.address);
			if (buf == nv50_3d.tsc.buffer && s->address >= nv50_3d.tsc.address)
				fprintf(stdout, " [TSC+0x%lx]", s->address - nv50_3d.tsc.address);
		}
	}

	return buf;
}

static const struct gpu_object_decoder objs[] =
{
		{ 0x502d, decode_nv50_2d_terse,   decode_nv50_2d_verbose },
		{ 0x5039, decode_nv50_m2mf_terse, NULL },
		{ 0x5097, decode_nv50_3d_terse,   decode_nv50_3d_verbose },
		{ 0x8297, decode_nv50_3d_terse,   decode_nv50_3d_verbose },
		{ 0x8397, decode_nv50_3d_terse,   decode_nv50_3d_verbose },
		{ 0x8597, decode_nv50_3d_terse,   decode_nv50_3d_verbose },
		{ 0x8697, decode_nv50_3d_terse,   decode_nv50_3d_verbose },
		{ 0x902d, decode_nvc0_2d_terse,   NULL },
		{ 0x9039, decode_nvc0_m2mf_terse, decode_nvc0_m2mf_verbose },
		{ 0x9097, decode_nvc0_3d_terse,   decode_nvc0_3d_verbose },
		{ 0x9197, decode_nvc0_3d_terse,   decode_nvc0_3d_verbose },
		{ 0x9297, decode_nvc0_3d_terse,   decode_nvc0_3d_verbose },
		{ 0xa040, decode_nve0_p2mf_terse, decode_nve0_p2mf_verbose },
		{ 0xa097, decode_nvc0_3d_terse,   decode_nvc0_3d_verbose },
		{ 0xa0b5, decode_nve0_copy_terse, NULL },
		{ 0, NULL, NULL }
};

const struct gpu_object_decoder *demmt_get_decoder(uint32_t class_)
{
	const struct gpu_object_decoder *o;
	for (o = objs; o->class_ != 0; o++)
		if (o->class_ == class_)
			return o;
	return NULL;
}
