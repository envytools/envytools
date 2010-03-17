#include "rnn.h"
#include <stdio.h>

int main(int argc, char **argv) {
	rnn_init();
	struct rnndb *db = rnn_newdb();
	rnn_parsefile (db, argv[1]);
	rnn_builddb (db);
	int i;
	for (i = 0; i < db->enumsnum; i++) {
		printf ("/* enum %s */\n", db->enums[i]->name);
		int j;
		for (j = 0; j < db->enums[i]->valsnum; j++)
			if (db->enums[i]->vals[j]->valvalid)
				printf ("#define %s	0x%llx\n", db->enums[i]->vals[j]->name, db->enums[i]->vals[j]->value);
	}
	return db->estatus;
}
