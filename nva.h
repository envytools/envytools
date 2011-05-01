#ifndef NVA_H
#define NVA_H
#include <stdint.h>

struct nva_card {
	struct pci_device *pci;
	uint32_t boot0;
	int chipset;
	int card_type;
	int is_nv03p;
	void *bar0;
};

int nva_init();
extern struct nva_card *nva_cards;
extern int nva_cardsnum;

static inline uint32_t nva_rd32(int card, uint32_t addr) {
	return *((volatile uint32_t*)(((volatile uint8_t *)nva_cards[card].bar0) + addr));
}

static inline void nva_wr32(int card, uint32_t addr, uint32_t val) {
	*((volatile uint32_t*)(((volatile uint8_t *)nva_cards[card].bar0) + addr)) = val;
}

static inline uint32_t nva_rd8(int card, uint32_t addr) {
	return *(((volatile uint8_t *)nva_cards[card].bar0) + addr);
}

static inline void nva_wr8(int card, uint32_t addr, uint32_t val) {
	*(((volatile uint8_t *)nva_cards[card].bar0) + addr) = val;
}

#endif
