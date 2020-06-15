/*
 * Copyright (C) 2009-2011 Marcelina Kościelnicka <mwk@0x04.net>
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

#include "dis.h"
#include "dis-intern.h"

int op1blen[] = { 1 };
int op2blen[] = { 2 };
int op3blen[] = { 3 };
int op4blen[] = { 4 };
int op5blen[] = { 5 };
int op6blen[] = { 6 };
int op8blen[] = { 8 };

static const struct {
	const char *name;
	struct disisa *isa;
} isas[] = {
	"g80", &g80_isa_s,
	"nv50", &g80_isa_s, /* XXX remove some day */
	"gf100", &gf100_isa_s,
	"nvc0", &gf100_isa_s, /* XXX remove some day */
	"gk110", &gk110_isa_s,
	"gm107", &gm107_isa_s,
	"ctx", &ctx_isa_s,
	"falcon", &falcon_isa_s,
	"fuc", &falcon_isa_s, /* XXX remove some day */
	"fµc", &falcon_isa_s, /* XXX remove some day */
	"hwsq", &hwsq_isa_s,
	"xtensa", &xtensa_isa_s,
	"vp2", &xtensa_isa_s, /* XXX remove some day, preferably before cracking the actual VP2 ISA open */
	"vuc", &vuc_isa_s,
	"vµc", &vuc_isa_s,
	"macro", &macro_isa_s,
	"vp1", &vp1_isa_s,
	"vcomp", &vcomp_isa_s,
};

const struct disisa *ed_getisa(const char *name) {
	int i;
	for (i = 0; i < sizeof isas / sizeof *isas; i++)
		if (!strcmp(name, isas[i].name)) {
			struct disisa *isa = isas[i].isa;
			if (!isa->prepdone) {
				if (isa->prep)
					isa->prep(isa);
				if (!isa->vardata) {
					isa->vardata = vardata_new("empty");
					vardata_validate(isa->vardata);
				}
				isa->prepdone = 1;
			}
			return isa;
		}
	return 0;
};

void ed_freeisa(const struct disisa *isa) {
	if (!isa->prepdone)
		return;
	vardata_del(isa->vardata);
	((struct disisa *)isa)->prepdone = 0;
}

uint32_t ed_getcbsz(const struct disisa *isa, struct varinfo *varinfo) {
	if (isa->getcbsz)
		return isa->getcbsz(isa, varinfo);
	return isa->posunit * 8;
}
