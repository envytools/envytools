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

#include "dis-intern.h"
#include "envyas.h"
#include <assert.h>

struct iasctx {
	const struct disisa *isa;
	struct varinfo *varinfo;
	struct litem **atoms;
	int atomsnum;
	int atomsmax;
};

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

static inline ull bf_(int s, int l, ull *a, ull *m) {
	int idx = s / 0x40;
	int bit = s % 0x40;
	ull res = 0;
	ull m0 = (((1ull << l) - 1) << bit);
	m[idx] |= m0;
	res |= (a[idx] & m0) >> bit;
	if (bit + l > 0x40) {
		ull m1 = (((1ull << l) - 1) >> (0x40 - bit));
		m[idx+1] |= m1;
		res |= (a[idx+1] & m1) << (0x40 - bit);
	}
	return res;
}
#define BF(s, l) bf_(s, l, a, m)

ull getrbf_as(const struct rbitfield *bf, ull *a, ull *m, ull cpos) {
	ull res = 0;
	int pos = bf->shr;
	int i;
	for (i = 0; i < sizeof(bf->sbf) / sizeof(bf->sbf[0]); i++) {
		res |= BF(bf->sbf[i].pos, bf->sbf[i].len) << pos;
		pos += bf->sbf[i].len;
	}
	switch (bf->mode) {
		case RBF_UNSIGNED:
			break;
		case RBF_SIGNED:
			if (res & 1ull << (pos - 1))
				res -= 1ull << pos;
			break;
		case RBF_SLIGHTLY_SIGNED:
			if (res & 1ull << (pos - 1) && res & 1ull << (pos - 2))
				res -= 1ull << pos;
			break;
		case RBF_ULTRASIGNED:
			res -= 1ull << pos;
			break;
	}
	if (bf->pcrel) {
		// <3 xtensa.
		res += (cpos + bf->pospreadd) & -(1ull << bf->shr);
	}
	res += bf->addend;
	return res;
}

ull getbf_as(const struct bitfield *bf, ull *a, ull *m) {
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
	res ^= bf->xorend;
	res += bf->addend;
	return res;
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

static int setrbf (struct match *res, const struct rbitfield *bf, struct easm_expr *expr) {
	if (!easm_isimm(expr))
		return 0;
	if (expr->type != EASM_EXPR_NUM || bf->pcrel) {
		assert (res->nrelocs != 8);
		res->relocs[res->nrelocs].bf = bf;
		res->relocs[res->nrelocs].expr = expr;
		res->nrelocs++;
		return 1;
	}
	ull num = expr->num - bf->addend;
	num >>= bf->shr;
	setsbf(res, bf->sbf[0].pos, bf->sbf[0].len, num);
	num >>= bf->sbf[0].len;
	setsbf(res, bf->sbf[1].pos, bf->sbf[1].len, num);
	ull mask = ~0ull;
	ull totalsz = bf->shr + bf->sbf[0].len + bf->sbf[1].len;
	if (bf->wrapok && totalsz < 64)
		mask = (1ull << totalsz) - 1;
	return (getrbf_as(bf, res->a, res->m, 0) & mask) == (expr->num & mask);
}

static int setbf (struct match *res, const struct bitfield *bf, ull num) {
	ull onum = num;
	num = (num - bf->addend) ^ bf->xorend;
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
	return (getbf_as(bf, res->a, res->m) & mask) == (onum & mask);
}

struct matches *tabdesc (struct iasctx *ctx, struct match m, const struct atom *atoms) {
	if (!atoms->fun_as) {
		struct matches *res = emptymatches();
		ADDARRAY(res->m, m);
		return res;
	}
	struct matches *ms = atoms->fun_as(ctx, atoms->arg, m.lpos);
	if (!ms)
		return 0;
	ms = mergematches(m, ms);
	atoms++;
	if (!atoms->fun_as)
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

struct matches *atomtab_a APROTO {
	const struct insn *tab = v;
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

struct matches *atomopl_a APROTO {
	struct matches *res = alwaysmatches(spos);
	res->m[0].oplen = *(int*)v;
	return res;
}

struct matches *atomsestart_a APROTO {
	struct litem *li = ctx->atoms[spos];
	if (li->type == LITEM_SESTART)
		return alwaysmatches(spos+1);
	else
		return 0;
}

struct matches *atomseend_a APROTO {
	struct litem *li = ctx->atoms[spos];
	if (li->type == LITEM_SEEND)
		return alwaysmatches(spos+1);
	else
		return 0;
}

struct matches *atomname_a APROTO {
	if (spos == ctx->atomsnum)
		return 0;
	struct litem *li = ctx->atoms[spos];
	if (li->type == LITEM_NAME && !strcmp(li->str, v))
		return alwaysmatches(spos+1);
	else
		return 0;
}

struct matches *atomcmd_a APROTO {
	if (spos == ctx->atomsnum || ctx->atoms[spos]->type != LITEM_EXPR)
		return 0;
	struct easm_expr *e = ctx->atoms[spos]->expr;
	if (e->type == EASM_EXPR_LABEL && !strcmp(e->str, v))
		return alwaysmatches(spos+1);
	else
		return 0;
}

struct matches *atomunk_a APROTO {
	return 0;
}

struct matches *atomimm_a APROTO {
	const struct bitfield *bf = v;
	if (spos == ctx->atomsnum || ctx->atoms[spos]->type != LITEM_EXPR)
		return 0;
	struct matches *res = alwaysmatches(spos+1);
	struct easm_expr *expr = ctx->atoms[spos]->expr;
	if (expr->type == EASM_EXPR_NUM && setbf(res->m, bf, expr->num))
		return res;
	else {
		free(res->m);
		free(res);
		return 0;
	}
}

struct matches *atomrimm_a APROTO {
	const struct rbitfield *bf = v;
	if (spos == ctx->atomsnum || ctx->atoms[spos]->type != LITEM_EXPR)
		return 0;
	struct matches *res = alwaysmatches(spos+1);
	if (setrbf(res->m, bf, ctx->atoms[spos]->expr))
		return res;
	else {
		free(res->m);
		free(res);
		return 0;
	}
}

struct matches *atomnop_a APROTO {
	return alwaysmatches(spos);
}

int matchreg (struct match *res, const struct reg *reg, const struct easm_expr *expr, struct iasctx *ctx) {
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

int matchshreg (struct match *res, const struct reg *reg, const struct easm_expr *expr, int shl, struct iasctx *ctx) {
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

struct matches *atomreg_a APROTO {
	const struct reg *reg = v;
	if (spos == ctx->atomsnum || ctx->atoms[spos]->type != LITEM_EXPR)
		return 0;
	struct easm_expr *e = ctx->atoms[spos]->expr;
	struct matches *res = alwaysmatches(spos+1);
	if (matchreg(res->m, reg, e, ctx))
		return res;
	else {
		free(res->m);
		free(res);
		return 0;
	}
}

struct matches *atomdiscard_a APROTO {
	if (spos == ctx->atomsnum || ctx->atoms[spos]->type != LITEM_EXPR)
		return 0;
	struct easm_expr *e = ctx->atoms[spos]->expr;
	if (e->type == EASM_EXPR_DISCARD) {
		return alwaysmatches(spos+1);
	} else {
		return 0;
	}
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

struct matches *atommem_a APROTO {
	const struct mem *mem = v;
	if (spos == ctx->atomsnum || ctx->atoms[spos]->type != LITEM_EXPR)
		return 0;
	struct easm_expr *expr = ctx->atoms[spos]->expr;
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
			if (!setrbf(&res, mem->imm, pexpr))
				return 0;
		} else {
			if (pexpr)
				return 0;
			if (iex) {
				if (!setrbf(&res, mem->imm, iex))
					return 0;
			} else {
				if (!setrbf(&res, mem->imm, easm_expr_num(EASM_EXPR_NUM, 0)))
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

struct matches *atomvec_a APROTO {
	const struct vec *vec = v;
	if (spos == ctx->atomsnum || ctx->atoms[spos]->type != LITEM_EXPR)
		return 0;
	struct match res = { 0, .lpos = spos+1 };
	const struct easm_expr *expr = ctx->atoms[spos]->expr;
	const struct easm_expr **vexprs = 0;
	int vexprsnum = 0;
	int vexprsmax = 0;
	if (expr->type != EASM_EXPR_ZVEC) {
		while (expr->type == EASM_EXPR_VEC) {
			ADDARRAY(vexprs, expr->e2);
			expr = expr->e1;
		}
		ADDARRAY(vexprs, expr);
	}
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

struct matches *atombf_a APROTO {
	const struct bitfield *bf = v;
	if (spos == ctx->atomsnum || ctx->atoms[spos]->type != LITEM_EXPR)
		return 0;
	struct match res = { 0, .lpos = spos+1 };
	const struct easm_expr *expr = ctx->atoms[spos]->expr;
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

void convert_expr_top(struct iasctx *ctx, struct easm_expr *expr);

void convert_mods(struct iasctx *ctx, struct easm_mods *mods) {
	int i;
	for (i = 0; i < mods->modsnum; i++) {
		struct litem *li = calloc(sizeof *li, 1);
		li->type = LITEM_NAME;
		li->str = mods->mods[i]->str;
		ADDARRAY(ctx->atoms, li);
	}
}

void convert_operand(struct iasctx *ctx, struct easm_operand *operand) {
	int i;
	convert_mods(ctx, operand->mods);
	for (i = 0; i < operand->exprsnum; i++) {
		/* XXX */
		convert_expr_top(ctx, operand->exprs[i]);
	}
}

void convert_sinsn(struct iasctx *ctx, struct easm_sinsn *sinsn) {
	int i;
	struct litem *li = calloc(sizeof *li, 1);
	li->type = LITEM_NAME;
	li->str = sinsn->str;
	ADDARRAY(ctx->atoms, li);
	for (i = 0; i < sinsn->operandsnum; i++) {
		convert_operand(ctx, sinsn->operands[i]);
	}
	convert_mods(ctx, sinsn->mods);
}

void convert_subinsn(struct iasctx *ctx, struct easm_subinsn *subinsn) {
	int i;
	for (i = 0; i < subinsn->prefsnum; i++) {
		convert_expr_top(ctx, subinsn->prefs[i]);
	}
	convert_sinsn(ctx, subinsn->sinsn);
}

void convert_expr_top(struct iasctx *ctx, struct easm_expr *expr) {
	if (expr->type == EASM_EXPR_SINSN) {
		struct litem *ses = calloc(sizeof *ses, 1);
		struct litem *see = calloc(sizeof *see, 1);
		ses->type = LITEM_SESTART;
		see->type = LITEM_SEEND;
		ADDARRAY(ctx->atoms, ses);
		convert_sinsn(ctx, expr->sinsn);
		ADDARRAY(ctx->atoms, see);
	} else {
		struct litem *li = calloc(sizeof *li, 1);
		li->type = LITEM_EXPR;
		li->expr = expr;
		ADDARRAY(ctx->atoms, li);
	}
}

void convert_insn(struct iasctx *ctx, struct easm_insn *insn) {
	int i;
	for (i = 0; i < insn->subinsnsnum; i++) {
		/* XXX */
		convert_subinsn(ctx, insn->subinsns[i]);
	}
}

struct matches *do_as(const struct disisa *isa, struct varinfo *varinfo, struct easm_insn *insn) {
	struct matches *res = calloc(sizeof *res, 1);
	struct iasctx c = { isa, varinfo };
	struct iasctx *ctx = &c;
	convert_insn(ctx, insn);
	const struct insn *root = isa->trootas ? isa->trootas : isa->troot;
	struct matches *m = atomtab_a(ctx, root, 0);
	int i;
	for (i = 0; i < m->mnum; i++)
		if (m->m[i].lpos == ctx->atomsnum) {
			ADDARRAY(res->m, m->m[i]);
		}
	return res;
}
