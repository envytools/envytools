#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlreader.h>
#include <stdint.h>
#include <string.h>
#include "rnn.h"

static char *catstr (char *a, char *b) {
	if (!a)
		return b;
	char *res = malloc (strlen(a) + strlen(b) + 2);
	strcpy(res, a);
	strcat(res, "_");
	strcat(res, b);
	return res;
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

struct rnndb *rnn_newdb() {
	struct rnndb *db = calloc(sizeof *db, 1);
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

static struct rnnvalue *parsevalue(struct rnndb *db, char *file, xmlNode *node) {
	struct rnnvalue *val = calloc(sizeof *val, 1);
	xmlAttr *attr = node->properties;
	while (attr) {
		if (!strcmp(attr->name, "name")) {
			val->name = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "value")) {
			val->value = getnumattrib(db, file, node->line, attr);
			val->valvalid = 1;
		} else if (!strcmp(attr->name, "varset")) {
			val->varsetstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "variants")) {
			val->variantsstr = strdup(getattrib(db, file, node->line, attr));
		} else {
			fprintf (stderr, "%s:%d: wrong attribute \"%s\" for value\n", file, node->line, attr->name);
			db->estatus = 1;
		}
		attr = attr->next;
	}
	xmlNode *chain = node->children;
	while (chain) {
		struct rnndelem *delem;
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (!trytop(db, file, chain)) {
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
		if (strdiff(cur->prefixstr, prefixstr) ||
				strdiff(cur->varsetstr, varsetstr) ||
				strdiff(cur->variantsstr, variantsstr) ||
				cur->isinline != isinline || cur->bare != bare) {
			fprintf (stderr, "%s:%d: merge fail for enum %s\n", file, node->line, node->name);
			db->estatus = 1;
		}
	} else {
		cur = calloc(sizeof *cur, 1);
		cur->name = strdup(name);
		cur->isinline = isinline;
		cur->bare = bare;
		cur->prefixstr = prefixstr;
		cur->varsetstr = varsetstr;
		cur->variantsstr = variantsstr;
		RNN_ADDARRAY(db->enums, cur);
	}
	xmlNode *chain = node->children;
	while (chain) {
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (trytop(db, file, chain)) {
		} else if (!strcmp(chain->name, "value")) {
			struct rnnvalue *val = parsevalue(db, file, chain);
			if (val)
				RNN_ADDARRAY(cur->vals, val);
		} else {
			fprintf (stderr, "%s:%d: wrong tag in enum: <%s>\n", file, chain->line, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
}

static struct rnnbitfield *parsebitfield(struct rnndb *db, char *file, xmlNode *node) {
	struct rnnbitfield *bf = calloc(sizeof *bf, 1);
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
		} else if (!strcmp(attr->name, "shr")) {
			bf->shr = getnumattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "type")) {
			char *str = getattrib(db, file, node->line, attr);
			while (1) {
				while (*str == ' ') str++;
				if (!*str) break;
				char *newstr = strchr (str, ' ');
				if (!newstr) newstr = str + strlen(str);
				struct rnntype *tp = calloc(sizeof *tp, 1);
				tp->name = strndup(str, newstr-str);
				RNN_ADDARRAY(bf->types,tp);
				str = newstr;
			}
		} else if (!strcmp(attr->name, "varset")) {
			bf->varsetstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "variants")) {
			bf->variantsstr = strdup(getattrib(db, file, node->line, attr));
		} else {
			fprintf (stderr, "%s:%d: wrong attribute \"%s\" for bitfield\n", file, node->line, attr->name);
			db->estatus = 1;
		}
		attr = attr->next;
	}
	xmlNode *chain = node->children;
	while (chain) {
		struct rnndelem *delem;
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (!strcmp(chain->name, "value")) {
			struct rnnvalue *val = parsevalue(db, file, chain);
			if (bf)
				RNN_ADDARRAY(bf->vals, val);
		} else if (!trytop(db, file, chain)) {
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
		if (strdiff(cur->prefixstr, prefixstr) ||
				strdiff(cur->varsetstr, varsetstr) ||
				strdiff(cur->variantsstr, variantsstr) ||
				cur->isinline != isinline || cur->bare != bare) {
			fprintf (stderr, "%s:%d: merge fail for bitset %s\n", file, node->line, node->name);
			db->estatus = 1;
		}
	} else {
		cur = calloc(sizeof *cur, 1);
		cur->name = strdup(name);
		cur->isinline = isinline;
		cur->bare = bare;
		cur->prefixstr = prefixstr;
		cur->varsetstr = varsetstr;
		cur->variantsstr = variantsstr;
		RNN_ADDARRAY(db->bitsets, cur);
	}
	xmlNode *chain = node->children;
	while (chain) {
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (trytop(db, file, chain)) {
		} else if (!strcmp(chain->name, "bitfield")) {
			struct rnnbitfield *bf = parsebitfield(db, file, chain);
			if (bf)
				RNN_ADDARRAY(cur->bitfields, bf);
		} else {
			fprintf (stderr, "%s:%d: wrong tag in enum: <%s>\n", file, chain->line, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
}

static void parsegroup(struct rnndb *db, char *file, xmlNode *node) {
	xmlNode *chain = node->children;
	while (chain) {
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (!trytop(db, file, chain)) {
			fprintf (stderr, "%s:%d: wrong tag in group: <%s>\n", file, chain->line, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
}
static struct rnndelem *trydelem(struct rnndb *db, char *file, xmlNode *node) {
	if (!strcmp(node->name, "stripe") || !strcmp(node->name, "array")) {
		struct rnndelem *res = calloc(sizeof *res, 1);
		res->type = (strcmp(node->name, "stripe")?RNN_ETYPE_ARRAY:RNN_ETYPE_STRIPE);
		res->length = 1;
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
				res->prefixstr = strdup(getattrib(db, file, node->line, attr));
			} else if (!strcmp(attr->name, "varset")) {
				res->varsetstr = strdup(getattrib(db, file, node->line, attr));
			} else if (!strcmp(attr->name, "variants")) {
				res->variantsstr = strdup(getattrib(db, file, node->line, attr));
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
			} else if (delem = trydelem(db, file, chain)) {
				RNN_ADDARRAY(res->subelems, delem);
			} else if (!trytop(db, file, chain)) {
				fprintf (stderr, "%s:%d: wrong tag in %s: <%s>\n", file, chain->line, node->name, chain->name);
				db->estatus = 1;
			}
			chain = chain->next;
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
	res->type = RNN_ETYPE_REG;
	res->width = width;
	res->length = 1;
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
			res->varsetstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "variants")) {
			res->variantsstr = strdup(getattrib(db, file, node->line, attr));
		} else if (!strcmp(attr->name, "shr")) {
			res->shr = getnumattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "type")) {
			char *str = getattrib(db, file, node->line, attr);
			while (1) {
				while (*str == ' ') str++;
				if (!*str) break;
				char *newstr = strchr (str, ' ');
				if (!newstr) newstr = str + strlen(str);
				struct rnntype *tp = calloc(sizeof *tp, 1);
				tp->name = strndup(str, newstr-str);
				RNN_ADDARRAY(res->types,tp);
				str = newstr;
			}
		} else {
			fprintf (stderr, "%s:%d: wrong attribute \"%s\" for register\n", file, node->line, attr->name);
			db->estatus = 1;
		}
		attr = attr->next;
	}
	xmlNode *chain = node->children;
	while (chain) {
		struct rnndelem *delem;
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (!strcmp(chain->name, "bitfield")) {
			struct rnnbitfield *bf = parsebitfield(db, file, chain);
			if (bf)
				RNN_ADDARRAY(res->bitfields, bf);
		} else if (!strcmp(chain->name, "value")) {
			struct rnnvalue *val = parsevalue(db, file, chain);
			if (val)
				RNN_ADDARRAY(res->vals, val);
		} else if (!trytop(db, file, chain)) {
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
		if (strdiff(cur->prefixstr, prefixstr) ||
				strdiff(cur->varsetstr, varsetstr) ||
				strdiff(cur->variantsstr, variantsstr) ||
				cur->width != width ||
				cur->bare != bare ||
				(size && cur->size && size != cur->size)) {
			fprintf (stderr, "%s:%d: merge fail for domain %s\n", file, node->line, node->name);
			db->estatus = 1;
		} else {
			if (size)
				cur->size = size;
		}
	} else {
		cur = calloc(sizeof *cur, 1);
		cur->name = strdup(name);
		cur->bare = bare;
		cur->width = width;
		cur->size = size;
		cur->prefixstr = prefixstr;
		cur->varsetstr = varsetstr;
		cur->variantsstr = variantsstr;
		RNN_ADDARRAY(db->domains, cur);
	}
	xmlNode *chain = node->children;
	while (chain) {
		struct rnndelem *delem;
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (delem = trydelem(db, file, chain)) {
			RNN_ADDARRAY(cur->subelems, delem);
		} else if (!trytop(db, file, chain)) {
			fprintf (stderr, "%s:%d: wrong tag in domain: <%s>\n", file, chain->line, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
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
	}
	return 0;
}

void rnn_parsefile (struct rnndb *db, char *file) {
	int i;
	for (i = 0; i < db->filesnum; i++)
		if (!strcmp(db->files[i], file))
			return;
	RNN_ADDARRAY(db->files, strdup(file));
	xmlDocPtr doc = xmlParseFile(file);
	if (!doc) {
		fprintf (stderr, "%s: couldn't open database file\n", file);
		db->estatus = 1;
		return;
	}
	xmlNode *root = doc->children;
	while (root) {
		if (root->type != XML_ELEMENT_NODE) {
		} else if (strcmp(root->name, "database")) {
			fprintf (stderr, "%s:%d: wrong top-level tag <%s>\n", file, root->line, root->name);
			db->estatus = 1;
		} else {
			xmlNode *chain = root->children;
			while (chain) {
				if (chain->type != XML_ELEMENT_NODE) {
				} else if (!trytop(db, file, chain)) {
					fprintf (stderr, "%s:%d: wrong tag in database: <%s>\n", file, chain->line, chain->name);
					db->estatus = 1;
				}
				chain = chain->next;
			}
		}
		root = root->next;
	}
	xmlFreeDoc(doc);
}

static struct rnnvalue *copyvalue (struct rnnvalue *val) {
	struct rnnvalue *res = calloc (sizeof *res, 1);
	res->name = val->name;
	res->valvalid = val->valvalid;
	res->value = val->value;
	res->prefixstr = val->prefixstr;
	res->varsetstr = val->varsetstr;
	res->variantsstr = val->variantsstr;
	return res;
}

static struct rnntype *copytype (struct rnntype *t) {
	struct rnntype *res = calloc (sizeof *res, 1);
	res->name = t->name;
	return res;
}

static struct rnnbitfield *copybitfield (struct rnnbitfield *bf) {
	struct rnnbitfield *res = calloc (sizeof *res, 1);
	res->name = bf->name;
	res->low = bf->low;
	res->high = bf->high;
	res->shr = bf->shr;
	res->prefixstr = bf->prefixstr;
	res->varsetstr = bf->varsetstr;
	res->variantsstr = bf->variantsstr;
	int i;
	for (i = 0; i < bf->valsnum; i++)
		RNN_ADDARRAY(res->vals, copyvalue(bf->vals[i]));
	for (i = 0; i < bf->typesnum; i++)
		RNN_ADDARRAY(res->types, copytype(bf->types[i]));
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

static void prepenum(struct rnndb *db, struct rnnenum *en);

static int findvidx (struct rnnenum *en, char *name) {
	int i;
	for (i = 0; i < en->valsnum; i++)
		if (!strcmp(en->vals[i]->name, name))
			return i;
	return -1;
}

static void prepvarinfo (struct rnndb *db, char *what, struct rnnvarinfo *vi, struct rnnvarinfo *parent, char *prefixstr, char *varsetstr, char *variantsstr) {
	if (parent)
		vi->prefenum = parent->prefenum;
	if (prefixstr)
		if (!strcmp(prefixstr, "none"))
			vi->prefenum = 0;
		else
			vi->prefenum = rnn_findenum(db, prefixstr); // XXX
	int i;
	if (parent)
		for (i = 0; i < parent->varsetsnum; i++)
			RNN_ADDARRAY(vi->varsets, copyvarset(parent->varsets[i]));
	struct rnnenum *varset = vi->prefenum;
	if (varsetstr)
		varset = rnn_findenum(db, varsetstr);
	if (variantsstr) {
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
			RNN_ADDARRAY(vi->varsets, vs);
		}
		while (1) {
			while (*variantsstr == ' ') variantsstr++;
			if (*variantsstr == 0)
				break;
			char *split = variantsstr;
			while (*split != ':' && *split != '-' && *split != ' '  && *split != 0)
				split++;
			char *first = 0;
			if (split != variantsstr)
				first = strndup(variantsstr, split-variantsstr);
			if (*split == ' ' || *split == 0) {
				int idx = findvidx(varset, first);
				if (idx != -1)
					vs->variants[idx] |= 2;
				variantsstr = split;
			} else {
				char *end = split+1;
				while (*end != ' '  && *end != 0)
					end++;
				char *second = 0;
				if (end != split+1)
					second = strndup(split+1, end-split-1);
				int idx1 = 0;
				if (first)
					idx1 = findvidx(varset, first);
				int idx2 = nvars;
				if (second) {
					idx2 = findvidx(varset, second);
					if (*split == '-')
						idx2++;
				}
				if (idx1 != -1 && idx2 != -1)
					for (i = idx1; i < idx2; i++)
						vs->variants[i] |= 2;
				variantsstr = end;
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
					vi->prefix = vi->prefenum->vals[i]->name;
					return;
				}
		} else {
			vi->prefix = vi->prefenum->vals[0]->name;
		}
	}
}

static void prepvalue(struct rnndb *db, struct rnnvalue *val, char *prefix, struct rnnvarinfo *parvi) {
	val->fullname = catstr(prefix, val->name);
	prepvarinfo (db, val->fullname, &val->varinfo, parvi, val->prefixstr, val->varsetstr, val->variantsstr);
	if (val->varinfo.dead)
		return;
	if (val->varinfo.prefix)
		val->fullname = catstr(val->varinfo.prefix, val->fullname);
}

static void prepbitfield(struct rnndb *db, struct rnnbitfield *bf, char *prefix, struct rnnvarinfo *parvi) {
	bf->fullname = catstr(prefix, bf->name);
	prepvarinfo (db, bf->fullname, &bf->varinfo, parvi, bf->prefixstr, bf->varsetstr, bf->variantsstr);
	if (bf->varinfo.dead)
		return;
	bf->mask = (1ULL<<(bf->high+1)) - (1ULL<<bf->low);
	int i;
	for (i = 0; i < bf->typesnum; i++) {
		bf->types[i]->type = RNN_TTYPE_OTHER;
		struct rnnenum *en = rnn_findenum (db, bf->types[i]->name);
		if (en) {
			if (en->isinline) {
				bf->types[i]->type = RNN_TTYPE_INLINE_ENUM;
				int j;
				for (j = 0; j < en->valsnum; j++)
					RNN_ADDARRAY(bf->vals, copyvalue(en->vals[j]));
			} else {
				bf->types[i]->type = RNN_TTYPE_ENUM;
				bf->types[i]->eenum = en;
			}
		}
	}
	for (i = 0; i < bf->valsnum; i++)
		prepvalue(db, bf->vals[i], bf->fullname, &bf->varinfo);
	if (bf->varinfo.prefix)
		bf->fullname = catstr(bf->varinfo.prefix, bf->fullname);
}

static void prepdelem(struct rnndb *db, struct rnndelem *elem, char *prefix, struct rnnvarinfo *parvi, int width) {
	if (elem->name)
		elem->fullname = catstr(prefix, elem->name);
	prepvarinfo (db, elem->fullname?elem->fullname:prefix, &elem->varinfo, parvi, elem->prefixstr, elem->varsetstr, elem->variantsstr);
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
	int i;
	for (i = 0; i < elem->typesnum; i++) {
		elem->types[i]->type = RNN_TTYPE_OTHER;
		struct rnnenum *en = rnn_findenum (db, elem->types[i]->name);
		if (en) {
			if (en->isinline) {
				elem->types[i]->type = RNN_TTYPE_INLINE_ENUM;
				int j;
				for (j = 0; j < en->valsnum; j++)
					RNN_ADDARRAY(elem->vals, copyvalue(en->vals[j]));
			} else {
				elem->types[i]->type = RNN_TTYPE_ENUM;
				elem->types[i]->eenum = en;
			}
		}
		struct rnnbitset *bs = rnn_findbitset (db, elem->types[i]->name);
		if (bs) {
			if (bs->isinline) {
				elem->types[i]->type = RNN_TTYPE_INLINE_BITSET;
				int j;
				for (j = 0; j < bs->bitfieldsnum; j++)
					RNN_ADDARRAY(elem->bitfields, copybitfield(bs->bitfields[j]));
			} else {
				elem->types[i]->type = RNN_TTYPE_BITSET;
				elem->types[i]->ebitset = bs;
			}
		}
	}
	for (i = 0; i < elem->subelemsnum; i++)
		prepdelem(db,  elem->subelems[i], elem->name?elem->fullname:prefix, &elem->varinfo, width);
	for (i = 0; i < elem->bitfieldsnum; i++)
		prepbitfield(db,  elem->bitfields[i], elem->name?elem->fullname:prefix, &elem->varinfo);
	for (i = 0; i < elem->valsnum; i++)
		prepvalue(db, elem->vals[i], elem->name?elem->fullname:prefix, &elem->varinfo);
	if (elem->varinfo.prefix && elem->name)
		elem->fullname = catstr(elem->varinfo.prefix, elem->fullname);
}

static void prepdomain(struct rnndb *db, struct rnndomain *dom) {
	prepvarinfo (db, dom->name, &dom->varinfo, 0, dom->prefixstr, dom->varsetstr, dom->variantsstr);
	int i;
	for (i = 0; i < dom->subelemsnum; i++)
		prepdelem(db, dom->subelems[i], dom->bare?0:dom->name, &dom->varinfo, dom->width);
	dom->fullname = catstr(dom->varinfo.prefix, dom->name);
}

static void prepenum(struct rnndb *db, struct rnnenum *en) {
	if (en->prepared)
		return;
	prepvarinfo (db, en->name, &en->varinfo, 0, en->prefixstr, en->varsetstr, en->variantsstr);
	int i;
	if (en->isinline)
		return;
	for (i = 0; i < en->valsnum; i++)
		prepvalue(db, en->vals[i], en->bare?0:en->name, &en->varinfo);
	en->fullname = catstr(en->varinfo.prefix, en->name);
	en->prepared = 1;
}

static void prepbitset(struct rnndb *db, struct rnnbitset *bs) {
	prepvarinfo (db, bs->name, &bs->varinfo, 0, bs->prefixstr, bs->varsetstr, bs->variantsstr);
	int i;
	if (bs->isinline)
		return;
	for (i = 0; i < bs->bitfieldsnum; i++)
		prepbitfield(db, bs->bitfields[i], bs->bare?0:bs->name, &bs->varinfo);
	bs->fullname = catstr(bs->varinfo.prefix, bs->name);
}

void rnn_prepdb (struct rnndb *db) {
	int i;
	for (i = 0; i < db->enumsnum; i++)
		prepenum(db, db->enums[i]);
	for (i = 0; i < db->bitsetsnum; i++)
		prepbitset(db, db->bitsets[i]);
	for (i = 0; i < db->domainsnum; i++)
		prepdomain(db, db->domains[i]);
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
