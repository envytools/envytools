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
#include <string.h>

int envy_bios_parse_rdcb (struct envy_bios *bios);

int envy_bios_parse_dcb (struct envy_bios *bios) {
	struct envy_bios_dcb *dcb = &bios->dcb;
	if (!dcb->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, dcb->offset, &dcb->version);
	if (err)
		return -EFAULT;
	int wanthlen, wantrlen;
	uint32_t sig;
	uint8_t defs;
	switch (dcb->version) {
		case 0x10:
		case 0x11:
			/* old useless crap */
			wanthlen = dcb->hlen = 0;
			wantrlen = dcb->rlen = 0;
			dcb->version = 0;
			bios->odcb_offset = dcb->offset;
			dcb->offset = 0;
			return 0;
		case 0x12:
			wanthlen = dcb->hlen = 4;
			dcb->entriesnum = 16;
			wantrlen = dcb->rlen = 8;
			err |= bios_u8(bios, dcb->offset+1, &defs);
			err |= bios_u16(bios, dcb->offset+2, &bios->i2c.offset);
			bios->i2c.def[0] = defs & 0xf;
			bios->i2c.def[1] = defs >> 4;
			bios->odcb_offset = dcb->offset - 0x80;
			envy_bios_block(bios, bios->odcb_offset, 0x80, "ODCB", -1);
			break;
		case 0x14:
		case 0x15:
			wanthlen = dcb->hlen = 4;
			dcb->entriesnum = 16;
			wantrlen = dcb->rlen = 10;
			err |= bios_u8(bios, dcb->offset+1, &defs);
			err |= bios_u16(bios, dcb->offset+2, &bios->i2c.offset);
			bios->i2c.def[0] = defs & 0xf;
			bios->i2c.def[1] = defs >> 4;
			break;
		case 0x22:
		case 0x20:
		case 0x21:
			wanthlen = dcb->hlen = 8;
			dcb->entriesnum = 16;
			wantrlen = dcb->rlen = 8;
			err |= bios_u8(bios, dcb->offset+1, &defs);
			err |= bios_u16(bios, dcb->offset+2, &bios->i2c.offset);
			err |= bios_u32(bios, dcb->offset+4, &sig);
			if (sig != 0x4edcbdcb) {
				ENVY_BIOS_ERR("DCB sig mismatch\n");
				return -EINVAL;
			}
			bios->i2c.def[0] = defs & 0xf;
			bios->i2c.def[1] = defs >> 4;
			break;
		case 0x30:
		case 0x40:
		case 0x41:
			wanthlen = 23;
			wantrlen = 8;
			err |= bios_u8(bios, dcb->offset+1, &dcb->hlen);
			err |= bios_u8(bios, dcb->offset+2, &dcb->entriesnum);
			err |= bios_u8(bios, dcb->offset+3, &dcb->rlen);
			err |= bios_u16(bios, dcb->offset+4, &bios->i2c.offset);
			err |= bios_u32(bios, dcb->offset+6, &sig);
			err |= bios_u16(bios, dcb->offset+10, &bios->gpio.offset);
			err |= bios_u16(bios, dcb->offset+12, &bios->inputdev.offset);
			err |= bios_u16(bios, dcb->offset+14, &bios->cinema.offset);
			err |= bios_u16(bios, dcb->offset+16, &bios->spreadspectrum.offset);
			err |= bios_u16(bios, dcb->offset+18, &bios->extdev.offset);
			err |= bios_u16(bios, dcb->offset+20, &bios->conn.offset);
			err |= bios_u8(bios, dcb->offset+22, &dcb->unk16);
			if (dcb->hlen >= 25) {
				wanthlen = 25;
				err |= bios_u16(bios, dcb->offset+23, &bios->hdtvtt.offset);
			}
			if (dcb->hlen >= 27) {
				wanthlen = 27;
				err |= bios_u16(bios, dcb->offset+25, &bios->mux.offset);
			}
			if (sig != 0x4edcbdcb) {
				ENVY_BIOS_ERR("DCB sig mismatch\n");
				return -EINVAL;
			}
			break;
		default:
			ENVY_BIOS_ERR("Unknown DCB table version %d.%d\n", dcb->version >> 4, dcb->version & 0xf);
			return -EINVAL;
	}
	envy_bios_block(bios, dcb->offset, dcb->hlen + dcb->rlen * dcb->entriesnum, "DCB", -1);
	if (dcb->version >= 0x14 && dcb->version < 0x30) {
		uint8_t dev_rec[7];
		int j;
		for (j = 0; j < 7; j++)
			err |= bios_u8(bios, dcb->offset-7+j, &dev_rec[j]);
		if (!err && !memcmp(dev_rec, "DEV_REC", 7)) {
			bios->dev_rec_offset = dcb->offset - 7;
			bios->odcb_offset = bios->dev_rec_offset - 0x80;
			envy_bios_block(bios, bios->dev_rec_offset, 7, "DEV_REC", -1);
			envy_bios_block(bios, bios->odcb_offset, 0x80, "ODCB", -1);
		} else {
			if (envy_bios_parse_rdcb(bios))
				ENVY_BIOS_ERR("Failed to parse RDCB table at 0x%04x version %d.%d\n", bios->dcb.offset, bios->dcb.rdcb_version >> 4, bios->dcb.rdcb_version & 0xf);
		}
	}
	if (err)
		return -EFAULT;
	if (dcb->hlen < wanthlen) {
		ENVY_BIOS_ERR("DCB table header too short [%d < %d]\n", dcb->hlen, wanthlen);
		return -EINVAL;
	}
	if (dcb->rlen < wantrlen) {
		ENVY_BIOS_ERR("DCB table record too short [%d < %d]\n", dcb->rlen, wantrlen);
		return -EINVAL;
	}
	if (dcb->hlen > wanthlen) {
		ENVY_BIOS_WARN("DCB table header longer than expected [%d > %d]\n", dcb->hlen, wanthlen);
	}
	if (dcb->rlen > wantrlen) {
		ENVY_BIOS_WARN("DCB table record longer than expected [%d > %d]\n", dcb->rlen, wantrlen);
	}
	dcb->entries = calloc(dcb->entriesnum, sizeof *dcb->entries);
	if (!dcb->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < dcb->entriesnum; i++) {
		struct envy_bios_dcb_entry *entry = &dcb->entries[i];
		entry->offset = dcb->offset + dcb->hlen + dcb->rlen * i;
		uint8_t bytes[10];
		int j;
		for (j = 0; j < 10 && j < dcb->rlen; j++) {
			err |= bios_u8(bios, entry->offset+j, &bytes[j]);
			if (err)
				return -EFAULT;
		}
		if (dcb->version >= 0x20) {
			entry->type = bytes[0] & 0xf;
			entry->i2c = bytes[0] >> 4;
			entry->heads = bytes[1] & 0xf;
			if (dcb->version >= 0x30)
				entry->conn = bytes[1] >> 4;
			else
				entry->unk01_4 = bytes[1] >> 4;
			entry->conntag = bytes[2] & 0xf;
			entry->loc = bytes[2] >> 4 & 3;
			entry->unk02_6 = bytes[2] >> 6;
			entry->or = bytes[3] & 0xf;
			entry->unk03_4 = bytes[3] >> 4; /* seen used once on quadro, not sure if it means something */
			entry->unk04 = bytes[4];
			entry->unk05 = bytes[5];
			entry->unk06 = bytes[6];
			entry->unk07 = bytes[7];
			switch (entry->type) {
				case ENVY_BIOS_DCB_ANALOG:
					if (dcb->version < 0x30) {
						entry->maxfreq = (bytes[4] | bytes[5] << 8) * 10;
						entry->unk04 = 0;
						entry->unk05 = 0;
					} else {
						entry->maxfreq = bytes[4] * 10000;
						entry->unk04 = 0;
					}
					break;
				case ENVY_BIOS_DCB_TMDS:
					if (dcb->version < 0x30) {
						entry->ext_addr = bytes[4] >> 4;
						entry->unk04 &= ~0xf0;
					} else {
						entry->ext_addr = bytes[5];
						entry->unk05 = 0;
						entry->tmds_hdmi = !!(bytes[6] & 2);
						entry->unk06 &= ~2;
					}
					break;
				case ENVY_BIOS_DCB_LVDS:
					entry->lvds_info = bytes[4] & 3;
					entry->lvds_pscript = bytes[4] >> 2 & 3;
					entry->unk04 &= ~0xf;
					break;
				case ENVY_BIOS_DCB_DP:
					entry->ext_addr = bytes[5];
					entry->unk05 = 0;
					entry->dp_bw = bytes[6] >> 5;
					entry->unk06 &= ~0xe0;
					entry->dp_lanes = bytes[7] & 0xf;
					entry->unk07 &= ~0xf;
					break;
				/* XXX: TV */
				default:
					break;
			}
			if ((entry->type == ENVY_BIOS_DCB_LVDS || entry->type == ENVY_BIOS_DCB_TMDS || entry->type == ENVY_BIOS_DCB_DP) && dcb->version >= 0x40) {
				entry->unk04 &= ~0x30;
				entry->links = bytes[4] >> 4 & 3;
				entry->unk06 &= ~2;
			}
		} else if (dcb->version >= 0x14) {
			entry->type = bytes[0] & 0xf;
			entry->unk00_4 = bytes[0] >> 4;
			entry->i2c = (bytes[1] >> 6 & 3) | (bytes[2] << 2 & 0xc);
			entry->heads = entry->or = 1 << (bytes[2] >> 2 & 7);
			entry->loc = (bytes[2] >> 5) | (bytes[3] << 3 & 1);
			entry->conntag = bytes[3] >> 1 & 7;
			if (entry->type == ENVY_BIOS_DCB_ANALOG)
				entry->maxfreq = (bytes[6] | bytes[7] << 8) * 10;
			/* XXX */
		} else {
			/* XXX */
		}
	}
	dcb->valid = 1;
	if (envy_bios_parse_i2c(bios))
		ENVY_BIOS_ERR("Failed to parse I2C table at 0x%04x version %d.%d\n", bios->i2c.offset, bios->i2c.version >> 4, bios->i2c.version & 0xf);
	if (envy_bios_parse_gpio(bios))
		ENVY_BIOS_ERR("Failed to parse GPIO table at 0x%04x version %d.%d\n", bios->gpio.offset, bios->gpio.version >> 4, bios->gpio.version & 0xf);
	if (envy_bios_parse_inputdev(bios))
		ENVY_BIOS_ERR("Failed to parse INPUTDEV table at 0x%04x version %d.%d\n", bios->inputdev.offset, bios->inputdev.version >> 4, bios->inputdev.version & 0xf);
	if (envy_bios_parse_cinema(bios))
		ENVY_BIOS_ERR("Failed to parse CINEMA table at 0x%04x version %d.%d\n", bios->cinema.offset, bios->cinema.version >> 4, bios->cinema.version & 0xf);
	if (envy_bios_parse_spreadspectrum(bios))
		ENVY_BIOS_ERR("Failed to parse SPREADSPECTRUM table at 0x%04x version %d.%d\n", bios->spreadspectrum.offset, bios->spreadspectrum.version >> 4, bios->spreadspectrum.version & 0xf);
	if (envy_bios_parse_extdev(bios))
		ENVY_BIOS_ERR("Failed to parse EXTDEV table at 0x%04x version %d.%d\n", bios->extdev.offset, bios->extdev.version >> 4, bios->extdev.version & 0xf);
	if (envy_bios_parse_conn(bios))
		ENVY_BIOS_ERR("Failed to parse CONN table at 0x%04x version %d.%d\n", bios->conn.offset, bios->conn.version >> 4, bios->conn.version & 0xf);
	if (envy_bios_parse_hdtvtt(bios))
		ENVY_BIOS_ERR("Failed to parse HDTVTT table at 0x%04x version %d.%d\n", bios->hdtvtt.offset, bios->hdtvtt.version >> 4, bios->hdtvtt.version & 0xf);
	if (envy_bios_parse_mux(bios))
		ENVY_BIOS_ERR("Failed to parse MUX table at 0x%04x version %d.%d\n", bios->mux.offset, bios->mux.version >> 4, bios->mux.version & 0xf);
	return 0;
}

int envy_bios_parse_rdcb (struct envy_bios *bios) {
	struct envy_bios_dcb *dcb = &bios->dcb;
	int err = 0;
	err |= bios_u8(bios, dcb->offset - 1, &dcb->rdcb_version);
	if (err)
		return -EFAULT;
	switch (dcb->rdcb_version) {
		case 0x11:
			dcb->rdcb_len = 0xc;
			break;
		case 0x15:
		case 0x16:
			dcb->rdcb_len = 0x18;
			break;
		case 0x17:
			dcb->rdcb_len = 0x1c;
			break;
		default:
			ENVY_BIOS_ERR("Unknown RDCB table version %d.%d\n", dcb->rdcb_version >> 4, dcb->rdcb_version & 0xf);
			return -EINVAL;
	}
	err |= bios_u16(bios, dcb->offset - 3, &bios->inputdev.offset);
	uint8_t tv0, tv1;
	int j;
	err |= bios_u8(bios, dcb->offset - 5, &tv0);
	err |= bios_u8(bios, dcb->offset - 4, &tv1);
	dcb->tvdac0_present = tv0 & 1;
	dcb->tvdac0_neg = tv0 >> 1 & 1;
	dcb->tvdac0_line = tv1 >> 4;
	dcb->rdcb_unk04_0 = tv1 & 0xf;
	dcb->rdcb_unk05_2 = tv0 >> 2;
	for (j = 0; j < 7; j++)
		err |= bios_u8(bios, dcb->offset - 6 - j, &dcb->rdcb_unk06[j]);
	if (dcb->rdcb_version >= 0x15) {
		err |= bios_u8(bios, dcb->offset - 13, &dcb->rdcb_unk0d);
		err |= bios_u16(bios, dcb->offset - 15, &bios->gpio.offset);
		for (j = 0; j < 9; j++)
			err |= bios_u8(bios, dcb->offset - 16 - j, &dcb->rdcb_unk10[j]);
	}
	if (dcb->rdcb_version >= 0x17) {
		for (j = 9; j < 13; j++)
			err |= bios_u8(bios, dcb->offset - 16 - j, &dcb->rdcb_unk10[j]);
	}
	if (err)
		return -EFAULT;
	envy_bios_block(bios, dcb->offset - dcb->rdcb_len, dcb->rdcb_len, "RDCB", -1);
	if (dcb->rdcb_version < 0x16) {
		bios->odcb_offset = dcb->offset - dcb->rdcb_len - 0x80;
		envy_bios_block(bios, bios->odcb_offset, 0x80, "ODCB", -1);
	}
	dcb->rdcb_valid = 1;
	return 0;
}

static struct enum_val dcb_types[] = {
	{ ENVY_BIOS_DCB_ANALOG,		"ANALOG" },
	{ ENVY_BIOS_DCB_TV,		"TV" },
	{ ENVY_BIOS_DCB_TMDS,		"TMDS" },
	{ ENVY_BIOS_DCB_LVDS,		"LVDS" },
	{ ENVY_BIOS_DCB_RESERVED4,	"RESERVED4" },
	{ ENVY_BIOS_DCB_SDI,		"SDI" },
	{ ENVY_BIOS_DCB_DP,		"DP" },
	{ ENVY_BIOS_DCB_RESERVED8,	"RESERVED8" },
	{ ENVY_BIOS_DCB_EOL,		"EOL" },
	{ ENVY_BIOS_DCB_UNUSED,		"UNUSED" },
};

static struct enum_val dcb_lvds_info_types[] = {
	{ ENVY_BIOS_DCB_LVDS_EDID_I2C,	"EDID_I2C" },
	{ ENVY_BIOS_DCB_LVDS_STRAPS,	"STRAPS" },
	{ ENVY_BIOS_DCB_LVDS_EDID_ACPI,	"EDID_ACPI" },
};

void envy_bios_print_dcb (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_dcb *dcb = &bios->dcb;
	if (!dcb->offset || !(mask & ENVY_BIOS_PRINT_DCB))
		return;
	if (!dcb->valid) {
		fprintf(out, "Failed to parse DCB table at 0x%04x version %d.%d\n\n", dcb->offset, dcb->version >> 4, dcb->version & 0xf);
		return;
	}
	fprintf(out, "DCB table at 0x%04x version %d.%d", dcb->offset, dcb->version >> 4, dcb->version & 0xf);
	if (dcb->version >= 0x30)
		fprintf(out, " unk16 0x%02x", dcb->unk16);
	fprintf(out, "\n");
	envy_bios_dump_hex(bios, out, dcb->offset, dcb->hlen, mask);
	int i;
	for (i = 0; i < dcb->entriesnum; i++) {
		struct envy_bios_dcb_entry *entry = &dcb->entries[i];
		if (entry->type != ENVY_BIOS_DCB_UNUSED || mask & ENVY_BIOS_PRINT_UNUSED) {
			fprintf(out, "DCB %d:", i);
			if (dcb->version >= 0x14) {
				const char *typename = find_enum(dcb_types, entry->type);
				const char *lvdsinfo = find_enum(dcb_lvds_info_types, entry->lvds_info);
				fprintf(out, " type %d [%s]", entry->type, typename);
				if (entry->type != ENVY_BIOS_DCB_EOL && entry->type != ENVY_BIOS_DCB_UNUSED) {
					if (entry->i2c != 15)
						fprintf(out, " I2C %d", entry->i2c);
					int j;
					if (entry->heads) {
						fprintf(out, " heads");
						for (j = 0; j < 4; j++)
							if (entry->heads & 1 << j)
								fprintf(out, " %d", j);
					}
					if (entry->unk00_4)
						fprintf(out, " unk00_4 %d", entry->unk00_4);
					if (dcb->version >= 0x30 && bios->conn.entries)
						fprintf(out, " CONN %d [0x%02x]", entry->conn, bios->conn.entries[entry->conn].type);
					else if (entry->unk01_4)
						fprintf(out, " unk01_4 %d", entry->unk01_4);
					fprintf(out, " conntag %d", entry->conntag);
					switch (entry->loc) {
						case 0:
							fprintf(out, " LOCAL");
							break;
						case 1:
							fprintf(out, " EXT %d", entry->ext_addr);
							break;
						default:
							fprintf(out, " unk loc %d", entry->loc);
							break;
					}
					if (entry->ext_addr && entry->loc != 1)
						fprintf(out, " unk_ext_addr %d", entry->ext_addr);
					if (entry->unk02_6)
						fprintf(out, " unk02_6 %d", entry->unk02_6);
					if (entry->or) {
						fprintf(out, " OR");
						for (j = 0; j < 4; j++)
							if (entry->or & 1 << j)
								fprintf(out, " %d", j);
					}
					if (entry->unk03_4)
						fprintf(out, " unk03_4 %d", entry->unk03_4);
					if ((entry->type == ENVY_BIOS_DCB_LVDS || entry->type == ENVY_BIOS_DCB_TMDS || entry->type == ENVY_BIOS_DCB_DP) && dcb->version >= 0x40) {
						fprintf(out, " links");
						for (j = 0; j < 2; j++)
							if (entry->links & 1 << j)
								fprintf(out, " %d", j);
					}
					if (entry->maxfreq)
						fprintf(out, " maxfreq %dkHz", entry->maxfreq);
					if (entry->tmds_hdmi)
						fprintf(out, " HDMI");
					if (entry->type == ENVY_BIOS_DCB_LVDS) {
						fprintf(out, " INFO %s", lvdsinfo);
						fprintf(out, " POWER %d", entry->lvds_pscript);
					}
					if (entry->type == ENVY_BIOS_DCB_DP) {
						switch (entry->dp_bw) {
							case 0:
								fprintf(out, " BW 162000");
								break;
							case 1:
								fprintf(out, " BW 270000");
								break;
							default:
								fprintf(out, " BW UNK%d", entry->dp_bw);
								break;
						}
						fprintf(out, " lanes");
						for (j = 0; j < 4; j++)
							if (entry->dp_lanes & 1 << j)
								fprintf(out, " %d", j);
					}
					if (entry->unk04)
						fprintf(out, " unk04 0x%02x", entry->unk04);
					if (entry->unk05)
						fprintf(out, " unk05 0x%02x", entry->unk05);
					if (entry->unk06)
						fprintf(out, " unk06 0x%02x", entry->unk06);
					if (entry->unk07)
						fprintf(out, " unk07 0x%02x", entry->unk07);
				}
			} else {
				/* XXX */
			}
			fprintf(out, "\n");
		}
		envy_bios_dump_hex(bios, out, entry->offset, dcb->rlen, mask);
	}
	fprintf(out, "\n");
	if (bios->dev_rec_offset) {
		fprintf(out, "DEV_REC at 0x%04x\n", bios->dev_rec_offset);
		envy_bios_dump_hex(bios, out, bios->dev_rec_offset, 7, mask);
		fprintf(out, "\n");
	}
	if (dcb->rdcb_valid) {
		fprintf(out, "RDCB table at 0x%04x version %d.%d", dcb->offset, dcb->rdcb_version >> 4, dcb->rdcb_version & 0xf);
		if (dcb->tvdac0_present) {
			fprintf(out, " TVDAC0 line %d", dcb->tvdac0_line);
			if (dcb->tvdac0_neg)
				fprintf(out, " NEG");
		}
		if (dcb->rdcb_unk04_0)
			fprintf(out, " unk04_0 %d", dcb->rdcb_unk04_0);
		if (dcb->rdcb_unk05_2)
			fprintf(out, " unk05_2 %d", dcb->rdcb_unk05_2);
		int j;
		for (j = 0; j < 7; j++) {
			if (dcb->rdcb_unk06[j])
				fprintf(out, " unk0x%02x 0x%02x", j+6, dcb->rdcb_unk06[j]);
		}
		if (dcb->rdcb_unk0d)
			fprintf(out, " unk0d 0x%02x", dcb->rdcb_unk0d);
		for (j = 0; j < 13; j++) {
			if (dcb->rdcb_unk10[j])
				fprintf(out, " unk0x%02x 0x%02x", j+0x10, dcb->rdcb_unk10[j]);
		}
		fprintf(out, "\n");
		envy_bios_dump_hex(bios, out, dcb->offset - dcb->rdcb_len, dcb->rdcb_len, mask);
		fprintf(out, "\n");
	}
}

void envy_bios_print_odcb (struct envy_bios *bios, FILE *out, unsigned mask) {
	if (bios->odcb_offset) {
		fprintf(out, "ODCB table at 0x%04x\n", bios->odcb_offset);
		envy_bios_dump_hex(bios, out, bios->odcb_offset, 0x80, mask);
		fprintf(out, "\n");
	}
}
