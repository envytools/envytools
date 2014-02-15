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

int envy_bios_parse_bit_i (struct envy_bios *bios, struct envy_bios_bit_entry *bit) {
	struct envy_bios_info *info = &bios->info;
	info->bit = bit;
	int wantlen = 0x19;
	if (bit->t_len < wantlen) {
		ENVY_BIOS_ERR("BIT 'i' table too short: 0x%04x < 0x%04x\n", bit->t_len, wantlen);
		return -EINVAL;
	}
	if (bit->t_len >= 0x23) wantlen = 0x23;
	if (bit->t_len >= 0x24) wantlen = 0x24;
	if (bit->t_len >= 0x25) wantlen = 0x25;
	if (bit->t_len >= 0x26) wantlen = 0x26;
	if (bit->t_len >= 0x28) wantlen = 0x28;
	if (bit->t_len >= 0x30) wantlen = 0x30;
	if (bit->t_len >= 0x41) wantlen = 0x41;
	if (bit->t_len >= 0x42) wantlen = 0x42;
	if (bit->t_len >= 0x43) wantlen = 0x43;
	if (bit->t_len >= 0x44) wantlen = 0x44;
	if (bit->t_len != wantlen) {
		ENVY_BIOS_WARN("BIT 'i' table has strange size: 0x%04x > 0x%04x\n", bit->t_len, wantlen);
	}
	int err = 0;
	err |= bios_u8(bios, bit->t_offset+0, &info->version[3]);
	err |= bios_u8(bios, bit->t_offset+1, &info->version[2]);
	err |= bios_u8(bios, bit->t_offset+2, &info->version[1]);
	err |= bios_u8(bios, bit->t_offset+3, &info->version[0]);
	err |= bios_u8(bios, bit->t_offset+4, &info->version[4]);
	err |= bios_u16(bios, bit->t_offset+5, &info->feature);
	int i;
	for (i = 0; i < 6; i++)
		err |= bios_u8(bios, bit->t_offset+7+i, &info->unk07[i]);
	err |= bios_u16(bios, bit->t_offset+0xd, &bios->dacload.offset);
	err |= bios_string(bios, bit->t_offset+0xf, info->date, 8);
	err |= bios_u16(bios, bit->t_offset+0x17, &info->real_pcidev);
	if (wantlen >= 0x23) {
		for (i = 0; i < 8; i++)
			err |= bios_u8(bios, bit->t_offset+0x19+i, &info->unk19[i]);
		err |= bios_u16(bios, bit->t_offset+0x21, &bios->iunk21.offset);
	}
	if (wantlen >= 0x24)
		err |= bios_u8(bios, bit->t_offset+0x23, &info->unk23);
	if (wantlen >= 0x25)
		err |= bios_u8(bios, bit->t_offset+0x24, &info->unk24);
	if (wantlen >= 0x26)
		err |= bios_u8(bios, bit->t_offset+0x25, &info->unk25);
	if (wantlen >= 0x28)
		err |= bios_u16(bios, bit->t_offset+0x26, &info->unk26);
	if (wantlen >= 0x30) {
		for (i = 0; i < 8; i++)
			err |= bios_u8(bios, bit->t_offset+0x28+i, &info->unk28[i]);
	}
	if (wantlen >= 0x41) {
		err |= bios_string(bios, bit->t_offset+0x30, info->unk30, 12);
		for (i = 0; i < 5; i++)
			err |= bios_u8(bios, bit->t_offset+0x3c+i, &info->unk3c[i]);
	}
	if (wantlen >= 0x42)
		err |= bios_u8(bios, bit->t_offset+0x3c+5, &info->unk3c[5]);
	if (wantlen >= 0x43)
		err |= bios_u8(bios, bit->t_offset+0x3c+6, &info->unk3c[6]);
	if (wantlen >= 0x44)
		err |= bios_u8(bios, bit->t_offset+0x3c+7, &info->unk3c[7]);
	if (err)
		return -EFAULT;
	info->valid = 1;
	switch(info->version[0] << 8 | info->version[1]) {
		case 0x0540:
			bios->chipset = 0x40;
			bios->chipset_name = "NV40";
			break;
		case 0x0541:
			bios->chipset = 0x41; /* or 0x42 */
			bios->chipset_name = "NV41/NV42";
			break;
		case 0x0543:
			bios->chipset = 0x43;
			bios->chipset_name = "NV43";
			break;
		case 0x0544:
			bios->chipset = 0x44; /* or 0x4a */
			bios->chipset_name = "NV44/NV44A";
			break;
		case 0x0570:
			bios->chipset = 0x47;
			bios->chipset_name = "G70";
			break;
		case 0x0571:
			bios->chipset = 0x49;
			bios->chipset_name = "G71";
			break;
		case 0x0572:
			bios->chipset = 0x46;
			bios->chipset_name = "G72";
			break;
		case 0x0573:
			/* yeah, ain't this one hilarious... */
			if ((bios->parts[0].pcir_device & 0xfff0) == 0x7e0) {
				bios->chipset = 0x63;
				bios->chipset_name = "MCP73";
			} else {
				bios->chipset = 0x4b;
				bios->chipset_name = "G73";
			}
			break;
		case 0x0551:
			bios->chipset = 0x4e;
			bios->chipset_name = "C51";
			break;
		case 0x0561:
			bios->chipset = 0x4c;
			bios->chipset_name = "MCP61";
			break;
		case 0x0567:
			bios->chipset = 0x67; /* or 0x68 */
			bios->chipset_name = "MCP67/MCP68";
			break;
		/* G80 */
		case 0x6080:
			bios->chipset = 0x50;
			bios->chipset_name = "G80";
			break;
		case 0x6084:
			bios->chipset = 0x84;
			bios->chipset_name = "G84";
			break;
		case 0x6086:
			bios->chipset = 0x86;
			bios->chipset_name = "G86";
			break;
		/* G92 */
		case 0x6292:
			bios->chipset = 0x86;
			bios->chipset_name = "G92";
			break;
		case 0x6294:
			bios->chipset = 0x94;
			bios->chipset_name = "G94/G96";
			break;
		case 0x6298:
			bios->chipset = 0x98;
			bios->chipset_name = "G98";
			break;
		case 0x6200:
			bios->chipset = 0xa0;
			bios->chipset_name = "G200";
			break;
		case 0x6277:
			bios->chipset = 0xaa;
			bios->chipset_name = "MCP77";
			break;
		case 0x6279:
			bios->chipset = 0xac;
			bios->chipset_name = "MCP79";
			break;
		/* GT212+ */
		case 0x7015:
			bios->chipset = 0xa3;
			bios->chipset_name = "GT215";
			break;
		case 0x7016:
			bios->chipset = 0xa5;
			bios->chipset_name = "GT216";
			break;
		case 0x7018:
			bios->chipset = 0xa8;
			bios->chipset_name = "GT218";
			break;
		case 0x7089:
			bios->chipset = 0xaf;
			bios->chipset_name = "MCP89";
			break;
		case 0x7000:
			bios->chipset = 0xc0;
			bios->chipset_name = "GF100";
			break;
		case 0x7004:
			bios->chipset = 0xc4;
			bios->chipset_name = "GF104";
			break;
		case 0x7006:
			bios->chipset = 0xc3;
			bios->chipset_name = "GF106";
			break;
		case 0x7008:
			bios->chipset = 0xc1;
			bios->chipset_name = "GF108";
			break;
		case 0x7010:
			bios->chipset = 0xc8;
			bios->chipset_name = "GF110";
			break;
		case 0x7024:
			bios->chipset = 0xce;
			bios->chipset_name = "GF114";
			break;
		case 0x7026:
			bios->chipset = 0xcf;
			bios->chipset_name = "GF116";
			break;
		/* GF119 */
		case 0x7519:
			bios->chipset = 0xd9;
			bios->chipset_name = "GF119";
			break;
		/* GK104 */
		case 0x8004:
			bios->chipset = 0xe4;
			bios->chipset_name = "GK104";
			break;
		case 0x8007:
			bios->chipset = 0xe7;
			bios->chipset_name = "GK107";
			break;
	}
	if (envy_bios_parse_dacload(bios))
		ENVY_BIOS_ERR("Failed to parse DACLOAD table at 0x%04x version %d.%d\n", bios->dacload.offset, bios->dacload.version >> 4, bios->dacload.version & 0xf);
	if (envy_bios_parse_iunk21(bios))
		ENVY_BIOS_ERR("Failed to parse IUNK21 table at 0x%04x version %d.%d\n", bios->iunk21.offset, bios->iunk21.version >> 4, bios->iunk21.version & 0xf);
	return 0;
}

void envy_bios_print_info (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_info *info = &bios->info;
	if (!info->bit || !(mask & ENVY_BIOS_PRINT_INFO))
		return;
	if (!info->valid) {
		fprintf(out, "Failed to parse BIT table 'i' at 0x%04x version 2\n\n", info->bit->t_offset);
		return;
	}
	fprintf(out, "INFO: BIOS version 0x%02x.%02x.%02x.%02x.%02x from %s for", info->version[0], info->version[1], info->version[2], info->version[3], info->version[4], info->date);
	if (bios->chipset) {
		fprintf(out, " NV%02X", bios->chipset);
		if (bios->chipset_name)
			fprintf(out, " [%s]", bios->chipset_name);
	} else {
		fprintf(out, " unknown chipset");
	}
	fprintf(out, "\n");
	if (info->real_pcidev)
		fprintf(out, "INFO: BR02 card, real device id 0x%04x\n", info->real_pcidev);
	fprintf(out, "INFO: features");
	int i;
	static const char *const featurenames[16] = {
		"UNK0",
		"UNK1",
		"UNK2",
		"UNK3",
		"MOBILE",
		/* fun: these mean that FP or TV *tables* are present, not that FP or TV is present - always set on BIT BIOS. Given up on nv50+. */
		"FP",
		"TV",
		"UNK7",
		"UNK8",
		"UNK9",
		"UNK10",
		"UNK11",
		"UNK12",
		"UNK13",
		"UNK14",
		"UNK15",
	};
	for (i = 0; i < 16; i++)
		if (info->feature & 1 << i)
			fprintf(out, " %s", featurenames[i]);
	if (!info->feature)
		fprintf(out, " (none)");
	fprintf(out, ", unk07");
	for (i = 0; i < 6; i++)
		fprintf(out, " 0x%02x", info->unk07[i]);
	int unk19_st = 0;
	for (i = 0; i < 8; i++)
		if (info->unk19[i])
			unk19_st = 1;
	if (unk19_st) {
		fprintf(out, ", unk19");
		for (i = 0; i < 8; i++)
			fprintf(out, " 0x%02x", info->unk19[i]);
	}
	if (info->unk23)
		fprintf(out, ", unk23 0x%02x", info->unk23);
	if (info->unk24)
		fprintf(out, ", unk24 0x%02x", info->unk24);
	if (info->unk25)
		fprintf(out, ", unk25 0x%02x", info->unk25);
	if (info->unk26)
		fprintf(out, ", unk26 0x%04x", info->unk26);
	int unk28_st = 0;
	for (i = 0; i < 8; i++)
		if (info->unk28[i])
			unk28_st = 1;
	if (unk28_st) {
		fprintf(out, ", unk28");
		for (i = 0; i < 8; i++)
			fprintf(out, " 0x%02x", info->unk28[i]);
	}
	if (info->unk30[0]) {
		fprintf(out, ", unk30 %s", info->unk30);
		/* umm wtf? */
		if (!info->unk30[3])
			fprintf(out, " %s", info->unk30+4);
	}
	int unk3c_st = 0;
	for (i = 0; i < 8; i++)
		if (info->unk3c[i])
			unk3c_st = 1;
	if (unk3c_st) {
		fprintf(out, ", unk3c");
		for (i = 0; i < 8; i++)
			fprintf(out, " 0x%02x", info->unk3c[i]);
	}
	fprintf(out, "\n");
	envy_bios_dump_hex(bios, out, info->bit->t_offset, info->bit->t_len, mask);
	fprintf(out, "\n");
}
