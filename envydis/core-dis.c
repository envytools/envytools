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

#include "dis-intern.h"
#include "easm.h"
#include <stdlib.h>

struct disctx {
	const struct disisa *isa;
	struct varinfo *varinfo;
	uint8_t *code;
	int *marks;
	const char **names;
	uint32_t codebase;
	uint32_t codesz;
	int oplen;
	uint32_t pos;
	struct label *labels;
	int labelsnum;
	int labelsmax;
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

static inline struct litem *makeli(struct easm_expr *e) {
	struct litem *li = calloc(sizeof *li, 1);
	li->type = LITEM_EXPR;
	li->expr = e;
	return li;
}

static void mark(struct disctx *ctx, uint32_t ptr, int m) {
	if (ptr < ctx->codebase || ptr >= ctx->codebase + ctx->codesz / ctx->isa->posunit)
		return;
	ctx->marks[ptr - ctx->codebase] |= m;
}

static int is_nr_mark(struct disctx *ctx, uint32_t ptr) {
	if (ptr < ctx->codebase || ptr >= ctx->codebase + ctx->codesz / ctx->isa->posunit)
		return 0;
	return ctx->marks[ptr - ctx->codebase] & 0x40;
}

void atomtab_d DPROTO {
	const struct insn *tab = v;
	int i;
	while ((a[0]&tab->mask) != tab->val || !var_ok(tab->fmask, tab->ptype, ctx->varinfo))
		tab++;
	m[0] |= tab->mask;
	for (i = 0; i < 16; i++)
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
	li->type = LITEM_UNK;
	li->str = strdup(v);
	ADDARRAY(ctx->atoms, li);
}

void atomimm_d DPROTO {
	const struct bitfield *bf = v;
	struct easm_expr *expr = easm_expr_num(EASM_EXPR_NUM, GETBF(bf));
	ADDARRAY(ctx->atoms, makeli(expr));
}

void atomctarg_d DPROTO {
	const struct bitfield *bf = v;
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
			return;
		}
	}
}

void atombtarg_d DPROTO {
	const struct bitfield *bf = v;
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
			return;
		}
	}
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
			nex->str = aprintf("%s%lld", nex->str, GETBF(mem->idx));
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
			return;
		uint32_t num = 0;
		int j;
		for (j = 0; j < 4; j++)
			num |= ctx->code[(ptr - ctx->codebase) * ctx->isa->posunit + j] << j*8;
		expr = easm_expr_num(EASM_EXPR_NUM, num);
		expr->special = EASM_SPEC_MEM;
		ADDARRAY(ctx->atoms, makeli(expr));
	}
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

ull getbf(const struct bitfield *bf, ull *a, ull *m, ull cpos) {
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
		res += (cpos + bf->pospreadd) & -(1ull << bf->shr);
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
					atomtab_d (ctx, a, m, isa->troot);
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
			atomtab_d (ctx, a, m, isa->troot);
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
		atomtab_d (ctx, a, m, isa->troot);

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
