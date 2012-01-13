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
	ENVY_BIOS_TYPE_NV03,
	ENVY_BIOS_TYPE_NV04,
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
#define ENVY_BIOS_PRINT_ALL	0xffffffff

int envy_bios_parse (struct envy_bios *bios);
void envy_bios_dump_hex (struct envy_bios *bios, FILE *out, unsigned int start, unsigned int length);
void envy_bios_print (struct envy_bios *bios, FILE *out, unsigned mask);

#endif
