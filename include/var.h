/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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

#ifndef VAR_H
#define VAR_H

#include "mask.h"
#include "symtab.h"

struct vardata_feature {
	char *name;
	char *description;
	uint32_t *ifmask;
	uint32_t *cfmask;
	int *implies;
	int impliesnum;
	int impliesmax;
	int *conflicts;
	int conflictsnum;
	int conflictsmax;
};

struct vardata_variant {
	char *name;
	char *description;
	uint32_t *fmask;
	int *features;
	int featuresnum;
	int featuresmax;
	int varset;
};

struct vardata_varset {
	char *name;
	char *description;
};

struct vardata_mode {
	char *name;
	char *description;
	uint32_t *rfmask;
	int *rfeatures;
	int rfeaturesnum;
	int rfeaturesmax;
	int modeset;
};

struct vardata_modeset {
	char *name;
	char *description;
	int defmode;
};

struct vardata {
	char *name;
	struct vardata_feature *features;
	int featuresnum;
	int featuresmax;
	struct vardata_variant *variants;
	int variantsnum;
	int variantsmax;
	struct vardata_varset *varsets;
	int varsetsnum;
	int varsetsmax;
	struct vardata_mode *modes;
	int modesnum;
	int modesmax;
	struct vardata_modeset *modesets;
	int modesetsnum;
	int modesetsmax;
	struct symtab *symtab;
	int validated;
};

enum vardata_symtype {
	VARDATA_ST_FEATURE,
	VARDATA_ST_VARIANT,
	VARDATA_ST_VARSET,
	VARDATA_ST_MODE,
	VARDATA_ST_MODESET,
};

struct vardata *vardata_new(const char *name);
void vardata_del(struct vardata *data);
int vardata_add_feature(struct vardata *data, const char *name, const char *description);
int vardata_add_varset(struct vardata *data, const char *name, const char *description);
int vardata_add_variant(struct vardata *data, const char *name, const char *description, int varset);
int vardata_add_modeset(struct vardata *data, const char *name, const char *description);
int vardata_add_mode(struct vardata *data, const char *name, const char *description, int modeset);
void vardata_feature_imply(struct vardata *data, int f1, int f2);
void vardata_feature_conflict(struct vardata *data, int f1, int f2);
void vardata_variant_feature(struct vardata *data, int v, int f);
void vardata_mode_require(struct vardata *data, int m, int f);
int vardata_modeset_def(struct vardata *data, int ms, int m);
int vardata_validate(struct vardata *data);

struct varinfo {
	struct vardata *data;
	uint32_t *fmask;
	int *variants;
	int *modes;
};

struct varinfo *varinfo_new(struct vardata *data);
void varinfo_del(struct varinfo *info);
int varinfo_set_variant(struct varinfo *info, const char *variant);
int varinfo_set_feature(struct varinfo *info, const char *feature);
int varinfo_set_mode(struct varinfo *info, const char *mode);

struct varselect {
	struct vardata *data;
	uint32_t *fmask;
	uint32_t *msmask;
	uint32_t *mmask;
	uint32_t *vsmask;
	uint32_t *vmask;
};

struct varselect *varselect_new(struct vardata *data);
void varselect_del(struct varselect *select);
void varselect_need_feature(struct varselect *select, int f);
void varselect_need_mode(struct varselect *select, int m);
void varselect_need_variant(struct varselect *select, int v);

int varselect_match(struct varselect *select, struct varinfo *info);

#endif
