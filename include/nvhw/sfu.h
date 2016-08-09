/*
 * Copyright (C) 2015 Marcin Kościelnicki <koriakin@0x04.net>
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

#ifndef NVHW_SFU_H
#define NVHW_SFU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum sfu_pre_mode {
	SFU_PRE_SIN = 0,
	SFU_PRE_EX2 = 1,
};

uint32_t sfu_pre(uint32_t x, enum sfu_pre_mode mode);
uint32_t sfu_rcp(uint32_t x);
uint32_t sfu_rsqrt(uint32_t x);
uint32_t sfu_sincos(uint32_t x, bool cos);
uint32_t sfu_ex2(uint32_t x);
uint32_t sfu_lg2(uint32_t x);
#if 0
uint32_t sfu_rcp64h(uint32_t x);
uint32_t sfu_rsqrt64h(uint32_t x);
#endif

extern const int32_t sfu_rcp_tab[0x80][3];
extern const int32_t sfu_rsqrt_tab[0x80][3];
extern const int32_t sfu_sin_tab[0x40][3];
extern const int32_t sfu_ex2_tab[0x40][3];
extern const int32_t sfu_lg2_tab[0x40][3];

#ifdef __cplusplus
}
#endif

#endif
