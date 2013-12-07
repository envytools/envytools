/*
 * Copyright (C) 2011 Martin Peres <martin.peres@ensi-bourges.fr>
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "nva.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#define NV_PRAMIN_OFFSET            0x00700000
#define NV_PROM_SIZE                0x00010000

#define EOK 1
#define EUNK 0
#define ECRC -1
#define ESIG -2
#define EIOFAIL -3
#define EARG -4
#define ENOTVBIOS -5
#define ECARD -6

#define EDIT_SIZE 100

static void chksum(uint8_t *data, unsigned int length)
{
	uint16_t i;
	uint8_t sum = 0;

	data[length-1] = 0;
	for (i = 0; i < length; i++)
		sum += data[i];

	if (sum)
		sum = 256 - sum;

	data[length-1] = sum;
}

/* vbios should at least be NV_PROM_SIZE bytes long */
int vbios_upload_pramin(int cnum, uint8_t *vbios, int length)
{
	uint64_t old_bar0_pramin = 0;
	uint32_t ret = EUNK;
	int i = 0;

	if (nva_cards[cnum].chipset < 0x04) {
		return ECARD;
	}

	/* Update the checksum */
	chksum(vbios, length);

	fprintf(stderr, "Attempt to upload the vbios to card %i (nv%02x) using PRAMIN\n",
			cnum, nva_cards[cnum].chipset);

	if (nva_cards[cnum].card_type >= 0x50) {
		uint64_t vbios_vram = (uint64_t)(nva_rd32(cnum, 0x619f04) & ~0xff) << 8;

		if (!vbios_vram)
			vbios_vram =((uint64_t)nva_rd32(cnum, 0x1700) << 16) + 0xf0000;

		old_bar0_pramin = nva_rd32(cnum, 0x1700);
		nva_wr32(cnum, 0x1700, vbios_vram >> 16);
	}

	length = length < NV_PROM_SIZE ? length : NV_PROM_SIZE;

	for (i = 0; i < length; i++)
		nva_wr8(cnum, NV_PRAMIN_OFFSET + i, vbios[i]);

	ret = EOK;

	if (nva_cards[cnum].card_type >= 0x50)
		nva_wr32(cnum, 0x1700, old_bar0_pramin);

	return ret;
}

int vbios_read(const char *filename, uint8_t **vbios, size_t *length)
{
	FILE * fd_bios;

	if (!vbios || !length)
		return EARG;
	*vbios = NULL;
	*length = 0;

	fd_bios = fopen(filename ,"rb");
	if (!fd_bios)
		return EIOFAIL;

	/* get the size */
	fseek(fd_bios, 0, SEEK_END);
	*length = ftell(fd_bios);
	fseek(fd_bios, 0, SEEK_SET);

	if (*length <= 0)
		return EIOFAIL;

	/* Read the vbios */
	*vbios = malloc(*length * sizeof(char));
	if (fread(*vbios, 1, *length, fd_bios) < *length) {
		free(*vbios);
		return EIOFAIL;
	}
	fclose (fd_bios);

	/* vbios verification */
	if ((*vbios)[0] != 0x55 || (*vbios)[1] != 0xaa)
		return ENOTVBIOS;

	/* Set the right length */
	*length = (*vbios)[2] * 512;

	return EOK;
}

void usage(int error_code)
{
	fprintf(stderr, "\nUsage: nvafakebios [-c card_number] [-e offset:value] vbios.rom\n");
	exit(error_code);
}

enum print_type { HEX = 0, UDEC = 1 };
struct edit_offset {
	uint16_t offset;
	uint32_t val;
	size_t size;
	enum print_type type;
};

uint8_t bios_edit_u8(uint8_t *vbios, size_t vbios_length, unsigned int offs, uint8_t new) {
	uint8_t orig = 0;
	if (offs >= vbios_length) {
		fprintf(stderr, "requested OOB u8 at 0x%04x\n", offs);
		return 0;
	}
	orig = vbios[offs];
	vbios[offs] = new;
	return orig;
}

uint16_t bios_edit_u16(uint8_t *vbios, size_t vbios_length, unsigned int offs, uint16_t new) {
	uint16_t orig = 0;
	if (offs+1 >= vbios_length) {
		fprintf(stderr, "requested OOB u16 at 0x%04x\n", offs);
		return 0;
	}
	orig = vbios[offs] | vbios[offs+1] << 8;
	vbios[offs + 0] = (new >> 0) & 0xff;
	vbios[offs + 1] = (new >> 8) & 0xff;
	return orig;
}

uint32_t bios_edit_u32(uint8_t *vbios, size_t vbios_length, unsigned int offs, uint32_t new) {
	uint32_t orig = 0;
	if (offs+3 >= vbios_length) {
		fprintf(stderr, "requested OOB u32 at 0x%04x\n", offs);
		return 0;
	}
	orig = vbios[offs] | vbios[offs+1] << 8 | vbios[offs+2] << 16 | vbios[offs+3] << 24;
	vbios[offs + 0] = (new >> 0) & 0xff;
	vbios[offs + 1] = (new >> 8) & 0xff;
	vbios[offs + 2] = (new >> 16) & 0xff;
	vbios[offs + 3] = (new >> 24) & 0xff;
	return orig;
}

void edit_bios(uint8_t *vbios, size_t vbios_length, struct edit_offset *edit)
{
	uint32_t orig = 0;
	if (edit->size == 8) {
		edit->val &= 0xff;
		orig = bios_edit_u8(vbios, vbios_length, edit->offset, edit->val);
	} else if (edit->size == 16) {
		edit->val &= 0xffff;
		orig = bios_edit_u16(vbios, vbios_length, edit->offset, edit->val);
	} else if (edit->size == 32)
		orig = bios_edit_u32(vbios, vbios_length, edit->offset, edit->val);

	if (edit->type == HEX)
		printf("Edit offset 0x%x from 0x%x to 0x%x (hex, %lu bits)\n", edit->offset, orig, edit->val, edit->size);
	else if (edit->type == UDEC)
		printf("Edit offset 0x%x from %u to %u (dec, %lu bits)\n", edit->offset, orig, edit->val, edit->size);
	else
		printf("Unknown print type for edit offset 0x%x\n", edit->offset);
}

int main(int argc, char **argv) {
	uint8_t *vbios = NULL;
	size_t vbios_length = 0;
	int c, i, cnum = 0, result = 0;

	struct edit_offset edits[100] = { { 0, 0, HEX } };
	int e = 0;

	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}

	/* Arguments parsing */
	while ((c = getopt (argc, argv, "hc:e:E:w:W:l:L:")) != -1)
		switch (c) {
			case 'h':
				usage(0);
				break;
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
			case 'e':
				sscanf(optarg, "%hx:%x", &edits[e].offset, &edits[e].val);
				edits[e].size = 8;
				edits[e].type = HEX;
				e++;
				break;
			case 'E':
				sscanf(optarg, "%hx:%u", &edits[e].offset, &edits[e].val);
				edits[e].size = 8;
				edits[e].type = UDEC;
				e++;
				break;
			case 'w':
				sscanf(optarg, "%hx:%x", &edits[e].offset, &edits[e].val);
				edits[e].size = 16;
				edits[e].type = HEX;
				e++;
				break;
			case 'W':
				sscanf(optarg, "%hx:%u", &edits[e].offset, &edits[e].val);
				edits[e].size = 16;
				edits[e].type = UDEC;
				e++;
				break;
			case 'l':
				sscanf(optarg, "%hx:%x", &edits[e].offset, &edits[e].val);
				edits[e].size = 32;
				edits[e].type = HEX;
				e++;
				break;
			case 'L':
				sscanf(optarg, "%hx:%u", &edits[e].offset, &edits[e].val);
				edits[e].size = 32;
				edits[e].type = UDEC;
				e++;
				break;
			default:
				usage(1);
		}
	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}

	if (optind >= argc) {
		fprintf (stderr, "No vbios specified.\n");
		return 1;
	}

	/* Read the vbios */
	result = vbios_read(argv[optind], &vbios, &vbios_length);
	if (result != EOK)
		goto out;

	/* do the edits */
	for (i = 0; i < e; i++)
		edit_bios(vbios, vbios_length, &edits[i]);

	/* Upload */
	result = vbios_upload_pramin(cnum, vbios, vbios_length);

out:
	switch (result) {
		case EOK:
			fprintf(stderr, "Upload done.\n");
			break;
		case EIOFAIL:
			fprintf(stderr, "Cannot read the vbios \"%s\".\n", argv[optind]);
			break;
		case ENOTVBIOS:
			fprintf(stderr, "The file \"%s\" isn't a valid vbios.\n", argv[optind]);
			break;
		case ECARD:
			fprintf(stderr, "Upload not possible on this card.\n");
			break;
		case EUNK:
		default:
			fprintf(stderr, "An unknown error hapenned.\n");
			break;
	}

	return 0;
}
