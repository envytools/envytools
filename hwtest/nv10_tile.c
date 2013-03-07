/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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
#include <string.h>
#include <unistd.h>

static void get_mc_config(struct hwtest_ctx *ctx, struct mc_config *mcc) {
	uint32_t cfg0, cfg1;
	if (is_igp(ctx->chipset)) {
		memset(mcc, 0, sizeof *mcc);
		return;
	}
	switch (pfb_type(ctx->chipset)) {
		case PFB_NV10:
			cfg0 = nva_rd32(ctx->cnum, 0x100200);
			mcc->mcbits = 2;
			switch (cfg0 & 0x30) {
				case 0x20:
					mcc->partbits = 0;
					break;
				case 0x00:
					mcc->partbits = 1;
					break;
				case 0x10:
					mcc->partbits = 2;
					break;
			}
			mcc->parts = 1;
			mcc->colbits = mcc->colbits_lo = cfg0 >> 24 & 0xf;
			/* XXX: figure out just what the fuck is happening here */
			if (mcc->colbits_lo > 9)
				mcc->colbits_lo = 9;
			mcc->bankbits_lo = 1;
			mcc->bankbits[0] = mcc->bankbits[1] = (cfg0 >> 16 & 1) + 1;
			mcc->rank_interleave = 0;
			mcc->ranks = (cfg0 >> 12 & 1) + 1;
			mcc->rowbits[0] = mcc->rowbits[1] = cfg0 >> 20 & 0xf;
			mcc->burstbits = cfg0 & 1 || ctx->chipset >= 0x17;
			break;
		case PFB_NV20:
		case PFB_NV40:
		case PFB_NV41:
		case PFB_NV44:
			cfg0 = nva_rd32(ctx->cnum, 0x100200);
			cfg1 = nva_rd32(ctx->cnum, 0x100204);
			if (pfb_type(ctx->chipset) == PFB_NV44) {
				mcc->partbits = 1 - (cfg0 >> 1 & 1);
			} else {
				switch (cfg0 & 3) {
					case 0:
						mcc->partbits = 0;
						break;
					case 1:
						mcc->partbits = 1;
						break;
					case 3:
						mcc->partbits = 2;
						break;
				}
			}
			if (ctx->chipset < 0x40) {
				mcc->mcbits = (cfg0 >> 2 & 1) + 2;
			} else {
				if (pfb_type(ctx->chipset) == PFB_NV44) {
					mcc->mcbits = 2;
				} else {
					mcc->mcbits = 3;
				}
			}
			mcc->parts = 1 << mcc->partbits;
			mcc->colbits = mcc->colbits_lo = cfg1 >> 12 & 0xf;
			if (mcc->colbits_lo > 9 && pfb_type(ctx->chipset) == PFB_NV44)
				mcc->colbits_lo = 9;
			mcc->bankbits_lo = tile_bankoff_bits(ctx->chipset);
			mcc->rowbits[0] = cfg1 >> 16 & 0xf;
			mcc->rowbits[1] = cfg1 >> 20 & 0xf;
			mcc->bankbits[0] = (cfg1 >> 24 & 0xf) + 1;
			mcc->bankbits[1] = (cfg1 >> 28 & 0xf) + 1;
			mcc->ranks = (cfg0 >> 8 & 1) + 1;
			mcc->rank_interleave = cfg0 >> 9 & 1;
			mcc->burstbits = (cfg0 >> 11 & 1) + 1; /* valid for at least NV44, doesn't matter for others */
			if (pfb_type(ctx->chipset) >= PFB_NV40) {
				if (is_g7x(ctx->chipset))
					mcc->partshift = 8 - (cfg0 >> 4 & 1);
				else
					mcc->partshift = 8;
			}
			break;
		default:
			abort();
	}
}

static int test_scan(struct hwtest_ctx *ctx) {
	int i;
	uint32_t bs_base = 0;
	uint32_t bs_limit_1 = 0;
	uint32_t bs_limit_0 = 0;
	uint32_t bs_pitch = 0;
	uint32_t bs_comp = 0;
	uint32_t bs_comp_off = 0;
	uint32_t mmio_tile = tile_mmio_region(ctx->chipset);
	uint32_t mmio_comp = tile_mmio_comp(ctx->chipset);;
	int n = num_tile_regions(ctx->chipset);
	if (has_large_tile(ctx->chipset)) {
		bs_base = 0xffff0001;
		bs_limit_1 = 0xffffffff;
		bs_limit_0 = 0x0000ffff;
		bs_pitch = 0x1ff00;
	} else {
		bs_base = 0xffffc001;
		bs_limit_1 = 0xffffffff;
		bs_limit_0 = 0x00003fff;
		bs_pitch = 0xff00;
	}
	if (ctx->chipset < 0x20) {
		bs_base = 0x87ffc000;
		bs_limit_1 = 0x07ffc000;
		bs_limit_0 = 0;
	}
	switch (comp_type(ctx->chipset)) {
		case COMP_NV20:
			bs_comp = 0xbc03ffc0;
			bs_comp_off = 0x83ffc00f;
			break;
		case COMP_NV25:
			bs_comp = 0x01f3ffc0;
			bs_comp_off = 0x83ffc01f;
			break;
		case COMP_NV30:
			bs_comp = 0x1fffffff;
			bs_comp_off = 0x83ffc01f;
			break;
		case COMP_NV35:
			bs_comp = 0x7fffffff;
			bs_comp_off = 0x83ffc01f;
			break;
		case COMP_NV36:
			bs_comp = 0xffffffff;
			bs_comp_off = 0x83ffc01f;
			break;
		case COMP_NV40:
			bs_comp = 0x7fffffff;
			break;
	}
	if (pfb_type(ctx->chipset) == PFB_NV44) {
		if (is_igp(ctx->chipset))
			bs_base |= 0xe;
		else if (tile_bankoff_bits(ctx->chipset))
			bs_base |= 0x12;
		else
			bs_base |= 2;
	} else  {
		switch (tile_bankoff_bits(ctx->chipset)) {
			case 0:
				break;
			case 1:
				bs_base |= 2;
				break;
			case 2:
				bs_base |= 0x30;
				break;
		}
	}
	for (i = 0; i < n; i++) {
		TEST_BITSCAN(mmio_tile + 0x0 + i * 0x10, bs_base, 0);
		TEST_BITSCAN(mmio_tile + 0x4 + i * 0x10, bs_limit_1, bs_limit_0);
		TEST_BITSCAN(mmio_tile + 0x8 + i * 0x10, bs_pitch, 0);
		if (bs_comp)
			TEST_BITSCAN(mmio_comp + i * 0x4, bs_comp, 0);
	}
	TEST_BITSCAN(0x100324, bs_comp_off, 0);
	return HWTEST_RES_PASS;
}

static uint32_t compute_status(int chipset, uint32_t pitch, int enable) {
	int shift, factor;
	int valid = tile_pitch_valid(chipset, pitch, &shift, &factor);
	if (!valid)
		return 0;
	return enable << 31 | shift << 4 | factor;
}

static void set_tile(struct hwtest_ctx *ctx, int idx, uint32_t base, uint32_t limit, uint32_t pitch, int bankoff) {
	uint32_t mmio = tile_mmio_region(ctx->chipset) + idx * 0x10;
	vram_rd32(ctx->cnum, 0);
	nva_wr32(ctx->cnum, mmio+8, pitch);
	nva_wr32(ctx->cnum, mmio+4, limit);
	if (ctx->chipset < 0x20) {
		nva_wr32(ctx->cnum, mmio, base | 0x80000000);
	} else if (is_igp(ctx->chipset)) {
		nva_wr32(ctx->cnum, mmio, base | 1 | bankoff << 3);
	} else if (ctx->chipset >= 0x30) {
		nva_wr32(ctx->cnum, mmio, base | 1 | bankoff << 4);
	} else {
		nva_wr32(ctx->cnum, mmio, base | 1 | bankoff << 1);
	}
	nva_rd32(ctx->cnum, mmio);
}

uint32_t get_tile_status(struct hwtest_ctx *ctx, int idx) {
	uint32_t mmio = tile_mmio_region(ctx->chipset) + idx * 0x10;
	return nva_rd32(ctx->cnum, mmio+0xc);
}

static void unset_tile(struct hwtest_ctx *ctx, int idx) {
	uint32_t mmio = tile_mmio_region(ctx->chipset) + idx * 0x10;
	vram_rd32(ctx->cnum, 0);
	nva_wr32(ctx->cnum, mmio, 0);
	nva_rd32(ctx->cnum, mmio);
}

static void unset_comp(struct hwtest_ctx *ctx, int idx) {
	if (comp_type(ctx->chipset) == COMP_NONE)
		return;
	uint32_t mmio = tile_mmio_comp(ctx->chipset) + 4 * idx;
	vram_rd32(ctx->cnum, 0);
	nva_wr32(ctx->cnum, mmio, 0);
	nva_rd32(ctx->cnum, mmio);
}

static void set_comp(struct hwtest_ctx *ctx, int idx, int format, int tagbase, int taglimit) {
	uint32_t mmio = tile_mmio_comp(ctx->chipset) + 4 * idx;
	vram_rd32(ctx->cnum, 0);
	switch (comp_type(ctx->chipset)) {
		case COMP_NV20:
			nva_wr32(ctx->cnum, mmio, 0x80000000 | format << 26 | (tagbase & 0x3ffc0));
			break;
		case COMP_NV25:
			nva_wr32(ctx->cnum, mmio, format << 20 | (tagbase & 0x3ffc0));
			break;
		case COMP_NV30:
			nva_wr32(ctx->cnum, mmio, format << 24 | (tagbase >> 6 & 0xfff) | (taglimit >> 6 & 0xfff) << 12);
			break;
		case COMP_NV35:
		case COMP_NV40:
			nva_wr32(ctx->cnum, mmio, format << 26 | (tagbase >> 6 & 0x1fff) | (taglimit >> 6 & 0x1fff) << 13);
			break;
		case COMP_NV36:
			nva_wr32(ctx->cnum, mmio, format << 28 | (tagbase >> 6 & 0x3fff) | (taglimit >> 6 & 0x3fff) << 14);
			break;
		default:
			abort();
	}
	nva_rd32(ctx->cnum, mmio);
}

static void clear_tile(struct hwtest_ctx *ctx) {
	int i, n = num_tile_regions(ctx->chipset);
	for (i = 0; i < n; i++) {
		unset_tile(ctx, i);
		unset_comp(ctx, i);
	}
}

static int test_status(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 0x200; i++) {
		uint32_t pitch = i << 8;
		set_tile(ctx, 0, 0, 0, pitch, 0);
		uint32_t exp = compute_status(ctx->chipset, pitch, 1);
		uint32_t real = get_tile_status(ctx, 0);
		if (exp != real && exp) {
			printf("Tile region status mismatch for pitch %05x [enabled]: is %08x, expected %08x\n", pitch, real, exp);
			return HWTEST_RES_FAIL;
		}
		unset_tile(ctx, 0);
		exp = compute_status(ctx->chipset, pitch, 0);
		real = get_tile_status(ctx, 0);
		if (exp != real && exp) {
			printf("Tile region status mismatch for pitch %05x: is %08x, expected %08x\n", pitch, real, exp);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_comp_size(struct hwtest_ctx *ctx) {
	uint32_t expected;
	uint32_t real = nva_rd32(ctx->cnum, 0x100320);
	switch (ctx->chipset) {
		case 0x20:
			expected = 0x7fff;
			break;
		case 0x25:
		case 0x28:
			expected = 0xbfff;
			break;
		case 0x30:
			expected = 0x2e3ff;
			break;
		case 0x31:
			expected = 0x3ffff;
			break;
		case 0x35:
			expected = 0x5c7ff;
			break;
		case 0x36:
			expected = 0xb9fff;
			break;
		case 0x40:
		case 0x41:
		case 0x42:
			expected = 0x2e3ff;
			break;
		case 0x43:
			expected = 0x5c7ff;
			break;
		case 0x47:
		case 0x49:
			expected = 0x47fff;
			break;
		case 0x4b:
			expected = 0x5cbff;
			break;
		default:
			printf("Don't know expected comp size for NV%02X [%08x] - please report!\n", ctx->chipset, real);
			return HWTEST_RES_UNPREP;
	}
	if (real != expected) {
		printf("Zcomp size mismatch: is %08x, expected %08x\n", real, expected);
		return HWTEST_RES_FAIL;
	}
	return HWTEST_RES_PASS;
}

static uint32_t comp_seek(int cnum, int part, int addr) {
	if (nva_cards[cnum].chipset == 0x20) {
		nva_wr32(cnum, 0x1000f0, 0x1300000 |
			 (part << 16) | (addr & 0x1fc0));
		return 0x100100 + (addr & 0x3c);
	} else if (nva_cards[cnum].chipset < 0x30) {
		nva_wr32(cnum, 0x1000f0, 0x2380000 |
			 ((addr << 6) & 0x40000) |
			 (part << 16) | (addr & 0xfc0));
		return 0x100100 + (addr & 0x3c);
	} else if (nva_cards[cnum].chipset < 0x35) {
		nva_wr32(cnum, 0x1000f0, 0x2380000 |
			 (part << 16) | (addr & 0x7fc0));
		return 0x100100 + (addr & 0x3c);
	} else if (nva_cards[cnum].chipset < 0x36) {
		nva_wr32(cnum, 0x1000f0, 0x2380000 | (addr & 4) << 16 |
			 (part << 16) | (addr >> 1 & 0x7fc0));
		return 0x100100 + (addr >> 1 & 0x3c);
	} else if (nva_cards[cnum].chipset < 0x40) {
		nva_wr32(cnum, 0x1000f0, 0x2380000 | (addr & 0xc) << 15 |
			 (part << 16) | (addr >> 2 & 0x7fc0));
		return 0x100100 + (addr >> 2 & 0x3c);
	} else {
		nva_wr32(cnum, 0x1000f0, 0x80000 |
			 (part << 16) | (addr & 0xffc0));
		return 0x100100 + (addr & 0x3c);
	}
}

static uint32_t comp_rd32(int cnum, int part, int addr) {
	uint32_t a = comp_seek(cnum, part, addr);
	uint32_t res = nva_rd32(cnum, a);
	nva_wr32(cnum, 0x1000f0, 0);
	return res;
}

void comp_wr32(int cnum, int part, int addr, uint32_t v) {
	uint32_t a = comp_seek(cnum, part, addr);
	nva_wr32(cnum, a, v);
	nva_rd32(cnum, a);
	nva_wr32(cnum, 0x1000f0, 0);
}

void clear_comp(int cnum) {
	uint32_t size = (nva_rd32(cnum, 0x100320) + 1) / 8;
	int i, j;
	for (i = 0; i < get_maxparts(nva_cards[cnum].chipset); i++) {
		for (j = 0; j < size; j += 0x4)
			comp_wr32(cnum, i, j, 0);
	}
}

static int test_comp_access(struct hwtest_ctx *ctx) {
	uint32_t size = (nva_rd32(ctx->cnum, 0x100320) + 1) / 8;
	int i, j;
	int parts = get_maxparts(ctx->chipset);
	if (0) {
		for (i = 0; i < 0x400; i++) {
			nva_wr32(ctx->cnum, 0x1000f0, i << 16);
			uint32_t old[16];
			printf("%03x:\n", i);
			for (j = 0; j < 16; j++)
				old[j] = nva_rd32(ctx->cnum, 0x100100 + j * 4);
			for (j = 0; j < 16; j++)
				printf("%08x%c", nva_rd32(ctx->cnum, 0x100100 + j * 4), j == 15 ? '\n' : ' ');
			for (j = 0; j < 16; j++)
				nva_wr32(ctx->cnum, 0x100100 + j * 4, 0xffffffff);
			for (j = 0; j < 16; j++)
				printf("%08x%c", nva_rd32(ctx->cnum, 0x100100 + j * 4), j == 15 ? '\n' : ' ');
			for (j = 0; j < 16; j++)
				nva_wr32(ctx->cnum, 0x100100 + j * 4, 0);
			for (j = 0; j < 16; j++)
				printf("%08x%c", nva_rd32(ctx->cnum, 0x100100 + j * 4), j == 15 ? '\n' : ' ');
			for (j = 0; j < 16; j++)
				nva_wr32(ctx->cnum, 0x100100 + j * 4, old[j]);
		}
		nva_wr32(ctx->cnum, 0x1000f0, 0);
		return HWTEST_RES_FAIL;
	}
	clear_comp(ctx->cnum);
	for (i = 0; i < parts; i++) {
		for (j = 0; j < size; j += 0x4)
			comp_wr32(ctx->cnum, i, j, 0xdea00000 | i << 16 | j);
	}
	for (i = 0; i < parts; i++) {
		for (j = 0; j < size; j += 0x4) {
			uint32_t val = comp_rd32(ctx->cnum, i, j);
			uint32_t x = 0xdea00000 | i << 16 | j;
			if (val != x) {
				printf("MISMATCH %08x %08x\n", x, val);
				return HWTEST_RES_FAIL;
			}
		}
	}
	return HWTEST_RES_PASS;
}

static int test_comp_layout(struct hwtest_ctx *ctx) {
	struct mc_config mcc;
	get_mc_config(ctx, &mcc);
	uint32_t comp_size = (nva_rd32(ctx->cnum, 0x100320) + 1);
	uint32_t fb_size = 0xe00000;
	int i, j;
	clear_comp(ctx->cnum);
	clear_tile(ctx);
	for (i = 0; i < fb_size; i += 0x10) {
		vram_wr32(ctx->cnum, i+0x0, 0x0000ffff);
		vram_wr32(ctx->cnum, i+0x4, 0x00000000);
		vram_wr32(ctx->cnum, i+0x8, 0x00000000);
		vram_wr32(ctx->cnum, i+0xc, 0x00000000);
	}
	uint32_t pitch = 0xe000;
	int test_format;
	switch (comp_type(ctx->chipset)) {
		case COMP_NV20:
			test_format = 1;
			break;
		case COMP_NV25:
		case COMP_NV30:
		case COMP_NV35:
		case COMP_NV36:
			test_format = 2;
			break;
		case COMP_NV40:
			test_format = 7;
			break;
		default:
			abort();
	}
	set_tile(ctx, 0, 0, fb_size-1, pitch, 0);
	set_comp(ctx, 0, test_format, 0, 0xffffffff);
	int txstep = (comp_type(ctx->chipset) < COMP_NV40 ? 0x10 : 0x20);
	for (j = 0; j < fb_size; j += txstep) {
		while ((j / pitch) % 4)
			j += pitch;
		if (j >= fb_size)
			break;
		int part, tag;
		tile_translate_addr(ctx->chipset, pitch, j, 1, 0, &mcc, &part, &tag);
		comp_wr32(ctx->cnum, part, tag >> 3, 1 << (tag & 0x1f));
		if (tag < comp_size && !vram_rd32(ctx->cnum, j + 4)) {
			printf("%08x: expected to be part %d tag %05x", j, part, tag);
			int p, t;
			for (p = 0; p < (1 << mcc.partbits); p++)
				for (t = 0; t < comp_size; t++) {
					comp_wr32(ctx->cnum, p, t >> 3, 1 << (t & 0x1f));
					uint32_t mm = vram_rd32(ctx->cnum, j + 0x4);
					comp_wr32(ctx->cnum, p, t >> 3, 0);
					if (mm) {
						printf(", found at part %d tag %05x instead\n", p, t);
						goto found;
					}
				}
			printf(", but not found in tagspace\n");
found:
			clear_tile(ctx);
			return HWTEST_RES_FAIL;
		}
		comp_wr32(ctx->cnum, part, tag >> 3, 0);
	}
	clear_tile(ctx);
	return HWTEST_RES_PASS;
}

static int test_format(struct hwtest_ctx *ctx) {
	int i, j;
	struct mc_config mcc;
	get_mc_config(ctx, &mcc);
	int res = HWTEST_RES_PASS;
	int bankoff;
	int banktry = 1 << tile_bankoff_bits(ctx->chipset);
	clear_tile(ctx);
	for (i = 0; i < 0x800000; i += 4)
		vram_wr32(ctx->cnum, i, 0xc0000000 | i);
	for (i = 0; i < 0x100; i++) {
		uint32_t pitch = i << 8;
		if (!compute_status(ctx->chipset, pitch, 1))
			continue;
		for (bankoff = 0; bankoff < banktry; bankoff++) {
			set_tile(ctx, 0, 0, 0x3fffff, pitch, bankoff);
			for (j = 0; j < 0x400000; j += 4) {
				uint32_t real = vram_rd32(ctx->cnum, j);
				uint32_t exp = 0xc0000000 | tile_translate_addr(ctx->chipset, pitch, j, 1, bankoff, &mcc, 0, 0);
				if (real != exp) {
					printf("Mismatch at %08x for pitch %05x bankoff %d: is %08x, expected %08x\n", j, pitch, bankoff, real, exp);
					res = HWTEST_RES_FAIL;
					break;
				}
			}
		}
		for (j = 0x400000; j < 0x600000; j += 4) {
			uint32_t real = vram_rd32(ctx->cnum, j);
			uint32_t exp = 0xc0000000 | j;
			if (real != exp) {
				printf("Mismatch at %08x [after limit] for pitch %05x: is %08x, expected %08x\n", j, pitch, real, exp);
				res = HWTEST_RES_FAIL;
				break;
			}
		}
		unset_tile(ctx, 0);
	}
	return res;
}

static int test_comp_format(struct hwtest_ctx *ctx) {
	int res = HWTEST_RES_PASS;
	struct mc_config mcc;
	get_mc_config(ctx, &mcc);
	uint32_t pitch = 0x200;
	set_tile(ctx, 0, 0, 0xffff, pitch, 0);
	int i;
	int formats = comp_type(ctx->chipset) >= COMP_NV25 ? 0x20 : 0x10;
	if (comp_type(ctx->chipset) == COMP_NV36)
		formats = 0x10;
	int f;
	int tw;
	if (comp_type(ctx->chipset) < COMP_NV40)
		tw = 0x10;
	else
		tw = 0x20;
	int th = 4;
	int tsz = tw * th;
	int sh = 0x80;
	for (f = 0; f < formats; f++) {
		for (i = 0; i < 10000; i++) {
			int x = nrand48(ctx->rand48) % pitch;
			x &= -tw;
			int y = nrand48(ctx->rand48) % sh;
			y &= -th;
			uint32_t addr = x + y * pitch;
			int tag;
			int part;
			tile_translate_addr(ctx->chipset, pitch, addr, 1, 0, &mcc, &part, &tag);
			uint8_t src[0x80], dst[0x80], rdst[0x80];
			int k;
			comp_wr32(ctx->cnum, part, tag >> 3, 0);
			unset_comp(ctx, 0);
			for (k = 0; k < tsz; k++)
				src[k] = jrand48(ctx->rand48) & 0xff;
			for (k = 0; k < tsz; k += 4) {
				uint32_t val = 0;
				int z;
				for (z = 0; z < 4; z++)
					val |= src[k+z] << z*8;
				vram_wr32(ctx->cnum, addr + k % tw + k / tw * pitch, val);
			}
			int tv = jrand48(ctx->rand48) & 1;
			set_comp(ctx, 0, f, 0, 0xffffffff);
			comp_wr32(ctx->cnum, part, tag >> 3, tv << (tag & 0x1f));
			for (x = 0; x < tw; x++)
				for (y = 0; y < th; y++)
					rdst[x+y*tw] = vram_rd32(ctx->cnum, addr + (x & ~3) + y * pitch) >> 8 * (x & 3);
			memcpy(dst, src, tsz);
			comp_decompress(ctx->chipset, f, dst, tv);
			int fail = 0;
			for (k = 0; k < tsz; k++)
				if (rdst[k] != dst[k])
					fail = 1;
			if (fail) {
				const char *br = "", *er = "";
				if (ctx->colors)
					br = "\x1b[31m", er = "\x1b[0m";
				printf("COMP decompression mismatch iter %d: format %02x tv %d addr %08x part %d tag %05x\n", i, f, tv, addr, part, tag);
				printf("source:\n");
				for (y = 0; y < th; y++)
					for (x = 0; x < tw; x++) 
						printf("%02x%s", src[x+y*tw], x == tw-1 ? "\n" : (x & 3) == 3 ? "  " : " ");
				printf("expected:\n");
				for (y = 0; y < th; y++)
					for (x = 0; x < tw; x++) {
						int isred = dst[x+y*tw] != rdst[x+y*tw];
						printf("%s%02x%s%s", isred?br:"", dst[x+y*tw], isred?er:"", x == tw-1 ? "\n" : (x & 3) == 3 ? "  " : " ");
					}
				printf("real:\n");
				for (y = 0; y < th; y++)
					for (x = 0; x < tw; x++) {
						int isred = dst[x+y*tw] != rdst[x+y*tw];
						printf("%s%02x%s%s", isred?br:"", rdst[x+y*tw], isred?er:"", x == tw-1 ? "\n" : (x & 3) == 3 ? "  " : " ");
					}
				res = HWTEST_RES_FAIL;
				break;
			}
		}
	}
	clear_tile(ctx);
	return res;
}

static int nv10_tile_prep(struct hwtest_ctx *ctx) {
	if (pfb_type(ctx->chipset) < PFB_NV10 || pfb_type(ctx->chipset) > PFB_NV44)
		return HWTEST_RES_NA;
	if (!(nva_rd32(ctx->cnum, 0x200) & 1 << 20)) {
		printf("Mem controller not up.\n");
		return HWTEST_RES_UNPREP;
	}
	if (ctx->chipset >= 0x30 && ctx->chipset < 0x40) {
		/* accessing 1000f0/100100 is a horribly bad idea while any mem operations are in progress on NV30+ [at least NV31]... */
		/* tell PCRTC* to fuck off */
		nva_wr8(ctx->cnum, 0x0c03c4, 1);
		nva_wr8(ctx->cnum, 0x0c03c5, 0x20);
		/* tell vgacon to fuck off */
		nva_wr32(ctx->cnum, 0x001854, 0);
	}
	return HWTEST_RES_PASS;
}

static int nv20_comp_prep(struct hwtest_ctx *ctx) {
	if (pfb_type(ctx->chipset) < PFB_NV20 || pfb_type(ctx->chipset) > PFB_NV41)
		return HWTEST_RES_NA;
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(nv20_comp,
	HWTEST_TEST(test_comp_size, 0),
	HWTEST_TEST(test_comp_access, 0),
	HWTEST_TEST(test_comp_layout, 0),
	HWTEST_TEST(test_comp_format, 0),
)

HWTEST_DEF_GROUP(nv10_tile,
	HWTEST_TEST(test_scan, 0),
	HWTEST_TEST(test_status, 0),
	HWTEST_TEST(test_format, 1),
	HWTEST_GROUP(nv20_comp),
)
