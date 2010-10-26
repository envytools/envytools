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

#include "dis.h"

/*
 * Color scheme
 */

char *cnorm = "\x1b[0m";	// lighgray: instruction code and misc stuff
char *cgray = "\x1b[0;37m";	// darkgray: instruction address
char *cgr = "\x1b[0;32m";	// green: instruction name and mods
char *cbl = "\x1b[0;34m";	// blue: $r registers
char *ccy = "\x1b[0;36m";	// cyan: memory accesses
char *cyel = "\x1b[0;33m";	// yellow: numbers
char *cred = "\x1b[0;31m";	// red: unknown stuff
char *cmag = "\x1b[0;35m";	// pink: funny registers, jump labels
char *cbr = "\x1b[1;37m";	// white: call labels
char *cbrmag = "\x1b[1;35m";	// white: call and jump labels

void atomtab APROTO {
	const struct insn *tab = v;
	int i;
	while ((a[0]&tab->mask) != tab->val || !(tab->ptype & ctx->ptype))
		tab++;
	m[0] |= tab->mask;
	for (i = 0; i < 16; i++)
		if (tab->atoms[i].fun)
			tab->atoms[i].fun (ctx, a, m, tab->atoms[i].arg);
}

int op8len[] = { 1 };
int op16len[] = { 2 };
int op24len[] = { 3 };
int op32len[] = { 4 };
int op40len[] = { 5 };
int op64len[] = { 8 };
void atomopl APROTO {
	ctx->oplen = *(int*)v;
}

void atomname APROTO {
	if (ctx->out)
		fprintf (ctx->out, " %s%s", cgr, (char *)v);
}

void atomunk APROTO {
	if (ctx->out)
		fprintf (ctx->out, " %s%s", cred, (char *)v);
}

void atomimm APROTO {
	if (!ctx->out)
		return;
	const struct bitfield *bf = v;
	ull num = GETBF(bf);
	if (num & 1ull << 63)
		fprintf (ctx->out, " %s-%#llx", cyel, -num);
	else
		fprintf (ctx->out, " %s%#llx", cyel, num);
}

void atomctarg APROTO {
	const struct bitfield *bf = v;
	ull num = GETBF(bf);
	markct8(ctx, num);
	if (!ctx->out)
		return;
	fprintf (ctx->out, " %s%#llx", cbr, num);
}

void atombtarg APROTO {
	const struct bitfield *bf = v;
	ull num = GETBF(bf);
	markbt8(ctx, num);
	if (!ctx->out)
		return;
	fprintf (ctx->out, " %s%#llx", cmag, num);
}

void atomign APROTO {
	const int *n = v;
	(void)BF(n[0], n[1]);
}

// print reg if non-0, ignore otherwise. return 1 if printed anything.
static int printreg (struct disctx *ctx, ull *a, ull *m, const struct reg *reg) {
	ull num = 0;
	if (reg->bf)
		num = GETBF(reg->bf);
	if (reg->specials) {
		int i;
		for (i = 0; reg->specials[i].num != -1; i++) {
			if (num == reg->specials[i].num) {
				switch (reg->specials[i].mode) {
					case SR_NAMED:
						fprintf (ctx->out, "%s$%s", cbl, reg->specials[i].name);
						return 1;
					case SR_ZERO:
						return 0;
					case SR_ONE:
						fprintf (ctx->out, "%s1", cyel);
						return 1;
					case SR_DISCARD:
						fprintf (ctx->out, "%s#", cbl);
						return 1;
				}
			}
		}
	}
	const char *suf = "";
	if (reg->suffix)
		suf = reg->suffix;
	if (reg->hilo) {
		if (num & 1)
			suf = "h";
		else
			suf = "l";
		num >>= 1;
	}
	const char *color = cbl;
	if (reg->always_special)
		color = cred;
	if (reg->bf)
		fprintf (ctx->out, "%s$%s%d%s", color, reg->name, num, suf);
	else
		fprintf (ctx->out, "%s$%s%s", color, reg->name, suf);
	return 1;
}

void atomreg APROTO {
	if (!ctx->out)
		return;
	const struct reg *reg = v;
	fprintf (ctx->out, " ");
	if (!printreg(ctx, a, m, reg))
		fprintf (ctx->out, "%s0", cyel);
}

void atommem APROTO {
	if (!ctx->out)
		return;
	const struct mem *mem = v;
	int anything = 0;
	fprintf (ctx->out, " ");
	if (mem->name) {
		fprintf (ctx->out, "%s%s", ccy, mem->name);
		if (mem->idx)
			fprintf (ctx->out, "%d", GETBF(mem->idx));
		fprintf (ctx->out, "[");
	}
	if (mem->reg)
		anything = printreg(ctx, a, m, mem->reg);
	if (mem->imm) {
		ull imm = GETBF(mem->imm);
		if (imm) {
			if (anything)
				fprintf (ctx->out, "%s+", ccy);
			fprintf(ctx->out, "%s%#llx", cyel, imm);
			anything = 1;
		}
	}
	if (mem->reg2) {
		if (anything)
			fprintf (ctx->out, "%s+", ccy);
		if (!printreg(ctx, a, m, mem->reg2))
			fprintf (ctx->out, "%s0", cyel);
		if (mem->reg2shr)
			fprintf (ctx->out, "%s*%s%#llx", ccy, cyel, 1ull << mem->reg2shr);
		anything = 1;
	}
	if (!anything)
		fprintf (ctx->out, "%s0", cyel);
	if (mem->name)
		fprintf (ctx->out, "%s]", ccy);
}

ull getbf(const struct bitfield *bf, ull *a, ull *m, struct disctx *ctx) {
	ull res = 0;
	int pos = bf->shr;
	int i;
	for (i = 0; i < sizeof(bf->sbf) / sizeof(bf->sbf[0]); i++) {
		res |= BF(bf->sbf[i].pos, bf->sbf[i].len) << pos;
		pos += bf->sbf[i].len;
	}
	switch (bf->mode) {
		case BF_UNSIGNED:
			break;
		case BF_SIGNED:
			if (res & 1ull << (pos - 1))
				res -= 1ull << pos;
			break;
		case BF_SLIGHTLY_SIGNED:
			if (res & 1ull << (pos - 1) && res & 1ull << (pos - 2))
				res -= 1ull << pos;
			break;
		case BF_ULTRASIGNED:
			res -= 1ull << pos;
			break;
		case BF_LUT:
			res = bf->lut[res];
			break;
	}
	if (bf->pcrel) {
		// <3 xtensa.
		res += (ctx->pos / ctx->isa->posunit) & -(1ull << bf->shr);
	}
	res += bf->addend;
	return res;
}

uint32_t readle32 (uint8_t *p) {
	return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

uint16_t readle16 (uint8_t *p) {
	return p[0] | p[1] << 8;
}

void markct8(struct disctx *ctx, uint32_t ptr) {
	ptr *= ctx->isa->posunit;
	if (ptr < ctx->codebase || ptr > ctx->codebase + ctx->codesz)
		return;
	ctx->labels[ptr - ctx->codebase] |= 2;
}

void markbt8(struct disctx *ctx, uint32_t ptr) {
	ptr *= ctx->isa->posunit;
	if (ptr < ctx->codebase || ptr > ctx->codebase + ctx->codesz)
		return;
	ctx->labels[ptr - ctx->codebase] |= 1;
}

/*
 * Disassembler driver
 *
 * You pass a block of memory to this function, disassembly goes out to given
 * FILE*.
 */

void envydis (struct disisa *isa, FILE *out, uint8_t *code, uint32_t start, int num, int ptype)
{
	struct disctx c = { 0 };
	struct disctx *ctx = &c;
	int cur = 0, i, j;
	start *= isa->posunit;
	ctx->code8 = code;
	ctx->labels = calloc(num, sizeof *ctx->labels);
	ctx->codebase = start;
	ctx->codesz = num;
	ctx->ptype = ptype;
	ctx->isa = isa;
	while (cur < num) {
		ull a = 0, m = 0;
		for (i = 0; i < 8 && cur + i < num; i++) {
			a |= (ull)code[cur + i] << i*8;
		}
		ctx->pos = cur + start;
		atomtab (ctx, &a, &m, isa->troot);
		if (ctx->oplen)
			cur += ctx->oplen;
		else
			cur++;
	}
	cur = 0;
	while (cur < num) {
		ull a = 0, m = 0;
		ctx->out = 0;
		for (i = 0; i < 8 && cur + i < num; i++) {
			a |= (ull)code[cur + i] << i*8;
		}
		ctx->oplen = 0;
		ctx->pos = cur + start;
		atomtab (ctx, &a, &m, isa->troot);
		ctx->out = out;

		if (ctx->labels[cur] & 2)
			fprintf (ctx->out, "\n");
		switch (ctx->labels[cur] & 3) {
			case 0:
				fprintf (ctx->out, "%s%08x:%s", cgray, (cur + start) / isa->posunit, cnorm);
				break;
			case 1:
				fprintf (ctx->out, "%s%08x:%s", cmag, (cur + start) / isa->posunit, cnorm);
				break;
			case 2:
				fprintf (ctx->out, "%s%08x:%s", cbr, (cur + start) / isa->posunit, cnorm);
				break;
			case 3:
				fprintf (ctx->out, "%s%08x:%s", cbrmag, (cur + start) / isa->posunit, cnorm);
				break;
		}

		for (i = 0; i < isa->maxoplen; i += isa->opunit) {
			fprintf (ctx->out, " ");
			for (j = isa->opunit - 1; j >= 0; j--)
				if (i+j && i+j >= ctx->oplen)
					fprintf (ctx->out, "  ");
				else if (cur+i+j >= num)
					fprintf (ctx->out, "%s??", cred);
				else
					fprintf (ctx->out, "%s%02x", cnorm, code[cur + i + j]);
		}
		fprintf (ctx->out, "  ");

		if (ctx->labels[cur] & 2)
			fprintf (ctx->out, "%sC", cbr);
		else
			fprintf (ctx->out, " ");
		if (ctx->labels[cur] & 1)
			fprintf (ctx->out, "%sB", cmag);
		else
			fprintf (ctx->out, " ");

		atomtab (ctx, &a, &m, isa->troot);

		if (ctx->oplen) {
			a &= ~m;
			if (ctx->oplen < 8)
				a &= (1ull << ctx->oplen * 8) - 1;
			if (a) {
				fprintf (ctx->out, " %s[unknown:", cred);
				for (i = 0; i < ctx->oplen || i == 0; i += isa->opunit) {
					fprintf (ctx->out, " ");
					for (j = isa->opunit - 1; j >= 0; j--)
						if (cur+i+j >= num)
							fprintf (ctx->out, "??");
						else
							fprintf (ctx->out, "%02llx", (a >> (i + j) * 8) & 0xff);
				}
				fprintf (ctx->out, "]");
			}
			if (cur + ctx->oplen > num) {
				fprintf (ctx->out, " %s[incomplete]%s", cred, cnorm);
			}
			cur += ctx->oplen;
		} else {
			fprintf (ctx->out, " %s[unknown op length]%s", cred, cnorm);
			cur++;
		}
		fprintf (ctx->out, "%s\n", cnorm);
	}
	free(ctx->labels);
}
