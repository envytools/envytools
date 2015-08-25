/*
 * Copyright (C) 2015 Samuel Pitoiset <samuel.pitoiset@gmail.com>.
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
#include <stdio.h>
#include <string.h>

#include "decode_utils.h"
#include "rnn.h"
#include "rnndec.h"
#include "util.h"

struct ioctl_call {
	int dir;
	uint32_t addr;
	uint32_t value;
	uint32_t mask;
};

struct trace {
	struct ioctl_call ioctls[2048];
	int nb_ioctl;
};

/* rnn context. */
struct rnndomain *rnndom;
struct rnndeccontext *rnnctx;
struct rnndb *rnndb;

static int
lookup(uint32_t addr, uint32_t val)
{
	struct rnndecaddrinfo *info;

	info = rnndec_decodeaddr(rnnctx, rnndom, addr, 0);
	if (!info)
		return 1;

	if (info->typeinfo) {
		char *res = rnndec_decodeval(rnnctx, info->typeinfo,
					     val, info->width);
		printf("%s => %s\n", info->name, res);
		free(res);
	} else {
		printf("%s\n", info->name);
	}
	rnndec_free_decaddrinfo(info);
	return 0;
}

static int
decode_trace(const char *chipset, const char *event)
{
	char cmd[1024], buf[1024];
	FILE *f;

	// TODO: use demmt api instead of this bad popen().
	sprintf(cmd, "demmt -l %s.trace -m %s -d all -e nvrm-mthd=0x20800122 "
		     " -e ioctl-desc 2> /dev/null | grep dir > %s.demmt",
		     event, chipset, event);

	if (!(f = popen(cmd, "r"))) {
		perror("popen");
		return 1;
	}

	while (fgets(buf, sizeof(buf), f) != NULL)
		printf("%s", buf);

	if (pclose(f) < 0) {
		perror("pclose");
		return 1;
	}

	return 0;
}

static struct trace *
parse_trace(const char *event)
{
	char line[1024], path[1024];
	struct trace *t;
	FILE *f;

	sprintf(path, "%s.demmt", event);

	f = fopen(path, "r");
	if (!f) {
		perror("fopen");
		return NULL;
	}

	if (!(t = (struct trace *)calloc(1, sizeof(*t)))) {
		perror("malloc");
		return NULL;
	}

	while (fgets (line, sizeof(line), f) != NULL) {
		struct ioctl_call *c = &t->ioctls[t->nb_ioctl++];
		char *token;

		/* Read/write. */
		token = strstr(line, "dir:");
		if (!token)
			goto bad_trace;
		sscanf(token + 5, "0x%08x", &c->dir);

		/* Register. */
		token = strstr(line, "mmio:");
		if (!token)
			goto bad_trace;
		sscanf(token + 6, "0x%08x", &c->addr);

		/* Value. */
		token = strstr(line, "value:");
		if (!token)
			goto bad_trace;
		sscanf(token + 7, "0x%08x", &c->value);

		/* Mask. */
		token = strstr(line, "mask:");
		if (!token)
			goto bad_trace;
		sscanf(token + 6, "0x%08x", &c->mask);
	}

	fclose(f);
	return t;

bad_trace:
	fclose(f);
	free(t);
	return NULL;
}

int
lookup_trace(const char *chipset, const char *event)
{
	struct trace *t;
	int i;

	if (decode_trace(chipset, event)) {
		fprintf(stderr, "Failed to decode trace using demmt!\n");
		return 1;
	}

	if (!(t = parse_trace(event)))
		return 1;

	for (i = 0; i < t->nb_ioctl; i++) {
		struct ioctl_call *c = &t->ioctls[i];

		printf("(%c) register: %06x, value: %08x, mask: %08x ",
		       (c->dir & 0x1 ? 'w' : 'r'), c->addr, c->value, c->mask);
		printf("%s ", (c->dir & 0x1 ? "<==" : "==>"));

		if (lookup(c->addr, c->value)) {
			fprintf(stderr, "Failed to decode addr using lookup!\n");
			return 1;
		}
	}
	free(t);

	return 0;
}

void
init_rnnctx(const char *chipset, int use_colors)
{
	rnn_init();
	rnndb = rnn_newdb();
	rnn_parsefile(rnndb, "root.xml");
	rnn_prepdb(rnndb);
	rnnctx = rnndec_newcontext(rnndb);
	if (use_colors)
		rnnctx->colors = &envy_def_colors;
	rnndec_varaddvalue(rnnctx, "chipset", strtoull(chipset, NULL, 16));
	rnndom = rnn_finddomain(rnndb, "NV_MMIO");
}

void
destroy_rnnctx()
{
	rnndec_freecontext(rnnctx);
	rnn_freedb(rnndb);
	rnn_fini();
}
