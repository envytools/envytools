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

#ifndef ED2I_PRE_H
#define ED2I_PRE_H

#include "ed2i.h"
#include "ed2_misc.h"

struct ed2ip_feature {
	char **names;
	int namesnum;
	int namesmax;
	char *description;
	char **implies;
	int impliesnum;
	int impliesmax;
	char **conflicts;
	int conflictsnum;
	int conflictsmax;
	struct ed2_loc loc;
};

struct ed2ip_variant {
	char **names;
	int namesnum;
	int namesmax;
	char *description;
	char **features;
	int featuresnum;
	int featuresmax;
	struct ed2_loc loc;
};

struct ed2ip_mode {
	char **names;
	int namesnum;
	int namesmax;
	char *description;
	char **features;
	int featuresnum;
	int featuresmax;
	int isdefault;
	struct ed2_loc loc;
};

struct ed2ip_isa {
	struct ed2ip_feature **features;
	int featuresnum;
	int featuresmax;
	struct ed2ip_variant **variants;
	int variantsnum;
	int variantsmax;
	struct ed2ip_mode **modes;
	int modesnum;
	int modesmax;
	int broken;
};

void ed2ip_del_feature(struct ed2ip_feature *f);
void ed2ip_del_variant(struct ed2ip_variant *v);
void ed2ip_del_mode(struct ed2ip_mode *m);
void ed2ip_del_isa(struct ed2ip_isa *isa);

struct ed2i_isa *ed2ip_transform(struct ed2ip_isa *isa);

#endif
