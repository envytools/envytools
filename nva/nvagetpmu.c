/*
 * Copyright 2011 Roy Spliet <rspliet@eclipso.eu>
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

void help() {
	printf("Retreives PMU from the GPU.\nJust run, and pipe output to file.\n");
}

int32_t peek(int32_t reg) {
	if((reg < 0x400300 || reg >= 0x400400) || (nva_rd32(0,0)&0x0e000000) != 0x02000000) {
		return nva_rd32(0,reg);
	} else {
		return 0;
	}
}

void poke(uint32_t reg, uint32_t value) {
	nva_wr32(0,reg,value);
}

/*
 * XXX: Take card number as parameter
 * XXX: Does this generalise to other falcon engines too?
 */

int main(int argc, char **argv){
	int i = 0, j = 0;
	uint32_t tmp_reg, tmp_mask;
	int64_t tmp_memaddr = 0;

	uint32_t hi = 0,lo = 0;
	int32_t ptable[256];
	int ptable_size = 0;

	if(nva_init()) {
		printf("Init failed\n");
		return -1;
	}

	for(i = 0; i < argc; i++) {
		if(strcmp("--help",argv[i]) == 0) {
			help();
			return 0;
		}
	}

	// First checking xfer-ext base to see if the fuc code was uploaded
	tmp_reg = peek(0x10a110);
	tmp_reg = tmp_reg & 0xffffff00;
	if((tmp_reg & 0xffffff00) != 0x001fff00) {
		fprintf(stderr,"Register 0x10a110 not in order\n");
		return -1;
	}
	// Is channel set up?
	tmp_reg = peek(0x10a47c);
	if((tmp_reg  & 0xf0000000) != 0x70000000) {
		fprintf(stderr,"Channel not set up\n");
		return -1;
	}

	// Find the pagetable based on this info
	tmp_memaddr = ((tmp_reg & 0x0fffffff) << 12) & 0x000000ffffffffff; //  probably not right
	hi = (tmp_memaddr >> 16) & 0xffffff;
	lo = (tmp_memaddr & 0x0000ffff);

	poke(0x1700, hi | 0x2000000);
	tmp_reg = peek(0x700000+lo+0x208);
	if ((tmp_reg & 0xff3) != 0x63) {
		fprintf(stderr,"Page table entry invalid\n");
		return -1;
	}
	// Mask off lower 12 bits
	tmp_reg = tmp_reg & 0xfffffff000;
	hi = (tmp_reg & 0xffff0000) >> 16;
	lo = tmp_reg & 0x0000ffff;

	if ((tmp_reg & 0xc) == 0xc) {
		tmp_mask = 0x2000000;
	} else if ((tmp_reg & 0xc) == 0x0) {
		tmp_mask = 0;
	} else {
		fprintf(stderr,"Page table entry points to unsupported memory\n");
		return -1;
	}

	// Lets go and read this pt
	poke(0x1700, hi | tmp_mask);
	for(i = 0; i < 256; i++) {
		j = i * 8;
		tmp_reg = peek(0x700000 + lo + j);
		if((tmp_reg & 0x000000ff) != 0x31) {
			i--;
			break;
		}
		ptable[i] = tmp_reg & 0xfffff000;
		if(ptable[i] == 0x00000000) {
			i--;
			break;
		}
	}
	ptable_size = i + 1;

	// And output the fuc codes
	for(i = 0; i < ptable_size; i++) {
		hi = (ptable[i] & 0xffff0000) >> 16;
		lo = ptable[i] & 0x0000ffff;
		poke(0x1700, hi | 0x2000000);

		// Read 4K from here and output this
		for(j = 0; j < 4096; j=j+4) {
			tmp_reg = peek(0x700000+lo+j);
			printf("%c%c%c%c",tmp_reg,tmp_reg>>8,tmp_reg>>16,tmp_reg>>24);
		}
	}

	return 0;
}
