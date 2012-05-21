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

int envy_bios_parse_gunk (struct envy_bios *bios);

int envy_bios_parse_gpio (struct envy_bios *bios) {
	struct envy_bios_gpio *gpio = &bios->gpio;
	int err = 0;
	if (!gpio->offset)
		return 0;
	err |= bios_u8(bios, gpio->offset, &gpio->version);
	err |= bios_u8(bios, gpio->offset+1, &gpio->hlen);
	err |= bios_u8(bios, gpio->offset+2, &gpio->entriesnum);
	err |= bios_u8(bios, gpio->offset+3, &gpio->rlen);
	if (err)
		return -EFAULT;
	int wanthlen = 0;
	int wantrlen = 0;
	switch (gpio->version) {
		case 0x30:
			wanthlen = 4;
			wantrlen = 2;
			break;
		case 0x31:
			wanthlen = 6;
			wantrlen = 2;
			break;
		case 0x40:
			wanthlen = 6;
			wantrlen = 4;
			break;
		case 0x41:
			wanthlen = 6;
			wantrlen = 5;
			break;
		default:
			ENVY_BIOS_ERR("Unknown GPIO table version %x.%x\n", bios->gpio.version >> 4, bios->gpio.version & 0xf);
			return -EINVAL;
	}
	if (gpio->hlen < wanthlen) {
		ENVY_BIOS_ERR("GPIO table header too short [%d < %d]\n", gpio->hlen, wanthlen);
		return -EINVAL;
	}
	if (gpio->rlen < wantrlen) {
		ENVY_BIOS_ERR("GPIO table record too short [%d < %d]\n", gpio->rlen, wantrlen);
		return -EINVAL;
	}
	if (gpio->hlen > wanthlen) {
		ENVY_BIOS_WARN("GPIO table header longer than expected [%d > %d]\n", gpio->hlen, wanthlen);
	}
	if (gpio->rlen > wantrlen) {
		ENVY_BIOS_WARN("GPIO table record longer than expected [%d > %d]\n", gpio->rlen, wantrlen);
	}
	if (gpio->version >= 0x31) {
		err |= bios_u16(bios, gpio->offset+4, &gpio->gunk.offset);
		if (err)
			return -EFAULT;
		if (envy_bios_parse_gunk(bios))
			ENVY_BIOS_ERR("Failed to parse GUNK table at %04x version %x.%x\n", bios->gpio.gunk.offset, bios->gpio.gunk.version >> 4, bios->gpio.gunk.version & 0xf);
	}
	gpio->entries = calloc(gpio->entriesnum, sizeof *gpio->entries);
	if (!gpio->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < gpio->entriesnum; i++) {
		uint16_t eoff = gpio->offset + gpio->hlen + gpio->rlen * i;
		struct envy_bios_gpio_entry *entry = &gpio->entries[i];
		uint8_t bytes[5];
		int j;
		for (j = 0; j < 5 && j < gpio->rlen; j++) {
			err |= bios_u8(bios, eoff+j, &bytes[j]);
			if (err)
				return -EFAULT;
		}
		uint16_t val;
		switch (gpio->version) {
			case 0x30:
			case 0x31:
				val = bytes[0] | bytes[1] << 8;
				entry->line = val & 0x1f;
				entry->tag = val >> 5 & 0x3f;
				entry->log[0] = val >> 11 & 3;
				entry->log[1] = val >> 13 & 3;
				entry->param = val >> 15 & 1;
				if (entry->tag == 0x3f)
					entry->tag = 0xff;
				break;
			case 0x40:
				entry->line = bytes[0] & 0x1f;
				entry->unk40_0 = bytes[0] >> 5 & 7;
				entry->tag = bytes[1];
				entry->unk40_2 = bytes[2];
				entry->def = bytes[3] & 1;
				entry->mode = bytes[3] >> 1 & 3;
				entry->log[0] = bytes[3] >> 3 & 3;
				entry->log[1] = bytes[3] >> 5 & 3;
				entry->param = bytes[3] >> 7 & 1;
				break;
			case 0x41:
				entry->line = bytes[0] & 0x1f;
				entry->unk40_0 = bytes[0] >> 5 & 3;
				entry->def = bytes[0] >> 7 & 1;
				entry->tag = bytes[1];
				entry->spec41 = bytes[2];
				entry->unk41_3_0 = bytes[3] & 0x1f;
				entry->unk41_3_1 = bytes[3] >> 5 & 3;
				entry->param = bytes[3] >> 7 & 1;
				entry->unk41_4 = bytes[4] & 0xf;
				entry->log[0] = bytes[4] >> 4 & 3;
				entry->log[1] = bytes[4] >> 6 & 3;
				break;
			default:
				abort();
		}
	}
	gpio->valid = 1;
	return 0;
}

static struct enum_val gpio_tags[] = {
	{ ENVY_BIOS_GPIO_PANEL_BACKLIGHT_ON,	"PANEL_BACKLIGHT_ON" },
	{ ENVY_BIOS_GPIO_PANEL_POWER,		"PANEL_POWER" },
	{ ENVY_BIOS_GPIO_PANEL_BACKLIGHT_LEVEL,	"PANEL_BACKLIGHT_LEVEL" },
	{ ENVY_BIOS_GPIO_TVDAC_0,		"TVDAC_0" },
	{ ENVY_BIOS_GPIO_TVDAC_1,		"TVDAC_1" },
	{ ENVY_BIOS_GPIO_FAN_CONTROL,		"FAN_CONTROL" },
	{ ENVY_BIOS_GPIO_FAN_SENSE,		"FAN_SENSE" },
	{ ENVY_BIOS_GPIO_VID_0,			"VID_0" },
	{ ENVY_BIOS_GPIO_VID_1,			"VID_1" },
	{ ENVY_BIOS_GPIO_VID_2,			"VID_2" },
	{ ENVY_BIOS_GPIO_VID_3,			"VID_3" },
	{ ENVY_BIOS_GPIO_VID_4,			"VID_4" },
	{ ENVY_BIOS_GPIO_HPD_0,			"HPD_0" },
	{ ENVY_BIOS_GPIO_HPD_1,			"HPD_1" },
	{ ENVY_BIOS_GPIO_HPD_2,			"HPD_2" },
	{ ENVY_BIOS_GPIO_HPD_3,			"HPD_3" },
	{ ENVY_BIOS_GPIO_HPD_4,			"HPD_4" },
	{ ENVY_BIOS_GPIO_HPD_5,			"HPD_5" },
	{ ENVY_BIOS_GPIO_HPD_6,			"HPD_6" },
	{ ENVY_BIOS_GPIO_ATX_POWER_BAD,		"ATX_POWER_BAD" },
	{ ENVY_BIOS_GPIO_ATX_POWER_BAD_ALT,	"ATX_POWER_BAD_ALT" },
	{ ENVY_BIOS_GPIO_SLI_SENSE_0,		"SLI_SENSE_0" },
	{ ENVY_BIOS_GPIO_SLI_SENSE_1,		"SLI_SENSE_1" },
	{ ENVY_BIOS_GPIO_SLI_SENSE_1_ALT,	"SLI_SENSE_1_ALT" },
	{ ENVY_BIOS_GPIO_UNUSED,		"UNUSED" },
	{ 0 }
};

static struct enum_val gpio_spec41[] = {
	{ 0x40, "SLI_SENSE_0" },
	{ 0x41, "SLI_SENSE_1" },
	/* 0x48 seen - tag 0x0f */
	/* 0x58 seen - tag 0x23 */
	{ 0x5c, "NVIO_PWM0" },
	{ 0x80,	"SOR0_PANEL_BACKLIGHT_LEVEL" },
	{ 0x81,	"SOR0_PANEL_POWER" },
	{ 0x82,	"SOR0_PANEL_BACKLIGHT_ON" },
	{ 0 },
};

void envy_bios_print_gunk (struct envy_bios *bios, FILE *out);

void envy_bios_print_gpio (struct envy_bios *bios, FILE *out) {
	struct envy_bios_gpio *gpio = &bios->gpio;
	if (!bios->gpio.offset)
		return;
	if (!bios->gpio.valid) {
		fprintf(out, "Failed to parse GPIO table at %04x version %x.%x\n\n", bios->gpio.offset, bios->gpio.version >> 4, bios->gpio.version & 0xf);
		return;
	}
	fprintf(out, "GPIO table at %04x version %x.%x\n", bios->gpio.offset, bios->gpio.version >> 4, bios->gpio.version & 0xf);
	envy_bios_dump_hex(bios, out, bios->gpio.offset, bios->gpio.hlen);
	int i;
	for (i = 0; i < gpio->entriesnum; i++) {
		uint16_t eoff = gpio->offset + gpio->hlen + gpio->rlen * i;
		struct envy_bios_gpio_entry *entry = &gpio->entries[i];
		const char *tagname = find_enum(gpio_tags, entry->tag);
		const char *specname = find_enum(gpio_spec41, entry->spec41);
		fprintf(out, "GPIO %d:", i);
		fprintf(out, " line %d", entry->line);
		fprintf(out, " tag 0x%02x [%s]", entry->tag, tagname);
		if (entry->log[0] == 0 && entry->log[1] == 1) {
			fprintf(out, " OUT");
		} else if (entry->log[0] == 1 && entry->log[1] == 0) {
			fprintf(out, " OUT NEG");
		} else if (entry->log[0] == 2 && entry->log[1] == 3) {
			fprintf(out, " IN");
		} else if (entry->log[0] == 3 && entry->log[1] == 2) {
			fprintf(out, " IN NEG");
		} else {
			fprintf(out, " LOG %d/%d", entry->log[0], entry->log[1]);
		}
		if (entry->mode == 0) {
		} else if (entry->mode == 1) {
			fprintf(out, " SPEC NVIO");
		} else if (entry->mode == 2) {
			fprintf(out, " SPEC SOR");
		} else {
			fprintf(out, " SPEC %d [???]", entry->mode);
		}
		if (entry->tag == ENVY_BIOS_GPIO_FAN_CONTROL
			|| entry->tag == ENVY_BIOS_GPIO_PANEL_BACKLIGHT_LEVEL) {
			fprintf(out, " %s", entry->param?"PWM":"TOGGLE");
		} else if (entry->param) {
			fprintf(out, " param 1");
		}
		if (entry->unk40_0)
			fprintf(out, " unk40_0 %d", entry->unk40_0);
		if (entry->unk40_2)
			fprintf(out, " unk40_2 0x%02x", entry->unk40_2);
		if (entry->spec41)
			fprintf(out, " SPEC 0x%02x [%s]", entry->spec41, specname);
		if (gpio->version == 0x41 && entry->unk41_4 != 15)
			fprintf(out, " unk41_4 %d", entry->unk41_4);
		if (entry->unk41_3_0)
			fprintf(out, " unk41_line %d", entry->unk41_3_0 + 11);
		if (entry->unk41_3_1)
			fprintf(out, " unk41_3_1 %d", entry->unk41_3_1);
		printf("\n");
		envy_bios_dump_hex(bios, out, eoff, gpio->rlen);
	}
	fprintf(out, "\n");
	if (bios->gpio.version >= 0x31)
		envy_bios_print_gunk(bios, out);
	fprintf(out, "\n");
}

int envy_bios_parse_subgunk (struct envy_bios *bios, struct envy_bios_subgunk *subgunk) {
	if (!subgunk->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, subgunk->offset, &subgunk->version);
	err |= bios_u8(bios, subgunk->offset+1, &subgunk->hlen);
	err |= bios_u8(bios, subgunk->offset+2, &subgunk->entriesnum);
	err |= bios_u8(bios, subgunk->offset+3, &subgunk->rlen);
	if (err)
		return -EFAULT;
	int wanthlen = 0;
	int wantrlen = 0;
	if (subgunk->version != bios->gpio.gunk.version)
		ENVY_BIOS_WARN("SUBGUNK version mismatch with GUNK\n");
	switch (subgunk->version) {
		case 0x30:
			wanthlen = 7;
			wantrlen = 2;
			break;
		case 0x40:
			wanthlen = 7;
			if (bios->gpio.version == 0x41)
				wantrlen = 5;
			else if (subgunk->rlen == 4)
				wantrlen = 4;
			else {
				ENVY_BIOS_WARN("SUBGUNK version 4.0 with 2-byte entries!\n");
				wantrlen = 2; /* XXX: HACK HACK HACK */
			}
			break;
		default:
			ENVY_BIOS_ERR("Unknown SUBGUNK table version %x.%x\n", subgunk->version >> 4, subgunk->version & 0xf);
			return -EINVAL;
	}
	if (subgunk->hlen < wanthlen) {
		ENVY_BIOS_ERR("SUBGUNK table header too short [%d < %d]\n", subgunk->hlen, wanthlen);
		return -EINVAL;
	}
	if (subgunk->rlen < wantrlen) {
		ENVY_BIOS_ERR("SUBGUNK table record too short [%d < %d]\n", subgunk->rlen, wantrlen);
		return -EINVAL;
	}
	if (subgunk->hlen > wanthlen) {
		ENVY_BIOS_WARN("SUBGUNK table header longer than expected [%d > %d]\n", subgunk->hlen, wanthlen);
	}
	if (subgunk->rlen > wantrlen) {
		ENVY_BIOS_WARN("SUBGUNK table record longer than expected [%d > %d]\n", subgunk->rlen, wantrlen);
	}
	/* XXX */
	/*
	subgunk->entries = calloc(gunk->entriesnum, sizeof *gunk->entries);
	if (!subgunk->entries)
		return -ENOMEM;
		*/
	int i;
	for (i = 0; i < subgunk->entriesnum; i++) {
		uint16_t eoff = subgunk->offset + subgunk->hlen + subgunk->rlen * i;
//		struct envy_bios_subgunk *subgunk = &subgunk->entries[i];
		/* XXX */
	}
	subgunk->valid = 1;
	return 0;
}

int envy_bios_parse_gunk (struct envy_bios *bios) {
	struct envy_bios_gunk *gunk = &bios->gpio.gunk;
	int err = 0;
	if (!gunk->offset)
		return 0;
	err |= bios_u8(bios, gunk->offset, &gunk->version);
	err |= bios_u8(bios, gunk->offset+1, &gunk->hlen);
	err |= bios_u8(bios, gunk->offset+2, &gunk->entriesnum);
	err |= bios_u8(bios, gunk->offset+3, &gunk->rlen);
	if (err)
		return -EFAULT;
	int wanthlen = 0;
	int wantrlen = 0;
	if (gunk->version != (bios->gpio.version & 0xf0))
		ENVY_BIOS_WARN("GUNK version mismatch with GPIO\n");
	switch (gunk->version) {
		case 0x30:
		case 0x40:
			wanthlen = 4;
			wantrlen = 2;
			break;
		default:
			ENVY_BIOS_ERR("Unknown GUNK table version %x.%x\n", gunk->version >> 4, gunk->version & 0xf);
			return -EINVAL;
	}
	if (gunk->hlen < wanthlen) {
		ENVY_BIOS_ERR("GUNK table header too short [%d < %d]\n", gunk->hlen, wanthlen);
		return -EINVAL;
	}
	if (gunk->rlen < wantrlen) {
		ENVY_BIOS_ERR("GUNK table record too short [%d < %d]\n", gunk->rlen, wantrlen);
		return -EINVAL;
	}
	if (gunk->hlen > wanthlen) {
		ENVY_BIOS_WARN("GUNK table header longer than expected [%d > %d]\n", gunk->hlen, wanthlen);
	}
	if (gunk->rlen > wantrlen) {
		ENVY_BIOS_WARN("GUNK table record longer than expected [%d > %d]\n", gunk->rlen, wantrlen);
	}
	gunk->entries = calloc(gunk->entriesnum, sizeof *gunk->entries);
	if (!gunk->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < gunk->entriesnum; i++) {
		uint16_t eoff = gunk->offset + gunk->hlen + gunk->rlen * i;
		struct envy_bios_subgunk *subgunk = &gunk->entries[i];
		err |= bios_u16(bios, eoff, &subgunk->offset);
		if (err)
			return -EFAULT;
		if (envy_bios_parse_subgunk(bios, subgunk))
			ENVY_BIOS_ERR("Failed to parse SUBGUNK table at %04x version %x.%x\n", subgunk->offset, subgunk->version >> 4, subgunk->version & 0xf);
	}
	gunk->valid = 1;
	return 0;
}

void envy_bios_print_subgunk (struct envy_bios *bios, FILE *out, struct envy_bios_subgunk *subgunk, int idx) {
	if (!subgunk->offset)
		return;
	if (!subgunk->valid) {
		fprintf(out, "Failed to parse SUBGUNK table at %04x version %x.%x\n\n", subgunk->offset, subgunk->version >> 4, subgunk->version & 0xf);
		return;
	}
	fprintf(out, "SUBGUNK table %d at %04x version %x.%x\n", idx, subgunk->offset, subgunk->version >> 4, subgunk->version & 0xf);
	envy_bios_dump_hex(bios, out, subgunk->offset, subgunk->hlen);
	int i;
	for (i = 0; i < subgunk->entriesnum; i++) {
		uint16_t eoff = subgunk->offset + subgunk->hlen + subgunk->rlen * i;
		envy_bios_dump_hex(bios, out, eoff, subgunk->rlen);
	}
	printf("\n");
}

void envy_bios_print_gunk (struct envy_bios *bios, FILE *out) {
	struct envy_bios_gunk *gunk = &bios->gpio.gunk;
	if (!gunk->offset)
		return;
	if (!gunk->valid) {
		fprintf(out, "Failed to parse GUNK table at %04x version %x.%x\n\n", gunk->offset, gunk->version >> 4, gunk->version & 0xf);
		return;
	}
	fprintf(out, "GUNK table at %04x version %x.%x, %d subtables\n", gunk->offset, gunk->version >> 4, gunk->version & 0xf, gunk->entriesnum);
	envy_bios_dump_hex(bios, out, gunk->offset, gunk->hlen + gunk->entriesnum * gunk->rlen);
	printf("\n");
	int i;
	for (i = 0; i < gunk->entriesnum; i++)
		envy_bios_print_subgunk(bios, out, &gunk->entries[i], i);
}
