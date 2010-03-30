#include "rnn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

void expand (const char* prefix, struct rnndelem *elem, uint64_t base, int width) {
	int i, j;
	if(elem->length > 1024)
		return;
	switch (elem->type) {
		case RNN_ETYPE_REG:
			for(i = 0; i < elem->length; ++i) {
					if(elem->length >= 2)
						printf("d %s %08"PRIx64" %s(%i)", prefix, base + elem->offset + i * elem->stride, elem->fullname, i);
					else
						printf("d %s %08"PRIx64" %s", prefix, base + elem->offset, elem->fullname);

				for(j = 0; j < elem->typeinfo.typesnum; ++j)
					printf("%c%s", j ? ',' : ' ', elem->typeinfo.types[j]->name);
				printf("\n");
			}
			break;
		case RNN_ETYPE_STRIPE:
		case RNN_ETYPE_ARRAY:
			for (i = 0; i < elem->length; i++)
				for (j = 0; j < elem->subelemsnum; j++)
					expand(prefix, elem->subelems[j], base + elem->offset + i * elem->stride, width);
			break;
		default:
			break;
	}
}

int main(int argc, char **argv) {
	rnn_init();
	struct rnndb *db = rnn_newdb();
	rnn_parsefile (db, argv[1]);
	rnn_prepdb (db);
	int i, j;
	for(i = 0; i < db->domainsnum; ++i)
		for (j = 0; j < db->domains[i]->subelemsnum; ++j)
			expand (db->domains[i]->name, db->domains[i]->subelems[j], 0, db->domains[i]->width);
	for(i = 0; i < db->enumsnum; ++i)
		for (j = 0; j < db->enums[i]->valsnum; ++j)
			if (db->enums[i]->vals[j]->valvalid)
				printf("e %s %"PRIx64" %s\n", db->enums[i]->name, db->enums[i]->vals[j]->value, db->enums[i]->vals[j]->name);
	return 0;
}
