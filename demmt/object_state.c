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

#include <stdlib.h>
#include "config.h"
#include "demmt.h"
#include "object.h"
#include "object_state.h"

const struct disisa *isa_gf100 = NULL;
const struct disisa *isa_gk110 = NULL;
const struct disisa *isa_gm107 = NULL;

struct buffer *find_buffer_by_gpu_address(uint64_t addr)
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

void decode_tsc(uint32_t tsc, int idx, uint32_t *data)
{
	struct rnndecaddrinfo *ai = rnndec_decodeaddr(g80_texture_ctx, tsc_domain, idx * 4, 1);

	char *dec_addr = ai->name;
	char *dec_val = rnndec_decodeval(g80_texture_ctx, ai->typeinfo, data[idx], ai->width);

	fprintf(stdout, "TSC[%d]: 0x%08x   %s = %s\n", tsc, data[idx], dec_addr, dec_val);

	free(ai);
	free(dec_val);
	free(dec_addr);
}

void decode_tic(uint32_t tic, int idx, uint32_t *data)
{
	struct rnndecaddrinfo *ai = rnndec_decodeaddr(g80_texture_ctx, tic_domain, idx * 4, 1);

	char *dec_addr = ai->name;
	char *dec_val = rnndec_decodeval(g80_texture_ctx, ai->typeinfo, data[idx], ai->width);

	fprintf(stdout, "TIC[%d]: 0x%08x   %s = %s\n", tic, data[idx], dec_addr, dec_val);

	free(ai);
	free(dec_val);
	free(dec_addr);
}

static void anb_set_high(struct addr_n_buf *s, uint32_t data);
static struct buffer *anb_set_low(struct addr_n_buf *s, uint32_t data, const char *usage);

int check_addresses_terse(struct pushbuf_decode_state *pstate, struct mthd2addr *addresses)
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

int check_addresses_verbose(struct pushbuf_decode_state *pstate, struct mthd2addr *addresses)
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
