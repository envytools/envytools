#ifndef RNN_H
#define RNN_H

#include <stdint.h>

struct rnndb {
	struct rnnenum **enums;
	int enumsnum;
	int enumsmax;
	struct rnnbitset **bitsets;
	int bitsetsnum;
	int bitsetsmax;
	struct rnndomain **domains;
	int domainsnum;
	int domainsmax;
	char **files;
	int filesnum;
	int filesmax;
	int estatus;
};

struct rnnenum {
	char *name;
	int bare;
	int isinline;
	char *prefixstr;
	char *varsetstr;
	char *variantsstr;
	struct rnnvalue **vals;
	int valsnum;
	int valsmax;
};

struct rnnvalue {
	char *name;
	int valvalid;
	uint64_t value;
	char *prefixstr;
	char *varsetstr;
	char *variantsstr;
	char *fullname;
};

struct rnntype {
	char *name;
	enum rnnttype {
		RNN_TTYPE_INLINE_ENUM,
		RNN_TTYPE_INLINE_BITSET,
		RNN_TTYPE_ENUM,
		RNN_TTYPE_BITSET,
		RNN_TTYPE_OTHER,
	} type;
	struct rnnenum *eenum;
	struct rnnbitset *ebitset;
};

struct rnnbitset {
	char *name;
	int bare;
	int isinline;
	char *prefixstr;
	char *varsetstr;
	char *variantsstr;
	struct rnnbitfield **bitfields;
	int bitfieldsnum;
	int bitfieldsmax;
};

struct rnnbitfield {
	char *name;
	int low, high;
	int shr;
	uint64_t mask;
	char *prefixstr;
	char *varsetstr;
	char *variantsstr;
	struct rnnvalue **vals;
	int valsnum;
	int valsmax;
	struct rnntype **types;
	int typesnum;
	int typesmax;
	char *fullname;
};

struct rnndomain {
	char *name;
	int bare;
	int width;
	uint64_t size;
	int sizevalid;
	char *prefixstr;
	char *varsetstr;
	char *variantsstr;
	struct rnndelem **subelems;
	int subelemsnum;
	int subelemsmax;
};

struct rnndelem {
	enum rnnetype {
		RNN_ETYPE_REG,
		RNN_ETYPE_ARRAY,
		RNN_ETYPE_STRIPE,
	} type;
	char *name;
	int width;
	uint64_t offset;
	uint64_t length;
	uint64_t stride;
	int shr;
	struct rnndelem **subelems;
	int subelemsnum;
	int subelemsmax;
	struct rnnbitfield **bitfields;
	int bitfieldsnum;
	int bitfieldsmax;
	struct rnnvalue **vals;
	int valsnum;
	int valsmax;
	char *prefixstr;
	char *varsetstr;
	char *variantsstr;
	struct rnntype **types;
	int typesnum;
	int typesmax;
	char *fullname;
};

void rnn_init();
struct rnndb *rnn_newdb();
void rnn_parsefile (struct rnndb *db, char *file);
void rnn_prepdb (struct rnndb *db);
struct rnnenum *rnn_findenum (struct rnndb *db, const char *name);
struct rnnbitset *rnn_findbitset (struct rnndb *db, const char *name);
struct rnndomain *rnn_finddomain (struct rnndb *db, const char *name);

#define RNN_ADDARRAY(a, e) \
	do { \
	if ((a ## num) >= (a ## max)) { \
		if (!(a ## max)) \
			(a ## max) = 16; \
		else \
			(a ## max) *= 2; \
		(a) = realloc((a), (a ## max)*sizeof(*(a))); \
	} \
	(a)[(a ## num)++] = (e); \
	} while(0)

#endif
