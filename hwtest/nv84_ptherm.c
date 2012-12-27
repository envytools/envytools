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

/* common */
static void force_temperature(struct hwtest_ctx *ctx, uint8_t temp) {
	nva_wr32(ctx->cnum, 0x20008, 0x80000000 | ((temp & 0x3f) << 22) | 0x8000);
}

/* nv50_read_temperature */
static int test_ADC_enabled(struct hwtest_ctx *ctx) {

	if (nva_rd32(ctx->cnum, 0x20008) & 0x80000000)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_FAIL;
}

static int test_read_temperature(struct hwtest_ctx *ctx) {
	if (nva_rd32(ctx->cnum, 0x20008) & 0xffff)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_FAIL;
}

static int test_read_calibrated_temperature(struct hwtest_ctx *ctx) {
	if (nva_rd32(ctx->cnum, 0x20400) > 0)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_FAIL;
}

static int test_credible_temperature(struct hwtest_ctx *ctx) {
	uint32_t temp = nva_rd32(ctx->cnum, 0x20400);
	if (temp > 0 && temp < 140)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_FAIL;
}

/* calibration */
static int test_temperature_enable_state(struct hwtest_ctx *ctx) {
	uint32_t r008 = nva_mask(ctx->cnum, 0x20008, 0x80000000, 0x0);
	uint32_t temp_disabled, temp_enabled;

	temp_disabled = nva_rd32(ctx->cnum, 0x20008) & 0x3fff;
	nva_mask(ctx->cnum, 0x20008, 0x80000000, 0x80000000);
	usleep(20000);
	temp_enabled = nva_rd32(ctx->cnum, 0x20008) & 0x3fff;

	nva_wr32(ctx->cnum, 0x20008, r008);

	if (temp_disabled  == 0 && temp_enabled)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_FAIL;
}

static int test_temperature_force(struct hwtest_ctx *ctx) {
	uint32_t r008 = nva_rd32(ctx->cnum, 0x20008);
	uint8_t temp_read, temp = rand() % 0x3f;

	force_temperature(ctx, temp);
	temp_read = nva_rd32(ctx->cnum, 0x20400);
	nva_wr32(ctx->cnum, 0x20008, r008);

	if (temp_read != temp)
		return HWTEST_RES_FAIL;
	else
		return HWTEST_RES_PASS;
}

/* thresholds */
struct therm_threshold {
	/* temperature thresholds */
	uint32_t thrs_addr;
	uint32_t hyst_addr;

	/* intr */
	uint8_t intr_bit;
	uint8_t intr_dir_id;
	uint8_t intr_inverted;

	/* state */
	uint8_t state_bit;
	uint8_t state_inverted;
	uint8_t state_set_equal;
	uint8_t state_unset_equal;

	/* thermal protect */
	uint8_t tp_use_bit;
	uint8_t tp_cfg_id;
	uint32_t tp_pwm_addr;
	uint8_t tp_pwm_shift;
};

enum therm_thresholds {
	therm_threshold_crit = 0,
	therm_threshold_1,
	therm_threshold_2,
	therm_threshold_3,
	therm_threshold_4
};

static struct therm_threshold nv84_therm_thresholds[] = {
	{ 0x20480, 0x20484, 2, 0, 1, 20, 0, 1, 0,  3,  0, 0x20110, 0 }, /* crit */
	{ 0x204c4, 0x00000, 3, 1, 0, 21, 1, 1, 1, -1, -1, 0x00000, 0 }, /* threshold_1 */
	{ 0x204c0, 0x00000, 4, 2, 1, 22, 0, 0, 0,  5,  1, 0x20110, 16 }, /* threshold_2 */
	{ 0x20418, 0x00000, 0, 3, 0, 23, 1, 1, 1, -1, -1, 0x00000, 0 }, /* threshold_3 */
	{ 0x20414, 0x00000, 1, 4, 1, 24, 0, 0, 0,  4,  2, 0x20110, 8 }  /* threshold_4 */
};

static int threshold_check_state(struct hwtest_ctx *ctx, const struct therm_threshold *thrs) {
	uint32_t exp_state;
	int temp, hyst, hyst_state, prev_hyst_state;
	int i, dir;

	hyst = 0;
	if (thrs->hyst_addr > 0) {
		nva_wr32(ctx->cnum, thrs->hyst_addr, 0);
		hyst = 0;
	}

	i = 1;
	dir = 1;
	hyst_state = 0;
	prev_hyst_state = 0;
	while (i > 0) {
		/* set the threshold */
		nva_wr32(ctx->cnum, thrs->thrs_addr, i);

		/* calculate the expected state */
		temp = nva_rd32(ctx->cnum, 0x20400);
		if (hyst_state)
			if (thrs->state_unset_equal)
				exp_state = (temp >= (i - hyst));
			else
				exp_state = (temp > (i - hyst));
		else
			if (thrs->state_set_equal)
				exp_state = (temp >= i);
			else
				exp_state = (temp > i);
		hyst_state = !exp_state;
		if (thrs->state_inverted)
			exp_state = !exp_state;
		exp_state <<= thrs->state_bit;

		/* check the state in 0x20000 */
		TEST_READ_MASK(0x20000, exp_state, 1 << thrs->state_bit,
			       "invalid state at temperature %i: dir (%i), threshold (%i), hyst (%i), hyst_state (%i, prev was %i)",
			       temp, dir, i, hyst, hyst_state, prev_hyst_state);

		prev_hyst_state = hyst_state;
		if (i == 0xff)
			dir = -1;
		i += dir;
	}

	return HWTEST_RES_PASS;
}

static int test_threshold_crit_state(struct hwtest_ctx *ctx) {
	return threshold_check_state(ctx, &nv84_therm_thresholds[therm_threshold_crit]);
}

static int test_threshold_1_state(struct hwtest_ctx *ctx) {
	return threshold_check_state(ctx, &nv84_therm_thresholds[therm_threshold_1]);
}

static int test_threshold_2_state(struct hwtest_ctx *ctx) {
	return threshold_check_state(ctx, &nv84_therm_thresholds[therm_threshold_2]);
}

static int test_threshold_3_state(struct hwtest_ctx *ctx) {
	return threshold_check_state(ctx, &nv84_therm_thresholds[therm_threshold_3]);
}

static int test_threshold_4_state(struct hwtest_ctx *ctx) {
	return threshold_check_state(ctx, &nv84_therm_thresholds[therm_threshold_4]);
}

static int threshold_gen_intr_dir(struct hwtest_ctx *ctx, struct therm_threshold *thrs, int dir) {
	int temp, lower_thrs, higher_thrs;

	force_temperature(ctx, 0x10 + (rand() % 0x2f));

	temp = nva_rd32(ctx->cnum, 0x20400);
	lower_thrs = temp - 10;
	higher_thrs = temp + 10;

	if (lower_thrs < 0)
		lower_thrs = 0;

	/* clear the state */
	if (thrs->hyst_addr)
		nva_wr32(ctx->cnum, thrs->hyst_addr, 0);

	if (thrs->intr_inverted) {
		if (dir > 0)
			nva_wr32(ctx->cnum, thrs->thrs_addr, higher_thrs);
		else
			nva_wr32(ctx->cnum, thrs->thrs_addr, lower_thrs);
	} else {
		if (dir > 0)
			nva_wr32(ctx->cnum, thrs->thrs_addr, lower_thrs);
		else
			nva_wr32(ctx->cnum, thrs->thrs_addr, higher_thrs);
	}

	/* ACK any IRQ */
	nva_wr32(ctx->cnum, 0x20100, 0xffffffff); /* ack ptherm's IRQs */
	nva_wr32(ctx->cnum, 0x1100, 0xffffffff); /* ack pbus' IRQs */

	/* Generate an IRQ */
	if (thrs->intr_inverted) {
		if (dir > 0)
			nva_wr32(ctx->cnum, thrs->thrs_addr, lower_thrs);
		else
			nva_wr32(ctx->cnum, thrs->thrs_addr, higher_thrs);
	} else {
		if (dir > 0)
			nva_wr32(ctx->cnum, thrs->thrs_addr, higher_thrs);
		else
			nva_wr32(ctx->cnum, thrs->thrs_addr, lower_thrs);
	}

	return HWTEST_RES_PASS;
}

static int threshold_check_intr_rising(struct hwtest_ctx *ctx, struct therm_threshold *thrs) {
	uint32_t intr_dir;
	if (ctx->chipset >= 0xa3)
		return HWTEST_RES_NA;

	/* enable the rising IRQs */
	intr_dir = 1;
	nva_wr32(ctx->cnum, 0x20000, intr_dir << (thrs->intr_dir_id * 2));
	threshold_gen_intr_dir(ctx, thrs, 1);
	TEST_READ_MASK(0x20100, 1 << thrs->intr_bit, 1 << thrs->intr_bit,
		       "rising: unexpected ptherm intr bit, expected bit %i. intr_dir: %x",
		       thrs->intr_bit, nva_rd32(ctx->cnum, 0x20000));
	TEST_READ_MASK(0x1100, 0x10000, 0x10000,
		       "rising: unexpected pbus intr bit, expected bit 16 %s", "");
	threshold_gen_intr_dir(ctx, thrs, -1);
	TEST_READ_MASK(0x20100, 0, 1 << thrs->intr_bit,
		  "falling: unexpected ptherm IRQ! intr_dir: %x",
		  nva_rd32(ctx->cnum, 0x20000));

	return HWTEST_RES_PASS;
}

static int threshold_check_intr_falling(struct hwtest_ctx *ctx, struct therm_threshold *thrs) {
	uint32_t intr_dir;
	if (ctx->chipset >= 0xa3)
		return HWTEST_RES_NA;

	/* enable the falling IRQs */
	intr_dir = 2;
	nva_wr32(ctx->cnum, 0x20000, intr_dir << (thrs->intr_dir_id * 2));
	threshold_gen_intr_dir(ctx, thrs, -1);
	TEST_READ_MASK(0x20100, 1 << thrs->intr_bit, 1 << thrs->intr_bit,
		       "falling: unexpected ptherm intr bit, expected bit %i. intr_dir: %x",
		       thrs->intr_bit, nva_rd32(ctx->cnum, 0x20000));
	TEST_READ_MASK(0x1100, 0x10000, 0x10000,
		       "falling: unexpected pbus intr bit, expected bit 16%s", "");
	threshold_gen_intr_dir(ctx, thrs, 1);
	TEST_READ_MASK(0x20100, 0, 1 << thrs->intr_bit,
		  "rising: unexpected ptherm IRQ! intr_dir: %x",
		  nva_rd32(ctx->cnum, 0x20000));

	return HWTEST_RES_PASS;
}

static int threshold_check_intr_both(struct hwtest_ctx *ctx, struct therm_threshold *thrs) {
	uint32_t intr_dir;
	if (ctx->chipset >= 0xa3)
		return HWTEST_RES_NA;

	/* enable the IRQs on both sides */
	intr_dir = 3;
	nva_wr32(ctx->cnum, 0x20000, intr_dir << (thrs->intr_dir_id * 2));
	threshold_gen_intr_dir(ctx, thrs, 1);
	TEST_READ_MASK(0x20100, 1 << thrs->intr_bit, 1 << thrs->intr_bit,
		       "rising: unexpected ptherm intr bit, expected bit %i. intr_dir: %x",
		       thrs->intr_bit, nva_rd32(ctx->cnum, 0x20000));
	threshold_gen_intr_dir(ctx, thrs, -1);
	TEST_READ_MASK(0x20100, 1 << thrs->intr_bit, 1 << thrs->intr_bit,
		       "falling: unexpected ptherm intr bit, expected bit %i. intr_dir: %x",
		       thrs->intr_bit, nva_rd32(ctx->cnum, 0x20000));

	return HWTEST_RES_PASS;
}

static int test_threshold_crit_intr_rising(struct hwtest_ctx *ctx) {
	return threshold_check_intr_rising(ctx, &nv84_therm_thresholds[therm_threshold_crit]);
}
static int test_threshold_crit_intr_falling(struct hwtest_ctx *ctx) {
	return threshold_check_intr_falling(ctx, &nv84_therm_thresholds[therm_threshold_crit]);
}
static int test_threshold_crit_intr_both(struct hwtest_ctx *ctx) {
	return threshold_check_intr_both(ctx, &nv84_therm_thresholds[therm_threshold_crit]);
}

static int test_threshold_1_intr_rising(struct hwtest_ctx *ctx) {
	return threshold_check_intr_rising(ctx, &nv84_therm_thresholds[therm_threshold_1]);
}
static int test_threshold_1_intr_falling(struct hwtest_ctx *ctx) {
	return threshold_check_intr_falling(ctx, &nv84_therm_thresholds[therm_threshold_1]);
}
static int test_threshold_1_intr_both(struct hwtest_ctx *ctx) {
	return threshold_check_intr_both(ctx, &nv84_therm_thresholds[therm_threshold_1]);
}

static int test_threshold_2_intr_rising(struct hwtest_ctx *ctx) {
	return threshold_check_intr_rising(ctx, &nv84_therm_thresholds[therm_threshold_2]);
}
static int test_threshold_2_intr_falling(struct hwtest_ctx *ctx) {
	return threshold_check_intr_falling(ctx, &nv84_therm_thresholds[therm_threshold_2]);
}
static int test_threshold_2_intr_both(struct hwtest_ctx *ctx) {
	return threshold_check_intr_both(ctx, &nv84_therm_thresholds[therm_threshold_2]);
}

static int test_threshold_3_intr_rising(struct hwtest_ctx *ctx) {
	return threshold_check_intr_rising(ctx, &nv84_therm_thresholds[therm_threshold_3]);
}
static int test_threshold_3_intr_falling(struct hwtest_ctx *ctx) {
	return threshold_check_intr_falling(ctx, &nv84_therm_thresholds[therm_threshold_3]);
}
static int test_threshold_3_intr_both(struct hwtest_ctx *ctx) {
	return threshold_check_intr_both(ctx, &nv84_therm_thresholds[therm_threshold_3]);
}

static int test_threshold_4_intr_rising(struct hwtest_ctx *ctx) {
	return threshold_check_intr_rising(ctx, &nv84_therm_thresholds[therm_threshold_4]);
}
static int test_threshold_4_intr_falling(struct hwtest_ctx *ctx) {
	return threshold_check_intr_falling(ctx, &nv84_therm_thresholds[therm_threshold_4]);
}
static int test_threshold_4_intr_both(struct hwtest_ctx *ctx) {
	return threshold_check_intr_both(ctx, &nv84_therm_thresholds[therm_threshold_4]);
}

/* clock gating */
static int clock_gating_reset(struct hwtest_ctx *ctx) {
	int i;

	nva_wr32(ctx->cnum, 0x20004, 0);
	for (i = 0x20060; i <= 0x20074; i+=4)
		nva_wr32(ctx->cnum, i, 0);
	nva_wr32(ctx->cnum, 0x20414, 0xff);
	nva_wr32(ctx->cnum, 0x20418, 0xff);
	nva_wr32(ctx->cnum, 0x20480, 0xff);
	nva_wr32(ctx->cnum, 0x204c0, 0xff);
	nva_wr32(ctx->cnum, 0x204c4, 0xff);

	return HWTEST_RES_PASS;
}

static int test_clock_gating_force_div_only(struct hwtest_ctx *ctx) {
	int i;

	clock_gating_reset(ctx);

	for (i = 1; i < 7; i++) {
		nva_wr32(ctx->cnum, 0x20064, i);
		TEST_READ_MASK(0x20048, (i << 12) | 0x200, 0xffff, "%s", "");
	}

	return HWTEST_RES_PASS;
}

static int test_clock_gating_force_div_pwm(struct hwtest_ctx *ctx) {
	int i;

	clock_gating_reset(ctx);

	for (i = 0; i < 0x100; i++) {
		nva_wr32(ctx->cnum, 0x20064, (i << 8) | 2);
		TEST_READ_MASK(0x20048, 0x2200 | i, 0xffff, "%s", "");
	}

	return HWTEST_RES_PASS;
}

/* TODO:
 * - check power-based clock gating
 */

static int test_clock_gating_thermal_protect(struct hwtest_ctx *ctx,
					     const struct therm_threshold *thrs) {
	int temp, lower_thrs;
	uint8_t rnd_div = 1 + (rand() % 6);
	uint8_t rnd_pwm = 1 + (rand() % 0xfe);
	uint8_t eds = 28; /* effective_div_shift */

	clock_gating_reset(ctx);

	force_temperature(ctx, 0x10 + (rand() % 0x2f));
	temp = nva_rd32(ctx->cnum, 0x20400);
	lower_thrs = temp - 10;

	if (lower_thrs < 0)
		lower_thrs = 0;

	if (ctx->chipset >= 0xa3)
		eds = 24;

	TEST_READ_MASK(0x20048, 0, 0x10000, "0 - THERMAL_PROTECT_ENABLED is set without reason%s", "");
	nva_wr32(ctx->cnum, 0x20004, 1 << thrs->tp_use_bit);

	nva_wr32(ctx->cnum, thrs->thrs_addr, lower_thrs);
	nva_wr32(ctx->cnum, 0x20060, (rnd_pwm << 8) | rnd_div); /* div = rnd_div */

	if (ctx->chipset >= 0x94 && thrs->tp_pwm_addr > 0)
		nva_wr32(ctx->cnum, thrs->tp_pwm_addr, rnd_pwm << thrs->tp_pwm_shift); /* PWM = rnd_pwm */

	/* set a divisor for the threshold but override it by using alt_div */
	nva_wr32(ctx->cnum, 0x20074, (1 << 31) | (rnd_div << (thrs->tp_cfg_id * 4)));

	if (ctx->chipset < 0xa3)
		TEST_READ_MASK(0x20048, 0x10000, 0x10000,
			       "1 - THERMAL_PROTECT_ENABLED didn't get set (use = %08x)",
			       nva_rd32(ctx->cnum, 0x20004));
	TEST_READ_MASK(0x20048, 0x800, 0x800, "1 - THERMAL_PROTECT_DIV_ACTIVE is not active!%s", "");
	TEST_READ_MASK(0x20048, rnd_pwm, 0xff, "1 - PWM isn't %i", rnd_pwm);
	TEST_READ_MASK(0x20048, rnd_div << 12, 0x7000, "1 - divisor isn't %i", rnd_div);
	TEST_READ_MASK(0x20074, rnd_div << eds, 0x7 << eds, "1 - effective divisor isn't %i", rnd_div);

	/* disable the force div to use the other divisor */
	nva_mask(ctx->cnum, 0x20060, 0xff00, 0);
	nva_mask(ctx->cnum, 0x20074, 0x80000000, 0);

	if (ctx->chipset < 0xa3)
		TEST_READ_MASK(0x20048, 0x10000, 0x10000,
			       "2 - THERMAL_PROTECT_ENABLED didn't get set (use = %08x)",
			       nva_rd32(ctx->cnum, 0x20004));
	TEST_READ_MASK(0x20048, 0x800, 0x800, "2 - THERMAL_PROTECT_DIV_ACTIVE is not active!%s", "");
	TEST_READ_MASK(0x20048, rnd_pwm, 0xff, "2 - PWM isn't %i", rnd_pwm);
	TEST_READ_MASK(0x20048, rnd_div << 12, 0x7000, "2 - divisor isn't %i", rnd_div);
 	TEST_READ_MASK(0x20074, rnd_div << eds, 0x7 << eds, "2 - effective divisor isn't %i", rnd_div);

	return HWTEST_RES_PASS;
}

static int test_clock_gating_thermal_protect_crit(struct hwtest_ctx *ctx) {
	return test_clock_gating_thermal_protect(ctx, &nv84_therm_thresholds[therm_threshold_crit]);
}
static int test_clock_gating_thermal_protect_threshold_2(struct hwtest_ctx *ctx) {
	return test_clock_gating_thermal_protect(ctx, &nv84_therm_thresholds[therm_threshold_2]);
}
static int test_clock_gating_thermal_protect_threshold_4(struct hwtest_ctx *ctx) {
	return test_clock_gating_thermal_protect(ctx, &nv84_therm_thresholds[therm_threshold_4]);
}


/* tests definitions */
static int nv84_ptherm_prep(struct hwtest_ctx *ctx) {
	if (ctx->card_type >= 0x84)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_NA;
}

static int nv84_read_temperature_prep(struct hwtest_ctx *ctx) {
	return nv84_ptherm_prep(ctx);
}

static int nv84_sensor_calibration_prep(struct hwtest_ctx *ctx) {
	return nv84_ptherm_prep(ctx);
}

static int nv84_temperature_thresholds_prep(struct hwtest_ctx *ctx) {
	return nv84_ptherm_prep(ctx);
}

static int nv84_clock_gating_prep(struct hwtest_ctx *ctx) {
	return nv84_ptherm_prep(ctx);
}

HWTEST_DEF_GROUP(nv84_read_temperature,
	HWTEST_TEST(test_ADC_enabled, 0),
	HWTEST_TEST(test_read_temperature, 0),
	HWTEST_TEST(test_read_calibrated_temperature, 0),
	HWTEST_TEST(test_credible_temperature, 0),
)

HWTEST_DEF_GROUP(nv84_sensor_calibration,
	HWTEST_TEST(test_temperature_enable_state, 0),
	HWTEST_TEST(test_temperature_force, 0),
)

HWTEST_DEF_GROUP(nv84_temperature_thresholds,
	HWTEST_TEST(test_threshold_crit_state, 0),
	HWTEST_TEST(test_threshold_1_state, 0),
	HWTEST_TEST(test_threshold_2_state, 0),
	HWTEST_TEST(test_threshold_3_state, 0),
	HWTEST_TEST(test_threshold_4_state, 0),
	HWTEST_TEST(test_threshold_crit_intr_rising, 0),
	HWTEST_TEST(test_threshold_crit_intr_falling, 0),
	HWTEST_TEST(test_threshold_crit_intr_both, 0),
	HWTEST_TEST(test_threshold_1_intr_rising, 0),
	HWTEST_TEST(test_threshold_1_intr_falling, 0),
	HWTEST_TEST(test_threshold_1_intr_both, 0),
	HWTEST_TEST(test_threshold_2_intr_rising, 0),
	HWTEST_TEST(test_threshold_2_intr_falling, 0),
	HWTEST_TEST(test_threshold_2_intr_both, 0),
	HWTEST_TEST(test_threshold_3_intr_rising, 0),
	HWTEST_TEST(test_threshold_3_intr_falling, 0),
	HWTEST_TEST(test_threshold_3_intr_both, 0),
	HWTEST_TEST(test_threshold_4_intr_rising, 0),
	HWTEST_TEST(test_threshold_4_intr_falling, 0),
	HWTEST_TEST(test_threshold_4_intr_both, 0),
)
HWTEST_DEF_GROUP(nv84_clock_gating,
	HWTEST_TEST(test_clock_gating_force_div_only, 0),
	HWTEST_TEST(test_clock_gating_force_div_pwm, 0),
	HWTEST_TEST(test_clock_gating_thermal_protect_crit, 0),
	HWTEST_TEST(test_clock_gating_thermal_protect_threshold_2, 0),
	HWTEST_TEST(test_clock_gating_thermal_protect_threshold_4, 0),
)

HWTEST_DEF_GROUP(nv84_ptherm,
	HWTEST_GROUP(nv84_read_temperature),
	HWTEST_GROUP(nv84_sensor_calibration),
	HWTEST_GROUP(nv84_temperature_thresholds),
	HWTEST_GROUP(nv84_clock_gating),
)
