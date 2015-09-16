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

int envy_bios_parse_xpiodir (struct envy_bios *bios);

int envy_bios_parse_gpio (struct envy_bios *bios) {
	struct envy_bios_gpio *gpio = &bios->gpio;
	int err = 0;
	if (!gpio->offset)
		return 0;
	err |= bios_u8(bios, gpio->offset, &gpio->version);
	if (gpio->version == 0x10) {
		gpio->hlen = 3;
		err |= bios_u8(bios, gpio->offset+1, &gpio->rlen);
		err |= bios_u8(bios, gpio->offset+2, &gpio->entriesnum);
	} else {
		err |= bios_u8(bios, gpio->offset+1, &gpio->hlen);
		err |= bios_u8(bios, gpio->offset+2, &gpio->entriesnum);
		err |= bios_u8(bios, gpio->offset+3, &gpio->rlen);
	}
	if (err)
		return -EFAULT;
	envy_bios_block(bios, gpio->offset, gpio->hlen + gpio->rlen * gpio->entriesnum, "GPIO", -1);
	int wanthlen = 0;
	int wantrlen = 0;
	switch (gpio->version) {
		case 0x10:
			wanthlen = 3;
			wantrlen = 2;
			break;
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
			ENVY_BIOS_ERR("Unknown GPIO table version %d.%d\n", bios->gpio.version >> 4, bios->gpio.version & 0xf);
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
		err |= bios_u16(bios, gpio->offset+4, &gpio->xpiodir.offset);
		if (err)
			return -EFAULT;
		if (envy_bios_parse_xpiodir(bios))
			ENVY_BIOS_ERR("Failed to parse XPIODIR table at 0x%04x version %d.%d\n", bios->gpio.xpiodir.offset, bios->gpio.xpiodir.version >> 4, bios->gpio.xpiodir.version & 0xf);
	}
	gpio->entries = calloc(gpio->entriesnum, sizeof *gpio->entries);
	if (!gpio->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < gpio->entriesnum; i++) {
		struct envy_bios_gpio_entry *entry = &gpio->entries[i];
		entry->offset = gpio->offset + gpio->hlen + gpio->rlen * i;
		uint8_t bytes[5] = { 0 };
		int j;
		for (j = 0; j < 5 && j < gpio->rlen; j++) {
			err |= bios_u8(bios, entry->offset+j, &bytes[j]);
			if (err)
				return -EFAULT;
		}
		uint16_t val;
		switch (gpio->version) {
			case 0x10:
			case 0x30:
			case 0x31:
				if (gpio->rlen < 2)
					return -EFAULT;
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
				if (gpio->rlen < 4)
					return -EFAULT;
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
				if (gpio->rlen < 5)
					return -EFAULT;
				entry->line = bytes[0] & 0x3f;
				entry->io = bytes[0] >> 6 & 1;
				entry->def = bytes[0] >> 7 & 1;
				entry->tag = bytes[1];
				entry->spec_out = bytes[2];
				entry->spec_in = bytes[3] & 0x1f;
				entry->gsync = bytes[3] >> 5 & 1;
				entry->reserved = bytes[3] >> 6 & 1;
				entry->param = bytes[3] >> 7 & 1;
				entry->lockpin = bytes[4] & 0xf;
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
	{ ENVY_BIOS_GPIO_PSR_FRAME_LOCK_A, 	"PSR_FRAME_LOCK_A", },
	{ ENVY_BIOS_GPIO_TVDAC_0,		"TVDAC_0" },
	{ ENVY_BIOS_GPIO_TVDAC_1,		"TVDAC_1" },
	{ ENVY_BIOS_GPIO_FAN_PWM,		"FAN_PWM" },
	{ ENVY_BIOS_GPIO_FAN_FAILSAFE_PWM,	"FAN_FAILSAFE_PWM" },
	{ ENVY_BIOS_GPIO_FAN_TACH,		"FAN_TACH" },
	{ ENVY_BIOS_GPIO_MEM_VOLTAGE,		"MEM_VOLTAGE" },
	{ ENVY_BIOS_GPIO_MEM_VREF,		"MEM_VREF" },
	{ ENVY_BIOS_GPIO_VID_0,			"VID_0" },
	{ ENVY_BIOS_GPIO_VID_1,			"VID_1" },
	{ ENVY_BIOS_GPIO_VID_2,			"VID_2" },
	{ ENVY_BIOS_GPIO_VID_3,			"VID_3" },
	{ ENVY_BIOS_GPIO_VID_4,			"VID_4" },
	{ ENVY_BIOS_GPIO_VID_5,			"VID_5" },
	{ ENVY_BIOS_GPIO_VID_6,			"VID_6" },
	{ ENVY_BIOS_GPIO_VID_7,			"VID_7" },
	{ ENVY_BIOS_GPIO_HPD_0,			"HPD_0" },
	{ ENVY_BIOS_GPIO_HPD_1,			"HPD_1" },
	{ ENVY_BIOS_GPIO_HPD_2,			"HPD_2" },
	{ ENVY_BIOS_GPIO_HPD_3,			"HPD_3" },
	{ ENVY_BIOS_GPIO_HPD_4,			"HPD_4" },
	{ ENVY_BIOS_GPIO_HPD_5,			"HPD_5" },
	{ ENVY_BIOS_GPIO_HPD_6,			"HPD_6" },
	{ ENVY_BIOS_GPIO_DP_EXT_0,		"DP_EXT_0" },
	{ ENVY_BIOS_GPIO_DP_EXT_1,		"DP_EXT_1" },
	{ ENVY_BIOS_GPIO_ATX_POWER_BAD,		"ATX_POWER_BAD" },
	{ ENVY_BIOS_GPIO_ATX_POWER_BAD_ALT,	"ATX_POWER_BAD_ALT" },
	{ ENVY_BIOS_GPIO_ATX_POWER_LOW,		"ATX_POWER_LOW" },
	{ ENVY_BIOS_GPIO_ATX_FORCE_LOW_PWR,	"ATX_FORCE_LOW_PWR" },
	{ ENVY_BIOS_GPIO_HW_PWR_SLOWDOWN,	"HW_PWR_SLOWDOWN" },
	{ ENVY_BIOS_GPIO_HW_SLOWDOWN_ENABLE,	"HW_SLOWDOWN_ENABLE", },
	{ ENVY_BIOS_GPIO_THERM_EVENT_DETECT,	"THERM_EVENT_DETECT" },
	{ ENVY_BIOS_GPIO_THERM_ALERT_OUT,	"THERM_ALERT_OUT" },
	{ ENVY_BIOS_GPIO_THERM_ALERT, 		"THERM_ALERT" },
	{ ENVY_BIOS_GPIO_THERM_SHUTDOWN,	"THERM_SHUTDOWN" },
	{ ENVY_BIOS_GPIO_SLI_SENSE_0,		"SLI_SENSE_0" },
	{ ENVY_BIOS_GPIO_SLI_SENSE_1,		"SLI_SENSE_1" },
	{ ENVY_BIOS_GPIO_SLI_SENSE_1_ALT,	"SLI_SENSE_1_ALT" },
	{ ENVY_BIOS_GPIO_VID_PWM,		"VID_PWM" },
	{ ENVY_BIOS_GPIO_SLI_LED_PWM,		"SLI_LED_PWM" },
	{ ENVY_BIOS_GPIO_MXM_3_0_PIN_26,	"MXM_PIN_26"},
	{ ENVY_BIOS_GPIO_MXM_3_0_PIN_28,	"MXM_PIN_28"},
	{ ENVY_BIOS_GPIO_MXM_3_0_PIN_30,	"MXM_PIN_30"},
	{ ENVY_BIOS_GPIO_FB_CLAMP_EC_JT_MEM_SRFSH,	"FB_CLAMP_EC_JT_MEM_SRFSH" },
	{ ENVY_BIOS_GPIO_FB_CLAMP_TOGGLE_REQ,	"FB_CLAMP_TOGGLE_REQ" },
	{ ENVY_BIOS_GPIO_SWAP_RDY_SYNC_A,	"SWAP_RDY_SYNC_A" },
	{ ENVY_BIOS_GPIO_STEREO_TOGGLE,		"STEREO_TOGGLE" },
	{ ENVY_BIOS_GPIO_FRAME_LOCK_DAUGHTER_INTRPT,	"FRAMELOCK_DAUGHTER_CARD_INTERRIPT"},
	{ ENVY_BIOS_GPIO_UNUSED,		"UNUSED" },
	{ 0 }
};

static struct enum_val gpio_spec_out[] = {
	{ 0x40, "NVIO_SLI_SENSE_0" },
	{ 0x41, "NVIO_SLI_SENSE_1" },
	/* 0x48 seen - tag 0x0f */
	/* 0x50 seen - tag 0x42 */
	{ 0x58, "NVIO_THERM_SHUTDOWN" },
	{ 0x59, "PWM_1" },
	{ 0x5c, "PWM_0" },
	{ 0x80,	"SOR0_PANEL_BACKLIGHT_LEVEL" },
	{ 0x81,	"SOR0_PANEL_POWER" },
	{ 0x82,	"SOR0_PANEL_BACKLIGHT_ON" },
	{ 0x84,	"SOR1_PANEL_BACKLIGHT_LEVEL" },
	{ 0x85,	"SOR1_PANEL_POWER" },
	{ 0x86,	"SOR1_PANEL_BACKLIGHT_ON" },
	{ 0x88,	"SOR2_PANEL_BACKLIGHT_LEVEL" },
	{ 0x89,	"SOR2_PANEL_POWER" },
	{ 0x8a,	"SOR2_PANEL_BACKLIGHT_ON" },
	{ 0x8c,	"SOR3_PANEL_BACKLIGHT_LEVEL" },
	{ 0x8d,	"SOR3_PANEL_POWER" },
	{ 0x8e,	"SOR3_PANEL_BACKLIGHT_ON" },
	{ 0 },
};

static struct enum_val gpio_spec_in[] = {
	{ 0x00, "AUXCH_HPD_0" },
	{ 0x01, "AUXCH_HPD_1" },
	{ 0x02, "AUXCH_HPD_2" },
	{ 0x03, "AUXCH_HPD_3" },

	{ 0x08, "NVIO_SLI_SENSE_0" },
	{ 0x09, "NVIO_SLI_SENSE_1" },

	/* 0x10 seen - tag 0x42 */

	{ 0x14, "PTHERM_INPUT_0" },
	{ 0x15, "PTHERM_INPUT_1" },
	{ 0x16, "PTHERM_INPUT_2" },
	{ 0x17, "FAN_TACH" },
	{ 0 },
};

void envy_bios_print_xpiodir (struct envy_bios *bios, FILE *out, unsigned mask);

void envy_bios_print_gpio (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_gpio *gpio = &bios->gpio;
	if (!bios->gpio.offset || !(mask & ENVY_BIOS_PRINT_GPIO))
		return;
	if (!bios->gpio.valid) {
		fprintf(out, "Failed to parse GPIO table at 0x%04x version %d.%d\n\n", bios->gpio.offset, bios->gpio.version >> 4, bios->gpio.version & 0xf);
		return;
	}
	fprintf(out, "GPIO table at 0x%04x version %d.%d\n", bios->gpio.offset, bios->gpio.version >> 4, bios->gpio.version & 0xf);
	envy_bios_dump_hex(bios, out, bios->gpio.offset, bios->gpio.hlen, mask);
	int i;
	for (i = 0; i < gpio->entriesnum; i++) {
		struct envy_bios_gpio_entry *entry = &gpio->entries[i];
		if (entry->tag != ENVY_BIOS_GPIO_UNUSED || mask & ENVY_BIOS_PRINT_UNUSED) {
			const char *tagname = find_enum(gpio_tags, entry->tag);
			const char *spec_out = find_enum(gpio_spec_out, entry->spec_out);
			const char *spec_in = find_enum(gpio_spec_in, entry->spec_in-1);
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
			if (gpio->version >= 0x40)
				fprintf(out, " DEF %d", entry->def);
			if (entry->mode == 0) {
			} else if (entry->mode == 1) {
				fprintf(out, " SPEC NVIO");
			} else if (entry->mode == 2) {
				fprintf(out, " SPEC SOR");
			} else {
				fprintf(out, " SPEC %d [???]", entry->mode);
			}
			if (entry->tag == ENVY_BIOS_GPIO_FAN_PWM
				|| entry->tag == ENVY_BIOS_GPIO_PANEL_BACKLIGHT_LEVEL) {
				fprintf(out, " %s", entry->param?"HW":"SW");
			} else if (entry->param) {
				fprintf(out, " param 1");
			}
			if (entry->unk40_0)
				fprintf(out, " unk40_0 %d", entry->unk40_0);
			if (gpio->version == 0x41)
				fprintf(out, " gpio: %s", entry->io ? "lockpin" : "normal");
			if (entry->unk40_2)
				fprintf(out, " unk40_2 0x%02x", entry->unk40_2);
			if (entry->spec_out)
				fprintf(out, " SPEC_OUT 0x%02x [%s]", entry->spec_out, spec_out);
			if (gpio->version == 0x41 && entry->lockpin != 15)
				fprintf(out, " lockpin %d", entry->lockpin);
			if (entry->spec_in)
				fprintf(out, " SPEC_IN 0x%02x [%s]", entry->spec_in-1, spec_in);
			if (entry->gsync)
				fprintf(out, " gsync %d", entry->gsync);
			fprintf(out, "\n");
		}
		envy_bios_dump_hex(bios, out, entry->offset, gpio->rlen, mask);
	}
	fprintf(out, "\n");
	if (bios->gpio.version >= 0x31)
		envy_bios_print_xpiodir(bios, out, mask);
}

int envy_bios_parse_xpio (struct envy_bios *bios, struct envy_bios_xpio *xpio, int idx) {
	if (!xpio->offset)
		return 0;
	int err = 0;
	uint8_t byte2;
	err |= bios_u8(bios, xpio->offset, &xpio->version);
	err |= bios_u8(bios, xpio->offset+1, &xpio->hlen);
	err |= bios_u8(bios, xpio->offset+2, &xpio->entriesnum);
	err |= bios_u8(bios, xpio->offset+3, &xpio->rlen);
	err |= bios_u8(bios, xpio->offset+4, &xpio->type);
	err |= bios_u8(bios, xpio->offset+5, &xpio->addr);
	err |= bios_u8(bios, xpio->offset+6, &byte2);
	if (err)
		return -EFAULT;
	envy_bios_block(bios, xpio->offset, xpio->hlen + xpio->rlen * xpio->entriesnum, "XPIO", idx);
	xpio->unk02_0 = byte2 & 0xf;
	xpio->bus = byte2 >> 4 & 1;
	xpio->unk02_5 = byte2 >> 5;
	int wanthlen = 0;
	int wantrlen = 0;
	if (xpio->version != bios->gpio.xpiodir.version)
		ENVY_BIOS_WARN("XPIO version mismatch with XPIODIR\n");
	switch (xpio->version) {
		case 0x30:
			wanthlen = 7;
			wantrlen = 2;
			break;
		case 0x40:
			wanthlen = 7;
			wantrlen = 4;
			if (bios->gpio.version == 0x41)
				wantrlen = 5;
			else if (xpio->rlen == 2) {
				ENVY_BIOS_WARN("XPIO version 4.0 with 2-byte entries!\n");
				xpio->rlen = 4;
			}
			break;
		default:
			ENVY_BIOS_ERR("Unknown XPIO table version %d.%d\n", xpio->version >> 4, xpio->version & 0xf);
			return -EINVAL;
	}
	if (xpio->hlen < wanthlen) {
		ENVY_BIOS_ERR("XPIO table header too short [%d < %d]\n", xpio->hlen, wanthlen);
		return -EINVAL;
	}
	if (xpio->rlen < wantrlen) {
		ENVY_BIOS_ERR("XPIO table record too short [%d < %d]\n", xpio->rlen, wantrlen);
		return -EINVAL;
	}
	if (xpio->hlen > wanthlen) {
		ENVY_BIOS_WARN("XPIO table header longer than expected [%d > %d]\n", xpio->hlen, wanthlen);
	}
	if (xpio->rlen > wantrlen) {
		ENVY_BIOS_WARN("XPIO table record longer than expected [%d > %d]\n", xpio->rlen, wantrlen);
	}
	xpio->entries = calloc(xpio->entriesnum, sizeof *xpio->entries);
	if (!xpio->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < xpio->entriesnum; i++) {
		struct envy_bios_gpio_entry *entry = &xpio->entries[i];
		entry->offset = xpio->offset + xpio->hlen + xpio->rlen * i;
		/* note: structure is the same as GPIO, but tags are different, and some fields don't make sense */
		uint8_t bytes[5] = { 0 };
		int j;
		for (j = 0; j < 5 && j < xpio->rlen; j++) {
			err |= bios_u8(bios, entry->offset+j, &bytes[j]);
			if (err)
				return -EFAULT;
		}
		uint16_t val;
		/* because someone forgot to bump XPIO version number on 4.1... */
		switch (bios->gpio.version) {
			case 0x30:
			case 0x31:
				if (xpio->rlen < 2)
					return -EFAULT;
				val = bytes[0] | bytes[1] << 8;
				entry->line = val & 0x1f;
				entry->tag = val >> 5 & 0x3f;
				entry->log[0] = val >> 11 & 3;
				entry->log[1] = val >> 13 & 3;
				entry->param = val >> 15 & 1;
				break;
			case 0x40:
				if (xpio->rlen < 4)
					return -EFAULT;
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
				if (xpio->rlen < 5)
					return -EFAULT;
				entry->line = bytes[0] & 0x3f;
				entry->io = bytes[0] >> 6 & 1;
				entry->def = bytes[0] >> 7 & 1;
				entry->tag = bytes[1];
				entry->spec_out = bytes[2];
				entry->spec_in = bytes[3] & 0x1f;
				entry->gsync = bytes[3] >> 5 & 1;
				entry->reserved = bytes[3] >> 6 & 1;
				entry->param = bytes[3] >> 7 & 1;
				entry->lockpin = bytes[4] & 0xf;
				entry->log[0] = bytes[4] >> 4 & 3;
				entry->log[1] = bytes[4] >> 6 & 3;
				break;
			default:
				abort();
		}
	}
	xpio->valid = 1;
	return 0;
}

int envy_bios_parse_xpiodir (struct envy_bios *bios) {
	struct envy_bios_xpiodir *xpiodir = &bios->gpio.xpiodir;
	int err = 0;
	if (!xpiodir->offset)
		return 0;
	err |= bios_u8(bios, xpiodir->offset, &xpiodir->version);
	err |= bios_u8(bios, xpiodir->offset+1, &xpiodir->hlen);
	err |= bios_u8(bios, xpiodir->offset+2, &xpiodir->entriesnum);
	err |= bios_u8(bios, xpiodir->offset+3, &xpiodir->rlen);
	envy_bios_block(bios, xpiodir->offset, xpiodir->hlen + xpiodir->rlen * xpiodir->entriesnum, "XPIODIR", -1);
	if (err)
		return -EFAULT;
	int wanthlen = 0;
	int wantrlen = 0;
	if (xpiodir->version != (bios->gpio.version & 0xf0))
		ENVY_BIOS_WARN("XPIODIR version mismatch with GPIO\n");
	switch (xpiodir->version) {
		case 0x30:
		case 0x40:
			wanthlen = 4;
			wantrlen = 2;
			break;
		default:
			ENVY_BIOS_ERR("Unknown XPIODIR table version %d.%d\n", xpiodir->version >> 4, xpiodir->version & 0xf);
			return -EINVAL;
	}
	if (xpiodir->hlen < wanthlen) {
		ENVY_BIOS_ERR("XPIODIR table header too short [%d < %d]\n", xpiodir->hlen, wanthlen);
		return -EINVAL;
	}
	if (xpiodir->rlen < wantrlen) {
		ENVY_BIOS_ERR("XPIODIR table record too short [%d < %d]\n", xpiodir->rlen, wantrlen);
		return -EINVAL;
	}
	if (xpiodir->hlen > wanthlen) {
		ENVY_BIOS_WARN("XPIODIR table header longer than expected [%d > %d]\n", xpiodir->hlen, wanthlen);
	}
	if (xpiodir->rlen > wantrlen) {
		ENVY_BIOS_WARN("XPIODIR table record longer than expected [%d > %d]\n", xpiodir->rlen, wantrlen);
	}
	xpiodir->entries = calloc(xpiodir->entriesnum, sizeof *xpiodir->entries);
	if (!xpiodir->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < xpiodir->entriesnum; i++) {
		uint16_t eoff = xpiodir->offset + xpiodir->hlen + xpiodir->rlen * i;
		struct envy_bios_xpio *xpio = &xpiodir->entries[i];
		err |= bios_u16(bios, eoff, &xpio->offset);
		if (err)
			return -EFAULT;
		if (envy_bios_parse_xpio(bios, xpio, i))
			ENVY_BIOS_ERR("Failed to parse XPIO table at 0x%04x version %d.%d\n", xpio->offset, xpio->version >> 4, xpio->version & 0xf);
	}
	xpiodir->valid = 1;
	return 0;
}

static struct enum_val xpio_types[] = {
	{ ENVY_BIOS_XPIO_ADT7473,	"ADT7473" },
	{ ENVY_BIOS_XPIO_PCA9536,	"PCA9536" },
	{ ENVY_BIOS_XPIO_UNUSED,	"UNUSED" },
	{ 0 },
};

void envy_bios_print_xpio (struct envy_bios *bios, FILE *out, struct envy_bios_xpio *xpio, int idx, unsigned mask) {
	if (!xpio->offset)
		return;
	if (!xpio->valid) {
		fprintf(out, "Failed to parse XPIO table at 0x%04x version %d.%d\n\n", xpio->offset, xpio->version >> 4, xpio->version & 0xf);
		return;
	}
	const char *typename = find_enum(xpio_types, xpio->type);
	fprintf(out, "XPIO table %d at 0x%04x version %d.%d", idx, xpio->offset, xpio->version >> 4, xpio->version & 0xf);
	fprintf(out, " type 0x%02x [%s]", xpio->type, typename);
	fprintf(out, " at 0x%02x defbus %d", xpio->addr, xpio->bus);
	if (xpio->unk02_0)
		fprintf(out, " unk02_0 %d", xpio->unk02_0);
	if (xpio->unk02_5)
		fprintf(out, " unk02_5 %d", xpio->unk02_5);
	fprintf(out, "\n");
	envy_bios_dump_hex(bios, out, xpio->offset, xpio->hlen, mask);
	int i;
	for (i = 0; i < xpio->entriesnum; i++) {
		struct envy_bios_gpio_entry *entry = &xpio->entries[i];
		if (entry->tag || mask & ENVY_BIOS_PRINT_UNUSED) {
			fprintf(out, "XPIO %d:", i);
			fprintf(out, " line %d", entry->line);
			if (entry->tag)
				fprintf(out, " tag 0x%02x", entry->tag);
			else
				fprintf(out, " UNUSED");
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
			if (bios->gpio.version >= 0x40)
				fprintf(out, " DEF %d", entry->def);
			if (entry->mode)
				fprintf(out, " SPEC %d [???]", entry->mode);
			if (entry->param)
				fprintf(out, " param 1");
			if (entry->unk40_0)
				fprintf(out, " unk40_0 %d", entry->unk40_0);
			if (entry->unk40_2)
				fprintf(out, " unk40_2 0x%02x", entry->unk40_2);
			if (entry->spec_out)
				fprintf(out, " SPEC_OUT 0x%02x", entry->spec_out);
			if (bios->gpio.version == 0x41 && entry->lockpin != 15)
				fprintf(out, " lockpin %d", entry->lockpin);
			if (entry->spec_in)
				fprintf(out, " SPEC_IN 0x%02x", entry->spec_in-1);
			if (entry->gsync)
				fprintf(out, " gsync %d", entry->gsync);
			fprintf(out, "\n");
		}
		envy_bios_dump_hex(bios, out, entry->offset, xpio->rlen, mask);
	}
	fprintf(out, "\n");
}

void envy_bios_print_xpiodir (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_xpiodir *xpiodir = &bios->gpio.xpiodir;
	if (!xpiodir->offset)
		return;
	if (!xpiodir->valid) {
		fprintf(out, "Failed to parse XPIODIR table at 0x%04x version %d.%d\n\n", xpiodir->offset, xpiodir->version >> 4, xpiodir->version & 0xf);
		return;
	}
	fprintf(out, "XPIODIR table at 0x%04x version %d.%d, %d subtables\n", xpiodir->offset, xpiodir->version >> 4, xpiodir->version & 0xf, xpiodir->entriesnum);
	envy_bios_dump_hex(bios, out, xpiodir->offset, xpiodir->hlen + xpiodir->entriesnum * xpiodir->rlen, mask);
	fprintf(out, "\n");
	int i;
	for (i = 0; i < xpiodir->entriesnum; i++)
		envy_bios_print_xpio(bios, out, &xpiodir->entries[i], i, mask);
}
