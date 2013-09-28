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

int envy_bios_parse_inputdev (struct envy_bios *bios) {
	struct envy_bios_inputdev *inputdev = &bios->inputdev;
	if (!inputdev->offset)
		return 0;
	int err = 0;
	int wanthlen = 4;
	int wantrlen = 1;
	if (bios->dcb.version >= 0x30) {
		err |= bios_u8(bios, inputdev->offset, &inputdev->version);
		err |= bios_u8(bios, inputdev->offset+1, &inputdev->hlen);
		err |= bios_u8(bios, inputdev->offset+2, &inputdev->entriesnum);
		err |= bios_u8(bios, inputdev->offset+3, &inputdev->rlen);
	} else {
		inputdev->version = bios->dcb.version;
		wanthlen = inputdev->hlen = 0;
		inputdev->entriesnum = 8;
		inputdev->rlen = 1;
	}
	if (err)
		return -EFAULT;
	envy_bios_block(bios, inputdev->offset, inputdev->hlen + inputdev->rlen * inputdev->entriesnum, "INPUTDEV", -1);
	switch (inputdev->version) {
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x30:
		case 0x40:
			break;
		default:
			ENVY_BIOS_ERR("Unknown INPUTDEV table version %d.%d\n", inputdev->version >> 4, inputdev->version & 0xf);
			return -EINVAL;
	}
	if (inputdev->hlen < wanthlen) {
		ENVY_BIOS_ERR("INPUTDEV table header too short [%d < %d]\n", inputdev->hlen, wanthlen);
		return -EINVAL;
	}
	if (inputdev->rlen < wantrlen) {
		ENVY_BIOS_ERR("INPUTDEV table record too short [%d < %d]\n", inputdev->rlen, wantrlen);
		return -EINVAL;
	}
	if (inputdev->hlen > wanthlen) {
		ENVY_BIOS_WARN("INPUTDEV table header longer than expected [%d > %d]\n", inputdev->hlen, wanthlen);
	}
	if (inputdev->rlen > wantrlen) {
		ENVY_BIOS_WARN("INPUTDEV table record longer than expected [%d > %d]\n", inputdev->rlen, wantrlen);
	}
	inputdev->entries = calloc(inputdev->entriesnum, sizeof *inputdev->entries);
	if (!inputdev->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < inputdev->entriesnum; i++) {
		struct envy_bios_inputdev_entry *entry = &inputdev->entries[i];
		entry->offset = inputdev->offset + inputdev->hlen + inputdev->rlen * i;
		err |= bios_u8(bios, entry->offset, &entry->entry);
		if (err)
			return -EFAULT;
	}
	inputdev->valid = 1;
	return 0;
}

static const char *dump_inputdev_type(unsigned type) {
	switch (type) {
	case 0: return "VCR";
	case 1: return "TV";
	default: return "UNK";
	}
}

static const char *dump_inputdev_vtype(unsigned type) {
	switch (type) {
	case 0: return "CVBS";
	case 1: return "Tuner";
	case 2: return "S-Video";
	default: return "UNK";
	}
}

void envy_bios_print_inputdev (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_inputdev *inputdev = &bios->inputdev;
	if (!inputdev->offset || !(mask & ENVY_BIOS_PRINT_DUNK))
		return;
	if (!inputdev->valid) {
		fprintf(out, "Failed to parse INPUTDEV table at 0x%04x version %d.%d\n\n", inputdev->offset, inputdev->version >> 4, inputdev->version & 0xf);
		return;
	}
	fprintf(out, "INPUTDEV table at 0x%04x version %d.%d\n", inputdev->offset, inputdev->version >> 4, inputdev->version & 0xf);
	envy_bios_dump_hex(bios, out, inputdev->offset, inputdev->hlen, mask);
	int i;
	for (i = 0; i < inputdev->entriesnum; i++) {
		struct envy_bios_inputdev_entry *entry = &inputdev->entries[i];
		if ((entry->entry & 0xf) != 0x0f || (mask & ENVY_BIOS_PRINT_UNUSED)) {
			fprintf(out, "INPUTDEV %d: mode: %x, type: %s, vtype: %s\n", i,
				entry->entry & 0xf,
				dump_inputdev_type((entry->entry >> 4) & 3),
				dump_inputdev_vtype((entry->entry >> 6) & 3));
		}
		envy_bios_dump_hex(bios, out, entry->offset, inputdev->rlen, mask);
	}
	fprintf(out, "\n");
}


int envy_bios_parse_cinema (struct envy_bios *bios) {
	struct envy_bios_cinema *cinema = &bios->cinema;
	if (!cinema->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, cinema->offset, &cinema->version);
	err |= bios_u8(bios, cinema->offset+1, &cinema->hlen);
	int i;
	for (i = 0; i < 10; i++)
		err |= bios_u8(bios, cinema->offset+2+i, &cinema->unk02[i]);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, cinema->offset, cinema->hlen, "CINEMA", -1);
	int wanthlen = 0;
	switch (cinema->version) {
		case 0x30:
		case 0x31:
		case 0x40:
			wanthlen = 12;
			break;
		default:
			ENVY_BIOS_ERR("Unknown CINEMA table version %d.%d\n", cinema->version >> 4, cinema->version & 0xf);
			return -EINVAL;
	}
	if (cinema->hlen < wanthlen) {
		ENVY_BIOS_ERR("CINEMA table header too short [%d < %d]\n", cinema->hlen, wanthlen);
		return -EINVAL;
	}
	if (cinema->hlen > wanthlen) {
		ENVY_BIOS_WARN("CINEMA table header longer than expected [%d > %d]\n", cinema->hlen, wanthlen);
	}
	cinema->valid = 1;
	return 0;
}

void envy_bios_print_cinema (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_cinema *cinema = &bios->cinema;
	if (!cinema->offset || !(mask & ENVY_BIOS_PRINT_DUNK))
		return;
	if (!cinema->valid) {
		fprintf(out, "Failed to parse CINEMA table at 0x%04x version %d.%d\n\n", cinema->offset, cinema->version >> 4, cinema->version & 0xf);
		return;
	}
	fprintf(out, "CINEMA table at 0x%04x version %d.%d data", cinema->offset, cinema->version >> 4, cinema->version & 0xf);
	int i;
	for (i = 0; i < 10; i++)
		fprintf(out, " 0x%02x", cinema->unk02[i]);
	fprintf(out, "\n");
	envy_bios_dump_hex(bios, out, cinema->offset, cinema->hlen, mask);
	fprintf(out, "\n");
}


int envy_bios_parse_spreadspectrum (struct envy_bios *bios) {
	struct envy_bios_spreadspectrum *spreadspectrum = &bios->spreadspectrum;
	if (!spreadspectrum->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, spreadspectrum->offset, &spreadspectrum->version);
	err |= bios_u8(bios, spreadspectrum->offset+1, &spreadspectrum->hlen);
	err |= bios_u8(bios, spreadspectrum->offset+2, &spreadspectrum->entriesnum);
	err |= bios_u8(bios, spreadspectrum->offset+3, &spreadspectrum->rlen);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, spreadspectrum->offset, spreadspectrum->hlen + spreadspectrum->rlen * spreadspectrum->entriesnum, "SPREADSPECTRUM", -1);
	int wanthlen = 0;
	int wantrlen = 0;
	switch (spreadspectrum->version) {
		case 0x31:
		case 0x40:
			wanthlen = 10;
			wantrlen = 1;
			err |= bios_u8(bios, spreadspectrum->offset+4, &spreadspectrum->unk04);
			err |= bios_u8(bios, spreadspectrum->offset+5, &spreadspectrum->unk05);
			err |= bios_u8(bios, spreadspectrum->offset+6, &spreadspectrum->unk06);
			err |= bios_u8(bios, spreadspectrum->offset+7, &spreadspectrum->unk07);
			err |= bios_u8(bios, spreadspectrum->offset+8, &spreadspectrum->unk08);
			err |= bios_u8(bios, spreadspectrum->offset+9, &spreadspectrum->unk09);
			if (err)
				return -EFAULT;
			break;
		case 0x41:
			wanthlen = 5;
			wantrlen = 1;
			if (spreadspectrum->rlen >= 2)
				wantrlen = 2;
			err |= bios_u8(bios, spreadspectrum->offset+4, &spreadspectrum->unk04);
			if (err)
				return -EFAULT;
			break;
		default:
			ENVY_BIOS_ERR("Unknown SPREADSPECTRUM table version %d.%d\n", spreadspectrum->version >> 4, spreadspectrum->version & 0xf);
			return -EINVAL;
	}
	if (spreadspectrum->hlen < wanthlen) {
		ENVY_BIOS_ERR("SPREADSPECTRUM table header too short [%d < %d]\n", spreadspectrum->hlen, wanthlen);
		return -EINVAL;
	}
	if (spreadspectrum->rlen < wantrlen) {
		ENVY_BIOS_ERR("SPREADSPECTRUM table record too short [%d < %d]\n", spreadspectrum->rlen, wantrlen);
		return -EINVAL;
	}
	if (spreadspectrum->hlen > wanthlen) {
		ENVY_BIOS_WARN("SPREADSPECTRUM table header longer than expected [%d > %d]\n", spreadspectrum->hlen, wanthlen);
	}
	if (spreadspectrum->rlen > wantrlen) {
		ENVY_BIOS_WARN("SPREADSPECTRUM table record longer than expected [%d > %d]\n", spreadspectrum->rlen, wantrlen);
	}
	spreadspectrum->entries = calloc(spreadspectrum->entriesnum, sizeof *spreadspectrum->entries);
	if (!spreadspectrum->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < spreadspectrum->entriesnum; i++) {
		struct envy_bios_spreadspectrum_entry *entry = &spreadspectrum->entries[i];
		entry->offset = spreadspectrum->offset + spreadspectrum->hlen + spreadspectrum->rlen * i;
		err |= bios_u8(bios, entry->offset+0, &entry->unk00);
		if (spreadspectrum->rlen >= 2)
			err |= bios_u8(bios, entry->offset+1, &entry->unk01);
		if (err)
			return -EFAULT;
	}
	spreadspectrum->valid = 1;
	return 0;
}

void envy_bios_print_spreadspectrum (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_spreadspectrum *spreadspectrum = &bios->spreadspectrum;
	if (!spreadspectrum->offset || !(mask & ENVY_BIOS_PRINT_DUNK))
		return;
	if (!spreadspectrum->valid) {
		fprintf(out, "Failed to parse SPREADSPECTRUM table at 0x%04x version %d.%d\n\n", spreadspectrum->offset, spreadspectrum->version >> 4, spreadspectrum->version & 0xf);
		return;
	}
	fprintf(out, "SPREADSPECTRUM table at 0x%04x version %d.%d", spreadspectrum->offset, spreadspectrum->version >> 4, spreadspectrum->version & 0xf);
	if (spreadspectrum->version < 0x41) {
		fprintf (out, " unk04 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", spreadspectrum->unk04, spreadspectrum->unk05, spreadspectrum->unk06, spreadspectrum->unk07, spreadspectrum->unk08, spreadspectrum->unk09);
	} else {
		fprintf (out, " unk04 0x%02x\n", spreadspectrum->unk04);
	}
	envy_bios_dump_hex(bios, out, spreadspectrum->offset, spreadspectrum->hlen, mask);
	int i;
	for (i = 0; i < spreadspectrum->entriesnum; i++) {
		unsigned int spread;
		struct envy_bios_spreadspectrum_entry *entry = &spreadspectrum->entries[i];
		envy_bios_dump_hex(bios, out, entry->offset, spreadspectrum->rlen, mask);
		if(!(entry->unk00 & 0x1)) {
			fprintf(out, "SPREADSPECTRUM %d: INVALID 0x%02x", i, entry->unk00);
						
			if (spreadspectrum->rlen >= 2)
				fprintf(out, " 0x%02x", entry->unk01);
		} else {
			fprintf(out, "SPREADSPECTRUM %d: ", i);
			fprintf(out, "idx(%1x) ", (entry->unk00 & 0x10) >> 4);
			switch(entry->unk00 & 0x6) {
			case 0x0:
				fprintf(out, "SRC_INTERNAL_0 ");
				break;
			case 0x2:
				fprintf(out, "SRC_INTERNAL_1 ");
				break;
			case 0x4:
				fprintf(out, "SRC_EXTERNAL ");
				break;
			case 0x6:
				fprintf(out, "SRC_PLL ");
				break;
			}
			spread = (entry->unk01 & 0x3f) * 5;
			fprintf(out, "SPREAD(%d.%02d\%) ", spread / 100, spread % 100);
			
			if(entry->unk01 & 0x70)
				fprintf(out, "DOWN");
			else
				fprintf(out, "CENTER");
		}
		fprintf(out, "\n");
	}
	fprintf(out, "\n");
}


int envy_bios_parse_hdtvtt (struct envy_bios *bios) {
	struct envy_bios_hdtvtt *hdtvtt = &bios->hdtvtt;
	if (!hdtvtt->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, hdtvtt->offset, &hdtvtt->version);
	err |= bios_u8(bios, hdtvtt->offset+1, &hdtvtt->hlen);
	err |= bios_u8(bios, hdtvtt->offset+2, &hdtvtt->entriesnum);
	err |= bios_u8(bios, hdtvtt->offset+3, &hdtvtt->rlen);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, hdtvtt->offset, hdtvtt->hlen + hdtvtt->rlen * hdtvtt->entriesnum, "HDTVTT", -1);
	int wanthlen = 0;
	int wantrlen = 0;
	switch (hdtvtt->version) {
		case 0x10:
		case 0x11:
			wanthlen = 4;
			if (hdtvtt->hlen >= 10)
				wanthlen = 10;
			wantrlen = 2;
			break;
		default:
			ENVY_BIOS_ERR("Unknown HDTVTT table version %d.%d\n", hdtvtt->version >> 4, hdtvtt->version & 0xf);
			return -EINVAL;
	}
	if (hdtvtt->hlen < wanthlen) {
		ENVY_BIOS_ERR("HDTVTT table header too short [%d < %d]\n", hdtvtt->hlen, wanthlen);
		return -EINVAL;
	}
	if (hdtvtt->rlen < wantrlen) {
		ENVY_BIOS_ERR("HDTVTT table record too short [%d < %d]\n", hdtvtt->rlen, wantrlen);
		return -EINVAL;
	}
	if (hdtvtt->hlen > wanthlen) {
		ENVY_BIOS_WARN("HDTVTT table header longer than expected [%d > %d]\n", hdtvtt->hlen, wanthlen);
	}
	if (hdtvtt->rlen > wantrlen) {
		ENVY_BIOS_WARN("HDTVTT table record longer than expected [%d > %d]\n", hdtvtt->rlen, wantrlen);
	}
	hdtvtt->entries = calloc(hdtvtt->entriesnum, sizeof *hdtvtt->entries);
	if (!hdtvtt->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < hdtvtt->entriesnum; i++) {
		struct envy_bios_hdtvtt_entry *entry = &hdtvtt->entries[i];
		entry->offset = hdtvtt->offset + hdtvtt->hlen + hdtvtt->rlen * i;
	}
	hdtvtt->valid = 1;
	return 0;
}

void envy_bios_print_hdtvtt (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_hdtvtt *hdtvtt = &bios->hdtvtt;
	if (!hdtvtt->offset || !(mask & ENVY_BIOS_PRINT_DUNK))
		return;
	if (!hdtvtt->valid) {
		fprintf(out, "Failed to parse HDTVTT table at 0x%04x version %d.%d\n\n", hdtvtt->offset, hdtvtt->version >> 4, hdtvtt->version & 0xf);
		return;
	}
	fprintf(out, "HDTVTT table at 0x%04x version %d.%d\n", hdtvtt->offset, hdtvtt->version >> 4, hdtvtt->version & 0xf);
	envy_bios_dump_hex(bios, out, hdtvtt->offset, hdtvtt->hlen, mask);
	int i;
	for (i = 0; i < hdtvtt->entriesnum; i++) {
		struct envy_bios_hdtvtt_entry *entry = &hdtvtt->entries[i];
		envy_bios_dump_hex(bios, out, entry->offset, hdtvtt->rlen, mask);
	}
	fprintf(out, "\n");
}
