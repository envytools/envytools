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

int envy_bios_parse_extdev (struct envy_bios *bios) {
	struct envy_bios_extdev *extdev = &bios->extdev;
	if (!extdev->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, extdev->offset, &extdev->version);
	err |= bios_u8(bios, extdev->offset+1, &extdev->hlen);
	err |= bios_u8(bios, extdev->offset+2, &extdev->entriesnum);
	err |= bios_u8(bios, extdev->offset+3, &extdev->rlen);
	if (extdev->hlen >= 5)
		err |= bios_u8(bios, extdev->offset+4, &extdev->unk04);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, extdev->offset, extdev->hlen + extdev->rlen * extdev->entriesnum, "EXTDEV", -1);
	int wanthlen = 4;
	int wantrlen = 4;
	switch (extdev->version) {
		case 0x30:
		case 0x40:
			break;
		default:
			ENVY_BIOS_ERR("Unknown EXTDEV table version %d.%d\n", extdev->version >> 4, extdev->version & 0xf);
			return -EINVAL;
	}
	if (extdev->hlen >= 5)
		wanthlen = 5;
	if (extdev->hlen < wanthlen) {
		ENVY_BIOS_ERR("EXTDEV table header too short [%d < %d]\n", extdev->hlen, wanthlen);
		return -EINVAL;
	}
	if (extdev->rlen < wantrlen) {
		ENVY_BIOS_ERR("EXTDEV table record too short [%d < %d]\n", extdev->rlen, wantrlen);
		return -EINVAL;
	}
	if (extdev->hlen > wanthlen) {
		ENVY_BIOS_WARN("EXTDEV table header longer than expected [%d > %d]\n", extdev->hlen, wanthlen);
	}
	if (extdev->rlen > wantrlen) {
		ENVY_BIOS_WARN("EXTDEV table record longer than expected [%d > %d]\n", extdev->rlen, wantrlen);
	}
	extdev->entries = calloc(extdev->entriesnum, sizeof *extdev->entries);
	if (!extdev->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < extdev->entriesnum; i++) {
		struct envy_bios_extdev_entry *entry = &extdev->entries[i];
		entry->offset = extdev->offset + extdev->hlen + extdev->rlen * i;
		uint8_t bytes[4];
		int j;
		for (j = 0; j < 4; j++) {
			err |= bios_u8(bios, entry->offset+j, &bytes[j]);
			if (err)
				return -EFAULT;
		}
		entry->type = bytes[0];
		entry->addr = bytes[1];
		entry->bus = bytes[2] >> 4 & 1;
		entry->unk02_0 = bytes[2] & 0xf;
		entry->unk02_5 = bytes[2] >> 5;
		entry->unk03 = bytes[3];
	}
	extdev->valid = 1;
	return 0;
}

static struct enum_val extdev_types[] = {
	{ ENVY_BIOS_EXTDEV_LM89,	"LM89" },
	{ ENVY_BIOS_EXTDEV_VT1103M,	"VT1103M" },
	{ ENVY_BIOS_EXTDEV_PX3540,	"PX3540" },
	{ ENVY_BIOS_EXTDEV_VT1105M,	"VT1105M" },
	{ ENVY_BIOS_EXTDEV_ADT7473,	"ADT7473" },
	{ ENVY_BIOS_EXTDEV_HDCP_EEPROM,	"HDCP_EEPROM" },
	{ ENVY_BIOS_EXTDEV_UNUSED,	"UNUSED" },
	{ 0 },
};

void envy_bios_print_extdev (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_extdev *extdev = &bios->extdev;
	if (!extdev->offset || !(mask & ENVY_BIOS_PRINT_EXTDEV))
		return;
	if (!extdev->valid) {
		fprintf(out, "Failed to parse EXTDEV table at 0x%04x version %d.%d\n\n", extdev->offset, extdev->version >> 4, extdev->version & 0xf);
		return;
	}
	fprintf(out, "EXTDEV table at 0x%04x version %d.%d", extdev->offset, extdev->version >> 4, extdev->version & 0xf);
	if (extdev->hlen > 4)
		fprintf(out, " unk04 0x%02x", extdev->unk04);
	fprintf(out, "\n");
	envy_bios_dump_hex(bios, out, extdev->offset, extdev->hlen, mask);
	int i;
	for (i = 0; i < extdev->entriesnum; i++) {
		struct envy_bios_extdev_entry *entry = &extdev->entries[i];
		if (entry->type != ENVY_BIOS_EXTDEV_UNUSED || mask & ENVY_BIOS_PRINT_UNUSED) {
			const char *typename = find_enum(extdev_types, entry->type);
			fprintf(out, "EXTDEV %d:", i);
			fprintf(out, " type 0x%02x [%s]", entry->type, typename);
			fprintf(out, " at 0x%02x defbus %d", entry->addr, entry->bus);
			if (entry->unk02_0)
				fprintf(out, " unk02_0 %d", entry->unk02_0);
			if (entry->unk02_5)
				fprintf(out, " unk02_5 %d", entry->unk02_5);
			if (entry->unk03)
				fprintf(out, " unk03 0x%02x", entry->unk03);
			fprintf(out, "\n");
		}
		envy_bios_dump_hex(bios, out, entry->offset, extdev->rlen, mask);
	}
	fprintf(out, "\n");
}
