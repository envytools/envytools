/*
 * Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
 * Copyright (C) 2010 Luca Barbieri <luca@luca-barbieri.com>
 * Copyright (C) 2010 Marcin Slusarz <marcin.slusarz@gmail.com>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "rnn.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

uint64_t *strides = 0;
int stridesnum = 0;
int stridesmax = 0;

int startcol = 64;

struct fout {
	char *name;
	FILE *file;
	char *guard;
};

struct fout *fouts = 0;
int foutsnum = 0;
int foutsmax = 0;

void seekcol (FILE *f, int src, int dst) {
	if (dst <= src)
		fprintf (f, "\t");
	else {
		int n = dst/8 - src/8;
		if (n) {
			while (n--)
				fprintf (f, "\t");
			n = dst&7;
		} else
			n = dst-src;
		while (n--)
			fprintf (f, " ");
	}
}

FILE *findfout (char *file) {
	int i;
	for (i = 0; i < foutsnum; i++)
		if (!strcmp(fouts[i].name, file))
			break;
	if (i == foutsnum) {
		fprintf (stderr, "AIII, didn't open file %s.\n", file);
		exit(1);
	}
	return fouts[i].file;
}

void printdef (char *name, char *suf, int type, uint64_t val, char *file) {
	FILE *dst = findfout(file);
	int len;
	if (suf)
		fprintf (dst, "#define %s__%s%n", name, suf, &len);
	else
		fprintf (dst, "#define %s%n", name, &len);
	if (type == 0 && val > 0xffffffffull)
		seekcol (dst, len, startcol-8);
	else
		seekcol (dst, len, startcol);
	switch (type) {
		case 0:
			if (val > 0xffffffffull)
				fprintf (dst, "0x%016"PRIx64"ULL\n", val);
			else
				fprintf (dst, "0x%08"PRIx64"\n", val);
			break;
		case 1:
			fprintf (dst, "%"PRIu64"\n", val);
			break;
	}
}

void printvalue (struct rnnvalue *val, int shift) {
	if (val->varinfo.dead)
		return;
	if (val->valvalid)
		printdef (val->fullname, 0, 0, val->value << shift, val->file);
}

void printbitfield (struct rnnbitfield *bf, int shift);

void printtypeinfo (struct rnntypeinfo *ti, char *prefix, int shift, char *file) {
	if (ti->shr)
		printdef (prefix, "SHR", 1, ti->shr, file);
	if (ti->minvalid)
		printdef (prefix, "MIN", 0, ti->min, file);
	if (ti->maxvalid)
		printdef (prefix, "MAX", 0, ti->max, file);
	if (ti->alignvalid)
		printdef (prefix, "ALIGN", 0, ti->align, file);
	if (ti->radixvalid)
		printdef (prefix, "RADIX", 0, ti->radix, file);
	int i;
	for (i = 0; i < ti->valsnum; i++)
		printvalue(ti->vals[i], shift);
	for (i = 0; i < ti->bitfieldsnum; i++)
		printbitfield(ti->bitfields[i], shift);
}

void printbitfield (struct rnnbitfield *bf, int shift) {
	if (bf->varinfo.dead)
		return;
	if (bf->typeinfo.type == RNN_TTYPE_BOOLEAN) {
		printdef (bf->fullname, 0, 0, bf->mask << shift, bf->file);
	} else {
		printdef (bf->fullname, "MASK", 0, bf->mask << shift, bf->file);
		printdef (bf->fullname, "SHIFT", 1, bf->low + shift, bf->file);
	}
	printtypeinfo (&bf->typeinfo, bf->fullname, bf->low + shift, bf->file);
}

void printdelem (struct rnndelem *elem, uint64_t offset) {
	if (elem->varinfo.dead)
		return;
	if (elem->length != 1)
		ADDARRAY(strides, elem->stride);
	if (elem->name) {
		if (stridesnum) {
			int len, total;
			FILE *dst = findfout(elem->file);
			fprintf (dst, "#define %s(%n", elem->fullname, &total);
			int i;
			for (i = 0; i < stridesnum; i++) {
				if (i) {
					fprintf(dst, ", ");
					total += 2;
				}
				fprintf (dst, "i%d%n", i, &len);
				total += len;
			}
			fprintf (dst, ")");
			total++;
			seekcol (dst, total, startcol-1);
			fprintf (dst, "(0x%08"PRIx64"", offset + elem->offset);
			for (i = 0; i < stridesnum; i++)
				fprintf (dst, " + %#" PRIx64 "*(i%d)", strides[i], i);
			fprintf (dst, ")\n");
		} else
			printdef (elem->fullname, 0, 0, offset + elem->offset, elem->file);
		if (elem->stride)
			printdef (elem->fullname, "ESIZE", 0, elem->stride, elem->file);
		if (elem->length != 1)
			printdef (elem->fullname, "LEN", 0, elem->length, elem->file);
		printtypeinfo (&elem->typeinfo, elem->fullname, 0, elem->file);
	}
	fprintf (findfout(elem->file), "\n");
	int j;
	for (j = 0; j < elem->subelemsnum; j++) {
		printdelem(elem->subelems[j], offset + elem->offset);
	}
	if (elem->length != 1) stridesnum--;
}

void print_file_info_(FILE *dst, struct stat* sb, struct tm* tm)
{
	char timestr[64];
	strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", tm);
	fprintf(dst, "(%7Lu bytes, from %s)\n", (unsigned long long)sb->st_size, timestr);
}

void print_file_info(FILE *dst, const char* file)
{
	struct stat sb;
	struct tm tm;
	stat(file, &sb);
	gmtime_r(&sb.st_mtime, &tm);
	print_file_info_(dst, &sb, &tm);
}

void printhead(struct fout f, struct rnndb *db) {
	int i, j;
	struct stat sb;
	struct tm tm;
	stat(f.name, &sb);
	gmtime_r(&sb.st_mtime, &tm);
	fprintf (f.file, "#ifndef %s\n", f.guard);
	fprintf (f.file, "#define %s\n", f.guard);
	fprintf (f.file, "\n");
	fprintf(f.file,
		"/* Autogenerated file, DO NOT EDIT manually!\n"
		"\n"
		"This file was generated by the rules-ng-ng headergen tool in this git repository:\n"
		"http://github.com/envytools/envytools/\n"
		"git clone https://github.com/envytools/envytools.git\n"
		"\n"
		"The rules-ng-ng source files this header was generated from are:\n");
	unsigned maxlen = 0;
	for(i = 0; i < db->filesnum; ++i) {
		unsigned len = strlen(db->files[i]);
		if(len > maxlen)
			maxlen = len;
	}
	for(i = 0; i < db->filesnum; ++i) {
		unsigned len = strlen(db->files[i]);
		fprintf(f.file, "- %s%*s ", db->files[i], maxlen - len, "");
		print_file_info(f.file, db->files[i]);
	}
	fprintf(f.file,
		"\n"
		"Copyright (C) ");
	if(db->copyright.firstyear && db->copyright.firstyear < (1900 + tm.tm_year))
		fprintf(f.file, "%u-", db->copyright.firstyear);
	fprintf(f.file, "%u", 1900 + tm.tm_year);
	if(db->copyright.authorsnum) {
		fprintf(f.file, " by the following authors:");
		for(i = 0; i < db->copyright.authorsnum; ++i) {
			fprintf(f.file, "\n- ");
			if(db->copyright.authors[i]->name)
				fprintf(f.file, "%s", db->copyright.authors[i]->name);
			if(db->copyright.authors[i]->email)
				fprintf(f.file, " <%s>", db->copyright.authors[i]->email);
			if(db->copyright.authors[i]->nicknamesnum) {
				for(j = 0; j < db->copyright.authors[i]->nicknamesnum; ++j) {
					fprintf(f.file, "%s%s", (j ? ", " : " ("), db->copyright.authors[i]->nicknames[j]);
				}
				fprintf(f.file, ")");
			}
		}
	}
	fprintf(f.file, "\n");
	if(db->copyright.license)
		fprintf(f.file, "\n%s\n", db->copyright.license);
	fprintf(f.file, "*/\n\n\n");
}

int main(int argc, char **argv) {
	struct rnndb *db;
	int i, j;

	if (argc < 2) {
		fprintf(stderr, "Usage:\n\theadergen database-file\n");
		exit(1);
	}

	rnn_init();
	db = rnn_newdb();
	rnn_parsefile (db, argv[1]);
	rnn_prepdb (db);
	for(i = 0; i < db->filesnum; ++i) {
		char *dstname = malloc(strlen(db->files[i]) + 3);
		char *pretty;
		strcpy(dstname, db->files[i]);
		strcat(dstname, ".h");
		struct fout f = { db->files[i], fopen(dstname, "w") };
		if (!f.file) {
			perror(dstname);
			exit(1);
		}
		free(dstname);
		pretty = strrchr(f.name, '/');
		if (pretty)
			pretty += 1;
		else
			pretty = f.name;
		f.guard = strdup(pretty);
		for (j = 0; j < strlen(f.guard); j++)
			if (isalnum(f.guard[j]))
				f.guard[j] = toupper(f.guard[j]);
			else
				f.guard[j] = '_';
		ADDARRAY(fouts, f);
		printhead(f, db);
	}

	for (i = 0; i < db->enumsnum; i++) {
		if (db->enums[i]->isinline)
			continue;
		int j;
		for (j = 0; j < db->enums[i]->valsnum; j++)
			printvalue (db->enums[i]->vals[j], 0);
	}
	for (i = 0; i < db->bitsetsnum; i++) {
		if (db->bitsets[i]->isinline)
			continue;
		int j;
		for (j = 0; j < db->bitsets[i]->bitfieldsnum; j++)
			printbitfield (db->bitsets[i]->bitfields[j], 0);
	}
	for (i = 0; i < db->domainsnum; i++) {
		if (db->domains[i]->size)
			printdef (db->domains[i]->fullname, "SIZE", 0, db->domains[i]->size, db->domains[i]->file);
		int j;
		for (j = 0; j < db->domains[i]->subelemsnum; j++) {
			printdelem(db->domains[i]->subelems[j], 0);
		}
	}
	for(i = 0; i < foutsnum; ++i) {
		fprintf (fouts[i].file, "\n#endif /* %s */\n", fouts[i].guard);
	}
	return db->estatus;
}
