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

static void envy_bios_parse_D_unk0(struct envy_bios *);
static void envy_bios_parse_D_unk2(struct envy_bios *);

struct D_known_tables {
	uint8_t offset;
	uint16_t *ptr;
	const char *name;
};

static int
parse_at(struct envy_bios *bios, unsigned int idx, const char **name)
{
	struct envy_bios_D *d = &bios->D;
	struct D_known_tables tbls[] = {
		{ 0x0, &d->unk0.offset, "UNK0" },
		{ 0x2, &d->unk2.offset, "UNK2 TABLE" },
	};
	int entries_count = (sizeof(tbls) / sizeof(struct D_known_tables));

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
envy_bios_parse_bit_D(struct envy_bios *bios, struct envy_bios_bit_entry *bit)
{
	struct envy_bios_D *d = &bios->D;
	unsigned int idx = 0;

	d->bit = bit;

	while (!parse_at(bios, idx, NULL))
		idx++;

	/* parse tables */
	envy_bios_parse_D_unk0(bios);
	envy_bios_parse_D_unk2(bios);

	return 0;
}

void
envy_bios_print_bit_D(struct envy_bios *bios, FILE *out, unsigned mask)
{
	struct envy_bios_D *d = &bios->D;
	uint16_t addr;
	int ret = 0, i = 0;

	if (!d->bit || !(mask & ENVY_BIOS_PRINT_D))
		return;

	fprintf(out, "BIT table 'D' at 0x%x, version %i\n",
		d->bit->offset, d->bit->version);

	for (i = 0; i * 2 < d->bit->t_len; ++i) {
		ret = bios_u16(bios, d->bit->t_offset + (i * 2), &addr);
		if (!ret && addr) {
			const char *name;
			ret = parse_at(bios, i, &name);
			fprintf(out, "0x%02x: 0x%x => D %s\n", i * 2, addr, name);
		}
	}

	fprintf(out, "\n");
}

static void
envy_bios_parse_D_unk0(struct envy_bios *bios)
{
	struct envy_bios_D_unk0 *unk0 = &bios->D.unk0;
	int i;

	if (!unk0->offset)
		return;

	unk0->version = 0;
	unk0->hlen = 0;
	unk0->rlen = 18;
	unk0->entriesnum = 11;
	unk0->valid = 1;

	unk0->entries = malloc(unk0->entriesnum * sizeof(struct envy_bios_D_unk0_entry));
	for (i = 0; i < unk0->entriesnum; ++i) {
		struct envy_bios_D_unk0_entry *e = &unk0->entries[i];
		e->offset = unk0->offset + unk0->hlen + i * unk0->rlen;
	}
}

void
envy_bios_print_D_unk0(struct envy_bios *bios, FILE *out, unsigned mask)
{
	struct envy_bios_D_unk0 *unk0 = &bios->D.unk0;
	int i;

	if (!unk0->offset || !(mask & ENVY_BIOS_PRINT_D))
		return;
	if (!unk0->valid) {
		ENVY_BIOS_ERR("Failed to parse D UNK0 table at 0x%x, version %x\n\n", unk0->offset, unk0->version);
		return;
	}

	fprintf(out, "D UNK0 at 0x%x, version %x\n", unk0->offset, unk0->version);
	envy_bios_dump_hex(bios, out, unk0->offset, unk0->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk0->entriesnum; ++i) {
		struct envy_bios_D_unk0_entry *e = &unk0->entries[i];
		envy_bios_dump_hex(bios, out, e->offset, unk0->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

static void
envy_bios_parse_D_unk2(struct envy_bios *bios)
{
	struct envy_bios_D_unk2 *unk2 = &bios->D.unk2;
	int err = 0, i;

	if (!unk2->offset)
		return;

	bios_u8(bios, unk2->offset, &unk2->version);
	switch (unk2->version) {
	case 0x20:
		err |= bios_u8(bios, unk2->offset + 0x1, &unk2->hlen);
		err |= bios_u8(bios, unk2->offset + 0x2, &unk2->rlen);
		err |= bios_u8(bios, unk2->offset + 0x3, &unk2->entriesnum);
		unk2->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK2 table version 0x%x\n", unk2->version);
		return;
	}

	unk2->entries = malloc(unk2->entriesnum * sizeof(struct envy_bios_D_unk2_entry));
	for (i = 0; i < unk2->entriesnum; ++i) {
		struct envy_bios_D_unk2_entry *e = &unk2->entries[i];
		e->offset = unk2->offset + unk2->hlen + i * unk2->rlen;
	}
}

void
envy_bios_print_D_unk2(struct envy_bios *bios, FILE *out, unsigned mask)
{
	struct envy_bios_D_unk2 *unk2 = &bios->D.unk2;
	int i;

	if (!unk2->offset || !(mask & ENVY_BIOS_PRINT_D))
		return;
	if (!unk2->valid) {
		ENVY_BIOS_ERR("Failed to parse D UNK2 table at 0x%x, version %x\n\n", unk2->offset, unk2->version);
		return;
	}

	fprintf(out, "D UNK2 table at 0x%x, version %x\n", unk2->offset, unk2->version);
	envy_bios_dump_hex(bios, out, unk2->offset, unk2->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk2->entriesnum; ++i) {
		struct envy_bios_D_unk2_entry *e = &unk2->entries[i];
		envy_bios_dump_hex(bios, out, e->offset, unk2->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}
