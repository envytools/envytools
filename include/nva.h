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

#ifndef NVA_H
#define NVA_H
#include <stdint.h>
#include <stddef.h>

struct nva_card {
	struct pci_device *pci;
	uint32_t boot0;
	int chipset;
	int card_type;
	int is_nv03t;
	void *bar0;
	size_t bar0len;
	int hasbar1;
	void *bar1;
	size_t bar1len;
	int hasbar2;
	void *bar2;
	size_t bar2len;
};

int nva_init();
extern struct nva_card *nva_cards;
extern int nva_cardsnum;

static inline uint32_t nva_grd32(void *base, uint32_t addr) {
	return *((volatile uint32_t*)(((volatile uint8_t *)base) + addr));
}

static inline void nva_gwr32(void *base, uint32_t addr, uint32_t val) {
	*((volatile uint32_t*)(((volatile uint8_t *)base) + addr)) = val;
}

static inline uint32_t nva_grd8(void *base, uint32_t addr) {
	return *(((volatile uint8_t *)base) + addr);
}

static inline void nva_gwr8(void *base, uint32_t addr, uint32_t val) {
	*(((volatile uint8_t *)base) + addr) = val;
}

static inline uint32_t nva_rd32(int card, uint32_t addr) {
	return nva_grd32(nva_cards[card].bar0, addr);
}

static inline void nva_wr32(int card, uint32_t addr, uint32_t val) {
	nva_gwr32(nva_cards[card].bar0, addr, val);
}

static inline uint32_t nva_rd8(int card, uint32_t addr) {
	return nva_grd8(nva_cards[card].bar0, addr);
}

static inline void nva_wr8(int card, uint32_t addr, uint32_t val) {
	nva_gwr8(nva_cards[card].bar0, addr, val);
}

static inline uint32_t nva_mask(int cnum, uint32_t reg, uint32_t mask, uint32_t val)
{
	uint32_t tmp = nva_rd32(cnum, reg);
	nva_wr32(cnum, reg, (tmp & ~mask) | val);
	return tmp;
}

struct nva_regspace {
	struct nva_card *card;
	int cnum;
	enum {
		NVA_REGSPACE_UNKNOWN,
		NVA_REGSPACE_BAR0,
		NVA_REGSPACE_BAR1,
		NVA_REGSPACE_BAR2,
		NVA_REGSPACE_PDAC,
		NVA_REGSPACE_EEPROM,
		NVA_REGSPACE_VGA_CR,
		NVA_REGSPACE_VGA_SR,
		NVA_REGSPACE_VGA_AR,
		NVA_REGSPACE_VGA_GR,
		NVA_REGSPACE_VGA_ST,
		NVA_REGSPACE_PIPE,
		NVA_REGSPACE_RDI,
		NVA_REGSPACE_VCOMP_CODE,
		NVA_REGSPACE_VCOMP_REG,
		NVA_REGSPACE_MACRO_CODE,
		NVA_REGSPACE_XT,
	} type;
	int regsz;
	int idx;
};

int nva_wr(struct nva_regspace *regspace, uint32_t addr, uint64_t val);
int nva_rd(struct nva_regspace *regspace, uint32_t addr, uint64_t *val);

enum nva_err {
	NVA_ERR_SUCCESS,
	NVA_ERR_RANGE,
	NVA_ERR_REGSZ,
	NVA_ERR_NOSPC,
	NVA_ERR_MAP,
};

int nva_rstype(const char *name);
int nva_rsdefsz(struct nva_regspace *regspace);
char nva_rserrc(enum nva_err err);
void nva_rsprint(struct nva_regspace *regspace, enum nva_err err, uint64_t val);

#endif
