#include "util.h"
#include "dis.h"
#include "bios.h"
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

struct envy_bios *bios;
uint8_t major_version, minor_version, micro_version, chip_version;
uint32_t card_codename = 0;
uint8_t magic_number = 0;
uint32_t strap = 0;

uint16_t bmpver = 0;
uint8_t bmpver_min;
uint8_t bmpver_maj;
uint32_t i2coffset = 0;
uint32_t gpiooffset = 0;
uint8_t dcbver, dcbhlen, dcbrlen, dcbentries;
uint8_t i2cver, i2chlen, i2crlen, i2centries, i2cd0, i2cd1;
uint8_t gpiover, gpiohlen, gpiorlen, gpioentries;
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

uint16_t pll_limit_tbl_ptr;

uint8_t p_tbls_ver = 0;
uint16_t pm_mode_tbl_ptr = 0;
uint16_t voltage_map_tbl_ptr = 0;
uint16_t voltage_tbl_ptr = 0;
uint16_t temperature_tbl_ptr = 0;
uint16_t timings_tbl_ptr = 0;
uint16_t timings_map_tbl_ptr = 0;
uint16_t pm_unknown_tbl_ptr = 0;

uint16_t *subs = 0;
int subsnum = 0, subsmax = 0;

uint16_t *calls = 0;
int callsnum = 0, callsmax = 0;

int usage(char* name) {
	printf("Usage: %s mybios.rom [-c XX] [-s strap]\n",name);
	printf("Options:\n");
	printf("  -c XX : override card generation\n");
	printf("  -m XX : override \"magic number\" for memtimings\n");
	printf("  -s XX : set the trap register\n");
	return 1;
}

void printhex (uint32_t start, uint32_t length) {
	envy_bios_dump_hex(bios, stdout, start, length);
}

void printcmd (uint16_t off, uint16_t length) {
	printf ("%04x:", off);
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
					printf ("\t%08x\n", le32(soff));
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
			case 0x36:
				printcmd (soff, 1);
				printf ("END_REPEAT\n");
				soff++;
				break;
			case 0x37:
				printcmd (soff, 11);
				printf ("COPY\t0x%04x[0x%02x] & ~0x%02x |= (R[%06x] %s 0x%02x) & %08x\n",
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
			case 0x4a:
				printcmd (soff, 11);
				printf ("IO_RESTRICT_PLL2\tR[0x%06x] =PLL= (0x%04x[0x%02x] & 0x%02x) >> %d) [{\n", le32(soff+7), le16(soff+1), bios->data[soff+3], bios->data[soff+4], bios->data[soff+5]);
				cnt = bios->data[soff+6];
				soff += 11;
				while (cnt--) {
					printcmd (soff, 4);
					printf ("\t%08x\n", le32(soff));
					soff += 4;
				}
				printcmd (soff, 0);
				printf ("}]\n");
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
			case 0x51:
				printcmd (soff, 5);
				cnt = bios->data[soff+4];
				dst = bios->data[soff+3];
				printf ("CR_INDEX_ADDR C[%02x] C[%02x] %02x %02x\n", bios->data[soff+1], bios->data[soff+2], bios->data[soff+3], bios->data[soff+4]);
				soff += 5;
				while (cnt--) {
					printcmd(soff, 1);
					printf ("\t\t[%02x] = %02x\n", dst, bios->data[soff]);
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
			case 0x5e:
				printcmd (soff, 6);
				printf ("I2C_IF\tI2C[0x%02x][0x%02x][0x%02x] & 0x%02x == 0x%02x\n", bios->data[soff+1], bios->data[soff+2], bios->data[soff+3], bios->data[soff+4], bios->data[soff+5]);
				soff += 6;
				break;
			case 0x5f:
				printcmd (soff, 16);
				printf ("\n");
				printcmd (soff+16, 6);
				printf ("COPY_NV_REG\tR[%06x] & ~0x%08x = (R[%06x] %s 0x%02x) & %08x ^ %08x\n",
						le32(soff+14), le32(soff+18), le32(soff+1), bios->data[soff+5]&0x80?"<<":">>",
						bios->data[soff+5]&0x80?0x100-bios->data[soff+5]:bios->data[soff+5], le32(soff+6), le32(soff+10));
				soff += 22;
				break;
			case 0x63:
				printcmd (soff, 1);
				printf ("COMPUTE_MEM\n");
				soff++;
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
				printf ("RAM_RESTRICT_ZM_REG_GROUP\tR[0x%06x] %02x %02x\n", le32(soff+1), bios->data[soff+5], bios->data[soff+6]);
				incr = bios->data[soff+5];
				cnt = bios->data[soff+6];
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
				printf ("COPY_ZM_REG\tR[0x%06x] = R[0x%06x]\n", le32(soff+5), le32(soff+1));
				soff += 9;
				break;
			case 0x97:
				printcmd (soff, 13);
				printf ("ZM_MASK_ADD\tR[0x%06x] & ~0x%08x += 0x%08x\n", le32(soff+1), le32(soff+5), le32(soff+9));
				soff += 13;
				break;
			case 0x9a:
				printcmd (soff, 7);
				printf ("I2C_IF_LONG\tI2C[0x%02x][0x%02x][0x%02x:0x%02x] & 0x%02x == 0x%02x\n", bios->data[soff+1], bios->data[soff+2], bios->data[soff+4], bios->data[soff+3], bios->data[soff+5], bios->data[soff+6]);
				soff += 7;
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

	major_version = bios->data[offset + 3];
	chip_version = bios->data[offset + 2];
	if(card_codename <= 0)
		card_codename = chip_version;
	minor_version = bios->data[offset + 1];
	micro_version = bios->data[offset + 0];
	printf("Bios version %02x.%02x.%02x.%02x\n\n",
		 bios->data[offset + 3], bios->data[offset + 2],
		 bios->data[offset + 1], bios->data[offset]);
	printf("Card codename %02x\n\n",
		 card_codename);
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

void find_strap(int argc, char **argv) {
	char *path;
	const char * strap_filename = "strap_peek";
	const char *pos = strrchr(argv[1], '/');
	if (pos == NULL)
		pos = argv[1];
	else
		pos++;
	int base_length = pos-argv[1];

	path = (char*) malloc(base_length + strlen(strap_filename)+1);
	strncpy(path, argv[1], base_length);
	strncpy(path+base_length, strap_filename, strlen(strap_filename));

	if(!set_strap_from_file(path))
		printf("Strap register found in '%s'\n", path);

	free(path);
}

int parse_args(int argc, char **argv) {
	int i;

	for(i = 2; i < argc; i++) {
		if (!strncmp(argv[i],"-c",2)) {
			i++;
			if(i < argc) {
				sscanf(argv[i],"%2x",&card_codename);
				printf("Card generation forced to nv%2x\n", card_codename);
			} else {
				return usage(argv[0]);
			}
		} else if (!strncmp(argv[i],"-m",2)) {
			i++;
			if(i < argc) {
				sscanf(argv[i],"%2hhx",&magic_number);
				printf("Magic number forced to %2hhx\n", magic_number);
			} else {
				return usage(argv[0]);
			}
		} else if (!strncmp(argv[i],"-s",2)) {
			i++;
			if (i < argc) {
				if (!strncmp(argv[i],"0x",2)) {
					set_strap_from_string(argv[i]+2);
				} else {
					set_strap_from_file(argv[i]);
				}
			} else {
				return usage(argv[0]);
			}
		} else {
			fprintf(stderr, "Unknown parameter '%s'\n", argv[i]);
		}
	}

	return 0;
}

int vbios_read(const char *filename, uint8_t **vbios, unsigned int *length)
{
	FILE * fd_bios;
	int i;

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
}

int main(int argc, char **argv) {
	int i;
	if (argc < 2) {
		return usage(argv[0]);
	}

	bios = calloc (sizeof *bios, 1);
	if (!bios) {
		perror("malloc");
		return 1;
	}

	parse_args(argc, argv);

	if (strap == 0) {
		find_strap(argc, argv);
		if (strap == 0)
			fprintf(stderr, "warning: No strap specified\n");
	}

	if (vbios_read(argv[1], &bios->data, &bios->origlength)) {
		fprintf(stderr, "Failed to read VBIOS!\n");
		return 1;
	}

	if (envy_bios_parse(bios)) {
		fprintf(stderr, "Failed to parse BIOS\n");
		return 1;
	}
	envy_bios_print(bios, stdout, ENVY_BIOS_PRINT_ALL);

	const struct ed2a_colors *discolors = &ed2a_def_colors;

	if (bios->bmp_offset && bios->type == ENVY_BIOS_TYPE_NV04) {
		bmpver_maj = bios->data[bios->bmp_offset+5];
		bmpver_min = bios->data[bios->bmp_offset+6];
		bmpver = bmpver_maj << 8 | bmpver_min;
		printf ("BMP %02x.%02x at %x\n", bmpver_maj, bmpver_min, bios->bmp_offset);
		printf ("\n");
		parse_bios_version(bios->bmp_offset + 10);
		printhex(bios->bmp_offset, 256);
		if (bmpver >= 0x527) {
			pm_mode_tbl_ptr = le16(bios->bmp_offset + 148);
			voltage_tbl_ptr = le16(bios->bmp_offset + 152);
		}
	}
	if (bios->bit_offset) {
		int maxentry = bios->data[bios->bit_offset+10];
		printf ("BIT at %x, %x entries\n", bios->bit_offset, maxentry);
		printf ("\n");
		printhex(bios->bit_offset, 12);
		for (i = 0; i < maxentry; i++) {
			uint32_t off = bios->bit_offset + 12 + i*6;
			uint8_t version = bios->data[off+1];
			uint16_t elen = le16(off+2);
			uint16_t eoff = le16(off+4);
			printf ("BIT table '%c' version %d at %04x length %04x\n", bios->data[off], version, eoff, elen);
				printhex(eoff, elen);
			switch (bios->data[off]) {
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
					if (bios->data[off+1] == 1 && elen >= 5) {
						ram_restrict_group_count = bios->data[eoff+2];
						ram_restrict_tbl_ptr = le16(eoff+3);
					} else if (bios->data[off+1] == 2 && elen >= 3) {
						ram_restrict_group_count = bios->data[eoff];
						ram_restrict_tbl_ptr = le16(eoff+1);
					}
					break;
				case 'C':
					pll_limit_tbl_ptr = le16(eoff + 8);
					break;
				case 'U':
					disp_script_tbl_ptr = le16(eoff);
					break;
				case 'P':
					printf("Bit P table version %x\n", version);

					p_tbls_ver = version;
					pm_mode_tbl_ptr = le16(eoff + 0);
					if (version == 1) {
						voltage_tbl_ptr = le16(eoff + 16);
						temperature_tbl_ptr = le16(eoff + 12);
						timings_tbl_ptr = le16(eoff + 4);
						pm_unknown_tbl_ptr = le16(eoff + 21);
					} else if (version == 2) {
						voltage_tbl_ptr = le16(eoff + 12);
						temperature_tbl_ptr = le16(eoff + 16);
						timings_tbl_ptr = le16(eoff + 8);
						timings_map_tbl_ptr = le16(eoff + 4);
						pm_unknown_tbl_ptr = le16(eoff + 24);
						if (elen >= 34)
							voltage_map_tbl_ptr = le16(eoff + 32);
					}

					break;
				case 'i':
					parse_bios_version(eoff);
					break;
			}
			printf ("\n");
		}
	}
	if (bios->dcb_offset) {
		dcbver = bios->data[bios->dcb_offset];
		uint16_t coff = 4;
		printf ("DCB %d.%d at %x\n", dcbver >> 4, dcbver&0xf, bios->dcb_offset);
		if (dcbver >= 0x30) {
			dcbhlen = bios->data[bios->dcb_offset+1];
			dcbentries = bios->data[bios->dcb_offset+2];
			dcbrlen = bios->data[bios->dcb_offset+3];
			i2coffset = le16(bios->dcb_offset+4);
			gpiooffset = le16(bios->dcb_offset+10);
		} else if (dcbver >= 0x20) {
			dcbhlen = 8;
			dcbentries = 16;
			dcbrlen = 8;
			i2coffset = le16(bios->dcb_offset+2);
		} else {
			dcbhlen = 4;
			dcbentries = 16;
			dcbrlen = 10;
			coff = 6;
			i2coffset = le16(bios->dcb_offset+2);
		}
		printhex(bios->dcb_offset, dcbhlen);
		printf("\n");
		for (i = 0; i < dcbentries; i++) {
			uint16_t soff = bios->dcb_offset + dcbhlen + dcbrlen * i;
			uint32_t conn = le32(soff);
			uint32_t conf = le32(soff + coff);
			if (dcbver >= 0x20) {
				int type = conn & 0xf;
				int i2c = conn >> 4 & 0xf;
				int heads = conn >> 8 & 0xf;
				int connector = conn >> 12 & 0xf;
				int bus = conn >> 16 & 0xf;
				int loc = conn >> 20 & 3;
				int or = conn >> 24 & 0xf;
				char *types[] = { "ANALOG", "TV", "TMDS", "LVDS", "???", "???", "DP", "???",
							"???", "???", "???", "???", "???", "???", "???", "???" };
				printf ("Type %x [%s] I2C %d heads %x connector %d bus %d loc %d or %x conf %x\n", type, types[type], i2c, heads, connector, bus, loc, or, conf);
			}
			printhex (soff, dcbrlen);
			printf("\n");
		}
		printf ("\n");
	}
	if (bios->hwsq_offset) {
		uint8_t entry_count, bytes_to_write, i, e;
		const struct disisa *hwsq_isa = ed_getisa("hwsq");
		struct ed2v_variant *hwsq_var_nv17 = ed2v_new_variant(hwsq_isa->ed2, "nv17");
		struct ed2v_variant *hwsq_var_nv41 = ed2v_new_variant(hwsq_isa->ed2, "nv41");

		bios->hwsq_offset += 4;

		entry_count = bios->data[bios->hwsq_offset];
		bytes_to_write = bios->data[bios->hwsq_offset + 1];

		printf("HWSQ table at %x: Entry count %u, entry length %u\n", bios->hwsq_offset, entry_count, bytes_to_write);

		for (i=0; i < entry_count; i++) {
			uint16_t entry_offset = bios->hwsq_offset + 2 + i * bytes_to_write;
			uint32_t sequencer = le32(entry_offset);
			//uint8_t bytes_written = 4;

			printf("-- HWSQ entry %u at %x: sequencer control = %u\n", i, entry_offset, sequencer);
			envydis(hwsq_isa, stdout, bios->data + entry_offset + 4, 0, bytes_to_write - 4, hwsq_var_nv41, -1, 0, 0, 0, discolors);
			printf ("\n");
		}
		printf ("\n");
	}
	if (i2coffset) {
		i2chlen = 0;
		i2centries = 16;
		i2crlen = 4;
		if (dcbver >= 0x30) {
			i2cver = bios->data[i2coffset];
			i2chlen = bios->data[i2coffset+1];
			i2centries = bios->data[i2coffset+2];
			i2crlen = bios->data[i2coffset+3];
			i2cd0 = bios->data[i2coffset+4] & 0xf;
			i2cd1 = bios->data[i2coffset+4] >> 4;
			printf ("I2C table %d.%d at %x, default entries %x %x\n", i2cver >> 4, i2cver&0xf, i2coffset, i2cd0, i2cd1);
			printhex(i2coffset, i2chlen);
		} else {
			i2cver = dcbver;
			printf ("I2C table at %x:\n", i2coffset);
		}
		printf ("\n");
		for (i = 0; i < i2centries; i++) {
			uint16_t off = i2coffset + i2chlen + i2crlen * i;
			printf ("Entry %x: ", i);
			if (bios->data[off+3] == 0xff)
				printf ("invalid\n");
			else {
				uint8_t pt = 0;
				uint8_t rd, wr;
				if (i2cver >= 0x30)
					pt = bios->data[off+3];
				if (i2cver >= 0x14) {
					wr = bios->data[off];
					rd = bios->data[off+1];
				} else {
					wr = bios->data[off+3];
					rd = bios->data[off+2];
				}
				switch (pt) {
					case 0:
						printf ("NV04-style wr %x rd %x\n", wr, rd);
						break;
					case 4:
						printf ("NV4E-style %x\n", rd);
						break;
					case 5:
						printf ("NV50-style %x\n", wr);
						break;
					default:
						printf ("unknown type %x\n", pt);
						break;
				}
			}
			printhex (off, i2crlen);
			printf ("\n");
		}
		printf ("\n");
	}

	if (gpiooffset) {
		gpiover = bios->data[gpiooffset];
		gpiohlen = bios->data[gpiooffset+1];
		gpioentries = bios->data[gpiooffset+2];
		gpiorlen = bios->data[gpiooffset+3];
		printf ("GPIO table %d.%d at %x\n", gpiover >> 4, gpiover&0xf, gpiooffset);
		printhex(gpiooffset, gpiohlen);

		uint16_t soff = gpiooffset + gpiohlen;
		for (i = 0; i < gpioentries; i++) {
			if (dcbver < 0x40) {
				uint16_t entry = le16(soff);
				if (((entry & 0x07e0) >> 5) == 0x3f)
					printf("-- invalid entry --\n");
				else
					printf("-- entry %04x, tag %02x, line %02x, invert %02x --\n", entry,
						(entry & 0x07e0) >> 5, (entry & 0x001f), ((entry & 0xf800) >> 11) != 4);
				printhex(soff, gpiorlen);
			} else {
				uint32_t entry = le32(soff);
				if (((entry & 0x0000ff00) >> 8) == 0xff)
					printf("-- invalid entry --\n");
				else
					printf("-- entry %04x, tag %02x, line %02x, state(def) %02x, state(0) %02x, state(1) %02x --\n",
						entry, (entry & 0x0000ff00) >> 8, (entry & 0x0000001f) >> 0, (entry & 0x01000000) >> 24,
						(entry & 0x18000000) >> 27, (entry & 0x60000000) >> 29);
				printhex(soff, gpiorlen);
			}
			soff += gpiorlen;
			printf("\n");
		}
		printf("\n");
	}



	if (pll_limit_tbl_ptr) {
		uint8_t ver = bios->data[pll_limit_tbl_ptr];
		uint8_t hlen = 0, rlen = 0, entries = 0;
		printf ("PLL limits table at %x, version %x\n", pll_limit_tbl_ptr, ver);
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
				printf("-- log2P_max [%d](XXX: must be less than 8), log2P_bias [%d], ref_clk %dkHz --\n",
					bios->data[soff+29], bios->data[soff+30], ref_clk);
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

					printf("-- log2P_max [%d], log2P_bias [%d], ref_clk %dkHz --\n",
						bios->data[rec_ptr+25], bios->data[rec_ptr+27], ref_clk);
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

	if (init_script_tbl_ptr) {
		i = 0;
		uint16_t off = init_script_tbl_ptr;
		uint16_t soff;
		while ((soff = le16(off))) {
			off += 2;
			ADDARRAY(subs, i);
			i++;
		}
		printf ("Init script table at %x: %d main scripts\n\n", init_script_tbl_ptr, i);
	}

	int subspos = 0, callspos = 0;
	int ssdone = 0;
	while (!ssdone || subspos < subsnum || callspos < callsnum) {
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
		} else if (!ssdone) {
			if (some_script_ptr) {
				printf ("Some script at %x:\n", some_script_ptr);
				printscript(some_script_ptr);
				printf("\n");
			}
			ssdone = 1;
		}
	}

	if (disp_script_tbl_ptr) {
		uint8_t ver = bios->data[disp_script_tbl_ptr];
		uint8_t hlen = bios->data[disp_script_tbl_ptr+1];
		uint8_t rlen = bios->data[disp_script_tbl_ptr+2];
		uint8_t entries = bios->data[disp_script_tbl_ptr+3];
		uint8_t rhlen = bios->data[disp_script_tbl_ptr+4];
		printf ("Display script table at %x, version %x:\n", disp_script_tbl_ptr, ver);
		printhex(disp_script_tbl_ptr, hlen + rlen * entries);
		printf ("\n");
		for (i = 0; i < entries; i++) {
			uint16_t table = le16(disp_script_tbl_ptr + hlen + i * rlen);
			if (!table)
				continue;
			uint32_t entry = le32(table);
			uint8_t configs = bios->data[table+5];
			printf ("Subtable %d at %x for %08x:\n", i, table, entry);
			printhex(table, rhlen + configs * 6);
			printf("\n");
		}
		printf("\n");
	}

	if (condition_tbl_ptr) {
		printf ("Condition table at %x: %d conditions:\n", condition_tbl_ptr, maxcond+1);
		for (i = 0; i <= maxcond; i++) {
			printcmd(condition_tbl_ptr + 12 * i, 12);
			printf ("[0x%02x] R[0x%06x] & 0x%08x == 0x%08x\n", i,
					le32(condition_tbl_ptr + 12 * i),
					le32(condition_tbl_ptr + 12 * i + 4),
					le32(condition_tbl_ptr + 12 * i + 8));
		}
		printf("\n");
	}

	if (macro_index_tbl_ptr) {
		printf ("Macro index table at %x: %d macro indices:\n", macro_index_tbl_ptr, maxmi+1);
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

	if (macro_tbl_ptr) {
		printf ("Macro table at %x: %d macros:\n", macro_tbl_ptr, maxmac+1);
		for (i = 0; i <= maxmac; i++) {
			printcmd(macro_tbl_ptr + 8 * i, 8);
			printf ("[0x%02x] R[0x%06x] = 0x%08x\n", i,
					le32(macro_tbl_ptr + 8 * i),
					le32(macro_tbl_ptr + 8 * i + 4));
		}
		printf("\n");
	}

	if (pm_mode_tbl_ptr) {
		uint8_t version = 0, entry_count = 0, entry_length = 0;
		uint8_t mode_info_length = 0, header_length = 0;
		uint8_t extra_data_length = 8, extra_data_count = 0;
		uint8_t subentry_offset = 0, subentry_size = 0, subentry_count = 0;
		uint16_t start = pm_mode_tbl_ptr;
		uint8_t ram_cfg = strap?(strap & 0x1c) >> 2:0xff;
		int e;

		if (major_version == 0x4) {
			header_length = bios->data[start+0];
			version = bios->data[start+1];
			entry_length = bios->data[start+3];
			entry_count = bios->data[start+2];
			mode_info_length = entry_length;
		} else if (major_version < 0x70) {
			version = bios->data[start+0];
			header_length = bios->data[start+1];
			entry_count = bios->data[start+2];
			mode_info_length = bios->data[start+3];
			extra_data_count = bios->data[start+4];
			extra_data_length = bios->data[start+5];
			entry_length = mode_info_length + extra_data_count * extra_data_length;
		} else if (major_version == 0x70) {
			version = bios->data[start+0];
			header_length = bios->data[start+1];
			subentry_offset = bios->data[start+2];
			subentry_size = bios->data[start+3];
			subentry_count = bios->data[start+4];
			entry_length = subentry_offset + subentry_size * subentry_count;
			mode_info_length = subentry_offset;
			entry_count = bios->data[start+5];
		}

		start += header_length;

		printf ("PM_Mode table at %x. Version %x. RamCFG %x. Info_length %i.\n",
			pm_mode_tbl_ptr, version, ram_cfg, mode_info_length
		);

		if (version < 0x40)
			printf("PWM_div 0x%x, ", le16(start+6));


		if (version > 0x15 && version < 0x40)
			printf("Extra_length %i. Extra_count %i.\n", extra_data_length, extra_data_count);
		else if (version == 0x40)
			printf("Subentry length %i. Subentry count %i. Subentry Offset %i\n", subentry_size, subentry_count, subentry_offset);
		else
			printf("Version unknown\n");

		printf("Header:\n");
		printcmd(pm_mode_tbl_ptr, header_length>0?header_length:10);
		printf ("\n");

		printf("%i performance levels\n", entry_count);
		for (i=0; i < entry_count; i++) {
			uint16_t id, fan, voltage;
			uint16_t core, shader = 0, memclk;
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

			/* Old BIOS' might give rounding errors! */
			if (version == 0x12 || version == 0x13 || version == 0x15) {
				id = bios->data[start+0];
				core = le16(start+1) / 100;
				memclk = le16(start+5) / 100;
				fan = bios->data[start+55];
				voltage = bios->data[start+56];
			} else if (version >= 0x21 && version <= 0x24) {
				id = bios->data[start+0];
				core = le16(start+6);
				shader = core + (signed char) bios->data[start+8];
				memclk = le16(start+11);
				fan = bios->data[start+4];
				voltage = bios->data[start+5];
			} else if (version == 0x25) {
				id = bios->data[start+0];
				core = le16(start+6);
				shader = le16(start+10);
				memclk = le16(start+12);
				fan = bios->data[start+4];
				voltage = bios->data[start+5];
			} else if (version == 0x30 || version == 0x35) {
				id = bios->data[start+0];
				fan = bios->data[start+6];
				voltage = bios->data[start+7];
				core = le16(start+8);
				shader = le16(start+10);
				memclk = le16(start+12);
			} else if (version == 0x40) {
#define subent(n) (subentry_offset + ((n) * subentry_size))
				id = bios->data[start+0];
				fan = 0;
				voltage = bios->data[start+2];

				if (subentry_size == 4) {
					shader = (le16(start+subent(3)) & 0xfff);
					core = shader / 2;
					memclk = (le16(start+subent(5)) & 0xfff);
				} else {
					core = (le16(start+subent(0)) & 0xfff);
					shader = (le16(start+subent(1)) & 0xfff);
					memclk = (le16(start+subent(2)) & 0xfff);
				}
			}

			timing_id = 0xff;
			if (version > 0x15 && version < 0x40) {
				uint16_t extra_start = start + mode_info_length;
				uint16_t timing_extra_data = extra_start+(ram_cfg*extra_data_length);

				if (ram_cfg < extra_data_count)
					timing_id = bios->data[timing_extra_data+1];

				printf ("\n-- ID 0x%x Core %dMHz Memory %dMHz Shader %dMHz Voltage %d[*10mV] Timing %d Fan %d PCIe link width %d --\n",
					id, core, memclk, shader, voltage, timing_id, fan, pcie_width );
			} else if (version == 0x40) {
				if (ram_cfg < subentry_count)
					timing_id = bios->data[start+subent(ram_cfg)+1];
				printf ("\n-- ID 0x%x Core %dMHz Memory %dMHz Shader %dMHz Voltage entry %d Timing %d PCIe link width %d --\n",
					id, core, memclk, shader, voltage, timing_id, pcie_width );
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
					printf("\n");
				}
			}
			printf("\n");

			start += entry_length;
		}
		printf("\n");
	}

	if (voltage_map_tbl_ptr) {
		uint8_t version = 0, entry_count = 0, entry_length = 0;
		uint8_t header_length = 0;
		uint16_t start = voltage_map_tbl_ptr;

		version = bios->data[start+0];
		header_length = bios->data[start+1];
		entry_length = bios->data[start+2];
		entry_count = bios->data[start+3];

		printf ("Voltage map table at %x. Version %x.\n", voltage_map_tbl_ptr, version);

		printf("Header:\n");
		printcmd(voltage_map_tbl_ptr, header_length>0?header_length:10);
		printf("\n\n");

		start += header_length;

		for (i=0; i < entry_count; i++) {
			printf ("-- ID = %u: voltage_min = %u, voltage_max = %u [µV] --\n",
				i, le32(start), le32(start+4));
			printcmd(start, entry_length);
			printf("\n\n");

			start += entry_length;
		}

		printf("\n");
	}

	if (voltage_tbl_ptr) {
		uint8_t version = 0, entry_count = 0, entry_length = 0;
		uint8_t header_length = 0, mask = 0;
		uint16_t start = voltage_tbl_ptr;

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
		} else if (version == 0x40) {
			header_length = bios->data[start+1];
			entry_length = bios->data[start+2];
			entry_count = bios->data[start+3];	// XXX: NFI what the entries are for
			mask = bios->data[start+11];			// guess
		}

		printf ("Voltage table at %x. Version %x.\n", voltage_tbl_ptr, version);

		printf("Header:\n");
		printcmd(voltage_tbl_ptr, header_length>0?header_length:10);
		printf (" mask = %x\n\n", mask);

		if (version < 0x40) {
			start += header_length;

			printf ("%i entries\n", entry_count);
			for (i=0; i < entry_count; i++) {
				uint32_t id, label;

				id = bios->data[start+1];
				label = bios->data[start+0] * 10000;

				printf ("-- ID = %x, voltage = %u µV --\n", id, label);
				if (entry_length > 20) {
					printcmd(start, 20); printf("\n");
					printcmd(start + 20, entry_length - 20);
				} else {
					printcmd(start, entry_length);
				}
				printf("\n\n");

				start += entry_length;
			}
		} else {
			/* That's what nouveau does, but it doesn't make much sense... */
			uint32_t volt_uv = le32(start+4);
			int16_t step_uv = le16(start+8);
			uint16_t nr_label = mask + 1; // XXX: hacky solution

			printf("-- Maximum voltage %d µV, voltage step %d µV, Maximum voltage to be used %d µV --\n",
					volt_uv, step_uv, le32(start+14));

			for (i = 0; i < nr_label; i++) {
				printf("-- Vid %d, voltage %d µV --\n", i, volt_uv);
				volt_uv += step_uv;
			}

//			printf ("-- Voltage range = %u-%u µV, step = %u µV--\n",
//				volt_uv, volt_uv + volt_uv * mask, step_uv);
		}
		printf("\n");
	}
	if (temperature_tbl_ptr) {
		uint8_t version = 0, entry_count = 0, entry_length = 0;
		uint8_t header_length = 0;
		uint16_t start = temperature_tbl_ptr;

		version = bios->data[start+0];
		header_length = bios->data[start+1];
		entry_length = bios->data[start+2];
		entry_count = bios->data[start+3];

		start += header_length;

		printf ("Temperature table at %x. Version %x.\n", voltage_tbl_ptr, version);

		printf("Header:\n");
		printcmd(temperature_tbl_ptr, header_length>0?header_length:10);
		printf ("\n\n");

		printf ("%i entries\n", entry_count);
		for (i=0; i < entry_count; i++) {
			uint8_t id = bios->data[(start+i*entry_length)+0];
			uint16_t data = le16((start+i*entry_length)+1);
			uint16_t temp = (data & 0x0ff0) >> 4;
			uint16_t type = (data & 0xf00f);
			const char *type_s = NULL, *threshold = NULL;
			const char *correction_target = NULL;
			uint16_t correction_value = 0;
			uint16_t fan_min, fan_max;

			type_s = (type == 0xa000?"ambient":"core");

			/* Temperatures */
			if (id == 0x4)
				threshold = "critical";
			else if (id == 0x5 || id == 0x7)
				threshold = "throttling";
			else if (id == 0x8)
				threshold = "fan boost";

			/* fan */
			fan_min = data & 0xff;
			fan_max= (data & 0xff00) >> 8;

			correction_value = data;
			if (id == 0x1) {
				correction_target = "diode offset";
				correction_value = (data >> 9) & 0x7f;
			}
			else if (id == 0x10)
				correction_target = "diode multiplier";
			else if (id == 0x11)
				correction_target = "diode divisor";
			else if (id == 0x12)
				correction_target = "slope multiplier";
			else if (id == 0x13)
				correction_target = "slope divisor";

			printcmd(start+i*entry_length, entry_length);
			printf ("id = 0x%x, data = 0x%x ",
						id, data);

			if (id == 0xff)
				printf("--disabled");
			else if (id == 0x0)
				printf ("-- new section type %i", data);
			else if (threshold)
				printf ("-- %s %s temperature is %i°C", threshold, type_s, temp);
			else if (id == 0x1 || (id >= 0x10 && id <= 0x13) || id == 0x22) {
				printf ("-- temp/fan management: ");
				if (correction_target)
					printf("%s = %i", correction_target, correction_value);
				else if (id == 0x22)
					printf("fan_min = %i, fan_max = %i", fan_min, fan_max);
				else
					printf("id = 0x%x, data = 0x%x", id, data);
			} else if (id == 0x24)
				printf ("-- bump fan speed when at %i°C type 0x%x", temp, type);
			else if (id == 0x26)
					printf("-- pwm frequency %i", data);
			else
				printf ("Unknown (temp ?= %i°C, type ?= 0x%x)", temp, type);

			printf("\n");
		}
		printf("\n");
	}
	if (timings_tbl_ptr) {
		uint8_t version = 0, entry_count = 0, entry_length = 0;
		uint8_t header_length = 0;
		uint16_t start = timings_tbl_ptr;
		uint8_t tWR, tUNK_1, tCL;
		uint8_t tRP;		/* Byte 3 */
		uint8_t tRAS;	/* Byte 5 */
		uint8_t tRFC;	/* Byte 7 */
		uint8_t tRC;		/* Byte 9 */
		uint8_t tUNK_10, tUNK_11, tUNK_12, tUNK_13, tUNK_14;
		uint8_t tUNK_18, tUNK_19, tUNK_20, tUNK_21;
		uint32_t reg_100220 = 0, reg_100224 = 0, reg_100228 = 0, reg_10022c = 0;
		uint32_t reg_100230 = 0, reg_100234 = 0, reg_100238 = 0, reg_10023c = 0;

		version = bios->data[start+0];
		if (version == 0x10) {
			header_length = bios->data[start+1];
			entry_count = bios->data[start+2];
			entry_length = bios->data[start+3];
		}

		printf ("Timing table at %x. Version %x.\n", timings_tbl_ptr, version);

		printf("Header:\n");
		printcmd(timings_tbl_ptr, header_length>0?header_length:10);
		printf ("\n\n");

		start += header_length;

		printf ("%i entries\n", entry_count);
		for (i = 0; i < entry_count; i++) {
			tUNK_18 = 1;
			tUNK_19 = 1;
			tUNK_20 = 0;
			tUNK_21 = 0;
			switch (entry_length<22?entry_length:22) {
			case 22:
				tUNK_21 = bios->data[start+21];
			case 21:
				tUNK_20 = bios->data[start+20];
			case 20:
				tUNK_19 = bios->data[start+19];
			case 19:
				tUNK_18 = bios->data[start+18];
			default:
				tWR  = bios->data[start+0];
				tUNK_1  = bios->data[start+1];
				tCL  = bios->data[start+2];
				tRP     = bios->data[start+3];
				tRAS    = bios->data[start+5];
				tRFC    = bios->data[start+7];
				tRC     = bios->data[start+9];
				tUNK_10 = bios->data[start+10];
				tUNK_11 = bios->data[start+11];
				tUNK_12 = bios->data[start+12];
				tUNK_13 = bios->data[start+13];
				tUNK_14 = bios->data[start+14];
				break;
			}

			printcmd(start, entry_length); printf("\n");
			if (bios->data[start+0] != 0) {
				printf("Entry %d: WR(%d), CL(%d)\n",
					i, tWR,tCL);
				printf("       : RP(%d), RAS(%d), RFC(%d), RC(%d)\n",
					tRP, tRAS, tRFC, tRC);

				/* XXX: I don't trust the -1's and +1's... they must come
				*      from somewhere! */
				if (card_codename < 0xc0) {
					reg_100220 = (tRC << 24 | tRFC << 16 | tRAS << 8 | tRP);
					reg_100224 = ((tWR + tUNK_19 + 1 + magic_number) << 24
								| (tUNK_1 + tUNK_19 + 1 + magic_number) << 8);

					if (card_codename == 0xa8) {
						reg_100224 |= (tCL - 1);
					} else {
						reg_100224 |= (tCL + 2 - magic_number);
					}
					reg_100224 += (tUNK_18 ? tUNK_18 : 1) << 16;

					reg_100228 = ((tUNK_12 << 16) | tUNK_11 << 8 | tUNK_10);
					if(header_length > 19 && card_codename > 0xa5) {
						reg_100228 += (tUNK_19 - 1) << 24;
					} else if (header_length <= 19 && card_codename < 0xa0) {
						reg_100228 += magic_number << 24;
					}

					if(card_codename < 0x50) {
						/* Don't know, don't care...
						* don't touch the rest */
						reg_100228 |= 0x20200000 + (magic_number << 24);
					} else {
						reg_100230 = (tUNK_20 << 24 | tUNK_21 << 16 |
									tUNK_13 << 8  | tUNK_13);

						reg_100234 = (tRAS << 24 | tRC);
						if(tUNK_10 > tUNK_11) {
							reg_100234 += tUNK_10 << 16;
						} else {
							reg_100234 += tUNK_11 << 16;
						}

						if(p_tbls_ver == 1) {
							reg_10022c = (0x14 + tCL) << 24 |
										0x16 << 16 |
										(tCL - 1) << 8 |
										(tCL - 1);
							reg_100234 |= (tCL + 2) << 8;
							reg_10023c |= 0x4000000 | (tCL - 1) << 16 | 0x202;
						} else {
							/* See d.bul in NV98..
							 * seems to have changed for G105M+
							 * 10023c seen as 06xxxxxx, 0bxxxxxx or 0fxxxxxx */
							reg_10022c = (tCL - 1);
							reg_100234 |= (tUNK_19 + 6) << 8;
							reg_10023c = 0x202;
						}
					}

					printf("Registers: 220: %08x %08x %08x %08x\n",
					reg_100220, reg_100224,
					reg_100228, reg_10022c);
					printf("           230: %08x %08x %08x %08x\n",
					reg_100230, reg_100234,
					reg_100238, reg_10023c);
				} else {
					reg_100220 = (tRC << 24 | (tRFC&0x7f) << 17 | tRAS << 8 | tRP);
					reg_100224 = 0x4c << 24 | (tUNK_11&0x0f) << 20 | (tUNK_19 << 7) | (tCL & 0x0f);
					reg_100228 = 0x44000011 | tWR << 16 | tUNK_1 << 8;
					reg_10022c = tUNK_20 << 9 | tUNK_13;
					reg_100230 = 0x42e00069 | tUNK_12 << 15;

					printf("Registers: 290: %08x %08x %08x %08x\n",
					reg_100220, reg_100224,
					reg_100228, reg_10022c);
					printf("           2a0: %08x %08x %08x %08x\n",
					reg_100230, reg_100234,
					reg_100238, reg_10023c);
				}
			}
			printf("\n");

			start += entry_length;
		}
	}

	if(timings_map_tbl_ptr) {
		/* Mapping timings to clockspeeds since 2009 */
		uint8_t 	version = 0, entry_count = 0, entry_length = 0,
				xinfo_count = 0, xinfo_length = 0,
				header_length = 0;
		uint16_t start = timings_map_tbl_ptr;
		uint16_t clock_low = 0, clock_hi = 0;
		int j;
		uint8_t ram_cfg = strap?(strap & 0x1c) >> 2:0xff;
		uint8_t timing;

		version = bios->data[start];
		if (version == 0x10) {
			header_length = bios->data[start+1];
			entry_count = bios->data[start+5];
			entry_length = bios->data[start+2];
			xinfo_count = bios->data[start+4];
			xinfo_length = bios->data[start+3];
		}
		printf ("Timing mapping table at %x. Version %x.\n", timings_map_tbl_ptr, version);
		printf("Header:\n");
		printcmd(timings_map_tbl_ptr, header_length>0?header_length:10);
		printf ("\n\n");

		start += header_length;

		for(i = 0; i < entry_count; i++) {
			clock_low = le16(start);
			clock_hi = le16(start+2);
			timing = bios->data[start+entry_length+((ram_cfg+1)*xinfo_length)+1];

			printf("Entry %d: %d MHz - %d MHz, Timing %i\n",i, clock_low, clock_hi,timing);
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

	if(pm_unknown_tbl_ptr) {
		uint8_t 	version = 0, entry_count = 0, entry_length = 0,
				header_length = 0;
		uint16_t start = pm_unknown_tbl_ptr;

		version = bios->data[start];

		if (version == 0x10) {
			header_length = bios->data[start+1];
			entry_count = bios->data[start+3];
			entry_length = bios->data[start+2];
		}

		printf ("Unknown PM table at %x. Version %x.\n", pm_unknown_tbl_ptr, version);
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
