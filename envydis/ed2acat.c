/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "ed2a.h"
#include "dis.h"

void convert_ipiece(struct line *line, struct ed2a_ipiece *ipiece);

static void fun (struct ed2a_insn *insn, void *parm) {
	FILE *ofile = parm;
	int i, j;
	ed2a_print_insn(insn, ofile, &envy_def_colors);
	for (i = 0; i < insn->piecesnum; i++) {
		struct line *line = calloc(sizeof *line, 1);
		convert_ipiece(line, insn->pieces[i]);
		
		int noblank = 0;

		for (j = 0; j < line->atomsnum; j++) {
			if (line->atoms[j]->type == EXPR_SEEND)
				noblank = 1;
			if (j && !noblank)
				fprintf (ofile, " ");
			printexpr(ofile, line->atoms[j], 0, &envy_def_colors);
			noblank = (line->atoms[j]->type == EXPR_SESTART);
		}
		fprintf(ofile, "\n");
	}
}

int main() {
//	ed2a_print_file(ed2a_read_file(stdin, 0, 0), stdout);
	ed2a_read_file(stdin, "stdin", fun, stdout);
	return 0;
}
