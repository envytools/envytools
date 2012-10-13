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

#include "nvhw.h"
#include <stdlib.h>

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
