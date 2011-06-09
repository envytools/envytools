#include "nva.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define NV_PRAMIN_OFFSET            0x00700000
#define NV_PROM_OFFSET              0x00300000
#define NV03_PROM_SIZE              0x00010000
#define NV_PROM_SIZE                0x00020000

#define EOK 1
#define EUNK 0
#define ECRC -1
#define ESIG -2
#define ECARD -3

static int nv_cksum(const uint8_t *data, unsigned int length)
{
	/*
	 * There's a few checksums in the BIOS, so here's a generic checking
	 * function.
	 */
	int i;
	uint8_t sum = 0;

	for (i = 0; i < length; i++)
		sum += data[i];

	if (sum) {
		fprintf(stderr, "Invalid checksum. Broken vbios or broken retrieval method?\n");
		return ECRC;
	}
	return EOK;
}

static int nv_checksignature(const uint8_t *data)
{
	uint16_t pcir_ptr;

	/* bail if no rom signature */
	if (data[0] != 0x55 || data[1] != 0xaa) {
		fprintf(stderr, "Invalid signature(0x55aa). You may want to try another retrieval method.\n");
		return ESIG;
	}

	/* additional check (see note below) - read PCI record header */
	pcir_ptr = data[0x18] | data[0x19] << 8;
	if (data[pcir_ptr] != 'P' ||
	    data[pcir_ptr + 1] != 'C' ||
	    data[pcir_ptr + 2] != 'I' ||
	    data[pcir_ptr + 3] != 'R') {

		fprintf(stderr, "Invalid signature(PCIR). You may want to try another retrieval method.\n");
		return ESIG;
	}

	return EOK;
}

static int nv_ckbios(const uint8_t *data, int *length)
{
	uint16_t pcir_ptr;
	uint32_t ret = EUNK;
	int vbios_len, vbios2_len;

	ret = nv_checksignature(data);
	if (ret != EOK)
		return ret;

	vbios_len = data[2] * 512;
	ret = nv_cksum(data, vbios_len);

	if (length)
		*length = vbios_len;

	pcir_ptr = data[0x18] | data[0x19] << 8;

	/* Check for a second vbios */
	if (!(data[pcir_ptr + 0x15] & 0x80)) {

		fprintf(stderr, "Card has second bios\n");

		ret = nv_checksignature(data + vbios_len);
		if (ret != EOK)
			return ret;

		vbios2_len = data[vbios_len + 2] * 512;

		ret = nv_cksum(data + vbios_len, vbios2_len);

		if (length)
			*length += vbios2_len;
	}
}

/* vbios should at least be NV_PROM_SIZE bytes long */
int vbios_extract_prom(int cnum, uint8_t *vbios, int *length)
{
	uint32_t pci_cfg_50 = 0;
	uint32_t ret = EUNK;
	int i;

	fprintf(stderr, "Attempt to extract the vbios from card %i (nv%02x) using PROM\n",
			cnum, nva_cards[cnum].chipset);

	int32_t prom_offset;
	int32_t prom_size;

	if (nva_cards[cnum].chipset < 0x04) {
		prom_offset = 0x110000;
		prom_size = NV03_PROM_SIZE;
	} else {
		prom_offset = 0x300000;
		prom_size = NV_PROM_SIZE;
		pci_device_cfg_read_u32(nva_cards[cnum].pci, &pci_cfg_50, 0x50);
		pci_device_cfg_write_u32(nva_cards[cnum].pci, 0x0, 0x50);
	}

	/* on some 6600GT/6800LE prom reads are messed up.  nvclock alleges a
	 * a good read may be obtained by waiting or re-reading (cargocult: 5x)
	 * each byte.  we'll hope pramin has something usable instead
	 */

	for (i = 0; i < prom_size; i++)
		vbios[i] = nva_rd8(cnum, prom_offset + i);

	ret = nv_ckbios(vbios, length);
out:
	if (nva_cards[cnum].chipset >= 0x04)
		pci_device_cfg_write_u32(nva_cards[cnum].pci, pci_cfg_50, 0x50);
	return ret;
}

/* vbios should at least be NV_PROM_SIZE bytes long */
int vbios_extract_pramin(int cnum, uint8_t *vbios, int *length)
{
	uint32_t old_bar0_pramin = 0;
	uint32_t ret = EUNK;
	int i;

	if (nva_cards[cnum].chipset < 0x04) {
		fprintf(stderr, "Card %i (nv%02x) does not support PRAMIN!\n",
				cnum, nva_cards[cnum].chipset);
		return ECARD;
	}

	fprintf(stderr, "Attempt to extract the vbios from card %i (nv%02x) using PRAMIN\n",
			cnum, nva_cards[cnum].chipset);

	if (nva_cards[cnum].card_type >= 0x50) {
		uint32_t vbios_vram = (nva_rd32(cnum, 0x619f04) & ~0xff) << 8;

		if (!vbios_vram)
			vbios_vram = (nva_rd32(cnum, 0x1700) << 16) + 0xf0000;

		old_bar0_pramin = nva_rd32(cnum, 0x1700);
		nva_wr32(cnum, 0x1700, vbios_vram >> 16);
	}

	for (i = 0; i < NV_PROM_SIZE; i++)
		vbios[i] = nva_rd8(cnum, NV_PRAMIN_OFFSET + i);

	ret = nv_ckbios(vbios, length);
out:
	if (nva_cards[cnum].card_type >= 0x50)
		nva_wr32(cnum, 0x1700, old_bar0_pramin);

	return ret;
}

void usage(int error_code)
{
	fprintf(stderr, "\nUsage: nvagetbios [-c card_number] [-s extraction_method] > my_vbios.rom\n");
	exit(error_code);
}

int main(int argc, char **argv) {
	uint8_t vbios[NV_PROM_SIZE];
	int c;
	int cnum =0;
	char const *source = NULL;
	int result = 0;
	int length;

	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}

	/* Arguments parsing */
	while ((c = getopt (argc, argv, "hc:s:")) != -1)
		switch (c) {
			case 'h':
				usage(0);
				break;
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
			case 's':
				source = optarg;
				break;
		}
	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}

	if (source == NULL) {
		if (nva_cards[cnum].chipset < 4)
			source = "PROM";
		else
			source = "PRAMIN";
		fprintf(stderr, "No extraction method specified (using -s extraction_method). Defaulting to %s.\n", source);
	}

	/* Extraction */
	if (strcasecmp(source, "pramin") == 0)
		result = vbios_extract_pramin(cnum, vbios, &length);
	else if (strcasecmp(source, "prom") == 0)
		result = vbios_extract_prom(cnum, vbios, &length);
	else {
		fprintf(stderr, "Unknown vbios extraction method.\n");
		usage(1);
	}

	if (isatty(fileno(stdout))) {
		fprintf(stderr, "Refusing to write BIOS image to a terminal - please redirect output to a file.\n");
	} else {
		/* print the vbios on stdout */
		fwrite(vbios, 1, length, stdout);
	}

/*
	switch (result) {
		case EOK:
			fprintf(stderr, "Extraction done. Valid checksum.\n");
			break;
		case EUNK:
			fprintf(stderr, "An unknown error hapenned.\n");
			break;
		case ECRC:
			fprintf(stderr, "Invalid checksum. Broken vbios or broken retrieval method?\n");
			break;
		case ESIG:
			fprintf(stderr, "Invalid signature. You may want to try another retrieval method.\n");
			break;
		case ECARD:
			fprintf(stderr, "Method not supported by this card. Try another retrieval method.\n");
			break;
	}
*/
	return 0;
}
