/*
 * Copyright 2009 Emil Velikov
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "nva.h"

int vbios_read(const char *filename, uint8_t **vbios, uint16_t *length);
int work_loop(int cnum, uint8_t *vbios_data, uint16_t vbios_length,
	      uint16_t timing_entry_offset, uint8_t entry_length, uint8_t perflvl);

void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [-c cnum] vbios.rom timing_table_offset timing_entry perflvl\n", argv[0]);
	exit(-1);
}

int parse_cmd_line(int argc, char **argv,
		   int *cnum, char **vbios,
		   uint16_t *timing_offset, uint8_t *timing_entry, uint8_t *perflvl)
{
	int c;

	*cnum = 0;
	*vbios = NULL;

	while ((c = getopt (argc, argv, "c:")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", cnum);
				break;
		}
	if (*cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}

	if (argc != optind + 4)
		usage(argc, argv);

	*vbios = argv[optind];
	sscanf(argv[optind + 1], "%x", (unsigned int*)timing_offset);
	*timing_entry = atoi(argv[optind + 2]);
	*perflvl = atoi(argv[optind + 3]);

	return 0;
}

int read_timings(uint8_t *vbios_data, uint16_t vbios_length,
		uint16_t timing_table_offset, uint8_t timing_entry,
		uint16_t *timing_offset, uint8_t *entry_length)
{
	uint8_t *header;

	header = &vbios_data[timing_table_offset];
	if (header[0] != 0x10) {
		fprintf(stderr, "unknow table version %x\n", header[0]);
		return -1;
	}

	if (timing_entry >= header[2]) {
		fprintf(stderr, "timing entry %i is higher than count(%i)\n", timing_entry, header[2]);
		return -1;
	}

	*entry_length = header[3];

	*timing_offset = timing_table_offset + header[1] + timing_entry * *entry_length;

	return 0;
}

int main(int argc, char** argv)
{
	/* after parsing the command line */
	int cnum;
	uint16_t timing_table_offset;
	uint8_t timing_entry, perflvl;
	char *vbios;

	/* after reading the vbios */
	uint8_t *vbios_data;
	uint16_t vbios_length;

	/* after parsing the timing table */
	uint16_t timing_offset;
	uint8_t entry_length;

	int ret;

	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	
	parse_cmd_line(argc, argv, &cnum, &vbios, &timing_table_offset, &timing_entry, &perflvl);

	if (!vbios_read(vbios, &vbios_data, &vbios_length)) {
		fprintf(stderr, "Error while reading the vbios\n");
		return 1;
	}
	
	ret = read_timings(vbios_data, vbios_length, timing_table_offset, timing_entry,
		&timing_offset, &entry_length);
	if (ret) {
		fprintf(stderr, "read_timings failed!\n");
		return ret;
	}

	/* TODO: Allow the user to select the operation mode he wants */
	return work_loop(cnum, vbios_data, vbios_length, timing_offset, entry_length, perflvl);
}
