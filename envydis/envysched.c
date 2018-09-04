/*
 * Copyright (C) 2018 Rhys Perry <pendingchaos02@gmail.com>
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

#include <unistd.h>
#include <stdio.h>
#include "sched-intern.h"
#include "codesched.h"
#include "envyas.h"
#include "easm.h"
#include "util.h"
#include "dis.h"

static void printhelp(char **argv) {
	fprintf(stderr, "Usage: %s [args ...]\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "Schedules assembly from stdin, writing the result to stdout.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, " -a            Schedule all code and error on .beginsched/.endsched\n");
	fprintf(stderr, " -s            Ignore \"sched\" instructions. Useful if they exist\n");
	fprintf(stderr, " -m <machine>  Select the ISA of the input code\n");
	/* Same descriptions and behaviour as in envyas.c */
	fprintf(stderr, " -V <variant>  Select variant of the ISA\n");
	fprintf(stderr, " -O <mode>     Select processor mode\n");
	fprintf(stderr, " -F <feature>  Enable optional ISA feature. Most of these are auto-selected by\n");
	fprintf(stderr, "               -V, but can also be specified manually\n");
}

static void print_line(struct easm_line *l) {
	switch (l->type) {
	case EASM_LINE_INSN: {
		easm_print_insn(stdout, &envy_null_colors, l->insn);
		printf("\n");
		break;
	}
	case EASM_LINE_DIRECTIVE: {
		printf(".%s ", l->directive->str);
		int i;
		for (i = 0; i < l->directive->paramsnum; i++) {
			easm_print_sexpr(stdout, &envy_null_colors, l->directive->params[i], -1);
			printf(" ");
		}
		printf("\n");
		break;
	}
	case EASM_LINE_LABEL: {
		printf("%s:\n", l->lname);
		break;
	}
	}
}

static int print_scheduled_lines(const struct disisa *isa, struct varinfo *varinfo, int lnum, struct easm_line **l) {
	uint64_t *insns = NULL;
	int insnsnum = 0, insnsmax = 0;
	int i;
	for (i = 0; i < lnum; i++) {
		if (l[i]->type == EASM_LINE_INSN) {
			struct matches *matches = do_as(isa, varinfo, l[i]->insn);
			if (!matches->mnum) {
				fprintf(stderr, LOC_FORMAT(l[i]->loc, "No match\n"));
				return 1;
			}
			assert(matches->mnum);
			struct match *m = &matches->m[0];
			/* non-64-bit instructions are currently not handled */
			assert(m->oplen == 8);
			uint64_t insn = m->a[0];
			ADDARRAY(insns, insn);
		}
	}

	struct schedinsn *sched = do_sched(isa, insnsnum, insns);
	if (!sched) {
		free(insns);
		return 1;
	}

	int done = 0;
	for (i = 0; i < lnum; i++) {
		if (l[i]->type == EASM_LINE_INSN) {
			isa->schedtarg->printsched(stdout, done, insnsnum-done, sched + done);
			print_line(l[i]);
			done++;
		} else {
			print_line(l[i]);
		}
	}

	isa->schedtarg->printpaddingnops(stdout, insnsnum);

	free(sched);
	free(insns);

	return 0;
}

static int issched(struct easm_insn *insn) {
	/* We could assemble the instruction and match it against isa->tsched,
	   but gk110 doesn't set it. So this is done instead and works well enough.
	 */
	if (insn->subinsnsnum != 1)
		return 0;
	struct easm_sinsn *sinsn = insn->subinsns[0]->sinsn;
	return strcmp(sinsn->str, "sched") == 0;
}

int main(int argc, char **argv) {
	int schedall = 0;
	int ignoresched = 0;
	const struct disisa *isa = NULL;
	const char **varnames = 0;
	int varnamesnum = 0, varnamesmax = 0;
	const char **modenames = 0;
	int modenamesnum = 0, modenamesmax = 0;
	const char **featnames = 0;
	int featnamesnum = 0, featnamesmax = 0;
	int c;
	while ((c = getopt(argc, argv, "asm:V:O:F:h")) != -1) {
		switch (c) {
			case 'a':
				schedall = 1;
				break;
			case 's':
				ignoresched = 1;
				break;
			case 'm':
				isa = ed_getisa(optarg);
				if (!isa || !isa->schedtarg) {
					fprintf(stderr, "Unknown or unsupported architecture\n");
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
			case 'h':
				printhelp(argv);
				return 1;
			case ':':
			case '?':
				fprintf(stderr, "Incorrect arguments\n");
				return 1;
		}
	}
	if (!isa) {
		fprintf(stderr, "No architecture specified\n");
		return 1;
	}

	struct varinfo *varinfo = varinfo_new(isa->vardata);
	int i;
	for (i = 0; i < varnamesnum; i++) {
		if (varinfo_set_variant(varinfo, varnames[i]) < 0)
			return 1;
	}
	for (i = 0; i < featnamesnum; i++) {
		if (varinfo_set_feature(varinfo, featnames[i]) < 0)
			return 1;
	}
	for (i = 0; i < modenamesnum; i++) {
		if (varinfo_set_mode(varinfo, modenames[i]) < 0)
			return 1;
	}
	if (!ed_getcbsz(isa, varinfo)) {
		fprintf(stderr, "Not enough variant info specified\n");
		return 1;
	}

	struct easm_file *f;
	easm_read_file(stdin, "stdin", &f);

	int beginsched = schedall;
	struct envy_loc beginschedloc;
	struct easm_line **lines = NULL;
	int linesnum = 0, linesmax = 0;
	for (i = 0; i < f->linesnum; i++) {
		struct easm_line *l = f->lines[i];
		switch (l->type) {
		case EASM_LINE_INSN: {
			if (beginsched) {
				int sched = issched(l->insn);
				if (sched && !ignoresched) {
					fprintf(stderr, LOC_FORMAT(l->insn->loc, "\"sched\" instruction between .beginsched/.endsched\n"));
					return 1;
				}
				if (!sched)
					ADDARRAY(lines, l);
			} else {
				print_line(l);
			}
			break;
		}
		case EASM_LINE_DIRECTIVE: {
			struct easm_directive *dir = l->directive;
			if (strcmp(dir->str, "beginsched") == 0) {
				if (!isa->schedtarg) {
					fprintf(stderr, "Unsupported ISA for scheduling\n");
					return 1;
				}
				if (schedall) {
					fprintf(stderr, LOC_FORMAT(dir->loc, ".beginsched used with -a\n"));
					return 1;
				}
				if (dir->paramsnum) {
					fprintf(stderr, LOC_FORMAT(dir->loc, "Too many arguments for .beginsched\n"));
					return 1;
				}
				if (beginsched) {
					fprintf(stderr, LOC_FORMAT(dir->loc, "Scheduling already begun\n"));
					return 1;
				}
				beginsched = 1;
				beginschedloc = dir->loc;
				lines = NULL;
				linesnum = linesmax = 0;
			} else if (strcmp(dir->str, "endsched") == 0) {
				if (schedall) {
					fprintf(stderr, LOC_FORMAT(dir->loc, ".endsched used with -a\n"));
					return 1;
				}
				if (dir->paramsnum) {
					fprintf(stderr, LOC_FORMAT(dir->loc, "Too many arguments for .endsched\n"));
					return 1;
				}
				if (!beginsched) {
					fprintf(stderr, LOC_FORMAT(dir->loc, "Scheduling has not begun\n"));
					return 1;
				}
				beginsched = 0;

				if (print_scheduled_lines(isa, varinfo, linesnum, lines))
					return 1;

				free(lines);
				lines = NULL;
				linesnum = linesmax = 0;
			} else if (beginsched) {
				fprintf(stderr, LOC_FORMAT(dir->loc, ".%s directive can't be inbetween .beginsched/.endsched\n"), dir->str);
				return 1;
			} else {
				print_line(l);
			}
			break;
		}
		case EASM_LINE_LABEL: {
			if (beginsched)
				ADDARRAY(lines, l);
			else
				print_line(l);
			break;
		}
		}
	}

	if (linesnum && !schedall) {
		fprintf(stderr, LOC_FORMAT(beginschedloc, "Unterminated .beginsched\n"));
		return 1;
	}

	if (linesnum)
		print_scheduled_lines(isa, varinfo, linesnum, lines);
	free(lines);

	easm_del_file(f);

	return 0;
}
