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
#include "util.h"
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
		envy_bios_block(bios, curpos, 3, "SIG", num);
		envy_bios_block(bios, curpos+0x18, 2, "PCIR_PTR", num);
		envy_bios_block(bios, curpos+pcir_offset, 0x18, "PCIR", num);
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
	if (!err) {
		bios->bmp_length = 0xe;
		envy_bios_block(bios, bios->bmp_offset, bios->bmp_length, "BMP", -1);
	}
	/* XXX: add block for init script */
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
	const uint8_t bitsig[7] = "\xff\xb8""BIT\0\0";
	const uint8_t hwsqsig[4] = "HWSQ";
	const uint8_t hweasig[4] = "HWEA";
	switch(vendor) {
	case 0x104a: /* SGS */
		if (device == 0x08 || device == 0x09) {
			bios->type = ENVY_BIOS_TYPE_NV01;
		} else {
			ENVY_BIOS_ERR("Unknown SGS pciid 0x%04x\n", device);
			break;
		}
		break;
	case 0x12d2: /* SGS + nvidia */
		if (device == 0x18 || device == 0x19) {
			bios->type = ENVY_BIOS_TYPE_NV03;
		} else {
			ENVY_BIOS_ERR("Unknown SGS/NVidia pciid 0x%04x\n", device);
			break;
		}
		bios_u16(bios, 0x54, &bios->subsystem_vendor);
		bios_u16(bios, 0x56, &bios->subsystem_device);
		envy_bios_block(bios, 0x54, 4, "HWINFO", -1);
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
		bios->bit.offset = find_string(bios, bitsig, 7);
		bios->hwsq_offset = find_string(bios, hwsqsig, 4);
		bios->hwea_offset = find_string(bios, hweasig, 4);
		bios_u16(bios, 0x36, &bios->dcb.offset);
		envy_bios_block(bios, 0x36, 2, "DCB_PTR", -1);
		bios_u16(bios, 0x54, &bios->subsystem_vendor);
		bios_u16(bios, 0x56, &bios->subsystem_device);
		if (envy_bios_parse_bit(bios))
			ENVY_BIOS_ERR("Failed to parse BIT table at 0x%04x version %d\n", bios->bit.offset, bios->bit.version);
		if (envy_bios_parse_dcb(bios))
			ENVY_BIOS_ERR("Failed to parse DCB table at 0x%04x version %d.%d\n", bios->dcb.offset, bios->dcb.version >> 4, bios->dcb.version & 0xf);
		if (bios->dcb.version >= 0x20) {
			/* XXX: should use chipset instead */
			/* note: NV17 and NV1F don't actually have these registers, but the bioses I've seen include the [nop] values anyway */
			int i;
			uint8_t sum = 0;
			for (i = 0x58; i <= 0x6b; i++) {
				uint8_t byte;
				bios_u8(bios, i, &byte);
				sum += byte;
			}
			uint8_t sig;
			bios_u8(bios, 0x6a, &sig);
			if (sig != 0xa5) {
				ENVY_BIOS_ERR("HWINFO ext sig mismatch [0x%02x]\n", sig);
				envy_bios_block(bios, 0x54, 4, "HWINFO", -1);
			} else if (sum) {
				ENVY_BIOS_ERR("HWINFO ext checksum mismatch [0x%02x]\n", sum);
				envy_bios_block(bios, 0x54, 4, "HWINFO", -1);
			} else {
				bios->hwinfo_ext_valid = 1;
				bios_u32(bios, 0x58, &bios->straps0_select);
				bios_u32(bios, 0x5c, &bios->straps0_value);
				bios_u32(bios, 0x60, &bios->straps1_select);
				bios_u32(bios, 0x64, &bios->straps1_value);
				uint16_t off;
				bios_u16(bios, 0x68, &off);
				bios->mmioinit_offset = off * 4;
				envy_bios_block(bios, 0x54, 0x18, "HWINFO", -1);
				if (off) {
				int pos = 0;
					while (1) {
						uint32_t word;
						struct envy_bios_mmioinit_entry entry = { 0 };
						if (bios_u32(bios, bios->mmioinit_offset + pos, &word)) {
							ENVY_BIOS_ERR("Failed to parse MMIOINIT\n");
							break;
						}
						entry.addr = (word & 0xffc) | (word >> 5 & 0xfffff000);
						entry.len = (word >> 12 & 0x1f) + 1;
						pos += 4;
						int j;
						for (j = 0; j < entry.len; j++, pos += 4)
							bios_u32(bios, bios->mmioinit_offset + pos, &entry.data[j]);
						ADDARRAY(bios->mmioinits, entry);
						if (word & 2)
							break;
					}
					bios->mmioinit_len = pos;
					envy_bios_block(bios, bios->mmioinit_offset, bios->mmioinit_len, "MMIOINIT", -1);
				}
			}
		} else {
			envy_bios_block(bios, 0x54, 4, "HWINFO", -1);
		}
		if (bios->hwea_offset) {
			int pos = 4;
			if (bios->chipset == 0xc0 || bios->chipset == 0xc3 || bios->chipset == 0xc4)
				bios->hwea_version = 0;
			else if (bios->chipset < 0xe0)
				bios->hwea_version = 1;
			else
				bios->hwea_version = 2;
			if (bios->hwea_version >= 2)
				pos = 8;
			while (1) {
				uint32_t word;
				int last = 0;
				struct envy_bios_hwea_entry entry = { 0 };
				if (bios_u32(bios, bios->hwea_offset + pos, &word)) {
					ENVY_BIOS_ERR("Failed to parse HWEA\n");
					break;
				}
				pos += 4;
				if (bios->hwea_version == 0) {
					entry.len = word;
					if (!word)
						break;
					if (bios_u32(bios, bios->hwea_offset + pos, &word)) {
						ENVY_BIOS_ERR("Failed to parse HWEA\n");
						break;
					}
					entry.base = word & 0xfffffff;
					entry.type = word >> 28;
					pos += 4;
					if (entry.type > 1)
						ENVY_BIOS_WARN("Unknown HWEA entry type %d\n", entry.type);
				} else {
					entry.base = word & 0x1fffffc;
					entry.type = word&3;
					switch (entry.type) {
						case 0:
						case 1:
							entry.len = (word >> 25 & 0x3f) + 1;
							break;
						case 3:
							last = 1;
						case 2:
							entry.len = 2;
					}
				}
				entry.data = calloc(sizeof *entry.data, entry.len);
				int j;
				for (j = 0; j < entry.len; j++) {
					if (bios_u32(bios, bios->hwea_offset + pos, &entry.data[j])) {
						ENVY_BIOS_ERR("Failed to parse HWEA\n");
						break;
					}
					pos += 4;
				}
				ADDARRAY(bios->hwea_entries, entry);
				if (last)
					break;
			}
			bios->hwea_len = pos;
			envy_bios_block(bios, bios->hwea_offset, bios->hwea_len, "HWEA", -1);
		}
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

void envy_bios_block(struct envy_bios *bios, unsigned start, unsigned len, const char *name, int idx) {
	struct envy_bios_block block = { start, len, name, idx };
	ADDARRAY(bios->blocks, block);
}
