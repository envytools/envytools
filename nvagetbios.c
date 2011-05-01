#include "nva.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#define NV_PRAMIN_OFFSET            0x00700000
#define NV_PROM_SIZE                0x00010000

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

	if (sum)
		return 0;

	return 1;
}

int vbios_extract_pramin(int cnum)
{
	uint8_t vbios[NV_PROM_SIZE];
	uint32_t old_bar0_pramin = 0;
	uint32_t ret = 0;
	int i;

	fprintf(stderr, "Attempts to extract the vbios from the card %i (nv%02x) using PRAMIN\n",
			cnum, nva_cards[cnum].chipset);

	if (nva_cards[cnum].chipset >= 0x50) {
		uint32_t vbios_vram = (nva_rd32(cnum, 0x619f04) & ~0xff) << 8;

		if (!vbios_vram)
			vbios_vram = (nva_rd32(cnum, 0x1700) << 16) + 0xf0000;

		old_bar0_pramin = nva_rd32(cnum, 0x1700);
		nva_wr32(cnum, 0x1700, vbios_vram >> 16);
	}

	/* bail if no rom signature */
	if (nva_rd8(cnum, NV_PRAMIN_OFFSET) != 0x55 ||
	    nva_rd8(cnum, NV_PRAMIN_OFFSET + 1) != 0xaa)
		goto out;

	for (i = 0; i < NV_PROM_SIZE; i++)
		vbios[i] = nva_rd8(cnum, NV_PRAMIN_OFFSET + i);

	ret = nv_cksum(vbios, vbios[2] * 512);
	if (ret)
		fwrite(vbios, 1, vbios[2] * 512, stdout);

out:
	if (nva_cards[cnum].chipset >= 0x50)
		nva_wr32(cnum, 0x1700, old_bar0_pramin);

	return ret;
}

void usage(int error_code)
{
	fprintf(stderr, "\nUsage: nvagetbios [-c card_number] -s extraction_method > my_vbios.rom\n");
	exit(error_code);
}

int main(int argc, char **argv) {
	assert(!nva_init());
	int c;
	int cnum =0;
	char source[21];
	int success = 0;

	while ((c = getopt (argc, argv, "hc:s:")) != -1)
		switch (c) {
			case 'h':
				usage(0);
				break;
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
			case 's':
				strncpy(source, optarg, 20);
				break;
		}
	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}

	if (strcasecmp(source, "pramin") == 0)
		success = vbios_extract_pramin(cnum);
	else {
		fprintf(stderr, "Unknown vbios extraction method.\n");
		usage(1);
	}

	if (success) {
		fprintf(stderr, "Extraction done. Valid checksum.\n");
		return 2;
	} else {
		fprintf(stderr, "Invalid checksum. Try another retrieval method.\n");
		return 2;
	}

	return 0;
}