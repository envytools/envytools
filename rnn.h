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
	char *prefix;
	struct rnnvalue **vals;
	int valsnum;
	int valsmax;
};

struct rnnvalue {
	char *name;
	int valvalid;
	uint64_t value;
	char *fullname;
};

struct rnnbitset {
	char *name;
	int bare;
	int isinline;
	char *prefix;
	struct rnnbitfield **bitfields;
	int bitfieldsnum;
	int bitfieldsmax;
};

struct rnnbitfield {
	char *name;
	int low, high;
	int shr;
	char *fullname;
};

struct rnndomain {
	char *name;
	int bare;
	int width;
	uint64_t size;
	int sizevalid;
	char *prefix;
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
	char *varsetstr;
	char *variantsstr;
	char *fullname;
};

void rnn_init();
struct rnndb *rnn_newdb();
void rnn_parsefile (struct rnndb *db, char *file);
void rnn_prepdb (struct rnndb *db);

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
