/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "bios.h"

int envy_bios_parse_bit_empty (struct envy_bios *bios, struct envy_bios_bit_entry *bit) {
	if (bit->t_len) {
		ENVY_BIOS_ERR("BIT table '%c' not empty!\n", bit->type);
		return -EINVAL;
	}
	return 0;
}

static const struct {
	uint8_t type;
	uint8_t version;
	int (*parse) (struct envy_bios *bios, struct envy_bios_bit_entry *bit);
} bit_types[] = {
	{ 'i', 2, envy_bios_parse_bit_i },	/* Info */
	{ '2', 1, envy_bios_parse_bit_2 },	/* i2c */
	{ 'A', 1, envy_bios_parse_bit_A },	/* Analog */
	{ 'N', 0, envy_bios_parse_bit_empty },	/* never seen non-empty */
	{ 'c', 0, envy_bios_parse_bit_empty },	/* never seen non-empty */
	{ 0 },
};

int envy_bios_parse_bit (struct envy_bios *bios) {
	struct envy_bios_bit *bit = &bios->bit;
	if (!bit->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, bit->offset+7, &bit->version);
	err |= bios_u8(bios, bit->offset+8, &bit->hlen);
	err |= bios_u8(bios, bit->offset+9, &bit->rlen);
	err |= bios_u8(bios, bit->offset+10, &bit->entriesnum);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, bit->offset, bit->hlen + bit->rlen * bit->entriesnum, "BIT", -1);
	uint8_t checksum = 0;
	int i;
	for (i = 0; i < bit->hlen; i++) {
		uint8_t byte;
		err |= bios_u8(bios, bit->offset+i, &byte);
		if (err)
			return -EFAULT;
		checksum += byte;
	}
	if (checksum) {
		ENVY_BIOS_ERR("BIT table checksum mismatch\n");
		return -EINVAL;
	}
	int wanthlen = 12;
	int wantrlen = 6;
	switch (bit->version) {
		case 1:
			break;
		default:
			ENVY_BIOS_ERR("Unknown BIT table version %d\n", bit->version);
			return -EINVAL;
	}
	if (bit->hlen < wanthlen) {
		ENVY_BIOS_ERR("BIT table header too short [%d < %d]\n", bit->hlen, wanthlen);
		return -EINVAL;
	}
	if (bit->rlen < wantrlen) {
		ENVY_BIOS_ERR("BIT table record too short [%d < %d]\n", bit->rlen, wantrlen);
		return -EINVAL;
	}
	if (bit->hlen > wanthlen) {
		ENVY_BIOS_WARN("BIT table header longer than expected [%d > %d]\n", bit->hlen, wanthlen);
	}
	if (bit->rlen > wantrlen) {
		ENVY_BIOS_WARN("BIT table record longer than expected [%d > %d]\n", bit->rlen, wantrlen);
	}
	bit->entries = calloc(bit->entriesnum, sizeof *bit->entries);
	if (!bit->entries)
		return -ENOMEM;
	for (i = 0; i < bit->entriesnum; i++) {
		struct envy_bios_bit_entry *entry = &bit->entries[i];
		entry->offset = bit->offset + bit->hlen + bit->rlen * i;
		err |= bios_u8(bios, entry->offset+0, &entry->type);
		err |= bios_u8(bios, entry->offset+1, &entry->version);
		err |= bios_u16(bios, entry->offset+2, &entry->t_len);
		err |= bios_u16(bios, entry->offset+4, &entry->t_offset);
		if (err)
			return -EFAULT;
		entry->is_unk = 1;
		if (entry->type != 'b' && entry->t_len)
			envy_bios_block(bios, entry->t_offset, entry->t_len, "BIT", entry->type);
	}
	int j;
	/* iterate over BIT tables by type first - some types of tables have to be parsed before others, notably 'i'. */
	for (j = 0; bit_types[j].parse; j++) {
		for (i = 0; i < bit->entriesnum; i++) {
			struct envy_bios_bit_entry *entry = &bit->entries[i];
			if (entry->type == bit_types[j].type && entry->version == bit_types[j].version) {
				if (bit_types[j].parse(bios, entry))
					ENVY_BIOS_ERR("Failed to parse BIT table '%c' at 0x%04x version %d\n", entry->type, entry->t_offset, entry->version);
				else
					entry->is_unk = 0;
			}
		}
	}
	bit->valid = 1;
	return 0;
}

void envy_bios_print_bit (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_bit *bit = &bios->bit;
	if (!bit->offset || !(mask & ENVY_BIOS_PRINT_BMP_BIT))
		return;
	if (!bit->valid) {
		fprintf(out, "Failed to parse BIT table at 0x%04x version %d\n\n", bit->offset, bit->version);
		return;
	}
	fprintf(out, "BIT table at 0x%04x version %d", bit->offset, bit->version);
	fprintf(out, "\n");
	envy_bios_dump_hex(bios, out, bit->offset, bit->hlen, mask);
	int i;
	for (i = 0; i < bit->entriesnum; i++) {
		struct envy_bios_bit_entry *entry = &bit->entries[i];
		fprintf (out, "BIT table '%c' version %d at 0x%04x length 0x%04x\n", entry->type, entry->version, entry->t_offset, entry->t_len);
		envy_bios_dump_hex(bios, out, entry->offset, bit->rlen, mask);
	}
	fprintf(out, "\n");
	for (i = 0; i < bit->entriesnum; i++) {
		struct envy_bios_bit_entry *entry = &bit->entries[i];
		if (entry->is_unk) {
			fprintf (out, "Unknown BIT table '%c' version %d\n", entry->type, entry->version);
			envy_bios_dump_hex(bios, out, entry->t_offset, entry->t_len, mask);
			fprintf(out, "\n");
		}
	}
}
