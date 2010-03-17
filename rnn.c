#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlreader.h>
#include <stdint.h>
#include <string.h>
#include "rnn.h"

static char *catstr (char *a, char *b) {
	char *res = malloc (strlen(a) + strlen(b) + 2);
	strcpy(res, a);
	strcat(res, "_");
	strcat(res, b);
	return res;
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
	uint64_t res = 0;
	if (!sscanf(c, "%lli", &res))  {
		fprintf (stderr, "%s:%d: invalid numeric value \"%s\" in attribute \"%s\"\n", file, line, c, attr->name);
		db->estatus = 1;
	}
	return res;
}

static int trytop (struct rnndb *db, char *file, xmlNode *node);

static void parseenum(struct rnndb *db, char *file, xmlNode *node) {
	xmlAttr *attr = node->properties;
	char *name = 0;
	int isinline = 0;
	char *prefix = "name";
	int i;
	while (attr) {
		if (!strcmp(attr->name, "name")) {
			name = getattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "prefix")) {
			prefix = getattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "inline")) {
			isinline = getboolattrib(db, file, node->line, attr);
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
		if (strcmp(cur->prefix, prefix) || cur->isinline != isinline) {
			fprintf (stderr, "%s:%d: merge fail for enum %s\n", file, node->line, node->name);
			db->estatus = 1;
		}
	} else {
		cur = calloc(sizeof *cur, 1);
		cur->name = strdup(name);
		cur->prefix = strdup(prefix);
		cur->isinline = isinline;
		RNN_ADDARRAY(db->enums, cur);
	}
	xmlNode *chain = node->children;
	while (chain) {
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (trytop(db, file, chain)) {
		} else if (!strcmp(chain->name, "value")) {
			struct rnnvalue *val = calloc(sizeof *val, 1);
			attr = chain->properties;
			while (attr) {
				if (!strcmp(attr->name, "name")) {
					val->name = strdup(getattrib(db, file, chain->line, attr));
				} else if (!strcmp(attr->name, "value")) {
					val->value = getnumattrib(db, file, node->line, attr);
					val->valvalid = 1;
				} else {
					fprintf (stderr, "%s:%d: wrong attribute \"%s\" for value\n", file, chain->line, attr->name);
					db->estatus = 1;
				}
				attr = attr->next;
			}
			if (!val->name) {
				fprintf (stderr, "%s:%d: nameless value\n", file, chain->line);
				db->estatus = 1;
			} else {
				RNN_ADDARRAY(cur->vals, val);
			}
		} else {
			fprintf (stderr, "%s:%d: wrong tag in enum: <%s>\n", file, chain->line, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
}

static void parsebitset(struct rnndb *db, char *file, xmlNode *node) {
	xmlNode *chain = node->children;
	while (chain) {
		if (chain->type != XML_ELEMENT_NODE) {
		} else if (!trytop(db, file, chain)) {
			fprintf (stderr, "%s:%d: wrong tag in bitset: <%s>\n", file, chain->line, chain->name);
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
				} else if (!strcmp(attr->name, "varset")) {
					res->varsetstr = strdup(getattrib(db, file, node->line, attr));
				} else if (!strcmp(attr->name, "variants")) {
					res->variantsstr = strdup(getattrib(db, file, node->line, attr));
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
		} else if (delem = trydelem(db, file, chain)) {
			RNN_ADDARRAY(res->subelems, delem);
		} else if (!trytop(db, file, chain)) {
			fprintf (stderr, "%s:%d: wrong tag in domain: <%s>\n", file, chain->line, chain->name);
			db->estatus = 1;
		}
		chain = chain->next;
	}
			return res;

	}
	int width;
	if (!strcmp(node->name, "reg8"))
		width = 8;
	else
	if (!strcmp(node->name, "reg16"))
		width = 16;
	else
	if (!strcmp(node->name, "reg32"))
		width = 32;
	else
	if (!strcmp(node->name, "reg64"))
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
				} else {
					fprintf (stderr, "%s:%d: wrong attribute \"%s\" for value\n", file, node->line, attr->name);
					db->estatus = 1;
				}
				attr = attr->next;
			}
			if (!res->name) {
				fprintf (stderr, "%s:%d: nameless value\n", file, node->line);
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
	char *prefix = "name";
	int i;
	while (attr) {
		if (!strcmp(attr->name, "name")) {
			name = getattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "prefix")) {
			prefix = getattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "size")) {
			size = getnumattrib(db, file, node->line, attr);
		} else if (!strcmp(attr->name, "width")) {
			width = getnumattrib(db, file, node->line, attr);
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
		if (strcmp(cur->prefix, prefix) || cur->width != width || (size && cur->size && size != cur->size)) {
			fprintf (stderr, "%s:%d: merge fail for domain %s\n", file, node->line, node->name);
			db->estatus = 1;
		} else {
			if (size)
				cur->size = size;
		}
	} else {
		cur = calloc(sizeof *cur, 1);
		cur->name = strdup(name);
		cur->prefix = strdup(prefix);
		cur->width = width;
		cur->size = size;
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

static void prepdelem(struct rnndb *db, struct rnndelem *elem, char *prefix) {
	if (elem->name)
		if (prefix)
			elem->fullname = catstr(prefix, elem->name);
		else
			elem->fullname = elem->name;
	int i;
	for (i = 0; i < elem->subelemsnum; i++)
		prepdelem(db,  elem->subelems[i], elem->fullname);
}

static void prepdomain(struct rnndb *db, struct rnndomain *dom) {
	int i;
	for (i = 0; i < dom->subelemsnum; i++)
		prepdelem(db, dom->subelems[i], 0);
}

void rnn_prepdb (struct rnndb *db) {
	int i;
	for (i = 0; i < db->domainsnum; i++)
		prepdomain(db, db->domains[i]);
}
