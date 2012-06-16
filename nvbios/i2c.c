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

int envy_bios_parse_i2c (struct envy_bios *bios) {
	struct envy_bios_i2c *i2c = &bios->i2c;
	if (!i2c->offset)
		return 0;
	int err = 0;
	int wanthlen = 5;
	int wantrlen = 4;
	uint8_t defs;
	if (bios->dcb.version >= 0x30) {
		err |= bios_u8(bios, i2c->offset, &i2c->version);
		err |= bios_u8(bios, i2c->offset+1, &i2c->hlen);
		err |= bios_u8(bios, i2c->offset+2, &i2c->entriesnum);
		err |= bios_u8(bios, i2c->offset+3, &i2c->rlen);
		err |= bios_u8(bios, i2c->offset+4, &defs);
		i2c->def[0] = defs & 0xf;
		i2c->def[1] = defs >> 4;
	} else {
		i2c->version = bios->dcb.version;
		wanthlen = i2c->hlen = 0;
		i2c->entriesnum = 16;
		i2c->rlen = 4;
	}
	if (err)
		return -EFAULT;
	switch (i2c->version) {
		case 0x12:
		case 0x14:
		case 0x15:
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x30:
		case 0x40:
			break;
		default:
			ENVY_BIOS_ERR("Unknown I2C table version %x.%x\n", i2c->version >> 4, i2c->version & 0xf);
			return -EINVAL;
	}
	if (i2c->hlen < wanthlen) {
		ENVY_BIOS_ERR("I2C table header too short [%d < %d]\n", i2c->hlen, wanthlen);
		return -EINVAL;
	}
	if (i2c->rlen < wantrlen) {
		ENVY_BIOS_ERR("I2C table record too short [%d < %d]\n", i2c->rlen, wantrlen);
		return -EINVAL;
	}
	if (i2c->hlen > wanthlen) {
		ENVY_BIOS_WARN("I2C table header longer than expected [%d > %d]\n", i2c->hlen, wanthlen);
	}
	if (i2c->rlen > wantrlen) {
		ENVY_BIOS_WARN("I2C table record longer than expected [%d > %d]\n", i2c->rlen, wantrlen);
	}
	i2c->entries = calloc(i2c->entriesnum, sizeof *i2c->entries);
	if (!i2c->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < i2c->entriesnum; i++) {
		struct envy_bios_i2c_entry *entry = &i2c->entries[i];
		entry->offset = i2c->offset + i2c->hlen + i2c->rlen * i;
		uint8_t bytes[4];
		int j;
		for (j = 0; j < 4; j++) {
			err |= bios_u8(bios, entry->offset+j, &bytes[j]);
			if (err)
				return -EFAULT;
		}
		if (i2c->version == 0x12) {
			entry->type = bytes[0] & 3;
			if (entry->type == 3)
				entry->type = 0xff;
			entry->vgacr_wr = bytes[3];
			entry->vgacr_rd = bytes[2];
		} else {
			entry->type = bytes[3];
			entry->unk02 = bytes[2];
			if (i2c->version < 0x20) {
				if (entry->type != 0xff)
					entry->type &= 7;
				entry->unk02 = 0;
			}
			switch (entry->type) {
				case ENVY_BIOS_I2C_VGACR:
					entry->vgacr_wr = bytes[0];
					entry->vgacr_rd = bytes[1];
					break;
				case ENVY_BIOS_I2C_PCRTC:
					entry->loc = bytes[0] & 0xf;
					entry->unk00_4 = bytes[0] >> 4;
					entry->addr = bytes[1];
					break;
				case ENVY_BIOS_I2C_PNVIO:
				case ENVY_BIOS_I2C_AUXCH:
					entry->loc = bytes[0] & 0xf;
					entry->unk00_4 = bytes[0] >> 4;
					entry->shared = bytes[1] & 1;
					entry->sharedid = bytes[1] >> 1 & 0xf;
					entry->unk01_5 = bytes[1] >> 5;
					break;
			}
		}
	}
	i2c->valid = 1;
	return 0;
}

static struct enum_val i2c_types[] = {
	{ ENVY_BIOS_I2C_VGACR,	"VGACR" },
	{ ENVY_BIOS_I2C_PCRTC,	"PCRTC" },
	{ ENVY_BIOS_I2C_PNVIO,	"PNVIO" },
	{ ENVY_BIOS_I2C_AUXCH,	"AUXCH" },
	{ ENVY_BIOS_I2C_UNUSED,	"UNUSED" },
	{ 0 },
};

void envy_bios_print_i2c (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_i2c *i2c = &bios->i2c;
	if (!i2c->offset || !(mask & ENVY_BIOS_PRINT_I2C))
		return;
	if (!i2c->valid) {
		fprintf(out, "Failed to parse I2C table at %04x version %x.%x\n\n", i2c->offset, i2c->version >> 4, i2c->version & 0xf);
		return;
	}
	fprintf(out, "I2C table at %04x version %x.%x", i2c->offset, i2c->version >> 4, i2c->version & 0xf);
	fprintf(out, " defaults %d %d", i2c->def[0], i2c->def[1]);
	fprintf(out, "\n");
	envy_bios_dump_hex(bios, out, i2c->offset, i2c->hlen, mask);
	int i;
	for (i = 0; i < i2c->entriesnum; i++) {
		struct envy_bios_i2c_entry *entry = &i2c->entries[i];
		const char *typename = find_enum(i2c_types, entry->type);
		fprintf(out, "I2C %d:", i);
		fprintf(out, " type 0x%02x [%s]", entry->type, typename);
		switch (entry->type) {
			case ENVY_BIOS_I2C_VGACR:
				fprintf(out, " wr 0x%02x", entry->vgacr_wr);
				fprintf(out, " rd 0x%02x", entry->vgacr_rd);
				if (entry->unk02)
					fprintf(out, " unk02 0x%02x", entry->unk02);
				break;
			case ENVY_BIOS_I2C_PCRTC:
				fprintf(out, " loc %d", entry->loc);
				if (entry->unk00_4)
					fprintf(out, " unk00_4 %d", entry->unk00_4);
				fprintf(out, " addr 0x%02x", entry->addr);
				if (entry->unk02)
					fprintf(out, " unk02 0x%02x", entry->unk02);
				break;
			case ENVY_BIOS_I2C_PNVIO:
			case ENVY_BIOS_I2C_AUXCH:
				fprintf(out, " loc %d", entry->loc);
				if (entry->unk00_4)
					fprintf(out, " unk00_4 %d", entry->unk00_4);
				if (entry->shared)
					fprintf(out, " shared %d", entry->sharedid);
				else if (entry->sharedid)
					fprintf(out, " unk01_1 %d", entry->sharedid);
				if (entry->unk01_5)
					fprintf(out, " unk01_5 %d", entry->unk01_5);
				if (entry->unk02)
					fprintf(out, " unk02 0x%02x", entry->unk02);
				break;
		}
		fprintf(out, "\n");
		envy_bios_dump_hex(bios, out, entry->offset, i2c->rlen, mask);
	}
	fprintf(out, "\n");
}
