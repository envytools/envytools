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

#include <inttypes.h>
#include <stdlib.h>
#include "config.h"
#include "demmt.h"
#include "buffer.h"
#include "nvrm.h"
#include "object.h"
#include "object_state.h"

const struct disisa *isa_macro = NULL;
const struct disisa *isa_g80 = NULL;
const struct disisa *isa_gf100 = NULL;
const struct disisa *isa_gk110 = NULL;
const struct disisa *isa_gm107 = NULL;

void demmt_cleanup_isas()
{
	if (isa_macro)
		ed_freeisa(isa_macro);
	if (isa_g80)
		ed_freeisa(isa_g80);
	if (isa_gf100)
		ed_freeisa(isa_gf100);
	if (isa_gk110)
		ed_freeisa(isa_gk110);
	if (isa_gm107)
		ed_freeisa(isa_gm107);
}

static int is_mapping_valid(struct gpu_mapping *m)
{
	struct gpu_object *obj;
	struct gpu_mapping *gpu_mapping;
	for (obj = gpu_objects; obj != NULL; obj = obj->next)
		for (gpu_mapping = obj->gpu_mappings; gpu_mapping != NULL; gpu_mapping = gpu_mapping->next)
			if (m == gpu_mapping)
				return 1;
	return 0;
}

struct rnndeccontext *create_g80_texture_ctx(struct gpu_object *obj)
{
	struct rnndeccontext *texture_ctx = rnndec_newcontext(rnndb_g80_texture);
	texture_ctx->colors = colors;

	struct rnnvalue *v = NULL;
	struct rnnenum *chs = rnn_findenum(rnndb, "chipset");
	FINDARRAY(chs->vals, v, v->value == (uint64_t)nvrm_get_chipset(obj));
	rnndec_varadd(texture_ctx, "chipset", v ? v->name : "NV1");
	rnndec_varadd(texture_ctx, "gk20a_extended_components", "NO");
	rnndec_varadd(texture_ctx, "gm200_tic_header_version", "ONE_D_BUFFER");

	return texture_ctx;
}

void decode_tsc(struct rnndeccontext *texture_ctx, uint32_t tsc, uint32_t *data)
{
	int idx;

	for (idx = 0; idx < 8; idx++)
	{
		struct rnndecaddrinfo *ai = rnndec_decodeaddr(texture_ctx, tsc_domain, idx * 4, 1);
		char *dec_val = rnndec_decodeval(texture_ctx, ai->typeinfo, data[idx], ai->width);

		mmt_printf("TSC[%d]: 0x%08x   %s = %s\n", tsc, data[idx], ai->name, dec_val);

		rnndec_free_decaddrinfo(ai);
		free(dec_val);
	}
}

void decode_tic(struct rnndeccontext *texture_ctx, struct rnndomain *domain,
		uint32_t tic, uint32_t *data)
{
	int idx;

	if (domain == tic_domain)
		rnndec_varmod(texture_ctx, "gk20a_extended_components",
			      data[0] & 0x80000000 ? "YES" : "NO");
	if (domain == tic2_domain) {
		int version = (data[2] >> 21) & 0x7;
		struct rnnvalue *v = NULL;
		struct rnnenum *hdr = rnn_findenum(rnndb_g80_texture, "gm200_tic_header_version");
		FINDARRAY(hdr->vals, v, v->value == (uint64_t)version);
		rnndec_varmod(texture_ctx, "gm200_tic_header_version",
			      v ? v->name : "ONE_D_BUFFER");
	}

	for (idx = 0; idx < 8; idx++)
	{
		struct rnndecaddrinfo *ai = rnndec_decodeaddr(texture_ctx, domain, idx * 4, 1);
		char *dec_val = rnndec_decodeval(texture_ctx, ai->typeinfo, data[idx], ai->width);

		mmt_printf("TIC[%d]: 0x%08x   %s = %s\n", tic, data[idx], ai->name, dec_val);

		rnndec_free_decaddrinfo(ai);
		free(dec_val);
	}
}

static void anb_set_high(struct addr_n_buf *s, uint32_t data);
static void anb_set_low(struct addr_n_buf *s, uint32_t data, const char *usage,
		struct gpu_object *dev, int check_offset);

int check_addresses_terse(struct pushbuf_decode_state *pstate, struct mthd2addr *addresses)
{
	static char dec_obj[1000], dec_mthd[1000];
	struct obj *obj = current_subchan_object(pstate);
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;
	int i;
	struct gpu_object *dev = nvrm_get_device(pstate->fifo);

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

			anb_set_low(tmp->buf, data, dec_obj, dev, tmp->check_offset);
			return 1;
		}

		if (tmp->length &&
				((mthd >= tmp->high && mthd < tmp->high + tmp->length * tmp->stride)
				||
				 (mthd >= tmp->low  && mthd < tmp->low  + tmp->length * tmp->stride))
			)
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

					anb_set_low(&tmp->buf[i], data, dec_obj, dev, tmp->check_offset);
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
			mmt_debug("buffer found: %d\n", tmp->buf->gpu_mapping ? 1 : 0);
			return 1;
		}
		tmp++;
	}
	return 0;
}

static void anb_set_high(struct addr_n_buf *s, uint32_t data)
{
	s->address = ((uint64_t)data) << 32;

	s->prev_gpu_mapping = s->gpu_mapping;
	s->gpu_mapping = NULL;
}

static void anb_set_low(struct addr_n_buf *s, uint32_t data, const char *usage,
		struct gpu_object *dev, int check_offset)
{
	s->address |= data;
	if (decode_pb)
		mmt_printf(" [0x%" PRIx64 "]", s->address);

	struct gpu_mapping *mapping = s->gpu_mapping = gpu_mapping_find(s->address, dev);
	struct gpu_object *obj = NULL;
	if (mapping)
		obj = mapping->object;

	if (s->prev_gpu_mapping)
	{
		struct gpu_mapping *m = s->prev_gpu_mapping;
		if (usage && is_mapping_valid(m))
		{
			int i;
			struct gpu_object *obj2 = m->object;
			for (i = 0; i < MAX_USAGES; ++i)
				if (obj2->usage[i].desc && strcmp(obj2->usage[i].desc, usage) == 0)
				{
					free(obj2->usage[i].desc);
					obj2->usage[i].desc = NULL;
					obj2->usage[i].address = 0;
					break;
				}

		}

		s->prev_gpu_mapping = NULL;
	}

	if (mapping && decode_pb)
	{
		uint64_t obj_gpu_addr = mapping->address - mapping->object_offset;
		if (s->address != obj_gpu_addr)
			mmt_printf(" [0x%" PRIx64 "+0x%" PRIx64 "]", obj_gpu_addr, s->address - obj_gpu_addr);
		if (mapping->object_offset && s->address != mapping->address)
			mmt_printf(" [0x%" PRIx64 "+0x%" PRIx64 "]", mapping->address, s->address - mapping->address);
	}

	if (mapping && dump_buffer_usage)
	{
		int i;
		if (usage)
		{
			int found = 0;
			for (i = 0; i < MAX_USAGES; ++i)
				if (obj->usage[i].desc && strcmp(obj->usage[i].desc, usage) == 0)
				{
					found = 1;
					obj->usage[i].address = s->address;
					break;
				}

			if (!found)
				for (i = 0; i < MAX_USAGES; ++i)
					if (!obj->usage[i].desc)
					{
						obj->usage[i].desc = strdup(usage);
						obj->usage[i].address = s->address;
						break;
					}
		}

		for (i = 0; i < MAX_USAGES; ++i)
			if (decode_pb && obj->usage[i].desc && s->address >= obj->usage[i].address && strcmp(obj->usage[i].desc, usage) != 0)
				mmt_printf(" [%s+0x%" PRIx64 "]", obj->usage[i].desc, s->address - obj->usage[i].address);
	}

	if (s->address && s->address != 0xffffffffff && !mapping)
	{
		if (check_offset)
			mapping = gpu_mapping_find(s->address + check_offset, dev);

		if (!mapping)
			mmt_printf(" [address not mapped, possible driver bug]%s", "");
	}
}

struct gpu_object_decoder obj_decoders[] =
{
	{ 0x502d, decode_g80_2d_init,        decode_g80_2d_terse,        decode_g80_2d_verbose },
	{ 0x5039, decode_g80_m2mf_init,      decode_g80_m2mf_terse,      decode_g80_m2mf_verbose },
	{ 0x5097, decode_g80_3d_init,        decode_g80_3d_terse,        decode_g80_3d_verbose },
	{ 0x8297, decode_g80_3d_init,        decode_g80_3d_terse,        decode_g80_3d_verbose },
	{ 0x8397, decode_g80_3d_init,        decode_g80_3d_terse,        decode_g80_3d_verbose },
	{ 0x8597, decode_g80_3d_init,        decode_g80_3d_terse,        decode_g80_3d_verbose },
	{ 0x8697, decode_g80_3d_init,        decode_g80_3d_terse,        decode_g80_3d_verbose },
	{ 0x50c0, decode_g80_compute_init,   decode_g80_compute_terse,   decode_g80_compute_verbose },
	{ 0x85c0, decode_g80_compute_init,   decode_g80_compute_terse,   decode_g80_compute_verbose },
	{ 0x902d, decode_gf100_2d_init,      decode_gf100_2d_terse,      NULL },
	{ 0x9039, decode_gf100_m2mf_init,    decode_gf100_m2mf_terse,    decode_gf100_m2mf_verbose },
	{ 0x9097, decode_gf100_3d_init,      decode_gf100_3d_terse,      decode_gf100_3d_verbose },
	{ 0x9197, decode_gf100_3d_init,      decode_gf100_3d_terse,      decode_gf100_3d_verbose },
	{ 0x9297, decode_gf100_3d_init,      decode_gf100_3d_terse,      decode_gf100_3d_verbose },
	{ 0x90c0, decode_gf100_compute_init, decode_gf100_compute_terse, decode_gf100_compute_verbose },
	{ 0x91c0, decode_gf100_compute_init, decode_gf100_compute_terse, decode_gf100_compute_verbose },
	{ 0xa040, decode_gk104_p2mf_init,    decode_gk104_p2mf_terse,    decode_gk104_p2mf_verbose },
	{ 0xa140, decode_gk104_p2mf_init,    decode_gk104_p2mf_terse,    decode_gk104_p2mf_verbose },
	{ 0xa097, decode_gk104_3d_init,      decode_gk104_3d_terse,      decode_gk104_3d_verbose },
	{ 0xa197, decode_gk104_3d_init,      decode_gk104_3d_terse,      decode_gk104_3d_verbose },
	{ 0xa297, decode_gk104_3d_init,      decode_gk104_3d_terse,      decode_gk104_3d_verbose },
	{ 0xb097, decode_gk104_3d_init,      decode_gk104_3d_terse,      decode_gk104_3d_verbose },
	{ 0xb197, decode_gk104_3d_init,      decode_gk104_3d_terse,      decode_gk104_3d_verbose },
	{ 0xc097, decode_gk104_3d_init,      decode_gk104_3d_terse,      decode_gk104_3d_verbose },
	{ 0xa0b5, decode_gk104_copy_init,    decode_gk104_copy_terse,    NULL },
	{ 0xb0b5, decode_gk104_copy_init,    decode_gk104_copy_terse,    NULL },
	{ 0xc0b5, decode_gk104_copy_init,    decode_gk104_copy_terse,    NULL },
	{ 0xc1b5, decode_gk104_copy_init,    decode_gk104_copy_terse,    NULL },
	{ 0xa0c0, decode_gk104_compute_init, decode_gk104_compute_terse, decode_gk104_compute_verbose },
	{ 0xa1c0, decode_gk104_compute_init, decode_gk104_compute_terse, decode_gk104_compute_verbose },
	{ 0xb0c0, decode_gk104_compute_init, decode_gk104_compute_terse, decode_gk104_compute_verbose },
	{ 0xb1c0, decode_gk104_compute_init, decode_gk104_compute_terse, decode_gk104_compute_verbose },
	{ 0xc0c0, decode_gk104_compute_init, decode_gk104_compute_terse, decode_gk104_compute_verbose },
	{ 0xc1c0, decode_gk104_compute_init, decode_gk104_compute_terse, decode_gk104_compute_verbose },
	{ 0, NULL, NULL, NULL }
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
