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

int envy_bios_parse_iunk21 (struct envy_bios *bios) {
	struct envy_bios_iunk21 *iunk21 = &bios->iunk21;
	if (!iunk21->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, iunk21->offset, &iunk21->version);
	err |= bios_u8(bios, iunk21->offset+1, &iunk21->hlen);
	err |= bios_u8(bios, iunk21->offset+2, &iunk21->rlen);
	err |= bios_u8(bios, iunk21->offset+3, &iunk21->entriesnum);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, iunk21->offset, iunk21->hlen + iunk21->rlen * iunk21->entriesnum, "IUNK21", -1);
	int wanthlen = 0;
	int wantrlen = 0;
	switch (iunk21->version) {
		case 0x10:
			wanthlen = 4;
			wantrlen = 3;
			break;
		default:
			ENVY_BIOS_ERR("Unknown IUNK21 table version %x.%x\n", iunk21->version >> 4, iunk21->version & 0xf);
			return -EINVAL;
	}
	if (iunk21->hlen < wanthlen) {
		ENVY_BIOS_ERR("IUNK21 table header too short [%d < %d]\n", iunk21->hlen, wanthlen);
		return -EINVAL;
	}
	if (iunk21->rlen < wantrlen) {
		ENVY_BIOS_ERR("IUNK21 table record too short [%d < %d]\n", iunk21->rlen, wantrlen);
		return -EINVAL;
	}
	if (iunk21->hlen > wanthlen) {
		ENVY_BIOS_WARN("IUNK21 table header longer than expected [%d > %d]\n", iunk21->hlen, wanthlen);
	}
	if (iunk21->rlen > wantrlen) {
		ENVY_BIOS_WARN("IUNK21 table record longer than expected [%d > %d]\n", iunk21->rlen, wantrlen);
	}
	iunk21->entries = calloc(iunk21->entriesnum, sizeof *iunk21->entries);
	if (!iunk21->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < iunk21->entriesnum; i++) {
		struct envy_bios_iunk21_entry *entry = &iunk21->entries[i];
		entry->offset = iunk21->offset + iunk21->hlen + iunk21->rlen * i;
		/* XXX */
		if (err)
			return -EFAULT;
	}
	iunk21->valid = 1;
	return 0;
}

void envy_bios_print_iunk21 (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_iunk21 *iunk21 = &bios->iunk21;
	if (!iunk21->offset || !(mask & ENVY_BIOS_PRINT_IUNK))
		return;
	if (!iunk21->valid) {
		fprintf(out, "Failed to parse IUNK21 table at %04x version %x.%x\n\n", iunk21->offset, iunk21->version >> 4, iunk21->version & 0xf);
		return;
	}
	fprintf(out, "IUNK21 table at %04x version %x.%x\n", iunk21->offset, iunk21->version >> 4, iunk21->version & 0xf);
	envy_bios_dump_hex(bios, out, iunk21->offset, iunk21->hlen, mask);
	int i;
	for (i = 0; i < iunk21->entriesnum; i++) {
		struct envy_bios_iunk21_entry *entry = &iunk21->entries[i];
		/* XXX */
		envy_bios_dump_hex(bios, out, entry->offset, iunk21->rlen, mask);
	}
	fprintf(out, "\n");
}
