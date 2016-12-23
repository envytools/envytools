/*
 * Copyright (C) 2016 Marcin Kościelnicki <koriakin@0x04.net>
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

#include "pgraph.h"
#include "pgraph_mthd.h"
#include "pgraph_class.h"
#include "nva.h"

namespace hwtest {
namespace pgraph {

void MthdOperation::emulate_mthd() {
	if (!extr(exp.nsource, 1, 1)) {
		insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
		insrt(egrobj[0], 15, 3, val);
		exp.ctx_cache[subc][0] = exp.ctx_switch[0];
		insrt(exp.ctx_cache[subc][0], 15, 3, val);
		if (extr(exp.debug[1], 20, 1))
			exp.ctx_switch[0] = exp.ctx_cache[subc][0];
	}
}

void MthdPatch::emulate_mthd() {
	if (chipset.chipset < 5) {
		if (val < 2) {
			if (extr(exp.debug[1], 8, 1) && val == 0) {
				exp.ctx_cache[subc][0] = exp.ctx_switch[0];
				insrt(exp.ctx_cache[subc][0], 24, 1, 0);
				if (extr(exp.debug[1], 20, 1))
					exp.ctx_switch[0] = exp.ctx_cache[subc][0];
			} else {
				exp.intr |= 0x10;
				exp.fifo_enable = 0;
			}
		}
		if (!extr(exp.nsource, 1, 1) && !extr(exp.intr, 4, 1)) {
			insrt(egrobj[0], 24, 8, extr(exp.ctx_switch[0], 24, 8));
			insrt(egrobj[0], 24, 1, 0);
		}
	} else {
		exp.intr |= 0x10;
		exp.fifo_enable = 0;
	}
}

}
}
