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

int envy_bios_parse_dunk0c (struct envy_bios *bios) {
	struct envy_bios_dunk0c *dunk0c = &bios->dunk0c;
	if (!dunk0c->offset)
		return 0;
	int err = 0;
	int wanthlen = 4;
	int wantrlen = 1;
	if (bios->dcb.version >= 0x30) {
		err |= bios_u8(bios, dunk0c->offset, &dunk0c->version);
		err |= bios_u8(bios, dunk0c->offset+1, &dunk0c->hlen);
		err |= bios_u8(bios, dunk0c->offset+2, &dunk0c->entriesnum);
		err |= bios_u8(bios, dunk0c->offset+3, &dunk0c->rlen);
	} else {
		dunk0c->version = bios->dcb.version;
		wanthlen = dunk0c->hlen = 0;
		dunk0c->entriesnum = 8;
		dunk0c->rlen = 1;
	}
	if (err)
		return -EFAULT;
	envy_bios_block(bios, dunk0c->offset, dunk0c->hlen + dunk0c->rlen * dunk0c->entriesnum, "DUNK0C", -1);
	switch (dunk0c->version) {
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x30:
		case 0x40:
			break;
		default:
			ENVY_BIOS_ERR("Unknown DUNK0C table version %d.%d\n", dunk0c->version >> 4, dunk0c->version & 0xf);
			return -EINVAL;
	}
	if (dunk0c->hlen < wanthlen) {
		ENVY_BIOS_ERR("DUNK0C table header too short [%d < %d]\n", dunk0c->hlen, wanthlen);
		return -EINVAL;
	}
	if (dunk0c->rlen < wantrlen) {
		ENVY_BIOS_ERR("DUNK0C table record too short [%d < %d]\n", dunk0c->rlen, wantrlen);
		return -EINVAL;
	}
	if (dunk0c->hlen > wanthlen) {
		ENVY_BIOS_WARN("DUNK0C table header longer than expected [%d > %d]\n", dunk0c->hlen, wanthlen);
	}
	if (dunk0c->rlen > wantrlen) {
		ENVY_BIOS_WARN("DUNK0C table record longer than expected [%d > %d]\n", dunk0c->rlen, wantrlen);
	}
	dunk0c->entries = calloc(dunk0c->entriesnum, sizeof *dunk0c->entries);
	if (!dunk0c->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < dunk0c->entriesnum; i++) {
		struct envy_bios_dunk0c_entry *entry = &dunk0c->entries[i];
		entry->offset = dunk0c->offset + dunk0c->hlen + dunk0c->rlen * i;
		err |= bios_u8(bios, entry->offset, &entry->unk00);
		if (err)
			return -EFAULT;
	}
	dunk0c->valid = 1;
	return 0;
}

void envy_bios_print_dunk0c (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_dunk0c *dunk0c = &bios->dunk0c;
	if (!dunk0c->offset || !(mask & ENVY_BIOS_PRINT_DUNK))
		return;
	if (!dunk0c->valid) {
		fprintf(out, "Failed to parse DUNK0C table at 0x%04x version %d.%d\n\n", dunk0c->offset, dunk0c->version >> 4, dunk0c->version & 0xf);
		return;
	}
	fprintf(out, "DUNK0C table at 0x%04x version %d.%d\n", dunk0c->offset, dunk0c->version >> 4, dunk0c->version & 0xf);
	envy_bios_dump_hex(bios, out, dunk0c->offset, dunk0c->hlen, mask);
	int i;
	for (i = 0; i < dunk0c->entriesnum; i++) {
		struct envy_bios_dunk0c_entry *entry = &dunk0c->entries[i];
		if (entry->unk00 != 0x0f || mask & ENVY_BIOS_PRINT_UNUSED) {
			fprintf(out, "DUNK0C %d: unk00 0x%02x\n", i, entry->unk00);
		}
		envy_bios_dump_hex(bios, out, entry->offset, dunk0c->rlen, mask);
	}
	fprintf(out, "\n");
}


int envy_bios_parse_dunk0e (struct envy_bios *bios) {
	struct envy_bios_dunk0e *dunk0e = &bios->dunk0e;
	if (!dunk0e->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, dunk0e->offset, &dunk0e->version);
	err |= bios_u8(bios, dunk0e->offset+1, &dunk0e->hlen);
	int i;
	for (i = 0; i < 10; i++)
		err |= bios_u8(bios, dunk0e->offset+2+i, &dunk0e->unk02[i]);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, dunk0e->offset, dunk0e->hlen, "DUNK0E", -1);
	int wanthlen = 0;
	switch (dunk0e->version) {
		case 0x30:
		case 0x31:
		case 0x40:
			wanthlen = 12;
			break;
		default:
			ENVY_BIOS_ERR("Unknown DUNK0E table version %d.%d\n", dunk0e->version >> 4, dunk0e->version & 0xf);
			return -EINVAL;
	}
	if (dunk0e->hlen < wanthlen) {
		ENVY_BIOS_ERR("DUNK0E table header too short [%d < %d]\n", dunk0e->hlen, wanthlen);
		return -EINVAL;
	}
	if (dunk0e->hlen > wanthlen) {
		ENVY_BIOS_WARN("DUNK0E table header longer than expected [%d > %d]\n", dunk0e->hlen, wanthlen);
	}
	dunk0e->valid = 1;
	return 0;
}

void envy_bios_print_dunk0e (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_dunk0e *dunk0e = &bios->dunk0e;
	if (!dunk0e->offset || !(mask & ENVY_BIOS_PRINT_DUNK))
		return;
	if (!dunk0e->valid) {
		fprintf(out, "Failed to parse DUNK0E table at 0x%04x version %d.%d\n\n", dunk0e->offset, dunk0e->version >> 4, dunk0e->version & 0xf);
		return;
	}
	fprintf(out, "DUNK0E table at 0x%04x version %d.%d data", dunk0e->offset, dunk0e->version >> 4, dunk0e->version & 0xf);
	int i;
	for (i = 0; i < 10; i++)
		fprintf(out, " 0x%02x", dunk0e->unk02[i]);
	fprintf(out, "\n");
	envy_bios_dump_hex(bios, out, dunk0e->offset, dunk0e->hlen, mask);
	fprintf(out, "\n");
}


int envy_bios_parse_dunk10 (struct envy_bios *bios) {
	struct envy_bios_dunk10 *dunk10 = &bios->dunk10;
	if (!dunk10->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, dunk10->offset, &dunk10->version);
	err |= bios_u8(bios, dunk10->offset+1, &dunk10->hlen);
	err |= bios_u8(bios, dunk10->offset+2, &dunk10->entriesnum);
	err |= bios_u8(bios, dunk10->offset+3, &dunk10->rlen);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, dunk10->offset, dunk10->hlen + dunk10->rlen * dunk10->entriesnum, "DUNK10", -1);
	int wanthlen = 0;
	int wantrlen = 0;
	switch (dunk10->version) {
		case 0x31:
		case 0x40:
			wanthlen = 10;
			wantrlen = 1;
			err |= bios_u8(bios, dunk10->offset+4, &dunk10->unk04);
			err |= bios_u8(bios, dunk10->offset+5, &dunk10->unk05);
			err |= bios_u8(bios, dunk10->offset+6, &dunk10->unk06);
			err |= bios_u8(bios, dunk10->offset+7, &dunk10->unk07);
			err |= bios_u8(bios, dunk10->offset+8, &dunk10->unk08);
			err |= bios_u8(bios, dunk10->offset+9, &dunk10->unk09);
			if (err)
				return -EFAULT;
			break;
		case 0x41:
			wanthlen = 5;
			wantrlen = 1;
			if (dunk10->rlen >= 2)
				wantrlen = 2;
			err |= bios_u8(bios, dunk10->offset+4, &dunk10->unk04);
			if (err)
				return -EFAULT;
			break;
		default:
			ENVY_BIOS_ERR("Unknown DUNK10 table version %d.%d\n", dunk10->version >> 4, dunk10->version & 0xf);
			return -EINVAL;
	}
	if (dunk10->hlen < wanthlen) {
		ENVY_BIOS_ERR("DUNK10 table header too short [%d < %d]\n", dunk10->hlen, wanthlen);
		return -EINVAL;
	}
	if (dunk10->rlen < wantrlen) {
		ENVY_BIOS_ERR("DUNK10 table record too short [%d < %d]\n", dunk10->rlen, wantrlen);
		return -EINVAL;
	}
	if (dunk10->hlen > wanthlen) {
		ENVY_BIOS_WARN("DUNK10 table header longer than expected [%d > %d]\n", dunk10->hlen, wanthlen);
	}
	if (dunk10->rlen > wantrlen) {
		ENVY_BIOS_WARN("DUNK10 table record longer than expected [%d > %d]\n", dunk10->rlen, wantrlen);
	}
	dunk10->entries = calloc(dunk10->entriesnum, sizeof *dunk10->entries);
	if (!dunk10->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < dunk10->entriesnum; i++) {
		struct envy_bios_dunk10_entry *entry = &dunk10->entries[i];
		entry->offset = dunk10->offset + dunk10->hlen + dunk10->rlen * i;
		err |= bios_u8(bios, entry->offset+0, &entry->unk00);
		if (dunk10->rlen >= 2)
			err |= bios_u8(bios, entry->offset+1, &entry->unk01);
		if (err)
			return -EFAULT;
	}
	dunk10->valid = 1;
	return 0;
}

void envy_bios_print_dunk10 (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_dunk10 *dunk10 = &bios->dunk10;
	if (!dunk10->offset || !(mask & ENVY_BIOS_PRINT_DUNK))
		return;
	if (!dunk10->valid) {
		fprintf(out, "Failed to parse DUNK10 table at 0x%04x version %d.%d\n\n", dunk10->offset, dunk10->version >> 4, dunk10->version & 0xf);
		return;
	}
	fprintf(out, "DUNK10 table at 0x%04x version %d.%d", dunk10->offset, dunk10->version >> 4, dunk10->version & 0xf);
	if (dunk10->version < 0x41) {
		fprintf (out, " unk04 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", dunk10->unk04, dunk10->unk05, dunk10->unk06, dunk10->unk07, dunk10->unk08, dunk10->unk09);
	} else {
		fprintf (out, " unk04 0x%02x\n", dunk10->unk04);
	}
	envy_bios_dump_hex(bios, out, dunk10->offset, dunk10->hlen, mask);
	int i;
	for (i = 0; i < dunk10->entriesnum; i++) {
		struct envy_bios_dunk10_entry *entry = &dunk10->entries[i];
		envy_bios_dump_hex(bios, out, entry->offset, dunk10->rlen, mask);
		fprintf(out, "DUNK10 %d: unk00 0x%02x", i, entry->unk00);
		if (dunk10->rlen >= 2)
			fprintf(out, " unk01 0x%02x", entry->unk01);
		fprintf(out, "\n");
	}
	fprintf(out, "\n");
}


int envy_bios_parse_dunk17 (struct envy_bios *bios) {
	struct envy_bios_dunk17 *dunk17 = &bios->dunk17;
	if (!dunk17->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, dunk17->offset, &dunk17->version);
	err |= bios_u8(bios, dunk17->offset+1, &dunk17->hlen);
	err |= bios_u8(bios, dunk17->offset+2, &dunk17->entriesnum);
	err |= bios_u8(bios, dunk17->offset+3, &dunk17->rlen);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, dunk17->offset, dunk17->hlen + dunk17->rlen * dunk17->entriesnum, "DUNK17", -1);
	int wanthlen = 0;
	int wantrlen = 0;
	switch (dunk17->version) {
		case 0x10:
		case 0x11:
			wanthlen = 4;
			if (dunk17->hlen >= 10)
				wanthlen = 10;
			wantrlen = 2;
			break;
		default:
			ENVY_BIOS_ERR("Unknown DUNK17 table version %d.%d\n", dunk17->version >> 4, dunk17->version & 0xf);
			return -EINVAL;
	}
	if (dunk17->hlen < wanthlen) {
		ENVY_BIOS_ERR("DUNK17 table header too short [%d < %d]\n", dunk17->hlen, wanthlen);
		return -EINVAL;
	}
	if (dunk17->rlen < wantrlen) {
		ENVY_BIOS_ERR("DUNK17 table record too short [%d < %d]\n", dunk17->rlen, wantrlen);
		return -EINVAL;
	}
	if (dunk17->hlen > wanthlen) {
		ENVY_BIOS_WARN("DUNK17 table header longer than expected [%d > %d]\n", dunk17->hlen, wanthlen);
	}
	if (dunk17->rlen > wantrlen) {
		ENVY_BIOS_WARN("DUNK17 table record longer than expected [%d > %d]\n", dunk17->rlen, wantrlen);
	}
	dunk17->entries = calloc(dunk17->entriesnum, sizeof *dunk17->entries);
	if (!dunk17->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < dunk17->entriesnum; i++) {
		struct envy_bios_dunk17_entry *entry = &dunk17->entries[i];
		entry->offset = dunk17->offset + dunk17->hlen + dunk17->rlen * i;
	}
	dunk17->valid = 1;
	return 0;
}

void envy_bios_print_dunk17 (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_dunk17 *dunk17 = &bios->dunk17;
	if (!dunk17->offset || !(mask & ENVY_BIOS_PRINT_DUNK))
		return;
	if (!dunk17->valid) {
		fprintf(out, "Failed to parse DUNK17 table at 0x%04x version %d.%d\n\n", dunk17->offset, dunk17->version >> 4, dunk17->version & 0xf);
		return;
	}
	fprintf(out, "DUNK17 table at 0x%04x version %d.%d\n", dunk17->offset, dunk17->version >> 4, dunk17->version & 0xf);
	envy_bios_dump_hex(bios, out, dunk17->offset, dunk17->hlen, mask);
	int i;
	for (i = 0; i < dunk17->entriesnum; i++) {
		struct envy_bios_dunk17_entry *entry = &dunk17->entries[i];
		envy_bios_dump_hex(bios, out, entry->offset, dunk17->rlen, mask);
	}
	fprintf(out, "\n");
}
