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

/*
 * Color scheme
 */

int var_ok(int fmask, int ptype, struct ed2v_variant *var) {
	return (!fmask || (var->fmask[0] & fmask) == fmask) && (!ptype || (var->mode != -1 && ptype & 1 << var->mode));
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
		while ((a[0]&tab->mask) != tab->val || !var_ok(tab->fmask, tab->ptype, ctx->variant))
			tab++;
		m[0] |= tab->mask;
		for (i = 0; i < 16; i++)
			if (tab->atoms[i].fun)
				tab->atoms[i].fun (ctx, a, m, tab->atoms[i].arg, 0);
	} else {
		struct matches *res = emptymatches();
		int i;
		for (i = 0; ; i++) {
			if (var_ok(tab[0].fmask, tab[0].ptype, ctx->variant)) {
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
	} else {
		struct matches *res = alwaysmatches(spos);
		res->m[0].oplen = *(int*)v;
		return res;
	}
}

struct matches *atomendmark APROTO {
	if (!ctx->reverse) {
		ctx->endmark = 1;
	} else {
		struct matches *res = alwaysmatches(spos);
		return res;
	}
}

struct matches *atomsestart APROTO {
	if (!ctx->reverse) {
		struct expr *expr = makeex(EXPR_SESTART);
		ADDARRAY(ctx->atoms, expr);
	} else {
		struct expr *e = ctx->line->atoms[spos];
		if (e->type == EXPR_SESTART)
			return alwaysmatches(spos+1);
		else
			return 0;
	}
}

struct matches *atomseend APROTO {
	if (!ctx->reverse) {
		struct expr *expr = makeex(EXPR_SEEND);
		ADDARRAY(ctx->atoms, expr);
	} else {
		struct expr *e = ctx->line->atoms[spos];
		if (e->type == EXPR_SEEND)
			return alwaysmatches(spos+1);
		else
			return 0;
	}
}

struct matches *matchid(struct disctx *ctx, int spos, const char *v) {
	if (spos == ctx->line->atomsnum)
		return 0;
	struct expr *e = ctx->line->atoms[spos];
	if (e->type == EXPR_ID && !strcmp(e->str, v))
		return alwaysmatches(spos+1);
	else
		return 0;
}

struct matches *atomname APROTO {
	if (!ctx->reverse) {
		struct expr *expr = makeex(EXPR_ID);
		expr->str = v;
		ADDARRAY(ctx->atoms, expr);
	} else {
		return matchid(ctx, spos, v);
	}
}

struct matches *atomcmd APROTO {
	if (!ctx->reverse) {
		struct expr *expr = makeex(EXPR_ID);
		expr->str = v;
		expr->special = 1;
		ADDARRAY(ctx->atoms, expr);
	} else {
		return matchid(ctx, spos, v);
	}
}

struct matches *atomunk APROTO {
	if (ctx->reverse)
		return 0;
	struct expr *expr = makeex(EXPR_ID);
	expr->str = v;
	expr->special = 2;
	ADDARRAY(ctx->atoms, expr);
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

int setbfe (struct match *res, const struct bitfield *bf, const struct expr *expr) {
	if (!expr->isimm)
		return 0;
	if (expr->type != EXPR_NUM || bf->pcrel) {
		assert (res->nrelocs != 8);
		res->relocs[res->nrelocs].bf = bf;
		res->relocs[res->nrelocs].expr = expr;
		res->nrelocs++;
		return 1;
	}
	ull num = expr->num1 - bf->addend;
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
	return (getbf(bf, res->a, res->m, 0) & mask) == (expr->num1 & mask);
}

int setbf (struct match *res, const struct bitfield *bf, ull num) {
	if (bf->pcrel) {
		struct expr *e = makeex(EXPR_NUM);
		e->num1 = num;
		e->isimm = 1;
		return setbfe(res, bf, e);
	} else {
		struct expr e = { .type = EXPR_NUM, .num1 = num, .isimm = 1 };
		return setbfe(res, bf, &e);
	}
}

struct matches *matchimm (struct disctx *ctx, const struct bitfield *bf, int spos) {
	if (spos == ctx->line->atomsnum)
		return 0;
	struct matches *res = alwaysmatches(spos+1);
	if (setbfe(res->m, bf, ctx->line->atoms[spos]))
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
	struct expr *expr = makeex(EXPR_NUM);
	expr->num1 = GETBF(bf);
	ADDARRAY(ctx->atoms, expr);
}

struct matches *atomctarg APROTO {
	const struct bitfield *bf = v;
	if (ctx->reverse)
		return matchimm(ctx, bf, spos);
	struct expr *expr = makeex(EXPR_NUM);
	expr->num1 = GETBF(bf);
	mark(ctx, expr->num1, 2);
	expr->special = 2;
	ADDARRAY(ctx->atoms, expr);
}

struct matches *atombtarg APROTO {
	const struct bitfield *bf = v;
	if (ctx->reverse)
		return matchimm(ctx, bf, spos);
	struct expr *expr = makeex(EXPR_NUM);
	expr->num1 = GETBF(bf);
	mark(ctx, expr->num1, 1);
	expr->special = 1;
	ADDARRAY(ctx->atoms, expr);
}

struct matches *atomign APROTO {
	if (ctx->reverse)
		return alwaysmatches(spos);
	const int *n = v;
	(void)BF(n[0], n[1]);
}

int matchreg (struct match *res, const struct reg *reg, const struct expr *expr, struct disctx *ctx) {
	if (reg->specials) {
		int i = 0;
		for (i = 0; reg->specials[i].num != -1; i++) {
			if (!var_ok(reg->specials[i].fmask, 0, ctx->variant))
				continue;
			switch (reg->specials[i].mode) {
				case SR_NAMED:
					if (expr->type == EXPR_REG && !strcmp(expr->str, reg->specials[i].name))
						return setbf(res, reg->bf, reg->specials[i].num);
					break;
				case SR_ZERO:
					if (expr->type == EXPR_NUM && expr->num1 == 0)
						return setbf(res, reg->bf, reg->specials[i].num);
					break;
				case SR_ONE:
					if (expr->type == EXPR_NUM && expr->num1 == 1)
						return setbf(res, reg->bf, reg->specials[i].num);
					break;
				case SR_DISCARD:
					if (expr->type == EXPR_DISCARD)
						return setbf(res, reg->bf, reg->specials[i].num);
					break;
			}
		}
	}
	if (expr->type != EXPR_REG)
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

int matchshreg (struct match *res, const struct reg *reg, const struct expr *expr, int shl, struct disctx *ctx) {
	ull sh = 0;
	while (1) {
		if (expr->type == EXPR_SHL && expr->expr2->type == EXPR_NUM) {
			sh += expr->expr2->num1;
			expr = expr->expr1;
		} else if (expr->type == EXPR_MUL && expr->expr2->type == EXPR_NUM) {
			ull num = expr->expr2->num1;
			if (!num || (num & num-1))
				return 0;
			while (num != 1)
				num >>= 1, sh++;
			expr = expr->expr1;
		} else if (expr->type == EXPR_MUL && expr->expr1->type == EXPR_NUM) {
			ull num = expr->expr1->num1;
			if (!num || (num & num-1))
				return 0;
			while (num != 1)
				num >>= 1, sh++;
			expr = expr->expr2;
		} else {
			break;
		}
	}
	if (sh != shl)
		return 0;
	return matchreg(res, reg, expr, ctx);
}

// print reg if non-0, ignore otherwise. return 1 if printed anything.
static struct expr *printreg (struct disctx *ctx, ull *a, ull *m, const struct reg *reg) {
	struct expr *expr;
	ull num = 0;
	if (reg->bf)
		num = GETBF(reg->bf);
	if (reg->specials) {
		int i;
		for (i = 0; reg->specials[i].num != -1; i++) {
			if (!var_ok(reg->specials[i].fmask, 0, ctx->variant))
				continue;
			if (num == reg->specials[i].num) {
				switch (reg->specials[i].mode) {
					case SR_NAMED:
						expr = makeex(EXPR_REG);
						expr->str = reg->specials[i].name;
						expr->special = 1;
						return expr;
					case SR_ZERO:
						return 0;
					case SR_ONE:
						expr = makeex(EXPR_NUM);
						expr->num1 = 1;
						return expr;
					case SR_DISCARD:
						expr = makeex(EXPR_DISCARD);
						return expr;
				}
			}
		}
	}
	expr = makeex(EXPR_REG);
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
	expr->special = reg->cool;
	if (reg->always_special)
		expr->special = 2;
	if (reg->bf)
		expr->str = aprint("%s%lld%s", reg->name, num, suf);
	else
		expr->str = aprint("%s%s", reg->name, suf);
	return expr;
}

struct matches *atomreg APROTO {
	const struct reg *reg = v;
	if (ctx->reverse) {
		if (spos == ctx->line->atomsnum)
			return 0;
		struct matches *res = alwaysmatches(spos+1);
		if (matchreg(res->m, reg, ctx->line->atoms[spos], ctx))
			return res;
		else {
			free(res->m);
			free(res);
			return 0;
		}
	}
	struct expr *expr = printreg(ctx, a, m, reg);
	if (!expr) expr = makeex(EXPR_NUM);
	ADDARRAY(ctx->atoms, expr);
}

struct matches *atomdiscard APROTO {
	if (ctx->reverse) {
		if (spos == ctx->line->atomsnum)
			return 0;
		if (ctx->line->atoms[spos]->type == EXPR_DISCARD) {
			return alwaysmatches(spos+1);
		} else {
			return 0;
		}
	}
	struct expr *expr = makeex(EXPR_DISCARD);
	ADDARRAY(ctx->atoms, expr);
}

int addexpr (const struct expr **iex, const struct expr *expr, int flip) {
	if (flip) {
		if (!*iex)
			*iex = makeunex(EXPR_NEG, expr);
		else
			*iex = makebinex(EXPR_SUB, *iex, expr);
	} else {
		if (!*iex)
			*iex = expr;
		else
			*iex = makebinex(EXPR_ADD, *iex, expr);
	}
	return 1;
}

int matchmemaddr(const struct expr **iex, const struct expr **niex1, const struct expr **niex2, const struct expr **piex, const struct expr *expr, int flip) {
	if (expr->type == EXPR_PIADD || expr->type == EXPR_PISUB) {
		if (flip || !expr->expr2->isimm)
			return 0;
		return matchmemaddr(iex, niex1, niex2, piex, expr->expr1, 0) && addexpr(piex, expr->expr2, expr->type == EXPR_PISUB);
	}
	if (expr->isimm)
		return addexpr(iex, expr, flip);
	if (expr->type == EXPR_ADD)
		return matchmemaddr(iex, niex1, niex2, piex, expr->expr1, flip) && matchmemaddr(iex, niex1, niex2, piex, expr->expr2, flip);
	if (expr->type == EXPR_SUB)
		return matchmemaddr(iex, niex1, niex2, piex, expr->expr1, flip) && matchmemaddr(iex, niex1, niex2, piex, expr->expr2, !flip);
	if (expr->type == EXPR_NEG)
		return matchmemaddr(iex, niex1, niex2, piex, expr->expr1, !flip);
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
		struct expr *expr = 0;
		if (mem->reg)
			expr = printreg(ctx, a, m, mem->reg);
		if (mem->imm) {
			ull imm = GETBF(mem->imm);
			if (mem->postincr) {
				if (!expr) expr = makeex(EXPR_NUM);
				struct expr *sexpr = makeex(EXPR_NUM);
				if (imm & 1ull << 63) {
					sexpr->num1 = -imm;
					expr = makebinex(EXPR_PISUB, expr, sexpr);
				} else {
					sexpr->num1 = imm;
					expr = makebinex(EXPR_PIADD, expr, sexpr);
				}
			} else if (imm) {
				struct expr *sexpr = makeex(EXPR_NUM);
				if (expr) {
					if (imm & 1ull << 63) {
						sexpr->num1 = -imm;
						expr = makebinex(EXPR_SUB, expr, sexpr);
					} else {
						sexpr->num1 = imm;
						expr = makebinex(EXPR_ADD, expr, sexpr);
					}
				} else {
					sexpr->num1 = imm;
					expr = sexpr;
				}
			}
		}
		if (mem->reg2) {
			struct expr *sexpr = printreg(ctx, a, m, mem->reg2);
			if (sexpr) {
				if (mem->reg2shr) {
					struct expr *ssexpr = makeex(EXPR_NUM);
					ssexpr->num1 = 1ull << mem->reg2shr;
					sexpr = makebinex(EXPR_MUL, sexpr, ssexpr);
				}
				if (expr)
					expr = makebinex(EXPR_ADD, expr, sexpr);
				else
					expr = sexpr;
			}
		}
		if (!expr) expr = makeex(EXPR_NUM);
		if (mem->name) {
			struct expr *nex = makeex(EXPR_MEM);
			nex->expr1 = expr;
			nex->str = mem->name;
			if (mem->idx)
				nex->str = aprint("%s%lld", nex->str, GETBF(mem->idx));
			expr = nex;
		}
		ADDARRAY(ctx->atoms, expr);
		if (mem->literal && expr->expr1->type == EXPR_NUM) {
			ull ptr = expr->expr1->num1;
			mark(ctx, ptr, 0x10);
			if (ptr < ctx->codebase || ptr > ctx->codebase + ctx->codesz)
				return;
			uint32_t num = 0;
			int j;
			for (j = 0; j < 4; j++)
				num |= ctx->code8[(ptr - ctx->codebase) * ctx->isa->posunit + j] << j*8;
			expr = makeex(EXPR_NUM);
			expr->num1 = num;
			expr->special = 3;
			ADDARRAY(ctx->atoms, expr);
		}
	} else {
		if (spos == ctx->line->atomsnum)
			return 0;
		struct match res = { 0, .lpos = spos+1 };
		const struct expr *expr = ctx->line->atoms[spos];
		if (expr->type == EXPR_MEM && !mem->name)
			return 0;
		if (expr->type != EXPR_MEM && mem->name)
			return 0;
		if (expr->type == EXPR_MEM) {
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
			expr = expr->expr1;
		}
		const struct expr *iex = 0;
		const struct expr *niex1 = 0;
		const struct expr *niex2 = 0;
		const struct expr *piex = 0;
		matchmemaddr(&iex, &niex1, &niex2, &piex, expr, 0);
		if (mem->imm) {
			if (mem->postincr) {
				if (!piex || iex)
					return 0;
				if (!setbfe(&res, mem->imm, piex))
					return 0;
			} else {
				if (piex)
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
			if (iex)
				return 0;
		}
		const static struct expr e0 = { .type = EXPR_NUM, .isimm = 1 };
		if (mem->reg && mem->reg2) {
			if (!niex1)
				niex1 = &e0;
			if (!niex2)
				niex2 = &e0;
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
				niex1 = &e0;
			if (!matchreg(&res, mem->reg, niex1, ctx))
				return 0;
		} else if (mem->reg2) {
			if (niex2)
				return 0;
			if (!niex1)
				niex1 = &e0;
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
		struct expr *expr = makeex(EXPR_VEC);
		ull base = GETBF(vec->bf);
		ull cnt = GETBF(vec->cnt);
		ull mask = -1ull;
		if (vec->mask)
			mask = GETBF(vec->mask);
		int k = 0, i;
		for (i = 0; i < cnt; i++) {
			if (mask & 1ull<<i) {
				struct expr *sexpr = makeex(EXPR_REG);
				sexpr->special = vec->cool;
				sexpr->str = aprint("%s%lld", vec->name,  base + k++);
				ADDARRAY(expr->vexprs, sexpr);
			} else {
				struct expr *sexpr = makeex(EXPR_DISCARD);
				ADDARRAY(expr->vexprs, sexpr);
			}
		}
		ADDARRAY(ctx->atoms, expr);
	} else {
		if (spos == ctx->line->atomsnum)
			return 0;
		struct match res = { 0, .lpos = spos+1 };
		const struct expr *expr = ctx->line->atoms[spos];
		if (expr->type != EXPR_VEC)
			return 0;
		ull start = 0, cnt = expr->vexprsnum, mask = 0, cur = 0;
		int i;
		for (i = 0; i < cnt; i++) {
			const struct expr *e = expr->vexprs[i];
			if (e->type == EXPR_DISCARD) {
			} else if (e->type == EXPR_REG) {
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
		struct expr *expr = makeex(EXPR_BITFIELD);
		expr->num1 = GETBF(&bf[0]);
		expr->num2 = expr->num1 + GETBF(&bf[1]);
		ADDARRAY(ctx->atoms, expr);
	} else {
		if (spos == ctx->line->atomsnum)
			return 0;
		struct match res = { 0, .lpos = spos+1 };
		const struct expr *expr = ctx->line->atoms[spos];
		if (expr->type != EXPR_BITFIELD)
			return 0;
		ull a = expr->num1;
		ull b = expr->num2 - a;
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
	res += bf->addend;
	return res;
}

/*
 * Disassembler driver
 *
 * You pass a block of memory to this function, disassembly goes out to given
 * FILE*.
 */

void envydis (const struct disisa *isa, FILE *out, uint8_t *code, uint32_t start, int num, struct ed2v_variant *variant, int quiet, struct label *labels, int labelsnum, const struct ed2a_colors *cols)
{
	struct disctx c = { 0 };
	struct disctx *ctx = &c;
	int cur = 0, i, j;
	ctx->code8 = code;
	ctx->marks = calloc((num + isa->posunit - 1) / isa->posunit, sizeof *ctx->marks);
	ctx->names = calloc((num + isa->posunit - 1) / isa->posunit, sizeof *ctx->names);
	ctx->codebase = start;
	ctx->codesz = num;
	ctx->variant = variant;
	ctx->isa = isa;
	if (labels) {
		for (i = 0; i < labelsnum; i++) {
			mark(ctx, labels[i].val, labels[i].type);
			if (labels[i].val >= ctx->codebase && labels[i].val < ctx->codebase + ctx->codesz) {
				if (labels[i].name)
					ctx->names[labels[i].val - ctx->codebase] = labels[i].name;
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
		if (ctx->names[cur / isa->posunit]) {
			if (skip) {
				if (nonzero)
					fprintf(out, "%s[%x bytes skipped]\n", cols->err, skip);
				else
					fprintf(out, "%s[%x zero bytes skipped]\n", cols->reset, skip);
				skip = 0;
				nonzero = 0;
			}
			if (ctx->marks[cur / isa->posunit] & 0x30)
				fprintf (out, "%s%s:\n", cols->reset, ctx->names[cur / isa->posunit]);
			else if (ctx->marks[cur / isa->posunit] & 2)
				fprintf (out, "\n%s%s:\n", cols->ctarg, ctx->names[cur / isa->posunit]);
			else if (ctx->marks[cur / isa->posunit] & 1)
				fprintf (out, "%s%s:\n", cols->btarg, ctx->names[cur / isa->posunit]);
			else
				fprintf (out, "%s%s:\n", cols->reset, ctx->names[cur / isa->posunit]);
		}
		if (ctx->marks[cur / isa->posunit] & 0x30) {
			if (skip) {
				if (nonzero)
					fprintf(out, "%s[%x bytes skipped]\n", cols->err, skip);
				else
					fprintf(out, "%s[%x zero bytes skipped]\n", cols->reset, skip);
				skip = 0;
				nonzero = 0;
			}
			fprintf (out, "%s%08x:%s", cols->mem, cur / isa->posunit + start, cols->reset);
			if (ctx->marks[cur / isa->posunit] & 0x10) {
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
		if (!active && ctx->marks[cur / isa->posunit] & 7)
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

		if (ctx->endmark || ctx->marks[cur / isa->posunit] & 4)
			active = 0;

		if (ctx->marks[cur / isa->posunit] & 2 && !ctx->names[cur / isa->posunit])
			fprintf (out, "\n");
		switch (ctx->marks[cur / isa->posunit] & 3) {
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

			if (ctx->marks[cur / isa->posunit] & 2)
				fprintf (out, "%sC", cols->ctarg);
			else
				fprintf (out, " ");
			if (ctx->marks[cur / isa->posunit] & 1)
				fprintf (out, "%sB", cols->btarg);
			else
				fprintf (out, " ");
		} else if (quiet == 1) {
			if (ctx->marks[cur / isa->posunit])
				fprintf (out, "\n");
		}

		int noblank = 0;

		for (i = 0; i < ctx->atomsnum; i++) {
			if (ctx->atoms[i]->type == EXPR_SEEND)
				noblank = 1;
			if ((i || !quiet) && !noblank)
				fprintf (out, " ");
			printexpr(out, ctx->atoms[i], 0, cols);
			noblank = (ctx->atoms[i]->type == EXPR_SESTART);
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
	"vp3m", &vp3m_isa_s,
	"macro", &macro_isa_s,
};

const struct disisa *ed_getisa(const char *name) {
	int i;
	for (i = 0; i < sizeof isas / sizeof *isas; i++)
		if (!strcmp(name, isas[i].name)) {
			if (!isas[i].isa->ed2)
				isas[i].isa->ed2 = ed2i_read_isa(isas[i].isa->ed2name);
			if (!isas[i].isa->ed2)
				return 0;
			return isas[i].isa;
		}
	return 0;
};
