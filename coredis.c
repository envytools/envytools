/*
 *
 * Copyright (C) 2009 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "coredis.h"

/*
 * Color scheme
 */

char *cnorm = "\x1b[0m";	// lighgray: instruction code and misc stuff
char *cgray = "\x1b[37m";	// darkgray: instruction address
char *cgr = "\x1b[32m";		// green: instruction name and mods
char *cbl = "\x1b[34m";		// blue: $r registers
char *ccy = "\x1b[36m";		// cyan: memory accesses
char *cyel = "\x1b[33m";	// yellow: numbers
char *cred = "\x1b[31m";	// red: unknown stuff
char *cbr = "\x1b[37m";		// white: code labels
char *cmag = "\x1b[35m";	// pink: funny registers

void atomtab APROTO {
	const struct insn *tab = v;
	int i;
	while ((a[0]&tab->mask) != tab->val || !(tab->ptype&ptype))
		tab++;
	m[0] |= tab->mask;
	for (i = 0; i < 16; i++)
		if (tab->atoms[i].fun)
			tab->atoms[i].fun (out, a, m, tab->atoms[i].arg, ptype, pos);
}

void atomname APROTO {
	fprintf (out, " %s%s", cgr, (char *)v);
}

void atomnl APROTO {
	fprintf (out, "\n                          ");
}

void atomoops APROTO {
	fprintf (out, " %s???", cred);
}

void atomnum APROTO {
	const int *n = v;
	ull num = BF(n[0], n[1])<<n[2];
	if (n[3] && num&1ull<<(n[1]+n[2]-1))
		fprintf (out, " %s-%#llx", cyel, (1ull<<n[1]) - num);
	else
		fprintf (out, " %s%#llx", cyel, num);
}

void atomign APROTO {
	const int *n = v;
	(void)BF(n[0], n[1]);
}

void atomreg APROTO {
	const int *n = v;
	int r = BF(n[0], n[1]);
	if (r == 127 && n[2] == 'o') fprintf (out, " %s#", cbl);
	else fprintf (out, " %s$%c%d", (n[2]=='r')?cbl:cmag, n[2], r);
}
void atomdreg APROTO {
	const int *n = v;
	fprintf (out, " %s$%c%lldd", (n[2]=='r')?cbl:cmag, n[2], BF(n[0], n[1]));
}
void atomqreg APROTO {
	const int *n = v;
	fprintf (out, " %s$%c%lldq", (n[2]=='r')?cbl:cmag, n[2], BF(n[0], n[1]));
}
void atomhreg APROTO {
	const int *n = v;
	int r = BF(n[0], n[1]);
	if (r == 127 && n[2] == 'o') fprintf (out, " %s#", cbl);
	else fprintf (out, " %s$%c%d%c", (n[2]=='r')?cbl:cmag, n[2], r>>1, "lh"[r&1]);
}

uint32_t readle32 (uint8_t *p) {
	return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

uint16_t readle16 (uint8_t *p) {
	return p[0] | p[1] << 8;
}
