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
	err |= bios_u8(bios, dunk0c->offset, &dunk0c->version);
	err |= bios_u8(bios, dunk0c->offset+1, &dunk0c->hlen);
	err |= bios_u8(bios, dunk0c->offset+2, &dunk0c->entriesnum);
	err |= bios_u8(bios, dunk0c->offset+3, &dunk0c->rlen);
	if (err)
		return -EFAULT;
	int wanthlen = 0;
	int wantrlen = 0;
	switch (dunk0c->version) {
		case 0x30:
			wanthlen = 4;
			wantrlen = 1;
			break;
		case 0x40:
			wanthlen = 4;
			wantrlen = 1;
			break;
		default:
			ENVY_BIOS_ERR("Unknown DUNK0C table version %x.%x\n", dunk0c->version >> 4, dunk0c->version & 0xf);
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
	/* XXX */
	/*
	dunk0c->entries = calloc(gunk->entriesnum, sizeof *gunk->entries);
	if (!dunk0c->entries)
		return -ENOMEM;
		*/
	int i;
	for (i = 0; i < dunk0c->entriesnum; i++) {
		uint16_t eoff = dunk0c->offset + dunk0c->hlen + dunk0c->rlen * i;
//		struct envy_bios_dunk0c *dunk0c = &dunk0c->entries[i];
		/* XXX */
	}
	dunk0c->valid = 1;
	return 0;
}

void envy_bios_print_dunk0c (struct envy_bios *bios, FILE *out) {
	struct envy_bios_dunk0c *dunk0c = &bios->dunk0c;
	if (!dunk0c->offset)
		return;
	if (!dunk0c->valid) {
		fprintf(out, "Failed to parse DUNK0C table at %04x version %x.%x\n\n", dunk0c->offset, dunk0c->version >> 4, dunk0c->version & 0xf);
		return;
	}
	fprintf(out, "DUNK0C table at %04x version %x.%x\n", dunk0c->offset, dunk0c->version >> 4, dunk0c->version & 0xf);
	envy_bios_dump_hex(bios, out, dunk0c->offset, dunk0c->hlen);
	int i;
	for (i = 0; i < dunk0c->entriesnum; i++) {
		uint16_t eoff = dunk0c->offset + dunk0c->hlen + dunk0c->rlen * i;
		envy_bios_dump_hex(bios, out, eoff, dunk0c->rlen);
	}
	printf("\n");
}


int envy_bios_parse_dunk0e (struct envy_bios *bios) {
	struct envy_bios_dunk0e *dunk0e = &bios->dunk0e;
	if (!dunk0e->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, dunk0e->offset, &dunk0e->version);
	err |= bios_u8(bios, dunk0e->offset+1, &dunk0e->hlen);
	err |= bios_u8(bios, dunk0e->offset+2, &dunk0e->entriesnum);
	err |= bios_u8(bios, dunk0e->offset+3, &dunk0e->rlen);
	if (err)
		return -EFAULT;
	int wanthlen = 0;
	int wantrlen = 0;
	switch (dunk0e->version) {
		case 0x30:
			wanthlen = 12;
			wantrlen = 0;
			break;
		case 0x31:
			wanthlen = 12;
			wantrlen = 0;
			break;
		case 0x40:
			wanthlen = 12;
			wantrlen = 0;
			break;
		default:
			ENVY_BIOS_ERR("Unknown DUNK0E table version %x.%x\n", dunk0e->version >> 4, dunk0e->version & 0xf);
			return -EINVAL;
	}
	if (dunk0e->hlen < wanthlen) {
		ENVY_BIOS_ERR("DUNK0E table header too short [%d < %d]\n", dunk0e->hlen, wanthlen);
		return -EINVAL;
	}
	if (dunk0e->rlen < wantrlen) {
		ENVY_BIOS_ERR("DUNK0E table record too short [%d < %d]\n", dunk0e->rlen, wantrlen);
		return -EINVAL;
	}
	if (dunk0e->hlen > wanthlen) {
		ENVY_BIOS_WARN("DUNK0E table header longer than expected [%d > %d]\n", dunk0e->hlen, wanthlen);
	}
	if (dunk0e->rlen > wantrlen) {
		ENVY_BIOS_WARN("DUNK0E table record longer than expected [%d > %d]\n", dunk0e->rlen, wantrlen);
	}
	/* XXX */
	/*
	dunk0e->entries = calloc(gunk->entriesnum, sizeof *gunk->entries);
	if (!dunk0e->entries)
		return -ENOMEM;
		*/
	int i;
	for (i = 0; i < dunk0e->entriesnum; i++) {
		uint16_t eoff = dunk0e->offset + dunk0e->hlen + dunk0e->rlen * i;
//		struct envy_bios_dunk0e *dunk0e = &dunk0e->entries[i];
		/* XXX */
	}
	dunk0e->valid = 1;
	return 0;
}

void envy_bios_print_dunk0e (struct envy_bios *bios, FILE *out) {
	struct envy_bios_dunk0e *dunk0e = &bios->dunk0e;
	if (!dunk0e->offset)
		return;
	if (!dunk0e->valid) {
		fprintf(out, "Failed to parse DUNK0E table at %04x version %x.%x\n\n", dunk0e->offset, dunk0e->version >> 4, dunk0e->version & 0xf);
		return;
	}
	fprintf(out, "DUNK0E table at %04x version %x.%x\n", dunk0e->offset, dunk0e->version >> 4, dunk0e->version & 0xf);
	envy_bios_dump_hex(bios, out, dunk0e->offset, dunk0e->hlen);
	int i;
	for (i = 0; i < dunk0e->entriesnum; i++) {
		uint16_t eoff = dunk0e->offset + dunk0e->hlen + dunk0e->rlen * i;
		envy_bios_dump_hex(bios, out, eoff, dunk0e->rlen);
	}
	printf("\n");
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
	int wanthlen = 0;
	int wantrlen = 0;
	switch (dunk10->version) {
		case 0x30:
		case 0x31:
		case 0x40:
			wanthlen = 10;
			wantrlen = 1;
			break;
		case 0x41:
			wanthlen = 5;
			wantrlen = 1;
			break;
		default:
			ENVY_BIOS_ERR("Unknown DUNK10 table version %x.%x\n", dunk10->version >> 4, dunk10->version & 0xf);
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
	/* XXX */
	/*
	dunk10->entries = calloc(gunk->entriesnum, sizeof *gunk->entries);
	if (!dunk10->entries)
		return -ENOMEM;
		*/
	int i;
	for (i = 0; i < dunk10->entriesnum; i++) {
		uint16_t eoff = dunk10->offset + dunk10->hlen + dunk10->rlen * i;
//		struct envy_bios_dunk10 *dunk10 = &dunk10->entries[i];
		/* XXX */
	}
	dunk10->valid = 1;
	return 0;
}

void envy_bios_print_dunk10 (struct envy_bios *bios, FILE *out) {
	struct envy_bios_dunk10 *dunk10 = &bios->dunk10;
	if (!dunk10->offset)
		return;
	if (!dunk10->valid) {
		fprintf(out, "Failed to parse DUNK10 table at %04x version %x.%x\n\n", dunk10->offset, dunk10->version >> 4, dunk10->version & 0xf);
		return;
	}
	fprintf(out, "DUNK10 table at %04x version %x.%x\n", dunk10->offset, dunk10->version >> 4, dunk10->version & 0xf);
	envy_bios_dump_hex(bios, out, dunk10->offset, dunk10->hlen);
	int i;
	for (i = 0; i < dunk10->entriesnum; i++) {
		uint16_t eoff = dunk10->offset + dunk10->hlen + dunk10->rlen * i;
		envy_bios_dump_hex(bios, out, eoff, dunk10->rlen);
	}
	printf("\n");
}

int envy_bios_parse_dunk12 (struct envy_bios *bios) {
	struct envy_bios_dunk12 *dunk12 = &bios->dunk12;
	if (!dunk12->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, dunk12->offset, &dunk12->version);
	err |= bios_u8(bios, dunk12->offset+1, &dunk12->hlen);
	err |= bios_u8(bios, dunk12->offset+2, &dunk12->entriesnum);
	err |= bios_u8(bios, dunk12->offset+3, &dunk12->rlen);
	if (err)
		return -EFAULT;
	int wanthlen = 0;
	int wantrlen = 0;
	switch (dunk12->version) {
		case 0x30:
			wanthlen = 4;
			wantrlen = 4;
			break;
		case 0x40:
			wanthlen = 4;
			wantrlen = 4;
			break;
		default:
			ENVY_BIOS_ERR("Unknown DUNK12 table version %x.%x\n", dunk12->version >> 4, dunk12->version & 0xf);
			return -EINVAL;
	}
	if (dunk12->hlen < wanthlen) {
		ENVY_BIOS_ERR("DUNK12 table header too short [%d < %d]\n", dunk12->hlen, wanthlen);
		return -EINVAL;
	}
	if (dunk12->rlen < wantrlen) {
		ENVY_BIOS_ERR("DUNK12 table record too short [%d < %d]\n", dunk12->rlen, wantrlen);
		return -EINVAL;
	}
	if (dunk12->hlen > wanthlen) {
		ENVY_BIOS_WARN("DUNK12 table header longer than expected [%d > %d]\n", dunk12->hlen, wanthlen);
	}
	if (dunk12->rlen > wantrlen) {
		ENVY_BIOS_WARN("DUNK12 table record longer than expected [%d > %d]\n", dunk12->rlen, wantrlen);
	}
	/* XXX */
	/*
	dunk12->entries = calloc(gunk->entriesnum, sizeof *gunk->entries);
	if (!dunk12->entries)
		return -ENOMEM;
		*/
	int i;
	for (i = 0; i < dunk12->entriesnum; i++) {
		uint16_t eoff = dunk12->offset + dunk12->hlen + dunk12->rlen * i;
//		struct envy_bios_dunk12 *dunk12 = &dunk12->entries[i];
		/* XXX */
	}
	dunk12->valid = 1;
	return 0;
}

void envy_bios_print_dunk12 (struct envy_bios *bios, FILE *out) {
	struct envy_bios_dunk12 *dunk12 = &bios->dunk12;
	if (!dunk12->offset)
		return;
	if (!dunk12->valid) {
		fprintf(out, "Failed to parse DUNK12 table at %04x version %x.%x\n\n", dunk12->offset, dunk12->version >> 4, dunk12->version & 0xf);
		return;
	}
	fprintf(out, "DUNK12 table at %04x version %x.%x\n", dunk12->offset, dunk12->version >> 4, dunk12->version & 0xf);
	envy_bios_dump_hex(bios, out, dunk12->offset, dunk12->hlen);
	int i;
	for (i = 0; i < dunk12->entriesnum; i++) {
		uint16_t eoff = dunk12->offset + dunk12->hlen + dunk12->rlen * i;
		envy_bios_dump_hex(bios, out, eoff, dunk12->rlen);
	}
	printf("\n");
}

int envy_bios_parse_dunk14 (struct envy_bios *bios) {
	struct envy_bios_dunk14 *dunk14 = &bios->dunk14;
	if (!dunk14->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, dunk14->offset, &dunk14->version);
	err |= bios_u8(bios, dunk14->offset+1, &dunk14->hlen);
	err |= bios_u8(bios, dunk14->offset+2, &dunk14->entriesnum);
	err |= bios_u8(bios, dunk14->offset+3, &dunk14->rlen);
	if (err)
		return -EFAULT;
	int wanthlen = 0;
	int wantrlen = 0;
	switch (dunk14->version) {
		case 0x30:
			wanthlen = 5;
			wantrlen = 2;
			break;
		case 0x40:
			wanthlen = 5;
			wantrlen = 2;
			break;
		default:
			ENVY_BIOS_ERR("Unknown DUNK14 table version %x.%x\n", dunk14->version >> 4, dunk14->version & 0xf);
			return -EINVAL;
	}
	if (dunk14->hlen < wanthlen) {
		ENVY_BIOS_ERR("DUNK14 table header too short [%d < %d]\n", dunk14->hlen, wanthlen);
		return -EINVAL;
	}
	if (dunk14->rlen < wantrlen) {
		ENVY_BIOS_ERR("DUNK14 table record too short [%d < %d]\n", dunk14->rlen, wantrlen);
		return -EINVAL;
	}
	if (dunk14->hlen > wanthlen) {
		ENVY_BIOS_WARN("DUNK14 table header longer than expected [%d > %d]\n", dunk14->hlen, wanthlen);
	}
	if (dunk14->rlen > wantrlen) {
		ENVY_BIOS_WARN("DUNK14 table record longer than expected [%d > %d]\n", dunk14->rlen, wantrlen);
	}
	/* XXX */
	/*
	dunk14->entries = calloc(gunk->entriesnum, sizeof *gunk->entries);
	if (!dunk14->entries)
		return -ENOMEM;
		*/
	int i;
	for (i = 0; i < dunk14->entriesnum; i++) {
		uint16_t eoff = dunk14->offset + dunk14->hlen + dunk14->rlen * i;
//		struct envy_bios_dunk14 *dunk14 = &dunk14->entries[i];
		/* XXX */
	}
	dunk14->valid = 1;
	return 0;
}

void envy_bios_print_dunk14 (struct envy_bios *bios, FILE *out) {
	struct envy_bios_dunk14 *dunk14 = &bios->dunk14;
	if (!dunk14->offset)
		return;
	if (!dunk14->valid) {
		fprintf(out, "Failed to parse DUNK14 table at %04x version %x.%x\n\n", dunk14->offset, dunk14->version >> 4, dunk14->version & 0xf);
		return;
	}
	fprintf(out, "DUNK14 table at %04x version %x.%x\n", dunk14->offset, dunk14->version >> 4, dunk14->version & 0xf);
	envy_bios_dump_hex(bios, out, dunk14->offset, dunk14->hlen);
	int i;
	for (i = 0; i < dunk14->entriesnum; i++) {
		uint16_t eoff = dunk14->offset + dunk14->hlen + dunk14->rlen * i;
		envy_bios_dump_hex(bios, out, eoff, dunk14->rlen);
	}
	printf("\n");
}
