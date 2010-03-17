#ifndef RNN_H
#define RNN_H

#include <stdint.h>

struct rnndb {
	struct rnnenum **enums;
	int enumsnum;
	int enumsmax;
	char **files;
	int filesnum;
	int filesmax;
	int estatus;
};

struct rnnenum {
	char *name;
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
};

void rnn_init();
struct rnndb *rnn_newdb();
void rnn_parsefile (struct rnndb *db, char *file);
void rnn_builddb (struct rnndb *db);

#endif
