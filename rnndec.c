#define _GNU_SOURCE // for asprintf
#include "rnndec.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

struct rnndeccolors rnndec_colorsterm = {
	.ceval = "\033[35m",
	.cimm = "\033[33m",
	.cname = "\033[32m",
	.cerr = "\033[31m",
	.cbool = "\033[36m",
	.cend = "\033[0m",
};

struct rnndeccolors rnndec_colorsnull = {
	.ceval = "",
	.cimm = "",
	.cname = "",
	.cerr = "",
	.cbool = "",
	.cend = "",
};

struct rnndeccontext *rnndec_newcontext(struct rnndb *db) {
	struct rnndeccontext *res = calloc (sizeof *res, 1);
	res->db = db;
	res->colors = &rnndec_colorsnull;
	return res;
}

int rnndec_varadd(struct rnndeccontext *ctx, char *varset, char *variant) {
	struct rnnenum *en = rnn_findenum(ctx->db, varset);
	if (!en) {
		fprintf (stderr, "Enum %s doesn't exist in database!\n", varset);
		return 0;
	}
	int i;
	for (i = 0; i < en->valsnum; i++)
		if (!strcmp(en->vals[i]->name, variant)) {
			struct rnndecvariant *ci = calloc (sizeof *ci, 1);
			ci->en = en;
			ci->variant = i;
			RNN_ADDARRAY(ctx->vars, ci);
			return 1;
		}
	fprintf (stderr, "Variant %s doesn't exist in enum %s!\n", variant, varset);
	return 0;
}

int rnndec_varmatch(struct rnndeccontext *ctx, struct rnnvarinfo *vi) {
	if (vi->dead)
		return 0;
	int i;
	for (i = 0; i < vi->varsetsnum; i++) {
		int j;
		for (j = 0; j < ctx->varsnum; j++)
			if (vi->varsets[i]->venum == ctx->vars[j]->en)
				break;
		if (j == ctx->varsnum) {
			fprintf (stderr, "I don't know which %s variant to use!\n", vi->varsets[i]->venum->name);
		} else {
			if (!vi->varsets[i]->variants[ctx->vars[j]->variant])
				return 0;
		}
	}
	return 1;
}

char *rnndec_decodeval(struct rnndeccontext *ctx, struct rnntypeinfo *ti, uint64_t value, int width) {
	char *res = 0;
	int i;
	struct rnnvalue **vals;
	int valsnum;
	struct rnnbitfield **bitfields;
	int bitfieldsnum;
	char *tmp;
	uint64_t mask;
	if (!ti)
		goto failhex;
	if (ti->shr) value <<= ti->shr;
	switch (ti->type) {
		case RNN_TTYPE_ENUM:
			vals = ti->eenum->vals;
			valsnum = ti->eenum->valsnum;
			goto doenum;
		case RNN_TTYPE_INLINE_ENUM:
			vals = ti->vals;
			valsnum = ti->valsnum;
			goto doenum;
		doenum:
			for (i = 0; i < valsnum; i++)
				if (rnndec_varmatch(ctx, &vals[i]->varinfo) && vals[i]->valvalid && vals[i]->value == value) {
					asprintf (&res, "%s%s%s", ctx->colors->ceval, vals[i]->name, ctx->colors->cend);
					return res;
				}
			goto failhex;
		case RNN_TTYPE_BITSET:
			bitfields = ti->ebitset->bitfields;
			bitfieldsnum = ti->ebitset->bitfieldsnum;
			goto dobitset;
		case RNN_TTYPE_INLINE_BITSET:
			bitfields = ti->bitfields;
			bitfieldsnum = ti->bitfieldsnum;
			goto dobitset;
		dobitset:
			mask = 0;
			for (i = 0; i < bitfieldsnum; i++) {
				if (!rnndec_varmatch(ctx, &bitfields[i]->varinfo))
					continue;
				uint64_t sval = (value & bitfields[i]->mask) >> bitfields[i]->low;
				mask |= bitfields[i]->mask;
				if (bitfields[i]->typeinfo.type == RNN_TTYPE_BOOLEAN) {
					if (sval == 0)
						continue;
					else if (sval == 1) {
						if (!res)
							asprintf (&res, "%s%s%s", ctx->colors->cbool, bitfields[i]->name, ctx->colors->cend);
						else {
							asprintf (&tmp, "%s | %s%s%s", res, ctx->colors->cbool, bitfields[i]->name, ctx->colors->cend);
							free(res);
							res = tmp;
						}
						continue;
					}
				}
				char *subval = rnndec_decodeval(ctx, &bitfields[i]->typeinfo, sval, bitfields[i]->high - bitfields[i]->low + 1);
				if (!res)
					asprintf (&res, "%s%s%s = %s", ctx->colors->cname, bitfields[i]->name, ctx->colors->cend, subval);
				else {
					asprintf (&tmp, "%s | %s%s%s = %s", res, ctx->colors->cname, bitfields[i]->name, ctx->colors->cend, subval);
					free(res);
					res = tmp;
				}
				free(subval);
			}
			if (value & ~mask) {
				if (!res)
					asprintf (&res, "%s%#"PRIx64"%s", ctx->colors->cerr, value & ~mask, ctx->colors->cend);
				else {
					asprintf (&tmp, "%s | %s%#"PRIx64"%s", res, ctx->colors->cerr, value & ~mask, ctx->colors->cend);
					free(res);
					res = tmp;
				}
			}
			if (!res)
				asprintf (&res, "%s0%s", ctx->colors->cimm, ctx->colors->cend);
			asprintf (&tmp, "{ %s }", res);
			free(res);
			return tmp;
		case RNN_TTYPE_SPECTYPE:
			return rnndec_decodeval(ctx, &ti->spectype->typeinfo, value, width);
		case RNN_TTYPE_HEX:
			asprintf (&res, "%s%#"PRIx64"%s", ctx->colors->cimm, value, ctx->colors->cend);
			return res;
		case RNN_TTYPE_UINT:
			asprintf (&res, "%s%"PRIu64"%s", ctx->colors->cimm, value, ctx->colors->cend);
			return res;
		case RNN_TTYPE_INT:
			if (value & UINT64_C(1) << (width-1))
				asprintf (&res, "%s-%"PRIi64"%s", ctx->colors->cimm, (UINT64_C(1) << width) - value, ctx->colors->cend);
			else
				asprintf (&res, "%s%"PRIi64"%s", ctx->colors->cimm, value, ctx->colors->cend);
			return res;
		case RNN_TTYPE_BOOLEAN:
			if (value == 0) {
				asprintf (&res, "%sFALSE%s", ctx->colors->cbool, ctx->colors->cend);
				return res;
			} else if (value == 1) {
				asprintf (&res, "%sTRUE%s", ctx->colors->cbool, ctx->colors->cend);
				return res;
			}
		case RNN_TTYPE_FLOAT:
			if (width == 64)
				asprintf(&res, "%s%f%s", ctx->colors->cimm,
					 *(double *)&value, ctx->colors->cend);
			else if (width == 32)
				asprintf(&res, "%s%f%s", ctx->colors->cimm,
					 *(float *)&value, ctx->colors->cend);
			else
				goto failhex;

			return res;
		failhex:
		default:
			asprintf (&res, "%s%#"PRIx64"%s", ctx->colors->cerr, value, ctx->colors->cend);
			return res;
			break;
	}
}

static char *appendidx (struct rnndeccontext *ctx, char *name, uint64_t idx) {
	char *res;
	asprintf (&res, "%s[%s%#"PRIx64"%s]", name, ctx->colors->cimm, idx, ctx->colors->cend);
	free (name);
	return res;
}

static struct rnndecaddrinfo *trymatch (struct rnndeccontext *ctx, struct rnndelem **elems, int elemsnum, uint64_t addr, int write, int dwidth, uint64_t *indices, int indicesnum) {
	struct rnndecaddrinfo *res;
	int i, j;
	for (i = 0; i < elemsnum; i++) {
		if (!rnndec_varmatch(ctx, &elems[i]->varinfo))
			continue;
		uint64_t offset, idx;
		char *tmp, *name;
		switch (elems[i]->type) {
			case RNN_ETYPE_REG:
				if (addr < elems[i]->offset)
					break;
				if (elems[i]->stride) {
					idx = (addr-elems[i]->offset)/elems[i]->stride;
					offset = (addr-elems[i]->offset)%elems[i]->stride;
				} else {
					idx = 0;
					offset = addr-elems[i]->offset;
				}
				if (offset >= elems[i]->width/dwidth)
					break;
				if (elems[i]->length && idx >= elems[i]->length)
					break;
				res = calloc (sizeof *res, 1);
				res->typeinfo = &elems[i]->typeinfo;
				res->width = elems[i]->width;
				asprintf (&res->name, "%s%s%s", ctx->colors->cname, elems[i]->name, ctx->colors->cend);
				for (j = 0; j < indicesnum; j++)
					res->name = appendidx(ctx, res->name, indices[j]);
				if (elems[i]->length != 1)
					res->name = appendidx(ctx, res->name, idx);
				if (offset) {
					asprintf (&tmp, "%s+%s%#"PRIx64"%s", res->name, ctx->colors->cerr, offset, ctx->colors->cend);
					free(res->name);
					res->name = tmp;
				}
				return res;
			case RNN_ETYPE_STRIPE:
				for (idx = 0; idx < elems[i]->length || !elems[i]->length; idx++) {
					if (addr < elems[i]->offset + elems[i]->stride * idx)
						break;
					offset = addr - (elems[i]->offset + elems[i]->stride * idx);
					int extraidx = (elems[i]->length != 1);
					int nindnum = (elems[i]->name ? 0 : indicesnum + extraidx);
					uint64_t nind[nindnum];
					if (!elems[i]->name) {
						for (j = 0; j < indicesnum; j++)
							nind[j] = indices[j];
						if (extraidx)
							nind[indicesnum] = idx;
					}
					res = trymatch (ctx, elems[i]->subelems, elems[i]->subelemsnum, offset, write, dwidth, nind, nindnum);
					if (!res)
						continue;
					if (!elems[i]->name)
						return res;
					asprintf (&name, "%s%s%s", ctx->colors->cname, elems[i]->name, ctx->colors->cend);
					for (j = 0; j < indicesnum; j++)
						name = appendidx(ctx, name, indices[j]);
					if (elems[i]->length != 1)
						name = appendidx(ctx, name, idx);
					asprintf (&tmp, "%s.%s", name, res->name);
					free(name);
					free(res->name);
					res->name = tmp;
					return res;
				}
				break;
			case RNN_ETYPE_ARRAY:
				if (addr < elems[i]->offset)
					break;
				idx = (addr-elems[i]->offset)/elems[i]->stride;
				offset = (addr-elems[i]->offset)%elems[i]->stride;
				if (elems[i]->length && idx >= elems[i]->length)
					break;
				asprintf (&name, "%s%s%s", ctx->colors->cname, elems[i]->name, ctx->colors->cend);
				for (j = 0; j < indicesnum; j++)
					name = appendidx(ctx, name, indices[j]);
				if (elems[i]->length != 1)
					name = appendidx(ctx, name, idx);
				if ((res = trymatch (ctx, elems[i]->subelems, elems[i]->subelemsnum, offset, write, dwidth, 0, 0))) {
					asprintf (&tmp, "%s.%s", name, res->name);
					free(name);
					free(res->name);
					res->name = tmp;
					return res;
				}
				res = calloc (sizeof *res, 1);
				asprintf (&tmp, "%s+%s%#"PRIx64"%s", name, ctx->colors->cerr, offset, ctx->colors->cend);
				free(name);
				res->name = tmp;
				return res;
			default:
				break;
		}
	}
	return 0;
}

struct rnndecaddrinfo *rnndec_decodeaddr(struct rnndeccontext *ctx, struct rnndomain *domain, uint64_t addr, int write) {
	struct rnndecaddrinfo *res = trymatch(ctx, domain->subelems, domain->subelemsnum, addr, write, domain->width, 0, 0);
	if (res)
		return res;
	res = calloc (sizeof *res, 1);
	asprintf (&res->name, "%s%#"PRIx64"%s", ctx->colors->cerr, addr, ctx->colors->cend);
	return res;
}
