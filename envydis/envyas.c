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

#include "dis.h"
#include <ctype.h>
#include <libgen.h>

static const struct disisa *envyas_isa = 0;

void convert_ipiece(struct line *line, struct ed2a_ipiece *ipiece);

enum {
	OFMT_RAW,
	OFMT_HEX8,
	OFMT_HEX32,
	OFMT_CHEX8,
	OFMT_CHEX32,
} envyas_ofmt = OFMT_HEX8;

struct ed2v_variant *envyas_variant;

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

ull calc (const struct expr *expr, struct disctx *ctx, struct ed2_loc loc) {
	int i;
	ull x;
	switch (expr->type) {
		case EXPR_NUM:
			return expr->num1;
		case EXPR_LABEL:
			if (expr->str[0] == '_' && expr->str[1] != '_') {
				const char *full_label = expand_local_label(expr->str, ctx->cur_global_label);
				free((char *)expr->str);
				((struct expr *)expr)->str = full_label;
			}
			for (i = 0; i < ctx->labelsnum; i++)
				if (!strcmp(ctx->labels[i].name, expr->str))
					return ctx->labels[i].val;
			fprintf (stderr, ED2_LOC_FORMAT(loc, "Undefined label \"%s\"\n"), expr->str);
			exit(1);
		case EXPR_NEG:
			return -calc(expr->expr1, ctx, loc);
		case EXPR_NOT:
			return ~calc(expr->expr1, ctx, loc);
		case EXPR_MUL:
			return calc(expr->expr1, ctx, loc) * calc(expr->expr2, ctx, loc);
		case EXPR_DIV:
			x = calc(expr->expr2, ctx, loc);
			if (x)
				return calc(expr->expr1, ctx, loc) / x;
			else {
				fprintf (stderr, ED2_LOC_FORMAT(loc, "Division by 0\n"));
				exit(1);
			}
		case EXPR_ADD:
			return calc(expr->expr1, ctx, loc) + calc(expr->expr2, ctx, loc);
		case EXPR_SUB:
			return calc(expr->expr1, ctx, loc) - calc(expr->expr2, ctx, loc);
		case EXPR_SHL:
			return calc(expr->expr1, ctx, loc) << calc(expr->expr2, ctx, loc);
		case EXPR_SHR:
			return calc(expr->expr1, ctx, loc) >> calc(expr->expr2, ctx, loc);
		case EXPR_AND:
			return calc(expr->expr1, ctx, loc) & calc(expr->expr2, ctx, loc);
		case EXPR_XOR:
			return calc(expr->expr1, ctx, loc) ^ calc(expr->expr2, ctx, loc);
		case EXPR_OR:
			return calc(expr->expr1, ctx, loc) | calc(expr->expr2, ctx, loc);
		default:
			assert(0);
	}
}

int resolve (struct disctx *ctx, ull *val, struct match m, ull pos, struct ed2_loc loc) {
	int i;
	for (i = 0; i < m.nrelocs; i++) {
		ull val = calc(m.relocs[i].expr, ctx, loc);
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
		ull mask = ~0ull;
		ull totalsz = bf->shr + bf->sbf[0].len + bf->sbf[1].len;
		if (bf->wrapok && totalsz < 64)
			mask = (1ull << totalsz) - 1;
		if ((getbf(bf, m.a, m.m, ctx) & mask) != (val & mask))
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

int donum (struct section *s, struct line *line, struct disctx *ctx, int wren) {
	if (line->str[0] != 'b' && line->str[0] != 's' && line->str[0] != 'u')
		return 0;
	char *end;
	ull bits = strtoull(line->str+1, &end, 0);
	if (*end)
		return 0;
	if (bits > 64 || bits & 7 || !bits)
		return 0;
	int i;
	for (i = 0; i < line->atomsnum; i++)
		if (!line->atoms[i]->isimm) {
			fprintf (stderr, ED2_LOC_FORMAT(line->loc, "Wrong arguments for %s\n"), line->str);
			exit(1);
		}
	if (wren) {
		extend(s, bits/8 * line->atomsnum);
		if (s->pos % (bits/8))
			fprintf (stderr, "Warning: Unaligned data %s; section '%s', offset 0x%x\n", line->str, s->name, s->pos);
		for (i = 0; i < line->atomsnum; i++) {
			ull num = calc(line->atoms[i], ctx, line->loc);
			if ((line->str[0] == 'u' && bits != 64 && num >= (1ull << bits))
				|| (line->str[0] == 's' && bits != 64 && num >= (1ull << (bits - 1)) && num < (-1ull << (bits - 1)))) {
				fprintf (stderr, ED2_LOC_FORMAT(line->loc, "Argument %d too large for %s\n"), i, line->str);
				exit(1);
			}
			int j;
			for (j = 0; j < bits/8; j++)
				s->code[s->pos+j + i * bits/8] = num >> (8*j);
			
		}
	}
	s->pos += bits/8 * line->atomsnum;
	return 1;
}

int find_label(struct disctx *ctx, struct section *sect, int ofs, int start_at) {
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

int envyas_process(struct file *file) {
	int i, j;
	struct disctx ctx_s = { 0 };
	struct disctx *ctx = &ctx_s;
	ctx->reverse = 1;
	ctx->isa = envyas_isa;
	ctx->variant = envyas_variant;
	struct matches *im = calloc(sizeof *im, file->linesnum);
	for (i = 0; i < file->linesnum; i++) {
		if (file->lines[i]->type == LINE_INSN) {
			ctx->line = file->lines[i];
			struct matches *m = atomtab(ctx, 0, 0, ctx->isa->troot, 0);
			for (j = 0; j < m->mnum; j++)
				if (m->m[j].lpos == ctx->line->atomsnum) {
					ADDARRAY(im[i].m, m->m[j]);
				}
			if (!im[i].mnum) {
				fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "No match\n"));
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
		ctx->cur_global_label = NULL;
		for (i = 0; i < sectionsnum; i++)
			free(sections[i].code);
		sectionsnum = 0;
		int cursect = 0;
		struct section def = { "default" };
		def.first_label = -1;
		ADDARRAY(sections, def);
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
					if (file->lines[i]->str[0] == '_' && file->lines[i]->str[1] != '_') {
						char *full_label = expand_local_label(file->lines[i]->str, ctx->cur_global_label);
						free(file->lines[i]->str);
						file->lines[i]->str = full_label;
					}
					else
						ctx->cur_global_label = file->lines[i]->str;

					for (j = 0; j < ctx->labelsnum; j++) {
						if (!strcmp(ctx->labels[j].name, file->lines[i]->str)) {
							fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "Label %s redeclared!\n"), file->lines[i]->str);
							return 1;
						}
					}
					struct label l = { file->lines[i]->str, sections[cursect].pos / ctx->isa->posunit + sections[cursect].base };
					if (sections[cursect].first_label < 0)
						sections[cursect].first_label = ctx->labelsnum;
					sections[cursect].last_label = ctx->labelsnum;
					ADDARRAY(ctx->labels, l);
					break;
				case LINE_DIR:
					if (!strcmp(file->lines[i]->str, "section")) {
						if (file->lines[i]->atomsnum > 2) {
							fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "Too many arguments for .section\n"));
							return 1;
						}
						if (file->lines[i]->atoms[0]->type != EXPR_LABEL || (file->lines[i]->atomsnum == 2 && file->lines[i]->atoms[1]->type != EXPR_NUM)) {
							fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "Wrong arguments for .section\n"));
							return 1;
						}
						for (j = 0; j < sectionsnum; j++)
							if (!strcmp(sections[j].name, file->lines[i]->atoms[0]->str))
								break;
						if (j == sectionsnum) {
							struct section s = { file->lines[i]->atoms[0]->str };
							s.first_label = -1;
							if (file->lines[i]->atomsnum == 2)
								s.base = file->lines[i]->atoms[1]->num1;
							ADDARRAY(sections, s);
						}
						cursect = j;
					} else if (!strcmp(file->lines[i]->str, "align")) {
						if (file->lines[i]->atomsnum > 1) {
							fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "Too many arguments for .align\n"));
							return 1;
						}
						if (file->lines[i]->atoms[0]->type != EXPR_NUM) {
							fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "Wrong arguments for .align\n"));
							return 1;
						}
						ull num = file->lines[i]->atoms[0]->num1;
						sections[cursect].pos += num - 1;
						sections[cursect].pos /= num;
						sections[cursect].pos *= num;
					} else if (!strcmp(file->lines[i]->str, "size")) {
						if (file->lines[i]->atomsnum > 1) {
							fprintf (stderr, "Too many arguments for .size\n");
							return 1;
						}
						if (file->lines[i]->atoms[0]->type != EXPR_NUM) {
							fprintf (stderr, "Wrong arguments for .size\n");
							return 1;
						}
						ull num = file->lines[i]->atoms[0]->num1;
						if (sections[cursect].pos > num) {
							fprintf (stderr, "Section '%s' exceeds .size by %llu bytes\n", sections[cursect].name, sections[cursect].pos - num);
							return 1;
						}
						sections[cursect].pos = num;
					} else if (!strcmp(file->lines[i]->str, "skip")) {
						if (file->lines[i]->atomsnum > 1) {
							fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "Too many arguments for .skip\n"));
							return 1;
						}
						if (file->lines[i]->atoms[0]->type != EXPR_NUM) {
							fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "Wrong arguments for .skip\n"));
							return 1;
						}
						ull num = file->lines[i]->atoms[0]->num1;
						sections[cursect].pos += num;
					} else if (!strcmp(file->lines[i]->str, "equ")) {
						if (file->lines[i]->atomsnum != 2
							|| file->lines[i]->atoms[0]->type != EXPR_LABEL
							|| !file->lines[i]->atoms[1]->isimm) {
							fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "Wrong arguments for .equ\n"));
							return 1;
						}
						if (file->lines[i]->atoms[0]->str[0] == '_' && file->lines[i]->atoms[0]->str[1] != '_') {
							char *full_label = expand_local_label(file->lines[i]->atoms[0]->str, ctx->cur_global_label);
							free((void*)file->lines[i]->atoms[0]->str);
							file->lines[i]->atoms[0]->str = full_label;
						}
						ull num = calc(file->lines[i]->atoms[1], ctx, file->lines[i]->loc);
						for (j = 0; j < ctx->labelsnum; j++) {
							if (!strcmp(ctx->labels[j].name, file->lines[i]->atoms[0]->str)) {
								fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "Label %s redeclared!\n"), file->lines[i]->atoms[0]->str);
								return 1;
							}
						}
						struct label l = { file->lines[i]->atoms[0]->str, num , /* Distinguish .equ labels from regular labels */ 1 };
						ADDARRAY(ctx->labels, l);
					} else if (!donum(&sections[cursect], file->lines[i], ctx, 0)) {
						fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "Unknown directive %s\n"), file->lines[i]->str);
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
			switch (file->lines[i]->type) {
				case LINE_INSN:
					if (!resolve(ctx, val, im[i].m[0], sections[cursect].pos / ctx->isa->posunit + sections[cursect].base, file->lines[i]->loc)) {
						sections[cursect].pos += im[i].m[0].oplen;
						im[i].m++;
						im[i].mnum--;
						if (!im[i].mnum) {
							fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "Relocaiton failed\n"));
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
							sections[cursect].code[sections[cursect].pos++] = val[j>>3] >> (8*(j&7));
					}
					break;
				case LINE_LABEL:
					if (file->lines[i]->str[0] != '_')
						ctx->cur_global_label = file->lines[i]->str;
					break;
				case LINE_DIR:
					if (!strcmp(file->lines[i]->str, "section")) {
						for (j = 0; j < sectionsnum; j++)
							if (!strcmp(sections[j].name, file->lines[i]->atoms[0]->str))
								break;
						cursect = j;
					} else if (!strcmp(file->lines[i]->str, "align")) {
						ull num = file->lines[i]->atoms[0]->num1;
						ull oldpos = sections[cursect].pos;
						sections[cursect].pos += num - 1;
						sections[cursect].pos /= num;
						sections[cursect].pos *= num;
						extend(&sections[cursect], 0);
						for (j = oldpos; j < sections[cursect].pos; j++)
							sections[cursect].code[j] = 0;
					} else if (!strcmp(file->lines[i]->str, "size")) {
						ull num = file->lines[i]->atoms[0]->num1;
						ull oldpos = sections[cursect].pos;
						if (sections[cursect].pos > num) {
							fprintf (stderr, "Section '%s' exceeds .size by %llu bytes\n", sections[cursect].name, sections[cursect].pos - num);
							return 1;
						}
						sections[cursect].pos = num;
						extend(&sections[cursect], 0);
						for (j = oldpos; j < sections[cursect].pos; j++)
							sections[cursect].code[j] = 0;
					} else if (!strcmp(file->lines[i]->str, "skip")) {
						ull num = file->lines[i]->atoms[0]->num1;
						ull oldpos = sections[cursect].pos;
						sections[cursect].pos += num;
						extend(&sections[cursect], 0);
						for (j = oldpos; j < sections[cursect].pos; j++)
							sections[cursect].code[j] = 0;
					} else if (!strcmp(file->lines[i]->str, "equ")) {
						/* nothing to be done */
					} else if (!donum(&sections[cursect], file->lines[i], ctx, 1)) {
						fprintf (stderr, ED2_LOC_FORMAT(file->lines[i]->loc, "Unknown directive %s\n"), file->lines[i]->str);
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
			if (envyas_ofmt == OFMT_CHEX32 || envyas_ofmt == OFMT_HEX32) {
				for (j = 0; j < sections[i].pos; j+=4) {
					uint32_t val;
					val = sections[i].code[j] | sections[i].code[j+1] << 8 | sections[i].code[j+2] << 16 | sections[i].code[j+3] << 24;
					if (envyas_ofmt == OFMT_CHEX8 || envyas_ofmt == OFMT_CHEX32) {
						int k;
						for (k = 0; k < 4; k++) {
							int l = find_label(ctx, &sections[i], j + k, -1);
							while (l >= 0) {
								fprintf(outfile, "/* 0x%04x: %s */\n", j + k, ctx->labels[l].name);
								l = find_label(ctx, &sections[i], j + k, l + 1);
							}
						}
						fprintf (outfile, "\t");
					}
					fprintf (outfile, "0x%08x,\n", val);
				}
			} else {
				for (j = 0; j < sections[i].pos; j++) {
					if (envyas_ofmt == OFMT_CHEX8 || envyas_ofmt == OFMT_CHEX32) {
						int l = find_label(ctx, &sections[i], j, -1);
						while (l >= 0) {
							fprintf(outfile, "/* 0x%04x: %s */\n", j, ctx->labels[l].name);
							l = find_label(ctx, &sections[i], j, l + 1);
						}
						fprintf (outfile, "\t");
					}
					fprintf (outfile, "0x%02x,\n", sections[i].code[j]);
				}
			}
			if (envyas_ofmt == OFMT_CHEX8 || envyas_ofmt == OFMT_CHEX32) {
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
	const char *varname = 0;
	const char *modename = 0;
	while ((c = getopt (argc, argv, "am:V:O:o:wi")) != -1)
		switch (c) {
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
				envyas_isa = ed_getisa(optarg);
				if (!envyas_isa) {
					fprintf (stderr, "Unknown architecure \"%s\"!\n", optarg);
					return 1;
				}
				break;
			case 'V':
				varname = optarg;
				break;
			case 'O':
				modename = optarg;
				break;
			case 'o':
				envyas_outname = optarg;
				break;
		}
	struct ed2a_file *file_ed2;
	if (optind < argc) {
		FILE *ifile = fopen(argv[optind], "r");
		if (!ifile) {
			perror(argv[optind]);
			return 1;
		}
		file_ed2 = ed2a_read_file(ifile, argv[optind], 0, 0);
		optind++;
		if (optind < argc) {
			fprintf (stderr, "Too many parameters!\n");
			return 1;
		}
	} else {
		file_ed2 = ed2a_read_file(stdin, "stdin", 0, 0);
	}
	if (!envyas_isa) {
		fprintf (stderr, "No architecture specified!\n");
		return 1;
	}
	envyas_variant = ed2v_new_variant(envyas_isa->ed2, varname);
	if (!envyas_variant)
		return 1;
	if (modename)
		if (ed2v_set_mode(envyas_variant, modename))
			return 1;
	struct file *file = calloc(sizeof *file, 1);
	int i;
	for (i = 0; i < file_ed2->insnsnum; i++) {
		struct ed2a_insn *insn = file_ed2->insns[i];
		if (insn->piecesnum != 1) {
			fprintf (stderr, "Multi-piece instruction!!\n");
			return 1;
		}
		struct ed2a_ipiece *ipiece = insn->pieces[0];
		struct line *line = calloc(sizeof *line, 1);
		if (!strcmp(ipiece->name, "label")) {
			if (ipiece->prefsnum || ipiece->iopsnum != 1 || ipiece->iops[0]->exprsnum != 1 || ipiece->iops[0]->modsnum || ipiece->iops[0]->exprs[0]->type != ED2A_ET_LABEL) {
				fprintf (stderr, "Invalid label instruction!!\n");
				return 1;
			}
			line->type = LINE_LABEL;
			line->str = ipiece->iops[0]->exprs[0]->str;
		} else if (!strcmp(ipiece->name, "section") || !strcmp(ipiece->name, "align") || !strcmp(ipiece->name, "skip") || !strcmp(ipiece->name, "equ") || ((ipiece->name[0] == 's' || ipiece->name[0] == 'b' || ipiece->name[0] == 'u') && isdigit(ipiece->name[1]))) {
			if (ipiece->prefsnum) {
				fprintf (stderr, "Invalid directive!!\n");
				return 1;
			}
			line->type = LINE_DIR;
			line->str = ipiece->name;
			int j;
			for (j = 0; j < ipiece->iopsnum; j++) {
				convert_iop(line, ipiece->iops[j]);
			}
		} else {
			line->type = LINE_INSN;
			convert_ipiece(line, ipiece);
		}
		line->loc = insn->loc;
		ADDARRAY(file->lines, line);
	}
	return envyas_process(file);
}
