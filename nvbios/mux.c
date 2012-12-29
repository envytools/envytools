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

int envy_bios_parse_mux (struct envy_bios *bios) {
	struct envy_bios_mux *mux = &bios->mux;
	if (!mux->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, mux->offset, &mux->version);
	err |= bios_u8(bios, mux->offset+1, &mux->hlen);
	err |= bios_u8(bios, mux->offset+2, &mux->entriesnum);
	err |= bios_u8(bios, mux->offset+3, &mux->rlen);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, mux->offset, mux->hlen + mux->rlen * mux->entriesnum, "MUX", -1);
	int wanthlen = 0;
	int wantrlen = 0;
	switch (mux->version) {
		case 0x10:
			wanthlen = 4;
			wantrlen = 5;
			break;
		default:
			ENVY_BIOS_ERR("Unknown MUX table version %d.%d\n", mux->version >> 4, mux->version & 0xf);
			return -EINVAL;
	}
	if (mux->hlen < wanthlen) {
		ENVY_BIOS_ERR("MUX table header too short [%d < %d]\n", mux->hlen, wanthlen);
		return -EINVAL;
	}
	if (mux->rlen < wantrlen) {
		ENVY_BIOS_ERR("MUX table record too short [%d < %d]\n", mux->rlen, wantrlen);
		return -EINVAL;
	}
	if (mux->hlen > wanthlen) {
		ENVY_BIOS_WARN("MUX table header longer than expected [%d > %d]\n", mux->hlen, wanthlen);
	}
	if (mux->rlen > wantrlen) {
		ENVY_BIOS_WARN("MUX table record longer than expected [%d > %d]\n", mux->rlen, wantrlen);
	}
	mux->entries = calloc(mux->entriesnum, sizeof *mux->entries);
	if (!mux->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < mux->entriesnum; i++) {
		struct envy_bios_mux_entry *entry = &mux->entries[i];
		entry->offset = mux->offset + mux->hlen + mux->rlen * i;
		uint8_t sub[4];
		err |= bios_u8(bios, entry->offset, &entry->idx);
		int j;
		for (j = 0; j < 4; j++) {
			err |= bios_u8(bios, entry->offset+j+1, &sub[j]);
			entry->sub_loc[j] = sub[j] & 1;
			entry->sub_line[j] = sub[j] >> 1 & 0x1f;
			entry->sub_val[j] = sub[j] >> 6 & 1;
			entry->sub_unk7[j] = sub[j] >> 7 & 1;
		}
		if (err)
			return -EFAULT;
	}
	mux->valid = 1;
	return 0;
}

void envy_bios_print_mux (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_mux *mux = &bios->mux;
	if (!mux->offset || !(mask & ENVY_BIOS_PRINT_MUX))
		return;
	if (!mux->valid) {
		fprintf(out, "Failed to parse MUX table at 0x%04x version %d.%d\n\n", mux->offset, mux->version >> 4, mux->version & 0xf);
		return;
	}
	fprintf(out, "MUX table at 0x%04x version %d.%d\n", mux->offset, mux->version >> 4, mux->version & 0xf);
	envy_bios_dump_hex(bios, out, mux->offset, mux->hlen, mask);
	int i;
	for (i = 0; i < mux->entriesnum; i++) {
		struct envy_bios_mux_entry *entry = &mux->entries[i];
		if (entry->idx != 0x1f || mask & ENVY_BIOS_PRINT_UNUSED) {
			fprintf(out, "MUX %d:", i);
			if (entry->idx != 0x1f)
				fprintf(out, " DCB %d", entry->idx);
			else
				fprintf(out, " UNUSED");
			int j;
			for (j = 0; j < 4; j++) {
				if (entry->sub_line[j] != 0x1f) {
					const char *const subs[4] = {
						"OUT",
						"UNK1",
						"HPD",
						"DDC",
					};
					fprintf(out, " %s: %s line %d val %d", subs[j], entry->sub_loc[j]?"XPIO":"GPIO", entry->sub_line[j], entry->sub_val[j]);
					if (entry->sub_unk7[j])
						fprintf(out, " unk7 %d", entry->sub_unk7[j]);
				}
			}
			fprintf(out, "\n");
		}
		envy_bios_dump_hex(bios, out, entry->offset, mux->rlen, mask);
	}
	fprintf(out, "\n");
}
