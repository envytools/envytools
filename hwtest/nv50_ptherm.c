/*
 * Copyright (C) 2012 Martin Peres <martin.peres@labri.fr>
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

#include "hwtest.h"
#include "nva.h"
#include "nvhw.h"
#include "util.h"
#include <stdio.h>
#include <unistd.h>

/* calibration */
static int test_temperature_enable_state(struct hwtest_ctx *ctx) {
	uint32_t r010 = nva_mask(ctx->cnum, 0x20010, 0x40000000, 0x40000000);
	uint32_t temp_disabled, temp_enabled;

	temp_disabled = nva_rd32(ctx->cnum, 0x20014) & 0x3fff;
	nva_wr32(ctx->cnum, 0x20010, 0);
	usleep(20000);
	temp_enabled = nva_rd32(ctx->cnum, 0x20014) & 0x3fff;

	nva_wr32(ctx->cnum, 0x20010, r010);

	if (temp_disabled == 0 && temp_enabled)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_FAIL;
}


/* tests definitions */
static int nv50_ptherm_prep(struct hwtest_ctx *ctx) {
	if (ctx->card_type == 0x50)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_NA;
}

static int nv50_sensor_calibration_prep(struct hwtest_ctx *ctx) {
	return nv50_ptherm_prep(ctx);
}

HWTEST_DEF_GROUP(nv50_sensor_calibration,
	HWTEST_TEST(test_temperature_enable_state, 0),
)

HWTEST_DEF_GROUP(nv50_ptherm,
	HWTEST_GROUP(nv50_sensor_calibration),
)
