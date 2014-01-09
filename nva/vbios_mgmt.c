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
	uint32_t old_bar0_pramin = 0;
	int i = 0;

	if (nva_cards[cnum].chipset.chipset < 0x04) {
		return ECARD;
	}

	fprintf(stderr, "Attempt to upload the vbios to card %i (nv%02x) using PRAMIN\n",
			cnum, nva_cards[cnum].chipset.chipset);

	/* Update the checksum */
	chksum(vbios, length);

	if (nva_cards[cnum].chipset.card_type >= 0x50) {
		uint32_t vbios_vram = (nva_rd32(cnum, 0x619f04) & ~0xff) << 8;

		if (!vbios_vram)
			vbios_vram = (nva_rd32(cnum, 0x1700) << 16) + 0xf0000;

		old_bar0_pramin = nva_rd32(cnum, 0x1700);
		nva_wr32(cnum, 0x1700, vbios_vram >> 16);
	}

	length = length < NV_PROM_SIZE ? length : NV_PROM_SIZE;

	for (i = 0; i < length; i++)
		nva_wr8(cnum, NV_PRAMIN_OFFSET + i, vbios[i]);

	if (nva_cards[cnum].chipset.card_type >= 0x50)
		nva_wr32(cnum, 0x1700, old_bar0_pramin);

	return EOK;
}

int vbios_read(const char *filename, uint8_t **vbios, uint16_t *length)
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
