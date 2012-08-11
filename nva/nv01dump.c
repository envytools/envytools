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

#include "nva.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
	int cnum =0;
	while ((c = getopt (argc, argv, "c:")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
		}
	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}
	int i;
	uint32_t pmc_enable = nva_rd32(cnum, 0x200);

	printf("PMC:\n");
	printf("\tID: %08x\n", nva_rd32(cnum, 0));
	printf("\tINTR: %08x\n", nva_rd32(cnum, 0x100));
	printf("\tINTR_EN: %08x\n", nva_rd32(cnum, 0x140));
	printf("\tINTR_LN: %08x\n", nva_rd32(cnum, 0x160));
	printf("\tENABLE: %08x\n", nva_rd32(cnum, 0x200));

	printf("PBUS:\n");
	printf("\tUNK1200: %08x\n", nva_rd32(cnum, 0x1200));
	printf("\tUNK1400: %08x\n", nva_rd32(cnum, 0x1400));
	printf("\tPCI_0:\n");
	for (i = 0; i < 16; i++)
		printf("\t\t%02x: %08x\n", i * 4, nva_rd32(cnum, 0x1800 + i * 4));
	printf("\tPCI_1:\n");
	for (i = 0; i < 16; i++)
		printf("\t\t%02x: %08x\n", i * 4, nva_rd32(cnum, 0x1900 + i * 4));

	if (pmc_enable & 0x100) {
		printf("PFIFO:\n");
		printf("\tWAIT_RETRY: %08x\n", nva_rd32(cnum, 0x2040));
		printf("\tCACHE_ERROR: %08x\n", nva_rd32(cnum, 0x2080));
		printf("\tINTR: %08x/%08x\n", nva_rd32(cnum, 0x2100), nva_rd32(cnum, 0x2140));
		printf("\tCONFIG: %08x\n", nva_rd32(cnum, 0x2200));
		printf("\tRUNOUT_STATUS: %08x\n", nva_rd32(cnum, 0x2400));
		printf("\tRUNOUT_PUT: %08x\n", nva_rd32(cnum, 0x2410));
		printf("\tRUNOUT_GET: %08x\n", nva_rd32(cnum, 0x2420));
		printf("\tCACHES: %08x\n", nva_rd32(cnum, 0x2500));
		printf("\tDEVICE: %08x\n", nva_rd32(cnum, 0x2800));
		printf("\tCACHE0:\n");
		printf("\t\tPUSH_CTRL: %08x\n", nva_rd32(cnum, 0x3000));
		printf("\t\tCHID: %08x\n", nva_rd32(cnum, 0x3010));
		printf("\t\tSTATUS: %08x\n", nva_rd32(cnum, 0x3020));
		printf("\t\tPUT: %08x\n", nva_rd32(cnum, 0x3030));
		printf("\t\tPULL_CTRL: %08x\n", nva_rd32(cnum, 0x3040));
		printf("\t\tPULL_STATE: %08x\n", nva_rd32(cnum, 0x3050));
		printf("\t\tGET: %08x\n", nva_rd32(cnum, 0x3070));
		printf("\t\tCONTEXT: %08x\n", nva_rd32(cnum, 0x3080));
		printf("\t\tADDR: %08x\n", nva_rd32(cnum, 0x3100));
		printf("\t\tDATA: %08x\n", nva_rd32(cnum, 0x3104));
		printf("\tCACHE1:\n");
		printf("\t\tPUSH_CTRL: %08x\n", nva_rd32(cnum, 0x3200));
		printf("\t\tCHID: %08x\n", nva_rd32(cnum, 0x3210));
		printf("\t\tSTATUS: %08x\n", nva_rd32(cnum, 0x3220));
		printf("\t\tPUT: %08x\n", nva_rd32(cnum, 0x3230));
		printf("\t\tPULL_CTRL: %08x\n", nva_rd32(cnum, 0x3240));
		printf("\t\tPULL_STATE: %08x\n", nva_rd32(cnum, 0x3250));
		printf("\t\tGET: %08x\n", nva_rd32(cnum, 0x3270));
		for (i = 0; i < 8; i++)
			printf("\t\tCONTEXT[%d]: %08x\n", i, nva_rd32(cnum, 0x3280 + i * 0x10));
		for (i = 0; i < 32; i++) {
			nva_rd32(cnum, 0x3200);
			printf("\t\tADDR[%d]: %08x\n", i, nva_rd32(cnum, 0x3300 + i * 8));
			printf("\t\tDATA[%d]: %08x\n", i, nva_rd32(cnum, 0x3304 + i * 8));
		}
	}

	if (pmc_enable & 0x10) {
		printf("PDMA:\n");
		printf("\tUNK0:\n");
		printf("\t\tINTR: %08x/%08x\n", nva_rd32(cnum, 0x100100), nva_rd32(cnum, 0x100140));
		printf("\t\tSTATUS: %08x\n", nva_rd32(cnum, 0x100414));
		printf("\t\tPHYS_ADDR: %08x\n", nva_rd32(cnum, 0x100440));
		printf("\t\tUNK100: %08x\n", nva_rd32(cnum, 0x100500));
		for (i = 0; i < 8; i++)
			printf("\t\tREAD_BUF[%d]: %08x\n", i, nva_rd32(cnum, 0x100580 + i * 4));
		printf("\tAUDIO:\n");
		printf("\t\tINTR: %08x/%08x\n", nva_rd32(cnum, 0x100108), nva_rd32(cnum, 0x100148));
		printf("\t\tFLAGS: %08x\n", nva_rd32(cnum, 0x100600));
		printf("\t\tLIMIT: %08x\n", nva_rd32(cnum, 0x100604));
		printf("\t\tPTE: %08x\n", nva_rd32(cnum, 0x100608));
		printf("\t\tACCESS: %08x\n", nva_rd32(cnum, 0x100610));
		printf("\t\tSTATUS: %08x\n", nva_rd32(cnum, 0x100614));
		printf("\t\tVALID: %08x\n", nva_rd32(cnum, 0x100618));
		printf("\t\tPTE_TAG: %08x\n", nva_rd32(cnum, 0x100620));
		printf("\t\tADDR_VIRT_ADJ: %08x\n", nva_rd32(cnum, 0x100630));
		printf("\t\tADDR_PHYS: %08x\n", nva_rd32(cnum, 0x100640));
		printf("\t\tINST: %08x\n", nva_rd32(cnum, 0x100680));
		for (i = 0; i < 4; i++)
			printf("\t\tWRITE_BUF[%d]: %08x\n", i, nva_rd32(cnum, 0x100700 + i * 4));
		printf("\tGRAPH:\n");
		printf("\t\tINTR: %08x/%08x\n", nva_rd32(cnum, 0x100110), nva_rd32(cnum, 0x100150));
		printf("\t\tFLAGS: %08x\n", nva_rd32(cnum, 0x100800));
		printf("\t\tLIMIT: %08x\n", nva_rd32(cnum, 0x100804));
		printf("\t\tPTE: %08x\n", nva_rd32(cnum, 0x100808));
		printf("\t\tACCESS: %08x\n", nva_rd32(cnum, 0x100810));
		printf("\t\tSTATUS: %08x\n", nva_rd32(cnum, 0x100814));
		printf("\t\tVALID: %08x\n", nva_rd32(cnum, 0x100818));
		printf("\t\tPTE_TAG: %08x\n", nva_rd32(cnum, 0x100820));
		printf("\t\tADDR_VIRT_ADJ: %08x\n", nva_rd32(cnum, 0x100830));
		printf("\t\tADDR_PHYS: %08x\n", nva_rd32(cnum, 0x100840));
		printf("\t\tINST: %08x\n", nva_rd32(cnum, 0x100880));
		for (i = 0; i < 8; i++)
			printf("\t\tWRITE_BUF[%d]: %08x\n", i, nva_rd32(cnum, 0x100900 + i * 4));

		printf("PTIMER:\n");
		printf("\tINTR: %08x/%08x\n", nva_rd32(cnum, 0x101100), nva_rd32(cnum, 0x101140));
		printf("\tCLOCK_DIV: %08x\n", nva_rd32(cnum, 0x101200));
		printf("\tCLOCK_MUL: %08x\n", nva_rd32(cnum, 0x101210));
		printf("\tTIME_LOW: %08x\n", nva_rd32(cnum, 0x101400));
		printf("\tTIME_HIGH: %08x\n", nva_rd32(cnum, 0x101404));
		printf("\tALARM: %08x\n", nva_rd32(cnum, 0x101410));
	}

	if (pmc_enable & 0x1) {
		uint32_t audio_en = nva_rd32(cnum, 0x300080);
		printf("PAUDIO:\n");
		printf("\tENABLE: %08x\n", nva_rd32(cnum, 0x300080));
		printf("\tINTR_0: %08x/%08x\n", nva_rd32(cnum, 0x300100), nva_rd32(cnum, 0x300140));
		printf("\tINTR_1: %08x/%08x\n", nva_rd32(cnum, 0x300104), nva_rd32(cnum, 0x300144));
		for (i = 0; i < 3; i++)
			printf("\tUNK200[%d]: %08x\n", i, nva_rd32(cnum, 0x300200 + i * 4));
		printf("\tUNK400: %08x\n", nva_rd32(cnum, 0x300400));
		if (audio_en & 1) {
			printf("\tAD1848:\n");
			uint8_t ctrl = nva_rd32(cnum, 0x300500);
			printf("\t\tCTRL: %02x\n", ctrl);
			for (i = 0; i < 16; i++) {
				nva_wr32(cnum, 0x300500, (ctrl & 0xf0) | i);
				printf("\t\tDATA[%d]: %02x\n", i, nva_rd32(cnum, 0x300510));
			}
			nva_wr32(cnum, 0x300500, ctrl);
			printf("\t\tSTATUS: %02x\n", nva_rd32(cnum, 0x300520));
		}
		for (i = 0; i < 3; i++)
			printf("\tUNK800[%d]: %08x\n", i, nva_rd32(cnum, 0x300800 + i * 4));
		printf("\tUNK980: %08x\n", nva_rd32(cnum, 0x300980));
	}

	if (pmc_enable & 0x1000) {
		printf("PGRAPH:\n");
		printf("\tUNK080: %08x\n", nva_rd32(cnum, 0x400080));
		printf("\tUNK084: %08x\n", nva_rd32(cnum, 0x400084));
		printf("\tUNK088: %08x\n", nva_rd32(cnum, 0x400088));
		printf("\tUNK08C: %08x\n", nva_rd32(cnum, 0x40008c));
		printf("\tINTR_0: %08x/%08x\n", nva_rd32(cnum, 0x400100), nva_rd32(cnum, 0x400140));
		printf("\tINTR_1: %08x/%08x\n", nva_rd32(cnum, 0x400104), nva_rd32(cnum, 0x400144));
		printf("\tUNK180: %08x\n", nva_rd32(cnum, 0x400180));
		printf("\tUNK190: %08x\n", nva_rd32(cnum, 0x400190));
		for (i = 0; i < 18; i++)
			printf("\tUNK400[%d]: %08x\n", i, nva_rd32(cnum, 0x400400 + i * 4));
		printf("\tUNK450: %08x\n", nva_rd32(cnum, 0x400450));
		printf("\tUNK454: %08x\n", nva_rd32(cnum, 0x400454));
		printf("\tUNK460: %08x\n", nva_rd32(cnum, 0x400460));
		printf("\tUNK464: %08x\n", nva_rd32(cnum, 0x400464));
		printf("\tUNK468: %08x\n", nva_rd32(cnum, 0x400468));
		printf("\tUNK46C: %08x\n", nva_rd32(cnum, 0x40046c));
		for (i = 0; i < 18; i++)
			printf("\tUNK480[%d]: %08x\n", i, nva_rd32(cnum, 0x400480 + i * 4));
		printf("\tUNK600: %08x\n", nva_rd32(cnum, 0x400600));
		printf("\tUNK604: %08x\n", nva_rd32(cnum, 0x400604));
		printf("\tUNK608: %08x\n", nva_rd32(cnum, 0x400608));
		printf("\tUNK60C: %08x\n", nva_rd32(cnum, 0x40060c));
		printf("\tUNK610: %08x\n", nva_rd32(cnum, 0x400610));
		printf("\tUNK614: %08x\n", nva_rd32(cnum, 0x400614));
		printf("\tUNK618: %08x\n", nva_rd32(cnum, 0x400618));
		printf("\tUNK61C: %08x\n", nva_rd32(cnum, 0x40061c));
		printf("\tUNK620: %08x\n", nva_rd32(cnum, 0x400620));
		printf("\tUNK624: %08x\n", nva_rd32(cnum, 0x400624));
		printf("\tUNK628: %08x\n", nva_rd32(cnum, 0x400628));
		printf("\tUNK62C: %08x\n", nva_rd32(cnum, 0x40062c));
		printf("\tUNK630: %08x\n", nva_rd32(cnum, 0x400630));
		printf("\tUNK634: %08x\n", nva_rd32(cnum, 0x400634));
		printf("\tUNK640: %08x\n", nva_rd32(cnum, 0x400640));
		printf("\tUNK644: %08x\n", nva_rd32(cnum, 0x400644));
		printf("\tUNK648: %08x\n", nva_rd32(cnum, 0x400648));
		printf("\tUNK64C: %08x\n", nva_rd32(cnum, 0x40064c));
		printf("\tUNK650: %08x\n", nva_rd32(cnum, 0x400650));
		printf("\tUNK654: %08x\n", nva_rd32(cnum, 0x400654));
		printf("\tUNK658: %08x\n", nva_rd32(cnum, 0x400658));
		printf("\tUNK65C: %08x\n", nva_rd32(cnum, 0x40065c));
		printf("\tUNK660: %08x\n", nva_rd32(cnum, 0x400660));
		printf("\tUNK680: %08x\n", nva_rd32(cnum, 0x400680));
		printf("\tUNK684: %08x\n", nva_rd32(cnum, 0x400684));
		printf("\tUNK688: %08x\n", nva_rd32(cnum, 0x400688));
		printf("\tUNK68C: %08x\n", nva_rd32(cnum, 0x40068c));
		printf("\tUNK690: %08x\n", nva_rd32(cnum, 0x400690));
		printf("\tUNK694: %08x\n", nva_rd32(cnum, 0x400694));
		printf("\tUNK698: %08x\n", nva_rd32(cnum, 0x400698));
		printf("\tUNK69C: %08x\n", nva_rd32(cnum, 0x40069c));
		printf("\tUNK6A0: %08x\n", nva_rd32(cnum, 0x4006a0));
		printf("\tUNK6A4: %08x\n", nva_rd32(cnum, 0x4006a4));
		printf("\tUNK6B0: %08x\n", nva_rd32(cnum, 0x4006b0));
		for (i = 0; i < 14; i++)
			printf("\tUNK700[%d]: %08x\n", i, nva_rd32(cnum, 0x400700 + i * 4));
	}

	if (pmc_enable & 0x1000000) {
		printf("PFB:\n");
		printf("\tBOOT_0: %08x\n", nva_rd32(cnum, 0x600000));
		printf("\tUNK040: %08x\n", nva_rd32(cnum, 0x600040));
		printf("\tUNK044: %08x\n", nva_rd32(cnum, 0x600044));
		printf("\tUNK080: %08x\n", nva_rd32(cnum, 0x600080));
		printf("\tGREEN: %08x\n", nva_rd32(cnum, 0x6000c0));
		printf("\tCONFIG: %08x\n", nva_rd32(cnum, 0x600200));
		printf("\tSTART: %08x\n", nva_rd32(cnum, 0x600400));
		printf("\tHOR_FRONT_PORCH: %08x\n", nva_rd32(cnum, 0x600500));
		printf("\tHOR_SYNC_WIDTH: %08x\n", nva_rd32(cnum, 0x600510));
		printf("\tHOR_BACK_PORCH: %08x\n", nva_rd32(cnum, 0x600520));
		printf("\tHOR_DISP_WIDTH: %08x\n", nva_rd32(cnum, 0x600530));
		printf("\tVER_FRONT_PORCH: %08x\n", nva_rd32(cnum, 0x600540));
		printf("\tVER_SYNC_WIDTH: %08x\n", nva_rd32(cnum, 0x600550));
		printf("\tVER_BACK_PORCH: %08x\n", nva_rd32(cnum, 0x600560));
		printf("\tVER_DISP_WIDTH: %08x\n", nva_rd32(cnum, 0x600570));
	}

	printf("PRAM:\n");
	printf("\tCONFIG: %08x\n", nva_rd32(cnum, 0x602200));

	printf("PCHIPID:\n");
	printf("\tCHIPID_0: %08x\n", nva_rd32(cnum, 0x605400));
	printf("\tCHIPID_1: %08x\n", nva_rd32(cnum, 0x605404));

	printf("PSTRAPS:\n");
	printf("\tSTRAPS: %08x\n", nva_rd32(cnum, 0x608000));

	printf("PDAC:\n");
	printf("\tPIXEL_MASK: %02x\n", nva_rd32(cnum, 0x609008));
	for (i = 0; i < 256; i++) {
		nva_wr32(cnum, 0x60900c, i);
		uint8_t r = nva_rd32(cnum, 0x609004);
		uint8_t g = nva_rd32(cnum, 0x609004);
		uint8_t b = nva_rd32(cnum, 0x609004);
		printf("\t\tPALETTE[%d]: %02x/%02x/%02x\n", i, r, g, b);
	}
	for (i = 0; i < 0x100; i++) {
		nva_wr32(cnum, 0x609010, i);
		nva_wr32(cnum, 0x609014, 0);
		printf("\t\tDAC[0x%02x]: %02x\n", i, nva_rd32(cnum, 0x609018));
	}
	for (i = 0; i < 0x100; i++) {
		nva_wr32(cnum, 0x609010, i);
		nva_wr32(cnum, 0x609014, 5);
		printf("\t\tCURSOR[0x%02x]: %02x\n", i, nva_rd32(cnum, 0x609018));
	}
	printf("\tGAME_PORT: %02x\n", nva_rd32(cnum, 0x60901c));

	if (pmc_enable & 0x10000) {
		printf("PRM:\n");
		printf("\tUNK080: %08x\n", nva_rd32(cnum, 0x6c0080));
		printf("\tINTR: %08x/%08x\n", nva_rd32(cnum, 0x6c0100), nva_rd32(cnum, 0x6c0140));
		printf("\tUNK200: %08x\n", nva_rd32(cnum, 0x6c0200));
		printf("\tUNK400: %08x\n", nva_rd32(cnum, 0x6c0400));
		printf("\tUNK1F00: %08x\n", nva_rd32(cnum, 0x6c1f00));
		printf("\tUNK1F10: %08x\n", nva_rd32(cnum, 0x6c1f10));
		printf("\tUNK1F20: %08x\n", nva_rd32(cnum, 0x6c1f20));
		printf("\tUNK1F24: %08x\n", nva_rd32(cnum, 0x6c1f24));

		printf("PRMIO:\n");
		/* XXX: audio crap */
		printf("\tVSE2: %02x\n", nva_rd8(cnum, 0x6d03c3));
		printf("\tMISC: %02x\n", nva_rd8(cnum, 0x6d03cc));
		printf("\tFC: %02x\n", nva_rd8(cnum, 0x6d03ca));
		printf("\tINP0: %02x\n", nva_rd8(cnum, 0x6d03c2));
		printf("\tINP1: %02x\n", nva_rd8(cnum, 0x6d03da));
		printf("\tCRX: %02x\n", nva_rd8(cnum, 0x6d03d4));
		uint8_t idx = nva_rd8(cnum, 0x6d03d4);
		for (i = 0; i < 32; i++) {
			nva_wr8(cnum, 0x6d03d4, i);
			printf("\tCR[0x%02x]: %02x\n", i, nva_rd8(cnum, 0x6d03d5));
		}
		nva_wr8(cnum, 0x6d03d4, idx);
		idx = nva_rd8(cnum, 0x6d03c4);
		for (i = 0; i < 8; i++) {
			nva_wr8(cnum, 0x6d03c4, i);
			printf("\tSR[0x%02x]: %02x\n", i, nva_rd8(cnum, 0x6d03c5));
		}
		nva_wr8(cnum, 0x6d03c4, idx);
		idx = nva_rd8(cnum, 0x6d03ce);
		for (i = 0; i < 16; i++) {
			nva_wr8(cnum, 0x6d03ce, i);
			printf("\tGR[0x%02x]: %02x\n", i, nva_rd8(cnum, 0x6d03cf));
		}
		nva_wr8(cnum, 0x6d03ce, idx);
		idx = nva_rd8(cnum, 0x6d03c0);
		for (i = 0; i < 32; i++) {
			nva_rd8(cnum, 0x6d03da);
			nva_wr8(cnum, 0x6d03c0, i);
			printf("\tAR[0x%02x]: %02x\n", i, nva_rd8(cnum, 0x6d03c1));
		}
		nva_rd8(cnum, 0x6d03da);
		nva_wr8(cnum, 0x6d03c0, idx);
	}
	return 0;
}
