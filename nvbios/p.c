/*
 * Copyright (C) 2018 Rhys Kidd
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

#include "bios.h"

int envy_bios_parse_p_falcon_ucode(struct envy_bios *bios);

struct p_known_tables {
	uint8_t offset;
	uint16_t *ptr;
	const char *name;
};

static int
parse_at(struct envy_bios *bios, struct envy_bios_p *p,
	     int idx, int offset, const char ** name)
{
	struct p_known_tables p1_tbls[] = {
		/* { 0x00, &p->unk0.offset, "UNKNOWN" }, */
	};
	struct p_known_tables p2_tbls[] = {
		{ 0x00, &p->falcon_ucode.offset, "FALCON UCODE" },
	};
	struct p_known_tables *tbls;
	int entries_count = 0;
	int ret;

	if (p->bit->version == 0x1) {
		tbls = p1_tbls;
		entries_count = (sizeof(p1_tbls) / sizeof(struct p_known_tables));
	} else if (p->bit->version == 0x2) {
		tbls = p2_tbls;
		entries_count = (sizeof(p2_tbls) / sizeof(struct p_known_tables));
	} else
		return -EINVAL;

	/* either we address by offset or idx */
	if (idx != -1 && offset != -1)
		return -EINVAL;

	/* lookup the index by the table's offset */
	if (offset > -1) {
		idx = 0;
		while (idx < entries_count && tbls[idx].offset != offset)
			idx++;
	}

	/* check the index */
	if (idx < 0 || idx >= entries_count)
		return -ENOENT;

	/* check the table has the right size */
	if (tbls[idx].offset + 2 > p->bit->t_len)
		return -ENOENT;

	if (name)
		*name = tbls[idx].name;

	uint32_t tblOffset;
	ret = bios_u32(bios, p->bit->t_offset + tbls[idx].offset, &tblOffset);
	*tbls[idx].ptr = tblOffset;
	return ret;
}

int
envy_bios_parse_bit_p (struct envy_bios *bios, struct envy_bios_bit_entry *bit)
{
	struct envy_bios_p *p = &bios->p;
	int idx = 0;

	p->bit = bit;
	while (!parse_at(bios, p, idx, -1, NULL))
		idx++;

	/* parse tables */
	envy_bios_parse_p_falcon_ucode(bios);

	return 0;
}

void
envy_bios_print_bit_p (struct envy_bios *bios, FILE *out, unsigned mask)
{
	struct envy_bios_p *p = &bios->p;
	const char *name;
	uint32_t addr;
	int ret = 0, i = 0;

	if (!p->bit || !(mask & ENVY_BIOS_PRINT_p))
		return;

	fprintf(out, "BIT table 'p' at 0x%x, version %i\n",
		p->bit->offset, p->bit->version);

	for (i = 0; i < p->bit->t_len; i+=4) {
		ret = bios_u32(bios, p->bit->t_offset + i, &addr);
		if (!ret && addr) {
			name = "UNKNOWN";
			ret = parse_at(bios, p, -1, i, &name);
			fprintf(out, "0x%02x: 0x%x => %s TABLE\n", i, addr, name);
		}
	}

	fprintf(out, "\n");
}

static struct enum_val falcon_ucode_application_id_types[] = {
	{ ENVY_BIOS_FALCON_UCODE_APPLICATION_ID_PRE_OS,	"PRE_OS" },
	{ ENVY_BIOS_FALCON_UCODE_APPLICATION_ID_DEVINIT,"DEVINIT" },
	{ 0 },
};

static struct enum_val falcon_ucode_target_id_types[] = {
	{ ENVY_BIOS_FALCON_UCODE_TARGET_ID_PMU,	"PMU" },
	{ 0 },
};

int
envy_bios_parse_p_falcon_ucode(struct envy_bios *bios)
{
	struct envy_bios_p_falcon_ucode *falcon_ucode = &bios->p.falcon_ucode;
	int i, err = 0;

	if (!falcon_ucode->offset)
		return -EINVAL;

	bios_u8(bios, falcon_ucode->offset + 0x0, &falcon_ucode->version);
	switch(falcon_ucode->version) {
	case 0x1:
		err |= bios_u8(bios, falcon_ucode->offset + 0x1, &falcon_ucode->hlen);
		err |= bios_u8(bios, falcon_ucode->offset + 0x2, &falcon_ucode->rlen);
		err |= bios_u8(bios, falcon_ucode->offset + 0x3, &falcon_ucode->entriesnum);

		err |= bios_u8(bios, falcon_ucode->offset + 0x4, &falcon_ucode->desc_version);
		err |= bios_u8(bios, falcon_ucode->offset + 0x5, &falcon_ucode->desc_size);
		falcon_ucode->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown FALCON UCODE table version 0x%x\n", falcon_ucode->version);
		return -EINVAL;
	};

	err = 0;
	falcon_ucode->entries = malloc(falcon_ucode->entriesnum * sizeof(struct envy_bios_p_falcon_ucode_entry));
	for (i = 0; i < falcon_ucode->entriesnum; i++) {
		uint32_t data = falcon_ucode->offset + falcon_ucode->hlen + i * falcon_ucode->rlen;

		falcon_ucode->entries[i].offset = data;
		err |= bios_u8(bios, data + 0x0, &falcon_ucode->entries[i].application_id);
		err |= bios_u8(bios, data + 0x1, &falcon_ucode->entries[i].target_id);
		err |= bios_u32(bios, data + 0x2, &falcon_ucode->entries[i].desc_ptr);
	}

	return 0;
}

void
envy_bios_print_p_falcon_ucode(struct envy_bios *bios, FILE *out, unsigned mask)
{
	struct envy_bios_p_falcon_ucode *falcon_ucode = &bios->p.falcon_ucode;
	int i;

	if (!falcon_ucode->offset || !(mask & ENVY_BIOS_PRINT_p))
		return;
	if (!falcon_ucode->valid) {
		ENVY_BIOS_ERR("Failed to parse FALCON UCODE table at 0x%x, version %x\n\n", falcon_ucode->offset, falcon_ucode->version);
		return;
	}

	fprintf(out, "FALCON UCODE table at 0x%x, version %x\n", falcon_ucode->offset, falcon_ucode->version);
	envy_bios_dump_hex(bios, out, falcon_ucode->offset, falcon_ucode->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < falcon_ucode->entriesnum; i++) {
		struct envy_bios_p_falcon_ucode_entry *e = &falcon_ucode->entries[i];
		const char *application_id = find_enum(falcon_ucode_application_id_types, e->application_id);
		const char *target_id = find_enum(falcon_ucode_target_id_types, e->target_id);
		fprintf(out, "UCODE %i: application_id 0x%02x [%s], target_id 0x%02x [%s], desc_ptr = 0x%x\n",
				i, e->application_id, application_id, e->target_id, target_id, e->desc_ptr);

		envy_bios_dump_hex(bios, out, e->offset, falcon_ucode->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}
