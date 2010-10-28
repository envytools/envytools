#include "envyas.h"
#include <libgen.h>

static struct disisa *envyas_isa = 0;

enum {
	OFMT_RAW,
	OFMT_HEX8,
	OFMT_HEX32,
	OFMT_CHEX8,
	OFMT_CHEX32,
} envyas_ofmt = OFMT_HEX8;

int envyas_ptype = -1;

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
	ctx->isa = envyas_isa;
	ctx->ptype = envyas_ptype;
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
		allok = 1;
		ctx->labelsnum = 0;
		pos = 0;
		for (i = 0; i < file->linesnum; i++) {
			switch (file->lines[i]->type) {
				case LINE_INSN:
					if (ctx->isa->i_need_nv50as_hack) {
						if (im[i].m[0].oplen == 8 && (pos & 7))
							pos &= ~7ull, pos += 8;
					}
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
						if (ctx->isa->i_need_nv50as_hack) {
							if (im[i].m[0].oplen == 8 && (pos & 7)) {
								fprintf (stderr, "FOOO!\n");
								j = i - 1;
								while (j != -1ull && file->lines[j]->type == LINE_LABEL)
									j--;
								assert (j != -1ull && file->lines[j]->type == LINE_INSN);
								if (im[j].m[0].oplen == 4) {
									im[j].m++;
									im[j].mnum--;
								} else {
								fprintf (stderr, "FNORD!\n");
								}
								allok = 0;
								pos &= ~7ull, pos += 8;
							}
						}
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
	if (envyas_ofmt == OFMT_RAW)
		fwrite (code, 1, pos, stdout);
	else {
		if (envyas_ofmt == OFMT_CHEX8) {
			printf ("uint8_t envyas_code[] = {\n");
		}
		if (envyas_ofmt == OFMT_CHEX32) {
			printf ("uint32_t envyas_code[] = {\n");
		}
		if (envyas_ofmt == OFMT_CHEX32 || envyas_ofmt == OFMT_HEX32) {
			for (i = 0; i < pos; i+=4) {
				uint32_t val;
				val = code[i] | code[i+1] << 8 | code[i+2] << 16 | code[i+3] << 24;
				if (envyas_ofmt == OFMT_CHEX8 || envyas_ofmt == OFMT_CHEX32)
					printf ("\t");
				printf ("0x%08x,\n", val);
			}
		} else {
			for (i = 0; i < pos; i++) {
				if (envyas_ofmt == OFMT_CHEX8 || envyas_ofmt == OFMT_CHEX32)
					printf ("\t");
				printf ("0x%02x,\n", code[i]);
			}
		}
		if (envyas_ofmt == OFMT_CHEX8 || envyas_ofmt == OFMT_CHEX32) {
			printf ("};\n");
		}
	}
}

int main(int argc, char **argv) {
	argv[0] = basename(argv[0]);
	if (!strcmp(argv[0], "nv50as")) {
		envyas_isa = nv50_isa;
		envyas_ofmt = OFMT_HEX32;
	}
	if (!strcmp(argv[0], "nvc0as")) {
		envyas_isa = nvc0_isa;
		envyas_ofmt = OFMT_HEX32;
	}
	if (!strcmp(argv[0], "ctxas")) {
		envyas_isa = ctx_isa;
		envyas_ofmt = OFMT_HEX32;
	}
	if (!strcmp(argv[0], "fucas")) {
		envyas_isa = fuc_isa;
	}
	if (!strcmp(argv[0], "macroas")) {
		envyas_isa = macro_isa;
		envyas_ofmt = OFMT_HEX32;
	}
	int ptype = -1;
	int c;
	unsigned base = 0, skip = 0, limit = 0;
	while ((c = getopt (argc, argv, "45vgfpcsbm:wi")) != -1)
		switch (c) {
			case '4':
				ptype = NV4x;
				break;
			case '5':
				ptype = NV5x;
				break;
			case 'v':
				ptype = VP;
				break;
			case 'g':
				ptype = GP;
				break;
			case 'f':
			case 'p':
				ptype = FP;
				break;
			case 'c':
				ptype = CP;
				break;
			case 's':
				ptype = VP|GP|FP;
				break;
			case 'b':
				if (envyas_ofmt == OFMT_HEX32)
					envyas_ofmt = OFMT_CHEX32;
				else
					envyas_ofmt = OFMT_CHEX8;
				break;
			case 'w':
				if (envyas_ofmt == OFMT_CHEX8)
					envyas_ofmt = OFMT_CHEX32;
				else
					envyas_ofmt = OFMT_HEX32;
				break;
			case 'i':
				envyas_ofmt = OFMT_RAW;
				break;
			case 'm':
				if (!strcmp(optarg, "nv50"))
					envyas_isa = nv50_isa;
				else if (!strcmp(optarg, "nvc0"))
					envyas_isa = nvc0_isa;
				else if (!strcmp(optarg, "ctx"))
					envyas_isa = ctx_isa;
				else if (!strcmp(optarg, "fuc"))
					envyas_isa = fuc_isa;
				else if (!strcmp(optarg, "macro"))
					envyas_isa = macro_isa;
				else {
					fprintf (stderr, "Unknown architecure \"%s\"!\n", optarg);
					return 1;
				}
				break;
		}
	if (!envyas_isa) {
		fprintf (stderr, "No architecture specified!\n");
		return 1;
	}
	return yyparse();
}
