/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ED2I_ISA_H
#define ED2I_ISA_H

#include "ed2s.h"
#include <stdio.h>

enum {
	ED2I_ST_FEATURE,
	ED2I_ST_VARIANT,
	ED2I_ST_MODE,
};

struct ed2i_feature {
	char **names;
	int namesnum;
	char *description;
	int *implies;
	int impliesnum;
	int *conflicts;
	int conflictsnum;
};

struct ed2i_variant {
	char **names;
	int namesnum;
	char *description;
	int *features;
	int featuresnum;
};

struct ed2i_mode {
	char **names;
	int namesnum;
	char *description;
	int *features;
	int featuresnum;
};

struct ed2i_isa {
	struct ed2i_feature *features;
	int featuresnum;
	struct ed2i_variant *variants;
	int variantsnum;
	struct ed2i_mode *modes;
	int modesnum;
	int defmode;
	struct ed2s_symtab *symtab;
};

void ed2i_del_isa(struct ed2i_isa *isa);
struct ed2i_isa *ed2i_read_isa (const char *isaname);

#endif
