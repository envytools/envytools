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
#include "object_state.h"
#include "log.h"
#include "macro.h"
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
	struct rnndecaddrinfo *ai = rnndec_decodeaddr(g80_texture_ctx, tsc_domain, idx * 4, 1);

	char *dec_addr = ai->name;
	char *dec_val = rnndec_decodeval(g80_texture_ctx, ai->typeinfo, data[idx], ai->width);

	fprintf(stdout, "TSC[%d]: 0x%08x   %s = %s\n", tsc, data[idx], dec_addr, dec_val);

	free(ai);
	free(dec_val);
	free(dec_addr);
}

static void decode_tic(uint32_t tic, int idx, uint32_t *data)
{
	struct rnndecaddrinfo *ai = rnndec_decodeaddr(g80_texture_ctx, tic_domain, idx * 4, 1);

	char *dec_addr = ai->name;
	char *dec_val = rnndec_decodeval(g80_texture_ctx, ai->typeinfo, data[idx], ai->width);

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

struct subchan
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
} g80_m2mf;

static struct mthd2addr g80_m2mf_addresses[] =
{
	{ 0x0238, 0x030c, &g80_m2mf.offset_in },
	{ 0x023c, 0x0310, &g80_m2mf.offset_out },
	{ 0, 0, NULL }
};

static void decode_g80_m2mf_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, g80_m2mf_addresses))
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
} g80_3d;

static const struct disisa *isa_g80 = NULL;

static void g80_3d_disassemble(struct buffer *buf, const char *mode, uint32_t start_id)
{
	if (!buf)
		return;

	mmt_debug("%s_start id 0x%08x\n", mode, start_id);
	struct region *reg;
	for (reg = buf->written_regions.head; reg != NULL; reg = reg->next)
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

		envydis(isa_g80, stdout, buf->data + reg->start, 0,
				reg->end - reg->start, var, 0, NULL, 0, colors);
		varinfo_del(var);
		break;
	}
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

static void decode_g80_3d_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, g80_3d_addresses))
	{ }
}

static void decode_g80_3d_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, g80_3d_addresses))
	{ }
	else if (mthd == 0x140c && dump_vp) // VP_START_ID
		g80_3d_disassemble(g80_3d.vp.buffer, "vp", data);
	else if (mthd == 0x1414 && dump_fp) // FP_START_ID
		g80_3d_disassemble(g80_3d.fp.buffer, "fp", data);
	else if (mthd == 0x1410 && dump_gp) // GP_START_ID
		g80_3d_disassemble(g80_3d.gp.buffer, "gp", data);
	else if (mthd >= 0x1444 && mthd < 0x1448 + 0x8 * 3)
	{
		int i;
		for (i = 0; i < 3; ++i)
		{
			if (dump_tsc && g80_3d.tsc.buffer && mthd == 0x1444 + i * 0x8) // BIND_TSC[i]
			{
				int j, tsc = (data >> 12) & 0xff;
				mmt_debug("bind tsc[%d]: 0x%08x\n", i, tsc);
				uint32_t *tsc_data = (uint32_t *)&g80_3d.tsc.buffer->data[8 * tsc];

				for (j = 0; j < 8; ++j)
					decode_tsc(tsc, j, tsc_data);

				break;
			}
			if (dump_tic && g80_3d.tic.buffer && mthd == 0x1448 + i * 0x8) // BIND_TIC[i]
			{
				int j, tic = (data >> 9) & 0x1ffff;
				mmt_debug("bind tic[%d]: 0x%08x\n", i, tic);
				uint32_t *tic_data = (uint32_t *)&g80_3d.tic.buffer->data[8 * tic];

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
} g80_2d;

static struct mthd2addr g80_2d_addresses[] =
{
	{ 0x0220, 0x0224, &g80_2d.dst },
	{ 0x0250, 0x0254, &g80_2d.src },
	{ 0, 0, NULL }
};

static void decode_g80_2d_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, g80_2d_addresses))
	{
		if (pstate->mthd == 0x0224) // DST_ADDRESS_LOW
			g80_2d.check_dst_buffer = g80_2d.dst.buffer != NULL;
	}
}

static void decode_g80_2d_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, g80_2d_addresses))
	{ }
	else if (mthd == 0x0204) // DST_LINEAR
		g80_2d.dst_linear = data;
	else if (mthd == 0x0838) // SIFC_WIDTH
	{
		if (g80_2d.dst.buffer)
			g80_2d.data_offset = g80_2d.dst.address - g80_2d.dst.buffer->gpu_start;
	}
	else if (mthd == 0x0860) // SIFC_DATA
	{
		if (g80_2d.check_dst_buffer)
		{
			g80_2d.check_dst_buffer = 0;

			if (!g80_2d.dst_linear)
				g80_2d.dst.buffer = NULL;
		}

		if (g80_2d.dst.buffer != NULL)
		{
			mmt_debug("2d sifc_data: 0x%08x\n", data);
			buffer_register_write(g80_2d.dst.buffer, g80_2d.data_offset, 4, &data);
			g80_2d.data_offset += 4;
		}
	}
}

struct nv1_graph
{
	struct addr_n_buf notify;
};

static const struct disisa *isa_gf100 = NULL;
static const struct disisa *isa_gk110 = NULL;
static const struct disisa *isa_gm107 = NULL;

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

static void decode_gf100_3d_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, gf100_3d_addresses))
	{ }
}

static void decode_gf100_3d_verbose(struct pushbuf_decode_state *pstate)
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

static struct
{
	struct nv1_graph graph;
	struct addr_n_buf src;
	struct addr_n_buf dst;
} gf100_2d;

static struct mthd2addr gf100_2d_addresses[] =
{
	{ 0x0104, 0x0108, &gf100_2d.graph.notify },
	{ 0x0250, 0x0254, &gf100_2d.src },
	{ 0x0220, 0x0224, &gf100_2d.dst },
	{ 0, 0, NULL }
};

static void decode_gf100_2d_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, gf100_2d_addresses))
	{ }
}

static struct
{
	struct addr_n_buf offset_in;
	struct addr_n_buf offset_out;
	struct addr_n_buf query;
	int data_offset;
} gf100_m2mf;

static struct mthd2addr gf100_m2mf_addresses[] =
{
	{ 0x0238, 0x023c, &gf100_m2mf.offset_out },
	{ 0x030c, 0x0310, &gf100_m2mf.offset_in },
	{ 0x032c, 0x0330, &gf100_m2mf.query },
	{ 0, 0, NULL }
};

static void decode_gf100_m2mf_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, gf100_m2mf_addresses))
	{
		if (pstate->mthd == 0x023c)
			if (gf100_m2mf.offset_out.buffer)
				gf100_m2mf.data_offset = gf100_m2mf.offset_out.address - gf100_m2mf.offset_out.buffer->gpu_start;
	}
}

static void decode_gf100_m2mf_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, gf100_m2mf_addresses))
	{ }
	else if (mthd == 0x0300) // EXEC
	{
		int flags_ok = (data & 0x111) == 0x111 ? 1 : 0;
		mmt_debug("m2mf exec: 0x%08x push&linear: %d\n", data, flags_ok);

		if (!flags_ok || gf100_m2mf.offset_out.buffer == NULL)
		{
			gf100_m2mf.offset_out.address = 0;
			gf100_m2mf.offset_out.buffer = NULL;
		}
	}
	else if (mthd == 0x0304) // DATA
	{
		mmt_debug("m2mf data: 0x%08x\n", data);
		if (gf100_m2mf.offset_out.buffer)
		{
			buffer_register_write(gf100_m2mf.offset_out.buffer, gf100_m2mf.data_offset, 4, &data);
			gf100_m2mf.data_offset += 4;
		}
	}
}

static struct
{
	struct addr_n_buf upload_dst;
	int data_offset;
} gk104_p2mf;

static struct mthd2addr gk104_p2mf_addresses[] =
{
	{ 0x0188, 0x018c, &gk104_p2mf.upload_dst },
	{ 0, 0, NULL }
};

static void decode_gk104_p2mf_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, gk104_p2mf_addresses))
	{
		if (pstate->mthd == 0x018c) // UPLOAD.DST_ADDRESS_LOW
			if (gk104_p2mf.upload_dst.buffer)
				gk104_p2mf.data_offset = gk104_p2mf.upload_dst.address - gk104_p2mf.upload_dst.buffer->gpu_start;
	}
}

static void decode_gk104_p2mf_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, gk104_p2mf_addresses))
	{ }
	else if (mthd == 0x01b0) // UPLOAD.EXEC
	{
		int flags_ok = (data & 0x1) == 0x1 ? 1 : 0;
		mmt_debug("p2mf exec: 0x%08x linear: %d\n", data, flags_ok);

		if (!flags_ok || gk104_p2mf.upload_dst.buffer == NULL)
		{
			gk104_p2mf.upload_dst.address = 0;
			gk104_p2mf.upload_dst.buffer = NULL;
		}
	}
	else if (mthd == 0x01b4) // UPLOAD.DATA
	{
		mmt_debug("p2mf data: 0x%08x\n", data);
		if (gk104_p2mf.upload_dst.buffer)
		{
			buffer_register_write(gk104_p2mf.upload_dst.buffer, gk104_p2mf.data_offset, 4, &data);
			gk104_p2mf.data_offset += 4;
		}
	}
}

static struct
{
	struct addr_n_buf src;
	struct addr_n_buf dst;
	struct addr_n_buf query;
} gk104_copy;

static struct mthd2addr gk104_copy_addresses[] =
{
	{ 0x0400, 0x0404, &gk104_copy.src },
	{ 0x0408, 0x040c, &gk104_copy.dst },
	{ 0x0240, 0x0244, &gk104_copy.query },
	{ 0, 0, NULL }
};

static void decode_gk104_copy_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, gk104_copy_addresses))
	{ }
}

static struct
{
	struct addr_n_buf temp;
	struct addr_n_buf query;
	struct addr_n_buf tsc;
	struct addr_n_buf tic;
	struct addr_n_buf code;
	struct addr_n_buf upload_dst;
	int data_offset;
	struct addr_n_buf launch_desc;
} gk104_compute;

static struct mthd2addr gk104_compute_addresses[] =
{
	{ 0x0790, 0x0794, &gk104_compute.temp },
	{ 0x1b00, 0x1b04, &gk104_compute.query },
	{ 0x155c, 0x1560, &gk104_compute.tsc },
	{ 0x1574, 0x1578, &gk104_compute.tic },
	{ 0x1608, 0x160c, &gk104_compute.code },
	{ 0x0188, 0x018c, &gk104_compute.upload_dst },
	{ 0, 0, NULL }
};

static void decode_gk104_compute_terse(struct pushbuf_decode_state *pstate)
{
	if (check_addresses_terse(pstate, gk104_compute_addresses))
	{
		if (pstate->mthd == 0x018c) // UPLOAD.DST_ADDRESS_LOW
			if (gk104_compute.upload_dst.buffer)
				gk104_compute.data_offset = gk104_compute.upload_dst.address - gk104_compute.upload_dst.buffer->gpu_start;
	}
}

static void decode_gk104_compute_verbose(struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (check_addresses_verbose(pstate, gk104_compute_addresses))
	{ }
	else if (mthd == 0x01b0) // UPLOAD.EXEC
	{
		int flags_ok = (data & 0x1) == 0x1 ? 1 : 0;
		mmt_debug("exec: 0x%08x linear: %d\n", data, flags_ok);

		if (!flags_ok || gk104_compute.upload_dst.buffer == NULL)
		{
			gk104_compute.upload_dst.address = 0;
			gk104_compute.upload_dst.buffer = NULL;
		}
	}
	else if (mthd == 0x01b4) // UPLOAD.DATA
	{
		mmt_debug("data: 0x%08x\n", data);
		if (gk104_compute.upload_dst.buffer)
		{
			buffer_register_write(gk104_compute.upload_dst.buffer, gk104_compute.data_offset, 4, &data);
			gk104_compute.data_offset += 4;
		}
	}
	else if (mthd == 0x02b4) // LAUNCH_DESC_ADDRESS
	{
		gk104_compute.launch_desc.address = ((uint64_t)data) << 8;
		gk104_compute.launch_desc.buffer = find_buffer_by_gpu_address(gk104_compute.launch_desc.address);
	}
	else if (mthd == 0x02bc) // LAUNCH
	{
		struct buffer *buf = gk104_compute.code.buffer;

		if (buf && dump_cp)
		{
			const struct disisa *isa;
			if (chipset >= 0xf0)
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
			}

			struct varinfo *var = varinfo_new(isa->vardata);

			if (chipset < 0xf0)
				varinfo_set_variant(var, "gk104");

			struct region *reg;
			for (reg = buf->written_regions.head; reg != NULL; reg = reg->next)
			{
				if (reg->start != ((uint32_t *)gk104_compute.launch_desc.buffer->data)[0x8] +
						gk104_compute.code.address - buf->gpu_start)
					continue;

				envydis(isa, stdout, buf->data + reg->start, 0,
						reg->end - reg->start, var, 0, NULL, 0, colors);
				break;
			}

			varinfo_del(var);
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
	if (decode_pb)
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

	if (buf && s->address != buf->gpu_start && decode_pb)
		fprintf(stdout, " [0x%lx+0x%lx]", buf->gpu_start, s->address - buf->gpu_start);

	if (buf && dump_buffer_usage)
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

		for (i = 0; i < MAX_USAGES; ++i)
			if (decode_pb && buf->usage[i].desc && s->address >= buf->usage[i].address && strcmp(buf->usage[i].desc, usage) != 0)
				fprintf(stdout, " [%s+0x%lx]", buf->usage[i].desc, s->address - buf->usage[i].address);
	}

	return buf;
}

struct gpu_object_decoder obj_decoders[] =
{
		{ 0x502d, decode_g80_2d_terse,   decode_g80_2d_verbose },
		{ 0x5039, decode_g80_m2mf_terse, NULL },
		{ 0x5097, decode_g80_3d_terse,   decode_g80_3d_verbose },
		{ 0x8297, decode_g80_3d_terse,   decode_g80_3d_verbose },
		{ 0x8397, decode_g80_3d_terse,   decode_g80_3d_verbose },
		{ 0x8597, decode_g80_3d_terse,   decode_g80_3d_verbose },
		{ 0x8697, decode_g80_3d_terse,   decode_g80_3d_verbose },
		{ 0x902d, decode_gf100_2d_terse,   NULL },
		{ 0x9039, decode_gf100_m2mf_terse, decode_gf100_m2mf_verbose },
		{ 0x9097, decode_gf100_3d_terse,   decode_gf100_3d_verbose },
		{ 0x9197, decode_gf100_3d_terse,   decode_gf100_3d_verbose },
		{ 0x9297, decode_gf100_3d_terse,   decode_gf100_3d_verbose },
		{ 0xa040, decode_gk104_p2mf_terse, decode_gk104_p2mf_verbose },
		{ 0xa140, decode_gk104_p2mf_terse, decode_gk104_p2mf_verbose },
		{ 0xa097, decode_gf100_3d_terse,   decode_gf100_3d_verbose },
		{ 0xb097, decode_gf100_3d_terse,   decode_gf100_3d_verbose },
		{ 0xa0b5, decode_gk104_copy_terse, NULL },
		{ 0xb0b5, decode_gk104_copy_terse, NULL },
		{ 0xa0c0, decode_gk104_compute_terse, decode_gk104_compute_verbose },
		{ 0xa1c0, decode_gk104_compute_terse, decode_gk104_compute_verbose },
		{ 0, NULL, NULL }
};

const struct gpu_object_decoder *demmt_get_decoder(uint32_t class_)
{
	const struct gpu_object_decoder *o;
	for (o = obj_decoders; o->class_ != 0; o++)
		if (o->class_ == class_)
		{
			if (o->disabled)
				return NULL;
			return o;
		}
	return NULL;
}
