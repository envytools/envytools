/*
 * Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "envyas.h"
#include "symtab.h"
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

struct asctx {
	const struct disisa *isa;
	struct varinfo *varinfo;
	struct label *labels;
	int labelsnum;
	int labelsmax;
	struct symtab *symtab;
	const char *cur_global_label;
	uint32_t pos;
	struct section *sections;
	int sectionsnum;
	int sectionsmax;
	struct matches *im;
};

enum envyas_ofmt {
	OFMT_RAW,
	OFMT_HEX8,
	OFMT_HEX32,
	OFMT_HEX64,
	OFMT_CHEX8,
	OFMT_CHEX32,
	OFMT_CHEX64,
};

static char* expand_local_label(const char *local, const char *global) {
	char *full = malloc((global ? strlen(global) : 0) + strlen(local) + 3);
	full[0] = '_';
	full[1] = '_';
	if (global)
		strcpy(full + 2, global);
	else
		full[2] = '\0';
	strcat(full, local);
	return full;
}

ull calc (struct easm_expr *expr, struct asctx *ctx) {
	int res;
	ull x;
	switch (expr->type) {
		case EASM_EXPR_NUM:
			return expr->num;
		case EASM_EXPR_LABEL:
			if (expr->str[0] == '_' && expr->str[1] != '_') {
				char *full_label = expand_local_label(expr->str, ctx->cur_global_label);
				free(expr->str);
				expr->str = full_label;
			}
			if (symtab_get(ctx->symtab, expr->str, 0, &res) != -1) {
				return ctx->labels[res].val;
			}
			fprintf (stderr, LOC_FORMAT(expr->loc, "Undefined label \"%s\"\n"), expr->str);
			exit(1);
		case EASM_EXPR_NEG:
			return -calc(expr->e1, ctx);
		case EASM_EXPR_NOT:
			return ~calc(expr->e1, ctx);
		case EASM_EXPR_LNOT:
			return !calc(expr->e1, ctx);
		case EASM_EXPR_MUL:
			return calc(expr->e1, ctx) * calc(expr->e2, ctx);
		case EASM_EXPR_DIV:
			x = calc(expr->e2, ctx);
			if (x)
				return calc(expr->e1, ctx) / x;
			else {
				fprintf (stderr, LOC_FORMAT(expr->loc, "Division by 0\n"));
				exit(1);
			}
		case EASM_EXPR_MOD:
			x = calc(expr->e2, ctx);
			if (x)
				return calc(expr->e1, ctx) % x;
			else {
				fprintf (stderr, LOC_FORMAT(expr->loc, "Division by 0\n"));
				exit(1);
			}
		case EASM_EXPR_ADD:
			return calc(expr->e1, ctx) + calc(expr->e2, ctx);
		case EASM_EXPR_SUB:
			return calc(expr->e1, ctx) - calc(expr->e2, ctx);
		case EASM_EXPR_SHL:
			return calc(expr->e1, ctx) << calc(expr->e2, ctx);
		case EASM_EXPR_SHR:
			return calc(expr->e1, ctx) >> calc(expr->e2, ctx);
		case EASM_EXPR_AND:
			return calc(expr->e1, ctx) & calc(expr->e2, ctx);
		case EASM_EXPR_XOR:
			return calc(expr->e1, ctx) ^ calc(expr->e2, ctx);
		case EASM_EXPR_OR:
			return calc(expr->e1, ctx) | calc(expr->e2, ctx);
		case EASM_EXPR_LAND:
			return calc(expr->e1, ctx) && calc(expr->e2, ctx);
		case EASM_EXPR_LOR:
			return calc(expr->e1, ctx) || calc(expr->e2, ctx);
		default:
			abort();
	}
}

int resolve (struct asctx *ctx, ull *val, struct match m, ull pos) {
	int i;
	for (i = 0; i < m.nrelocs; i++) {
		ull val = calc(m.relocs[i].expr, ctx);
		const struct rbitfield *bf = m.relocs[i].bf;
		ull num = val - bf->addend;
		if (bf->pcrel)
			num -= (pos + bf->pospreadd) & -(1ull << bf->shr);
		num >>= bf->shr;
		setsbf(&m, bf->sbf[0].pos, bf->sbf[0].len, num);
		num >>= bf->sbf[0].len;
		setsbf(&m, bf->sbf[1].pos, bf->sbf[1].len, num);
		ctx->pos = pos;
		ull mask = ~0ull;
		ull totalsz = bf->shr + bf->sbf[0].len + bf->sbf[1].len;
		if (bf->wrapok && totalsz < 64)
			mask = (1ull << totalsz) - 1;
		if ((getrbf_as(bf, m.a, m.m, ctx->pos) & mask) != (val & mask))
			return 0;
	}
	for (i = 0; i < MAXOPLEN; i++)
		val[i] = m.a[i];
	return 1;
}

struct section {
	const char *name;
	ull base;
	uint8_t *code;
	int pos;
	int maxpos;
	int first_label, last_label;
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

int donum (struct section *s, struct easm_directive *direct, struct asctx *ctx, int wren) {
	if (direct->str[0] != 'b' && direct->str[0] != 's' && direct->str[0] != 'u')
		return 0;
	char *end;
	ull bits = strtoull(direct->str+1, &end, 0);
	if (*end)
		return 0;
	if (bits > 64 || bits & 7 || !bits)
		return 0;
	int i;
	if (wren) {
		extend(s, bits/8 * direct->paramsnum);
		if (s->pos % (bits/8))
			fprintf (stderr, LOC_FORMAT(direct->loc, "Warning: Unaligned data .%s; section '%s', offset 0x%x\n"), direct->str, s->name, s->pos);
	}
	for (i = 0; i < direct->paramsnum; i++) {
		if (!easm_isimm(direct->params[i])) {
			fprintf (stderr, LOC_FORMAT(direct->loc, "Wrong arguments for .%s\n"), direct->str);
			exit(1);
		}
		if (wren) {
			ull num = calc(direct->params[i], ctx);
			if ((direct->str[0] == 'u' && bits != 64 && num >= (1ull << bits))
				|| (direct->str[0] == 's' && bits != 64 && num >= (1ull << (bits - 1)) && num < (-1ull << (bits - 1)))) {
				fprintf (stderr, LOC_FORMAT(direct->loc, "Argument %d too large for .%s\n"), i, direct->str);
				exit(1);
			}
			int j;
			for (j = 0; j < bits/8; j++)
				s->code[s->pos+j + i * bits/8] = num >> (8*j);
			
		}
	}
	s->pos += bits/8 * direct->paramsnum;
	return 1;
}

int find_label(struct asctx *ctx, struct section *sect, int ofs, int start_at) {
	int i;
	/* Doesn't have any labels */
	if (sect->first_label == -1)
		return -1;
	for (i = start_at >= 0 ? start_at : sect->first_label; i <= sect->last_label; i++) {
		if (ctx->labels[i].type == 0 && ctx->labels[i].val == ofs)
			return i;
	}
	return -1;
}

int envyas_process(struct asctx *ctx, struct easm_file *file) {
	int i;
	ctx->im = calloc(sizeof *ctx->im, file->linesnum);
	for (i = 0; i < file->linesnum; i++) {
		if (file->lines[i]->type == EASM_LINE_INSN) {
			ctx->im[i] = *do_as(ctx->isa, ctx->varinfo, file->lines[i]->insn);
			if (!ctx->im[i].mnum) {
				fprintf (stderr, LOC_FORMAT(file->lines[i]->loc, "No match\n"));
				return 1;
			}
		}
	}
	return 0;
}

int envyas_layout(struct asctx *ctx, struct easm_file *file) {
	int i, j;
	int allok = 1;
	int stride = ed_getcstride(ctx->isa, ctx->varinfo);
	ctx->symtab = symtab_new();
	do {
		symtab_del(ctx->symtab);
		ctx->symtab = symtab_new();
		allok = 1;
		ctx->labelsnum = 0;
		ctx->cur_global_label = NULL;
		for (i = 0; i < ctx->sectionsnum; i++)
			free(ctx->sections[i].code);
		ctx->sectionsnum = 0;
		int cursect = 0;
		struct section def = { "default" };
		def.first_label = -1;
		ADDARRAY(ctx->sections, def);
		for (i = 0; i < file->linesnum; i++) {
			struct easm_directive *direct = file->lines[i]->directive;
			switch (file->lines[i]->type) {
				case EASM_LINE_INSN:
					if (ctx->isa->i_need_g80as_hack) {
						if (ctx->im[i].m[0].oplen == 8 && (ctx->sections[cursect].pos & 7))
							ctx->sections[cursect].pos &= ~7ull, ctx->sections[cursect].pos += 8;
					}
					ctx->sections[cursect].pos += ctx->im[i].m[0].oplen * stride;
					break;
				case EASM_LINE_LABEL:
					if (file->lines[i]->lname[0] == '_' && file->lines[i]->lname[1] != '_') {
						char *full_label = expand_local_label(file->lines[i]->lname, ctx->cur_global_label);
						free(file->lines[i]->lname);
						file->lines[i]->lname = full_label;
					}
					else
						ctx->cur_global_label = file->lines[i]->lname;

					if (symtab_put(ctx->symtab, file->lines[i]->lname, 0, ctx->labelsnum) == -1) {
						fprintf (stderr, LOC_FORMAT(file->lines[i]->loc, "Label %s redeclared!\n"), file->lines[i]->lname);
						return 1;
					}
					struct label l = { file->lines[i]->lname, ctx->sections[cursect].pos / stride + ctx->sections[cursect].base };
					if (ctx->sections[cursect].first_label < 0)
						ctx->sections[cursect].first_label = ctx->labelsnum;
					ctx->sections[cursect].last_label = ctx->labelsnum;
					ADDARRAY(ctx->labels, l);
					break;
				case EASM_LINE_DIRECTIVE:
					if (!strcmp(direct->str, "section")) {
						if (direct->paramsnum > 2) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Too many arguments for .section\n"));
							return 1;
						}
						if (direct->params[0]->type != EASM_EXPR_LABEL || (direct->paramsnum == 2 && direct->params[1]->type != EASM_EXPR_NUM)) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Wrong arguments for .section\n"));
							return 1;
						}
						for (j = 0; j < ctx->sectionsnum; j++)
							if (!strcmp(ctx->sections[j].name, direct->params[0]->str))
								break;
						if (j == ctx->sectionsnum) {
							struct section s = { direct->params[0]->str };
							s.first_label = -1;
							if (direct->paramsnum == 2)
								s.base = direct->params[1]->num;
							ADDARRAY(ctx->sections, s);
						}
						cursect = j;
					} else if (!strcmp(direct->str, "align")) {
						if (direct->paramsnum > 1) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Too many arguments for .align\n"));
							return 1;
						}
						if (direct->params[0]->type != EASM_EXPR_NUM) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Wrong arguments for .align\n"));
							return 1;
						}
						ull num = direct->params[0]->num;
						ctx->sections[cursect].pos += num - 1;
						ctx->sections[cursect].pos /= num;
						ctx->sections[cursect].pos *= num;
					} else if (!strcmp(direct->str, "size")) {
						if (direct->paramsnum > 1) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Too many arguments for .size\n"));
							return 1;
						}
						if (direct->params[0]->type != EASM_EXPR_NUM) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Wrong arguments for .size\n"));
							return 1;
						}
						ull num = direct->params[0]->num;
						if (ctx->sections[cursect].pos > num) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Section '%s' exceeds .size by %llu bytes\n"), ctx->sections[cursect].name, ctx->sections[cursect].pos - num);
							return 1;
						}
						ctx->sections[cursect].pos = num;
					} else if (!strcmp(direct->str, "skip")) {
						if (direct->paramsnum > 1) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Too many arguments for .skip\n"));
							return 1;
						}
						if (direct->params[0]->type != EASM_EXPR_NUM) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Wrong arguments for .skip\n"));
							return 1;
						}
						ull num = direct->params[0]->num;
						ctx->sections[cursect].pos += num;
					} else if (!strcmp(direct->str, "equ")) {
						if (direct->paramsnum != 2
							|| direct->params[0]->type != EASM_EXPR_LABEL
							|| !easm_isimm(direct->params[1])) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Wrong arguments for .equ\n"));
							return 1;
						}
						if (direct->params[0]->str[0] == '_' && direct->params[0]->str[1] != '_') {
							char *full_label = expand_local_label(direct->params[0]->str, ctx->cur_global_label);
							free((void*)direct->params[0]->str);
							direct->params[0]->str = full_label;
						}
						ull num = calc(direct->params[1], ctx);
						if (symtab_put(ctx->symtab, direct->params[0]->str, 0, ctx->labelsnum) == -1) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Label %s redeclared!\n"), direct->params[0]->str);
							return 1;
						}
						struct label l = { direct->params[0]->str, num , /* Distinguish .equ labels from regular labels */ 1 };
						ADDARRAY(ctx->labels, l);
					} else if (!donum(&ctx->sections[cursect], direct, ctx, 0)) {
						fprintf (stderr, LOC_FORMAT(direct->loc, "Unknown directive .%s\n"), direct->str);
						return 1;
					}
					break;
			}
		}
		cursect = 0;
		ctx->cur_global_label = NULL;
		for (j = 0; j < ctx->sectionsnum; j++)
			ctx->sections[j].pos = 0;
		ull val[MAXOPLEN];
		for (i = 0; i < file->linesnum; i++) {
			struct easm_directive *direct = file->lines[i]->directive;
			switch (file->lines[i]->type) {
				case EASM_LINE_INSN:
					if (!resolve(ctx, val, ctx->im[i].m[0], ctx->sections[cursect].pos / stride + ctx->sections[cursect].base)) {
						ctx->sections[cursect].pos += ctx->im[i].m[0].oplen * stride;
						ctx->im[i].m++;
						ctx->im[i].mnum--;
						if (!ctx->im[i].mnum) {
							fprintf (stderr, LOC_FORMAT(file->lines[i]->loc, "Relocation failed\n"));
							return 1;
						}
						allok = 0;
					} else {
						if (ctx->isa->i_need_g80as_hack) {
							if (ctx->im[i].m[0].oplen == 8 && (ctx->sections[cursect].pos & 7)) {
								j = i - 1;
								while (j != -1ull && file->lines[j]->type == EASM_LINE_LABEL)
									j--;
								assert (j != -1ull && file->lines[j]->type == EASM_LINE_INSN);
								if (ctx->im[j].m[0].oplen == 4) {
									ctx->im[j].m++;
									ctx->im[j].mnum--;
								}
								allok = 0;
								ctx->sections[cursect].pos &= ~7ull, ctx->sections[cursect].pos += 8;
							}
						}
						extend(&ctx->sections[cursect], ctx->im[i].m[0].oplen * stride);
						for (j = 0; j < ctx->im[i].m[0].oplen * stride; j++)
							ctx->sections[cursect].code[ctx->sections[cursect].pos++] = val[j>>3] >> (8*(j&7));
					}
					break;
				case EASM_LINE_LABEL:
					if (file->lines[i]->lname[0] != '_')
						ctx->cur_global_label = file->lines[i]->lname;
					break;
				case EASM_LINE_DIRECTIVE:
					if (!strcmp(direct->str, "section")) {
						for (j = 0; j < ctx->sectionsnum; j++)
							if (!strcmp(ctx->sections[j].name, direct->params[0]->str))
								break;
						cursect = j;
					} else if (!strcmp(direct->str, "align")) {
						ull num = direct->params[0]->num;
						ull oldpos = ctx->sections[cursect].pos;
						ctx->sections[cursect].pos += num - 1;
						ctx->sections[cursect].pos /= num;
						ctx->sections[cursect].pos *= num;
						extend(&ctx->sections[cursect], 0);
						for (j = oldpos; j < ctx->sections[cursect].pos; j++)
							ctx->sections[cursect].code[j] = 0;
					} else if (!strcmp(direct->str, "size")) {
						ull num = direct->params[0]->num;
						ull oldpos = ctx->sections[cursect].pos;
						if (ctx->sections[cursect].pos > num) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Section '%s' exceeds .size by %llu bytes\n"), ctx->sections[cursect].name, ctx->sections[cursect].pos - num);
							return 1;
						}
						ctx->sections[cursect].pos = num;
						extend(&ctx->sections[cursect], 0);
						for (j = oldpos; j < ctx->sections[cursect].pos; j++)
							ctx->sections[cursect].code[j] = 0;
					} else if (!strcmp(direct->str, "skip")) {
						ull num = direct->params[0]->num;
						ull oldpos = ctx->sections[cursect].pos;
						ctx->sections[cursect].pos += num;
						extend(&ctx->sections[cursect], 0);
						for (j = oldpos; j < ctx->sections[cursect].pos; j++)
							ctx->sections[cursect].code[j] = 0;
					} else if (!strcmp(direct->str, "equ")) {
						/* nothing to be done */
					} else if (!donum(&ctx->sections[cursect], direct, ctx, 1)) {
						fprintf (stderr, LOC_FORMAT(direct->loc, "Unknown directive .%s\n"), direct->str);
						return 1;
					}
			}
		}
	} while (!allok);
	return 0;
}

int envyas_output(struct asctx *ctx, enum envyas_ofmt ofmt, const char *outname, int stride) {
	FILE *outfile = stdout;
	int i, j, k;
	if (outname) {
		if (!(outfile = fopen(outname, "w"))) {
			perror(outname);
			return 1;
		}
	}
	int cbsz = ed_getcstride(ctx->isa, ctx->varinfo);
	if (!stride)
		stride = cbsz;
	if (stride < cbsz) {
		fprintf(stderr, "Stride smaller than code byte size!\n");
		return 1;
	}
	for (i = 0; i < ctx->sectionsnum; i++) {
		if (!strcmp(ctx->sections[i].name, "default") && !ctx->sections[i].pos)
			continue;
		if (ofmt == OFMT_RAW) {
			for (j = 0; j < ctx->sections[i].pos; j += cbsz) {
				fwrite (ctx->sections[i].code + j, 1, cbsz, outfile);
				for (k = 0; k < stride-cbsz; k++)
					fputc(0, outfile);
			}
		} else {
			int ischex = 0;
			if (ofmt == OFMT_CHEX8) {
				fprintf (outfile, "uint8_t %s[] = {\n", ctx->sections[i].name);
				ischex = 1;
			}
			if (ofmt == OFMT_CHEX32) {
				fprintf (outfile, "uint32_t %s[] = {\n", ctx->sections[i].name);
				ischex = 1;
			}
			if (ofmt == OFMT_CHEX64) {
				fprintf (outfile, "uint64_t %s[] = {\n", ctx->sections[i].name);
				ischex = 1;
			}
			int step;
			if (ofmt == OFMT_CHEX64 || ofmt == OFMT_HEX64) {
				step = 8;
			} else if (ofmt == OFMT_CHEX32 || ofmt == OFMT_HEX32) {
				step = 4;
			} else {
				step = 1;
			}
			if (cbsz != 1) {
				if (step < cbsz && ischex) {
					fprintf(stderr, "Output format word too small to hold a full code byte!\n");
					return 1;
				}
				step = cbsz;
			}
			for (j = 0; j < ctx->sections[i].pos; j+=step) {
				uint64_t val = 0;
				if (ofmt == OFMT_CHEX8 || ofmt == OFMT_CHEX32 || ofmt == OFMT_CHEX64) {
					for (k = 0; k < step; k++) {
						int l = find_label(ctx, &ctx->sections[i], j + k, -1);
						while (l >= 0) {
							fprintf(outfile, "/* 0x%04x: %s */\n", j + k, ctx->labels[l].name);
							l = find_label(ctx, &ctx->sections[i], j + k, l + 1);
						}
					}
					fprintf (outfile, "\t");
				}
				for (k = 0; k < step && j + k < ctx->sections[i].pos; k++)
					val |= (uint64_t)ctx->sections[i].code[j + k] << (k * 8);
				if (ofmt == OFMT_CHEX64 || ofmt == OFMT_HEX64) {
					fprintf (outfile, "0x%016"PRIx64",\n", val);
				} else if (ofmt == OFMT_CHEX32 || ofmt == OFMT_HEX32) {
					fprintf (outfile, "0x%08"PRIx64",\n", val);
				} else {
					fprintf (outfile, "0x%02"PRIx64",\n", val);
				}
			}
			if (ischex) {
				fprintf (outfile, "};\n");
				if (i != ctx->sectionsnum - 1)
					fprintf(outfile, "\n");
			}
		}
	}
	return 0;
}

int main(int argc, char **argv) {
	struct asctx ctx_s = { 0 };
	struct asctx *ctx = &ctx_s;
	enum envyas_ofmt ofmt = OFMT_HEX8;
	const char *outname = 0;
	int stride = 0;
	argv[0] = basename(argv[0]);
	int len = strlen(argv[0]);
	if (len > 2 && !strcmp(argv[0] + len - 2, "as")) {
		argv[0][len-2] = 0;
		ctx->isa = ed_getisa(argv[0]);
		if (ctx->isa && ctx->isa->opunit == 4)
			ofmt = OFMT_HEX32;
	}
	int c;
	const char **varnames = 0;
	int varnamesnum = 0;
	int varnamesmax = 0;
	const char **modenames = 0;
	int modenamesnum = 0;
	int modenamesmax = 0;
	const char **featnames = 0;
	int featnamesnum = 0;
	int featnamesmax = 0;
	while ((c = getopt (argc, argv, "am:V:O:F:o:wWiS:")) != -1)
		switch (c) {
			case 'a':
				if (ofmt == OFMT_HEX64)
					ofmt = OFMT_CHEX64;
				else if (ofmt == OFMT_HEX32)
					ofmt = OFMT_CHEX32;
				else
					ofmt = OFMT_CHEX8;
				break;
			case 'w':
				if (ofmt == OFMT_CHEX8)
					ofmt = OFMT_CHEX32;
				else
					ofmt = OFMT_HEX32;
				break;
			case 'W':
				if (ofmt == OFMT_CHEX8)
					ofmt = OFMT_CHEX64;
				else
					ofmt = OFMT_HEX64;
				break;
			case 'i':
				ofmt = OFMT_RAW;
				break;
			case 'm':
				ctx->isa = ed_getisa(optarg);
				if (!ctx->isa) {
					fprintf (stderr, "Unknown architecture \"%s\"!\n", optarg);
					return 1;
				}
				break;
			case 'V':
				ADDARRAY(varnames, optarg);
				break;
			case 'O':
				ADDARRAY(modenames, optarg);
				break;
			case 'F':
				ADDARRAY(featnames, optarg);
				break;
			case 'o':
				outname = optarg;
				break;
			case 'S':
				sscanf(optarg, "%x", &stride);
				break;
		}
	FILE *ifile = stdin;
	const char *filename = "stdin";
	if (optind < argc) {
		filename = argv[optind];
		ifile = fopen(argv[optind], "r");
		if (!ifile) {
			perror(argv[optind]);
			return 1;
		}
		optind++;
		if (optind < argc) {
			fprintf (stderr, "Too many parameters!\n");
			return 1;
		}
	}
	if (!ctx->isa) {
		fprintf (stderr, "No architecture specified!\n");
		return 1;
	}
	ctx->varinfo = varinfo_new(ctx->isa->vardata);
	if (!ctx->varinfo)
		return 1;
	int i;
	for (i = 0; i < varnamesnum; i++)
		if (varinfo_set_variant(ctx->varinfo, varnames[i]))
			return 1;
	for (i = 0; i < featnamesnum; i++)
		if (varinfo_set_feature(ctx->varinfo, featnames[i]))
			return 1;
	for (i = 0; i < modenamesnum; i++)
		if (varinfo_set_mode(ctx->varinfo, modenames[i]))
			return 1;
	if (!ed_getcbsz(ctx->isa, ctx->varinfo)) {
		fprintf(stderr, "Not enough variant info specified!\n");
		return 1;
	}
	struct easm_file *file;
	int r = easm_read_file(ifile, filename, &file);
	if (r)
		return r;
	if (envyas_process(ctx, file))
		return 1;
	if (envyas_layout(ctx, file))
		return 1;
	if (envyas_output(ctx, ofmt, outname, stride))
		return 1;
	return 0;
}
