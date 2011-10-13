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

#ifndef ED2V_H
#define ED2V_H

#include "ed2i.h"

struct ed2v_variant {
	struct ed2i_isa *isa;
	int mode;
	uint32_t fmask[0];
};

#define ED2V_CHUNK_SIZE 32

static inline int ed2v_has_feature(struct ed2v_variant *var, int feature) {
	return (var->fmask[feature/32] >> (feature % 32)) & 1;
};

struct ed2v_variant *ed2v_new_variant_i(struct ed2i_isa *isa, int variant);
struct ed2v_variant *ed2v_new_variant(struct ed2i_isa *isa, const char *variant);
void ed2v_del_variant(struct ed2v_variant *var);
int ed2v_add_feature_i(struct ed2v_variant *var, int feature);
int ed2v_add_feature(struct ed2v_variant *var, const char *feature);
int ed2v_set_mode_i(struct ed2v_variant *var, int mode);
int ed2v_set_mode(struct ed2v_variant *var, const char *mode);

#endif
