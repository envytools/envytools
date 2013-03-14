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

int envy_bios_parse_bit_A (struct envy_bios *bios, struct envy_bios_bit_entry *bit) {
	struct envy_bios_dacload *dacload = &bios->dacload;
	dacload->bit = bit;
	int wantlen = 3;
	if (bit->t_len < wantlen) {
		ENVY_BIOS_ERR("Analog load table too short: %d < %d\n", bit->t_len, wantlen);
		return -EINVAL;
	}
	if (bit->t_len > wantlen)
		ENVY_BIOS_WARN("Analog load table longer than expected: %d > %d\n", bit->t_len, wantlen);
	if (dacload->valid) {
		ENVY_BIOS_ERR("Found BIT analog table, but DACLOAD table already referenced from info table!\n");
		return -EEXIST;
	}
	int err = 0;
	err |= bios_u16(bios, bit->t_offset+0, &dacload->offset);
	err |= bios_u8(bios, bit->t_offset+2, &dacload->unk02);
	if (err)
		return -EFAULT;
	if (envy_bios_parse_dacload(bios))
		ENVY_BIOS_ERR("Failed to parse DACLOAD table at 0x%04x version %d.%d\n", bios->dacload.offset, bios->dacload.version >> 4, bios->dacload.version & 0xf);
	return 0;
}

int envy_bios_parse_dacload (struct envy_bios *bios) {
	struct envy_bios_dacload *dacload = &bios->dacload;
	if (!dacload->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, dacload->offset, &dacload->version);
	err |= bios_u8(bios, dacload->offset+1, &dacload->hlen);
	err |= bios_u8(bios, dacload->offset+2, &dacload->rlen);
	err |= bios_u8(bios, dacload->offset+3, &dacload->entriesnum);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, dacload->offset, dacload->hlen + dacload->rlen * dacload->entriesnum, "DACLOAD", -1);
	int wanthlen = 0;
	int wantrlen = 0;
	switch (dacload->version) {
		case 0x00:
		case 0x10:
			wanthlen = 4;
			wantrlen = 4;
			break;
		default:
			ENVY_BIOS_ERR("Unknown DACLOAD table version %d.%d\n", dacload->version >> 4, dacload->version & 0xf);
			return -EINVAL;
	}
	if (dacload->hlen < wanthlen) {
		ENVY_BIOS_ERR("DACLOAD table header too short [%d < %d]\n", dacload->hlen, wanthlen);
		return -EINVAL;
	}
	if (dacload->rlen < wantrlen) {
		ENVY_BIOS_ERR("DACLOAD table record too short [%d < %d]\n", dacload->rlen, wantrlen);
		return -EINVAL;
	}
	if (dacload->hlen > wanthlen) {
		ENVY_BIOS_WARN("DACLOAD table header longer than expected [%d > %d]\n", dacload->hlen, wanthlen);
	}
	if (dacload->rlen > wantrlen) {
		ENVY_BIOS_WARN("DACLOAD table record longer than expected [%d > %d]\n", dacload->rlen, wantrlen);
	}
	dacload->entries = calloc(dacload->entriesnum, sizeof *dacload->entries);
	if (!dacload->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < dacload->entriesnum; i++) {
		struct envy_bios_dacload_entry *entry = &dacload->entries[i];
		entry->offset = dacload->offset + dacload->hlen + dacload->rlen * i;
		err |= bios_u32(bios, entry->offset, &entry->val);
		if (err)
			return -EFAULT;
	}
	dacload->valid = 1;
	return 0;
}

void envy_bios_print_dacload (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_dacload *dacload = &bios->dacload;
	if (!(mask & ENVY_BIOS_PRINT_DACLOAD))
		return;
	if (dacload->bit) {
		fprintf(out, "BIT 'A' unk02 0x%02x\n", dacload->unk02);
		envy_bios_dump_hex(bios, out, dacload->bit->t_offset, dacload->bit->t_len, mask);
	}
	if (!dacload->offset)
		return;
	if (!dacload->valid) {
		fprintf(out, "Failed to parse DACLOAD table at 0x%04x version %d.%d\n\n", dacload->offset, dacload->version >> 4, dacload->version & 0xf);
		return;
	}
	fprintf(out, "DACLOAD table at 0x%04x version %d.%d\n", dacload->offset, dacload->version >> 4, dacload->version & 0xf);
	envy_bios_dump_hex(bios, out, dacload->offset, dacload->hlen, mask);
	int i;
	for (i = 0; i < dacload->entriesnum; i++) {
		struct envy_bios_dacload_entry *entry = &dacload->entries[i];
		fprintf(out, "DACLOAD %d: 0x%08x\n", i, entry->val);
		envy_bios_dump_hex(bios, out, entry->offset, dacload->rlen, mask);
	}
	fprintf(out, "\n");
}
