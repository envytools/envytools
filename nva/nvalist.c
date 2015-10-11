/*
 * Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
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
#include <stdlib.h>
#include <pciaccess.h>

void list_gpu(struct nva_card *card) {
	printf (" %s %08x\n", card->chipset.name, card->chipset.pmc_id);
}

void list_apu(struct nva_card *card) {
	printf (" APU\n");
}

void list_smu(struct nva_card *card) {
	printf (" SMU\n");
}

int main() {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int i;
	for (i = 0; i < nva_cardsnum; i++) {
		struct nva_card *card = nva_cards[i];
		printf ("%d: ", i);
		switch (card->bus_type) {
		case NVA_BUS_PCI:
			printf ("(pci) %04x:%02x:%02x.%x",
				card->bus.pci->domain, card->bus.pci->bus,
				card->bus.pci->dev, card->bus.pci->func);
			break;
		case NVA_BUS_PLATFORM:
			printf ("(platform) 0x%08x", card->bus.platform_address);
			break;
		}

		switch (card->type) {
			case NVA_DEVICE_GPU:
				list_gpu(card);
				break;
			case NVA_DEVICE_SMU:
				list_smu(card);
				break;
			case NVA_DEVICE_APU:
				list_apu(card);
				break;
			default:
				abort();
		}
	}
	return 0;
}
