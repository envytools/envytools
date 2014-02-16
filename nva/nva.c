/*
 * Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
 * Copyright (C) 2011 Martin Peres <martin.peres@ensi-bourges.fr>
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

#include <pciaccess.h>
#include <stdio.h>
#include <stdlib.h>
#include "nva.h"
#include "util.h"

struct nva_card **nva_cards = 0;
int nva_cardsnum = 0;
int nva_cardsmax = 0;
int nva_vgaarberr = 0;

struct pci_id_match nv_match[] = {
	{0x104a, 0x0009, PCI_MATCH_ANY, PCI_MATCH_ANY, 0, 0},
	{0x12d2, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, 0x30000, 0xffff0000},
	{0x10de, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, 0x30000, 0xffff0000},
	{0x10de, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, 0x48000, 0xffffff00},
};

struct nva_card *nva_init_gpu(struct pci_device *dev) {
	struct nva_card *card = calloc(sizeof *card, 1);
	if (!card)
		return 0;
	card->type = NVA_DEVICE_GPU;
	card->pci = dev;
	int ret = pci_device_map_range(dev, dev->regions[0].base_addr, dev->regions[0].size, PCI_DEV_MAP_FLAG_WRITABLE, &card->bar0);
	if (ret) {
		fprintf (stderr, "WARN: Can't probe %04x:%02x:%02x.%x\n", dev->domain, dev->bus, dev->dev, dev->func);
		free(card);
		return 0;
	}
	card->bar0len = dev->regions[0].size;
	if (dev->regions[1].size) {
		card->hasbar1 = 1;
		card->bar1len = dev->regions[1].size;
		ret = pci_device_map_range(dev, dev->regions[1].base_addr, dev->regions[1].size, PCI_DEV_MAP_FLAG_WRITABLE, &card->bar1);
		if (ret) {
			card->bar1 = 0;
		}
	}
	if (dev->regions[2].size && !dev->regions[2].is_IO) {
		card->hasbar2 = 1;
		card->bar2len = dev->regions[2].size;
		ret = pci_device_map_range(dev, dev->regions[2].base_addr, dev->regions[2].size, PCI_DEV_MAP_FLAG_WRITABLE, &card->bar2);
		if (ret) {
			card->bar2 = 0;
		}
	} else if (dev->regions[3].size) {
		card->hasbar2 = 1;
		card->bar2len = dev->regions[3].size;
		ret = pci_device_map_range(dev, dev->regions[3].base_addr, dev->regions[3].size, PCI_DEV_MAP_FLAG_WRITABLE, &card->bar2);
		if (ret) {
			card->bar2 = 0;
		}
	}
	/* ignore errors */
	pci_device_map_legacy(dev, 0, 0x100000, PCI_DEV_MAP_FLAG_WRITABLE, &card->rawmem);
	card->rawio = pci_legacy_open_io(dev, 0, 0x10000);
	int iobar = -1;
	if (dev->regions[2].size && dev->regions[2].is_IO)
		iobar = 2;
	if (dev->regions[5].size && dev->regions[5].is_IO)
		iobar = 5;
	if (iobar != -1) {
		card->iobar = pci_device_open_io(dev, dev->regions[iobar].base_addr, dev->regions[iobar].size);
		card->iobarlen = dev->regions[iobar].size;
	}
	uint32_t pmc_id = nva_grd32(card->bar0, 0);
	parse_pmc_id(pmc_id, &card->chipset);
	return card;
}

int nva_init() {
	int ret;
	ret = pci_system_init();
	if (ret)
		return -1;
	nva_vgaarberr = pci_device_vgaarb_init();
	int i;

	for (i = 0; i < ARRAY_SIZE(nv_match); i++) {
		struct pci_device_iterator* it = pci_id_match_iterator_create(&nv_match[i]);
		if (!it) {
			pci_system_cleanup();
			return -1;
		}

		struct pci_device *dev;
		while ((dev = pci_device_next(it))) {
			ret = pci_device_probe(dev);
			if (ret) {
				fprintf (stderr, "WARN: Can't probe %04x:%02x:%02x.%x\n", dev->domain, dev->bus, dev->dev, dev->func);
				continue;
			}
			pci_device_enable(dev);
			struct nva_card *card = nva_init_gpu(dev);
			if (card)
				ADDARRAY(nva_cards, card);
		}
		pci_iterator_destroy(it);
	}
	return (nva_cardsnum == 0);
}
