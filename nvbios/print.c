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
#include <stdio.h>
#include <string.h>

void dump_hex_script (struct envy_bios *bios, FILE *out, unsigned int start, unsigned int length) {
	int cnt = 0;
	while (length) {
		unsigned int i, len = length;
		if (len > 16) len = 16;
		if (cnt)
			fprintf (out, "     ");
		else
			fprintf (out, "%04x:", start);
		for (i = 0; i < len; i++)
			fprintf(out, " %02x", bios->data[start+i]);
		start += len;
		length -= len;
		if (length) {
			fprintf(out, "\n");
		} else {
			while (len < 16) {
				fprintf(out, "   ");
				len++;
			}
			fprintf(out, "  ");
		}
		cnt++;
	}
}

void envy_bios_dump_hex (struct envy_bios *bios, FILE *out, unsigned int start, unsigned int length, unsigned mask) {
	if (start + length > bios->length) {
		fprintf(out, "ERR: Cannot dump 0x%x bytes at 0x%x: OOB\n", length, start);
		if (start < bios->length) {
			length = bios->length - start;
		} else {
			return;
		}
	}
	if (mask & ENVY_BIOS_PRINT_VERBOSE)
		while (length) {
			unsigned int i, len = length;
			if (len > 16) len = 16;
			fprintf (out, "0x%04x:", start);
			for (i = 0; i < len; i++)
				fprintf(out, " %02x", bios->data[start+i]);
			fprintf(out, "\n");
			start += len;
			length -= len;
		}
}

static void print_pcir(struct envy_bios *bios, FILE *out, unsigned mask) {
	int i;
	if (!(mask & ENVY_BIOS_PRINT_PCIR))
		return;
	fprintf(out, "BIOS size 0x%x [orig: 0x%x], %d valid parts:\n", bios->length, bios->origlength, bios->partsnum);
	for (i = 0; i < bios->partsnum; i++) {
		fprintf(out, "\n");
		if (bios->parts[i].pcir_code_type == 0) {
			fprintf(out, "BIOS part %d at 0x%x size 0x%x [init: 0x%x]. Sig:\n", i, bios->parts[i].start, bios->parts[i].length, bios->parts[i].init_length);
			envy_bios_dump_hex(bios, out, bios->parts[i].start, 3, mask);
			if (!bios->parts[i].chksum_pass)
				fprintf(out, "WARN: checksum fail\n");
		} else {
			fprintf(out, "BIOS part %d at 0x%x size 0x%x. Sig:\n", i, bios->parts[i].start, bios->parts[i].length);
			envy_bios_dump_hex(bios, out, bios->parts[i].start, 2, mask);
		}
		fprintf(out, "PCIR [rev 0x%02x]:\n", bios->parts[i].pcir_rev);
		envy_bios_dump_hex(bios, out, bios->parts[i].start + bios->parts[i].pcir_offset, bios->parts[i].pcir_len, mask);
		fprintf(out, "PCI device: 0x%04x:0x%04x, class 0x%02x%02x%02x\n", bios->parts[i].pcir_vendor, bios->parts[i].pcir_device, bios->parts[i].pcir_class[2], bios->parts[i].pcir_class[1], bios->parts[i].pcir_class[0]);
		if (bios->parts[i].pcir_vpd)
			fprintf(out, "VPD: 0x%x\n", bios->parts[i].pcir_vpd);
		fprintf(out, "Code type 0x%02x, rev 0x%04x\n", bios->parts[i].pcir_code_type, bios->parts[i].pcir_code_rev);
		fprintf(out, "PCIR indicator: 0x%02x\n", bios->parts[i].pcir_indi);
	}
	if (bios->broken_part)
		fprintf(out, "\nWARN: Couldn't read part %d!\n", bios->partsnum);
	fprintf(out, "\n");
}

static void print_bmp_nv03(struct envy_bios *bios, FILE *out, unsigned mask) {
	if (!(mask & ENVY_BIOS_PRINT_BMP_BIT) || !bios->bmp_length)
		return;
	fprintf(out, "BMP 0x%02x.%02x at 0x%x\n", bios->bmp_ver_major, bios->bmp_ver_minor, bios->bmp_offset);
	envy_bios_dump_hex(bios, out, bios->bmp_offset, bios->bmp_length, mask);
	fprintf(out, "x86 mode pointer: 0x%x\n", bios->mode_x86);
	fprintf(out, "x86 init pointer: 0x%x\n", bios->init_x86);
	fprintf(out, "init script pointer: 0x%x\n", bios->init_script);
	fprintf(out, "\n");
}

static void print_nv01_init_script(struct envy_bios *bios, FILE *out, unsigned offset, unsigned mask) {
	unsigned len;
	uint8_t op;
	uint8_t arg8_0, arg8_1, arg8_2;
	uint16_t arg16_0;
	uint32_t arg32_0, arg32_1, arg32_2;
	int err = 0;
	if (!(mask & ENVY_BIOS_PRINT_SCRIPTS))
		return;
	fprintf(out, "Init script at 0x%x:\n", offset);
	while (1) {
		if (bios_u8(bios, offset, &op)) {
			ENVY_BIOS_ERR("Init script out of bounds!\n");
			return;
		}
		switch (op) {
		case 0x6e:	/* NV01+ */
			len = 13;
			err |= bios_u32(bios, offset+1, &arg32_0);
			err |= bios_u32(bios, offset+5, &arg32_1);
			err |= bios_u32(bios, offset+9, &arg32_2);
			if (err) {
				ENVY_BIOS_ERR("Init script out of bounds!\n");
				return;
			}
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tMMIO_MASK 0x%06x &= 0x%08x |= 0x%08x\n", arg32_0, arg32_1, arg32_2);
			break;
		case 0x7a:	/* NV03+ */
			len = 9;
			err |= bios_u32(bios, offset+1, &arg32_0);
			err |= bios_u32(bios, offset+5, &arg32_1);
			if (err) {
				ENVY_BIOS_ERR("Init script out of bounds!\n");
				return;
			}
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tMMIO_WR 0x%06x <= 0x%08x\n", arg32_0, arg32_1);
			break;
		case 0x77:	/* NV03+ */
			len = 7;
			err |= bios_u32(bios, offset+1, &arg32_0);
			err |= bios_u16(bios, offset+5, &arg16_0);
			if (err) {
				ENVY_BIOS_ERR("Init script out of bounds!\n");
				return;
			}
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tMMIO_WR16 0x%06x <= 0x%04x\n", arg32_0, arg16_0);
			break;
		case 0x79:	/* NV03+ */
			len = 13;
			err |= bios_u32(bios, offset+1, &arg32_0);
			err |= bios_u32(bios, offset+5, &arg32_1);
			err |= bios_u32(bios, offset+9, &arg32_2);
			if (err) {
				ENVY_BIOS_ERR("Init script out of bounds!\n");
				return;
			}
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tMMIO_WR_CRYSTAL 0x%06x <= 0x%08x / 0x%08x\n", arg32_0, arg32_1, arg32_2);
			break;
		case 0x74:	/* NV03+ */
			len = 3;
			err |= bios_u16(bios, offset+1, &arg16_0);
			if (err) {
				ENVY_BIOS_ERR("Init script out of bounds!\n");
				return;
			}
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tUSLEEP %d\n", arg16_0);
			break;
		case 0x69:	/* NV03+ */
			len = 5;
			err |= bios_u16(bios, offset+1, &arg16_0);
			err |= bios_u8(bios, offset+3, &arg8_0);
			err |= bios_u8(bios, offset+4, &arg8_1);
			if (err) {
				ENVY_BIOS_ERR("Init script out of bounds!\n");
				return;
			}
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tIO_MASK 0x%04x &= 0x%02x |= 0x%02x\n", arg16_0, arg8_0, arg8_1);
			break;
		case 0x78:	/* NV03+ */
			len = 6;
			err |= bios_u16(bios, offset+1, &arg16_0);
			err |= bios_u8(bios, offset+3, &arg8_0);
			err |= bios_u8(bios, offset+4, &arg8_1);
			err |= bios_u8(bios, offset+5, &arg8_2);
			if (err) {
				ENVY_BIOS_ERR("Init script out of bounds!\n");
				return;
			}
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tIOIDX_MASK 0x%04x[0x%02x] &= 0x%02x |= 0x%02x\n", arg16_0, arg8_0, arg8_1, arg8_2);
			break;
		case 0x6d:	/* NV03+ */
			len = 2;
			err |= bios_u8(bios, offset+1, &arg8_0);
			if (err) {
				ENVY_BIOS_ERR("Init script out of bounds!\n");
				return;
			}
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tIF_MEM_SIZE 0x%02x\n", arg8_0);
			break;
		case 0x73:	/* NV03+ */
			len = 9;
			err |= bios_u32(bios, offset+1, &arg32_0);
			err |= bios_u32(bios, offset+5, &arg32_1);
			if (err) {
				ENVY_BIOS_ERR("Init script out of bounds!\n");
				return;
			}
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tIF_STRAPS & 0x%08x == 0x%08x\n", arg32_0, arg32_1);
			break;
		case 0x72:	/* NV03+ */
			len = 1;
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tRESUME\n");
			break;
		case 0x63:	/* NV03+ */
			len = 1;
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tCOMPUTE_MEM\n");
			break;
		case 0x71:	/* NV01+ */
			len = 1;
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tQUIT\n");
			fprintf(out, "\n");
			return;
		case 0x70:	/* NV01:NV03 */
			len = 7;
			err |= bios_u16(bios, offset+1, &arg16_0);
			err |= bios_u32(bios, offset+3, &arg32_0);
			if (err) {
				ENVY_BIOS_ERR("Init script out of bounds!\n");
				return;
			}
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tDAC_PLL 0x%04x <= 0x%08x\n", arg16_0, arg32_0);
			break;
		case 0x64:	/* NV01:NV03 */
			len = 5;
			err |= bios_u16(bios, offset+1, &arg16_0);
			err |= bios_u8(bios, offset+3, &arg8_0);
			err |= bios_u8(bios, offset+4, &arg8_1);
			if (err) {
				ENVY_BIOS_ERR("Init script out of bounds!\n");
				return;
			}
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tDAC_MASK 0x%04x &= 0x%02x |= 0x%02x\n", arg16_0, arg8_0, arg8_1);
			break;
		case 0xff:	/* NV01:NV03 */
			len = 1;
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\tQUIT\n");
			fprintf(out, "\n");
			return;
		default:
			len = 1;
			dump_hex_script(bios, out, offset, len);
			fprintf(out, "\n");
			ENVY_BIOS_ERR("Unknown op 0x%02x in init script!\n", op);
			return;
		}
		offset += len;
	}
}

static int block_cmp(const void *va, const void *vb) {
	const struct envy_bios_block *a = va;
	const struct envy_bios_block *b = vb;
	if (a->start < b->start)
		return -1;
	if (a->start > b->start)
		return 1;
	return 0;
}

void envy_bios_print (struct envy_bios *bios, FILE *out, unsigned mask) {
	print_pcir(bios, out, mask);
	switch (bios->type) {
	case ENVY_BIOS_TYPE_UNKNOWN:
		if (mask & ENVY_BIOS_PRINT_VERSION)
			fprintf(out, "BIOS type: UNKNOWN!\n\n");
		break;
	case ENVY_BIOS_TYPE_NV01:
		if (mask & ENVY_BIOS_PRINT_VERSION)
			fprintf(out, "BIOS type: NV01\n\n");
		if (mask & ENVY_BIOS_PRINT_SCRIPTS) {
			/* XXX: how to find these properly? */
			fprintf(out, "Pre-mem scripts:\n\n");
			print_nv01_init_script(bios, out, 0x17bc, mask);
			print_nv01_init_script(bios, out, 0x17a2, mask);
			print_nv01_init_script(bios, out, 0x18f4, mask);
			fprintf(out, "1MB script:\n\n");
			print_nv01_init_script(bios, out, 0x199a, mask);
			fprintf(out, "2MB script:\n\n");
			print_nv01_init_script(bios, out, 0x19db, mask);
			fprintf(out, "4MB script:\n\n");
			print_nv01_init_script(bios, out, 0x1a1c, mask);
			fprintf(out, "Post-mem scripts:\n\n");
			print_nv01_init_script(bios, out, 0x198d, mask);
			print_nv01_init_script(bios, out, 0x1929, mask);
			fprintf(out, "Unknown scripts:\n\n");
			print_nv01_init_script(bios, out, 0x184f, mask);
		}
		break;
	case ENVY_BIOS_TYPE_NV03:
		if (mask & ENVY_BIOS_PRINT_VERSION)
			fprintf(out, "BIOS type: NV03\n\n");
		if (mask & ENVY_BIOS_PRINT_HWINFO) {
			fprintf(out, "Subsystem id: 0x%04x:0x%04x\n", bios->subsystem_vendor, bios->subsystem_device);
			envy_bios_dump_hex(bios, out, 0x54, 4, mask);
			fprintf(out, "\n");
		}
		print_bmp_nv03(bios, out, mask);
		if (bios->init_script)
			print_nv01_init_script(bios, out, bios->init_script, mask);
		break;
	case ENVY_BIOS_TYPE_NV04:
		if (mask & ENVY_BIOS_PRINT_VERSION)
			fprintf(out, "BIOS type: NV04\n\n");
		if (mask & ENVY_BIOS_PRINT_HWINFO) {
			fprintf(out, "Subsystem id: 0x%04x:0x%04x\n", bios->subsystem_vendor, bios->subsystem_device);
			envy_bios_dump_hex(bios, out, 0x54, 4, mask);
			if (bios->hwinfo_ext_valid) {
				fprintf(out, "Straps 0: select 0x%08x value 0x%08x\n", bios->straps0_select, bios->straps0_value);
				fprintf(out, "Straps 1: select 0x%08x value 0x%08x\n", bios->straps1_select, bios->straps1_value);
				envy_bios_dump_hex(bios, out, 0x58, 0x14, mask);
				int i, j;
				for (i = 0; i < bios->mmioinitsnum; i++) {
					fprintf(out, "Addr 0x%08x len %d\n", bios->mmioinits[i].addr, bios->mmioinits[i].len);
					for (j = 0; j < bios->mmioinits[i].len; j++)
						fprintf(out, "\tR[0x%08x] <= 0x%08x\n", bios->mmioinits[i].addr + j * 4, bios->mmioinits[i].data[j]);
				}
				envy_bios_dump_hex(bios, out, bios->mmioinit_offset, bios->mmioinit_len, mask);
			}
			fprintf(out, "\n");
			if (bios->hwea_offset) {
				fprintf(out, "HWEA at 0x%04x:\n", bios->hwea_offset);
				int i, j;
				for (i = 0; i < bios->hwea_entriesnum; i++) {
					struct envy_bios_hwea_entry *entry = &bios->hwea_entries[i];
					if (bios->hwea_version == 0) {
						for (j = 0; j < entry->len; j++) {
							fprintf(out, "\t");
							switch (entry->type) {
								case 0:
									fprintf(out, "C");
									break;
								case 1:
									fprintf(out, "D");
									break;
								default:
									fprintf(out, "UNK%02X", entry->type);
									break;
							}
							fprintf(out, "[0x%04x] <= 0x%08x\n", entry->base + j * 4, entry->data[j]);
						}
					} else {
						if (entry->type < 2) {
							for (j = 0; j < entry->len; j++) {
								fprintf(out, "\tR[0x%06x] <= 0x%08x\n", entry->base + j * 4 * !entry->type, entry->data[j]);
							}
						} else {
							fprintf(out, "\tR[0x%06x] &= 0x%08x |= 0x%08x\n", entry->base, entry->data[0], entry->data[1]);
						}
					}
				}
				envy_bios_dump_hex(bios, out, bios->hwea_offset, bios->hwea_len, mask);
				fprintf(out, "\n");
			}
		}
		envy_bios_print_bit(bios, stdout, mask);
		envy_bios_print_info(bios, stdout, mask);
		envy_bios_print_dacload(bios, stdout, mask);
		envy_bios_print_iunk21(bios, stdout, mask);
		envy_bios_print_i2cscript(bios, stdout, mask);

		envy_bios_print_dcb(bios, stdout, mask);
		envy_bios_print_odcb(bios, stdout, mask);
		envy_bios_print_i2c(bios, stdout, mask);
		envy_bios_print_gpio(bios, stdout, mask);
		envy_bios_print_dunk0c(bios, stdout, mask);
		envy_bios_print_dunk0e(bios, stdout, mask);
		envy_bios_print_dunk10(bios, stdout, mask);
		envy_bios_print_extdev(bios, stdout, mask);
		envy_bios_print_conn(bios, stdout, mask);
		envy_bios_print_dunk17(bios, stdout, mask);
		envy_bios_print_mux(bios, stdout, mask);
		break;
	}
	if (mask & ENVY_BIOS_PRINT_BLOCKS) {
		qsort(bios->blocks, bios->blocksnum, sizeof *bios->blocks, block_cmp);
		int i;
		unsigned last = 0;
		for (i = 0; i < bios->blocksnum; i++) {
			unsigned start = bios->blocks[i].start;
			unsigned end = start + bios->blocks[i].len;
			if (start > last) {
				fprintf(out, "0x%08x:0x%08x ???\n", last, start);
			}
			if (start < last) {
				fprintf(out, "overlap detected!\n");
			}
			fprintf(out, "0x%08x:0x%08x %s", start, end, bios->blocks[i].name);
			if (bios->blocks[i].idx != -1) {
				if (!strcmp(bios->blocks[i].name, "BIT"))
					fprintf(out, " '%c'", bios->blocks[i].idx);
				else
					fprintf(out, "[%d]", bios->blocks[i].idx);
			}
			fprintf(out, "\n");
			last = end;
		}
		fprintf(out, "\n");
	}
}
