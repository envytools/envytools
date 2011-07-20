#ifndef RNN_H
#define RNN_H

#include <stdint.h>
#include <stdlib.h>

struct rnnauthor {
	char* name;
	char* email;
	char* contributions;
	char* license;
	char** nicknames;
	int nicknamesnum;
	int nicknamesmax;
};

struct rnncopyright {
	unsigned firstyear;
	char* license;
	struct rnnauthor **authors;
	int authorsnum;
	int authorsmax;
};

struct rnndb {
	struct rnncopyright copyright;
	struct rnnenum **enums;
	int enumsnum;
	int enumsmax;
	struct rnnbitset **bitsets;
	int bitsetsnum;
	int bitsetsmax;
	struct rnndomain **domains;
	int domainsnum;
	int domainsmax;
	struct rnngroup **groups;
	int groupsnum;
	int groupsmax;
	struct rnnspectype **spectypes;
	int spectypesnum;
	int spectypesmax;
	char **files;
	int filesnum;
	int filesmax;
	int estatus;
};

struct rnnvarset {
	struct rnnenum *venum;
	int *variants;
};

struct rnnvarinfo {
	char *prefixstr;
	char *varsetstr;
	char *variantsstr;
	int dead;
	struct rnnenum *prefenum;
	char *prefix;
	struct rnnvarset **varsets;
	int varsetsnum;
	int varsetsmax;
};

struct rnnenum {
	char *name;
	int bare;
	int isinline;
	struct rnnvarinfo varinfo;
	struct rnnvalue **vals;
	int valsnum;
	int valsmax;
	char *fullname;
	int prepared;
	char *file;
};

struct rnnvalue {
	char *name;
	int valvalid;
	uint64_t value;
	struct rnnvarinfo varinfo;
	char *fullname;
	char *file;
};

struct rnntypeinfo {
	char *name;
	enum rnnttype {
		RNN_TTYPE_INLINE_ENUM,
		RNN_TTYPE_INLINE_BITSET,
		RNN_TTYPE_ENUM,
		RNN_TTYPE_BITSET,
		RNN_TTYPE_SPECTYPE,
		RNN_TTYPE_HEX,
		RNN_TTYPE_INT,
		RNN_TTYPE_UINT,
		RNN_TTYPE_FLOAT,
		RNN_TTYPE_BOOLEAN,
	} type;
	struct rnnenum *eenum;
	struct rnnbitset *ebitset;
	struct rnnspectype *spectype;
	struct rnnbitfield **bitfields;
	int bitfieldsnum;
	int bitfieldsmax;
	struct rnnvalue **vals;
	int valsnum;
	int valsmax;
	int shr;
	uint64_t min, max, align;
	int minvalid, maxvalid, alignvalid;
};

struct rnnbitset {
	char *name;
	int bare;
	int isinline;
	struct rnnvarinfo varinfo;
	struct rnnbitfield **bitfields;
	int bitfieldsnum;
	int bitfieldsmax;
	char *fullname;
	char *file;
};

struct rnnbitfield {
	char *name;
	int low, high;
	uint64_t mask;
	struct rnnvarinfo varinfo;
	struct rnntypeinfo typeinfo;
	char *fullname;
	char *file;
};

struct rnndomain {
	char *name;
	int bare;
	int width;
	uint64_t size;
	int sizevalid;
	struct rnnvarinfo varinfo;
	struct rnndelem **subelems;
	int subelemsnum;
	int subelemsmax;
	char *fullname;
	char *file;
};

struct rnngroup {
	char *name;
	struct rnndelem **subelems;
	int subelemsnum;
	int subelemsmax;
};

struct rnndelem {
	enum rnnetype {
		RNN_ETYPE_REG,
		RNN_ETYPE_ARRAY,
		RNN_ETYPE_STRIPE,
		RNN_ETYPE_USE_GROUP,
	} type;
	char *name;
	int width;
	enum rnnaccess {
		RNN_ACCESS_R,
		RNN_ACCESS_W,
		RNN_ACCESS_RW,
	} access;
	uint64_t offset;
	uint64_t length;
	uint64_t stride;
	struct rnndelem **subelems;
	int subelemsnum;
	int subelemsmax;
	struct rnnvarinfo varinfo;
	struct rnntypeinfo typeinfo;
	char *fullname;
	char *file;
};

struct rnnspectype {
	char *name;
	struct rnntypeinfo typeinfo;
	char *file;
};

void rnn_init();
struct rnndb *rnn_newdb();
void rnn_parsefile (struct rnndb *db, char *file);
void rnn_prepdb (struct rnndb *db);
struct rnnenum *rnn_findenum (struct rnndb *db, const char *name);
struct rnnbitset *rnn_findbitset (struct rnndb *db, const char *name);
struct rnndomain *rnn_finddomain (struct rnndb *db, const char *name);
struct rnnspectype *rnn_findspectype (struct rnndb *db, const char *name);

#endif
