#include "rnn.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

uint64_t *strides = 0;
int stridesnum = 0;
int stridesmax = 0;

void printvalue (struct rnnvalue *val, int shift) {
	if (val->valvalid)
		printf ("#define %s\t%#"PRIx64"\n", val->fullname, val->value<<shift);
}

void printbitfield (struct rnnbitfield *bf) {
	printf ("#define %s\t%#"PRIx64"\n", bf->fullname, bf->mask);
	printf ("#define %s__SHIFT\t%d\n", bf->fullname, bf->low);
	if (bf->shr)
		printf ("#define %s__SHR\t%d\n", bf->fullname, bf->shr);
	int i;
	for (i = 0; i < bf->valsnum; i++)
		printvalue(bf->vals[i], bf->low);
}

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
			printf (")\t(%#"PRIx64"", offset + elem->offset);
			for (i = 0; i < stridesnum; i++)
				printf (" + %#" PRIx64 "*(i%d)", strides[i], i);
			printf (")\n");
		} else
			printf ("\t%#"PRIx64"\n", offset + elem->offset);
		if (elem->stride)
			printf ("#define %s__ESIZE\t%#"PRIx64"\n", elem->fullname, elem->stride);
		if (elem->length != 1)
			printf ("#define %s__LEN\t%#"PRIx64"\n", elem->fullname, elem->length);
		if (elem->shr)
			printf ("#define %s__SHR\t%d\n", elem->fullname, elem->shr);
		int i;
		for (i = 0; i < elem->bitfieldsnum; i++)
			printbitfield(elem->bitfields[i]);
		for (i = 0; i < elem->valsnum; i++)
			printvalue(elem->vals[i], 0);
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
			printvalue (db->enums[i]->vals[j], 0);
		printf ("\n");
	}
	for (i = 0; i < db->bitsetsnum; i++) {
		if (db->bitsets[i]->isinline)
			continue;
		printf ("/* bitset %s */\n", db->bitsets[i]->name);
		int j;
		for (j = 0; j < db->bitsets[i]->bitfieldsnum; j++)
			printbitfield (db->bitsets[i]->bitfields[j]);
		printf ("\n");
	}
	for (i = 0; i < db->domainsnum; i++) {
		printf ("/* domain %s of width %d */\n", db->domains[i]->name, db->domains[i]->width);
		if (db->domains[i]->size) 
			printf ("#define %s__SIZE\t%#"PRIx64"\n", db->domains[i]->name, db->domains[i]->size);
		int j;
		for (j = 0; j < db->domains[i]->subelemsnum; j++) {
			printdelem(db->domains[i]->subelems[j], 0);
		}
		printf ("\n");
	}
	return db->estatus;
}
