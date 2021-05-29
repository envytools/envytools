/*
 * Copyright (C) 2020 Karol Herbst <nouveau@karolherbst.de>
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

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rnn.h"
#include "rnndec.h"
#include "util.h"

#define MAX_SUBCHANS 8
#define MAX(a,b) (((a)>(b))?(a):(b))

struct depushbuf_context
{
	const char *file;
	uint16_t chipset;
	FILE *f;

	struct rnndomain *rnndomain;
	struct rnndeccontext *rnndecctx[MAX_SUBCHANS];
	const char *subchan_name[MAX_SUBCHANS];
};

static void usage()
{
	fflush(stdout);
	fprintf(stderr,
		"Usage: depushbuf [OPTION]\n"
		"\n"
		"Options:\n"
		"\t-h, --help: prints this help\n"
		"\t-f, --file: the file to parse\n"
		"\t-c, --chipset: the GPUs chipset\n");
	exit(1);
}

static bool
configure(int argc, char *argv[], struct depushbuf_context *ctx)
{
	struct option opts[] = {
		{"help",	no_argument, 		NULL, 	'h'},
		{"file",	required_argument, 	NULL,	'f'},
		{"chipset",	required_argument, 	NULL,	'c'},
		{},
	};

	int c;
	do {
		c = getopt_long_only(argc, argv, "", opts, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'f':
			ctx->file = strdup(optarg);
			break;

		case 'c':
			ctx->chipset = strtoul(optarg, NULL, 16);
			break;

		case 'h':
		case '?':
			usage();
			break;
		}
	} while (true);

	return true;
}

static bool
get_next_data(struct depushbuf_context *ctx, uint32_t *data, uint32_t *skip)
{
	char line[1024];

	while (fgets(line, 1024, ctx->f)) {

		uint32_t tmp, start, end;
		if (sscanf(line, "nouveau: ch%i: psh (unmapped) %x %x %x", &tmp, &tmp, &start, &end) > 1) {
			*skip = ((end & 0x7fffff) - start) / 4;
			return true;
		}

		if (sscanf(line, "nouveau:%*9[ \t]0x%x", data)) {
			*skip = 0;
			return true;
		}
	}
	return false;
}

static void
decode_header(struct depushbuf_context *ctx, uint32_t hdr)
{
	printf("header %08x\n", hdr);
}


static void
decode_mthd(struct depushbuf_context *ctx, uint32_t mthd, uint32_t val, uint32_t subchan)
{
	if (!ctx->rnndecctx[subchan]) {
		printf("[%u] not mapped\n", subchan);
		return;
	}

	struct rnndeccontext *c = ctx->rnndecctx[subchan];
	struct rnndecaddrinfo *info = rnndec_decodeaddr(c, ctx->rnndomain, mthd, 1);
	char *val_str = rnndec_decodeval(c, info->typeinfo, val, info->width);
	if (val_str) {
		printf("0x%08x [%u] %s%s%s.%s = %s\n", val, subchan, c->colors->rname, ctx->subchan_name[subchan], c->colors->reset, info->name, val_str);
		free(val_str);
	} else {
		printf("0x%08x [%u] %s%s%s.%s = 0x%x\n", val, subchan, c->colors->rname, ctx->subchan_name[subchan], c->colors->reset, info->name, val);
	}
}

static uint16_t
class_of_subchan(uint16_t chipset, uint8_t subchan)
{
	if (chipset >= 0xc0) {
		switch (subchan) {
		// 3D
		case 0:
			if (chipset == 0x130 || chipset == 0x13b)
				return 0xc097;
			else if (chipset >= 0x130)
				return 0xc197;
			else if (chipset >= 0x120)
				return 0xb197;
			else if (chipset >= 0x110)
				return 0xb097;
			else if (chipset >= 0xf0)
				return 0xa197;
			else if (chipset == 0xea)
				return 0xa297;
			else if (chipset >= 0xe0)
				return 0xa097;
			else if (chipset >= 0xd0 || chipset == 0xc8)
				return 0x9297;
			else if (chipset == 0xc1)
				return 0x9197;
			else
				return 0x9097;

		// CP
		case 1:
			if (chipset == 0x130 || chipset == 0x13b)
				return 0xc0c0;
			else if (chipset >= 0x130)
				return 0xc1c0;
			else if (chipset >= 0x120)
				return 0xb1c0;
			else if (chipset >= 0x110)
				return 0xb0c0;
			else if (chipset >= 0xf0)
				return 0xa1c0;
			else if (chipset >= 0xe0)
				return 0xa0c0;
			else
				return 0x90c0;

		// M2MF/P2MF
		case 2:
			if (chipset >= 0x0f0)
				return 0xa140;
			else if (chipset >= 0x0e0)
				return 0xa040;
			else
				return 0x9039;

		// 2D
		case 3:
			return 0x902d;

		// COPY
		case 4:
			if (chipset == 0x130 || chipset == 0x13b)
				return 0xc0b5;
			else if (chipset >= 0x130)
				return 0xc1b5;
			else if (chipset >= 0x110)
				return 0xb0b5;
			else
				return 0xa0b5;
		}
	} else if (chipset >= 0x50) {
		switch (subchan) {
		// 3D
		case 3:
			if (chipset == 0xaf)
				return 0x8697;
			else if (chipset == 0xa0 || chipset == 0xaa || chipset == 0xac)
				return 0x8397;
			else if (chipset >= 0xa0)
				return 0x8597;
			else if (chipset >= 0x80)
				return 0x8297;
			else
				return 0x5097;

		// 2D
		case 4:
			return 0x502d;

		// M2MF
		case 5:
			return 0x5039;

		// CP
		case 6:
			return 0x50c0;
		}
	}
	return 0x0000;
}

int
main(int argc, char *argv[])
{
	struct depushbuf_context ctx = {};

	/* parse the command line */
	if (!configure(argc, argv, &ctx))
		return EINVAL;

	rnn_init();
	struct rnndb *rnndb = rnn_newdb();
	rnn_parsefile(rnndb, "fifo/nv_objects.xml");
	if (rnndb->estatus)
		return EINVAL;

	rnn_prepdb(rnndb);
	ctx.rnndomain = rnn_finddomain(rnndb, "SUBCHAN");
	if (!ctx.rnndomain)
		return EINVAL;

	struct rnnvalue *v;
	struct rnnenum *chs = rnn_findenum(rnndb, "chipset");
	struct rnnenum *cls = rnn_findenum(rnndb, "obj-class");

	FINDARRAY(chs->vals, v, v->value == ctx.chipset);
	if (!v)
		return EINVAL;

	for (int i = 0; i < MAX_SUBCHANS; ++i) {
		uint16_t c = class_of_subchan(ctx.chipset, i);
		if (!c)
			continue;

		ctx.rnndecctx[i] = rnndec_newcontext(rnndb);
		if (!ctx.rnndecctx[i])
			return EINVAL;

		ctx.rnndecctx[i]->colors = &envy_def_colors;
		rnndec_varadd(ctx.rnndecctx[i], "chipset", v->name);

		struct rnnvalue *vc;
		FINDARRAY(cls->vals, vc, vc->value == c);
		if (!v)
			return EINVAL;

		rnndec_varadd(ctx.rnndecctx[i], "obj-class", vc->name);
		ctx.subchan_name[i] = vc->name;
	}

	if (ctx.file) {
		ctx.f = fopen(ctx.file, "r");
		if (!ctx.f)
			return ENOMEM;
	} else {
		ctx.f = stdin;
	}

	uint32_t entry, skip;
	while (get_next_data(&ctx, &entry, &skip))
	{
		if (skip) {
			fprintf(stderr, "skipped data\n");
			return EINVAL;
		}

		uint32_t size;
		uint32_t addr;
		uint32_t val;
		uint32_t subchan = (entry & 0xe000) >> 13;

		int32_t inc;
		if (ctx.chipset >= 0xc0) {
			uint32_t mode = (entry & 0xe0000000) >> 29;
			addr = (entry & 0x1fff) << 2;
			size = (entry & 0x1fff0000) >> 16;

			switch (mode) {
			case 1:
				inc = size;
				break;
			case 3:
				inc = 0;
				break;
			case 4:
				val = size;
				size = 0;
				inc = 0;
				break;
			case 5:
				inc = 1;
				break;
			default:
				fprintf(stderr, "unknown mode %u\n", mode);
				return EINVAL;
			}

			decode_header(&ctx, entry);
		} else {
			uint32_t mode = (entry & 0xe0000000) >> 29;
			size = (entry & 0x1ffc0000) >> 18;
			addr = entry & 0x1ffc;

			switch (mode) {
			case 0: {
				uint32_t type = (entry & 0x30000) >> 16;
				switch (type) {
				case 0:
					inc = size;
					break;
				default:
					fprintf(stderr, "unknown type %u\n", mode);
					return EINVAL;
				}

				break;
			}

			case 2:
				inc = 0;
				break;

			default:
				fprintf(stderr, "unknown mode %u\n", mode);
				return EINVAL;
			}

			decode_header(&ctx, entry);
		}

		if (size) {
			for (int i = 0; i < size; ++i) {
				get_next_data(&ctx, &val, &skip);
				if (skip) {
					i += skip - 1;
					printf("MISSING DATA (unmapped pushbuffer) missing %u entries\n", skip);
					addr += (inc - MAX((int32_t)(inc - skip), 0)) * 4;
					inc = MAX((int32_t)(inc - skip), 0);
					continue;
				}

				decode_mthd(&ctx, addr, val, subchan);
				if (inc) {
					inc--;
					addr += 4;
				}
			}
		} else {
			decode_mthd(&ctx, addr, val, subchan);
		}
	}

	if (ctx.file)
		fclose(ctx.f);
}
