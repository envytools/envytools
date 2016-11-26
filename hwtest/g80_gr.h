/*
 * Copyright (C) 2016 Marcin Kościelnicki <koriakin@0x04.net>
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

#ifndef G80_GR_H
#define G80_GR_H

#include "hwtest.h"
#include "old.h"

int g80_gr_intr(struct hwtest_ctx *ctx);
int g80_gr_mthd(struct hwtest_ctx *ctx, uint32_t subc, uint32_t mthd, uint32_t data);
int g80_gr_idle(struct hwtest_ctx *ctx);
int g80_gr_prep(struct hwtest_ctx *ctx);
int g80_gr_compute_prep(struct hwtest_ctx *ctx);

extern const struct hwtest_group g80_int_group;
extern const struct hwtest_group g80_fp_group;
extern const struct hwtest_group g80_sfu_group;
extern const struct hwtest_group g80_fp64_group;
extern const struct hwtest_group g80_atom32_group;
extern const struct hwtest_group g80_atom64_group;

#endif
