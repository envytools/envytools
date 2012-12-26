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
	
	temp_disabled = nva_rd32(ctx->cnum, 0x20008 & 0xffff);
	nva_wr32(ctx->cnum, 0x20008, r008);
	usleep(20000);
	temp_enabled = nva_rd32(ctx->cnum, 0x20008 & 0xffff);
	
	if ((temp_disabled & 0xffff) == 0 && temp_enabled & 0xffff)
		return HWTEST_RES_FAIL;
	else
		return HWTEST_RES_PASS;
}

/* thresholds */
struct therm_threshold {
	uint32_t thrs_addr;
	uint32_t hyst_addr;
	uint8_t state_bit;
	uint8_t intr_bit;
	uint8_t intr_dir_id;
	uint8_t intr_inverted;
	uint8_t state_inverted;
	uint8_t state_set_equal;
	uint8_t state_unset_equal;
};

enum therm_thresholds {
	therm_threshold_crit = 0,
	therm_threshold_1,
	therm_threshold_2,
	therm_threshold_3,
	therm_threshold_4
};

struct therm_threshold nv50_therm_thresholds[] = {
	{ 0x20480, 0x20484, 20, 2, 0, 1, 0, 1, 0 }, /* crit */
	{ 0x204c4, 0x00000, 21, 3, 1, 0, 1, 1, 1 }, /* threshold_1 */
	{ 0x204c0, 0x00000, 22, 4, 2, 1, 0, 0, 0 }, /* threshold_2 */
	{ 0x20418, 0x00000, 23, 0, 3, 0, 1, 1, 1 }, /* threshold_3 */
	{ 0x20414, 0x00000, 24, 1, 4, 1, 0, 0, 0 }  /* threshold_4 */
};

static int threshold_check_state(struct hwtest_ctx *ctx, struct therm_threshold *thrs) {
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
	return threshold_check_state(ctx, &nv50_therm_thresholds[therm_threshold_crit]);
}

static int test_threshold_1_state(struct hwtest_ctx *ctx) {
	return threshold_check_state(ctx, &nv50_therm_thresholds[therm_threshold_1]);
}

static int test_threshold_2_state(struct hwtest_ctx *ctx) {
	return threshold_check_state(ctx, &nv50_therm_thresholds[therm_threshold_2]);
}

static int test_threshold_3_state(struct hwtest_ctx *ctx) {
	return threshold_check_state(ctx, &nv50_therm_thresholds[therm_threshold_3]);
}

static int test_threshold_4_state(struct hwtest_ctx *ctx) {
	return threshold_check_state(ctx, &nv50_therm_thresholds[therm_threshold_4]);
}

static int threshold_gen_intr_dir(struct hwtest_ctx *ctx, struct therm_threshold *thrs, int dir) {
	int temp = nva_rd32(ctx->cnum, 0x20400);
	int lower_thrs = temp - 20;
	int higher_thrs = temp + 20;

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
	if (ctx->card_type >= 0xa3)
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
	TEST_READ(0x20100, 0,
		  "falling: unexpected ptherm IRQ! intr_dir: %x",
		  nva_rd32(ctx->cnum, 0x20000));
	
	return HWTEST_RES_PASS;
}

static int threshold_check_intr_falling(struct hwtest_ctx *ctx, struct therm_threshold *thrs) {
	uint32_t intr_dir;
	if (ctx->card_type >= 0xa3)
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
	TEST_READ(0x20100, 0, 
		  "rising: unexpected ptherm IRQ! intr_dir: %x",
		  nva_rd32(ctx->cnum, 0x20000));

	return HWTEST_RES_PASS;
}

static int threshold_check_intr_both(struct hwtest_ctx *ctx, struct therm_threshold *thrs) {
	uint32_t intr_dir;
	if (ctx->card_type >= 0xa3)
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
	return threshold_check_intr_rising(ctx, &nv50_therm_thresholds[therm_threshold_crit]);
}
static int test_threshold_crit_intr_falling(struct hwtest_ctx *ctx) {
	return threshold_check_intr_falling(ctx, &nv50_therm_thresholds[therm_threshold_crit]);
}
static int test_threshold_crit_intr_both(struct hwtest_ctx *ctx) {
	return threshold_check_intr_both(ctx, &nv50_therm_thresholds[therm_threshold_crit]);
}

static int test_threshold_1_intr_rising(struct hwtest_ctx *ctx) {
	return threshold_check_intr_rising(ctx, &nv50_therm_thresholds[therm_threshold_1]);
}
static int test_threshold_1_intr_falling(struct hwtest_ctx *ctx) {
	return threshold_check_intr_falling(ctx, &nv50_therm_thresholds[therm_threshold_1]);
}
static int test_threshold_1_intr_both(struct hwtest_ctx *ctx) {
	return threshold_check_intr_both(ctx, &nv50_therm_thresholds[therm_threshold_1]);
}

static int test_threshold_2_intr_rising(struct hwtest_ctx *ctx) {
	return threshold_check_intr_rising(ctx, &nv50_therm_thresholds[therm_threshold_2]);
}
static int test_threshold_2_intr_falling(struct hwtest_ctx *ctx) {
	return threshold_check_intr_falling(ctx, &nv50_therm_thresholds[therm_threshold_2]);
}
static int test_threshold_2_intr_both(struct hwtest_ctx *ctx) {
	return threshold_check_intr_both(ctx, &nv50_therm_thresholds[therm_threshold_2]);
}

static int test_threshold_3_intr_rising(struct hwtest_ctx *ctx) {
	return threshold_check_intr_rising(ctx, &nv50_therm_thresholds[therm_threshold_3]);
}
static int test_threshold_3_intr_falling(struct hwtest_ctx *ctx) {
	return threshold_check_intr_falling(ctx, &nv50_therm_thresholds[therm_threshold_3]);
}
static int test_threshold_3_intr_both(struct hwtest_ctx *ctx) {
	return threshold_check_intr_both(ctx, &nv50_therm_thresholds[therm_threshold_3]);
}

static int test_threshold_4_intr_rising(struct hwtest_ctx *ctx) {
	return threshold_check_intr_rising(ctx, &nv50_therm_thresholds[therm_threshold_4]);
}
static int test_threshold_4_intr_falling(struct hwtest_ctx *ctx) {
	return threshold_check_intr_falling(ctx, &nv50_therm_thresholds[therm_threshold_4]);
}
static int test_threshold_4_intr_both(struct hwtest_ctx *ctx) {
	return threshold_check_intr_both(ctx, &nv50_therm_thresholds[therm_threshold_4]);
}

/* tests definitions */
static int nv50_ptherm_prep(struct hwtest_ctx *ctx) {
	if (ctx->card_type >= 0x50)
		return HWTEST_RES_PASS;
	else
		return HWTEST_RES_NA;
}

static int nv50_read_temperature_prep(struct hwtest_ctx *ctx) {
	return nv50_ptherm_prep(ctx);
}

static int nv50_temperature_thresholds_prep(struct hwtest_ctx *ctx) {
	return nv50_ptherm_prep(ctx);
}

static int nv50_sensor_calibration_prep(struct hwtest_ctx *ctx) {
	return nv50_ptherm_prep(ctx);
}


HWTEST_DEF_GROUP(nv50_read_temperature,
	HWTEST_TEST(test_ADC_enabled, 0),
	HWTEST_TEST(test_read_temperature, 0),
	HWTEST_TEST(test_read_calibrated_temperature, 0),
	HWTEST_TEST(test_credible_temperature, 0),
)

HWTEST_DEF_GROUP(nv50_temperature_thresholds,
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

HWTEST_DEF_GROUP(nv50_sensor_calibration,
	HWTEST_TEST(test_temperature_enable_state, 0),
)

HWTEST_DEF_GROUP(nv50_ptherm,
	HWTEST_GROUP(nv50_read_temperature),
	HWTEST_GROUP(nv50_sensor_calibration),
	HWTEST_GROUP(nv50_temperature_thresholds),
)