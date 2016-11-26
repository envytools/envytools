/*
 * Copyright (C) 2013 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "hwtest.h"
#include "old.h"
#include "nvhw/mpeg.h"
#include "nva.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int test_sess_key(struct hwtest_ctx *ctx) {
	int i;
	nva_wr32(ctx->cnum, 0x200, 0xfffffffd);
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	for (i = 0; i < 10000000; i++) {
		uint32_t host = jrand48(ctx->rand48);
		uint16_t host_key = host & 0xffff;
		uint8_t host_sel = host >> 24 & 7;
		uint8_t host_chash = mpeg_crypt_host_hash(host_key, host_sel);
		if (jrand48(ctx->rand48) & 1) {
			/* use correct hash with 1/2 probability, random otherwise */
			host &= ~(0xff << 16);
			host |= host_chash << 16;
		}
		uint8_t host_hash = host >> 16 & 0xff;
		uint32_t prev = nva_rd32(ctx->cnum, 0xb354);
		nva_wr32(ctx->cnum, 0xb350, host);
		uint32_t mpeg = nva_rd32(ctx->cnum, 0xb354);
		if (host_hash != host_chash) {
			uint32_t exp = prev;
			if (ctx->chipset.chipset >= 0x80 || ctx->chipset.chipset == 0x50)
				exp = 0;
			if (mpeg != exp) {
				printf("Host val %08x accepted but shouldn't be; exp %08x prev %08x mpeg %08x\n", host, exp, prev, mpeg);
				return HWTEST_RES_FAIL;
			}
		} else {
			uint16_t mpeg_key = mpeg & 0xffff;
			uint8_t tkey_id = mpeg >> 16 & 0xff;
			uint8_t sess_hash = mpeg >> 24 & 0xff;
			uint8_t sess_chash = mpeg_crypt_sess_hash(host_key, mpeg_key);
			uint8_t tkeys[8] = { 0x55, 0x89, 0x33, 0xeb, 0xef, 0x2b, 0x8b, 0x4b };
			if (sess_hash != sess_chash || (tkeys[host_sel] != tkey_id && tkeys[host_sel] != (tkey_id ^ 0xaa))) {
				if (mpeg == 0 || mpeg == prev) {
					printf("Host val %08x not accepted but should be\n", host);
					return HWTEST_RES_FAIL;
				}
				if (sess_hash != sess_chash)
					printf("sess hash mismatch: host %08x mpeg %08x exp %02x\n", host, mpeg, sess_chash);
				if (tkeys[host_sel] != tkey_id && tkeys[host_sel] != (tkey_id ^ 0xaa))
					printf("tkey mismatch: host %08x mpeg %08x exp %02x %02x\n", host, mpeg, tkeys[host_sel], tkeys[host_sel] ^ 0xaa);
				return HWTEST_RES_FAIL;
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int mpeg_crypt_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset < 0x17 || ctx->chipset.chipset > 0xa0 || ctx->chipset.chipset == 0x98 || (ctx->chipset.chipset & 0xf0) == 0x20 || ctx->chipset.chipset == 0x1a)
		return HWTEST_RES_NA;
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(mpeg_crypt,
	HWTEST_TEST(test_sess_key, 0),
)
