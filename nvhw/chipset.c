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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "nvhw/chipset.h"
#include "cgen/gpu.h"
#include <stdlib.h>
#include <string.h>

int parse_pmc_id(uint32_t pmc_id, struct chipset_info *info) {
	/* First, detect PMC_ID format and endian. There are four cases:
	 *
	 * - pre-NV10 card: always little endian, bits 7 and 31 are guaranteed
	 *   to be 0 (they're high bits of fields that never got big values)
	 * - NV10+ little-endian card: bit 7 is guaranteed to be 1 (MSB of
	 *   stepping, which is always 0xaX or 0xbX), bit 31 is guaranteed
	 *   to be 0
	 * - NV10+ big-endian card: like above, but with byteswapping - bit 7
	 *   is 0, bit 31 is 1
	 * - something is broken with BAR0 access and ID returns 0xffffffff
	 *   - both bit 7 and bit 31 are set
	 */
	memset(info, 0, sizeof *info);
	info->endian = 0;
	info->gpu = -1;
	info->gpu_desc = 0;
	if (pmc_id & 0x80000000) {
		/* bit 31 set - set endian flag and byteswap */
		info->endian = 1;
		pmc_id = (pmc_id & 0x0000ffff) << 16 | (pmc_id & 0xffff0000) >> 16;
		pmc_id = (pmc_id & 0x00ff00ff) << 8 | (pmc_id & 0xff00ff00) >> 8;
	}
	info->pmc_id = pmc_id;
	if (pmc_id & 0x80000000) {
		/* bit 31 still set - ie. old bit 7 was set - BAR0 is broken */
		return -1;
	}
	if (pmc_id & 0x80) {
		/* NV10+ */
		info->chipset = pmc_id >> 20 & 0x1ff;
		info->card_type = info->chipset & 0x1f0;
		if (info->card_type == 0x60) {
			info->card_type = 0x40;
		} else if (info->card_type >= 0x80 && info->card_type <= 0xa0) {
			info->card_type = 0x50;
		} else if (info->card_type >= 0xc0 && info->card_type <= 0x100) {
			info->card_type = 0xc0;
		}
		for (int i = 0; i < NUM_GPU; i++) {
			if (gpu_list[i].id_valid && gpu_list[i].id == info->chipset) {
				info->gpu = i;
			}
		}
	} else {
		/* pre-NV10 */
		if (pmc_id & 0xf000) {
			if (pmc_id & 0xf00000) {
				info->chipset = 5;
				info->gpu = GPU_NV5;
			} else {
				info->chipset = 4;
				info->gpu = GPU_NV4;
			}
			info->card_type = 4;
		} else {
			info->chipset = pmc_id >> 16 & 0xf;
			info->card_type = info->chipset;
			if (info->chipset == 1) {
				info->gpu = GPU_NV1;
			} else if (info->chipset == 3) {
				if ((pmc_id & 0xff) >= 0x20) {
					info->gpu = GPU_NV3T;
				} else {
					info->gpu = GPU_NV3;
				}
			}
		}
	}
	if (info->gpu != -1) {
		info->gpu_desc = &gpu_list[info->gpu];
		return 0;
	} else {
		return -1;
	}
}

int is_igp(int chipset) {
	switch (chipset) {
		case 0x0a:
		case 0x1a:
		case 0x1f:
		case 0x2a:
		case 0x4e:
		case 0x4c:
		case 0x67:
		case 0x68:
		case 0x63:
		case 0xaa:
		case 0xac:
		case 0xaf:
			return 1;
		default:
			return 0;
	}
}

int is_g7x(int chipset) {
	if (chipset < 0x46 || chipset == 0x4a || chipset == 0x4e || chipset == 0x50 || chipset >= 0x70)
		return 0;
	return 1;
}

int pfb_type(int chipset) {
	if (chipset <3)
		return PFB_NV01;
	if (chipset < 0x10)
		return PFB_NV03;
	if (chipset < 0x1a || chipset == 0x34)
		return PFB_NV10;
	if (chipset < 0x20)
		return PFB_NONE; /* IGP */
	if (chipset < 0x40)
		return PFB_NV20;
	if (chipset == 0x40 || chipset == 0x45)
		return PFB_NV40;
	if (chipset != 0x44 && chipset != 0x46 && chipset != 0x4a && chipset < 0x4c)
		return PFB_NV41;
	if (chipset != 0x50 && chipset < 0x80)
		return PFB_NV44;
	if (chipset < 0xc0)
		return PFB_NV50;
	return PFB_NVC0;
}

int get_maxparts(int chipset) {
	switch (chipset) {
		case 0x1a:
		case 0x1f:
		case 0x2a:
		case 0x4e:
		case 0x4c:
		case 0x67:
		case 0x68:
		case 0x63:
			return 0;
		case 0x10:
		case 0x15:
		case 0x11:
		case 0x17:
		case 0x18:
		case 0x20:
		case 0x25:
		case 0x28:
		case 0x30:
		case 0x35:
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x47:
		case 0x49:
			return 4;
		case 0x31:
		case 0x36:
		case 0x43:
		case 0x4b:
			return 2;
		case 0x44:
		case 0x4a:
		case 0x46:
			return 1;
		default:
			abort();
	}
}
