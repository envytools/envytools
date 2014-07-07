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

static void decode_nv50_2d(int mthd, uint32_t data)
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

static struct
{
	uint64_t vp_address;
	struct buffer *vp_buffer;
	uint64_t fp_address;
	struct buffer *fp_buffer;
	uint64_t gp_address;
	struct buffer *gp_buffer;
} nva3_3d = { 0, NULL, 0, NULL, 0, NULL };

static const struct disisa *isa_nv50 = NULL;

static void nva3_3d_disassemble(struct buffer *buf, const char *mode, uint32_t start_id)
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
		varinfo_set_variant(var, "nva3");
		varinfo_set_mode(var, mode);

		envydis(isa_nv50, stdout, buf->data + reg->start, 0,
				reg->end - reg->start, var, 0, NULL, 0, colors);
		varinfo_del(var);
		break;
	}
}

static void decode_nva3_3d(int mthd, uint32_t data)
{
	if (mthd == 0x0f7c) // VP_ADDRESS_HIGH
		nva3_3d.vp_address = ((uint64_t)data) << 32;
	else if (mthd == 0x0f80) // VP_ADDRESS_LOW
	{
		nva3_3d.vp_address |= data;
		nva3_3d.vp_buffer = find_buffer_by_gpu_address(nva3_3d.vp_address);
		mmt_debug("vp address: 0x%08lx, buffer found: %d\n", nva3_3d.vp_address, nva3_3d.vp_buffer ? 1 : 0);
	}
	else if (mthd == 0x0fa4) // FP_ADDRESS_HIGH
		nva3_3d.fp_address = ((uint64_t)data) << 32;
	else if (mthd == 0x0fa8) // FP_ADDRESS_LOW
	{
		nva3_3d.fp_address |= data;
		nva3_3d.fp_buffer = find_buffer_by_gpu_address(nva3_3d.fp_address);
		mmt_debug("fp address: 0x%08lx, buffer found: %d\n", nva3_3d.fp_address, nva3_3d.fp_buffer ? 1 : 0);
	}
	else if (mthd == 0x0f70) // GP_ADDRESS_HIGH
		nva3_3d.gp_address = ((uint64_t)data) << 32;
	else if (mthd == 0x0f74) // GP_ADDRESS_LOW
	{
		nva3_3d.gp_address |= data;
		nva3_3d.gp_buffer = find_buffer_by_gpu_address(nva3_3d.gp_address);
		mmt_debug("gp address: 0x%08lx, buffer found: %d\n", nva3_3d.gp_address, nva3_3d.gp_buffer ? 1 : 0);
	}
	else if (mthd == 0x140c) // VP_START_ID
		nva3_3d_disassemble(nva3_3d.vp_buffer, "vp", data);
	else if (mthd == 0x1414) // FP_START_ID
		nva3_3d_disassemble(nva3_3d.fp_buffer, "fp", data);
	else if (mthd == 0x1410) // GP_START_ID
		nva3_3d_disassemble(nva3_3d.gp_buffer, "gp", data);
}

static const struct disisa *isa_nvc0 = NULL;

static struct
{
	uint64_t code_address;
	struct buffer *code_buffer;
} nvc1_3d = { 0, NULL };

static void decode_nvc1_3d(int mthd, uint32_t data)
{
	if (mthd == 0x1608) // CODE_ADDRESS_HIGH
		nvc1_3d.code_address = ((uint64_t)data) << 32;
	else if (mthd == 0x160c) // CODE_ADDRESS_LOW
	{
		nvc1_3d.code_address |= data;
		nvc1_3d.code_buffer = find_buffer_by_gpu_address(nvc1_3d.code_address);
		mmt_debug("code address: 0x%08lx, buffer found: %d\n", nvc1_3d.code_address, nvc1_3d.code_buffer ? 1 : 0);
	}
	else if (nvc1_3d.code_buffer && mthd >= 0x2000 && mthd < 0x2000 + 0x40 * 6 && (mthd & 0x4) == 4) // SP
	{
		int i;
		for (i = 0; i < 6; ++i)
			if (mthd == 0x2004 + i * 0x40) // SP[i].START_ID
			{
				mmt_debug("start id[%d]: 0x%08x\n", i, data);
				struct region *reg;
				for (reg = nvc1_3d.code_buffer->written_regions; reg != NULL; reg = reg->next)
				{
					if (reg->start == data)
					{
						if (MMT_DEBUG)
						{
							uint32_t x;
							mmt_debug("CODE: %s\n", "");
							for (x = reg->start + 20 * 4; x < reg->end; x += 4)
								mmt_debug("0x%08x ", *(uint32_t *)(nvc1_3d.code_buffer->data + x));
							mmt_debug("%s\n", "");
						}

						if (!isa_nvc0)
							isa_nvc0 = ed_getisa("nvc0");
						struct varinfo *var = varinfo_new(isa_nvc0->vardata);

						envydis(isa_nvc0, stdout, nvc1_3d.code_buffer->data + reg->start + 20 * 4, 0,
								reg->end - reg->start - 20 * 4, var, 0, NULL, 0, colors);
						varinfo_del(var);
						break;
					}
				}
				break;
			}
	}
}

static struct
{
	uint64_t offset_out;
	struct buffer *offset_out_buffer;
	int data_offset;
} nvc0_m2mf = { 0, NULL, 0 };

static void decode_nvc0_m2mf(int mthd, uint32_t data)
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

#define END_OF_OBJS { 0, NULL }
struct gpu_object
{
	uint32_t class_;
	void (*fun)(int, uint32_t);
};

static const struct gpu_object no_objs[] = { END_OF_OBJS };

static const struct gpu_object *nv04_objs = no_objs;
static const struct gpu_object *nv05_objs = no_objs;
static const struct gpu_object *nv10_objs = no_objs;
static const struct gpu_object *nv15_objs = no_objs;
static const struct gpu_object *nv1a_objs = no_objs;
static const struct gpu_object *nv11_objs = no_objs;
static const struct gpu_object *nv17_objs = no_objs;
static const struct gpu_object *nv1f_objs = no_objs;
static const struct gpu_object *nv18_objs = no_objs;
static const struct gpu_object *nv20_objs = no_objs;
static const struct gpu_object *nv2a_objs = no_objs;
static const struct gpu_object *nv25_objs = no_objs;
static const struct gpu_object *nv28_objs = no_objs;
static const struct gpu_object *nv30_objs = no_objs;
static const struct gpu_object *nv35_objs = no_objs;
static const struct gpu_object *nv31_objs = no_objs;
static const struct gpu_object *nv36_objs = no_objs;
static const struct gpu_object *nv34_objs = no_objs;
static const struct gpu_object *nv40_objs = no_objs;
static const struct gpu_object *nv45_objs = no_objs;
static const struct gpu_object *nv41_objs = no_objs;
static const struct gpu_object *nv42_objs = no_objs;
static const struct gpu_object *nv43_objs = no_objs;
static const struct gpu_object *nv44_objs = no_objs;
static const struct gpu_object *nv4a_objs = no_objs;
static const struct gpu_object *nv47_objs = no_objs;
static const struct gpu_object *nv49_objs = no_objs;
static const struct gpu_object *nv4b_objs = no_objs;
static const struct gpu_object *nv46_objs = no_objs;
static const struct gpu_object *nv4e_objs = no_objs;
static const struct gpu_object *nv4c_objs = no_objs;
static const struct gpu_object *nv67_objs = no_objs;
static const struct gpu_object *nv68_objs = no_objs;
static const struct gpu_object *nv63_objs = no_objs;

static const struct gpu_object nv50_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		END_OF_OBJS
};

static const struct gpu_object nv84_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		END_OF_OBJS
};

static const struct gpu_object nv86_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		END_OF_OBJS
};

static const struct gpu_object nv92_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		END_OF_OBJS
};

static const struct gpu_object nv94_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		END_OF_OBJS
};

static const struct gpu_object nv96_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		END_OF_OBJS
};

static const struct gpu_object nv98_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		END_OF_OBJS
};

static const struct gpu_object nva0_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		END_OF_OBJS
};

static const struct gpu_object nvaa_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		END_OF_OBJS
};

static const struct gpu_object nvac_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		END_OF_OBJS
};

static const struct gpu_object nva3_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		{ 0x8597, decode_nva3_3d },
		END_OF_OBJS
};

static const struct gpu_object nva5_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		{ 0x8597, decode_nva3_3d },
		END_OF_OBJS
};

static const struct gpu_object nva8_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		{ 0x8597, decode_nva3_3d },
		END_OF_OBJS
};

static const struct gpu_object nvaf_objs[] =
{
		{ 0x502d, decode_nv50_2d },
		END_OF_OBJS
};

static const struct gpu_object nvc0_objs[] =
{
		{ 0x9039, decode_nvc0_m2mf },
		END_OF_OBJS
};

static const struct gpu_object nvc4_objs[] =
{
		{ 0x9039, decode_nvc0_m2mf },
		END_OF_OBJS
};

static const struct gpu_object nvc3_objs[] =
{
		{ 0x9039, decode_nvc0_m2mf },
		END_OF_OBJS
};

static const struct gpu_object nvce_objs[] =
{
		{ 0x9039, decode_nvc0_m2mf },
		END_OF_OBJS
};

static const struct gpu_object nvcf_objs[] =
{
		{ 0x9039, decode_nvc0_m2mf },
		END_OF_OBJS
};

static const struct gpu_object nvc1_objs[] =
{
		{ 0x9039, decode_nvc0_m2mf },
		{ 0x9197, decode_nvc1_3d },
		END_OF_OBJS
};

static const struct gpu_object nvc8_objs[] =
{
		{ 0x9039, decode_nvc0_m2mf },
		END_OF_OBJS
};

static const struct gpu_object nvd9_objs[] =
{
		{ 0x9039, decode_nvc0_m2mf },
		END_OF_OBJS
};

static const struct gpu_object nvd7_objs[] =
{
		{ 0x9039, decode_nvc0_m2mf },
		END_OF_OBJS
};

static const struct gpu_object *nve4_objs = no_objs;
static const struct gpu_object *nve7_objs = no_objs;
static const struct gpu_object *nve6_objs = no_objs;
static const struct gpu_object *nvea_objs = no_objs;
static const struct gpu_object *nvf0_objs = no_objs;
static const struct gpu_object *nvf1_objs = no_objs;
static const struct gpu_object *nv108_objs = no_objs;
static const struct gpu_object *nv117_objs = no_objs;

static const struct gpu_object *objects;

void demmt_object_init_chipset(int chipset)
{
	if (chipset == 0x04) objects = nv04_objs;
	else if (chipset == 0x05) objects = nv05_objs;
	else if (chipset == 0x10) objects = nv10_objs;
	else if (chipset == 0x15) objects = nv15_objs;
	else if (chipset == 0x1a) objects = nv1a_objs;
	else if (chipset == 0x11) objects = nv11_objs;
	else if (chipset == 0x17) objects = nv17_objs;
	else if (chipset == 0x1f) objects = nv1f_objs;
	else if (chipset == 0x18) objects = nv18_objs;
	else if (chipset == 0x20) objects = nv20_objs;
	else if (chipset == 0x2a) objects = nv2a_objs;
	else if (chipset == 0x25) objects = nv25_objs;
	else if (chipset == 0x28) objects = nv28_objs;
	else if (chipset == 0x30) objects = nv30_objs;
	else if (chipset == 0x35) objects = nv35_objs;
	else if (chipset == 0x31) objects = nv31_objs;
	else if (chipset == 0x36) objects = nv36_objs;
	else if (chipset == 0x34) objects = nv34_objs;
	else if (chipset == 0x40) objects = nv40_objs;
	else if (chipset == 0x45) objects = nv45_objs;
	else if (chipset == 0x41) objects = nv41_objs;
	else if (chipset == 0x42) objects = nv42_objs;
	else if (chipset == 0x43) objects = nv43_objs;
	else if (chipset == 0x44) objects = nv44_objs;
	else if (chipset == 0x4a) objects = nv4a_objs;
	else if (chipset == 0x47) objects = nv47_objs;
	else if (chipset == 0x49) objects = nv49_objs;
	else if (chipset == 0x4b) objects = nv4b_objs;
	else if (chipset == 0x46) objects = nv46_objs;
	else if (chipset == 0x4e) objects = nv4e_objs;
	else if (chipset == 0x4c) objects = nv4c_objs;
	else if (chipset == 0x67) objects = nv67_objs;
	else if (chipset == 0x68) objects = nv68_objs;
	else if (chipset == 0x63) objects = nv63_objs;
	else if (chipset == 0x50) objects = nv50_objs;
	else if (chipset == 0x84) objects = nv84_objs;
	else if (chipset == 0x86) objects = nv86_objs;
	else if (chipset == 0x92) objects = nv92_objs;
	else if (chipset == 0x94) objects = nv94_objs;
	else if (chipset == 0x96) objects = nv96_objs;
	else if (chipset == 0x98) objects = nv98_objs;
	else if (chipset == 0xa0) objects = nva0_objs;
	else if (chipset == 0xaa) objects = nvaa_objs;
	else if (chipset == 0xac) objects = nvac_objs;
	else if (chipset == 0xa3) objects = nva3_objs;
	else if (chipset == 0xa5) objects = nva5_objs;
	else if (chipset == 0xa8) objects = nva8_objs;
	else if (chipset == 0xaf) objects = nvaf_objs;
	else if (chipset == 0xc0) objects = nvc0_objs;
	else if (chipset == 0xc4) objects = nvc4_objs;
	else if (chipset == 0xc3) objects = nvc3_objs;
	else if (chipset == 0xce) objects = nvce_objs;
	else if (chipset == 0xcf) objects = nvcf_objs;
	else if (chipset == 0xc1) objects = nvc1_objs;
	else if (chipset == 0xc8) objects = nvc8_objs;
	else if (chipset == 0xd9) objects = nvd9_objs;
	else if (chipset == 0xd7) objects = nvd7_objs;
	else if (chipset == 0xe4) objects = nve4_objs;
	else if (chipset == 0xe7) objects = nve7_objs;
	else if (chipset == 0xe6) objects = nve6_objs;
	else if (chipset == 0xea) objects = nvea_objs;
	else if (chipset == 0xf0) objects = nvf0_objs;
	else if (chipset == 0xf1) objects = nvf1_objs;
	else if (chipset == 0x108) objects = nv108_objs;
	else if (chipset == 0x117) objects = nv117_objs;
	else
		mmt_log("object code does not know anything about chipset nv%x\n", chipset);
}

void demmt_parse_command(uint32_t class_, int mthd, uint32_t data)
{
	if (!disassemble_shaders)
		return;

	const struct gpu_object *o = objects;
	for (o = objects; o->class_ != 0; o++)
	{
		if (o->class_ == class_)
		{
			o->fun(mthd, data);
			return;
		}
	}
}
