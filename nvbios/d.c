/*
 * Copyright (C) 2016 Karol Herbst
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

static void envy_bios_parse_d_dp_info(struct envy_bios *);

struct d_known_tables {
	uint8_t offset;
	uint16_t *ptr;
	const char *name;
};

static int
parse_at(struct envy_bios *bios, unsigned int idx, const char **name)
{
	struct envy_bios_d *d = &bios->d;
	struct d_known_tables tbls[] = {
		{ 0x0, &d->dp_info.offset, "DP INFO" },
	};
	int entries_count = (sizeof(tbls) / sizeof(struct d_known_tables));

	/* check the index */
	if (idx >= entries_count)
		return -ENOENT;

	/* check the table has the right size */
	if (tbls[idx].offset + 2 > d->bit->t_len)
		return -ENOENT;

	if (name)
		*name = tbls[idx].name;

	return bios_u16(bios, d->bit->t_offset + tbls[idx].offset, tbls[idx].ptr);
}

int
envy_bios_parse_bit_d(struct envy_bios *bios, struct envy_bios_bit_entry *bit)
{
	struct envy_bios_d *d = &bios->d;
	unsigned int idx = 0;

	d->bit = bit;

	while (!parse_at(bios, idx, NULL))
		idx++;

	/* parse tables */
	envy_bios_parse_d_dp_info(bios);

	return 0;
}

void
envy_bios_print_bit_d(struct envy_bios *bios, FILE *out, unsigned mask)
{
	struct envy_bios_d *d = &bios->d;
	uint16_t addr;
	int ret = 0, i = 0;

	if (!d->bit || !(mask & ENVY_BIOS_PRINT_d))
		return;

	fprintf(out, "BIT table 'd' at 0x%x, version %i\n",
		d->bit->offset, d->bit->version);

	for (i = 0; i * 2 < d->bit->t_len; ++i) {
		ret = bios_u16(bios, d->bit->t_offset + (i * 2), &addr);
		if (!ret && addr) {
			const char *name;
			ret = parse_at(bios, i, &name);
			fprintf(out, "0x%02x: 0x%x => d %s\n", i * 2, addr, name);
		}
	}

	fprintf(out, "\n");
}

static void
envy_bios_parse_d_dp_info(struct envy_bios *bios)
{
	struct envy_bios_d_dp_info *dp_info = &bios->d.dp_info;
	int err = 0, i;

	if (!dp_info->offset)
		return;

	bios_u8(bios, dp_info->offset, &dp_info->version);
	switch (dp_info->version) {
	case 0x40:
		err |= bios_u8(bios, dp_info->offset + 0x1, &dp_info->hlen);
		err |= bios_u8(bios, dp_info->offset + 0x2, &dp_info->rlen);
		err |= bios_u8(bios, dp_info->offset + 0x3, &dp_info->entriesnum);
		dp_info->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown d DP INFO table version 0x%x\n", dp_info->version);
		return;
	}

	dp_info->entries = malloc(dp_info->entriesnum * sizeof(struct envy_bios_d_dp_info_entry));
	for (i = 0; i < dp_info->entriesnum; ++i) {
		struct envy_bios_d_dp_info_entry *e = &dp_info->entries[i];
		e->offset = dp_info->offset + dp_info->hlen + i * dp_info->rlen;
	}
}

void
envy_bios_print_d_dp_info(struct envy_bios *bios, FILE *out, unsigned mask)
{
	struct envy_bios_d_dp_info *dp_info = &bios->d.dp_info;
	int i;

	if (!dp_info->offset || !(mask & ENVY_BIOS_PRINT_d))
		return;
	if (!dp_info->valid) {
		ENVY_BIOS_ERR("Failed to parse d DP INFO table at 0x%x, version %x\n\n", dp_info->offset, dp_info->version);
		return;
	}

	fprintf(out, "d DP INFO table at 0x%x, version %x\n", dp_info->offset, dp_info->version);
	envy_bios_dump_hex(bios, out, dp_info->offset, dp_info->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < dp_info->entriesnum; ++i) {
		struct envy_bios_d_dp_info_entry *e = &dp_info->entries[i];
		envy_bios_dump_hex(bios, out, e->offset, dp_info->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}
