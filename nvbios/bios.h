/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
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

#ifndef BIOS_H
#define BIOS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#define ENVY_BIOS_ERR(fmt, arg...) fprintf(stderr, fmt, ##arg)
#define ENVY_BIOS_WARN(fmt, arg...) fprintf(stderr, fmt, ##arg)

struct envy_bios_part {
	unsigned int start;
	unsigned int length;
	unsigned int pcir_offset;
	uint16_t pcir_vendor;
	uint16_t pcir_device;
	uint16_t pcir_vpd;
	uint16_t pcir_len;
	uint8_t pcir_rev;
	uint8_t pcir_class[3];
	uint16_t pcir_code_rev;
	uint8_t pcir_code_type;
	uint8_t pcir_indi;
	unsigned int init_length;
	int chksum_pass;
};

enum envy_bios_type {
	ENVY_BIOS_TYPE_UNKNOWN = 0,
	ENVY_BIOS_TYPE_NV01,
	ENVY_BIOS_TYPE_NV03,
	ENVY_BIOS_TYPE_NV04,
};

enum envy_bios_gpio_tag {
	ENVY_BIOS_GPIO_PANEL_BACKLIGHT_ON	= 0x00,
	ENVY_BIOS_GPIO_PANEL_POWER		= 0x01,

	ENVY_BIOS_GPIO_VID_0			= 0x04,
	ENVY_BIOS_GPIO_VID_1			= 0x05,
	ENVY_BIOS_GPIO_VID_2			= 0x06,
	ENVY_BIOS_GPIO_HPD_0			= 0x07,
	ENVY_BIOS_GPIO_HPD_1			= 0x08,
	ENVY_BIOS_GPIO_FAN_CONTROL		= 0x09,

	ENVY_BIOS_GPIO_TVDAC_0			= 0x0c,
	/* 0x0d seen, neg input [NV40, NV44, NV46, NV4A, NV4E] */

	/* 0x0f seen, output [NV50, NVA0, NV98, NVA5, NVA8, NV86, NVD9], SPEC NVIO, uses unk40_0 and unk41_4 */
	ENVY_BIOS_GPIO_ATX_POWER_BAD		= 0x10,
	ENVY_BIOS_GPIO_THERM_ALERT		= 0x11, /* eg. ADT7473 THERM* input [pin 9] */
	/* 0x12 seen, input [NV40, NV42, NV47, NV50] */

	/* 0x18 seen, output [NV92, NV94, NV98, NVA3, NVA5, NVA8, NVC0, NVC1, NVC3, NVC4, NVC8, NVCE, NVCF, NVE4] */
	/* 0x19 seen, output [NV92, NV96] */
	ENVY_BIOS_GPIO_VID_3			= 0x1a,

	ENVY_BIOS_GPIO_PANEL_BACKLIGHT_LEVEL	= 0x21,
	/* 0x22 seen, input [NV47, NV49, NV4B, NV84, NV86, NV92 */
	ENVY_BIOS_GPIO_THERM_SHUTDOWN		= 0x23,	/* XXX: is input sometimes, and even has NVD9+ SPEC_IN? */

	/* 0x25 seen, input [NV46, NV49, NV84, NV86] */

	/* 0x28 seen, output [NV84, NV86] */
	/* 0x29 seen, input [NV42, NV47, NV49, NV50, NVA0, NVC0] */

	/* 0x2b seen, neg input [NV86, NV92, NV94, NV96, NV98, NVA3, NVA5, NVCE] */
	/* 0x2c seen, output [NV4B] */
	ENVY_BIOS_GPIO_TVDAC_1			= 0x2d,
	/* 0x2e seen, output neg [lotsa NV84+ cards], related to mem reclocking... also used as a SPEC NVIO input on nv50 */

	/* 0x30 seen, output or neg input [NVA0, NVC0], *twice*... and sometimes in lots of copies */

	/* 0x34 seen, neg input [NV96, NVA5, NVA8, NVE4] SPEC NVIO on NV96, uses unk41_line */

	ENVY_BIOS_GPIO_FAN_SENSE		= 0x3d,

	ENVY_BIOS_GPIO_SLI_SENSE_0		= 0x40, /* XXX: uses unk40_0, unk41_4, unk41_line */
	ENVY_BIOS_GPIO_SLI_SENSE_1		= 0x41, /* XXX: uses unk40_0, unk41_4, unk41_line */
	/* 0x42 seen, input [NV50, NV92, NVA0, NVC0], SPEC NVIO -- uses unk40_0, unk40_2 */
	/* 0x43 seen, output [NV50, NVA0], SPEC NVIO [or not]... not seen on NVC0+ */
	ENVY_BIOS_GPIO_SLI_SENSE_1_ALT		= 0x44, /* used on NV50 instead of 0x41 for some reason */

	/* 0x49 seen, output [NV98, NVA3, NVA8, NVCE] or input [NVD9], unk41_line used... related to PWM? */
	ENVY_BIOS_GPIO_DP_EXT_0			= 0x4a,	/* XXX: figure out what this is... some input */
	ENVY_BIOS_GPIO_DP_EXT_1			= 0x4b,
	ENVY_BIOS_GPIO_ATX_POWER_BAD_ALT	= 0x4c,
	/* 0x4d seen, neg input [NV84, NV86] */

	ENVY_BIOS_GPIO_HPD_2			= 0x51,
	ENVY_BIOS_GPIO_HPD_3			= 0x52,
	/* 0x53 seen, neg input [NV96] */

	/* 0x56 seen, output [NV92, NVA0] */

	/* 0x5a seen, neg output [NV96], related to 0x5c? */

	/* 0x5c seen, neg output [NV94, NV96, NV98] */

	ENVY_BIOS_GPIO_HPD_4			= 0x5e,
	ENVY_BIOS_GPIO_HPD_5			= 0x5f,
	ENVY_BIOS_GPIO_HPD_6			= 0x60,

	/* 0x6a seen, output [NVAC] */

	/* 0x6c, 0x6d, 0x6e seen, output [NVCE] */
	/* 0x6f seen, input [NVA5, NVA8] SPEC NVIO */

	ENVY_BIOS_GPIO_VID_4			= 0x73,

	/* 0x78 seen, output [NVC0, NVC3, NVC4, NVC8, NVCE, NVCF, NVE4 */
	/* 0x79 seen, neg input [NVC0, NVE4], uses unk41_line */
	/* 0x7a seen, open-collector output [NVE4] */

	/* 0x83 seen, input [NVE4], SPEC - connected to PWM??? */

	ENVY_BIOS_GPIO_UNUSED			= 0xff,
};

struct envy_bios_gpio_entry {
	uint16_t offset;
	uint8_t tag;
	uint8_t line;
	uint8_t log[2];
	uint8_t param;
	uint8_t unk40_0;
	uint8_t unk40_2;
	uint8_t def;
	uint8_t mode;
	uint8_t spec_out;
	uint8_t spec_in;
	uint8_t unk41_3_1;
	uint8_t unk41_4; /* same thing as unk40_0 probably... */
};

enum envy_bios_xpio_type {
	ENVY_BIOS_XPIO_UNUSED	= 0x00,
	/* 0x01 seen, address 0x40 */
	ENVY_BIOS_XPIO_ADT7473	= 0x02,
	ENVY_BIOS_XPIO_PCA9536	= 0x05,
	/* 0x07 seen, address 0x92 */
};

struct envy_bios_xpio {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t type;
	uint8_t addr;
	uint8_t bus;
	uint8_t unk02_0;
	uint8_t unk02_5;
};

struct envy_bios_xpiodir {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	struct envy_bios_xpio *entries;
};

struct envy_bios_gpio {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	struct envy_bios_gpio_entry *entries;
	struct envy_bios_xpiodir xpiodir;
};

struct envy_bios_dunk0c {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
};

struct envy_bios_dunk0e {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
};

struct envy_bios_dunk10 {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
};

enum envy_bios_extdev_type {
	ENVY_BIOS_EXTDEV_LM89		= 0x02,
	/* 0x04 seen, at address 0x98 */
	ENVY_BIOS_EXTDEV_VT1103M	= 0x40,
	ENVY_BIOS_EXTDEV_PX3540		= 0x41,
	ENVY_BIOS_EXTDEV_VT1105M	= 0x42, /* or close enough... */
	/* 0x43 seen, at address 0x60 */
	/* 0x48 seen, at address 0x60, 0xe0 and 0xe2 */
	/* 0x4c seen, at address 0x90, 0x92, 0x94 */
	/* 0x50 seen, at address 0xca */
	ENVY_BIOS_EXTDEV_ADT7473	= 0x70,
	ENVY_BIOS_EXTDEV_HDCP_EEPROM	= 0x90,
	/* 0xa0 seen, at address 0xa8 */
	ENVY_BIOS_EXTDEV_UNUSED		= 0xff,
};

struct envy_bios_extdev_entry {
	uint16_t offset;
	uint8_t type;
	uint8_t addr;
	uint8_t bus;
	uint8_t unk02_0;
	uint8_t unk02_5;
	uint8_t unk03;
};

struct envy_bios_extdev {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t unk04;
	struct envy_bios_extdev_entry *entries;
};

enum envy_bios_conn_type {
	/* VGA */
	ENVY_BIOS_CONN_VGA			= 0x00,
	/* TV */
	ENVY_BIOS_CONN_COMPOSITE		= 0x10,
	ENVY_BIOS_CONN_S_VIDEO			= 0x11,
	ENVY_BIOS_CONN_S_VIDEO_COMPOSITE	= 0x12,	/* used on NV40 only - later cards have separate 0x10 and 0x11 entries instead */
	ENVY_BIOS_CONN_RGB			= 0x13, /* ... or YCbCr? */
	/* 0x17 seen, another type of TV */
	/* DVI */
	ENVY_BIOS_CONN_DVI_I			= 0x30,
	ENVY_BIOS_CONN_DVI_D			= 0x31,
	ENVY_BIOS_CONN_DMS59_0			= 0x38,
	ENVY_BIOS_CONN_DMS59_1			= 0x39,
	/* LVDS and DP */
	ENVY_BIOS_CONN_LVDS			= 0x40,
	ENVY_BIOS_CONN_LVDS_SPWG		= 0x41,
	ENVY_BIOS_CONN_DP			= 0x46,
	ENVY_BIOS_CONN_EDP			= 0x47,
	/* ??? */
	/* 0x54 and 0x55 seen - some crazy TMDS connector in use on Dell laptops */
	/* HDMI... and DP? */
	/* 0x60 seen, never connected to anything sensible, alleged to be HDMI by nouveau */
	ENVY_BIOS_CONN_HDMI			= 0x61,
	/* 0x62 seen, never connected to anything sensible */
	ENVY_BIOS_CONN_DMS59_DP0		= 0x64,
	ENVY_BIOS_CONN_DMS59_DP1		= 0x65,
	ENVY_BIOS_CONN_UNUSED			= 0xff,
};

struct envy_bios_conn_entry {
	uint16_t offset;
	uint8_t type;
	uint8_t tag;
	int8_t hpd;
	int8_t dp_ext;
	uint8_t unk02_2;
	uint8_t unk02_4; /* a bitmask [like hpd/dp_ext] of something to do with native DP [but not EDP] encoders */
	uint8_t unk02_7; /* probably unused */
	uint8_t unk03_3; /* probably unused */
};

struct envy_bios_conn {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	struct envy_bios_conn_entry *entries;
};

struct envy_bios_dunk17 {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
};

struct envy_bios_dunk19 {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
};

struct envy_bios {
	uint8_t *data;
	unsigned int length;
	unsigned int origlength;

	struct envy_bios_part *parts;
	int partsnum;
	int broken_part;

	enum envy_bios_type type;

	unsigned int bmp_offset;
	unsigned int bmp_length;
	uint8_t bmp_ver_major;
	uint8_t bmp_ver_minor;
	uint16_t bmp_ver;
	uint16_t mode_x86;
	uint16_t init_x86;
	uint16_t init_script;

	unsigned int bit_offset;

	unsigned int hwsq_offset;

	uint16_t dcb_offset;

	struct envy_bios_gpio gpio;
	struct envy_bios_dunk0c dunk0c;
	struct envy_bios_dunk0e dunk0e;
	struct envy_bios_dunk10 dunk10;
	struct envy_bios_extdev extdev;
	struct envy_bios_conn conn;
	struct envy_bios_dunk17 dunk17;
	struct envy_bios_dunk19 dunk19;
};

static inline int bios_u8(struct envy_bios *bios, unsigned int offs, uint8_t *res) {
	if (offs >= bios->length) {
		*res = 0;
		ENVY_BIOS_ERR("requested OOB u8 at 0x%04x\n", offs);
		return -EFAULT;
	}
	*res = bios->data[offs];
	return 0;
}

static inline int bios_u16(struct envy_bios *bios, unsigned int offs, uint16_t *res) {
	if (offs+1 >= bios->length) {
		*res = 0;
		ENVY_BIOS_ERR("requested OOB u16 at 0x%04x\n", offs);
		return -EFAULT;
	}
	*res = bios->data[offs] | bios->data[offs+1] << 8;
	return 0;
}

static inline int bios_u32(struct envy_bios *bios, unsigned int offs, uint32_t *res) {
	if (offs+3 >= bios->length) {
		*res = 0;
		ENVY_BIOS_ERR("requested OOB u32 at 0x%04x\n", offs);
		return -EFAULT;
	}
	*res = bios->data[offs] | bios->data[offs+1] << 8 | bios->data[offs+2] << 16 | bios->data[offs+3] << 24;
	return 0;
}

#define ENVY_BIOS_PRINT_PCIR	0x00000001
#define ENVY_BIOS_PRINT_BMP_BIT	0x00000002
#define ENVY_BIOS_PRINT_SCRIPTS	0x00000004
#define ENVY_BIOS_PRINT_GPIO	0x00000008
#define ENVY_BIOS_PRINT_ALL	0xffffffff

int envy_bios_parse (struct envy_bios *bios);
void envy_bios_dump_hex (struct envy_bios *bios, FILE *out, unsigned int start, unsigned int length);
void envy_bios_print (struct envy_bios *bios, FILE *out, unsigned mask);

int envy_bios_parse_gpio (struct envy_bios *bios);
void envy_bios_print_gpio (struct envy_bios *bios, FILE *out);
int envy_bios_parse_dunk0c (struct envy_bios *bios);
void envy_bios_print_dunk0c (struct envy_bios *bios, FILE *out);
int envy_bios_parse_dunk0e (struct envy_bios *bios);
void envy_bios_print_dunk0e (struct envy_bios *bios, FILE *out);
int envy_bios_parse_dunk10 (struct envy_bios *bios);
void envy_bios_print_dunk10 (struct envy_bios *bios, FILE *out);
int envy_bios_parse_extdev (struct envy_bios *bios);
void envy_bios_print_extdev (struct envy_bios *bios, FILE *out);
int envy_bios_parse_conn (struct envy_bios *bios);
void envy_bios_print_conn (struct envy_bios *bios, FILE *out);
int envy_bios_parse_dunk17 (struct envy_bios *bios);
void envy_bios_print_dunk17 (struct envy_bios *bios, FILE *out);
int envy_bios_parse_dunk19 (struct envy_bios *bios);
void envy_bios_print_dunk19 (struct envy_bios *bios, FILE *out);

struct enum_val {
	int val;
	const char *str;
};

const char *find_enum(struct enum_val *evals, int val);

#endif
