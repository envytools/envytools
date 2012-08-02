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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
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
};

static const struct disisa *envyas_isa = 0;

enum {
	OFMT_RAW,
	OFMT_HEX8,
	OFMT_HEX32,
	OFMT_HEX64,
	OFMT_CHEX8,
	OFMT_CHEX32,
	OFMT_CHEX64,
} envyas_ofmt = OFMT_HEX8;

struct varinfo *envyas_varinfo;

char *envyas_outname = 0;

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

ull calc (struct easm_expr *expr, struct asctx *ctx, struct envy_loc loc) {
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
			fprintf (stderr, LOC_FORMAT(loc, "Undefined label \"%s\"\n"), expr->str);
			exit(1);
		case EASM_EXPR_NEG:
			return -calc(expr->e1, ctx, loc);
		case EASM_EXPR_NOT:
			return ~calc(expr->e1, ctx, loc);
		case EASM_EXPR_LNOT:
			return !calc(expr->e1, ctx, loc);
		case EASM_EXPR_MUL:
			return calc(expr->e1, ctx, loc) * calc(expr->e2, ctx, loc);
		case EASM_EXPR_DIV:
			x = calc(expr->e2, ctx, loc);
			if (x)
				return calc(expr->e1, ctx, loc) / x;
			else {
				fprintf (stderr, LOC_FORMAT(loc, "Division by 0\n"));
				exit(1);
			}
		case EASM_EXPR_MOD:
			x = calc(expr->e2, ctx, loc);
			if (x)
				return calc(expr->e1, ctx, loc) % x;
			else {
				fprintf (stderr, LOC_FORMAT(loc, "Division by 0\n"));
				exit(1);
			}
		case EASM_EXPR_ADD:
			return calc(expr->e1, ctx, loc) + calc(expr->e2, ctx, loc);
		case EASM_EXPR_SUB:
			return calc(expr->e1, ctx, loc) - calc(expr->e2, ctx, loc);
		case EASM_EXPR_SHL:
			return calc(expr->e1, ctx, loc) << calc(expr->e2, ctx, loc);
		case EASM_EXPR_SHR:
			return calc(expr->e1, ctx, loc) >> calc(expr->e2, ctx, loc);
		case EASM_EXPR_AND:
			return calc(expr->e1, ctx, loc) & calc(expr->e2, ctx, loc);
		case EASM_EXPR_XOR:
			return calc(expr->e1, ctx, loc) ^ calc(expr->e2, ctx, loc);
		case EASM_EXPR_OR:
			return calc(expr->e1, ctx, loc) | calc(expr->e2, ctx, loc);
		case EASM_EXPR_LAND:
			return calc(expr->e1, ctx, loc) && calc(expr->e2, ctx, loc);
		case EASM_EXPR_LOR:
			return calc(expr->e1, ctx, loc) || calc(expr->e2, ctx, loc);
		default:
			assert(0);
	}
}

int resolve (struct asctx *ctx, ull *val, struct match m, ull pos, struct envy_loc loc) {
	int i;
	for (i = 0; i < m.nrelocs; i++) {
		ull val = calc(m.relocs[i].expr, ctx, loc);
		const struct bitfield *bf = m.relocs[i].bf;
		ull num = (val - bf->addend) ^ bf->xorend;
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
		ull mask = ~0ull;
		ull totalsz = bf->shr + bf->sbf[0].len + bf->sbf[1].len;
		if (bf->wrapok && totalsz < 64)
			mask = (1ull << totalsz) - 1;
		if ((getbf(bf, m.a, m.m, ctx->pos) & mask) != (val & mask))
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
			ull num = calc(direct->params[i], ctx, direct->loc);
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

int envyas_process(struct easm_file *file) {
	int i, j;
	struct asctx ctx_s = { 0 };
	struct asctx *ctx = &ctx_s;
	ctx->isa = envyas_isa;
	ctx->varinfo = envyas_varinfo;
	struct matches *im = calloc(sizeof *im, file->linesnum);
	for (i = 0; i < file->linesnum; i++) {
		if (file->lines[i]->type == EASM_LINE_INSN) {
			im[i] = *do_as(ctx->isa, ctx->varinfo, file->lines[i]->insn);
			if (!im[i].mnum) {
				fprintf (stderr, LOC_FORMAT(file->lines[i]->loc, "No match\n"));
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
	ctx->symtab = symtab_new();
	do {
		symtab_del(ctx->symtab);
		ctx->symtab = symtab_new();
		allok = 1;
		ctx->labelsnum = 0;
		ctx->cur_global_label = NULL;
		for (i = 0; i < sectionsnum; i++)
			free(sections[i].code);
		sectionsnum = 0;
		int cursect = 0;
		struct section def = { "default" };
		def.first_label = -1;
		ADDARRAY(sections, def);
		for (i = 0; i < file->linesnum; i++) {
			struct easm_directive *direct = file->lines[i]->directive;
			switch (file->lines[i]->type) {
				case EASM_LINE_INSN:
					if (ctx->isa->i_need_nv50as_hack) {
						if (im[i].m[0].oplen == 8 && (sections[cursect].pos & 7))
							sections[cursect].pos &= ~7ull, sections[cursect].pos += 8;
					}
					sections[cursect].pos += im[i].m[0].oplen;
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
					struct label l = { file->lines[i]->lname, sections[cursect].pos / ctx->isa->posunit + sections[cursect].base };
					if (sections[cursect].first_label < 0)
						sections[cursect].first_label = ctx->labelsnum;
					sections[cursect].last_label = ctx->labelsnum;
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
						for (j = 0; j < sectionsnum; j++)
							if (!strcmp(sections[j].name, direct->params[0]->str))
								break;
						if (j == sectionsnum) {
							struct section s = { direct->params[0]->str };
							s.first_label = -1;
							if (direct->paramsnum == 2)
								s.base = direct->params[1]->num;
							ADDARRAY(sections, s);
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
						sections[cursect].pos += num - 1;
						sections[cursect].pos /= num;
						sections[cursect].pos *= num;
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
						if (sections[cursect].pos > num) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Section '%s' exceeds .size by %llu bytes\n"), sections[cursect].name, sections[cursect].pos - num);
							return 1;
						}
						sections[cursect].pos = num;
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
						sections[cursect].pos += num;
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
						ull num = calc(direct->params[1], ctx, direct->loc);
						if (symtab_put(ctx->symtab, direct->params[0]->str, 0, ctx->labelsnum) == -1) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Label %s redeclared!\n"), direct->params[0]->str);
							return 1;
						}
						struct label l = { direct->params[0]->str, num , /* Distinguish .equ labels from regular labels */ 1 };
						ADDARRAY(ctx->labels, l);
					} else if (!donum(&sections[cursect], direct, ctx, 0)) {
						fprintf (stderr, LOC_FORMAT(direct->loc, "Unknown directive .%s\n"), direct->str);
						return 1;
					}
					break;
			}
		}
		cursect = 0;
		ctx->cur_global_label = NULL;
		for (j = 0; j < sectionsnum; j++)
			sections[j].pos = 0;
		ull val[MAXOPLEN];
		for (i = 0; i < file->linesnum; i++) {
			struct easm_directive *direct = file->lines[i]->directive;
			switch (file->lines[i]->type) {
				case EASM_LINE_INSN:
					if (!resolve(ctx, val, im[i].m[0], sections[cursect].pos / ctx->isa->posunit + sections[cursect].base, file->lines[i]->loc)) {
						sections[cursect].pos += im[i].m[0].oplen;
						im[i].m++;
						im[i].mnum--;
						if (!im[i].mnum) {
							fprintf (stderr, LOC_FORMAT(file->lines[i]->loc, "Relocaiton failed\n"));
							return 1;
						}
						allok = 0;
					} else {
						if (ctx->isa->i_need_nv50as_hack) {
							if (im[i].m[0].oplen == 8 && (sections[cursect].pos & 7)) {
								j = i - 1;
								while (j != -1ull && file->lines[j]->type == EASM_LINE_LABEL)
									j--;
								assert (j != -1ull && file->lines[j]->type == EASM_LINE_INSN);
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
							sections[cursect].code[sections[cursect].pos++] = val[j>>3] >> (8*(j&7));
					}
					break;
				case EASM_LINE_LABEL:
					if (file->lines[i]->lname[0] != '_')
						ctx->cur_global_label = file->lines[i]->lname;
					break;
				case EASM_LINE_DIRECTIVE:
					if (!strcmp(direct->str, "section")) {
						for (j = 0; j < sectionsnum; j++)
							if (!strcmp(sections[j].name, direct->params[0]->str))
								break;
						cursect = j;
					} else if (!strcmp(direct->str, "align")) {
						ull num = direct->params[0]->num;
						ull oldpos = sections[cursect].pos;
						sections[cursect].pos += num - 1;
						sections[cursect].pos /= num;
						sections[cursect].pos *= num;
						extend(&sections[cursect], 0);
						for (j = oldpos; j < sections[cursect].pos; j++)
							sections[cursect].code[j] = 0;
					} else if (!strcmp(direct->str, "size")) {
						ull num = direct->params[0]->num;
						ull oldpos = sections[cursect].pos;
						if (sections[cursect].pos > num) {
							fprintf (stderr, LOC_FORMAT(direct->loc, "Section '%s' exceeds .size by %llu bytes\n"), sections[cursect].name, sections[cursect].pos - num);
							return 1;
						}
						sections[cursect].pos = num;
						extend(&sections[cursect], 0);
						for (j = oldpos; j < sections[cursect].pos; j++)
							sections[cursect].code[j] = 0;
					} else if (!strcmp(direct->str, "skip")) {
						ull num = direct->params[0]->num;
						ull oldpos = sections[cursect].pos;
						sections[cursect].pos += num;
						extend(&sections[cursect], 0);
						for (j = oldpos; j < sections[cursect].pos; j++)
							sections[cursect].code[j] = 0;
					} else if (!strcmp(direct->str, "equ")) {
						/* nothing to be done */
					} else if (!donum(&sections[cursect], direct, ctx, 1)) {
						fprintf (stderr, LOC_FORMAT(direct->loc, "Unknown directive .%s\n"), direct->str);
						return 1;
					}
			}
		}
	} while (!allok);
	FILE *outfile = stdout;
	if (envyas_outname) {
		if (!(outfile = fopen(envyas_outname, "w"))) {
			perror(envyas_outname);
			return 1;
		}
	}

	for (i = 0; i < sectionsnum; i++) {
		if (!strcmp(sections[i].name, "default") && !sections[i].pos)
			continue;
		if (envyas_ofmt == OFMT_RAW)
			fwrite (sections[i].code, 1, sections[i].pos, outfile);
		else {
			if (envyas_ofmt == OFMT_CHEX8) {
				fprintf (outfile, "uint8_t %s[] = {\n", sections[i].name);
			}
			if (envyas_ofmt == OFMT_CHEX32) {
				fprintf (outfile, "uint32_t %s[] = {\n", sections[i].name);
			}
			if (envyas_ofmt == OFMT_CHEX64) {
				fprintf (outfile, "uint64_t %s[] = {\n", sections[i].name);
			}
			int step;
			if (envyas_ofmt == OFMT_CHEX64 || envyas_ofmt == OFMT_HEX64) {
				step = 8;
			} else if (envyas_ofmt == OFMT_CHEX32 || envyas_ofmt == OFMT_HEX32) {
				step = 4;
			} else {
				step = 1;
			}
			for (j = 0; j < sections[i].pos; j+=step) {
				uint64_t val = 0;
				int k;
				if (envyas_ofmt == OFMT_CHEX8 || envyas_ofmt == OFMT_CHEX32 || envyas_ofmt == OFMT_CHEX64) {
					for (k = 0; k < step; k++) {
						int l = find_label(ctx, &sections[i], j + k, -1);
						while (l >= 0) {
							fprintf(outfile, "/* 0x%04x: %s */\n", j + k, ctx->labels[l].name);
							l = find_label(ctx, &sections[i], j + k, l + 1);
						}
					}
					fprintf (outfile, "\t");
				}
				for (k = 0; k < step; k++)
					val |= (uint64_t)sections[i].code[j + k] << (k * 8);
				if (envyas_ofmt == OFMT_CHEX64 || envyas_ofmt == OFMT_HEX64) {
					fprintf (outfile, "0x%016"PRIx64",\n", val);
				} else if (envyas_ofmt == OFMT_CHEX32 || envyas_ofmt == OFMT_HEX32) {
					fprintf (outfile, "0x%08"PRIx64",\n", val);
				} else {
					fprintf (outfile, "0x%02"PRIx64",\n", val);
				}
			}
			if (envyas_ofmt == OFMT_CHEX8 || envyas_ofmt == OFMT_CHEX32 || envyas_ofmt == OFMT_CHEX64) {
				fprintf (outfile, "};\n");
				if (i != sectionsnum - 1)
					fprintf(outfile, "\n");
			}
		}
	}
	return 0;
}

int main(int argc, char **argv) {
	argv[0] = basename(argv[0]);
	int len = strlen(argv[0]);
	if (len > 2 && !strcmp(argv[0] + len - 2, "as")) {
		argv[0][len-2] = 0;
		envyas_isa = ed_getisa(argv[0]);
		if (envyas_isa && envyas_isa->opunit == 4)
			envyas_ofmt = OFMT_HEX32;
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
	while ((c = getopt (argc, argv, "am:V:O:F:o:wWi")) != -1)
		switch (c) {
			case 'a':
				if (envyas_ofmt == OFMT_HEX64)
					envyas_ofmt = OFMT_CHEX64;
				else if (envyas_ofmt == OFMT_HEX32)
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
			case 'W':
				if (envyas_ofmt == OFMT_CHEX8)
					envyas_ofmt = OFMT_CHEX64;
				else
					envyas_ofmt = OFMT_HEX64;
				break;
			case 'i':
				envyas_ofmt = OFMT_RAW;
				break;
			case 'm':
				envyas_isa = ed_getisa(optarg);
				if (!envyas_isa) {
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
				envyas_outname = optarg;
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
	if (!envyas_isa) {
		fprintf (stderr, "No architecture specified!\n");
		return 1;
	}
	envyas_varinfo = varinfo_new(envyas_isa->vardata);
	if (!envyas_varinfo)
		return 1;
	int i;
	for (i = 0; i < varnamesnum; i++)
		if (varinfo_set_variant(envyas_varinfo, varnames[i]))
			return 1;
	for (i = 0; i < featnamesnum; i++)
		if (varinfo_set_feature(envyas_varinfo, featnames[i]))
			return 1;
	for (i = 0; i < modenamesnum; i++)
		if (varinfo_set_mode(envyas_varinfo, modenames[i]))
			return 1;
	struct easm_file *file;
	int r = easm_read_file(ifile, filename, &file);
	if (r)
		return r;
	return envyas_process(file);
}
