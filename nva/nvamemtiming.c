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
int complete_dump(int cnum, uint8_t *vbios_data, uint16_t vbios_length,
	      uint16_t timing_entry_offset, uint8_t entry_length,
	      uint8_t perflvl, int do_mmiotrace);

int manual_check(int cnum, int manual_entry, int manual_value,
		 uint8_t *vbios_data, uint16_t vbios_length,
		 uint16_t timing_entry_offset, uint8_t entry_length,
		 uint8_t perflvl, int do_mmiotrace);

void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [-c cnum ...] vbios.rom timing_table_offset timing_entry perflvl\n", argv[0]);
	fprintf(stderr, "\n\nOptional args:\n");
	fprintf(stderr, "\t-c cnum: Specify the card number\n");
	fprintf(stderr, "\t-t: Generate a mmiotrace of all meaningful operations\n");
	//fprintf(stderr, "\t-d: Force deep testing\n");
	fprintf(stderr, "\t-e: Only modify the specified entry (to be used with -v)\n");
	fprintf(stderr, "\t-v: Set the specified value to the specified entry\n");
	exit(-1);
}

int parse_cmd_line(int argc, char **argv,
		   int *cnum, int *mmiotrace,
		   int *manual_entry, int *manual_value,
		   char **vbios,
		   uint16_t *timing_offset, uint8_t *timing_entry, uint8_t *perflvl)
{
	int c;

	*cnum = 0;
	*mmiotrace = 0;
	*vbios = NULL;
	*manual_entry = -1;
	*manual_value = -1;

	while ((c = getopt (argc, argv, "htc:e:v:")) != -1)
		switch (c) {
			case 'e':
				sscanf(optarg, "%d", manual_entry);
				break;
			case 'v':
				sscanf(optarg, "%d", manual_value);
				break;
			case 'c':
				sscanf(optarg, "%d", cnum);
				break;
			case 't':
				*mmiotrace = 1;
				break;
			case 'h':
				usage(argc, argv);
		}
	if (*cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}

	if ((*manual_entry < 0 && *manual_value >= 0) ||
	    (*manual_entry >= 0 && *manual_value < 0))
		usage(argc, argv);

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
	int cnum, mmiotrace, manual_entry, manual_value;
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
	
	parse_cmd_line(argc, argv, &cnum, &mmiotrace, &manual_entry, &manual_value, &vbios, &timing_table_offset, &timing_entry, &perflvl);

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

	if (manual_entry < 0)
		return complete_dump(cnum, vbios_data, vbios_length, timing_offset, entry_length, perflvl, mmiotrace);
	else
		return manual_check(cnum, manual_entry, manual_value, vbios_data, vbios_length, timing_offset, entry_length, perflvl, mmiotrace);
}
