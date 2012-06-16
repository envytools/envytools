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

#include "bios.h"
#include <string.h>

static int parse_pcir (struct envy_bios *bios) {
	bios->length = bios->origlength;
	unsigned int curpos = 0;
	unsigned int num = 0;
	int next = 0;
	do {
		uint16_t pcir_offset;
		uint16_t sig;
		uint32_t pcir_sig;
		uint16_t pcir_ilen;
		uint8_t pcir_indi;
		uint16_t pcir_res;
		next = 0;
		if (bios_u16(bios, curpos, &sig)) {
broken_part:
			bios->broken_part = 1;
			break;
		}
		if (sig != 0xaa55)
			goto broken_part;
		if (bios_u16(bios, curpos + 0x18, &pcir_offset))
			goto broken_part;
		if (bios_u32(bios, curpos+pcir_offset, &pcir_sig))
			goto broken_part;
		if (pcir_sig != 0x52494350)
			goto broken_part;
		if (bios_u16(bios, curpos+pcir_offset+0x10, &pcir_ilen))
			goto broken_part;
		if (bios_u8(bios, curpos+pcir_offset+0x15, &pcir_indi))
			goto broken_part;
		if (bios_u16(bios, curpos+pcir_offset+0x16, &pcir_res))
			goto broken_part;
		if (curpos + pcir_ilen * 0x200 > bios->origlength)
			goto broken_part;
		if (!(pcir_indi & 0x80))
			next = 1;
		curpos += pcir_ilen * 0x200;
		num++;
	} while(next);
	if (!num)
		return -EINVAL;
	bios->partsnum = num;
	bios->parts = calloc(bios->partsnum, sizeof *bios->parts);
	if (!bios->parts)
		return -ENOMEM;
	bios->length = curpos;
	curpos = 0;
	for (num = 0; num < bios->partsnum; num++) {
		uint16_t pcir_offset;
		uint16_t pcir_ilen;
		bios->parts[num].start = curpos;
		bios_u16(bios, curpos + 0x18, &pcir_offset);
		bios_u16(bios, curpos+pcir_offset+4, &bios->parts[num].pcir_vendor);
		bios_u16(bios, curpos+pcir_offset+6, &bios->parts[num].pcir_device);
		bios_u16(bios, curpos+pcir_offset+0x8, &bios->parts[num].pcir_vpd);
		bios_u16(bios, curpos+pcir_offset+0xa, &bios->parts[num].pcir_len);
		bios_u8(bios, curpos+pcir_offset+0xc, &bios->parts[num].pcir_rev);
		bios_u8(bios, curpos+pcir_offset+0xd, &bios->parts[num].pcir_class[0]);
		bios_u8(bios, curpos+pcir_offset+0xe, &bios->parts[num].pcir_class[1]);
		bios_u8(bios, curpos+pcir_offset+0xf, &bios->parts[num].pcir_class[2]);
		bios_u16(bios, curpos+pcir_offset+0x10, &pcir_ilen);
		bios_u16(bios, curpos+pcir_offset+0x12, &bios->parts[num].pcir_code_rev);
		bios_u8(bios, curpos+pcir_offset+0x14, &bios->parts[num].pcir_code_type);
		bios_u8(bios, curpos+pcir_offset+0x15, &bios->parts[num].pcir_indi);
		bios->parts[num].pcir_offset = pcir_offset;
		bios->parts[num].length = pcir_ilen * 0x200;
		if (bios->parts[num].pcir_code_type == 0) {
			int i;
			uint8_t sum = 0;
			uint8_t init_ilen;
			bios_u8(bios, curpos + 2, &init_ilen);
			bios->parts[num].init_length = init_ilen * 0x200;
			for (i = 0; i < bios->parts[num].init_length; i++)
				sum += bios->data[curpos + i];
			bios->parts[num].chksum_pass = (sum == 0);
		}
		curpos += bios->parts[num].length;
	}
	return 0;
}

static unsigned int find_string(struct envy_bios *bios, const uint8_t *str, int len)
{
	int i;
	for (i = 0; i <= (bios->length - len); i++)
		if (!memcmp(bios->data + i, str, len))
			return i;
	return 0;
}

static void parse_bmp_nv03(struct envy_bios *bios) {
	int err = 0;
	err |= bios_u8(bios, bios->bmp_offset + 5, &bios->bmp_ver_major);
	err |= bios_u8(bios, bios->bmp_offset + 6, &bios->bmp_ver_minor);
	err |= bios_u16(bios, bios->bmp_offset + 8, &bios->mode_x86);
	err |= bios_u16(bios, bios->bmp_offset + 0xa, &bios->init_x86);
	err |= bios_u16(bios, bios->bmp_offset + 0xc, &bios->init_script);
	bios->bmp_ver = bios->bmp_ver_major << 8 | bios->bmp_ver_minor;
	if (bios->bmp_ver != 0x01)
		ENVY_BIOS_WARN("NV03 BMP version not 0.1!\n");
	if (!err)
		bios->bmp_length = 0xe;
}

int envy_bios_parse (struct envy_bios *bios) {
	int ret = 0;
	uint16_t vendor, device;
	ret = parse_pcir(bios);
	if (ret)
		return ret;
	if (!bios->partsnum)
		return -EINVAL;
	vendor = bios->parts[0].pcir_vendor;
	device = bios->parts[0].pcir_device;
	const uint8_t bmpsig[5] = "\xff\x7f""NV\0";
	const uint8_t bitsig[5] = "\xff\xb8""BIT";
	const uint8_t hwsqsig[4] = "HWSQ";
	switch(vendor) {
	case 0x104a: /* SGS */
		if (device == 0x08 || device == 0x09) {
			bios->type = ENVY_BIOS_TYPE_NV01;
		} else {
			ENVY_BIOS_ERR("Unknown SGS pciid %04x\n", device);
			break;
		}
		break;
	case 0x12d2: /* SGS + nvidia */
		if (device == 0x18 || device == 0x19) {
			bios->type = ENVY_BIOS_TYPE_NV03;
		} else {
			ENVY_BIOS_ERR("Unknown SGS/NVidia pciid %04x\n", device);
			break;
		}
		bios->bmp_offset = find_string(bios, bmpsig, 5);
		if (!bios->bmp_offset) {
			ENVY_BIOS_ERR("BMP not found\n");
			break;
		}
		parse_bmp_nv03(bios);
		break;
	case 0x10de: /* nvidia */
		bios->type = ENVY_BIOS_TYPE_NV04;
		bios->bmp_offset = find_string(bios, bmpsig, 5);
		bios->bit_offset = find_string(bios, bitsig, 5);
		bios->hwsq_offset = find_string(bios, hwsqsig, 4);
		bios_u16(bios, 0x36, &bios->dcb.offset);
		if (envy_bios_parse_dcb(bios))
			ENVY_BIOS_ERR("Failed to parse DCB table at %04x version %x.%x\n", bios->dcb.offset, bios->dcb.version >> 4, bios->dcb.version & 0xf);
		break;
	}
	return 0;
}

const char *find_enum(struct enum_val *evals, int val) {
	int i;
	for (i = 0; evals[i].str; i++)
		if (val == evals[i].val)
			return evals[i].str;
	return "???";
}
