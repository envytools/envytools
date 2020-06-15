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
#include "easm.h"
#include <stdlib.h>

struct disctx {
	const struct disisa *isa;
	struct varinfo *varinfo;
	int oplen;
	struct litem **atoms;
	int atomsnum;
	int atomsmax;
	int endmark;
};

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

ull getbf(const struct bitfield *bf, ull *a, ull *m) {
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

struct easm_expr *getrbf(const struct rbitfield *bf, ull *a, ull *m) {
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
		struct easm_expr *expr = easm_expr_simple(EASM_EXPR_POS);
		if (bf->pospreadd)
			expr = easm_expr_bin(EASM_EXPR_ADD, expr, easm_expr_num(EASM_EXPR_NUM, bf->pospreadd));
		if (bf->shr)
			expr = easm_expr_bin(EASM_EXPR_AND, expr, easm_expr_num(EASM_EXPR_NUM, -(1ull << bf->shr)));
		expr = easm_expr_bin(EASM_EXPR_ADD, expr, easm_expr_num(EASM_EXPR_NUM, res));
		if (bf->addend)
			expr = easm_expr_bin(EASM_EXPR_ADD, expr, easm_expr_num(EASM_EXPR_NUM, bf->addend));
		return expr;
	} else {
		res += bf->addend;
		return easm_expr_num(EASM_EXPR_NUM, res);
	}
}

#define GETBF(bf) getbf(bf, a, m)
#define GETRBF(bf) getrbf(bf, a, m)

static inline struct litem *makeli(struct easm_expr *e) {
	struct litem *li = calloc(sizeof *li, 1);
	li->type = LITEM_EXPR;
	li->expr = e;
	return li;
}

void atomtab_d DPROTO {
	const struct insn *tab = v;
	int i;
	while ((a[0]&tab->mask) != tab->val || !var_ok(tab->fmask, tab->ptype, ctx->varinfo))
		tab++;
	m[0] |= tab->mask;
	for (i = 0; i < ARRAY_SIZE(tab->atoms); i++)
		if (tab->atoms[i].fun_dis)
			tab->atoms[i].fun_dis (ctx, a, m, tab->atoms[i].arg);
}

void atomopl_d DPROTO {
	ctx->oplen = *(int*)v;
}

void atomendmark_d DPROTO {
	ctx->endmark = 1;
}

void atomsestart_d DPROTO {
	struct litem *li = calloc(sizeof *li, 1);
	li->type = LITEM_SESTART;
	ADDARRAY(ctx->atoms, li);
}

void atomseend_d DPROTO {
	struct litem *li = calloc(sizeof *li, 1);
	li->type = LITEM_SEEND;
	ADDARRAY(ctx->atoms, li);
}

void atomname_d DPROTO {
	struct litem *li = calloc(sizeof *li, 1);
	li->type = LITEM_NAME;
	li->str = strdup(v);
	ADDARRAY(ctx->atoms, li);
}

void atomcmd_d DPROTO {
	struct litem *li = makeli(easm_expr_str(EASM_EXPR_LABEL, strdup(v)));
	ADDARRAY(ctx->atoms, li);
}

void atomunk_d DPROTO {
	struct litem *li = calloc(sizeof *li, 1);
	li->type = LITEM_NAME;
	li->str = strdup(v);
	li->isunk = 1;
	ADDARRAY(ctx->atoms, li);
}

void atomimm_d DPROTO {
	const struct bitfield *bf = v;
	struct easm_expr *expr = easm_expr_num(EASM_EXPR_NUM, GETBF(bf));
	ADDARRAY(ctx->atoms, makeli(expr));
}

void atomrimm_d DPROTO {
	const struct rbitfield *bf = v;
	struct easm_expr *expr = GETRBF(bf);
	ADDARRAY(ctx->atoms, makeli(expr));
}

void atomctarg_d DPROTO {
	const struct rbitfield *bf = v;
	struct easm_expr *expr = GETRBF(bf);
	expr->special = EASM_SPEC_CTARG;
	ADDARRAY(ctx->atoms, makeli(expr));
}

void atombtarg_d DPROTO {
	const struct rbitfield *bf = v;
	struct easm_expr *expr = GETRBF(bf);
	expr->special = EASM_SPEC_BTARG;
	ADDARRAY(ctx->atoms, makeli(expr));
}

void atomign_d DPROTO {
	const int *n = v;
	(void)BF(n[0], n[1]);
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
						return easm_expr_simple(EASM_EXPR_DISCARD);
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
		str = aprintf("%s%lld%s", reg->name, num, suf);
	else
		str = aprintf("%s%s", reg->name, suf);
	expr = easm_expr_str(EASM_EXPR_REG, str);
	if (reg->cool)
		expr->special = EASM_SPEC_REGSP;
	if (reg->always_special)
		expr->special = EASM_SPEC_ERR;
	return expr;
}

void atomreg_d DPROTO {
	const struct reg *reg = v;
	struct easm_expr *expr = printreg(ctx, a, m, reg);
	if (!expr) expr = easm_expr_num(EASM_EXPR_NUM, 0);
	ADDARRAY(ctx->atoms, makeli(expr));
}

void atomdiscard_d DPROTO {
	struct easm_expr *expr = easm_expr_simple(EASM_EXPR_DISCARD);
	ADDARRAY(ctx->atoms, makeli(expr));
}

void atommem_d DPROTO {
	const struct mem *mem = v;
	int type = EASM_EXPR_MEM;
	struct easm_expr *expr = 0;
	struct easm_expr *pexpr = 0;
	if (mem->reg)
		expr = printreg(ctx, a, m, mem->reg);
	if (mem->imm) {
		struct easm_expr *imm = GETRBF(mem->imm);
		if (mem->postincr) {
			type = EASM_EXPR_MEMPP;
			pexpr = imm;
		} else {
			if (expr) {
				expr = easm_expr_bin(EASM_EXPR_ADD, expr, imm);
			} else {
				expr = imm;
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
	if (mem->name) {
		struct easm_expr *nex;
		if (pexpr)
			nex = easm_expr_bin(type, expr, pexpr);
		else
			nex = easm_expr_un(type, expr);
		if (mem->idx)
			nex->str = aprintf("%s%lld", mem->name, GETBF(mem->idx));
		else
			nex->str = strdup(mem->name);
		nex->mods = calloc(sizeof *nex->mods, 1);
		expr = nex;
	} else if (type != EASM_EXPR_MEM) {
		abort();
	}
	if (mem->literal && expr->type == EASM_EXPR_MEM)
		expr->special = EASM_SPEC_LITERAL;
	ADDARRAY(ctx->atoms, makeli(expr));
}

void atomvec_d DPROTO {
	const struct vec *vec = v;
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
			char *name = aprintf("%s%lld", vec->name,  base + k++);
			sexpr = easm_expr_str(EASM_EXPR_REG, name);
			if (vec->cool)
				sexpr->special = EASM_SPEC_REGSP;
		} else {
			sexpr = easm_expr_simple(EASM_EXPR_DISCARD);
		}
		if (expr)
			expr = easm_expr_bin(EASM_EXPR_VEC, expr, sexpr);
		else
			expr = sexpr;
	}
	if (!expr)
		expr = easm_expr_simple(EASM_EXPR_ZVEC);
	ADDARRAY(ctx->atoms, makeli(expr));
}

void atombf_d DPROTO {
	const struct bitfield *bf = v;
	uint64_t num1 = GETBF(&bf[0]);
	uint64_t num2 = num1 + GETBF(&bf[1]);
	struct easm_expr *expr = easm_expr_bin(EASM_EXPR_VEC,
			easm_expr_num(EASM_EXPR_NUM, num1),
			easm_expr_num(EASM_EXPR_NUM, num2));
	ADDARRAY(ctx->atoms, makeli(expr));
}

struct dis_op_chunk {
	int len;
	int isbe;
};

struct dis_res {
	enum dis_status {
		DIS_STATUS_OK = 0,
		DIS_STATUS_EOF = 0x1,		/* EOF in the middle of an opcode */
		DIS_STATUS_UNK_FORM = 0x2,	/* failed to determine instruction format - opcode length uncertain */
		DIS_STATUS_UNK_INSN = 0x4,	/* failed to determine instruction name - unknown opcode or due to one of the above errors */
		DIS_STATUS_UNK_OPERAND = 0x8,	/* failed to determine instruction operands */
		DIS_STATUS_UNUSED_BITS = 0x10,	/* instruction decoded, but unused bitfields have non-default values */
	} status;
	uint32_t oplen;
	struct dis_op_chunk *chunks;
	int chunksnum;
	int chunksmax;
//	uint32_t align;
//	uint32_t askip;
	ull a[MAXOPLEN], m[MAXOPLEN];
	struct easm_insn *insn;
	int endmark;
//	uint32_t *umask;
};

static void dis_del_res(struct dis_res *dres)
{
	easm_del_insn(dres->insn);

	free(dres);
}

static struct easm_sinsn *dis_parse_sinsn(struct disctx *ctx, enum dis_status *status, int *spos);

static struct easm_expr *dis_parse_expr(struct disctx *ctx, enum dis_status *status, int *spos) {
	if (*spos >= ctx->atomsnum)
		abort();
	if (ctx->atoms[*spos]->type == LITEM_EXPR)
		return ctx->atoms[(*spos)++]->expr;
	if (ctx->atoms[(*spos)++]->type != LITEM_SESTART)
		abort();
	struct easm_expr *res = easm_expr_sinsn(dis_parse_sinsn(ctx, status, spos));
	if (ctx->atoms[(*spos)++]->type != LITEM_SEEND)
		abort();
	return res;
}

static struct easm_sinsn *dis_parse_sinsn(struct disctx *ctx, enum dis_status *status, int *spos) {
	struct easm_sinsn *res = calloc(sizeof *res, 1);
	res->str = ctx->atoms[*spos]->str;
	res->isunk = ctx->atoms[*spos]->isunk;
	if (res->isunk)
		*status |= DIS_STATUS_UNK_INSN;
	if (ctx->atoms[(*spos)++]->type != LITEM_NAME)
		abort();
	struct easm_mods *mods = calloc (sizeof *mods, 1);
	while (*spos < ctx->atomsnum && ctx->atoms[*spos]->type != LITEM_SEEND) {
		if (ctx->atoms[*spos]->type == LITEM_NAME) {
			struct easm_mod *mod = calloc(sizeof *mod, 1);
			mod->str = ctx->atoms[*spos]->str;
			mod->isunk = ctx->atoms[*spos]->isunk;
			if (mod->isunk)
				*status |= DIS_STATUS_UNK_OPERAND;
			ADDARRAY(mods->mods, mod);
			(*spos)++;
		} else {
			struct easm_operand *op = calloc (sizeof *op, 1);
			op->mods = mods;
			mods = calloc (sizeof *mods, 1);
			ADDARRAY(op->exprs, dis_parse_expr(ctx, status, spos));
			ADDARRAY(res->operands, op);
		}
	}
	res->mods = mods;
	return res;
}

static struct easm_subinsn *dis_parse_subinsn(struct disctx *ctx, enum dis_status *status, int *spos) {
	struct easm_subinsn *res = calloc(sizeof *res, 1);
	while (ctx->atoms[*spos]->type != LITEM_NAME)
		ADDARRAY(res->prefs, dis_parse_expr(ctx, status, spos));
	res->sinsn = dis_parse_sinsn(ctx, status, spos);
	return res;
}

static struct easm_insn *dis_parse_insn(struct disctx *ctx, enum dis_status *status) {
	int spos = 0;
	struct easm_insn *res = calloc(sizeof *res, 1);
	ADDARRAY(res->subinsns, dis_parse_subinsn(ctx, status, &spos));
	if (spos != ctx->atomsnum)
		abort();
	return res;
}

struct decoctx {
	const struct disisa *isa;
	struct varinfo *varinfo;
	uint8_t *code;
	int *marks;
	const char **names;
	uint32_t codebase;
	uint32_t codesz;
	struct label *labels;
	int labelsnum;
	int labelsmax;
};

struct dis_res *do_dis(struct decoctx *deco, uint32_t cur) {
	struct disctx c = { 0 };
	struct disctx *ctx = &c;
	struct dis_res *res = calloc(sizeof *res, 1);
	int i;
	int stride = ed_getcstride(deco->isa, deco->varinfo);
	for (i = 0; i < MAXOPLEN*8 && cur + i/stride < deco->codesz; i++) {
		res->a[i/8] |= (ull)deco->code[cur*stride + i] << (i&7)*8;
	}
	ctx->isa = deco->isa;
	ctx->varinfo = deco->varinfo;
	if (deco->isa->tsched && (cur % deco->isa->schedpos) == 0)
		atomtab_d (ctx, res->a, res->m, deco->isa->tsched);
	else
		atomtab_d (ctx, res->a, res->m, deco->isa->troot);
	res->oplen = ctx->oplen;
	if (res->oplen + cur > deco->codesz)
		res->status |= DIS_STATUS_EOF;
	if (res->oplen == 0) {
		res->status |= DIS_STATUS_UNK_FORM;
		res->oplen = ctx->isa->opunit;
	}
	res->endmark = ctx->endmark;
	/* XXX unused status */
	res->insn = dis_parse_insn(ctx, &res->status);

	for (i = 0; i < ctx->atomsnum; ++i)
		free(ctx->atoms[i]);
	free(ctx->atoms);

	return res;
}

static void mark(struct decoctx *ctx, uint32_t ptr, int m) {
	if (ptr < ctx->codebase || ptr >= ctx->codebase + ctx->codesz)
		return;
	ctx->marks[ptr - ctx->codebase] |= m;
}

static int is_nr_mark(struct decoctx *ctx, uint32_t ptr) {
	if (ptr < ctx->codebase || ptr >= ctx->codebase + ctx->codesz)
		return 0;
	return ctx->marks[ptr - ctx->codebase] & 0x40;
}

char *deco_label(struct decoctx *ctx, uint64_t val) {
	int i;
	for (i = 0; i < ctx->labelsnum; i++)
		if (ctx->labels[i].val == val && ctx->labels[i].name)
			return strdup(ctx->labels[i].name);
	return 0;
}

static void dis_pp_sinsn(struct decoctx *deco, struct dis_res *dres, struct easm_sinsn *sinsn, uint64_t pos);

static void dis_pp_expr(struct decoctx *deco, struct dis_res *dres, struct easm_expr *expr, uint64_t pos) {
	if (expr->e1)
		dis_pp_expr(deco, dres, expr->e1, pos);
	if (expr->e2)
		dis_pp_expr(deco, dres, expr->e2, pos);
	if (expr->sinsn)
		dis_pp_sinsn(deco, dres, expr->sinsn, pos);
	easm_substpos_expr(expr, pos);
	if (easm_cfold_expr(expr)) {
		if (expr->special == EASM_SPEC_CTARG) {
			mark(deco, expr->num, 2);
			expr->alabel = deco_label(deco, expr->num);
			if (is_nr_mark(deco, expr->num))
				dres->endmark = 1;
		} else if (expr->special == EASM_SPEC_BTARG) {
			mark(deco, expr->num, 1);
			expr->alabel = deco_label(deco, expr->num);
		}
		if (expr->num & 1ull << 63 && !expr->special) {
			expr->type = EASM_EXPR_NEG;
			expr->e1 = easm_expr_num(EASM_EXPR_NUM, -expr->num);
			expr->num = 0;
		}
	}
	if (expr->type == EASM_EXPR_ADD && expr->e1->type == EASM_EXPR_NUM && expr->e1->num == 0) {
		struct easm_expr *oe2 = expr->e2;
		free(expr->e1);
		*expr = *oe2;
		free(oe2);
	}
	if ((expr->type == EASM_EXPR_ADD || expr->type == EASM_EXPR_SUB) && expr->e2->type == EASM_EXPR_NUM && expr->e2->num == 0) {
		struct easm_expr *oe1 = expr->e1;
		free(expr->e2);
		*expr = *oe1;
		free(oe1);
	}
	if (expr->type == EASM_EXPR_ADD && expr->e2->type == EASM_EXPR_NUM && expr->e2->num & 1ull << 63) {
		expr->e2->num = -expr->e2->num;
		expr->type = EASM_EXPR_SUB;
	}
	if (expr->type == EASM_EXPR_MEM && expr->special == EASM_SPEC_LITERAL) {
		if (expr->e1->type != EASM_EXPR_NUM) {
			expr->special = EASM_SPEC_NONE;
		} else {
			ull ptr = expr->e1->num;
			mark(deco, ptr, 0x10);
			if (ptr < deco->codebase || ptr > deco->codebase + deco->codesz) {
				expr->special = EASM_SPEC_NONE;
			} else {
				if (ed_getcbsz(deco->isa, deco->varinfo) != 8)
					abort();
				uint32_t num = 0;
				int j;
				for (j = 0; j < 4; j++)
					num |= deco->code[ptr - deco->codebase + j] << j*8;
				expr->alit = num;
			}
		}
	}
}

static void dis_pp_sinsn(struct decoctx *deco, struct dis_res *dres, struct easm_sinsn *sinsn, uint64_t pos) {
	int i, j;
	for (i = 0; i < sinsn->operandsnum; i++)
		for (j = 0; j < sinsn->operands[i]->exprsnum; j++)
			dis_pp_expr(deco, dres, sinsn->operands[i]->exprs[j], pos);
}

static void dis_pp_subinsn(struct decoctx *deco, struct dis_res *dres, struct easm_subinsn *subinsn, uint64_t pos) {
	int i;
	for (i = 0; i < subinsn->prefsnum; i++)
		dis_pp_expr(deco, dres, subinsn->prefs[i], pos);
	dis_pp_sinsn(deco, dres, subinsn->sinsn, pos);
}

static void dis_pp_insn(struct decoctx *deco, struct dis_res *dres, struct easm_insn *insn, uint64_t pos) {
	int i;
	for (i = 0; i < insn->subinsnsnum; i++)
		dis_pp_subinsn(deco, dres, insn->subinsns[i], pos);
}

void dis_dopp(struct decoctx *deco, struct dis_res *dres, uint64_t pos) {
	dis_pp_insn(deco, dres, dres->insn, pos);
}

/*
 * Disassembler driver
 *
 * You pass a block of memory to this function, disassembly goes out to given
 * FILE*.
 */

void envydis (const struct disisa *isa, FILE *out, uint8_t *code, uint32_t start, int num, struct varinfo *varinfo, int quiet, struct label *labels, int labelsnum, const struct envy_colors *cols)
{
	struct decoctx c = { 0 };
	struct decoctx *ctx = &c;
	int cur = 0, i, j;
	ctx->code = code;
	ctx->codesz = num;
	ctx->marks = calloc(num, sizeof *ctx->marks);
	ctx->names = calloc(num, sizeof *ctx->names);
	ctx->codebase = start;
	ctx->varinfo = varinfo;
	ctx->isa = isa;
	ctx->labels = labels;
	ctx->labelsnum = labelsnum;
	int stride = ed_getcstride(ctx->isa, ctx->varinfo);
	int cbsz = ed_getcbsz(ctx->isa, ctx->varinfo);
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
				if (!active && (ctx->marks[cur] & 3) && !(ctx->marks[cur] & 8)) {
					done = 0;
					active = 1;
					ctx->marks[cur] |= 8;
				}
				if (active) {
					struct dis_res *dres = do_dis(ctx, cur);
					dis_dopp(ctx, dres, cur + start);
					if (dres->oplen && !dres->endmark && !(ctx->marks[cur] & 4))
						cur += dres->oplen;
					else
						active = 0;
					dis_del_res(dres);
				} else {
					cur++;
				}
			}
		} while (!done);
	} else {
		while (cur < num) {
			struct dis_res *dres = do_dis(ctx, cur);
			dis_dopp(ctx, dres, cur + start);
			if (dres->oplen)
				cur += dres->oplen;
			else
				cur++;
			dis_del_res(dres);
		}
	}
	cur = 0;
	int active = 0;
	int skip = 0, nonzero = 0;
	while (cur < num) {
		int mark = ctx->marks[cur];
		if (ctx->names[cur]) {
			if (skip) {
				if (nonzero)
					fprintf(out, "%s[%x bytes skipped]\n", cols->err, skip);
				else
					fprintf(out, "%s[%x zero bytes skipped]\n", cols->reset, skip);
				skip = 0;
				nonzero = 0;
			}
			if (mark & 0x1b0)
				fprintf (out, "%s%s:\n", cols->reset, ctx->names[cur]);
			else if (mark & 2)
				fprintf (out, "\n%s%s:\n", cols->ctarg, ctx->names[cur]);
			else if (mark & 1)
				fprintf (out, "%s%s:\n", cols->btarg, ctx->names[cur]);
			else
				fprintf (out, "%s%s:\n", cols->reset, ctx->names[cur]);
		}
		if (mark & 0x1b0 && !active) {
			if (skip) {
				if (nonzero)
					fprintf(out, "%s[%x bytes skipped]\n", cols->err, skip);
				else
					fprintf(out, "%s[%x zero bytes skipped]\n", cols->reset, skip);
				skip = 0;
				nonzero = 0;
			}
			if (cbsz != 8)
				abort();
			fprintf (out, "%s%08x:%s", cols->mem, cur + start, cols->reset);
			if (mark & 0x80) {
				uint8_t val = code[cur];
				fprintf (out, " %s%02x\n", cols->num, val);
				cur += 1;
			} else if (mark & 0x100) {
				uint16_t val = 0;
				for (i = 0; i < 2 && cur + i < num; i++) {
					val |= code[cur + i] << i*8;
				}
				fprintf (out, " %s%04x\n", cols->num, val);
				cur += 2;
			} else if (mark & 0x10) {
				uint32_t val = 0;
				for (i = 0; i < 4 && cur + i < num; i++) {
					val |= code[cur + i] << i*8;
				}
				fprintf (out, " %s%08x\n", cols->num, val);
				cur += 4;
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
			for (i = 0; i < stride; i++)
				if (code[cur*stride+i])
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
		struct dis_res *dres = do_dis(ctx, cur);
		dis_dopp(ctx, dres, cur + start);

		if (dres->endmark || mark & 4)
			active = 0;

		if (mark & 2 && !ctx->names[cur])
			fprintf (out, "\n");
		switch (mark & 3) {
			case 0:
				if (!quiet)
					fprintf (out, "%s%08x:%s", cols->reset, cur + start, cols->reset);
				break;
			case 1:
				fprintf (out, "%s%08x:%s", cols->btarg, cur + start, cols->reset);
				break;
			case 2:
				fprintf (out, "%s%08x:%s", cols->ctarg, cur + start, cols->reset);
				break;
			case 3:
				fprintf (out, "%s%08x:%s", cols->bctarg, cur + start, cols->reset);
				break;
		}

		if (!quiet) {
			for (i = 0; i < isa->maxoplen; i += isa->opunit) {
				fprintf (out, " ");
				for (j = isa->opunit*stride - 1; j >= 0; j--)
					if (i+j/stride && i+j/stride >= dres->oplen) {
						fprintf (out, "  ");
					} else if (cur+i+j/stride >= num) {
						fprintf (out, "%s??", cols->err);
					} else {
						fprintf (out, "%s%02x", cols->reset, code[(cur + i)*stride + j]);
					}
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
			fprintf(out, " ");
		} else if (quiet == 1) {
			if (mark)
				fprintf (out, "\n");
		}

		easm_print_insn(out, cols, dres->insn);

		if (dres->status & DIS_STATUS_UNK_FORM) {
			fprintf (out, " %s[unknown op length]%s", cols->err, cols->reset);
		} else {
			int fl = 0;
			for (i = dres->oplen; i < MAXOPLEN * 8; i++)
				dres->a[i/8] &= ~(0xffull << (i & 7) * 8);
			for (i = 0; i < MAXOPLEN; i++) {
				dres->a[i] &= ~dres->m[i];
				if (dres->a[i])
					fl = 1;
			}
			if (fl) {
				fprintf (out, " %s[unknown:", cols->err);
				for (i = 0; i < dres->oplen || i == 0; i += isa->opunit) {
					fprintf (out, " ");
					for (j = isa->opunit*stride - 1; j >= 0; j--)
						if (cur+i+j >= num)
							fprintf (out, "??");
						else
							fprintf (out, "%02llx", (dres->a[(i+j)/8] >> ((i + j)&7) * 8) & 0xff);
				}
				fprintf (out, "]");
			}
		}
		if (dres->status & DIS_STATUS_EOF) {
			fprintf (out, " %s[incomplete]%s", cols->err, cols->reset);
		}
		if (dres->status & DIS_STATUS_UNK_INSN) {
			fprintf (out, " %s[unknown instruction]%s", cols->err, cols->reset);
		}
		if (dres->status & DIS_STATUS_UNK_OPERAND) {
			fprintf (out, " %s[unknown operand]%s", cols->err, cols->reset);
		}
		fprintf (out, "%s\n", cols->reset);
		cur += dres->oplen;

		dis_del_res(dres);
	}
	free(ctx->marks);
	free(ctx->names);
}
