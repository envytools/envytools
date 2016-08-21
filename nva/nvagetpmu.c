/*
 * Copyright 2011,2016 Roy Spliet <nouveau@spliet.org>
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

void help()
{
	printf("Retreives PMU from the GPU.\nJust run, and pipe output to file.\n");
}

int32_t peek(int32_t reg)
{
	if ((reg < 0x400300 || reg >= 0x400400) ||
			(nva_rd32(0,0)&0x0e000000) != 0x02000000) {
		return nva_rd32(0,reg);
	} else {
		return 0;
	}
}

void poke(uint32_t reg, uint32_t value)
{
	nva_wr32(0,reg,value);
}

int g80_pte_from_pde(uint64_t chan_ptr, uint64_t *pte, int *hostmem)
{
	uint32_t hi, lo;
	uint32_t pde;

	hi = (chan_ptr >> 16) & 0xffffff;
	lo = (chan_ptr & 0x0000ffff);

	poke(0x1700, hi | 0x2000000);
	pde = peek(0x700000+lo+0x208);
	if ((pde & 0xff3) != 0x63) {
		fprintf(stderr,"Page directory entry invalid\n");
		return -1;
	}

	switch ((pde & 0xc) >> 2) {
	case 0:
		*hostmem = 0;
		break;
	case 3:
		*hostmem = 1;
		break;
	default:
		fprintf(stderr, "Page table in unknown memory\n");
		return -1;
		break;
	}

	hi = peek(0x700000+lo+0x20c);
	hi &= 0x000000ff;
	*pte = ((uint64_t)hi << 32ull) | (pde & 0xfffff000);

	return 0;
}

/*
 * XXX: Take card number as parameter
 * XXX: Does this generalise to other falcon engines too?
 */

int main(int argc, char **argv)
{
	int i = 0, j = 0;
	uint32_t tmp_reg, tmp_mask = 0;
	uint32_t boot0;
	uint64_t chan_ptr, pte;
	int hostmem;
	int ret;

	uint32_t hi = 0,lo = 0;
	int32_t ptable[256];
	int ptable_size = 0;

	if (nva_init()) {
		printf("Init failed\n");
		return -1;
	}

	for (i = 0; i < argc; i++) {
		if(strcmp("--help",argv[i]) == 0) {
			help();
			return 0;
		}
	}

	// Check card generation
	boot0 = (peek(0x0) & 0x1ff00000) >> 20;
	if (boot0 < 0xa3 || boot0 >= 0xc0){
		fprintf(stderr,"Card unsupported.\n");
		return -1;
	}

	// First checking xfer-ext base to see if the fuc code was uploaded
	tmp_reg = peek(0x10a110);
	tmp_reg = tmp_reg & 0xffffff00;
	if ((tmp_reg & 0xffffff00) != 0x001fff00) {
		fprintf(stderr,"Register 0x10a110 not in order\n");
		return -1;
	}

	// Is channel set up?
	tmp_reg = peek(0x10a47c);
	if ((tmp_reg  & 0xf0000000) != 0x70000000) {
		fprintf(stderr,"Channel not set up\n");
		return -1;
	}

	// Find the pagetable based on this info
	chan_ptr = ((tmp_reg & 0x0fffffff) << 12) & 0x000000ffffffffff; // XXX: Mask?
	ret = g80_pte_from_pde(chan_ptr, &pte, &hostmem);
	if (ret)
		return ret;

	if (hostmem)
		tmp_mask = 0x2000000;

	hi = (pte & 0xffffff0000ull) >> 16;
	lo = (pte & 0xffff);

	// Lets go and read this pt
	poke(0x1700, hi | tmp_mask);
	for (i = 0; i < 256; i++) {
		j = i * 8;
		tmp_reg = peek(0x700000 + lo + j);
		if ((tmp_reg & 0x000000ff) != 0x31) {
			i--;
			break;
		}
		ptable[i] = tmp_reg & 0xfffff000;
		if (ptable[i] == 0x00000000) {
			i--;
			break;
		}
	}
	ptable_size = i + 1;

	// And output the fuc codes
	for (i = 0; i < ptable_size; i++) {
		hi = (ptable[i] & 0xffff0000) >> 16;
		lo = ptable[i] & 0x0000ffff;
		poke(0x1700, hi | 0x2000000);

		// Read 4K from here and output this
		for (j = 0; j < 4096; j=j+4) {
			tmp_reg = peek(0x700000+lo+j);
			printf("%c%c%c%c",tmp_reg,tmp_reg>>8,tmp_reg>>16,tmp_reg>>24);
		}
	}

	return 0;
}
