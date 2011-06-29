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
	uint32_t ret = EUNK;
	int i = 0;

	if (nva_cards[cnum].chipset < 0x04) {
		return ECARD;
	}

	fprintf(stderr, "Attempt to upload the vbios to card %i (nv%02x) using PRAMIN\n",
			cnum, nva_cards[cnum].chipset);

	if (nva_cards[cnum].card_type >= 0x50) {
		uint32_t vbios_vram = (nva_rd32(cnum, 0x619f04) & ~0xff) << 8;

		if (!vbios_vram)
			vbios_vram = (nva_rd32(cnum, 0x1700) << 16) + 0xf0000;

		old_bar0_pramin = nva_rd32(cnum, 0x1700);
		nva_wr32(cnum, 0x1700, vbios_vram >> 16);
	}

	length = length < NV_PROM_SIZE ? length : NV_PROM_SIZE;

	for (i = 0; i < length; i++)
		nva_wr8(cnum, NV_PRAMIN_OFFSET + i, vbios[i]);

	ret = EOK;

out:
	if (nva_cards[cnum].card_type >= 0x50)
		nva_wr32(cnum, 0x1700, old_bar0_pramin);

	return ret;
}

int vbios_read(const char *filename, uint8_t **vbios, unsigned int *length)
{
	FILE * fd_bios;
	int i;

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

	/* Update the checksum */
	chksum(*vbios, *length);

	return EOK;
}

void usage(int error_code)
{
	fprintf(stderr, "\nUsage: nvafakebios [-c card_number] vbios.rom\n");
	exit(error_code);
}

int main(int argc, char **argv) {
	uint8_t *vbios = NULL;
	unsigned int vbios_length = 0;
	int c;
	int cnum = 0;
	int result = 0;

	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}

	/* Arguments parsing */
	while ((c = getopt (argc, argv, "hc:")) != -1)
		switch (c) {
			case 'h':
				usage(0);
				break;
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
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
