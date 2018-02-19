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

//

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
		/* { 0x00, &p->unk0.offset, "UNKNOWN" }, */
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
