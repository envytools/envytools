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
	if (addr == 0)
		return NULL;

	for (buf = buffers_list; buf != NULL; buf = buf->next)
	{
		if (!buf->gpu_start)
			continue;
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

static int is_buffer_valid(struct buffer *b)
{
	struct buffer *buf;

	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (b == buf)
			return 1;

	for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
		if (b == buf)
			return 1;

	return 0;
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
	struct buffer *prev_buffer;
};

static void anb_set_high(struct addr_n_buf *s, uint32_t data);
static struct buffer *anb_set_low(struct addr_n_buf *s, uint32_t data, const char *usage);

struct nv01_subchan
{
	struct addr_n_buf semaphore;
};

struct mthd2addr
{
	uint32_t high, low;
	struct addr_n_buf *buf;
	int length, stride;
};

static int check_addresses_terse(struct pushbuf_decode_state *pstate, struct mthd2addr *addresses)
{
	static char dec_obj[1000], dec_mthd[1000];
	struct obj *obj = subchans[pstate->subchan];
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;
	int i;

	struct mthd2addr *tmp = addresses;
	while (tmp->high)
	{
		if (tmp->high == mthd)
		{
			anb_set_high(tmp->buf, data);
			return 1;
		}

		if (tmp->low == mthd)
		{
			decode_method_raw(mthd, 0, obj, dec_obj, dec_mthd, NULL);
			strcat(dec_obj, ".");
			strcat(dec_obj, dec_mthd);

			anb_set_low(tmp->buf, data, dec_obj);
			return 1;
		}

		if (tmp->length)
		{
			for (i = 1; i < tmp->length; ++i)
			{
				if (tmp->high + i * tmp->stride == mthd)
				{
					anb_set_high(&tmp->buf[i], data);
					return 1;
				}
				if (tmp->low + i * tmp->stride == mthd)
				{
					decode_method_raw(mthd, 0, obj, dec_obj, dec_mthd, NULL);
					strcat(dec_obj, ".");
					strcat(dec_obj, dec_mthd);

					anb_set_low(&tmp->buf[i], data, dec_obj);
					return 1;
				}
			}
		}
		tmp++;
	}
	return 0;
}

static int check_addresses_verbose(struct pushbuf_decode_state *pstate, struct mthd2addr *addresses)
{
	int mthd = pstate->mthd;
	struct mthd2addr *tmp = addresses;
	while (tmp->high)
	{
		if (tmp->low == mthd)
		{
			mmt_debug("buffer found: %d\n", tmp->buf->buffer ? 1 : 0);
			return 1;
		}
		tmp++;
	}
	return 0;
}

static struct
{
	struct addr_n_buf offset_in;
	struct addr_n_buf offset_out;
} nv50_m2mf;

static struct mthd2addr nv50_m2mf_addresses[] =
{
	{ 0x0238, 0x030c, &nv50_m2mf.offset_in },
	{ 0x023c, 0x0310, &nv50_m2mf.offset_out },
	{ 0, 0, NULL }
};

static void decode_nv50_m2mf_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, nv50_m2mf_addresses))
	{ }
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

static struct mthd2addr nv50_3d_addresses[] =
{
	{ 0x0200, 0x0204, &nv50_3d.rt[0], 8, 32 },
	{ 0x0904, 0x0908, &nv50_3d.vertex_array_start[0], 16, 16 },
	{ 0x1080, 0x1084, &nv50_3d.vertex_array_limit[0], 16, 8 },

	{ 0x0f70, 0x0f74, &nv50_3d.gp },
	{ 0x0f7c, 0x0f80, &nv50_3d.vp },
	{ 0x0f84, 0x0f88, &nv50_3d.vertex_runout },
	{ 0x0fa4, 0x0fa8, &nv50_3d.fp },
	{ 0x0fe0, 0x0fe4, &nv50_3d.zeta },
	{ 0x155c, 0x1560, &nv50_3d.tsc },
	{ 0x1574, 0x1578, &nv50_3d.tic },
	{ 0x1280, 0x1284, &nv50_3d.cb_def },
	{ 0x1b00, 0x1b04, &nv50_3d.query },
	{ 0, 0, NULL }
};

static void decode_nv50_3d_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, nv50_3d_addresses))
	{ }
}

static void decode_nv50_3d_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, nv50_3d_addresses))
	{ }
	else if (mthd == 0x140c) // VP_START_ID
		nv50_3d_disassemble(nv50_3d.vp.buffer, "vp", data);
	else if (mthd == 0x1414) // FP_START_ID
		nv50_3d_disassemble(nv50_3d.fp.buffer, "fp", data);
	else if (mthd == 0x1410) // GP_START_ID
		nv50_3d_disassemble(nv50_3d.gp.buffer, "gp", data);
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

static struct mthd2addr nv50_2d_addresses[] =
{
	{ 0x0220, 0x0224, &nv50_2d.dst },
	{ 0x0250, 0x0254, &nv50_2d.src },
	{ 0, 0, NULL }
};

static void decode_nv50_2d_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, nv50_2d_addresses))
	{
		if (pstate->mthd == 0x0224) // DST_ADDRESS_LOW
		{
			nv50_2d.check_dst_buffer = nv50_2d.dst.buffer != NULL;

			if (nv50_2d.dst.buffer)
				nv50_2d.data_offset = nv50_2d.dst.address - nv50_2d.dst.buffer->gpu_start;
		}
	}
}

static void decode_nv50_2d_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, nv50_2d_addresses))
	{ }
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

static struct mthd2addr nvc0_3d_addresses[] =
{
	{ 0x0010, 0x0014, &nvc0_3d.subchan.semaphore },
	{ 0x0104, 0x0108, &nvc0_3d.graph.notify },

	{ 0x0790, 0x0794, &nvc0_3d.temp },
	{ 0x07e8, 0x07ec, &nvc0_3d.zcull },
	{ 0x07f0, 0x07f4, &nvc0_3d.zcull_limit },
	{ 0x0f84, 0x0f88, &nvc0_3d.vertex_runout },
	{ 0x0fe0, 0x0fe4, &nvc0_3d.zeta },
	{ 0x155c, 0x1560, &nvc0_3d.tsc },
	{ 0x1574, 0x1578, &nvc0_3d.tic },
	{ 0x1608, 0x160c, &nvc0_3d.code },
	{ 0x17bc, 0x17c0, &nvc0_3d.vertex_quarantine },
	{ 0x17c8, 0x17cc, &nvc0_3d.index_array_start },
	{ 0x17d0, 0x17d4, &nvc0_3d.index_array_limit },
	{ 0x1b00, 0x1b04, &nvc0_3d.query },
	{ 0x2384, 0x2388, &nvc0_3d.cb },

	{ 0x0800, 0x0804, &nvc0_3d.rt[0], 8, 64 },
	{ 0x1c04, 0x1c08, &nvc0_3d.vertex_array_start[0], 32, 16 },
	{ 0x1f00, 0x1f04, &nvc0_3d.vertex_array_limit[0], 32, 8 },
	{ 0x2700, 0x2704, &nvc0_3d.image[0], 8, 0x20 },

	{ 0, 0, NULL }
};

static void decode_nvc0_3d_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, nvc0_3d_addresses))
	{ }
}

static void decode_nvc0_3d_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, nvc0_3d_addresses))
	{ }
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

static struct mthd2addr nvc0_2d_addresses[] =
{
	{ 0x0104, 0x0108, &nvc0_2d.graph.notify },
	{ 0x0250, 0x0254, &nvc0_2d.src },
	{ 0x0220, 0x0224, &nvc0_2d.dst },
	{ 0, 0, NULL }
};

static void decode_nvc0_2d_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, nvc0_2d_addresses))
	{ }
}

static struct
{
	struct addr_n_buf offset_in;
	struct addr_n_buf offset_out;
	struct addr_n_buf query;
	int data_offset;
} nvc0_m2mf;

static struct mthd2addr nvc0_m2mf_addresses[] =
{
	{ 0x0238, 0x023c, &nvc0_m2mf.offset_out },
	{ 0x030c, 0x0310, &nvc0_m2mf.offset_in },
	{ 0x032c, 0x0330, &nvc0_m2mf.query },
	{ 0, 0, NULL }
};

static void decode_nvc0_m2mf_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, nvc0_m2mf_addresses))
	{
		if (pstate->mthd == 0x023c)
			if (nvc0_m2mf.offset_out.buffer)
				nvc0_m2mf.data_offset = nvc0_m2mf.offset_out.address - nvc0_m2mf.offset_out.buffer->gpu_start;
	}
}

static void decode_nvc0_m2mf_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, nvc0_m2mf_addresses))
	{ }
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

static struct mthd2addr nve0_p2mf_addresses[] =
{
	{ 0x0188, 0x018c, &nve0_p2mf.upload_dst },
	{ 0, 0, NULL }
};

static void decode_nve0_p2mf_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, nve0_p2mf_addresses))
	{
		if (pstate->mthd == 0x018c) // UPLOAD.DST_ADDRESS_LOW
			if (nve0_p2mf.upload_dst.buffer)
				nve0_p2mf.data_offset = nve0_p2mf.upload_dst.address - nve0_p2mf.upload_dst.buffer->gpu_start;
	}
}

static void decode_nve0_p2mf_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, nve0_p2mf_addresses))
	{ }
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

static struct mthd2addr nve0_copy_addresses[] =
{
	{ 0x0400, 0x0404, &nve0_copy.src },
	{ 0x0408, 0x040c, &nve0_copy.dst },
	{ 0x0240, 0x0244, &nve0_copy.query },
	{ 0, 0, NULL }
};

static void decode_nve0_copy_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, nve0_copy_addresses))
	{ }
}

static struct
{
	struct addr_n_buf query;
	struct addr_n_buf tsc;
	struct addr_n_buf tic;
	struct addr_n_buf code;
	struct addr_n_buf upload_dst;
	int data_offset;
	struct addr_n_buf launch_desc;
} nvf0_compute;

static struct mthd2addr nvf0_compute_addresses[] =
{
	{ 0x1b00, 0x1b04, &nvf0_compute.query },
	{ 0x155c, 0x1560, &nvf0_compute.tsc },
	{ 0x1574, 0x1578, &nvf0_compute.tic },
	{ 0x1608, 0x160c, &nvf0_compute.code },
	{ 0x0188, 0x018c, &nvf0_compute.upload_dst },
	{ 0, 0, NULL }
};

static void decode_nvf0_compute_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, nvf0_compute_addresses))
	{
		if (pstate->mthd == 0x018c) // UPLOAD.DST_ADDRESS_LOW
			if (nvf0_compute.upload_dst.buffer)
				nvf0_compute.data_offset = nvf0_compute.upload_dst.address - nvf0_compute.upload_dst.buffer->gpu_start;
	}
}

static void decode_nvf0_compute_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, nvf0_compute_addresses))
	{ }
	else if (mthd == 0x01b0) // UPLOAD.EXEC
	{
		int flags_ok = (data & 0x1) == 0x1 ? 1 : 0;
		mmt_debug("exec: 0x%08x linear: %d\n", data, flags_ok);

		if (!flags_ok || nvf0_compute.upload_dst.buffer == NULL)
		{
			nvf0_compute.upload_dst.address = 0;
			nvf0_compute.upload_dst.buffer = NULL;
		}
	}
	else if (mthd == 0x01b4) // UPLOAD.DATA
	{
		mmt_debug("data: 0x%08x\n", data);
		if (nvf0_compute.upload_dst.buffer)
		{
			buffer_register_write(nvf0_compute.upload_dst.buffer, nvf0_compute.data_offset, 4, &data);
			nvf0_compute.data_offset += 4;
		}
	}
	else if (mthd == 0x02b4) // LAUNCH_DESC_ADDRESS
	{
		nvf0_compute.launch_desc.address = ((uint64_t)data) << 8;
		nvf0_compute.launch_desc.buffer = find_buffer_by_gpu_address(nvf0_compute.launch_desc.address);
	}
	else if (mthd == 0x02bc) // LAUNCH
	{
		struct buffer *buf = nvf0_compute.launch_desc.buffer;

		if (buf)
		{
			if (!isa_nvc0)
				isa_nvc0 = ed_getisa("nvc0");

			struct varinfo *var = varinfo_new(isa_nvc0->vardata);
			varinfo_set_variant(var, "nve4");

			struct region *reg;
			for (reg = buf->written_regions; reg != NULL; reg = reg->next)
			{
				if (reg->start != 0)
					continue;

				envydis(isa_nvc0, stdout, buf->data + reg->start + 20 * 4, 0, // is 20 * 4 correct?
						reg->end - reg->start - 20 * 4, var, 0, NULL, 0, colors);
				varinfo_del(var);
			}
		}
	}
}

static void anb_set_high(struct addr_n_buf *s, uint32_t data)
{
	s->address = ((uint64_t)data) << 32;
	s->prev_buffer = s->buffer;
	s->buffer = NULL;
}

static struct buffer *anb_set_low(struct addr_n_buf *s, uint32_t data, const char *usage)
{
	s->address |= data;
	struct buffer *buf = s->buffer = find_buffer_by_gpu_address(s->address);
	fprintf(stdout, " [0x%lx]", s->address);

	if (s->prev_buffer)
	{
		struct buffer *pbuf = s->prev_buffer;
		if (usage && is_buffer_valid(pbuf))
		{
			int i;
			for (i = 0; i < MAX_USAGES; ++i)
				if (pbuf->usage[i].desc && strcmp(pbuf->usage[i].desc, usage) == 0)
				{
					free(pbuf->usage[i].desc);
					pbuf->usage[i].desc = NULL;
					pbuf->usage[i].address = 0;
					break;
				}

		}

		s->prev_buffer = NULL;
	}

	if (buf)
	{
		int i;
		if (usage)
		{
			int found = 0;
			for (i = 0; i < MAX_USAGES; ++i)
				if (buf->usage[i].desc && strcmp(buf->usage[i].desc, usage) == 0)
				{
					found = 1;
					buf->usage[i].address = s->address;
					break;
				}

			if (!found)
				for (i = 0; i < MAX_USAGES; ++i)
					if (!buf->usage[i].desc)
					{
						buf->usage[i].desc = strdup(usage);
						buf->usage[i].address = s->address;
						break;
					}
		}

		if (s->address != buf->gpu_start)
			fprintf(stdout, " [0x%lx+0x%lx]", buf->gpu_start, s->address - buf->gpu_start);

		for (i = 0; i < MAX_USAGES; ++i)
			if (buf->usage[i].desc && s->address >= buf->usage[i].address && strcmp(buf->usage[i].desc, usage) != 0)
				fprintf(stdout, " [%s+0x%lx]", buf->usage[i].desc, s->address - buf->usage[i].address);
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
		{ 0xa1c0, decode_nvf0_compute_terse, decode_nvf0_compute_verbose },
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
