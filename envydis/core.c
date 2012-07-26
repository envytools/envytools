/*
 * Copyright (C) 2009-2011 Marcin Kościelnicki <koriakin@0x04.net>
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
#include "dis-intern.h"
#include <stdarg.h>

int var_ok(int fmask, int ptype, struct varinfo *varinfo) {
	return (!fmask || (varinfo->fmask[0] & fmask) == fmask) && (!ptype || (varinfo->modes[0] != -1 && ptype & 1 << varinfo->modes[0]));
}

char *aprint(const char *format, ...) {
	va_list va;
	va_start(va, format);
	size_t sz = vsnprintf(0, 0, format, va);
	va_end(va);
	char *res = malloc(sz + 1);
	va_start(va, format);
	vsnprintf(res, sz + 1, format, va);
	va_end(va);
	return res;
}

void mark(struct disctx *ctx, uint32_t ptr, int m) {
	if (ptr < ctx->codebase || ptr >= ctx->codebase + ctx->codesz / ctx->isa->posunit)
		return;
	ctx->marks[ptr - ctx->codebase] |= m;
}

int is_nr_mark(struct disctx *ctx, uint32_t ptr) {
	if (ptr < ctx->codebase || ptr >= ctx->codebase + ctx->codesz / ctx->isa->posunit)
		return 0;
	return ctx->marks[ptr - ctx->codebase] & 0x40;
}

struct matches *emptymatches() {
	struct matches *res = calloc(sizeof *res, 1);
	return res;
}

struct matches *alwaysmatches(int lpos) {
	struct matches *res = calloc(sizeof *res, 1);
	struct match m = { .lpos = lpos };
	ADDARRAY(res->m, m);
	return res;
}

struct matches *catmatches(struct matches *a, struct matches *b) {
	int i;
	for (i = 0; i < b->mnum; i++)
		ADDARRAY(a->m, b->m[i]);
	free(b->m);
	free(b);
	return a;
}

struct matches *mergematches(struct match a, struct matches *b) {
	struct matches *res = emptymatches();
	int i, j;
	for (i = 0; i < b->mnum; i++) {
		for (j = 0; j < MAXOPLEN; j++) {
			ull cmask = a.m[j] & b->m[i].m[j];
			if ((a.a[j] & cmask) != (b->m[i].a[j] & cmask))
				break;
		}
		if (j == MAXOPLEN) {
			struct match nm = b->m[i];
			if (!nm.oplen)
				nm.oplen = a.oplen;
			for (j = 0; j < MAXOPLEN; j++) {
				nm.a[j] |= a.a[j];
				nm.m[j] |= a.m[j];
			}
			int j;
			assert (a.nrelocs + nm.nrelocs <= 8);
			for (j = 0; j < a.nrelocs; j++)
				nm.relocs[nm.nrelocs + j] = a.relocs[j];
			nm.nrelocs += a.nrelocs;
			ADDARRAY(res->m, nm);
		}
	}
	free(b->m);
	free(b);
	return res;
}

struct matches *tabdesc (struct disctx *ctx, struct match m, const struct atom *atoms) {
	if (!atoms->fun) {
		struct matches *res = emptymatches();
		ADDARRAY(res->m, m);
		return res;
	}
	struct matches *ms = atoms->fun(ctx, 0, 0, atoms->arg, m.lpos);
	if (!ms)
		return 0;
	ms = mergematches(m, ms);
	atoms++;
	if (!atoms->fun)
		return ms;
	struct matches *res = emptymatches();
	int i;
	for (i = 0; i < ms->mnum; i++) {
		struct matches *tmp = tabdesc(ctx, ms->m[i], atoms);
		if (tmp)
			res = catmatches(res, tmp);
	}
	free(ms->m);
	free(ms);
	return res;
}

struct matches *atomtab APROTO {
	const struct insn *tab = v;
	if (!ctx->reverse) {
		int i;
		while ((a[0]&tab->mask) != tab->val || !var_ok(tab->fmask, tab->ptype, ctx->varinfo))
			tab++;
		m[0] |= tab->mask;
		for (i = 0; i < 16; i++)
			if (tab->atoms[i].fun)
				tab->atoms[i].fun (ctx, a, m, tab->atoms[i].arg, 0);
		return NULL;
	} else {
		struct matches *res = emptymatches();
		int i;
		for (i = 0; ; i++) {
			if (var_ok(tab[i].fmask, tab[i].ptype, ctx->varinfo)) {
				struct match sm = { 0, .a = {tab[i].val}, .m = {tab[i].mask}, .lpos = spos };
				struct matches *subm = tabdesc(ctx, sm, tab[i].atoms); 
				if (subm)
					res = catmatches(res, subm);
			}
			if (!tab[i].mask && !tab[i].fmask && !tab[i].ptype) break;
		}
		return res;
	}
}

int op8len[] = { 1 };
int op16len[] = { 2 };
int op24len[] = { 3 };
int op32len[] = { 4 };
int op40len[] = { 5 };
int op64len[] = { 8 };
struct matches *atomopl APROTO {
	if (!ctx->reverse) {
		ctx->oplen = *(int*)v;
		return NULL;
	} else {
		struct matches *res = alwaysmatches(spos);
		res->m[0].oplen = *(int*)v;
		return res;
	}
}

struct matches *atomendmark APROTO {
	if (!ctx->reverse) {
		ctx->endmark = 1;
		return NULL;
	} else {
		struct matches *res = alwaysmatches(spos);
		return res;
	}
}

struct matches *atomsestart APROTO {
	if (!ctx->reverse) {
		struct litem *li = calloc(sizeof *li, 1);
		li->type = LITEM_SESTART;
		ADDARRAY(ctx->atoms, li);
		return NULL;
	} else {
		struct litem *li = ctx->line->atoms[spos];
		if (li->type == LITEM_SESTART)
			return alwaysmatches(spos+1);
		else
			return 0;
	}
}

struct matches *atomseend APROTO {
	if (!ctx->reverse) {
		struct litem *li = calloc(sizeof *li, 1);
		li->type = LITEM_SEEND;
		ADDARRAY(ctx->atoms, li);
		return NULL;
	} else {
		struct litem *li = ctx->line->atoms[spos];
		if (li->type == LITEM_SEEND)
			return alwaysmatches(spos+1);
		else
			return 0;
	}
}

struct matches *atomname APROTO {
	if (!ctx->reverse) {
		struct litem *li = calloc(sizeof *li, 1);
		li->type = LITEM_NAME;
		li->str = strdup(v);
		ADDARRAY(ctx->atoms, li);
		return NULL;
	} else {
		if (spos == ctx->line->atomsnum)
			return 0;
		struct litem *li = ctx->line->atoms[spos];
		if (li->type == LITEM_NAME && !strcmp(li->str, v))
			return alwaysmatches(spos+1);
		else
			return 0;
	}
}

struct matches *atomcmd APROTO {
	if (!ctx->reverse) {
		struct litem *li = makeli(easm_expr_str(EASM_EXPR_LABEL, strdup(v)));
		ADDARRAY(ctx->atoms, li);
		return NULL;
	} else {
		if (spos == ctx->line->atomsnum || ctx->line->atoms[spos]->type != LITEM_EXPR)
			return 0;
		struct easm_expr *e = ctx->line->atoms[spos]->expr;
		if (e->type == EASM_EXPR_LABEL && !strcmp(e->str, v))
			return alwaysmatches(spos+1);
		else
			return 0;
	}
}

struct matches *atomunk APROTO {
	if (ctx->reverse)
		return 0;
	struct litem *li = calloc(sizeof *li, 1);
	li->type = LITEM_UNK;
	li->str = strdup(v);
	ADDARRAY(ctx->atoms, li);
	return NULL;
}

int setsbf (struct match *res, int pos, int len, ull num) {
	if (!len)
		return 1;
	int idx = pos / 0x40;
	pos %= 0x40;
	ull m = ((1ull << len) - 1) << pos;
	ull a = (num << pos) & m;
	if ((a & m & res->m[idx]) == (res->a[idx] & m & res->m[idx])) {
		res->a[idx] |= a;
		res->m[idx] |= m;
	} else {
		return 0;
	}
	if (pos + len > 0x40) {
		ull m1 = (((1ull << len) - 1) >> (0x40 - pos));
		ull a1 = (num >> (0x40 - pos)) & m1;
		if ((a1 & m1 & res->m[idx+1]) == (res->a[idx+1] & m1 & res->m[idx+1])) {
			res->a[idx+1] |= a1;
			res->m[idx+1] |= m1;
		} else {
			return 0;
		}
	}
	return 1;
}

int setbfe (struct match *res, const struct bitfield *bf, struct easm_expr *expr) {
	if (!easm_isimm(expr))
		return 0;
	if (expr->type != EASM_EXPR_NUM || bf->pcrel) {
		assert (res->nrelocs != 8);
		res->relocs[res->nrelocs].bf = bf;
		res->relocs[res->nrelocs].expr = expr;
		res->nrelocs++;
		return 1;
	}
	ull num = (expr->num - bf->addend) ^ bf->xorend;
	if (bf->lut) {
		int max = 1 << (bf->sbf[0].len + bf->sbf[1].len);
		int j = 0;
		for (j = 0; j < max; j++)
			if (bf->lut[j] == num)
				break;
		if (j == max)
			return 0;
		num = j;
	}
	num >>= bf->shr;
	setsbf(res, bf->sbf[0].pos, bf->sbf[0].len, num);
	num >>= bf->sbf[0].len;
	setsbf(res, bf->sbf[1].pos, bf->sbf[1].len, num);
	ull mask = ~0ull;
	ull totalsz = bf->shr + bf->sbf[0].len + bf->sbf[1].len;
	if (bf->wrapok && totalsz < 64)
		mask = (1ull << totalsz) - 1;
	return (getbf(bf, res->a, res->m, 0) & mask) == (expr->num & mask);
}

int setbf (struct match *res, const struct bitfield *bf, ull num) {
	struct easm_expr *e = easm_expr_num(EASM_EXPR_NUM, num);
	return setbfe(res, bf, e);
}

struct matches *matchimm (struct disctx *ctx, const struct bitfield *bf, int spos) {
	if (spos == ctx->line->atomsnum || ctx->line->atoms[spos]->type != LITEM_EXPR)
		return 0;
	struct matches *res = alwaysmatches(spos+1);
	if (setbfe(res->m, bf, ctx->line->atoms[spos]->expr))
		return res;
	else {
		free(res->m);
		free(res);
		return 0;
	}
}

struct matches *atomimm APROTO {
	const struct bitfield *bf = v;
	if (ctx->reverse)
		return matchimm(ctx, bf, spos);
	struct easm_expr *expr = easm_expr_num(EASM_EXPR_NUM, GETBF(bf));
	ADDARRAY(ctx->atoms, makeli(expr));
	return NULL;
}

struct matches *atomctarg APROTO {
	const struct bitfield *bf = v;
	if (ctx->reverse)
		return matchimm(ctx, bf, spos);
	struct easm_expr *expr = easm_expr_num(EASM_EXPR_NUM, GETBF(bf));
	mark(ctx, expr->num, 2);
	if (is_nr_mark(ctx, expr->num))
		ctx->endmark = 1;
	expr->special = EASM_SPEC_CTARG;
	ADDARRAY(ctx->atoms, makeli(expr));
	int i;
	for (i = 0; i < ctx->labelsnum; i++) {
		if (ctx->labels[i].val == expr->num && ctx->labels[i].name) {
			struct easm_expr *expr = easm_expr_str(EASM_EXPR_LABEL, strdup(ctx->labels[i].name));
			expr->special = EASM_SPEC_CTARG;
			ADDARRAY(ctx->atoms, makeli(expr));
			return NULL;
		}
	}
	return NULL;
}

struct matches *atombtarg APROTO {
	const struct bitfield *bf = v;
	if (ctx->reverse)
		return matchimm(ctx, bf, spos);
	struct easm_expr *expr = easm_expr_num(EASM_EXPR_NUM, GETBF(bf));
	mark(ctx, expr->num, 1);
	expr->special = EASM_SPEC_BTARG;
	ADDARRAY(ctx->atoms, makeli(expr));
	int i;
	for (i = 0; i < ctx->labelsnum; i++) {
		if (ctx->labels[i].val == expr->num && ctx->labels[i].name) {
			struct easm_expr *expr = easm_expr_str(EASM_EXPR_LABEL, strdup(ctx->labels[i].name));
			expr->special = EASM_SPEC_BTARG;
			ADDARRAY(ctx->atoms, makeli(expr));
			return NULL;
		}
	}
	return NULL;
}

struct matches *atomign APROTO {
	if (ctx->reverse)
		return alwaysmatches(spos);
	const int *n = v;
	(void)BF(n[0], n[1]);
	return NULL;
}

int matchreg (struct match *res, const struct reg *reg, const struct easm_expr *expr, struct disctx *ctx) {
	if (reg->specials) {
		int i = 0;
		for (i = 0; reg->specials[i].num != -1; i++) {
			if (!var_ok(reg->specials[i].fmask, 0, ctx->varinfo))
				continue;
			switch (reg->specials[i].mode) {
				case SR_NAMED:
					if (expr->type == EASM_EXPR_REG && !strcmp(expr->str, reg->specials[i].name))
						return setbf(res, reg->bf, reg->specials[i].num);
					break;
				case SR_ZERO:
					if (expr->type == EASM_EXPR_NUM && expr->num == 0)
						return setbf(res, reg->bf, reg->specials[i].num);
					break;
				case SR_ONE:
					if (expr->type == EASM_EXPR_NUM && expr->num == 1)
						return setbf(res, reg->bf, reg->specials[i].num);
					break;
				case SR_DISCARD:
					if (expr->type == EASM_EXPR_DISCARD)
						return setbf(res, reg->bf, reg->specials[i].num);
					break;
			}
		}
	}
	if (expr->type != EASM_EXPR_REG)
		return 0;
	if (strncmp(expr->str, reg->name, strlen(reg->name)))
		return 0;
	const char *str = expr->str + strlen(reg->name);
	char *end;
	ull num = strtoull(str, &end, 10);
	if (!reg->hilo) {
		if (strcmp(end, (reg->suffix?reg->suffix:"")))
			return 0;
	} else {
		if (strlen(end) != 1)
			return 0;
		if (*end != 'h' && *end != 'l')
			return 0;
		num <<= 1;
		if (*end == 'h')
			num |= 1;
	}
	if (reg->bf)
		return setbf(res, reg->bf, num);
	else
		return !num;
}

int matchshreg (struct match *res, const struct reg *reg, const struct easm_expr *expr, int shl, struct disctx *ctx) {
	ull sh = 0;
	while (1) {
		if (expr->type == EASM_EXPR_SHL && expr->e2->type == EASM_EXPR_NUM) {
			sh += expr->e2->num;
			expr = expr->e1;
		} else if (expr->type == EASM_EXPR_MUL && expr->e2->type == EASM_EXPR_NUM) {
			ull num = expr->e2->num;
			if (!num || (num & (num-1)))
				return 0;
			while (num != 1)
				num >>= 1, sh++;
			expr = expr->e1;
		} else if (expr->type == EASM_EXPR_MUL && expr->e1->type == EASM_EXPR_NUM) {
			ull num = expr->e1->num;
			if (!num || (num & (num-1)))
				return 0;
			while (num != 1)
				num >>= 1, sh++;
			expr = expr->e2;
		} else {
			break;
		}
	}
	if (sh != shl)
		return 0;
	return matchreg(res, reg, expr, ctx);
}

// print reg if non-0, ignore otherwise. return 1 if printed anything.
static struct easm_expr *printreg (struct disctx *ctx, ull *a, ull *m, const struct reg *reg) {
	struct easm_expr *expr;
	ull num = 0;
	if (reg->bf)
		num = GETBF(reg->bf);
	if (reg->specials) {
		int i;
		for (i = 0; reg->specials[i].num != -1; i++) {
			if (!var_ok(reg->specials[i].fmask, 0, ctx->varinfo))
				continue;
			if (num == reg->specials[i].num) {
				switch (reg->specials[i].mode) {
					case SR_NAMED:
						expr = easm_expr_str(EASM_EXPR_REG, strdup(reg->specials[i].name));
						expr->special = EASM_SPEC_REGSP;
						return expr;
					case SR_ZERO:
						return 0;
					case SR_ONE:
						return easm_expr_num(EASM_EXPR_NUM, 1);
					case SR_DISCARD:
						return easm_expr_discard();
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
	char *str;
	if (reg->bf)
		str = aprint("%s%lld%s", reg->name, num, suf);
	else
		str = aprint("%s%s", reg->name, suf);
	expr = easm_expr_str(EASM_EXPR_REG, str);
	if (reg->cool)
		expr->special = EASM_SPEC_REGSP;
	if (reg->always_special)
		expr->special = EASM_SPEC_ERR;
	return expr;
}

struct matches *atomreg APROTO {
	const struct reg *reg = v;
	if (ctx->reverse) {
		if (spos == ctx->line->atomsnum || ctx->line->atoms[spos]->type != LITEM_EXPR)
			return 0;
		struct easm_expr *e = ctx->line->atoms[spos]->expr;
		struct matches *res = alwaysmatches(spos+1);
		if (matchreg(res->m, reg, e, ctx))
			return res;
		else {
			free(res->m);
			free(res);
			return 0;
		}
	}
	struct easm_expr *expr = printreg(ctx, a, m, reg);
	if (!expr) expr = easm_expr_num(EASM_EXPR_NUM, 0);
	ADDARRAY(ctx->atoms, makeli(expr));
	return NULL;
}

struct matches *atomdiscard APROTO {
	if (ctx->reverse) {
		if (spos == ctx->line->atomsnum || ctx->line->atoms[spos]->type != LITEM_EXPR)
			return 0;
		struct easm_expr *e = ctx->line->atoms[spos]->expr;
		if (e->type == EASM_EXPR_DISCARD) {
			return alwaysmatches(spos+1);
		} else {
			return 0;
		}
	}
	struct easm_expr *expr = easm_expr_discard();
	ADDARRAY(ctx->atoms, makeli(expr));
	return NULL;
}

int addexpr (struct easm_expr **iex, struct easm_expr *expr, int flip) {
	if (flip) {
		if (!*iex)
			*iex = easm_expr_un(EASM_EXPR_NEG, expr);
		else
			*iex = easm_expr_bin(EASM_EXPR_SUB, *iex, expr);
	} else {
		if (!*iex)
			*iex = expr;
		else
			*iex = easm_expr_bin(EASM_EXPR_ADD, *iex, expr);
	}
	return 1;
}

int matchmemaddr(struct easm_expr **iex, struct easm_expr **niex1, struct easm_expr **niex2, struct easm_expr *expr, int flip) {
	if (easm_isimm(expr))
		return addexpr(iex, expr, flip);
	if (expr->type == EASM_EXPR_ADD)
		return matchmemaddr(iex, niex1, niex2, expr->e1, flip) && matchmemaddr(iex, niex1, niex2, expr->e2, flip);
	if (expr->type == EASM_EXPR_SUB)
		return matchmemaddr(iex, niex1, niex2, expr->e1, flip) && matchmemaddr(iex, niex1, niex2, expr->e2, !flip);
	if (expr->type == EASM_EXPR_NEG)
		return matchmemaddr(iex, niex1, niex2, expr->e1, !flip);
	if (flip)
		return 0;
	if (niex1 && !*niex1)
		*niex1 = expr;
	else if (niex2 && !*niex2)
		*niex2 = expr;
	else
		return 0;
	return 1;
}

struct matches *atommem APROTO {
	const struct mem *mem = v;
	if (!ctx->reverse) {
		int type = EASM_EXPR_MEM;
		struct easm_expr *expr = 0;
		struct easm_expr *pexpr = 0;
		if (mem->reg)
			expr = printreg(ctx, a, m, mem->reg);
		if (mem->imm) {
			ull imm = GETBF(mem->imm);
			if (mem->postincr) {
				pexpr = easm_expr_num(EASM_EXPR_NUM, 0);
				if (imm & 1ull << 63) {
					pexpr->num = -imm;
					type = EASM_EXPR_MEMMM;
				} else {
					pexpr->num = imm;
					type = EASM_EXPR_MEMPP;
				}
			} else if (imm) {
				struct easm_expr *sexpr = easm_expr_num(EASM_EXPR_NUM, 0);
				if (expr) {
					if (imm & 1ull << 63) {
						sexpr->num = -imm;
						expr = easm_expr_bin(EASM_EXPR_SUB, expr, sexpr);
					} else {
						sexpr->num = imm;
						expr = easm_expr_bin(EASM_EXPR_ADD, expr, sexpr);
					}
				} else {
					sexpr->num = imm;
					expr = sexpr;
				}
			}
		}
		if (mem->reg2) {
			struct easm_expr *sexpr = printreg(ctx, a, m, mem->reg2);
			if (sexpr) {
				if (mem->reg2shr) {
					uint64_t num = 1ull << mem->reg2shr;
					struct easm_expr *ssexpr = easm_expr_num(EASM_EXPR_NUM, num);
					sexpr = easm_expr_bin(EASM_EXPR_MUL, sexpr, ssexpr);
				}
				if (expr)
					expr = easm_expr_bin(EASM_EXPR_ADD, expr, sexpr);
				else
					expr = sexpr;
			}
		}
		if (!expr) expr = easm_expr_num(EASM_EXPR_NUM, 0);
		struct easm_expr *oexpr = expr;
		if (mem->name) {
			struct easm_expr *nex;
			if (pexpr)
				nex = easm_expr_bin(type, expr, pexpr);
			else
				nex = easm_expr_un(type, expr);
			if (mem->idx)
				nex->str = aprint("%s%lld", nex->str, GETBF(mem->idx));
			else
				nex->str = strdup(mem->name);
			nex->mods = calloc(sizeof *nex->mods, 1);
			expr = nex;
		} else if (type != EASM_EXPR_MEM) {
			abort();
		}
		ADDARRAY(ctx->atoms, makeli(expr));
		if (mem->literal && !pexpr && oexpr->type == EASM_EXPR_NUM) {
			ull ptr = oexpr->num;
			mark(ctx, ptr, 0x10);
			if (ptr < ctx->codebase || ptr > ctx->codebase + ctx->codesz)
				return 0;
			uint32_t num = 0;
			int j;
			for (j = 0; j < 4; j++)
				num |= ctx->code[(ptr - ctx->codebase) * ctx->isa->posunit + j] << j*8;
			expr = easm_expr_num(EASM_EXPR_NUM, num);
			expr->special = EASM_SPEC_MEM;
			ADDARRAY(ctx->atoms, makeli(expr));
		}
		return NULL;
	} else {
		if (spos == ctx->line->atomsnum || ctx->line->atoms[spos]->type != LITEM_EXPR)
			return 0;
		struct easm_expr *expr = ctx->line->atoms[spos]->expr;
		struct easm_expr *pexpr = 0;
		struct match res = { 0, .lpos = spos+1 };
		int ismem = expr->type >= EASM_EXPR_MEM && expr->type <= EASM_EXPR_MEMME;
		if (ismem && !mem->name)
			return 0;
		if (!ismem && mem->name)
			return 0;
		if (ismem) {
			if (strncmp(expr->str, mem->name, strlen(mem->name)))
				return 0;
			if (mem->idx) {
				const char *str = expr->str + strlen(mem->name);
				if (!*str)
					return 0;
				char *end;
				ull num = strtoull(str, &end, 10);
				if (*end)
					return 0;
				if (!setbf(&res, mem->idx, num))
					return 0;
			} else {
				if (strlen(expr->str) != strlen(mem->name))
					return 0;
			}
			if (expr->type == EASM_EXPR_MEMPP)
				addexpr(&pexpr, expr->e2, 0);
			else if (expr->type == EASM_EXPR_MEMMM)
				addexpr(&pexpr, expr->e2, 1);
			else if (expr->type != EASM_EXPR_MEM)
				return 0;
			expr = expr->e1;
		}
		struct easm_expr *iex = 0;
		struct easm_expr *niex1 = 0;
		struct easm_expr *niex2 = 0;
		matchmemaddr(&iex, &niex1, &niex2, expr, 0);
		if (mem->imm) {
			if (mem->postincr) {
				if (!pexpr || iex)
					return 0;
				if (!setbfe(&res, mem->imm, pexpr))
					return 0;
			} else {
				if (pexpr)
					return 0;
				if (iex) {
					if (!setbfe(&res, mem->imm, iex))
						return 0;
				} else {
					if (!setbf(&res, mem->imm, 0))
						return 0;
				}
			}
		} else {
			if (iex || pexpr)
				return 0;
		}
		if (mem->reg && mem->reg2) {
			if (!niex1)
				niex1 = easm_expr_num(EASM_EXPR_NUM, 0);
			if (!niex2)
				niex2 = easm_expr_num(EASM_EXPR_NUM, 0);
			struct match sres = res;
			if (!matchreg(&res, mem->reg, niex1, ctx) || !matchshreg(&res, mem->reg2, niex2, mem->reg2shr, ctx)) {
				res = sres;
				if (!matchreg(&res, mem->reg, niex2, ctx) || !matchshreg(&res, mem->reg2, niex1, mem->reg2shr, ctx))
					return 0;
			}
		} else if (mem->reg) {
			if (niex2)
				return 0;
			if (!niex1)
				niex1 = easm_expr_num(EASM_EXPR_NUM, 0);
			if (!matchreg(&res, mem->reg, niex1, ctx))
				return 0;
		} else if (mem->reg2) {
			if (niex2)
				return 0;
			if (!niex1)
				niex1 = easm_expr_num(EASM_EXPR_NUM, 0);
			if (!matchshreg(&res, mem->reg2, niex1, mem->reg2shr, ctx))
				return 0;
		} else {
			if (niex1 || niex2)
				return 0;
		}
		struct matches *rres = emptymatches();
		ADDARRAY(rres->m, res);
		return rres;
	}
}

struct matches *atomvec APROTO {
	const struct vec *vec = v;
	if (!ctx->reverse) {
		struct easm_expr *expr = 0;
		ull base = GETBF(vec->bf);
		ull cnt = GETBF(vec->cnt);
		ull mask = -1ull;
		if (vec->mask)
			mask = GETBF(vec->mask);
		int k = 0, i;
		for (i = 0; i < cnt; i++) {
			struct easm_expr *sexpr;
			if (mask & 1ull<<i) {
				char *name = aprint("%s%lld", vec->name,  base + k++);
				sexpr = easm_expr_str(EASM_EXPR_REG, name);
				if (vec->cool)
					sexpr->special = EASM_SPEC_REGSP;
			} else {
				sexpr = easm_expr_discard();
			}
			if (expr)
				expr = easm_expr_bin(EASM_EXPR_VEC, expr, sexpr);
			else
				expr = sexpr;
		}
		ADDARRAY(ctx->atoms, makeli(expr));
		return NULL;
	} else {
		if (spos == ctx->line->atomsnum || ctx->line->atoms[spos]->type != LITEM_EXPR)
			return 0;
		struct match res = { 0, .lpos = spos+1 };
		const struct easm_expr *expr = ctx->line->atoms[spos]->expr;
		const struct easm_expr **vexprs = 0;
		int vexprsnum = 0;
		int vexprsmax = 0;
		while (expr->type == EASM_EXPR_VEC) {
			ADDARRAY(vexprs, expr->e2);
			expr = expr->e1;
		}
		ADDARRAY(vexprs, expr);
		int i;
		for (i = 0; i < vexprsnum/2; i++) {
			const struct easm_expr *tmp = vexprs[i];
			vexprs[i] = vexprs[vexprsnum-1-i];
			vexprs[vexprsnum-1-i] = tmp;
		}
		ull start = 0, cnt = vexprsnum, mask = 0, cur = 0;
		for (i = 0; i < cnt; i++) {
			const struct easm_expr *e = vexprs[i];
			if (e->type == EASM_EXPR_DISCARD) {
			} else if (e->type == EASM_EXPR_REG) {
				if (strncmp(e->str, vec->name, strlen(vec->name)))
					return 0;
				char *end;
				ull num = strtoull(e->str + strlen(vec->name), &end, 10);
				if (*end)
					return 0;
				if (mask) {
					if (num != cur)
						return 0;
					cur++;
				} else {
					start = num;
					cur = num + 1;
				}
				mask |= 1ull << i;
			} else {
				return 0;
			}
		}
		if (!setbf(&res, vec->bf, start))
			return 0;
		if (!setbf(&res, vec->cnt, cnt))
			return 0;
		if (vec->mask) {
			if (!setbf(&res, vec->mask, mask))
				return 0;
		} else {
			if (mask != (1ull << cnt) - 1)
			       return 0;	
		}
		struct matches *rres = emptymatches();
		ADDARRAY(rres->m, res);
		return rres;
	}
}

struct matches *atombf APROTO {
	const struct bitfield *bf = v;
	if (!ctx->reverse) {
		uint64_t num1 = GETBF(&bf[0]);
		uint64_t num2 = num1 + GETBF(&bf[1]);
		struct easm_expr *expr = easm_expr_bin(EASM_EXPR_VEC,
				easm_expr_num(EASM_EXPR_NUM, num1),
				easm_expr_num(EASM_EXPR_NUM, num2));
		ADDARRAY(ctx->atoms, makeli(expr));
		return NULL;
	} else {
		if (spos == ctx->line->atomsnum || ctx->line->atoms[spos]->type != LITEM_EXPR)
			return 0;
		struct match res = { 0, .lpos = spos+1 };
		const struct easm_expr *expr = ctx->line->atoms[spos]->expr;
		if (expr->type != EASM_EXPR_VEC || expr->e1->type != EASM_EXPR_NUM || expr->e2->type != EASM_EXPR_NUM)
			return 0;
		uint64_t a = expr->e1->num;
		uint64_t b = expr->e2->num - a;
		if (!setbf(&res, &bf[0], a))
			return 0;
		if (!setbf(&res, &bf[1], b))
			return 0;
		struct matches *rres = emptymatches();
		ADDARRAY(rres->m, res);
		return rres;
	}
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
		res += (ctx->pos + bf->pospreadd) & -(1ull << bf->shr);
	}
	res ^= bf->xorend;
	res += bf->addend;
	return res;
}

/*
 * Disassembler driver
 *
 * You pass a block of memory to this function, disassembly goes out to given
 * FILE*.
 */

void envydis (const struct disisa *isa, FILE *out, uint8_t *code, uint32_t start, int num, struct varinfo *varinfo, int quiet, struct label *labels, int labelsnum, const struct envy_colors *cols)
{
	struct disctx c = { 0 };
	struct disctx *ctx = &c;
	int cur = 0, i, j;
	ctx->code = code;
	ctx->marks = calloc((num + isa->posunit - 1) / isa->posunit, sizeof *ctx->marks);
	ctx->names = calloc((num + isa->posunit - 1) / isa->posunit, sizeof *ctx->names);
	ctx->codebase = start;
	ctx->codesz = num;
	ctx->varinfo = varinfo;
	ctx->isa = isa;
	ctx->labels = labels;
	ctx->labelsnum = labelsnum;
	if (labels) {
		for (i = 0; i < labelsnum; i++) {
			mark(ctx, labels[i].val, labels[i].type);
			if (labels[i].val >= ctx->codebase && labels[i].val < ctx->codebase + ctx->codesz) {
				if (labels[i].name)
					ctx->names[labels[i].val - ctx->codebase] = labels[i].name;
			}
			if (labels[i].size) {
				for (j = 0; j < labels[i].size; j+=4)
					mark(ctx, labels[i].val + j, labels[i].type);
			}
		}
		int done;
		do {
			done = 1;
			cur = 0;
			int active = 0;
			while (cur < num) {
				if (!active && (ctx->marks[cur / isa->posunit] & 3) && !(ctx->marks[cur / isa->posunit] & 8)) {
					done = 0;
					active = 1;
					ctx->marks[cur / isa->posunit] |= 8;
				}
				if (active) {
					ull a[MAXOPLEN] = {0}, m[MAXOPLEN] = {0};
					for (i = 0; i < MAXOPLEN*8 && cur + i < num; i++) {
						a[i/8] |= (ull)code[cur + i] << (i&7)*8;
					}
					ctx->oplen = 0;
					ctx->pos = cur / isa->posunit + start;
					ctx->endmark = 0;
					atomtab (ctx, a, m, isa->troot, 0);
					if (ctx->oplen && !ctx->endmark && !(ctx->marks[cur / isa->posunit] & 4))
						cur += ctx->oplen;
					else
						active = 0;
				} else {
					cur++;
				}
			}
		} while (!done);
	} else {
		while (cur < num) {
			ull a[MAXOPLEN] = {0}, m[MAXOPLEN] = {0};
			for (i = 0; i < MAXOPLEN*8 && cur + i < num; i++) {
				a[i/8] |= (ull)code[cur + i] << (i&7)*8;
			}
			ctx->oplen = 0;
			ctx->pos = cur / isa->posunit + start;
			atomtab (ctx, a, m, isa->troot, 0);
			if (ctx->oplen)
				cur += ctx->oplen;
			else
				cur++;
		}
	}
	cur = 0;
	int active = 0;
	int skip = 0, nonzero = 0;
	while (cur < num) {
		int mark = (cur % isa->posunit ? 0 : ctx->marks[cur / isa->posunit]);
		if (!(cur % isa->posunit) && ctx->names[cur / isa->posunit]) {
			if (skip) {
				if (nonzero)
					fprintf(out, "%s[%x bytes skipped]\n", cols->err, skip);
				else
					fprintf(out, "%s[%x zero bytes skipped]\n", cols->reset, skip);
				skip = 0;
				nonzero = 0;
			}
			if (mark & 0x30)
				fprintf (out, "%s%s:\n", cols->reset, ctx->names[cur / isa->posunit]);
			else if (mark & 2)
				fprintf (out, "\n%s%s:\n", cols->ctarg, ctx->names[cur / isa->posunit]);
			else if (mark & 1)
				fprintf (out, "%s%s:\n", cols->btarg, ctx->names[cur / isa->posunit]);
			else
				fprintf (out, "%s%s:\n", cols->reset, ctx->names[cur / isa->posunit]);
		}
		if (mark & 0x30) {
			if (skip) {
				if (nonzero)
					fprintf(out, "%s[%x bytes skipped]\n", cols->err, skip);
				else
					fprintf(out, "%s[%x zero bytes skipped]\n", cols->reset, skip);
				skip = 0;
				nonzero = 0;
			}
			fprintf (out, "%s%08x:%s", cols->mem, cur / isa->posunit + start, cols->reset);
			if (mark & 0x10) {
				uint32_t val = 0;
				for (i = 0; i < 4 && cur + i < num; i++) {
					val |= code[cur + i] << i*8;
				}
				fprintf (out, " %s%08x\n", cols->num, val);
				cur += 4 / isa->posunit;
			} else {
				fprintf (out, " %s\"", cols->num);
				while (code[cur]) {
					switch (code[cur]) {
						case '\n':
							fprintf (out, "\\n");
							break;
						case '\\':
							fprintf (out, "\\\\");
							break;
						case '\"':
							fprintf (out, "\\\"");
							break;
						default:
							fprintf (out, "%c", code[cur]);
							break;
					}
					cur++;
				}
				cur++;
				fprintf (out, "\"\n");
			}
			continue;
		}
		if (!active && mark & 7)
			active = 1;
		if (!active && labels) {
			if (code[cur])
				nonzero = 1;
			cur++;
			skip++;
			continue;
		}
		if (skip) {
			if (nonzero)
				fprintf(out, "%s[%x bytes skipped]\n", cols->err, skip);
			else
				fprintf(out, "%s[%x zero bytes skipped]\n", cols->reset, skip);
			skip = 0;
			nonzero = 0;
		}
		ull a[MAXOPLEN] = {0}, m[MAXOPLEN] = {0};
		for (i = 0; i < MAXOPLEN*8 && cur + i < num; i++) {
			a[i/8] |= (ull)code[cur + i] << (i&7)*8;
		}
		ctx->oplen = 0;
		ctx->pos = cur / isa->posunit + start;
		ctx->atomsnum = 0;
		ctx->endmark = 0;
		atomtab (ctx, a, m, isa->troot, 0);

		if (ctx->endmark || mark & 4)
			active = 0;

		if (mark & 2 && !ctx->names[cur / isa->posunit])
			fprintf (out, "\n");
		switch (mark & 3) {
			case 0:
				if (!quiet)
					fprintf (out, "%s%08x:%s", cols->reset, cur / isa->posunit + start, cols->reset);
				break;
			case 1:
				fprintf (out, "%s%08x:%s", cols->btarg, cur / isa->posunit + start, cols->reset);
				break;
			case 2:
				fprintf (out, "%s%08x:%s", cols->ctarg, cur / isa->posunit + start, cols->reset);
				break;
			case 3:
				fprintf (out, "%s%08x:%s", cols->bctarg, cur / isa->posunit + start, cols->reset);
				break;
		}

		if (!quiet) {
			for (i = 0; i < isa->maxoplen; i += isa->opunit) {
				fprintf (out, " ");
				for (j = isa->opunit - 1; j >= 0; j--)
					if (i+j && i+j >= ctx->oplen)
						fprintf (out, "  ");
					else if (cur+i+j >= num)
						fprintf (out, "%s??", cols->err);
					else
						fprintf (out, "%s%02x", cols->reset, code[cur + i + j]);
			}
			fprintf (out, "  ");

			if (mark & 2)
				fprintf (out, "%sC", cols->ctarg);
			else
				fprintf (out, " ");
			if (mark & 1)
				fprintf (out, "%sB", cols->btarg);
			else
				fprintf (out, " ");
		} else if (quiet == 1) {
			if (mark)
				fprintf (out, "\n");
		}

		int noblank = 0;

		for (i = 0; i < ctx->atomsnum; i++) {
			if (ctx->atoms[i]->type == LITEM_SEEND)
				noblank = 1;
			if ((i || !quiet) && !noblank)
				fprintf (out, " ");
			switch(ctx->atoms[i]->type) {
				case LITEM_NAME:
					fprintf(out, "%s%s%s", cols->iname, ctx->atoms[i]->str, cols->reset);
					break;
				case LITEM_UNK:
					fprintf(out, "%s%s%s", cols->err, ctx->atoms[i]->str, cols->reset);
					break;
				case LITEM_SESTART:
					fprintf(out, "%s(%s", cols->sym, cols->reset);
					break;
				case LITEM_SEEND:
					fprintf(out, "%s)%s", cols->sym, cols->reset);
					break;
				case LITEM_EXPR:
					easm_print_sexpr(out, cols, ctx->atoms[i]->expr, -1);
					break;
			}
			noblank = (ctx->atoms[i]->type == LITEM_SESTART);
		}

		if (ctx->oplen) {
			int fl = 0;
			for (i = ctx->oplen; i < MAXOPLEN * 8; i++)
				a[i/8] &= ~(0xffull << (i & 7) * 8);
			for (i = 0; i < MAXOPLEN; i++) {
				a[i] &= ~m[i];
				if (a[i])
					fl = 1;
			}
			if (fl) {
				fprintf (out, " %s[unknown:", cols->err);
				for (i = 0; i < ctx->oplen || i == 0; i += isa->opunit) {
					fprintf (out, " ");
					for (j = isa->opunit - 1; j >= 0; j--)
						if (cur+i+j >= num)
							fprintf (out, "??");
						else
							fprintf (out, "%02llx", (a[(i+j)/8] >> ((i + j)&7) * 8) & 0xff);
				}
				fprintf (out, "]");
			}
			if (cur + ctx->oplen > num) {
				fprintf (out, " %s[incomplete]%s", cols->err, cols->reset);
			}
			cur += ctx->oplen;
		} else {
			fprintf (out, " %s[unknown op length]%s", cols->err, cols->reset);
			cur++;
		}
		fprintf (out, "%s\n", cols->reset);
	}
	free(ctx->marks);
}

static const struct {
	const char *name;
	struct disisa *isa;
} isas[] = {
	"nv50", &nv50_isa_s,
	"nvc0", &nvc0_isa_s,
	"ctx", &ctx_isa_s,
	"fuc", &fuc_isa_s,
	"fµc", &fuc_isa_s,
	"hwsq", &hwsq_isa_s,
	"vp2", &vp2_isa_s,
	"vuc", &vuc_isa_s,
	"vµc", &vuc_isa_s,
	"macro", &macro_isa_s,
	"vp1", &vp1_isa_s,
};

const struct disisa *ed_getisa(const char *name) {
	int i;
	for (i = 0; i < sizeof isas / sizeof *isas; i++)
		if (!strcmp(name, isas[i].name)) {
			struct disisa *isa = isas[i].isa;
			if (!isa->prepdone) {
				isas[i].isa->ed2 = ed2i_read_isa(isas[i].isa->ed2name);
				if (!isas[i].isa->ed2)
					return 0;
				isas[i].isa->vardata = isas[i].isa->ed2->vardata;
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
