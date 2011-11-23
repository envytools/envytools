/*
 * Copyright (C) 2011 Martin Peres <martin.peres@ensi-bourges.fr>
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
 * Copyright (C) 2011 Paweł Czaplejewicz <nodr.pcz@porcupinefactory.org>
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

/* Uploads vp2 vuc code to PBSP from file. File format: 4 bytes per word, 2 words per opcode. First word: low 32 bit, second word: high 8 bit, both system-endian.
Command:
nvavucstart [-u] [-c cnum] -[b|v] payload.bin
-u specifies upload only, don't start the program
*/

#include "nva.h"
#include <stdio.h>
#include <unistd.h>


#define XTENSA_HOSTMAPPED_BASE 0xcffc0000
#define HOST_XTENSA_BASE 0x103000
#define HOST_XTENSA_INDEX 0x103cfc
#define XTENSA_SECTION_SIZE_BYTES 0x40
#define XTENSA_WORD_SIZE_BYTES 4

#define XTENSA_VUC_CONTROL 0xcffca200
#define XTENSA_VUC_CODE_ACCESS 0xcffca300

#define VUC_CONTROL_HOLD (1<<16)


/* write and read for PBSP */
uint32_t pbsp_get_host_addr(uint32_t xt_addr) {
    uint32_t offset = xt_addr - XTENSA_HOSTMAPPED_BASE;
    return HOST_XTENSA_BASE + ((offset / XTENSA_SECTION_SIZE_BYTES) & (~(XTENSA_WORD_SIZE_BYTES - 1)));
}

uint32_t pbsp_get_host_index(uint32_t xt_addr) {
    uint32_t offset = xt_addr - XTENSA_HOSTMAPPED_BASE;
    return (offset & (XTENSA_SECTION_SIZE_BYTES - 1)) / XTENSA_WORD_SIZE_BYTES;
}

void host_xt_wr32(int cnum, uint32_t xt_addr, uint32_t val) {
    uint32_t host_addr = pbsp_get_host_addr(xt_addr);
    uint32_t index = pbsp_get_host_index(xt_addr);
    
//    printf("0x%x -> 0x%x i 0x%x\n", xt_addr, host_addr, index);
    
    nva_wr32(cnum, HOST_XTENSA_INDEX, index);
    nva_wr32(cnum, host_addr, val);
}

uint32_t host_xt_rd32(int cnum, uint32_t xt_addr) {
    uint32_t host_addr = pbsp_get_host_addr(xt_addr);
    uint32_t index = pbsp_get_host_index(xt_addr);
    
    nva_wr32(cnum, HOST_XTENSA_INDEX, index);
    return nva_rd32(cnum, host_addr);
}


void vuc_start(int cnum, uint32_t vuc_code_addr) {
    host_xt_wr32(cnum, XTENSA_VUC_CONTROL, vuc_code_addr);
}

void vuc_hold(int cnum, uint32_t vuc_code_addr) {
    host_xt_wr32(cnum, XTENSA_VUC_CONTROL, VUC_CONTROL_HOLD | vuc_code_addr);
}


int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
	int cnum =0;
	int base =0;
	int start = 1;
	while ((c = getopt (argc, argv, "c:bvu")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
			case 'b':
			    base = 0xabcd;
				break;
			case 'u':
			    start = 0;
		}

	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}

	if (!base) {
		fprintf (stderr, "No engine specified. Specify -b for PBSP, -v for PVP\n");
		return 1;
	}
	
	if (optind >= argc) {
	    fprintf(stderr, "At least specify payload file.\n");
	    return 1;
	}

	FILE *payload = fopen(argv[optind], "rb");
    
    if (payload == NULL) {
        fprintf(stderr, "Binary file %s could not be opened.\n", argv[optind]);
        return 1;
    } else {
        fprintf(stderr, "Uploading file %s.\n", argv[optind]);
    }
    
    int i;

    vuc_hold(cnum, 0);

	for (i = 0; ; i += 4) {
	    uint32_t word = 0;
	    int y;
	    int character = 0;
	    for (y = 0; y < 4; y++) {
    		character = fgetc(payload);
    		if (character == EOF) {
    		    break;
    		}
    		word |= ((unsigned char)character) << (y * 8);
    	}
    	if (y == 0) {
    	    break;
    	}
    	host_xt_wr32(cnum, XTENSA_VUC_CODE_ACCESS, word);
   		if (character == EOF) {
   		    break;
   		}
	}
	if (start) {
    	vuc_start(cnum, 0);
    }
	return 0;
}
