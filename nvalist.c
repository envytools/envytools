#include "nva.h"
#include <stdio.h>
#include <assert.h>
#include <pciaccess.h>

int main() {
	assert(!nva_init());
	int i;
	for (i = 0; i < nva_cardsnum; i++)
		printf ("%d: %04x:%02x:%02x.%x NV%02X%s %08x\n", i,
				nva_cards[i].pci->domain, nva_cards[i].pci->bus, nva_cards[i].pci->dev, nva_cards[i].pci->func,
			       	nva_cards[i].chipset, (nva_cards[i].is_nv03p?"P":""), nva_cards[i].boot0);
	return 0;
}
