#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

uint8_t bios[0x10000];
uint32_t len;
const uint8_t bit_signature[] = { 0xff, 0xb8, 'B', 'I', 'T' };
const uint8_t bmp_signature[] = { 0xff, 0x7f, 'N', 'V', 0x0 };

#define RNN_ADDARRAY(a, e) \
	do { \
	if ((a ## num) >= (a ## max)) { \
		if (!(a ## max)) \
			(a ## max) = 16; \
		else \
			(a ## max) *= 2; \
		(a) = realloc((a), (a ## max)*sizeof(*(a))); \
	} \
	(a)[(a ## num)++] = (e); \
	} while(0)

uint32_t bmpoffset = 0;
uint32_t bitoffset = 0;
uint32_t dcboffset = 0;
uint8_t dcbver, dcbhlen, dcbrlen, dcbentries;

uint16_t init_script_tbl_ptr;
uint16_t macro_index_tbl_ptr;
uint16_t macro_tbl_ptr;
uint16_t condition_tbl_ptr;
uint16_t io_condition_tbl_ptr;
uint16_t io_flag_condition_tbl_ptr;
uint16_t init_function_tbl_ptr;
uint16_t some_script_ptr;
uint16_t init96_tbl_ptr;

uint8_t ram_restrict_group_count;
uint16_t ram_restrict_tbl_ptr;

uint16_t *subs = 0;
int subsnum = 0, subsmax = 0;

uint16_t *calls = 0;
int callsnum = 0, callsmax = 0;

static bool nv_cksum(const uint8_t *data, unsigned int length)
{
	/*
	 * There's a few checksums in the BIOS, so here's a generic checking
	 * function.
	 */
	int i;
	uint8_t sum = 0;

	for (i = 0; i < length; i++)
		sum += data[i];

	if (sum)
		return true;

	return false;
}

static uint16_t findstr(uint8_t *data, int n, const uint8_t *str, int len)
{
	int i, j;

	for (i = 0; i <= (n - len); i++) {
		for (j = 0; j < len; j++)
			if (data[i + j] != str[j])
				break;
		if (j == len)
			return i;
	}

	return 0;
}

void printhex (uint32_t start, uint32_t length) {
	while (length) {
		uint32_t i, len = length;
		if (len > 16) len = 16;
		printf ("%04x:", start);
		for (i = 0; i < len; i++)
			printf(" %02x", bios[start+i]);
		printf("\n");
		start += len;
		length -= len;
	}
}

void printcmd (uint16_t off, uint16_t length) {
	printf ("%04x:", off);
	int i;
	for (i = 0; i < length; i++)
		printf(" %02x", bios[off+i]);
	for (; i < 16; i++)
		printf ("   ");
}

uint32_t le32 (uint16_t off) {
	return bios[off] | bios[off+1] << 8 | bios[off+2] << 16 | bios[off+3] << 24;
}

uint16_t le16 (uint16_t off) {
	return bios[off] | bios[off+1] << 8;
}

void printscript (uint16_t soff) {
	while (1) {
		uint8_t op = bios[soff];
		uint32_t dst;
		uint8_t incr;
		uint8_t cnt;
		int i;
		uint32_t x;
		switch (op) {
			case 0x32:
				printcmd (soff, 11);
				printf ("IO_RESTRICT_PROG\tR[0x%06x] = (0x%04x[0x%02x] & 0x%02x) >> %d) [{\n", le32(soff+7), le16(soff+1), bios[soff+3], bios[soff+4], bios[soff+5]);
				cnt = bios[soff+6];
				soff += 11;
				while (cnt--) {
					printcmd (soff, 4);
					printf ("\t%08x\n", le32(soff));
					soff += 4;
				}
				printcmd (soff, 0);
				printf ("}]\n");
				break;
			case 0x33:
				printcmd (soff, 2);
				printf ("REPEAT\t0x%02x\n", bios[soff+1]);
				soff += 2;
				break;
			case 0x36:
				printcmd (soff, 1);
				printf ("END_REPEAT\n");
				soff++;
				break;
			case 0x37:
				printcmd (soff, 11);
				printf ("COPY\t0x%04x[0x%02x] & ~0x%02x |= (R[%06x] %s 0x%02x) & %08x\n",
						le16(soff+7), bios[soff+9], bios[soff+10], le32(soff+1), bios[soff+5]&0x80?"<<":">>",
						bios[soff+5]&0x80?0x100-bios[soff+5]:bios[soff+5], bios[soff+6]);
				soff += 11;
				break;
			case 0x38:
				printcmd (soff, 1);
				printf ("NOT\n");
				soff++;
				break;
			case 0x51:
				printcmd (soff, 5);
				cnt = bios[soff+4];
				dst = bios[soff+3];
				printf ("CR_INDEX_ADDR C[%02x] C[%02x] %02x %02x\n", bios[soff+1], bios[soff+2], bios[soff+3], bios[soff+4]);
				soff += 5;
				while (cnt--) {
					printcmd(soff, 1);
					printf ("\t\t[%02x] = %02x\n", dst, bios[soff]);
					soff++;
					dst++;
				}
				break;
			case 0x52:
				printcmd (soff, 4);
				printf ("CR\t\tC[0x%02x] &= 0x%02x |= 0x%02x\n", bios[soff+1], bios[soff+2], bios[soff+3]);
				soff += 4;
				break;
			case 0x53:
				printcmd (soff, 3);
				printf ("ZM_CR\tC[0x%02x] = 0x%02x\n", bios[soff+1], bios[soff+2]);
				soff += 3;
				break;
			case 0x56:
				printcmd (soff, 3);
				printf ("CONDITION_TIME\t0x%02x 0x%02x\n", bios[soff+1], bios[soff+2]);
				soff += 3;
				break;
			case 0x58:
				printcmd (soff, 6);
				dst = le32(soff+1);
				cnt = bios[soff+5];
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
					RNN_ADDARRAY(calls, x);
				break;
			case 0x5f:
				printcmd (soff, 16);
				printf ("\n");
				printcmd (soff+16, 6);
				printf ("COPY_NV_REG\tR[%06x] & ~0x%08x = (R[%06x] %s 0x%02x) & %08x ^ %08x\n",
						le32(soff+14), le32(soff+18), le32(soff+1), bios[soff+5]&0x80?"<<":">>",
						bios[soff+5]&0x80?0x100-bios[soff+5]:bios[soff+5], le32(soff+6), le32(soff+10));
				soff += 22;
				break;
			case 0x63:
				printcmd (soff, 1);
				printf ("COMPUTE_MEM\n");
				soff++;
				break;
			case 0x69:
				printcmd (soff, 5);
				printf ("IO\t\tI[0x%04x] &= 0x%02x |= 0x%02x\n", le16(soff+1), bios[soff+3], bios[soff+4]);
				soff += 5;
				break;
			case 0x6b:
				printcmd (soff, 2);
				printf ("SUB\t0x%02x\n", bios[soff+1]);
				x = bios[soff+1];
				soff += 2;
				for (i = 0; i < subsnum; i++)
					if (subs[i] == x)
						break;
				if (i == subsnum)
					RNN_ADDARRAY(subs, x);
				break;
			case 0x6e:
				printcmd (soff, 13);
				printf ("NV_REG\tR[0x%06x] &= 0x%08x |= 0x%08x\n", le32(soff+1), le32(soff+5), le32(soff+9));
				soff += 13;
				break;
			case 0x6f:
				printcmd (soff, 2);
				printf ("MACRO\t0x%02x\n", bios[soff+1]);
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
				printf ("CONDITION\t0x%02x\n", bios[soff+1]);
				soff += 2;
				break;
			case 0x7a:
				printcmd (soff, 9);
				printf ("ZM_REG\tR[0x%06x] = 0x%08x\n", le32(soff+1), le32(soff+5));
				soff += 9;
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
				printf ("RAM_RESTRICT_ZM_REG_GROUP\tR[0x%06x] %02x %02x\n", le32(soff+1), bios[soff+5], bios[soff+6]);
				incr = bios[soff+5];
				cnt = bios[soff+6];
				dst = le32(soff+1);
				soff += 7;
				while (cnt--) {
					printcmd (soff, 0);
					printf ("\tR[0x%06x] = {\n", dst);
					for (i = 0; i < ram_restrict_group_count; i++) {
						printcmd (soff, 4);
						printf ("\t\t%08x\n", le32(soff));
						soff += 4;
					}
					printcmd (soff, 0);
					printf ("\t}\n");
					dst += incr;
				}
				break;
			case 0x90:
				printcmd (soff, 9);
				printf ("COPY_ZM_REG\tR[0x%06x] = R[0x%06x]\n", le32(soff+1), le32(soff+5));
				soff += 9;
				break;
			case 0x97:
				printcmd (soff, 13);
				printf ("ZM_MASK_ADD\tR[0x%06x] & ~0x%08x += 0x%08x\n", le32(soff+1), le32(soff+5), le32(soff+9));
				soff += 13;
				break;
			default:
				printcmd (soff, 1);
				printf ("???\n");
				soff++;
				break;
		}
	}
}

int main(int argc, char **argv) {
	FILE *biosfile = fopen(argv[1], "r");
	uint32_t filelen = fread(bios, 1, 0x10000, biosfile);
	fclose(biosfile);
	if (filelen < 512 || bios[0] != 0x55 || bios[1] != 0xaa) {
		printf ("No signature found [%02x %02x]\n", bios[0], bios[1]);
		return 1;
	}
	if (filelen < bios[2]*512) {
		printf ("Wrong length [%x %x]\n", filelen, bios[2]*512);
		return 1;
	}
	len = bios[2]*512;
	printf ("BIOS length %x...\n", len);
	printf ("Checksum %s\n", nv_cksum(bios, len)?"FAIL":"ok");
	bmpoffset = findstr(bios, len, bmp_signature, 5);
	bitoffset = findstr(bios, len, bit_signature, 5);
	dcboffset = le16(0x36);
	printf ("\n");
	if (bmpoffset) {
		printf ("BMP at %x\n", bmpoffset);
		printf ("\n");
	}
	if (bitoffset) {
		int maxentry = bios[bitoffset+10];
		printf ("BIT at %x, %x entries\n", bitoffset, maxentry);
		printf ("\n");
		int i;
		printhex(bitoffset, 12);
		for (i = 0; i < maxentry; i++) {
			uint32_t off = bitoffset + 12 + i*6;
			uint16_t elen = le16(off+2);
			uint16_t eoff = le16(off+4);
			printf ("BIT table '%c' version %d at %04x length %04x\n", bios[off], bios[off+1], eoff, elen);
				printhex(eoff, elen);
			switch (bios[off]) {
				case 'I':
					init_script_tbl_ptr = le32(eoff);
					macro_index_tbl_ptr = le32(eoff+2);
					macro_tbl_ptr = le32(eoff+4);
					condition_tbl_ptr = le32(eoff+6);
					io_condition_tbl_ptr = le32(eoff+8);
					io_flag_condition_tbl_ptr = le32(eoff+10);
					init_function_tbl_ptr = le32(eoff+12);
					if (elen >= 16)
						some_script_ptr = le32(eoff+14);
					if (elen >= 18)
						init96_tbl_ptr = le32(eoff+16);
					break;
				case 'M':
					if (bios[off+1] == 1 && elen >= 5) {
						ram_restrict_group_count = bios[eoff+2];
						ram_restrict_tbl_ptr = le16(eoff+3);
					} else if (bios[off+1] == 2 && elen >= 3) {
						ram_restrict_group_count = bios[eoff];
						ram_restrict_tbl_ptr = le16(eoff+1);
					}
					break;
			}
			printf ("\n");
		}
	}
	if (dcboffset) {
		dcbver = bios[dcboffset];
		printf ("DCB %d.%d at %x\n", dcbver >> 4, dcbver&0xf, dcboffset);
		if (dcbver >= 0x30) {
			dcbhlen = bios[dcboffset+1];
			dcbentries = bios[dcboffset+2];
			dcbrlen = bios[dcboffset+3];
		} else if (dcbver >= 0x20) {
			dcbhlen = 8;
			dcbentries = 16;
			dcbrlen = 8;
		} else {
			dcbhlen = 4;
			dcbentries = 16;
			dcbrlen = 10;
		}
		printhex(dcboffset, dcbhlen);
		int i;
		for (i = 0; i < dcbentries; i++)
			printhex (dcboffset + dcbhlen + dcbrlen * i, dcbrlen);
		printf ("\n");
	}
	
	if (init_script_tbl_ptr) {
		int i = 0;
		uint16_t off = init_script_tbl_ptr;
		uint16_t soff;
		while ((soff = le16(off))) {
			off += 2;
			RNN_ADDARRAY(subs, i);
			i++;
		}
		printf ("Init script table at %x: %d main scripts\n\n", init_script_tbl_ptr, i);
	}
	
	int subspos = 0, callspos = 0;
	while (subspos < subsnum || callspos < callsnum) {
		if (callspos < callsnum) {
			uint16_t soff = calls[callspos++];
			printf ("Subroutine at %x:\n", soff);
			printscript(soff);
			printf("\n");
		} else if (subspos < subsnum) {
			int i = subs[subspos++];
			uint16_t soff = le32(init_script_tbl_ptr + 2*i);
			printf ("Init script %d at %x:\n", i, soff);
			printscript(soff);
			printf("\n");
		}
	}
	
	return 0;
}
