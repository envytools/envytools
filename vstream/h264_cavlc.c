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

#include "h264.h"
#include <stdio.h>
#include <stdlib.h>

/* bits 0-7 totalcoeff, bits 8-11 trailingones */
static const struct vs_vlc_val coeff_token_0[] = {
	{ 0x000,  1, 1 },
	{ 0x001,  6, 0,0,0,1,0,1 },
	{ 0x101,  2, 0,1 },
	{ 0x002,  8, 0,0,0,0,0,1,1,1 },
	{ 0x102,  6, 0,0,0,1,0,0 },
	{ 0x202,  3, 0,0,1 },
	{ 0x003,  9, 0,0,0,0,0,0,1,1,1 },
	{ 0x103,  8, 0,0,0,0,0,1,1,0 },
	{ 0x203,  7, 0,0,0,0,1,0,1 },
	{ 0x303,  5, 0,0,0,1,1 },
	{ 0x004, 10, 0,0,0,0,0,0,0,1,1,1 },
	{ 0x104,  9, 0,0,0,0,0,0,1,1,0 },
	{ 0x204,  8, 0,0,0,0,0,1,0,1 },
	{ 0x304,  6, 0,0,0,0,1,1 },
	{ 0x005, 11, 0,0,0,0,0,0,0,0,1,1,1 },
	{ 0x105, 10, 0,0,0,0,0,0,0,1,1,0 },
	{ 0x205,  9, 0,0,0,0,0,0,1,0,1 },
	{ 0x305,  7, 0,0,0,0,1,0,0 },
	{ 0x006, 13, 0,0,0,0,0,0,0,0,0,1,1,1,1 },
	{ 0x106, 11, 0,0,0,0,0,0,0,0,1,1,0 },
	{ 0x206, 10, 0,0,0,0,0,0,0,1,0,1 },
	{ 0x306,  8, 0,0,0,0,0,1,0,0 },
	{ 0x007, 13, 0,0,0,0,0,0,0,0,0,1,0,1,1 },
	{ 0x107, 13, 0,0,0,0,0,0,0,0,0,1,1,1,0 },
	{ 0x207, 11, 0,0,0,0,0,0,0,0,1,0,1 },
	{ 0x307,  9, 0,0,0,0,0,0,1,0,0 },
	{ 0x008, 13, 0,0,0,0,0,0,0,0,0,1,0,0,0 },
	{ 0x108, 13, 0,0,0,0,0,0,0,0,0,1,0,1,0 },
	{ 0x208, 13, 0,0,0,0,0,0,0,0,0,1,1,0,1 },
	{ 0x308, 10, 0,0,0,0,0,0,0,1,0,0 },
	{ 0x009, 14, 0,0,0,0,0,0,0,0,0,0,1,1,1,1 },
	{ 0x109, 14, 0,0,0,0,0,0,0,0,0,0,1,1,1,0 },
	{ 0x209, 13, 0,0,0,0,0,0,0,0,0,1,0,0,1 },
	{ 0x309, 11, 0,0,0,0,0,0,0,0,1,0,0 },
	{ 0x00a, 14, 0,0,0,0,0,0,0,0,0,0,1,0,1,1 },
	{ 0x10a, 14, 0,0,0,0,0,0,0,0,0,0,1,0,1,0 },
	{ 0x20a, 14, 0,0,0,0,0,0,0,0,0,0,1,1,0,1 },
	{ 0x30a, 13, 0,0,0,0,0,0,0,0,0,1,1,0,0 },
	{ 0x00b, 15, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1 },
	{ 0x10b, 15, 0,0,0,0,0,0,0,0,0,0,0,1,1,1,0 },
	{ 0x20b, 14, 0,0,0,0,0,0,0,0,0,0,1,0,0,1 },
	{ 0x30b, 14, 0,0,0,0,0,0,0,0,0,0,1,1,0,0 },
	{ 0x00c, 15, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,1 },
	{ 0x10c, 15, 0,0,0,0,0,0,0,0,0,0,0,1,0,1,0 },
	{ 0x20c, 15, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,1 },
	{ 0x30c, 14, 0,0,0,0,0,0,0,0,0,0,1,0,0,0 },
	{ 0x00d, 16, 0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1 },
	{ 0x10d, 15, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 },
	{ 0x20d, 15, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,1 },
	{ 0x30d, 15, 0,0,0,0,0,0,0,0,0,0,0,1,1,0,0 },
	{ 0x00e, 16, 0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1 },
	{ 0x10e, 16, 0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0 },
	{ 0x20e, 16, 0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1 },
	{ 0x30e, 15, 0,0,0,0,0,0,0,0,0,0,0,1,0,0,0 },
	{ 0x00f, 16, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1 },
	{ 0x10f, 16, 0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0 },
	{ 0x20f, 16, 0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1 },
	{ 0x30f, 16, 0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0 },
	{ 0x010, 16, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0 },
	{ 0x110, 16, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0 },
	{ 0x210, 16, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1 },
	{ 0x310, 16, 0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0 },
	{ 0 },
};

static const struct vs_vlc_val coeff_token_2[] = {
	{ 0x000,  2, 1,1 },
	{ 0x001,  6, 0,0,1,0,1,1 },
	{ 0x101,  2, 1,0 },
	{ 0x002,  6, 0,0,0,1,1,1 },
	{ 0x102,  5, 0,0,1,1,1 },
	{ 0x202,  3, 0,1,1 },
	{ 0x003,  7, 0,0,0,0,1,1,1 },
	{ 0x103,  6, 0,0,1,0,1,0 },
	{ 0x203,  6, 0,0,1,0,0,1 },
	{ 0x303,  4, 0,1,0,1 },
	{ 0x004,  8, 0,0,0,0,0,1,1,1 },
	{ 0x104,  6, 0,0,0,1,1,0 },
	{ 0x204,  6, 0,0,0,1,0,1 },
	{ 0x304,  4, 0,1,0,0 },
	{ 0x005,  8, 0,0,0,0,0,1,0,0 },
	{ 0x105,  7, 0,0,0,0,1,1,0 },
	{ 0x205,  7, 0,0,0,0,1,0,1 },
	{ 0x305,  5, 0,0,1,1,0 },
	{ 0x006,  9, 0,0,0,0,0,0,1,1,1 },
	{ 0x106,  8, 0,0,0,0,0,1,1,0 },
	{ 0x206,  8, 0,0,0,0,0,1,0,1 },
	{ 0x306,  6, 0,0,1,0,0,0 },
	{ 0x007, 11, 0,0,0,0,0,0,0,1,1,1,1 },
	{ 0x107,  9, 0,0,0,0,0,0,1,1,0 },
	{ 0x207,  9, 0,0,0,0,0,0,1,0,1 },
	{ 0x307,  6, 0,0,0,1,0,0 },
	{ 0x008, 11, 0,0,0,0,0,0,0,1,0,1,1 },
	{ 0x108, 11, 0,0,0,0,0,0,0,1,1,1,0 },
	{ 0x208, 11, 0,0,0,0,0,0,0,1,1,0,1 },
	{ 0x308,  7, 0,0,0,0,1,0,0 },
	{ 0x009, 12, 0,0,0,0,0,0,0,0,1,1,1,1 },
	{ 0x109, 11, 0,0,0,0,0,0,0,1,0,1,0 },
	{ 0x209, 11, 0,0,0,0,0,0,0,1,0,0,1 },
	{ 0x309,  9, 0,0,0,0,0,0,1,0,0 },
	{ 0x00a, 12, 0,0,0,0,0,0,0,0,1,0,1,1 },
	{ 0x10a, 12, 0,0,0,0,0,0,0,0,1,1,1,0 },
	{ 0x20a, 12, 0,0,0,0,0,0,0,0,1,1,0,1 },
	{ 0x30a, 11, 0,0,0,0,0,0,0,1,1,0,0 },
	{ 0x00b, 12, 0,0,0,0,0,0,0,0,1,0,0,0 },
	{ 0x10b, 12, 0,0,0,0,0,0,0,0,1,0,1,0 },
	{ 0x20b, 12, 0,0,0,0,0,0,0,0,1,0,0,1 },
	{ 0x30b, 11, 0,0,0,0,0,0,0,1,0,0,0 },
	{ 0x00c, 13, 0,0,0,0,0,0,0,0,0,1,1,1,1 },
	{ 0x10c, 13, 0,0,0,0,0,0,0,0,0,1,1,1,0 },
	{ 0x20c, 13, 0,0,0,0,0,0,0,0,0,1,1,0,1 },
	{ 0x30c, 12, 0,0,0,0,0,0,0,0,1,1,0,0 },
	{ 0x00d, 13, 0,0,0,0,0,0,0,0,0,1,0,1,1 },
	{ 0x10d, 13, 0,0,0,0,0,0,0,0,0,1,0,1,0 },
	{ 0x20d, 13, 0,0,0,0,0,0,0,0,0,1,0,0,1 },
	{ 0x30d, 13, 0,0,0,0,0,0,0,0,0,1,1,0,0 },
	{ 0x00e, 13, 0,0,0,0,0,0,0,0,0,0,1,1,1 },
	{ 0x10e, 14, 0,0,0,0,0,0,0,0,0,0,1,0,1,1 },
	{ 0x20e, 13, 0,0,0,0,0,0,0,0,0,0,1,1,0 },
	{ 0x30e, 13, 0,0,0,0,0,0,0,0,0,1,0,0,0 },
	{ 0x00f, 14, 0,0,0,0,0,0,0,0,0,0,1,0,0,1 },
	{ 0x10f, 14, 0,0,0,0,0,0,0,0,0,0,1,0,0,0 },
	{ 0x20f, 14, 0,0,0,0,0,0,0,0,0,0,1,0,1,0 },
	{ 0x30f, 13, 0,0,0,0,0,0,0,0,0,0,0,0,1 },
	{ 0x010, 14, 0,0,0,0,0,0,0,0,0,0,0,1,1,1 },
	{ 0x110, 14, 0,0,0,0,0,0,0,0,0,0,0,1,1,0 },
	{ 0x210, 14, 0,0,0,0,0,0,0,0,0,0,0,1,0,1 },
	{ 0x310, 14, 0,0,0,0,0,0,0,0,0,0,0,1,0,0 },
	{ 0 },
};

static const struct vs_vlc_val coeff_token_4[] = {
	{ 0x000,  4, 1,1,1,1 },
	{ 0x001,  6, 0,0,1,1,1,1 },
	{ 0x101,  4, 1,1,1,0 },
	{ 0x002,  6, 0,0,1,0,1,1 },
	{ 0x102,  5, 0,1,1,1,1 },
	{ 0x202,  4, 1,1,0,1 },
	{ 0x003,  6, 0,0,1,0,0,0 },
	{ 0x103,  5, 0,1,1,0,0 },
	{ 0x203,  5, 0,1,1,1,0 },
	{ 0x303,  4, 1,1,0,0 },
	{ 0x004,  7, 0,0,0,1,1,1,1 },
	{ 0x104,  5, 0,1,0,1,0 },
	{ 0x204,  5, 0,1,0,1,1 },
	{ 0x304,  4, 1,0,1,1 },
	{ 0x005,  7, 0,0,0,1,0,1,1 },
	{ 0x105,  5, 0,1,0,0,0 },
	{ 0x205,  5, 0,1,0,0,1 },
	{ 0x305,  4, 1,0,1,0 },
	{ 0x006,  7, 0,0,0,1,0,0,1 },
	{ 0x106,  6, 0,0,1,1,1,0 },
	{ 0x206,  6, 0,0,1,1,0,1 },
	{ 0x306,  4, 1,0,0,1 },
	{ 0x007,  7, 0,0,0,1,0,0,0 },
	{ 0x107,  6, 0,0,1,0,1,0 },
	{ 0x207,  6, 0,0,1,0,0,1 },
	{ 0x307,  4, 1,0,0,0 },
	{ 0x008,  8, 0,0,0,0,1,1,1,1 },
	{ 0x108,  7, 0,0,0,1,1,1,0 },
	{ 0x208,  7, 0,0,0,1,1,0,1 },
	{ 0x308,  5, 0,1,1,0,1 },
	{ 0x009,  8, 0,0,0,0,1,0,1,1 },
	{ 0x109,  8, 0,0,0,0,1,1,1,0 },
	{ 0x209,  7, 0,0,0,1,0,1,0 },
	{ 0x309,  6, 0,0,1,1,0,0 },
	{ 0x00a,  9, 0,0,0,0,0,1,1,1,1 },
	{ 0x10a,  8, 0,0,0,0,1,0,1,0 },
	{ 0x20a,  8, 0,0,0,0,1,1,0,1 },
	{ 0x30a,  7, 0,0,0,1,1,0,0 },
	{ 0x00b,  9, 0,0,0,0,0,1,0,1,1 },
	{ 0x10b,  9, 0,0,0,0,0,1,1,1,0 },
	{ 0x20b,  8, 0,0,0,0,1,0,0,1 },
	{ 0x30b,  8, 0,0,0,0,1,1,0,0 },
	{ 0x00c,  9, 0,0,0,0,0,1,0,0,0 },
	{ 0x10c,  9, 0,0,0,0,0,1,0,1,0 },
	{ 0x20c,  9, 0,0,0,0,0,1,1,0,1 },
	{ 0x30c,  8, 0,0,0,0,1,0,0,0 },
	{ 0x00d, 10, 0,0,0,0,0,0,1,1,0,1 },
	{ 0x10d,  9, 0,0,0,0,0,0,1,1,1 },
	{ 0x20d,  9, 0,0,0,0,0,1,0,0,1 },
	{ 0x30d,  9, 0,0,0,0,0,1,1,0,0 },
	{ 0x00e, 10, 0,0,0,0,0,0,1,0,0,1 },
	{ 0x10e, 10, 0,0,0,0,0,0,1,1,0,0 },
	{ 0x20e, 10, 0,0,0,0,0,0,1,0,1,1 },
	{ 0x30e, 10, 0,0,0,0,0,0,1,0,1,0 },
	{ 0x00f, 10, 0,0,0,0,0,0,0,1,0,1 },
	{ 0x10f, 10, 0,0,0,0,0,0,1,0,0,0 },
	{ 0x20f, 10, 0,0,0,0,0,0,0,1,1,1 },
	{ 0x30f, 10, 0,0,0,0,0,0,0,1,1,0 },
	{ 0x010, 10, 0,0,0,0,0,0,0,0,0,1 },
	{ 0x110, 10, 0,0,0,0,0,0,0,1,0,0 },
	{ 0x210, 10, 0,0,0,0,0,0,0,0,1,1 },
	{ 0x310, 10, 0,0,0,0,0,0,0,0,1,0 },
	{ 0 },
};

static const struct vs_vlc_val coeff_token_8[] = {
	{ 0x000,  6, 0,0,0,0,1,1 },
	{ 0x001,  6, 0,0,0,0,0,0 },
	{ 0x101,  6, 0,0,0,0,0,1 },
	{ 0x002,  6, 0,0,0,1,0,0 },
	{ 0x102,  6, 0,0,0,1,0,1 },
	{ 0x202,  6, 0,0,0,1,1,0 },
	{ 0x003,  6, 0,0,1,0,0,0 },
	{ 0x103,  6, 0,0,1,0,0,1 },
	{ 0x203,  6, 0,0,1,0,1,0 },
	{ 0x303,  6, 0,0,1,0,1,1 },
	{ 0x004,  6, 0,0,1,1,0,0 },
	{ 0x104,  6, 0,0,1,1,0,1 },
	{ 0x204,  6, 0,0,1,1,1,0 },
	{ 0x304,  6, 0,0,1,1,1,1 },
	{ 0x005,  6, 0,1,0,0,0,0 },
	{ 0x105,  6, 0,1,0,0,0,1 },
	{ 0x205,  6, 0,1,0,0,1,0 },
	{ 0x305,  6, 0,1,0,0,1,1 },
	{ 0x006,  6, 0,1,0,1,0,0 },
	{ 0x106,  6, 0,1,0,1,0,1 },
	{ 0x206,  6, 0,1,0,1,1,0 },
	{ 0x306,  6, 0,1,0,1,1,1 },
	{ 0x007,  6, 0,1,1,0,0,0 },
	{ 0x107,  6, 0,1,1,0,0,1 },
	{ 0x207,  6, 0,1,1,0,1,0 },
	{ 0x307,  6, 0,1,1,0,1,1 },
	{ 0x008,  6, 0,1,1,1,0,0 },
	{ 0x108,  6, 0,1,1,1,0,1 },
	{ 0x208,  6, 0,1,1,1,1,0 },
	{ 0x308,  6, 0,1,1,1,1,1 },
	{ 0x009,  6, 1,0,0,0,0,0 },
	{ 0x109,  6, 1,0,0,0,0,1 },
	{ 0x209,  6, 1,0,0,0,1,0 },
	{ 0x309,  6, 1,0,0,0,1,1 },
	{ 0x00a,  6, 1,0,0,1,0,0 },
	{ 0x10a,  6, 1,0,0,1,0,1 },
	{ 0x20a,  6, 1,0,0,1,1,0 },
	{ 0x30a,  6, 1,0,0,1,1,1 },
	{ 0x00b,  6, 1,0,1,0,0,0 },
	{ 0x10b,  6, 1,0,1,0,0,1 },
	{ 0x20b,  6, 1,0,1,0,1,0 },
	{ 0x30b,  6, 1,0,1,0,1,1 },
	{ 0x00c,  6, 1,0,1,1,0,0 },
	{ 0x10c,  6, 1,0,1,1,0,1 },
	{ 0x20c,  6, 1,0,1,1,1,0 },
	{ 0x30c,  6, 1,0,1,1,1,1 },
	{ 0x00d,  6, 1,1,0,0,0,0 },
	{ 0x10d,  6, 1,1,0,0,0,1 },
	{ 0x20d,  6, 1,1,0,0,1,0 },
	{ 0x30d,  6, 1,1,0,0,1,1 },
	{ 0x00e,  6, 1,1,0,1,0,0 },
	{ 0x10e,  6, 1,1,0,1,0,1 },
	{ 0x20e,  6, 1,1,0,1,1,0 },
	{ 0x30e,  6, 1,1,0,1,1,1 },
	{ 0x00f,  6, 1,1,1,0,0,0 },
	{ 0x10f,  6, 1,1,1,0,0,1 },
	{ 0x20f,  6, 1,1,1,0,1,0 },
	{ 0x30f,  6, 1,1,1,0,1,1 },
	{ 0x010,  6, 1,1,1,1,0,0 },
	{ 0x110,  6, 1,1,1,1,0,1 },
	{ 0x210,  6, 1,1,1,1,1,0 },
	{ 0x310,  6, 1,1,1,1,1,1 },
	{ 0 },
};

static const struct vs_vlc_val coeff_token_m1[] = {
	{ 0x000,  2, 0,1 },
	{ 0x001,  6, 0,0,0,1,1,1 },
	{ 0x101,  1, 1 },
	{ 0x002,  6, 0,0,0,1,0,0 },
	{ 0x102,  6, 0,0,0,1,1,0 },
	{ 0x202,  3, 0,0,1 },
	{ 0x003,  6, 0,0,0,0,1,1 },
	{ 0x103,  7, 0,0,0,0,0,1,1 },
	{ 0x203,  7, 0,0,0,0,0,1,0 },
	{ 0x303,  6, 0,0,0,1,0,1 },
	{ 0x004,  6, 0,0,0,0,1,0 },
	{ 0x104,  8, 0,0,0,0,0,0,1,1 },
	{ 0x204,  8, 0,0,0,0,0,0,1,0 },
	{ 0x304,  7, 0,0,0,0,0,0,0 },
	{ 0 },
};

static const struct vs_vlc_val coeff_token_m2[] = {
	{ 0x000,  1, 1 },
	{ 0x001,  7, 0,0,0,1,1,1,1 },
	{ 0x101,  2, 0,1 },
	{ 0x002,  7, 0,0,0,1,1,1,0 },
	{ 0x102,  7, 0,0,0,1,1,0,1 },
	{ 0x202,  3, 0,0,1 },
	{ 0x003,  9, 0,0,0,0,0,0,1,1,1 },
	{ 0x103,  7, 0,0,0,1,1,0,0 },
	{ 0x203,  7, 0,0,0,1,0,1,1 },
	{ 0x303,  5, 0,0,0,0,1 },
	{ 0x004,  9, 0,0,0,0,0,0,1,1,0 },
	{ 0x104,  9, 0,0,0,0,0,0,1,0,1 },
	{ 0x204,  7, 0,0,0,1,0,1,0 },
	{ 0x304,  6, 0,0,0,0,0,1 },
	{ 0x005, 10, 0,0,0,0,0,0,0,1,1,1 },
	{ 0x105, 10, 0,0,0,0,0,0,0,1,1,0 },
	{ 0x205,  9, 0,0,0,0,0,0,1,0,0 },
	{ 0x305,  7, 0,0,0,1,0,0,1 },
	{ 0x006, 11, 0,0,0,0,0,0,0,0,1,1,1 },
	{ 0x106, 11, 0,0,0,0,0,0,0,0,1,1,0 },
	{ 0x206, 10, 0,0,0,0,0,0,0,1,0,1 },
	{ 0x306,  7, 0,0,0,1,0,0,0 },
	{ 0x007, 12, 0,0,0,0,0,0,0,0,0,1,1,1 },
	{ 0x107, 12, 0,0,0,0,0,0,0,0,0,1,1,0 },
	{ 0x207, 11, 0,0,0,0,0,0,0,0,1,0,1 },
	{ 0x307, 10, 0,0,0,0,0,0,0,1,0,0 },
	{ 0x008, 13, 0,0,0,0,0,0,0,0,0,0,1,1,1 },
	{ 0x108, 12, 0,0,0,0,0,0,0,0,0,1,0,1 },
	{ 0x208, 12, 0,0,0,0,0,0,0,0,0,1,0,0 },
	{ 0x308, 11, 0,0,0,0,0,0,0,0,1,0,0 },
	{ 0 },
};

static const struct vs_vlc_val *const coeff_token_tab[17] = {
	coeff_token_0,
	coeff_token_0,
	coeff_token_2,
	coeff_token_2,
	coeff_token_4,
	coeff_token_4,
	coeff_token_4,
	coeff_token_4,
	coeff_token_8,
	coeff_token_8,
	coeff_token_8,
	coeff_token_8,
	coeff_token_8,
	coeff_token_8,
	coeff_token_8,
	coeff_token_8,
	coeff_token_8,
};

int h264_coeff_token(struct bitstream *str, struct h264_slice *slice, int cat, int idx, uint32_t *trailingOnes, uint32_t *totalCoeff) {
	const struct vs_vlc_val *tab = 0;
	enum h264_block_size bs;
	int which;
	switch (cat) {
		case H264_CTXBLOCKCAT_CHROMA_DC:
			if (slice->chroma_array_type == 1)
				tab = coeff_token_m1;
			else
				tab = coeff_token_m2;
			break;
		case H264_CTXBLOCKCAT_CHROMA_AC:
			which = (idx >> 3) + 1;
			idx &= 7;
			bs = H264_BLOCK_CHROMA;
			break;
		case H264_CTXBLOCKCAT_LUMA_DC:
		case H264_CTXBLOCKCAT_LUMA_AC:
		case H264_CTXBLOCKCAT_LUMA_4X4:
		case H264_CTXBLOCKCAT_LUMA_8X8:
			which = 0;
			bs = H264_BLOCK_4X4;
			break;
		case H264_CTXBLOCKCAT_CB_DC:
		case H264_CTXBLOCKCAT_CB_AC:
		case H264_CTXBLOCKCAT_CB_4X4:
		case H264_CTXBLOCKCAT_CB_8X8:
			which = 1;
			bs = H264_BLOCK_4X4;
			break;
		case H264_CTXBLOCKCAT_CR_DC:
		case H264_CTXBLOCKCAT_CR_AC:
		case H264_CTXBLOCKCAT_CR_4X4:
		case H264_CTXBLOCKCAT_CR_8X8:
			which = 2;
			bs = H264_BLOCK_4X4;
			break;
	}
	if (!tab) {
		int idxA, idxB;
		const struct h264_macroblock *mbT = h264_mb_nb_p(slice, H264_MB_THIS, 0);
		int inter = h264_is_inter_mb_type(mbT->mb_type);
		const struct h264_macroblock *mbA = h264_mb_nb_b(slice, H264_MB_A, bs, inter, idx, &idxA);
		const struct h264_macroblock *mbB = h264_mb_nb_b(slice, H264_MB_B, bs, inter, idx, &idxB);
		mbA = h264_inter_filter(slice, mbA, inter);
		mbB = h264_inter_filter(slice, mbB, inter);
		int availA = mbA->mb_type != H264_MB_TYPE_UNAVAIL;
		int availB = mbB->mb_type != H264_MB_TYPE_UNAVAIL;
		int nA = mbA->total_coeff[which][idxA];
		int nB = mbB->total_coeff[which][idxB];
		int nC;
		if (availA && availB)
			nC = (nA + nB + 1) >> 1;
		else if (availA)
			nC = nA;
		else if (availB)
			nC = nB;
		else
			nC = 0;
		tab = coeff_token_tab[nC];
	}
	uint32_t tmp;
	tmp = *trailingOnes << 8 | *totalCoeff;
	if (vs_vlc(str, &tmp, tab)) return 1;
	if (str->dir == VS_DECODE) {
		*trailingOnes = tmp >> 8;
		*totalCoeff = tmp & 0xff;
	}
	return 0;
}

int h264_level(struct bitstream *str, int suffixLength, int onebias, int32_t *val) {
	/*
	 * each level_prefix takes 1 << suffix_size consecutive level_codes from the code space
	 * level_prefix 0..13:
	 *   suffix size = suffixLength
	 * level prefix 14:
	 *   suffix size = suffixLength ? suffixLength : 4
	 * level prefix 15+:
	 *   suffix size = level_prefix - 3 [thus 12+]
	 */
	int suffix_size;
	int level_code;
	int level_prefix;
	uint32_t level_suffix;
	uint32_t tmp;
	if (str->dir == VS_ENCODE) {
		if (*val < 0)
			level_code = -((*val << 1) + 1);
		else
			level_code = ((*val << 1) - 2);
		if (onebias)
			level_code -= 2;

		level_prefix = 0;
		level_suffix = level_code;
		while (1) {
			suffix_size = suffixLength;
			if (level_prefix == 14 && !suffix_size)
				suffix_size = 4;
			if (level_prefix >= 15)
				suffix_size = level_prefix - 3;
			if (level_suffix < (1 << suffix_size))
				break;
			level_suffix -= (1 << suffix_size);
			level_prefix++;
			tmp = 0;
			if (vs_u(str, &tmp, 1)) return 1;
		}
		tmp = 1;
		if (vs_u(str, &tmp, 1)) return 1;
		if (suffix_size)
			if (vs_u(str, &level_suffix, suffix_size)) return 1;
	} else {
		level_code = 0;
		level_prefix = 0;
		while (1) {
			suffix_size = suffixLength;
			if (level_prefix == 14 && !suffix_size)
				suffix_size = 4;
			if (level_prefix >= 15)
				suffix_size = level_prefix - 3;
			if (vs_u(str, &tmp, 1)) return 1;
			if (tmp)
				break;
			level_code += (1 << suffix_size);
			level_prefix++;
		}
		if (suffix_size) {
			if (vs_u(str, &level_suffix, suffix_size)) return 1;
			level_code += level_suffix;
		}

		if (onebias)
			level_code += 2;
		if (level_code & 1)
			*val = (-level_code - 1) >> 1;
		else
			*val = (level_code + 2) >> 1;
	}
	return 0;
}

static const struct vs_vlc_val total_zeros_1[] = {
	{  0,  1, 1 },
	{  1,  3, 0,1,1 },
	{  2,  3, 0,1,0 },
	{  3,  4, 0,0,1,1 },
	{  4,  4, 0,0,1,0 },
	{  5,  5, 0,0,0,1,1 },
	{  6,  5, 0,0,0,1,0 },
	{  7,  6, 0,0,0,0,1,1 },
	{  8,  6, 0,0,0,0,1,0 },
	{  9,  7, 0,0,0,0,0,1,1 },
	{ 10,  7, 0,0,0,0,0,1,0 },
	{ 11,  8, 0,0,0,0,0,0,1,1 },
	{ 12,  8, 0,0,0,0,0,0,1,0 },
	{ 13,  9, 0,0,0,0,0,0,0,1,1 },
	{ 14,  9, 0,0,0,0,0,0,0,1,0 },
	{ 15,  9, 0,0,0,0,0,0,0,0,1 },
};

static const struct vs_vlc_val total_zeros_2[] = {
	{  0,  3, 1,1,1 },
	{  1,  3, 1,1,0 },
	{  2,  3, 1,0,1 },
	{  3,  3, 1,0,0 },
	{  4,  3, 0,1,1 },
	{  5,  4, 0,1,0,1 },
	{  6,  4, 0,1,0,0 },
	{  7,  4, 0,0,1,1 },
	{  8,  4, 0,0,1,0 },
	{  9,  5, 0,0,0,1,1 },
	{ 10,  5, 0,0,0,1,0 },
	{ 11,  6, 0,0,0,0,1,1 },
	{ 12,  6, 0,0,0,0,1,0 },
	{ 13,  6, 0,0,0,0,0,1 },
	{ 14,  6, 0,0,0,0,0,0 },
};

static const struct vs_vlc_val total_zeros_3[] = {
	{  0,  4, 0,1,0,1 },
	{  1,  3, 1,1,1 },
	{  2,  3, 1,1,0 },
	{  3,  3, 1,0,1 },
	{  4,  4, 0,1,0,0 },
	{  5,  4, 0,0,1,1 },
	{  6,  3, 1,0,0 },
	{  7,  3, 0,1,1 },
	{  8,  4, 0,0,1,0 },
	{  9,  5, 0,0,0,1,1 },
	{ 10,  5, 0,0,0,1,0 },
	{ 11,  6, 0,0,0,0,0,1 },
	{ 12,  5, 0,0,0,0,1 },
	{ 13,  6, 0,0,0,0,0,0 },
};

static const struct vs_vlc_val total_zeros_4[] = {
	{  0,  5, 0,0,0,1,1 },
	{  1,  3, 1,1,1 },
	{  2,  4, 0,1,0,1 },
	{  3,  4, 0,1,0,0 },
	{  4,  3, 1,1,0 },
	{  5,  3, 1,0,1 },
	{  6,  3, 1,0,0 },
	{  7,  4, 0,0,1,1 },
	{  8,  3, 0,1,1 },
	{  9,  4, 0,0,1,0 },
	{ 10,  5, 0,0,0,1,0 },
	{ 11,  5, 0,0,0,0,1 },
	{ 12,  5, 0,0,0,0,0 },
};

static const struct vs_vlc_val total_zeros_5[] = {
	{  0,  4, 0,1,0,1 },
	{  1,  4, 0,1,0,0 },
	{  2,  4, 0,0,1,1 },
	{  3,  3, 1,1,1 },
	{  4,  3, 1,1,0 },
	{  5,  3, 1,0,1 },
	{  6,  3, 1,0,0 },
	{  7,  3, 0,1,1 },
	{  8,  4, 0,0,1,0 },
	{  9,  5, 0,0,0,0,1 },
	{ 10,  4, 0,0,0,1 },
	{ 11,  5, 0,0,0,0,0 },
};

static const struct vs_vlc_val total_zeros_6[] = {
	{  0,  6, 0,0,0,0,0,1 },
	{  1,  5, 0,0,0,0,1 },
	{  2,  3, 1,1,1 },
	{  3,  3, 1,1,0 },
	{  4,  3, 1,0,1 },
	{  5,  3, 1,0,0 },
	{  6,  3, 0,1,1 },
	{  7,  3, 0,1,0 },
	{  8,  4, 0,0,0,1 },
	{  9,  3, 0,0,1 },
	{ 10,  6, 0,0,0,0,0,0 },
};

static const struct vs_vlc_val total_zeros_7[] = {
	{  0,  6, 0,0,0,0,0,1 },
	{  1,  5, 0,0,0,0,1 },
	{  2,  3, 1,0,1 },
	{  3,  3, 1,0,0 },
	{  4,  3, 0,1,1 },
	{  5,  2, 1,1 },
	{  6,  3, 0,1,0 },
	{  7,  4, 0,0,0,1 },
	{  8,  3, 0,0,1 },
	{  9,  6, 0,0,0,0,0,0 },
};

static const struct vs_vlc_val total_zeros_8[] = {
	{  0,  6, 0,0,0,0,0,1 },
	{  1,  4, 0,0,0,1 },
	{  2,  5, 0,0,0,0,1 },
	{  3,  3, 0,1,1 },
	{  4,  2, 1,1 },
	{  5,  2, 1,0 },
	{  6,  3, 0,1,0 },
	{  7,  3, 0,0,1 },
	{  8,  6, 0,0,0,0,0,0 },
};

static const struct vs_vlc_val total_zeros_9[] = {
	{  0,  6, 0,0,0,0,0,1 },
	{  1,  6, 0,0,0,0,0,0 },
	{  2,  4, 0,0,0,1 },
	{  3,  2, 1,1 },
	{  4,  2, 1,0 },
	{  5,  3, 0,0,1 },
	{  6,  2, 0,1 },
	{  7,  5, 0,0,0,0,1 },
};

static const struct vs_vlc_val total_zeros_10[] = {
	{  0,  5, 0,0,0,0,1 },
	{  1,  5, 0,0,0,0,0 },
	{  2,  3, 0,0,1 },
	{  3,  2, 1,1 },
	{  4,  2, 1,0 },
	{  5,  2, 0,1 },
	{  6,  4, 0,0,0,1 },
};

static const struct vs_vlc_val total_zeros_11[] = {
	{  0,  4, 0,0,0,0 },
	{  1,  4, 0,0,0,1 },
	{  2,  3, 0,0,1 },
	{  3,  3, 0,1,0 },
	{  4,  1, 1 },
	{  5,  3, 0,1,1 },
};

static const struct vs_vlc_val total_zeros_12[] = {
	{  0,  4, 0,0,0,0 },
	{  1,  4, 0,0,0,1 },
	{  2,  2, 0,1 },
	{  3,  1, 1 },
	{  4,  3, 0,0,1 },
};

static const struct vs_vlc_val total_zeros_13[] = {
	{  0,  3, 0,0,0 },
	{  1,  3, 0,0,1 },
	{  2,  1, 1 },
	{  3,  2, 0,1 },
};

static const struct vs_vlc_val total_zeros_14[] = {
	{  0,  2, 0,0 },
	{  1,  2, 0,1 },
	{  2,  1, 1 },
};

static const struct vs_vlc_val total_zeros_15[] = {
	{  0,  1, 0 },
	{  1,  1, 1 },
};

static const struct vs_vlc_val total_zeros_c1_1[] = {
	{  0,  1, 1 },
	{  1,  2, 0,1 },
	{  2,  3, 0,0,1 },
	{  3,  3, 0,0,0 },
};

static const struct vs_vlc_val total_zeros_c1_2[] = {
	{  0,  1, 1 },
	{  1,  2, 0,1 },
	{  2,  2, 0,0 },
};

static const struct vs_vlc_val total_zeros_c1_3[] = {
	{  0,  1, 1 },
	{  1,  1, 0 },
};

static const struct vs_vlc_val total_zeros_c2_1[] = {
	{  0,  1, 1 },
	{  1,  3, 0,1,0 },
	{  2,  3, 0,1,1 },
	{  3,  4, 0,0,1,0 },
	{  4,  4, 0,0,1,1 },
	{  5,  4, 0,0,0,1 },
	{  6,  5, 0,0,0,0,1 },
	{  7,  5, 0,0,0,0,0 },
};

static const struct vs_vlc_val total_zeros_c2_2[] = {
	{  0,  3, 0,0,0 },
	{  1,  2, 0,1 },
	{  2,  3, 0,0,1 },
	{  3,  3, 1,0,0 },
	{  4,  3, 1,0,1 },
	{  5,  3, 1,1,0 },
	{  6,  3, 1,1,1 },
};

static const struct vs_vlc_val total_zeros_c2_3[] = {
	{  0,  3, 0,0,0 },
	{  1,  3, 0,0,1 },
	{  2,  2, 0,1 },
	{  3,  2, 1,0 },
	{  4,  3, 1,1,0 },
	{  5,  3, 1,1,1 },
};

static const struct vs_vlc_val total_zeros_c2_4[] = {
	{  0,  3, 1,1,0 },
	{  1,  2, 0,0 },
	{  2,  2, 0,1 },
	{  3,  2, 1,0 },
	{  4,  3, 1,1,1 },
};

static const struct vs_vlc_val total_zeros_c2_5[] = {
	{  0,  2, 0,0 },
	{  1,  2, 0,1 },
	{  2,  2, 1,0 },
	{  3,  2, 1,1 },
};

static const struct vs_vlc_val total_zeros_c2_6[] = {
	{  0,  2, 0,0 },
	{  1,  2, 0,1 },
	{  2,  1, 1 },
};

static const struct vs_vlc_val total_zeros_c2_7[] = {
	{  0,  1, 0 },
	{  1,  1, 1 },
};

static const struct vs_vlc_val *const total_zeros_tab[16] = {
	0,
	total_zeros_1,
	total_zeros_2,
	total_zeros_3,
	total_zeros_4,
	total_zeros_5,
	total_zeros_6,
	total_zeros_7,
	total_zeros_8,
	total_zeros_9,
	total_zeros_10,
	total_zeros_11,
	total_zeros_12,
	total_zeros_13,
	total_zeros_14,
	total_zeros_15,
};

static const struct vs_vlc_val *const total_zeros_c1_tab[4] = {
	0,
	total_zeros_c1_1,
	total_zeros_c1_2,
	total_zeros_c1_3,
};

static const struct vs_vlc_val *const total_zeros_c2_tab[8] = {
	0,
	total_zeros_c2_1,
	total_zeros_c2_2,
	total_zeros_c2_3,
	total_zeros_c2_4,
	total_zeros_c2_5,
	total_zeros_c2_6,
	total_zeros_c2_7,
};

int h264_total_zeros(struct bitstream *str, int mode, int tzVlcIndex, uint32_t *val) {
	const struct vs_vlc_val *tab;
	switch (mode) {
		case 0:
			tab = total_zeros_tab[tzVlcIndex];
			break;
		case 1:
			tab = total_zeros_c1_tab[tzVlcIndex];
			break;
		case 2:
			tab = total_zeros_c2_tab[tzVlcIndex];
			break;
	}
	if (vs_vlc(str, val, tab)) return 1;
	return 0;
}

static const struct vs_vlc_val run_before_1[] = {
	{ 0, 1, 1 },
	{ 1, 1, 0 },
	{ 0 },
};

static const struct vs_vlc_val run_before_2[] = {
	{ 0, 1, 1 },
	{ 1, 2, 0,1 },
	{ 2, 2, 0,0 },
	{ 0 },
};

static const struct vs_vlc_val run_before_3[] = {
	{ 0, 2, 1,1 },
	{ 1, 2, 1,0 },
	{ 2, 2, 0,1 },
	{ 3, 2, 0,0 },
	{ 0 },
};

static const struct vs_vlc_val run_before_4[] = {
	{ 0, 2, 1,1 },
	{ 1, 2, 1,0 },
	{ 2, 2, 0,1 },
	{ 3, 3, 0,0,1 },
	{ 4, 3, 0,0,0 },
	{ 0 },
};

static const struct vs_vlc_val run_before_5[] = {
	{ 0, 2, 1,1 },
	{ 1, 2, 1,0 },
	{ 2, 3, 0,1,1 },
	{ 3, 3, 0,1,0 },
	{ 4, 3, 0,0,1 },
	{ 5, 3, 0,0,0 },
	{ 0 },
};

static const struct vs_vlc_val run_before_6[] = {
	{ 0, 2, 1,1 },
	{ 1, 3, 0,0,0 },
	{ 2, 3, 0,0,1 },
	{ 3, 3, 0,1,1 },
	{ 4, 3, 0,1,0 },
	{ 5, 3, 1,0,1 },
	{ 6, 3, 1,0,0 },
	{ 0 },
};

static const struct vs_vlc_val run_before_x[] = {
	{ 0, 3, 1,1,1 },
	{ 1, 3, 1,1,0 },
	{ 2, 3, 1,0,1 },
	{ 3, 3, 1,0,0 },
	{ 4, 3, 0,1,1 },
	{ 5, 3, 0,1,0 },
	{ 6, 3, 0,0,1 },
	{ 7, 4, 0,0,0,1 },
	{ 8, 5, 0,0,0,0,1 },
	{ 9, 6, 0,0,0,0,0,1 },
	{ 10, 7, 0,0,0,0,0,0,1 },
	{ 11, 8, 0,0,0,0,0,0,0,1 },
	{ 12, 9, 0,0,0,0,0,0,0,0,1 },
	{ 13, 10, 0,0,0,0,0,0,0,0,0,1 },
	{ 14, 11, 0,0,0,0,0,0,0,0,0,0,1 },
	{ 0 },
};

static const struct vs_vlc_val *const run_before_tab[16] = {
	0,
	run_before_1,
	run_before_2,
	run_before_3,
	run_before_4,
	run_before_5,
	run_before_6,
	run_before_x,
	run_before_x,
	run_before_x,
	run_before_x,
	run_before_x,
	run_before_x,
	run_before_x,
	run_before_x,
	run_before_x,
};

int h264_run_before(struct bitstream *str, int zerosLeft, uint32_t *val) {
	if (zerosLeft)
		return vs_vlc(str, val, run_before_tab[zerosLeft]);
	else
		return vs_infer(str, val, 0);
}
