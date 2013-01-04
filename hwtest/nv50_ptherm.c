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
static void start_sensor(struct hwtest_ctx *ctx) {
	nva_mask(ctx->cnum, 0x20010, 0x40000000, 0x0);
	usleep(20000);
}

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

/* thresholds */
struct therm_threshold {
	/* intr calibration */
	uint32_t thrs_addr;
	uint32_t thrs_mask;
	uint8_t thrs_lshift;
	
	/* intr_dir */
	uint32_t dir_addr;
	uint32_t dir_mask;
	uint8_t dir_lshift;
	uint8_t dir_inverted;
	
	/* pbus intr bits */
	uint8_t pbus_irq_bit;
};

enum therm_thresholds {
	therm_threshold_crit = 0,
	therm_threshold_low,
	therm_threshold_high,
};

static struct therm_threshold nv84_therm_thresholds[] = {
	{ 0x20010, 0x00003fff, 00, 0x20000, 0x00003, 00, 0, 16 }, /* crit */
	{ 0x2001c, 0x00003fff, 00, 0x20004, 0x00003, 00, 1, 17 }, /* threshold_low */
	{ 0x2001c, 0x3fff0000, 16, 0x20004, 0x30000, 16, 0, 18 }, /* threshold_high */
};

static int threshold_reset(struct hwtest_ctx *ctx)
{
	nva_wr32(ctx->cnum, 0x20000, 0);
	nva_wr32(ctx->cnum, 0x20004, 0);
	nva_wr32(ctx->cnum, 0x20010, 0x3fff);
	nva_wr32(ctx->cnum, 0x2001c, 0x3fff0000);
}

static int threshold_gen_intr_dir(struct hwtest_ctx *ctx, struct therm_threshold *thrs, int dir) {
	int temp, lower_thrs, higher_thrs, tmp;

	temp = nva_rd32(ctx->cnum, 0x20014) & 0xffff;
	lower_thrs = temp - 0x100;
	higher_thrs = temp + 0x100;

	if (lower_thrs < 0)
		lower_thrs = 0;

	if (dir > 0)
		tmp = lower_thrs;
	else
		tmp = higher_thrs;

	nva_mask(ctx->cnum, thrs->thrs_addr, thrs->thrs_mask, 
		 (tmp << thrs->thrs_lshift) & thrs->thrs_mask); 
	
	/* enable the IRQs */
	nva_wr32(ctx->cnum, 0x1140, 0x70000);

	/* ACK any IRQ */
	nva_wr32(ctx->cnum, 0x1100, 0xffffffff); /* ack pbus' IRQs */

	/* Generate an IRQ */
	if (dir > 0)
		tmp = higher_thrs;
	else
		tmp = lower_thrs;
	nva_mask(ctx->cnum, thrs->thrs_addr, thrs->thrs_mask, 
		 (tmp << thrs->thrs_lshift) & thrs->thrs_mask); 

	/* necessary */
	usleep(1);

	return HWTEST_RES_PASS;
}

static int threshold_check_intr_rising(struct hwtest_ctx *ctx, struct therm_threshold *thrs) {
	uint32_t intr_dir;

	/* enable the rising IRQs */
	if (thrs->dir_inverted)
		intr_dir = 1;
	else
		intr_dir = 2;
	nva_wr32(ctx->cnum, thrs->dir_addr, intr_dir << (thrs->dir_lshift));
	threshold_gen_intr_dir(ctx, thrs, 1);
	TEST_READ_MASK(0x1100, 1 << thrs->pbus_irq_bit, 1 << thrs->pbus_irq_bit,
		       "rising: unexpected pbus intr bit, expected bit %i",
		       thrs->pbus_irq_bit);
	threshold_gen_intr_dir(ctx, thrs, -1);
	TEST_READ_MASK(0x1100, 0x0, 1 << thrs->pbus_irq_bit,
		       "falling: unexpected pbus intr bit, didn't expect an IRQ%s", "");

	return HWTEST_RES_PASS;
}

static int threshold_check_intr_falling(struct hwtest_ctx *ctx, struct therm_threshold *thrs) {
	uint32_t intr_dir;

	/* enable the falling IRQs */
	if (thrs->dir_inverted)
		intr_dir = 2;
	else
		intr_dir = 1;
	nva_wr32(ctx->cnum, thrs->dir_addr, intr_dir << (thrs->dir_lshift));
	threshold_gen_intr_dir(ctx, thrs, -1);
	TEST_READ_MASK(0x1100, 1 << thrs->pbus_irq_bit, 1 << thrs->pbus_irq_bit,
		       "falling: unexpected pbus intr bit, expected bit %i",
		       thrs->pbus_irq_bit);
	threshold_gen_intr_dir(ctx, thrs, 1);
	TEST_READ_MASK(0x1100, 0x0, 1 << thrs->pbus_irq_bit,
		       "rising: unexpected pbus intr bit, didn't expect an IRQ%s", "");

	return HWTEST_RES_PASS;
}

static int threshold_check_intr_both(struct hwtest_ctx *ctx, struct therm_threshold *thrs) {
	uint32_t intr_dir;

	/* enable the IRQs on both sides */
	intr_dir = 3;
	nva_wr32(ctx->cnum, thrs->dir_addr, intr_dir << (thrs->dir_lshift));
	threshold_gen_intr_dir(ctx, thrs, 1);
	TEST_READ_MASK(0x1100, 1 << thrs->pbus_irq_bit, 1 << thrs->pbus_irq_bit,
		       "rising: unexpected pbus intr bit, expected bit %i",
		       thrs->pbus_irq_bit);
	threshold_gen_intr_dir(ctx, thrs, -1);
	TEST_READ_MASK(0x1100, 1 << thrs->pbus_irq_bit, 1 << thrs->pbus_irq_bit,
		       "falling: unexpected pbus intr bit, expected bit %i",
		       thrs->pbus_irq_bit);

	return HWTEST_RES_PASS;
}

static int test_threshold_crit_intr_rising(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x80000000);
	return threshold_check_intr_rising(ctx, &nv84_therm_thresholds[therm_threshold_crit]);
}
static int test_threshold_crit_intr_falling(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x80000000);
	return threshold_check_intr_falling(ctx, &nv84_therm_thresholds[therm_threshold_crit]);
}
static int test_threshold_crit_intr_both(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x80000000);
	return threshold_check_intr_both(ctx, &nv84_therm_thresholds[therm_threshold_crit]);
}

static int test_threshold_crit_intr_rising_disabled(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x00000000);
	if (threshold_check_intr_rising(ctx, &nv84_therm_thresholds[therm_threshold_crit]) == HWTEST_RES_FAIL)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_FAIL;
}
static int test_threshold_crit_intr_falling_disabled(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x00000000);
	if (threshold_check_intr_falling(ctx, &nv84_therm_thresholds[therm_threshold_crit]) == HWTEST_RES_FAIL)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_FAIL;
}
static int test_threshold_crit_intr_both_disabled(struct hwtest_ctx *ctx) {
	nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x00000000);
	if (threshold_check_intr_both(ctx, &nv84_therm_thresholds[therm_threshold_crit]) == HWTEST_RES_FAIL)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_FAIL;
}

static int test_threshold_low_intr_rising(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	return threshold_check_intr_rising(ctx, &nv84_therm_thresholds[therm_threshold_low]);
}
static int test_threshold_low_intr_falling(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	return threshold_check_intr_falling(ctx, &nv84_therm_thresholds[therm_threshold_low]);
}
static int test_threshold_low_intr_both(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	return threshold_check_intr_both(ctx, &nv84_therm_thresholds[therm_threshold_low]);
}

static int test_threshold_high_intr_rising(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	return threshold_check_intr_rising(ctx, &nv84_therm_thresholds[therm_threshold_high]);
}
static int test_threshold_high_intr_falling(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	return threshold_check_intr_falling(ctx, &nv84_therm_thresholds[therm_threshold_high]);
}
static int test_threshold_high_intr_both(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	return threshold_check_intr_both(ctx, &nv84_therm_thresholds[therm_threshold_high]);
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

static int nv50_temperature_thresholds_prep(struct hwtest_ctx *ctx) {
	start_sensor(ctx);
	return nv50_ptherm_prep(ctx);
}

HWTEST_DEF_GROUP(nv50_sensor_calibration,
	HWTEST_TEST(test_temperature_enable_state, 0),
)

HWTEST_DEF_GROUP(nv50_temperature_thresholds,
	HWTEST_TEST(test_threshold_crit_intr_rising, 0),
	HWTEST_TEST(test_threshold_crit_intr_falling, 0),
	HWTEST_TEST(test_threshold_crit_intr_both, 0),
	HWTEST_TEST(test_threshold_crit_intr_rising_disabled, 0),
	HWTEST_TEST(test_threshold_crit_intr_falling_disabled, 0),
	HWTEST_TEST(test_threshold_crit_intr_both_disabled, 0),
	HWTEST_TEST(test_threshold_low_intr_rising, 0),
	HWTEST_TEST(test_threshold_low_intr_falling, 0),
	HWTEST_TEST(test_threshold_low_intr_both, 0),
	HWTEST_TEST(test_threshold_high_intr_rising, 0),
	HWTEST_TEST(test_threshold_high_intr_falling, 0),
	HWTEST_TEST(test_threshold_high_intr_both, 0),
)

HWTEST_DEF_GROUP(nv50_ptherm,
	HWTEST_GROUP(nv50_sensor_calibration),
	HWTEST_GROUP(nv50_temperature_thresholds),
)
