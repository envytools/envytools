/*
 * Copyright (C) 2014-2015 Martin Peres <martin.peres@free.fr>
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
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <malloc.h>

FILE *open_input(char *filename) {
	const char * const tab[][2] = {
		{ ".gz", "zcat" },
		{ ".Z", "zcat" },
		{ ".bz2", "bzcat" },
		{ ".xz", "xzcat" },
	};
	int i;
	int flen = strlen(filename);
	for (i = 0; i < sizeof tab / sizeof tab[0]; i++) {
		int elen = strlen(tab[i][0]);
		if (flen > elen && !strcmp(filename + flen - elen, tab[i][0])) {
			fprintf (stderr, "Compressed trace detected, trying to decompress...\n");
			char *cmd = malloc(flen + strlen(tab[i][1]) + 2);
			FILE *res;
			strcpy(cmd, tab[i][1]);
			strcat(cmd, " ");
			strcat(cmd, filename);
			res = popen(cmd, "r");
			free(cmd);
			return res;
		}
	}
	return fopen(filename, "r");
}

int parse_line(const char *line, size_t len, uint32_t *reg, uint32_t *val)
{
	unsigned int size, useless;
	float time;

	if (line[0] != 'W')
		return 0;

	sscanf(line + 2, "%u %f %u %x %x", &size, &time, &useless, reg, val);
	*reg &= 0xffffff;

	return 1;
}

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c, cnum, verbose = 0;
	size_t start = 0, steps = -1;
	uint32_t mmio_start = 0, mmio_end = 0xffffffff;
	struct nva_regspace rs = { 0 };
	while ((c = getopt (argc, argv, "c:b:s:l:h:v")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
			case 'b':
				sscanf(optarg, "%zu", &start);
				break;
			case 's':
				sscanf(optarg, "%zu", &steps);
				break;
			case 'l':
				sscanf(optarg, "%x", &mmio_start);
				break;
			case 'h':
				sscanf(optarg, "%x", &mmio_end);
				break;
			case 'v':
				verbose++;
				break;
		}
	if (rs.cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}

	if (optind >= argc) {
		fprintf (stderr, "No mmiotrace specified.\n");
		return 1;
	}

	FILE *f = open_input(argv[optind]);
	if (!f) {
		fprintf(stderr, "couldn't open '%s' for reading\n", argv[optind]);
		return 1;
	}

	if (mmio_start != 0 || mmio_end != 0xffffffff)
		printf("limit the replay to registers in the range [%x:%x]\n",
		       mmio_start, mmio_end);

	char *buf = NULL;
	size_t len, cur = 0, reg_writes = -1;

	while (getline(&buf, &len, f) > 0) {
		uint32_t reg, val;
		if (cur >= start) {
			if (reg_writes == (size_t) -1) {
				if (steps < (size_t) -1) {
					printf("replay %zu writes starting from line %zu: ",
					       steps, cur);
					fflush(stdout);
					reg_writes = 0;
				} else
					printf("replay from line %zu to the end\n", cur);
			}

			if (parse_line(buf, len, &reg, &val) &&
				reg >= mmio_start && reg <= mmio_end)
			{
				nva_wr32(cnum, reg, val);
				if (verbose > 0)
					printf("\n	%x <= %x ", reg, val);
				reg_writes++;
			}

			if ((reg_writes % steps) == (steps - 1)) {
				printf("Press enter to continue.");
				fflush(stdout);
				getchar();
				reg_writes = -1;
			}
		}
		cur++;
	}
	printf("\n");

	return 0;
}
