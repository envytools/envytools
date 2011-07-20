#include <pciaccess.h>
#include <stdio.h>
#include <stdlib.h>
#include "nva.h"
#include "util.h"

struct nva_card *nva_cards = 0;
int nva_cardsnum = 0;
int nva_cardsmax = 0;

int nva_init() {
	int ret;
	ret = pci_system_init();
	if (ret)
		return -1;
	struct pci_id_match nv_match = {0x10de, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, 0x30000, 0xffff0000};
	struct pci_device_iterator* it = pci_id_match_iterator_create(&nv_match);
	if (!it) {
		pci_system_cleanup();
		return -1;
	}

	struct pci_device *dev;
	while (dev = pci_device_next(it)) {
		struct nva_card c = { 0 };
		ret = pci_device_probe(dev);
		if (ret) {
			fprintf (stderr, "WARN: Can't probe %04x:%02x:%02x.%x\n", dev->domain, dev->bus, dev->dev, dev->func);
			continue;
		}
		c.pci = dev;
		ADDARRAY(nva_cards, c);
	}
	pci_iterator_destroy(it);

	struct pci_id_match nv_sgs_match = {0x12d2, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, 0x30000, 0xffff0000};
	it = pci_id_match_iterator_create(&nv_sgs_match);
	if (!it) {
		pci_system_cleanup();
		return -1;
	}

	while (dev = pci_device_next(it)) {
		struct nva_card c = { 0 };
		ret = pci_device_probe(dev);
		if (ret) {
			fprintf (stderr, "WARN: Can't probe %04x:%02x:%02x.%x\n", dev->domain, dev->bus, dev->dev, dev->func);
			continue;
		}
		c.pci = dev;
		ADDARRAY(nva_cards, c);
	}
	pci_iterator_destroy(it);

	int i;
	for (i = 0; i < nva_cardsnum; i++) {
		dev = nva_cards[i].pci;
		ret = pci_device_map_range(dev, dev->regions[0].base_addr, dev->regions[0].size, PCI_DEV_MAP_FLAG_WRITABLE, &nva_cards[i].bar0);
		if (ret)
			return -1;
		nva_cards[i].boot0 = nva_rd32(i, 0);
		nva_cards[i].chipset = nva_cards[i].boot0 >> 20 & 0xff;
		if (nva_cards[i].chipset < 0x10) {
			if (nva_cards[i].boot0 & 0xf000) {
				if (nva_cards[i].boot0 & 0xf00000)
					nva_cards[i].chipset = 4;
				else
					nva_cards[i].chipset = 5;
			} else {
				nva_cards[i].chipset = nva_cards[i].boot0 >> 16 & 0xf;
				if ((nva_cards[i].boot0 & 0xff) >= 0x20)
					nva_cards[i].is_nv03p = 1;
			}
		}

		if (nva_cards[i].chipset < 0x04)
			nva_cards[i].card_type = nva_cards[i].chipset;
		else if (nva_cards[i].chipset < 0x10)
			nva_cards[i].card_type = 0x04;
		else if (nva_cards[i].chipset < 0x20)
			nva_cards[i].card_type = 0x10;
		else if (nva_cards[i].chipset < 0x30)
			nva_cards[i].card_type = 0x20;
		else if (nva_cards[i].chipset < 0x40)
			nva_cards[i].card_type = 0x30;
		else if (nva_cards[i].chipset < 0x50 ||
			nva_cards[i].chipset & 0xf0 == 0x60)
			nva_cards[i].card_type = 0x40;
		else if (nva_cards[i].chipset < 0xc0)
			nva_cards[i].card_type = 0x50;
		else
			nva_cards[i].card_type = 0xc0;
	}
	return 0;
}
