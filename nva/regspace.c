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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "nva.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

int nva_wr(struct nva_regspace *regspace, uint32_t addr, uint64_t val) {
	void *rawbase = 0;
	size_t rawlen;
	void *raw;
	int i;
	uint32_t vgaio;
	uint32_t vgabase;
	switch (regspace->type) {
		case NVA_REGSPACE_BAR0:
			rawbase = regspace->card->bar0;
			rawlen = regspace->card->bar0len;
			goto raw;
		case NVA_REGSPACE_BAR1:
			rawbase = regspace->card->bar1;
			rawlen = regspace->card->bar1len;
			if (!regspace->card->hasbar1)
				return NVA_ERR_NOSPC;
			goto raw;
		case NVA_REGSPACE_BAR2:
			rawbase = regspace->card->bar2;
			rawlen = regspace->card->bar2len;
			if (!regspace->card->hasbar2)
				return NVA_ERR_NOSPC;
			goto raw;
		raw:
			if (!rawbase)
				return NVA_ERR_MAP;
			if (addr > rawlen - regspace->regsz)
				return NVA_ERR_RANGE;
			raw = (uint8_t *)rawbase + addr;
			switch (regspace->regsz) {
				case 1:
					*(volatile uint8_t *)raw = val;
					return 0;
				case 2:
					*(volatile uint16_t *)raw = val;
					return 0;
				case 4:
					*(volatile uint32_t *)raw = val;
					return 0;
				case 8:
					*(volatile uint64_t *)raw = val;
					return 0;
				default:
					return NVA_ERR_REGSZ;
			}
		case NVA_REGSPACE_PDAC:
			if (regspace->card->chipset != 0x01)
				return NVA_ERR_NOSPC;
			if (addr > 0x10000 - regspace->regsz)
				return NVA_ERR_RANGE;
			if (regspace->regsz > 8)
				return NVA_ERR_REGSZ;
			nva_wr32(regspace->cnum, 0x609010, addr & 0xff);
			nva_wr32(regspace->cnum, 0x609014, addr >> 8);
			for (i = 0; i < regspace->regsz; i++)
				nva_wr32(regspace->cnum, 0x609018, (val >> i * 8) & 0xff);
			return 0;
		case NVA_REGSPACE_VGA_CR:
			vgaio = 0x3d4;
			if (addr >= 0x100)
				return NVA_ERR_RANGE;
			goto vga;
		case NVA_REGSPACE_VGA_SR:
			vgaio = 0x3c4;
			if (addr >= 8)
				return NVA_ERR_RANGE;
			goto vga;
		case NVA_REGSPACE_VGA_GR:
			vgaio = 0x3ce;
			if (addr >= 0x10)
				return NVA_ERR_RANGE;
			goto vga;
		case NVA_REGSPACE_VGA_AR:
			vgaio = 0x3c0;
			if (addr >= 0x20)
				return NVA_ERR_RANGE;
			goto vga;
		vga:
			if (regspace->regsz != 1)
				return NVA_ERR_REGSZ;
			if (regspace->card->card_type == 0x01) {
				vgabase = 0x6d0000;
				if (regspace->idx != 0)
					return NVA_ERR_NOSPC;
			} else if (regspace->card->card_type < 0x50) {
				if (vgaio == 0x3c4 || vgaio == 0x3ce)
					vgabase = 0x0c0000;
				else
					vgabase = 0x601000;
				if (regspace->idx > 2)
					return NVA_ERR_NOSPC;
				if ((regspace->card->chipset < 0x17 || regspace->card->chipset == 0x1a || regspace->card->chipset == 0x20) && regspace->idx == 1)
					return NVA_ERR_NOSPC;
				vgabase += regspace->idx * 0x2000;
			} else {
				vgabase = 0x601000;
				if (regspace->idx != 0)
					return NVA_ERR_NOSPC;
			}
			if (vgaio == 0x3c0) {
				nva_rd8(regspace->cnum, vgabase + 0x3da);
				uint8_t idx = nva_rd8(regspace->cnum, vgabase + 0x3c0);
				nva_rd8(regspace->cnum, vgabase + 0x3da);
				nva_wr8(regspace->cnum, vgabase + 0x3c0, addr);
				nva_wr8(regspace->cnum, vgabase + 0x3c0, val);
				nva_wr8(regspace->cnum, vgabase + 0x3c0, idx);
			} else {
				uint8_t idx = nva_rd8(regspace->cnum, vgabase + vgaio);
				nva_wr8(regspace->cnum, vgabase + vgaio, addr);
				nva_wr8(regspace->cnum, vgabase + vgaio + 1, val);
				nva_wr8(regspace->cnum, vgabase + vgaio, idx);
			}
			return 0;
		default:
			return NVA_ERR_NOSPC;
	}
}

int nva_rd(struct nva_regspace *regspace, uint32_t addr, uint64_t *val) {
	void *rawbase = 0;
	size_t rawlen;
	void *raw;
	int i;
	uint32_t vgaio;
	uint32_t vgabase;
	switch (regspace->type) {
		case NVA_REGSPACE_BAR0:
			rawbase = regspace->card->bar0;
			rawlen = regspace->card->bar0len;
			goto raw;
		case NVA_REGSPACE_BAR1:
			rawbase = regspace->card->bar1;
			rawlen = regspace->card->bar1len;
			goto raw;
		case NVA_REGSPACE_BAR2:
			rawbase = regspace->card->bar2;
			rawlen = regspace->card->bar2len;
			goto raw;
		raw:
			if (!rawbase)
				return NVA_ERR_MAP;
			if (addr > rawlen - regspace->regsz)
				return NVA_ERR_RANGE;
			raw = (uint8_t *)rawbase + addr;
			switch (regspace->regsz) {
				case 1:
					*val = *(volatile uint8_t *)raw;
					return 0;
				case 2:
					*val = *(volatile uint16_t *)raw;
					return 0;
				case 4:
					*val = *(volatile uint32_t *)raw;
					return 0;
				case 8:
					*val = *(volatile uint64_t *)raw;
					return 0;
				default:
					return NVA_ERR_REGSZ;
			}
		case NVA_REGSPACE_PDAC:
			if (regspace->card->chipset != 0x01)
				return NVA_ERR_NOSPC;
			if (addr > 0x10000 - regspace->regsz)
				return NVA_ERR_RANGE;
			if (regspace->regsz > 8)
				return NVA_ERR_REGSZ;
			nva_wr32(regspace->cnum, 0x609010, addr & 0xff);
			nva_wr32(regspace->cnum, 0x609014, addr >> 8);
			*val = 0;
			for (i = 0; i < regspace->regsz; i++)
				*val |= (uint64_t)(nva_rd32(regspace->cnum, 0x609018) & 0xff) << i * 8;
			return 0;
		case NVA_REGSPACE_VGA_CR:
			vgaio = 0x3d4;
			if (addr >= 0x100)
				return NVA_ERR_RANGE;
			goto vga;
		case NVA_REGSPACE_VGA_SR:
			vgaio = 0x3c4;
			if (addr >= 8)
				return NVA_ERR_RANGE;
			goto vga;
		case NVA_REGSPACE_VGA_GR:
			vgaio = 0x3ce;
			if (addr >= 0x10)
				return NVA_ERR_RANGE;
			goto vga;
		case NVA_REGSPACE_VGA_AR:
			vgaio = 0x3c0;
			if (addr >= 0x20)
				return NVA_ERR_RANGE;
			goto vga;
		vga:
			if (regspace->regsz != 1)
				return NVA_ERR_REGSZ;
			if (regspace->card->card_type == 0x01) {
				vgabase = 0x6d0000;
				if (regspace->idx != 0)
					return NVA_ERR_NOSPC;
			} else if (regspace->card->card_type < 0x50) {
				if (vgaio == 0x3c4 || vgaio == 0x3ce)
					vgabase = 0x0c0000;
				else
					vgabase = 0x601000;
				if (regspace->idx > 2)
					return NVA_ERR_NOSPC;
				if ((regspace->card->chipset < 0x17 || regspace->card->chipset == 0x1a || regspace->card->chipset == 0x20) && regspace->idx == 1)
					return NVA_ERR_NOSPC;
				vgabase += regspace->idx * 0x2000;
			} else {
				vgabase = 0x601000;
				if (regspace->idx != 0)
					return NVA_ERR_NOSPC;
			}
			if (vgaio == 0x3c0) {
				nva_rd8(regspace->cnum, vgabase + 0x3da);
				uint8_t idx = nva_rd8(regspace->cnum, vgabase + 0x3c0);
				nva_rd8(regspace->cnum, vgabase + 0x3da);
				nva_wr8(regspace->cnum, vgabase + 0x3c0, addr);
				*val = nva_rd8(regspace->cnum, vgabase + 0x3c1);
				nva_rd8(regspace->cnum, vgabase + 0x3da);
				nva_wr8(regspace->cnum, vgabase + 0x3c0, idx);
			} else {
				uint8_t idx = nva_rd8(regspace->cnum, vgabase + vgaio);
				nva_wr8(regspace->cnum, vgabase + vgaio, addr);
				*val = nva_rd8(regspace->cnum, vgabase + vgaio + 1);
				nva_wr8(regspace->cnum, vgabase + vgaio, idx);
			}
			return 0;
		default:
			return NVA_ERR_NOSPC;
	}
}

int nva_rstype(const char *name) {
	if (!strcmp(name, "bar0"))
		return NVA_REGSPACE_BAR0;
	if (!strcmp(name, "mmio"))
		return NVA_REGSPACE_BAR0;
	if (!strcmp(name, "bar1"))
		return NVA_REGSPACE_BAR1;
	if (!strcmp(name, "fb"))
		return NVA_REGSPACE_BAR1;
	if (!strcmp(name, "bar2"))
		return NVA_REGSPACE_BAR2;
	if (!strcmp(name, "bar3"))
		return NVA_REGSPACE_BAR2;
	if (!strcmp(name, "ramin"))
		return NVA_REGSPACE_BAR2;
	if (!strcmp(name, "pdac"))
		return NVA_REGSPACE_PDAC;
	if (!strcmp(name, "cr"))
		return NVA_REGSPACE_VGA_CR;
	if (!strcmp(name, "sr"))
		return NVA_REGSPACE_VGA_SR;
	if (!strcmp(name, "gr"))
		return NVA_REGSPACE_VGA_GR;
	if (!strcmp(name, "ar"))
		return NVA_REGSPACE_VGA_AR;
	if (!strcmp(name, "cr"))
		return NVA_REGSPACE_VGA_CR;
	return -1;
}

int nva_rsdefsz(struct nva_regspace *regspace) {
	switch (regspace->type) {
		case NVA_REGSPACE_BAR0:
		case NVA_REGSPACE_BAR1:
		case NVA_REGSPACE_BAR2:
			return 4;
		default:
			return 1;
	}
}

char nva_rserrc(enum nva_err err) {
	switch (err) {
		case NVA_ERR_RANGE:
			return 'R';
		case NVA_ERR_REGSZ:
			return 'B';
		case NVA_ERR_NOSPC:
			return 'S';
		case NVA_ERR_MAP:
			return 'M';
		default:
			abort();
	}
}

void nva_rsprint(struct nva_regspace *regspace, enum nva_err err, uint64_t val) {
	if (err) {
		int k;
		char ec = nva_rserrc(err);
		printf(" ");
		for (k = 0; k < regspace->regsz; k++)
			printf("%c%c", ec, ec);
	} else {
		switch (regspace->regsz) {
			case 1:
				printf (" %02"PRIx64, val);
				break;
			case 2:
				printf (" %04"PRIx64, val);
				break;
			case 4:
				printf (" %08"PRIx64, val);
				break;
			case 8:
				printf (" %016"PRIx64, val);
				break;
		}
	}
}
