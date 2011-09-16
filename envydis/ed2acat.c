#include "ed2a.h"
#include "dis.h"

void convert_ipiece(struct line *line, struct ed2a_ipiece *ipiece);

static void fun (struct ed2a_insn *insn, void *parm) {
	FILE *ofile = parm;
	int i, j;
	ed2a_print_insn(insn, ofile, &ed2a_def_colors);
	for (i = 0; i < insn->piecesnum; i++) {
		struct line *line = calloc(sizeof *line, 1);
		convert_ipiece(line, insn->pieces[i]);
		
		int noblank = 0;

		for (j = 0; j < line->atomsnum; j++) {
			if (line->atoms[j]->type == EXPR_SEEND)
				noblank = 1;
			if (j && !noblank)
				fprintf (ofile, " ");
			printexpr(ofile, line->atoms[j], 0);
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
