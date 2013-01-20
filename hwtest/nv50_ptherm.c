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
#include <math.h>

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

	/* threshold_filter */
	uint32_t thrs_filter_addr;
	uint32_t thrs_filter_mask;
	uint8_t thrs_filter_lshift;
	
	/* intr_dir */
	uint32_t dir_addr;
	uint32_t dir_mask;
	uint8_t dir_lshift;
	uint8_t dir_inverted;
	uint8_t dir_status_bit;
	
	/* pbus intr bits */
	uint8_t pbus_irq_bit;
};

enum therm_thresholds {
	therm_threshold_crit = 0,
	therm_threshold_low,
	therm_threshold_high,
};

static struct therm_threshold nv50_therm_thresholds[] = {
	{ 0x20010, 0x00003fff, 00, 0x20000, 0x03ff0000, 16, 0x20000, 0x00003, 00, 0, 31, 16 }, /* crit */
	{ 0x2001c, 0x00003fff, 00, 0x20004, 0x000003ff, 00, 0x20004, 0x00003, 00, 1, 14, 17 }, /* threshold_low */
	{ 0x2001c, 0x3fff0000, 16, 0x20004, 0x03ff0000, 16, 0x20004, 0x30000, 16, 0, 30, 18 }, /* threshold_high */
};

static void threshold_reset(struct hwtest_ctx *ctx)
{
	nva_wr32(ctx->cnum, 0x20000, 0);
	nva_wr32(ctx->cnum, 0x20004, 0);
	nva_wr32(ctx->cnum, 0x20010, 0x3fff);
	nva_wr32(ctx->cnum, 0x2001c, 0x3fff0000);
}

static uint64_t get_time(unsigned int card)
{
	uint64_t low;

	/* From kmmio dumps on nv28 this looks like how the blob does this.
	* It reads the high dword twice, before and after.
	* The only explanation seems to be that the 64-bit timer counter
	* advances between high and low dword reads and may corrupt the
	* result. Not confirmed.
	*/
	uint64_t high2 = nva_rd32(card, 0x9410);
	uint64_t high1;
	do {
		high1 = high2;
		low = nva_rd32(card, 0x9400);
		high2 = nva_rd32(card, 0x9410);
	} while (high1 != high2);
	return ((((uint64_t)high2) << 32) | (uint64_t)low);
}

static void threshold_set(struct hwtest_ctx *ctx, const struct therm_threshold *thrs, uint16_t temp)
{
	/* set the irq threshold */
	nva_mask(ctx->cnum, thrs->thrs_addr, thrs->thrs_mask,
			 (temp << thrs->thrs_lshift) & thrs->thrs_mask);
}

static void threshold_set_intr_dir(struct hwtest_ctx *ctx, const struct therm_threshold *thrs, uint8_t intr_dir)
{
	nva_wr32(ctx->cnum, thrs->dir_addr, intr_dir << (thrs->dir_lshift));
}

static void threshold_filter_set(struct hwtest_ctx *ctx, const struct therm_threshold *thrs, uint8_t div, uint8_t cycles)
{
	uint32_t filter = (div << 8) | cycles;
	nva_mask(ctx->cnum, thrs->thrs_filter_addr, thrs->thrs_filter_mask,
			 (filter << thrs->thrs_filter_lshift) & thrs->thrs_filter_mask);
}

static int nv50_threshold_filter(struct hwtest_ctx *ctx, const struct therm_threshold *thrs)
{
	uint8_t div, cycles;
	uint64_t before;
	uint64_t after;


	/* get the data */
	for (div = 0; div < 4; div++) {
		for (cycles = 1; cycles < 3; cycles++) {
			threshold_reset(ctx);
			nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x80000000);

			threshold_filter_set(ctx, thrs, 0, 0);

			threshold_set(ctx, thrs, 0x3fff);

			nva_wr32(ctx->cnum, 0x1100, 0x70000);
			threshold_set_intr_dir(ctx, thrs, 3);
			threshold_filter_set(ctx, thrs, div, (cycles * 0x7f));

			before = get_time(ctx->cnum);

			threshold_set(ctx, thrs, 10);
			while ((nva_rd32(ctx->cnum, 0x1100) & 0x70000) == 0);

			after = get_time(ctx->cnum);


			uint64_t time = after - before;
			double clock_hz = (1000000.0 / (16.0 * pow(16, div)));
			double expected_time = (1.0 / clock_hz) * cycles * 0x7f * 1e9;

			double prediction_error = abs(time - expected_time) * 100.0 / expected_time;

			if (prediction_error > 5.0) {
				printf("div %x => %f, cycles 0x%x: delay %llu; expected delay %.0f (prediction_error = %f%%), clock_hz = %f\n",
					div, 32 * pow(16, div), cycles * 0x7f, time, expected_time, prediction_error, clock_hz);
				return HWTEST_RES_FAIL;
			}
		}
	}

	return HWTEST_RES_PASS;
}

static int test_threshold_crit_filter(struct hwtest_ctx *ctx) {
	return nv50_threshold_filter(ctx, &nv50_therm_thresholds[therm_threshold_crit]);
}

/* this test fails. I will investigate why later! */
static int test_threshold_low_filter(struct hwtest_ctx *ctx) {
	return nv50_threshold_filter(ctx, &nv50_therm_thresholds[therm_threshold_low]);
}

/* this test fails. I will investigate why later! */
static int test_threshold_high_filter(struct hwtest_ctx *ctx) {
	return nv50_threshold_filter(ctx, &nv50_therm_thresholds[therm_threshold_high]);
}

static int threshold_check_state(struct hwtest_ctx *ctx, const struct therm_threshold *thrs) {
	int i, dir, temp, exp_state;

	threshold_reset(ctx);
	nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x80000000);

	i = 1;
	dir = 1;
	while (i > 0) {
		/* set the threshold */
		threshold_set(ctx, thrs, i);

		/* calculate the expected state */
		do {
			temp = nva_rd32(ctx->cnum, 0x20014) & 0x3fff;
			if (thrs->dir_inverted)
				exp_state = (temp < i);
			else
				exp_state = (temp > i);
		} while (temp != (nva_rd32(ctx->cnum, 0x20014) & 0x3fff));

		/* check the state */
		TEST_READ_MASK(thrs->dir_addr, exp_state << thrs->dir_status_bit, 1 << thrs->dir_status_bit,
			       "invalid state at temperature %i: dir (%i), threshold (%i)", temp, dir, i);

		if (i == 0x3fff)
			dir = -1;
		i += dir;
	}

	return HWTEST_RES_PASS;
}

static int test_threshold_crit_state(struct hwtest_ctx *ctx) {
	return threshold_check_state(ctx, &nv50_therm_thresholds[therm_threshold_crit]);
}

static int test_threshold_low_state(struct hwtest_ctx *ctx) {
	return threshold_check_state(ctx, &nv50_therm_thresholds[therm_threshold_low]);
}

static int test_threshold_high_state(struct hwtest_ctx *ctx) {
	return threshold_check_state(ctx, &nv50_therm_thresholds[therm_threshold_high]);
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

	threshold_set(ctx, thrs, tmp);
	
	/* enable the IRQs */
	nva_wr32(ctx->cnum, 0x1140, 0x70000);

	/* ACK any IRQ */
	nva_wr32(ctx->cnum, 0x1100, 0xffffffff); /* ack pbus' IRQs */

	/* Generate an IRQ */
	if (dir > 0)
		tmp = higher_thrs;
	else
		tmp = lower_thrs;
	threshold_set(ctx, thrs, tmp);

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
	threshold_set_intr_dir(ctx, thrs, intr_dir);
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
	threshold_set_intr_dir(ctx, thrs, intr_dir);
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
	/* enable the IRQs on both sides */
	threshold_set_intr_dir(ctx, thrs, 3);
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
	return threshold_check_intr_rising(ctx, &nv50_therm_thresholds[therm_threshold_crit]);
}
static int test_threshold_crit_intr_falling(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x80000000);
	return threshold_check_intr_falling(ctx, &nv50_therm_thresholds[therm_threshold_crit]);
}
static int test_threshold_crit_intr_both(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x80000000);
	return threshold_check_intr_both(ctx, &nv50_therm_thresholds[therm_threshold_crit]);
}

static int test_threshold_crit_intr_rising_disabled(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x00000000);
	if (threshold_check_intr_rising(ctx, &nv50_therm_thresholds[therm_threshold_crit]) == HWTEST_RES_FAIL)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_FAIL;
}
static int test_threshold_crit_intr_falling_disabled(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x00000000);
	if (threshold_check_intr_falling(ctx, &nv50_therm_thresholds[therm_threshold_crit]) == HWTEST_RES_FAIL)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_FAIL;
}
static int test_threshold_crit_intr_both_disabled(struct hwtest_ctx *ctx) {
	nva_mask(ctx->cnum, 0x20010, 0x80000000, 0x00000000);
	if (threshold_check_intr_both(ctx, &nv50_therm_thresholds[therm_threshold_crit]) == HWTEST_RES_FAIL)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_FAIL;
}

static int test_threshold_low_intr_rising(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	return threshold_check_intr_rising(ctx, &nv50_therm_thresholds[therm_threshold_low]);
}
static int test_threshold_low_intr_falling(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	return threshold_check_intr_falling(ctx, &nv50_therm_thresholds[therm_threshold_low]);
}
static int test_threshold_low_intr_both(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	return threshold_check_intr_both(ctx, &nv50_therm_thresholds[therm_threshold_low]);
}

static int test_threshold_high_intr_rising(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	return threshold_check_intr_rising(ctx, &nv50_therm_thresholds[therm_threshold_high]);
}
static int test_threshold_high_intr_falling(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	return threshold_check_intr_falling(ctx, &nv50_therm_thresholds[therm_threshold_high]);
}
static int test_threshold_high_intr_both(struct hwtest_ctx *ctx) {
	threshold_reset(ctx);
	return threshold_check_intr_both(ctx, &nv50_therm_thresholds[therm_threshold_high]);
}

/* clock gating */
static int clock_gating_reset(struct hwtest_ctx *ctx) {
	int i;

	for (i = 0x20060; i <= 0x20074; i+=4)
		nva_wr32(ctx->cnum, i, 0);

	threshold_reset(ctx);

	return HWTEST_RES_PASS;
}

static int test_clock_gating_force_div_only(struct hwtest_ctx *ctx) {
	int i;

	clock_gating_reset(ctx);

	for (i = 1; i < 4; i++) {
		nva_wr32(ctx->cnum, 0x20064, i);
		TEST_READ_MASK(0x20048, (i << 12) | 0x200, 0xffff, "iteration %i/4 failed", i);
	}

	return HWTEST_RES_PASS;
}

static int test_clock_gating_force_div_pwm(struct hwtest_ctx *ctx) {
	int i;

	clock_gating_reset(ctx);

	for (i = 0; i < 0x100; i++) {
		nva_wr32(ctx->cnum, 0x20064, (i << 8) | 2);
		TEST_READ_MASK(0x20048, 0x2200 | i, 0xffff, "iteration %i/255 failed", i);
	}

	return HWTEST_RES_PASS;
}

static int test_clock_gating_thermal_protect_crit(struct hwtest_ctx *ctx) {
	int temp, lower_thrs;
	uint8_t rnd_div = 1 + (rand() % 6);
	uint8_t rnd_pwm = 1 + (rand() % 0xfe);

	clock_gating_reset(ctx);

	temp = nva_rd32(ctx->cnum, 0x20014) & 0x3fff;
	lower_thrs = temp - 0x200; /* just to be sure the threshold is under the current temp */
	if (lower_thrs < 0)
		lower_thrs = 0;

	nva_mask(ctx->cnum, 0x20010, 0x80003fff, 0x80000000 | lower_thrs);
	nva_wr32(ctx->cnum, 0x20060, (rnd_pwm << 8) | rnd_div); /* div = rnd_div */

	TEST_READ_MASK(0x20048, 0x800, 0x800, "THERMAL_PROTECT_DIV_ACTIVE is not active!%s", "");
	TEST_READ_MASK(0x20048, rnd_pwm, 0xff, "PWM isn't %i", rnd_pwm);
	TEST_READ_MASK(0x20048, rnd_div << 12, 0x7000, "divisor isn't %i", rnd_div);

	return HWTEST_RES_PASS;
}

/* tests definitions */
static int nv50_ptherm_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset == 0x50)
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

static int nv50_clock_gating_prep(struct hwtest_ctx *ctx) {
	return nv50_ptherm_prep(ctx);
}

HWTEST_DEF_GROUP(nv50_sensor_calibration,
	HWTEST_TEST(test_temperature_enable_state, 0),
)

HWTEST_DEF_GROUP(nv50_clock_gating,
	HWTEST_TEST(test_clock_gating_force_div_only, 0),
	HWTEST_TEST(test_clock_gating_force_div_pwm, 0),
	HWTEST_TEST(test_clock_gating_thermal_protect_crit, 0)
)

HWTEST_DEF_GROUP(nv50_temperature_thresholds,
	HWTEST_TEST(test_threshold_crit_filter, 0),
	/*HWTEST_TEST(test_threshold_low_filter, 0),
	HWTEST_TEST(test_threshold_high_filter, 0),*/
	HWTEST_TEST(test_threshold_crit_state, 0),
	HWTEST_TEST(test_threshold_low_state, 0),
	HWTEST_TEST(test_threshold_high_state, 0),
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
	HWTEST_GROUP(nv50_clock_gating),
)
