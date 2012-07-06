/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ed2_misc.h"
#include "mask.h"
#include "ed2i.h"
#include <stdlib.h>

void ed2i_del_isa(struct ed2i_isa *isa) {
	symtab_del(isa->symtab);
	vardata_del(isa->vardata);
	int i;
	for (i = 0; i < isa->opfieldsnum; i++) {
		free(isa->opfields[i].name);
		ed2_free_strings(isa->opfields[i].enumvals, isa->opfields[i].enumvalsnum);
	}
	free(isa->opfields);
	free(isa->opdefault);
	free(isa->opdefmask);
	free(isa);
}

void ed2i_set_opfield_1(uint32_t *mask, struct ed2i_opfield *opf) {
	/* XXX: unsuck? */
	int i;
	for (i = 0; i < opf->len; i++) {
		mask_set(mask, opf->start + i);
	}
}

void ed2i_set_opfield_val(uint32_t *mask, struct ed2i_opfield *opf, uint64_t val) {
	/* XXX: unsuck? */
	int i;
	for (i = 0; i < opf->len; i++) {
		if (i < 64 && ((val >> i) & 1))
			mask_set(mask, opf->start + i);
	}
}
