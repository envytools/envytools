#include "ed2a.h"
#include <string.h>

struct ed2a_expr *ed2a_make_expr(enum ed2a_etype type) {
	struct ed2a_expr *res = calloc (sizeof *res, 1);
	res->type = type;
	return res;
}

struct ed2a_expr *ed2a_make_expr_str(enum ed2a_etype type, char *str) {
	struct ed2a_expr *res = ed2a_make_expr(type);
	res->str = str;
	return res;
}

struct ed2a_expr *ed2a_make_expr_bi(enum ed2a_etype type, struct ed2a_expr *e1, struct ed2a_expr *e2) {
	struct ed2a_expr *res = ed2a_make_expr(type);
	res->e1 = e1;
	res->e2 = e2;
	return res;
}

struct ed2a_expr *ed2a_make_expr_mem(enum ed2a_etype type, char *str, struct ed2a_expr *e1, struct ed2a_expr *e2) {
	struct ed2a_expr *res = ed2a_make_expr(type);
	res->str = str;
	res->e1 = e1;
	res->e2 = e2;
	return res;
}

struct ed2a_expr *ed2a_make_expr_ipiece(struct ed2a_ipiece *ip) {
	struct ed2a_expr *res = ed2a_make_expr(ED2A_ET_IPIECE);
	res->ipiece = ip;
	return res;
}

struct ed2a_expr *ed2a_make_expr_swz(struct ed2a_expr *ex, struct ed2a_swz *swz) {
	struct ed2a_expr *res = ed2a_make_expr(ED2A_ET_SWZ);
	res->swz = swz;
	res->e1 = ex;
	return res;
}

struct ed2a_expr *ed2a_make_expr_rvec(struct ed2a_rvec *rvec) {
	struct ed2a_expr *res = ed2a_make_expr(ED2A_ET_RVEC);
	res->rvec = rvec;
	return res;
}

struct ed2a_expr *ed2a_make_expr_num(uint64_t num) {
	struct ed2a_expr *res = ed2a_make_expr(ED2A_ET_NUM);
	res->num = num;
	return res;
}

struct ed2a_expr *ed2a_make_expr_num2(uint64_t num, uint64_t num2) {
	struct ed2a_expr *res = ed2a_make_expr(ED2A_ET_NUM2);
	res->num = num;
	res->num2 = num2;
	return res;
}

struct ed2a_expr *ed2a_make_expr_reg2(char *str, char *str2) {
	struct ed2a_expr *res = ed2a_make_expr(ED2A_ET_REG2);
	res->str = str;
	res->str2 = str2;
	return res;
}

struct ed2a_insn *ed2a_make_label_insn(char *str) {
	struct ed2a_expr *ex = ed2a_make_expr_str(ED2A_ET_LABEL, str);
	struct ed2a_iop *iop = calloc (sizeof *iop, 1);
	ADDARRAY(iop->exprs, ex);
	struct ed2a_ipiece *ipiece = calloc (sizeof *ipiece, 1);
	ipiece->name = strdup("label");
	ADDARRAY(ipiece->iops, iop);
	struct ed2a_insn *insn = calloc (sizeof *insn, 1);
	ADDARRAY(insn->pieces, ipiece);
	return insn;
}

struct ed2a_swz *ed2a_make_swz_empty() {
	return calloc(sizeof *ed2a_make_swz_empty(), 1);
}

struct ed2a_swz *ed2a_make_swz_num(uint64_t num) {
	struct ed2a_swz *res = ed2a_make_swz_empty();
	ADDARRAY(res->elems, num);
	res->style = ED2A_SWZ_NUM;
	return res;
}

static const char *const swz_sets[] = { 0, "xyzw", "XYZW", "rgba", "RGBA", "stpq", "STPQ", "lh", "LH", "numeric" };

struct ed2a_swz *ed2a_make_swz_str(char *str) {
	struct ed2a_swz *res = ed2a_make_swz_empty();
	int i;
	int setnum = 0;
	for (i = 0; str[i]; i++) {
		int j, k;
		for (j = 1; j < ED2A_SWZ_NUM; j++) {
			for (k = 0; swz_sets[j][k]; k++) {
				if (str[i] == swz_sets[j][k]) {
					if (!setnum)
						setnum = j;
					if (setnum != j) {
						fprintf (stderr, "mismatched swizzle styles: %s and %s\n", swz_sets[setnum], swz_sets[j]);
						goto err;
					}
					ADDARRAY(res->elems, k);
					goto out;
				}
			}
		}
		fprintf (stderr, "unknown swizzle specifier: %c\n", str[i]);
err:
		free(str);
		free(res->elems);
		free(res);
		return 0;
out:;
	}
	free(str);
	res->style = setnum;
	return res;
}

struct ed2a_swz *ed2a_make_swz_cat(struct ed2a_swz *a, struct ed2a_swz *b) {
	if (a->style && b->style && a->style != b->style) {
		fprintf (stderr, "mismatched swizzle styles: %s and %s\n", swz_sets[a->style], swz_sets[b->style]);
		ed2a_del_swz(a);
		ed2a_del_swz(b);
		return 0;
	}
	int i;
	for (i = 0; i < b->elemsnum; i++) {
		ADDARRAY(a->elems, b->elems[i]);
	}
	if (!a->style)
		a->style = b->style;
	free(b->elems);
	free(b);
	return a;
}

void ed2a_del_file(struct ed2a_file *file) {
	int i;
	for (i = 0; i < file->insnsnum; i++)
		ed2a_del_insn(file->insns[i]);
	free(file->insns);
	free(file);
}

void ed2a_del_insn(struct ed2a_insn *insn) {
	int i;
	for (i = 0; i < insn->piecesnum; i++)
		ed2a_del_ipiece(insn->pieces[i]);
	free(insn->pieces);
	free(insn);
}

void ed2a_del_ipiece(struct ed2a_ipiece *ipiece) {
	int i;
	for (i = 0; i < ipiece->prefsnum; i++)
		ed2a_del_expr(ipiece->prefs[i]);
	free(ipiece->prefs);
	for (i = 0; i < ipiece->iopsnum; i++)
		ed2a_del_iop(ipiece->iops[i]);
	free(ipiece->iops);
	free(ipiece);
}

void ed2a_del_iop(struct ed2a_iop *iop) {
	int i;
	for (i = 0; i < iop->modsnum; i++)
		free(iop->mods[i]);
	free(iop->exprs);
	for (i = 0; i < iop->exprsnum; i++)
		ed2a_del_expr(iop->exprs[i]);
	free(iop->exprs);
	free(iop);
}

void ed2a_del_expr(struct ed2a_expr *expr) {
	if (expr->e1)
		ed2a_del_expr(expr->e1);
	if (expr->e2)
		ed2a_del_expr(expr->e2);
	if (expr->str)
		free(expr->str);
	if (expr->str2)
		free(expr->str2);
	if (expr->ipiece)
		ed2a_del_ipiece(expr->ipiece);
	if (expr->rvec)
		ed2a_del_rvec(expr->rvec);
	if (expr->swz)
		ed2a_del_swz(expr->swz);
	free(expr);
}

void ed2a_del_rvec(struct ed2a_rvec *rvec) {
	int i;
	for (i = 0; i < rvec->elemsnum; i++)
		free(rvec->elems[i]);
	free(rvec->elems);
	free(rvec);
}

void ed2a_del_swz(struct ed2a_swz *swz) {
	free(swz->elems);
	free(swz);
}
