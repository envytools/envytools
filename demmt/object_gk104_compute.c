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
#include "nvrm.h"
#include "object.h"

struct gk104_compute_data
{
	struct addr_n_buf temp;
	struct addr_n_buf query;
	struct addr_n_buf tsc;
	struct addr_n_buf tic;
	struct addr_n_buf code;
	struct gk104_upload upload;
	int data_offset;
	struct addr_n_buf launch_desc;

	struct mthd2addr *addresses;
};

static void destroy_gk104_compute_data(struct gpu_object *obj)
{
	struct gk104_compute_data *d = obj->class_data;
	free(d->addresses);
	free(d);
}

void decode_gk104_compute_init(struct gpu_object *obj)
{
	struct gk104_compute_data *d = obj->class_data = calloc(1, sizeof(struct gk104_compute_data));
	obj->class_data_destroy = destroy_gk104_compute_data;

#define SZ 8
	struct mthd2addr *tmp = d->addresses = calloc(SZ, sizeof(*d->addresses));
	m2a_set1(tmp++, 0x0790, 0x0794, &d->temp);
	m2a_set1(tmp++, 0x1b00, 0x1b04, &d->query );
	m2a_set1(tmp++, 0x155c, 0x1560, &d->tsc);
	m2a_set1(tmp++, 0x1574, 0x1578, &d->tic);
	m2a_set1(tmp++, 0x1608, 0x160c, &d->code);
	m2a_set1(tmp++, 0x0188, 0x018c, &d->upload.dst);
	m2a_set1(tmp++, 0x01dc, 0x01e0, &d->upload.query);
	m2a_set1(tmp++, 0, 0, NULL);
	assert(tmp - d->addresses == SZ);
#undef SZ
}

void decode_gk104_compute_terse(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	struct gk104_compute_data *objdata = obj->class_data;

	if (check_addresses_terse(pstate, objdata->addresses))
	{
		if (pstate->mthd == 0x018c) // UPLOAD.DST_ADDRESS_LOW
		{
			if (objdata->upload.dst.gpu_mapping)
				objdata->data_offset = 0;
		}
	}
}

void decode_gk104_compute_verbose(struct gpu_object *obj, struct pushbuf_decode_state *pstate)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;
	struct gk104_compute_data *objdata = obj->class_data;

	if (check_addresses_verbose(pstate, objdata->addresses))
	{ }
	else if (mthd == 0x01b0) // UPLOAD.EXEC
	{
		int flags_ok = (data & 0x1) == 0x1 ? 1 : 0;
		mmt_debug("exec: 0x%08x linear: %d\n", data, flags_ok);

		if (!flags_ok || objdata->upload.dst.gpu_mapping == NULL)
		{
			objdata->upload.dst.address = 0;
			objdata->upload.dst.gpu_mapping = NULL;
		}
	}
	else if (mthd == 0x01b4) // UPLOAD.DATA
	{
		mmt_debug("data: 0x%08x\n", data);
		if (objdata->upload.dst.gpu_mapping)
		{
			gpu_mapping_register_write(objdata->upload.dst.gpu_mapping,
					objdata->upload.dst.address + objdata->data_offset, 4, &data);
			objdata->data_offset += 4;
		}
	}
	else if (mthd == 0x02b4) // LAUNCH_DESC_ADDRESS
	{
		objdata->launch_desc.address = ((uint64_t)data) << 8;
		objdata->launch_desc.gpu_mapping = gpu_mapping_find(objdata->launch_desc.address, nvrm_get_device(pstate->fifo));
	}
	else if (mthd == 0x02bc) // LAUNCH
	{
		if (dump_cp)
		{
			const struct disisa *isa;
			struct varinfo *var = NULL;
			int chipset = nvrm_get_chipset(pstate->fifo);
			if (chipset >= 0x117)
			{
				if (!isa_gm107)
					isa_gm107 = ed_getisa("gm107");
				isa = isa_gm107;
			}
			else if (chipset >= 0xf0 || chipset == 0xea)
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

				varinfo_set_variant(var, "gk104");
			}

			struct region *reg;

			uint8_t *code = NULL;
			uint64_t code_addr = objdata->code.address;
			uint32_t *header = gpu_mapping_get_data(objdata->launch_desc.gpu_mapping, objdata->launch_desc.address, 0x100);
			uint32_t start_id = header[8];
			struct gpu_mapping *m = objdata->code.gpu_mapping;
			if (!m) {
				// blob has been observed to feed an
				// unmapped address as the base of the
				// code page, and then use larger
				// start id's.
				m = gpu_mapping_find(code_addr + start_id, nvrm_get_device(pstate->fifo));
				code_addr = m->address;
				start_id -= m->address - objdata->code.address;
			}
			if (m)
			{
				code = gpu_mapping_get_data(m, code_addr, 0);

				if (code_addr != m->address)
					code = NULL;// FIXME
			}

			mmt_printf("HEADER:%s\n", "");
			int x;
			for (x = 0; x < 0x40; ++x)
				decode_gf100_p_header(x, header, gk104_cp_header_domain);

			mmt_printf("CODE:%s\n", "");
			if (code)
				for (reg = m->object->written_regions.head; reg != NULL; reg = reg->next)
				{
					if (reg->start != start_id + code_addr - m->address)
						continue;

					envydis(isa, stdout, code + reg->start, 0,
							reg->end - reg->start, var, 0, NULL, 0, colors);
					break;
				}

			if (var)
				varinfo_del(var);
		}
	}
}
