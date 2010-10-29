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
				fprintf (stderr, "Division by 0\n");
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

int resolve (struct disctx *ctx, ull *val, struct match m, ull pos) {
	int i;
	for (i = 0; i < m.nrelocs; i++) {
		ull val = calc(m.relocs[i].expr, ctx);
		const struct bitfield *bf = m.relocs[i].bf;
		ull num = val - bf->addend;
		if (bf->pcrel)
			num -= (pos + bf->pospreadd) & -(1ull << bf->shr);
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

struct section {
	const char *name;
	ull base;
	uint8_t *code;
	int pos;
	int maxpos;
};

void extend(struct section *s, int add) {
	while (s->pos + add >= s->maxpos) {
		if (!s->maxpos)
			s->maxpos = 256;
		else
			s->maxpos *= 2;
		s->code = realloc (s->code, s->maxpos);
	}
}

int donum (struct section *s, struct line *line, struct disctx *ctx, int wren) {
	if (line->str[1] != 'b' && line->str[1] != 's' && line->str[1] != 'u')
		return 0;
	char *end;
	ull bits = strtoull(line->str+2, &end, 0);
	if (*end)
		return 0;
	if (bits > 64 || bits & 7 || !bits)
		return 0;
	int i;
	for (i = 0; i < line->atomsnum; i++)
		if (!line->atoms[i]->isimm) {
			fprintf (stderr, "Wrong arguments for %s\n", line->str);
			exit(1);
		}
	if (wren) {
		extend(s, bits/8 * line->atomsnum);
		for (i = 0; i < line->atomsnum; i++) {
			ull num = calc(line->atoms[i], ctx);
			if ((line->str[1] == 'u' && bits != 64 && num >= (1ull << bits))
				|| (line->str[1] == 's' && bits != 64 && num >= (1ull << bits - 1) && num < (-1ull << bits - 1))) {
				fprintf (stderr, "Argument %d too large for %s\n", i, line->str);
				exit(1);
			}
			int j;
			for (j = 0; j < bits/8; j++)
				s->code[s->pos+j] = num >> (8*j);
			
		}
	}
	s->pos += bits/8 * line->atomsnum;
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
	int allok = 1;
	struct section *sections = 0;
	int sectionsnum = 0;
	int sectionsmax = 0;
	do {
		allok = 1;
		ctx->labelsnum = 0;
		for (i = 0; i < sectionsnum; i++)
			free(sections[i].code);
		sectionsnum = 0;
		int cursect = 0;
		struct section def = { "default" };
		RNN_ADDARRAY(sections, def);
		for (i = 0; i < file->linesnum; i++) {
			switch (file->lines[i]->type) {
				case LINE_INSN:
					if (ctx->isa->i_need_nv50as_hack) {
						if (im[i].m[0].oplen == 8 && (sections[cursect].pos & 7))
							sections[cursect].pos &= ~7ull, sections[cursect].pos += 8;
					}
					sections[cursect].pos += im[i].m[0].oplen;
					break;
				case LINE_LABEL:
					for (j = 0; j < ctx->labelsnum; j++) {
						if (!strcmp(ctx->labels[j].name, file->lines[i]->str)) {
							fprintf (stderr, "Label %s redeclared!\n", file->lines[i]->str);
							return 1;
						}
					}
					struct label l = { file->lines[i]->str, sections[cursect].pos / ctx->isa->posunit + sections[cursect].base };
					RNN_ADDARRAY(ctx->labels, l);
					break;
				case LINE_DIR:
					if (!strcmp(file->lines[i]->str, ".section")) {
						if (file->lines[i]->atomsnum > 2) {
							fprintf (stderr, "Too many arguments for .section\n");
							return 1;
						}
						if (file->lines[i]->atoms[0]->type != EXPR_ID || (file->lines[i]->atomsnum == 2 && file->lines[i]->atoms[1]->type != EXPR_NUM)) {
							fprintf (stderr, "Wrong arguments for .section\n");
							return 1;
						}
						for (j = 0; j < sectionsnum; j++)
							if (!strcmp(sections[j].name, file->lines[i]->atoms[0]->str))
								break;
						if (j == sectionsnum) {
							struct section s = { file->lines[i]->atoms[0]->str };
							if (file->lines[i]->atomsnum == 2)
								s.base = file->lines[i]->atoms[1]->num1;
							RNN_ADDARRAY(sections, s);
						}
						cursect = j;
					} else if (!strcmp(file->lines[i]->str, ".align")) {
						if (file->lines[i]->atomsnum > 1) {
							fprintf (stderr, "Too many arguments for .align\n");
							return 1;
						}
						if (file->lines[i]->atoms[0]->type != EXPR_NUM) {
							fprintf (stderr, "Wrong arguments for .align\n");
							return 1;
						}
						ull num = file->lines[i]->atoms[0]->num1;
						sections[cursect].pos += num - 1;
						sections[cursect].pos /= num;
						sections[cursect].pos *= num;
					} else if (!strcmp(file->lines[i]->str, ".skip")) {
						if (file->lines[i]->atomsnum > 1) {
							fprintf (stderr, "Too many arguments for .skip\n");
							return 1;
						}
						if (file->lines[i]->atoms[0]->type != EXPR_NUM) {
							fprintf (stderr, "Wrong arguments for .skip\n");
							return 1;
						}
						ull num = file->lines[i]->atoms[0]->num1;
						sections[cursect].pos += num;
					} else if (!strcmp(file->lines[i]->str, ".equ")) {
						if (file->lines[i]->atomsnum != 2
							|| file->lines[i]->atoms[0]->type != EXPR_ID
							|| !file->lines[i]->atoms[1]->isimm) {
							fprintf (stderr, "Wrong arguments for .equ\n");
							return 1;
						}
						ull num = calc(file->lines[i]->atoms[1], ctx);
						for (j = 0; j < ctx->labelsnum; j++) {
							if (!strcmp(ctx->labels[j].name, file->lines[i]->atoms[0]->str)) {
								fprintf (stderr, "Label %s redeclared!\n", file->lines[i]->atoms[0]->str);
								return 1;
							}
						}
						struct label l = { file->lines[i]->atoms[0]->str, num };
						RNN_ADDARRAY(ctx->labels, l);
					} else if (!donum(&sections[cursect], file->lines[i], ctx, 0)) {
						fprintf (stderr, "Unknown directive %s\n", file->lines[i]->str);
						return 1;
					}
					break;
			}
		}
		cursect = 0;
		for (j = 0; j < sectionsnum; j++)
			sections[j].pos = 0;
		ull val;
		for (i = 0; i < file->linesnum; i++) {
			switch (file->lines[i]->type) {
				case LINE_INSN:
					if (!resolve(ctx, &val, im[i].m[0], sections[cursect].pos / ctx->isa->posunit + sections[cursect].base)) {
						sections[cursect].pos += im[i].m[0].oplen;
						im[i].m++;
						im[i].mnum--;
						if (!im[i].mnum) {
							fprintf (stderr, "Relocation failed on insn %d!\n", i+1);
							return 1;
						}
						allok = 0;
					} else {
						if (ctx->isa->i_need_nv50as_hack) {
							if (im[i].m[0].oplen == 8 && (sections[cursect].pos & 7)) {
								j = i - 1;
								while (j != -1ull && file->lines[j]->type == LINE_LABEL)
									j--;
								assert (j != -1ull && file->lines[j]->type == LINE_INSN);
								if (im[j].m[0].oplen == 4) {
									im[j].m++;
									im[j].mnum--;
								}
								allok = 0;
								sections[cursect].pos &= ~7ull, sections[cursect].pos += 8;
							}
						}
						extend(&sections[cursect], im[i].m[0].oplen);
						for (j = 0; j < im[i].m[0].oplen; j++)
							sections[cursect].code[sections[cursect].pos++] = val >> (8*j);
					}
					break;
				case LINE_LABEL:
					break;
				case LINE_DIR:
					if (!strcmp(file->lines[i]->str, ".section")) {
						for (j = 0; j < sectionsnum; j++)
							if (!strcmp(sections[j].name, file->lines[i]->atoms[0]->str))
								break;
						cursect = j;
					} else if (!strcmp(file->lines[i]->str, ".align")) {
						ull num = file->lines[i]->atoms[0]->num1;
						ull oldpos = sections[cursect].pos;
						sections[cursect].pos += num - 1;
						sections[cursect].pos /= num;
						sections[cursect].pos *= num;
						extend(&sections[cursect], 0);
						for (j = oldpos; j < sections[cursect].pos; j++)
							sections[cursect].code[j] = 0;
					} else if (!strcmp(file->lines[i]->str, ".skip")) {
						ull num = file->lines[i]->atoms[0]->num1;
						ull oldpos = sections[cursect].pos;
						sections[cursect].pos += num;
						extend(&sections[cursect], 0);
						for (j = oldpos; j < sections[cursect].pos; j++)
							sections[cursect].code[j] = 0;
					} else if (!strcmp(file->lines[i]->str, ".equ")) {
						/* nothing to be done */
					} else if (!donum(&sections[cursect], file->lines[i], ctx, 1)) {
						fprintf (stderr, "Unknown directive %s\n", file->lines[i]->str);
						return 1;
					}
			}
		}
	} while (!allok);
	for (i = 0; i < sectionsnum; i++) {
		if (!strcmp(sections[i].name, "default") && !sections[i].pos)
			continue;
		if (envyas_ofmt == OFMT_RAW)
			fwrite (sections[i].code, 1, sections[i].pos, stdout);
		else {
			if (envyas_ofmt == OFMT_CHEX8) {
				printf ("uint8_t %s[] = {\n", sections[i].name);
			}
			if (envyas_ofmt == OFMT_CHEX32) {
				printf ("uint32_t %s[] = {\n", sections[i].name);
			}
			if (envyas_ofmt == OFMT_CHEX32 || envyas_ofmt == OFMT_HEX32) {
				for (j = 0; j < sections[i].pos; j+=4) {
					uint32_t val;
					val = sections[i].code[j] | sections[i].code[j+1] << 8 | sections[i].code[j+2] << 16 | sections[i].code[j+3] << 24;
					if (envyas_ofmt == OFMT_CHEX8 || envyas_ofmt == OFMT_CHEX32)
						printf ("\t");
					printf ("0x%08x,\n", val);
				}
			} else {
				for (j = 0; j < sections[i].pos; j++) {
					if (envyas_ofmt == OFMT_CHEX8 || envyas_ofmt == OFMT_CHEX32)
						printf ("\t");
					printf ("0x%02x,\n", sections[i].code[j]);
				}
			}
			if (envyas_ofmt == OFMT_CHEX8 || envyas_ofmt == OFMT_CHEX32) {
				printf ("};\n");
				if (i != sectionsnum - 1)
					printf("\n");
			}
		}
	}
	return 0;
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
	if (!strcmp(argv[0], "vp2as")) {
		envyas_isa = vp2_isa;
	}
	if (!strcmp(argv[0], "macroas")) {
		envyas_isa = macro_isa;
		envyas_ofmt = OFMT_HEX32;
	}
	int ptype = -1;
	int c;
	unsigned base = 0, skip = 0, limit = 0;
	while ((c = getopt (argc, argv, "45vgfpcsam:wi")) != -1)
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
			case 'a':
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
				else if (!strcmp(optarg, "vp2"))
					envyas_isa = vp2_isa;
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
