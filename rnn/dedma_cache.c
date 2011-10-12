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

#include "dedma.h"

#define CACHE_ENT(s, i)							\
	(&(s)->cache.ent[((i) - (s)->cache.i0 + (s)->cache.k) % MAX_CACHE])

struct ent *
get_ent(struct state *s, int i)
{
	struct cache *c = &s->cache;

	if (i < c->i0 || i > c->i1)
		return NULL;

	if (i == c->i1 && !s->op.parse(s))
		return NULL;

	return CACHE_ENT(s, i);
}

void
rotate_ent(struct state *s, int i, int j)
{
	const int d = sizeof(struct ent);
	struct cache *c = &s->cache;
	int ki = CACHE_ENT(s, i) - c->ent;
	int kj = CACHE_ENT(s, j) - c->ent;
	struct ent tmp;

	assert(j >= i && j >= c->i0 && j < c->i1 &&
	       i >= c->i0 && i < c->i1);

	memmove(&tmp, c->ent + kj, d);

	if (ki >= c->k && kj < c->k) {
		/* the region wraps around */
		memmove(c->ent + 1, c->ent, d * kj);
		memmove(c->ent, c->ent + MAX_CACHE - 1, d);
		memmove(c->ent + ki + 1, c->ent + ki, d * (MAX_CACHE - ki - 1));

	} else {
		/* easy case */
		memmove(c->ent + ki + 1, c->ent + ki, d * (kj - ki));
	}

	memmove(c->ent + ki, &tmp, d);
}

void
add_ent(struct state *s, struct ent *e)
{
	struct cache *c = &s->cache;

	if (c->i1 - c->i0 == MAX_CACHE) {
		s->op.flush(s, CACHE_ENT(s, c->i0));
		c->k = (c->k + 1) % MAX_CACHE;
		c->i0++;
	}

	*CACHE_ENT(s, c->i1) = *e;
	c->i1++;
}

void
drop_ent(struct state *s, int i)
{
	struct cache *c = &s->cache;

	rotate_ent(s, c->i0, i);
	c->k = (c->k + 1) % MAX_CACHE;
	c->i1--;
}

void
flush_cache(struct state *s)
{
	struct cache *c = &s->cache;
	int i;

	for (i = c->i0; i < c->i1; i++)
		s->op.flush(s, CACHE_ENT(s, i));

	c->i0 = c->i1 = 0;
}
