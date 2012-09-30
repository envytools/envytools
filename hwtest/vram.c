/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "hwtest.h"
#include "nva.h"

uint32_t vram_rd32(int card, uint64_t addr) {
	if (nva_cards[card].card_type < 3) {
		return nva_rd32(card, 0x1000000 + addr);
	} else if (nva_cards[card].card_type < 0x30) {
		return nva_grd32(nva_cards[card].bar1, addr);
	} else if (nva_cards[card].card_type < 0x50) {
		nva_wr32(card, 0x1570, addr);
		return nva_rd32(card, 0x1574);
	} else {
		uint32_t old = nva_rd32(card, 0x1700);
		nva_wr32(card, 0x1700, addr >> 16);
		uint32_t res = nva_rd32(card, 0x700000 | (addr & 0xffff));
		nva_wr32(card, 0x1700, old);
		return res;
	}
}

void vram_wr32(int card, uint64_t addr, uint32_t val) {
	if (nva_cards[card].card_type < 3) {
		nva_wr32(card, 0x1000000 + addr, val);
	} else if (nva_cards[card].card_type < 0x30) {
		nva_gwr32(nva_cards[card].bar1, addr, val);
	} else if (nva_cards[card].card_type < 0x50) {
		nva_wr32(card, 0x1570, addr);
		nva_wr32(card, 0x1574, val);
	} else {
		uint32_t old = nva_rd32(card, 0x1700);
		nva_wr32(card, 0x1700, addr >> 16);
		nva_wr32(card, 0x700000 | (addr & 0xffff), val);
		nva_wr32(card, 0x1700, old);
		nva_wr32(card, 0x70000, 1);
		while (nva_rd32(card, 0x70000) & 2);
	}
}
