/*
 * Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
 * Copyright (C) 2010 Luca Barbieri <luca@luca-barbieri.com>
 * Copyright (C) 2010 Francisco Jerez <currojerez@riseup.net>
 * Copyright (C) 2010 Martin Peres <martin.peres@ensi-bourges.fr>
 * Copyright (C) 2010 Marcin Slusarz <marcin.slusarz@gmail.com>
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

#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlreader.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include "rnn.h"
#include "rnn/rnn_path.h"
#include "util.h"

static char *catstr (char *a, char *b) {
	if (!a)
		return strdup(b);
	return aprintf("%s_%s", a, b);
}

static int strdiff (const char *a, const char *b) {
	if (!a && !b)
		return 0;
	if (!a || !b)
		return 1;
	return strcmp (a, b);
}

void rnn_init() {
	LIBXML_TEST_VERSION
	xmlInitParser();
}

void rnn_fini() {
	xmlCleanupParser();
}

struct rnndb *rnn_newdb() {
	struct rnndb *db = calloc(sizeof *db, 1);
	return db;
}

static char *getcontent (xmlNode *attr) {
	xmlNode *chain = attr->children;
	size_t size = 0;
	char *content, *p;
	while (chain) {
		if (chain->type == XML_TEXT_NODE)
			size += strlen(chain->content);
		chain = chain->next;
	}
	p = content = malloc(size + 1);
	chain = attr->children;
	while (chain) {
		if (chain->type == XML_TEXT_NODE) {
			char* sp = chain->content;
			if(p == content) {
				while(isspace(*sp))
					++sp;
			}
			size_t len = strlen(sp);
			memcpy(p, sp, len);
			p += len;
		}
		chain = chain->next;
	}
	while(p != content && isspace(p[-1]))
		--p;
	*p = 0;
	return content;
}

static char *getattrib (struct rnndb *db, char *file, int line, xmlAttr *attr) {
	xmlNode *chain = attr->children;
	while (chain) {
		if (chain->type != XML_TEXT_NODE) {
			fprintf (stderr, "%s:%d: unknown attribute child \"%s\" in attribute \"%s\"\n", file, line, chain->name, attr->name);
			db->estatus = 1;
		} else {
			return chain->content;
		}
		chain = chain->next;
	}
	return "";
}

static int getboolattrib (struct rnndb *db, char *file, int line, xmlAttr *attr) {
	char *c = getattrib(db, file, line, attr);
	if (!strcmp(c, "yes") || !strcmp(c, "1"))
		return 1;
	if (!strcmp(c, "no") || !strcmp(c, "0"))
		return 0;
	fprintf (stderr, "%s:%d: invalid boolean value \"%s\" in attribute \"%s\"\n", file, line, c, attr->name);
	db->estatus = 1;
	return 0;
}

static uint64_t getnumattrib (struct rnndb *db, char *file, int line, xmlAttr *attr) {
	char *c = getattrib(db, file, line, attr);
	char *cc;
	uint64_t res;
	if (strchr(c, 'x') || strchr(c, 'X'))
		res = strtoull(c, &cc, 16);
	else
		res = strtoull(c, &cc, 10);
	if (*cc)  {
		fprintf (stderr, "%s:%d: invalid numeric value \"%s\" in attribute \"%s\"\n", file, line, c, attr->name);
		db->estatus = 1;
	}
	return res;
}

static int trytop (struct rnndb *db, char *file, xmlNode *node);

static int trydoc (struct rnndb *db, char *file, xmlNode *node) {
	if (!strcmp(node->name, "brief")) {
		return 1;
	} else if (!strcmp(node->name, "doc")) {
		return 1;
	}
	return 0;
}

static struct rnnvalue *parsevalue(struct rnndb *db, char *file, xmlNode *node);
static struct rnnbitfield *parsebitfield(struct rnndb *db, char *file, xmlNode *node);

static int trytypetag (struct rnndb *db, char *file, xmlNode *node, struct rnntypeinfo *ti) {
	if (!strcmp(node->name, "value")) {
		struct rnnvalue *val = parsevalue(db, file, node);
		if (val)
			ADDARRAY(ti->vals, val);
		return 1;
	} else if (!strcmp(node->name, "bitfield")) {
		struct rnnbitfield *bf = parsebitfield(db, file, node);
		if (bf)
			ADDARRAY(ti->bitfields, bf);
		return 1;
	}
	return 0;
}
static int trytypeattr (struct rnndb *db, char *file, xmlNode *node, xmlAttr *attr, struct rnntypeinfo *ti) {
	if (!strcmp(attr->name, "shr")) {
		ti->shr = getnumattrib(db, file, node->line, attr);
		return 1;
	} else if (!strcmp(attr->name, "add")) {
		ti->add = getnumattrib(db, file, node->line, attr);
		return 1;
	} else if (!strcmp(attr->name, "min")) {
		ti->min = getnumattrib(db, file, node->line, attr);
		ti->minvalid = 1;
		return 1;
	} else if (!strcmp(attr->name, "max")) {
		ti->max = getnumattrib(db, file, node->line, attr);
		ti->maxvalid = 1;
		return 1;
	} else if (!strcmp(attr->name, "align")) {
		ti->align = getnumattrib(db, file, node->line, attr);
		ti->alignvalid = 1;
		return 1;
	} else if (!strcmp(attr->name, "type")) {
		ti->name = strdup(getattrib(db, file, node->line, attr));;
		return 1;
	} else if (!strcmp(attr->name, "radix")) {
		ti->radix = getnumattrib(db, file, node->line, attr);
		ti->radixvalid = 1;
		return 1;
	}
	return 0;
}

static struct rnnvalue *parsevalue(struct rnndb *db, char *file, xmlNode *node) {
	struct rnnvalue *val = calloc(sizeof *val, 1);
	val->file = file;
	xmlAttr *attr = node->properties;
	while (attr) {
		if (!strcmp(attr->name, "name")) {
			val->name = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "value")) {
			val->value = getnumattrib(db, file, node->line, attr);
			val->valvalid = 1;
		} else if (!strcmp(attr->name, "varset")) {
			val->varinfo.varsetstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "variants")) {
			val->varinfo.variantsstr = strdup(getattrib(db, file, node->line, attr));
		} else {
			fprintf (stderr, "%s:%d: wrong attribute \"%s\" for value\n", file, node->line, attr->name);
			db->estatus = 1;
		}
		attr = attr->next;
	}
	xmlNode *chain = node->children;
	while (chain) {
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (!trytop(db, file, chain) && !trydoc(db, file, chain)) {
			fprintf (stderr, "%s:%d: wrong tag in %s: <%s>\n", file, chain->line, node->name, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
	if (!val->name) {
		fprintf (stderr, "%s:%d: nameless value\n", file, node->line);
		db->estatus = 1;
		return 0;
	} else {
		return val;
	}
}

static void parsespectype(struct rnndb *db, char *file, xmlNode *node) {
	struct rnnspectype *res = calloc (sizeof *res, 1);
	res->file = file;
	xmlAttr *attr = node->properties;
	int i;
	while (attr) {
		if (!strcmp(attr->name, "name")) {
			res->name = strdup(getattrib(db, file, node->line, attr));
		} else if (!trytypeattr(db, file, node, attr, &res->typeinfo)) {
			fprintf (stderr, "%s:%d: wrong attribute \"%s\" for spectype\n", file, node->line, attr->name);
			db->estatus = 1;
		}
		attr = attr->next;
	}
	if (!res->name) {
		fprintf (stderr, "%s:%d: nameless spectype\n", file, node->line);
		db->estatus = 1;
		return;
	}
	for (i = 0; i < db->spectypesnum; i++)
		if (!strcmp(db->spectypes[i]->name, res->name)) {
			fprintf (stderr, "%s:%d: duplicated spectype name %s\n", file, node->line, res->name);
			db->estatus = 1;
			return;
		}
	ADDARRAY(db->spectypes, res);
	xmlNode *chain = node->children;
	while (chain) {
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (!trytypetag(db, file, chain, &res->typeinfo) && !trytop(db, file, chain) && !trydoc(db, file, chain)) {
			fprintf (stderr, "%s:%d: wrong tag in spectype: <%s>\n", file, chain->line, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
}

static void parseenum(struct rnndb *db, char *file, xmlNode *node) {
	xmlAttr *attr = node->properties;
	char *name = 0;
	int isinline = 0;
	int bare = 0;
	char *prefixstr = 0;
	char *varsetstr = 0;
	char *variantsstr = 0;
	int i;
	while (attr) {
		if (!strcmp(attr->name, "name")) {
			name = getattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "bare")) {
			bare = getboolattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "inline")) {
			isinline = getboolattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "prefix")) {
			prefixstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "varset")) {
			varsetstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "variants")) {
			variantsstr = strdup(getattrib(db, file, node->line, attr));
		} else {
			fprintf (stderr, "%s:%d: wrong attribute \"%s\" for enum\n", file, node->line, attr->name);
			db->estatus = 1;
		}
		attr = attr->next;
	}
	if (!name) {
		fprintf (stderr, "%s:%d: nameless enum\n", file, node->line);
		db->estatus = 1;
		return;
	}
	struct rnnenum *cur = 0;
	for (i = 0; i < db->enumsnum; i++)
		if (!strcmp(db->enums[i]->name, name)) {
			cur = db->enums[i];
			break;
		}
	if (cur) {
		if (strdiff(cur->varinfo.prefixstr, prefixstr) ||
				strdiff(cur->varinfo.varsetstr, varsetstr) ||
				strdiff(cur->varinfo.variantsstr, variantsstr) ||
				cur->isinline != isinline || cur->bare != bare) {
			fprintf (stderr, "%s:%d: merge fail for enum %s\n", file, node->line, node->name);
			db->estatus = 1;
		}
		free(prefixstr);
		free(varsetstr);
		free(variantsstr);
	} else {
		cur = calloc(sizeof *cur, 1);
		cur->name = strdup(name);
		cur->isinline = isinline;
		cur->bare = bare;
		cur->varinfo.prefixstr = prefixstr;
		cur->varinfo.varsetstr = varsetstr;
		cur->varinfo.variantsstr = variantsstr;
		cur->file = file;
		ADDARRAY(db->enums, cur);
	}
	xmlNode *chain = node->children;
	while (chain) {
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (!strcmp(chain->name, "value")) {
			struct rnnvalue *val = parsevalue(db, file, chain);
			if (val)
				ADDARRAY(cur->vals, val);
		} else if (!trytop(db, file, chain) && !trydoc(db, file, chain)) {
			fprintf (stderr, "%s:%d: wrong tag in enum: <%s>\n", file, chain->line, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
}

static struct rnnbitfield *parsebitfield(struct rnndb *db, char *file, xmlNode *node) {
	struct rnnbitfield *bf = calloc(sizeof *bf, 1);
	bf->file = file;
	xmlAttr *attr = node->properties;
	int highok = 0, lowok = 0;
	while (attr) {
		if (!strcmp(attr->name, "name")) {
			bf->name = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "high")) {
			bf->high = getnumattrib(db, file, node->line, attr);
			highok = 1;
		} else if (!strcmp(attr->name, "low")) {
			bf->low = getnumattrib(db, file, node->line, attr);
			lowok = 1;
		} else if (!strcmp(attr->name, "pos")) {
			bf->high = bf->low = getnumattrib(db, file, node->line, attr);
			lowok = highok = 1;
		} else if (!strcmp(attr->name, "varset")) {
			bf->varinfo.varsetstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "variants")) {
			bf->varinfo.variantsstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!trytypeattr(db, file, node, attr, &bf->typeinfo)) {
			fprintf (stderr, "%s:%d: wrong attribute \"%s\" for bitfield\n", file, node->line, attr->name);
			db->estatus = 1;
		}
		attr = attr->next;
	}
	xmlNode *chain = node->children;
	while (chain) {
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (!trytypetag(db, file, chain, &bf->typeinfo) && !trytop(db, file, chain) && !trydoc(db, file, chain)) {
			fprintf (stderr, "%s:%d: wrong tag in %s: <%s>\n", file, chain->line, node->name, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
	if (!bf->name) {
		fprintf (stderr, "%s:%d: nameless bitfield\n", file, node->line);
		db->estatus = 1;
		return 0;
	} else if (!highok || !lowok || bf->high < bf->low) {
		fprintf (stderr, "%s:%d: bitfield has wrong placement\n", file, node->line);
		db->estatus = 1;
		return 0;
	} else {
		return bf;
	}
}

static void parsebitset(struct rnndb *db, char *file, xmlNode *node) {
	xmlAttr *attr = node->properties;
	char *name = 0;
	int isinline = 0;
	int bare = 0;
	char *prefixstr = 0;
	char *varsetstr = 0;
	char *variantsstr = 0;
	int i;
	while (attr) {
		if (!strcmp(attr->name, "name")) {
			name = getattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "bare")) {
			bare = getboolattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "inline")) {
			isinline = getboolattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "prefix")) {
			prefixstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "varset")) {
			varsetstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "variants")) {
			variantsstr = strdup(getattrib(db, file, node->line, attr));
		} else {
			fprintf (stderr, "%s:%d: wrong attribute \"%s\" for bitset\n", file, node->line, attr->name);
			db->estatus = 1;
		}
		attr = attr->next;
	}
	if (!name) {
		fprintf (stderr, "%s:%d: nameless bitset\n", file, node->line);
		db->estatus = 1;
		return;
	}
	struct rnnbitset *cur = 0;
	for (i = 0; i < db->bitsetsnum; i++)
		if (!strcmp(db->bitsets[i]->name, name)) {
			cur = db->bitsets[i];
			break;
		}
	if (cur) {
		if (strdiff(cur->varinfo.prefixstr, prefixstr) ||
				strdiff(cur->varinfo.varsetstr, varsetstr) ||
				strdiff(cur->varinfo.variantsstr, variantsstr) ||
				cur->isinline != isinline || cur->bare != bare) {
			fprintf (stderr, "%s:%d: merge fail for bitset %s\n", file, node->line, node->name);
			db->estatus = 1;
		}
		free(prefixstr);
		free(varsetstr);
		free(variantsstr);
	} else {
		cur = calloc(sizeof *cur, 1);
		cur->name = strdup(name);
		cur->isinline = isinline;
		cur->bare = bare;
		cur->varinfo.prefixstr = prefixstr;
		cur->varinfo.varsetstr = varsetstr;
		cur->varinfo.variantsstr = variantsstr;
		cur->file = file;
		ADDARRAY(db->bitsets, cur);
	}
	xmlNode *chain = node->children;
	while (chain) {
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (!strcmp(chain->name, "bitfield")) {
			struct rnnbitfield *bf = parsebitfield(db, file, chain);
			if (bf)
				ADDARRAY(cur->bitfields, bf);
		} else if (!trytop(db, file, chain) && !trydoc(db, file, chain)) {
			fprintf (stderr, "%s:%d: wrong tag in bitset: <%s>\n", file, chain->line, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
}

static struct rnndelem *trydelem(struct rnndb *db, char *file, xmlNode *node) {
	if (!strcmp(node->name, "use-group")) {
		struct rnndelem *res = calloc(sizeof *res, 1);
		res->file = file;
		res->type = RNN_ETYPE_USE_GROUP;
		xmlAttr *attr = node->properties;
		while (attr) {
			if (!strcmp(attr->name, "name")) {
				res->name = strdup(getattrib(db, file, node->line, attr));
			} else {
				fprintf (stderr, "%s:%d: wrong attribute \"%s\" for %s\n", file, node->line, attr->name, node->name);
				db->estatus = 1;
			}
			attr = attr->next;
		}
		if (!res->name) {
			fprintf (stderr, "%s:%d: nameless use-group\n", file, node->line);
			free(res);
			db->estatus = 1;
			return 0;
		}
		return res;
	} else if (!strcmp(node->name, "stripe") || !strcmp(node->name, "array")) {
		struct rnndelem *res = calloc(sizeof *res, 1);
		res->type = (strcmp(node->name, "stripe")?RNN_ETYPE_ARRAY:RNN_ETYPE_STRIPE);
		res->length = 1;
		res->file = file;
		xmlAttr *attr = node->properties;
		while (attr) {
			if (!strcmp(attr->name, "name")) {
				res->name = strdup(getattrib(db, file, node->line, attr));
			} else if (!strcmp(attr->name, "offset")) {
				res->offset = getnumattrib(db, file, node->line, attr);
			} else if (!strcmp(attr->name, "length")) {
				res->length = getnumattrib(db, file, node->line, attr);
			} else if (!strcmp(attr->name, "stride")) {
				res->stride = getnumattrib(db, file, node->line, attr);
			} else if (!strcmp(attr->name, "prefix")) {
				res->varinfo.prefixstr = strdup(getattrib(db, file, node->line, attr));
			} else if (!strcmp(attr->name, "varset")) {
				res->varinfo.varsetstr = strdup(getattrib(db, file, node->line, attr));
			} else if (!strcmp(attr->name, "variants")) {
				res->varinfo.variantsstr = strdup(getattrib(db, file, node->line, attr));
			} else {
				fprintf (stderr, "%s:%d: wrong attribute \"%s\" for %s\n", file, node->line, attr->name, node->name);
				db->estatus = 1;
			}
			attr = attr->next;
		}
		xmlNode *chain = node->children;
		while (chain) {
			struct rnndelem *delem;
			if (chain->type != XML_ELEMENT_NODE) {
			} else if ((delem = trydelem(db, file, chain))) {
				ADDARRAY(res->subelems, delem);
			} else if (!trytop(db, file, chain) && !trydoc(db, file, chain)) {
				fprintf (stderr, "%s:%d: wrong tag in %s: <%s>\n", file, chain->line, node->name, chain->name);
				db->estatus = 1;
			}
			chain = chain->next;
		}

		/* Sanity checking */
		if (res->type == RNN_ETYPE_ARRAY && res->stride == 0) {
			fprintf(stderr, "%s: Array %s's stride is undefined. Aborting.\n", file, res->name);
			exit(-1);
		}
		return res;

	}
	int width;
	if (!strcmp(node->name, "reg8"))
		width = 8;
	else if (!strcmp(node->name, "reg16"))
		width = 16;
	else if (!strcmp(node->name, "reg32"))
		width = 32;
	else if (!strcmp(node->name, "reg64"))
		width = 64;
	else
		return 0;
	struct rnndelem *res = calloc(sizeof *res, 1);
	res->file = file;
	res->type = RNN_ETYPE_REG;
	res->width = width;
	res->length = 1;
	res->access = RNN_ACCESS_RW;
	xmlAttr *attr = node->properties;
	while (attr) {
		if (!strcmp(attr->name, "name")) {
			res->name = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "offset")) {
			res->offset = getnumattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "length")) {
			res->length = getnumattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "stride")) {
			res->stride = getnumattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "varset")) {
			res->varinfo.varsetstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "variants")) {
			res->varinfo.variantsstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "access")) {
			char *str = getattrib(db, file, node->line, attr);
			if (!strcmp(str, "r"))
				res->access = RNN_ACCESS_R;
			else if (!strcmp(str, "w"))
				res->access = RNN_ACCESS_W;
			else if (!strcmp(str, "rw"))
				res->access = RNN_ACCESS_RW;
			else
				fprintf (stderr, "%s:%d: wrong access type \"%s\" for register\n", file, node->line, str);
		} else if (!trytypeattr(db, file, node, attr, &res->typeinfo)) {
			fprintf (stderr, "%s:%d: wrong attribute \"%s\" for register\n", file, node->line, attr->name);
			db->estatus = 1;
		}
		attr = attr->next;
	}
	xmlNode *chain = node->children;
	while (chain) {
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (!trytypetag(db, file, chain, &res->typeinfo) && !trytop(db, file, chain) && !trydoc(db, file, chain)) {
			fprintf (stderr, "%s:%d: wrong tag in %s: <%s>\n", file, chain->line, node->name, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
	if (!res->name) {
		fprintf (stderr, "%s:%d: nameless register\n", file, node->line);
		db->estatus = 1;
		return 0;
	} else {
	}
	return res;
}

static void parsegroup(struct rnndb *db, char *file, xmlNode *node) {
	xmlAttr *attr = node->properties;
	char *name = 0;
	int i;
	while (attr) {
		if (!strcmp(attr->name, "name")) {
			name = getattrib(db, file, node->line, attr);
		} else {
			fprintf (stderr, "%s:%d: wrong attribute \"%s\" for group\n", file, node->line, attr->name);
			db->estatus = 1;
		}
		attr = attr->next;
	}
	if (!name) {
		fprintf (stderr, "%s:%d: nameless group\n", file, node->line);
		db->estatus = 1;
		return;
	}
	struct rnngroup *cur = 0;
	for (i = 0; i < db->groupsnum; i++)
		if (!strcmp(db->groups[i]->name, name)) {
			cur = db->groups[i];
			break;
		}
	if (!cur) {
		cur = calloc(sizeof *cur, 1);
		cur->name = strdup(name);
		ADDARRAY(db->groups, cur);
	}
	xmlNode *chain = node->children;
	while (chain) {
		struct rnndelem *delem;
		if (chain->type != XML_ELEMENT_NODE) {
		} else if ((delem = trydelem(db, file, chain))) {
			ADDARRAY(cur->subelems, delem);
		} else if (!trytop(db, file, chain) && !trydoc(db, file, chain)) {
			fprintf (stderr, "%s:%d: wrong tag in group: <%s>\n", file, chain->line, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
}

static void parsedomain(struct rnndb *db, char *file, xmlNode *node) {
	xmlAttr *attr = node->properties;
	char *name = 0;
	uint64_t size = 0; int width = 8;
	int bare = 0;
	char *prefixstr = 0;
	char *varsetstr = 0;
	char *variantsstr = 0;
	int i;
	while (attr) {
		if (!strcmp(attr->name, "name")) {
			name = getattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "bare")) {
			bare = getboolattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "size")) {
			size = getnumattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "width")) {
			width = getnumattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "prefix")) {
			prefixstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "varset")) {
			varsetstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "variants")) {
			variantsstr = strdup(getattrib(db, file, node->line, attr));
		} else {
			fprintf (stderr, "%s:%d: wrong attribute \"%s\" for domain\n", file, node->line, attr->name);
			db->estatus = 1;
		}
		attr = attr->next;
	}
	if (!name) {
		fprintf (stderr, "%s:%d: nameless domain\n", file, node->line);
		db->estatus = 1;
		return;
	}
	struct rnndomain *cur = 0;
	for (i = 0; i < db->domainsnum; i++)
		if (!strcmp(db->domains[i]->name, name)) {
			cur = db->domains[i];
			break;
		}
	if (cur) {
		if (strdiff(cur->varinfo.prefixstr, prefixstr) ||
				strdiff(cur->varinfo.varsetstr, varsetstr) ||
				strdiff(cur->varinfo.variantsstr, variantsstr) ||
				cur->width != width ||
				cur->bare != bare ||
				(size && cur->size && size != cur->size)) {
			fprintf (stderr, "%s:%d: merge fail for domain %s\n", file, node->line, node->name);
			db->estatus = 1;
		} else {
			if (size)
				cur->size = size;
		}
		free(prefixstr);
		free(varsetstr);
		free(variantsstr);
	} else {
		cur = calloc(sizeof *cur, 1);
		cur->name = strdup(name);
		cur->bare = bare;
		cur->width = width;
		cur->size = size;
		cur->varinfo.prefixstr = prefixstr;
		cur->varinfo.varsetstr = varsetstr;
		cur->varinfo.variantsstr = variantsstr;
		cur->file = file;
		ADDARRAY(db->domains, cur);
	}
	xmlNode *chain = node->children;
	while (chain) {
		struct rnndelem *delem;
		if (chain->type != XML_ELEMENT_NODE) {
		} else if ((delem = trydelem(db, file, chain))) {
			ADDARRAY(cur->subelems, delem);
		} else if (!trytop(db, file, chain) && !trydoc(db, file, chain)) {
			fprintf (stderr, "%s:%d: wrong tag in domain: <%s>\n", file, chain->line, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
}

static void parsecopyright(struct rnndb *db, char *file, xmlNode *node) {
	struct rnncopyright* copyright = &db->copyright;
	xmlAttr *attr = node->properties;
	while (attr) {
		if (!strcmp(attr->name, "year")) {
			unsigned firstyear = getnumattrib(db, file, node->line, attr);
			if(!copyright->firstyear || firstyear < copyright->firstyear)
				copyright->firstyear = firstyear;
		} else {
			fprintf (stderr, "%s:%d: wrong attribute \"%s\" for copyright\n", file, node->line, attr->name);
			db->estatus = 1;
		}
		attr = attr->next;
	}
	xmlNode *chain = node->children;
	while (chain) {
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (!strcmp(chain->name, "license"))
			if(copyright->license) {
				if(strcmp(copyright->license, node->content)) {
					fprintf(stderr, "fatal error: multiple different licenses specified!\n");
					abort(); /* TODO: do something better here, but headergen, xml2html, etc. should not produce anything in this case */
				}
			} else
				copyright->license = getcontent(chain);
		else if (!strcmp(chain->name, "author")) {
			struct rnnauthor* author = calloc(sizeof *author, 1);
			xmlAttr* authorattr = chain->properties;
			xmlNode *authorchild = chain->children;
			author->contributions = getcontent(chain);
			while (authorattr) {
				if (!strcmp(authorattr->name, "name"))
					author->name = strdup(getattrib(db, file, chain->line, authorattr));
				else if (!strcmp(authorattr->name, "email"))
					author->email = strdup(getattrib(db, file, chain->line, authorattr));
				else {
					fprintf (stderr, "%s:%d: wrong attribute \"%s\" for author\n", file, chain->line, authorattr->name);
					db->estatus = 1;
				}
				authorattr = authorattr->next;
			}
			while(authorchild)  {
				if (authorchild->type != XML_ELEMENT_NODE) {
				} else if (!strcmp(authorchild->name, "nick")) {
					xmlAttr* nickattr = authorchild->properties;
					char* nickname = 0;
					while(nickattr) {
						if (!strcmp(nickattr->name, "name"))
							nickname = strdup(getattrib(db, file, authorchild->line, nickattr));
						else {
							fprintf (stderr, "%s:%d: wrong attribute \"%s\" for nick\n", file, authorchild->line, nickattr->name);
							db->estatus = 1;
						}
						nickattr = nickattr->next;
					}
					if(!nickname) {
						fprintf (stderr, "%s:%d: missing \"name\" attribute for nick\n", file, authorchild->line);
						db->estatus = 1;
					} else
						ADDARRAY(author->nicknames, nickname);
				} else {
					fprintf (stderr, "%s:%d: wrong tag in author: <%s>\n", file, authorchild->line, authorchild->name);
					db->estatus = 1;
				}
				authorchild = authorchild->next;
			}
			ADDARRAY(copyright->authors, author);
		} else {
			fprintf (stderr, "%s:%d: wrong tag in copyright: <%s>\n", file, chain->line, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
}

static void freeauthor(struct rnnauthor *author) {
	int i;
	for (i = 0; i < author->nicknamesnum; i++)
		free(author->nicknames[i]);
	free(author->nicknames);

	free(author->contributions);
	free(author->email);
	free(author->license);
	free(author->name);
	free(author);
}

static void cleanupcopyright(struct rnncopyright *copyright) {
	int i;
	for (i = 0; i < copyright->authorsnum; i++)
		freeauthor(copyright->authors[i]);
	free(copyright->authors);
	free(copyright->license);
}

static int trytop (struct rnndb *db, char *file, xmlNode *node) {
	if (!strcmp(node->name, "enum")) {
		parseenum(db, file, node);
		return 1;
	} else if (!strcmp(node->name, "bitset")) {
		parsebitset(db, file, node);
		return 1;
	} else if (!strcmp(node->name, "group")) {
		parsegroup(db, file, node);
		return 1;
	} else if (!strcmp(node->name, "domain")) {
		parsedomain(db, file, node);
		return 1;
	} else if (!strcmp(node->name, "spectype")) {
		parsespectype(db, file, node);
		return 1;
	} else if (!strcmp(node->name, "import")) {
		xmlAttr *attr = node->properties;
		char *subfile = 0;
		while (attr) {
			if (!strcmp(attr->name, "file")) {
				subfile = getattrib(db, file, node->line, attr);
			} else {
				fprintf (stderr, "%s:%d: wrong attribute \"%s\" for import\n", file, node->line, attr->name);
				db->estatus = 1;
			}
			attr = attr->next;
		}
		if (!subfile) {
			fprintf (stderr, "%s:%d: missing \"file\" attribute for import\n", file, node->line);
			db->estatus = 1;
		} else {
			rnn_parsefile(db, subfile);
		}
		return 1;
	} else if (!strcmp(node->name, "copyright")) {
		parsecopyright(db, file, node);
		return 1;
	}
	return 0;
}

void rnn_parsefile (struct rnndb *db, char *file_orig) {
	int i;
	char *fname;
	const char *rnn_path = getenv("RNN_PATH");

	if (!rnn_path)
		rnn_path = RNN_DEF_PATH;

	FILE *file = find_in_path(file_orig, rnn_path, &fname);
	if (!file) {
		fprintf (stderr, "%s: couldn't find database file. Please set the env var RNN_PATH.\n", file_orig);
		db->estatus = 1;
		return;
	}
	fclose(file);

	for (i = 0; i < db->filesnum; i++)
		if (!strcmp(db->files[i], fname)) {
			free(fname);
			return;
		}
		
	ADDARRAY(db->files, fname);
	xmlDocPtr doc = xmlParseFile(fname);
	if (!doc) {
		fprintf (stderr, "%s: couldn't open database file. Please set the env var RNN_PATH.\n", fname);
		db->estatus = 1;
		return;
	}
	xmlNode *root = doc->children;
	while (root) {
		if (root->type != XML_ELEMENT_NODE) {
		} else if (strcmp(root->name, "database")) {
			fprintf (stderr, "%s:%d: wrong top-level tag <%s>\n", fname, root->line, root->name);
			db->estatus = 1;
		} else {
			xmlNode *chain = root->children;
			while (chain) {
				if (chain->type != XML_ELEMENT_NODE) {
				} else if (!trytop(db, fname, chain) && !trydoc(db, fname, chain)) {
					fprintf (stderr, "%s:%d: wrong tag in database: <%s>\n", fname, chain->line, chain->name);
					db->estatus = 1;
				}
				chain = chain->next;
			}
		}
		root = root->next;
	}
	xmlFreeDoc(doc);
}

static struct rnnvarset *copyvarset (struct rnnvarset *varset);

static void copyvarinfo (struct rnnvarinfo *dst, struct rnnvarinfo *src) {
	int i;
	memset(dst, 0, sizeof(*dst));
	if (src->prefix)
		dst->prefix = strdup(src->prefix);
	if (src->prefixstr)
		dst->prefixstr = strdup(src->prefixstr);
	if (src->varsetstr)
		dst->varsetstr = strdup(src->varsetstr);
	if (src->variantsstr)
		dst->variantsstr = strdup(src->variantsstr);
	dst->dead = src->dead;
	dst->prefenum = src->prefenum;

	for (i = 0; i < src->varsetsnum; i++)
		ADDARRAY(dst->varsets, copyvarset(src->varsets[i]));
}

static struct rnnvalue *copyvalue (struct rnnvalue *val, char *file) {
	struct rnnvalue *res = calloc (sizeof *res, 1);
	res->name = strdup(val->name);
	res->valvalid = val->valvalid;
	res->value = val->value;
	copyvarinfo(&res->varinfo, &val->varinfo);
	res->file = file;
	return res;
}

static struct rnnbitfield *copybitfield (struct rnnbitfield *bf, char *file);


static void copytypeinfo (struct rnntypeinfo *dst, struct rnntypeinfo *src, char *file) {
	int i;
	dst->name = src->name;
	if (dst->name)
		dst->name = strdup(dst->name);
	dst->shr = src->shr;
	dst->add = src->add;
	dst->min = src->min;
	dst->max = src->max;
	dst->align = src->align;
	for (i = 0; i < src->valsnum; i++)
		ADDARRAY(dst->vals, copyvalue(src->vals[i], file));
	for (i = 0; i < src->bitfieldsnum; i++)
		ADDARRAY(dst->bitfields, copybitfield(src->bitfields[i], file));
}

static struct rnnbitfield *copybitfield (struct rnnbitfield *bf, char *file) {
	struct rnnbitfield *res = calloc (sizeof *res, 1);
	res->name = strdup(bf->name);
	res->low = bf->low;
	res->high = bf->high;
	copyvarinfo(&res->varinfo, &bf->varinfo);
	res->file = file;
	copytypeinfo(&res->typeinfo, &bf->typeinfo, file);
	return res;
}

static struct rnndelem *copydelem (struct rnndelem *elem, char *file) {
	struct rnndelem *res = calloc (sizeof *res, 1);
	res->type = elem->type;
	res->name = elem->name;
	if (res->name)
		res->name = strdup(res->name);
	res->width = elem->width;
	res->access = elem->access;
	res->offset = elem->offset;
	res->length = elem->length;
	res->stride = elem->stride;
	copyvarinfo(&res->varinfo, &elem->varinfo);
	res->file = file;
	copytypeinfo(&res->typeinfo, &elem->typeinfo, file);
	int i;
	for (i = 0; i < elem->subelemsnum; i++)
		ADDARRAY(res->subelems, copydelem(elem->subelems[i], file));
	return res;
}

static struct rnnvarset *copyvarset (struct rnnvarset *varset) {
	struct rnnvarset *res = calloc(sizeof *res, 1);
	res->venum = varset->venum;
	res->variants = calloc(sizeof *res->variants, res->venum->valsnum);
	int i;
	for (i = 0; i < res->venum->valsnum; i++)
		res->variants[i] = varset->variants[i];
	return res;
}

static void freevarset(struct rnnvarset *varset) {
	free(varset->variants);
	free(varset);
}

static void prepenum(struct rnndb *db, struct rnnenum *en);

static int findvidx (struct rnndb *db, struct rnnenum *en, char *name) {
	int i;
	for (i = 0; i < en->valsnum; i++)
		if (!strcmp(en->vals[i]->name, name))
			return i;
	fprintf (stderr, "Cannot find variant %s in enum %s!\n", name, en->name);
	db->estatus = 1;
	return -1;
}

static void prepvarinfo (struct rnndb *db, char *what, struct rnnvarinfo *vi, struct rnnvarinfo *parent) {
	if (parent)
		vi->prefenum = parent->prefenum;
	if (vi->prefixstr) {
		if (!strcmp(vi->prefixstr, "none"))
			vi->prefenum = 0;
		else
			vi->prefenum = rnn_findenum(db, vi->prefixstr); // XXX
	}
	int i;
	if (parent)
		for (i = 0; i < parent->varsetsnum; i++)
			ADDARRAY(vi->varsets, copyvarset(parent->varsets[i]));
	struct rnnenum *varset = vi->prefenum;
	if (!varset && !vi->varsetstr && parent)
		vi->varsetstr = parent->varsetstr ? strdup(parent->varsetstr) : NULL;
	if (vi->varsetstr)
		varset = rnn_findenum(db, vi->varsetstr);
	if (vi->variantsstr) {
		char *vars = vi->variantsstr;
		if (!varset) {
			fprintf (stderr, "%s: tried to use variants without active varset!\n", what);
			db->estatus = 1;
			return;
		}
		struct rnnvarset *vs = 0;
		int nvars = varset->valsnum;
		for (i = 0; i < vi->varsetsnum; i++)
			if (vi->varsets[i]->venum == varset) {
				vs = vi->varsets[i];
				break;
			}
		if (!vs) {
			vs = calloc (sizeof *vs, 1);
			vs->venum = varset;
			vs->variants = calloc(sizeof *vs->variants, nvars);
			for (i = 0; i < nvars; i++)
				vs->variants[i] = 1;
			ADDARRAY(vi->varsets, vs);
		}
		while (1) {
			while (*vars == ' ') vars++;
			if (*vars == 0)
				break;
			char *split = vars;
			while (*split != ':' && *split != '-' && *split != ' '  && *split != 0)
				split++;
			char *first = 0;
			if (split != vars)
				first = strndup(vars, split-vars);
			if (*split == ' ' || *split == 0) {
				int idx = findvidx(db, varset, first);
				if (idx != -1)
					vs->variants[idx] |= 2;
				vars = split;
			} else {
				char *end = split+1;
				while (*end != ' '  && *end != 0)
					end++;
				char *second = 0;
				if (end != split+1)
					second = strndup(split+1, end-split-1);
				int idx1 = 0;
				if (first)
					idx1 = findvidx(db, varset, first);
				int idx2 = nvars;
				if (second) {
					idx2 = findvidx(db, varset, second);
					if (*split == '-')
						idx2++;
				}
				if (idx1 != -1 && idx2 != -1)
					for (i = idx1; i < idx2; i++)
						vs->variants[i] |= 2;
				vars = end;
				free(second);
			}
			free(first);
		}
		vi->dead = 1;
		for (i = 0; i < nvars; i++) {
			vs->variants[i] = (vs->variants[i] == 3);
			if (vs->variants[i])
				vi->dead = 0;
		}
	}
	if (vi->dead)
		return;
	if (vi->prefenum) {
		struct rnnvarset *vs = 0;
		for (i = 0; i < vi->varsetsnum; i++)
			if (vi->varsets[i]->venum == vi->prefenum) {
				vs = vi->varsets[i];
				break;
			}
		if (vs) {
			for (i = 0; i < vi->prefenum->valsnum; i++)
				if (vs->variants[i]) {
					vi->prefix = strdup(vi->prefenum->vals[i]->name);
					return;
				}
		} else {
			vi->prefix = strdup(vi->prefenum->vals[0]->name);
		}
	}
}

static void cleanupvarinfo(struct rnnvarinfo *vi) {
	free(vi->prefix);
	free(vi->prefixstr);
	free(vi->varsetstr);
	free(vi->variantsstr);

	int i;
	for (i = 0; i < vi->varsetsnum; i++)
		freevarset(vi->varsets[i]);
	free(vi->varsets);
}

static void prepvalue(struct rnndb *db, struct rnnvalue *val, char *prefix, struct rnnvarinfo *parvi) {
	val->fullname = catstr(prefix, val->name);
	prepvarinfo (db, val->fullname, &val->varinfo, parvi);
	if (val->varinfo.dead)
		return;
	if (val->varinfo.prefix) {
		char *tmp = val->fullname;
		val->fullname = catstr(val->varinfo.prefix, val->fullname);
		free(tmp);
	}
}

static void freevalue(struct rnnvalue *val) {
	cleanupvarinfo(&val->varinfo);
	free(val->fullname);
	free(val->name);
	free(val);
}

static void prepbitfield(struct rnndb *db, struct rnnbitfield *bf, char *prefix, struct rnnvarinfo *parvi);

static void preptypeinfo(struct rnndb *db, struct rnntypeinfo *ti, char *prefix, struct rnnvarinfo *vi, int width, char *file) {
	int i;
	if (ti->name) {
		struct rnnenum *en = rnn_findenum (db, ti->name);
		struct rnnbitset *bs = rnn_findbitset (db, ti->name);
		struct rnnspectype *st = rnn_findspectype (db, ti->name);
		if (en) {
			if (en->isinline) {
				ti->type = RNN_TTYPE_INLINE_ENUM;
				int j;
				for (j = 0; j < en->valsnum; j++)
					ADDARRAY(ti->vals, copyvalue(en->vals[j], file));
			} else {
				ti->type = RNN_TTYPE_ENUM;
				ti->eenum = en;
			}
		} else if (bs) {
			if (bs->isinline) {
				ti->type = RNN_TTYPE_INLINE_BITSET;
				int j;
				for (j = 0; j < bs->bitfieldsnum; j++)
					ADDARRAY(ti->bitfields, copybitfield(bs->bitfields[j], file));
			} else {
				ti->type = RNN_TTYPE_BITSET;
				ti->ebitset = bs;
			}
		} else if (st) {
			ti->type = RNN_TTYPE_SPECTYPE;
			ti->spectype = st;
		} else if (!strcmp(ti->name, "hex")) {
			ti->type = RNN_TTYPE_HEX;
		} else if (!strcmp(ti->name, "float")) {
			ti->type = RNN_TTYPE_FLOAT;
		} else if (!strcmp(ti->name, "uint")) {
			ti->type = RNN_TTYPE_UINT;
		} else if (!strcmp(ti->name, "int")) {
			ti->type = RNN_TTYPE_INT;
		} else if (!strcmp(ti->name, "boolean")) {
			ti->type = RNN_TTYPE_BOOLEAN;
		} else if (!strcmp(ti->name, "bitfield")) {
			ti->type = RNN_TTYPE_INLINE_BITSET;
		} else if (!strcmp(ti->name, "enum")) {
			ti->type = RNN_TTYPE_INLINE_ENUM;
		} else if (!strcmp(ti->name, "fixed")) {
			ti->type = RNN_TTYPE_FIXED;
		} else if (!strcmp(ti->name, "ufixed")) {
			ti->type = RNN_TTYPE_UFIXED;
		} else if (!strcmp(ti->name, "a3xx_regid")) {
			ti->type = RNN_TTYPE_A3XX_REGID;
		} else {
			ti->type = RNN_TTYPE_HEX;
			fprintf (stderr, "%s: unknown type %s\n", prefix, ti->name);
			db->estatus = 1;
		}
	} else if (ti->bitfieldsnum) {
		ti->name = strdup("bitfield");
		ti->type = RNN_TTYPE_INLINE_BITSET;
	} else if (ti->valsnum) {
		ti->name = strdup("enum");
		ti->type = RNN_TTYPE_INLINE_ENUM;
	} else if (width == 1) {
		ti->name = strdup("boolean");
		ti->type = RNN_TTYPE_BOOLEAN;
	} else {
		ti->name = strdup("hex");
		ti->type = RNN_TTYPE_HEX;
	}
	for (i = 0; i < ti->bitfieldsnum; i++)
		prepbitfield(db,  ti->bitfields[i], prefix, vi);
	for (i = 0; i < ti->valsnum; i++)
		prepvalue(db, ti->vals[i], prefix, vi);
}

static void freebitfield(struct rnnbitfield *bf);

static void cleanuptypeinfo(struct rnntypeinfo *ti) {
	int i;
	for (i = 0; i < ti->bitfieldsnum; i++)
		freebitfield(ti->bitfields[i]);
	free(ti->bitfields);

	for (i = 0; i < ti->valsnum; i++)
		freevalue(ti->vals[i]);
	free(ti->vals);

	free(ti->name);
}

static void prepbitfield(struct rnndb *db, struct rnnbitfield *bf, char *prefix, struct rnnvarinfo *parvi) {
	bf->fullname = catstr(prefix, bf->name);
	prepvarinfo (db, bf->fullname, &bf->varinfo, parvi);
	if (bf->varinfo.dead)
		return;
	if (bf->high == 63)
		bf->mask = - (1ULL<<bf->low);
	else
		bf->mask = (1ULL<<(bf->high+1)) - (1ULL<<bf->low);
	preptypeinfo(db, &bf->typeinfo, bf->fullname, &bf->varinfo, bf->high - bf->low + 1, bf->file);
	if (bf->varinfo.prefix) {
		char *tmp = bf->fullname;
		bf->fullname = catstr(bf->varinfo.prefix, bf->fullname);
		free(tmp);
	}
}

static void freebitfield(struct rnnbitfield *bf) {
	cleanupvarinfo (&bf->varinfo);
	cleanuptypeinfo(&bf->typeinfo);
	free(bf->fullname);
	free(bf->name);
	free(bf);
}

static void prepdelem(struct rnndb *db, struct rnndelem *elem, char *prefix, struct rnnvarinfo *parvi, int width) {
	if (elem->type == RNN_ETYPE_USE_GROUP) {
		int i;
		struct rnngroup *gr = 0;
		for (i = 0; i < db->groupsnum; i++)
			if (!strcmp(db->groups[i]->name, elem->name)) {
				gr = db->groups[i];
				break;
			}
		if (gr) {
			for (i = 0; i < gr->subelemsnum; i++)
				ADDARRAY(elem->subelems, copydelem(gr->subelems[i], elem->file));
		} else {
			fprintf (stderr, "group %s not found!\n", elem->name);
			db->estatus = 1;
		}
		elem->type = RNN_ETYPE_STRIPE;
		elem->length = 1;
		free(elem->name);
		elem->name = 0;
	}
	if (elem->name)
		elem->fullname = catstr(prefix, elem->name);
	prepvarinfo (db, elem->fullname?elem->fullname:prefix, &elem->varinfo, parvi);
	if (elem->varinfo.dead)
		return;
	if (elem->length != 1 && !elem->stride) {
		if (elem->type != RNN_ETYPE_REG) {
			fprintf (stderr, "%s has non-1 length, but no stride!\n", elem->fullname);
			db->estatus = 1;
		} else {
			elem->stride = elem->width/width;
		}
	}
	preptypeinfo(db, &elem->typeinfo, elem->name?elem->fullname:prefix, &elem->varinfo, elem->width, elem->file);

	int i;
	for (i = 0; i < elem->subelemsnum; i++)
		prepdelem(db,  elem->subelems[i], elem->name?elem->fullname:prefix, &elem->varinfo, width);
	if (elem->varinfo.prefix && elem->name) {
		char *tmp = elem->fullname;
		elem->fullname = catstr(elem->varinfo.prefix, elem->fullname);
		free(tmp);
	}
}

static void freedelem(struct rnndelem *elem) {
	int i;
	cleanupvarinfo (&elem->varinfo);
	cleanuptypeinfo(&elem->typeinfo);
	for (i = 0; i < elem->subelemsnum; i++)
		freedelem(elem->subelems[i]);
	free(elem->subelems);

	free(elem->fullname);
	free(elem->name);
	free(elem);
}

static void prepdomain(struct rnndb *db, struct rnndomain *dom) {
	prepvarinfo (db, dom->name, &dom->varinfo, 0);
	int i;
	for (i = 0; i < dom->subelemsnum; i++)
		prepdelem(db, dom->subelems[i], dom->bare?0:dom->name, &dom->varinfo, dom->width);
	dom->fullname = catstr(dom->varinfo.prefix, dom->name);
}

static void freedomain(struct rnndomain *dom) {
	int i;
	cleanupvarinfo (&dom->varinfo);
	for (i = 0; i < dom->subelemsnum; i++)
		freedelem(dom->subelems[i]);
	free(dom->subelems);

	free(dom->fullname);
	free(dom->name);
	free(dom);
}

static void prepenum(struct rnndb *db, struct rnnenum *en) {
	if (en->prepared)
		return;
	prepvarinfo (db, en->name, &en->varinfo, 0);
	int i;
	if (en->isinline)
		return;
	for (i = 0; i < en->valsnum; i++)
		prepvalue(db, en->vals[i], en->bare?0:en->name, &en->varinfo);
	en->fullname = catstr(en->varinfo.prefix, en->name);
	en->prepared = 1;
}

static void freeenum(struct rnnenum *en) {
	int i;
	cleanupvarinfo (&en->varinfo);

	for (i = 0; i < en->valsnum; i++)
		freevalue(en->vals[i]);
	free(en->vals);

	free(en->fullname);
	free(en->name);
	free(en);
}

static void prepbitset(struct rnndb *db, struct rnnbitset *bs) {
	prepvarinfo (db, bs->name, &bs->varinfo, 0);
	int i;
	if (bs->isinline)
		return;
	for (i = 0; i < bs->bitfieldsnum; i++)
		prepbitfield(db, bs->bitfields[i], bs->bare?0:bs->name, &bs->varinfo);
	bs->fullname = catstr(bs->varinfo.prefix, bs->name);
}

static void freebitset(struct rnnbitset *bs) {
	int i;
	cleanupvarinfo(&bs->varinfo);
	for (i = 0; i < bs->bitfieldsnum; i++)
		freebitfield(bs->bitfields[i]);
	free(bs->bitfields);
	free(bs->fullname);
	free(bs->name);
	free(bs);
}

static void prepspectype(struct rnndb *db, struct rnnspectype *st) {
	preptypeinfo(db, &st->typeinfo, st->name, 0, 32, st->file); // XXX doesn't exactly make sense...
}

static void freespectype(struct rnnspectype *st) {
	cleanuptypeinfo(&st->typeinfo);
	free(st->name);
	free(st);
}

void rnn_prepdb (struct rnndb *db) {
	int i;
	for (i = 0; i < db->enumsnum; i++)
		prepenum(db, db->enums[i]);
	for (i = 0; i < db->bitsetsnum; i++)
		prepbitset(db, db->bitsets[i]);
	for (i = 0; i < db->domainsnum; i++)
		prepdomain(db, db->domains[i]);
	for (i = 0; i < db->spectypesnum; i++)
		prepspectype(db, db->spectypes[i]);
}

struct rnnenum *rnn_findenum (struct rnndb *db, const char *name) {
	int i;
	for (i = 0; i < db->enumsnum; i++)
		if (!strcmp(db->enums[i]->name, name))
			return db->enums[i];
	return 0;
}

struct rnnbitset *rnn_findbitset (struct rnndb *db, const char *name) {
	int i;
	for (i = 0; i < db->bitsetsnum; i++)
		if (!strcmp(db->bitsets[i]->name, name))
			return db->bitsets[i];
	return 0;
}

struct rnndomain *rnn_finddomain (struct rnndb *db, const char *name) {
	int i;
	for (i = 0; i < db->domainsnum; i++)
		if (!strcmp(db->domains[i]->name, name))
			return db->domains[i];
	return 0;
}

struct rnnspectype *rnn_findspectype (struct rnndb *db, const char *name) {
	int i;
	for (i = 0; i < db->spectypesnum; i++)
		if (!strcmp(db->spectypes[i]->name, name))
			return db->spectypes[i];
	return 0;
}

static void freegroup(struct rnngroup *group) {
	int i;
	free(group->name);

	for (i = 0; i < group->subelemsnum; i++)
		freedelem(group->subelems[i]);
	free(group->subelems);
	free(group);
}

void rnn_freedb (struct rnndb *db) {
	int i;

	for (i = 0; i < db->enumsnum; i++)
		freeenum(db->enums[i]);
	free(db->enums);

	for (i = 0; i < db->bitsetsnum; i++)
		freebitset(db->bitsets[i]);
	free(db->bitsets);

	for (i = 0; i < db->domainsnum; i++)
		freedomain(db->domains[i]);
	free(db->domains);

	for (i = 0; i < db->spectypesnum; i++)
		freespectype(db->spectypes[i]);
	free(db->spectypes);

	for (i = 0; i < db->groupsnum; i++)
		freegroup(db->groups[i]);
	free(db->groups);

	cleanupcopyright(&db->copyright);

	for (i = 0; i < db->filesnum; i++)
		free(db->files[i]);
	free(db->files);

	free(db);
}
