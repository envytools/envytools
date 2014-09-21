/*
 * Copyright (C) 2014 Marcin Ślusarz <marcin.slusarz@gmail.com>.
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

#include <stdlib.h>
#include "region.h"
#include "demmt.h"

void dump_regions(struct regions *regions)
{
	struct region *cur = regions->head;
	while (cur)
	{
		mmt_log("<0x%08x, 0x%08x>\n", cur->start, cur->end);
		cur = cur->next;
	}
}

static int regions_are_sane(struct regions *regions)
{
	struct region *cur = regions->head;
	if (!cur)
		return 1;

	if (cur->prev)
	{
		mmt_error("%s", "start->prev != NULL\n");
		return 0;
	}

	while (cur)
	{
		if (cur->start >= cur->end)
		{
			mmt_error("cur->start >= cur->end 0x%x 0x%x\n", cur->start, cur->end);
			return 0;
		}

		if (cur->next)
		{
			if (cur->end >= cur->next->start)
			{
				mmt_error("cur->end >= cur->next->start 0x%x 0x%x\n", cur->end, cur->next->start);
				return 0;
			}
		}

		cur = cur->next;
	}

	return 1;
}

static int range_in_regions(struct regions *regions, uint32_t start, uint8_t len)
{
	struct region *cur = regions->head;

	while (cur)
	{
		if (start >= cur->start && start + len <= cur->end)
			return 1;

		cur = cur->next;
	}

	return 0;
}

static void drop_region(struct region *reg, struct regions *parent)
{
	struct region *prev = reg->prev;
	struct region *next = reg->next;
	mmt_debug("dropping entry <0x%08x, 0x%08x>\n", reg->start, reg->end);
	free(reg);
	if (prev)
		prev->next = next;
	else
	{
		mmt_debug("new head is at <0x%08x, 0x%08x>\n", next->start, next->end);
		parent->head = next;
	}

	if (next)
		next->prev = prev;
	else
	{
		mmt_debug("new last is at <0x%08x, 0x%08x>\n", prev->start, prev->end);
		parent->last = prev;
	}
}

static void maybe_merge_with_next(struct region *cur, struct regions *parent)
{
	// next exists?
	if (!cur->next)
		return;

	// next starts after this one?
	if (cur->next->start > cur->end)
		return;

	// now next starts within this one

	// next ends within this one?
	if (cur->next->end <= cur->end)
	{
		// drop next one
		drop_region(cur->next, parent);

		// and see if next^2 should be merged
		maybe_merge_with_next(cur, parent);
	}
	else
	{
		// now next ends after this one

		// extend current end
		mmt_debug("extending entry <0x%08x, 0x%08x> right to 0x%08x\n",
				cur->start, cur->end, cur->next->end);
		cur->end = cur->next->end;

		// and drop next entry
		drop_region(cur->next, parent);
	}
}

static void maybe_merge_with_previous(struct region *cur, struct regions *parent)
{
	// previous exists?
	if (!cur->prev)
		return;

	// previous ends before start of this one?
	if (cur->prev->end < cur->start)
		return;

	// now previous ends within this one

	// previous starts within this one?
	if (cur->prev->start >= cur->start)
	{
		// just drop previous entry
		drop_region(cur->prev, parent);

		// and see if previous^2 should be merged
		maybe_merge_with_previous(cur, parent);
	}
	else
	{
		// now previous starts before this one

		// extend current start
		mmt_debug("extending entry <0x%08x, 0x%08x> left to 0x%08x\n",
				cur->start, cur->end, cur->prev->start);
		cur->start = cur->prev->start;

		// and drop previous entry
		drop_region(cur->prev, parent);
	}
}

void free_regions(struct regions *regions)
{
	struct region *cur = regions->head, *next;
	if (!cur)
		return;

	while (cur)
	{
		next = cur->next;
		free(cur);
		cur = next;
	}

	regions->head = NULL;
	regions->last = NULL;
}

static void __regions_add_range(struct regions *regions, uint32_t start, uint8_t len)
{
	struct region *cur = regions->head;

	if (cur == NULL)
	{
		regions->head = cur = malloc(sizeof(struct region));
		cur->start = start;
		cur->end = start + len;
		cur->prev = NULL;
		cur->next = NULL;
		regions->last = cur;
		return;
	}

	struct region *last_reg = regions->last;
	if (start == last_reg->end)
	{
		mmt_debug("extending last entry <0x%08x, 0x%08x> right by %d\n",
				last_reg->start, last_reg->end, len);
		last_reg->end += len;
		return;
	}

	if (start > last_reg->end)
	{
		mmt_debug("adding last entry <0x%08x, 0x%08x> after <0x%08x, 0x%08x>\n",
				start, start + len, last_reg->start, last_reg->end);
		cur = malloc(sizeof(struct region));
		cur->start = start;
		cur->end = start + len;
		cur->prev = last_reg;
		cur->next = NULL;
		last_reg->next = cur;
		regions->last = cur;
		return;
	}

	if (start + len > last_reg->end)
	{
		mmt_debug("extending last entry <0x%08x, 0x%08x> right to 0x%08x\n",
				last_reg->start, last_reg->end, start + len);
		last_reg->end = start + len;
		if (start < last_reg->start)
		{
			mmt_debug("... and extending last entry <0x%08x, 0x%08x> left to 0x%08x\n",
					last_reg->start, last_reg->end, start);
			last_reg->start = start;
			maybe_merge_with_previous(last_reg, regions);
		}
		return;
	}


	while (start + len > cur->end) // if we will ever crash on cur being NULL then it means we screwed up earlier
	{
		if (cur->end == start) // optimization
		{
			mmt_debug("extending entry <0x%08x, 0x%08x> by %d\n", cur->start, cur->end, len);
			cur->end += len;
			maybe_merge_with_next(cur, regions);
			return;
		}
		cur = cur->next;
	}

	// now it ends before end of current

	// does it start in this one?
	if (start >= cur->start)
		return;

	// does it end before start of current one?
	if (start + len < cur->start)
	{
		mmt_debug("adding new entry <0x%08x, 0x%08x> before <0x%08x, 0x%08x>\n",
				start, start + len, cur->start, cur->end);
		struct region *tmp = malloc(sizeof(struct region));
		tmp->start = start;
		tmp->end = start + len;
		tmp->prev = cur->prev;
		tmp->next = cur;
		if (cur->prev)
			cur->prev->next = tmp;
		else
			regions->head = tmp;
		cur->prev = tmp;
		maybe_merge_with_previous(tmp, regions);
		return;
	}

	// now it ends in current and starts before
	cur->start = start;
	maybe_merge_with_previous(cur, regions);
}

int regions_add_range(struct regions *regions, uint32_t start, uint8_t len)
{
	__regions_add_range(regions, start, len);

	if (!regions_are_sane(regions))
		return 0;

	if (!range_in_regions(regions, start, len))
	{
		mmt_error("region <0x%08x, 0x%08x> was not added!\n", start, start + len);
		return 0;
	}

	return 1;
}
