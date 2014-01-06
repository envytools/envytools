/*
 * Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
 * Copyright (C) 2010-2011 Martin Peres <martin.peres@ensi-bourges.fr>
 * Copyright (C) 2010-2011 Emil Velikov <emil.l.velikov@gmail.com>
 * Copyright (C) 2011 Roy Spliet <r.spliet@student.tudelft.nl>
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

#include "util.h"
#include "var.h"
#include "dis.h"
#include "bios.h"
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

struct envy_bios *bios;
unsigned printmask = 0;
uint8_t tCWL = 0;
uint32_t strap = 0;
uint16_t script_print = 0;

uint16_t bmpver = 0;
uint8_t bmpver_min;
uint8_t bmpver_maj;
int maxcond = -1;
int maxiocond = -1;
int maxiofcond = -1;
int maxmi = -1;
int maxmac = -1;

uint16_t init_script_tbl_ptr;
uint16_t macro_index_tbl_ptr;
uint16_t macro_tbl_ptr;
uint16_t condition_tbl_ptr;
uint16_t io_condition_tbl_ptr;
uint16_t io_flag_condition_tbl_ptr;
uint16_t init_function_tbl_ptr;
uint16_t some_script_ptr;
uint16_t init96_tbl_ptr;

uint16_t disp_script_tbl_ptr;

uint8_t ram_restrict_group_count;
uint16_t ram_restrict_tbl_ptr;
uint16_t ram_type_tbl_ptr;

uint16_t pll_limit_tbl_ptr;

uint16_t *subs = 0;
int subsnum = 0, subsmax = 0;

uint16_t *calls = 0;
int callsnum = 0, callsmax = 0;

struct {
	const char *name;
	unsigned mask;
} printmasks[] = {
	{ "pcir",	ENVY_BIOS_PRINT_PCIR },
	{ "version",	ENVY_BIOS_PRINT_VERSION },
	{ "hwinfo",	ENVY_BIOS_PRINT_HWINFO },
	{ "bit",	ENVY_BIOS_PRINT_BMP_BIT },
	{ "bmp",	ENVY_BIOS_PRINT_BMP_BIT },
	{ "info",	ENVY_BIOS_PRINT_INFO },
	{ "dacload",	ENVY_BIOS_PRINT_DACLOAD },
	{ "iunk",	ENVY_BIOS_PRINT_IUNK },
	{ "scripts",	ENVY_BIOS_PRINT_SCRIPTS },
	{ "hwsq",	ENVY_BIOS_PRINT_HWSQ },
	{ "pll",	ENVY_BIOS_PRINT_PLL },
	{ "ram",	ENVY_BIOS_PRINT_RAM },
	{ "perf",	ENVY_BIOS_PRINT_PERF },
	{ "i2cscript",	ENVY_BIOS_PRINT_I2CSCRIPT },
	{ "dcball",	ENVY_BIOS_PRINT_DCB_ALL },
	{ "dcb",	ENVY_BIOS_PRINT_DCB },
	{ "gpio",	ENVY_BIOS_PRINT_GPIO },
	{ "i2c",	ENVY_BIOS_PRINT_I2C },
	{ "extdev",	ENVY_BIOS_PRINT_EXTDEV },
	{ "conn",	ENVY_BIOS_PRINT_CONN },
	{ "mux",	ENVY_BIOS_PRINT_MUX },
	{ "dunk",	ENVY_BIOS_PRINT_DUNK },
};

int usage(char* name) {
	int i;

	printf("\nUsage:\n %s [options] mybios.rom\n\n", name);
	printf("Options:\n");
	printf(" -h        print usage and exit\n");
	printf(" -m XX     set tCWL (for timing entry size < 20)\n");
	printf(" -s XX     set the trap register\n");
	printf(" -i XXXX   print init script at XXXX and exit\n");
	printf(" -v        be verbose\n");
	printf(" -u        print unused\n");
	printf(" -b        print blocks\n");
	printf(" -p XXXX   set print mask, XXXX can be: ");
	for (i = 0; i < sizeof(printmasks) / sizeof(*printmasks) - 1; i++)
		printf("%s, ", printmasks[i].name);
	printf("%s\n", printmasks[i].name);

	printf("\nFor more details see code\n");
	return 1;
}

void printhex (uint32_t start, uint32_t length) {
	envy_bios_dump_hex(bios, stdout, start, length, printmask);
}

void printcmd (uint16_t off, uint16_t length) {
	printf ("0x%04x:", off);
	int i;
	for (i = 0; i < length; i++)
		printf(" %02x", bios->data[off+i]);
	for (; i < 16; i++)
		printf ("   ");
}

uint32_t le32 (uint16_t off) {
	return bios->data[off] | bios->data[off+1] << 8 | bios->data[off+2] << 16 | bios->data[off+3] << 24;
}

uint16_t le16 (uint16_t off) {
	return bios->data[off] | bios->data[off+1] << 8;
}

uint8_t parse_memtm_mapping_entry_10(uint16_t pos, uint16_t len, uint16_t hdr_pos)
{
	if(len < 4) {
		printf("Unknown timing mapping entry length: %u\n", len);
		return 0xff;
	}
	if(bios->data[pos+1] != 0xff) {
		printf(" Timing: %u\n", bios->data[pos+1]);
	} else {
		printf(" Timing: none/boot\n");
	}

	if(bios->power.bit->version == 1) {
		uint8_t c = bios->data[pos+3];
		printf(" 100710: (r & 0x101) | 0x%x | 0x%x\n",
		       (c & 0x2) ? 0x100 : 0, (c & 0x8) ? 0 : 1);
		printf(" 100714: (r & 0x20) | 0x%x\n", (c & 0x8) ? 0 : 0x20);
		printf(" 10071c: (r & 0x100) | 0x%x", (c & 0x1) ? 0x100 : 0);
	}

	if(bios->power.bit->version == 2) {
		printf(" DLL: ");
		if(bios->data[pos+2] & 0x40) {
			printf("disabled\n");
		} else {
			printf("enabled\n");
		}

		if(bios->data[hdr_pos+4] & 0x08 && len >= 10) {
			printf(" Feature unk0 enabled\n");
			if(bios->chipset < 0xC0) {
				printf("   10053c: 0x00000000\n");
				printf("   1005a0: 0x00%02hhx%02hhx%02hhx\n",
						bios->data[pos+6], bios->data[pos+5],
						bios->data[pos+5]);
				printf("   1005a4: 0x0000%02hhx%02hhx\n",
						bios->data[pos+8],
						bios->data[pos+7]);
				printf("   10f804: 0x80%02x00%02x\n",
						bios->data[pos+9] & 0xf0,
						bios->data[pos+9] & 0x0f);
			} else {
				printf("   10f658: 0x00%02hhx%02hhx%02hhx\n",
						bios->data[pos+6], bios->data[pos+5],
						bios->data[pos+5]);
				printf("   10f660: 0x0000%02hhx%02hhx\n",
						bios->data[pos+8],
						bios->data[pos+7]);
				printf("   10f668: 0x80%02x00%02x\n",
						bios->data[pos+9] & 0xf0,
						bios->data[pos+9] & 0x0f);
			}
		} else {
			printf(" Feature unk0 disabled\n");
			if(bios->chipset < 0xC0) {
				printf("   10053c: 0x00001000\n");
				printf("   10f804: 0x00?0000?\n");
			} else {
				printf("   10f668: 0x00?0000?\n");
			}
		}
	}

	printf("\n");

	return bios->data[pos+1];
}

uint8_t parse_memtm_mapping_entry_11(uint16_t pos, uint16_t len, uint16_t hdr_pos)
{
	if(len < 4) {
		printf("Unknown timing mapping entry length: %u\n", len);
		return 0xff;
	}
	if(bios->data[pos+0] != 0xff) {
		printf(" Timing: %u\n", bios->data[pos]);
	} else {
		printf(" Timing: none/boot\n");
	}

	return bios->data[pos];
}

void printscript (uint16_t soff) {
	while (1) {
		uint8_t op = bios->data[soff];
		uint32_t dst;
		uint8_t incr;
		uint8_t cnt;
		int i;
		uint32_t x;
		switch (op) {
			case 0x32:
				printcmd (soff, 11);
				printf ("IO_RESTRICT_PROG\tR[0x%06x] = (0x%04x[0x%02x] & 0x%02x) >> %d) [{\n", le32(soff+7), le16(soff+1), bios->data[soff+3], bios->data[soff+4], bios->data[soff+5]);
				cnt = bios->data[soff+6];
				soff += 11;
				while (cnt--) {
					printcmd (soff, 4);
					printf ("\t0x%08x\n", le32(soff));
					soff += 4;
				}
				printcmd (soff, 0);
				printf ("}]\n");
				break;
			case 0x33:
				printcmd (soff, 2);
				printf ("REPEAT\t0x%02x\n", bios->data[soff+1]);
				soff += 2;
				break;
			case 0x34:
				printcmd (soff, 12);
				printf ("IO_RESTRICT_PLL\tR[0x%06x] =PLL= (0x%04x[0x%02x] & 0x%02x) >> %d) IOFCOND 0x%02x [{\n", le32(soff+8), le16(soff+1), bios->data[soff+3], bios->data[soff+4], bios->data[soff+5], bios->data[soff+6]);
				if (bios->data[soff+6] > maxiofcond && bios->data[soff+6] != 0xff)
					maxiofcond = bios->data[soff+6];
				cnt = bios->data[soff+7];
				soff += 12;
				while (cnt--) {
					printcmd (soff, 2);
					printf ("\t%dkHz\n", le16(soff) * 10);
					soff += 2;
				}
				printcmd (soff, 0);
				printf ("}]\n");
				break;
			case 0x36:
				printcmd (soff, 1);
				printf ("END_REPEAT\n");
				soff++;
				break;
			case 0x37:
				printcmd (soff, 11);
				printf ("COPY\t0x%04x[0x%02x] & ~0x%02x |= (R[0x%06x] %s 0x%02x) & 0x%08x\n",
						le16(soff+7), bios->data[soff+9], bios->data[soff+10], le32(soff+1), bios->data[soff+5]&0x80?"<<":">>",
						bios->data[soff+5]&0x80?0x100-bios->data[soff+5]:bios->data[soff+5], bios->data[soff+6]);
				soff += 11;
				break;
			case 0x38:
				printcmd (soff, 1);
				printf ("NOT\n");
				soff++;
				break;
			case 0x39:
				printcmd (soff, 2);
				printf ("IO_FLAG_CONDITION\t0x%02x\n", bios->data[soff+1]);
				if (bios->data[soff+1] > maxiofcond)
					maxiofcond = bios->data[soff+1];
				soff += 2;
				break;
			case 0x3a:
				printcmd (soff, 3);
				printf ("DP_CONDITION\t0x%02x 0x%02x\n", bios->data[soff+1], bios->data[soff+2]);
				soff += 3;
				break;
			case 0x3b:
				printcmd (soff, 2);
				printf ("IO_MASK_OR\t0x03d4[0x%02x] &= ~(1 << OR)\n", bios->data[soff+1]);
				soff += 2;
				break;
			case 0x3c:
				printcmd (soff, 2);
				printf ("IO_OR\t0x03d4[0x%02x] |= 1 << OR\n", bios->data[soff+1]);
				soff += 2;
				break;
			case 0x49:
				printcmd (soff, 9);
				printf ("INDEX_ADDRESS_LATCHED\tR[0x%06x] : R[0x%06x]\n", le32(soff+1), le32(soff+5));
				soff += 9;
				printcmd (soff, 9);
				printf ("\tCTRL &= 0x%08x |= 0x%08x\n", le32(soff), le32(soff+4));
				cnt = bios->data[soff+8];
				soff += 9;
				while (cnt--) {
					printcmd (soff, 2);
					printf("\t[0x%02x] = 0x%02x\n", bios->data[soff], bios->data[soff+1]);
					soff += 2;
				}
				break;
			case 0x4a:
				printcmd (soff, 11);
				printf ("IO_RESTRICT_PLL2\tR[0x%06x] =PLL= (0x%04x[0x%02x] & 0x%02x) >> %d) [{\n", le32(soff+7), le16(soff+1), bios->data[soff+3], bios->data[soff+4], bios->data[soff+5]);
				cnt = bios->data[soff+6];
				soff += 11;
				while (cnt--) {
					printcmd (soff, 4);
					printf ("\t%dkHz\n", le32(soff));
					soff += 4;
				}
				printcmd (soff, 0);
				printf ("}]\n");
				break;
			case 0x4b:
				printcmd (soff, 9);
				printf ("PLL2\tR[0x%06x] =PLL= %dkHz\n", le32(soff+1), le32(soff+5));
				soff += 9;
				break;
			case 0x4c:
				printcmd (soff, 4);
				printf ("I2C_BYTE\tI2C[0x%02x][0x%02x]\n", bios->data[soff+1], bios->data[soff+2]);
				cnt = bios->data[soff+3];
				soff += 4;
				while (cnt--) {
					printcmd (soff, 3);
					printf ("\t[0x%02x] &= 0x%02x |= 0x%02x\n", bios->data[soff], bios->data[soff+1], bios->data[soff+2]);
					soff += 3;
				}
				break;
			case 0x4d:
				printcmd (soff, 4);
				printf ("ZM_I2C_BYTE\tI2C[0x%02x][0x%02x]\n", bios->data[soff+1], bios->data[soff+2]);
				cnt = bios->data[soff+3];
				soff += 4;
				while (cnt--) {
					printcmd (soff, 2);
					printf ("\t[0x%02x] = 0x%02x\n", bios->data[soff], bios->data[soff+1]);
					soff += 2;
				}
				break;
			case 0x4e:
				printcmd (soff, 4);
				printf ("ZM_I2C\tI2C[0x%02x][0x%02x]\n", bios->data[soff+1], bios->data[soff+2]);
				cnt = bios->data[soff+3];
				soff += 4;
				while (cnt--) {
					printcmd (soff, 1);
					printf ("\t\t0x%02x\n", bios->data[soff]);
					soff++;
				}
				break;
			case 0x4f:
				printcmd (soff, 5);
				printf ("TMDS\tT[0x%02x][0x%02x] &= 0x%02x |= 0x%02x\n", bios->data[soff+1], bios->data[soff+2], bios->data[soff+3], bios->data[soff+4]);
				soff += 5;
				break;
			case 0x50:
				printcmd (soff, 3);
				printf ("TMDS_ZM_GROUP\tT[0x%02x]\n", bios->data[soff+1]);
				cnt = bios->data[soff+2];
				soff += 3;
				while (cnt--) {
					printcmd (soff, 2);
					printf ("\t[0x%02x] = 0x%02x\n", bios->data[soff], bios->data[soff+1]);
					soff += 2;
				}
				break;
			case 0x51:
				printcmd (soff, 5);
				cnt = bios->data[soff+4];
				dst = bios->data[soff+3];
				printf ("CR_INDEX_ADDR C[0x%02x] C[0x%02x] 0x%02x 0x%02x\n", bios->data[soff+1], bios->data[soff+2], bios->data[soff+3], bios->data[soff+4]);
				soff += 5;
				while (cnt--) {
					printcmd(soff, 1);
					printf ("\t\t[0x%02x] = 0x%02x\n", dst, bios->data[soff]);
					soff++;
					dst++;
				}
				break;
			case 0x52:
				printcmd (soff, 4);
				printf ("CR\t\tC[0x%02x] &= 0x%02x |= 0x%02x\n", bios->data[soff+1], bios->data[soff+2], bios->data[soff+3]);
				soff += 4;
				break;
			case 0x53:
				printcmd (soff, 3);
				printf ("ZM_CR\tC[0x%02x] = 0x%02x\n", bios->data[soff+1], bios->data[soff+2]);
				soff += 3;
				break;
			case 0x54:
				printcmd (soff, 2);
				printf ("ZM_CR_GROUP\n");
				cnt = bios->data[soff+1];
				soff += 2;
				while (cnt--) {
					printcmd(soff, 2);
					printf ("\t\tC[0x%02x] = 0x%02x\n", bios->data[soff], bios->data[soff+1]);
					soff += 2;
				}
				break;
			case 0x56:
				printcmd (soff, 3);
				printf ("CONDITION_TIME\t0x%02x 0x%02x\n", bios->data[soff+1], bios->data[soff+2]);
				if (bios->data[soff+1] > maxcond)
					maxcond = bios->data[soff+1];
				soff += 3;
				break;
			case 0x57:
				printcmd (soff, 3);
				printf ("LTIME\t0x%04x\n", le16(soff+1));
				soff += 3;
				break;
			case 0x58:
				printcmd (soff, 6);
				dst = le32(soff+1);
				cnt = bios->data[soff+5];
				soff += 6;
				printf ("ZM_REG_SEQUENCE\t0x%02x\n", cnt);
				while (cnt--) {
					printcmd (soff, 4);
					printf ("\t\tR[0x%06x] = 0x%08x\n", dst, le32(soff));
					dst += 4;
					soff += 4;
				}
				break;
			case 0x5b:
				printcmd (soff, 3);
				x = le16(soff+1);
				printf ("CALL\t0x%04x\n", le16(soff+1));
				soff += 3;
				for (i = 0; i < callsnum; i++)
					if (calls[i] == x)
						break;
				if (i == callsnum)
					ADDARRAY(calls, x);
				break;
			case 0x5c:
				printcmd (soff, 3);
				printf ("JUMP\t0x%04x\n", le16(soff+1));
				soff += 3;
				break;
			case 0x5e:
				printcmd (soff, 6);
				printf ("I2C_IF\tI2C[0x%02x][0x%02x][0x%02x] & 0x%02x == 0x%02x\n", bios->data[soff+1], bios->data[soff+2], bios->data[soff+3], bios->data[soff+4], bios->data[soff+5]);
				soff += 6;
				break;
			case 0x5f:
				printcmd (soff, 16);
				printf ("\n");
				printcmd (soff+16, 6);
				printf ("COPY_NV_REG\tR[0x%06x] & ~0x%08x = (R[0x%06x] %s 0x%02x) & 0x%08x ^ 0x%08x\n",
						le32(soff+14), le32(soff+18), le32(soff+1), bios->data[soff+5]&0x80?"<<":">>",
						bios->data[soff+5]&0x80?0x100-bios->data[soff+5]:bios->data[soff+5], le32(soff+6), le32(soff+10));
				soff += 22;
				break;
			case 0x62:
				printcmd (soff, 5);
				printf ("ZM_INDEX_IO\tI[0x%04x][0x%02x] = 0x%02x\n", le16(soff+1), bios->data[soff+3], bios->data[soff+4]);
				soff += 5;
				break;
			case 0x63:
				printcmd (soff, 1);
				printf ("COMPUTE_MEM\n");
				soff++;
				break;
			case 0x65:
				printcmd (soff, 13);
				printf ("RESET\tR[0x%06x] = 0x%08x, 0x%08x\n", le32(soff+1), le32(soff+5), le32(soff+9));
				soff += 13;
				break;
			case 0x66:
				printcmd (soff, 1);
				printf ("CONFIGURE_MEM\n");
				soff += 1;
				break;
			case 0x67:
				printcmd (soff, 1);
				printf ("CONFIGURE_CLOCK\n");
				soff += 1;
				break;
			case 0x68:
				printcmd (soff, 1);
				printf ("CONFIGURE_PREINIT\n");
				soff += 1;
				break;
			case 0x69:
				printcmd (soff, 5);
				printf ("IO\t\tI[0x%04x] &= 0x%02x |= 0x%02x\n", le16(soff+1), bios->data[soff+3], bios->data[soff+4]);
				soff += 5;
				break;
			case 0x6b:
				printcmd (soff, 2);
				printf ("SUB\t0x%02x\n", bios->data[soff+1]);
				x = bios->data[soff+1];
				soff += 2;
				for (i = 0; i < subsnum; i++)
					if (subs[i] == x)
						break;
				if (i == subsnum)
					ADDARRAY(subs, x);
				break;
			case 0x6d:
				printcmd (soff, 3);
				printf ("RAM_CONDITION\t"
					"(R[0x100000] & 0x%02x) == 0x%02x\n",
					bios->data[soff+1], bios->data[soff+2]);
				soff += 3;
				break;
			case 0x6e:
				printcmd (soff, 13);
				printf ("NV_REG\tR[0x%06x] &= 0x%08x |= 0x%08x\n", le32(soff+1), le32(soff+5), le32(soff+9));
				soff += 13;
				break;
			case 0x6f:
				printcmd (soff, 2);
				printf ("MACRO\t0x%02x\n", bios->data[soff+1]);
				if (bios->data[soff+1] > maxmi)
					maxmi = bios->data[soff+1];
				soff += 2;
				break;
			case 0x71:
				printcmd (soff, 1);
				printf ("DONE\n");
				return;
			case 0x72:
				printcmd (soff, 1);
				printf ("RESUME\n");
				soff++;
				break;
			case 0x74:
				printcmd (soff, 3);
				printf ("TIME\t0x%04x\n", le16(soff+1));
				soff += 3;
				break;
			case 0x75:
				printcmd (soff, 2);
				printf ("CONDITION\t0x%02x\n", bios->data[soff+1]);
				if (bios->data[soff+1] > maxcond)
					maxcond = bios->data[soff+1];
				soff += 2;
				break;
			case 0x76:
				printcmd (soff, 2);
				printf ("IO_CONDITION\t0x%02x\n", bios->data[soff+1]);
				if (bios->data[soff+1] > maxiocond)
					maxiocond = bios->data[soff+1];
				soff += 2;
				break;
			case 0x78:
				printcmd (soff, 6);
				printf ("INDEX_IO\tI[0x%04x][0x%02x] &= 0x%02x |= 0x%02x\n", le16(soff+1), bios->data[soff+3], bios->data[soff+4], bios->data[soff+5]);
				soff += 6;
				break;
			case 0x79:
				printcmd (soff, 7);
				printf ("PLL\tR[0x%06x] =PLL= %dkHz\n", le32(soff+1), le16(soff+5) * 10);
				soff += 7;
				break;
			case 0x7a:
				printcmd (soff, 9);
				printf ("ZM_REG\tR[0x%06x] = 0x%08x\n", le32(soff+1), le32(soff+5));
				soff += 9;
				break;
			case 0x87:
				printcmd (soff, 2);
				printf ("RAM_RESTRICT_PLL 0x%02x\n", bios->data[soff+1]);
				soff += 2;
				for (i = 0; i < ram_restrict_group_count; i++) {
					printcmd(soff, 4);
					printf("\t[0x%02x] 0x%08x\n", i, le32(soff));
					soff += 4;
				}
				break;
			case 0x8c:
				printcmd (soff, 1);
				printf ("UNK8C\n");
				soff++;
				break;
			case 0x8d:
				printcmd (soff, 1);
				printf ("UNK8D\n");
				soff++;
				break;
			case 0x8e:
				printcmd (soff, 1);
				printf ("GPIO\n");
				soff++;
				break;
			case 0x8f:
				printcmd (soff, 7);
				printf ("RAM_RESTRICT_ZM_REG_GROUP\tR[0x%06x] 0x%02x 0x%02x\n", le32(soff+1), bios->data[soff+5], bios->data[soff+6]);
				incr = bios->data[soff+5];
				cnt = bios->data[soff+6];
				dst = le32(soff+1);
				soff += 7;
				while (cnt--) {
					printcmd (soff, 0);
					printf ("\tR[0x%06x] = {\n", dst);
					for (i = 0; i < ram_restrict_group_count; i++) {
						printcmd (soff, 4);
						printf ("\t\t0x%08x\n", le32(soff));
						soff += 4;
					}
					printcmd (soff, 0);
					printf ("\t}\n");
					dst += incr;
				}
				break;
			case 0x90:
				printcmd (soff, 9);
				printf ("COPY_ZM_REG\tR[0x%06x] = R[0x%06x]\n", le32(soff+5), le32(soff+1));
				soff += 9;
				break;
			case 0x91:
				printcmd (soff, 7);
				printf ("ZM_REG_GROUP\tR[0x%06x] =\n", le32(soff+1));
				cnt = bios->data[soff+5];
				soff += 6;
				while (cnt--) {
					printcmd (soff, 4);
					printf ("\t\t0x%08x\n", le32(soff));
					soff += 4;
				}
				break;
			case 0x92:
				printcmd (soff, 1);
				printf ("UNK92\n");
				soff++;
				break;
			case 0x96:
				printcmd (soff, 16);
				printf ("\n");
				printcmd (soff+16, 1);
				x = bios->data[soff+5];
				printf ("XLAT\tR[0x%06x] &= 0x%08x |= "
					"(X%02x((R[0x%06x] %s 0x%02x) & 0x%02x) << 0x%02x)\n",
					le32(soff+8), le32(soff+12),
					bios->data[soff+7], le32(soff+1),
					(x & 0x80) ? "<<" : ">>",
					(x & 0x80) ? (0x100 - x) : x,
					bios->data[soff+6], bios->data[soff+16]);
				soff += 17;
				break;
			case 0x97:
				printcmd (soff, 13);
				printf ("ZM_MASK_ADD\tR[0x%06x] & ~0x%08x += 0x%08x\n", le32(soff+1), le32(soff+5), le32(soff+9));
				soff += 13;
				break;
			case 0x98:
				printcmd (soff, 6);
				cnt = bios->data[soff+5];
				x = le32(soff+1);
				printf ("AUXCH\tAUX[0x%08x] 0x%02x\n", x, cnt);
				soff += 6;
				while (cnt--) {
					printcmd (soff, 2);
					printf ("\t\tAUX[0x%08x] &= 0x%02x |= 0x%02x\n", x, bios->data[soff], bios->data[soff+1]);
					soff += 2;
				}
				break;
			case 0x99:
				printcmd (soff, 6);
				cnt = bios->data[soff+5];
				x = le32(soff+1);
				printf ("ZM_AUXCH\tAUX[0x%08x] 0x%02x\n", x, cnt);
				soff += 6;
				while (cnt--) {
					printcmd (soff, 1);
					printf ("\t\tAUX[0x%08x] = 0x%02x\n", x, bios->data[soff]);
					soff++;
				}
				break;
			case 0x9a:
				printcmd (soff, 7);
				printf ("I2C_IF_LONG\tI2C[0x%02x][0x%02x][0x%02x:0x%02x] & 0x%02x == 0x%02x\n", bios->data[soff+1], bios->data[soff+2], bios->data[soff+4], bios->data[soff+3], bios->data[soff+5], bios->data[soff+6]);
				soff += 7;
				break;
			case 0xa9:
				printcmd (soff, 2);
				printf ("GPIO_NE\n");
				cnt = bios->data[soff+1];
				soff += 2;
				while (cnt--) {
					printcmd (soff, 1);
					x = bios->data[soff];
					if (x != 0xff)
						printf ("\t\tFUNC[0x%02x]\n", x);
					else
						printf ("\n");
					soff++;
				}
				break;
			case 0xaa:
				printcmd (soff, 4);
				printf("UNKAA\n");
				soff += 4;
				break;
			default:
				printcmd (soff, 1);
				printf ("???\n");
				soff++;
				break;
		}
	}
}

static void parse_bios_version(uint16_t offset)
{
	/*
	 * offset + 0  (8 bits): Micro version
	 * offset + 1  (8 bits): Minor version
	 * offset + 2  (8 bits): Chip version
	 * offset + 3  (8 bits): Major version
	 */

	bios->info.version[0] = bios->data[offset + 3];
	bios->info.version[1] = bios->data[offset + 2];
	bios->info.version[2] = bios->data[offset + 1];
	bios->info.version[3] = bios->data[offset + 0];

	if (printmask & ENVY_BIOS_PRINT_BMP_BIT) {
	printf("Bios version 0x%02x.%02x.%02x.%02x\n\n",
		 bios->data[offset + 3], bios->data[offset + 2],
		 bios->data[offset + 1], bios->data[offset]);
	}
}

int set_strap_from_string(const char* strap_s)
{
	char* end_ptr;
	unsigned long int value;

	if (strap_s != NULL) {
		value = strtoul(strap_s, &end_ptr, 16);
		if (value != ULONG_MAX) {
			strap = value;
			printf("Strap set to 0x%x\n", strap);
			return 0;
		}

		fprintf(stderr, "Invalid strap value!\n");
	}

	return 1;
}

int set_strap_from_file(const char *path)
{
	FILE *strapfile = NULL;
	char tmp[21];

	strapfile = fopen(path, "r");
	if (strapfile) {
		fread(tmp, 1, 21, strapfile);
		return set_strap_from_string(tmp);
	}

	return 1;
}

void find_strap(char *filename) {
	char *path;
	const char * strap_filename = "strap_peek";
	const char *pos = strrchr(filename, '/');
	if (pos == NULL)
		pos = filename;
	else
		pos++;
	int base_length = pos-filename;

	path = (char*) malloc(base_length + strlen(strap_filename)+1);
	strncpy(path, filename, base_length);
	strncpy(path+base_length, strap_filename, strlen(strap_filename));

	if(!set_strap_from_file(path))
		printf("Strap register found in '%s'\n", path);

	free(path);
}

int vbios_read(const char *filename, uint8_t **vbios, unsigned int *length)
{
	FILE * fd_bios;

	*vbios = NULL;
	*length = 0;

	fd_bios = fopen(filename ,"rb");
	if (!fd_bios) {
		perror("fopen");
		return 1;
	}

	/* get the size */
	fseek(fd_bios, 0, SEEK_END);
	*length = ftell(fd_bios);
	fseek(fd_bios, 0, SEEK_SET);

	if (*length <= 0) {
		perror("fseek");
		return 1;
	}

	/* Read the vbios */
	*vbios = malloc(*length * sizeof(char));
	if (!*vbios) {
		perror("malloc");
		return 1;
	}
	if (fread(*vbios, 1, *length, fd_bios) < *length) {
		free(*vbios);
		perror("fread");
		return 1;
	}
	if (fclose (fd_bios)) {
		free(*vbios);
		perror("fclose");
		return 1;
	}
	return 0;
}

const char * mem_type(uint8_t version, uint16_t start)
{
	uint8_t ram_type = bios->data[start] & 0x0f;

	if (version != 0x10)
		return "unknown table version (0x%x)\n";

	switch (ram_type) {
	case 0:
		return "DDR2";
		break;
	case 1:
		return "DDR3";
		break;
	case 2:
		return "GDDR3";
		break;
	case 3:
		return "GDDR5";
		break;
	default:
		return "Unknown ram type";
	}
}

int main(int argc, char **argv) {
	int i;
	int c;
	while ((c = getopt (argc, argv, "m:s:i:p:vubh")) != -1)
		switch (c) {
			case 'm':
				sscanf(optarg,"%2hhx",&tCWL);
				printf("tCWL set to %2hhx\n", tCWL);
				break;
			case 's':
				if (!strncmp(optarg,"0x",2)) {
					set_strap_from_string(optarg+2);
				} else {
					set_strap_from_file(optarg);
				}
				break;
			case 'i':
				sscanf(optarg,"%4hx",&script_print);
				break;
			case 'v':
				printmask |= ENVY_BIOS_PRINT_VERBOSE;
				break;
			case 'u':
				printmask |= ENVY_BIOS_PRINT_UNUSED;
				break;
			case 'b':
				printmask |= ENVY_BIOS_PRINT_BLOCKS;
				break;
			case 'p':
				for (i = 0; i < sizeof printmasks / sizeof *printmasks; i++) {
					if (!strcmp(printmasks[i].name, optarg))
						printmask |= printmasks[i].mask;
				}
				break;
			case 'h':
				return usage(argv[0]);
		}

	if (!(printmask & ENVY_BIOS_PRINT_ALL))
		printmask |= ENVY_BIOS_PRINT_ALL;

	if (optind >= argc) {
		return usage(argv[0]);
	}

	bios = calloc (sizeof *bios, 1);
	if (!bios) {
		perror("malloc");
		return 1;
	}

	if (strap == 0) {
		find_strap(argv[optind]);
		if (strap == 0)
			fprintf(stderr, "warning: No strap specified\n");
	}

	if (vbios_read(argv[optind], &bios->data, &bios->origlength)) {
		fprintf(stderr, "Failed to read VBIOS!\n");
		return 1;
	}

	if (envy_bios_parse(bios)) {
		fprintf(stderr, "Failed to parse BIOS\n");
		return 1;
	}
	envy_bios_print(bios, stdout, printmask);

	const struct envy_colors *discolors = &envy_def_colors;
	bool old_init_script = false;

	if (bios->bmp_offset && bios->type == ENVY_BIOS_TYPE_NV04) {
		bmpver_maj = bios->data[bios->bmp_offset+5];
		bmpver_min = bios->data[bios->bmp_offset+6];
		bmpver = bmpver_maj << 8 | bmpver_min;
		if (printmask & ENVY_BIOS_PRINT_BMP_BIT) {
			printf ("BMP 0x%02x.%02x at 0x%x\n", bmpver_maj, bmpver_min, bios->bmp_offset);
			printf ("\n");
		}
		parse_bios_version(bios->bmp_offset + 10);
		if (printmask & ENVY_BIOS_PRINT_BMP_BIT) {
			printhex(bios->bmp_offset, 256);
		}
		if (bmpver >= 0x510) {
			init_script_tbl_ptr = le32(bios->bmp_offset + 75);
			macro_index_tbl_ptr = le32(bios->bmp_offset + 77);
			macro_tbl_ptr = le32(bios->bmp_offset + 79);
			condition_tbl_ptr = le32(bios->bmp_offset + 81);
			io_condition_tbl_ptr = le32(bios->bmp_offset + 83);
			io_flag_condition_tbl_ptr = le32(bios->bmp_offset + 85);
			init_function_tbl_ptr = le32(bios->bmp_offset + 87);
		} else {
			init_script_tbl_ptr = bios->bmp_offset + (bmpver < 0x200 ? 14 : 18);
			old_init_script = true;
		}
		if (bmpver >= 0x527) {
			bios->power.perf.offset = le16(bios->bmp_offset + 148);
			bios->power.volt.offset = le16(bios->bmp_offset + 152);
		}
	}

	if (bios->bit.offset) {
		for (i = 0; i < bios->bit.entriesnum; i++) {
			struct envy_bios_bit_entry *entry = &bios->bit.entries[i];
			uint16_t eoff = entry->t_offset;
			switch (entry->type) {
				case 'I':
					init_script_tbl_ptr = le32(eoff);
					macro_index_tbl_ptr = le32(eoff+2);
					macro_tbl_ptr = le32(eoff+4);
					condition_tbl_ptr = le32(eoff+6);
					io_condition_tbl_ptr = le32(eoff+8);
					io_flag_condition_tbl_ptr = le32(eoff+10);
					init_function_tbl_ptr = le32(eoff+12);
					if (entry->t_len >= 16)
						some_script_ptr = le32(eoff+14);
					if (entry->t_len >= 18)
						init96_tbl_ptr = le32(eoff+16);
					break;
				case 'M':
					if (entry->version == 1) {
						if (entry->t_len >= 5) {
							ram_restrict_group_count = bios->data[eoff+2];
							ram_restrict_tbl_ptr = le16(eoff+3);
						}
					} else if (entry->version == 2) {
						if (entry->t_len >= 3) {
							ram_restrict_group_count = bios->data[eoff];
							ram_restrict_tbl_ptr = le16(eoff+1);
							ram_type_tbl_ptr = le16(eoff+3);
						}
					}

					if (printmask & ENVY_BIOS_PRINT_BMP_BIT) {
						if (entry->t_len >= 7)
							printf("M.tbl_05 at 0x%x\n", le16(eoff+5));
						if (entry->t_len >= 9)
							printf("M.tbl_07 at 0x%x\n", le16(eoff+7));
						if (entry->t_len >= 0xb)
							printf("M.tbl_09 at 0x%x\n", le16(eoff+9));
						if (entry->t_len >= 0xd)
							printf("M.tbl_0b at 0x%x\n", le16(eoff+0xb));
						if (entry->t_len >= 0xf)
							printf("M.tbl_0c at 0x%x\n", le16(eoff+0xd));
						if (entry->t_len >= 0x11)
							printf("M.tbl_0d at 0x%x\n", le16(eoff+0xf));
					}
					break;
				case 'C':
					pll_limit_tbl_ptr = le16(eoff + 8);
					break;
				case 'U':
					disp_script_tbl_ptr = le16(eoff);
					break;
			}
		}
	}

	if(script_print != 0) {
		printscript(script_print);
		exit(0);
	}

	if (bios->hwsq_offset && (printmask & ENVY_BIOS_PRINT_HWSQ)) {
		uint8_t entry_count, bytes_to_write, i;
		const struct disisa *hwsq_isa = ed_getisa("hwsq");
		struct varinfo *hwsq_var_nv41 = varinfo_new(hwsq_isa->vardata);
		varinfo_set_variant(hwsq_var_nv41, "nv41");

		bios->hwsq_offset += 4;

		entry_count = bios->data[bios->hwsq_offset];
		bytes_to_write = bios->data[bios->hwsq_offset + 1];

		printf("HWSQ table at 0x%x: Entry count %u, entry length %u\n", bios->hwsq_offset, entry_count, bytes_to_write);

		for (i=0; i < entry_count; i++) {
			uint16_t entry_offset = bios->hwsq_offset + 2 + i * bytes_to_write;
			uint32_t sequencer = le32(entry_offset);
			//uint8_t bytes_written = 4;

			printf("-- HWSQ entry %u at 0x%x: sequencer control = %u\n", i, entry_offset, sequencer);
			envydis(hwsq_isa, stdout, bios->data + entry_offset + 4, 0, bytes_to_write - 4, hwsq_var_nv41, 0, 0, 0, discolors);
			printf ("\n");
		}
		printf ("\n");
	}

	if (pll_limit_tbl_ptr && (printmask & ENVY_BIOS_PRINT_PLL)) {
		uint8_t ver = bios->data[pll_limit_tbl_ptr];
		uint8_t hlen = 0, rlen = 0, entries = 0;
		printf ("PLL limits table at 0x%x, version %d\n", pll_limit_tbl_ptr, ver);
		switch (ver) {
			case 0:
				break;
			case 0x10:
			case 0x11:
				hlen = 1;
				rlen = 0x18;
				entries = 1;
				break;
			case 0x20:
			case 0x21:
			case 0x30:
			case 0x40:
				hlen = bios->data[pll_limit_tbl_ptr+1];
				rlen = bios->data[pll_limit_tbl_ptr+2];
				entries = bios->data[pll_limit_tbl_ptr+3];
		}
		printhex(pll_limit_tbl_ptr, hlen);
		uint16_t soff = pll_limit_tbl_ptr + hlen;
		for (i = 0; i < entries; i++) {
			if (ver == 0x20 || ver ==0x21) {
				uint32_t ref_clk = 0;
				printf("-- Register 0x%08x --\n", le32(soff));
				printf("-- VCO1 - ");
				printf("freq [%d-%d]MHz, inputfreq [%d-%d]MHz, N [%d-%d], M [%d-%d] --\n",
					le16(soff+4), le16(soff+6), le16(soff+12), le16(soff+14),
					bios->data[soff+20], bios->data[soff+21], bios->data[soff+22], bios->data[soff+23]);
				printf("-- VCO2 - ");
				printf("freq [%d-%d]MHz, inputfreq [%d-%d]MHz, N [%d-%d], M [%d-%d] --\n",
					le16(soff+8), le16(soff+10), le16(soff+16), le16(soff+18),
					bios->data[soff+24], bios->data[soff+25], bios->data[soff+26], bios->data[soff+27]);

				if (rlen > 0x22)
					ref_clk = le32(soff+31);

				// nv4x cards detect the ref_clk from the strap
				// XXX: C51 has some extra ref_clk hacks
				if (strap && ref_clk == 0)
					switch (strap & (1 << 22 | 1 << 6)) {
					case 0:
						ref_clk = 13500;
						break;
					case (1 << 6):
						ref_clk = 14318;
						break;
					case (1 << 22):
						ref_clk = 27000;
						break;
					case (1 << 22 | 1 << 6):
						ref_clk = 25000;
						break;
					}
				printf("-- log2P_min [%d], log2P_max [%d](XXX: must be less than 8), log2P_bias [%d], ref_clk %dkHz --\n",
					bios->data[soff+28], bios->data[soff+29], bios->data[soff+30], ref_clk);
				printhex(soff, rlen);
				printf("\n");
			} else if (ver == 0x30) {
				uint16_t rec_ptr = le16(soff+1);
				uint32_t ref_clk;
				printf("-- ID 0x%02x Register 0x%08x rec_ptr 0x%x --\n",
					bios->data[soff], le32(soff+3), rec_ptr);
				printhex(soff, rlen);
				printf("\n");
				if (rec_ptr) {
					printf("-- VCO1 - ");
					printf("freq [%d-%d]MHz, inputfreq [%d-%d]MHz, N [%d-%d], M [%d-%d] --\n",
						le16(rec_ptr), le16(rec_ptr+2), le16(rec_ptr+8), le16(rec_ptr+10),
						bios->data[rec_ptr+16], bios->data[rec_ptr+17], bios->data[rec_ptr+18], bios->data[rec_ptr+19]);
					printf("-- VCO2 - ");
					printf("freq [%d-%d]MHz, inputfreq [%d-%d]MHz, N [%d-%d], M [%d-%d] --\n",
						le16(rec_ptr+4), le16(rec_ptr+6), le16(rec_ptr+12), le16(rec_ptr+14),
						bios->data[rec_ptr+20], bios->data[rec_ptr+21], bios->data[rec_ptr+22], bios->data[rec_ptr+23]);

					/* Some rare cases have an ref_clk set to 0 in the entry
					 * Use the same approach we did for nv4x
					 */
					ref_clk = le32(rec_ptr+28);

					if (strap && ref_clk == 0)
						switch (strap & (1 << 22 | 1 << 6)) {
						case 0:
							ref_clk = 13500;
							break;
						case (1 << 6):
							ref_clk = 14318;
							break;
						case (1 << 22):
							ref_clk = 27000;
							break;
						case (1 << 22 | 1 << 6):
							ref_clk = 25000;
							break;
						}

					printf("-- log2P_min [%d], log2P_max [%d], log2P_UNK [%d], log2P_bias [%d], ref_clk %dkHz --\n",
						bios->data[rec_ptr+24], bios->data[rec_ptr+25], bios->data[rec_ptr+26], bios->data[rec_ptr+27], ref_clk);
					printhex(rec_ptr, 32);
					printf("\n");
				}
			} else if (ver == 0x40) {
				uint16_t rec_ptr = le16(soff+1);
				printf("-- ID 0x%02x Register 0x%08x rec_ptr 0x%x, ref_clk %dkHz, (?)alt_ref_clk %dkHz --\n",
					bios->data[soff], le32(soff+3), rec_ptr, le16(soff+9)*1000, le16(soff+7)*1000);
				printhex(soff, rlen);
				printf("\n");
				if (rec_ptr) {
					printf("-- VCO1 - ");
					printf("freq [%d-%d]MHz, inputfreq [%d-%d]MHz, M [%d-%d], N [%d-%d], P [%d-%d] --\n",
						le16(rec_ptr), le16(rec_ptr+2), le16(rec_ptr+4), le16(rec_ptr+6),
						bios->data[rec_ptr+8], bios->data[rec_ptr+9], bios->data[rec_ptr+10], bios->data[rec_ptr+11],
						bios->data[rec_ptr+12], bios->data[rec_ptr+13]);
					printhex(rec_ptr, 14);
					printf("\n");
				}
			}
			soff += rlen;
		}
		printf("\n");
	}

	if (init_script_tbl_ptr && (printmask & ENVY_BIOS_PRINT_SCRIPTS)) {
		i = 0;
		uint16_t off = init_script_tbl_ptr;
		uint16_t soff;
		while ((soff = le16(off)) && (!old_init_script || i < 2)) {
			off += 2;
			ADDARRAY(subs, i);
			i++;
		}
		printf ("Init script table at 0x%x: %d main scripts\n\n", init_script_tbl_ptr, i);
	}

	int subspos = 0, callspos = 0;
	int ssdone = 0;
	while ((!ssdone || subspos < subsnum || callspos < callsnum) && (printmask & ENVY_BIOS_PRINT_SCRIPTS)) {
		if (callspos < callsnum) {
			uint16_t soff = calls[callspos++];
			printf ("Subroutine at 0x%x:\n", soff);
			printscript(soff);
			printf("\n");
		} else if (subspos < subsnum) {
			int i = subs[subspos++];
			uint16_t soff = le32(init_script_tbl_ptr + 2*i);
			printf ("Init script %d at 0x%x:\n", i, soff);
			printscript(soff);
			printf("\n");
		} else if (!ssdone) {
			if (some_script_ptr) {
				printf ("Some script at 0x%x:\n", some_script_ptr);
				printscript(some_script_ptr);
				printf("\n");
			}
			ssdone = 1;
		}
	}

	if (disp_script_tbl_ptr && (printmask & ENVY_BIOS_PRINT_SCRIPTS)) {
		uint8_t ver = bios->data[disp_script_tbl_ptr];
		uint8_t hlen = bios->data[disp_script_tbl_ptr+1];
		uint8_t rlen = bios->data[disp_script_tbl_ptr+2];
		uint8_t entries = bios->data[disp_script_tbl_ptr+3];
		uint8_t rhlen = bios->data[disp_script_tbl_ptr+4];
		printf ("Display script table at 0x%x, version %d:\n", disp_script_tbl_ptr, ver);
		printhex(disp_script_tbl_ptr, hlen + rlen * entries);
		printf ("\n");
		for (i = 0; i < entries; i++) {
			uint16_t table = le16(disp_script_tbl_ptr + hlen + i * rlen);
			if (!table)
				continue;
			uint32_t entry = le32(table);
			uint8_t configs = bios->data[table+5];
			printf ("Subtable %d at 0x%x for 0x%08x:\n", i, table, entry);
			printhex(table, rhlen + configs * 6);
			printf("\n");
		}
		printf("\n");
	}

	if (condition_tbl_ptr && (printmask & ENVY_BIOS_PRINT_SCRIPTS)) {
		printf ("Condition table at 0x%x: %d conditions:\n", condition_tbl_ptr, maxcond+1);
		for (i = 0; i <= maxcond; i++) {
			printcmd(condition_tbl_ptr + 12 * i, 12);
			printf ("[0x%02x] R[0x%06x] & 0x%08x == 0x%08x\n", i,
					le32(condition_tbl_ptr + 12 * i),
					le32(condition_tbl_ptr + 12 * i + 4),
					le32(condition_tbl_ptr + 12 * i + 8));
		}
		printf("\n");
	}

	if (macro_index_tbl_ptr && (printmask & ENVY_BIOS_PRINT_SCRIPTS)) {
		printf ("Macro index table at 0x%x: %d macro indices:\n", macro_index_tbl_ptr, maxmi+1);
		for (i = 0; i <= maxmi; i++) {
			printcmd(macro_index_tbl_ptr + 2 * i, 2);
			printf ("[0x%02x] 0x%02x *%d\n", i,
					bios->data[macro_index_tbl_ptr + 2 * i],
					bios->data[macro_index_tbl_ptr + 2 * i + 1]);
			if (bios->data[macro_index_tbl_ptr + 2 * i] > maxmac)
				maxmac = bios->data[macro_index_tbl_ptr + 2 * i];
		}
		printf("\n");
	}

	if (macro_tbl_ptr && (printmask & ENVY_BIOS_PRINT_SCRIPTS)) {
		printf ("Macro table at 0x%x: %d macros:\n", macro_tbl_ptr, maxmac+1);
		for (i = 0; i <= maxmac; i++) {
			printcmd(macro_tbl_ptr + 8 * i, 8);
			printf ("[0x%02x] R[0x%06x] = 0x%08x\n", i,
					le32(macro_tbl_ptr + 8 * i),
					le32(macro_tbl_ptr + 8 * i + 4));
		}
		printf("\n");
	}

	if (ram_type_tbl_ptr && (printmask & ENVY_BIOS_PRINT_RAM)) {
		uint8_t version, entry_count = 0, entry_length = 0;
		uint16_t start = ram_type_tbl_ptr;
		uint8_t ram_cfg = strap?(strap & 0x1c) >> 2:0xff;
		int i;
		version = bios->data[start];
		entry_count = bios->data[start+3];
		entry_length = bios->data[start+2];

		printf("Ram type table at 0x%x: Version %d, %u entries\n", start, version, entry_count);
		start += bios->data[start+1];
		if(version == 0x10) {
			printf("Detected ram type: %s\n",
			       mem_type(version, start + (ram_cfg*entry_length)));
		}
		printf("\n");

		for (i = 0; i < entry_count; i++) {
			if(i == ram_cfg) {
				printf("*");
			} else {
				printf(" ");
			}
			printcmd(start, entry_length);
			printf("Ram type: %s\n", mem_type(version, start));
			start += entry_length;
		}

		printf("\n");
	}
#define subent(n) (subentry_offset + ((n) * subentry_size))
	if (bios->power.perf.offset && (printmask & ENVY_BIOS_PRINT_PERF)) {
		uint8_t version = 0, entry_count = 0, entry_length = 0;
		uint8_t mode_info_length = 0, header_length = 0;
		uint8_t extra_data_length = 8, extra_data_count = 0;
		uint8_t subentry_offset = 0, subentry_size = 0, subentry_count = 0;
		uint16_t start = bios->power.perf.offset;
		uint8_t ram_cfg = strap?(strap & 0x1c) >> 2:0xff;
		char sub_entry_engine[16][11] = { "unk" };
		int e;
		
		for (i = 0; i < sizeof(sub_entry_engine) / sizeof(*sub_entry_engine); i++)
			strncpy(sub_entry_engine[i], "unk_engine", 10);

		if (bios->info.version[0] == 0x4) {
			header_length = bios->data[start+0];
			version = bios->data[start+1];
			entry_length = bios->data[start+3];
			entry_count = bios->data[start+2];
			mode_info_length = entry_length;
		} else if (bios->info.version[0] < 0x70) {
			version = bios->data[start+0];
			header_length = bios->data[start+1];
			entry_count = bios->data[start+2];
			mode_info_length = bios->data[start+3];
			extra_data_count = bios->data[start+4];
			extra_data_length = bios->data[start+5];
			entry_length = mode_info_length + extra_data_count * extra_data_length;
		} else if ((bios->info.version[0] & 0xf0) <= 0x80) {
			version = bios->data[start+0];
			header_length = bios->data[start+1];
			subentry_offset = bios->data[start+2];
			subentry_size = bios->data[start+3];
			subentry_count = bios->data[start+4];
			entry_length = subentry_offset + subentry_size * subentry_count;
			mode_info_length = subentry_offset;
			entry_count = bios->data[start+5];
		} else {
			printf("Unknown PM major version %d\n", bios->info.version[0]);
		}

		printf ("PM_Mode table at 0x%x. Version %d. RamCFG 0x%x. Info_length %i.\n",
			start, version, ram_cfg, mode_info_length
		);

		if (version > 0x20 && version < 0x40)
			printf("PWM_div 0x%x, ", le16(start+6));


		if (version > 0x15 && version < 0x40)
			printf("Extra_length %i. Extra_count %i.\n", extra_data_length, extra_data_count);
		else if (version == 0x40)
			printf("Subentry length %i. Subentry count %i. Subentry Offset %i\n", subentry_size, subentry_count, subentry_offset);
		else
			printf("Version unknown\n");

		printf("Header:\n");
		printcmd(start, header_length>0?header_length:10);
		printf ("\n");

		start += header_length;

		printf("%i performance levels\n", entry_count);
		for (i=0; i < entry_count; i++) {
			uint16_t id, fan, voltage, memscript;
			uint16_t core, shader = 0, memclk, vdec = 0;
			uint8_t pcie_width = 0xff;
			uint8_t timing_id = 0xff;

			if (mode_info_length >= 28 && version >= 0x25) {
				/* it is possible that the link width is a uint16_t
				* note the variable used is uint8_t
				* 0x25 is a guess, ofcourse cards before NV40 don't actually
				* have PCI Express... so this value would be silly
				*/
				pcie_width = bios->data[start+28];
			}

			if (version > 0x15 && version < 0x40) {
				uint16_t extra_start = start + mode_info_length;
				uint16_t timing_extra_data = extra_start+(ram_cfg*extra_data_length);
				timing_id = parse_memtm_mapping_entry_10(timing_extra_data, extra_data_length, 0);
				if (ram_cfg < extra_data_count)
					timing_id = bios->data[timing_extra_data+1];
			} else if (version == 0x40 && ram_cfg < subentry_count) {
				// Get the timing from somewhere else
				// timing_id = bios->data[start+subent(ram_cfg)+1];
				timing_id = 0xff;
			}

			/* Old BIOS' might give rounding errors! */
			if (version == 0x12 || version == 0x13 || version == 0x15) {
				id = bios->data[start+0];
				core = le16(start+1) / 100;
				memclk = le16(start+5) / 100;
				fan = bios->data[start+55];
				voltage = bios->data[start+56];

				printf ("\n-- ID 0x%x Core %dMHz Memory %dMHz "
					"Voltage %d[*10mV] Timing %d Fan %d PCIe link width %d --\n",
					id, core, memclk, voltage, timing_id, fan, pcie_width );
			} else if (version >= 0x21 && version <= 0x24) {
				id = bios->data[start+0];
				core = le16(start+6);
				shader = core + (signed char) bios->data[start+8];
				memclk = le16(start+11);
				fan = bios->data[start+4];
				voltage = bios->data[start+5];

				printf ("\n-- ID 0x%x Core %dMHz Memory %dMHz Shader %dMHz "
					"Voltage %d[*10mV] Timing %d Fan %d PCIe link width %d --\n",
					id, core, memclk, shader, voltage, timing_id, fan, pcie_width );
			} else if (version == 0x25) {
				id = bios->data[start+0];
				core = le16(start+6);
				shader = le16(start+10);
				memclk = le16(start+12);
				fan = bios->data[start+4];
				voltage = bios->data[start+5];

				printf ("\n-- ID 0x%x Core %dMHz Memory %dMHz Shader %dMHz "
					"Voltage %d[*10mV] Timing %d Fan %d PCIe link width %d --\n",
					id, core, memclk, shader, voltage, timing_id, fan, pcie_width );
			} else if (version == 0x30 || version == 0x35) {
				uint16_t dom6 = 0;
				id = bios->data[start+0];
				fan = bios->data[start+6];
				voltage = bios->data[start+7];
				core = le16(start+8);
				shader = le16(start+10);
				memclk = le16(start+12);
				vdec = le16(start+16);
				dom6 = le16(start+20);
				memscript = le16(start+2);

				printf(" 10053c: 0x%.8x", memclk >= 348 ? 0x1000 : 0);
				printf ("\n-- ID 0x%x Core %dMHz Memory %dMHz Shader %dMHz Vdec %dMHz "
					"Dom6 %dMHz Voltage %d[*10mV] Timing %d Fan %d PCIe link width %d --\n",
					id, core, memclk, shader, vdec, dom6, voltage, timing_id, fan, pcie_width );
				if(memscript > 0) {
					printf("Memory reclock script:\n");
					printscript(memscript);
					printf("\n");
				}
			} else if (version == 0x40) {
				id = bios->data[start+0];
				voltage = bios->data[start+2];

				if (bios->chipset < 0xc0) {
					strncpy(sub_entry_engine[0], "core", 10);
					strncpy(sub_entry_engine[1], "shader", 10);
					strncpy(sub_entry_engine[2], "memclk", 10);
					strncpy(sub_entry_engine[3], "vdec", 10);
					strncpy(sub_entry_engine[4], "unka0", 10);

					printf ("\n-- ID 0x%x Voltage entry %d PCIe link width %d --\n",
						id, voltage, pcie_width );
				} else {
					strncpy(sub_entry_engine[0], "hub06", 10);
					strncpy(sub_entry_engine[1], "hub01", 10);
					strncpy(sub_entry_engine[2], "copy", 10);
					strncpy(sub_entry_engine[3], "shader", 10);
					strncpy(sub_entry_engine[4], "rop", 10);
					strncpy(sub_entry_engine[5], "memclk", 10);
					strncpy(sub_entry_engine[6], "vdec", 10);
					strncpy(sub_entry_engine[10], "daemon", 10);
					strncpy(sub_entry_engine[11], "hub07", 10);

					printf ("\n-- ID 0x%x Voltage entry %d PCIe link width %d --\n",
						id, voltage, pcie_width );
				}
			}

			if (mode_info_length > 20) {
				int i=0;
				while (mode_info_length - i*20 > 20) {
					printcmd(start+i*20, 20); printf("\n");
					i++;
				}
				printcmd(start + i*20, mode_info_length%20);
			} else {
				printcmd(start, mode_info_length);
			}
			printf("\n");

			if (version > 0x15 && version < 0x40) {
				for(e=0; e < extra_data_count; e++) {
					printf("	%i:", e);
					printcmd(start + mode_info_length + (e*extra_data_length), extra_data_length);
					printf("\n");
				}
			} else if (version == 0x40) {
				for(e=0; e < subentry_count; e++) {
					printf("	%i:", e);
					printcmd(start + mode_info_length + (e*subentry_size), subentry_size);
					printf(" : %s freq = %u MHz\n", sub_entry_engine[e], le16(start+subent(e)) & 0xfff);
				}
			}
			printf("\n\n");

			start += entry_length;
		}
		printf("\n");
	}

	if (bios->power.volt_map.offset && (printmask & ENVY_BIOS_PRINT_PERF)) {
		uint8_t version = 0, entry_count = 0, entry_length = 0;
		uint8_t header_length = 0;
		uint16_t start = bios->power.volt_map.offset;

		version = bios->data[start+0];
		header_length = bios->data[start+1];
		entry_length = bios->data[start+2];
		entry_count = bios->data[start+3];

		printf ("Voltage map table at 0x%x. Version %d.\n", start, version);

		printf("Header:\n");
		printcmd(start, header_length>0?header_length:10);
		printf("\n\n");

		start += header_length;

		switch(version) {
		case 0x10:
			for (i=0; i < entry_count; i++) {
				printf ("-- ID = %u: voltage_min = %u, voltage_max = %u [µV] --\n",
					i, le32(start), le32(start+4));
				printcmd(start, entry_length);
				printf("\n\n");

				start += entry_length;
			}
			break;
		case 0x20:
			for (i=0; i < entry_count; i++) {
				printf ("-- ID = %u, link: %hhx, voltage_min = %u, voltage_max = %u [µV] --\n",
					i, bios->data[start+1], le32(start+2), le32(start+6));
				printcmd(start, entry_length);
				printf("\n\n");

				start += entry_length;
			}
		}

		printf("\n");
	}

	if (bios->power.volt.offset && (printmask & ENVY_BIOS_PRINT_PERF)) {
		uint8_t version = 0, entry_count = 0, entry_length = 0;
		uint8_t header_length = 0, mask = 0;
		uint16_t start = bios->power.volt.offset;
		int16_t step_uv = 0;
		uint32_t volt_uv = 0, volt;
		uint8_t shift = 0;
		const int vidtag[] =  { 0x04, 0x05, 0x06, 0x1a, 0x73, 0x74, 0x75, 0x76 };
		int nr_vidtag = sizeof(vidtag) / sizeof(vidtag[0]);

		version = bios->data[start+0];
		if (version == 0x10 || version == 0x12) {
			entry_count = bios->data[start+2];
			entry_length = bios->data[start+1];
			header_length = 5;
			mask = bios->data[start+4];
		} else if (version == 0x20) {
			entry_count = bios->data[start+2];
			entry_length = bios->data[start+3];
			header_length = bios->data[start+1];
			mask = bios->data[start+5];
		} else if (version == 0x30) {
			entry_count = bios->data[start+3];
			entry_length = bios->data[start+2];
			header_length = bios->data[start+1];
			mask = bios->data[start+4];
			shift = 2;
		} else if (version == 0x40) {
			header_length = bios->data[start+1];
			entry_length = bios->data[start+2];
			entry_count = bios->data[start+3];	// XXX: NFI what the entries are for
			mask = bios->data[start+11];			// guess
			step_uv = le16(start+8);
			volt_uv = le32(start+4);
		} else if (version == 0x50) {
			header_length = bios->data[start+1];
			entry_length = bios->data[start+2];
			entry_count = bios->data[start+3];	// XXX: NFI what the entries are for
			mask = bios->data[start+6];			// guess
			step_uv = le16(start+22);
			volt_uv = le32(start+10);
			volt_uv &= 0x00ffffff;
		}


		printf ("Voltage table at 0x%x. Version %d.\n", start, version);

		printf("Header:\n");
		printcmd(start, header_length>0?header_length:10);
		printf (" mask = 0x%x\n\n", mask);

		if (version < 0x40) {
			start += header_length;

			printf ("%i entries\n", entry_count);
			for (i=0; i < entry_count; i++) {
				uint32_t id, label;

				id = bios->data[start+1] >> shift;
				label = bios->data[start+0] * 10000;

				printf ("-- Vid = 0x%x, voltage = %u µV --\n", id, label);

				/* List the gpio tags assosiated with each voltage id */
				int j;
				for (j = 0; j < nr_vidtag; j++) {
					if (!(mask & (1 << j))) {
					/*	printf("-- Voltage unused/overridden by voltage mask --\n");*/
						continue;
	 				}
					printf("-- GPIO tag 0x%x(VID) data (logic %d) --\n", vidtag[j], (!!(id & (1 << j))));
				}

				if (entry_length > 20) {
					printcmd(start, 20); printf("\n");
					printcmd(start + 20, entry_length - 20);
				} else {
					printcmd(start, entry_length);
				}
				printf("\n\n");

				start += entry_length;
			}
		} else if (version == 0x40) {
			int realtag[8];
			int max_uv_used = le32(start+14);

			printf("-- Maximum voltage %d µV, voltage step %d µV, Maximum voltage to be used %d µV --\n",
					volt_uv, step_uv, max_uv_used);
			printf ("-- Voltage range = %u-%u µV, step = %i µV --\n\n",
				volt_uv, volt_uv + step_uv * mask, step_uv);
			start += header_length;

			memcpy(realtag, vidtag, sizeof(realtag));

			for (i = 0; i < entry_count; i++) {
#if 0
				// Speculation, not confirmed in any way
				// Nvidia seems to have 8 1-byte entries for v40:
				// 00 11 22 33 44 58 68 78

				uint8_t bit = bios->data[start] >> 4;
				uint8_t gpio = bios->data[start] & 0xf;

				if (gpio < 8)
					realtag[bit] = vidtag[gpio];
				else
					realtag[bit] = 0;

				printf("-- bit %u mapped to GPIO tag 0x%x(VID)\n", bit, realtag[bit]);
#endif
				printcmd(start, entry_length); printf("\n\n");
				start += entry_length;
			}

			for (i = 0; i <= mask; ++i, volt_uv += step_uv) {
				int j;

				printf("-- Vid %d, voltage %d µV --\n", i, volt_uv);
				if (i & ~mask) {
					printf("-- Voltage unused/overridden by voltage mask --\n\n");
					continue;
				}

				if (volt_uv > max_uv_used) {
					printf("-- Voltage unused: higher than limit --\n\n");
					continue;
				}

				for (j = 0; j < nr_vidtag; j++) {
					if ((1 << j) & ~mask)
						continue;
					printf("-- GPIO tag 0x%x(VID) data (logic %d) --\n", realtag[j], (!!(i & (1 << j))));
				}
				printf("\n");
			}
		} else {
			printf("-- Maximum voltage %d µV, voltage step %d µV, Maximum voltage to be used %d µV --\n",
					volt_uv, step_uv, le32(start+14));
//			printf ("-- Voltage range = %u-%u µV, step = %i µV --\n\n",
//				volt_uv, volt_uv + step_uv * mask, step_uv);
			start += header_length;

			for (i = 0; i < entry_count; i++) {
				volt = le32(start) & 0x000fffff;
				printf("-- Vid %d, voltage %d µV/%d µV --\n", i, volt_uv, volt);
				volt_uv += step_uv;
				/* List the gpio tags assosiated with each voltage id */
				int j;
				for (j = 0; j < nr_vidtag; j++) {
					if (!(mask & (1 << j))) {
					/*	printf("-- Voltage unused/overridden by voltage mask --\n");*/
						continue;
					}
					printf("-- GPIO tag 0x%x(VID) data (logic %d) --\n", vidtag[j], (!!(i & (1 << j))));
				}
				printcmd(start, entry_length); printf("\n");
				printf("\n");
				start += entry_length;
			}
		}

		printf("\n");
	}
	if (bios->power.therm.offset && (printmask & ENVY_BIOS_PRINT_PERF)) {
		uint8_t version = 0, entry_count = 0, entry_length = 0;
		uint8_t header_length = 0;
		uint16_t start = bios->power.therm.offset;
		uint8_t cur_section = -1;

		uint8_t fan_speed_tbl[] = { 0, 0, 25, 0, 40, 0, 50, 0, 75, 0, 85, 0, 100, 0, 100, 0 };

		version = bios->data[start+0];
		header_length = bios->data[start+1];
		entry_length = bios->data[start+2];
		entry_count = bios->data[start+3];

		start += header_length;

		printf ("Temperature table at 0x%x. Version %d.\n", start, version);

		printf("Header:\n");
		printcmd(start, header_length>0?header_length:10);
		printf ("\n\n");

		printf ("%i entries\n", entry_count);
		for (i=0; i < entry_count; i++) {
			uint8_t id = bios->data[(start+i*entry_length)+0];
			uint16_t data = le16((start+i*entry_length)+1);
			uint16_t temp = (data & 0x0ff0) >> 4;
			uint16_t type = (data & 0xf00f);
			uint8_t fan_speed = fan_speed_tbl[(type >> 12) & 0xf];
			uint8_t hysteresis = type & 0xf;
			const char *section_s = NULL, *threshold = NULL;
			const char *correction_target = NULL;
			int16_t correction_value = 0;
			uint16_t byte_low, byte_high;

			if (id == 0x0)
				cur_section = data;

			section_s = (cur_section == 0?"core":"ambient");

			/* Temperatures */
			if (id == 0x4)
				threshold = "critical";
			else if (id == 0x5 || id == 0x7)
				threshold = "throttling";
			else if (id == 0x8)
				threshold = "fan boost";

			/* fan */
			byte_low = data & 0xff;
			byte_high = (data & 0xff00) >> 8;

			correction_value = data;
			if (id == 0x1) {
				correction_target = "temp offset";
				correction_value = ((int8_t)bios->data[(start+i*entry_length)+2]) / 2;
			}
			else if (id == 0x10)
				correction_target = "offset numerator";
			else if (id == 0x11)
				correction_target = "offset denominator";
			else if (id == 0x12)
				correction_target = "slope multiplier";
			else if (id == 0x13)
				correction_target = "slope divisor";

			printcmd(start+i*entry_length, entry_length);
			printf ("id = 0x%x, data = 0x%x ",
						id, data);

			if (id == 0xff)
				printf("	-- disabled");
			else if (id == 0x0)
				printf ("	-- new section type %i -> %s", data, section_s);
			else if (threshold)
				printf ("	-- %s %s temperature is %i°C hysteresis %i°C", threshold, section_s, temp, hysteresis);
			else if (id == 0x1 || (id >= 0x10 && id <= 0x13)) {
				printf ("	-- temp management: ");
				if (correction_target)
					printf("%s = %i", correction_target, correction_value);
				else
					printf("id = 0x%x, data = 0x%x", id, data);
			} else if (id == 0x22)
				printf ("	-- fan_min = %i, fan_max = %i", byte_low, byte_high);
			else if (id == 0x24)
				printf ("	-- bump fan speed to %i%% when at %i°C hysteresis %i°C", fan_speed, temp, hysteresis);
			else if (id == 0x25)
				printf ("	-- bump fan speed to %i%%", data);
			else if (id == 0x26)
				printf("	-- pwm frequency %i", data);
			else if (id == 0x3b)
				printf ("	-- fan bump (about 3 %%/s) periodicity %i ms", data);
			else if (id == 0x3c)
				printf ("	-- fan slow down periodicity %i ms", data);
			else if (id == 0x46)
				printf ("	-- Linear fan mode: fan_bump_temp_min = %i°C, fan_max_temp = %i°C", byte_low, byte_high);
			else
				printf ("	-- unknown (temp ?= %i°C, type ?= 0x%x)", temp, type);

			printf("\n");
		}
		printf("\n");
	}
	if (bios->power.timing.offset && (printmask & ENVY_BIOS_PRINT_PERF)) {
		uint8_t version = 0, entry_count = 0, entry_length = 0;
		uint8_t header_length = 0;
		uint16_t start = bios->power.timing.offset;
		uint8_t tWR, tWTR, tCL;
		uint8_t tRC;		/* Byte 3 */
		uint8_t tRFC;	/* Byte 5 */
		uint8_t tRAS;	/* Byte 7 */
		uint8_t tRCD;		/* Byte 9 */
		uint8_t tUNK_10, tUNK_11, tUNK_12, tUNK_13, tRAM_FT1; /* 14 */
		uint8_t tUNK_18, /* 19 == tCWL */tUNK_20, tUNK_21;
		uint32_t reg_100220 = 0, reg_100224 = 0, reg_100228 = 0, reg_10022c = 0;
		uint32_t reg_100230 = 0, reg_100234 = 0, reg_100238 = 0, reg_10023c = 0;
		uint32_t *bios_data;

		version = bios->data[start+0];
		if (version == 0x10) {
			header_length = bios->data[start+1];
			entry_count = bios->data[start+2];
			entry_length = bios->data[start+3];
		} else if(version == 0x20) {
			header_length = bios->data[start+1];
			entry_count = bios->data[start+3];
			entry_length = bios->data[start+2];
		}

		printf ("Timing table at 0x%x. Version %d.\n", start, version);

		printf("Header:\n");
		printcmd(start, header_length>0?header_length:10);
		printf ("\n\n");

		start += header_length;

		printf ("%i entries\n", entry_count);
		if(version == 0x10) {
			for (i = 0; i < entry_count; i++) {
				tUNK_18 = 1;
				tUNK_20 = 0;
				tUNK_21 = 0;
				switch (entry_length<22?entry_length:22) {
				case 22:
					tUNK_21 = bios->data[start+21];
				case 21:
					tUNK_20 = bios->data[start+20];
				case 20:
					if(bios->data[start+19] > 0)
						tCWL = bios->data[start+19];
				case 19:
					tUNK_18 = bios->data[start+18];
				default:
					tWR  = bios->data[start+0];
					tWTR  = bios->data[start+1];
					tCL  = bios->data[start+2];
					tRC     = bios->data[start+3];
					tRFC    = bios->data[start+5];
					tRAS    = bios->data[start+7];
					tRCD     = bios->data[start+9];
					tUNK_10 = bios->data[start+10];
					tUNK_11 = bios->data[start+11];
					tUNK_12 = bios->data[start+12];
					tUNK_13 = bios->data[start+13];
					tRAM_FT1 = bios->data[start+14];
					break;
				}

				printcmd(start, entry_length); printf("\n");
				if (bios->data[start+0] != 0) {
					printf("Entry %d: WR(%02d), WTR(%02d),  CL(%02d)\n",
						i, tWR,tWTR,tCL);
					printf("       : RC(%02d), RFC(%02d), RAS(%02d), RCD(%02d)\n\n",
						tRC, tRFC, tRAS, tRCD);
					/** First parse RAM_FT1 */
					printf("RAM FT:");
					printf(" ODT(%x)", tRAM_FT1 & 0xf);
					printf(" DRIVE_STRENGTH(%x)\n", (tRAM_FT1 & 0xf0) >> 4);

					if (bios->chipset < 0xc0) {
						reg_100220 = (tRCD << 24 | tRAS << 16 | tRFC << 8 | tRC);
						reg_100224 = ((tWR + 2 + (tCWL - 1)) << 24 |
									(tUNK_18 ? tUNK_18 : 1) << 16 |
									(tWTR + 2 + (tCWL - 1)) << 8);

						reg_100228 = ((tCWL - 1) << 24 | (tUNK_12 << 16) | tUNK_11 << 8 | tUNK_10);

						if(bios->chipset < 0x50) {
							/* Don't know, don't care...
							* don't touch the rest */
							reg_100224 |= (tCL + 2 - (tCWL - 1));
							reg_100228 |= 0x20200000;
						} else {
							reg_100230 = (tUNK_13 << 8  | tUNK_13);

							reg_100234 = (tRFC << 24 | tRCD);
							if(tUNK_10 > tUNK_11) {
								reg_100234 += tUNK_10 << 16;
							} else {
								reg_100234 += tUNK_11 << 16;
							}

							if(bios->power.bit->version == 1) {
								reg_100224 |= (tCL + 2 - (tCWL - 1));
								reg_10022c = (0x14 + tCL) << 24 |
											0x16 << 16 |
											(tCL - 1) << 8 |
											(tCL - 1);
								reg_100234 |= (tCL + 2) << 8;
								reg_100238 = (0x33 - tCWL) << 16 |
											tCWL << 8 |
											(0x2E + tCL - tCWL);
								reg_10023c |= 0x4000202 | (tCL - 1) << 16;
							} else {
								/* See d.bul in NV98..
								 * seems to have changed for G105M+
								 * 10023c seen as 06xxxxxx, 0bxxxxxx or 0fxxxxxx */
								reg_100224 |= (5 + tCL - tCWL);
								reg_10022c = (tCL - 1);
								reg_100230 |= tUNK_20 << 24 | tUNK_21 << 16;
								reg_100234 |= (tCWL + 6) << 8;
								reg_100238 = (0x5A + tCL) << 16 |
											(6 - tCL + tCWL) << 8 |
											(0x50 + tCL - tCWL);
								reg_10023c = 0x202;
							}
						}

						printf("Registers: 220: 0x%08x 0x%08x 0x%08x 0x%08x\n",
						reg_100220, reg_100224,
						reg_100228, reg_10022c);
						printf("           230: 0x%08x 0x%08x 0x%08x 0x%08x\n",
						reg_100230, reg_100234,
						reg_100238, reg_10023c);
					} else {
						reg_100220 = (tRCD << 24 | (tRAS&0x7f) << 17 | tRFC << 8 | tRC);
						reg_100224 = 0x4c << 24 | (tUNK_11&0x0f) << 20 | (tCWL << 7) | (tCL & 0x0f);
						reg_100228 = 0x44000011 | tWR << 16 | tWTR << 8;
						reg_10022c = tUNK_20 << 9 | tUNK_13;
						reg_100230 = 0x42e00069 | tUNK_12 << 15;

						printf("Registers: 290: 0x%08x 0x%08x 0x%08x 0x%08x\n",
						reg_100220, reg_100224,
						reg_100228, reg_10022c);
						printf("           2a0: 0x%08x 0x%08x 0x%08x 0x%08x\n",
						reg_100230, reg_100234,
						reg_100238, reg_10023c);
					}
				}
				printf("\n");

				start += entry_length;
			}
		} else if(version == 0x20) {
			for (i = 0; i < entry_count; i++) {
				printcmd(start, entry_length); printf("\n");
				bios_data = (uint32_t *)&bios->data[start];

				reg_100220 = bios_data[0];
				reg_100224 = bios_data[1];
				reg_100228 = bios_data[2];
				reg_10022c = bios_data[3];
				reg_100230 = bios_data[4];
				reg_100234 = bios_data[5];
				reg_100238 = bios_data[6];
				reg_10023c = bios_data[7];

				printf("Registers: 290: %08x %08x %08x %08x\n",
				reg_100220, reg_100224,
				reg_100228, reg_10022c);
				printf("           2a0: %08x %08x %08x %08x\n",
				reg_100230, reg_100234,
				reg_100238, reg_10023c);

				printf("\n");

				start += entry_length;
			}
		}
	}

	if(bios->power.timing_map.offset && (printmask & ENVY_BIOS_PRINT_PERF)) {
		/* Mapping timings to clockspeeds since 2009 */
		uint8_t 	version = 0, entry_count = 0, entry_length = 0,
				xinfo_count = 0, xinfo_length = 0,
				header_length = 0;
		uint16_t start = bios->power.timing_map.offset;
		uint16_t clock_low = 0, clock_hi = 0;
		uint16_t entry;
		int j;
		uint8_t ram_cfg = strap?(strap & 0x1c) >> 2:0xff;

		version = bios->data[start];
		if (version == 0x10 || version == 0x11) {
			header_length = bios->data[start+1];
			entry_count = bios->data[start+5];
			entry_length = bios->data[start+2];
			xinfo_count = bios->data[start+4];
			xinfo_length = bios->data[start+3];
		}
		printf ("Timing mapping table at 0x%x. Version %d.\n", start, version);
		printf("Header:\n");
		printcmd(start, header_length>0?header_length:10);
		printf ("\n\n");

		start += header_length;

		for(i = 0; i < entry_count; i++) {
			/* XXX: NVEx: frequency x2? */
			clock_low = le16(start);
			clock_hi = le16(start+2);

			printf("Entry %d: %d MHz - %d MHz\n",i, clock_low, clock_hi);
			entry = start+entry_length+(ram_cfg*xinfo_length);
			if(version == 0x10)
				parse_memtm_mapping_entry_10(entry, xinfo_length, start);
			else if(version == 0x11)
				parse_memtm_mapping_entry_11(entry, xinfo_length, start);

			printcmd(start, entry_length>0?entry_length:10);
			start += entry_length;
			for(j = 0; j < xinfo_count; j++) {
				printf("\n");
				if(ram_cfg == j)
					printf("	*%i:",j);
				else
					printf("	 %i:", j);
				printcmd(start+(xinfo_length * j), xinfo_length);
			}
			printf ("\n");

			start += (xinfo_length * xinfo_count);
		}
		printf("\n");
	}

	if(bios->power.unk.offset && (printmask & ENVY_BIOS_PRINT_PERF)) {
		uint8_t 	version = 0, entry_count = 0, entry_length = 0,
				header_length = 0;
		uint16_t start = bios->power.unk.offset;

		version = bios->data[start];

		if (version == 0x10) {
			header_length = bios->data[start+1];
			entry_count = bios->data[start+3];
			entry_length = bios->data[start+2];
		}

		printf ("Unknown PM table at 0x%x. Version %d.\n", start, version);
		printcmd(start, header_length>0?header_length:10);
		printf("\n\n");

		start += header_length;
		for(i = 0; i < entry_count; i++) {
			printcmd(start, entry_length>0?entry_length:10);
			printf("\n");
			start += entry_length;
		}
	}
	return 0;
}
