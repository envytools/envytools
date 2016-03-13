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
	if (pmc_id & 0x80000000) {
		/* bit 31 set - set endian flag and byteswap */
		info->endian = 1;
		pmc_id = (pmc_id & 0x0000ffff) << 16 | (pmc_id & 0xffff0000) >> 16;
		pmc_id = (pmc_id & 0x00ff00ff) << 8 | (pmc_id & 0xff00ff00) >> 8;
	}
	if (pmc_id & 0x80000000) {
		/* bit 31 still set - ie. old bit 7 was set - BAR0 is broken */
		info->chipset = -1;
		info->name = "(disabled)";
		return -1;
	}
	info->pmc_id = pmc_id;
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
		switch (info->chipset) {
			/* celsius */
			case 0x10: info->name = "NV10"; break;
			case 0x15: info->name = "NV15"; break;
			case 0x1a: info->name = "NV1A"; break;
			case 0x11: info->name = "NV11"; break;
			case 0x17: info->name = "NV17"; break;
			case 0x18: info->name = "NV18"; break;
			case 0x1f: info->name = "NV1F"; break;

			/* kelvin */
			case 0x20: info->name = "NV20"; break;
			case 0x2a: info->name = "NV2A"; break;
			case 0x25: info->name = "NV25"; break;
			case 0x28: info->name = "NV28"; break;

			/* rankine */
			case 0x30: info->name = "NV30"; break;
			case 0x35: info->name = "NV35"; break;
			case 0x31: info->name = "NV31"; break;
			case 0x36: info->name = "NV36"; break;
			case 0x34: info->name = "NV34"; break;

			/* curie */
			case 0x40: info->name = "NV40"; break;
			case 0x45: info->name = "NV45"; break;
			case 0x41: info->name = "NV41"; break;
			case 0x42: info->name = "NV42"; break;
			case 0x43: info->name = "NV43"; break;
			case 0x44: info->name = "NV44"; break;
			case 0x4a: info->name = "NV44A"; break;
			case 0x4e: info->name = "C51"; break;

			/* curie2 */
			case 0x47: info->name = "G70"; break;
			case 0x49: info->name = "G71"; break;
			case 0x4b: info->name = "G73"; break;
			case 0x46: info->name = "G72"; break;
			case 0x4c: info->name = "MCP61"; break;
			case 0x67: info->name = "MCP67"; break;
			case 0x68: info->name = "MCP68"; break;
			case 0x63: info->name = "MCP73"; break;
			case 0x4d: info->name = "RSX"; break;

			/* tesla */
			case 0x50: info->name = "G80"; break;
			case 0x84: info->name = "G84"; break;
			case 0x86: info->name = "G86"; break;
			case 0x92: info->name = "G92"; break;
			case 0x94: info->name = "G94"; break;
			case 0x96: info->name = "G96"; break;
			case 0x98: info->name = "G98"; break;
			case 0xa0: info->name = "G200"; break;
			case 0xaa: info->name = "MCP77"; break;
			case 0xac: info->name = "MCP79"; break;

			/* tesla2 */
			case 0xa3: info->name = "GT215"; break;
			case 0xa5: info->name = "GT216"; break;
			case 0xa8: info->name = "GT218"; break;
			case 0xaf: info->name = "MCP89"; break;

			/* fermi */
			case 0xc0: info->name = "GF100"; break;
			case 0xc4: info->name = "GF104"; break;
			case 0xc3: info->name = "GF106"; break;
			case 0xce: info->name = "GF114"; break;
			case 0xcf: info->name = "GF116"; break;
			case 0xc1: info->name = "GF108"; break;
			case 0xc8: info->name = "GF110"; break;
			case 0xd9: info->name = "GF119"; break;
			case 0xd7: info->name = "GF117"; break;

			/* kepler */
			case 0xe4: info->name = "GK104"; break;
			case 0xe6: info->name = "GK106"; break;
			case 0xe7: info->name = "GK107"; break;
			case 0xea: info->name = "GK20A"; break;
			case 0xf0: info->name = "GK110"; break;
			case 0xf1: info->name = "GK110B"; break;
			case 0x108: info->name = "GK208"; break;
			case 0x106: info->name = "GK208B"; break;

			/* maxwell */
			case 0x117: info->name = "GM107"; break;
			case 0x118: info->name = "GM108"; break;
			case 0x120: info->name = "GM200"; break;
			case 0x124: info->name = "GM204"; break;
			case 0x126: info->name = "GM206"; break;
			case 0x12b: info->name = "GM20B"; break;

			/* wtf */
			default: info->name = "???";
		}
	} else {
		/* pre-NV10 */
		if (pmc_id & 0xf000) {
			if (pmc_id & 0xf00000) {
				info->chipset = 5;
				info->name = "NV05";
			} else {
				info->chipset = 4;
				info->name = "NV04";
			}
			info->card_type = 4;
		} else {
			info->chipset = pmc_id >> 16 & 0xf;
			info->card_type = info->chipset;
			if (info->chipset == 1) {
				info->name = "NV01";
			} else if (info->chipset == 3) {
				if ((pmc_id & 0xff) >= 0x20) {
					info->name = "NV03T";
					info->is_nv03t = 1;
				} else {
					info->name = "NV03";
				}
			} else {
				info->name = "???";
			}
		}
	}
	return 0;
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
