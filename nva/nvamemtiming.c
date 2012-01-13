/*
 * Copyright 2011 Martin Peres
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
#include <signal.h>
#include <sys/time.h>

#include "nva.h"
#include "nvamemtiming.h"

void signal_handler(int sig)
{
	if (sig == SIGSEGV)
		fprintf(stderr, "Received a sigsegv. Exit nicely...\n");
	else
		fprintf(stderr, "Received a termination request. Exit nicely...\n");

	system("killall X");
	exit(99);
}

void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [-c cnum ...] vbios.rom timing_table_offset timing_entry perflvl\n", argv[0]);
	fprintf(stderr, "\n\nOptional args:\n");
	fprintf(stderr, "\t-c cnum: Specify the card number\n");
	fprintf(stderr, "\t-t: Generate a mmiotrace of all meaningful operations\n");
	fprintf(stderr, "\t-e: Only modify the specified index (to be used with -v)\n");
	fprintf(stderr, "\t-v: Set the specified value to the specified entry\n");
	fprintf(stderr, "\t-b index: Consider the specified index as a bitfield and RE it\n");
	fprintf(stderr, "\t-d timing_entry_high: For each indexes, iterate between the timing_entry and the timing_entry_high value (Deep mode)\n");
	exit(-1);
}

int read_timings(struct nvamemtiming_conf *conf)
{
	uint8_t *header;

	printf("timing table at: %x\n", conf->vbios.timing_table_offset);
	header = &conf->vbios.data[conf->vbios.timing_table_offset];
	if (header[0] != 0x10) {
		fprintf(stderr, "unknow table version %x\n", header[0]);
		return -1;
	}

	if (conf->timing.entry >= header[2]) {
		fprintf(stderr, "timing entry %i is higher than count(%i)\n", conf->timing.entry, header[2]);
		return -1;
	}

	conf->vbios.timing_entry_length = header[3];
	conf->vbios.timing_entry_offset = conf->vbios.timing_table_offset + header[1] + conf->timing.entry * header[3];

	if (conf->mode == MODE_DEEP)
		conf->deep.timing_entry_offset = conf->vbios.timing_table_offset + header[1] + conf->deep.entry * header[3];

	return 0;
}

int parse_cmd_line(int argc, char **argv, struct nvamemtiming_conf *conf)
{
	int c, val;

	conf->cnum = 0;
	conf->mmiotrace = false;
	conf->mode = MODE_AUTO;
	conf->vbios.file = NULL;
	conf->range.start = -1;
	conf->range.end = -1;

	while ((c = getopt (argc, argv, "hts:f:d:b:c:e:v:")) != -1)
		switch (c) {
			case 'e':
				if (conf->mode != MODE_AUTO && conf->mode != MODE_MANUAL)
					usage(argc, argv);
				conf->mode = MODE_MANUAL;
				sscanf(optarg, "%d", &val);
				conf->manual.index = val;
				break;
			case 'v':
				if (conf->mode != MODE_AUTO && conf->mode != MODE_MANUAL)
					usage(argc, argv);
				conf->mode = MODE_MANUAL;
				sscanf(optarg, "%d", &val);
				conf->manual.value = val;
				break;
			case 'd':
				if (conf->mode != MODE_AUTO)
					usage(argc, argv);
				conf->mode = MODE_DEEP;
				sscanf(optarg, "%d", &val);
				conf->deep.entry = val;
				break;
			case 'b':
				if (conf->mode != MODE_AUTO)
					usage(argc, argv);
				conf->mode = MODE_BITFIELD;
				sscanf(optarg, "%d", &val);
				conf->bitfield.index = val;
				break;
			case 's':
				if (conf->mode != MODE_AUTO)
					usage(argc, argv);
				sscanf(optarg, "%d", &val);
				conf->range.start = val;
				break;
			case 'f':
				if (conf->mode != MODE_AUTO)
					usage(argc, argv);
				sscanf(optarg, "%d", &val);
				conf->range.end = val;
				break;
			case 'c':
				sscanf(optarg, "%d", &val);
				conf->cnum = val;
				break;
			case 't':
				conf->mmiotrace = true;
				break;
			case 'h':
				usage(argc, argv);
		}
	if (conf->cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}

	if (argc != optind + 4)
		usage(argc, argv);

	conf->vbios.file = argv[optind];
	sscanf(argv[optind + 1], "%hx", &conf->vbios.timing_table_offset);
	conf->timing.entry = atoi(argv[optind + 2]);
	conf->timing.perflvl = atoi(argv[optind + 3]);

	return 0;
}

int main(int argc, char** argv)
{
	unsigned long start_time, total_time;
	struct nvamemtiming_conf conf;
	struct timeval tv;
	int ret;

	signal(SIGINT, signal_handler);
	signal(SIGSEGV, signal_handler);

	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}

	parse_cmd_line(argc, argv, &conf);

	/* read the vbios */
	if (vbios_read(conf.vbios.file, &conf.vbios.data, &conf.vbios.length) != 1) {
		fprintf(stderr, "Error while reading the vbios\n");
		return 1;
	}

	/* parse the timing table */
	ret = read_timings(&conf);
	if (ret) {
		fprintf(stderr, "read_timings failed!\n");
		return ret;
	}

	/* start the counter to return how long did the run take */
	gettimeofday(&tv, NULL);
	start_time = tv.tv_sec;

	switch (conf.mode) {
	case MODE_AUTO:
		ret = shallow_dump(&conf);
		break;
	case MODE_DEEP:
		ret = deep_dump(&conf);
		break;
	case MODE_BITFIELD:
		ret = bitfield_check(&conf);
		break;
	case MODE_MANUAL:
		ret = manual_check(&conf);
		break;
	}

	gettimeofday(&tv, NULL);
	total_time = tv.tv_sec - start_time;

	printf("The run tested %i configurations in %li:%li:%lis\n",
	       conf.counter,
	       total_time / 3600,
	       (total_time % 3600) / 60,
	       total_time % 60);

	return ret;
}
