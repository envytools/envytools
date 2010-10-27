#include "envyas.h"

ull calc (const struct expr *expr, struct disctx *ctx) {
	int i;
	ull x;
	switch (expr->type) {
		case EXPR_NUM:
			return expr->num1;
		case EXPR_ID:
			for (i = 0; i < ctx->labelsnum; i++)
				if (!strcmp(ctx->labels[i].name, expr->str))
					return ctx->labels[i].val;
			fprintf (stderr, "Undefined label \"%s\"\n", expr->str);
			exit(1);
		case EXPR_NEG:
			return -calc(expr->expr1, ctx);
		case EXPR_NOT:
			return ~calc(expr->expr1, ctx);
		case EXPR_MUL:
			return calc(expr->expr1, ctx) * calc(expr->expr2, ctx);
		case EXPR_DIV:
			x = calc(expr->expr2, ctx);
			if (x)
				return calc(expr->expr1, ctx) / x;
			else {
				fprintf (stderr, "Division by 0\n", expr->str);
				exit(1);
			}
		case EXPR_ADD:
			return calc(expr->expr1, ctx) + calc(expr->expr2, ctx);
		case EXPR_SUB:
			return calc(expr->expr1, ctx) - calc(expr->expr2, ctx);
		case EXPR_SHL:
			return calc(expr->expr1, ctx) << calc(expr->expr2, ctx);
		case EXPR_SHR:
			return calc(expr->expr1, ctx) >> calc(expr->expr2, ctx);
		case EXPR_AND:
			return calc(expr->expr1, ctx) & calc(expr->expr2, ctx);
		case EXPR_XOR:
			return calc(expr->expr1, ctx) ^ calc(expr->expr2, ctx);
		case EXPR_OR:
			return calc(expr->expr1, ctx) | calc(expr->expr2, ctx);
		default:
			assert(0);
	}
}

int resolve (struct disctx *ctx, ull *val, struct match m, int pos) {
	int i;
	for (i = 0; i < m.nrelocs; i++) {
		ull val = calc(m.relocs[i].expr, ctx);
		const struct bitfield *bf = m.relocs[i].bf;
		ull num = val - bf->addend;
		if (bf->pcrel)
			num -= (pos / ctx->isa->posunit + bf->pospreadd) & -(1ull << bf->shr);
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
		setsbf(&m, bf->sbf[0].pos, bf->sbf[0].len, num);
		num >>= bf->sbf[0].len;
		setsbf(&m, bf->sbf[1].pos, bf->sbf[1].len, num);
		ctx->pos = pos;
		if (getbf(bf, &m.a, &m.m, ctx) != val)
			return 0;
	}
	*val = m.a;
	return 1;
}

int envyas_process(struct file *file) {
	int i, j;
	struct disctx ctx_s = { 0 };
	struct disctx *ctx = &ctx_s;
	ctx->reverse = 1;
	ctx->isa = macro_isa;
	ctx->ptype = -1;
	struct matches *im = calloc(sizeof *im, file->linesnum);
	for (i = 0; i < file->linesnum; i++) {
		if (file->lines[i]->type == LINE_INSN) {
			ctx->line = file->lines[i];
			struct matches *m = atomtab(ctx, 0, 0, ctx->isa->troot, 0);
			for (j = 0; j < m->mnum; j++)
				if (m->m[j].lpos == ctx->line->atomsnum) {
					RNN_ADDARRAY(im[i].m, m->m[j]);
				}
			if (!im[i].mnum) {
				fprintf (stderr, "No match on insn %d!\n", i+1);
				return 1;
			}
#if 0
			if (im[i].mnum > 2 || (im[i].mnum == 2 && im[i].m[0].oplen == im[i].m[1].oplen)) {
				fprintf (stderr, "warning: multiple matches on insn %d:\n", i+1);
				for (j = 0; j < im[i].mnum; j++)
					fprintf (stderr, "%d	%016llx	%016llx\n", im[i].m[j].oplen, im[i].m[j].a, im[i].m[j].m);
			}
#endif
		}
	}
	int pos;
	int maxpos = 256;
	uint8_t *code = malloc (maxpos);
	int allok = 1;
	do {
		ctx->labelsnum = 0;
		pos = 0;
		for (i = 0; i < file->linesnum; i++) {
			switch (file->lines[i]->type) {
				case LINE_INSN:
					pos += im[i].m[0].oplen;
					break;
				case LINE_LABEL:
					for (j = 0; j < ctx->labelsnum; j++) {
						if (!strcmp(ctx->labels[j].name, file->lines[i]->str)) {
							fprintf (stderr, "Label %s redeclared!\n", file->lines[i]->str);
							return 1;
						}
					}
					struct label l = { file->lines[i]->str, pos / ctx->isa->posunit };
					RNN_ADDARRAY(ctx->labels, l);
					break;
			}
		}
		pos = 0;
		ull val;
		for (i = 0; i < file->linesnum; i++) {
			switch (file->lines[i]->type) {
				case LINE_INSN:
					if (!resolve(ctx, &val, im[i].m[0], pos)) {
						pos += im[i].m[0].oplen;
						im[i].m++;
						im[i].mnum--;
						if (!im[i].mnum) {
							fprintf (stderr, "Relocation failed on insn %d!\n", i+1);
							return 1;
						}
						allok = 0;
					} else {
						if (pos + im[i].m[0].oplen >= maxpos) maxpos *= 2, code = realloc (code, maxpos);
						for (j = 0; j < im[i].m[0].oplen; j++)
							code[pos+j] = val >> (8*j);
						pos += im[i].m[0].oplen;
					}
					break;
				case LINE_LABEL:
					break;
			}
		}

	} while (!allok);
	for (i = 0; i < pos; i+=4) {
		uint32_t val;
		val = code[i] | code[i+1] << 8 | code[i+2] << 16 | code[i+3] << 24;
		printf ("0x%08x,\n", val);
	}
}

int main() {
	return yyparse();
}
