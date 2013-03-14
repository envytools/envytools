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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "bios.h"

int envy_bios_parse_conn (struct envy_bios *bios) {
	struct envy_bios_conn *conn = &bios->conn;
	if (!conn->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, conn->offset, &conn->version);
	err |= bios_u8(bios, conn->offset+1, &conn->hlen);
	err |= bios_u8(bios, conn->offset+2, &conn->entriesnum);
	err |= bios_u8(bios, conn->offset+3, &conn->rlen);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, conn->offset, conn->hlen + conn->rlen * conn->entriesnum, "CONN", -1);
	int wanthlen = 5;
	int wantrlen = 4;
	if (conn->rlen < 4)
		wantrlen = 2;
	switch (conn->version) {
		case 0x30:
		case 0x40:
			break;
		default:
			ENVY_BIOS_ERR("Unknown CONN table version %d.%d\n", conn->version >> 4, conn->version & 0xf);
			return -EINVAL;
	}
	if (conn->hlen < wanthlen) {
		ENVY_BIOS_ERR("CONN table header too short [%d < %d]\n", conn->hlen, wanthlen);
		return -EINVAL;
	}
	if (conn->rlen < wantrlen) {
		ENVY_BIOS_ERR("CONN table record too short [%d < %d]\n", conn->rlen, wantrlen);
		return -EINVAL;
	}
	if (conn->hlen > wanthlen) {
		ENVY_BIOS_WARN("CONN table header longer than expected [%d > %d]\n", conn->hlen, wanthlen);
	}
	if (conn->rlen > wantrlen) {
		ENVY_BIOS_WARN("CONN table record longer than expected [%d > %d]\n", conn->rlen, wantrlen);
	}
	conn->entries = calloc(conn->entriesnum, sizeof *conn->entries);
	if (!conn->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < conn->entriesnum; i++) {
		struct envy_bios_conn_entry *entry = &conn->entries[i];
		entry->offset = conn->offset + conn->hlen + conn->rlen * i;
		uint8_t bytes[4] = { 0 };
		uint32_t val = 0;
		int j;
		static const int hpds[7] = { 12, 13, 16, 17, 24, 25, 26 };
		entry->hpd = -1;
		entry->dp_ext = -1;
		for (j = 0; j < 4 && j < conn->rlen; j++) {
			err |= bios_u8(bios, entry->offset+j, &bytes[j]);
			if (err)
				return -EFAULT;
			val |= bytes[j] << j * 8;
		}
		entry->type = bytes[0];
		entry->tag = bytes[1] & 0xf;
		for (j = 0; j < 7; j++) {
			if (val & 1 << hpds[j]) {
				if (entry->hpd == -1)
					entry->hpd = j;
				else
					ENVY_BIOS_ERR("CONN %d: duplicate HPD bits\n", i);
			}
		}
		for (j = 0; j < 2; j++) {
			if (val & 1 << (j+14)) {
				if (entry->dp_ext == -1)
					entry->dp_ext = j;
				else
					ENVY_BIOS_ERR("CONN %d: duplicate DP_AUX bits\n", i);
			}
		}
		entry->unk02_2 = bytes[2] >> 2 & 3;
		entry->unk02_4 = bytes[2] >> 4 & 7;
		entry->unk02_7 = bytes[2] >> 7 & 1;
		entry->unk03_3 = bytes[3] >> 3 & 0x1f;
	}
	conn->valid = 1;
	return 0;
}

static struct enum_val conn_types[] = {
	{ ENVY_BIOS_CONN_VGA,			"VGA" },
	{ ENVY_BIOS_CONN_COMPOSITE,		"COMPOSITE" },
	{ ENVY_BIOS_CONN_S_VIDEO,		"S_VIDEO" },
	{ ENVY_BIOS_CONN_S_VIDEO_COMPOSITE,	"S_VIDEO_COMPOSITE" },
	{ ENVY_BIOS_CONN_RGB,			"RGB" },
	{ ENVY_BIOS_CONN_DVI_I,			"DVI_I" },
	{ ENVY_BIOS_CONN_DVI_D,			"DVI_D" },
	{ ENVY_BIOS_CONN_DMS59_0,		"DMS59_0" },
	{ ENVY_BIOS_CONN_DMS59_1,		"DMS59_1" },
	{ ENVY_BIOS_CONN_LVDS,			"LVDS" },
	{ ENVY_BIOS_CONN_LVDS_SPWG,		"LVDS_SPWG" },
	{ ENVY_BIOS_CONN_DP,			"DP" },
	{ ENVY_BIOS_CONN_EDP,			"EDP" },
	{ ENVY_BIOS_CONN_STEREO,		"STEREO" },
	{ ENVY_BIOS_CONN_HDMI,			"HDMI" },
	{ ENVY_BIOS_CONN_DMS59_DP0,		"DMS59_DP0" },
	{ ENVY_BIOS_CONN_DMS59_DP1,		"DMS59_DP1" },
	{ ENVY_BIOS_CONN_UNUSED,		"UNUSED" },
	{ 0 },
};

void envy_bios_print_conn (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_conn *conn = &bios->conn;
	if (!conn->offset || !(mask & ENVY_BIOS_PRINT_CONN))
		return;
	if (!conn->valid) {
		fprintf(out, "Failed to parse CONN table at 0x%04x version %d.%d\n\n", conn->offset, conn->version >> 4, conn->version & 0xf);
		return;
	}
	fprintf(out, "CONN table at 0x%04x version %d.%d\n", conn->offset, conn->version >> 4, conn->version & 0xf);
	envy_bios_dump_hex(bios, out, conn->offset, conn->hlen, mask);
	int i;
	for (i = 0; i < conn->entriesnum; i++) {
		struct envy_bios_conn_entry *entry = &conn->entries[i];
		if (entry->type != ENVY_BIOS_CONN_UNUSED || mask & ENVY_BIOS_PRINT_UNUSED) {
			const char *typename = find_enum(conn_types, entry->type);
			fprintf(out, "CONN %d:", i);
			fprintf(out, " type 0x%02x [%s] tag %d", entry->type, typename, entry->tag);
			if (entry->hpd != -1)
				fprintf(out, " HPD_%d", entry->hpd);
			if (entry->dp_ext != -1)
				fprintf(out, " DP_EXT_%d", entry->dp_ext);
			if (entry->unk02_2)
				fprintf(out, " unk02_2 0x%02x", entry->unk02_2);
			if (entry->unk02_4)
				fprintf(out, " unk02_4 0x%02x", entry->unk02_4);
			if (entry->unk02_7)
				fprintf(out, " unk02_7 0x%02x", entry->unk02_7);
			if (entry->unk03_3)
				fprintf(out, " unk03_3 0x%02x", entry->unk03_3);
			fprintf(out, "\n");
		}
		envy_bios_dump_hex(bios, out, entry->offset, conn->rlen, mask);
	}
	fprintf(out, "\n");
}
