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

static struct
{
	uint64_t dst_address;
	uint32_t dst_linear;
	struct buffer *dst_buffer;
	int find_dst_buffer;
	int data_offset;
} nv50_2d = { 0, 0, NULL, 0, 0 };

static void decode_nv50_2d(struct pushbuf_decode_state *pstate, int mthd, uint32_t data)
{
	if (mthd == 0x0220) // DST_ADDRESS_HIGH
	{
		nv50_2d.dst_address = ((uint64_t)data) << 32;
		nv50_2d.dst_buffer = NULL;
		nv50_2d.find_dst_buffer = 1;
	}
	else if (mthd == 0x0224) // DST_ADDRESS_LOW
		nv50_2d.dst_address |= data;
	else if (mthd == 0x0204) // DST_LINEAR
		nv50_2d.dst_linear = data;
	else if (mthd == 0x0860) // SIFC_DATA
	{
		if (nv50_2d.dst_buffer == NULL && nv50_2d.find_dst_buffer)
		{
			nv50_2d.find_dst_buffer = 0;

			if (nv50_2d.dst_linear)
			{
				nv50_2d.dst_buffer = find_buffer_by_gpu_address(nv50_2d.dst_address);

				if (nv50_2d.dst_buffer)
				{
					mmt_debug("buffer found: 0x%08lx\n", nv50_2d.dst_buffer->gpu_start);
					nv50_2d.data_offset = nv50_2d.dst_address - nv50_2d.dst_buffer->gpu_start;
				}
			}
		}

		if (nv50_2d.dst_buffer != NULL)
		{
			mmt_debug("2d sifc_data: 0x%08x\n", data);
			buffer_register_write(nv50_2d.dst_buffer, nv50_2d.data_offset, 4, &data);
			nv50_2d.data_offset += 4;
		}
	}
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

static struct
{
	uint64_t vp_address;
	struct buffer *vp_buffer;

	uint64_t fp_address;
	struct buffer *fp_buffer;

	uint64_t gp_address;
	struct buffer *gp_buffer;

	uint64_t tsc_address;
	struct buffer *tsc_buffer;

	uint64_t tic_address;
	struct buffer *tic_buffer;
} nv50_3d = { 0, NULL, 0, NULL, 0, NULL };

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
			mmt_debug("CODE: %s\n", "");
			for (x = reg->start; x < reg->end; x += 4)
				mmt_debug("0x%08x ", *(uint32_t *)(buf->data + x));
			mmt_debug("%s\n", "");
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

static void decode_nv50_3d(struct pushbuf_decode_state *pstate, int mthd, uint32_t data)
{
	if (mthd == 0x0f7c) // VP_ADDRESS_HIGH
		nv50_3d.vp_address = ((uint64_t)data) << 32;
	else if (mthd == 0x0f80) // VP_ADDRESS_LOW
	{
		nv50_3d.vp_address |= data;
		nv50_3d.vp_buffer = find_buffer_by_gpu_address(nv50_3d.vp_address);
		mmt_debug("vp address: 0x%08lx, buffer found: %d\n", nv50_3d.vp_address, nv50_3d.vp_buffer ? 1 : 0);
	}
	else if (mthd == 0x0fa4) // FP_ADDRESS_HIGH
		nv50_3d.fp_address = ((uint64_t)data) << 32;
	else if (mthd == 0x0fa8) // FP_ADDRESS_LOW
	{
		nv50_3d.fp_address |= data;
		nv50_3d.fp_buffer = find_buffer_by_gpu_address(nv50_3d.fp_address);
		mmt_debug("fp address: 0x%08lx, buffer found: %d\n", nv50_3d.fp_address, nv50_3d.fp_buffer ? 1 : 0);
	}
	else if (mthd == 0x0f70) // GP_ADDRESS_HIGH
		nv50_3d.gp_address = ((uint64_t)data) << 32;
	else if (mthd == 0x0f74) // GP_ADDRESS_LOW
	{
		nv50_3d.gp_address |= data;
		nv50_3d.gp_buffer = find_buffer_by_gpu_address(nv50_3d.gp_address);
		mmt_debug("gp address: 0x%08lx, buffer found: %d\n", nv50_3d.gp_address, nv50_3d.gp_buffer ? 1 : 0);
	}
	else if (mthd == 0x140c) // VP_START_ID
		nv50_3d_disassemble(nv50_3d.vp_buffer, "vp", data);
	else if (mthd == 0x1414) // FP_START_ID
		nv50_3d_disassemble(nv50_3d.fp_buffer, "fp", data);
	else if (mthd == 0x1410) // GP_START_ID
		nv50_3d_disassemble(nv50_3d.gp_buffer, "gp", data);
	else if (mthd == 0x155c) // TSC_ADDRESS_HIGH
		nv50_3d.tsc_address = ((uint64_t)data) << 32;
	else if (mthd == 0x1560) // TSC_ADDRESS_LOW
	{
		nv50_3d.tsc_address |= data;
		nv50_3d.tsc_buffer = find_buffer_by_gpu_address(nv50_3d.tsc_address);
		mmt_debug("tsc address: 0x%08lx, buffer found: %d\n", nv50_3d.tsc_address, nv50_3d.tsc_buffer ? 1 : 0);
	}
	else if (mthd == 0x1574) // TIC_ADDRESS_HIGH
		nv50_3d.tic_address = ((uint64_t)data) << 32;
	else if (mthd == 0x1578) // TIC_ADDRESS_LOW
	{
		nv50_3d.tic_address |= data;
		nv50_3d.tic_buffer = find_buffer_by_gpu_address(nv50_3d.tic_address);
		mmt_debug("tic address: 0x%08lx, buffer found: %d\n", nv50_3d.tic_address, nv50_3d.tic_buffer ? 1 : 0);
	}
	else if (mthd >= 0x1444 && mthd < 0x1448 + 0x8 * 3)
	{
		int i;
		for (i = 0; i < 3; ++i)
		{
			if (nv50_3d.tsc_buffer && mthd == 0x1444 + i * 0x8) // BIND_TSC[i]
			{
				int j, tsc = (data >> 12) & 0xff;
				mmt_debug("bind tsc[%d]: 0x%08x\n", i, tsc);
				uint32_t *tsc_data = (uint32_t *)&nv50_3d.tsc_buffer->data[8 * tsc];

				for (j = 0; j < 8; ++j)
					decode_tsc(tsc, j, tsc_data);

				break;
			}
			if (nv50_3d.tic_buffer && mthd == 0x1448 + i * 0x8) // BIND_TIC[i]
			{
				int j, tic = (data >> 9) & 0x1ffff;
				mmt_debug("bind tic[%d]: 0x%08x\n", i, tic);
				uint32_t *tic_data = (uint32_t *)&nv50_3d.tic_buffer->data[8 * tic];

				for (j = 0; j < 8; ++j)
					decode_tic(tic, j, tic_data);

				break;
			}
		}
	}
}

static const struct disisa *isa_nvc0 = NULL;
static const struct disisa *isa_macro = NULL;

static struct
{
	uint64_t code_address;
	struct buffer *code_buffer;

	struct buffer *macro_buffer;
	int last_macro_code_pos;
	int cur_macro_code_pos;

	uint64_t tsc_address;
	struct buffer *tsc_buffer;

	uint64_t tic_address;
	struct buffer *tic_buffer;

	struct rnndomain *shaders[6];
} nvc0_3d = { 0, NULL };

static int decode_nvc0_3d_tsc_tic(struct pushbuf_decode_state *pstate, int mthd, uint32_t data)
{
	if (mthd == 0x155c) // TSC_ADDRESS_HIGH
		nvc0_3d.tsc_address = ((uint64_t)data) << 32;
	else if (mthd == 0x1560) // TSC_ADDRESS_LOW
	{
		nvc0_3d.tsc_address |= data;
		nvc0_3d.tsc_buffer = find_buffer_by_gpu_address(nvc0_3d.tsc_address);
		mmt_debug("tsc address: 0x%08lx, buffer found: %d\n", nvc0_3d.tsc_address, nvc0_3d.tsc_buffer ? 1 : 0);
	}
	else if (mthd == 0x1574) // TIC_ADDRESS_HIGH
		nvc0_3d.tic_address = ((uint64_t)data) << 32;
	else if (mthd == 0x1578) // TIC_ADDRESS_LOW
	{
		nvc0_3d.tic_address |= data;
		nvc0_3d.tic_buffer = find_buffer_by_gpu_address(nvc0_3d.tic_address);
		mmt_debug("tic address: 0x%08lx, buffer found: %d\n", nvc0_3d.tic_address, nvc0_3d.tic_buffer ? 1 : 0);
	}
	else if (mthd >= 0x2400 && mthd < 0x2404 + 0x20 * 5)
	{
		int i;
		for (i = 0; i < 5; ++i)
		{
			if (nvc0_3d.tsc_buffer && mthd == 0x2400 + i * 0x20) // BIND_TSC[i]
			{
				int j, tsc = (data >> 12) & 0xfff;
				mmt_debug("bind tsc[%d]: 0x%08x\n", i, tsc);
				uint32_t *tsc_data = (uint32_t *)&nvc0_3d.tsc_buffer->data[32 * tsc];

				for (j = 0; j < 8; ++j)
					decode_tsc(tsc, j, tsc_data);

				break;
			}
			if (nvc0_3d.tic_buffer && mthd == 0x2404 + i * 0x20) // BIND_TIC[i]
			{
				int j, tic = (data >> 9) & 0x1ffff;
				mmt_debug("bind tic[%d]: 0x%08x\n", i, tic);
				uint32_t *tic_data = (uint32_t *)&nvc0_3d.tic_buffer->data[32 * tic];

				for (j = 0; j < 8; ++j)
					decode_tic(tic, j, tic_data);

				break;
			}
		}
	}
	else
		return 0;
	return 1;
}

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

static void decode_nvc0_3d(struct pushbuf_decode_state *pstate, int mthd, uint32_t data)
{
	if (mthd == 0x1608) // CODE_ADDRESS_HIGH
		nvc0_3d.code_address = ((uint64_t)data) << 32;
	else if (mthd == 0x160c) // CODE_ADDRESS_LOW
	{
		nvc0_3d.code_address |= data;
		nvc0_3d.code_buffer = find_buffer_by_gpu_address(nvc0_3d.code_address);
		mmt_debug("code address: 0x%08lx, buffer found: %d\n", nvc0_3d.code_address, nvc0_3d.code_buffer ? 1 : 0);
	}
	else if (nvc0_3d.code_buffer && mthd >= 0x2000 && mthd < 0x2000 + 0x40 * 6) // SP
	{
		int i;
		for (i = 0; i < 6; ++i)
		{
			if (mthd == 0x2000 + i * 0x40) // SP[i].SELECT
			{
				int program = (data >> 4) & 0x7;
				if (program == 0 || program == 1) // VP
					nvc0_3d.shaders[i] = nvc0_vp_header_domain;
				else if (program == 4) // GP
					nvc0_3d.shaders[i] = nvc0_gp_header_domain;
				else if (program == 5) // FP
					nvc0_3d.shaders[i] = nvc0_fp_header_domain;
				else // TCP/TEP
					nvc0_3d.shaders[i] = NULL;
				break;
			}

			if (mthd != 0x2004 + i * 0x40) // SP[i].START_ID
				continue;

			mmt_debug("start id[%d]: 0x%08x\n", i, data);
			struct region *reg;
			for (reg = nvc0_3d.code_buffer->written_regions; reg != NULL; reg = reg->next)
			{
				if (reg->start != data)
					continue;

				uint32_t x;
				fprintf(stdout, "HEADER:\n");
				if (nvc0_3d.shaders[i])
					for (x = 0; x < 20; ++x)
						decode_nvc0_p_header(x, (uint32_t *)(nvc0_3d.code_buffer->data + reg->start), nvc0_3d.shaders[i]);
				else
					for (x = reg->start; x < reg->start + 20 * 4; x += 4)
						fprintf(stdout, "0x%08x\n", *(uint32_t *)(nvc0_3d.code_buffer->data + x));

				fprintf(stdout, "CODE:\n");
				if (MMT_DEBUG)
				{
					uint32_t x;
					for (x = reg->start + 20 * 4; x < reg->end; x += 4)
						mmt_debug("0x%08x ", *(uint32_t *)(nvc0_3d.code_buffer->data + x));
					mmt_debug("%s\n", "");
				}

				if (!isa_nvc0)
					isa_nvc0 = ed_getisa("nvc0");
				struct varinfo *var = varinfo_new(isa_nvc0->vardata);

				if (chipset >= 0xe4)
					varinfo_set_variant(var, "nve4");

				envydis(isa_nvc0, stdout, nvc0_3d.code_buffer->data + reg->start + 20 * 4, 0,
						reg->end - reg->start - 20 * 4, var, 0, NULL, 0, colors);
				varinfo_del(var);
				break;
			}
			break;
		}
	}
	else if (mthd == 0x0114) // GRAPH.MACRO_CODE_POS
	{
		struct buffer *buf = nvc0_3d.macro_buffer;
		if (buf == NULL)
		{
			nvc0_3d.macro_buffer = buf = calloc(1, sizeof(struct buffer));
			buf->id = -1;
			buf->length = 0x2000;
			buf->data = calloc(buf->length, 1);
		}
		nvc0_3d.last_macro_code_pos = data * 4;
		nvc0_3d.cur_macro_code_pos = data * 4;
	}
	else if (mthd == 0x0118) // GRAPH.MACRO_CODE_DATA
	{
		struct buffer *buf = nvc0_3d.macro_buffer;
		if (nvc0_3d.cur_macro_code_pos >= buf->length)
			mmt_log("not enough space for more macro code, truncating%s\n", "");
		else
		{
			buffer_register_write(buf, nvc0_3d.cur_macro_code_pos, 4, &data);
			nvc0_3d.cur_macro_code_pos += 4;
			if (pstate->size == 0)
			{
				if (!isa_macro)
					isa_macro = ed_getisa("macro");
				struct varinfo *var = varinfo_new(isa_macro->vardata);

				envydis(isa_macro, stdout, buf->data + nvc0_3d.last_macro_code_pos, 0,
						(nvc0_3d.cur_macro_code_pos - nvc0_3d.last_macro_code_pos) / 4,
						var, 0, NULL, 0, colors);
				varinfo_del(var);
			}
		}
	}
	else if (chipset < 0xe0 && decode_nvc0_3d_tsc_tic(pstate, mthd, data))
	{ }
}

static struct
{
	uint64_t offset_out;
	struct buffer *offset_out_buffer;
	int data_offset;
} nvc0_m2mf = { 0, NULL, 0 };

static void decode_nvc0_m2mf(struct pushbuf_decode_state *pstate, int mthd, uint32_t data)
{
	if (mthd == 0x0238) // OFFSET_OUT_HIGH
	{
		nvc0_m2mf.offset_out = ((uint64_t)data) << 32;
		nvc0_m2mf.offset_out_buffer = NULL;
	}
	else if (mthd == 0x023c) // OFFSET_OUT_LOW
		nvc0_m2mf.offset_out |= data;
	else if (mthd == 0x0300) // EXEC
	{
		int flags_ok = (data & 0x111) == 0x111 ? 1 : 0;
		mmt_debug("m2mf exec: 0x%08x push&linear: %d\n", data, flags_ok);
		if (flags_ok)
			nvc0_m2mf.offset_out_buffer = find_buffer_by_gpu_address(nvc0_m2mf.offset_out);

		if (!flags_ok || nvc0_m2mf.offset_out_buffer == NULL)
		{
			nvc0_m2mf.offset_out = 0;
			nvc0_m2mf.offset_out_buffer = NULL;
		}
		if (nvc0_m2mf.offset_out_buffer)
			nvc0_m2mf.data_offset = nvc0_m2mf.offset_out - nvc0_m2mf.offset_out_buffer->gpu_start;
	}
	else if (mthd == 0x0304) // DATA
	{
		mmt_debug("m2mf data: 0x%08x\n", data);
		if (nvc0_m2mf.offset_out_buffer)
		{
			buffer_register_write(nvc0_m2mf.offset_out_buffer, nvc0_m2mf.data_offset, 4, &data);
			nvc0_m2mf.data_offset += 4;
		}
	}
}

static struct
{
	uint64_t offset_out;
	struct buffer *offset_out_buffer;
	int data_offset;
} nve0_p2mf = { 0, NULL, 0 };

static void decode_nve0_p2mf(struct pushbuf_decode_state *pstate, int mthd, uint32_t data)
{
	if (mthd == 0x0188) // UPLOAD.DST_ADDRESS_HIGH
	{
		nve0_p2mf.offset_out = ((uint64_t)data) << 32;
		nve0_p2mf.offset_out_buffer = NULL;
	}
	else if (mthd == 0x018c) // UPLOAD.DST_ADDRESS_LOW
		nve0_p2mf.offset_out |= data;
	else if (mthd == 0x01b0) // UPLOAD.EXEC
	{
		int flags_ok = (data & 0x1) == 0x1 ? 1 : 0;
		mmt_debug("p2mf exec: 0x%08x linear: %d\n", data, flags_ok);
		if (flags_ok)
			nve0_p2mf.offset_out_buffer = find_buffer_by_gpu_address(nve0_p2mf.offset_out);

		if (!flags_ok || nve0_p2mf.offset_out_buffer == NULL)
		{
			nve0_p2mf.offset_out = 0;
			nve0_p2mf.offset_out_buffer = NULL;
		}
		if (nve0_p2mf.offset_out_buffer)
			nve0_p2mf.data_offset = nve0_p2mf.offset_out - nve0_p2mf.offset_out_buffer->gpu_start;
	}
	else if (mthd == 0x01b4) // UPLOAD.DATA
	{
		mmt_debug("p2mf data: 0x%08x\n", data);
		if (nve0_p2mf.offset_out_buffer)
		{
			buffer_register_write(nve0_p2mf.offset_out_buffer, nve0_p2mf.data_offset, 4, &data);
			nve0_p2mf.data_offset += 4;
		}
	}
}

static const struct gpu_object
{
	uint32_t class_;
	void (*fun)(struct pushbuf_decode_state *, int, uint32_t);
}
objs[] =
{
		{ 0x502d, decode_nv50_2d },
		{ 0x5097, decode_nv50_3d },
		{ 0x8297, decode_nv50_3d },
		{ 0x8397, decode_nv50_3d },
		{ 0x8597, decode_nv50_3d },
		{ 0x8697, decode_nv50_3d },
		{ 0x9039, decode_nvc0_m2mf },
		{ 0x9097, decode_nvc0_3d },
		{ 0x9197, decode_nvc0_3d },
		{ 0x9297, decode_nvc0_3d },
		{ 0xa040, decode_nve0_p2mf },
		{ 0xa097, decode_nvc0_3d },
		{ 0, NULL}
};

void *demmt_get_decoder(uint32_t class_)
{
	const struct gpu_object *o;
	for (o = objs; o->class_ != 0; o++)
		if (o->class_ == class_)
			return o->fun;
	return NULL;
}
