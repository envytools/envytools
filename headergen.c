#include "rnn.h"
#include <stdio.h>

int *strides = 0;
int stridesnum = 0;
int stridesmax = 0;

void printdelem (struct rnndelem *elem, uint64_t offset) {
	if (elem->length != 1)
		RNN_ADDARRAY(strides, elem->stride);
	if (elem->name) {
		printf ("#define %s", elem->fullname, offset + elem->offset);
		if (stridesnum) {
			int i;
			printf ("(");
			for (i = 0; i < stridesnum; i++) {
				if (i) printf(", ");
				printf ("i%d", i);
			}
		printf (")\t(%#llx", offset + elem->offset);
			for (i = 0; i < stridesnum; i++)
				printf (" + %#llx*(i%d)", strides[i], i);

		printf (")\n");
		} else
		printf ("\t%#llx\n", offset + elem->offset);
		if (elem->stride)
		printf ("#define %s__ESIZE\t%#llx\n", elem->fullname, elem->stride);
		if (elem->length != 1)
		printf ("#define %s__LEN\t%#llx\n", elem->fullname, elem->length);
		if (elem->shr)
		printf ("#define %s__SHR\t%#llx\n", elem->fullname, elem->shr);
	}
	int j;
	for (j = 0; j < elem->subelemsnum; j++) {
		printdelem(elem->subelems[j], offset + elem->offset);
	}
	if (elem->length != 1) stridesnum--;
}

int main(int argc, char **argv) {
	rnn_init();
	struct rnndb *db = rnn_newdb();
	rnn_parsefile (db, argv[1]);
	rnn_prepdb (db);
	int i;
	for (i = 0; i < db->enumsnum; i++) {
		if (db->enums[i]->isinline)
			continue;
		printf ("/* enum %s */\n", db->enums[i]->name);
		int j;
		for (j = 0; j < db->enums[i]->valsnum; j++)
			if (db->enums[i]->vals[j]->valvalid)
				printf ("#define %s\t%#llx\n", db->enums[i]->vals[j]->name, db->enums[i]->vals[j]->value);
		printf ("\n");
	}
	for (i = 0; i < db->domainsnum; i++) {
		printf ("/* domain %s of width %d */\n", db->domains[i]->name, db->domains[i]->width);
		if (db->domains[i]->size) 
			printf ("#define %s__SIZE\t%#llx\n", db->domains[i]->name, db->domains[i]->size);
		int j;
		for (j = 0; j < db->domains[i]->subelemsnum; j++) {
			printdelem(db->domains[i]->subelems[j], 0);
		}
		printf ("\n");
	}
	return db->estatus;
}
