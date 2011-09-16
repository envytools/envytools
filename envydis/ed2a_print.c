#include "ed2a.h"
#include <assert.h>
#include <string.h>

const struct ed2a_colors ed2a_null_colors = {
	"",
	"",
	"",
	"",
	"",
	"",
	"",
};

const struct ed2a_colors ed2a_def_colors = {
	"\x1b[32m",
	"\x1b[36m",
	"\x1b[36m",
	"\x1b[31m",
	"\x1b[33m",
	"\x1b[35m",
	"\x1b[1;31m",
};

void ed2a_print_file(struct ed2a_file *file, FILE *ofile, const struct ed2a_colors *col) {
	int i;
	if (file->broken)
		fprintf (ofile, "%s[broken file]\n", col->err);
	for (i = 0; i < file->insnsnum; i++) {
		ed2a_print_insn(file->insns[i], ofile, col);
	}
}

void ed2a_print_insn(struct ed2a_insn *insn, FILE *ofile, const struct ed2a_colors *col) {
	int i;
	for (i = 0; i < insn->piecesnum; i++) {
		if (i)
			fprintf(ofile, " ");
		ed2a_print_ipiece(insn->pieces[i], ofile, col);
		if (i == insn->piecesnum - 1) {
			fprintf(ofile, "%s;", col->sym);
		} else {
			fprintf(ofile, " %s&", col->sym);
		}
		fprintf(ofile, "\n");
	}
}

void ed2a_print_ipiece(struct ed2a_ipiece *ipiece, FILE *ofile, const struct ed2a_colors *col) {
	int i;
	for (i = 0; i < ipiece->prefsnum; i++) {
		ed2a_print_expr(ipiece->prefs[i], ofile, col, 0);
		fprintf(ofile, " ");
	}
	fprintf(ofile, "%s%s", col->iname, ipiece->name);
	for (i = 0; i < ipiece->iopsnum; i++) {
		fprintf(ofile, " ");
		ed2a_print_iop(ipiece->iops[i], ofile, col);
	}
}

void ed2a_print_iop(struct ed2a_iop *iop, FILE *ofile, const struct ed2a_colors *col) {
	int i;
	for (i = 0; i < iop->modsnum; i++)
		fprintf(ofile, "%s%s ", col->mod, iop->mods[i]);
	for (i = 0; i < iop->exprsnum; i++) {
		if (i)
			fprintf (ofile, "%s|", col->sym);
		ed2a_print_expr(iop->exprs[i], ofile, col, 0);
	}
}

void ed2a_print_expr(struct ed2a_expr *expr, FILE *ofile, const struct ed2a_colors *col, int prio) {
	int i;
	const char *s;
	switch (expr->type) {
	case ED2A_ET_PLUS:
	case ED2A_ET_MINUS:
		if (prio > 4)
			fprintf(ofile, "%s(", col->sym);
		ed2a_print_expr(expr->e1, ofile, col, 4);
		fprintf(ofile, "%s%c", col->sym, (expr->type == ED2A_ET_PLUS ? '+' : '-'));
		ed2a_print_expr(expr->e2, ofile, col, 5);
		if (prio > 4)
			fprintf(ofile, "%s)", col->sym);
		break;
	case ED2A_ET_UMINUS:
		if (prio > 6)
			fprintf(ofile, "%s(", col->sym);
		fprintf(ofile, "%s-", col->sym);
		ed2a_print_expr(expr->e1, ofile, col, 6);
		if (prio > 6)
			fprintf(ofile, "%s)", col->sym);
		break;
	case ED2A_ET_SWZ:
		if (prio > 7)
			fprintf(ofile, "%s(", col->sym);
		ed2a_print_expr(expr->e1, ofile, col, 7);
		fprintf(ofile, "%s.", col->sym);
		uint64_t maxswz = 0;
		for (i = 0; i < expr->swz->elemsnum; i++)
			if (expr->swz->elems[i] > maxswz)
				maxswz = expr->swz->elems[i];
		s = 0;
		switch (expr->swz->style) {
		case ED2A_SWZ_xyzw:
			s = "xyzw";
			break;
		case ED2A_SWZ_XYZW:
			s = "XYZW";
			break;
		case ED2A_SWZ_rgba:
			s = "rgba";
			break;
		case ED2A_SWZ_RGBA:
			s = "RGBA";
			break;
		case ED2A_SWZ_stpq:
			s = "stpq";
			break;
		case ED2A_SWZ_STPQ:
			s = "STPQ";
			break;
		case ED2A_SWZ_lh:
			s = "lh";
			break;
		case ED2A_SWZ_LH:
			s = "LH";
			break;
		}
		if (s && maxswz < strlen(s)) {
			for (i = 0; i < expr->swz->elemsnum; i++)
				fprintf(ofile, "%c", s[expr->swz->elems[i]]);
		} else if (expr->swz->elemsnum == 1) {
			fprintf(ofile, "%"PRIu64, expr->swz->elems[0]);
		} else {
			fprintf(ofile, "(");
			for (i = 0; i < expr->swz->elemsnum; i++) {
				if (i)
					fprintf(ofile, " ");
				fprintf(ofile, "%"PRIu64, expr->swz->elems[i]);
			}
			fprintf(ofile, ")");
		}
		if (prio > 7)
			fprintf(ofile, "%s)", col->sym);
		break;
	case ED2A_ET_MUL:
		if (prio > 5)
			fprintf(ofile, "%s(", col->sym);
		ed2a_print_expr(expr->e1, ofile, col, 5);
		fprintf(ofile, "%s*", col->sym);
		ed2a_print_expr(expr->e2, ofile, col, 6);
		if (prio > 5)
			fprintf(ofile, "%s)", col->sym);
		break;
	case ED2A_ET_DISCARD:
		fprintf(ofile, "%s#", col->reg);
		break;
	case ED2A_ET_IPIECE:
		fprintf(ofile, "%s(", col->sym);
		ed2a_print_ipiece(expr->ipiece, ofile, col);
		fprintf(ofile, "%s)", col->sym);
		break;
	case ED2A_ET_MEM:
	case ED2A_ET_MEMPOSTI:
	case ED2A_ET_MEMPOSTD:
	case ED2A_ET_MEMPREI:
	case ED2A_ET_MEMPRED:
		if (expr->str)
			fprintf(ofile, "%s%s", col->mem, expr->str);
		fprintf(ofile, "%s[", col->mem);
		ed2a_print_expr(expr->e1, ofile, col, 0);
		if (expr->type != ED2A_ET_MEM) {
			switch (expr->type) {
			case ED2A_ET_MEMPOSTI:
				fprintf(ofile, "%s++", col->mem);
				break;
			case ED2A_ET_MEMPOSTD:
				fprintf(ofile, "%s--", col->mem);
				break;
			case ED2A_ET_MEMPREI:
				fprintf(ofile, "%s+=", col->mem);
				break;
			case ED2A_ET_MEMPRED:
				fprintf(ofile, "%s-=", col->mem);
				break;
			}
			ed2a_print_expr(expr->e2, ofile, col, 0);
		}
		fprintf(ofile, "%s]", col->mem);
		break;
	case ED2A_ET_LABEL:
		fprintf(ofile, "%s#%s", col->num, expr->str);
		break;
	case ED2A_ET_NUM:
		fprintf(ofile, "%s%#"PRIx64, col->num, expr->num);
		break;
	case ED2A_ET_NUM2:
		fprintf(ofile, "%s%#"PRIx64"%s:%s%#"PRIx64, col->num, expr->num, col->sym, col->num, expr->num2);
		break;
	case ED2A_ET_REG:
		fprintf(ofile, "%s$%s", col->reg, expr->str);
		break;
	case ED2A_ET_REG2:
		fprintf(ofile, "%s$%s%s:%s$%s", col->reg, expr->str, col->sym, col->reg, expr->str2);
		break;
	case ED2A_ET_RVEC:
		fprintf(ofile, "%s{", col->sym);
		for (i = 0; i < expr->rvec->elemsnum; i++) {
			if (expr->rvec->elems[i])
				fprintf(ofile, " %s$%s", col->reg, expr->rvec->elems[i]);
			else
				fprintf(ofile, " %s#", col->reg);
		}		
		fprintf(ofile, " %s}", col->sym);
		break;
	case ED2A_ET_STR:
		fprintf(ofile, "%s\"", col->num);
		for (i = 0; expr->str[i]; i++) {
			switch (expr->str[i]) {
			case '\'':
			case '\"':
			case '\?':
			case '\\':
				fprintf(ofile, "\\%c", expr->str[i]);
				break;
			case '\n':
				fprintf(ofile, "\\n");
				break;
			case '\r':
				fprintf(ofile, "\\r");
				break;
			case '\t':
				fprintf(ofile, "\\t");
				break;
			case '\f':
				fprintf(ofile, "\\f");
				break;
			case '\v':
				fprintf(ofile, "\\v");
				break;
			case '\a':
				fprintf(ofile, "\\a");
				break;
			default:
				if (expr->str[i] >= 0x20 && expr->str[i] < 0x7f) {
					fprintf(ofile, "%c", expr->str[i]);
				} else {
					fprintf(ofile, "\\x%02x", expr->str[i]);
				}
				break;
			}
		}
		fprintf(ofile, "\"");
		break;
	case ED2A_ET_ERR:
		fprintf(ofile, "%s?", col->err);
		break;
	default:
		assert(0);
		break;
	}
}

void ed2a_print_rvec(struct ed2a_rvec *rvec, FILE *ofile, const struct ed2a_colors *col);
void ed2a_print_swz(struct ed2a_swz *swz, FILE *ofile, const struct ed2a_colors *col);

