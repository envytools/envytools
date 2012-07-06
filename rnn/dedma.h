/*
 * Copyright (C) 2010 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __DEDMA_H__
#define __DEDMA_H__

#define _GNU_SOURCE
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "rnn.h"
#include "rnndec.h"

#define MAX_DELTA 16384 /* size of the reordering window */
#define MAX_OUT_OF_BAND 8 /* maximum consecutive out of band data entries */
#define MAX_CACHE 4096 /* dump cache size */
#define MAX_OBJECTS 256 /* object cache size */
#define MAX_SUBCHAN 8

struct ent {
	uint32_t addr;
	uint32_t val;
	bool out_of_band;
};

struct obj {
	uint32_t handle;
	uint32_t class;
	char *name;
	struct rnndeccontext *ctx;
};

struct filter {
	int map;
	uint32_t addr0, addr1; /* address range we care about */
};

struct cache {
	int i0, i1; /* cached index range */
	int k; /* offset the cache starts in */
	struct ent ent[MAX_CACHE];
};

struct dma {
	int incr;
	int subchan;
	int addr;
	int size;
	int skip;
	int nops;
};

struct state {
	struct {
		bool (*parse)(struct state *);
		void (*flush)(struct state *, struct ent *);
		int (*print)(const char *, ...);
	} op;

	FILE *f;
	uint32_t chipset;
	uint32_t pci_id;

	struct rnndb *db;
	struct rnndomain *dom;
	const struct envy_colors *colors;

	struct filter filter;
	struct cache cache;

	struct {
		char *buf;
		size_t n;
		uint32_t addr;
	} parse;

	struct obj objects[MAX_OBJECTS];
	struct obj *subchan[MAX_SUBCHAN];
	struct dma dma;
};

/* dedma.c */
void
add_object(struct state *s, uint32_t handle, uint32_t class);

struct obj *
get_object(struct state *s, uint32_t handle);

int
dedma(struct state *s, FILE *f, bool dry_run);

/* dedma_cache.c */
struct ent *
get_ent(struct state *s, int i);

void
rotate_ent(struct state *s, int i, int j);

void
add_ent(struct state *s, struct ent *e);

void
drop_ent(struct state *s, int i);

void
flush_cache(struct state *s);

/* dedma_back.c */
void
parse_renouveau_chipset(struct state *s, char *path);

void
parse_renouveau_objects(struct state *s, char *path);

void
parse_renouveau_startup(struct state *s, char *path);

bool
parse_renouveau(struct state *s);

bool
parse_valgrind(struct state *s);

#endif
